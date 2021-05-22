// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_saveg.h
/// \brief Savegame I/O, archiving, persistence

#ifndef __P_SAVEG__
#define __P_SAVEG__

#ifdef __GNUG__
#pragma interface
#endif

// Persistent storage/archiving.
// These are the load / save game routines.

void P_SaveGame(INT16 mapnum);
void P_SaveNetGame(boolean resending);
boolean P_LoadGame(INT16 mapoverride);
boolean P_LoadNetGame(boolean reloading);

mobj_t *P_FindNewPosition(UINT32 oldposition);

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

#endif
