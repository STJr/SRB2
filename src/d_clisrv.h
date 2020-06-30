// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
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
#include "d_net.h"
#include "tables.h"
#include "d_player.h"

/*
The 'packet version' is used to distinguish packet formats.
This version is independent of VERSION and SUBVERSION. Different
applications may follow different packet versions.
*/
#define PACKETVERSION 3

// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.

// Networking and tick handling related.
#define BACKUPTICS 1024
#define CLIENTBACKUPTICS 32
#define MAXTEXTCMD 256
//
// Packet structure
//
typedef enum
{
	PT_NOTHING,       // To send a nop through the network. ^_~
	PT_SERVERCFG,     // Server config used in start game
	                  // (must stay 1 for backwards compatibility).
	                  // This is a positive response to a CLIENTJOIN request.
	PT_CLIENTCMD,     // Ticcmd of the client.
	PT_CLIENTMIS,     // Same as above with but saying resend from.
	PT_CLIENT2CMD,    // 2 cmds in the packet for splitscreen.
	PT_CLIENT2MIS,    // Same as above with but saying resend from
	PT_NODEKEEPALIVE, // Same but without ticcmd and consistancy
	PT_NODEKEEPALIVEMIS,
	PT_SERVERTICS,    // All cmds for the tic.
	PT_SERVERREFUSE,  // Server refuses joiner (reason inside).
	PT_SERVERSHUTDOWN,
	PT_CLIENTQUIT,    // Client closes the connection.

	PT_ASKINFO,       // Anyone can ask info of the server.
	PT_SERVERINFO,    // Send game & server info (gamespy).
	PT_PLAYERINFO,    // Send information for players in game (gamespy).
	PT_REQUESTFILE,   // Client requests a file transfer
	PT_ASKINFOVIAMS,  // Packet from the MS requesting info be sent to new client.
	                  // If this ID changes, update masterserver definition.
	PT_RESYNCHEND,    // Player is now resynched and is being requested to remake the gametic
	PT_RESYNCHGET,    // Player got resynch packet

	PT_SENDINGLUAFILE, // Server telling a client Lua needs to open a file
	PT_ASKLUAFILE,     // Client telling the server they don't have the file
	PT_HASLUAFILE,     // Client telling the server they have the file

	// Add non-PT_CANFAIL packet types here to avoid breaking MS compatibility.

	PT_CANFAIL,       // This is kind of a priority. Anything bigger than CANFAIL
	                  // allows HSendPacket(*, true, *, *) to return false.
	                  // In addition, this packet can't occupy all the available slots.

	PT_FILEFRAGMENT = PT_CANFAIL, // A part of a file.
	PT_FILEACK,
	PT_FILERECEIVED,

	PT_TEXTCMD,       // Extra text commands from the client.
	PT_TEXTCMD2,      // Splitscreen text commands.
	PT_CLIENTJOIN,    // Client wants to join; used in start game.
	PT_NODETIMEOUT,   // Packet sent to self if the connection times out.
	PT_RESYNCHING,    // Packet sent to resync players.
	                  // Blocks game advance until synched.

	PT_LOGIN,         // Login attempt from the client.

	PT_PING,          // Packet sent to tell clients the other client's latency to server.
	NUMPACKETTYPE
} packettype_t;

#ifdef PACKETDROP
void Command_Drop(void);
void Command_Droprate(void);
#endif
#ifdef _DEBUG
void Command_Numnodes(void);
#endif

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// Client to server packet
typedef struct
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd;
} ATTRPACK clientcmd_pak;

// Splitscreen packet
// WARNING: must have the same format of clientcmd_pak, for more easy use
typedef struct
{
	UINT8 client_tic;
	UINT8 resendfrom;
	INT16 consistancy;
	ticcmd_t cmd, cmd2;
} ATTRPACK client2cmd_pak;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// Server to client packet
// this packet is too large
typedef struct
{
	tic_t starttic;
	UINT8 numtics;
	UINT8 numslots; // "Slots filled": Highest player number in use plus one.
	ticcmd_t cmds[45]; // Normally [BACKUPTIC][MAXPLAYERS] but too large
} ATTRPACK servertics_pak;

// Sent to client when all consistency data
// for players has been restored
typedef struct
{
	UINT32 randomseed;

	// CTF flag stuff
	SINT8 flagplayer[2];
	INT32 flagloose[2];
	INT32 flagflags[2];
	fixed_t flagx[2];
	fixed_t flagy[2];
	fixed_t flagz[2];

	UINT32 ingame;  // Spectator bit for each player
	UINT32 outofcoop;  // outofcoop bit for each player
	INT32 ctfteam[MAXPLAYERS]; // Which team? (can't be 1 bit, since in regular Match there are no teams)

	// Resynch game scores and the like all at once
	UINT32 score[MAXPLAYERS]; // Everyone's score
	INT16 numboxes[MAXPLAYERS];
	INT16 totalring[MAXPLAYERS];
	tic_t realtime[MAXPLAYERS];
	UINT8 laps[MAXPLAYERS];
} ATTRPACK resynchend_pak;

typedef struct
{
	// Player stuff
	UINT8 playernum;

	// Do not send anything visual related.
	// Only send data that we need to know for physics.
	UINT8 playerstate; // playerstate_t
	UINT32 pflags; // pflags_t
	UINT8 panim; // panim_t

	INT16 angleturn;
	INT16 oldrelangleturn;

	angle_t aiming;
	INT32 currentweapon;
	INT32 ringweapons;
	UINT16 ammoremoval;
	tic_t ammoremovaltimer;
	INT32 ammoremovalweapon;
	UINT16 powers[NUMPOWERS];

	// Score is resynched in the confirm resync packet
	INT16 rings;
	INT16 spheres;
	SINT8 lives;
	SINT8 continues;
	UINT8 scoreadd;
	SINT8 xtralife;
	SINT8 pity;

	UINT16 skincolor;
	INT32 skin;
	UINT32 availabilities;
	// Just in case Lua does something like
	// modify these at runtime
	fixed_t camerascale;
	fixed_t shieldscale;
	fixed_t normalspeed;
	fixed_t runspeed;
	UINT8 thrustfactor;
	UINT8 accelstart;
	UINT8 acceleration;
	UINT8 charability;
	UINT8 charability2;
	UINT32 charflags;
	UINT32 thokitem; // mobjtype_t
	UINT32 spinitem; // mobjtype_t
	UINT32 revitem; // mobjtype_t
	UINT32 followitem; // mobjtype_t
	fixed_t actionspd;
	fixed_t mindash;
	fixed_t maxdash;
	fixed_t jumpfactor;
	fixed_t playerheight;
	fixed_t playerspinheight;

	fixed_t speed;
	UINT8 secondjump;
	UINT8 fly1;
	tic_t glidetime;
	UINT8 climbing;
	INT32 deadtimer;
	tic_t exiting;
	UINT8 homing;
	tic_t dashmode;
	tic_t skidtime;
	fixed_t cmomx;
	fixed_t cmomy;
	fixed_t rmomx;
	fixed_t rmomy;

	INT32 weapondelay;
	INT32 tossdelay;

	INT16 starpostx;
	INT16 starposty;
	INT16 starpostz;
	INT32 starpostnum;
	tic_t starposttime;
	angle_t starpostangle;
	fixed_t starpostscale;

	INT32 maxlink;
	fixed_t dashspeed;
	angle_t angle_pos;
	angle_t old_angle_pos;
	tic_t bumpertime;
	INT32 flyangle;
	tic_t drilltimer;
	INT32 linkcount;
	tic_t linktimer;
	INT32 anotherflyangle;
	tic_t nightstime;
	INT32 drillmeter;
	UINT8 drilldelay;
	UINT8 bonustime;
	UINT8 mare;
	INT16 lastsidehit, lastlinehit;

	tic_t losstime;
	UINT8 timeshit;
	INT32 onconveyor;

	//player->mo stuff
	UINT8 hasmo; // Boolean

	INT32 health;
	angle_t angle;
	angle_t rollangle;
	fixed_t x;
	fixed_t y;
	fixed_t z;
	fixed_t momx;
	fixed_t momy;
	fixed_t momz;
	fixed_t friction;
	fixed_t movefactor;

	spritenum_t sprite;
	UINT32 frame;
	UINT8 sprite2;
	UINT16 anim_duration;
	INT32 tics;
	statenum_t statenum;
	UINT32 flags;
	UINT32 flags2;
	UINT16 eflags;

	fixed_t radius;
	fixed_t height;
	fixed_t scale;
	fixed_t destscale;
	fixed_t scalespeed;
} ATTRPACK resynch_pak;

typedef struct
{
	UINT8 version; // Different versions don't work
	UINT8 subversion; // Contains build version

	// Server launch stuffs
	UINT8 serverplayer;
	UINT8 totalslotnum; // "Slots": highest player number in use plus one.

	tic_t gametic;
	UINT8 clientnode;
	UINT8 gamestate;

	// 0xFF == not in game; else player skin num
	UINT8 playerskins[MAXPLAYERS];
	UINT16 playercolor[MAXPLAYERS];
	UINT32 playeravailabilities[MAXPLAYERS];

	UINT8 gametype;
	UINT8 modifiedgame;
	SINT8 adminplayers[MAXPLAYERS]; // Needs to be signed

	char server_context[8]; // Unique context id, generated at server startup.

	UINT8 varlengthinputs[0]; // Playernames and netvars
} ATTRPACK serverconfig_pak;

typedef struct
{
	UINT8 fileid;
	UINT32 filesize;
	UINT8 iteration;
	UINT32 position;
	UINT16 size;
	UINT8 data[0]; // Size is variable using hardware_MAXPACKETLENGTH
} ATTRPACK filetx_pak;

typedef struct
{
	UINT32 start;
	UINT32 acks;
} ATTRPACK fileacksegment_t;

typedef struct
{
	UINT8 fileid;
	UINT8 iteration;
	UINT8 numsegments;
	fileacksegment_t segments[0];
} ATTRPACK fileack_pak;

#ifdef _MSC_VER
#pragma warning(default : 4200)
#endif

#define MAXAPPLICATION 16

typedef struct
{
	UINT8 _255;/* see serverinfo_pak */
	UINT8 packetversion;
	char application[MAXAPPLICATION];
	UINT8 version; // Different versions don't work
	UINT8 subversion; // Contains build version
	UINT8 localplayers;
	UINT8 mode;
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME];
} ATTRPACK clientconfig_pak;

#define MAXSERVERNAME 32
#define MAXFILENEEDED 915
// This packet is too large
typedef struct
{
	/*
	In the old packet, 'version' is the first field. Now that field is set
	to 255 always, so older versions won't be confused with the new
	versions or vice-versa.
	*/
	UINT8 _255;
	UINT8 packetversion;
	char  application[MAXAPPLICATION];
	UINT8 version;
	UINT8 subversion;
	UINT8 numberofplayer;
	UINT8 maxplayer;
	UINT8 refusereason; // 0: joinable, 1: joins disabled, 2: full
	char gametypename[24];
	UINT8 modifiedgame;
	UINT8 cheatsenabled;
	UINT8 isdedicated;
	UINT8 fileneedednum;
	tic_t time;
	tic_t leveltime;
	char servername[MAXSERVERNAME];
	char mapname[8];
	char maptitle[33];
	unsigned char mapmd5[16];
	UINT8 actnum;
	UINT8 iszone;
	UINT8 fileneeded[MAXFILENEEDED]; // is filled with writexxx (byteptr.h)
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
	UINT8 num;
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
	UINT16 color;
	UINT32 pflags;
	UINT32 score;
	UINT8 ctfteam;
} ATTRPACK plrconfig;

//
// Network packet data
//
typedef struct
{
	UINT32 checksum;
	UINT8 ack; // If not zero the node asks for acknowledgement, the receiver must resend the ack
	UINT8 ackreturn; // The return of the ack number

	UINT8 packettype;
	UINT8 reserved; // Padding
	union
	{
		clientcmd_pak clientpak;            //         144 bytes
		client2cmd_pak client2pak;          //         200 bytes
		servertics_pak serverpak;           //      132495 bytes (more around 360, no?)
		serverconfig_pak servercfg;         //         773 bytes
		resynchend_pak resynchend;          //
		resynch_pak resynchpak;             //
		UINT8 resynchgot;                   //
		UINT8 textcmd[MAXTEXTCMD+1];        //       66049 bytes (wut??? 64k??? More like 257 bytes...)
		filetx_pak filetxpak;               //         139 bytes
		fileack_pak fileack;
		UINT8 filereceived;
		clientconfig_pak clientcfg;         //         136 bytes
		UINT8 md5sum[16];
		serverinfo_pak serverinfo;          //        1024 bytes
		serverrefuse_pak serverrefuse;      //       65025 bytes (somehow I feel like those values are garbage...)
		askinfo_pak askinfo;                //          61 bytes
		msaskinfo_pak msaskinfo;            //          22 bytes
		plrinfo playerinfo[MAXPLAYERS];     //         576 bytes(?)
		plrconfig playerconfig[MAXPLAYERS]; // (up to) 528 bytes(?)
		UINT32 pingtable[MAXPLAYERS+1];     //          68 bytes
	} u; // This is needed to pack diff packet types data together
} ATTRPACK doomdata_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#define MAXSERVERLIST 64 // Depends only on the display
typedef struct
{
	SINT8 node;
	serverinfo_pak info;
} serverelem_t;

extern serverelem_t serverlist[MAXSERVERLIST];
extern UINT32 serverlistcount;
extern INT32 mapchangepending;

// Points inside doomcom
extern doomdata_t *netbuffer;

extern consvar_t cv_showjoinaddress;
extern consvar_t cv_playbackspeed;

#define BASEPACKETSIZE      offsetof(doomdata_t, u)
#define FILETXHEADER        offsetof(filetx_pak, data)
#define BASESERVERTICSSIZE  offsetof(doomdata_t, u.serverpak.cmds[0])

#define KICK_MSG_GO_AWAY     1
#define KICK_MSG_CON_FAIL    2
#define KICK_MSG_PLAYER_QUIT 3
#define KICK_MSG_TIMEOUT     4
#define KICK_MSG_BANNED      5
#define KICK_MSG_PING_HIGH   6
#define KICK_MSG_CUSTOM_KICK 7
#define KICK_MSG_CUSTOM_BAN  8
#define KICK_MSG_KEEP_BODY   0x80

typedef enum
{
	KR_KICK          = 1, //Kicked by server
	KR_PINGLIMIT     = 2, //Broke Ping Limit
	KR_SYNCH         = 3, //Synch Failure
	KR_TIMEOUT       = 4, //Connection Timeout
	KR_BAN           = 5, //Banned by server
	KR_LEAVE         = 6, //Quit the game

} kickreason_t;

extern boolean server;
#define client (!server)
extern boolean dedicated; // For dedicated server
extern UINT16 software_MAXPACKETLENGTH;
extern boolean acceptnewnode;
extern SINT8 servernode;

void Command_Ping_f(void);
extern tic_t connectiontimeout;
extern tic_t jointimeout;
extern UINT16 pingmeasurecount;
extern UINT32 realpingtable[MAXPLAYERS];
extern UINT32 playerpingtable[MAXPLAYERS];
extern tic_t servermaxping;

extern consvar_t cv_allownewplayer, cv_joinnextround, cv_maxplayers, cv_joindelay, cv_rejointimeout;
extern consvar_t cv_resynchattempts, cv_blamecfail;
extern consvar_t cv_maxsend, cv_noticedownload, cv_downloadspeed;

// Used in d_net, the only dependence
tic_t ExpandTics(INT32 low, INT32 node);
void D_ClientServerInit(void);

// Initialise the other field
void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum));
void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam);
void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam); // splitsreen player
void SendKick(UINT8 playernum, UINT8 msg);

// Create any new ticcmds and broadcast to other players.
void NetUpdate(void);

void SV_StartSinglePlayerServer(void);
boolean SV_SpawnServer(void);
void SV_StopServer(void);
void SV_ResetServer(void);
void CL_AddSplitscreenPlayer(void);
void CL_RemoveSplitscreenPlayer(void);
void CL_Reset(void);
void CL_ClearPlayer(INT32 playernum);
void CL_UpdateServerList(boolean internetsearch, INT32 room);
// Is there a game running
boolean Playing(void);

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame(void);

//? How many ticks to run?
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

void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest);

extern UINT8 hu_resynching;

extern UINT8 adminpassmd5[16];
extern boolean adminpasswordset;
#endif
