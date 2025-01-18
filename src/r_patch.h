// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2024 by Lactozilla.
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
#include "r_picformats.h"
#include "r_fps.h"
#include "doomdef.h"

// Patch functions
patch_t *Patch_Create(INT16 width, INT16 height);
patch_t *Patch_CreateFromDoomPatch(softwarepatch_t *source);
void Patch_CalcDataSizes(softwarepatch_t *source, size_t *total_pixels, size_t *total_posts);
void Patch_MakeColumns(softwarepatch_t *source, size_t num_columns, INT16 width, UINT8 *pixels, column_t *columns, post_t *posts, boolean flip);
void Patch_Free(patch_t *patch);

#define Patch_FreeTag(tagnum) Patch_FreeTags(tagnum, tagnum)
void Patch_FreeTags(INT32 lowtag, INT32 hightag);

void Patch_GenerateFlat(patch_t *patch, pictureflags_t flags);

#ifdef HWRENDER
void *Patch_AllocateHardwarePatch(patch_t *patch);
void *Patch_CreateGL(patch_t *patch);
#endif

#ifdef ROTSPRITE
void Patch_Rotate(patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip);
patch_t *Patch_GetRotated(patch_t *patch, INT32 angle, boolean flip);
patch_t *Patch_GetRotatedSprite(
	spriteframe_t *sprite,
	size_t frame, size_t spriteangle,
	boolean flip,
	void *info, INT32 rotationangle);
angle_t R_ModelRotationAngle(interpmobjstate_t *interp);
angle_t R_SpriteRotationAngle(interpmobjstate_t *interp);
INT32 R_GetRollAngle(angle_t rollangle);
#endif

#endif // __R_PATCH__
