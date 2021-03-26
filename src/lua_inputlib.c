// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_inputlib.c
/// \brief input library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "g_input.h"
#include "g_game.h"
#include "i_system.h"

#include "lua_script.h"
#include "lua_libs.h"

///////////////
// FUNCTIONS //
///////////////

static int lib_joyAxis(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushinteger(L, JoyAxis(i));
	return 1;
}

static int lib_joy2Axis(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushinteger(L, Joy2Axis(i));
	return 1;
}

static int lib_getMouseGrab(lua_State *L)
{
	lua_pushboolean(L, I_GetMouseGrab());
	return 1;
}

static int lib_setMouseGrab(lua_State *L)
{
	boolean grab = luaL_checkboolean(L, 1);
	I_SetMouseGrab(grab);
	return 0;
}

static boolean lib_getCursorPosition(lua_State *L)
{
	int x, y;
	I_GetCursorPosition(&x, &y);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 2;
}

static luaL_Reg lib[] = {
	{"G_JoyAxis", lib_joyAxis},
	{"G_Joy2Axis", lib_joy2Axis},
	{"I_GetMouseGrab", lib_getMouseGrab},
	{"I_SetMouseGrab", lib_setMouseGrab},
	{"I_GetCursorPosition", lib_getCursorPosition},
	{NULL, NULL}
};

///////////
// MOUSE //
///////////

static int mouse_get(lua_State *L)
{
	mouse_t *m = *((mouse_t **)luaL_checkudata(L, 1, META_MOUSE));
	const char *field = luaL_checkstring(L, 2);

	I_Assert(m != NULL);

	if (fastcmp(field,"dx"))
		lua_pushinteger(L, m->dx);
	else if (fastcmp(field,"dy"))
		lua_pushinteger(L, m->dy);
	else if (fastcmp(field,"mlookdy"))
		lua_pushinteger(L, m->mlookdy);
	else if (fastcmp(field,"rdx"))
		lua_pushinteger(L, m->rdx);
	else if (fastcmp(field,"rdy"))
		lua_pushinteger(L, m->rdy);
	else if (fastcmp(field,"buttons"))
		lua_pushinteger(L, m->buttons);
	else
		return luaL_error(L, "mouse_t has no field named %s", field);

	return 1;
}

// #mouse -> 1 or 2
static int mouse_num(lua_State *L)
{
	mouse_t *m = *((mouse_t **)luaL_checkudata(L, 1, META_MOUSE));

	I_Assert(m != NULL);

	lua_pushinteger(L, m == &mouse ? 1 : 2);
	return 1;
}

int LUA_InputLib(lua_State *L)
{
	luaL_newmetatable(L, META_MOUSE);
		lua_pushcfunction(L, mouse_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, mouse_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	// Set global functions
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}
