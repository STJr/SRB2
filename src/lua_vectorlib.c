// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_vectorlib.c
/// \brief vector library for Lua scripting

#include "vector3d.h"
#include "lua_script.h"
#include "lua_libs.h"

// shared by both Vector2D and Vector3D
enum vectorfield_e {
	vectorfield_clone = 0,
	vectorfield_opposite,
	vectorfield_x,
	vectorfield_y,
	vectorfield_z,
	vectorfield_length,
	vectorfield_normalized,
};

static const char *const vectorfield_opt[] = {
	"vector_clone",
	"vector_opposite",
	"x",
	"y",
	"z",
	"vector_length",
	"vector_normalized",
	NULL};

////////////////////////////
// VECTOR2 STATIC MEMBERS //
////////////////////////////

/////////////////////
// VECTOR2 MEMBERS //
/////////////////////

static int vector2d_get(lua_State *L)
{
	vector2_t *vec = *((vector2_t **)luaL_checkudata(L, 1, META_VECTOR2));
	enum vectorfield_e field = luaL_checkoption(L, 2, vectorfield_opt[0], vectorfield_opt);

	switch(field)
	{
		case vectorfield_x: lua_pushfixed(L, vec->x); return 1;
		case vectorfield_y: lua_pushfixed(L, vec->y); return 1;
		default: break;
	}

	return 0;
}

/////////////
// VECTOR3 //
/////////////

static vector3_t *NewVector3(lua_State *L)
{
	vector3_t *vec = lua_newuserdata(L, sizeof(*vec));
	luaL_getmetatable(L, META_VECTOR3);
	lua_setmetatable(L, -2);
	return vec;
}

////////////////////////////
// VECTOR3 STATIC MEMBERS //
////////////////////////////

static int vector3d_new(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_optfixed(L, 3, 0);

	Vector3D_Set(NewVector3(L), x, y, z);
	return 1;
}

static luaL_Reg vector3d[] = {
	{"new", vector3d_new},
	{NULL, NULL}
};

/////////////////////
// VECTOR3 MEMBERS //
/////////////////////

static int vector3d_clone(lua_State *L)
{
	vector3_t *vec = luaL_checkudata(L, 1, META_VECTOR3);
	Vector3D_Copy(NewVector3(L), vec);
	return 1;
}

static int vector3d_opposite(lua_State *L)
{
	vector3_t *vec = luaL_checkudata(L, 1, META_VECTOR3);
	Vector3D_Opposite(NewVector3(L), vec);
	return 1;
}

static int vector3d_get(lua_State *L)
{
	vector3_t *vec = luaL_checkudata(L, 1, META_VECTOR3);
	enum vectorfield_e field = luaL_checkoption(L, 2, vectorfield_opt[0], vectorfield_opt);

	if (!vec)
		return luaL_error(L, "accessed vector3_t doesn't exist anymore.");

	switch(field)
	{
		case vectorfield_clone: lua_pushcfunction(L, vector3d_clone); return 1;
		case vectorfield_opposite: lua_pushcfunction(L, vector3d_opposite); return 1;

		case vectorfield_x: lua_pushfixed(L, vec->x); return 1;
		case vectorfield_y: lua_pushfixed(L, vec->y); return 1;
		case vectorfield_z: lua_pushfixed(L, vec->z); return 1;
		case vectorfield_length: lua_pushfixed(L, Vector3D_Length(vec)); return 1;
		case vectorfield_normalized: Vector3D_Normalize(NewVector3(L), vec); return 1;

		default: break;
	}

	return 0;
}

///////////////////////
// VECTOR3 OPERATORS //
///////////////////////

static int vector3d_eq(lua_State *L)
{
	vector3_t *vec1 = luaL_checkudata(L, 1, META_VECTOR3);
	vector3_t *vec2 = luaL_checkudata(L, 2, META_VECTOR3);
	lua_pushboolean(L, Vector3D_Equal(vec1, vec2));
	return 1;
}

static int vector3d_op(
	lua_State *L,
	vector3_t *(*opvector)(vector3_t*, vector3_t*, vector3_t*),
	vector3_t *(*opfixed)(vector3_t*, vector3_t*, fixed_t)
)
{
	if (lua_isnumber(L, 1) && (opfixed == Vector3D_AddFixed || opfixed == Vector3D_MulFixed))
	{
		fixed_t n1 = lua_tofixed(L, 1);
		vector3_t *vec2 = luaL_checkudata(L, 2, META_VECTOR3);
		opfixed(NewVector3(L), vec2, n1);
	}
	else if (lua_isnumber(L, 2))
	{
		vector3_t *vec1 = luaL_checkudata(L, 1, META_VECTOR3);
		fixed_t n2 = lua_tofixed(L, 2);
		opfixed(NewVector3(L), vec1, n2);
	}
	else
	{
		vector3_t *vec1 = luaL_checkudata(L, 1, META_VECTOR3);
		vector3_t *vec2 = luaL_checkudata(L, 2, META_VECTOR3);
		opvector(NewVector3(L), vec1, vec2);
	}

	return 1;
}

static int vector3d_add(lua_State *L)
{
	return vector3d_op(L, Vector3D_Add, Vector3D_AddFixed);
}

static int vector3d_sub(lua_State *L)
{
	return vector3d_op(L, Vector3D_Sub, Vector3D_SubFixed);
}

static int vector3d_mul(lua_State *L)
{
	return vector3d_op(L, Vector3D_Mul, Vector3D_MulFixed);
}

static int vector3d_div(lua_State *L)
{
	return vector3d_op(L, Vector3D_Div, Vector3D_DivFixed);
}

static int vector3d_unm(lua_State *L)
{
	vector3_t *vec = luaL_checkudata(L, 1, META_VECTOR3);
	Vector3D_Opposite(NewVector3(L), vec);
	return 1;
}

int LUA_VectorLib(lua_State *L)
{
	LUA_RegisterUserdataMetatable(L, META_VECTOR2, vector2d_get, NULL, NULL);

	luaL_newmetatable(L, META_VECTOR3);
	LUA_SetCFunctionField(L, "__index", vector3d_get);
	LUA_SetCFunctionField(L, "__eq", vector3d_eq);
	LUA_SetCFunctionField(L, "__add", vector3d_add);
	LUA_SetCFunctionField(L, "__sub", vector3d_sub);
	LUA_SetCFunctionField(L, "__mul", vector3d_mul);
	LUA_SetCFunctionField(L, "__div", vector3d_div);
	LUA_SetCFunctionField(L, "__unm", vector3d_unm);
	lua_pop(L, 1);

	luaL_register(L, "Vector3D", vector3d);
	return 0;
}
