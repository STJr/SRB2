// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//-----------------------------------------------------------------------------
/// \file
/// \brief NDS 3D API for SRB2.

#ifndef __R_NDS3D__
#define __R_NDS3D__

#include "../doomtype.h"
#include "../hardware/hw_defs.h"
#include "../hardware/hw_dll.h"
#include "../hardware/hw_md2.h"

#define FAR_CLIPPING_PLANE 150000.0f
#define ASPECT_RATIO 1.0f

typedef float FCOORD;

extern FCOORD NEAR_CLIPPING_PLANE;
extern float fov;

boolean NDS3D_Init(I_Error_t ErrorFunction);
void NDS3D_Shutdown(void);
void NDS3D_SetPalette(RGBA_t *ppal, RGBA_t *pgamma);
void NDS3D_FinishUpdate(INT32 waitvbl);
void NDS3D_Draw2DLine(F2DCoord *v1, F2DCoord *v2, RGBA_t Color);
void NDS3D_DrawPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags);
void NDS3D_SetBlend(FBITFIELD PolyFlags);
void NDS3D_ClearBuffer(FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor);
void NDS3D_SetTexture(FTextureInfo *TexInfo);
void NDS3D_ReadRect(INT32 x, INT32 y, INT32 width, INT32 height, INT32 dst_stride, UINT16 *dst_data);
void NDS3D_GClipRect(INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip);
void NDS3D_ClearMipMapCache(void);
void NDS3D_SetSpecialState(hwdspecialstate_t IdState, INT32 Value);
void NDS3D_DrawMD2(INT32 *gl_cmd_buffer, md2_frame_t *frame, FTransform *pos, float scale);
void NDS3D_DrawMD2i(INT32 *gl_cmd_buffer, md2_frame_t *frame, UINT32 duration, UINT32 tics, md2_frame_t *nextframe, FTransform *pos, float scale, UINT8 flipped, UINT8 *color);
void NDS3D_SetTransform(FTransform *ptransform);
INT32 NDS3D_GetTextureUsed(void);
INT32 NDS3D_GetRenderVersion(void);

#endif
