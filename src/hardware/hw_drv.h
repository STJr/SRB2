// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_drv.h
/// \brief imports/exports for the 3D hardware low-level interface API

#ifndef __HWR_DRV_H__
#define __HWR_DRV_H__

// this must be here 19991024 by Kin
#include "../screen.h"
#include "hw_data.h"
#include "hw_defs.h"
#include "hw_md2.h"

#include "hw_dll.h"

// ==========================================================================
//                                                       STANDARD DLL EXPORTS
// ==========================================================================

EXPORT boolean HWRAPI(Init) (void);
#ifndef HAVE_SDL
EXPORT void HWRAPI(Shutdown) (void);
#endif
EXPORT void HWRAPI(SetTexturePalette) (RGBA_t *ppal);
EXPORT void HWRAPI(FinishUpdate) (INT32 waitvbl);
EXPORT void HWRAPI(Draw2DLine) (F2DCoord *v1, F2DCoord *v2, RGBA_t Color);
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags);
EXPORT void HWRAPI(DrawIndexedTriangles) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, UINT32 *IndexArray);
EXPORT void HWRAPI(RenderSkyDome) (gl_sky_t *sky);
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags);
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor);
EXPORT void HWRAPI(SetTexture) (GLMipmap_t *TexInfo);
EXPORT void HWRAPI(UpdateTexture) (GLMipmap_t *TexInfo);
EXPORT void HWRAPI(DeleteTexture) (GLMipmap_t *TexInfo);
EXPORT void HWRAPI(ReadScreenTexture) (int tex, UINT8 *dst_data);
EXPORT void HWRAPI(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip);
EXPORT void HWRAPI(ClearMipMapCache) (void);

EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value);

//Hurdler: added for new development
EXPORT void HWRAPI(DrawModel) (model_t *model, INT32 frameIndex, float duration, float tics, INT32 nextFrameIndex, FTransform *pos, float hscale, float vscale, UINT8 flipped, UINT8 hflipped, FSurfaceInfo *Surface);
EXPORT void HWRAPI(CreateModelVBOs) (model_t *model);
EXPORT void HWRAPI(SetTransform) (FTransform *ptransform);
EXPORT INT32 HWRAPI(GetTextureUsed) (void);

EXPORT void HWRAPI(FlushScreenTextures) (void);
EXPORT void HWRAPI(DoScreenWipe) (int wipeStart, int wipeEnd, FSurfaceInfo *surf, FBITFIELD polyFlags);
EXPORT void HWRAPI(DrawScreenTexture) (int tex, FSurfaceInfo *surf, FBITFIELD polyflags);
EXPORT void HWRAPI(MakeScreenTexture) (int tex);
EXPORT void HWRAPI(DrawScreenFinalTexture) (int tex, int width, int height);

#define SCREENVERTS 10
EXPORT void HWRAPI(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2]);

EXPORT boolean HWRAPI(InitShaders) (void);
EXPORT void HWRAPI(LoadShader) (int slot, char *code, hwdshaderstage_t stage);
EXPORT boolean HWRAPI(CompileShader) (int slot);
EXPORT void HWRAPI(SetShader) (int slot);
EXPORT void HWRAPI(UnSetShader) (void);

EXPORT void HWRAPI(SetShaderInfo) (hwdshaderinfo_t info, INT32 value);

EXPORT void HWRAPI(SetPaletteLookup)(UINT8 *lut);
EXPORT UINT32 HWRAPI(CreateLightTable)(RGBA_t *hw_lighttable);
EXPORT void HWRAPI(UpdateLightTable)(UINT32 id, RGBA_t *hw_lighttable);
EXPORT void HWRAPI(ClearLightTables)(void);
EXPORT void HWRAPI(SetScreenPalette)(RGBA_t *palette);

// ==========================================================================
//                                      HWR DRIVER OBJECT, FOR CLIENT PROGRAM
// ==========================================================================

#if !defined (_CREATE_DLL_)

struct hwdriver_s
{
	Init                pfnInit;
	SetTexturePalette   pfnSetTexturePalette;
	FinishUpdate        pfnFinishUpdate;
	Draw2DLine          pfnDraw2DLine;
	DrawPolygon         pfnDrawPolygon;
	DrawIndexedTriangles    pfnDrawIndexedTriangles;
	RenderSkyDome       pfnRenderSkyDome;
	SetBlend            pfnSetBlend;
	ClearBuffer         pfnClearBuffer;
	SetTexture          pfnSetTexture;
	UpdateTexture       pfnUpdateTexture;
	DeleteTexture       pfnDeleteTexture;
	ReadScreenTexture   pfnReadScreenTexture;
	GClipRect           pfnGClipRect;
	ClearMipMapCache    pfnClearMipMapCache;
	SetSpecialState     pfnSetSpecialState;
	DrawModel           pfnDrawModel;
	CreateModelVBOs     pfnCreateModelVBOs;
	SetTransform        pfnSetTransform;
	GetTextureUsed      pfnGetTextureUsed;
#ifdef _WINDOWS
	GetModeList         pfnGetModeList;
#endif
#ifndef HAVE_SDL
	Shutdown            pfnShutdown;
#endif
	PostImgRedraw       pfnPostImgRedraw;
	FlushScreenTextures pfnFlushScreenTextures;
	DoScreenWipe        pfnDoScreenWipe;
	DrawScreenTexture   pfnDrawScreenTexture;
	MakeScreenTexture   pfnMakeScreenTexture;
	DrawScreenFinalTexture  pfnDrawScreenFinalTexture;

	InitShaders         pfnInitShaders;
	LoadShader          pfnLoadShader;
	CompileShader       pfnCompileShader;
	SetShader           pfnSetShader;
	UnSetShader         pfnUnSetShader;

	SetShaderInfo       pfnSetShaderInfo;

	SetPaletteLookup    pfnSetPaletteLookup;
	CreateLightTable    pfnCreateLightTable;
	UpdateLightTable    pfnUpdateLightTable;
	ClearLightTables    pfnClearLightTables;
	SetScreenPalette    pfnSetScreenPalette;
};

extern struct hwdriver_s hwdriver;

#define HWD hwdriver

#endif //not defined _CREATE_DLL_

#endif //__HWR_DRV_H__
