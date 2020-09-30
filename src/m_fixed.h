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
/// \file  m_fixed.h
/// \brief Fixed point arithmetics implementation
///        Fixed point, 32bit as 16.16.

#ifndef __M_FIXED__
#define __M_FIXED__

#include "doomtype.h"
#ifdef __GNUC__
#include <stdlib.h>
#endif

/*!
  \brief bits of the fraction
*/
#define FRACBITS 16
/*!
  \brief units of the fraction
*/
#define FRACUNIT (1<<FRACBITS)
#define FRACMASK (FRACUNIT -1)
/**	\brief	Redefinition of INT32 as fixed_t
	unit used as fixed_t
*/

typedef INT32 fixed_t;

/*!
  \brief convert fixed_t into floating number
*/

FUNCMATH FUNCINLINE static ATTRINLINE float FixedToFloat(fixed_t x)
{
	return x / (float)FRACUNIT;
}

FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FloatToFixed(float f)
{
	return (fixed_t)(f * FRACUNIT);
}

// for backwards compat
#define FIXED_TO_FLOAT(x) FixedToFloat(x) // (((float)(x)) / ((float)FRACUNIT))
#define FLOAT_TO_FIXED(f) FloatToFixed(f) // (fixed_t)((f) * ((float)FRACUNIT))


#if defined (__WATCOMC__) && FRACBITS == 16
	#pragma aux FixedMul =  \
		"imul ebx",         \
		"shrd eax,edx,16"   \
		parm    [eax] [ebx] \
		value   [eax]       \
		modify exact [eax edx]

	#pragma aux FixedDiv2 = \
		"cdq",              \
		"shld edx,eax,16",  \
		"sal eax,16",       \
		"idiv ebx"          \
		parm    [eax] [ebx] \
		value   [eax]       \
		modify exact [eax edx]
#elif defined (__GNUC__) && defined (__i386__) && !defined (NOASM)
	// DJGPP, i386 linux, cygwin or mingw
	FUNCMATH FUNCINLINE static inline fixed_t FixedMul(fixed_t a, fixed_t b) // asm
	{
		fixed_t ret;
		asm
		(
			 "imull %2;"           // a*b
			 "shrdl %3,%%edx,%0;"  // shift logical right FRACBITS bits
			:"=a" (ret)            // eax is always the result and the first operand (%0,%1)
			:"0" (a), "r" (b)      // and %2 is what we use imull on with what in %1
			, "I" (FRACBITS)       // %3 holds FRACBITS (normally 16)
			:"cc", "%edx"         // edx and condition codes clobbered
		);
		return ret;
	}

	FUNCMATH FUNCINLINE static inline fixed_t FixedDiv2(fixed_t a, fixed_t b)
	{
		fixed_t ret;
		asm
		(
			  "movl  %1,%%edx;"    // these two instructions allow the next two to pair, on the Pentium processor.
			  "sarl  $31,%%edx;"   // shift arithmetic right 31 on EDX
			  "shldl %3,%1,%%edx;" // DP shift logical left FRACBITS on EDX
			  "sall  %3,%0;"       // shift arithmetic left FRACBITS on EAX
			  "idivl %2;"          // EDX/b = EAX
			: "=a" (ret)
			: "0" (a), "r" (b)
			, "I" (FRACBITS)
			: "%edx"
		);
		return ret;
	}
#elif defined (__GNUC__) && defined (__arm__) && !defined(__thumb__) && !defined(NOASM) //ARMv4 ASM
	FUNCMATH FUNCINLINE static inline fixed_t FixedMul(fixed_t a, fixed_t b) // let abuse smull
	{
		fixed_t ret;
		asm
		(
			  "smull %[lo], r1, %[a], %[b];"
			  "mov %[lo], %[lo], lsr %3;"
			  "orr %[lo], %[lo], r1, lsl %3;"
			: [lo] "=&r" (ret) // rhi, rlo and rm must be distinct registers
			: [a] "r" (a), [b] "r" (b)
			, "i" (FRACBITS)
			: "r1"
		);
		return ret;
	}

	#define __USE_C_FIXEDDIV__ // no double or asm div in ARM land
#elif defined (__GNUC__) && defined (__ppc__) && !defined(NOASM) && 0 // WII: PPC CPU
	FUNCMATH FUNCINLINE static inline fixed_t FixedMul(fixed_t a, fixed_t b) // asm
	{
		fixed_t ret, hi, lo;
		asm
		(
			  "mullw %0, %2, %3;"
			  "mulhw %1, %2, %3"
			: "=r" (hi), "=r" (lo)
			: "r" (a), "r" (b)
			, "I" (FRACBITS)
		);
		ret = (INT64)((hi>>FRACBITS)+lo)<<FRACBITS;
		return ret;
	}

	#define __USE_C_FIXEDDIV__// Alam: I am lazy
#elif defined (__GNUC__) && defined (__mips__) && !defined(NOASM) && 0 // PSP: MIPS CPU
	FUNCMATH FUNCINLINE static inline fixed_t FixedMul(fixed_t a, fixed_t b) // asm
	{
		fixed_t ret;
		asm
		(
			  "mult %3, %4;"    // a*b=h<32+l
			: "=r" (ret), "=l" (a), "=h" (b) //todo: abuse shr opcode
			: "0" (a), "r" (b)
			, "I" (FRACBITS)
			//: "+l", "+h"
		);
		ret = (INT64)((a>>FRACBITS)+b)<<FRACBITS;
		return ret;
	}

	#define __USE_C_FIXEDDIV__ // no 64b asm div in MIPS land
#elif defined (__GNUC__) && defined (__sh__) && 0 // DC: SH4 CPU
#elif defined (__GNUC__) && defined (__m68k__) && 0 // DEAD: Motorola 6800 CPU
#elif defined (_MSC_VER) && defined(USEASM) && FRACBITS == 16
	// Microsoft Visual C++ (no asm inline)
	fixed_t __cdecl FixedMul(fixed_t a, fixed_t b);
	fixed_t __cdecl FixedDiv2(fixed_t a, fixed_t b);
#else
	#define __USE_C_FIXEDMUL__
	#define __USE_C_FIXEDDIV__
#endif

#ifdef __USE_C_FIXEDMUL__
FUNCMATH fixed_t FixedMul(fixed_t a, fixed_t b);
#endif

#ifdef __USE_C_FIXEDDIV__
FUNCMATH fixed_t FixedDiv2(fixed_t a, fixed_t b);
#endif

/**	\brief	The FixedInt function

	\param	a	fixed_t number

	\return	 a/FRACUNIT
*/

FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FixedInt(fixed_t a)
{
	return FixedMul(a, 1);
}

/**	\brief	The FixedDiv function

	\param	a	fixed_t number
	\param	b	fixed_t number

	\return	a/b


*/
FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FixedDiv(fixed_t a, fixed_t b)
{
	if ((abs(a) >> (FRACBITS-2)) >= abs(b))
		return (a^b) < 0 ? INT32_MIN : INT32_MAX;

	return FixedDiv2(a, b);
}

/**	\brief	The FixedSqrt function

	\param	x	fixed_t number

	\return	sqrt(x)


*/
FUNCMATH fixed_t FixedSqrt(fixed_t x);

/**	\brief	The FixedHypot function

	\param	x	fixed_t number
	\param	y	fixed_t number

	\return	sqrt(x*x+y*y)


*/
FUNCMATH fixed_t FixedHypot(fixed_t x, fixed_t y);

/**	\brief	The FixedFloor function

	\param	x	fixed_t number

	\return	floor(x)


*/
FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FixedFloor(fixed_t x)
{
	const fixed_t a = abs(x); //absolute of x
	const fixed_t i = (a>>FRACBITS)<<FRACBITS; // cut out the fractional part
	const fixed_t f = a-i; // cut out the integral part
	if (f == 0)
		return x;
	if (x != INT32_MIN)
	{ // return rounded down to nearest whole number
		if (x > 0)
			return x-f;
		else
			return x-(FRACUNIT-f);
	}
	return INT32_MIN;
}

/**	\brief	The FixedTrunc function

	\param	x	fixed_t number

	\return trunc(x)


*/
FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FixedTrunc(fixed_t x)
{
	const fixed_t a = abs(x); //absolute of x
	const fixed_t i = (a>>FRACBITS)<<FRACBITS; // cut out the fractional part
	const fixed_t f = a-i; // cut out the integral part
	if (x != INT32_MIN)
	{ // return rounded to nearest whole number, towards zero
		if (x > 0)
			return x-f;
		else
			return x+f;
	}
	return INT32_MIN;
}

/**	\brief	The FixedCeil function

	\param	x	fixed_t number

	\return	ceil(x)


*/
FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FixedCeil(fixed_t x)
{
	const fixed_t a = abs(x); //absolute of x
	const fixed_t i = (a>>FRACBITS)<<FRACBITS; // cut out the fractional part
	const fixed_t f = a-i; // cut out the integral part
	if (f == 0)
		return x;
	if (x == INT32_MIN)
		return INT32_MIN;
	else if (x < FixedFloor(INT32_MAX))
	{ // return rounded up to nearest whole number
		if (x > 0)
			return x+(FRACUNIT-f);
		else
			return x+f;
	}
	return INT32_MAX;
}

/**	\brief	The FixedRound function

	\param	x	fixed_t number

	\return	round(x)


*/
FUNCMATH FUNCINLINE static ATTRINLINE fixed_t FixedRound(fixed_t x)
{
	const fixed_t a = abs(x); //absolute of x
	const fixed_t i = (a>>FRACBITS)<<FRACBITS; // cut out the fractional part
	const fixed_t f = a-i; // cut out the integral part
	if (f == 0)
		return x;
	if (x == INT32_MIN)
		return INT32_MIN;
	else if (x < FixedFloor(INT32_MAX))
	{ // return rounded to nearest whole number, away from zero
		if (x > 0)
			return x+(FRACUNIT-f);
		else
			return x-(FRACUNIT-f);
	}
	return INT32_MAX;
}

typedef struct
{
	fixed_t x;
	fixed_t y;
} vector2_t;

vector2_t *FV2_Load(vector2_t *vec, fixed_t x, fixed_t y);
vector2_t *FV2_UnLoad(vector2_t *vec, fixed_t *x, fixed_t *y);
vector2_t *FV2_Copy(vector2_t *a_o, const vector2_t *a_i);
vector2_t *FV2_AddEx(const vector2_t *a_i, const vector2_t *a_c, vector2_t *a_o);
vector2_t *FV2_Add(vector2_t *a_i, const vector2_t *a_c);
vector2_t *FV2_SubEx(const vector2_t *a_i, const vector2_t *a_c, vector2_t *a_o);
vector2_t *FV2_Sub(vector2_t *a_i, const vector2_t *a_c);
vector2_t *FV2_MulEx(const vector2_t *a_i, fixed_t a_c, vector2_t *a_o);
vector2_t *FV2_Mul(vector2_t *a_i, fixed_t a_c);
vector2_t *FV2_DivideEx(const vector2_t *a_i, fixed_t a_c, vector2_t *a_o);
vector2_t *FV2_Divide(vector2_t *a_i, fixed_t a_c);
vector2_t *FV2_Midpoint(const vector2_t *a_1, const vector2_t *a_2, vector2_t *a_o);
fixed_t FV2_Distance(const vector2_t *p1, const vector2_t *p2);
fixed_t FV2_Magnitude(const vector2_t *a_normal);
fixed_t FV2_NormalizeEx(const vector2_t *a_normal, vector2_t *a_o);
fixed_t FV2_Normalize(vector2_t *a_normal);
vector2_t *FV2_NegateEx(const vector2_t *a_1, vector2_t *a_o);
vector2_t *FV2_Negate(vector2_t *a_1);
boolean FV2_Equal(const vector2_t *a_1, const vector2_t *a_2);
fixed_t FV2_Dot(const vector2_t *a_1, const vector2_t *a_2);
vector2_t *FV2_Point2Vec (const vector2_t *point1, const vector2_t *point2, vector2_t *a_o);

typedef struct
{
	fixed_t x, y, z;
} vector3_t;

vector3_t *FV3_Load(vector3_t *vec, fixed_t x, fixed_t y, fixed_t z);
vector3_t *FV3_UnLoad(vector3_t *vec, fixed_t *x, fixed_t *y, fixed_t *z);
vector3_t *FV3_Copy(vector3_t *a_o, const vector3_t *a_i);
vector3_t *FV3_AddEx(const vector3_t *a_i, const vector3_t *a_c, vector3_t *a_o);
vector3_t *FV3_Add(vector3_t *a_i, const vector3_t *a_c);
vector3_t *FV3_SubEx(const vector3_t *a_i, const vector3_t *a_c, vector3_t *a_o);
vector3_t *FV3_Sub(vector3_t *a_i, const vector3_t *a_c);
vector3_t *FV3_MulEx(const vector3_t *a_i, fixed_t a_c, vector3_t *a_o);
vector3_t *FV3_Mul(vector3_t *a_i, fixed_t a_c);
vector3_t *FV3_DivideEx(const vector3_t *a_i, fixed_t a_c, vector3_t *a_o);
vector3_t *FV3_Divide(vector3_t *a_i, fixed_t a_c);
vector3_t *FV3_Midpoint(const vector3_t *a_1, const vector3_t *a_2, vector3_t *a_o);
fixed_t FV3_Distance(const vector3_t *p1, const vector3_t *p2);
fixed_t FV3_Magnitude(const vector3_t *a_normal);
fixed_t FV3_NormalizeEx(const vector3_t *a_normal, vector3_t *a_o);
fixed_t FV3_Normalize(vector3_t *a_normal);
vector3_t *FV3_NegateEx(const vector3_t *a_1, vector3_t *a_o);
vector3_t *FV3_Negate(vector3_t *a_1);
boolean FV3_Equal(const vector3_t *a_1, const vector3_t *a_2);
fixed_t FV3_Dot(const vector3_t *a_1, const vector3_t *a_2);
vector3_t *FV3_Cross(const vector3_t *a_1, const vector3_t *a_2, vector3_t *a_o);
vector3_t *FV3_ClosestPointOnLine(const vector3_t *Line, const vector3_t *p, vector3_t *out);
void FV3_ClosestPointOnVector(const vector3_t *dir, const vector3_t *p, vector3_t *out);
void FV3_ClosestPointOnTriangle(const vector3_t *tri, const vector3_t *point, vector3_t *result);
vector3_t *FV3_Point2Vec(const vector3_t *point1, const vector3_t *point2, vector3_t *a_o);
fixed_t FV3_Normal(const vector3_t *a_triangle, vector3_t *a_normal);
fixed_t FV3_Strength(const vector3_t *a_1, const vector3_t *dir);
fixed_t FV3_PlaneDistance(const vector3_t *a_normal, const vector3_t *a_point);
boolean FV3_IntersectedPlane(const vector3_t *a_triangle, const vector3_t *a_line, vector3_t *a_normal, fixed_t *originDistance);
fixed_t FV3_PlaneIntersection(const vector3_t *pOrigin, const vector3_t *pNormal, const vector3_t *rOrigin, const vector3_t *rVector);
fixed_t FV3_IntersectRaySphere(const vector3_t *rO, const vector3_t *rV, const vector3_t *sO, fixed_t sR);
vector3_t *FV3_IntersectionPoint(const vector3_t *vNormal, const vector3_t *vLine, fixed_t distance, vector3_t *ReturnVec);
UINT8 FV3_PointOnLineSide(const vector3_t *point, const vector3_t *line);
boolean FV3_PointInsideBox(const vector3_t *point, const vector3_t *box);

typedef struct
{
	fixed_t m[16];
} matrix_t;

void FM_LoadIdentity(matrix_t* matrix);
void FM_CreateObjectMatrix(matrix_t *matrix, fixed_t x, fixed_t y, fixed_t z, fixed_t anglex, fixed_t angley, fixed_t anglez, fixed_t upx, fixed_t upy, fixed_t upz, fixed_t radius);
void FM_MultMatrixVec3(const matrix_t *matrix, const vector3_t *vec, vector3_t *out);
void FM_MultMatrix(matrix_t *dest, const matrix_t *multme);
void FM_Translate(matrix_t *dest, fixed_t x, fixed_t y, fixed_t z);
void FM_Scale(matrix_t *dest, fixed_t x, fixed_t y, fixed_t z);

#endif //m_fixed.h
