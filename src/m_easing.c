// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_easing.c
/// \brief Easing functions
///        Referenced from https://easings.net/

#include "m_easing.h"
#include "tables.h"
#include "doomdef.h"

/*
	For the computation of the logarithm, we choose, by trial and error, from among
	a sequence of particular factors those, that when multiplied with the function
	argument, normalize it to unity. For every factor chosen, we add up the
	corresponding logarithm value stored in a table. The sum then corresponds to
	the logarithm of the function argument.

	For the integer portion, we would want to choose
		2^i, i = 1, 2, 4, 8, ...
	and for the factional part we choose
		1+2^-i, i = 1, 2, 3, 4, 5 ...

	The algorithm for the exponential is closely related and quite literally the inverse
	of the logarithm algorithm. From among the sequence of tabulated logarithms for our
	chosen factors, we pick those that when subtracted from the function argument ultimately
	reduce it to zero. Starting with unity, we multiply with all the factors whose logarithms
	we have subtracted in the process. The resulting product corresponds to the result of the exponentiation.

	Logarithms of values greater than unity can be computed by applying the algorithm to the reciprocal
	of the function argument (with the negation of the result as appropriate), likewise exponentiation with
	negative function arguments requires us negate the function argument and compute the reciprocal at the end.
*/

static fixed_t logtabdec[FRACBITS] =
{
	0x95c1, 0x526a, 0x2b80, 0x1663,
	0xb5d, 0x5b9, 0x2e0, 0x170,
	0xb8, 0x5c, 0x2e, 0x17,
	0x0b, 0x06, 0x03, 0x01
};

static fixed_t fixlog2(fixed_t a)
{
	UINT32 x = a, y = 0;
	INT32 t, i, shift = 8;

	if (x > FRACUNIT)
		x = FixedDiv(FRACUNIT, x);

	// Integer part
	//   1<<19 = 0x80000
	//   1<<18 = 0x40000
	//   1<<17 = 0x20000
	//   1<<16 = 0x10000

#define dologtab(i) \
	t = (x << shift); \
	if (t < FRACUNIT) \
	{ \
		x = t; \
		y += (1 << (19 - i)); \
	} \
	shift /= 2;

	dologtab(0)
	dologtab(1)
	dologtab(2)
	dologtab(3)

#undef dologtab

	// Decimal part
	for (i = 0; i < FRACBITS; i++)
	{
		t = x + (x >> (i + 1));
		if (t < FRACUNIT)
		{
			x = t;
			y += logtabdec[i];
		}
	}

	if (a <= FRACUNIT)
		return -y;

	return y;
}

// Notice how this is symmetric to fixlog2.
static INT32 fixexp(fixed_t a)
{
	UINT32 x, y;
	fixed_t t, i, shift = 8;

	// Underflow prevention.
	if (a <= -15 * FRACUNIT)
		return 0;

	x = (a < 0) ? (-a) : (a);
	y = FRACUNIT;

	// Integer part (see fixlog2)
#define dologtab(i) \
	t = x - (1 << (19 - i)); \
	if (t >= 0) \
	{ \
		x = t; \
		y <<= shift; \
	} \
	shift /= 2;

	dologtab(0)
	dologtab(1)
	dologtab(2)
	dologtab(3)

#undef dologtab

	// Decimal part
	for (i = 0; i < FRACBITS; i++)
	{
		t = (x - logtabdec[i]);
		if (t >= 0)
		{
			x = t;
			y += (y >> (i + 1));
		}
	}

	if (a < 0)
		return FixedDiv(FRACUNIT, y);

	return y;
}

#define fixpow(x, y) fixexp(FixedMul((y), fixlog2(x)))
#define fixintmul(x, y) FixedMul((x) * FRACUNIT, y)
#define fixintdiv(x, y) FixedDiv(x, (y) * FRACUNIT)
#define fixinterp(start, end, t) FixedMul((FRACUNIT - (t)), start) + FixedMul(t, end)

// ==================
//  EASING FUNCTIONS
// ==================

#define EASINGFUNC(type) fixed_t Easing_ ## type (fixed_t t, fixed_t start, fixed_t end)

//
// Linear
//

EASINGFUNC(Linear)
{
	return fixinterp(start, end, t);
}

//
// Sine
//

// This is equivalent to calculating (x * pi) and converting the result from radians into degrees.
#define fixang(x) FixedMul((x), 180*FRACUNIT)

EASINGFUNC(InSine)
{
	fixed_t c = fixang(t / 2);
	fixed_t x = FRACUNIT - FINECOSINE(FixedAngle(c)>>ANGLETOFINESHIFT);
	return fixinterp(start, end, x);
}

EASINGFUNC(OutSine)
{
	fixed_t c = fixang(t / 2);
	fixed_t x = FINESINE(FixedAngle(c)>>ANGLETOFINESHIFT);
	return fixinterp(start, end, x);
}

EASINGFUNC(InOutSine)
{
	fixed_t c = fixang(t);
	fixed_t x = -(FINECOSINE(FixedAngle(c)>>ANGLETOFINESHIFT) - FRACUNIT) / 2;
	return fixinterp(start, end, x);
}

#undef fixang

//
// Quad
//

EASINGFUNC(InQuad)
{
	return fixinterp(start, end, FixedMul(t, t));
}

EASINGFUNC(OutQuad)
{
	return fixinterp(start, end, FRACUNIT - FixedMul(FRACUNIT - t, FRACUNIT - t));
}

EASINGFUNC(InOutQuad)
{
	fixed_t x = t < (FRACUNIT/2)
	? fixintmul(2, FixedMul(t, t))
	: FRACUNIT - fixpow(FixedMul(-2*FRACUNIT, t) + 2*FRACUNIT, 2*FRACUNIT) / 2;
	return fixinterp(start, end, x);
}

//
// Cubic
//

EASINGFUNC(InCubic)
{
	fixed_t x = FixedMul(t, FixedMul(t, t));
	return fixinterp(start, end, x);
}

EASINGFUNC(OutCubic)
{
	return fixinterp(start, end, FRACUNIT - fixpow(FRACUNIT - t, 3*FRACUNIT));
}

EASINGFUNC(InOutCubic)
{
	fixed_t x = t < (FRACUNIT/2)
	? fixintmul(4, FixedMul(t, FixedMul(t, t)))
	: FRACUNIT - fixpow(fixintmul(-2, t) + 2*FRACUNIT, 3*FRACUNIT) / 2;
	return fixinterp(start, end, x);
}

//
// "Quart"
//

EASINGFUNC(InQuart)
{
	fixed_t x = FixedMul(FixedMul(t, t), FixedMul(t, t));
	return fixinterp(start, end, x);
}

EASINGFUNC(OutQuart)
{
	fixed_t x = FRACUNIT - fixpow(FRACUNIT - t, 4 * FRACUNIT);
	return fixinterp(start, end, x);
}

EASINGFUNC(InOutQuart)
{
	fixed_t x = t < (FRACUNIT/2)
	? fixintmul(8, FixedMul(FixedMul(t, t), FixedMul(t, t)))
	: FRACUNIT - fixpow(fixintmul(-2, t) + 2*FRACUNIT, 4*FRACUNIT) / 2;
	return fixinterp(start, end, x);
}

//
// "Quint"
//

EASINGFUNC(InQuint)
{
	fixed_t x = FixedMul(t, FixedMul(FixedMul(t, t), FixedMul(t, t)));
	return fixinterp(start, end, x);
}

EASINGFUNC(OutQuint)
{
	fixed_t x = FRACUNIT - fixpow(FRACUNIT - t, 5 * FRACUNIT);
	return fixinterp(start, end, x);
}

EASINGFUNC(InOutQuint)
{
	fixed_t x = t < (FRACUNIT/2)
	? FixedMul(16*FRACUNIT, FixedMul(t, FixedMul(FixedMul(t, t), FixedMul(t, t))))
	: FRACUNIT - fixpow(fixintmul(-2, t) + 2*FRACUNIT, 5*FRACUNIT) / 2;
	return fixinterp(start, end, x);
}

//
// Exponential
//

EASINGFUNC(InExpo)
{
	fixed_t x = (!t) ? 0 : fixpow(2*FRACUNIT, fixintmul(10, t) - 10*FRACUNIT);
	return fixinterp(start, end, x);
}

EASINGFUNC(OutExpo)
{
	fixed_t x = (t >= FRACUNIT) ? FRACUNIT
	: FRACUNIT - fixpow(2*FRACUNIT, fixintmul(-10, t));
	return fixinterp(start, end, x);
}

EASINGFUNC(InOutExpo)
{
	fixed_t x;

	if (!t)
		x = 0;
	else if (t >= FRACUNIT)
		x = FRACUNIT;
	else
	{
		if (t < FRACUNIT / 2)
		{
			x = fixpow(2*FRACUNIT, fixintmul(20, t) - 10*FRACUNIT);
			x = fixintdiv(x, 2);
		}
		else
		{
			x = fixpow(2*FRACUNIT, fixintmul(-20, t) + 10*FRACUNIT);
			x = fixintdiv((2*FRACUNIT) - x, 2);
		}
	}

	return fixinterp(start, end, x);
}

//
// "Back"
//

#define EASEBACKCONST1 111514 // 1.70158
#define EASEBACKCONST2 99942 // 1.525

static fixed_t EaseInBack(fixed_t t, fixed_t start, fixed_t end, fixed_t c1)
{
	const fixed_t c3 = c1 + FRACUNIT;
	fixed_t x = FixedMul(FixedMul(t, t), FixedMul(c3, t) - c1);
	return fixinterp(start, end, x);
}

EASINGFUNC(InBack)
{
	return EaseInBack(t, start, end, EASEBACKCONST1);
}

static fixed_t EaseOutBack(fixed_t t, fixed_t start, fixed_t end, fixed_t c1)
{
	const fixed_t c3 = c1 + FRACUNIT;
	fixed_t x;
	t -= FRACUNIT;
	x = FRACUNIT + FixedMul(FixedMul(t, t), FixedMul(c3, t) + c1);
	return fixinterp(start, end, x);
}

EASINGFUNC(OutBack)
{
	return EaseOutBack(t, start, end, EASEBACKCONST1);
}

static fixed_t EaseInOutBack(fixed_t t, fixed_t start, fixed_t end, fixed_t c2)
{
	fixed_t x, y;
	const fixed_t f2 = 2*FRACUNIT;

	if (t < FRACUNIT / 2)
	{
		x = fixpow(FixedMul(t, f2), f2);
		y = FixedMul(c2 + FRACUNIT, FixedMul(t, f2));
		x = FixedMul(x, y - c2);
	}
	else
	{
		x = fixpow(-(FixedMul(t, f2) - f2), f2);
		y = FixedMul(c2 + FRACUNIT, FixedMul(t, f2) - f2);
		x = FixedMul(x, y + c2);
		x += f2;
	}

	x /= 2;

	return fixinterp(start, end, x);
}

EASINGFUNC(InOutBack)
{
	return EaseInOutBack(t, start, end, EASEBACKCONST2);
}

#undef EASINGFUNC
#define EASINGFUNC(type) fixed_t Easing_ ## type (fixed_t t, fixed_t start, fixed_t end, fixed_t param)

EASINGFUNC(InBackParameterized)
{
	return EaseInBack(t, start, end, param);
}

EASINGFUNC(OutBackParameterized)
{
	return EaseOutBack(t, start, end, param);
}

EASINGFUNC(InOutBackParameterized)
{
	return EaseInOutBack(t, start, end, param);
}

#undef EASINGFUNC

// Function list

#define EASINGFUNC(type) Easing_ ## type
#define COMMA ,

easingfunc_t easing_funclist[EASE_MAX] =
{
	EASINGFUNCLIST(COMMA)
};

// Function names

#undef EASINGFUNC
#define EASINGFUNC(type) #type

const char *easing_funcnames[EASE_MAX] =
{
	EASINGFUNCLIST(COMMA)
};

#undef COMMA
#undef EASINGFUNC
