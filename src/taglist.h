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

taggroup_t* tags_sectors[MAXTAGS + 1];
taggroup_t* tags_lines[MAXTAGS + 1];
taggroup_t* tags_mapthings[MAXTAGS + 1];

void Taggroup_Add (taggroup_t *garray[], const mtag_t tag, size_t id);
void Taggroup_Remove (taggroup_t *garray[], const mtag_t tag, size_t id);

void Taglist_InitGlobalTables(void);

INT32 Tag_Iterate_Sectors (const mtag_t tag, const size_t p);
INT32 Tag_Iterate_Lines (const mtag_t tag, const size_t p);
INT32 Tag_Iterate_Things (const mtag_t tag, const size_t p);

INT32 Tag_FindLineSpecial(const INT16 special, const INT16 tag);

#define TAG_ITER_C size_t kkkk;
#define TAG_ITER(fn, tag, id) for(kkkk = 0; (id = fn(tag, kkkk)) >= 0; kkkk++)

#define TAG_ITER_SECTORS(tag, id) TAG_ITER(Tag_Iterate_Sectors, tag, id)
#define TAG_ITER_LINES(tag, id)   TAG_ITER(Tag_Iterate_Lines, tag, id)
#define TAG_ITER_THINGS(tag, id)  TAG_ITER(Tag_Iterate_Things, tag, id)

#endif //__R_TAGLIST__
