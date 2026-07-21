// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_archive.h
/// \brief Lua gamestate archival

#ifndef LUA_ARCHIVE_H
#define LUA_ARCHIVE_H

#include "lua_script.h"
#include "p_saveg.h"

void LUA_Archive(save_t *save_p);
void LUA_UnArchive(save_t *save_p);
void LUA_HookNetArchive(lua_CFunction archFunc);

#endif/*LUA_ARCHIVE_H*/
