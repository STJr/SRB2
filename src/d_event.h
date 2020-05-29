// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_event.h
/// \brief Event handling

#ifndef __D_EVENT__
#define __D_EVENT__

#include "doomtype.h"
#include "g_state.h"

// Input event types.
typedef enum
{
	ev_keydown,
	ev_keyup,
	ev_console,
	ev_mouse,
	ev_joystick,
	ev_mouse2,
	ev_joystick2,
} evtype_t;

// Event structure.
typedef struct
{
	evtype_t type;
	INT32 data1; // keys / mouse/joystick buttons
	INT32 data2; // mouse/joystick x move
	INT32 data3; // mouse/joystick y move
} event_t;

//
// GLOBAL VARIABLES
//
#define MAXEVENTS 128

extern event_t events[MAXEVENTS];
extern INT32 eventhead, eventtail;

#endif
