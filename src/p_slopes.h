// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004      by Stephen McGranahan
// Copyright (C) 2015-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_slopes.c
/// \brief ZDoom + Eternity Engine Slopes, ported and enhanced by Kalaron

#ifndef P_SLOPES_H__
#define P_SLOPES_H__

#ifdef ESLOPE
void P_ResetDynamicSlopes(void);
void P_RunDynamicSlopes(void);
// P_SpawnSlope_Line
// Creates one or more slopes based on the given line type and front/back
// sectors.
void P_SpawnSlope_Line(int linenum);

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
void P_HandleSlopeLanding(mobj_t *thing, pslope_t *slope);
void P_ButteredSlope(mobj_t *mo);

#endif

// EOF
#endif // #ifdef ESLOPE
