// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_clisrv.h
/// \brief high level networking stuff

#ifndef __D_CLISRV__
#define __D_CLISRV__

#include "d_ticcmd.h"
#include "d_netcmd.h"
#include "tables.h"
#include "d_player.h"

// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.

// Networking and tick handling related.
#define MAXTEXTCMD 256

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// client to server packet
typedef struct
{
	UINT8 client_tic;
	UINT8 resendfrom;
	ticcmd_t cmd;
} ATTRPACK clientcmd_pak;

// splitscreen packet
// WARNING: must have the same format of clientcmd_pak, for more easy use
typedef struct
{
	UINT8 client_tic;
	UINT8 resendfrom;
	ticcmd_t cmd, cmd2;
} ATTRPACK client2cmd_pak;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// Server to client packet
// this packet is too large
typedef struct
{
	UINT8 starttic;
	UINT8 numtics;
	UINT8 numslots; // "Slots filled": Highest player number in use plus one.
	ticcmd_t cmds[45]; // normally [BACKUPTIC][MAXPLAYERS] but too large
} ATTRPACK servertics_pak;

typedef struct
{
	UINT8 version; // different versions don't work
	UINT8 subversion; // contains build version

	// server launch stuffs
	UINT8 serverplayer;
	UINT8 totalslotnum; // "Slots": highest player number in use plus one.

	tic_t gametic;
	UINT8 clientnode;
	UINT8 gamestate;

	// 0xFF == not in game; else player skin num
	UINT8 playerskins[MAXPLAYERS];
	UINT8 playercolor[MAXPLAYERS];

	UINT8 gametype;
	UINT8 modifiedgame;
	SINT8 adminplayer; // needs to be signed

	char server_context[8]; // unique context id, generated at server startup.

	UINT8 varlengthinputs[0]; // playernames and netvars
} ATTRPACK serverconfig_pak;

typedef struct {
	UINT8 fileid;
	UINT32 position;
	UINT16 size;
	UINT8 data[0]; // size is variable using hardware_MAXPACKETLENGTH
} ATTRPACK filetx_pak;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

typedef struct
{
	UINT8 version; // different versions don't work
	UINT8 subversion; // contains build version
	UINT8 localplayers;
	UINT8 mode;
} ATTRPACK clientconfig_pak;

#define MAXSERVERNAME 32
// this packet is too large
typedef struct
{
	UINT8 version;
	UINT8 subversion;
	UINT8 numberofplayer;
	UINT8 maxplayer;
	UINT8 gametype;
	UINT8 modifiedgame;
	UINT8 cheatsenabled;
	UINT8 isdedicated;
	UINT8 fileneedednum;
	SINT8 adminplayer;
	tic_t time;
	tic_t leveltime;
	char servername[MAXSERVERNAME];
	char mapname[8];
	char maptitle[33];
	unsigned char mapmd5[16];
	UINT8 actnum;
	UINT8 iszone;
	UINT8 fileneeded[915]; // is filled with writexxx (byteptr.h)
} ATTRPACK serverinfo_pak;

typedef struct
{
	char reason[255];
} ATTRPACK serverrefuse_pak;

typedef struct
{
	UINT8 version;
	tic_t time; // used for ping evaluation
} ATTRPACK askinfo_pak;

typedef struct
{
	char clientaddr[22];
	tic_t time; // used for ping evaluation
} ATTRPACK msaskinfo_pak;

// Shorter player information for external use.
typedef struct
{
	UINT8 node;
	char name[MAXPLAYERNAME+1];
	UINT8 address[4]; // sending another string would run us up against MAXPACKETLENGTH
	UINT8 team;
	UINT8 skin;
	UINT8 data; // Color is first four bits, hasflag, isit and issuper have one bit each, the last is unused.
	UINT32 score;
	UINT16 timeinserver; // In seconds.
} ATTRPACK plrinfo;

// Shortest player information for join during intermission.
typedef struct
{
	char name[MAXPLAYERNAME+1];
	UINT8 skin;
	UINT8 color;
	UINT32 pflags;
	UINT32 score;
	UINT8 ctfteam;
} ATTRPACK plrconfig;

//
// Network packet data.
//
typedef struct
{
	UINT32 checksum;
	UINT8 ack; // if not null the node asks for acknowledgement, the receiver must resend the ack
	UINT8 ackreturn; // the return of the ack number

	UINT8 packettype;
	UINT8 reserved; // padding
	union
	{
		clientcmd_pak clientpak;    //      144 bytes
		client2cmd_pak client2pak;  //      200 bytes
		servertics_pak serverpak;   //   132495 bytes
		serverconfig_pak servercfg; //      773 bytes
		UINT8 textcmd[MAXTEXTCMD+1]; //   66049 bytes
		filetx_pak filetxpak;       //      139 bytes
		clientconfig_pak clientcfg; //      136 bytes
		serverinfo_pak serverinfo;  //     1024 bytes
		serverrefuse_pak serverrefuse; // 65025 bytes
		askinfo_pak askinfo;        //       61 bytes
		msaskinfo_pak msaskinfo;    //       22 bytes
		plrinfo playerinfo[MAXPLAYERS]; // 1152 bytes
		plrconfig playerconfig[MAXPLAYERS]; // (up to) 896 bytes
	} u; // this is needed to pack diff packet types data together
} ATTRPACK doomdata_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#define MAXSERVERLIST 64 // depends only on the display
typedef struct
{
	SINT8 node;
	serverinfo_pak info;
} serverelem_t;

extern serverelem_t serverlist[MAXSERVERLIST];
extern UINT32 serverlistcount;
extern INT32 mapchangepending;

extern consvar_t cv_playbackspeed;

#define BASEPACKETSIZE ((size_t)&(((doomdata_t *)0)->u))
#define FILETXHEADER ((size_t)((filetx_pak *)0)->data)
#define BASESERVERTICSSIZE ((size_t)&(((doomdata_t *)0)->u.serverpak.cmds[0]))

typedef enum {
	// Player left
	KICK_MSG_PLAYER_QUIT,

	// Generic kick/ban
	KICK_MSG_GO_AWAY,
	KICK_MSG_BANNED,

	// Custom kick/ban
	KICK_MSG_CUSTOM_KICK,
	KICK_MSG_CUSTOM_BAN,

	// Networking errors
	KICK_MSG_TIMEOUT,
	KICK_MSG_PING_HIGH,
	KICK_MSG_XD_FAIL,
	KICK_MSG_STOP_HACKING
} kickmsg_e;

extern boolean server;
extern boolean dedicated; // for dedicated server
extern UINT16 software_MAXPACKETLENGTH;
extern boolean acceptnewnode;
extern SINT8 servernode;

extern consvar_t cv_joinnextround, cv_allownewplayer, cv_maxplayers, cv_maxsend;

// used in d_net, the only dependence
tic_t ExpandTics(INT32 low);
void D_ClientServerInit(void);

// initialise the other field
void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum));
void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam);
void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam); // splitsreen player

// Create any new ticcmds and broadcast to other players.
void NetUpdate(void);

void SV_StartSinglePlayerServer(void);
boolean SV_SpawnServer(void);
void SV_SpawnPlayer(INT32 playernum, INT32 x, INT32 y, angle_t angle);
void SV_StopServer(void);
void SV_ResetServer(void);
void CL_AddSplitscreenPlayer(void);
void CL_RemoveSplitscreenPlayer(void);
void CL_Reset(void);
void CL_ClearPlayer(INT32 playernum);
void CL_UpdateServerList(boolean internetsearch, INT32 room);
// is there a game running
boolean Playing(void);

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame(void);

//? how many ticks to run?
void TryRunTics(tic_t realtic);

// extra data for lmps
// these functions scare me. they contain magic.
/*boolean AddLmpExtradata(UINT8 **demo_p, INT32 playernum);
void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum);*/

#ifndef NONET
// translate a playername in a player number return -1 if not found and
// print a error message in the console
SINT8 nametonum(const char *name);
#endif

extern char motd[254], server_context[8];
extern UINT8 playernode[MAXPLAYERS];

INT32 D_NumPlayers(void);
void D_ResetTiccmds(void);

tic_t GetLag(INT32 node);
UINT8 GetFreeXCmdSize(void);

#endif
