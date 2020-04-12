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

#define Tag_IterateSectors(tag, sc)\
size_t kk;\
if(tags_sectors[tag])\
	for(kk = 0, sc = tags_sectors[tag]->elements[0];\
		kk < tags_sectors[tag]->count;\
		sc = tags_sectors[tag]->elements[++kk])

#define Tag_IterateLines(tag, li)\
size_t kk;\
if(tags_lines[tag])\
	for(kk = 0, li = tags_lines[tag]->elements[0];\
		kk < tags_lines[tag]->count;\
		li = tags_lines[tag]->elements[++kk])

#define Tag_IterateMapthings(tag, mt)\
size_t kk;\
if(tags_mapthings[tag])\
	for(kk = 0, mt = tags_mapthings[tag]->elements[0];\
		kk < tags_mapthings[tag]->count;\
		mt = tags_mapthings[tag]->elements[++kk])
