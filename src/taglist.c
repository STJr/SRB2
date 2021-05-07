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
/// \file  taglist.c
/// \brief Ingame sector/line/mapthing tagging.

#include "taglist.h"
#include "z_zone.h"
#include "r_data.h"

// Bit array of whether a tag exists for sectors/lines/things.
bitarray_t tags_available[BIT_ARRAY_SIZE (MAXTAGS)];

size_t num_tags;

// Taggroups are used to list elements of the same tag, for iteration.
// Since elements can now have multiple tags, it means an element may appear
// in several taggroups at the same time. These are built on level load.
taggroup_t* tags_sectors[MAXTAGS + 1];
taggroup_t* tags_lines[MAXTAGS + 1];
taggroup_t* tags_mapthings[MAXTAGS + 1];

/// Adds a tag to a given element's taglist.
/// \warning This does not rebuild the global taggroups, which are used for iteration.
void Tag_Add (taglist_t* list, const mtag_t tag)
{
	list->tags = Z_Realloc(list->tags, (list->count + 1) * sizeof(list->tags), PU_LEVEL, NULL);
	list->tags[list->count++] = tag;
}

/// Sets the first tag entry in a taglist.
/// Replicates the old way of accessing element->tag.
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
/// Replicates the old way of accessing element->tag.
mtag_t Tag_FGet (const taglist_t* list)
{
	if (list->count)
		return list->tags[0];

	return 0;
}

/// Returns true if the given tag exist inside the list.
boolean Tag_Find (const taglist_t* list, const mtag_t tag)
{
	size_t i;
	for (i = 0; i < list->count; i++)
		if (list->tags[i] == tag)
			return true;

	return false;
}

/// Returns true if at least one tag is shared between two given lists.
boolean Tag_Share (const taglist_t* list1, const taglist_t* list2)
{
	size_t i;
	for (i = 0; i < list1->count; i++)
		if (Tag_Find(list2, list1->tags[i]))
			return true;

	return false;
}

/// Returns true if both lists are identical.
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

/// Search for an element inside a global taggroup.
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

/// group->count, but also checks for NULL
size_t Taggroup_Count (const taggroup_t *group)
{
	return group ? group->count : 0;
}

/// Iterate thru elements in a global taggroup.
INT32 Taggroup_Iterate
(		taggroup_t *garray[],
		const size_t max_elements,
		const mtag_t tag,
		const size_t p)
{
	const taggroup_t *group;

	if (tag == MTAG_GLOBAL)
	{
		if (p < max_elements)
			return p;
		return -1;
	}

	group = garray[(UINT16)tag];

	if (group)
	{
		if (p < group->count)
			return group->elements[p];
		return -1;
	}
	return -1;
}

/// Add an element to a global taggroup.
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

	if (! in_bit_array(tags_available, tag))
	{
		num_tags++;
		set_bit_array(tags_available, tag);
	}

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
	}

	group->elements = Z_Realloc(group->elements, (group->count + 1) * sizeof(size_t), PU_LEVEL, NULL);

	// Offset existing elements to make room for the new one.
	if (i < group->count)
		memmove(&group->elements[i + 1], &group->elements[i], group->count - i);

	group->count++;
	group->elements[i] = id;
}

static size_t total_elements_with_tag (const mtag_t tag)
{
	return
		(
				Taggroup_Count(tags_sectors[tag]) +
				Taggroup_Count(tags_lines[tag]) +
				Taggroup_Count(tags_mapthings[tag])
		);
}

/// Remove an element from a global taggroup.
void Taggroup_Remove (taggroup_t *garray[], const mtag_t tag, size_t id)
{
	taggroup_t *group;
	size_t rempos;
	size_t oldcount;

	if (tag == MTAG_GLOBAL)
		return;

	group = garray[(UINT16)tag];

	if ((rempos = Taggroup_Find(group, id)) == (size_t)-1)
		return;

	if (group->count == 1 && total_elements_with_tag(tag) == 1)
	{
		num_tags--;
		unset_bit_array(tags_available, tag);
	}

	// Strip away taggroup if no elements left.
	if (!(oldcount = group->count--))
	{
		Z_Free(group->elements);
		Z_Free(group);
		garray[(UINT16)tag] = NULL;
	}
	else
	{
		size_t *newelements = Z_Malloc(group->count * sizeof(size_t), PU_LEVEL, NULL);
		size_t i;

		// Copy the previous entries save for the one to remove.
		for (i = 0; i < rempos; i++)
			newelements[i] = group->elements[i];

		for (i = rempos + 1; i < oldcount; i++)
			newelements[i - 1] = group->elements[i];

		Z_Free(group->elements);
		group->elements = newelements;
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

/// After all taglists have been built for each element (sectors, lines, things),
/// the global taggroups, made for iteration, are built here.
void Taglist_InitGlobalTables(void)
{
	size_t i, j;

	memset(tags_available, 0, sizeof tags_available);
	num_tags = 0;

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

// Iteration, ingame search.

INT32 Tag_Iterate_Sectors (const mtag_t tag, const size_t p)
{
	return Taggroup_Iterate(tags_sectors, numsectors, tag, p);
}

INT32 Tag_Iterate_Lines (const mtag_t tag, const size_t p)
{
	return Taggroup_Iterate(tags_lines, numlines, tag, p);
}

INT32 Tag_Iterate_Things (const mtag_t tag, const size_t p)
{
	return Taggroup_Iterate(tags_mapthings, nummapthings, tag, p);
}

INT32 Tag_FindLineSpecial(const INT16 special, const mtag_t tag)
{
	size_t i;

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

/// Backwards compatibility iteration function for Lua scripts.
INT32 P_FindSpecialLineFromTag(INT16 special, INT16 tag, INT32 start)
{
	if (tag == -1)
	{
		start++;

		if (start >= (INT32)numlines)
			return -1;

		while (start < (INT32)numlines && lines[start].special != special)
			start++;

		return start;
	}
	else
	{
		size_t p = 0;
		INT32 id;

		// For backwards compatibility's sake, simulate the old linked taglist behavior:
		// Iterate through the taglist and find the "start" line's position in the list,
		// And start checking with the next one (if it exists).
		if (start != -1)
		{
			for (; (id = Tag_Iterate_Lines(tag, p)) >= 0; p++)
				if (id == start)
				{
					p++;
					break;
				}
		}

		for (; (id = Tag_Iterate_Lines(tag, p)) >= 0; p++)
			if (lines[id].special == special)
				return id;

		return -1;
	}
}


// Ingame list manipulation.

/// Changes the first tag for a given sector, and updates the global taggroups.
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
