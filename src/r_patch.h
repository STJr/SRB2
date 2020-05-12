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
#include "w_wad.h"
#include "doomdef.h"

void *R_CacheSoftwarePatch(UINT16 wad, UINT16 lump, INT32 tag, boolean store);
void *R_CacheGLPatch(UINT16 wad, UINT16 lump, INT32 tag, boolean store);

void R_UpdatePatchReferences(void);
void R_FreePatchReferences(void);

void R_InitPatchCache(wadfile_t *wadfile);

// Structs
struct patchreference_s
{
	struct patchreference_s *prev, *next;
	UINT16 wad, lump;
	INT32 tag;
	void *ptr;
};
typedef struct patchreference_s patchreference_t;

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

boolean R_ApplyPixelMapToColumn(pixelmap_t *pmap, INT32 *map, patch_t *patch, UINT8 *post, size_t *colsize, boolean flipped);
UINT8 *GetPatchPixel(patch_t *patch, INT32 x, INT32 y, boolean flip);

// Conversions between patches / flats / textures...
boolean R_CheckIfPatch(lumpnum_t lump);
void R_TextureToFlat(size_t tex, UINT8 *flat);
void R_PatchToFlat(patch_t *patch, UINT8 *flat);
patch_t *R_FlatToPatch(UINT8 *raw, UINT16 width, UINT16 height, UINT16 leftoffset, UINT16 topoffset, size_t *destsize, boolean transparency);
patch_t *R_TransparentFlatToPatch(UINT16 *raw, UINT16 width, UINT16 height, UINT16 leftoffset, UINT16 topoffset, size_t *destsize);

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

rotsprite_t *R_GetRotSpriteNumPwad(UINT16 wad, UINT16 lump, INT32 tag);
rotsprite_t *R_GetRotSpriteNum(lumpnum_t lumpnum, INT32 tag);
rotsprite_t *R_GetRotSpriteName(const char *name, INT32 tag);
rotsprite_t *R_GetRotSpriteLongName(const char *name, INT32 tag);

void R_CacheRotSprite(rotsprite_t *rotsprite, INT32 rollangle, spriteframepivot_t *pivot, boolean flip);
void R_CacheRotSpriteColumns(pixelmap_t *pixelmap, pmcache_t *cache, patch_t *patch, boolean flip);
void R_CacheRotSpritePixelMap(INT32 rollangle, patch_t *patch, pixelmap_t *pixelmap, spriteframepivot_t *spritepivot, boolean flip);
patch_t *R_CacheRotSpritePatch(rotsprite_t *rotsprite, INT32 rollangle, boolean flip);

patch_t *R_GetRotatedPatch(UINT32 lumpnum, INT32 tag, INT32 rollangle, boolean flip);
patch_t *R_GetRotatedPatchForSprite(UINT32 lumpnum, INT32 tag, INT32 rollangle, spriteframepivot_t *pivot, boolean flip);
patch_t *R_GetRotatedPatchPwad(UINT16 wad, UINT16 lump, INT32 tag, INT32 rollangle, boolean flip);
patch_t *R_GetRotatedPatchName(const char *name, INT32 tag, INT32 rollangle, boolean flip);
patch_t *R_GetRotatedPatchLongName(const char *name, INT32 tag, INT32 rollangle, boolean flip);

void R_FreeRotSprite(rotsprite_t *rotsprite);
void R_FreeAllRotSprites(void);

extern fixed_t rollcosang[ROTANGLES];
extern fixed_t rollsinang[ROTANGLES];
#endif

#endif // __R_PATCH__
