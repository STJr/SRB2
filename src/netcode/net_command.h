// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  net_command.h
/// \brief Net command handling

#ifndef __D_NET_COMMAND__
#define __D_NET_COMMAND__

#include "d_clisrv.h"
#include "../doomtype.h"

// Must be a power of two
#define TEXTCMD_HASH_SIZE 4

typedef struct textcmdplayer_s
{
	INT32 playernum;
	UINT8 cmd[MAXTEXTCMD];
	struct textcmdplayer_s *next;
} textcmdplayer_t;

typedef struct textcmdtic_s
{
	tic_t tic;
	textcmdplayer_t *playercmds[TEXTCMD_HASH_SIZE];
	struct textcmdtic_s *next;
} textcmdtic_t;

extern textcmdtic_t *textcmds[TEXTCMD_HASH_SIZE];

extern UINT8 localtextcmd[MAXTEXTCMD];
extern UINT8 localtextcmd2[MAXTEXTCMD]; // splitscreen

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum));
void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam);
void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam); // splitsreen player

UINT8 GetFreeXCmdSize(void);
void D_FreeTextcmd(tic_t tic);

// Gets the buffer for the specified ticcmd, or NULL if there isn't one
UINT8* D_GetExistingTextcmd(tic_t tic, INT32 playernum);

// Gets the buffer for the specified ticcmd, creating one if necessary
UINT8* D_GetTextcmd(tic_t tic, INT32 playernum);

void ExtraDataTicker(void);

// used at txtcmds received to check packetsize bound
size_t TotalTextCmdPerTic(tic_t tic);

void PT_TextCmd(SINT8 node, INT32 netconsole);
void SV_WriteNetCommandsForTic(tic_t tic, UINT8 **buf);
void CL_CopyNetCommandsFromServerPacket(tic_t tic, UINT8 **buf);
void CL_SendNetCommands(void);
void SendKick(UINT8 playernum, UINT8 msg);
void SendKicksForNode(SINT8 node, UINT8 msg);

#endif
