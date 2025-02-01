// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021-2024 by Lactozilla.
// Copyright (C) 2014-2024 by Sonic Team Junior.
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
#include "v_video.h"

#include "lua_script.h"
#include "lua_libs.h"

#define COLORLIB_USE_LOOKUP

#ifdef COLORLIB_USE_LOOKUP
	static colorlookup_t colormix_lut;
	#define GetNearestColor(r, g, b) GetColorLUT(&colormix_lut, r, g, b)
#else
	#define GetNearestColor(r, g, b) NearestPaletteColor(r, g, b, pMasterPalette)
#endif

////////////////
// Color library
////////////////

static int lib_colorPaletteToRgb(lua_State *L)
{
	RGBA_t color = V_GetMasterColor((UINT8)luaL_checkinteger(L, 1));
	lua_pushinteger(L, color.s.red);
	lua_pushinteger(L, color.s.green);
	lua_pushinteger(L, color.s.blue);
	return 3;
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

static UINT8 GetHTMLColorLength(const char *str)
{
	if (str[0] == '#')
		str++;
	return strlen(str) >= 8 ? 4 : 3;
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

static UINT8 GetPackedRGBA(UINT32 colors, UINT8 *rgba)
{
	if (colors > 0xFFFFFF)
	{
		rgba[0] = (colors >> 24) & 0xFF;
		rgba[1] = (colors >> 16) & 0xFF;
		rgba[2] = (colors >> 8) & 0xFF;
		rgba[3] = colors & 0xFF;
		return 4;
	}
	else
	{
		rgba[0] = (colors >> 16) & 0xFF;
		rgba[1] = (colors >> 8) & 0xFF;
		rgba[2] = colors & 0xFF;
		rgba[3] = 0xFF;
		return 3;
	}
}

static UINT8 GetArgsRGBA(lua_State *L, UINT8 index, INT32 *r, INT32 *g, INT32 *b, INT32 *a)
{
	UINT8 rgba[4] = { 0, 0, 0, 255 };
	UINT8 num = 0;

	if (lua_gettop(L) == 1 && lua_type(L, index) == LUA_TNUMBER)
	{
		num = GetPackedRGBA(luaL_checkinteger(L, 1), rgba);

		*r = rgba[0];
		*g = rgba[1];
		*b = rgba[2];
		if (a)
			*a = rgba[3];
	}
	else if (lua_type(L, index) == LUA_TSTRING)
	{
		const char *str = lua_tostring(L, index);
		UINT8 parsed = ParseHTMLColor(str, rgba, GetHTMLColorLength(str));
		if (!parsed)
			luaL_error(L, "Malformed HTML color '%s'", str);

		num = parsed == 8 ? 4 : 3;

		*r = rgba[0];
		*g = rgba[1];
		*b = rgba[2];
		if (a)
			*a = rgba[3];
	}
	else
	{
		INT32 temp;

#define CHECKINT(i) luaL_checkinteger(L, i)
#define GETCOLOR(c, i, desc) { \
			temp = CHECKINT(i); \
			if (temp < 0 || temp > 255) \
				luaL_error(L, desc " channel %d out of range (0 - 255)", temp); \
			c = temp; \
			num++; \
		}

		GETCOLOR(*r, index + 0, "red color");
		GETCOLOR(*g, index + 1, "green color");
		GETCOLOR(*b, index + 2, "blue color");
#undef CHECKINT
#define CHECKINT(i) luaL_optinteger(L, i, 255)
		if (a)
			GETCOLOR(*a, index + 3, "alpha");
#undef CHECKINT
#undef GETCOLOR

		num = 3 + (lua_type(L, index + 3) == LUA_TNUMBER);
	}

	return num;
}

static int lib_colorRgbToPalette(lua_State *L)
{
	INT32 r, g, b;
	GetArgsRGBA(L, 1, &r, &g, &b, NULL);

#ifdef COLORLIB_USE_LOOKUP
	InitColorLUT(&colormix_lut, pMasterPalette, false);
#endif

	lua_pushinteger(L, GetNearestColor(r, g, b));
	return 1;
}

#define SCALE_UINT8_TO_FIXED(val) FixedDiv(val * FRACUNIT, 255 * FRACUNIT)
#define SCALE_FIXED_TO_UINT8(val) FixedRound(FixedMul(val, 255 * FRACUNIT)) / FRACUNIT

static fixed_t hue2rgb(fixed_t p, fixed_t q, fixed_t t)
{
	if (t < 0)
		t += FRACUNIT;
	if (t > FRACUNIT)
		t -= FRACUNIT;

	fixed_t out;

	if (t < FRACUNIT / 6)
		out = p + FixedMul(FixedMul(q - p, 6 * FRACUNIT), t);
	else if (t < FRACUNIT / 2)
		out = q;
	else if (t < 2 * FRACUNIT / 3)
		out = p + FixedMul(FixedMul(q - p, 2 * FRACUNIT / 3 - t), 6 * FRACUNIT);
	else
		out = p;

	return out;
}

static int lib_colorHslToRgb(lua_State *L)
{
	fixed_t h, s, l;

#define GETHSL(c, i, desc) \
	c = luaL_checkinteger(L, i); \
	if (c < 0 || c > 255) \
		luaL_error(L, desc " %d out of range (0 - 255)", c)

	GETHSL(h, 1, "hue");
	GETHSL(s, 2, "saturation");
	GETHSL(l, 3, "value");
#undef GETHSL

	if (!s)
	{
		lua_pushinteger(L, l);
		lua_pushinteger(L, l);
		lua_pushinteger(L, l);
	}
	else
	{
		h = SCALE_UINT8_TO_FIXED(h);
		s = SCALE_UINT8_TO_FIXED(s);
		l = SCALE_UINT8_TO_FIXED(l);

		fixed_t q, p;

		if (l < FRACUNIT/2)
			q = FixedMul(l, FRACUNIT + s);
		else
			q = l + s - FixedMul(l, s);

		p = l * 2 - q;

		lua_pushinteger(L, SCALE_FIXED_TO_UINT8(hue2rgb(p, q, h + FRACUNIT/3)));
		lua_pushinteger(L, SCALE_FIXED_TO_UINT8(hue2rgb(p, q, h)));
		lua_pushinteger(L, SCALE_FIXED_TO_UINT8(hue2rgb(p, q, h - FRACUNIT/3)));
	}

	return 3;
}

static int lib_colorRgbToHsl(lua_State *L)
{
	INT32 ir, ig, ib;
	GetArgsRGBA(L, 1, &ir, &ig, &ib, NULL);

	fixed_t r = SCALE_UINT8_TO_FIXED(ir);
	fixed_t g = SCALE_UINT8_TO_FIXED(ig);
	fixed_t b = SCALE_UINT8_TO_FIXED(ib);

	fixed_t cmin = min(min(r, g), b);
	fixed_t cmax = max(max(r, g), b);

	fixed_t h, s, l = (cmax + cmin) / 2;
	fixed_t delta = cmax - cmin;

	if (!delta)
		h = s = 0;
	else
	{
		if (l > FRACUNIT / 2)
			s = FixedDiv(delta, (FRACUNIT * 2) - cmax - cmin);
		else
			s = FixedDiv(delta, cmax + cmin);

		if (r > g && r > b)
		{
			h = FixedDiv(g - b, delta);

			if (g < b)
				h += FRACUNIT * 6;
		}
		else
		{
			h = FixedDiv(r - g, delta);

			if (g > b)
				h += FRACUNIT * 2;
			else
				h += FRACUNIT * 4;
		}

		h = FixedDiv(h, FRACUNIT * 6);
	}

	lua_pushinteger(L, SCALE_FIXED_TO_UINT8(h));
	lua_pushinteger(L, SCALE_FIXED_TO_UINT8(s));
	lua_pushinteger(L, SCALE_FIXED_TO_UINT8(l));

	return 3;
}

static int lib_colorHexToRgb(lua_State *L)
{
	UINT8 rgba[4] = { 0, 0, 0, 255 };

	const char *str = luaL_checkstring(L, 1);
	UINT8 parsed = ParseHTMLColor(str, rgba, 4), num = 3;
	if (!parsed)
		luaL_error(L, "Malformed HTML color '%s'", str);
	else if (parsed == 8)
		num++;

	lua_pushinteger(L, rgba[0]);
	lua_pushinteger(L, rgba[1]);
	lua_pushinteger(L, rgba[2]);
	if (num == 4)
		lua_pushinteger(L, rgba[3]);

	return num;
}

static int lib_colorRgbToHex(lua_State *L)
{
	INT32 r, g, b, a;
	UINT8 num = GetArgsRGBA(L, 1, &r, &g, &b, &a);

	char buffer[10];
	if (num >= 4)
		snprintf(buffer, sizeof buffer, "#%02X%02X%02X%02X", r, g, b, a);
	else
		snprintf(buffer, sizeof buffer, "#%02X%02X%02X", r, g, b);

	lua_pushstring(L, buffer);
	return 1;
}

static int lib_colorPackRgb(lua_State *L)
{
	INT32 r, g, b;

	GetArgsRGBA(L, 1, &r, &g, &b, NULL);

	UINT32 packed = ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);

	lua_pushinteger(L, packed);
	return 1;
}

static int lib_colorPackRgba(lua_State *L)
{
	INT32 r, g, b, a;

	GetArgsRGBA(L, 1, &r, &g, &b, &a);

	UINT32 packed = ((r & 0xFF) << 24) | ((g & 0xFF) << 16) | ((b & 0xFF) << 8) | (a & 0xFF);

	lua_pushinteger(L, packed);
	return 1;
}

static int lib_colorUnpackRgb(lua_State *L)
{
	UINT8 rgba[4];

	UINT8 num = GetPackedRGBA(lua_tointeger(L, 1), rgba);

	for (UINT8 i = 0; i < num; i++)
	{
		lua_pushinteger(L, rgba[i]);
	}

	return num;
}

static luaL_Reg color_lib[] = {
	{"paletteToRgb", lib_colorPaletteToRgb},
	{"rgbToPalette", lib_colorRgbToPalette},
	{"hslToRgb", lib_colorHslToRgb},
	{"rgbToHsl", lib_colorRgbToHsl},
	{"hexToRgb", lib_colorHexToRgb},
	{"rgbToHex", lib_colorRgbToHex},
	{"packRgb", lib_colorPackRgb},
	{"packRgba", lib_colorPackRgba},
	{"unpackRgb", lib_colorUnpackRgb},
	{NULL, NULL}
};

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
	extracolormap_fade_end
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
	NULL};

static int extracolormap_get(lua_State *L)
{
	extracolormap_t *exc = *((extracolormap_t **)luaL_checkudata(L, 1, META_EXTRACOLORMAP));
	enum extracolormap_e field = luaL_checkoption(L, 2, NULL, extracolormap_opt);

	if (!exc)
		return LUA_ErrInvalid(L, "extracolormap_t");

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
		GetPackedRGBA(lua_tointeger(L, arg), rgba);
	}
}

static int extracolormap_set(lua_State *L)
{
	extracolormap_t *exc = *((extracolormap_t **)luaL_checkudata(L, 1, META_EXTRACOLORMAP));
	enum extracolormap_e field = luaL_checkoption(L, 2, NULL, extracolormap_opt);

	if (!exc)
		return LUA_ErrInvalid(L, "extracolormap_t");

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
	}

#undef val

	if (exc->rgba != old_rgba
		|| exc->fadergba != old_fade_rgba
		|| exc->fadestart != old_fade_start
		|| exc->fadeend != old_fade_end)
	R_GenerateLightTable(exc, true);

	return 0;
}

int LUA_ColorLib(lua_State *L)
{
	LUA_RegisterUserdataMetatable(L, META_EXTRACOLORMAP, extracolormap_get, extracolormap_set, NULL);

	luaL_register(L, "color", color_lib);

	return 0;
}
