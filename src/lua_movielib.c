// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_movielib.c
/// \brief movie library for Lua scripting

#include "lua_script.h"
#include "lua_libs.h"
#include "movie_decode.h"
#include "d_main.h"
#include "i_video.h"

///////////////
// FUNCTIONS //
///////////////

static int lib_play(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	if (activemovie)
		return luaL_error(L, "movie already playing");
	activemovie = MovieDecode_Play(name, (rendermode == render_soft));
	return 0;
}

static int lib_stop(lua_State *L)
{
	if (!activemovie)
		return luaL_error(L, "no movie currently playing");
	MovieDecode_Stop(&activemovie);
	return 0;
}

static int lib_seek(lua_State *L)
{
	if (!activemovie)
		return luaL_error(L, "no movie currently playing");
	int tic = luaL_checkinteger(L, 1);
	MovieDecode_Seek(activemovie, tic);
	return 0;
}

static int lib_duration(lua_State *L)
{
	if (!activemovie)
		return luaL_error(L, "no movie currently playing");
	lua_pushinteger(L, MovieDecode_GetDuration(activemovie));
	return 1;
}

static luaL_Reg lib[] = {
	{"play", lib_play},
	{"stop", lib_stop},
	{"seek", lib_seek},
	{"duration", lib_duration},
	{NULL, NULL}
};

int LUA_MovieLib(lua_State *L)
{
	luaL_register(L, "movie", lib);
	return 0;
}
