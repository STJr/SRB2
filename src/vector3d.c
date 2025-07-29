// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  vector3d.c
/// \brief Fixed-point 3D vector

#include "vector3d.h"
#include "r_main.h" // R_PointToDist2

vector3_t *Vector3D_Set(vector3_t *vec, fixed_t x, fixed_t y, fixed_t z)
{
	vec->x = x;
	vec->y = y;
	vec->z = z;

	return vec;
}

vector3_t *Vector3D_Copy(vector3_t *out, vector3_t *in)
{
	return memcpy(out, in, sizeof(*out));
}

fixed_t Vector3D_Length(vector3_t *vec)
{
	return R_PointToDist2(0, 0, R_PointToDist2(0, 0, vec->x, vec->y), vec->z);
}

vector3_t *Vector3D_Opposite(vector3_t *out, vector3_t *in)
{
	return Vector3D_Set(out, -in->x, -in->y, -in->z);
}

vector3_t *Vector3D_Normalize(vector3_t *out, vector3_t *in)
{
	fixed_t len = Vector3D_Length(in);

	if (len == 0)
		return Vector3D_Set(out, in->x, in->y, in->z);
	else
		return Vector3D_Set(out, FixedDiv(in->x, len), FixedDiv(in->y, len), FixedDiv(in->z, len));
}

boolean Vector3D_Equal(vector3_t *a, vector3_t *b)
{
	return (a->x == b->x && a->y == b->y && a->z == b->z);
}

vector3_t *Vector3D_Add(vector3_t *out, vector3_t *a, vector3_t *b)
{
	out->x = a->x + b->x;
	out->y = a->y + b->y;
	out->z = a->z + b->z;

	return out;
}

vector3_t *Vector3D_Sub(vector3_t *out, vector3_t *a, vector3_t *b)
{
	out->x = a->x - b->x;
	out->y = a->y - b->y;
	out->z = a->z - b->z;

	return out;
}

vector3_t *Vector3D_Mul(vector3_t *out, vector3_t *a, vector3_t *b)
{
	out->x = FixedMul(a->x, b->x);
	out->y = FixedMul(a->y, b->y);
	out->z = FixedMul(a->z, b->z);

	return out;
}

vector3_t *Vector3D_Div(vector3_t *out, vector3_t *a, vector3_t *b)
{
	out->x = FixedDiv(a->x, b->x);
	out->y = FixedDiv(a->y, b->y);
	out->z = FixedDiv(a->z, b->z);

	return out;
}

vector3_t *Vector3D_AddFixed(vector3_t *out, vector3_t *a, fixed_t b)
{
	out->x = a->x + b;
	out->y = a->y + b;
	out->z = a->z + b;

	return out;
}

vector3_t *Vector3D_SubFixed(vector3_t *out, vector3_t *a, fixed_t b)
{
	out->x = a->x - b;
	out->y = a->y - b;
	out->z = a->z - b;

	return out;
}

vector3_t *Vector3D_MulFixed(vector3_t *out, vector3_t *a, fixed_t b)
{
	out->x = FixedMul(a->x, b);
	out->y = FixedMul(a->y, b);
	out->z = FixedMul(a->z, b);

	return out;
}

vector3_t *Vector3D_DivFixed(vector3_t *out, vector3_t *a, fixed_t b)
{
	out->x = FixedDiv(a->x, b);
	out->y = FixedDiv(a->y, b);
	out->z = FixedDiv(a->z, b);

	return out;
}
