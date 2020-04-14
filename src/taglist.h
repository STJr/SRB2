#ifndef __R_TAGLIST__
#define __R_TAGLIST__

#include "doomtype.h"

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

INT32 Tag_Iterate_Sectors (const INT16 tag, const size_t p);
INT32 Tag_Iterate_Lines (const INT16 tag, const size_t p);
INT32 Tag_Iterate_Things (const INT16 tag, const size_t p);

INT32 Tag_FindLineSpecial(const INT16 tag, const INT16 special);

#define TAG_ITER_C size_t kkkk;
#define TAG_ITER(fn, tag, id) for(kkkk = 0; (id = fn(tag, kkkk)) >= 0; kkkk++)

#define TAG_ITER_SECTORS(tag, id) TAG_ITER(Tag_Iterate_Sectors, tag, id)
#define TAG_ITER_LINES(tag, id)   TAG_ITER(Tag_Iterate_Lines, tag, id)
#define TAG_ITER_THINGS(tag, id)  TAG_ITER(Tag_Iterate_Things, tag, id)

#endif //__R_TAGLIST__
