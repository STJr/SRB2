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

/// Multitag list. Each taggable element will have its own taglist.
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

/// Taggroup list. It is essentially just an element id list.
typedef struct
{
	size_t *elements;
	size_t count;
} taggroup_t;

extern bitarray_t tags_available[];

extern size_t num_tags;

extern taggroup_t* tags_sectors[];
extern taggroup_t* tags_lines[];
extern taggroup_t* tags_mapthings[];

void Taggroup_Add (taggroup_t *garray[], const mtag_t tag, size_t id);
void Taggroup_Remove (taggroup_t *garray[], const mtag_t tag, size_t id);
size_t Taggroup_Find (const taggroup_t *group, const size_t id);
size_t Taggroup_Count (const taggroup_t *group);

INT32 Taggroup_Iterate
(		taggroup_t *garray[],
		const size_t max_elements,
		const mtag_t tag,
		const size_t p);

void Taglist_InitGlobalTables(void);

INT32 Tag_Iterate_Sectors (const mtag_t tag, const size_t p);
INT32 Tag_Iterate_Lines (const mtag_t tag, const size_t p);
INT32 Tag_Iterate_Things (const mtag_t tag, const size_t p);

INT32 Tag_FindLineSpecial(const INT16 special, const mtag_t tag);
INT32 P_FindSpecialLineFromTag(INT16 special, INT16 tag, INT32 start);

// Use this macro to declare an iterator position variable.
#define TAG_ITER_DECLARECOUNTER(level) size_t ICNT_##level

#define TAG_ITER(level, fn, tag, return_varname) for(ICNT_##level = 0; (return_varname = fn(tag, ICNT_##level)) >= 0; ICNT_##level++)

// Use these macros as wrappers for a taglist iteration.
#define TAG_ITER_SECTORS(level, tag, return_varname) TAG_ITER(level, Tag_Iterate_Sectors, tag, return_varname)
#define TAG_ITER_LINES(level, tag, return_varname)   TAG_ITER(level, Tag_Iterate_Lines, tag, return_varname)
#define TAG_ITER_THINGS(level, tag, return_varname)  TAG_ITER(level, Tag_Iterate_Things, tag, return_varname)

/* ITERATION MACROS
TAG_ITER_DECLARECOUNTER must be used before using the iterators.

'level':
For each nested iteration, an additional TAG_ITER_DECLARECOUNTER
must be used with a different level number to avoid conflict with
the outer iterations.
Most cases don't have nested iterations and thus the level is just 0.

'tag':
Pretty much the elements' tag to iterate through.

'return_varname':
Target variable's name to return the iteration results to.


EXAMPLE:
{
	TAG_ITER_DECLARECOUNTER(0);
	TAG_ITER_DECLARECOUNTER(1); // For the nested iteration.

	size_t li;
	size_t sec;

	INT32 tag1 = 4;

	...

	TAG_ITER_LINES(0, tag1, li)
	{
		line_t *line = lines + li;

		...

		if (something)
		{
			mtag_t tag2 = 8;

			// Nested iteration; just make sure the level is higher
			// and that it has its own counter declared in scope.
			TAG_ITER_SECTORS(1, tag2, sec)
			{
				sector_t *sector = sectors + sec;

				...
			}
		}
	}
}

Notes:
If no elements are found for a given tag, the loop inside won't be executed.
*/

#endif //__R_TAGLIST__
