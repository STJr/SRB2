// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2000 by Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko (prboom)
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_fps.h
/// \brief Uncapped framerate stuff.

#ifndef __R_FPS_H__
#define __R_FPS_H__

#include "m_fixed.h"
#include "p_local.h"
#include "r_state.h"

enum viewcontext_e
{
	VIEWCONTEXT_PLAYER1 = 0,
	VIEWCONTEXT_PLAYER2,
	VIEWCONTEXT_SKY1,
	VIEWCONTEXT_SKY2
};

typedef struct {
	fixed_t x;
	fixed_t y;
	fixed_t z;
	boolean sky;
	sector_t *sector;
	player_t *player;

	angle_t angle;
	angle_t aim;
	fixed_t cos;
	fixed_t sin;
	mobj_t *mobj;
} viewvars_t;

extern viewvars_t *newview;

typedef struct {
	fixed_t x;
	fixed_t y;
	fixed_t z;
	angle_t angle;
} interpmobjstate_t;

// Level interpolators

// The union tag for levelinterpolator_t
typedef enum {
	LVLINTERP_SectorPlane,
	LVLINTERP_SectorScroll,
	LVLINTERP_SideScroll,
} levelinterpolator_type_e;

// Tagged union of a level interpolator
typedef struct levelinterpolator_s {
	levelinterpolator_type_e type;
	thinker_t *thinker;
	union {
		struct {
			sector_t *sector;
			fixed_t oldheight;
			fixed_t bakheight;
			boolean ceiling;
		} sectorplane;
		struct {
			sector_t *sector;
			fixed_t oldxoffs, oldyoffs, bakxoffs, bakyoffs;
			boolean ceiling;
		} sectorscroll;
		struct {
			side_t *side;
			fixed_t oldtextureoffset, oldrowoffset, baktextureoffset, bakrowoffset;
		} sidescroll;
	};
} levelinterpolator_t;

// Interpolates the current view variables (r_state.h) against the selected view context in R_SetViewContext
void R_InterpolateView(fixed_t frac);
// Buffer the current new views into the old views. Call once after each real tic.
void R_UpdateViewInterpolation(void);
// Reset the view states (e.g. after level load) so R_InterpolateView doesn't interpolate invalid data
void R_ResetViewInterpolation(void);
// Set the current view context (the viewvars pointed to by newview)
void R_SetViewContext(enum viewcontext_e _viewcontext);
// Evaluate the interpolated mobj state for the given mobj
void R_InterpolateMobjState(mobj_t *mobj, fixed_t frac, interpmobjstate_t *out);
// Evaluate the interpolated mobj state for the given precipmobj
void R_InterpolatePrecipMobjState(precipmobj_t *mobj, fixed_t frac, interpmobjstate_t *out);

void R_CreateInterpolator_SectorPlane(thinker_t *thinker, sector_t *sector, boolean ceiling);
void R_CreateInterpolator_SectorScroll(thinker_t *thinker, sector_t *sector, boolean ceiling);
void R_CreateInterpolator_SideScroll(thinker_t *thinker, side_t *side);

// Initialize level interpolators after a level change
void R_InitializeLevelInterpolators(void);
// Update level interpolators, storing the previous and current states.
void R_UpdateLevelInterpolators(void);
// Clear states for all level interpolators for the thinker
void R_ClearLevelInterpolatorState(thinker_t *thinker);
// Apply level interpolators to the actual game state
void R_ApplyLevelInterpolators(fixed_t frac);
// Restore level interpolators to the real game state
void R_RestoreLevelInterpolators(void);

#endif
