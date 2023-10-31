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

boolean PaletteRemap_AddIndexRange(remaptable_t *tr, int start, int end, int pal1, int pal2);
boolean PaletteRemap_AddColorRange(remaptable_t *tr, int start, int end, int _r1, int _g1, int _b1, int _r2, int _g2, int _b2);
boolean PaletteRemap_AddDesaturation(remaptable_t *tr, int start, int end, double r1, double g1, double b1, double r2, double g2, double b2);
boolean PaletteRemap_AddColourisation(remaptable_t *tr, int start, int end, int r, int g, int b);
boolean PaletteRemap_AddTint(remaptable_t *tr, int start, int end, int r, int g, int b, int amount);

enum PaletteRemapType
{
	REMAP_ADD_INDEXRANGE,
	REMAP_ADD_COLORRANGE,
	REMAP_ADD_COLOURISATION,
	REMAP_ADD_DESATURATION,
	REMAP_ADD_TINT
};

struct PaletteRemapParseResult
{
	int start, end;
	enum PaletteRemapType type;
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

	boolean has_error;
	char error[4096];
};

struct PaletteRemapParseResult *PaletteRemap_ParseTranslation(const char *translation);

void PaletteRemap_ApplyResult(remaptable_t *tr, struct PaletteRemapParseResult *data);

typedef struct CustomTranslation
{
	char *name;
	unsigned id;
	UINT32 hash;
} customtranslation_t;

extern customtranslation_t *customtranslations;
extern unsigned numcustomtranslations;

int R_FindCustomTranslation(const char *name);
void R_AddCustomTranslation(const char *name, int trnum);
unsigned R_NumCustomTranslations(void);
remaptable_t *R_GetTranslationByID(int id);
boolean R_TranslationIsValid(int id);

void R_ParseTrnslate(INT32 wadNum, UINT16 lumpnum);
void R_LoadParsedTranslations(void);

remaptable_t *R_GetBuiltInTranslation(SINT8 tc);

#endif
