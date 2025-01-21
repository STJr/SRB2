// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_mathlib.c
/// \brief basic math library for Lua scripting

#include "doomdef.h"
//#include "fastcmp.h"
#include "tables.h"
#include "p_local.h"
#include "doomstat.h" // for ALL7EMERALDS
#include "r_main.h" // for R_PointToDist2
#include "m_easing.h"

#include "lua_script.h"
#include "lua_libs.h"

// General math
//////////////////

static int lib_abs(lua_State *L)
{
	int a = (int)luaL_checkinteger(L, 1);
	lua_pushinteger(L, abs(a));
	return 1;
}

static int lib_min(lua_State *L)
{
	int a = luaL_checkinteger(L, 1);
	int b = luaL_checkinteger(L, 2);
	lua_pushinteger(L, min(a,b));
	return 1;
}

static int lib_max(lua_State *L)
{
	int a = luaL_checkinteger(L, 1);
	int b = luaL_checkinteger(L, 2);
	lua_pushinteger(L, max(a,b));
	return 1;
}

// Angle math
////////////////

static int lib_fixedangle(lua_State *L)
{
	lua_pushangle(L, FixedAngle(luaL_checkfixed(L, 1)));
	return 1;
}

static int lib_anglefixed(lua_State *L)
{
	lua_pushfixed(L, AngleFixed(luaL_checkangle(L, 1)));
	return 1;
}

static int lib_invangle(lua_State *L)
{
	lua_pushangle(L, InvAngle(luaL_checkangle(L, 1)));
	return 1;
}

static int lib_finesine(lua_State *L)
{
	lua_pushfixed(L, FINESINE((luaL_checkangle(L, 1)>>ANGLETOFINESHIFT) & FINEMASK));
	return 1;
}

static int lib_finecosine(lua_State *L)
{
	lua_pushfixed(L, FINECOSINE((luaL_checkangle(L, 1)>>ANGLETOFINESHIFT) & FINEMASK));
	return 1;
}

static int lib_finetangent(lua_State *L)
{
	// HACK: add ANGLE_90 to make tan() in Lua start at 0 like it should
	// use & 4095 instead of & FINEMASK (8191), so it doesn't go out of the array's bounds
	lua_pushfixed(L, FINETANGENT(((luaL_checkangle(L, 1)+ANGLE_90)>>ANGLETOFINESHIFT) & 4095));
	return 1;
}

static int lib_fixedasin(lua_State *L)
{
	lua_pushangle(L, -FixedAcos(luaL_checkfixed(L, 1)) + ANGLE_90);
	return 1;
}

static int lib_fixedacos(lua_State *L)
{
	lua_pushangle(L, FixedAcos(luaL_checkfixed(L, 1)));
	return 1;
}

// Fixed math
////////////////

static int lib_fixedmul(lua_State *L)
{
	lua_pushfixed(L, FixedMul(luaL_checkfixed(L, 1), luaL_checkfixed(L, 2)));
	return 1;
}

static int lib_fixedint(lua_State *L)
{
	lua_pushinteger(L, FixedInt(luaL_checkfixed(L, 1)));
	return 1;
}

static int lib_fixeddiv(lua_State *L)
{
	fixed_t i = luaL_checkfixed(L, 1);
	fixed_t j = luaL_checkfixed(L, 2);
	if (j == 0)
		return luaL_error(L, "divide by zero");
	lua_pushfixed(L, FixedDiv(i, j));
	return 1;
}

// TODO: 2.3: Delete
static int lib_fixedrem(lua_State *L)
{
	LUA_Deprecated(L, "FixedRem(a, b)", "a % b");
	lua_pushfixed(L, luaL_checkfixed(L, 1) % luaL_checkfixed(L, 2));
	return 1;
}

static int lib_fixedsqrt(lua_State *L)
{
	fixed_t i = luaL_checkfixed(L, 1);
	if (i < 0)
		return luaL_error(L, "square root domain error");
	lua_pushfixed(L, FixedSqrt(i));
	return 1;
}

static int lib_fixedhypot(lua_State *L)
{
	lua_pushfixed(L, R_PointToDist2(0, 0, luaL_checkfixed(L, 1), luaL_checkfixed(L, 2)));
	return 1;
}

static int lib_fixedfloor(lua_State *L)
{
	lua_pushfixed(L, FixedFloor(luaL_checkfixed(L, 1)));
	return 1;
}

static int lib_fixedtrunc(lua_State *L)
{
	lua_pushfixed(L, FixedTrunc(luaL_checkfixed(L, 1)));
	return 1;
}

static int lib_fixedceil(lua_State *L)
{
	lua_pushfixed(L, FixedCeil(luaL_checkfixed(L, 1)));
	return 1;
}

static int lib_fixedround(lua_State *L)
{
	lua_pushfixed(L, FixedRound(luaL_checkfixed(L, 1)));
	return 1;
}

// Misc. math
// (aka extra little funcs that don't quite fit in baselib)
//////////////////////////////////////////////////////////////////

static int lib_getsecspecial(lua_State *L)
{
	lua_pushinteger(L, GETSECSPECIAL(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2)));
	return 1;
}

static int lib_all7emeralds(lua_State *L)
{
	lua_pushboolean(L, ALL7EMERALDS(luaL_checkinteger(L, 1)));
	return 1;
}

// Returns both color and signpost shade numbers!
static int lib_coloropposite(lua_State *L)
{
	UINT16 colornum = (UINT16)luaL_checkinteger(L, 1);
	if (!colornum || colornum >= numskincolors)
		return luaL_error(L, "skincolor %d out of range (1 - %d).", colornum, numskincolors-1);
	lua_pushinteger(L, skincolors[colornum].invcolor); // push color
	lua_pushinteger(L, skincolors[colornum].invshade); // push sign shade index, 0-15
	return 2;
}

static luaL_Reg lib_math[] = {
	{"abs", lib_abs},
	{"min", lib_min},
	{"max", lib_max},
	{"sin", lib_finesine},
	{"cos", lib_finecosine},
	{"tan", lib_finetangent},
	{"asin", lib_fixedasin},
	{"acos", lib_fixedacos},
	{"FixedAngle", lib_fixedangle},
	{"fixangle"  , lib_fixedangle},
	{"AngleFixed", lib_anglefixed},
	{"anglefix"  , lib_anglefixed},
	{"InvAngle", lib_invangle},
	{"FixedMul", lib_fixedmul},
	{"fixmul"  , lib_fixedmul},
	{"FixedInt", lib_fixedint},
	{"fixint"  , lib_fixedint},
	{"FixedDiv", lib_fixeddiv},
	{"fixdiv"  , lib_fixeddiv},
	{"FixedRem", lib_fixedrem},
	{"fixrem"  , lib_fixedrem},
	{"FixedSqrt", lib_fixedsqrt},
	{"fixsqrt"  , lib_fixedsqrt},
	{"FixedHypot", lib_fixedhypot},
	{"fixhypot"  , lib_fixedhypot},
	{"FixedFloor", lib_fixedfloor},
	{"fixfloor"  , lib_fixedfloor},
	{"FixedTrunc", lib_fixedtrunc},
	{"fixtrunc"  , lib_fixedtrunc},
	{"FixedCeil", lib_fixedceil},
	{"fixceil"  , lib_fixedceil},
	{"FixedRound", lib_fixedround},
	{"fixround"  , lib_fixedround},
	{"GetSecSpecial", lib_getsecspecial},
	{"All7Emeralds", lib_all7emeralds},
	{"ColorOpposite", lib_coloropposite},
	{NULL, NULL}
};

//
// Easing functions
//

#define EASINGFUNC(easetype) \
{ \
	fixed_t start = 0; \
	fixed_t end = FRACUNIT; \
	fixed_t t = luaL_checkfixed(L, 1); \
	int n = lua_gettop(L); \
	if (n == 2) \
		end = luaL_checkfixed(L, 2); \
	else if (n >= 3) \
	{ \
		start = luaL_checkfixed(L, 2); \
		end = luaL_checkfixed(L, 3); \
	} \
	lua_pushfixed(L, (Easing_ ## easetype)(t, start, end)); \
	return 1; \
} \

static int lib_easelinear(lua_State *L) { EASINGFUNC(Linear) }

static int lib_easeinsine(lua_State *L) { EASINGFUNC(InSine) }
static int lib_easeoutsine(lua_State *L) { EASINGFUNC(OutSine) }
static int lib_easeinoutsine(lua_State *L) { EASINGFUNC(InOutSine) }

static int lib_easeinquad(lua_State *L) { EASINGFUNC(InQuad) }
static int lib_easeoutquad(lua_State *L) { EASINGFUNC(OutQuad) }
static int lib_easeinoutquad(lua_State *L) { EASINGFUNC(InOutQuad) }

static int lib_easeincubic(lua_State *L) { EASINGFUNC(InCubic) }
static int lib_easeoutcubic(lua_State *L) { EASINGFUNC(OutCubic) }
static int lib_easeinoutcubic(lua_State *L) { EASINGFUNC(InOutCubic) }

static int lib_easeinquart(lua_State *L) { EASINGFUNC(InQuart) }
static int lib_easeoutquart(lua_State *L) { EASINGFUNC(OutQuart) }
static int lib_easeinoutquart(lua_State *L) { EASINGFUNC(InOutQuart) }

static int lib_easeinquint(lua_State *L) { EASINGFUNC(InQuint) }
static int lib_easeoutquint(lua_State *L) { EASINGFUNC(OutQuint) }
static int lib_easeinoutquint(lua_State *L) { EASINGFUNC(InOutQuint) }

static int lib_easeinexpo(lua_State *L) { EASINGFUNC(InExpo) }
static int lib_easeoutexpo(lua_State *L) { EASINGFUNC(OutExpo) }
static int lib_easeinoutexpo(lua_State *L) { EASINGFUNC(InOutExpo) }

#undef EASINGFUNC

#define EASINGFUNC(easetype) \
{ \
	boolean useparam = false; \
	fixed_t param = 0; \
	fixed_t start = 0; \
	fixed_t end = FRACUNIT; \
	fixed_t t = luaL_checkfixed(L, 1); \
	int n = lua_gettop(L); \
	if (n == 2) \
		end = luaL_checkfixed(L, 2); \
	else if (n >= 3) \
	{ \
		start = (fixed_t)luaL_optinteger(L, 2, start); \
		end = (fixed_t)luaL_optinteger(L, 3, end); \
		if ((n >= 4) && (useparam = (!lua_isnil(L, 4)))) \
			param = luaL_checkfixed(L, 4); \
	} \
	if (useparam) \
		lua_pushfixed(L, (Easing_ ## easetype ## Parameterized)(t, start, end, param)); \
	else \
		lua_pushfixed(L, (Easing_ ## easetype)(t, start, end)); \
	return 1; \
} \

static int lib_easeinback(lua_State *L) { EASINGFUNC(InBack) }
static int lib_easeoutback(lua_State *L) { EASINGFUNC(OutBack) }
static int lib_easeinoutback(lua_State *L) { EASINGFUNC(InOutBack) }

#undef EASINGFUNC

static luaL_Reg lib_ease[] = {
	{"linear", lib_easelinear},

	{"insine", lib_easeinsine},
	{"outsine", lib_easeoutsine},
	{"inoutsine", lib_easeinoutsine},

	{"inquad", lib_easeinquad},
	{"outquad", lib_easeoutquad},
	{"inoutquad", lib_easeinoutquad},

	{"incubic", lib_easeincubic},
	{"outcubic", lib_easeoutcubic},
	{"inoutcubic", lib_easeinoutcubic},

	{"inquart", lib_easeinquart},
	{"outquart", lib_easeoutquart},
	{"inoutquart", lib_easeinoutquart},

	{"inquint", lib_easeinquint},
	{"outquint", lib_easeoutquint},
	{"inoutquint", lib_easeinoutquint},

	{"inexpo", lib_easeinexpo},
	{"outexpo", lib_easeoutexpo},
	{"inoutexpo", lib_easeinoutexpo},

	{"inback", lib_easeinback},
	{"outback", lib_easeoutback},
	{"inoutback", lib_easeinoutback},

	{NULL, NULL}
};

int LUA_MathLib(lua_State *L)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib_math);
	luaL_register(L, "ease", lib_ease);
	return 0;
}
