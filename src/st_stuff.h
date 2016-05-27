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
/// \file  st_stuff.h
/// \brief Status bar header

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"
#include "d_player.h"
#include "r_defs.h"

//
// STATUS BAR
//

// Called by main loop.
void ST_Ticker(void);

// Called by main loop.
void ST_Drawer(void);

// Called when the console player is spawned on each level.
void ST_Start(void);

// Called by startup code.
void ST_Init(void);

// Called by G_Responder() when pressing F12 while viewing a demo.
void ST_changeDemoView(void);

void ST_UnloadGraphics(void);
void ST_LoadGraphics(void);

// face load graphics, called when skin changes
void ST_LoadFaceGraphics(char *facestr, char *superstr, INT32 playernum);
void ST_ReloadSkinFaceGraphics(void);
#ifdef DELFILE
void ST_UnLoadFaceGraphics(INT32 skinnum);
#endif

void ST_doPaletteStuff(void);

// return if player a is in the same team as player b
boolean ST_SameTeam(player_t *a, player_t *b);

//--------------------
// status bar overlay
//--------------------

extern boolean st_overlay; // sb overlay on or off when fullscreen

extern lumpnum_t st_borderpatchnum;
// patches, also used in intermission
extern patch_t *tallnum[10];

extern patch_t *smallnum[10]; // 0-9, small numbers for TIME and SCORE in ND, probably WILL be used in intermission

extern patch_t *sboscore;
extern patch_t *sbotime;
extern patch_t *sbocolon;
extern patch_t *sboperiod;
extern patch_t *faceprefix[MAXSKINS]; // face status patches
extern patch_t *superprefix[MAXSKINS]; // super face status patches
extern patch_t *livesback;
extern patch_t *ngradeletters[7];

extern patch_t *sboscolon;
extern patch_t *sbosperiod;

/** HUD location information (don't move this comment)
  */
typedef struct
{
	INT32 x, y;
} hudinfo_t;

typedef enum
{
	HUD_LIVESNAME,
	HUD_LIVESPIC,
	HUD_LIVESNUM,
	HUD_LIVESX,

	HUD_RINGS,
	HUD_RINGSSPLIT,
	HUD_RINGSNUM,
	HUD_RINGSNUMSPLIT,

	HUD_SCORE,
	HUD_SCORENUM,

	HUD_TIME,
	HUD_TIMESPLIT,
	HUD_MINUTES,
	HUD_MINUTESSPLIT,
	HUD_TIMECOLON,
	HUD_TIMECOLONSPLIT,
	HUD_SECONDS,
	HUD_SECONDSSPLIT,
	HUD_TIMETICCOLON,
	HUD_TICS,

	HUD_SS_TOTALRINGS,
	HUD_SS_TOTALRINGS_SPLIT,

	HUD_GETRINGS,
	HUD_GETRINGSNUM,
	HUD_TIMELEFT,
	HUD_TIMELEFTNUM,
	HUD_TIMEUP,
	HUD_HUNTPICS,
	HUD_GRAVBOOTSICO,
	HUD_LAP,

	// twoplayer stuff for regualr damage mode
	HUD_LIVESPIC1P,

	HUD_RINGS1P,
	HUD_RINGSNUM1P,

	HUD_RINGS2P,
	HUD_RINGSNUM2P,

	HUD_SCORENUM1P,

	HUD_SCORE2P,
	HUD_SCORENUM2P,

	HUD_TIME2P,
	HUD_MINUTES2P,
	HUD_TIMECOLON2P,
	HUD_SECONDS2P,

	HUD_SS_TOTALRINGS1P,

	HUD_SS_TOTALRINGS2P,

	HUD_HUNTPICS1P,
	HUD_GRAVBOOTSICO1P,

	HUD_HUNTPICS2P,
	HUD_GRAVBOOTSICO2P,

	HUD_ND_RINGENERGY,
	HUD_ND_HEALTHNUM,
	HUD_ND_HEALTHSLASH,
	HUD_ND_HEALTHTOTAL,

	// twoplayer player 1
	HUD_ND_RINGENERGY1P,
	HUD_ND_HEALTHNUM1P,
	HUD_ND_HEALTHSLASH1P,
	HUD_ND_HEALTHTOTAL1P,

	// twoplayer player 2
	HUD_ND_RINGENERGY2P,
	HUD_ND_HEALTHNUM2P,
	HUD_ND_HEALTHSLASH2P,
	HUD_ND_HEALTHTOTAL2P,

	HUD_ND_EMBLEMICON,
	HUD_ND_EMBLEMS,

	// twoplayer positioning
	HUD_ND_EMBLEMICON2P,
	HUD_ND_EMBLEMS2P,

	HUD_ND_SMALLTIME,
	HUD_ND_MINUTES,
	HUD_ND_TIMECOLON,
	HUD_ND_SECONDS,
	HUD_ND_TIMETICPERIOD,
	HUD_ND_TICS,

	// twoplayer positioning
	HUD_ND_SMALLTIME2P,
	HUD_ND_MINUTES2P,
	HUD_ND_TIMECOLON2P,
	HUD_ND_SECONDS2P,
	HUD_ND_TIMETICPERIOD2P,
	HUD_ND_TICS2P,

	HUD_ND_SMALLSCORE,
	HUD_ND_SCORENUM,

	// splitscreen positioning
	HUD_ND_SMALLSCORESPLIT,
	HUD_ND_SCORENUMSPLIT,

	// twoplayer positioning
	HUD_ND_SMALLSCORE2P,
	HUD_ND_SCORENUM2P,

	HUD_ND_LIVESPIC,
	HUD_ND_LIVESNUM,

	// twoplayer player 1
	HUD_ND_LIVESPIC1P,

	// twoplayer player 2
	HUD_ND_LIVESPIC2P,
	HUD_ND_LIVESNUM2P,

	HUD_BOSS,
	HUD_ND_BOSS,
	HUD_ND_BOSS2P,

	NUMHUDITEMS
} hudnum_t;

extern hudinfo_t hudinfo[NUMHUDITEMS];

extern UINT16 objectsdrawn;

#endif
