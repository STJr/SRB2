// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2005-2009 by Andrey "entryway" Budko.
// Copyright (C) 2018-2021 by Jaime "Lactozilla" Passos.
// Copyright (C) 2019-2021 by Sonic Team Junior.
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

static unsigned char imgbuf[1<<26];

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

	if (Picture_IsPatchFormat(outformat))
		return Picture_PatchConvert(informat, picture, outformat, insize, outsize, inwidth, inheight, inleftoffset, intopoffset, flags);
	else if (Picture_IsFlatFormat(outformat))
		return Picture_FlatConvert(informat, picture, outformat, insize, outsize, inwidth, inheight, inleftoffset, intopoffset, flags);
	else
		I_Error("Picture_Convert: unsupported input format!");

	return NULL;
}

/** Converts a picture to a patch.
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
  */
void *Picture_PatchConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	INT16 inwidth, INT16 inheight, INT16 inleftoffset, INT16 intopoffset,
	pictureflags_t flags)
{
	INT16 x, y;
	UINT8 *img;
	UINT8 *imgptr = imgbuf;
	UINT8 *colpointers, *startofspan;
	size_t size = 0;
	patch_t *inpatch = NULL;
	INT32 inbpp = Picture_FormatBPP(informat);

	(void)insize; // ignore

	if (informat == PICFMT_NONE)
		I_Error("Picture_PatchConvert: input format was PICFMT_NONE!");
	else if (outformat == PICFMT_NONE)
		I_Error("Picture_PatchConvert: output format was PICFMT_NONE!");
	else if (informat == outformat)
		I_Error("Picture_PatchConvert: input and output formats were the same!");

	if (inbpp == PICDEPTH_NONE)
		I_Error("Picture_PatchConvert: unknown input bits per pixel?!");
	if (Picture_FormatBPP(outformat) == PICDEPTH_NONE)
		I_Error("Picture_PatchConvert: unknown output bits per pixel?!");

	// If it's a patch, you can just figure out
	// the dimensions from the header.
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

	// Write image size and offset
	WRITEINT16(imgptr, inwidth);
	WRITEINT16(imgptr, inheight);
	WRITEINT16(imgptr, inleftoffset);
	WRITEINT16(imgptr, intopoffset);

	// Leave placeholder to column pointers
	colpointers = imgptr;
	imgptr += inwidth*4;

	// Write columns
	for (x = 0; x < inwidth; x++)
	{
		int lastStartY = 0;
		int spanSize = 0;
		startofspan = NULL;

		// Write column pointer
		WRITEINT32(colpointers, imgptr - imgbuf);

		// Write pixels
		for (y = 0; y < inheight; y++)
		{
			void *input = NULL;
			boolean opaque = false;

			// Read pixel
			if (Picture_IsPatchFormat(informat))
				input = Picture_GetPatchPixel(inpatch, informat, x, y, flags);
			else if (Picture_IsFlatFormat(informat))
			{
				size_t offs = ((y * inwidth) + x);
				switch (informat)
				{
					case PICFMT_FLAT32:
						input = (UINT32 *)picture + offs;
						break;
					case PICFMT_FLAT16:
						input = (UINT16 *)picture + offs;
						break;
					case PICFMT_FLAT:
						input = (UINT8 *)picture + offs;
						break;
					default:
						I_Error("Picture_PatchConvert: unsupported flat input format!");
						break;
				}
			}
			else
				I_Error("Picture_PatchConvert: unsupported input format!");

			// Determine opacity
			if (input != NULL)
			{
				UINT8 alpha = 0xFF;
				if (inbpp == PICDEPTH_32BPP)
				{
					RGBA_t px = *(RGBA_t *)input;
					alpha = px.s.alpha;
				}
				else if (inbpp == PICDEPTH_16BPP)
				{
					UINT16 px = *(UINT16 *)input;
					alpha = (px & 0xFF00) >> 8;
				}
				else if (inbpp == PICDEPTH_8BPP)
				{
					UINT8 px = *(UINT8 *)input;
					if (px == TRANSPARENTPIXEL)
						alpha = 0;
				}
				opaque = (alpha > 1);
			}

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
			switch (outformat)
			{
				case PICFMT_PATCH32:
				case PICFMT_DOOMPATCH32:
				{
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t out = *(RGBA_t *)input;
						WRITEUINT32(imgptr, out.rgba);
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						RGBA_t out = pMasterPalette[*((UINT16 *)input) & 0xFF];
						WRITEUINT32(imgptr, out.rgba);
					}
					else // PICFMT_PATCH
					{
						RGBA_t out = pMasterPalette[*((UINT8 *)input) & 0xFF];
						WRITEUINT32(imgptr, out.rgba);
					}
					break;
				}
				case PICFMT_PATCH16:
				case PICFMT_DOOMPATCH16:
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						WRITEUINT16(imgptr, (0xFF00 | out));
					}
					else if (inbpp == PICDEPTH_16BPP)
						WRITEUINT16(imgptr, *(UINT16 *)input);
					else // PICFMT_PATCH
						WRITEUINT16(imgptr, (0xFF00 | (*(UINT8 *)input)));
					break;
				default: // PICFMT_PATCH
				{
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						WRITEUINT8(imgptr, out);
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						UINT16 out = *(UINT16 *)input;
						WRITEUINT8(imgptr, (out & 0xFF));
					}
					else // PICFMT_PATCH
						WRITEUINT8(imgptr, *(UINT8 *)input);
					break;
				}
			}

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

	if (Picture_IsInternalPatchFormat(outformat))
	{
		patch_t *converted = Patch_Create((softwarepatch_t *)img, size, NULL);

#ifdef HWRENDER
		Patch_CreateGL(converted);
#endif

		Z_Free(img);

		if (outsize != NULL)
			*outsize = sizeof(patch_t);
		return converted;
	}
	else
	{
		if (outsize != NULL)
			*outsize = size;
		return img;
	}
}

/** Converts a picture to a flat.
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
  */
void *Picture_FlatConvert(
	pictureformat_t informat, void *picture, pictureformat_t outformat,
	size_t insize, size_t *outsize,
	INT16 inwidth, INT16 inheight, INT16 inleftoffset, INT16 intopoffset,
	pictureflags_t flags)
{
	void *outflat;
	patch_t *inpatch = NULL;
	INT32 inbpp = Picture_FormatBPP(informat);
	INT32 outbpp = Picture_FormatBPP(outformat);
	INT32 x, y;
	size_t size;

	(void)insize; // ignore
	(void)inleftoffset; // ignore
	(void)intopoffset; // ignore

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
			void *input;
			size_t offs = ((y * inwidth) + x);

			// Read pixel
			if (Picture_IsPatchFormat(informat))
				input = Picture_GetPatchPixel(inpatch, informat, x, y, flags);
			else if (Picture_IsFlatFormat(informat))
				input = (UINT8 *)picture + (offs * (inbpp / 8));
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
						f32[offs] = out.rgba;
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						RGBA_t out = pMasterPalette[*((UINT16 *)input) & 0xFF];
						f32[offs] = out.rgba;
					}
					else // PICFMT_PATCH
					{
						RGBA_t out = pMasterPalette[*((UINT8 *)input) & 0xFF];
						f32[offs] = out.rgba;
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
						f16[offs] = (0xFF00 | out);
					}
					else if (inbpp == PICDEPTH_16BPP)
						f16[offs] = *(UINT16 *)input;
					else // PICFMT_PATCH
						f16[offs] = (0xFF00 | *((UINT8 *)input));
					break;
				}
				case PICFMT_FLAT:
				{
					UINT8 *f8 = (UINT8 *)outflat;
					if (inbpp == PICDEPTH_32BPP)
					{
						RGBA_t in = *(RGBA_t *)input;
						UINT8 out = NearestColor(in.s.red, in.s.green, in.s.blue);
						f8[offs] = out;
					}
					else if (inbpp == PICDEPTH_16BPP)
					{
						UINT16 out = *(UINT16 *)input;
						f8[offs] = (out & 0xFF);
					}
					else // PICFMT_PATCH
						f8[offs] = *(UINT8 *)input;
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
	fixed_t ofs;
	column_t *column;
	UINT8 *s8 = NULL;
	UINT16 *s16 = NULL;
	UINT32 *s32 = NULL;
	softwarepatch_t *doompatch = (softwarepatch_t *)patch;
	boolean isdoompatch = Picture_IsDoomPatchFormat(informat);
	INT16 width;

	if (patch == NULL)
		I_Error("Picture_GetPatchPixel: patch == NULL");

	width = (isdoompatch ? SHORT(doompatch->width) : patch->width);

	if (x >= 0 && x < width)
	{
		INT32 colx = (flags & PICFLAGS_XFLIP) ? (width-1)-x : x;
		INT32 topdelta, prevdelta = -1;
		INT32 colofs = (isdoompatch ? LONG(doompatch->columnofs[colx]) : patch->columnofs[colx]);

		// Column offsets are pointers, so no casting is required.
		if (isdoompatch)
			column = (column_t *)((UINT8 *)doompatch + colofs);
		else
			column = (column_t *)((UINT8 *)patch->columns + colofs);

		while (column->topdelta != 0xff)
		{
			topdelta = column->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;
			s8 = (UINT8 *)(column) + 3;
			if (Picture_FormatBPP(informat) == PICDEPTH_32BPP)
				s32 = (UINT32 *)s8;
			else if (Picture_FormatBPP(informat) == PICDEPTH_16BPP)
				s16 = (UINT16 *)s8;
			for (ofs = 0; ofs < column->length; ofs++)
			{
				if ((topdelta + ofs) == y)
				{
					if (Picture_FormatBPP(informat) == PICDEPTH_32BPP)
						return &s32[ofs];
					else if (Picture_FormatBPP(informat) == PICDEPTH_16BPP)
						return &s16[ofs];
					else // PICDEPTH_8BPP
						return &s8[ofs];
				}
			}
			if (Picture_FormatBPP(informat) == PICDEPTH_32BPP)
				column = (column_t *)((UINT32 *)column + column->length);
			else if (Picture_FormatBPP(informat) == PICDEPTH_16BPP)
				column = (column_t *)((UINT16 *)column + column->length);
			else
				column = (column_t *)((UINT8 *)column + column->length);
			column = (column_t *)((UINT8 *)column + 4);
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
  * PICFMT_DOOMPATCH only.
  *
  * \param patch Input patch.
  * \param picture Input patch size.
  * \return True if the input patch is valid.
  */
boolean Picture_CheckIfDoomPatch(softwarepatch_t *patch, size_t size)
{
	INT16 width, height;
	boolean result;

	// minimum length of a valid Doom patch
	if (size < 13)
		return false;

	width = SHORT(patch->width);
	height = SHORT(patch->height);
	result = (height > 0 && height <= 16384 && width > 0 && width <= 16384);

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

/** Converts a texture to a flat.
  *
  * \param trickytex The texture number.
  * \return The converted flat.
  */
void *Picture_TextureToFlat(size_t trickytex)
{
	texture_t *texture;
	size_t tex;

	UINT8 *converted;
	size_t flatsize;
	fixed_t col, ofs;
	column_t *column;
	UINT8 *desttop, *dest, *deststop;
	UINT8 *source;

	if (trickytex >= (unsigned)numtextures)
		I_Error("Picture_TextureToFlat: invalid texture number!");

	// Check the texture cache
	// If the texture's not there, it'll be generated right now
	tex = trickytex;
	texture = textures[tex];
	R_CheckTextureCache(tex);

	// Allocate the flat
	flatsize = (texture->width * texture->height);
	converted = Z_Malloc(flatsize, PU_STATIC, NULL);
	memset(converted, TRANSPARENTPIXEL, flatsize);

	// Now we're gonna write to it
	desttop = converted;
	deststop = desttop + flatsize;
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

	png_bytep trans;
	int trans_num;
	png_color_16p trans_values;

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
					UINT32 rgb = R_PutRgbaRGBA(pal->red, pal->green, pal->blue, 0xFF);
					if (rgb != pMasterPalette[i].rgba)
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
			png_get_tRNS(png_ptr, png_info_ptr, &trans, &trans_num, &trans_values);

			if (trans && trans_num == 256)
			{
				INT32 i;
				for (i = 0; i < trans_num; i++)
				{
					// libpng will transform this image into RGB even if
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
		converted = Picture_PatchConvert(informat, flat, outformat, insize, outsize, (INT16)width, (INT16)height, *leftoffset, *topoffset, flags);
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
		I_Error("Picture_PNGDimensions: Couldn't initialize libpng!");

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		I_Error("Picture_PNGDimensions: libpng couldn't allocate memory!");
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		I_Error("Picture_PNGDimensions: libpng load error!");
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

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
