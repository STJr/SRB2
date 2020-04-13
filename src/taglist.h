#include "doomtype.h"

#ifndef __R_TAGLIST__
#define __R_TAGLIST__

/// Multitag list.
typedef struct
{
	UINT16* tags;
	UINT16 count;
} taglist_t;

void Tag_Add (taglist_t* list, const UINT16 tag);
boolean Tag_Compare (const taglist_t* list1, const taglist_t* list2);

typedef struct
{
	size_t *elements;
	size_t count;
} taggroup_t;

#define MAXTAGS 65536
taggroup_t* tags_sectors[MAXTAGS];
taggroup_t* tags_lines[MAXTAGS];
taggroup_t* tags_mapthings[MAXTAGS];

void Taglist_AddToSectors (const size_t tag, const size_t itemid);
void Taglist_AddToLines (const size_t tag, const size_t itemid);
void Taglist_AddToMapthings (const size_t tag, const size_t itemid);
#endif //__R_TAGLIST__

#define TAG_ITER_C size_t kkkk;

#define TAG_ITER(group, tag, id)\
if (group[tag])\
	for(id = group[tag]->elements[kkkk = 0] = 0;\
	kkkk < group[tag]->count;\
	id = group[tag]->elements[++kkkk])

#define TAG_ITER_SECTORS(tag, id) TAG_ITER(tags_sectors, tag, id)
#define TAG_ITER_LINES(tag, id)   TAG_ITER(tags_lines, tag, id)
#define TAG_ITER_THINGS(tag, id)  TAG_ITER(tags_mapthings, tag, id)
