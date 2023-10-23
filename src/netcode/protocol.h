// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  protocol.h
/// \brief Data exchanged through the network

#ifndef __PROTOCOL__
#define __PROTOCOL__

#include "d_net.h"
#include "../d_ticcmd.h"
#include "../doomdef.h"

/*
The 'packet version' is used to distinguish packet
formats. This version is independent of VERSION and
SUBVERSION. Different applications may follow different
packet versions.

If you change the struct or the meaning of a field
therein, increment this number.
*/
#define PACKETVERSION 4

// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.

#define BACKUPTICS 1024
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

	PT_WILLRESENDGAMESTATE, // Hey Client, I am about to resend you the gamestate!
	PT_CANRECEIVEGAMESTATE, // Okay Server, I'm ready to receive it, you can go ahead.
	PT_RECEIVEDGAMESTATE,   // Thank you Server, I am ready to play again!

	PT_SENDINGLUAFILE, // Server telling a client Lua needs to open a file
	PT_ASKLUAFILE,     // Client telling the server they don't have the file
	PT_HASLUAFILE,     // Client telling the server they have the file

	PT_BASICKEEPALIVE, // Keep the network alive during wipes, as tics aren't advanced and NetUpdate isn't called

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

	PT_LOGIN,         // Login attempt from the client.

	PT_TELLFILESNEEDED, // Client, to server: "what other files do I need starting from this number?"
	PT_MOREFILESNEEDED, // Server, to client: "you need these (+ more on top of those)"

	PT_PING,          // Packet sent to tell clients the other client's latency to server.
	NUMPACKETTYPE
} packettype_t;

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
typedef struct
{
	tic_t starttic;
	UINT8 numtics;
	UINT8 numslots; // "Slots filled": Highest player number in use plus one.
	ticcmd_t cmds[45];
} ATTRPACK servertics_pak;

typedef struct
{
	// Server launch stuffs
	UINT8 serverplayer;
	UINT8 totalslotnum; // "Slots": highest player number in use plus one.

	tic_t gametic;
	UINT8 clientnode;
	UINT8 gamestate;

	UINT8 gametype;
	UINT8 modifiedgame;
	UINT8 usedCheats;

	char server_context[8]; // Unique context id, generated at server startup.
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
	UINT8 modversion;
	char application[MAXAPPLICATION];
	UINT8 localplayers;
	UINT8 mode;
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME];
} ATTRPACK clientconfig_pak;

#define SV_DEDICATED    0x40 // server is dedicated
#define SV_LOTSOFADDONS 0x20 // flag used to ask for full file list in d_netfil

enum {
	REFUSE_JOINS_DISABLED = 1,
	REFUSE_SLOTS_FULL,
	REFUSE_BANNED,
};

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
	UINT8 refusereason; // 0: joinable, REFUSE enum
	char gametypename[24];
	UINT8 modifiedgame;
	UINT8 cheatsenabled;
	UINT8 flags;
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
} ATTRPACK plrinfo_pak;

// Shortest player information for join during intermission.
typedef struct
{
	char name[MAXPLAYERNAME+1];
	UINT8 skin;
	UINT16 color;
	UINT32 pflags;
	UINT32 score;
	UINT8 ctfteam;
} ATTRPACK plrconfig_pak;

typedef struct
{
	INT32 first;
	UINT8 num;
	UINT8 more;
	UINT8 files[MAXFILENEEDED]; // is filled with writexxx (byteptr.h)
} ATTRPACK filesneededconfig_pak;

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
		clientcmd_pak clientpak;
		client2cmd_pak client2pak;
		servertics_pak serverpak;
		serverconfig_pak servercfg;
		UINT8 textcmd[MAXTEXTCMD+1];
		filetx_pak filetxpak;
		fileack_pak fileack;
		UINT8 filereceived;
		clientconfig_pak clientcfg;
		UINT8 md5sum[16];
		serverinfo_pak serverinfo;
		serverrefuse_pak serverrefuse;
		askinfo_pak askinfo;
		msaskinfo_pak msaskinfo;
		plrinfo_pak playerinfo[MAXPLAYERS];
		plrconfig_pak playerconfig[MAXPLAYERS];
		INT32 filesneedednum;
		filesneededconfig_pak filesneededcfg;
		UINT32 pingtable[MAXPLAYERS+1];
	} u; // This is needed to pack diff packet types data together
} ATTRPACK doomdata_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#define FILETXHEADER        offsetof(filetx_pak, data)

#define KICK_MSG_GO_AWAY     1
#define KICK_MSG_CON_FAIL    2
#define KICK_MSG_PLAYER_QUIT 3
#define KICK_MSG_TIMEOUT     4
#define KICK_MSG_BANNED      5
#define KICK_MSG_PING_HIGH   6
#define KICK_MSG_CUSTOM_KICK 7
#define KICK_MSG_CUSTOM_BAN  8
#define KICK_MSG_KEEP_BODY   0x80

#endif
