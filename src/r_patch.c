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

//
// Creates a patch.
//

patch_t *Patch_Create(INT16 width, INT16 height)
{
	patch_t *patch = Z_Calloc(sizeof(patch_t), PU_PATCH, NULL);

	patch->width = width;
	patch->height = height;
	patch->type = PATCH_TYPE_STATIC;

	return patch;
}

patch_t *Patch_CreateDynamic(INT16 width, INT16 height)
{
	patch_t *patch = Z_Calloc(sizeof(patch_t), PU_PATCH, NULL);

	patch->width = width;
	patch->height = height;
	patch->type = PATCH_TYPE_DYNAMIC;

	return patch;
}

patch_t *Patch_CreateFromDoomPatch(softwarepatch_t *source)
{
	patch_t *patch = Patch_Create(0, 0);
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
	patch->posts = Z_Calloc(sizeof(post_t) * total_posts, PU_PATCH_DATA, NULL);
	patch->pixels = Z_Calloc(sizeof(UINT8) * total_pixels, PU_PATCH_DATA, NULL);

	Patch_MakeColumns(source, patch->width, patch->width, patch->pixels, patch->columns, patch->posts, false);

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

static void Patch_RebuildColumn(column_t *column, INT16 height, UINT8 *is_opaque)
{
	post_t *post;
	boolean was_opaque = false;

	unsigned post_count = column->num_posts;
	column->num_posts = 0;

	for (INT32 y = 0; y < height; y++)
	{
		// End span if we have a transparent pixel
		if (!is_opaque[y])
		{
			was_opaque = false;
			continue;
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
			post->data_offset = post->topdelta;
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
	if (x < 0 || x >= patch->width || patch->columns == NULL)
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
	if (inbpp == PICDEPTH_32BPP)
		writePixelFunc = PicFmt_WritePixel_i32o8;
	else if (inbpp == PICDEPTH_16BPP)
		writePixelFunc = PicFmt_WritePixel_i16o8;
	else if (inbpp == PICDEPTH_8BPP)
		writePixelFunc = PicFmt_WritePixel_i8o8;

	column_t *column = NULL;

	boolean did_update_column = false;

	// If the patch is empty
	if (!patch->columns)
	{
		if (!pixel_is_opaque)
		{
			// If the pixel is transparent, do nothing
			return;
		}

		if (patch->pixels == NULL)
			patch->pixels = Z_Calloc(patch->width * patch->height, PU_PATCH_DATA, NULL);
		if (patch->columns == NULL)
			patch->columns = Z_Calloc(sizeof(column_t) * patch->width, PU_PATCH_DATA, NULL);

		column = &patch->columns[x];
		column->pixels = &patch->pixels[patch->height * x];
	}
	else
	{
		column = &patch->columns[x];
		column->pixels = &patch->pixels[patch->height * x];

		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];

			int this_topdelta = (int)post->topdelta;
			int next_topdelta;
			if (i < column->num_posts - 1)
				next_topdelta = column->posts[i + 1].topdelta;
			else
				next_topdelta = patch->height;

			// Handle the case where this is the first post, and the pixel is before it
			if (i == 0 && y < this_topdelta)
			{
				if (!pixel_is_opaque)
				{
					// If the pixel is transparent, ignore
					continue;
				}

				column->posts = Z_Realloc(column->posts, sizeof(post_t) * (column->num_posts + 1), PU_PATCH_DATA, NULL);

				memmove(&column->posts[1], &column->posts[0], column->num_posts * sizeof(post_t));

				column->num_posts++;

				post_t *dest_post = &column->posts[0];
				dest_post->topdelta = (unsigned)y;
				dest_post->length = 1;
				dest_post->data_offset = dest_post->topdelta;

				writePixelFunc(&column->pixels[dest_post->data_offset], pixel);

				did_update_column = true;

				break;
			}
			// Pixel is inside a post
			else if (y >= this_topdelta && y < this_topdelta + (signed)post->length)
			{
				// Handle the case where the pixel needs to be removed
				if (!pixel_is_opaque)
				{
					if (!transparent_overwrite)
						continue;

					int resulting_post_length = y - post->topdelta;
					if ((resulting_post_length == (signed)post->length - 1 && i == column->num_posts - 1)
					|| (resulting_post_length == 0))
					{
						if (resulting_post_length == 0)
							post->topdelta++;
						post->length--;
						if (post->length == 0)
						{
							if (column->num_posts > 1 && i < column->num_posts - 1)
								memmove(&column->posts[i], &column->posts[i + 1], (column->num_posts - i) * sizeof(post_t));
							column->num_posts--;
						}
					}
					else
					{
						column->num_posts++;
						column->posts = Z_Realloc(column->posts, sizeof(post_t) * column->num_posts, PU_PATCH_DATA, NULL);

						if (i != column->num_posts)
							memmove(&column->posts[i + 1], &column->posts[i], ((column->num_posts - 1) - i) * sizeof(post_t));

						column->posts[i + 1].length = column->posts[i].length - resulting_post_length - 1;
						column->posts[i + 1].topdelta = y + 1;
						column->posts[i + 1].data_offset = column->posts[i + 1].topdelta;
						column->posts[i].length = resulting_post_length;
					}
				}
				else
					writePixelFunc(&column->pixels[post->data_offset + (y - post->topdelta)], pixel);

				did_update_column = true;

				break;
			}
			// After this post, but before the next one
			else if (y >= this_topdelta + (signed)post->length && y < next_topdelta)
			{
				if (!pixel_is_opaque)
				{
					// If the pixel is transparent, ignore
					continue;
				}

				post_t *dest_post = NULL;

				column->posts = Z_Realloc(column->posts, sizeof(post_t) * (column->num_posts + 1), PU_PATCH_DATA, NULL);

				if (i == column->num_posts - 1)
					dest_post = &column->posts[column->num_posts];
				else
				{
					memmove(&column->posts[i + 1], &column->posts[i], (column->num_posts - i) * sizeof(post_t));
					dest_post = &column->posts[i];
				}

				column->num_posts++;

				dest_post->topdelta = (unsigned)y;
				dest_post->length = 1;
				dest_post->data_offset = dest_post->topdelta;

				writePixelFunc(&column->pixels[dest_post->data_offset], pixel);

				did_update_column = true;

				break;
			}
		}
	}

	if (!did_update_column && column && column->num_posts == 0)
	{
		if (!pixel_is_opaque)
		{
			// If the pixel is transparent, do nothing
			return;
		}

		column->num_posts = 1;
		column->posts = Z_Realloc(column->posts, sizeof(post_t) * column->num_posts, PU_PATCH_DATA, NULL);

		post_t *post = &column->posts[0];
		post->topdelta = (unsigned)y;
		post->length = 1;
		post->data_offset = post->topdelta;

		writePixelFunc(&column->pixels[post->data_offset], pixel);

		did_update_column = true;
	}

	if (did_update_column)
	{
		Patch_FreeMiscData(patch);

#ifdef HWRENDER
		if (patch->hardware)
			HWR_UpdatePatch(patch);
#endif
	}
}

void Patch_Update(patch_t *patch,
	void *pixels, INT32 src_img_width, INT32 src_img_height,
	pictureformat_t informat,
	INT32 sx, INT32 sy, INT32 sw, INT32 sh, INT32 dx, INT32 dy,
	boolean transparent_overwrite)
{
	if (patch->type != PATCH_TYPE_DYNAMIC)
		return;

	if (src_img_width <= 0 || src_img_height <= 0 || sw <= 0 || sh <= 0)
		return;

	boolean did_update = false;

	if (patch->columns == NULL)
		patch->columns = Z_Calloc(sizeof(column_t) * patch->width, PU_PATCH_DATA, NULL);
	if (patch->pixels == NULL)
		patch->pixels = Z_Calloc(patch->width * patch->height, PU_PATCH_DATA, NULL);

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

	if (readPixelFunc == NULL)
		I_Error("Patch_Update: unsupported input format");

	INT32 inbpp = Picture_FormatBPP(informat);

	if (inbpp == PICDEPTH_32BPP)
	{
		getAlphaFunc = PicFmt_GetAlpha_32bpp;
		writePixelFunc = PicFmt_WritePixel_i32o8;
	}
	else if (inbpp == PICDEPTH_16BPP)
	{
		getAlphaFunc = PicFmt_GetAlpha_16bpp;
		writePixelFunc = PicFmt_WritePixel_i16o8;
	}
	else if (inbpp == PICDEPTH_8BPP)
	{
		getAlphaFunc = PicFmt_GetAlpha_8bpp;
		writePixelFunc = PicFmt_WritePixel_i8o8;
	}

	UINT8 *is_opaque = malloc(patch->height * sizeof(UINT8));
	if (!is_opaque)
	{
		abort();
		return;
	}

	// Write columns
	for (INT32 x = dx; x < dx + sw; x++, sx++)
	{
		if (x < 0 || sx < 0)
			continue;
		else if (x >= patch->width || sx >= src_img_width)
			break;

		column_t *column = &patch->columns[x];
		column->pixels = &patch->pixels[patch->height * x];

		memset(is_opaque, 0, patch->height * sizeof(UINT8));

		for (unsigned i = 0; i < column->num_posts; i++)
		{
			post_t *post = &column->posts[i];
			memset(&is_opaque[post->topdelta], 1, post->length);
		}

		boolean did_update_column = false;

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

			if (!opaque)
			{
				if (transparent_overwrite)
				{
					is_opaque[y] = false;
					did_update = did_update_column = true;
				}
			}
			else
			{
				did_update = did_update_column = true;
				is_opaque[y] = true;
				writePixelFunc(&column->pixels[y], input);
			}
		}

		if (!did_update_column)
			continue;

		Patch_RebuildColumn(column, patch->height, is_opaque);
	}

	if (did_update)
	{
		Patch_FreeMiscData(patch);

#ifdef HWRENDER
		if (patch->hardware)
			HWR_UpdatePatch(patch);
#endif
	}

	free(is_opaque);
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

	if (patch->type == PATCH_TYPE_DYNAMIC)
	{
		for (INT32 x = 0; x < patch->width; x++)
			Z_Free(patch->columns[x].posts);
	}

	if (patch->pixels)
		Z_Free(patch->pixels);
	if (patch->columns)
		Z_Free(patch->columns);
	if (patch->posts)
		Z_Free(patch->posts);
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
		patch->flats[flip] = Picture_Convert(PICFMT_PATCH, patch, PICFMT_FLAT16, 0, NULL, 0, 0, 0, 0, flags);
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
