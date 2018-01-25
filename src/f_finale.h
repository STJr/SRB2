// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
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

// Called by main loop.
FUNCMATH void F_GameEndDrawer(void);
void F_IntroDrawer(void);
void F_TitleScreenDrawer(void);

void F_GameEvaluationDrawer(void);
void F_StartGameEvaluation(void);
void F_GameEvaluationTicker(void);

void F_CreditTicker(void);
void F_CreditDrawer(void);

void F_StartCustomCutscene(INT32 cutscenenum, boolean precutscene, boolean resetplayer);
void F_CutsceneDrawer(void);
void F_EndCutScene(void);

void F_StartGameEnd(void);
void F_StartIntro(void);
void F_StartTitleScreen(void);
void F_StartCredits(void);

boolean F_ContinueResponder(event_t *event);
void F_StartContinue(void);
void F_ContinueTicker(void);
void F_ContinueDrawer(void);

extern INT32 titlescrollspeed;

typedef enum
{
	TITLEMAP_OFF = 0,
	TITLEMAP_LOADING,
	TITLEMAP_RUNNING
} titlemap_enum;

extern UINT8 titlemapinaction;

//
// WIPE
//
extern boolean WipeInAction;
extern INT32 lastwipetic;

void F_WipeStartScreen(void);
void F_WipeEndScreen(void);
void F_RunWipe(UINT8 wipetype, boolean drawMenu);

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
	wipe_cutscene_final,

	// custom intermissions
	wipe_specinter_final,
	wipe_multinter_final,

	NUMWIPEDEFS
};
#define WIPEFINALSHIFT 13
extern UINT8 wipedefs[NUMWIPEDEFS];

#endif
