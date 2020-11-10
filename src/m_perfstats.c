// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Sonic Team Junior.
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

int ps_tictime = 0;

int ps_playerthink_time = 0;
int ps_thinkertime = 0;

int ps_thlist_times[NUM_THINKERLISTS];
static const char* thlist_names[] = {
	"Polyobjects:     %d",
	"Main:            %d",
	"Mobjs:           %d",
	"Dynamic slopes:  %d",
	"Precipitation:   %d",
	NULL
};
static const char* thlist_shortnames[] = {
	"plyobjs %d",
	"main    %d",
	"mobjs   %d",
	"dynslop %d",
	"precip  %d",
	NULL
};

int ps_checkposition_calls = 0;

int ps_lua_thinkframe_time = 0;
int ps_lua_mobjhooks = 0;

// dynamically allocated resizeable array for thinkframe hook stats
ps_hookinfo_t *thinkframe_hooks = NULL;
int thinkframe_hooks_length = 0;
int thinkframe_hooks_capacity = 16;

void PS_SetThinkFrameHookInfo(int index, UINT32 time_taken, char* short_src)
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

void M_DrawPerfStats(void)
{
	char s[100];
	int currenttime = I_GetTimeMicros();
	int frametime = currenttime - ps_prevframetime;
	ps_prevframetime = currenttime;

	if (cv_perfstats.value == 1) // rendering
	{
		if (vid.width < 640 || vid.height < 400) // low resolution
		{
			snprintf(s, sizeof s - 1, "frmtime %d", frametime);
			V_DrawThinString(20, 10, V_MONOSPACE | V_YELLOWMAP, s);
			if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
			{
				snprintf(s, sizeof s - 1, "ui      %d", ps_uitime);
				V_DrawThinString(20, 18, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "finupdt %d", ps_swaptime);
				V_DrawThinString(20, 26, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "logic   %d", ps_tictime);
				V_DrawThinString(20, 38, V_MONOSPACE | V_GRAYMAP, s);
				return;
			}
			snprintf(s, sizeof s - 1, "drwtime %d", ps_rendercalltime);
			V_DrawThinString(20, 18, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "bspcall %d", ps_numbspcalls);
			V_DrawThinString(90, 10, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "sprites %d", ps_numsprites);
			V_DrawThinString(90, 18, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "drwnode %d", ps_numdrawnodes);
			V_DrawThinString(90, 26, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "plyobjs %d", ps_numpolyobjects);
			V_DrawThinString(90, 34, V_MONOSPACE | V_BLUEMAP, s);
#ifdef HWRENDER
			if (rendermode == render_opengl) // OpenGL specific stats
			{
				snprintf(s, sizeof s - 1, "skybox  %d", ps_hw_skyboxtime);
				V_DrawThinString(24, 26, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "bsptime %d", ps_bsptime);
				V_DrawThinString(24, 34, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "nodesrt %d", ps_hw_nodesorttime);
				V_DrawThinString(24, 42, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "nodedrw %d", ps_hw_nodedrawtime);
				V_DrawThinString(24, 50, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "sprsort %d", ps_hw_spritesorttime);
				V_DrawThinString(24, 58, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "sprdraw %d", ps_hw_spritedrawtime);
				V_DrawThinString(24, 66, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "other   %d",
					ps_rendercalltime - ps_hw_skyboxtime - ps_bsptime - ps_hw_nodesorttime
					- ps_hw_nodedrawtime - ps_hw_spritesorttime - ps_hw_spritedrawtime
					- ps_hw_batchsorttime - ps_hw_batchdrawtime);
				V_DrawThinString(24, 74, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "ui      %d", ps_uitime);
				V_DrawThinString(20, 82, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "finupdt %d", ps_swaptime);
				V_DrawThinString(20, 90, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "logic   %d", ps_tictime);
				V_DrawThinString(20, 102, V_MONOSPACE | V_GRAYMAP, s);
				if (cv_glbatching.value)
				{
					snprintf(s, sizeof s - 1, "batsort %d", ps_hw_batchsorttime);
					V_DrawThinString(90, 46, V_MONOSPACE | V_REDMAP, s);
					snprintf(s, sizeof s - 1, "batdraw %d", ps_hw_batchdrawtime);
					V_DrawThinString(90, 54, V_MONOSPACE | V_REDMAP, s);

					snprintf(s, sizeof s - 1, "polygon %d", ps_hw_numpolys);
					V_DrawThinString(155, 10, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "drwcall %d", ps_hw_numcalls);
					V_DrawThinString(155, 18, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "shaders %d", ps_hw_numshaders);
					V_DrawThinString(155, 26, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "vertex  %d", ps_hw_numverts);
					V_DrawThinString(155, 34, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "texture %d", ps_hw_numtextures);
					V_DrawThinString(220, 10, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "polyflg %d", ps_hw_numpolyflags);
					V_DrawThinString(220, 18, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "colors  %d", ps_hw_numcolors);
					V_DrawThinString(220, 26, V_MONOSPACE | V_PURPLEMAP, s);
				}
				else
				{
					// reset these vars so the "other" measurement isn't off
					ps_hw_batchsorttime = 0;
					ps_hw_batchdrawtime = 0;
				}
			}
			else // software specific stats
#endif
			{
				snprintf(s, sizeof s - 1, "bsptime %d", ps_bsptime);
				V_DrawThinString(24, 26, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "sprclip %d", ps_sw_spritecliptime);
				V_DrawThinString(24, 34, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "portals %d", ps_sw_portaltime);
				V_DrawThinString(24, 42, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "planes  %d", ps_sw_planetime);
				V_DrawThinString(24, 50, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "masked  %d", ps_sw_maskedtime);
				V_DrawThinString(24, 58, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "other   %d",
					ps_rendercalltime - ps_bsptime - ps_sw_spritecliptime
					- ps_sw_portaltime - ps_sw_planetime - ps_sw_maskedtime);
				V_DrawThinString(24, 66, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "ui      %d", ps_uitime);
				V_DrawThinString(20, 74, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "finupdt %d", ps_swaptime);
				V_DrawThinString(20, 82, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "logic   %d", ps_tictime);
				V_DrawThinString(20, 94, V_MONOSPACE | V_GRAYMAP, s);
			}
		}
		else // high resolution
		{
			snprintf(s, sizeof s - 1, "Frame time:     %d", frametime);
			V_DrawSmallString(20, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
			{
				snprintf(s, sizeof s - 1, "UI render:      %d", ps_uitime);
				V_DrawSmallString(20, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "I_FinishUpdate: %d", ps_swaptime);
				V_DrawSmallString(20, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Game logic:     %d", ps_tictime);
				V_DrawSmallString(20, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
				return;
			}
			snprintf(s, sizeof s - 1, "3d rendering:   %d", ps_rendercalltime);
			V_DrawSmallString(20, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "BSP calls:    %d", ps_numbspcalls);
			V_DrawSmallString(115, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Sprites:      %d", ps_numsprites);
			V_DrawSmallString(115, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Drawnodes:    %d", ps_numdrawnodes);
			V_DrawSmallString(115, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Polyobjects:  %d", ps_numpolyobjects);
			V_DrawSmallString(115, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
#ifdef HWRENDER
			if (rendermode == render_opengl) // OpenGL specific stats
			{
				snprintf(s, sizeof s - 1, "Skybox render:  %d", ps_hw_skyboxtime);
				V_DrawSmallString(24, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "RenderBSPNode:  %d", ps_bsptime);
				V_DrawSmallString(24, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Drwnode sort:   %d", ps_hw_nodesorttime);
				V_DrawSmallString(24, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Drwnode render: %d", ps_hw_nodedrawtime);
				V_DrawSmallString(24, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Sprite sort:    %d", ps_hw_spritesorttime);
				V_DrawSmallString(24, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Sprite render:  %d", ps_hw_spritedrawtime);
				V_DrawSmallString(24, 45, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				// Remember to update this calculation when adding more 3d rendering stats!
				snprintf(s, sizeof s - 1, "Other:          %d",
					ps_rendercalltime - ps_hw_skyboxtime - ps_bsptime - ps_hw_nodesorttime
					- ps_hw_nodedrawtime - ps_hw_spritesorttime - ps_hw_spritedrawtime
					- ps_hw_batchsorttime - ps_hw_batchdrawtime);
				V_DrawSmallString(24, 50, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "UI render:      %d", ps_uitime);
				V_DrawSmallString(20, 55, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "I_FinishUpdate: %d", ps_swaptime);
				V_DrawSmallString(20, 60, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Game logic:     %d", ps_tictime);
				V_DrawSmallString(20, 70, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
				if (cv_glbatching.value)
				{
					snprintf(s, sizeof s - 1, "Batch sort:   %d", ps_hw_batchsorttime);
					V_DrawSmallString(115, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_REDMAP, s);
					snprintf(s, sizeof s - 1, "Batch render: %d", ps_hw_batchdrawtime);
					V_DrawSmallString(115, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_REDMAP, s);

					snprintf(s, sizeof s - 1, "Polygons:   %d", ps_hw_numpolys);
					V_DrawSmallString(200, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Vertices:   %d", ps_hw_numverts);
					V_DrawSmallString(200, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Draw calls: %d", ps_hw_numcalls);
					V_DrawSmallString(200, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Shaders:    %d", ps_hw_numshaders);
					V_DrawSmallString(200, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Textures:   %d", ps_hw_numtextures);
					V_DrawSmallString(200, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Polyflags:  %d", ps_hw_numpolyflags);
					V_DrawSmallString(200, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Colors:     %d", ps_hw_numcolors);
					V_DrawSmallString(200, 45, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
				}
				else
				{
					// reset these vars so the "other" measurement isn't off
					ps_hw_batchsorttime = 0;
					ps_hw_batchdrawtime = 0;
				}
			}
			else // software specific stats
#endif
			{
				snprintf(s, sizeof s - 1, "RenderBSPNode:  %d", ps_bsptime);
				V_DrawSmallString(24, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "R_ClipSprites:  %d", ps_sw_spritecliptime);
				V_DrawSmallString(24, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Portals+Skybox: %d", ps_sw_portaltime);
				V_DrawSmallString(24, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "R_DrawPlanes:   %d", ps_sw_planetime);
				V_DrawSmallString(24, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "R_DrawMasked:   %d", ps_sw_maskedtime);
				V_DrawSmallString(24, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				// Remember to update this calculation when adding more 3d rendering stats!
				snprintf(s, sizeof s - 1, "Other:          %d",
					ps_rendercalltime - ps_bsptime - ps_sw_spritecliptime
					- ps_sw_portaltime - ps_sw_planetime - ps_sw_maskedtime);
				V_DrawSmallString(24, 45, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "UI render:      %d", ps_uitime);
				V_DrawSmallString(20, 50, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "I_FinishUpdate: %d", ps_swaptime);
				V_DrawSmallString(20, 55, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Game logic:     %d", ps_tictime);
				V_DrawSmallString(20, 65, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
			}
		}
	}
	else if (cv_perfstats.value == 2) // logic
	{
		int i = 0;
		thinker_t *thinker;
		int thinkercount = 0;
		int polythcount = 0;
		int mainthcount = 0;
		int mobjcount = 0;
		int nothinkcount = 0;
		int scenerycount = 0;
		int dynslopethcount = 0;
		int precipcount = 0;
		int removecount = 0;
		// y offset for drawing columns
		int yoffset1 = 0;
		int yoffset2 = 0;

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
					}
				}
				else if (i == THINK_DYNSLOPE)
					dynslopethcount++;
				else if (i == THINK_PRECIP)
					precipcount++;
			}
		}

		if (vid.width < 640 || vid.height < 400) // low resolution
		{
			snprintf(s, sizeof s - 1, "logic   %d", ps_tictime);
			V_DrawThinString(20, 10, V_MONOSPACE | V_YELLOWMAP, s);
			if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
				return;
			snprintf(s, sizeof s - 1, "plrthnk %d", ps_playerthink_time);
			V_DrawThinString(24, 18, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "thnkers %d", ps_thinkertime);
			V_DrawThinString(24, 26, V_MONOSPACE | V_YELLOWMAP, s);
			for (i = 0; i < NUM_THINKERLISTS; i++)
			{
				yoffset1 += 8;
				snprintf(s, sizeof s - 1, thlist_shortnames[i], ps_thlist_times[i]);
				V_DrawThinString(28, 26+yoffset1, V_MONOSPACE | V_YELLOWMAP, s);
			}
			snprintf(s, sizeof s - 1, "lthinkf %d", ps_lua_thinkframe_time);
			V_DrawThinString(24, 34+yoffset1, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "other   %d",
				ps_tictime - ps_playerthink_time - ps_thinkertime - ps_lua_thinkframe_time);
			V_DrawThinString(24, 42+yoffset1, V_MONOSPACE | V_YELLOWMAP, s);

			snprintf(s, sizeof s - 1, "thnkers %d", thinkercount);
			V_DrawThinString(90, 10, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "plyobjs %d", polythcount);
			V_DrawThinString(94, 18, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "main    %d", mainthcount);
			V_DrawThinString(94, 26, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "mobjs   %d", mobjcount);
			V_DrawThinString(94, 34, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "regular %d", mobjcount - scenerycount - nothinkcount);
			V_DrawThinString(98, 42, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "scenery %d", scenerycount);
			V_DrawThinString(98, 50, V_MONOSPACE | V_BLUEMAP, s);
			if (nothinkcount)
			{
				snprintf(s, sizeof s - 1, "nothink %d", nothinkcount);
				V_DrawThinString(98, 58, V_MONOSPACE | V_BLUEMAP, s);
				yoffset2 += 8;
			}
			snprintf(s, sizeof s - 1, "dynslop %d", dynslopethcount);
			V_DrawThinString(94, 58+yoffset2, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "precip  %d", precipcount);
			V_DrawThinString(94, 66+yoffset2, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "remove  %d", removecount);
			V_DrawThinString(94, 74+yoffset2, V_MONOSPACE | V_BLUEMAP, s);

			snprintf(s, sizeof s - 1, "lmhooks %d", ps_lua_mobjhooks);
			V_DrawThinString(170, 10, V_MONOSPACE | V_PURPLEMAP, s);
			snprintf(s, sizeof s - 1, "chkpos  %d", ps_checkposition_calls);
			V_DrawThinString(170, 18, V_MONOSPACE | V_PURPLEMAP, s);
		}
		else // high resolution
		{
			snprintf(s, sizeof s - 1, "Game logic:      %d", ps_tictime);
			V_DrawSmallString(20, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			if (!(gamestate == GS_LEVEL || (gamestate == GS_TITLESCREEN && titlemapinaction)))
				return;
			snprintf(s, sizeof s - 1, "P_PlayerThink:   %d", ps_playerthink_time);
			V_DrawSmallString(24, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "P_RunThinkers:   %d", ps_thinkertime);
			V_DrawSmallString(24, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			for (i = 0; i < NUM_THINKERLISTS; i++)
			{
				yoffset1 += 5;
				snprintf(s, sizeof s - 1, thlist_names[i], ps_thlist_times[i]);
				V_DrawSmallString(28, 20+yoffset1, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			}
			snprintf(s, sizeof s - 1, "LUAh_ThinkFrame: %d", ps_lua_thinkframe_time);
			V_DrawSmallString(24, 25+yoffset1, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "Other:           %d",
				ps_tictime - ps_playerthink_time - ps_thinkertime - ps_lua_thinkframe_time);
			V_DrawSmallString(24, 30+yoffset1, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);

			snprintf(s, sizeof s - 1, "Thinkers:        %d", thinkercount);
			V_DrawSmallString(115, 10+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Polyobjects:     %d", polythcount);
			V_DrawSmallString(119, 15+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Main:            %d", mainthcount);
			V_DrawSmallString(119, 20+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Mobjs:           %d", mobjcount);
			V_DrawSmallString(119, 25+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Regular:         %d", mobjcount - scenerycount - nothinkcount);
			V_DrawSmallString(123, 30+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Scenery:         %d", scenerycount);
			V_DrawSmallString(123, 35+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			if (nothinkcount)
			{
				snprintf(s, sizeof s - 1, "Nothink:         %d", nothinkcount);
				V_DrawSmallString(123, 40+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
				yoffset2 += 5;
			}
			snprintf(s, sizeof s - 1, "Dynamic slopes:  %d", dynslopethcount);
			V_DrawSmallString(119, 40+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Precipitation:   %d", precipcount);
			V_DrawSmallString(119, 45+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Pending removal: %d", removecount);
			V_DrawSmallString(119, 50+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);

			snprintf(s, sizeof s - 1, "Calls:");
			V_DrawSmallString(212, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
			snprintf(s, sizeof s - 1, "Lua mobj hooks:  %d", ps_lua_mobjhooks);
			V_DrawSmallString(216, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
			snprintf(s, sizeof s - 1, "P_CheckPosition: %d", ps_checkposition_calls);
			V_DrawSmallString(216, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
		}
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
				snprintf(s, sizeof s - 1, "%20s: %u", str, thinkframe_hooks[i].time_taken);
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
