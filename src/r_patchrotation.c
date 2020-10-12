// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patchrotation.c
/// \brief Patch rotation.

#include "r_patchrotation.h"
#include "r_things.h" // FEETADJUST
#include "z_zone.h"
#include "w_wad.h"

#ifdef ROTSPRITE
fixed_t rollcosang[ROTANGLES];
fixed_t rollsinang[ROTANGLES];

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

patch_t *Patch_GetRotated(patch_t *patch, INT32 angle, boolean flip)
{
	rotsprite_t *rotsprite = patch->rotated;
	if (rotsprite == NULL || angle < 1 || angle >= ROTANGLES)
		return NULL;

	if (flip)
		angle += rotsprite->angles;

	return rotsprite->patches[angle];
}

patch_t *Patch_GetRotatedSprite(
	spriteframe_t *sprite,
	size_t frame, size_t spriteangle,
	boolean flip, boolean adjustfeet,
	void *info, INT32 rotationangle)
{
	rotsprite_t *rotsprite;
	spriteinfo_t *sprinfo = (spriteinfo_t *)info;
	INT32 idx = rotationangle;
	UINT8 type = (adjustfeet ? 1 : 0);

	if (rotationangle < 1 || rotationangle >= ROTANGLES)
		return NULL;

	rotsprite = sprite->rotated[type][spriteangle];

	if (rotsprite == NULL)
	{
		rotsprite = RotatedPatch_Create(ROTANGLES);
		sprite->rotated[type][spriteangle] = rotsprite;
	}

	if (flip)
		idx += rotsprite->angles;

	if (rotsprite->patches[idx] == NULL)
	{
		patch_t *patch;
		INT32 xpivot = 0, ypivot = 0;
		lumpnum_t lump = sprite->lumppat[spriteangle];

		if (lump == LUMPERROR)
			return NULL;

		patch = W_CachePatchNum(lump, PU_SPRITE);

		if (sprinfo->available)
		{
			xpivot = sprinfo->pivot[frame].x;
			ypivot = sprinfo->pivot[frame].y;
		}
		else
		{
			xpivot = patch->leftoffset;
			ypivot = patch->height / 2;
		}

		RotatedPatch_DoRotation(rotsprite, patch, rotationangle, xpivot, ypivot, flip);

		//BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
		if (adjustfeet)
			((patch_t *)rotsprite->patches[idx])->topoffset += FEETADJUST>>FRACBITS;
	}

	return rotsprite->patches[idx];
}

void Patch_Rotate(patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip)
{
	if (patch->rotated == NULL)
		patch->rotated = RotatedPatch_Create(ROTANGLES);
	RotatedPatch_DoRotation(patch->rotated, patch, angle, xpivot, ypivot, flip);
}

rotsprite_t *RotatedPatch_Create(INT32 numangles)
{
	rotsprite_t *rotsprite = Z_Calloc(sizeof(rotsprite_t), PU_STATIC, NULL);
	rotsprite->angles = numangles;
	rotsprite->patches = Z_Calloc(rotsprite->angles * 2 * sizeof(void *), PU_STATIC, NULL);
	return rotsprite;
}

static void RotatedPatch_CalculateDimensions(
	INT32 width, INT32 height,
	fixed_t ca, fixed_t sa,
	INT32 *newwidth, INT32 *newheight)
{
	fixed_t fixedwidth = (width * FRACUNIT);
	fixed_t fixedheight = (height * FRACUNIT);

	fixed_t w1 = abs(FixedMul(fixedwidth, ca) - FixedMul(fixedheight, sa));
	fixed_t w2 = abs(FixedMul(-fixedwidth, ca) - FixedMul(fixedheight, sa));
	fixed_t h1 = abs(FixedMul(fixedwidth, sa) + FixedMul(fixedheight, ca));
	fixed_t h2 = abs(FixedMul(-fixedwidth, sa) + FixedMul(fixedheight, ca));

	w1 = FixedInt(FixedCeil(w1 + (FRACUNIT/2)));
	w2 = FixedInt(FixedCeil(w2 + (FRACUNIT/2)));
	h1 = FixedInt(FixedCeil(h1 + (FRACUNIT/2)));
	h2 = FixedInt(FixedCeil(h2 + (FRACUNIT/2)));

	*newwidth = max(width, max(w1, w2));
	*newheight = max(height, max(h1, h2));
}

void RotatedPatch_DoRotation(rotsprite_t *rotsprite, patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip)
{
	patch_t *rotated;
	UINT16 *rawdst, *rawconv;
	size_t size;
	pictureflags_t bflip = (flip) ? PICFLAGS_XFLIP : 0;

	INT32 width = patch->width;
	INT32 height = patch->height;
	INT32 leftoffset = patch->leftoffset;
	INT32 newwidth, newheight;

	fixed_t ca = rollcosang[angle];
	fixed_t sa = rollsinang[angle];
	fixed_t xcenter, ycenter;
	INT32 idx = angle;
	INT32 x, y;
	INT32 sx, sy;
	INT32 dx, dy;
	INT32 ox, oy;
	INT32 minx, miny, maxx, maxy;

	// Don't cache angle = 0
	if (angle < 1 || angle >= ROTANGLES)
		return;

	if (flip)
	{
		idx += rotsprite->angles;
		xpivot = width - xpivot;
		leftoffset = width - leftoffset;
	}

	if (rotsprite->patches[idx])
		return;

	// Find the dimensions of the rotated patch.
	RotatedPatch_CalculateDimensions(width, height, ca, sa, &newwidth, &newheight);

	xcenter = (xpivot * FRACUNIT);
	ycenter = (ypivot * FRACUNIT);

	if (xpivot != width / 2 || ypivot != height / 2)
	{
		newwidth *= 2;
		newheight *= 2;
	}

	minx = newwidth;
	miny = newheight;
	maxx = 0;
	maxy = 0;

	// Draw the rotated sprite to a temporary buffer.
	size = (newwidth * newheight);
	if (!size)
		size = (width * height);
	rawdst = Z_Calloc(size * sizeof(UINT16), PU_STATIC, NULL);

	for (dy = 0; dy < newheight; dy++)
	{
		for (dx = 0; dx < newwidth; dx++)
		{
			x = (dx - (newwidth / 2)) * FRACUNIT;
			y = (dy - (newheight / 2)) * FRACUNIT;
			sx = FixedMul(x, ca) + FixedMul(y, sa) + xcenter;
			sy = -FixedMul(x, sa) + FixedMul(y, ca) + ycenter;

			sx >>= FRACBITS;
			sy >>= FRACBITS;

			if (sx >= 0 && sy >= 0 && sx < width && sy < height)
			{
				void *input = Picture_GetPatchPixel(patch, PICFMT_PATCH, sx, sy, bflip);
				if (input != NULL)
				{
					rawdst[(dy * newwidth) + dx] = (0xFF00 | (*(UINT8 *)input));
					if (dx < minx)
						minx = dx;
					if (dy < miny)
						miny = dy;
					if (dx > maxx)
						maxx = dx;
					if (dy > maxy)
						maxy = dy;
				}
			}
		}
	}

	ox = (newwidth / 2) + (leftoffset - xpivot);
	oy = (newheight / 2) + (patch->topoffset - ypivot);
	width = (maxx - minx);
	height = (maxy - miny);

	if ((unsigned)(width * height) != size)
	{
		UINT16 *src, *dest;

		size = (width * height);
		rawconv = Z_Calloc(size * sizeof(UINT16), PU_STATIC, NULL);

		src = &rawdst[(miny * newwidth) + minx];
		dest = rawconv;
		dy = height;

		while (dy--)
		{
			M_Memcpy(dest, src, width * sizeof(UINT16));
			dest += width;
			src += newwidth;
		}

		ox -= minx;
		oy -= miny;

		Z_Free(rawdst);
	}
	else
	{
		rawconv = rawdst;
		width = newwidth;
		height = newheight;
	}

	// make patch
	rotated = (patch_t *)Picture_Convert(PICFMT_FLAT16, rawconv, PICFMT_PATCH, 0, NULL, width, height, 0, 0, 0);

	Z_ChangeTag(rotated, PU_PATCH_ROTATED);
	Z_SetUser(rotated, (void **)(&rotsprite->patches[idx]));
	Z_Free(rawconv);

	rotated->leftoffset = ox;
	rotated->topoffset = oy;
}
#endif
