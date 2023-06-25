// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.c
/// \brief Handles multiple resolutions, 8bpp/16bpp(highcolor) modes

#include "doomdef.h"
#include "doomstat.h"
#include "screen.h"
#include "console.h"
#include "am_map.h"
#include "i_time.h"
#include "i_system.h"
#include "i_video.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"
#include "m_misc.h"
#include "v_video.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "d_main.h"
#include "d_clisrv.h"
#include "f_finale.h"
#include "y_inter.h" // usebuffer
#include "i_sound.h" // closed captions
#include "s_sound.h" // ditto
#include "g_game.h" // ditto
#include "p_local.h" // P_AutoPause()

#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#include "hardware/hw_model.h"
#endif

// SRB2Kart
#include "r_fps.h" // R_GetFramerateCap

#if defined (USEASM) && !defined (NORUSEASM)//&& (!defined (_MSC_VER) || (_MSC_VER <= 1200))
#define RUSEASM //MSC.NET can't patch itself
#endif

// -----------------------------
// assembly or c drawer routines
// -----------------------------
void (*colfunc)(void);
void (*colfuncs[COLUMN_MAX])(void);

INT32 column_translu;
INT32 column_translu_mapped;
INT32 column_translu_multipatch;

void (*spanfunc)(void);
void (*spanfuncs[SPAN_MAX])(void);
void (*spanfuncs_npo2[SPAN_MAX])(void);

INT32 span_translu;
INT32 span_translu_tilted;
INT32 span_translu_splat;
INT32 span_translu_solidcolor;
INT32 span_translu_tilted_solidcolor;
INT32 span_translu_sprite;
INT32 span_translu_sprite_tilted;
INT32 span_water;
INT32 span_water_tilted;
INT32 span_water_solidcolor;
INT32 span_water_tilted_solidcolor;

// ------------------
// global video state
// ------------------
viddef_t vid;
INT32 setmodeneeded; //video mode change needed if > 0 (the mode number to set + 1)
INT32 setrenderneeded;

static CV_PossibleValue_t scr_depth_cons_t[] = {{8, "8 bits"}, {16, "16 bits"}, {24, "24 bits"}, {32, "32 bits"}, {0, NULL}};

//added : 03-02-98: default screen mode, as loaded/saved in config
consvar_t cv_scr_width = CVAR_INIT ("scr_width", "1280", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_height = CVAR_INIT ("scr_height", "800", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_scr_depth = CVAR_INIT ("scr_depth", "16 bits", CV_SAVE, scr_depth_cons_t, NULL);

consvar_t cv_renderview = CVAR_INIT ("renderview", "On", 0, CV_OnOff, NULL);

CV_PossibleValue_t cv_renderer_t[] = {
	{1, "Software"},
	{2, "Software (truecolor)"},
#ifdef HWRENDER
	{3, "OpenGL"},
#endif
	{0, NULL}
};

consvar_t cv_renderer = CVAR_INIT ("renderer", "Software", CV_SAVE|CV_CALL, cv_renderer_t, SCR_ChangeRenderer);

static void SCR_ChangeFullscreen(void);

consvar_t cv_fullscreen = CVAR_INIT ("fullscreen", "Yes", CV_SAVE|CV_CALL, CV_YesNo, SCR_ChangeFullscreen);

// =========================================================================
//                           SCREEN VARIABLES
// =========================================================================

INT32 scr_bpp; // current video mode bytes per pixel
UINT8 *scr_borderpatch; // flat used to fill the reduced view borders set at ST_Init()

// =========================================================================

//  Short and Tall sky drawer, for the current color mode
void (*walldrawerfunc)(void);

boolean R_ASM = true;
boolean R_486 = false;
boolean R_586 = false;
boolean R_MMX = false;
boolean R_SSE = false;
boolean R_3DNow = false;
boolean R_MMXExt = false;
boolean R_SSE2 = false;

void SCR_SetDrawFuncs(void)
{
	if (vid.bpp == 1)
	{
		colfuncs[BASEDRAWFUNC] = R_DrawColumn_8;
		spanfuncs[BASEDRAWFUNC] = R_DrawSpan_8;

		colfunc = colfuncs[BASEDRAWFUNC];
		spanfunc = spanfuncs[BASEDRAWFUNC];

#define COLFUNC8(type, func) colfuncs[COLUMN_##type] = R_Draw##func##Column_8
		COLFUNCLIST8(COLFUNC8);
		COLFUNCLIST8_NOTEXTURE(COLFUNC8);
#undef COLFUNC8

#define SPANFUNC8(type, func) spanfuncs[SPAN_##type] = R_Draw##func##_8
		SPANFUNCLIST8(SPANFUNC8);
		SPANFUNCLIST8_NOTEXTURE(SPANFUNC8);
#undef SPANFUNC8

#define SPANFUNC8(type, func) spanfuncs_npo2[SPAN_##type] = R_Draw##func##_NPO2_8
		SPANFUNCLIST8(SPANFUNC8);
#undef SPANFUNC8

#ifdef RUSEASM
		if (R_ASM)
		{
			if (R_MMX)
			{
				colfuncs[BASEDRAWFUNC] = R_DrawColumn_8_MMX;
				colfuncs[COLUMN_MULTIPATCH] = R_Draw2sMultiPatchColumn_8_MMX;
				spanfuncs[BASEDRAWFUNC] = R_DrawSpan_8_MMX;
			}
			else
			{
				colfuncs[BASEDRAWFUNC] = R_DrawColumn_8_ASM;
				colfuncs[COLUMN_MULTIPATCH] = R_Draw2sMultiPatchColumn_8_ASM;
			}
		}
#endif
	}
#ifdef TRUECOLOR
	else if (vid.bpp == 4)
	{
		colfuncs[BASEDRAWFUNC] = R_DrawColumn_32;
		spanfuncs[BASEDRAWFUNC] = R_DrawSpan_32;

		colfunc = colfuncs[BASEDRAWFUNC];
		spanfunc = spanfuncs[BASEDRAWFUNC];

#define COLFUNC32(type, func) colfuncs[COLUMN_##type] = R_Draw##func##Column_32
		COLFUNCLIST32(COLFUNC32);
		COLFUNCLIST32_NOTEXTURE(COLFUNC32);
#undef COLFUNC32

#define SPANFUNC32(type, func) spanfuncs[SPAN_##type] = R_Draw##func##_32
		SPANFUNCLIST32(SPANFUNC32);
		SPANFUNCLIST32_NOTEXTURE(SPANFUNC32);
#undef SPANFUNC32

#define SPANFUNC32(type, func) spanfuncs_npo2[SPAN_##type] = R_Draw##func##_NPO2_32
		SPANFUNCLIST32(SPANFUNC32);
#undef SPANFUNC8
	}
#endif
	else
		I_Error("unknown bytes per pixel mode %d\n", vid.bpp);
}

#undef SPANFUNCLIST

void SCR_SetSoftwareTranslucency(void)
{
	if (truecolor)
		usetranstables = false;
	else
		usetranstables = cv_transtables.value;

	if (usetranstables)
	{
		column_translu = COLUMN_TRANSTAB;
		column_translu_mapped = COLUMN_MAPPED_TRANSTAB;
		column_translu_multipatch = COLUMN_MULTIPATCH_TRANSTAB;

		span_translu = SPAN_TRANSTAB;
		span_translu_tilted = SPAN_TILTED_TRANSTAB;
		span_translu_splat = SPAN_SPLAT_TRANSTAB;
		span_translu_solidcolor = SPAN_SOLIDCOLOR_TRANSTAB;
		span_translu_tilted_solidcolor = SPAN_TILTED_SOLIDCOLOR_TRANSTAB;
		span_translu_sprite = SPAN_SPRITE_TRANSTAB;
		span_translu_sprite_tilted = SPAN_SPRITE_TILTED_TRANSTAB;
		span_water = SPAN_WATER_TRANSTAB;
		span_water_tilted = SPAN_WATER_TILTED_TRANSTAB;
		span_water_solidcolor = SPAN_WATER_SOLIDCOLOR_TRANSTAB;
		span_water_tilted_solidcolor = SPAN_WATER_TILTED_SOLIDCOLOR_TRANSTAB;
	}
	else
	{
		column_translu = COLUMN_ALPHA;
		column_translu_mapped = COLUMN_MAPPED_ALPHA;
		column_translu_multipatch = COLUMN_MULTIPATCH_ALPHA;

		span_translu = SPAN_ALPHA;
		span_translu_tilted = SPAN_TILTED_ALPHA;
		span_translu_splat = SPAN_SPLAT_ALPHA;
		span_translu_solidcolor = SPAN_SOLIDCOLOR_ALPHA;
		span_translu_tilted_solidcolor = SPAN_TILTED_SOLIDCOLOR_ALPHA;
		span_translu_sprite = SPAN_SPRITE_ALPHA;
		span_translu_sprite_tilted = SPAN_SPRITE_TILTED_ALPHA;
		span_water = SPAN_WATER_ALPHA;
		span_water_tilted = SPAN_WATER_TILTED_ALPHA;
		span_water_solidcolor = SPAN_WATER_SOLIDCOLOR_ALPHA;
		span_water_tilted_solidcolor = SPAN_WATER_TILTED_SOLIDCOLOR_ALPHA;
	}
}

void SCR_SetMode(void)
{
	if (dedicated)
		return;

	if (!(setmodeneeded || setrenderneeded) || WipeInAction)
		return; // should never happen and don't change it during a wipe, BAD!

	// Lactozilla: Renderer switching
	if (setrenderneeded)
	{
		// stop recording movies (APNG only)
		if (setrenderneeded && (moviemode == MM_APNG))
			M_StopMovie();

		// VID_SetMode will call VID_CheckRenderer itself,
		// so no need to do this in here.
		if (!setmodeneeded)
			VID_CheckRenderer();

		vid.recalc = 1;
	}

	// Set the video mode in the video interface.
	if (setmodeneeded)
		VID_SetMode(setmodeneeded - 1);

	V_SetPalette(0);

	SCR_SetDrawFuncs();

	// set the apprpriate drawer for the sky (tall or INT16)
	setmodeneeded = 0;
	setrenderneeded = 0;
}

// do some initial settings for the game loading screen
//
void SCR_Startup(void)
{
	const CPUInfoFlags *RCpuInfo = I_CPUInfo();
	if (!M_CheckParm("-NOCPUID") && RCpuInfo)
	{
#if defined (__i386__) || defined (_M_IX86) || defined (__WATCOMC__)
		R_486 = true;
#endif
		if (RCpuInfo->RDTSC)
			R_586 = true;
		if (RCpuInfo->MMX)
			R_MMX = true;
		if (RCpuInfo->AMD3DNow)
			R_3DNow = true;
		if (RCpuInfo->MMXExt)
			R_MMXExt = true;
		if (RCpuInfo->SSE)
			R_SSE = true;
		if (RCpuInfo->SSE2)
			R_SSE2 = true;
		CONS_Printf("CPU Info: 486: %i, 586: %i, MMX: %i, 3DNow: %i, MMXExt: %i, SSE2: %i\n", R_486, R_586, R_MMX, R_3DNow, R_MMXExt, R_SSE2);
	}

	if (M_CheckParm("-noASM"))
		R_ASM = false;
	if (M_CheckParm("-486"))
		R_486 = true;
	if (M_CheckParm("-586"))
		R_586 = true;
	if (M_CheckParm("-MMX"))
		R_MMX = true;
	if (M_CheckParm("-3DNow"))
		R_3DNow = true;
	if (M_CheckParm("-MMXExt"))
		R_MMXExt = true;

	if (M_CheckParm("-SSE"))
		R_SSE = true;
	if (M_CheckParm("-noSSE"))
		R_SSE = false;

	if (M_CheckParm("-SSE2"))
		R_SSE2 = true;

	M_SetupMemcpy();

	if (dedicated)
	{
		V_Init();
		V_SetPalette(0);
		return;
	}

	vid.modenum = 0;

	V_Init();
	V_Recalc();

	CV_RegisterVar(&cv_ticrate);
	CV_RegisterVar(&cv_constextsize);

	V_SetPalette(0);
}

// Called at new frame, if the video mode has changed
//
void SCR_Recalc(void)
{
	if (dedicated)
		return;

	// bytes per pixel quick access
	scr_bpp = vid.bpp;

	V_Recalc();

	// toggle off (then back on) the automap because some screensize-dependent values will
	// be calculated next time the automap is activated.
	if (automapactive)
	{
		am_recalc = true;
		AM_Start();
	}

	// set the screen[x] ptrs on the new vidbuffers
	V_Init();

	// scr_viewsize doesn't change, neither detailLevel, but the pixels
	// per screenblock is different now, since we've changed resolution.
	R_SetViewSize(); //just set setsizeneeded true now ..

	// vid.recalc lasts only for the next refresh...
	con_recalc = true;
	am_recalc = true;

#ifdef HWRENDER
	// Shoot! The screen texture was flushed!
	if ((rendermode == render_opengl) && (gamestate == GS_INTERMISSION))
		usebuffer = false;
#endif
}

// Check for screen cmd-line parms: to force a resolution.
//
// Set the video mode to set at the 1st display loop (setmodeneeded)
//

void SCR_CheckDefaultMode(void)
{
	INT32 scr_forcex, scr_forcey; // resolution asked from the cmd-line

	if (dedicated)
		return;

	// 0 means not set at the cmd-line
	scr_forcex = scr_forcey = 0;

	if (M_CheckParm("-width") && M_IsNextParm())
		scr_forcex = atoi(M_GetNextParm());

	if (M_CheckParm("-height") && M_IsNextParm())
		scr_forcey = atoi(M_GetNextParm());

	if (scr_forcex && scr_forcey)
	{
		CONS_Printf(M_GetText("Using resolution: %d x %d\n"), scr_forcex, scr_forcey);
		// returns -1 if not found, thus will be 0 (no mode change) if not found
		setmodeneeded = VID_GetModeForSize(scr_forcex, scr_forcey) + 1;
	}
	else
	{
		CONS_Printf(M_GetText("Default resolution: %d x %d (%d bits)\n"), cv_scr_width.value,
			cv_scr_height.value, cv_scr_depth.value);
		// see note above
		setmodeneeded = VID_GetModeForSize(cv_scr_width.value, cv_scr_height.value) + 1;
	}

	if (cv_renderer.value != (signed)rendermode)
	{
		if (chosenrendermode == render_none) // nothing set at command line
			SCR_ChangeRenderer();
		else
		{
			// Set cv_renderer to the current render mode
			CV_StealthSetValue(&cv_renderer, rendermode);
		}
	}
}

// sets the modenum as the new default video mode to be saved in the config file
void SCR_SetDefaultMode(void)
{
	// remember the default screen size
	CV_SetValue(&cv_scr_width, vid.width);
	CV_SetValue(&cv_scr_height, vid.height);
}

// Change fullscreen on/off according to cv_fullscreen
void SCR_ChangeFullscreen(void)
{
#ifdef DIRECTFULLSCREEN
	// allow_fullscreen is set by VID_PrepareModeList
	// it is used to prevent switching to fullscreen during startup
	if (!allow_fullscreen)
		return;

	if (graphics_started)
	{
		VID_PrepareModeList();
		setmodeneeded = VID_GetModeForSize(vid.width, vid.height) + 1;
	}
#endif
}

void SCR_ChangeRenderer(void)
{
	if (chosenrendermode != render_none
	|| (signed)rendermode == cv_renderer.value)
		return;

#ifdef HWRENDER
	// Check if OpenGL loaded successfully (or wasn't disabled) before switching to it.
	if ((vid.glstate == VID_GL_LIBRARY_ERROR)
	&& (cv_renderer.value == render_opengl))
	{
		if (M_CheckParm("-nogl"))
			CONS_Alert(CONS_ERROR, "OpenGL rendering was disabled!\n");
		else
			CONS_Alert(CONS_ERROR, "OpenGL never loaded\n");
		return;
	}

	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED)) // Clear these out before switching to software
		HWR_ClearAllTextures();

#endif

	// Set the new render mode
	setrenderneeded = cv_renderer.value;
}

boolean SCR_IsAspectCorrect(INT32 width, INT32 height)
{
	return
	 (  width % BASEVIDWIDTH == 0
	 && height % BASEVIDHEIGHT == 0
	 && width / BASEVIDWIDTH == height / BASEVIDHEIGHT
	 );
}

double averageFPS = 0.0f;

#define USE_FPS_SAMPLES

#ifdef USE_FPS_SAMPLES
#define FPS_SAMPLE_RATE (0.05) // How often to update FPS samples, in seconds
#define NUM_FPS_SAMPLES (16) // Number of samples to store

static double fps_samples[NUM_FPS_SAMPLES];
static double updateElapsed = 0.0;
#endif

static boolean fps_init = false;
static precise_t fps_enter = 0;

void SCR_CalculateFPS(void)
{
	precise_t fps_finish = 0;

	double frameElapsed = 0.0;

	if (fps_init == false)
	{
		fps_enter = I_GetPreciseTime();
		fps_init = true;
	}

	fps_finish = I_GetPreciseTime();
	frameElapsed = (double)((INT64)(fps_finish - fps_enter)) / I_GetPrecisePrecision();
	fps_enter = fps_finish;

#ifdef USE_FPS_SAMPLES
	updateElapsed += frameElapsed;

	if (updateElapsed >= FPS_SAMPLE_RATE)
	{
		static int sampleIndex = 0;
		int i;

		fps_samples[sampleIndex] = frameElapsed;

		sampleIndex++;
		if (sampleIndex >= NUM_FPS_SAMPLES)
			sampleIndex = 0;

		averageFPS = 0.0;
		for (i = 0; i < NUM_FPS_SAMPLES; i++)
		{
			averageFPS += fps_samples[i];
		}

		if (averageFPS > 0.0)
		{
			averageFPS = 1.0 / (averageFPS / NUM_FPS_SAMPLES);
		}
	}

	while (updateElapsed >= FPS_SAMPLE_RATE)
	{
		updateElapsed -= FPS_SAMPLE_RATE;
	}
#else
	// Direct, unsampled counter.
	averageFPS = 1.0 / frameElapsed;
#endif
}

void SCR_DisplayTicRate(void)
{
	INT32 ticcntcolor = 0;
	const INT32 h = vid.height-(8*vid.dupy);
	UINT32 cap = R_GetFramerateCap();
	double fps = round(averageFPS);

	if (gamestate == GS_NULL)
		return;

	if (cap > 0)
	{
		if (fps <= cap / 2.0) ticcntcolor = V_REDMAP;
		else if (fps <= cap * 0.90) ticcntcolor = V_YELLOWMAP;
		else ticcntcolor = V_GREENMAP;
	}
	else
	{
		ticcntcolor = V_GREENMAP;
	}

	if (cv_ticrate.value == 2) // compact counter
	{
		V_DrawRightAlignedString(vid.width, h,
			ticcntcolor|V_NOSCALESTART|V_USERHUDTRANS, va("%04.2f", averageFPS)); // use averageFPS directly
	}
	else if (cv_ticrate.value == 1) // full counter
	{
		const char *drawnstr;
		INT32 width;

		// The highest assignable cap is < 1000, so 3 characters is fine.
		if (cap > 0)
			drawnstr = va("%3.0f/%3u", fps, cap);
		else
			drawnstr = va("%4.2f", averageFPS);

		width = V_StringWidth(drawnstr, V_NOSCALESTART);

		V_DrawString(vid.width - ((7 * 8 * vid.dupx) + V_StringWidth("FPS: ", V_NOSCALESTART)), h,
			V_YELLOWMAP|V_NOSCALESTART|V_USERHUDTRANS, "FPS:");
		V_DrawString(vid.width - width, h,
			ticcntcolor|V_NOSCALESTART|V_USERHUDTRANS, drawnstr);
	}
}

void SCR_DisplayLocalPing(void)
{
	UINT32 ping = playerpingtable[consoleplayer];	// consoleplayer's ping is everyone's ping in a splitnetgame :P
	if (cv_showping.value == 1 || (cv_showping.value == 2 && servermaxping && ping > servermaxping))	// only show 2 (warning) if our ping is at a bad level
	{
		INT32 dispy = cv_ticrate.value ? 180 : 189;
		HU_drawPing(307, dispy, ping, true, V_SNAPTORIGHT | V_SNAPTOBOTTOM);
	}
}


void SCR_ClosedCaptions(void)
{
	UINT8 i;
	boolean gamestopped = (paused || P_AutoPause());
	INT32 basey = BASEVIDHEIGHT;

	if (gamestate != wipegamestate)
		return;

	if (gamestate == GS_LEVEL)
	{
		if (promptactive)
			basey -= 42;
		else if (splitscreen)
			basey -= 8;
		else if ((modeattacking == ATTACKING_NIGHTS)
		|| (!(maptol & TOL_NIGHTS)
		&& ((cv_powerupdisplay.value == 2) // "Always"
		 || (cv_powerupdisplay.value == 1 && !camera.chase)))) // "First-person only"
			basey -= 16;
	}

	for (i = 0; i < NUMCAPTIONS; i++)
	{
		INT32 flags, y;
		char dot;
		boolean music;

		if (!closedcaptions[i].s)
			continue;

		music = (closedcaptions[i].s-S_sfx == sfx_None);

		if (music && !gamestopped && (closedcaptions[i].t < flashingtics) && (closedcaptions[i].t & 1))
			continue;

		flags = V_SNAPTORIGHT|V_SNAPTOBOTTOM|V_ALLOWLOWERCASE;
		y = basey-((i + 2)*10);

		if (closedcaptions[i].b)
		{
			y -= closedcaptions[i].b * vid.dupy;
			if (renderisnewtic)
			{
				closedcaptions[i].b--;
			}
		}

		if (closedcaptions[i].t < CAPTIONFADETICS)
			flags |= (((CAPTIONFADETICS-closedcaptions[i].t)/2)*V_10TRANS);

		if (music)
			dot = '\x19';
		else if (closedcaptions[i].c && closedcaptions[i].c->origin)
			dot = '\x1E';
		else
			dot = ' ';

		V_DrawRightAlignedString(BASEVIDWIDTH - 20, y, flags,
			va("%c [%s]", dot, (closedcaptions[i].s->caption[0] ? closedcaptions[i].s->caption : closedcaptions[i].s->name)));
	}
}

void SCR_DisplayMarathonInfo(void)
{
	INT32 flags = V_SNAPTOBOTTOM;
	static tic_t entertic, oldentertics = 0, antisplice[2] = {48,0};
	const char *str;
#if 0 // eh, this probably isn't going to be a problem
	if (((signed)marathontime) < 0)
	{
		flags |= V_REDMAP;
		str = "No waiting out the clock to submit a bogus time.";
	}
	else
#endif
	{
		entertic = I_GetTime();
		if (gamecomplete)
			flags |= V_YELLOWMAP;
		else if (marathonmode & MA_INGAME)
			; // see also G_Ticker
		else if (marathonmode & MA_INIT)
			marathonmode &= ~MA_INIT;
		else
			marathontime += entertic - oldentertics;

		// Create a sequence of primes such that their LCM is nice and big.
#define PRIMEV1 13
#define PRIMEV2 17 // I can't believe it! I'm on TV!
		antisplice[0] += (entertic - oldentertics)*PRIMEV2;
		antisplice[0] %= PRIMEV1*((vid.width/vid.dupx)+1);
		antisplice[1] += (entertic - oldentertics)*PRIMEV1;
		antisplice[1] %= PRIMEV1*((vid.width/vid.dupx)+1);
		str = va("%i:%02i:%02i.%02i",
			G_TicsToHours(marathontime),
			G_TicsToMinutes(marathontime, false),
			G_TicsToSeconds(marathontime),
			G_TicsToCentiseconds(marathontime));
		oldentertics = entertic;
	}
	V_DrawFill((antisplice[0]/PRIMEV1)-1, BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT);
	V_DrawFill((antisplice[0]/PRIMEV1),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTOLEFT|31);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)-1), BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT);
	V_DrawFill(BASEVIDWIDTH-((antisplice[1]/PRIMEV1)),   BASEVIDHEIGHT-8, 1, 8, V_SNAPTOBOTTOM|V_SNAPTORIGHT|31);
#undef PRIMEV1
#undef PRIMEV2
	V_DrawPromptBack(-8, cons_backcolor.value);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT-8, flags, str);
}
