// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_interceptlib.c
/// \brief intercept library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_local.h"
#include "lua_script.h"
#include "lua_libs.h"

#include "lua_hud.h" // hud_running errors

#define NOHUD if (hud_running)\
return luaL_error(L, "HUD rendering code should not call this function!");

enum intercept_e {
	intercept_valid = 0,
	intercept_frac,
	intercept_thing,
	intercept_line
};

static const char *const intercept_opt[] = {
	"valid",
	"frac",
	"thing",
	"line",
	NULL};

static int intercept_fields_ref = LUA_NOREF;

static boolean Lua_PathTraverser(intercept_t *in)
{
	boolean traverse = false;
	I_Assert(in != NULL);
	
	lua_settop(gL, 6);
	lua_pushcfunction(gL, LUA_GetErrorMessage);
	
	I_Assert(lua_isfunction(gL, -2));
	
	lua_pushvalue(gL, -2);
	LUA_PushUserdata(gL, in, META_INTERCEPT);
	LUA_Call(gL, 1, 1, -3);
	
	traverse = lua_toboolean(gL, -2);
	lua_pop(gL, 1);
	//lua_settop(gL, 0);
	
	//CONS_Printf("%d\n", in->isaline);
	//CONS_Printf("%s\n", in->isaline ? "true" : "false");
	return traverse;
}

static int intercept_get(lua_State *L)
{
	intercept_t *in = *((intercept_t **)luaL_checkudata(L, 1, META_INTERCEPT));
	enum intercept_e field = Lua_optoption(L, 2, intercept_valid, intercept_fields_ref);
	
	if (!in)
	{
		if (field == intercept_valid) {
			lua_pushboolean(L, 0);
			return 1;
		}
		return luaL_error(L, "accessed intercept_t doesn't exist anymore.");
	}
	
	switch(field)
	{
	case intercept_valid: // valid
		lua_pushboolean(L, 1);
		return 1;
	case intercept_frac:
		lua_pushfixed(L, in->frac);
		return 1;
	case intercept_thing:
		if (in->isaline)
			return 0;
		LUA_PushUserdata(L, in->d.thing, META_MOBJ);
		return 1;
	case intercept_line:
		if (!in->isaline)
			return 0;
		LUA_PushUserdata(L, in->d.line, META_LINE);
		return 1;
	}
	return 0;
}

static int lib_pPathTraverse(lua_State *L)
{
	fixed_t px1 = luaL_checkfixed(L, 1);
	fixed_t px2 = luaL_checkfixed(L, 2);
	fixed_t py1 = luaL_checkfixed(L, 3);
	fixed_t py2 = luaL_checkfixed(L, 4);
	INT32 flags = (INT32)luaL_checkinteger(L, 5);
	luaL_checktype(L, 6, LUA_TFUNCTION);
	NOHUD
	INLEVEL
	lua_pushboolean(L, P_PathTraverse(px1, py1, px2, py2, flags, Lua_PathTraverser));
	return 1;
}

int LUA_InterceptLib(lua_State *L)
{
	LUA_RegisterUserdataMetatable(L, META_INTERCEPT, intercept_get, NULL, NULL);
	intercept_fields_ref = Lua_CreateFieldTable(L, intercept_opt);
	lua_register(L, "P_PathTraverse", lib_pPathTraverse);
	return 0;
}
