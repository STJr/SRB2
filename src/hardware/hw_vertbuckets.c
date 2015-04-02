// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 by Sonic Team Jr.
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
//
//-----------------------------------------------------------------------------
/// \file
/// \brief Vertex buckets mapping based on texture binding, implementation

#include "hw_vertbuckets.h"

#include "hw_glob.h"
#include "hw_drv.h"

#include "../z_zone.h"

// 6 because we assume at least a quad per texture.
#define INITIAL_BUCKET_SIZE 6

static FVertexArray * buckets = NULL;
static int numTextureBuckets = 0;

static int bucketsHaveBeenInitialized = 0;

static aatree_t * flatArrays;
static aatree_t * textureArrays;

static UINT8 arraysInitialized = 0;

typedef enum
{
	RMODE_FLAT,
	RMODE_TEXTURE
} EMode;

static EMode rmode;

static void FreeArraysFunc(INT32 key, void * value)
{
	FVertexArray * varray = (FVertexArray *) value;
	HWR_FreeVertexArray(varray);
	Z_Free(varray);
}

static void DrawArraysFunc(INT32 key, void * value)
{
	FVertexArray * varray = (FVertexArray *) value;
	FSurfaceInfo surf;

	if (varray->used == 0)
	{
		return;
	}


	surf.FlatColor.rgba = 0xFFFFFFFF;

	if (rmode == RMODE_FLAT)
	{
		HWR_GetFlat(key);
	}
	else if (rmode == RMODE_TEXTURE)
	{
		HWR_GetTexture(key);
	}
	// TODO support flags, translucency, etc
	HWD.pfnDrawPolygon(&surf, varray->buffer, varray->used, PF_Occlude, PP_Triangles);
}

static FVertexArray * GetVArray(aatree_t * tree, int index)
{
	FVertexArray * varray = (FVertexArray *) M_AATreeGet(tree, index);
	if (!varray)
	{
		// no varray for this flat yet, create it
		varray = (FVertexArray *) Z_Calloc(sizeof(FVertexArray), PU_STATIC, NULL);
		HWR_InitVertexArray(varray, INITIAL_BUCKET_SIZE);
		M_AATreeSet(tree, index, (void *) varray);
	}

	return varray;
}

void HWR_ResetVertexBuckets(void)
{
	if (arraysInitialized)
	{
		M_AATreeIterate(flatArrays, FreeArraysFunc);
		M_AATreeIterate(textureArrays, FreeArraysFunc);
		M_AATreeFree(flatArrays);
		M_AATreeFree(textureArrays);
	}

	flatArrays = M_AATreeAlloc(0);
	textureArrays = M_AATreeAlloc(0);
}

FVertexArray * HWR_GetVertexArrayForTexture(int texNum)
{
	return GetVArray(textureArrays, texNum);
}

FVertexArray * HWR_GetVertexArrayForFlat(lumpnum_t lump)
{
	return GetVArray(flatArrays, lump);
}

void HWR_DrawVertexBuckets(void)
{
	rmode = RMODE_TEXTURE;
	M_AATreeIterate(textureArrays, DrawArraysFunc);
	rmode = RMODE_FLAT;
	M_AATreeIterate(flatArrays, DrawArraysFunc);
}
