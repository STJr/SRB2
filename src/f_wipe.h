// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  f_wipe.h
/// \brief Screen wipes

#ifndef __F_WIPE__
#define __F_WIPE__

#include "screen.h"

//
// WIPES
//

#if NUMSCREENS < 5
#define NOWIPE // do not enable wipe image post processing for ARM, SH and MIPS CPUs
#endif

#define DEFAULTWIPE -1

extern boolean wipe_running;
extern boolean wipe_drawmenuontop;

typedef enum
{
	WIPESTYLE_NORMAL,
	WIPESTYLE_COLORMAP
} wipestyle_t;

typedef enum
{
	WIPEFLAGS_FADEIN    = 1,
	WIPEFLAGS_TOWHITE   = 1<<1,
	WIPEFLAGS_CROSSFADE = 1<<2
} wipeflags_t;

typedef void (*wipe_callback_t)(void);

typedef struct
{
	UINT8 type;
	wipestyle_t style;
	wipeflags_t flags;
	boolean drawmenuontop;
	tic_t holdframes;
	wipe_callback_t callback;
} wipe_t;

typedef enum
{
	SPECIALWIPE_NONE,
	SPECIALWIPE_SSTAGE,
	SPECIALWIPE_RETRY,
} specialwipe_t;
extern specialwipe_t ranspecialwipe;

void ScreenWipe_Start(UINT8 type, wipeflags_t flags);
void ScreenWipe_StartParametrized(wipe_t *wipe);
void ScreenWipe_DoFadeOut(INT16 type, wipeflags_t flags, wipe_callback_t callback);
void ScreenWipe_DoFadeIn(INT16 type, wipeflags_t flags, wipe_callback_t callback);
void ScreenWipe_DoFadeOutIn(void);
void ScreenWipe_DoCrossfade(INT16 type);

void ScreenWipe_Run(void);
void ScreenWipe_Display(void);
void ScreenWipe_Stop(void);
void ScreenWipe_StopAll(void);

void ScreenWipe_StartPending(void);
wipe_t *ScreenWipe_GetQueued(void);
wipestyle_t ScreenWipe_GetStyle(wipeflags_t flags);
void ScreenWipe_SetupFadeOut(wipeflags_t flags);

void ScreenWipe_StartScreen(void);
void ScreenWipe_EndScreen(void);

#define ScreenWipe_DoColorFill(c) V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, c)

#define FADECOLORMAPDIV 8
#define FADECOLORMAPROWS (256/FADECOLORMAPDIV)

#define FADEREDFACTOR   15
#define FADEGREENFACTOR 15
#define FADEBLUEFACTOR  10

tic_t ScreenWipe_GetLength(UINT8 type);
boolean ScreenWipe_Exists(UINT8 type);

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
