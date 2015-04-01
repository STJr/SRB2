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
	varray->buffer = Z_Malloc(varray->size * sizeof(FOutVector), PU_STATIC, varray);
}

void HWR_FreeVertexArray(FVertexArray * varray)
{
	Z_Free(varray->buffer);
}

void HWR_InsertVertexArray(FVertexArray * varray, int numElements, FOutVector * elements)
{
	int i = 0;
	if (varray->used + numElements >= varray->size)
	{
		varray->size *= RESIZE_FACTOR;
		varray->buffer = Z_Realloc(varray->buffer, varray->size * sizeof(FOutVector), PU_STATIC, varray);
	}

	for (i = 0; i < numElements; i++)
	{
		varray->buffer[i + varray->used] = elements[i];
	}
	
	varray->used += numElements;
}
