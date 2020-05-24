// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hooklib.c
/// \brief hooks for Lua scripting

#include "doomdef.h"
#include "doomstat.h"
#include "p_mobj.h"
#include "g_game.h"
#include "r_skins.h"
#include "b_bot.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hook.h"
#include "lua_hud.h" // hud_running errors

static UINT8 hooksAvailable[(hook_MAX/8)+1];

const char *const hookNames[hook_MAX+1] = {
	"NetVars",
	"MapChange",
	"MapLoad",
	"PlayerJoin",
	"PreThinkFrame",
	"ThinkFrame",
	"PostThinkFrame",
	"MobjSpawn",
	"MobjCollide",
	"MobjLineCollide",
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
	"JumpSpecial",
	"AbilitySpecial",
	"SpinSpecial",
	"JumpSpinSpecial",
	"BotTiccmd",
	"BotAI",
	"BotRespawn",
	"LinedefExecute",
	"PlayerMsg",
	"HurtMsg",
	"PlayerSpawn",
	"ShieldSpawn",
	"ShieldSpecial",
	"MobjMoveBlocked",
	"MapThingSpawn",
	"FollowMobj",
	"PlayerCanDamage",
	"PlayerQuit",
	"IntermissionThinker",
	"TeamSwitch",
	"ViewpointSwitch",
	"SeenPlayer",
	"PlayerThink",
	"ShouldJingleContinue",
	NULL
};

// Hook metadata
struct hook_s
{
	struct hook_s *next;
	enum hook type;
	UINT16 id;
	union {
		mobjtype_t mt;
		char *str;
	} s;
	boolean error;
};
typedef struct hook_s* hook_p;

#define FMT_HOOKID "hook_%d"

// For each mobj type, a linked list to its thinker and collision hooks.
// That way, we don't have to iterate through all the hooks.
// We could do that with all other mobj hooks, but it would probably just be
// a waste of memory since they are only called occasionally. Probably...
static hook_p mobjthinkerhooks[NUMMOBJTYPES];
static hook_p mobjcollidehooks[NUMMOBJTYPES];

// For each mobj type, a linked list for other mobj hooks
static hook_p mobjhooks[NUMMOBJTYPES];

// A linked list for player hooks
static hook_p playerhooks;

// A linked list for linedef executor hooks
static hook_p linedefexecutorhooks;

// For other hooks, a unique linked list
hook_p roothook;

// Takes hook, function, and additional arguments (mobj type to act on, etc.)
static int lib_addHook(lua_State *L)
{
	static struct hook_s hook = {NULL, 0, 0, {0}, false};
	static UINT32 nextid;
	hook_p hookp, *lastp;

	hook.type = luaL_checkoption(L, 1, NULL, hookNames);
	lua_remove(L, 1);

	luaL_checktype(L, 1, LUA_TFUNCTION);

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	switch(hook.type)
	{
	// Take a mobjtype enum which this hook is specifically for.
	case hook_MobjSpawn:
	case hook_MobjCollide:
	case hook_MobjLineCollide:
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
	case hook_HurtMsg:
	case hook_MobjMoveBlocked:
	case hook_MapThingSpawn:
	case hook_FollowMobj:
		hook.s.mt = MT_NULL;
		if (lua_isnumber(L, 2))
			hook.s.mt = lua_tonumber(L, 2);
		luaL_argcheck(L, hook.s.mt < NUMMOBJTYPES, 2, "invalid mobjtype_t");
		break;
	case hook_BotAI:
	case hook_ShouldJingleContinue:
		hook.s.str = NULL;
		if (lua_isstring(L, 2))
		{ // lowercase copy
			hook.s.str = Z_StrDup(lua_tostring(L, 2));
			strlwr(hook.s.str);
		}
		break;
	case hook_LinedefExecute: // Linedef executor functions
		hook.s.str = Z_StrDup(luaL_checkstring(L, 2));
		strupr(hook.s.str);
		break;
	default:
		break;
	}
	lua_settop(L, 1); // lua stack contains only the function now.

	hooksAvailable[hook.type/8] |= 1<<(hook.type%8);

	// set hook.id to the highest id + 1
	hook.id = nextid++;

	// Special cases for some hook types (see the comments above mobjthinkerhooks declaration)
	switch(hook.type)
	{
	case hook_MobjThinker:
		lastp = &mobjthinkerhooks[hook.s.mt];
		break;
	case hook_MobjCollide:
	case hook_MobjLineCollide:
	case hook_MobjMoveCollide:
		lastp = &mobjcollidehooks[hook.s.mt];
		break;
	case hook_MobjSpawn:
	case hook_TouchSpecial:
	case hook_MobjFuse:
	case hook_BossThinker:
	case hook_ShouldDamage:
	case hook_MobjDamage:
	case hook_MobjDeath:
	case hook_BossDeath:
	case hook_MobjRemoved:
	case hook_MobjMoveBlocked:
	case hook_MapThingSpawn:
	case hook_FollowMobj:
		lastp = &mobjhooks[hook.s.mt];
		break;
	case hook_JumpSpecial:
	case hook_AbilitySpecial:
	case hook_SpinSpecial:
	case hook_JumpSpinSpecial:
	case hook_PlayerSpawn:
	case hook_PlayerCanDamage:
	case hook_TeamSwitch:
	case hook_ViewpointSwitch:
	case hook_SeenPlayer:
	case hook_ShieldSpawn:
	case hook_ShieldSpecial:
	case hook_PlayerThink:
		lastp = &playerhooks;
		break;
	case hook_LinedefExecute:
		lastp = &linedefexecutorhooks;
		break;
	default:
		lastp = &roothook;
		break;
	}

	// iterate the hook metadata structs
	// set lastp to the last hook struct's "next" pointer.
	for (hookp = *lastp; hookp; hookp = hookp->next)
		lastp = &hookp->next;
	// allocate a permanent memory struct to stuff hook.
	hookp = ZZ_Alloc(sizeof(struct hook_s));
	memcpy(hookp, &hook, sizeof(struct hook_s));
	// tack it onto the end of the linked list.
	*lastp = hookp;

	// set the hook function in the registry.
	lua_pushfstring(L, FMT_HOOKID, hook.id);
	lua_pushvalue(L, 1);
	lua_settable(L, LUA_REGISTRYINDEX);
	return 0;
}

int LUA_HookLib(lua_State *L)
{
	memset(hooksAvailable,0,sizeof(UINT8[(hook_MAX/8)+1]));
	roothook = NULL;
	lua_register(L, "addHook", lib_addHook);
	return 0;
}

boolean LUAh_MobjHook(mobj_t *mo, enum hook which)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[which/8] & (1<<(which%8))))
		return false;

	I_Assert(mo->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic mobj hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
			LUA_PushUserdata(gL, mo, META_MOBJ);
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		if (lua_pcall(gL, 1, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[mo->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
			LUA_PushUserdata(gL, mo, META_MOBJ);
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		if (lua_pcall(gL, 1, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

boolean LUAh_PlayerHook(player_t *plr, enum hook which)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[which/8] & (1<<(which%8))))
		return false;

	lua_settop(gL, 0);

	for (hookp = playerhooks; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
			LUA_PushUserdata(gL, plr, META_PLAYER);
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		if (lua_pcall(gL, 1, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for map change (before load)
void LUAh_MapChange(INT16 mapnumber)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_MapChange/8] & (1<<(hook_MapChange%8))))
		return;

	lua_settop(gL, 0);
	lua_pushinteger(gL, mapnumber);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MapChange)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		LUA_Call(gL, 1);
	}

	lua_settop(gL, 0);
}

// Hook for map load
void LUAh_MapLoad(void)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_MapLoad/8] & (1<<(hook_MapLoad%8))))
		return;

	lua_settop(gL, 0);
	lua_pushinteger(gL, gamemap);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MapLoad)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		LUA_Call(gL, 1);
	}

	lua_settop(gL, 0);
}

// Hook for Got_AddPlayer
void LUAh_PlayerJoin(int playernum)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_PlayerJoin/8] & (1<<(hook_PlayerJoin%8))))
		return;

	lua_settop(gL, 0);
	lua_pushinteger(gL, playernum);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_PlayerJoin)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		LUA_Call(gL, 1);
	}

	lua_settop(gL, 0);
}

// Hook for frame (before mobj and player thinkers)
void LUAh_PreThinkFrame(void)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_PreThinkFrame/8] & (1<<(hook_PreThinkFrame%8))))
		return;

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_PreThinkFrame)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		if (lua_pcall(gL, 0, 0, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
		}
	}
}

// Hook for frame (after mobj and player thinkers)
void LUAh_ThinkFrame(void)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_ThinkFrame/8] & (1<<(hook_ThinkFrame%8))))
		return;

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_ThinkFrame)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		if (lua_pcall(gL, 0, 0, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
		}
	}
}


// Hook for frame (at end of tick, ie after overlays, precipitation, specials)
void LUAh_PostThinkFrame(void)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_PostThinkFrame/8] & (1<<(hook_PostThinkFrame%8))))
		return;

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_PostThinkFrame)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		if (lua_pcall(gL, 0, 0, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
		}
	}
}

// Hook for mobj collisions
UINT8 LUAh_MobjCollideHook(mobj_t *thing1, mobj_t *thing2, enum hook which)
{
	hook_p hookp;
	UINT8 shouldCollide = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL || !(hooksAvailable[which/8] & (1<<(which%8))))
		return 0;

	I_Assert(thing1->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic mobj collision hooks
	for (hookp = mobjcollidehooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, thing1, META_MOBJ);
			LUA_PushUserdata(gL, thing2, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	for (hookp = mobjcollidehooks[thing1->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, thing1, META_MOBJ);
			LUA_PushUserdata(gL, thing2, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return shouldCollide;
}

UINT8 LUAh_MobjLineCollideHook(mobj_t *thing, line_t *line, enum hook which)
{
	hook_p hookp;
	UINT8 shouldCollide = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL || !(hooksAvailable[which/8] & (1<<(which%8))))
		return 0;

	I_Assert(thing->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic mobj collision hooks
	for (hookp = mobjcollidehooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, thing, META_MOBJ);
			LUA_PushUserdata(gL, line, META_LINE);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	for (hookp = mobjcollidehooks[thing->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != which)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, thing, META_MOBJ);
			LUA_PushUserdata(gL, line, META_LINE);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return shouldCollide;
}

// Hook for mobj thinkers
boolean LUAh_MobjThinker(mobj_t *mo)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_MobjThinker/8] & (1<<(hook_MobjThinker%8))))
		return false;

	I_Assert(mo->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic mobj thinker hooks
	for (hookp = mobjthinkerhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (lua_gettop(gL) == 0)
			LUA_PushUserdata(gL, mo, META_MOBJ);
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		if (lua_pcall(gL, 1, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjthinkerhooks[mo->type]; hookp; hookp = hookp->next)
	{
		if (lua_gettop(gL) == 0)
			LUA_PushUserdata(gL, mo, META_MOBJ);
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2);
		if (lua_pcall(gL, 1, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for P_TouchSpecialThing by mobj type
boolean LUAh_TouchSpecial(mobj_t *special, mobj_t *toucher)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_TouchSpecial/8] & (1<<(hook_TouchSpecial%8))))
		return 0;

	I_Assert(special->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic touch special hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_TouchSpecial)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, special, META_MOBJ);
			LUA_PushUserdata(gL, toucher, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[special->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_TouchSpecial)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, special, META_MOBJ);
			LUA_PushUserdata(gL, toucher, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for P_DamageMobj by mobj type (Should mobj take damage?)
UINT8 LUAh_ShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	hook_p hookp;
	UINT8 shouldDamage = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL || !(hooksAvailable[hook_ShouldDamage/8] & (1<<(hook_ShouldDamage%8))))
		return 0;

	I_Assert(target->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic should damage hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_ShouldDamage)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, target, META_MOBJ);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damage);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		if (lua_pcall(gL, 5, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{
			if (lua_toboolean(gL, -1))
				shouldDamage = 1; // Force yes
			else
				shouldDamage = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[target->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_ShouldDamage)
			continue;
		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, target, META_MOBJ);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damage);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		if (lua_pcall(gL, 5, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{
			if (lua_toboolean(gL, -1))
				shouldDamage = 1; // Force yes
			else
				shouldDamage = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return shouldDamage;
}

// Hook for P_DamageMobj by mobj type (Mobj actually takes damage!)
boolean LUAh_MobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_MobjDamage/8] & (1<<(hook_MobjDamage%8))))
		return 0;

	I_Assert(target->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic mobj damage hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MobjDamage)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, target, META_MOBJ);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damage);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		if (lua_pcall(gL, 5, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[target->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MobjDamage)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, target, META_MOBJ);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damage);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		if (lua_pcall(gL, 5, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for P_KillMobj by mobj type
boolean LUAh_MobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_MobjDeath/8] & (1<<(hook_MobjDeath%8))))
		return 0;

	I_Assert(target->type < NUMMOBJTYPES);

	lua_settop(gL, 0);

	// Look for all generic mobj death hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MobjDeath)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, target, META_MOBJ);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		if (lua_pcall(gL, 4, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[target->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MobjDeath)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, target, META_MOBJ);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		if (lua_pcall(gL, 4, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for B_BuildTiccmd
boolean LUAh_BotTiccmd(player_t *bot, ticcmd_t *cmd)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_BotTiccmd/8] & (1<<(hook_BotTiccmd%8))))
		return false;

	lua_settop(gL, 0);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_BotTiccmd)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, bot, META_PLAYER);
			LUA_PushUserdata(gL, cmd, META_TICCMD);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for B_BuildTailsTiccmd by skin name
boolean LUAh_BotAI(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_BotAI/8] & (1<<(hook_BotAI%8))))
		return false;

	lua_settop(gL, 0);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_BotAI
		|| (hookp->s.str && strcmp(hookp->s.str, ((skin_t*)tails->skin)->name)))
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, sonic, META_MOBJ);
			LUA_PushUserdata(gL, tails, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 8, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}

		// This turns forward, backward, left, right, jump, and spin into a proper ticcmd for tails.
		if (lua_istable(gL, 2+1)) {
			boolean forward=false, backward=false, left=false, right=false, strafeleft=false, straferight=false, jump=false, spin=false;
#define CHECKFIELD(field) \
			lua_getfield(gL, 2+1, #field);\
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
			B_KeysToTiccmd(tails, cmd, lua_toboolean(gL, 2+1), lua_toboolean(gL, 2+2), lua_toboolean(gL, 2+3), lua_toboolean(gL, 2+4), lua_toboolean(gL, 2+5), lua_toboolean(gL, 2+6), lua_toboolean(gL, 2+7), lua_toboolean(gL, 2+8));

		lua_pop(gL, 8);
		hooked = true;
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for B_CheckRespawn
boolean LUAh_BotRespawn(mobj_t *sonic, mobj_t *tails)
{
	hook_p hookp;
	UINT8 shouldRespawn = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL || !(hooksAvailable[hook_BotRespawn/8] & (1<<(hook_BotRespawn%8))))
		return false;

	lua_settop(gL, 0);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_BotRespawn)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, sonic, META_MOBJ);
			LUA_PushUserdata(gL, tails, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{
			if (lua_toboolean(gL, -1))
				shouldRespawn = 1; // Force yes
			else
				shouldRespawn = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return shouldRespawn;
}

// Hook for linedef executors
boolean LUAh_LinedefExecute(line_t *line, mobj_t *mo, sector_t *sector)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_LinedefExecute/8] & (1<<(hook_LinedefExecute%8))))
		return 0;

	lua_settop(gL, 0);

	for (hookp = linedefexecutorhooks; hookp; hookp = hookp->next)
	{
		if (strcmp(hookp->s.str, line->text))
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, line, META_LINE);
			LUA_PushUserdata(gL, mo, META_MOBJ);
			LUA_PushUserdata(gL, sector, META_SECTOR);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -4);
		lua_pushvalue(gL, -4);
		lua_pushvalue(gL, -4);
		LUA_Call(gL, 3);
		hooked = true;
	}

	lua_settop(gL, 0);
	return hooked;
}


boolean LUAh_PlayerMsg(int source, int target, int flags, char *msg)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_PlayerMsg/8] & (1<<(hook_PlayerMsg%8))))
		return false;

	lua_settop(gL, 0);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_PlayerMsg)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, &players[source], META_PLAYER); // Source player
			if (flags & 2 /*HU_CSAY*/) { // csay TODO: make HU_CSAY accessible outside hu_stuff.c
				lua_pushinteger(gL, 3); // type
				lua_pushnil(gL); // target
			} else if (target == -1) { // sayteam
				lua_pushinteger(gL, 1); // type
				lua_pushnil(gL); // target
			} else if (target == 0) { // say
				lua_pushinteger(gL, 0); // type
				lua_pushnil(gL); // target
			} else { // sayto
				lua_pushinteger(gL, 2); // type
				LUA_PushUserdata(gL, &players[target-1], META_PLAYER); // target
			}
			lua_pushstring(gL, msg); // msg
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		if (lua_pcall(gL, 4, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}


// Hook for hurt messages
boolean LUAh_HurtMsg(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_HurtMsg/8] & (1<<(hook_HurtMsg%8))))
		return false;

	lua_settop(gL, 0);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_HurtMsg
		|| (hookp->s.mt && !(inflictor && hookp->s.mt == inflictor->type)))
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			LUA_PushUserdata(gL, inflictor, META_MOBJ);
			LUA_PushUserdata(gL, source, META_MOBJ);
			lua_pushinteger(gL, damagetype);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		lua_pushvalue(gL, -5);
		if (lua_pcall(gL, 4, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

void LUAh_NetArchiveHook(lua_CFunction archFunc)
{
	hook_p hookp;

	if (!gL || !(hooksAvailable[hook_NetVars/8] & (1<<(hook_NetVars%8))))
		return;

	// stack: tables
	I_Assert(lua_gettop(gL) > 0);
	I_Assert(lua_istable(gL, -1));

	// tables becomes an upvalue of archFunc
	lua_pushvalue(gL, -1);
	lua_pushcclosure(gL, archFunc, 1);
	// stack: tables, archFunc

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_NetVars)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -2); // archFunc
		LUA_Call(gL, 1);
	}

	lua_pop(gL, 1); // pop archFunc
	// stack: tables
}

boolean LUAh_MapThingSpawn(mobj_t *mo, mapthing_t *mthing)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_MapThingSpawn/8] & (1<<(hook_MapThingSpawn%8))))
		return false;

	lua_settop(gL, 0);

	// Look for all generic mobj map thing spawn hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MapThingSpawn)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, mo, META_MOBJ);
			LUA_PushUserdata(gL, mthing, META_MAPTHING);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[mo->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_MapThingSpawn)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, mo, META_MOBJ);
			LUA_PushUserdata(gL, mthing, META_MAPTHING);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for P_PlayerAfterThink Smiles mobj-following
boolean LUAh_FollowMobj(player_t *player, mobj_t *mobj)
{
	hook_p hookp;
	boolean hooked = false;
	if (!gL || !(hooksAvailable[hook_FollowMobj/8] & (1<<(hook_FollowMobj%8))))
		return 0;

	lua_settop(gL, 0);

	// Look for all generic mobj follow item hooks
	for (hookp = mobjhooks[MT_NULL]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_FollowMobj)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			LUA_PushUserdata(gL, mobj, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	for (hookp = mobjhooks[mobj->type]; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_FollowMobj)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			LUA_PushUserdata(gL, mobj, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (lua_toboolean(gL, -1))
			hooked = true;
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return hooked;
}

// Hook for P_PlayerCanDamage
UINT8 LUAh_PlayerCanDamage(player_t *player, mobj_t *mobj)
{
	hook_p hookp;
	UINT8 shouldCollide = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL || !(hooksAvailable[hook_PlayerCanDamage/8] & (1<<(hook_PlayerCanDamage%8))))
		return 0;

	lua_settop(gL, 0);

	for (hookp = playerhooks; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_PlayerCanDamage)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			LUA_PushUserdata(gL, mobj, META_MOBJ);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave shouldCollide = 0.
			if (lua_toboolean(gL, -1))
				shouldCollide = 1; // Force yes
			else
				shouldCollide = 2; // Force no
		}
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return shouldCollide;
}

void LUAh_PlayerQuit(player_t *plr, kickreason_t reason)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_PlayerQuit/8] & (1<<(hook_PlayerQuit%8))))
		return;

	lua_settop(gL, 0);

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_PlayerQuit)
			continue;

	    if (lua_gettop(gL) == 0)
	    {
	        LUA_PushUserdata(gL, plr, META_PLAYER); // Player that quit
	        lua_pushinteger(gL, reason); // Reason for quitting
	    }
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		LUA_Call(gL, 2);
	}

	lua_settop(gL, 0);
}

// Hook for Y_Ticker
void LUAh_IntermissionThinker(void)
{
	hook_p hookp;
	if (!gL || !(hooksAvailable[hook_IntermissionThinker/8] & (1<<(hook_IntermissionThinker%8))))
		return;

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_IntermissionThinker)
			continue;

		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		if (lua_pcall(gL, 0, 0, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
		}
	}
}

// Hook for team switching
// It's just an edit of LUAh_ViewpointSwitch.
boolean LUAh_TeamSwitch(player_t *player, int newteam, boolean fromspectators, boolean tryingautobalance, boolean tryingscramble)
{
	hook_p hookp;
	boolean canSwitchTeam = true;
	if (!gL || !(hooksAvailable[hook_TeamSwitch/8] & (1<<(hook_TeamSwitch%8))))
		return true;

	lua_settop(gL, 0);

	for (hookp = playerhooks; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_TeamSwitch)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			lua_pushinteger(gL, newteam);
			lua_pushboolean(gL, fromspectators);
			lua_pushboolean(gL, tryingautobalance);
			lua_pushboolean(gL, tryingscramble);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		lua_pushvalue(gL, -6);
		if (lua_pcall(gL, 5, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1) && !lua_toboolean(gL, -1))
			canSwitchTeam = false; // Can't switch team
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	return canSwitchTeam;
}

// Hook for spy mode
UINT8 LUAh_ViewpointSwitch(player_t *player, player_t *newdisplayplayer, boolean forced)
{
	hook_p hookp;
	UINT8 canSwitchView = 0; // 0 = default, 1 = force yes, 2 = force no.
	if (!gL || !(hooksAvailable[hook_ViewpointSwitch/8] & (1<<(hook_ViewpointSwitch%8))))
		return 0;

	lua_settop(gL, 0);
	hud_running = true; // local hook

	for (hookp = playerhooks; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_ViewpointSwitch)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			LUA_PushUserdata(gL, newdisplayplayer, META_PLAYER);
			lua_pushboolean(gL, forced);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -4);
		lua_pushvalue(gL, -4);
		lua_pushvalue(gL, -4);
		if (lua_pcall(gL, 3, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1))
		{ // if nil, leave canSwitchView = 0.
			if (lua_toboolean(gL, -1))
				canSwitchView = 1; // Force viewpoint switch
			else
				canSwitchView = 2; // Skip viewpoint switch
		}
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	hud_running = false;

	return canSwitchView;
}

// Hook for MT_NAMECHECK
#ifdef SEENAMES
boolean LUAh_SeenPlayer(player_t *player, player_t *seenfriend)
{
	hook_p hookp;
	boolean hasSeenPlayer = true;
	if (!gL || !(hooksAvailable[hook_SeenPlayer/8] & (1<<(hook_SeenPlayer%8))))
		return true;

	lua_settop(gL, 0);
	hud_running = true; // local hook

	for (hookp = playerhooks; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_SeenPlayer)
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			LUA_PushUserdata(gL, seenfriend, META_PLAYER);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1) && !lua_toboolean(gL, -1))
			hasSeenPlayer = false; // Hasn't seen player
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	hud_running = false;

	return hasSeenPlayer;
}
#endif // SEENAMES

boolean LUAh_ShouldJingleContinue(player_t *player, const char *musname)
{
	hook_p hookp;
	boolean keepplaying = false;
	if (!gL || !(hooksAvailable[hook_ShouldJingleContinue/8] & (1<<(hook_ShouldJingleContinue%8))))
		return true;

	lua_settop(gL, 0);
	hud_running = true; // local hook

	for (hookp = roothook; hookp; hookp = hookp->next)
	{
		if (hookp->type != hook_ShouldJingleContinue
			|| (hookp->s.str && strcmp(hookp->s.str, musname)))
			continue;

		if (lua_gettop(gL) == 0)
		{
			LUA_PushUserdata(gL, player, META_PLAYER);
			lua_pushstring(gL, musname);
		}
		lua_pushfstring(gL, FMT_HOOKID, hookp->id);
		lua_gettable(gL, LUA_REGISTRYINDEX);
		lua_pushvalue(gL, -3);
		lua_pushvalue(gL, -3);
		if (lua_pcall(gL, 2, 1, 0)) {
			if (!hookp->error || cv_debug & DBG_LUA)
				CONS_Alert(CONS_WARNING,"%s\n",lua_tostring(gL, -1));
			lua_pop(gL, 1);
			hookp->error = true;
			continue;
		}
		if (!lua_isnil(gL, -1) && lua_toboolean(gL, -1))
			keepplaying = true; // Keep playing this boolean
		lua_pop(gL, 1);
	}

	lua_settop(gL, 0);
	hud_running = false;

	return keepplaying;
}
