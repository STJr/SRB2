// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_plane.c
/// \brief Here is a core component: drawing the floors and ceilings,
///        while maintaining a per column clipping list only.
///        Moreover, the sky areas have to be determined.

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "r_data.h"
#include "r_local.h"
#include "r_state.h"
#include "r_splats.h" // faB(21jan):testing
#include "r_sky.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_tick.h"

#include "p_setup.h" // levelflats

#include "p_slopes.h"

//
// opening
//

// Quincunx antialiasing of flats!
//#define QUINCUNX

// good night sweet prince
#define SHITPLANESPARENCY

//SoM: 3/23/2000: Use Boom visplane hashing.
#define MAXVISPLANES 512

static visplane_t *visplanes[MAXVISPLANES];
static visplane_t *freetail;
static visplane_t **freehead = &freetail;

visplane_t *floorplane;
visplane_t *ceilingplane;
static visplane_t *currentplane;

planemgr_t ffloor[MAXFFLOORS];
INT32 numffloors;

//SoM: 3/23/2000: Boom visplane hashing routine.
#define visplane_hash(picnum,lightlevel,height) \
  ((unsigned)((picnum)*3+(lightlevel)+(height)*7) & (MAXVISPLANES-1))

//SoM: 3/23/2000: Use boom opening limit removal
size_t maxopenings;
INT16 *openings, *lastopening; /// \todo free leak

//
// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
//
INT16 floorclip[MAXVIDWIDTH], ceilingclip[MAXVIDWIDTH];
fixed_t frontscale[MAXVIDWIDTH];

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
static INT32 spanstart[MAXVIDHEIGHT];

//
// texture mapping
//
lighttable_t **planezlight;
static fixed_t planeheight;

//added : 10-02-98: yslopetab is what yslope used to be,
//                yslope points somewhere into yslopetab,
//                now (viewheight/2) slopes are calculated above and
//                below the original viewheight for mouselook
//                (this is to calculate yslopes only when really needed)
//                (when mouselookin', yslope is moving into yslopetab)
//                Check R_SetupFrame, R_SetViewSize for more...
fixed_t yslopetab[MAXVIDHEIGHT*4];
fixed_t *yslope;

fixed_t distscale[MAXVIDWIDTH];
fixed_t basexscale, baseyscale;

fixed_t cachedheight[MAXVIDHEIGHT];
fixed_t cacheddistance[MAXVIDHEIGHT];
fixed_t cachedxstep[MAXVIDHEIGHT];
fixed_t cachedystep[MAXVIDHEIGHT];

static fixed_t xoffs, yoffs;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes(void)
{
	// FIXME: unused
}

// R_PortalStoreClipValues
// Saves clipping values for later. -Red
void R_PortalStoreClipValues(INT32 start, INT32 end, INT16 *ceil, INT16 *floor, fixed_t *scale)
{
	INT32 i;
	for (i = 0; i < end-start; i++)
	{
		*ceil = ceilingclip[start+i];
		ceil++;
		*floor = floorclip[start+i];
		floor++;
		*scale = frontscale[start+i];
		scale++;
	}
}

// R_PortalRestoreClipValues
// Inverse of the above. Restores the old value!
void R_PortalRestoreClipValues(INT32 start, INT32 end, INT16 *ceil, INT16 *floor, fixed_t *scale)
{
	INT32 i;
	for (i = 0; i < end-start; i++)
	{
		ceilingclip[start+i] = *ceil;
		ceil++;
		floorclip[start+i] = *floor;
		floor++;
		frontscale[start+i] = *scale;
		scale++;
	}

	// HACKS FOLLOW
	for (i = 0; i < start; i++)
	{
		floorclip[i] = -1;
		ceilingclip[i] = (INT16)viewheight;
	}
	for (i = end; i < vid.width; i++)
	{
		floorclip[i] = -1;
		ceilingclip[i] = (INT16)viewheight;
	}
}


//profile stuff ---------------------------------------------------------
//#define TIMING
#ifdef TIMING
#include "p5prof.h"
         INT64 mycount;
         INT64 mytotal = 0;
         UINT32 nombre = 100000;
#endif
//profile stuff ---------------------------------------------------------


//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//  xoffs
//  yoffs
//  planeangle
//
// BASIC PRIMITIVE
//
#ifndef NOWATER
static INT32 bgofs;
static INT32 wtofs=0;
static INT32 waterofs;
static boolean itswater;
#endif

#ifdef __mips__
//#define NOWATER
#endif

#ifndef NOWATER
static void R_DrawTranslucentWaterSpan_8(void)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	size_t count;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds_xfrac << nflatshiftup; yposition = (ds_yfrac + waterofs) << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = ylookup[ds_y] + columnofs[ds_x1];
	dsrc = screens[1] + (ds_y+bgofs)*vid.width + ds_x1;
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		*dest++ = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}
#endif

void R_MapPlane(INT32 y, INT32 x1, INT32 x2)
{
	angle_t angle;
	fixed_t distance, length;
	size_t pindex;

#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapPlane: %d, %d at %d", x1, x2, y);
#endif

	// from r_splats's R_RenderFloorSplat
	if (x1 >= vid.width) x1 = vid.width - 1;

	if (planeheight != cachedheight[y])
	{
		cachedheight[y] = planeheight;
		distance = cacheddistance[y] = FixedMul(planeheight, yslope[y]);
		ds_xstep = cachedxstep[y] = FixedMul(distance, basexscale);
		ds_ystep = cachedystep[y] = FixedMul(distance, baseyscale);
	}
	else
	{
		distance = cacheddistance[y];
		ds_xstep = cachedxstep[y];
		ds_ystep = cachedystep[y];
	}

	length = FixedMul (distance,distscale[x1]);
	angle = (currentplane->viewangle + currentplane->plangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
	/// \note Wouldn't it be faster just to add viewx and viewy
	// to the plane's x/yoffs anyway??

	ds_xfrac = FixedMul(FINECOSINE(angle), length) + xoffs;
	ds_yfrac = yoffs - FixedMul(FINESINE(angle), length);

#ifndef NOWATER
	if (itswater)
	{
		const INT32 yay = (wtofs + (distance>>9) ) & 8191;
		// ripples da water texture
		bgofs = FixedDiv(FINESINE(yay), (1<<12) + (distance>>11))>>FRACBITS;

		angle = (angle + 2048) & 8191;  //90�
		ds_xfrac += FixedMul(FINECOSINE(angle), (bgofs<<FRACBITS));
		ds_yfrac += FixedMul(FINESINE(angle), (bgofs<<FRACBITS));

		if (y+bgofs>=viewheight)
			bgofs = viewheight-y-1;
		if (y+bgofs<0)
			bgofs = -y;
	}
#endif

	pindex = distance >> LIGHTZSHIFT;

	if (pindex >= MAXLIGHTZ)
		pindex = MAXLIGHTZ - 1;

#ifdef ESLOPE
	if (currentplane->slope)
		ds_colormap = colormaps;
	else
#endif
	ds_colormap = planezlight[pindex];

	if (currentplane->extra_colormap)
		ds_colormap = currentplane->extra_colormap->colormap + (ds_colormap - colormaps);

	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;

	// profile drawer
#ifdef TIMING
	ProfZeroTimer();
#endif

	spanfunc();

#ifdef TIMING
	RDMSR(0x10, &mycount);
	mytotal += mycount; // 64bit add
	if (!(nombre--))
	I_Error("spanfunc() CPU Spy reports: 0x%d %d\n", *((INT32 *)&mytotal+1), (INT32)mytotal);
#endif
}

//
// R_ClearPlanes
// At begining of frame.
//
// NOTE: Uses con_clipviewtop, so that when console is on,
//       we don't draw the part of the view hidden under the console.
void R_ClearPlanes(void)
{
	INT32 i, p;
	angle_t angle;

	// opening / clipping determination
	for (i = 0; i < viewwidth; i++)
	{
		floorclip[i] = (INT16)viewheight;
		ceilingclip[i] = (INT16)con_clipviewtop;
		frontscale[i] = INT32_MAX;
		for (p = 0; p < MAXFFLOORS; p++)
		{
			ffloor[p].f_clip[i] = (INT16)viewheight;
			ffloor[p].c_clip[i] = (INT16)con_clipviewtop;
		}
	}

	numffloors = 0;

	for (i = 0; i < MAXVISPLANES; i++)
	for (*freehead = visplanes[i], visplanes[i] = NULL;
		freehead && *freehead ;)
	{
		freehead = &(*freehead)->next;
	}

	lastopening = openings;

	// texture calculation
	memset(cachedheight, 0, sizeof (cachedheight));

	// left to right mapping
	angle = (viewangle-ANGLE_90)>>ANGLETOFINESHIFT;

	// scale will be unit scale at SCREENWIDTH/2 distance
	basexscale = FixedDiv (FINECOSINE(angle),centerxfrac);
	baseyscale = -FixedDiv (FINESINE(angle),centerxfrac);
}

static visplane_t *new_visplane(unsigned hash)
{
	visplane_t *check = freetail;
	if (!check)
	{
		check = calloc(2, sizeof (*check));
		if (check == NULL) I_Error("%s: Out of memory", "new_visplane"); // FIXME: ugly
	}
	else
	{
		freetail = freetail->next;
		if (!freetail)
			freehead = &freetail;
	}
	check->next = visplanes[hash];
	visplanes[hash] = check;
	return check;
}

//
// R_FindPlane: Seek a visplane having the identical values:
//              Same height, same flattexture, same lightlevel.
//              If not, allocates another of them.
//
visplane_t *R_FindPlane(fixed_t height, INT32 picnum, INT32 lightlevel,
	fixed_t xoff, fixed_t yoff, angle_t plangle, extracolormap_t *planecolormap,
	ffloor_t *pfloor
#ifdef POLYOBJECTS_PLANES
			, polyobj_t *polyobj
#endif
#ifdef ESLOPE
			, pslope_t *slope
#endif
			)
{
	visplane_t *check;
	unsigned hash;

#ifdef ESLOPE
	if (slope); else // Don't mess with this right now if a slope is involved
#endif
	if (plangle != 0)
	{
		// Add the view offset, rotated by the plane angle.
		angle_t angle = plangle>>ANGLETOFINESHIFT;
		xoff += FixedMul(viewx,FINECOSINE(angle))-FixedMul(viewy,FINESINE(angle));
		yoff += -FixedMul(viewx,FINESINE(angle))-FixedMul(viewy,FINECOSINE(angle));
	}
	else
	{
		xoff += viewx;
		yoff -= viewy;
	}

	// This appears to fix the Nimbus Ruins sky bug.
	if (picnum == skyflatnum && pfloor)
	{
		height = 0; // all skies map together
		lightlevel = 0;
	}

	// New visplane algorithm uses hash table
	hash = visplane_hash(picnum, lightlevel, height);

	for (check = visplanes[hash]; check; check = check->next)
	{
#ifdef POLYOBJECTS_PLANES
		if (check->polyobj && pfloor)
			continue;
		if (polyobj != check->polyobj)
			continue;
#endif
		if (height == check->height && picnum == check->picnum
			&& lightlevel == check->lightlevel
			&& xoff == check->xoffs && yoff == check->yoffs
			&& planecolormap == check->extra_colormap
			&& !pfloor && !check->ffloor
			&& check->viewx == viewx && check->viewy == viewy && check->viewz == viewz
			&& check->viewangle == viewangle
#ifdef ESLOPE
			&& check->slope == slope
#endif
			)
		{
			return check;
		}
	}

	check = new_visplane(hash);

	check->height = height;
	check->picnum = picnum;
	check->lightlevel = lightlevel;
	check->minx = vid.width;
	check->maxx = -1;
	check->xoffs = xoff;
	check->yoffs = yoff;
	check->extra_colormap = planecolormap;
	check->ffloor = pfloor;
	check->viewx = viewx;
	check->viewy = viewy;
	check->viewz = viewz;
	check->viewangle = viewangle;
	check->plangle = plangle;
#ifdef POLYOBJECTS_PLANES
	check->polyobj = polyobj;
#endif
#ifdef ESLOPE
	check->slope = slope;
#endif

	memset(check->top, 0xff, sizeof (check->top));
	memset(check->bottom, 0x00, sizeof (check->bottom));

	return check;
}

//
// R_CheckPlane: return same visplane or alloc a new one if needed
//
visplane_t *R_CheckPlane(visplane_t *pl, INT32 start, INT32 stop)
{
	INT32 intrl, intrh;
	INT32 unionl, unionh;
	INT32 x;

	if (start < pl->minx)
	{
		intrl = pl->minx;
		unionl = start;
	}
	else
	{
		unionl = pl->minx;
		intrl = start;
	}

	if (stop > pl->maxx)
	{
		intrh = pl->maxx;
		unionh = stop;
	}
	else
	{
		unionh = pl->maxx;
		intrh = stop;
	}

	// 0xff is not equal to -1 with shorts...
	for (x = intrl; x <= intrh; x++)
		if (pl->top[x] != 0xffff || pl->bottom[x] != 0x0000)
			break;

	if (x > intrh) /* Can use existing plane; extend range */
	{
		pl->minx = unionl;
		pl->maxx = unionh;
	}
	else /* Cannot use existing plane; create a new one */
	{
		unsigned hash =
			visplane_hash(pl->picnum, pl->lightlevel, pl->height);
		visplane_t *new_pl = new_visplane(hash);

		new_pl->height = pl->height;
		new_pl->picnum = pl->picnum;
		new_pl->lightlevel = pl->lightlevel;
		new_pl->xoffs = pl->xoffs;
		new_pl->yoffs = pl->yoffs;
		new_pl->extra_colormap = pl->extra_colormap;
		new_pl->ffloor = pl->ffloor;
		new_pl->viewx = pl->viewx;
		new_pl->viewy = pl->viewy;
		new_pl->viewz = pl->viewz;
		new_pl->viewangle = pl->viewangle;
		new_pl->plangle = pl->plangle;
#ifdef POLYOBJECTS_PLANES
		new_pl->polyobj = pl->polyobj;
#endif
#ifdef ESLOPE
		new_pl->slope = pl->slope;
#endif
		pl = new_pl;
		pl->minx = start;
		pl->maxx = stop;
		memset(pl->top, 0xff, sizeof pl->top);
		memset(pl->bottom, 0x00, sizeof pl->bottom);
	}
	return pl;
}


//
// R_ExpandPlane
//
// This function basically expands the visplane or I_Errors.
// The reason for this is that when creating 3D floor planes, there is no
// need to create new ones with R_CheckPlane, because 3D floor planes
// are created by subsector and there is no way a subsector can graphically
// overlap.
void R_ExpandPlane(visplane_t *pl, INT32 start, INT32 stop)
{
	INT32 unionl, unionh;
//	INT32 x;

#ifdef POLYOBJECTS_PLANES
	// Don't expand polyobject planes here - we do that on our own.
	if (pl->polyobj)
		return;
#endif

	if (start < pl->minx)
	{
		unionl = start;
	}
	else
	{
		unionl = pl->minx;
	}

	if (stop > pl->maxx)
	{
		unionh = stop;
	}
	else
	{
		unionh = pl->maxx;
	}
/*
	for (x = start; x <= stop; x++)
		if (pl->top[x] != 0xffff || pl->bottom[x] != 0x0000)
			break;

	if (x <= stop)
		I_Error("R_ExpandPlane: planes in same subsector overlap?!\nminx: %d, maxx: %d, start: %d, stop: %d\n", pl->minx, pl->maxx, start, stop);
*/
	pl->minx = unionl, pl->maxx = unionh;
}

//
// R_MakeSpans
//
void R_MakeSpans(INT32 x, INT32 t1, INT32 b1, INT32 t2, INT32 b2)
{
	//    Alam: from r_splats's R_RenderFloorSplat
	if (t1 >= vid.height) t1 = vid.height-1;
	if (b1 >= vid.height) b1 = vid.height-1;
	if (t2 >= vid.height) t2 = vid.height-1;
	if (b2 >= vid.height) b2 = vid.height-1;
	if (x-1 >= vid.width) x = vid.width;

	while (t1 < t2 && t1 <= b1)
	{
		R_MapPlane(t1, spanstart[t1], x - 1);
		t1++;
	}
	while (b1 > b2 && b1 >= t1)
	{
		R_MapPlane(b1, spanstart[b1], x - 1);
		b1--;
	}

	while (t2 < t1 && t2 <= b2)
		spanstart[t2++] = x;
	while (b2 > b1 && b2 >= t2)
		spanstart[b2--] = x;
}

void R_DrawPlanes(void)
{
	visplane_t *pl;
	INT32 x;
	INT32 angle;
	INT32 i;

	spanfunc = basespanfunc;
	wallcolfunc = walldrawerfunc;

	for (i = 0; i < MAXVISPLANES; i++, pl++)
	{
		for (pl = visplanes[i]; pl; pl = pl->next)
		{
			// sky flat
			if (pl->picnum == skyflatnum)
			{
				if (!viewsky)
				{
					skyVisible = true;
					continue;
				}

				// use correct aspect ratio scale
				dc_iscale = skyscale;

				// Sky is always drawn full bright,
				//  i.e. colormaps[0] is used.
				// Because of this hack, sky is not affected
				//  by INVUL inverse mapping.
				dc_colormap = colormaps;
				dc_texturemid = skytexturemid;
				dc_texheight = textureheight[skytexture]
					>>FRACBITS;
				for (x = pl->minx; x <= pl->maxx; x++)
				{
					dc_yl = pl->top[x];
					dc_yh = pl->bottom[x];

					if (dc_yl <= dc_yh)
					{
						angle = (pl->viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
						dc_x = x;
						dc_source =
							R_GetColumn(skytexture,
								angle);
						wallcolfunc();
					}
				}
				continue;
			}

			if (pl->ffloor != NULL
#ifdef POLYOBJECTS_PLANES
			|| pl->polyobj != NULL
#endif
			)
				continue;

			R_DrawSinglePlane(pl);
		}
	}
#ifndef NOWATER
	waterofs = (leveltime & 1)*16384;
	wtofs = leveltime * 140;
#endif
}

void R_DrawSinglePlane(visplane_t *pl)
{
	INT32 light = 0;
	INT32 x;
	INT32 stop, angle;
	size_t size;
	ffloor_t *rover;

	if (!(pl->minx <= pl->maxx))
		return;

#ifndef NOWATER
	itswater = false;
#endif
	spanfunc = basespanfunc;

#ifdef POLYOBJECTS_PLANES
	if (pl->polyobj && pl->polyobj->translucency != 0) {
		spanfunc = R_DrawTranslucentSpan_8;

		// Hacked up support for alpha value in software mode Tails 09-24-2002 (sidenote: ported to polys 10-15-2014, there was no time travel involved -Red)
		if (pl->polyobj->translucency >= 10)
			return; // Don't even draw it
		else if (pl->polyobj->translucency > 0)
			ds_transmap = transtables + ((pl->polyobj->translucency-1)<<FF_TRANSSHIFT);
		else // Opaque, but allow transparent flat pixels
			spanfunc = splatfunc;

#ifdef SHITPLANESPARENCY
		if (spanfunc == splatfunc || (pl->extra_colormap && pl->extra_colormap->fog))
#else
		if (!pl->extra_colormap || !(pl->extra_colormap->fog & 2))
#endif
			light = (pl->lightlevel >> LIGHTSEGSHIFT);
		else
			light = LIGHTLEVELS-1;

	} else
#endif
	if (pl->ffloor)
	{
		// Don't draw planes that shouldn't be drawn.
		for (rover = pl->ffloor->target->ffloors; rover; rover = rover->next)
		{
			if ((pl->ffloor->flags & FF_CUTEXTRA) && (rover->flags & FF_EXTRA))
			{
				if (pl->ffloor->flags & FF_EXTRA)
				{
					// The plane is from an extra 3D floor... Check the flags so
					// there are no undesired cuts.
					if (((pl->ffloor->flags & (FF_FOG|FF_SWIMMABLE)) == (rover->flags & (FF_FOG|FF_SWIMMABLE)))
						&& pl->height < *rover->topheight
						&& pl->height > *rover->bottomheight)
						return;
				}
			}
		}

		if (pl->ffloor->flags & FF_TRANSLUCENT)
		{
			spanfunc = R_DrawTranslucentSpan_8;

			// Hacked up support for alpha value in software mode Tails 09-24-2002
			if (pl->ffloor->alpha < 12)
				return; // Don't even draw it
			else if (pl->ffloor->alpha < 38)
				ds_transmap = transtables + ((tr_trans90-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 64)
				ds_transmap = transtables + ((tr_trans80-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 89)
				ds_transmap = transtables + ((tr_trans70-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 115)
				ds_transmap = transtables + ((tr_trans60-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 140)
				ds_transmap = transtables + ((tr_trans50-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 166)
				ds_transmap = transtables + ((tr_trans40-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 192)
				ds_transmap = transtables + ((tr_trans30-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 217)
				ds_transmap = transtables + ((tr_trans20-1)<<FF_TRANSSHIFT);
			else if (pl->ffloor->alpha < 243)
				ds_transmap = transtables + ((tr_trans10-1)<<FF_TRANSSHIFT);
			else // Opaque, but allow transparent flat pixels
				spanfunc = splatfunc;

#ifdef SHITPLANESPARENCY
			if (spanfunc == splatfunc || (pl->extra_colormap && pl->extra_colormap->fog))
#else
			if (!pl->extra_colormap || !(pl->extra_colormap->fog & 2))
#endif
				light = (pl->lightlevel >> LIGHTSEGSHIFT);
			else
				light = LIGHTLEVELS-1;
		}
		else if (pl->ffloor->flags & FF_FOG)
		{
			spanfunc = R_DrawFogSpan_8;
			light = (pl->lightlevel >> LIGHTSEGSHIFT);
		}
		else light = (pl->lightlevel >> LIGHTSEGSHIFT);

#ifndef NOWATER
		if (pl->ffloor->flags & FF_RIPPLE
#ifdef ESLOPE
				&& !pl->slope
#endif
			)
		{
			INT32 top, bottom;

			itswater = true;
			if (spanfunc == R_DrawTranslucentSpan_8)
			{
				spanfunc = R_DrawTranslucentWaterSpan_8;

				// Copy the current scene, ugh
				top = pl->high-8;
				bottom = pl->low+8;

				if (top < 0)
					top = 0;
				if (bottom > vid.height)
					bottom = vid.height;

				// Only copy the part of the screen we need
				VID_BlitLinearScreen((splitscreen && viewplayer == &players[secondarydisplayplayer]) ? screens[0] + (top+(vid.height>>1))*vid.width : screens[0]+((top)*vid.width), screens[1]+((top)*vid.width),
				                     vid.width, bottom-top,
				                     vid.width, vid.width);
			}
		}
#endif
	}
	else light = (pl->lightlevel >> LIGHTSEGSHIFT);

#ifdef ESLOPE
	if (!pl->slope) // Don't mess with angle on slopes! We'll handle this ourselves later
#endif
	if (viewangle != pl->viewangle+pl->plangle)
	{
		memset(cachedheight, 0, sizeof (cachedheight));
		angle = (pl->viewangle+pl->plangle-ANGLE_90)>>ANGLETOFINESHIFT;
		basexscale = FixedDiv(FINECOSINE(angle),centerxfrac);
		baseyscale = -FixedDiv(FINESINE(angle),centerxfrac);
		viewangle = pl->viewangle+pl->plangle;
	}

	currentplane = pl;

	ds_source = (UINT8 *)
		W_CacheLumpNum(levelflats[pl->picnum].lumpnum,
			PU_STATIC); // Stay here until Z_ChangeTag

	size = W_LumpLength(levelflats[pl->picnum].lumpnum);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			nflatmask = 0x3FF800;
			nflatxshift = 21;
			nflatyshift = 10;
			nflatshiftup = 5;
			break;
		case 1048576: // 1024x1024 lump
			nflatmask = 0xFFC00;
			nflatxshift = 22;
			nflatyshift = 12;
			nflatshiftup = 6;
			break;
		case 262144:// 512x512 lump'
			nflatmask = 0x3FE00;
			nflatxshift = 23;
			nflatyshift = 14;
			nflatshiftup = 7;
			break;
		case 65536: // 256x256 lump
			nflatmask = 0xFF00;
			nflatxshift = 24;
			nflatyshift = 16;
			nflatshiftup = 8;
			break;
		case 16384: // 128x128 lump
			nflatmask = 0x3F80;
			nflatxshift = 25;
			nflatyshift = 18;
			nflatshiftup = 9;
			break;
		case 1024: // 32x32 lump
			nflatmask = 0x3E0;
			nflatxshift = 27;
			nflatyshift = 22;
			nflatshiftup = 11;
			break;
		default: // 64x64 lump
			nflatmask = 0xFC0;
			nflatxshift = 26;
			nflatyshift = 20;
			nflatshiftup = 10;
			break;
	}

	xoffs = pl->xoffs;
	yoffs = pl->yoffs;
	planeheight = abs(pl->height - pl->viewz);

	if (light >= LIGHTLEVELS)
		light = LIGHTLEVELS-1;

	if (light < 0)
		light = 0;

#ifdef ESLOPE
	if (pl->slope) {
		// Potentially override other stuff for now cus we're mean. :< But draw a slope plane!
		// I copied ZDoom's code and adapted it to SRB2... -Red
		floatv3_t p, m, n;
		float ang;
		float vx, vy, vz;
		float fudge;
		// compiler complains when P_GetZAt is used in FLOAT_TO_FIXED directly
		// use this as a temp var to store P_GetZAt's return value each time
		fixed_t temp;

		xoffs &= ((1 << (32-nflatshiftup))-1);
		yoffs &= ((1 << (32-nflatshiftup))-1);

		xoffs -= (pl->slope->o.x + (1 << (31-nflatshiftup))) & ~((1 << (32-nflatshiftup))-1);
		yoffs += (pl->slope->o.y + (1 << (31-nflatshiftup))) & ~((1 << (32-nflatshiftup))-1);

		// Okay, look, don't ask me why this works, but without this setup there's a disgusting-looking misalignment with the textures. -Red
		fudge = ((1<<nflatshiftup)+1.0f)/(1<<nflatshiftup);

		xoffs = (fixed_t)(xoffs*fudge);
		yoffs = (fixed_t)(yoffs/fudge);

		vx = FIXED_TO_FLOAT(pl->viewx+xoffs);
		vy = FIXED_TO_FLOAT(pl->viewy-yoffs);
		vz = FIXED_TO_FLOAT(pl->viewz);

		temp = P_GetZAt(pl->slope, pl->viewx, pl->viewy);
		zeroheight = FIXED_TO_FLOAT(temp);

#define ANG2RAD(angle) ((float)((angle)*M_PI)/ANGLE_180)

		// p is the texture origin in view space
		// Don't add in the offsets at this stage, because doing so can result in
		// errors if the flat is rotated.
		ang = ANG2RAD(ANGLE_270 - pl->viewangle);
		p.x = vx * cos(ang) - vy * sin(ang);
		p.z = vx * sin(ang) + vy * cos(ang);
		temp = P_GetZAt(pl->slope, -xoffs, yoffs);
		p.y = FIXED_TO_FLOAT(temp) - vz;

		// m is the v direction vector in view space
		ang = ANG2RAD(ANGLE_180 - (pl->viewangle + pl->plangle));
		m.x = cos(ang);
		m.z = sin(ang);

		// n is the u direction vector in view space
		n.x = sin(ang);
		n.z = -cos(ang);

		ang = ANG2RAD(pl->plangle);
		temp = P_GetZAt(pl->slope, pl->viewx + FLOAT_TO_FIXED(sin(ang)), pl->viewy + FLOAT_TO_FIXED(cos(ang)));
		m.y = FIXED_TO_FLOAT(temp) - zeroheight;
		temp = P_GetZAt(pl->slope, pl->viewx + FLOAT_TO_FIXED(cos(ang)), pl->viewy - FLOAT_TO_FIXED(sin(ang)));
		n.y = FIXED_TO_FLOAT(temp) - zeroheight;

		m.x /= fudge;
		m.y /= fudge;
		m.z /= fudge;

		n.x *= fudge;
		n.y *= fudge;
		n.z *= fudge;

		// Eh. I tried making this stuff fixed-point and it exploded on me. Here's a macro for the only floating-point vector function I recall using.
#define CROSS(d, v1, v2) \
   d.x = (v1.y * v2.z) - (v1.z * v2.y);\
   d.y = (v1.z * v2.x) - (v1.x * v2.z);\
   d.z = (v1.x * v2.y) - (v1.y * v2.x)
		CROSS(ds_su, p, m);
		CROSS(ds_sv, p, n);
		CROSS(ds_sz, m, n);
#undef CROSS

		ds_su.z *= focallengthf;
		ds_sv.z *= focallengthf;
		ds_sz.z *= focallengthf;

		// Premultiply the texture vectors with the scale factors
#define SFMULT 65536.f*(1<<nflatshiftup)
		ds_su.x *= SFMULT;
		ds_su.y *= SFMULT;
		ds_su.z *= SFMULT;
		ds_sv.x *= SFMULT;
		ds_sv.y *= SFMULT;
		ds_sv.z *= SFMULT;
#undef SFMULT

		if (spanfunc == R_DrawTranslucentSpan_8)
			spanfunc = R_DrawTiltedTranslucentSpan_8;
		else if (spanfunc == splatfunc)
			spanfunc = R_DrawTiltedSplat_8;
		else
			spanfunc = R_DrawTiltedSpan_8;

		planezlight = scalelight[light];
	} else
#endif // ESLOPE

	planezlight = zlight[light];

	// set the maximum value for unsigned
	pl->top[pl->maxx+1] = 0xffff;
	pl->top[pl->minx-1] = 0xffff;
	pl->bottom[pl->maxx+1] = 0x0000;
	pl->bottom[pl->minx-1] = 0x0000;

	stop = pl->maxx + 1;

	if (viewx != pl->viewx || viewy != pl->viewy)
	{
		viewx = pl->viewx;
		viewy = pl->viewy;
	}
	if (viewz != pl->viewz)
		viewz = pl->viewz;

	for (x = pl->minx; x <= stop; x++)
	{
		R_MakeSpans(x, pl->top[x-1], pl->bottom[x-1],
			pl->top[x], pl->bottom[x]);
	}

/*
QUINCUNX anti-aliasing technique (sort of)

Normally, Quincunx antialiasing staggers pixels
in a 5-die pattern like so:

o   o
  o
o   o

To simulate this, we offset the plane by
FRACUNIT/4 in each direction, and draw
at 50% translucency. The result is
a 'smoothing' of the texture while
using the palette colors.
*/
#ifdef QUINCUNX
	if (spanfunc == R_DrawSpan_8)
	{
		INT32 i;
		ds_transmap = transtables + ((tr_trans50-1)<<FF_TRANSSHIFT);
		spanfunc = R_DrawTranslucentSpan_8;
		for (i=0; i<4; i++)
		{
			xoffs = pl->xoffs;
			yoffs = pl->yoffs;

			switch(i)
			{
				case 0:
					xoffs -= FRACUNIT/4;
					yoffs -= FRACUNIT/4;
					break;
				case 1:
					xoffs -= FRACUNIT/4;
					yoffs += FRACUNIT/4;
					break;
				case 2:
					xoffs += FRACUNIT/4;
					yoffs -= FRACUNIT/4;
					break;
				case 3:
					xoffs += FRACUNIT/4;
					yoffs += FRACUNIT/4;
					break;
			}
			planeheight = abs(pl->height - pl->viewz);

			if (light >= LIGHTLEVELS)
				light = LIGHTLEVELS-1;

			if (light < 0)
				light = 0;

			planezlight = zlight[light];

			// set the maximum value for unsigned
			pl->top[pl->maxx+1] = 0xffff;
			pl->top[pl->minx-1] = 0xffff;
			pl->bottom[pl->maxx+1] = 0x0000;
			pl->bottom[pl->minx-1] = 0x0000;

			stop = pl->maxx + 1;

			for (x = pl->minx; x <= stop; x++)
				R_MakeSpans(x, pl->top[x-1], pl->bottom[x-1],
					pl->top[x], pl->bottom[x]);
		}
	}
#endif

	Z_ChangeTag(ds_source, PU_CACHE);
}

void R_PlaneBounds(visplane_t *plane)
{
	INT32 i;
	INT32 hi, low;

	hi = plane->top[plane->minx];
	low = plane->bottom[plane->minx];

	for (i = plane->minx + 1; i <= plane->maxx; i++)
	{
		if (plane->top[i] < hi)
		hi = plane->top[i];
		if (plane->bottom[i] > low)
		low = plane->bottom[i];
	}
	plane->high = hi;
	plane->low = low;
}
