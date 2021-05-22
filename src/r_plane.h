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
/// \file  r_plane.h
/// \brief Refresh, visplane stuff (floor, ceilings)

#ifndef __R_PLANE__
#define __R_PLANE__

#include "screen.h" // needs MAXVIDWIDTH/MAXVIDHEIGHT
#include "r_data.h"
#include "r_textures.h"
#include "p_polyobj.h"

#define VISPLANEHASHBITS 9
#define VISPLANEHASHMASK ((1<<VISPLANEHASHBITS)-1)
// the last visplane list is outside of the hash table and is used for fof planes
#define MAXVISPLANES ((1<<VISPLANEHASHBITS)+1)

//
// Now what is a visplane, anyway?
// Simple: kinda floor/ceiling polygon optimised for SRB2 rendering.
//
typedef struct visplane_s
{
	struct visplane_s *next;

	fixed_t height;
	fixed_t viewx, viewy, viewz;
	angle_t viewangle;
	angle_t plangle;
	INT32 picnum;
	INT32 lightlevel;
	INT32 minx, maxx;

	// colormaps per sector
	extracolormap_t *extra_colormap;

	// leave pads for [minx-1]/[maxx+1]
	UINT16 padtopstart, top[MAXVIDWIDTH], padtopend;
	UINT16 padbottomstart, bottom[MAXVIDWIDTH], padbottomend;
	INT32 high, low; // R_PlaneBounds should set these.

	fixed_t xoffs, yoffs; // Scrolling flats.

	struct ffloor_s *ffloor;
	polyobj_t *polyobj;
	pslope_t *slope;
} visplane_t;

extern visplane_t *visplanes[MAXVISPLANES];
extern visplane_t *floorplane;
extern visplane_t *ceilingplane;

// Visplane related.
extern INT16 *lastopening, *openings;
extern size_t maxopenings;

extern INT16 floorclip[MAXVIDWIDTH], ceilingclip[MAXVIDWIDTH];
extern fixed_t frontscale[MAXVIDWIDTH], yslopetab[MAXVIDHEIGHT*16];
extern fixed_t cachedheight[MAXVIDHEIGHT];
extern fixed_t cacheddistance[MAXVIDHEIGHT];
extern fixed_t cachedxstep[MAXVIDHEIGHT];
extern fixed_t cachedystep[MAXVIDHEIGHT];
extern fixed_t basexscale, baseyscale;

extern fixed_t *yslope;
extern lighttable_t **planezlight;

void R_InitPlanes(void);
void R_ClearPlanes(void);
void R_ClearFFloorClips (void);

void R_MapPlane(INT32 y, INT32 x1, INT32 x2);
void R_MakeSpans(INT32 x, INT32 t1, INT32 b1, INT32 t2, INT32 b2);
void R_DrawPlanes(void);
visplane_t *R_FindPlane(fixed_t height, INT32 picnum, INT32 lightlevel, fixed_t xoff, fixed_t yoff, angle_t plangle,
	extracolormap_t *planecolormap, ffloor_t *ffloor, polyobj_t *polyobj, pslope_t *slope);
visplane_t *R_CheckPlane(visplane_t *pl, INT32 start, INT32 stop);
void R_ExpandPlane(visplane_t *pl, INT32 start, INT32 stop);
void R_PlaneBounds(visplane_t *plane);

void R_CheckFlatLength(size_t size);
boolean R_CheckPowersOfTwo(void);

// Draws a single visplane.
void R_DrawSinglePlane(visplane_t *pl);

// Calculates the slope vectors needed for tilted span drawing.
void R_CalculateSlopeVectors(pslope_t *slope, fixed_t planeviewx, fixed_t planeviewy, fixed_t planeviewz, fixed_t planexscale, fixed_t planeyscale, fixed_t planexoffset, fixed_t planeyoffset, angle_t planeviewangle, angle_t planeangle, float fudge);

// Sets the slope vector pointers for the current tilted span.
void R_SetTiltedSpan(INT32 span);

typedef struct planemgr_s
{
	visplane_t *plane;
	fixed_t height;
	fixed_t f_pos; // F for Front sector
	fixed_t b_pos; // B for Back sector
	fixed_t f_frac, f_step;
	fixed_t b_frac, b_step;
	INT16 f_clip[MAXVIDWIDTH];
	INT16 c_clip[MAXVIDWIDTH];

	// For slope rendering; the height at the other end
	fixed_t f_pos_slope;
	fixed_t b_pos_slope;

	struct pslope_s *slope;

	struct ffloor_s *ffloor;
	polyobj_t *polyobj;
} visffloor_t;

extern visffloor_t ffloor[MAXFFLOORS];
extern INT32 numffloors;

void Portal_AddSkyboxPortals (void);
#endif
