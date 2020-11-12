// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file m_perfstats.h
/// \brief Performance measurement tools.

#ifndef __M_PERFSTATS_H__
#define __M_PERFSTATS_H__

#include "doomdef.h"
#include "lua_script.h"
#include "p_local.h"

extern int ps_tictime;

extern int ps_playerthink_time;
extern int ps_thinkertime;

extern int ps_thlist_times[];

extern int ps_checkposition_calls;

extern int ps_lua_thinkframe_time;
extern int ps_lua_mobjhooks;

typedef struct
{
	UINT32 time_taken;
	char short_src[LUA_IDSIZE];
} ps_hookinfo_t;

void PS_SetThinkFrameHookInfo(int index, UINT32 time_taken, char* short_src);

void M_DrawPerfStats(void);

#endif
