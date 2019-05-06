// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2018 by Sonic Team Jr.
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

void I_GetConsoleEvents(void);

void SDLforceUngrabMouse(void);

#endif
