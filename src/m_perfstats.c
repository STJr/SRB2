// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2021 by Sonic Team Junior.
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

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

struct perfstatcol;
struct perfstatrow;

typedef struct perfstatcol perfstatcol_t;
typedef struct perfstatrow perfstatrow_t;

struct perfstatcol {
	INT32 lores_x;
	INT32 hires_x;
	INT32 color;
	perfstatrow_t * rows;
};

struct perfstatrow {
	const char * lores_label;
	const char * hires_label;
	void       * value;
};

static precise_t ps_frametime = 0;

precise_t ps_tictime = 0;

precise_t ps_playerthink_time = 0;
precise_t ps_thinkertime = 0;

precise_t ps_thlist_times[NUM_THINKERLISTS];

int ps_checkposition_calls = 0;

precise_t ps_lua_thinkframe_time = 0;
int ps_lua_mobjhooks = 0;

// dynamically allocated resizeable array for thinkframe hook stats
ps_hookinfo_t *thinkframe_hooks = NULL;
int thinkframe_hooks_length = 0;
int thinkframe_hooks_capacity = 16;

static INT32 draw_row;

void PS_SetThinkFrameHookInfo(int index, precise_t time_taken, char* short_src)
{
	if (!thinkframe_hooks)
	{
		// array needs to be initialized
		thinkframe_hooks = Z_Malloc(sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	if (index >= thinkframe_hooks_capacity)
	{
		// array needs more space, realloc with double size
		thinkframe_hooks_capacity *= 2;
		thinkframe_hooks = Z_Realloc(thinkframe_hooks,
			sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	thinkframe_hooks[index].time_taken = time_taken;
	memcpy(thinkframe_hooks[index].short_src, short_src, LUA_IDSIZE * sizeof(char));
	// since the values are set sequentially from begin to end, the last call should leave
	// the correct value to this variable
	thinkframe_hooks_length = index + 1;
}

static void PS_SetFrameTime(void)
{
	precise_t currenttime = I_GetPreciseTime();
	ps_frametime = currenttime - ps_prevframetime;
	ps_prevframetime = currenttime;
}

static boolean M_HighResolution(void)
{
	return (vid.width >= 640 && vid.height >= 400);
}

enum {
	PERF_TIME,
	PERF_COUNT,
};

static void M_DrawPerfString(perfstatcol_t *col, int type)
{
	const boolean hires = M_HighResolution();

	INT32 draw_flags = V_MONOSPACE | col->color;

	perfstatrow_t * row;

	int value;

	if (hires)
		draw_flags |= V_ALLOWLOWERCASE;

	for (row = col->rows; row->lores_label; ++row)
	{
		if (type == PERF_TIME)
			value = I_PreciseToMicros(*(precise_t *)row->value);
		else
			value = *(int *)row->value;

		if (hires)
		{
			V_DrawSmallString(col->hires_x, draw_row, draw_flags,
					va("%s %d", row->hires_label, value));

			draw_row += 5;
		}
		else
		{
			V_DrawThinString(col->lores_x, draw_row, draw_flags,
					va("%s %d", row->lores_label, value));

			draw_row += 8;
		}
	}
}

static void M_DrawPerfTiming(perfstatcol_t *col)
{
	M_DrawPerfString(col, PERF_TIME);
}

static void M_DrawPerfCount(perfstatcol_t *col)
{
	M_DrawPerfString(col, PERF_COUNT);
}

static void M_DrawRenderStats(void)
{
	const boolean hires = M_HighResolution();

	const int half_row = hires ? 5 : 4;

	precise_t extrarendertime;

	perfstatrow_t frametime_row[] = {
		{"frmtime", "Frame time:    ", &ps_frametime},
		{0}
	};

	perfstatrow_t rendercalltime_row[] = {
		{"drwtime", "3d rendering:  ", &ps_rendercalltime},
		{0}
	};

	perfstatrow_t opengltime_row[] = {
		{"skybox ", "Skybox render: ", &ps_hw_skyboxtime},
		{"bsptime", "RenderBSPNode: ", &ps_bsptime},
		{"nodesrt", "Drwnode sort:  ", &ps_hw_nodesorttime},
		{"nodedrw", "Drwnode render:", &ps_hw_nodedrawtime},
		{"sprsort", "Sprite sort:   ", &ps_hw_spritesorttime},
		{"sprdraw", "Sprite render: ", &ps_hw_spritedrawtime},
		{"other  ", "Other:         ", &extrarendertime},
		{0}
	};

	perfstatrow_t softwaretime_row[] = {
		{"bsptime", "RenderBSPNode: ", &ps_bsptime},
		{"sprclip", "R_ClipSprites: ", &ps_sw_spritecliptime},
		{"portals", "Portals+Skybox:", &ps_sw_portaltime},
		{"planes ", "R_DrawPlanes:  ", &ps_sw_planetime},
		{"masked ", "R_DrawMasked:  ", &ps_sw_maskedtime},
		{"other  ", "Other:         ", &extrarendertime},
		{0}
	};

	perfstatrow_t uiswaptime_row[] = {
		{"ui     ", "UI render:     ", &ps_uitime},
		{"finupdt", "I_FinishUpdate:", &ps_swaptime},
		{0}
	};

	perfstatrow_t tictime_row[] = {
		{"logic  ", "Game logic:    ", &ps_tictime},
		{0}
	};

	perfstatrow_t rendercalls_row[] = {
		{"bspcall", "BSP calls:   ", &ps_numbspcalls},
		{"sprites", "Sprites:     ", &ps_numsprites},
		{"drwnode", "Drawnodes:   ", &ps_numdrawnodes},
		{"plyobjs", "Polyobjects: ", &ps_numpolyobjects},
		{0}
	};

	perfstatrow_t batchtime_row[] = {
		{"batsort", "Batch sort:  ", &ps_hw_batchsorttime},
		{"batdraw", "Batch render:", &ps_hw_batchdrawtime},
		{0}
	};

	perfstatrow_t batchcount_row[] = {
		{"polygon", "Polygons:  ", &ps_hw_numpolys},
		{"vertex ", "Vertices:  ", &ps_hw_numverts},
		{0}
	};

	perfstatrow_t batchcalls_row[] = {
		{"drwcall", "Draw calls:", &ps_hw_numcalls},
		{"shaders", "Shaders:   ", &ps_hw_numshaders},
		{"texture", "Textures:  ", &ps_hw_numtextures},
		{"polyflg", "Polyflags: ", &ps_hw_numpolyflags},
		{"colors ", "Colors:    ", &ps_hw_numcolors},
		{0}
	};

	perfstatcol_t      frametime_col =  {20,  20, V_YELLOWMAP,      frametime_row};
	perfstatcol_t rendercalltime_col =  {20,  20, V_YELLOWMAP, rendercalltime_row};

	perfstatcol_t     opengltime_col =  {24,  24, V_YELLOWMAP,     opengltime_row};
	perfstatcol_t   softwaretime_col =  {24,  24, V_YELLOWMAP,   softwaretime_row};

	perfstatcol_t     uiswaptime_col =  {20,  20, V_YELLOWMAP,     uiswaptime_row};
	perfstatcol_t        tictime_col =  {20,  20, V_GRAYMAP,          tictime_row};

	perfstatcol_t    rendercalls_col =  {90, 115, V_BLUEMAP,      rendercalls_row};

	perfstatcol_t      batchtime_col =  {90, 115, V_REDMAP,         batchtime_row};

	perfstatcol_t     batchcount_col = {155, 200, V_PURPLEMAP,     batchcount_row};
	perfstatcol_t     batchcalls_col = {220, 200, V_PURPLEMAP,     batchcalls_row};


	boolean rendering = (
			gamestate == GS_LEVEL ||
			(gamestate == GS_TITLESCREEN && titlemapinaction)
	);

	draw_row = 10;
	M_DrawPerfTiming(&frametime_col);

	if (rendering)
	{
		M_DrawPerfTiming(&rendercalltime_col);

		// Remember to update this calculation when adding more 3d rendering stats!
		extrarendertime = ps_rendercalltime - ps_bsptime;

#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			extrarendertime -=
				ps_hw_skyboxtime +
				ps_hw_nodesorttime +
				ps_hw_nodedrawtime +
				ps_hw_spritesorttime +
				ps_hw_spritedrawtime;

			if (cv_glbatching.value)
			{
				extrarendertime -=
					ps_hw_batchsorttime +
					ps_hw_batchdrawtime;
			}

			M_DrawPerfTiming(&opengltime_col);
		}
		else
#endif
		{
			extrarendertime -=
				ps_sw_spritecliptime +
				ps_sw_portaltime +
				ps_sw_planetime +
				ps_sw_maskedtime;

			M_DrawPerfTiming(&softwaretime_col);
		}
	}

	M_DrawPerfTiming(&uiswaptime_col);

	draw_row += half_row;
	M_DrawPerfTiming(&tictime_col);

	if (rendering)
	{
		draw_row = 10;
		M_DrawPerfCount(&rendercalls_col);

#ifdef HWRENDER
		if (rendermode == render_opengl && cv_glbatching.value)
		{
			draw_row += half_row;
			M_DrawPerfTiming(&batchtime_col);

			draw_row = 10;
			M_DrawPerfCount(&batchcount_col);

			if (hires)
				draw_row += half_row;
			else
				draw_row  = 10;

			M_DrawPerfCount(&batchcalls_col);
		}
#endif
	}
}

static void M_DrawTickStats(void)
{
	int i = 0;
	thinker_t *thinker;
	int thinkercount = 0;
	int polythcount = 0;
	int mainthcount = 0;
	int mobjcount = 0;
	int nothinkcount = 0;
	int scenerycount = 0;
	int regularcount = 0;
	int dynslopethcount = 0;
	int precipcount = 0;
	int removecount = 0;

	precise_t extratime =
		ps_tictime -
		ps_playerthink_time -
		ps_thinkertime -
		ps_lua_thinkframe_time;

	perfstatrow_t tictime_row[] = {
		{"logic  ", "Game logic:     ", &ps_tictime},
		{0}
	};

	perfstatrow_t thinker_time_row[] = {
		{"plrthnk", "P_PlayerThink:  ", &ps_playerthink_time},
		{"thnkers", "P_RunThinkers:  ", &ps_thinkertime},
		{0}
	};

	perfstatrow_t detailed_thinker_time_row[] = {
		{"plyobjs", "Polyobjects:    ", &ps_thlist_times[THINK_POLYOBJ]},
		{"main   ", "Main:           ", &ps_thlist_times[THINK_MAIN]},
		{"mobjs  ", "Mobjs:          ", &ps_thlist_times[THINK_MOBJ]},
		{"dynslop", "Dynamic slopes: ", &ps_thlist_times[THINK_DYNSLOPE]},
		{"precip ", "Precipitation:  ", &ps_thlist_times[THINK_PRECIP]},
		{0}
	};

	perfstatrow_t extra_thinker_time_row[] = {
		{"lthinkf", "LUAh_ThinkFrame:", &ps_lua_thinkframe_time},
		{"other  ", "Other:          ", &extratime},
		{0}
	};

	perfstatrow_t thinkercount_row[] = {
		{"thnkers", "Thinkers:       ", &thinkercount},
		{0}
	};

	perfstatrow_t detailed_thinkercount_row[] = {
		{"plyobjs", "Polyobjects:    ", &polythcount},
		{"main   ", "Main:           ", &mainthcount},
		{"mobjs  ", "Mobjs:          ", &mobjcount},
		{0}
	};

	perfstatrow_t mobjthinkercount_row[] = {
		{"regular", "Regular:        ", &regularcount},
		{"scenery", "Scenery:        ", &scenerycount},
		{0}
	};

	perfstatrow_t nothinkcount_row[] = {
		{"nothink", "Nothink:        ", &nothinkcount},
		{0}
	};

	perfstatrow_t detailed_thinkercount_row2[] = {
		{"dynslop", "Dynamic slopes: ", &dynslopethcount},
		{"precip ", "Precipitation:  ", &precipcount},
		{"remove ", "Pending removal:", &removecount},
		{0}
	};

	perfstatrow_t misc_calls_row[] = {
		{"lmhook", "Lua mobj hooks: ", &ps_lua_mobjhooks},
		{"chkpos", "P_CheckPosition:", &ps_checkposition_calls},
		{0}
	};

	perfstatcol_t               tictime_col  =  {20,  20, V_YELLOWMAP,               tictime_row};
	perfstatcol_t          thinker_time_col  =  {24,  24, V_YELLOWMAP,          thinker_time_row};
	perfstatcol_t detailed_thinker_time_col  =  {28,  28, V_YELLOWMAP, detailed_thinker_time_row};
	perfstatcol_t    extra_thinker_time_col  =  {24,  24, V_YELLOWMAP,    extra_thinker_time_row};

	perfstatcol_t          thinkercount_col  =  {90, 115, V_BLUEMAP,            thinkercount_row};
	perfstatcol_t detailed_thinkercount_col  =  {94, 119, V_BLUEMAP,   detailed_thinkercount_row};
	perfstatcol_t      mobjthinkercount_col  =  {98, 123, V_BLUEMAP,        mobjthinkercount_row};
	perfstatcol_t          nothinkcount_col  =  {98, 123, V_BLUEMAP,            nothinkcount_row};
	perfstatcol_t detailed_thinkercount_col2 =  {94, 119, V_BLUEMAP,   detailed_thinkercount_row2};
	perfstatcol_t            misc_calls_col  = {170, 216, V_PURPLEMAP,            misc_calls_row};

	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (thinker = thlist[i].next; thinker != &thlist[i]; thinker = thinker->next)
		{
			thinkercount++;
			if (thinker->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				removecount++;
			else if (i == THINK_POLYOBJ)
				polythcount++;
			else if (i == THINK_MAIN)
				mainthcount++;
			else if (i == THINK_MOBJ)
			{
				if (thinker->function.acp1 == (actionf_p1)P_MobjThinker)
				{
					mobj_t *mobj = (mobj_t*)thinker;
					mobjcount++;
					if (mobj->flags & MF_NOTHINK)
						nothinkcount++;
					else if (mobj->flags & MF_SCENERY)
						scenerycount++;
					else
						regularcount++;
				}
			}
			else if (i == THINK_DYNSLOPE)
				dynslopethcount++;
			else if (i == THINK_PRECIP)
				precipcount++;
		}
	}

	draw_row = 10;
	M_DrawPerfTiming(&tictime_col);
	M_DrawPerfTiming(&thinker_time_col);
	M_DrawPerfTiming(&detailed_thinker_time_col);
	M_DrawPerfTiming(&extra_thinker_time_col);

	draw_row = 10;
	M_DrawPerfCount(&thinkercount_col);
	M_DrawPerfCount(&detailed_thinkercount_col);
	M_DrawPerfCount(&mobjthinkercount_col);

	if (nothinkcount)
		M_DrawPerfCount(&nothinkcount_col);

	M_DrawPerfCount(&detailed_thinkercount_col2);

	if (M_HighResolution())
	{
		V_DrawSmallString(212, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, "Calls:");

		draw_row = 15;
	}
	else
	{
		draw_row = 10;
	}

	M_DrawPerfCount(&misc_calls_col);
}

void M_DrawPerfStats(void)
{
	char s[100];

	PS_SetFrameTime();

	if (cv_perfstats.value == 1) // rendering
	{
		M_DrawRenderStats();
	}
	else if (cv_perfstats.value == 2) // logic
	{
		M_DrawTickStats();
	}
	else if (cv_perfstats.value == 3) // lua thinkframe
	{
		if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
			return;
		if (vid.width < 640 || vid.height < 400) // low resolution
		{
			// it's not gonna fit very well..
			V_DrawThinString(30, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "Not available for resolutions below 640x400");
		}
		else // high resolution
		{
			int i;
			// text writing position
			int x = 2;
			int y = 4;
			UINT32 text_color;
			char tempbuffer[LUA_IDSIZE];
			char last_mod_name[LUA_IDSIZE];
			last_mod_name[0] = '\0';
			for (i = 0; i < thinkframe_hooks_length; i++)
			{
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
						y += 4; // repeated code!
						if (y > 192)
						{
							y = 4;
							x += 106;
							if (x > 214)
								break;
						}
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
				snprintf(s, sizeof s - 1, "%20s: %d", str, I_PreciseToMicros(thinkframe_hooks[i].time_taken));
				V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | text_color, s);
				y += 4; // repeated code!
				if (y > 192)
				{
					y = 4;
					x += 106;
					if (x > 214)
						break;
				}
			}
		}
	}
}
