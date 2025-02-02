// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014-2016 by John "JTE" Muniz.
// Copyright (C) 2014-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_custombuild.c
/// \brief currently only used for lua scripts to detect this build

#include "doomdef.h"
#include "fastcmp.h"
#include "dehacked.h"
#include "deh_lua.h"
#include "lua_script.h"
#include "lua_libs.h"
#include "lua_custombuild.h"

boolean takis_custombuild = true;

//this build isnt officially called anything, so i'll just use "takis" for now
INT32 Takis_PushGlobals(lua_State *L, const char *word)
{
    if (fastcmp(word,"takis_custombuild")) {
		lua_pushboolean(L, takis_custombuild);
		return 1;
    }
    return 0;
}