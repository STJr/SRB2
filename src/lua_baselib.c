// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_baselib.c
/// \brief basic functions for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "p_local.h"
#include "p_setup.h" // So we can have P_SetupLevelSky
#include "z_zone.h"
#include "r_main.h"
#include "r_things.h"
#include "m_random.h"
#include "s_sound.h"
#include "g_game.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

#define NOHUD if (hud_running)\
return luaL_error(L, "HUD rendering code should not call this function!");
#define INLEVEL if (gamestate != GS_LEVEL)\
return luaL_error(L, "This function can only be used in a level!");

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

static const struct {
	const char *meta;
	const char *utype;
} meta2utype[] = {
	{META_STATE,        "state_t"},
	{META_MOBJINFO,     "mobjinfo_t"},
	{META_SFXINFO,      "sfxinfo_t"},

	{META_MOBJ,         "mobj_t"},
	{META_MAPTHING,     "mapthing_t"},

	{META_PLAYER,       "player_t"},
	{META_TICCMD,       "ticcmd_t"},
	{META_SKIN,         "skin_t"},
	{META_POWERS,       "player_t.powers"},
	{META_SOUNDSID,     "skin_t.soundsid"},

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
	{META_MAPHEADER,    "mapheader_t"},

	{META_CVAR,         "consvar_t"},

	{META_SECTORLINES,  "sector_t.lines"},
	{META_SIDENUM,      "line_t.sidenum"},
#ifdef HAVE_LUA_SEGS
	{META_NODEBBOX,     "node_t.bbox"},
	{META_NODECHILDREN, "node_t.children"},
#endif

	{META_BBOX,         "bbox"},

	{META_HUDINFO,      "hudinfo_t"},
	{META_PATCH,        "patch_t"},
	{META_COLORMAP,     "colormap"},
	{META_CAMERA,       "camera_t"},

	{META_ACTION,       "action"},
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
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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

// P_IsValidSprite2 technically doesn't exist, and probably never should... but too much would need to be exposed to allow this to be checked by other methods.

static int lib_pIsValidSprite2(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	UINT8 spr2 = (UINT8)luaL_checkinteger(L, 2);
	//HUDSAFE
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, (mobj->skin && (((skin_t *)mobj->skin)->sprites[spr2].numframes)));
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
	if (P_IsLocalPlayer(player)) // Only display it on your own view.
	{
		mobj_t *visual = P_SpawnMobj(lockon->x, lockon->y, lockon->z, MT_LOCKON); // positioning, flip handled in P_SceneryThinker
		visual->target = lockon;
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
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
	if (nstate >= NUMSTATES)
		return luaL_error(L, "state %d out of range (0 - %d)", nstate, NUMSTATES-1);
	P_SpawnParaloop(x, y, z, radius, number, type, nstate, rotangle, spawncenter);
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
	NOHUD
	INLEVEL
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (newscale < FRACUNIT/100)
		newscale = FRACUNIT/100;
	P_SetScale(mobj, newscale);
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
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	//HUDSAFE
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!rover)
		return LUA_ErrInvalid(L, "ffloor_t");
	lua_pushboolean(L, P_CheckSolidLava(mo, rover));
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

static int lib_pRestoreMusic(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!player || P_IsLocalPlayer(player))
	{
		P_RestoreMusic(player);
		lua_pushboolean(L, true);
	}
	else
		lua_pushnil(L);
	return 1;
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

static int lib_pDoPlayerExit(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoPlayerExit(player);
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

static int lib_pHomingAttack(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *enemy = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	INLEVEL
	if (!source || !enemy)
		return LUA_ErrInvalid(L, "mobj_t");
	P_HomingAttack(source, enemy);
	return 0;
}

static int lib_pSuperReady(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_SuperReady(player));
	return 1;
}

static int lib_pDoJump(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean soundandstate = (boolean)lua_opttrueboolean(L, 2);
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoJump(player, soundandstate);
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (type >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", type, NUMMOBJTYPES-1);
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
	lua_pushboolean(L, P_TeleportMove(thing, x, y, z));
	LUA_PushUserdata(L, tmthing, META_MOBJ);
	P_SetTarget(&tmthing, ptmthing);
	return 2;
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
	NOHUD
	INLEVEL
	if (!spot || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RadiusAttack(spot, source, damagedist, damagetype);
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
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
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
	NOHUD
	INLEVEL
	P_FadeLight(tag, destvalue, speed);
	return 0;
}

static int lib_pThingOnSpecial3DFloor(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_ThingOnSpecial3DFloor(mo), META_SECTOR);
	return 1;
}

static int lib_pIsFlagAtBase(lua_State *L)
{
	mobjtype_t flag = luaL_checkinteger(L, 1);
	NOHUD
	INLEVEL
	if (flag >= NUMMOBJTYPES)
		return luaL_error(L, "mobj type %d out of range (0 - %d)", flag, NUMMOBJTYPES-1);
	lua_pushboolean(L, P_IsFlagAtBase(flag));
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
	sector_t *sec = *((sector_t **)luaL_checkudata(L, 1, META_SECTOR));
	ffloor_t *rover = *((ffloor_t **)luaL_checkudata(L, 2, META_FFLOOR));
	NOHUD
	INLEVEL
	if (!sec)
		return LUA_ErrInvalid(L, "sector_t");
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
	c[1] = 0;

	lua_pushstring(L, c);
	lua_pushinteger(L, c[0]);
	return 2;
}

// R_SetPlayerSkin technically doesn't exist either, although it's basically just SetPlayerSkin and SetPlayerSkinByNum handled in one place for convenience
static int lib_rSetPlayerSkin(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	INLEVEL
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (lua_isnoneornil(L, 2))
		return luaL_error(L, "argument #2 not given (expected number or string)");
	else if (lua_type(L, 2) == LUA_TNUMBER) // skin number
	{
		INT32 i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= MAXSKINS)
			return luaL_error(L, "skin number (argument #2) %d out of range (0 - %d)", i, MAXSKINS-1);
		SetPlayerSkinByNum(player-players, i);
	}
	else // skin name
	{
		const char *skinname = luaL_checkstring(L, 2);
		SetPlayerSkin(player-players, skinname);
	}
	return 0;
}

// S_SOUND
////////////

static int lib_sStartSound(lua_State *L)
{
	const void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	player_t *player = NULL;
	NOHUD
	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);
	if (!lua_isnil(L, 1))
	{
		origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (!origin)
			return LUA_ErrInvalid(L, "mobj_t");
	}
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		S_StartSound(origin, sound_id);
	return 0;
}

static int lib_sStartSoundAtVolume(lua_State *L)
{
	const void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	INT32 volume = (INT32)luaL_checkinteger(L, 3);
	player_t *player = NULL;
	NOHUD

	if (!lua_isnil(L, 1))
	{
		origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (!origin)
			return LUA_ErrInvalid(L, "mobj_t");
	}
	if (sound_id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", sound_id, NUMSFX-1);
	if (!lua_isnone(L, 4) && lua_isuserdata(L, 4))
	{
		player = *((player_t **)luaL_checkudata(L, 4, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
	S_StartSoundAtVolume(origin, sound_id, volume);
	return 0;
}

static int lib_sStopSound(lua_State *L)
{
	void *origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!origin)
		return LUA_ErrInvalid(L, "mobj_t");
	S_StopSound(origin);
	return 0;
}

static int lib_sChangeMusic(lua_State *L)
{
#ifdef MUSICSLOT_COMPATIBILITY
	const char *music_name;
	UINT32 music_num, position, prefadems, fadeinms;
	char music_compat_name[7];

	boolean looping;
	player_t *player = NULL;
	UINT16 music_flags = 0;
	NOHUD

	if (lua_isnumber(L, 1))
	{
		music_num = (UINT32)luaL_checkinteger(L, 1);
		music_flags = (UINT16)(music_num & 0x0000FFFF);
		if (music_flags && music_flags <= 1035)
			snprintf(music_compat_name, 7, "%sM", G_BuildMapName((INT32)music_flags));
		else if (music_flags && music_flags <= 1050)
			strncpy(music_compat_name, compat_special_music_slots[music_flags - 1036], 7);
		else
			music_compat_name[0] = 0; // becomes empty string
		music_compat_name[6] = 0;
		music_name = (const char *)&music_compat_name;
		music_flags = 0;
	}
	else
	{
		music_num = 0;
		music_name = luaL_checkstring(L, 1);
	}

	looping = (boolean)lua_opttrueboolean(L, 2);

#else
	const char *music_name = luaL_checkstring(L, 1);
	boolean looping = (boolean)lua_opttrueboolean(L, 2);
	player_t *player = NULL;
	UINT16 music_flags = 0;
	NOHUD

#endif
	if (!lua_isnone(L, 3) && lua_isuserdata(L, 3))
	{
		player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}

#ifdef MUSICSLOT_COMPATIBILITY
	if (music_num)
		music_flags = (UINT16)((music_num & 0x7FFF0000) >> 16);
	else
#endif
	music_flags = (UINT16)luaL_optinteger(L, 4, 0);

	position = (UINT32)luaL_optinteger(L, 5, 0);
	prefadems = (UINT32)luaL_optinteger(L, 6, 0);
	fadeinms = (UINT32)luaL_optinteger(L, 7, 0);

	if (!player || P_IsLocalPlayer(player))
	{
		S_ChangeMusicAdvanced(music_name, music_flags, looping, position, prefadems, fadeinms);
		lua_pushboolean(L, true);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sSpeedMusic(lua_State *L)
{
	fixed_t fixedspeed = luaL_checkfixed(L, 1);
	float speed = FIXED_TO_FLOAT(fixedspeed);
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushboolean(L, S_SpeedMusic(speed));
	else
		lua_pushnil(L);
	return 1;
}

#ifdef HAVE_LUA_MUSICPLUS
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

static int lib_sMusicInfo(lua_State *L)
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
		char mname[7];
		UINT16 mflags;
		boolean looping;
		if (S_MusicInfo(mname, &mflags, &looping))
		{
			lua_pushstring(L, mname);
			lua_pushinteger(L, mflags);
			lua_pushboolean(L, looping);
		}
		else
			lua_pushboolean(L, false);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sMusicExists(lua_State *L)
{
#ifdef MUSICSLOT_COMPATIBILITY
	const char *music_name;
	UINT32 music_num;
	char music_compat_name[7];
	UINT16 music_flags = 0;
	NOHUD
	if (lua_isnumber(L, 1))
	{
		music_num = (UINT32)luaL_checkinteger(L, 1);
		music_flags = (UINT16)(music_num & 0x0000FFFF);
		if (music_flags && music_flags <= 1035)
			snprintf(music_compat_name, 7, "%sM", G_BuildMapName((INT32)music_flags));
		else if (music_flags && music_flags <= 1050)
			strncpy(music_compat_name, compat_special_music_slots[music_flags - 1036], 7);
		else
			music_compat_name[0] = 0; // becomes empty string
		music_compat_name[6] = 0;
		music_name = (const char *)&music_compat_name;
	}
	else
	{
		music_num = 0;
		music_name = luaL_checkstring(L, 1);
	}
#else
	const char *music_name = luaL_checkstring(L, 1);
#endif
	boolean checkMIDI = lua_opttrueboolean(L, 2);
	boolean checkDigi = lua_opttrueboolean(L, 3);
	NOHUD
	lua_pushboolean(L, S_MusicExists(music_name, checkMIDI, checkDigi));
	return 1;
}

static int lib_sGetMusicLength(lua_State *L)
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
		lua_pushinteger(L, (int)S_GetMusicLength());
	else
		lua_pushnil(L);
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

static int lib_sSetMusicPosition(lua_State *L)
{
	UINT32 position = (UINT32)luaL_checkinteger(L, 1);
	player_t *player = NULL;
	NOHUD
	if (!lua_isnone(L, 2) && lua_isuserdata(L, 2))
	{
		player = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
		if (!player)
			return LUA_ErrInvalid(L, "player_t");
	}
	if (!player || P_IsLocalPlayer(player))
		lua_pushboolean(L, S_SetMusicPosition(position));
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sGetMusicPosition(lua_State *L)
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
		lua_pushinteger(L, (int)S_GetMusicPosition());
	else
		lua_pushnil(L);
	return 1;
}

static int lib_sStopMusic(lua_State *L)
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
		S_StopMusic();
		lua_pushboolean(L, true);
	}
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

static int lib_sSetInternalMusicVolume(lua_State *L)
{
	UINT32 volume = (UINT32)luaL_checkinteger(L, 1);
	player_t *player = NULL;
	NOHUD
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
	NOHUD
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
	NOHUD
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
	else if (luaL_optinteger(L, 3, UINT32_MAX) == UINT32_MAX)
	{
		ms = (UINT32)luaL_checkinteger(L, 2);
		source_volume = -1;
	}
	else
	{
		source_volume = (INT32)luaL_checkinteger(L, 2);
		ms = (UINT32)luaL_checkinteger(L, 3);
	}

	NOHUD

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
	NOHUD
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

#endif
static int lib_sOriginPlaying(lua_State *L)
{
	void *origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	INLEVEL
	if (!origin)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, S_OriginPlaying(origin));
	return 1;
}

static int lib_sIdPlaying(lua_State *L)
{
	sfxenum_t id = luaL_checkinteger(L, 1);
	NOHUD
	if (id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", id, NUMSFX-1);
	lua_pushboolean(L, S_IdPlaying(id));
	return 1;
}

static int lib_sSoundPlaying(lua_State *L)
{
	void *origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	sfxenum_t id = luaL_checkinteger(L, 2);
	NOHUD
	INLEVEL
	if (!origin)
		return LUA_ErrInvalid(L, "mobj_t");
	if (id >= NUMSFX)
		return luaL_error(L, "sfx %d out of range (0 - %d)", id, NUMSFX-1);
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
	INLEVEL

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

// G_GAME
////////////

static int lib_gBuildMapName(lua_State *L)
{
	INT32 map = luaL_optinteger(L, 1, gamemap);
	//HUDSAFE
	INLEVEL
	lua_pushstring(L, G_BuildMapName(map));
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

static int lib_gExitLevel(lua_State *L)
{
	int n = lua_gettop(L); // Num arguments
	NOHUD
	INLEVEL

	// LUA EXTENSION: Custom exit like support
	// Supported:
	//	G_ExitLevel();			[no modifications]
	//	G_ExitLevel(int)		[nextmap override only]
	//	G_ExitLevel(bool)		[skipstats only]
	//	G_ExitLevel(int, bool)	[both of the above]
	if (n >= 1)
	{
		if (lua_isnumber(L, 1) || n >= 2)
		{
			nextmapoverride = (INT16)luaL_checknumber(L, 1);
			lua_pop(L, 1); // pop nextmapoverride; skipstats now 1 if available
		}
		skipstats = lua_optboolean(L, 1);
	}
	// ---

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

static int lib_gTagGametype(lua_State *L)
{
	//HUDSAFE
	INLEVEL
	lua_pushboolean(L, G_TagGametype());
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

static luaL_Reg lib[] = {
	{"print", lib_print},
	{"userdataType", lib_userdataType},

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
	{"P_IsObjectInGoop",lib_pIsObjectInGoop},
	{"P_IsObjectOnGround",lib_pIsObjectOnGround},
	{"P_InSpaceSector",lib_pInSpaceSector},
	{"P_InQuicksand",lib_pInQuicksand},
	{"P_SetObjectMomZ",lib_pSetObjectMomZ},
	{"P_RestoreMusic",lib_pRestoreMusic},
	{"P_SpawnShieldOrb",lib_pSpawnShieldOrb},
	{"P_SpawnGhostMobj",lib_pSpawnGhostMobj},
	{"P_GivePlayerRings",lib_pGivePlayerRings},
	{"P_GivePlayerLives",lib_pGivePlayerLives},
	{"P_GiveCoopLives",lib_pGiveCoopLives},
	{"P_ResetScore",lib_pResetScore},
	{"P_DoJumpShield",lib_pDoJumpShield},
	{"P_DoBubbleBounce",lib_pDoBubbleBounce},
	{"P_BlackOw",lib_pBlackOw},
	{"P_ElementalFire",lib_pElementalFire},
	{"P_DoPlayerExit",lib_pDoPlayerExit},
	{"P_InstaThrust",lib_pInstaThrust},
	{"P_ReturnThrustX",lib_pReturnThrustX},
	{"P_ReturnThrustY",lib_pReturnThrustY},
	{"P_LookForEnemies",lib_pLookForEnemies},
	{"P_NukeEnemies",lib_pNukeEnemies},
	{"P_HomingAttack",lib_pHomingAttack},
	{"P_SuperReady",lib_pSuperReady},
	{"P_DoJump",lib_pDoJump},
	{"P_SpawnThokMobj",lib_pSpawnThokMobj},
	{"P_SpawnSpinMobj",lib_pSpawnSpinMobj},
	{"P_Telekinesis",lib_pTelekinesis},
	{"P_SwitchShield",lib_pSwitchShield},

	// p_map
	{"P_CheckPosition",lib_pCheckPosition},
	{"P_TryMove",lib_pTryMove},
	{"P_Move",lib_pMove},
	{"P_TeleportMove",lib_pTeleportMove},
	{"P_SlideMove",lib_pSlideMove},
	{"P_BounceMove",lib_pBounceMove},
	{"P_CheckSight", lib_pCheckSight},
	{"P_CheckHoopPosition",lib_pCheckHoopPosition},
	{"P_RadiusAttack",lib_pRadiusAttack},
	{"P_FloorzAtPos",lib_pFloorzAtPos},
	{"P_DoSpring",lib_pDoSpring},

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
	{"P_SetMobjStateNF",lib_pSetMobjStateNF},
	{"P_DoSuperTransformation",lib_pDoSuperTransformation},
	{"P_ExplodeMissile",lib_pExplodeMissile},
	{"P_PlayerTouchingSectorSpecial",lib_pPlayerTouchingSectorSpecial},
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
	{"P_ThingOnSpecial3DFloor",lib_pThingOnSpecial3DFloor},
	{"P_IsFlagAtBase",lib_pIsFlagAtBase},
	{"P_SetupLevelSky",lib_pSetupLevelSky},
	{"P_SetSkyboxMobj",lib_pSetSkyboxMobj},
	{"P_StartQuake",lib_pStartQuake},
	{"EV_CrumbleChain",lib_evCrumbleChain},
	{"EV_StartCrumble",lib_evStartCrumble},

	// r_defs
	{"R_PointToAngle",lib_rPointToAngle},
	{"R_PointToAngle2",lib_rPointToAngle2},
	{"R_PointToDist",lib_rPointToDist},
	{"R_PointToDist2",lib_rPointToDist2},
	{"R_PointInSubsector",lib_rPointInSubsector},

	// r_things (sprite)
	{"R_Char2Frame",lib_rChar2Frame},
	{"R_Frame2Char",lib_rFrame2Char},
	{"R_SetPlayerSkin",lib_rSetPlayerSkin},

	// s_sound
	{"S_StartSound",lib_sStartSound},
	{"S_StartSoundAtVolume",lib_sStartSoundAtVolume},
	{"S_StopSound",lib_sStopSound},
	{"S_ChangeMusic",lib_sChangeMusic},
	{"S_SpeedMusic",lib_sSpeedMusic},
#ifdef HAVE_LUA_MUSICPLUS
	{"S_MusicType",lib_sMusicType},
	{"S_MusicPlaying",lib_sMusicPlaying},
	{"S_MusicPaused",lib_sMusicPaused},
	{"S_MusicInfo",lib_sMusicInfo},
	{"S_MusicExists",lib_sMusicExists},
	{"S_GetMusicLength",lib_sGetMusicLength},
	{"S_SetMusicLoopPoint",lib_sSetMusicLoopPoint},
	{"S_GetMusicLoopPoint",lib_sGetMusicLoopPoint},
	{"S_SetMusicPosition",lib_sSetMusicPosition},
	{"S_GetMusicPosition",lib_sGetMusicPosition},
	{"S_PauseMusic",lib_sPauseMusic},
	{"S_ResumeMusic",lib_sResumeMusic},
	{"S_StopMusic",lib_sStopMusic},
	{"S_SetInternalMusicVolume", lib_sSetInternalMusicVolume},
	{"S_StopFadingMusic",lib_sStopFadingMusic},
	{"S_FadeMusic",lib_sFadeMusic},
	{"S_FadeOutStopMusic",lib_sFadeOutStopMusic},
#endif
	{"S_OriginPlaying",lib_sOriginPlaying},
	{"S_IdPlaying",lib_sIdPlaying},
	{"S_SoundPlaying",lib_sSoundPlaying},
	{"S_StartMusicCaption", lib_sStartMusicCaption},

	// g_game
	{"G_BuildMapName",lib_gBuildMapName},
	{"G_DoReborn",lib_gDoReborn},
	{"G_ExitLevel",lib_gExitLevel},
	{"G_IsSpecialStage",lib_gIsSpecialStage},
	{"G_GametypeUsesLives",lib_gGametypeUsesLives},
	{"G_GametypeHasTeams",lib_gGametypeHasTeams},
	{"G_GametypeHasSpectators",lib_gGametypeHasSpectators},
	{"G_RingSlingerGametype",lib_gRingSlingerGametype},
	{"G_PlatformGametype",lib_gPlatformGametype},
	{"G_TagGametype",lib_gTagGametype},
	{"G_TicsToHours",lib_gTicsToHours},
	{"G_TicsToMinutes",lib_gTicsToMinutes},
	{"G_TicsToSeconds",lib_gTicsToSeconds},
	{"G_TicsToCentiseconds",lib_gTicsToCentiseconds},
	{"G_TicsToMilliseconds",lib_gTicsToMilliseconds},

	{NULL, NULL}
};

int LUA_BaseLib(lua_State *L)
{
	// Set metatable for string
	lua_pushliteral(L, "");  // dummy string
	lua_getmetatable(L, -1);  // get string metatable
	lua_pushcfunction(L,lib_concat); // push concatination function
	lua_setfield(L,-2,"__add"); // ... store it as mathematical addition
	lua_pop(L, 2); // pop metatable and dummy string

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);

	// Set global functions
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}

#endif
