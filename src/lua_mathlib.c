// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by John "JTE" Muniz.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_mathlib.c
/// \brief basic math library for Lua scripting

#include "doomdef.h"
#ifdef HAVE_BLUA
//#include "fastcmp.h"
#include "tables.h"
#include "p_local.h"
#include "doomstat.h" // for ALL7EMERALDS

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
	// 2.1.15 ONLY HACK: optional boolean argument, only add ANGLE_90 if true
	boolean newtan = lua_optboolean(L, 2);

	if (newtan)
	{
		// HACK: add ANGLE_90 to make tan() in Lua start at 0 like it should
		// use & 4095 instead of & FINEMASK (8191), so it doesn't go out of the array's bounds
		lua_pushfixed(L, FINETANGENT(((luaL_checkangle(L, 1)+ANGLE_90)>>ANGLETOFINESHIFT) & 4095));
	}
	else
	{
		LUA_Deprecated(L, "tan(angle)", "tan(angle, true)");
		lua_pushfixed(L, FINETANGENT((luaL_checkangle(L, 1)>>ANGLETOFINESHIFT) & 4095));
	}
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

static int lib_fixedrem(lua_State *L)
{
	lua_pushfixed(L, FixedRem(luaL_checkfixed(L, 1), luaL_checkfixed(L, 2)));
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
	lua_pushfixed(L, FixedHypot(luaL_checkfixed(L, 1), luaL_checkfixed(L, 2)));
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

// Whee, special Lua-exclusive function for making use of Color_Opposite[] without needing *2 or +1
// Returns both color and frame numbers!
static int lib_coloropposite(lua_State *L)
{
	int colornum = ((int)luaL_checkinteger(L, 1)) % MAXSKINCOLORS;
	lua_pushinteger(L, Color_Opposite[colornum*2]); // push color
	lua_pushinteger(L, Color_Opposite[colornum*2+1]); // push frame
	return 2;
}

static luaL_Reg lib[] = {
	{"abs", lib_abs},
	{"min", lib_min},
	{"max", lib_max},
	{"sin", lib_finesine},
	{"cos", lib_finecosine},
	{"tan", lib_finetangent},
	{"FixedAngle", lib_fixedangle},
	{"AngleFixed", lib_anglefixed},
	{"InvAngle", lib_invangle},
	{"FixedMul", lib_fixedmul},
	{"FixedInt", lib_fixedint},
	{"FixedDiv", lib_fixeddiv},
	{"FixedRem", lib_fixedrem},
	{"FixedSqrt", lib_fixedsqrt},
	{"FixedHypot", lib_fixedhypot},
	{"FixedFloor", lib_fixedfloor},
	{"FixedTrunc", lib_fixedtrunc},
	{"FixedCeil", lib_fixedceil},
	{"FixedRound", lib_fixedround},
	{"GetSecSpecial", lib_getsecspecial},
	{"All7Emeralds", lib_all7emeralds},
	{"ColorOpposite", lib_coloropposite},
	{NULL, NULL}
};

int LUA_MathLib(lua_State *L)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, NULL, lib);
	return 0;
}

#endif
