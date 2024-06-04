// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_baselib.c
/// \brief basic functions for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_local.h"
#include "p_setup.h" // So we can have P_SetupLevelSky
#include "p_slopes.h" // P_GetSlopeZAt
#include "z_zone.h"
#include "r_main.h"
#include "r_draw.h"
#include "r_things.h" // R_Frame2Char etc
#include "m_random.h"
#include "s_sound.h"
#include "g_game.h"
#include "m_menu.h"
#include "y_inter.h"
#include "hu_stuff.h"	// HU_AddChatText
#include "console.h"
#include "netcode/d_netcmd.h" // IsPlayerAdmin
#include "m_menu.h" // Player Setup menu color stuff
#include "m_misc.h" // M_MapNumber
#include "b_bot.h" // B_UpdateBotleader
#include "netcode/d_clisrv.h" // CL_RemovePlayer
#include "i_system.h" // I_GetPreciseTime, I_GetPrecisePrecision

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "taglist.h" // P_FindSpecialLineFromTag
#include "lua_hook.h" // hook_cmd_running errors

#define NOHUD if (hud_running)\
return luaL_error(L, "HUD rendering code should not call this function!");\
else if (hook_cmd_running)\
return luaL_error(L, "CMD building code should not call this function!");

#define NOSPAWNNULL if (type >= NUMMOBJTYPES)\
return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);\
else if (type == MT_NULL)\
{\
	if (!nospawnnull_seen) {\
		nospawnnull_seen = true;\
		CONS_Alert(CONS_WARNING,"Spawning an \"MT_NULL\" mobj is deprecated and will be removed.\nUse \"MT_RAY\" instead.\n");\
	}\
type = MT_RAY;\
}
static boolean nospawnnull_seen = false; // TODO: 2.3: Delete
// TODO: 2.3: Use the below NOSPAWNNULL define instead. P_SpawnMobj used to say "if MT_NULL, use MT_RAY instead", so the above define maintains Lua script compatibility until v2.3
/*#define NOSPAWNNULL if (type <= MT_NULL || type >= NUMMOBJTYPES)\
return luaL_error(L, "mobj type %d out of range (1 - %d)", type, NUMMOBJTYPES-1);*/

boolean luaL_checkboolean(lua_State *L, int narg) {
	luaL_checktype(L, narg, LUA_TBOOLEAN);
	return lua_toboolean(L, narg);
}

// String concatination
static int lib_concat(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  char *r = NULL;
  size_t rl = 0,sl;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tolstring(L, -1, &sl);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
													 LUA_QL("__add"));
		r = Z_Realloc(r, rl+sl, PU_STATIC, NULL);
		M_Memcpy(r+rl, s, sl);
		rl += sl;
    lua_pop(L, 1);  /* pop result */
  }
  lua_pushlstring(L, r, rl);
  Z_Free(r);
	return 1;
}

// Wrapper for CONS_Printf
// Copied from base Lua code
static int lib_print(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  //HUDSAFE
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
													 LUA_QL("print"));
    if (i>1) CONS_Printf("\n");
    CONS_Printf("%s", s);
    lua_pop(L, 1);  /* pop result */
  }
	CONS_Printf("\n");
	return 0;
}

// Print stuff in the chat, or in the console if we can't.
static int lib_chatprint(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);	// retrieve string
	boolean sound = lua_optboolean(L, 2);	// retrieve sound boolean
	int len = strlen(str);

	if (str == NULL)	// error if we don't have a string!
		return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("chatprint"));

	if (len > 255)	// string is too long!!!
		return luaL_error(L, "String exceeds the 255 characters limit of the chat buffer.");

	HU_AddChatText(str, sound);
	return 0;
}

// Same as above, but do it for only one player.
static int lib_chatprintf(lua_State *L)
{
	int n = lua_gettop(L);  /* number of arguments */
	const char *str = luaL_checkstring(L, 2);	// retrieve string
	boolean sound = lua_optboolean(L, 3);	// sound?
	int len = strlen(str);
	player_t *plr;

	if (n < 2)
		return luaL_error(L, "chatprintf requires at least two arguments: player and text.");

	plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));	// retrieve player
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");
	if (plr != &players[consoleplayer])
		return 0;

	if (str == NULL)	// error if we don't have a string!
		return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("chatprintf"));

	if (len > 255)	// string is too long!!!
		return luaL_error(L, "String exceeds the 255 characters limit of the chat buffer.");

	HU_AddChatText(str, sound);
	return 0;
}

static const struct {
	const char *meta;
	const char *utype;
} meta2utype[] = {
	{META_STATE,        "state_t"},
	{META_MOBJINFO,     "mobjinfo_t"},
	{META_SFXINFO,      "sfxinfo_t"},
	{META_SKINCOLOR,    "skincolor_t"},
	{META_COLORRAMP,    "skincolor_t.ramp"},
	{META_SPRITEINFO,   "spriteinfo_t"},
	{META_PIVOTLIST,    "spriteframepivot_t[]"},
	{META_FRAMEPIVOT,   "spriteframepivot_t"},

	{META_TAGLIST,      "taglist"},

	{META_MOBJ,         "mobj_t"},
	{META_MAPTHING,     "mapthing_t"},

	{META_PLAYER,       "player_t"},
	{META_TICCMD,       "ticcmd_t"},
	{META_SKIN,         "skin_t"},
	{META_POWERS,       "player_t.powers"},
	{META_SOUNDSID,     "skin_t.soundsid"},
	{META_SKINSPRITES,  "skin_t.sprites"},
	{META_SKINSPRITESLIST,  "skin_t.sprites[]"},

	{META_VERTEX,       "vertex_t"},
	{META_LINE,         "line_t"},
	{META_SIDE,         "side_t"},
	{META_SUBSECTOR,    "subsector_t"},
	{META_SECTOR,       "sector_t"},
	{META_FFLOOR,       "ffloor_t"},
#ifdef HAVE_LUA_SEGS
	{META_SEG,          "seg_t"},
	{META_NODE,         "node_t"},
#endif
	{META_SLOPE,        "slope_t"},
	{META_VECTOR2,      "vector2_t"},
	{META_VECTOR3,      "vector3_t"},
	{META_MAPHEADER,    "mapheader_t"},

	{META_POLYOBJ,      "polyobj_t"},
	{META_POLYOBJVERTICES, "polyobj_t.vertices"},
	{META_POLYOBJLINES, "polyobj_t.lines"},

	{META_CVAR,         "consvar_t"},

	{META_SECTORLINES,  "sector_t.lines"},
#ifdef MUTABLE_TAGS
	{META_SECTORTAGLIST, "sector_t.taglist"},
#endif
	{META_SIDENUM,      "line_t.sidenum"},
	{META_LINEARGS,     "line_t.args"},
	{META_LINESTRINGARGS, "line_t.stringargs"},

	{META_THINGARGS,     "mapthing.args"},
	{META_THINGSTRINGARGS, "mapthing.stringargs"},
#ifdef HAVE_LUA_SEGS
	{META_NODEBBOX,     "node_t.bbox"},
	{META_NODECHILDREN, "node_t.children"},
#endif

	{META_BBOX,         "bbox"},

	{META_HUDINFO,      "hudinfo_t"},
	{META_PATCH,        "patch_t"},
	{META_COLORMAP,     "colormap"},
	{META_EXTRACOLORMAP,"extracolormap_t"},
	{META_CAMERA,       "camera_t"},

	{META_ACTION,       "action"},

	{META_LUABANKS,     "luabanks[]"},

	{META_KEYEVENT,     "keyevent_t"},
	{META_MOUSE,        "mouse_t"},
	{NULL,              NULL}
};

// goes through the above list and returns the utype string for the userdata type
// returns "unknown" instead if we couldn't find the right userdata type
static const char *GetUserdataUType(lua_State *L)
{
	UINT8 i;
	lua_getmetatable(L, -1);

	for (i = 0; meta2utype[i].meta; i++)
	{
		luaL_getmetatable(L, meta2utype[i].meta);
		if (lua_rawequal(L, -1, -2))
		{
			lua_pop(L, 2);
			return meta2utype[i].utype;
		}
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	return "unknown";
}

// Return a string representing the type of userdata the given var is
// e.g. players[0] -> "player_t"
//   or players[0].powers -> "player_t.powers"
static int lib_userdataType(lua_State *L)
{
	lua_settop(L, 1); // pop everything except arg 1 (in case somebody decided to add more)
	luaL_checktype(L, 1, LUA_TUSERDATA);
	lua_pushstring(L, GetUserdataUType(L));
	return 1;
}

// Takes a metatable as first and only argument
// Only callable during script loading
static int lib_registerMetatable(lua_State *L)
{
	static UINT16 nextid = 1;

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");
	luaL_checktype(L, 1, LUA_TTABLE);

	if (nextid == 0)
		return luaL_error(L, "Too many metatables registered?! Please consider rewriting your script once you are sober again.\n");

	lua_getfield(L, LUA_REGISTRYINDEX, LREG_METATABLES); // 2
		// registry.metatables[metatable] = nextid
		lua_pushvalue(L, 1); // 3
			lua_pushnumber(L, nextid); // 4
		lua_settable(L, 2);

		// registry.metatables[nextid] = metatable
		lua_pushnumber(L, nextid); // 3
			lua_pushvalue(L, 1); // 4
		lua_settable(L, 2);
	lua_pop(L, 1);

	nextid++;

	return 0;
}

// Takes a string as only argument and returns the metatable
// associated to the userdata type this string refers to
// Returns nil if the string does not refer to a valid userdata type
static int lib_userdataMetatable(lua_State *L)
{
	UINT32 i;
	const char *udname = luaL_checkstring(L, 1);

	// Find internal metatable name
	for (i = 0; meta2utype[i].meta; i++)
		if (!strcmp(udname, meta2utype[i].utype))
		{
			luaL_getmetatable(L, meta2utype[i].meta);
			return 1;
		}

	lua_pushnil(L);
	return 1;
}

static int lib_isPlayerAdmin(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, IsPlayerAdmin(player-players));
	return 1;
}

static int lib_reserveLuabanks(lua_State *L)
{
	static boolean reserved = false;
	if (!lua_lumploading)
		return luaL_error(L, "luabanks[] cannot be reserved from within a hook or coroutine!");
	if (reserved)
		return luaL_error(L, "luabanks[] has already been reserved! Only one savedata-enabled mod at a time may use this feature.");
	reserved = true;
	LUA_PushUserdata(L, &luabanks, META_LUABANKS);
	return 1;
}

static int lib_tofixed(lua_State *L)
{
	const char *arg = luaL_checkstring(L, 1);
	char *end;
	float f = strtof(arg, &end);
	if (*end != '\0')
		lua_pushnil(L);
	else
		lua_pushnumber(L, FLOAT_TO_FIXED(f));
	return 1;
}

// M_MENU
//////////////

static int lib_pMoveColorBefore(lua_State *L)
{
	UINT16 color = (UINT16)luaL_checkinteger(L, 1);
	UINT16 targ = (UINT16)luaL_checkinteger(L, 2);

	NOHUD
	M_MoveColorBefore(color, targ);
	return 0;
}

static int lib_pMoveColorAfter(lua_State *L)
{
	UINT16 color = (UINT16)luaL_checkinteger(L, 1);
	UINT16 targ = (UINT16)luaL_checkinteger(L, 2);

	NOHUD
	M_MoveColorAfter(color, targ);
	return 0;
}

static int lib_pGetColorBefore(lua_State *L)
{
	UINT16 color = (UINT16)luaL_checkinteger(L, 1);
	lua_pushinteger(L, M_GetColorBefore(color));
	return 1;
}

static int lib_pGetColorAfter(lua_State *L)
{
	UINT16 color = (UINT16)luaL_checkinteger(L, 1);
	lua_pushinteger(L, M_GetColorAfter(color));
	return 1;
}

// M_MISC
//////////////

static int lib_mMapNumber(lua_State *L)
{
	const char *arg = luaL_checkstring(L, 1);
	size_t len = strlen(arg);
	if (len == 2 || len == 5) {
		char first = arg[len-2];
		char second = arg[len-1];
		lua_pushinteger(L, M_MapNumber(first, second));
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

// M_RANDOM
//////////////

static int lib_pRandomFixed(lua_State *L)
{
	NOHUD
	lua_pushfixed(L, P_RandomFixed());
	return 1;
}

static int lib_pRandomByte(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_RandomByte());
	return 1;
}

static int lib_pRandomKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);

	NOHUD
	if (a > 65536)
		LUA_UsageWarning(L, "P_RandomKey: range > 65536 is undefined behavior");
	lua_pushinteger(L, P_RandomKey(a));
	return 1;
}

static int lib_pRandomRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);

	NOHUD
	if (b < a) {
		INT32 c = a;
		a = b;
		b = c;
	}
	if ((b-a+1) > 65536)
		LUA_UsageWarning(L, "P_RandomRange: range > 65536 is undefined behavior");
	lua_pushinteger(L, P_RandomRange(a, b));
	return 1;
}

// Macros.
static int lib_pSignedRandom(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_SignedRandom());
	return 1;
}

static int lib_pRandomChance(lua_State *L)
{
	fixed_t p = luaL_checkfixed(L, 1);
	NOHUD
	lua_pushboolean(L, P_RandomChance(p));
	return 1;
}

// P_MAPUTIL
///////////////

static int lib_pAproxDistance(lua_State *L)
{
	fixed_t dx = luaL_checkfixed(L, 1);
	fixed_t dy = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, P_AproxDistance(dx, dy));
	return 1;
}

static int lib_pClosestPointOnLine(lua_State *L)
{
	int n = lua_gettop(L);
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	vertex_t result;
	//HUDSAFE
	if (lua_isuserdata(L, 3)) // use a real linedef to get our points
	{
		line_t *line = *((line_t **)luaL_checkudata(L, 3, META_LINE));
		if (!line)
			return LUA_ErrInvalid(L, "line_t");
		P_ClosestPointOnLine(x, y, line, &result);
	}
	else // use custom coordinates of our own!
	{
		vertex_t v1, v2; // fake vertexes
		line_t junk; // fake linedef

		if (n < 6)
			return luaL_error(L, "arguments 3 to 6 not all given (expected 4 fixed-point integers)");

		v1.x = luaL_checkfixed(L, 3);
		v1.y = luaL_checkfixed(L, 4);
		v2.x = luaL_checkfixed(L, 5);
		v2.y = luaL_checkfixed(L, 6);

		junk.v1 = &v1;
		junk.v2 = &v2;
		junk.dx = v2.x - v1.x;
		junk.dy = v2.y - v1.y;
		P_ClosestPointOnLine(x, y, &junk, &result);
	}

	lua_pushfixed(L, result.x);
	lua_pushfixed(L, result.y);
	return 2;
}

static int lib_pPointOnLineSide(lua_State *L)
{
	int n = lua_gettop(L);
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	if (lua_isuserdata(L, 3)) // use a real linedef to get our points
	{
		line_t *line = *((line_t **)luaL_checkudata(L, 3, META_LINE));
		if (!line)
			return LUA_ErrInvalid(L, "line_t");
		lua_pushinteger(L, P_PointOnLineSide(x, y, line));
	}
	else // use custom coordinates of our own!
	{
		vertex_t v1, v2; // fake vertexes
		line_t junk; // fake linedef

		if (n < 6)
			return luaL_error(L, "arguments 3 to 6 not all given (expected 4 fixed-point integers)");

		v1.x = luaL_checkfixed(L, 3);
		v1.y = luaL_checkfixed(L, 4);
		v2.x = luaL_checkfixed(L, 5);
		v2.y = luaL_checkfixed(L, 6);

		junk.v1 = &v1;
		junk.v2 = &v2;
		junk.dx = v2.x - v1.x;
		junk.dy = v2.y - v1.y;
		lua_pushinteger(L, P_PointOnLineSide(x, y, &junk));
	}
	return 1;
}

// P_ENEMY
/////////////

static int lib_pCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckMeleeRange(actor));
	return 1;
}

static int lib_pJetbCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_JetbCheckMeleeRange(actor));
	return 1;
}

static int lib_pFaceStabCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_FaceStabCheckMeleeRange(actor));
	return 1;
}

static int lib_pSkimCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SkimCheckMeleeRange(actor));
	return 1;
}

static int lib_pCheckMissileRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckMissileRange(actor));
	return 1;
}

static int lib_pNewChaseDir(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_NewChaseDir(actor);
	return 0;
}

static int lib_pLookForPlayers(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t dist = (fixed_t)luaL_optinteger(L, 2, 0);
	boolean allaround = lua_optboolean(L, 3);
	boolean tracer = lua_optboolean(L, 4);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_LookForPlayers(actor, allaround, tracer, dist));
	return 1;
}

// P_MOBJ
////////////

static int lib_pSpawnMobj(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	mobjtype_t type = luaL_checkinteger(L, 4);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	LUA_PushUserdata(L, P_SpawnMobj(x, y, z, type), META_MOBJ);
	return 1;
}

static int lib_pSpawnMobjFromMobj(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	mobjtype_t type = luaL_checkinteger(L, 5);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnMobjFromMobj(actor, x, y, z, type), META_MOBJ);
	return 1;
}

static int lib_pRemoveMobj(lua_State *L)
{
	mobj_t *th = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!th)
		return LUA_ErrInvalid(L, "mobj_t");
	if (th->player)
		return luaL_error(L, "Attempt to remove player mobj with P_RemoveMobj.");
	P_RemoveMobj(th);
	return 0;
}

static int lib_pIsValidSprite2(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	UINT16 spr2 = (UINT16)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, mobj->skin && P_IsValidSprite2(mobj->skin, spr2));
	return 1;
}

// P_SpawnLockOn doesn't exist either, but we want to expose making a local mobj without encouraging hacks.

static int lib_pSpawnLockOn(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *lockon = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	statenum_t state = luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!lockon)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (state >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", state, NUMSTATES-1);
	if (P_IsLocalPlayer(player)) // Only display it on your own view. Don't display it for spectators
	{
		mobj_t *visual = P_SpawnMobj(lockon->x, lockon->y, lockon->z, MT_LOCKON); // positioning, flip handled in P_SceneryThinker
		P_SetTarget(&visual->target, lockon);
		visual->flags2 |= MF2_DONTDRAW;
		visual->drawonlyforplayer = player; // Hide it from the other player in splitscreen, and yourself when spectating
		P_SetMobjStateNF(visual, state);
	}
	return 0;
}

static int lib_pSpawnMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *dest = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnMissile(source, dest, type), META_MOBJ);
	return 1;
}

static int lib_pSpawnXYZMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *dest = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 3);
	fixed_t x = luaL_checkfixed(L, 4);
	fixed_t y = luaL_checkfixed(L, 5);
	fixed_t z = luaL_checkfixed(L, 6);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnXYZMissile(source, dest, type, x, y, z), META_MOBJ);
	return 1;
}

static int lib_pSpawnPointMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t xa = luaL_checkfixed(L, 2);
	fixed_t ya = luaL_checkfixed(L, 3);
	fixed_t za = luaL_checkfixed(L, 4);
	mobjtype_t type = luaL_checkinteger(L, 5);
	fixed_t x = luaL_checkfixed(L, 6);
	fixed_t y = luaL_checkfixed(L, 7);
	fixed_t z = luaL_checkfixed(L, 8);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnPointMissile(source, xa, ya, za, type, x, y, z), META_MOBJ);
	return 1;
}

static int lib_pSpawnAlteredDirectionMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	fixed_t x = luaL_checkfixed(L, 3);
	fixed_t y = luaL_checkfixed(L, 4);
	fixed_t z = luaL_checkfixed(L, 5);
	INT32 shiftingAngle = (INT32)luaL_checkinteger(L, 5);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnAlteredDirectionMissile(source, type, x, y, z, shiftingAngle), META_MOBJ);
	return 1;
}

static int lib_pColorTeamMissile(lua_State *L)
{
	mobj_t *missile = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *source = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	NOHUD
	INLEVEL
	if (!missile)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!source)
		return LUA_ErrInvalid(L, "player_t");
	P_ColorTeamMissile(missile, source);
	return 0;
}

static int lib_pSPMAngle(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	angle_t angle = luaL_checkangle(L, 3);
	UINT8 allowaim = (UINT8)luaL_optinteger(L, 4, 0);
	UINT32 flags2 = (UINT32)luaL_optinteger(L, 5, 0);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SPMAngle(source, type, angle, allowaim, flags2), META_MOBJ);
	return 1;
}

static int lib_pSpawnPlayerMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	UINT32 flags2 = (UINT32)luaL_optinteger(L, 3, 0);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnPlayerMissile(source, type, flags2), META_MOBJ);
	return 1;
}

static int lib_pMobjFlip(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushinteger(L, P_MobjFlip(mobj));
	return 1;
}

static int lib_pGetMobjGravity(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushfixed(L, P_GetMobjGravity(mobj));
	return 1;
}

static int lib_pWeaponOrPanel(lua_State *L)
{
	mobjtype_t type = luaL_checkinteger(L, 1);
	//HUDSAFE
	NOSPAWNNULL
	lua_pushboolean(L, P_WeaponOrPanel(type));
	return 1;
}

static int lib_pFlashPal(lua_State *L)
{
	player_t *pl = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT16 type = (UINT16)luaL_checkinteger(L, 2);
	UINT16 duration = (UINT16)luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!pl)
		return LUA_ErrInvalid(L, "player_t");
	P_FlashPal(pl, type, duration);
	return 0;
}

static int lib_pGetClosestAxis(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_GetClosestAxis(source), META_MOBJ);
	return 1;
}

static int lib_pSpawnParaloop(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	fixed_t radius = luaL_checkfixed(L, 4);
	INT32 number = (INT32)luaL_checkinteger(L, 5);
	mobjtype_t type = luaL_checkinteger(L, 6);
	angle_t rotangle = luaL_checkangle(L, 7);
	statenum_t nstate = luaL_optinteger(L, 8, S_NULL);
	boolean spawncenter = lua_optboolean(L, 9);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (nstate >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", nstate, NUMSTATES-1);
	P_SpawnParaloop(x, y, z, radius, number, type, nstate, rotangle, spawncenter);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

static int lib_pBossTargetPlayer(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	boolean closest = lua_optboolean(L, 2);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_BossTargetPlayer(actor, closest));
	return 1;
}

static int lib_pSupermanLook4Players(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SupermanLook4Players(actor));
	return 1;
}

static int lib_pSetScale(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t newscale = luaL_checkfixed(L, 2);
	boolean instant = lua_optboolean(L, 3);
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (newscale < FRACUNIT/100)
		newscale = FRACUNIT/100;
	P_SetScale(mobj, newscale, instant);
	return 0;
}

static int lib_pInsideANonSolidFFloor(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	lua_pushboolean(L, P_InsideANonSolidFFloor(mobj, rover));
	return 1;
}

static int lib_pCheckDeathPitCollide(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckDeathPitCollide(mo));
	return 1;
}

static int lib_pCheckSolidLava(lua_State *L)
{
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	//HUDSAFE
	INLEVEL
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	lua_pushboolean(L, P_CheckSolidLava(rover));
	return 1;
}

static int lib_pCanRunOnWater(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	lua_pushboolean(L, P_CanRunOnWater(player, rover));
	return 1;
}

static int lib_pMaceRotate(lua_State *L)
{
	mobj_t *center = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	INT32 baserot = luaL_checkinteger(L, 2);
	INT32 baseprevrot = luaL_checkinteger(L, 3);
	NOHUD
	INLEVEL
	if (!center)
		return LUA_ErrInvalid(L, "mobj_t");
	P_MaceRotate(center, baserot, baseprevrot);
	return 0;
}

static int lib_pCreateFloorSpriteSlope(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, (pslope_t *)P_CreateFloorSpriteSlope(mobj), META_SLOPE);
	return 1;
}

static int lib_pRemoveFloorSpriteSlope(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RemoveFloorSpriteSlope(mobj);
	return 1;
}

static int lib_pRailThinker(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_RailThinker(mobj));
	P_SetTarget(&tmthing, ptmthing);
	return 1;
}

static int lib_pCheckSkyHit(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	line_t *line = *((line_t **)luaL_checkudata(L, 2, META_LINE));
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!line)
		return LUA_ErrInvalid(L, "line_t");
	lua_pushboolean(L, P_CheckSkyHit(mobj, line));
	return 1;
}

static int lib_pXYMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_XYMovement(actor);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

static int lib_pRingXYMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RingXYMovement(actor);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

static int lib_pSceneryXYMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SceneryXYMovement(actor);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

static int lib_pZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_ZMovement(actor));
	if (!P_MobjWasRemoved(actor))
		P_CheckPosition(actor, actor->x, actor->y);
	P_SetTarget(&tmthing, ptmthing);
	return 1;
}

static int lib_pRingZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RingZMovement(actor);
	P_CheckPosition(actor, actor->x, actor->y);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

static int lib_pSceneryZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SceneryZMovement(actor));
	if (!P_MobjWasRemoved(actor))
		P_CheckPosition(actor, actor->x, actor->y);
	P_SetTarget(&tmthing, ptmthing);
	return 1;
}

static int lib_pPlayerZMovement(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	P_PlayerZMovement(actor);
	P_CheckPosition(actor, actor->x, actor->y);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

// P_USER
////////////

static int lib_pGetPlayerHeight(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushfixed(L, P_GetPlayerHeight(player));
	return 1;
}

static int lib_pGetPlayerSpinHeight(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushfixed(L, P_GetPlayerSpinHeight(player));
	return 1;
}

static int lib_pGetPlayerControlDirection(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, P_GetPlayerControlDirection(player));
	return 1;
}

static int lib_pAddPlayerScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT32 amount = (UINT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_AddPlayerScore(player, amount);
	return 0;
}

static int lib_pStealPlayerScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT32 amount = (UINT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_StealPlayerScore(player, amount);
	return 0;
}

static int lib_pGetJumpFlags(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, P_GetJumpFlags(player));
	return 1;
}

static int lib_pPlayerInPain(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_PlayerInPain(player));
	return 1;
}

static int lib_pDoPlayerPain(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *source = NULL, *inflictor = NULL;
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		inflictor = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	P_DoPlayerPain(player, source, inflictor);
	return 0;
}

static int lib_pResetPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ResetPlayer(player);
	return 0;
}

static int lib_pPlayerCanDamage(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD // was hud safe but then i added a lua hook
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_PlayerCanDamage(player, thing));
	return 1;
}

static int lib_pPlayerFullbright(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_PlayerFullbright(player));
	return 1;
}


static int lib_pIsObjectInGoop(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_IsObjectInGoop(mo));
	return 1;
}

static int lib_pIsObjectOnGround(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_IsObjectOnGround(mo));
	return 1;
}

static int lib_pInSpaceSector(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_InSpaceSector(mo));
	return 1;
}

static int lib_pInQuicksand(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_InQuicksand(mo));
	return 1;
}

static int lib_pInJumpFlipSector(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_InJumpFlipSector(mo));
	return 1;
}

static int lib_pSetObjectMomZ(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t value = luaL_checkfixed(L, 2);
	boolean relative = lua_optboolean(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SetObjectMomZ(mo, value, relative);
	return 0;
}

static int lib_pIsLocalPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//NOHUD
	//INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_IsLocalPlayer(player));
	return 1;
}

static int lib_pPlayJingle(lua_State *L)
{
	player_t *player = NULL;
	jingletype_t jingletype = luaL_checkinteger(L, 2);
	//NOHUD
	//INLEVEL
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (jingletype >= NUMJINGLES)
		return luaL_error(L, "jingletype %d out of range (0 - %d)", jingletype, NUMJINGLES-1);
	P_PlayJingle(player, jingletype);
	return 0;
}

static int lib_pPlayJingleMusic(lua_State *L)
{
	player_t *player = NULL;
	const char *musnamearg = luaL_checkstring(L, 2);
	char musname[7], *p = musname;
	UINT16 musflags = luaL_optinteger(L, 3, 0);
	boolean looping = lua_opttrueboolean(L, 4);
	jingletype_t jingletype = luaL_optinteger(L, 5, JT_OTHER);
	//NOHUD
	//INLEVEL
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (jingletype >= NUMJINGLES)
		return luaL_error(L, "jingletype %d out of range (0 - %d)", jingletype, NUMJINGLES-1);

	musname[6] = '\0';
	strncpy(musname, musnamearg, 6);

	while (*p) {
		*p = tolower(*p);
		++p;
	}

	P_PlayJingleMusic(player, musname, musflags, looping, jingletype);
	return 0;
}

static int lib_pRestoreMusic(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//NOHUD
	//INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (P_IsLocalPlayer(player))
		P_RestoreMusic(player);
	return 0;
}

static int lib_pSpawnShieldOrb(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_SpawnShieldOrb(player);
	return 0;
}

static int lib_pSpawnGhostMobj(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_SpawnGhostMobj(mobj), META_MOBJ);
	return 1;
}

static int lib_pGivePlayerRings(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 num_rings = (INT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_GivePlayerRings(player, num_rings);
	return 0;
}

static int lib_pGivePlayerSpheres(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 num_spheres = (INT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_GivePlayerSpheres(player, num_spheres);
	return 0;
}

static int lib_pGivePlayerLives(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 numlives = (INT32)luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_GivePlayerLives(player, numlives);
	return 0;
}

static int lib_pGiveCoopLives(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 numlives = (INT32)luaL_checkinteger(L, 2);
	boolean sound = (boolean)lua_opttrueboolean(L, 3);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_GiveCoopLives(player, numlives, sound);
	return 0;
}

static int lib_pResetScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ResetScore(player);
	return 0;
}

static int lib_pDoJumpShield(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoJumpShield(player);
	return 0;
}

static int lib_pDoBubbleBounce(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoBubbleBounce(player);
	return 0;
}

static int lib_pBlackOw(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_BlackOw(player);
	return 0;
}

static int lib_pElementalFire(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean cropcircle = lua_optboolean(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ElementalFire(player, cropcircle);
	return 0;
}

static int lib_pSpawnSkidDust(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	fixed_t radius = luaL_checkfixed(L, 2);
	boolean sound = lua_optboolean(L, 3);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_SpawnSkidDust(player, radius, sound);
	return 0;
}

static int lib_pMovePlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *ptmthing = tmthing;
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_MovePlayer(player);
	P_SetTarget(&tmthing, ptmthing);
	return 0;
}

static int lib_pDoPlayerFinish(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoPlayerFinish(player);
	return 0;
}

static int lib_pDoPlayerExit(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean finishedflag = lua_opttrueboolean(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoPlayerExit(player, finishedflag);
	return 0;
}

static int lib_pInstaThrust(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = luaL_checkangle(L, 2);
	fixed_t move = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_InstaThrust(mo, angle, move);
	return 0;
}

static int lib_pInstaThrustEvenIn2D(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = luaL_checkangle(L, 2);
	fixed_t move = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_InstaThrustEvenIn2D(mo, angle, move);
	return 0;
}

static int lib_pReturnThrustX(lua_State *L)
{
	angle_t angle;
	fixed_t move;
	if (lua_isnil(L, 1) || lua_isuserdata(L, 1))
		lua_remove(L, 1); // ignore mobj as arg1
	angle = luaL_checkangle(L, 1);
	move = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, P_ReturnThrustX(NULL, angle, move));
	return 1;
}

static int lib_pReturnThrustY(lua_State *L)
{
	angle_t angle;
	fixed_t move;
	if (lua_isnil(L, 1) || lua_isuserdata(L, 1))
		lua_remove(L, 1); // ignore mobj as arg1
	angle = luaL_checkangle(L, 1);
	move = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, P_ReturnThrustY(NULL, angle, move));
	return 1;
}

static int lib_pLookForEnemies(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean nonenemies = lua_opttrueboolean(L, 2);
	boolean bullet = lua_optboolean(L, 3);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, P_LookForEnemies(player, nonenemies, bullet), META_MOBJ);
	return 1;
}

static int lib_pNukeEnemies(lua_State *L)
{
	mobj_t *inflictor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t radius = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!inflictor || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_NukeEnemies(inflictor, source, radius);
	return 0;
}

static int lib_pEarthquake(lua_State *L)
{
	mobj_t *inflictor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t radius = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!inflictor || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_Earthquake(inflictor, source, radius);
	return 0;
}

static int lib_pHomingAttack(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *enemy = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!source || !enemy)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_HomingAttack(source, enemy));
	return 1;
}

static int lib_pResetCamera(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 2, META_CAMERA));

	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!cam)
		return LUA_ErrInvalid(L, "camera_t");
	P_ResetCamera(player, cam);
	return 0;
}

static int lib_pSuperReady(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean transform = (boolean)lua_opttrueboolean(L, 2);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_SuperReady(player, transform));
	return 1;
}

static int lib_pDoJump(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean soundandstate = (boolean)lua_opttrueboolean(L, 2);
	boolean allowflip = (boolean)lua_opttrueboolean(L, 3);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoJump(player, soundandstate, allowflip);
	return 0;
}

static int lib_pDoSpinDashDust(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoSpinDashDust(player);
	return 0;
}

static int lib_pSpawnThokMobj(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_SpawnThokMobj(player);
	return 0;
}

static int lib_pSpawnSpinMobj(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobjtype_t type = luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	NOSPAWNNULL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_SpawnSpinMobj(player, type);
	return 0;
}

static int lib_pTelekinesis(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	fixed_t thrust = luaL_checkfixed(L, 2);
	fixed_t range = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_Telekinesis(player, thrust, range);
	return 0;
}

static int lib_pSwitchShield(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT16 shield = luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_SwitchShield(player, shield);
	return 0;
}

static int lib_pDoTailsOverlay(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *tails = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!tails)
		return LUA_ErrInvalid(L, "mobj_t");
	P_DoTailsOverlay(player, tails);
	return 0;
}

static int lib_pDoMetalJetFume(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *fume = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!fume)
		return LUA_ErrInvalid(L, "mobj_t");
	P_DoMetalJetFume(player, fume);
	return 0;
}

static int lib_pDoFollowMobj(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	mobj_t *followmobj = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!followmobj)
		return LUA_ErrInvalid(L, "mobj_t");
	P_DoFollowMobj(player, followmobj);
	return 0;
}

static int lib_pPlayerCanEnterSpinGaps(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_PlayerCanEnterSpinGaps(player));
	return 1;
}

static int lib_pPlayerShouldUseSpinHeight(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_PlayerShouldUseSpinHeight(player));
	return 1;
}

// P_MAP
///////////

static int lib_pCheckPosition(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckPosition(thing, x, y));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
}

static int lib_pTryMove(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	boolean allowdropoff = lua_optboolean(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_TryMove(thing, x, y, allowdropoff));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
}

static int lib_pMove(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t speed = luaL_checkfixed(L, 2);
	NOHUD
	INLEVEL
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_Move(actor, speed));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
}

// TODO: 2.3: Delete
static int lib_pTeleportMove(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_Deprecated(L, "P_TeleportMove", "P_SetOrigin\" or \"P_MoveOrigin");
	lua_pushboolean(L, P_MoveOrigin(thing, x, y, z));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
}

static int lib_pSetOrigin(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SetOrigin(thing, x, y, z));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
}

static int lib_pMoveOrigin(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	NOHUD
	INLEVEL
	if (!thing)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_MoveOrigin(thing, x, y, z));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
}

static int lib_pLineIsBlocking(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	line_t *line = *((line_t **)luaL_checkudata(L, 2, META_LINE));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!line)
		return LUA_ErrInvalid(L, "line_t");
	
	// P_LineOpening in P_LineIsBlocking sets these variables.
	// We want to keep their old values after so that whatever
	// map collision code uses them doesn't get messed up.
	fixed_t oldopentop = opentop;
	fixed_t oldopenbottom = openbottom;
	fixed_t oldopenrange = openrange;
	fixed_t oldlowfloor = lowfloor;
	fixed_t oldhighceiling = highceiling;
	pslope_t *oldopentopslope = opentopslope;
	pslope_t *oldopenbottomslope = openbottomslope;
	ffloor_t *oldopenfloorrover = openfloorrover;
	ffloor_t *oldopenceilingrover = openceilingrover;
	
	lua_pushboolean(L, P_LineIsBlocking(mo, line));
	
	opentop = oldopentop;
	openbottom = oldopenbottom;
	openrange = oldopenrange;
	lowfloor = oldlowfloor;
	highceiling = oldhighceiling;
	opentopslope = oldopentopslope;
	openbottomslope = oldopenbottomslope;
	openfloorrover = oldopenfloorrover;
	openceilingrover = oldopenceilingrover;
	
	return 1;
}

static int lib_pSlideMove(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SlideMove(mo);
	return 0;
}

static int lib_pBounceMove(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_BounceMove(mo);
	return 0;
}

static int lib_pCheckSight(lua_State *L)
{
	mobj_t *t1 = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *t2 = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	//HUDSAFE?
	INLEVEL
	if (!t1 || !t2)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckSight(t1, t2));
	return 1;
}

static int lib_pCheckHoopPosition(lua_State *L)
{
	mobj_t *hoopthing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	fixed_t radius = luaL_checkfixed(L, 5);
	NOHUD
	INLEVEL
	if (!hoopthing)
		return LUA_ErrInvalid(L, "mobj_t");
	P_CheckHoopPosition(hoopthing, x, y, z, radius);
	return 0;
}

static int lib_pRadiusAttack(lua_State *L)
{
	mobj_t *spot = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t damagedist = luaL_checkfixed(L, 3);
	UINT8 damagetype = luaL_optinteger(L, 4, 0);
	boolean sightcheck = lua_opttrueboolean(L, 5);
	NOHUD
	INLEVEL
	if (!spot || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RadiusAttack(spot, source, damagedist, damagetype, sightcheck);
	return 0;
}

static int lib_pFloorzAtPos(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	fixed_t height = luaL_checkfixed(L, 4);
	//HUDSAFE
	INLEVEL
	lua_pushfixed(L, P_FloorzAtPos(x, y, z, height));
	return 1;
}

static int lib_pCeilingzAtPos(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);
	fixed_t height = luaL_checkfixed(L, 4);
	//HUDSAFE
	INLEVEL
	lua_pushfixed(L, P_CeilingzAtPos(x, y, z, height));
	return 1;
}

static int lib_pGetSectorLightLevelAt(lua_State *L)
{
	boolean has_sector = false;
	sector_t *sector = NULL;
	if (!lua_isnoneornil(L, 1))
	{
		has_sector = true;
		sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	}
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	INLEVEL
	if (has_sector && !sector)
		return LUA_ErrInvalid(L, "sector_t");
	if (sector)
		lua_pushinteger(L, P_GetLightLevelFromSectorAt(sector, x, y, z));
	else
		lua_pushinteger(L, P_GetSectorLightLevelAt(x, y, z));
	return 1;
}

static int lib_pGetSectorColormapAt(lua_State *L)
{
	boolean has_sector = false;
	sector_t *sector = NULL;
	if (!lua_isnoneornil(L, 1))
	{
		has_sector = true;
		sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	}
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);
	INLEVEL
	if (has_sector && !sector)
		return LUA_ErrInvalid(L, "sector_t");
	extracolormap_t *exc;
	if (sector)
		exc = P_GetColormapFromSectorAt(sector, x, y, z);
	else
		exc = P_GetSectorColormapAt(x, y, z);
	LUA_PushUserdata(L, exc, META_EXTRACOLORMAP);
	return 1;
}

static int lib_pDoSpring(lua_State *L)
{
	mobj_t *spring = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *object = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!spring || !object)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_DoSpring(spring, object));
	return 1;
}

static int lib_pTryCameraMove(lua_State *L)
{
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 1, META_CAMERA));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);

	if (!cam)
		return LUA_ErrInvalid(L, "camera_t");
	lua_pushboolean(L, P_TryCameraMove(x, y, cam));
	return 1;
}

static int lib_pTeleportCameraMove(lua_State *L)
{
	camera_t *cam = *((camera_t **)luaL_checkudata(L, 1, META_CAMERA));
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);

	if (!cam)
		return LUA_ErrInvalid(L, "camera_t");
	cam->x = x;
	cam->y = y;
	cam->z = z;
	P_CheckCameraPosition(x, y, cam);
	cam->subsector = R_PointInSubsector(x, y);
	cam->floorz = tmfloorz;
	cam->ceilingz = tmceilingz;
	return 0;
}

// P_INTER
////////////

static int lib_pRemoveShield(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_RemoveShield(player);
	return 0;
}

static int lib_pDamageMobj(lua_State *L)
{
	mobj_t *target = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)), *inflictor = NULL, *source = NULL;
	INT32 damage;
	UINT8 damagetype;
	NOHUD
	INLEVEL
	if (!target)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	damage = (INT32)luaL_optinteger(L, 4, 1);
	damagetype = (UINT8)luaL_optinteger(L, 5, 0);
	lua_pushboolean(L, P_DamageMobj(target, inflictor, source, damage, damagetype));
	return 1;
}

static int lib_pKillMobj(lua_State *L)
{
	mobj_t *target = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)), *inflictor = NULL, *source = NULL;
	UINT8 damagetype;
	NOHUD
	INLEVEL
	if (!target)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	damagetype = (UINT8)luaL_optinteger(L, 4, 0);
	P_KillMobj(target, inflictor, source, damagetype);
	return 0;
}

static int lib_pPlayerRingBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 num_rings = (INT32)luaL_optinteger(L, 2, -1);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (num_rings == -1)
		num_rings = player->rings;
	P_PlayerRingBurst(player, num_rings);
	return 0;
}

static int lib_pPlayerWeaponPanelBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerWeaponPanelBurst(player);
	return 0;
}

static int lib_pPlayerWeaponAmmoBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerWeaponAmmoBurst(player);
	return 0;
}

static int lib_pPlayerWeaponPanelOrAmmoBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerWeaponPanelOrAmmoBurst(player);
	return 0;
}

static int lib_pPlayerEmeraldBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean toss = lua_optboolean(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerEmeraldBurst(player, toss);
	return 0;
}

static int lib_pPlayerFlagBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean toss = lua_optboolean(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerFlagBurst(player, toss);
	return 0;
}

static int lib_pPlayRinglossSound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *player = NULL;
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		P_PlayRinglossSound(source);
	return 0;
}

static int lib_pPlayDeathSound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *player = NULL;
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		P_PlayDeathSound(source);
	return 0;
}

static int lib_pPlayVictorySound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *player = NULL;
	NOHUD
	INLEVEL
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		P_PlayVictorySound(source);
	return 0;
}

static int lib_pPlayLivesJingle(lua_State *L)
{
	player_t *player = NULL;
	//NOHUD
	//INLEVEL
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	P_PlayLivesJingle(player);
	return 0;
}

static int lib_pCanPickupItem(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean weapon = lua_optboolean(L, 2);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_CanPickupItem(player, weapon));
	return 1;
}

static int lib_pDoNightsScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoNightsScore(player);
	return 0;
}

static int lib_pDoMatchSuper(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoMatchSuper(player);
	return 0;
}

static int lib_pTouchSpecialThing(lua_State *L)
{
	mobj_t *special = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *toucher = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	boolean heightcheck = lua_optboolean(L, 3);
	NOHUD
	INLEVEL
	if (!special || !toucher)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!toucher->player)
		return luaL_error(L, "P_TouchSpecialThing requires a valid toucher.player.");
	P_TouchSpecialThing(special, toucher, heightcheck);
	return 0;
}

// P_SPEC
////////////

static int lib_pThrust(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = luaL_checkangle(L, 2);
	fixed_t move = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_Thrust(mo, angle, move);
	return 0;
}

static int lib_pThrustEvenIn2D(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = luaL_checkangle(L, 2);
	fixed_t move = luaL_checkfixed(L, 3);
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_ThrustEvenIn2D(mo, angle, move);
	return 0;
}

static int lib_pVectorInstaThrust(lua_State *L)
{
    fixed_t xa = luaL_checkfixed(L, 1);
    fixed_t xb = luaL_checkfixed(L, 2);
    fixed_t xc = luaL_checkfixed(L, 3);
    fixed_t ya = luaL_checkfixed(L, 4);
    fixed_t yb = luaL_checkfixed(L, 5);
    fixed_t yc = luaL_checkfixed(L, 6);
    fixed_t za = luaL_checkfixed(L, 7);
    fixed_t zb = luaL_checkfixed(L, 8);
    fixed_t zc = luaL_checkfixed(L, 9);
    fixed_t momentum = luaL_checkfixed(L, 10);
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 11, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_VectorInstaThrust(xa, xb, xc, ya, yb, yc, za, zb, zc, momentum, mo);
	return 0;
}

static int lib_pSetMobjStateNF(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	statenum_t state = luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (state >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", state, NUMSTATES-1);
	if (mobj->player && state == S_NULL)
		return luaL_error(L, "Attempt to remove player mobj with S_NULL.");
	lua_pushboolean(L, P_SetMobjStateNF(mobj, state));
	return 1;
}

static int lib_pDoSuperTransformation(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean giverings = lua_optboolean(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoSuperTransformation(player, giverings);
	return 0;
}

static int lib_pDoSuperDetransformation(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoSuperDetransformation(player);
	return 0;
}

static int lib_pExplodeMissile(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_ExplodeMissile(mo);
	return 0;
}

static int lib_pMobjTouchingSectorSpecial(lua_State *L)
{
	mobj_t *mo = *((mobj_t**)luaL_checkudata(L, 1, META_MOBJ));
	INT32 section = (INT32)luaL_checkinteger(L, 2);
	INT32 number = (INT32)luaL_checkinteger(L, 3);
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_MobjTouchingSectorSpecial(mo, section, number), META_SECTOR);
	return 1;
}

// TODO: 2.3: Delete
static int lib_pThingOnSpecial3DFloor(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_Deprecated(L, "P_ThingOnSpecial3DFloor", "P_MobjTouchingSectorSpecial\" or \"P_MobjTouchingSectorSpecialFlag");
	LUA_PushUserdata(L, P_ThingOnSpecial3DFloor(mo), META_SECTOR);
	return 1;
}

static int lib_pMobjTouchingSectorSpecialFlag(lua_State *L)
{
	mobj_t *mo = *((mobj_t**)luaL_checkudata(L, 1, META_MOBJ));
	sectorspecialflags_t flag = (INT32)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_MobjTouchingSectorSpecialFlag(mo, flag), META_SECTOR);
	return 1;
}

static int lib_pPlayerTouchingSectorSpecial(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 section = (INT32)luaL_checkinteger(L, 2);
	INT32 number = (INT32)luaL_checkinteger(L, 3);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, P_PlayerTouchingSectorSpecial(player, section, number), META_SECTOR);
	return 1;
}

static int lib_pPlayerTouchingSectorSpecialFlag(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	sectorspecialflags_t flag = (INT32)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, P_PlayerTouchingSectorSpecialFlag(player, flag), META_SECTOR);
	return 1;
}

static int lib_pFindLowestFloorSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindLowestFloorSurrounding(sector));
	return 1;
}

static int lib_pFindHighestFloorSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindHighestFloorSurrounding(sector));
	return 1;
}

static int lib_pFindNextHighestFloor(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	fixed_t currentheight;
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	// defaults to floorheight of sector arg
	currentheight = (fixed_t)luaL_optinteger(L, 2, sector->floorheight);
	lua_pushfixed(L, P_FindNextHighestFloor(sector, currentheight));
	return 1;
}

static int lib_pFindNextLowestFloor(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	fixed_t currentheight;
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	// defaults to floorheight of sector arg
	currentheight = (fixed_t)luaL_optinteger(L, 2, sector->floorheight);
	lua_pushfixed(L, P_FindNextLowestFloor(sector, currentheight));
	return 1;
}

static int lib_pFindLowestCeilingSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindLowestCeilingSurrounding(sector));
	return 1;
}

static int lib_pFindHighestCeilingSurrounding(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	//HUDSAFE
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	lua_pushfixed(L, P_FindHighestCeilingSurrounding(sector));
	return 1;
}

static int lib_pFindSpecialLineFromTag(lua_State *L)
{
	INT16 special = (INT16)luaL_checkinteger(L, 1);
	INT16 line = (INT16)luaL_checkinteger(L, 2);
	INT32 start = (INT32)luaL_optinteger(L, 3, -1);
	NOHUD
	INLEVEL
	lua_pushinteger(L, P_FindSpecialLineFromTag(special, line, start));
	return 1;
}

static int lib_pSwitchWeather(lua_State *L)
{
	INT32 weathernum = (INT32)luaL_checkinteger(L, 1);
	player_t *user = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2)) // if a player, setup weather for only the player, otherwise setup weather for all players
		user = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	if (!user) // global
		globalweather = weathernum;
	if (!user || P_IsLocalPlayer(user))
		P_SwitchWeather(weathernum);
	return 0;
}

static int lib_pLinedefExecute(lua_State *L)
{
	INT32 tag = (INT16)luaL_checkinteger(L, 1);
	mobj_t *actor = NULL;
	sector_t *caller = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
		actor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
		caller = *((sector_t **)luaL_checkudata(L, 3, META_SECTOR));
	P_LinedefExecute(tag, actor, caller);
	return 0;
}

static int lib_pSpawnLightningFlash(lua_State *L)
{
	sector_t *sector = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	NOHUD
	INLEVEL
	if (!sector)
		return LUA_ErrInvalid(L, "sector_t");
	P_SpawnLightningFlash(sector);
	return 0;
}

static int lib_pFadeLight(lua_State *L)
{
	INT16 tag = (INT16)luaL_checkinteger(L, 1);
	INT32 destvalue = (INT32)luaL_checkinteger(L, 2);
	INT32 speed = (INT32)luaL_checkinteger(L, 3);
	boolean ticbased = lua_optboolean(L, 4);
	boolean force = lua_optboolean(L, 5);
	boolean relative = lua_optboolean(L, 6);
	NOHUD
	INLEVEL
	P_FadeLight(tag, destvalue, speed, ticbased, force, relative);
	return 0;
}

static int lib_pIsFlagAtBase(lua_State *L)
{
	mobjtype_t type = luaL_checkinteger(L, 1);
	//HUDSAFE
	INLEVEL
	NOSPAWNNULL
	lua_pushboolean(L, P_IsFlagAtBase(type));
	return 1;
}

static int lib_pSetupLevelSky(lua_State *L)
{
	INT32 skynum = (INT32)luaL_checkinteger(L, 1);
	player_t *user = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2)) // if a player, setup sky for only the player, otherwise setup sky for all players
		user = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	if (!user) // global
		P_SetupLevelSky(skynum, true);
	else if (P_IsLocalPlayer(user))
		P_SetupLevelSky(skynum, false);
	return 0;
}

// Shhh, P_SetSkyboxMobj doesn't actually exist yet.
static int lib_pSetSkyboxMobj(lua_State *L)
{
	int n = lua_gettop(L);
	mobj_t *mo = NULL;
	player_t *user = NULL;
	int w = 0;

	NOHUD
	INLEVEL
	if (!lua_isnil(L,1)) // nil leaves mo as NULL to remove the skybox rendering.
	{
		mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)); // otherwise it is a skybox mobj.
		if (!mo)
			return LUA_ErrInvalid(L, "mobj_t");
	}

	if (n == 1)
		;
	else if (lua_isuserdata(L, 2))
		user = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	else if (lua_isnil(L, 2))
		w = 0;
	else if (lua_isboolean(L, 2))
	{
		if (lua_toboolean(L, 2))
			w = 1;
		else
			w = 0;
	}
	else
		w = luaL_optinteger(L, 2, 0);

	if (n > 2 && lua_isuserdata(L, 3))
	{
		user = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!user)
			return LUA_ErrInvalid(L, "player_t");
	}

	if (w > 1 || w < 0)
		return luaL_error(L, "skybox mobj index %d is out of range for P_SetSkyboxMobj argument #2 (expected 0 or 1)", w);

	if (!user || P_IsLocalPlayer(user))
		skyboxmo[w] = mo;
	return 0;
}

// Shhh, neither does P_StartQuake.
static int lib_pStartQuake(lua_State *L)
{
	fixed_t q_intensity = luaL_checkinteger(L, 1);
	UINT16  q_time = (UINT16)luaL_checkinteger(L, 2);
	static mappoint_t q_epicenter = {0,0,0};

	NOHUD
	INLEVEL

	// While technically we don't support epicenter and radius,
	// we get their values anyway if they exist.
	// This way when support is added we won't have to change anything.
	if (!lua_isnoneornil(L, 3))
	{
		luaL_checktype(L, 3, LUA_TTABLE);

		lua_getfield(L, 3, "x");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 1);
		}
		if (!lua_isnil(L, -1))
			q_epicenter.x = luaL_checkinteger(L, -1);
		else
			q_epicenter.x = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "y");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 2);
		}
		if (!lua_isnil(L, -1))
			q_epicenter.y = luaL_checkinteger(L, -1);
		else
			q_epicenter.y = 0;
		lua_pop(L, 1);

		lua_getfield(L, 3, "z");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_rawgeti(L, 3, 3);
		}
		if (!lua_isnil(L, -1))
			q_epicenter.z = luaL_checkinteger(L, -1);
		else
			q_epicenter.z = 0;
		lua_pop(L, 1);

		quake.epicenter = &q_epicenter;
	}
	else
		quake.epicenter = NULL;
	quake.radius = luaL_optinteger(L, 4, 512*FRACUNIT);

	// These things are actually used in 2.1.
	quake.intensity = q_intensity;
	quake.time = q_time;
	return 0;
}

static int lib_evCrumbleChain(lua_State *L)
{
	sector_t *sec = NULL;
	ffloor_t *rover = NULL;
	NOHUD
	INLEVEL
	if (!lua_isnone(L, 2))
	{
		if (!lua_isnil(L, 1))
		{
			sec = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
			if (!sec)
				return LUA_ErrInvalid(L, "sector_t");
		}
		rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	}
	else
		rover = *((ffloor_t **)luaL_checkudata(L, 1, META_FFLOOR));
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	EV_CrumbleChain(sec, rover);
	return 0;
}

static int lib_evStartCrumble(lua_State *L)
{
	sector_t *sec = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	boolean floating = lua_optboolean(L, 3);
	player_t *player = NULL;
	fixed_t origalpha;
	boolean crumblereturn = lua_optboolean(L, 6);
	NOHUD
	if (!sec)
		return LUA_ErrInvalid(L, "sector_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	if (!lua_isnone(L, 4) && lua_isuserdata(L, 4))
	{
		player = *((player_t **)luaL_checkudata(L, 4, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!lua_isnone(L,5))
		origalpha = luaL_checkfixed(L, 5);
	else
		origalpha = rover->alpha;
	lua_pushboolean(L, EV_StartCrumble(sec, rover, floating, player, origalpha, crumblereturn) != 0);
	return 0;
}

// P_SLOPES
////////////

static int lib_pGetZAt(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	//HUDSAFE
	if (lua_isnil(L, 1))
	{
		fixed_t z = luaL_checkfixed(L, 4);
		lua_pushfixed(L, P_GetZAt(NULL, x, y, z));
	}
	else
	{
		pslope_t *slope = *((pslope_t **)luaL_checkudata(L, 1, META_SLOPE));
		lua_pushfixed(L, P_GetSlopeZAt(slope, x, y));
	}

	return 1;
}

static int lib_pButteredSlope(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	P_ButteredSlope(mobj);
	return 0;
}

// R_DEFS
////////////

static int lib_rPointToAngle(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushangle(L, R_PointToAngle(x, y));
	return 1;
}

static int lib_rPointToAngle2(lua_State *L)
{
	fixed_t px2 = luaL_checkfixed(L, 1);
	fixed_t py2 = luaL_checkfixed(L, 2);
	fixed_t px1 = luaL_checkfixed(L, 3);
	fixed_t py1 = luaL_checkfixed(L, 4);
	//HUDSAFE
	lua_pushangle(L, R_PointToAngle2(px2, py2, px1, py1));
	return 1;
}

static int lib_rPointToDist(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	lua_pushfixed(L, R_PointToDist(x, y));
	return 1;
}

static int lib_rPointToDist2(lua_State *L)
{
	fixed_t px2 = luaL_checkfixed(L, 1);
	fixed_t py2 = luaL_checkfixed(L, 2);
	fixed_t px1 = luaL_checkfixed(L, 3);
	fixed_t py1 = luaL_checkfixed(L, 4);
	//HUDSAFE
	lua_pushfixed(L, R_PointToDist2(px2, py2, px1, py1));
	return 1;
}

static int lib_rPointInSubsector(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	//HUDSAFE
	INLEVEL
	LUA_PushUserdata(L, R_PointInSubsector(x, y), META_SUBSECTOR);
	return 1;
}

static int lib_rPointInSubsectorOrNil(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	subsector_t *sub = R_PointInSubsectorOrNull(x, y);
	//HUDSAFE
	INLEVEL
	if (sub)
		LUA_PushUserdata(L, sub, META_SUBSECTOR);
	else
		lua_pushnil(L);
	return 1;
}

// R_THINGS
////////////

static int lib_rChar2Frame(lua_State *L)
{
	const char *p = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_Char2Frame(*p)); // first character only
	return 1;
}

static int lib_rFrame2Char(lua_State *L)
{
	UINT8 ch = (UINT8)luaL_checkinteger(L, 1);
	char c[2] = "";
	//HUDSAFE

	c[0] = R_Frame2Char(ch);
	if (c[0] == '\xFF')
		return luaL_error(L, "frame %u cannot be represented by a character", ch);

	c[1] = 0;

	lua_pushstring(L, c);
	lua_pushinteger(L, c[0]);
	return 2;
}

// R_SKINS
////////////

// R_SetPlayerSkin technically doesn't exist either, although it's basically just SetPlayerSkin and SetPlayerSkinByNum handled in one place for convenience
static int lib_rSetPlayerSkin(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 i = -1, j = -1;
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");

	j = (player-players);

	if (lua_isnoneornil(L, 2))
		return luaL_error(L, "argument #2 not given (expected number or string)");
	else if (lua_type(L, 2) == LUA_TNUMBER) // skin number
	{
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= numskins)
			return luaL_error(L, "skin %d (argument #2) out of range (0 - %d)", i, numskins-1);
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 2);
		i = R_SkinAvailable(skinname);
		if (i == -1)
			return luaL_error(L, "skin %s (argument 2) is not loaded", skinname);
	}

	if (!R_SkinUsable(j, i))
		return luaL_error(L, "skin %d (argument 2) not usable - check with R_SkinUsable(player_t, skin) first.", i);
	SetPlayerSkinByNum(j, i);
	return 0;
}

static int lib_rSkinUsable(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 i = -1, j = -1;
	if (player)
		j = (player-players);
	else if (netgame || multiplayer)
		return luaL_error(L, "player_t (argument #1) must be provided in multiplayer games");
	if (lua_isnoneornil(L, 2))
		return luaL_error(L, "argument #2 not given (expected number or string)");
	else if (lua_type(L, 2) == LUA_TNUMBER) // skin number
	{
		i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= numskins)
			return luaL_error(L, "skin %d (argument #2) out of range (0 - %d)", i, numskins-1);
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 2);
		i = R_SkinAvailable(skinname);
		if (i == -1)
			return luaL_error(L, "skin %s (argument 2) is not loaded", skinname);
	}

	lua_pushboolean(L, R_SkinUsable(j, i));
	return 1;
}

static int lib_pGetStateSprite2(lua_State *L)
{
	int statenum = luaL_checkinteger(L, 1);
	if (statenum < 0 || statenum >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", statenum, NUMSTATES-1);

	lua_pushinteger(L, P_GetStateSprite2(&states[statenum]));
	return 1;
}

static int lib_pGetSprite2StateFrame(lua_State *L)
{
	int statenum = luaL_checkinteger(L, 1);
	if (statenum < 0 || statenum >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", statenum, NUMSTATES-1);

	lua_pushinteger(L, P_GetSprite2StateFrame(&states[statenum]));
	return 1;
}

static int lib_pIsStateSprite2Super(lua_State *L)
{
	int statenum = luaL_checkinteger(L, 1);
	if (statenum < 0 || statenum >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", statenum, NUMSTATES-1);

	lua_pushboolean(L, P_IsStateSprite2Super(&states[statenum]));
	return 1;
}

// Not a real function. Who cares? I know I don't.
static int lib_pGetSuperSprite2(lua_State *L)
{
	int animID = luaL_checkinteger(L, 1) & SPR2F_MASK;
	if (animID < 0 || animID >= NUMPLAYERSPRITES)
		return luaL_error(L, "sprite2 %d out of range (0 - %d)", animID, NUMPLAYERSPRITES-1);

	lua_pushinteger(L, animID | SPR2F_SUPER);
	return 1;
}

// R_DATA
////////////

static int lib_rCheckTextureNumForName(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_CheckTextureNumForName(name));
	return 1;
}

static int lib_rTextureNumForName(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_TextureNumForName(name));
	return 1;
}

static int lib_rCheckTextureNameForNum(lua_State *L)
{
	INT32 num = (INT32)luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushstring(L, R_CheckTextureNameForNum(num));
	return 1;
}

static int lib_rTextureNameForNum(lua_State *L)
{
	INT32 num = (INT32)luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushstring(L, R_TextureNameForNum(num));
	return 1;
}

// R_DRAW
////////////
static int lib_rGetColorByName(lua_State *L)
{
	const char* colorname = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_GetColorByName(colorname));
	return 1;
}

static int lib_rGetSuperColorByName(lua_State *L)
{
	const char* colorname = luaL_checkstring(L, 1);
	//HUDSAFE
	lua_pushinteger(L, R_GetSuperColorByName(colorname));
	return 1;
}

// Lua exclusive function, returns the name of a color from the SKINCOLOR_ constant.
// SKINCOLOR_GREEN > "Green" for example
static int lib_rGetNameByColor(lua_State *L)
{
	UINT16 colornum = (UINT16)luaL_checkinteger(L, 1);
	if (!colornum || colornum >= numskincolors)
		return luaL_error(L, "skincolor %d out of range (1 - %d).", colornum, numskincolors-1);
	lua_pushstring(L, skincolors[colornum].name);
	return 1;
}

// S_SOUND
////////////
static int GetValidSoundOrigin(lua_State *L, void **origin)
{
	const char *type;

	lua_settop(L, 1);
	type = GetUserdataUType(L);

	if (fasticmp(type, "mobj_t"))
	{
		*origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (!(*origin))
			return LUA_ErrInvalid(L, "mobj_t");
		return 1;
	}
	else if (fasticmp(type, "sector_t"))
	{
		*origin = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
		if (!(*origin))
			return LUA_ErrInvalid(L, "sector_t");

		*origin = &((sector_t *)(*origin))->soundorg;
		return 1;
	}

	return LUA_ErrInvalid(L, "mobj_t/sector_t");
}

static int lib_sStartSound(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	player_t *player = NULL;
	//NOHUD

	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);

	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return 0;
	if (!player || P_IsLocalPlayer(player))
	{
		if (hud_running || hook_cmd_running)
			origin = NULL;	// HUD rendering and CMD building startsound shouldn't have an origin, just remove it instead of having a retarded error.

		S_StartSound(origin, sound_id);
	}
	return 0;
}

static int lib_sStartSoundAtVolume(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	INT32 volume = (INT32)luaL_checkinteger(L, 3);
	player_t *player = NULL;
	//NOHUD

	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);
	if (!lua_isnone(L, 4) && lua_isuserdata(L, 4))
	{
		player = *((player_t **)luaL_checkudata(L, 4, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return LUA_ErrInvalid(L, "mobj_t/sector_t");

	if (!player || P_IsLocalPlayer(player))
		S_StartSoundAtVolume(origin, sound_id, volume);
	return 0;
}

static int lib_sStopSound(lua_State *L)
{
	void *origin = NULL;
	//NOHUD
	if (!GetValidSoundOrigin(L, &origin))
		return LUA_ErrInvalid(L, "mobj_t/sector_t");

	S_StopSound(origin);
	return 0;
}

static int lib_sStopSoundByID(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	//NOHUD

	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);
	if (!lua_isnil(L, 1))
		if (!GetValidSoundOrigin(L, &origin))
			return LUA_ErrInvalid(L, "mobj_t/sector_t");

	S_StopSoundByID(origin, sound_id);
	return 0;
}

static int lib_sChangeMusic(lua_State *L)
{
	UINT32 position, prefadems, fadeinms;

	const char *music_name = luaL_checkstring(L, 1);
	boolean looping = (boolean)lua_opttrueboolean(L, 2);
	player_t *player = NULL;
	UINT16 music_flags = 0;

	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}

	music_flags = (UINT16)luaL_optinteger(L, 4, 0);
	position = (UINT32)luaL_optinteger(L, 5, 0);
	prefadems = (UINT32)luaL_optinteger(L, 6, 0);
	fadeinms = (UINT32)luaL_optinteger(L, 7, 0);

	if (!player || P_IsLocalPlayer(player))
		S_ChangeMusicEx(music_name, music_flags, looping, position, prefadems, fadeinms);
	return 0;
}

static int lib_sSpeedMusic(lua_State *L)
{
	fixed_t fixedspeed = luaL_checkfixed(L, 1);
	float speed = FIXED_TO_FLOAT(fixedspeed);
	player_t *player = NULL;
	//NOHUD
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		S_SpeedMusic(speed);
	return 0;
}

static int lib_sStopMusic(lua_State *L)
{
	player_t *player = NULL;
	//NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		S_StopMusic();
	return 0;
}

static int lib_sSetInternalMusicVolume(lua_State *L)
{
	UINT32 volume = (UINT32)luaL_checkinteger(L, 1);
	player_t *player = NULL;
	//NOHUD
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
	{
		S_SetInternalMusicVolume(volume);
		lua_pushboolean(L, true);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sStopFadingMusic(lua_State *L)
{
	player_t *player = NULL;
	//NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
	{
		S_StopFadingMusic();
		lua_pushboolean(L, true);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sFadeMusic(lua_State *L)
{
	UINT32 target_volume = (UINT32)luaL_checkinteger(L, 1);
	UINT32 ms;
	INT32 source_volume;
	player_t *player = NULL;
	//NOHUD
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
		ms = (UINT32)luaL_checkinteger(L, 2);
		source_volume = -1;
	}
	else if (!lua_isnone(L, 4) && lua_isuserdata(L, 4))
	{
		player = *((player_t **)luaL_checkudata(L, 4, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
		source_volume = (INT32)luaL_checkinteger(L, 2);
		ms = (UINT32)luaL_checkinteger(L, 3);
	}
	else if (luaL_optinteger(L, 3, INT32_MAX) == INT32_MAX)
	{
		ms = (UINT32)luaL_checkinteger(L, 2);
		source_volume = -1;
	}
	else
	{
		source_volume = (INT32)luaL_checkinteger(L, 2);
		ms = (UINT32)luaL_checkinteger(L, 3);
	}

	if (!player || P_IsLocalPlayer(player))
		lua_pushboolean(L, S_FadeMusicFromVolume(target_volume, source_volume, ms));
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sFadeOutStopMusic(lua_State *L)
{
	UINT32 ms = (UINT32)luaL_checkinteger(L, 1);
	player_t *player = NULL;
	//NOHUD
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
	{
		lua_pushboolean(L, S_FadeOutStopMusic(ms));
	}
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sGetMusicLength(lua_State *L)
{
	lua_pushinteger(L, S_GetMusicLength());
	return 1;
}

static int lib_sGetMusicPosition(lua_State *L)
{
	lua_pushinteger(L, S_GetMusicPosition());
	return 1;
}

static int lib_sSetMusicPosition(lua_State *L)
{
	UINT32 pos = (UINT32)luaL_checkinteger(L, 1);
	lua_pushboolean(L, S_SetMusicPosition(pos));
	return 1;
}

static int lib_sOriginPlaying(lua_State *L)
{
	void *origin = NULL;
	//NOHUD
	INLEVEL
	if (!GetValidSoundOrigin(L, &origin))
		return LUA_ErrInvalid(L, "mobj_t/sector_t");

	lua_pushboolean(L, S_OriginPlaying(origin));
	return 1;
}

static int lib_sIdPlaying(lua_State *L)
{
	sfxenum_t id = luaL_checkinteger(L, 1);
	//NOHUD
	if (id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", id, NUMSFX-1);
	lua_pushboolean(L, S_IdPlaying(id));
	return 1;
}

static int lib_sSoundPlaying(lua_State *L)
{
	void *origin = NULL;
	sfxenum_t id = luaL_checkinteger(L, 2);
	//NOHUD
	INLEVEL
	if (id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", id, NUMSFX-1);
	if (!GetValidSoundOrigin(L, &origin))
		return LUA_ErrInvalid(L, "mobj_t/sector_t");

	lua_pushboolean(L, S_SoundPlaying(origin, id));
	return 1;
}

// This doesn't really exist, but we're providing it as a handy netgame-safe wrapper for stuff that should be locally handled.

static int lib_sStartMusicCaption(lua_State *L)
{
	player_t *player = NULL;
	const char *caption = luaL_checkstring(L, 1);
	UINT16 lifespan = (UINT16)luaL_checkinteger(L, 2);
	//HUDSAFE
	//INLEVEL

	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}

	if (lifespan && (!player || P_IsLocalPlayer(player)))
	{
		strlcpy(S_sfx[sfx_None].caption, caption, sizeof(S_sfx[sfx_None].caption));
		S_StartCaption(sfx_None, -1, lifespan);
	}
	return 0;
}

static int lib_sMusicType(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushinteger(L, S_MusicType());
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sMusicPlaying(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushboolean(L, S_MusicPlaying());
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sMusicPaused(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushboolean(L, S_MusicPaused());
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sMusicName(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushstring(L, S_MusicName());
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sMusicExists(lua_State *L)
{
	boolean checkMIDI = lua_opttrueboolean(L, 2);
	boolean checkDigi = lua_opttrueboolean(L, 3);
	const char *music_name = luaL_checkstring(L, 1);
	NOHUD
	lua_pushboolean(L, S_MusicExists(music_name, checkMIDI, checkDigi));
	return 1;
}

static int lib_sSetMusicLoopPoint(lua_State *L)
{
	UINT32 looppoint = (UINT32)luaL_checkinteger(L, 1);
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushboolean(L, S_SetMusicLoopPoint(looppoint));
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sGetMusicLoopPoint(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushinteger(L, (int)S_GetMusicLoopPoint());
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sPauseMusic(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
	{
		S_PauseAudio();
		lua_pushboolean(L, true);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sResumeMusic(lua_State *L)
{
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 1) && lua_isuserdata(L, 1))
	{
		player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
	{
		S_ResumeAudio();
		lua_pushboolean(L, true);
	}
	else
		lua_pushnil(L);
	return 1;
}

// G_GAME
////////////

// Copypasted from lib_cvRegisterVar :]
static int lib_gAddGametype(lua_State *L)
{
	const char *k;
	lua_Integer i;

	const char *gtname = NULL;
	const char *gtconst = NULL;
	const char *gtdescription = NULL;
	INT16 newgtidx = 0;
	UINT32 newgtrules = 0;
	UINT32 newgttol = 0;
	INT32 newgtpointlimit = 0;
	INT32 newgttimelimit = 0;
	UINT8 newgtleftcolor = 0;
	UINT8 newgtrightcolor = 0;
	INT16 newgtrankingstype = -1;
	int newgtinttype = 0;

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1); // Clear out all other possible arguments, leaving only the first one.

	if (!lua_lumploading)
		return luaL_error(L, "This function cannot be called from within a hook or coroutine!");

	// Ran out of gametype slots
	if (gametypecount == NUMGAMETYPEFREESLOTS)
		return luaL_error(L, "Ran out of free gametype slots!");

#define FIELDERROR(f, e) luaL_error(L, "bad value for " LUA_QL(f) " in table passed to " LUA_QL("G_AddGametype") " (%s)", e);
#define TYPEERROR(f, t) FIELDERROR(f, va("%s expected, got %s", lua_typename(L, t), luaL_typename(L, -1)))

	lua_pushnil(L);
	while (lua_next(L, 1)) {
		// stack: gametype table, key/index, value
		//               1            2        3
		i = 0;
		k = NULL;
		if (lua_isnumber(L, 2))
			i = lua_tointeger(L, 2);
		else if (lua_isstring(L, 2))
			k = lua_tostring(L, 2);

		// Sorry, no gametype rules as key names.
		if (i == 1 || (k && fasticmp(k, "name"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("name", LUA_TSTRING)
			gtname = Z_StrDup(lua_tostring(L, 3));
		} else if (i == 2 || (k && fasticmp(k, "identifier"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("identifier", LUA_TSTRING)
			gtconst = Z_StrDup(lua_tostring(L, 3));
		} else if (i == 3 || (k && fasticmp(k, "rules"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("rules", LUA_TNUMBER)
			newgtrules = (UINT32)lua_tointeger(L, 3);
		} else if (i == 4 || (k && fasticmp(k, "typeoflevel"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("typeoflevel", LUA_TNUMBER)
			newgttol = (UINT32)lua_tointeger(L, 3);
		} else if (i == 5 || (k && fasticmp(k, "rankingtype"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("rankingtype", LUA_TNUMBER)
			newgtrankingstype = (INT16)lua_tointeger(L, 3);
		} else if (i == 6 || (k && fasticmp(k, "intermissiontype"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("intermissiontype", LUA_TNUMBER)
			newgtinttype = (int)lua_tointeger(L, 3);
		} else if (i == 7 || (k && fasticmp(k, "defaultpointlimit"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("defaultpointlimit", LUA_TNUMBER)
			newgtpointlimit = (INT32)lua_tointeger(L, 3);
		} else if (i == 8 || (k && fasticmp(k, "defaulttimelimit"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("defaulttimelimit", LUA_TNUMBER)
			newgttimelimit = (INT32)lua_tointeger(L, 3);
		} else if (i == 9 || (k && fasticmp(k, "description"))) {
			if (!lua_isstring(L, 3))
				TYPEERROR("description", LUA_TSTRING)
			gtdescription = Z_StrDup(lua_tostring(L, 3));
		} else if (i == 10 || (k && fasticmp(k, "headerleftcolor"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("headerleftcolor", LUA_TNUMBER)
			newgtleftcolor = (UINT8)lua_tointeger(L, 3);
		} else if (i == 11 || (k && fasticmp(k, "headerrightcolor"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("headerrightcolor", LUA_TNUMBER)
			newgtrightcolor = (UINT8)lua_tointeger(L, 3);
		// Key name specified
		} else if ((!i) && (k && fasticmp(k, "headercolor"))) {
			if (!lua_isnumber(L, 3))
				TYPEERROR("headercolor", LUA_TNUMBER)
			newgtleftcolor = newgtrightcolor = (UINT8)lua_tointeger(L, 3);
		}
		lua_pop(L, 1);
	}

#undef FIELDERROR
#undef TYPEERROR

	// pop gametype table
	lua_pop(L, 1);

	// Set defaults
	if (gtname == NULL)
		gtname = Z_StrDup("Unnamed gametype");
	if (gtdescription == NULL)
		gtdescription = Z_StrDup("???");

	// Add the new gametype
	newgtidx = G_AddGametype(newgtrules);
	G_AddGametypeTOL(newgtidx, newgttol);
	G_SetGametypeDescription(newgtidx, NULL, newgtleftcolor, newgtrightcolor);
	strncpy(gametypedesc[newgtidx].notes, gtdescription, 441);

	// Not covered by G_AddGametype alone.
	if (newgtrankingstype == -1)
		newgtrankingstype = newgtidx;
	gametyperankings[newgtidx] = newgtrankingstype;
	intermissiontypes[newgtidx] = newgtinttype;
	pointlimits[newgtidx] = newgtpointlimit;
	timelimits[newgtidx] = newgttimelimit;

	// Write the new gametype name.
	Gametype_Names[newgtidx] = gtname;

	// Write the constant name.
	if (gtconst == NULL)
		gtconst = gtname;
	G_AddGametypeConstant(newgtidx, gtconst);

	// Update gametype_cons_t accordingly.
	G_UpdateGametypeSelections();

	// done
	CONS_Printf("Added gametype %s\n", Gametype_Names[newgtidx]);
	return 0;
}

// Bot adding function!
// Partly lifted from Got_AddPlayer
static int lib_gAddPlayer(lua_State *L)
{
	INT16 i, newplayernum;
	player_t *newplayer;
	SINT8 skinnum = 0, bot;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			break;
	}

	if (i >= MAXPLAYERS)
	{
		lua_pushnil(L);
		return 1;
	}

	newplayernum = i;

	CL_ClearPlayer(newplayernum);

	playeringame[newplayernum] = true;
	G_AddPlayer(newplayernum);
	newplayer = &players[newplayernum];

	newplayer->jointime = 0;
	newplayer->quittime = 0;
	newplayer->lastinputtime = 0;

	// Read the skin argument (defaults to Sonic)
	if (!lua_isnoneornil(L, 1))
	{
		skinnum = R_SkinAvailable(luaL_checkstring(L, 1));
		skinnum = skinnum < 0 ? 0 : skinnum;
	}

	// Read the color (defaults to skin prefcolor)
	if (!lua_isnoneornil(L, 2))
		newplayer->skincolor = R_GetColorByName(luaL_checkstring(L, 2));
	else
		newplayer->skincolor = skins[skinnum]->prefcolor;

	// Set the bot default name as the skin
	strcpy(player_names[newplayernum], skins[skinnum]->realname);

	// Read the bot name, if given
	if (!lua_isnoneornil(L, 3))
		strlcpy(player_names[newplayernum], luaL_checkstring(L, 3), sizeof(*player_names));

	bot = luaL_optinteger(L, 4, 3);
	newplayer->bot = (bot >= BOT_NONE && bot <= BOT_MPAI) ? bot : BOT_MPAI;

	// If our bot is a 2P type, we'll need to set its leader so it can spawn
	if (newplayer->bot == BOT_2PAI || newplayer->bot == BOT_2PHUMAN)
		B_UpdateBotleader(newplayer);

	// Set the skin (can't do this until AFTER bot type is set!)
	SetPlayerSkinByNum(newplayernum, skinnum);

	if (netgame)
	{
		char joinmsg[256];

		// Truncate bot name
		player_names[newplayernum][sizeof(*player_names) - 8] = '\0'; // The length of colored [BOT] + 1

		strcpy(joinmsg, M_GetText("\x82*Bot %s has joined the game (player %d)"));
		strcpy(joinmsg, va(joinmsg, player_names[newplayernum], newplayernum));
		HU_AddChatText(joinmsg, false);

		// Append blue [BOT] tag at the end
		strlcat(player_names[newplayernum], "\x84[BOT]\x80", sizeof(*player_names));
	}

	LUA_PushUserdata(L, newplayer, META_PLAYER);
	return 1;
}


// Bot removing function
static int lib_gRemovePlayer(lua_State *L)
{
	UINT8 pnum = -1;
	if (!lua_isnoneornil(L, 1))
		pnum = luaL_checkinteger(L, 1);
	else // No argument
		return luaL_error(L, "argument #1 not given (expected number)");
	if (pnum >= MAXPLAYERS) // Out of range
		return luaL_error(L, "playernum %d out of range (0 - %d)", pnum, MAXPLAYERS-1);
	if (playeringame[pnum]) // Found player
	{
		if (players[pnum].bot == BOT_NONE) // Can't remove clients.
			return luaL_error(L, "G_RemovePlayer can only be used on players with a bot value other than BOT_NONE.");
		else
		{
			players[pnum].removing = true;
			lua_pushboolean(L, true);
			return 1;
		}
	}
	// Fell through. Invalid player
	return LUA_ErrInvalid(L, "player_t");
}


static int lib_gSetUsedCheats(lua_State *L)
{
	// Let large-scale level packs using Lua be able to add cheat commands.
	boolean silent = lua_optboolean(L, 1);
	//NOHUD
	//INLEVEL
	G_SetUsedCheats(silent);
	return 0;
}

static int Lcheckmapnumber (lua_State *L, int idx, const char *fun)
{
	if (ISINLEVEL)
		return luaL_optinteger(L, idx, gamemap);
	else
	{
		if (lua_isnoneornil(L, idx))
		{
			return luaL_error(L,
					"%s can only be used without a parameter while in a level.",
					fun
			);
		}
		else
			return luaL_checkinteger(L, idx);
	}
}

static int lib_gBuildMapName(lua_State *L)
{
	INT32 map = Lcheckmapnumber(L, 1, "G_BuildMapName");
	//HUDSAFE
	lua_pushstring(L, G_BuildMapName(map));
	return 1;
}

static int lib_gBuildMapTitle(lua_State *L)
{
	INT32 map = Lcheckmapnumber(L, 1, "G_BuildMapTitle");
	char *name;
	if (map < 1 || map > NUMMAPS)
	{
		return luaL_error(L,
				"map number %d out of range (1 - %d)",
				map,
				NUMMAPS
		);
	}
	name = G_BuildMapTitle(map);
	lua_pushstring(L, name);
	Z_Free(name);
	return 1;
}

static void
Lpushdim (lua_State *L, int c, struct searchdim *v)
{
	int i;
	lua_createtable(L, c, 0);/* I guess narr is numeric indices??? */
	for (i = 0; i < c; ++i)
	{
		lua_createtable(L, 0, 2);/* and hashed indices (field)... */
			lua_pushnumber(L, v[i].pos);
			lua_setfield(L, -2, "pos");

			lua_pushnumber(L, v[i].siz);
			lua_setfield(L, -2, "siz");
		lua_rawseti(L, -2, 1 + i);
	}
}

/*
I decided to make this return a table because userdata
is scary and tables let the user set their own fields.
*/
/*
Returns:

[1] => map number
[2] => map title
[3] => search frequency table

The frequency table is unsorted. It has the following format:

{
	['mapnum'],

	['matchd'] => matches in map title string
	['keywhd'] => matches in map keywords

	The above two tables have the following format:

	{
		['pos'] => offset from start of string
		['siz'] => length of match
	}...

	['total'] => the total matches
}...
*/
static int lib_gFindMap(lua_State *L)
{
	const char *query = luaL_checkstring(L, 1);

	INT32 map;
	char *realname;
	INT32 frc;
	mapsearchfreq_t *frv;

	INT32 i;

	map = G_FindMap(query, &realname, &frv, &frc);

	lua_settop(L, 0);

	lua_pushnumber(L, map);
	lua_pushstring(L, realname);

	lua_createtable(L, frc, 0);
	for (i = 0; i < frc; ++i)
	{
		lua_createtable(L, 0, 4);
			lua_pushnumber(L, frv[i].mapnum);
			lua_setfield(L, -2, "mapnum");

			Lpushdim(L, frv[i].matchc, frv[i].matchd);
			lua_setfield(L, -2, "matchd");

			Lpushdim(L, frv[i].keywhc, frv[i].keywhd);
			lua_setfield(L, -2, "keywhd");

			lua_pushnumber(L, frv[i].total);
			lua_setfield(L, -2, "total");
		lua_rawseti(L, -2, 1 + i);
	}

	G_FreeMapSearch(frv, frc);
	Z_Free(realname);

	return 3;
}

/*
Returns:

[1] => map number
[2] => map title
*/
static int lib_gFindMapByNameOrCode(lua_State *L)
{
	const char *query = luaL_checkstring(L, 1);
	INT32 map;
	char *realname;
	map = G_FindMapByNameOrCode(query, &realname);
	lua_pushnumber(L, map);
	if (map)
	{
		lua_pushstring(L, realname);
		Z_Free(realname);
		return 2;
	}
	else
		return 1;
}

static int lib_gDoReborn(lua_State *L)
{
	INT32 playernum = luaL_checkinteger(L, 1);
	NOHUD
	INLEVEL
	if (playernum >= MAXPLAYERS)
		return luaL_error(L, "playernum %d out of range (0 - %d)", playernum, MAXPLAYERS-1);
	G_DoReborn(playernum);
	return 0;
}

// Another Lua function that doesn't actually exist!
// Sets nextmapoverride, skipstats and nextgametype without instantly ending the level, for instances where other sources should be exiting the level, like normal signposts.
static int lib_gSetCustomExitVars(lua_State *L)
{
	int n = lua_gettop(L); // Num arguments
	NOHUD
	INLEVEL

	// LUA EXTENSION: Custom exit like support
	// Supported:
	//	G_SetCustomExitVars();               [reset to defaults]
	//	G_SetCustomExitVars(int)             [nextmap override only]
	//	G_SetCustomExitVars(nil, int)        [skipstats only]
	//	G_SetCustomExitVars(int, int)        [both of the above]
	//	G_SetCustomExitVars(int, int, int)   [nextmapoverride, skipstats and nextgametype]

	nextmapoverride = 0;
	skipstats = 0;
	nextgametype = -1;

	if (n >= 1)
	{
		nextmapoverride = (INT16)luaL_optinteger(L, 1, 0);
		skipstats = (INT16)luaL_optinteger(L, 2, 0);
		nextgametype = (INT16)luaL_optinteger(L, 3, -1);
	}

	return 0;
}

static int lib_gEnoughPlayersFinished(lua_State *L)
{
	INLEVEL
	lua_pushboolean(L, G_EnoughPlayersFinished());
	return 1;
}

static int lib_gExitLevel(lua_State *L)
{
	int n = lua_gettop(L); // Num arguments
	NOHUD
	// Moved this bit to G_SetCustomExitVars
	if (n >= 1) // Don't run the reset to defaults option
		lib_gSetCustomExitVars(L);
	G_ExitLevel();
	return 0;
}

static int lib_gIsSpecialStage(lua_State *L)
{
	INT32 mapnum = luaL_optinteger(L, 1, gamemap);
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_IsSpecialStage(mapnum));
	return 1;
}

static int lib_gGametypeUsesLives(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeUsesLives());
	return 1;
}

static int lib_gGametypeUsesCoopLives(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeUsesCoopLives());
	return 1;
}

static int lib_gGametypeUsesCoopStarposts(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeUsesCoopStarposts());
	return 1;
}

static int lib_gGametypeHasTeams(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeHasTeams());
	return 1;
}

static int lib_gGametypeHasSpectators(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_GametypeHasSpectators());
	return 1;
}

static int lib_gRingSlingerGametype(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_RingSlingerGametype());
	return 1;
}

static int lib_gPlatformGametype(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_PlatformGametype());
	return 1;
}

static int lib_gCoopGametype(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_CoopGametype());
	return 1;
}

static int lib_gTagGametype(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_TagGametype());
	return 1;
}

static int lib_gCompetitionGametype(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_CompetitionGametype());
	return 1;
}

static int lib_gTicsToHours(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToHours(rtic));
	return 1;
}

static int lib_gTicsToMinutes(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	boolean rfull = lua_optboolean(L, 2);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToMinutes(rtic, rfull));
	return 1;
}

static int lib_gTicsToSeconds(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToSeconds(rtic));
	return 1;
}

static int lib_gTicsToCentiseconds(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToCentiseconds(rtic));
	return 1;
}

static int lib_gTicsToMilliseconds(lua_State *L)
{
	tic_t rtic = luaL_checkinteger(L, 1);
	//HUDSAFE
	lua_pushinteger(L, G_TicsToMilliseconds(rtic));
	return 1;
}

static int lib_getTimeMicros(lua_State *L)
{
	lua_pushinteger(L, I_GetPreciseTime() / (I_GetPrecisePrecision() / 1000000));
	return 1;
}

static luaL_Reg lib[] = {
	{"print", lib_print},
	{"chatprint", lib_chatprint},
	{"chatprintf", lib_chatprintf},
	{"userdataType", lib_userdataType},
	{"registerMetatable", lib_registerMetatable},
	{"userdataMetatable", lib_userdataMetatable},
	{"IsPlayerAdmin", lib_isPlayerAdmin},
	{"reserveLuabanks", lib_reserveLuabanks},
	{"tofixed", lib_tofixed},

	// m_menu
	{"M_MoveColorAfter",lib_pMoveColorAfter},
	{"M_MoveColorBefore",lib_pMoveColorBefore},
	{"M_GetColorAfter",lib_pGetColorAfter},
	{"M_GetColorBefore",lib_pGetColorBefore},

	// m_misc
	{"M_MapNumber",lib_mMapNumber},

	// m_random
	{"P_RandomFixed",lib_pRandomFixed},
	{"P_RandomByte",lib_pRandomByte},
	{"P_RandomKey",lib_pRandomKey},
	{"P_RandomRange",lib_pRandomRange},
	{"P_SignedRandom",lib_pSignedRandom}, // MACRO
	{"P_RandomChance",lib_pRandomChance}, // MACRO

	// p_maputil
	{"P_AproxDistance",lib_pAproxDistance},
	{"P_ClosestPointOnLine",lib_pClosestPointOnLine},
	{"P_PointOnLineSide",lib_pPointOnLineSide},

	// p_enemy
	{"P_CheckMeleeRange", lib_pCheckMeleeRange},
	{"P_JetbCheckMeleeRange", lib_pJetbCheckMeleeRange},
	{"P_FaceStabCheckMeleeRange", lib_pFaceStabCheckMeleeRange},
	{"P_SkimCheckMeleeRange", lib_pSkimCheckMeleeRange},
	{"P_CheckMissileRange", lib_pCheckMissileRange},
	{"P_NewChaseDir", lib_pNewChaseDir},
	{"P_LookForPlayers", lib_pLookForPlayers},

	// p_mobj
	// don't add P_SetMobjState or P_SetPlayerMobjState, use "mobj.state = S_NEWSTATE" instead.
	{"P_SpawnMobj",lib_pSpawnMobj},
	{"P_SpawnMobjFromMobj",lib_pSpawnMobjFromMobj},
	{"P_RemoveMobj",lib_pRemoveMobj},
	{"P_IsValidSprite2", lib_pIsValidSprite2},
	{"P_SpawnLockOn", lib_pSpawnLockOn},
	{"P_SpawnMissile",lib_pSpawnMissile},
	{"P_SpawnXYZMissile",lib_pSpawnXYZMissile},
	{"P_SpawnPointMissile",lib_pSpawnPointMissile},
	{"P_SpawnAlteredDirectionMissile",lib_pSpawnAlteredDirectionMissile},
	{"P_ColorTeamMissile",lib_pColorTeamMissile},
	{"P_SPMAngle",lib_pSPMAngle},
	{"P_SpawnPlayerMissile",lib_pSpawnPlayerMissile},
	{"P_MobjFlip",lib_pMobjFlip},
	{"P_GetMobjGravity",lib_pGetMobjGravity},
	{"P_WeaponOrPanel",lib_pWeaponOrPanel},
	{"P_FlashPal",lib_pFlashPal},
	{"P_GetClosestAxis",lib_pGetClosestAxis},
	{"P_SpawnParaloop",lib_pSpawnParaloop},
	{"P_BossTargetPlayer",lib_pBossTargetPlayer},
	{"P_SupermanLook4Players",lib_pSupermanLook4Players},
	{"P_SetScale",lib_pSetScale},
	{"P_InsideANonSolidFFloor",lib_pInsideANonSolidFFloor},
	{"P_CheckDeathPitCollide",lib_pCheckDeathPitCollide},
	{"P_CheckSolidLava",lib_pCheckSolidLava},
	{"P_CanRunOnWater",lib_pCanRunOnWater},
	{"P_MaceRotate",lib_pMaceRotate},
	{"P_CreateFloorSpriteSlope",lib_pCreateFloorSpriteSlope},
	{"P_RemoveFloorSpriteSlope",lib_pRemoveFloorSpriteSlope},
	{"P_RailThinker",lib_pRailThinker},
	{"P_CheckSkyHit",lib_pCheckSkyHit},
	{"P_XYMovement",lib_pXYMovement},
	{"P_RingXYMovement",lib_pRingXYMovement},
	{"P_SceneryXYMovement",lib_pSceneryXYMovement},
	{"P_ZMovement",lib_pZMovement},
	{"P_RingZMovement",lib_pRingZMovement},
	{"P_SceneryZMovement",lib_pSceneryZMovement},
	{"P_PlayerZMovement",lib_pPlayerZMovement},

	// p_user
	{"P_GetPlayerHeight",lib_pGetPlayerHeight},
	{"P_GetPlayerSpinHeight",lib_pGetPlayerSpinHeight},
	{"P_GetPlayerControlDirection",lib_pGetPlayerControlDirection},
	{"P_AddPlayerScore",lib_pAddPlayerScore},
	{"P_StealPlayerScore",lib_pStealPlayerScore},
	{"P_GetJumpFlags",lib_pGetJumpFlags},
	{"P_PlayerInPain",lib_pPlayerInPain},
	{"P_DoPlayerPain",lib_pDoPlayerPain},
	{"P_ResetPlayer",lib_pResetPlayer},
	{"P_PlayerCanDamage",lib_pPlayerCanDamage},
	{"P_PlayerFullbright",lib_pPlayerFullbright},
	{"P_IsObjectInGoop",lib_pIsObjectInGoop},
	{"P_IsObjectOnGround",lib_pIsObjectOnGround},
	{"P_InSpaceSector",lib_pInSpaceSector},
	{"P_InQuicksand",lib_pInQuicksand},
	{"P_InJumpFlipSector",lib_pInJumpFlipSector},
	{"P_SetObjectMomZ",lib_pSetObjectMomZ},
	{"P_IsLocalPlayer",lib_pIsLocalPlayer},
	{"P_PlayJingle",lib_pPlayJingle},
	{"P_PlayJingleMusic",lib_pPlayJingleMusic},
	{"P_RestoreMusic",lib_pRestoreMusic},
	{"P_SpawnShieldOrb",lib_pSpawnShieldOrb},
	{"P_SpawnGhostMobj",lib_pSpawnGhostMobj},
	{"P_GivePlayerRings",lib_pGivePlayerRings},
	{"P_GivePlayerSpheres",lib_pGivePlayerSpheres},
	{"P_GivePlayerLives",lib_pGivePlayerLives},
	{"P_GiveCoopLives",lib_pGiveCoopLives},
	{"P_ResetScore",lib_pResetScore},
	{"P_DoJumpShield",lib_pDoJumpShield},
	{"P_DoBubbleBounce",lib_pDoBubbleBounce},
	{"P_BlackOw",lib_pBlackOw},
	{"P_ElementalFire",lib_pElementalFire},
	{"P_SpawnSkidDust", lib_pSpawnSkidDust},
	{"P_MovePlayer",lib_pMovePlayer},
	{"P_DoPlayerFinish",lib_pDoPlayerFinish},
	{"P_DoPlayerExit",lib_pDoPlayerExit},
	{"P_InstaThrust",lib_pInstaThrust},
	{"P_InstaThrustEvenIn2D",lib_pInstaThrustEvenIn2D},
	{"P_ReturnThrustX",lib_pReturnThrustX},
	{"P_ReturnThrustY",lib_pReturnThrustY},
	{"P_LookForEnemies",lib_pLookForEnemies},
	{"P_NukeEnemies",lib_pNukeEnemies},
	{"P_Earthquake",lib_pEarthquake},
	{"P_HomingAttack",lib_pHomingAttack},
	{"P_ResetCamera",lib_pResetCamera},
	{"P_SuperReady",lib_pSuperReady},
	{"P_DoJump",lib_pDoJump},
	{"P_DoSpinDashDust",lib_pDoSpinDashDust},
	{"P_SpawnThokMobj",lib_pSpawnThokMobj},
	{"P_SpawnSpinMobj",lib_pSpawnSpinMobj},
	{"P_Telekinesis",lib_pTelekinesis},
	{"P_SwitchShield",lib_pSwitchShield},
	{"P_DoTailsOverlay",lib_pDoTailsOverlay},
	{"P_DoMetalJetFume",lib_pDoMetalJetFume},
	{"P_DoFollowMobj",lib_pDoFollowMobj},
	{"P_PlayerCanEnterSpinGaps",lib_pPlayerCanEnterSpinGaps},
	{"P_PlayerShouldUseSpinHeight",lib_pPlayerShouldUseSpinHeight},

	// p_map
	{"P_CheckPosition",lib_pCheckPosition},
	{"P_TryMove",lib_pTryMove},
	{"P_Move",lib_pMove},
	{"P_TeleportMove",lib_pTeleportMove},
	{"P_SetOrigin",lib_pSetOrigin},
	{"P_MoveOrigin",lib_pMoveOrigin},
	{"P_LineIsBlocking",lib_pLineIsBlocking},
	{"P_SlideMove",lib_pSlideMove},
	{"P_BounceMove",lib_pBounceMove},
	{"P_CheckSight", lib_pCheckSight},
	{"P_CheckHoopPosition",lib_pCheckHoopPosition},
	{"P_RadiusAttack",lib_pRadiusAttack},
	{"P_FloorzAtPos",lib_pFloorzAtPos},
	{"P_CeilingzAtPos",lib_pCeilingzAtPos},
	{"P_GetSectorLightLevelAt",lib_pGetSectorLightLevelAt},
	{"P_GetSectorColormapAt",lib_pGetSectorColormapAt},
	{"P_DoSpring",lib_pDoSpring},
	{"P_TouchSpecialThing",lib_pTouchSpecialThing},
	{"P_TryCameraMove", lib_pTryCameraMove},
	{"P_TeleportCameraMove", lib_pTeleportCameraMove},

	// p_inter
	{"P_RemoveShield",lib_pRemoveShield},
	{"P_DamageMobj",lib_pDamageMobj},
	{"P_KillMobj",lib_pKillMobj},
	{"P_PlayerRingBurst",lib_pPlayerRingBurst},
	{"P_PlayerWeaponPanelBurst",lib_pPlayerWeaponPanelBurst},
	{"P_PlayerWeaponAmmoBurst",lib_pPlayerWeaponAmmoBurst},
	{"P_PlayerWeaponPanelOrAmmoBurst", lib_pPlayerWeaponPanelOrAmmoBurst},
	{"P_PlayerEmeraldBurst",lib_pPlayerEmeraldBurst},
	{"P_PlayerFlagBurst",lib_pPlayerFlagBurst},
	{"P_PlayRinglossSound",lib_pPlayRinglossSound},
	{"P_PlayDeathSound",lib_pPlayDeathSound},
	{"P_PlayVictorySound",lib_pPlayVictorySound},
	{"P_PlayLivesJingle",lib_pPlayLivesJingle},
	{"P_CanPickupItem",lib_pCanPickupItem},
	{"P_DoNightsScore",lib_pDoNightsScore},
	{"P_DoMatchSuper",lib_pDoMatchSuper},

	// p_spec
	{"P_Thrust",lib_pThrust},
	{"P_ThrustEvenIn2D",lib_pThrustEvenIn2D},
	{"P_VectorInstaThrust",lib_pVectorInstaThrust},
	{"P_SetMobjStateNF",lib_pSetMobjStateNF},
	{"P_DoSuperTransformation",lib_pDoSuperTransformation},
	{"P_DoSuperDetransformation",lib_pDoSuperDetransformation},
	{"P_ExplodeMissile",lib_pExplodeMissile},
	{"P_MobjTouchingSectorSpecial",lib_pMobjTouchingSectorSpecial},
	{"P_ThingOnSpecial3DFloor",lib_pThingOnSpecial3DFloor},
	{"P_MobjTouchingSectorSpecialFlag",lib_pMobjTouchingSectorSpecialFlag},
	{"P_PlayerTouchingSectorSpecial",lib_pPlayerTouchingSectorSpecial},
	{"P_PlayerTouchingSectorSpecialFlag",lib_pPlayerTouchingSectorSpecialFlag},
	{"P_FindLowestFloorSurrounding",lib_pFindLowestFloorSurrounding},
	{"P_FindHighestFloorSurrounding",lib_pFindHighestFloorSurrounding},
	{"P_FindNextHighestFloor",lib_pFindNextHighestFloor},
	{"P_FindNextLowestFloor",lib_pFindNextLowestFloor},
	{"P_FindLowestCeilingSurrounding",lib_pFindLowestCeilingSurrounding},
	{"P_FindHighestCeilingSurrounding",lib_pFindHighestCeilingSurrounding},
	{"P_FindSpecialLineFromTag",lib_pFindSpecialLineFromTag},
	{"P_SwitchWeather",lib_pSwitchWeather},
	{"P_LinedefExecute",lib_pLinedefExecute},
	{"P_SpawnLightningFlash",lib_pSpawnLightningFlash},
	{"P_FadeLight",lib_pFadeLight},
	{"P_IsFlagAtBase",lib_pIsFlagAtBase},
	{"P_SetupLevelSky",lib_pSetupLevelSky},
	{"P_SetSkyboxMobj",lib_pSetSkyboxMobj},
	{"P_StartQuake",lib_pStartQuake},
	{"EV_CrumbleChain",lib_evCrumbleChain},
	{"EV_StartCrumble",lib_evStartCrumble},

	// p_slopes
	{"P_GetZAt",lib_pGetZAt},
	{"P_ButteredSlope",lib_pButteredSlope},

	// r_defs
	{"R_PointToAngle",lib_rPointToAngle},
	{"R_PointToAngle2",lib_rPointToAngle2},
	{"R_PointToDist",lib_rPointToDist},
	{"R_PointToDist2",lib_rPointToDist2},
	{"R_PointInSubsector",lib_rPointInSubsector},
	{"R_PointInSubsectorOrNil",lib_rPointInSubsectorOrNil},

	// r_things (sprite)
	{"R_Char2Frame",lib_rChar2Frame},
	{"R_Frame2Char",lib_rFrame2Char},
	{"R_SetPlayerSkin",lib_rSetPlayerSkin},

	// r_skins
	{"R_SkinUsable",lib_rSkinUsable},
	{"P_GetStateSprite2",lib_pGetStateSprite2},
	{"P_GetSprite2StateFrame",lib_pGetSprite2StateFrame},
	{"P_IsStateSprite2Super",lib_pIsStateSprite2Super},
	{"P_GetSuperSprite2",lib_pGetSuperSprite2},

	// r_data
	{"R_CheckTextureNumForName",lib_rCheckTextureNumForName},
	{"R_TextureNumForName",lib_rTextureNumForName},
	{"R_CheckTextureNameForNum", lib_rCheckTextureNameForNum},
	{"R_TextureNameForNum", lib_rTextureNameForNum},

	// r_draw
	{"R_GetColorByName", lib_rGetColorByName},
	{"R_GetSuperColorByName", lib_rGetSuperColorByName},
	{"R_GetNameByColor", lib_rGetNameByColor},

	// s_sound
	{"S_StartSound",lib_sStartSound},
	{"S_StartSoundAtVolume",lib_sStartSoundAtVolume},
	{"S_StopSound",lib_sStopSound},
	{"S_StopSoundByID",lib_sStopSoundByID},
	{"S_ChangeMusic",lib_sChangeMusic},
	{"S_SpeedMusic",lib_sSpeedMusic},
	{"S_StopMusic",lib_sStopMusic},
	{"S_SetInternalMusicVolume", lib_sSetInternalMusicVolume},
	{"S_StopFadingMusic",lib_sStopFadingMusic},
	{"S_FadeMusic",lib_sFadeMusic},
	{"S_FadeOutStopMusic",lib_sFadeOutStopMusic},
	{"S_GetMusicLength",lib_sGetMusicLength},
	{"S_GetMusicPosition",lib_sGetMusicPosition},
	{"S_SetMusicPosition",lib_sSetMusicPosition},
	{"S_OriginPlaying",lib_sOriginPlaying},
	{"S_IdPlaying",lib_sIdPlaying},
	{"S_SoundPlaying",lib_sSoundPlaying},
	{"S_StartMusicCaption", lib_sStartMusicCaption},
	{"S_MusicType",lib_sMusicType},
	{"S_MusicPlaying",lib_sMusicPlaying},
	{"S_MusicPaused",lib_sMusicPaused},
	{"S_MusicName",lib_sMusicName},
	{"S_MusicExists",lib_sMusicExists},
	{"S_SetMusicLoopPoint",lib_sSetMusicLoopPoint},
	{"S_GetMusicLoopPoint",lib_sGetMusicLoopPoint},
	{"S_PauseMusic",lib_sPauseMusic},
	{"S_ResumeMusic", lib_sResumeMusic},

	// g_game
	{"G_AddGametype", lib_gAddGametype},
	{"G_AddPlayer", lib_gAddPlayer},
	{"G_RemovePlayer", lib_gRemovePlayer},
	{"G_SetUsedCheats", lib_gSetUsedCheats},
	{"G_BuildMapName",lib_gBuildMapName},
	{"G_BuildMapTitle",lib_gBuildMapTitle},
	{"G_FindMap",lib_gFindMap},
	{"G_FindMapByNameOrCode",lib_gFindMapByNameOrCode},
	{"G_DoReborn",lib_gDoReborn},
	{"G_SetCustomExitVars",lib_gSetCustomExitVars},
	{"G_EnoughPlayersFinished",lib_gEnoughPlayersFinished},
	{"G_ExitLevel",lib_gExitLevel},
	{"G_IsSpecialStage",lib_gIsSpecialStage},
	{"G_GametypeUsesLives",lib_gGametypeUsesLives},
	{"G_GametypeUsesCoopLives",lib_gGametypeUsesCoopLives},
	{"G_GametypeUsesCoopStarposts",lib_gGametypeUsesCoopStarposts},
	{"G_GametypeHasTeams",lib_gGametypeHasTeams},
	{"G_GametypeHasSpectators",lib_gGametypeHasSpectators},
	{"G_RingSlingerGametype",lib_gRingSlingerGametype},
	{"G_PlatformGametype",lib_gPlatformGametype},
	{"G_CoopGametype",lib_gCoopGametype},
	{"G_TagGametype",lib_gTagGametype},
	{"G_CompetitionGametype",lib_gCompetitionGametype},
	{"G_TicsToHours",lib_gTicsToHours},
	{"G_TicsToMinutes",lib_gTicsToMinutes},
	{"G_TicsToSeconds",lib_gTicsToSeconds},
	{"G_TicsToCentiseconds",lib_gTicsToCentiseconds},
	{"G_TicsToMilliseconds",lib_gTicsToMilliseconds},

	{"getTimeMicros",lib_getTimeMicros},

	{NULL, NULL}
};

int LUA_BaseLib(lua_State *L)
{
	// Set metatable for string
	lua_pushliteral(L, "");  // dummy string
	lua_getmetatable(L, -1);  // get string metatable
	LUA_SetCFunctionField(L, "__add", lib_concat);
	lua_pop(L, 2); // pop metatable and dummy string

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);

	// Set global functions
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}
