// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2019-2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2019-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_rotsprite.c
/// \brief Rotated patch generation.

#include "byteptr.h"
#include "dehacked.h"
#include "i_video.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_patch.h"
#include "r_rotsprite.h"
#include "r_things.h"
#include "z_zone.h"
#include "w_wad.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

#ifdef ROTSPRITE
fixed_t rollcosang[ROTANGLES];
fixed_t rollsinang[ROTANGLES];

static unsigned char imgbuf[1<<26];

/** Get a rotation angle from a roll angle.
  *
  * \param rollangle The roll angle.
  * \return The rotation angle.
  */
INT32 R_GetRollAngle(angle_t rollangle)
{
	INT32 ra = AngleFixed(rollangle)>>FRACBITS;
#if (ROTANGDIFF > 1)
	ra += (ROTANGDIFF/2);
#endif
	ra /= ROTANGDIFF;
	ra %= ROTANGLES;
	return ra;
}

/** Get rotated sprite info for a patch.
  *
  * \param wad The patch's wad number.
  * \param lump The patch's lump number.
  * \param tag Zone memory tag.
  * \param rsvars Sprite rotation variables.
  * \param store Store the rotated patch into the patch reference list.
  * \return The rotated sprite info.
  * \sa Patch_CacheRotatedPwad
  */
rotsprite_t *RotSprite_GetFromPatchNumPwad(UINT16 wad, UINT16 lump, INT32 tag, rotsprite_vars_t rsvars, boolean store)
{
	void *cache = GetRotatedPatchInfo(wad, lump, rendermode, rsvars.flip);
	if (!GetRotatedPatchInfo(wad, lump, rendermode, rsvars.flip))
	{
		rotsprite_t *ptr = Z_Calloc(sizeof(rotsprite_t), tag, &cache);
		ptr->lumpnum = WADANDLUMP(wad, lump);
		SetRotatedPatchInfo(wad, lump, rendermode, rsvars.flip, cache);
		return (void *)ptr;
	}
	else
		Z_ChangeTag(cache, tag);

	// insert into list
	if (store)
		Patch_StoreReference(wad, lump, tag, cache, rsvars.rollangle, rsvars.flip);

	return cache;
}

rotsprite_t *RotSprite_GetFromPatchNum(lumpnum_t lumpnum, INT32 tag, rotsprite_vars_t rsvars, boolean store)
{
	return RotSprite_GetFromPatchNumPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum),tag,rsvars,store);
}

rotsprite_t *RotSprite_GetFromPatchName(const char *name, INT32 tag, rotsprite_vars_t rsvars, boolean store)
{
	lumpnum_t num = W_CheckNumForName(name);
	if (num == LUMPERROR)
		return RotSprite_GetFromPatchNum(W_GetNumForName("MISSING"), tag, rsvars, store);
	return RotSprite_GetFromPatchNum(num, tag, rsvars, store);
}

rotsprite_t *RotSprite_GetFromPatchLongName(const char *name, INT32 tag, rotsprite_vars_t rsvars, boolean store)
{
	lumpnum_t num = W_CheckNumForLongName(name);
	if (num == LUMPERROR)
		return RotSprite_GetFromPatchNum(W_GetNumForLongName("MISSING"), tag, rsvars, store);
	return RotSprite_GetFromPatchNum(num, tag, rsvars, store);
}

/** Creates a rotated sprite by calculating a pixel map.
  * Caches column data and patches between levels.
  *
  * \param rotsprite The rotated sprite info.
  * \param rsvars Sprite rotation variables.
  * \sa RotSprite_CreatePatch
  */
void RotSprite_Create(rotsprite_t *rotsprite, rotsprite_vars_t rsvars)
{
	patch_t *patch;
	pixelmap_t *pixelmap = &rotsprite->pixelmap[rsvars.rollangle];
	lumpnum_t lump = rotsprite->lumpnum;

	// Sprite lump is invalid.
	if (lump == LUMPERROR)
		return;

	// Cache the patch.
	patch = (patch_t *)W_CacheSoftwarePatchNum(lump, PU_CACHE);

	// If this pixel map was not generated, do it.
	if (!rotsprite->cached[rsvars.rollangle])
	{
		RotSprite_CreatePixelMap(patch, pixelmap, rsvars);
		rotsprite->cached[rsvars.rollangle] = true;
	}
}

/** Caches columns of a rotated sprite, applying the pixel map.
  *
  * \param pixelmap The source pixel map.
  * \param cache The destination pixel map cache.
  * \param patch The source patch.
  * \param rsvars Sprite rotation variables.
  * \sa RotSprite_CreatePatch
  */
void RotSprite_CreateColumns(pixelmap_t *pixelmap, pmcache_t *cache, patch_t *patch, rotsprite_vars_t rsvars)
{
	void **colofs;
	UINT8 *data;
	boolean *colexists;
	size_t *coltbl;
	static UINT8 pixelmapcol[0xFFFF];
	size_t totalsize = 0, colsize = 0;
	INT16 width = pixelmap->width, x;

	Z_Malloc(width * sizeof(void **), PU_LEVEL, &cache->columnofs);
	Z_Malloc(width * sizeof(void *), PU_LEVEL, &cache->columnsize);

	colexists = Z_Calloc(width * sizeof(boolean), PU_STATIC, NULL);
	coltbl = Z_Calloc(width * sizeof(size_t), PU_STATIC, NULL);
	colofs = cache->columnofs;

	for (x = 0; x < width; x++)
	{
		size_t colpos = totalsize;
		colexists[x] = PixelMap_ApplyToColumn(pixelmap, &(pixelmap->map[x * pixelmap->height]), patch, pixelmapcol, &colsize, rsvars.flip);
		cache->columnsize[x] = colsize;
		totalsize += colsize;

		// copy pixels
		if (colexists[x])
		{
			data = Z_Realloc(cache->data, totalsize, PU_LEVEL, &cache->data);
			data += colpos;
			coltbl[x] = colpos;
			M_Memcpy(data, pixelmapcol, colsize);
		}
	}

	for (x = 0; x < width; x++)
	{
		if (colexists[x])
			colofs[x] = &(cache->data[coltbl[x]]);
		else
			colofs[x] = NULL;
	}

	Z_Free(colexists);
	Z_Free(coltbl);
}

/** Calculates the dimensions of a rotated rectangle.
  *
  * \param width Rectangle width.
  * \param height Rectangle height.
  * \param ca Cosine for the angle.
  * \param sa Sine for the angle.
  * \param pivot Rotation pivot.
  * \param newwidth The width for the rotated rectangle.
  * \param newheight The height for the rotated rectangle.
  * \sa RotSprite_CreatePixelMap
  */
static void CalculateRotatedRectangleDimensions(INT16 width, INT16 height, fixed_t ca, fixed_t sa, spriteframepivot_t *pivot, INT16 *newwidth, INT16 *newheight)
{
	if (pivot)
	{
		*newwidth = width + (height * 2);
		*newheight = height + (width * 2);
	}
	else
	{
		fixed_t fw = (width * FRACUNIT);
		fixed_t fh = (height * FRACUNIT);
		INT32 w1 = abs(FixedMul(fw, ca) - FixedMul(fh, sa));
		INT32 w2 = abs(FixedMul(-fw, ca) - FixedMul(fh, sa));
		INT32 h1 = abs(FixedMul(fw, sa) + FixedMul(fh, ca));
		INT32 h2 = abs(FixedMul(-fw, sa) + FixedMul(fh, ca));
		w1 = FixedInt(FixedCeil(w1 + (FRACUNIT/2)));
		w2 = FixedInt(FixedCeil(w2 + (FRACUNIT/2)));
		h1 = FixedInt(FixedCeil(h1 + (FRACUNIT/2)));
		h2 = FixedInt(FixedCeil(h2 + (FRACUNIT/2)));
		*newwidth = max(width, max(w1, w2));
		*newheight = max(height, max(h1, h2));
	}
}

/** Creates a pixel map for a rotated patch.
  *
  * \param patch The source patch.
  * \param pixelmap The destination pixel map.
  * \param rsvars Sprite rotation variables.
  * \sa RotSprite_CreatePixelMap
  */
void RotSprite_CreatePixelMap(patch_t *patch, pixelmap_t *pixelmap, rotsprite_vars_t rsvars)
{
	size_t size;
	INT32 dx, dy;
	INT16 newwidth, newheight;
	fixed_t ca = rollcosang[rsvars.rollangle];
	fixed_t sa = rollsinang[rsvars.rollangle];

	INT16 width = SHORT(patch->width);
	INT16 height = SHORT(patch->height);
	INT16 leftoffset = SHORT(patch->leftoffset);

	spriteframepivot_t pivot;
	INT16 rotxcenter, rotycenter;
	spriteframepivot_t *spritepivot = rsvars.pivot;

	pivot.x = (spritepivot ? spritepivot->x : (rsvars.sprite ? leftoffset : (width / 2)));
	pivot.y = (spritepivot ? spritepivot->y : (height / 2));

	if (rsvars.flip)
	{
		pivot.x = width - pivot.x;
		leftoffset = width - leftoffset;
	}

	// Find the dimensions of the rotated patch.
	CalculateRotatedRectangleDimensions(width, height, ca, sa, (spritepivot ? &pivot : NULL), &newwidth, &newheight);
	rotxcenter = (newwidth / 2);
	rotycenter = (newheight / 2);
	size = (newwidth * newheight);

	// Build pixel map.
	if (pixelmap->map)
		Z_Free(pixelmap->map);
	pixelmap->map = Z_Calloc(size * sizeof(INT32) * 2, PU_STATIC, NULL);
	pixelmap->size = size;
	pixelmap->width = newwidth;
	pixelmap->height = newheight;

	// Calculate the position of every pixel.
	for (dy = 0; dy < newheight; dy++)
	{
		for (dx = 0; dx < newwidth; dx++)
		{
			INT32 dst = (dx*newheight)+dy;
			INT32 x = (dx-rotxcenter) << FRACBITS;
			INT32 y = (dy-rotycenter) << FRACBITS;
			INT32 sx = FixedMul(x, ca) + FixedMul(y, sa) + (pivot.x << FRACBITS);
			INT32 sy = -FixedMul(x, sa) + FixedMul(y, ca) + (pivot.y << FRACBITS);
			sx >>= FRACBITS;
			sy >>= FRACBITS;
			pixelmap->map[dst] = sy;
			pixelmap->map[dst + size] = sx;
		}
	}

	// Set offsets.
	if (rsvars.sprite)
	{
		pixelmap->leftoffset = (newwidth / 2) + (leftoffset - pivot.x);
		pixelmap->topoffset = (newheight / 2) + (SHORT(patch->topoffset) - pivot.y);
		pixelmap->topoffset += FEETADJUST>>FRACBITS;
	}
	else
	{
		pixelmap->leftoffset = leftoffset;
		pixelmap->topoffset = SHORT(patch->topoffset);
	}
}

/** Creates a rotated patch from sprite rotation data.
  *
  * \param rotsprite The rotated sprite info.
  * \param rsvars Sprite rotation variables.
  * \return The rotated patch.
  * \sa RotSprite_CreatePixelMap
  */
patch_t *RotSprite_CreatePatch(rotsprite_t *rotsprite, rotsprite_vars_t rsvars)
{
	UINT8 *img, *imgptr = imgbuf;
	pixelmap_t *pixelmap = &rotsprite->pixelmap[rsvars.rollangle];
	patch_t *patch = (patch_t *)W_CacheSoftwarePatchNum(rotsprite->lumpnum, PU_CACHE);
	pmcache_t *pmcache = &pixelmap->cache;
	UINT32 x, width = pixelmap->width;
	UINT8 *colpointers;
	void **colofs;
	size_t size = 0;

	WRITEINT16(imgptr, (INT16)width);
	WRITEINT16(imgptr, (INT16)pixelmap->height);
	WRITEINT16(imgptr, pixelmap->leftoffset);
	WRITEINT16(imgptr, pixelmap->topoffset);

	colpointers = imgptr;
	imgptr += width*4;

	if (!pmcache->columnofs)
		RotSprite_CreateColumns(pixelmap, pmcache, patch, rsvars);
	colofs = pmcache->columnofs;

	for (x = 0; x < width; x++)
	{
		column_t *column = (column_t *)(colofs[x]);
		WRITEINT32(colpointers, imgptr - imgbuf);
		if (column)
		{
			size = pmcache->columnsize[x];
			WRITEMEM(imgptr, column, size);
		}
		else
			WRITEUINT8(imgptr, 0xFF);
	}

	size = imgptr-imgbuf;
	img = Z_Malloc(size, PU_STATIC, NULL);
	M_Memcpy(img, imgbuf, size);

#ifdef HWRENDER
	// Ugh
	if (rendermode == render_opengl)
	{
		GLPatch_t *grPatch = Z_Calloc(sizeof(GLPatch_t), PU_HWRPATCHINFO, NULL);
		grPatch->mipmap = Z_Calloc(sizeof(GLMipmap_t), PU_HWRPATCHINFO, NULL);
		grPatch->patch = (patch_t *)img;
		rotsprite->patches[rsvars.rollangle] = (patch_t *)grPatch;
		HWR_MakePatch((patch_t *)img, grPatch, grPatch->mipmap, false);
	}
	else
#endif
		rotsprite->patches[rsvars.rollangle] = (patch_t *)img;

	return rotsprite->patches[rsvars.rollangle];
}

/** Recreates sprite rotation data in memory.
  *
  * \param rotsprite The rotated sprite info.
  * \sa RotSprite_RecreateAll
  * \sa Patch_CacheRotated
  * \sa Patch_CacheRotatedForSprite
  */
void RotSprite_Recreate(rotsprite_t *rotsprite, rendermode_t rmode)
{
	INT32 ang;
	for (ang = 0; ang < ROTANGLES; ang++)
	{
		if (rotsprite->cached[ang])
		{
			pixelmap_t *pixelmap = &rotsprite->pixelmap[ang];
			pmcache_t *cache = &pixelmap->cache;

			rotsprite->cached[ang] = false;

			if (pixelmap->map)
				Z_Free(pixelmap->map);
			if (cache->columnofs)
				Z_Free(cache->columnofs);
			if (cache->columnsize)
				Z_Free(cache->columnsize);
			if (cache->data)
				Z_Free(cache->data);

			pixelmap->map = NULL;
			cache->columnofs = NULL;
			cache->columnsize = NULL;
			cache->data = NULL;

#ifdef HWRENDER
			if (rmode == render_opengl)
			{
				GLPatch_t *grPatch = (GLPatch_t *)(rotsprite->patches[ang]);
				if (grPatch && grPatch->patch)
				{
					Z_Free(grPatch->patch);
					grPatch->patch = NULL;
				}
			}
			else
#endif
			if (rotsprite->patches[ang])
				Z_Free(rotsprite->patches[ang]);
			rotsprite->patches[ang] = NULL;

			if (rotsprite->vars.sprite)
				Patch_CacheRotatedForSprite(rotsprite->lumpnum, rotsprite->tag, rotsprite->vars, false);
			else
				Patch_CacheRotated(rotsprite->lumpnum, rotsprite->tag, rotsprite->vars, false);
		}
	}
}

/** Recreates all sprite rotation data in memory.
  *
  * \sa RotSprite_Recreate
  */
void RotSprite_RecreateAll(void)
{
	// TODO: Write this in a more efficient way.
	INT32 w, l, r, f;
	for (r = render_first; r < render_last; r++)
		for (w = 0; w < numwadfiles; w++)
			for (l = 0; l < wadfiles[w]->numlumps; l++)
			{
				for (f = 0; f < 2; f++)
				{
					rotsprite_t *rotsprite = GetRotatedPatchInfo(w, l, r, (f ? true : false));
					if (rotsprite)
						RotSprite_Recreate(rotsprite, r);
				}
			}
}

#endif // ROTSPRITE
