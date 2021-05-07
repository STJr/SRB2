// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
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
#include "r_textures.h"

//
// POV related.
//
extern fixed_t viewcos, viewsin;
extern INT32 viewheight;
extern INT32 centerx, centery;

extern fixed_t centerxfrac, centeryfrac;
extern fixed_t projection, projectiony;
extern fixed_t fovtan;

// WARNING: a should be unsigned but to add with 2048, it isn't!
#define AIMINGTODY(a) FixedDiv((FINETANGENT((2048+(((INT32)a)>>ANGLETOFINESHIFT)) & FINEMASK)*160), fovtan)

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

#define LIGHTRESOLUTIONFIX (640*fovtan/vid.width)

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
angle_t R_PointToAngle64(INT64 x, INT64 y);
angle_t R_PointToAngle2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1);
angle_t R_PointToAngleEx(INT32 x2, INT32 y2, INT32 x1, INT32 y1);
fixed_t R_PointToDist(fixed_t x, fixed_t y);
fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1);

fixed_t R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
subsector_t *R_PointInSubsectorOrNull(fixed_t x, fixed_t y);

boolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph);

// Render stats

extern precise_t ps_prevframetime;// time when previous frame was rendered
extern precise_t ps_rendercalltime;
extern precise_t ps_uitime;
extern precise_t ps_swaptime;

extern precise_t ps_bsptime;

extern precise_t ps_sw_spritecliptime;
extern precise_t ps_sw_portaltime;
extern precise_t ps_sw_planetime;
extern precise_t ps_sw_maskedtime;

extern int ps_numbspcalls;
extern int ps_numsprites;
extern int ps_numdrawnodes;
extern int ps_numpolyobjects;

//
// REFRESH - the actual rendering functions.
//

extern consvar_t cv_showhud, cv_translucenthud;
extern consvar_t cv_homremoval;
extern consvar_t cv_chasecam, cv_chasecam2;
extern consvar_t cv_flipcam, cv_flipcam2;

extern consvar_t cv_shadow;
extern consvar_t cv_ffloorclip;
extern consvar_t cv_translucency;
extern consvar_t cv_drawdist, cv_drawdist_nights, cv_drawdist_precip;
extern consvar_t cv_fov;
extern consvar_t cv_skybox;
extern consvar_t cv_tailspickup;

// Called by startup code.
void R_Init(void);

void R_CheckViewMorph(void);
void R_ApplyViewMorph(void);

// just sets setsizeneeded true
extern boolean setsizeneeded;
void R_SetViewSize(void);

// do it (sometimes explicitly called)
void R_ExecuteSetViewSize(void);

void R_SetupFrame(player_t *player);
void R_SkyboxFrame(player_t *player);

boolean R_ViewpointHasChasecam(player_t *player);
boolean R_IsViewpointThirdPerson(player_t *player, boolean skybox);

// Called by D_Display.
void R_RenderPlayerView(player_t *player);

// add commands related to engine, at game startup
void R_RegisterEngineStuff(void);
#endif
