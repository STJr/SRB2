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
/// \file wad_static.c
/// \brief Static functions for internal libwad operations.

#include "wad_static.h"

#define WAD_MAXERROR 1024
static char wad_error[WAD_MAXERROR];

void WAD_ErrorSet(const char *text)
{
	strncpy(wad_error, text, WAD_MAXERROR);
	wad_error[WAD_MAXERROR] = '\0';
}

inline void WAD_FreeMemory(FILE *file, wadheader_t *header, void *lumpdir, lump_t *lumps, wad_t *wad)
{
	lump_t *l, *ln = NULL;

	if (file)
		fclose(file);
	if (header)
		free(header);
	if (lumpdir)
		free(lumpdir);

	if (lumps)
	{
		for (l = lumps; l; l = ln)
		{
			ln = l->next;
			free(l->data);
			free(l);
		}
	}

	if (wad)
	{
		if (wad->filename)
			free(wad->filename);

		free(wad);
	}
}

char *WAD_Error(void)
{
	return wad_error;
}

char *WAD_BaseName(char *path)
{
	wad_int32_t i;

	//
	// Check if there is no text to check.
	//
	if (!path || path[0] == '\0')
		return NULL;

	//
	// Find the buffer entry where the actual filename begins.
	//
	for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);

	return (i >= 0) ? &path[i+1] : path;
}

char *WAD_VA(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

inline wad_int16_t WAD_INT16_Endianness(wad_int16_t x)
{
#ifdef _BIG_ENDIAN
	return ((wad_int16_t)((((wad_uint16_t)(x) & (wad_uint16_t)0x00ffU) << 8) | (((wad_uint16_t)(x) & (wad_uint16_t)0xff00U) >> 8)));
#else
	return (wad_int16_t)x;
#endif
}

inline wad_int32_t WAD_INT32_Endianness(wad_int32_t x)
{
#ifdef _BIG_ENDIAN
	return ((wad_int32_t)(\
	(((wad_uint32_t)(x) & (wad_uint32_t)0x000000ffUL) << 24) |\
	(((wad_uint32_t)(x) & (wad_uint32_t)0x0000ff00UL) <<  8) |\
	(((wad_uint32_t)(x) & (wad_uint32_t)0x00ff0000UL) >>  8) |\
	(((wad_uint32_t)(x) & (wad_uint32_t)0xff000000UL) >> 24)));
#else
	return (wad_int32_t)x;
#endif
}
