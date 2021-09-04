// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2021 by Sonic Team Junior.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief System specific interface stuff.

#ifndef __sdlmain__
#define __sdlmain__

extern SDL_bool consolevent;
extern SDL_bool framebuffer;

#include "../m_fixed.h"

// SDL2 stub macro
#ifdef _MSC_VER
#define SDL2STUB() CONS_Printf("SDL2: stubbed: %s:%d\n", __FUNCTION__, __LINE__)
#else
#define SDL2STUB() CONS_Printf("SDL2: stubbed: %s:%d\n", __func__, __LINE__)
#endif

// So m_menu knows whether to store cv_usejoystick value or string
#define JOYSTICK_HOTPLUG

/**	\brief	The JoyInfo_s struct

  info about joystick
*/
typedef struct SDLJoyInfo_s
{
	/// Joystick handle
	SDL_Joystick *dev;
	/// number of old joystick
	int oldjoy;
	/// number of axies
	int axises;
	/// scale of axises
	INT32 scale;
	/// number of buttons
	int buttons;
	/// number of hats
	int hats;
	/// number of balls
	int balls;

} SDLJoyInfo_t;

/**	\brief SDL info about joystick 1
*/
extern SDLJoyInfo_t JoyInfo;

/**	\brief joystick axis deadzone
*/
#define SDL_JDEADZONE 153
#undef SDL_JDEADZONE

/**	\brief SDL inof about joystick 2
*/
extern SDLJoyInfo_t JoyInfo2;

// So we can call this from i_video event loop
void I_ShutdownJoystick(void);
void I_ShutdownJoystick2(void);

// Cheat to get the device index for a joystick handle
INT32 I_GetJoystickDeviceIndex(SDL_Joystick *dev);

void I_GetConsoleEvents(void);

void SDLforceUngrabMouse(void);

// Needed for some WIN32 functions
extern SDL_Window *window;

#endif
