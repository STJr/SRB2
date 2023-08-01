// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023-2023 by Louis-Antoine de Moulins de Rochefort.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  snake.h
/// \brief Snake minigame for the download screen.

#ifndef __SNAKE__
#define __SNAKE__

#include "d_event.h"

void Snake_Allocate(void **opaque);
void Snake_Update(void *opaque);
void Snake_Draw(void *opaque);
void Snake_Free(void **opaque);
boolean Snake_JoyGrabber(void *opaque, event_t *ev);

#endif
