// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  dehacked.h
/// \brief Dehacked files.

#ifndef __DEHACKED_H__
#define __DEHACKED_H__

#include "m_fixed.h" // for get_number

typedef enum
{
	UNDO_NONE    = 0x00,
	UNDO_NEWLINE = 0x01,
	UNDO_SPACE   = 0x02,
	UNDO_CUTLINE = 0x04,
	UNDO_HEADER  = 0x07,
	UNDO_ENDTEXT = 0x08,
	UNDO_TODO = 0,
	UNDO_DONE = 0,
} undotype_f;

void DEH_LoadDehackedLump(lumpnum_t lumpnum);
void DEH_LoadDehackedLumpPwad(UINT16 wad, UINT16 lump, boolean mainfile);

fixed_t get_number(const char *word);
FUNCPRINTF void deh_warning(const char *first, ...);
void deh_strlcpy(char *dst, const char *src, size_t size, const char *warntext);

extern boolean deh_loaded;

extern boolean gamedataadded;
extern boolean titlechanged;
extern boolean introchanged;

#define MAXRECURSION 30
extern const char *superactions[MAXRECURSION];
extern UINT8 superstack;

// If the dehacked patch does not match this version, we throw a warning
#define PATCHVERSION 220

#define MAXLINELEN 1024

// the code was first write for a file
// converted to use memory with this functions
typedef struct
{
	char *data;
	char *curpos;
	size_t size;
	UINT16 wad;
} MYFILE;
#define myfeof(a) (a->data + a->size <= a->curpos)
char *myfgets(char *buf, size_t bufsize, MYFILE *f);
char *myhashfgets(char *buf, size_t bufsize, MYFILE *f);
#endif
