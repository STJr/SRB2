// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2020 by Ethan Watson.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_context.h
/// \brief Render context

#ifndef __R_CONTEXT__
#define __R_CONTEXT__

#include "r_defs.h"
#include "r_bsp.h"
#include "r_plane.h"
#include "r_splats.h"
#include "r_things.h"

//
// Render context is required for threaded rendering.
// So everything "global" goes in here. Everything.
//

typedef struct viewcontext_s
{
	fixed_t x, y, z;
	angle_t angle, aimingangle;
	fixed_t cos, sin;
	sector_t *sector;
	player_t *player;
	mobj_t *mobj;
} viewcontext_t;

typedef struct bspcontext_s
{
    seg_t *curline;
    side_t *sidedef;
    line_t *linedef;
    sector_t *frontsector;
    sector_t *backsector;

    // very ugly realloc() of drawsegs at run-time, I upped it to 512
    // instead of 256.. and someone managed to send me a level with
    // 896 drawsegs! So too bad here's a limit removal a-la-Boom
    drawseg_t *curdrawsegs; /**< This is used to handle multiple lists for masked drawsegs. */
    drawseg_t *drawsegs;
    drawseg_t *ds_p;
    drawseg_t *firstseg;
    size_t maxdrawsegs;

    // newend is one past the last valid seg
    cliprange_t *newend;
    cliprange_t solidsegs[MAXSEGS];

    sector_t ftempsec;
    sector_t btempsec;

    UINT8 doorclosed;

    // Linked list for portals.
    struct portal_s *portal_base, *portal_cap;

    // When rendering a portal, it establishes the depth of the current BSP traversal.
    UINT8 portalrender;

    boolean portalline;
    line_t *portalclipline;
    sector_t *portalcullsector;
    INT32 portalclipstart, portalclipend;
} bspcontext_t;

typedef struct planecontext_s
{
	//SoM: 3/23/2000: Use Boom visplane hashing.
	visplane_t *visplanes[MAXVISPLANES];
	visplane_t *freetail;
	visplane_t **freehead;

	visplane_t *floorplane;
	visplane_t *ceilingplane;
	visplane_t *currentplane;

	planemgr_t ffloor[MAXFFLOORS];
	INT32 numffloors;

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
	INT32 spanstart[MAXVIDHEIGHT];

	//
	// texture mapping
	//
	lighttable_t **zlight;
	fixed_t planeheight;

	fixed_t cachedheight[MAXVIDHEIGHT];
	fixed_t cacheddistance[MAXVIDHEIGHT];
	fixed_t cachedxstep[MAXVIDHEIGHT];
	fixed_t cachedystep[MAXVIDHEIGHT];

	fixed_t xoffs, yoffs;

	struct
	{
		INT32 offset;
		fixed_t xfrac, yfrac;
		boolean active;
	} ripple;

	struct rastery_s *prastertab;
	struct rastery_s rastertab[MAXVIDHEIGHT];
} planecontext_t;

typedef struct spritecontext_s
{
	UINT32 visspritecount;
	UINT32 clippedvissprites;
	vissprite_t *visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS];
	vissprite_t overflowsprite;

	lighttable_t **spritelights;

	boolean *sectorvisited;

	drawnode_t nodebankhead;
	vissprite_t vsprsortedhead;
} spritecontext_t;

typedef struct wallcontext_s
{
	angle_t normalangle;
	// angle to line origin
	angle_t angle1;
	fixed_t distance;

	//
	// regular wall
	//
	angle_t centerangle;
	fixed_t offset;
	fixed_t offset2; // for splats
	fixed_t scale, scalestep;
	fixed_t midtexturemid, toptexturemid, bottomtexturemid;
	fixed_t toptextureslide, midtextureslide, bottomtextureslide; // Defines how to adjust Y offsets along the wall for slopes
	fixed_t midtextureback, midtexturebackslide; // Values for masked midtexture height calculation

	INT32 *silhouette;
	fixed_t *tsilheight;
	fixed_t *bsilheight;
} wallcontext_t;

typedef struct rendercontext_s
{
	// Setup
	vbuffer_t buffer;
	INT32 num;

	INT32 begincolumn;
	INT32 endcolumn;

	viewcontext_t viewcontext;
	bspcontext_t bspcontext;
	planecontext_t planecontext;
	spritecontext_t spritecontext;

	colcontext_t colcontext;
	spancontext_t spancontext;
} rendercontext_t;

#endif
