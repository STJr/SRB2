// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2015 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netmobj.h
/// \brief Network handling for MObj.

#ifndef __D_NETMOBJ__
#define __D_NETMOBJ__
/// \todo Fix NONET...

#include "p_mobj.h"

struct net_mobj_s
{
	UINT16 id;

	fixed_t x, y, z;
	fixed_t momx, momy, momz;
	fixed_t radius, height;

	angle_t angle;
	spritenum_t sprite;
	UINT8 sprite2;
	UINT32 frame;

	INT32 tics;
	state_t *state;
};

// Server.
void Net_AddMobj(mobj_t *mo); // Send a new mobj to clients.
void Net_WriteMobj(mobj_t *mo); // Send mobj updates to clients.

// Client.
mobj_t *Net_ReadMobj(void *p); // Spawn a new mobj from a server message.
void Net_UpdateMobj(mobj_t *mo, void *p); // Update mobj from server message.

#endif
