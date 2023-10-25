// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  gamestate.h
/// \brief Gamestate (re)sending

#ifndef __GAMESTATE__
#define __GAMESTATE__

#include "../doomtype.h"

extern UINT8 hu_redownloadinggamestate;
extern boolean cl_redownloadinggamestate;

boolean SV_ResendingSavegameToAnyone(void);
void SV_SendSaveGame(INT32 node, boolean resending);
void SV_SavedGame(void);
void CL_LoadReceivedSavegame(boolean reloading);
void CL_ReloadReceivedSavegame(void);
void Command_ResendGamestate(void);
void PT_CanReceiveGamestate(SINT8 node);
void PT_ReceivedGamestate(SINT8 node);
void PT_WillResendGamestate(SINT8 node);

#endif
