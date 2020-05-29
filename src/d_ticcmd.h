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
/// \file  d_ticcmd.h
/// \brief Button/action code definitions, ticcmd_t

#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "m_fixed.h"
#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

// Button/action code definitions.
typedef enum
{
	// First 4 bits are weapon change info, DO NOT USE!
	BT_WEAPONMASK = 0x0F, //our first four bits.

	BT_WEAPONNEXT = 1<<4,
	BT_WEAPONPREV = 1<<5,

	BT_ATTACK     = 1<<6, // shoot rings
	BT_USE        = 1<<7, // spin
	BT_CAMLEFT    = 1<<8, // turn camera left
	BT_CAMRIGHT   = 1<<9, // turn camera right
	BT_TOSSFLAG   = 1<<10,
	BT_JUMP       = 1<<11,
	BT_FIRENORMAL = 1<<12, // Fire a normal ring no matter what

	BT_CUSTOM1    = 1<<13,
	BT_CUSTOM2    = 1<<14,
	BT_CUSTOM3    = 1<<15,
} buttoncode_t;

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.

// bits in angleturn
#define TICCMD_RECEIVED 1
#define TICCMD_XY 2

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{
	SINT8 forwardmove; // -MAXPLMOVE to MAXPLMOVE (50)
	SINT8 sidemove; // -MAXPLMOVE to MAXPLMOVE (50)
	INT16 angleturn; // <<16 for angle delta - saved as 1 byte into demos
	INT16 aiming; // vertical aiming, see G_BuildTicCmd
	UINT16 buttons;
} ATTRPACK ticcmd_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

#endif
