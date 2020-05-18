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
/// \file  hu_stuff.h
/// \brief Heads up display

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"
#include "w_wad.h"
#include "r_defs.h"

//------------------------------------
//           heads up font
//------------------------------------
#define HU_FONTSTART '\x16' // the first font character
#define HU_FONTEND '~'

#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1)

// Level title font
#define LT_FONTSTART '!' // the first font characters
#define LT_FONTEND 'z' // the last font characters
#define LT_FONTSIZE (LT_FONTEND - LT_FONTSTART + 1)

#define CRED_FONTSTART '!' // the first font character
#define CRED_FONTEND 'Z' // the last font character
#define CRED_FONTSIZE (CRED_FONTEND - CRED_FONTSTART + 1)

// Name tag font
// Used by base and outline font set
#define NT_FONTSTART '!' // the first font character
#define NT_FONTEND 'Z' // the last font character
#define NT_FONTSIZE (NT_FONTEND - NT_FONTSTART + 1)

#define HU_CROSSHAIRS 3 // maximum of 9 - see HU_Init();

extern char *shiftxform; // english translation shift table
extern char english_shiftxform[];

//------------------------------------
//        sorted player lines
//------------------------------------

typedef struct
{
	UINT32 count;
	INT32 num;
	INT32 color;
	INT32 emeralds;
	const char *name;
} playersort_t;

//------------------------------------
//           chat stuff
//------------------------------------
#define HU_MAXMSGLEN 224
#define CHAT_BUFSIZE 64		// that's enough messages, right? We'll delete the older ones when that gets out of hand.
#ifdef NETSPLITSCREEN
#define OLDCHAT (cv_consolechat.value == 1 || dedicated || vid.width < 640)
#else
#define OLDCHAT (cv_consolechat.value == 1 || dedicated || vid.width < 640 || splitscreen)
#endif
#define CHAT_MUTE (cv_mute.value && !(server || IsPlayerAdmin(consoleplayer)))	// this still allows to open the chat but not to type. That's used for scrolling and whatnot.
#define OLD_MUTE (OLDCHAT && cv_mute.value && !(server || IsPlayerAdmin(consoleplayer)))	// this is used to prevent oldchat from opening when muted.

// some functions
void HU_AddChatText(const char *text, boolean playsound);

// set true when entering a chat message
extern boolean chat_on;

extern patch_t *hu_font[HU_FONTSIZE], *tny_font[HU_FONTSIZE];
extern patch_t *tallnum[10];
extern patch_t *nightsnum[10];
extern patch_t *lt_font[LT_FONTSIZE];
extern patch_t *cred_font[CRED_FONTSIZE];
extern patch_t *ntb_font[NT_FONTSIZE];
extern patch_t *nto_font[NT_FONTSIZE];
extern patch_t *ttlnum[10];
extern patch_t *emeraldpics[3][8];
extern patch_t *rflagico;
extern patch_t *bflagico;
extern patch_t *rmatcico;
extern patch_t *bmatcico;
extern patch_t *tagico;
extern patch_t *tallminus;
extern patch_t *tallinfin;
extern patch_t *tokenicon;

// set true whenever the tab rankings are being shown for any reason
extern boolean hu_showscores;

// init heads up data at game startup.
void HU_Init(void);

void HU_LoadGraphics(void);

// reset heads up when consoleplayer respawns.
void HU_Start(void);

boolean HU_Responder(event_t *ev);
void HU_Ticker(void);
void HU_Drawer(void);
char HU_dequeueChatChar(void);
void HU_Erase(void);
void HU_clearChatChars(void);
void HU_drawPing(INT32 x, INT32 y, UINT32 ping, boolean notext, INT32 flags);	// Lat': Ping drawer for scoreboard.
void HU_DrawTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer);
void HU_DrawTeamTabRankings(playersort_t *tab, INT32 whiteplayer);
void HU_DrawDualTabRankings(INT32 x, INT32 y, playersort_t *tab, INT32 scorelines, INT32 whiteplayer);
void HU_DrawEmeralds(INT32 x, INT32 y, INT32 pemeralds);

INT32 HU_CreateTeamScoresTbl(playersort_t *tab, UINT32 dmtotals[]);

// CECHO interface.
void HU_ClearCEcho(void);
void HU_SetCEchoDuration(INT32 seconds);
void HU_SetCEchoFlags(INT32 flags);
void HU_DoCEcho(const char *msg);

// Demo playback info
extern UINT32 hu_demoscore;
extern UINT32 hu_demotime;
extern UINT16 hu_demorings;
#endif
