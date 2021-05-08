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

#ifdef TIMING
#include "p5prof.h"
	INT64 mycount;
	INT64 mytotal = 0;
	UINT32 nombre = 100000;
#endif

//
// opening
//

// Quincunx antialiasing of flats!
//#define QUINCUNX

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

//
// Water ripple effect
// Needs the height of the plane, and the vertical position of the span.
// Sets planeripple.xfrac and planeripple.yfrac, added to ds_xfrac and ds_yfrac, if the span is not tilted.
//

struct
{
	INT32 offset;
	fixed_t xfrac, yfrac;
	boolean active;
} planeripple;

static void R_CalculatePlaneRipple(visplane_t *plane, INT32 y, fixed_t plheight, boolean calcfrac)
{
	fixed_t distance = FixedMul(plheight, yslope[y]);
	const INT32 yay = (planeripple.offset + (distance>>9)) & 8191;

	// ripples da water texture
	ds_bgofs = FixedDiv(FINESINE(yay), (1<<12) + (distance>>11))>>FRACBITS;

	if (calcfrac)
	{
		angle_t angle = (plane->viewangle + plane->plangle)>>ANGLETOFINESHIFT;
		angle = (angle + 2048) & 8191; // 90 degrees
		planeripple.xfrac = FixedMul(FINECOSINE(angle), (ds_bgofs<<FRACBITS));
		planeripple.yfrac = FixedMul(FINESINE(angle), (ds_bgofs<<FRACBITS));
	}
}

static void R_UpdatePlaneRipple(void)
{
	ds_waterofs = (leveltime & 1)*16384;
	planeripple.offset = (leveltime * 140);
}

//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  basexscale
//  baseyscale
//  centerx

void R_MapPlane(INT32 y, INT32 x1, INT32 x2)
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

	if (!currentplane->slope)
	{
		angle = (currentplane->viewangle + currentplane->plangle)>>ANGLETOFINESHIFT;
		planecos = FINECOSINE(angle);
		planesin = FINESINE(angle);

		if (planeheight != cachedheight[y])
		{
			cachedheight[y] = planeheight;
			cacheddistance[y] = distance = FixedMul(planeheight, yslope[y]);
			span = abs(centery - y);

			if (span) // don't divide by zero
			{
				ds_xstep = FixedMul(planesin, planeheight) / span;
				ds_ystep = FixedMul(planecos, planeheight) / span;
			}
			else
			{
				ds_xstep = FixedMul(distance, basexscale);
				ds_ystep = FixedMul(distance, baseyscale);
			}

			cachedxstep[y] = ds_xstep;
			cachedystep[y] = ds_ystep;
		}
		else
		{
			distance = cacheddistance[y];
			ds_xstep = cachedxstep[y];
			ds_ystep = cachedystep[y];
		}

		ds_xfrac = xoffs + FixedMul(planecos, distance) + (x1 - centerx) * ds_xstep;
		ds_yfrac = yoffs - FixedMul(planesin, distance) + (x1 - centerx) * ds_ystep;
	}

	// Water ripple effect
	if (planeripple.active)
	{
		// Needed for ds_bgofs
		R_CalculatePlaneRipple(currentplane, y, planeheight, (!currentplane->slope));

		if (currentplane->slope)
		{
			ds_sup = &ds_su[y];
			ds_svp = &ds_sv[y];
			ds_szp = &ds_sz[y];
		}
		else
		{
			ds_xfrac += planeripple.xfrac;
			ds_yfrac += planeripple.yfrac;
		}

		if ((y + ds_bgofs) >= viewheight)
			ds_bgofs = viewheight-y-1;
		if ((y + ds_bgofs) < 0)
			ds_bgofs = -y;
	}

	if (currentplane->slope)
		ds_colormap = colormaps;
	else
	{
		pindex = distance >> LIGHTZSHIFT;
		if (pindex >= MAXLIGHTZ)
			pindex = MAXLIGHTZ - 1;
		ds_colormap = planezlight[pindex];
	}

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
	angle_t angle;

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
			fixed_t cosinecomponent = FINECOSINE(plangle>>ANGLETOFINESHIFT);
			fixed_t sinecomponent = FINESINE(plangle>>ANGLETOFINESHIFT);
			fixed_t oldxoff = xoff;
			xoff = FixedMul(xoff,cosinecomponent)+FixedMul(yoff,sinecomponent);
			yoff = -FixedMul(oldxoff,sinecomponent)+FixedMul(yoff,cosinecomponent);
		}
	}

	if (polyobj)
	{
		if (polyobj->angle != 0)
		{
			angle_t fineshift = polyobj->angle >> ANGLETOFINESHIFT;
			xoff -= FixedMul(FINECOSINE(fineshift), polyobj->centerPt.x)+FixedMul(FINESINE(fineshift), polyobj->centerPt.y);
			yoff -= FixedMul(FINESINE(fineshift), polyobj->centerPt.x)-FixedMul(FINECOSINE(fineshift), polyobj->centerPt.y);
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
// This function basically expands the visplane or I_Errors.
// The reason for this is that when creating 3D floor planes, there is no
// need to create new ones with R_CheckPlane, because 3D floor planes
// are created by subsector and there is no way a subsector can graphically
// overlap.
void R_ExpandPlane(visplane_t *pl, INT32 start, INT32 stop)
{
//	INT32 unionl, unionh;
//	INT32 x;

	// Don't expand polyobject planes here - we do that on our own.
	if (pl->polyobj)
		return;

	if (pl->minx > start) pl->minx = start;
	if (pl->maxx < stop)  pl->maxx = stop;
/*
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
	for (x = start; x <= stop; x++)
		if (pl->top[x] != 0xffff || pl->bottom[x] != 0x0000)
			break;

	if (x <= stop)
		I_Error("R_ExpandPlane: planes in same subsector overlap?!\nminx: %d, maxx: %d, start: %d, stop: %d\n", pl->minx, pl->maxx, start, stop);

	pl->minx = unionl, pl->maxx = unionh;
*/

}

//
// R_MakeSpans
//
void R_MakeSpans(INT32 x, INT32 t1, INT32 b1, INT32 t2, INT32 b2)
{
	//    Alam: from r_splats's R_RasterizeFloorSplat
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

// Potentially override other stuff for now cus we're mean. :< But draw a slope plane!
// I copied ZDoom's code and adapted it to SRB2... -Red
void R_CalculateSlopeVectors(pslope_t *slope, fixed_t planeviewx, fixed_t planeviewy, fixed_t planeviewz, fixed_t planexscale, fixed_t planeyscale, fixed_t planexoffset, fixed_t planeyoffset, angle_t planeviewangle, angle_t planeangle, float fudge)
{
	floatv3_t p, m, n;
	float ang;
	float vx, vy, vz;
	float xscale = FIXED_TO_FLOAT(planexscale);
	float yscale = FIXED_TO_FLOAT(planeyscale);
	// compiler complains when P_GetSlopeZAt is used in FLOAT_TO_FIXED directly
	// use this as a temp var to store P_GetSlopeZAt's return value each time
	fixed_t temp;

	vx = FIXED_TO_FLOAT(planeviewx+planexoffset);
	vy = FIXED_TO_FLOAT(planeviewy-planeyoffset);
	vz = FIXED_TO_FLOAT(planeviewz);

	temp = P_GetSlopeZAt(slope, planeviewx, planeviewy);
	zeroheight = FIXED_TO_FLOAT(temp);

	// p is the texture origin in view space
	// Don't add in the offsets at this stage, because doing so can result in
	// errors if the flat is rotated.
	ang = ANG2RAD(ANGLE_270 - planeviewangle);
	p.x = vx * cos(ang) - vy * sin(ang);
	p.z = vx * sin(ang) + vy * cos(ang);
	temp = P_GetSlopeZAt(slope, -planexoffset, planeyoffset);
	p.y = FIXED_TO_FLOAT(temp) - vz;

	// m is the v direction vector in view space
	ang = ANG2RAD(ANGLE_180 - (planeviewangle + planeangle));
	m.x = yscale * cos(ang);
	m.z = yscale * sin(ang);

	// n is the u direction vector in view space
	n.x = xscale * sin(ang);
	n.z = -xscale * cos(ang);

	ang = ANG2RAD(planeangle);
	temp = P_GetSlopeZAt(slope, planeviewx + FLOAT_TO_FIXED(yscale * sin(ang)), planeviewy + FLOAT_TO_FIXED(yscale * cos(ang)));
	m.y = FIXED_TO_FLOAT(temp) - zeroheight;
	temp = P_GetSlopeZAt(slope, planeviewx + FLOAT_TO_FIXED(xscale * cos(ang)), planeviewy - FLOAT_TO_FIXED(xscale * sin(ang)));
	n.y = FIXED_TO_FLOAT(temp) - zeroheight;

	if (ds_powersoftwo)
	{
		m.x /= fudge;
		m.y /= fudge;
		m.z /= fudge;

		n.x *= fudge;
		n.y *= fudge;
		n.z *= fudge;
	}

	// Eh. I tried making this stuff fixed-point and it exploded on me. Here's a macro for the only floating-point vector function I recall using.
#define CROSS(d, v1, v2) \
d->x = (v1.y * v2.z) - (v1.z * v2.y);\
d->y = (v1.z * v2.x) - (v1.x * v2.z);\
d->z = (v1.x * v2.y) - (v1.y * v2.x)
		CROSS(ds_sup, p, m);
		CROSS(ds_svp, p, n);
		CROSS(ds_szp, m, n);
#undef CROSS

	ds_sup->z *= focallengthf;
	ds_svp->z *= focallengthf;
	ds_szp->z *= focallengthf;

	// Premultiply the texture vectors with the scale factors
#define SFMULT 65536.f
	if (ds_powersoftwo)
	{
		ds_sup->x *= (SFMULT * (1<<nflatshiftup));
		ds_sup->y *= (SFMULT * (1<<nflatshiftup));
		ds_sup->z *= (SFMULT * (1<<nflatshiftup));
		ds_svp->x *= (SFMULT * (1<<nflatshiftup));
		ds_svp->y *= (SFMULT * (1<<nflatshiftup));
		ds_svp->z *= (SFMULT * (1<<nflatshiftup));
	}
	else
	{
		// Lactozilla: I'm essentially multiplying the vectors by FRACUNIT...
		ds_sup->x *= SFMULT;
		ds_sup->y *= SFMULT;
		ds_sup->z *= SFMULT;
		ds_svp->x *= SFMULT;
		ds_svp->y *= SFMULT;
		ds_svp->z *= SFMULT;
	}
#undef SFMULT
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

static void R_SetSlopePlaneVectors(visplane_t *pl, INT32 y, fixed_t xoff, fixed_t yoff, float fudge)
{
	R_SetTiltedSpan(y);
	R_CalculateSlopeVectors(pl->slope, pl->viewx, pl->viewy, pl->viewz, FRACUNIT, FRACUNIT, xoff, yoff, pl->viewangle, pl->plangle, fudge);
}

void R_DrawSinglePlane(visplane_t *pl)
{
	levelflat_t *levelflat;
	INT32 light = 0;
	INT32 x;
	INT32 stop, angle;
	ffloor_t *rover;
	int type;
	int spanfunctype = BASEDRAWFUNC;

	if (!(pl->minx <= pl->maxx))
		return;

	// sky flat
	if (pl->picnum == skyflatnum)
	{
		R_DrawSkyPlane(pl);
		return;
	}

	planeripple.active = false;
	spanfunc = spanfuncs[BASEDRAWFUNC];

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
					ds_transmap = R_GetTranslucencyTable(tr_trans90);
				else if (pl->ffloor->alpha < 64)
					ds_transmap = R_GetTranslucencyTable(tr_trans80);
				else if (pl->ffloor->alpha < 89)
					ds_transmap = R_GetTranslucencyTable(tr_trans70);
				else if (pl->ffloor->alpha < 115)
					ds_transmap = R_GetTranslucencyTable(tr_trans60);
				else if (pl->ffloor->alpha < 140)
					ds_transmap = R_GetTranslucencyTable(tr_trans50);
				else if (pl->ffloor->alpha < 166)
					ds_transmap = R_GetTranslucencyTable(tr_trans40);
				else if (pl->ffloor->alpha < 192)
					ds_transmap = R_GetTranslucencyTable(tr_trans30);
				else if (pl->ffloor->alpha < 217)
					ds_transmap = R_GetTranslucencyTable(tr_trans20);
				else if (pl->ffloor->alpha < 243)
					ds_transmap = R_GetTranslucencyTable(tr_trans10);
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
				INT32 top, bottom;

				planeripple.active = true;

				if (spanfunctype == SPANDRAWFUNC_TRANS)
				{
					spanfunctype = SPANDRAWFUNC_WATER;

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
		}
		else
			light = (pl->lightlevel >> LIGHTSEGSHIFT);
	}

	currentplane = pl;
	levelflat = &levelflats[pl->picnum];

	/* :james: */
	type = levelflat->type;
	switch (type)
	{
		case LEVELFLAT_NONE:
			return;
		case LEVELFLAT_FLAT:
			ds_source = (UINT8 *)R_GetFlat(levelflat->u.flat.lumpnum);
			R_CheckFlatLength(W_LumpLength(levelflat->u.flat.lumpnum));
			// Raw flats always have dimensions that are powers-of-two numbers.
			ds_powersoftwo = true;
			break;
		default:
			ds_source = (UINT8 *)R_GetLevelFlat(levelflat);
			if (!ds_source)
				return;
			// Check if this texture or patch has power-of-two dimensions.
			if (R_CheckPowersOfTwo())
				R_CheckFlatLength(ds_flatwidth * ds_flatheight);
	}

	if (!pl->slope // Don't mess with angle on slopes! We'll handle this ourselves later
		&& viewangle != pl->viewangle+pl->plangle)
	{
		memset(cachedheight, 0, sizeof (cachedheight));
		angle = (pl->viewangle+pl->plangle-ANGLE_90)>>ANGLETOFINESHIFT;
		basexscale = FixedDiv(FINECOSINE(angle),centerxfrac);
		baseyscale = -FixedDiv(FINESINE(angle),centerxfrac);
		viewangle = pl->viewangle+pl->plangle;
	}

	xoffs = pl->xoffs;
	yoffs = pl->yoffs;
	planeheight = abs(pl->height - pl->viewz);

	if (light >= LIGHTLEVELS)
		light = LIGHTLEVELS-1;

	if (light < 0)
		light = 0;

	if (pl->slope)
	{
		float fudgecanyon = 0;
		angle_t hack = (pl->plangle & (ANGLE_90-1));

		yoffs *= 1;

		if (ds_powersoftwo)
		{
			fixed_t temp;
			// Okay, look, don't ask me why this works, but without this setup there's a disgusting-looking misalignment with the textures. -Red
			fudgecanyon = ((1<<nflatshiftup)+1.0f)/(1<<nflatshiftup);
			if (hack)
			{
				/*
				Essentially: We can't & the components along the regular axes when the plane is rotated.
				This is because the distance on each regular axis in order to loop is different.
				We rotate them, & the components, add them together, & them again, and then rotate them back.
				These three seperate & operations are done per axis in order to prevent overflows.
				toast 10/04/17
				*/
				const fixed_t cosinecomponent = FINECOSINE(hack>>ANGLETOFINESHIFT);
				const fixed_t sinecomponent = FINESINE(hack>>ANGLETOFINESHIFT);

				const fixed_t modmask = ((1 << (32-nflatshiftup)) - 1);

				fixed_t ox = (FixedMul(pl->slope->o.x,cosinecomponent) & modmask) - (FixedMul(pl->slope->o.y,sinecomponent) & modmask);
				fixed_t oy = (-FixedMul(pl->slope->o.x,sinecomponent) & modmask) - (FixedMul(pl->slope->o.y,cosinecomponent) & modmask);

				temp = ox & modmask;
				oy &= modmask;
				ox = FixedMul(temp,cosinecomponent)+FixedMul(oy,-sinecomponent); // negative sine for opposite direction
				oy = -FixedMul(temp,-sinecomponent)+FixedMul(oy,cosinecomponent);

				temp = xoffs;
				xoffs = (FixedMul(temp,cosinecomponent) & modmask) + (FixedMul(yoffs,sinecomponent) & modmask);
				yoffs = (-FixedMul(temp,sinecomponent) & modmask) + (FixedMul(yoffs,cosinecomponent) & modmask);

				temp = xoffs & modmask;
				yoffs &= modmask;
				xoffs = FixedMul(temp,cosinecomponent)+FixedMul(yoffs,-sinecomponent); // ditto
				yoffs = -FixedMul(temp,-sinecomponent)+FixedMul(yoffs,cosinecomponent);

				xoffs -= (pl->slope->o.x - ox);
				yoffs += (pl->slope->o.y + oy);
			}
			else
			{
				xoffs &= ((1 << (32-nflatshiftup))-1);
				yoffs &= ((1 << (32-nflatshiftup))-1);
				xoffs -= (pl->slope->o.x + (1 << (31-nflatshiftup))) & ~((1 << (32-nflatshiftup))-1);
				yoffs += (pl->slope->o.y + (1 << (31-nflatshiftup))) & ~((1 << (32-nflatshiftup))-1);
			}

			xoffs = (fixed_t)(xoffs*fudgecanyon);
			yoffs = (fixed_t)(yoffs/fudgecanyon);
		}

		if (planeripple.active)
		{
			fixed_t plheight = abs(P_GetSlopeZAt(pl->slope, pl->viewx, pl->viewy) - pl->viewz);

			R_PlaneBounds(pl);

			for (x = pl->high; x < pl->low; x++)
			{
				R_CalculatePlaneRipple(pl, x, plheight, true);
				R_SetSlopePlaneVectors(pl, x, (xoffs + planeripple.xfrac), (yoffs + planeripple.yfrac), fudgecanyon);
			}
		}
		else
			R_SetSlopePlaneVectors(pl, 0, xoffs, yoffs, fudgecanyon);

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

		planezlight = scalelight[light];
	}
	else
		planezlight = zlight[light];

	// Use the correct span drawer depending on the powers-of-twoness
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
	if (spanfunc == spanfuncs[BASEDRAWFUNC])
	{
		INT32 i;
		ds_transmap = R_GetTranslucencyTable(tr_trans50);
		spanfunc = spanfuncs[SPANDRAWFUNC_TRANS];
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
