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

typedef enum GLTextureFormat_e
{
	GL_TEXFMT_P_8                 = 0x01, /* 8-bit palette */
	GL_TEXFMT_AP_88               = 0x02, /* 8-bit alpha, 8-bit palette */

	GL_TEXFMT_RGBA                = 0x10, /* 32 bit RGBA! */

	GL_TEXFMT_ALPHA_8             = 0x20, /* (0..0xFF) alpha     */
	GL_TEXFMT_INTENSITY_8         = 0x21, /* (0..0xFF) intensity */
	GL_TEXFMT_ALPHA_INTENSITY_88  = 0x22,
} GLTextureFormat_t;

// Colormap structure for mipmaps.
struct GLColormap_s
{
	const UINT8 *source;
	UINT8 data[256];
};
typedef struct GLColormap_s GLColormap_t;


// Texture information (misleadingly named "mipmap" all over the code.)
// The *data pointer holds the address of the graphics data cached in heap memory.
// NULL if the texture is not in SRB2's heap cache.
struct GLMipmap_s
{
	// for UpdateTexture
	GLTextureFormat_t     format;
	void                 *data;

	UINT32                flags;
	UINT16                height;
	UINT16                width;
	UINT32                downloaded; // The GPU has this texture.

	struct GLMipmap_s    *nextcolormap;
	struct GLColormap_s  *colormap;
};
typedef struct GLMipmap_s GLMipmap_t;


//
// Level textures, as cached for hardware rendering.
//
struct GLMapTexture_s
{
	GLMipmap_t  mipmap;
	float       scaleX; // Used for scaling textures on walls
	float       scaleY;
};
typedef struct GLMapTexture_s GLMapTexture_t;


// Patch information for the hardware renderer.
struct GLPatch_s
{
	GLMipmap_t *mipmap; // Texture data. Allocated whenever the patch is.
	float       max_s, max_t;
};
typedef struct GLPatch_s GLPatch_t;

#endif //_HWR_DATA_
