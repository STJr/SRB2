// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  quaternion.c
/// \brief Fixed-point 3D vector

#include "quaternion.h"
#include "vector3d.h"
#include "matrix.h"
#include "r_main.h" // R_PointToDist2

quaternion_t *Quaternion_Set(quaternion_t *quat, fixed_t x, fixed_t y, fixed_t z, fixed_t w)
{
	quat->x = x;
	quat->y = y;
	quat->z = z;
	quat->w = w;

	return quat;
}

quaternion_t *Quaternion_SetIdentity(quaternion_t *quat)
{
	return Quaternion_Set(quat, 0, 0, 0, FRACUNIT);
}

quaternion_t *Quaternion_SetAxisRotation(quaternion_t *quat, vector3_t *axis, fixed_t angle)
{
	fixed_t cosangle = FINECOSINE(((angle / 2) >> ANGLETOFINESHIFT) & FINEMASK);
	fixed_t sinangle = FINESINE(((angle / 2) >> ANGLETOFINESHIFT) & FINEMASK);
	vector3_t normaxis;

	Vector3D_Normalize(&normaxis, axis);

	return Quaternion_Set(quat,
		FixedMul(normaxis.x, sinangle),
		FixedMul(normaxis.y, sinangle),
		FixedMul(normaxis.z, sinangle),
		cosangle
	);
}

quaternion_t *Quaternion_Copy(quaternion_t *out, quaternion_t *in)
{
	return memcpy(out, in, sizeof(*out));
}

matrix_t *Quaternion_ToMatrix(matrix_t *mat, const quaternion_t *quat)
{
	fixed_t x = quat->x, y = quat->y, z = quat->z, w = quat->w;

	fixed_t xx2 = 2 * FixedMul(x, x);
	fixed_t xy2 = 2 * FixedMul(x, y);
	fixed_t xz2 = 2 * FixedMul(x, z);
	fixed_t xw2 = 2 * FixedMul(x, w);
	fixed_t yy2 = 2 * FixedMul(y, y);
	fixed_t yz2 = 2 * FixedMul(y, z);
	fixed_t yw2 = 2 * FixedMul(y, w);
	fixed_t zz2 = 2 * FixedMul(z, z);
	fixed_t zw2 = 2 * FixedMul(z, w);

	Matrix_SetIdentity(mat);

	mat->matrix[0][0] = FRACUNIT - yy2 - zz2;
	mat->matrix[0][1] = xy2 - zw2;
	mat->matrix[0][2] = xz2 + yw2;

	mat->matrix[1][0] = xy2 + zw2;
	mat->matrix[1][1] = FRACUNIT - xx2 - zz2;
	mat->matrix[1][2] = yz2 - xw2;

	mat->matrix[2][0] = xz2 - yw2;
	mat->matrix[2][1] = yz2 + xw2;
	mat->matrix[2][2] = FRACUNIT - xx2 - yy2;

	mat->matrix[3][3] = FRACUNIT;

	return mat;
}

quaternion_t *Quaternion_Normalize(quaternion_t *out, quaternion_t *in)
{
	fixed_t sqlen =
		FixedMul(in->x, in->x) +
		FixedMul(in->y, in->y) +
		FixedMul(in->z, in->z) +
		FixedMul(in->w, in->w);

	if (sqlen < FRACUNIT / 1024)
		return Quaternion_Set(out, in->x, in->y, in->z, in->w);

	fixed_t len = R_PointToDist2(0, 0, R_PointToDist2(0, 0, R_PointToDist2(0, 0, in->x, in->y), in->z), in->w);

	return Quaternion_Set(out,
		FixedDiv(in->x, len),
		FixedDiv(in->y, len),
		FixedDiv(in->z, len),
		FixedDiv(in->w, len)
	);
}

quaternion_t *Quaternion_Mul(quaternion_t *out, quaternion_t *a, quaternion_t *b)
{
	fixed_t ax = a->x, ay = a->y, az = a->z, aw = a->w;
	fixed_t bx = b->x, by = b->y, bz = b->z, bw = b->w;

	return Quaternion_Normalize(out, Quaternion_Set(out,
		FixedMul(aw, bx) + FixedMul(ax, bw) + FixedMul(ay, bz) - FixedMul(az, by),
		FixedMul(aw, by) - FixedMul(ax, bz) + FixedMul(ay, bw) + FixedMul(az, bx),
		FixedMul(aw, bz) + FixedMul(ax, by) - FixedMul(ay, bx) + FixedMul(az, bw),
		FixedMul(aw, bw) - FixedMul(ax, bx) - FixedMul(ay, by) - FixedMul(az, bz)
	));
}
