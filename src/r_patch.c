// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005-2009 by Andrey "entryway" Budko.
// Copyright (C) 2018-2020 by Jaime "Lactozilla" Passos.
// Copyright (C) 2019-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patch.c
/// \brief Patch caching and generation.

#include "byteptr.h"
#include "dehacked.h"
#include "i_video.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_patch.h"
#include "r_things.h"
#include "z_zone.h"
#include "w_wad.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include "png.h"
#ifndef PNG_READ_SUPPORTED
#undef HAVE_PNG
#endif
#endif

#ifdef ROTSPRITE
fixed_t rollcosang[ROTANGLES];
fixed_t rollsinang[ROTANGLES];
#endif

static unsigned char imgbuf[1<<26];

// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current render mode. For the software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, and
// the heap memory cache then acts as a 'level 2' cache just after the
// graphics card memory.

// If patches need to be cached only once, references to those patches
// can be obtained with W_GetPatchPointer and relatives. By returning a
// double pointer that can be dereferenced, the addresses of the patches
// can be swapped out to point to another patch instead. The main use case
// for this is renderer switching.

static patchreference_t *patchlist_head = NULL;
static patchreference_t *patchlist_tail = NULL;

// Find patch in list
static patchreference_t *FindPatchReference(UINT16 wad, UINT16 lump)
{
	patchreference_t *patch = patchlist_head;

	CONS_Debug(DBG_RENDER, "FindPatchReference: searching for %s\n", W_CheckNameForNumPwad(wad, lump));

	while (patch)
	{
		patchreference_t *prev = NULL, *next = NULL;

		CONS_Debug(DBG_RENDER, "%s\n", W_CheckNameForNumPwad(patch->wad, patch->lump));

		if (patch->wad == wad && patch->lump == lump)
		{
			CONS_Debug(DBG_RENDER, "found\n");
			return patch;
		}

		// Lazy memory deallocation.
		// References aren't removed until an iteration
		// knows the pointer isn't valid anymore.
		next = patch->next;
		if (patch->ptr == NULL)
		{
			CONS_Debug(DBG_RENDER, "freeing this patch\n");
			prev = patch->prev;
			if (prev)
				prev->next = next;
			Z_Free(patch);
		}
		patch = next;
	}

	return NULL;
}

// Insert patch in list
static patchreference_t *InsertPatchReference(UINT16 wad, UINT16 lump, void *ptr, INT32 tag)
{
	patchreference_t *found = FindPatchReference(wad, lump);
	if (found)
		return found;

	if (!patchlist_head)
	{
		patchlist_head = Z_Calloc(sizeof(patchreference_t), PU_STATIC, NULL);
		patchlist_head->wad = wad;
		patchlist_head->lump = lump;
		patchlist_head->ptr = ptr;
		patchlist_head->tag = tag;
		patchlist_tail = patchlist_head;
		return patchlist_head;
	}
	else
	{
		patchreference_t *list = Z_Calloc(sizeof(patchreference_t), PU_STATIC, NULL);
		list->wad = wad;
		list->lump = lump;
		list->ptr = ptr;
		list->tag = tag;
		list->prev = patchlist_tail; // set the previous item to the tail
		patchlist_tail->next = list; // set the tail's next item to the new item
		patchlist_tail = list; // set the tail to the new item
		return list;
	}
}

//
// Cache a patch into heap memory, convert the patch format as necessary
//

#define GetPatchCache(wadnum, lumpnum, mode) \
	M_AATreeGet(M_AATreeGet(wadfiles[wadnum]->patchcache.renderer, (mode - 1)), lumpnum)

#define SetPatchCache(wadnum, lumpnum, mode, ptr) \
	M_AATreeSet(M_AATreeGet(wadfiles[wadnum]->patchcache.renderer, (mode - 1)), lumpnum, ptr)

#define UpdatePatchCache(wadnum, lumpnum, mode) \
	wadfiles[wadnum]->patchcache.current[lumpnum] = GetPatchCache(wadnum, lumpnum, mode)

void *R_CacheSoftwarePatch(UINT16 wad, UINT16 lump, INT32 tag, boolean store)
{
	void *cache = GetPatchCache(wad, lump, render_soft);
	if (!GetPatchCache(wad, lump, render_soft))
	{
		size_t len = W_LumpLengthPwad(wad, lump);
		void *ptr, *lumpdata;
#ifndef NO_PNG_LUMPS
		void *converted = NULL;
#endif

		ptr = Z_Malloc(len, tag, &cache);
		lumpdata = Z_Malloc(len, tag, NULL);

		// read the lump in full
		W_ReadLumpHeaderPwad(wad, lump, lumpdata, 0, 0);

#ifndef NO_PNG_LUMPS
		// lump is a png so convert it
		if (R_IsLumpPNG((UINT8 *)lumpdata, len))
		{
			size_t newlen;
			converted = R_PNGToPatch((UINT8 *)lumpdata, len, &newlen);
			ptr = Z_Realloc(ptr, newlen, tag, &cache);
			M_Memcpy(ptr, converted, newlen);
			Z_Free(converted);
		}
		else // just copy it into the patch cache
#endif
			M_Memcpy(ptr, lumpdata, len);

		// insert into list
		if (store)
			InsertPatchReference(wad, lump, ptr, tag);

		SetPatchCache(wad, lump, render_soft, ptr);
		UpdatePatchCache(wad, lump, render_soft);

		return ptr;
	}
	else
		Z_ChangeTag(cache, tag);

	return cache;
}

#ifdef HWRENDER
void *R_CacheGLPatch(UINT16 wad, UINT16 lump, INT32 tag, boolean store)
{
	GLPatch_t *grPatch = HWR_GetCachedGLPatchPwad(wad, lump);

	if (grPatch->mipmap->grInfo.data)
	{
		if (tag == PU_CACHE)
			tag = PU_HWRCACHE;
		Z_ChangeTag(grPatch->mipmap->grInfo.data, tag);
	}
	else
	{
		patch_t *ptr = NULL;

		// Only load the patch if we haven't initialised the grPatch yet
		if (grPatch->mipmap->width == 0)
			ptr = W_CacheLumpNumPwad(grPatch->wadnum, grPatch->lumpnum, PU_STATIC);

		// Run HWR_MakePatch in all cases, to recalculate some things
		HWR_MakePatch(ptr, grPatch, grPatch->mipmap, false);
		Z_Free(ptr);

		// insert into list
		if (store)
			InsertPatchReference(wad, lump, (void *)grPatch, tag);

		SetPatchCache(wad, lump, render_opengl, (void *)grPatch);
		UpdatePatchCache(wad, lump, render_opengl);
	}

	// return GLPatch_t, which can be casted to (patch_t *) with valid patch header info
	return (void *)grPatch;
}
#endif

//
// Update patch references to point to the current renderer's patches.
//
void R_UpdatePatchReferences(void)
{
	patchreference_t *patch = patchlist_head;
	while (patch)
	{
#ifdef HWRENDER
		if (rendermode == render_opengl)
			R_CacheGLPatch(patch->wad, patch->lump, patch->tag, false);
		else
#endif
			R_CacheSoftwarePatch(patch->wad, patch->lump, patch->tag, false);
		UpdatePatchCache(patch->wad, patch->lump, rendermode);
		patch = patch->next;
	}
}

//
// Free the patch reference list.
//
void R_FreePatchReferences(void)
{
	patchreference_t *patch = patchlist_head;
	while (patch)
	{
		patchreference_t *next = patch->next;
		Z_Free(patch);
		patch = next;
	}
}

//
// R_CheckIfPatch
//
// Returns true if the lump is a valid patch.
//
boolean R_CheckIfPatch(lumpnum_t lump)
{
	size_t size;
	INT16 width, height;
	patch_t *patch;
	boolean result;

	size = W_LumpLength(lump);

	// minimum length of a valid Doom patch
	if (size < 13)
		return false;

	patch = (patch_t *)W_CacheLumpNum(lump, PU_STATIC);

	width = SHORT(patch->width);
	height = SHORT(patch->height);

	result = (height > 0 && height <= 16384 && width > 0 && width <= 16384 && width < (INT16)(size / 4));

	if (result)
	{
		// The dimensions seem like they might be valid for a patch, so
		// check the column directory for extra security. All columns
		// must begin after the column directory, and none of them must
		// point past the end of the patch.
		INT16 x;

		for (x = 0; x < width; x++)
		{
			UINT32 ofs = LONG(patch->columnofs[x]);

			// Need one byte for an empty column (but there's patches that don't know that!)
			if (ofs < (UINT32)width * 4 + 8 || ofs >= (UINT32)size)
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

//
// R_TextureToFlat
//
// Convert a texture to a flat.
//
void R_TextureToFlat(size_t tex, UINT8 *flat)
{
	texture_t *texture = textures[tex];

	fixed_t col, ofs;
	column_t *column;
	UINT8 *desttop, *dest, *deststop;
	UINT8 *source;

	// yea
	R_CheckTextureCache(tex);

	desttop = flat;
	deststop = desttop + (texture->width * texture->height);

	for (col = 0; col < texture->width; col++, desttop++)
	{
		// no post_t info
		if (!texture->holes)
		{
			column = (column_t *)(R_GetColumn(tex, col));
			source = (UINT8 *)(column);
			dest = desttop;
			for (ofs = 0; dest < deststop && ofs < texture->height; ofs++)
			{
				if (source[ofs] != TRANSPARENTPIXEL)
					*dest = source[ofs];
				dest += texture->width;
			}
		}
		else
		{
			INT32 topdelta, prevdelta = -1;
			column = (column_t *)((UINT8 *)R_GetColumn(tex, col) - 3);
			while (column->topdelta != 0xff)
			{
				topdelta = column->topdelta;
				if (topdelta <= prevdelta)
					topdelta += prevdelta;
				prevdelta = topdelta;

				dest = desttop + (topdelta * texture->width);
				source = (UINT8 *)column + 3;
				for (ofs = 0; dest < deststop && ofs < column->length; ofs++)
				{
					if (source[ofs] != TRANSPARENTPIXEL)
						*dest = source[ofs];
					dest += texture->width;
				}
				column = (column_t *)((UINT8 *)column + column->length + 4);
			}
		}
	}
}

//
// R_PatchToFlat
//
// Convert a patch to a flat.
//
void R_PatchToFlat(patch_t *patch, UINT8 *flat)
{
	fixed_t col, ofs;
	column_t *column;
	UINT8 *desttop, *dest, *deststop;
	UINT8 *source;

	desttop = flat;
	deststop = desttop + (SHORT(patch->width) * SHORT(patch->height));

	for (col = 0; col < SHORT(patch->width); col++, desttop++)
	{
		INT32 topdelta, prevdelta = -1;
		column = (column_t *)((UINT8 *)patch + LONG(patch->columnofs[col]));

		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;

			dest = desttop + (topdelta * SHORT(patch->width));
			source = (UINT8 *)(column) + 3;
			for (ofs = 0; dest < deststop && ofs < column->length; ofs++)
			{
				*dest = source[ofs];
				dest += SHORT(patch->width);
			}
			column = (column_t *)((UINT8 *)column + column->length + 4);
		}
	}
}

//
// R_PatchToMaskedFlat
//
// Convert a patch to a masked flat.
// Now, what is a "masked" flat anyway?
// It means the flat uses two bytes to store image data.
// The upper byte is used to store the transparent pixel,
// and the lower byte stores a palette index.
//
void R_PatchToMaskedFlat(patch_t *patch, UINT16 *raw, boolean flip)
{
	fixed_t col, ofs;
	column_t *column;
	UINT16 *desttop, *dest, *deststop;
	UINT8 *source;

	desttop = raw;
	deststop = desttop + (SHORT(patch->width) * SHORT(patch->height));

	for (col = 0; col < SHORT(patch->width); col++, desttop++)
	{
		INT32 topdelta, prevdelta = -1;
		column = (column_t *)((UINT8 *)patch + LONG(patch->columnofs[flip ? (patch->width-1-col) : col]));
		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			dest = desttop + (topdelta * SHORT(patch->width));
			source = (UINT8 *)(column) + 3;
			for (ofs = 0; dest < deststop && ofs < column->length; ofs++)
			{
				*dest = source[ofs];
				dest += SHORT(patch->width);
			}
			column = (column_t *)((UINT8 *)column + column->length + 4);
		}
	}
}

//
// R_FlatToPatch
//
// Convert a flat to a patch.
//
patch_t *R_FlatToPatch(UINT8 *raw, UINT16 width, UINT16 height, UINT16 leftoffset, UINT16 topoffset, size_t *destsize, boolean transparency)
{
	UINT32 x, y;
	UINT8 *img;
	UINT8 *imgptr = imgbuf;
	UINT8 *colpointers, *startofspan;
	size_t size = 0;

	if (!raw)
		return NULL;

	// Write image size and offset
	WRITEINT16(imgptr, width);
	WRITEINT16(imgptr, height);
	WRITEINT16(imgptr, leftoffset);
	WRITEINT16(imgptr, topoffset);

	// Leave placeholder to column pointers
	colpointers = imgptr;
	imgptr += width*4;

	// Write columns
	for (x = 0; x < width; x++)
	{
		int lastStartY = 0;
		int spanSize = 0;
		startofspan = NULL;

		// Write column pointer
		WRITEINT32(colpointers, imgptr - imgbuf);

		// Write pixels
		for (y = 0; y < height; y++)
		{
			UINT8 paletteIndex = raw[((y * width) + x)];
			boolean opaque = transparency ? (paletteIndex != TRANSPARENTPIXEL) : true;

			// End span if we have a transparent pixel
			if (!opaque)
			{
				if (startofspan)
					WRITEUINT8(imgptr, 0);
				startofspan = NULL;
				continue;
			}

			// Start new column if we need to
			if (!startofspan || spanSize == 255)
			{
				int writeY = y;

				// If we reached the span size limit, finish the previous span
				if (startofspan)
					WRITEUINT8(imgptr, 0);

				if (y > 254)
				{
					// Make sure we're aligned to 254
					if (lastStartY < 254)
					{
						WRITEUINT8(imgptr, 254);
						WRITEUINT8(imgptr, 0);
						imgptr += 2;
						lastStartY = 254;
					}

					// Write stopgap empty spans if needed
					writeY = y - lastStartY;

					while (writeY > 254)
					{
						WRITEUINT8(imgptr, 254);
						WRITEUINT8(imgptr, 0);
						imgptr += 2;
						writeY -= 254;
					}
				}

				startofspan = imgptr;
				WRITEUINT8(imgptr, writeY);
				imgptr += 2;
				spanSize = 0;

				lastStartY = y;
			}

			// Write the pixel
			WRITEUINT8(imgptr, paletteIndex);
			spanSize++;
			startofspan[1] = spanSize;
		}

		if (startofspan)
			WRITEUINT8(imgptr, 0);

		WRITEUINT8(imgptr, 0xFF);
	}

	size = imgptr-imgbuf;
	img = Z_Malloc(size, PU_STATIC, NULL);
	memcpy(img, imgbuf, size);

	Z_Free(raw);

	if (destsize != NULL)
		*destsize = size;
	return (patch_t *)img;
}

//
// R_MaskedFlatToPatch
//
// Convert a masked flat to a patch.
// Explanation of "masked" flats in R_PatchToMaskedFlat.
//
patch_t *R_MaskedFlatToPatch(UINT16 *raw, UINT16 width, UINT16 height, UINT16 leftoffset, UINT16 topoffset, size_t *destsize)
{
	UINT32 x, y;
	UINT8 *img;
	UINT8 *imgptr = imgbuf;
	UINT8 *colpointers, *startofspan;
	size_t size = 0;

	if (!raw)
		return NULL;

	// Write image size and offset
	WRITEINT16(imgptr, width);
	WRITEINT16(imgptr, height);
	WRITEINT16(imgptr, leftoffset);
	WRITEINT16(imgptr, topoffset);

	// Leave placeholder to column pointers
	colpointers = imgptr;
	imgptr += width*4;

	// Write columns
	for (x = 0; x < width; x++)
	{
		int lastStartY = 0;
		int spanSize = 0;
		startofspan = NULL;

		// Write column pointer
		WRITEINT32(colpointers, imgptr - imgbuf);

		// Write pixels
		for (y = 0; y < height; y++)
		{
			UINT16 pixel = raw[((y * width) + x)];
			UINT8 paletteIndex = (pixel & 0xFF);
			UINT8 opaque = (pixel != 0xFF00); // If 1, we have a pixel

			// End span if we have a transparent pixel
			if (!opaque)
			{
				if (startofspan)
					WRITEUINT8(imgptr, 0);
				startofspan = NULL;
				continue;
			}

			// Start new column if we need to
			if (!startofspan || spanSize == 255)
			{
				int writeY = y;

				// If we reached the span size limit, finish the previous span
				if (startofspan)
					WRITEUINT8(imgptr, 0);

				if (y > 254)
				{
					// Make sure we're aligned to 254
					if (lastStartY < 254)
					{
						WRITEUINT8(imgptr, 254);
						WRITEUINT8(imgptr, 0);
						imgptr += 2;
						lastStartY = 254;
					}

					// Write stopgap empty spans if needed
					writeY = y - lastStartY;

					while (writeY > 254)
					{
						WRITEUINT8(imgptr, 254);
						WRITEUINT8(imgptr, 0);
						imgptr += 2;
						writeY -= 254;
					}
				}

				startofspan = imgptr;
				WRITEUINT8(imgptr, writeY);
				imgptr += 2;
				spanSize = 0;

				lastStartY = y;
			}

			// Write the pixel
			WRITEUINT8(imgptr, paletteIndex);
			spanSize++;
			startofspan[1] = spanSize;
		}

		if (startofspan)
			WRITEUINT8(imgptr, 0);

		WRITEUINT8(imgptr, 0xFF);
	}

	size = imgptr-imgbuf;
	img = Z_Malloc(size, PU_STATIC, NULL);
	memcpy(img, imgbuf, size);

	if (destsize != NULL)
		*destsize = size;
	return (patch_t *)img;
}

//
// R_IsLumpPNG
//
// Returns true if the lump is a valid PNG.
//
boolean R_IsLumpPNG(const UINT8 *d, size_t s)
{
	if (s < 67) // http://garethrees.org/2007/11/14/pngcrush/
		return false;
	// Check for PNG file signature using memcmp
	// As it may be faster on CPUs with slow unaligned memory access
	// Ref: http://www.libpng.org/pub/png/spec/1.2/PNG-Rationale.html#R.PNG-file-signature
	return (memcmp(&d[0], "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", 8) == 0);
}

#ifndef NO_PNG_LUMPS
#ifdef HAVE_PNG

/*#if PNG_LIBPNG_VER_DLLNUM < 14
typedef PNG_CONST png_byte *png_const_bytep;
#endif*/
typedef struct
{
	const UINT8 *buffer;
	UINT32 size;
	UINT32 position;
} png_io_t;

static void PNG_IOReader(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_io_t *f = png_get_io_ptr(png_ptr);
	if (length > (f->size - f->position))
		png_error(png_ptr, "PNG_IOReader: buffer overrun");
	memcpy(data, f->buffer + f->position, length);
	f->position += length;
}

typedef struct
{
	char name[4];
	void *data;
	size_t size;
} png_chunk_t;

static png_byte *chunkname = NULL;
static png_chunk_t chunk;

static int PNG_ChunkReader(png_structp png_ptr, png_unknown_chunkp chonk)
{
	(void)png_ptr;
	if (!memcmp(chonk->name, chunkname, 4))
	{
		memcpy(chunk.name, chonk->name, 4);
		chunk.size = chonk->size;
		chunk.data = Z_Malloc(chunk.size, PU_STATIC, NULL);
		memcpy(chunk.data, chonk->data, chunk.size);
		return 1;
	}
	return 0;
}

static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);
	//I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static png_bytep *PNG_Read(const UINT8 *png, UINT16 *w, UINT16 *h, INT16 *topoffset, INT16 *leftoffset, size_t size)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	png_uint_32 y;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif

	png_io_t png_io;
	png_bytep *row_pointers;

	png_byte grAb_chunk[5] = {'g', 'r', 'A', 'b', (png_byte)'\0'};
	png_voidp *user_chunk_ptr;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on initialize libpng\n");
		return NULL;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return NULL;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng load error on %s\n", filename);
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		return NULL;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	// set our own read function
	png_io.buffer = png;
	png_io.size = size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

	memset(&chunk, 0x00, sizeof(png_chunk_t));
	chunkname = grAb_chunk; // I want to read a grAb chunk

	user_chunk_ptr = png_get_user_chunk_ptr(png_ptr);
	png_set_read_user_chunk_fn(png_ptr, user_chunk_ptr, PNG_ChunkReader);
	png_set_keep_unknown_chunks(png_ptr, 2, chunkname, 1);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);
	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);

	// Read the image
	row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (y = 0; y < height; y++)
		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, png_info_ptr));
	png_read_image(png_ptr, row_pointers);

	// Read grAB chunk
	if ((topoffset || leftoffset) && (chunk.data != NULL))
	{
		INT32 *offsets = (INT32 *)chunk.data;
		// read left offset
		if (leftoffset != NULL)
			*leftoffset = (INT16)BIGENDIAN_LONG(*offsets);
		offsets++;
		// read top offset
		if (topoffset != NULL)
			*topoffset = (INT16)BIGENDIAN_LONG(*offsets);
	}

	// bye
	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
	if (chunk.data)
		Z_Free(chunk.data);

	*w = (INT32)width;
	*h = (INT32)height;
	return row_pointers;
}

// Convert a PNG to a raw image.
static UINT8 *PNG_RawConvert(const UINT8 *png, UINT16 *w, UINT16 *h, INT16 *topoffset, INT16 *leftoffset, size_t size)
{
	UINT8 *flat;
	png_uint_32 x, y;
	png_bytep *row_pointers = PNG_Read(png, w, h, topoffset, leftoffset, size);
	png_uint_32 width = *w, height = *h;

	if (!row_pointers)
		I_Error("PNG_RawConvert: conversion failed");

	// Convert the image to 8bpp
	flat = Z_Malloc(width * height, PU_LEVEL, NULL);
	memset(flat, TRANSPARENTPIXEL, width * height);
	for (y = 0; y < height; y++)
	{
		png_bytep row = row_pointers[y];
		for (x = 0; x < width; x++)
		{
			png_bytep px = &(row[x * 4]);
			if ((UINT8)px[3])
				flat[((y * width) + x)] = NearestColor((UINT8)px[0], (UINT8)px[1], (UINT8)px[2]);
		}
	}
	free(row_pointers);

	return flat;
}

// Convert a PNG with transparency to a raw image.
static UINT16 *PNG_MaskedRawConvert(const UINT8 *png, UINT16 *w, UINT16 *h, INT16 *topoffset, INT16 *leftoffset, size_t size)
{
	UINT16 *flat;
	png_uint_32 x, y;
	png_bytep *row_pointers = PNG_Read(png, w, h, topoffset, leftoffset, size);
	png_uint_32 width = *w, height = *h;
	size_t flatsize, i;

	if (!row_pointers)
		I_Error("PNG_MaskedRawConvert: conversion failed");

	// Convert the image to 16bpp
	flatsize = (width * height);
	flat = Z_Malloc(flatsize * sizeof(UINT16), PU_LEVEL, NULL);

	// can't memset here
	for (i = 0; i < flatsize; i++)
		flat[i] = 0xFF00;

	for (y = 0; y < height; y++)
	{
		png_bytep row = row_pointers[y];
		for (x = 0; x < width; x++)
		{
			png_bytep px = &(row[x * 4]);
			if ((UINT8)px[3])
				flat[((y * width) + x)] = NearestColor((UINT8)px[0], (UINT8)px[1], (UINT8)px[2]);
		}
	}
	free(row_pointers);

	return flat;
}

//
// R_PNGToFlat
//
// Convert a PNG to a flat.
//
UINT8 *R_PNGToFlat(UINT16 *width, UINT16 *height, UINT8 *png, size_t size)
{
	return PNG_RawConvert(png, width, height, NULL, NULL, size);
}

//
// R_PNGToPatch
//
// Convert a PNG to a patch.
//
patch_t *R_PNGToPatch(const UINT8 *png, size_t size, size_t *destsize)
{
	UINT16 width, height;
	INT16 topoffset = 0, leftoffset = 0;
	UINT16 *raw = PNG_MaskedRawConvert(png, &width, &height, &topoffset, &leftoffset, size);

	if (!raw)
		I_Error("R_PNGToPatch: conversion failed");

	return R_MaskedFlatToPatch(raw, width, height, leftoffset, topoffset, destsize);
}

//
// R_PNGDimensions
//
// Get the dimensions of a PNG file.
//
boolean R_PNGDimensions(UINT8 *png, INT16 *width, INT16 *height, size_t size)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 w, h;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif

	png_io_t png_io;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on initialize libpng\n");
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng load error on %s\n", filename);
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	// set our own read function
	png_io.buffer = png;
	png_io.size = size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);

	png_get_IHDR(png_ptr, png_info_ptr, &w, &h, &bit_depth, &color_type,
	 NULL, NULL, NULL);

	// okay done. stop.
	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	*width = (INT32)w;
	*height = (INT32)h;
	return true;
}
#endif
#endif

//
// R_ParseSpriteInfoFrame
//
// Parse a SPRTINFO frame.
//
static void R_ParseSpriteInfoFrame(spriteinfo_t *info)
{
	char *sprinfoToken;
	size_t sprinfoTokenLength;
	char *frameChar = NULL;
	UINT8 frameFrame = 0xFF;
	INT16 frameXPivot = 0;
	INT16 frameYPivot = 0;
	rotaxis_t frameRotAxis = 0;

	// Sprite identifier
	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite frame should be");
	}
	sprinfoTokenLength = strlen(sprinfoToken);
	if (sprinfoTokenLength != 1)
	{
		I_Error("Error parsing SPRTINFO lump: Invalid frame \"%s\"",sprinfoToken);
	}
	else
		frameChar = sprinfoToken;

	frameFrame = R_Char2Frame(frameChar[0]);
	Z_Free(sprinfoToken);

	// Left Curly Brace
	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
		I_Error("Error parsing SPRTINFO lump: Missing sprite info");
	else
	{
		if (strcmp(sprinfoToken,"{")==0)
		{
			Z_Free(sprinfoToken);
			sprinfoToken = M_GetToken(NULL);
			if (sprinfoToken == NULL)
			{
				I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite info should be");
			}
			while (strcmp(sprinfoToken,"}")!=0)
			{
				if (stricmp(sprinfoToken, "XPIVOT")==0)
				{
					Z_Free(sprinfoToken);
					sprinfoToken = M_GetToken(NULL);
					frameXPivot = atoi(sprinfoToken);
				}
				else if (stricmp(sprinfoToken, "YPIVOT")==0)
				{
					Z_Free(sprinfoToken);
					sprinfoToken = M_GetToken(NULL);
					frameYPivot = atoi(sprinfoToken);
				}
				else if (stricmp(sprinfoToken, "ROTAXIS")==0)
				{
					Z_Free(sprinfoToken);
					sprinfoToken = M_GetToken(NULL);
					if ((stricmp(sprinfoToken, "X")==0) || (stricmp(sprinfoToken, "XAXIS")==0) || (stricmp(sprinfoToken, "ROLL")==0))
						frameRotAxis = ROTAXIS_X;
					else if ((stricmp(sprinfoToken, "Y")==0) || (stricmp(sprinfoToken, "YAXIS")==0) || (stricmp(sprinfoToken, "PITCH")==0))
						frameRotAxis = ROTAXIS_Y;
					else if ((stricmp(sprinfoToken, "Z")==0) || (stricmp(sprinfoToken, "ZAXIS")==0) || (stricmp(sprinfoToken, "YAW")==0))
						frameRotAxis = ROTAXIS_Z;
				}
				Z_Free(sprinfoToken);

				sprinfoToken = M_GetToken(NULL);
				if (sprinfoToken == NULL)
				{
					I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite info or right curly brace should be");
				}
			}
		}
		Z_Free(sprinfoToken);
	}

	// set fields
	info->pivot[frameFrame].x = frameXPivot;
	info->pivot[frameFrame].y = frameYPivot;
	info->pivot[frameFrame].rotaxis = frameRotAxis;
}

//
// R_ParseSpriteInfo
//
// Parse a SPRTINFO lump.
//
static void R_ParseSpriteInfo(boolean spr2)
{
	spriteinfo_t *info;
	char *sprinfoToken;
	size_t sprinfoTokenLength;
	char newSpriteName[5]; // no longer dynamically allocated
	spritenum_t sprnum = NUMSPRITES;
	playersprite_t spr2num = NUMPLAYERSPRITES;
	INT32 i;
	INT32 skinnumbers[MAXSKINS];
	INT32 foundskins = 0;

	// Sprite name
	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite name should be");
	}
	sprinfoTokenLength = strlen(sprinfoToken);
	if (sprinfoTokenLength != 4)
	{
		I_Error("Error parsing SPRTINFO lump: Sprite name \"%s\" isn't 4 characters long",sprinfoToken);
	}
	else
	{
		memset(&newSpriteName, 0, 5);
		M_Memcpy(newSpriteName, sprinfoToken, sprinfoTokenLength);
		// ^^ we've confirmed that the token is == 4 characters so it will never overflow a 5 byte char buffer
		strupr(newSpriteName); // Just do this now so we don't have to worry about it
	}
	Z_Free(sprinfoToken);

	if (!spr2)
	{
		for (i = 0; i <= NUMSPRITES; i++)
		{
			if (i == NUMSPRITES)
				I_Error("Error parsing SPRTINFO lump: Unknown sprite name \"%s\"", newSpriteName);
			if (!memcmp(newSpriteName,sprnames[i],4))
			{
				sprnum = i;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i <= NUMPLAYERSPRITES; i++)
		{
			if (i == NUMPLAYERSPRITES)
				I_Error("Error parsing SPRTINFO lump: Unknown sprite2 name \"%s\"", newSpriteName);
			if (!memcmp(newSpriteName,spr2names[i],4))
			{
				spr2num = i;
				break;
			}
		}
	}

	// allocate a spriteinfo
	info = Z_Calloc(sizeof(spriteinfo_t), PU_STATIC, NULL);
	info->available = true;

#ifdef ROTSPRITE
	if ((sprites != NULL) && (!spr2))
		R_FreeSingleRotSprite(&sprites[sprnum]);
#endif

	// Left Curly Brace
	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where open curly brace for sprite \"%s\" should be",newSpriteName);
	}
	if (strcmp(sprinfoToken,"{")==0)
	{
		Z_Free(sprinfoToken);
		sprinfoToken = M_GetToken(NULL);
		if (sprinfoToken == NULL)
		{
			I_Error("Error parsing SPRTINFO lump: Unexpected end of file where definition for sprite \"%s\" should be",newSpriteName);
		}
		while (strcmp(sprinfoToken,"}")!=0)
		{
			if (stricmp(sprinfoToken, "SKIN")==0)
			{
				INT32 skinnum;
				char *skinName = NULL;
				if (!spr2)
					I_Error("Error parsing SPRTINFO lump: \"SKIN\" token found outside of a sprite2 definition");

				Z_Free(sprinfoToken);

				// Skin name
				sprinfoToken = M_GetToken(NULL);
				if (sprinfoToken == NULL)
				{
					I_Error("Error parsing SPRTINFO lump: Unexpected end of file where skin frame should be");
				}

				// copy skin name yada yada
				sprinfoTokenLength = strlen(sprinfoToken);
				skinName = (char *)Z_Malloc((sprinfoTokenLength+1)*sizeof(char),PU_STATIC,NULL);
				M_Memcpy(skinName,sprinfoToken,sprinfoTokenLength*sizeof(char));
				skinName[sprinfoTokenLength] = '\0';
				strlwr(skinName);
				Z_Free(sprinfoToken);

				skinnum = R_SkinAvailable(skinName);
				if (skinnum == -1)
					I_Error("Error parsing SPRTINFO lump: Unknown skin \"%s\"", skinName);

				skinnumbers[foundskins] = skinnum;
				foundskins++;
			}
			else if (stricmp(sprinfoToken, "FRAME")==0)
			{
				R_ParseSpriteInfoFrame(info);
				Z_Free(sprinfoToken);

				if (spr2)
				{
					if (!foundskins)
						I_Error("Error parsing SPRTINFO lump: No skins specified in this sprite2 definition");

					for (i = 0; i < foundskins; i++)
					{
						size_t skinnum = skinnumbers[i];
						skin_t *skin = &skins[skinnum];
						spriteinfo_t *sprinfo = skin->sprinfo;
#ifdef ROTSPRITE
						R_FreeSkinRotSprites(skinnum);
#endif
						M_Memcpy(&sprinfo[spr2num], info, sizeof(spriteinfo_t));
					}
				}
				else
					M_Memcpy(&spriteinfo[sprnum], info, sizeof(spriteinfo_t));
			}
			else
			{
				I_Error("Error parsing SPRTINFO lump: Unknown keyword \"%s\" in sprite %s",sprinfoToken,newSpriteName);
			}

			sprinfoToken = M_GetToken(NULL);
			if (sprinfoToken == NULL)
			{
				I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite info or right curly brace for sprite \"%s\" should be",newSpriteName);
			}
		}
	}
	else
	{
		I_Error("Error parsing SPRTINFO lump: Expected \"{\" for sprite \"%s\", got \"%s\"",newSpriteName,sprinfoToken);
	}
	Z_Free(sprinfoToken);
	Z_Free(info);
}

//
// R_ParseSPRTINFOLump
//
// Read a SPRTINFO lump.
//
void R_ParseSPRTINFOLump(UINT16 wadNum, UINT16 lumpNum)
{
	char *sprinfoLump;
	size_t sprinfoLumpLength;
	char *sprinfoText;
	char *sprinfoToken;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	sprinfoLump = (char *)W_CacheLumpNumPwad(wadNum, lumpNum, PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (sprinfoLump == NULL) return;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	sprinfoLumpLength = W_LumpLengthPwad(wadNum, lumpNum);
	sprinfoText = (char *)Z_Malloc((sprinfoLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(sprinfoText,sprinfoLump,sprinfoLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	sprinfoText[sprinfoLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(sprinfoLump);

	sprinfoToken = M_GetToken(sprinfoText);
	while (sprinfoToken != NULL)
	{
		if (!stricmp(sprinfoToken, "SPRITE"))
			R_ParseSpriteInfo(false);
		else if (!stricmp(sprinfoToken, "SPRITE2"))
			R_ParseSpriteInfo(true);
		else
			I_Error("Error parsing SPRTINFO lump: Unknown keyword \"%s\"", sprinfoToken);
		Z_Free(sprinfoToken);
		sprinfoToken = M_GetToken(NULL);
	}
	Z_Free((void *)sprinfoText);
}

//
// R_LoadSpriteInfoLumps
//
// Load and read every SPRTINFO lump from the specified file.
//
void R_LoadSpriteInfoLumps(UINT16 wadnum, UINT16 numlumps)
{
	lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo;
	UINT16 i;
	char *name;

	for (i = 0; i < numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		// Load SPRTINFO and SPR_ lumps as SpriteInfo
		if (!memcmp(name, "SPRTINFO", 8) || !memcmp(name, "SPR_", 4))
			R_ParseSPRTINFOLump(wadnum, i);
	}
}

UINT8 *GetPatchPixel(patch_t *patch, INT32 x, INT32 y, boolean flip)
{
	fixed_t ofs;
	column_t *column;
	UINT8 *source;

	if (x >= 0 && x < SHORT(patch->width))
	{
		INT32 topdelta, prevdelta = -1;
		column = (column_t *)((UINT8 *)patch + LONG(patch->columnofs[flip ? (SHORT(patch->width)-1-x) : x]));
		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			source = (UINT8 *)(column) + 3;
			for (ofs = 0; ofs < column->length; ofs++)
			{
				if ((topdelta + ofs) == y)
					return &source[ofs];
			}
			column = (column_t *)((UINT8 *)column + column->length + 4);
		}
	}

	return NULL;
}

boolean R_ApplyPixelMapToColumn(pixelmap_t *pmap, INT32 *map, patch_t *patch, UINT8 *post, size_t *colsize, boolean flipped)
{
	INT32 x, y;
	size_t pmsize = pmap->size;
	size_t i = 0;
	int lastStartY = 0;
	int spanSize = 0;
	UINT8 *px, *startofspan = NULL, *dest = post;
	boolean written = false;

	while (i < pmap->height)
	{
		y = map[i];
		x = map[i + pmsize];
		px = GetPatchPixel(patch, x, y, flipped); // If not NULL, we have a pixel
		i++;

		// End span if we have a transparent pixel
		if (px == NULL)
		{
			if (startofspan)
				WRITEUINT8(dest, 0);
			startofspan = NULL;
			continue;
		}

		// Start new column if we need to
		if (!startofspan || spanSize == 255)
		{
			int writeY = i;

			// If we reached the span size limit, finish the previous span
			if (startofspan)
				WRITEUINT8(dest, 0);

			if (i > 254)
			{
				// Make sure we're aligned to 254
				if (lastStartY < 254)
				{
					WRITEUINT8(dest, 254);
					WRITEUINT8(dest, 0);
					dest += 2;
					lastStartY = 254;
				}

				// Write stopgap empty spans if needed
				writeY = y - lastStartY;

				while (writeY > 254)
				{
					WRITEUINT8(dest, 254);
					WRITEUINT8(dest, 0);
					dest += 2;
					writeY -= 254;
				}
			}

			startofspan = dest;
			WRITEUINT8(dest, writeY);
			dest += 2;
			spanSize = 0;

			lastStartY = i;
		}

		// Write the pixel
		WRITEUINT8(dest, *px);
		spanSize++;
		startofspan[1] = spanSize;
		written = true;
	}

	if (startofspan)
		WRITEUINT8(dest, 0);
	WRITEUINT8(dest, 0xFF);

	if (colsize)
		*colsize = (dest - post);
	return written;
}

#ifdef ROTSPRITE
INT32 R_GetRollAngle(angle_t rollangle)
{
	INT32 ra = AngleFixed(rollangle)>>FRACBITS;
#if (ROTANGDIFF > 1)
	ra += (ROTANGDIFF/2);
#endif
	ra /= ROTANGDIFF;
	ra %= ROTANGLES;
	return ra;
}

#if 0
patch_t *R_GetRotatedPatch(rotsprite_t *rotsprite, INT32 rollangle, size_t rot)
{
	return rotsprite->patches[rollangle][rot][rendermode-1];
}
#endif

//
// Creates a rotated sprite by calculating a pixel map.
// Caches column data between levels.
//
void R_CacheRotSprite(INT32 rollangle, spriteinfo_t *sprinfo, spriteframe_t *sprframe, UINT8 frame, INT32 rot, UINT16 flip)
{
	patch_t *patch;
	pixelmap_t *pixelmap = &sprframe->rotsprite.pixelmap[rot][rollangle];
	lumpnum_t lump = sprframe->lumppat[rot];
	spriteframepivot_t *pivot = NULL;

	// Sprite lump is invalid.
	if (lump == LUMPERROR)
		return;

	// Cache the patch.
	patch = (patch_t *)W_CachePatchNum(lump, PU_CACHE);

	// Get this frame's pivot from the sprite info.
	if (sprinfo && sprinfo->available)
		pivot = &sprinfo->pivot[frame];

	// If this pixel map was not generated, do it.
	if (!(sprframe->rotsprite.cached[rollangle] & (1<<rot)))
		R_GetRotSpritePixelMap(rollangle, patch, pixelmap, pivot, sprframe, rot, flip);
}

//
// Caches columns of a rotated sprite, applying the pixel map.
//
void R_CacheRotSpriteColumns(pixelmap_t *pixelmap, pmcache_t *cache, patch_t *patch, UINT16 flip)
{
	void **columnofs;
	UINT8 *data;
	boolean *colexists;
	size_t *coltbl;
	static UINT8 pixelmapcol[0xFFFF];
	size_t totalsize = 0, colsize = 0;
	INT16 width = pixelmap->width, x;

	Z_Malloc(width * sizeof(void **), PU_LEVEL, &cache->columnofs);
	colexists = Z_Calloc(width * sizeof(boolean), PU_STATIC, NULL);
	coltbl = Z_Calloc(width * sizeof(size_t), PU_STATIC, NULL);
	columnofs = cache->columnofs;

	for (x = 0; x < width; x++)
	{
		size_t colpos = totalsize;
		colexists[x] = R_ApplyPixelMapToColumn(pixelmap, &(pixelmap->map[x * pixelmap->height]), patch, pixelmapcol, &colsize, flip);
		totalsize += colsize;

		// copy pixels
		if (colexists[x])
		{
			data = Z_Realloc(cache->data, totalsize, PU_LEVEL, &cache->data);
			data += colpos;
			coltbl[x] = colpos;
			M_Memcpy(data, pixelmapcol, colsize);
		}
	}

	for (x = 0; x < width; x++)
	{
		if (colexists[x])
			columnofs[x] = &(cache->data[coltbl[x]]);
		else
			columnofs[x] = NULL;
	}

	Z_Free(colexists);
	Z_Free(coltbl);
}

//
// Calculates the dimensions of a rotated rectangle.
//
static void CalculateRotatedRectangleDimensions(INT16 width, INT16 height, fixed_t ca, fixed_t sa, spriteframepivot_t *pivot, INT16 *newwidth, INT16 *newheight)
{
	if (pivot)
	{
		*newwidth = width + (height * 2);
		*newheight = height + (width * 2);
	}
	else
	{
		fixed_t fw = (width * FRACUNIT);
		fixed_t fh = (height * FRACUNIT);
		INT32 w1 = abs(FixedMul(fw, ca) - FixedMul(fh, sa));
		INT32 w2 = abs(FixedMul(-fw, ca) - FixedMul(fh, sa));
		INT32 h1 = abs(FixedMul(fw, sa) + FixedMul(fh, ca));
		INT32 h2 = abs(FixedMul(-fw, sa) + FixedMul(fh, ca));
		w1 = FixedInt(FixedCeil(w1 + (FRACUNIT/2)));
		w2 = FixedInt(FixedCeil(w2 + (FRACUNIT/2)));
		h1 = FixedInt(FixedCeil(h1 + (FRACUNIT/2)));
		h2 = FixedInt(FixedCeil(h2 + (FRACUNIT/2)));
		*newwidth = max(width, max(w1, w2));
		*newheight = max(height, max(h1, h2));
	}
}

//
// Creates a pixel map for a rotated sprite.
//
void R_GetRotSpritePixelMap(INT32 rollangle, patch_t *patch, pixelmap_t *pixelmap, spriteframepivot_t *spritepivot, spriteframe_t *sprframe, INT32 rot, UINT16 flip)
{
	size_t size;
	INT32 dx, dy;
	INT16 newwidth, newheight;
	fixed_t ca = rollcosang[rollangle];
	fixed_t sa = rollsinang[rollangle];

	INT16 width = SHORT(patch->width);
	INT16 height = SHORT(patch->height);
	INT16 leftoffset = SHORT(patch->leftoffset);

	spriteframepivot_t pivot;
	INT16 rotxcenter, rotycenter;

	pivot.x = (spritepivot ? spritepivot->x : leftoffset);
	pivot.y = (spritepivot ? spritepivot->y : (height / 2));

	if (flip)
	{
		pivot.x = width - pivot.x;
		leftoffset = width - leftoffset;
	}

	// Find the dimensions of the rotated patch.
	CalculateRotatedRectangleDimensions(width, height, ca, sa, (spritepivot ? &pivot : NULL), &newwidth, &newheight);
	rotxcenter = (newwidth / 2);
	rotycenter = (newheight / 2);
	size = (newwidth * newheight);

	// Build pixel map.
	if (pixelmap->map)
		Z_Free(pixelmap->map);
	pixelmap->map = Z_Calloc(size * sizeof(INT32) * 2, PU_STATIC, NULL);
	pixelmap->size = size;
	pixelmap->width = newwidth;
	pixelmap->height = newheight;

	// Calculate the position of every pixel.
	for (dy = 0; dy < newheight; dy++)
	{
		for (dx = 0; dx < newwidth; dx++)
		{
			INT32 dst = (dx*newheight)+dy;
			INT32 x = (dx-rotxcenter) << FRACBITS;
			INT32 y = (dy-rotycenter) << FRACBITS;
			INT32 sx = FixedMul(x, ca) + FixedMul(y, sa) + (pivot.x << FRACBITS);
			INT32 sy = -FixedMul(x, sa) + FixedMul(y, ca) + (pivot.y << FRACBITS);
			sx >>= FRACBITS;
			sy >>= FRACBITS;
			pixelmap->map[dst] = sy;
			pixelmap->map[dst + size] = sx;
		}
	}

	// Set offsets.
	pixelmap->leftoffset = (newwidth / 2) + (leftoffset - pivot.x);
	pixelmap->topoffset = (newheight / 2) + (SHORT(patch->topoffset) - pivot.y);
	pixelmap->topoffset += FEETADJUST>>FRACBITS;

	// This rotation is cached now
	sprframe->rotsprite.cached[rollangle] |= (1<<rot);
}

//
// Free sprite rotation data from memory, for a single spritedef.
//
void R_FreeSingleRotSprite(spritedef_t *spritedef)
{
	UINT8 frame;
	INT32 rot, ang;
	rendermode_t renderer;

	for (frame = 0; frame < spritedef->numframes; frame++)
	{
		spriteframe_t *sprframe = &spritedef->spriteframes[frame];
		for (rot = 0; rot < 16; rot++)
		{
			for (ang = 0; ang < ROTANGLES; ang++)
			{
				if (sprframe->rotsprite.cached[ang] & (1<<rot))
				{
					pixelmap_t *pixelmap = &sprframe->rotsprite.pixelmap[rot][ang];
					pmcache_t *cache = &pixelmap->cache;

					if (pixelmap->map)
						Z_Free(pixelmap->map);
					if (cache->columnofs)
						Z_Free(cache->columnofs);
					if (cache->data)
						Z_Free(cache->data);

					pixelmap->map = NULL;
					cache->columnofs = NULL;
					cache->data = NULL;
				}
				sprframe->rotsprite.cached[ang] &= ~(1<<rot);
			}
		}
	}
}

//
// Free sprite rotation data from memory, for a skin.
// Calls R_FreeSingleRotSprite.
//
void R_FreeSkinRotSprites(size_t skinnum)
{
	size_t i;
	skin_t *skin = &skins[skinnum];
	spritedef_t *skinsprites = skin->sprites;

	for (i = 0; i < NUMPLAYERSPRITES*2; i++)
	{
		R_FreeSingleRotSprite(skinsprites);
		skinsprites++;
	}
}

//
// Frees all sprite rotation data from memory.
//
void R_FreeAllRotSprites(void)
{
	INT32 i;
	size_t s;
	for (s = 0; s < numsprites; s++)
		R_FreeSingleRotSprite(&sprites[s]);
	for (i = 0; i < numskins; ++i)
		R_FreeSkinRotSprites(i);
}
#endif
