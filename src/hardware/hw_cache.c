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
/// \brief load and convert graphics to the hardware format

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_drv.h"

#include "../doomstat.h"    //gamemode
#include "../i_video.h"     //rendermode
#include "../r_data.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../r_draw.h"

//Hurdler: 25/04/2000: used for new colormap code in hardware mode
//static UINT8 *gr_colormap = NULL; // by default it must be NULL ! (because colormap tables are not initialized)
boolean firetranslucent = false;

// Values set after a call to HWR_ResizeBlock()
static INT32 blocksize, blockwidth, blockheight;

INT32 patchformat = GR_TEXFMT_AP_88; // use alpha for holes
INT32 textureformat = GR_TEXFMT_P_8; // use chromakey for hole

// sprite, use alpha and chroma key for hole
static void HWR_DrawPatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight, INT32 blockmodulo,
	INT32 ptexturewidth, INT32 ptextureheight,
	INT32 originx, INT32 originy, // where to draw patch in surface block
	const patch_t *realpatch, INT32 bpp)
{
	INT32 x, x1, x2;
	INT32 col, ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfrac, yfracstep, position, count;
	fixed_t scale_y;
	RGBA_t colortemp;
	UINT8 *dest;
	const UINT8 *source;
	const column_t *patchcol;
	UINT8 alpha;
	UINT8 *block = mipmap->grInfo.data;
	UINT8 texel;
	UINT16 texelu16;

	if (!ptexturewidth)
		return;

	x1 = originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1 > ptexturewidth || x2 < 0)
		return; // patch not located within texture's x bounds, ignore

	if (originy > ptextureheight || (originy + SHORT(realpatch->height)) < 0)
		return; // patch not located within texture's y bounds, ignore

	// patch is actually inside the texture!
	// now check if texture is partly off-screen and adjust accordingly

	// left edge
	if (x1 < 0)
		x = 0;
	else
		x = x1;

	// right edge
	if (x2 > ptexturewidth)
		x2 = ptexturewidth;


	col = x * pblockwidth / ptexturewidth;
	ncols = ((x2 - x) * pblockwidth) / ptexturewidth;

/*
	CONS_Debug(DBG_RENDER, "patch %dx%d texture %dx%d block %dx%d\n", SHORT(realpatch->width),
															SHORT(realpatch->height),
															ptexturewidth,
															textureheight,
															pblockwidth,pblockheight);
	CONS_Debug(DBG_RENDER, "      col %d ncols %d x %d\n", col, ncols, x);
*/

	// source advance
	xfrac = 0;
	if (x1 < 0)
		xfrac = -x1<<FRACBITS;

	xfracstep = (ptexturewidth << FRACBITS) / pblockwidth;
	yfracstep = (ptextureheight<< FRACBITS) / pblockheight;
	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawPatchInCache: no drawer defined for this bpp (%d)\n",bpp);

	for (block += col*bpp; ncols--; block += bpp, xfrac += xfracstep)
	{
		INT32 topdelta, prevdelta = -1;
		patchcol = (const column_t *)((const UINT8 *)realpatch
		 + LONG(realpatch->columnofs[xfrac>>FRACBITS]));

		scale_y = (pblockheight << FRACBITS) / ptextureheight;

		while (patchcol->topdelta != 0xff)
		{
			topdelta = patchcol->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			source = (const UINT8 *)patchcol + 3;
			count  = ((patchcol->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
			position = originy + topdelta;

			yfrac = 0;
			//yfracstep = (patchcol->length << FRACBITS) / count;
			if (position < 0)
			{
				yfrac = -position<<FRACBITS;
				count += (((position * scale_y) + (FRACUNIT/2)) >> FRACBITS);
				position = 0;
			}

			position = ((position * scale_y) + (FRACUNIT/2)) >> FRACBITS;

			if (position < 0)
				position = 0;

			if (position + count >= pblockheight)
				count = pblockheight - position;

			dest = block + (position*blockmodulo);
			while (count > 0)
			{
				count--;

				texel = source[yfrac>>FRACBITS];

				if (firetranslucent && (transtables[(texel<<8)+0x40000]!=texel))
					alpha = 0x80;
				else
					alpha = 0xff;

				//Hurdler: not perfect, but better than holes
				if (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX && (mipmap->flags & TF_CHROMAKEYED))
					texel = HWR_CHROMAKEY_EQUIVALENTCOLORINDEX;
				//Hurdler: 25/04/2000: now support colormap in hardware mode
				else if (mipmap->colormap)
					texel = mipmap->colormap[texel];

				// hope compiler will get this switch out of the loops (dreams...)
				// gcc do it ! but vcc not ! (why don't use cygwin gcc for win32 ?)
				// Alam: SRB2 uses Mingw, HUGS
				switch (bpp)
				{
					case 2 : texelu16 = (UINT16)((alpha<<8) | texel);
					         memcpy(dest, &texelu16, sizeof(UINT16));
					         break;
					case 3 : colortemp = V_GetColor(texel);
					         memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(UINT8));
					         break;
					case 4 : colortemp = V_GetColor(texel);
					         colortemp.s.alpha = alpha;
					         memcpy(dest, &colortemp, sizeof(RGBA_t));
					         break;
					// default is 1
					default: *dest = texel;
					         break;
				}

				dest += blockmodulo;
				yfrac += yfracstep;
			}
			patchcol = (const column_t *)((const UINT8 *)patchcol + patchcol->length + 4);
		}
	}
}


// resize the patch to be 3dfx compliant
// set : blocksize = blockwidth * blockheight  (no bpp used)
//       blockwidth
//       blockheight
//note :  8bit (1 byte per pixel) palettized format
static void HWR_ResizeBlock(INT32 originalwidth, INT32 originalheight,
	GrTexInfo *grInfo)
{
	//   Build the full textures from patches.
	static const GrLOD_t gr_lods[9] =
	{
		GR_LOD_LOG2_256,
		GR_LOD_LOG2_128,
		GR_LOD_LOG2_64,
		GR_LOD_LOG2_32,
		GR_LOD_LOG2_16,
		GR_LOD_LOG2_8,
		GR_LOD_LOG2_4,
		GR_LOD_LOG2_2,
		GR_LOD_LOG2_1
	};

	typedef struct
	{
		GrAspectRatio_t aspect;
		float           max_s;
		float           max_t;
	} booring_aspect_t;

	static const booring_aspect_t gr_aspects[8] =
	{
		{GR_ASPECT_LOG2_1x1, 255, 255},
		{GR_ASPECT_LOG2_2x1, 255, 127},
		{GR_ASPECT_LOG2_4x1, 255,  63},
		{GR_ASPECT_LOG2_8x1, 255,  31},

		{GR_ASPECT_LOG2_1x1, 255, 255},
		{GR_ASPECT_LOG2_1x2, 127, 255},
		{GR_ASPECT_LOG2_1x4,  63, 255},
		{GR_ASPECT_LOG2_1x8,  31, 255}
	};

	INT32     j,k;
	INT32     max,min;

	// find a power of 2 width/height
	if (cv_grrounddown.value)
	{
		blockwidth = 256;
		while (originalwidth < blockwidth)
			blockwidth >>= 1;
		if (blockwidth < 1)
			I_Error("3D GenerateTexture : too small");

		blockheight = 256;
		while (originalheight < blockheight)
			blockheight >>= 1;
		if (blockheight < 1)
			I_Error("3D GenerateTexture : too small");
	}
	else if (cv_voodoocompatibility.value)
	{
		if (originalwidth > 256 || originalheight > 256)
		{
			blockwidth = 256;
			while (originalwidth < blockwidth)
				blockwidth >>= 1;
			if (blockwidth < 1)
				I_Error("3D GenerateTexture : too small");

			blockheight = 256;
			while (originalheight < blockheight)
				blockheight >>= 1;
			if (blockheight < 1)
				I_Error("3D GenerateTexture : too small");
		}
		else
		{
			//size up to nearest power of 2
			blockwidth = 1;
			while (blockwidth < originalwidth)
				blockwidth <<= 1;
			// scale down the original graphics to fit in 256
			if (blockwidth > 256)
				blockwidth = 256;
				//I_Error("3D GenerateTexture : too big");

			//size up to nearest power of 2
			blockheight = 1;
			while (blockheight < originalheight)
				blockheight <<= 1;
			// scale down the original graphics to fit in 256
			if (blockheight > 256)
				blockheight = 255;
				//I_Error("3D GenerateTexture : too big");
		}
	}
	else
	{
		//size up to nearest power of 2
		blockwidth = 1;
		while (blockwidth < originalwidth)
			blockwidth <<= 1;
		// scale down the original graphics to fit in 256
		if (blockwidth > 2048)
			blockwidth = 2048;
			//I_Error("3D GenerateTexture : too big");

		//size up to nearest power of 2
		blockheight = 1;
		while (blockheight < originalheight)
			blockheight <<= 1;
		// scale down the original graphics to fit in 256
		if (blockheight > 2048)
			blockheight = 2048;
			//I_Error("3D GenerateTexture : too big");
	}

	// do the boring LOD stuff.. blech!
	if (blockwidth >= blockheight)
	{
		max = blockwidth;
		min = blockheight;
	}
	else
	{
		max = blockheight;
		min = blockwidth;
	}

	for (k = 2048, j = 0; k > max; j++)
		k>>=1;
	grInfo->smallLodLog2 = gr_lods[j];
	grInfo->largeLodLog2 = gr_lods[j];

	for (k = max, j = 0; k > min && j < 4; j++)
		k>>=1;
	// aspect ratio too small for 3Dfx (eg: 8x128 is 1x16 : use 1x8)
	if (j == 4)
	{
		j = 3;
		//CONS_Debug(DBG_RENDER, "HWR_ResizeBlock : bad aspect ratio %dx%d\n", blockwidth,blockheight);
		if (blockwidth < blockheight)
			blockwidth = max>>3;
		else
			blockheight = max>>3;
	}
	if (blockwidth < blockheight)
		j += 4;
	grInfo->aspectRatioLog2 = gr_aspects[j].aspect;

	blocksize = blockwidth * blockheight;

	//CONS_Debug(DBG_RENDER, "Width is %d, Height is %d\n", blockwidth, blockheight);
}


static const INT32 format2bpp[16] =
{
	0, //0
	0, //1
	1, //2  GR_TEXFMT_ALPHA_8
	1, //3  GR_TEXFMT_INTENSITY_8
	1, //4  GR_TEXFMT_ALPHA_INTENSITY_44
	1, //5  GR_TEXFMT_P_8
	4, //6  GR_RGBA
	0, //7
	0, //8
	0, //9
	2, //10 GR_TEXFMT_RGB_565
	2, //11 GR_TEXFMT_ARGB_1555
	2, //12 GR_TEXFMT_ARGB_4444
	2, //13 GR_TEXFMT_ALPHA_INTENSITY_88
	2, //14 GR_TEXFMT_AP_88
};

static UINT8 *MakeBlock(GLMipmap_t *grMipmap)
{
	UINT8 *block;
	INT32 bpp, i;
	UINT16 bu16 = ((0x00 <<8) | HWR_CHROMAKEY_EQUIVALENTCOLORINDEX);

	bpp =  format2bpp[grMipmap->grInfo.format];
	block = Z_Malloc(blocksize*bpp, PU_HWRCACHE, &(grMipmap->grInfo.data));

	switch (bpp)
	{
		case 1: memset(block, HWR_PATCHES_CHROMAKEY_COLORINDEX, blocksize); break;
		case 2:
				// fill background with chromakey, alpha = 0
				for (i = 0; i < blocksize; i++)
				//[segabor]
					memcpy(block+i*sizeof(UINT16), &bu16, sizeof(UINT16));
				break;
		case 4: memset(block, 0x00, blocksize*sizeof(UINT32)); break;
	}

	return block;
}

//
// Create a composite texture from patches, adapt the texture size to a power of 2
// height and width for the hardware texture cache.
//
static void HWR_GenerateTexture(INT32 texnum, GLTexture_t *grtex)
{
	UINT8 *block;
	texture_t *texture;
	texpatch_t *patch;
	patch_t *realpatch;

	INT32 i;
	boolean skyspecial = false; //poor hack for Legacy large skies..

	texture = textures[texnum];

	// hack the Legacy skies..
	if (texture->name[0] == 'S' &&
	    texture->name[1] == 'K' &&
	    texture->name[2] == 'Y' &&
	    (texture->name[4] == 0 ||
	     texture->name[5] == 0)
	   )
	{
		skyspecial = true;
		grtex->mipmap.flags = TF_WRAPXY; // don't use the chromakey for sky
	}
	else
		grtex->mipmap.flags = TF_CHROMAKEYED | TF_WRAPXY;

	HWR_ResizeBlock (texture->width, texture->height, &grtex->mipmap.grInfo);
	grtex->mipmap.width = (UINT16)blockwidth;
	grtex->mipmap.height = (UINT16)blockheight;
	grtex->mipmap.grInfo.format = textureformat;

	block = MakeBlock(&grtex->mipmap);

	if (skyspecial) //Hurdler: not efficient, but better than holes in the sky (and it's done only at level loading)
	{
		INT32 j;
		RGBA_t col;

		col = V_GetColor(HWR_CHROMAKEY_EQUIVALENTCOLORINDEX);
		for (j = 0; j < blockheight; j++)
		{
			for (i = 0; i < blockwidth; i++)
			{
				block[4*(j*blockwidth+i)+0] = col.s.red;
				block[4*(j*blockwidth+i)+1] = col.s.green;
				block[4*(j*blockwidth+i)+2] = col.s.blue;
				block[4*(j*blockwidth+i)+3] = 0xff;
			}
		}
	}

	// Composite the columns together.
	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		realpatch = W_CacheLumpNumPwad(patch->wad, patch->lump, PU_CACHE);
		HWR_DrawPatchInCache(&grtex->mipmap,
		                     blockwidth, blockheight,
		                     blockwidth*format2bpp[grtex->mipmap.grInfo.format],
		                     texture->width, texture->height,
		                     patch->originx, patch->originy,
		                     realpatch,
		                     format2bpp[grtex->mipmap.grInfo.format]);
		Z_Unlock(realpatch);
	}
	//Hurdler: not efficient at all but I don't remember exactly how HWR_DrawPatchInCache works :(
	if (format2bpp[grtex->mipmap.grInfo.format]==4)
	{
		for (i = 3; i < blocksize*4; i += 4) // blocksize*4 because blocksize doesn't include the bpp
		{
			if (block[i] == 0)
			{
				grtex->mipmap.flags |= TF_TRANSPARENT;
				break;
			}
		}
	}

	grtex->scaleX = 1.0f/(texture->width*FRACUNIT);
	grtex->scaleY = 1.0f/(texture->height*FRACUNIT);
}

// patch may be NULL if grMipmap has been initialised already and makebitmap is false
void HWR_MakePatch (const patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, boolean makebitmap)
{
	INT32 newwidth, newheight;

	// don't do it twice (like a cache)
	if (grMipmap->width == 0)
	{
		// save the original patch header so that the GLPatch can be casted
		// into a standard patch_t struct and the existing code can get the
		// orginal patch dimensions and offsets.
		grPatch->width = SHORT(patch->width);
		grPatch->height = SHORT(patch->height);
		grPatch->leftoffset = SHORT(patch->leftoffset);
		grPatch->topoffset = SHORT(patch->topoffset);

		// find the good 3dfx size (boring spec)
		HWR_ResizeBlock (SHORT(patch->width), SHORT(patch->height), &grMipmap->grInfo);
		grMipmap->width = (UINT16)blockwidth;
		grMipmap->height = (UINT16)blockheight;

		// no wrap around, no chroma key
		grMipmap->flags = 0;
		// setup the texture info
		grMipmap->grInfo.format = patchformat;
	}
	else
	{
		blockwidth = grMipmap->width;
		blockheight = grMipmap->height;
		blocksize = blockwidth * blockheight;
	}

	Z_Free(grMipmap->grInfo.data);
	grMipmap->grInfo.data = NULL;

	// if rounddown, rounddown patches as well as textures
	if (cv_grrounddown.value)
	{
		newwidth = blockwidth;
		newheight = blockheight;
	}
	else if (cv_voodoocompatibility.value) // Only scales down textures that exceed 256x256.
	{
		// no rounddown, do not size up patches, so they don't look 'scaled'
		newwidth  = min(grPatch->width, blockwidth);
		newheight = min(grPatch->height, blockheight);

		if (newwidth > 256 || newheight > 256)
		{
			newwidth = blockwidth;
			newheight = blockheight;
		}
	}
	else
	{
		// no rounddown, do not size up patches, so they don't look 'scaled'
		newwidth  = min(grPatch->width, blockwidth);
		newheight = min(grPatch->height, blockheight);
	}

	if (makebitmap)
	{
		MakeBlock(grMipmap);

		HWR_DrawPatchInCache(grMipmap,
			newwidth, newheight,
			blockwidth*format2bpp[grMipmap->grInfo.format],
			grPatch->width, grPatch->height,
			0, 0,
			patch,
			format2bpp[grMipmap->grInfo.format]);
	}

	grPatch->max_s = (float)newwidth / (float)blockwidth;
	grPatch->max_t = (float)newheight / (float)blockheight;
}


// =================================================
//             CACHING HANDLING
// =================================================

static size_t gr_numtextures;
static GLTexture_t *gr_textures; // for ALL Doom textures

void HWR_InitTextureCache(void)
{
	gr_numtextures = 0;
	gr_textures = NULL;
}


// Callback function for HWR_FreeTextureCache.
static void FreeMipmapColormap(INT32 patchnum, void *patch)
{
	GLPatch_t* const grpatch = patch;
	(void)patchnum; //unused
	while (grpatch->mipmap.nextcolormap)
	{
		GLMipmap_t *grmip = grpatch->mipmap.nextcolormap;
		grpatch->mipmap.nextcolormap = grmip->nextcolormap;
		if (grmip->grInfo.data) Z_Free(grmip->grInfo.data);
		free(grmip);
	}
}

void HWR_FreeTextureCache(void)
{
	INT32 i;
	// free references to the textures
	HWD.pfnClearMipMapCache();

	// free all hardware-converted graphics cached in the heap
	// our gool is only the textures since user of the texture is the texture cache
	Z_FreeTags(PU_HWRCACHE, PU_HWRCACHE);
	Z_FreeTags(PU_HWRCACHE_UNLOCKED, PU_HWRCACHE_UNLOCKED);

	// Alam: free the Z_Blocks before freeing it's users

	// free all skin after each level: must be done after pfnClearMipMapCache!
	for (i = 0; i < numwadfiles; i++)
		M_AATreeIterate(wadfiles[i]->hwrcache, FreeMipmapColormap);

	// now the heap don't have any 'user' pointing to our
	// texturecache info, we can free it
	if (gr_textures)
		free(gr_textures);
	gr_textures = NULL;
	gr_numtextures = 0;
}

void HWR_PrepLevelCache(size_t pnumtextures)
{
	// problem: the mipmap cache management hold a list of mipmaps.. but they are
	//           reallocated on each level..
	//sub-optimal, but 1) just need re-download stuff in hardware cache VERY fast
	//   2) sprite/menu stuff mixed with level textures so can't do anything else

	// we must free it since numtextures changed
	HWR_FreeTextureCache();

	gr_numtextures = pnumtextures;
	gr_textures = calloc(pnumtextures, sizeof (*gr_textures));
	if (gr_textures == NULL)
		I_Error("3D can't alloc gr_textures");
}

void HWR_SetPalette(RGBA_t *palette)
{
	//Hudler: 16/10/99: added for OpenGL gamma correction
	RGBA_t gamma_correction = {0x7F7F7F7F};

	//Hurdler 16/10/99: added for OpenGL gamma correction
	gamma_correction.s.red   = (UINT8)cv_grgammared.value;
	gamma_correction.s.green = (UINT8)cv_grgammagreen.value;
	gamma_correction.s.blue  = (UINT8)cv_grgammablue.value;
	HWD.pfnSetPalette(palette, &gamma_correction);

	// hardware driver will flush there own cache if cache is non paletized
	// now flush data texture cache so 32 bit texture are recomputed
	if (patchformat == GR_RGBA || textureformat == GR_RGBA)
	{
		Z_FreeTags(PU_HWRCACHE, PU_HWRCACHE);
		Z_FreeTags(PU_HWRCACHE_UNLOCKED, PU_HWRCACHE_UNLOCKED);
	}
}

// --------------------------------------------------------------------------
// Make sure texture is downloaded and set it as the source
// --------------------------------------------------------------------------
GLTexture_t *HWR_GetTexture(INT32 tex)
{
	GLTexture_t *grtex;
#ifdef PARANOIA
	if ((unsigned)tex >= gr_numtextures)
		I_Error(" HWR_GetTexture: tex >= numtextures\n");
#endif
	grtex = &gr_textures[tex];

	if (!grtex->mipmap.grInfo.data && !grtex->mipmap.downloaded)
		HWR_GenerateTexture(tex, grtex);

	HWD.pfnSetTexture(&grtex->mipmap);

	// The system-memory data can be purged now.
	Z_ChangeTag(grtex->mipmap.grInfo.data, PU_HWRCACHE_UNLOCKED);

	return grtex;
}


static void HWR_CacheFlat(GLMipmap_t *grMipmap, lumpnum_t flatlumpnum)
{
	size_t size, pflatsize;

	// setup the texture info
	grMipmap->grInfo.smallLodLog2 = GR_LOD_LOG2_64;
	grMipmap->grInfo.largeLodLog2 = GR_LOD_LOG2_64;
	grMipmap->grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
	grMipmap->grInfo.format = GR_TEXFMT_P_8;
	grMipmap->flags = TF_WRAPXY|TF_CHROMAKEYED;

	size = W_LumpLength(flatlumpnum);

	switch (size)
	{
		case 4194304: // 2048x2048 lump
			pflatsize = 2048;
			break;
		case 1048576: // 1024x1024 lump
			pflatsize = 1024;
			break;
		case 262144:// 512x512 lump
			pflatsize = 512;
			break;
		case 65536: // 256x256 lump
			pflatsize = 256;
			break;
		case 16384: // 128x128 lump
			pflatsize = 128;
			break;
		case 1024: // 32x32 lump
			pflatsize = 32;
			break;
		default: // 64x64 lump
			pflatsize = 64;
			break;
	}
	grMipmap->width  = (UINT16)pflatsize;
	grMipmap->height = (UINT16)pflatsize;

	// the flat raw data needn't be converted with palettized textures
	W_ReadLump(flatlumpnum, Z_Malloc(W_LumpLength(flatlumpnum),
		PU_HWRCACHE, &grMipmap->grInfo.data));
}


// Download a Doom 'flat' to the hardware cache and make it ready for use
void HWR_GetFlat(lumpnum_t flatlumpnum)
{
	GLMipmap_t *grmip;

	grmip = &HWR_GetCachedGLPatch(flatlumpnum)->mipmap;

	if (!grmip->downloaded && !grmip->grInfo.data)
		HWR_CacheFlat(grmip, flatlumpnum);

	HWD.pfnSetTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->grInfo.data, PU_HWRCACHE_UNLOCKED);
}

//
// HWR_LoadMappedPatch(): replace the skin color of the sprite in cache
//                          : load it first in doom cache if not already
//
static void HWR_LoadMappedPatch(GLMipmap_t *grmip, GLPatch_t *gpatch)
{
	if (!grmip->downloaded && !grmip->grInfo.data)
	{
		patch_t *patch = W_CacheLumpNumPwad(gpatch->wadnum, gpatch->lumpnum, PU_STATIC);
		HWR_MakePatch(patch, gpatch, grmip, true);

		Z_Free(patch);
	}

	HWD.pfnSetTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->grInfo.data, PU_HWRCACHE_UNLOCKED);
}

// -----------------+
// HWR_GetPatch     : Download a patch to the hardware cache and make it ready for use
// -----------------+
void HWR_GetPatch(GLPatch_t *gpatch)
{
	// is it in hardware cache
	if (!gpatch->mipmap.downloaded && !gpatch->mipmap.grInfo.data)
	{
		// load the software patch, PU_STATIC or the Z_Malloc for hardware patch will
		// flush the software patch before the conversion! oh yeah I suffered
		patch_t *ptr = W_CacheLumpNumPwad(gpatch->wadnum, gpatch->lumpnum, PU_STATIC);
		HWR_MakePatch(ptr, gpatch, &gpatch->mipmap, true);

		// this is inefficient.. but the hardware patch in heap is purgeable so it should
		// not fragment memory, and besides the REAL cache here is the hardware memory
		Z_Free(ptr);
	}

	HWD.pfnSetTexture(&gpatch->mipmap);

	// The system-memory patch data can be purged now.
	Z_ChangeTag(gpatch->mipmap.grInfo.data, PU_HWRCACHE_UNLOCKED);
}


// -------------------+
// HWR_GetMappedPatch : Same as HWR_GetPatch for sprite color
// -------------------+
void HWR_GetMappedPatch(GLPatch_t *gpatch, const UINT8 *colormap)
{
	GLMipmap_t *grmip, *newmip;

	if (colormap == colormaps || colormap == NULL)
	{
		// Load the default (green) color in doom cache (temporary?) AND hardware cache
		HWR_GetPatch(gpatch);
		return;
	}

	// search for the mimmap
	// skip the first (no colormap translated)
	for (grmip = &gpatch->mipmap; grmip->nextcolormap; )
	{
		grmip = grmip->nextcolormap;
		if (grmip->colormap == colormap)
		{
			HWR_LoadMappedPatch(grmip, gpatch);
			return;
		}
	}
	// not found, create it!
	// If we are here, the sprite with the current colormap is not already in hardware memory

	//BP: WARNING: don't free it manually without clearing the cache of harware renderer
	//              (it have a liste of mipmap)
	//    this malloc is cleared in HWR_FreeTextureCache
	//    (...) unfortunately z_malloc fragment alot the memory :(so malloc is better
	newmip = calloc(1, sizeof (*newmip));
	if (newmip == NULL)
		I_Error("%s: Out of memory", "HWR_GetMappedPatch");
	grmip->nextcolormap = newmip;

	newmip->colormap = colormap;
	HWR_LoadMappedPatch(newmip, gpatch);
}

void HWR_UnlockCachedPatch(GLPatch_t *gpatch)
{
	if (!gpatch)
		return;

	Z_ChangeTag(gpatch->mipmap.grInfo.data, PU_HWRCACHE_UNLOCKED);
	Z_ChangeTag(gpatch, PU_HWRPATCHINFO_UNLOCKED);
}

static const INT32 picmode2GR[] =
{
	GR_TEXFMT_P_8,                // PALETTE
	0,                            // INTENSITY          (unsupported yet)
	GR_TEXFMT_ALPHA_INTENSITY_88, // INTENSITY_ALPHA    (corona use this)
	0,                            // RGB24              (unsupported yet)
	GR_RGBA,                      // RGBA32             (opengl only)
};

static void HWR_DrawPicInCache(UINT8 *block, INT32 pblockwidth, INT32 pblockheight,
	INT32 blockmodulo, pic_t *pic, INT32 bpp)
{
	INT32 i,j;
	fixed_t posx, posy, stepx, stepy;
	UINT8 *dest, *src, texel;
	UINT16 texelu16;
	INT32 picbpp;
	RGBA_t col;

	stepy = ((INT32)SHORT(pic->height)<<FRACBITS)/pblockheight;
	stepx = ((INT32)SHORT(pic->width)<<FRACBITS)/pblockwidth;
	picbpp = format2bpp[picmode2GR[pic->mode]];
	posy = 0;
	for (j = 0; j < pblockheight; j++)
	{
		posx = 0;
		dest = &block[j*blockmodulo];
		src = &pic->data[(posy>>FRACBITS)*SHORT(pic->width)*picbpp];
		for (i = 0; i < pblockwidth;i++)
		{
			switch (pic->mode)
			{ // source bpp
				case PALETTE :
					texel = src[(posx+FRACUNIT/2)>>FRACBITS];
					switch (bpp)
					{ // destination bpp
						case 1 :
							*dest++ = texel; break;
						case 2 :
							texelu16 = (UINT16)(texel | 0xff00);
							memcpy(dest, &texelu16, sizeof(UINT16));
							dest += sizeof(UINT16);
							break;
						case 3 :
							col = V_GetColor(texel);
							memcpy(dest, &col, sizeof(RGBA_t)-sizeof(UINT8));
							dest += sizeof(RGBA_t)-sizeof(UINT8);
							break;
						case 4 :
							memcpy(dest, &V_GetColor(texel), sizeof(RGBA_t));
							dest += sizeof(RGBA_t);
							break;
					}
					break;
				case INTENSITY :
					*dest++ = src[(posx+FRACUNIT/2)>>FRACBITS];
					break;
				case INTENSITY_ALPHA : // assume dest bpp = 2
					memcpy(dest, src + ((posx+FRACUNIT/2)>>FRACBITS)*sizeof(UINT16), sizeof(UINT16));
					dest += sizeof(UINT16);
					break;
				case RGB24 :
					break;  // not supported yet
				case RGBA32 : // assume dest bpp = 4
					dest += sizeof(UINT32);
					memcpy(dest, src + ((posx+FRACUNIT/2)>>FRACBITS)*sizeof(UINT32), sizeof(UINT32));
					break;
			}
			posx += stepx;
		}
		posy += stepy;
	}
}

// -----------------+
// HWR_GetPic       : Download a Doom pic (raw row encoded with no 'holes')
// Returns          :
// -----------------+
GLPatch_t *HWR_GetPic(lumpnum_t lumpnum)
{
	GLPatch_t *grpatch;

	grpatch = HWR_GetCachedGLPatch(lumpnum);

	if (!grpatch->mipmap.downloaded && !grpatch->mipmap.grInfo.data)
	{
		pic_t *pic;
		UINT8 *block;
		size_t len;
		INT32 newwidth, newheight;

		pic = W_CacheLumpNum(lumpnum, PU_CACHE);
		grpatch->width = SHORT(pic->width);
		grpatch->height = SHORT(pic->height);
		len = W_LumpLength(lumpnum) - sizeof (pic_t);

		grpatch->leftoffset = 0;
		grpatch->topoffset = 0;

		// find the good 3dfx size (boring spec)
		HWR_ResizeBlock (grpatch->width, grpatch->height, &grpatch->mipmap.grInfo);
		grpatch->mipmap.width = (UINT16)blockwidth;
		grpatch->mipmap.height = (UINT16)blockheight;

		if (pic->mode == PALETTE)
			grpatch->mipmap.grInfo.format = textureformat; // can be set by driver
		else
			grpatch->mipmap.grInfo.format = picmode2GR[pic->mode];

		Z_Free(grpatch->mipmap.grInfo.data);

		// allocate block
		block = MakeBlock(&grpatch->mipmap);

		// if rounddown, rounddown patches as well as textures
		if (cv_grrounddown.value)
		{
			newwidth = blockwidth;
			newheight = blockheight;
		}
		else if (cv_voodoocompatibility.value) // Only scales down textures that exceed 256x256.
		{
			// no rounddown, do not size up patches, so they don't look 'scaled'
			newwidth  = min(SHORT(pic->width),blockwidth);
			newheight = min(SHORT(pic->height),blockheight);

			if (newwidth > 256 || newheight > 256)
			{
				newwidth = blockwidth;
				newheight = blockheight;
			}
		}
		else
		{
			// no rounddown, do not size up patches, so they don't look 'scaled'
			newwidth  = min(SHORT(pic->width),blockwidth);
			newheight = min(SHORT(pic->height),blockheight);
		}


		if (grpatch->width  == blockwidth &&
			grpatch->height == blockheight &&
			format2bpp[grpatch->mipmap.grInfo.format] == format2bpp[picmode2GR[pic->mode]])
		{
			// no conversion needed
			M_Memcpy(grpatch->mipmap.grInfo.data, pic->data,len);
		}
		else
			HWR_DrawPicInCache(block, newwidth, newheight,
			                   blockwidth*format2bpp[grpatch->mipmap.grInfo.format],
			                   pic,
			                   format2bpp[grpatch->mipmap.grInfo.format]);

		Z_Unlock(pic);
		Z_ChangeTag(block, PU_HWRCACHE_UNLOCKED);

		grpatch->mipmap.flags = 0;
		grpatch->max_s = (float)newwidth  / (float)blockwidth;
		grpatch->max_t = (float)newheight / (float)blockheight;
	}
	HWD.pfnSetTexture(&grpatch->mipmap);
	//CONS_Debug(DBG_RENDER, "picloaded at %x as texture %d\n",grpatch->mipmap.grInfo.data, grpatch->mipmap.downloaded);

	return grpatch;
}

GLPatch_t *HWR_GetCachedGLPatchPwad(UINT16 wadnum, UINT16 lumpnum)
{
	aatree_t *hwrcache = wadfiles[wadnum]->hwrcache;
	GLPatch_t *grpatch;

	if (!(grpatch = M_AATreeGet(hwrcache, lumpnum)))
	{
		grpatch = Z_Calloc(sizeof(GLPatch_t), PU_HWRPATCHINFO, NULL);
		grpatch->wadnum = wadnum;
		grpatch->lumpnum = lumpnum;
		M_AATreeSet(hwrcache, lumpnum, grpatch);
	}

	return grpatch;
}

GLPatch_t *HWR_GetCachedGLPatch(lumpnum_t lumpnum)
{
	return HWR_GetCachedGLPatchPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum));
}

// Need to do this because they aren't powers of 2
static void HWR_DrawFadeMaskInCache(GLMipmap_t *mipmap, INT32 pblockwidth, INT32 pblockheight,
	lumpnum_t fademasklumpnum, UINT16 fmwidth, UINT16 fmheight)
{
	INT32 i,j;
	fixed_t posx, posy, stepx, stepy;
	UINT8 *block = mipmap->grInfo.data; // places the data directly into here, it already has the space allocated from HWR_ResizeBlock
	UINT8 *flat;
	UINT8 *dest, *src, texel;
	RGBA_t col;

	// Place the flats data into flat
	W_ReadLump(fademasklumpnum, Z_Malloc(W_LumpLength(fademasklumpnum),
		PU_HWRCACHE, &flat));

	stepy = ((INT32)SHORT(fmheight)<<FRACBITS)/pblockheight;
	stepx = ((INT32)SHORT(fmwidth)<<FRACBITS)/pblockwidth;
	posy = 0;
	for (j = 0; j < pblockheight; j++)
	{
		posx = 0;
		dest = &block[j*blockwidth]; // 1bpp
		src = &flat[(posy>>FRACBITS)*SHORT(fmwidth)];
		for (i = 0; i < pblockwidth;i++)
		{
			// fademask bpp is always 1, and is used just for alpha
			texel = src[(posx)>>FRACBITS];
			col = V_GetColor(texel);
			*dest = col.s.red; // take the red level of the colour and use it for alpha, as fademasks do

			dest++;
			posx += stepx;
		}
		posy += stepy;
	}

	Z_Free(flat);
}

static void HWR_CacheFadeMask(GLMipmap_t *grMipmap, lumpnum_t fademasklumpnum)
{
	size_t size;
	UINT16 fmheight = 0, fmwidth = 0;

	// setup the texture info
	grMipmap->grInfo.format = GR_TEXFMT_ALPHA_8; // put the correct alpha levels straight in so I don't need to convert it later
	grMipmap->flags = 0;

	size = W_LumpLength(fademasklumpnum);

	switch (size)
	{
		// None of these are powers of 2, so I'll need to do what is done for textures and make them powers of 2 before they can be used
		case 256000: // 640x400
			fmwidth = 640;
			fmheight = 400;
			break;
		case 64000: // 320x200
			fmwidth = 320;
			fmheight = 200;
			break;
		case 16000: // 160x100
			fmwidth = 160;
			fmheight = 100;
			break;
		case 4000: // 80x50 (minimum)
			fmwidth = 80;
			fmheight = 50;
			break;
		default: // Bad lump
			CONS_Alert(CONS_WARNING, "Fade mask lump of incorrect size, ignored\n"); // I should avoid this by checking the lumpnum in HWR_RunWipe
			break;
	}

	// Thankfully, this will still work for this scenario
	HWR_ResizeBlock(fmwidth, fmheight, &grMipmap->grInfo);

	grMipmap->width  = blockwidth;
	grMipmap->height = blockheight;

	MakeBlock(grMipmap);

	HWR_DrawFadeMaskInCache(grMipmap, blockwidth, blockheight, fademasklumpnum, fmwidth, fmheight);

	// I DO need to convert this because it isn't power of 2 and we need the alpha
}


void HWR_GetFadeMask(lumpnum_t fademasklumpnum)
{
	GLMipmap_t *grmip;

	grmip = &HWR_GetCachedGLPatch(fademasklumpnum)->mipmap;

	if (!grmip->downloaded && !grmip->grInfo.data)
		HWR_CacheFadeMask(grmip, fademasklumpnum);

	HWD.pfnSetTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->grInfo.data, PU_HWRCACHE_UNLOCKED);
}

#endif //HWRENDER
