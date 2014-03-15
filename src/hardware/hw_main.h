// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief 3D render mode functions

#ifndef __HWR_MAIN_H__
#define __HWR_MAIN_H__

#include "hw_data.h"
#include "hw_defs.h"

#include "../am_map.h"
#include "../d_player.h"
#include "../r_defs.h"

// Startup & Shutdown the hardware mode renderer
void HWR_Startup(void);
void HWR_Shutdown(void);

void HWR_clearAutomap(void);
void HWR_drawAMline(const fline_t *fl, INT32 color);
void HWR_FadeScreenMenuBack(UINT32 color, INT32 height);
void HWR_DrawConsoleBack(UINT32 color, INT32 height);
void HWR_RenderPlayerView(INT32 viewnumber, player_t *player);
void HWR_DrawViewBorder(INT32 clearlines);
void HWR_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum);
UINT8 *HWR_GetScreenshot(void);
boolean HWR_Screenshot(const char *lbmname);
void HWR_InitTextureMapping(void);
void HWR_SetViewSize(void);
void HWR_DrawPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option);
void HWR_DrawClippedPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option);
void HWR_DrawSciencePatch(GLPatch_t *gpatch, fixed_t x, fixed_t y, INT32 option, fixed_t scale);
void HWR_DrawTranslucentPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option);
void HWR_DrawSmallPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option, const UINT8 *colormap);
void HWR_DrawMappedPatch(GLPatch_t *gpatch, INT32 x, INT32 y, INT32 option, const UINT8 *colormap);
void HWR_MakePatch (const patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, boolean makebitmap);
void HWR_CreatePlanePolygons(INT32 bspnum);
void HWR_CreateStaticLightmaps(INT32 bspnum);
void HWR_PrepLevelCache(size_t pnumtextures);
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color);
void HWR_DrawPic(INT32 x,INT32 y,lumpnum_t lumpnum);

void HWR_AddCommands(void);
void HWR_CorrectSWTricks(void);
void transform(float *cx, float *cy, float *cz);
FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf);
void HWR_SetPaletteColor(INT32 palcolor);
INT32 HWR_GetTextureUsed(void);
void HWR_DoPostProcessor(player_t *player);
void HWR_StartScreenWipe(void);
void HWR_EndScreenWipe(void);
void HWR_DoScreenWipe(void);
void HWR_PrepFadeToBlack(void);
void HWR_DrawIntermissionBG(void);

// This stuff is put here so MD2's can use them
UINT32 HWR_Lighting(INT32 light, UINT32 color, UINT32 fadecolor, boolean fogblockpoly, boolean plane);
FUNCMATH UINT8 LightLevelToLum(INT32 l);

extern CV_PossibleValue_t granisotropicmode_cons_t[];

extern consvar_t cv_grdynamiclighting;
extern consvar_t cv_grstaticlighting;
extern consvar_t cv_grcoronas;
extern consvar_t cv_grcoronasize;
extern consvar_t cv_grfov;
extern consvar_t cv_grmd2;
extern consvar_t cv_grfog;
extern consvar_t cv_grfogcolor;
extern consvar_t cv_grfogdensity;
extern consvar_t cv_grsoftwarefog;
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;
extern consvar_t cv_grfiltermode;
extern consvar_t cv_granisotropicmode;
extern consvar_t cv_grcorrecttricks;
extern consvar_t cv_voodoocompatibility;
extern consvar_t cv_grfovchange;
extern consvar_t cv_grsolvetjoin;

extern float gr_viewwidth, gr_viewheight, gr_baseviewwindowy;

extern float gr_viewwindowx, gr_basewindowcentery;

// BP: big hack for a test in lighting ref : 1249753487AB
extern fixed_t *hwbbox;
extern FTransform atransform;

typedef struct
{
	wallVert3D    floorVerts[4];
	FSurfaceInfo  Surf;
	INT32           texnum;
	INT32           blend;
	INT32           drawcount;
} floorinfo_t;

#endif
