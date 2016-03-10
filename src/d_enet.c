#include <enet/enet.h>

#include "doomdef.h"
#include "doomstat.h"
#include "byteptr.h"
#include "d_enet.h"
#include "z_zone.h"
#include "m_menu.h"
#include "d_datawrap.h"
#include "g_game.h"
#include "p_local.h"
#include "d_main.h"
#include "i_system.h"

UINT8 net_nodecount, net_playercount;
UINT8 playernode[MAXPLAYERS];
SINT8 nodetoplayer[MAXNETNODES];
SINT8 nodetoplayer2[MAXNETNODES]; // say the numplayer for this node if any (splitscreen)
UINT8 playerpernode[MAXNETNODES]; // used specialy for scplitscreen
boolean nodeingame[MAXNETNODES]; // set false as nodes leave game

#define MAX_SERVER_MESSAGE 320

static tic_t lastMove;
static ticcmd_t lastCmd;

enum {
	CHANNEL_GENERAL = 0,
	CHANNEL_CHAT,
	CHANNEL_MOVE,
	NET_CHANNELS,

	DISCONNECT_UNKNOWN = 0,
	DISCONNECT_SHUTDOWN,
	DISCONNECT_FULL,
	DISCONNECT_VERSION,

	CLIENT_ASKMSINFO = 0,
	CLIENT_JOIN,
	CLIENT_CHAT,
	CLIENT_CHARACTER,
	CLIENT_MOVE,

	SERVER_MSINFO = 0,
	SERVER_MAPINFO,
	SERVER_MESSAGE,
	SERVER_SPAWN,
	SERVER_REMOVE,
	SERVER_MOVE
};

static ENetHost *ServerHost = NULL,
	*ClientHost = NULL;
static ENetPeer *nodetopeer[MAXNETNODES];

typedef struct {
	ticcmd_t cmd;
	fixed_t x,y,z,
		vx,vy,vz,
		ax,ay,az;
	angle_t angle, aiming;
	enum state state;
} GhostData;

typedef struct PeerData_s {
	UINT8 node;
	UINT8 flags;
	char name[MAXPLAYERNAME+1];
	GhostData ghost;
} PeerData;

enum {
	PEER_LEAVING = 1
};

static UINT8 mynode;

void Net_ServerMessage(const char *fmt, ...);
static void ServerSendMapInfo(UINT8 node);
static void Net_SendMove(void);
static void Net_MovePlayers(void);

boolean Net_GetNetStat(void)
{
	// set getbps, sendbps, gamelostpercent, lostpercent, etc.
	return false;
}

static void DisconnectNode(UINT8 node, UINT8 why)
{
	if (nodetopeer[node] == NULL)
		return;
	PeerData *pdata = nodetopeer[node]->data;
	pdata->flags |= PEER_LEAVING;
	enet_peer_disconnect(nodetopeer[node], why);
}

static void ServerHandlePacket(UINT8 node, DataWrap data)
{
	PeerData *pdata = nodetopeer[node]->data;

	switch(DW_ReadUINT8(data))
	{
	case CLIENT_JOIN:
	{
		UINT16 version = DW_ReadUINT16(data);
		UINT16 subversion = DW_ReadUINT16(data);

		if (version != VERSION || subversion != SUBVERSION)
		{
			CONS_Printf("NETWORK: Version mismatch!?\n");
			DisconnectNode(node, DISCONNECT_VERSION);
			break;
		}

		char *name = DW_ReadStringn(data, MAXPLAYERNAME);
		strcpy(pdata->name, name);
		Z_Free(name);

		Net_ServerMessage("%s connected.", pdata->name);

		net_playercount++;
		ServerSendMapInfo(node);
		break;
	}

	case CLIENT_CHAT:
		Net_ServerMessage("\3<%s> %s", pdata->name, DW_ReadStringn(data, 256));
		break;

	case CLIENT_CHARACTER:
	{
		UINT8 pnum, i;
		// this message might be sent when the player first spawns
		if (nodetoplayer[node] == -1)
		{
			for (pnum = 0; pnum < MAXPLAYERS; pnum++)
				if (!playeringame[pnum])
					break;
			I_Assert(pnum != MAXPLAYERS);
			CL_ClearPlayer(pnum);
			playeringame[pnum] = true;
			G_AddPlayer(pnum);
			playernode[pnum] = node;
			nodetoplayer[node] = pnum;

			for (i = 0; i < MAXPLAYERS; i++)
				if (i != pnum && playeringame[i])
					Net_SpawnPlayer(i, node);
		}
		pnum = nodetoplayer[node];
		SetPlayerSkin(pnum, DW_ReadStringn(data, SKINNAMESIZE));
		players[pnum].skincolor = DW_ReadUINT8(data) % MAXSKINCOLORS;
		break;
	}

	case CLIENT_MOVE:
	{
		player_t *player;
		GhostData ghost;

		ghost.cmd.forwardmove = DW_ReadSINT8(data);
		ghost.cmd.sidemove = DW_ReadSINT8(data);
		ghost.cmd.angleturn = DW_ReadINT16(data);
		ghost.cmd.aiming = DW_ReadINT16(data);
		ghost.cmd.buttons = DW_ReadUINT16(data);
		ghost.x = DW_ReadINT16(data) << 16;
		ghost.y = DW_ReadINT16(data) << 16;
		ghost.z = DW_ReadINT16(data) << 16;

		// TODO: Compare new data to old data, etc.
		memcpy(&pdata->ghost, &ghost, sizeof(ghost));

		if (nodetoplayer[node] == -1)
			break;

		player = &players[nodetoplayer[node]];
		player->cmd.forwardmove = ghost.cmd.forwardmove;
		player->cmd.sidemove = ghost.cmd.sidemove;
		player->cmd.angleturn = ghost.cmd.angleturn;
		player->cmd.aiming = ghost.cmd.aiming;
		player->cmd.buttons = ghost.cmd.buttons;
		P_TeleportMove(player->mo, ghost.x, ghost.y, ghost.z);
		break;
	}

	default:
		CONS_Printf("NETWORK: Unknown message type recieved from node %u!\n", node);
		break;
	}
}

void CL_ConnectionSuccessful(void);

static void ClientHandlePacket(UINT8 node, DataWrap data)
{
	switch(DW_ReadUINT8(data))
	{
	case SERVER_MAPINFO:
	{
		CL_ConnectionSuccessful();
		mynode = DW_ReadUINT8(data);
		CONS_Printf("NETWORK: I'm player %u\n", mynode);
		gamemap = DW_ReadINT16(data);
		gametype = DW_ReadINT16(data);
		G_InitNew(false, G_BuildMapName(gamemap), true, true);
		M_StartControlPanel();
		M_SetupNetgameChoosePlayer();
		break;
	}

	case SERVER_MESSAGE:
	{
		char *msg = DW_ReadStringn(data, MAX_SERVER_MESSAGE);
		CONS_Printf("%s\n", msg);
		Z_Free(msg);
		break;
	}

	case SERVER_SPAWN:
	{
		UINT16 id = DW_ReadUINT16(data);

		if (id == mynode+1)
			break;

		// Spawn a player.
		if (id < 1000)
		{
			const fixed_t x = DW_ReadINT16(data) << 16,
				y = DW_ReadINT16(data) << 16,
				z = DW_ReadINT16(data) << 16;
			mobj_t *mobj = P_SpawnMobj(x, y, z, MT_PLAYER);
			mobj->flags &= ~MF_SOLID;
			mobj->mobjnum = id;
			mobj->angle = DW_ReadUINT8(data) << 24;
			mobj->skin = &skins[DW_ReadUINT8(data)];
			mobj->color = DW_ReadUINT8(data);
		}
		break;
	}

	case SERVER_MOVE:
	{
		thinker_t *th;
		mobj_t *mobj = NULL;
		UINT16 id = DW_ReadUINT16(data);
		fixed_t x,y,z;

		if (id == mynode+1)
			break;

		for (th = thinkercap.next; th != &thinkercap; th = th->next)
			if (th->function.acp1 == (actionf_p1)P_MobjThinker && ((mobj_t *)th)->mobjnum == id)
			{
				mobj = (mobj_t *)th;
				break;
			}

		if (!mobj)
		{
			//CONS_Printf("NETWORK: Failed to find mobj with id %u\n", id);
			break;
		}

		x = DW_ReadINT16(data) << 16,
		y = DW_ReadINT16(data) << 16,
		z = DW_ReadINT16(data) << 16;
		P_MapStart();
		P_TeleportMove(mobj, x, y, z);
		P_MapEnd();

		mobj->momx = DW_ReadINT16(data) << 8,
		mobj->momy = DW_ReadINT16(data) << 8,
		mobj->momz = DW_ReadINT16(data) << 8;

		mobj->angle = DW_ReadUINT8(data) << 24;

		mobj->sprite = SPR_PLAY;
		mobj->sprite2 = DW_ReadUINT8(data);
		mobj->frame = DW_ReadUINT8(data);
		mobj->tics = -1;
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

	Net_SendMove();
	Net_MovePlayers();

	while (ClientHost && enet_host_service(ClientHost, &e, 0) > 0)
		switch (e.type)
		{
		case ENET_EVENT_TYPE_DISCONNECT:
			if (!server)
			{
				INT32 disconnect_type = e.data;
				nodetopeer[servernode] = NULL;
				enet_host_destroy(ClientHost);
				ClientHost = NULL;

				CL_Reset();
				D_StartTitle();
				switch(disconnect_type)
				{
				case DISCONNECT_SHUTDOWN:
					M_StartMessage(M_GetText("Server shutting down.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				case DISCONNECT_FULL:
					M_StartMessage(M_GetText("Server is full.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				case DISCONNECT_VERSION:
					M_StartMessage(M_GetText("Game version mismatch.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				default:
					M_StartMessage(M_GetText("Disconnected from server.\n\nPress ESC\n"), NULL, MM_NOTHING);
					break;
				}
			}
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			if (setjmp(safety))
				CONS_Printf("NETWORK: There was an EOF error in a recieved packet from server! len %u\n", e.packet->dataLength);
			else
				ClientHandlePacket(servernode, D_NewDataWrap(e.packet->data, e.packet->dataLength, &safety));
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
			pdata->flags = 0;
			strcpy(pdata->name, "New Player");
			memset(&pdata->ghost, 0, sizeof(GhostData));

			e.peer->data = pdata;

			CONS_Printf("NETWORK: Node %u connected.\n", i);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			if (!e.peer->data)
				break;
			pdata = (PeerData *)e.peer->data;
			if (!(pdata->flags & PEER_LEAVING))
				Net_ServerMessage("%s disconnected.", pdata->name);
			if (playeringame[nodetoplayer[pdata->node]])
				CL_RemovePlayer(nodetoplayer[pdata->node]);
			net_nodecount--;
			nodetopeer[pdata->node] = NULL;
			nodeingame[pdata->node] = false;
			Z_Free(pdata);
			e.peer->data = NULL;
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			pdata = (PeerData *)e.peer->data;
			if (!(pdata->flags & PEER_LEAVING))
			{
				if (setjmp(safety))
					CONS_Printf("NETWORK: There was an EOF error in a recieved packet! Node %u, len %u\n", pdata->node, e.packet->dataLength);
				else
					ServerHandlePacket(pdata->node, D_NewDataWrap(e.packet->data, e.packet->dataLength, &safety));
			}
			enet_packet_destroy(e.packet);
			break;

		default:
			break;
		}
}

void D_NetOpen(void)
{
	ENetAddress address = { ENET_HOST_ANY, 5029 };
	ServerHost = enet_host_create(&address, MAXNETNODES, NET_CHANNELS, 0, 0);
	if (!ServerHost)
		I_Error("ENet failed to open server host. (Check if the port is in use?)");

	servernode = 0;
	nodeingame[servernode] = true;
	net_nodecount = 1;
	net_playercount = 0;
	lastMove = I_GetTime();
}

boolean D_NetConnect(const char *hostname, const char *port)
{
	ENetAddress address;
	ENetEvent e;

	ClientHost = enet_host_create(NULL, 1, NET_CHANNELS, 0, 0);
	if (!ClientHost)
		I_Error("ENet failed to initialize client host.\n");

	netgame = multiplayer = true;
	servernode = 1;
	lastMove = I_GetTime();
	memset(&lastCmd, 0, sizeof(ticcmd_t));

	enet_address_set_host(&address, hostname);
	address.port = 5029;
	if (port != NULL)
		address.port = atoi(port) || address.port;

	nodetopeer[servernode] = enet_host_connect(ClientHost, &address, NET_CHANNELS, 0);
	if (!nodetopeer[servernode])
		I_Error("Failed to allocate ENet peer for connecting ???\n");
	nodeingame[servernode] = true;

	if (enet_host_service(ClientHost, &e, 5000) > 0
	&& e.type == ENET_EVENT_TYPE_CONNECT)
	{
		CONS_Printf("NETWORK: Connection successful!\n");
		return true;
	}

	CONS_Printf("NETWORK: Connection failed...\n");
	servernode = 0;
	enet_host_destroy(ClientHost);
	ClientHost = NULL;
	return false;
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

			default:
				break;
			}

		// alright, we're finished.
		enet_host_destroy(ServerHost);
		ServerHost = NULL;
	}

	if (ClientHost)
	{
		enet_peer_disconnect(nodetopeer[servernode], DISCONNECT_SHUTDOWN);
		nodeingame[servernode] = false;
		servernode = 0;

		while (enet_host_service(ClientHost, &e, 3000) > 0)
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

			default:
				break;
			}
		}

		enet_host_destroy(ClientHost);
		ClientHost = NULL;
	}

	netgame = false;
	addedtogame = false;
	servernode = 0;
	net_nodecount = net_playercount = 0;
}

void Net_CloseConnection(INT32 node)
{
	DisconnectNode(node, 0);
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
	enet_peer_send(nodetopeer[servernode], CHANNEL_GENERAL, packet);
}

static void ServerSendMapInfo(UINT8 node)
{
	ENetPacket *packet;
	UINT8 data[5];
	UINT8 *buf = data;

	WRITEUINT8(buf, SERVER_MAPINFO);
	WRITEUINT8(buf, node);
	WRITEINT16(buf, gamemap);
	WRITEINT16(buf, gametype);

	packet = enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(nodetopeer[node], CHANNEL_GENERAL, packet);
}

void Net_ServerMessage(const char *fmt, ...)
{
	va_list argptr;
	UINT8 data[1+MAX_SERVER_MESSAGE];
	UINT8 *buf = data;

	WRITEUINT8(buf, SERVER_MESSAGE);
	va_start(argptr, fmt);
	vsnprintf((char *)buf, MAX_SERVER_MESSAGE, fmt, argptr);
	va_end(argptr);
	buf += strlen((char *)buf)+1;

	CONS_Printf("%s\n", data+1);
	enet_host_broadcast(ServerHost, 0, enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE));
}

void Net_SendChat(char *line)
{
	ENetPacket *packet;
	UINT8 data[MAX_SERVER_MESSAGE];
	UINT8 *buf = data;

	if (server)
	{
		WRITEUINT8(buf, SERVER_MESSAGE);
		sprintf((char *)buf, "\3<~%s> %s", cv_playername.string, line);
		buf += strlen((char *)buf)+1;
		CONS_Printf("%s\n", data+1);
	}
	else
	{
		WRITEUINT8(buf, CLIENT_CHAT);
		WRITESTRINGN(buf, line, 256);
	}

	packet = enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE);
	if (server)
		enet_host_broadcast(ServerHost, CHANNEL_CHAT, packet);
	else
		enet_peer_send(nodetopeer[servernode], CHANNEL_CHAT, packet);
}

void Net_SendCharacter(void)
{
	ENetPacket *packet;
	UINT8 data[1+SKINNAMESIZE+1];
	UINT8 *buf = data;

	if (server)
		return;

	WRITEUINT8(buf, CLIENT_CHARACTER);
	WRITESTRINGN(buf, cv_skin.string, SKINNAMESIZE);
	WRITEUINT8(buf, cv_playercolor.value);

	packet = enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(nodetopeer[servernode], CHANNEL_GENERAL, packet);
}

static void Net_SendMove(void)
{
	ENetPacket *packet;
	UINT8 data[15];
	UINT8 *buf = data;

	if (server || !addedtogame || !players[consoleplayer].mo)
		return;

	// only update once a second unless buttons changed.
	if (lastMove+TICRATE < I_GetTime() && !memcmp(&lastCmd, &players[consoleplayer].cmd, sizeof(ticcmd_t)))
		return;
	lastMove = I_GetTime();
	G_CopyTiccmd(&lastCmd, &players[consoleplayer].cmd, 1);

	WRITEUINT8(buf, CLIENT_MOVE);
	WRITESINT8(buf, players[consoleplayer].cmd.forwardmove);
	WRITESINT8(buf, players[consoleplayer].cmd.sidemove);
	WRITEINT16(buf, players[consoleplayer].cmd.angleturn);
	WRITEINT16(buf, players[consoleplayer].cmd.aiming);
	WRITEUINT16(buf, players[consoleplayer].cmd.buttons);
	WRITEINT16(buf, players[consoleplayer].mo->x >> 16);
	WRITEINT16(buf, players[consoleplayer].mo->y >> 16);
	WRITEINT16(buf, players[consoleplayer].mo->z >> 16);

	packet = enet_packet_create(data, buf-data, 0);
	enet_peer_send(nodetopeer[servernode], CHANNEL_MOVE, packet);
}

void Net_SpawnPlayer(UINT8 pnum, UINT8 node)
{
	ENetPacket *packet;
	UINT8 data[12];
	UINT8 *buf = data;

	WRITEUINT8(buf, SERVER_SPAWN);
	WRITEUINT16(buf, pnum+1);
	WRITEINT16(buf, players[pnum].mo->x >> 16);
	WRITEINT16(buf, players[pnum].mo->y >> 16);
	WRITEINT16(buf, players[pnum].mo->z >> 16);
	WRITEUINT8(buf, players[pnum].mo->angle >> 24);
	WRITEUINT8(buf, players[pnum].skin);
	WRITEUINT8(buf, players[pnum].skincolor);

	packet = enet_packet_create(data, buf-data, ENET_PACKET_FLAG_RELIABLE);
	if (node == 0)
		enet_host_broadcast(ServerHost, CHANNEL_MOVE, packet);
	else
		enet_peer_send(nodetopeer[node], CHANNEL_MOVE, packet);
}

static void Net_MovePlayers(void)
{
	ENetPacket *packet;
	UINT8 data[18], *buf, i;

	if (!server || lastMove == I_GetTime())
		return;

	lastMove = I_GetTime();

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || !players[i].mo)
			continue;

		buf = data;
		WRITEUINT8(buf, SERVER_MOVE);
		WRITEUINT16(buf, i+1);
		WRITEINT16(buf, players[i].mo->x >> 16);
		WRITEINT16(buf, players[i].mo->y >> 16);
		WRITEINT16(buf, players[i].mo->z >> 16);
		WRITEINT16(buf, players[i].mo->momx >> 8);
		WRITEINT16(buf, players[i].mo->momy >> 8);
		WRITEINT16(buf, players[i].mo->momz >> 8);
		WRITEUINT8(buf, players[i].mo->angle >> 24);
		if (players[i].mo->sprite == SPR_PLAY)
			WRITEUINT8(buf, players[i].mo->sprite2);
		else
			WRITEUINT8(buf, SPR2_STND);
		WRITEUINT8(buf, players[i].mo->frame);

		packet = enet_packet_create(data, buf-data, 0);
		enet_host_broadcast(ServerHost, CHANNEL_MOVE, packet);
	}
}
