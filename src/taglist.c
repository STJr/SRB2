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
