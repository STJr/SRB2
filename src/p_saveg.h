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

#ifdef __GNUG__
#pragma interface
#endif

#include "tables.h"

#define NEWSKINSAVES (INT16_MAX) // TODO: 2.3: Delete (Purely for backwards compatibility)

// Persistent storage/archiving.
// These are the load / save game routines.

typedef struct
{
	unsigned char *buf;
	size_t size;
	size_t pos;
} save_t;

void P_SaveGame(save_t *save_p, INT16 mapnum);
void P_SaveNetGame(save_t *save_p, boolean resending);
boolean P_LoadGame(save_t *save_p, INT16 mapoverride);
boolean P_LoadNetGame(save_t *save_p, boolean reloading);

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

void P_WriteUINT8(save_t *p, UINT8 v);
void P_WriteSINT8(save_t *p, SINT8 v);
void P_WriteUINT16(save_t *p, UINT16 v);
void P_WriteINT16(save_t *p, INT16 v);
void P_WriteUINT32(save_t *p, UINT32 v);
void P_WriteINT32(save_t *p, INT32 v);
void P_WriteChar(save_t *p, char v);
void P_WriteFixed(save_t *p, fixed_t v);
void P_WriteAngle(save_t *p, angle_t v);
void P_WriteStringN(save_t *p, char const *s, size_t n);
void P_WriteStringL(save_t *p, char const *s, size_t n);
void P_WriteString(save_t *p, char const *s);
void P_WriteMem(save_t *p, void const *s, size_t n);

void P_SkipStringN(save_t *p, size_t n);
void P_SkipStringL(save_t *p, size_t n);
void P_SkipString(save_t *p);

UINT8 P_ReadUINT8(save_t *p);
SINT8 P_ReadSINT8(save_t *p);
UINT16 P_ReadUINT16(save_t *p);
INT16 P_ReadINT16(save_t *p);
UINT32 P_ReadUINT32(save_t *p);
INT32 P_ReadINT32(save_t *p);
char P_ReadChar(save_t *p);
fixed_t P_ReadFixed(save_t *p);
angle_t P_ReadAngle(save_t *p);
void P_ReadStringN(save_t *p, char *s, size_t n);
void P_ReadStringL(save_t *p, char *s, size_t n);
void P_ReadString(save_t *p, char *s);
void P_ReadMem(save_t *p, void *s, size_t n);

#endif
