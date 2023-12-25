// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_clisrv.h
/// \brief high level networking stuff

#ifndef __D_CLISRV__
#define __D_CLISRV__

#include "protocol.h"
#include "../d_ticcmd.h"
#include "d_net.h"
#include "d_netcmd.h"
#include "d_net.h"
#include "../tables.h"
#include "../d_player.h"
#include "mserv.h"

#define CLIENTBACKUPTICS 32

#ifdef PACKETDROP
void Command_Drop(void);
void Command_Droprate(void);
#endif
#ifdef _DEBUG
void Command_Numnodes(void);
#endif

extern INT32 mapchangepending;

// Points inside doomcom
extern doomdata_t *netbuffer;

#define BASEPACKETSIZE      offsetof(doomdata_t, u)
#define BASESERVERTICSSIZE  offsetof(doomdata_t, u.serverpak.cmds[0])

typedef enum
{
	KR_KICK          = 1, //Kicked by server
	KR_PINGLIMIT     = 2, //Broke Ping Limit
	KR_SYNCH         = 3, //Synch Failure
	KR_TIMEOUT       = 4, //Connection Timeout
	KR_BAN           = 5, //Banned by server
	KR_LEAVE         = 6, //Quit the game
	KR_IDLE          = 7, //Remained still for too long
} kickreason_t;

/* the max number of name changes in some time period */
#define MAXNAMECHANGES (5)
#define NAMECHANGERATE (60*TICRATE)

extern boolean server;
extern boolean serverrunning;
#define client (!server)
extern boolean dedicated; // For dedicated server
extern UINT16 software_MAXPACKETLENGTH;
extern boolean acceptnewnode;
extern SINT8 servernode;
extern tic_t maketic;
extern tic_t neededtic;
extern INT16 consistancy[BACKUPTICS];

void Command_Ping_f(void);
extern tic_t connectiontimeout;
extern UINT16 pingmeasurecount;
extern UINT32 realpingtable[MAXPLAYERS];
extern UINT32 playerpingtable[MAXPLAYERS];
extern tic_t servermaxping;

extern consvar_t cv_netticbuffer, cv_resynchattempts, cv_blamecfail, cv_playbackspeed, cv_idletime, cv_dedicatedidletime;

// Used in d_net, the only dependence
void D_ClientServerInit(void);

// Create any new ticcmds and broadcast to other players.
void NetUpdate(void);

// Maintain connections to nodes without timing them all out.
void NetKeepAlive(void);

void GetPackets(void);
void ResetNode(INT32 node);
INT16 Consistancy(void);

void SV_StartSinglePlayerServer(void);
void SV_SpawnServer(void);
void SV_StopServer(void);
void SV_ResetServer(void);
void CL_AddSplitscreenPlayer(void);
void CL_RemoveSplitscreenPlayer(void);
void CL_Reset(void);
void CL_ClearPlayer(INT32 playernum);
void CL_RemovePlayer(INT32 playernum, kickreason_t reason);
void CL_HandleTimeout(void);
// Is there a game running
boolean Playing(void);

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame(void);

//? How many ticks to run?
boolean TryRunTics(tic_t realtic);

// extra data for lmps
// these functions scare me. they contain magic.
/*boolean AddLmpExtradata(UINT8 **demo_p, INT32 playernum);
void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum);*/

// translate a playername in a player number return -1 if not found and
// print a error message in the console
SINT8 nametonum(const char *name);

extern char motd[254], server_context[8];
extern UINT8 playernode[MAXPLAYERS];

INT32 D_NumPlayers(void);
INT32 D_NumBots(void);

tic_t GetLag(INT32 node);

void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest);

extern UINT8 (*adminpassmd5)[16];
extern UINT32 adminpasscount;

extern boolean hu_stopped;

#endif
