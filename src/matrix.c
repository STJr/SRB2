// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  matrix.c
/// \brief Fixed-point 3D vector

#include "matrix.h"

static matrix_t identitymatrix = {{
	{FRACUNIT, 0, 0, 0},
	{0, FRACUNIT, 0, 0},
	{0, 0, FRACUNIT, 0},
	{0, 0, 0, FRACUNIT},
}};

matrix_t *Matrix_SetIdentity(matrix_t *mat)
{
	return memcpy(mat, identitymatrix.matrix, sizeof(*mat));
}

matrix_t *Matrix_SetTranslation(matrix_t *mat, fixed_t x, fixed_t y, fixed_t z)
{
	Matrix_SetIdentity(mat);

	mat->matrix[0][3] = x;
	mat->matrix[1][3] = y;
	mat->matrix[2][3] = z;

	return mat;
}

matrix_t *Matrix_SetScaling(matrix_t *mat, fixed_t x, fixed_t y, fixed_t z)
{
	Matrix_SetIdentity(mat);

	mat->matrix[0][0] = x;
	mat->matrix[1][1] = y;
	mat->matrix[2][2] = z;

	return mat;
}

matrix_t *Matrix_Copy(matrix_t *out, matrix_t *in)
{
	return memcpy(out, in, sizeof(*out));
}

matrix_t *Matrix_Mul(matrix_t *out, matrix_t *a, matrix_t *b)
{
	for (size_t r = 0; r < 4; r++)
		for (size_t c = 0; c < 4; c++)
			for (size_t i = 0; i < 4; i++)
				out->matrix[r][c] += FixedMul(a->matrix[r][i], b->matrix[i][c]);

	return out;
}

vector3_t *Matrix_MulVector(vector3_t *out, matrix_t *a, vector3_t *b)
{
	out->x = FixedMul(a->matrix[0][0], b->x) + FixedMul(a->matrix[0][1], b->y) + FixedMul(a->matrix[0][2], b->z) + a->matrix[0][3];
	out->y = FixedMul(a->matrix[1][0], b->x) + FixedMul(a->matrix[1][1], b->y) + FixedMul(a->matrix[1][2], b->z) + a->matrix[1][3];
	out->z = FixedMul(a->matrix[2][0], b->x) + FixedMul(a->matrix[2][1], b->y) + FixedMul(a->matrix[2][2], b->z) + a->matrix[2][3];

	return out;
}
