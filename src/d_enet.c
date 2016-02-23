#include <enet/enet.h>

#include "doomdef.h"
#include "doomstat.h"
#include "byteptr.h"
#include "d_enet.h"
#include "z_zone.h"
#include "m_menu.h"
#include "d_datawrap.h"

UINT8 net_nodecount, net_playercount;
UINT8 playernode[MAXPLAYERS];
SINT8 nodetoplayer[MAXNETNODES];
SINT8 nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
UINT8 playerpernode[MAXNETNODES]; // used specialy for scplitscreen
boolean nodeingame[MAXNETNODES]; // set false as nodes leave game

#define NETCHANNELS 4

enum {
	DISCONNECT_UNKNOWN = 0,
	DISCONNECT_SHUTDOWN,
	DISCONNECT_FULL,
	DISCONNECT_VERSION,

	CLIENT_ASKMSINFO = 0,
	CLIENT_JOIN,

	SERVER_MSINFO = 0,
	SERVER_MAPINFO
};

static ENetHost *ServerHost = NULL,
	*ClientHost = NULL;
static ENetPeer *nodetopeer[MAXNETNODES];
static UINT8 nodeleaving[MAXNETNODES];

typedef struct PeerData_s {
	UINT8 node;
} PeerData;

static void ServerSendMapInfo(UINT8 node);

boolean Net_GetNetStat(void)
{
	// set getbps, sendbps, gamelostpercent, lostpercent, etc.
	return false;
}

static void ServerHandlePacket(UINT8 node, DataWrap data)
{
	switch(data->ReadUINT8(data))
	{
	case CLIENT_JOIN:
	{
		UINT16 version = data->ReadUINT16(data);
		UINT16 subversion = data->ReadUINT16(data);
		if (version != VERSION || subversion != SUBVERSION)
		{
			CONS_Printf("NETWORK: Version mismatch!?\n");
			nodeleaving[node] = true;
			enet_peer_disconnect(nodetopeer[node], DISCONNECT_VERSION);
			break;
		}
		char *name = data->ReadStringn(data, MAXPLAYERNAME);
		CONS_Printf("NETWORK: Player '%s' joining...\n", name);
		net_playercount++;
		ServerSendMapInfo(node);
		break;
	}
	default:
		CONS_Printf("NETWORK: Unknown message type recieved from node %u!\n", node);
		break;
	}
}

static void ClientHandlePacket(UINT8 node, DataWrap data)
{
	switch(data->ReadUINT8(data))
	{
	case SERVER_MAPINFO:
	{
		INT16 mapnum = data->ReadINT16(data);
		gametype = data->ReadINT16(data);
		G_InitNew(false, G_BuildMapName(mapnum), true, true);
		break;
	}
	default:
		CONS_Printf("NETWORK: Unknown message type recieved from node %u!\n", node);
		break;
	}
}

void Net_AckTicker(void)
{
	ENetEvent e;
	UINT8 i;
	PeerData *pdata;
	jmp_buf safety;

	while (ClientHost && enet_host_service(ClientHost, &e, 0) > 0)
		switch (e.type)
		{
		case ENET_EVENT_TYPE_DISCONNECT:
			if (!server)
			{
				CL_Reset();
				D_StartTitle();
				switch(e.data)
				{
				case DISCONNECT_SHUTDOWN:
					M_StartMessage(M_GetText("Server shut down.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				case DISCONNECT_FULL:
					M_StartMessage(M_GetText("Server is full.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				default:
					M_StartMessage(M_GetText("Disconnected from server.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				}
			}
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			pdata = (PeerData *)e.peer->data;
			if (setjmp(safety))
				CONS_Printf("NETWORK: There was an EOF error in a recieved packet from server! len %u\n", e.packet->dataLength);
			else
				ClientHandlePacket(pdata->node, D_NewDataWrap(e.packet->data, e.packet->dataLength, &safety));
			enet_packet_destroy(e.packet);
			break;

		default:
			break;
		}

	while (ServerHost && enet_host_service(ServerHost, &e, 0) > 0)
		switch (e.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			// turn away new connections when maxplayers is hit
			// TODO: wait to see if it's a remote console first or something.
			if (net_playercount >= cv_maxplayers.value)
			{
				CONS_Printf("NETWORK: New connection tossed, server is full.\n");
				enet_peer_disconnect(e.peer, DISCONNECT_FULL);
				break;
			}

			for (i = 0; i < MAXNETNODES && nodeingame[i]; i++)
				;
			I_Assert(i < MAXNETNODES); // ENet should not be able to send connect events when nodes are full.

			net_nodecount++;
			nodeingame[i] = true;
			nodetopeer[i] = e.peer;

			pdata = ZZ_Alloc(sizeof(*pdata));
			pdata->node = i;
			e.peer->data = pdata;

			CONS_Printf("NETWORK: Node %u connected.\n", i);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			if (!e.peer->data)
				break;
			pdata = (PeerData *)e.peer->data;
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
			pdata = (PeerData *)e.peer->data;
			if (setjmp(safety))
				CONS_Printf("NETWORK: There was an EOF error in a recieved packet! Node %u, len %u\n", pdata->node, e.packet->dataLength);
			else
				ServerHandlePacket(pdata->node, D_NewDataWrap(e.packet->data, e.packet->dataLength, &safety));
			enet_packet_destroy(e.packet);
			break;

		default:
			break;
		}
}

void D_NetOpen(void)
{
	ENetAddress address = { ENET_HOST_ANY, 5029 };
	ServerHost = enet_host_create(&address, MAXNETNODES, NETCHANNELS, 0, 0);
	if (!ServerHost)
		I_Error("ENet failed to open server host. (Check if the port is in use?)");

	servernode = 0;
	nodeingame[servernode] = true;
	net_nodecount = 1;
	net_playercount = 0;
}

void D_NetConnect(const char *hostname, const char *port)
{
	ENetAddress address;
	ENetEvent e;

	ClientHost = enet_host_create(NULL, 1, NETCHANNELS, 0, 0);
	if (!ClientHost)
		I_Error("ENet failed to initialize client host.");

	netgame = multiplayer = true;
	servernode = 0;

	enet_address_set_host(&address, hostname);
	address.port = 5029;
	if (port != NULL)
		address.port = atoi(port) || address.port;

	nodetopeer[servernode] = enet_host_connect(ClientHost, &address, NETCHANNELS, 0);
	if (!nodetopeer[servernode])
		I_Error("Failed to allocate ENet peer for connecting ???");
	nodeingame[servernode] = true;

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
		UINT8 i, waiting=0;
		// tell everyone to go away
		for (i = 0; i < MAXNETNODES; i++)
			if (nodeingame[i] && servernode != i)
			{
				enet_peer_disconnect(nodetopeer[i], DISCONNECT_SHUTDOWN);
				waiting++;
			}
		// wait for messages to go through.
		while (waiting > 0 && enet_host_service(ServerHost, &e, 3000) > 0)
			switch (e.type)
			{
			// i don't care, shut up.
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(e.packet);
				break;
			// good, go away.
			case ENET_EVENT_TYPE_DISCONNECT:
				waiting--;
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
		enet_peer_disconnect(nodetopeer[servernode], DISCONNECT_SHUTDOWN);
		while (enet_host_service(ServerHost, &e, 3000) > 0)
		{
			if (e.type == ENET_EVENT_TYPE_DISCONNECT)
				break;
			else switch (e.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				enet_packet_destroy(e.packet);
				break;
			case ENET_EVENT_TYPE_CONNECT:
				// how the what ???
				enet_peer_reset(e.peer);
				break;
			}
		}
		enet_host_destroy(ClientHost);
	}

	netgame = false;
	addedtogame = false;
	servernode = 0;
	net_nodecount = net_playercount = 0;
}

void Net_CloseConnection(INT32 node)
{
	if (nodeleaving[node] || nodetopeer[node] == NULL)
		return;
	nodeleaving[node] = true;
	enet_peer_disconnect(nodetopeer[node], 0);
}

// Client: Can I play? =3 My name is Player so-and-so!
void Net_SendJoin(void)
{
	ENetPacket *packet;
	UINT8 data[5+MAXPLAYERNAME];
	UINT8 *buf = data;

	WRITEUINT8(buf, CLIENT_JOIN);
	WRITEUINT16(buf, VERSION);
	WRITEUINT16(buf, SUBVERSION);
	WRITESTRINGN(buf, cv_playername.string, MAXPLAYERNAME);

	packet = enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(nodetopeer[servernode], 0, packet);
}

static void ServerSendMapInfo(UINT8 node)
{
	ENetPacket *packet;
	UINT8 data[5];
	UINT8 *buf = data;

	WRITEUINT8(buf, SERVER_MAPINFO);
	WRITEINT16(buf, gamemap);
	WRITEINT16(buf, gametype);

	packet = enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(nodetopeer[node], 0, packet);
}
