// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_net.h
/// \brief part of layer 4 (transport) (tp4) of the osi model
///        assure the reception of packet and proceed a checksums
///
///        There is a data struct that stores network communication related
///        stuff, and one that defines the actual packets to be transmitted

#ifndef __D_NET__
#define __D_NET__

#include "i_net.h"
#include "../doomtype.h"

// Max computers in a game
// 127 is probably as high as this can go, because
// SINT8 is used for nodes sometimes >:(
#define MAXNETNODES 127
#define BROADCASTADDR MAXNETNODES
#define MAXSPLITSCREENPLAYERS 2 // Max number of players on a single computer
//#define NETSPLITSCREEN // Kart's splitscreen netgame feature
#define PACKETLOSSCYCLES 4 // amount of cycles to do when measuring packet loss

#define FORCECLOSE 0x8000

#define STATLENGTH (TICRATE*2)

// stat of net
extern INT32 ticruned, ticmiss;
extern INT32 getbps, sendbps;
extern float lostpercent, duppercent, gamelostpercent;
extern INT32 packetheaderlength;
boolean Net_GetNetStat(void);
extern INT32 getbytes;
extern INT64 sendbytes; // Realtime updated
extern UINT8 plcycle;
extern UINT32 sentpackets[PACKETLOSSCYCLES][MAXNETNODES];
extern UINT32 lostpackets[PACKETLOSSCYCLES][MAXNETNODES];

#define PACKETMEASUREWINDOW (TICRATE*2)
extern boolean packetloss[MAXPLAYERS][PACKETMEASUREWINDOW];

typedef struct netnode_s
{
	boolean ingame; // set false as nodes leave game
	tic_t freezetimeout; // Until when can this node freeze the server before getting a timeout?

	SINT8 player;
	SINT8 player2; // say the numplayer for this node if any (splitscreen)
	UINT8 numplayers; // used specialy for scplitscreen

	tic_t tic; // what tic the client have received
	tic_t supposedtic; // nettics prevision for smaller packet

	boolean sendingsavegame; // Are we sending the savegame?
	boolean resendingsavegame; // Are we resending the savegame?
	tic_t savegameresendcooldown; // How long before we can resend again?
} netnode_t;

extern netnode_t netnodes[MAXNETNODES];

extern boolean serverrunning;

void Net_AckTicker(void);

// If reliable return true if packet sent, 0 else
boolean HSendPacket(doomcom_t *doomcom, boolean reliable, UINT8 acknum);
void HGetPacket(void (*handler)(doomcom_t *doomcom));
doomcom_t *D_NewPacket(UINT8 type, INT16 node, INT16 length);
boolean D_CheckNetGame(void);
void D_CloseConnection(void);
boolean Net_IsNodeIPv6(INT32 node);
void Net_UnAcknowledgePacket(INT32 node);
void Net_CloseConnection(INT32 node);
void Net_ConnectionTimeout(INT32 node);
void Net_SendAcks(INT32 node);
void Net_WaitAllAckReceived(UINT32 timeout);

#endif
