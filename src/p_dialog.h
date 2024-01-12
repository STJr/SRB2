// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_dialog.h
/// \brief Text prompt system

#ifndef __P_DIALOG__
#define __P_DIALOG__

#include "doomtype.h"

#include "d_player.h"
#include "d_event.h"

//
// CUTSCENE TEXT WRITING
//
typedef struct
{
	const char *basetext;
	char *disptext;
	size_t disptext_size;
	INT32 baseptr;
	INT32 writeptr;
	INT32 textcount;
	INT32 textspeed;
	UINT8 boostspeed;
} textwriter_t;

UINT8 P_CutsceneWriteText(textwriter_t *writer);
void P_ResetTextWriter(textwriter_t *writer, const char *basetext);

//
// PROMPT STATE
//
typedef struct dialog_s
{
	INT32 promptnum;
	INT32 pagenum;
	textprompt_t *prompt;
	textpage_t *page;
	INT32 timetonext;
	textwriter_t writer;
	INT16 postexectag;
	boolean blockcontrols;
	char *pagetext;
	player_t *callplayer;
	INT32 picnum;
	INT32 pictoloop;
	INT32 pictimer;
	INT32 picmode;
	INT32 numpics;
	cutscene_pic_t *pics;
	boolean showchoices;
	INT32 curchoice;
	INT32 numchoices;
	INT32 nochoice;
	INT32 longestchoice;
	boolean selectedchoice;
} dialog_t;

void P_LoadDialogueLumps(UINT16 wadnum);

void P_StartTextPrompt(player_t *player, INT32 promptnum, INT32 pagenum, UINT16 postexectag, boolean blockcontrols, boolean freezerealtime, boolean allplayers);
void P_EndTextPrompt(player_t *player, boolean forceexec, boolean noexec);
void P_EndAllTextPrompts(boolean forceexec, boolean noexec);
void P_RunDialog(player_t *player);
void P_FreeDialog(dialog_t *dialog);
void P_DialogSetText(dialog_t *dialog, char *pagetext, INT32 numchars);

boolean P_SetCurrentDialogChoice(player_t *player, INT32 choice);
boolean P_SelectDialogChoice(player_t *player, INT32 choice);

boolean F_GetPromptHideHudAll(void);
boolean F_GetPromptHideHud(fixed_t y);
void F_TextPromptDrawer(void);

INT32 P_GetTextPromptByName(const char *name);
void P_GetPromptPageByNamedTag(const char *tag, INT32 *promptnum, INT32 *pagenum);
void P_SetMetaPage(textpage_t *page, textpage_t *metapage);
void P_SetPicsMetaPage(textpage_t *page, textpage_t *metapage);
void P_FreeTextPrompt(textprompt_t *prompt);

INT32 P_ParsePromptBackColor(const char *word);

#endif
