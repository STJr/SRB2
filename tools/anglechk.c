/*
 *      anglechk.c
 *
 *      Copyright 2007 Alam Arias <Alam.GBC@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#ifdef _MSC_VER
#include <assert.h>
#endif
#define NOASM
#include "../src/tables.h"
#define NO_M
#include "../src/m_fixed.c"
#define FIXEDPOINTCONV

// With angle_t,
// 360 deg = 2^32
// 45 deg = 2^29
// 1 deg = 2^29 / 45
// To convert an angle to a fixed-point number of degrees, then, use
//   fixed = angle * FRACUNIT / ((1<<29) / 45)
// But, done literally like that, this will overflow.
// It's mathematically equivalent to
//   fixed = angle * (1<<FRACBITS) / (1<<29) * 45
//   fixed = 45 * angle * (1<<(FRACBITS-29))
//   fixed = 45 * angle * (1>>(29-FRACBITS))
//   fixed = (angle>>(29-FRACBITS)) * 45

#define ANGLE_TO_FIXED(a) (fixed_t)(((a)>>(29-FRACBITS))*45)
#define FIXED_TO_ANGLE(x) (angle_t)(((x)/45)<<(29-FRACBITS))

/*
Old code that failed if FRACBITS was not 16.
In particular, the use of FRACUNIT in the definition of ANGLE_F is completely
wrong. The value wanted actually happens to be 65536 due to the definition
of angle_t (it's specified so that 360 degrees = 2^32, to take advantage of
modular arithmetic). That 65536 has nothing whatsoever to do with the setting
of FRACUNIT.

#define ANGF_D 8192
#define ANGF_N 45
#define ANGLE_F (ANGF_N*FRACUNIT/ANGF_D)
#define FIXED_TO_ANGLE(x) (angle_t)FixedDiv(x, ANGLE_F) // angle_t = FixedDiv(fixed_t, ANGLE_F)
#define ANGLE_TO_FIXED(x) FixedMul((fixed_t)(x), ANGLE_F) // fixed_t = FixedMul(angle_t, ANGLE_F)
*/

static fixed_t AngleFixed204(angle_t af)
{
	const fixed_t cfn = 180*FRACUNIT;
	if (af == 0)
		return 0;
	else if (af > ANGLE_180)
		return cfn + ANGLE_TO_FIXED(af - ANGLE_180);
	else if (af < ANGLE_180)
		return ANGLE_TO_FIXED(af);
	else return cfn;
}

static inline angle_t FixedAngleC204(fixed_t fa, fixed_t factor)
{
#if 0 //FixedMod off?
	const boolean neqda = da < 0, neqfa = fa < 0;
	const fixed_t afactor = abs(factor);
	angle_t ra = ANGLE_180;

	if (factor == 0)
		return FixedAngle204(fa);
	else if (fa == 0)
		return 0;

	if (neqfactor)
	{
		const fixed_t maf = FixedDiv(afactor, ANGLE_F);
		const fixed_t cfn = FixedMul(360*FRACUNIT, afactor), hcfn = (cfn/2);
		const fixed_t fam = FixedMod(fa, cfn);

		if (fam > hcfn)
			ra = ANGLE_180 + (angle_t)FixedMul(fam - hcfn, maf);
		else if (fam < hcfn)
			ra = (angle_t)FixedMul(fam, maf);
	}
	else
	{
		const fixed_t maf = FixedMul(afactor, ANGLE_F);
		const fixed_t cfn = FixedDiv(360*FRACUNIT, afactor), hcfn = (cfn/2);
		const fixed_t fam = FixedMod(fa, cfn);

		if (fam > hcfn)
			ra = ANGLE_180 + (angle_t)FixedDiv(fam - hcfn, maf);
		else if (fam < hcfn)
			ra = (angle_t)FixedDiv(fam, maf);
	}

	if (neqfa)
		ra = ANGLE_MAX-ra;

	return ra;
#else
	if (factor == 0)
		return FixedAngle(fa);
	else if (factor > 0)
		return (angle_t)((FIXED_TO_FLOAT(fa)/FIXED_TO_FLOAT(factor))*(ANGLE_45/45));
	else
		return (angle_t)((FIXED_TO_FLOAT(fa)*FIXED_TO_FLOAT(-factor))*(ANGLE_45/45));
#endif
}

angle_t FixedAngle(fixed_t fa)
{
#if 0 //FixedMod off?
	const boolean neqfa = fa < 0;
	const fixed_t cfn = 180*FRACUNIT;
	const fixed_t fam = FixedMod(fa, 360*FRACUNIT);
	angle_t ra = ANGLE_180;

	if (fa == 0)
		return 0;

	if (fam > cfn)
		ra = ANGLE_180+FIXED_TO_ANGLE(fam-cfn);
	else if (fam < cfn)
		ra = FIXED_TO_ANGLE(fam);

	if (neqfa)
		ra = ANGLE_MAX-ra;

	return ra;
#else
	return (angle_t)(FIXED_TO_FLOAT(fa)*(ANGLE_45/45));
#endif
}

fixed_t AngleFixed205(angle_t af)
{
#ifdef FIXEDPOINTCONV
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	fixed_t rf = 0*FRACUNIT;
	//const angle_t adj = 0x2000;

	//if (af < adj) // too small to notice
		//return rf;

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
#else
	const fixed_t cfn = 180*FRACUNIT;
	if (af == 0)
		return 0;
	else if (af > ANGLE_180)
		return cfn + ANGLE_TO_FIXED(af - ANGLE_180);
	else if (af < ANGLE_180)
		return ANGLE_TO_FIXED(af);
	else return cfn;
#endif
}

#ifdef FIXEDPOINTCONV
static FUNCMATH angle_t AngleAdj(const fixed_t fa, const fixed_t wf,
                                 angle_t ra)
{
	const angle_t adj = 0x77;
	const boolean fan = fa < 0;
	const fixed_t sl = FixedDiv(fa, wf*2);
	const fixed_t lb = FixedRem(fa, wf*2);
	const fixed_t lo = (wf*2)-lb;

	if (ra == 0)
	{
		if (lb == 0)
		{
			ra = FixedMul(FRACUNIT/512, sl);
			if (ra > FRACUNIT/64)
				return ANGLE_MAX-ra+1;
			return ra;
		}
		else if (lb > 0)
			return ANGLE_MAX-FixedMul(lo*FRACUNIT, adj)+1;
		else
			return ANGLE_MAX-FixedMul(lo*FRACUNIT, adj)+1;
	}

	if (fan)
		return ANGLE_MAX-ra+1;
	else
		return ra;
}
#endif

angle_t FixedAngleC205(fixed_t fa, fixed_t factor)
{
#ifdef FIXEDPOINTCONV
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	angle_t ra = 0;
	const fixed_t cfa = fa;
	fixed_t cwf = wf;

	if (fa == 0)
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
#else
	if (factor == 0)
		return FixedAngle(fa);

	//fa = FixedMod(fa, 360*FRACUNIT);

	if (factor > 0)
		return (angle_t)((FIXED_TO_FLOAT(fa)/FIXED_TO_FLOAT(factor))*(ANGLE_45/45));
	else
		return (angle_t)((FIXED_TO_FLOAT(fa)*FIXED_TO_FLOAT(-factor))*(ANGLE_45/45));
#endif
}

angle_t FixedAngle205(fixed_t fa)
{
#ifdef FIXEDPOINTCONV
	angle_t wa = ANGLE_180;
	fixed_t wf = 180*FRACUNIT;
	angle_t ra = 0;
	const fixed_t cfa = fa;
	const fixed_t cwf = wf;

	if (fa == 0)
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
#else
	//fa = FixedMod(fa, 360*FRACUNIT);

	if (fa == 0)
		return 0;

	return (angle_t)(FIXED_TO_FLOAT(fa)*(ANGLE_45/45));
#endif
}

int main(int argc, char** argv)
{
	fixed_t f, f204, f205;
	INT64 a;
	angle_t a204, a205;
	fixed_t CF = 40*FRACUNIT;
	int err = 0;
	(void)argc;
	(void)argv;

	err = 0x29; //41

	if (1)
	for (a = 0; a < ANGLE_MAX; a += 0x1)
	{
		f204 = AngleFixed204((angle_t)a);
		f205 = AngleFixed205((angle_t)a);
		if (f204 != f205 && (abs(f204-f205) > err || f204 == 0 || f205 == 0))
		{
			printf("Angle: %u, %d, %d, %d\n", (angle_t)a, f204, f205, f204-f205);
			//err = abs(f204-f205);
		}
	}

	//err = FixedDiv(FRACUNIT, 120*FRACUNIT); // 547
	err = FixedDiv(FRACUNIT,  62*FRACUNIT); //1059

	if (1)
	for (f = FRACUNIT*-720; f < FRACUNIT*720; f += 1)
	{
		a204 = FixedAngle(f);
		a205 = FixedAngle205(f);
		if (a204 != a205 && (abs(a204-a205) > err || a204 == 0 || a205 == 0))
		{
			printf("Fixed: %f, %u, %u, %d\n", FIXED_TO_FLOAT(f), a204, a205, a204-a205);
			err = abs(a204-a205);
		}
	}

	//err = FixedDiv(FRACUNIT, 316*FRACUNIT); //207
	err = FixedDiv(FRACUNIT, 125*FRACUNIT); //526

	if (1)
	for (f = FixedMul(FRACUNIT*-720, CF); f < FixedMul(FRACUNIT*720, CF); f += FRACUNIT/16)
	{
		a204 = FixedAngleC204(f, CF);
		a205 = FixedAngleC205(f, CF);
		if (a204 != a205 && (abs(a204-a205) > err || a204 == 0 || a205 == 0))
		{
			printf("FixedC: %f, %u, %u, %d\n", FIXED_TO_FLOAT(f), a204, a205, a204-a205);
			//err = abs(a204-a205);
		}
	}

	return 0;
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
