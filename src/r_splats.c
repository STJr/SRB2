// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_splats.c
/// \brief Floor splats

#include "r_draw.h"
#include "r_fps.h"
#include "r_main.h"
#include "r_splats.h"
#include "r_bsp.h"
#include "p_local.h"
#include "p_slopes.h"
#include "w_wad.h"
#include "z_zone.h"

struct rastery_s *prastertab; // for ASM code

static struct rastery_s rastertab[MAXVIDHEIGHT];
static void prepare_rastertab(void);

// ==========================================================================
//                                                               FLOOR SPLATS
// ==========================================================================

static void R_RasterizeFloorSplat(floorsplat_t *pSplat, vector2_t *verts, vissprite_t *vis);

static void rasterize_segment_tex(INT32 x1, INT32 y1, INT32 x2, INT32 y2, INT32 tv1, INT32 tv2, INT32 tc, INT32 dir)
{
	{
		fixed_t xs, xe, count;
		fixed_t dx0, dx1;

		if (y1 == y2)
			return;

		if (y2 > y1)
		{
			count = (y2-y1)+1;

			dx0 = FixedDiv((x2-x1)<<FRACBITS, count<<FRACBITS);
			dx1 = FixedDiv((tv2-tv1)<<FRACBITS, count<<FRACBITS);

			xs = x1 << FRACBITS;
			xe = tv1 << FRACBITS;
			tc <<= FRACBITS;

			if (dir == 0)
			{
				for (;;)
				{
					rastertab[y1].maxx = xs;
					rastertab[y1].tx2 = xe;
					rastertab[y1].ty2 = tc;

					xs += dx0;
					xe += dx1;
					y1++;

					if (count-- < 1) break;
				}
			}
			else
			{
				for (;;)
				{
					rastertab[y1].maxx = xs;
					rastertab[y1].tx2 = tc;
					rastertab[y1].ty2 = xe;

					xs += dx0;
					xe += dx1;
					y1++;

					if (count-- < 1) break;
				}
			}
		}
		else
		{
			count = (y1-y2)+1;

			dx0 = FixedDiv((x1-x2)<<FRACBITS, count<<FRACBITS);
			dx1 = FixedDiv((tv1-tv2)<<FRACBITS, count<<FRACBITS);

			xs = x2 << FRACBITS;
			xe = tv2 << FRACBITS;
			tc <<= FRACBITS;

			if (dir == 0)
			{
				for (;;)
				{
					rastertab[y2].minx = xs;
					rastertab[y2].tx1 = xe;
					rastertab[y2].ty1 = tc;

					xs += dx0;
					xe += dx1;
					y2++;

					if (count-- < 1) break;
				}
			}
			else
			{
				for (;;)
				{
					rastertab[y2].minx = xs;
					rastertab[y2].tx1 = tc;
					rastertab[y2].ty1 = xe;

					xs += dx0;
					xe += dx1;
					y2++;

					if (count-- < 1) break;
				}
			}
		}
	}
}

void R_DrawFloorSplat(vissprite_t *spr)
{
	floorsplat_t splat;
	mobj_t *mobj = spr->mobj;
	fixed_t tr_x, tr_y, rot_x, rot_y, rot_z;

	vector3_t *v3d;
	vector2_t v2d[4];
	vector2_t rotated[4];

	fixed_t x, y;
	fixed_t w, h;
	angle_t angle, splatangle;
	fixed_t ca, sa;
	fixed_t xscale, yscale;
	fixed_t xoffset, yoffset;
	fixed_t leftoffset, topoffset;
	INT32 i;

	boolean hflip = (spr->xiscale < 0);
	boolean vflip = (spr->cut & SC_VFLIP);
	UINT8 flipflags = 0;

	renderflags_t renderflags = spr->renderflags;

	if (hflip)
		flipflags |= PICFLAGS_XFLIP;
	if (vflip)
		flipflags |= PICFLAGS_YFLIP;

	if (!mobj || P_MobjWasRemoved(mobj))
		return;

	Patch_GenerateFlat(spr->patch, flipflags);
	splat.pic = spr->patch->flats[flipflags];
	if (splat.pic == NULL)
		return;

	splat.mobj = mobj;
	splat.width = spr->patch->width;
	splat.height = spr->patch->height;
	splat.scale = mobj->scale;

	if (mobj->skin && ((skin_t *)mobj->skin)->flags & SF_HIRES)
		splat.scale = FixedMul(splat.scale, ((skin_t *)mobj->skin)->highresscale);

	if (spr->rotateflags & SRF_3D || renderflags & RF_NOSPLATBILLBOARD)
		splatangle = mobj->angle;
	else
		splatangle = spr->viewpoint.angle;

	if (!(spr->cut & SC_ISROTATED))
		splatangle += mobj->spriteroll;

	splat.angle = -splatangle;
	splat.angle += ANGLE_90;

	topoffset = spr->spriteyoffset;
	leftoffset = spr->spritexoffset;
	if (hflip)
		leftoffset = ((splat.width * FRACUNIT) - leftoffset);

	xscale = spr->spritexscale;
	yscale = spr->spriteyscale;

	splat.xscale = FixedMul(splat.scale, xscale);
	splat.yscale = FixedMul(splat.scale, yscale);

	xoffset = FixedMul(leftoffset, splat.xscale);
	yoffset = FixedMul(topoffset, splat.yscale);

	x = mobj->x;
	y = mobj->y;
	w = (splat.width * splat.xscale);
	h = (splat.height * splat.yscale);

	splat.x = x;
	splat.y = y;
	splat.z = spr->pz;
	splat.slope = NULL;

	// Set positions

	// 3--2
	// |  |
	// 0--1

	splat.verts[0].x = w - xoffset;
	splat.verts[0].y = yoffset;

	splat.verts[1].x = -xoffset;
	splat.verts[1].y = yoffset;

	splat.verts[2].x = -xoffset;
	splat.verts[2].y = -h + yoffset;

	splat.verts[3].x = w - xoffset;
	splat.verts[3].y = -h + yoffset;

	angle = -splat.angle>>ANGLETOFINESHIFT;
	ca = FINECOSINE(angle);
	sa = FINESINE(angle);

	// Rotate
	for (i = 0; i < 4; i++)
	{
		rotated[i].x = FixedMul(splat.verts[i].x, ca) - FixedMul(splat.verts[i].y, sa);
		rotated[i].y = FixedMul(splat.verts[i].x, sa) + FixedMul(splat.verts[i].y, ca);
	}

	if (renderflags & (RF_SLOPESPLAT | RF_OBJECTSLOPESPLAT))
	{
		pslope_t *standingslope = mobj->standingslope; // The slope that the object is standing on.

		// The slope that was defined for the sprite.
		if (renderflags & RF_SLOPESPLAT)
			splat.slope = mobj->floorspriteslope;

		if (standingslope && (renderflags & RF_OBJECTSLOPESPLAT))
			splat.slope = standingslope;
	}

	// Translate
	for (i = 0; i < 4; i++)
	{
		tr_x = rotated[i].x + mobj->x;
		tr_y = rotated[i].y + mobj->y;

		if (splat.slope)
		{
			rot_z = P_GetSlopeZAt(splat.slope, tr_x, tr_y);
			splat.verts[i].z = rot_z;
		}
		else
			splat.verts[i].z = splat.z;

		splat.verts[i].x = tr_x;
		splat.verts[i].y = tr_y;
	}

	angle = spr->viewpoint.angle >> ANGLETOFINESHIFT;
	ca = FINECOSINE(angle);
	sa = FINESINE(angle);

	// Project
	for (i = 0; i < 4; i++)
	{
		v3d = &splat.verts[i];

		// transform the origin point
		tr_x = v3d->x - spr->viewpoint.x;
		tr_y = v3d->y - spr->viewpoint.y;

		// rotation around vertical y axis
		rot_x = FixedMul(tr_x - (mobj->x - x), sa) - FixedMul(tr_y - (mobj->y - y), ca);
		rot_y = FixedMul(tr_x - (mobj->x - x), ca) + FixedMul(tr_y - (mobj->y - y), sa);
		rot_z = v3d->z - spr->viewpoint.z;

		if (rot_y < FRACUNIT)
			return;

		// note: y from view above of map, is distance far away
		xscale = FixedDiv(projection, rot_y);
		yscale = -FixedDiv(projectiony, rot_y);

		// projection
		v2d[i].x = (centerxfrac + FixedMul(rot_x, xscale))>>FRACBITS;
		v2d[i].y = (centeryfrac + FixedMul(rot_z, yscale))>>FRACBITS;
	}

	R_RasterizeFloorSplat(&splat, v2d, spr);
}

// --------------------------------------------------------------------------
// Rasterize the four edges of a floor splat polygon,
// fill the polygon with linear interpolation, call span drawer for each
// scan line
// --------------------------------------------------------------------------
static void R_RasterizeFloorSplat(floorsplat_t *pSplat, vector2_t *verts, vissprite_t *vis)
{
	// rasterizing
	INT32 miny = viewheight + 1, maxy = 0;
	INT32 y, x1, ry1, x2, y2, i;
	fixed_t offsetx = 0, offsety = 0;
	fixed_t planeheight = 0;
	fixed_t step;

	int spanfunctype;

#define RASTERPARAMS(vnum1, vnum2, tv1, tv2, tc, dir) \
    x1 = verts[vnum1].x; \
    ry1 = verts[vnum1].y; \
    x2 = verts[vnum2].x; \
    y2 = verts[vnum2].y; \
    if (y2 > ry1) \
        step = FixedDiv(x2-x1, y2-ry1+1); \
    else if (y2 == ry1) \
        step = 0; \
    else \
        step = FixedDiv(x2-x1, ry1-y2+1); \
    if (ry1 < 0) { \
        if (step) { \
            x1 <<= FRACBITS; \
            x1 += (-ry1)*step; \
            x1 >>= FRACBITS; \
        } \
        ry1 = 0; \
    } \
    if (ry1 >= vid.height) { \
        if (step) { \
            x1 <<= FRACBITS; \
            x1 -= (vid.height-1-ry1)*step; \
            x1 >>= FRACBITS; \
        } \
        ry1 = vid.height - 1; \
    } \
    if (y2 < 0) { \
        if (step) { \
            x2 <<= FRACBITS; \
            x2 -= (-y2)*step; \
            x2 >>= FRACBITS; \
        } \
        y2 = 0; \
    } \
    if (y2 >= vid.height) { \
        if (step) { \
            x2 <<= FRACBITS; \
            x2 += (vid.height-1-y2)*step; \
            x2 >>= FRACBITS; \
        } \
        y2 = vid.height - 1; \
    } \
    rasterize_segment_tex(x1, ry1, x2, y2, tv1, tv2, tc, dir); \
    if (ry1 < miny) \
        miny = ry1; \
    if (ry1 > maxy) \
        maxy = ry1;

	ds_source = (UINT8 *)pSplat->pic;
	ds_flatwidth = pSplat->width;
	ds_flatheight = pSplat->height;

	ds_powersoftwo = ds_solidcolor = ds_fog = false;

	if (R_CheckSolidColorFlat())
		ds_solidcolor = true;
	else if (R_CheckPowersOfTwo())
	{
		R_SetFlatVars(ds_flatwidth * ds_flatheight);
		ds_powersoftwo = true;
	}

	if (pSplat->slope)
	{
		R_SetScaledSlopePlane(pSplat->slope, vis->viewpoint.x, vis->viewpoint.y, vis->viewpoint.z, pSplat->xscale, pSplat->yscale, -pSplat->verts[0].x, pSplat->verts[0].y, vis->viewpoint.angle, pSplat->angle);
	}
	else if (!ds_solidcolor)
	{
		planeheight = abs(pSplat->z - vis->viewpoint.z);

		if (pSplat->angle)
		{
			// Add the view offset, rotated by the plane angle.
			fixed_t a = -pSplat->verts[0].x + vis->viewpoint.x;
			fixed_t b = -pSplat->verts[0].y + vis->viewpoint.y;
			angle_t angle = (pSplat->angle >> ANGLETOFINESHIFT);
			offsetx = FixedMul(a, FINECOSINE(angle)) - FixedMul(b, FINESINE(angle));
			offsety = -FixedMul(a, FINESINE(angle)) - FixedMul(b, FINECOSINE(angle));
		}
		else
		{
			offsetx = vis->viewpoint.x - pSplat->verts[0].x;
			offsety = pSplat->verts[0].y - vis->viewpoint.y;
		}
	}

	ds_colormap = vis->colormap;
	ds_translation = R_GetTranslationForThing(vis->mobj, vis->color, vis->translation);
	if (ds_translation == NULL)
		ds_translation = colormaps;

	if (vis->extra_colormap)
	{
		if (!ds_colormap)
			ds_colormap = vis->extra_colormap->colormap;
		else
			ds_colormap = &vis->extra_colormap->colormap[ds_colormap - colormaps];
	}

	ds_transmap = vis->transmap;

	// Determine which R_DrawWhatever to use

	// Solid color
	if (ds_solidcolor)
	{
		UINT16 px = *(UINT16 *)ds_source;

		// Uh, it's not visible.
		if (!(px & 0xFF00))
			return;

		// Pixel color is contained in the lower 8 bits (upper 8 are the opacity), so advance the pointer
		ds_source++;

		if (pSplat->slope)
		{
			if (ds_transmap)
				spanfunctype = SPANDRAWFUNC_TILTEDTRANSSOLID;
			else
				spanfunctype = SPANDRAWFUNC_TILTEDSOLID;
		}
		else
		{
			if (ds_transmap)
				spanfunctype = SPANDRAWFUNC_TRANSSOLID;
			else
				spanfunctype = SPANDRAWFUNC_SOLID;
		}
	}
	// Transparent
	else if (ds_transmap)
	{
		if (pSplat->slope)
			spanfunctype = SPANDRAWFUNC_TILTEDTRANSSPRITE;
		else
			spanfunctype = SPANDRAWFUNC_TRANSSPRITE;
	}
	// Opaque
	else
	{
		if (pSplat->slope)
			spanfunctype = SPANDRAWFUNC_TILTEDSPRITE;
		else
			spanfunctype = SPANDRAWFUNC_SPRITE;
	}

	if (ds_powersoftwo || ds_solidcolor)
		spanfunc = spanfuncs[spanfunctype];
	else
		spanfunc = spanfuncs_npo2[spanfunctype];

	prepare_rastertab();

	// do segment a -> top of texture
	RASTERPARAMS(3,2,0,pSplat->width-1,0,0);
	// do segment b -> right side of texture
	RASTERPARAMS(2,1,0,pSplat->width-1,pSplat->height-1,0);
	// do segment c -> bottom of texture
	RASTERPARAMS(1,0,pSplat->width-1,0,pSplat->height-1,0);
	// do segment d -> left side of texture
	RASTERPARAMS(0,3,pSplat->width-1,0,0,1);

	if (maxy >= vid.height)
		maxy = vid.height-1;

	for (y = miny; y <= maxy; y++)
	{
		boolean cliptab[MAXVIDWIDTH+1];

		x1 = rastertab[y].minx>>FRACBITS;
		x2 = rastertab[y].maxx>>FRACBITS;

		if (x1 > x2)
		{
			INT32 swap = x1;
			x1 = x2;
			x2 = swap;
		}

		if (x1 == INT16_MIN || x2 == INT16_MAX)
			continue;

		if (x1 < 0)
			x1 = 0;
		if (x2 >= viewwidth)
			x2 = viewwidth - 1;

		if (x1 >= viewwidth || x2 < 0)
			continue;

		for (i = x1; i <= x2; i++)
			cliptab[i] = (y >= mfloorclip[i] || y <= mceilingclip[i]);

		// clip left
		while (cliptab[x1])
		{
			x1++;
			if (x1 >= viewwidth)
				break;
		}

		// clip right
		i = x2;

		while (i > x1)
		{
			if (cliptab[i])
				x2 = i-1;
			i--;
			if (i < 0)
				break;
		}

		if (x2 < x1)
			continue;

		if (!ds_solidcolor && !pSplat->slope)
		{
			fixed_t xstep, ystep;
			fixed_t distance, span;

			angle_t angle = (vis->viewpoint.angle + pSplat->angle)>>ANGLETOFINESHIFT;
			angle_t planecos = FINECOSINE(angle);
			angle_t planesin = FINESINE(angle);

			// [RH] Notice that I dumped the caching scheme used by Doom.
			// It did not offer any appreciable speedup.
			distance = FixedMul(planeheight, yslope[y]);
			span = abs(centery - y);

			if (span) // Don't divide by zero
			{
				xstep = FixedMul(planesin, planeheight) / span;
				ystep = FixedMul(planecos, planeheight) / span;
			}
			else
				xstep = ystep = FRACUNIT;

			ds_xstep = FixedDiv(xstep, pSplat->xscale);
			ds_ystep = FixedDiv(ystep, pSplat->yscale);

			ds_xfrac = FixedDiv(offsetx + FixedMul(planecos, distance) + (x1 - centerx) * xstep, pSplat->xscale);
			ds_yfrac = FixedDiv(offsety - FixedMul(planesin, distance) + (x1 - centerx) * ystep, pSplat->yscale);
		}

		ds_y = y;
		ds_x1 = x1;
		ds_x2 = x2;
		spanfunc();

		rastertab[y].minx = INT32_MAX;
		rastertab[y].maxx = INT32_MIN;
	}
}

static void prepare_rastertab(void)
{
	INT32 i;
	prastertab = rastertab;
	for (i = 0; i < vid.height; i++)
	{
		rastertab[i].minx = INT32_MAX;
		rastertab[i].maxx = INT32_MIN;
	}
}
