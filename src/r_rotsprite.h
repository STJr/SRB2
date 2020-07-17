// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2019-2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2019-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_rotsprite.h
/// \brief Rotated patch generation.

#ifndef __R_ROTSPRITE__
#define __R_ROTSPRITE__

#include "r_defs.h"
#include "r_patch.h"
#include "doomtype.h"

#ifdef ROTSPRITE
INT32 R_GetRollAngle(angle_t rollangle);

typedef struct
{
	pixelmap_t pixelmap[ROTANGLES];
	patch_t *patches[ROTANGLES];
	boolean cached[ROTANGLES];

	UINT32 lumpnum;
	rotsprite_vars_t vars;
	INT32 tag;
} rotsprite_t;

rotsprite_t *RotSprite_GetFromPatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag, rotsprite_vars_t rsvars, boolean store);
rotsprite_t *RotSprite_GetFromPatchNum(lumpnum_t lumpnum, INT32 tag, rotsprite_vars_t rsvars, boolean store);
rotsprite_t *RotSprite_GetFromPatchName(const char *name, INT32 tag, rotsprite_vars_t rsvars, boolean store);
rotsprite_t *RotSprite_GetFromPatchLongName(const char *name, INT32 tag, rotsprite_vars_t rsvars, boolean store);

void RotSprite_Create(rotsprite_t *rotsprite, rotsprite_vars_t rsvars);
void RotSprite_CreateColumns(pixelmap_t *pixelmap, pmcache_t *cache, patch_t *patch, rotsprite_vars_t rsvars);
void RotSprite_CreatePixelMap(patch_t *patch, pixelmap_t *pixelmap, rotsprite_vars_t rsvars);
patch_t *RotSprite_CreatePatch(rotsprite_t *rotsprite, rotsprite_vars_t rsvars);

void RotSprite_Recreate(rotsprite_t *rotsprite, rendermode_t rmode);
void RotSprite_RecreateAll(void);

void RotSprite_InitPatchTree(patchtree_t *rcache);
void RotSprite_AllocCurrentPatchInfo(patchinfo_t *patchinfo, UINT16 lumpnum);
int RotSprite_GetCurrentPatchInfoIdx(INT32 rollangle, boolean flip);

#endif // ROTSPRITE

#endif // __R_ROTSPRITE__
