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
/// \brief Resizeable vertex array implementation

#include "hw_vertarray.h"

#include "../z_zone.h"

#define RESIZE_FACTOR 2


void HWR_InitVertexArray(FVertexArray * varray, unsigned int initialSize)
{
	varray->size = initialSize;
	varray->used = 0;
	varray->buffer = (FOutVector *) Z_Malloc(varray->size * sizeof(FOutVector), PU_STATIC, varray);
}

void HWR_FreeVertexArray(FVertexArray * varray)
{
	Z_Free(varray->buffer);
}

void HWR_InsertVertexArray(FVertexArray * varray, int numElements, FOutVector * elements)
{
	int i = 0;
	FOutVector * actualElements = NULL;
	int numActualElements = numElements;

	// Expand array from fan to triangles
	// Not optimal... but the overhead will make up for it significantly
	{
		int j = 0;
		int k = 0;
		numActualElements -= 3;
		numActualElements *= 3;
		numActualElements += 3;

		actualElements = (FOutVector *) Z_Malloc(numActualElements * sizeof(FOutVector), PU_STATIC, NULL);

		for (j = 2; j < numElements; j++)
		{
			actualElements[k] = elements[0];
			actualElements[k+1] = elements[j-1];
			actualElements[k+2] = elements[j];
			k += 3;
		}
	}

	while (varray->used + numActualElements >= varray->size)
	{
		varray->buffer = (FOutVector *) Z_Realloc(varray->buffer, varray->size * RESIZE_FACTOR * sizeof(FOutVector), PU_STATIC, varray);
		varray->size *= RESIZE_FACTOR;
	}

	for (i = 0; i < numActualElements; i++)
	{
		varray->buffer[i + varray->used] = actualElements[i];
	}
	
	varray->used += numActualElements;
	Z_Free(actualElements);
}
