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
/// \file  r_data.c
/// \brief Preparation of data for rendering, generation of lookups, caching, retrieval by name

#include "doomdef.h"
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "m_misc.h"
#include "r_data.h"
#include "r_patch.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h" // levelflats
#include "v_video.h" // pMasterPalette
#include "f_finale.h" // wipes
#include "byteptr.h"
#include "dehacked.h"

#ifdef _WIN32
#include <malloc.h> // alloca(sizeof)
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h" // HWR_LoadTextures
#endif

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// Not sure if this is necessary, but it was in w_wad.c, so I'm putting it here too -Shadow Hog
#if 0
#define AVOID_ERRNO
#else
#include <errno.h>
#endif

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
	INT16 originx, originy;
	INT16 patch, stepdir, colormap;
} ATTRPACK mappatch_t;

//
// Texture definition.
// An SRB2 wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
	char name[8];
	INT32 masked;
	INT16 width;
	INT16 height;
	INT32 columndirectory; // FIXTHIS: OBSOLETE
	INT16 patchcount;
	mappatch_t patches[1];
} ATTRPACK maptexture_t;

#if defined(_MSC_VER)
#pragma pack()
#endif


// Store lists of lumps for F_START/F_END etc.
typedef struct
{
	UINT16 wadfile;
	UINT16 firstlump;
	size_t numlumps;
} lumplist_t;

//
// Graphics.
// SRB2 graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

size_t numspritelumps, max_spritelumps;

// textures
INT32 numtextures = 0; // total number of textures found,
// size of following tables

texture_t **textures = NULL;
textureflat_t *texflats = NULL;
static UINT32 **texturecolumnofs; // column offset lookup table for each texture
static UINT8 **texturecache; // graphics data for each generated full-size texture

INT32 *texturewidth;
fixed_t *textureheight; // needed for texture pegging

INT32 *texturetranslation;

// needed for pre rendering
sprcache_t *spritecachedinfo;

lighttable_t *colormaps;
lighttable_t *fadecolormap;

// for debugging/info purposes
size_t flatmemory, spritememory, texturememory;

// highcolor stuff
INT16 color8to16[256]; // remap color index to highcolor rgb value
INT16 *hicolormaps; // test a 32k colormap remaps high -> high

// Painfully simple texture id cacheing to make maps load faster. :3
static struct {
	char name[9];
	INT32 id;
} *tidcache = NULL;
static INT32 tidcachelen = 0;

//
// MAPTEXTURE_T CACHING
// When a texture is first needed, it counts the number of composite columns
//  required in the texture and allocates space for a column directory and
//  any new columns.
// The directory will simply point inside other patches if there is only one
//  patch in a given column, but any columns with multiple patches will have
//  new column_ts generated.
//

//
// R_DrawColumnInCache
// Clip and draw a column from a patch into a cached post.
//
static inline void R_DrawColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	(void)patchheight; // This parameter is unused

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		source = (UINT8 *)patch + 3;
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source -= position; // start further down the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		if (count > 0)
			M_Memcpy(cache + position, source, count);

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

//
// R_DrawFlippedColumnInCache
// Similar to R_DrawColumnInCache; it draws the column inverted, however.
//
static inline void R_DrawFlippedColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = patchheight-patch->length-topdelta;
		source = (UINT8 *)patch + 2 + patch->length; // patch + 3 + (patch->length-1)
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source += position; // start further UP the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		if (count > 0)
		{
			for (; dest < cache + position + count; --source)
				*dest++ = *source;
		}

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

UINT32 ASTBlendPixel(RGBA_t background, RGBA_t foreground, int style, UINT8 alpha)
{
	RGBA_t output;
	INT16 fullalpha = (alpha - (0xFF - foreground.s.alpha));
	if (style == AST_TRANSLUCENT)
	{
		if (fullalpha <= 0)
			output.rgba = background.rgba;
		else
		{
			// don't go too high
			if (fullalpha >= 0xFF)
				fullalpha = 0xFF;
			alpha = (UINT8)fullalpha;

			// if the background pixel is empty,
			// match software and don't blend anything
			if (!background.s.alpha)
				output.s.alpha = 0;
			else
			{
				UINT8 beta = (0xFF - alpha);
				output.s.red = ((background.s.red * beta) + (foreground.s.red * alpha)) / 0xFF;
				output.s.green = ((background.s.green * beta) + (foreground.s.green * alpha)) / 0xFF;
				output.s.blue = ((background.s.blue * beta) + (foreground.s.blue * alpha)) / 0xFF;
				output.s.alpha = 0xFF;
			}
		}
		return output.rgba;
	}
#define clamp(c) max(min(c, 0xFF), 0x00);
	else
	{
		float falpha = ((float)alpha / 256.0f);
		float fr = ((float)foreground.s.red * falpha);
		float fg = ((float)foreground.s.green * falpha);
		float fb = ((float)foreground.s.blue * falpha);
		if (style == AST_ADD)
		{
			output.s.red = clamp((int)(background.s.red + fr));
			output.s.green = clamp((int)(background.s.green + fg));
			output.s.blue = clamp((int)(background.s.blue + fb));
		}
		else if (style == AST_SUBTRACT)
		{
			output.s.red = clamp((int)(background.s.red - fr));
			output.s.green = clamp((int)(background.s.green - fg));
			output.s.blue = clamp((int)(background.s.blue - fb));
		}
		else if (style == AST_REVERSESUBTRACT)
		{
			output.s.red = clamp((int)((-background.s.red) + fr));
			output.s.green = clamp((int)((-background.s.green) + fg));
			output.s.blue = clamp((int)((-background.s.blue) + fb));
		}
		else if (style == AST_MODULATE)
		{
			fr = ((float)foreground.s.red / 256.0f);
			fg = ((float)foreground.s.green / 256.0f);
			fb = ((float)foreground.s.blue / 256.0f);
			output.s.red = clamp((int)(background.s.red * fr));
			output.s.green = clamp((int)(background.s.green * fg));
			output.s.blue = clamp((int)(background.s.blue * fb));
		}
		// just copy the pixel
		else if (style == AST_COPY)
			output.rgba = foreground.rgba;

		output.s.alpha = 0xFF;
		return output.rgba;
	}
#undef clamp
	return 0;
}

UINT8 ASTBlendPixel_8bpp(UINT8 background, UINT8 foreground, int style, UINT8 alpha)
{
	// Alpha style set to translucent?
	if (style == AST_TRANSLUCENT)
	{
		// Is the alpha small enough for translucency?
		if (alpha <= (10*255/11))
		{
			UINT8 *mytransmap;
			// Is the patch way too translucent? Don't blend then.
			if (alpha < 255/11)
				return background;
			// The equation's not exact but it works as intended. I'll call it a day for now.
			mytransmap = transtables + ((8*(alpha) + 255/8)/(255 - 255/11) << FF_TRANSSHIFT);
			if (background != 0xFF)
				return *(mytransmap + (background<<8) + foreground);
		}
		else // just copy the pixel
			return foreground;
	}
	// just copy the pixel
	else if (style == AST_COPY)
		return foreground;
	// use ASTBlendPixel for all other blend modes
	// and find the nearest colour in the palette
	else if (style != AST_TRANSLUCENT)
	{
		RGBA_t texel;
		RGBA_t bg = V_GetMasterColor(background);
		RGBA_t fg = V_GetMasterColor(foreground);
		texel.rgba = ASTBlendPixel(bg, fg, style, alpha);
		return NearestColor(texel.s.red, texel.s.green, texel.s.blue);
	}
	// fallback if all above fails, somehow
	// return the background pixel
	return background;
}

//
// R_DrawBlendColumnInCache
// Draws a translucent column into the cache, applying a half-cooked equation to get a proper translucency value (Needs code in R_GenerateTexture()).
//
static inline void R_DrawBlendColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	(void)patchheight; // This parameter is unused

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		source = (UINT8 *)patch + 3;
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source -= position; // start further down the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		if (count > 0)
		{
			for (; dest < cache + position + count; source++, dest++)
				if (*source != 0xFF)
					*dest = ASTBlendPixel_8bpp(*dest, *source, originPatch->style, originPatch->alpha);
		}

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

//
// R_DrawBlendFlippedColumnInCache
// Similar to the one above except that the column is inverted.
//
static inline void R_DrawBlendFlippedColumnInCache(column_t *patch, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 topdelta, prevdelta = -1;
	INT32 originy = originPatch->originy;

	while (patch->topdelta != 0xff)
	{
		topdelta = patch->topdelta;
		if (topdelta <= prevdelta)
			topdelta += prevdelta;
		prevdelta = topdelta;
		topdelta = patchheight-patch->length-topdelta;
		source = (UINT8 *)patch + 2 + patch->length; // patch + 3 + (patch->length-1)
		count = patch->length;
		position = originy + topdelta;

		if (position < 0)
		{
			count += position;
			source += position; // start further UP the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		if (count > 0)
		{
			for (; dest < cache + position + count; --source, dest++)
				if (*source != 0xFF)
					*dest = ASTBlendPixel_8bpp(*dest, *source, originPatch->style, originPatch->alpha);
		}

		patch = (column_t *)((UINT8 *)patch + patch->length + 4);
	}
}

//
// R_GenerateTexture
//
// Allocate space for full size texture, either single patch or 'composite'
// Build the full textures from patches.
// The texture caching system is a little more hungry of memory, but has
// been simplified for the sake of highcolor (lol), dynamic ligthing, & speed.
//
// This is not optimised, but it's supposed to be executed only once
// per level, when enough memory is available.
//
static UINT8 *R_GenerateTexture(size_t texnum)
{
	UINT8 *block;
	UINT8 *blocktex;
	texture_t *texture;
	texpatch_t *patch;
	patch_t *realpatch;
	UINT8 *pdata;
	int x, x1, x2, i, width, height;
	size_t blocksize;
	column_t *patchcol;
	UINT8 *colofs;

	UINT16 wadnum;
	lumpnum_t lumpnum;
	size_t lumplength;

	I_Assert(texnum <= (size_t)numtextures);
	texture = textures[texnum];
	I_Assert(texture != NULL);

	// allocate texture column offset lookup

	// single-patch textures can have holes in them and may be used on
	// 2sided lines so they need to be kept in 'packed' format
	// BUT this is wrong for skies and walls with over 255 pixels,
	// so check if there's holes and if not strip the posts.
	if (texture->patchcount == 1)
	{
		boolean holey = false;
		patch = texture->patches;

		wadnum = patch->wad;
		lumpnum = patch->lump;
		lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		realpatch = (patch_t *)pdata;

#ifndef NO_PNG_LUMPS
		if (R_IsLumpPNG((UINT8 *)realpatch, lumplength))
			goto multipatch;
#endif
#ifdef WALLFLATS
		if (texture->type == TEXTURETYPE_FLAT)
			goto multipatch;
#endif

		// Check the patch for holes.
		if (texture->width > SHORT(realpatch->width) || texture->height > SHORT(realpatch->height))
			holey = true;
		colofs = (UINT8 *)realpatch->columnofs;
		for (x = 0; x < texture->width && !holey; x++)
		{
			column_t *col = (column_t *)((UINT8 *)realpatch + LONG(*(UINT32 *)&colofs[x<<2]));
			INT32 topdelta, prevdelta = -1, y = 0;
			while (col->topdelta != 0xff)
			{
				topdelta = col->topdelta;
				if (topdelta <= prevdelta)
					topdelta += prevdelta;
				prevdelta = topdelta;
				if (topdelta > y)
					break;
				y = topdelta + col->length + 1;
				col = (column_t *)((UINT8 *)col + col->length + 4);
			}
			if (y < texture->height)
				holey = true; // this texture is HOLEy! D:
		}

		// If the patch uses transparency, we have to save it this way.
		if (holey)
		{
			texture->holes = true;
			texture->flip = patch->flip;
			blocksize = lumplength;
			block = Z_Calloc(blocksize, PU_STATIC, // will change tag at end of this function
				&texturecache[texnum]);
			M_Memcpy(block, realpatch, blocksize);
			texturememory += blocksize;

			// use the patch's column lookup
			colofs = (block + 8);
			texturecolumnofs[texnum] = (UINT32 *)colofs;
			blocktex = block;
			if (patch->flip & 1) // flip the patch horizontally
			{
				UINT8 *realcolofs = (UINT8 *)realpatch->columnofs;
				for (x = 0; x < texture->width; x++)
					*(UINT32 *)&colofs[x<<2] = realcolofs[( texture->width-1-x )<<2]; // swap with the offset of the other side of the texture
			}
			// we can't as easily flip the patch vertically sadly though,
			//  we have wait until the texture itself is drawn to do that
			for (x = 0; x < texture->width; x++)
				*(UINT32 *)&colofs[x<<2] = LONG(LONG(*(UINT32 *)&colofs[x<<2]) + 3);
			goto done;
		}

		// Otherwise, do multipatch format.
	}

	// multi-patch textures (or 'composite')
	multipatch:
	texture->holes = false;
	texture->flip = 0;
	blocksize = (texture->width * 4) + (texture->width * texture->height);
	texturememory += blocksize;
	block = Z_Malloc(blocksize+1, PU_STATIC, &texturecache[texnum]);

	memset(block, TRANSPARENTPIXEL, blocksize+1); // Transparency hack

	// columns lookup table
	colofs = block;
	texturecolumnofs[texnum] = (UINT32 *)colofs;

	// texture data after the lookup table
	blocktex = block + (texture->width*4);

	// Composite the columns together.
	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		boolean dealloc = true;
		static void (*ColumnDrawerPointer)(column_t *, UINT8 *, texpatch_t *, INT32, INT32); // Column drawing function pointer.
		if (patch->style != AST_COPY)
			ColumnDrawerPointer = (patch->flip & 2) ? R_DrawBlendFlippedColumnInCache : R_DrawBlendColumnInCache;
		else
			ColumnDrawerPointer = (patch->flip & 2) ? R_DrawFlippedColumnInCache : R_DrawColumnInCache;

		wadnum = patch->wad;
		lumpnum = patch->lump;
		pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		realpatch = (patch_t *)pdata;
		dealloc = true;

#ifndef NO_PNG_LUMPS
		if (R_IsLumpPNG((UINT8 *)realpatch, lumplength))
			realpatch = R_PNGToPatch((UINT8 *)realpatch, lumplength, NULL);
		else
#endif
#ifdef WALLFLATS
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = R_FlatToPatch(pdata, texture->width, texture->height, 0, 0, NULL, false);
		else
#endif
		{
			(void)lumplength;
			dealloc = false;
		}

		x1 = patch->originx;
		width = SHORT(realpatch->width);
		height = SHORT(realpatch->height);
		x2 = x1 + width;

		if (x1 > texture->width || x2 < 0)
			continue; // patch not located within texture's x bounds, ignore

		if (patch->originy > texture->height || (patch->originy + height) < 0)
			continue; // patch not located within texture's y bounds, ignore

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

		for (; x < x2; x++)
		{
			if (patch->flip & 1)
				patchcol = (column_t *)((UINT8 *)realpatch + LONG(realpatch->columnofs[(x1+width-1)-x]));
			else
				patchcol = (column_t *)((UINT8 *)realpatch + LONG(realpatch->columnofs[x-x1]));

			// generate column ofset lookup
			*(UINT32 *)&colofs[x<<2] = LONG((x * texture->height) + (texture->width*4));
			ColumnDrawerPointer(patchcol, block + LONG(*(UINT32 *)&colofs[x<<2]), patch, texture->height, height);
		}

		if (dealloc)
			Z_Free(realpatch);
	}

done:
	// Now that the texture has been built in column cache, it is purgable from zone memory.
	Z_ChangeTag(block, PU_CACHE);
	return blocktex;
}

//
// R_GetTextureNum
//
// Returns the actual texture id that we should use.
// This can either be texnum, the current frame for texnum's anim (if animated),
// or 0 if not valid.
//
INT32 R_GetTextureNum(INT32 texnum)
{
	if (texnum < 0 || texnum >= numtextures)
		return 0;
	return texturetranslation[texnum];
}

//
// R_CheckTextureCache
//
// Use this if you need to make sure the texture is cached before R_GetColumn calls
// e.g.: midtextures and FOF walls
//
void R_CheckTextureCache(INT32 tex)
{
	if (!texturecache[tex])
		R_GenerateTexture(tex);
}

//
// R_GetColumn
//
UINT8 *R_GetColumn(fixed_t tex, INT32 col)
{
	UINT8 *data;
	INT32 width = texturewidth[tex];

	if (width & (width - 1))
		col = (UINT32)col % width;
	else
		col &= (width - 1);

	data = texturecache[tex];
	if (!data)
		data = R_GenerateTexture(tex);

	return data + LONG(texturecolumnofs[tex][col]);
}

// convert flats to hicolor as they are requested
//
UINT8 *R_GetFlat(lumpnum_t flatlumpnum)
{
	return W_CacheLumpNum(flatlumpnum, PU_CACHE);
}

//
// Empty the texture cache (used for load wad at runtime)
//
void R_FlushTextureCache(void)
{
	INT32 i;

	if (numtextures)
		for (i = 0; i < numtextures; i++)
			Z_Free(texturecache[i]);
}

// Need these prototypes for later; defining them here instead of r_data.h so they're "private"
int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum);
void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *index);

#ifdef WALLFLATS
static INT32
Rloadflats (INT32 i, INT32 w)
{
	UINT16 j;
	UINT16 texstart, texend;
	texture_t *texture;
	texpatch_t *patch;

	// Yes
	if (wadfiles[w]->type == RET_PK3)
	{
		texstart = W_CheckNumForFolderStartPK3("flats/", (UINT16)w, 0);
		texend = W_CheckNumForFolderEndPK3("flats/", (UINT16)w, texstart);
	}
	else
	{
		texstart = W_CheckNumForMarkerStartPwad("F_START", (UINT16)w, 0);
		texend = W_CheckNumForNamePwad("F_END", (UINT16)w, texstart);
	}

	if (!( texstart == INT16_MAX || texend == INT16_MAX ))
	{
		// Work through each lump between the markers in the WAD.
		for (j = 0; j < (texend - texstart); j++)
		{
			UINT8 *flatlump;
			UINT16 wadnum = (UINT16)w;
			lumpnum_t lumpnum = texstart + j;
			size_t lumplength;
			size_t flatsize = 0;

			if (wadfiles[w]->type == RET_PK3)
			{
				if (W_IsLumpFolder(wadnum, lumpnum)) // Check if lump is a folder
					continue; // If it is then SKIP IT
			}

			flatlump = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
			lumplength = W_LumpLengthPwad(wadnum, lumpnum);

			switch (lumplength)
			{
				case 4194304: // 2048x2048 lump
					flatsize = 2048;
					break;
				case 1048576: // 1024x1024 lump
					flatsize = 1024;
					break;
				case 262144:// 512x512 lump
					flatsize = 512;
					break;
				case 65536: // 256x256 lump
					flatsize = 256;
					break;
				case 16384: // 128x128 lump
					flatsize = 128;
					break;
				case 1024: // 32x32 lump
					flatsize = 32;
					break;
				default: // 64x64 lump
					flatsize = 64;
					break;
			}

			//CONS_Printf("\n\"%s\" is a flat, dimensions %d x %d",W_CheckNameForNumPwad((UINT16)w,texstart+j),flatsize,flatsize);
			texture = textures[i] = Z_Calloc(sizeof(texture_t) + sizeof(texpatch_t), PU_STATIC, NULL);

			// Set texture properties.
			M_Memcpy(texture->name, W_CheckNameForNumPwad(wadnum, lumpnum), sizeof(texture->name));

#ifndef NO_PNG_LUMPS
			if (R_IsLumpPNG((UINT8 *)flatlump, lumplength))
			{
				INT16 width, height;
				R_PNGDimensions((UINT8 *)flatlump, &width, &height, lumplength);
				texture->width = width;
				texture->height = height;
			}
			else
#endif
				texture->width = texture->height = flatsize;

			texture->type = TEXTURETYPE_FLAT;
			texture->patchcount = 1;
			texture->holes = false;
			texture->flip = 0;

			// Allocate information for the texture's patches.
			patch = &texture->patches[0];

			patch->originx = patch->originy = 0;
			patch->wad = (UINT16)w;
			patch->lump = texstart + j;
			patch->flip = 0;

			Z_Unlock(flatlump);

			texturewidth[i] = texture->width;
			textureheight[i] = texture->height << FRACBITS;
			i++;
		}
	}

	return i;
}
#endif/*WALLFLATS*/

#define TX_START "TX_START"
#define TX_END "TX_END"

static INT32
Rloadtextures (INT32 i, INT32 w)
{
	UINT16 j;
	UINT16 texstart, texend, texturesLumpPos;
	texture_t *texture;
	patch_t *patchlump;
	texpatch_t *patch;

	// Get the lump numbers for the markers in the WAD, if they exist.
	if (wadfiles[w]->type == RET_PK3)
	{
		texstart = W_CheckNumForFolderStartPK3("textures/", (UINT16)w, 0);
		texend = W_CheckNumForFolderEndPK3("textures/", (UINT16)w, texstart);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		while (texturesLumpPos != INT16_MAX)
		{
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);
			texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, texturesLumpPos + 1);
		}
	}
	else
	{
		texstart = W_CheckNumForMarkerStartPwad(TX_START, (UINT16)w, 0);
		texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, 0);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		if (texturesLumpPos != INT16_MAX)
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);
	}

	if (!( texstart == INT16_MAX || texend == INT16_MAX ))
	{
		// Work through each lump between the markers in the WAD.
		for (j = 0; j < (texend - texstart); j++)
		{
			UINT16 wadnum = (UINT16)w;
			lumpnum_t lumpnum = texstart + j;
#ifndef NO_PNG_LUMPS
			size_t lumplength;
#endif

			if (wadfiles[w]->type == RET_PK3)
			{
				if (W_IsLumpFolder(wadnum, lumpnum)) // Check if lump is a folder
					continue; // If it is then SKIP IT
			}

			patchlump = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
#ifndef NO_PNG_LUMPS
			lumplength = W_LumpLengthPwad(wadnum, lumpnum);
#endif

			//CONS_Printf("\n\"%s\" is a single patch, dimensions %d x %d",W_CheckNameForNumPwad((UINT16)w,texstart+j),patchlump->width, patchlump->height);
			texture = textures[i] = Z_Calloc(sizeof(texture_t) + sizeof(texpatch_t), PU_STATIC, NULL);

			// Set texture properties.
			M_Memcpy(texture->name, W_CheckNameForNumPwad(wadnum, lumpnum), sizeof(texture->name));

#ifndef NO_PNG_LUMPS
			if (R_IsLumpPNG((UINT8 *)patchlump, lumplength))
			{
				INT16 width, height;
				R_PNGDimensions((UINT8 *)patchlump, &width, &height, lumplength);
				texture->width = width;
				texture->height = height;
			}
			else
#endif
			{
				texture->width = SHORT(patchlump->width);
				texture->height = SHORT(patchlump->height);
			}

			texture->type = TEXTURETYPE_SINGLEPATCH;
			texture->patchcount = 1;
			texture->holes = false;
			texture->flip = 0;

			// Allocate information for the texture's patches.
			patch = &texture->patches[0];

			patch->originx = patch->originy = 0;
			patch->wad = (UINT16)w;
			patch->lump = texstart + j;
			patch->flip = 0;

			Z_Unlock(patchlump);

			texturewidth[i] = texture->width;
			textureheight[i] = texture->height << FRACBITS;
			i++;
		}
	}

	return i;
}

//
// R_LoadTextures
// Initializes the texture list with the textures from the world map.
//
void R_LoadTextures(void)
{
	INT32 i, w;
	UINT16 j;
	UINT16 texstart, texend, texturesLumpPos;

	// Free previous memory before numtextures change.
	if (numtextures)
	{
		for (i = 0; i < numtextures; i++)
		{
			Z_Free(textures[i]);
			Z_Free(texturecache[i]);
		}
		Z_Free(texturetranslation);
		Z_Free(textures);
		Z_Free(texflats);
	}

	// Load patches and textures.

	// Get the number of textures to check.
	// NOTE: Make SURE the system does not process
	// the markers.
	// This system will allocate memory for all duplicate/patched textures even if it never uses them,
	// but the alternative is to spend a ton of time checking and re-checking all previous entries just to skip any potentially patched textures.
	for (w = 0, numtextures = 0; w < numwadfiles; w++)
	{
#ifdef WALLFLATS
		// Count flats
		if (wadfiles[w]->type == RET_PK3)
		{
			texstart = W_CheckNumForFolderStartPK3("flats/", (UINT16)w, 0);
			texend = W_CheckNumForFolderEndPK3("flats/", (UINT16)w, texstart);
		}
		else
		{
			texstart = W_CheckNumForMarkerStartPwad("F_START", (UINT16)w, 0);
			texend = W_CheckNumForNamePwad("F_END", (UINT16)w, texstart);
		}

		if (!( texstart == INT16_MAX || texend == INT16_MAX ))
		{
			// PK3s have subfolders, so we can't just make a simple sum
			if (wadfiles[w]->type == RET_PK3)
			{
				for (j = texstart; j < texend; j++)
				{
					if (!W_IsLumpFolder((UINT16)w, j)) // Check if lump is a folder; if not, then count it
						numtextures++;
				}
			}
			else // Add all the textures between F_START and F_END
			{
				numtextures += (UINT32)(texend - texstart);
			}
		}
#endif/*WALLFLATS*/

		// Count the textures from TEXTURES lumps
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		while (texturesLumpPos != INT16_MAX)
		{
			numtextures += R_CountTexturesInTEXTURESLump((UINT16)w, (UINT16)texturesLumpPos);
			texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, texturesLumpPos + 1);
		}

		// Count single-patch textures
		if (wadfiles[w]->type == RET_PK3)
		{
			texstart = W_CheckNumForFolderStartPK3("textures/", (UINT16)w, 0);
			texend = W_CheckNumForFolderEndPK3("textures/", (UINT16)w, texstart);
		}
		else
		{
			texstart = W_CheckNumForMarkerStartPwad(TX_START, (UINT16)w, 0);
			texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, 0);
		}

		if (texstart == INT16_MAX || texend == INT16_MAX)
			continue;

		// PK3s have subfolders, so we can't just make a simple sum
		if (wadfiles[w]->type == RET_PK3)
		{
			for (j = texstart; j < texend; j++)
			{
				if (!W_IsLumpFolder((UINT16)w, j)) // Check if lump is a folder; if not, then count it
					numtextures++;
			}
		}
		else // Add all the textures between TX_START and TX_END
		{
			numtextures += (UINT32)(texend - texstart);
		}
	}

	// If no textures found by this point, bomb out
	if (!numtextures)
		I_Error("No textures detected in any WADs!\n");

	// Allocate memory and initialize to 0 for all the textures we are initialising.
	// There are actually 5 buffers allocated in one for convenience.
	textures = Z_Calloc((numtextures * sizeof(void *)) * 5, PU_STATIC, NULL);
	texflats = Z_Calloc((numtextures * sizeof(*texflats)), PU_STATIC, NULL);

	// Allocate texture column offset table.
	texturecolumnofs = (void *)((UINT8 *)textures + (numtextures * sizeof(void *)));
	// Allocate texture referencing cache.
	texturecache     = (void *)((UINT8 *)textures + ((numtextures * sizeof(void *)) * 2));
	// Allocate texture width table.
	texturewidth     = (void *)((UINT8 *)textures + ((numtextures * sizeof(void *)) * 3));
	// Allocate texture height table.
	textureheight    = (void *)((UINT8 *)textures + ((numtextures * sizeof(void *)) * 4));
	// Create translation table for global animation.
	texturetranslation = Z_Malloc((numtextures + 1) * sizeof(*texturetranslation), PU_STATIC, NULL);

	for (i = 0; i < numtextures; i++)
		texturetranslation[i] = i;

	for (i = 0, w = 0; w < numwadfiles; w++)
	{
#ifdef WALLFLATS
		i = Rloadflats(i, w);
#endif
		i = Rloadtextures(i, w);
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_LoadTextures(numtextures);
#endif
}

static texpatch_t *R_ParsePatch(boolean actuallyLoadPatch)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	char *patchName = NULL;
	INT16 patchXPos;
	INT16 patchYPos;
	UINT8 flip = 0;
	UINT8 alpha = 255;
	enum patchalphastyle style = AST_COPY;
	texpatch_t *resultPatch = NULL;
	lumpnum_t patchLumpNum;

	// Patch identifier
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch name should be");
	}
	texturesTokenLength = strlen(texturesToken);
	if (texturesTokenLength>8)
	{
		I_Error("Error parsing TEXTURES lump: Patch name \"%s\" exceeds 8 characters",texturesToken);
	}
	else
	{
		if (patchName != NULL)
		{
			Z_Free(patchName);
		}
		patchName = (char *)Z_Malloc((texturesTokenLength+1)*sizeof(char),PU_STATIC,NULL);
		M_Memcpy(patchName,texturesToken,texturesTokenLength*sizeof(char));
		patchName[texturesTokenLength] = '\0';
	}

	// Comma 1
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after \"%s\"'s patch name should be",patchName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after %s's patch name, got \"%s\"",patchName,texturesToken);
	}

	// XPos
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s x coordinate should be",patchName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	patchXPos = strtol(texturesToken,&endPos,10);
	(void)patchXPos; //unused for now
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		)
	{
		I_Error("Error parsing TEXTURES lump: Expected an integer for patch \"%s\"'s x coordinate, got \"%s\"",patchName,texturesToken);
	}

	// Comma 2
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after patch \"%s\"'s x coordinate should be",patchName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after patch \"%s\"'s x coordinate, got \"%s\"",patchName,texturesToken);
	}

	// YPos
	Z_Free(texturesToken);
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s y coordinate should be",patchName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	patchYPos = strtol(texturesToken,&endPos,10);
	(void)patchYPos; //unused for now
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		)
	{
		I_Error("Error parsing TEXTURES lump: Expected an integer for patch \"%s\"'s y coordinate, got \"%s\"",patchName,texturesToken);
	}
	Z_Free(texturesToken);

	// Patch parameters block (OPTIONAL)
	// added by Monster Iestyn (22/10/16)

	// Left Curly Brace
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
		; // move on and ignore, R_ParseTextures will deal with this
	else
	{
		if (strcmp(texturesToken,"{")==0)
		{
			Z_Free(texturesToken);
			texturesToken = M_GetToken(NULL);
			if (texturesToken == NULL)
			{
				I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s parameters should be",patchName);
			}
			while (strcmp(texturesToken,"}")!=0)
			{
				if (stricmp(texturesToken, "ALPHA")==0)
				{
					Z_Free(texturesToken);
					texturesToken = M_GetToken(NULL);
					alpha = 255*strtof(texturesToken, NULL);
				}
				else if (stricmp(texturesToken, "STYLE")==0)
				{
					Z_Free(texturesToken);
					texturesToken = M_GetToken(NULL);
					if (stricmp(texturesToken, "TRANSLUCENT")==0)
						style = AST_TRANSLUCENT;
					else if (stricmp(texturesToken, "ADD")==0)
						style = AST_ADD;
					else if (stricmp(texturesToken, "SUBTRACT")==0)
						style = AST_SUBTRACT;
					else if (stricmp(texturesToken, "REVERSESUBTRACT")==0)
						style = AST_REVERSESUBTRACT;
					else if (stricmp(texturesToken, "MODULATE")==0)
						style = AST_MODULATE;
				}
				else if (stricmp(texturesToken, "FLIPX")==0)
					flip |= 1;
				else if (stricmp(texturesToken, "FLIPY")==0)
					flip |= 2;
				Z_Free(texturesToken);

				texturesToken = M_GetToken(NULL);
				if (texturesToken == NULL)
				{
					I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch \"%s\"'s parameters or right curly brace should be",patchName);
				}
			}
		}
		else
		{
			 // this is not what we wanted...
			 // undo last read so R_ParseTextures can re-get the token for its own purposes
			M_UnGetToken();
		}
		Z_Free(texturesToken);
	}

	if (actuallyLoadPatch == true)
	{
		// Check lump exists
		patchLumpNum = W_GetNumForName(patchName);
		// If so, allocate memory for texpatch_t and fill 'er up
		resultPatch = (texpatch_t *)Z_Malloc(sizeof(texpatch_t),PU_STATIC,NULL);
		resultPatch->originx = patchXPos;
		resultPatch->originy = patchYPos;
		resultPatch->lump = patchLumpNum & 65535;
		resultPatch->wad = patchLumpNum>>16;
		resultPatch->flip = flip;
		resultPatch->alpha = alpha;
		resultPatch->style = style;
		// Clean up a little after ourselves
		Z_Free(patchName);
		// Then return it
		return resultPatch;
	}
	else
	{
		Z_Free(patchName);
		return NULL;
	}
}

static texture_t *R_ParseTexture(boolean actuallyLoadTexture)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	INT32 newTextureWidth;
	INT32 newTextureHeight;
	texture_t *resultTexture = NULL;
	texpatch_t *newPatch;
	char newTextureName[9]; // no longer dynamically allocated

	// Texture name
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture name should be");
	}
	texturesTokenLength = strlen(texturesToken);
	if (texturesTokenLength>8)
	{
		I_Error("Error parsing TEXTURES lump: Texture name \"%s\" exceeds 8 characters",texturesToken);
	}
	else
	{
		memset(&newTextureName, 0, 9);
		M_Memcpy(newTextureName, texturesToken, texturesTokenLength);
		// ^^ we've confirmed that the token is <= 8 characters so it will never overflow a 9 byte char buffer
		strupr(newTextureName); // Just do this now so we don't have to worry about it
	}
	Z_Free(texturesToken);

	// Comma 1
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after texture \"%s\"'s name should be",newTextureName);
	}
	else if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after texture \"%s\"'s name, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Width
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture \"%s\"'s width should be",newTextureName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	newTextureWidth = strtol(texturesToken,&endPos,10);
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		|| newTextureWidth < 0) // Number is not positive
	{
		I_Error("Error parsing TEXTURES lump: Expected a positive integer for texture \"%s\"'s width, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Comma 2
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where comma after texture \"%s\"'s width should be",newTextureName);
	}
	if (strcmp(texturesToken,",")!=0)
	{
		I_Error("Error parsing TEXTURES lump: Expected \",\" after texture \"%s\"'s width, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Height
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where texture \"%s\"'s height should be",newTextureName);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	newTextureHeight = strtol(texturesToken,&endPos,10);
	if (endPos == texturesToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		|| newTextureHeight < 0) // Number is not positive
	{
		I_Error("Error parsing TEXTURES lump: Expected a positive integer for texture \"%s\"'s height, got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	// Left Curly Brace
	texturesToken = M_GetToken(NULL);
	if (texturesToken == NULL)
	{
		I_Error("Error parsing TEXTURES lump: Unexpected end of file where open curly brace for texture \"%s\" should be",newTextureName);
	}
	if (strcmp(texturesToken,"{")==0)
	{
		if (actuallyLoadTexture)
		{
			// Allocate memory for a zero-patch texture. Obviously, we'll be adding patches momentarily.
			resultTexture = (texture_t *)Z_Calloc(sizeof(texture_t),PU_STATIC,NULL);
			M_Memcpy(resultTexture->name, newTextureName, 8);
			resultTexture->width = newTextureWidth;
			resultTexture->height = newTextureHeight;
			resultTexture->type = TEXTURETYPE_COMPOSITE;
		}
		Z_Free(texturesToken);
		texturesToken = M_GetToken(NULL);
		if (texturesToken == NULL)
		{
			I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch definition for texture \"%s\" should be",newTextureName);
		}
		while (strcmp(texturesToken,"}")!=0)
		{
			if (stricmp(texturesToken, "PATCH")==0)
			{
				Z_Free(texturesToken);
				if (resultTexture)
				{
					// Get that new patch
					newPatch = R_ParsePatch(true);
					// Make room for the new patch
					resultTexture = Z_Realloc(resultTexture, sizeof(texture_t) + (resultTexture->patchcount+1)*sizeof(texpatch_t), PU_STATIC, NULL);
					// Populate the uninitialized values in the new patch entry of our array
					M_Memcpy(&resultTexture->patches[resultTexture->patchcount], newPatch, sizeof(texpatch_t));
					// Account for the new number of patches in the texture
					resultTexture->patchcount++;
					// Then free up the memory assigned to R_ParsePatch, as it's unneeded now
					Z_Free(newPatch);
				}
				else
				{
					R_ParsePatch(false);
				}
			}
			else
			{
				I_Error("Error parsing TEXTURES lump: Expected \"PATCH\" in texture \"%s\", got \"%s\"",newTextureName,texturesToken);
			}

			texturesToken = M_GetToken(NULL);
			if (texturesToken == NULL)
			{
				I_Error("Error parsing TEXTURES lump: Unexpected end of file where patch declaration or right curly brace for texture \"%s\" should be",newTextureName);
			}
		}
		if (resultTexture && resultTexture->patchcount == 0)
		{
			I_Error("Error parsing TEXTURES lump: Texture \"%s\" must have at least one patch",newTextureName);
		}
	}
	else
	{
		I_Error("Error parsing TEXTURES lump: Expected \"{\" for texture \"%s\", got \"%s\"",newTextureName,texturesToken);
	}
	Z_Free(texturesToken);

	if (actuallyLoadTexture) return resultTexture;
	else return NULL;
}

// Parses the TEXTURES lump... but just to count the number of textures.
int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	UINT32 numTexturesInLump = 0;
	char *texturesToken;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	texturesLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (texturesLump == NULL) return 0;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	texturesLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	texturesText = (char *)Z_Malloc((texturesLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(texturesText,texturesLump,texturesLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	texturesText[texturesLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(texturesLump);

	texturesToken = M_GetToken(texturesText);
	while (texturesToken != NULL)
	{
		if (stricmp(texturesToken, "WALLTEXTURE") == 0 || stricmp(texturesToken, "TEXTURE") == 0)
		{
			numTexturesInLump++;
			Z_Free(texturesToken);
			R_ParseTexture(false);
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\" or \"TEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);

	return numTexturesInLump;
}

// Parses the TEXTURES lump... for real, this time.
void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *texindex)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	char *texturesToken;
	texture_t *newTexture;

	I_Assert(texindex != NULL);

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	texturesLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (texturesLump == NULL) return;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	texturesLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	texturesText = (char *)Z_Malloc((texturesLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(texturesText,texturesLump,texturesLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	texturesText[texturesLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(texturesLump);

	texturesToken = M_GetToken(texturesText);
	while (texturesToken != NULL)
	{
		if (stricmp(texturesToken, "WALLTEXTURE") == 0 || stricmp(texturesToken, "TEXTURE") == 0)
		{
			Z_Free(texturesToken);
			// Get the new texture
			newTexture = R_ParseTexture(true);
			// Store the new texture
			textures[*texindex] = newTexture;
			texturewidth[*texindex] = newTexture->width;
			textureheight[*texindex] = newTexture->height << FRACBITS;
			// Increment i back in R_LoadTextures()
			(*texindex)++;
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\" or \"TEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);
}

#ifdef EXTRACOLORMAPLUMPS
static lumplist_t *colormaplumps = NULL; ///\todo free leak
static size_t numcolormaplumps = 0;

static inline lumpnum_t R_CheckNumForNameList(const char *name, lumplist_t *list, size_t listsize)
{
	size_t i;
	UINT16 lump;

	for (i = listsize - 1; i < INT16_MAX; i--)
	{
		lump = W_CheckNumForNamePwad(name, list[i].wadfile, list[i].firstlump);
		if (lump == INT16_MAX || lump > (list[i].firstlump + list[i].numlumps))
			continue;
		else
			return (list[i].wadfile<<16)+lump;
	}
	return LUMPERROR;
}

static void R_InitExtraColormaps(void)
{
	lumpnum_t startnum, endnum;
	UINT16 cfile, clump;
	static size_t maxcolormaplumps = 16;

	for (cfile = clump = 0; cfile < numwadfiles; cfile++, clump = 0)
	{
		startnum = W_CheckNumForNamePwad("C_START", cfile, clump);
		if (startnum == INT16_MAX)
			continue;

		endnum = W_CheckNumForNamePwad("C_END", cfile, clump);

		if (endnum == INT16_MAX)
			I_Error("R_InitExtraColormaps: C_START without C_END\n");

		// This shouldn't be possible when you use the Pwad function, silly
		//if (WADFILENUM(startnum) != WADFILENUM(endnum))
			//I_Error("R_InitExtraColormaps: C_START and C_END in different wad files!\n");

		if (numcolormaplumps >= maxcolormaplumps)
			maxcolormaplumps *= 2;
		colormaplumps = Z_Realloc(colormaplumps,
			sizeof (*colormaplumps) * maxcolormaplumps, PU_STATIC, NULL);
		colormaplumps[numcolormaplumps].wadfile = cfile;
		colormaplumps[numcolormaplumps].firstlump = startnum+1;
		colormaplumps[numcolormaplumps].numlumps = endnum - (startnum + 1);
		numcolormaplumps++;
	}
	CONS_Printf(M_GetText("Number of Extra Colormaps: %s\n"), sizeu1(numcolormaplumps));
}
#endif

// Search for flat name.
lumpnum_t R_GetFlatNumForName(const char *name)
{
	INT32 i;
	lumpnum_t lump;
	lumpnum_t start;
	lumpnum_t end;

	// Scan wad files backwards so patched flats take preference.
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		switch (wadfiles[i]->type)
		{
		case RET_WAD:
			if ((start = W_CheckNumForMarkerStartPwad("F_START", (UINT16)i, 0)) == INT16_MAX)
			{
				if ((start = W_CheckNumForMarkerStartPwad("FF_START", (UINT16)i, 0)) == INT16_MAX)
					continue;
				else if ((end = W_CheckNumForNamePwad("FF_END", (UINT16)i, start)) == INT16_MAX)
					continue;
			}
			else
				if ((end = W_CheckNumForNamePwad("F_END", (UINT16)i, start)) == INT16_MAX)
					continue;
			break;
		case RET_PK3:
			if ((start = W_CheckNumForFolderStartPK3("Flats/", i, 0)) == INT16_MAX)
				continue;
			if ((end = W_CheckNumForFolderEndPK3("Flats/", i, start)) == INT16_MAX)
				continue;
			break;
		default:
			continue;
		}

		// Now find lump with specified name in that range.
		lump = W_CheckNumForNamePwad(name, (UINT16)i, start);
		if (lump < end)
		{
			lump += (i<<16); // found it, in our constraints
			break;
		}
		lump = LUMPERROR;
	}

	return lump;
}

//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad, so the sprite does not need to be
// cached completely, just for having the header info ready during rendering.
//

//
// allocate sprite lookup tables
//
static void R_InitSpriteLumps(void)
{
	numspritelumps = 0;
	max_spritelumps = 8192;

	Z_Malloc(max_spritelumps*sizeof(*spritecachedinfo), PU_STATIC, &spritecachedinfo);
}

//
// R_CreateFadeColormaps
//

static void R_CreateFadeColormaps(void)
{
	UINT8 px, fade;
	RGBA_t rgba;
	INT32 r, g, b;
	size_t len, i;

	len = (256 * FADECOLORMAPROWS);
	fadecolormap = Z_MallocAlign(len*2, PU_STATIC, NULL, 8);
	for (i = 0; i < len*2; i++)
		fadecolormap[i] = (i%256);

	// Load in the light tables, now 64k aligned for smokie...
	{
		lumpnum_t lump = W_CheckNumForName("FADECMAP");
		lumpnum_t wlump = W_CheckNumForName("FADEWMAP");

		// to black
		if (lump != LUMPERROR)
			W_ReadLumpHeader(lump, fadecolormap, len, 0U);
		// to white
		if (wlump != LUMPERROR)
			W_ReadLumpHeader(wlump, fadecolormap+len, len, 0U);

		// missing "to white" colormap lump
		if (lump != LUMPERROR && wlump == LUMPERROR)
			goto makewhite;
		// missing "to black" colormap lump
		else if (lump == LUMPERROR && wlump != LUMPERROR)
			goto makeblack;
		// both lumps found
		else if (lump != LUMPERROR && wlump != LUMPERROR)
			return;
	}

#define GETCOLOR \
	px = colormaps[i%256]; \
	fade = (i/256) * (256 / FADECOLORMAPROWS); \
	rgba = V_GetMasterColor(px);

	// to black
	makeblack:
	for (i = 0; i < len; i++)
	{
		// find pixel and fade amount
		GETCOLOR;

		// subtractive color blending
		r = rgba.s.red - FADEREDFACTOR*fade/10;
		g = rgba.s.green - FADEGREENFACTOR*fade/10;
		b = rgba.s.blue - FADEBLUEFACTOR*fade/10;

		// clamp values
		if (r < 0) r = 0;
		if (g < 0) g = 0;
		if (b < 0) b = 0;

		// find nearest color in palette
		fadecolormap[i] = NearestColor(r,g,b);
	}

	// to white
	makewhite:
	for (i = len; i < len*2; i++)
	{
		// find pixel and fade amount
		GETCOLOR;

		// additive color blending
		r = rgba.s.red + FADEREDFACTOR*fade/10;
		g = rgba.s.green + FADEGREENFACTOR*fade/10;
		b = rgba.s.blue + FADEBLUEFACTOR*fade/10;

		// clamp values
		if (r > 255) r = 255;
		if (g > 255) g = 255;
		if (b > 255) b = 255;

		// find nearest color in palette
		fadecolormap[i] = NearestColor(r,g,b);
	}
#undef GETCOLOR
}

//
// R_InitColormaps
//
static void R_InitColormaps(void)
{
	size_t len;
	lumpnum_t lump;

	// Load in the light tables
	lump = W_GetNumForName("COLORMAP");
	len = W_LumpLength(lump);
	colormaps = Z_MallocAlign(len, PU_STATIC, NULL, 8);
	W_ReadLump(lump, colormaps);

	// Make colormap for fades
	R_CreateFadeColormaps();

	// Init Boom colormaps.
	R_ClearColormaps();
#ifdef EXTRACOLORMAPLUMPS
	R_InitExtraColormaps();
#endif
}

void R_ReInitColormaps(UINT16 num)
{
	char colormap[9] = "COLORMAP";
	lumpnum_t lump;
	const lumpnum_t basecolormaplump = W_GetNumForName(colormap);
	if (num > 0 && num <= 10000)
		snprintf(colormap, 8, "CLM%04u", num-1);

	// Load in the light tables, now 64k aligned for smokie...
	lump = W_GetNumForName(colormap);
	if (lump == LUMPERROR)
		lump = basecolormaplump;
	else
	{
		if (W_LumpLength(lump) != W_LumpLength(basecolormaplump))
		{
			CONS_Alert(CONS_WARNING, "%s lump size does not match COLORMAP, results may be unexpected.\n", colormap);
		}
	}

	W_ReadLumpHeader(lump, colormaps, W_LumpLength(basecolormaplump), 0U);
	if (fadecolormap)
		Z_Free(fadecolormap);
	R_CreateFadeColormaps();

	// Init Boom colormaps.
	R_ClearColormaps();
}

//
// R_ClearColormaps
//
// Clears out extra colormaps between levels.
//
void R_ClearColormaps(void)
{
	// Purged by PU_LEVEL, just overwrite the pointer
	extra_colormaps = R_CreateDefaultColormap(true);
}

//
// R_CreateDefaultColormap()
// NOTE: The result colormap is not added to the extra_colormaps chain. You must do that yourself!
//
extracolormap_t *R_CreateDefaultColormap(boolean lighttable)
{
	extracolormap_t *exc = Z_Calloc(sizeof (*exc), PU_LEVEL, NULL);
	exc->fadestart = 0;
	exc->fadeend = 31;
	exc->flags = 0;
	exc->rgba = 0;
	exc->fadergba = 0x19000000;
	exc->colormap = lighttable ? R_CreateLightTable(exc) : NULL;
#ifdef EXTRACOLORMAPLUMPS
	exc->lump = LUMPERROR;
	exc->lumpname[0] = 0;
#endif
	exc->next = exc->prev = NULL;
	return exc;
}

//
// R_GetDefaultColormap()
//
extracolormap_t *R_GetDefaultColormap(void)
{
#ifdef COLORMAPREVERSELIST
	extracolormap_t *exc;
#endif

	if (!extra_colormaps)
		return (extra_colormaps = R_CreateDefaultColormap(true));

#ifdef COLORMAPREVERSELIST
	for (exc = extra_colormaps; exc->next; exc = exc->next);
	return exc;
#else
	return extra_colormaps;
#endif
}

//
// R_CopyColormap()
// NOTE: The result colormap is not added to the extra_colormaps chain. You must do that yourself!
//
extracolormap_t *R_CopyColormap(extracolormap_t *extra_colormap, boolean lighttable)
{
	extracolormap_t *exc = Z_Calloc(sizeof (*exc), PU_LEVEL, NULL);

	if (!extra_colormap)
		extra_colormap = R_GetDefaultColormap();

	*exc = *extra_colormap;
	exc->next = exc->prev = NULL;

#ifdef EXTRACOLORMAPLUMPS
	strncpy(exc->lumpname, extra_colormap->lumpname, 9);

	if (exc->lump != LUMPERROR && lighttable)
	{
		// aligned on 8 bit for asm code
		exc->colormap = Z_MallocAlign(W_LumpLength(lump), PU_LEVEL, NULL, 16);
		W_ReadLump(lump, exc->colormap);
	}
	else
#endif
	if (lighttable)
		exc->colormap = R_CreateLightTable(exc);
	else
		exc->colormap = NULL;

	return exc;
}

//
// R_AddColormapToList
//
// Sets prev/next chain for extra_colormaps var
// Copypasta from P_AddFFloorToList
//
void R_AddColormapToList(extracolormap_t *extra_colormap)
{
#ifndef COLORMAPREVERSELIST
	extracolormap_t *exc;
#endif

	if (!extra_colormaps)
	{
		extra_colormaps = extra_colormap;
		extra_colormap->next = 0;
		extra_colormap->prev = 0;
		return;
	}

#ifdef COLORMAPREVERSELIST
	extra_colormaps->prev = extra_colormap;
	extra_colormap->next = extra_colormaps;
	extra_colormaps = extra_colormap;
	extra_colormap->prev = 0;
#else
	for (exc = extra_colormaps; exc->next; exc = exc->next);

	exc->next = extra_colormap;
	extra_colormap->prev = exc;
	extra_colormap->next = 0;
#endif
}

//
// R_CheckDefaultColormapByValues()
//
#ifdef EXTRACOLORMAPLUMPS
boolean R_CheckDefaultColormapByValues(boolean checkrgba, boolean checkfadergba, boolean checkparams,
	INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags, lumpnum_t lump)
#else
boolean R_CheckDefaultColormapByValues(boolean checkrgba, boolean checkfadergba, boolean checkparams,
	INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags)
#endif
{
	return (
		(!checkparams ? true :
			(fadestart == 0
				&& fadeend == 31
				&& !flags)
			)
		&& (!checkrgba ? true : rgba == 0)
		&& (!checkfadergba ? true : fadergba == 0x19000000)
#ifdef EXTRACOLORMAPLUMPS
		&& lump == LUMPERROR
		&& extra_colormap->lumpname[0] == 0
#endif
		);
}

boolean R_CheckDefaultColormap(extracolormap_t *extra_colormap, boolean checkrgba, boolean checkfadergba, boolean checkparams)
{
	if (!extra_colormap)
		return true;

#ifdef EXTRACOLORMAPLUMPS
	return R_CheckDefaultColormapByValues(checkrgba, checkfadergba, checkparams, extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags, extra_colormap->lump);
#else
	return R_CheckDefaultColormapByValues(checkrgba, checkfadergba, checkparams, extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags);
#endif
}

boolean R_CheckEqualColormaps(extracolormap_t *exc_a, extracolormap_t *exc_b, boolean checkrgba, boolean checkfadergba, boolean checkparams)
{
	// Treat NULL as default colormap
	// We need this because what if one exc is a default colormap, and the other is NULL? They're really both equal.
	if (!exc_a)
		exc_a = R_GetDefaultColormap();
	if (!exc_b)
		exc_b = R_GetDefaultColormap();

	if (exc_a == exc_b)
		return true;

	return (
		(!checkparams ? true :
			(exc_a->fadestart == exc_b->fadestart
				&& exc_a->fadeend == exc_b->fadeend
				&& exc_a->flags == exc_b->flags)
			)
		&& (!checkrgba ? true : exc_a->rgba == exc_b->rgba)
		&& (!checkfadergba ? true : exc_a->fadergba == exc_b->fadergba)
#ifdef EXTRACOLORMAPLUMPS
		&& exc_a->lump == exc_b->lump
		&& !strncmp(exc_a->lumpname, exc_b->lumpname, 9)
#endif
		);
}

//
// R_GetColormapFromListByValues()
// NOTE: Returns NULL if no match is found
//
#ifdef EXTRACOLORMAPLUMPS
extracolormap_t *R_GetColormapFromListByValues(INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags, lumpnum_t lump)
#else
extracolormap_t *R_GetColormapFromListByValues(INT32 rgba, INT32 fadergba, UINT8 fadestart, UINT8 fadeend, UINT8 flags)
#endif
{
	extracolormap_t *exc;
	UINT32 dbg_i = 0;

	for (exc = extra_colormaps; exc; exc = exc->next)
	{
		if (rgba == exc->rgba
			&& fadergba == exc->fadergba
			&& fadestart == exc->fadestart
			&& fadeend == exc->fadeend
			&& flags == exc->flags
#ifdef EXTRACOLORMAPLUMPS
			&& (lump != LUMPERROR && lump == exc->lump)
#endif
		)
		{
			CONS_Debug(DBG_RENDER, "Found Colormap %d: rgba(%d,%d,%d,%d) fadergba(%d,%d,%d,%d)\n",
				dbg_i, R_GetRgbaR(rgba), R_GetRgbaG(rgba), R_GetRgbaB(rgba), R_GetRgbaA(rgba),
				R_GetRgbaR(fadergba), R_GetRgbaG(fadergba), R_GetRgbaB(fadergba), R_GetRgbaA(fadergba));
			return exc;
		}
		dbg_i++;
	}
	return NULL;
}

extracolormap_t *R_GetColormapFromList(extracolormap_t *extra_colormap)
{
#ifdef EXTRACOLORMAPLUMPS
	return R_GetColormapFromListByValues(extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags, extra_colormap->lump);
#else
	return R_GetColormapFromListByValues(extra_colormap->rgba, extra_colormap->fadergba, extra_colormap->fadestart, extra_colormap->fadeend, extra_colormap->flags);
#endif
}

#ifdef EXTRACOLORMAPLUMPS
extracolormap_t *R_ColormapForName(char *name)
{
	lumpnum_t lump;
	extracolormap_t *exc;

	lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
	if (lump == LUMPERROR)
		I_Error("R_ColormapForName: Cannot find colormap lump %.8s\n", name);

	exc = R_GetColormapFromListByValues(0, 0x19000000, 0, 31, 0, lump);
	if (exc)
		return exc;

	exc = Z_Calloc(sizeof (*exc), PU_LEVEL, NULL);

	exc->lump = lump;
	strncpy(exc->lumpname, name, 9);
	exc->lumpname[8] = 0;

	// aligned on 8 bit for asm code
	exc->colormap = Z_MallocAlign(W_LumpLength(lump), PU_LEVEL, NULL, 16);
	W_ReadLump(lump, exc->colormap);

	// We set all params of the colormap to normal because there
	// is no real way to tell how GL should handle a colormap lump anyway..
	exc->fadestart = 0;
	exc->fadeend = 31;
	exc->flags = 0;
	exc->rgba = 0;
	exc->fadergba = 0x19000000;

	R_AddColormapToList(exc);

	return exc;
}
#endif

//
// R_CreateColormap
//
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.
//
static double deltas[256][3], map[256][3];

static int RoundUp(double number);

lighttable_t *R_CreateLightTable(extracolormap_t *extra_colormap)
{
	double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
	double maskamt = 0, othermask = 0;

	UINT8 cr = R_GetRgbaR(extra_colormap->rgba),
		cg = R_GetRgbaG(extra_colormap->rgba),
		cb = R_GetRgbaB(extra_colormap->rgba),
		ca = R_GetRgbaA(extra_colormap->rgba),
		cfr = R_GetRgbaR(extra_colormap->fadergba),
		cfg = R_GetRgbaG(extra_colormap->fadergba),
		cfb = R_GetRgbaB(extra_colormap->fadergba);
//		cfa = R_GetRgbaA(extra_colormap->fadergba); // unused in software

	UINT8 fadestart = extra_colormap->fadestart,
		fadedist = extra_colormap->fadeend - extra_colormap->fadestart;

	lighttable_t *lighttable = NULL;
	size_t i;

	/////////////////////
	// Calc the RGBA mask
	/////////////////////
	cmaskr = cr;
	cmaskg = cg;
	cmaskb = cb;

	maskamt = (double)(ca/24.0l);
	othermask = 1 - maskamt;
	maskamt /= 0xff;

	cmaskr *= maskamt;
	cmaskg *= maskamt;
	cmaskb *= maskamt;

	/////////////////////
	// Calc the RGBA fade mask
	/////////////////////
	cdestr = cfr;
	cdestg = cfg;
	cdestb = cfb;

	// fade alpha unused in software
	// maskamt = (double)(cfa/24.0l);
	// othermask = 1 - maskamt;
	// maskamt /= 0xff;

	// cdestr *= maskamt;
	// cdestg *= maskamt;
	// cdestb *= maskamt;

	/////////////////////
	// This code creates the colormap array used by software renderer
	/////////////////////
	{
		double r, g, b, cbrightness;
		int p;
		char *colormap_p;

		// Initialise the map and delta arrays
		// map[i] stores an RGB color (as double) for index i,
		//  which is then converted to SRB2's palette later
		// deltas[i] stores a corresponding fade delta between the RGB color and the final fade color;
		//  map[i]'s values are decremented by after each use
		for (i = 0; i < 256; i++)
		{
			r = pMasterPalette[i].s.red;
			g = pMasterPalette[i].s.green;
			b = pMasterPalette[i].s.blue;
			cbrightness = sqrt((r*r) + (g*g) + (b*b));

			map[i][0] = (cbrightness * cmaskr) + (r * othermask);
			if (map[i][0] > 255.0l)
				map[i][0] = 255.0l;
			deltas[i][0] = (map[i][0] - cdestr) / (double)fadedist;

			map[i][1] = (cbrightness * cmaskg) + (g * othermask);
			if (map[i][1] > 255.0l)
				map[i][1] = 255.0l;
			deltas[i][1] = (map[i][1] - cdestg) / (double)fadedist;

			map[i][2] = (cbrightness * cmaskb) + (b * othermask);
			if (map[i][2] > 255.0l)
				map[i][2] = 255.0l;
			deltas[i][2] = (map[i][2] - cdestb) / (double)fadedist;
		}

		// Now allocate memory for the actual colormap array itself!
		// aligned on 8 bit for asm code
		colormap_p = Z_MallocAlign((256 * 34) + 10, PU_LEVEL, NULL, 8);
		lighttable = (UINT8 *)colormap_p;

		// Calculate the palette index for each palette index, for each light level
		// (as well as the two unused colormap lines we inherited from Doom)
		for (p = 0; p < 34; p++)
		{
			for (i = 0; i < 256; i++)
			{
				*colormap_p = NearestColor((UINT8)RoundUp(map[i][0]),
					(UINT8)RoundUp(map[i][1]),
					(UINT8)RoundUp(map[i][2]));
				colormap_p++;

				if ((UINT32)p < fadestart)
					continue;
#define ABS2(x) ((x) < 0 ? -(x) : (x))
				if (ABS2(map[i][0] - cdestr) > ABS2(deltas[i][0]))
					map[i][0] -= deltas[i][0];
				else
					map[i][0] = cdestr;

				if (ABS2(map[i][1] - cdestg) > ABS2(deltas[i][1]))
					map[i][1] -= deltas[i][1];
				else
					map[i][1] = cdestg;

				if (ABS2(map[i][2] - cdestb) > ABS2(deltas[i][1]))
					map[i][2] -= deltas[i][2];
				else
					map[i][2] = cdestb;
#undef ABS2
			}
		}
	}

	return lighttable;
}

extracolormap_t *R_CreateColormap(char *p1, char *p2, char *p3)
{
	extracolormap_t *extra_colormap, *exc;

	// default values
	UINT8 cr = 0, cg = 0, cb = 0, ca = 0, cfr = 0, cfg = 0, cfb = 0, cfa = 25;
	UINT32 fadestart = 0, fadeend = 31;
	UINT8 flags = 0;
	INT32 rgba = 0, fadergba = 0x19000000;

#define HEX2INT(x) (UINT32)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : x >= '0' && x <= '9' ? 25 : 0)

	// Get base colormap value
	// First alpha-only, then full value
	if (p1[0] >= 'a' && p1[0] <= 'z' && !p1[1])
		ca = (p1[0] - 'a');
	else if (p1[0] == '#' && p1[1] >= 'a' && p1[1] <= 'z' && !p1[2])
		ca = (p1[1] - 'a');
	else if (p1[0] >= 'A' && p1[0] <= 'Z' && !p1[1])
		ca = (p1[0] - 'A');
	else if (p1[0] == '#' && p1[1] >= 'A' && p1[1] <= 'Z' && !p1[2])
		ca = (p1[1] - 'A');
	else if (p1[0] == '#')
	{
		// For each subsequent value, the value before it must exist
		// If we don't get every value, then set alpha to max
		if (p1[1] && p1[2])
		{
			cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
			if (p1[3] && p1[4])
			{
				cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
				if (p1[5] && p1[6])
				{
					cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));

					if (p1[7] >= 'a' && p1[7] <= 'z')
						ca = (p1[7] - 'a');
					else if (p1[7] >= 'A' && p1[7] <= 'Z')
						ca = (p1[7] - 'A');
					else
						ca = 25;
				}
				else
					ca = 25;
			}
			else
				ca = 25;
		}
		else
			ca = 25;
	}

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)

	// Get parameters like fadestart, fadeend, and flags
	if (p2[0] == '#')
	{
		if (p2[1])
		{
			flags = NUMFROMCHAR(p2[1]);
			if (p2[2] && p2[3])
			{
				fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
				if (p2[4] && p2[5])
					fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
			}
		}

		if (fadestart > 30)
			fadestart = 0;
		if (fadeend > 31 || fadeend < 1)
			fadeend = 31;
	}

#undef NUMFROMCHAR

	// Get fade (dark) colormap value
	// First alpha-only, then full value
	if (p3[0] >= 'a' && p3[0] <= 'z' && !p3[1])
		cfa = (p3[0] - 'a');
	else if (p3[0] == '#' && p3[1] >= 'a' && p3[1] <= 'z' && !p3[2])
		cfa = (p3[1] - 'a');
	else if (p3[0] >= 'A' && p3[0] <= 'Z' && !p3[1])
		cfa = (p3[0] - 'A');
	else if (p3[0] == '#' && p3[1] >= 'A' && p3[1] <= 'Z' && !p3[2])
		cfa = (p3[1] - 'A');
	else if (p3[0] == '#')
	{
		// For each subsequent value, the value before it must exist
		// If we don't get every value, then set alpha to max
		if (p3[1] && p3[2])
		{
			cfr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
			if (p3[3] && p3[4])
			{
				cfg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
				if (p3[5] && p3[6])
				{
					cfb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));

					if (p3[7] >= 'a' && p3[7] <= 'z')
						cfa = (p3[7] - 'a');
					else if (p3[7] >= 'A' && p3[7] <= 'Z')
						cfa = (p3[7] - 'A');
					else
						cfa = 25;
				}
				else
					cfa = 25;
			}
			else
				cfa = 25;
		}
		else
			cfa = 25;
	}
#undef ALPHA2INT
#undef HEX2INT

	// Pack rgba values into combined var
	// OpenGL also uses this instead of lighttables for rendering
	rgba = R_PutRgbaRGBA(cr, cg, cb, ca);
	fadergba = R_PutRgbaRGBA(cfr, cfg, cfb, cfa);

	// Did we just make a default colormap?
#ifdef EXTRACOLORMAPLUMPS
	if (R_CheckDefaultColormapByValues(true, true, true, rgba, fadergba, fadestart, fadeend, flags, LUMPERROR))
		return NULL;
#else
	if (R_CheckDefaultColormapByValues(true, true, true, rgba, fadergba, fadestart, fadeend, flags))
		return NULL;
#endif

	// Look for existing colormaps
#ifdef EXTRACOLORMAPLUMPS
	exc = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags, LUMPERROR);
#else
	exc = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags);
#endif
	if (exc)
		return exc;

	CONS_Debug(DBG_RENDER, "Creating Colormap: rgba(%d,%d,%d,%d) fadergba(%d,%d,%d,%d)\n",
		cr, cg, cb, ca, cfr, cfg, cfb, cfa);

	extra_colormap = Z_Calloc(sizeof (*extra_colormap), PU_LEVEL, NULL);

	extra_colormap->fadestart = (UINT16)fadestart;
	extra_colormap->fadeend = (UINT16)fadeend;
	extra_colormap->flags = flags;

	extra_colormap->rgba = rgba;
	extra_colormap->fadergba = fadergba;

#ifdef EXTRACOLORMAPLUMPS
	extra_colormap->lump = LUMPERROR;
	extra_colormap->lumpname[0] = 0;
#endif

	// Having lighttables for alpha-only entries is kind of pointless,
	// but if there happens to be a matching rgba entry that is NOT alpha-only (but has same rgb values),
	// then it needs this lighttable because we share matching entries.
	extra_colormap->colormap = R_CreateLightTable(extra_colormap);

	R_AddColormapToList(extra_colormap);

	return extra_colormap;
}

//
// R_AddColormaps()
// NOTE: The result colormap is not added to the extra_colormaps chain. You must do that yourself!
//
extracolormap_t *R_AddColormaps(extracolormap_t *exc_augend, extracolormap_t *exc_addend,
	boolean subR, boolean subG, boolean subB, boolean subA,
	boolean subFadeR, boolean subFadeG, boolean subFadeB, boolean subFadeA,
	boolean subFadeStart, boolean subFadeEnd, boolean ignoreFlags,
	boolean useAltAlpha, INT16 altAlpha, INT16 altFadeAlpha,
	boolean lighttable)
{
	INT16 red, green, blue, alpha;

	// exc_augend is added (or subtracted) onto by exc_addend
	// In Rennaisance times, the first number was considered the augend, the second number the addend
	// But since the commutative property was discovered, today they're both called addends!
	// So let's be Olde English for a hot second.

	exc_augend = R_CopyColormap(exc_augend, false);
	if(!exc_addend)
		exc_addend = R_GetDefaultColormap();

	///////////////////
	// base rgba
	///////////////////

	red = max(min(
		R_GetRgbaR(exc_augend->rgba)
			+ (subR ? -1 : 1) // subtract R
			* R_GetRgbaR(exc_addend->rgba)
		, 255), 0);

	green = max(min(
		R_GetRgbaG(exc_augend->rgba)
			+ (subG ? -1 : 1) // subtract G
			* R_GetRgbaG(exc_addend->rgba)
		, 255), 0);

	blue = max(min(
		R_GetRgbaB(exc_augend->rgba)
			+ (subB ? -1 : 1) // subtract B
			* R_GetRgbaB(exc_addend->rgba)
		, 255), 0);

	alpha = useAltAlpha ? altAlpha : R_GetRgbaA(exc_addend->rgba);
	alpha = max(min(R_GetRgbaA(exc_augend->rgba) + (subA ? -1 : 1) * alpha, 25), 0);

	exc_augend->rgba = R_PutRgbaRGBA(red, green, blue, alpha);

	///////////////////
	// fade/dark rgba
	///////////////////

	red = max(min(
		R_GetRgbaR(exc_augend->fadergba)
			+ (subFadeR ? -1 : 1) // subtract R
			* R_GetRgbaR(exc_addend->fadergba)
		, 255), 0);

	green = max(min(
		R_GetRgbaG(exc_augend->fadergba)
			+ (subFadeG ? -1 : 1) // subtract G
			* R_GetRgbaG(exc_addend->fadergba)
		, 255), 0);

	blue = max(min(
		R_GetRgbaB(exc_augend->fadergba)
			+ (subFadeB ? -1 : 1) // subtract B
			* R_GetRgbaB(exc_addend->fadergba)
		, 255), 0);

	alpha = useAltAlpha ? altFadeAlpha : R_GetRgbaA(exc_addend->fadergba);
	if (alpha == 25 && !useAltAlpha && !R_GetRgbaRGB(exc_addend->fadergba))
		alpha = 0; // HACK: fadergba A defaults at 25, so don't add anything in this case
	alpha = max(min(R_GetRgbaA(exc_augend->fadergba) + (subFadeA ? -1 : 1) * alpha, 25), 0);

	exc_augend->fadergba = R_PutRgbaRGBA(red, green, blue, alpha);

	///////////////////
	// parameters
	///////////////////

	exc_augend->fadestart = max(min(
		exc_augend->fadestart
			+ (subFadeStart ? -1 : 1) // subtract fadestart
			* exc_addend->fadestart
		, 31), 0);

	exc_augend->fadeend = max(min(
		exc_augend->fadeend
			+ (subFadeEnd ? -1 : 1) // subtract fadeend
			* (exc_addend->fadeend == 31 && !exc_addend->fadestart ? 0 : exc_addend->fadeend)
				// HACK: fadeend defaults to 31, so don't add anything in this case
		, 31), 0);

	if (!ignoreFlags) // overwrite flags with new value
		exc_augend->flags = exc_addend->flags;

	///////////////////
	// put it together
	///////////////////

	exc_augend->colormap = lighttable ? R_CreateLightTable(exc_augend) : NULL;
	exc_augend->next = exc_augend->prev = NULL;
	return exc_augend;
}

// Thanks to quake2 source!
// utils3/qdata/images.c
UINT8 NearestPaletteColor(UINT8 r, UINT8 g, UINT8 b, RGBA_t *palette)
{
	int dr, dg, db;
	int distortion, bestdistortion = 256 * 256 * 4, bestcolor = 0, i;

	// Use master palette if none specified
	if (palette == NULL)
		palette = pMasterPalette;

	for (i = 0; i < 256; i++)
	{
		dr = r - palette[i].s.red;
		dg = g - palette[i].s.green;
		db = b - palette[i].s.blue;
		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
				return (UINT8)i;

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return (UINT8)bestcolor;
}

// Rounds off floating numbers and checks for 0 - 255 bounds
static int RoundUp(double number)
{
	if (number > 255.0l)
		return 255;
	if (number < 0.0l)
		return 0;

	if ((int)number <= (int)(number - 0.5f))
		return (int)number + 1;

	return (int)number;
}

#ifdef EXTRACOLORMAPLUMPS
const char *R_NameForColormap(extracolormap_t *extra_colormap)
{
	if (!extra_colormap)
		return "NONE";

	if (extra_colormap->lump == LUMPERROR)
		return "INLEVEL";

	return extra_colormap->lumpname;
}
#endif

//
// build a table for quick conversion from 8bpp to 15bpp
//

//
// added "static inline" keywords, linking with the debug version
// of allegro, it have a makecol15 function of it's own, now
// with "static inline" keywords,it sloves this problem ;)
//
FUNCMATH static inline int makecol15(int r, int g, int b)
{
	return (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3)));
}

static void R_Init8to16(void)
{
	UINT8 *palette;
	int i;

	palette = W_CacheLumpName("PLAYPAL",PU_CACHE);

	for (i = 0; i < 256; i++)
	{
		// PLAYPAL uses 8 bit values
		color8to16[i] = (INT16)makecol15(palette[0], palette[1], palette[2]);
		palette += 3;
	}

	// test a big colormap
	hicolormaps = Z_Malloc(16384*sizeof(*hicolormaps), PU_STATIC, NULL);
	for (i = 0; i < 16384; i++)
		hicolormaps[i] = (INT16)(i<<1);
}

//
// R_InitData
//
// Locates all the lumps that will be used by all views
// Must be called after W_Init.
//
void R_InitData(void)
{
	if (highcolor)
	{
		CONS_Printf("InitHighColor...\n");
		R_Init8to16();
	}

	CONS_Printf("R_LoadTextures()...\n");
	R_LoadTextures();

	CONS_Printf("P_InitPicAnims()...\n");
	P_InitPicAnims();

	CONS_Printf("R_InitSprites()...\n");
	R_InitSpriteLumps();
	R_InitSprites();

	CONS_Printf("R_InitColormaps()...\n");
	R_InitColormaps();
}

void R_ClearTextureNumCache(boolean btell)
{
	if (tidcache)
		Z_Free(tidcache);
	tidcache = NULL;
	if (btell)
		CONS_Debug(DBG_SETUP, "Fun Fact: There are %d textures used in this map.\n", tidcachelen);
	tidcachelen = 0;
}

//
// R_CheckTextureNumForName
//
// Check whether texture is available. Filter out NoTexture indicator.
//
INT32 R_CheckTextureNumForName(const char *name)
{
	INT32 i;

	// "NoTexture" marker.
	if (name[0] == '-')
		return 0;

	for (i = 0; i < tidcachelen; i++)
		if (!strncasecmp(tidcache[i].name, name, 8))
			return tidcache[i].id;

	// Need to parse the list backwards, so textures loaded more recently are used in lieu of ones loaded earlier
	//for (i = 0; i < numtextures; i++) <- old
	for (i = (numtextures - 1); i >= 0; i--) // <- new
		if (!strncasecmp(textures[i]->name, name, 8))
		{
			tidcachelen++;
			Z_Realloc(tidcache, tidcachelen * sizeof(*tidcache), PU_STATIC, &tidcache);
			strncpy(tidcache[tidcachelen-1].name, name, 8);
			tidcache[tidcachelen-1].name[8] = '\0';
#ifndef ZDEBUG
			CONS_Debug(DBG_SETUP, "texture #%s: %s\n", sizeu1(tidcachelen), tidcache[tidcachelen-1].name);
#endif
			tidcache[tidcachelen-1].id = i;
			return i;
		}

	return -1;
}

//
// R_TextureNumForName
//
// Calls R_CheckTextureNumForName, aborts with error message.
//
INT32 R_TextureNumForName(const char *name)
{
	const INT32 i = R_CheckTextureNumForName(name);

	if (i == -1)
	{
		static INT32 redwall = -2;
		CONS_Debug(DBG_SETUP, "WARNING: R_TextureNumForName: %.8s not found\n", name);
		if (redwall == -2)
			redwall = R_CheckTextureNumForName("REDWALL");
		if (redwall != -1)
			return redwall;
		return 1;
	}
	return i;
}

//
// R_PrecacheLevel
//
// Preloads all relevant graphics for the level.
//
void R_PrecacheLevel(void)
{
	char *texturepresent, *spritepresent;
	size_t i, j, k;
	lumpnum_t lump;

	thinker_t *th;
	spriteframe_t *sf;

	if (demoplayback)
		return;

	// do not flush the memory, Z_Malloc twice with same user will cause error in Z_CheckHeap()
	if (rendermode != render_soft)
		return;

	// Precache flats.
	flatmemory = P_PrecacheLevelFlats();

	//
	// Precache textures.
	//
	// no need to precache all software textures in 3D mode
	// (note they are still used with the reference software view)
	texturepresent = calloc(numtextures, sizeof (*texturepresent));
	if (texturepresent == NULL) I_Error("%s: Out of memory looking up textures", "R_PrecacheLevel");

	for (j = 0; j < numsides; j++)
	{
		// huh, a potential bug here????
		if (sides[j].toptexture >= 0 && sides[j].toptexture < numtextures)
			texturepresent[sides[j].toptexture] = 1;
		if (sides[j].midtexture >= 0 && sides[j].midtexture < numtextures)
			texturepresent[sides[j].midtexture] = 1;
		if (sides[j].bottomtexture >= 0 && sides[j].bottomtexture < numtextures)
			texturepresent[sides[j].bottomtexture] = 1;
	}

	// Sky texture is always present.
	// Note that F_SKY1 is the name used to indicate a sky floor/ceiling as a flat,
	// while the sky texture is stored like a wall texture, with a skynum dependent name.
	texturepresent[skytexture] = 1;

	texturememory = 0;
	for (j = 0; j < (unsigned)numtextures; j++)
	{
		if (!texturepresent[j])
			continue;

		if (!texturecache[j])
			R_GenerateTexture(j);
		// pre-caching individual patches that compose textures became obsolete,
		// since we cache entire composite textures
	}
	free(texturepresent);

	//
	// Precache sprites.
	//
	spritepresent = calloc(numsprites, sizeof (*spritepresent));
	if (spritepresent == NULL) I_Error("%s: Out of memory looking up sprites", "R_PrecacheLevel");

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			spritepresent[((mobj_t *)th)->sprite] = 1;

	spritememory = 0;
	for (i = 0; i < numsprites; i++)
	{
		if (!spritepresent[i])
			continue;

		for (j = 0; j < sprites[i].numframes; j++)
		{
			sf = &sprites[i].spriteframes[j];
#define cacheang(a) {\
		lump = sf->lumppat[a];\
		if (devparm)\
			spritememory += W_LumpLength(lump);\
		W_CachePatchNum(lump, PU_PATCH);\
	}
			// see R_InitSprites for more about lumppat,lumpid
			switch (sf->rotate)
			{
				case SRF_SINGLE:
					cacheang(0);
					break;
				case SRF_2D:
					cacheang(2);
					cacheang(6);
					break;
				default:
					k = (sf->rotate & SRF_3DGE ? 16 : 8);
					while (k--)
						cacheang(k);
					break;
			}
#undef cacheang
		}
	}
	free(spritepresent);

	// FIXME: this is no longer correct with OpenGL render mode
	CONS_Debug(DBG_SETUP, "Precache level done:\n"
			"flatmemory:    %s k\n"
			"texturememory: %s k\n"
			"spritememory:  %s k\n", sizeu1(flatmemory>>10), sizeu2(texturememory>>10), sizeu3(spritememory>>10));
}
