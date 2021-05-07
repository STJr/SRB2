// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_batching.h
/// \brief Draw call batching and related things.

#ifndef __HWR_BATCHING_H__
#define __HWR_BATCHING_H__

#include "hw_defs.h"
#include "hw_data.h"
#include "hw_drv.h"

typedef struct
{
	FSurfaceInfo surf;// surf also has its own polyflags for some reason, but it seems unused
	unsigned int vertsIndex;// location of verts in unsortedVertexArray
	FUINT numVerts;
	FBITFIELD polyFlags;
	GLMipmap_t *texture;
	int shader;
	// this tells batching that the plane belongs to a horizon line and must be drawn in correct order with the skywalls
	boolean horizonSpecial;
} PolygonArrayEntry;

void HWR_StartBatching(void);
void HWR_SetCurrentTexture(GLMipmap_t *texture);
void HWR_ProcessPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, int shader, boolean horizonSpecial);
void HWR_RenderBatches(void);

#endif
