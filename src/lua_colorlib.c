// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021-2022 by "Lactozilla".
// Copyright (C) 2014-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_colorlib.c
/// \brief color and colormap libraries for Lua scripting

#include "doomdef.h"
#include "fastcmp.h"
#include "r_data.h"

#include "lua_script.h"
#include "lua_libs.h"

static UINT8 GetRGBAColorsFromTable(lua_State *L, UINT32 index, UINT8 *rgba, boolean useAlpha)
{
	UINT8 num = 0;

	lua_pushnil(L);

	while (lua_next(L, index)) {
		lua_Integer i = 0;
		const char *field = NULL;
		if (lua_isnumber(L, -2))
			i = lua_tointeger(L, -2);
		else
			field = luaL_checkstring(L, -2);

#define CHECKFIELD(p, c) (i == p || (field && fastcmp(field, c)))
#define RGBASET(p, c) { \
			INT32 color = luaL_checkinteger(L, -1); \
			if (color < 0 || color > 255) \
				luaL_error(L, c " channel %d out of range (0 - 255)", color); \
			rgba[p] = (UINT8)color; \
			num++; \
		}

		if (CHECKFIELD(1, "r")) {
			RGBASET(0, "red color");
		} else if (CHECKFIELD(2, "g")) {
			RGBASET(1, "green color");
		} else if (CHECKFIELD(3, "b")) {
			RGBASET(2, "blue color");
		} else if (useAlpha && CHECKFIELD(4, "a")) {
			RGBASET(3, "alpha");
		}

#undef CHECKFIELD
#undef RGBASET

		lua_pop(L, 1);
	}

	return num;
}

#define IS_HEX_CHAR(x) ((x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'))
#define ARE_HEX_CHARS(str, i) IS_HEX_CHAR(str[i]) && IS_HEX_CHAR(str[i + 1])

static UINT32 hex2int(char x)
{
	if (x >= '0' && x <= '9')
		return x - '0';
	else if (x >= 'a' && x <= 'f')
		return x - 'a' + 10;
	else if (x >= 'A' && x <= 'F')
		return x - 'A' + 10;

	return 0;
}

static UINT8 ParseHTMLColor(const char *str, UINT8 *rgba, size_t numc)
{
	const char *hex = str;

	if (hex[0] == '#')
		hex++;
	else if (!IS_HEX_CHAR(hex[0]))
		return 0;

	size_t len = strlen(hex);

	if (len == 3)
	{
		// Shorthand like #09C
		for (unsigned i = 0; i < 3; i++)
		{
			if (!IS_HEX_CHAR(hex[i]))
				return 0;

			UINT32 hx = hex2int(hex[i]);
			*rgba++ = (hx * 16) + hx;
		}

		return 3;
	}
	else if (len == 6 || len == 8)
	{
		if (numc != 4)
			len = 6;

		// A triplet like #0099CC
		for (unsigned i = 0; i < len; i += 2)
		{
			if (!ARE_HEX_CHARS(hex, i))
				return false;

			*rgba++ = (hex2int(hex[i]) * 16) + hex2int(hex[i + 1]);
		}

		return len;
	}

	return 0;
}

/////////////////////////
// extracolormap userdata
/////////////////////////

enum extracolormap_e {
	extracolormap_r = 0,
	extracolormap_g,
	extracolormap_b,
	extracolormap_a,
	extracolormap_rgba,
	extracolormap_fade_r,
	extracolormap_fade_g,
	extracolormap_fade_b,
	extracolormap_fade_a,
	extracolormap_fade_rgba,
	extracolormap_fade_start,
	extracolormap_fade_end,
	extracolormap_colormap
};

static const char *const extracolormap_opt[] = {
	"r",
	"g",
	"b",
	"a",
	"rgba",
	"fade_r",
	"fade_g",
	"fade_b",
	"fade_a",
	"fade_rgba",
	"fade_start",
	"fade_end",
	"colormap",
	NULL};

static int extracolormap_get(lua_State *L)
{
	extracolormap_t *exc = *((extracolormap_t **)luaL_checkudata(L, 1, META_EXTRACOLORMAP));
	enum extracolormap_e field = luaL_checkoption(L, 2, NULL, extracolormap_opt);

	switch (field)
	{
	case extracolormap_r:
		lua_pushinteger(L, R_GetRgbaR(exc->rgba));
		break;
	case extracolormap_g:
		lua_pushinteger(L, R_GetRgbaG(exc->rgba));
		break;
	case extracolormap_b:
		lua_pushinteger(L, R_GetRgbaB(exc->rgba));
		break;
	case extracolormap_a:
		lua_pushinteger(L, R_GetRgbaA(exc->rgba));
		break;
	case extracolormap_rgba:
		lua_pushinteger(L, R_GetRgbaR(exc->rgba));
		lua_pushinteger(L, R_GetRgbaG(exc->rgba));
		lua_pushinteger(L, R_GetRgbaB(exc->rgba));
		lua_pushinteger(L, R_GetRgbaA(exc->rgba));
		return 4;
	case extracolormap_fade_r:
		lua_pushinteger(L, R_GetRgbaR(exc->fadergba));
		break;
	case extracolormap_fade_g:
		lua_pushinteger(L, R_GetRgbaG(exc->fadergba));
		break;
	case extracolormap_fade_b:
		lua_pushinteger(L, R_GetRgbaB(exc->fadergba));
		break;
	case extracolormap_fade_a:
		lua_pushinteger(L, R_GetRgbaA(exc->fadergba));
		break;
	case extracolormap_fade_rgba:
		lua_pushinteger(L, R_GetRgbaR(exc->fadergba));
		lua_pushinteger(L, R_GetRgbaG(exc->fadergba));
		lua_pushinteger(L, R_GetRgbaB(exc->fadergba));
		lua_pushinteger(L, R_GetRgbaA(exc->fadergba));
		return 4;
	case extracolormap_fade_start:
		lua_pushinteger(L, exc->fadestart);
		break;
	case extracolormap_fade_end:
		lua_pushinteger(L, exc->fadeend);
		break;
	case extracolormap_colormap:
		LUA_PushUserdata(L, exc->colormap, META_LIGHTTABLE);
		break;
	}
	return 1;
}

static void GetExtraColormapRGBA(lua_State *L, UINT8 *rgba)
{
	if (lua_type(L, 3) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, 3);
		UINT8 parsed = ParseHTMLColor(str, rgba, 4);
		if (!parsed)
			luaL_error(L, "Malformed HTML color '%s'", str);
	}
	else
		GetRGBAColorsFromTable(L, 3, rgba, true);
}

static int extracolormap_set(lua_State *L)
{
	extracolormap_t *exc = *((extracolormap_t **)luaL_checkudata(L, 1, META_EXTRACOLORMAP));
	enum extracolormap_e field = luaL_checkoption(L, 2, NULL, extracolormap_opt);

	UINT8 r = R_GetRgbaR(exc->rgba);
	UINT8 g = R_GetRgbaG(exc->rgba);
	UINT8 b = R_GetRgbaB(exc->rgba);
	UINT8 a = R_GetRgbaA(exc->rgba);

	UINT8 fr = R_GetRgbaR(exc->fadergba);
	UINT8 fg = R_GetRgbaG(exc->fadergba);
	UINT8 fb = R_GetRgbaB(exc->fadergba);
	UINT8 fa = R_GetRgbaA(exc->fadergba);

	UINT8 rgba[4];

	INT32 old_rgba = exc->rgba, old_fade_rgba = exc->fadergba; // It's not unsigned?
	UINT8 old_fade_start = exc->fadestart, old_fade_end = exc->fadeend;

#define val luaL_checkinteger(L, 3)

	switch(field)
	{
	case extracolormap_r:
		exc->rgba = R_PutRgbaRGBA(val, g, b, a);
		break;
	case extracolormap_g:
		exc->rgba = R_PutRgbaRGBA(r, val, b, a);
		break;
	case extracolormap_b:
		exc->rgba = R_PutRgbaRGBA(r, g, val, a);
		break;
	case extracolormap_a:
		exc->rgba = R_PutRgbaRGBA(r, g, b, val);
		break;
	case extracolormap_rgba:
		rgba[0] = r;
		rgba[1] = g;
		rgba[2] = b;
		rgba[3] = a;
		GetExtraColormapRGBA(L, rgba);
		exc->rgba = R_PutRgbaRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
		break;
	case extracolormap_fade_r:
		exc->fadergba = R_PutRgbaRGBA(val, fg, fb, fa);
		break;
	case extracolormap_fade_g:
		exc->fadergba = R_PutRgbaRGBA(fr, val, fb, fa);
		break;
	case extracolormap_fade_b:
		exc->fadergba = R_PutRgbaRGBA(fr, fg, val, fa);
		break;
	case extracolormap_fade_a:
		exc->fadergba = R_PutRgbaRGBA(fr, fg, fb, val);
		break;
	case extracolormap_fade_rgba:
		rgba[0] = fr;
		rgba[1] = fg;
		rgba[2] = fb;
		rgba[3] = fa;
		GetExtraColormapRGBA(L, rgba);
		exc->fadergba = R_PutRgbaRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
		break;
	case extracolormap_fade_start:
		if (val > 31)
			return luaL_error(L, "fade start %d out of range (0 - 31)", val);
		exc->fadestart = val;
		break;
	case extracolormap_fade_end:
		if (val > 31)
			return luaL_error(L, "fade end %d out of range (0 - 31)", val);
		exc->fadeend = val;
		break;
	case extracolormap_colormap:
		return luaL_error(L, LUA_QL("extracolormap_t") " field " LUA_QS " should not be set directly.", extracolormap_opt[field]);
	}

#undef val

	if (exc->rgba != old_rgba
		|| exc->fadergba != old_fade_rgba
		|| exc->fadestart != old_fade_start
		|| exc->fadeend != old_fade_end)
	R_GenerateLightTable(exc, true);

	return 0;
}

static int lighttable_get(lua_State *L)
{
	void **userdata;

	lighttable_t *table = *((lighttable_t **)luaL_checkudata(L, 1, META_LIGHTTABLE));
	UINT32 row = luaL_checkinteger(L, 2);
	if (row < 1 || row > 34)
		return luaL_error(L, "lighttable row %d out of range (1 - %d)", row, 34);

	userdata = lua_newuserdata(L, sizeof(void *));
	*userdata = &table[256 * (row - 1)];
	luaL_getmetatable(L, META_COLORMAP);
	lua_setmetatable(L, -2);

	return 1;
}

static int lighttable_len(lua_State *L)
{
	lua_pushinteger(L, NUM_PALETTE_ENTRIES);
	return 1;
}

int LUA_ColorLib(lua_State *L)
{
	luaL_newmetatable(L, META_EXTRACOLORMAP);
		lua_pushcfunction(L, extracolormap_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, extracolormap_set);
		lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);

	luaL_newmetatable(L, META_LIGHTTABLE);
		lua_pushcfunction(L, lighttable_get);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, lighttable_len);
		lua_setfield(L, -2, "__len");
	lua_pop(L, 1);

	return 0;
}
