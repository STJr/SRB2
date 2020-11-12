// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C)      2020 by Nev3r.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  taglist.h
/// \brief Tag iteration and reading functions and macros' declarations.

#ifndef __R_TAGLIST__
#define __R_TAGLIST__

#include "doomtype.h"

typedef INT16 mtag_t;
#define MAXTAGS UINT16_MAX
#define MTAG_GLOBAL -1

/// Multitag list.
typedef struct
{
	mtag_t* tags;
	UINT16 count;
} taglist_t;

void Tag_Add (taglist_t* list, const mtag_t tag);

void Tag_FSet (taglist_t* list, const mtag_t tag);
mtag_t Tag_FGet (const taglist_t* list);
boolean Tag_Find (const taglist_t* list, const mtag_t tag);
boolean Tag_Share (const taglist_t* list1, const taglist_t* list2);

boolean Tag_Compare (const taglist_t* list1, const taglist_t* list2);

void Tag_SectorFSet (const size_t id, const mtag_t tag);

typedef struct
{
	size_t *elements;
	size_t count;
} taggroup_t;

extern taggroup_t* tags_sectors[];
extern taggroup_t* tags_lines[];
extern taggroup_t* tags_mapthings[];

void Taggroup_Add (taggroup_t *garray[], const mtag_t tag, size_t id);
void Taggroup_Remove (taggroup_t *garray[], const mtag_t tag, size_t id);
size_t Taggroup_Find (const taggroup_t *group, const size_t id);

void Taglist_InitGlobalTables(void);

INT32 Tag_Iterate_Sectors (const mtag_t tag, const size_t p);
INT32 Tag_Iterate_Lines (const mtag_t tag, const size_t p);
INT32 Tag_Iterate_Things (const mtag_t tag, const size_t p);

INT32 Tag_FindLineSpecial(const INT16 special, const mtag_t tag);
INT32 P_FindSpecialLineFromTag(INT16 special, INT16 tag, INT32 start);

// Use this macro to declare the iterator position variable.
#define TAG_ITER_DECLARECOUNTER size_t tag_iterator_pos

#define TAG_ITER(fn, tag, id) for(tag_iterator_pos = 0; (id = fn(tag, tag_iterator_pos)) >= 0; tag_iterator_pos++)

// Use these macros as wrappers for the taglist iterations.
#define TAG_ITER_SECTORS(tag, id) TAG_ITER(Tag_Iterate_Sectors, tag, id)
#define TAG_ITER_LINES(tag, id)   TAG_ITER(Tag_Iterate_Lines, tag, id)
#define TAG_ITER_THINGS(tag, id)  TAG_ITER(Tag_Iterate_Things, tag, id)

#endif //__R_TAGLIST__
