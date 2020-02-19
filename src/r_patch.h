// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2018-2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2019-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patch.h
/// \brief Patch generation.

#ifndef __R_PATCH__
#define __R_PATCH__

#include "r_defs.h"
#include "doomdef.h"

// Structs
typedef enum
{
	ROTAXIS_X, // Roll (the default)
	ROTAXIS_Y, // Pitch
	ROTAXIS_Z  // Yaw
} rotaxis_t;

typedef struct
{
	INT32 x, y;
	rotaxis_t rotaxis;
} spriteframepivot_t;

typedef struct
{
	spriteframepivot_t pivot[64];
	boolean available;
} spriteinfo_t;

// Conversions between patches / flats / textures...
boolean R_CheckIfPatch(lumpnum_t lump);
void R_TextureToFlat(size_t tex, UINT8 *flat);
void R_PatchToFlat(patch_t *patch, UINT8 *flat);
void R_PatchToMaskedFlat(patch_t *patch, UINT16 *raw, boolean flip);
patch_t *R_FlatToPatch(UINT8 *raw, UINT16 width, UINT16 height, UINT16 leftoffset, UINT16 topoffset, size_t *destsize, boolean transparency);
patch_t *R_MaskedFlatToPatch(UINT16 *raw, UINT16 width, UINT16 height, UINT16 leftoffset, UINT16 topoffset, size_t *destsize);

// Portable Network Graphics
boolean R_IsLumpPNG(const UINT8 *d, size_t s);
#define W_ThrowPNGError(lumpname, wadfilename) I_Error("W_Wad: Lump \"%s\" in file \"%s\" is a .png - please convert to either Doom or Flat (raw) image format.", lumpname, wadfilename); // Fears Of LJ Sonic

#ifndef NO_PNG_LUMPS
UINT8 *R_PNGToFlat(UINT16 *width, UINT16 *height, UINT8 *png, size_t size);
patch_t *R_PNGToPatch(const UINT8 *png, size_t size, size_t *destsize);
boolean R_PNGDimensions(UINT8 *png, INT16 *width, INT16 *height, size_t size);
#endif

// SpriteInfo
extern spriteinfo_t spriteinfo[NUMSPRITES];
void R_LoadSpriteInfoLumps(UINT16 wadnum, UINT16 numlumps);
void R_ParseSPRTINFOLump(UINT16 wadNum, UINT16 lumpNum);

// Sprite rotation
#ifdef ROTSPRITE
INT32 R_GetRollAngle(angle_t rollangle);
void R_CacheRotSprite(spritenum_t sprnum, UINT8 frame, spriteinfo_t *sprinfo, spriteframe_t *sprframe, INT32 rot, UINT8 flip);
void R_FreeSingleRotSprite(spritedef_t *spritedef);
void R_FreeSkinRotSprite(size_t skinnum);
extern fixed_t rollcosang[ROTANGLES];
extern fixed_t rollsinang[ROTANGLES];
void R_FreeAllRotSprite(void);
#endif

#endif // __R_PATCH__
