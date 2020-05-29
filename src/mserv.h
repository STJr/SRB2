// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mserv.h
/// \brief Header file for the master server routines

#ifndef _MSERV_H_
#define _MSERV_H_

#define MASTERSERVERS21 // MasterServer v2.1

// lowered from 32 due to menu changes
#define NUM_LIST_ROOMS 16

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef union
{
	char buffer[16]; // information such as password
	UINT32 signature;
} ATTRPACK msg_header_t;

// Keep this structure 8 bytes aligned (current size is 80)
typedef struct
{
	msg_header_t header;
	char ip[16];
	char port[8];
	char name[32];
	INT32 room;
	char version[8]; // format is: x.yy.z (like 1.30.2 or 1.31)
} ATTRPACK msg_server_t;

typedef struct
{
	msg_header_t header;
	INT32 id;
	char name[32];
	char motd[255];
} ATTRPACK msg_rooms_t;

typedef struct
{
	msg_header_t header;
	char ipstart[16];
	char ipend[16];
	char endstamp[32];
	char reason[255];
	INT32 hostonly;
} ATTRPACK msg_ban_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

// ================================ GLOBALS ===============================

extern consvar_t cv_masterserver, cv_servername;

// < 0 to not connect (usually -1) (offline mode)
// == 0 to show all rooms, not a valid hosting room
// anything else is whatever room the MS assigns to that number (online mode)
extern INT16 ms_RoomId;

const char *GetMasterServerPort(void);
const char *GetMasterServerIP(void);

void MSOpenUDPSocket(void);
void MSCloseUDPSocket(void);

void SendAskInfoViaMS(INT32 node, tic_t asktime);

void RegisterServer(void);
void UnregisterServer(void);

void MasterClient_Ticker(void);

const msg_server_t *GetShortServersList(INT32 room);
INT32 GetRoomsList(boolean hosting);
#ifdef UPDATE_ALERT
const char *GetMODVersion(void);
void GetMODVersion_Console(void);
#endif
extern msg_rooms_t room_list[NUM_LIST_ROOMS+1];

void AddMServCommands(void);

#endif
