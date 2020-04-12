#include "taglist.h"
#include "z_zone.h"

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
