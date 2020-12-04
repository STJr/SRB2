// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by James R.
// Copyright (C) 2020 by Sonic Team Junior.
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
	const INT32 element = next_element(L, state->tag, state->p);

	if (element == -1)
		return 0;
	else
	{
		push_next_element(L, element);
		state->p++;
		return 1;
	}
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

static int lib_numTaggroupElements(lua_State *L)
{
	const mtag_t tag = *(mtag_t *)lua_touserdata(L, 1);
	if (tag == MTAG_GLOBAL)
		lua_pushnumber(L, *(size_t *)lua_touserdata(L, up_max_elements));
	else
	{
		const taggroup_t ** garray = lua_touserdata(L, up_garray);
		lua_pushnumber(L, garray[tag] ? garray[tag]->count : 0);
	}
	return 1;
}

void LUA_InsertTaggroupIterator
(		lua_State *L,
		taggroup_t *garray[],
		size_t * max_elements,
		void * element_array,
		size_t sizeof_element,
		const char * meta)
{
	lua_createtable(L, 0, 2);
		lua_pushlightuserdata(L, garray);
		lua_pushlightuserdata(L, max_elements);

		lua_pushvalue(L, -2);
		lua_pushvalue(L, -2);
		lua_pushlightuserdata(L, element_array);
		lua_pushnumber(L, sizeof_element);
		luaL_getmetatable(L, meta);
		lua_pushcclosure(L, element_iterator, 5);
		lua_setfield(L, -4, "__call");

		lua_pushcclosure(L, lib_numTaggroupElements, 2);
		lua_setfield(L, -2, "__len");
	lua_pushcclosure(L, lib_getTaggroup, 1);
	lua_setfield(L, -2, "tagged");
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

	return 0;
}
