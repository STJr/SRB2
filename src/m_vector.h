// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2004 Stephen McGranahan
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Vectors
//      SoM created 05/18/09
//
//-----------------------------------------------------------------------------

#ifndef M_VECTOR_H__
#define M_VECTOR_H__

#ifdef ESLOPE

#include "m_fixed.h"
#include "tables.h"

#define TWOPI	    M_PI*2.0
#define HALFPI 	    M_PI*0.5
#define QUARTERPI   M_PI*0.25
#define EPSILON     0.000001f
#define OMEGA       10000000.0f

typedef struct
{
   fixed_t x, y, z;
} v3fixed_t;

typedef struct
{
   fixed_t x, y;
} v2fixed_t;

typedef struct
{
   float x, y, z;
} v3float_t;

typedef struct
{
	float yaw, pitch, roll;
} angles3d_t;

typedef struct
{
	double x, y, z;
} v3double_t;

typedef struct
{
   float x, y;
} v2float_t;


v3fixed_t *M_MakeVec3(const v3fixed_t *point1, const v3fixed_t *point2, v3fixed_t *a_o);
v3float_t *M_MakeVec3f(const v3float_t *point1, const v3float_t *point2, v3float_t *a_o);
void M_TranslateVec3(v3fixed_t *vec);
void M_TranslateVec3f(v3float_t *vec);
void M_AddVec3(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2);
void M_AddVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2);
void M_SubVec3(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2);
void M_SubVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2);
fixed_t M_DotVec3(const v3fixed_t *v1, const v3fixed_t *v2);
float M_DotVec3f(const v3float_t *v1, const v3float_t *v2);

#ifdef SESLOPE
v3double_t *M_MakeVec3d(const v3double_t *point1, const v3double_t *point2, v3double_t *a_o);
double M_DotVec3d(const v3double_t *v1, const v3double_t *v2);
void M_TranslateVec3d(v3double_t *vec);
#endif

void M_CrossProduct3(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2);
void M_CrossProduct3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2);
fixed_t FV_Magnitude(const v3fixed_t *a_normal);
float FV_Magnitudef(const v3float_t *a_normal);
fixed_t FV_NormalizeO(const v3fixed_t *a_normal, v3fixed_t *a_o);
float FV_NormalizeOf(const v3float_t *a_normal, v3float_t *a_o);
fixed_t FV_Normalize(v3fixed_t *a_normal);
fixed_t FV_Normalizef(v3float_t *a_normal);
void FV_Normal(const v3fixed_t *a_triangle, v3fixed_t *a_normal);
void FV_Normalf(const v3float_t *a_triangle, v3float_t *a_normal);
v3fixed_t *M_LoadVec(v3fixed_t *vec, fixed_t x, fixed_t y, fixed_t z);
v3fixed_t *M_CopyVec(v3fixed_t *a_o, const v3fixed_t *a_i);
v3float_t *M_LoadVecf(v3float_t *vec, float x, float y, float z);
v3float_t *M_CopyVecf(v3float_t *a_o, const v3float_t *a_i);
v3fixed_t *FV_Midpoint(const v3fixed_t *a_1, const v3fixed_t *a_2, v3fixed_t *a_o);
fixed_t FV_Distance(const v3fixed_t *p1, const v3fixed_t *p2);
v3float_t *FV_Midpointf(const v3float_t *a_1, const v3float_t *a_2, v3float_t *a_o);
angle_t FV_AngleBetweenVectors(const v3fixed_t *Vector1, const v3fixed_t *Vector2);
float FV_AngleBetweenVectorsf(const v3float_t *Vector1, const v3float_t *Vector2);
float FV_Distancef(const v3float_t *p1, const v3float_t *p2);


// Kalaron: something crazy, vector physics
float M_VectorYaw(v3float_t v);
float M_VectorPitch(v3float_t v);
angles3d_t *M_VectorAlignTo(float Pitch, float Yaw, float Roll, v3float_t v, byte AngleAxis, float Rate);



#endif

// EOF
#endif // #ifdef ESLOPE

