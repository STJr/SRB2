// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_data.h
/// \brief defines structures and exports for the hardware interface used by Sonic Robo Blast 2

#ifndef _HWR_DATA_
#define _HWR_DATA_

#if defined (_WIN32) && !defined (__CYGWIN__)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif

#include "../doomdef.h"
#include "../screen.h"

// ==========================================================================
//                                                               TEXTURE INFO
// ==========================================================================

enum GLTextureFormat_e
{
	GPU_TEXFMT_P_8                 = 0x01, /* 8-bit palette */
	GPU_TEXFMT_AP_88               = 0x02, /* 8-bit alpha, 8-bit palette */

	GPU_TEXFMT_RGBA                = 0x10, /* 32 bit RGBA! */

	GPU_TEXFMT_ALPHA_8             = 0x20, /* (0..0xFF) alpha     */
	GPU_TEXFMT_INTENSITY_8         = 0x21, /* (0..0xFF) intensity */
	GPU_TEXFMT_ALPHA_INTENSITY_88  = 0x22,
};
typedef enum GLTextureFormat_e GLTextureFormat_t;

// Colormap structure for textures.
struct HWRColormap_s
{
	const UINT8 *source;
	UINT8 data[256];
};
typedef struct HWRColormap_s HWRColormap_t;


// data holds the address of the graphics data cached in heap memory
// NULL if the texture is not in Doom heap cache.
struct HWRTexture_s
{
	GLTextureFormat_t     format;
	void                 *data;

	UINT32                flags;
	UINT16                height;
	UINT16                width;
	UINT32                downloaded;     // The GPU has this texture.

	struct HWRTexture_s  *nextcolormap;
	struct HWRColormap_s *colormap;
};
typedef struct HWRTexture_s HWRTexture_t;

// Texture info, as cached for hardware rendering
struct GLMapTexture_s
{
	HWRTexture_t  texture;
	float         scaleX;
	float         scaleY;
};
typedef struct GLMapTexture_s GLMapTexture_t;

// A cached patch, as converted to hardware format
struct GLPatch_s
{
	float               max_s,max_t;
	HWRTexture_t       *texture;
};
typedef struct GLPatch_s GLPatch_t;

#endif //_HWR_DATA_
