// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_time.c
/// \brief Timing for the system layer.

#include "i_time.h"

#include "command.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "i_system.h"

timestate_t g_time;

static CV_PossibleValue_t timescale_cons_t[] = {{FRACUNIT/20, "MIN"}, {20*FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_timescale = CVAR_INIT ("timescale", "1.0", CV_NETVAR|CV_CHEAT|CV_FLOAT, timescale_cons_t, NULL);

static precise_t enterprecise, oldenterprecise;
static fixed_t entertic, oldentertics;
static double tictimer;

tic_t I_GetTime(void)
{
	return g_time.time;
}

void I_InitializeTime(void)
{
	g_time.time = 0;
	g_time.timefrac = 0;
	g_time.realtics = 0;
	g_time.deltaseconds = 0;
	g_time.ticrate = FLOAT_TO_FIXED(TICRATE);

	enterprecise = 0;
	oldenterprecise = 0;
	tictimer = 0.0;

	CV_RegisterVar(&cv_timescale);

	// I_StartupTimer is preserved for potential subsystems that need to setup
	// timing information for I_GetPreciseTime and sleeping
	I_StartupTimer();
}

void I_UpdateTime(fixed_t timescale)
{
	double ticratescaled;
	double elapsedseconds;
	tic_t realtics;

	// get real tics
	ticratescaled = (double)TICRATE * FIXED_TO_FLOAT(timescale);

	enterprecise = I_GetPreciseTime();
	elapsedseconds = I_PreciseElapsedSeconds(oldenterprecise, enterprecise);
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
	g_time.realtics = realtics;
	g_time.deltatics = FLOAT_TO_FIXED(elapsedseconds * ticratescaled);
	g_time.deltaseconds = FLOAT_TO_FIXED(elapsedseconds);
	g_time.ticrate = FLOAT_TO_FIXED(ticratescaled);
}
