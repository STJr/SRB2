// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
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

#define NOHUD if (hud_running) return luaL_error(L, "HUD rendering code should not call this function!");


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

// M_RANDOM
//////////////

static int lib_pRandom(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_Random());
	return 1;
}

static int lib_pSignedRandom(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_SignedRandom());
	return 1;
}

static int lib_pRandomKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);

	NOHUD
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
	lua_pushinteger(L, P_RandomRange(a, b));
	return 1;
}

// P_MAPUTIL
///////////////

static int lib_pAproxDistance(lua_State *L)
{
	fixed_t dx = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t dy = (fixed_t)luaL_checkinteger(L, 2);
	//HUDSAFE
	lua_pushinteger(L, P_AproxDistance(dx, dy));
	return 1;
}

static int lib_pClosestPointOnLine(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	line_t *line = *((line_t **)luaL_checkudata(L, 3, META_LINE));
	vertex_t result;
	//HUDSAFE
	if (!line)
		return LUA_ErrInvalid(L, "line_t");
	P_ClosestPointOnLine(x, y, line, &result);
	lua_pushinteger(L, result.x);
	lua_pushinteger(L, result.y);
	return 2;
}

// P_ENEMY
/////////////

static int lib_pCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckMeleeRange(actor));
	return 1;
}

static int lib_pJetbCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_JetbCheckMeleeRange(actor));
	return 1;
}

static int lib_pFaceStabCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_FaceStabCheckMeleeRange(actor));
	return 1;
}

static int lib_pSkimCheckMeleeRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SkimCheckMeleeRange(actor));
	return 1;
}

static int lib_pCheckMissileRange(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckMissileRange(actor));
	return 1;
}

static int lib_pNewChaseDir(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
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
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_LookForPlayers(actor, allaround, tracer, dist));
	return 1;
}

// P_MOBJ
////////////

static int lib_pSpawnMobj(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 3);
	mobjtype_t type = luaL_checkinteger(L, 4);
	NOHUD
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SpawnMobj(x, y, z, type), META_MOBJ);
	return 1;
}

static int lib_pRemoveMobj(lua_State *L)
{
	mobj_t *th = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!th)
		return LUA_ErrInvalid(L, "mobj_t");
	if (th->player)
		return luaL_error(L, "Attempt to remove player mobj with P_RemoveMobj.");
	P_RemoveMobj(th);
	return 0;
}

static int lib_pSpawnMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *dest = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 3);
	NOHUD
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SpawnMissile(source, dest, type), META_MOBJ);
	return 1;
}

static int lib_pSpawnXYZMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *dest = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 3);
	fixed_t x = (fixed_t)luaL_checkinteger(L, 4);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 5);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 6);
	NOHUD
	if (!source || !dest)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SpawnXYZMissile(source, dest, type, x, y, z), META_MOBJ);
	return 1;
}

static int lib_pSpawnPointMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t xa = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t ya = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t za = (fixed_t)luaL_checkinteger(L, 4);
	mobjtype_t type = luaL_checkinteger(L, 5);
	fixed_t x = (fixed_t)luaL_checkinteger(L, 6);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 7);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 8);
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SpawnPointMissile(source, xa, ya, za, type, x, y, z), META_MOBJ);
	return 1;
}

static int lib_pSpawnAlteredDirectionMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	fixed_t x = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 4);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 5);
	INT32 shiftingAngle = (INT32)luaL_checkinteger(L, 5);
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SpawnAlteredDirectionMissile(source, type, x, y, z, shiftingAngle), META_MOBJ);
	return 1;
}

static int lib_pColorTeamMissile(lua_State *L)
{
	mobj_t *missile = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	player_t *source = *((player_t **)luaL_checkudata(L, 2, META_PLAYER));
	NOHUD
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
	angle_t angle = (angle_t)luaL_checkinteger(L, 3);
	UINT8 allowaim = (UINT8)luaL_optinteger(L, 4, 0);
	UINT32 flags2 = (UINT32)luaL_optinteger(L, 5, 0);
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SPMAngle(source, type, angle, allowaim, flags2), META_MOBJ);
	return 1;
}

static int lib_pSpawnPlayerMissile(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobjtype_t type = luaL_checkinteger(L, 2);
	UINT32 flags2 = (UINT32)luaL_optinteger(L, 3, 0);
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	LUA_PushUserdata(L, P_SpawnPlayerMissile(source, type, flags2), META_MOBJ);
	return 1;
}

static int lib_pMobjFlip(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushinteger(L, P_MobjFlip(mobj));
	return 1;
}

static int lib_pWeaponOrPanel(lua_State *L)
{
	mobjtype_t type = luaL_checkinteger(L, 1);
	//HUDSAFE
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	lua_pushboolean(L, P_WeaponOrPanel(type));
	return 1;
}

static int lib_pFlashPal(lua_State *L)
{
	player_t *pl = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	UINT16 type = (UINT16)luaL_checkinteger(L, 2);
	UINT16 duration = (UINT16)luaL_checkinteger(L, 3);
	NOHUD
	if (!pl)
		return LUA_ErrInvalid(L, "player_t");
	P_FlashPal(pl, type, duration);
	return 0;
}

static int lib_pGetClosestAxis(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_GetClosestAxis(source), META_MOBJ);
	return 1;
}

static int lib_pSpawnParaloop(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t radius = (fixed_t)luaL_checkinteger(L, 4);
	INT32 number = (INT32)luaL_checkinteger(L, 5);
	mobjtype_t type = luaL_checkinteger(L, 6);
	angle_t rotangle = (angle_t)luaL_checkinteger(L, 7);
	statenum_t nstate = luaL_optinteger(L, 8, S_NULL);
	boolean spawncenter = lua_optboolean(L, 9);
	NOHUD
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	P_SpawnParaloop(x, y, z, radius, number, type, nstate, rotangle, spawncenter);
	return 0;
}

static int lib_pBossTargetPlayer(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	boolean closest = lua_optboolean(L, 2);
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_BossTargetPlayer(actor, closest));
	return 1;
}

static int lib_pSupermanLook4Players(lua_State *L)
{
	mobj_t *actor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!actor)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_SupermanLook4Players(actor));
	return 1;
}

static int lib_pSetScale(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t newscale = (fixed_t)luaL_checkinteger(L, 2);
	NOHUD
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
	if (newscale < FRACUNIT/100)
		newscale = FRACUNIT/100;
	P_SetScale(mobj, newscale);
	return 0;
}

static int lib_pCheckDeathPitCollide(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckDeathPitCollide(mo));
	return 1;
}

// P_USER
////////////

static int lib_pGetPlayerHeight(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, P_GetPlayerHeight(player));
	return 1;
}

static int lib_pGetPlayerSpinHeight(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushinteger(L, P_GetPlayerSpinHeight(player));
	return 1;
}

static int lib_pGetPlayerControlDirection(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_AddPlayerScore(player, amount);
	return 0;
}

static int lib_pPlayerInPain(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (!lua_isnone(L, 2) && lua_touserdata(L, 2))
		source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_touserdata(L, 3))
		inflictor = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	P_DoPlayerPain(player, source, inflictor);
	return 0;
}

static int lib_pResetPlayer(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ResetPlayer(player);
	return 0;
}

static int lib_pIsObjectOnGround(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	//HUDSAFE
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_IsObjectOnGround(mo));
	return 1;
}

static int lib_pSetObjectMomZ(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t value = (fixed_t)luaL_checkinteger(L, 2);
	boolean relative = lua_optboolean(L, 3);
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SetObjectMomZ(mo, value, relative);
	return 0;
}

static int lib_pRestoreMusic(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_RestoreMusic(player);
	return 0;
}

static int lib_pSpawnGhostMobj(lua_State *L)
{
	mobj_t *mobj = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_GivePlayerLives(player, numlives);
	return 0;
}

static int lib_pResetScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ResetScore(player);
	return 0;
}

static int lib_pBlackOw(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_BlackOw(player);
	return 0;
}

static int lib_pElementalFireTrail(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_ElementalFireTrail(player);
	return 0;
}

static int lib_pDoPlayerExit(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoPlayerExit(player);
	return 0;
}

static int lib_pInstaThrust(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = (angle_t)luaL_checkinteger(L, 2);
	fixed_t move = (fixed_t)luaL_checkinteger(L, 3);
	NOHUD
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
	angle = (angle_t)luaL_checkinteger(L, 1);
	move = (fixed_t)luaL_checkinteger(L, 2);
	//HUDSAFE
	lua_pushinteger(L, P_ReturnThrustX(NULL, angle, move));
	return 1;
}

static int lib_pReturnThrustY(lua_State *L)
{
	angle_t angle;
	fixed_t move;
	if (lua_isnil(L, 1) || lua_isuserdata(L, 1))
		lua_remove(L, 1); // ignore mobj as arg1
	angle = (angle_t)luaL_checkinteger(L, 1);
	move = (fixed_t)luaL_checkinteger(L, 2);
	//HUDSAFE
	lua_pushinteger(L, P_ReturnThrustY(NULL, angle, move));
	return 1;
}

static int lib_pLookForEnemies(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_LookForEnemies(player));
	return 1;
}

static int lib_pNukeEnemies(lua_State *L)
{
	mobj_t *inflictor = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t radius = (fixed_t)luaL_checkinteger(L, 3);
	NOHUD
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
	if (!source || !enemy)
		return LUA_ErrInvalid(L, "mobj_t");
	P_HomingAttack(source, enemy);
	return 0;
}

static int lib_pSuperReady(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	//HUDSAFE
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoJump(player, soundandstate);
	return 0;
}

static int lib_pSpawnThokMobj(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (type > MT_LASTFREESLOT)
		return luaL_error(L, "mobjtype_t out of bounds error!");
	P_SpawnSpinMobj(player, type);
	return 0;
}


// P_MAP
///////////

static int lib_pCheckPosition(lua_State *L)
{
	mobj_t *ptmthing = tmthing;
	mobj_t *thing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 3);
	NOHUD
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
	fixed_t x = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 3);
	boolean allowdropoff = lua_optboolean(L, 4);
	NOHUD
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
	fixed_t speed = (fixed_t)luaL_checkinteger(L, 2);
	NOHUD
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
	fixed_t x = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 4);
	NOHUD
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
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	P_SlideMove(mo);
	return 0;
}

static int lib_pBounceMove(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
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
	if (!t1 || !t2)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, P_CheckSight(t1, t2));
	return 1;
}

static int lib_pCheckHoopPosition(lua_State *L)
{
	mobj_t *hoopthing = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	fixed_t x = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 4);
	fixed_t radius = (fixed_t)luaL_checkinteger(L, 5);
	NOHUD
	if (!hoopthing)
		return LUA_ErrInvalid(L, "mobj_t");
	P_CheckHoopPosition(hoopthing, x, y, z, radius);
	return 0;
}

static int lib_pRadiusAttack(lua_State *L)
{
	mobj_t *spot = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	fixed_t damagedist = (fixed_t)luaL_checkinteger(L, 3);
	NOHUD
	if (!spot || !source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_RadiusAttack(spot, source, damagedist);
	return 0;
}

static int lib_pFloorzAtPos(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t z = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t height = (fixed_t)luaL_checkinteger(L, 4);
	//HUDSAFE
	lua_pushinteger(L, P_FloorzAtPos(x, y, z, height));
	return 1;
}

static int lib_pDoSpring(lua_State *L)
{
	mobj_t *spring = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	mobj_t *object = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	NOHUD
	if (!spring || !object)
		return LUA_ErrInvalid(L, "mobj_t");
	P_DoSpring(spring, object);
	return 0;
}

// P_INTER
////////////

static int lib_pRemoveShield(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_RemoveShield(player);
	return 0;
}

static int lib_pDamageMobj(lua_State *L)
{
	mobj_t *target = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)), *inflictor = NULL, *source = NULL;
	INT32 damage;
	NOHUD
	if (!target)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_touserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_touserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	damage = (INT32)luaL_optinteger(L, 4, 1);
	lua_pushboolean(L, P_DamageMobj(target, inflictor, source, damage));
	return 1;
}

static int lib_pKillMobj(lua_State *L)
{
	mobj_t *target = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ)), *inflictor = NULL, *source = NULL;
	NOHUD
	if (!target)
		return LUA_ErrInvalid(L, "mobj_t");
	if (!lua_isnone(L, 2) && lua_touserdata(L, 2))
		inflictor = *((mobj_t **)luaL_checkudata(L, 2, META_MOBJ));
	if (!lua_isnone(L, 3) && lua_touserdata(L, 3))
		source = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
	P_KillMobj(target, inflictor, source);
	return 0;
}

static int lib_pPlayerRingBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	INT32 num_rings = (INT32)luaL_optinteger(L, 2, -1);
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	if (num_rings == -1)
		num_rings = player->health - 1;
	P_PlayerRingBurst(player, num_rings);
	return 0;
}

static int lib_pPlayerWeaponPanelBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerWeaponPanelBurst(player);
	return 0;
}

static int lib_pPlayerWeaponAmmoBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerWeaponAmmoBurst(player);
	return 0;
}

static int lib_pPlayerEmeraldBurst(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	boolean toss = lua_optboolean(L, 2);
	NOHUD
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_PlayerFlagBurst(player, toss);
	return 0;
}

static int lib_pPlayRinglossSound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_PlayRinglossSound(source);
	return 0;
}

static int lib_pPlayDeathSound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_PlayDeathSound(source);
	return 0;
}

static int lib_pPlayVictorySound(lua_State *L)
{
	mobj_t *source = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!source)
		return LUA_ErrInvalid(L, "mobj_t");
	P_PlayVictorySound(source);
	return 0;
}

static int lib_pPlayLivesJingle(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	lua_pushboolean(L, P_CanPickupItem(player, weapon));
	return 1;
}

static int lib_pDoNightsScore(lua_State *L)
{
	player_t *player = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	NOHUD
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoNightsScore(player);
	return 0;
}

// P_SPEC
////////////

static int lib_pThrust(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	angle_t angle = (angle_t)luaL_checkinteger(L, 2);
	fixed_t move = (fixed_t)luaL_checkinteger(L, 3);
	NOHUD
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
	if (!mobj)
		return LUA_ErrInvalid(L, "mobj_t");
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	P_DoSuperTransformation(player, giverings);
	return 0;
}

static int lib_pExplodeMissile(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
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
	if (!player)
		return LUA_ErrInvalid(L, "player_t");
	LUA_PushUserdata(L, P_PlayerTouchingSectorSpecial(player, section, number), META_SECTOR);
	return 1;
}

static int lib_pFindSpecialLineFromTag(lua_State *L)
{
	INT16 special = (INT16)luaL_checkinteger(L, 1);
	INT16 line = (INT16)luaL_checkinteger(L, 2);
	INT32 start = (INT32)luaL_optinteger(L, 3, -1);
	NOHUD
	lua_pushinteger(L, P_FindSpecialLineFromTag(special, line, start));
	return 1;
}

static int lib_pSwitchWeather(lua_State *L)
{
	INT32 weathernum = (INT32)luaL_checkinteger(L, 1);
	NOHUD
	P_SwitchWeather(weathernum);
	return 0;
}

static int lib_pLinedefExecute(lua_State *L)
{
	INT32 tag = (INT16)luaL_checkinteger(L, 1);
	mobj_t *actor = NULL;
	sector_t *caller = NULL;
	NOHUD
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
	P_FadeLight(tag, destvalue, speed);
	return 0;
}

static int lib_pThingOnSpecial3DFloor(lua_State *L)
{
	mobj_t *mo = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!mo)
		return LUA_ErrInvalid(L, "mobj_t");
	LUA_PushUserdata(L, P_ThingOnSpecial3DFloor(mo), META_SECTOR);
	return 1;
}

static int lib_pSetupLevelSky(lua_State *L)
{
	INT32 skynum = (INT32)luaL_checkinteger(L, 1);
	boolean global = lua_optboolean(L, 2);
	NOHUD
	P_SetupLevelSky(skynum, global);
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

// R_DEFS
////////////

static int lib_rPointToAngle(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	//HUDSAFE
	lua_pushinteger(L, R_PointToAngle(x, y));
	return 1;
}

static int lib_rPointToAngle2(lua_State *L)
{
	fixed_t px2 = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t py2 = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t px1 = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t py1 = (fixed_t)luaL_checkinteger(L, 4);
	//HUDSAFE
	lua_pushinteger(L, R_PointToAngle2(px2, py2, px1, py1));
	return 1;
}

static int lib_rPointToDist(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	//HUDSAFE
	lua_pushinteger(L, R_PointToDist(x, y));
	return 1;
}

static int lib_rPointToDist2(lua_State *L)
{
	fixed_t px2 = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t py2 = (fixed_t)luaL_checkinteger(L, 2);
	fixed_t px1 = (fixed_t)luaL_checkinteger(L, 3);
	fixed_t py1 = (fixed_t)luaL_checkinteger(L, 4);
	//HUDSAFE
	lua_pushinteger(L, R_PointToDist2(px2, py2, px1, py1));
	return 1;
}

static int lib_rPointInSubsector(lua_State *L)
{
	fixed_t x = (fixed_t)luaL_checkinteger(L, 1);
	fixed_t y = (fixed_t)luaL_checkinteger(L, 2);
	//HUDSAFE
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

// S_SOUND
////////////

static int lib_sStartSound(lua_State *L)
{
	const void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	NOHUD
	if (!lua_isnil(L, 1))
	{
		origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (!origin)
			return LUA_ErrInvalid(L, "mobj_t");
	}
	S_StartSound(origin, sound_id);
	return 0;
}

static int lib_sStartSoundAtVolume(lua_State *L)
{
	const void *origin = NULL;
	sfxenum_t sound_id = luaL_checkinteger(L, 2);
	INT32 volume = (INT32)luaL_checkinteger(L, 3);
	NOHUD
	if (!lua_isnil(L, 1))
	{
		origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
		if (!origin)
			return LUA_ErrInvalid(L, "mobj_t");
	}
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
	UINT32 music_num = (UINT32)luaL_checkinteger(L, 1);
	boolean looping = (boolean)lua_opttrueboolean(L, 2);
	NOHUD
	S_ChangeMusic(music_num, looping);
	return 0;
}

static int lib_sSpeedMusic(lua_State *L)
{
	fixed_t fixedspeed = (fixed_t)luaL_checkinteger(L, 1);
	float speed = FIXED_TO_FLOAT(fixedspeed);
	NOHUD
	lua_pushboolean(L, S_SpeedMusic(speed));
	return 1;
}

static int lib_sStopMusic(lua_State *L)
{
	NOHUD
	S_StopMusic();
	return 0;
}

static int lib_sOriginPlaying(lua_State *L)
{
	void *origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	NOHUD
	if (!origin)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, S_OriginPlaying(origin));
	return 1;
}

static int lib_sIdPlaying(lua_State *L)
{
	sfxenum_t id = luaL_checkinteger(L, 1);
	NOHUD
	lua_pushboolean(L, S_IdPlaying(id));
	return 1;
}

static int lib_sSoundPlaying(lua_State *L)
{
	void *origin = *((mobj_t **)luaL_checkudata(L, 1, META_MOBJ));
	sfxenum_t id = luaL_checkinteger(L, 2);
	NOHUD
	if (!origin)
		return LUA_ErrInvalid(L, "mobj_t");
	lua_pushboolean(L, S_SoundPlaying(origin, id));
	return 1;
}

// G_GAME
////////////

static int lib_gBuildMapName(lua_State *L)
{
	INT32 map = luaL_optinteger(L, 1, gamemap);
	//HUDSAFE
	lua_pushstring(L, G_BuildMapName(map));
	return 1;
}

static int lib_gDoReborn(lua_State *L)
{
	INT32 playernum = luaL_checkinteger(L, 1);
	NOHUD
	if (playernum >= MAXPLAYERS)
		return luaL_error(L, "playernum out of bounds error!");
	G_DoReborn(playernum);
	return 0;
}

static int lib_gExitLevel(lua_State *L)
{
	NOHUD
	G_ExitLevel();
	return 0;
}

static int lib_gIsSpecialStage(lua_State *L)
{
	INT32 mapnum = luaL_optinteger(L, 1, gamemap);
	//HUDSAFE
	lua_pushboolean(L, G_IsSpecialStage(mapnum));
	return 1;
}

static int lib_gGametypeUsesLives(lua_State *L)
{
	//HUDSAFE
	lua_pushboolean(L, G_GametypeUsesLives());
	return 1;
}

static int lib_gGametypeHasTeams(lua_State *L)
{
	//HUDSAFE
	lua_pushboolean(L, G_GametypeHasTeams());
	return 1;
}

static int lib_gGametypeHasSpectators(lua_State *L)
{
	//HUDSAFE
	lua_pushboolean(L, G_GametypeHasSpectators());
	return 1;
}

static int lib_gRingSlingerGametype(lua_State *L)
{
	//HUDSAFE
	lua_pushboolean(L, G_RingSlingerGametype());
	return 1;
}

static int lib_gPlatformGametype(lua_State *L)
{
	//HUDSAFE
	lua_pushboolean(L, G_PlatformGametype());
	return 1;
}

static int lib_gTagGametype(lua_State *L)
{
	//HUDSAFE
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

	// m_random
	{"P_Random",lib_pRandom},
	{"P_SignedRandom",lib_pSignedRandom},
	{"P_RandomKey",lib_pRandomKey},
	{"P_RandomRange",lib_pRandomRange},

	// p_maputil
	{"P_AproxDistance",lib_pAproxDistance},
	{"P_ClosestPointOnLine",lib_pClosestPointOnLine},

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
	{"P_RemoveMobj",lib_pRemoveMobj},
	{"P_SpawnMissile",lib_pSpawnMissile},
	{"P_SpawnXYZMissile",lib_pSpawnXYZMissile},
	{"P_SpawnPointMissile",lib_pSpawnPointMissile},
	{"P_SpawnAlteredDirectionMissile",lib_pSpawnAlteredDirectionMissile},
	{"P_ColorTeamMissile",lib_pColorTeamMissile},
	{"P_SPMAngle",lib_pSPMAngle},
	{"P_SpawnPlayerMissile",lib_pSpawnPlayerMissile},
	{"P_MobjFlip",lib_pMobjFlip},
	{"P_WeaponOrPanel",lib_pWeaponOrPanel},
	{"P_FlashPal",lib_pFlashPal},
	{"P_GetClosestAxis",lib_pGetClosestAxis},
	{"P_SpawnParaloop",lib_pSpawnParaloop},
	{"P_BossTargetPlayer",lib_pBossTargetPlayer},
	{"P_SupermanLook4Players",lib_pSupermanLook4Players},
	{"P_SetScale",lib_pSetScale},
	{"P_CheckDeathPitCollide",lib_pCheckDeathPitCollide},

	// p_user
	{"P_GetPlayerHeight",lib_pGetPlayerHeight},
	{"P_GetPlayerSpinHeight",lib_pGetPlayerSpinHeight},
	{"P_GetPlayerControlDirection",lib_pGetPlayerControlDirection},
	{"P_AddPlayerScore",lib_pAddPlayerScore},
	{"P_PlayerInPain",lib_pPlayerInPain},
	{"P_DoPlayerPain",lib_pDoPlayerPain},
	{"P_ResetPlayer",lib_pResetPlayer},
	{"P_IsObjectOnGround",lib_pIsObjectOnGround},
	{"P_SetObjectMomZ",lib_pSetObjectMomZ},
	{"P_RestoreMusic",lib_pRestoreMusic},
	{"P_SpawnGhostMobj",lib_pSpawnGhostMobj},
	{"P_GivePlayerRings",lib_pGivePlayerRings},
	{"P_GivePlayerLives",lib_pGivePlayerLives},
	{"P_ResetScore",lib_pResetScore},
	{"P_BlackOw",lib_pBlackOw},
	{"P_ElementalFireTrail",lib_pElementalFireTrail},
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
	{"P_PlayerEmeraldBurst",lib_pPlayerEmeraldBurst},
	{"P_PlayerFlagBurst",lib_pPlayerFlagBurst},
	{"P_PlayRinglossSound",lib_pPlayRinglossSound},
	{"P_PlayDeathSound",lib_pPlayDeathSound},
	{"P_PlayVictorySound",lib_pPlayVictorySound},
	{"P_PlayLivesJingle",lib_pPlayLivesJingle},
	{"P_CanPickupItem",lib_pCanPickupItem},
	{"P_DoNightsScore",lib_pDoNightsScore},

	// p_spec
	{"P_Thrust",lib_pThrust},
	{"P_SetMobjStateNF",lib_pSetMobjStateNF},
	{"P_DoSuperTransformation",lib_pDoSuperTransformation},
	{"P_ExplodeMissile",lib_pExplodeMissile},
	{"P_PlayerTouchingSectorSpecial",lib_pPlayerTouchingSectorSpecial},
	{"P_FindSpecialLineFromTag",lib_pFindSpecialLineFromTag},
	{"P_SwitchWeather",lib_pSwitchWeather},
	{"P_LinedefExecute",lib_pLinedefExecute},
	{"P_SpawnLightningFlash",lib_pSpawnLightningFlash},
	{"P_FadeLight",lib_pFadeLight},
	{"P_ThingOnSpecial3DFloor",lib_pThingOnSpecial3DFloor},
	{"P_SetupLevelSky",lib_pSetupLevelSky},
	{"P_SetSkyboxMobj",lib_pSetSkyboxMobj},
	{"P_StartQuake",lib_pStartQuake},

	// r_defs
	{"R_PointToAngle",lib_rPointToAngle},
	{"R_PointToAngle2",lib_rPointToAngle2},
	{"R_PointToDist",lib_rPointToDist},
	{"R_PointToDist2",lib_rPointToDist2},
	{"R_PointInSubsector",lib_rPointInSubsector},

	// r_things (sprite)
	{"R_Char2Frame",lib_rChar2Frame},
	{"R_Frame2Char",lib_rFrame2Char},

	// s_sound
	{"S_StartSound",lib_sStartSound},
	{"S_StartSoundAtVolume",lib_sStartSoundAtVolume},
	{"S_StopSound",lib_sStopSound},
	{"S_ChangeMusic",lib_sChangeMusic},
	{"S_SpeedMusic",lib_sSpeedMusic},
	{"S_StopMusic",lib_sStopMusic},
	{"S_OriginPlaying",lib_sOriginPlaying},
	{"S_IdPlaying",lib_sIdPlaying},
	{"S_SoundPlaying",lib_sSoundPlaying},

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
