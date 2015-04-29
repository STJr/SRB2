// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 Stephen McGranahan
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Slopes
//      SoM created 05/10/09
//
//-----------------------------------------------------------------------------

#ifndef P_SLOPES_H__
#define P_SLOPES_H__

#ifdef ESLOPE
void P_ResetDynamicSlopes(void);
void P_RunDynamicSlopes(void);
// P_SpawnSlope_Line
// Creates one or more slopes based on the given line type and front/back
// sectors.
void P_SpawnSlope_Line(int linenum);

#ifdef SPRINGCLEAN
// Loads just map objects that make slopes,
// terrain affecting objects have to be spawned first
void P_SetSlopesFromVertexHeights(lumpnum_t lumpnum);

typedef enum
{
	THING_SlopeFloorPointLine = 9500,
	THING_SlopeCeilingPointLine = 9501,
	THING_SetFloorSlope = 9502,
	THING_SetCeilingSlope = 9503,
	THING_CopyFloorPlane = 9510,
	THING_CopyCeilingPlane = 9511,
	THING_VavoomFloor=1500,
	THING_VavoomCeiling=1501,
	THING_VertexFloorZ=1504,
	THING_VertexCeilingZ=1505,
} slopething_e;
#endif

//
// P_CopySectorSlope
//
// Searches through tagged sectors and copies
//
void P_CopySectorSlope(line_t *line);

// Returns the height of the sloped plane at (x, y) as a fixed_t
fixed_t P_GetZAt(pslope_t *slope, fixed_t x, fixed_t y);


// Returns the height of the sloped plane at (x, y) as a float
float P_GetZAtf(pslope_t *slope, float x, float y);


// Unused? -Red
// Returns the distance of the given point from the given origin and normal.
float P_DistFromPlanef(const v3float_t *point, const v3float_t *pori,
                       const v3float_t *pnormal);

// Lots of physics-based bullshit
void P_QuantizeMomentumToSlope(v3fixed_t *momentum, pslope_t *slope);
void P_SlopeLaunch(mobj_t *mo);
void P_ButteredSlope(mobj_t *mo);

#endif

// EOF
#endif // #ifdef ESLOPE

