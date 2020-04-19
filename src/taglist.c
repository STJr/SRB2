#include "taglist.h"
#include "z_zone.h"
#include "r_data.h"

void Tag_Add (taglist_t* list, const mtag_t tag)
{
	list->tags = Z_Realloc(list->tags, (list->count + 1) * sizeof(list->tags), PU_LEVEL, NULL);
	list->tags[list->count++] = tag;
}

/// Sets the first tag entry in a taglist.
void Tag_FSet (taglist_t* list, const mtag_t tag)
{
	if (!list->count)
	{
		Tag_Add(list, tag);
		return;
	}

	list->tags[0] = tag;
}

/// Gets the first tag entry in a taglist.
mtag_t Tag_FGet (const taglist_t* list)
{
	if (list->count)
		return list->tags[0];

	return 0;
}

boolean Tag_Find (const taglist_t* list, const mtag_t tag)
{
	size_t i;
	for (i = 0; i < list->count; i++)
		if (list->tags[i] == tag)
			return true;

	return false;
}

boolean Tag_Share (const taglist_t* list1, const taglist_t* list2)
{
	size_t i;
	for (i = 0; i < list1->count; i++)
		if (Tag_Find(list2, list1->tags[i]))
			return true;

	return false;
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


size_t Taggroup_Find (const taggroup_t *group, const size_t id)
{
	size_t i;

	if (!group)
		return -1;

	for (i = 0; i < group->count; i++)
		if (group->elements[i] == id)
			return i;

	return -1;
}

void Taggroup_Add (taggroup_t *garray[], const mtag_t tag, size_t id)
{
	taggroup_t *group;
	size_t i; // Insert position.

	if (tag == MTAG_GLOBAL)
		return;

	group = garray[(UINT16)tag];

	// Don't add duplicate entries.
	if (Taggroup_Find(group, id) != (size_t)-1)
		return;

	// Create group if empty.
	if (!group)
	{
		i = 0;
		group = garray[(UINT16)tag] = Z_Calloc(sizeof(taggroup_t), PU_LEVEL, NULL);
	}
	else
	{
		// Keep the group element ids in an ascending order.
		// Find the location to insert the element to.
		for (i = 0; i < group->count; i++)
			if (group->elements[i] > id)
				break;

		group->elements = Z_Realloc(group->elements, (group->count + 1) * sizeof(size_t), PU_LEVEL, NULL);

		// Offset existing elements to make room for the new one.
		if (i < group->count)
		{
			// Temporary memory block for copying.
			size_t size = group->count - i;
			size_t *temp = malloc(size);
			memcpy(temp, &group->elements[i], size);
			memcpy(&group->elements[i + 1], temp, size);
			free(temp);
		}
	}

	group->count++;
	group->elements = Z_Realloc(group->elements, group->count * sizeof(size_t), PU_LEVEL, NULL);
	group->elements[i] = id;
}

void Taggroup_Remove (taggroup_t *garray[], const mtag_t tag, size_t id)
{
	taggroup_t *group;
	size_t rempos;
	size_t newcount;

	if (tag == MTAG_GLOBAL)
		return;

	group = garray[(UINT16)tag];

	if ((rempos = Taggroup_Find(group, id)) == (size_t)-1)
		return;

	// Strip away taggroup if no elements left.
	if (!(newcount = --group->count))
	{
		Z_Free(group->elements);
		Z_Free(group);
		garray[(UINT16)tag] = NULL;
	}
	else
	{
		size_t *newelements = Z_Malloc(newcount * sizeof(size_t), PU_LEVEL, NULL);
		size_t i;

		// Copy the previous entries save for the one to remove.
		for (i = 0; i < rempos; i++)
			newelements[i] = group->elements[i];

		for (i = rempos + 1; i < group->count; i++)
			newelements[i - 1] = group->elements[i];

		Z_Free(group->elements);
		group->elements = newelements;
		group->count = newcount;
	}
}

// Initialization.

static void Taglist_AddToSectors (const mtag_t tag, const size_t itemid)
{
	Taggroup_Add(tags_sectors, tag, itemid);
}

static void Taglist_AddToLines (const mtag_t tag, const size_t itemid)
{
	Taggroup_Add(tags_lines, tag, itemid);
}

static void Taglist_AddToMapthings (const mtag_t tag, const size_t itemid)
{
	Taggroup_Add(tags_mapthings, tag, itemid);
}

void Taglist_InitGlobalTables(void)
{
	size_t i, j;

	for (i = 0; i < MAXTAGS; i++)
	{
		tags_sectors[i] = NULL;
		tags_lines[i] = NULL;
		tags_mapthings[i] = NULL;
	}
	for (i = 0; i < numsectors; i++)
	{
		for (j = 0; j < sectors[i].tags.count; j++)
			Taglist_AddToSectors(sectors[i].tags.tags[j], i);
	}
	for (i = 0; i < numlines; i++)
	{
		for (j = 0; j < lines[i].tags.count; j++)
			Taglist_AddToLines(lines[i].tags.tags[j], i);
	}
	for (i = 0; i < nummapthings; i++)
	{
		for (j = 0; j < mapthings[i].tags.count; j++)
			Taglist_AddToMapthings(mapthings[i].tags.tags[j], i);
	}
}

// Iteration, inagme search.

INT32 Tag_Iterate_Sectors (const mtag_t tag, const size_t p)
{
	if (tag == MTAG_GLOBAL)
	{
		if (p < numsectors)
			return p;
		return -1;
	}

	if (tags_sectors[(UINT16)tag])
	{
		if (p < tags_sectors[(UINT16)tag]->count)
			return tags_sectors[(UINT16)tag]->elements[p];
		return -1;
	}
	return -1;
}

INT32 Tag_Iterate_Lines (const mtag_t tag, const size_t p)
{
	if (tag == MTAG_GLOBAL)
	{
		if (p < numlines)
			return p;
		return -1;
	}

	if (tags_lines[(UINT16)tag])
	{
		if (p < tags_lines[(UINT16)tag]->count)
			return tags_lines[(UINT16)tag]->elements[p];
		return -1;
	}
	return -1;
}

INT32 Tag_Iterate_Things (const mtag_t tag, const size_t p)
{
	if (tag == MTAG_GLOBAL)
	{
		if (p < nummapthings)
			return p;
		return -1;
	}

	if (tags_mapthings[(UINT16)tag])
	{
		if (p < tags_mapthings[(UINT16)tag]->count)
			return tags_mapthings[(UINT16)tag]->elements[p];
		return -1;
	}
	return -1;
}

INT32 Tag_FindLineSpecial(const INT16 special, const mtag_t tag)
{
	INT32 i;

	if (tag == MTAG_GLOBAL)
	{
		for (i = 0; i < numlines; i++)
			if (lines[i].special == special)
				return i;
	}
	else if (tags_lines[(UINT16)tag])
	{
		taggroup_t *tagged = tags_lines[(UINT16)tag];
		for (i = 0; i < tagged->count; i++)
			if (lines[tagged->elements[i]].special == special)
				return tagged->elements[i];
	}
	return -1;
}

// Ingame list manipulation.

void Tag_SectorFSet (const size_t id, const mtag_t tag)
{
	sector_t* sec = &sectors[id];
	mtag_t curtag = Tag_FGet(&sec->tags);
	if (curtag == tag)
		return;

	Taggroup_Remove(tags_sectors, curtag, id);
	Taggroup_Add(tags_sectors, tag, id);
	Tag_FSet(&sec->tags, tag);
}
