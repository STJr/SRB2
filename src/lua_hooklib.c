// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hooklib.c
/// \brief hooks for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "doomstat.h"
#include "p_mobj.h"
#include "r_things.h"
#include "b_bot.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hook.h"
#include "lua_hud.h" // hud_running errors

const char *const hookNames[hook_MAX+1] = {
	"NetVars",
	"MapChange",
	"MapLoad",
	"PlayerJoin",
	"ThinkFrame",
	"MobjSpawn",
	"MobjCollide",
	"MobjMoveCollide",
	"TouchSpecial",
	"MobjFuse",
	"MobjThinker",
	"BossThinker",
	"ShouldDamage",
	"MobjDamage",
	"MobjDeath",
	"BossDeath",
	"MobjRemoved",
	"BotTiccmd",
	"BotAI",
	"LinedefExecute",
	NULL
};

// Takes hook, function, and additional arguments (mobj type to act on, etc.)
static int lib_addHook(lua_State *L)
{
	UINT16 hook;
	boolean notable = false;
	boolean subtable = false;
	UINT32 subindex = 0;
	char *subfield = NULL;
	const char *lsubfield = NULL;

	hook = (UINT16)luaL_checkoption(L, 1, NULL, hookNames);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	if (hud_running)
		return luaL_error(L, "HUD rendering code should not call this function!");

	switch(hook)
	{
	// Take a mobjtype enum which this hook is specifically for.
	case hook_MobjSpawn:
	case hook_MobjCollide:
	case hook_MobjMoveCollide:
	case hook_TouchSpecial:
	case hook_MobjFuse:
	case hook_MobjThinker:
	case hook_BossThinker:
	case hook_ShouldDamage:
	case hook_MobjDamage:
	case hook_MobjDeath:
	case hook_BossDeath:
	case hook_MobjRemoved:
		subtable = true;
		if (lua_isnumber(L, 3))
			subindex = (UINT32)luaL_checkinteger(L, 3);
		else
			lsubfield = "a";
		lua_settop(L, 2);
		break;
	case hook_BotAI: // Only one AI function per skin, please!
		notable = true;
		subtable = true;
		subfield = ZZ_Alloc(strlen(luaL_checkstring(L, 3))+1);
		{ // lowercase copy
			char *p = subfield;
			const char *s = luaL_checkstring(L, 3);
			do {
				*p = tolower(*s);
				++p;
			} while(*(++s));
			*p = 0;
		}
		lua_settop(L, 3);
		break;
	case hook_LinedefExecute: // Get one linedef executor function by name
		notable = true;
		subtable = true;
		subfield = ZZ_Alloc(strlen(luaL_checkstring(L, 3))+1);
		{ // uppercase copy
			char *p = subfield;
			const char *s = luaL_checkstring(L, 3);
			do {
				*p = toupper(*s);
				++p;
			} while(*(++s));
			*p = 0;
		}
		lua_settop(L, 3);
		break;
	default:
		lua_settop(L, 2);
		break;
	}

	lua_getfield(L, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(L, -1));

	// This hook type only allows one entry, not an array of hooks.
	// New hooks will overwrite the previous ones, and the stack is one table shorter.
	if (notable)
	{
		if (subtable)
		{
			lua_rawgeti(L, -1, hook);
			lua_remove(L, -2); // pop "hook"
			I_Assert(lua_istable(L, -1));
			lua_pushvalue(L, 2);
			if (subfield)
				lua_setfield(L, -2, subfield);
			else if (lsubfield)
				lua_setfield(L, -2, lsubfield);
			else
				lua_rawseti(L, -2, subindex);
		} else {
			lua_pushvalue(L, 2);
			lua_rawseti(L, -2, hook);
		}
		return 0;
	}

	// Fetch the hook's table from the registry.
	// It should always exist, since LUA_HookLib creates a table for every hook.
	lua_rawgeti(L, -1, hook);
	lua_remove(L, -2); // pop "hook"
	I_Assert(lua_istable(L, -1));
	if (subtable)
	{
		// Fetch a subtable based on index
		if (subfield)
			lua_getfield(L, -1, subfield);
		else if (lsubfield)
			lua_getfield(L, -1, lsubfield);
		else
			lua_rawgeti(L, -1, subindex);

		// Subtable doesn't exist, make one now.
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_newtable(L);

			// Store a link to the subtable for later.
			lua_pushvalue(L, -1);
			if (subfield)
				lua_setfield(L, -3, subfield);
			else if (lsubfield)
				lua_setfield(L, -3, lsubfield);
			else
				lua_rawseti(L, -3, subindex);
	}	}

	// Add function to the table.
	lua_pushvalue(L, 2);
	lua_rawseti(L, -2, (int)(lua_objlen(L, -2) + 1));

	if (subfield)
		Z_Free(subfield);
	return 0;
}

int LUA_HookLib(lua_State *L)
{
	// Create all registry tables
	enum hook i;
	lua_newtable(L);
	for (i = 0; i < hook_MAX; i++)
	{
		lua_newtable(L);
		switch(i)
		{
		default:
			break;
		case hook_MobjSpawn:
		case hook_MobjCollide:
		case hook_MobjMoveCollide:
		case hook_TouchSpecial:
		case hook_MobjFuse:
		case hook_MobjThinker:
		case hook_BossThinker:
		case hook_ShouldDamage:
		case hook_MobjDamage:
		case hook_MobjDeath:
		case hook_BossDeath:
		case hook_MobjRemoved:
			lua_pushstring(L, "a");
			lua_newtable(L);
			lua_rawset(L, -3);
			break;
		}
		lua_rawseti(L, -2, i);
	}
	lua_setfield(L, LUA_REGISTRYINDEX, "hook");
	lua_register(L, "addHook", lib_addHook);
	return 0;
}

boolean LUAh_MobjHook(mobj_t *mo, enum hook which)
{
	boolean hooked = false;
	if (!gL)
		return false;

	// clear the stack (just in case)
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, which);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// generic subtable
	lua_pushstring(gL, "a");
	lua_rawget(gL, -2);
	I_Assert(lua_istable(gL, -1));

	LUA_PushUserdata(gL, mo, META_MOBJ);
	lua_pushnil(gL);
	while (lua_next(gL, -3)) {
		CONS_Debug(DBG_LUA, "MobjHook: Calling hook_%s for generic mobj types\n", hookNames[which]);
		lua_pushvalue(gL, -3); // mo
		// stack is: hook_Mobj table, subtable "a", mobj, i, function, mobj
		if (lua_pcall(gL, 1, 1, 0)) {
			// A run-time error occurred.
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			// Remove this function from the hook table to prevent further errors.
			lua_pushvalue(gL, -1); // key
			lua_pushnil(gL); // value
			lua_rawset(gL, -5); // table
			CONS_Printf("Hook removed.\n");
		}
		else
		{
			if (lua_toboolean(gL, -1))
				hooked = true;
			lua_pop(gL, 1);
		}
	}
	// stack is: hook_Mobj table, subtable "a", mobj
	lua_remove(gL, -2); // pop subtable, leave mobj

	// mobjtype subtable
	// stack is: hook_Mobj table, mobj
	lua_rawgeti(gL, -2, mo->type);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 3); // pop hook_Mobj table, mobj, and nil
		// the stack should now be empty.
		return false;
	}
	lua_remove(gL, -3); // remove hook table
	// stack is: mobj, mobjtype subtable
	lua_insert(gL, lua_gettop(gL)-1); // swap subtable with mobj
	// stack is: mobjtype subtable, mobj

	lua_pushnil(gL);
	while (lua_next(gL, -3)) {
		CONS_Debug(DBG_LUA, "MobjHook: Calling hook_%s for mobj type %d\n", hookNames[which], mo->type);
		lua_pushvalue(gL, -3); // mo
		// stack is: mobjtype subtable, mobj, i, function, mobj
		if (lua_pcall(gL, 1, 1, 0)) {
			// A run-time error occurred.
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			// Remove this function from the hook table to prevent further errors.
			lua_pushvalue(gL, -1); // key
			lua_pushnil(gL); // value
			lua_rawset(gL, -5); // table
			CONS_Printf("Hook removed.\n");
		}
		else
		{
			if (lua_toboolean(gL, -1))
				hooked = true;
			lua_pop(gL, 1);
		}
	}

	lua_pop(gL, 2); // pop mobj and subtable
	// the stack should now be empty.

	lua_gc(gL, LUA_GCSTEP, 3);
	return hooked;
}

// Hook for map change (before load)
void LUAh_MapChange(void)
{
	if (!gL)
		return;

	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_MapChange);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	lua_pushinteger(gL, gamemap);
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // gamemap
		LUA_Call(gL, 1);
	}
	lua_pop(gL, 1);
	lua_gc(gL, LUA_GCSTEP, 1);
}

// Hook for map load
void LUAh_MapLoad(void)
{
	if (!gL)
		return;

	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_MapLoad);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	lua_pushinteger(gL, gamemap);
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // gamemap
		LUA_Call(gL, 1);
	}
	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCCOLLECT, 0);
}

// Hook for Got_AddPlayer
void LUAh_PlayerJoin(int playernum)
{
	if (!gL)
		return;

	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_PlayerJoin);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	lua_pushinteger(gL, playernum);
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // playernum
		LUA_Call(gL, 1);
	}
	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCCOLLECT, 0);
}

// Hook for frame (after mobj and player thinkers)
void LUAh_ThinkFrame(void)
{
	if (!gL)
		return;

	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_ThinkFrame);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	lua_pushnil(gL);
	while (lua_next(gL, -2) != 0)
	{
		//LUA_Call(gL, 0);
		if (lua_pcall(gL, 0, 0, 0))
		{
			// A run-time error occurred.
			CONS_Alert(CONS_WARNING,"%s\n", lua_tostring(gL, -1));
			lua_pop(gL, 1);
			// Remove this function from the hook table to prevent further errors.
			lua_pushvalue(gL, -1); // key
			lua_pushnil(gL); // value
			lua_rawset(gL, -4); // table
			CONS_Printf("Hook removed.\n");
		}
	}
	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCCOLLECT, 0);
}

// Hook for PIT_CheckThing by (thing) mobj type (thing1 = thing, thing2 = tmthing)
UINT8 LUAh_MobjCollide(mobj_t *thing1, mobj_t *thing2)
{
	UINT8 shouldCollide = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL)
		return 0;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_MobjCollide);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// mobjtype subtable
	lua_rawgeti(gL, -1, thing1->type);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return 0;
	}
	lua_remove(gL, -2); // remove hook table

	LUA_PushUserdata(gL, thing1, META_MOBJ);
	LUA_PushUserdata(gL, thing2, META_MOBJ);
	lua_pushnil(gL);
	while (lua_next(gL, -4)) {
		lua_pushvalue(gL, -4); // thing1
		lua_pushvalue(gL, -4); // thing2
		if (lua_pcall(gL, 2, 1, 0)) {
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1); // pop return value
	}
	lua_pop(gL, 3); // pop arguments and mobjtype table

	lua_gc(gL, LUA_GCSTEP, 1);
	return shouldCollide;
}

// Hook for PIT_CheckThing by (tmthing) mobj type (thing1 = tmthing, thing2 = thing)
UINT8 LUAh_MobjMoveCollide(mobj_t *thing1, mobj_t *thing2)
{
	UINT8 shouldCollide = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL)
		return 0;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_MobjMoveCollide);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// mobjtype subtable
	lua_rawgeti(gL, -1, thing1->type);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return 0;
	}
	lua_remove(gL, -2); // remove hook table

	LUA_PushUserdata(gL, thing1, META_MOBJ);
	LUA_PushUserdata(gL, thing2, META_MOBJ);
	lua_pushnil(gL);
	while (lua_next(gL, -4)) {
		lua_pushvalue(gL, -4); // thing1
		lua_pushvalue(gL, -4); // thing2
		if (lua_pcall(gL, 2, 1, 0)) {
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1); // pop return value
	}
	lua_pop(gL, 3); // pop arguments and mobjtype table

	lua_gc(gL, LUA_GCSTEP, 1);
	return shouldCollide;
}

// Hook for P_TouchSpecialThing by mobj type
boolean LUAh_TouchSpecial(mobj_t *special, mobj_t *toucher)
{
	boolean hooked = false;
	if (!gL)
		return false;

	// clear the stack
	lua_pop(gL, -1);

	// get hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_TouchSpecial);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// get mobjtype subtable
	lua_pushinteger(gL, special->type);
	lua_rawget(gL, 1);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return false;
	}
	lua_remove(gL, 1); // pop hook table off the stack

	LUA_PushUserdata(gL, special, META_MOBJ);
	LUA_PushUserdata(gL, toucher, META_MOBJ);

	lua_pushnil(gL);
	while (lua_next(gL, 1) != 0) {
		lua_pushvalue(gL, 2); // special
		lua_pushvalue(gL, 3); // toucher
		LUA_Call(gL, 2); // pops hook function, special, toucher
		hooked = true;
	}

	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCSTEP, 1);
	return hooked;
}

// Hook for P_DamageMobj by mobj type (Should mobj take damage?)
UINT8 LUAh_ShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage)
{
	UINT8 shouldDamage = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL)
		return 0;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_ShouldDamage);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// mobjtype subtable
	lua_rawgeti(gL, -1, target->type);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return 0;
	}
	lua_remove(gL, -2); // remove hook table

	LUA_PushUserdata(gL, target, META_MOBJ);
	LUA_PushUserdata(gL, inflictor, META_MOBJ);
	LUA_PushUserdata(gL, source, META_MOBJ);
	lua_pushinteger(gL, damage);
	lua_pushnil(gL);
	while (lua_next(gL, -6)) {
		lua_pushvalue(gL, -6); // target
		lua_pushvalue(gL, -6); // inflictor
		lua_pushvalue(gL, -6); // source
		lua_pushvalue(gL, -6); // damage
		if (lua_pcall(gL, 4, 1, 0)) {
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldDamage = 0.
			if (lua_toboolean(gL, -1))
				shouldDamage = 1; // Force yes
			else
				shouldDamage = 2; // Force no
		}
		lua_pop(gL, 1); // pop return value
	}
	lua_pop(gL, 5); // pop arguments and mobjtype table

	lua_gc(gL, LUA_GCSTEP, 1);
	return shouldDamage;
}

// Hook for P_DamageMobj by mobj type (Mobj actually takes damage!)
boolean LUAh_MobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage)
{
	boolean handled = false;
	if (!gL)
		return false;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_MobjDamage);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// mobjtype subtable
	lua_rawgeti(gL, -1, target->type);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return false;
	}
	lua_remove(gL, -2); // remove hook table

	LUA_PushUserdata(gL, target, META_MOBJ);
	LUA_PushUserdata(gL, inflictor, META_MOBJ);
	LUA_PushUserdata(gL, source, META_MOBJ);
	lua_pushinteger(gL, damage);
	lua_pushnil(gL);
	while (lua_next(gL, -6)) {
		lua_pushvalue(gL, -6); // target
		lua_pushvalue(gL, -6); // inflictor
		lua_pushvalue(gL, -6); // source
		lua_pushvalue(gL, -6); // damage
		if (lua_pcall(gL, 4, 1, 0)) {
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			continue;
		}
		if (lua_toboolean(gL, -1))
			handled = true;
		lua_pop(gL, 1); // pop return value
	}
	lua_pop(gL, 5); // pop arguments and mobjtype table

	lua_gc(gL, LUA_GCSTEP, 1);
	return handled;
}

// Hook for P_KillMobj by mobj type
boolean LUAh_MobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source)
{
	boolean handled = false;
	if (!gL)
		return false;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_MobjDeath);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// mobjtype subtable
	lua_rawgeti(gL, -1, target->type);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return false;
	}
	lua_remove(gL, -2); // remove hook table

	LUA_PushUserdata(gL, target, META_MOBJ);
	LUA_PushUserdata(gL, inflictor, META_MOBJ);
	LUA_PushUserdata(gL, source, META_MOBJ);
	lua_pushnil(gL);
	while (lua_next(gL, -5)) {
		lua_pushvalue(gL, -5); // target
		lua_pushvalue(gL, -5); // inflictor
		lua_pushvalue(gL, -5); // source
		if (lua_pcall(gL, 3, 1, 0)) {
			CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
			lua_pop(gL, 1);
			continue;
		}
		if (lua_toboolean(gL, -1))
			handled = true;
		lua_pop(gL, 1); // pop return value
	}
	lua_pop(gL, 4); // pop arguments and mobjtype table

	lua_gc(gL, LUA_GCSTEP, 1);
	return handled;
}

// Hook for B_BuildTiccmd
boolean LUAh_BotTiccmd(player_t *bot, ticcmd_t *cmd)
{
	boolean hooked = false;
	if (!gL)
		return false;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_BotTiccmd);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	LUA_PushUserdata(gL, bot, META_PLAYER);
	LUA_PushUserdata(gL, cmd, META_TICCMD);

	lua_pushnil(gL);
	while (lua_next(gL, 1)) {
		lua_pushvalue(gL, 2); // bot
		lua_pushvalue(gL, 3); // cmd
		LUA_Call(gL, 2);
		hooked = true;
	}

	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCSTEP, 1);
	return hooked;
}

// Hook for B_BuildTailsTiccmd by skin name
boolean LUAh_BotAI(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd)
{
	if (!gL || !tails->skin)
		return false;

	// clear the stack
	lua_pop(gL, -1);

	// hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_BotAI);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// bot skin ai function
	lua_getfield(gL, 1, ((skin_t *)tails->skin)->name);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return false;
	}
	lua_remove(gL, 1); // pop the hook table

	// Takes sonic, tails
	// Returns forward, backward, left, right, jump, spin
	LUA_PushUserdata(gL, sonic, META_MOBJ);
	LUA_PushUserdata(gL, tails, META_MOBJ);
	if (lua_pcall(gL, 2, 8, 0)) {
		CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL,-1));
		lua_pop(gL,-1);
		return false;
	}

	// This turns forward, backward, left, right, jump, and spin into a proper ticcmd for tails.
	if (lua_istable(gL, 1)) {
		boolean forward=false, backward=false, left=false, right=false, strafeleft=false, straferight=false, jump=false, spin=false;

#define CHECKFIELD(field) \
		lua_getfield(gL, 1, #field);\
		if (lua_toboolean(gL, -1))\
			field = true;\
		lua_pop(gL, 1);

		CHECKFIELD(forward)
		CHECKFIELD(backward)
		CHECKFIELD(left)
		CHECKFIELD(right)
		CHECKFIELD(strafeleft)
		CHECKFIELD(straferight)
		CHECKFIELD(jump)
		CHECKFIELD(spin)

#undef CHECKFIELD

		B_KeysToTiccmd(tails, cmd, forward, backward, left, right, strafeleft, straferight, jump, spin);
	} else
		B_KeysToTiccmd(tails, cmd, lua_toboolean(gL, 1), lua_toboolean(gL, 2), lua_toboolean(gL, 3), lua_toboolean(gL, 4), lua_toboolean(gL, 5), lua_toboolean(gL, 6), lua_toboolean(gL, 7), lua_toboolean(gL, 8));

	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCSTEP, 1);
	return true;
}

// Hook for linedef executors
boolean LUAh_LinedefExecute(line_t *line, mobj_t *mo)
{
	if (!gL)
		return false;

	// clear the stack
	lua_pop(gL, -1);

	// get hook table
	lua_getfield(gL, LUA_REGISTRYINDEX, "hook");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, hook_LinedefExecute);
	lua_remove(gL, -2);
	I_Assert(lua_istable(gL, -1));

	// get function by line text
	lua_getfield(gL, 1, line->text);
	if (lua_isnil(gL, -1)) {
		lua_pop(gL, 2);
		return false;
	}
	lua_remove(gL, 1); // pop hook table off the stack

	LUA_PushUserdata(gL, line, META_LINE);
	LUA_PushUserdata(gL, mo, META_MOBJ);
	LUA_Call(gL, 2); // pops hook function, line, mo

	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCSTEP, 1);
	return true;
}

#endif
