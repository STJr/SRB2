// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_data.c
/// \brief Preparation of data for rendering,generation of lookups, caching, retrieval by name

#include "doomdef.h"
#include "g_game.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_local.h"
#include "m_misc.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_setup.h" // levelflats
#include "v_video.h" // pLocalPalette
#include "dehacked.h"

#if defined (_WIN32) || defined (_WIN32_WCE)
#include <malloc.h> // alloca(sizeof)
#endif

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// Not sure if this is necessary, but it was in w_wad.c, so I'm putting it here too -Shadow Hog
#ifdef _WIN32_WCE
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
static UINT32 **texturecolumnofs; // column offset lookup table for each texture
static UINT8 **texturecache; // graphics data for each generated full-size texture

// texture width is a power of 2, so it can easily repeat along sidedefs using a simple mask
INT32 *texturewidthmask;

fixed_t *textureheight; // needed for texture pegging

INT32 *texturetranslation;

// needed for pre rendering
sprcache_t *spritecachedinfo;

lighttable_t *colormaps;

// for debugging/info purposes
static size_t flatmemory, spritememory, texturememory;

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
static inline void R_DrawColumnInCache(column_t *patch, UINT8 *cache, INT32 originy, INT32 cacheheight)
{
	INT32 count, position;
	UINT8 *source;
	INT32 topdelta, prevdelta = -1;

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
// R_GenerateTexture
//
// Allocate space for full size texture, either single patch or 'composite'
// Build the full textures from patches.
// The texture caching system is a little more hungry of memory, but has
// been simplified for the sake of highcolor, dynamic ligthing, & speed.
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
	int x, x1, x2, i;
	size_t blocksize;
	column_t *patchcol;
	UINT32 *colofs;

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
		realpatch = W_CacheLumpNumPwad(patch->wad, patch->lump, PU_CACHE);

		// Check the patch for holes.
		if (texture->width > SHORT(realpatch->width) || texture->height > SHORT(realpatch->height))
			holey = true;
		colofs = (UINT32 *)realpatch->columnofs;
		for (x = 0; x < texture->width && !holey; x++)
		{
			column_t *col = (column_t *)((UINT8 *)realpatch + LONG(colofs[x]));
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
			blocksize = W_LumpLengthPwad(patch->wad, patch->lump);
			block = Z_Calloc(blocksize, PU_STATIC, // will change tag at end of this function
				&texturecache[texnum]);
			M_Memcpy(block, realpatch, blocksize);
			texturememory += blocksize;

			// use the patch's column lookup
			colofs = (UINT32 *)(void *)(block + 8);
			texturecolumnofs[texnum] = colofs;
			blocktex = block;
			for (x = 0; x < texture->width; x++)
				colofs[x] = LONG(LONG(colofs[x]) + 3);
			goto done;
		}

		// Otherwise, do multipatch format.
	}

	// multi-patch textures (or 'composite')
	texture->holes = false;
	blocksize = (texture->width * 4) + (texture->width * texture->height);
	texturememory += blocksize;
	block = Z_Malloc(blocksize+1, PU_STATIC, &texturecache[texnum]);

	memset(block, 0xF7, blocksize+1); // Transparency hack

	// columns lookup table
	colofs = (UINT32 *)(void *)block;
	texturecolumnofs[texnum] = colofs;

	// texture data after the lookup table
	blocktex = block + (texture->width*4);

	// Composite the columns together.
	for (i = 0, patch = texture->patches; i < texture->patchcount; i++, patch++)
	{
		realpatch = W_CacheLumpNumPwad(patch->wad, patch->lump, PU_CACHE);
		x1 = patch->originx;
		x2 = x1 + SHORT(realpatch->width);

		if (x1 < 0)
			x = 0;
		else
			x = x1;

		if (x2 > texture->width)
			x2 = texture->width;

		for (; x < x2; x++)
		{
			patchcol = (column_t *)((UINT8 *)realpatch + LONG(realpatch->columnofs[x-x1]));

			// generate column ofset lookup
			colofs[x] = LONG((x * texture->height) + (texture->width*4));
			R_DrawColumnInCache(patchcol, block + LONG(colofs[x]), patch->originy, texture->height);
		}
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

	col &= texturewidthmask[tex];
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
int R_CountTexturesInTEXTURESLump(UINT16 wadNum);
void R_ParseTEXTURESLump(UINT16 wadNum, INT32 *index);

//
// R_LoadTextures
// Initializes the texture list with the textures from the world map.
//
#define TX_START "TX_START"
#define TX_END "TX_END"
void R_LoadTextures(void)
{
	INT32 i, k, w;
	UINT16 j;
	UINT16 texstart, texend, texturesLumpPos;
	patch_t *patchlump;
	texpatch_t *patch;
	texture_t *texture;

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
	}

	// Load patches and textures.

	// Get the number of textures to check.
	// NOTE: Make SURE the system does not process
	// the markers.
	// This system will allocate memory for all duplicate/patched textures even if it never uses them,
	// but the alternative is to spend a ton of time checking and re-checking all previous entries just to skip any potentially patched textures.
	for (w = 0, numtextures = 0; w < numwadfiles; w++)
	{
		texstart = W_CheckNumForNamePwad(TX_START, (UINT16)w, 0) + 1;
		texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, 0);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);

		if (texturesLumpPos != INT16_MAX)
		{
			numtextures += R_CountTexturesInTEXTURESLump((UINT16)w);
		}

		// Add all the textures between TX_START and TX_END
		if (texstart != INT16_MAX && texend != INT16_MAX)
		{
			numtextures += (UINT32)(texend - texstart);
		}

		// If no textures found by this point, bomb out
		if (!numtextures && w == (numwadfiles - 1))
		{
			I_Error("No textures detected in any WADs!\n");
		}
	}

	// Allocate memory and initialize to 0 for all the textures we are initialising.
	// There are actually 5 buffers allocated in one for convenience.
	textures = Z_Calloc((numtextures * sizeof(void *)) * 5, PU_STATIC, NULL);

	// Allocate texture column offset table.
	texturecolumnofs = (void *)((UINT8 *)textures + (numtextures * sizeof(void *)));
	// Allocate texture referencing cache.
	texturecache     = (void *)((UINT8 *)textures + ((numtextures * sizeof(void *)) * 2));
	// Allocate texture width mask table.
	texturewidthmask = (void *)((UINT8 *)textures + ((numtextures * sizeof(void *)) * 3));
	// Allocate texture height mask table.
	textureheight    = (void *)((UINT8 *)textures + ((numtextures * sizeof(void *)) * 4));
	// Create translation table for global animation.
	texturetranslation = Z_Malloc((numtextures + 1) * sizeof(*texturetranslation), PU_STATIC, NULL);

	for (i = 0; i < numtextures; i++)
		texturetranslation[i] = i;

	for (i = 0, w = 0; w < numwadfiles; w++)
	{
		// Get the lump numbers for the markers in the WAD, if they exist.
		texstart = W_CheckNumForNamePwad(TX_START, (UINT16)w, 0) + 1;
		texend = W_CheckNumForNamePwad(TX_END, (UINT16)w, 0);
		texturesLumpPos = W_CheckNumForNamePwad("TEXTURES", (UINT16)w, 0);

		if (texturesLumpPos != INT16_MAX)
			R_ParseTEXTURESLump(w,&i);

		if (texstart == INT16_MAX || texend == INT16_MAX)
			continue;

		// Work through each lump between the markers in the WAD.
		for (j = 0; j < (texend - texstart); i++, j++)
		{
			patchlump = W_CacheLumpNumPwad((UINT16)w, texstart + j, PU_CACHE);

			// Then, check the lump directly to see if it's a texture SOC,
			// and if it is, load it using dehacked instead.
			if (strstr((const char *)patchlump, "TEXTURE"))
			{
				CONS_Alert(CONS_WARNING, "%s is a Texture SOC.\n", W_CheckNameForNumPwad((UINT16)w,texstart+j));
				Z_Unlock(patchlump);
				DEH_LoadDehackedLumpPwad((UINT16)w, texstart + j);
			}
			else
			{
				UINT16 patchcount = 1;
				//CONS_Printf("\n\"%s\" is a single patch, dimensions %d x %d",W_CheckNameForNumPwad((UINT16)w,texstart+j),patchlump->width, patchlump->height);
				if (SHORT(patchlump->width) == 64
				&& SHORT(patchlump->height) == 64)
				{ // 64x64 patch
					const column_t *column;
					for (k = 0; k < SHORT(patchlump->width); k++)
					{ // Find use of transparency.
						column = (const column_t *)((const UINT8 *)patchlump + LONG(patchlump->columnofs[k]));
						if (column->length != SHORT(patchlump->height))
							break;
					}
					if (k == SHORT(patchlump->width))
						patchcount = 2; // No transparency? 64x128 texture.
				}
				texture = textures[i] = Z_Calloc(sizeof(texture_t) + (sizeof(texpatch_t) * patchcount), PU_STATIC, NULL);

				// Set texture properties.
				M_Memcpy(texture->name, W_CheckNameForNumPwad((UINT16)w, texstart + j), sizeof(texture->name));
				texture->width = SHORT(patchlump->width);
				texture->height = SHORT(patchlump->height)*patchcount;
				texture->patchcount = patchcount;
				texture->holes = false;

				// Allocate information for the texture's patches.
				for (k = 0; k < patchcount; k++)
				{
					patch = &texture->patches[k];

					patch->originx = 0;
					patch->originy = (INT16)(k*patchlump->height);
					patch->wad = (UINT16)w;
					patch->lump = texstart + j;
				}

				Z_Unlock(patchlump);

				k = 1;
				while (k << 1 <= texture->width)
					k <<= 1;

				texturewidthmask[i] = k - 1;
				textureheight[i] = texture->height << FRACBITS;
			}
		}
	}
}

static texpatch_t *R_ParsePatch(boolean actuallyLoadPatch)
{
	char *texturesToken;
	size_t texturesTokenLength;
	char *endPos;
	char *patchName = NULL;
	INT16 patchXPos;
	INT16 patchYPos;
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
int R_CountTexturesInTEXTURESLump(UINT16 wadNum)
{
	char *texturesLump;
	size_t texturesLumpLength;
	char *texturesText;
	UINT32 numTexturesInLump = 0;
	char *texturesToken;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	texturesLump = (char *)W_CacheLumpNumPwad(wadNum,W_CheckNumForNamePwad("TEXTURES", wadNum, 0),PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (texturesLump == NULL) return 0;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	texturesLumpLength = W_LumpLengthPwad(wadNum,W_CheckNumForNamePwad("TEXTURES",wadNum,0));
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
		if (stricmp(texturesToken, "WALLTEXTURE")==0)
		{
			numTexturesInLump++;
			Z_Free(texturesToken);
			R_ParseTexture(false);
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);

	return numTexturesInLump;
}

// Parses the TEXTURES lump... for real, this time.
void R_ParseTEXTURESLump(UINT16 wadNum, INT32 *texindex)
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
	texturesLump = (char *)W_CacheLumpNumPwad(wadNum,W_CheckNumForNamePwad("TEXTURES", wadNum, 0),PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (texturesLump == NULL) return;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	texturesLumpLength = W_LumpLengthPwad(wadNum,W_CheckNumForNamePwad("TEXTURES",wadNum,0));
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
		if (stricmp(texturesToken, "WALLTEXTURE")==0)
		{
			Z_Free(texturesToken);
			// Get the new texture
			newTexture = R_ParseTexture(true);
			// Store the new texture
			textures[*texindex] = newTexture;
			texturewidthmask[*texindex] = newTexture->width - 1;
			textureheight[*texindex] = newTexture->height << FRACBITS;
			// Increment i back in R_LoadTextures()
			(*texindex)++;
		}
		else
		{
			I_Error("Error parsing TEXTURES lump: Expected \"WALLTEXTURE\", got \"%s\"",texturesToken);
		}
		texturesToken = M_GetToken(NULL);
	}
	Z_Free(texturesToken);
	Z_Free((void *)texturesText);
}

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

static lumplist_t *colormaplumps = NULL; ///\todo free leak
static size_t numcolormaplumps = 0;

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

// 12/14/14 -- only take flats in F_START/F_END
lumpnum_t R_GetFlatNumForName(const char *name)
{
	lumpnum_t lump = W_CheckNumForNameInBlock(name, "F_START", "F_END");
	if (lump == LUMPERROR)
		lump = W_CheckNumForNameInBlock(name, "FF_START", "FF_END"); // deutex, some other old things
	if (lump == LUMPERROR)
	{
		if (strcmp(name, SKYFLATNAME))
			CONS_Debug(DBG_SETUP, "R_GetFlatNumForName: Could not find flat %.8s\n", name);
		lump = W_CheckNumForName("REDFLR");
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
// R_InitColormaps
//
static void R_InitColormaps(void)
{
	lumpnum_t lump;

	// Load in the light tables
	lump = W_GetNumForName("COLORMAP");
	colormaps = Z_MallocAlign(W_LumpLength (lump), PU_STATIC, NULL, 8);
	W_ReadLump(lump, colormaps);

	// Init Boom colormaps.
	R_ClearColormaps();
	R_InitExtraColormaps();
}

void R_ReInitColormaps(UINT16 num)
{
	char colormap[9] = "COLORMAP";
	lumpnum_t lump;

	if (num > 0 && num <= 10000)
		snprintf(colormap, 8, "CLM%04u", num-1);

	// Load in the light tables, now 64k aligned for smokie...
	lump = W_GetNumForName(colormap);
	if (lump == LUMPERROR)
		lump = W_GetNumForName("COLORMAP");
	W_ReadLump(lump, colormaps);

	// Init Boom colormaps.
	R_ClearColormaps();
}

static lumpnum_t foundcolormaps[MAXCOLORMAPS];

static char colormapFixingArray[MAXCOLORMAPS][3][9];
static size_t carrayindex;

//
// R_ClearColormaps
//
// Clears out extra colormaps between levels.
//
void R_ClearColormaps(void)
{
	size_t i;

	num_extra_colormaps = 0;

	carrayindex = 0;

	for (i = 0; i < MAXCOLORMAPS; i++)
		foundcolormaps[i] = LUMPERROR;

	memset(extra_colormaps, 0, sizeof (extra_colormaps));
}

INT32 R_ColormapNumForName(char *name)
{
	lumpnum_t lump, i;

	if (num_extra_colormaps == MAXCOLORMAPS)
		I_Error("R_ColormapNumForName: Too many colormaps! the limit is %d\n", MAXCOLORMAPS);

	lump = R_CheckNumForNameList(name, colormaplumps, numcolormaplumps);
	if (lump == LUMPERROR)
		I_Error("R_ColormapNumForName: Cannot find colormap lump %.8s\n", name);

	for (i = 0; i < num_extra_colormaps; i++)
		if (lump == foundcolormaps[i])
			return i;

	foundcolormaps[num_extra_colormaps] = lump;

	// aligned on 8 bit for asm code
	extra_colormaps[num_extra_colormaps].colormap = Z_MallocAlign(W_LumpLength(lump), PU_LEVEL, NULL, 16);
	W_ReadLump(lump, extra_colormaps[num_extra_colormaps].colormap);

	// We set all params of the colormap to normal because there
	// is no real way to tell how GL should handle a colormap lump anyway..
	extra_colormaps[num_extra_colormaps].maskcolor = 0xffff;
	extra_colormaps[num_extra_colormaps].fadecolor = 0x0;
	extra_colormaps[num_extra_colormaps].maskamt = 0x0;
	extra_colormaps[num_extra_colormaps].fadestart = 0;
	extra_colormaps[num_extra_colormaps].fadeend = 31;
	extra_colormaps[num_extra_colormaps].fog = 0;

	num_extra_colormaps++;
	return (INT32)num_extra_colormaps - 1;
}

//
// R_CreateColormap
//
// This is a more GL friendly way of doing colormaps: Specify colormap
// data in a special linedef's texture areas and use that to generate
// custom colormaps at runtime. NOTE: For GL mode, we only need to color
// data and not the colormap data.
//
static double deltas[256][3], map[256][3];

static UINT8 NearestColor(UINT8 r, UINT8 g, UINT8 b);
static int RoundUp(double number);

INT32 R_CreateColormap(char *p1, char *p2, char *p3)
{
	double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
	double r, g, b, cbrightness, maskamt = 0, othermask = 0;
	int mask, fog = 0;
	size_t mapnum = num_extra_colormaps;
	size_t i;
	UINT32 cr, cg, cb, maskcolor, fadecolor;
	UINT32 fadestart = 0, fadeend = 31, fadedist = 31;

#define HEX2INT(x) (UINT32)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
	if (p1[0] == '#')
	{
		cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
		cmaskr = cr;
		cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
		cmaskg = cg;
		cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));
		cmaskb = cb;
		// Create a rough approximation of the color (a 16 bit color)
		maskcolor = ((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11);
		if (p1[7] >= 'a' && p1[7] <= 'z')
			mask = (p1[7] - 'a');
		else if (p1[7] >= 'A' && p1[7] <= 'Z')
			mask = (p1[7] - 'A');
		else
			mask = 24;

		maskamt = (double)(mask/24.0l);

		othermask = 1 - maskamt;
		maskamt /= 0xff;
		cmaskr *= maskamt;
		cmaskg *= maskamt;
		cmaskb *= maskamt;
	}
	else
	{
		cmaskr = cmaskg = cmaskb = 0xff;
		maskamt = 0;
		maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
	}

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)
	if (p2[0] == '#')
	{
		// Get parameters like fadestart, fadeend, and the fogflag
		fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
		fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
		if (fadestart > 30)
			fadestart = 0;
		if (fadeend > 31 || fadeend < 1)
			fadeend = 31;
		fadedist = fadeend - fadestart;
		fog = NUMFROMCHAR(p2[1]);
	}
#undef getnum

	if (p3[0] == '#')
	{
		cdestr = cr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
		cdestg = cg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
		cdestb = cb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));
		fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
	}
	else
		cdestr = cdestg = cdestb = fadecolor = 0;
#undef HEX2INT

	for (i = 0; i < num_extra_colormaps; i++)
	{
		if (foundcolormaps[i] != LUMPERROR)
			continue;
		if (maskcolor == extra_colormaps[i].maskcolor
			&& fadecolor == extra_colormaps[i].fadecolor
			&& (float)maskamt == (float)extra_colormaps[i].maskamt
			&& fadestart == extra_colormaps[i].fadestart
			&& fadeend == extra_colormaps[i].fadeend
			&& fog == extra_colormaps[i].fog)
		{
			return (INT32)i;
		}
	}

	if (num_extra_colormaps == MAXCOLORMAPS)
		I_Error("R_CreateColormap: Too many colormaps! the limit is %d\n", MAXCOLORMAPS);

	strncpy(colormapFixingArray[num_extra_colormaps][0], p1, 8);
	strncpy(colormapFixingArray[num_extra_colormaps][1], p2, 8);
	strncpy(colormapFixingArray[num_extra_colormaps][2], p3, 8);

	num_extra_colormaps++;

	if (rendermode == render_soft)
	{
		for (i = 0; i < 256; i++)
		{
			r = pLocalPalette[i].s.red;
			g = pLocalPalette[i].s.green;
			b = pLocalPalette[i].s.blue;
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
	}

	foundcolormaps[mapnum] = LUMPERROR;

	// aligned on 8 bit for asm code
	extra_colormaps[mapnum].colormap = NULL;
	extra_colormaps[mapnum].maskcolor = (UINT16)maskcolor;
	extra_colormaps[mapnum].fadecolor = (UINT16)fadecolor;
	extra_colormaps[mapnum].maskamt = maskamt;
	extra_colormaps[mapnum].fadestart = (UINT16)fadestart;
	extra_colormaps[mapnum].fadeend = (UINT16)fadeend;
	extra_colormaps[mapnum].fog = fog;

	return (INT32)mapnum;
}

void R_MakeColormaps(void)
{
	size_t i;

	carrayindex = num_extra_colormaps;
	num_extra_colormaps = 0;

	for (i = 0; i < carrayindex; i++)
		R_CreateColormap2(colormapFixingArray[i][0], colormapFixingArray[i][1],
			colormapFixingArray[i][2]);
}

void R_CreateColormap2(char *p1, char *p2, char *p3)
{
	double cmaskr, cmaskg, cmaskb, cdestr, cdestg, cdestb;
	double r, g, b, cbrightness;
	double maskamt = 0, othermask = 0;
	int mask, p, fog = 0;
	size_t mapnum = num_extra_colormaps;
	size_t i;
	char *colormap_p;
	UINT32 cr, cg, cb, maskcolor, fadecolor;
	UINT32 fadestart = 0, fadeend = 31, fadedist = 31;

#define HEX2INT(x) (UINT32)(x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
	if (p1[0] == '#')
	{
		cr = ((HEX2INT(p1[1]) * 16) + HEX2INT(p1[2]));
		cmaskr = cr;
		cg = ((HEX2INT(p1[3]) * 16) + HEX2INT(p1[4]));
		cmaskg = cg;
		cb = ((HEX2INT(p1[5]) * 16) + HEX2INT(p1[6]));
		cmaskb = cb;
		// Create a rough approximation of the color (a 16 bit color)
		maskcolor = ((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11);
		if (p1[7] >= 'a' && p1[7] <= 'z')
			mask = (p1[7] - 'a');
		else if (p1[7] >= 'A' && p1[7] <= 'Z')
			mask = (p1[7] - 'A');
		else
			mask = 24;

		maskamt = (double)(mask/24.0l);

		othermask = 1 - maskamt;
		maskamt /= 0xff;
		cmaskr *= maskamt;
		cmaskg *= maskamt;
		cmaskb *= maskamt;
	}
	else
	{
		cmaskr = cmaskg = cmaskb = 0xff;
		maskamt = 0;
		maskcolor = ((0xff) >> 3) + (((0xff) >> 2) << 5) + (((0xff) >> 3) << 11);
	}

#define NUMFROMCHAR(c) (c >= '0' && c <= '9' ? c - '0' : 0)
	if (p2[0] == '#')
	{
		// Get parameters like fadestart, fadeend, and the fogflag
		fadestart = NUMFROMCHAR(p2[3]) + (NUMFROMCHAR(p2[2]) * 10);
		fadeend = NUMFROMCHAR(p2[5]) + (NUMFROMCHAR(p2[4]) * 10);
		if (fadestart > 30)
			fadestart = 0;
		if (fadeend > 31 || fadeend < 1)
			fadeend = 31;
		fadedist = fadeend - fadestart;
		fog = NUMFROMCHAR(p2[1]);
	}
#undef getnum

	if (p3[0] == '#')
	{
		cdestr = cr = ((HEX2INT(p3[1]) * 16) + HEX2INT(p3[2]));
		cdestg = cg = ((HEX2INT(p3[3]) * 16) + HEX2INT(p3[4]));
		cdestb = cb = ((HEX2INT(p3[5]) * 16) + HEX2INT(p3[6]));
		fadecolor = (((cb) >> 3) + (((cg) >> 2) << 5) + (((cr) >> 3) << 11));
	}
	else
		cdestr = cdestg = cdestb = fadecolor = 0;
#undef HEX2INT

	for (i = 0; i < num_extra_colormaps; i++)
	{
		if (foundcolormaps[i] != LUMPERROR)
			continue;
		if (maskcolor == extra_colormaps[i].maskcolor
			&& fadecolor == extra_colormaps[i].fadecolor
			&& (float)maskamt == (float)extra_colormaps[i].maskamt
			&& fadestart == extra_colormaps[i].fadestart
			&& fadeend == extra_colormaps[i].fadeend
			&& fog == extra_colormaps[i].fog)
		{
			return;
		}
	}

	if (num_extra_colormaps == MAXCOLORMAPS)
		I_Error("R_CreateColormap: Too many colormaps! the limit is %d\n", MAXCOLORMAPS);

	num_extra_colormaps++;

	if (rendermode == render_soft)
	{
		for (i = 0; i < 256; i++)
		{
			r = pLocalPalette[i].s.red;
			g = pLocalPalette[i].s.green;
			b = pLocalPalette[i].s.blue;
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
	}

	foundcolormaps[mapnum] = LUMPERROR;

	// aligned on 8 bit for asm code
	extra_colormaps[mapnum].colormap = NULL;
	extra_colormaps[mapnum].maskcolor = (UINT16)maskcolor;
	extra_colormaps[mapnum].fadecolor = (UINT16)fadecolor;
	extra_colormaps[mapnum].maskamt = maskamt;
	extra_colormaps[mapnum].fadestart = (UINT16)fadestart;
	extra_colormaps[mapnum].fadeend = (UINT16)fadeend;
	extra_colormaps[mapnum].fog = fog;

#define ABS2(x) ((x) < 0 ? -(x) : (x))
	if (rendermode == render_soft)
	{
		colormap_p = Z_MallocAlign((256 * 34) + 10, PU_LEVEL, NULL, 8);
		extra_colormaps[mapnum].colormap = (UINT8 *)colormap_p;

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
			}
		}
	}
#undef ABS2

	return;
}

// Thanks to quake2 source!
// utils3/qdata/images.c
static UINT8 NearestColor(UINT8 r, UINT8 g, UINT8 b)
{
	int dr, dg, db;
	int distortion, bestdistortion = 256 * 256 * 4, bestcolor = 0, i;

	for (i = 0; i < 256; i++)
	{
		dr = r - pLocalPalette[i].s.red;
		dg = g - pLocalPalette[i].s.green;
		db = b - pLocalPalette[i].s.blue;
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

const char *R_ColormapNameForNum(INT32 num)
{
	if (num == -1)
		return "NONE";

	if (num < 0 || num > MAXCOLORMAPS)
		I_Error("R_ColormapNameForNum: num %d is invalid!\n", num);

	if (foundcolormaps[num] == LUMPERROR)
		return "INLEVEL";

	return W_CheckNameForNum(foundcolormaps[num]);
}


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

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
		if (th->function.acp1 == (actionf_p1)P_MobjThinker)
			spritepresent[((mobj_t *)th)->sprite] = 1;

	spritememory = 0;
	for (i = 0; i < numsprites; i++)
	{
		if (!spritepresent[i])
			continue;

		for (j = 0; j < sprites[i].numframes; j++)
		{
			sf = &sprites[i].spriteframes[j];
			for (k = 0; k < 8; k++)
			{
				// see R_InitSprites for more about lumppat,lumpid
				lump = sf->lumppat[k];
				if (devparm)
					spritememory += W_LumpLength(lump);
				W_CachePatchNum(lump, PU_CACHE);
			}
		}
	}
	free(spritepresent);

	// FIXME: this is no longer correct with OpenGL render mode
	CONS_Debug(DBG_SETUP, "Precache level done:\n"
			"flatmemory:    %s k\n"
			"texturememory: %s k\n"
			"spritememory:  %s k\n", sizeu1(flatmemory>>10), sizeu2(texturememory>>10), sizeu3(spritememory>>10));
}
