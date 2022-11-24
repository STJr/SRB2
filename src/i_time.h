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
/// \file  i_time.h
/// \brief Timing for the system layer.

#ifndef __I_TIME_H__
#define __I_TIME_H__

#include "command.h"
#include "doomtype.h"
#include "m_fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timestate_s {
	tic_t time;
	fixed_t timefrac;
} timestate_t;

extern timestate_t g_time;
extern consvar_t cv_timescale;

/**	\brief  Called by D_SRB2Loop, returns current time in game tics.
*/
tic_t I_GetTime(void);

/**	\brief  Initializes timing system.
*/
void I_InitializeTime(void);

void I_UpdateTime(fixed_t timescale);

/** \brief  Block for at minimum the duration specified. This function makes a
            best effort not to oversleep, and will spinloop if sleeping would
			take too long. However, callers should still check the current time
			after this returns.
*/
void I_SleepDuration(precise_t duration);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __I_TIME_H__
