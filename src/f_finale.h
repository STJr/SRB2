// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_finale.h
/// \brief Title screen, intro, game evaluation, and credits.
///        Also includes protos for screen wipe functions.

#ifndef __F_FINALE__
#define __F_FINALE__

#include "doomtype.h"
#include "d_event.h"
#include "p_mobj.h"

//
// FINALE
//

// Called by main loop.
boolean F_IntroResponder(event_t *ev);
boolean F_CutsceneResponder(event_t *ev);
boolean F_CreditResponder(event_t *ev);

// Called by main loop.
void F_GameEndTicker(void);
void F_IntroTicker(void);
void F_TitleScreenTicker(boolean run);
void F_CutsceneTicker(void);
void F_TitleDemoTicker(void);
void F_TextPromptTicker(void);

// Called by main loop.
void F_GameEndDrawer(void);
void F_IntroDrawer(void);
void F_TitleScreenDrawer(void);
void F_SkyScroll(INT32 scrollxspeed, INT32 scrollyspeed, const char *patchname);

void F_GameEvaluationDrawer(void);
void F_StartGameEvaluation(void);
void F_GameEvaluationTicker(void);

void F_EndingTicker(void);
void F_EndingDrawer(void);

void F_CreditTicker(void);
void F_CreditDrawer(void);

void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer);
void F_CutsceneDrawer(void);
void F_EndCutScene(void);

void F_StartTextPrompt(INT32 promptnum, INT32 pagenum, mobj_t *mo, UINT16 postexectag, boolean blockcontrols, boolean freezerealtime);
void F_GetPromptPageByNamedTag(const char *tag, INT32 *promptnum, INT32 *pagenum);
void F_TextPromptDrawer(void);
void F_EndTextPrompt(boolean forceexec, boolean noexec);
boolean F_GetPromptHideHudAll(void);
boolean F_GetPromptHideHud(fixed_t y);

void F_StartGameEnd(void);
void F_StartIntro(void);
void F_StartTitleScreen(void);
void F_StartEnding(void);
void F_StartCredits(void);

boolean F_ContinueResponder(event_t *event);
void F_StartContinue(void);
void F_ContinueTicker(void);
void F_ContinueDrawer(void);

extern INT32 finalecount;
extern INT32 titlescrollxspeed;
extern INT32 titlescrollyspeed;

typedef enum
{
	TTMODE_NONE = 0,
	TTMODE_OLD,
	TTMODE_ALACROIX,
	TTMODE_USER
} ttmode_enum;

#define TTMAX_ALACROIX 30 // max frames for SONIC typeface, plus one for NULL terminating entry
#define TTMAX_USER 100

extern ttmode_enum ttmode;
extern UINT8 ttscale;
// ttmode user vars
extern char ttname[9];
extern INT16 ttx;
extern INT16 tty;
extern INT16 ttloop;
extern UINT16 tttics;
extern boolean ttavailable[6];


typedef enum
{
	TITLEMAP_OFF = 0,
	TITLEMAP_LOADING,
	TITLEMAP_RUNNING
} titlemap_enum;

// Current menu parameters

extern mobj_t *titlemapcameraref;
extern char curbgname[9];
extern SINT8 curfadevalue;
extern INT32 curbgcolor;
extern INT32 curbgxspeed;
extern INT32 curbgyspeed;
extern boolean curbghide;
extern boolean hidetitlemap;

extern boolean curhidepics;
extern ttmode_enum curttmode;
extern UINT8 curttscale;
// ttmode user vars
extern char curttname[9];
extern INT16 curttx;
extern INT16 curtty;
extern INT16 curttloop;
extern UINT16 curtttics;

#define TITLEBACKGROUNDACTIVE (curfadevalue >= 0 || curbgname[0])

void F_InitMenuPresValues(void);
void F_MenuPresTicker(boolean run);

//
// WIPE
//
// HACK for menu fading while titlemapinaction; skips the level check
#define FORCEWIPE -3
#define FORCEWIPEOFF -2

extern boolean WipeInAction;
extern boolean WipeStageTitle;

typedef enum
{
	WIPESTYLE_NORMAL,
	WIPESTYLE_COLORMAP
} wipestyle_t;
extern wipestyle_t wipestyle;

typedef enum
{
	WSF_FADEOUT   = 1,
	WSF_FADEIN    = 1<<1,
	WSF_TOWHITE   = 1<<2,
	WSF_CROSSFADE = 1<<3,
} wipestyleflags_t;
extern wipestyleflags_t wipestyleflags;

// Even my function names are borderline
boolean F_ShouldColormapFade(void);
boolean F_TryColormapFade(UINT8 wipecolor);
#ifndef NOWIPE
void F_DecideWipeStyle(void);
#endif

#define FADECOLORMAPDIV 8
#define FADECOLORMAPROWS (256/FADECOLORMAPDIV)

#define FADEREDFACTOR   15
#define FADEGREENFACTOR 15
#define FADEBLUEFACTOR  10

extern INT32 lastwipetic;

// Don't know where else to place this constant
// But this file seems appropriate
#define PRELEVELTIME 24 // frames in tics

void F_WipeStartScreen(void);
void F_WipeEndScreen(void);
void F_RunWipe(UINT8 wipetype, boolean drawMenu);
void F_WipeStageTitle(void);
#define F_WipeColorFill(c) V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, c)
tic_t F_GetWipeLength(UINT8 wipetype);
boolean F_WipeExists(UINT8 wipetype);

enum
{
	wipe_credits_intermediate, // makes a good 0 I guess.

	wipe_level_toblack,
	wipe_intermission_toblack,
	wipe_continuing_toblack,
	wipe_titlescreen_toblack,
	wipe_timeattack_toblack,
	wipe_credits_toblack,
	wipe_evaluation_toblack,
	wipe_gameend_toblack,
	wipe_intro_toblack,
	wipe_ending_toblack,
	wipe_cutscene_toblack,

	// custom intermissions
	wipe_specinter_toblack,
	wipe_multinter_toblack,
	wipe_speclevel_towhite,

	wipe_level_final,
	wipe_intermission_final,
	wipe_continuing_final,
	wipe_titlescreen_final,
	wipe_timeattack_final,
	wipe_credits_final,
	wipe_evaluation_final,
	wipe_gameend_final,
	wipe_intro_final,
	wipe_ending_final,
	wipe_cutscene_final,

	// custom intermissions
	wipe_specinter_final,
	wipe_multinter_final,

	NUMWIPEDEFS,
	WIPEFINALSHIFT = (wipe_level_final-wipe_level_toblack)
};
extern UINT8 wipedefs[NUMWIPEDEFS];

#endif
