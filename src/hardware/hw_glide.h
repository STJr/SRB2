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
//-----------------------------------------------------------------------------
/// \file
/// \brief  Declaration needed by Glide renderer
///	!!! To be replaced by our own def in the future !!!

#ifndef _GLIDE_H_
#define _GLIDE_H_

#ifndef __GLIDE_H__

typedef unsigned long   FxU32;
typedef long            FxI32;

typedef FxI32 GrAspectRatio_t;
#define GR_ASPECT_LOG2_8x1        3       /* 8W x 1H */
#define GR_ASPECT_LOG2_4x1        2       /* 4W x 1H */
#define GR_ASPECT_LOG2_2x1        1       /* 2W x 1H */
#define GR_ASPECT_LOG2_1x1        0       /* 1W x 1H */
#define GR_ASPECT_LOG2_1x2       -1       /* 1W x 2H */
#define GR_ASPECT_LOG2_1x4       -2       /* 1W x 4H */
#define GR_ASPECT_LOG2_1x8       -3       /* 1W x 8H */

typedef FxI32 GrLOD_t;
#define GR_LOD_LOG2_256         0x8
#define GR_LOD_LOG2_128         0x7
#define GR_LOD_LOG2_64          0x6
#define GR_LOD_LOG2_32          0x5
#define GR_LOD_LOG2_16          0x4
#define GR_LOD_LOG2_8           0x3
#define GR_LOD_LOG2_4           0x2
#define GR_LOD_LOG2_2           0x1
#define GR_LOD_LOG2_1           0x0

typedef FxI32 GrTextureFormat_t;
#define GR_TEXFMT_ALPHA_8               0x2 /* (0..0xFF) alpha     */
#define GR_TEXFMT_INTENSITY_8           0x3 /* (0..0xFF) intensity */
#define GR_TEXFMT_ALPHA_INTENSITY_44    0x4
#define GR_TEXFMT_P_8                   0x5 /* 8-bit palette */
#define GR_TEXFMT_RGB_565               0xa
#define GR_TEXFMT_ARGB_1555             0xb
#define GR_TEXFMT_ARGB_4444             0xc
#define GR_TEXFMT_ALPHA_INTENSITY_88    0xd
#define GR_TEXFMT_AP_88                 0xe /* 8-bit alpha 8-bit palette */
#define GR_RGBA                         0x6 // 32 bit RGBA !

typedef struct
{
	GrLOD_t           smallLodLog2;
	GrLOD_t           largeLodLog2;
	GrAspectRatio_t   aspectRatioLog2;
	GrTextureFormat_t format;
	void              *data;
} GrTexInfo;

#endif // __GLIDE_H__ (defined in <glide.h>)

#endif // _GLIDE_H_
