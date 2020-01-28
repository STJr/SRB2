// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_glide.h
/// \brief  Declaration needed by Glide renderer
///	!!! To be replaced by our own def in the future !!!

#ifndef _GLIDE_H_
#define _GLIDE_H_

#ifndef __GLIDE_H__

typedef unsigned long   FxU32;
typedef long            FxI32;

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
	GrTextureFormat_t format;
	void              *data;
} GrTexInfo;

#endif // __GLIDE_H__ (defined in <glide.h>)

#endif // _GLIDE_H_
