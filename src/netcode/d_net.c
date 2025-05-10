// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_net.c
/// \brief SRB2 network game communication and protocol, all OS independent parts.
//
///        Implement a Sliding window protocol without receiver window
///        (out of order reception)
///        This protocol uses a mix of "goback n" and "selective repeat" implementation
///        The NOTHING packet is sent when connection is idle to acknowledge packets

#include "../doomdef.h"
#include "../g_game.h"
#include "../i_time.h"
#include "i_net.h"
#include "../i_system.h"
#include "../m_argv.h"
#include "d_net.h"
#include "../w_wad.h"
#include "d_netfil.h"
#include "d_clisrv.h"
#include "tic_command.h"
#include "net_command.h"
#include "../z_zone.h"
#include "i_tcp.h"
#include "../d_main.h" // srb2home

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// Server:
//   maketic is the tic that hasn't had control made for it yet
//   nettics is the tic for each node
//   firstticstosend is the lowest value of nettics
// Client:
//   neededtic is the tic needed by the client to run the game
//   firstticstosend is used to optimize a condition
// Normally maketic >= gametic > 0

#define FORCECLOSE 0x8000
tic_t connectiontimeout = (10*TICRATE);

/// \brief network packet
doomcom_t *doomcom = NULL;
/// \brief network packet data, points inside doomcom
doomdata_t *netbuffer = NULL;

#ifdef DEBUGFILE
FILE *debugfile = NULL; // put some net info in a file during the game
#endif

#define MAXREBOUND 8
static doomdata_t reboundstore[MAXREBOUND];
static INT16 reboundsize[MAXREBOUND];
static INT32 rebound_head, rebound_tail;

/// \brief max length per packet
INT16 hardware_MAXPACKETLENGTH;

boolean (*I_NetGet)(void) = NULL;
void (*I_NetSend)(void) = NULL;
void (*I_NetCloseSocket)(void) = NULL;
void (*I_NetFreeNodenum)(INT32 nodenum) = NULL;
SINT8 (*I_NetMakeNodewPort)(const char *address, const char* port) = NULL;
boolean (*I_NetOpenSocket)(void) = NULL;
boolean (*I_Ban) (INT32 node) = NULL;
void (*I_ClearBans)(void) = NULL;
const char *(*I_GetNodeAddress) (INT32 node) = NULL;
const char *(*I_GetBanAddress) (size_t ban) = NULL;
const char *(*I_GetBanMask) (size_t ban) = NULL;
boolean (*I_SetBanAddress) (const char *address, const char *mask) = NULL;
boolean *bannednode = NULL;


// network stats
static tic_t statstarttic;
INT32 getbytes = 0;
INT64 sendbytes = 0;
static INT32 retransmit = 0, duppacket = 0;
static INT32 sendackpacket = 0, getackpacket = 0;
INT32 ticruned = 0, ticmiss = 0;

// globals
INT32 getbps, sendbps;
float lostpercent, duppercent, gamelostpercent;
INT32 packetheaderlength;

boolean Net_GetNetStat(void)
{
	const tic_t t = I_GetTime();
	static INT64 oldsendbyte = 0;
	if (statstarttic+STATLENGTH <= t)
	{
		const tic_t df = t-statstarttic;
		const INT64 newsendbyte = sendbytes - oldsendbyte;
		sendbps = (INT32)(newsendbyte*TICRATE)/df;
		getbps = (getbytes*TICRATE)/df;
		if (sendackpacket)
			lostpercent = 100.0f*(float)retransmit/(float)sendackpacket;
		else
			lostpercent = 0.0f;
		if (getackpacket)
			duppercent = 100.0f*(float)duppacket/(float)getackpacket;
		else
			duppercent = 0.0f;
		if (ticruned)
			gamelostpercent = 100.0f*(float)ticmiss/(float)ticruned;
		else
			gamelostpercent = 0.0f;

		ticmiss = ticruned = 0;
		oldsendbyte = sendbytes;
		getbytes = 0;
		sendackpacket = getackpacket = duppacket = retransmit = 0;
		statstarttic = t;

		return 1;
	}
	return 0;
}

// -----------------------------------------------------------------
// Some structs and functions for acknowledgement of packets
// -----------------------------------------------------------------
#define MAXACKPACKETS 96 // Minimum number of nodes (wat)
#define MAXACKTOSEND 96
#define URGENTFREESLOTNUM 10
#define ACKTOSENDTIMEOUT (TICRATE/11)

typedef struct
{
	UINT8 acknum;
	UINT8 nextacknum;
	UINT8 destinationnode; // The node to send the ack to
	tic_t senttime; // The time when the ack was sent
	UINT16 length; // The packet size
	UINT16 resentnum; // The number of times the ack has been resent
	union {
		SINT8 raw[MAXPACKETLENGTH];
		doomdata_t data;
	} pak;
} ackpak_t;

typedef enum
{
	NF_CLOSE = 1, // Flag is set when connection is closing
	NF_TIMEOUT = 2, // Flag is set when the node got a timeout
} node_flags_t;

// Table of packets that were not acknowleged can be resent (the sender window)
static ackpak_t ackpak[MAXACKPACKETS];

typedef struct
{
	// ack return to send (like sliding window protocol)
	UINT8 firstacktosend;

	// automatically send keep alive packet when not enough trafic
	tic_t lasttimeacktosend_sent;
	// detect connection lost
	tic_t lasttimepacketreceived;

	// flow control: do not send too many packets with ack
	UINT8 remotefirstack;
	UINT8 nextacknum;

	UINT8 flags;
} node_t;

static node_t nodes[MAXNETNODES];
#define NODETIMEOUT 14

// return <0 if a < b (mod 256)
//         0 if a = n (mod 256)
//        >0 if a > b (mod 256)
// mnemonic: to use it compare to 0: cmpack(a,b)<0 is "a < b" ...
FUNCMATH static INT32 cmpack(UINT8 a, UINT8 b)
{
	register INT32 d = a - b;

	if (d >= 127 || d < -128)
		return -d;
	return d;
}

/** Sets freeack to a free acknum and copies the netbuffer in the ackpak table
  *
  * \param freeack  The address to store the free acknum at
  * \return True if a free acknum was found
  */
static boolean GetFreeAcknum(UINT8 *freeack)
{
	node_t *node = &nodes[doomcom->remotenode];
	INT32 numfreeslot = 0;

	if (cmpack((UINT8)((node->remotefirstack + MAXACKTOSEND) % 256), node->nextacknum) < 0)
	{
		DEBFILE(va("too fast %d %d\n",node->remotefirstack,node->nextacknum));
		return false;
	}

	for (INT32 i = 0; i < MAXACKPACKETS; i++)
		if (!ackpak[i].acknum)
		{
			// For low priority packets, make sure to let freeslots so urgent packets can be sent
			if (netbuffer->packettype >= PT_CANFAIL)
			{
				numfreeslot++;
				if (numfreeslot <= URGENTFREESLOTNUM)
					continue;
			}

			ackpak[i].acknum = node->nextacknum;
			ackpak[i].nextacknum = node->nextacknum;
			node->nextacknum++;
			if (!node->nextacknum)
				node->nextacknum++;
			ackpak[i].destinationnode = (UINT8)(node - nodes);
			ackpak[i].length = doomcom->datalength;
			ackpak[i].senttime = I_GetTime();
			ackpak[i].resentnum = 0;
			M_Memcpy(ackpak[i].pak.raw, netbuffer, ackpak[i].length);

			*freeack = ackpak[i].acknum;

			sendackpacket++; // For stat

			return true;
		}
#ifdef PARANOIA
	CONS_Debug(DBG_NETPLAY, "No more free ackpacket\n");
#endif
	if (netbuffer->packettype < PT_CANFAIL)
		I_Error("Connection lost\n");
	return false;
}

// Get a ack to send in the queue of this node
static UINT8 GetAcktosend(INT32 node)
{
	nodes[node].lasttimeacktosend_sent = I_GetTime();
	return nodes[node].firstacktosend;
}

static void RemoveAck(INT32 i)
{
	INT32 node = ackpak[i].destinationnode;
	DEBFILE(va("Remove ack %d\n",ackpak[i].acknum));
	ackpak[i].acknum = 0;
	if (nodes[node].flags & NF_CLOSE)
		Net_CloseConnection(node);
}

// We have got a packet, proceed the ack request and ack return
static boolean Processackpak(void)
{
	boolean goodpacket = true;
	node_t *node = &nodes[doomcom->remotenode];

	// Received an ack return, so remove the ack in the list
	if (netbuffer->ackreturn && cmpack(node->remotefirstack, netbuffer->ackreturn) < 0)
	{
		node->remotefirstack = netbuffer->ackreturn;
		// Search the ackbuffer and free it
		for (INT32 i = 0; i < MAXACKPACKETS; i++)
			if (ackpak[i].acknum && ackpak[i].destinationnode == doomcom->remotenode
				&& cmpack(ackpak[i].acknum, netbuffer->ackreturn) <= 0)
			{
				RemoveAck(i);
			}
	}

	// Received a packet with ack, queue it to send the ack back
	if (netbuffer->ack)
	{
		UINT8 ack = netbuffer->ack;
		getackpacket++;
		if (cmpack(ack, node->firstacktosend) <= 0)
		{
			DEBFILE(va("Discard(1) ack %d (duplicated)\n", ack));
			duppacket++;
			goodpacket = false; // Discard packet (duplicate)
		}
		else
		{
			// Is a good packet so increment the acknowledge number,
			// Then search for a "hole" in the queue
			UINT8 nextfirstack = (UINT8)(node->firstacktosend + 1);
			if (!nextfirstack)
				nextfirstack = 1;

			if (ack == nextfirstack)
			{
				node->firstacktosend = nextfirstack;
			}
			else // Out of order packet
			{
				// Don't increment firsacktosend, put it in asktosend queue
				// Will be incremented when the nextfirstack comes (code above)
				DEBFILE(va("out of order packet (%d expected)\n", nextfirstack));
				goodpacket = false;
			}
		}
	}
	return goodpacket;
}

void Net_ConnectionTimeout(INT32 node)
{
	// Don't timeout several times
	if (nodes[node].flags & NF_TIMEOUT)
		return;
	nodes[node].flags |= NF_TIMEOUT;

	if (server)
		SendKicksForNode(node, KICK_MSG_TIMEOUT | KICK_MSG_KEEP_BODY);
	else
		CL_HandleTimeout();

	// Do not redo it quickly (if we do not close connection it is
	// for a good reason!)
	nodes[node].lasttimepacketreceived = I_GetTime();
}

// Resend the data if needed
void Net_AckTicker(void)
{

	for (INT32 i = 0; i < MAXACKPACKETS; i++)
	{
		const INT32 nodei = ackpak[i].destinationnode;
		node_t *node = &nodes[nodei];
		if (ackpak[i].acknum && ackpak[i].senttime + NODETIMEOUT < I_GetTime())
		{
			if (ackpak[i].resentnum > 20 && (node->flags & NF_CLOSE))
			{
				DEBFILE(va("ack %d sent 20 times so connection is supposed lost: node %d\n",
					i, nodei));
				Net_CloseConnection(nodei | FORCECLOSE);

				ackpak[i].acknum = 0;
				continue;
			}
			DEBFILE(va("Resend ack %d, %u<%d at %u\n", ackpak[i].acknum, ackpak[i].senttime,
				NODETIMEOUT, I_GetTime()));
			M_Memcpy(netbuffer, ackpak[i].pak.raw, ackpak[i].length);
			ackpak[i].senttime = I_GetTime();
			ackpak[i].resentnum++;
			ackpak[i].nextacknum = node->nextacknum;
			retransmit++; // For stat
			HSendPacket((INT32)(node - nodes), false, ackpak[i].acknum,
				(size_t)(ackpak[i].length - BASEPACKETSIZE));
		}
	}

	for (INT32 i = 1; i < MAXNETNODES; i++)
	{
		// This is something like node open flag
		if (nodes[i].firstacktosend)
		{
			if (!(nodes[i].flags & NF_CLOSE)
				&& nodes[i].lasttimepacketreceived + connectiontimeout < I_GetTime())
			{
				Net_ConnectionTimeout(i);
			}
		}
	}
}

// Remove last packet received ack before resending the ackreturn
// (the higher layer doesn't have room, or something else ....)
void Net_UnAcknowledgePacket(INT32 node)
{
	DEBFILE(va("UnAcknowledge node %d\n", node));
	if (!node)
		return;
	nodes[node].firstacktosend--;
	if (!nodes[node].firstacktosend)
		nodes[node].firstacktosend = UINT8_MAX;
}

/** Checks if all acks have been received
  *
  * \return True if all acks have been received
  *
  */
static boolean Net_AllAcksReceived(void)
{
	for (INT32 i = 0; i < MAXACKPACKETS; i++)
		if (ackpak[i].acknum)
			return false;

	return true;
}

/** Waits for all ackreturns
  *
  * \param timeout Timeout in seconds
  *
  */
void Net_WaitAllAckReceived(UINT32 timeout)
{
	tic_t tictac = I_GetTime();
	timeout = tictac + timeout*NEWTICRATE;

	HGetPacket();
	while (timeout > I_GetTime() && !Net_AllAcksReceived())
	{
		while (tictac == I_GetTime())
		{
			I_Sleep(cv_sleep.value);
			I_UpdateTime(cv_timescale.value);
		}
		tictac = I_GetTime();
		HGetPacket();
		Net_AckTicker();
	}
}

static void InitNode(node_t *node)
{
	node->firstacktosend = 0;
	node->nextacknum = 1;
	node->remotefirstack = 0;
	node->flags = 0;
}

static void InitAck(void)
{
	for (INT32 i = 0; i < MAXACKPACKETS; i++)
		ackpak[i].acknum = 0;

	for (INT32 i = 0; i < MAXNETNODES; i++)
		InitNode(&nodes[i]);
}

/** Removes all acks of a given packet type
  *
  * \param packettype The packet type to forget
  *
  */
void Net_AbortPacketType(UINT8 packettype)
{
	for (INT32 i = 0; i < MAXACKPACKETS; i++)
		if (ackpak[i].acknum && (ackpak[i].pak.data.packettype == packettype
			|| packettype == UINT8_MAX))
		{
			ackpak[i].acknum = 0;
		}
}

// -----------------------------------------------------------------
// end of acknowledge function
// -----------------------------------------------------------------

// remove a node, clear all ack from this node and reset askret
void Net_CloseConnection(INT32 node)
{
	boolean forceclose = (node & FORCECLOSE) != 0;

	if (node == -1)
	{
		DEBFILE(M_GetText("Net_CloseConnection: node -1 detected!\n"));
		return; // nope, just ignore it
	}

	node &= ~FORCECLOSE;

	if (!node)
		return;

	if (node < 0 || node >= MAXNETNODES) // prevent invalid nodes from crashing the game
	{
		DEBFILE(va(M_GetText("Net_CloseConnection: invalid node %d detected!\n"), node));
		return;
	}

	nodes[node].flags |= NF_CLOSE;

	if (nodes[node].firstacktosend)
	{
		// send a PT_NOTHING back to acknowledge the packet
		netbuffer->packettype = PT_NOTHING;
		HSendPacket(node, false, 0, 0);
	}

	// check if we are waiting for an ack from this node
	for (INT32 i = 0; i < MAXACKPACKETS; i++)
		if (ackpak[i].acknum && ackpak[i].destinationnode == node)
		{
			if (!forceclose)
				return; // connection will be closed when ack is returned
			else
				ackpak[i].acknum = 0;
		}

	InitNode(&nodes[node]);
	SV_AbortSendFiles(node);
	if (server)
		SV_AbortLuaFileTransfer(node);
	I_NetFreeNodenum(node);
}

//
// Checksum
//
static UINT32 NetbufferChecksum(void)
{
	UINT32 c = 0x1234567;
	const INT32 l = doomcom->datalength - 4;
	const UINT8 *buf = (UINT8 *)netbuffer + 4;

	for (INT32 i = 0; i < l; i++, buf++)
		c += (*buf) * (i+1);

	return LONG(c);
}

#ifdef DEBUGFILE

static void fprintfstring(char *s, size_t len)
{
	INT32 mode = 0;

	for (size_t i = 0; i < len; i++)
		if (s[i] < 32)
		{
			if (!mode)
			{
				fprintf(debugfile, "[%d", (UINT8)s[i]);
				mode = 1;
			}
			else
				fprintf(debugfile, ",%d", (UINT8)s[i]);
		}
		else
		{
			if (mode)
			{
				fprintf(debugfile, "]");
				mode = 0;
			}
			fprintf(debugfile, "%c", s[i]);
		}
	if (mode)
		fprintf(debugfile, "]");
}

static void fprintfstringnewline(char *s, size_t len)
{
	fprintfstring(s, len);
	fprintf(debugfile, "\n");
}

/// \warning Keep this up-to-date if you add/remove/rename packet types
static const char *packettypename[NUMPACKETTYPE] =
{
	"NOTHING",
	"SERVERCFG",
	"CLIENTCMD",
	"CLIENTMIS",
	"CLIENT2CMD",
	"CLIENT2MIS",
	"NODEKEEPALIVE",
	"NODEKEEPALIVEMIS",
	"SERVERTICS",
	"SERVERREFUSE",
	"SERVERSHUTDOWN",
	"CLIENTQUIT",

	"ASKINFO",
	"SERVERINFO",
	"PLAYERINFO",
	"REQUESTFILE",
	"ASKINFOVIAMS",

	"WILLRESENDGAMESTATE",
	"CANRECEIVEGAMESTATE",
	"RECEIVEDGAMESTATE",

	"SENDINGLUAFILE",
	"ASKLUAFILE",
	"HASLUAFILE",

	"PT_BASICKEEPALIVE",

	"FILEFRAGMENT",
	"FILEACK",
	"FILERECEIVED",

	"TEXTCMD",
	"TEXTCMD2",
	"CLIENTJOIN",
	"LOGIN",
	"TELLFILESNEEDED",
	"MOREFILESNEEDED",
	"PING"
};

static void DebugPrintpacket(const char *header)
{
	fprintf(debugfile, "%-12s (node %d,ack %d,ackret %d,size %d) type(%d) : %s\n",
		header, doomcom->remotenode, netbuffer->ack, netbuffer->ackreturn, doomcom->datalength,
		netbuffer->packettype, packettypename[netbuffer->packettype]);

	switch (netbuffer->packettype)
	{
		case PT_ASKINFO:
		case PT_ASKINFOVIAMS:
			fprintf(debugfile, "    time %u\n", (tic_t)LONG(netbuffer->u.askinfo.time));
			break;
		case PT_CLIENTJOIN:
			fprintf(debugfile, "    number %d mode %d\n", netbuffer->u.clientcfg.localplayers,
				netbuffer->u.clientcfg.mode);
			break;
		case PT_SERVERTICS:
		{
			servertics_pak *serverpak = &netbuffer->u.serverpak;
			UINT8 *cmd = (UINT8 *)(&serverpak->cmds[serverpak->numslots * serverpak->numtics]);
			size_t ntxtcmd = &((UINT8 *)netbuffer)[doomcom->datalength] - cmd;

			fprintf(debugfile, "    firsttic %u ply %d tics %d ntxtcmd %s\n    ",
				(UINT32)serverpak->starttic, serverpak->numslots, serverpak->numtics, sizeu1(ntxtcmd));
			/// \todo Display more readable information about net commands
			fprintfstringnewline((char *)cmd, ntxtcmd);
			/*fprintfstring((char *)cmd, 3);
			if (ntxtcmd > 4)
			{
				fprintf(debugfile, "[%s]", netxcmdnames[*((cmd) + 3) - 1]);
				fprintfstring(((char *)cmd) + 4, ntxtcmd - 4);
			}
			fprintf(debugfile, "\n");*/
			break;
		}
		case PT_CLIENTCMD:
		case PT_CLIENT2CMD:
		case PT_CLIENTMIS:
		case PT_CLIENT2MIS:
		case PT_NODEKEEPALIVE:
		case PT_NODEKEEPALIVEMIS:
			fprintf(debugfile, "    tic %4u resendfrom %u\n",
				(UINT32)ExpandTics(netbuffer->u.clientpak.client_tic, doomcom->remotenode),
				(UINT32)ExpandTics (netbuffer->u.clientpak.resendfrom, doomcom->remotenode));
			break;
		case PT_BASICKEEPALIVE:
			fprintf(debugfile, "    wipetime\n");
			break;
		case PT_TEXTCMD:
		case PT_TEXTCMD2:
			fprintf(debugfile, "    length %d\n    ", netbuffer->u.textcmd[0]);
			fprintf(debugfile, "[%s]", netxcmdnames[netbuffer->u.textcmd[1] - 1]);
			fprintfstringnewline((char *)netbuffer->u.textcmd + 2, netbuffer->u.textcmd[0] - 1);
			break;
		case PT_SERVERCFG:
			fprintf(debugfile, "    playerslots %d clientnode %d serverplayer %d "
				"gametic %u gamestate %d gametype %d modifiedgame %d\n",
				netbuffer->u.servercfg.totalslotnum, netbuffer->u.servercfg.clientnode,
				netbuffer->u.servercfg.serverplayer, (UINT32)LONG(netbuffer->u.servercfg.gametic),
				netbuffer->u.servercfg.gamestate, netbuffer->u.servercfg.gametype,
				netbuffer->u.servercfg.modifiedgame);
			break;
		case PT_SERVERINFO:
			fprintf(debugfile, "    '%s' player %d/%d, map %s, filenum %d, time %u \n",
				netbuffer->u.serverinfo.servername, netbuffer->u.serverinfo.numberofplayer,
				netbuffer->u.serverinfo.maxplayer, netbuffer->u.serverinfo.mapname,
				netbuffer->u.serverinfo.fileneedednum,
				(UINT32)LONG(netbuffer->u.serverinfo.time));
			fprintfstringnewline((char *)netbuffer->u.serverinfo.fileneeded,
				(UINT8)((UINT8 *)netbuffer + doomcom->datalength
				- (UINT8 *)netbuffer->u.serverinfo.fileneeded));
			break;
		case PT_SERVERREFUSE:
			fprintf(debugfile, "    reason %s\n", netbuffer->u.serverrefuse.reason);
			break;
		case PT_FILEFRAGMENT:
			fprintf(debugfile, "    fileid %d datasize %d position %u\n",
				netbuffer->u.filetxpak.fileid, (UINT16)SHORT(netbuffer->u.filetxpak.size),
				(UINT32)LONG(netbuffer->u.filetxpak.position));
			break;
		case PT_REQUESTFILE:
		default: // write as a raw packet
			fprintfstringnewline((char *)netbuffer->u.textcmd,
				(UINT8)((UINT8 *)netbuffer + doomcom->datalength - (UINT8 *)netbuffer->u.textcmd));
			break;
	}
}
#endif

#ifdef PACKETDROP
static INT32 packetdropquantity[NUMPACKETTYPE] = {0};
static INT32 packetdroprate = 0;

void Command_Drop(void)
{
	INT32 packetquantity;
	const char *packetname;

	if (COM_Argc() < 2)
	{
		CONS_Printf("drop <packettype> [quantity]: drop packets\n"
					"drop reset: cancel all packet drops\n");
		return;
	}

	if (!(stricmp(COM_Argv(1), "reset") && stricmp(COM_Argv(1), "cancel") && stricmp(COM_Argv(1), "stop")))
	{
		memset(packetdropquantity, 0, sizeof(packetdropquantity));
		return;
	}

	if (COM_Argc() >= 3)
	{
		packetquantity = atoi(COM_Argv(2));
		if (packetquantity <= 0 && COM_Argv(2)[0] != '0')
		{
			CONS_Printf("Invalid quantity\n");
			return;
		}
	}
	else
		packetquantity = -1;

	packetname = COM_Argv(1);

	if (!(stricmp(packetname, "all") && stricmp(packetname, "any")))
		for (size_t i = 0; i < NUMPACKETTYPE; i++)
			packetdropquantity[i] = packetquantity;
	else
	{
		for (size_t i = 0; i < NUMPACKETTYPE; i++)
			if (!stricmp(packetname, packettypename[i]))
			{
				packetdropquantity[i] = packetquantity;
				return;
			}

		CONS_Printf("Unknown packet name\n");
	}
}

void Command_Droprate(void)
{
	INT32 droprate;

	if (COM_Argc() < 2)
	{
		CONS_Printf("Packet drop rate: %d%%\n", packetdroprate);
		return;
	}

	droprate = atoi(COM_Argv(1));
	if ((droprate <= 0 && COM_Argv(1)[0] != '0') || droprate > 100)
	{
		CONS_Printf("Packet drop rate must be between 0 and 100!\n");
		return;
	}

	packetdroprate = droprate;
}

static boolean ShouldDropPacket(void)
{
	return (packetdropquantity[netbuffer->packettype])
		|| (packetdroprate != 0 && rand() < (((double)RAND_MAX) * (packetdroprate / 100.f))) || packetdroprate == 100;
}
#endif

//
// HSendPacket
//
boolean HSendPacket(INT32 node, boolean reliable, UINT8 acknum, size_t packetlength)
{
	doomcom->datalength = (INT16)(packetlength + BASEPACKETSIZE);
	if (node == 0) // Packet is to go back to us
	{
		if ((rebound_head+1) % MAXREBOUND == rebound_tail)
		{
#ifdef PARANOIA
			CONS_Debug(DBG_NETPLAY, "No more rebound buf\n");
#endif
			return false;
		}
		netbuffer->ack = netbuffer->ackreturn = 0; // don't hold over values from last packet sent/received
		M_Memcpy(&reboundstore[rebound_head], netbuffer,
			doomcom->datalength);
		reboundsize[rebound_head] = doomcom->datalength;
		rebound_head = (rebound_head+1) % MAXREBOUND;
#ifdef DEBUGFILE
		if (debugfile)
		{
			doomcom->remotenode = (INT16)node;
			DebugPrintpacket("SENDLOCAL");
		}
#endif
		return true;
	}

	if (!netgame)
		I_Error("Tried to transmit to another node");

	// do this before GetFreeAcknum because this function backups
	// the current packet
	doomcom->remotenode = (INT16)node;
	if (doomcom->datalength <= 0)
	{
		DEBFILE("HSendPacket: nothing to send\n");
#ifdef DEBUGFILE
		if (debugfile)
			DebugPrintpacket("TRISEND");
#endif
		return false;
	}

	if (node < MAXNETNODES) // Can be a broadcast
		netbuffer->ackreturn = GetAcktosend(node);
	else
		netbuffer->ackreturn = 0;
	if (reliable)
	{
		if (!GetFreeAcknum(&netbuffer->ack))
			return false;
	}
	else
		netbuffer->ack = acknum;

	netbuffer->checksum = NetbufferChecksum();
	sendbytes += packetheaderlength + doomcom->datalength; // For stat

#ifdef PACKETDROP
	// Simulate internet :)
	//if (rand() >= (INT32)(RAND_MAX * (PACKETLOSSRATE / 100.f)))
	if (!ShouldDropPacket())
	{
#endif
#ifdef DEBUGFILE
		if (debugfile)
			DebugPrintpacket("SENT");
#endif
		I_NetSend();
#ifdef PACKETDROP
	}
	else
	{
		if (packetdropquantity[netbuffer->packettype] > 0)
			packetdropquantity[netbuffer->packettype]--;
#ifdef DEBUGFILE
		if (debugfile)
			DebugPrintpacket("NOT SENT");
#endif
	}
#endif

	return true;
}

//
// HGetPacket
// Returns false if no packet is waiting
// Check Datalength and checksum
//
boolean HGetPacket(void)
{
	//boolean nodejustjoined;

	// Get a packet from self
	if (rebound_tail != rebound_head)
	{
		M_Memcpy(netbuffer, &reboundstore[rebound_tail], reboundsize[rebound_tail]);
		doomcom->datalength = reboundsize[rebound_tail];
		doomcom->remotenode = 0;

		rebound_tail = (rebound_tail+1) % MAXREBOUND;
#ifdef DEBUGFILE
		if (debugfile)
			DebugPrintpacket("GETLOCAL");
#endif
		return true;
	}

	if (!netgame)
		return false;

	while(true)
	{
		//nodejustjoined = I_NetGet();
		I_NetGet();

		if (doomcom->remotenode == -1) // No packet received
			return false;

		getbytes += packetheaderlength + doomcom->datalength; // For stat

		if (doomcom->remotenode >= MAXNETNODES)
		{
			DEBFILE(va("Received packet from node %d!\n", doomcom->remotenode));
			continue;
		}

		nodes[doomcom->remotenode].lasttimepacketreceived = I_GetTime();

		if (netbuffer->checksum != NetbufferChecksum())
		{
			DEBFILE("Bad packet checksum\n");
			// Do not disconnect or anything, just ignore the packet.
			// Bad checksums with UDP tend to happen very scarcely
			// so they are not normally an issue.
			continue;
		}

#ifdef DEBUGFILE
		if (debugfile)
			DebugPrintpacket("GET");
#endif

		/*// If a new node sends an unexpected packet, just ignore it
		if (nodejustjoined && server
			&& !(netbuffer->packettype == PT_ASKINFO
				|| netbuffer->packettype == PT_SERVERINFO
				|| netbuffer->packettype == PT_PLAYERINFO
				|| netbuffer->packettype == PT_REQUESTFILE
				|| netbuffer->packettype == PT_ASKINFOVIAMS
				|| netbuffer->packettype == PT_CLIENTJOIN))
		{
			DEBFILE(va("New node sent an unexpected %s packet\n", packettypename[netbuffer->packettype]));
			//CONS_Alert(CONS_NOTICE, "New node sent an unexpected %s packet\n", packettypename[netbuffer->packettype]);
			Net_CloseConnection(doomcom->remotenode | FORCECLOSE);
			continue;
		}*/

		// Proceed the ack and ackreturn field
		if (!Processackpak())
			continue; // discarded (duplicated)

		// A packet with just ackreturn
		if (netbuffer->packettype == PT_NOTHING)
			continue;
		break;
	}

	return true;
}

static boolean Internal_Get(void)
{
	doomcom->remotenode = -1;
	return false;
}

FUNCNORETURN static ATTRNORETURN void Internal_Send(void)
{
	I_Error("Send without netgame\n");
}

static void Internal_FreeNodenum(INT32 nodenum)
{
	(void)nodenum;
}

char *I_NetSplitAddress(char *host, char **port)
{
	boolean v4 = (host[0] != '[');

	host = strtok(host, v4 ? ":" : "[]");

	if (port)
		*port = strtok(NULL, ":");

	return host;
}

SINT8 I_NetMakeNode(const char *hostname)
{
	SINT8 newnode = -1;
	if (I_NetMakeNodewPort)
	{
		char *localhostname = strdup(hostname);
		char *port;
		if (!localhostname)
			return newnode;

		// retrieve portnum from address!
		hostname = I_NetSplitAddress(localhostname, &port);

		newnode = I_NetMakeNodewPort(hostname, port);
		free(localhostname);
	}
	return newnode;
}

void D_SetDoomcom(void)
{
	if (doomcom) return;
	doomcom = Z_Calloc(sizeof (doomcom_t), PU_STATIC, NULL);
	doomcom->numslots = doomcom->numnodes = 1;
	doomcom->extratics = 0;
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
boolean D_CheckNetGame(void)
{
	boolean ret = false;

	InitAck();
	rebound_tail = rebound_head = 0;

	statstarttic = I_GetTime();

	I_NetGet = Internal_Get;
	I_NetSend = Internal_Send;
	I_NetCloseSocket = NULL;
	I_NetFreeNodenum = Internal_FreeNodenum;
	I_NetMakeNodewPort = NULL;

	hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
	// I_InitNetwork sets doomcom and netgame
	// check and initialize the network driver
	multiplayer = false;

	// only dos version with external driver will return true
	netgame = I_InitNetwork();
	if (!netgame && !I_NetOpenSocket)
	{
		D_SetDoomcom();
		netgame = I_InitTcpNetwork();
	}

	if (netgame)
		ret = true;
	if (client && netgame)
		netgame = false;
	server = true; // WTF? server always true???
		// no! The deault mode is server. Client is set elsewhere
		// when the client executes connect command.

	if (M_CheckParm("-extratic"))
	{
		if (M_IsNextParm())
			doomcom->extratics = (INT16)atoi(M_GetNextParm());
		else
			doomcom->extratics = 1;
		CONS_Printf(M_GetText("Set extratics to %d\n"), doomcom->extratics);
	}

	software_MAXPACKETLENGTH = hardware_MAXPACKETLENGTH;
	if (M_CheckParm("-packetsize"))
	{
		if (M_IsNextParm())
		{
			INT32 p = atoi(M_GetNextParm());
			if (p < 75)
				p = 75;
			if (p > hardware_MAXPACKETLENGTH)
				p = hardware_MAXPACKETLENGTH;
			software_MAXPACKETLENGTH = (UINT16)p;
		}
		else
			I_Error("usage: -packetsize <bytes_per_packet>");
	}

	if (netgame)
		multiplayer = true;

	if (doomcom->numnodes > MAXNETNODES)
		I_Error("Too many nodes (%d), max:%d", doomcom->numnodes, MAXNETNODES);

	netbuffer = (doomdata_t *)(void *)&doomcom->data;

#ifdef DEBUGFILE
	if (M_CheckParm("-debugfile"))
	{
		char filename[21];
		INT32 k = consoleplayer - 1;
		if (M_IsNextParm())
			k = atoi(M_GetNextParm()) - 1;
		while (!debugfile && k < MAXPLAYERS)
		{
			k++;
			sprintf(filename, "debug%d.txt", k);
			debugfile = fopen(va("%s" PATHSEP "%s", srb2home, filename), "w");
		}
		if (debugfile)
			CONS_Printf(M_GetText("debug output to: %s\n"), va("%s" PATHSEP "%s", srb2home, filename));
		else
			CONS_Alert(CONS_WARNING, M_GetText("cannot debug output to file %s!\n"), va("%s" PATHSEP "%s", srb2home, filename));
	}
#endif

	D_ClientServerInit();

	return ret;
}

struct pingcell
{
	INT32 num;
	INT32 ms;
};

static int pingcellcmp(const void *va, const void *vb)
{
	const struct pingcell *a, *b;
	a = va;
	b = vb;
	return ( a->ms - b->ms );
}

/*
New ping command formatted nicely to present ping in
ascending order. And with equally spaced columns.
The caller's ping is presented at the bottom too, for
convenience.
*/

void Command_Ping_f(void)
{
	struct pingcell pingv[MAXPLAYERS];
	INT32           pingc;

	int name_width = 0;
	int   ms_width = 0;

	pingc = 0;
	for (INT32 i = 1; i < MAXPLAYERS; ++i)
		if (playeringame[i])
	{
		int n;

		n = strlen(player_names[i]);
		if (n > name_width)
			name_width = n;

		n = playerpingtable[i];
		if (n > ms_width)
			ms_width = n;

		pingv[pingc].num = i;
		pingv[pingc].ms  = playerpingtable[i];
		pingc++;
	}

	     if (ms_width < 10)  ms_width = 1;
	else if (ms_width < 100) ms_width = 2;
	else                     ms_width = 3;

	qsort(pingv, pingc, sizeof (struct pingcell), &pingcellcmp);

	for (INT32 i = 0; i < pingc; ++i)
	{
		CONS_Printf("%02d : %-*s %*d ms\n",
				pingv[i].num,
				name_width, player_names[pingv[i].num],
				ms_width,   pingv[i].ms);
	}

	if (!server && playeringame[consoleplayer])
	{
		CONS_Printf("\nYour ping is %d ms\n", playerpingtable[consoleplayer]);
	}
}

void D_CloseConnection(void)
{
	if (netgame)
	{
		// wait the ackreturn with timout of 5 Sec
		Net_WaitAllAckReceived(5);

		// close all connection
		for (INT32 i = 0; i < MAXNETNODES; i++)
			Net_CloseConnection(i|FORCECLOSE);

		InitAck();

		if (I_NetCloseSocket)
			I_NetCloseSocket();

		I_NetGet = Internal_Get;
		I_NetSend = Internal_Send;
		I_NetCloseSocket = NULL;
		I_NetFreeNodenum = Internal_FreeNodenum;
		I_NetMakeNodewPort = NULL;
		netgame = false;
		addedtogame = false;
	}

	D_ResetTiccmds();
}
