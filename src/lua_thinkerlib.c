// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_thinkerlib.c
/// \brief thinker library for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "p_local.h"
#include "lua_script.h"
#include "lua_libs.h"

static const char *const iter_opt[] = {
	"all",
	"mobj",
	NULL};

static int lib_iterateThinkers(lua_State *L)
{
	int state = luaL_checkoption(L, 1, "mobj", iter_opt);

	thinker_t *th = NULL;
	actionf_p1 searchFunc;
	const char *searchMeta;

	lua_settop(L, 2);
	lua_remove(L, 1); // remove state now.

	switch(state)
	{
		case 0:
			searchFunc = NULL;
			searchMeta = NULL;
			break;
		case 1:
		default:
			searchFunc = (actionf_p1)P_MobjThinker;
			searchMeta = META_MOBJ;
			break;
	}

	if (!lua_isnil(L, 1)) {
		if (lua_islightuserdata(L, 1))
			th = (thinker_t *)lua_touserdata(L, 1);
		else if (searchMeta)
			th = *((thinker_t **)luaL_checkudata(L, 1, searchMeta));
		else
			th = *((thinker_t **)lua_touserdata(L, 1));
	} else
		th = &thinkercap;

	if (!th) // something got our userdata invalidated!
		return 0;

	if (searchFunc == NULL)
	{
		if ((th = th->next) != &thinkercap)
		{
			if (th->function.acp1 == (actionf_p1)P_MobjThinker)
				LUA_PushUserdata(L, th, META_MOBJ);
			else
				lua_pushlightuserdata(L, th);
			return 1;
		}
		return 0;
	}

	for (th = th->next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != searchFunc)
			continue;

		LUA_PushUserdata(L, th, searchMeta);
		return 1;
	}
	return 0;
}

static int lib_startIterate(lua_State *L)
{
	luaL_checkoption(L, 1, iter_opt[0], iter_opt);
	lua_pushcfunction(L, lib_iterateThinkers);
	lua_pushvalue(L, 1);
	return 2;
}

int LUA_ThinkerLib(lua_State *L)
{
	lua_createtable(L, 0, 1);
		lua_pushcfunction(L, lib_startIterate);
		lua_setfield(L, -2, "iterate");
	lua_setglobal(L, "thinkers");
	return 0;
}

#endif
