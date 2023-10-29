// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file m_perfstats.c
/// \brief Performance measurement tools.

#include "m_perfstats.h"
#include "v_video.h"
#include "i_video.h"
#include "d_netcmd.h"
#include "r_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"
#include "r_fps.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

struct perfstatrow;

typedef struct perfstatrow perfstatrow_t;

struct perfstatrow {
	const char  * lores_label;
	const char  * hires_label;
	ps_metric_t * metric;
	UINT8         flags;
};

// perfstatrow_t flags

#define PS_TIME      1  // metric measures time (uses precise_t instead of INT32)
#define PS_LEVEL     2  // metric is valid only when a level is active
#define PS_SW        4  // metric is valid only in software mode
#define PS_HW        8  // metric is valid only in opengl mode
#define PS_BATCHING  16 // metric is valid only when opengl batching is active
#define PS_HIDE_ZERO 32 // hide metric if its value is zero

static ps_metric_t ps_frametime = {0};

ps_metric_t ps_tictime = {0};

ps_metric_t ps_playerthink_time = {0};
ps_metric_t ps_thinkertime = {0};

ps_metric_t ps_thlist_times[NUM_THINKERLISTS];

static ps_metric_t ps_thinkercount = {0};
static ps_metric_t ps_polythcount = {0};
static ps_metric_t ps_mainthcount = {0};
static ps_metric_t ps_mobjcount = {0};
static ps_metric_t ps_regularcount = {0};
static ps_metric_t ps_scenerycount = {0};
static ps_metric_t ps_nothinkcount = {0};
static ps_metric_t ps_dynslopethcount = {0};
static ps_metric_t ps_precipcount = {0};
static ps_metric_t ps_removecount = {0};

ps_metric_t ps_checkposition_calls = {0};

ps_metric_t ps_lua_thinkframe_time = {0};
ps_metric_t ps_lua_mobjhooks = {0};

ps_metric_t ps_otherlogictime = {0};

// Columns for perfstats pages.

// Position on screen is determined separately in the drawing functions.

// New columns must also be added to the drawing and update functions.
// Drawing functions: PS_DrawRenderStats, PS_DrawGameLogicStats, etc.
// Update functions:
//  - PS_UpdateFrameStats for frame-dependent values
//  - PS_UpdateTickStats for tick-dependent values

// Rendering stats columns

perfstatrow_t rendertime_rows[] = {
	{"frmtime", "Frame time:    ", &ps_frametime, PS_TIME},
	{"drwtime", "3d rendering:  ", &ps_rendercalltime, PS_TIME|PS_LEVEL},

#ifdef HWRENDER
	{" skybox ", " Skybox render: ", &ps_hw_skyboxtime, PS_TIME|PS_LEVEL|PS_HW},
	{" bsptime", " RenderBSPNode: ", &ps_bsptime, PS_TIME|PS_LEVEL|PS_HW},
	{" batsort", " Batch sort:    ", &ps_hw_batchsorttime, PS_TIME|PS_LEVEL|PS_HW|PS_BATCHING},
	{" batdraw", " Batch render:  ", &ps_hw_batchdrawtime, PS_TIME|PS_LEVEL|PS_HW|PS_BATCHING},
	{" sprsort", " Sprite sort:   ", &ps_hw_spritesorttime, PS_TIME|PS_LEVEL|PS_HW},
	{" sprdraw", " Sprite render: ", &ps_hw_spritedrawtime, PS_TIME|PS_LEVEL|PS_HW},
	{" nodesrt", " Drwnode sort:  ", &ps_hw_nodesorttime, PS_TIME|PS_LEVEL|PS_HW},
	{" nodedrw", " Drwnode render:", &ps_hw_nodedrawtime, PS_TIME|PS_LEVEL|PS_HW},
	{" other  ", " Other:         ", &ps_otherrendertime, PS_TIME|PS_LEVEL|PS_HW},
#endif

	{" bsptime", " RenderBSPNode: ", &ps_bsptime, PS_TIME|PS_LEVEL|PS_SW},
	{" sprclip", " R_ClipSprites: ", &ps_sw_spritecliptime, PS_TIME|PS_LEVEL|PS_SW},
	{" portals", " Portals+Skybox:", &ps_sw_portaltime, PS_TIME|PS_LEVEL|PS_SW},
	{" planes ", " R_DrawPlanes:  ", &ps_sw_planetime, PS_TIME|PS_LEVEL|PS_SW},
	{" masked ", " R_DrawMasked:  ", &ps_sw_maskedtime, PS_TIME|PS_LEVEL|PS_SW},
	{" other  ", " Other:         ", &ps_otherrendertime, PS_TIME|PS_LEVEL|PS_SW},

	{"ui     ", "UI render:     ", &ps_uitime, PS_TIME},
	{"finupdt", "I_FinishUpdate:", &ps_swaptime, PS_TIME},
	{0}
};

perfstatrow_t gamelogicbrief_row[] = {
	{"logic  ", "Game logic:    ", &ps_tictime, PS_TIME},
	{0}
};

perfstatrow_t commoncounter_rows[] = {
	{"bspcall", "BSP calls:   ", &ps_numbspcalls, 0},
	{"sprites", "Sprites:     ", &ps_numsprites, 0},
	{"drwnode", "Drawnodes:   ", &ps_numdrawnodes, 0},
	{"plyobjs", "Polyobjects: ", &ps_numpolyobjects, 0},
	{0}
};

perfstatrow_t interpolation_rows[] = {
	{"intpfrc", "Interp frac: ", &ps_interp_frac, PS_TIME},
	{"intplag", "Interp lag:  ", &ps_interp_lag, PS_TIME},
	{0}
};

#ifdef HWRENDER
perfstatrow_t batchcount_rows[] = {
	{"polygon", "Polygons:  ", &ps_hw_numpolys, 0},
	{"vertex ", "Vertices:  ", &ps_hw_numverts, 0},
	{0}
};

perfstatrow_t batchcalls_rows[] = {
	{"drwcall", "Draw calls:", &ps_hw_numcalls, 0},
	{"shaders", "Shaders:   ", &ps_hw_numshaders, 0},
	{"texture", "Textures:  ", &ps_hw_numtextures, 0},
	{"polyflg", "Polyflags: ", &ps_hw_numpolyflags, 0},
	{"colors ", "Colors:    ", &ps_hw_numcolors, 0},
	{0}
};
#endif

// Game logic stats columns

perfstatrow_t gamelogic_rows[] = {
	{"logic  ", "Game logic:     ", &ps_tictime, PS_TIME},
	{" plrthnk", " P_PlayerThink:  ", &ps_playerthink_time, PS_TIME|PS_LEVEL},
	{" thnkers", " P_RunThinkers:  ", &ps_thinkertime, PS_TIME|PS_LEVEL},
	{"  plyobjs", "  Polyobjects:    ", &ps_thlist_times[THINK_POLYOBJ], PS_TIME|PS_LEVEL},
	{"  main   ", "  Main:           ", &ps_thlist_times[THINK_MAIN], PS_TIME|PS_LEVEL},
	{"  mobjs  ", "  Mobjs:          ", &ps_thlist_times[THINK_MOBJ], PS_TIME|PS_LEVEL},
	{"  dynslop", "  Dynamic slopes: ", &ps_thlist_times[THINK_DYNSLOPE], PS_TIME|PS_LEVEL},
	{"  precip ", "  Precipitation:  ", &ps_thlist_times[THINK_PRECIP], PS_TIME|PS_LEVEL},
	{" lthinkf", " LUAh_ThinkFrame:", &ps_lua_thinkframe_time, PS_TIME|PS_LEVEL},
	{" other  ", " Other:          ", &ps_otherlogictime, PS_TIME|PS_LEVEL},
	{0}
};

perfstatrow_t thinkercount_rows[] = {
	{"thnkers", "Thinkers:       ", &ps_thinkercount, PS_LEVEL},
	{" plyobjs", " Polyobjects:    ", &ps_polythcount, PS_LEVEL},
	{" main   ", " Main:           ", &ps_mainthcount, PS_LEVEL},
	{" mobjs  ", " Mobjs:          ", &ps_mobjcount, PS_LEVEL},
	{"  regular", "  Regular:        ", &ps_regularcount, PS_LEVEL},
	{"  scenery", "  Scenery:        ", &ps_scenerycount, PS_LEVEL},
	{"  nothink", "  Nothink:        ", &ps_nothinkcount, PS_HIDE_ZERO|PS_LEVEL},
	{" dynslop", " Dynamic slopes: ", &ps_dynslopethcount, PS_LEVEL},
	{" precip ", " Precipitation:  ", &ps_precipcount, PS_LEVEL},
	{" remove ", " Pending removal:", &ps_removecount, PS_LEVEL},
	{0}
};

perfstatrow_t misc_calls_rows[] = {
	{"lmhook", "Lua mobj hooks: ", &ps_lua_mobjhooks, PS_LEVEL},
	{"chkpos", "P_CheckPosition:", &ps_checkposition_calls, PS_LEVEL},
	{0}
};

// Sample collection status for averaging.
// Maximum of these two is shown to user if nonzero to tell that
// the reported averages are not correct yet.
int ps_frame_samples_left = 0;
int ps_tick_samples_left = 0;
// History writing positions for frame and tick based metrics
int ps_frame_index = 0;
int ps_tick_index = 0;

// dynamically allocated resizeable array for thinkframe hook stats
ps_hookinfo_t *thinkframe_hooks = NULL;
int thinkframe_hooks_length = 0;
int thinkframe_hooks_capacity = 16;

void PS_SetThinkFrameHookInfo(int index, precise_t time_taken, char* short_src)
{
	if (!thinkframe_hooks)
	{
		// array needs to be initialized
		thinkframe_hooks = Z_Calloc(sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	if (index >= thinkframe_hooks_capacity)
	{
		// array needs more space, realloc with double size
		int new_capacity = thinkframe_hooks_capacity * 2;
		thinkframe_hooks = Z_Realloc(thinkframe_hooks,
			sizeof(ps_hookinfo_t) * new_capacity, PU_STATIC, NULL);
		// initialize new memory with zeros so the pointers in the structs are null
		memset(&thinkframe_hooks[thinkframe_hooks_capacity], 0,
			sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity);
		thinkframe_hooks_capacity = new_capacity;
	}
	thinkframe_hooks[index].time_taken.value.p = time_taken;
	memcpy(thinkframe_hooks[index].short_src, short_src, LUA_IDSIZE * sizeof(char));
	// since the values are set sequentially from begin to end, the last call should leave
	// the correct value to this variable
	thinkframe_hooks_length = index + 1;
}

static boolean PS_HighResolution(void)
{
	return (vid.width >= 640 && vid.height >= 400);
}

static boolean PS_IsLevelActive(void)
{
	return gamestate == GS_LEVEL ||
			(gamestate == GS_TITLESCREEN && titlemapinaction);
}

// Is the row valid in the current context?
static boolean PS_IsRowValid(perfstatrow_t *row)
{
	return !((row->flags & PS_LEVEL && !PS_IsLevelActive())
		|| (row->flags & PS_SW && rendermode != render_soft)
		|| (row->flags & PS_HW && rendermode != render_opengl)
#ifdef HWRENDER
		|| (row->flags & PS_BATCHING && !cv_glbatching.value)
#endif
		);
}

// Should the row be visible on the screen?
static boolean PS_IsRowVisible(perfstatrow_t *row)
{
	boolean value_is_zero;

	if (row->flags & PS_TIME)
		value_is_zero = row->metric->value.p == 0;
	else
		value_is_zero = row->metric->value.i == 0;

	return !(!PS_IsRowValid(row) ||
		(row->flags & PS_HIDE_ZERO && value_is_zero));
}

static INT32 PS_GetMetricAverage(ps_metric_t *metric, boolean time_metric)
{
	char* history_read_pos = metric->history; // char* used for pointer arithmetic
	INT64 sum = 0;
	int i;
	int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);

	for (i = 0; i < cv_ps_samplesize.value; i++)
	{
		if (time_metric)
			sum += (*((precise_t*)history_read_pos)) / (I_GetPrecisePrecision() / 1000000);
		else
			sum += *((INT32*)history_read_pos);
		history_read_pos += value_size;
	}

	return sum / cv_ps_samplesize.value;
}

static INT32 PS_GetMetricMinOrMax(ps_metric_t *metric, boolean time_metric, boolean get_max)
{
	char* history_read_pos = metric->history; // char* used for pointer arithmetic
	INT32 found_value = get_max ? INT32_MIN : INT32_MAX;
	int i;
	int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);

	for (i = 0; i < cv_ps_samplesize.value; i++)
	{
		INT32 value;
		if (time_metric)
			value = (*((precise_t*)history_read_pos)) / (I_GetPrecisePrecision() / 1000000);
		else
			value = *((INT32*)history_read_pos);

		if ((get_max && value > found_value) ||
			(!get_max && value < found_value))
		{
			found_value = value;
		}
		history_read_pos += value_size;
	}

	return found_value;
}

// Calculates the standard deviation for metric.
static INT32 PS_GetMetricSD(ps_metric_t *metric, boolean time_metric)
{
	char* history_read_pos = metric->history; // char* used for pointer arithmetic
	INT64 sum = 0;
	int i;
	int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);
	INT32 avg = PS_GetMetricAverage(metric, time_metric);

	for (i = 0; i < cv_ps_samplesize.value; i++)
	{
		INT64 value;
		if (time_metric)
			value = (*((precise_t*)history_read_pos)) / (I_GetPrecisePrecision() / 1000000);
		else
			value = *((INT32*)history_read_pos);

		value -= avg;
		sum += value * value;

		history_read_pos += value_size;
	}

	return round(sqrt(sum / cv_ps_samplesize.value));
}

// Returns the value to show on screen for metric.
static INT32 PS_GetMetricScreenValue(ps_metric_t *metric, boolean time_metric)
{
	if (cv_ps_samplesize.value > 1 && metric->history)
	{
		if (cv_ps_descriptor.value == 1)
			return PS_GetMetricAverage(metric, time_metric);
		else if (cv_ps_descriptor.value == 2)
			return PS_GetMetricSD(metric, time_metric);
		else if (cv_ps_descriptor.value == 3)
			return PS_GetMetricMinOrMax(metric, time_metric, false);
		else
			return PS_GetMetricMinOrMax(metric, time_metric, true);
	}
	else
	{
		if (time_metric)
			return (metric->value.p) / (I_GetPrecisePrecision() / 1000000);
		else
			return metric->value.i;
	}
}

static int PS_DrawPerfRows(int x, int y, int color, perfstatrow_t *rows)
{
	const boolean hires = PS_HighResolution();
	INT32 draw_flags = V_MONOSPACE | color;
	perfstatrow_t * row;
	int draw_y = y;

	if (hires)
		draw_flags |= V_ALLOWLOWERCASE;

	for (row = rows; row->lores_label; ++row)
	{
		const char *label;
		INT32 value;
		char *final_str;

		if (!PS_IsRowVisible(row))
			continue;

		label = hires ? row->hires_label : row->lores_label;
		value = PS_GetMetricScreenValue(row->metric, !!(row->flags & PS_TIME));
		final_str = va("%s %d", label, value);

		if (hires)
		{
			V_DrawSmallString(x, draw_y, draw_flags, final_str);
			draw_y += 5;
		}
		else
		{
			V_DrawThinString(x, draw_y, draw_flags, final_str);
			draw_y += 8;
		}
	}

	return draw_y;
}

static void PS_UpdateMetricHistory(ps_metric_t *metric, boolean time_metric, boolean frame_metric, boolean set_user)
{
	int index = frame_metric ? ps_frame_index : ps_tick_index;

	if (!metric->history)
	{
		// allocate history table
		int value_size = time_metric ? sizeof(precise_t) : sizeof(INT32);
		void** memory_user = set_user ? &metric->history : NULL;

		metric->history = Z_Calloc(value_size * cv_ps_samplesize.value, PU_PERFSTATS,
				memory_user);

		// reset "samples left" counter since this history table needs to be filled
		if (frame_metric)
			ps_frame_samples_left = cv_ps_samplesize.value;
		else
			ps_tick_samples_left = cv_ps_samplesize.value;
	}

	if (time_metric)
	{
		precise_t *history = (precise_t*)metric->history;
		history[index] = metric->value.p;
	}
	else
	{
		INT32 *history = (INT32*)metric->history;
		history[index] = metric->value.i;
	}
}

static void PS_UpdateRowHistories(perfstatrow_t *rows, boolean frame_metric)
{
	perfstatrow_t *row;
	for (row = rows; row->lores_label; row++)
	{
		if (PS_IsRowValid(row))
			PS_UpdateMetricHistory(row->metric, !!(row->flags & PS_TIME), frame_metric, true);
	}
}

// Update all metrics that are calculated on every frame.
static void PS_UpdateFrameStats(void)
{
	// update frame time
	precise_t currenttime = I_GetPreciseTime();
	ps_frametime.value.p = currenttime - ps_prevframetime;
	ps_prevframetime = currenttime;

	// update 3d rendering stats
	if (PS_IsLevelActive())
	{
		// Remember to update this calculation when adding more 3d rendering stats!
		ps_otherrendertime.value.p = ps_rendercalltime.value.p - ps_bsptime.value.p;

#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			ps_otherrendertime.value.p -=
				ps_hw_skyboxtime.value.p +
				ps_hw_nodesorttime.value.p +
				ps_hw_nodedrawtime.value.p +
				ps_hw_spritesorttime.value.p +
				ps_hw_spritedrawtime.value.p;

			if (cv_glbatching.value)
			{
				ps_otherrendertime.value.p -=
					ps_hw_batchsorttime.value.p +
					ps_hw_batchdrawtime.value.p;
			}
		}
		else
#endif
		{
			ps_otherrendertime.value.p -=
				ps_sw_spritecliptime.value.p +
				ps_sw_portaltime.value.p +
				ps_sw_planetime.value.p +
				ps_sw_maskedtime.value.p;
		}
	}

	if (cv_ps_samplesize.value > 1)
	{
		PS_UpdateRowHistories(rendertime_rows, true);
		if (PS_IsLevelActive())
			PS_UpdateRowHistories(commoncounter_rows, true);

		if (R_UsingFrameInterpolation())
			PS_UpdateRowHistories(interpolation_rows, true);

#ifdef HWRENDER
		if (rendermode == render_opengl && cv_glbatching.value)
		{
			PS_UpdateRowHistories(batchcount_rows, true);
			PS_UpdateRowHistories(batchcalls_rows, true);
		}
#endif

		ps_frame_index++;
		if (ps_frame_index >= cv_ps_samplesize.value)
			ps_frame_index = 0;
		if (ps_frame_samples_left)
			ps_frame_samples_left--;
	}
}

// Update thinker counters by iterating the thinker lists.
static void PS_CountThinkers(void)
{
	int i;
	thinker_t *thinker;

	ps_thinkercount.value.i = 0;
	ps_polythcount.value.i = 0;
	ps_mainthcount.value.i = 0;
	ps_mobjcount.value.i = 0;
	ps_regularcount.value.i = 0;
	ps_scenerycount.value.i = 0;
	ps_nothinkcount.value.i = 0;
	ps_dynslopethcount.value.i = 0;
	ps_precipcount.value.i = 0;
	ps_removecount.value.i = 0;

	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (thinker = thlist[i].next; thinker != &thlist[i]; thinker = thinker->next)
		{
			ps_thinkercount.value.i++;
			if (thinker->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				ps_removecount.value.i++;
			else if (i == THINK_POLYOBJ)
				ps_polythcount.value.i++;
			else if (i == THINK_MAIN)
				ps_mainthcount.value.i++;
			else if (i == THINK_MOBJ)
			{
				if (thinker->function.acp1 == (actionf_p1)P_MobjThinker)
				{
					mobj_t *mobj = (mobj_t*)thinker;
					ps_mobjcount.value.i++;
					if (mobj->flags & MF_NOTHINK)
						ps_nothinkcount.value.i++;
					else if (mobj->flags & MF_SCENERY)
						ps_scenerycount.value.i++;
					else
						ps_regularcount.value.i++;
				}
			}
			else if (i == THINK_DYNSLOPE)
				ps_dynslopethcount.value.i++;
			else if (i == THINK_PRECIP)
				ps_precipcount.value.i++;
		}
	}
}

// Update all metrics that are calculated on every tick.
void PS_UpdateTickStats(void)
{
	if (cv_perfstats.value == 1 && cv_ps_samplesize.value > 1)
	{
		PS_UpdateRowHistories(gamelogicbrief_row, false);
	}
	if (cv_perfstats.value == 2)
	{
		if (PS_IsLevelActive())
		{
			ps_otherlogictime.value.p =
				ps_tictime.value.p -
				ps_playerthink_time.value.p -
				ps_thinkertime.value.p -
				ps_lua_thinkframe_time.value.p;

			PS_CountThinkers();
		}

		if (cv_ps_samplesize.value > 1)
		{
			PS_UpdateRowHistories(gamelogic_rows, false);
			PS_UpdateRowHistories(thinkercount_rows, false);
			PS_UpdateRowHistories(misc_calls_rows, false);
		}
	}
	if (cv_perfstats.value == 3 && cv_ps_samplesize.value > 1 && PS_IsLevelActive())
	{
		int i;
		for (i = 0; i < thinkframe_hooks_length; i++)
		{
			PS_UpdateMetricHistory(&thinkframe_hooks[i].time_taken, true, false, false);
		}
	}
	if (cv_perfstats.value && cv_ps_samplesize.value > 1)
	{
		ps_tick_index++;
		if (ps_tick_index >= cv_ps_samplesize.value)
			ps_tick_index = 0;
		if (ps_tick_samples_left)
			ps_tick_samples_left--;
	}
}

static void PS_DrawDescriptorHeader(void)
{
	if (cv_ps_samplesize.value > 1)
	{
		const char* descriptor_names[] = {
			"average",
			"standard deviation",
			"minimum",
			"maximum"
		};
		const boolean hires = PS_HighResolution();
		char* str;
		INT32 flags = V_MONOSPACE | V_ALLOWLOWERCASE;
		int samples_left = max(ps_frame_samples_left, ps_tick_samples_left);
		int x, y;

		if (cv_perfstats.value == 3)
		{
			x = 2;
			y = 0;
		}
		else
		{
			x = 20;
			y = hires ? 5 : 2;
		}

		if (samples_left)
		{
			str = va("Samples needed for correct results: %d", samples_left);
			flags |= V_REDMAP;
		}
		else
		{
			str = va("Showing the %s of %d samples.",
					descriptor_names[cv_ps_descriptor.value - 1], cv_ps_samplesize.value);
			flags |= V_GREENMAP;
		}

		if (hires)
			V_DrawSmallString(x, y, flags, str);
		else
			V_DrawThinString(x, y, flags, str);
	}
}

static void PS_DrawRenderStats(void)
{
	const boolean hires = PS_HighResolution();
	const int half_row = hires ? 5 : 4;
	int x, y, cy = 10;

	PS_DrawDescriptorHeader();

	y = PS_DrawPerfRows(20, 10, V_YELLOWMAP, rendertime_rows);

	PS_DrawPerfRows(20, y + half_row, V_GRAYMAP, gamelogicbrief_row);

	if (PS_IsLevelActive())
	{
		x = hires ? 115 : 90;
		cy = PS_DrawPerfRows(x, 10, V_BLUEMAP, commoncounter_rows) + half_row;

#ifdef HWRENDER
		if (rendermode == render_opengl && cv_glbatching.value)
		{
			x = hires ? 200 : 155;
			y = PS_DrawPerfRows(x, 10, V_PURPLEMAP, batchcount_rows);

			x = hires ? 200 : 220;
			y = hires ? y + half_row : 10;
			PS_DrawPerfRows(x, y, V_PURPLEMAP, batchcalls_rows);
		}
#endif
	}

	if (R_UsingFrameInterpolation())
	{
		x = hires ? 115 : 90;
		PS_DrawPerfRows(x, cy, V_ROSYMAP, interpolation_rows);
	}
}

static void PS_DrawGameLogicStats(void)
{
	const boolean hires = PS_HighResolution();
	int x, y;

	PS_DrawDescriptorHeader();

	PS_DrawPerfRows(20, 10, V_YELLOWMAP, gamelogic_rows);

	x = hires ? 115 : 90;
	PS_DrawPerfRows(x, 10, V_BLUEMAP, thinkercount_rows);

	if (hires)
		V_DrawSmallString(212, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, "Calls:");

	x = hires ? 216 : 170;
	y = hires ? 15 : 10;
	PS_DrawPerfRows(x, y, V_PURPLEMAP, misc_calls_rows);
}

static void PS_DrawThinkFrameStats(void)
{
	char s[100];
	int i;
	// text writing position
	int x = 2;
	int y = 4;
	UINT32 text_color;
	char tempbuffer[LUA_IDSIZE];
	char last_mod_name[LUA_IDSIZE];
	last_mod_name[0] = '\0';

	PS_DrawDescriptorHeader();

	for (i = 0; i < thinkframe_hooks_length; i++)
	{

#define NEXT_ROW() \
y += 4; \
if (y > 192) \
{ \
	y = 4; \
	x += 106; \
	if (x > 214) \
		break; \
}

		char* str = thinkframe_hooks[i].short_src;
		char* tempstr = tempbuffer;
		int len = (int)strlen(str);
		char* str_ptr;
		if (strcmp(".lua", str + len - 4) == 0)
		{
			str[len-4] = '\0'; // remove .lua at end
			len -= 4;
		}
		// we locate the wad/pk3 name in the string and compare it to
		// what we found on the previous iteration.
		// if the name has changed, print it out on the screen
		strcpy(tempstr, str);
		str_ptr = strrchr(tempstr, '|');
		if (str_ptr)
		{
			*str_ptr = '\0';
			str = str_ptr + 1; // this is the name of the hook without the mod file name
			str_ptr = strrchr(tempstr, PATHSEP[0]);
			if (str_ptr)
				tempstr = str_ptr + 1;
			// tempstr should now point to the mod name, (wad/pk3) possibly truncated
			if (strcmp(tempstr, last_mod_name) != 0)
			{
				strcpy(last_mod_name, tempstr);
				len = (int)strlen(tempstr);
				if (len > 25)
					tempstr += len - 25;
				snprintf(s, sizeof s - 1, "%s", tempstr);
				V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
				NEXT_ROW()
			}
			text_color = V_YELLOWMAP;
		}
		else
		{
			// probably a standalone lua file
			// cut off the folder if it's there
			str_ptr = strrchr(tempstr, PATHSEP[0]);
			if (str_ptr)
				str = str_ptr + 1;
			text_color = 0; // white
		}
		len = (int)strlen(str);
		if (len > 20)
			str += len - 20;
		snprintf(s, sizeof s - 1, "%20s: %d", str,
				PS_GetMetricScreenValue(&thinkframe_hooks[i].time_taken, true));
		V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | text_color, s);
		NEXT_ROW()

#undef NEXT_ROW

	}
}

void M_DrawPerfStats(void)
{
	if (cv_perfstats.value == 1) // rendering
	{
		PS_UpdateFrameStats();
		PS_DrawRenderStats();
	}
	else if (cv_perfstats.value == 2) // logic
	{
		// PS_UpdateTickStats is called in TryRunTics, since otherwise it would miss
		// tics when frame skips happen
		PS_DrawGameLogicStats();
	}
	else if (cv_perfstats.value == 3) // lua thinkframe
	{
		if (!PS_IsLevelActive())
			return;
		if (!PS_HighResolution())
		{
			// Low resolutions can't really use V_DrawSmallString that is used by thinkframe stats.
			// A low-res version using V_DrawThinString could be implemented,
			// but it would have much less space for information.
			V_DrawThinString(80, 92, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "Perfstats 3 is not available");
			V_DrawThinString(80, 100, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "for resolutions below 640x400.");
		}
		else
		{
			PS_DrawThinkFrameStats();
		}
	}
}

// remove and unallocate history from all metrics
static void PS_ClearHistory(void)
{
	int i;

	Z_FreeTag(PU_PERFSTATS);
	// thinkframe hook metric history pointers need to be cleared manually
	for (i = 0; i < thinkframe_hooks_length; i++)
	{
		thinkframe_hooks[i].time_taken.history = NULL;
	}

	ps_frame_index = ps_tick_index = 0;
	// PS_UpdateMetricHistory will set these correctly when it runs
	ps_frame_samples_left = ps_tick_samples_left = 0;
}

void PS_PerfStats_OnChange(void)
{
	if (cv_perfstats.value && cv_ps_samplesize.value > 1)
		PS_ClearHistory();
}

void PS_SampleSize_OnChange(void)
{
	if (cv_ps_samplesize.value > 1)
		PS_ClearHistory();
}
