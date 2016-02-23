#include <enet/enet.h>

#include "doomdef.h"
#include "doomstat.h"
#include "d_enet.h"
#include "z_zone.h"
#include "m_menu.h"

UINT8 net_nodecount, net_playercount;
UINT8 playernode[MAXPLAYERS];
SINT8 nodetoplayer[MAXNETNODES];
SINT8 nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
UINT8 playerpernode[MAXNETNODES]; // used specialy for scplitscreen
boolean nodeingame[MAXNETNODES]; // set false as nodes leave game

#define NETCHANNELS 4

#define DISCONNECT_SHUTDOWN 1

static ENetHost *ServerHost = NULL,
	*ClientHost = NULL;
static ENetPeer *nodetopeer[MAXNETNODES];
static UINT8 nodeleaving[MAXNETNODES];

typedef struct PeerData_s {
	UINT8 node;
} PeerData_t;

boolean Net_GetNetStat(void)
{
	// set getbps, sendbps, gamelostpercent, lostpercent, etc.
	return false;
}

void Net_AckTicker(void)
{
	ENetEvent e;
	UINT8 i;
	PeerData_t *pdata;

	while (ClientHost && enet_host_service(ClientHost, &e, 0) > 0)
		switch (e.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			if (!server)
			{
				CL_Reset();
				D_StartTitle();
				if (e.data == DISCONNECT_SHUTDOWN)
					M_StartMessage(M_GetText("Server shut down.\n\nPress ESC\n"), NULL, MM_NOTHING);
				else
					M_StartMessage(M_GetText("Disconnected from server.\n\nPress ESC\n"), NULL, MM_NOTHING);
			}
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy(e.packet);
			break;
		default:
			break;
		}

	while (ServerHost && enet_host_service(ServerHost, &e, 0) > 0)
		switch (e.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			for (i = 0; i < MAXNETNODES && nodetopeer[i]; i++)
				;
			I_Assert(i < MAXNETNODES); // ENet should not be able to send connect events when nodes are full.
			nodetopeer[i] = e.peer;
			pdata = ZZ_Alloc(sizeof(*pdata));
			pdata->node = i;
			e.peer->data = pdata;
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			pdata = (PeerData_t *)e.peer->data;
			if (!nodeleaving[pdata->node])
			{
				XBOXSTATIC UINT8 buf[2];
				buf[0] = nodetoplayer[pdata->node];
				buf[1] = KICK_MSG_PLAYER_QUIT;
				SendNetXCmd(XD_KICK, &buf, 2);
				if (playerpernode[pdata->node] == 2)
				{
					buf[0] = nodetoplayer2[pdata->node];
					SendNetXCmd(XD_KICK, &buf, 2);
				}
			}
			net_nodecount--;
			nodeleaving[pdata->node] = false;
			nodetopeer[pdata->node] = NULL;
			Z_Free(pdata);
			e.peer->data = NULL;
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			enet_packet_destroy(e.packet);
			break;
		default:
			break;
		}
}

boolean Net_AllAckReceived(void)
{
	return true;
}

void D_SetDoomcom(void)
{
	net_nodecount = 0;
	net_playercount = 0;
}

void D_NetOpen(void)
{
	ENetAddress address = { ENET_HOST_ANY, 5029 };
	ServerHost = enet_host_create(&address, MAXNETNODES, NETCHANNELS, 0, 0);
	if (!ServerHost)
		I_Error("ENet failed to open server host. (Check if the port is in use?)");
	servernode = 0;
}

void D_NetConnect(const char *hostname, const char *port)
{
	ENetAddress address;
	ENetEvent e;

	ClientHost = enet_host_create(NULL, 1, NETCHANNELS, 0, 0);
	if (!ClientHost)
		I_Error("ENet failed to initialize client host.");

	netgame = multiplayer = true;
	servernode = 1;

	enet_address_set_host(&address, hostname);
	address.port = 5029;
	if (port != NULL)
		address.port = atoi(port) || address.port;

	nodetopeer[servernode] = enet_host_connect(ClientHost, &address, NETCHANNELS, 0);
	if (!nodetopeer[servernode])
		I_Error("Failed to allocate ENet peer for connecting ???");

	if (enet_host_service(ClientHost, &e, 5000) > 0
	&& e.type == ENET_EVENT_TYPE_CONNECT)
	{
		CONS_Printf("Connection successful!");
		return;
	}
	M_StartMessage(M_GetText("Failed to connect to server.\n\nPress ESC\n"), NULL, MM_NOTHING);
}

// Initialize network.
// Returns true if the server is booting up right into a level according to startup args and whatnot.
// netgame is set to true before this is called if -server was passed.
boolean D_CheckNetGame(void)
{
	if (enet_initialize())
		I_Error("Failed to initialize ENet.\n");
	if (netgame)
	{
		if (server)
			D_NetOpen();
	}
	else
		server = true;
	multiplayer = netgame;
	D_ClientServerInit();
	return netgame;
}

void D_CloseConnection(void)
{
	ENetEvent e;
	if (ServerHost)
	{
		UINT8 i;
		// tell everyone to go away
		for (i = 0; i < MAXNETNODES; i++)
			if (nodeingame[i])
				enet_peer_disconnect(nodetopeer[i], DISCONNECT_SHUTDOWN);
		// wait for messages to go through.
		while (enet_host_service(ServerHost, &e, 3000) > 0)
			switch (e.type)
			{
			// i don't care, shut up.
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(e.packet);
				break;
			// good, go away.
			case ENET_EVENT_TYPE_DISCONNECT:
				break;
			// no, we're shutting down.
			case ENET_EVENT_TYPE_CONNECT:
				enet_peer_reset(e.peer);
				break;
			}
		// alright, we're finished.
		enet_host_destroy(ServerHost);
	}
	if (ClientHost)
	{
		enet_peer_disconnect(nodetopeer[servernode], 0);
		while (enet_host_service(ServerHost, &e, 3000) > 0)
			switch (e.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(e.packet);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				break;
			case ENET_EVENT_TYPE_CONNECT:
				// how the what ???
				enet_peer_reset(e.peer);
				break;
			}
		enet_host_destroy(ClientHost);
	}
	netgame = false;
	addedtogame = false;
	servernode = 0;
}

void Net_UnAcknowledgPacket(INT32 node)
{
}

void Net_CloseConnection(INT32 node)
{
	if (nodeleaving[node] || nodetopeer[node] == NULL)
		return;
	nodeleaving[node] = true;
	enet_peer_disconnect(nodetopeer[node], 0);
}

void Net_AbortPacketType(UINT8 packettype)
{
}

void Net_SendAcks(INT32 node)
{
}

void Net_WaitAllAckReceived(UINT32 timeout)
{
}
