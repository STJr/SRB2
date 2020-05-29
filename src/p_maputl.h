// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_maputl.h
/// \brief map utility functions

#ifndef __P_MAPUTL__
#define __P_MAPUTL__

#include "doomtype.h"
#include "r_defs.h"
#include "m_fixed.h"

//
// P_MAPUTL
//
typedef struct
{
	fixed_t x, y, dx, dy;
} divline_t;

typedef struct
{
	fixed_t frac; // along trace line
	boolean isaline;
	union
	{
		mobj_t *thing;
		line_t *line;
	} d;
} intercept_t;

typedef boolean (*traverser_t)(intercept_t *in);

boolean P_PathTraverse(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2,
	INT32 pflags, traverser_t ptrav);

FUNCMATH fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
void P_ClosestPointOnLine(fixed_t x, fixed_t y, line_t *line, vertex_t *result);
void P_ClosestPointOnLine3D(const vector3_t *p, const vector3_t *line, vector3_t *result);
INT32 P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line);
void P_MakeDivline(line_t *li, divline_t *dl);
void P_CameraLineOpening(line_t *plinedef);
fixed_t P_InterceptVector(divline_t *v2, divline_t *v1);
INT32 P_BoxOnLineSide(fixed_t *tmbox, line_t *ld);
void P_UnsetPrecipThingPosition(precipmobj_t *thing);
void P_SetPrecipitationThingPosition(precipmobj_t *thing);
void P_CreatePrecipSecNodeList(precipmobj_t *thing, fixed_t x,fixed_t y);
boolean P_SceneryTryMove(mobj_t *thing, fixed_t x, fixed_t y);

extern fixed_t opentop, openbottom, openrange, lowfloor, highceiling;
extern pslope_t *opentopslope, *openbottomslope;
extern ffloor_t *openfloorrover, *openceilingrover;

void P_LineOpening(line_t *plinedef, mobj_t *mobj);

boolean P_BlockLinesIterator(INT32 x, INT32 y, boolean(*func)(line_t *));
boolean P_BlockThingsIterator(INT32 x, INT32 y, boolean(*func)(mobj_t *));

#define PT_ADDLINES     1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT     4

extern divline_t trace;

extern fixed_t tmbbox[4]; // p_map.c

// call your user function for each line of the blockmap in the
// bbox defined by the radius
//boolean P_RadiusLinesCheck(fixed_t radius, fixed_t x, fixed_t y,
//	boolean (*func)(line_t *));
#endif // __P_MAPUTL__
