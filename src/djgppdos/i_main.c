// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
/// \brief Main program, simply calls D_SRB2Main high level loop.

#include "../doomdef.h"

#include "../m_argv.h"
#include "../d_main.h"

#include "../i_system.h"

#ifdef REMOTE_DEBUGGING
#include <i386-stub.h>
#include "rdb.h"
#endif

//### let's try with Allegro ###
#define  alleg_mouse_unused
#define  alleg_timer_unused
#define  ALLEGRO_NO_KEY_DEFINES
#define  alleg_keyboard_unused
#define  alleg_joystick_unused
#define  alleg_gfx_driver_unused
#define  alleg_palette_unused
#define  alleg_graphics_unused
#define  alleg_vidmem_unused
#define  alleg_flic_unused
#define  alleg_sound_unused
#define  alleg_file_unused
#define  alleg_datafile_unused
#define  alleg_math_unused
#define  alleg_gui_unused
#include <allegro.h>
//### end of Allegro include ###

int main (int argc, char **argv)
{
	myargc = argc;
	myargv = argv;

	{
		//added:03-01-98:
		//       Setup signal handlers and other stuff BEFORE ANYTHING ELSE!
		I_StartupSystem();
#ifdef REMOTE_DEBUGGING
		/* Only setup if remote debugging  is to be done, Muhahahaha!*/
		gdb_serial_init(DEBUG_COM_PORT,DEBUG_COM_PORT_SPEED);
		gdb_target_init();
		breakpoint();
#endif

		D_SRB2Main();
		D_SRB2Loop();

	}
	//added:03-01-98:
	//       hmmm... it will never go here.

	return 0;
}
#if ALLEGRO_VERSION == 4
END_OF_MAIN()
#endif
