// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004      by Stephen McGranahan
// Copyright (C) 2015-2021 by Sonic Team Junior.
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

extern pslope_t *slopelist;
extern UINT16 slopecount;

typedef enum
{
	TMSP_FRONTFLOOR,
	TMSP_FRONTCEILING,
	TMSP_BACKFLOOR,
	TMSP_BACKCEILING,
} textmapslopeplane_t;

typedef enum
{
	TMSC_FRONTTOBACKFLOOR   = 1,
	TMSC_BACKTOFRONTFLOOR   = 1<<1,
	TMSC_FRONTTOBACKCEILING = 1<<2,
	TMSC_BACKTOFRONTCEILING = 1<<3,
} textmapslopecopy_t;

typedef enum
{
	TMS_NONE,
	TMS_FRONT,
	TMS_BACK,
} textmapside_t;

typedef enum
{
	TMSL_NOPHYSICS = 1,
	TMSL_DYNAMIC = 2,
} textmapslopeflags_t;

void P_LinkSlopeThinkers (void);

void P_CalculateSlopeNormal(pslope_t *slope);
void P_SpawnSlopes(const boolean fromsave);

//
// P_CopySectorSlope
//
// Searches through tagged sectors and copies
//
void P_CopySectorSlope(line_t *line);

pslope_t *P_SlopeById(UINT16 id);

// Returns the height of the sloped plane at (x, y) as a fixed_t
fixed_t P_GetSlopeZAt(const pslope_t *slope, fixed_t x, fixed_t y);

// Like P_GetSlopeZAt but falls back to z if slope is NULL
fixed_t P_GetZAt(const pslope_t *slope, fixed_t x, fixed_t y, fixed_t z);

// Returns the height of the sector at (x, y)
fixed_t P_GetSectorFloorZAt  (const sector_t *sector, fixed_t x, fixed_t y);
fixed_t P_GetSectorCeilingZAt(const sector_t *sector, fixed_t x, fixed_t y);

// Returns the height of the FOF at (x, y)
fixed_t P_GetFFloorTopZAt   (const ffloor_t *ffloor, fixed_t x, fixed_t y);
fixed_t P_GetFFloorBottomZAt(const ffloor_t *ffloor, fixed_t x, fixed_t y);

// Returns the height of the light list at (x, y)
fixed_t P_GetLightZAt(const lightlist_t *light, fixed_t x, fixed_t y);

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
#endif // #ifndef P_SLOPES_H__
