// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005-2009 by Andrey "entryway" Budko.
// Copyright (C) 2018-2024 by Lactozilla.
// Copyright (C) 2019-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_picformats.c
/// \brief Picture generation.

#include "byteptr.h"
#include "dehacked.h"
#include "i_video.h"
#include "r_data.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_textures.h"
#include "r_things.h"
#include "r_draw.h"
#include "v_video.h"
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

#ifdef PICTURE_PNG_USELOOKUP
static colorlookup_t png_colorlookup;
#endif

/** Converts a picture between two formats.
  *
  * \param informat Input picture format.
  * \param picture Input picture data.
  * \param outformat Output picture format.
  * \param insize Input picture size.
  * \param outsize Output picture size, as a pointer.
  * \param inwidth Input picture width.
  * \param inheight Input picture height.
  * \param inleftoffset Input picture left offset, for patches.
  * \param intopoffset Input picture top offset, for patches.
  * \param flags Input picture flags.
  * \return A pointer to the converted picture.
  * \sa Picture_PatchConvert
  * \sa Picture_FlatConvert
  */
void *Picture_Convert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	INT32 inwidth, INT32 inheight, INT32 inleftoffset, INT32 intopoffset,
	pictureflags_t flags)
{
	if (informat == PICFMT_NONE)
		I_Error("Picture_Convert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_Convert: output format was PICFMT_NONE!");
	else if (informat == outformat)
		I_Error("Picture_Convert: input and output formats were the same!");

	(void)insize;

	if (Picture_IsPatchFormat(outformat))
	{
		void *converted = Picture_PatchConvert(informat, insize, picture, outformat, outsize, inwidth, inheight, inleftoffset, intopoffset, flags);
		if (converted == NULL)
			I_Error("Picture_Convert: conversion to patch did not result in a valid graphic");
		return converted;
	}
	else if (Picture_IsFlatFormat(outformat))
		return Picture_FlatConvert(informat, picture, outformat, outsize, inwidth, intopoffset, flags);
	else
		I_Error("Picture_Convert: unsupported input format!");

	return NULL;
}

static void *ReadPixelFunc_Patch(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)inwidth;
	(void)inheight;
	return Picture_GetPatchPixel((patch_t*)picture, informat, x, y, flags);
}

static void *ReadPixelFunc_Flat_8bpp(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)informat;
	(void)flags;
	(void)inheight;
	return (UINT8 *)picture + ((y * inwidth) + x);
}

static void *ReadPixelFunc_Flat_16bpp(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)informat;
	(void)flags;
	(void)inheight;
	return (UINT16 *)picture + ((y * inwidth) + x);
}

static void *ReadPixelFunc_Flat_32bpp(void *picture, pictureformat_t informat, INT32 x, INT32 y, INT32 inwidth, INT32 inheight, pictureflags_t flags)
{
	(void)informat;
	(void)flags;
	(void)inheight;
	return (UINT32 *)picture + ((y * inwidth) + x);
}

static UINT8 GetAlphaFunc_32bpp(void *input, pictureflags_t flags)
{
	(void)flags;
	RGBA_t px = *(RGBA_t *)input;
	return px.s.alpha;
}

static UINT8 GetAlphaFunc_16bpp(void *input, pictureflags_t flags)
{
	(void)flags;
	UINT16 px = *(UINT16 *)input;
	return (px & 0xFF00) >> 8;
}

static UINT8 GetAlphaFunc_8bpp(void *input, pictureflags_t flags)
{
	UINT8 px = *(UINT8 *)input;
	if (px == TRANSPARENTPIXEL && (flags & PICFLAGS_USE_TRANSPARENTPIXEL))
		return 0;
	else
		return 255;
}

// input 32bpp output 32bpp
static void *WritePatchPixel_i32o32(void *ptr, void *input)
{
	RGBA_t px = *(RGBA_t *)input;
	WRITEUINT32(ptr, px.rgba);
	return ptr;
}

// input 16bpp output 32bpp
static void *WritePatchPixel_i16o32(void *ptr, void *input)
{
	RGBA_t px = pMasterPalette[*((UINT16 *)input) & 0xFF];
	WRITEUINT32(ptr, px.rgba);
	return ptr;
}

// input 8bpp output 32bpp
static void *WritePatchPixel_i8o32(void *ptr, void *input)
{
	RGBA_t px = pMasterPalette[*((UINT8 *)input) & 0xFF];
	WRITEUINT32(ptr, px.rgba);
	return ptr;
}

// input 32bpp output 16bpp
static void *WritePatchPixel_i32o16(void *ptr, void *input)
{
	RGBA_t in = *(RGBA_t *)input;
	UINT8 px = NearestColor(in.s.red, in.s.green, in.s.blue);
	WRITEUINT16(ptr, (0xFF00 | px));
	return ptr;
}

// input 16bpp output 16bpp
static void *WritePatchPixel_i16o16(void *ptr, void *input)
{
	WRITEUINT16(ptr, *(UINT16 *)input);
	return ptr;
}

// input 8bpp output 16bpp
static void *WritePatchPixel_i8o16(void *ptr, void *input)
{
	WRITEUINT16(ptr, (0xFF00 | (*(UINT8 *)input)));
	return ptr;
}

// input 32bpp output 8bpp
static void *WritePatchPixel_i32o8(void *ptr, void *input)
{
	RGBA_t in = *(RGBA_t *)input;
	UINT8 px = NearestColor(in.s.red, in.s.green, in.s.blue);
	WRITEUINT8(ptr, px);
	return ptr;
}

// input 16bpp output 8bpp
static void *WritePatchPixel_i16o8(void *ptr, void *input)
{
	UINT16 px = *(UINT16 *)input;
	WRITEUINT8(ptr, (px & 0xFF));
	return ptr;
}

// input 8bpp output 8bpp
static void *WritePatchPixel_i8o8(void *ptr, void *input)
{
	WRITEUINT8(ptr, *(UINT8 *)input);
	return ptr;
}

/** Converts a picture to a patch.
  *
  * \param informat Input picture format.
  * \param picture Input picture data.
  * \param outformat Output picture format.
  * \param outsize Output picture size, as a pointer.
  * \param inwidth Input picture width.
  * \param inheight Input picture height.
  * \param inleftoffset Input picture left offset, for patches.
  * \param intopoffset Input picture top offset, for patches.
  * \param flags Input picture flags.
  * \return A pointer to the converted picture.
  */
void *Picture_PatchConvert(
	pictureformat_t informat, size_t insize, void *picture,
	pictureformat_t outformat, size_t *outsize,
	INT32 inwidth, INT32 inheight, INT32 inleftoffset, INT32 intopoffset,
	pictureflags_t flags)
{
	// Shortcut: If converting a Doom patch into a regular patch, use Patch_Create
	if (informat == PICFMT_DOOMPATCH && outformat == PICFMT_PATCH && flags == 0)
	{
		if (outsize != NULL)
			*outsize = sizeof(patch_t);
		return Patch_CreateFromDoomPatch(picture, insize);
	}

	INT32 outbpp = Picture_FormatBPP(outformat);
	INT32 inbpp = Picture_FormatBPP(informat);
	patch_t *inpatch = NULL;

	if (informat == PICFMT_NONE)
		I_Error("Picture_PatchConvert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_PatchConvert: output format was PICFMT_NONE!");
	else if (Picture_IsDoomPatchFormat(outformat))
		I_Error("Picture_PatchConvert: cannot convert to Doom patch!");
	else if (informat == outformat)
		I_Error("Picture_PatchConvert: input and output formats were the same!");

	if (inbpp == PICDEPTH_NONE)
		I_Error("Picture_PatchConvert: unknown input bits per pixel!");
	if (outbpp == PICDEPTH_NONE)
		I_Error("Picture_PatchConvert: unknown output bits per pixel!");

	// If it's a patch, we can just figure out the dimensions from the header.
	if (Picture_IsPatchFormat(informat))
	{
		inpatch = (patch_t *)picture;
		if (Picture_IsDoomPatchFormat(informat))
		{
			softwarepatch_t *doompatch = (softwarepatch_t *)picture;
			inwidth = SHORT(doompatch->width);
			inheight = SHORT(doompatch->height);
			inleftoffset = SHORT(doompatch->leftoffset);
			intopoffset = SHORT(doompatch->topoffset);
		}
		else
		{
			inwidth = inpatch->width;
			inheight = inpatch->height;
			inleftoffset = inpatch->leftoffset;
			intopoffset = inpatch->topoffset;
		}
	}

	void *(*readPixelFunc)(void *, pictureformat_t, INT32, INT32, INT32, INT32, pictureflags_t) = NULL;
	UINT8 (*getAlphaFunc)(void *, pictureflags_t) = NULL;
	void *(*writePatchPixel)(void *, void *) = NULL;

	if (Picture_IsPatchFormat(informat))
		readPixelFunc = ReadPixelFunc_Patch;
	else if (Picture_IsFlatFormat(informat))
	{
		switch (informat)
		{
			case PICFMT_FLAT32:
				readPixelFunc = ReadPixelFunc_Flat_32bpp;
				break;
			case PICFMT_FLAT16:
				readPixelFunc = ReadPixelFunc_Flat_16bpp;
				break;
			case PICFMT_FLAT:
				readPixelFunc = ReadPixelFunc_Flat_8bpp;
				break;
			default:
				I_Error("Picture_PatchConvert: unsupported flat input format!");
				break;
		}
	}
	else
		I_Error("Picture_PatchConvert: unsupported input format!");

	if (inbpp == PICDEPTH_32BPP)
		getAlphaFunc = GetAlphaFunc_32bpp;
	else if (inbpp == PICDEPTH_16BPP)
		getAlphaFunc = GetAlphaFunc_16bpp;
	else if (inbpp == PICDEPTH_8BPP)
		getAlphaFunc = GetAlphaFunc_8bpp;

	switch (outformat)
	{
		case PICFMT_PATCH32:
		case PICFMT_DOOMPATCH32:
		{
			if (inbpp == PICDEPTH_32BPP)
				writePatchPixel = WritePatchPixel_i32o32;
			else if (inbpp == PICDEPTH_16BPP)
				writePatchPixel = WritePatchPixel_i16o32;
			else // PICFMT_PATCH
				writePatchPixel = WritePatchPixel_i8o32;
			break;
		}
		case PICFMT_PATCH16:
		case PICFMT_DOOMPATCH16:
			if (inbpp == PICDEPTH_32BPP)
				writePatchPixel = WritePatchPixel_i32o16;
			else if (inbpp == PICDEPTH_16BPP)
				writePatchPixel = WritePatchPixel_i16o16;
			else // PICFMT_PATCH
				writePatchPixel = WritePatchPixel_i8o16;
			break;
		default: // PICFMT_PATCH
		{
			if (inbpp == PICDEPTH_32BPP)
				writePatchPixel = WritePatchPixel_i32o8;
			else if (inbpp == PICDEPTH_16BPP)
				writePatchPixel = WritePatchPixel_i16o8;
			else // PICFMT_PATCH
				writePatchPixel = WritePatchPixel_i8o8;
			break;
		}
	}

	patch_t *out = Z_Calloc(sizeof(patch_t), PU_PATCH, NULL);

	out->width = inwidth;
	out->height = inheight;
	out->leftoffset = inleftoffset;
	out->topoffset = intopoffset;

	size_t max_pixels = out->width * out->height;
	unsigned num_posts = 0;

	out->columns = Z_Calloc(sizeof(column_t) * out->width, PU_PATCH_DATA, NULL);
	out->pixels = Z_Calloc(max_pixels * (outbpp / 8), PU_PATCH_DATA, NULL);
	out->posts = NULL;

	UINT8 *imgptr = out->pixels;

	unsigned *column_posts = Z_Calloc(sizeof(unsigned) * inwidth, PU_STATIC, NULL);

	// Write columns
	for (INT32 x = 0; x < inwidth; x++)
	{
		post_t *post = NULL;
		size_t post_data_offset = 0;
		boolean was_opaque = false;

		column_t *column = &out->columns[x];
		column->pixels = imgptr;
		column->posts = NULL;
		column->num_posts = 0;

		column_posts[x] = (unsigned)-1;

		// Write pixels
		for (INT32 y = 0; y < inheight; y++)
		{
			boolean opaque = false;

			// Read pixel
			void *input = readPixelFunc(picture, informat, x, y, inwidth, inheight, flags);

			// Determine opacity
			if (input != NULL)
				opaque = getAlphaFunc(input, flags) > 0;

			// End span if we have a transparent pixel
			if (!opaque)
			{
				was_opaque = false;
				continue;
			}

			if (!was_opaque)
			{
				num_posts++;

				out->posts = Z_Realloc(out->posts, sizeof(post_t) * num_posts, PU_PATCH_DATA, NULL);
				post = &out->posts[num_posts - 1];
				post->topdelta = (size_t)y;
				post->length = 0;
				post->data_offset = post_data_offset;
				if (column_posts[x] == (unsigned)-1)
					column_posts[x] = num_posts - 1;
				column->num_posts++;
			}

			was_opaque = true;

			// Write the pixel
			UINT8 *last_ptr = imgptr;
			imgptr = writePatchPixel(last_ptr, input);

			post->length++;
			post_data_offset += imgptr - last_ptr;
		}
	}

	UINT8 *old_pixels = out->pixels;
	size_t total_pixels = imgptr - out->pixels;
	if (total_pixels != max_pixels)
		out->pixels = Z_Realloc(out->pixels, total_pixels, PU_PATCH_DATA, NULL);

	for (INT16 x = 0; x < inwidth; x++)
	{
		column_t *column = &out->columns[x];
		if (column->num_posts > 0)
			column->posts = &out->posts[column_posts[x]];
		if (old_pixels != out->pixels)
			column->pixels = out->pixels + (column->pixels - old_pixels);
	}

	Z_Free(column_posts);

	if (outsize != NULL)
		*outsize = sizeof(patch_t);

	return (void *)out;
}

/** Converts a picture to a flat.
  *
  * \param informat Input picture format.
  * \param picture Input picture data.
  * \param outformat Output picture format.
  * \param outsize Output picture size, as a pointer.
  * \param inwidth Input picture width.
  * \param inheight Input picture height.
  * \param flags Input picture flags.
  * \return A pointer to the converted picture.
  */
void *Picture_FlatConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t *outsize,
	INT32 inwidth, INT32 inheight,
	pictureflags_t flags)
{
	void *outflat;
	patch_t *inpatch = NULL;
	INT32 inbpp = Picture_FormatBPP(informat);
	INT32 outbpp = Picture_FormatBPP(outformat);
	INT32 x, y;
	size_t size;

	if (informat == PICFMT_NONE)
		I_Error("Picture_FlatConvert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_FlatConvert: output format was PICFMT_NONE!");
	else if (informat == outformat)
		I_Error("Picture_FlatConvert: input and output formats were the same!");

	if (inbpp == PICDEPTH_NONE)
		I_Error("Picture_FlatConvert: unknown input bits per pixel?!");
	if (outbpp == PICDEPTH_NONE)
		I_Error("Picture_FlatConvert: unknown output bits per pixel?!");

	// If it's a patch, you can just figure out
	// the dimensions from the header.
	if (Picture_IsPatchFormat(informat))
	{
		inpatch = (patch_t *)picture;
		if (Picture_IsDoomPatchFormat(informat))
		{
			softwarepatch_t *doompatch = ((softwarepatch_t *)picture);
			inwidth = SHORT(doompatch->width);
			inheight = SHORT(doompatch->height);
		}
		else
		{
			inwidth = inpatch->width;
			inheight = inpatch->height;
		}
	}

	size = (inwidth * inheight) * (outbpp / 8);
	outflat = Z_Calloc(size, PU_STATIC, NULL);
	if (outsize)
		*outsize = size;

	// Set transparency
	if (outbpp == PICDEPTH_8BPP)
		memset(outflat, TRANSPARENTPIXEL, size);

	for (y = 0; y < inheight; y++)
		for (x = 0; x < inwidth; x++)
		{
			void *input = NULL;
			int sx = x;
			int sy = y;

			if (flags & PICFLAGS_XFLIP)
				sx = inwidth - x - 1;
			if (flags & PICFLAGS_YFLIP)
				sy = inheight - y - 1;

			size_t in_offs = ((sy * inwidth) + sx);
			size_t out_offs = ((y * inwidth) + x);

			// Read pixel
			if (Picture_IsPatchFormat(informat))
				input = Picture_GetPatchPixel(inpatch, informat, sx, sy, 0);
			else if (Picture_IsFlatFormat(informat))
				input = (UINT8 *)picture + (in_offs * (inbpp / 8));
			else
				I_Error("Picture_FlatConvert: unsupported input format!");

			if (!input)
				continue;

			switch (outformat)
			{
				case PICFMT_FLAT32:
				{
					UINT32 *f32 = (UINT32 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t out = *(RGBA_t *)input;
						f32[out_offs] = out.rgba;
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						RGBA_t out = pMasterPalette[*((UINT16 *)input) & 0xFF];
						f32[out_offs] = out.rgba;
					}
					else // PICFMT_PATCH
					{
						RGBA_t out = pMasterPalette[*((UINT8 *)input) & 0xFF];
						f32[out_offs] = out.rgba;
					}
					break;
				}
				case PICFMT_FLAT16:
				{
					UINT16 *f16 = (UINT16 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						f16[out_offs] = (0xFF00 | out);
					}
					else if (inbpp == PICDEPTH_16BPP)
						f16[out_offs] = *(UINT16 *)input;
					else // PICFMT_PATCH
						f16[out_offs] = (0xFF00 | *((UINT8 *)input));
					break;
				}
				case PICFMT_FLAT:
				{
					UINT8 *f8 = (UINT8 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						f8[out_offs] = out;
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						UINT16 out = *(UINT16 *)input;
						f8[out_offs] = (out & 0xFF);
					}
					else // PICFMT_PATCH
						f8[out_offs] = *(UINT8 *)input;
					break;
				}
				default:
					I_Error("Picture_FlatConvert: unsupported output format!");
			}
		}

	return outflat;
}

/** Returns a pixel from a patch.
  *
  * \param patch Input patch.
  * \param informat Input picture format.
  * \param x Pixel X position.
  * \param y Pixel Y position.
  * \param flags Input picture flags.
  * \return A pointer to a pixel in the patch. Returns NULL if not opaque.
  */
void *Picture_GetPatchPixel(
	patch_t *patch, pictureformat_t informat,
	INT32 x, INT32 y,
	pictureflags_t flags)
{
	INT32 inbpp = Picture_FormatBPP(informat);
	softwarepatch_t *doompatch = (softwarepatch_t *)patch;
	boolean isdoompatch = Picture_IsDoomPatchFormat(informat);

	if (patch == NULL)
		I_Error("Picture_GetPatchPixel: patch == NULL");

	INT16 width = (isdoompatch ? SHORT(doompatch->width) : patch->width);
	INT16 height = (isdoompatch ? SHORT(doompatch->height) : patch->height);

	if (x < 0 || x >= width || y < 0 || y >= height)
		return NULL;

	INT32 sx = (flags & PICFLAGS_XFLIP) ? (width-1)-x : x;
	INT32 sy = (flags & PICFLAGS_YFLIP) ? (height-1)-y : y;
	UINT8 *s8 = NULL;
	UINT16 *s16 = NULL;
	UINT32 *s32 = NULL;

	if (isdoompatch)
	{
		INT32 prevdelta = -1;
		INT32 colofs = LONG(doompatch->columnofs[sx]);

		// Column offsets are pointers, so no casting is required.
		doompost_t *column = (doompost_t *)((UINT8 *)doompatch + colofs);

		while (column->topdelta != 0xff)
		{
			INT32 topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;

			size_t ofs = sy - topdelta;

			if (sy >= topdelta && ofs < column->length)
			{
				s8 = (UINT8 *)(column) + 3;
				switch (inbpp)
				{
					case PICDEPTH_32BPP:
						s32 = (UINT32 *)s8;
						return &s32[ofs];
					case PICDEPTH_16BPP:
						s16 = (UINT16 *)s8;
						return &s16[ofs];
					default: // PICDEPTH_8BPP
						return &s8[ofs];
				}
			}

			if (inbpp == PICDEPTH_32BPP)
				column = (doompost_t *)((UINT32 *)column + column->length);
			else if (inbpp == PICDEPTH_16BPP)
				column = (doompost_t *)((UINT16 *)column + column->length);
			else
				column = (doompost_t *)((UINT8 *)column + column->length);
			column = (doompost_t *)((UINT8 *)column + 4);
		}
	}
	else
	{
		column_t *column = &patch->columns[sx];
		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];

			size_t ofs = sy - post->topdelta;

			if (sy >= (INT32)post->topdelta && ofs < post->length)
			{
				s8 = column->pixels + post->data_offset;
				switch (inbpp)
				{
					case PICDEPTH_32BPP:
						s32 = (UINT32 *)s8;
						return &s32[ofs];
					case PICDEPTH_16BPP:
						s16 = (UINT16 *)s8;
						return &s16[ofs];
					default: // PICDEPTH_8BPP
						return &s8[ofs];
				}
			}
		}
	}

	return NULL;
}

/** Returns the amount of bits per pixel in the specified picture format.
  *
  * \param format Input picture format.
  * \return The bits per pixel amount of the picture format.
  */
INT32 Picture_FormatBPP(pictureformat_t format)
{
	INT32 bpp = PICDEPTH_NONE;
	switch (format)
	{
		case PICFMT_PATCH32:
		case PICFMT_FLAT32:
		case PICFMT_DOOMPATCH32:
		case PICFMT_PNG:
			bpp = PICDEPTH_32BPP;
			break;
		case PICFMT_PATCH16:
		case PICFMT_FLAT16:
		case PICFMT_DOOMPATCH16:
			bpp = PICDEPTH_16BPP;
			break;
		case PICFMT_PATCH:
		case PICFMT_FLAT:
		case PICFMT_DOOMPATCH:
			bpp = PICDEPTH_8BPP;
			break;
		default:
			break;
	}
	return bpp;
}

/** Checks if the specified picture format is a patch.
  *
  * \param format Input picture format.
  * \return True if the picture format is a patch, false if not.
  */
boolean Picture_IsPatchFormat(pictureformat_t format)
{
	return (Picture_IsInternalPatchFormat(format) || Picture_IsDoomPatchFormat(format));
}

/** Checks if the specified picture format is an internal patch.
  *
  * \param format Input picture format.
  * \return True if the picture format is an internal patch, false if not.
  */
boolean Picture_IsInternalPatchFormat(pictureformat_t format)
{
	switch (format)
	{
		case PICFMT_PATCH:
		case PICFMT_PATCH16:
		case PICFMT_PATCH32:
			return true;
		default:
			return false;
	}
}

/** Checks if the specified picture format is a Doom patch.
  *
  * \param format Input picture format.
  * \return True if the picture format is a Doom patch, false if not.
  */
boolean Picture_IsDoomPatchFormat(pictureformat_t format)
{
	switch (format)
	{
		case PICFMT_DOOMPATCH:
		case PICFMT_DOOMPATCH16:
		case PICFMT_DOOMPATCH32:
			return true;
		default:
			return false;
	}
}

/** Checks if the specified picture format is a flat.
  *
  * \param format Input picture format.
  * \return True if the picture format is a flat, false if not.
  */
boolean Picture_IsFlatFormat(pictureformat_t format)
{
	return (format == PICFMT_FLAT || format == PICFMT_FLAT16 || format == PICFMT_FLAT32);
}

/** Returns true if the lump is a valid Doom patch.
  *
  * \param patch Input patch.
  * \param picture Input patch size.
  * \return True if the input patch is valid, false if not.
  */
boolean Picture_CheckIfDoomPatch(softwarepatch_t *patch, size_t size)
{
	// Does not meet minimum size requirements
	if (size < MIN_PATCH_LUMP_SIZE)
		return false;

	INT16 width = SHORT(patch->width);
	INT16 height = SHORT(patch->height);

	// Quickly check for the dimensions first.
	if (width <= 0 || height <= 0 || width > MAX_PATCH_DIMENSIONS || height > MAX_PATCH_DIMENSIONS)
		return false;

	// Lump size makes no sense given the width
	if (!VALID_PATCH_LUMP_SIZE(size, width))
		return false;

	// The dimensions seem like they might be valid for a patch, so
	// check the column directory for extra security. All columns
	// must begin after the column directory, and none of them must
	// point past the end of the patch.
	for (INT16 x = 0; x < width; x++)
	{
		UINT32 ofs = LONG(patch->columnofs[x]);

		// Need one byte for an empty column (but there's patches that don't know that!)
		if (ofs < FIRST_PATCH_LUMP_COLUMN(width) || (size_t)ofs >= size)
		{
			return false;
		}
	}

	return true;
}

/** Converts a texture to a flat.
  *
  * \param texnum The texture number.
  * \return The converted flat.
  */
void *Picture_TextureToFlat(size_t texnum)
{
	texture_t *texture;

	UINT8 *converted;
	size_t flatsize;
	UINT8 *desttop, *dest, *deststop;
	UINT8 *source;

	if (texnum >= (unsigned)numtextures)
		I_Error("Picture_TextureToFlat: invalid texture number!");

	// Check the texture cache
	// If the texture's not there, it'll be generated right now
	texture = textures[texnum];
	R_CheckTextureCache(texnum);

	// Allocate the flat
	flatsize = texture->width * texture->height;
	converted = Z_Malloc(flatsize, PU_STATIC, NULL);
	memset(converted, TRANSPARENTPIXEL, flatsize);

	// Now we're gonna write to it
	desttop = converted;
	deststop = desttop + flatsize;

	for (size_t col = 0; col < (size_t)texture->width; col++, desttop++)
	{
		column_t *column = (column_t *)R_GetColumn(texnum, col);
		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];
			dest = desttop + (post->topdelta * texture->width);
			source = column->pixels + post->data_offset;
			for (size_t ofs = 0; dest < deststop && ofs < post->length; ofs++)
			{
				if (source[ofs] != TRANSPARENTPIXEL)
					*dest = source[ofs];
				dest += texture->width;
			}
		}
	}

	return converted;
}

/** Returns true if the lump is a valid PNG.
  *
  * \param d The lump to be checked.
  * \param s The lump size.
  * \return True if the lump is a PNG image.
  */
boolean Picture_IsLumpPNG(const UINT8 *d, size_t s)
{
	if (s < PNG_MIN_SIZE)
		return false;

	// Check for PNG file signature using memcmp
	// As it may be faster on CPUs with slow unaligned memory access
	// Ref: http://www.libpng.org/pub/png/spec/1.2/PNG-Rationale.html#R.PNG-file-signature
	return (memcmp(&d[0], "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", PNG_HEADER_SIZE) == 0);
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
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static png_byte grAb_chunk[5] = {'g', 'r', 'A', 'b', (png_byte)'\0'};

static png_bytep *PNG_Read(
	const UINT8 *png,
	INT32 *w, INT32 *h, INT16 *topoffset, INT16 *leftoffset,
	boolean *use_palette, size_t size)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
	png_uint_32 y;

	png_colorp palette;
	int palette_size;

	png_bytep trans = NULL;
	int num_trans = 0;

#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif

	png_io_t png_io;
	png_bytep *row_pointers;
	png_voidp *user_chunk_ptr;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
		I_Error("PNG_Read: Couldn't initialize libpng!");

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		I_Error("PNG_Read: libpng couldn't allocate memory!");
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		I_Error("PNG_Read: libpng load error!");
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_io.buffer = png;
	png_io.size = size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

	memset(&chunk, 0x00, sizeof(png_chunk_t));
	chunkname = grAb_chunk;

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

	palette = NULL;
	*use_palette = false;

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		boolean usepal = false;

		// Lactozilla: Check if the PNG has a palette, and if its color count
		// matches the color count of SRB2's palette: 256 colors.
		if (png_get_PLTE(png_ptr, png_info_ptr, &palette, &palette_size))
		{
			if (palette_size == 256 && pMasterPalette)
			{
				png_colorp pal = palette;
				INT32 i;

				usepal = true;

				for (i = 0; i < 256; i++)
				{
					byteColor_t *curpal = &(pMasterPalette[i].s);
					if (pal->red != curpal->red || pal->green != curpal->green || pal->blue != curpal->blue)
					{
						usepal = false;
						break;
					}
					pal++;
				}
			}
		}

		// If any of the tRNS colors have an alpha lower than 0xFF, and that
		// color is present on the image, the palette flag is disabled.
		if (usepal)
		{
			png_uint_32 result = png_get_tRNS(png_ptr, png_info_ptr, &trans, &num_trans, NULL);

			if ((result & PNG_INFO_tRNS) && num_trans > 0 && trans != NULL)
			{
				INT32 i;
				for (i = 0; i < num_trans; i++)
				{
					// libpng will transform this image into RGBA even if
					// the transparent index does not exist in the image,
					// and there is no way around that.
					if (trans[i] < 0xFF)
					{
						usepal = false;
						break;
					}
				}
			}
		}

		if (usepal)
			*use_palette = true;
		else
			png_set_palette_to_rgb(png_ptr);
	}

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

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
	if (chunk.data)
		Z_Free(chunk.data);

	*w = (INT32)width;
	*h = (INT32)height;

	return row_pointers;
}

/** Converts a PNG to a picture.
  *
  * \param png The PNG image.
  * \param outformat The output picture's format.
  * \param w The output picture's width, as a pointer.
  * \param h The output picture's height, as a pointer.
  * \param topoffset The output picture's top offset, for sprites, as a pointer.
  * \param leftoffset The output picture's left offset, for sprites, as a pointer.
  * \param insize The input picture's size.
  * \param outsize A pointer to the output picture's size.
  * \param flags Input picture flags.
  * \return A pointer to the converted picture.
  */
void *Picture_PNGConvert(
	const UINT8 *png, pictureformat_t outformat,
	INT32 *w, INT32 *h,
	INT16 *topoffset, INT16 *leftoffset,
	size_t insize, size_t *outsize,
	pictureflags_t flags)
{
	void *flat;
	INT32 outbpp;
	size_t flatsize;
	png_uint_32 x, y;
	png_bytep row;
	boolean palette = false;
	png_bytep *row_pointers = NULL;
	png_uint_32 width, height;

	INT32 pngwidth, pngheight;
	INT16 loffs = 0, toffs = 0;

	if (png == NULL)
		I_Error("Picture_PNGConvert: picture was NULL!");

	if (w == NULL)
		w = &pngwidth;
	if (h == NULL)
		h = &pngheight;
	if (topoffset == NULL)
		topoffset = &toffs;
	if (leftoffset == NULL)
		leftoffset = &loffs;

	row_pointers = PNG_Read(png, w, h, topoffset, leftoffset, &palette, insize);
	width = *w;
	height = *h;

	if (row_pointers == NULL)
		I_Error("Picture_PNGConvert: row_pointers was NULL!");

	// Find the output format's bits per pixel amount
	outbpp = Picture_FormatBPP(outformat);

	// Hack for patches because you'll want to preserve transparency.
	if (Picture_IsPatchFormat(outformat))
	{
		// Force a higher bit depth
		if (outbpp == PICDEPTH_8BPP)
			outbpp = PICDEPTH_16BPP;
	}

	// Shouldn't happen.
	if (outbpp == PICDEPTH_NONE)
		I_Error("Picture_PNGConvert: unknown output bits per pixel?!");

	// Figure out the size
	flatsize = (width * height) * (outbpp / 8);
	if (outsize)
		*outsize = flatsize;

	// Convert the image
	flat = Z_Calloc(flatsize, PU_STATIC, NULL);

	// Set transparency
	if (outbpp == PICDEPTH_8BPP)
		memset(flat, TRANSPARENTPIXEL, (width * height));

#ifdef PICTURE_PNG_USELOOKUP
	if (outbpp != PICDEPTH_32BPP)
		InitColorLUT(&png_colorlookup, pMasterPalette, false);
#endif

	if (outbpp == PICDEPTH_32BPP)
	{
		RGBA_t out;
		UINT32 *outflat = (UINT32 *)flat;

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					out = V_GetColor(row[x]);
					outflat[((y * width) + x)] = out.rgba;
				}
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					png_bytep px = &(row[x * 4]);
					if ((UINT8)px[3])
					{
						out.s.red = (UINT8)px[0];
						out.s.green = (UINT8)px[1];
						out.s.blue = (UINT8)px[2];
						out.s.alpha = (UINT8)px[3];
						outflat[((y * width) + x)] = out.rgba;
					}
					else
						outflat[((y * width) + x)] = 0x00000000;
				}
			}
		}
	}
	else if (outbpp == PICDEPTH_16BPP)
	{
		UINT16 *outflat = (UINT16 *)flat;

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
					outflat[((y * width) + x)] = (0xFF << 8) | row[x];
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					png_bytep px = &(row[x * 4]);
					UINT8 red = (UINT8)px[0];
					UINT8 green = (UINT8)px[1];
					UINT8 blue = (UINT8)px[2];
					UINT8 alpha = (UINT8)px[3];

					if (alpha)
					{
#ifdef PICTURE_PNG_USELOOKUP
						UINT8 palidx = GetColorLUT(&png_colorlookup, red, green, blue);
#else
						UINT8 palidx = NearestColor(red, green, blue);
#endif
						outflat[((y * width) + x)] = (0xFF << 8) | palidx;
					}
					else
						outflat[((y * width) + x)] = 0x0000;
				}
			}
		}
	}
	else // 8bpp
	{
		UINT8 *outflat = (UINT8 *)flat;

		if (palette)
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
					outflat[((y * width) + x)] = row[x];
			}
		}
		else
		{
			for (y = 0; y < height; y++)
			{
				row = row_pointers[y];
				for (x = 0; x < width; x++)
				{
					png_bytep px = &(row[x * 4]);
					UINT8 red = (UINT8)px[0];
					UINT8 green = (UINT8)px[1];
					UINT8 blue = (UINT8)px[2];
					UINT8 alpha = (UINT8)px[3];

					if (alpha)
					{
#ifdef PICTURE_PNG_USELOOKUP
						UINT8 palidx = GetColorLUT(&png_colorlookup, red, green, blue);
#else
						UINT8 palidx = NearestColor(red, green, blue);
#endif
						outflat[((y * width) + x)] = palidx;
					}
				}
			}
		}
	}

	// Free the row pointers that we allocated for libpng.
	for (y = 0; y < height; y++)
		free(row_pointers[y]);
	free(row_pointers);

	// But wait, there's more!
	if (Picture_IsPatchFormat(outformat))
	{
		void *converted;
		pictureformat_t informat = PICFMT_NONE;

		// Figure out the format of the flat, from the bit depth of the output format
		switch (outbpp)
		{
			case 32:
				informat = PICFMT_FLAT32;
				break;
			case 16:
				informat = PICFMT_FLAT16;
				break;
			default:
				informat = PICFMT_FLAT;
				break;
		}

		// Now, convert it!
		converted = Picture_PatchConvert(informat, flatsize, flat, outformat, outsize, (INT32)width, (INT32)height, *leftoffset, *topoffset, flags);
		if (converted == NULL)
			I_Error("Picture_PNGConvert: conversion to patch did not result in a valid graphic");
		Z_Free(flat);
		return converted;
	}

	// Return the converted flat!
	return flat;
}

/** Returns the dimensions of a PNG image, but doesn't perform any conversions.
  *
  * \param png The PNG image.
  * \param width A pointer to the input picture's width.
  * \param height A pointer to the input picture's height.
  * \param topoffset A pointer to the input picture's vertical offset.
  * \param leftoffset A pointer to the input picture's horizontal offset.
  * \param size The input picture's size.
  * \return True if reading the file succeeded, false if it failed.
  */
boolean Picture_PNGDimensions(UINT8 *png, INT32 *width, INT32 *height, INT16 *topoffset, INT16 *leftoffset, size_t size)
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
	png_voidp *user_chunk_ptr;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Alert(CONS_ERROR, "Picture_PNGDimensions: Couldn't initialize libpng!\n");
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		CONS_Alert(CONS_ERROR, "Picture_PNGDimensions: libpng couldn't allocate memory!\n");
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		CONS_Alert(CONS_ERROR, "Picture_PNGDimensions: libpng load error!\n");
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_io.buffer = png;
	png_io.size = size;
	png_io.position = 0;
	png_set_read_fn(png_ptr, &png_io, PNG_IOReader);

	memset(&chunk, 0x00, sizeof(png_chunk_t));
	chunkname = grAb_chunk;

	user_chunk_ptr = png_get_user_chunk_ptr(png_ptr);
	png_set_read_user_chunk_fn(png_ptr, user_chunk_ptr, PNG_ChunkReader);
	png_set_keep_unknown_chunks(png_ptr, 2, chunkname, 1);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);
	png_get_IHDR(png_ptr, png_info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);

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

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
	if (chunk.data)
		Z_Free(chunk.data);

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
	char newSpriteName[MAXSPRITENAME + 1]; // no longer dynamically allocated
	spritenum_t sprnum = NUMSPRITES;
	playersprite_t spr2num = NUMPLAYERSPRITES;
	INT32 i;
	UINT8 *skinnumbers = NULL;
	INT32 foundskins = 0;

	// Sprite name
	sprinfoToken = M_GetToken(NULL);
	if (sprinfoToken == NULL)
	{
		I_Error("Error parsing SPRTINFO lump: Unexpected end of file where sprite name should be");
	}
	sprinfoTokenLength = strlen(sprinfoToken);
	if (sprinfoTokenLength > MAXSPRITENAME)
		I_Error("Error parsing SPRTINFO lump: Sprite name \"%s\" is longer than %d characters", sprinfoToken, MAXSPRITENAME);
	strcpy(newSpriteName, sprinfoToken);
	strupr(newSpriteName); // Just do this now so we don't have to worry about it
	Z_Free(sprinfoToken);

	if (!spr2)
	{
		sprnum = R_GetSpriteNumByName(newSpriteName);
		if (sprnum == NUMSPRITES)
			I_Error("Error parsing SPRTINFO lump: Unknown sprite name \"%s\"", newSpriteName);
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

				if (skinnumbers == NULL)
					skinnumbers = Z_Malloc(sizeof(UINT8) * numskins, PU_STATIC, NULL);
				skinnumbers[foundskins] = (UINT8)skinnum;
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
						skin_t *skin = skins[skinnumbers[i]];
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
	if (skinnumbers)
		Z_Free(skinnumbers);
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
