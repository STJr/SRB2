// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
// Copyright (C) 2009 by Stephen McGranahan.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_vector.c
/// \brief Basic vector functions

#include "doomdef.h"

#include "m_vector.h"

void DVector3_Load(dvector3_t *vec, double x, double y, double z)
{
	vec->x = x;
	vec->y = y;
	vec->z = z;
}

void DVector3_Copy(dvector3_t *a_o, const dvector3_t *a_i)
{
	memcpy(a_o, a_i, sizeof(dvector3_t));
}

void DVector3_Add(const dvector3_t *a_i, const dvector3_t *a_c, dvector3_t *a_o)
{
	a_o->x = a_i->x + a_c->x;
	a_o->y = a_i->y + a_c->y;
	a_o->z = a_i->z + a_c->z;
}

void DVector3_Subtract(const dvector3_t *a_i, const dvector3_t *a_c, dvector3_t *a_o)
{
	a_o->x = a_i->x - a_c->x;
	a_o->y = a_i->y - a_c->y;
	a_o->z = a_i->z - a_c->z;
}

void DVector3_Multiply(const dvector3_t *a_i, double a_c, dvector3_t *a_o)
{
	a_o->x = a_i->x * a_c;
	a_o->y = a_i->y * a_c;
	a_o->z = a_i->z * a_c;
}

double DVector3_Magnitude(const dvector3_t *a_normal)
{
	double xs = a_normal->x * a_normal->x;
	double ys = a_normal->y * a_normal->y;
	double zs = a_normal->z * a_normal->z;
	return sqrt(xs + ys + zs);
}

double DVector3_Normalize(dvector3_t *a_normal)
{
	double magnitude = DVector3_Magnitude(a_normal);
	a_normal->x /= magnitude;
	a_normal->y /= magnitude;
	a_normal->z /= magnitude;
	return magnitude;
}

void DVector3_Negate(dvector3_t *a_o)
{
	a_o->x = -a_o->x;
	a_o->y = -a_o->y;
	a_o->z = -a_o->z;
}

void DVector3_Cross(const dvector3_t *a_1, const dvector3_t *a_2, dvector3_t *a_o)
{
	a_o->x = (a_1->y * a_2->z) - (a_1->z * a_2->y);
	a_o->y = (a_1->z * a_2->x) - (a_1->x * a_2->z);
	a_o->z = (a_1->x * a_2->y) - (a_1->y * a_2->x);
}
