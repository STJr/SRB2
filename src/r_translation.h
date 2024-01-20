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

typedef enum
{
	REMAP_ADD_INDEXRANGE,
	REMAP_ADD_COLORRANGE,
	REMAP_ADD_COLOURISATION,
	REMAP_ADD_DESATURATION,
	REMAP_ADD_TINT,
	REMAP_ADD_INVERT
} paletteremaptype_t;

typedef struct
{
	int start, end;
	paletteremaptype_t type;
	union
	{
		struct
		{
			int pal1, pal2;
		} indexRange;
		struct
		{
			int r1, g1, b1;
			int r2, g2, b2;
		} colorRange;
		struct
		{
			double r1, g1, b1;
			double r2, g2, b2;
		} desaturation;
		struct
		{
			int r, g, b;
		} colourisation;
		struct
		{
			int r, g, b, amount;
		} tint;
	};
} paletteremap_t;

typedef struct
{
	UINT8 remap[256];
	unsigned num_entries;

	paletteremap_t *sources;
	unsigned num_sources;

	// A typical remap is 256 bytes long, and there is currently a maximum of 1182 skincolors.
	// This means allocating (1182 * 256) bytes, which equals 302592, or ~302kb of memory for every translation.
	// So we allocate a list instead.
	UINT8 **skincolor_remap;
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
UINT8 *R_GetTranslationRemap(int id, skincolornum_t skincolor, INT32 skinnum);
boolean R_TranslationIsValid(int id);

void R_ParseTrnslate(INT32 wadNum, UINT16 lumpnum);
void R_LoadParsedTranslations(void);

remaptable_t *R_GetBuiltInTranslation(SINT8 tc);

#endif
