// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  quaternion.c
/// \brief Fixed-point quaternion

#ifndef __QUATERNION__
#define __QUATERNION__

#include "m_fixed.h"
#include "matrix.h"

typedef struct
{
	fixed_t x, y, z, w;
} quaternion_t;

quaternion_t *Quaternion_Set(quaternion_t *quat, fixed_t x, fixed_t y, fixed_t z, fixed_t w);
quaternion_t *Quaternion_SetIdentity(quaternion_t *quat);
quaternion_t *Quaternion_SetAxisRotation(quaternion_t *quat, vector3_t *axis, fixed_t angle);
quaternion_t *Quaternion_Copy(quaternion_t *out, quaternion_t *in);
matrix_t *Quaternion_ToMatrix(matrix_t *mat, const quaternion_t *quat);
quaternion_t *Quaternion_Normalize(quaternion_t *out, quaternion_t *in);
quaternion_t *Quaternion_Mul(quaternion_t *out, quaternion_t *a, quaternion_t *b);

#endif
