// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_script.h
/// \brief Lua scripting basics

#ifndef LUA_SCRIPT_H
#define LUA_SCRIPT_H

#include "m_fixed.h"
#include "doomtype.h"
#include "d_player.h"
#include "g_state.h"
#include "taglist.h"

#include "blua/lua.h"
#include "blua/lualib.h"
#include "blua/lauxlib.h"

#define lua_optboolean(L, i) (!lua_isnoneornil(L, i) && lua_toboolean(L, i))
#define lua_opttrueboolean(L, i) (lua_isnoneornil(L, i) || lua_toboolean(L, i))

// fixed_t casting
// TODO add some distinction between fixed numbers and integer numbers
// for at least the purpose of printing and maybe math.
#define luaL_checkfixed(L, i) luaL_checkinteger(L, i)
#define lua_pushfixed(L, f) lua_pushinteger(L, f)

// angle_t casting
// TODO deal with signedness
#define luaL_checkangle(L, i) ((angle_t)luaL_checkinteger(L, i))
#define lua_pushangle(L, a) lua_pushinteger(L, a)

#ifdef _DEBUG
void LUA_ClearExtVars(void);
#endif

extern INT32 lua_lumploading; // is LUA_LoadLump being called?

int LUA_GetErrorMessage(lua_State *L);
int LUA_Call(lua_State *L, int nargs, int nresults, int errorhandlerindex);
void LUA_LoadLump(UINT16 wad, UINT16 lump, boolean noresults);
#ifdef LUA_ALLOW_BYTECODE
void LUA_DumpFile(const char *filename);
#endif
fixed_t LUA_EvalMath(const char *word);
void LUA_Step(void);
void LUA_Archive(void);
void LUA_UnArchive(void);
int LUA_PushGlobals(lua_State *L, const char *word);
int LUA_CheckGlobals(lua_State *L, const char *word);
void Got_Luacmd(UINT8 **cp, INT32 playernum); // lua_consolelib.c
void LUA_CVarChanged(void *cvar); // lua_consolelib.c
int Lua_optoption(lua_State *L, int narg,
	const char *def, const char *const lst[]);
void LUAh_NetArchiveHook(lua_CFunction archFunc);

void LUA_PushTaggableObjectArray
(		lua_State *L,
		const char *field,
		lua_CFunction iterator,
		lua_CFunction indexer,
		lua_CFunction counter,
		taggroup_t *garray[],
		size_t * max_elements,
		void * element_array,
		size_t sizeof_element,
		const char *meta);

void LUA_InsertTaggroupIterator
(		lua_State *L,
		taggroup_t *garray[],
		size_t * max_elements,
		void * element_array,
		size_t sizeof_element,
		const char * meta);

typedef enum {
	LPUSHED_NIL,
	LPUSHED_NEW,
	LPUSHED_EXISTING,
} lpushed_t;

void LUA_PushUserdata(lua_State *L, void *data, const char *meta);
lpushed_t LUA_RawPushUserdata(lua_State *L, void *data);

void LUA_InvalidateUserdata(void *data);

void LUA_InvalidateLevel(void);
void LUA_InvalidateMapthings(void);
void LUA_InvalidatePlayer(player_t *player);

// Console wrapper
void COM_Lua_f(void);

#define LUA_ErrInvalid(L, type) luaL_error(L, "accessed " type " doesn't exist anymore, please check 'valid' before using " type ".");

#define LUA_ErrSetDirectly(L, type, field) luaL_error(L, type " field " LUA_QL(field) " cannot be set directly.")

// Deprecation warnings
// Shows once upon use. Then doesn't show again.
#define LUA_Deprecated(L,this_func,use_instead)\
{\
	static UINT8 seen = 0;\
	if (!seen) {\
		seen = 1;\
		CONS_Alert(CONS_WARNING,"\"%s\" is deprecated and will be removed.\nUse \"%s\" instead.\n", this_func, use_instead);\
	}\
}

// Warnings about incorrect function usage.
// Shows once, then never again, like deprecation
#define LUA_UsageWarning(L, warningmsg)\
{\
	static UINT8 seen = 0;\
	if (!seen) {\
		seen = 1;\
		CONS_Alert(CONS_WARNING,"%s\n", warningmsg);\
	}\
}

// uncomment if you want seg_t/node_t in Lua
// #define HAVE_LUA_SEGS

#define ISINLEVEL \
	(gamestate == GS_LEVEL || titlemapinaction)

#define INLEVEL if (! ISINLEVEL)\
return luaL_error(L, "This can only be used in a level!");

#endif/*LUA_SCRIPT_H*/
