// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_playerlib.c
/// \brief player object library for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "p_mobj.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h" // hud_running errors
#include "lua_hook.h" // hook_cmd_running errors

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
			return luaL_error(L, "players[] index %d out of range (0 - %d)", i, MAXPLAYERS-1);
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

enum player_e
{
	player_valid,
	player_name,
	player_realmo,
	player_mo,
	player_cmd,
	player_playerstate,
	player_camerascale,
	player_shieldscale,
	player_viewz,
	player_viewheight,
	player_deltaviewheight,
	player_bob,
	player_viewrollangle,
	player_aiming,
	player_drawangle,
	player_rings,
	player_spheres,
	player_pity,
	player_currentweapon,
	player_ringweapons,
	player_ammoremoval,
	player_ammoremovaltimer,
	player_ammoremovalweapon,
	player_powers,
	player_pflags,
	player_panim,
	player_flashcount,
	player_flashpal,
	player_skincolor,
	player_skin,
	player_availabilities,
	player_score,
	player_recordscore,
	player_dashspeed,
	player_normalspeed,
	player_runspeed,
	player_thrustfactor,
	player_accelstart,
	player_acceleration,
	player_charability,
	player_charability2,
	player_charflags,
	player_thokitem,
	player_spinitem,
	player_revitem,
	player_followitem,
	player_followmobj,
	player_actionspd,
	player_mindash,
	player_maxdash,
	player_jumpfactor,
	player_height,
	player_spinheight,
	player_lives,
	player_continues,
	player_xtralife,
	player_gotcontinue,
	player_speed,
	player_secondjump,
	player_fly1,
	player_scoreadd,
	player_glidetime,
	player_climbing,
	player_deadtimer,
	player_exiting,
	player_homing,
	player_dashmode,
	player_skidtime,
	player_cmomx,
	player_cmomy,
	player_rmomx,
	player_rmomy,
	player_numboxes,
	player_totalring,
	player_realtime,
	player_laps,
	player_ctfteam,
	player_gotflag,
	player_weapondelay,
	player_tossdelay,
	player_starpostx,
	player_starposty,
	player_starpostz,
	player_starpostnum,
	player_starposttime,
	player_starpostangle,
	player_starpostscale,
	player_angle_pos,
	player_old_angle_pos,
	player_axis1,
	player_axis2,
	player_bumpertime,
	player_flyangle,
	player_drilltimer,
	player_linkcount,
	player_linktimer,
	player_anotherflyangle,
	player_nightstime,
	player_drillmeter,
	player_drilldelay,
	player_bonustime,
	player_capsule,
	player_drone,
	player_oldscale,
	player_mare,
	player_marelap,
	player_marebonuslap,
	player_marebegunat,
	player_startedtime,
	player_finishedtime,
	player_lapbegunat,
	player_lapstartedtime,
	player_finishedspheres,
	player_finishedrings,
	player_marescore,
	player_lastmarescore,
	player_totalmarescore,
	player_lastmare,
	player_lastmarelap,
	player_lastmarebonuslap,
	player_totalmarelap,
	player_totalmarebonuslap,
	player_maxlink,
	player_texttimer,
	player_textvar,
	player_lastsidehit,
	player_lastlinehit,
	player_losstime,
	player_timeshit,
	player_onconveyor,
	player_awayviewmobj,
	player_awayviewtics,
	player_awayviewaiming,
	player_spectator,
	player_outofcoop,
	player_bot,
	player_botleader,
	player_lastbuttons,
	player_blocked,
	player_jointime,
	player_quittime,
	player_ping,
#ifdef HWRENDER
	player_fovadd,
#endif
};

static const char *const player_opt[] = {
	"valid",
	"name",
	"realmo",
	"mo",
	"cmd",
	"playerstate",
	"camerascale",
	"shieldscale",
	"viewz",
	"viewheight",
	"deltaviewheight",
	"bob",
	"viewrollangle",
	"aiming",
	"drawangle",
	"rings",
	"spheres",
	"pity",
	"currentweapon",
	"ringweapons",
	"ammoremoval",
	"ammoremovaltimer",
	"ammoremovalweapon",
	"powers",
	"pflags",
	"panim",
	"flashcount",
	"flashpal",
	"skincolor",
	"skin",
	"availabilities",
	"score",
	"recordscore",
	"dashspeed",
	"normalspeed",
	"runspeed",
	"thrustfactor",
	"accelstart",
	"acceleration",
	"charability",
	"charability2",
	"charflags",
	"thokitem",
	"spinitem",
	"revitem",
	"followitem",
	"followmobj",
	"actionspd",
	"mindash",
	"maxdash",
	"jumpfactor",
	"height",
	"spinheight",
	"lives",
	"continues",
	"xtralife",
	"gotcontinue",
	"speed",
	"secondjump",
	"fly1",
	"scoreadd",
	"glidetime",
	"climbing",
	"deadtimer",
	"exiting",
	"homing",
	"dashmode",
	"skidtime",
	"cmomx",
	"cmomy",
	"rmomx",
	"rmomy",
	"numboxes",
	"totalring",
	"realtime",
	"laps",
	"ctfteam",
	"gotflag",
	"weapondelay",
	"tossdelay",
	"starpostx",
	"starposty",
	"starpostz",
	"starpostnum",
	"starposttime",
	"starpostangle",
	"starpostscale",
	"angle_pos",
	"old_angle_pos",
	"axis1",
	"axis2",
	"bumpertime",
	"flyangle",
	"drilltimer",
	"linkcount",
	"linktimer",
	"anotherflyangle",
	"nightstime",
	"drillmeter",
	"drilldelay",
	"bonustime",
	"capsule",
	"drone",
	"oldscale",
	"mare",
	"marelap",
	"marebonuslap",
	"marebegunat",
	"startedtime",
	"finishedtime",
	"lapbegunat",
	"lapstartedtime",
	"finishedspheres",
	"finishedrings",
	"marescore",
	"lastmarescore",
	"totalmarescore",
	"lastmare",
	"lastmarelap",
	"lastmarebonuslap",
	"totalmarelap",
	"totalmarebonuslap",
	"maxlink",
	"texttimer",
	"textvar",
	"lastsidehit",
	"lastlinehit",
	"losstime",
	"timeshit",
	"onconveyor",
	"awayviewmobj",
	"awayviewtics",
	"awayviewaiming",
	"spectator",
	"outofcoop",
	"bot",
	"botleader",
	"lastbuttons",
	"blocked",
	"jointime",
	"quittime",
	"ping",
#ifdef HWRENDER
	"fovadd",
#endif
	NULL,
};

static int player_fields_ref = LUA_NOREF;

static int player_get(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	enum player_e field = Lua_optoption(L, 2, -1, player_fields_ref);
	lua_settop(L, 2);

	if (!plr)
	{
		if (field == player_valid)
		{
			lua_pushboolean(L, false);
			return 1;
		}
		return LUA_ErrInvalid(L, "player_t");
	}

	switch (field)
	{
	case player_valid:
		lua_pushboolean(L, true);
		break;
	case player_name:
		lua_pushstring(L, player_names[plr-players]);
		break;
	case player_realmo:
		LUA_PushUserdata(L, plr->mo, META_MOBJ);
		break;
	// Kept for backward-compatibility
	// Should be fixed to work like "realmo" later
	case player_mo:
		if (plr->spectator)
			lua_pushnil(L);
		else
			LUA_PushUserdata(L, plr->mo, META_MOBJ);
		break;
	case player_cmd:
		LUA_PushUserdata(L, &plr->cmd, META_TICCMD);
		break;
	case player_playerstate:
		lua_pushinteger(L, plr->playerstate);
		break;
	case player_camerascale:
		lua_pushfixed(L, plr->camerascale);
		break;
	case player_shieldscale:
		lua_pushfixed(L, plr->shieldscale);
		break;
	case player_viewz:
		lua_pushfixed(L, plr->viewz);
		break;
	case player_viewheight:
		lua_pushfixed(L, plr->viewheight);
		break;
	case player_deltaviewheight:
		lua_pushfixed(L, plr->deltaviewheight);
		break;
	case player_bob:
		lua_pushfixed(L, plr->bob);
		break;
	case player_viewrollangle:
		lua_pushangle(L, plr->viewrollangle);
		break;
	case player_aiming:
		lua_pushangle(L, plr->aiming);
		break;
	case player_drawangle:
		lua_pushangle(L, plr->drawangle);
		break;
	case player_rings:
		lua_pushinteger(L, plr->rings);
		break;
	case player_spheres:
		lua_pushinteger(L, plr->spheres);
		break;
	case player_pity:
		lua_pushinteger(L, plr->pity);
		break;
	case player_currentweapon:
		lua_pushinteger(L, plr->currentweapon);
		break;
	case player_ringweapons:
		lua_pushinteger(L, plr->ringweapons);
		break;
	case player_ammoremoval:
		lua_pushinteger(L, plr->ammoremoval);
		break;
	case player_ammoremovaltimer:
		lua_pushinteger(L, plr->ammoremovaltimer);
		break;
	case player_ammoremovalweapon:
		lua_pushinteger(L, plr->ammoremovalweapon);
		break;
	case player_powers:
		LUA_PushUserdata(L, plr->powers, META_POWERS);
		break;
	case player_pflags:
		lua_pushinteger(L, plr->pflags);
		break;
	case player_panim:
		lua_pushinteger(L, plr->panim);
		break;
	case player_flashcount:
		lua_pushinteger(L, plr->flashcount);
		break;
	case player_flashpal:
		lua_pushinteger(L, plr->flashpal);
		break;
	case player_skincolor:
		lua_pushinteger(L, plr->skincolor);
		break;
	case player_skin:
		lua_pushinteger(L, plr->skin);
		break;
	case player_availabilities:
		lua_pushinteger(L, plr->availabilities);
		break;
	case player_score:
		lua_pushinteger(L, plr->score);
		break;
	case player_recordscore:
		lua_pushinteger(L, plr->recordscore);
		break;
	case player_dashspeed:
		lua_pushfixed(L, plr->dashspeed);
		break;
	case player_normalspeed:
		lua_pushfixed(L, plr->normalspeed);
		break;
	case player_runspeed:
		lua_pushfixed(L, plr->runspeed);
		break;
	case player_thrustfactor:
		lua_pushinteger(L, plr->thrustfactor);
		break;
	case player_accelstart:
		lua_pushinteger(L, plr->accelstart);
		break;
	case player_acceleration:
		lua_pushinteger(L, plr->acceleration);
		break;
	case player_charability:
		lua_pushinteger(L, plr->charability);
		break;
	case player_charability2:
		lua_pushinteger(L, plr->charability2);
		break;
	case player_charflags:
		lua_pushinteger(L, plr->charflags);
		break;
	case player_thokitem:
		lua_pushinteger(L, plr->thokitem);
		break;
	case player_spinitem:
		lua_pushinteger(L, plr->spinitem);
		break;
	case player_revitem:
		lua_pushinteger(L, plr->revitem);
		break;
	case player_followitem:
		lua_pushinteger(L, plr->followitem);
		break;
	case player_followmobj:
		LUA_PushUserdata(L, plr->followmobj, META_MOBJ);
		break;
	case player_actionspd:
		lua_pushfixed(L, plr->actionspd);
		break;
	case player_mindash:
		lua_pushfixed(L, plr->mindash);
		break;
	case player_maxdash:
		lua_pushfixed(L, plr->maxdash);
		break;
	case player_jumpfactor:
		lua_pushfixed(L, plr->jumpfactor);
		break;
	case player_height:
		lua_pushfixed(L, plr->height);
		break;
	case player_spinheight:
		lua_pushfixed(L, plr->spinheight);
		break;
	case player_lives:
		lua_pushinteger(L, plr->lives);
		break;
	case player_continues:
		lua_pushinteger(L, plr->continues);
		break;
	case player_xtralife:
		lua_pushinteger(L, plr->xtralife);
		break;
	case player_gotcontinue:
		lua_pushinteger(L, plr->gotcontinue);
		break;
	case player_speed:
		lua_pushfixed(L, plr->speed);
		break;
	case player_secondjump:
		lua_pushinteger(L, plr->secondjump);
		break;
	case player_fly1:
		lua_pushinteger(L, plr->fly1);
		break;
	case player_scoreadd:
		lua_pushinteger(L, plr->scoreadd);
		break;
	case player_glidetime:
		lua_pushinteger(L, plr->glidetime);
		break;
	case player_climbing:
		lua_pushinteger(L, plr->climbing);
		break;
	case player_deadtimer:
		lua_pushinteger(L, plr->deadtimer);
		break;
	case player_exiting:
		lua_pushinteger(L, plr->exiting);
		break;
	case player_homing:
		lua_pushinteger(L, plr->homing);
		break;
	case player_dashmode:
		lua_pushinteger(L, plr->dashmode);
		break;
	case player_skidtime:
		lua_pushinteger(L, plr->skidtime);
		break;
	case player_cmomx:
		lua_pushfixed(L, plr->cmomx);
		break;
	case player_cmomy:
		lua_pushfixed(L, plr->cmomy);
		break;
	case player_rmomx:
		lua_pushfixed(L, plr->rmomx);
		break;
	case player_rmomy:
		lua_pushfixed(L, plr->rmomy);
		break;
	case player_numboxes:
		lua_pushinteger(L, plr->numboxes);
		break;
	case player_totalring:
		lua_pushinteger(L, plr->totalring);
		break;
	case player_realtime:
		lua_pushinteger(L, plr->realtime);
		break;
	case player_laps:
		lua_pushinteger(L, plr->laps);
		break;
	case player_ctfteam:
		lua_pushinteger(L, plr->ctfteam);
		break;
	case player_gotflag:
		lua_pushinteger(L, plr->gotflag);
		break;
	case player_weapondelay:
		lua_pushinteger(L, plr->weapondelay);
		break;
	case player_tossdelay:
		lua_pushinteger(L, plr->tossdelay);
		break;
	case player_starpostx:
		lua_pushinteger(L, plr->starpostx);
		break;
	case player_starposty:
		lua_pushinteger(L, plr->starposty);
		break;
	case player_starpostz:
		lua_pushinteger(L, plr->starpostz);
		break;
	case player_starpostnum:
		lua_pushinteger(L, plr->starpostnum);
		break;
	case player_starposttime:
		lua_pushinteger(L, plr->starposttime);
		break;
	case player_starpostangle:
		lua_pushangle(L, plr->starpostangle);
		break;
	case player_starpostscale:
		lua_pushfixed(L, plr->starpostscale);
		break;
	case player_angle_pos:
		lua_pushangle(L, plr->angle_pos);
		break;
	case player_old_angle_pos:
		lua_pushangle(L, plr->old_angle_pos);
		break;
	case player_axis1:
		LUA_PushUserdata(L, plr->axis1, META_MOBJ);
		break;
	case player_axis2:
		LUA_PushUserdata(L, plr->axis2, META_MOBJ);
		break;
	case player_bumpertime:
		lua_pushinteger(L, plr->bumpertime);
		break;
	case player_flyangle:
		lua_pushinteger(L, plr->flyangle);
		break;
	case player_drilltimer:
		lua_pushinteger(L, plr->drilltimer);
		break;
	case player_linkcount:
		lua_pushinteger(L, plr->linkcount);
		break;
	case player_linktimer:
		lua_pushinteger(L, plr->linktimer);
		break;
	case player_anotherflyangle:
		lua_pushinteger(L, plr->anotherflyangle);
		break;
	case player_nightstime:
		lua_pushinteger(L, plr->nightstime);
		break;
	case player_drillmeter:
		lua_pushinteger(L, plr->drillmeter);
		break;
	case player_drilldelay:
		lua_pushinteger(L, plr->drilldelay);
		break;
	case player_bonustime:
		lua_pushboolean(L, plr->bonustime);
		break;
	case player_capsule:
		LUA_PushUserdata(L, plr->capsule, META_MOBJ);
		break;
	case player_drone:
		LUA_PushUserdata(L, plr->drone, META_MOBJ);
		break;
	case player_oldscale:
		lua_pushfixed(L, plr->oldscale);
		break;
	case player_mare:
		lua_pushinteger(L, plr->mare);
		break;
	case player_marelap:
		lua_pushinteger(L, plr->marelap);
		break;
	case player_marebonuslap:
		lua_pushinteger(L, plr->marebonuslap);
		break;
	case player_marebegunat:
		lua_pushinteger(L, plr->marebegunat);
		break;
	case player_startedtime:
		lua_pushinteger(L, plr->startedtime);
		break;
	case player_finishedtime:
		lua_pushinteger(L, plr->finishedtime);
		break;
	case player_lapbegunat:
		lua_pushinteger(L, plr->lapbegunat);
		break;
	case player_lapstartedtime:
		lua_pushinteger(L, plr->lapstartedtime);
		break;
	case player_finishedspheres:
		lua_pushinteger(L, plr->finishedspheres);
		break;
	case player_finishedrings:
		lua_pushinteger(L, plr->finishedrings);
		break;
	case player_marescore:
		lua_pushinteger(L, plr->marescore);
		break;
	case player_lastmarescore:
		lua_pushinteger(L, plr->lastmarescore);
		break;
	case player_totalmarescore:
		lua_pushinteger(L, plr->totalmarescore);
		break;
	case player_lastmare:
		lua_pushinteger(L, plr->lastmare);
		break;
	case player_lastmarelap:
		lua_pushinteger(L, plr->lastmarelap);
		break;
	case player_lastmarebonuslap:
		lua_pushinteger(L, plr->lastmarebonuslap);
		break;
	case player_totalmarelap:
		lua_pushinteger(L, plr->totalmarelap);
		break;
	case player_totalmarebonuslap:
		lua_pushinteger(L, plr->totalmarebonuslap);
		break;
	case player_maxlink:
		lua_pushinteger(L, plr->maxlink);
		break;
	case player_texttimer:
		lua_pushinteger(L, plr->texttimer);
		break;
	case player_textvar:
		lua_pushinteger(L, plr->textvar);
		break;
	case player_lastsidehit:
		lua_pushinteger(L, plr->lastsidehit);
		break;
	case player_lastlinehit:
		lua_pushinteger(L, plr->lastlinehit);
		break;
	case player_losstime:
		lua_pushinteger(L, plr->losstime);
		break;
	case player_timeshit:
		lua_pushinteger(L, plr->timeshit);
		break;
	case player_onconveyor:
		lua_pushinteger(L, plr->onconveyor);
		break;
	case player_awayviewmobj:
		LUA_PushUserdata(L, plr->awayviewmobj, META_MOBJ);
		break;
	case player_awayviewtics:
		lua_pushinteger(L, plr->awayviewtics);
		break;
	case player_awayviewaiming:
		lua_pushangle(L, plr->awayviewaiming);
		break;
	case player_spectator:
		lua_pushboolean(L, plr->spectator);
		break;
	case player_outofcoop:
		lua_pushboolean(L, plr->outofcoop);
		break;
	case player_bot:
		lua_pushinteger(L, plr->bot);
		break;
	case player_botleader:
		LUA_PushUserdata(L, plr->botleader, META_PLAYER);
		break;
	case player_lastbuttons:
		lua_pushinteger(L, plr->lastbuttons);
		break;
	case player_blocked:
		lua_pushboolean(L, plr->blocked);
		break;
	case player_jointime:
		lua_pushinteger(L, plr->jointime);
		break;
	case player_quittime:
		lua_pushinteger(L, plr->quittime);
		break;
	case player_ping:
		lua_pushinteger(L, playerpingtable[plr - players]);
		break;
#ifdef HWRENDER
	case player_fovadd:
		lua_pushfixed(L, plr->fovadd);
		break;
#endif
	default:
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, plr);
		lua_rawget(L, -2);
		if (!lua_istable(L, -1)) { // no extra values table
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no extvars table or field named '%s'; returning nil.\n"), "player_t", lua_tostring(L, 2));
			return 0;
		}
		lua_pushvalue(L, 2); // field name
		lua_gettable(L, -2);
		if (lua_isnil(L, -1)) // no value for this field
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; returning nil.\n"), "player_t", lua_tostring(L, 2));
		break;
	}

	return 1;
}

#define NOSET luaL_error(L, LUA_QL("player_t") " field " LUA_QS " should not be set directly.", player_opt[field])
static int player_set(lua_State *L)
{
	player_t *plr = *((player_t **)luaL_checkudata(L, 1, META_PLAYER));
	enum player_e field = Lua_optoption(L, 2, player_cmd, player_fields_ref);
	if (!plr)
		return LUA_ErrInvalid(L, "player_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in CMD building code!");

	switch (field)
	{
	case player_mo:
	case player_realmo:
	{
		mobj_t *newmo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		plr->mo->player = NULL; // remove player pointer from old mobj
		(newmo->player = plr)->mo = newmo; // set player pointer for new mobj, and set new mobj as the player's mobj
		break;
	}
	case player_cmd:
		return NOSET;
	case player_playerstate:
		plr->playerstate = luaL_checkinteger(L, 3);
		break;
	case player_camerascale:
		plr->camerascale = luaL_checkfixed(L, 3);
		break;
	case player_shieldscale:
		plr->shieldscale = luaL_checkfixed(L, 3);
		break;
	case player_viewz:
		plr->viewz = luaL_checkfixed(L, 3);
		break;
	case player_viewheight:
		plr->viewheight = luaL_checkfixed(L, 3);
		break;
	case player_deltaviewheight:
		plr->deltaviewheight = luaL_checkfixed(L, 3);
		break;
	case player_bob:
		plr->bob = luaL_checkfixed(L, 3);
		break;
	case player_viewrollangle:
		plr->viewrollangle = luaL_checkangle(L, 3);
		break;
	case player_aiming:
	{
		plr->aiming = luaL_checkangle(L, 3);
		if (plr == &players[consoleplayer])
			localaiming = plr->aiming;
		else if (plr == &players[secondarydisplayplayer])
			localaiming2 = plr->aiming;
		break;
	}
	case player_drawangle:
		plr->drawangle = luaL_checkangle(L, 3);
		break;
	case player_rings:
		plr->rings = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_spheres:
		plr->spheres = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_pity:
		plr->pity = (SINT8)luaL_checkinteger(L, 3);
		break;
	case player_currentweapon:
		plr->currentweapon = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_ringweapons:
		plr->ringweapons = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_ammoremoval:
		plr->ammoremoval = (UINT16)luaL_checkinteger(L, 3);
		break;
	case player_ammoremovaltimer:
		plr->ammoremovaltimer = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_ammoremovalweapon:
		plr->ammoremovalweapon = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_powers:
		return NOSET;
	case player_pflags:
		plr->pflags = luaL_checkinteger(L, 3);
		break;
	case player_panim:
		plr->panim = luaL_checkinteger(L, 3);
		break;
	case player_flashcount:
		plr->flashcount = (UINT16)luaL_checkinteger(L, 3);
		break;
	case player_flashpal:
		plr->flashpal = (UINT16)luaL_checkinteger(L, 3);
		break;
	case player_skincolor:
	{
		UINT16 newcolor = (UINT16)luaL_checkinteger(L,3);
		if (newcolor >= numskincolors)
			return luaL_error(L, "player.skincolor %d out of range (0 - %d).", newcolor, numskincolors-1);
		plr->skincolor = newcolor;
		break;
	}
	case player_skin:
		return NOSET;
	case player_availabilities:
		return NOSET;
	case player_score:
		plr->score = (UINT32)luaL_checkinteger(L, 3);
		break;
	case player_recordscore:
		plr->recordscore = (UINT32)luaL_checkinteger(L, 3);
		break;
	case player_dashspeed:
		plr->dashspeed = luaL_checkfixed(L, 3);
		break;
	case player_normalspeed:
		plr->normalspeed = luaL_checkfixed(L, 3);
		break;
	case player_runspeed:
		plr->runspeed = luaL_checkfixed(L, 3);
		break;
	case player_thrustfactor:
		plr->thrustfactor = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_accelstart:
		plr->accelstart = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_acceleration:
		plr->acceleration = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_charability:
		plr->charability = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_charability2:
		plr->charability2 = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_charflags:
		plr->charflags = (UINT32)luaL_checkinteger(L, 3);
		break;
	case player_thokitem:
		plr->thokitem = luaL_checkinteger(L, 3);
		break;
	case player_spinitem:
		plr->spinitem = luaL_checkinteger(L, 3);
		break;
	case player_revitem:
		plr->revitem = luaL_checkinteger(L, 3);
		break;
	case player_followitem:
		plr->followitem = luaL_checkinteger(L, 3);
		break;
	case player_followmobj:
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->followmobj, mo);
		break;
	}
	case player_actionspd:
		plr->actionspd = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_mindash:
		plr->mindash = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_maxdash:
		plr->maxdash = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_jumpfactor:
		plr->jumpfactor = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_height:
		plr->height = luaL_checkfixed(L, 3);
		break;
	case player_spinheight:
		plr->spinheight = luaL_checkfixed(L, 3);
		break;
	case player_lives:
		plr->lives = (SINT8)luaL_checkinteger(L, 3);
		break;
	case player_continues:
		plr->continues = (SINT8)luaL_checkinteger(L, 3);
		break;
	case player_xtralife:
		plr->xtralife = (SINT8)luaL_checkinteger(L, 3);
		break;
	case player_gotcontinue:
		plr->gotcontinue = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_speed:
		plr->speed = luaL_checkfixed(L, 3);
		break;
	case player_secondjump:
		plr->secondjump = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_fly1:
		plr->fly1 = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_scoreadd:
		plr->scoreadd = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_glidetime:
		plr->glidetime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_climbing:
		plr->climbing = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_deadtimer:
		plr->deadtimer = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_exiting:
		plr->exiting = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_homing:
		plr->homing = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_dashmode:
		plr->dashmode = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_skidtime:
		plr->skidtime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_cmomx:
		plr->cmomx = luaL_checkfixed(L, 3);
		break;
	case player_cmomy:
		plr->cmomy = luaL_checkfixed(L, 3);
		break;
	case player_rmomx:
		plr->rmomx = luaL_checkfixed(L, 3);
		break;
	case player_rmomy:
		plr->rmomy = luaL_checkfixed(L, 3);
		break;
	case player_numboxes:
		plr->numboxes = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_totalring:
		plr->totalring = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_realtime:
		plr->realtime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_laps:
		plr->laps = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_ctfteam:
		plr->ctfteam = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_gotflag:
		plr->gotflag = (UINT16)luaL_checkinteger(L, 3);
		break;
	case player_weapondelay:
		plr->weapondelay = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_tossdelay:
		plr->tossdelay = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_starpostx:
		plr->starpostx = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_starposty:
		plr->starposty = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_starpostz:
		plr->starpostz = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_starpostnum:
		plr->starpostnum = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_starposttime:
		plr->starposttime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_starpostangle:
		plr->starpostangle = luaL_checkangle(L, 3);
		break;
	case player_starpostscale:
		plr->starpostscale = luaL_checkfixed(L, 3);
		break;
	case player_angle_pos:
		plr->angle_pos = luaL_checkangle(L, 3);
		break;
	case player_old_angle_pos:
		plr->old_angle_pos = luaL_checkangle(L, 3);
		break;
	case player_axis1:
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->axis1, mo);
		break;
	}
	case player_axis2:
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->axis2, mo);
		break;
	}
	case player_bumpertime:
		plr->bumpertime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_flyangle:
		plr->flyangle = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_drilltimer:
		plr->drilltimer = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_linkcount:
		plr->linkcount = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_linktimer:
		plr->linktimer = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_anotherflyangle:
		plr->anotherflyangle = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_nightstime:
		plr->nightstime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_drillmeter:
		plr->drillmeter = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_drilldelay:
		plr->drilldelay = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_bonustime:
		plr->bonustime = luaL_checkboolean(L, 3);
		break;
	case player_capsule:
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->capsule, mo);
		break;
	}
	case player_drone:
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		P_SetTarget(&plr->drone, mo);
		break;
	}
	case player_oldscale:
		plr->oldscale = luaL_checkfixed(L, 3);
		break;
	case player_mare:
		plr->mare = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_marelap:
		plr->marelap = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_marebonuslap:
		plr->marebonuslap = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_marebegunat:
		plr->marebegunat = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_startedtime:
		plr->startedtime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_finishedtime:
		plr->finishedtime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_lapbegunat:
		plr->lapbegunat = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_lapstartedtime:
		plr->lapstartedtime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_finishedspheres:
		plr->finishedspheres = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_finishedrings:
		plr->finishedrings = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_marescore:
		plr->marescore = (UINT32)luaL_checkinteger(L, 3);
		break;
	case player_lastmarescore:
		plr->lastmarescore = (UINT32)luaL_checkinteger(L, 3);
		break;
	case player_totalmarescore:
		plr->totalmarescore = (UINT32)luaL_checkinteger(L, 3);
		break;
	case player_lastmare:
		plr->lastmare = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_lastmarelap:
		plr->lastmarelap = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_lastmarebonuslap:
		plr->lastmarebonuslap = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_totalmarelap:
		plr->totalmarelap = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_totalmarebonuslap:
		plr->totalmarebonuslap = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_maxlink:
		plr->maxlink = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_texttimer:
		plr->texttimer = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_textvar:
		plr->textvar = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_lastsidehit:
		plr->lastsidehit = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_lastlinehit:
		plr->lastlinehit = (INT16)luaL_checkinteger(L, 3);
		break;
	case player_losstime:
		plr->losstime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_timeshit:
		plr->timeshit = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_onconveyor:
		plr->onconveyor = (INT32)luaL_checkinteger(L, 3);
		break;
	case player_awayviewmobj:
	{
		mobj_t *mo = NULL;
		if (!lua_isnil(L, 3))
			mo = *((mobj_t **)luaL_checkudata(L, 3, META_MOBJ));
		if (plr->awayviewmobj != mo) {
			P_SetTarget(&plr->awayviewmobj, mo);
			if (plr->awayviewtics) {
				if (!plr->awayviewmobj)
					plr->awayviewtics = 0; // can't have a NULL awayviewmobj with awayviewtics!
				if (plr == &players[displayplayer])
					P_ResetCamera(plr, &camera); // reset p1 camera on p1 getting an awayviewmobj
				else if (splitscreen && plr == &players[secondarydisplayplayer])
					P_ResetCamera(plr, &camera2);  // reset p2 camera on p2 getting an awayviewmobj
			}
		}
		break;
	}
	case player_awayviewtics:
	{
		INT32 tics = (INT32)luaL_checkinteger(L, 3);
		if (tics && !plr->awayviewmobj) // awayviewtics must ALWAYS have an awayviewmobj set!!
			P_SetTarget(&plr->awayviewmobj, plr->mo); // but since the script might set awayviewmobj immediately AFTER setting awayviewtics, use player mobj as filler for now.
		if ((tics && !plr->awayviewtics) || (!tics && plr->awayviewtics)) {
			if (plr == &players[displayplayer])
				P_ResetCamera(plr, &camera); // reset p1 camera on p1 transitioning to/from zero awayviewtics
			else if (splitscreen && plr == &players[secondarydisplayplayer])
				P_ResetCamera(plr, &camera2);  // reset p2 camera on p2 transitioning to/from zero awayviewtics
		}
		plr->awayviewtics = tics;
		break;
	}
	case player_awayviewaiming:
		plr->awayviewaiming = luaL_checkangle(L, 3);
		break;
	case player_spectator:
		plr->spectator = lua_toboolean(L, 3);
		break;
	case player_outofcoop:
		plr->outofcoop = lua_toboolean(L, 3);
		break;
	case player_bot:
		return NOSET;
	case player_botleader:
	{
		player_t *player = NULL;
		if (!lua_isnil(L, 3))
			player = *((player_t **)luaL_checkudata(L, 3, META_PLAYER));
		plr->botleader = player;
		break;
	}
	case player_lastbuttons:
		plr->lastbuttons = (UINT16)luaL_checkinteger(L, 3);
		break;
	case player_blocked:
		plr->blocked = (UINT8)luaL_checkinteger(L, 3);
		break;
	case player_jointime:
		plr->jointime = (tic_t)luaL_checkinteger(L, 3);
		break;
	case player_quittime:
		plr->quittime = (tic_t)luaL_checkinteger(L, 3);
		break;
#ifdef HWRENDER
	case player_fovadd:
		plr->fovadd = luaL_checkfixed(L, 3);
		break;
#endif
	default:
		lua_getfield(L, LUA_REGISTRYINDEX, LREG_EXTVARS);
		I_Assert(lua_istable(L, -1));
		lua_pushlightuserdata(L, plr);
		lua_rawget(L, -2);
		if (lua_isnil(L, -1)) {
			// This index doesn't have a table for extra values yet, let's make one.
			lua_pop(L, 1);
			CONS_Debug(DBG_LUA, M_GetText("'%s' has no field named '%s'; adding it as Lua data.\n"), "player_t", lua_tostring(L, 2));
			lua_newtable(L);
			lua_pushlightuserdata(L, plr);
			lua_pushvalue(L, -2); // ext value table
			lua_rawset(L, -4); // LREG_EXTVARS table
		}
		lua_pushvalue(L, 2); // key
		lua_pushvalue(L, 3); // value to store
		lua_settable(L, -3);
		lua_pop(L, 2);
		break;
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
		return luaL_error(L, LUA_QL("powertype_t") " cannot be %d", (INT16)p);
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
		return luaL_error(L, LUA_QL("powertype_t") " cannot be %d", (INT16)p);
	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");
	if (hook_cmd_running)
		return luaL_error(L, "Do not alter player_t in CMD building code!");
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
#define NOSET luaL_error(L, LUA_QL("ticcmd_t") " field " LUA_QS " should not be set directly.", ticcmd_opt[field])

enum ticcmd_e
{
	ticcmd_forwardmove,
	ticcmd_sidemove,
	ticcmd_angleturn,
	ticcmd_aiming,
	ticcmd_buttons,
	ticcmd_latency,
};

static const char *const ticcmd_opt[] = {
	"forwardmove",
	"sidemove",
	"angleturn",
	"aiming",
	"buttons",
	"latency",
	NULL,
};

static int ticcmd_fields_ref = LUA_NOREF;

static int ticcmd_get(lua_State *L)
{
	ticcmd_t *cmd = *((ticcmd_t **)luaL_checkudata(L, 1, META_TICCMD));
	enum ticcmd_e field = Lua_optoption(L, 2, -1, ticcmd_fields_ref);
	if (!cmd)
		return LUA_ErrInvalid(L, "player_t");

	switch (field)
	{
	case ticcmd_forwardmove:
		lua_pushinteger(L, cmd->forwardmove);
		break;
	case ticcmd_sidemove:
		lua_pushinteger(L, cmd->sidemove);
		break;
	case ticcmd_angleturn:
		lua_pushinteger(L, cmd->angleturn);
		break;
	case ticcmd_aiming:
		lua_pushinteger(L, cmd->aiming);
		break;
	case ticcmd_buttons:
		lua_pushinteger(L, cmd->buttons);
		break;
	case ticcmd_latency:
		lua_pushinteger(L, cmd->latency);
		break;
	default:
		return NOFIELD;
	}

	return 1;
}

static int ticcmd_set(lua_State *L)
{
	ticcmd_t *cmd = *((ticcmd_t **)luaL_checkudata(L, 1, META_TICCMD));
	enum ticcmd_e field = Lua_optoption(L, 2, -1, ticcmd_fields_ref);
	if (!cmd)
		return LUA_ErrInvalid(L, "ticcmd_t");

	if (hud_running)
		return luaL_error(L, "Do not alter player_t in HUD rendering code!");

	switch (field)
	{
	case ticcmd_forwardmove:
		cmd->forwardmove = (SINT8)luaL_checkinteger(L, 3);
		break;
	case ticcmd_sidemove:
		cmd->sidemove = (SINT8)luaL_checkinteger(L, 3);
		break;
	case ticcmd_angleturn:
		cmd->angleturn = (INT16)luaL_checkinteger(L, 3);
		break;
	case ticcmd_aiming:
		cmd->aiming = (INT16)luaL_checkinteger(L, 3);
		break;
	case ticcmd_buttons:
		cmd->buttons = (UINT16)luaL_checkinteger(L, 3);
		break;
	case ticcmd_latency:
		return NOSET;
	default:
		return NOFIELD;
	}

	return 0;
}

#undef NOFIELD
#undef NOSET

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

	player_fields_ref = Lua_CreateFieldTable(L, player_opt);

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

	ticcmd_fields_ref = Lua_CreateFieldTable(L, ticcmd_opt);

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
