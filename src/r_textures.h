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
/// \file  r_textures.h
/// \brief Texture generation.

#ifndef __R_TEXTURES__
#define __R_TEXTURES__

#include "r_defs.h"
#include "r_state.h"
#include "p_setup.h" // levelflats
#include "r_data.h"

#ifdef __GNUG__
#pragma interface
#endif

// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct
{
	// Block origin (always UL), which has already accounted for the internal origin of the patch.
	INT16 originx, originy;
	UINT16 wad, lump;
	UINT8 flip; // 1 = flipx, 2 = flipy, 3 = both
	UINT8 alpha; // Translucency value
	enum patchalphastyle style;
} texpatch_t;

// texture type
enum
{
	TEXTURETYPE_UNKNOWN,
	TEXTURETYPE_SINGLEPATCH,
	TEXTURETYPE_COMPOSITE,
	TEXTURETYPE_FLAT
};

// A texture_t describes a rectangular texture,
//  which is composed of one or more texpatch_t structures
//  that arrange graphic patches.
typedef struct
{
	// Keep name for switch changing, etc.
	char name[8];
	UINT32 hash;
	UINT8 type; // TEXTURETYPE_*
	boolean transparency;
	INT16 width, height;
	UINT8 flip; // 1 = flipx, 2 = flipy, 3 = both
	void *flat; // The texture, as a flat.

	// All the patches[patchcount] are drawn back to front into the cached texture.
	INT16 patchcount;
	texpatch_t patches[0];
} texture_t;

// all loaded and prepared textures from the start of the game
extern texture_t **textures;

extern INT32 *texturewidth;
extern fixed_t *textureheight; // needed for texture pegging

extern column_t **texturecolumns; // columns for each texture
extern UINT8 **texturecache; // graphics data for each generated full-size texture

// Load TEXTURES definitions, create lookup tables
void R_LoadTextures(void);
void R_LoadTexturesPwad(UINT16 wadnum);
void R_FlushTextureCache(void);

// Texture generation
UINT8 *R_GenerateTexture(size_t texnum);
UINT8 *R_GetFlatForTexture(size_t texnum);
INT32 R_GetTextureNum(INT32 texnum);
void R_CheckTextureCache(INT32 tex);
void R_ClearTextureNumCache(boolean btell);

// Retrieve texture data.
column_t *R_GetColumn(fixed_t tex, INT32 col);
void *R_GetFlat(levelflat_t *levelflat);

INT32 R_GetTextureNumForFlat(levelflat_t *levelflat);

boolean R_CheckPowersOfTwo(void);
boolean R_CheckSolidColorFlat(void);

UINT16 R_GetFlatSize(size_t length);
UINT8 R_GetFlatBits(INT32 size);
void R_SetFlatVars(size_t length);

// Returns the texture number for the texture name.
INT32 R_TextureNumForName(const char *name);
INT32 R_CheckTextureNumForName(const char *name);
INT32 R_CheckFlatNumForName(const char *name);

// Returns the texture name for the texture number (in case you ever needed it)
const char *R_CheckTextureNameForNum(INT32 num);
const char *R_TextureNameForNum(INT32 num);

extern INT32 numtextures;

#endif
