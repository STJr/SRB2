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
/// \brief Vertex buckets mapping based on texture binding

#ifndef __HW_VERTBUCKETS_H__
#define __HW_VERTBUCKETS_H__

#include "hw_vertarray.h"

/* The vertex buckets are a mapping of runtime texture number to FVertexArray.
 * BSP traversal, instead of sending DrawPolygon calls to the driver directly,
 * will add vertices to these buckets based on the texture used. After BSP
 * traversal, you should call HWR_DrawVertexBuckets in order to send DrawPolygon
 * calls for each texture in the buckets.
 */

/** Empty the vertex buckets to prepare for another BSP traversal. This function
  * should be called before rendering any BSP node. */
void HWR_ResetVertexBuckets(void);

FVertexArray * HWR_GetVertexArrayForFlat(lumpnum_t lumpnum);
/** Get a pointer to the vertex array used for a given texture number. */
FVertexArray * HWR_GetVertexArrayForTexture(int texNum);

/** Send draw commands to the hardware driver to draw each of the buckets. */
void HWR_DrawVertexBuckets(void);

#endif //__HW_VERTBUCKETS_H__
