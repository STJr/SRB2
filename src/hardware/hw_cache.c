// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_cache.c
/// \brief load and convert graphics to the hardware format

#include "../doomdef.h"

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_batching.h"

#include "../doomstat.h"    //gamemode
#include "../i_video.h"     //rendermode
#include "../r_data.h"
#include "../r_textures.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../v_video.h"
#include "../r_draw.h"
#include "../r_picformats.h"
#include "../p_setup.h"

INT32 patchformat = GL_TEXFMT_AP_88; // use alpha for holes
INT32 textureformat = GL_TEXFMT_P_8; // use chromakey for hole

static INT32 format2bpp(GLTextureFormat_t format)
{
	if (format == GL_TEXFMT_RGBA)
		return 4;
	else if (format == GL_TEXFMT_ALPHA_INTENSITY_88 || format == GL_TEXFMT_AP_88)
		return 2;
	else
		return 1;
}

// This code was originally placed directly in HWR_DrawPatchInCache.
// It is now split from it for my sanity! (and the sanity of others)
// -- Monster Iestyn (13/02/19)
static void HWR_DrawColumnInCache(const column_t *patchcol, UINT8 *block, GLMipmap_t *mipmap,
								INT32 pblockheight, INT32 blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, INT32 patchheight,
								INT32 bpp)
{
	fixed_t yfrac, position, count;
	UINT8 *dest;
	const UINT8 *source;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = 0;

	// for writing a pixel to dest
	RGBA_t colortemp;
	UINT8 alpha;
	UINT8 texel;
	UINT16 texelu16;

	(void)patchheight; // This parameter is unused

	if (originPatch) // originPatch can be NULL here, unlike in the software version
		originy = originPatch->originy;

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
			alpha = 0xFF;
			// Make pixel transparent if chroma keyed
			if ((mipmap->flags & TF_CHROMAKEYED) && (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX))
				alpha = 0x00;

			//Hurdler: 25/04/2000: now support colormap in hardware mode
			if (mipmap->colormap)
				texel = mipmap->colormap[texel];

			// hope compiler will get this switch out of the loops (dreams...)
			// gcc do it ! but vcc not ! (why don't use cygwin gcc for win32 ?)
			// Alam: SRB2 uses Mingw, HUGS
			switch (bpp)
			{
				case 2 : // uhhhhhhhh..........
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 texel = ASTBlendPaletteIndexes(*(dest+1), texel, originPatch->style, originPatch->alpha);
						 texelu16 = (UINT16)((alpha<<8) | texel);
						 memcpy(dest, &texelu16, sizeof(UINT16));
						 break;
				case 3 : colortemp = V_GetColor(texel);
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(UINT32 *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(UINT8));
						 break;
				case 4 : colortemp = V_GetColor(texel);
						 colortemp.s.alpha = alpha;
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(UINT32 *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t));
						 break;
				// default is 1
				default:
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 *dest = ASTBlendPaletteIndexes(*dest, texel, originPatch->style, originPatch->alpha);
						 else
							 *dest = texel;
						 break;
			}

			dest += blockmodulo;
			yfrac += yfracstep;
		}
		patchcol = (const column_t *)((const UINT8 *)patchcol + patchcol->length + 4);
	}
}

static void HWR_DrawFlippedColumnInCache(const column_t *patchcol, UINT8 *block, GLMipmap_t *mipmap,
								INT32 pblockheight, INT32 blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, INT32 patchheight,
								INT32 bpp)
{
	fixed_t yfrac, position, count;
	UINT8 *dest;
	const UINT8 *source;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = 0;

	// for writing a pixel to dest
	RGBA_t colortemp;
	UINT8 alpha;
	UINT8 texel;
	UINT16 texelu16;

	if (originPatch) // originPatch can be NULL here, unlike in the software version
		originy = originPatch->originy;

	while (patchcol->topdelta != 0xff)
	{
		topdelta = patchcol->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = patchheight-patchcol->length-topdelta;
		source = (const UINT8 *)patchcol + 3;
		count  = ((patchcol->length * scale_y) + (FRACUNIT/2)) >> FRACBITS;
		position = originy + topdelta;

		yfrac = (patchcol->length-1) << FRACBITS;

		if (position < 0)
		{
			yfrac += position<<FRACBITS;
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
			alpha = 0xFF;
			// Make pixel transparent if chroma keyed
			if ((mipmap->flags & TF_CHROMAKEYED) && (texel == HWR_PATCHES_CHROMAKEY_COLORINDEX))
				alpha = 0x00;

			//Hurdler: 25/04/2000: now support colormap in hardware mode
			if (mipmap->colormap)
				texel = mipmap->colormap[texel];

			// hope compiler will get this switch out of the loops (dreams...)
			// gcc do it ! but vcc not ! (why don't use cygwin gcc for win32 ?)
			// Alam: SRB2 uses Mingw, HUGS
			switch (bpp)
			{
				case 2 : // uhhhhhhhh..........
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 texel = ASTBlendPaletteIndexes(*(dest+1), texel, originPatch->style, originPatch->alpha);
						 texelu16 = (UINT16)((alpha<<8) | texel);
						 memcpy(dest, &texelu16, sizeof(UINT16));
						 break;
				case 3 : colortemp = V_GetColor(texel);
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(UINT32 *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t)-sizeof(UINT8));
						 break;
				case 4 : colortemp = V_GetColor(texel);
						 colortemp.s.alpha = alpha;
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
						 {
							 RGBA_t rgbatexel;
							 rgbatexel.rgba = *(UINT32 *)dest;
							 colortemp.rgba = ASTBlendTexturePixel(rgbatexel, colortemp, originPatch->style, originPatch->alpha);
						 }
						 memcpy(dest, &colortemp, sizeof(RGBA_t));
						 break;
				// default is 1
				default:
						 if ((originPatch != NULL) && (originPatch->style != AST_COPY))
							 *dest = ASTBlendPaletteIndexes(*dest, texel, originPatch->style, originPatch->alpha);
						 else
							 *dest = texel;
						 break;
			}

			dest += blockmodulo;
			yfrac -= yfracstep;
		}
		patchcol = (const column_t *)((const UINT8 *)patchcol + patchcol->length + 4);
	}
}


// Simplified patch caching function
// for use by sprites and other patches that are not part of a wall texture
// no alpha or flipping should be present since we do not want non-texture graphics to have them
// no offsets are used either
// -- Monster Iestyn (13/02/19)
static void HWR_DrawPatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight,
	INT32 pwidth, INT32 pheight,
	const patch_t *realpatch)
{
	INT32 ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfracstep, scale_y;
	const column_t *patchcol;
	UINT8 *block = mipmap->data;
	INT32 bpp;
	INT32 blockmodulo;

	if (pwidth <= 0 || pheight <= 0)
		return;

	ncols = pwidth;

	// source advance
	xfrac = 0;
	xfracstep = FRACUNIT;
	yfracstep = FRACUNIT;
	scale_y   = FRACUNIT;

	bpp = format2bpp(mipmap->format);

	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawPatchInCache: no drawer defined for this bpp (%d)\n",bpp);

	// NOTE: should this actually be pblockwidth*bpp?
	blockmodulo = pblockwidth*bpp;

	// Draw each column to the block cache
	for (; ncols--; block += bpp, xfrac += xfracstep)
	{
		patchcol = (const column_t *)((const UINT8 *)realpatch + LONG(realpatch->columnofs[xfrac>>FRACBITS]));

		HWR_DrawColumnInCache(patchcol, block, mipmap,
								pblockheight, blockmodulo,
								yfracstep, scale_y,
								NULL, pheight, // not that pheight is going to get used anyway...
								bpp);
	}
}

// This function we use for caching patches that belong to textures
static void HWR_DrawTexturePatchInCache(GLMipmap_t *mipmap,
	INT32 pblockwidth, INT32 pblockheight,
	texture_t *texture, texpatch_t *patch,
	const patch_t *realpatch)
{
	INT32 x, x1, x2;
	INT32 col, ncols;
	fixed_t xfrac, xfracstep;
	fixed_t yfracstep, scale_y;
	const column_t *patchcol;
	UINT8 *block = mipmap->data;
	INT32 bpp;
	INT32 blockmodulo;
	INT32 width, height;
	// Column drawing function pointer.
	static void (*ColumnDrawerPointer)(const column_t *patchcol, UINT8 *block, GLMipmap_t *mipmap,
								INT32 pblockheight, INT32 blockmodulo,
								fixed_t yfracstep, fixed_t scale_y,
								texpatch_t *originPatch, INT32 patchheight,
								INT32 bpp);

	if (texture->width <= 0 || texture->height <= 0)
		return;

	ColumnDrawerPointer = (patch->flip & 2) ? HWR_DrawFlippedColumnInCache : HWR_DrawColumnInCache;

	x1 = patch->originx;
	width = SHORT(realpatch->width);
	height = SHORT(realpatch->height);
	x2 = x1 + width;

	if (x1 > texture->width || x2 < 0)
		return; // patch not located within texture's x bounds, ignore

	if (patch->originy > texture->height || (patch->originy + height) < 0)
		return; // patch not located within texture's y bounds, ignore

	// patch is actually inside the texture!
	// now check if texture is partly off-screen and adjust accordingly

	// left edge
	if (x1 < 0)
		x = 0;
	else
		x = x1;

	// right edge
	if (x2 > texture->width)
		x2 = texture->width;


	col = x * pblockwidth / texture->width;
	ncols = ((x2 - x) * pblockwidth) / texture->width;

/*
	CONS_Debug(DBG_RENDER, "patch %dx%d texture %dx%d block %dx%d\n",
															width, height,
															texture->width,          texture->height,
															pblockwidth,             pblockheight);
	CONS_Debug(DBG_RENDER, "      col %d ncols %d x %d\n", col, ncols, x);
*/

	// source advance
	xfrac = 0;
	if (x1 < 0)
		xfrac = -x1<<FRACBITS;

	xfracstep = (texture->width << FRACBITS) / pblockwidth;
	yfracstep = (texture->height<< FRACBITS) / pblockheight;
	scale_y   = (pblockheight  << FRACBITS) / texture->height;

	bpp = format2bpp(mipmap->format);

	if (bpp < 1 || bpp > 4)
		I_Error("HWR_DrawPatchInCache: no drawer defined for this bpp (%d)\n",bpp);

	// NOTE: should this actually be pblockwidth*bpp?
	blockmodulo = pblockwidth*bpp;

	// Draw each column to the block cache
	for (block += col*bpp; ncols--; block += bpp, xfrac += xfracstep)
	{
		if (patch->flip & 1)
			patchcol = (const column_t *)((const UINT8 *)realpatch + LONG(realpatch->columnofs[(width-1)-(xfrac>>FRACBITS)]));
		else
			patchcol = (const column_t *)((const UINT8 *)realpatch + LONG(realpatch->columnofs[xfrac>>FRACBITS]));

		ColumnDrawerPointer(patchcol, block, mipmap,
								pblockheight, blockmodulo,
								yfracstep, scale_y,
								patch, height,
								bpp);
	}
}

static UINT8 *MakeBlock(GLMipmap_t *grMipmap)
{
	UINT8 *block;
	INT32 bpp, i;
	UINT16 bu16 = ((0x00 <<8) | HWR_PATCHES_CHROMAKEY_COLORINDEX);
	INT32 blocksize = (grMipmap->width * grMipmap->height);

	bpp =  format2bpp(grMipmap->format);
	block = Z_Malloc(blocksize*bpp, PU_HWRCACHE, &(grMipmap->data));

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
static void HWR_GenerateTexture(INT32 texnum, GLMapTexture_t *grtex)
{
	UINT8 *block;
	texture_t *texture;
	texpatch_t *patch;
	patch_t *realpatch;
	UINT8 *pdata;
	INT32 blockwidth, blockheight, blocksize;

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

	grtex->mipmap.width = (UINT16)texture->width;
	grtex->mipmap.height = (UINT16)texture->height;
	grtex->mipmap.format = textureformat;

	blockwidth = texture->width;
	blockheight = texture->height;
	blocksize = (blockwidth * blockheight);
	block = MakeBlock(&grtex->mipmap);

	if (skyspecial) //Hurdler: not efficient, but better than holes in the sky (and it's done only at level loading)
	{
		INT32 j;
		RGBA_t col;

		col = V_GetColor(HWR_PATCHES_CHROMAKEY_COLORINDEX);
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
		boolean dealloc = true;
		size_t lumplength = W_LumpLengthPwad(patch->wad, patch->lump);
		pdata = W_CacheLumpNumPwad(patch->wad, patch->lump, PU_CACHE);
		realpatch = (patch_t *)pdata;

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG((UINT8 *)realpatch, lumplength))
			realpatch = (patch_t *)Picture_PNGConvert(pdata, PICFMT_PATCH, NULL, NULL, NULL, NULL, lumplength, NULL, 0);
		else
#endif
#ifdef WALLFLATS
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = (patch_t *)Picture_Convert(PICFMT_FLAT, pdata, PICFMT_PATCH, 0, NULL, texture->width, texture->height, 0, 0, 0);
		else
#endif
		{
			(void)lumplength;
			dealloc = false;
		}

		HWR_DrawTexturePatchInCache(&grtex->mipmap, blockwidth, blockheight, texture, patch, realpatch);

		if (dealloc)
			Z_Unlock(realpatch);
	}
	//Hurdler: not efficient at all but I don't remember exactly how HWR_DrawPatchInCache works :(
	if (format2bpp(grtex->mipmap.format)==4)
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
#ifndef NO_PNG_LUMPS
	// lump is a png so convert it
	size_t len = W_LumpLengthPwad(grPatch->wadnum, grPatch->lumpnum);
	if ((patch != NULL) && Picture_IsLumpPNG((const UINT8 *)patch, len))
		patch = (patch_t *)Picture_PNGConvert((const UINT8 *)patch, PICFMT_PATCH, NULL, NULL, NULL, NULL, len, NULL, 0);
#endif

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

		grMipmap->width = grMipmap->height = 1;
		while (grMipmap->width < grPatch->width) grMipmap->width <<= 1;
		while (grMipmap->height < grPatch->height) grMipmap->height <<= 1;

		// no wrap around, no chroma key
		grMipmap->flags = 0;
		// setup the texture info
		grMipmap->format = patchformat;

		//grPatch->max_s = grPatch->max_t = 1.0f;
		grPatch->max_s = (float)grPatch->width / (float)grMipmap->width;
		grPatch->max_t = (float)grPatch->height / (float)grMipmap->height;
	}

	Z_Free(grMipmap->data);
	grMipmap->data = NULL;

	if (makebitmap)
	{
		MakeBlock(grMipmap);

		HWR_DrawPatchInCache(grMipmap,
			grMipmap->width, grMipmap->height,
			grPatch->width, grPatch->height,
			patch);
	}
}


// =================================================
//             CACHING HANDLING
// =================================================

static size_t gl_numtextures = 0; // Texture count
static GLMapTexture_t *gl_textures; // For all textures
static GLMapTexture_t *gl_flats; // For all (texture) flats, as normal flats don't need to be cached

void HWR_InitTextureCache(void)
{
	gl_textures = NULL;
	gl_flats = NULL;
}

// Callback function for HWR_FreeTextureCache.
static void FreeMipmapColormap(INT32 patchnum, void *patch)
{
	GLPatch_t* const pat = patch;
	(void)patchnum; //unused

	// The patch must be valid, obviously
	if (!pat)
		return;

	// The mipmap must be valid, obviously
	while (pat->mipmap)
	{
		// Confusing at first, but pat->mipmap->nextcolormap
		// at the beginning of the loop is the first colormap
		// from the linked list of colormaps.
		GLMipmap_t *next = NULL;

		// No mipmap in this patch, break out of the loop.
		if (!pat->mipmap)
			break;

		// No colormap mipmap either.
		if (!pat->mipmap->nextcolormap)
			break;

		// Set the first colormap to the one that comes after it.
		next = pat->mipmap->nextcolormap;
		pat->mipmap->nextcolormap = next->nextcolormap;

		// Free image data from memory.
		if (next->data)
			Z_Free(next->data);
		next->data = NULL;

		// Free the old colormap mipmap from memory.
		free(next);
	}
}

void HWR_FreeMipmapCache(void)
{
	INT32 i;

	// free references to the textures
	HWD.pfnClearMipMapCache();

	// free all hardware-converted graphics cached in the heap
	// our gool is only the textures since user of the texture is the texture cache
	Z_FreeTag(PU_HWRCACHE);
	Z_FreeTag(PU_HWRCACHE_UNLOCKED);

	// Alam: free the Z_Blocks before freeing it's users
	// free all patch colormaps after each level: must be done after ClearMipMapCache!
	for (i = 0; i < numwadfiles; i++)
		M_AATreeIterate(wadfiles[i]->hwrcache, FreeMipmapColormap);
}

void HWR_FreeTextureCache(void)
{
	// free references to the textures
	HWR_FreeMipmapCache();

	// now the heap don't have any 'user' pointing to our
	// texturecache info, we can free it
	if (gl_textures)
		free(gl_textures);
	if (gl_flats)
		free(gl_flats);
	gl_textures = NULL;
	gl_flats = NULL;
	gl_numtextures = 0;
}

void HWR_LoadTextures(size_t pnumtextures)
{
	// we must free it since numtextures changed
	HWR_FreeTextureCache();

	// Why not Z_Malloc?
	gl_numtextures = pnumtextures;
	gl_textures = calloc(gl_numtextures, sizeof(*gl_textures));
	gl_flats = calloc(gl_numtextures, sizeof(*gl_flats));

	// Doesn't tell you which it _is_, but hopefully
	// should never ever happen (right?!)
	if ((gl_textures == NULL) || (gl_flats == NULL))
		I_Error("HWR_LoadTextures: ran out of memory for OpenGL textures. Sad!");
}

void HWR_SetPalette(RGBA_t *palette)
{
	HWD.pfnSetPalette(palette);

	// hardware driver will flush there own cache if cache is non paletized
	// now flush data texture cache so 32 bit texture are recomputed
	if (patchformat == GL_TEXFMT_RGBA || textureformat == GL_TEXFMT_RGBA)
	{
		Z_FreeTag(PU_HWRCACHE);
		Z_FreeTag(PU_HWRCACHE_UNLOCKED);
	}
}

// --------------------------------------------------------------------------
// Make sure texture is downloaded and set it as the source
// --------------------------------------------------------------------------
GLMapTexture_t *HWR_GetTexture(INT32 tex)
{
	GLMapTexture_t *grtex;
#ifdef PARANOIA
	if ((unsigned)tex >= gl_numtextures)
		I_Error("HWR_GetTexture: tex >= numtextures\n");
#endif

	// Every texture in memory, stored in the
	// hardware renderer's bit depth format. Wow!
	grtex = &gl_textures[tex];

	// Generate texture if missing from the cache
	if (!grtex->mipmap.data && !grtex->mipmap.downloaded)
		HWR_GenerateTexture(tex, grtex);

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!grtex->mipmap.downloaded)
		HWD.pfnSetTexture(&grtex->mipmap);

	HWR_SetCurrentTexture(&grtex->mipmap);

	// The system-memory data can be purged now.
	Z_ChangeTag(grtex->mipmap.data, PU_HWRCACHE_UNLOCKED);

	return grtex;
}

static void HWR_CacheFlat(GLMipmap_t *grMipmap, lumpnum_t flatlumpnum)
{
	size_t size, pflatsize;

	// setup the texture info
	grMipmap->format = GL_TEXFMT_P_8;
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
		PU_HWRCACHE, &grMipmap->data));
}

static void HWR_CacheTextureAsFlat(GLMipmap_t *grMipmap, INT32 texturenum)
{
	UINT8 *flat;
	UINT8 *converted;
	size_t size;

	// setup the texture info
	grMipmap->format = GL_TEXFMT_P_8;
	grMipmap->flags = TF_WRAPXY|TF_CHROMAKEYED;

	grMipmap->width  = (UINT16)textures[texturenum]->width;
	grMipmap->height = (UINT16)textures[texturenum]->height;
	size = (grMipmap->width * grMipmap->height);

	flat = Z_Malloc(size, PU_HWRCACHE, &grMipmap->data);
	converted = (UINT8 *)Picture_TextureToFlat(texturenum);
	M_Memcpy(flat, converted, size);
	Z_Free(converted);
}

// Download a Doom 'flat' to the hardware cache and make it ready for use
void HWR_LiterallyGetFlat(lumpnum_t flatlumpnum)
{
	GLMipmap_t *grmip;
	if (flatlumpnum == LUMPERROR)
		return;

	grmip = HWR_GetCachedGLPatch(flatlumpnum)->mipmap;
	if (!grmip->downloaded && !grmip->data)
		HWR_CacheFlat(grmip, flatlumpnum);

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!grmip->downloaded)
		HWD.pfnSetTexture(grmip);

	HWR_SetCurrentTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}

void HWR_GetLevelFlat(levelflat_t *levelflat)
{
	// Who knows?
	if (levelflat == NULL)
		return;

	if (levelflat->type == LEVELFLAT_FLAT)
		HWR_LiterallyGetFlat(levelflat->u.flat.lumpnum);
	else if (levelflat->type == LEVELFLAT_TEXTURE)
	{
		GLMapTexture_t *grtex;
		INT32 texturenum = levelflat->u.texture.num;
#ifdef PARANOIA
		if ((unsigned)texturenum >= gl_numtextures)
			I_Error("HWR_GetLevelFlat: texturenum >= numtextures");
#endif

		// Who knows?
		if (texturenum == 0 || texturenum == -1)
			return;

		// Every texture in memory, stored as a 8-bit flat. Wow!
		grtex = &gl_flats[texturenum];

		// Generate flat if missing from the cache
		if (!grtex->mipmap.data && !grtex->mipmap.downloaded)
			HWR_CacheTextureAsFlat(&grtex->mipmap, texturenum);

		// If hardware does not have the texture, then call pfnSetTexture to upload it
		if (!grtex->mipmap.downloaded)
			HWD.pfnSetTexture(&grtex->mipmap);

		HWR_SetCurrentTexture(&grtex->mipmap);

		// The system-memory data can be purged now.
		Z_ChangeTag(grtex->mipmap.data, PU_HWRCACHE_UNLOCKED);
	}
	else if (levelflat->type == LEVELFLAT_PATCH)
	{
		GLPatch_t *patch = W_CachePatchNum(levelflat->u.flat.lumpnum, PU_CACHE);
		levelflat->width = (UINT16)SHORT(patch->width);
		levelflat->height = (UINT16)SHORT(patch->height);
		HWR_GetPatch(patch);
	}
#ifndef NO_PNG_LUMPS
	else if (levelflat->type == LEVELFLAT_PNG)
	{
		INT32 pngwidth = 0, pngheight = 0;
		GLMipmap_t *mipmap = levelflat->mipmap;
		UINT8 *flat;
		size_t size;

		// Cache the picture.
		if (!levelflat->picture)
		{
			levelflat->picture = Picture_PNGConvert(W_CacheLumpNum(levelflat->u.flat.lumpnum, PU_CACHE), PICFMT_FLAT, &pngwidth, &pngheight, NULL, NULL, W_LumpLength(levelflat->u.flat.lumpnum), NULL, 0);
			levelflat->width = (UINT16)pngwidth;
			levelflat->height = (UINT16)pngheight;
		}

		// Make the mipmap.
		if (mipmap == NULL)
		{
			mipmap = Z_Calloc(sizeof(GLMipmap_t), PU_LEVEL, NULL);
			mipmap->format = GL_TEXFMT_P_8;
			mipmap->flags = TF_WRAPXY|TF_CHROMAKEYED;
			levelflat->mipmap = mipmap;
		}

		if (!mipmap->data && !mipmap->downloaded)
		{
			mipmap->width = levelflat->width;
			mipmap->height = levelflat->height;
			size = (mipmap->width * mipmap->height);
			flat = Z_Malloc(size, PU_LEVEL, &mipmap->data);
			if (levelflat->picture == NULL)
				I_Error("HWR_GetLevelFlat: levelflat->picture == NULL");
			M_Memcpy(flat, levelflat->picture, size);
		}

		// Tell the hardware driver to bind the current texture to the flat's mipmap
		HWD.pfnSetTexture(mipmap);
	}
#endif
	else // set no texture
		HWR_SetCurrentTexture(NULL);
}

//
// HWR_LoadMappedPatch(): replace the skin color of the sprite in cache
//                          : load it first in doom cache if not already
//
static void HWR_LoadMappedPatch(GLMipmap_t *grmip, GLPatch_t *gpatch)
{
	if (!grmip->downloaded && !grmip->data)
	{
		patch_t *patch = gpatch->rawpatch;
		if (!patch)
			patch = W_CacheLumpNumPwad(gpatch->wadnum, gpatch->lumpnum, PU_STATIC);
		HWR_MakePatch(patch, gpatch, grmip, true);

		// You can't free rawpatch for some reason?
		// (Obviously I can't, sprite rotation needs that...)
		if (!gpatch->rawpatch)
			Z_Free(patch);
	}

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!grmip->downloaded)
		HWD.pfnSetTexture(grmip);

	HWR_SetCurrentTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}

// -----------------+
// HWR_GetPatch     : Download a patch to the hardware cache and make it ready for use
// -----------------+
void HWR_GetPatch(GLPatch_t *gpatch)
{
	// is it in hardware cache
	if (!gpatch->mipmap->downloaded && !gpatch->mipmap->data)
	{
		// load the software patch, PU_STATIC or the Z_Malloc for hardware patch will
		// flush the software patch before the conversion! oh yeah I suffered
		patch_t *ptr = gpatch->rawpatch;
		if (!ptr)
			ptr = W_CacheLumpNumPwad(gpatch->wadnum, gpatch->lumpnum, PU_STATIC);
		HWR_MakePatch(ptr, gpatch, gpatch->mipmap, true);

		// this is inefficient.. but the hardware patch in heap is purgeable so it should
		// not fragment memory, and besides the REAL cache here is the hardware memory
		if (!gpatch->rawpatch)
			Z_Free(ptr);
	}

	// If hardware does not have the texture, then call pfnSetTexture to upload it
	if (!gpatch->mipmap->downloaded)
		HWD.pfnSetTexture(gpatch->mipmap);

	HWR_SetCurrentTexture(gpatch->mipmap);

	// The system-memory patch data can be purged now.
	Z_ChangeTag(gpatch->mipmap->data, PU_HWRCACHE_UNLOCKED);
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
	for (grmip = gpatch->mipmap; grmip->nextcolormap; )
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

	Z_ChangeTag(gpatch->mipmap->data, PU_HWRCACHE_UNLOCKED);
	Z_ChangeTag(gpatch, PU_HWRPATCHINFO_UNLOCKED);
}

static const INT32 picmode2GR[] =
{
	GL_TEXFMT_P_8,                // PALETTE
	0,                            // INTENSITY          (unsupported yet)
	GL_TEXFMT_ALPHA_INTENSITY_88, // INTENSITY_ALPHA    (corona use this)
	0,                            // RGB24              (unsupported yet)
	GL_TEXFMT_RGBA,               // RGBA32             (opengl only)
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
	picbpp = format2bpp(picmode2GR[pic->mode]);
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
	GLPatch_t *grpatch = HWR_GetCachedGLPatch(lumpnum);
	if (!grpatch->mipmap->downloaded && !grpatch->mipmap->data)
	{
		pic_t *pic;
		UINT8 *block;
		size_t len;

		pic = W_CacheLumpNum(lumpnum, PU_CACHE);
		grpatch->width = SHORT(pic->width);
		grpatch->height = SHORT(pic->height);
		len = W_LumpLength(lumpnum) - sizeof (pic_t);

		grpatch->leftoffset = 0;
		grpatch->topoffset = 0;

		grpatch->mipmap->width = (UINT16)grpatch->width;
		grpatch->mipmap->height = (UINT16)grpatch->height;

		if (pic->mode == PALETTE)
			grpatch->mipmap->format = textureformat; // can be set by driver
		else
			grpatch->mipmap->format = picmode2GR[pic->mode];

		Z_Free(grpatch->mipmap->data);

		// allocate block
		block = MakeBlock(grpatch->mipmap);

		if (grpatch->width  == SHORT(pic->width) &&
			grpatch->height == SHORT(pic->height) &&
			format2bpp(grpatch->mipmap->format) == format2bpp(picmode2GR[pic->mode]))
		{
			// no conversion needed
			M_Memcpy(grpatch->mipmap->data, pic->data,len);
		}
		else
			HWR_DrawPicInCache(block, SHORT(pic->width), SHORT(pic->height),
			                   SHORT(pic->width)*format2bpp(grpatch->mipmap->format),
			                   pic,
			                   format2bpp(grpatch->mipmap->format));

		Z_Unlock(pic);
		Z_ChangeTag(block, PU_HWRCACHE_UNLOCKED);

		grpatch->mipmap->flags = 0;
		grpatch->max_s = grpatch->max_t = 1.0f;
	}
	HWD.pfnSetTexture(grpatch->mipmap);
	//CONS_Debug(DBG_RENDER, "picloaded at %x as texture %d\n",grpatch->mipmap.data, grpatch->mipmap.downloaded);

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
		grpatch->mipmap = Z_Calloc(sizeof(GLMipmap_t), PU_HWRPATCHINFO, NULL);
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
	UINT8 *block = mipmap->data; // places the data directly into here
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
		dest = &block[j*(mipmap->width)]; // 1bpp
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
	grMipmap->format = GL_TEXFMT_ALPHA_8; // put the correct alpha levels straight in so I don't need to convert it later
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
	grMipmap->width  = fmwidth;
	grMipmap->height = fmheight;

	MakeBlock(grMipmap);

	HWR_DrawFadeMaskInCache(grMipmap, fmwidth, fmheight, fademasklumpnum, fmwidth, fmheight);

	// I DO need to convert this because it isn't power of 2 and we need the alpha
}


void HWR_GetFadeMask(lumpnum_t fademasklumpnum)
{
	GLMipmap_t *grmip = HWR_GetCachedGLPatch(fademasklumpnum)->mipmap;
	if (!grmip->downloaded && !grmip->data)
		HWR_CacheFadeMask(grmip, fademasklumpnum);

	HWD.pfnSetTexture(grmip);

	// The system-memory data can be purged now.
	Z_ChangeTag(grmip->data, PU_HWRCACHE_UNLOCKED);
}

#endif //HWRENDER
