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
static int numBuckets = 0;

static int bucketsHaveBeenInitialized = 0;

void HWR_ResetVertexBuckets(void)
{
	// Free all arrays if they've been initialized before.
	if (bucketsHaveBeenInitialized)
	{
		int i = 0;
		for (i = 0; i < numBuckets; i++)
		{
			HWR_FreeVertexArray(&buckets[i]);
		}
		Z_Free(buckets);
	}

	// We will only have as many buckets as gr_numtextures
	numBuckets = HWR_GetNumCacheTextures();
	buckets = Z_Malloc(numBuckets * sizeof(FVertexArray), PU_HWRCACHE, NULL);
	
	{
		int i = 0;
		for (i = 0; i < numBuckets; i++)
		{
			HWR_InitVertexArray(&buckets[i], INITIAL_BUCKET_SIZE);
		}
	}
	bucketsHaveBeenInitialized = 1;
}

FVertexArray * HWR_GetVertexArrayForTexture(int texNum)
{
	if (texNum >= numBuckets || texNum < 0)
	{
		I_Error("HWR_GetVertexArrayForTexture: texNum >= numBuckets");
		//return NULL;
	}
	return &buckets[texNum];
}

void HWR_DrawVertexBuckets(void)
{
	int i = 0;
	for (i = 0; i < numBuckets; i++)
	{
		FSurfaceInfo surface;
		FVertexArray * bucket = &buckets[i];
		
		if (bucket->used == 0)
		{
			// We did not add any vertices with this texture, so we can skip binding and drawing.
			continue;
		}
		
		// Bind the texture.
		
		HWR_GetTexture(i);
		// Draw the triangles.
		// TODO handle polyflags and modulation for lighting/translucency?
		HWD.pfnDrawPolygon(&surface, bucket->buffer, bucket->used, 0);
		CONS_Printf("%d: %d tris\n", i, bucket->used);
	}
}
