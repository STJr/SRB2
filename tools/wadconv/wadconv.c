// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// SRB2 WAD Converter
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

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
	wad_int16_t originx, originy;
	wad_int16_t patch, stepdir, colormap;
} mappatch_t;

//
// Texture definition.
// An SRB2 wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
	char name[8];
	wad_int32_t masked;
	wad_int16_t width;
	wad_int16_t height;
	wad_int32_t columndirectory; // FIXTHIS: OBSOLETE
	wad_int16_t patchcount;
	mappatch_t patches[1];
} maptexture_t;

// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct
{
	// Block origin (always UL), which has already accounted for the internal origin of the patch.
	wad_uint16_t originx, originy;
	lump_t *patch;
} texpatch_t;

// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.
typedef struct
{
	// Keep name for switch changing, etc.
	char name[8];
	wad_int16_t width, height;

	// All the patches[patchcount] are drawn back to front into the cached texture.
	wad_uint16_t patchcount;
	texpatch_t patches[1];
} texture_t;

static void ConvertTextures(wad_t *wad)
{
	wad_uint32_t i, j;
	lump_t *l;
	// PNAMES lump.
	char pname[9];
	char *pnamesdata;
	lump_t *pnames, **patchlookup;
	wad_uint32_t numpatches;
	// TEXTURE lump.
	lump_t *texturex = NULL;
	texture_t **textures;
	texpatch_t *patch;
	maptexture_t *mtexture;
	mappatch_t *mpatch;
	wad_uint32_t *directory, *maptex, numtextures, offset;
	size_t maxoffset;
	// TX_START.
	lump_t *tx_start;

	// PNAMES.
	if (!(pnames = WAD_CacheLumpInWADByName(wad, "PNAMES")))
	{
		printf("error caching lump: %s\n", WAD_Error());
		exit(EXIT_FAILURE);
	}

	pnamesdata = pnames->data + 4;

	memcpy(&numpatches, pnames->data, sizeof(wad_uint32_t));
	numpatches = WAD_INT32_Endianness(numpatches);

	if (!(patchlookup = malloc(sizeof(*patchlookup) * numpatches)))
	{
		printf("unable to allocate patchlookup array for wad %s\n", WAD_BaseName(wad->filename));
		exit(EXIT_FAILURE);
	}

	pname[8] = '\0';
	for (i = 0; i < numpatches; i++)
	{
		strncpy(pname, pnamesdata+i*8, 8);
		patchlookup[i] = WAD_LumpInWADByName(wad, pname);
	}

	WAD_UncacheLump(pnames);

	// TEXTURE.

	for (i = 1; (texturex = WAD_LumpInWADByName(wad, WAD_VA("TEXTURE%i", i))); i++)
		if (texturex)
			break;

	texturex = WAD_CacheLump(texturex);
	maptex = (wad_uint32_t *)texturex->data;
	numtextures = WAD_INT32_Endianness(*maptex);
	maxoffset = WAD_LumpSize(texturex);
	directory = maptex + 1;
	textures = malloc(sizeof(*textures) * numtextures);

	for (i = 0; i < numtextures; i++, directory++)
	{
		memcpy(&offset, directory, sizeof(wad_uint32_t));
		offset = WAD_INT32_Endianness(offset);

		if (offset > maxoffset)
		{
			printf("bad texture directory for wad %s\n", WAD_BaseName(wad->filename));
			exit(EXIT_FAILURE);
		}

		mtexture = (maptexture_t *)((wad_uint8_t *)maptex + offset);
		textures[i] = malloc(sizeof(texture_t) + (sizeof(texpatch_t) * (WAD_INT16_Endianness(mtexture->patchcount) - 1)));

		memcpy(textures[i]->name, mtexture->name, sizeof(textures[i]->name));
		textures[i]->width = WAD_INT16_Endianness(mtexture->width);
		textures[i]->height = WAD_INT16_Endianness(mtexture->height);
		textures[i]->patchcount = WAD_INT16_Endianness(mtexture->patchcount);

		mpatch = &mtexture->patches[0];
		patch = &textures[i]->patches[0];

		for (j = 0; j < textures[i]->patchcount; j++, mpatch++, patch++)
		{
			patch->originx = WAD_INT16_Endianness(mpatch->originx);
			patch->originy = WAD_INT16_Endianness(mpatch->originy);
			patch->patch = patchlookup[WAD_INT16_Endianness(mpatch->patch)];
			if (!patch->patch)
				printf("missing patch in texture %s in wad %s\n", textures[i]->name, wad->filename);
		}
	}

	WAD_UncacheLump(texturex);

	if (!(tx_start = WAD_LumpInWADByName(wad, "TX_START")))
	{
		if (WAD_LumpInWADByName(wad, "P_END"))
			tx_start = WAD_AddLumpInWADByNum(wad, WAD_LumpNumByName("P_END") + 1, "TX_START", NULL);
		else
			tx_start = WAD_AddLump(wad, NULL, "TX_START", NULL);

		if (!tx_start)
		{
			printf("unable to find TX_START in %s\n", wad->filename);
			exit(EXIT_FAILURE);
		}
	}
	WAD_RemoveLumpInWADByName(wad, "TX_END");

	for (i = 0, l = WAD_LumpInWADByNum(wad, WAD_LumpNum(tx_start) + 1); i < numtextures; i++, directory++)
	{
		if (textures[i]->patchcount == 1 && !strncmp(textures[i]->name, textures[i]->patches[0].patch->name, 8))
			WAD_MoveLump(textures[i]->patches[0].patch, l);
		else
		{
			FILE *socfile;
			char *soc, *socname = textures[i]->name;

			for (j = 0; j < textures[i]->patchcount; j++)
			{
				if (!strncmp(textures[i]->name, textures[i]->patches[j].patch->name, 8))
				{
					wad_int32_t k;
					char socname1[5];

					socname = malloc(sizeof(*socname) * 9);

					socname1[4] = '\0';
					strncpy(socname1, textures[i]->name, 4);

					for (k = 0 ;; k++)
						if (!WAD_LumpInWADByName(wad, WAD_VA((k > 9) ? "%sSC%i" : "%sSC0%i", socname1, k)))
							break;

					sprintf(socname, (k > 9) ? "%sSC%i" : "%sSC0%i", socname1, k);
					break;
				}
			}

			soc = WAD_VA("%s%s", socname, ".lmp");

			remove(soc);
			socfile = fopen(soc, "wb");

			fprintf(socfile, "TEXTURE %s\n", textures[i]->name);
			fprintf(socfile, "WIDTH = %i\n", textures[i]->width);
			fprintf(socfile, "HEIGHT = %i\n", textures[i]->height);
			fprintf(socfile, "NUMPATCHES = %i\n\n", textures[i]->patchcount);

			for (j = 0; j < textures[i]->patchcount; j++)
			{
				fprintf(socfile, "PATCH %s\n", textures[i]->patches[j].patch->name);
				fprintf(socfile, "X = %i\n", textures[i]->patches[j].originx);
				fprintf(socfile, "Y = %i\n\n", textures[i]->patches[j].originy);
			}

			fclose(socfile);

			WAD_AddLump(wad, l, socname, soc);
			remove(soc);
		}
	}
	WAD_AddLump(wad, l, "TX_END", NULL);

	WAD_RemoveLump(pnames);
	WAD_RemoveLump(texturex);
}

int main(int argc, char **argv)
{
	wad_t *w;
	wad_uint32_t i;

	if (argc < 2)
	{
		puts("Usage: wadconv [filename]");
		puts("Example: wadconv wadfile.wad");
		return EXIT_FAILURE;
	}

	// Open the WAD files.
	for (i = 1; i < (unsigned)argc; i++)
	{
		if (!WAD_OpenWAD(argv[i]))
		{
			printf("Loading WAD failed: %s\n", WAD_Error());
			return EXIT_FAILURE;
		}
	}

	for (i = 0; i < wad_numwads; i = WAD_WADLoopAdvance(i))
	{
		w = WAD_WADByNum(i);
		// Convert textures.
		printf("Converting textures for wad %s...\n", WAD_BaseName(w->filename));
		ConvertTextures(w);
	}

	for (i = 0; i < wad_numwads; i = WAD_WADLoopAdvance(i))
		WAD_SaveCloseWADByNum(i);

	puts("Finished.");
	return EXIT_SUCCESS;
}
