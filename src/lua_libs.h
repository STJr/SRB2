// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_libs.h
/// \brief libraries for Lua scripting

#ifdef HAVE_BLUA

extern lua_State *gL;

#define LREG_VALID "VALID_USERDATA"
#define LREG_EXTVARS "LUA_VARS"
#define LREG_STATEACTION "STATE_ACTION"
#define LREG_ACTIONS "MOBJ_ACTION"

#define META_STATE "STATE_T*"
#define META_MOBJINFO "MOBJINFO_T*"
#define META_SFXINFO "SFXINFO_T*"
#define META_MUSICINFO "MUSICINFO_T*"

#define META_MOBJ "MOBJ_T*"
#define META_MAPTHING "MAPTHING_T*"

#define META_PLAYER "PLAYER_T*"
#define META_TICCMD "TICCMD_T*"
#define META_SKIN "SKIN_T*"
#define META_POWERS "PLAYER_T*POWERS"

#define META_VERTEX "VERTEX_T*"
#define META_LINE "LINE_T*"
#define META_SIDE "SIDE_T*"
#define META_SUBSECTOR "SUBSECTOR_T*"
#define META_SECTOR "SECTOR_T*"

#define META_CVAR "CONSVAR_T*"

#define META_SIDENUM "LINE_T*SIDENUM"

#define META_HUDINFO "HUDINFO_T*"
#define META_PATCH "PATCH_T*"
#define META_COLORMAP "COLORMAP"

boolean luaL_checkboolean(lua_State *L, int narg);

int LUA_EnumLib(lua_State *L);
int LUA_SOCLib(lua_State *L);
int LUA_BaseLib(lua_State *L);
int LUA_MathLib(lua_State *L);
int LUA_HookLib(lua_State *L);
int LUA_ConsoleLib(lua_State *L);
int LUA_InfoLib(lua_State *L);
int LUA_MobjLib(lua_State *L);
int LUA_PlayerLib(lua_State *L);
int LUA_SkinLib(lua_State *L);
int LUA_ThinkerLib(lua_State *L);
int LUA_MapLib(lua_State *L);
int LUA_HudLib(lua_State *L);

#endif
