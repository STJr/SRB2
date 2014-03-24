// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2014      by John "JTE" Muniz.
// Copyright (C) 2014      by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_hudlib.c
/// \brief custom HUD rendering library for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
#include "r_defs.h"
#include "st_stuff.h" // hudinfo[]
#include "g_game.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"

boolean hud_running = false;
static UINT8 hud_enabled[(hud_MAX/8)+1];

// must match enum hud in lua_hud.h
static const char *const hud_disable_options[] = {
	"stagetitle",
	"textspectator",

	"score",
	"time",
	"rings",
	"lives",

	"nightslink",
	"nightsdrill",
	"nightsrings",
	"nightsscore",
	"nightstime",
	"nightsrecords",

	"rankings",
	"coopemeralds",
	"tokens",
	"tabemblems",
	NULL};

enum hudinfo {
	hudinfo_x = 0,
	hudinfo_y
};

static const char *const hudinfo_opt[] = {
	"x",
	"y",
	NULL};

enum patch {
	patch_valid = 0,
	patch_width,
	patch_height,
	patch_leftoffset,
	patch_topoffset
};
static const char *const patch_opt[] = {
	"valid",
	"width",
	"height",
	"leftoffset",
	"topoffset",
	NULL};

enum hudhook {
	hudhook_game = 0,
	hudhook_scores
};
static const char *const hudhook_opt[] = {
	"game",
	"scores",
	NULL};

enum align {
	align_left = 0,
	align_center,
	align_right,
	align_fixed
};
static const char *const align_opt[] = {
	"left",
	"center",
	"right",
	"fixed",
	NULL};

static int lib_getHudInfo(lua_State *L)
{
	// arg 1 is empty table with this metatable set
	lua_Integer mindex = luaL_checkinteger(L,2);
	hudinfo_t **userdata = lua_newuserdata(L, sizeof(hudinfo_t *));
	luaL_getmetatable(L, META_HUDINFO);
	lua_setmetatable(L, -2);
	*userdata = &hudinfo[mindex];
	return 1;
}

static int lib_hudinfolen(lua_State *L)
{
	lua_pushinteger(L, NUMHUDITEMS);
	return 1;
}

static int hudinfo_get(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	enum hudinfo field = luaL_checkoption(L, 2, hudinfo_opt[0], hudinfo_opt);
	I_Assert(info != NULL); // huditems are always valid

	switch(field)
	{
	case hudinfo_x:
		lua_pushinteger(L, info->x);
		break;
	case hudinfo_y:
		lua_pushinteger(L, info->y);
		break;
	}
	return 1;
}

static int hudinfo_set(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	enum hudinfo field = luaL_checkoption(L, 2, hudinfo_opt[0], hudinfo_opt);
	I_Assert(info != NULL);

	switch(field)
	{
	case hudinfo_x:
		info->x = (INT32)luaL_checkinteger(L, 3);
		break;
	case hudinfo_y:
		info->y = (INT32)luaL_checkinteger(L, 3);
		break;
	}
	return 0;
}

static int hudinfo_num(lua_State *L)
{
	hudinfo_t *info = *((hudinfo_t **)luaL_checkudata(L, 1, META_HUDINFO));
	lua_pushinteger(L, info-hudinfo);
	return 1;
}

static int patch_get(lua_State *L)
{
	patch_t *patch = *((patch_t **)luaL_checkudata(L, 1, META_PATCH));
	enum patch field = luaL_checkoption(L, 2, NULL, patch_opt);

	// patches are CURRENTLY always valid, expected to be cached with PU_STATIC
	// this may change in the future, so patch.valid still exists
	I_Assert(patch != NULL);

	switch (field)
	{
	case patch_valid:
		lua_pushboolean(L, patch != NULL);
		break;
	case patch_width:
		lua_pushinteger(L, SHORT(patch->width));
		break;
	case patch_height:
		lua_pushinteger(L, SHORT(patch->height));
		break;
	case patch_leftoffset:
		lua_pushinteger(L, SHORT(patch->leftoffset));
		break;
	case patch_topoffset:
		lua_pushinteger(L, SHORT(patch->topoffset));
		break;
	}
	return 1;
}

static int patch_set(lua_State *L)
{
	return luaL_error(L, LUA_QL("patch_t") " struct cannot be edited by Lua.");
}

//
// lib_draw
//

static int libd_patchExists(lua_State *L)
{
	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	lua_pushboolean(L, W_CheckNumForName(luaL_checkstring(L, 1)) != LUMPERROR);
	return 1;
}

static int libd_cachePatch(lua_State *L)
{
	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	LUA_PushUserdata(L, W_CachePatchName(luaL_checkstring(L, 1), PU_STATIC), META_PATCH);
	return 1;
}

static int libd_draw(lua_State *L)
{
	INT32 x, y, flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	patch = *((patch_t **)luaL_checkudata(L, 3, META_PATCH));
	flags = luaL_optinteger(L, 4, 0);
	if (!lua_isnoneornil(L, 5))
		colormap = luaL_checkudata(L, 5, META_COLORMAP);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawFixedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT, flags, patch, colormap);
	return 0;
}

static int libd_drawScaled(lua_State *L)
{
	fixed_t x, y, scale;
	INT32 flags;
	patch_t *patch;
	const UINT8 *colormap = NULL;

	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	scale = luaL_checkinteger(L, 3);
	patch = *((patch_t **)luaL_checkudata(L, 4, META_PATCH));
	flags = luaL_optinteger(L, 5, 0);
	if (!lua_isnoneornil(L, 6))
		colormap = luaL_checkudata(L, 6, META_COLORMAP);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawFixedPatch(x, y, scale, flags, patch, colormap);
	return 0;
}

static int libd_drawNum(lua_State *L)
{
	INT32 x, y, flags, num;
	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = luaL_checkinteger(L, 3);
	flags = luaL_optinteger(L, 4, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawTallNum(x, y, flags, num);
	return 0;
}

static int libd_drawPaddedNum(lua_State *L)
{
	INT32 x, y, flags, num, digits;
	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	x = luaL_checkinteger(L, 1);
	y = luaL_checkinteger(L, 2);
	num = abs(luaL_checkinteger(L, 3));
	digits = luaL_optinteger(L, 4, 2);
	flags = luaL_optinteger(L, 5, 0);
	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	V_DrawPaddedTallNum(x, y, flags, num, digits);
	return 0;
}

static int libd_drawFill(lua_State *L)
{
	INT32 x = luaL_optinteger(L, 1, 0);
	INT32 y = luaL_optinteger(L, 2, 0);
	INT32 w = luaL_optinteger(L, 3, BASEVIDWIDTH);
	INT32 h = luaL_optinteger(L, 4, BASEVIDHEIGHT);
	INT32 c = luaL_optinteger(L, 5, 31);

	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	V_DrawFill(x, y, w, h, c);
	return 0;
}

static int libd_drawString(lua_State *L)
{
	fixed_t x = luaL_checkinteger(L, 1);
	fixed_t y = luaL_checkinteger(L, 2);
	const char *str = luaL_checkstring(L, 3);
	INT32 flags = luaL_optinteger(L, 4, V_ALLOWLOWERCASE);
	enum align align = luaL_checkoption(L, 5, "left", align_opt);

	flags &= ~V_PARAMMASK; // Don't let crashes happen.

	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	switch(align)
	{
	case align_left:
		V_DrawString(x, y, flags, str);
		break;
	case align_center:
		V_DrawCenteredString(x, y, flags, str);
		break;
	case align_right:
		V_DrawRightAlignedString(x, y, flags, str);
		break;
	case align_fixed:
		V_DrawStringAtFixed(x, y, flags, str);
		break;
	}
	return 0;
}

static int libd_stringWidth(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	INT32 flags = luaL_optinteger(L, 2, V_ALLOWLOWERCASE);

	if (!hud_running)
		return luaL_error(L, "HUD rendering code should not be called outside of rendering hooks!");

	lua_pushinteger(L, V_StringWidth(str, flags));
	return 1;
}

static luaL_Reg lib_draw[] = {
	{"patchExists", libd_patchExists},
	{"cachePatch", libd_cachePatch},
	{"draw", libd_draw},
	{"drawScaled", libd_drawScaled},
	{"drawNum", libd_drawNum},
	{"drawPaddedNum", libd_drawPaddedNum},
	{"drawFill", libd_drawFill},
	{"drawString", libd_drawString},
	{"stringWidth", libd_stringWidth},
	{NULL, NULL}
};

//
// lib_hud
//

// enable vanilla HUD element
static int lib_hudenable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] |= 1<<(option%8);
	return 0;
}

// disable vanilla HUD element
static int lib_huddisable(lua_State *L)
{
	enum hud option = luaL_checkoption(L, 1, NULL, hud_disable_options);
	hud_enabled[option/8] &= ~(1<<(option%8));
	return 0;
}

// add a HUD element for rendering
static int lib_hudadd(lua_State *L)
{
	enum hudhook field;

	luaL_checktype(L, 1, LUA_TFUNCTION);
	field = luaL_checkoption(L, 2, "game", hudhook_opt);

	lua_getfield(L, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(L, -1));
	lua_rawgeti(L, -1, field+2); // HUD[2+]
	I_Assert(lua_istable(L, -1));
	lua_remove(L, -2);

	lua_pushvalue(L, 1);
	lua_rawseti(L, -2, (int)(lua_objlen(L, -2) + 1));
	return 0;
}

static luaL_Reg lib_hud[] = {
	{"enable", lib_hudenable},
	{"disable", lib_huddisable},
	{"add", lib_hudadd},
	{NULL, NULL}
};

//
//
//

int LUA_HudLib(lua_State *L)
{
	memset(hud_enabled, 0xff, (hud_MAX/8)+1);

	lua_newtable(L); // HUD registry table
		lua_newtable(L);
		luaL_register(L, NULL, lib_draw);
		lua_rawseti(L, -2, 1); // HUD[1] = lib_draw

		lua_newtable(L);
		lua_rawseti(L, -2, 2); // HUD[2] = game rendering functions array

		lua_newtable(L);
		lua_rawseti(L, -2, 3); // HUD[2] = scores rendering functions array
	lua_setfield(L, LUA_REGISTRYINDEX, "HUD");

	luaL_newmetatable(L, META_HUDINFO);
		lua_pushcfunction(L, hudinfo_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, hudinfo_set);
		lua_setfield(L, -2, "__newindex");

		lua_pushcfunction(L, hudinfo_num);
		lua_setfield(L, -2, "__len");
	lua_pop(L,1);

	lua_newuserdata(L, 0);
		lua_createtable(L, 0, 2);
			lua_pushcfunction(L, lib_getHudInfo);
			lua_setfield(L, -2, "__index");

			lua_pushcfunction(L, lib_hudinfolen);
			lua_setfield(L, -2, "__len");
		lua_setmetatable(L, -2);
	lua_setglobal(L, "hudinfo");

	luaL_newmetatable(L, META_PATCH);
		lua_pushcfunction(L, patch_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, patch_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L,1);

	luaL_register(L, "hud", lib_hud);
	return 0;
}

boolean LUA_HudEnabled(enum hud option)
{
	if (!gL || hud_enabled[option/8] & (1<<(option%8)))
		return true;
	return false;
}

// Hook for HUD rendering
void LUAh_GameHUD(player_t *stplyr)
{
	if (!gL)
		return;

	hud_running = true;
	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 2); // HUD[2] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	LUA_PushUserdata(gL, stplyr, META_PLAYER);

	lua_pushnil(gL);
	while (lua_next(gL, -4) != 0) {
		lua_pushvalue(gL, -4); // graphics library (HUD[1])
		lua_pushvalue(gL, -4); // stplyr
		LUA_Call(gL, 2);
	}
	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCCOLLECT, 0);

	hud_running = false;
}

void LUAh_ScoresHUD(void)
{
	if (!gL)
		return;

	hud_running = true;
	lua_pop(gL, -1);

	lua_getfield(gL, LUA_REGISTRYINDEX, "HUD");
	I_Assert(lua_istable(gL, -1));
	lua_rawgeti(gL, -1, 3); // HUD[3] = rendering funcs
	I_Assert(lua_istable(gL, -1));

	lua_rawgeti(gL, -2, 1); // HUD[1] = lib_draw
	I_Assert(lua_istable(gL, -1));
	lua_remove(gL, -3); // pop HUD
	lua_pushnil(gL);
	while (lua_next(gL, -3) != 0) {
		lua_pushvalue(gL, -3); // graphics library (HUD[1])
		LUA_Call(gL, 1);
	}
	lua_pop(gL, -1);
	lua_gc(gL, LUA_GCCOLLECT, 0);

	hud_running = false;
}

#endif
