// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief MD2 Handling
///	Inspired from md2.c by Mete Ciragan (mete@swissquake.ch)


#ifdef __GNUC__
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../doomdef.h"
#include "../doomstat.h"
#include "../fastcmp.h"

#ifdef HWRENDER
#include "hw_drv.h"
#include "hw_light.h"
#include "hw_md2.h"
#include "../r_bsp.h"
#include "../r_main.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../r_things.h"

#include "hw_main.h"
#include "../v_video.h"
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
 #if PNG_LIBPNG_VER < 100207
 //#undef HAVE_PNG
 #endif
#endif

#define NUMVERTEXNORMALS 162
float avertexnormals[NUMVERTEXNORMALS][3] = {
{-0.525731f, 0.000000f, 0.850651f},
{-0.442863f, 0.238856f, 0.864188f},
{-0.295242f, 0.000000f, 0.955423f},
{-0.309017f, 0.500000f, 0.809017f},
{-0.162460f, 0.262866f, 0.951056f},
{0.000000f, 0.000000f, 1.000000f},
{0.000000f, 0.850651f, 0.525731f},
{-0.147621f, 0.716567f, 0.681718f},
{0.147621f, 0.716567f, 0.681718f},
{0.000000f, 0.525731f, 0.850651f},
{0.309017f, 0.500000f, 0.809017f},
{0.525731f, 0.000000f, 0.850651f},
{0.295242f, 0.000000f, 0.955423f},
{0.442863f, 0.238856f, 0.864188f},
{0.162460f, 0.262866f, 0.951056f},
{-0.681718f, 0.147621f, 0.716567f},
{-0.809017f, 0.309017f, 0.500000f},
{-0.587785f, 0.425325f, 0.688191f},
{-0.850651f, 0.525731f, 0.000000f},
{-0.864188f, 0.442863f, 0.238856f},
{-0.716567f, 0.681718f, 0.147621f},
{-0.688191f, 0.587785f, 0.425325f},
{-0.500000f, 0.809017f, 0.309017f},
{-0.238856f, 0.864188f, 0.442863f},
{-0.425325f, 0.688191f, 0.587785f},
{-0.716567f, 0.681718f, -0.147621f},
{-0.500000f, 0.809017f, -0.309017f},
{-0.525731f, 0.850651f, 0.000000f},
{0.000000f, 0.850651f, -0.525731f},
{-0.238856f, 0.864188f, -0.442863f},
{0.000000f, 0.955423f, -0.295242f},
{-0.262866f, 0.951056f, -0.162460f},
{0.000000f, 1.000000f, 0.000000f},
{0.000000f, 0.955423f, 0.295242f},
{-0.262866f, 0.951056f, 0.162460f},
{0.238856f, 0.864188f, 0.442863f},
{0.262866f, 0.951056f, 0.162460f},
{0.500000f, 0.809017f, 0.309017f},
{0.238856f, 0.864188f, -0.442863f},
{0.262866f, 0.951056f, -0.162460f},
{0.500000f, 0.809017f, -0.309017f},
{0.850651f, 0.525731f, 0.000000f},
{0.716567f, 0.681718f, 0.147621f},
{0.716567f, 0.681718f, -0.147621f},
{0.525731f, 0.850651f, 0.000000f},
{0.425325f, 0.688191f, 0.587785f},
{0.864188f, 0.442863f, 0.238856f},
{0.688191f, 0.587785f, 0.425325f},
{0.809017f, 0.309017f, 0.500000f},
{0.681718f, 0.147621f, 0.716567f},
{0.587785f, 0.425325f, 0.688191f},
{0.955423f, 0.295242f, 0.000000f},
{1.000000f, 0.000000f, 0.000000f},
{0.951056f, 0.162460f, 0.262866f},
{0.850651f, -0.525731f, 0.000000f},
{0.955423f, -0.295242f, 0.000000f},
{0.864188f, -0.442863f, 0.238856f},
{0.951056f, -0.162460f, 0.262866f},
{0.809017f, -0.309017f, 0.500000f},
{0.681718f, -0.147621f, 0.716567f},
{0.850651f, 0.000000f, 0.525731f},
{0.864188f, 0.442863f, -0.238856f},
{0.809017f, 0.309017f, -0.500000f},
{0.951056f, 0.162460f, -0.262866f},
{0.525731f, 0.000000f, -0.850651f},
{0.681718f, 0.147621f, -0.716567f},
{0.681718f, -0.147621f, -0.716567f},
{0.850651f, 0.000000f, -0.525731f},
{0.809017f, -0.309017f, -0.500000f},
{0.864188f, -0.442863f, -0.238856f},
{0.951056f, -0.162460f, -0.262866f},
{0.147621f, 0.716567f, -0.681718f},
{0.309017f, 0.500000f, -0.809017f},
{0.425325f, 0.688191f, -0.587785f},
{0.442863f, 0.238856f, -0.864188f},
{0.587785f, 0.425325f, -0.688191f},
{0.688191f, 0.587785f, -0.425325f},
{-0.147621f, 0.716567f, -0.681718f},
{-0.309017f, 0.500000f, -0.809017f},
{0.000000f, 0.525731f, -0.850651f},
{-0.525731f, 0.000000f, -0.850651f},
{-0.442863f, 0.238856f, -0.864188f},
{-0.295242f, 0.000000f, -0.955423f},
{-0.162460f, 0.262866f, -0.951056f},
{0.000000f, 0.000000f, -1.000000f},
{0.295242f, 0.000000f, -0.955423f},
{0.162460f, 0.262866f, -0.951056f},
{-0.442863f, -0.238856f, -0.864188f},
{-0.309017f, -0.500000f, -0.809017f},
{-0.162460f, -0.262866f, -0.951056f},
{0.000000f, -0.850651f, -0.525731f},
{-0.147621f, -0.716567f, -0.681718f},
{0.147621f, -0.716567f, -0.681718f},
{0.000000f, -0.525731f, -0.850651f},
{0.309017f, -0.500000f, -0.809017f},
{0.442863f, -0.238856f, -0.864188f},
{0.162460f, -0.262866f, -0.951056f},
{0.238856f, -0.864188f, -0.442863f},
{0.500000f, -0.809017f, -0.309017f},
{0.425325f, -0.688191f, -0.587785f},
{0.716567f, -0.681718f, -0.147621f},
{0.688191f, -0.587785f, -0.425325f},
{0.587785f, -0.425325f, -0.688191f},
{0.000000f, -0.955423f, -0.295242f},
{0.000000f, -1.000000f, 0.000000f},
{0.262866f, -0.951056f, -0.162460f},
{0.000000f, -0.850651f, 0.525731f},
{0.000000f, -0.955423f, 0.295242f},
{0.238856f, -0.864188f, 0.442863f},
{0.262866f, -0.951056f, 0.162460f},
{0.500000f, -0.809017f, 0.309017f},
{0.716567f, -0.681718f, 0.147621f},
{0.525731f, -0.850651f, 0.000000f},
{-0.238856f, -0.864188f, -0.442863f},
{-0.500000f, -0.809017f, -0.309017f},
{-0.262866f, -0.951056f, -0.162460f},
{-0.850651f, -0.525731f, 0.000000f},
{-0.716567f, -0.681718f, -0.147621f},
{-0.716567f, -0.681718f, 0.147621f},
{-0.525731f, -0.850651f, 0.000000f},
{-0.500000f, -0.809017f, 0.309017f},
{-0.238856f, -0.864188f, 0.442863f},
{-0.262866f, -0.951056f, 0.162460f},
{-0.864188f, -0.442863f, 0.238856f},
{-0.809017f, -0.309017f, 0.500000f},
{-0.688191f, -0.587785f, 0.425325f},
{-0.681718f, -0.147621f, 0.716567f},
{-0.442863f, -0.238856f, 0.864188f},
{-0.587785f, -0.425325f, 0.688191f},
{-0.309017f, -0.500000f, 0.809017f},
{-0.147621f, -0.716567f, 0.681718f},
{-0.425325f, -0.688191f, 0.587785f},
{-0.162460f, -0.262866f, 0.951056f},
{0.442863f, -0.238856f, 0.864188f},
{0.162460f, -0.262866f, 0.951056f},
{0.309017f, -0.500000f, 0.809017f},
{0.147621f, -0.716567f, 0.681718f},
{0.000000f, -0.525731f, 0.850651f},
{0.425325f, -0.688191f, 0.587785f},
{0.587785f, -0.425325f, 0.688191f},
{0.688191f, -0.587785f, 0.425325f},
{-0.955423f, 0.295242f, 0.000000f},
{-0.951056f, 0.162460f, 0.262866f},
{-1.000000f, 0.000000f, 0.000000f},
{-0.850651f, 0.000000f, 0.525731f},
{-0.955423f, -0.295242f, 0.000000f},
{-0.951056f, -0.162460f, 0.262866f},
{-0.864188f, 0.442863f, -0.238856f},
{-0.951056f, 0.162460f, -0.262866f},
{-0.809017f, 0.309017f, -0.500000f},
{-0.864188f, -0.442863f, -0.238856f},
{-0.951056f, -0.162460f, -0.262866f},
{-0.809017f, -0.309017f, -0.500000f},
{-0.681718f, 0.147621f, -0.716567f},
{-0.681718f, -0.147621f, -0.716567f},
{-0.850651f, 0.000000f, -0.525731f},
{-0.688191f, 0.587785f, -0.425325f},
{-0.587785f, 0.425325f, -0.688191f},
{-0.425325f, 0.688191f, -0.587785f},
{-0.425325f, -0.688191f, -0.587785f},
{-0.587785f, -0.425325f, -0.688191f},
{-0.688191f, -0.587785f, -0.425325f},
};

md2_t md2_models[NUMSPRITES];
md2_t md2_playermodels[MAXSKINS];


/*
 * free model
 */
static void md2_freeModel (md2_model_t *model)
{
	if (model)
	{
		if (model->skins)
			free(model->skins);

		if (model->texCoords)
			free(model->texCoords);

		if (model->triangles)
			free(model->triangles);

		if (model->frames)
		{
			size_t i;

			for (i = 0; i < model->header.numFrames; i++)
			{
				if (model->frames[i].vertices)
					free(model->frames[i].vertices);
			}
			free(model->frames);
		}

		if (model->spr2frames)
			free(model->spr2frames);

		if (model->glCommandBuffer)
			free(model->glCommandBuffer);

		free(model);
	}
}


//
// load model
//
// Hurdler: the current path is the Legacy.exe path
static md2_model_t *md2_readModel(const char *filename)
{
	FILE *file;
	md2_model_t *model;
	UINT8 buffer[MD2_MAX_FRAMESIZE];
	size_t i;

	model = calloc(1, sizeof (*model));
	if (model == NULL)
		return 0;

	file = fopen(filename, "rb");
	if (!file)
	{
		free(model);
		return 0;
	}

	// initialize model and read header

	if (fread(&model->header, sizeof (model->header), 1, file) != 1
		|| model->header.magic != MD2_IDENT
		|| model->header.version != MD2_VERSION)
	{
		fclose(file);
		free(model);
		return 0;
	}

	model->header.numSkins = 1;

#define MD2LIMITCHECK(field, max, msgname) \
	if (field > max) \
	{ \
		CONS_Alert(CONS_ERROR, "md2_readModel: %s has too many " msgname " (# found: %d, maximum: %d)\n", filename, field, max); \
		md2_freeModel (model); \
		fclose(file); \
		return 0; \
	}

	// Uncomment if these are actually needed
//	MD2LIMITCHECK(model->header.numSkins,     MD2_MAX_SKINS,     "skins")
//	MD2LIMITCHECK(model->header.numTexCoords, MD2_MAX_TEXCOORDS, "texture coordinates")
	MD2LIMITCHECK(model->header.numTriangles, MD2_MAX_TRIANGLES, "triangles")
	MD2LIMITCHECK(model->header.numFrames,    MD2_MAX_FRAMES,    "frames")
	MD2LIMITCHECK(model->header.numVertices,  MD2_MAX_VERTICES,  "vertices")

#undef MD2LIMITCHECK

	// read skins
	fseek(file, model->header.offsetSkins, SEEK_SET);
	if (model->header.numSkins > 0)
	{
		model->skins = calloc(sizeof (md2_skin_t), model->header.numSkins);
		if (!model->skins || model->header.numSkins !=
			fread(model->skins, sizeof (md2_skin_t), model->header.numSkins, file))
		{
			md2_freeModel (model);
			fclose(file);
			return 0;
		}
	}

	// read texture coordinates
	fseek(file, model->header.offsetTexCoords, SEEK_SET);
	if (model->header.numTexCoords > 0)
	{
		model->texCoords = calloc(sizeof (md2_textureCoordinate_t), model->header.numTexCoords);
		if (!model->texCoords || model->header.numTexCoords !=
			fread(model->texCoords, sizeof (md2_textureCoordinate_t), model->header.numTexCoords, file))
		{
			md2_freeModel (model);
			fclose(file);
			return 0;
		}
	}

	// read triangles
	fseek(file, model->header.offsetTriangles, SEEK_SET);
	if (model->header.numTriangles > 0)
	{
		model->triangles = calloc(sizeof (md2_triangle_t), model->header.numTriangles);
		if (!model->triangles || model->header.numTriangles !=
			fread(model->triangles, sizeof (md2_triangle_t), model->header.numTriangles, file))
		{
			md2_freeModel (model);
			fclose(file);
			return 0;
		}
	}

	// read alias frames
	fseek(file, model->header.offsetFrames, SEEK_SET);
	if (model->header.numFrames > 0)
	{
		model->frames = calloc(sizeof (md2_frame_t), model->header.numFrames);
		if (!model->frames)
		{
			md2_freeModel (model);
			fclose(file);
			return 0;
		}

		for (i = 0; i < model->header.numFrames; i++)
		{
			md2_alias_frame_t *frame = (md2_alias_frame_t *)(void *)buffer;
			size_t j;

			model->frames[i].vertices = calloc(sizeof (md2_triangleVertex_t), model->header.numVertices);
			if (!model->frames[i].vertices || model->header.frameSize !=
				fread(frame, 1, model->header.frameSize, file))
			{
				md2_freeModel (model);
				fclose(file);
				return 0;
			}

			strcpy(model->frames[i].name, frame->name);
			if (frame->name[0] == 'S')
			{
				boolean super;
				if ((super = (fastncmp("UPER", frame->name+1, 4))) // SUPER
					|| fastncmp("PR2_", frame->name+1, 4)) // SPR2_
				{
					UINT8 spr2;
					for (spr2 = 0; spr2 < free_spr2; spr2++)
						if (fastncmp(frame->name+5,spr2names[spr2],3)
						&& ((frame->name[8] == spr2names[spr2][3])
							|| (frame->name[8] == '.' && spr2names[spr2][3] == '_')))
							break;

					if (spr2 < free_spr2)
					{
						if (!model->spr2frames)
						{
							model->spr2frames = calloc(sizeof (size_t), 2*NUMPLAYERSPRITES*2);
							if (!model->spr2frames)
							{
								md2_freeModel (model);
								fclose(file);
								return 0;
							}
						}
						if (super)
							spr2 |= FF_SPR2SUPER;
						if (model->spr2frames[spr2*2 + 1]++ == 0) // numspr2frames
							model->spr2frames[spr2*2] = i; // starting frame
						CONS_Debug(DBG_RENDER, "frame %s, sprite2 %s - starting frame %s, number of frames %s\n", frame->name, spr2names[spr2 & ~FF_SPR2SUPER], sizeu1(model->spr2frames[spr2*2]), sizeu2(model->spr2frames[spr2*2 + 1]));
					}
				}
			}
			for (j = 0; j < model->header.numVertices; j++)
			{
				model->frames[i].vertices[j].vertex[0] = (float) ((INT32) frame->alias_vertices[j].vertex[0]) * frame->scale[0] + frame->translate[0];
				model->frames[i].vertices[j].vertex[2] = -1* ((float) ((INT32) frame->alias_vertices[j].vertex[1]) * frame->scale[1] + frame->translate[1]);
				model->frames[i].vertices[j].vertex[1] = (float) ((INT32) frame->alias_vertices[j].vertex[2]) * frame->scale[2] + frame->translate[2];
				model->frames[i].vertices[j].normal[0] = avertexnormals[frame->alias_vertices[j].lightNormalIndex][0];
				model->frames[i].vertices[j].normal[1] = avertexnormals[frame->alias_vertices[j].lightNormalIndex][1];
				model->frames[i].vertices[j].normal[2] = avertexnormals[frame->alias_vertices[j].lightNormalIndex][2];
			}
		}
	}

	// read gl commands
	fseek(file, model->header.offsetGlCommands, SEEK_SET);
	if (model->header.numGlCommands)
	{
		model->glCommandBuffer = calloc(sizeof (INT32), model->header.numGlCommands);
		if (!model->glCommandBuffer || model->header.numGlCommands !=
			fread(model->glCommandBuffer, sizeof (INT32), model->header.numGlCommands, file))
		{
			md2_freeModel (model);
			fclose(file);
			return 0;
		}
	}

	fclose(file);

	return model;
}

static inline void md2_printModelInfo (md2_model_t *model)
{
#if 0
	INT32 i;

	CONS_Debug(DBG_RENDER, "magic:\t\t\t%c%c%c%c\n", model->header.magic>>24,
	            (model->header.magic>>16)&0xff,
	            (model->header.magic>>8)&0xff,
	             model->header.magic&0xff);
	CONS_Debug(DBG_RENDER, "version:\t\t%d\n", model->header.version);
	CONS_Debug(DBG_RENDER, "skinWidth:\t\t%d\n", model->header.skinWidth);
	CONS_Debug(DBG_RENDER, "skinHeight:\t\t%d\n", model->header.skinHeight);
	CONS_Debug(DBG_RENDER, "frameSize:\t\t%d\n", model->header.frameSize);
	CONS_Debug(DBG_RENDER, "numSkins:\t\t%d\n", model->header.numSkins);
	CONS_Debug(DBG_RENDER, "numVertices:\t\t%d\n", model->header.numVertices);
	CONS_Debug(DBG_RENDER, "numTexCoords:\t\t%d\n", model->header.numTexCoords);
	CONS_Debug(DBG_RENDER, "numTriangles:\t\t%d\n", model->header.numTriangles);
	CONS_Debug(DBG_RENDER, "numGlCommands:\t\t%d\n", model->header.numGlCommands);
	CONS_Debug(DBG_RENDER, "numFrames:\t\t%d\n", model->header.numFrames);
	CONS_Debug(DBG_RENDER, "offsetSkins:\t\t%d\n", model->header.offsetSkins);
	CONS_Debug(DBG_RENDER, "offsetTexCoords:\t%d\n", model->header.offsetTexCoords);
	CONS_Debug(DBG_RENDER, "offsetTriangles:\t%d\n", model->header.offsetTriangles);
	CONS_Debug(DBG_RENDER, "offsetFrames:\t\t%d\n", model->header.offsetFrames);
	CONS_Debug(DBG_RENDER, "offsetGlCommands:\t%d\n", model->header.offsetGlCommands);
	CONS_Debug(DBG_RENDER, "offsetEnd:\t\t%d\n", model->header.offsetEnd);

	for (i = 0; i < model->header.numFrames; i++)
		CONS_Debug(DBG_RENDER, "%s ", model->frames[i].name);
	CONS_Debug(DBG_RENDER, "\n");
#else
	(void)model;
#endif
}

#ifdef HAVE_PNG
static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);
	//I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Debug(DBG_RENDER, "libpng warning at %p: %s", PNG, pngtext);
}

static GrTextureFormat_t PNG_Load(const char *filename, int *w, int *h, GLPatch_t *grpatch)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;
	char *pngfilename = va("md2/%s", filename);

	FIL_ForceExtension(pngfilename, ".png");
	png_FILE = fopen(pngfilename, "rb");
	if (!png_FILE)
	{
		//CONS_Debug(DBG_RENDER, "M_SavePNG: Error on opening %s for loading\n", filename);
		return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on initialize libpng\n");
		fclose(png_FILE);
		return 0;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Debug(DBG_RENDER, "PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(png_FILE);
		return 0;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Debug(DBG_RENDER, "libpng load error on %s\n", filename);
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		fclose(png_FILE);
		Z_Free(grpatch->mipmap.grInfo.data);
		return 0;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof jmp_buf);
#endif

	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);

	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type,
	 NULL, NULL, NULL);

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

	{
		png_uint_32 i, pitch = png_get_rowbytes(png_ptr, png_info_ptr);
		png_bytep PNG_image = Z_Malloc(pitch*height, PU_HWRCACHE, &grpatch->mipmap.grInfo.data);
		png_bytepp row_pointers = png_malloc(png_ptr, height * sizeof (png_bytep));
		for (i = 0; i < height; i++)
			row_pointers[i] = PNG_image + i*pitch;
		png_read_image(png_ptr, row_pointers);
		png_free(png_ptr, (png_voidp)row_pointers);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	fclose(png_FILE);
	*w = (int)width;
	*h = (int)height;
	return GR_RGBA;
}
#endif

typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bitsPerPixel;
	INT16 xmin;
	INT16 ymin;
	INT16 xmax;
	INT16 ymax;
	INT16 hDpi;
	INT16 vDpi;
	UINT8 colorMap[48];
	UINT8 reserved;
	UINT8 numPlanes;
	INT16 bytesPerLine;
	INT16 paletteInfo;
	INT16 hScreenSize;
	INT16 vScreenSize;
	UINT8 filler[54];
} PcxHeader;

static GrTextureFormat_t PCX_Load(const char *filename, int *w, int *h,
	GLPatch_t *grpatch)
{
	PcxHeader header;
#define PALSIZE 768
	UINT8 palette[PALSIZE];
	const UINT8 *pal;
	RGBA_t *image;
	size_t pw, ph, size, ptr = 0;
	INT32 ch, rep;
	FILE *file;
	char *pcxfilename = va("md2/%s", filename);

	FIL_ForceExtension(pcxfilename, ".pcx");
	file = fopen(pcxfilename, "rb");
	if (!file)
		return 0;

	if (fread(&header, sizeof (PcxHeader), 1, file) != 1)
	{
		fclose(file);
		return 0;
	}

	if (header.bitsPerPixel != 8)
	{
		fclose(file);
		return 0;
	}

	fseek(file, -PALSIZE, SEEK_END);

	pw = *w = header.xmax - header.xmin + 1;
	ph = *h = header.ymax - header.ymin + 1;
	image = Z_Malloc(pw*ph*4, PU_HWRCACHE, &grpatch->mipmap.grInfo.data);

	if (fread(palette, sizeof (UINT8), PALSIZE, file) != PALSIZE)
	{
		Z_Free(image);
		fclose(file);
		return 0;
	}
	fseek(file, sizeof (PcxHeader), SEEK_SET);

	size = pw * ph;
	while (ptr < size)
	{
		ch = fgetc(file);  //Hurdler: beurk
		if (ch >= 192)
		{
			rep = ch - 192;
			ch = fgetc(file);
		}
		else
		{
			rep = 1;
		}
		while (rep--)
		{
			pal = palette + ch*3;
			image[ptr].s.red   = *pal++;
			image[ptr].s.green = *pal++;
			image[ptr].s.blue  = *pal++;
			image[ptr].s.alpha = 0xFF;
			ptr++;
		}
	}
	fclose(file);
	return GR_RGBA;
}

// -----------------+
// md2_loadTexture  : Download a pcx or png texture for MD2 models
// -----------------+
static void md2_loadTexture(md2_t *model)
{
	GLPatch_t *grpatch;
	const char *filename = model->filename;

	if (model->grpatch)
	{
		grpatch = model->grpatch;
		Z_Free(grpatch->mipmap.grInfo.data);
	}
	else
		grpatch = Z_Calloc(sizeof *grpatch, PU_HWRPATCHINFO,
		                   &(model->grpatch));

	if (!grpatch->mipmap.downloaded && !grpatch->mipmap.grInfo.data)
	{
		int w = 0, h = 0;
#ifdef HAVE_PNG
		grpatch->mipmap.grInfo.format = PNG_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap.grInfo.format == 0)
#endif
		grpatch->mipmap.grInfo.format = PCX_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap.grInfo.format == 0)
			return;

		grpatch->mipmap.downloaded = 0;
		grpatch->mipmap.flags = 0;

		grpatch->width = (INT16)w;
		grpatch->height = (INT16)h;
		grpatch->mipmap.width = (UINT16)w;
		grpatch->mipmap.height = (UINT16)h;

		// not correct!
		grpatch->mipmap.grInfo.smallLodLog2 = GR_LOD_LOG2_256;
		grpatch->mipmap.grInfo.largeLodLog2 = GR_LOD_LOG2_256;
		grpatch->mipmap.grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
	}
	HWD.pfnSetTexture(&grpatch->mipmap);
	HWR_UnlockCachedPatch(grpatch);
}

// -----------------+
// md2_loadBlendTexture  : Download a pcx or png texture for blending MD2 models
// -----------------+
static void md2_loadBlendTexture(md2_t *model)
{
	GLPatch_t *grpatch;
	char *filename = Z_Malloc(strlen(model->filename)+7, PU_STATIC, NULL);
	strcpy(filename, model->filename);

	FIL_ForceExtension(filename, "_blend.png");

	if (model->blendgrpatch)
	{
		grpatch = model->blendgrpatch;
		Z_Free(grpatch->mipmap.grInfo.data);
	}
	else
		grpatch = Z_Calloc(sizeof *grpatch, PU_HWRPATCHINFO,
		                   &(model->blendgrpatch));

	if (!grpatch->mipmap.downloaded && !grpatch->mipmap.grInfo.data)
	{
		int w = 0, h = 0;
#ifdef HAVE_PNG
		grpatch->mipmap.grInfo.format = PNG_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap.grInfo.format == 0)
#endif
		grpatch->mipmap.grInfo.format = PCX_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap.grInfo.format == 0)
		{
			Z_Free(filename);
			return;
		}

		grpatch->mipmap.downloaded = 0;
		grpatch->mipmap.flags = 0;

		grpatch->width = (INT16)w;
		grpatch->height = (INT16)h;
		grpatch->mipmap.width = (UINT16)w;
		grpatch->mipmap.height = (UINT16)h;

		// not correct!
		grpatch->mipmap.grInfo.smallLodLog2 = GR_LOD_LOG2_256;
		grpatch->mipmap.grInfo.largeLodLog2 = GR_LOD_LOG2_256;
		grpatch->mipmap.grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
	}
	HWD.pfnSetTexture(&grpatch->mipmap); // We do need to do this so that it can be cleared and knows to recreate it when necessary
	HWR_UnlockCachedPatch(grpatch);

	Z_Free(filename);
}

// Don't spam the console, or the OS with fopen requests!
static boolean nomd2s = false;

void HWR_InitMD2(void)
{
	size_t i;
	INT32 s;
	FILE *f;
	char name[18], filename[32];
	float scale, offset;

	CONS_Printf("InitMD2()...\n");
	for (s = 0; s < MAXSKINS; s++)
	{
		md2_playermodels[s].scale = -1.0f;
		md2_playermodels[s].model = NULL;
		md2_playermodels[s].grpatch = NULL;
		md2_playermodels[s].skin = -1;
		md2_playermodels[s].notfound = true;
		md2_playermodels[s].error = false;
	}
	for (i = 0; i < NUMSPRITES; i++)
	{
		md2_models[i].scale = -1.0f;
		md2_models[i].model = NULL;
		md2_models[i].grpatch = NULL;
		md2_models[i].skin = -1;
		md2_models[i].notfound = true;
		md2_models[i].error = false;
	}

	// read the md2.dat file
	f = fopen("md2.dat", "rt");

	if (!f)
	{
		CONS_Printf("%s", M_GetText("Error while loading md2.dat\n"));
		nomd2s = true;
		return;
	}
	while (fscanf(f, "%19s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		if (stricmp(name, "PLAY") == 0)
		{
			CONS_Printf("MD2 for sprite PLAY detected in md2.dat, use a player skin instead!\n");
			continue;
		}

		for (i = 0; i < NUMSPRITES; i++)
		{
			if (stricmp(name, sprnames[i]) == 0)
			{
				//if (stricmp(name, "PLAY") == 0)
					//continue;

				//CONS_Debug(DBG_RENDER, "  Found: %s %s %f %f\n", name, filename, scale, offset);
				md2_models[i].scale = scale;
				md2_models[i].offset = offset;
				md2_models[i].notfound = false;
				strcpy(md2_models[i].filename, filename);
				goto md2found;
			}
		}

		for (s = 0; s < MAXSKINS; s++)
		{
			if (stricmp(name, skins[s].name) == 0)
			{
				//CONS_Printf("  Found: %s %s %f %f\n", name, filename, scale, offset);
				md2_playermodels[s].skin = s;
				md2_playermodels[s].scale = scale;
				md2_playermodels[s].offset = offset;
				md2_playermodels[s].notfound = false;
				strcpy(md2_playermodels[s].filename, filename);
				goto md2found;
			}
		}
		// no sprite/player skin name found?!?
		CONS_Printf("Unknown sprite/player skin %s detected in md2.dat\n", name);
md2found:
		// move on to next line...
		continue;
	}
	fclose(f);
}

void HWR_AddPlayerMD2(int skin) // For MD2's that were added after startup
{
	FILE *f;
	char name[18], filename[32];
	float scale, offset;

	if (nomd2s)
		return;

	CONS_Printf("AddPlayerMD2()...\n");

	// read the md2.dat file
	f = fopen("md2.dat", "rt");

	if (!f)
	{
		CONS_Printf("Error while loading md2.dat\n");
		nomd2s = true;
		return;
	}

	// Check for any MD2s that match the names of player skins!
	while (fscanf(f, "%19s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		if (stricmp(name, skins[skin].name) == 0)
		{
			md2_playermodels[skin].skin = skin;
			md2_playermodels[skin].scale = scale;
			md2_playermodels[skin].offset = offset;
			md2_playermodels[skin].notfound = false;
			strcpy(md2_playermodels[skin].filename, filename);
			goto playermd2found;
		}
	}

	//CONS_Printf("MD2 for player skin %s not found\n", skins[skin].name);
	md2_playermodels[skin].notfound = true;
playermd2found:
	fclose(f);
}


void HWR_AddSpriteMD2(size_t spritenum) // For MD2s that were added after startup
{
	FILE *f;
	// name[18] is used to check for names in the md2.dat file that match with sprites or player skins
	// sprite names are always 4 characters long, and names is for player skins can be up to 19 characters long
	char name[18], filename[32];
	float scale, offset;

	if (nomd2s)
		return;

	if (spritenum == SPR_PLAY) // Handled already NEWMD2: Per sprite, per-skin check
		return;

	// Read the md2.dat file
	f = fopen("md2.dat", "rt");

	if (!f)
	{
		CONS_Printf("Error while loading md2.dat\n");
		nomd2s = true;
		return;
	}

	// Check for any MD2s that match the names of player skins!
	while (fscanf(f, "%19s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		if (stricmp(name, sprnames[spritenum]) == 0)
		{
			md2_models[spritenum].scale = scale;
			md2_models[spritenum].offset = offset;
			md2_models[spritenum].notfound = false;
			strcpy(md2_models[spritenum].filename, filename);
			goto spritemd2found;
		}
	}

	//CONS_Printf("MD2 for sprite %s not found\n", sprnames[spritenum]);
	md2_models[spritenum].notfound = true;
spritemd2found:
	fclose(f);
}

static void HWR_CreateBlendedTexture(GLPatch_t *gpatch, GLPatch_t *blendgpatch, GLMipmap_t *grmip, skincolors_t color)
{
	UINT16 w = gpatch->width, h = gpatch->height;
	UINT32 size = w*h;
	RGBA_t *image, *blendimage, *cur, blendcolor;

	if (grmip->width == 0)
	{

		grmip->width = gpatch->width;
		grmip->height = gpatch->height;

		// no wrap around, no chroma key
		grmip->flags = 0;
		// setup the texture info
		grmip->grInfo.format = GR_RGBA;
	}

	Z_Free(grmip->grInfo.data);
	grmip->grInfo.data = NULL;

	cur = Z_Malloc(size*4, PU_HWRCACHE, &grmip->grInfo.data);
	memset(cur, 0x00, size*4);

	image = gpatch->mipmap.grInfo.data;
	blendimage = blendgpatch->mipmap.grInfo.data;

	if (color == SKINCOLOR_NONE || color >= MAXTRANSLATIONS)
		blendcolor = V_GetColor(0xff);
	else
		blendcolor = V_GetColor(Color_Index[color-1][4]);

	while (size--)
	{
		if (blendimage->s.alpha == 0)
		{
			// Don't bother with blending the pixel if the alpha of the blend pixel is 0
			cur->rgba = image->rgba;
		}
		else
		{
			INT32 tempcolor;
			INT16 tempmult, tempalpha;
			tempalpha = -(abs(blendimage->s.red-127)-127)*2;
			if (tempalpha > 255)
				tempalpha = 255;
			else if (tempalpha < 0)
				tempalpha = 0;

			tempmult = (blendimage->s.red-127)*2;
			if (tempmult > 255)
				tempmult = 255;
			else if (tempmult < 0)
				tempmult = 0;

			tempcolor = (image->s.red*(255-blendimage->s.alpha))/255 + ((tempmult + ((tempalpha*blendcolor.s.red)/255)) * blendimage->s.alpha)/255;
			cur->s.red = (UINT8)tempcolor;
			tempcolor = (image->s.green*(255-blendimage->s.alpha))/255 + ((tempmult + ((tempalpha*blendcolor.s.green)/255)) * blendimage->s.alpha)/255;
			cur->s.green = (UINT8)tempcolor;
			tempcolor = (image->s.blue*(255-blendimage->s.alpha))/255 + ((tempmult + ((tempalpha*blendcolor.s.blue)/255)) * blendimage->s.alpha)/255;
			cur->s.blue = (UINT8)tempcolor;
			cur->s.alpha = image->s.alpha;
		}

		cur++; image++; blendimage++;
	}

	return;
}

static void HWR_GetBlendedTexture(GLPatch_t *gpatch, GLPatch_t *blendgpatch, const UINT8 *colormap, skincolors_t color)
{
	// mostly copied from HWR_GetMappedPatch, hence the similarities and comment
	GLMipmap_t *grmip, *newmip;

	if (colormap == colormaps || colormap == NULL)
	{
		// Don't do any blending
		HWD.pfnSetTexture(&gpatch->mipmap);
		return;
	}

	// search for the mimmap
	// skip the first (no colormap translated)
	for (grmip = &gpatch->mipmap; grmip->nextcolormap; )
	{
		grmip = grmip->nextcolormap;
		if (grmip->colormap == colormap)
		{
			if (grmip->downloaded && grmip->grInfo.data)
			{
				HWD.pfnSetTexture(grmip); // found the colormap, set it to the correct texture
				Z_ChangeTag(grmip->grInfo.data, PU_HWRCACHE_UNLOCKED);
				return;
			}
		}
	}

	// If here, the blended texture has not been created
	// So we create it

	//BP: WARNING: don't free it manually without clearing the cache of harware renderer
	//              (it have a liste of mipmap)
	//    this malloc is cleared in HWR_FreeTextureCache
	//    (...) unfortunately z_malloc fragment alot the memory :(so malloc is better
	newmip = calloc(1, sizeof (*newmip));
	if (newmip == NULL)
		I_Error("%s: Out of memory", "HWR_GetMappedPatch");
	grmip->nextcolormap = newmip;
	newmip->colormap = colormap;

	HWR_CreateBlendedTexture(gpatch, blendgpatch, newmip, color);

	HWD.pfnSetTexture(newmip);
	Z_ChangeTag(newmip->grInfo.data, PU_HWRCACHE_UNLOCKED);
}


// -----------------+
// HWR_DrawMD2      : Draw MD2
//                  : (monsters, bonuses, weapons, lights, ...)
// Returns          :
// -----------------+
	/*
	wait/stand
	death
	pain
	walk
	shoot/fire

	die?
	atka?
	atkb?
	attacka/b/c/d?
	res?
	run?
	*/

static UINT8 P_GetModelSprite2(md2_t *md2, skin_t *skin, UINT8 spr2, player_t *player)
{
	UINT8 super = 0, i = 0;

	if (!md2 || !skin)
		return 0;

	while (!(md2->model->spr2frames[spr2*2 + 1])
		&& spr2 != SPR2_STND
		&& ++i != 32) // recursion limiter
	{
		if (spr2 & FF_SPR2SUPER)
		{
			super = FF_SPR2SUPER;
			spr2 &= ~FF_SPR2SUPER;
			continue;
		}

		switch(spr2)
		{

		// Normal special cases.
		case SPR2_JUMP:
			spr2 = ((player
					? player->charflags
					: skin->flags)
					& SF_NOJUMPSPIN) ? SPR2_SPNG : SPR2_ROLL;
			break;
		case SPR2_TIRE:
			spr2 = (player && player->charability == CA_SWIM) ? SPR2_SWIM : SPR2_FLY;
			break;

		// Use the handy list, that's what it's there for!
		default:
			spr2 = spr2defaults[spr2];
			break;
		}

		spr2 |= super;
	}

	return spr2;
}

#define NORMALFOG 0x00000000
#define FADEFOG 0x19000000
void HWR_DrawMD2(gr_vissprite_t *spr)
{
	FSurfaceInfo Surf;

	char filename[64];
	INT32 frame;
	FTransform p;
	md2_t *md2;
	UINT8 color[4];

	if (!cv_grmd2.value)
		return;

	if (spr->precip)
		return;

	// MD2 colormap fix
	// colormap test
	{
		sector_t *sector = spr->mobj->subsector->sector;
		UINT8 lightlevel = 255;
		extracolormap_t *colormap = sector->extra_colormap;

		if (sector->numlights)
		{
			INT32 light;

			light = R_GetPlaneLight(sector, spr->mobj->z + spr->mobj->height, false); // Always use the light at the top instead of whatever I was doing before

			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = *sector->lightlist[light].lightlevel;

			if (sector->lightlist[light].extra_colormap)
				colormap = sector->lightlist[light].extra_colormap;
		}
		else
		{
			if (!(spr->mobj->frame & FF_FULLBRIGHT))
				lightlevel = sector->lightlevel;

			if (sector->extra_colormap)
				colormap = sector->extra_colormap;
		}

		if (colormap)
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, colormap->rgba, colormap->fadergba, false, false);
		else
			Surf.FlatColor.rgba = HWR_Lighting(lightlevel, NORMALFOG, FADEFOG, false, false);
	}

	// Look at HWR_ProjectSprite for more
	{
		GLPatch_t *gpatch;
		INT32 *buff;
		UINT32 durs = spr->mobj->state->tics;
		UINT32 tics = spr->mobj->tics;
		md2_frame_t *curr, *next = NULL;
		const UINT8 flip = (UINT8)(!(spr->mobj->eflags & MFE_VERTICALFLIP) != !(spr->mobj->frame & FF_VERTICALFLIP));
		spritedef_t *sprdef;
		spriteframe_t *sprframe;
		float finalscale;

		// Apparently people don't like jump frames like that, so back it goes
		//if (tics > durs)
			//durs = tics;

		if (spr->mobj->flags2 & MF2_SHADOW)
			Surf.FlatColor.s.alpha = 0x40;
		else if (spr->mobj->frame & FF_TRANSMASK)
			HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else
			Surf.FlatColor.s.alpha = 0xFF;

		// dont forget to enabled the depth test because we can't do this like
		// before: polygons models are not sorted

		// 1. load model+texture if not already loaded
		// 2. draw model with correct position, rotation,...
		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY) // Use the player MD2 list if the mobj has a skin and is using the player sprites
		{
			md2 = &md2_playermodels[(skin_t*)spr->mobj->skin-skins];
			md2->skin = (skin_t*)spr->mobj->skin-skins;
		}
		else
			md2 = &md2_models[spr->mobj->sprite];

		if (md2->error)
			return; // we already failed loading this before :(
		if (!md2->model)
		{
			//CONS_Debug(DBG_RENDER, "Loading MD2... (%s)", sprnames[spr->mobj->sprite]);
			sprintf(filename, "md2/%s", md2->filename);
			md2->model = md2_readModel(filename);

			if (md2->model)
			{
				md2_printModelInfo(md2->model);
			}
			else
			{
				//CONS_Debug(DBG_RENDER, " FAILED\n");
				md2->error = true; // prevent endless fail
				return;
			}
		}
		//HWD.pfnSetBlend(blend); // This seems to actually break translucency?
		finalscale = md2->scale;
		//Hurdler: arf, I don't like that implementation at all... too much crappy
		gpatch = md2->grpatch;
		if (!gpatch || !gpatch->mipmap.grInfo.format || !gpatch->mipmap.downloaded)
			md2_loadTexture(md2);
		gpatch = md2->grpatch; // Load it again, because it isn't being loaded into gpatch after md2_loadtexture...

		if ((gpatch && gpatch->mipmap.grInfo.format) // don't load the blend texture if the base texture isn't available
			&& (!md2->blendgrpatch || !((GLPatch_t *)md2->blendgrpatch)->mipmap.grInfo.format || !((GLPatch_t *)md2->blendgrpatch)->mipmap.downloaded))
			md2_loadBlendTexture(md2);

		if (gpatch && gpatch->mipmap.grInfo.format) // else if meant that if a texture couldn't be loaded, it would just end up using something else's texture
		{
			if ((skincolors_t)spr->mobj->color != SKINCOLOR_NONE &&
				md2->blendgrpatch && ((GLPatch_t *)md2->blendgrpatch)->mipmap.grInfo.format
				&& gpatch->width == ((GLPatch_t *)md2->blendgrpatch)->width && gpatch->height == ((GLPatch_t *)md2->blendgrpatch)->height)
			{
				HWR_GetBlendedTexture(gpatch, (GLPatch_t *)md2->blendgrpatch, spr->colormap, (skincolors_t)spr->mobj->color);
			}
			else
			{
				// This is safe, since we know the texture has been downloaded
				HWD.pfnSetTexture(&gpatch->mipmap);
			}
		}
		else
		{
			// Sprite
			gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);
			HWR_GetMappedPatch(gpatch, spr->colormap);
		}

		if (spr->mobj->frame & FF_ANIMATE)
		{
			// set duration and tics to be the correct values for FF_ANIMATE states
			durs = spr->mobj->state->var2;
			tics = spr->mobj->anim_duration;
		}

#define INTERPOLERATION_LIMIT TICRATE/4

		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY && md2->model->spr2frames)
		{
			UINT8 spr2 = P_GetModelSprite2(md2, spr->mobj->skin, spr->mobj->sprite2, spr->mobj->player);
			UINT8 mod = md2->model->spr2frames[spr2*2 + 1] ? md2->model->spr2frames[spr2*2 + 1] : md2->model->header.numFrames;
			if (mod > ((skin_t *)spr->mobj->skin)->sprites[spr2].numframes)
				mod = ((skin_t *)spr->mobj->skin)->sprites[spr2].numframes;
			//FIXME: this is not yet correct
			frame = (spr->mobj->frame & FF_FRAMEMASK);
			if (frame >= mod)
				frame = 0;
			buff = md2->model->glCommandBuffer;
			curr = &md2->model->frames[md2->model->spr2frames[spr2*2] + frame];
			if (cv_grmd2.value == 1 && tics <= INTERPOLERATION_LIMIT)
			{
				if (durs > INTERPOLERATION_LIMIT)
					durs = INTERPOLERATION_LIMIT;

				if (spr->mobj->frame & FF_ANIMATE
					|| (spr->mobj->state->nextstate != S_NULL
					&& states[spr->mobj->state->nextstate].sprite == spr->mobj->sprite
					&& (states[spr->mobj->state->nextstate].frame & FF_FRAMEMASK) == spr->mobj->sprite2))
				{
					if (++frame >= mod)
						frame = 0;
					if (frame || !(spr->mobj->state->frame & FF_SPR2ENDSTATE))
						next = &md2->model->frames[md2->model->spr2frames[spr2*2] + frame];
				}
			}
		}
		else
		{
			//FIXME: this is not yet correct
			frame = (spr->mobj->frame & FF_FRAMEMASK) % md2->model->header.numFrames;
			buff = md2->model->glCommandBuffer;
			curr = &md2->model->frames[frame];
			if (cv_grmd2.value == 1 && tics <= INTERPOLERATION_LIMIT)
			{
				if (durs > INTERPOLERATION_LIMIT)
					durs = INTERPOLERATION_LIMIT;

				// frames are handled differently for states with FF_ANIMATE, so get the next frame differently for the interpolation
				if (spr->mobj->frame & FF_ANIMATE)
				{
					UINT32 nextframe = (spr->mobj->frame & FF_FRAMEMASK) + 1;
					if (nextframe >= (UINT32)spr->mobj->state->var1)
						nextframe = (spr->mobj->state->frame & FF_FRAMEMASK);
					nextframe %= md2->model->header.numFrames;
					next = &md2->model->frames[nextframe];
				}
				else
				{
					if (spr->mobj->state->nextstate != S_NULL
					&& states[spr->mobj->state->nextstate].sprite == spr->mobj->sprite)
					{
						const UINT32 nextframe = (states[spr->mobj->state->nextstate].frame & FF_FRAMEMASK) % md2->model->header.numFrames;
						next = &md2->model->frames[nextframe];
					}
				}
			}
		}

#undef INTERPOLERATION_LIMIT

		//Hurdler: it seems there is still a small problem with mobj angle
		p.x = FIXED_TO_FLOAT(spr->mobj->x);
		p.y = FIXED_TO_FLOAT(spr->mobj->y)+md2->offset;

		if (flip)
			p.z = FIXED_TO_FLOAT(spr->mobj->z + spr->mobj->height);
		else
			p.z = FIXED_TO_FLOAT(spr->mobj->z);

		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			sprdef = &((skin_t *)spr->mobj->skin)->sprites[spr->mobj->sprite2];
		else
			sprdef = &sprites[spr->mobj->sprite];

		sprframe = &sprdef->spriteframes[spr->mobj->frame & FF_FRAMEMASK];

		if (sprframe->rotate)
		{
			const fixed_t anglef = AngleFixed((spr->mobj->player ? spr->mobj->player->drawangle : spr->mobj->angle));
			p.angley = FIXED_TO_FLOAT(anglef);
		}
		else
		{
			const fixed_t anglef = AngleFixed((R_PointToAngle(spr->mobj->x, spr->mobj->y))-ANGLE_180);
			p.angley = FIXED_TO_FLOAT(anglef);
		}
		p.anglex = 0.0f;

		color[0] = Surf.FlatColor.s.red;
		color[1] = Surf.FlatColor.s.green;
		color[2] = Surf.FlatColor.s.blue;
		color[3] = Surf.FlatColor.s.alpha;

		// SRB2CBTODO: MD2 scaling support
		finalscale *= FIXED_TO_FLOAT(spr->mobj->scale);

		p.flip = atransform.flip;

		HWD.pfnDrawMD2i(buff, curr, durs, tics, next, &p, finalscale, flip, color);
	}
}

#endif //HWRENDER
