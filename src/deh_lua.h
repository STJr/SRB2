// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_lua.h
/// \brief Lua SOC library

#ifndef __DEH_LUA_H__
#define __DEH_LUA_H__

#include "g_game.h"
#include "s_sound.h"
#include "z_zone.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_local.h"
#include "st_stuff.h"
#include "r_translation.h"
#include "fastcmp.h"
#include "lua_script.h"
#include "lua_libs.h"

#include "dehacked.h"
#include "deh_tables.h"

void LUA_UpdateSprName(const char *name, lua_Integer value);
boolean LUA_SetLuaAction(void *state, const char *actiontocompare);
const char *LUA_GetActionName(void *action);
void LUA_SetActionByName(void *state, const char *actiontocompare);
enum actionnum LUA_GetActionNumByName(const char *actiontocompare);

#endif
