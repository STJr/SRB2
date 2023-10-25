// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  server_connection.h
/// \brief Server-side part of connection handling

#ifndef __D_SERVER_CONNECTION__
#define __D_SERVER_CONNECTION__

#include "../command.h"
#include "../doomdef.h"
#include "../doomtype.h"

void PT_ClientJoin(SINT8 node);
void PT_AskInfoViaMS(SINT8 node);
void PT_TellFilesNeeded(SINT8 node);
void PT_AskInfo(SINT8 node);

extern tic_t jointimeout;
extern tic_t joindelay;
extern char playeraddress[MAXPLAYERS][64];
extern consvar_t cv_showjoinaddress, cv_allownewplayer, cv_maxplayers, cv_joindelay, cv_rejointimeout;

#endif
