// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2006 by Randy Heit.
// Copyright (C) 2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_translation.h
/// \brief Translation table handling

#ifndef __R_TRANSLATION__
#define __R_TRANSLATION__

#include "doomdef.h"

typedef struct
{
	UINT8 remap[256];
	unsigned num_entries;
} remaptable_t;

void PaletteRemap_Init(void);
remaptable_t *PaletteRemap_New(void);
remaptable_t *PaletteRemap_Copy(remaptable_t *tr);
boolean PaletteRemap_Equal(remaptable_t *a, remaptable_t *b);
void PaletteRemap_SetIdentity(remaptable_t *tr);
boolean PaletteRemap_IsIdentity(remaptable_t *tr);
unsigned PaletteRemap_Add(remaptable_t *tr);

int R_FindCustomTranslation(const char *name);
int R_FindCustomTranslation_CaseInsensitive(const char *name);
void R_AddCustomTranslation(const char *name, int trnum);
const char *R_GetCustomTranslationName(unsigned id);
unsigned R_NumCustomTranslations(void);
remaptable_t *R_GetTranslationByID(int id);
boolean R_TranslationIsValid(int id);

void R_ParseTrnslate(INT32 wadNum, UINT16 lumpnum);
void R_LoadParsedTranslations(void);

remaptable_t *R_GetBuiltInTranslation(SINT8 tc);

#endif
