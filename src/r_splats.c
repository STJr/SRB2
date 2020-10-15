// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_splats.c
/// \brief Floor splats

#include "r_draw.h"
#include "r_main.h"
#include "r_splats.h"
#include "r_bsp.h"
#include "p_slopes.h"
#include "w_wad.h"
#include "z_zone.h"

struct rastery_s *prastertab; // for ASM code

#ifdef FLOORSPLATS
static struct rastery_s rastertab[MAXVIDHEIGHT];
static void prepare_rastertab(void);

// ==========================================================================
//                                                               FLOOR SPLATS
// ==========================================================================

#ifdef USEASM
void ASMCALL rasterize_segment_tex_asm(INT32 x1, INT32 y1, INT32 x2, INT32 y2, INT32 tv1, INT32 tv2, INT32 tc, INT32 dir);
#endif

// Lactozilla
static void rasterize_segment_tex(INT32 x1, INT32 y1, INT32 x2, INT32 y2, INT32 tv1, INT32 tv2, INT32 tc, INT32 dir)
{
#ifdef USEASM
	if (R_ASM)
	{
		rasterize_segment_tex_asm(x1, y1, x2, y2, tv1, tv2, tc, dir);
		return;
	}
	else
#endif
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

// --------------------------------------------------------------------------
// Rasterize the four edges of a floor splat polygon,
// fill the polygon with linear interpolation, call span drawer for each
// scan line
// --------------------------------------------------------------------------
void R_RenderFloorSplat(floorsplat_t *pSplat, vector2_t *verts, vissprite_t *vis)
{
	// rasterizing
	INT32 miny = viewheight + 1, maxy = 0;
	INT32 y, x1, ry1, x2, y2, i;
	fixed_t offsetx = 0, offsety = 0;
	fixed_t step;

	fixed_t planeheight;
	fixed_t xstep, ystep;
	angle_t angle, planecos, planesin;
	fixed_t distance, span;

	int spanfunctype = SPANDRAWFUNC_SPRITE;

	prepare_rastertab();

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

	// do segment a -> top of texture
	RASTERPARAMS(3,2,0,pSplat->width-1,0,0);
	// do segment b -> right side of texture
	RASTERPARAMS(2,1,0,pSplat->width-1,pSplat->height-1,0);
	// do segment c -> bottom of texture
	RASTERPARAMS(1,0,pSplat->width-1,0,pSplat->height-1,0);
	// do segment d -> left side of texture
	RASTERPARAMS(0,3,pSplat->width-1,0,0,1);

	ds_source = (UINT8 *)pSplat->pic;
	ds_flatwidth = pSplat->width;
	ds_flatheight = pSplat->height;

	if (R_CheckPowersOfTwo())
		R_CheckFlatLength(ds_flatwidth * ds_flatheight);

	// Lactozilla: I don't know what I'm doing
	if (pSplat->tilted)
	{
		ds_sup = &ds_su[0];
		ds_svp = &ds_sv[0];
		ds_szp = &ds_sz[0];
		R_CalculateSlopeVectors(&pSplat->slope, viewx, viewy, viewz, pSplat->xscale, pSplat->yscale, -pSplat->verts[0].x, pSplat->verts[0].y, viewangle, pSplat->angle, 1.0f);
		spanfunctype = SPANDRAWFUNC_TILTEDSPRITE;
	}
	else
	{
		if (pSplat->angle)
		{
			// Add the view offset, rotated by the plane angle.
			fixed_t a = -pSplat->verts[0].x + viewx;
			fixed_t b = -pSplat->verts[0].y + viewy;
			angle = (pSplat->angle >> ANGLETOFINESHIFT);
			offsetx = FixedMul(a, FINECOSINE(angle)) - FixedMul(b,FINESINE(angle));
			offsety = -FixedMul(a, FINESINE(angle)) - FixedMul(b,FINECOSINE(angle));
		}
		else
		{
			offsetx = viewx - pSplat->verts[0].x;
			offsety = pSplat->verts[0].y - viewy;
		}
	}

	ds_transmap = NULL;

	if (vis->transmap)
	{
		ds_transmap = vis->transmap;

		if (pSplat->tilted)
			spanfunctype = SPANDRAWFUNC_TILTEDTRANSSPRITE;
		else
			spanfunctype = SPANDRAWFUNC_TRANSSPRITE;
	}

	if (ds_powersoftwo)
		spanfunc = spanfuncs[spanfunctype];
	else
		spanfunc = spanfuncs_npo2[spanfunctype];

	if (pSplat->angle && !pSplat->tilted)
	{
		memset(cachedheight, 0, sizeof(cachedheight));
		angle = (viewangle + pSplat->angle - ANGLE_90) >> ANGLETOFINESHIFT;
		basexscale = FixedDiv(FINECOSINE(angle), centerxfrac);
		baseyscale = -FixedDiv(FINESINE(angle), centerxfrac);
	}
	else
	{
		angle = (viewangle - ANGLE_90) >> ANGLETOFINESHIFT;
		basexscale = FixedDiv(FINECOSINE(angle), centerxfrac);
		baseyscale = -FixedDiv(FINESINE(angle), centerxfrac);
	}

	planeheight = abs(pSplat->z - viewz);
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
			cliptab[i] = (y >= mfloorclip[i]);

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

		ds_colormap = vis->colormap;
		ds_translation = R_GetSpriteTranslation(vis);
		if (ds_translation == NULL)
			ds_translation = colormaps;

		if (vis->extra_colormap)
		{
			if (!ds_colormap)
				ds_colormap = vis->extra_colormap->colormap;
			else
				ds_colormap = &vis->extra_colormap->colormap[ds_colormap - colormaps];
		}

		if (!pSplat->tilted)
		{
			angle = (viewangle + pSplat->angle)>>ANGLETOFINESHIFT;
			planecos = FINECOSINE(angle);
			planesin = FINESINE(angle);

			if (planeheight != cachedheight[y])
			{
				cachedheight[y] = planeheight;
				distance = cacheddistance[y] = FixedMul(planeheight, yslope[y]);
				xstep = cachedxstep[y] = FixedMul(distance, basexscale);
				ystep = cachedystep[y] = FixedMul(distance, baseyscale);

				// don't divide by zero
				if ((span = abs(centery-y)))
				{
					xstep = cachedxstep[y] = FixedMul(planesin, planeheight) / span;
					ystep = cachedystep[y] = FixedMul(planecos, planeheight) / span;
				}
			}
			else
			{
				distance = cacheddistance[y];
				xstep = cachedxstep[y];
				ystep = cachedystep[y];
			}

			ds_xstep = FixedDiv(xstep, pSplat->xscale);
			ds_ystep = FixedDiv(ystep, pSplat->yscale);

			ds_xfrac = FixedDiv(offsetx + FixedMul(planecos, distance) + (x1 - centerx) * xstep, pSplat->xscale);
			ds_yfrac = FixedDiv(offsety - FixedMul(planesin, distance) + (x1 - centerx) * ystep, pSplat->yscale);
		}

		if (x2 >= x1)
		{
			ds_y = y;
			ds_x1 = x1;
			ds_x2 = x2;
			spanfunc();
		}

		rastertab[y].minx = INT32_MAX;
		rastertab[y].maxx = INT32_MIN;
	}

	if (pSplat->angle && !pSplat->tilted)
	{
		memset(cachedheight, 0, sizeof(cachedheight));
		angle = (viewangle - ANGLE_90) >> ANGLETOFINESHIFT;
		basexscale = FixedDiv(FINECOSINE(angle), centerxfrac);
		baseyscale = -FixedDiv(FINESINE(angle), centerxfrac);
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

#endif // FLOORSPLATS
