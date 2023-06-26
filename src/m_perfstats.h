// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Sonic Team Junior.
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

typedef struct
{
	union {
		precise_t p;
		INT32 i;
	} value;
	void *history;
} ps_metric_t;

typedef struct
{
	ps_metric_t time_taken;
	char short_src[LUA_IDSIZE];
} ps_hookinfo_t;

#define PS_START_TIMING(metric) metric.value.p = I_GetPreciseTime()
#define PS_STOP_TIMING(metric) metric.value.p = I_GetPreciseTime() - metric.value.p

extern ps_metric_t ps_tictime;

extern ps_metric_t ps_playerthink_time;
extern ps_metric_t ps_thinkertime;

extern ps_metric_t ps_thlist_times[];

extern ps_metric_t ps_checkposition_calls;

extern ps_metric_t ps_lua_thinkframe_time;
extern ps_metric_t ps_lua_mobjhooks;

extern ps_metric_t ps_otherlogictime;

void PS_SetThinkFrameHookInfo(int index, precise_t time_taken, char* short_src);

void PS_UpdateTickStats(void);

void M_DrawPerfStats(void);

void PS_PerfStats_OnChange(void);
void PS_SampleSize_OnChange(void);

#endif
