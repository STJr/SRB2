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
/// \file lump.c
/// \brief Lump interface functionality.

#include "wad_static.h"

//
// Create a lump name from the file name
// of a lump. Used for when no name for
// a lump was given.
//
char *WAD_LumpNameFromFileName(char *path)
{
	wad_int32_t i;
	char *name, *tempname, *file = WAD_BaseName(path);

	if (!file)
		return NULL;

	//
	// Process the filename for a suitable lump name.
	//
	// Get rid of any hidden file period.
	if (file[0] == '.')
	{
		if (!(tempname = malloc(sizeof(*file) * strlen(&file[1]))))
		{
			free(file);
			return NULL;
		}
		strcpy(tempname, &file[1]);
		free(file);
	}
	else
		tempname = file;

	// Get rid of any extensions.
	for (i = 0; tempname[i] != '.' && tempname[i] != '\0'; i++);

	// Then, take the first eight characters of
	// the resulting filename to get the lump name.
	if (i > 8)
		i = 8;

	if (!(name = malloc(sizeof(*name) * (i+1))))
	{
		free(tempname);
		return NULL;
	}

	strncpy(name, tempname, i);
	name[i+1] = '\0';

	//
	// Done processing lump name.
	//
	free(tempname);
	return name;
}

lump_t *WAD_LumpInWADByNum(wad_t *wad, wad_uint32_t num)
{
	lump_t *lump;

	if (!wad || !num)
		return NULL;

	for (lump = wad->lumps; lump; lump = lump->next)
		if (num == lump->num)
			return lump;

	return NULL;
}

lump_t *WAD_LumpInWADByName(wad_t *wad, const char *name)
{
	lump_t *lump;

	if (!wad || !name)
		return NULL;

	for (lump = wad->lumps; lump; lump = lump->next)
		if (!strcmp(name, lump->name))
			return lump;

	return NULL;
}

lump_t *WAD_LumpByName(const char *name)
{
	wad_uint32_t i;
	lump_t *lump;

	for (i = 0; i < wad_numwads; i = WAD_WADLoopAdvance(i))
		if ((lump = WAD_LumpInWADByName(WAD_WADByNum(i), name)))
			return lump;

	return NULL;
}

inline wad_uint32_t WAD_LumpNum(lump_t *lump)
{
	return lump->num;
}

inline wad_uint32_t WAD_LumpNumInWADByName(wad_t *wad, const char *name)
{
	return WAD_LumpNum(WAD_LumpInWADByName(wad, name));
}

inline wad_uint32_t WAD_LumpNumByName(const char *name)
{
	return WAD_LumpNum(WAD_LumpByName(name));
}

inline size_t WAD_LumpSize(lump_t *lump)
{
	return lump->size;
}

inline size_t WAD_LumpSizeInWADByName(wad_t *wad, const char *name)
{
	return WAD_LumpSize(WAD_LumpInWADByName(wad, name));
}

inline size_t WAD_LumpSizeByName(const char *name)
{
	return WAD_LumpSize(WAD_LumpByName(name));
}

static void WAD_AddLumpToTree(lump_t *lump, lump_t *destlump)
{
	lump_t *l;

	if (destlump)
	{
		lump->wad = destlump->wad;
		lump->num = destlump->num;
		for (l = destlump; l; l = l->next)
			l->num++;
		if (destlump->prev)
			destlump->prev->next = lump;
		lump->prev = destlump->prev;
		destlump->prev = lump;
		lump->next = destlump;
	}
	else
	{
		for (l = lump->wad->lumps; l; l = l->next)
			if (!l->next)
				break;
		lump->num = lump->wad->header->numlumps;
		l->next = lump;
		lump->prev = l;
		lump->next = NULL;
	}
	lump->wad->header->numlumps++;
}

lump_t *WAD_AddLump(wad_t *wad, lump_t *destlump, const char *name, const char *filename)
{
	// The open lump file.
	FILE *file = NULL;
	// Pointer to the lump.
	lump_t *lump;
	// Lump structure.
	wad_uint64_t disksize;	// lump->disksize
	size_t size;		// lump->size
	wad_boolean_t compressed;// lump->compressed
	void *data;		// lump->data

	if (!wad)
		return NULL;

	if (!name && !filename)
		return NULL;

	if (filename)
	{
		if (!(file = fopen(filename, "rb")))
			return NULL;

		fseek(file, 0, SEEK_END);
		disksize = size = (unsigned)ftell(file);
		compressed = false;


		if (!(data = malloc(size)))
		{
			fclose(file);
			return NULL;
		}

		fseek(file, 0, SEEK_SET);
		if (fread(data, disksize, 1, file) < 1)
			return NULL;
		fclose(file);
	}
	else
	{
		disksize = size = 0;
		compressed = false;
		data = NULL;
	}


	if (!(lump = malloc(sizeof(*lump))))
	{
		WAD_FreeMemory(NULL, NULL, NULL, lump, NULL);
		return NULL;
	}

	lump->wad = wad;
	if (!strcpy(lump->name, (name) ? name : WAD_LumpNameFromFileName((char *)filename)))
	{
		WAD_FreeMemory(NULL, NULL, NULL, lump, NULL);
		return NULL;
	}
	lump->disksize = disksize;
	lump->size = size;
	lump->compressed = compressed;

	lump->data = data;

	WAD_AddLumpToTree(lump, destlump);

	return lump;
}

inline lump_t *WAD_AddLumpInWADByNum(wad_t *wad, wad_uint32_t num, const char *name, const char *filename)
{
	return WAD_AddLump(wad, WAD_LumpInWADByNum(wad, num), name, filename);
}

inline lump_t *WAD_AddLumpInWADByName(wad_t *wad, const char *destname, const char *name, const char *filename)
{
	return WAD_AddLump(wad, WAD_LumpInWADByName(wad, destname), name, filename);
}

inline lump_t *WAD_AddLumpByName(const char *destname, const char *name, const char *filename)
{
	lump_t *l = WAD_LumpByName(destname);
	return WAD_AddLump(l->wad, l, name, filename);
}

lump_t *WAD_CacheLump(lump_t *lump)
{
	return lump; // TODO: WAD_CacheLump, dynamic lump loading.
}

inline lump_t *WAD_CacheLumpInWADByNum(wad_t *wad, wad_uint32_t num)
{
	return WAD_CacheLump(WAD_LumpInWADByNum(wad, num));
}

inline lump_t *WAD_CacheLumpInWADByName(wad_t *wad, const char *name)
{
	return WAD_CacheLump(WAD_LumpInWADByName(wad, name));
}

inline lump_t *WAD_CacheLumpByName(const char *name)
{
	return WAD_CacheLump(WAD_LumpByName(name));
}

lump_t *WAD_MoveLump(lump_t *lump, lump_t *destlump)
{
	lump_t *l;

	if (!destlump)
		return NULL;

	//
	// Link the previous and next nodes in the WAD tree with each other.
	//
	if (lump->next)
	{
		lump->next->prev = lump->prev;
		for (l = lump->next; l; l = l->next)
			l->num--;
	}
	if (lump->prev)
		lump->prev->next = lump->next;
	lump->wad->header->numlumps--;

	//
	WAD_AddLumpToTree(lump, destlump);

	return lump;
}

inline lump_t *WAD_MoveLumpInWADByNum(wad_t *wad, wad_uint32_t num, wad_t *destwad, wad_uint32_t destnum)
{
	return WAD_MoveLump(WAD_LumpInWADByNum(wad, num), WAD_LumpInWADByNum(destwad, destnum));
}

inline lump_t *WAD_MoveLumpInWADByName(wad_t *wad, const char *name, wad_t *destwad, const char *destname)
{
	return WAD_MoveLump(WAD_LumpInWADByName(wad, name), WAD_LumpInWADByName(destwad, destname));
}

inline lump_t *WAD_MoveLumpByName(const char *name, const char *destname)
{
	return WAD_MoveLump(WAD_LumpByName(name), WAD_LumpByName(destname));
}

void WAD_UncacheLump(lump_t *lump)
{
	(void)lump; // TODO: WAD_UncacheLump, dynamic lump loading.
}

inline void WAD_UncacheLumpInWADByNum(wad_t *wad, wad_uint32_t num)
{
	WAD_UncacheLump(WAD_LumpInWADByNum(wad, num));
}

inline void WAD_UncacheLumpInWADByName(wad_t *wad, const char *name)
{
	WAD_UncacheLump(WAD_LumpInWADByName(wad, name));
}

inline void WAD_UncacheLumpByName(const char *name)
{
	WAD_UncacheLump(WAD_LumpByName(name));
}

void WAD_RemoveLump(lump_t *lump)
{
	lump_t *l;

	if (!lump)
		return;

	//
	// Link the previous and next nodes in the WAD tree with each other.
	//
	lump->next->prev = lump->prev;
	lump->prev->next = lump->next;

	//
	// Modify the lump numbers for all the subsequent lumps,
	// and the total lumps in the WAD.
	//
	lump->wad->header->numlumps--;

	if (lump->num == 0)
		lump->wad->lumps = lump->next;

	for (l = lump->next; l; l = l->next)
		l->num--;

	//
	// While, at the same time, blanking the references
	// to these nodes so they don't get removed.
	//
	lump->next = NULL;

	//
	// Free this lump's memory.
	//
	WAD_FreeMemory(NULL, NULL, NULL, lump, NULL);
}

inline void WAD_RemoveLumpInWADByNum(wad_t *wad, wad_uint32_t num)
{
	WAD_RemoveLump(WAD_LumpInWADByNum(wad, num));
}

inline void WAD_RemoveLumpInWADByName(wad_t *wad, const char *name)
{
	WAD_RemoveLump(WAD_LumpInWADByName(wad, name));
}

inline void WAD_RemoveLumpByName(const char *name)
{
	WAD_RemoveLump(WAD_LumpByName(name));
}
