#include "taglist.h"
#include "z_zone.h"
#include "r_data.h"

void Tag_Add (taglist_t* list, const UINT16 tag)
{
	list->tags = Z_Realloc(list->tags, (list->count + 1) * sizeof(list->tags), PU_LEVEL, NULL);
	list->tags[list->count++] = tag;
}

boolean Tag_Compare (const taglist_t* list1, const taglist_t* list2)
{
	size_t i;

	if (list1->count != list2->count)
		return false;

	for (i = 0; i < list1->count; i++)
	if (list1->tags[i] != list2->tags[i])
		return false;

	return true;
}

void Taglist_AddToSectors (const size_t tag, const size_t itemid)
{
	taggroup_t* tagelems;
	if (!tags_sectors[tag])
		tags_sectors[tag] = Z_Calloc(sizeof(taggroup_t), PU_LEVEL, NULL);

	tagelems = tags_sectors[tag];
	tagelems->count++;
	tagelems->elements = Z_Realloc(tagelems->elements, tagelems->count * sizeof(size_t), PU_LEVEL, NULL);
	tagelems->elements[tagelems->count - 1] = itemid;
}

void Taglist_AddToLines (const size_t tag, const size_t itemid)
{
	taggroup_t* tagelems;
	if (!tags_lines[tag])
		tags_lines[tag] = Z_Calloc(sizeof(taggroup_t), PU_LEVEL, NULL);

	tagelems = tags_lines[tag];
	tagelems->count++;
	tagelems->elements = Z_Realloc(tagelems->elements, tagelems->count * sizeof(size_t), PU_LEVEL, NULL);
	tagelems->elements[tagelems->count - 1] = itemid;
}

void Taglist_AddToMapthings (const size_t tag, const size_t itemid)
{
	taggroup_t* tagelems;
	if (!tags_mapthings[tag])
		tags_mapthings[tag] = Z_Calloc(sizeof(taggroup_t), PU_LEVEL, NULL);

	tagelems = tags_mapthings[tag];
	tagelems->count++;
	tagelems->elements = Z_Realloc(tagelems->elements, tagelems->count * sizeof(size_t), PU_LEVEL, NULL);
	tagelems->elements[tagelems->count - 1] = itemid;
}

INT32 Tag_Iterate_Sectors (const INT16 tag, const size_t p)
{
	if (tag == -1)
	{
		if (p < numsectors)
			return p;
		return -1;
	}

	if (tags_sectors[tag])
	{
		if (p < tags_sectors[tag]->count)
			return tags_sectors[tag]->elements[p];
		return -1;
	}
	return -1;
}

INT32 Tag_Iterate_Lines (const INT16 tag, const size_t p)
{
	if (tag == -1)
	{
		if (p < numlines)
			return p;
		return -1;
	}

	if (tags_lines[tag])
	{
		if (p < tags_lines[tag]->count)
			return tags_lines[tag]->elements[p];
		return -1;
	}
	return -1;
}

INT32 Tag_Iterate_Things (const INT16 tag, const size_t p)
{
	if (tag == -1)
	{
		if (p < nummapthings)
			return p;
		return -1;
	}

	if (tags_mapthings[tag])
	{
		if (p < tags_mapthings[tag]->count)
			return tags_mapthings[tag]->elements[p];
		return -1;
	}
	return -1;
}

INT32 Tag_FindLineSpecial(const INT16 tag, const INT16 special)
{
	TAG_ITER_C
	INT32 i;

	TAG_ITER_LINES(tag, i)
	{
		if (i == -1)
			return -1;

		if (lines[i].special == special)
			return i;
	}
	return -1;
}
