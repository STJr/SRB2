// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_main.h
/// \brief Rendering variables, consvars, defines

#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"

//
// POV related.
//
extern fixed_t viewcos, viewsin;
extern INT32 viewheight;
extern INT32 centerx, centery;

extern fixed_t centerxfrac, centeryfrac;
extern fixed_t projection, projectiony;

extern size_t validcount, linecount, loopcount, framecount;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now with 32 levels.
#define LIGHTLEVELS 32
#define LIGHTSEGSHIFT 3

#define MAXLIGHTSCALE 48
#define LIGHTSCALESHIFT 12
#define MAXLIGHTZ 128
#define LIGHTZSHIFT 20

extern lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern lighttable_t *scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS 32

// Utility functions.
INT32 R_PointOnSide(fixed_t x, fixed_t y, node_t *node);
INT32 R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line);
angle_t R_PointToAngle(fixed_t x, fixed_t y);
angle_t R_PointToAngle2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1);
fixed_t R_PointToDist(fixed_t x, fixed_t y);
fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1);

// ZDoom C++ to Legacy C conversion Tails 04-29-2002
fixed_t R_SecplaneZatPoint(secplane_t *secplane, fixed_t x, fixed_t y);
fixed_t R_SecplaneZatPointDist(secplane_t *secplane, fixed_t x, fixed_t y,
	fixed_t dist);
void R_SecplaneFlipVert(secplane_t *secplane);
boolean R_ArePlanesSame(secplane_t *original,  secplane_t *other);
boolean R_ArePlanesDifferent(secplane_t *original,  secplane_t *other);
void R_SecplaneChangeHeight(secplane_t *secplane, fixed_t hdiff);
fixed_t R_SecplaneHeightDiff(secplane_t *secplane, fixed_t oldd);
fixed_t R_SecplanePointToDist(secplane_t *secplane, fixed_t x, fixed_t y, fixed_t z);
fixed_t R_SecplanePointToDist2(secplane_t *secplane, fixed_t x, fixed_t y, fixed_t z);

fixed_t R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
subsector_t *R_IsPointInSubsector(fixed_t x, fixed_t y);

//
// REFRESH - the actual rendering functions.
//

extern consvar_t cv_showhud;
extern consvar_t cv_homremoval;
extern consvar_t cv_grtranslucenthud;
extern consvar_t cv_chasecam, cv_chasecam2;
extern consvar_t cv_flipcam, cv_flipcam2;
extern consvar_t cv_shadow, cv_shadowoffs;
extern consvar_t cv_precipdensity, cv_drawdist, cv_drawdist_nights, cv_drawdist_precip;
extern consvar_t cv_skybox;
extern consvar_t cv_tailspickup;

// Called by startup code.
void R_Init(void);

// just sets setsizeneeded true
extern boolean setsizeneeded;
void R_SetViewSize(void);

// do it (sometimes explicitly called)
void R_ExecuteSetViewSize(void);

void R_SetupFrame(player_t *player, boolean skybox);
// Called by G_Drawer.
void R_RenderPlayerView(player_t *player);

// add commands related to engine, at game startup
void R_RegisterEngineStuff(void);
#endif
