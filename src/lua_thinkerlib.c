// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_thinkerlib.c
/// \brief thinker library for Lua scripting

#include "doomdef.h"
#include "p_local.h"
#include "lua_script.h"
#include "lua_libs.h"

#define META_ITERATIONSTATE "iteration state"

/*static const char *const iter_opt[] = {
	"all",
	"mobj",
	NULL};

static const actionf_p1 iter_funcs[] = {
	NULL,
	(actionf_p1)P_MobjThinker
};*/

struct iterationState {
	actionf_p1 filter;
	int next;
};

static int iterationState_gc(lua_State *L)
{
	struct iterationState *it = luaL_checkudata(L, -1, META_ITERATIONSTATE);
	if (it->next != LUA_REFNIL)
	{
		luaL_unref(L, LUA_REGISTRYINDEX, it->next);
		it->next = LUA_REFNIL;
	}
	return 0;
}

#define push_thinker(th) {\
	if ((th)->function.acp1 == (actionf_p1)P_MobjThinker) \
		LUA_PushUserdata(L, (th), META_MOBJ); \
	else \
		lua_pushlightuserdata(L, (th)); \
}

static int lib_iterateThinkers(lua_State *L)
{
	thinker_t *th = NULL, *next = NULL;
	struct iterationState *it;

	INLEVEL

	it = luaL_checkudata(L, 1, META_ITERATIONSTATE);

	lua_settop(L, 2);

	if (lua_isnil(L, 2))
		th = &thlist[THINK_MOBJ];
	else if (lua_isuserdata(L, 2))
	{
		if (lua_islightuserdata(L, 2))
			th = lua_touserdata(L, 2);
		else
		{
			th = *(thinker_t **)lua_touserdata(L, -1);
			if (!th)
			{
				if (it->next == LUA_REFNIL)
					return 0;

				lua_rawgeti(L, LUA_REGISTRYINDEX, it->next);
				if (lua_islightuserdata(L, -1))
					next = lua_touserdata(L, -1);
				else
					next = *(thinker_t **)lua_touserdata(L, -1);
			}
		}
	}

	luaL_unref(L, LUA_REGISTRYINDEX, it->next);
	it->next = LUA_REFNIL;

	if (th && !next)
		next = th->next;
	if (!next)
		return luaL_error(L, "next thinker invalidated during iteration");

	for (; next != &thlist[THINK_MOBJ]; next = next->next)
		if (!it->filter || next->function.acp1 == it->filter)
		{
			push_thinker(next);
			if (next->next != &thlist[THINK_MOBJ])
			{
				push_thinker(next->next);
				it->next = luaL_ref(L, LUA_REGISTRYINDEX);
			}
			return 1;
		}
	return 0;
}

static int lib_startIterate(lua_State *L)
{
	struct iterationState *it;

	INLEVEL

	lua_pushvalue(L, lua_upvalueindex(1));
	it = lua_newuserdata(L, sizeof(struct iterationState));
	luaL_getmetatable(L, META_ITERATIONSTATE);
	lua_setmetatable(L, -2);

	it->filter = (actionf_p1)P_MobjThinker; //iter_funcs[luaL_checkoption(L, 1, "mobj", iter_opt)];
	it->next = LUA_REFNIL;
	return 2;
}

#undef push_thinker

int LUA_ThinkerLib(lua_State *L)
{
	luaL_newmetatable(L, META_ITERATIONSTATE);
	lua_pushcfunction(L, iterationState_gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);

	lua_createtable(L, 0, 1);
		lua_pushcfunction(L, lib_iterateThinkers);
		lua_pushcclosure(L, lib_startIterate, 1);
		lua_setfield(L, -2, "iterate");
	lua_setglobal(L, "mobjs");
	return 0;
}
