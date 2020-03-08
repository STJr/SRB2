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
/// \brief floor and wall splats

#include "r_draw.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_splats.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_netcmd.h"

#ifdef WALLSPLATS
static wallsplat_t wallsplats[MAXLEVELSPLATS]; // WALL splats
static INT32 freewallsplat;
#endif

#ifdef USEASM
/// \brief for floorsplats \note accessed by asm code
struct rastery_s *prastertab;
#endif

#ifdef FLOORSPLATS
static floorsplat_t floorsplats[1]; // FLOOR splats
static INT32 freefloorsplat;

struct rastery_s
{
	fixed_t minx, maxx; // for each raster line starting at line 0
	fixed_t tx1, ty1;
	fixed_t tx2, ty2; // start/end points in texture at this line
};
static struct rastery_s rastertab[MAXVIDHEIGHT];

static void prepare_rastertab(void);
#endif

// --------------------------------------------------------------------------
// setup splat cache
// --------------------------------------------------------------------------
void R_ClearLevelSplats(void)
{
#ifdef WALLSPLATS
	freewallsplat = 0;
	memset(wallsplats, 0, sizeof (wallsplats));
#endif
#ifdef FLOORSPLATS
	freefloorsplat = 0;
	memset(floorsplats, 0, sizeof (floorsplats));

	// setup to draw floorsplats
	prastertab = rastertab;
	prepare_rastertab();
#endif
}

// ==========================================================================
//                                                                WALL SPLATS
// ==========================================================================
#ifdef WALLSPLATS
// --------------------------------------------------------------------------
// Return a pointer to a splat free for use, or NULL if no more splats are
// available
// --------------------------------------------------------------------------
static wallsplat_t *R_AllocWallSplat(void)
{
	wallsplat_t *splat;
	wallsplat_t *p_splat;
	line_t *li;

	// clear the splat from the line if it was in use
	splat = &wallsplats[freewallsplat];
	li = splat->line;
	if (li)
	{
		// remove splat from line splats list
		if (li->splats == splat)
			li->splats = splat->next; // remove from head
		else
		{
			I_Assert(li->splats != NULL);
			for (p_splat = li->splats; p_splat->next; p_splat = p_splat->next)
				if (p_splat->next == splat)
				{
					p_splat->next = splat->next;
					break;
				}
		}
	}

	memset(splat, 0, sizeof (wallsplat_t));

	// for next allocation
	freewallsplat++;
	if (freewallsplat >= 20)
		freewallsplat = 0;

	return splat;
}

// Add a new splat to the linedef:
// top: top z coord
// wallfrac: frac along the linedef vector (0 to FRACUNIT)
// splatpatchname: name of patch to draw
void R_AddWallSplat(line_t *wallline, INT16 sectorside, const char *patchname, fixed_t top,
	fixed_t wallfrac, INT32 flags)
{
	fixed_t fracsplat, linelength;
	wallsplat_t *splat = NULL;
	wallsplat_t *p_splat;
	patch_t *patch;
	sector_t *backsector = NULL;

	if (W_CheckNumForName(patchname) != LUMPERROR)
		splat = R_AllocWallSplat();
	if (!splat)
		return;

	// set the splat
	splat->patch = W_GetNumForName(patchname);
	sectorside ^= 1;
	if (wallline->sidenum[sectorside] != 0xffff)
	{
		backsector = sides[wallline->sidenum[sectorside]].sector;

		if (top < backsector->floorheight)
		{
			splat->yoffset = &backsector->floorheight;
			top -= backsector->floorheight;
		}
		else if (top > backsector->ceilingheight)
		{
			splat->yoffset = &backsector->ceilingheight;
			top -= backsector->ceilingheight;
		}
	}

	splat->top = top;
	splat->flags = flags;

	// bad.. but will be needed for drawing anyway..
	patch = W_CachePatchNum(splat->patch, PU_PATCH);

	// offset needed by draw code for texture mapping
	linelength = P_SegLength((seg_t *)wallline);
	splat->offset = FixedMul(wallfrac, linelength) - (SHORT(patch->width)<<(FRACBITS-1));
	fracsplat = FixedDiv(((SHORT(patch->width)<<FRACBITS)>>1), linelength);

	wallfrac -= fracsplat;
	if (wallfrac > linelength)
		return;
	splat->v1.x = wallline->v1->x + FixedMul(wallline->dx, wallfrac);
	splat->v1.y = wallline->v1->y + FixedMul(wallline->dy, wallfrac);
	wallfrac += fracsplat + fracsplat;
	if (wallfrac < 0)
		return;
	splat->v2.x = wallline->v1->x + FixedMul(wallline->dx, wallfrac);
	splat->v2.y = wallline->v1->y + FixedMul(wallline->dy, wallfrac);

	if (wallline->frontsector && wallline->frontsector == backsector)
		return;

	// insert splat in the linedef splat list
	// BP: why not insert in head is much more simple?
	// BP: because for remove it is more simple!
	splat->line = wallline;
	splat->next = NULL;
	if (wallline->splats)
	{
		p_splat = wallline->splats;
		while (p_splat->next)
			p_splat = p_splat->next;
		p_splat->next = splat;
	}
	else
		wallline->splats = splat;
}
#endif // WALLSPLATS

// ==========================================================================
//                                                               FLOOR SPLATS
// ==========================================================================
#ifdef FLOORSPLATS

// --------------------------------------------------------------------------
// Return a pointer to a splat free for use, or NULL if no more splats are
// available
// --------------------------------------------------------------------------
static floorsplat_t *R_AllocFloorSplat(void)
{
	floorsplat_t *splat;
	floorsplat_t *p_splat;
	subsector_t *sub;

	// find splat to use
	freefloorsplat++;
	if (freefloorsplat >= 1)
		freefloorsplat = 0;

	// clear the splat from the line if it was in use
	splat = &floorsplats[freefloorsplat];
	sub = splat->subsector;
	if (sub)
	{
		// remove splat from subsector splats list
		if (sub->splats == splat)
			sub->splats = splat->next; // remove from head
		else
		{
			p_splat = sub->splats;
			while (p_splat->next)
			{
				if (p_splat->next == splat)
					p_splat->next = splat->next;
			}
		}
	}

	memset(splat, 0, sizeof (floorsplat_t));
	return splat;
}

// --------------------------------------------------------------------------
// Add a floor splat to the subsector
// --------------------------------------------------------------------------
void R_AddFloorSplat(subsector_t *subsec, mobj_t *mobj, const char *picname, fixed_t x, fixed_t y, fixed_t z,
	INT32 flags)
{
	floorsplat_t *splat = NULL;
	floorsplat_t *p_splat;
	INT32 size;

	if (W_CheckNumForName(picname) != LUMPERROR)
		splat = R_AllocFloorSplat();
	if (!splat)
		return;

	// set the splat
	splat->pic = W_GetNumForName(picname);
	splat->flags = flags;
	splat->mobj = mobj;

	splat->z = z;

	size = W_LumpLength(splat->pic);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			splat->size = 1024;
			break;
		case 1048576: // 1024x1024 lump
			splat->size = 512;
			break;
		case 262144:// 512x512 lump
			splat->size = 256;
			break;
		case 65536: // 256x256 lump
			splat->size = 128;
			break;
		case 16384: // 128x128 lump
			splat->size = 64;
			break;
		case 1024: // 32x32 lump
			splat->size = 16;
			break;
		default: // 64x64 lump
			splat->size = 32;
			break;
	}

	// 3--2
	// |  |
	// 0--1
	//
	splat->verts[0].x = splat->verts[3].x = x - (splat->size<<FRACBITS);
	splat->verts[2].x = splat->verts[1].x = x + ((splat->size-1)<<FRACBITS);
	splat->verts[3].y = splat->verts[2].y = y + ((splat->size-1)<<FRACBITS);
	splat->verts[0].y = splat->verts[1].y = y - (splat->size<<FRACBITS);

	// insert splat in the subsector splat list
	splat->subsector = subsec;
	splat->next = NULL;
	if (subsec->splats)
	{
		p_splat = subsec->splats;
		while (p_splat->next)
			p_splat = p_splat->next;
		p_splat->next = splat;
	}
	else
		subsec->splats = splat;
}

// --------------------------------------------------------------------------
// Before each frame being rendered, clear the visible floorsplats list
// --------------------------------------------------------------------------
static floorsplat_t *visfloorsplats;

void R_ClearVisibleFloorSplats(void)
{
	visfloorsplats = NULL;
}

// --------------------------------------------------------------------------
// Add a floorsplat to the visible floorsplats list, for the current frame
// --------------------------------------------------------------------------
void R_AddVisibleFloorSplats(subsector_t *subsec)
{
	floorsplat_t *pSplat;
	I_Assert(subsec->splats != NULL);

	pSplat = subsec->splats;
	// the splat is not visible from below
	// FIXME: depending on some flag in pSplat->flags, some splats may be visible from 2 sides
	// (above/below)
	if (pSplat->z < viewz)
	{
		pSplat->nextvis = visfloorsplats;
		visfloorsplats = pSplat;
	}

	while (pSplat->next)
	{
		pSplat = pSplat->next;
		if (pSplat->z < viewz)
		{
			pSplat->nextvis = visfloorsplats;
			visfloorsplats = pSplat;
		}
	}
}

#ifdef USEASM
// tv1, tv2 = x/y qui varie dans la texture, tc = x/y qui est constant.
void ASMCALL rasterize_segment_tex(INT32 x1, INT32 y1, INT32 x2, INT32 y2, INT32 tv1, INT32 tv2,
	INT32 tc, INT32 dir);
#endif

// current test with floor tile
//#define FLOORSPLATSOLIDCOLOR

// --------------------------------------------------------------------------
// Rasterize the four edges of a floor splat polygon,
// fill the polygon with linear interpolation, call span drawer for each
// scan line
// --------------------------------------------------------------------------
static void R_RenderFloorSplat(floorsplat_t *pSplat, vertex_t *verts, UINT8 *pTex)
{
	// rasterizing
	INT32 miny = vid.height + 1, maxy = 0, y, x1, ry1, x2, y2;
	fixed_t offsetx, offsety;

#ifdef FLOORSPLATSOLIDCOLOR
	UINT8 *pDest;
	INT32 tdx, tdy, ty, tx, x;
#else
	lighttable_t **planezlight;
	fixed_t planeheight;
	angle_t angle, planecos, planesin;
	fixed_t distance, span;
	size_t indexr;
	INT32 light;
#endif
	(void)pTex;

	offsetx = pSplat->verts[0].x & ((pSplat->size << FRACBITS)-1);
	offsety = pSplat->verts[0].y & ((pSplat->size << FRACBITS)-1);

	// do segment a -> top of texture
	x1 = verts[3].x;
	ry1 = verts[3].y;
	x2 = verts[2].x;
	y2 = verts[2].y;
	if (ry1 < 0)
		ry1 = 0;
	if (ry1 >= vid.height)
		ry1 = vid.height - 1;
	if (y2 < 0)
		y2 = 0;
	if (y2 >= vid.height)
		y2 = vid.height - 1;
	rasterize_segment_tex(x1, ry1, x2, y2, 0, pSplat->size - 1, 0, 0);
	if (ry1 < miny)
		miny = ry1;
	if (ry1 > maxy)
		maxy = ry1;

	// do segment b -> right side of texture
	x1 = x2;
	ry1 = y2;
	x2 = verts[1].x;
	y2 = verts[1].y;
	if (ry1 < 0)
		ry1 = 0;
	if (ry1 >= vid.height)
		ry1 = vid.height - 1;
	if (y2 < 0)
		y2 = 0;
	if (y2 >= vid.height)
		y2 = vid.height - 1;
	rasterize_segment_tex(x1, ry1, x2, y2, 0, pSplat->size - 1, pSplat->size - 1, 1);
	if (ry1 < miny)
		miny = ry1;
	if (ry1 > maxy)
		maxy = ry1;

	// do segment c -> bottom of texture
	x1 = x2;
	ry1 = y2;
	x2 = verts[0].x;
	y2 = verts[0].y;
	if (ry1 < 0)
		ry1 = 0;
	if (ry1 >= vid.height)
		ry1 = vid.height - 1;
	if (y2 < 0)
		y2 = 0;
	if (y2 >= vid.height)
		y2 = vid.height - 1;
	rasterize_segment_tex(x1, ry1, x2, y2, pSplat->size - 1, 0, pSplat->size - 1, 0);
	if (ry1 < miny)
		miny = ry1;
	if (ry1 > maxy)
		maxy = ry1;

	// do segment d -> left side of texture
	x1 = x2;
	ry1 = y2;
	x2 = verts[3].x;
	y2 = verts[3].y;
	if (ry1 < 0)
		ry1 = 0;
	if (ry1 >= vid.height)
		ry1 = vid.height - 1;
	if (y2 < 0)
		y2 = 0;
	if (y2 >= vid.height)
		y2 = vid.height - 1;
	rasterize_segment_tex(x1, ry1, x2, y2, pSplat->size - 1, 0, 0, 1);
	if (ry1 < miny)
		miny = ry1;
	if (ry1 > maxy)
		maxy = ry1;

#ifndef FLOORSPLATSOLIDCOLOR
	// prepare values for all the splat
	ds_source = W_CacheLumpNum(pSplat->pic, PU_CACHE);
	planeheight = abs(pSplat->z - viewz);
	light = (pSplat->subsector->sector->lightlevel >> LIGHTSEGSHIFT);
	if (light >= LIGHTLEVELS)
		light = LIGHTLEVELS - 1;
	if (light < 0)
		light = 0;
	planezlight = zlight[light];

	for (y = miny; y <= maxy; y++)
	{
		x1 = rastertab[y].minx>>FRACBITS;
		x2 = rastertab[y].maxx>>FRACBITS;

		if (x1 < 0)
			x1 = 0;
		if (x2 >= vid.width)
			x2 = vid.width - 1;

		angle = (currentplane->viewangle + currentplane->plangle)>>ANGLETOFINESHIFT;
		planecos = FINECOSINE(angle);
		planesin = FINESINE(angle);

		if (planeheight != cachedheight[y])
		{
			cachedheight[y] = planeheight;
			distance = cacheddistance[y] = FixedMul(planeheight, yslope[y]);
			ds_xstep = cachedxstep[y] = FixedMul(distance,basexscale);
			ds_ystep = cachedystep[y] = FixedMul(distance,baseyscale);

			if ((span = abs(centery-y)))
			{
				ds_xstep = cachedxstep[y] = FixedMul(planesin, planeheight) / span;
				ds_ystep = cachedystep[y] = FixedMul(planecos, planeheight) / span;
			}
		}
		else
		{
			distance = cacheddistance[y];
			ds_xstep = cachedxstep[y];
			ds_ystep = cachedystep[y];
		}

		ds_xfrac = xoffs + FixedMul(planecos, distance) + (x1 - centerx) * ds_xstep;
		ds_yfrac = yoffs - FixedMul(planesin, distance) + (x1 - centerx) * ds_ystep;
		ds_xfrac -= offsetx;
		ds_yfrac += offsety;

		indexr = distance >> LIGHTZSHIFT;
		if (indexr >= MAXLIGHTZ)
			indexr = MAXLIGHTZ - 1;
		ds_colormap = planezlight[indexr];

		ds_y = y;
		if (x2 >= x1) // sanity check
		{
			ds_x1 = x1;
			ds_x2 = x2;
			ds_transmap = transtables + ((tr_trans50-1)<<FF_TRANSSHIFT);
			(spanfuncs[SPANDRAWFUNC_SPLAT])();
		}

		// reset for next calls to edge rasterizer
		rastertab[y].minx = INT32_MAX;
		rastertab[y].maxx = INT32_MIN;
	}

#else
	for (y = miny; y <= maxy; y++)
	{
		x1 = rastertab[y].minx>>FRACBITS;
		x2 = rastertab[y].maxx>>FRACBITS;
		if (x1 < 0)
			x1 = 0;
		if (x2 >= vid.width)
			x2 = vid.width - 1;

//		pDest = ylookup[y] + columnofs[x1];
		pDest = &topleft[y*vid.width + x1];

		x = x2 - x1 + 1;

		// starting point of the texture
		tx = rastertab[y].tx1;
		ty = rastertab[y].ty1;

		// HORRIBLE BUG!!!
		if (x > 0)
		{
			tdx = (rastertab[y].tx2 - tx) / x;
			tdy = (rastertab[y].ty2 - ty) / x;

			while (x-- > 0)
			{
				*(pDest++) = (UINT8)(y&1);
				tx += tdx;
				ty += tdy;
			}
		}

		// reinitialise the minimum and maximum for the next approach
		rastertab[y].minx = INT32_MAX;
		rastertab[y].maxx = INT32_MIN;
	}
#endif
}

// --------------------------------------------------------------------------
// R_DrawVisibleFloorSplats
// draw the flat floor/ceiling splats
// --------------------------------------------------------------------------
void R_DrawVisibleFloorSplats(void)
{
	floorsplat_t *pSplat;
	INT32 iCount = 0, i;
	fixed_t tr_x, tr_y, rot_x, rot_y, rot_z, xscale, yscale;
	vertex_t *v3d;
	vertex_t v2d[4];

	pSplat = visfloorsplats;
	while (pSplat)
	{
		iCount++;

		// Draw a floor splat
		// 3--2
		// |  |
		// 0--1

		rot_z = pSplat->z - viewz;
		for (i = 0; i < 4; i++)
		{
			v3d = &pSplat->verts[i];

			// transform the origin point
			tr_x = v3d->x - viewx;
			tr_y = v3d->y - viewy;

			// rotation around vertical y axis
			rot_x = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos);
			rot_y = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);

			if (rot_y < 4*FRACUNIT)
				goto skipit;

			// note: y from view above of map, is distance far away
			xscale = FixedDiv(projection, rot_y);
			yscale = -FixedDiv(projectiony, rot_y);

			// projection
			v2d[i].x = (centerxfrac + FixedMul (rot_x, xscale))>>FRACBITS;
			v2d[i].y = (centeryfrac + FixedMul (rot_z, yscale))>>FRACBITS;
		}

		R_RenderFloorSplat(pSplat, v2d, NULL);
skipit:
		pSplat = pSplat->nextvis;
	}
}

static void prepare_rastertab(void)
{
	INT32 iLine;
	for (iLine = 0; iLine < vid.height; iLine++)
	{
		rastertab[iLine].minx = INT32_MAX;
		rastertab[iLine].maxx = INT32_MIN;
	}
}

#endif // FLOORSPLATS
