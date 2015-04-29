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

#include "doomdef.h"
#include "m_vector.h"
#include "r_main.h"
#include "m_fixed.h"
#include "m_misc.h"
#include "tables.h"

#ifdef ESLOPE

v3fixed_t *M_LoadVec(v3fixed_t *vec, fixed_t x, fixed_t y, fixed_t z)
{
	vec->x = x;
	vec->y = y;
	vec->z = z;
	return vec;
}

v3fixed_t *M_CopyVec(v3fixed_t *a_o, const v3fixed_t *a_i)
{
	return M_Memcpy(a_o, a_i, sizeof(v3fixed_t));
}

v3float_t *M_LoadVecf(v3float_t *vec, float x, float y, float z)
{
	vec->x = x;
	vec->y = y;
	vec->z = z;
	return vec;
}

v3float_t *M_CopyVecf(v3float_t *a_o, const v3float_t *a_i)
{
	return M_Memcpy(a_o, a_i, sizeof(v3float_t));
}

//
// M_MakeVec3
//
// Given two points, create a vector between them.
//
v3fixed_t *M_MakeVec3(const v3fixed_t *point1, const v3fixed_t *point2, v3fixed_t *a_o)
{
	a_o->x = point1->x - point2->x;
	a_o->y = point1->y - point2->y;
	a_o->z = point1->z - point2->z;
	return a_o;
}


//
// M_MakeVec3f
//
// Given two points, create a vector between them.
//
v3float_t *M_MakeVec3f(const v3float_t *point1, const v3float_t *point2, v3float_t *a_o)
{
	a_o->x = point1->x - point2->x;
	a_o->y = point1->y - point2->y;
	a_o->z = point1->z - point2->z;
	return a_o;
}

//
// M_TranslateVec3
//
// Translates the given vector (in the game's coordinate system) to the camera
// space (in right-handed coordinate system) This function is used for slopes.
//
void M_TranslateVec3(v3fixed_t *vec)
{
	fixed_t tx, ty, tz;

	tx = vec->x - viewx;
	ty = viewz - vec->y;
	tz = vec->z - viewy;

	// Just like wall projection.
	vec->x = (tx * viewcos) - (tz * viewsin);
	vec->z = (tz * viewcos) + (tx * viewsin);
	vec->y = ty;
}

//
// M_TranslateVec3f
//
// Translates the given vector (in the game's coordinate system) to the camera
// space (in right-handed coordinate system) This function is used for slopes.
//
void M_TranslateVec3f(v3float_t *vec)
{
   float tx, ty, tz;

   tx = vec->x - viewx; // SRB2CBTODO: This may need float viewxyz
   ty = viewz - vec->y;
   tz = vec->z - viewy;

   // Just like wall projection.
   vec->x = (tx * viewcos) - (tz * viewsin);
   vec->z = (tz * viewcos) + (tx * viewsin);
   vec->y = ty;
}

#ifdef SESLOPE
//
// M_TranslateVec3d
//
// Translates the given vector (in the game's coordinate system) to the camera
// space (in right-handed coordinate system) This function is used for slopes.
//
void M_TranslateVec3d(v3double_t *vec)
{
	double tx, ty, tz;

	tx = vec->x - viewx; // SRB2CBTODO: This may need float viewxyz
	ty = viewz - vec->y;
	tz = vec->z - viewy;

	// Just like wall projection.
	vec->x = (tx * viewcos) - (tz * viewsin);
	vec->z = (tz * viewcos) + (tx * viewsin);
	vec->y = ty;
}
#endif

//
// M_AddVec3
//
// Adds v2 to v1 stores in dest
//
void M_AddVec3(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2)
{
	dest->x = v1->x + v2->x;
	dest->y = v1->y + v2->y;
	dest->z = v1->z + v2->z;
}

//
// M_AddVec3f
//
// Adds v2 to v1 stores in dest
//
void M_AddVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2)
{
   dest->x = v1->x + v2->x;
   dest->y = v1->y + v2->y;
   dest->z = v1->z + v2->z;
}

//
// M_SubVec3
//
// Adds v2 to v1 stores in dest
//
void M_SubVec3(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2) // SRB2CBTODO: Make a function that allows the destxyz to equal the change of 2 args
{
	dest->x = v1->x - v2->x;
	dest->y = v1->y - v2->y;
	dest->z = v1->z - v2->z;
}

//
// M_SubVec3f
//
// Subtracts v2 from v1 stores in dest
//
void M_SubVec3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2)
{
   dest->x = v1->x - v2->x;
   dest->y = v1->y - v2->y;
   dest->z = v1->z - v2->z;
}

//
// M_DotVec3
//
// Returns the dot product of v1 and v2
//
fixed_t M_DotVec3(const v3fixed_t *v1, const v3fixed_t *v2)
{
	return FixedMul(v1->x, v2->x) + FixedMul(v1->y, v2->y) + FixedMul(v1->z, v2->z);
}

//
// M_DotVec3f
//
// Returns the dot product of v1 and v2
//
float M_DotVec3f(const v3float_t *v1, const v3float_t *v2)
{
	if (!v1 || !v2)
		I_Error("M_DotVec3f: No vertexes!");
	return (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}

#ifdef SESLOPE
//
// M_DotVec3d
//
// Returns the dot product of v1 and v2
//
double M_DotVec3d(const v3double_t *v1, const v3double_t *v2)
{
	return (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}
#endif

//
// M_CrossProduct3
//
// Gets the cross product of v1 and v2 and stores in dest
//
void M_CrossProduct3(v3fixed_t *dest, const v3fixed_t *v1, const v3fixed_t *v2)
{
	v3fixed_t tmp;
	tmp.x = (v1->y * v2->z) - (v1->z * v2->y);
	tmp.y = (v1->z * v2->x) - (v1->x * v2->z);
	tmp.z = (v1->x * v2->y) - (v1->y * v2->x);
	memcpy(dest, &tmp, sizeof(v3fixed_t));
}

//
// M_CrossProduct3f
//
// Gets the cross product of v1 and v2 and stores in dest
//
void M_CrossProduct3f(v3float_t *dest, const v3float_t *v1, const v3float_t *v2)
{
   v3float_t tmp;
   tmp.x = (v1->y * v2->z) - (v1->z * v2->y);
   tmp.y = (v1->z * v2->x) - (v1->x * v2->z);
   tmp.z = (v1->x * v2->y) - (v1->y * v2->x);
   memcpy(dest, &tmp, sizeof(v3float_t));
}

fixed_t FV_Magnitude(const v3fixed_t *a_normal)
{
	fixed_t xs = FixedMul(a_normal->x,a_normal->x);
	fixed_t ys = FixedMul(a_normal->y,a_normal->y);
	fixed_t zs = FixedMul(a_normal->z,a_normal->z);
	return FixedSqrt(xs+ys+zs);
}

float FV_Magnitudef(const v3float_t *a_normal)
{
	float xs = (a_normal->x * a_normal->x);
	float ys = (a_normal->y * a_normal->y);
	float zs = (a_normal->z * a_normal->z);
	return (float)sqrt(xs+ys+zs);
}

// Vector Complex Math
v3fixed_t *FV_Midpoint(const v3fixed_t *a_1, const v3fixed_t *a_2, v3fixed_t *a_o)
{
	a_o->x = FixedDiv(a_2->x - a_1->x, 2*FRACUNIT);
	a_o->y = FixedDiv(a_2->y - a_1->y, 2*FRACUNIT);
	a_o->z = FixedDiv(a_2->z - a_1->z, 2*FRACUNIT);
	a_o->x = a_1->x + a_o->x;
	a_o->y = a_1->y + a_o->y;
	a_o->z = a_1->z + a_o->z;
	return a_o;
}

fixed_t FV_Distance(const v3fixed_t *p1, const v3fixed_t *p2)
{
	fixed_t xs = FixedMul(p2->x-p1->x,p2->x-p1->x);
	fixed_t ys = FixedMul(p2->y-p1->y,p2->y-p1->y);
	fixed_t zs = FixedMul(p2->z-p1->z,p2->z-p1->z);
	return FixedSqrt(xs+ys+zs);
}

v3float_t *FV_Midpointf(const v3float_t *a_1, const v3float_t *a_2, v3float_t *a_o)
{
	a_o->x = (a_2->x - a_1->x / 2.0f);
	a_o->y = (a_2->y - a_1->y / 2.0f);
	a_o->z = (a_2->z - a_1->z / 2.0f);
	a_o->x = a_1->x + a_o->x;
	a_o->y = a_1->y + a_o->y;
	a_o->z = a_1->z + a_o->z;
	return a_o;
}



//
// AngleBetweenVectors
//
// This checks to see if a point is inside the ranges of a polygon
//
angle_t FV_AngleBetweenVectors(const v3fixed_t *Vector1, const v3fixed_t *Vector2)
{
	// Remember, above we said that the Dot Product of returns the cosine of the angle
	// between 2 vectors?  Well, that is assuming they are unit vectors (normalize vectors).
	// So, if we don't have a unit vector, then instead of just saying  arcCos(DotProduct(A, B))
	// We need to divide the dot product by the magnitude of the 2 vectors multiplied by each other.
	// Here is the equation:   arc cosine of (V . W / || V || * || W || )
	// the || V || means the magnitude of V.  This then cancels out the magnitudes dot product magnitudes.
	// But basically, if you have normalize vectors already, you can forget about the magnitude part.

	// Get the dot product of the vectors
	fixed_t dotProduct = M_DotVec3(Vector1, Vector2);

	// Get the product of both of the vectors magnitudes
	fixed_t vectorsMagnitude = FixedMul(FV_Magnitude(Vector1), FV_Magnitude(Vector2));

	// Return the arc cosine of the (dotProduct / vectorsMagnitude) which is the angle in RADIANS.
	return FixedAcos(FixedDiv(dotProduct, vectorsMagnitude));
}

float FV_AngleBetweenVectorsf(const v3float_t *Vector1, const v3float_t *Vector2)
{
	// Remember, above we said that the Dot Product of returns the cosine of the angle
	// between 2 vectors?  Well, that is assuming they are unit vectors (normalize vectors).
	// So, if we don't have a unit vector, then instead of just saying  arcCos(DotProduct(A, B))
	// We need to divide the dot product by the magnitude of the 2 vectors multiplied by each other.
	// Here is the equation:   arc cosine of (V . W / || V || * || W || )
	// the || V || means the magnitude of V.  This then cancels out the magnitudes dot product magnitudes.
	// But basically, if you have normalize vectors already, you can forget about the magnitude part.

	// Get the dot product of the vectors
	float dotProduct = M_DotVec3f(Vector1, Vector2);

	// Get the product of both of the vectors magnitudes
	float vectorsMagnitude = FV_Magnitudef(Vector1)*FV_Magnitudef(Vector2);

	// Return the arc cosine of the (dotProduct / vectorsMagnitude) which is the angle in RADIANS.
	return acos(dotProduct/vectorsMagnitude);
}



// Crazy physics code

float M_VectorYaw(v3float_t v)
{
	return atan2(v.x, v.z);
}
float M_VectorPitch(v3float_t v)
{
	return -atan2(v.y, sqrt(v.x*v.x+v.z*v.z));
}

#include "z_zone.h"

// Returns pitch roll and yaw values, allows objects to align to a slope
angles3d_t *M_VectorAlignTo(float Pitch, float Yaw, float Roll, v3float_t v, byte AngleAxis, float Rate)
{
	CONS_Printf("P %f\n", Pitch);
	CONS_Printf("R %f\n", Roll);
	CONS_Printf("Y %f\n", Yaw);
	if (AngleAxis == 1)
	{
		float DestYaw   = (atan2(v.z,v.x)* 180 / M_PI);
		float DestRoll  = (atan2(v.y,v.x)* 180 / M_PI);

		Yaw   = Yaw+(DestYaw-Yaw)*Rate;
		Roll  = Roll+(DestRoll-Roll)*Rate;
	}
	else if (AngleAxis == 2)
	{
		float DestPitch = (atan2(v.z,v.y)* 180 / M_PI);
		float DestRoll  = (-atan2(v.x,v.y)* 180 / M_PI);

		Pitch = Pitch+(DestPitch-Pitch)*Rate;
		Roll  = Roll+(DestRoll-Roll)*Rate;
	}
	else if (AngleAxis == 3)
	{
		float DestPitch = (-atan2(v.y,v.z)* 180 / M_PI);
		float DestYaw   = (-atan2(v.x,v.z)* 180 / M_PI);

		Pitch = Pitch+(DestPitch-Pitch)*Rate;
		Yaw   = Yaw+(DestYaw-Yaw)*Rate;
	}

	angles3d_t *returnangles = Z_Malloc(sizeof(angles3d_t), PU_LEVEL, NULL);
	memset(returnangles, 0, sizeof(*returnangles));
	returnangles->yaw = Yaw;
	returnangles->pitch = Pitch;
	returnangles->roll = Roll;

	return returnangles;

}

//
// RotateVector
//
// Rotates a vector around another vector
//
void M_VecRotate(v3fixed_t *rotVec, const v3fixed_t *axisVec, const angle_t angle)
{
	// Rotate the point (x,y,z) around the vector (u,v,w)
	fixed_t ux = FixedMul(axisVec->x, rotVec->x);
	fixed_t uy = FixedMul(axisVec->x, rotVec->y);
	fixed_t uz = FixedMul(axisVec->x, rotVec->z);
	fixed_t vx = FixedMul(axisVec->y, rotVec->x);
	fixed_t vy = FixedMul(axisVec->y, rotVec->y);
	fixed_t vz = FixedMul(axisVec->y, rotVec->z);
	fixed_t wx = FixedMul(axisVec->z, rotVec->x);
	fixed_t wy = FixedMul(axisVec->z, rotVec->y);
	fixed_t wz = FixedMul(axisVec->z, rotVec->z);
	fixed_t sa = FINESINE(angle>>ANGLETOFINESHIFT);
	fixed_t ca = FINECOSINE(angle>>ANGLETOFINESHIFT);
	fixed_t ua = ux+vy+wz;
	fixed_t ax = FixedMul(axisVec->x,ua);
	fixed_t ay = FixedMul(axisVec->y,ua);
	fixed_t az = FixedMul(axisVec->z,ua);
	fixed_t xs = FixedMul(axisVec->x,axisVec->x);
	fixed_t ys = FixedMul(axisVec->y,axisVec->y);
	fixed_t zs = FixedMul(axisVec->z,axisVec->z);
	fixed_t bx = FixedMul(rotVec->x,ys+zs);
	fixed_t by = FixedMul(rotVec->y,xs+zs);
	fixed_t bz = FixedMul(rotVec->z,xs+ys);
	fixed_t cx = FixedMul(axisVec->x,vy+wz);
	fixed_t cy = FixedMul(axisVec->y,ux+wz);
	fixed_t cz = FixedMul(axisVec->z,ux+vy);
	fixed_t dx = FixedMul(bx-cx, ca);
	fixed_t dy = FixedMul(by-cy, ca);
	fixed_t dz = FixedMul(bz-cz, ca);
	fixed_t ex = FixedMul(vz-wy, sa);
	fixed_t ey = FixedMul(wx-uz, sa);
	fixed_t ez = FixedMul(uy-vx, sa);

	rotVec->x = ax+dx+ex;
	rotVec->y = ay+dy+ey;
	rotVec->z = az+dz+ez;
}





#if 0 // Backport
v3fixed_t *FV_SubO(const v3fixed_t *a_i, const v3fixed_t *a_c, v3fixed_t *a_o)
{
	a_o->x = a_i->x - a_c->x;
	a_o->y = a_i->y - a_c->y;
	a_o->z = a_i->z - a_c->z;
	return a_o;
}

boolean FV_Equal(const v3fixed_t *a_1, const v3fixed_t *a_2)
{
	fixed_t Epsilon = FRACUNIT/FRACUNIT;

	if ((abs(a_2->x - a_1->x) > Epsilon) ||
		(abs(a_2->y - a_1->y) > Epsilon) ||
		(abs(a_2->z - a_1->z) > Epsilon))
	{
		return true;
	}

	return false;
}

boolean FV_Equalf(const v3float_t *a_1, const v3float_t *a_2)
{
	float Epsilon = 1.0f/1.0f;

	if ((abs(a_2->x - a_1->x) > Epsilon) ||
		(abs(a_2->y - a_1->y) > Epsilon) ||
		(abs(a_2->z - a_1->z) > Epsilon))
	{
		return true;
	}

	return false;
}

//
// Normal
//
// Calculates the normal of a polygon.
//
void FV_Normal (const v3fixed_t *a_triangle, v3fixed_t *a_normal)
{
	v3fixed_t a_1;
	v3fixed_t a_2;

	FV_Point2Vec(&a_triangle[2], &a_triangle[0], &a_1);
	FV_Point2Vec(&a_triangle[1], &a_triangle[0], &a_2);

	FV_Cross(&a_1, &a_2, a_normal);

	FV_NormalizeO(a_normal, a_normal);
}

//
// PlaneDistance
//
// Calculates distance between a plane and the origin.
//
fixed_t FV_PlaneDistance(const v3fixed_t *a_normal, const v3fixed_t *a_point)
{
	return -(FixedMul(a_normal->x, a_point->x) + FixedMul(a_normal->y, a_point->y) + FixedMul(a_normal->z, a_point->z));
}

boolean FV_IntersectedPlane(const v3fixed_t *a_triangle, const v3fixed_t *a_line, v3fixed_t *a_normal, fixed_t *originDistance)
{
	fixed_t distance1 = 0, distance2 = 0;

	FV_Normal(a_triangle, a_normal);

	*originDistance = FV_PlaneDistance(a_normal, &a_triangle[0]);

	distance1 = (FixedMul(a_normal->x, a_line[0].x)  + FixedMul(a_normal->y, a_line[0].y)
				 + FixedMul(a_normal->z, a_line[0].z)) + *originDistance;

	distance2 = (FixedMul(a_normal->x, a_line[1].x)  + FixedMul(a_normal->y, a_line[1].y)
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
fixed_t FV_PlaneIntersection(const v3fixed_t *pOrigin, const v3fixed_t *pNormal, const v3fixed_t *rOrigin, const v3fixed_t *rVector)
{
	fixed_t d = -(FV_Dot(pNormal, pOrigin));
	fixed_t number = FV_Dot(pNormal,rOrigin) + d;
	fixed_t denom = FV_Dot(pNormal,rVector);
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
fixed_t FV_IntersectRaySphere(const v3fixed_t *rO, const v3fixed_t *rV, const v3fixed_t *sO, fixed_t sR)
{
	v3fixed_t Q;
	fixed_t c, v, d;
	FV_SubO(sO, rO, &Q);

	c = FV_Magnitude(&Q);
	v = FV_Dot(&Q, rV);
	d = FixedMul(sR, sR) - (FixedMul(c,c) - FixedMul(v,v));

	// If there was no intersection, return -1
	if (d < 0*FRACUNIT)
		return (-1*FRACUNIT);

	// Return the distance to the [first] intersecting point
	return (v - FixedSqrt(d));
}

//
// IntersectionPoint
//
// This returns the intersection point of the line that intersects the plane
//
v3fixed_t *FV_IntersectionPoint(const v3fixed_t *vNormal, const v3fixed_t *vLine, fixed_t distance, v3fixed_t *ReturnVec)
{
	v3fixed_t vLineDir; // Variables to hold the point and the line's direction
	fixed_t Numerator = 0, Denominator = 0, dist = 0;

	// Here comes the confusing part.  We need to find the 3D point that is actually
	// on the plane.  Here are some steps to do that:

	// 1)  First we need to get the vector of our line, Then normalize it so it's a length of 1
	FV_Point2Vec(&vLine[1], &vLine[0], &vLineDir);		// Get the Vector of the line
	FV_NormalizeO(&vLineDir, &vLineDir);				// Normalize the lines vector


	// 2) Use the plane equation (distance = Ax + By + Cz + D) to find the distance from one of our points to the plane.
	//    Here I just chose a arbitrary point as the point to find that distance.  You notice we negate that
	//    distance.  We negate the distance because we want to eventually go BACKWARDS from our point to the plane.
	//    By doing this is will basically bring us back to the plane to find our intersection point.
	Numerator = - (FixedMul(vNormal->x, vLine[0].x) +		// Use the plane equation with the normal and the line
				   FixedMul(vNormal->y, vLine[0].y) +
				   FixedMul(vNormal->z, vLine[0].z) + distance);

	// 3) If we take the dot product between our line vector and the normal of the polygon,
	//    this will give us the cosine of the angle between the 2 (since they are both normalized - length 1).
	//    We will then divide our Numerator by this value to find the offset towards the plane from our arbitrary point.
	Denominator = FV_Dot(vNormal, &vLineDir);		// Get the dot product of the line's vector and the normal of the plane

	// Since we are using division, we need to make sure we don't get a divide by zero error
	// If we do get a 0, that means that there are INFINITE points because the the line is
	// on the plane (the normal is perpendicular to the line - (Normal.Vector = 0)).
	// In this case, we should just return any point on the line.

	if( Denominator == 0*FRACUNIT) // Check so we don't divide by zero
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

	dist = FixedDiv(Numerator, Denominator);				// Divide to get the multiplying (percentage) factor

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
unsigned int FV_PointOnLineSide(const v3fixed_t *point, const v3fixed_t *line)
{
	fixed_t s1 = FixedMul((point->y - line[0].y),(line[1].x - line[0].x));
	fixed_t s2 = FixedMul((point->x - line[0].x),(line[1].y - line[0].y));
	return s1 - s2 < 0;
}

//
// PointInsideBox
//
// Given four points of a box,
// determines if the supplied point is
// inside the box or not.
//
boolean FV_PointInsideBox(const v3fixed_t *point, const v3fixed_t *box)
{
	v3fixed_t lastLine[2];

	FV_Load(&lastLine[0], box[3].x, box[3].y, box[3].z);
	FV_Load(&lastLine[1], box[0].x, box[0].y, box[0].z);

	if (FV_PointOnLineSide(point, &box[0])
		|| FV_PointOnLineSide(point, &box[1])
		|| FV_PointOnLineSide(point, &box[2])
		|| FV_PointOnLineSide(point, lastLine))
		return false;

	return true;
}
//
// LoadIdentity
//
// Loads the identity matrix into a matrix
//
void FM_LoadIdentity(fmatrix_t* matrix)
{
#define M(row,col)  matrix->m[col * 4 + row]
	memset(matrix, 0x00, sizeof(fmatrix_t));

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
void FM_CreateObjectMatrix(fmatrix_t *matrix, fixed_t x, fixed_t y, fixed_t z, fixed_t anglex, fixed_t angley, fixed_t anglez, fixed_t upx, fixed_t upy, fixed_t upz, fixed_t radius)
{
	v3fixed_t upcross;
	v3fixed_t upvec;
	v3fixed_t basevec;

	FV_Load(&upvec, upx, upy, upz);
	FV_Load(&basevec, anglex, angley, anglez);
	FV_Cross(&upvec, &basevec, &upcross);
	FV_Normalize(&upcross);

	FM_LoadIdentity(matrix);

	matrix->m[0] = upcross.x;
	matrix->m[1] = upcross.y;
	matrix->m[2] = upcross.z;
	matrix->m[3] = 0*FRACUNIT;

	matrix->m[4] = upx;
	matrix->m[5] = upy;
	matrix->m[6] = upz;
	matrix->m[7] = 0;

	matrix->m[8] = anglex;
	matrix->m[9] = angley;
	matrix->m[10] = anglez;
	matrix->m[11] = 0;

	matrix->m[12] = x - FixedMul(upx,radius);
	matrix->m[13] = y - FixedMul(upy,radius);
	matrix->m[14] = z - FixedMul(upz,radius);
	matrix->m[15] = FRACUNIT;
}

//
// MultMatrixVec
//
// Multiplies a vector by the specified matrix
//
void FM_MultMatrixVec(const fmatrix_t *matrix, const v3fixed_t *vec, v3fixed_t *out)
{
#define M(row,col)  matrix->m[col * 4 + row]
	out->x = FixedMul(vec->x,M(0, 0))
	+ FixedMul(vec->y,M(0, 1))
	+ FixedMul(vec->z,M(0, 2))
	+ M(0, 3);

	out->y = FixedMul(vec->x,M(1, 0))
	+ FixedMul(vec->y,M(1, 1))
	+ FixedMul(vec->z,M(1, 2))
	+ M(1, 3);

	out->z = FixedMul(vec->x,M(2, 0))
	+ FixedMul(vec->y,M(2, 1))
	+ FixedMul(vec->z,M(2, 2))
	+ M(2, 3);
#undef M
}

//
// MultMatrix
//
// Multiples one matrix into another
//
void FM_MultMatrix(fmatrix_t *dest, const fmatrix_t *multme)
{
	fmatrix_t result;
	unsigned int i, j;
#define M(row,col)  multme->m[col * 4 + row]
#define D(row,col)  dest->m[col * 4 + row]
#define R(row,col)  result.m[col * 4 + row]

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
			R(i, j) = FixedMul(D(i, 0),M(0, j)) + FixedMul(D(i, 1),M(1, j)) + FixedMul(D(i, 2),M(2, j)) + FixedMul(D(i, 3),M(3, j));
	}

	M_Memcpy(dest, &result, sizeof(fmatrix_t));

#undef R
#undef D
#undef M
}

//
// Translate
//
// Translates a matrix
//
void FM_Translate(fmatrix_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	fmatrix_t trans;
#define M(row,col)  trans.m[col * 4 + row]

	memset(&trans, 0x00, sizeof(fmatrix_t));

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
void FM_Scale(fmatrix_t *dest, fixed_t x, fixed_t y, fixed_t z)
{
	fmatrix_t scale;
#define M(row,col)  scale.m[col * 4 + row]

	memset(&scale, 0x00, sizeof(fmatrix_t));

	M(3, 3) = FRACUNIT;
	M(0, 0) = x;
	M(1, 1) = y;
	M(2, 2) = z;

	FM_MultMatrix(dest, &scale);
#undef M
}


v3fixed_t *FV_Cross(const v3fixed_t *a_1, const v3fixed_t *a_2, v3fixed_t *a_o)
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
v3fixed_t *FV_ClosestPointOnLine(const v3fixed_t *Line, const v3fixed_t *p, v3fixed_t *out)
{
	// Determine t (the length of the vector from √´Line[0]√≠ to √´p√≠)
	v3fixed_t c, V;
	fixed_t t, d = 0;
	FV_SubO(p, &Line[0], &c);
	FV_SubO(&Line[1], &Line[0], &V);
	FV_NormalizeO(&V, &V);

	d = FV_Distance(&Line[0], &Line[1]);
	t = FV_Dot(&V, &c);

	// Check to see if √´t√≠ is beyond the extents of the line segment
	if (t < 0)
	{
		return FV_Copy(out, &Line[0]);
	}
	if (t > d)
	{
		return FV_Copy(out, &Line[1]);
	}

	// Return the point between √´Line[0]√≠ and √´Line[1]√≠
	FV_Mul(&V, t);

	return FV_AddO(&Line[0], &V, out);
}

//
// ClosestPointOnTriangle
//
// Given a triangle and a point,
// the closest point on the edge of
// the triangle is returned.
//
void FV_ClosestPointOnTriangle (const v3fixed_t *tri, const v3fixed_t *point, v3fixed_t *result)
{
	unsigned int i;
	fixed_t dist, closestdist;
	v3fixed_t EdgePoints[3];
	v3fixed_t Line[2];

	FV_Copy(&Line[0], (v3fixed_t*)&tri[0]);
	FV_Copy(&Line[1], (v3fixed_t*)&tri[1]);
	FV_ClosestPointOnLine(Line, point, &EdgePoints[0]);

	FV_Copy(&Line[0], (v3fixed_t*)&tri[1]);
	FV_Copy(&Line[1], (v3fixed_t*)&tri[2]);
	FV_ClosestPointOnLine(Line, point, &EdgePoints[1]);

	FV_Copy(&Line[0], (v3fixed_t*)&tri[2]);
	FV_Copy(&Line[1], (v3fixed_t*)&tri[0]);
	FV_ClosestPointOnLine(Line, point, &EdgePoints[2]);

	// Find the closest one of the three
	FV_Copy(result, &EdgePoints[0]);
	closestdist = FV_Distance(point, &EdgePoints[0]);
	for (i = 1; i < 3; i++)
	{
		dist = FV_Distance(point, &EdgePoints[i]);

		if (dist < closestdist)
		{
			closestdist = dist;
			FV_Copy(result, &EdgePoints[i]);
		}
	}

	// We now have the closest point! Whee!
}

//
// InsidePolygon
//
// This checks to see if a point is inside the ranges of a polygon
//
boolean FV_InsidePolygon(const fvector_t *vIntersection, const fvector_t *Poly, const int vertexCount)
{
	int i;
	UINT64 Angle = 0;					// Initialize the angle
	fvector_t vA, vB;					// Create temp vectors

	// Just because we intersected the plane, doesn't mean we were anywhere near the polygon.
	// This functions checks our intersection point to make sure it is inside of the polygon.
	// This is another tough function to grasp at first, but let me try and explain.
	// It's a brilliant method really, what it does is create triangles within the polygon
	// from the intersection point.  It then adds up the inner angle of each of those triangles.
	// If the angles together add up to 360 degrees (or 2 * PI in radians) then we are inside!
	// If the angle is under that value, we must be outside of polygon.  To further
	// understand why this works, take a pencil and draw a perfect triangle.  Draw a dot in
	// the middle of the triangle.  Now, from that dot, draw a line to each of the vertices.
	// Now, we have 3 triangles within that triangle right?  Now, we know that if we add up
	// all of the angles in a triangle we get 360 right?  Well, that is kinda what we are doing,
	// but the inverse of that.  Say your triangle is an isosceles triangle, so add up the angles
	// and you will get 360 degree angles.  90 + 90 + 90 is 360.

	for (i = 0; i < vertexCount; i++)		// Go in a circle to each vertex and get the angle between
	{
		FV_Point2Vec(&Poly[i], vIntersection, &vA);	// Subtract the intersection point from the current vertex
		// Subtract the point from the next vertex
		FV_Point2Vec(&Poly[(i + 1) % vertexCount], vIntersection, &vB);

		Angle += FV_AngleBetweenVectors(&vA, &vB);	// Find the angle between the 2 vectors and add them all up as we go along
	}

	// Now that we have the total angles added up, we need to check if they add up to 360 degrees.
	// Since we are using the dot product, we are working in radians, so we check if the angles
	// equals 2*PI.  We defined PI in 3DMath.h.  You will notice that we use a MATCH_FACTOR
	// in conjunction with our desired degree.  This is because of the inaccuracy when working
	// with floating point numbers.  It usually won't always be perfectly 2 * PI, so we need
	// to use a little twiddling.  I use .9999, but you can change this to fit your own desired accuracy.

	if(Angle >= ANGLE_MAX)	// If the angle is greater than 2 PI, (360 degrees)
		return 1; // The point is inside of the polygon

	return 0; // If you get here, it obviously wasn't inside the polygon.
}

//
// IntersectedPolygon
//
// This checks if a line is intersecting a polygon
//
boolean FV_IntersectedPolygon(const fvector_t *vPoly, const fvector_t *vLine, const int vertexCount, fvector_t *collisionPoint)
{
	fvector_t vNormal, vIntersection;
	fixed_t originDistance = 0*FRACUNIT;


	// First we check to see if our line intersected the plane.  If this isn't true
	// there is no need to go on, so return false immediately.
	// We pass in address of vNormal and originDistance so we only calculate it once

	if(!FV_IntersectedPlane(vPoly, vLine,   &vNormal,   &originDistance))
		return false;

	// Now that we have our normal and distance passed back from IntersectedPlane(),
	// we can use it to calculate the intersection point.  The intersection point
	// is the point that actually is ON the plane.  It is between the line.  We need
	// this point test next, if we are inside the polygon.  To get the I-Point, we
	// give our function the normal of the plane, the points of the line, and the originDistance.

	FV_IntersectionPoint(&vNormal, vLine, originDistance, &vIntersection);

	// Now that we have the intersection point, we need to test if it's inside the polygon.
	// To do this, we pass in :
	// (our intersection point, the polygon, and the number of vertices our polygon has)

	if(FV_InsidePolygon(&vIntersection, vPoly, vertexCount))
	{
		if (collisionPoint != NULL) // Optional - load the collision point.
		{
			collisionPoint->x = vIntersection.x;
			collisionPoint->y = vIntersection.y;
			collisionPoint->z = vIntersection.z;
		}
		return true; // We collided!
	}

	// If we get here, we must have NOT collided
	return false;
}

void FM_Rotate(fmatrix_t *dest, angle_t angle, fixed_t x, fixed_t y, fixed_t z)
{
#define M(row,col) dest->m[row * 4 + col]
	const fixed_t sinA = FINESINE(angle>>ANGLETOFINESHIFT);
	const fixed_t cosA = FINECOSINE(angle>>ANGLETOFINESHIFT);
	const fixed_t invCosA = FRACUNIT - cosA;
	fvector_t nrm = {x, y, z};
	fixed_t xSq, ySq, zSq;
	fixed_t sx, sy, sz;
	fixed_t sxy, sxz, syz;

	FV_Normalize(&nrm);

	x = nrm.x;
	y = nrm.y;
	z = nrm.z;

	xSq = FixedMul(x, FixedMul(invCosA,x));
	ySq = FixedMul(y, FixedMul(invCosA,y));
	zSq = FixedMul(z, FixedMul(invCosA,z));

	sx = FixedMul(sinA, x);
	sy = FixedMul(sinA, y);
	sz = FixedMul(sinA, z);

	sxy = FixedMul(x, FixedMul(invCosA,y));
	sxz = FixedMul(x, FixedMul(invCosA,z));
	syz = FixedMul(y, FixedMul(invCosA,z));


	M(0, 0) = xSq + cosA;
	M(1, 0) = sxy - sz;
	M(2, 0) = sxz + sy;
	M(3, 0) = 0;

	M(0, 1) = sxy + sz;
	M(1, 1) = ySq + cosA;
	M(2, 1) = syz - sx;
	M(3, 1) = 0;

	M(0, 2) = sxz - sy;
	M(1, 2) = syz + sx;
	M(2, 2) = zSq + cosA;
	M(3, 2) = 0;

	M(0, 3) = 0;
	M(1, 3) = 0;
	M(2, 3) = 0;
	M(3, 3) = FRACUNIT;
#undef M
}

#endif




float FV_Distancef(const v3float_t *p1, const v3float_t *p2)
{
	float xs = (p2->x-p1->x * p2->x-p1->x);
	float ys = (p2->y-p1->y * p2->y-p1->y);
	float zs = (p2->z-p1->z * p2->z-p1->z);
	return sqrt(xs+ys+zs);
}

// Also returns the magnitude
fixed_t FV_NormalizeO(const v3fixed_t *a_normal, v3fixed_t *a_o)
{
	fixed_t magnitude = FV_Magnitude(a_normal);
	a_o->x = FixedDiv(a_normal->x, magnitude);
	a_o->y = FixedDiv(a_normal->y, magnitude);
	a_o->z = FixedDiv(a_normal->z, magnitude);
	return magnitude;
}

// Also returns the magnitude
float FV_NormalizeOf(const v3float_t *a_normal, v3float_t *a_o)
{
	float magnitude = FV_Magnitudef(a_normal);
	a_o->x = (a_normal->x/magnitude);
	a_o->y = (a_normal->y/magnitude);
	a_o->z = (a_normal->z/magnitude);
	return magnitude;
}

fixed_t FV_Normalize(v3fixed_t *a_normal)
{
	return FV_NormalizeO(a_normal, a_normal);
}

fixed_t FV_Normalizef(v3float_t *a_normal)
{
	return FV_NormalizeOf(a_normal, a_normal);
}

//
// FV_Normalf
//
// Calculates the normal of a polygon.
//
void FV_Normal(const v3fixed_t *a_triangle, v3fixed_t *a_normal)
{
	v3fixed_t a_1;
	v3fixed_t a_2;

	M_MakeVec3(&a_triangle[2], &a_triangle[0], &a_1);
	M_MakeVec3(&a_triangle[1], &a_triangle[0], &a_2);

	M_CrossProduct3(&a_1, &a_2, a_normal);

	FV_NormalizeO(a_normal, a_normal);
}

//
// FV_Normalf
//
// Calculates the normal of a polygon.
//
void FV_Normalf(const v3float_t *a_triangle, v3float_t *a_normal)
{
	v3float_t a_1;
	v3float_t a_2;

	M_MakeVec3f(&a_triangle[2], &a_triangle[0], &a_1);
	M_MakeVec3f(&a_triangle[1], &a_triangle[0], &a_2);

	M_CrossProduct3f(&a_1, &a_2, a_normal);

	FV_NormalizeOf(a_normal, a_normal);
}


// EOF
#endif // #ifdef ESLOPE

