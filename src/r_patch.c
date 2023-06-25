// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patch.c
/// \brief Patch generation.

#include "doomdef.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_defs.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

//
// Creates a patch.
// Assumes a PU_PATCH zone memory tag and no user.
//

patch_t *Patch_Create(softwarepatch_t *source, size_t srcsize, void *dest)
{
	patch_t *patch = (dest == NULL) ? Z_Calloc(sizeof(patch_t), PU_PATCH, NULL) : (patch_t *)(dest);

	if (source)
	{
		INT32 col, colsize;
		size_t size = sizeof(INT32) * SHORT(source->width);
		size_t offs = (sizeof(INT16) * 4) + size;

		patch->width      = SHORT(source->width);
		patch->height     = SHORT(source->height);
		patch->leftoffset = SHORT(source->leftoffset);
		patch->topoffset  = SHORT(source->topoffset);
		patch->columnofs  = Z_Calloc(size, PU_PATCH_DATA, NULL);

		for (col = 0; col < source->width; col++)
		{
			// This makes the column offsets relative to the column data itself,
			// instead of the entire patch data
			patch->columnofs[col] = LONG(source->columnofs[col]) - offs;
		}

		if (!srcsize)
			I_Error("Patch_Create: no source size!");

		colsize = (INT32)(srcsize) - (INT32)offs;
		if (colsize <= 0)
			I_Error("Patch_Create: no column data!");

		patch->columns = Z_Calloc(colsize, PU_PATCH_DATA, NULL);
		M_Memcpy(patch->columns, ((UINT8 *)source + LONG(source->columnofs[0])), colsize);
	}

	patch->format = PICFMT_PATCH;
	patch->extra = Z_Calloc(sizeof(patchextra_t), PU_PATCH_DATA, NULL);

	return patch;
}

//
// Frees a patch from memory.
//

static void Patch_FreeExtra(patchextra_t *ext)
{
	INT32 i;

	if (ext->truecolor)
		Patch_Free(ext->truecolor);

	if (ext->source.data)
		Z_Free(ext->source.data);

	if (ext->flats)
	{
		for (i = 0; i < 4; i++)
		{
			if (ext->flats[i])
				Z_Free(ext->flats[i]);
		}

		Z_Free(ext->flats);
	}

#ifdef ROTSPRITE
	if (ext->rotated)
	{
		rotsprite_t *rotsprite = ext->rotated;

		for (i = 0; i < rotsprite->angles; i++)
		{
			if (rotsprite->patches[i])
				Patch_Free(rotsprite->patches[i]);
		}

		Z_Free(rotsprite->patches);
		Z_Free(rotsprite);
	}
#endif

	Z_Free(ext);
}

static void Patch_FreeData(patch_t *patch)
{
#ifdef HWRENDER
	if (patch->hardware)
		HWR_FreeTexture(patch);
#endif

	Patch_FreeExtra(patch->extra);

	if (patch->columnofs)
		Z_Free(patch->columnofs);
	if (patch->columns)
		Z_Free(patch->columns);
}

void Patch_Free(patch_t *patch)
{
	Patch_FreeData(patch);
	Z_Free(patch);
}

//
// Frees patches with a tag range.
//

static boolean Patch_FreeTagsCallback(void *mem)
{
	patch_t *patch = (patch_t *)mem;
	Patch_FreeData(patch);
	return true;
}

void Patch_FreeTags(INT32 lowtag, INT32 hightag)
{
	Z_IterateTags(lowtag, hightag, Patch_FreeTagsCallback);
}

void Patch_GenerateFlat(patch_t *patch, pictureflags_t flags, pictureformat_t format)
{
	UINT8 flip = (flags & (PICFLAGS_XFLIP | PICFLAGS_YFLIP));

	if (patch->extra->flats == NULL)
		patch->extra->flats = Z_Calloc(sizeof(void *) * 4, PU_PATCH_DATA, NULL);

	if (patch->extra->flats[flip] == NULL)
		patch->extra->flats[flip] = Picture_Convert(patch->format, patch, format, 0, NULL, 0, 0, 0, 0, flags);
}

//
// Allocates a truecolor patch.
//

patch_t *Patch_GetTruecolor(patch_t *patch)
{
#ifdef PICTURES_ALLOWDEPTH
	if (!patch->extra->truecolor && patch->extra->source.data)
	{
		patch_t *tc = Picture_PNGConvert(
						(UINT8 *)patch->extra->source.data, PICFMT_PATCH32,
						NULL, NULL, NULL, NULL,
						patch->extra->source.len, NULL, 0);

		tc->format = PICFMT_PATCH32;

		patch->extra->truecolor = tc;
	}

	return patch->extra->truecolor;
#else
	return NULL;
#endif
}

//
// Allocates a hardware patch.
//

#ifdef HWRENDER
void *Patch_AllocateHardwarePatch(patch_t *patch)
{
	if (!patch->hardware)
	{
		GLPatch_t *grPatch = Z_Calloc(sizeof(GLPatch_t), PU_HWRPATCHINFO, &patch->hardware);
		grPatch->mipmap = Z_Calloc(sizeof(GLMipmap_t), PU_HWRPATCHINFO, &grPatch->mipmap);
		grPatch->picfmt = PICFMT_PATCH;
	}
	return (void *)(patch->hardware);
}

//
// Creates a hardware patch.
//

void *Patch_CreateGL(patch_t *patch)
{
	GLPatch_t *grPatch = (GLPatch_t *)Patch_AllocateHardwarePatch(patch);
	if (!grPatch->mipmap->data) // Run HWR_MakePatch in all cases, to recalculate some things
		HWR_MakePatch(patch, grPatch, grPatch->mipmap, false);
	return grPatch;
}
#endif // HWRENDER
