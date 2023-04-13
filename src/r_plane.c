// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
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
#include "m_easing.h" // For Easing_InOutSine, used in R_UpdatePlaneRipple
#include "g_game.h"
#include "p_setup.h" // levelflats
#include "p_slopes.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_local.h"
#include "r_state.h"
#include "r_splats.h" // faB(21jan):testing
#include "r_sky.h"
#include "r_portal.h"

#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_tick.h"

//
// opening
//

//SoM: 3/23/2000: Use Boom visplane hashing.

visplane_t *visplanes[MAXVISPLANES];
static visplane_t *freetail;
static visplane_t **freehead = &freetail;

visplane_t *floorplane;
visplane_t *ceilingplane;
static visplane_t *currentplane;

visffloor_t ffloor[MAXFFLOORS];
INT32 numffloors;

//SoM: 3/23/2000: Boom visplane hashing routine.
#define visplane_hash(picnum,lightlevel,height) \
  ((unsigned)((picnum)*3+(lightlevel)+(height)*7) & VISPLANEHASHMASK)

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
fixed_t yslopetab[MAXVIDHEIGHT*16];
fixed_t *yslope;

fixed_t cachedheight[MAXVIDHEIGHT];
fixed_t cacheddistance[MAXVIDHEIGHT];
fixed_t cachedxstep[MAXVIDHEIGHT];
fixed_t cachedystep[MAXVIDHEIGHT];

static fixed_t xoffs, yoffs;
static floatv3_t ds_slope_origin, ds_slope_u, ds_slope_v;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes(void)
{
	// FIXME: unused
}

//
// Water ripple effect
// Needs the height of the plane, and the vertical position of the span.
// Sets planeripple.xfrac and planeripple.yfrac, added to ds_xfrac and ds_yfrac, if the span is not tilted.
//

static struct
{
	INT32 offset;
	fixed_t xfrac, yfrac;
	boolean active;
} planeripple;

// ripples da water texture
static fixed_t R_CalculateRippleOffset(INT32 y)
{
	fixed_t distance = FixedMul(planeheight, yslope[y]);
	const INT32 yay = (planeripple.offset + (distance>>9)) & 8191;
	return FixedDiv(FINESINE(yay), (1<<12) + (distance>>11));
}

static void R_CalculatePlaneRipple(angle_t angle)
{
	angle >>= ANGLETOFINESHIFT;
	angle = (angle + 2048) & 8191; // 90 degrees
	planeripple.xfrac = FixedMul(FINECOSINE(angle), ds_bgofs);
	planeripple.yfrac = FixedMul(FINESINE(angle), ds_bgofs);
}

static void R_UpdatePlaneRipple(void)
{
	// ds_waterofs oscillates between 0 and 16384 every other tic
	// Now that frame interpolation is a thing, HOW does it oscillate?
	// The difference between linear interpolation and a sine wave is miniscule here,
	// but a sine wave is ever so slightly smoother and sleeker
	ds_waterofs = Easing_InOutSine(((leveltime & 1)*FRACUNIT) + rendertimefrac,16384,0);

	// Meanwhile, planeripple.offset just counts up, so it gets simple linear interpolation
	planeripple.offset = ((leveltime-1)*140) + ((rendertimefrac*140) / FRACUNIT);
}

static void R_MapPlane(INT32 y, INT32 x1, INT32 x2)
{
	angle_t angle, planecos, planesin;
	fixed_t distance = 0, span;
	size_t pindex;

#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapPlane: %d, %d at %d", x1, x2, y);
#endif

	if (x1 >= vid.width)
		x1 = vid.width - 1;

	angle = (currentplane->viewangle + currentplane->plangle)>>ANGLETOFINESHIFT;
	planecos = FINECOSINE(angle);
	planesin = FINESINE(angle);

	if (planeheight != cachedheight[y])
	{
		cachedheight[y] = planeheight;
		cacheddistance[y] = distance = FixedMul(planeheight, yslope[y]);
		span = abs(centery - y);

		if (span) // Don't divide by zero
		{
			ds_xstep = FixedMul(planesin, planeheight) / span;
			ds_ystep = FixedMul(planecos, planeheight) / span;
		}
		else
			ds_xstep = ds_ystep = FRACUNIT;

		cachedxstep[y] = ds_xstep;
		cachedystep[y] = ds_ystep;
	}
	else
	{
		distance = cacheddistance[y];
		ds_xstep = cachedxstep[y];
		ds_ystep = cachedystep[y];
	}

	// [RH] Instead of using the xtoviewangle array, I calculated the fractional values
	// at the middle of the screen, then used the calculated ds_xstep and ds_ystep
	// to step from those to the proper texture coordinate to start drawing at.
	// That way, the texture coordinate is always calculated by its position
	// on the screen and not by its position relative to the edge of the visplane.
	ds_xfrac = xoffs + FixedMul(planecos, distance) + (x1 - centerx) * ds_xstep;
	ds_yfrac = yoffs - FixedMul(planesin, distance) + (x1 - centerx) * ds_ystep;

	// Water ripple effect
	if (planeripple.active)
	{
		ds_bgofs = R_CalculateRippleOffset(y);

		R_CalculatePlaneRipple(currentplane->viewangle + currentplane->plangle);

		ds_xfrac += planeripple.xfrac;
		ds_yfrac += planeripple.yfrac;
		ds_bgofs >>= FRACBITS;

		if ((y + ds_bgofs) >= viewheight)
			ds_bgofs = viewheight-y-1;
		if ((y + ds_bgofs) < 0)
			ds_bgofs = -y;
	}

	pindex = distance >> LIGHTZSHIFT;
	if (pindex >= MAXLIGHTZ)
		pindex = MAXLIGHTZ - 1;

	ds_colormap = planezlight[pindex];
	if (currentplane->extra_colormap)
		ds_colormap = currentplane->extra_colormap->colormap + (ds_colormap - colormaps);

	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;

	spanfunc();
}

static void R_MapTiltedPlane(INT32 y, INT32 x1, INT32 x2)
{
#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapTiltedPlane: %d, %d at %d", x1, x2, y);
#endif

	if (x1 >= vid.width)
		x1 = vid.width - 1;

	// Water ripple effect
	if (planeripple.active)
	{
		ds_bgofs = R_CalculateRippleOffset(y);

		ds_sup = &ds_su[y];
		ds_svp = &ds_sv[y];
		ds_szp = &ds_sz[y];

		ds_bgofs >>= FRACBITS;

		if ((y + ds_bgofs) >= viewheight)
			ds_bgofs = viewheight-y-1;
		if ((y + ds_bgofs) < 0)
			ds_bgofs = -y;
	}

	if (currentplane->extra_colormap)
		ds_colormap = currentplane->extra_colormap->colormap;
	else
		ds_colormap = colormaps;

	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;

	spanfunc();
}

static void R_MapFogPlane(INT32 y, INT32 x1, INT32 x2)
{
	fixed_t distance;
	size_t pindex;

#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapFogPlane: %d, %d at %d", x1, x2, y);
#endif

	if (x1 >= vid.width)
		x1 = vid.width - 1;

	if (planeheight != cachedheight[y])
		distance = FixedMul(planeheight, yslope[y]);
	else
		distance = cacheddistance[y];

	pindex = distance >> LIGHTZSHIFT;
	if (pindex >= MAXLIGHTZ)
		pindex = MAXLIGHTZ - 1;

	ds_colormap = planezlight[pindex];
	if (currentplane->extra_colormap)
		ds_colormap = currentplane->extra_colormap->colormap + (ds_colormap - colormaps);

	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;

	spanfunc();
}

static void R_MapTiltedFogPlane(INT32 y, INT32 x1, INT32 x2)
{
#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapTiltedFogPlane: %d, %d at %d", x1, x2, y);
#endif

	if (x1 >= vid.width)
		x1 = vid.width - 1;

	if (currentplane->extra_colormap)
		ds_colormap = currentplane->extra_colormap->colormap;
	else
		ds_colormap = colormaps;

	ds_y = y;
	ds_x1 = x1;
	ds_x2 = x2;

	spanfunc();
}

void R_ClearFFloorClips (void)
{
	INT32 i, p;

	// opening / clipping determination
	for (i = 0; i < viewwidth; i++)
	{
		for (p = 0; p < MAXFFLOORS; p++)
		{
			ffloor[p].f_clip[i] = (INT16)viewheight;
			ffloor[p].c_clip[i] = -1;
		}
	}

	numffloors = 0;
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes(void)
{
	INT32 i, p;

	// opening / clipping determination
	for (i = 0; i < viewwidth; i++)
	{
		floorclip[i] = (INT16)viewheight;
		ceilingclip[i] = -1;
		frontscale[i] = INT32_MAX;
		for (p = 0; p < MAXFFLOORS; p++)
		{
			ffloor[p].f_clip[i] = (INT16)viewheight;
			ffloor[p].c_clip[i] = -1;
		}
	}

	for (i = 0; i < MAXVISPLANES; i++)
	for (*freehead = visplanes[i], visplanes[i] = NULL;
		freehead && *freehead ;)
	{
		freehead = &(*freehead)->next;
	}

	lastopening = openings;

	// texture calculation
	memset(cachedheight, 0, sizeof (cachedheight));
}

static visplane_t *new_visplane(unsigned hash)
{
	visplane_t *check = freetail;
	if (!check)
	{
		check = malloc(sizeof (*check));
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
	ffloor_t *pfloor, polyobj_t *polyobj, pslope_t *slope)
{
	visplane_t *check;
	unsigned hash;

	if (!slope) // Don't mess with this right now if a slope is involved
	{
		xoff += viewx;
		yoff -= viewy;
		if (plangle != 0)
		{
			// Add the view offset, rotated by the plane angle.
			float ang = ANG2RAD(plangle);
			float x = FixedToFloat(xoff);
			float y = FixedToFloat(yoff);
			xoff = FloatToFixed(x * cos(ang) + y * sin(ang));
			yoff = FloatToFixed(-x * sin(ang) + y * cos(ang));
		}
	}

	if (polyobj)
	{
		if (polyobj->angle != 0)
		{
			float ang = ANG2RAD(polyobj->angle);
			float x = FixedToFloat(polyobj->centerPt.x);
			float y = FixedToFloat(polyobj->centerPt.y);
			xoff -= FloatToFixed(x * cos(ang) + y * sin(ang));
			yoff -= FloatToFixed(x * sin(ang) - y * cos(ang));
		}
		else
		{
			xoff -= polyobj->centerPt.x;
			yoff += polyobj->centerPt.y;
		}
	}

	// This appears to fix the Nimbus Ruins sky bug.
	if (picnum == skyflatnum && pfloor)
	{
		height = 0; // all skies map together
		lightlevel = 0;
	}

	if (!pfloor)
	{
		hash = visplane_hash(picnum, lightlevel, height);
		for (check = visplanes[hash]; check; check = check->next)
		{
			if (polyobj != check->polyobj)
				continue;
			if (height == check->height && picnum == check->picnum
				&& lightlevel == check->lightlevel
				&& xoff == check->xoffs && yoff == check->yoffs
				&& planecolormap == check->extra_colormap
				&& check->viewx == viewx && check->viewy == viewy && check->viewz == viewz
				&& check->viewangle == viewangle
				&& check->plangle == plangle
				&& check->slope == slope)
			{
				return check;
			}
		}
	}
	else
	{
		hash = MAXVISPLANES - 1;
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
	check->polyobj = polyobj;
	check->slope = slope;

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
		visplane_t *new_pl;
		if (pl->ffloor)
		{
			new_pl = new_visplane(MAXVISPLANES - 1);
		}
		else
		{
			unsigned hash =
				visplane_hash(pl->picnum, pl->lightlevel, pl->height);
			new_pl = new_visplane(hash);
		}

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
		new_pl->polyobj = pl->polyobj;
		new_pl->slope = pl->slope;
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
// This function basically expands the visplane.
// The reason for this is that when creating 3D floor planes, there is no
// need to create new ones with R_CheckPlane, because 3D floor planes
// are created by subsector and there is no way a subsector can graphically
// overlap.
void R_ExpandPlane(visplane_t *pl, INT32 start, INT32 stop)
{
	// Don't expand polyobject planes here - we do that on our own.
	if (pl->polyobj)
		return;

	if (pl->minx > start) pl->minx = start;
	if (pl->maxx < stop)  pl->maxx = stop;
}

static void R_MakeSpans(void (*mapfunc)(INT32, INT32, INT32), INT32 x, INT32 t1, INT32 b1, INT32 t2, INT32 b2)
{
	//    Alam: from r_splats's R_RasterizeFloorSplat
	if (t1 >= vid.height) t1 = vid.height-1;
	if (b1 >= vid.height) b1 = vid.height-1;
	if (t2 >= vid.height) t2 = vid.height-1;
	if (b2 >= vid.height) b2 = vid.height-1;
	if (x-1 >= vid.width) x = vid.width;

	while (t1 < t2 && t1 <= b1)
	{
		mapfunc(t1, spanstart[t1], x - 1);
		t1++;
	}
	while (b1 > b2 && b1 >= t1)
	{
		mapfunc(b1, spanstart[b1], x - 1);
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
	INT32 i;

	R_UpdatePlaneRipple();

	for (i = 0; i < MAXVISPLANES; i++, pl++)
	{
		for (pl = visplanes[i]; pl; pl = pl->next)
		{
			if (pl->ffloor != NULL || pl->polyobj != NULL)
				continue;

			R_DrawSinglePlane(pl);
		}
	}
}

// R_DrawSkyPlane
//
// Draws the sky within the plane's top/bottom bounds
// Note: this uses column drawers instead of span drawers, since the sky is always a texture
//
static void R_DrawSkyPlane(visplane_t *pl)
{
	INT32 x;
	INT32 angle;

	// Reset column drawer function (note: couldn't we just call walldrawerfunc directly?)
	// (that is, unless we'll need to switch drawers in future for some reason)
	colfunc = colfuncs[BASEDRAWFUNC];

	// use correct aspect ratio scale
	dc_iscale = skyscale;

	// Sky is always drawn full bright,
	//  i.e. colormaps[0] is used.
	// Because of this hack, sky is not affected
	//  by sector colormaps (INVUL inverse mapping is not implemented in SRB2 so is irrelevant).
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
			dc_iscale = FixedMul(skyscale, FINECOSINE(xtoviewangle[x]>>ANGLETOFINESHIFT));
			dc_x = x;
			dc_source =
				R_GetColumn(texturetranslation[skytexture],
					-angle); // get negative of angle for each column to display sky correct way round! --Monster Iestyn 27/01/18
			colfunc();
		}
	}
}

// Returns the height of the sloped plane at (x, y) as a 32.16 fixed_t
static INT64 R_GetSlopeZAt(const pslope_t *slope, fixed_t x, fixed_t y)
{
	INT64 x64 = ((INT64)x - (INT64)slope->o.x);
	INT64 y64 = ((INT64)y - (INT64)slope->o.y);

	x64 = (x64 * (INT64)slope->d.x) / FRACUNIT;
	y64 = (y64 * (INT64)slope->d.y) / FRACUNIT;

	return (INT64)slope->o.z + ((x64 + y64) * (INT64)slope->zdelta) / FRACUNIT;
}

// Sets the texture origin vector of the sloped plane.
static void R_SetSlopePlaneOrigin(pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xoff, fixed_t yoff, fixed_t angle)
{
	floatv3_t *p = &ds_slope_origin;

	INT64 vx = (INT64)xpos + (INT64)xoff;
	INT64 vy = (INT64)ypos - (INT64)yoff;

	float vxf = vx / (float)FRACUNIT;
	float vyf = vy / (float)FRACUNIT;
	float ang = ANG2RAD(ANGLE_270 - angle);

	// p is the texture origin in view space
	// Don't add in the offsets at this stage, because doing so can result in
	// errors if the flat is rotated.
	p->x = vxf * cos(ang) - vyf * sin(ang);
	p->z = vxf * sin(ang) + vyf * cos(ang);
	p->y = (R_GetSlopeZAt(slope, -xoff, yoff) - zpos) / (float)FRACUNIT;
}

// This function calculates all of the vectors necessary for drawing a sloped plane.
void R_SetSlopePlane(pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle)
{
	// Potentially override other stuff for now cus we're mean. :< But draw a slope plane!
	// I copied ZDoom's code and adapted it to SRB2... -Red
	floatv3_t *m = &ds_slope_v, *n = &ds_slope_u;
	fixed_t height, temp;
	float ang;

	R_SetSlopePlaneOrigin(slope, xpos, ypos, zpos, xoff, yoff, angle);
	height = P_GetSlopeZAt(slope, xpos, ypos);
	zeroheight = FixedToFloat(height - zpos);

	// m is the v direction vector in view space
	ang = ANG2RAD(ANGLE_180 - (angle + plangle));
	m->x = cos(ang);
	m->z = sin(ang);

	// n is the u direction vector in view space
	n->x = sin(ang);
	n->z = -cos(ang);

	plangle >>= ANGLETOFINESHIFT;
	temp = P_GetSlopeZAt(slope, xpos + FINESINE(plangle), ypos + FINECOSINE(plangle));
	m->y = FixedToFloat(temp - height);
	temp = P_GetSlopeZAt(slope, xpos + FINECOSINE(plangle), ypos - FINESINE(plangle));
	n->y = FixedToFloat(temp - height);
}

// This function calculates all of the vectors necessary for drawing a sloped and scaled plane.
void R_SetScaledSlopePlane(pslope_t *slope, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xs, fixed_t ys, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle)
{
	floatv3_t *m = &ds_slope_v, *n = &ds_slope_u;
	fixed_t height, temp;

	float xscale = FixedToFloat(xs);
	float yscale = FixedToFloat(ys);
	float ang;

	R_SetSlopePlaneOrigin(slope, xpos, ypos, zpos, xoff, yoff, angle);
	height = P_GetSlopeZAt(slope, xpos, ypos);
	zeroheight = FixedToFloat(height - zpos);

	// m is the v direction vector in view space
	ang = ANG2RAD(ANGLE_180 - (angle + plangle));
	m->x = yscale * cos(ang);
	m->z = yscale * sin(ang);

	// n is the u direction vector in view space
	n->x = xscale * sin(ang);
	n->z = -xscale * cos(ang);

	ang = ANG2RAD(plangle);
	temp = P_GetSlopeZAt(slope, xpos + FloatToFixed(yscale * sin(ang)), ypos + FloatToFixed(yscale * cos(ang)));
	m->y = FixedToFloat(temp - height);
	temp = P_GetSlopeZAt(slope, xpos + FloatToFixed(xscale * cos(ang)), ypos - FloatToFixed(xscale * sin(ang)));
	n->y = FixedToFloat(temp - height);
}

void R_CalculateSlopeVectors(void)
{
	float sfmult = 65536.f;

	// Eh. I tried making this stuff fixed-point and it exploded on me. Here's a macro for the only floating-point vector function I recall using.
#define CROSS(d, v1, v2) \
d->x = (v1.y * v2.z) - (v1.z * v2.y);\
d->y = (v1.z * v2.x) - (v1.x * v2.z);\
d->z = (v1.x * v2.y) - (v1.y * v2.x)
	CROSS(ds_sup, ds_slope_origin, ds_slope_v);
	CROSS(ds_svp, ds_slope_origin, ds_slope_u);
	CROSS(ds_szp, ds_slope_v, ds_slope_u);
#undef CROSS

	ds_sup->z *= focallengthf;
	ds_svp->z *= focallengthf;
	ds_szp->z *= focallengthf;

	if (ds_solidcolor)
		return;

	// Premultiply the texture vectors with the scale factors
	if (ds_powersoftwo)
		sfmult *= (1 << nflatshiftup);

	ds_sup->x *= sfmult;
	ds_sup->y *= sfmult;
	ds_sup->z *= sfmult;
	ds_svp->x *= sfmult;
	ds_svp->y *= sfmult;
	ds_svp->z *= sfmult;
}

void R_SetTiltedSpan(INT32 span)
{
	if (ds_su == NULL)
		ds_su = Z_Malloc(sizeof(*ds_su) * vid.height, PU_STATIC, NULL);
	if (ds_sv == NULL)
		ds_sv = Z_Malloc(sizeof(*ds_sv) * vid.height, PU_STATIC, NULL);
	if (ds_sz == NULL)
		ds_sz = Z_Malloc(sizeof(*ds_sz) * vid.height, PU_STATIC, NULL);

	ds_sup = &ds_su[span];
	ds_svp = &ds_sv[span];
	ds_szp = &ds_sz[span];
}

static void R_SetSlopePlaneVectors(visplane_t *pl, INT32 y, fixed_t xoff, fixed_t yoff)
{
	R_SetTiltedSpan(y);
	R_SetSlopePlane(pl->slope, pl->viewx, pl->viewy, pl->viewz, xoff, yoff, pl->viewangle, pl->plangle);
	R_CalculateSlopeVectors();
}

static inline void R_AdjustSlopeCoordinates(vector3_t *origin)
{
	const fixed_t modmask = ((1 << (32-nflatshiftup)) - 1);

	fixed_t ox = (origin->x & modmask);
	fixed_t oy = -(origin->y & modmask);

	xoffs &= modmask;
	yoffs &= modmask;

	xoffs -= (origin->x - ox);
	yoffs += (origin->y + oy);
}

static inline void R_AdjustSlopeCoordinatesNPO2(vector3_t *origin)
{
	const fixed_t modmaskw = (ds_flatwidth << FRACBITS);
	const fixed_t modmaskh = (ds_flatheight << FRACBITS);

	fixed_t ox = (origin->x % modmaskw);
	fixed_t oy = -(origin->y % modmaskh);

	xoffs %= modmaskw;
	yoffs %= modmaskh;

	xoffs -= (origin->x - ox);
	yoffs += (origin->y + oy);
}

void R_DrawSinglePlane(visplane_t *pl)
{
	INT32 light = 0;
	INT32 x, stop;
	ffloor_t *rover;
	boolean fog = false;
	INT32 spanfunctype = BASEDRAWFUNC;
	void (*mapfunc)(INT32, INT32, INT32);

	if (!(pl->minx <= pl->maxx))
		return;

	// sky flat
	if (pl->picnum == skyflatnum)
	{
		R_DrawSkyPlane(pl);
		return;
	}

	planeripple.active = false;

	if (pl->polyobj)
	{
		// Hacked up support for alpha value in software mode Tails 09-24-2002 (sidenote: ported to polys 10-15-2014, there was no time travel involved -Red)
		if (pl->polyobj->translucency >= 10)
			return; // Don't even draw it
		else if (pl->polyobj->translucency > 0)
		{
			spanfunctype = (pl->polyobj->flags & POF_SPLAT) ? SPANDRAWFUNC_TRANSSPLAT : SPANDRAWFUNC_TRANS;
			ds_transmap = R_GetTranslucencyTable(pl->polyobj->translucency);
		}
		else if (pl->polyobj->flags & POF_SPLAT) // Opaque, but allow transparent flat pixels
			spanfunctype = SPANDRAWFUNC_SPLAT;

		if (pl->polyobj->translucency == 0 || (pl->extra_colormap && (pl->extra_colormap->flags & CMF_FOG)))
			light = (pl->lightlevel >> LIGHTSEGSHIFT);
		else
			light = LIGHTLEVELS-1;
	}
	else
	{
		if (pl->ffloor)
		{
			// Don't draw planes that shouldn't be drawn.
			for (rover = pl->ffloor->target->ffloors; rover; rover = rover->next)
			{
				if ((pl->ffloor->fofflags & FOF_CUTEXTRA) && (rover->fofflags & FOF_EXTRA))
				{
					if (pl->ffloor->fofflags & FOF_EXTRA)
					{
						// The plane is from an extra 3D floor... Check the flags so
						// there are no undesired cuts.
						if (((pl->ffloor->fofflags & (FOF_FOG|FOF_SWIMMABLE)) == (rover->fofflags & (FOF_FOG|FOF_SWIMMABLE)))
							&& pl->height < *rover->topheight
							&& pl->height > *rover->bottomheight)
							return;
					}
				}
			}

			if (pl->ffloor->fofflags & FOF_TRANSLUCENT)
			{
				spanfunctype = (pl->ffloor->fofflags & FOF_SPLAT) ? SPANDRAWFUNC_TRANSSPLAT : SPANDRAWFUNC_TRANS;

				// Hacked up support for alpha value in software mode Tails 09-24-2002
				// ...unhacked by toaster 04-01-2021, re-hacked a little by sphere 19-11-2021
				{
					INT32 trans = (10*((256+12) - pl->ffloor->alpha))/255;
					if (trans >= 10)
						return; // Don't even draw it
					if (pl->ffloor->blend) // additive, (reverse) subtractive, modulative
						ds_transmap = R_GetBlendTable(pl->ffloor->blend, trans);
					else if (!(ds_transmap = R_GetTranslucencyTable(trans)) || trans == 0)
						spanfunctype = SPANDRAWFUNC_SPLAT; // Opaque, but allow transparent flat pixels
				}

				if ((spanfunctype == SPANDRAWFUNC_SPLAT) || (pl->extra_colormap && (pl->extra_colormap->flags & CMF_FOG)))
					light = (pl->lightlevel >> LIGHTSEGSHIFT);
				else
					light = LIGHTLEVELS-1;
			}
			else if (pl->ffloor->fofflags & FOF_FOG)
			{
				fog = true;
				spanfunctype = SPANDRAWFUNC_FOG;
				light = (pl->lightlevel >> LIGHTSEGSHIFT);
			}
			else light = (pl->lightlevel >> LIGHTSEGSHIFT);

			if (pl->ffloor->fofflags & FOF_RIPPLE && !fog)
			{
				planeripple.active = true;

				if (spanfunctype == SPANDRAWFUNC_TRANS)
				{
					// Copy the current scene, ugh
					INT32 top = pl->high-8;
					INT32 bottom = pl->low+8;

					if (top < 0)
						top = 0;
					if (bottom > vid.height)
						bottom = vid.height;

					spanfunctype = SPANDRAWFUNC_WATER;

					// Only copy the part of the screen we need
					VID_BlitLinearScreen((splitscreen && viewplayer == &players[secondarydisplayplayer]) ? screens[0] + (top+(vid.height>>1))*vid.width : screens[0]+((top)*vid.width), screens[1]+((top)*vid.width),
										 vid.width, bottom-top,
										 vid.width, vid.width);
				}
			}
		}
		else
			light = (pl->lightlevel >> LIGHTSEGSHIFT);
	}

	ds_powersoftwo = ds_solidcolor = false;

	if (fog)
	{
		// Since all fog planes do is apply a colormap, it's not required
		// to know any information about their textures.
		mapfunc = R_MapFogPlane;
	}
	else
	{
		levelflat_t *levelflat = &levelflats[pl->picnum];

		/* :james: */
		switch (levelflat->type)
		{
			case LEVELFLAT_NONE:
				return;
			case LEVELFLAT_FLAT:
				ds_source = (UINT8 *)R_GetFlat(levelflat->u.flat.lumpnum);
				R_SetFlatVars(W_LumpLength(levelflat->u.flat.lumpnum));
				if (R_CheckSolidColorFlat())
					ds_solidcolor = true;
				else
					ds_powersoftwo = true;
				break;
			default:
				ds_source = (UINT8 *)R_GetLevelFlat(levelflat);
				if (!ds_source)
					return;
				else if (R_CheckSolidColorFlat())
					ds_solidcolor = true;
				else if (R_CheckPowersOfTwo())
				{
					R_SetFlatVars(ds_flatwidth * ds_flatheight);
					ds_powersoftwo = true;
				}
		}

		// Don't mess with angle on slopes! We'll handle this ourselves later
		if (!pl->slope && viewangle != pl->viewangle+pl->plangle)
		{
			memset(cachedheight, 0, sizeof (cachedheight));
			viewangle = pl->viewangle+pl->plangle;
		}

		mapfunc = R_MapPlane;

		if (ds_solidcolor)
		{
			switch (spanfunctype)
			{
				case SPANDRAWFUNC_WATER:
					spanfunctype = SPANDRAWFUNC_WATERSOLID;
					break;
				case SPANDRAWFUNC_TRANS:
					spanfunctype = SPANDRAWFUNC_TRANSSOLID;
					break;
				default:
					spanfunctype = SPANDRAWFUNC_SOLID;
					break;
			}
		}
	}

	xoffs = pl->xoffs;
	yoffs = pl->yoffs;

	if (light >= LIGHTLEVELS)
		light = LIGHTLEVELS-1;

	if (light < 0)
		light = 0;

	if (pl->slope)
	{
		if (fog)
			mapfunc = R_MapTiltedFogPlane;
		else
		{
			mapfunc = R_MapTiltedPlane;

			if (!pl->plangle && !ds_solidcolor)
			{
				if (ds_powersoftwo)
					R_AdjustSlopeCoordinates(&pl->slope->o);
				else
					R_AdjustSlopeCoordinatesNPO2(&pl->slope->o);
			}
		}

		if (planeripple.active)
		{
			planeheight = abs(P_GetSlopeZAt(pl->slope, pl->viewx, pl->viewy) - pl->viewz);

			R_PlaneBounds(pl);

			for (x = pl->high; x < pl->low; x++)
			{
				ds_bgofs = R_CalculateRippleOffset(x);
				R_CalculatePlaneRipple(pl->viewangle + pl->plangle);
				R_SetSlopePlaneVectors(pl, x, (xoffs + planeripple.xfrac), (yoffs + planeripple.yfrac));
			}
		}
		else
			R_SetSlopePlaneVectors(pl, 0, xoffs, yoffs);

		switch (spanfunctype)
		{
			case SPANDRAWFUNC_WATER:
				spanfunctype = SPANDRAWFUNC_TILTEDWATER;
				break;
			case SPANDRAWFUNC_TRANS:
				spanfunctype = SPANDRAWFUNC_TILTEDTRANS;
				break;
			case SPANDRAWFUNC_SPLAT:
				spanfunctype = SPANDRAWFUNC_TILTEDSPLAT;
				break;
			case SPANDRAWFUNC_SOLID:
				spanfunctype = SPANDRAWFUNC_TILTEDSOLID;
				break;
			case SPANDRAWFUNC_TRANSSOLID:
				spanfunctype = SPANDRAWFUNC_TILTEDTRANSSOLID;
				break;
			case SPANDRAWFUNC_WATERSOLID:
				spanfunctype = SPANDRAWFUNC_TILTEDWATERSOLID;
				break;
			case SPANDRAWFUNC_FOG:
				spanfunctype = SPANDRAWFUNC_TILTEDFOG;
				break;
			default:
				spanfunctype = SPANDRAWFUNC_TILTED;
				break;
		}

		planezlight = scalelight[light];
	}
	else
	{
		planeheight = abs(pl->height - pl->viewz);
		planezlight = zlight[light];
	}

	// Set the span drawer
	if (!ds_powersoftwo)
	{
		if (spanfuncs_npo2[spanfunctype])
			spanfunc = spanfuncs_npo2[spanfunctype];
		else
			spanfunc = spanfuncs[spanfunctype];
	}
	else
		spanfunc = spanfuncs[spanfunctype];

	// set the maximum value for unsigned
	pl->top[pl->maxx+1] = 0xffff;
	pl->top[pl->minx-1] = 0xffff;
	pl->bottom[pl->maxx+1] = 0x0000;
	pl->bottom[pl->minx-1] = 0x0000;

	currentplane = pl;
	stop = pl->maxx + 1;

	for (x = pl->minx; x <= stop; x++)
		R_MakeSpans(mapfunc, x, pl->top[x-1], pl->bottom[x-1], pl->top[x], pl->bottom[x]);
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
