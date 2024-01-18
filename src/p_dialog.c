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
/// \file  p_dialog.c
/// \brief Text prompt system

#include "doomdef.h"
#include "doomstat.h"
#include "p_dialog.h"
#include "p_dialogscript.h"
#include "p_local.h"
#include "g_game.h"
#include "g_input.h"
#include "s_sound.h"
#include "v_video.h"
#include "r_textures.h"
#include "m_writebuffer.h"
#include "hu_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "fastcmp.h"

#include <errno.h>

static INT16 chevronAnimCounter;

void P_ResetTextWriter(textwriter_t *writer, const char *basetext, size_t basetextlength)
{
	writer->basetext = basetext;
	writer->basetextlength = basetextlength;
	writer->writeptr = writer->baseptr = 0;
	writer->textspeed = 9;
	writer->textcount = TICRATE/2;

	if (writer->disptext && writer->disptextsize)
		memset(writer->disptext, 0, writer->disptextsize);
}

// ==================
//  TEXT PROMPTS
// ==================

static void P_LockPlayerControls(player_t *player);
static void P_UpdatePromptGfx(dialog_t *dialog);
static void P_PlayDialogSound(player_t *player, sfxenum_t sound);

static textprompt_t *P_GetTextPromptByID(INT32 promptnum)
{
	if (promptnum < 0 || promptnum >= MAX_PROMPTS || !textprompts[promptnum])
		return NULL;

	return textprompts[promptnum];
}

INT32 P_GetTextPromptByName(const char *name)
{
	for (INT32 i = 0; i < MAX_PROMPTS; i++)
	{
		if (textprompts[i] && textprompts[i]->name && strcmp(name, textprompts[i]->name) == 0)
			return i;
	}

	return -1;
}

INT32 P_GetPromptPageByName(textprompt_t *prompt, const char *name)
{
	for (INT32 i = 0; i < prompt->numpages; i++)
	{
		const char *pagename = prompt->page[i].pagename;

		if (pagename && strcmp(name, pagename) == 0)
			return i;
	}

	return -1;
}

void P_InitTextPromptPage(textpage_t *page)
{
	page->lines = 4; // default line count to four
	page->textspeed = TICRATE/5; // default text speed to 1/5th of a second
	page->backcolor = 1; // default to gray
	page->hidehud = 1; // hide appropriate HUD elements
}

void P_FreeTextPrompt(textprompt_t *prompt)
{
	if (!prompt)
		return;

	for (INT32 i = 0; i < prompt->numpages; i++)
	{
		textpage_t *page = &prompt->page[i];

		for (INT32 j = 0; j < page->numchoices; j++)
		{
			promptchoice_t *choice = &page->choices[j];
			Z_Free(choice->text);
			Z_Free(choice->nextpromptname);
			Z_Free(choice->nextpagename);
		}

		Z_Free(page->choices);
		Z_Free(page->pics);
		Z_Free(page->pagename);
		Z_Free(page->nextpromptname);
		Z_Free(page->nextpagename);
		Z_Free(page->name);
		Z_Free(page->text);
	}

	Z_Free(prompt->name);
	Z_Free(prompt);
}

dialog_t *P_GetPlayerDialog(player_t *player)
{
	return globaltextprompt ? globaltextprompt : player->textprompt;
}

typedef struct
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh;
	INT32 texth;
	INT32 texty;
	INT32 namey;
	INT32 chevrony;
	INT32 textx;
	INT32 textr;
	INT32 choicesx;
	INT32 choicesy;
	struct {
		INT32 x, y, w, h;
	} choicesbox;
} dialog_geometry_t;

static void GetPageTextGeometry(dialog_t *dialog, dialog_geometry_t *geo)
{
	boolean has_icon = dialog->iconlump != LUMPERROR;

	INT32 pagelines = dialog->page->lines;
	boolean rightside = has_icon && dialog->page->rightside;

	// Vertical calculations
	INT32 boxh = pagelines*2;
	INT32 texth = dialog->speaker[0] ? (pagelines-1)*2 : pagelines*2; // name takes up first line if it exists
	INT32 namey = BASEVIDHEIGHT - ((boxh * 4) + (boxh/2)*4);

	geo->texty = BASEVIDHEIGHT - ((texth * 4) + (texth/2)*4);
	geo->namey = namey;
	geo->chevrony = BASEVIDHEIGHT - (((1*2) * 4) + ((1*2)/2)*4); // force on last line
	geo->pagelines = pagelines;
	geo->rightside = rightside;
	geo->boxh = boxh;
	geo->texth = texth;

	// Horizontal calculations
	// Shift text to the right if we have a character icon on the left side
	// Add 4 margin against icon
	geo->textx = (has_icon && !rightside) ? ((boxh * 4) + (boxh/2)*4) + 4 : 4;
	geo->textr = rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4) + 4) : BASEVIDWIDTH-4;

	// Calculate choices box
	if (dialog->numchoices)
	{
		const int choices_box_spacing = 4;

		if (dialog->longestchoice == -1)
		{
			for (INT32 i = 0; i < dialog->numchoices; i++)
			{
				INT32 width = V_StringWidth(dialog->page->choices[i].text, 0);
				if (width > dialog->longestchoice)
					dialog->longestchoice = width;
			}
		}

		INT32 longestchoice = dialog->longestchoice + 16;
		INT32 choices_height = dialog->numchoices * 10;
		INT32 choices_x, choices_y, choices_h, choices_w = longestchoice + (choices_box_spacing * 2);

		if (dialog->page->choicesleftside)
			choices_x = 16;
		else
			choices_x = (BASEVIDWIDTH - 8) - choices_w;

		choices_h = choices_height + (choices_box_spacing * 2);
		choices_y = namey - 8 - choices_h;

		geo->choicesbox.x = choices_x;
		geo->choicesbox.w = choices_w;
		geo->choicesbox.y = choices_y;
		geo->choicesbox.h = choices_h;

		geo->choicesx = choices_x + choices_box_spacing;
		geo->choicesy = choices_y + choices_box_spacing;
	}
	else
	{
		geo->choicesbox.x = 0;
		geo->choicesbox.y = 0;
		geo->choicesbox.w = 0;
		geo->choicesbox.w = 0;
		geo->choicesx = 0;
		geo->choicesy = 0;
	}
}

static fixed_t F_GetPromptHideHudBound(dialog_t *dialog)
{
	dialog_geometry_t geo;

	INT32 boxh;

	if (!dialog->prompt || !dialog->page || !dialog->page->hidehud)
		return 0;
	else if (dialog->page->hidehud == 2) // hide all
		return BASEVIDHEIGHT;

	GetPageTextGeometry(dialog, &geo);

	// calc boxheight (see V_DrawPromptBack)
	boxh = geo.boxh * vid.dup;
	boxh = (boxh * 4) + (boxh/2)*5; // 4 lines of space plus gaps between and some leeway

	// return a coordinate to check
	// if negative: don't show hud elements below this coordinate (visually)
	// if positive: don't show hud elements above this coordinate (visually)
	return 0 - boxh; // \todo: if prompt at top of screen (someday), make this return positive
}

boolean F_GetPromptHideHudAll(void)
{
	if (!stplyr || !stplyr->promptactive)
		return false;

	dialog_t *dialog = P_GetPlayerDialog(stplyr);
	if (!dialog)
		return false;

	if (!dialog->prompt || !dialog->page || !dialog->page->hidehud)
		return false;
	else if (dialog->page->hidehud == 2) // hide all
		return true;
	else
		return false;
}

boolean F_GetPromptHideHud(fixed_t y)
{
	INT32 ybound;
	boolean fromtop;
	fixed_t ytest;

	if (!stplyr || !stplyr->promptactive)
		return false;

	dialog_t *dialog = P_GetPlayerDialog(stplyr);
	if (!dialog)
		return false;

	ybound = F_GetPromptHideHudBound(dialog);
	fromtop = (ybound >= 0);
	ytest = (fromtop ? ybound : BASEVIDHEIGHT + ybound);

	return (fromtop ? y < ytest : y >= ytest); // true means hide
}

void P_SetMetaPage(textpage_t *page, textpage_t *metapage)
{
	Z_Free(page->name);
	if (metapage->name)
		page->name = Z_StrDup(metapage->name);
	strlcpy(page->iconname, metapage->iconname, sizeof(page->iconname));
	page->rightside = metapage->rightside;
	page->iconflip = metapage->iconflip;
	page->lines = metapage->lines;
	page->backcolor = metapage->backcolor;
	page->align = metapage->align;
	page->verticalalign = metapage->verticalalign;
	page->textspeed = metapage->textspeed;
	page->textsfx = metapage->textsfx;
	page->hidehud = metapage->hidehud;

	// music: don't copy, else each page change may reset the music
}

void P_SetPicsMetaPage(textpage_t *page, textpage_t *metapage)
{
	page->numpics = metapage->numpics;
	page->picmode = metapage->picmode;
	page->pictoloop = metapage->pictoloop;
	page->pictostart = metapage->pictostart;

	page->pics = Z_Realloc(page->pics, sizeof(cutscene_pic_t) * page->numpics, PU_STATIC, NULL);

	memcpy(page->pics, metapage->pics, sizeof(cutscene_pic_t) * page->numpics);
}

void P_SetDialogSpeaker(dialog_t *dialog, const char *speaker)
{
	if (!speaker || !strlen(speaker))
	{
		dialog->speaker[0] = '\0';
		return;
	}

	strlcpy(dialog->speaker, speaker, sizeof(dialog->speaker));
}

void P_SetDialogIcon(dialog_t *dialog, const char *icon)
{
	dialog->iconlump = LUMPERROR;

	if (!icon || !strlen(icon))
	{
		dialog->icon[0] = '\0';
		return;
	}

	strlcpy(dialog->icon, icon, sizeof(dialog->icon));

	lumpnum_t iconlump = W_CheckNumForLongName(icon);
	if (iconlump != LUMPERROR && W_IsValidPatchNum(iconlump))
		dialog->iconlump = iconlump;
}

static void P_DialogGetDispText(char *disptext, const char *text, size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		const char *c = &text[i];
		if (*c == '\0')
		{
			*disptext = *c;
			return;
		}

		const UINT8 *code = (const UINT8*)c;
		if (*code == TP_OP_CONTROL)
		{
			i += P_DialogSkipOpcode(code + 1);
			continue;
		}

		*disptext = *c;

		disptext++;
	}
}

enum
{
	DIALOG_WRITETEXT_DONE,
	DIALOG_WRITETEXT_TALK,
	DIALOG_WRITETEXT_SILENT
};

static UINT8 P_DialogWriteText(dialog_t *dialog, textwriter_t *writer)
{
	const UINT8 *baseptr = (const UINT8*)writer->basetext;
	if (!baseptr || writer->baseptr >= writer->basetextlength)
		return DIALOG_WRITETEXT_DONE;

	UINT8 lastchar = '\0';

	writer->numtowrite = 1;

	const int max_speed = 8;

	if (writer->boostspeed && !writer->paused)
	{
		// When the player is holding jump or spin
		writer->numtowrite = max_speed;
	}
	else
	{
		// Don't draw any characters if the count was 1 or more when we started
		if (--writer->textcount >= 0)
			return DIALOG_WRITETEXT_SILENT;

		writer->paused = false;

		if (writer->textspeed >= 0 && writer->textspeed < max_speed-1)
			writer->numtowrite = max_speed - writer->textspeed;
	}

	while (writer->numtowrite > 0)
	{
		const UINT8 *c = &baseptr[writer->baseptr];
		if (!*c)
			return DIALOG_WRITETEXT_DONE;

		const UINT8 *code = (const UINT8*)c;
		if (*code == TP_OP_CONTROL)
		{
			code = P_DialogRunOpcode(code + 1, dialog, writer);
			writer->baseptr = code - baseptr;
			lastchar = '\0';
			continue;
		}

		lastchar = *c;

		writer->writeptr++;
		writer->baseptr++;

		if (writer->numtowrite <= 0)
			break;

		// Ignore non-printable characters
		// (that is, decrement numtowrite if this character is printable.)
		if (lastchar < 0x80 && writer->textspeed >= 0)
			--writer->numtowrite;
	}

	// Reset textcount for next tic based on speed
	// if it wasn't already set by a delay.
	if (writer->textcount < 0)
	{
		writer->textcount = 0;
		if (writer->textspeed > max_speed-1)
			writer->textcount = writer->textspeed - max_speed-1;
	}

	if (lastchar == '\0' || isspace(lastchar))
		return DIALOG_WRITETEXT_SILENT;
	else
		return DIALOG_WRITETEXT_TALK;
}

static void P_DialogSetDisplayText(dialog_t *dialog)
{
	P_DialogGetDispText(dialog->disptext, dialog->pagetext, dialog->pagetextlength);

	dialog_geometry_t geo;

	GetPageTextGeometry(dialog, &geo);

	V_WordWrapInPlace(geo.textx, geo.textr, 0, dialog->disptext);
}

static char *P_ProcessPageText(dialog_t *dialog, UINT8 *pagetext, size_t textlength, size_t *outlength)
{
	writebuffer_t buf;

	M_BufferInit(&buf);

	UINT8 *text = pagetext;
	UINT8 *end = pagetext + textlength;

	while (text < end)
	{
		if (!P_DialogPreprocessOpcode(dialog, &text, &buf))
		{
			M_BufferWrite(&buf, *text);
			text++;
		}
	}

	*outlength = buf.pos;

	return (char*)buf.data;
}

void P_SetDialogText(dialog_t *dialog, char *pagetext, size_t textlength)
{
	Z_Free(dialog->pagetext);

	if (pagetext && pagetext[0])
		dialog->pagetext = P_ProcessPageText(dialog, (UINT8 *)pagetext, textlength, &dialog->pagetextlength);
	else
	{
		dialog->pagetextlength = 0;
		dialog->pagetext = Z_StrDup("");
	}

	Z_Free(dialog->disptext);
	dialog->disptextsize = textlength + 1;
	dialog->disptext = Z_Calloc(dialog->disptextsize, PU_STATIC, NULL);

	textwriter_t *writer = &dialog->writer;

	P_ResetTextWriter(writer, dialog->pagetext, dialog->pagetextlength);

	INT32 textspeed = dialog->page->textspeed;
	if (textspeed >= -1)
		writer->textspeed = textspeed;
	else
		writer->textspeed = TICRATE/5;

	writer->textcount = 0; // no delay in beginning
	writer->boostspeed = false; // don't print 8 characters to start

	P_DialogSetDisplayText(dialog);

	// Immediately type the first character
	P_DialogWriteText(dialog, writer);
}

static void P_DialogStartPage(player_t *player, dialog_t *dialog)
{
	dialog->paused = false;
	dialog->gotonext = false;

	dialog->jumpdown = 0;
	dialog->spindown = 0;

	// on page mode, number of tics before allowing boost
	// on timer mode, number of tics until page advances
	dialog->timetonext = dialog->page->timetonext ? dialog->page->timetonext : TICRATE/10;

	dialog->longestchoice = -1;

	if (dialog->page->numchoices > 0)
	{
		INT32 startchoice = dialog->page->startchoice ? dialog->page->startchoice - 1 : 0;
		INT32 nochoice = dialog->page->nochoice ? dialog->page->nochoice - 1 : 0;

		dialog->numchoices = dialog->page->numchoices;

		if (startchoice >= 0 && startchoice < dialog->numchoices)
			dialog->curchoice = startchoice;
		if (nochoice >= 0 && nochoice < dialog->numchoices)
			dialog->nochoice = nochoice;
	}
	else
	{
		dialog->numchoices = 0;
		dialog->curchoice = 0;
		dialog->nochoice = -1;
	}

	dialog->showchoices = false;
	dialog->selectedchoice = false;

	// gfx
	dialog->numpics = dialog->page->numpics;
	dialog->pics = dialog->page->pics;

	if (dialog->pics)
	{
		dialog->picnum = dialog->page->pictostart;
		dialog->pictoloop = dialog->page->pictoloop > 0 ? dialog->page->pictoloop - 1 : 0;
		dialog->pictimer = dialog->pics[dialog->picnum].duration;
		dialog->picmode = dialog->page->picmode;
	}
	else
	{
		dialog->picnum = 0;
		dialog->pictoloop = 0;
		dialog->pictimer = 0;
		dialog->picmode = 0;
	}

	// Set up narrator
	P_SetDialogSpeaker(dialog, dialog->page->name);
	P_SetDialogIcon(dialog, dialog->page->iconname);

	dialog->iconflip = dialog->page->iconflip;

	// Prepare page text
	P_SetDialogText(dialog, dialog->page->text, dialog->page->textlength);

	// music change
	if (P_IsLocalPlayer(player) || globaltextprompt)
	{
		if (dialog->page->musswitch[0])
		{
			S_ChangeMusic(dialog->page->musswitch,
				dialog->page->musswitchflags,
				dialog->page->musicloop);
		}
		else if (dialog->page->restoremusic)
		{
			S_ChangeMusic(mapheaderinfo[gamemap-1]->musname,
				mapheaderinfo[gamemap-1]->mustrack,
				true);
		}
	}

	chevronAnimCounter = 0;
}

static boolean P_LoadNextPageAndPrompt(player_t *player, dialog_t *dialog, INT32 nextprompt, INT32 nextpage)
{
	textprompt_t *oldprompt = dialog->prompt;

	// determine next prompt
	if (nextprompt != INT32_MAX)
	{
		if (nextprompt >= 0 && nextprompt < MAX_PROMPTS && textprompts[nextprompt])
		{
			dialog->promptnum = nextprompt;
			dialog->prompt = textprompts[nextprompt];
		}
		else
		{
			dialog->promptnum = INT32_MAX;
			dialog->prompt = NULL;
		}
	}

	// determine next page
	if (nextpage != INT32_MAX)
	{
		if (dialog->prompt != NULL)
		{
			if (nextpage >= MAX_PAGES || nextpage > dialog->prompt->numpages-1)
			{
				dialog->pagenum = INT32_MAX;
				dialog->page = NULL;
			}
			else
			{
				dialog->pagenum = nextpage;
				dialog->page = &dialog->prompt->page[nextpage];
			}
		}
	}
	else if (dialog->prompt != NULL)
	{
		if (dialog->prompt != oldprompt)
		{
			dialog->pagenum = 0;
			dialog->page = &dialog->prompt->page[0];
		}
		else if (dialog->pagenum + 1 < MAX_PAGES && dialog->pagenum < dialog->prompt->numpages-1)
		{
			dialog->pagenum++;
			dialog->page = &dialog->prompt->page[dialog->pagenum];
		}
		else
		{
			dialog->pagenum = INT32_MAX;
			dialog->page = NULL;
		}
	}

	// close the prompt if either num is invalid
	if (dialog->prompt == NULL || dialog->page == NULL)
	{
		P_EndTextPrompt(player, false, false);
		return false;
	}

	P_DialogStartPage(player, dialog);

	return true;
}

static void P_GetNextPromptAndPage(textprompt_t *dialog, char *nextpromptname, char *nextpagename, char *nexttag, INT32 nextprompt, INT32 nextpage, INT32 *foundprompt, INT32 *foundpage)
{
	INT32 prompt = INT32_MAX, page = INT32_MAX;

	if (nextpromptname && nextpromptname[0])
		prompt = P_GetTextPromptByName(nextpromptname);
	else if (nextprompt)
		prompt = nextprompt - 1;

	textprompt_t *nextdialog = P_GetTextPromptByID(prompt);
	if (!nextdialog)
		nextdialog = dialog;

	if (nextpagename && nextpagename[0])
	{
		if (nextdialog)
			page = P_GetPromptPageByName(nextdialog, nextpagename);
	}
	else if (nextpage)
		page = nextpage - 1;

	if (nexttag && nexttag[0])
		P_GetPromptPageByNamedTag(nexttag, &prompt, &page);

	*foundprompt = prompt;
	*foundpage = page;
}

static void P_PageGetNextPromptAndPage(textprompt_t *prompt, textpage_t *page, INT32 *nextprompt, INT32 *nextpage)
{
	P_GetNextPromptAndPage(prompt,
		page->nextpromptname, page->nextpagename, page->nexttag,
		page->nextprompt, page->nextpage,
		nextprompt, nextpage);
}

static void P_ChoiceGetNextPromptAndPage(textprompt_t *prompt, promptchoice_t *choice, INT32 *nextprompt, INT32 *nextpage)
{
	P_GetNextPromptAndPage(prompt,
		choice->nextpromptname, choice->nextpagename, choice->nexttag,
		choice->nextprompt, choice->nextpage,
		nextprompt, nextpage);
}

static boolean P_AdvanceToNextPage(player_t *player, dialog_t *dialog)
{
	textprompt_t *curprompt = dialog->prompt;
	textpage_t *curpage = dialog->page;

	if (curpage->exectag)
		P_LinedefExecute(curpage->exectag, player->mo, NULL);

	if (!player->promptactive || dialog->prompt != curprompt || dialog->page != curpage)
		return false;

	if (curpage->endprompt)
	{
		P_EndTextPrompt(player, false, false);
		return false;
	}

	INT32 nextprompt = INT32_MAX, nextpage = INT32_MAX;

	P_PageGetNextPromptAndPage(dialog->prompt, curpage, &nextprompt, &nextpage);

	return P_LoadNextPageAndPrompt(player, dialog, nextprompt, nextpage);
}

static boolean P_ExecuteChoice(player_t *player, dialog_t *dialog, promptchoice_t *choice)
{
	textprompt_t *curprompt = dialog->prompt;
	textpage_t *curpage = dialog->page;

	if (choice->exectag)
		P_LinedefExecute(choice->exectag, player->mo, NULL);

	if (!player->promptactive || dialog->prompt != curprompt || dialog->page != curpage)
		return false;

	if (choice->endprompt)
	{
		P_EndTextPrompt(player, false, false);
		return false;
	}

	INT32 nextprompt = INT32_MAX, nextpage = INT32_MAX;

	P_ChoiceGetNextPromptAndPage(curprompt, choice, &nextprompt, &nextpage);

	return P_LoadNextPageAndPrompt(player, dialog, nextprompt, nextpage);
}

void P_FreeDialog(dialog_t *dialog)
{
	if (dialog)
	{
		Z_Free(dialog->writer.disptext);
		Z_Free(dialog->pagetext);
		Z_Free(dialog->disptext);
		Z_Free(dialog);
	}
}

static void P_FreePlayerDialog(player_t *player)
{
	if (player->textprompt == globaltextprompt)
		return;

	P_FreeDialog(player->textprompt);

	player->textprompt = NULL;
}

static INT16 P_DoEndDialog(player_t *player, dialog_t *dialog, boolean forceexec, boolean noexec)
{
	boolean promptwasactive = player->promptactive;

	INT16 postexectag = 0;

	player->promptactive = false;
	player->textprompt = NULL;

	if (dialog)
	{
		postexectag = dialog->postexectag;

		if (promptwasactive)
		{
			if (dialog->blockcontrols)
			{
				player->mo->reactiontime = TICRATE/4; // prevent jumping right away
				if (dialog->jumpdown)
					player->pflags |= PF_JUMPDOWN;
				if (dialog->spindown)
					player->pflags |= PF_SPINDOWN;
			}
		}
	}

	if ((promptwasactive || forceexec) && !noexec)
		return postexectag;

	return 0;
}

static void P_EndGlobalTextPrompt(boolean forceexec, boolean noexec)
{
	if (!globaltextprompt)
		return;

	INT16 postexectag = 0;

	player_t *player = globaltextprompt->player;
	if (player)
	{
		if ((player->promptactive || forceexec) && !noexec && globaltextprompt->postexectag)
			postexectag = globaltextprompt->postexectag;
	}

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			P_DoEndDialog(&players[i], globaltextprompt, false, true);
	}

	P_FreeDialog(globaltextprompt);

	globaltextprompt = NULL;

	if (postexectag)
		P_LinedefExecute(postexectag, player->mo, NULL);
}

void P_EndTextPrompt(player_t *player, boolean forceexec, boolean noexec)
{
	if (globaltextprompt && player->textprompt == globaltextprompt)
	{
		P_EndGlobalTextPrompt(forceexec, noexec);
		return;
	}

	if (!player->textprompt)
		return;

	INT16 postexectag = P_DoEndDialog(player, player->textprompt, forceexec, noexec);

	if (player->textprompt)
		P_FreePlayerDialog(player);

	if (postexectag)
		P_LinedefExecute(postexectag, player->mo, NULL);
}

void P_EndAllTextPrompts(boolean forceexec, boolean noexec)
{
	if (globaltextprompt)
		P_EndGlobalTextPrompt(forceexec, noexec);
	else
	{
		for (INT32 i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				P_EndTextPrompt(&players[i], forceexec, noexec);
		}
	}
}

void P_StartTextPrompt(player_t *player, INT32 promptnum, INT32 pagenum, UINT16 postexectag, boolean blockcontrols, boolean freezerealtime, boolean allplayers)
{
	INT32 i;

	dialog_t *dialog = NULL;

	if (allplayers)
	{
		P_EndAllTextPrompts(false, true);

		globaltextprompt = Z_Calloc(sizeof(dialog_t), PU_LEVEL, NULL);

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				players[i].textprompt = globaltextprompt;
		}

		dialog = globaltextprompt;
	}
	else
	{
		if (player->textprompt)
			dialog = player->textprompt;
		else
		{
			dialog = Z_Calloc(sizeof(dialog_t), PU_LEVEL, NULL);
			player->textprompt = dialog;
		}
	}

	dialog->timetonext = 0;
	dialog->pictimer = 0;

	// Set up state
	dialog->postexectag = postexectag;
	dialog->blockcontrols = blockcontrols;
	(void)freezerealtime; // \todo freeze player->realtime, maybe this needs to cycle through player thinkers

	// Initialize current prompt and scene
	dialog->player = player;
	dialog->promptnum = (promptnum < MAX_PROMPTS && textprompts[promptnum]) ? promptnum : INT32_MAX;
	dialog->pagenum = (dialog->promptnum != INT32_MAX && pagenum < MAX_PAGES && pagenum <= textprompts[dialog->promptnum]->numpages-1) ? pagenum : INT32_MAX;
	dialog->prompt = NULL;
	dialog->page = NULL;

	boolean promptactive = dialog->promptnum != INT32_MAX && dialog->pagenum != INT32_MAX;

	if (promptactive)
	{
		if (allplayers)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
					players[i].promptactive = true;
			}
		}
		else
			player->promptactive = true;

		dialog->prompt = textprompts[dialog->promptnum];
		dialog->page = &dialog->prompt->page[dialog->pagenum];

		P_DialogStartPage(player, dialog);
	}
	else
	{
		// run the post-effects immediately
		if (allplayers)
			P_EndGlobalTextPrompt(true, false);
		else
			P_EndTextPrompt(player, true, false);
	}
}

static boolean P_GetTextPromptTutorialTag(char *tag, INT32 length)
{
	INT32 gcs = gcs_custom;
	boolean suffixed = true;

	if (!tag || !tag[0] || !tutorialmode)
		return false;

	if (!strncmp(tag, "TAM", 3)) // Movement
		gcs = G_GetControlScheme(gamecontrol, gcl_movement, num_gcl_movement);
	else if (!strncmp(tag, "TAC", 3)) // Camera
	{
		// Check for gcl_movement so we can differentiate between FPS and Platform schemes.
		gcs = G_GetControlScheme(gamecontrol, gcl_movement, num_gcl_movement);
		if (gcs == gcs_custom) // try again, maybe we'll get a match
			gcs = G_GetControlScheme(gamecontrol, gcl_camera, num_gcl_camera);
		if (gcs == gcs_fps && !cv_usemouse.value)
			gcs = gcs_platform; // Platform (arrow) scheme is stand-in for no mouse
	}
	else if (!strncmp(tag, "TAD", 3)) // Movement and Camera
		gcs = G_GetControlScheme(gamecontrol, gcl_movement_camera, num_gcl_movement_camera);
	else if (!strncmp(tag, "TAJ", 3)) // Jump
		gcs = G_GetControlScheme(gamecontrol, gcl_jump, num_gcl_jump);
	else if (!strncmp(tag, "TAS", 3)) // Spin
		gcs = G_GetControlScheme(gamecontrol, gcl_spin, num_gcl_spin);
	else if (!strncmp(tag, "TAA", 3)) // Char ability
		gcs = G_GetControlScheme(gamecontrol, gcl_jump, num_gcl_jump);
	else if (!strncmp(tag, "TAW", 3)) // Shield ability
		gcs = G_GetControlScheme(gamecontrol, gcl_jump_spin, num_gcl_jump_spin);
	else
		gcs = G_GetControlScheme(gamecontrol, gcl_tutorial_used, num_gcl_tutorial_used);

	switch (gcs)
	{
		case gcs_fps:
			// strncat(tag, "FPS", length);
			suffixed = false;
			break;

		case gcs_platform:
			strncat(tag, "PLATFORM", length);
			break;

		default:
			strncat(tag, "CUSTOM", length);
			break;
	}

	return suffixed;
}

void P_GetPromptPageByNamedTag(const char *tag, INT32 *promptnum, INT32 *pagenum)
{
	INT32 nosuffixpromptnum = INT32_MAX, nosuffixpagenum = INT32_MAX;
	INT32 tutorialpromptnum = (tutorialmode) ? TUTORIAL_PROMPT-1 : 0;
	boolean suffixed = false, found = false;
	char suffixedtag[33];

	*promptnum = *pagenum = INT32_MAX;

	if (!tag || !tag[0])
		return;

	strncpy(suffixedtag, tag, 33);
	suffixedtag[32] = 0;

	if (tutorialmode)
		suffixed = P_GetTextPromptTutorialTag(suffixedtag, 33);

	for (*promptnum = 0 + tutorialpromptnum; *promptnum < MAX_PROMPTS; (*promptnum)++)
	{
		if (!textprompts[*promptnum])
			continue;

		for (*pagenum = 0; *pagenum < textprompts[*promptnum]->numpages && *pagenum < MAX_PAGES; (*pagenum)++)
		{
			if (suffixed && fastcmp(suffixedtag, textprompts[*promptnum]->page[*pagenum].tag))
			{
				// this goes first because fastcmp ends early if first string is shorter
				found = true;
				break;
			}
			else if (nosuffixpromptnum == INT32_MAX && nosuffixpagenum == INT32_MAX && fastcmp(tag, textprompts[*promptnum]->page[*pagenum].tag))
			{
				if (suffixed)
				{
					nosuffixpromptnum = *promptnum;
					nosuffixpagenum = *pagenum;
					// continue searching for the suffixed tag
				}
				else
				{
					found = true;
					break;
				}
			}
		}

		if (found)
			break;
	}

	if (suffixed && !found && nosuffixpromptnum != INT32_MAX && nosuffixpagenum != INT32_MAX)
	{
		found = true;
		*promptnum = nosuffixpromptnum;
		*pagenum = nosuffixpagenum;
	}

	if (!found)
		CONS_Debug(DBG_GAMELOGIC, "Text prompt: Can't find a page with named tag %s or suffixed tag %s\n", tag, suffixedtag);
}

void P_RunDialog(player_t *player)
{
	if (!player || !player->promptactive)
		return;

	dialog_t *dialog = P_GetPlayerDialog(player);
	if (!dialog)
	{
		player->promptactive = false;
		return;
	}

	player_t *promptplayer = dialog->player;
	if (!promptplayer)
		return;

	// for the chevron
	if (P_IsLocalPlayer(player))
	{
		if (--chevronAnimCounter <= 0)
			chevronAnimCounter = 8;
	}

	// Don't update if the player is a spectator, or quit the game
	if (player->spectator || player->quittime)
		return;

	// Block player controls
	if (dialog->blockcontrols)
		P_LockPlayerControls(player);

	// Stop here if this player isn't who started this dialog
	if (player != promptplayer)
		return;

	textwriter_t *writer = &dialog->writer;

	writer->boostspeed = false;

	// button handling
	if (dialog->page->timetonext)
	{
		// same procedure as below, just without the button handling
		if (dialog->timetonext >= 1)
			dialog->timetonext--;

		if (!dialog->timetonext)
		{
			P_AdvanceToNextPage(player, dialog);
			return;
		}
		else if (!dialog->paused)
		{
			INT32 write = P_DialogWriteText(dialog, writer);
			if (write == DIALOG_WRITETEXT_TALK)
			{
				if (dialog->page->textsfx)
					P_PlayDialogSound(player, dialog->page->textsfx);
			}
		}
	}
	else
	{
		if (dialog->blockcontrols)
		{
			boolean gotonext = dialog->gotonext;

			// Handle choices
			if (dialog->showchoices)
			{
				if (dialog->selectedchoice)
				{
					dialog->selectedchoice = false;

					P_PlayDialogSound(player, sfx_menu1);

					P_ExecuteChoice(player, dialog, &dialog->page->choices[dialog->curchoice]);
					return;
				}
			}
			else
			{
				if (player->cmd.buttons & BT_JUMP)
				{
					if (dialog->jumpdown < TICRATE)
						dialog->jumpdown++;
				}
				else
					dialog->jumpdown = 0;

				if (player->cmd.buttons & BT_SPIN)
				{
					if (dialog->spindown < TICRATE)
						dialog->spindown++;
				}
				else
					dialog->spindown = 0;
			}

			if (dialog->jumpdown || dialog->spindown)
			{
				if (dialog->timetonext > 1)
					dialog->timetonext--;
				else if (writer->baseptr) // don't set boost if we just reset the string
					writer->boostspeed = true; // only after a slight delay

				if (dialog->paused)
				{
					if (dialog->jumpdown == 1 || dialog->spindown == 1)
						dialog->paused = false;
				}
				else if (!dialog->timetonext && !dialog->showchoices) // timetonext is 0 when finished generating text
					gotonext = true;
			}

			if (gotonext)
			{
				if (P_AdvanceToNextPage(player, dialog))
					P_PlayDialogSound(player, sfx_menu1);
				return;
			}
		}

		// never show the chevron if we can't toggle pages
		if (!dialog->page->text || !dialog->page->text[0])
			dialog->timetonext = !dialog->blockcontrols;

		// generate letter-by-letter text
		if (!dialog->paused)
		{
			INT32 write = P_DialogWriteText(dialog, writer);
			if (write != DIALOG_WRITETEXT_DONE)
			{
				if (write == DIALOG_WRITETEXT_TALK && dialog->page->textsfx)
					P_PlayDialogSound(player, dialog->page->textsfx);
			}
			else
			{
				if (dialog->blockcontrols && !dialog->showchoices && dialog->numchoices)
					dialog->showchoices = true;
				dialog->timetonext = !dialog->blockcontrols;
			}
		}
	}

	P_UpdatePromptGfx(dialog);
}

static void P_LockPlayerControls(player_t *player)
{
	player->powers[pw_nocontrol] = 1;

	if (player->mo && !P_MobjWasRemoved(player->mo))
	{
		if (player->mo->state == &states[S_PLAY_STND] && player->mo->tics != -1)
			player->mo->tics++;
		else if (player->mo->state == &states[S_PLAY_WAIT])
			P_SetMobjState(player->mo, S_PLAY_STND);
	}
}

static void P_UpdatePromptGfx(dialog_t *dialog)
{
	if (!dialog->pics || dialog->picnum < 0 || dialog->picnum >= dialog->numpics)
		return;

	if (dialog->pictimer <= 0)
	{
		boolean persistanimtimer = false;

		if (dialog->picnum < dialog->numpics-1 && dialog->pics[dialog->picnum+1].name[0] != '\0')
			dialog->picnum++;
		else if (dialog->picmode == PROMPT_PIC_LOOP)
			dialog->picnum = dialog->pictoloop;
		else if (dialog->picmode == PROMPT_PIC_DESTROY)
			dialog->picnum = -1;
		else // if (dialog->picmode == PROMPT_PIC_PERSIST)
			persistanimtimer = true;

		if (!persistanimtimer && dialog->picnum >= 0)
			dialog->pictimer = dialog->pics[dialog->picnum].duration;
	}
	else
		dialog->pictimer--;
}

static void P_PlayDialogSound(player_t *player, sfxenum_t sound)
{
	if (P_IsLocalPlayer(player) || &players[displayplayer] == player)
		S_StartSound(NULL, sound);
}

boolean P_SetCurrentDialogChoice(player_t *player, INT32 choice)
{
	if (!player->promptactive)
		return false;

	dialog_t *dialog = P_GetPlayerDialog(player);

	if (!dialog || choice < 0 || choice >= dialog->numchoices)
		return false;

	dialog->curchoice = choice;

	P_PlayDialogSound(player, sfx_menu1);

	return true;
}

boolean P_SelectDialogChoice(player_t *player, INT32 choice)
{
	if (!player->promptactive)
		return false;

	dialog_t *dialog = P_GetPlayerDialog(player);

	if (!dialog || choice < 0 || choice >= dialog->numchoices)
		return false;

	dialog->curchoice = choice;
	dialog->selectedchoice = true;

	return true;
}

static void F_DrawDialogText(INT32 x, INT32 y, INT32 option, const char *string, size_t numchars, boolean do_line_breaks, const UINT8 *highlight)
{
	INT32 w, c, cx = x, cy = y, dup, scrwidth, left = 0;
	const char *ch = string;
	INT32 charflags = option & V_CHARCOLORMASK;
	const UINT8 *colormap = NULL;
	const INT32 spacewidth = 4;

	INT32 lowercase = (option & V_ALLOWLOWERCASE);
	option &= ~V_FLIP; // which is also shared with V_ALLOWLOWERCASE...

	if (option & V_NOSCALESTART)
	{
		dup = vid.dup;
		scrwidth = vid.width;
	}
	else
	{
		dup = 1;
		scrwidth = vid.width/vid.dup;
		left = (scrwidth - BASEVIDWIDTH)/2;
		scrwidth -= left;
	}

	if (option & V_NOSCALEPATCH)
		scrwidth *= vid.dup;

	for (size_t i = 0; i < numchars; ch++, i++)
	{
		if (!*ch)
			break;
		if (*ch & 0x80) //color parsing -x 2.16.09
		{
			// manually set flags override color codes
			charflags = ((*ch & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK;
			continue;
		}
		if (*ch == '\n')
		{
			if (do_line_breaks)
			{
				cx = x;
				cy += 12*dup;
			}
			continue;
		}

		c = *ch;
		if (!lowercase)
			c = toupper(c);
		c -= HU_FONTSTART;

		// character does not exist or is a space
		if (c < 0 || c >= HU_FONTSIZE || !hu_font[c])
		{
			cx += spacewidth * dup;
			continue;
		}

		w = hu_font[c]->width * dup;

		if (cx > scrwidth)
			continue;
		if (cx+left + w < 0) //left boundary check
		{
			cx += w;
			continue;
		}

		colormap = V_GetStringColormap(charflags);
		if (!colormap)
			colormap = highlight;
		V_DrawFixedPatch(cx<<FRACBITS, cy<<FRACBITS, FRACUNIT, option, hu_font[c], colormap);

		cx += w;
	}
}

static void F_DrawDialogIcon(dialog_t *dialog, dialog_geometry_t *geo, INT32 flags)
{
	INT32 iconx, icony, scale, scaledsize;
	INT32 width, height;

	INT32 boxh = geo->boxh;
	INT32 namey = geo->namey;
	boolean rightside = geo->rightside;

	patch_t *patch = NULL;

	if (dialog->iconlump != LUMPERROR)
	{
		patch = W_CachePatchLongName(dialog->icon, PU_PATCH_LOWPRIORITY);
		if (!patch)
			return;

		width = patch->width;
		height = patch->height;
	}
	else
		return;

	// scale and center
	if (width > height)
	{
		scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, width);
		scaledsize = FixedMul(height, scale);
		iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
		icony = ((namey-4) << FRACBITS) + FixedDiv(BASEVIDHEIGHT - namey + 4 - scaledsize, 2); // account for 4 margin
	}
	else if (height > width)
	{
		scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, height);
		scaledsize = FixedMul(width, scale);
		iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
		icony = namey << FRACBITS;
		iconx += FixedDiv(FixedMul(height, scale) - scaledsize, 2);
	}
	else
	{
		scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, width);
		iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
		icony = namey << FRACBITS;
	}

	if (dialog->iconflip)
	{
		iconx += FixedMul(width, scale) << FRACBITS;
		flags |= V_FLIP;
	}

	V_DrawFixedPatch(iconx, icony, scale, flags, patch, NULL);
}

void F_TextPromptDrawer(void)
{
	if (!stplyr || !stplyr->promptactive)
		return;

	dialog_t *dialog = P_GetPlayerDialog(stplyr);
	if (!dialog)
		return;

	dialog_geometry_t geo;

	INT32 draw_flags = V_PERPLAYER;
	INT32 snap_flags = V_SNAPTOBOTTOM;

	// Data
	patch_t *patch;

	GetPageTextGeometry(dialog, &geo);

	INT32 boxh = geo.boxh, texty = geo.texty, namey = geo.namey, chevrony = geo.chevrony;
	INT32 textx = geo.textx, textr = geo.textr;
	INT32 choicesx = geo.choicesx, choicesy = geo.choicesy;

	// Draw gfx first
	if (dialog->pics && dialog->picnum >= 0 && dialog->picnum < dialog->numpics && dialog->pics[dialog->picnum].name[0] != '\0')
	{
		cutscene_pic_t *pic = &dialog->pics[dialog->picnum];

		patch = W_CachePatchLongName(pic->name, PU_PATCH_LOWPRIORITY);

		if (pic->hires)
			V_DrawSmallScaledPatch(pic->xcoord, pic->ycoord, draw_flags, patch);
		else
			V_DrawScaledPatch(pic->xcoord, pic->ycoord, draw_flags, patch);
	}

	// Draw background
	V_DrawPromptBack(boxh, dialog->page->backcolor, draw_flags|snap_flags);

	// Draw narrator icon
	F_DrawDialogIcon(dialog, &geo, draw_flags | snap_flags);

	// Draw text
	if (dialog->disptext)
		F_DrawDialogText(textx, texty, (draw_flags|snap_flags|V_ALLOWLOWERCASE), dialog->disptext, dialog->writer.writeptr, true, NULL);

	// Draw name
	if (dialog->speaker[0])
		F_DrawDialogText(textx, namey, (draw_flags|snap_flags|V_ALLOWLOWERCASE|V_YELLOWMAP), dialog->speaker, strlen(dialog->speaker), false, NULL);

	// Draw choices
	if (dialog->showchoices)
	{
		INT32 choices_flags = draw_flags | snap_flags;

		if (dialog->page->choicesleftside)
			choices_flags |= V_SNAPTOLEFT;
		else
			choices_flags |= V_SNAPTORIGHT;

		V_DrawPromptRect(geo.choicesbox.x, geo.choicesbox.y, geo.choicesbox.w, geo.choicesbox.h, dialog->page->backcolor, choices_flags);

		textx = choicesx + 12;

		for (INT32 i = 0; i < dialog->numchoices; i++)
		{
			const char *text = dialog->page->choices[i].text;

			INT32 flags = choices_flags | V_ALLOWLOWERCASE;
			INT32 highlight_flags = 0;

			boolean selected = dialog->curchoice == i;

			if (selected)
				highlight_flags |= V_YELLOWMAP;

			F_DrawDialogText(textx, choicesy, flags, text, strlen(text), false, V_GetStringColormap(highlight_flags & V_CHARCOLORMASK));

			if (selected)
				V_DrawString(choicesx + (chevronAnimCounter/5), choicesy, choices_flags|V_YELLOWMAP, "\x1D");

			choicesy += 10;
		}
	}

	if (globaltextprompt && (globaltextprompt->player != &players[displayplayer]))
		return;

	// Draw chevron
	if (dialog->blockcontrols && (!dialog->timetonext || dialog->paused) && !dialog->showchoices)
		V_DrawString(textr-8, chevrony + (chevronAnimCounter/5), (draw_flags|snap_flags|V_YELLOWMAP), "\x1B"); // down arrow
}
