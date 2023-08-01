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

#include "../doomtype.h"

// Max computers in a game
// 127 is probably as high as this can go, because
// SINT8 is used for nodes sometimes >:(
#define MAXNETNODES 127
#define BROADCASTADDR MAXNETNODES
#define MAXSPLITSCREENPLAYERS 2 // Max number of players on a single computer
//#define NETSPLITSCREEN // Kart's splitscreen netgame feature

#define STATLENGTH (TICRATE*2)

// stat of net
extern INT32 ticruned, ticmiss;
extern INT32 getbps, sendbps;
extern float lostpercent, duppercent, gamelostpercent;
extern INT32 packetheaderlength;
boolean Net_GetNetStat(void);
extern INT32 getbytes;
extern INT64 sendbytes; // Realtime updated

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

INT32 Net_GetFreeAcks(boolean urgent);
void Net_AckTicker(void);

// If reliable return true if packet sent, 0 else
boolean HSendPacket(INT32 node, boolean reliable, UINT8 acknum,
	size_t packetlength);
boolean HGetPacket(void);
void D_SetDoomcom(void);
boolean D_CheckNetGame(void);
void D_CloseConnection(void);
void Net_UnAcknowledgePacket(INT32 node);
void Net_CloseConnection(INT32 node);
void Net_ConnectionTimeout(INT32 node);
void Net_AbortPacketType(UINT8 packettype);
void Net_SendAcks(INT32 node);
void Net_WaitAllAckReceived(UINT32 timeout);

#endif
