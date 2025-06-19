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

static void CheckActiveMovie(lua_State *L)
{
	if (!activemovie)
		luaL_error(L, "no movie currently playing");
}

static int lib_play(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	boolean usedithering = lua_opttrueboolean(L, 2);
	MovieDecode_Stop(&activemovie);
	activemovie = MovieDecode_Play(name, (rendermode == render_soft), usedithering);
	return 0;
}

static int lib_stop(lua_State *L)
{
	CheckActiveMovie(L);
	MovieDecode_Stop(&activemovie);
	return 0;
}

static int lib_setPosition(lua_State *L)
{
	CheckActiveMovie(L);
	int ms = luaL_checkinteger(L, 1);
	if (ms < 0)
		return luaL_error(L, "the position cannot be negative");
	MovieDecode_SetPosition(activemovie, ms);
	return 0;
}

static int lib_seek(lua_State *L)
{
	CheckActiveMovie(L);
	int ms = luaL_checkinteger(L, 1);
	if (ms < 0)
		return luaL_error(L, "the position cannot be negative");
	MovieDecode_Seek(activemovie, ms);
	return 0;
}

static int lib_duration(lua_State *L)
{
	CheckActiveMovie(L);
	lua_pushinteger(L, MovieDecode_GetDuration(activemovie));
	return 1;
}

static int lib_dimensions(lua_State *L)
{
	CheckActiveMovie(L);
	INT32 width, height;
	MovieDecode_GetDimensions(activemovie, &width, &height);
	lua_pushinteger(L, width);
	lua_pushinteger(L, height);
	return 2;
}

static luaL_Reg lib[] = {
	{"play", lib_play},
	{"stop", lib_stop},
	{"setPosition", lib_setPosition},
	{"seek", lib_seek},
	{"duration", lib_duration},
	{"dimensions", lib_dimensions},
	{NULL, NULL}
};

int LUA_MovieLib(lua_State *L)
{
	luaL_register(L, "movie", lib);
	return 0;
}
