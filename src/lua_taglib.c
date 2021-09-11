// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by James R.
// Copyright (C) 2020-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_taglib.c
/// \brief tag list iterator for Lua scripting

#include "doomdef.h"
#include "taglist.h"
#include "r_state.h"

#include "lua_script.h"
#include "lua_libs.h"

#ifdef MUTABLE_TAGS
#include "z_zone.h"
#endif

static int tag_iterator(lua_State *L)
{
	INT32 tag = lua_isnil(L, 2) ? -1 : lua_tonumber(L, 2);
	do
	{
		if (++tag >= MAXTAGS)
			return 0;
	}
	while (! in_bit_array(tags_available, tag)) ;
	lua_pushnumber(L, tag);
	return 1;
}

enum {
#define UPVALUE lua_upvalueindex
	up_garray         = UPVALUE(1),
	up_max_elements   = UPVALUE(2),
	up_element_array  = UPVALUE(3),
	up_sizeof_element = UPVALUE(4),
	up_meta           = UPVALUE(5),
#undef UPVALUE
};

static INT32 next_element(lua_State *L, const mtag_t tag, const size_t p)
{
	taggroup_t ** garray = lua_touserdata(L, up_garray);
	const size_t * max_elements = lua_touserdata(L, up_max_elements);
	return Taggroup_Iterate(garray, *max_elements, tag, p);
}

static void push_element(lua_State *L, void *element)
{
	if (LUA_RawPushUserdata(L, element) == LPUSHED_NEW)
	{
		lua_pushvalue(L, up_meta);
		lua_setmetatable(L, -2);
	}
}

static void push_next_element(lua_State *L, const INT32 element)
{
	char * element_array = *(char **)lua_touserdata(L, up_element_array);
	const size_t sizeof_element = lua_tonumber(L, up_sizeof_element);
	push_element(L, &element_array[element * sizeof_element]);
}

struct element_iterator_state {
	mtag_t tag;
	size_t p;
};

static int element_iterator(lua_State *L)
{
	struct element_iterator_state * state = lua_touserdata(L, 1);
	if (lua_isnoneornil(L, 3))
		state->p = 0;
	lua_pushnumber(L, ++state->p);
	lua_gettable(L, 1);
	return 1;
}

static int lib_iterateTags(lua_State *L)
{
	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, tag_iterator);
		return 1;
	}
	else
		return tag_iterator(L);
}

static int lib_numTags(lua_State *L)
{
	lua_pushnumber(L, num_tags);
	return 1;
}

static int lib_getTaggroup(lua_State *L)
{
	struct element_iterator_state *state;

	mtag_t tag;

	if (lua_gettop(L) > 1)
		return luaL_error(L, "too many arguments");

	if (lua_isnoneornil(L, 1))
	{
		tag = MTAG_GLOBAL;
	}
	else
	{
		tag = lua_tonumber(L, 1);
		luaL_argcheck(L, tag >= -1, 1, "tag out of range");
	}

	state = lua_newuserdata(L, sizeof *state);
	state->tag = tag;
	state->p = 0;

	lua_pushvalue(L, lua_upvalueindex(1));
	lua_setmetatable(L, -2);

	return 1;
}

static int lib_getTaggroupElement(lua_State *L)
{
	const size_t p = luaL_checknumber(L, 2) - 1;
	const mtag_t tag = *(mtag_t *)lua_touserdata(L, 1);
	const INT32 element = next_element(L, tag, p);

	if (element == -1)
		return 0;
	else
	{
		push_next_element(L, element);
		return 1;
	}
}

static int lib_numTaggroupElements(lua_State *L)
{
	const mtag_t tag = *(mtag_t *)lua_touserdata(L, 1);
	if (tag == MTAG_GLOBAL)
		lua_pushnumber(L, *(size_t *)lua_touserdata(L, up_max_elements));
	else
	{
		const taggroup_t ** garray = lua_touserdata(L, up_garray);
		lua_pushnumber(L, Taggroup_Count(garray[tag]));
	}
	return 1;
}

#ifdef MUTABLE_TAGS
static int meta_ref[2];
#endif

static int has_valid_field(lua_State *L)
{
	int equal;
	lua_rawgeti(L, LUA_ENVIRONINDEX, 1);
	equal = lua_rawequal(L, 2, -1);
	lua_pop(L, 1);
	return equal;
}

static taglist_t * valid_taglist(lua_State *L, int idx, boolean getting)
{
	taglist_t *list = *(taglist_t **)lua_touserdata(L, idx);

	if (list == NULL)
	{
		if (getting && has_valid_field(L))
			lua_pushboolean(L, 0);
		else
			LUA_ErrInvalid(L, "taglist");/* doesn't actually return */
		return NULL;
	}
	else
		return list;
}

static taglist_t * check_taglist(lua_State *L, int idx)
{
	if (lua_isuserdata(L, idx) && lua_getmetatable(L, idx))
	{
		lua_getref(L, meta_ref[0]);
		lua_getref(L, meta_ref[1]);

		if (lua_rawequal(L, -3, -2) || lua_rawequal(L, -3, -1))
		{
			lua_pop(L, 3);
			return valid_taglist(L, idx, false);
		}
	}

	return luaL_argerror(L, idx, "must be a tag list"), NULL;
}

static int taglist_get(lua_State *L)
{
	const taglist_t *list = valid_taglist(L, 1, true);

	if (list == NULL)/* valid check */
		return 1;

	if (lua_isnumber(L, 2))
	{
		const size_t i = lua_tonumber(L, 2);

		if (list && i <= list->count)
		{
			lua_pushnumber(L, list->tags[i - 1]);
			return 1;
		}
		else
			return 0;
	}
	else if (has_valid_field(L))
	{
		lua_pushboolean(L, 1);
		return 1;
	}
	else
	{
		lua_getmetatable(L, 1);
		lua_replace(L, 1);
		lua_rawget(L, 1);
		return 1;
	}
}

static int taglist_len(lua_State *L)
{
	const taglist_t *list = valid_taglist(L, 1, false);
	lua_pushnumber(L, list->count);
	return 1;
}

static int taglist_equal(lua_State *L)
{
	const taglist_t *lhs = check_taglist(L, 1);
	const taglist_t *rhs = check_taglist(L, 2);
	lua_pushboolean(L, Tag_Compare(lhs, rhs));
	return 1;
}

static int taglist_iterator(lua_State *L)
{
	const taglist_t *list = valid_taglist(L, 1, false);
	const size_t i = 1 + lua_tonumber(L, lua_upvalueindex(1));
	if (i <= list->count)
	{
		lua_pushnumber(L, list->tags[i - 1]);
		/* watch me exploit an upvalue as a control because
			I want to use the control as the value */
		lua_pushnumber(L, i);
		lua_replace(L, lua_upvalueindex(1));
		return 1;
	}
	else
		return 0;
}

static int taglist_iterate(lua_State *L)
{
	check_taglist(L, 1);
	lua_pushnumber(L, 0);
	lua_pushcclosure(L, taglist_iterator, 1);
	lua_pushvalue(L, 1);
	return 2;
}

static int taglist_find(lua_State *L)
{
	const taglist_t *list = check_taglist(L, 1);
	const mtag_t tag = luaL_checknumber(L, 2);
	lua_pushboolean(L, Tag_Find(list, tag));
	return 1;
}

static int taglist_shares(lua_State *L)
{
	const taglist_t *lhs = check_taglist(L, 1);
	const taglist_t *rhs = check_taglist(L, 2);
	lua_pushboolean(L, Tag_Share(lhs, rhs));
	return 1;
}

/* only sector tags are mutable... */

#ifdef MUTABLE_TAGS
static size_t sector_of_taglist(taglist_t *list)
{
	return (sector_t *)((char *)list - offsetof (sector_t, tags)) - sectors;
}

static int this_taglist(lua_State *L)
{
	lua_settop(L, 1);
	return 1;
}

static int taglist_add(lua_State *L)
{
	taglist_t *list = *(taglist_t **)luaL_checkudata(L, 1, META_SECTORTAGLIST);
	const mtag_t tag = luaL_checknumber(L, 2);

	if (! Tag_Find(list, tag))
	{
		Taggroup_Add(tags_sectors, tag, sector_of_taglist(list));
		Tag_Add(list, tag);
	}

	return this_taglist(L);
}

static int taglist_remove(lua_State *L)
{
	taglist_t *list = *(taglist_t **)luaL_checkudata(L, 1, META_SECTORTAGLIST);
	const mtag_t tag = luaL_checknumber(L, 2);

	size_t i;

	for (i = 0; i < list->count; ++i)
	{
		if (list->tags[i] == tag)
		{
			if (list->count > 1)
			{
				memmove(&list->tags[i], &list->tags[i + 1],
						(list->count - 1 - i) * sizeof (mtag_t));
				list->tags = Z_Realloc(list->tags,
						(--list->count) * sizeof (mtag_t), PU_LEVEL, NULL);
				Taggroup_Remove(tags_sectors, tag, sector_of_taglist(list));
			}
			else/* reset to default tag */
				Tag_SectorFSet(sector_of_taglist(list), 0);
			break;
		}
	}

	return this_taglist(L);
}
#endif/*MUTABLE_TAGS*/

void LUA_InsertTaggroupIterator
(		lua_State *L,
		taggroup_t *garray[],
		size_t * max_elements,
		void * element_array,
		size_t sizeof_element,
		const char * meta)
{
	lua_createtable(L, 0, 3);
		lua_pushlightuserdata(L, garray);
		lua_pushlightuserdata(L, max_elements);

		lua_pushvalue(L, -2);
		lua_pushvalue(L, -2);
		lua_pushlightuserdata(L, element_array);
		lua_pushnumber(L, sizeof_element);
		luaL_getmetatable(L, meta);
		lua_pushcclosure(L, lib_getTaggroupElement, 5);
		lua_setfield(L, -4, "__index");

		lua_pushcclosure(L, lib_numTaggroupElements, 2);
		lua_setfield(L, -2, "__len");

		lua_pushcfunction(L, element_iterator);
		lua_setfield(L, -2, "__call");
	lua_pushcclosure(L, lib_getTaggroup, 1);
	lua_setfield(L, -2, "tagged");
}

static luaL_Reg taglist_lib[] = {
	{"iterate", taglist_iterate},
	{"find", taglist_find},
	{"shares", taglist_shares},
#ifdef MUTABLE_TAGS
	{"add", taglist_add},
	{"remove", taglist_remove},
#endif
	{0}
};

static void open_taglist(lua_State *L)
{
	luaL_register(L, "taglist", taglist_lib);

	lua_getfield(L, -1, "find");
	lua_setfield(L, -2, "has");
}

#define new_literal(L, s) \
	(lua_pushliteral(L, s), luaL_ref(L, -2))

#ifdef MUTABLE_TAGS
static int
#else
static void
#endif
set_taglist_metatable(lua_State *L, const char *meta)
{
	luaL_newmetatable(L, meta);
		lua_pushcfunction(L, taglist_get);
		lua_createtable(L, 0, 1);
			new_literal(L, "valid");
		lua_setfenv(L, -2);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, taglist_len);
		lua_setfield(L, -2, "__len");

		lua_pushcfunction(L, taglist_equal);
		lua_setfield(L, -2, "__eq");
#ifdef MUTABLE_TAGS
	return luaL_ref(L, LUA_REGISTRYINDEX);
#endif
}

int LUA_TagLib(lua_State *L)
{
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_createtable(L, 0, 1);
				lua_pushcfunction(L, lib_iterateTags);
				lua_setfield(L, -2, "iterate");
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_numTags);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "tags");

	open_taglist(L);

#ifdef MUTABLE_TAGS
	meta_ref[0] = set_taglist_metatable(L, META_TAGLIST);
	meta_ref[1] = set_taglist_metatable(L, META_SECTORTAGLIST);
#else
	set_taglist_metatable(L, META_TAGLIST);
#endif

	return 0;
}
