// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  commands.h
/// \brief Various netgame commands, such as kick and ban

#ifndef __COMMANDS__
#define __COMMANDS__

#include "../doomdef.h"

#define MAX_REASONLENGTH 30

void Ban_Add(const char *reason);
void D_SaveBan(void);
void Ban_Load_File(boolean warning);
void Command_ShowBan(void);
void Command_ClearBans(void);
void Command_Ban(void);
void Command_BanIP(void);
void Command_ReloadBan(void);
void Command_Kick(void);
void Command_connect(void);
void Command_GetPlayerNum(void);
void Command_Nodes(void);

#endif
