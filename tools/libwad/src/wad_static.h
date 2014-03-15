// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// libwad: Doom WAD format interface library.
// Copyright (C) 2011 by Callum Dickinson.
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
/// \file wad_static.h
/// \brief Static header for internal libwad operations.

#ifndef __WAD_STATIC_H__
#define __WAD_STATIC_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "../include/wad.h"

void WAD_ErrorSet(const char *text);

inline void WAD_FreeMemory(FILE *file, wadheader_t *header, void *lumpdir, lump_t *lumps, wad_t *wad);

#endif // __WAD_STATIC_H__
