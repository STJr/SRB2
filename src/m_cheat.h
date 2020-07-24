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
/// \file  m_cheat.h
/// \brief Cheat code checking

#ifndef __M_CHEAT__
#define __M_CHEAT__

#include "d_event.h"
#include "d_player.h"
#include "p_mobj.h"
#include "command.h"

boolean cht_Responder(event_t *ev);
void cht_Init(void);

//
// ObjectPlace
//
void Command_ObjectPlace_f(void);
void Command_Writethings_f(void);

extern consvar_t cv_opflags, cv_ophoopflags, cv_mapthingnum, cv_speed;
//extern consvar_t cv_snapto, cv_grid;

extern boolean objectplacing;
extern mobjtype_t op_currentthing;
extern UINT16 op_currentdoomednum;
extern UINT32 op_displayflags;

boolean OP_FreezeObjectplace(void);
void OP_ResetObjectplace(void);
void OP_NightsObjectplace(player_t *player);
void OP_ObjectplaceMovement(player_t *player);

//
// Other cheats
//
void Command_CheatNoClip_f(void);
void Command_CheatGod_f(void);
void Command_CheatNoTarget_f(void);
void Command_Savecheckpoint_f(void);
void Command_Getallemeralds_f(void);
void Command_Resetemeralds_f(void);
void Command_Setrings_f(void);
void Command_Setlives_f(void);
void Command_Setcontinues_f(void);
void Command_Devmode_f(void);
void Command_Scale_f(void);
void Command_Gravflip_f(void);
void Command_Hurtme_f(void);
void Command_JumpToAxis_f(void);
void Command_Charability_f(void);
void Command_Charspeed_f(void);
void Command_Teleport_f(void);
void Command_RTeleport_f(void);
void Command_Skynum_f(void);
void Command_Weather_f(void);
void Command_Toggletwod_f(void);
#ifdef _DEBUG
void Command_CauseCfail_f(void);
#endif
#ifdef LUA_ALLOW_BYTECODE
void Command_Dumplua_f(void);
#endif

#endif
