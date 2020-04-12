#include "taglist.h"
#include "z_zone.h"

void Tag_Add (taglist_t* list, const UINT16 tag)
{
	list->tags = Z_Realloc(list->tags, (list->count + 1) * sizeof(list->tags), PU_LEVEL, NULL);
	list->tags[list->count++] = tag;
}
