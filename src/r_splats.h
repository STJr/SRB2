// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_splats.h
/// \brief Flat sprites & splats effects

#ifndef __R_SPLATS_H__
#define __R_SPLATS_H__

#include "r_defs.h"
#include "r_things.h"

// ==========================================================================
// DEFINITIONS
// ==========================================================================

struct rastery_s
{
	fixed_t minx, maxx; // for each raster line starting at line 0
	fixed_t tx1, ty1;   // start points in texture at this line
	fixed_t tx2, ty2;   // end points in texture at this line
};
extern struct rastery_s *prastertab; // for ASM code

typedef struct floorsplat_s
{
	UINT16 *pic;
	INT32 width, height;
	fixed_t scale, xscale, yscale;
	angle_t angle;
	boolean tilted; // Uses the tilted drawer
	pslope_t slope;

	vector3_t verts[4]; // (x,y,z) as viewed from above on map
	fixed_t x, y, z; // position
	mobj_t *mobj; // Mobj it is tied to
} floorsplat_t;

void R_DrawFloorSplat(vissprite_t *spr);

#endif /*__R_SPLATS_H__*/
