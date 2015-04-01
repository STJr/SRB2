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
/// \brief Resizeable vertex array

#ifndef __HWR_VERTARRAY_H__
#define __HWR_VERTARRAY_H__

#include "hw_defs.h"

typedef struct
{
	FOutVector * buffer;
	unsigned int used;
	unsigned int size;
} FVertexArray;

void HWR_InitVertexArray(FVertexArray * varray, unsigned int initialSize);
void HWR_FreeVertexArray(FVertexArray * varray);

void HWR_InsertVertexArray(FVertexArray * varray, int numElements, FOutVector * elements);

#endif // __HWR_VERTARRAY_H__
