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
/// \file  st_stuff.c
/// \brief Status bar code
///        Does the face/direction indicator animatin.
///        Does palette indicators as well (red pain/berserk, bright pickup)

#include "doomdef.h"
#include "g_game.h"
#include "r_local.h"
#include "p_local.h"
#include "f_finale.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "z_zone.h"
#include "hu_stuff.h"
#include "console.h"
#include "s_sound.h"
#include "i_system.h"
#include "m_menu.h"
#include "m_cheat.h"
#include "m_misc.h" // moviemode
#include "m_anigif.h" // cv_gif_downscale
#include "p_setup.h" // NiGHTS grading

//random index
#include "m_random.h"

// item finder
#include "m_cond.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "lua_hud.h"

UINT16 objectsdrawn = 0;

//
// STATUS BAR DATA
//

patch_t *faceprefix[MAXSKINS]; // face status patches
patch_t *superprefix[MAXSKINS]; // super face status patches

// ------------------------------------------
//             status bar overlay
// ------------------------------------------

// icons for overlay
patch_t *sboscore; // Score logo
patch_t *sbotime; // Time logo
patch_t *sbocolon; // Colon for time
patch_t *sboperiod; // Period for time centiseconds
patch_t *livesback; // Lives icon background
patch_t *stlivex;
static patch_t *nrec_timer; // Timer for NiGHTS records
static patch_t *sborings;
static patch_t *slidgame;
static patch_t *slidtime;
static patch_t *slidover;
static patch_t *sboredrings;
static patch_t *sboredtime;
static patch_t *getall; // Special Stage HUD
static patch_t *timeup; // Special Stage HUD
static patch_t *hunthoming[6];
static patch_t *itemhoming[6];
static patch_t *race1;
static patch_t *race2;
static patch_t *race3;
static patch_t *racego;
static patch_t *nightslink;
static patch_t *curweapon;
static patch_t *normring;
static patch_t *bouncering;
static patch_t *infinityring;
static patch_t *autoring;
static patch_t *explosionring;
static patch_t *scatterring;
static patch_t *grenadering;
static patch_t *railring;
static patch_t *jumpshield;
static patch_t *forceshield;
static patch_t *ringshield;
static patch_t *watershield;
static patch_t *bombshield;
static patch_t *pityshield;
static patch_t *pinkshield;
static patch_t *flameshield;
static patch_t *bubbleshield;
static patch_t *thundershield;
static patch_t *invincibility;
static patch_t *sneakers;
static patch_t *gravboots;
static patch_t *nonicon;
static patch_t *nonicon2;
static patch_t *bluestat;
static patch_t *byelstat;
static patch_t *orngstat;
static patch_t *redstat;
static patch_t *yelstat;
static patch_t *nbracket;
static patch_t *nring;
static patch_t *nhud[12];
static patch_t *nsshud;
static patch_t *nbon[12];
static patch_t *nssbon;
static patch_t *narrow[9];
static patch_t *nredar[8]; // Red arrow
static patch_t *drillbar;
static patch_t *drillfill[3];
static patch_t *capsulebar;
static patch_t *capsulefill;
patch_t *ngradeletters[7];
static patch_t *minus5sec;
static patch_t *minicaps;
static patch_t *gotrflag;
static patch_t *gotbflag;
static patch_t *fnshico;

static boolean facefreed[MAXPLAYERS];

hudinfo_t hudinfo[NUMHUDITEMS] =
{
	{  16, 176, V_SNAPTOLEFT|V_SNAPTOBOTTOM}, // HUD_LIVES

	{  16,  42, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_RINGS
	{  96,  42, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_RINGSNUM
	{ 120,  42, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_RINGSNUMTICS

	{  16,  10, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_SCORE
	{ 120,  10, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_SCORENUM

	{  16,  26, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_TIME
	{  72,  26, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_MINUTES
	{  72,  26, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_TIMECOLON
	{  96,  26, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_SECONDS
	{  96,  26, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_TIMETICCOLON
	{ 120,  26, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_TICS

	{   0,  56, V_SNAPTOLEFT|V_SNAPTOTOP}, // HUD_SS_TOTALRINGS

	{ 110,  93, 0}, // HUD_GETRINGS
	{ 160,  93, 0}, // HUD_GETRINGSNUM
	{ 124, 160, 0}, // HUD_TIMELEFT
	{ 168, 176, 0}, // HUD_TIMELEFTNUM
	{ 130,  93, 0}, // HUD_TIMEUP
	{ 152, 168, 0}, // HUD_HUNTPICS

	{ 288, 176, V_SNAPTORIGHT|V_SNAPTOBOTTOM}, // HUD_POWERUPS
};

//
// STATUS BAR CODE
//

boolean ST_SameTeam(player_t *a, player_t *b)
{
	// Just pipe team messages to everyone in co-op or race.
	if (!G_RingSlingerGametype())
		return true;

	// Spectator chat.
	if (a->spectator && b->spectator)
		return true;

	// Team chat.
	if (G_GametypeHasTeams())
		return a->ctfteam == b->ctfteam;

	if (G_TagGametype())
		return ((a->pflags & PF_TAGIT) == (b->pflags & PF_TAGIT));

	return false;
}

static boolean st_stopped = true;

void ST_Ticker(boolean run)
{
	if (st_stopped)
		return;

	if (run)
		ST_runTitleCard();
}

// 0 is default, any others are special palettes.
INT32 st_palette = 0;
INT32 st_translucency = 10;

void ST_doPaletteStuff(void)
{
	INT32 palette;

	if (paused || P_AutoPause())
		palette = 0;
	else if (stplyr && stplyr->flashcount)
		palette = stplyr->flashpal;
	else
		palette = 0;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		palette = 0; // No flashpals here in OpenGL
#endif

	palette = min(max(palette, 0), 13);

	if (palette != st_palette)
	{
		st_palette = palette;

		if (rendermode != render_none)
		{
			V_SetPaletteLump(GetPalette()); // Reset the palette
			if (!splitscreen)
				V_SetPalette(palette);
		}
	}
}

void ST_UnloadGraphics(void)
{
	Z_FreeTag(PU_HUDGFX);
}

void ST_LoadGraphics(void)
{
	int i;

	// SRB2 border patch
	st_borderpatchnum = W_GetNumForName("GFZFLR01");
	scr_borderpatch = W_CacheLumpNum(st_borderpatchnum, PU_HUDGFX);

	// the original Doom uses 'STF' as base name for all face graphics
	// Graue 04-08-2004: face/name graphics are now indexed by skins
	//                   but load them in R_AddSkins, that gets called
	//                   first anyway
	// cache the status bar overlay icons (fullscreen mode)

	// Prefix "STT" is whitelisted (doesn't trigger ISGAMEMODIFIED), btw
	sborings = W_CachePatchName("STTRINGS", PU_HUDGFX);
	sboredrings = W_CachePatchName("STTRRING", PU_HUDGFX);
	sboscore = W_CachePatchName("STTSCORE", PU_HUDGFX);
	sbotime = W_CachePatchName("STTTIME", PU_HUDGFX); // Time logo
	sboredtime = W_CachePatchName("STTRTIME", PU_HUDGFX);
	sbocolon = W_CachePatchName("STTCOLON", PU_HUDGFX); // Colon for time
	sboperiod = W_CachePatchName("STTPERIO", PU_HUDGFX); // Period for time centiseconds

	slidgame = W_CachePatchName("SLIDGAME", PU_HUDGFX);
	slidtime = W_CachePatchName("SLIDTIME", PU_HUDGFX);
	slidover = W_CachePatchName("SLIDOVER", PU_HUDGFX);

	stlivex = W_CachePatchName("STLIVEX", PU_HUDGFX);
	livesback = W_CachePatchName("STLIVEBK", PU_HUDGFX);
	nrec_timer = W_CachePatchName("NGRTIMER", PU_HUDGFX); // Timer for NiGHTS
	getall = W_CachePatchName("GETALL", PU_HUDGFX); // Special Stage HUD
	timeup = W_CachePatchName("TIMEUP", PU_HUDGFX); // Special Stage HUD
	race1 = W_CachePatchName("RACE1", PU_HUDGFX);
	race2 = W_CachePatchName("RACE2", PU_HUDGFX);
	race3 = W_CachePatchName("RACE3", PU_HUDGFX);
	racego = W_CachePatchName("RACEGO", PU_HUDGFX);
	nightslink = W_CachePatchName("NGHTLINK", PU_HUDGFX);

	for (i = 0; i < 6; ++i)
	{
		hunthoming[i] = W_CachePatchName(va("HOMING%d", i+1), PU_HUDGFX);
		itemhoming[i] = W_CachePatchName(va("HOMITM%d", i+1), PU_HUDGFX);
	}

	curweapon = W_CachePatchName("CURWEAP", PU_HUDGFX);
	normring = W_CachePatchName("RINGIND", PU_HUDGFX);
	bouncering = W_CachePatchName("BNCEIND", PU_HUDGFX);
	infinityring = W_CachePatchName("INFNIND", PU_HUDGFX);
	autoring = W_CachePatchName("AUTOIND", PU_HUDGFX);
	explosionring = W_CachePatchName("BOMBIND", PU_HUDGFX);
	scatterring = W_CachePatchName("SCATIND", PU_HUDGFX);
	grenadering = W_CachePatchName("GRENIND", PU_HUDGFX);
	railring = W_CachePatchName("RAILIND", PU_HUDGFX);
	jumpshield = W_CachePatchName("TVWWICON", PU_HUDGFX);
	forceshield = W_CachePatchName("TVFOICON", PU_HUDGFX);
	ringshield = W_CachePatchName("TVATICON", PU_HUDGFX);
	watershield = W_CachePatchName("TVELICON", PU_HUDGFX);
	bombshield = W_CachePatchName("TVARICON", PU_HUDGFX);
	pityshield = W_CachePatchName("TVPIICON", PU_HUDGFX);
	pinkshield = W_CachePatchName("TVPPICON", PU_HUDGFX);
	flameshield = W_CachePatchName("TVFLICON", PU_HUDGFX);
	bubbleshield = W_CachePatchName("TVBBICON", PU_HUDGFX);
	thundershield = W_CachePatchName("TVZPICON", PU_HUDGFX);
	invincibility = W_CachePatchName("TVIVICON", PU_HUDGFX);
	sneakers = W_CachePatchName("TVSSICON", PU_HUDGFX);
	gravboots = W_CachePatchName("TVGVICON", PU_HUDGFX);

	tagico = W_CachePatchName("TAGICO", PU_HUDGFX);
	rflagico = W_CachePatchName("RFLAGICO", PU_HUDGFX);
	bflagico = W_CachePatchName("BFLAGICO", PU_HUDGFX);
	rmatcico = W_CachePatchName("RMATCICO", PU_HUDGFX);
	bmatcico = W_CachePatchName("BMATCICO", PU_HUDGFX);
	gotrflag = W_CachePatchName("GOTRFLAG", PU_HUDGFX);
	gotbflag = W_CachePatchName("GOTBFLAG", PU_HUDGFX);
	fnshico = W_CachePatchName("FNSHICO", PU_HUDGFX);
	nonicon = W_CachePatchName("NONICON", PU_HUDGFX);
	nonicon2 = W_CachePatchName("NONICON2", PU_HUDGFX);

	// NiGHTS HUD things
	bluestat = W_CachePatchName("BLUESTAT", PU_HUDGFX);
	byelstat = W_CachePatchName("BYELSTAT", PU_HUDGFX);
	orngstat = W_CachePatchName("ORNGSTAT", PU_HUDGFX);
	redstat = W_CachePatchName("REDSTAT", PU_HUDGFX);
	yelstat = W_CachePatchName("YELSTAT", PU_HUDGFX);
	nbracket = W_CachePatchName("NBRACKET", PU_HUDGFX);
	nring = W_CachePatchName("NRNG1", PU_HUDGFX);
	for (i = 0; i < 12; ++i)
	{
		nhud[i] = W_CachePatchName(va("NHUD%d", i+1), PU_HUDGFX);
		nbon[i] = W_CachePatchName(va("NBON%d", i+1), PU_HUDGFX);
	}
	nsshud = W_CachePatchName("NSSHUD", PU_HUDGFX);
	nssbon = W_CachePatchName("NSSBON", PU_HUDGFX);
	minicaps = W_CachePatchName("MINICAPS", PU_HUDGFX);

	for (i = 0; i < 8; ++i)
	{
		narrow[i] = W_CachePatchName(va("NARROW%d", i+1), PU_HUDGFX);
		nredar[i] = W_CachePatchName(va("NREDAR%d", i+1), PU_HUDGFX);
	}

	// non-animated version
	narrow[8] = W_CachePatchName("NARROW9", PU_HUDGFX);

	drillbar = W_CachePatchName("DRILLBAR", PU_HUDGFX);
	for (i = 0; i < 3; ++i)
		drillfill[i] = W_CachePatchName(va("DRILLFI%d", i+1), PU_HUDGFX);
	capsulebar = W_CachePatchName("CAPSBAR", PU_HUDGFX);
	capsulefill = W_CachePatchName("CAPSFILL", PU_HUDGFX);
	minus5sec = W_CachePatchName("MINUS5", PU_HUDGFX);

	for (i = 0; i < 7; ++i)
		ngradeletters[i] = W_CachePatchName(va("GRADE%d", i), PU_HUDGFX);
}

// made separate so that skins code can reload custom face graphics
void ST_LoadFaceGraphics(INT32 skinnum)
{
	if (skins[skinnum].sprites[SPR2_XTRA].numframes > XTRA_LIFEPIC)
	{
		spritedef_t *sprdef = &skins[skinnum].sprites[SPR2_XTRA];
		spriteframe_t *sprframe = &sprdef->spriteframes[XTRA_LIFEPIC];
		faceprefix[skinnum] = W_CachePatchNum(sprframe->lumppat[0], PU_HUDGFX);
		if (skins[skinnum].sprites[(SPR2_XTRA|FF_SPR2SUPER)].numframes > XTRA_LIFEPIC)
		{
			sprdef = &skins[skinnum].sprites[SPR2_XTRA|FF_SPR2SUPER];
			sprframe = &sprdef->spriteframes[0];
			superprefix[skinnum] = W_CachePatchNum(sprframe->lumppat[0], PU_HUDGFX);
		}
		else
			superprefix[skinnum] = faceprefix[skinnum]; // not manually freed, okay to set to same pointer
	}
	else
		faceprefix[skinnum] = superprefix[skinnum] = W_CachePatchName("MISSING", PU_HUDGFX); // ditto
	facefreed[skinnum] = false;
}

void ST_ReloadSkinFaceGraphics(void)
{
	INT32 i;

	for (i = 0; i < numskins; i++)
		ST_LoadFaceGraphics(i);
}

static inline void ST_InitData(void)
{
	// 'link' the statusbar display to a player, which could be
	// another player than consoleplayer, for example, when you
	// change the view in a multiplayer demo with F12.
	stplyr = &players[displayplayer];

	st_palette = -1;
}

static inline void ST_Stop(void)
{
	if (st_stopped)
		return;

	V_SetPalette(0);

	st_stopped = true;
}

void ST_Start(void)
{
	if (!st_stopped)
		ST_Stop();

	ST_InitData();
	st_stopped = false;
}

//
// Initializes the status bar, sets the defaults border patch for the window borders.
//

// used by OpenGL mode, holds lumpnum of flat used to fill space around the viewwindow
lumpnum_t st_borderpatchnum;

void ST_Init(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
		facefreed[i] = true;

	if (dedicated)
		return;

	ST_LoadGraphics();
}

// change the status bar too, when pressing F12 while viewing a demo.
void ST_changeDemoView(void)
{
	// the same routine is called at multiplayer deathmatch spawn
	// so it can be called multiple times
	ST_Start();
}

// =========================================================================
//                         STATUS BAR OVERLAY
// =========================================================================

boolean st_overlay;

// =========================================================================
//                          INTERNAL DRAWING
// =========================================================================
#define ST_DrawTopLeftOverlayPatch(x,y,p)         V_DrawScaledPatch(x, y, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTOLEFT|V_HUDTRANS, p)
#define ST_DrawNumFromHud(h,n,flags)        V_DrawTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f|V_PERPLAYER|flags, n)
#define ST_DrawPadNumFromHud(h,n,q,flags)   V_DrawPaddedTallNum(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f|V_PERPLAYER|flags, n, q)
#define ST_DrawPatchFromHud(h,p,flags)      V_DrawScaledPatch(hudinfo[h].x, hudinfo[h].y, hudinfo[h].f|V_PERPLAYER|flags, p)

// Draw a number, scaled, over the view, maybe with set translucency
// Always draw the number completely since it's overlay
//
// Supports different colors! woo!
static void ST_DrawNightsOverlayNum(fixed_t x /* right border */, fixed_t y, fixed_t s, INT32 a,
	UINT32 num, patch_t **numpat, skincolornum_t colornum)
{
	fixed_t w = SHORT(numpat[0]->width)*s;
	const UINT8 *colormap;

	// I want my V_SNAPTOx flags. :< -Red
	//a &= V_ALPHAMASK;

	if (colornum == 0)
		colormap = colormaps;
	else // Uses the player colors.
		colormap = R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE);

	//I_Assert(num >= 0); // this function does not draw negative numbers

	// draw the number
	do
	{
		x -= w;
		V_DrawFixedPatch(x, y, s, a, numpat[num % 10], colormap);
		num /= 10;
	} while (num);

	// Sorry chum, this function only draws UNSIGNED values!
	// then why is num not UINT32? ~toast
}

// Devmode information
static void ST_drawDebugInfo(void)
{
	INT32 height = 0, h = 8, w = 18, lowh;
	void (*textfunc)(INT32, INT32, INT32, const char *);

	if (!(stplyr->mo && cv_debug))
		return;

#define VFLAGS V_MONOSPACE|V_SNAPTOTOP|V_SNAPTORIGHT

	if ((moviemode == MM_GIF && cv_gif_downscale.value) || vid.dupx == 1)
	{
		textfunc = V_DrawRightAlignedString;
		lowh = ((vid.height/vid.dupy) - 16);
	}
	else
	{
		textfunc = V_DrawRightAlignedSmallString;
		h /= 2;
		w /= 2;
		lowh = 0;
	}

#define V_DrawDebugLine(str) if (lowh && (height > lowh))\
							{\
								V_DrawRightAlignedThinString(320,  8+lowh, VFLAGS|V_REDMAP, "SOME INFO NOT VISIBLE");\
								return;\
							}\
							textfunc(320, height, VFLAGS, str);\
							height += h;

#define V_DrawDebugFlag(f, str) textfunc(width, height, VFLAGS|f, str);\
								width -= w

	if (cv_debug & DBG_MEMORY)
	{
		V_DrawDebugLine(va("Heap: %8sKB", sizeu1(Z_TotalUsage()>>10)));

		height += h/2;
	}

	if (cv_debug & DBG_RANDOMIZER) // randomizer testing
	{
		fixed_t peekres = P_RandomPeek();
		peekres *= 10000;     // Change from fixed point
		peekres >>= FRACBITS; // to displayable decimal

		V_DrawDebugLine(va("Init: %08x", P_GetInitSeed()));
		V_DrawDebugLine(va("Seed: %08x", P_GetRandSeed()));
		V_DrawDebugLine(va("==  :    .%04d", peekres));

		height += h/2;
	}

	if (cv_debug & DBG_PLAYER)
	{
		INT32 width = 320;
		const fixed_t d = AngleFixed(stplyr->drawangle);

		V_DrawDebugLine(va("SHIELD: %5x", stplyr->powers[pw_shield]));
		V_DrawDebugLine(va("SCALE: %5d%%", (stplyr->mo->scale*100)>>FRACBITS));
		V_DrawDebugLine(va("CARRY: %5x", stplyr->powers[pw_carry]));
		V_DrawDebugLine(va("AIR: %4d, %3d", stplyr->powers[pw_underwater], stplyr->powers[pw_spacetime]));
		V_DrawDebugLine(va("ABILITY: %3d, %3d", stplyr->charability, stplyr->charability2));
		V_DrawDebugLine(va("ACTIONSPD: %5d", stplyr->actionspd>>FRACBITS));
		V_DrawDebugLine(va("PEEL: %3d", stplyr->dashmode));
		V_DrawDebugLine(va("SCOREADD: %3d", stplyr->scoreadd));

		// Flags
		V_DrawDebugFlag(((stplyr->pflags & PF_SHIELDABILITY)  ? V_GREENMAP : V_REDMAP), "SH");
		V_DrawDebugFlag(((stplyr->pflags & PF_THOKKED)        ? V_GREENMAP : V_REDMAP), "TH");
		V_DrawDebugFlag(((stplyr->pflags & PF_STARTDASH)      ? V_GREENMAP : V_REDMAP), "ST");
		V_DrawDebugFlag(((stplyr->pflags & PF_SPINNING)       ? V_GREENMAP : V_REDMAP), "SP");
		V_DrawDebugFlag(((stplyr->pflags & PF_NOJUMPDAMAGE)   ? V_GREENMAP : V_REDMAP), "ND");
		V_DrawDebugFlag(((stplyr->pflags & PF_JUMPED)         ? V_GREENMAP : V_REDMAP), "JD");
		V_DrawDebugFlag(((stplyr->pflags & PF_STARTJUMP)      ? V_GREENMAP : V_REDMAP), "SJ");
		V_DrawDebugFlag(0, "PF/SF:");
		height += h;
		width = 320;
		V_DrawDebugFlag(((stplyr->pflags & PF_INVIS)          ? V_GREENMAP : V_REDMAP), "*I");
		V_DrawDebugFlag(((stplyr->pflags & PF_NOCLIP)         ? V_GREENMAP : V_REDMAP), "*C");
		V_DrawDebugFlag(((stplyr->pflags & PF_GODMODE)        ? V_GREENMAP : V_REDMAP), "*G");
		V_DrawDebugFlag(((stplyr->charflags & SF_SUPER)       ? V_GREENMAP : V_REDMAP), "SU");
		V_DrawDebugFlag(((stplyr->pflags & PF_APPLYAUTOBRAKE) ? V_GREENMAP : V_REDMAP), "AA");
		V_DrawDebugFlag(((stplyr->pflags & PF_SLIDING)        ? V_GREENMAP : V_REDMAP), "SL");
		V_DrawDebugFlag(((stplyr->pflags & PF_BOUNCING)       ? V_GREENMAP : V_REDMAP), "BO");
		V_DrawDebugFlag(((stplyr->pflags & PF_GLIDING)        ? V_GREENMAP : V_REDMAP), "GL");
		height += h;

		V_DrawDebugLine(va("DRAWANGLE: %6d", FixedInt(d)));

		height += h/2;
	}

	if (cv_debug & DBG_DETAILED)
	{
		INT32 width = 320;

		V_DrawDebugLine(va("CEILINGZ: %6d", stplyr->mo->ceilingz>>FRACBITS));
		V_DrawDebugLine(va("FLOORZ: %6d", stplyr->mo->floorz>>FRACBITS));

		V_DrawDebugLine(va("CMOMX: %6d", stplyr->cmomx>>FRACBITS));
		V_DrawDebugLine(va("CMOMY: %6d", stplyr->cmomy>>FRACBITS));
		V_DrawDebugLine(va("PMOMZ: %6d", stplyr->mo->pmomz>>FRACBITS));

		width = 320;
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_APPLYPMOMZ)      ? V_GREENMAP : V_REDMAP), "AP");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_SPRUNG)          ? V_GREENMAP : V_REDMAP), "SP");
		//V_DrawDebugFlag(((stplyr->mo->eflags & MFE_PUSHED)          ? V_GREENMAP : V_REDMAP), "PU"); -- not relevant to players
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_GOOWATER)        ? V_GREENMAP : V_REDMAP), "GW");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_VERTICALFLIP)    ? V_GREENMAP : V_REDMAP), "VF");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_JUSTSTEPPEDDOWN) ? V_GREENMAP : V_REDMAP), "JS");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_UNDERWATER)      ? V_GREENMAP : V_REDMAP), "UW");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_TOUCHWATER)      ? V_GREENMAP : V_REDMAP), "TW");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_JUSTHITFLOOR)    ? V_GREENMAP : V_REDMAP), "JH");
		V_DrawDebugFlag(((stplyr->mo->eflags & MFE_ONGROUND)        ? V_GREENMAP : V_REDMAP), "OG");
		V_DrawDebugFlag(0, "MFE:");
		height += h;

		V_DrawDebugLine(va("MOMX: %6d", stplyr->rmomx>>FRACBITS));
		V_DrawDebugLine(va("MOMY: %6d", stplyr->rmomy>>FRACBITS));
		V_DrawDebugLine(va("MOMZ: %6d", stplyr->mo->momz>>FRACBITS));

		V_DrawDebugLine(va("SPEED: %6d", stplyr->speed>>FRACBITS));

		height += h/2;
	}

	if (cv_debug & DBG_BASIC)
	{
		const fixed_t d = AngleFixed(stplyr->mo->angle);
		V_DrawDebugLine(va("X: %6d", stplyr->mo->x>>FRACBITS));
		V_DrawDebugLine(va("Y: %6d", stplyr->mo->y>>FRACBITS));
		V_DrawDebugLine(va("Z: %6d", stplyr->mo->z>>FRACBITS));
		V_DrawDebugLine(va("A: %6d", FixedInt(d)));

		//height += h/2;
	}

#undef V_DrawDebugFlag
#undef V_DrawDebugLine
#undef VFLAGS
}

static void ST_drawScore(void)
{
	if (F_GetPromptHideHud(hudinfo[HUD_SCORE].y))
		return;

	// SCORE:
	ST_DrawPatchFromHud(HUD_SCORE, sboscore, V_HUDTRANS);
	if (objectplacing)
	{
		if (op_displayflags > UINT16_MAX)
			ST_DrawTopLeftOverlayPatch((hudinfo[HUD_SCORENUM].x-tallminus->width), hudinfo[HUD_SCORENUM].y, tallminus);
		else
			ST_DrawNumFromHud(HUD_SCORENUM, op_displayflags, V_HUDTRANS);
	}
	else
		ST_DrawNumFromHud(HUD_SCORENUM, stplyr->score, V_HUDTRANS);
}

static void ST_drawRaceNum(INT32 time)
{
	INT32 height, bounce;
	patch_t *racenum;

	time += TICRATE;
	height = ((3*BASEVIDHEIGHT)>>2) - 8;
	bounce = TICRATE - (1 + (time % TICRATE));

	switch (time/TICRATE)
	{
		case 3:
			racenum = race3;
			break;
		case 2:
			racenum = race2;
			break;
		case 1:
			racenum = race1;
			break;
		default:
			racenum = racego;
			break;
	}
	if (bounce < 3)
	{
		height -= (2 - bounce);
		if (!(P_AutoPause() || paused) && !bounce)
				S_StartSound(0, ((racenum == racego) ? sfx_s3kad : sfx_s3ka7));
	}
	V_DrawScaledPatch(((BASEVIDWIDTH - SHORT(racenum->width))/2), height, V_PERPLAYER, racenum);
}

static void ST_drawTime(void)
{
	INT32 seconds, minutes, tictrn, tics;
	boolean downwards = false;

	if (objectplacing)
	{
		tics    = objectsdrawn;
		seconds = objectsdrawn%100;
		minutes = objectsdrawn/100;
		tictrn  = 0;
	}
	else
	{
		// Counting down the hidetime?
		if ((gametyperules & GTR_STARTCOUNTDOWN) && (stplyr->realtime <= (hidetime*TICRATE)))
		{
			tics = (hidetime*TICRATE - stplyr->realtime);
			if (tics < 3*TICRATE)
				ST_drawRaceNum(tics);
			tics += (TICRATE-1); // match the race num
			downwards = true;
		}
		else
		{
			// Hidetime finish!
			if ((gametyperules & GTR_STARTCOUNTDOWN) && (stplyr->realtime < ((hidetime+1)*TICRATE)))
				ST_drawRaceNum(hidetime*TICRATE - stplyr->realtime);

			// Time limit?
			if ((gametyperules & GTR_TIMELIMIT) && cv_timelimit.value && timelimitintics > 0)
			{
				if (timelimitintics > stplyr->realtime)
				{
					tics = (timelimitintics - stplyr->realtime);
					if (tics < 3*TICRATE)
						ST_drawRaceNum(tics);
					tics += (TICRATE-1); // match the race num
				}
				else // Overtime!
					tics = 0;
				downwards = true;
			}
			// Post-hidetime normal.
			else if (gametyperules & GTR_STARTCOUNTDOWN)
				tics = stplyr->realtime - hidetime*TICRATE;
			// "Shadow! What are you doing? Hurry and get back here
			// right now before the island blows up with you on it!"
			// "Blows up??" *awkward silence* "I've got to get outta
			// here and find Amy and Tails right away!"
			else if (mapheaderinfo[gamemap-1]->countdown)
			{
				tics = countdowntimer;
				downwards = true;
			}
			// Normal.
			else
				tics = stplyr->realtime;
		}

		minutes = G_TicsToMinutes(tics, true);
		seconds = G_TicsToSeconds(tics);
		tictrn  = G_TicsToCentiseconds(tics);
	}

	if (F_GetPromptHideHud(hudinfo[HUD_TIME].y))
		return;

	downwards = (downwards && (tics < 30*TICRATE) && (leveltime/5 & 1) && !stoppedclock); // overtime?

	// TIME:
	ST_DrawPatchFromHud(HUD_TIME, (downwards ? sboredtime : sbotime), V_HUDTRANS);

	if (downwards) // overtime!
		return;

	if (cv_timetic.value == 3) // Tics only -- how simple is this?
		ST_DrawNumFromHud(HUD_SECONDS, tics, V_HUDTRANS);
	else
	{
		ST_DrawNumFromHud(HUD_MINUTES, minutes, V_HUDTRANS); // Minutes
		ST_DrawPatchFromHud(HUD_TIMECOLON, sbocolon, V_HUDTRANS); // Colon
		ST_DrawPadNumFromHud(HUD_SECONDS, seconds, 2, V_HUDTRANS); // Seconds

		if (cv_timetic.value == 1 || cv_timetic.value == 2 || modeattacking || marathonmode)
		{
			ST_DrawPatchFromHud(HUD_TIMETICCOLON, sboperiod, V_HUDTRANS); // Period
			ST_DrawPadNumFromHud(HUD_TICS, tictrn, 2, V_HUDTRANS); // Tics
		}
	}
}

static inline void ST_drawRings(void)
{
	INT32 ringnum;

	if (F_GetPromptHideHud(hudinfo[HUD_RINGS].y))
		return;

	ST_DrawPatchFromHud(HUD_RINGS, ((!stplyr->spectator && stplyr->rings <= 0 && leveltime/5 & 1) ? sboredrings : sborings), ((stplyr->spectator) ? V_HUDTRANSHALF : V_HUDTRANS));

	if (objectplacing)
		ringnum = op_currentdoomednum;
	else if (stplyr->rings < 0 || stplyr->spectator || stplyr->playerstate == PST_REBORN)
		ringnum = 0;
	else
		ringnum = stplyr->rings;

	if (cv_timetic.value == 2) // Yes, even in modeattacking
		ST_DrawNumFromHud(HUD_RINGSNUMTICS, ringnum, V_PERPLAYER|((stplyr->spectator) ? V_HUDTRANSHALF : V_HUDTRANS));
	else
		ST_DrawNumFromHud(HUD_RINGSNUM, ringnum, V_PERPLAYER|((stplyr->spectator) ? V_HUDTRANSHALF : V_HUDTRANS));
}

static void ST_drawLivesArea(void)
{
	INT32 v_colmap = V_YELLOWMAP, livescount;
	boolean notgreyedout;

	if (!stplyr->skincolor)
		return; // Just joined a server, skin isn't loaded yet!

	if (F_GetPromptHideHud(hudinfo[HUD_LIVES].y))
		return;

	// face background
	V_DrawSmallScaledPatch(hudinfo[HUD_LIVES].x, hudinfo[HUD_LIVES].y,
		hudinfo[HUD_LIVES].f|V_PERPLAYER|V_HUDTRANS, livesback);

	// face
	if (stplyr->spectator)
	{
		// spectator face
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, SKINCOLOR_CLOUDY, GTC_CACHE);
		V_DrawSmallMappedPatch(hudinfo[HUD_LIVES].x, hudinfo[HUD_LIVES].y,
			hudinfo[HUD_LIVES].f|V_PERPLAYER|V_HUDTRANSHALF, faceprefix[stplyr->skin], colormap);
	}
	else if (stplyr->mo && stplyr->mo->color)
	{
		// skincolor face/super
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, stplyr->mo->color, GTC_CACHE);
		patch_t *face = faceprefix[stplyr->skin];
		if (stplyr->powers[pw_super])
			face = superprefix[stplyr->skin];
		V_DrawSmallMappedPatch(hudinfo[HUD_LIVES].x, hudinfo[HUD_LIVES].y,
			hudinfo[HUD_LIVES].f|V_PERPLAYER|V_HUDTRANS, face, colormap);
		if (st_translucency == 10 && stplyr->powers[pw_super] == 1 && stplyr->mo->tracer)
		{
			INT32 v_supertrans = (stplyr->mo->tracer->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;
			if (v_supertrans < 10)
			{
				v_supertrans <<= V_ALPHASHIFT;
				colormap = R_GetTranslationColormap(stplyr->skin, stplyr->mo->tracer->color, GTC_CACHE);
				V_DrawSmallMappedPatch(hudinfo[HUD_LIVES].x, hudinfo[HUD_LIVES].y,
					hudinfo[HUD_LIVES].f|V_PERPLAYER|v_supertrans, face, colormap);
			}
		}
	}
	else if (stplyr->skincolor)
	{
		// skincolor face
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, stplyr->skincolor, GTC_CACHE);
		V_DrawSmallMappedPatch(hudinfo[HUD_LIVES].x, hudinfo[HUD_LIVES].y,
			hudinfo[HUD_LIVES].f|V_PERPLAYER|V_HUDTRANS, faceprefix[stplyr->skin], colormap);
	}

	// Metal Sonic recording
	if (metalrecording)
	{
		if (((2*leveltime)/TICRATE) & 1)
			V_DrawRightAlignedString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y+8,
				hudinfo[HUD_LIVES].f|V_PERPLAYER|V_REDMAP|V_HUDTRANS, "REC");
	}
	// Spectator
	else if (stplyr->spectator)
		v_colmap = V_GRAYMAP;
	// Tag
	else if (gametyperules & GTR_TAG)
	{
		if (stplyr->pflags & PF_TAGIT)
		{
			V_DrawRightAlignedString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y+8, V_HUDTRANS|hudinfo[HUD_LIVES].f|V_PERPLAYER, "IT!");
			v_colmap = V_ORANGEMAP;
		}
	}
	// Team name
	else if (G_GametypeHasTeams())
	{
		if (stplyr->ctfteam == 1)
		{
			V_DrawRightAlignedString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y+8, V_HUDTRANS|hudinfo[HUD_LIVES].f|V_PERPLAYER, "RED");
			v_colmap = V_REDMAP;
		}
		else if (stplyr->ctfteam == 2)
		{
			V_DrawRightAlignedString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y+8, V_HUDTRANS|hudinfo[HUD_LIVES].f|V_PERPLAYER, "BLUE");
			v_colmap = V_BLUEMAP;
		}
	}
	// Lives number
	else
	{
		boolean candrawlives = true;

		// Co-op and Competition, normal life counter
		if (G_GametypeUsesLives())
		{
			// Handle cooplives here
			if ((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 3)
			{
				INT32 i;
				livescount = 0;
				notgreyedout = (stplyr->lives > 0);
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i])
						continue;

					if (players[i].lives < 1)
						continue;

					if (players[i].lives > 1)
						notgreyedout = true;

					if (players[i].lives == INFLIVES)
					{
						livescount = INFLIVES;
						break;
					}
					else if (livescount < 99)
						livescount += (players[i].lives);
				}
			}
			else
			{
				livescount = (((netgame || multiplayer) && G_GametypeUsesCoopLives() && cv_cooplives.value == 0) ? INFLIVES : stplyr->lives);
				notgreyedout = true;
			}
		}
		// Infinity symbol (Race)
		else if (G_PlatformGametype() && !(gametyperules & GTR_LIVES))
		{
			livescount = INFLIVES;
			notgreyedout = true;
		}
		// Otherwise nothing, sorry.
		// Special Stages keep not showing lives,
		// as G_GametypeUsesLives() returns false in
		// Special Stages, and the infinity symbol
		// cannot show up because Special Stages
		// still have the GTR_LIVES gametype rule
		// by default.
		else
			candrawlives = false;

		// Draw the lives counter here.
		if (candrawlives)
		{
			// x
			V_DrawScaledPatch(hudinfo[HUD_LIVES].x+22, hudinfo[HUD_LIVES].y+10, hudinfo[HUD_LIVES].f|V_PERPLAYER|V_HUDTRANS, stlivex);
			if (livescount == INFLIVES)
				V_DrawCharacter(hudinfo[HUD_LIVES].x+50, hudinfo[HUD_LIVES].y+8,
					'\x16' | 0x80 | hudinfo[HUD_LIVES].f|V_PERPLAYER|V_HUDTRANS, false);
			else
			{
				if (stplyr->playerstate == PST_DEAD && !(stplyr->spectator) && (livescount || stplyr->deadtimer < (TICRATE<<1)) && !(stplyr->pflags & PF_FINISHED))
					livescount++;
				if (livescount > 99)
					livescount = 99;
				V_DrawRightAlignedString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y+8,
					hudinfo[HUD_LIVES].f|V_PERPLAYER|(notgreyedout ? V_HUDTRANS : V_HUDTRANSHALF), va("%d",livescount));
			}
		}
#undef ST_drawLivesX
	}

	// name
	v_colmap |= (V_HUDTRANS|hudinfo[HUD_LIVES].f|V_PERPLAYER);
	if (strlen(skins[stplyr->skin].hudname) <= 5)
		V_DrawRightAlignedString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y, v_colmap, skins[stplyr->skin].hudname);
	else if (V_StringWidth(skins[stplyr->skin].hudname, v_colmap) <= 48)
		V_DrawString(hudinfo[HUD_LIVES].x+18, hudinfo[HUD_LIVES].y, v_colmap, skins[stplyr->skin].hudname);
	else if (V_ThinStringWidth(skins[stplyr->skin].hudname, v_colmap) <= 40)
		V_DrawRightAlignedThinString(hudinfo[HUD_LIVES].x+58, hudinfo[HUD_LIVES].y, v_colmap, skins[stplyr->skin].hudname);
	else
		V_DrawThinString(hudinfo[HUD_LIVES].x+18, hudinfo[HUD_LIVES].y, v_colmap, skins[stplyr->skin].hudname);

	// Power Stones collected
	if (G_RingSlingerGametype() && LUA_HudEnabled(hud_powerstones))
	{
		INT32 workx = hudinfo[HUD_LIVES].x+1, j;
		if ((leveltime & 1) && stplyr->powers[pw_invulnerability] && (stplyr->powers[pw_sneakers] == stplyr->powers[pw_invulnerability])) // hack; extremely unlikely to be activated unintentionally
		{
			for (j = 0; j < 7; ++j) // "super" indicator
			{
				V_DrawScaledPatch(workx, hudinfo[HUD_LIVES].y-9, V_HUDTRANS|hudinfo[HUD_LIVES].f|V_PERPLAYER, emeraldpics[1][j]);
				workx += 8;
			}
		}
		else
		{
			for (j = 0; j < 7; ++j) // powerstones
			{
				if (stplyr->powers[pw_emeralds] & (1 << j))
					V_DrawScaledPatch(workx, hudinfo[HUD_LIVES].y-9, V_HUDTRANS|hudinfo[HUD_LIVES].f|V_PERPLAYER, emeraldpics[1][j]);
				workx += 8;
			}
		}
	}
}

static void ST_drawInput(void)
{
	const INT32 accent = V_SNAPTOLEFT|V_SNAPTOBOTTOM|(stplyr->skincolor ? skincolors[stplyr->skincolor].ramp[4] : 0);
	INT32 col;
	UINT8 offs;

	INT32 x = hudinfo[HUD_LIVES].x, y = hudinfo[HUD_LIVES].y;

	if (stplyr->powers[pw_carry] == CR_NIGHTSMODE)
		y -= 16;

	if (F_GetPromptHideHud(y))
		return;

	// O backing
	V_DrawFill(x, y-1, 16, 16, hudinfo[HUD_LIVES].f|20);
	V_DrawFill(x, y+15, 16, 1, hudinfo[HUD_LIVES].f|29);

	if (cv_showinputjoy.value) // joystick render!
	{
		/*V_DrawFill(x   , y   , 16,  1, hudinfo[HUD_LIVES].f|16);
		V_DrawFill(x   , y+15, 16,  1, hudinfo[HUD_LIVES].f|16);
		V_DrawFill(x   , y+ 1,  1, 14, hudinfo[HUD_LIVES].f|16);
		V_DrawFill(x+15, y+ 1,  1, 14, hudinfo[HUD_LIVES].f|16); -- red's outline*/
		if (stplyr->cmd.sidemove || stplyr->cmd.forwardmove)
		{
			// joystick hole
			V_DrawFill(x+5, y+4, 6, 6, hudinfo[HUD_LIVES].f|29);
			// joystick top
			V_DrawFill(x+3+stplyr->cmd.sidemove/12,
				y+2-stplyr->cmd.forwardmove/12,
				10, 10, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+3+stplyr->cmd.sidemove/9,
				y+1-stplyr->cmd.forwardmove/9,
				10, 10, accent);
		}
		else
		{
			// just a limited, greyed out joystick top
			V_DrawFill(x+3, y+11, 10, 1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+3,
				y+1,
				10, 10, hudinfo[HUD_LIVES].f|16);
		}
	}
	else // arrows!
	{
		// <
		if (stplyr->cmd.sidemove < 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = hudinfo[HUD_LIVES].f|16;
			V_DrawFill(x- 2, y+10,  6,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+ 4, y+ 9,  1,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+ 5, y+ 8,  1,  1, hudinfo[HUD_LIVES].f|29);
		}
		V_DrawFill(x- 2, y+ 5-offs,  6,  6, col);
		V_DrawFill(x+ 4, y+ 6-offs,  1,  4, col);
		V_DrawFill(x+ 5, y+ 7-offs,  1,  2, col);

		// ^
		if (stplyr->cmd.forwardmove > 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = hudinfo[HUD_LIVES].f|16;
			V_DrawFill(x+ 5, y+ 3,  1,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+ 6, y+ 4,  1,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+ 7, y+ 5,  2,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+ 9, y+ 4,  1,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+10, y+ 3,  1,  1, hudinfo[HUD_LIVES].f|29);
		}
		V_DrawFill(x+ 5, y- 2-offs,  6,  6, col);
		V_DrawFill(x+ 6, y+ 4-offs,  4,  1, col);
		V_DrawFill(x+ 7, y+ 5-offs,  2,  1, col);

		// >
		if (stplyr->cmd.sidemove > 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = hudinfo[HUD_LIVES].f|16;
			V_DrawFill(x+12, y+10,  6,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+11, y+ 9,  1,  1, hudinfo[HUD_LIVES].f|29);
			V_DrawFill(x+10, y+ 8,  1,  1, hudinfo[HUD_LIVES].f|29);
		}
		V_DrawFill(x+12, y+ 5-offs,  6,  6, col);
		V_DrawFill(x+11, y+ 6-offs,  1,  4, col);
		V_DrawFill(x+10, y+ 7-offs,  1,  2, col);

		// v
		if (stplyr->cmd.forwardmove < 0)
		{
			offs = 0;
			col = accent;
		}
		else
		{
			offs = 1;
			col = hudinfo[HUD_LIVES].f|16;
			V_DrawFill(x+ 5, y+17,  6,  1, hudinfo[HUD_LIVES].f|29);
		}
		V_DrawFill(x+ 5, y+12-offs,  6,  6, col);
		V_DrawFill(x+ 6, y+11-offs,  4,  1, col);
		V_DrawFill(x+ 7, y+10-offs,  2,  1, col);
	}

#define drawbutt(xoffs, yoffs, butt, symb)\
	if (stplyr->cmd.buttons & butt)\
	{\
		offs = 0;\
		col = accent;\
	}\
	else\
	{\
		offs = 1;\
		col = hudinfo[HUD_LIVES].f|16;\
		V_DrawFill(x+16+(xoffs), y+9+(yoffs), 10, 1, hudinfo[HUD_LIVES].f|29);\
	}\
	V_DrawFill(x+16+(xoffs), y+(yoffs)-offs, 10, 10, col);\
	V_DrawCharacter(x+16+1+(xoffs), y+1+(yoffs)-offs, hudinfo[HUD_LIVES].f|symb, false)

	drawbutt( 4,-3, BT_JUMP, 'J');
	drawbutt(15,-3, BT_USE,  'S');

	V_DrawFill(x+16+4, y+8, 21, 10, hudinfo[HUD_LIVES].f|20); // sundial backing
	if (stplyr->mo)
	{
		UINT8 i, precision;
		angle_t ang = (stplyr->powers[pw_carry] == CR_NIGHTSMODE)
		? (FixedAngle((stplyr->flyangle-90)<<FRACBITS)>>ANGLETOFINESHIFT)
		: (stplyr->mo->angle - R_PointToAngle(stplyr->mo->x, stplyr->mo->y))>>ANGLETOFINESHIFT;
		fixed_t xcomp = FINESINE(ang)>>13;
		fixed_t ycomp = FINECOSINE(ang)>>14;
		if (ycomp == 4)
			ycomp = 3;

		if (ycomp > 0)
			V_DrawFill(x+16+13-xcomp, y+11-ycomp, 3, 3, accent); // point (behind)

		precision = max(3, abs(xcomp));
		for (i = 0; i < precision; i++) // line
		{
			V_DrawFill(x+16+14-(i*xcomp)/precision,
				y+12-(i*ycomp)/precision,
				1, 1, hudinfo[HUD_LIVES].f|16);
		}

		if (ycomp <= 0)
			V_DrawFill(x+16+13-xcomp, y+11-ycomp, 3, 3, accent); // point (in front)
	}

#undef drawbutt

	// text above
	x -= 2;
	y -= 13;
	if (stplyr->powers[pw_carry] != CR_NIGHTSMODE)
	{
		if (stplyr->pflags & PF_AUTOBRAKE)
		{
			V_DrawThinString(x, y,
				hudinfo[HUD_LIVES].f|
				((!stplyr->powers[pw_carry]
				&& (stplyr->pflags & PF_APPLYAUTOBRAKE)
				&& !(stplyr->cmd.sidemove || stplyr->cmd.forwardmove)
				&& (stplyr->rmomx || stplyr->rmomy)
				&& (!stplyr->capsule || (stplyr->capsule->reactiontime != (stplyr-players)+1)))
				? 0 : V_GRAYMAP),
				"AUTOBRAKE");
			y -= 8;
		}
		switch (P_ControlStyle(stplyr))
		{
		case CS_LMAOGALOG:
			V_DrawThinString(x, y, hudinfo[HUD_LIVES].f, "ANALOG");
			y -= 8;
			break;

		case CS_SIMPLE:
			V_DrawThinString(x, y, hudinfo[HUD_LIVES].f, "SIMPLE");
			y -= 8;
			break;

		default:
			break;
		}
	}
	if (!demosynced) // should always be last, so it doesn't push anything else around
		V_DrawThinString(x, y, hudinfo[HUD_LIVES].f|((leveltime & 4) ? V_YELLOWMAP : V_REDMAP), "BAD DEMO!!");
}

static patch_t *lt_patches[3];
static INT32 lt_scroll = 0;
static INT32 lt_mom = 0;
static INT32 lt_zigzag = 0;

tic_t lt_ticker = 0, lt_lasttic = 0;
tic_t lt_exitticker = 0, lt_endtime = 0;

//
// Load the graphics for the title card.
// Don't let LJ see this
//
static void ST_cacheLevelTitle(void)
{
#define SETPATCH(default, warning, custom, idx) \
{ \
	lumpnum_t patlumpnum = LUMPERROR; \
	if (mapheaderinfo[gamemap-1]->custom[0] != '\0') \
	{ \
		patlumpnum = W_CheckNumForName(mapheaderinfo[gamemap-1]->custom); \
		if (patlumpnum != LUMPERROR) \
			lt_patches[idx] = (patch_t *)W_CachePatchNum(patlumpnum, PU_HUDGFX); \
	} \
	if (patlumpnum == LUMPERROR) \
	{ \
		if (!(mapheaderinfo[gamemap-1]->levelflags & LF_WARNINGTITLE)) \
			lt_patches[idx] = (patch_t *)W_CachePatchName(default, PU_HUDGFX); \
		else \
			lt_patches[idx] = (patch_t *)W_CachePatchName(warning, PU_HUDGFX); \
	} \
}

	SETPATCH("LTACTBLU", "LTACTRED", ltactdiamond, 0)
	SETPATCH("LTZIGZAG", "LTZIGRED", ltzzpatch, 1)
	SETPATCH("LTZZTEXT", "LTZZWARN", ltzztext, 2)

#undef SETPATCH
}

//
// Start the title card.
//
void ST_startTitleCard(void)
{
	// cache every HUD patch used
	ST_cacheLevelTitle();

	// initialize HUD variables
	lt_ticker = lt_exitticker = lt_lasttic = 0;
	lt_endtime = 2*TICRATE + (10*NEWTICRATERATIO);
	lt_scroll = BASEVIDWIDTH * FRACUNIT;
	lt_zigzag = -((lt_patches[1])->width * FRACUNIT);
	lt_mom = 0;
}

//
// What happens before drawing the title card.
// Which is just setting the HUD translucency.
//
void ST_preDrawTitleCard(void)
{
	if (!G_IsTitleCardAvailable())
		return;

	if (lt_ticker >= (lt_endtime + TICRATE))
		return;

	if (!lt_exitticker)
		st_translucency = 0;
	else
		st_translucency = max(0, min((INT32)lt_exitticker-4, cv_translucenthud.value));
}

//
// Run the title card.
// Called from ST_Ticker.
//
void ST_runTitleCard(void)
{
	boolean run = !(paused || P_AutoPause());

	if (!G_IsTitleCardAvailable())
		return;

	if (lt_ticker >= (lt_endtime + TICRATE))
		return;

	if (run || (lt_ticker < PRELEVELTIME))
	{
		// tick
		lt_ticker++;
		if (lt_ticker >= lt_endtime)
			lt_exitticker++;

		// scroll to screen (level title)
		if (!lt_exitticker)
		{
			if (abs(lt_scroll) > FRACUNIT)
				lt_scroll -= (lt_scroll>>2);
			else
				lt_scroll = 0;
		}
		// scroll away from screen (level title)
		else
		{
			lt_mom -= FRACUNIT*6;
			lt_scroll += lt_mom;
		}

		// scroll to screen (zigzag)
		if (!lt_exitticker)
		{
			if (abs(lt_zigzag) > FRACUNIT)
				lt_zigzag -= (lt_zigzag>>2);
			else
				lt_zigzag = 0;
		}
		// scroll away from screen (zigzag)
		else
			lt_zigzag += lt_mom;
	}
}

//
// Draw the title card itself.
//
void ST_drawTitleCard(void)
{
	char *lvlttl = mapheaderinfo[gamemap-1]->lvlttl;
	char *subttl = mapheaderinfo[gamemap-1]->subttl;
	UINT8 actnum = mapheaderinfo[gamemap-1]->actnum;
	INT32 lvlttlxpos, ttlnumxpos, zonexpos;
	INT32 subttlxpos = BASEVIDWIDTH/2;
	INT32 ttlscroll = FixedInt(lt_scroll);
	INT32 zzticker;
	patch_t *actpat, *zigzag, *zztext;
	UINT8 colornum;
	const UINT8 *colormap;

	if (players[consoleplayer].skincolor)
		colornum = players[consoleplayer].skincolor;
	else
		colornum = cv_playercolor.value;

	colormap = R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE);

	if (!G_IsTitleCardAvailable())
		return;

	if (!LUA_HudEnabled(hud_stagetitle))
		goto luahook;

	if (lt_ticker >= (lt_endtime + TICRATE))
		goto luahook;

	if ((lt_ticker-lt_lasttic) > 1)
		lt_ticker = lt_lasttic+1;

	ST_cacheLevelTitle();
	actpat = lt_patches[0];
	zigzag = lt_patches[1];
	zztext = lt_patches[2];

	lvlttlxpos = ((BASEVIDWIDTH/2) - (V_LevelNameWidth(lvlttl)/2));

	if (actnum > 0)
		lvlttlxpos -= V_LevelActNumWidth(actnum);

	ttlnumxpos = lvlttlxpos + V_LevelNameWidth(lvlttl);
	zonexpos = ttlnumxpos - V_LevelNameWidth(M_GetText("Zone"));
	ttlnumxpos++;

	if (lvlttlxpos < 0)
		lvlttlxpos = 0;

	if (!splitscreen || (splitscreen && stplyr == &players[displayplayer]))
	{
		zzticker = lt_ticker;
		V_DrawMappedPatch(FixedInt(lt_zigzag), (-zzticker) % zigzag->height, V_SNAPTOTOP|V_SNAPTOLEFT, zigzag, colormap);
		V_DrawMappedPatch(FixedInt(lt_zigzag), (zigzag->height-zzticker) % zigzag->height, V_SNAPTOTOP|V_SNAPTOLEFT, zigzag, colormap);
		V_DrawMappedPatch(FixedInt(lt_zigzag), (-zigzag->height+zzticker) % zztext->height, V_SNAPTOTOP|V_SNAPTOLEFT, zztext, colormap);
		V_DrawMappedPatch(FixedInt(lt_zigzag), (zzticker) % zztext->height, V_SNAPTOTOP|V_SNAPTOLEFT, zztext, colormap);
	}

	if (actnum)
	{
		if (!splitscreen)
		{
			if (actnum > 9) // slightly offset the act diamond for two-digit act numbers
				V_DrawMappedPatch(ttlnumxpos + (V_LevelActNumWidth(actnum)/4) + ttlscroll, 104 - ttlscroll, 0, actpat, colormap);
			else
				V_DrawMappedPatch(ttlnumxpos + ttlscroll, 104 - ttlscroll, 0, actpat, colormap);
		}
		V_DrawLevelActNum(ttlnumxpos + ttlscroll, 104, V_PERPLAYER, actnum);
	}

	V_DrawLevelTitle(lvlttlxpos - ttlscroll, 80, V_PERPLAYER, lvlttl);
	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		V_DrawLevelTitle(zonexpos + ttlscroll, 104, V_PERPLAYER, M_GetText("Zone"));
	V_DrawCenteredString(subttlxpos - ttlscroll, 135, V_PERPLAYER|V_ALLOWLOWERCASE, subttl);

	lt_lasttic = lt_ticker;

luahook:
	LUAh_TitleCardHUD(stplyr);
}

//
// Drawer for G_PreLevelTitleCard.
//
void ST_preLevelTitleCardDrawer(void)
{
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, levelfadecol);
	ST_drawWipeTitleCard();
	I_OsPolling();
	I_UpdateNoBlit();
}

//
// Draw the title card while on a wipe.
// Also used in G_PreLevelTitleCard.
//
void ST_drawWipeTitleCard(void)
{
	stplyr = &players[consoleplayer];
	ST_preDrawTitleCard();
	ST_drawTitleCard();
	if (splitscreen)
	{
		stplyr = &players[secondarydisplayplayer];
		ST_preDrawTitleCard();
		ST_drawTitleCard();
	}
}

static void ST_drawPowerupHUD(void)
{
	patch_t *p = NULL;
	UINT16 invulntime = 0;
	INT32 offs = hudinfo[HUD_POWERUPS].x;
	const UINT8 q = ((splitscreen && stplyr == &players[secondarydisplayplayer]) ? 1 : 0);
	static INT32 flagoffs[2] = {0, 0}, shieldoffs[2] = {0, 0}, finishoffs[2] = {0, 0};
#define ICONSEP (16+4) // matches weapon rings HUD

	if (F_GetPromptHideHud(hudinfo[HUD_POWERUPS].y))
		return;

	if (stplyr->spectator || stplyr->playerstate != PST_LIVE)
		return;

// ---------
// Finish icon
// ---------

	// Let's have a power-like icon to represent finishing the level!
	if (stplyr->pflags & PF_FINISHED && cv_exitmove.value && multiplayer)
	{
		finishoffs[q] = ICONSEP;
		V_DrawSmallScaledPatch(offs, hudinfo[HUD_POWERUPS].y, V_PERPLAYER|hudinfo[HUD_POWERUPS].f|V_HUDTRANS, fnshico);
	}
	else if (finishoffs[q])
	{
		if (finishoffs[q] > 1)
			finishoffs[q] = 2*finishoffs[q]/3;
		else
			finishoffs[q] = 0;
	}

	offs -= finishoffs[q];

// -------
// Shields
// -------

	// Graue 06-18-2004: no V_NOSCALESTART, no SCX, no SCY, snap to right
	if (stplyr->powers[pw_shield] & SH_NOSTACK)
	{
		shieldoffs[q] = ICONSEP;

		if ((stplyr->powers[pw_shield] & SH_NOSTACK & ~SH_FORCEHP) == SH_FORCE)
		{
			UINT8 i, max = (stplyr->powers[pw_shield] & SH_FORCEHP);
			for (i = 0; i <= max; i++)
			{
				V_DrawSmallScaledPatch(offs-(i<<1), hudinfo[HUD_POWERUPS].y-(i<<1), (V_PERPLAYER|hudinfo[HUD_POWERUPS].f)|((i == max) ? V_HUDTRANS : V_HUDTRANSHALF), forceshield);
			}
		}
		else
		{
			switch (stplyr->powers[pw_shield] & SH_NOSTACK)
			{
				case SH_WHIRLWIND:   p = jumpshield;    break;
				case SH_ELEMENTAL:   p = watershield;   break;
				case SH_ARMAGEDDON:  p = bombshield;    break;
				case SH_ATTRACT:     p = ringshield;    break;
				case SH_PITY:        p = pityshield;    break;
				case SH_PINK:        p = pinkshield;    break;
				case SH_FLAMEAURA:   p = flameshield;   break;
				case SH_BUBBLEWRAP:  p = bubbleshield;  break;
				case SH_THUNDERCOIN: p = thundershield; break;
				default: break;
			}

			if (p)
				V_DrawSmallScaledPatch(offs, hudinfo[HUD_POWERUPS].y, V_PERPLAYER|hudinfo[HUD_POWERUPS].f|V_HUDTRANS, p);
		}
	}
	else if (shieldoffs[q])
	{
		if (shieldoffs[q] > 1)
			shieldoffs[q] = 2*shieldoffs[q]/3;
		else
			shieldoffs[q] = 0;
	}

	offs -= shieldoffs[q];

// ---------
// CTF flags
// ---------

	// YOU have a flag. Display a monitor-like icon for it.
	if (stplyr->gotflag)
	{
		flagoffs[q] = ICONSEP;
		p = (stplyr->gotflag & GF_REDFLAG) ? gotrflag : gotbflag;
		V_DrawSmallScaledPatch(offs, hudinfo[HUD_POWERUPS].y, V_PERPLAYER|hudinfo[HUD_POWERUPS].f|V_HUDTRANS, p);
	}
	else if (flagoffs[q])
	{
		if (flagoffs[q] > 1)
			flagoffs[q] = 2*flagoffs[q]/3;
		else
			flagoffs[q] = 0;
	}

	offs -= flagoffs[q];

// --------------------
// Timer-based powerups
// --------------------

#define DRAWTIMERICON(patch, timer) \
	V_DrawSmallScaledPatch(offs, hudinfo[HUD_POWERUPS].y, V_PERPLAYER|hudinfo[HUD_POWERUPS].f|V_HUDTRANS, patch); \
	V_DrawRightAlignedThinString(offs + 16, hudinfo[HUD_POWERUPS].y + 8, V_PERPLAYER|hudinfo[HUD_POWERUPS].f, va("%d", timer/TICRATE));

	// Invincibility, both from monitor and after being hit
	invulntime = stplyr->powers[pw_flashing] ? stplyr->powers[pw_flashing] : stplyr->powers[pw_invulnerability];
	// Note: pw_flashing always makes the icon flicker regardless of time, unlike pw_invulnerability
	if (stplyr->powers[pw_invulnerability] > 3*TICRATE || (invulntime && leveltime & 1))
	{
		DRAWTIMERICON(invincibility, invulntime)
	}

	if (invulntime > 7)
		offs -= ICONSEP;
	else
	{
		UINT8 a = ICONSEP, b = 7-invulntime;
		while (b--)
			a = 2*a/3;
		offs -= a;
	}

	// Super Sneakers
	if (stplyr->powers[pw_sneakers] > 3*TICRATE || (stplyr->powers[pw_sneakers] && leveltime & 1))
	{
		DRAWTIMERICON(sneakers, stplyr->powers[pw_sneakers])
	}

	if (stplyr->powers[pw_sneakers] > 7)
		offs -= ICONSEP;
	else
	{
		UINT8 a = ICONSEP, b = 7-stplyr->powers[pw_sneakers];
		while (b--)
			a = 2*a/3;
		offs -= a;
	}

	// Gravity Boots
	if (stplyr->powers[pw_gravityboots] > 3*TICRATE || (stplyr->powers[pw_gravityboots] && leveltime & 1))
	{
		DRAWTIMERICON(gravboots, stplyr->powers[pw_gravityboots])
	}

#undef DRAWTIMERICON
#undef ICONSEP
}

static void ST_drawFirstPersonHUD(void)
{
	patch_t *p = NULL;
	UINT32 airtime;
	spriteframe_t *sprframe;
	// If both air timers are active, use the air timer with the least time left
	if (stplyr->powers[pw_underwater] && stplyr->powers[pw_spacetime])
		airtime = min(stplyr->powers[pw_underwater], stplyr->powers[pw_spacetime]);
	else // Use whichever one is active otherwise
		airtime = (stplyr->powers[pw_spacetime]) ? stplyr->powers[pw_spacetime] : stplyr->powers[pw_underwater];

	if (airtime < 1)
		return; // No air timers are active, nothing would be drawn anyway

	airtime--; // The original code was all n*TICRATE + 1, so let's remove 1 tic for simplicity

	if (airtime > 11*TICRATE)
		return; // Not time to draw any drown numbers yet

	if (!((airtime > 10*TICRATE - 5)
	|| (airtime <= 9*TICRATE && airtime > 8*TICRATE - 5)
	|| (airtime <= 7*TICRATE && airtime > 6*TICRATE - 5)
	|| (airtime <= 5*TICRATE && airtime > 4*TICRATE - 5)
	|| (airtime <= 3*TICRATE && airtime > 2*TICRATE - 5)
	|| (airtime <= 1*TICRATE)))
		return; // Don't draw anything between numbers

	if (!((airtime % 10) < 5))
		return; // Keep in line with the flashing thing from third person.

	airtime /= (2*TICRATE); // To be strictly accurate it'd need to be ((airtime/TICRATE) - 1)/2, but integer division rounds down for us

	if (stplyr->charflags & SF_MACHINE)
		airtime += 6;  // Robots use different drown numbers

	// Get the front angle patch for the frame
	sprframe = &sprites[SPR_DRWN].spriteframes[airtime];
	p = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);

	// Display the countdown drown numbers!
	if (p && !F_GetPromptHideHud(60 - SHORT(p->topoffset)))
		V_DrawScaledPatch((BASEVIDWIDTH/2) - (SHORT(p->width)/2) + SHORT(p->leftoffset), 60 - SHORT(p->topoffset),
			V_PERPLAYER|V_PERPLAYER|V_TRANSLUCENT, p);
}

static void ST_drawNightsRecords(void)
{
	INT32 aflag = V_PERPLAYER;

	if (!stplyr->texttimer)
		return;

	if (stplyr->texttimer < TICRATE/2)
		aflag |= (9 - 9*stplyr->texttimer/(TICRATE/2)) << V_ALPHASHIFT;

	switch (stplyr->textvar)
	{
		case 1: // A "Bonus Time Start" by any other name...
		{
			V_DrawCenteredString(BASEVIDWIDTH/2, 52, V_GREENMAP|aflag, M_GetText("GET TO THE GOAL!"));
			V_DrawCenteredString(BASEVIDWIDTH/2, 60,            aflag, M_GetText("SCORE MULTIPLIER START!"));

			if (stplyr->finishedtime)
			{
				V_DrawString(BASEVIDWIDTH/2 - 48, 140, aflag, "TIME:");
				V_DrawString(BASEVIDWIDTH/2 - 48, 148, aflag, "BONUS:");
				V_DrawRightAlignedString(BASEVIDWIDTH/2 + 48, 140, V_ORANGEMAP|aflag, va("%d", (stplyr->startedtime - stplyr->finishedtime)/TICRATE));
				V_DrawRightAlignedString(BASEVIDWIDTH/2 + 48, 148, V_ORANGEMAP|aflag, va("%d", (stplyr->finishedtime/TICRATE) * 100));
			}
			break;
		}
		case 2: // Get n Spheres
		case 3: // Get n more Spheres
		{
			if (!stplyr->capsule)
				return;

			// Yes, this string is an abomination.
			V_DrawCenteredString(BASEVIDWIDTH/2, 60, aflag,
								 va(M_GetText("\x80GET\x82 %d\x80 %s%s%s!"), stplyr->capsule->health,
									(stplyr->textvar == 3) ? M_GetText("MORE ") : "",
									(G_IsSpecialStage(gamemap)) ? "SPHERE" : "CHIP",
									(stplyr->capsule->health > 1) ? "S" : ""));
			break;
		}
		case 4: // End Bonus
		{
			V_DrawString(BASEVIDWIDTH/2 - 56, 140, aflag, (G_IsSpecialStage(gamemap)) ? "SPHERES:" : "CHIPS:");
			V_DrawString(BASEVIDWIDTH/2 - 56, 148, aflag, "BONUS:");
			V_DrawRightAlignedString(BASEVIDWIDTH/2 + 56, 140, V_ORANGEMAP|aflag, va("%d", stplyr->finishedspheres));
			V_DrawRightAlignedString(BASEVIDWIDTH/2 + 56, 148, V_ORANGEMAP|aflag, va("%d", stplyr->finishedspheres * 50));
			ST_DrawNightsOverlayNum((BASEVIDWIDTH/2 + 56)<<FRACBITS, 160<<FRACBITS, FRACUNIT, aflag, stplyr->lastmarescore, nightsnum, SKINCOLOR_AZURE);

			// If new record, say so!
			if (!(netgame || multiplayer) && G_GetBestNightsScore(gamemap, stplyr->lastmare + 1) <= stplyr->lastmarescore)
			{
				if (stplyr->texttimer & 16)
					V_DrawCenteredString(BASEVIDWIDTH/2, 184, V_YELLOWMAP|aflag, "* NEW RECORD *");
			}

			if (P_HasGrades(gamemap, stplyr->lastmare + 1))
			{
				UINT8 grade = P_GetGrade(stplyr->lastmarescore, gamemap, stplyr->lastmare);
				if (modeattacking || grade >= GRADE_A)
					V_DrawTranslucentPatch(BASEVIDWIDTH/2 + 60, 160, aflag, ngradeletters[grade]);
			}
			break;
		}
		default:
			break;
	}
}

// 2.0-1: [21:42] <+Rob> Beige - Lavender - Steel Blue - Peach - Orange - Purple - Silver - Yellow - Pink - Red - Blue - Green - Cyan - Gold
/*#define NUMLINKCOLORS 14
static skincolornum_t linkColor[NUMLINKCOLORS] =
{SKINCOLOR_BEIGE,  SKINCOLOR_LAVENDER, SKINCOLOR_AZURE, SKINCOLOR_PEACH, SKINCOLOR_ORANGE,
 SKINCOLOR_MAGENTA, SKINCOLOR_SILVER, SKINCOLOR_SUPERGOLD4, SKINCOLOR_PINK,  SKINCOLOR_RED,
 SKINCOLOR_BLUE, SKINCOLOR_GREEN, SKINCOLOR_CYAN, SKINCOLOR_GOLD};*/

// 2.2 indev list: (unix time 1470866042) <Rob> Emerald, Aqua, Cyan, Blue, Pastel, Purple, Magenta, Rosy, Red, Orange, Gold, Yellow, Peridot
/*#define NUMLINKCOLORS 13
static skincolornum_t linkColor[NUMLINKCOLORS] =
{SKINCOLOR_EMERALD, SKINCOLOR_AQUA, SKINCOLOR_CYAN, SKINCOLOR_BLUE, SKINCOLOR_PASTEL,
 SKINCOLOR_PURPLE, SKINCOLOR_MAGENTA, SKINCOLOR_ROSY, SKINCOLOR_RED,  SKINCOLOR_ORANGE,
 SKINCOLOR_GOLD, SKINCOLOR_YELLOW, SKINCOLOR_PERIDOT};*/

// 2.2 indev list again: [19:59:52] <baldobo> Ruby > Red > Flame > Sunset > Orange > Gold > Yellow > Lime > Green > Aqua  > cyan > Sky > Blue > Pastel > Purple > Bubblegum > Magenta > Rosy > repeat
// [20:00:25] <baldobo> Also Icy for the link freeze text color
// [20:04:03] <baldobo> I would start it on lime
/*#define NUMLINKCOLORS 18
static skincolornum_t linkColor[NUMLINKCOLORS] =
{SKINCOLOR_LIME, SKINCOLOR_EMERALD, SKINCOLOR_AQUA, SKINCOLOR_CYAN, SKINCOLOR_SKY,
 SKINCOLOR_SAPPHIRE, SKINCOLOR_PASTEL, SKINCOLOR_PURPLE, SKINCOLOR_BUBBLEGUM, SKINCOLOR_MAGENTA,
 SKINCOLOR_ROSY, SKINCOLOR_RUBY, SKINCOLOR_RED, SKINCOLOR_FLAME, SKINCOLOR_SUNSET,
 SKINCOLOR_ORANGE, SKINCOLOR_GOLD, SKINCOLOR_YELLOW};*/

// 2.2+ list for real this time: https://wiki.srb2.org/wiki/User:Rob/Sandbox (check history around 31/10/17, spoopy)
#define NUMLINKCOLORS 12
static skincolornum_t linkColor[2][NUMLINKCOLORS] = {
{SKINCOLOR_EMERALD, SKINCOLOR_AQUA, SKINCOLOR_SKY, SKINCOLOR_BLUE, SKINCOLOR_PURPLE, SKINCOLOR_MAGENTA,
 SKINCOLOR_ROSY, SKINCOLOR_RED, SKINCOLOR_ORANGE, SKINCOLOR_GOLD, SKINCOLOR_YELLOW, SKINCOLOR_PERIDOT},
{SKINCOLOR_SEAFOAM, SKINCOLOR_CYAN, SKINCOLOR_WAVE, SKINCOLOR_SAPPHIRE, SKINCOLOR_VAPOR, SKINCOLOR_BUBBLEGUM,
 SKINCOLOR_VIOLET, SKINCOLOR_RUBY, SKINCOLOR_FLAME, SKINCOLOR_SUNSET, SKINCOLOR_SANDY, SKINCOLOR_LIME}};

static void ST_drawNiGHTSLink(void)
{
	static INT32 prevsel[2] = {0, 0}, prevtime[2] = {0, 0};
	const UINT8 q = ((splitscreen && stplyr == &players[secondarydisplayplayer]) ? 1 : 0);
	INT32 sel = ((stplyr->linkcount-1) / 5) % NUMLINKCOLORS, aflag = V_PERPLAYER, mag = ((stplyr->linkcount-1 >= 300) ? 1 : 0);
	skincolornum_t colornum;
	fixed_t x, y, scale;

	if (sel != prevsel[q])
	{
		prevsel[q] = sel;
		prevtime[q] = 2 + mag;
	}

	if (stplyr->powers[pw_nights_linkfreeze] && (!(stplyr->powers[pw_nights_linkfreeze] & 2) || (stplyr->powers[pw_nights_linkfreeze] > flashingtics)))
		colornum = SKINCOLOR_ICY;
	else
		colornum = linkColor[mag][sel];

	aflag |= ((stplyr->linktimer < (UINT32)nightslinktics/3)
	? (9 - 9*stplyr->linktimer/(nightslinktics/3)) << V_ALPHASHIFT
	: 0);

	y = (160+11)<<FRACBITS;
	aflag |= V_SNAPTOBOTTOM;

	x = (160+4)<<FRACBITS;

	if (prevtime[q])
	{
		scale = ((32 + prevtime[q])<<FRACBITS)/32;
		prevtime[q]--;
	}
	else
		scale = FRACUNIT;

	y -= (11*scale);

	ST_DrawNightsOverlayNum(x-(4*scale), y, scale, aflag, (stplyr->linkcount-1), nightsnum, colornum);
	V_DrawFixedPatch(x+(4*scale), y, scale, aflag, nightslink,
		colornum == 0 ? colormaps : R_GetTranslationColormap(TC_DEFAULT, colornum, GTC_CACHE));

	// Show remaining link time left in debug
	if (cv_debug & DBG_NIGHTSBASIC)
		V_DrawCenteredString(BASEVIDWIDTH/2, 180, V_SNAPTOBOTTOM, va("End in %d.%02d", stplyr->linktimer/TICRATE, G_TicsToCentiseconds(stplyr->linktimer)));
}


static void ST_drawNiGHTSHUD(void)
{
	INT32 origamount;
	INT32 total_spherecount, total_ringcount;
	const boolean oldspecialstage = (G_IsSpecialStage(gamemap) && !(maptol & TOL_NIGHTS));

	// Drill meter
	if (LUA_HudEnabled(hud_nightsdrill) && stplyr->powers[pw_carry] == CR_NIGHTSMODE)
	{
		INT32 locx = 16, locy = 180;
		INT32 dfill;
		UINT8 fillpatch;

		// Use which patch?
		if (stplyr->pflags & PF_DRILLING)
			fillpatch = (stplyr->drillmeter & 1) + 1;
		else
			fillpatch = 0;

		V_DrawScaledPatch(locx, locy, V_PERPLAYER|V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS, drillbar);
		for (dfill = 0; dfill < stplyr->drillmeter/20 && dfill < 96; ++dfill)
			V_DrawScaledPatch(locx + 2 + dfill, locy + 3, V_PERPLAYER|V_SNAPTOLEFT|V_SNAPTOBOTTOM|V_HUDTRANS, drillfill[fillpatch]);

		// Display actual drill amount and bumper time
		if (!splitscreen && (cv_debug & DBG_NIGHTSBASIC))
		{
			if (stplyr->bumpertime)
				V_DrawString(locx, locy - 8, V_REDMAP|V_MONOSPACE, va("BUMPER: 0.%02d", G_TicsToCentiseconds(stplyr->bumpertime)));
			else
				V_DrawString(locx, locy - 8, V_MONOSPACE, va("Drill: %3d%%", (stplyr->drillmeter*100)/(96*20)));
		}
	}

	/*if (G_IsSpecialStage(gamemap))
	{ // Since special stages share score, time, rings, etc.
		// disable splitscreen mode for its HUD.
		// --------------------------------------
		// NOPE! Consistency between different splitscreen stuffs
		// now we've got the screen squashing instead. ~toast
		if (stplyr != &players[displayplayer])
			return;
		nosshack = splitscreen;
		splitscreen = false;
	}*/

	// Link drawing
	if (!oldspecialstage
	// Don't display when the score is showing (it popping up for a split second when exiting a map is intentional)
	&& !(stplyr->texttimer && stplyr->textvar == 4)
	&& LUA_HudEnabled(hud_nightslink)
	&& ((cv_debug & DBG_NIGHTSBASIC) || stplyr->linkcount > 1)) // When debugging, show "0 Link".
	{
		ST_drawNiGHTSLink();
	}

	if (gametyperules & GTR_RACE)
	{
		ST_drawScore();
		ST_drawTime();
		return;
	}

	// Begin drawing brackets/chip display
	if (LUA_HudEnabled(hud_nightsspheres))
	{
	ST_DrawTopLeftOverlayPatch(16, 8, nbracket);
	if (G_IsSpecialStage(gamemap))
		ST_DrawTopLeftOverlayPatch(24, 16, (
			(stplyr->bonustime && (leveltime & 4) && (states[S_BLUESPHEREBONUS].frame & FF_ANIMATE)) ? nssbon : nsshud));
	else
		ST_DrawTopLeftOverlayPatch(24, 16, *(((stplyr->bonustime) ? nbon : nhud)+((leveltime/2)%12)));

	if (G_IsSpecialStage(gamemap))
	{
		INT32 i;
		total_spherecount = total_ringcount = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;
			total_spherecount += players[i].spheres;
			total_ringcount += players[i].rings;
		}
	}
	else
	{
		total_spherecount = stplyr->spheres;
		total_ringcount = stplyr->spheres;
	}

	if (stplyr->capsule)
	{
		INT32 amount;
		const INT32 length = 88;

		origamount = stplyr->capsule->spawnpoint->angle;
		I_Assert(origamount > 0); // should not happen now

		ST_DrawTopLeftOverlayPatch(72, 8, nbracket);
		ST_DrawTopLeftOverlayPatch(74, 8 + 4, minicaps);

		if (stplyr->capsule->reactiontime != 0)
		{
			INT32 r;
			const INT32 orblength = 20;

			for (r = 0; r < 5; r++)
			{
				V_DrawScaledPatch(230 - (7*r), 144, V_PERPLAYER|V_HUDTRANS, redstat);
				V_DrawScaledPatch(188 - (7*r), 144, V_PERPLAYER|V_HUDTRANS, orngstat);
				V_DrawScaledPatch(146 - (7*r), 144, V_PERPLAYER|V_HUDTRANS, yelstat);
				V_DrawScaledPatch(104 - (7*r), 144, V_PERPLAYER|V_HUDTRANS, byelstat);
			}

			amount = (origamount - stplyr->capsule->health);
			amount = (amount * orblength)/origamount;

			if (amount > 0)
			{
				INT32 t;

				// Fill up the bar with blue orbs... in reverse! (yuck)
				for (r = amount; r > 0; r--)
				{
					t = r;

					if (r > 15) ++t;
					if (r > 10) ++t;
					if (r > 5)  ++t;

					V_DrawScaledPatch(69 + (7*t), 144, V_PERPLAYER|V_HUDTRANS, bluestat);
				}
			}
		}
		else
		{
			INT32 cfill;

			// Lil' white box!
			V_DrawScaledPatch(15, 8 + 34, V_PERPLAYER|V_SNAPTOLEFT|V_SNAPTOTOP|V_HUDTRANS, capsulebar);

			amount = (origamount - stplyr->capsule->health);
			amount = (amount * length)/origamount;

			for (cfill = 0; cfill < amount && cfill < length; ++cfill)
				V_DrawScaledPatch(15 + cfill + 1, 8 + 35, V_PERPLAYER|V_SNAPTOLEFT|V_SNAPTOTOP|V_HUDTRANS, capsulefill);
		}

		if (total_spherecount >= stplyr->capsule->health)
			ST_DrawTopLeftOverlayPatch(40, 8 + 5, nredar[leveltime&7]);
		else
			ST_DrawTopLeftOverlayPatch(40, 8 + 5, narrow[(leveltime/2)&7]);
	}
	else if (oldspecialstage && total_spherecount < (INT32)ssspheres)
	{
		INT32 cfill, amount;
		const INT32 length = 88;
		UINT8 em = P_GetNextEmerald();
		ST_DrawTopLeftOverlayPatch(72, 8, nbracket);

		if (em <= 7)
			ST_DrawTopLeftOverlayPatch(80, 8 + 8, emeraldpics[0][em]);

		ST_DrawTopLeftOverlayPatch(40, 8 + 5, narrow[(leveltime/2)&7]);

		// Lil' white box!
		V_DrawScaledPatch(15, 8 + 34, V_PERPLAYER|V_SNAPTOLEFT|V_SNAPTOTOP|V_HUDTRANS, capsulebar);

		amount = (total_spherecount * length)/ssspheres;

		for (cfill = 0; cfill < amount && cfill < length; ++cfill)
			V_DrawScaledPatch(15 + cfill + 1, 8 + 35, V_PERPLAYER|V_SNAPTOLEFT|V_SNAPTOTOP|V_HUDTRANS, capsulefill);
	}
	else
		ST_DrawTopLeftOverlayPatch(40, 8 + 5, narrow[8]);

	if (oldspecialstage)
	{
		// invert for s3k style junk
		total_spherecount = ssspheres - total_spherecount;
		if (total_spherecount < 0)
			total_spherecount = 0;

		if (nummaprings > 0) // don't count down if there ISN'T a valid maximum number of rings, like sonic 3
		{
			total_ringcount = nummaprings - total_ringcount;
			if (total_ringcount < 0)
				total_ringcount = 0;
		}

		// now rings! you know, for that perfect bonus.
		V_DrawScaledPatch(272, 8, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTORIGHT|V_HUDTRANS, nbracket);
		V_DrawScaledPatch(280, 16+1, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTORIGHT|V_HUDTRANS, nring);
		V_DrawScaledPatch(280, 8+5, V_FLIP|V_PERPLAYER|V_SNAPTOTOP|V_SNAPTORIGHT|V_HUDTRANS, narrow[8]);
		V_DrawTallNum(272, 8 + 11, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTORIGHT|V_HUDTRANS, total_ringcount);
	}

	if (total_spherecount >= 100)
		V_DrawTallNum((total_spherecount >= 1000) ? 76 : 72, 8 + 11, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTOLEFT|V_HUDTRANS, total_spherecount);
	else
		V_DrawTallNum(68, 8 + 11, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTOLEFT|V_HUDTRANS, total_spherecount);
	}

	// Score
	if (!stplyr->exiting && !oldspecialstage && LUA_HudEnabled(hud_nightsscore))
		ST_DrawNightsOverlayNum(304<<FRACBITS, 14<<FRACBITS, FRACUNIT, V_PERPLAYER|V_SNAPTOTOP|V_SNAPTORIGHT, stplyr->marescore, nightsnum, SKINCOLOR_AZURE);

	// TODO give this its own section for Lua
	if (!stplyr->exiting && LUA_HudEnabled(hud_nightsscore))
	{
		if (modeattacking == ATTACKING_NIGHTS)
		{
			INT32 maretime = max(stplyr->realtime - stplyr->marebegunat, 0);

#define VFLAGS V_SNAPTOBOTTOM|V_SNAPTORIGHT|V_PERPLAYER|V_HUDTRANS
			V_DrawScaledPatch(BASEVIDWIDTH-22, BASEVIDHEIGHT-20, VFLAGS, W_CachePatchName("NGRTIMER", PU_HUDGFX));
			V_DrawPaddedTallNum(BASEVIDWIDTH-22, BASEVIDHEIGHT-20, VFLAGS, G_TicsToCentiseconds(maretime), 2);
			V_DrawScaledPatch(BASEVIDWIDTH-46, BASEVIDHEIGHT-20, VFLAGS, sboperiod);
			if (maretime < 60*TICRATE)
				V_DrawTallNum(BASEVIDWIDTH-46, BASEVIDHEIGHT-20, VFLAGS, G_TicsToSeconds(maretime));
			else
			{
				V_DrawPaddedTallNum(BASEVIDWIDTH-46, BASEVIDHEIGHT-20, VFLAGS, G_TicsToSeconds(maretime), 2);
				V_DrawScaledPatch(BASEVIDWIDTH-70, BASEVIDHEIGHT-20, VFLAGS, sbocolon);
				V_DrawTallNum(BASEVIDWIDTH-70, BASEVIDHEIGHT-20, VFLAGS, G_TicsToMinutes(maretime, true));
			}
#undef VFLAGS
		}
	}

	// Ideya time remaining
	if (!stplyr->exiting && stplyr->nightstime > 0 && LUA_HudEnabled(hud_nightstime))
	{
		INT32 realnightstime = stplyr->nightstime/TICRATE;
		INT32 numbersize;
		UINT8 col = ((realnightstime < 10) ? SKINCOLOR_RED : SKINCOLOR_SUPERGOLD4);

		if (G_IsSpecialStage(gamemap))
		{
			tic_t lowest_time = stplyr->nightstime;
			INT32 i;
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && players[i].powers[pw_carry] == CR_NIGHTSMODE && players[i].nightstime < lowest_time)
					lowest_time = players[i].nightstime;
			realnightstime = lowest_time/TICRATE;
		}

		if (stplyr->powers[pw_flashing] > TICRATE) // was hit
		{
			UINT16 flashingLeft = stplyr->powers[pw_flashing]-(TICRATE);
			if (flashingLeft < TICRATE/2) // Start fading out
			{
				UINT32 fadingFlag = (9 - 9*flashingLeft/(TICRATE/2)) << V_ALPHASHIFT;
				V_DrawTranslucentPatch(160 - (minus5sec->width/2), 28, V_PERPLAYER|fadingFlag, minus5sec);
			}
			else
				V_DrawScaledPatch(160 - (minus5sec->width/2), 28, V_PERPLAYER, minus5sec);
		}

		if (realnightstime < 10)
			numbersize = 16/2;
		else if (realnightstime < 100)
			numbersize = 32/2;
		else
			numbersize = 48/2;

		if ((oldspecialstage && leveltime & 2)
			&& (stplyr->mo->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER))
			&& !(stplyr->powers[pw_shield] & SH_PROTECTWATER))
			col = SKINCOLOR_ORANGE;

		ST_DrawNightsOverlayNum((160 + numbersize)<<FRACBITS, 14<<FRACBITS, FRACUNIT, V_PERPLAYER|V_SNAPTOTOP, realnightstime, nightsnum, col);

		// Show exact time in debug
		if (cv_debug & DBG_NIGHTSBASIC)
			V_DrawString(160 + numbersize + 8, 24, V_SNAPTOTOP|((realnightstime < 10) ? V_REDMAP : V_YELLOWMAP), va("%02d", G_TicsToCentiseconds(stplyr->nightstime)));
	}

	if (oldspecialstage)
	{
		if (leveltime < 5*TICRATE)
		{
			INT32 aflag = V_PERPLAYER;
			tic_t drawtime = (5*TICRATE) - leveltime;
			if (drawtime < TICRATE/2)
				aflag |= (9 - 9*drawtime/(TICRATE/2)) << V_ALPHASHIFT;
			// This one, not quite as much so.
			V_DrawCenteredString(BASEVIDWIDTH/2, 60, aflag,
		                     va(M_GetText("\x80GET\x82 %d\x80 SPHERE%s!"), ssspheres,
		                        (ssspheres > 1) ? "S" : ""));
		}
	}
	else
	{
		// Show pickup durations
		if (cv_debug & DBG_NIGHTSBASIC)
		{
			UINT16 pwr;

			if (stplyr->powers[pw_nights_superloop])
			{
				pwr = stplyr->powers[pw_nights_superloop];
				V_DrawSmallScaledPatch(110, 44, 0, W_CachePatchName("NPRUA0",PU_CACHE));
				V_DrawThinString(106, 52, V_MONOSPACE, va("%2d.%02d", pwr/TICRATE, G_TicsToCentiseconds(pwr)));
			}

			if (stplyr->powers[pw_nights_helper])
			{
				pwr = stplyr->powers[pw_nights_helper];
				V_DrawSmallScaledPatch(150, 44, 0, W_CachePatchName("NPRUC0",PU_CACHE));
				V_DrawThinString(146, 52, V_MONOSPACE, va("%2d.%02d", pwr/TICRATE, G_TicsToCentiseconds(pwr)));
			}

			if (stplyr->powers[pw_nights_linkfreeze])
			{
				pwr = stplyr->powers[pw_nights_linkfreeze];
				V_DrawSmallScaledPatch(190, 44, 0, W_CachePatchName("NPRUE0",PU_CACHE));
				V_DrawThinString(186, 52, V_MONOSPACE, va("%2d.%02d", pwr/TICRATE, G_TicsToCentiseconds(pwr)));
			}
		}

		// Records/extra text
		if (LUA_HudEnabled(hud_nightsrecords))
			ST_drawNightsRecords();
	}
}

static void ST_drawWeaponSelect(INT32 xoffs, INT32 y)
{
	INT32 q = stplyr->weapondelay, del = 0, p = 16;
	while (q)
	{
		if (q > p)
		{
			del += p;
			q -= p;
			q /= 2;
			if (p > 1)
				p /= 2;
		}
		else
		{
			del += q;
			break;
		}
	}
	V_DrawScaledPatch(6 + xoffs, y-2 - del/2, V_PERPLAYER|V_SNAPTOBOTTOM, curweapon);
}

static void ST_drawWeaponRing(powertype_t weapon, INT32 rwflag, INT32 wepflag, INT32 xoffs, INT32 y, patch_t *pat)
{
	INT32 txtflags = 0, patflags = 0;

	if (stplyr->powers[weapon])
	{
		if (stplyr->powers[weapon] >= rw_maximums[wepflag])
			txtflags |= V_YELLOWMAP;

		if (weapon == pw_infinityring
		|| (stplyr->ringweapons & rwflag))
			; //txtflags |= V_20TRANS;
		else
		{
			txtflags |= V_TRANSLUCENT;
			patflags =  V_80TRANS;
		}

		V_DrawScaledPatch(8 + xoffs, y, V_PERPLAYER|V_SNAPTOBOTTOM|patflags, pat);
		V_DrawRightAlignedThinString(24 + xoffs, y + 8, V_PERPLAYER|V_SNAPTOBOTTOM|txtflags, va("%d", stplyr->powers[weapon]));

		if (stplyr->currentweapon == wepflag)
			ST_drawWeaponSelect(xoffs, y);
	}
	else if (stplyr->ringweapons & rwflag)
		V_DrawScaledPatch(8 + xoffs, y, V_PERPLAYER|V_SNAPTOBOTTOM|V_TRANSLUCENT, pat);
}

static void ST_drawMatchHUD(void)
{
	char penaltystr[7];
	const INT32 y = 176; // HUD_LIVES
	INT32 offset = (BASEVIDWIDTH / 2) - (NUM_WEAPONS * 10) - 6;

	if (F_GetPromptHideHud(y))
		return;

	if (!G_RingSlingerGametype())
		return;

	if (G_TagGametype() && !(stplyr->pflags & PF_TAGIT))
		return;

	{
		if (stplyr->powers[pw_infinityring])
			ST_drawWeaponRing(pw_infinityring, 0, 0, offset, y, infinityring);
		else
		{
			if (stplyr->rings > 0)
				V_DrawScaledPatch(8 + offset, y, V_PERPLAYER|V_SNAPTOBOTTOM, normring);
			else
				V_DrawTranslucentPatch(8 + offset, y, V_PERPLAYER|V_SNAPTOBOTTOM|V_80TRANS, normring);

			if (!stplyr->currentweapon)
				ST_drawWeaponSelect(offset, y);
		}

		ST_drawWeaponRing(pw_automaticring, RW_AUTO, WEP_AUTO, offset + 20, y, autoring);
		ST_drawWeaponRing(pw_bouncering, RW_BOUNCE, WEP_BOUNCE, offset + 40, y, bouncering);
		ST_drawWeaponRing(pw_scatterring, RW_SCATTER, WEP_SCATTER, offset + 60, y, scatterring);
		ST_drawWeaponRing(pw_grenadering, RW_GRENADE, WEP_GRENADE, offset + 80, y, grenadering);
		ST_drawWeaponRing(pw_explosionring, RW_EXPLODE, WEP_EXPLODE, offset + 100, y, explosionring);
		ST_drawWeaponRing(pw_railring, RW_RAIL, WEP_RAIL, offset + 120, y, railring);

		if (stplyr->ammoremovaltimer && leveltime % 8 < 4)
		{
			sprintf(penaltystr, "-%d", stplyr->ammoremoval);
			V_DrawString(offset + 8 + stplyr->ammoremovalweapon * 20, y,
					V_REDMAP, penaltystr);
		}

	}
}

static void ST_drawTextHUD(void)
{
	INT32 y = 42 + 16; // HUD_RINGS
	boolean donef12 = false;

#define textHUDdraw(str) \
{\
	V_DrawThinString(16, y, V_PERPLAYER|V_HUDTRANS|V_SNAPTOLEFT|V_SNAPTOTOP, str);\
	y += 8;\
}

	if (F_GetPromptHideHud(y))
		return;

	if (stplyr->spectator && (!G_CoopGametype() || stplyr->playerstate == PST_LIVE))
		textHUDdraw(M_GetText("\x86""Spectator mode:"))

	if (circuitmap)
	{
		if (stplyr->exiting)
			textHUDdraw(M_GetText("\x82""FINISHED!"))
		else
			textHUDdraw(va("Lap:""\x82 %u/%d", stplyr->laps+1, cv_numlaps.value))
	}

	if (!G_CoopGametype() && (stplyr->exiting || (G_GametypeUsesLives() && stplyr->lives <= 0 && countdown != 1)))
	{
		if (!splitscreen && !donef12)
		{
			textHUDdraw(M_GetText("\x82""VIEWPOINT:""\x80 Switch view"))
			donef12 = true;
		}
	}
	else if ((gametyperules & GTR_RESPAWNDELAY) && stplyr->playerstate == PST_DEAD && stplyr->lives) // Death overrides spectator text.
	{
		INT32 respawntime = cv_respawntime.value - stplyr->deadtimer/TICRATE;

		if (respawntime > 0 && !stplyr->spectator)
			textHUDdraw(va(M_GetText("Respawn in %d..."), respawntime))
		else
			textHUDdraw(M_GetText("\x82""JUMP:""\x80 Respawn"))
	}
	else if (stplyr->spectator && (!G_CoopGametype() || stplyr->playerstate == PST_LIVE))
	{
		if (!splitscreen && !donef12)
		{
			textHUDdraw(M_GetText("\x82""VIEWPOINT:""\x80 Switch view"))
			donef12 = true;
		}

		textHUDdraw(M_GetText("\x82""JUMP:""\x80 Rise"))
		textHUDdraw(M_GetText("\x82""SPIN:""\x80 Lower"))

		if (G_IsSpecialStage(gamemap))
			textHUDdraw(M_GetText("\x82""Wait for the stage to end..."))
		else if (G_PlatformGametype())
		{
			if (G_GametypeUsesCoopLives())
			{
				if (stplyr->lives <= 0
				&& cv_cooplives.value == 2
				&& (netgame || multiplayer))
				{
					INT32 i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;

						if (&players[i] == stplyr)
							continue;

						if (players[i].lives > 1)
							break;
						}

					if (i != MAXPLAYERS)
						textHUDdraw(M_GetText("You'll steal a life on respawn..."))
					else
						textHUDdraw(M_GetText("Wait to respawn..."))
				}
				else
					textHUDdraw(M_GetText("Wait to respawn..."))
			}
		}
		else if (G_GametypeHasSpectators())
			textHUDdraw(M_GetText("\x82""FIRE:""\x80 Enter game"))
	}

	if (G_CoopGametype() && (!stplyr->spectator || (!(maptol & TOL_NIGHTS) && G_IsSpecialStage(gamemap))) && (stplyr->exiting || (stplyr->pflags & PF_FINISHED)))
	{
		UINT8 numneeded = (G_IsSpecialStage(gamemap) ? 4 : cv_playersforexit.value);
		if (numneeded)
		{
			INT32 i, total = 0, exiting = 0;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;
				if (players[i].lives <= 0)
					continue;

				total++;
				if (players[i].exiting || (players[i].pflags & PF_FINISHED))
					exiting++;
			}

			if (numneeded != 4)
			{
				total *= cv_playersforexit.value;
				if (total & 3)
					total += 4; // round up
				total /= 4;
			}

			if (exiting < total)
			{
				if (!splitscreen && !donef12)
				{
					textHUDdraw(M_GetText("\x82""VIEWPOINT:""\x80 Switch view"))
					donef12 = true;
				}
				total -= exiting;
				textHUDdraw(va(M_GetText("%d player%s remaining"), total, ((total == 1) ? "" : "s")))
			}
		}
	}
	else if ((gametyperules & GTR_TAG) && (!stplyr->spectator))
	{
		if (leveltime < hidetime * TICRATE)
		{
			if (stplyr->pflags & PF_TAGIT)
			{
				if (gametyperules & GTR_BLINDFOLDED)
					textHUDdraw(M_GetText("\x82""You are blindfolded!"))
				textHUDdraw(M_GetText("Waiting for players to hide..."))
			}
			else if (gametyperules & GTR_HIDEFROZEN)
				textHUDdraw(M_GetText("Hide before time runs out!"))
			else
				textHUDdraw(M_GetText("Flee before you are hunted!"))
		}
		else if ((gametyperules & GTR_HIDEFROZEN) && !(stplyr->pflags & PF_TAGIT))
		{
			if (!splitscreen && !donef12)
			{
				textHUDdraw(M_GetText("\x82""VIEWPOINT:""\x80 Switch view"))
				donef12 = true;
			}
			textHUDdraw(M_GetText("You cannot move while hiding."))
		}
	}

#undef textHUDdraw

}

static inline void ST_drawRaceHUD(void)
{
	if (leveltime > TICRATE && leveltime <= 5*TICRATE)
		ST_drawRaceNum(4*TICRATE - leveltime);
}

static void ST_drawTeamHUD(void)
{
	patch_t *p;
#define SEP 20

	if (F_GetPromptHideHud(0)) // y base is 0
		return;

	if (gametyperules & GTR_TEAMFLAGS)
		p = bflagico;
	else
		p = bmatcico;

	if (LUA_HudEnabled(hud_teamscores))
		V_DrawSmallScaledPatch(BASEVIDWIDTH/2 - SEP - SHORT(p->width)/4, 4, V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, p);

	if (gametyperules & GTR_TEAMFLAGS)
		p = rflagico;
	else
		p = rmatcico;

	if (LUA_HudEnabled(hud_teamscores))
		V_DrawSmallScaledPatch(BASEVIDWIDTH/2 + SEP - SHORT(p->width)/4, 4, V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, p);

	if (!(gametyperules & GTR_TEAMFLAGS))
		goto num;
	{
		INT32 i;
		UINT16 whichflag = 0;

		// Show which flags aren't at base.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			// Blue flag isn't at base
			if (players[i].gotflag & GF_BLUEFLAG && LUA_HudEnabled(hud_teamscores))
				V_DrawScaledPatch(BASEVIDWIDTH/2 - SEP - SHORT(nonicon->width)/2, 0, V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, nonicon);

			// Red flag isn't at base
			if (players[i].gotflag & GF_REDFLAG && LUA_HudEnabled(hud_teamscores))
				V_DrawScaledPatch(BASEVIDWIDTH/2 + SEP - SHORT(nonicon2->width)/2, 0, V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, nonicon2);

			whichflag |= players[i].gotflag;

			if ((whichflag & (GF_REDFLAG|GF_BLUEFLAG)) == (GF_REDFLAG|GF_BLUEFLAG))
				break; // both flags were found, let's stop early
		}

		// Display a countdown timer showing how much time left until the flag returns to base.
		{
			if (blueflag && blueflag->fuse > 1 && LUA_HudEnabled(hud_teamscores))
				V_DrawCenteredString(BASEVIDWIDTH/2 - SEP, 8, V_YELLOWMAP|V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, va("%u", (blueflag->fuse / TICRATE)));

			if (redflag && redflag->fuse > 1 && LUA_HudEnabled(hud_teamscores))
				V_DrawCenteredString(BASEVIDWIDTH/2 + SEP, 8, V_YELLOWMAP|V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, va("%u", (redflag->fuse / TICRATE)));
		}
	}

num:
	if (LUA_HudEnabled(hud_teamscores))
		V_DrawCenteredString(BASEVIDWIDTH/2 - SEP, 16, V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, va("%u", bluescore));

	if (LUA_HudEnabled(hud_teamscores))
		V_DrawCenteredString(BASEVIDWIDTH/2 + SEP, 16, V_HUDTRANS|V_PERPLAYER|V_SNAPTOTOP, va("%u", redscore));

#undef SEP
}

/*static void ST_drawSpecialStageHUD(void)
{
	if (ssspheres > 0)
	{
		if (hudinfo[HUD_SS_TOTALRINGS].x)
			ST_DrawNumFromHud(HUD_SS_TOTALRINGS, ssspheres, V_HUDTRANS);
		else if (cv_timetic.value == 2)
			V_DrawTallNum(hudinfo[HUD_RINGSNUMTICS].x, hudinfo[HUD_SS_TOTALRINGS].y, hudinfo[HUD_RINGSNUMTICS].f|V_PERPLAYER|V_HUDTRANS, ssspheres);
		else
			V_DrawTallNum(hudinfo[HUD_RINGSNUM].x, hudinfo[HUD_SS_TOTALRINGS].y, hudinfo[HUD_RINGSNUM].f|V_PERPLAYER|V_HUDTRANS, ssspheres);
	}

	if (leveltime < 5*TICRATE && ssspheres > 0)
	{
		ST_DrawPatchFromHud(HUD_GETRINGS, getall, V_HUDTRANS);
		ST_DrawNumFromHud(HUD_GETRINGSNUM, ssspheres, V_HUDTRANS);
	}

	if (sstimer)
	{
		V_DrawString(hudinfo[HUD_TIMELEFT].x, hudinfo[HUD_TIMELEFT].y, hudinfo[HUD_TIMELEFT].f|V_PERPLAYER|V_HUDTRANS, M_GetText("TIME LEFT"));
		ST_DrawNumFromHud(HUD_TIMELEFTNUM, sstimer/TICRATE, V_HUDTRANS);
	}
	else
		ST_DrawPatchFromHud(HUD_TIMEUP, timeup, V_HUDTRANS);
}*/

static INT32 ST_drawEmeraldHuntIcon(mobj_t *hunt, patch_t **patches, INT32 offset)
{
	INT32 interval, i;
	UINT32 dist = ((UINT32)P_AproxDistance(P_AproxDistance(stplyr->mo->x - hunt->x, stplyr->mo->y - hunt->y), stplyr->mo->z - hunt->z))>>FRACBITS;

	if (dist < 128)
	{
		i = 5;
		interval = 5;
	}
	else if (dist < 512)
	{
		i = 4;
		interval = 10;
	}
	else if (dist < 1024)
	{
		i = 3;
		interval = 20;
	}
	else if (dist < 2048)
	{
		i = 2;
		interval = 30;
	}
	else if (dist < 3072)
	{
		i = 1;
		interval = 35;
	}
	else
	{
		i = 0;
		interval = 0;
	}

	if (!F_GetPromptHideHud(hudinfo[HUD_HUNTPICS].y))
		V_DrawScaledPatch(hudinfo[HUD_HUNTPICS].x+offset, hudinfo[HUD_HUNTPICS].y, hudinfo[HUD_HUNTPICS].f|V_PERPLAYER|V_HUDTRANS, patches[i]);
	return interval;
}

// Separated a few things to stop the SOUND EFFECTS BLARING UGH SHUT UP AAAA
static void ST_doHuntIconsAndSound(void)
{
	INT32 interval = 0, newinterval = 0;

	if (hunt1 && hunt1->health)
		interval = ST_drawEmeraldHuntIcon(hunt1, hunthoming, -20);

	if (hunt2 && hunt2->health)
	{
		newinterval = ST_drawEmeraldHuntIcon(hunt2, hunthoming, 0);
		if (newinterval && (!interval || newinterval < interval))
			interval = newinterval;
	}

	if (hunt3 && hunt3->health)
	{
		newinterval = ST_drawEmeraldHuntIcon(hunt3, hunthoming, 20);
		if (newinterval && (!interval || newinterval < interval))
			interval = newinterval;
	}

	if (!(P_AutoPause() || paused) && interval > 0 && leveltime && leveltime % interval == 0)
		S_StartSound(NULL, sfx_emfind);
}

static void ST_doItemFinderIconsAndSound(void)
{
	INT32 emblems[16];
	thinker_t *th;
	mobj_t *mo2;

	UINT8 stemblems = 0, stunfound = 0;
	INT32 i;
	INT32 interval = 0, newinterval = 0;
	INT32 soffset;

	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].type > ET_SKIN || emblemlocations[i].level != gamemap)
			continue;

		emblems[stemblems++] = i;

		if (!emblemlocations[i].collected)
			++stunfound;

		if (stemblems >= 16)
			break;
	}
	// Found all/none exist? Don't waste our time
	if (!stunfound)
		return;

	// Scan thinkers to find emblem mobj with these ids
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type != MT_EMBLEM)
			continue;

		if (!(mo2->flags & MF_SPECIAL))
			continue;

		for (i = 0; i < stemblems; ++i)
		{
			if (mo2->health == emblems[i] + 1)
			{
				soffset = (i * 20) - ((stemblems - 1) * 10);

				newinterval = ST_drawEmeraldHuntIcon(mo2, itemhoming, soffset);
				if (newinterval && (!interval || newinterval < interval))
					interval = newinterval;

				break;
			}
		}

	}

	if (!(P_AutoPause() || paused) && interval > 0 && leveltime && leveltime % interval == 0)
		S_StartSound(NULL, sfx_emfind);
}

//
// Draw the status bar overlay, customisable: the user chooses which
// kind of information to overlay
//
static void ST_overlayDrawer(void)
{
	// Decide whether to draw the stage title or not
	boolean stagetitle = false;

	// Check for a valid level title
	// If the HUD is enabled
	// And, if Lua is running, if the HUD library has the stage title enabled
	if (G_IsTitleCardAvailable() && *mapheaderinfo[gamemap-1]->lvlttl != '\0' && !(hu_showscores && (netgame || multiplayer)))
	{
		stagetitle = true;
		ST_preDrawTitleCard();
	}

	// hu_showscores = auto hide score/time/rings when tab rankings are shown
	if (!(hu_showscores && (netgame || multiplayer)))
	{
		if ((maptol & TOL_NIGHTS || G_IsSpecialStage(gamemap)) &&
			!F_GetPromptHideHudAll())
			ST_drawNiGHTSHUD();
		else
		{
			if (LUA_HudEnabled(hud_score))
				ST_drawScore();
			if (LUA_HudEnabled(hud_time))
				ST_drawTime();
			if (LUA_HudEnabled(hud_rings))
				ST_drawRings();

			if (!modeattacking && LUA_HudEnabled(hud_lives))
				ST_drawLivesArea();
		}
	}

	// GAME OVER hud
	if (G_GametypeUsesCoopLives()
		&& (netgame || multiplayer)
		&& (cv_cooplives.value == 0))
	;
	else if ((G_GametypeUsesLives() || ((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE)) && stplyr->lives <= 0 && !(hu_showscores && (netgame || multiplayer)))
	{
		INT32 i = MAXPLAYERS;
		INT32 deadtimer = stplyr->spectator ? TICRATE : (stplyr->deadtimer-(TICRATE<<1));

		if (G_GametypeUsesCoopLives()
		&& (netgame || multiplayer)
		&& (cv_cooplives.value != 1))
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				if (&players[i] == stplyr)
					continue;

				if (players[i].lives > 0)
					break;
			}
		}

		if (i == MAXPLAYERS && deadtimer >= 0)
		{
			INT32 lvlttlx = min(6*deadtimer, BASEVIDWIDTH/2);
			UINT32 flags = V_PERPLAYER|(stplyr->spectator ? V_HUDTRANSHALF : V_HUDTRANS);

			V_DrawScaledPatch(lvlttlx - 8, BASEVIDHEIGHT/2, flags, (countdown == 1 ? slidtime : slidgame));
			V_DrawScaledPatch(BASEVIDWIDTH + 8 - lvlttlx, BASEVIDHEIGHT/2, flags, slidover);
		}
	}

	if (G_GametypeHasTeams())
		ST_drawTeamHUD();

	if (!hu_showscores) // hide the following if TAB is held
	{
		// Countdown timer for Race Mode
		if (countdown > 1)
		{
			tic_t time = countdown/TICRATE + 1;
			if (time < 4)
				ST_drawRaceNum(countdown);
			else
			{
				tic_t num = time;
				INT32 sz = SHORT(tallnum[0]->width)/2, width = 0;
				do
				{
					width += sz;
					num /= 10;
				} while (num);
				V_DrawTallNum((BASEVIDWIDTH/2) + width, ((3*BASEVIDHEIGHT)>>2) - 7, V_PERPLAYER, time);
				//V_DrawCenteredString(BASEVIDWIDTH/2, 176, V_PERPLAYER, va("%d", countdown/TICRATE + 1));
			}
		}

		// If you are in overtime, put a big honkin' flashin' message on the screen.
		if (((gametyperules & GTR_TIMELIMIT) && (gametyperules & GTR_OVERTIME)) && cv_overtime.value
		&& (leveltime > (timelimitintics + TICRATE/2)) && cv_timelimit.value && (leveltime/TICRATE % 2 == 0))
			V_DrawCenteredString(BASEVIDWIDTH/2, 184, V_PERPLAYER, M_GetText("OVERTIME!"));

		// Draw Match-related stuff
		//\note Match HUD is drawn no matter what gametype.
		// ... just not if you're a spectator.
		if (!stplyr->spectator && LUA_HudEnabled(hud_weaponrings))
			ST_drawMatchHUD();

		// Race HUD Stuff
		if (gametyperules & GTR_RACE)
			ST_drawRaceHUD();

		// Emerald Hunt Indicators
		if (cv_itemfinder.value && M_SecretUnlocked(SECRET_ITEMFINDER))
			ST_doItemFinderIconsAndSound();
		else
			ST_doHuntIconsAndSound();

		if(!P_IsLocalPlayer(stplyr))
		{
			char name[MAXPLAYERNAME+1];
			// shorten the name if its more than twelve characters.
			strlcpy(name, player_names[stplyr-players], 13);

			// Show name of player being displayed
			V_DrawCenteredString((BASEVIDWIDTH/6), BASEVIDHEIGHT-80, 0, M_GetText("Viewpoint:"));
			V_DrawCenteredString((BASEVIDWIDTH/6), BASEVIDHEIGHT-64, V_ALLOWLOWERCASE, name);
		}

		// This is where we draw all the fun cheese if you have the chasecam off!
		if (!(maptol & TOL_NIGHTS))
		{
			if ((stplyr == &players[displayplayer] && !camera.chase)
			|| ((splitscreen && stplyr == &players[secondarydisplayplayer]) && !camera2.chase))
			{
				ST_drawFirstPersonHUD();
				if (cv_powerupdisplay.value)
					ST_drawPowerupHUD();  // same as it ever was...
			}
			else if (cv_powerupdisplay.value == 2)
				ST_drawPowerupHUD();  // same as it ever was...
		}
	}
	else if (!(netgame || multiplayer) && cv_powerupdisplay.value == 2)
		ST_drawPowerupHUD(); // same as it ever was...

	if (!(netgame || multiplayer) || !hu_showscores)
		LUAh_GameHUD(stplyr);

	// draw level title Tails
	if (stagetitle && (!WipeInAction) && (!WipeStageTitle))
		ST_drawTitleCard();

	if (!hu_showscores && (netgame || multiplayer) && LUA_HudEnabled(hud_textspectator))
		ST_drawTextHUD();

	if (modeattacking && !(demoplayback && hu_showscores))
		ST_drawInput();

	ST_drawDebugInfo();
}

void ST_Drawer(void)
{
	if (needpatchrecache)
		R_ReloadHUDGraphics();

#ifdef SEENAMES
	if (cv_seenames.value && cv_allowseenames.value && displayplayer == consoleplayer && seenplayer && seenplayer->mo)
	{
		INT32 c = 0;
		switch (cv_seenames.value)
		{
			case 1: // Colorless
				break;
			case 2: // Team
				if (G_GametypeHasTeams())
					c = (seenplayer->ctfteam == 1) ? V_REDMAP : V_BLUEMAP;
				break;
			case 3: // Ally/Foe
			default:
				// Green = Ally, Red = Foe
				if (G_GametypeHasTeams())
					c = (players[consoleplayer].ctfteam == seenplayer->ctfteam) ? V_GREENMAP : V_REDMAP;
				else // Everyone is an ally, or everyone is a foe!
					c = (G_RingSlingerGametype()) ? V_REDMAP : V_GREENMAP;
				break;
		}

		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2 + 15, V_HUDTRANSHALF|c, player_names[seenplayer-players]);
	}
#endif

	// Doom's status bar only updated if necessary.
	// However, ours updates every frame regardless, so the "refresh" param was removed
	//(void)refresh;

	// force a set of the palette by using doPaletteStuff()
	if (vid.recalc)
		st_palette = -1;

	// Do red-/gold-shifts from damage/items
#ifdef HWRENDER
	//25/08/99: Hurdler: palette changes is done for all players,
	//                   not only player1! That's why this part
	//                   of code is moved somewhere else.
	if (rendermode == render_soft)
#endif
		if (rendermode != render_none) ST_doPaletteStuff();

	// Blindfold!
	if ((gametyperules & GTR_BLINDFOLDED)
	&& (leveltime < hidetime * TICRATE))
	{
		if (players[displayplayer].pflags & PF_TAGIT)
		{
			stplyr = &players[displayplayer];
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31|V_PERPLAYER);
		}
		else if (splitscreen && players[secondarydisplayplayer].pflags & PF_TAGIT)
		{
			stplyr = &players[secondarydisplayplayer];
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31|V_PERPLAYER);
		}
	}

	st_translucency = cv_translucenthud.value;

	if (st_overlay)
	{
		// No deadview!
		stplyr = &players[displayplayer];
		ST_overlayDrawer();

		if (splitscreen)
		{
			stplyr = &players[secondarydisplayplayer];
			ST_overlayDrawer();
		}
	}
}
