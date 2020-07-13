// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2013-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2013-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_anigif.h
/// \brief Animated GIF creation movie mode.

#ifndef __M_ANIGIF_H__
#define __M_ANIGIF_H__

#include "doomdef.h"
#include "command.h"
#include "screen.h"

#if NUMSCREENS > 2
#define HAVE_ANIGIF
#endif

#ifdef HAVE_ANIGIF
INT32 GIF_open(const char *filename);
void GIF_frame(void);
INT32 GIF_close(void);
#endif

extern consvar_t cv_gif_optimize, cv_gif_downscale, cv_gif_dynamicdelay, cv_gif_localcolortable;

#endif
