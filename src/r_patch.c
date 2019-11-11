// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patch.c
/// \brief Patch generation.

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

static unsigned char imgbuf[1<<26];
fixed_t cosang2rad[ROTANGLES];
fixed_t sinang2rad[ROTANGLES];

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
// R_PatchToFlat_16bpp
//
// Convert a patch to a 16-bit flat.
//
void R_PatchToFlat_16bpp(patch_t *patch, UINT16 *raw, boolean flip)
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
// R_FlatToPatch_16bpp
//
// Convert a 16-bit flat to a patch.
//
patch_t *R_FlatToPatch_16bpp(UINT16 *raw, UINT16 width, UINT16 height, size_t *size)
{
	UINT32 x, y;
	UINT8 *img;
	UINT8 *imgptr = imgbuf;
	UINT8 *colpointers, *startofspan;

	if (!raw)
		return NULL;

	// Write image size and offset
	WRITEINT16(imgptr, width);
	WRITEINT16(imgptr, height);
	// no offsets
	WRITEINT16(imgptr, 0);
	WRITEINT16(imgptr, 0);

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

	*size = imgptr-imgbuf;
	img = Z_Malloc(*size, PU_STATIC, NULL);
	memcpy(img, imgbuf, *size);
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
patch_t *R_PNGToPatch(const UINT8 *png, size_t size, size_t *destsize, boolean transparency)
{
	UINT16 width, height;
	INT16 topoffset = 0, leftoffset = 0;
	UINT8 *raw = PNG_RawConvert(png, &width, &height, &topoffset, &leftoffset, size);

	if (!raw)
		I_Error("R_PNGToPatch: conversion failed");

	return R_FlatToPatch(raw, width, height, leftoffset, topoffset, destsize, transparency);
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
	char *frameChar;
	UINT8 frameFrame;
#ifdef ROTSPRITE
	INT16 frameXPivot = 0;
	INT16 frameYPivot = 0;
#endif

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
#ifdef ROTSPRITE
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
#endif
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
#ifdef ROTSPRITE
	info->pivot[frameFrame].x = frameXPivot;
	info->pivot[frameFrame].y = frameYPivot;
#endif
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
						skin_t *skin = &skins[skinnumbers[i]];
						spriteinfo_t *sprinfo = skin->sprinfo;
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
		if (!stricmp(sprinfoToken, "SPRITE") || !stricmp(sprinfoToken, "SPRITE2"))
		{
			Z_Free(sprinfoToken);
			R_ParseSpriteInfo(!stricmp(sprinfoToken, "SPRITE2"));
		}
		else
			I_Error("Error parsing SPRTINFO lump: Unknown keyword \"%s\"", sprinfoToken);
		sprinfoToken = M_GetToken(NULL);
	}
	Z_Free(sprinfoToken);
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
		// load SPRTINFO lumps
		if (!stricmp(name, "SPRTINFO"))
			R_ParseSPRTINFOLump(wadnum, i);
		// load SPR_ lumps (as DEHACKED lump)
		else if (!memcmp(name, "SPR_", 4))
			DEH_LoadDehackedLumpPwad(wadnum, i, false);
	}
}

#ifdef ROTSPRITE
//
// R_CacheRotSprite
//
// Create a rotated sprite.
//
void R_CacheRotSprite(spritenum_t sprnum, UINT8 frame, spriteinfo_t *sprinfo, spriteframe_t *sprframe, INT32 rot, UINT8 flip)
{
	UINT32 i;
	INT32 angle;
	patch_t *patch;
	patch_t *newpatch;
	UINT16 *rawsrc, *rawdst;
	size_t size, size2;
	INT32 bflip = ((flip != 0x00) ? -1 : 1);

#define SPRITE_XCENTER (patch->leftoffset)
#define SPRITE_YCENTER (height / 2)
#define ROTSPRITE_XCENTER (newwidth / 2)
#define ROTSPRITE_YCENTER (newheight / 2)

	if (!sprframe->rotsprite.cached[rot])
	{
		INT32 dx, dy;
		INT32 px, py;
		INT32 width, height;
		fixed_t ca, sa;
		lumpnum_t lump = sprframe->lumppat[rot];

		if (lump == LUMPERROR)
			return;
		// Because there's something wrong with SPR_DFLM, I guess
		if (!R_CheckIfPatch(lump))
			return;

		patch = (patch_t *)W_CacheLumpNum(lump, PU_STATIC);
		width = patch->width;
		height = patch->height;

		// rotation pivot
		px = SPRITE_XCENTER;
		py = SPRITE_YCENTER;

		// get correct sprite info for sprite
		if (sprinfo == NULL)
			sprinfo = &spriteinfo[sprnum];
		if (sprinfo->available)
		{
			px = sprinfo->pivot[frame].x;
			py = sprinfo->pivot[frame].y;
		}
		if (flip)
			px = width - px;

		// Draw the sprite to a temporary buffer.
		size = (width*height);
		rawsrc = Z_Malloc(size * sizeof(UINT16), PU_STATIC, NULL);

		// can't memset here
		for (i = 0; i < size; i++)
			rawsrc[i] = 0xFF00;

		R_PatchToFlat_16bpp(patch, rawsrc, (flip != 0x00));

		// Don't cache angle = 0
		for (angle = 1; angle < ROTANGLES; angle++)
		{
			INT32 newwidth, newheight;

			ca = cosang2rad[angle];
			sa = sinang2rad[angle];

			// Find the dimensions of the rotated patch.
			{
				INT32 w1 = abs(FixedMul(width << FRACBITS, ca) - FixedMul(height << FRACBITS, sa));
				INT32 w2 = abs(FixedMul(-(width << FRACBITS), ca) - FixedMul(height << FRACBITS, sa));
				INT32 h1 = abs(FixedMul(width << FRACBITS, sa) + FixedMul(height << FRACBITS, ca));
				INT32 h2 = abs(FixedMul(-(width << FRACBITS), sa) + FixedMul(height << FRACBITS, ca));
				w1 = FixedInt(FixedCeil(w1 + (FRACUNIT/2)));
				w2 = FixedInt(FixedCeil(w2 + (FRACUNIT/2)));
				h1 = FixedInt(FixedCeil(h1 + (FRACUNIT/2)));
				h2 = FixedInt(FixedCeil(h2 + (FRACUNIT/2)));
				newwidth = max(width, max(w1, w2));
				newheight = max(height, max(h1, h2));
			}

			// check boundaries
			{
				fixed_t top[2][2];
				fixed_t bottom[2][2];

				top[0][0] = FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, sa) + (px << FRACBITS);
				top[0][1] = FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, ca) + (py << FRACBITS);
				top[1][0] = FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, sa) + (px << FRACBITS);
				top[1][1] = FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((-ROTSPRITE_YCENTER) << FRACBITS, ca) + (py << FRACBITS);

				bottom[0][0] = FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, sa) + (px << FRACBITS);
				bottom[0][1] = -FixedMul((-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, ca) + (py << FRACBITS);
				bottom[1][0] = FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, ca) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, sa) + (px << FRACBITS);
				bottom[1][1] = -FixedMul((newwidth-ROTSPRITE_XCENTER) << FRACBITS, sa) + FixedMul((newheight-ROTSPRITE_YCENTER) << FRACBITS, ca) + (py << FRACBITS);

				top[0][0] >>= FRACBITS;
				top[0][1] >>= FRACBITS;
				top[1][0] >>= FRACBITS;
				top[1][1] >>= FRACBITS;

				bottom[0][0] >>= FRACBITS;
				bottom[0][1] >>= FRACBITS;
				bottom[1][0] >>= FRACBITS;
				bottom[1][1] >>= FRACBITS;

#define BOUNDARYWCHECK(b) (b[0] < 0 || b[0] >= width)
#define BOUNDARYHCHECK(b) (b[1] < 0 || b[1] >= height)
#define BOUNDARYADJUST(x) x *= 2
				// top left/right
				if (BOUNDARYWCHECK(top[0]) || BOUNDARYWCHECK(top[1]))
					BOUNDARYADJUST(newwidth);
				// bottom left/right
				else if (BOUNDARYWCHECK(bottom[0]) || BOUNDARYWCHECK(bottom[1]))
					BOUNDARYADJUST(newwidth);
				// top left/right
				if (BOUNDARYHCHECK(top[0]) || BOUNDARYHCHECK(top[1]))
					BOUNDARYADJUST(newheight);
				// bottom left/right
				else if (BOUNDARYHCHECK(bottom[0]) || BOUNDARYHCHECK(bottom[1]))
					BOUNDARYADJUST(newheight);
#undef BOUNDARYWCHECK
#undef BOUNDARYHCHECK
#undef BOUNDARYADJUST
			}

			size2 = (newwidth * newheight);
			if (!size2)
				size2 = size;

			rawdst = Z_Malloc(size2 * sizeof(UINT16), PU_STATIC, NULL);

			// can't memset here
			for (i = 0; i < size2; i++)
				rawdst[i] = 0xFF00;

			// Draw the rotated sprite to a temporary buffer.
			for (dy = 0; dy < newheight; dy++)
			{
				for (dx = 0; dx < newwidth; dx++)
				{
					INT32 x = (dx-ROTSPRITE_XCENTER) << FRACBITS;
					INT32 y = (dy-ROTSPRITE_YCENTER) << FRACBITS;
					INT32 sx = FixedMul(x, ca) + FixedMul(y, sa) + (px << FRACBITS);
					INT32 sy = -FixedMul(x, sa) + FixedMul(y, ca) + (py << FRACBITS);
					sx >>= FRACBITS;
					sy >>= FRACBITS;
					if (sx >= 0 && sy >= 0 && sx < width && sy < height)
						rawdst[(dy*newwidth)+dx] = rawsrc[(sy*width)+sx];
				}
			}

			// make patch
			newpatch = R_FlatToPatch_16bpp(rawdst, newwidth, newheight, &size);
			newpatch->leftoffset = (newpatch->width / 2) - ((SPRITE_XCENTER - patch->leftoffset) * bflip);
			newpatch->topoffset = (newpatch->height / 2) - (SPRITE_YCENTER - patch->topoffset);
			newpatch->leftoffset += ((width / 2) - px);
			newpatch->topoffset += (SPRITE_YCENTER - py);

			//BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
			if (rendermode != render_none) // not for psprite
				newpatch->topoffset += FEETADJUST>>FRACBITS;

			// P_PrecacheLevel
			if (devparm) spritememory += size;

#ifdef HWRENDER
			if (rendermode == render_opengl)
			{
				GLPatch_t *grPatch = HWR_GetCachedGLRotSprite(sprframe->rotsprite.hardware_patch[rot], angle, newpatch);
				HWR_MakePatch(newpatch, grPatch, grPatch->mipmap, false);
				sprframe->rotsprite.patch[rot][angle] = (patch_t *)grPatch;
			}
			else
#endif // HWRENDER
				sprframe->rotsprite.patch[rot][angle] = newpatch;

			// free rotated image data
			Z_Free(rawdst);
		}

		// This rotation is cached now
		sprframe->rotsprite.cached[rot] = true;

		// free image data
		Z_Free(rawsrc);
		Z_Free(patch);
	}
#undef SPRITE_XCENTER
#undef SPRITE_YCENTER
#undef ROTSPRITE_XCENTER
#undef ROTSPRITE_YCENTER
}
#endif
