// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004      by Stephen McGranahan
// Copyright (C) 2015-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_slopes.c
/// \brief ZDoom + Eternity Engine Slopes, ported and enhanced by Kalaron

#ifndef P_SLOPES_H__
#define P_SLOPES_H__

#include "m_fixed.h" // Vectors

#ifdef ESLOPE

extern pslope_t *slopelist;
extern UINT16 slopecount;

void P_LinkSlopeThinkers (void);

void P_CalculateSlopeNormal(pslope_t *slope);
void P_ResetDynamicSlopes(const boolean fromsave);

//
// P_CopySectorSlope
//
// Searches through tagged sectors and copies
//
void P_CopySectorSlope(line_t *line);

pslope_t *P_SlopeById(UINT16 id);

// Returns the height of the sloped plane at (x, y) as a fixed_t
fixed_t P_GetZAt(pslope_t *slope, fixed_t x, fixed_t y);

// Lots of physics-based bullshit
void P_QuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope);
void P_ReverseQuantizeMomentumToSlope(vector3_t *momentum, pslope_t *slope);
void P_SlopeLaunch(mobj_t *mo);
fixed_t P_GetWallTransferMomZ(mobj_t *mo, pslope_t *slope);
void P_HandleSlopeLanding(mobj_t *thing, pslope_t *slope);
void P_ButteredSlope(mobj_t *mo);


/// Dynamic plane type enum for the thinker. Will have a different functionality depending on this.
typedef enum {
	DP_FRONTFLOOR,
	DP_FRONTCEIL,
	DP_BACKFLOOR,
	DP_BACKCEIL,
	DP_VERTEX
} dynplanetype_t;

/// Permit slopes to be dynamically altered through a thinker.
typedef struct
{
	thinker_t thinker;

	pslope_t* slope;
	dynplanetype_t type;

	// Used by line slopes.
	line_t* sourceline;
	fixed_t extent;

	// Used by mapthing vertex slopes.
	INT16 tags[3];
	vector3_t vex[3];
} dynplanethink_t;

void T_DynamicSlopeLine (dynplanethink_t* th);
void T_DynamicSlopeVert (dynplanethink_t* th);
#endif // #ifdef ESLOPE
#endif // #ifndef P_SLOPES_H__
