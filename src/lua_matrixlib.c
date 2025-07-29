// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_matrixlib.c
/// \brief matrix library for Lua scripting

#include "matrix.h"
#include "vector3d.h"
#include "lua_script.h"
#include "lua_libs.h"

static vector3_t *GetVector(lua_State *L, int index)
{
	vector3_t *vec = lua_touserdata(L, index);

	if (!vec)
		return NULL;

	if (!lua_getmetatable(L, index))
		return NULL;

	lua_getfield(L, LUA_REGISTRYINDEX, META_VECTOR3);
	if (!lua_rawequal(L, -1, -2))
		return NULL;

	lua_pop(L, 2);

	return vec;
}

static matrix_t *NewMatrix(lua_State *L)
{
	matrix_t *mat = lua_newuserdata(L, sizeof(*mat));
	luaL_getmetatable(L, META_MATRIX);
	lua_setmetatable(L, -2);
	return mat;
}

////////////////////
// STATIC MEMBERS //
////////////////////

static int matrix_new(lua_State *L)
{
	Matrix_SetIdentity(NewMatrix(L));
	return 1;
}

static int matrix_fromTranslation(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);

	Matrix_SetTranslation(NewMatrix(L), x, y, z);
	return 1;
}

static int matrix_fromScaling(lua_State *L)
{
	fixed_t x = luaL_checkfixed(L, 1);
	fixed_t y = luaL_checkfixed(L, 2);
	fixed_t z = luaL_checkfixed(L, 3);

	Matrix_SetScaling(NewMatrix(L), x, y, z);
	return 1;
}

static luaL_Reg matrix[] = {
	{"new", matrix_new},
	{"fromTranslation", matrix_fromTranslation},
	{"fromScaling", matrix_fromScaling},
	{NULL, NULL}
};

/////////////
// MEMBERS //
/////////////

enum matrixfield_e {
	matrixfield_clone = 0,
	matrixfield_getvalue,
	matrixfield_setvalue,
	matrixfield_mulXYZ,
};

static const char *const matrixfield_opt[] = {
	"clone",
	"getvalue",
	"setvalue",
	"mulXYZ",
	NULL};

static int matrix_clone(lua_State *L)
{
	matrix_t *mat = luaL_checkudata(L, 1, META_MATRIX);
	Matrix_Copy(NewMatrix(L), mat);
	return 1;
}

static int matrix_getvalue(lua_State *L)
{
	matrix_t *mat = luaL_checkudata(L, 1, META_MATRIX);
	INT32 row = luaL_checkinteger(L, 2);
	INT32 col = luaL_checkinteger(L, 3);

	if (row < 1 || row > 4)
		return luaL_error(L, "matrix row %d out of range (1 - 4)", row);
	if (col < 1 || col > 4)
		return luaL_error(L, "matrix column %d out of range (1 - 4)", col);

	lua_pushfixed(L, mat->matrix[row - 1][col - 1]);
	return 1;
}

static int matrix_setvalue(lua_State *L)
{
	matrix_t *mat = luaL_checkudata(L, 1, META_MATRIX);
	INT32 row = luaL_checkinteger(L, 2);
	INT32 col = luaL_checkinteger(L, 3);
	fixed_t value = luaL_checkfixed(L, 4);

	if (row < 1 || row > 4)
		return luaL_error(L, "matrix row %d out of range (1 - 4)", row);
	if (col < 1 || col > 4)
		return luaL_error(L, "matrix column %d out of range (1 - 4)", col);

	mat->matrix[row - 1][col - 1] = value;
	return 0;
}

static int matrix_mulXYZ(lua_State *L)
{
	matrix_t *mat = luaL_checkudata(L, 1, META_MATRIX);
	fixed_t x = luaL_checkfixed(L, 2);
	fixed_t y = luaL_checkfixed(L, 3);
	fixed_t z = luaL_checkfixed(L, 4);

	vector3_t vec;
	vector3_t result;
	Vector3D_Set(&vec, x, y, z);
	Matrix_MulVector(&result, mat, &vec);

	lua_pushfixed(L, result.x);
	lua_pushfixed(L, result.y);
	lua_pushfixed(L, result.z);
	return 3;
}

static int matrix_get(lua_State *L)
{
	enum matrixfield_e field = luaL_checkoption(L, 2, matrixfield_opt[0], matrixfield_opt);

	switch(field)
	{
		case matrixfield_clone: lua_pushcfunction(L, matrix_clone); return 1;
		case matrixfield_getvalue: lua_pushcfunction(L, matrix_getvalue); return 1;
		case matrixfield_setvalue: lua_pushcfunction(L, matrix_setvalue); return 1;
		case matrixfield_mulXYZ: lua_pushcfunction(L, matrix_mulXYZ); return 1;

		default: break;
	}

	return 0;
}

///////////////
// OPERATORS //
///////////////

static int matrix_mul(lua_State *L)
{
	matrix_t *mat1 = luaL_checkudata(L, 1, META_MATRIX);

	vector3_t *vec2 = GetVector(L, 2);
	if (vec2)
	{
		vector3_t *result = lua_newuserdata(L, sizeof(*result));
		luaL_getmetatable(L, META_VECTOR3);
		lua_setmetatable(L, -2);

		Matrix_MulVector(result, mat1, vec2);
	}
	else
	{
		matrix_t *mat2 = luaL_checkudata(L, 2, META_MATRIX);
		Matrix_Mul(NewMatrix(L), mat1, mat2);
	}

	return 1;
}

int LUA_MatrixLib(lua_State *L)
{
	luaL_newmetatable(L, META_MATRIX);
	LUA_SetCFunctionField(L, "__index", matrix_get);
	LUA_SetCFunctionField(L, "__mul", matrix_mul);
	lua_pop(L, 1);

	luaL_register(L, "Matrix", matrix);
	return 0;
}
