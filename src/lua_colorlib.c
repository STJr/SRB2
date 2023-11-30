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
	extracolormap_red = 0,
	extracolormap_green,
	extracolormap_blue,
	extracolormap_alpha,
	extracolormap_color,
	extracolormap_fade_red,
	extracolormap_fade_green,
	extracolormap_fade_blue,
	extracolormap_fade_alpha,
	extracolormap_fade_color,
	extracolormap_fade_start,
	extracolormap_fade_end,
	extracolormap_colormap
};

static const char *const extracolormap_opt[] = {
	"red",
	"green",
	"blue",
	"alpha",
	"color",
	"fade_red",
	"fade_green",
	"fade_blue",
	"fade_alpha",
	"fade_color",
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
	case extracolormap_red:
		lua_pushinteger(L, R_GetRgbaR(exc->rgba));
		break;
	case extracolormap_green:
		lua_pushinteger(L, R_GetRgbaG(exc->rgba));
		break;
	case extracolormap_blue:
		lua_pushinteger(L, R_GetRgbaB(exc->rgba));
		break;
	case extracolormap_alpha:
		lua_pushinteger(L, R_GetRgbaA(exc->rgba));
		break;
	case extracolormap_color:
		lua_pushinteger(L, R_GetRgbaR(exc->rgba));
		lua_pushinteger(L, R_GetRgbaG(exc->rgba));
		lua_pushinteger(L, R_GetRgbaB(exc->rgba));
		lua_pushinteger(L, R_GetRgbaA(exc->rgba));
		return 4;
	case extracolormap_fade_red:
		lua_pushinteger(L, R_GetRgbaR(exc->fadergba));
		break;
	case extracolormap_fade_green:
		lua_pushinteger(L, R_GetRgbaG(exc->fadergba));
		break;
	case extracolormap_fade_blue:
		lua_pushinteger(L, R_GetRgbaB(exc->fadergba));
		break;
	case extracolormap_fade_alpha:
		lua_pushinteger(L, R_GetRgbaA(exc->fadergba));
		break;
	case extracolormap_fade_color:
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

static void GetExtraColormapRGBA(lua_State *L, UINT8 *rgba, int arg)
{
	if (lua_type(L, arg) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, arg);
		UINT8 parsed = ParseHTMLColor(str, rgba, 4);
		if (!parsed)
			luaL_error(L, "Malformed HTML color '%s'", str);
	}
	else
	{
		UINT32 colors = lua_tointeger(L, arg);
		if (colors > 0xFFFFFF)
		{
			rgba[0] = (colors >> 24) & 0xFF;
			rgba[1] = (colors >> 16) & 0xFF;
			rgba[2] = (colors >> 8) & 0xFF;
			rgba[3] = colors & 0xFF;
		}
		else
		{
			rgba[0] = (colors >> 16) & 0xFF;
			rgba[1] = (colors >> 8) & 0xFF;
			rgba[2] = colors & 0xFF;
			rgba[3] = 0xFF;
		}
	}
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
	case extracolormap_red:
		exc->rgba = R_PutRgbaRGBA(val, g, b, a);
		break;
	case extracolormap_green:
		exc->rgba = R_PutRgbaRGBA(r, val, b, a);
		break;
	case extracolormap_blue:
		exc->rgba = R_PutRgbaRGBA(r, g, val, a);
		break;
	case extracolormap_alpha:
		exc->rgba = R_PutRgbaRGBA(r, g, b, val);
		break;
	case extracolormap_color:
		rgba[0] = r;
		rgba[1] = g;
		rgba[2] = b;
		rgba[3] = a;
		GetExtraColormapRGBA(L, rgba, 3);
		exc->rgba = R_PutRgbaRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
		break;
	case extracolormap_fade_red:
		exc->fadergba = R_PutRgbaRGBA(val, fg, fb, fa);
		break;
	case extracolormap_fade_green:
		exc->fadergba = R_PutRgbaRGBA(fr, val, fb, fa);
		break;
	case extracolormap_fade_blue:
		exc->fadergba = R_PutRgbaRGBA(fr, fg, val, fa);
		break;
	case extracolormap_fade_alpha:
		exc->fadergba = R_PutRgbaRGBA(fr, fg, fb, val);
		break;
	case extracolormap_fade_color:
		rgba[0] = fr;
		rgba[1] = fg;
		rgba[2] = fb;
		rgba[3] = fa;
		GetExtraColormapRGBA(L, rgba, 3);
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
