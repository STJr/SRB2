// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_fixed.c
/// \brief Fixed point implementation

#if 0 //#ifndef NO_M
#include <math.h>
#define HAVE_SQRT
 #if 0 //#ifndef _WIN32 // MSVCRT does not have *f() functions
 #define HAVE_SQRTF
 #endif
#endif
#include "doomdef.h"
#include "m_fixed.h"

#ifdef __USE_C_FIXEDMUL__

/**	\brief	The FixedMul function

	\param	a	fixed_t number
	\param	b	fixed_t number

	\return	a*b>>FRACBITS

*/
fixed_t FixedMul(fixed_t a, fixed_t b)
{
	// Need to cast to unsigned before shifting to avoid undefined behaviour
	// for negative integers
	return (fixed_t)(((UINT64)((INT64)a * b)) >> FRACBITS);
}

#endif //__USE_C_FIXEDMUL__

#ifdef __USE_C_FIXEDDIV__
/**	\brief	The FixedDiv2 function

	\param	a	fixed_t number
	\param	b	fixed_t number

	\return	a/b * FRACUNIT

*/
fixed_t FixedDiv2(fixed_t a, fixed_t b)
{
	INT64 ret;

	if (b == 0)
		I_Error("FixedDiv: divide by zero");

	ret = (((INT64)a * FRACUNIT)) / b;

	if ((ret > INT32_MAX) || (ret < INT32_MIN))
		I_Error("FixedDiv: divide by zero");
	return (fixed_t)ret;
}

#endif // __USE_C_FIXEDDIV__

fixed_t FixedSqrt(fixed_t x)
{
#ifdef HAVE_SQRT
	const float fx = FIXED_TO_FLOAT(x);
	float fr;
#ifdef HAVE_SQRTF
	fr = sqrtf(fx);
#else
	fr = (float)sqrt(fx);
#endif
	return FLOAT_TO_FIXED(fr);
#else
	// The neglected art of Fixed Point arithmetic
	// Jetro Lauha
	// Seminar Presentation
	// Assembly 2006, 3rd- 6th August 2006
	// (Revised: September 13, 2006)
	// URL: http://jet.ro/files/The_neglected_art_of_Fixed_Point_arithmetic_20060913.pdf
	register UINT32 root, remHi, remLo, testDiv, count;
	root = 0;         /* Clear root */
	remHi = 0;        /* Clear high part of partial remainder */
	remLo = x;        /* Get argument into low part of partial remainder */
	count = (15 + (FRACBITS >> 1));    /* Load loop counter */
	do
	{
		remHi = (remHi << 2) | (remLo >> 30); remLo <<= 2;  /* get 2 bits of arg */
		root <<= 1;   /* Get ready for the next bit in the root */
		testDiv = (root << 1) + 1;    /* Test radical */
		if (remHi >= testDiv)
		{
			remHi -= testDiv;
			root += 1;
		}
	} while (count-- != 0);
	return root;
#endif
}

fixed_t FixedHypot(fixed_t x, fixed_t y)
{
	fixed_t ax, yx, yx2, yx1;
	if (abs(y) > abs(x)) // |y|>|x|
	{
		ax = abs(y); // |y| => ax
		yx = FixedDiv(x, y); // (x/y)
	}
	else // |x|>|y|
	{
		ax = abs(x); // |x| => ax
		yx = FixedDiv(y, x); // (x/y)
	}
	yx2 = FixedMul(yx, yx); // (x/y)^2
	yx1 = FixedSqrt(1 * FRACUNIT + yx2); // (1 + (x/y)^2)^1/2
	return FixedMul(ax, yx1); // |x|*((1 + (x/y)^2)^1/2)
}

vector2_t *FV2_Load(vector2_t *vec, fixed_t x, fixed_t y)
{
	vec->x = x;
	vec->y = y;
	return vec;
}

vector2_t *FV2_UnLoad(vector2_t *vec, fixed_t *x, fixed_t *y)
{
	*x = vec->x;
	*y = vec->y;
	return vec;
}

vector2_t *FV2_Copy(vector2_t *a_o, const vector2_t *a_i)
{
	return M_Memcpy(a_o, a_i, sizeof(vector2_t));
}

vector2_t *FV2_AddEx(const vector2_t *a_i, const vector2_t *a_c, vector2_t *a_o)
{
	a_o->x = a_i->x + a_c->x;
	a_o->y = a_i->y + a_c->y;
	return a_o;
}

vector2_t *FV2_Add(vector2_t *a_i, const vector2_t *a_c)
{
	return FV2_AddEx(a_i, a_c, a_i);
}

vector2_t *FV2_SubEx(const vector2_t *a_i, const vector2_t *a_c, vector2_t *a_o)
{
	a_o->x = a_i->x - a_c->x;
	a_o->y = a_i->y - a_c->y;
	return a_o;
}

vector2_t *FV2_Sub(vector2_t *a_i, const vector2_t *a_c)
{
	return FV2_SubEx(a_i, a_c, a_i);
}

vector2_t *FV2_MulEx(const vector2_t *a_i, fixed_t a_c, vector2_t *a_o)
{
	a_o->x = FixedMul(a_i->x, a_c);
	a_o->y = FixedMul(a_i->y, a_c);
	return a_o;
}

vector2_t *FV2_Mul(vector2_t *a_i, fixed_t a_c)
{
	return FV2_MulEx(a_i, a_c, a_i);
}

vector2_t *FV2_DivideEx(const vector2_t *a_i, fixed_t a_c, vector2_t *a_o)
{
	a_o->x = FixedDiv(a_i->x, a_c);
	a_o->y = FixedDiv(a_i->y, a_c);
	return a_o;
}

vector2_t *FV2_Divide(vector2_t *a_i, fixed_t a_c)
{
	return FV2_DivideEx(a_i, a_c, a_i);
}

// Vector Complex Math
vector2_t *FV2_Midpoint(const vector2_t *a_1, const vector2_t *a_2, vector2_t *a_o)
{
	a_o->x = FixedDiv(a_2->x - a_1->x, 2 * FRACUNIT);
	a_o->y = FixedDiv(a_2->y - a_1->y, 2 * FRACUNIT);
	a_o->x = a_1->x + a_o->x;
	a_o->y = a_1->y + a_o->y;
	return a_o;
}

fixed_t FV2_Distance(const vector2_t *p1, const vector2_t *p2)
{
	fixed_t xs = FixedMul(p2->x - p1->x, p2->x - p1->x);
	fixed_t ys = FixedMul(p2->y - p1->y, p2->y - p1->y);
	return FixedSqrt(xs + ys);
}

fixed_t FV2_Magnitude(const vector2_t *a_normal)
{
	fixed_t xs = FixedMul(a_normal->x, a_normal->x);
	fixed_t ys = FixedMul(a_normal->y, a_normal->y);
	return FixedSqrt(xs + ys);
}

// Also returns the magnitude
fixed_t FV2_NormalizeEx(const vector2_t *a_normal, vector2_t *a_o)
{
	fixed_t magnitude = FV2_Magnitude(a_normal);
	a_o->x = FixedDiv(a_normal->x, magnitude);
	a_o->y = FixedDiv(a_normal->y, magnitude);
	return magnitude;
}

fixed_t FV2_Normalize(vector2_t *a_normal)
{
	return FV2_NormalizeEx(a_normal, a_normal);
}

vector2_t *FV2_NegateEx(const vector2_t *a_1, vector2_t *a_o)
{
	a_o->x = -a_1->x;
	a_o->y = -a_1->y;
	return a_o;
}

vector2_t *FV2_Negate(vector2_t *a_1)
{
	return FV2_NegateEx(a_1, a_1);
}

boolean FV2_Equal(const vector2_t *a_1, const vector2_t *a_2)
{
	fixed_t Epsilon = FRACUNIT / FRACUNIT;

	if ((abs(a_2->x - a_1->x) > Epsilon) ||
		(abs(a_2->y - a_1->y) > Epsilon))
	{
		return true;
	}

	return false;
}

fixed_t FV2_Dot(const vector2_t *a_1, const vector2_t *a_2)
{
	return (FixedMul(a_1->x, a_2->x) + FixedMul(a_1->y, a_2->y));
}

//
// Point2Vec
//
// Given two points, create a vector between them.
//
vector2_t *FV2_Point2Vec(const vector2_t *point1, const vector2_t *point2, vector2_t *a_o)
{
	a_o->x = point1->x - point2->x;
	a_o->y = point1->y - point2->y;
	return a_o;
}

vector3_t *FV3_Load(vector3_t *vec, fixed_t x, fixed_t y, fixed_t z)
{
	vec->x = x;
	vec->y = y;
	vec->z = z;
	return vec;
}

vector3_t *FV3_UnLoad(vector3_t *vec, fixed_t *x, fixed_t *y, fixed_t *z)
{
	*x = vec->x;
	*y = vec->y;
	*z = vec->z;
	return vec;
}

vector3_t *FV3_Copy(vector3_t *a_o, const vector3_t *a_i)
{
	return M_Memcpy(a_o, a_i, sizeof(vector3_t));
}

vector3_t *FV3_AddEx(const vector3_t *a_i, const vector3_t *a_c, vector3_t *a_o)
{
	a_o->x = a_i->x + a_c->x;
	a_o->y = a_i->y + a_c->y;
	a_o->z = a_i->z + a_c->z;
	return a_o;
}

vector3_t *FV3_Add(vector3_t *a_i, const vector3_t *a_c)
{
	return FV3_AddEx(a_i, a_c, a_i);
}

vector3_t *FV3_SubEx(const vector3_t *a_i, const vector3_t *a_c, vector3_t *a_o)
{
	a_o->x = a_i->x - a_c->x;
	a_o->y = a_i->y - a_c->y;
	a_o->z = a_i->z - a_c->z;
	return a_o;
}

vector3_t *FV3_Sub(vector3_t *a_i, const vector3_t *a_c)
{
	return FV3_SubEx(a_i, a_c, a_i);
}

vector3_t *FV3_MulEx(const vector3_t *a_i, fixed_t a_c, vector3_t *a_o)
{
	a_o->x = FixedMul(a_i->x, a_c);
	a_o->y = FixedMul(a_i->y, a_c);
	a_o->z = FixedMul(a_i->z, a_c);
	return a_o;
}

vector3_t *FV3_Mul(vector3_t *a_i, fixed_t a_c)
{
	return FV3_MulEx(a_i, a_c, a_i);
}

vector3_t *FV3_DivideEx(const vector3_t *a_i, fixed_t a_c, vector3_t *a_o)
{
	a_o->x = FixedDiv(a_i->x, a_c);
	a_o->y = FixedDiv(a_i->y, a_c);
	a_o->z = FixedDiv(a_i->z, a_c);
	return a_o;
}

vector3_t *FV3_Divide(vector3_t *a_i, fixed_t a_c)
{
	return FV3_DivideEx(a_i, a_c, a_i);
}

// Vector Complex Math
vector3_t *FV3_Midpoint(const vector3_t *a_1, const vector3_t *a_2, vector3_t *a_o)
{
	a_o->x = FixedDiv(a_2->x - a_1->x, 2 * FRACUNIT);
	a_o->y = FixedDiv(a_2->y - a_1->y, 2 * FRACUNIT);
	a_o->z = FixedDiv(a_2->z - a_1->z, 2 * FRACUNIT);
	a_o->x = a_1->x + a_o->x;
	a_o->y = a_1->y + a_o->y;
	a_o->z = a_1->z + a_o->z;
	return a_o;
}

fixed_t FV3_Distance(const vector3_t *p1, const vector3_t *p2)
{
	fixed_t xs = FixedMul(p2->x - p1->x, p2->x - p1->x);
	fixed_t ys = FixedMul(p2->y - p1->y, p2->y - p1->y);
	fixed_t zs = FixedMul(p2->z - p1->z, p2->z - p1->z);
	return FixedSqrt(xs + ys + zs);
}

fixed_t FV3_Magnitude(const vector3_t *a_normal)
{
	fixed_t xs = FixedMul(a_normal->x, a_normal->x);
	fixed_t ys = FixedMul(a_normal->y, a_normal->y);
	fixed_t zs = FixedMul(a_normal->z, a_normal->z);
	return FixedSqrt(xs + ys + zs);
}

// Also returns the magnitude
fixed_t FV3_NormalizeEx(const vector3_t *a_normal, vector3_t *a_o)
{
	fixed_t magnitude = FV3_Magnitude(a_normal);
	a_o->x = FixedDiv(a_normal->x, magnitude);
	a_o->y = FixedDiv(a_normal->y, magnitude);
	a_o->z = FixedDiv(a_normal->z, magnitude);
	return magnitude;
}

fixed_t FV3_Normalize(vector3_t *a_normal)
{
	return FV3_NormalizeEx(a_normal, a_normal);
}

vector3_t *FV3_NegateEx(const vector3_t *a_1, vector3_t *a_o)
{
	a_o->x = -a_1->x;
	a_o->y = -a_1->y;
	a_o->z = -a_1->z;
	return a_o;
}

vector3_t *FV3_Negate(vector3_t *a_1)
{
	return FV3_NegateEx(a_1, a_1);
}

boolean FV3_Equal(const vector3_t *a_1, const vector3_t *a_2)
{
	fixed_t Epsilon = FRACUNIT / FRACUNIT;

	if ((abs(a_2->x - a_1->x) > Epsilon) ||
		(abs(a_2->y - a_1->y) > Epsilon) ||
		(abs(a_2->z - a_1->z) > Epsilon))
	{
		return true;
	}

	return false;
}

fixed_t FV3_Dot(const vector3_t *a_1, const vector3_t *a_2)
{
	return (FixedMul(a_1->x, a_2->x) + FixedMul(a_1->y, a_2->y) + FixedMul(a_1->z, a_2->z));
}

vector3_t *FV3_Cross(const vector3_t *a_1, const vector3_t *a_2, vector3_t *a_o)
{
	a_o->x = FixedMul(a_1->y, a_2->z) - FixedMul(a_1->z, a_2->y);
	a_o->y = FixedMul(a_1->z, a_2->x) - FixedMul(a_1->x, a_2->z);
	a_o->z = FixedMul(a_1->x, a_2->y) - FixedMul(a_1->y, a_2->x);
	return a_o;
}

//
// ClosestPointOnLine
//
// Finds the point on a line closest
// to the specified point.
//
vector3_t *FV3_ClosestPointOnLine(const vector3_t *Line, const vector3_t *p, vector3_t *out)
{
	// Determine t (the length of the vector from �Line[0]� to �p�)
	vector3_t c, V;
	fixed_t t, d = 0;
	FV3_SubEx(p, &Line[0], &c);
	FV3_SubEx(&Line[1], &Line[0], &V);
	FV3_NormalizeEx(&V, &V);

	d = FV3_Distance(&Line[0], &Line[1]);
	t = FV3_Dot(&V, &c);

	// Check to see if �t� is beyond the extents of the line segment
	if (t < 0)
	{
		return FV3_Copy(out, &Line[0]);
	}
	if (t > d)
	{
		return FV3_Copy(out, &Line[1]);
	}

	// Return the point between �Line[0]� and �Line[1]�
	FV3_Mul(&V, t);

	return FV3_AddEx(&Line[0], &V, out);
}

//
// ClosestPointOnVector
//
// Similar to ClosestPointOnLine, but uses a vector instead of two points.
//
void FV3_ClosestPointOnVector(const vector3_t *dir, const vector3_t *p, vector3_t *out)
{
	fixed_t t = FV3_Dot(dir, p);

	// Return the point on the line closest
	FV3_MulEx(dir, t, out);
	return;
}

//
// ClosestPointOnTriangle
//
// Given a triangle and a point,
// the closest point on the edge of
// the triangle is returned.
//
void FV3_ClosestPointOnTriangle(const vector3_t *tri, const vector3_t *point, vector3_t *result)
{
	UINT8 i;
	fixed_t dist, closestdist;
	vector3_t EdgePoints[3];
	vector3_t Line[2];

	FV3_Copy(&Line[0], &tri[0]);
	FV3_Copy(&Line[1], &tri[1]);
	FV3_ClosestPointOnLine(Line, point, &EdgePoints[0]);

	FV3_Copy(&Line[0], &tri[1]);
	FV3_Copy(&Line[1], &tri[2]);
	FV3_ClosestPointOnLine(Line, point, &EdgePoints[1]);

	FV3_Copy(&Line[0], &tri[2]);
	FV3_Copy(&Line[1], &tri[0]);
	FV3_ClosestPointOnLine(Line, point, &EdgePoints[2]);

	// Find the closest one of the three
	FV3_Copy(result, &EdgePoints[0]);
	closestdist = FV3_Distance(point, &EdgePoints[0]);
	for (i = 1; i < 3; i++)
	{
		dist = FV3_Distance(point, &EdgePoints[i]);

		if (dist < closestdist)
		{
			closestdist = dist;
			FV3_Copy(result, &EdgePoints[i]);
		}
	}

	// We now have the closest point! Whee!
}

//
// Point2Vec
//
// Given two points, create a vector between them.
//
vector3_t *FV3_Point2Vec(const vector3_t *point1, const vector3_t *point2, vector3_t *a_o)
{
	a_o->x = point1->x - point2->x;
	a_o->y = point1->y - point2->y;
	a_o->z = point1->z - point2->z;
	return a_o;
}

//
// Normal
//
// Calculates the normal of a polygon.
//
fixed_t FV3_Normal(const vector3_t *a_triangle, vector3_t *a_normal)
{
	vector3_t a_1;
	vector3_t a_2;

	FV3_Point2Vec(&a_triangle[2], &a_triangle[0], &a_1);
	FV3_Point2Vec(&a_triangle[1], &a_triangle[0], &a_2);

	FV3_Cross(&a_1, &a_2, a_normal);

	return FV3_NormalizeEx(a_normal, a_normal);
}

//
// Strength
//
// Measures the 'strength' of a vector in a particular direction.
//
fixed_t FV3_Strength(const vector3_t *a_1, const vector3_t *dir)
{
	vector3_t normal;
	fixed_t dist = FV3_NormalizeEx(a_1, &normal);
	fixed_t dot = FV3_Dot(&normal, dir);

	FV3_ClosestPointOnVector(dir, a_1, &normal);

	dist = FV3_Magnitude(&normal);

	if (dot < 0) // Not facing same direction, so negate result.
		dist = -dist;

	return dist;
}

//
// PlaneDistance
//
// Calculates distance between a plane and the origin.
//
fixed_t FV3_PlaneDistance(const vector3_t *a_normal, const vector3_t *a_point)
{
	return -(FixedMul(a_normal->x, a_point->x) + FixedMul(a_normal->y, a_point->y) + FixedMul(a_normal->z, a_point->z));
}

boolean FV3_IntersectedPlane(const vector3_t *a_triangle, const vector3_t *a_line, vector3_t *a_normal, fixed_t *originDistance)
{
	fixed_t distance1 = 0, distance2 = 0;

	FV3_Normal(a_triangle, a_normal);

	*originDistance = FV3_PlaneDistance(a_normal, &a_triangle[0]);

	distance1 = (FixedMul(a_normal->x, a_line[0].x) + FixedMul(a_normal->y, a_line[0].y)
		+ FixedMul(a_normal->z, a_line[0].z)) + *originDistance;

	distance2 = (FixedMul(a_normal->x, a_line[1].x) + FixedMul(a_normal->y, a_line[1].y)
		+ FixedMul(a_normal->z, a_line[1].z)) + *originDistance;

	// Positive or zero number means no intersection
	if (FixedMul(distance1, distance2) >= 0)
		return false;

	return true;
}

//
// PlaneIntersection
//
// Returns the distance from
// rOrigin to where the ray
// intersects the plane. Assumes
// you already know it intersects
// the plane.
//
fixed_t FV3_PlaneIntersection(const vector3_t *pOrigin, const vector3_t *pNormal, const vector3_t *rOrigin, const vector3_t *rVector)
{
	fixed_t d = -(FV3_Dot(pNormal, pOrigin));
	fixed_t number = FV3_Dot(pNormal, rOrigin) + d;
	fixed_t denom = FV3_Dot(pNormal, rVector);
	return -FixedDiv(number, denom);
}

//
// IntersectRaySphere
// Input : rO - origin of ray in world space
//         rV - vector describing direction of ray in world space
//         sO - Origin of sphere
//         sR - radius of sphere
// Notes : Normalized directional vectors expected
// Return: distance to sphere in world units, -1 if no intersection.
//
fixed_t FV3_IntersectRaySphere(const vector3_t *rO, const vector3_t *rV, const vector3_t *sO, fixed_t sR)
{
	vector3_t Q;
	fixed_t c, v, d;
	FV3_SubEx(sO, rO, &Q);

	c = FV3_Magnitude(&Q);
	v = FV3_Dot(&Q, rV);
	d = FixedMul(sR, sR) - (FixedMul(c, c) - FixedMul(v, v));

	// If there was no intersection, return -1
	if (d < 0 * FRACUNIT)
		return (-1 * FRACUNIT);

	// Return the distance to the [first] intersecting point
	return (v - FixedSqrt(d));
}

//
// IntersectionPoint
//
// This returns the intersection point of the line that intersects the plane
//
vector3_t *FV3_IntersectionPoint(const vector3_t *vNormal, const vector3_t *vLine, fixed_t distance, vector3_t *ReturnVec)
{
	vector3_t vLineDir; // Variables to hold the point and the line's direction
	fixed_t Numerator = 0, Denominator = 0, dist = 0;

	// Here comes the confusing part.  We need to find the 3D point that is actually
	// on the plane.  Here are some steps to do that:

	// 1)  First we need to get the vector of our line, Then normalize it so it's a length of 1
	FV3_Point2Vec(&vLine[1], &vLine[0], &vLineDir);		// Get the Vector of the line
	FV3_NormalizeEx(&vLineDir, &vLineDir);				// Normalize the lines vector


	// 2) Use the plane equation (distance = Ax + By + Cz + D) to find the distance from one of our points to the plane.
	//    Here I just chose a arbitrary point as the point to find that distance.  You notice we negate that
	//    distance.  We negate the distance because we want to eventually go BACKWARDS from our point to the plane.
	//    By doing this is will basically bring us back to the plane to find our intersection point.
	Numerator = -(FixedMul(vNormal->x, vLine[0].x) +		// Use the plane equation with the normal and the line
		FixedMul(vNormal->y, vLine[0].y) +
		FixedMul(vNormal->z, vLine[0].z) + distance);

	// 3) If we take the dot product between our line vector and the normal of the polygon,
	//    this will give us the cosine of the angle between the 2 (since they are both normalized - length 1).
	//    We will then divide our Numerator by this value to find the offset towards the plane from our arbitrary point.
	Denominator = FV3_Dot(vNormal, &vLineDir);		// Get the dot product of the line's vector and the normal of the plane

	// Since we are using division, we need to make sure we don't get a divide by zero error
	// If we do get a 0, that means that there are INFINITE points because the the line is
	// on the plane (the normal is perpendicular to the line - (Normal.Vector = 0)).
	// In this case, we should just return any point on the line.

	if (Denominator == 0 * FRACUNIT) // Check so we don't divide by zero
	{
		ReturnVec->x = vLine[0].x;
		ReturnVec->y = vLine[0].y;
		ReturnVec->z = vLine[0].z;
		return ReturnVec;	// Return an arbitrary point on the line
	}

	// We divide the (distance from the point to the plane) by (the dot product)
	// to get the distance (dist) that we need to move from our arbitrary point.  We need
	// to then times this distance (dist) by our line's vector (direction).  When you times
	// a scalar (single number) by a vector you move along that vector.  That is what we are
	// doing.  We are moving from our arbitrary point we chose from the line BACK to the plane
	// along the lines vector.  It seems logical to just get the numerator, which is the distance
	// from the point to the line, and then just move back that much along the line's vector.
	// Well, the distance from the plane means the SHORTEST distance.  What about in the case that
	// the line is almost parallel with the polygon, but doesn't actually intersect it until half
	// way down the line's length.  The distance from the plane is short, but the distance from
	// the actual intersection point is pretty long.  If we divide the distance by the dot product
	// of our line vector and the normal of the plane, we get the correct length.  Cool huh?

	dist = FixedDiv(Numerator, Denominator); // Divide to get the multiplying (percentage) factor

	// Now, like we said above, we times the dist by the vector, then add our arbitrary point.
	// This essentially moves the point along the vector to a certain distance.  This now gives
	// us the intersection point.  Yay!

	// Return the intersection point
	ReturnVec->x = vLine[0].x + FixedMul(vLineDir.x, dist);
	ReturnVec->y = vLine[0].y + FixedMul(vLineDir.y, dist);
	ReturnVec->z = vLine[0].z + FixedMul(vLineDir.z, dist);
	return ReturnVec;
}

//
// PointOnLineSide
//
// If on the front side of the line, returns 1.
// If on the back side of the line, returns 0.
// 2D only.
//
UINT8 FV3_PointOnLineSide(const vector3_t *point, const vector3_t *line)
{
	fixed_t s1 = FixedMul((point->y - line[0].y), (line[1].x - line[0].x));
	fixed_t s2 = FixedMul((point->x - line[0].x), (line[1].y - line[0].y));
	return (UINT8)(s1 - s2 < 0);
}

//
// PointInsideBox
//
// Given four points of a box,
// determines if the supplied point is
// inside the box or not.
//
boolean FV3_PointInsideBox(const vector3_t *point, const vector3_t *box)
{
	vector3_t lastLine[2];

	FV3_Load(&lastLine[0], box[3].x, box[3].y, box[3].z);
	FV3_Load(&lastLine[1], box[0].x, box[0].y, box[0].z);

	if (FV3_PointOnLineSide(point, &box[0])
		|| FV3_PointOnLineSide(point, &box[1])
		|| FV3_PointOnLineSide(point, &box[2])
		|| FV3_PointOnLineSide(point, lastLine))
		return false;

	return true;
}
//
// LoadIdentity
//
// Loads the identity matrix into a matrix
//
void FM_LoadIdentity(matrix_t* matrix)
{
#define M(row,col)  matrix->m[col * 4 + row]
	memset(matrix, 0x00, sizeof(matrix_t));

	M(0, 0) = FRACUNIT;
	M(1, 1) = FRACUNIT;
	M(2, 2) = FRACUNIT;
	M(3, 3) = FRACUNIT;
#undef M
}

//
// CreateObjectMatrix
//
// Creates a matrix that can be used for
// adjusting the position of an object
//
void FM_CreateObjectMatrix(matrix_t *matrix, fixed_t x, fixed_t y, fixed_t z, fixed_t anglex, fixed_t angley, fixed_t anglez, fixed_t upx, fixed_t upy, fixed_t upz, fixed_t radius)
{
	vector3_t upcross;
	vector3_t upvec;
	vector3_t basevec;

	FV3_Load(&upvec, upx, upy, upz);
	FV3_Load(&basevec, anglex, angley, anglez);
	FV3_Cross(&upvec, &basevec, &upcross);
	FV3_Normalize(&upcross);

	FM_LoadIdentity(matrix);

	matrix->m[0] = upcross.x;
	matrix->m[1] = upcross.y;
	matrix->m[2] = upcross.z;
	matrix->m[3] = 0 * FRACUNIT;

	matrix->m[4] = upx;
	matrix->m[5] = upy;
	matrix->m[6] = upz;
	matrix->m[7] = 0;

	matrix->m[8] = anglex;
	matrix->m[9] = angley;
	matrix->m[10] = anglez;
	matrix->m[11] = 0;

	matrix->m[12] = x - FixedMul(upx, radius);
	matrix->m[13] = y - FixedMul(upy, radius);
	matrix->m[14] = z - FixedMul(upz, radius);
	matrix->m[15] = FRACUNIT;
}

//
// MultMatrixVec
//
// Multiplies a vector by the specified matrix
//
void FM_MultMatrixVec3(const matrix_t *matrix, const vector3_t *vec, vector3_t *out)
{
#define M(row,col)  matrix->m[col * 4 + row]
	out->x = FixedMul(vec->x, M(0, 0))
		+ FixedMul(vec->y, M(0, 1))
		+ FixedMul(vec->z, M(0, 2))
		+ M(0, 3);

	out->y = FixedMul(vec->x, M(1, 0))
		+ FixedMul(vec->y, M(1, 1))
		+ FixedMul(vec->z, M(1, 2))
		+ M(1, 3);

	out->z = FixedMul(vec->x, M(2, 0))
		+ FixedMul(vec->y, M(2, 1))
		+ FixedMul(vec->z, M(2, 2))
		+ M(2, 3);
#undef M
}

//
// MultMatrix
//
// Multiples one matrix into another
//
void FM_MultMatrix(matrix_t *dest, const matrix_t *multme)
{
	matrix_t result;
	UINT8 i, j;
#define M(row,col)  multme->m[col * 4 + row]
#define D(row,col)  dest->m[col * 4 + row]
#define R(row,col)  result.m[col * 4 + row]

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
			R(i, j) = FixedMul(D(i, 0), M(0, j)) + FixedMul(D(i, 1), M(1, j)) + FixedMul(D(i, 2), M(2, j)) + FixedMul(D(i, 3), M(3, j));
	}

	M_Memcpy(dest, &result, sizeof(matrix_t));

#undef R
#undef D
#undef M
}

//
// Translate
//
// Translates a matrix
//
void FM_Translate(matrix_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	matrix_t trans;
#define M(row,col)  trans.m[col * 4 + row]

	memset(&trans, 0x00, sizeof(matrix_t));

	M(0, 0) = M(1, 1) = M(2, 2) = M(3, 3) = FRACUNIT;
	M(0, 3) = x;
	M(1, 3) = y;
	M(2, 3) = z;

	FM_MultMatrix(dest, &trans);
#undef M
}

//
// Scale
//
// Scales a matrix
//
void FM_Scale(matrix_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	matrix_t scale;
#define M(row,col)  scale.m[col * 4 + row]

	memset(&scale, 0x00, sizeof(matrix_t));

	M(3, 3) = FRACUNIT;
	M(0, 0) = x;
	M(1, 1) = y;
	M(2, 2) = z;

	FM_MultMatrix(dest, &scale);
#undef M
}

#ifdef M_TESTCASE
//#define MULDIV_TEST
#define SQRT_TEST

static inline void M_print(INT64 a)
{
	const fixed_t w = (a >> FRACBITS);
	fixed_t f = a % FRACUNIT;
	fixed_t d = FRACUNIT;

	if (f == 0)
	{
		printf("%d", (fixed_t)w);
		return;
	}
	else while (f != 1 && f / 2 == f >> 1)
	{
		d /= 2;
		f /= 2;
	}

	if (w == 0)
		printf("%d/%d", (fixed_t)f, d);
	else
		printf("%d+(%d/%d)", (fixed_t)w, (fixed_t)f, d);
}

FUNCMATH FUNCINLINE static inline fixed_t FixedMulC(fixed_t a, fixed_t b)
{
	return (fixed_t)((((INT64)a * b)) / FRACUNIT);
}

FUNCMATH FUNCINLINE static inline fixed_t FixedDivC2(fixed_t a, fixed_t b)
{
	INT64 ret;

	if (b == 0)
		I_Error("FixedDiv: divide by zero");

	ret = (((INT64)a * FRACUNIT)) / b;

	if ((ret > INT32_MAX) || (ret < INT32_MIN))
		I_Error("FixedDiv: divide by zero");
	return (fixed_t)ret;
}

FUNCMATH FUNCINLINE static inline fixed_t FixedDivC(fixed_t a, fixed_t b)
{
	if ((abs(a) >> (FRACBITS - 2)) >= abs(b))
		return (a^b) < 0 ? INT32_MIN : INT32_MAX;

	return FixedDivC2(a, b);
}

FUNCMATH FUNCINLINE static inline fixed_t FixedSqrtC(fixed_t x)
{
	const float fx = FIXED_TO_FLOAT(x);
	float fr;
#ifdef HAVE_SQRTF
	fr = sqrtf(fx);
#else
	fr = (float)sqrt(fx);
#endif
	return FLOAT_TO_FIXED(fr);
}
int main(int argc, char** argv)
{
	int n = 10;
	INT64 a, b;
	fixed_t c, d;
	(void)argc;
	(void)argv;

#ifdef MULDIV_TEST
	for (a = 1; a <= INT32_MAX; a += FRACUNIT)
		for (b = 0; b <= INT32_MAX; b += FRACUNIT)
		{
			c = FixedMul(a, b);
			d = FixedMulC(a, b);
			if (c != d)
			{
				printf("(");
				M_print(a);
				printf(") * (");
				M_print(b);
				printf(") = (");
				M_print(c);
				printf(") != (");
				M_print(d);
				printf(") \n");
				n--;
				printf("%d != %d\n", c, d);
			}
			c = FixedDiv(a, b);
			d = FixedDivC(a, b);
			if (c != d)
			{
				printf("(");
				M_print(a);
				printf(") / (");
				M_print(b);
				printf(") = (");
				M_print(c);
				printf(") != (");
				M_print(d);
				printf(")\n");
				n--;
				printf("%d != %d\n", c, d);
			}
			if (n <= 0)
				exit(-1);
		}
#endif

#ifdef SQRT_TEST
	for (a = 0; a <= INT32_MAX; a += 1)
	{
		c = FixedSqrt(a);
		d = FixedSqrtC(a);
		b = abs(c - d);
		if (b > 1)
		{
			printf("sqrt(");
			M_print(a);
			printf(") = {(");
			M_print(c);
			printf(") != (");
			M_print(d);
			printf(")} \n");
			//n--;
			printf("%d != %d {", c, d);
			M_print(b);
			printf("}\n");
		}
		if (n <= 0)
			exit(-1);
	}
#endif
	exit(0);
}

static void *cpu_cpy(void *dest, const void *src, size_t n)
{
	return memcpy(dest, src, n);
}

void *(*M_Memcpy)(void* dest, const void* src, size_t n) = cpu_cpy;

void I_Error(const char *error, ...)
{
	(void)error;
	exit(-1);
}
#endif
