// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
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

// Quincunx antialiasing of flats!
//#define QUINCUNX

//SoM: 3/23/2000: Boom visplane hashing routine.
#define visplane_hash(picnum,lightlevel,height) \
  ((unsigned)((picnum)*3+(lightlevel)+(height)*7) & VISPLANEHASHMASK)

//added : 10-02-98: yslopetab is what yslope used to be,
//                yslope points somewhere into yslopetab,
//                now (viewheight/2) slopes are calculated above and
//                below the original viewheight for mouselook
//                (this is to calculate yslopes only when really needed)
//                (when mouselookin', yslope is moving into yslopetab)
//                Check R_SetupFrame, R_SetViewSize for more...
fixed_t yslopetab[MAXVIDHEIGHT*16];
fixed_t *yslope;

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

// ripples da water texture
static fixed_t R_CalculateRippleOffset(planecontext_t *planecontext, INT32 y)
{
	fixed_t distance = FixedMul(planecontext->planeheight, yslope[y]);
	const INT32 yay = (planecontext->ripple.offset + (distance>>9)) & 8191;
	return FixedDiv(FINESINE(yay), (1<<12) + (distance>>11));
}

static void R_CalculatePlaneRipple(planecontext_t *planecontext, angle_t angle, fixed_t bgofs)
{
	angle >>= ANGLETOFINESHIFT;
	angle = (angle + 2048) & 8191; // 90 degrees
	planecontext->ripple.xfrac = FixedMul(FINECOSINE(angle), bgofs);
	planecontext->ripple.yfrac = FixedMul(FINESINE(angle), bgofs);
}

static void R_UpdatePlaneRipple(rendercontext_t *context)
{
	context->spancontext.waterofs = (leveltime & 1)*16384;
	context->planecontext.ripple.offset = (leveltime * 140);
}

static void R_MapPlane(planecontext_t *planecontext, spancontext_t *ds, INT32 y, INT32 x1, INT32 x2)
{
	angle_t angle, planecos, planesin;
	fixed_t distance = 0, span, planeheight = planecontext->planeheight;
	visplane_t *curplane = planecontext->currentplane;
	size_t pindex;

#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapPlane: %d, %d at %d", x1, x2, y);
#endif

	if (x1 >= vid.width)
		x1 = vid.width - 1;

	angle = (curplane->viewangle + curplane->plangle)>>ANGLETOFINESHIFT;
	planecos = FINECOSINE(angle);
	planesin = FINESINE(angle);

	if (planeheight != planecontext->cachedheight[y])
	{
		planecontext->cachedheight[y] = planeheight;
		planecontext->cacheddistance[y] = distance = FixedMul(planeheight, yslope[y]);
		span = abs(centery - y);

		if (span) // Don't divide by zero
		{
			ds->xstep = FixedMul(planesin, planeheight) / span;
			ds->ystep = FixedMul(planecos, planeheight) / span;
		}
		else
			ds->xstep = ds->ystep = FRACUNIT;

		planecontext->cachedxstep[y] = ds->xstep;
		planecontext->cachedystep[y] = ds->ystep;
	}
	else
	{
		distance = planecontext->cacheddistance[y];
		ds->xstep = planecontext->cachedxstep[y];
		ds->ystep = planecontext->cachedystep[y];
	}

	// [RH] Instead of using the xtoviewangle array, I calculated the fractional values
	// at the middle of the screen, then used the calculated ds->xstep and ds->ystep
	// to step from those to the proper texture coordinate to start drawing at.
	// That way, the texture coordinate is always calculated by its position
	// on the screen and not by its position relative to the edge of the visplane.
	ds->xfrac = planecontext->xoffs + FixedMul(planecos, distance) + (x1 - centerx) * ds->xstep;
	ds->yfrac = planecontext->yoffs - FixedMul(planesin, distance) + (x1 - centerx) * ds->ystep;

	// Water ripple effect
	if (planecontext->ripple.active)
	{
		ds->bgofs = R_CalculateRippleOffset(planecontext, y);

		R_CalculatePlaneRipple(planecontext, curplane->viewangle + curplane->plangle, ds->bgofs);

		ds->xfrac += planecontext->ripple.xfrac;
		ds->yfrac += planecontext->ripple.yfrac;
		ds->bgofs >>= FRACBITS;

		if ((y + ds->bgofs) >= viewheight)
			ds->bgofs = viewheight-y-1;
		if ((y + ds->bgofs) < 0)
			ds->bgofs = -y;
	}

	pindex = distance >> LIGHTZSHIFT;
	if (pindex >= MAXLIGHTZ)
		pindex = MAXLIGHTZ - 1;

	ds->colormap = planecontext->zlight[pindex];
	if (curplane->extra_colormap)
		ds->colormap = curplane->extra_colormap->colormap + (ds->colormap - colormaps);

	ds->y = y;
	ds->x1 = x1;
	ds->x2 = x2;

	ds->func(ds);
}

static void R_MapTiltedPlane(planecontext_t *planecontext, spancontext_t *ds, INT32 y, INT32 x1, INT32 x2)
{
	visplane_t *curplane = planecontext->currentplane;

#ifdef RANGECHECK
	if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
		I_Error("R_MapTiltedPlane: %d, %d at %d", x1, x2, y);
#endif

	if (x1 >= vid.width)
		x1 = vid.width - 1;

	// Water ripple effect
	if (planecontext->ripple.active)
	{
		ds->bgofs = R_CalculateRippleOffset(planecontext, y);

		ds->sup = &ds->su[y];
		ds->svp = &ds->sv[y];
		ds->szp = &ds->sz[y];

		ds->bgofs >>= FRACBITS;

		if ((y + ds->bgofs) >= viewheight)
			ds->bgofs = viewheight-y-1;
		if ((y + ds->bgofs) < 0)
			ds->bgofs = -y;
	}

	ds->zlight = planecontext->zlight;

	if (curplane->extra_colormap)
		ds->colormap = curplane->extra_colormap->colormap;
	else
		ds->colormap = colormaps;

	ds->y = y;
	ds->x1 = x1;
	ds->x2 = x2;

	ds->func(ds);
}

void R_ClearFFloorClips(planecontext_t *planecontext)
{
	INT32 i, p;

	// opening / clipping determination
	for (i = 0; i < viewwidth; i++)
	{
		for (p = 0; p < MAXFFLOORS; p++)
		{
			planecontext->ffloor[p].f_clip[i] = (INT16)viewheight;
			planecontext->ffloor[p].c_clip[i] = -1;
		}
	}

	planecontext->numffloors = 0;
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes(planecontext_t *planecontext)
{
	INT32 i, p;

	// opening / clipping determination
	for (i = 0; i < viewwidth; i++)
	{
		planecontext->floorclip[i] = (INT16)viewheight;
		planecontext->ceilingclip[i] = -1;
		planecontext->frontscale[i] = INT32_MAX;
		for (p = 0; p < MAXFFLOORS; p++)
		{
			planecontext->ffloor[p].f_clip[i] = (INT16)viewheight;
			planecontext->ffloor[p].c_clip[i] = -1;
		}
	}

	for (i = 0; i < MAXVISPLANES; i++)
	for (*(planecontext->freehead) = planecontext->visplanes[i], planecontext->visplanes[i] = NULL;
		planecontext->freehead && *(planecontext->freehead) ;)
	{
		planecontext->freehead = &(*(planecontext->freehead))->next;
	}

	planecontext->lastopening = planecontext->openings;

	memset(planecontext->cachedheight, 0, sizeof(planecontext->cachedheight));
}

static visplane_t *new_visplane(planecontext_t *planecontext, unsigned hash)
{
	visplane_t *check = planecontext->freetail;
	if (!check)
	{
		check = calloc(2, sizeof (*check));
		if (check == NULL) I_Error("%s: Out of memory", "new_visplane"); // FIXME: ugly
	}
	else
	{
		planecontext->freetail = planecontext->freetail->next;
		if (!planecontext->freetail)
			planecontext->freehead = &planecontext->freetail;
	}
	check->next = planecontext->visplanes[hash];
	planecontext->visplanes[hash] = check;
	return check;
}

//
// R_FindPlane: Seek a visplane having the identical values:
//              Same height, same flattexture, same lightlevel.
//              If not, allocates another of them.
//
visplane_t *R_FindPlane(planecontext_t *planecontext, viewcontext_t *viewcontext,
	fixed_t height, INT32 picnum, INT32 lightlevel,
	fixed_t xoff, fixed_t yoff, angle_t plangle, extracolormap_t *planecolormap,
	ffloor_t *pfloor, polyobj_t *polyobj, pslope_t *slope)
{
	visplane_t *check;
	unsigned hash;

	if (!slope) // Don't mess with this right now if a slope is involved
	{
		xoff += viewcontext->x;
		yoff -= viewcontext->y;

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
		for (check = planecontext->visplanes[hash]; check; check = check->next)
		{
			if (polyobj != check->polyobj)
				continue;
			if (height == check->height && picnum == check->picnum
				&& lightlevel == check->lightlevel
				&& xoff == check->xoffs && yoff == check->yoffs
				&& planecolormap == check->extra_colormap
				&& check->viewx == viewcontext->x && check->viewy == viewcontext->y && check->viewz == viewcontext->z
				&& check->viewangle == viewcontext->angle
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

	check = new_visplane(planecontext, hash);

	check->height = height;
	check->picnum = picnum;
	check->lightlevel = lightlevel;
	check->minx = vid.width;
	check->maxx = -1;
	check->xoffs = xoff;
	check->yoffs = yoff;
	check->extra_colormap = planecolormap;
	check->ffloor = pfloor;
	check->viewx = viewcontext->x;
	check->viewy = viewcontext->y;
	check->viewz = viewcontext->z;
	check->viewangle = viewcontext->angle;
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
visplane_t *R_CheckPlane(planecontext_t *planecontext, visplane_t *pl, INT32 start, INT32 stop)
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
			new_pl = new_visplane(planecontext, MAXVISPLANES - 1);
		}
		else
		{
			unsigned hash =
				visplane_hash(pl->picnum, pl->lightlevel, pl->height);
			new_pl = new_visplane(planecontext, hash);
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

typedef void (*R_MapFunc) (planecontext_t *, spancontext_t *, INT32, INT32, INT32);

static void R_MakeSpans(planecontext_t *planecontext, spancontext_t *spancontext, R_MapFunc mapfunc, INT32 x, INT32 t1, INT32 b1, INT32 t2, INT32 b2)
{
	//    Alam: from r_splats's R_RasterizeFloorSplat
	if (t1 >= vid.height) t1 = vid.height-1;
	if (b1 >= vid.height) b1 = vid.height-1;
	if (t2 >= vid.height) t2 = vid.height-1;
	if (b2 >= vid.height) b2 = vid.height-1;
	if (x-1 >= vid.width) x = vid.width;

	while (t1 < t2 && t1 <= b1)
	{
		mapfunc(planecontext, spancontext, t1, planecontext->spanstart[t1], x - 1);
		t1++;
	}
	while (b1 > b2 && b1 >= t1)
	{
		mapfunc(planecontext, spancontext, b1, planecontext->spanstart[b1], x - 1);
		b1--;
	}

	while (t2 < t1 && t2 <= b2)
		planecontext->spanstart[t2++] = x;
	while (b2 > b1 && b2 >= t2)
		planecontext->spanstart[b2--] = x;
}

void R_DrawPlanes(rendercontext_t *context)
{
	visplane_t *pl;
	INT32 i;

	R_UpdatePlaneRipple(context);

	for (i = 0; i < MAXVISPLANES; i++, pl++)
	{
		for (pl = context->planecontext.visplanes[i]; pl; pl = pl->next)
		{
			if (pl->ffloor != NULL || pl->polyobj != NULL)
				continue;

			R_DrawSinglePlane(context, pl);
		}
	}
}

// R_DrawSkyPlane
//
// Draws the sky within the plane's top/bottom bounds
// Note: this uses column drawers instead of span drawers, since the sky is always a texture
//
static void R_DrawSkyPlane(colcontext_t *dc, visplane_t *pl)
{
	INT32 x;

	// Reset column drawer function (note: couldn't we just call (colfuncs[BASEDRAWFUNC])() directly?)
	// (that is, unless we'll need to switch drawers in future for some reason)
	dc->func = colfuncs[BASEDRAWFUNC];

	// use correct aspect ratio scale
	dc->iscale = skyscale;

	// Sky is always drawn full bright,
	//  i.e. colormaps[0] is used.
	dc->colormap = colormaps;
	dc->texturemid = skytexturemid;
	dc->texheight = textureheight[skytexture]>>FRACBITS;

	for (x = pl->minx; x <= pl->maxx; x++)
	{
		dc->yl = pl->top[x];
		dc->yh = pl->bottom[x];

		if (dc->yl <= dc->yh)
		{
			INT32 angle = (pl->viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
			dc->iscale = FixedMul(skyscale, FINECOSINE(xtoviewangle[x]>>ANGLETOFINESHIFT));
			dc->x = x;

			// get negative of angle for each column to display sky correct
			// way round! --Monster Iestyn 27/01/18
			dc->source = R_GetColumn(texturetranslation[skytexture], -angle); 
			dc->func(dc);
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
static void R_SetSlopePlaneOrigin(pslope_t *slope, spancontext_t *ds, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xoff, fixed_t yoff, fixed_t angle)
{
	floatv3_t *p = &ds->slope_origin;

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
void R_SetSlopePlane(pslope_t *slope, spancontext_t *ds, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle)
{
	// Potentially override other stuff for now cus we're mean. :< But draw a slope plane!
	// I copied ZDoom's code and adapted it to SRB2... -Red
	floatv3_t *m = &ds->slope_v, *n = &ds->slope_u;
	fixed_t height, temp;
	float ang;

	R_SetSlopePlaneOrigin(slope, ds, xpos, ypos, zpos, xoff, yoff, angle);
	height = P_GetSlopeZAt(slope, xpos, ypos);
	ds->zeroheight = FixedToFloat(height - zpos);

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
void R_SetScaledSlopePlane(pslope_t *slope, spancontext_t *ds, fixed_t xpos, fixed_t ypos, fixed_t zpos, fixed_t xs, fixed_t ys, fixed_t xoff, fixed_t yoff, angle_t angle, angle_t plangle)
{
	floatv3_t *m = &ds->slope_v, *n = &ds->slope_u;
	fixed_t height, temp;

	float xscale = FixedToFloat(xs);
	float yscale = FixedToFloat(ys);
	float ang;

	R_SetSlopePlaneOrigin(slope, ds, xpos, ypos, zpos, xoff, yoff, angle);
	height = P_GetSlopeZAt(slope, xpos, ypos);
	ds->zeroheight = FixedToFloat(height - zpos);

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

void R_CalculateSlopeVectors(spancontext_t *ds)
{
	float sfmult = 65536.f;

	// Eh. I tried making this stuff fixed-point and it exploded on me. Here's a macro for the only floating-point vector function I recall using.
#define CROSS(d, v1, v2) \
d->x = (v1.y * v2.z) - (v1.z * v2.y);\
d->y = (v1.z * v2.x) - (v1.x * v2.z);\
d->z = (v1.x * v2.y) - (v1.y * v2.x)
	CROSS(ds->sup, ds->slope_origin, ds->slope_v);
	CROSS(ds->svp, ds->slope_origin, ds->slope_u);
	CROSS(ds->szp, ds->slope_v, ds->slope_u);
#undef CROSS

	ds->sup->z *= focallengthf;
	ds->svp->z *= focallengthf;
	ds->szp->z *= focallengthf;

	// Premultiply the texture vectors with the scale factors
	if (ds->powersoftwo)
		sfmult *= (1 << ds->nflatshiftup);

	ds->sup->x *= sfmult;
	ds->sup->y *= sfmult;
	ds->sup->z *= sfmult;
	ds->svp->x *= sfmult;
	ds->svp->y *= sfmult;
	ds->svp->z *= sfmult;
}

void R_SetTiltedSpan(spancontext_t *ds, INT32 span)
{
	if (ds->su == NULL)
		ds->su = Z_Malloc(sizeof(*ds->su) * vid.height, PU_STATIC, NULL);
	if (ds->sv == NULL)
		ds->sv = Z_Malloc(sizeof(*ds->sv) * vid.height, PU_STATIC, NULL);
	if (ds->sz == NULL)
		ds->sz = Z_Malloc(sizeof(*ds->sz) * vid.height, PU_STATIC, NULL);

	ds->sup = &ds->su[span];
	ds->svp = &ds->sv[span];
	ds->szp = &ds->sz[span];
}

static void R_SetSlopePlaneVectors(visplane_t *pl, spancontext_t *ds, INT32 y, fixed_t xoff, fixed_t yoff)
{
	R_SetTiltedSpan(ds, y);
	R_SetSlopePlane(pl->slope, ds, pl->viewx, pl->viewy, pl->viewz, xoff, yoff, pl->viewangle, pl->plangle);
	R_CalculateSlopeVectors(ds);
}

static inline void R_AdjustSlopeCoordinates(planecontext_t *planecontext, INT32 flatshiftup, vector3_t *origin)
{
	const fixed_t modmask = ((1 << (32-flatshiftup)) - 1);

	fixed_t ox = (origin->x & modmask);
	fixed_t oy = -(origin->y & modmask);

	planecontext->xoffs &= modmask;
	planecontext->yoffs &= modmask;

	planecontext->xoffs -= (origin->x - ox);
	planecontext->yoffs += (origin->y + oy);
}

static inline void R_AdjustSlopeCoordinatesNPO2(planecontext_t *planecontext, fixed_t flatwidth, fixed_t flatheight, vector3_t *origin)
{
	const fixed_t modmaskw = (flatwidth << FRACBITS);
	const fixed_t modmaskh = (flatheight << FRACBITS);

	fixed_t ox = (origin->x % modmaskw);
	fixed_t oy = -(origin->y % modmaskh);

	planecontext->xoffs %= modmaskw;
	planecontext->yoffs %= modmaskh;

	planecontext->xoffs -= (origin->x - ox);
	planecontext->yoffs += (origin->y + oy);
}

void R_DrawSinglePlane(rendercontext_t *context, visplane_t *pl)
{
	levelflat_t *levelflat;
	INT32 light = 0;
	INT32 x, stop;
	ffloor_t *rover;
	INT32 spanfunctype = BASEDRAWFUNC;
	R_MapFunc mapfunc = R_MapPlane;

	planecontext_t *planecontext = &context->planecontext;
	spancontext_t *ds = &context->spancontext;

	if (!(pl->minx <= pl->maxx))
		return;

	// sky flat
	if (pl->picnum == skyflatnum)
	{
		R_DrawSkyPlane(&context->colcontext, pl);
		return;
	}

	planecontext->ripple.active = false;

	ds->func = spanfuncs[BASEDRAWFUNC];

	if (pl->polyobj)
	{
		// Hacked up support for alpha value in software mode Tails 09-24-2002 (sidenote: ported to polys 10-15-2014, there was no time travel involved -Red)
		if (pl->polyobj->translucency >= 10)
			return; // Don't even draw it
		else if (pl->polyobj->translucency > 0)
		{
			spanfunctype = (pl->polyobj->flags & POF_SPLAT) ? SPANDRAWFUNC_TRANSSPLAT : SPANDRAWFUNC_TRANS;
			ds->transmap = R_GetTranslucencyTable(pl->polyobj->translucency);
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
				spanfunctype = (pl->ffloor->master->flags & ML_EFFECT6) ? SPANDRAWFUNC_TRANSSPLAT : SPANDRAWFUNC_TRANS;

				// Hacked up support for alpha value in software mode Tails 09-24-2002
				if (pl->ffloor->alpha < 12)
					return; // Don't even draw it
				else if (pl->ffloor->alpha < 38)
					ds->transmap = R_GetTranslucencyTable(tr_trans90);
				else if (pl->ffloor->alpha < 64)
					ds->transmap = R_GetTranslucencyTable(tr_trans80);
				else if (pl->ffloor->alpha < 89)
					ds->transmap = R_GetTranslucencyTable(tr_trans70);
				else if (pl->ffloor->alpha < 115)
					ds->transmap = R_GetTranslucencyTable(tr_trans60);
				else if (pl->ffloor->alpha < 140)
					ds->transmap = R_GetTranslucencyTable(tr_trans50);
				else if (pl->ffloor->alpha < 166)
					ds->transmap = R_GetTranslucencyTable(tr_trans40);
				else if (pl->ffloor->alpha < 192)
					ds->transmap = R_GetTranslucencyTable(tr_trans30);
				else if (pl->ffloor->alpha < 217)
					ds->transmap = R_GetTranslucencyTable(tr_trans20);
				else if (pl->ffloor->alpha < 243)
					ds->transmap = R_GetTranslucencyTable(tr_trans10);
				else // Opaque, but allow transparent flat pixels
					spanfunctype = SPANDRAWFUNC_SPLAT;

				if ((spanfunctype == SPANDRAWFUNC_SPLAT) || (pl->extra_colormap && (pl->extra_colormap->flags & CMF_FOG)))
					light = (pl->lightlevel >> LIGHTSEGSHIFT);
				else
					light = LIGHTLEVELS-1;
			}
			else if (pl->ffloor->flags & FF_FOG)
			{
				spanfunctype = SPANDRAWFUNC_FOG;
				light = (pl->lightlevel >> LIGHTSEGSHIFT);
			}
			else light = (pl->lightlevel >> LIGHTSEGSHIFT);

			if (pl->ffloor->flags & FF_RIPPLE)
			{
				planecontext->ripple.active = true;

				if (spanfunctype == SPANDRAWFUNC_TRANS)
				{
					// Copy the current scene, ugh
					INT32 top = pl->high-8;
					INT32 bottom = pl->low+8;

					spanfunctype = SPANDRAWFUNC_WATER;

					if (top < 0)
						top = 0;
					if (bottom > vid.height)
						bottom = vid.height;

					// Only copy the part of the screen we need
					VID_BlitLinearScreen((splitscreen && context->viewcontext.player == &players[secondarydisplayplayer]) ? screens[0] + (top+(vid.height>>1))*vid.width : screens[0]+((top)*vid.width), screens[1]+((top)*vid.width),
										 vid.width, bottom-top,
										 vid.width, vid.width);
				}
			}
		}
		else
			light = (pl->lightlevel >> LIGHTSEGSHIFT);
	}

	planecontext->currentplane = pl;
	levelflat = &levelflats[pl->picnum];

	/* :james: */
	switch (levelflat->type)
	{
		case LEVELFLAT_NONE:
			return;
		case LEVELFLAT_FLAT:
			ds->source = (UINT8 *)R_GetFlat(levelflat->u.flat.lumpnum);
			R_CheckFlatLength(ds, W_LumpLength(levelflat->u.flat.lumpnum));
			// Raw flats always have dimensions that are powers-of-two numbers.
			ds->powersoftwo = true;
			break;
		default:
			ds->source = (UINT8 *)R_GetLevelFlat(levelflat, &ds->flatwidth, &ds->flatheight);
			if (!ds->source)
				return;
			// Check if this texture or patch has power-of-two dimensions.
			if (R_CheckPowersOfTwo(ds->flatwidth, ds->flatheight))
				R_CheckFlatLength(ds, ds->flatwidth * ds->flatheight);
	}

	if (!pl->slope // Don't mess with angle on slopes! We'll handle this ourselves later
		&& context->viewcontext.angle != pl->viewangle+pl->plangle)
	{
		memset(planecontext->cachedheight, 0, sizeof(planecontext->cachedheight));
		context->viewcontext.angle = pl->viewangle+pl->plangle;
	}

	planecontext->xoffs = pl->xoffs;
	planecontext->yoffs = pl->yoffs;

	if (light >= LIGHTLEVELS)
		light = LIGHTLEVELS-1;

	if (light < 0)
		light = 0;

	if (pl->slope)
	{
		mapfunc = R_MapTiltedPlane;

		if (!pl->plangle)
		{
			if (ds->powersoftwo)
				R_AdjustSlopeCoordinates(planecontext, ds->nflatshiftup, &pl->slope->o);
			else
				R_AdjustSlopeCoordinatesNPO2(planecontext, ds->flatwidth, ds->flatheight, &pl->slope->o);
		}

		if (planecontext->ripple.active)
		{
			planecontext->planeheight = abs(P_GetSlopeZAt(pl->slope, pl->viewx, pl->viewy) - pl->viewz);

			R_PlaneBounds(pl);

			for (x = pl->high; x < pl->low; x++)
			{
				ds->bgofs = R_CalculateRippleOffset(planecontext, x);
				R_CalculatePlaneRipple(planecontext, pl->viewangle + pl->plangle, ds->bgofs);
				R_SetSlopePlaneVectors(pl, ds, x, (planecontext->xoffs + planecontext->ripple.xfrac), (planecontext->yoffs + planecontext->ripple.yfrac));
			}
		}
		else
			R_SetSlopePlaneVectors(pl, ds, 0, planecontext->xoffs, planecontext->yoffs);

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
			default:
				spanfunctype = SPANDRAWFUNC_TILTED;
				break;
		}

		planecontext->zlight = scalelight[light];
	}
	else
	{
		planecontext->planeheight = abs(pl->height - pl->viewz);
		planecontext->zlight = zlight[light];
	}

	// Use the correct span drawer depending on the powers-of-twoness
	if (!ds->powersoftwo)
	{
		if (spanfuncs_npo2[spanfunctype])
			ds->func = spanfuncs_npo2[spanfunctype];
		else
			ds->func = spanfuncs[spanfunctype];
	}
	else
		ds->func = spanfuncs[spanfunctype];

	// set the maximum value for unsigned
	pl->top[pl->maxx+1] = 0xffff;
	pl->top[pl->minx-1] = 0xffff;
	pl->bottom[pl->maxx+1] = 0x0000;
	pl->bottom[pl->minx-1] = 0x0000;

	stop = pl->maxx + 1;

	for (x = pl->minx; x <= stop; x++)
		R_MakeSpans(planecontext, ds, mapfunc, x, pl->top[x-1], pl->bottom[x-1], pl->top[x], pl->bottom[x]);

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
	if (ds->func == spanfuncs[BASEDRAWFUNC])
	{
		INT32 i;
		ds->transmap = R_GetTranslucencyTable(tr_trans50);
		ds->func = spanfuncs[SPANDRAWFUNC_TRANS];
		for (i=0; i<4; i++)
		{
			planecontext->xoffs = pl->xoffs;
			planecontext->yoffs = pl->yoffs;

			switch(i)
			{
				case 0:
					planecontext->xoffs -= FRACUNIT/4;
					planecontext->yoffs -= FRACUNIT/4;
					break;
				case 1:
					planecontext->xoffs -= FRACUNIT/4;
					planecontext->yoffs += FRACUNIT/4;
					break;
				case 2:
					planecontext->xoffs += FRACUNIT/4;
					planecontext->yoffs -= FRACUNIT/4;
					break;
				case 3:
					planecontext->xoffs += FRACUNIT/4;
					planecontext->yoffs += FRACUNIT/4;
					break;
			}
			planecontext->planeheight = abs(pl->height - pl->viewz);

			if (light >= LIGHTLEVELS)
				light = LIGHTLEVELS-1;

			if (light < 0)
				light = 0;

			planecontext->zlight = zlight[light];

			// set the maximum value for unsigned
			pl->top[pl->maxx+1] = 0xffff;
			pl->top[pl->minx-1] = 0xffff;
			pl->bottom[pl->maxx+1] = 0x0000;
			pl->bottom[pl->minx-1] = 0x0000;

			stop = pl->maxx + 1;

			for (x = pl->minx; x <= stop; x++)
				R_MakeSpans(planecontext, ds, mapfunc, x, pl->top[x-1], pl->bottom[x-1],
					pl->top[x], pl->bottom[x]);
		}
	}
#endif
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
