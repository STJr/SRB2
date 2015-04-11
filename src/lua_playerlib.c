// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_playerlib.c
/// \brief player object library for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "fastcmp.h"
#include "p_mobj.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors

static int lib_iteratePlayers(lua_State *L)
{
	INT32 i = -1;
	if (lua_gettop(L) < 2)
	{
		//return luaL_error(L, "Don't call players.iterate() directly, use it as 'for player in players.iterate do <block> end'.");
		lua_pushcfunction(L, lib_iteratePlayers);
		return 1;
	}
	lua_settop(L, 2);
	lua_remove(L, 1); // state is unused.
	if (!lua_isnil(L, 1))
		i = (INT32)(*((player_t **)luaL_checkudata(L, 1, META_PLAYER)) - players);
	for (i++; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (!players[i].mo)
			continue;
		LUA_PushUserdata(L, &players[i], META_PLAYER);
		return 1;
	}
	return 0;
}

static int lib_getPlayer(lua_State *L)
{
	const char *field;
	// i -> players[i]
	if (lua_type(L, 2) == LUA_TNUMBER)
	{
		lua_Integer i = luaL_checkinteger(L, 2);
		if (i < 0 || i >= MAXPLAYERS)
			return luaL_error(L, "players[] index cannot exceed MAXPLAYERS");
		if (!playeringame[i])
			return 0;
		if (!players[i].mo)
			return 0;
		LUA_PushUserdata(L, &players[i], META_PLAYER);
		return 1;
	}

	field = luaL_checkstring(L, 2);
	if (fastcmp(field,"iterate"))
	{
		lua_pushcfunction(L, lib_iteratePlayers);
		return 1;
	}
	return 0;
}

// #players -> MAXPLAYERS
static int lib_lenPlayer(lua_State *L)
{
	lua_pushinteger(L, MAXPLAYERS);
	return 1;
}

static int player_get(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	const char *field = luaL_checkstring(L, 2);

	if (!plr) {
		if (fastcmp(field,"valid")) {
			lua_pushboolean(L, false);
			return 1;
		}
		return LUA_ErrInvalid(L, "player_t");
	}

	if (fastcmp(field,"valid"))
		lua_pushboolean(L, true);
	else if (fastcmp(field,"name"))
		lua_pushstring(L, player_names[plr-players]);
	else if (fastcmp(field,"mo"))
	{
		if (plr->spectator)
			lua_pushnil(L);
		else
			LUA_PushUserdata(L, plr->mo, META_MOBJ);
	}
	else if (fastcmp(field,"cmd"))
		LUA_PushUserdata(L, &plr->cmd, META_TICCMD);
	else if (fastcmp(field,"playerstate"))
		lua_pushinteger(L, plr->playerstate);
	else if (fastcmp(field,"viewz"))
		lua_pushinteger(L, plr->viewz);
	else if (fastcmp(field,"viewheight"))
		lua_pushinteger(L, plr->viewheight);
	else if (fastcmp(field,"deltaviewheight"))
		lua_pushinteger(L, plr->deltaviewheight);
	else if (fastcmp(field,"bob"))
		lua_pushinteger(L, plr->bob);
	else if (fastcmp(field,"aiming"))
		lua_pushinteger(L, plr->aiming);
	else if (fastcmp(field,"health"))
		lua_pushinteger(L, plr->health);
	else if (fastcmp(field,"pity"))
		lua_pushinteger(L, plr->pity);
	else if (fastcmp(field,"currentweapon"))
		lua_pushinteger(L, plr->currentweapon);
	else if (fastcmp(field,"ringweapons"))
		lua_pushinteger(L, plr->ringweapons);
	else if (fastcmp(field,"powers"))
		LUA_PushUserdata(L, plr->powers, META_POWERS);
	else if (fastcmp(field,"pflags"))
		lua_pushinteger(L, plr->pflags);
	else if (fastcmp(field,"panim"))
		lua_pushinteger(L, plr->panim);
	else if (fastcmp(field,"flashcount"))
		lua_pushinteger(L, plr->flashcount);
	else if (fastcmp(field,"flashpal"))
		lua_pushinteger(L, plr->flashpal);
	else if (fastcmp(field,"skincolor"))
		lua_pushinteger(L, plr->skincolor);
	else if (fastcmp(field,"score"))
		lua_pushinteger(L, plr->score);
	else if (fastcmp(field,"dashspeed"))
		lua_pushinteger(L, plr->dashspeed);
	else if (fastcmp(field,"dashtime"))
		lua_pushinteger(L, plr->dashtime);
	else if (fastcmp(field,"normalspeed"))
		lua_pushinteger(L, plr->normalspeed);
	else if (fastcmp(field,"runspeed"))
		lua_pushinteger(L, plr->runspeed);
	else if (fastcmp(field,"thrustfactor"))
		lua_pushinteger(L, plr->thrustfactor);
	else if (fastcmp(field,"accelstart"))
		lua_pushinteger(L, plr->accelstart);
	else if (fastcmp(field,"acceleration"))
		lua_pushinteger(L, plr->acceleration);
	else if (fastcmp(field,"charability"))
		lua_pushinteger(L, plr->charability);
	else if (fastcmp(field,"charability2"))
		lua_pushinteger(L, plr->charability2);
	else if (fastcmp(field,"charflags"))
		lua_pushinteger(L, plr->charflags);
	else if (fastcmp(field,"thokitem"))
		lua_pushinteger(L, plr->thokitem);
	else if (fastcmp(field,"spinitem"))
		lua_pushinteger(L, plr->spinitem);
	else if (fastcmp(field,"revitem"))
		lua_pushinteger(L, plr->revitem);
	else if (fastcmp(field,"actionspd"))
		lua_pushinteger(L, plr->actionspd);
	else if (fastcmp(field,"mindash"))
		lua_pushinteger(L, plr->mindash);
	else if (fastcmp(field,"maxdash"))
		lua_pushinteger(L, plr->maxdash);
	else if (fastcmp(field,"jumpfactor"))
		lua_pushinteger(L, plr->jumpfactor);
	else if (fastcmp(field,"lives"))
		lua_pushinteger(L, plr->lives);
	else if (fastcmp(field,"continues"))
		lua_pushinteger(L, plr->continues);
	else if (fastcmp(field,"xtralife"))
		lua_pushinteger(L, plr->xtralife);
	else if (fastcmp(field,"gotcontinue"))
		lua_pushinteger(L, plr->gotcontinue);
	else if (fastcmp(field,"speed"))
		lua_pushinteger(L, plr->speed);
	else if (fastcmp(field,"jumping"))
		lua_pushboolean(L, plr->jumping);
	else if (fastcmp(field,"secondjump"))
		lua_pushinteger(L, plr->secondjump);
	else if (fastcmp(field,"fly1"))
		lua_pushinteger(L, plr->fly1);
	else if (fastcmp(field,"scoreadd"))
		lua_pushinteger(L, plr->scoreadd);
	else if (fastcmp(field,"glidetime"))
		lua_pushinteger(L, plr->glidetime);
	else if (fastcmp(field,"climbing"))
		lua_pushinteger(L, plr->climbing);
	else if (fastcmp(field,"deadtimer"))
		lua_pushinteger(L, plr->deadtimer);
	else if (fastcmp(field,"exiting"))
		lua_pushinteger(L, plr->exiting);
	else if (fastcmp(field,"homing"))
		lua_pushinteger(L, plr->homing);
	else if (fastcmp(field,"skidtime"))
		lua_pushinteger(L, plr->skidtime);
	else if (fastcmp(field,"cmomx"))
		lua_pushinteger(L, plr->cmomx);
	else if (fastcmp(field,"cmomy"))
		lua_pushinteger(L, plr->cmomy);
	else if (fastcmp(field,"rmomx"))
		lua_pushinteger(L, plr->rmomx);
	else if (fastcmp(field,"rmomy"))
		lua_pushinteger(L, plr->rmomy);
	else if (fastcmp(field,"numboxes"))
		lua_pushinteger(L, plr->numboxes);
	else if (fastcmp(field,"totalring"))
		lua_pushinteger(L, plr->totalring);
	else if (fastcmp(field,"realtime"))
		lua_pushinteger(L, plr->realtime);
	else if (fastcmp(field,"laps"))
		lua_pushinteger(L, plr->laps);
	else if (fastcmp(field,"ctfteam"))
		lua_pushinteger(L, plr->ctfteam);
	else if (fastcmp(field,"gotflag"))
		lua_pushinteger(L, plr->gotflag);
	else if (fastcmp(field,"weapondelay"))
		lua_pushinteger(L, plr->weapondelay);
	else if (fastcmp(field,"tossdelay"))
		lua_pushinteger(L, plr->tossdelay);
	else if (fastcmp(field,"starpostx"))
		lua_pushinteger(L, plr->starpostx);
	else if (fastcmp(field,"starposty"))
		lua_pushinteger(L, plr->starposty);
	else if (fastcmp(field,"starpostz"))
		lua_pushinteger(L, plr->starpostz);
	else if (fastcmp(field,"starpostnum"))
		lua_pushinteger(L, plr->starpostnum);
	else if (fastcmp(field,"starposttime"))
		lua_pushinteger(L, plr->starposttime);
	else if (fastcmp(field,"starpostangle"))
		lua_pushinteger(L, plr->starpostangle);
	else if (fastcmp(field,"angle_pos"))
		lua_pushinteger(L, plr->angle_pos);
	else if (fastcmp(field,"old_angle_pos"))
		lua_pushinteger(L, plr->old_angle_pos);
	else if (fastcmp(field,"axis1"))
		LUA_PushUserdata(L, plr->axis1, META_MOBJ);
	else if (fastcmp(field,"axis2"))
		LUA_PushUserdata(L, plr->axis2, META_MOBJ);
	else if (fastcmp(field,"bumpertime"))
		lua_pushinteger(L, plr->bumpertime);
	else if (fastcmp(field,"flyangle"))
		lua_pushinteger(L, plr->flyangle);
	else if (fastcmp(field,"drilltimer"))
		lua_pushinteger(L, plr->drilltimer);
	else if (fastcmp(field,"linkcount"))
		lua_pushinteger(L, plr->linkcount);
	else if (fastcmp(field,"linktimer"))
		lua_pushinteger(L, plr->linktimer);
	else if (fastcmp(field,"anotherflyangle"))
		lua_pushinteger(L, plr->anotherflyangle);
	else if (fastcmp(field,"nightstime"))
		lua_pushinteger(L, plr->nightstime);
	else if (fastcmp(field,"drillmeter"))
		lua_pushinteger(L, plr->drillmeter);
	else if (fastcmp(field,"drilldelay"))
		lua_pushinteger(L, plr->drilldelay);
	else if (fastcmp(field,"bonustime"))
		lua_pushboolean(L, plr->bonustime);
	else if (fastcmp(field,"capsule"))
		LUA_PushUserdata(L, plr->capsule, META_MOBJ);
	else if (fastcmp(field,"mare"))
		lua_pushinteger(L, plr->mare);
	else if (fastcmp(field,"marebegunat"))
		lua_pushinteger(L, plr->marebegunat);
	else if (fastcmp(field,"startedtime"))
		lua_pushinteger(L, plr->startedtime);
	else if (fastcmp(field,"finishedtime"))
		lua_pushinteger(L, plr->finishedtime);
	else if (fastcmp(field,"finishedrings"))
		lua_pushinteger(L, plr->finishedrings);
	else if (fastcmp(field,"marescore"))
		lua_pushinteger(L, plr->marescore);
	else if (fastcmp(field,"lastmarescore"))
		lua_pushinteger(L, plr->lastmarescore);
	else if (fastcmp(field,"lastmare"))
		lua_pushinteger(L, plr->lastmare);
	else if (fastcmp(field,"maxlink"))
		lua_pushinteger(L, plr->maxlink);
	else if (fastcmp(field,"texttimer"))
		lua_pushinteger(L, plr->texttimer);
	else if (fastcmp(field,"textvar"))
		lua_pushinteger(L, plr->textvar);
	else if (fastcmp(field,"lastsidehit"))
		lua_pushinteger(L, plr->lastsidehit);
	else if (fastcmp(field,"lastlinehit"))
		lua_pushinteger(L, plr->lastlinehit);
	else if (fastcmp(field,"losstime"))
		lua_pushinteger(L, plr->losstime);
	else if (fastcmp(field,"onconveyor"))
		lua_pushinteger(L, plr->onconveyor);
#ifdef TOPDOWN
	else if (fastcmp(field,"maxflyheight"))
		lua_pushinteger(L, plr->maxflyheight);
	else if (fastcmp(field,"emblems"))
		lua_pushinteger(L, plr->emblems);
	else if (fastcmp(field,"climbtime"))
		lua_pushinteger(L, plr->climbtime);
	else if (fastcmp(field,"damagededuct"))
		lua_pushinteger(L, plr->damagededuct);
	else if (fastcmp(field,"levelscore"))
		lua_pushinteger(L, plr->levelscore);
#endif
	else if (fastcmp(field,"awayviewmobj"))
		LUA_PushUserdata(L, plr->awayviewmobj, META_MOBJ);
	else if (fastcmp(field,"awayviewtics"))
		lua_pushinteger(L, plr->awayviewtics);
	else if (fastcmp(field,"awayviewaiming"))
		lua_pushinteger(L, plr->awayviewaiming);
	else if (fastcmp(field,"spectator"))
		lua_pushinteger(L, plr->spectator);
	else if (fastcmp(field,"bot"))
		lua_pushinteger(L, plr->bot);
	else if (fastcmp(field,"jointime"))
		lua_pushinteger(L, plr->jointime);
#ifdef HWRENDER
	else if (fastcmp(field,"fovadd"))
		lua_pushinteger(L, plr->fovadd);
#endif
	else {
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, plr);
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) { // no extra values table
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no extvars table or field named '%s'; returning nil.\n"), "player_t", field);
			return 0;
		}
		lua_getfield(L, -1, field);
		if (lua_isnil(L, -1)) // no value for this field
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "player_t", field);
	}

	return 1;
}

#define NOSET luaL_error(L, LUA_QL("player_t") " field " LUA_QS " should not be set directly.", field)
static int player_set(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	const char *field = luaL_checkstring(L, 2);
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");

	if (fastcmp(field,"mo")) {
		mobj_t *newmo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		plr->mo->player = NULL; // remove player pointer from old mobj
		(newmo->player = plr)->mo = newmo; // set player pointer for new mobj, and set new mobj as the player's mobj
	}
	else if (fastcmp(field,"cmd"))
		return NOSET;
	else if (fastcmp(field,"playerstate"))
		plr->playerstate = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"viewz"))
		plr->viewz = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"viewheight"))
		plr->viewheight = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"deltaviewheight"))
		plr->deltaviewheight = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bob"))
		plr->bob = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aiming")) {
		plr->aiming = (angle_t)luaL_checkinteger(L, 3);
		if (plr == &players[consoleplayer])
			localaiming = plr->aiming;
		else if (plr == &players[secondarydisplayplayer])
			localaiming2 = plr->aiming;
	}
	else if (fastcmp(field,"health"))
		plr->health = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"pity"))
		plr->pity = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"currentweapon"))
		plr->currentweapon = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ringweapons"))
		plr->ringweapons = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"powers"))
		return NOSET;
	else if (fastcmp(field,"pflags"))
		plr->pflags = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"panim"))
		plr->panim = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flashcount"))
		plr->flashcount = (UINT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flashpal"))
		plr->flashpal = (UINT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"skincolor"))
		plr->skincolor = ((UINT8)luaL_checkinteger(L, 3)) % MAXSKINCOLORS;
	else if (fastcmp(field,"score"))
		plr->score = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dashspeed"))
		plr->dashspeed = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"dashtime"))
		plr->dashtime = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"normalspeed"))
		plr->normalspeed = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"runspeed"))
		plr->runspeed = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"thrustfactor"))
		plr->thrustfactor = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"accelstart"))
		plr->accelstart = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"acceleration"))
		plr->acceleration = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"charability"))
		plr->charability = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"charability2"))
		plr->charability2 = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"charflags"))
		plr->charflags = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"thokitem"))
		plr->thokitem = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spinitem"))
		plr->spinitem = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"revitem"))
		plr->revitem = luaL_checkinteger(L, 3);
	else if (fastcmp(field,"actionspd"))
		plr->actionspd = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"mindash"))
		plr->mindash = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"maxdash"))
		plr->maxdash = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"jumpfactor"))
		plr->jumpfactor = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lives"))
		plr->lives = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"continues"))
		plr->continues = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"xtralife"))
		plr->xtralife = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"gotcontinue"))
		plr->gotcontinue = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"speed"))
		plr->speed = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"jumping"))
		plr->jumping = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"secondjump"))
		plr->secondjump = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"fly1"))
		plr->fly1 = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"scoreadd"))
		plr->scoreadd = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"glidetime"))
		plr->glidetime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"climbing"))
		plr->climbing = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"deadtimer"))
		plr->deadtimer = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"exiting"))
		plr->exiting = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"homing"))
		plr->homing = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"skidtime"))
		plr->skidtime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"cmomx"))
		plr->cmomx = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"cmomy"))
		plr->cmomy = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rmomx"))
		plr->rmomx = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"rmomy"))
		plr->rmomy = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"numboxes"))
		plr->numboxes = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"totalring"))
		plr->totalring = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"realtime"))
		plr->realtime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"laps"))
		plr->laps = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"ctfteam"))
		plr->ctfteam = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"gotflag"))
		plr->gotflag = (UINT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"weapondelay"))
		plr->weapondelay = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"tossdelay"))
		plr->tossdelay = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starpostx"))
		plr->starpostx = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starposty"))
		plr->starposty = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starpostz"))
		plr->starpostz = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starpostnum"))
		plr->starpostnum = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starposttime"))
		plr->starposttime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"starpostangle"))
		plr->starpostangle = (angle_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"angle_pos"))
		plr->angle_pos = (angle_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"old_angle_pos"))
		plr->old_angle_pos = (angle_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"axis1"))
		P_SetTarget(&plr->axis1, *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ)));
	else if (fastcmp(field,"axis2"))
		P_SetTarget(&plr->axis2, *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ)));
	else if (fastcmp(field,"bumpertime"))
		plr->bumpertime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"flyangle"))
		plr->flyangle = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"drilltimer"))
		plr->drilltimer = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"linkcount"))
		plr->linkcount = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"linktimer"))
		plr->linktimer = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"anotherflyangle"))
		plr->anotherflyangle = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"nightstime"))
		plr->nightstime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"drillmeter"))
		plr->drillmeter = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"drilldelay"))
		plr->drilldelay = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"bonustime"))
		plr->bonustime = luaL_checkboolean(L, 3);
	else if (fastcmp(field,"capsule"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->capsule, mo);
	}
	else if (fastcmp(field,"mare"))
		plr->mare = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"marebegunat"))
		plr->marebegunat = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"startedtime"))
		plr->startedtime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"finishedtime"))
		plr->finishedtime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"finishedrings"))
		plr->finishedrings = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"marescore"))
		plr->marescore = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastmarescore"))
		plr->lastmarescore = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastmare"))
		plr->lastmare = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"maxlink"))
		plr->maxlink = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"texttimer"))
		plr->texttimer = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"textvar"))
		plr->textvar = (UINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastsidehit"))
		plr->lastsidehit = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"lastlinehit"))
		plr->lastlinehit = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"losstime"))
		plr->losstime = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"onconveyor"))
		plr->onconveyor = (INT32)luaL_checkinteger(L, 3);
#ifdef TOPDOWN
	else if (fastcmp(field,"maxflyheight"))
		plr->maxflyheight = (fixed_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"emblems"))
		plr->emblems = (INT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"climbtime"))
		plr->emblems = (tic_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"damagededuct"))
		plr->emblems = (UINT32)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"levelscore"))
		plr->emblems = (UINT32)luaL_checkinteger(L, 3);
#endif
	else if (fastcmp(field,"awayviewmobj"))
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->awayviewmobj, mo);
	}
	else if (fastcmp(field,"awayviewtics"))
	{
		plr->awayviewtics = (INT32)luaL_checkinteger(L, 3);
		if (plr->awayviewtics && !plr->awayviewmobj) // awayviewtics must ALWAYS have an awayviewmobj set!!
			P_SetTarget(&plr->awayviewmobj, plr->mo); // but since the script might set awayviewmobj immediately AFTER setting awayviewtics, use player mobj as filler for now.
	}
	else if (fastcmp(field,"awayviewaiming"))
		plr->awayviewaiming = (angle_t)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"spectator"))
		plr->spectator = lua_toboolean(L, 3);
	else if (fastcmp(field,"bot"))
		return NOSET;
	else if (fastcmp(field,"jointime"))
		plr->jointime = (tic_t)luaL_checkinteger(L, 3);
#ifdef HWRENDER
	else if (fastcmp(field,"fovadd"))
		plr->fovadd = (fixed_t)luaL_checkinteger(L, 3);
#endif
	else {
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, plr);
		lua_rawget(L, -2);
		if (lua_isnil(L, -1)) {
			// This index doesn't have a table for extra values yet, let's make one.
			lua_pop(L, 1);
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; adding it as Lua data.\n"), "player_t", field);
			lua_newtable(L);
			lua_pushlightuserdata(L, plr);
			lua_pushvalue(L, -2); // ext value table
			lua_rawset(L, -4); // LREG_EXTVARS table
		}
		lua_pushvalue(L, 3); // value to store
		lua_setfield(L, -2, field);
		lua_pop(L, 2);
	}

	return 0;
}

#undef NOSET

static int player_num(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	if (!plr)
		return luaL_error(L, "accessed player_t doesn't exist anymore.");
	lua_pushinteger(L, plr-players);
	return 1;
}

// powers, p -> powers[p]
static int power_get(lua_State *L)
{
	UINT16 *powers = *((UINT16 **)luaL_checkudata(L, 1, META_POWERS));
	powertype_t p = luaL_checkinteger(L, 2);
	if (p >= NUMPOWERS)
		return luaL_error(L, LUA_QL("powertype_t") " cannot be %u", p);
	lua_pushinteger(L, powers[p]);
	return 1;
}

// powers, p, value -> powers[p] = value
static int power_set(lua_State *L)
{
	UINT16 *powers = *((UINT16 **)luaL_checkudata(L, 1, META_POWERS));
	powertype_t p = luaL_checkinteger(L, 2);
	UINT16 i = (UINT16)luaL_checkinteger(L, 3);
	if (p >= NUMPOWERS)
		return luaL_error(L, LUA_QL("powertype_t") " cannot be %u", p);
	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	powers[p] = i;
	return 0;
}

// #powers -> NUMPOWERS
static int power_len(lua_State *L)
{
	lua_pushinteger(L, NUMPOWERS);
	return 1;
}

#define NOFIELD luaL_error(L, LUA_QL("ticcmd_t") " has no field named " LUA_QS, field)

static int ticcmd_get(lua_State *L)
{
	ticcmd_t *cmd = *((ticcmd_t **)luaL_checkudata(L, 1, META_TICCMD));
	const char *field = luaL_checkstring(L, 2);
	if (!cmd)
		return LUA_ErrInvalid(L, "player_t");

	if (fastcmp(field,"forwardmove"))
		lua_pushinteger(L, cmd->forwardmove);
	else if (fastcmp(field,"sidemove"))
		lua_pushinteger(L, cmd->sidemove);
	else if (fastcmp(field,"angleturn"))
		lua_pushinteger(L, cmd->angleturn);
	else if (fastcmp(field,"aiming"))
		lua_pushinteger(L, cmd->aiming);
	else if (fastcmp(field,"buttons"))
		lua_pushinteger(L, cmd->buttons);
	else
		return NOFIELD;

	return 1;
}

static int ticcmd_set(lua_State *L)
{
	ticcmd_t *cmd = *((ticcmd_t **)luaL_checkudata(L, 1, META_TICCMD));
	const char *field = luaL_checkstring(L, 2);
	if (!cmd)
		return LUA_ErrInvalid(L, "ticcmd_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");

	if (fastcmp(field,"forwardmove"))
		cmd->forwardmove = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"sidemove"))
		cmd->sidemove = (SINT8)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"angleturn"))
		cmd->angleturn = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"aiming"))
		cmd->aiming = (INT16)luaL_checkinteger(L, 3);
	else if (fastcmp(field,"buttons"))
		cmd->buttons = (UINT16)luaL_checkinteger(L, 3);
	else
		return NOFIELD;

	return 0;
}

#undef NOFIELD

int LUA_PlayerLib(lua_State *L)
{
	luaL_newmetatable(L, META_PLAYER);
		lua_pushcfunction(L, player_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, player_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, player_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_POWERS);
		lua_pushcfunction(L, power_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, power_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, power_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	luaL_newmetatable(L, META_TICCMD);
		lua_pushcfunction(L, ticcmd_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, ticcmd_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getPlayer);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_lenPlayer);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "players");
	return 0;
}

#endif
