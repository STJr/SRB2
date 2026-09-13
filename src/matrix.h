// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  matrix.c
/// \brief Fixed-point 4x4 matrix

#ifndef __MATRIX__
#define __MATRIX__

#include "m_fixed.h"
#include "vector3d.h"

typedef struct
{
    fixed_t matrix[4][4];
} matrix_t;

matrix_t *Matrix_SetIdentity(matrix_t *mat);
matrix_t *Matrix_SetTranslation(matrix_t *mat, fixed_t x, fixed_t y, fixed_t z);
matrix_t *Matrix_SetScaling(matrix_t *mat, fixed_t x, fixed_t y, fixed_t z);
matrix_t *Matrix_Copy(matrix_t *out, matrix_t *in);
matrix_t *Matrix_Mul(matrix_t *out, matrix_t *a, matrix_t *b);
vector3_t *Matrix_MulVector(vector3_t *out, matrix_t *a, vector3_t *b);

#endif
