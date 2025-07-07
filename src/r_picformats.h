// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2018-2024 by Lactozilla.
// Copyright (C) 2019-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_picformats.h
/// \brief Patch generation.

#ifndef __R_PICFORMATS__
#define __R_PICFORMATS__

#include "r_defs.h"
#include "doomdef.h"

typedef enum
{
	PICFMT_NONE = 0,

	// Doom formats
	PICFMT_PATCH,
	PICFMT_FLAT,
	PICFMT_DOOMPATCH,

	// PNG
	PICFMT_PNG,

	// 16bpp
	PICFMT_PATCH16,
	PICFMT_FLAT16,
	PICFMT_DOOMPATCH16,

	// 32bpp
	PICFMT_PATCH32,
	PICFMT_FLAT32,
	PICFMT_DOOMPATCH32
} pictureformat_t;

typedef enum
{
	PICFLAGS_XFLIP                = 1,
	PICFLAGS_YFLIP                = 1<<1,
	PICFLAGS_USE_TRANSPARENTPIXEL = 1<<2
} pictureflags_t;

enum
{
	PICDEPTH_NONE = 0,
	PICDEPTH_8BPP = 8,
	PICDEPTH_16BPP = 16,
	PICDEPTH_32BPP = 32
};

// Maximum allowed dimensions for a patch
#define MAX_PATCH_DIMENSIONS 8192

// Minimum amount of bytes required for a valid patch lump header
#define MIN_PATCH_LUMP_HEADER_SIZE ((sizeof(INT16) * 4) + sizeof(INT32))

// Minimum length of a valid Doom patch lump
// This is the size of a 1x1 patch.
#define MIN_PATCH_LUMP_SIZE (MIN_PATCH_LUMP_HEADER_SIZE + 1)

// Gets the offset to the very first column in a patch lump
#define FIRST_PATCH_LUMP_COLUMN(width) ((sizeof(INT16) * 4) + ((width) * sizeof(INT32)))

// Checks if the size of a lump is valid for a patch, given a certain width
#define VALID_PATCH_LUMP_SIZE(lumplen, width) ((lumplen) >= FIRST_PATCH_LUMP_COLUMN(width))

// Minimum size of a PNG file.
// See: https://web.archive.org/web/20230524232139/http://garethrees.org/2007/11/14/pngcrush/
#define PNG_MIN_SIZE 67

// Size of a PNG header
#define PNG_HEADER_SIZE 8

void *Picture_Convert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	INT32 inwidth, INT32 inheight, INT32 inleftoffset, INT32 intopoffset,
	pictureflags_t flags);

void *Picture_PatchConvert(
	pictureformat_t informat, size_t insize, void *picture,
	pictureformat_t outformat, size_t *outsize,
	INT32 inwidth, INT32 inheight, INT32 inleftoffset, INT32 intopoffset,
	pictureflags_t flags);
void *Picture_FlatConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t *outsize,
	INT32 inwidth, INT32 inheight,
	pictureflags_t flags);
void *Picture_GetPatchPixel(
	patch_t *patch, pictureformat_t informat,
	INT32 x, INT32 y,
	pictureflags_t flags);

void *Picture_TextureToFlat(size_t texnum);

INT32 Picture_FormatBPP(pictureformat_t format);
boolean Picture_IsPatchFormat(pictureformat_t format);
boolean Picture_IsInternalPatchFormat(pictureformat_t format);
boolean Picture_IsDoomPatchFormat(pictureformat_t format);
boolean Picture_IsFlatFormat(pictureformat_t format);
boolean Picture_CheckIfDoomPatch(softwarepatch_t *patch, size_t size);

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
} spriteframepivot_t;

typedef struct
{
	spriteframepivot_t pivot[MAXFRAMENUM];
	boolean available;
} spriteinfo_t;

boolean Picture_IsLumpPNG(const UINT8 *d, size_t s);

#ifndef NO_PNG_LUMPS
void *Picture_PNGConvert(
	const UINT8 *png, pictureformat_t outformat,
	INT32 *w, INT32 *h,
	INT16 *topoffset, INT16 *leftoffset,
	size_t insize, size_t *outsize,
	pictureflags_t flags);
boolean Picture_PNGDimensions(UINT8 *png, INT32 *width, INT32 *height, INT16 *topoffset, INT16 *leftoffset, size_t size);

#define PICTURE_PNG_USELOOKUP
#endif

// SpriteInfo
extern spriteinfo_t spriteinfo[NUMSPRITES];
void R_LoadSpriteInfoLumps(UINT16 wadnum, UINT16 numlumps);
void R_ParseSPRTINFOLump(UINT16 wadNum, UINT16 lumpNum);

#endif // __R_PICFORMATS__
