// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2007-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  b_bot.h
/// \brief Basic bot handling

void B_BuildTiccmd(player_t *player, ticcmd_t *cmd);
void B_KeysToTiccmd(mobj_t *mo, ticcmd_t *cmd, boolean forward, boolean backward, boolean left, boolean right, boolean strafeleft, boolean straferight, boolean jump, boolean spin);
boolean B_CheckRespawn(player_t *player);
void B_MoveBlocked(player_t *player);
void B_RespawnBot(INT32 playernum);
void B_HandleFlightIndicator(player_t *player);
