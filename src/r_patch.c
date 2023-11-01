// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_patch.c
/// \brief Patch generation.

#include "doomdef.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_defs.h"
#include "z_zone.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

static boolean Patch_CheckDirtyRect(dynamicpatch_t *dpatch);
static void Patch_ClearDirtyRect(dynamicpatch_t *dpatch);

static boolean Patch_CheckDirtyColumns(dynamicpatch_t *dpatch);
static void Patch_ClearDirtyColumns(dynamicpatch_t *dpatch);

patch_t *Patch_Create(INT16 width, INT16 height)
{
	patch_t *patch = Z_Calloc(sizeof(staticpatch_t), PU_PATCH, NULL);

	patch->width = width;
	patch->height = height;
	patch->type = PATCH_TYPE_STATIC;
	patch->format = PATCH_FORMAT_PALETTE;

	return patch;
}

patch_t *Patch_CreateDynamic(INT16 width, INT16 height, UINT8 format)
{
	patch_t *patch = Z_Calloc(sizeof(dynamicpatch_t), PU_PATCH, NULL);

	patch->width = width;
	patch->height = height;
	patch->type = PATCH_TYPE_DYNAMIC;
	patch->format = format;

	dynamicpatch_t *dpatch = (dynamicpatch_t*)patch;
	dpatch->is_dirty = false;
	dpatch->update_columns = false;

	if (format == PATCH_FORMAT_PALETTE)
		dpatch->pixels_opaque = Z_Calloc(BIT_ARRAY_SIZE(width * height), PU_PATCH_DATA, NULL);
	else if (format == PATCH_FORMAT_RGBA)
		patch->pixels = Z_Calloc(patch->width * patch->height * Patch_GetBpp(patch), PU_PATCH_DATA, NULL);

	Patch_ClearDirtyRect(dpatch);
	Patch_ClearDirtyColumns(dpatch);

	return patch;
}

void Patch_MarkDirtyRect(patch_t *patch, INT16 left, INT16 top, INT16 right, INT16 bottom)
{
	dynamicpatch_t *dpatch = (dynamicpatch_t*)patch;

	left = max(0, left);
	right = min(right, patch->width);
	top = max(0, top);
	bottom = min(bottom, patch->height);

	if (left < dpatch->rect_dirty[0])
		dpatch->rect_dirty[0] = left;
	if (top < dpatch->rect_dirty[1])
		dpatch->rect_dirty[1] = top;
	if (right > dpatch->rect_dirty[2])
		dpatch->rect_dirty[2] = right;
	if (bottom > dpatch->rect_dirty[3])
		dpatch->rect_dirty[3] = bottom;

	if (left < dpatch->column_dirty[0])
		dpatch->column_dirty[0] = left;
	if (right > dpatch->column_dirty[1])
		dpatch->column_dirty[1] = right;
}

static boolean Patch_CheckDirtyRect(dynamicpatch_t *dpatch)
{
	patch_t *patch = (patch_t*)dpatch;

	// left
	if (dpatch->rect_dirty[0] < 0
	|| dpatch->rect_dirty[0] > patch->width
	|| dpatch->rect_dirty[0] >= dpatch->rect_dirty[2]) // right
		return false;

	// top
	if (dpatch->rect_dirty[1] < 0
	|| dpatch->rect_dirty[1] > patch->height
	|| dpatch->rect_dirty[1] >= dpatch->rect_dirty[3]) // bottom
		return false;

	// right
	if (dpatch->rect_dirty[2] > patch->width
	|| dpatch->rect_dirty[2] < 0
	|| dpatch->rect_dirty[2] <= dpatch->rect_dirty[0]) // left
		return false;

	// bottom
	if (dpatch->rect_dirty[3] > patch->height
	|| dpatch->rect_dirty[3] < 0
	|| dpatch->rect_dirty[3] <= dpatch->rect_dirty[1]) // top
		return false;

	return true;
}

static void Patch_ClearDirtyRect(dynamicpatch_t *dpatch)
{
	dpatch->rect_dirty[0] = INT16_MAX;
	dpatch->rect_dirty[1] = INT16_MAX;
	dpatch->rect_dirty[2] = INT16_MIN;
	dpatch->rect_dirty[3] = INT16_MIN;
}

static boolean Patch_CheckDirtyColumns(dynamicpatch_t *dpatch)
{
	patch_t *patch = (patch_t*)dpatch;

	// left
	if (dpatch->column_dirty[0] < 0
	|| dpatch->column_dirty[0] > patch->width
	|| dpatch->column_dirty[0] >= dpatch->column_dirty[1]) // right
		return false;

	// right
	if (dpatch->column_dirty[1] > patch->width
	|| dpatch->column_dirty[1] < 0
	|| dpatch->column_dirty[1] <= dpatch->column_dirty[0]) // left
		return false;

	return true;
}

static void Patch_ClearDirtyColumns(dynamicpatch_t *dpatch)
{
	dpatch->column_dirty[0] = INT16_MAX;
	dpatch->column_dirty[1] = INT16_MIN;
}

static void Patch_InitDynamicColumns(patch_t *patch)
{
	if (patch->columns == NULL)
		patch->columns = Z_Calloc(sizeof(column_t) * patch->width, PU_PATCH_DATA, NULL);

	size_t bpp = Patch_GetBpp(patch);

	for (INT32 x = 0; x < patch->width; x++)
	{
		column_t *column = &patch->columns[x];
		column->pixels = &patch->pixels[patch->height * x * bpp];
	}
}

void Patch_Clear(patch_t *patch)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return;

	size_t total_pixels = patch->width * patch->height;

	memset(patch->pixels, 0, total_pixels * sizeof(UINT8));

	if (patch->columns)
	{
		for (INT32 x = 0; x < patch->width; x++)
			patch->columns[x].num_posts = 0;
	}

	dynamicpatch_t *dpatch = (dynamicpatch_t*)patch;

	if (dpatch->pixels_opaque)
		memset(dpatch->pixels_opaque, 0, BIT_ARRAY_SIZE(total_pixels));

	dpatch->is_dirty = dpatch->update_columns = true;
}

void Patch_ClearRect(patch_t *patch, INT16 x, INT16 y, INT16 width, INT16 height)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return;

	dynamicpatch_t *dpatch = (dynamicpatch_t*)patch;

	for (INT16 dy = y; dy < y + height; dy++)
	{
		if (dy < 0)
			continue;
		else if (dy >= patch->height)
			break;

		for (INT16 dx = x; dx < x + width; dx++)
		{
			if (dx < 0)
				continue;
			else if (dx >= patch->width)
				break;

			size_t position = (dx * patch->height) + dy;
			if (patch->format == PATCH_FORMAT_RGBA)
			{
				UINT32 *pixels = (UINT32 *)patch->pixels;
				pixels[position] = 0;
			}
			else if (dpatch->pixels_opaque)
			{
				unset_bit_array(dpatch->pixels_opaque, position);
			}

			dpatch->is_dirty = dpatch->update_columns = true;
		}
	}
}

patch_t *Patch_CreateFromDoomPatch(softwarepatch_t *source)
{
	patch_t *patch = (patch_t*)Patch_Create(0, 0);
	if (!source)
		return patch;

	patch->width      = SHORT(source->width);
	patch->height     = SHORT(source->height);
	patch->leftoffset = SHORT(source->leftoffset);
	patch->topoffset  = SHORT(source->topoffset);

	size_t total_pixels = 0;
	size_t total_posts = 0;

	Patch_CalcDataSizes(source, &total_pixels, &total_posts);

	int width_po2 = 1;
	while (width_po2 < patch->width)
		width_po2 <<= 1;
	patch->width_mask = width_po2 - 1;

	patch->columns = Z_Calloc(sizeof(column_t) * patch->width, PU_PATCH_DATA, NULL);
	patch->pixels = Z_Calloc(sizeof(UINT8) * total_pixels, PU_PATCH_DATA, NULL);

	staticpatch_t *spatch = (staticpatch_t*)patch;
	spatch->posts = Z_Calloc(sizeof(post_t) * total_posts, PU_PATCH_DATA, NULL);

	Patch_MakeColumns(source, patch->width, patch->width, patch->pixels, patch->columns, spatch->posts, false);

	return patch;
}

void Patch_CalcDataSizes(softwarepatch_t *source, size_t *total_pixels, size_t *total_posts)
{
	for (INT32 i = 0; i < source->width; i++)
	{
		doompost_t *src_posts = (doompost_t*)((UINT8 *)source + LONG(source->columnofs[i]));
		for (doompost_t *post = src_posts; post->topdelta != 0xff ;)
		{
			(*total_posts)++;
			(*total_pixels) += post->length;
			post = (doompost_t *)((UINT8 *)post + post->length + 4);
		}
	}
}

void Patch_MakeColumns(softwarepatch_t *source, size_t num_columns, INT16 width, UINT8 *pixels, column_t *columns, post_t *posts, boolean flip)
{
	column_t *column = flip ? columns + (num_columns - 1) : columns;

	for (size_t i = 0; i < num_columns; i++)
	{
		size_t prevdelta = 0;
		size_t data_offset = 0;

		column->pixels = pixels;
		column->posts = posts;
		column->num_posts = 0;

		if (i >= (unsigned)width)
			continue;

		doompost_t *src_posts = (doompost_t*)((UINT8 *)source + LONG(source->columnofs[i]));

		for (doompost_t *post = src_posts; post->topdelta != 0xff ;)
		{
			size_t topdelta = post->topdelta;
			if (topdelta <= prevdelta)
				topdelta += prevdelta;
			prevdelta = topdelta;

			posts->topdelta = topdelta;
			posts->length = (size_t)post->length;
			posts->data_offset = data_offset;

			memcpy(pixels, (UINT8 *)post + 3, post->length);

			data_offset += posts->length;
			pixels += posts->length;

			column->num_posts++;

			posts++;

			post = (doompost_t *)((UINT8 *)post + post->length + 4);
		}

		if (flip)
			column--;
		else
			column++;
	}
}

//
// Other functions
//

unsigned Patch_GetBpp(patch_t *patch)
{
	if (patch->format == PATCH_FORMAT_RGBA)
		return 4;
	else
		return 1;
}

static void Patch_RebuildColumn(patch_t *patch, INT32 x, bitarray_t *is_opaque)
{
	post_t *post = NULL;
	boolean was_opaque = false;

	column_t *column = &patch->columns[x];
	INT16 height = patch->height;
	size_t bpp = Patch_GetBpp(patch);

	unsigned post_count = column->num_posts;
	column->num_posts = 0;

	for (INT32 y = 0; y < height; y++)
	{
		size_t position = (x * height) + y;

		// End span if we have a transparent pixel
		if (patch->format == PATCH_FORMAT_RGBA)
		{
			UINT32 *pixels = (UINT32 *)patch->pixels;
			if (R_GetRgbaA(pixels[position]) == 0)
			{
				was_opaque = false;
				continue;
			}
		}
		else
		{
			if (!in_bit_array(is_opaque, position))
			{
				was_opaque = false;
				continue;
			}
		}

		if (!was_opaque)
		{
			column->num_posts++;

			if (column->num_posts > post_count)
			{
				column->posts = Z_Realloc(column->posts, sizeof(post_t) * column->num_posts, PU_PATCH_DATA, NULL);
				post_count = column->num_posts;
			}

			post = &column->posts[column->num_posts - 1];
			post->topdelta = (unsigned)y;
			post->length = 0;
			post->data_offset = post->topdelta * bpp;
		}

		was_opaque = true;

		post->length++;
	}
}

column_t *Patch_GetColumn(patch_t *patch, unsigned column)
{
	if (column >= (unsigned)patch->width)
	{
		if (patch->width_mask + 1 == patch->width)
			column &= patch->width_mask;
		else
			column %= patch->width;
	}

	return &patch->columns[column];
}

void *Patch_GetPixel(patch_t *patch, INT32 x, INT32 y)
{
	if (x < 0 || x >= patch->width || y < 0 || y >= patch->height)
		return NULL;

	if (patch->format == PATCH_FORMAT_RGBA)
	{
		// Well, that makes it easy
		size_t position = (x * patch->height) + y;
		return &patch->pixels[position * 4];
	}
	else
	{
		bitarray_t *pixels_opaque = Patch_GetOpaqueRegions(patch);
		if (pixels_opaque)
		{
			size_t position = (x * patch->height) + y;

			if (!in_bit_array(pixels_opaque, position))
				return NULL;

			return &patch->pixels[position];
		}
	}

	if (Patch_NeedsUpdate(patch, true))
		Patch_DoDynamicUpdate(patch, true);

	if (patch->columns == NULL)
		return NULL;

	column_t *column = &patch->columns[x];

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];

		size_t ofs = y - post->topdelta;

		if (y >= (INT32)post->topdelta && ofs < post->length)
		{
			UINT8 *pixels = &column->pixels[post->data_offset];
			return &pixels[ofs];
		}
	}

	return NULL;
}

void Patch_SetPixel(patch_t *patch, void *pixel, pictureformat_t informat, INT32 x, INT32 y, boolean transparent_overwrite)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return;

	if (x < 0 || x >= patch->width || y < 0 || y >= patch->height)
		return;

	INT32 inbpp = Picture_FormatBPP(informat);

	boolean pixel_is_opaque = false;

	if (pixel)
	{
		if (inbpp == PICDEPTH_32BPP)
			pixel_is_opaque = PicFmt_GetAlpha_32bpp(pixel, 0) > 0;
		else if (inbpp == PICDEPTH_16BPP)
			pixel_is_opaque = PicFmt_GetAlpha_16bpp(pixel, 0) > 0;
		else if (inbpp == PICDEPTH_8BPP)
			pixel_is_opaque = PicFmt_GetAlpha_8bpp(pixel, 0) > 0;
	}

	if (!pixel_is_opaque && !transparent_overwrite)
		return;

	void *(*writePixelFunc)(void *, void *) = NULL;

	if (patch->format == PATCH_FORMAT_RGBA)
	{
		if (inbpp == PICDEPTH_32BPP)
			writePixelFunc = PicFmt_WritePixel_i32o32;
		else if (inbpp == PICDEPTH_16BPP)
			writePixelFunc = PicFmt_WritePixel_i16o32;
		else if (inbpp == PICDEPTH_8BPP)
			writePixelFunc = PicFmt_WritePixel_i8o32;
	}
	else
	{
		if (inbpp == PICDEPTH_32BPP)
			writePixelFunc = PicFmt_WritePixel_i32o8;
		else if (inbpp == PICDEPTH_16BPP)
			writePixelFunc = PicFmt_WritePixel_i16o8;
		else if (inbpp == PICDEPTH_8BPP)
			writePixelFunc = PicFmt_WritePixel_i8o8;
	}

	// If the patch is empty
	if (!patch->pixels)
	{
		if (!pixel_is_opaque)
		{
			// If the pixel is transparent, do nothing
			return;
		}

		if (patch->pixels == NULL)
			patch->pixels = Z_Calloc(patch->width * patch->height * Patch_GetBpp(patch), PU_PATCH_DATA, NULL);
	}

	dynamicpatch_t *dpatch = (dynamicpatch_t *)patch;

	size_t position = (x * patch->height) + y;

	if (patch->format == PATCH_FORMAT_RGBA)
	{
		RGBA_t *dest = (RGBA_t*)(&patch->pixels[position * 4]);

		if (!pixel_is_opaque)
		{
			if (transparent_overwrite)
			{
				dest->s.alpha = 0;
				dpatch->is_dirty = true;

				// No longer a pixel in this position, so columns need to be rebuilt
				dpatch->update_columns = true;
			}
		}
		else
		{
			// No longer a pixel in this position, so columns need to be rebuilt
			if (dest->s.alpha == 0)
				dpatch->update_columns = true;

			writePixelFunc(dest, pixel);

			dpatch->is_dirty = true;
		}
	}
	else
	{
		if (!pixel_is_opaque)
		{
			if (transparent_overwrite)
			{
				unset_bit_array(dpatch->pixels_opaque, position);
				dpatch->is_dirty = true;

				// No longer a pixel in this position, so columns need to be rebuilt
				dpatch->update_columns = true;
			}
		}
		else
		{
			// No longer a pixel in this position, so columns need to be rebuilt
			if (!in_bit_array(dpatch->pixels_opaque, position))
			{
				set_bit_array(dpatch->pixels_opaque, position);
				dpatch->update_columns = true;
			}

			writePixelFunc(&patch->pixels[position], pixel);

			dpatch->is_dirty = true;
		}
	}

	if (dpatch->is_dirty)
		Patch_MarkDirtyRect(patch, x, y, x + 1, y + 1);
}

void Patch_UpdatePixels(patch_t *patch,
	void *pixels, INT32 src_img_width, INT32 src_img_height,
	pictureformat_t informat,
	INT32 sx, INT32 sy, INT32 sw, INT32 sh, INT32 dx, INT32 dy,
	boolean transparent_overwrite)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return;

	if (src_img_width <= 0 || src_img_height <= 0 || sw <= 0 || sh <= 0)
		return;

	if (patch->pixels == NULL)
		patch->pixels = Z_Calloc(patch->width * patch->height * Patch_GetBpp(patch), PU_PATCH_DATA, NULL);

	void *(*readPixelFunc)(void *, pictureformat_t, INT32, INT32, INT32, INT32, pictureflags_t) = NULL;
	UINT8 (*getAlphaFunc)(void *, pictureflags_t) = NULL;
	void *(*writePixelFunc)(void *, void *) = NULL;

	if (Picture_IsPatchFormat(informat))
		readPixelFunc = PicFmt_ReadPixel_Patch;
	else if (Picture_IsFlatFormat(informat))
	{
		switch (informat)
		{
			case PICFMT_FLAT32:
				readPixelFunc = PicFmt_ReadPixel_Flat_32bpp;
				break;
			case PICFMT_FLAT16:
				readPixelFunc = PicFmt_ReadPixel_Flat_16bpp;
				break;
			case PICFMT_FLAT:
				readPixelFunc = PicFmt_ReadPixel_Flat_8bpp;
				break;
			default:
				break;
		}
	}

	if (patch->format == PATCH_FORMAT_RGBA)
	{
		switch (Picture_FormatBPP(informat))
		{
		case PICDEPTH_32BPP:
			getAlphaFunc = PicFmt_GetAlpha_32bpp;
			writePixelFunc = PicFmt_WritePixel_i32o32;
			break;
		case PICDEPTH_16BPP:
			getAlphaFunc = PicFmt_GetAlpha_16bpp;
			writePixelFunc = PicFmt_WritePixel_i16o32;
			break;
		case PICDEPTH_8BPP:
			getAlphaFunc = PicFmt_GetAlpha_8bpp;
			writePixelFunc = PicFmt_WritePixel_i8o32;
			break;
		}
	}
	else
	{
		switch (Picture_FormatBPP(informat))
		{
		case PICDEPTH_32BPP:
			getAlphaFunc = PicFmt_GetAlpha_32bpp;
			writePixelFunc = PicFmt_WritePixel_i32o8;
			break;
		case PICDEPTH_16BPP:
			getAlphaFunc = PicFmt_GetAlpha_16bpp;
			writePixelFunc = PicFmt_WritePixel_i16o8;
			break;
		case PICDEPTH_8BPP:
			getAlphaFunc = PicFmt_GetAlpha_8bpp;
			writePixelFunc = PicFmt_WritePixel_i8o8;
			break;
		}
	}

	if (readPixelFunc == NULL || writePixelFunc == NULL || getAlphaFunc == NULL)
		I_Error("Patch_UpdatePixels: unsupported input format");

	dynamicpatch_t *dpatch = (dynamicpatch_t *)patch;

	Patch_MarkDirtyRect(patch, dx, dy, dx + sw, dy + sh);

	if (patch->format == PATCH_FORMAT_RGBA)
	{
		for (INT32 y = dy; y < dy + sh; y++, sy++)
		{
			if (y < 0 || sy < 0)
				continue;
			else if (y >= patch->height || sy >= src_img_height)
				break;

			INT32 src_x = sx;

			for (INT32 x = dx; x < dx + sw; x++, src_x++)
			{
				if (x < 0 || src_x < 0)
					continue;
				else if (x >= patch->width || src_x >= src_img_width)
					break;

				boolean opaque = false;

				// Read pixel
				void *input = readPixelFunc(pixels, informat, src_x, sy, src_img_width, src_img_height, 0);

				// Determine opacity
				if (input != NULL)
					opaque = getAlphaFunc(input, 0) > 0;

				size_t position = (x * patch->height) + y;

				RGBA_t *dest = (RGBA_t*)(&patch->pixels[position * 4]);

				if (!opaque)
				{
					if (transparent_overwrite)
					{
						dest->s.alpha = 0;
						dpatch->update_columns = dpatch->is_dirty = true;
					}
				}
				else
				{
					if (dest->s.alpha == 0)
						dpatch->update_columns = true;

					writePixelFunc(dest, input);
					dpatch->is_dirty = true;
				}
			}
		}
	}
	else
	{
		for (INT32 x = dx; x < dx + sw; x++, sx++)
		{
			if (x < 0 || sx < 0)
				continue;
			else if (x >= patch->width || sx >= src_img_width)
				break;

			INT32 src_y = sy;

			for (INT32 y = dy; y < dy + sh; y++, src_y++)
			{
				if (y < 0 || src_y < 0)
					continue;
				else if (y >= patch->height || src_y >= src_img_height)
					break;

				boolean opaque = false;

				// Read pixel
				void *input = readPixelFunc(pixels, informat, sx, src_y, src_img_width, src_img_height, 0);

				// Determine opacity
				if (input != NULL)
					opaque = getAlphaFunc(input, 0) > 0;

				size_t position = (x * patch->height) + y;

				if (!opaque)
				{
					if (transparent_overwrite)
					{
						unset_bit_array(dpatch->pixels_opaque, position);
						dpatch->update_columns = dpatch->is_dirty = true;
					}
				}
				else
				{
					if (!in_bit_array(dpatch->pixels_opaque, position))
					{
						set_bit_array(dpatch->pixels_opaque, position);
						dpatch->update_columns = true;
					}

					writePixelFunc(&patch->pixels[position], input);

					dpatch->is_dirty = true;
				}
			}
		}
	}
}

boolean Patch_NeedsUpdate(patch_t *patch, boolean needs_columns)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return false;

	dynamicpatch_t *dpatch = (dynamicpatch_t *)patch;
	return dpatch->is_dirty || (needs_columns && dpatch->update_columns);
}

void Patch_DoDynamicUpdate(patch_t *patch, boolean update_columns)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return;

	dynamicpatch_t *dpatch = (dynamicpatch_t *)patch;

	if (Patch_CheckDirtyRect(dpatch))
	{
#ifdef HWRENDER
		if (patch->hardware)
			HWR_UpdatePatchRegion(patch, dpatch->rect_dirty[0], dpatch->rect_dirty[1], dpatch->rect_dirty[2], dpatch->rect_dirty[3]);
#endif
	}

	if (update_columns && dpatch->update_columns && Patch_CheckDirtyColumns(dpatch))
	{
		if (patch->columns == NULL)
			Patch_InitDynamicColumns(patch);

		for (INT32 x = dpatch->column_dirty[0]; x < dpatch->column_dirty[1]; x++)
			Patch_RebuildColumn(patch, x, dpatch->pixels_opaque);

		dpatch->update_columns = false;

		Patch_ClearDirtyColumns(dpatch);
	}

	Patch_FreeMiscData(patch);

	dpatch->is_dirty = false;

	Patch_ClearDirtyRect(dpatch);
}

bitarray_t *Patch_GetOpaqueRegions(patch_t *patch)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return NULL;

	dynamicpatch_t *dpatch = (dynamicpatch_t *)patch;
	return dpatch->pixels_opaque;
}

//
// Frees a patch from memory.
//

void Patch_FreeMiscData(patch_t *patch)
{
	for (INT32 i = 0; i < 4; i++)
	{
		Z_Free(patch->flats[i]);
		patch->flats[i] = NULL;
	}

#ifdef ROTSPRITE
	if (patch->rotated)
	{
		rotsprite_t *rotsprite = patch->rotated;

		for (INT32 i = 0; i < rotsprite->angles; i++)
			Patch_Free(rotsprite->patches[i]);

		Z_Free(rotsprite->patches);
		Z_Free(rotsprite);

		patch->rotated = NULL;
	}
#endif
}

static void Patch_FreeData(patch_t *patch)
{
#ifdef HWRENDER
	if (patch->hardware)
		HWR_FreeTexture(patch);
#endif

	Patch_FreeMiscData(patch);

	if (patch->type == PATCH_TYPE_STATIC)
	{
		staticpatch_t *spatch = (staticpatch_t*)patch;
		Z_Free(spatch->posts);
	}
	else if (patch->type == PATCH_TYPE_DYNAMIC && patch->format == PATCH_FORMAT_PALETTE)
	{
		dynamicpatch_t *dpatch = (dynamicpatch_t*)patch;
		for (INT32 x = 0; x < dpatch->patch.width; x++)
			Z_Free(dpatch->patch.columns[x].posts);
		Z_Free(dpatch->pixels_opaque);
	}

	Z_Free(patch->pixels);
	Z_Free(patch->columns);
}

void Patch_Free(patch_t *patch)
{
	if (patch == NULL)
		return;

	Patch_FreeData(patch);
	Z_Free(patch);
}

//
// Frees patches with a tag range.
//

static boolean Patch_FreeTagsCallback(void *mem)
{
	patch_t *patch = (patch_t *)mem;
	Patch_FreeData(patch);
	return true;
}

void Patch_FreeTags(INT32 lowtag, INT32 hightag)
{
	Z_IterateTags(lowtag, hightag, Patch_FreeTagsCallback);
}

void Patch_GenerateFlat(patch_t *patch, pictureflags_t flags)
{
	UINT8 flip = (flags & (PICFLAGS_XFLIP | PICFLAGS_YFLIP));
	if (patch->flats[flip] == NULL)
	{
		if (patch->format == PATCH_FORMAT_RGBA)
			patch->flats[flip] = Picture_Convert(PICFMT_PATCH32, patch->pixels, PICFMT_FLAT32, 0, NULL, 0, 0, 0, 0, flags);
		else
			patch->flats[flip] = Picture_Convert(PICFMT_PATCH, patch, PICFMT_FLAT16, 0, NULL, 0, 0, 0, 0, flags);
	}
}

#ifdef HWRENDER
//
// Allocates a hardware patch.
//

void *Patch_AllocateHardwarePatch(patch_t *patch)
{
	if (!patch->hardware)
	{
		GLPatch_t *grPatch = Z_Calloc(sizeof(GLPatch_t), PU_HWRPATCHINFO, &patch->hardware);
		grPatch->mipmap = Z_Calloc(sizeof(GLMipmap_t), PU_HWRPATCHINFO, &grPatch->mipmap);
	}
	return (void *)(patch->hardware);
}

//
// Creates a hardware patch.
//

void *Patch_CreateGL(patch_t *patch)
{
	GLPatch_t *grPatch = (GLPatch_t *)Patch_AllocateHardwarePatch(patch);
	if (!grPatch->mipmap->data) // Run HWR_MakePatch in all cases, to recalculate some things
		HWR_MakePatch(patch, grPatch, grPatch->mipmap, false);
	return grPatch;
}
#endif // HWRENDER
