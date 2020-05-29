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
/// \file  r_sky.h
/// \brief Sky rendering

#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"

#ifdef __GNUG__
#pragma interface
#endif

/// \brief SKY, store the number for name.
#define SKYFLATNAME "F_SKY1"

/// \brief The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT 22

extern INT32 skytexture, skytexturemid;
extern fixed_t skyscale;

extern INT32 skyflatnum;
extern INT32 levelskynum;
extern INT32 globallevelskynum;

// call after skytexture is set to adapt for old/new skies
void R_SetupSkyDraw(void);

void R_SetSkyScale(void);

#endif
