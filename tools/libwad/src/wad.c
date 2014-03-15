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
/// \file wad.c
/// \brief WAD interface functionality.

#include "wad_static.h"

//
// Offset for the first lump's data is always 12 bytes.
//
#define LUMPOFFSET 12

//
// Maximum of wad files used at the same time.
// (There is a max of simultaneous open files anyway, and this should be plenty.)
//
#define MAX_WADFILES 48

wad_t *wads[MAX_WADFILES];
wad_uint32_t wad_numwads;

wad_uint32_t wad_oldnumwads;
wad_uint32_t wad_removednum;

static inline void WAD_ChangeWADNum(wad_int32_t i, wad_t *wad)
{
	wads[i] = wad;
}

static inline void WAD_CacheWAD(wad_t *wad)
{
	WAD_ChangeWADNum(wad_numwads, wad);
	wad_oldnumwads = wad_numwads;
	wad_numwads++;
}

static void WAD_UncacheWAD(wad_t *wad)
{
	wad_uint32_t i;

	for (i = 0; i < wad_numwads; i++)
		if (WAD_WADByNum(i) == wad)
			break;

	WAD_ChangeWADNum(i, NULL);
	wad_removednum = i;

	for (; i < wad_numwads; i++)
	{
		if (WAD_WADByNum(i+1))
		{
			WAD_ChangeWADNum(i, WAD_WADByNum(i+1));
			WAD_ChangeWADNum(i+1, NULL);
		}
	}

	wad_oldnumwads = wad_numwads;
	wad_numwads--;
}

wad_uint32_t WAD_WADLoopAdvance(wad_uint32_t i)
{
	wad_uint32_t o;

	o = wad_oldnumwads;
	wad_oldnumwads = wad_numwads;

	return (o > wad_numwads && i <= wad_removednum) ?  i : i + 1;
}

wad_t *WAD_WADByNum(wad_uint16_t num)
{
	return (wads[num]) ? (wad_t *)wads[num] : NULL;
}

wad_t *WAD_WADByFileName(const char *filename)
{
	wad_uint32_t i;
	wad_t *w;

	for (i = 0; i < wad_numwads; i = WAD_WADLoopAdvance(i))
	{
		w = WAD_WADByNum(i);
		if (!strcmp(filename, w->filename))
			return w;
	}

	return NULL;
}

//
// Allocate a wadfile, setup the lumpinfo (directory) and
// lumpcache, add the wadfile to the current active wadfiles
//
// 1: Reached the maximum WAD number.
// 2: Cannot open the file.
// 3: Cannot read the WAD file's header.
// 4: Invalid WAD ID.
// 5: WAD's lump directory is corrupt.
// 6: Corrupt compressed WAD.
//
wad_t *WAD_OpenWAD(const char *filename)
{
	size_t i;
	// Pointer to the WAD.
	wad_t *wad = NULL;
	// WAD file structure information.
	// wad->filename is the function's argument.
	FILE *file;			// wad->file
	wadheader_t *header;		// wad->header
	lump_t *l, *lumps = NULL;	// wad->lumps
	// Temporary lump information.
	wad_uint64_t position;
	lumpdir_t *lumpdir;
	lump_t *ln = NULL;

	//
	// Check if the limit of maximum WAD files has been
	// reached or not.
	//
	if (wad_numwads >= MAX_WADFILES)
		return NULL;

	//
	// Open the WAD file.
	//
	if (!(file = fopen(filename, "rb")))
		return NULL;

	//
	// Read the WAD file's header.
	//
	header = malloc(sizeof(*header));
	if (fread(header, 1, sizeof(*header), file) < sizeof(*header))
		return NULL;

	// Check if the ID is valid.
	if (memcmp(header->id, "IWAD", 4) && memcmp(header->id, "PWAD", 4) && memcmp(header->id, "SDLL", 4))
	{
		WAD_FreeMemory(file, header, NULL, NULL, NULL);
		return NULL;
	}

	// Check the rest of the header for type.
	header->numlumps = WAD_INT32_Endianness(header->numlumps);
	header->lumpdir = WAD_INT32_Endianness(header->lumpdir);

	//
	// Read the WAD file's lump directory.
	//
	i = header->numlumps * sizeof(*lumpdir);
	lumpdir = malloc(i);

	if (fseek(file, header->lumpdir, SEEK_SET) < 0 || fread(lumpdir, i, 1, file) < 1)
	{
		WAD_FreeMemory(file, header, lumpdir, NULL, NULL);
		return NULL;
	}

	//
	// Fill lump information structure.
	//
	for (i = 0; i < header->numlumps; i++, lumpdir++)
	{
		if (!(l = malloc(sizeof(*l))))
		{
			WAD_FreeMemory(file, header, lumpdir, lumps, NULL);
			return NULL;
		}

		position = WAD_INT32_Endianness(lumpdir->position);

		strncpy(l->name, lumpdir->name, 8);
		l->size = l->disksize = WAD_INT32_Endianness(lumpdir->size);
		l->compressed = false; // TODO: Compressed WAD support.

		if (l->disksize)
		{
			fseek(file, position, SEEK_SET);
			if (!(l->data = malloc(l->disksize)) || fread(l->data, l->disksize, 1, file) < 1)
			{
				WAD_FreeMemory(file, header, lumpdir, lumps, NULL);

				if (!l->data)
					return NULL;
				else
					return NULL;
			}
		}

		l->num = i;
		l->next = NULL;
		if (!lumps)
		{
			l->prev = NULL;
			ln = lumps = l;
		}
		else
		{
			l->prev = ln;
			ln = ln->next = l;
		}
	}
	//free(lumpdir);

	//
	// Allocate and fill WAD entry.
	//
	if (!(wad = malloc(sizeof(*wad))))
	{
		WAD_FreeMemory(file, header, NULL, lumps, NULL);
		return NULL;
	}
	wad->filename = (char *)filename;

	wad->header = header;
	wad->compressed = false;

	wad->lumps = lumps;

	for (l = wad->lumps; l; l = l->next)
		l->wad = wad;

	fclose(file);

	//
	// Add the WAD to the wadfiles buffer and wad_numwads number.
	//
	WAD_CacheWAD(wad);

	//
	// Done, loaded the WAD file.
	//
	return wad;
}

// 1: Invalid wad number.
// 2: Unable to open file for writing.
// 3: Unable to write wad ID to the header.
// 4: Unable to write number of lumps to the header.
// 5: Unable to write temporary data to the header.
// 6: Unable to write lump data.
// 7: Unable to write position of lump data.
// 8: Unable to Write size of lump data.
// 9: Unable to write lump name.
// 9: Unable to write directory position.
wad_int32_t WAD_SaveWAD(wad_t *wad)
{
	lump_t *l;
	FILE *file;
	char *newfilename;
	wad_int64_t diroffset, lumpoffset;

	if (!wad)
		return 1;

	newfilename = WAD_VA("%s%s", wad->filename, ".bak");

	remove(newfilename);
	rename(wad->filename, newfilename);

	if (!(file = fopen(wad->filename, "wb")))
		return 2;

	//
	// Write four-character WAD ID.
	//
	if(fwrite(wad->header->id, 4, 1, file) < 1)
		return 3;

	//
	// Write number of lumps.
	//
	if (fwrite(&wad->header->numlumps, 4, 1, file) < 1)
		return 4;

	//
	// Offset of directory is not known yet. For now, write number of lumps
	// again, just to fill the space.
	//
	if (fwrite(&wad->header->numlumps, 4, 1, file) < 1)
		return 5;

	//
	// Loop through lump list, writing lump data.
	//
	for (l = wad->lumps; l; l = l->next)
	{
		// Don't write anything for the head of the lump list or for lumps of
		// zero length.
		if (!l->data)
			continue;

		// Write the data.
		if (fwrite(l->data, l->disksize, 1, file) < 1)
			return 6;
	}

	//
	// Current position is where directory will start.
	//
	diroffset = ftell(file);

	//
	// Pointer to the offset for the first lump's data.
	//
	lumpoffset = LUMPOFFSET;

	//
	// Loop through lump list again, this time writing directory entries.
	//
	for (l = wad->lumps; l; l = l->next)
	{
		// Write position of lump data.
		if (fwrite(&lumpoffset, 4, 1, file) < 1)
			return 7;

		// Write size of lump data.
		if (fwrite(&(l->disksize), 4, 1, file) < 1)
			return 8;

		// Write lump name.
		if (fwrite(&(l->name), 8, 1, file) < 1)
			return 9;

		// Increment lumpoffset variable as appropriate.
		lumpoffset += l->disksize;
	}

	//
	// Go back to header and write the proper directory position.
	//
	fseek(file, 8, SEEK_SET);
	if (fwrite(&diroffset, 4, 1, file) < 1)
		return 10;

	//
	// Done, saved the WAD file.
	//
	return 0;
}

inline wad_int32_t WAD_SaveWADByNum(wad_uint16_t num)
{
	return WAD_SaveWAD(WAD_WADByNum(num));
}

inline wad_int32_t WAD_SaveWADByFileName(const char *filename)
{
	return WAD_SaveWAD(WAD_WADByFileName(filename));
}

inline wad_int32_t WAD_SaveCloseWAD(wad_t *wad)
{
	wad_int32_t ret;

	if (!(ret = WAD_SaveWAD(wad)))
		return ret;

	WAD_CloseWAD(wad);
	return ret;
}

inline wad_int32_t WAD_SaveCloseWADByNum(wad_uint16_t num)
{
	return WAD_SaveCloseWAD(WAD_WADByNum(num));
}

inline wad_int32_t WAD_SaveCloseWADByFileName(const char *filename)
{
	return WAD_SaveCloseWAD(WAD_WADByFileName(filename));
}

void WAD_CloseWAD(wad_t *wad)
{
	if (!wad)
		return;

	//
	// Free memory used by the WAD.
	//
	WAD_FreeMemory(NULL, wad->header, NULL, wad->lumps, wad);

	//
	// Remove references to the WAD.
	//
	WAD_UncacheWAD(wad);
}

inline void WAD_CloseWADByNum(wad_uint16_t num)
{
	WAD_CloseWAD(WAD_WADByNum(num));
}

inline void WAD_CloseWADByFileName(const char *name)
{
	WAD_CloseWAD(WAD_WADByFileName(name));
}
