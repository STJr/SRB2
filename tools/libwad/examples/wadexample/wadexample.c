// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// wadexample: libwad example program.
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
/// \file wadconv.c
/// \brief Converts wads made for version 2.0 to 2.1.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wad.h"

int main(int argc, char **argv)
{
	wad_uint32_t i;

	wad_t *w;
	lump_t *l;

	if (argc < 2)
	{
		puts("Usage: wadexample [filename] [..]");
		puts("Example: example wadfile.wad wadfile2.wad");
		return EXIT_FAILURE;
	}

	// Open the WAD files.
	puts("Opening WADs...");
	for (i = 1; i < (unsigned)argc; i++)
	{
		if (!WAD_OpenWAD(argv[i]))
		{
			printf("Error opening WAD %s: %s\n", argv[i], WAD_Error());
			return 1;
		}
	}

	for (i = 0; i < wad_numwads; i = WAD_WADLoopAdvance(i))
	{
		w = WAD_WADByNum(i);
		printf("List of lumps inside wad %s:\n", WAD_BaseName(w->filename));
		for (l = w->lumps; l; l = l->next)
			printf("Lump #%i: %s\n", l->num, l->name);
	}

	puts("Closing WADs...");
	for (i = 0; i < wad_numwads; i++)
		WAD_CloseWADByNum(i);

	return EXIT_SUCCESS;
}
