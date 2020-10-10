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

patch_t *Patch_GetRotatedSprite(spriteframe_t *sprite, size_t frame, size_t spriteangle, boolean flip, void *info, INT32 rotationangle)
{
	rotsprite_t *rotsprite = sprite->rotated[spriteangle];
	spriteinfo_t *sprinfo = (spriteinfo_t *)info;
	INT32 idx = rotationangle;

	if (rotationangle < 1 || rotationangle >= ROTANGLES)
		return NULL;

	if (rotsprite == NULL)
	{
		rotsprite = RotatedPatch_Create(ROTANGLES);
		sprite->rotated[spriteangle] = rotsprite;
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

void RotatedPatch_DoRotation(rotsprite_t *rotsprite, patch_t *patch, INT32 angle, INT32 xpivot, INT32 ypivot, boolean flip)
{
	patch_t *rotated;
	UINT16 *rawdst;
	size_t size;
	pictureflags_t bflip = (flip) ? PICFLAGS_XFLIP : 0;

	INT32 width = patch->width;
	INT32 height = patch->height;
	INT32 leftoffset = patch->leftoffset;
	INT32 newwidth, newheight;

	INT32 dx, dy;
	fixed_t ca = rollcosang[angle];
	fixed_t sa = rollsinang[angle];
	INT32 idx = angle;

	// Don't cache angle = 0
	if (angle < 1 || angle >= ROTANGLES)
		return;

#define ROTSPRITE_XCENTER (newwidth / 2)
#define ROTSPRITE_YCENTER (newheight / 2)

	if (flip)
	{
		idx += rotsprite->angles;
		xpivot = width - xpivot;
		leftoffset = width - leftoffset;
	}

	if (rotsprite->patches[idx])
		return;

	// Find the dimensions of the rotated patch.
	{
		INT32 w1 = abs(FixedMul(width << FRACBITS, ca) - FixedMul(height << FRACBITS, sa));
		INT32 w2 = abs(FixedMul(-(width << FRACBITS), ca) - FixedMul(height << FRACBITS, sa));
		INT32 h1 = abs(FixedMul(width << FRACBITS, sa) + FixedMul(height << FRACBITS, ca));
		INT32 h2 = abs(FixedMul(-(width << FRACBITS), sa) + FixedMul(height << FRACBITS, ca));
		w1 = FixedInt(FixedCeil(w1 + (FRACUNIT/2)));
		w2 = FixedInt(FixedCeil(w2 + (FRACUNIT/2)));
		h1 = FixedInt(FixedCeil(h1 + (FRACUNIT/2)));
		h2 = FixedInt(FixedCeil(h2 + (FRACUNIT/2)));
		newwidth = max(width, max(w1, w2));
		newheight = max(height, max(h1, h2));
	}

	// check boundaries
	{
		fixed_t top[2][2];
		fixed_t bottom[2][2];

		top[0][0] = FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, sa) + (xpivot << FRACBITS);
		top[0][1] = FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, ca) + (ypivot << FRACBITS);
		top[1][0] = FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, sa) + (xpivot << FRACBITS);
		top[1][1] = FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, ca) + (ypivot << FRACBITS);

		bottom[0][0] = FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, sa) + (xpivot << FRACBITS);
		bottom[0][1] = -FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, ca) + (ypivot << FRACBITS);
		bottom[1][0] = FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, sa) + (xpivot << FRACBITS);
		bottom[1][1] = -FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, ca) + (ypivot << FRACBITS);

		top[0][0] >>= FRACBITS;
		top[0][1] >>= FRACBITS;
		top[1][0] >>= FRACBITS;
		top[1][1] >>= FRACBITS;

		bottom[0][0] >>= FRACBITS;
		bottom[0][1] >>= FRACBITS;
		bottom[1][0] >>= FRACBITS;
		bottom[1][1] >>= FRACBITS;

#define BOUNDARYWCHECK(b) (b[0] < 0 || b[0] >= width)
#define BOUNDARYHCHECK(b) (b[1] < 0 || b[1] >= height)
#define BOUNDARYADJUST(x) x *= 2
		// top left/right
		if (BOUNDARYWCHECK(top[0]) || BOUNDARYWCHECK(top[1]))
			BOUNDARYADJUST(newwidth);
		// bottom left/right
		else if (BOUNDARYWCHECK(bottom[0]) || BOUNDARYWCHECK(bottom[1]))
			BOUNDARYADJUST(newwidth);
		// top left/right
		if (BOUNDARYHCHECK(top[0]) || BOUNDARYHCHECK(top[1]))
			BOUNDARYADJUST(newheight);
		// bottom left/right
		else if (BOUNDARYHCHECK(bottom[0]) || BOUNDARYHCHECK(bottom[1]))
			BOUNDARYADJUST(newheight);
#undef BOUNDARYWCHECK
#undef BOUNDARYHCHECK
#undef BOUNDARYADJUST
	}

	// Draw the rotated sprite to a temporary buffer.
	size = (newwidth * newheight);
	if (!size)
		size = (width * height);
	rawdst = Z_Calloc(size * sizeof(UINT16), PU_STATIC, NULL);

	for (dy = 0; dy < newheight; dy++)
	{
		for (dx = 0; dx < newwidth; dx++)
		{
			INT32 x = (dx-ROTSPRITE_XCENTER) << FRACBITS;
			INT32 y = (dy-ROTSPRITE_YCENTER) << FRACBITS;
			INT32 sx = FixedMul(x, ca) + FixedMul(y, sa) + (xpivot << FRACBITS);
			INT32 sy = -FixedMul(x, sa) + FixedMul(y, ca) + (ypivot << FRACBITS);
			sx >>= FRACBITS;
			sy >>= FRACBITS;
			if (sx >= 0 && sy >= 0 && sx < width && sy < height)
			{
				void *input = Picture_GetPatchPixel(patch, PICFMT_PATCH, sx, sy, bflip);
				if (input != NULL)
					rawdst[(dy*newwidth)+dx] = (0xFF00 | (*(UINT8 *)input));
			}
		}
	}

	// make patch
	rotated = (patch_t *)Picture_Convert(PICFMT_FLAT16, rawdst, PICFMT_PATCH, 0, &size, newwidth, newheight, 0, 0, 0);

	Z_ChangeTag(rotated, PU_PATCH_ROTATED);
	Z_SetUser(rotated, (void **)(&rotsprite->patches[idx]));

	rotated->leftoffset = (rotated->width / 2) + (leftoffset - xpivot);
	rotated->topoffset = (rotated->height / 2) + (patch->topoffset - ypivot);

	//BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
	rotated->topoffset += FEETADJUST>>FRACBITS;

	// free rotated image data
	Z_Free(rawdst);

#undef ROTSPRITE_XCENTER
#undef ROTSPRITE_YCENTER
}
#endif
