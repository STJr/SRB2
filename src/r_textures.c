// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_textures.c
/// \brief Texture generation.

#include "doomdef.h"
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "m_misc.h"
#include "r_data.h"
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h" // levelflats
#include "byteptr.h"
#include "dehacked.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h" // HWR_LoadMapTextures
#endif

#include <errno.h>

//
// TEXTURE_T CACHING
// When a texture is first needed, it counts the number of composite columns
//  required in the texture and allocates space for a column directory and
//  any new columns.
// The directory will simply point inside other patches if there is only one
//  patch in a given column, but any columns with multiple patches will have
//  new column_ts generated.
//

INT32 numtextures = 0; // total number of textures found,
// size of following tables

texture_t **textures = NULL;
column_t **texturecolumns; // columns for each texture
UINT8 **texturecache; // graphics data for each generated full-size texture

INT32 *texturewidth;
fixed_t *textureheight; // needed for texture pegging

INT32 *texturetranslation;

// Painfully simple texture id cacheing to make maps load faster. :3
static struct {
	char name[9];
	UINT32 hash;
	INT32 id;
	UINT8 type;
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
static void R_DrawColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source;
	INT32 originy = originPatch->originy;

	(void)patchheight; // This parameter is unused

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		source = column->pixels + post->data_offset;
		count = post->length;
		position = originy + post->topdelta;

		if (position < 0)
		{
			count += position;
			source -= position; // start further down the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		if (count > 0)
		{
			M_Memcpy(cache + position, source, count);
			memset(opaque_pixels + position, true, count);
		}
	}
}

//
// R_DrawFlippedColumnInCache
// Similar to R_DrawColumnInCache; it draws the column inverted, however.
//
static void R_DrawFlippedColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 originy = originPatch->originy;
	INT32 topdelta;
	UINT8 *is_opaque;

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		topdelta = patchheight - post->length - post->topdelta;
		source = column->pixels + post->data_offset + (post->length - 1);
		count = post->length;
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
		is_opaque = opaque_pixels + position;

		if (count > 0)
		{
			for (; dest < cache + position + count; --source, dest++, is_opaque++)
			{
				*dest = *source;
				*is_opaque = true;
			}
		}
	}
}

//
// R_DrawBlendColumnInCache
// Draws a translucent column into the cache.
//
static void R_DrawBlendColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 originy = originPatch->originy;
	UINT8 *is_opaque;

	(void)patchheight; // This parameter is unused

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		source = column->pixels + post->data_offset;
		count = post->length;
		position = originy + post->topdelta;

		if (position < 0)
		{
			count += position;
			source -= position; // start further down the column
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		dest = cache + position;
		is_opaque = opaque_pixels + position;

		if (count > 0)
		{
			for (; dest < cache + position + count; source++, dest++, is_opaque++)
			{
				if (originPatch->alpha <= ASTTextureBlendingThreshold[1] && !(*is_opaque))
					continue;
				*dest = ASTBlendPaletteIndexes(*dest, *source, originPatch->style, originPatch->alpha);
				*is_opaque = true;
			}
		}
	}
}

//
// R_DrawBlendFlippedColumnInCache
// Similar to the one above except that the column is inverted.
//
static void R_DrawBlendFlippedColumnInCache(column_t *column, UINT8 *cache, texpatch_t *originPatch, INT32 cacheheight, INT32 patchheight, UINT8 *opaque_pixels)
{
	INT32 count, position;
	UINT8 *source, *dest;
	INT32 originy = originPatch->originy;
	INT32 topdelta;
	UINT8 *is_opaque;

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		topdelta = patchheight - post->length - post->topdelta;
		source = column->pixels + post->data_offset + (post->length - 1);
		count = post->length;
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
		is_opaque = opaque_pixels + position;

		if (count > 0)
		{
			for (; dest < cache + position + count; --source, dest++, is_opaque++)
			{
				if (originPatch->alpha <= ASTTextureBlendingThreshold[1] && !(*is_opaque))
					continue;
				*dest = ASTBlendPaletteIndexes(*dest, *source, originPatch->style, originPatch->alpha);
				*is_opaque = true;
			}
		}
	}
}

//
// R_GenerateTexture
//
// Allocate space for full size texture, either single patch or 'composite'
// Build the full textures from patches.
// The texture caching system is a little more hungry of memory, but has
// been simplified for the sake of highcolor, dynamic lighting, & speed.
//
// This is not optimised, but it's supposed to be executed only once
// per level, when enough memory is available.
//
UINT8 *R_GenerateTexture(size_t texnum)
{
	UINT8 *block;
	UINT8 *blocktex;
	UINT8 *temp_block;
	texture_t *texture;
	texpatch_t *patch;
	int x, x1, x2, i, width, height;
	size_t blocksize;
	unsigned *column_posts;
	UINT8 *opaque_pixels;
	column_t *columns, *temp_columns;
	post_t *posts, *temp_posts = NULL;
	size_t total_posts = 0;
	size_t total_pixels = 0;

	I_Assert(texnum <= (size_t)numtextures);
	texture = textures[texnum];
	I_Assert(texture != NULL);

	// Just create a composite one
	if (texture->type == TEXTURETYPE_FLAT)
		goto multipatch;

	// single-patch textures can have holes in them and may be used on
	// 2sided lines so they need to be kept in 'packed' format
	// BUT this is wrong for skies and walls with over 255 pixels,
	// so check if there's holes and if not strip the posts.
	if (texture->patchcount == 1)
	{
		patch = &texture->patches[0];

		UINT16 wadnum = patch->wad;
		UINT16 lumpnum = patch->lump;
		UINT8 *pdata;
		softwarepatch_t *realpatch;

#ifndef NO_PNG_LUMPS
		UINT8 header[PNG_HEADER_SIZE];

		W_ReadLumpHeaderPwad(wadnum, lumpnum, header, PNG_HEADER_SIZE, 0);

		// Not worth converting
		if (Picture_IsLumpPNG(header, W_LumpLengthPwad(wadnum, lumpnum)))
			goto multipatch;
#endif

		pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		realpatch = (softwarepatch_t *)pdata;

		texture->transparency = false;

		// Check the patch for holes.
		if (texture->width > SHORT(realpatch->width) || texture->height > SHORT(realpatch->height))
			texture->transparency = true;
		else
		{
			UINT8 *colofs = (UINT8 *)realpatch->columnofs;
			for (x = 0; x < texture->width; x++)
			{
				doompost_t *col = (doompost_t *)((UINT8 *)realpatch + LONG(*(UINT32 *)&colofs[x<<2]));
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
					col = (doompost_t *)((UINT8 *)col + col->length + 4);
				}
				if (y < texture->height)
					texture->transparency = true; // this texture is HOLEy! D:
			}
		}

		// If the patch uses transparency, we have to save it this way.
		if (texture->transparency)
		{
			texture->flip = patch->flip;

			Patch_CalcDataSizes(realpatch, &total_pixels, &total_posts);

			blocksize = (sizeof(column_t) * texture->width) + (sizeof(post_t) * total_posts) + (sizeof(UINT8) * total_pixels);
			texturememory += blocksize;

			block = Z_Calloc(blocksize, PU_STATIC, &texturecache[texnum]);
			blocktex = block;

			columns = (column_t *)(block + (sizeof(UINT8) * total_pixels));
			posts = (post_t *)(block + (sizeof(UINT8) * total_pixels) + (sizeof(column_t) * texture->width));

			texturecolumns[texnum] = columns;

			// Handles flipping as well.
			// we can't as easily flip the patch vertically sadly though,
			//  we have wait until the texture itself is drawn to do that
			Patch_MakeColumns(realpatch, texture->width, texture->width, blocktex, columns, posts, patch->flip & 1);

			Z_Free(pdata);

			goto done;
		}

		// Otherwise, do multipatch format.
	}

	// multi-patch textures (or 'composite')
	multipatch:
	texture->flip = 0;

	// To make things easier, I just allocate WxH always
	total_pixels = texture->width * texture->height;

	opaque_pixels = Z_Calloc(total_pixels * sizeof(UINT8), PU_STATIC, NULL);
	temp_columns = Z_Calloc(sizeof(column_t) * texture->width, PU_STATIC, NULL);
	temp_block = Z_Calloc(total_pixels, PU_STATIC, NULL);

#ifdef TEXTURE_255_IS_TRANSPARENT
	texture->transparency = false;

	// Transparency hack
	memset(temp_block, TRANSPARENTPIXEL, total_pixels);
#else
	texture->transparency = true;
#endif

	for (x = 0; x < texture->width; x++)
	{
		column_t *column = &temp_columns[x];
		column->num_posts = 0;
		column->posts = NULL;
		column->pixels = temp_block + (texture->height * x);
	}

	// Composite the columns together.
	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		static void (*columnDrawer)(column_t *, UINT8 *, texpatch_t *, INT32, INT32, UINT8 *);
		if (patch->style != AST_COPY)
			columnDrawer = (patch->flip & 2) ? R_DrawBlendFlippedColumnInCache : R_DrawBlendColumnInCache;
		else
			columnDrawer = (patch->flip & 2) ? R_DrawFlippedColumnInCache : R_DrawColumnInCache;

		UINT16 wadnum = patch->wad;
		lumpnum_t lumpnum = patch->lump;
		UINT8 *pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		patch_t *realpatch = NULL;
		boolean free_patch = true;

#ifndef NO_PNG_LUMPS
		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		if (Picture_IsLumpPNG(pdata, lumplength))
			realpatch = (patch_t *)Picture_PNGConvert(pdata, PICFMT_PATCH, NULL, NULL, NULL, NULL, lumplength, NULL, 0);
		else
#endif
		if (texture->type == TEXTURETYPE_FLAT)
			realpatch = (patch_t *)Picture_Convert(PICFMT_FLAT, pdata, PICFMT_PATCH, 0, NULL, texture->width, texture->height, 0, 0, PICFLAGS_USE_TRANSPARENTPIXEL);
		else
		{
			// If this patch has already been loaded, we just use it from the cache.
			realpatch = W_GetCachedPatchNumPwad(wadnum, lumpnum);
			free_patch = false;

			// Otherwise, we load it here.
			if (realpatch == NULL)
				realpatch = W_CachePatchNumPwad(wadnum, lumpnum, PU_PATCH);
		}

		x1 = patch->originx;
		width = realpatch->width;
		height = realpatch->height;
		x2 = x1 + width;

		if (x1 > texture->width || x2 < 0)
		{
			if (free_patch)
				Patch_Free(realpatch);
			continue; // patch not located within texture's x bounds, ignore
		}

		if (patch->originy > texture->height || (patch->originy + height) < 0)
		{
			if (free_patch)
				Patch_Free(realpatch);
			continue; // patch not located within texture's y bounds, ignore
		}

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
			INT32 colx;

			if (patch->flip & 1)
				colx = (x1+width-1)-x;
			else
				colx = x-x1;

			column_t *patchcol = &realpatch->columns[colx];

			if (patchcol->num_posts > 0)
				columnDrawer(patchcol, temp_columns[x].pixels, patch, texture->height, height, &opaque_pixels[x * texture->height]);
		}

		if (free_patch)
			Patch_Free(realpatch);
	}

	// Now write the columns
	column_posts = Z_Calloc(sizeof(unsigned) * texture->width, PU_STATIC, NULL);

#ifdef TEXTURE_255_IS_TRANSPARENT
	total_posts = texture->width;
	temp_posts = Z_Realloc(temp_posts, sizeof(post_t) * total_posts, PU_CACHE, NULL);
#endif

	for (x = 0; x < texture->width; x++)
	{
		post_t *post = NULL;

		column_t *column = &temp_columns[x];

#ifdef TEXTURE_255_IS_TRANSPARENT
		post = &temp_posts[x];
		post->topdelta = 0;
		post->length = texture->height;
		post->data_offset = 0;
		column_posts[x] = x;
		column->num_posts = 1;
#else
		boolean was_opaque = false;

		column_posts[x] = (unsigned)-1;

		for (INT32 y = 0; y < texture->height; y++)
		{
			// End span if we have a transparent pixel
			if (!opaque_pixels[(x * texture->height) + y])
			{
				was_opaque = false;
				continue;
			}

			if (!was_opaque)
			{
				total_posts++;

				temp_posts = Z_Realloc(temp_posts, sizeof(post_t) * total_posts, PU_CACHE, NULL);
				post = &temp_posts[total_posts - 1];
				post->topdelta = (size_t)y;
				post->length = 0;
				post->data_offset = (size_t)y;
				if (column_posts[x] == (unsigned)-1)
					column_posts[x] = total_posts - 1;
				column->num_posts++;
			}

			was_opaque = true;

			post->length++;
		}
#endif
	}

	blocksize = (sizeof(column_t) * texture->width) + (sizeof(post_t) * total_posts) + (sizeof(UINT8) * total_pixels);
	texturememory += blocksize;

	block = Z_Calloc(blocksize, PU_STATIC, &texturecache[texnum]);
	blocktex = block;

	memcpy(blocktex, temp_block, total_pixels);

	Z_Free(temp_block);

	columns = (column_t *)(block + (sizeof(UINT8) * total_pixels));
	posts = (post_t *)(block + (sizeof(UINT8) * total_pixels) + (sizeof(column_t) * texture->width));

	memcpy(columns, temp_columns, sizeof(column_t) * texture->width);
	memcpy(posts, temp_posts, sizeof(post_t) * total_posts);

	texturecolumns[texnum] = columns;

	for (x = 0; x < texture->width; x++)
	{
		column_t *column = &columns[x];
		if (column->num_posts > 0)
			column->posts = &posts[column_posts[x]];
		column->pixels = blocktex + (texture->height * x);
	}

done:
	// Now that the texture has been built in column cache, it is purgable from zone memory.
	Z_ChangeTag(block, PU_CACHE);
	return blocktex;
}

UINT8 *R_GetFlatForTexture(size_t texnum)
{
	if (texnum >= (unsigned)numtextures)
		return NULL;

	texture_t *texture = textures[texnum];
	if (texture->flat != NULL)
		return texture->flat;

	// Special case: Textures that are flats don't need to be converted FROM a texture INTO a flat.
	if (texture->type == TEXTURETYPE_FLAT)
	{
		texpatch_t *patch = &texture->patches[0];
		UINT16 wadnum = patch->wad;
		lumpnum_t lumpnum = patch->lump;
		UINT8 *pdata = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);

#ifndef NO_PNG_LUMPS
		if (Picture_IsLumpPNG(pdata, lumplength))
			texture->flat = Picture_PNGConvert(pdata, PICFMT_FLAT, NULL, NULL, NULL, NULL, lumplength, NULL, 0);
		else
#endif
		{
			texture->flat = Z_Malloc(lumplength, PU_STATIC, NULL);
			memcpy(texture->flat, pdata, lumplength);
		}

		Z_SetUser(texture->flat, &texture->flat);

		Z_Free(pdata);
	}
	else
		texture->flat = (UINT8 *)Picture_TextureToFlat(texnum);

	flatmemory += texture->width * texture->height;

	return texture->flat;
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

column_t *R_GetColumn(fixed_t tex, INT32 col)
{
	INT32 width = texturewidth[tex];
	if (width & (width - 1))
		col = (UINT32)col % width;
	else
		col &= (width - 1);

	return &texturecolumns[tex][col];
}

INT32 R_GetTextureNumForFlat(levelflat_t *levelflat)
{
	return texturetranslation[levelflat->texture_id];
}

void *R_GetFlat(levelflat_t *levelflat)
{
	if (levelflat->type == LEVELFLAT_NONE)
		return NULL;

	return R_GetFlatForTexture(R_GetTextureNumForFlat(levelflat));
}

//
// Checks if the current flat's dimensions are powers of two
//
boolean R_CheckPowersOfTwo(void)
{
	if (ds_flatwidth > 2048 || ds_flatheight > 2048)
		return false;

	boolean wpow2 = !(ds_flatwidth & (ds_flatwidth - 1));
	boolean hpow2 = !(ds_flatheight & (ds_flatheight - 1));

	return ds_flatwidth == ds_flatheight && wpow2 && hpow2;
}

//
// Checks if the current flat's dimensions are 1x1
//
boolean R_CheckSolidColorFlat(void)
{
	return ds_flatwidth == 1 && ds_flatheight == 1;
}

//
// Returns the flat size corresponding to the length of a lump
//
UINT16 R_GetFlatSize(size_t length)
{
	switch (length)
	{
		case 4194304: // 2048x2048 lump
			return 2048;
		case 1048576: // 1024x1024 lump
			return 1024;
		case 262144:// 512x512 lump
			return 512;
		case 65536: // 256x256 lump
			return 256;
		case 16384: // 128x128 lump
			return 128;
		case 1024: // 32x32 lump
			return 32;
		case 256: // 16x16 lump
			return 16;
		case 64: // 8x8 lump
			return 8;
		case 16: // 4x4 lump
			return 4;
		case 4: // 2x2 lump
			return 2;
		case 1: // 1x1 lump
			return 1;
		default: // 64x64 lump
			return 64;
	}
}

//
// Determines a flat's width bits from its size
//
UINT8 R_GetFlatBits(INT32 size)
{
	switch (size)
	{
		case 2048: return 11;
		case 1024: return 10;
		case 512:  return 9;
		case 256:  return 8;
		case 128:  return 7;
		case 32:   return 5;
		case 16:   return 4;
		case 8:    return 3;
		case 4:    return 2;
		case 2:    return 1;
		case 1:    return 0;
		default:   return 6; // 64x64
	}
}

void R_SetFlatVars(size_t length)
{
	UINT16 size = R_GetFlatSize(length);
	UINT8 bits = R_GetFlatBits(size);

	ds_flatwidth = ds_flatheight = size;

	if (bits == 0)
		return;

	nflatshiftup = 16 - bits;
	nflatxshift = 16 + nflatshiftup;
	nflatyshift = nflatxshift - bits;
	nflatmask = (size - 1) * size;
}

//
// Empty the texture cache (used for load wad at runtime)
//
void R_FlushTextureCache(void)
{
	INT32 i;

	if (numtextures)
		for (i = 0; i < numtextures; i++)
		{
			Z_Free(textures[i]->flat);
			Z_Free(texturecache[i]);
		}
}

// Need these prototypes for later; defining them here instead of r_textures.h so they're "private"
int R_CountTexturesInTEXTURESLump(UINT16 wadNum, UINT16 lumpNum);
void R_ParseTEXTURESLump(UINT16 wadNum, UINT16 lumpNum, INT32 *index);

static void R_AddSinglePatchTexture(INT32 i, UINT16 wadnum, UINT16 lumpnum, INT16 width, INT16 height, unsigned type)
{
	texture_t *texture = Z_Calloc(sizeof(texture_t) + sizeof(texpatch_t), PU_STATIC, NULL);

	// Set texture properties.
	M_Memcpy(texture->name, W_CheckNameForNumPwad(wadnum, lumpnum), sizeof(texture->name));
	texture->hash = quickncasehash(texture->name, 8);

	texture->width = width;
	texture->height = height;

	texture->type = type;
	texture->patchcount = 1;
	texture->flip = 0;

	// Allocate information for the texture's patches.
	texpatch_t *patch = &texture->patches[0];

	patch->originx = patch->originy = 0;
	patch->wad = wadnum;
	patch->lump = lumpnum;
	patch->flip = 0;

	texturewidth[i] = texture->width;
	textureheight[i] = texture->height << FRACBITS;

	textures[i] = texture;
}

static INT32
Rloadflats (INT32 i, INT32 w)
{
	UINT16 j, numlumps = 0;
	UINT16 texstart = 0, texend = 0;
	UINT32 *list = NULL;

	// Get every lump inside the Flats/ folder
	if (W_FileHasFolders(wadfiles[w]))
	{
		W_GetFolderLumpsPwad("Flats/", (UINT16)w, &list, NULL, &numlumps);
	}
	else
	{
		// Else, get the lumps between F_START/F_END markers
		texstart = W_CheckNumForMarkerStartPwad("F_START", (UINT16)w, 0);
		texend = W_CheckNumForNamePwad("F_END", (UINT16)w, texstart);
		if (texstart == INT16_MAX || texend == INT16_MAX)
			return i;

		numlumps = texend - texstart;
	}

	// Now add every entry as a texture
	for (j = 0; j < numlumps; j++)
	{
		UINT16 wadnum = list ? WADFILENUM(list[j]) : (UINT16)w;
		UINT16 lumpnum = list ? LUMPNUM(list[j]) : (texstart + j);

		size_t lumplength = W_LumpLengthPwad(wadnum, lumpnum);
		size_t flatsize = R_GetFlatSize(lumplength);

		INT16 width = flatsize, height = flatsize;

#ifndef NO_PNG_LUMPS
		UINT8 header[PNG_HEADER_SIZE];

		W_ReadLumpHeaderPwad(wadnum, lumpnum, header, sizeof header, 0);

		if (Picture_IsLumpPNG(header, lumplength))
		{
			INT32 texw, texh;
			UINT8 *flatlump = W_CacheLumpNumPwad(wadnum, lumpnum, PU_CACHE);
			if (Picture_PNGDimensions((UINT8 *)flatlump, &texw, &texh, NULL, NULL, lumplength))
			{
				width = (INT16)texw;
				height = (INT16)texh;
			}
			else
			{
				width = 1;
				height = 1;
			}
			Z_Free(flatlump);
		}
#endif

		// printf("\"%s\" (wad: %u, lump: %u) is a flat, dimensions %d x %d\n",W_CheckNameForNumPwad(wadnum,lumpnum),wadnum,lumpnum,width,height);

		R_AddSinglePatchTexture(i, wadnum, lumpnum, width, height, TEXTURETYPE_FLAT);

		i++;
	}

	if (list)
		Z_Free(list);

	return i;
}

#define TX_START "TX_START"
#define TX_END "TX_END"

static INT32
Rloadtextures (INT32 i, INT32 w)
{
	UINT16 j, numlumps = 0;
	UINT16 texstart = 0, texend = 0;
	UINT16 texturesLumpPos;
	UINT32 *list = NULL;

	// Get every lump inside the Textures/ folder
	if (W_FileHasFolders(wadfiles[w]))
	{
		W_GetFolderLumpsPwad("Textures/", (UINT16)w, &list, NULL, &numlumps);

		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		while (texturesLumpPos != INT16_MAX)
		{
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);
			texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, texturesLumpPos + 1);
		}
	}
	// Else, get the lumps between TX_START/TX_END markers
	else
	{
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);
		if (texturesLumpPos != INT16_MAX)
			R_ParseTEXTURESLump(w, texturesLumpPos, &i);

		texstart = W_CheckNumForMarkerStartPwad(TX_START, (UINT16)w, 0);
		texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, texstart);
		if (texstart == INT16_MAX || texend == INT16_MAX)
			return i;

		numlumps = texend - texstart;
	}

	// Now add every entry as a texture
	for (j = 0; j < numlumps; j++)
	{
		UINT16 wadnum = list ? WADFILENUM(list[j]) : (UINT16)w;
		UINT16 lumpnum = list ? LUMPNUM(list[j]) : (texstart + j);

		INT16 width = 0, height = 0;

		if (!W_ReadPatchHeaderPwad(wadnum, lumpnum, &width, &height, NULL, NULL))
		{
			width = 1;
			height = 1;
		}

		// printf("\"%s\" (wad: %u, lump: %u) is a single patch, dimensions %d x %d\n",W_CheckNameForNumPwad(wadnum,lumpnum),wadnum,lumpnum,width,height);

		R_AddSinglePatchTexture(i, wadnum, lumpnum, width, height, TEXTURETYPE_SINGLEPATCH);

		i++;
	}

	if (list)
		Z_Free(list);

	return i;
}

static INT32
count_range
(		const char * marker_start,
		const char * marker_end,
		const char * folder,
		UINT16 wadnum)
{
	INT32 count = 0;

	if (W_FileHasFolders(wadfiles[wadnum]))
		count += W_CountFolderLumpsPwad(folder, wadnum);
	else
	{
		UINT16 texstart = W_CheckNumForMarkerStartPwad(marker_start, wadnum, 0);
		UINT16 texend = W_CheckNumForNamePwad(marker_end, wadnum, texstart);

		if (texstart != INT16_MAX && texend != INT16_MAX)
			count += (texend - texstart);
	}

	return count;
}

static INT32 R_CountTextures(UINT16 wadnum)
{
	UINT16 texturesLumpPos;
	INT32 count = 0;

	// Load patches and textures.

	// Get the number of textures to check.
	// NOTE: Make SURE the system does not process
	// the markers.
	// This system will allocate memory for all duplicate/patched textures even if it never uses them,
	// but the alternative is to spend a ton of time checking and re-checking all previous entries just to skip any potentially patched textures.

	count += count_range("F_START", "F_END", "flats/", wadnum);

	// Count the textures from TEXTURES lumps
	texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", wadnum, 0);

	while (texturesLumpPos != INT16_MAX)
	{
		count += R_CountTexturesInTEXTURESLump(wadnum, texturesLumpPos);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", wadnum, texturesLumpPos + 1);
	}

	// Count single-patch textures
	count += count_range(TX_START, TX_END, "textures/", wadnum);

	return count;
}

static void
recallocuser
(		void * user,
		size_t old,
		size_t new)
{
	char *p = Z_Realloc(*(void**)user,
			new, PU_STATIC, user);

	if (new > old)
		memset(&p[old], 0, (new - old));
}

static void R_AllocateTextures(INT32 add)
{
	const INT32 newtextures = (numtextures + add);
	const size_t newsize = newtextures * sizeof (void*);
	const size_t oldsize = numtextures * sizeof (void*);

	INT32 i;

	// Allocate memory and initialize to 0 for all the textures we are initialising.
	recallocuser(&textures, oldsize, newsize);

	// Allocate texture column offset table.
	recallocuser(&texturecolumns, oldsize, newsize);
	// Allocate texture referencing cache.
	recallocuser(&texturecache, oldsize, newsize);
	// Allocate texture width table.
	recallocuser(&texturewidth, oldsize, newsize);
	// Allocate texture height table.
	recallocuser(&textureheight, oldsize, newsize);
	// Create translation table for global animation.
	Z_Realloc(texturetranslation, (newtextures + 1) * sizeof(*texturetranslation), PU_STATIC, &texturetranslation);

	for (i = 0; i < numtextures; ++i)
	{
		// R_FlushTextureCache relies on the user for
		// Z_Free, texturecache has been reallocated so the
		// user is now garbage memory.
		Z_SetUser(texturecache[i],
				(void**)&texturecache[i]);
	}

	while (i < newtextures)
	{
		texturetranslation[i] = i;
		i++;
	}
}

static INT32 R_DefineTextures(INT32 i, UINT16 w)
{
	i = Rloadflats(i, w);
	return Rloadtextures(i, w);
}

static void R_FinishLoadingTextures(INT32 add)
{
	numtextures += add;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_LoadMapTextures(numtextures);
#endif
}

//
// R_LoadTextures
// Initializes the texture list with the textures from the world map.
//
void R_LoadTextures(void)
{
	INT32 i, w;
	INT32 newtextures = 0;

	for (w = 0; w < numwadfiles; w++)
	{
		newtextures += R_CountTextures((UINT16)w);
	}

	// If no textures found by this point, bomb out
	if (!newtextures)
		I_Error("No textures detected in any WADs!\n");

	R_AllocateTextures(newtextures);

	for (i = 0, w = 0; w < numwadfiles; w++)
	{
		i = R_DefineTextures(i, w);
	}

	R_FinishLoadingTextures(newtextures);
}

void R_LoadTexturesPwad(UINT16 wadnum)
{
	INT32 newtextures = R_CountTextures(wadnum);

	R_AllocateTextures(newtextures);
	R_DefineTextures(numtextures, wadnum);
	R_FinishLoadingTextures(newtextures);
}

static lumpnum_t W_GetTexPatchLumpNum(const char *name)
{
	// Flats as a texture patch crashes horribly, and flats
	// can share the same name as textures.

	// But even if they worked, we want to prioritize:
	// Patches -> Textures -> anything else

	enum
	{
		USE_PATCHES,
		USE_TEXTURES,
		USE__MAX,
	};

	lumpnum_t lump = LUMPERROR;
	INT32 lump_type_it;
	

	for (lump_type_it = 0; lump_type_it < USE__MAX; lump_type_it++)
	{
		INT32 i;

		for (i = numwadfiles - 1; i >= 0; i--) // Scan wad files backwards so patched lumps take precedent
		{
			lumpnum_t start = LUMPERROR;
			lumpnum_t end = LUMPERROR;

			switch (wadfiles[i]->type)
			{
				case RET_WAD:
					if (lump_type_it == USE_PATCHES)
					{
						if ((start = W_CheckNumForMarkerStartPwad("P_START", (UINT16)i, 0)) == INT16_MAX)
							continue;
						else if ((end = W_CheckNumForNamePwad("P_END", (UINT16)i, start)) == INT16_MAX)
							continue;
					}
					else if (lump_type_it == USE_TEXTURES)
					{
						if ((start = W_CheckNumForMarkerStartPwad("TX_START", (UINT16)i, 0)) == INT16_MAX)
							continue;
						else if ((end = W_CheckNumForNamePwad("TX_END", (UINT16)i, start)) == INT16_MAX)
							continue;
					}
					break;
				case RET_PK3:
				case RET_FOLDER:
					if (lump_type_it == USE_PATCHES)
					{
						if ((start = W_CheckNumForFolderStartPK3("Patches/", i, 0)) == INT16_MAX)
							continue;
						if ((end = W_CheckNumForFolderEndPK3("Patches/", i, start)) == INT16_MAX)
							continue;
					}
					else if (lump_type_it == USE_TEXTURES)
					{
						if ((start = W_CheckNumForFolderStartPK3("Textures/", i, 0)) == INT16_MAX)
							continue;
						if ((end = W_CheckNumForFolderEndPK3("Textures/", i, start)) == INT16_MAX)
							continue;
					}
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

		if (lump != LUMPERROR)
		{
			break;
		}
	}

	if (lump == LUMPERROR)
	{
		// Use whatever else you can find.
		return W_CheckNumForPatchName(name);
	}

	return lump;
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
		patchLumpNum = W_GetTexPatchLumpNum(patchName);
		// If so, allocate memory for texpatch_t and fill 'er up
		resultPatch = (texpatch_t *)Z_Malloc(sizeof(texpatch_t),PU_STATIC,NULL);
		resultPatch->originx = patchXPos;
		resultPatch->originy = patchYPos;
		resultPatch->lump = LUMPNUM(patchLumpNum);
		resultPatch->wad = WADFILENUM(patchLumpNum);
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
			resultTexture->hash = quickncasehash(newTextureName, 8);
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

void R_ClearTextureNumCache(boolean btell)
{
	if (tidcache)
		Z_Free(tidcache);
	tidcache = NULL;
	if (btell)
		CONS_Debug(DBG_SETUP, "Fun Fact: There are %d textures used in this map.\n", tidcachelen);
	tidcachelen = 0;
}

static void AddTextureToCache(const char *name, UINT32 hash, INT32 id, UINT8 type)
{
	tidcachelen++;
	Z_Realloc(tidcache, tidcachelen * sizeof(*tidcache), PU_STATIC, &tidcache);
	strncpy(tidcache[tidcachelen-1].name, name, 8);
	tidcache[tidcachelen-1].name[8] = '\0';
#ifndef ZDEBUG
	CONS_Debug(DBG_SETUP, "texture #%s: %s\n", sizeu1(tidcachelen), tidcache[tidcachelen-1].name);
#endif
	tidcache[tidcachelen-1].hash = hash;
	tidcache[tidcachelen-1].id = id;
	tidcache[tidcachelen-1].type = type;
}

//
// R_CheckTextureNumForName
//
// Check whether texture is available. Filter out NoTexture indicator.
//
INT32 R_CheckTextureNumForName(const char *name)
{
	INT32 i;
	UINT32 hash;

	// "NoTexture" marker.
	if (name[0] == '-')
		return 0;

	hash = quickncasehash(name, 8);

	for (i = 0; i < tidcachelen; i++)
		if (tidcache[i].hash == hash && !strncasecmp(tidcache[i].name, name, 8))
			return tidcache[i].id;

	// Need to parse the list backwards, so textures loaded more recently are used in lieu of ones loaded earlier
	for (i = numtextures - 1; i >= 0; i--)
		if (textures[i]->hash == hash && !strncasecmp(textures[i]->name, name, 8))
		{
			AddTextureToCache(name, hash, i, textures[i]->type);
			return i;
		}

	return -1;
}

//
// R_CheckTextureNameForNum
//
// because sidedefs use numbers and sometimes you want names
// returns no texture marker if no texture was found
//
const char *R_CheckTextureNameForNum(INT32 num)
{
	if (num > 0 && num < numtextures)
		return textures[num]->name;
	
	return "-";
}

//
// R_TextureNameForNum
//
// calls R_CheckTextureNameForNum and returns REDWALL if result is a no texture marker
//
const char *R_TextureNameForNum(INT32 num)
{
	const char *result = R_CheckTextureNameForNum(num);

	if (strcmp(result, "-") == 0)
		return "REDWALL";

	return result;
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

// Like R_CheckTextureNumForName, but only looks in the flat namespace specifically.
INT32 R_CheckFlatNumForName(const char *name)
{
	INT32 i;
	UINT32 hash;

	// "NoTexture" marker.
	if (name[0] == '-')
		return 0;

	hash = quickncasehash(name, 8);

	for (i = 0; i < tidcachelen; i++)
		if (tidcache[i].type == TEXTURETYPE_FLAT && tidcache[i].hash == hash && !strncasecmp(tidcache[i].name, name, 8))
			return tidcache[i].id;

	for (i = numtextures - 1; i >= 0; i--)
		if (textures[i]->hash == hash && !strncasecmp(textures[i]->name, name, 8) && textures[i]->type == TEXTURETYPE_FLAT)
		{
			AddTextureToCache(name, hash, i, TEXTURETYPE_FLAT);
			return i;
		}

	return -1;
}
