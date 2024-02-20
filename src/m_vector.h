// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
// Copyright (C) 2009 by Stephen McGranahan.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_vector.h
/// \brief Basic vector functions

#ifndef __M_VECTOR__
#define __M_VECTOR__

typedef struct
{
	double x, y, z;
} dvector3_t;

void DVector3_Load(dvector3_t *vec, double x, double y, double z);
double DVector3_Magnitude(const dvector3_t *a_normal);
double DVector3_Normalize(dvector3_t *a_normal);
void DVector3_Negate(dvector3_t *a_o);
void DVector3_Cross(const dvector3_t *a_1, const dvector3_t *a_2, dvector3_t *a_o);

#endif
