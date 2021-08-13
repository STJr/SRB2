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
#include "hu_stuff.h"
#include "i_system.h"

#include "lua_script.h"
#include "lua_libs.h"

///////////////
// FUNCTIONS //
///////////////

static int lib_gameControlDown(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= num_gamecontrols)
		return luaL_error(L, "gc_* constant %d out of range (0 - %d)", i, num_gamecontrols-1);
	lua_pushinteger(L, PLAYER1INPUTDOWN(i));
	return 1;
}

static int lib_gameControl2Down(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= num_gamecontrols)
		return luaL_error(L, "gc_* constant %d out of range (0 - %d)", i, num_gamecontrols-1);
	lua_pushinteger(L, PLAYER2INPUTDOWN(i));
	return 1;
}

static int lib_gameControlToKeyNum(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= num_gamecontrols)
		return luaL_error(L, "gc_* constant %d out of range (0 - %d)", i, num_gamecontrols-1);
	lua_pushinteger(L, gamecontrol[i][0]);
	lua_pushinteger(L, gamecontrol[i][1]);
	return 2;
}

static int lib_gameControl2ToKeyNum(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= num_gamecontrols)
		return luaL_error(L, "gc_* constant %d out of range (0 - %d)", i, num_gamecontrols-1);
	lua_pushinteger(L, gamecontrolbis[i][0]);
	lua_pushinteger(L, gamecontrolbis[i][1]);
	return 2;
}

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

static int lib_keyNumToString(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushstring(L, G_KeyNumToString(i));
	return 1;
}

static int lib_keyStringToNum(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	lua_pushinteger(L, G_KeyStringToNum(str));
	return 1;
}

static int lib_keyNumPrintable(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushboolean(L, i >= 32 && i <= 127);
	return 1;
}

static int lib_shiftKeyNum(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i >= 32 && i <= 127)
		lua_pushinteger(L, shiftxform[i]);
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

static int lib_getCursorPosition(lua_State *L)
{
	int x, y;
	I_GetCursorPosition(&x, &y);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 2;
}

static luaL_Reg lib[] = {
	{"gameControlDown", lib_gameControlDown},
	{"gameControl2Down", lib_gameControl2Down},
	{"gameControlToKeyNum", lib_gameControlToKeyNum},
	{"gameControl2ToKeyNum", lib_gameControl2ToKeyNum},
	{"joyAxis", lib_joyAxis},
	{"joy2Axis", lib_joy2Axis},
	{"keyNumToString", lib_keyNumToString},
	{"keyStringToNum", lib_keyStringToNum},
	{"keyNumPrintable", lib_keyNumPrintable},
	{"shiftKeyNum", lib_shiftKeyNum},
	{"getMouseGrab", lib_getMouseGrab},
	{"setMouseGrab", lib_setMouseGrab},
	{"getCursorPosition", lib_getCursorPosition},
	{NULL, NULL}
};

///////////////////
// gamekeydown[] //
///////////////////

static int lib_getGameKeyDown(lua_State *L)
{
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUMINPUTS)
		return luaL_error(L, "gamekeydown[] index %d out of range (0 - %d)", i, NUMINPUTS-1);
	lua_pushboolean(L, gamekeydown[i]);
	return 1;
}

static int lib_setGameKeyDown(lua_State *L)
{
	int i = luaL_checkinteger(L, 2);
	boolean j = luaL_checkboolean(L, 3);
	if (i < 0 || i >= NUMINPUTS)
		return luaL_error(L, "gamekeydown[] index %d out of range (0 - %d)", i, NUMINPUTS-1);
	gamekeydown[i] = j;
	return 0;
}

static int lib_lenGameKeyDown(lua_State *L)
{
	lua_pushinteger(L, NUMINPUTS);
	return 1;
}

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
	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getGameKeyDown);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_setGameKeyDown);
			lua_setfield(L, -2, "__newindex");

			lua_pushcfunction(L, lib_lenGameKeyDown);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "gamekeydown");

	luaL_newmetatable(L, META_MOUSE);
		lua_pushcfunction(L, mouse_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, mouse_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_register(L, "input", lib);
	return 0;
}
