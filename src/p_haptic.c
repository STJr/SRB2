// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021-2022 by Jaime "Lactozilla" Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_haptic.c
/// \brief Haptic feedback

#include "p_haptic.h"
#include "g_game.h"
#include "netcode/d_netcmd.h"
#include "i_gamepad.h"
#include "doomstat.h"

// Helper function: Returns the gamepad index for a player if it's enabled
static INT16 GetGamepadIndex(player_t *player)
{
	INT16 index = G_GetGamepadForPlayer(player);

	if (index >= 0 && cv_usegamepad[index].value)
		return index;

	return -1;
}

// Rumbles a player's gamepad, or all gamepads
boolean P_DoRumble(player_t *player, fixed_t large_magnitude, fixed_t small_magnitude, tic_t duration)
{
	if (!I_RumbleSupported())
		return false;

	// Rumble every gamepad
	if (player == NULL)
	{
		for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
		{
			if (cv_gamepad_rumble[i].value)
				G_RumbleGamepad(i, large_magnitude, small_magnitude, duration);
		}

		return true;
	}

	INT16 which = GetGamepadIndex(player);
	if (which < 0 || !cv_gamepad_rumble[which].value)
		return false;

	return G_RumbleGamepad((UINT8)which, large_magnitude, small_magnitude, duration);
}

// Pauses or unpauses gamepad rumble for a player (or all of them)
// Rumble is paused or unpaused regardless if it's enabled or not
static void SetRumblePaused(player_t *player, boolean pause)
{
	INT16 which = GetGamepadIndex(player);

	if (which >= 0)
		G_SetGamepadRumblePaused((UINT8)which, pause);
	else if (player == NULL)
	{
		// Pause or unpause every gamepad
		for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
			G_SetGamepadRumblePaused(i, pause);
	}
}

void P_PauseRumble(player_t *player)
{
	SetRumblePaused(player, true);
}

void P_UnpauseRumble(player_t *player)
{
	SetRumblePaused(player, false);
}

boolean P_IsRumbleEnabled(player_t *player)
{
	INT16 which = GetGamepadIndex(player);
	if (which < 0 || !cv_gamepad_rumble[which].value)
		return false;

	return G_RumbleSupported((UINT8)which);
}

boolean P_IsRumblePaused(player_t *player)
{
	INT16 which = GetGamepadIndex(player);
	if (which < 0 || !cv_gamepad_rumble[which].value)
		return false;

	return G_GetGamepadRumblePaused((UINT8)which);
}

// Stops gamepad rumble for a player (or all of them)
void P_StopRumble(player_t *player)
{
	if (!I_RumbleSupported())
		return;

	if (player)
	{
		INT16 which = GetGamepadIndex(player);
		if (which >= 0)
			G_StopGamepadRumble((UINT8)which);
		return;
	}

	// Stop every gamepad instead
	for (UINT8 i = 0; i < NUM_GAMEPADS; i++)
		G_StopGamepadRumble(i);
}
