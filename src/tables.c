// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  tables.c
/// \brief Lookup tables
///        Do not try to look them up :-).

// In the order of appearance:

// fixed_t finetangent[4096]   - Tangents LUT.
//  Should work with BAM fairly well (12 of 16bit, effectively, by shifting).

// fixed_t finesine[10240]     - Sine lookup.
//  Guess what, serves as cosine, too.
//  Remarkable thing is, how to use BAMs with this?

// fixed_t tantoangle[2049]    - ArcTan LUT,
//  Maps tan(angle) to angle fast. Gotta search.

#include "tables.h"

unsigned SlopeDiv(unsigned num, unsigned den)
{
	unsigned ans;
	num <<= (FINE_FRACBITS-FRACBITS);
	den <<= (FINE_FRACBITS-FRACBITS);
	if (den < 512)
		return SLOPERANGE;
	ans = (num<<3) / (den>>8);
	return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

UINT64 SlopeDivEx(unsigned int num, unsigned int den)
{
	UINT64 ans;
	if (den < 512)
		return SLOPERANGE;
	ans = ((UINT64)num<<3)/(den>>8);
	return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

fixed_t AngleFixed(angle_t af)
{
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	fixed_t rf = 0*FRACUNIT;

	while (af)
	{
		while (af < wa)
		{
			wa /= 2;
			wf /= 2;
		}
		rf += wf;
		af -= wa;
	}

	return rf;
}

static FUNCMATH angle_t AngleAdj(const fixed_t fa, const fixed_t wf,
                                 angle_t ra)
{
	const angle_t adj = 0x77;
	const boolean fan = fa < 0;
	const fixed_t sl = FixedDiv(fa, wf*2);
	const fixed_t lb = fa % (wf*2);
	const fixed_t lo = (wf*2)-lb;

	if (ra == 0)
	{
		if (lb == 0)
		{
			ra = FixedMul(FRACUNIT/512, sl);
			if (ra > FRACUNIT/64)
				return InvAngle(ra);
			return ra;
		}
		else if (lb > 0)
			return InvAngle(FixedMul(lo*FRACUNIT, adj));
		else
			return InvAngle(FixedMul(lo*FRACUNIT, adj));
	}

	if (fan)
		return InvAngle(ra);
	else
		return ra;
}

angle_t FixedAngleC(fixed_t fa, fixed_t factor)
{
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	angle_t ra = 0;
	const fixed_t cfa = fa;
	fixed_t cwf = wf;

	if (fa == 0)
		return 0;

	// -2,147,483,648 has no absolute value in a 32 bit signed integer
	// so this code _would_ infinite loop if passed it
	if (fa == INT32_MIN)
		return 0;

	if (factor == 0)
		return FixedAngle(fa);
	else if (factor > 0)
		cwf = wf = FixedMul(wf, factor);
	else if (factor < 0)
		cwf = wf = FixedDiv(wf, -factor);

	fa = abs(fa);

	while (fa)
	{
		while (fa < wf)
		{
			wa /= 2;
			wf /= 2;
		}
		ra = ra + wa;
		fa = fa - wf;
	}

	return AngleAdj(cfa, cwf, ra);
}

angle_t FixedAngle(fixed_t fa)
{
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	angle_t ra = 0;
	const fixed_t cfa = fa;
	const fixed_t cwf = wf;

	if (fa == 0)
		return 0;

	// -2,147,483,648 has no absolute value in a 32 bit signed integer
	// so this code _would_ infinite loop if passed it
	if (fa == INT32_MIN)
		return 0;

	fa = abs(fa);

	while (fa)
	{
		while (fa < wf)
		{
			wa /= 2;
			wf /= 2;
		}
		ra = ra + wa;
		fa = fa - wf;
	}

	return AngleAdj(cfa, cwf, ra);
}


#include "t_ftan.c"

#include "t_fsin.c"
fixed_t *finecosine = &finesine[FINEANGLES/4];

#include "t_tan2a.c"

#include "t_facon.c"


FUNCMATH angle_t FixedAcos(fixed_t x)
{
	if (-FRACUNIT > x || x >= FRACUNIT) return 0;
	return fineacon[((x<<(FINE_FRACBITS-FRACBITS)))+FRACUNIT];
}

//
// AngleBetweenVectors
//
// This checks to see if a point is inside the ranges of a polygon
//
angle_t FV2_AngleBetweenVectors(const vector2_t *Vector1, const vector2_t *Vector2)
{
	// Remember, above we said that the Dot Product of returns the cosine of the angle
	// between 2 vectors?  Well, that is assuming they are unit vectors (normalize vectors).
	// So, if we don't have a unit vector, then instead of just saying  arcCos(DotProduct(A, B))
	// We need to divide the dot product by the magnitude of the 2 vectors multiplied by each other.
	// Here is the equation:   arc cosine of (V . W / || V || * || W || )
	// the || V || means the magnitude of V.  This then cancels out the magnitudes dot product magnitudes.
	// But basically, if you have normalize vectors already, you can forget about the magnitude part.

	// Get the dot product of the vectors
	fixed_t dotProduct = FV2_Dot(Vector1, Vector2);

	// Get the product of both of the vectors magnitudes
	fixed_t vectorsMagnitude = FixedMul(FV2_Magnitude(Vector1), FV2_Magnitude(Vector2));

	// Return the arc cosine of the (dotProduct / vectorsMagnitude) which is the angle in RADIANS.
	return FixedAcos(FixedDiv(dotProduct, vectorsMagnitude));
}

angle_t FV3_AngleBetweenVectors(const vector3_t *Vector1, const vector3_t *Vector2)
{
	// Remember, above we said that the Dot Product of returns the cosine of the angle
	// between 2 vectors?  Well, that is assuming they are unit vectors (normalize vectors).
	// So, if we don't have a unit vector, then instead of just saying  arcCos(DotProduct(A, B))
	// We need to divide the dot product by the magnitude of the 2 vectors multiplied by each other.
	// Here is the equation:   arc cosine of (V . W / || V || * || W || )
	// the || V || means the magnitude of V.  This then cancels out the magnitudes dot product magnitudes.
	// But basically, if you have normalize vectors already, you can forget about the magnitude part.

	// Get the dot product of the vectors
	fixed_t dotProduct = FV3_Dot(Vector1, Vector2);

	// Get the product of both of the vectors magnitudes
	fixed_t vectorsMagnitude = FixedMul(FV3_Magnitude(Vector1), FV3_Magnitude(Vector2));

	// Return the arc cosine of the (dotProduct / vectorsMagnitude) which is the angle in RADIANS.
	return FixedAcos(FixedDiv(dotProduct, vectorsMagnitude));
}

//
// InsidePolygon
//
// This checks to see if a point is inside the ranges of a polygon
//
boolean FV2_InsidePolygon(const vector2_t *vIntersection, const vector2_t *Poly, const INT32 vertexCount)
{
	INT32 i;
	UINT64 Angle = 0;					// Initialize the angle
	vector2_t vA, vB;					// Create temp vectors

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
		FV2_Point2Vec(&Poly[i], vIntersection, &vA);	// Subtract the intersection point from the current vertex
												// Subtract the point from the next vertex
		FV2_Point2Vec(&Poly[(i + 1) % vertexCount], vIntersection, &vB);

		Angle += FV2_AngleBetweenVectors(&vA, &vB);	// Find the angle between the 2 vectors and add them all up as we go along
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

boolean FV3_InsidePolygon(const vector3_t *vIntersection, const vector3_t *Poly, const INT32 vertexCount)
{
	INT32 i;
	UINT64 Angle = 0;					// Initialize the angle
	vector3_t vA, vB;					// Create temp vectors

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
		FV3_Point2Vec(&Poly[i], vIntersection, &vA);	// Subtract the intersection point from the current vertex
												// Subtract the point from the next vertex
		FV3_Point2Vec(&Poly[(i + 1) % vertexCount], vIntersection, &vB);

		Angle += FV3_AngleBetweenVectors(&vA, &vB);	// Find the angle between the 2 vectors and add them all up as we go along
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
boolean FV3_IntersectedPolygon(const vector3_t *vPoly, const vector3_t *vLine, const INT32 vertexCount, vector3_t *collisionPoint)
{
	vector3_t vNormal, vIntersection;
	fixed_t originDistance = 0*FRACUNIT;


	// First we check to see if our line intersected the plane.  If this isn't true
	// there is no need to go on, so return false immediately.
	// We pass in address of vNormal and originDistance so we only calculate it once

	if(!FV3_IntersectedPlane(vPoly, vLine,   &vNormal,   &originDistance))
		return false;

	// Now that we have our normal and distance passed back from IntersectedPlane(),
	// we can use it to calculate the intersection point.  The intersection point
	// is the point that actually is ON the plane.  It is between the line.  We need
	// this point test next, if we are inside the polygon.  To get the I-Point, we
	// give our function the normal of the plane, the points of the line, and the originDistance.

	FV3_IntersectionPoint(&vNormal, vLine, originDistance, &vIntersection);

	// Now that we have the intersection point, we need to test if it's inside the polygon.
	// To do this, we pass in :
	// (our intersection point, the polygon, and the number of vertices our polygon has)

	if(FV3_InsidePolygon(&vIntersection, vPoly, vertexCount))
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

//
// RotateVector
//
// Rotates a vector around another vector
//
void FV3_Rotate(vector3_t *rotVec, const vector3_t *axisVec, const angle_t angle)
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
	fixed_t sa = FINESINE(angle);
	fixed_t ca = FINECOSINE(angle);
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

void FM_Rotate(matrix_t *dest, angle_t angle, fixed_t x, fixed_t y, fixed_t z)
{
#define M(row,col) dest->m[row * 4 + col]
	const fixed_t sinA = FINESINE(angle>>ANGLETOFINESHIFT);
	const fixed_t cosA = FINECOSINE(angle>>ANGLETOFINESHIFT);
	const fixed_t invCosA = FRACUNIT - cosA;
	vector3_t nrm;
	fixed_t xSq, ySq, zSq;
	fixed_t sx, sy, sz;
	fixed_t sxy, sxz, syz;

	nrm.x = x;
	nrm.y = y;
	nrm.z = z;
	FV3_Normalize(&nrm);

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
