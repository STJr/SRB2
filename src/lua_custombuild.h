// SONIC ROBO BLAST 2; TSOURDT3RD
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Star "Guy Who Names Scripts After Him" ManiaKG.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_custombuild.h
/// \brief Custom Build BLua stuff

#ifndef __TAKIS_LUA__
#define __TAKIS_LUA__

#include "lua_script.h"

extern boolean takis_custombuild; 
INT32 Takis_PushGlobals(lua_State *L, const char *word);

#endif // __TAKIS_LUA__