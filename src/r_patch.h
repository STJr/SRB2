// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Jaime "Lactozilla" Passos.
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

enum
{
	PATCH_TYPE_STATIC,
	PATCH_TYPE_DYNAMIC
};

// Patch functions
patch_t *Patch_Create(INT16 width, INT16 height);
patch_t *Patch_CreateDynamic(INT16 width, INT16 height);

void *Patch_GetPixel(patch_t *patch, INT32 x, INT32 y);
void Patch_SetPixel(patch_t *patch, void *pixel, pictureformat_t informat, INT32 x, INT32 y, boolean transparent_overwrite);
void Patch_Update(patch_t *patch,
	void *pixels, INT32 src_img_width, INT32 src_img_height,
	pictureformat_t informat,
	INT32 sx, INT32 sy, INT32 sw, INT32 sh, INT32 dx, INT32 dy,
	boolean transparent_overwrite);

void Patch_Free(patch_t *patch);
void Patch_FreeMiscData(patch_t *patch);

patch_t *Patch_CreateFromDoomPatch(softwarepatch_t *source);
void Patch_CalcDataSizes(softwarepatch_t *source, size_t *total_pixels, size_t *total_posts);
void Patch_MakeColumns(softwarepatch_t *source, size_t num_columns, INT16 width, UINT8 *pixels, column_t *columns, post_t *posts, boolean flip);

column_t *Patch_GetColumn(patch_t *patch, unsigned column);

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
	boolean flip, boolean adjustfeet,
	void *info, INT32 rotationangle);
angle_t R_ModelRotationAngle(interpmobjstate_t *interp);
angle_t R_SpriteRotationAngle(interpmobjstate_t *interp);
INT32 R_GetRollAngle(angle_t rollangle);
#endif

#endif // __R_PATCH__
