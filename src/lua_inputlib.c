// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021-2022 by Sonic Team Junior.
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
#include "i_gamepad.h"

#include "lua_script.h"
#include "lua_libs.h"

boolean mousegrabbedbylua = true;

///////////////
// FUNCTIONS //
///////////////

static int lib_gameControlDown(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= NUM_GAMECONTROLS)
		return luaL_error(L, "GC_* constant %d out of range (0 - %d)", i, NUM_GAMECONTROLS-1);
	lua_pushinteger(L, G_PlayerInputDown(0, i));
	return 1;
}

static int lib_gameControl2Down(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= NUM_GAMECONTROLS)
		return luaL_error(L, "GC_* constant %d out of range (0 - %d)", i, NUM_GAMECONTROLS-1);
	lua_pushinteger(L, G_PlayerInputDown(1, i));
	return 1;
}

static int lib_gameControlToKeyNum(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= NUM_GAMECONTROLS)
		return luaL_error(L, "GC_* constant %d out of range (0 - %d)", i, NUM_GAMECONTROLS-1);
	lua_pushinteger(L, gamecontrol[i][0]);
	lua_pushinteger(L, gamecontrol[i][1]);
	return 2;
}

static int lib_gameControl2ToKeyNum(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	if (i < 0 || i >= NUM_GAMECONTROLS)
		return luaL_error(L, "GC_* constant %d out of range (0 - %d)", i, NUM_GAMECONTROLS-1);
	lua_pushinteger(L, gamecontrolbis[i][0]);
	lua_pushinteger(L, gamecontrolbis[i][1]);
	return 2;
}

static int lib_joyAxis(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushinteger(L, G_JoyAxis(0, i) / 32);
	return 1;
}

static int lib_joy2Axis(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushinteger(L, G_JoyAxis(1, i) / 32);
	return 1;
}

static int lib_keyNumToName(lua_State *L)
{
	int i = luaL_checkinteger(L, 1);
	lua_pushstring(L, G_KeyNumToName(i));
	return 1;
}

static int lib_keyNameToNum(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	lua_pushinteger(L, G_KeyNameToNum(str));
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
	lua_pushboolean(L, mousegrabbedbylua);
	return 1;
}

static int lib_setMouseGrab(lua_State *L)
{
	mousegrabbedbylua = luaL_checkboolean(L, 1);
	I_UpdateMouseGrab();
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

static int lib_getPlayerGamepad(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (!player)
		return LUA_ErrInvalid(L, "player_t");

	INT16 which = G_GetGamepadForPlayer(player);
	if (which >= 0)
		LUA_PushUserdata(L, &gamepads[which], META_GAMEPAD);
	else
		lua_pushnil(L);

	return 1;
}

static luaL_Reg lib[] = {
	{"gameControlDown", lib_gameControlDown},
	{"gameControl2Down", lib_gameControl2Down},
	{"gameControlToKeyNum", lib_gameControlToKeyNum},
	{"gameControl2ToKeyNum", lib_gameControl2ToKeyNum},
	{"joyAxis", lib_joyAxis},
	{"joy2Axis", lib_joy2Axis},
	{"keyNumToName", lib_keyNumToName},
	{"keyNameToNum", lib_keyNameToNum},
	{"keyNumPrintable", lib_keyNumPrintable},
	{"shiftKeyNum", lib_shiftKeyNum},
	{"getMouseGrab", lib_getMouseGrab},
	{"setMouseGrab", lib_setMouseGrab},
	{"getCursorPosition", lib_getCursorPosition},
	{"getPlayerGamepad", lib_getPlayerGamepad},
	{NULL, NULL}
};

///////////////////
// gamekeydown[] //
///////////////////

static int lib_getGameKeyDown(lua_State *L)
{
	int i = luaL_checkinteger(L, 2);
	if (i < 0 || i >= NUMINPUTS)
		return luaL_error(L, "Key index %d out of range (0 - %d)", i, NUMINPUTS-1);
	lua_pushboolean(L, G_CheckKeyDown(0, i, false) || G_CheckKeyDown(1, i, false));
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

///////////////
// KEY EVENT //
///////////////

static int keyevent_get(lua_State *L)
{
	event_t *event = *((event_t **)luaL_checkudata(L, 1, META_KEYEVENT));
	const char *field = luaL_checkstring(L, 2);

	I_Assert(event != NULL);

	if (fastcmp(field,"name"))
		lua_pushstring(L, G_KeyNumToName(event->key));
	else if (fastcmp(field,"num"))
		lua_pushinteger(L, event->key);
	else if (fastcmp(field,"repeated"))
		lua_pushboolean(L, event->repeated);
	else
		return luaL_error(L, "keyevent_t has no field named %s", field);

	return 1;
}

/////////////
// GAMEPAD //
/////////////

enum gamepad_leftright_e {
	gamepad_opt_left,
	gamepad_opt_right
};

static const char *const gamepad_leftright_opt[] = {
	"left",
	"right",
	NULL};

// Buttons
static int gamepad_isButtonDown(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	gamepad_button_e button = luaL_checkoption(L, 2, NULL, gamepad_button_names);
	lua_pushboolean(L, gamepad->buttons[button] == 1);
	return 1;
}

// Axes
static int gamepad_getAxis(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	gamepad_axis_e axis = luaL_checkoption(L, 2, NULL, gamepad_axis_names);
	boolean applyDeadzone = luaL_opt(L, luaL_checkboolean, 3, true);
	lua_pushfixed(L, G_GetAdjustedGamepadAxis(gamepad->num, axis, applyDeadzone));
	return 1;
}

// Sticks
static int gamepad_getStick(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	enum gamepad_leftright_e stick = luaL_checkoption(L, 2, NULL, gamepad_leftright_opt);
	boolean applyDeadzone = luaL_opt(L, luaL_checkboolean, 3, true);

	switch (stick)
	{
	case gamepad_opt_left:
		lua_pushfixed(L, G_GetAdjustedGamepadAxis(gamepad->num, GAMEPAD_AXIS_LEFTX, applyDeadzone));
		lua_pushfixed(L, G_GetAdjustedGamepadAxis(gamepad->num, GAMEPAD_AXIS_LEFTY, applyDeadzone));
		break;
	case gamepad_opt_right:
		lua_pushfixed(L, G_GetAdjustedGamepadAxis(gamepad->num, GAMEPAD_AXIS_RIGHTX, applyDeadzone));
		lua_pushfixed(L, G_GetAdjustedGamepadAxis(gamepad->num, GAMEPAD_AXIS_RIGHTY, applyDeadzone));
		break;
	}

	return 2;
}

// Triggers
static int gamepad_getTrigger(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	enum gamepad_leftright_e stick = luaL_checkoption(L, 2, NULL, gamepad_leftright_opt);
	boolean applyDeadzone = luaL_opt(L, luaL_checkboolean, 3, true);
	gamepad_axis_e axis = 0;

	switch (stick)
	{
	case gamepad_opt_left:
		axis = GAMEPAD_AXIS_TRIGGERLEFT;
		break;
	case gamepad_opt_right:
		axis = GAMEPAD_AXIS_TRIGGERRIGHT;
		break;
	}

	lua_pushfixed(L, G_GetAdjustedGamepadAxis(gamepad->num, axis, applyDeadzone));
	return 1;
}

// Button and axis names
static int gamepad_getButtonName(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	gamepad_button_e button = luaL_checkoption(L, 2, NULL, gamepad_button_names);
	lua_pushstring(L, G_GetGamepadButtonString(gamepad->type, button, GAMEPAD_STRING_DEFAULT));
	return 1;
}

static int gamepad_getAxisName(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	gamepad_axis_e axis = luaL_checkoption(L, 2, NULL, gamepad_axis_names);
	lua_pushstring(L, G_GetGamepadAxisString(gamepad->type, axis, GAMEPAD_STRING_DEFAULT, false));
	return 1;
}

static int gamepad_getTriggerName(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	enum gamepad_leftright_e stick = luaL_checkoption(L, 2, NULL, gamepad_leftright_opt);
	gamepad_axis_e axis = 0;

	switch (stick)
	{
	case gamepad_opt_left:
		axis = GAMEPAD_AXIS_TRIGGERLEFT;
		break;
	case gamepad_opt_right:
		axis = GAMEPAD_AXIS_TRIGGERRIGHT;
		break;
	}

	lua_pushstring(L, G_GetGamepadAxisString(gamepad->type, axis, GAMEPAD_STRING_DEFAULT, false));
	return 1;
}

// Rumble
static int gamepad_doRumble(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	fixed_t large_magnitude = luaL_checkfixed(L, 2);
	fixed_t small_magnitude = luaL_optfixed(L, 3, large_magnitude);
	tic_t duration = luaL_optinteger(L, 4, 0);

#define CHECK_MAGNITUDE(which) \
	if (which##_magnitude < 0 || which##_magnitude > FRACUNIT) \
		return luaL_error(L, va(#which " motor frequency %f out of range (minimum is 0.0, maximum is 1.0)", \
			FixedToFloat(which##_magnitude)))

	CHECK_MAGNITUDE(large);
	CHECK_MAGNITUDE(small);

#undef CHECK_MAGNITUDE

	lua_pushboolean(L, G_RumbleGamepad(gamepad->num, large_magnitude, small_magnitude, duration));
	return 1;
}

static int gamepad_stopRumble(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	G_StopGamepadRumble(gamepad->num);
	return 0;
}

// Accessing gamepad userdata
enum gamepad_opt_e {
	gamepad_opt_connected,
	gamepad_opt_type,
	gamepad_opt_isXbox,
	gamepad_opt_isPlayStation,
	gamepad_opt_isNintendoSwitch,
	gamepad_opt_isJoyCon,
	gamepad_opt_hasRumble,
	gamepad_opt_isRumbling,
	gamepad_opt_isRumblePaused,
	gamepad_opt_largeMotorFrequency,
	gamepad_opt_smallMotorFrequency,
	gamepad_opt_isButtonDown,
	gamepad_opt_getAxis,
	gamepad_opt_getStick,
	gamepad_opt_getTrigger,
	gamepad_opt_getButtonName,
	gamepad_opt_getAxisName,
	gamepad_opt_getTriggerName,
	gamepad_opt_rumble,
	gamepad_opt_stopRumble
};

static const char *const gamepad_opt[] = {
	"connected",
	"type",
	"isXbox",
	"isPlayStation",
	"isNintendoSwitch",
	"isJoyCon",
	"hasRumble",
	"isRumbling",
	"isRumblePaused",
	"largeMotorFrequency",
	"smallMotorFrequency",
	"isButtonDown",
	"getAxis",
	"getStick",
	"getTrigger",
	"getButtonName",
	"getAxisName",
	"getTriggerName",
	"rumble",
	"stopRumble",
	NULL};

static int (*gamepad_fn_list[9])(lua_State *L) = {
	gamepad_isButtonDown,
	gamepad_getAxis,
	gamepad_getStick,
	gamepad_getTrigger,
	gamepad_getButtonName,
	gamepad_getAxisName,
	gamepad_getTriggerName,
	gamepad_doRumble,
	gamepad_stopRumble
};

static int gamepad_get(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	enum gamepad_opt_e field = luaL_checkoption(L, 2, NULL, gamepad_opt);

	switch (field)
	{
	case gamepad_opt_connected:
		lua_pushboolean(L, gamepad->connected);
		break;
	case gamepad_opt_type:
		lua_pushstring(L, G_GamepadTypeToString(gamepad->type));
		break;
	case gamepad_opt_isXbox:
		lua_pushboolean(L, G_GamepadTypeIsXbox(gamepad->type));
		break;
	case gamepad_opt_isPlayStation:
		lua_pushboolean(L, G_GamepadTypeIsPlayStation(gamepad->type));
		break;
	case gamepad_opt_isNintendoSwitch:
		lua_pushboolean(L, G_GamepadTypeIsNintendoSwitch(gamepad->type));
		break;
	case gamepad_opt_isJoyCon:
		// No, this does not include the grip.
		lua_pushboolean(L, G_GamepadTypeIsJoyCon(gamepad->type));
		break;
	case gamepad_opt_hasRumble:
		lua_pushboolean(L, G_RumbleSupported(gamepad->num));
		break;
	case gamepad_opt_isRumbling:
		lua_pushboolean(L, gamepad->rumble.active);
		break;
	case gamepad_opt_isRumblePaused:
		lua_pushboolean(L, G_GetGamepadRumblePaused(gamepad->num));
		break;
	case gamepad_opt_largeMotorFrequency:
		lua_pushfixed(L, G_GetLargeMotorFreq(gamepad->num));
		break;
	case gamepad_opt_smallMotorFrequency:
		lua_pushfixed(L, G_GetSmallMotorFreq(gamepad->num));
		break;
	case gamepad_opt_isButtonDown:
	case gamepad_opt_getAxis:
	case gamepad_opt_getStick:
	case gamepad_opt_getTrigger:
	case gamepad_opt_getButtonName:
	case gamepad_opt_getAxisName:
	case gamepad_opt_getTriggerName:
	case gamepad_opt_rumble:
	case gamepad_opt_stopRumble:
		lua_pushcfunction(L, gamepad_fn_list[field - gamepad_opt_isButtonDown]);
		break;
	}
	return 1;
}

static int gamepad_set(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	enum gamepad_opt_e field = luaL_checkoption(L, 2, NULL, gamepad_opt);

	switch (field)
	{
	case gamepad_opt_isRumblePaused:
		G_SetGamepadRumblePaused(gamepad->num, luaL_checkboolean(L, 3));
		break;
	case gamepad_opt_largeMotorFrequency:
		G_SetLargeMotorFreq(gamepad->num, luaL_checkfixed(L, 3));
		break;
	case gamepad_opt_smallMotorFrequency:
		G_SetSmallMotorFreq(gamepad->num, luaL_checkfixed(L, 3));
		break;
	default:
		return luaL_error(L, LUA_QL("gamepad") " field " LUA_QS " should not be set directly.", gamepad_opt[field]);
	}
	return 1;
}

static int gamepad_num(lua_State *L)
{
	gamepad_t *gamepad = *((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD));
	lua_pushinteger(L, gamepad->num + 1);
	return 1;
}

static int lib_iterateGamepads(lua_State *L)
{
	INT32 i = -1;
	if (lua_gettop(L) < 2)
	{
		lua_pushcfunction(L, lib_iterateGamepads);
		return 1;
	}
	lua_settop(L, 2);
	lua_remove(L, 1); // State is unused
	if (!lua_isnil(L, 1))
		i = (INT32)(*((gamepad_t **)luaL_checkudata(L, 1, META_GAMEPAD)) - gamepads);
	for (i++; i < NUM_GAMEPADS; i++)
	{
		if (!gamepads[i].connected)
			continue;
		LUA_PushUserdata(L, &gamepads[i], META_GAMEPAD);
		return 1;
	}
	return 0;
}

static int lib_getGamepad(lua_State *L)
{
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		lua_Integer i = luaL_checkinteger(L, 2);
		if (i < 1 || i > NUM_GAMEPADS)
			return luaL_error(L, "gamepads[] index %d out of range (1 - %d)", i, NUM_GAMEPADS);
		LUA_PushUserdata(L, &gamepads[i - 1], META_GAMEPAD);
		return 1;
	}

	if (fastcmp(luaL_checkstring(L, 2), "iterate"))
	{
		lua_pushcfunction(L, lib_iterateGamepads);
		return 1;
	}

	return 0;
}

static int lib_lenGamepad(lua_State *L)
{
	lua_pushinteger(L, NUM_GAMEPADS);
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

	luaL_newmetatable(L, META_KEYEVENT);
		lua_pushcfunction(L, keyevent_get);
		lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_GAMEPAD);
		lua_pushcfunction(L, gamepad_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, gamepad_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, gamepad_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getGamepad);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_lenGamepad);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "gamepads");

	luaL_newmetatable(L, META_MOUSE);
		lua_pushcfunction(L, mouse_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, mouse_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	luaL_register(L, "input", lib);
	return 0;
}
