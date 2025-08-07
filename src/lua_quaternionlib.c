// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_quaternionlib.c
/// \brief quaternion library for Lua scripting

#include "quaternion.h"
#include "lua_script.h"
#include "lua_libs.h"

static quaternion_t *NewQuaternion(lua_State *L)
{
	quaternion_t *quat = lua_newuserdata(L, sizeof(*quat));
	luaL_getmetatable(L, META_QUATERNION);
	lua_setmetatable(L, -2);
	return quat;
}

////////////////////
// STATIC MEMBERS //
////////////////////

static int quaternion_new(lua_State *L)
{
	Quaternion_SetIdentity(NewQuaternion(L));
	return 1;
}

static int quaternion_fromAxisRotation(lua_State *L)
{
	vector3_t *axis = luaL_checkudata(L, 1, META_VECTOR3);
	fixed_t angle = luaL_checkfixed(L, 2);
	Quaternion_SetAxisRotation(NewQuaternion(L), axis, angle);
	return 1;
}

static luaL_Reg quaternion[] = {
	{"new", quaternion_new},
	{"fromAxisRotation", quaternion_fromAxisRotation},
	{NULL, NULL}
};

/////////////
// MEMBERS //
/////////////

static int quaternion_clone(lua_State *L)
{
	quaternion_t *quat = luaL_checkudata(L, 1, META_QUATERNION);
	Quaternion_Copy(NewQuaternion(L), quat);
	return 1;
}

static int quaternion_toMatrix(lua_State *L)
{
	quaternion_t *quat = luaL_checkudata(L, 1, META_QUATERNION);

	matrix_t *mat = lua_newuserdata(L, sizeof(*mat));
	luaL_getmetatable(L, META_MATRIX);
	lua_setmetatable(L, -2);

	Quaternion_ToMatrix(mat, quat);
	return 1;
}

enum quaternionfield_e {
	quaternionfield_clone = 0,
	quaternionfield_toMatrix,
	quaternionfield_x,
	quaternionfield_y,
	quaternionfield_z,
	quaternionfield_w,
};

static const char *const quaternionfield_opt[] = {
	"clone",
	"toMatrix",
	"x",
	"y",
	"z",
	"w",
	NULL};

static int quaternion_get(lua_State *L)
{
	quaternion_t *quat = luaL_checkudata(L, 1, META_QUATERNION);
	enum quaternionfield_e field = luaL_checkoption(L, 2, quaternionfield_opt[0], quaternionfield_opt);

	switch(field)
	{
		case quaternionfield_clone: lua_pushcfunction(L, quaternion_clone); return 1;
		case quaternionfield_toMatrix: lua_pushcfunction(L, quaternion_toMatrix); return 1;

		case quaternionfield_x: lua_pushfixed(L, quat->x); return 1;
		case quaternionfield_y: lua_pushfixed(L, quat->y); return 1;
		case quaternionfield_z: lua_pushfixed(L, quat->z); return 1;
		case quaternionfield_w: lua_pushfixed(L, quat->w); return 1;

		default: break;
	}

	return 0;
}

///////////////
// OPERATORS //
///////////////

static int quaternion_mul(lua_State *L)
{
	quaternion_t *quat1 = luaL_checkudata(L, 1, META_QUATERNION);
	quaternion_t *quat2 = luaL_checkudata(L, 2, META_QUATERNION);
	Quaternion_Mul(NewQuaternion(L), quat1, quat2);
	return 1;
}

int LUA_QuaternionLib(lua_State *L)
{
	luaL_newmetatable(L, META_QUATERNION);
	LUA_SetCFunctionField(L, "__index", quaternion_get);
	LUA_SetCFunctionField(L, "__mul", quaternion_mul);
	lua_pop(L, 1);

	luaL_register(L, "Quaternion", quaternion);
	return 0;
}
