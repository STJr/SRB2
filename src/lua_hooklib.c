// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2022 by Sonic Team Junior.
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

#include "m_perfstats.h"
#include "d_netcmd.h" // for cv_perfstats
#include "i_system.h" // I_GetPreciseTime

/* =========================================================================
                                  ABSTRACTION
   ========================================================================= */

#define LIST(id, M) \
	static const char * const id [] = { M (TOSTR)  NULL }

LIST   (mobjHookNames,   MOBJ_HOOK_LIST);
LIST       (hookNames,        HOOK_LIST);
LIST    (hudHookNames,    HUD_HOOK_LIST);
LIST (stringHookNames, STRING_HOOK_LIST);

#undef LIST

typedef struct {
	int numHooks;
	int *ids;
} hook_t;

typedef struct {
	int numGeneric;
	int ref;
} stringhook_t;

static hook_t hookIds[HOOK(MAX)];
static hook_t hudHookIds[HUD_HOOK(MAX)];
static hook_t mobjHookIds[NUMMOBJTYPES][MOBJ_HOOK(MAX)];

// Lua tables are used to lookup string hook ids.
static stringhook_t stringHooks[STRING_HOOK(MAX)];

// This will be indexed by hook id, the value of which fetches the registry.
static int * hookRefs;
static int   nextid;

// After a hook errors once, don't print the error again.
static UINT8 * hooksErrored;

static int errorRef;

static boolean mobj_hook_available(int hook_type, mobjtype_t mobj_type)
{
	return
		(
				mobjHookIds [MT_NULL] [hook_type].numHooks > 0 ||
				mobjHookIds[mobj_type][hook_type].numHooks > 0
		);
}

static int hook_in_list
(
		const char * const         name,
		const char * const * const list
){
	int type;

	for (type = 0; list[type] != NULL; ++type)
	{
		if (strcmp(name, list[type]) == 0)
			break;
	}

	return type;
}

static void get_table(lua_State *L)
{
	lua_pushvalue(L, -1);
	lua_rawget(L, -3);

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_createtable(L, 1, 0);
		lua_pushvalue(L, -2);
		lua_pushvalue(L, -2);
		lua_rawset(L, -5);
	}

	lua_remove(L, -2);
}

static void add_hook_to_table(lua_State *L, int n)
{
	lua_pushnumber(L, nextid);
	lua_rawseti(L, -2, n);
}

static void add_string_hook(lua_State *L, int type)
{
	stringhook_t * hook = &stringHooks[type];

	char * string = NULL;

	switch (type)
	{
		case STRING_HOOK(BotAI):
		case STRING_HOOK(ShouldJingleContinue):
			if (lua_isstring(L, 3))
			{ // lowercase copy
				string = Z_StrDup(lua_tostring(L, 3));
				strlwr(string);
			}
			break;

		case STRING_HOOK(LinedefExecute):
			string = Z_StrDup(luaL_checkstring(L, 3));
			strupr(string);
			break;
	}

	if (hook->ref > 0)
		lua_getref(L, hook->ref);
	else
	{
		lua_newtable(L);
		lua_pushvalue(L, -1);
		hook->ref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	if (string)
	{
		lua_pushstring(L, string);
		get_table(L);
		add_hook_to_table(L, 1 + lua_objlen(L, -1));
	}
	else
		add_hook_to_table(L, ++hook->numGeneric);
}

static void add_hook(hook_t *map)
{
	Z_Realloc(map->ids, (map->numHooks + 1) * sizeof *map->ids,
			PU_STATIC, &map->ids);
	map->ids[map->numHooks++] = nextid;
}

static void add_mobj_hook(lua_State *L, int hook_type)
{
	mobjtype_t   mobj_type = luaL_optnumber(L, 3, MT_NULL);

	luaL_argcheck(L, mobj_type < NUMMOBJTYPES, 3, "invalid mobjtype_t");

	add_hook(&mobjHookIds[mobj_type][hook_type]);
}

static void add_hud_hook(lua_State *L, int idx)
{
	add_hook(&hudHookIds[luaL_checkoption(L,
				idx, "game", hudHookNames)]);
}

static void add_hook_ref(lua_State *L, int idx)
{
	if (!(nextid & 7))
	{
		Z_Realloc(hooksErrored,
				BIT_ARRAY_SIZE (nextid + 1) * sizeof *hooksErrored,
				PU_STATIC, &hooksErrored);
		hooksErrored[nextid >> 3] = 0;
	}

	Z_Realloc(hookRefs, (nextid + 1) * sizeof *hookRefs, PU_STATIC, &hookRefs);

	// set the hook function in the registry.
	lua_pushvalue(L, idx);
	hookRefs[nextid++] = luaL_ref(L, LUA_REGISTRYINDEX);
}

// Takes hook, function, and additional arguments (mobj type to act on, etc.)
static int lib_addHook(lua_State *L)
{
	const char * name;
	int type;

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	name = luaL_checkstring(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);

	/* this is a very special case */
	if (( type = hook_in_list(name, stringHookNames) ) < STRING_HOOK(MAX))
	{
		add_string_hook(L, type);
	}
	else if (( type = hook_in_list(name, mobjHookNames) ) < MOBJ_HOOK(MAX))
	{
		add_mobj_hook(L, type);
	}
	else if (( type = hook_in_list(name, hookNames) ) < HOOK(MAX))
	{
		add_hook(&hookIds[type]);
	}
	else if (strcmp(name, "HUD") == 0)
	{
		add_hud_hook(L, 3);
	}
	else
	{
		return luaL_argerror(L, 1, lua_pushfstring(L, "invalid hook " LUA_QS, name));
	}

	add_hook_ref(L, 2);/* the function */

	return 0;
}

int LUA_HookLib(lua_State *L)
{
	lua_pushcfunction(L, LUA_GetErrorMessage);
	errorRef = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_register(L, "addHook", lib_addHook);

	return 0;
}

/* TODO: remove in next backwards incompatible release */
int lib_hudadd(lua_State *L);/* yeah compiler */
int lib_hudadd(lua_State *L)
{
	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	luaL_checktype(L, 1, LUA_TFUNCTION);

	add_hud_hook(L, 2);
	add_hook_ref(L, 1);

	return 0;
}

typedef struct Hook_State Hook_State;
typedef void (*Hook_Callback)(Hook_State *);

struct Hook_State {
	INT32        status;/* return status to calling function */
	void       * userdata;
	int          hook_type;
	mobjtype_t   mobj_type;/* >0 if mobj hook */
	const char * string;/* used to fetch table, ran first if set */
	int          top;/* index of last argument passed to hook */
	int          id;/* id to fetch ref */
	int          values;/* num arguments passed to hook */
	int          results;/* num values returned by hook */
	Hook_Callback results_handler;/* callback when hook successfully returns */
};

enum {
	EINDEX = 1,/* error handler */
	SINDEX = 2,/* string itself is pushed in case of string hook */
};

static void push_error_handler(void)
{
	lua_getref(gL, errorRef);
}

/* repush hook string */
static void push_string(void)
{
	lua_pushvalue(gL, SINDEX);
}

static boolean begin_hook_values(Hook_State *hook)
{
	hook->top = lua_gettop(gL);
	return true;
}

static void start_hook_stack(void)
{
	lua_settop(gL, 0);
	push_error_handler();
}

static boolean init_hook_type
(
		Hook_State * hook,
		int          status,
		int          hook_type,
		mobjtype_t   mobj_type,
		const char * string,
		int          nonzero
){
	hook->status = status;

	if (nonzero)
	{
		start_hook_stack();
		hook->hook_type = hook_type;
		hook->mobj_type = mobj_type;
		hook->string = string;
		return begin_hook_values(hook);
	}
	else
		return false;
}

static boolean prepare_hook
(
		Hook_State * hook,
		int default_status,
		int hook_type
){
	return init_hook_type(hook, default_status,
			hook_type, 0, NULL,
			hookIds[hook_type].numHooks);
}

static boolean prepare_mobj_hook
(
		Hook_State * hook,
		int          default_status,
		int          hook_type,
		mobjtype_t   mobj_type
){
#ifdef PARANOIA
	if (mobj_type == MT_NULL)
		I_Error("MT_NULL has been passed to a mobj hook\n");
#endif
	return init_hook_type(hook, default_status,
			hook_type, mobj_type, NULL,
			mobj_hook_available(hook_type, mobj_type));
}

static boolean prepare_string_hook
(
		Hook_State * hook,
		int          default_status,
		int          hook_type,
		const char * string
){
	if (init_hook_type(hook, default_status,
				hook_type, 0, string,
				stringHooks[hook_type].ref))
	{
		lua_pushstring(gL, string);
		return begin_hook_values(hook);
	}
	else
		return false;
}

static void init_hook_call
(
		Hook_State * hook,
		int    results,
		Hook_Callback results_handler
){
	const int top = lua_gettop(gL);
	hook->values = (top - hook->top);
	hook->top = top;
	hook->results = results;
	hook->results_handler = results_handler;
}

static void get_hook(Hook_State *hook, const int *ids, int n)
{
	hook->id = ids[n];
	lua_getref(gL, hookRefs[hook->id]);
}

static void get_hook_from_table(Hook_State *hook, int n)
{
	lua_rawgeti(gL, -1, n);
	hook->id = lua_tonumber(gL, -1);
	lua_pop(gL, 1);
	lua_getref(gL, hookRefs[hook->id]);
}

static int call_single_hook_no_copy(Hook_State *hook)
{
	if (lua_pcall(gL, hook->values, hook->results, EINDEX) == 0)
	{
		if (hook->results > 0)
		{
			(*hook->results_handler)(hook);
			lua_pop(gL, hook->results);
		}
	}
	else
	{
		/* print the error message once */
		if (cv_debug & DBG_LUA || !in_bit_array(hooksErrored, hook->id))
		{
			CONS_Alert(CONS_WARNING, "%s\n", lua_tostring(gL, -1));
			set_bit_array(hooksErrored, hook->id);
		}
		lua_pop(gL, 1);
	}

	return 1;
}

static int call_single_hook(Hook_State *hook)
{
	int i;

	for (i = -(hook->values) + 1; i <= 0; ++i)
		lua_pushvalue(gL, hook->top + i);

	return call_single_hook_no_copy(hook);
}

static int call_hook_table_for(Hook_State *hook, int n)
{
	int k;

	for (k = 1; k <= n; ++k)
	{
		get_hook_from_table(hook, k);
		call_single_hook(hook);
	}

	return n;
}

static int call_hook_table(Hook_State *hook)
{
	return call_hook_table_for(hook, lua_objlen(gL, -1));
}

static int call_mapped(Hook_State *hook, const hook_t *map)
{
	int k;

	for (k = 0; k < map->numHooks; ++k)
	{
		get_hook(hook, map->ids, k);
		call_single_hook(hook);
	}

	return map->numHooks;
}

static int call_string_hooks(Hook_State *hook)
{
	const stringhook_t *map = &stringHooks[hook->hook_type];

	int calls = 0;

	lua_getref(gL, map->ref);

	/* call generic string hooks first */
	calls += call_hook_table_for(hook, map->numGeneric);

	push_string();
	lua_rawget(gL, -2);
	calls += call_hook_table(hook);

	return calls;
}

static int call_mobj_type_hooks(Hook_State *hook, mobjtype_t mobj_type)
{
	return call_mapped(hook, &mobjHookIds[mobj_type][hook->hook_type]);
}

static int call_hooks
(
		Hook_State * hook,
		int        results,
		Hook_Callback results_handler
){
	int calls = 0;

	init_hook_call(hook, results, results_handler);

	if (hook->string)
	{
		calls += call_string_hooks(hook);
	}
	else if (hook->mobj_type > 0)
	{
		/* call generic mobj hooks first */
		calls += call_mobj_type_hooks(hook, MT_NULL);
		calls += call_mobj_type_hooks(hook, hook->mobj_type);

		ps_lua_mobjhooks.value.i += calls;
	}
	else
		calls += call_mapped(hook, &hookIds[hook->hook_type]);

	lua_settop(gL, 0);

	return calls;
}

/* =========================================================================
                            COMMON RESULT HANDLERS
   ========================================================================= */

#define res_none NULL

static void res_true(Hook_State *hook)
{
	if (lua_toboolean(gL, -1))
		hook->status = true;
}

static void res_false(Hook_State *hook)
{
	if (!lua_isnil(gL, -1) && !lua_toboolean(gL, -1))
		hook->status = false;
}

static void res_force(Hook_State *hook)
{
	if (!lua_isnil(gL, -1))
	{
		if (lua_toboolean(gL, -1))
			hook->status = 1; // Force yes
		else
			hook->status = 2; // Force no
	}
}

/* =========================================================================
                               GENERALISED HOOKS
   ========================================================================= */

int LUA_HookMobj(mobj_t *mobj, int hook_type)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, false, hook_type, mobj->type))
	{
		LUA_PushUserdata(gL, mobj, META_MOBJ);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

int LUA_Hook2Mobj(mobj_t *t1, mobj_t *t2, int hook_type)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, 0, hook_type, t1->type))
	{
		LUA_PushUserdata(gL, t1, META_MOBJ);
		LUA_PushUserdata(gL, t2, META_MOBJ);
		call_hooks(&hook, 1, res_force);
	}
	return hook.status;
}

void LUA_HookVoid(int type)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, type))
		call_hooks(&hook, 0, res_none);
}

void LUA_HookInt(INT32 number, int hook_type)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, hook_type))
	{
		lua_pushinteger(gL, number);
		call_hooks(&hook, 0, res_none);
	}
}

void LUA_HookBool(boolean value, int hook_type)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, hook_type))
	{
		lua_pushboolean(gL, value);
		call_hooks(&hook, 0, res_none);
	}
}

int LUA_HookPlayer(player_t *player, int hook_type)
{
	Hook_State hook;
	if (prepare_hook(&hook, false, hook_type))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

int LUA_HookTiccmd(player_t *player, ticcmd_t *cmd, int hook_type)
{
	Hook_State hook;
	if (prepare_hook(&hook, false, hook_type))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		LUA_PushUserdata(gL, cmd, META_TICCMD);

		if (hook_type == HOOK(PlayerCmd))
			hook_cmd_running = true;

		call_hooks(&hook, 1, res_true);

		if (hook_type == HOOK(PlayerCmd))
			hook_cmd_running = false;
	}
	return hook.status;
}

int LUA_HookKey(event_t *event, int hook_type)
{
	Hook_State hook;
	if (prepare_hook(&hook, false, hook_type))
	{
		LUA_PushUserdata(gL, event, META_KEYEVENT);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

void LUA_HookHUD(int hook_type)
{
	const hook_t * map = &hudHookIds[hook_type];
	Hook_State hook;
	if (map->numHooks > 0)
	{
		start_hook_stack();
		begin_hook_values(&hook);

		LUA_SetHudHook(hook_type);

		hud_running = true; // local hook
		init_hook_call(&hook, 0, res_none);
		call_mapped(&hook, map);
		hud_running = false;
	}
}

/* =========================================================================
                               SPECIALIZED HOOKS
   ========================================================================= */

void LUA_HookThinkFrame(void)
{
	const int type = HOOK(ThinkFrame);

	// variables used by perf stats
	int hook_index = 0;
	precise_t time_taken = 0;

	Hook_State hook;

	const hook_t * map = &hookIds[type];
	int k;

	if (prepare_hook(&hook, 0, type))
	{
		init_hook_call(&hook, 0, res_none);

		for (k = 0; k < map->numHooks; ++k)
		{
			get_hook(&hook, map->ids, k);

			if (cv_perfstats.value == 3)
			{
				lua_pushvalue(gL, -1);/* need the function again */
				time_taken = I_GetPreciseTime();
			}

			call_single_hook(&hook);

			if (cv_perfstats.value == 3)
			{
				lua_Debug ar;
				time_taken = I_GetPreciseTime() - time_taken;
				lua_getinfo(gL, ">S", &ar);
				PS_SetThinkFrameHookInfo(hook_index, time_taken, ar.short_src);
				hook_index++;
			}
		}

		lua_settop(gL, 0);
	}
}

int LUA_HookMobjLineCollide(mobj_t *mobj, line_t *line)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, 0, MOBJ_HOOK(MobjLineCollide), mobj->type))
	{
		LUA_PushUserdata(gL, mobj, META_MOBJ);
		LUA_PushUserdata(gL, line, META_LINE);
		call_hooks(&hook, 1, res_force);
	}
	return hook.status;
}

int LUA_HookTouchSpecial(mobj_t *special, mobj_t *toucher)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, false, MOBJ_HOOK(TouchSpecial), special->type))
	{
		LUA_PushUserdata(gL, special, META_MOBJ);
		LUA_PushUserdata(gL, toucher, META_MOBJ);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

static int damage_hook
(
		mobj_t *target,
		mobj_t *inflictor,
		mobj_t *source,
		INT32   damage,
		UINT8   damagetype,
		int     hook_type,
		Hook_Callback results_handler
){
	Hook_State hook;
	if (prepare_mobj_hook(&hook, 0, hook_type, target->type))
	{
		LUA_PushUserdata(gL, target, META_MOBJ);
		LUA_PushUserdata(gL, inflictor, META_MOBJ);
		LUA_PushUserdata(gL, source, META_MOBJ);
		if (hook_type != MOBJ_HOOK(MobjDeath))
			lua_pushinteger(gL, damage);
		lua_pushinteger(gL, damagetype);
		call_hooks(&hook, 1, results_handler);
	}
	return hook.status;
}

int LUA_HookShouldDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	return damage_hook(target, inflictor, source, damage, damagetype,
			MOBJ_HOOK(ShouldDamage), res_force);
}

int LUA_HookMobjDamage(mobj_t *target, mobj_t *inflictor, mobj_t *source, INT32 damage, UINT8 damagetype)
{
	return damage_hook(target, inflictor, source, damage, damagetype,
			MOBJ_HOOK(MobjDamage), res_true);
}

int LUA_HookMobjDeath(mobj_t *target, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	return damage_hook(target, inflictor, source, 0, damagetype,
			MOBJ_HOOK(MobjDeath), res_true);
}

int LUA_HookMobjMoveBlocked(mobj_t *t1, mobj_t *t2, line_t *line)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, 0, MOBJ_HOOK(MobjMoveBlocked), t1->type))
	{
		LUA_PushUserdata(gL, t1, META_MOBJ);
		LUA_PushUserdata(gL, t2, META_MOBJ);
		LUA_PushUserdata(gL, line, META_LINE);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

typedef struct {
	mobj_t   * tails;
	ticcmd_t * cmd;
} BotAI_State;

static boolean checkbotkey(const char *field)
{
	return lua_toboolean(gL, -1) && strcmp(lua_tostring(gL, -2), field) == 0;
}

static void res_botai(Hook_State *hook)
{
	BotAI_State *botai = hook->userdata;

	int k[8];

	int fields = 0;

	// This turns forward, backward, left, right, jump, and spin into a proper ticcmd for tails.
	if (lua_istable(gL, -8)) {
		lua_pushnil(gL); // key
		while (lua_next(gL, -9)) {
#define CHECK(n, f) (checkbotkey(f) ? (k[(n)-1] = 1) : 0)
			if (
					CHECK(1,    "forward") || CHECK(2,    "backward") ||
					CHECK(3,       "left") || CHECK(4,       "right") ||
					CHECK(5, "strafeleft") || CHECK(6, "straferight") ||
					CHECK(7,       "jump") || CHECK(8,        "spin")
			){
				if (8 <= ++fields)
				{
					lua_pop(gL, 2); // pop key and value
					break;
				}
			}

			lua_pop(gL, 1); // pop value
#undef CHECK
		}
	} else {
		while (fields < 8)
		{
			k[fields] = lua_toboolean(gL, -8 + fields);
			fields++;
		}
	}

	B_KeysToTiccmd(botai->tails, botai->cmd,
			k[0],k[1],k[2],k[3],k[4],k[5],k[6],k[7]);

	hook->status = true;
}

int LUA_HookBotAI(mobj_t *sonic, mobj_t *tails, ticcmd_t *cmd)
{
	const char *skin = ((skin_t *)tails->skin)->name;

	Hook_State hook;
	BotAI_State botai;

	if (prepare_string_hook(&hook, false, STRING_HOOK(BotAI), skin))
	{
		LUA_PushUserdata(gL, sonic, META_MOBJ);
		LUA_PushUserdata(gL, tails, META_MOBJ);

		botai.tails = tails;
		botai.cmd   = cmd;

		hook.userdata = &botai;

		call_hooks(&hook, 8, res_botai);
	}

	return hook.status;
}

void LUA_HookLinedefExecute(line_t *line, mobj_t *mo, sector_t *sector)
{
	Hook_State hook;
	if (prepare_string_hook
			(&hook, 0, STRING_HOOK(LinedefExecute), line->stringargs[0]))
	{
		LUA_PushUserdata(gL, line, META_LINE);
		LUA_PushUserdata(gL, mo, META_MOBJ);
		LUA_PushUserdata(gL, sector, META_SECTOR);
		ps_lua_mobjhooks.value.i += call_hooks(&hook, 0, res_none);
	}
}

int LUA_HookPlayerMsg(int source, int target, int flags, char *msg)
{
	Hook_State hook;
	if (prepare_hook(&hook, false, HOOK(PlayerMsg)))
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
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

int LUA_HookHurtMsg(player_t *player, mobj_t *inflictor, mobj_t *source, UINT8 damagetype)
{
	Hook_State hook;
	if (prepare_hook(&hook, false, HOOK(HurtMsg)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		LUA_PushUserdata(gL, inflictor, META_MOBJ);
		LUA_PushUserdata(gL, source, META_MOBJ);
		lua_pushinteger(gL, damagetype);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

void LUA_HookNetArchive(lua_CFunction archFunc)
{
	const hook_t * map = &hookIds[HOOK(NetVars)];
	Hook_State hook;
	/* this is a remarkable case where the stack isn't reset */
	if (map->numHooks > 0)
	{
		// stack: tables
		I_Assert(lua_gettop(gL) > 0);
		I_Assert(lua_istable(gL, -1));

		push_error_handler();
		lua_insert(gL, EINDEX);

		begin_hook_values(&hook);

		// tables becomes an upvalue of archFunc
		lua_pushvalue(gL, -1);
		lua_pushcclosure(gL, archFunc, 1);
		// stack: tables, archFunc

		init_hook_call(&hook, 0, res_none);
		call_mapped(&hook, map);

		lua_pop(gL, 1); // pop archFunc
		lua_remove(gL, EINDEX); // pop error handler
		// stack: tables
	}
}

int LUA_HookMapThingSpawn(mobj_t *mobj, mapthing_t *mthing)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, false, MOBJ_HOOK(MapThingSpawn), mobj->type))
	{
		LUA_PushUserdata(gL, mobj, META_MOBJ);
		LUA_PushUserdata(gL, mthing, META_MAPTHING);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

int LUA_HookFollowMobj(player_t *player, mobj_t *mobj)
{
	Hook_State hook;
	if (prepare_mobj_hook(&hook, false, MOBJ_HOOK(FollowMobj), mobj->type))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		LUA_PushUserdata(gL, mobj, META_MOBJ);
		call_hooks(&hook, 1, res_true);
	}
	return hook.status;
}

int LUA_HookPlayerCanDamage(player_t *player, mobj_t *mobj)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, HOOK(PlayerCanDamage)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		LUA_PushUserdata(gL, mobj, META_MOBJ);
		call_hooks(&hook, 1, res_force);
	}
	return hook.status;
}

void LUA_HookPlayerQuit(player_t *plr, kickreason_t reason)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, HOOK(PlayerQuit)))
	{
		LUA_PushUserdata(gL, plr, META_PLAYER); // Player that quit
		lua_pushinteger(gL, reason); // Reason for quitting
		call_hooks(&hook, 0, res_none);
	}
}

int LUA_HookTeamSwitch(player_t *player, int newteam, boolean fromspectators, boolean tryingautobalance, boolean tryingscramble)
{
	Hook_State hook;
	if (prepare_hook(&hook, true, HOOK(TeamSwitch)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		lua_pushinteger(gL, newteam);
		lua_pushboolean(gL, fromspectators);
		lua_pushboolean(gL, tryingautobalance);
		lua_pushboolean(gL, tryingscramble);
		call_hooks(&hook, 1, res_false);
	}
	return hook.status;
}

int LUA_HookViewpointSwitch(player_t *player, player_t *newdisplayplayer, boolean forced)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, HOOK(ViewpointSwitch)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		LUA_PushUserdata(gL, newdisplayplayer, META_PLAYER);
		lua_pushboolean(gL, forced);

		hud_running = true; // local hook
		call_hooks(&hook, 1, res_force);
		hud_running = false;
	}
	return hook.status;
}

int LUA_HookSeenPlayer(player_t *player, player_t *seenfriend)
{
	Hook_State hook;
	if (prepare_hook(&hook, true, HOOK(SeenPlayer)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		LUA_PushUserdata(gL, seenfriend, META_PLAYER);

		hud_running = true; // local hook
		call_hooks(&hook, 1, res_false);
		hud_running = false;
	}
	return hook.status;
}

int LUA_HookShouldJingleContinue(player_t *player, const char *musname)
{
	Hook_State hook;
	if (prepare_string_hook
			(&hook, false, STRING_HOOK(ShouldJingleContinue), musname))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		push_string();

		hud_running = true; // local hook
		call_hooks(&hook, 1, res_true);
		hud_running = false;
	}
	return hook.status;
}

boolean hook_cmd_running = false;

static void update_music_name(struct MusicChange *musicchange)
{
	size_t length;
	const char * new = lua_tolstring(gL, -6, &length);

	if (length < 7)
	{
		strcpy(musicchange->newname, new);
		lua_pushvalue(gL, -6);/* may as well keep it for next call */
	}
	else
	{
		memcpy(musicchange->newname, new, 6);
		musicchange->newname[6] = '\0';
		lua_pushlstring(gL, new, 6);
	}

	lua_replace(gL, -7);
}

static void res_musicchange(Hook_State *hook)
{
	struct MusicChange *musicchange = hook->userdata;

	// output 1: true, false, or string musicname override
	if (lua_isstring(gL, -6))
		update_music_name(musicchange);
	else if (lua_isboolean(gL, -6) && lua_toboolean(gL, -6))
		hook->status = true;

	// output 2: mflags override
	if (lua_isnumber(gL, -5))
		*musicchange->mflags = lua_tonumber(gL, -5);
	// output 3: looping override
	if (lua_isboolean(gL, -4))
		*musicchange->looping = lua_toboolean(gL, -4);
	// output 4: position override
	if (lua_isnumber(gL, -3))
		*musicchange->position = lua_tonumber(gL, -3);
	// output 5: prefadems override
	if (lua_isnumber(gL, -2))
		*musicchange->prefadems = lua_tonumber(gL, -2);
	// output 6: fadeinms override
	if (lua_isnumber(gL, -1))
		*musicchange->fadeinms = lua_tonumber(gL, -1);
}

int LUA_HookMusicChange(const char *oldname, struct MusicChange *param)
{
	const int type = HOOK(MusicChange);
	const hook_t * map = &hookIds[type];

	Hook_State hook;

	int k;

	if (prepare_hook(&hook, false, type))
	{
		init_hook_call(&hook, 6, res_musicchange);
		hook.values = 7;/* values pushed later */
		hook.userdata = param;

		lua_pushstring(gL, oldname);/* the only constant value */
		lua_pushstring(gL, param->newname);/* semi constant */

		for (k = 0; k < map->numHooks; ++k)
		{
			get_hook(&hook, map->ids, k);

			lua_pushvalue(gL, -3);
			lua_pushvalue(gL, -3);
			lua_pushinteger(gL, *param->mflags);
			lua_pushboolean(gL, *param->looping);
			lua_pushinteger(gL, *param->position);
			lua_pushinteger(gL, *param->prefadems);
			lua_pushinteger(gL, *param->fadeinms);

			call_single_hook_no_copy(&hook);
		}

		lua_settop(gL, 0);
	}

	return hook.status;
}

static void res_playerheight(Hook_State *hook)
{
	if (!lua_isnil(gL, -1))
	{
		fixed_t returnedheight = lua_tonumber(gL, -1);
		// 0 height has... strange results, but it's not problematic like negative heights are.
		// when an object's height is set to a negative number directly with lua, it's forced to 0 instead.
		// here, I think it's better to ignore negatives so that they don't replace any results of previous hooks!
		if (returnedheight >= 0)
			hook->status = returnedheight;
	}
}

fixed_t LUA_HookPlayerHeight(player_t *player)
{
	Hook_State hook;
	if (prepare_hook(&hook, -1, HOOK(PlayerHeight)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		call_hooks(&hook, 1, res_playerheight);
	}
	return hook.status;
}

int LUA_HookPlayerCanEnterSpinGaps(player_t *player)
{
	Hook_State hook;
	if (prepare_hook(&hook, 0, HOOK(PlayerCanEnterSpinGaps)))
	{
		LUA_PushUserdata(gL, player, META_PLAYER);
		call_hooks(&hook, 1, res_force);
	}
	return hook.status;
}
