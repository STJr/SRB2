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
/// \file  p_saveg.h
/// \brief Savegame I/O, archiving, persistence

#ifndef __P_SAVEG__
#define __P_SAVEG__

#ifdef __cplusplus
extern "C" {
#endif

#define NEWSKINSAVES (INT16_MAX) // TODO: 2.3: Delete (Purely for backwards compatibility)

// Persistent storage/archiving.
// These are the load / save game routines.

void P_SaveGame(INT16 mapnum);
void P_SaveNetGame(boolean resending);
boolean P_LoadGame(INT16 mapoverride);
boolean P_LoadNetGame(boolean reloading);

mobj_t *P_FindNewPosition(UINT32 oldposition);

typedef struct
{
	UINT8 *buffer;
	UINT8 *p;
	UINT8 *end;
	size_t size;
} savebuffer_t;

typedef struct
{
	UINT8 skin;
	UINT8 botskin;
	INT32 score;
	INT32 lives;
	INT32 continues;
	UINT16 emeralds;
	UINT8 numgameovers;
} savedata_t;

extern savedata_t savedata;
extern UINT8 *save_p;
extern UINT8 *save_start;
extern UINT8 *save_end;
extern size_t save_length;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
