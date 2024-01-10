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
#include "p_local.h"
#include "g_game.h"
#include "g_input.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "fastcmp.h"

static INT16 skullAnimCounter; // Prompts: Chevron animation

static boolean IsSpeedControlChar(UINT8 chr)
{
	return chr >= 0xA0 && chr <= 0xAF;
}

static boolean IsDelayControlChar(UINT8 chr)
{
	return chr >= 0xB0 && chr <= (0xB0+TICRATE-1);
}

static void WriterTextBufferAlloc(textwriter_t *writer)
{
	if (!writer->disptext_size)
		writer->disptext_size = 16;

	size_t oldsize = writer->disptext_size;

	while (((unsigned)writer->writeptr) + 1 >= writer->disptext_size)
		writer->disptext_size *= 2;

	if (!writer->disptext)
		writer->disptext = Z_Calloc(writer->disptext_size, PU_STATIC, NULL);
	else if (oldsize != writer->disptext_size)
	{
		writer->disptext = Z_Realloc(writer->disptext, writer->disptext_size, PU_STATIC, NULL);
		memset(&writer->disptext[writer->writeptr], 0x00, writer->disptext_size - writer->writeptr);
	}
}

//
// This alters the text string writer->disptext.
// Use the typical string drawing functions to display it.
// Returns 0 if \0 is reached (end of input)
//
UINT8 P_CutsceneWriteText(textwriter_t *writer)
{
	INT32 numtowrite = 1;
	const char *c;

	if (writer->boostspeed)
	{
		// for custom cutscene speedup mode
		numtowrite = 8;
	}
	else
	{
		// Don't draw any characters if the count was 1 or more when we started
		if (--writer->textcount >= 0)
			return 1;

		if (writer->textspeed < 7)
			numtowrite = 8 - writer->textspeed;
	}

	for (;numtowrite > 0;++writer->baseptr)
	{
		c = &writer->basetext[writer->baseptr];
		if (!c || !*c || *c=='#')
			return 0;

		// \xA0 - \xAF = change text speed
		if (IsSpeedControlChar((UINT8)*c))
		{
			writer->textspeed = (INT32)((UINT8)*c - 0xA0);
			continue;
		}
		// \xB0 - \xD2 = delay character for up to one second (35 tics)
		else if (IsDelayControlChar((UINT8)*c))
		{
			writer->textcount = (INT32)((UINT8)*c - 0xAF);
			numtowrite = 0;
			continue;
		}

		WriterTextBufferAlloc(writer);

		writer->disptext[writer->writeptr++] = *c;

		// Ignore other control codes (color)
		if ((UINT8)*c < 0x80)
			--numtowrite;
	}
	// Reset textcount for next tic based on speed
	// if it wasn't already set by a delay.
	if (writer->textcount < 0)
	{
		writer->textcount = 0;
		if (writer->textspeed > 7)
			writer->textcount = writer->textspeed - 7;
	}
	return 1;
}

static UINT8 P_DialogWriteText(dialog_t *dialog, textwriter_t *writer)
{
	INT32 numtowrite = 1;
	const char *c;

	unsigned char lastchar = 0;

	(void)dialog;

	if (writer->boostspeed)
	{
		// for custom cutscene speedup mode
		numtowrite = 8;
	}
	else
	{
		// Don't draw any characters if the count was 1 or more when we started
		if (--writer->textcount >= 0)
			return 2;

		if (writer->textspeed < 7)
			numtowrite = 8 - writer->textspeed;
	}

	for (;numtowrite > 0;++writer->baseptr)
	{
		c = &writer->basetext[writer->baseptr];
		if (!c || !*c)
			return 0;

		lastchar = *c;

		// \xA0 - \xAF = change text speed
		if (IsSpeedControlChar(lastchar))
		{
			writer->textspeed = (INT32)(lastchar - 0xA0);
			continue;
		}
		// \xB0 - \xD2 = delay character for up to one second (35 tics)
		else if (IsDelayControlChar(lastchar))
		{
			writer->textcount = (INT32)(lastchar - 0xAF);
			numtowrite = 0;
			continue;
		}

		WriterTextBufferAlloc(writer);

		writer->disptext[writer->writeptr++] = lastchar;

		// Ignore other control codes (color)
		if ((UINT8)lastchar < 0x80)
			--numtowrite;
	}

	// Reset textcount for next tic based on speed
	// if it wasn't already set by a delay.
	if (writer->textcount < 0)
	{
		writer->textcount = 0;
		if (writer->textspeed > 7)
			writer->textcount = writer->textspeed - 7;
	}

	if (!lastchar || isspace(lastchar))
		return 2;
	else
		return 1;
}

void P_ResetTextWriter(textwriter_t *writer, const char *basetext)
{
	writer->basetext = basetext;
	if (writer->disptext && writer->disptext_size)
		memset(writer->disptext,0,writer->disptext_size);
	writer->writeptr = writer->baseptr = 0;
	writer->textspeed = 9;
	writer->textcount = TICRATE/2;
}

// ==================
//  TEXT PROMPTS
// ==================

static void F_GetPageTextGeometry(dialog_t *dialog, UINT8 *pagelines, boolean *rightside, INT32 *boxh, INT32 *texth, INT32 *texty, INT32 *namey, INT32 *chevrony, INT32 *textx, INT32 *textr)
{
	lumpnum_t iconlump = W_CheckNumForName(dialog->page->iconname);

	*pagelines = dialog->page->lines ? dialog->page->lines : 4;
	*rightside = (iconlump != LUMPERROR && dialog->page->rightside);

	// Vertical calculations
	*boxh = *pagelines*2;
	*texth = dialog->page->name[0] ? (*pagelines-1)*2 : *pagelines*2; // name takes up first line if it exists
	*texty = BASEVIDHEIGHT - ((*texth * 4) + (*texth/2)*4);
	*namey = BASEVIDHEIGHT - ((*boxh * 4) + (*boxh/2)*4);
	*chevrony = BASEVIDHEIGHT - (((1*2) * 4) + ((1*2)/2)*4); // force on last line

	// Horizontal calculations
	// Shift text to the right if we have a character icon on the left side
	// Add 4 margin against icon
	*textx = (iconlump != LUMPERROR && !*rightside) ? ((*boxh * 4) + (*boxh/2)*4) + 4 : 4;
	*textr = *rightside ? BASEVIDWIDTH - (((*boxh * 4) + (*boxh/2)*4) + 4) : BASEVIDWIDTH-4;
}

static fixed_t F_GetPromptHideHudBound(dialog_t *dialog)
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	if (!dialog->prompt || !dialog->page ||
		!dialog->page->hidehud ||
		(splitscreen && dialog->page->hidehud != 2)) // don't hide on splitscreen, unless hide all is forced
		return 0;
	else if (dialog->page->hidehud == 2) // hide all
		return BASEVIDHEIGHT;

	F_GetPageTextGeometry(dialog, &pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	// calc boxheight (see V_DrawPromptBack)
	boxh *= vid.dup;
	boxh = (boxh * 4) + (boxh/2)*5; // 4 lines of space plus gaps between and some leeway

	// return a coordinate to check
	// if negative: don't show hud elements below this coordinate (visually)
	// if positive: don't show hud elements above this coordinate (visually)
	return 0 - boxh; // \todo: if prompt at top of screen (someday), make this return positive
}

boolean F_GetPromptHideHudAll(void)
{
	if (!players[displayplayer].promptactive)
		return false;

	dialog_t *dialog = globaltextprompt ? globaltextprompt : players[displayplayer].textprompt;
	if (!dialog)
		return false;

	if (!dialog->prompt || !dialog->page ||
		!dialog->page->hidehud ||
		(splitscreen && dialog->page->hidehud != 2)) // don't hide on splitscreen, unless hide all is forced
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

	if (!players[displayplayer].promptactive)
		return false;

	dialog_t *dialog = globaltextprompt ? globaltextprompt : players[displayplayer].textprompt;
	if (!dialog)
		return false;

	ybound = F_GetPromptHideHudBound(dialog);
	fromtop = (ybound >= 0);
	ytest = (fromtop ? ybound : BASEVIDHEIGHT + ybound);

	return (fromtop ? y < ytest : y >= ytest); // true means hide
}

void P_DialogSetText(dialog_t *dialog, char *pagetext, INT32 numchars)
{
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	F_GetPageTextGeometry(dialog, &pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	if (dialog->pagetext)
		Z_Free(dialog->pagetext);
	dialog->pagetext = (pagetext && pagetext[0]) ? V_WordWrap(textx, textr, 0, pagetext) : Z_StrDup("");

	textwriter_t *writer = &dialog->writer;

	P_ResetTextWriter(writer, dialog->pagetext);

	writer->textspeed = dialog->page->textspeed ? dialog->page->textspeed : TICRATE/5;
	writer->textcount = 0; // no delay in beginning
	writer->boostspeed = 0; // don't print 8 characters to start

	if (numchars <= 0)
		return;

	while (writer->writeptr < numchars)
	{
		const char *c = &writer->basetext[writer->baseptr];
		if (!c || !*c || *c=='#')
			return;

		writer->baseptr++;

		char chr = *c;

		if (!IsSpeedControlChar((UINT8)chr) && !IsDelayControlChar((UINT8)chr))
		{
			WriterTextBufferAlloc(writer);

			writer->disptext[writer->writeptr++] = chr;
		}
	}
}

static void P_PreparePageText(dialog_t *dialog, char *pagetext)
{
	P_DialogSetText(dialog, pagetext, 0);

	// \todo update control hot strings on re-config
	// and somehow don't reset cutscene text counters
}

static void P_DialogStartPage(dialog_t *dialog)
{
	// on page mode, number of tics before allowing boost
	// on timer mode, number of tics until page advances
	dialog->timetonext = dialog->page->timetonext ? dialog->page->timetonext : TICRATE/10;
	P_PreparePageText(dialog, dialog->page->text);

	// gfx
	dialog->numpics = dialog->page->numpics;
	dialog->picnum = dialog->page->pictostart;
	dialog->pictoloop = dialog->page->pictoloop > 0 ? dialog->page->pictoloop - 1 : 0;
	dialog->pictimer = dialog->page->pics[dialog->picnum].duration;
	dialog->picmode = dialog->page->picmode;

	memcpy(dialog->pics, dialog->page->pics, sizeof(cutscene_pic_t) * dialog->page->numpics);

	// music change
	if (dialog->page->musswitch[0])
	{
		S_ChangeMusic(dialog->page->musswitch,
			dialog->page->musswitchflags,
			dialog->page->musicloop);
	}
}

static void P_AdvanceToNextPage(player_t *player, dialog_t *dialog)
{
	INT32 nextprompt = INT32_MAX, nextpage = INT32_MAX;

	if (dialog->page->nextprompt)
		nextprompt = dialog->page->nextprompt - 1;
	if (dialog->page->nextpage)
		nextpage = dialog->page->nextpage - 1;

	textprompt_t *oldprompt = dialog->prompt;

	if (dialog->page->nexttag[0])
		P_GetPromptPageByNamedTag(dialog->page->nexttag, &nextprompt, &nextpage);

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
		P_EndTextPrompt(player, false, false);
	else
		P_DialogStartPage(dialog);
}

void P_FreeTextPrompt(dialog_t *dialog)
{
	if (dialog)
	{
		Z_Free(dialog->writer.disptext);
		Z_Free(dialog);
	}
}

static void P_FreePlayerDialog(player_t *player)
{
	if (player->textprompt == globaltextprompt)
		return;

	P_FreeTextPrompt(player->textprompt);

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
				player->mo->reactiontime = TICRATE/4; // prevent jumping right away // \todo account freeze realtime for this)
			// \todo reset frozen realtime?
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

	player_t *callplayer = globaltextprompt->callplayer;

	if (callplayer)
	{
		if ((callplayer->promptactive || forceexec) && !noexec && globaltextprompt->postexectag)
			postexectag = globaltextprompt->postexectag;
	}

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			P_DoEndDialog(&players[i], globaltextprompt, false, true);
	}

	P_FreeTextPrompt(globaltextprompt);

	globaltextprompt = NULL;

	if (postexectag)
		P_LinedefExecute(postexectag, callplayer->mo, NULL);
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

	skullAnimCounter = 0;

	// Set up state
	dialog->postexectag = postexectag;
	dialog->blockcontrols = blockcontrols;
	(void)freezerealtime; // \todo freeze player->realtime, maybe this needs to cycle through player thinkers

	// Initialize current prompt and scene
	dialog->callplayer = player;
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

		P_DialogStartPage(dialog);
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

void F_TextPromptDrawer(void)
{
	if (!players[displayplayer].promptactive)
		return;

	dialog_t *dialog = globaltextprompt ? globaltextprompt : players[displayplayer].textprompt;
	if (!dialog)
		return;

	lumpnum_t iconlump;
	UINT8 pagelines;
	boolean rightside;
	INT32 boxh, texth, texty, namey, chevrony;
	INT32 textx, textr;

	// Data
	patch_t *patch;

	iconlump = W_CheckNumForName(dialog->page->iconname);
	F_GetPageTextGeometry(dialog, &pagelines, &rightside, &boxh, &texth, &texty, &namey, &chevrony, &textx, &textr);

	// Draw gfx first
	if (dialog->picnum >= 0 && dialog->picnum < dialog->numpics && dialog->pics[dialog->picnum].name[0] != '\0')
	{
		cutscene_pic_t *pic = &dialog->pics[dialog->picnum];

		if (pic->hires)
			V_DrawSmallScaledPatch(pic->xcoord, pic->ycoord, 0,
				W_CachePatchLongName(pic->name, PU_PATCH_LOWPRIORITY));
		else
			V_DrawScaledPatch(pic->xcoord, pic->ycoord, 0,
				W_CachePatchLongName(pic->name, PU_PATCH_LOWPRIORITY));
	}

	// Draw background
	V_DrawPromptBack(boxh, dialog->page->backcolor);

	// Draw narrator icon
	if (iconlump != LUMPERROR)
	{
		INT32 iconx, icony, scale, scaledsize;
		patch = W_CachePatchName(dialog->page->iconname, PU_PATCH_LOWPRIORITY);

		// scale and center
		if (patch->width > patch->height)
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->width);
			scaledsize = FixedMul(patch->height, scale);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = ((namey-4) << FRACBITS) + FixedDiv(BASEVIDHEIGHT - namey + 4 - scaledsize, 2); // account for 4 margin
		}
		else if (patch->height > patch->width)
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->height);
			scaledsize = FixedMul(patch->width, scale);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = namey << FRACBITS;
			iconx += FixedDiv(FixedMul(patch->height, scale) - scaledsize, 2);
		}
		else
		{
			scale = FixedDiv(((boxh * 4) + (boxh/2)*4) - 4, patch->width);
			iconx = (rightside ? BASEVIDWIDTH - (((boxh * 4) + (boxh/2)*4)) : 4) << FRACBITS;
			icony = namey << FRACBITS;
		}

		if (dialog->page->iconflip)
			iconx += FixedMul(patch->width, scale) << FRACBITS;

		V_DrawFixedPatch(iconx, icony, scale, (V_SNAPTOBOTTOM|(dialog->page->iconflip ? V_FLIP : 0)), patch, NULL);
		W_UnlockCachedPatch(patch);
	}

	// Draw text
	if (dialog->writer.disptext)
		V_DrawString(textx, texty, (V_SNAPTOBOTTOM|V_ALLOWLOWERCASE), dialog->writer.disptext);

	// Draw name
	// Don't use V_YELLOWMAP here so that the name color can be changed with control codes
	if (dialog->page->name[0])
		V_DrawString(textx, namey, (V_SNAPTOBOTTOM|V_ALLOWLOWERCASE), dialog->page->name);

	if (globaltextprompt && (globaltextprompt->callplayer != &players[displayplayer]))
		return;

	// Draw chevron
	if (dialog->blockcontrols && !dialog->timetonext)
		V_DrawString(textr-8, chevrony + (skullAnimCounter/5), (V_SNAPTOBOTTOM|V_YELLOWMAP), "\x1B"); // down arrow
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
	if (dialog->picnum < 0 || dialog->picnum >= dialog->numpics)
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

void P_RunTextPrompt(player_t *player)
{
	if (!player->promptactive)
		return;

	dialog_t *dialog = globaltextprompt ? globaltextprompt : player->textprompt;
	if (!dialog)
	{
		player->promptactive = false;
		return;
	}

	textwriter_t *writer = &dialog->writer;

	// advance animation
	writer->boostspeed = 0;

	// for the chevron
	if (P_IsLocalPlayer(player) && --skullAnimCounter <= 0)
		skullAnimCounter = 8;

	player_t *promptplayer = player;

	if (globaltextprompt)
	{
		promptplayer = globaltextprompt->callplayer;

		// Reassign the callplayer if they either quit, or became a spectator
		if (promptplayer->spectator || promptplayer->quittime)
		{
			for (INT32 i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && !(players[i].spectator || players[i].quittime))
				{
					promptplayer = globaltextprompt->callplayer = &players[i];
					break;
				}
			}
		}
	}

	// button handling
	if (dialog->page->timetonext)
	{
		if (dialog->blockcontrols) // same procedure as below, just without the button handling
			P_LockPlayerControls(player);

		if (player == promptplayer)
		{
			if (dialog->timetonext >= 1)
				dialog->timetonext--;

			if (!dialog->timetonext)
			{
				P_AdvanceToNextPage(player, dialog);
				return;
			}
			else
			{
				INT32 write = P_DialogWriteText(dialog, &dialog->writer);
				if (write == 1 && dialog->page->textsfx)
					S_StartSound(NULL, dialog->page->textsfx);
			}
		}
	}
	else
	{
		if (dialog->blockcontrols)
		{
			P_LockPlayerControls(player);

			if ((promptplayer->cmd.buttons & BT_SPIN) || (promptplayer->cmd.buttons & BT_JUMP))
			{
				if (dialog->timetonext > 1)
					dialog->timetonext--;
				else if (writer->baseptr) // don't set boost if we just reset the string
					writer->boostspeed = 1; // only after a slight delay

				if (!dialog->timetonext) // is 0 when finished generating text
				{
					P_AdvanceToNextPage(player, dialog);

					if (promptplayer->promptactive)
						S_StartSound(NULL, sfx_menu1);

					return;
				}
			}
		}

		// never show the chevron if we can't toggle pages
		if (dialog->pagenum >= MAX_PAGES || !dialog->page->text || !dialog->page->text[0])
			dialog->timetonext = !dialog->blockcontrols;

		// generate letter-by-letter text
		if (player == promptplayer)
		{
			INT32 write = P_DialogWriteText(dialog, &dialog->writer);
			if (write)
			{
				if (write == 1 && dialog->page->textsfx)
					S_StartSound(NULL, dialog->page->textsfx);
			}
			else
				dialog->timetonext = !dialog->blockcontrols;
		}
	}

	if (player == promptplayer)
		P_UpdatePromptGfx(dialog);
}
