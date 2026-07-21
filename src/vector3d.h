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

#ifndef __VECTOR3D__
#define __VECTOR3D__

#include "m_fixed.h"

vector3_t *Vector3D_Set(vector3_t *vec, fixed_t x, fixed_t y, fixed_t z);
vector3_t *Vector3D_Copy(vector3_t *out, vector3_t *in);
fixed_t Vector3D_Length(vector3_t *vec);
vector3_t *Vector3D_Opposite(vector3_t *out, vector3_t *in);
vector3_t *Vector3D_Normalize(vector3_t *out, vector3_t *in);
boolean Vector3D_Equal(vector3_t *a, vector3_t *b);

vector3_t *Vector3D_Add(vector3_t *out, vector3_t *a, vector3_t *b);
vector3_t *Vector3D_Sub(vector3_t *out, vector3_t *a, vector3_t *b);
vector3_t *Vector3D_Mul(vector3_t *out, vector3_t *a, vector3_t *b);
vector3_t *Vector3D_Div(vector3_t *out, vector3_t *a, vector3_t *b);

vector3_t *Vector3D_AddFixed(vector3_t *out, vector3_t *a, fixed_t b);
vector3_t *Vector3D_SubFixed(vector3_t *out, vector3_t *a, fixed_t b);
vector3_t *Vector3D_MulFixed(vector3_t *out, vector3_t *a, fixed_t b);
vector3_t *Vector3D_DivFixed(vector3_t *out, vector3_t *a, fixed_t b);

#endif
