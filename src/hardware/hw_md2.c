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
		|| model->header.magic !=
		(INT32)(('2' << 24) + ('P' << 16) + ('D' << 8) + 'I'))
	{
		fclose(file);
		free(model);
		return 0;
	}

	model->header.numSkins = 1;

	// read skins
	fseek(file, model->header.offsetSkins, SEEK_SET);
	if (model->header.numSkins > 0)
	{
		model->skins = calloc(sizeof (md2_skin_t), model->header.numSkins);
		if (!model->skins || model->header.numSkins !=
			fread(model->skins, sizeof (md2_skin_t), model->header.numSkins, file))
		{
			md2_freeModel (model);
			return 0;
		}

		;
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
				return 0;
			}

			strcpy(model->frames[i].name, frame->name);
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
			return 0;
		}
	}

	fclose(file);

	return model;
}

/*
 * center model
 */
static inline void md2_getBoundingBox (md2_model_t *model, float *minmax)
{
	size_t i;
	float minx, maxx;
	float miny, maxy;
	float minz, maxz;

	minx = miny = minz = 999999.0f;
	maxx = maxy = maxz = -999999.0f;

	/* get bounding box */
	for (i = 0; i < model->header.numVertices; i++)
	{
		md2_triangleVertex_t *v = &model->frames[0].vertices[i];

		if (v->vertex[0] < minx)
			minx = v->vertex[0];
		else if (v->vertex[0] > maxx)
			maxx = v->vertex[0];

		if (v->vertex[1] < miny)
			miny = v->vertex[1];
		else if (v->vertex[1] > maxy)
			maxy = v->vertex[1];

		if (v->vertex[2] < minz)
			minz = v->vertex[2];
		else if (v->vertex[2] > maxz)
			maxz = v->vertex[2];
	}

	minmax[0] = minx;
	minmax[1] = maxx;
	minmax[2] = miny;
	minmax[3] = maxy;
	minmax[4] = minz;
	minmax[5] = maxz;
}

static inline INT32 md2_getAnimationCount(md2_model_t *model)
{
	size_t i, pos;
	INT32 j = 0, count;
	char name[16], last[16];

	strcpy(last, model->frames[0].name);
	pos = strlen(last) - 1;
	while (last[pos] >= '0' && last[pos] <= '9' && j < 2)
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	count = 0;

	for (i = 0; i <= model->header.numFrames; i++)
	{
		if (i == model->header.numFrames)
			strcpy(name, ""); // some kind of a sentinel
		else
			strcpy(name, model->frames[i].name);
		pos = strlen(name) - 1;
		j = 0;
		while (name[pos] >= '0' && name[pos] <= '9' && j < 2)
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if (strcmp(last, name))
		{
			strcpy(last, name);
			count++;
		}
	}

	return count;
}

static inline const char * md2_getAnimationName (md2_model_t *model, INT32 animation)
{
	size_t i, pos;
	INT32 j = 0, count;
	static char last[32];
	char name[32];

	strcpy(last, model->frames[0].name);
	pos = strlen(last) - 1;
	while (last[pos] >= '0' && last[pos] <= '9' && j < 2)
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	count = 0;

	for (i = 0; i <= model->header.numFrames; i++)
	{
		if (i == model->header.numFrames)
			strcpy(name, ""); // some kind of a sentinel
		else
			strcpy(name, model->frames[i].name);
		pos = strlen(name) - 1;
		j = 0;
		while (name[pos] >= '0' && name[pos] <= '9' && j < 2)
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if (strcmp(last, name))
		{
			if (count == animation)
				return last;

			strcpy(last, name);
			count++;
		}
	}

	return 0;
}

static inline void md2_getAnimationFrames(md2_model_t *model,
	INT32 animation, INT32 *startFrame, INT32 *endFrame)
{
	size_t i, pos;
	INT32 j = 0, count, numFrames, frameCount;
	char name[16], last[16];

	strcpy(last, model->frames[0].name);
	pos = strlen(last) - 1;
	while (last[pos] >= '0' && last[pos] <= '9' && j < 2)
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	count = 0;
	numFrames = 0;
	frameCount = 0;

	for (i = 0; i <= model->header.numFrames; i++)
	{
		if (i == model->header.numFrames)
			strcpy(name, ""); // some kind of a sentinel
		else
			strcpy(name, model->frames[i].name);
		pos = strlen(name) - 1;
		j = 0;
		while (name[pos] >= '0' && name[pos] <= '9' && j < 2)
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if (strcmp(last, name))
		{
			strcpy(last, name);

			if (count == animation)
			{
				*startFrame = frameCount - numFrames;
				*endFrame = frameCount - 1;
				return;
			}

			count++;
			numFrames = 0;
		}
		frameCount++;
		numFrames++;
	}
	*startFrame = *endFrame = 0;
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
	}
	for (i = 0; i < NUMSPRITES; i++)
	{
		md2_models[i].scale = -1.0f;
		md2_models[i].model = NULL;
		md2_models[i].grpatch = NULL;
		md2_models[i].skin = -1;
		md2_models[i].notfound = true;
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

	if (!spr->precip)
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
		const UINT8 flip = (UINT8)((spr->mobj->eflags & MFE_VERTICALFLIP) == MFE_VERTICALFLIP);
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

		if (gpatch && gpatch->mipmap.grInfo.format) // else if meant that if a texture couldn't be loaded, it would just end up using something else's texture
		{
			// This is safe, since we know the texture has been downloaded
			HWD.pfnSetTexture(&gpatch->mipmap);
		}
		else
		{
			// Sprite
			gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);
			HWR_GetMappedPatch(gpatch, spr->colormap);
		}

		//FIXME: this is not yet correct
		frame = (spr->mobj->frame & FF_FRAMEMASK) % md2->model->header.numFrames;
		buff = md2->model->glCommandBuffer;
		curr = &md2->model->frames[frame];
		if (cv_grmd2.value == 1
		    && spr->mobj->state->nextstate != S_NULL && states[spr->mobj->state->nextstate].sprite != SPR_NULL
		    && !(spr->mobj->player && (spr->mobj->state->nextstate == S_PLAY_TAP1 || spr->mobj->state->nextstate == S_PLAY_TAP2) && spr->mobj->state == &states[S_PLAY_STND]))
		{
			const INT32 nextframe = (states[spr->mobj->state->nextstate].frame & FF_FRAMEMASK) % md2->model->header.numFrames;
			next = &md2->model->frames[nextframe];
		}

		//Hurdler: it seems there is still a small problem with mobj angle
		p.x = FIXED_TO_FLOAT(spr->mobj->x);
		p.y = FIXED_TO_FLOAT(spr->mobj->y)+md2->offset;

		if (spr->mobj->eflags & MFE_VERTICALFLIP)
			p.z = FIXED_TO_FLOAT(spr->mobj->z + spr->mobj->height);
		else
			p.z = FIXED_TO_FLOAT(spr->mobj->z);

		if (spr->mobj->skin && spr->mobj->sprite == SPR_PLAY)
			sprdef = &((skin_t *)spr->mobj->skin)->spritedef;
		else
			sprdef = &sprites[spr->mobj->sprite];

		sprframe = &sprdef->spriteframes[spr->mobj->frame & FF_FRAMEMASK];

		if (sprframe->rotate)
		{
			const fixed_t anglef = AngleFixed(spr->mobj->angle);
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

		if (postimgtype == postimg_flip)
			p.flip = true;
		else
			p.flip = false;

		HWD.pfnDrawMD2i(buff, curr, durs, tics, next, &p, finalscale, flip, color);
	}
}

#endif //HWRENDER
