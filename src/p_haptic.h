// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021-2022 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_haptic.h
/// \brief Haptic feedback

#ifndef __P_HAPTIC__
#define __P_HAPTIC__

#include "doomdef.h"
#include "p_local.h"

boolean P_DoRumble(player_t *player, fixed_t large_magnitude, fixed_t small_magnitude, tic_t duration);
void P_PauseRumble(player_t *player);
void P_UnpauseRumble(player_t *player);
boolean P_IsRumbleEnabled(player_t *player);
boolean P_IsRumblePaused(player_t *player);
void P_StopRumble(player_t *player);

#define P_DoRumbleCombined(player, magnitude, dur) P_DoRumble(player, magnitude, magnitude, dur);

#endif // __P_HAPTIC__
