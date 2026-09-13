#include "m_random.h" // m_random
#include "lua_script.h"
#include "lua_libs.h"
#include "lua_hud.h"
#include "lua_hook.h"

#define NOHUD if (hud_running)\
return luaL_error(L, "HUD rendering code should not call this function!");\
else if (hook_cmd_running)\
return luaL_error(L, "CMD building code should not call this function!");

// M_RANDOM
//////////////

static int lib_randomFixed(lua_State *L)
{
	NOHUD
	lua_pushfixed(L, P_RandomFixed());
	return 1;
}

static int lib_randomByte(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_RandomByte());
	return 1;
}

static int lib_randomKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);

	NOHUD
	if (a > 65536)
		LUA_UsageWarning(L, "random.key: range > 65536 is undefined behavior");
	lua_pushinteger(L, P_RandomKey(a));
	return 1;
}

static int lib_randomRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);

	NOHUD
	if (b < a) {
		INT32 c = a;
		a = b;
		b = c;
	}

	if ((b - a + 1) > 65536)
		LUA_UsageWarning(L, "random.range: range > 65536 is undefined behavior");
	lua_pushinteger(L, P_RandomRange(a, b));
	return 1;
}

// Macros.
static int lib_randomSigned(lua_State *L)
{
	NOHUD
	lua_pushinteger(L, P_SignedRandom());
	return 1;
}

static int lib_randomChance(lua_State *L)
{
	fixed_t p = luaL_checkfixed(L, 1);
	NOHUD
	lua_pushboolean(L, P_RandomChance(p));
	return 1;
}

static int lib_randomLocalFixed(lua_State *L)
{
	lua_pushfixed(L, M_RandomFixed());
	return 1;
}

static int lib_randomLocalByte(lua_State *L)
{
	lua_pushinteger(L, M_RandomByte());
	return 1;
}

static int lib_randomLocalKey(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);

	lua_pushinteger(L, M_RandomKey(a));
	return 1;
}

static int lib_randomLocalRange(lua_State *L)
{
	INT32 a = (INT32)luaL_checkinteger(L, 1);
	INT32 b = (INT32)luaL_checkinteger(L, 2);

	lua_pushinteger(L, M_RandomRange(a, b));
	return 1;
}

// Macros.
static int lib_randomLocalSigned(lua_State *L)
{
	lua_pushinteger(L, M_SignedRandom());
	return 1;
}

static int lib_randomLocalChance(lua_State *L)
{
	fixed_t p = luaL_checkfixed(L, 1);
	lua_pushboolean(L, M_RandomChance(p));
	return 1;
}

// globalized client_side random functions.
static luaL_Reg lib_randomclient[] = {
	{"fixed",lib_randomFixed},
	{"byte",lib_randomByte},
	{"key",lib_randomKey},
	{"range",lib_randomRange},
	{"signed",lib_randomSigned}, // MACRO
	{"chance",lib_randomChance}, // MACRO
	// local variants
	{"localFixed",lib_randomLocalFixed},
	{"localByte",lib_randomLocalByte},
	{"localKey",lib_randomLocalKey},
	{"localRange",lib_randomLocalRange},
	{"localSigned",lib_randomLocalSigned}, // MACRO
	{"localChance",lib_randomLocalChance}, // MACRO
	{NULL, NULL}
};

int LUA_RandomLib(lua_State* L)
{
	luaL_register(L, "random", lib_randomclient);
	return 0;
}