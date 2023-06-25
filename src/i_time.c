// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_time.c
/// \brief Timing for the system layer.

#include "i_time.h"

#include <math.h>

#include "command.h"
#include "doomtype.h"
#include "d_netcmd.h"
#include "m_fixed.h"
#include "i_system.h"

timestate_t g_time;

static CV_PossibleValue_t timescale_cons_t[] = {{FRACUNIT/20, "MIN"}, {20*FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_timescale = CVAR_INIT ("timescale", "1.0", CV_NETVAR|CV_CHEAT|CV_FLOAT, timescale_cons_t, NULL);

static precise_t enterprecise, oldenterprecise;
static fixed_t entertic, oldentertics;
static double tictimer;

// A little more than the minimum sleep duration on Windows.
// May be incorrect for other platforms, but we don't currently have a way to
// query the scheduler granularity. SDL will do what's needed to make this as
// low as possible though.
#define MIN_SLEEP_DURATION_MS 2.1

tic_t I_GetTime(void)
{
	return g_time.time;
}

void I_InitializeTime(void)
{
	CV_RegisterVar(&cv_timescale);

	// I_StartupTimer is preserved for potential subsystems that need to setup
	// timing information for I_GetPreciseTime and sleeping
	I_StartupTimer();

	g_time.time = 0;
	g_time.timefrac = 0;

	enterprecise = I_GetPreciseTime();
	oldenterprecise = enterprecise;
	entertic = 0;
	oldentertics = 0;
	tictimer = 0.0;
}

void I_UpdateTime(fixed_t timescale)
{
	double ticratescaled;
	double elapsedseconds;
	tic_t realtics;

	// get real tics
	ticratescaled = (double)TICRATE * FIXED_TO_FLOAT(timescale);

	enterprecise = I_GetPreciseTime();
	elapsedseconds = (double)(enterprecise - oldenterprecise) / I_GetPrecisePrecision();
	tictimer += elapsedseconds;
	while (tictimer > 1.0/ticratescaled)
	{
		entertic += 1;
		tictimer -= 1.0/ticratescaled;
	}
	realtics = entertic - oldentertics;
	oldentertics = entertic;
	oldenterprecise = enterprecise;

	// Update global time state
	g_time.time += realtics;
	{
		double fractional, integral;
		fractional = modf(tictimer * ticratescaled, &integral);
		g_time.timefrac = FLOAT_TO_FIXED(fractional);
	}
}

void I_SleepDuration(precise_t duration)
{
	UINT64 precision = I_GetPrecisePrecision();
	INT32 sleepvalue = cv_sleep.value;
	UINT64 delaygranularity;
	precise_t cur;
	precise_t dest;

	{
		double gran = round(((double)(precision / 1000) * sleepvalue * MIN_SLEEP_DURATION_MS));
		delaygranularity = (UINT64)gran;
	}

	cur = I_GetPreciseTime();
	dest = cur + duration;

	// the reason this is not dest > cur is because the precise counter may wrap
	// two's complement arithmetic is our friend here, though!
	// e.g. cur 0xFFFFFFFFFFFFFFFE = -2, dest 0x0000000000000001 = 1
	// 0x0000000000000001 - 0xFFFFFFFFFFFFFFFE = 3
	while ((INT64)(dest - cur) > 0)
	{
		// If our cv_sleep value exceeds the remaining sleep duration, use the
		// hard sleep function.
		if (sleepvalue > 0 && (dest - cur) > delaygranularity)
		{
			I_Sleep(sleepvalue);
		}

		// Otherwise, this is a spinloop.

		cur = I_GetPreciseTime();
	}
}
