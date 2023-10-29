// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_translation.h
/// \brief Translations

#include "doomdef.h"

typedef struct
{
	UINT8 remap[256];
	unsigned num_entries;
} remaptable_t;

extern remaptable_t **paletteremaps;
extern unsigned numpaletteremaps;

void PaletteRemap_Init(void);
remaptable_t *PaletteRemap_New(void);
remaptable_t *PaletteRemap_Copy(remaptable_t *tr);
boolean PaletteRemap_Equal(remaptable_t *a, remaptable_t *b);
void PaletteRemap_SetIdentity(remaptable_t *tr);
boolean PaletteRemap_IsIdentity(remaptable_t *tr);
unsigned PaletteRemap_Add(remaptable_t *tr);

boolean PaletteRemap_AddIndexRange(remaptable_t *tr, int start, int end, int pal1, int pal2);
boolean PaletteRemap_AddColorRange(remaptable_t *tr, int start, int end, int _r1,int _g1, int _b1, int _r2, int _g2, int _b2);
boolean PaletteRemap_AddDesaturation(remaptable_t *tr, int start, int end, double r1, double g1, double b1, double r2, double g2, double b2);
boolean PaletteRemap_AddColourisation(remaptable_t *tr, int start, int end, int r, int g, int b);
boolean PaletteRemap_AddTint(remaptable_t *tr, int start, int end, int r, int g, int b, int amount);

struct PaletteRemapParseResult
{
	char error[4096];
};

struct PaletteRemapParseResult *PaletteRemap_ParseString(remaptable_t *tr, char *translation);

int R_FindCustomTranslation(const char *name);
void R_AddCustomTranslation(const char *name, int trnum);
remaptable_t *R_GetTranslationByID(int id);

void R_LoadTrnslateLumps(void);
