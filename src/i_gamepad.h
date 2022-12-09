// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_gamepad.h
/// \brief Gamepads

#ifndef __I_GAMEPAD_H__
#define __I_GAMEPAD_H__

#include "g_input.h"
#include "p_haptic.h"

// So m_menu knows whether to store cv_usegamepad value or string
#define GAMEPAD_HOTPLUG

// Value range for axes
#define JOYAXISRANGE INT16_MAX
#define OLDJOYAXISRANGE 1023

// Starts all gamepads
void I_InitGamepads(void);

// Returns the number of gamepads on the system
INT32 I_NumGamepads(void);

// Changes a gamepad's device
void I_ChangeGamepad(UINT8 which);

// Toggles a gamepad's digital axis setting
void I_SetGamepadDigital(UINT8 which, boolean enable);

// Shuts down all gamepads
void I_ShutdownGamepads(void);

// Returns the name of a gamepad from its index
const char *I_GetGamepadName(INT32 joyindex);

// Gamepad rumble interface
boolean I_RumbleSupported(void);
boolean I_RumbleGamepad(UINT8 which, const haptic_t *effect);

boolean I_GetGamepadRumblePaused(UINT8 which);

boolean I_SetGamepadLargeMotorFreq(UINT8 which, fixed_t freq);
boolean I_SetGamepadSmallMotorFreq(UINT8 which, fixed_t freq);
void I_SetGamepadRumblePaused(UINT8 which, boolean pause);

boolean I_GetGamepadRumbleSupported(UINT8 which);

void I_StopGamepadRumble(UINT8 which);

#endif // __I_GAMEPAD_H__
