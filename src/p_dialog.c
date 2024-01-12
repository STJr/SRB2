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
#include "hu_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_tokenizer.h"
#include "deh_soc.h"
#include "fastcmp.h"

#include <errno.h>

static INT16 chevronAnimCounter;

static boolean IsCutsceneSpeedControlChar(UINT8 chr)
{
	return chr >= 0xA0 && chr <= 0xAF;
}

static boolean IsCutsceneDelayControlChar(UINT8 chr)
{
	return chr >= 0xB0 && chr <= (0xB0+TICRATE-1);
}

static void WriterTextBufferAlloc(textwriter_t *writer)
{
	if (!writer->disptextsize)
		writer->disptextsize = 16;

	size_t oldsize = writer->disptextsize;

	while (((unsigned)writer->writeptr) + 1 >= writer->disptextsize)
		writer->disptextsize *= 2;

	if (!writer->disptext || oldsize != writer->disptextsize)
		writer->disptext = Z_Realloc(writer->disptext, writer->disptextsize, PU_STATIC, NULL);
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
		if (IsCutsceneSpeedControlChar((UINT8)*c))
		{
			writer->textspeed = (INT32)((UINT8)*c - 0xA0);
			continue;
		}
		// \xB0 - \xD2 = delay character for up to one second (35 tics)
		else if (IsCutsceneDelayControlChar((UINT8)*c))
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

static INT32 P_FindTextPromptSlot(const char *name)
{
	INT32 id = P_GetTextPromptByName(name);
	if (id != -1)
		return id;

	for (INT32 i = 0; i < MAX_PROMPTS; i++)
	{
		if (!textprompts[i])
			return i;
	}

	return -1;
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
		Z_Free(page->text);
	}

	Z_Free(prompt->name);
	Z_Free(prompt);
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

static void F_GetPageTextGeometry(dialog_t *dialog, dialog_geometry_t *geo)
{
	lumpnum_t iconlump = LUMPERROR;

	if (dialog->icon[0])
		iconlump = W_CheckNumForLongName(dialog->icon);

	INT32 pagelines = dialog->page->lines ? dialog->page->lines : 4;
	boolean rightside = iconlump != LUMPERROR && dialog->page->rightside;

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
	geo->textx = (iconlump != LUMPERROR && !rightside) ? ((boxh * 4) + (boxh/2)*4) + 4 : 4;
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

	F_GetPageTextGeometry(dialog, &geo);

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

	dialog_t *dialog = globaltextprompt ? globaltextprompt : stplyr->textprompt;
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

	if (!stplyr || !stplyr->promptactive)
		return false;

	dialog_t *dialog = globaltextprompt ? globaltextprompt : stplyr->textprompt;
	if (!dialog)
		return false;

	ybound = F_GetPromptHideHudBound(dialog);
	fromtop = (ybound >= 0);
	ytest = (fromtop ? ybound : BASEVIDHEIGHT + ybound);

	return (fromtop ? y < ytest : y >= ytest); // true means hide
}

void P_SetMetaPage(textpage_t *page, textpage_t *metapage)
{
	strlcpy(page->name, metapage->name, sizeof(page->name));
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

static void P_SetDialogSpeaker(dialog_t *dialog, const char *speaker)
{
	char name[256];

	// Add yellow control char
	name[0] = '\x82';
	name[1] = 0;

	strlcat(name, speaker, sizeof(name));
	strlcpy(dialog->speaker, name, sizeof(dialog->speaker));
}

#define READ_NUM(where) { \
	int temp = 0; \
	temp |= (*code) << 24; code++; \
	temp |= (*code) << 16; code++; \
	temp |= (*code) << 8; code++; \
	temp |= (*code); code++; \
	where = temp; \
}

static char *BytecodeReadString(const UINT8 **code)
{
	const UINT8 *c = *code;
	UINT8 sz = *c++;
	if (!sz)
		return NULL;
	char *str = Z_Malloc(sz + 1, PU_STATIC, NULL);
	char *str_p = str;
	while (sz--)
		*str_p++ = *c++;
	*str_p = '\0';
	*code = c;
	return str;
}

// Dialog control codes
enum
{
	TP_OP_SPEED,
	TP_OP_DELAY,

	TP_OP_NAME,
	TP_OP_ICON,

	TP_OP_CHARNAME,
	TP_OP_PLAYERNAME,

	TP_OP_CONTROL = 0xFF
};

static boolean ProcessInstruction(dialog_t *dialog, UINT8 **cptr, UINT8 **buffer, size_t *buffer_pos, size_t *buffer_capacity)
{
	UINT8 *code = *cptr;
	if (*code++ != TP_OP_CONTROL)
		return false;

	player_t *player = dialog->callplayer;

	switch (*code)
	{
		case TP_OP_CHARNAME: {
			char charname[256];
			strlcpy(charname, skins[player->skin]->realname, sizeof(charname));
			M_BufferMemWrite((UINT8 *)charname, strlen(charname), buffer, buffer_pos, buffer_capacity);
			break;
		}
		case TP_OP_PLAYERNAME: {
			char playername[256];
			if (netgame)
				strlcpy(playername, player_names[player-players], sizeof(playername));
			else
				strlcpy(playername, skins[player->skin]->realname, sizeof(playername));
			M_BufferMemWrite((UINT8 *)playername, strlen(playername), buffer, buffer_pos, buffer_capacity);
			break;
		}
		default:
			return false;
	}

	*cptr = code;

	return true;
}

static const UINT8 *InterpretBytecode(const UINT8 *code, dialog_t *dialog, textwriter_t *writer)
{
	int num = 0;

	switch (*code++)
	{
		case TP_OP_SPEED:
			READ_NUM(num);
			writer->textspeed = num;
			break;
		case TP_OP_DELAY:
			READ_NUM(num);
			writer->textcount = num;
			writer->numtowrite = 0;
			break;
		case TP_OP_NAME: {
			char *speaker = BytecodeReadString(&code);
			if (speaker)
			{
				P_SetDialogSpeaker(dialog, speaker);
				Z_Free(speaker);
			}
			break;
		}
		case TP_OP_ICON: {
			char *icon = BytecodeReadString(&code);
			if (icon)
			{
				strlcpy(dialog->icon, icon, sizeof(dialog->icon));
				Z_Free(icon);
			}
			break;
		case TP_OP_CHARNAME:
		case TP_OP_PLAYERNAME:
			// Ignore
			break;
		}
	}

	return code;
}

static int SkipBytecode(const UINT8 *code)
{
	const UINT8 *baseptr = code;

	switch (*code++)
	{
		case TP_OP_SPEED:
		case TP_OP_DELAY:
			code += 4;
			break;
		case TP_OP_NAME:
		case TP_OP_ICON: {
			UINT8 n = *code++;
			code += n;
			break;
		case TP_OP_CHARNAME:
		case TP_OP_PLAYERNAME:
			// Nothing else to read
			break;
		}
	}

	return code - baseptr;
}

#undef READ_NUM

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
			i += SkipBytecode(code + 1);
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

	if (writer->boostspeed)
		writer->numtowrite = 8;
	else
	{
		// Don't draw any characters if the count was 1 or more when we started
		if (--writer->textcount >= 0)
			return DIALOG_WRITETEXT_SILENT;

		if (writer->textspeed < 7)
			writer->numtowrite = 8 - writer->textspeed;
	}

	while (writer->numtowrite > 0)
	{
		const UINT8 *c = &baseptr[writer->baseptr];
		if (!*c)
			return DIALOG_WRITETEXT_DONE;

		const UINT8 *code = (const UINT8*)c;
		if (*code == TP_OP_CONTROL)
		{
			code = InterpretBytecode(code + 1, dialog, writer);
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
		if (lastchar < 0x80)
			--writer->numtowrite;
	}

	// Reset textcount for next tic based on speed
	// if it wasn't already set by a delay.
	if (writer->textcount < 0)
	{
		writer->textcount = 0;
		if (writer->textspeed > 7)
			writer->textcount = writer->textspeed - 7;
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

	F_GetPageTextGeometry(dialog, &geo);

	V_WordWrapInPlace(geo.textx, geo.textr, 0, dialog->disptext);
}

static char *P_ProcessPageText(dialog_t *dialog, UINT8 *pagetext, size_t textlength, size_t *outlength)
{
	size_t buffer_pos = 0;
	size_t buffer_capacity = 0;

	UINT8 *buf = NULL;

	UINT8 *text = pagetext;
	UINT8 *end = pagetext + textlength;

	while (text < end)
	{
		if (!ProcessInstruction(dialog, &text, &buf, &buffer_pos, &buffer_capacity))
		{
			M_StringBufferWrite(*text, (char**)&buf, &buffer_pos, &buffer_capacity);
			text++;
		}
	}

	*outlength = buffer_pos;

	return (char*)buf;
}

void P_DialogSetText(dialog_t *dialog, char *pagetext, size_t textlength)
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

	writer->textspeed = dialog->page->textspeed ? dialog->page->textspeed : TICRATE/5;
	writer->textcount = 0; // no delay in beginning
	writer->boostspeed = false; // don't print 8 characters to start

	P_DialogSetDisplayText(dialog);
}

static void P_PreparePageText(dialog_t *dialog, char *pagetext, size_t textlength)
{
	P_DialogSetText(dialog, pagetext, textlength);

	// \todo update control hot strings on re-config
	// and somehow don't reset cutscene text counters
}

static void P_DialogStartPage(player_t *player, dialog_t *dialog)
{
	P_PreparePageText(dialog, dialog->page->text, dialog->page->textlength);

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

	strlcpy(dialog->icon, dialog->page->iconname, sizeof(dialog->icon));

	dialog->iconflip = dialog->page->iconflip;

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

	P_FreeDialog(globaltextprompt);

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

	chevronAnimCounter = 0;

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

static void F_DrawDialogText(INT32 x, INT32 y, INT32 option, const char *string, size_t numchars)
{
	INT32 w, c, cx = x, cy = y, dup, scrwidth, left = 0;
	const char *ch = string;
	INT32 charflags = 0;
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
			cx = x;
			cy += 12*dup;
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
		V_DrawFixedPatch(cx<<FRACBITS, cy<<FRACBITS, FRACUNIT, option, hu_font[c], colormap);

		cx += w;
	}
}

void F_TextPromptDrawer(void)
{
	if (!stplyr || !stplyr->promptactive)
		return;

	dialog_t *dialog = globaltextprompt ? globaltextprompt : stplyr->textprompt;
	if (!dialog)
		return;

	lumpnum_t iconlump = LUMPERROR;
	dialog_geometry_t geo;

	INT32 draw_flags = V_PERPLAYER;
	INT32 snap_flags = V_SNAPTOBOTTOM;

	// Data
	patch_t *patch;

	F_GetPageTextGeometry(dialog, &geo);

	boolean rightside = geo.rightside;
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
	if (dialog->icon[0])
		iconlump = W_CheckNumForLongName(dialog->icon);

	if (iconlump != LUMPERROR)
	{
		INT32 iconx, icony, scale, scaledsize;
		patch = W_CachePatchLongName(dialog->icon, PU_PATCH_LOWPRIORITY);

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

		if (dialog->iconflip)
			iconx += FixedMul(patch->width, scale) << FRACBITS;

		V_DrawFixedPatch(iconx, icony, scale, (draw_flags|snap_flags|(dialog->iconflip ? V_FLIP : 0)), patch, NULL);
		W_UnlockCachedPatch(patch);
	}

	// Draw text
	if (dialog->disptext)
		F_DrawDialogText(textx, texty, (draw_flags|snap_flags|V_ALLOWLOWERCASE), dialog->disptext, dialog->writer.writeptr);

	// Draw name
	// Don't use V_YELLOWMAP here so that the name color can be changed with control codes
	if (dialog->speaker[0])
		V_DrawString(textx, namey, (draw_flags|snap_flags|V_ALLOWLOWERCASE), dialog->speaker);

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

			boolean selected = dialog->curchoice == i;

			if (selected)
				flags |= V_YELLOWMAP;

			V_DrawString(textx, choicesy, flags, text);

			if (selected)
				V_DrawString(choicesx + (chevronAnimCounter/5), choicesy, choices_flags|V_YELLOWMAP, "\x1D");

			choicesy += 10;
		}
	}

	if (globaltextprompt && (globaltextprompt->callplayer != &players[displayplayer]))
		return;

	// Draw chevron
	if (dialog->blockcontrols && !dialog->timetonext && !dialog->showchoices)
		V_DrawString(textr-8, chevrony + (chevronAnimCounter/5), (draw_flags|snap_flags|V_YELLOWMAP), "\x1B"); // down arrow
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

static dialog_t *P_GetPlayerDialog(player_t *player)
{
	return globaltextprompt ? globaltextprompt : player->textprompt;
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

	player_t *promptplayer = dialog->callplayer;
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
		else
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
			else if ((player->cmd.buttons & BT_SPIN) || (player->cmd.buttons & BT_JUMP))
			{
				if (dialog->timetonext > 1)
					dialog->timetonext--;
				else if (writer->baseptr) // don't set boost if we just reset the string
					writer->boostspeed = true; // only after a slight delay

				if (!dialog->timetonext && !dialog->showchoices) // timetonext is 0 when finished generating text
				{
					if (P_AdvanceToNextPage(player, dialog))
						P_PlayDialogSound(player, sfx_menu1);

					return;
				}
			}
		}

		// never show the chevron if we can't toggle pages
		if (!dialog->page->text || !dialog->page->text[0])
			dialog->timetonext = !dialog->blockcontrols;

		// generate letter-by-letter text
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

	P_UpdatePromptGfx(dialog);
}

// DIALOGUE parsing
static boolean StringToNumber(const char *tkn, int *out)
{
	char *endPos = NULL;

#ifndef AVOID_ERRNO
	errno = 0;
#endif

	int result = strtol(tkn, &endPos, 10);
	if (endPos == tkn || *endPos != '\0')
		return false;

#ifndef AVOID_ERRNO
	if (errno == ERANGE)
		return false;
#endif

	*out = result;

	return true;
}

INT32 P_ParsePromptBackColor(const char *word)
{
	struct {
		const char *name;
		int id;
	} all_colors[] = {
		{ "white",      0 },
		{ "gray",       1 },
		{ "grey",       1 },
		{ "black",      1 },
		{ "sepia",      2 },
		{ "brown",      3 },
		{ "pink",       4 },
		{ "raspberry",  5 },
		{ "red",        6 },
		{ "creamsicle", 7 },
		{ "orange",     8 },
		{ "gold",       9 },
		{ "yellow",     10 },
		{ "emerald",    11 },
		{ "green",      12 },
		{ "cyan",       13 },
		{ "aqua",       13 },
		{ "steel",      14 },
		{ "periwinkle", 15 },
		{ "blue",       16 },
		{ "purple",     17 },
		{ "lavender",   18 }
	};

	for (size_t i = 0; i < sizeof(all_colors) / sizeof(all_colors[0]); i++)
	{
		if (fastcmp(word, all_colors[i].name))
			return all_colors[i].id;
	}

	return -1;
}

#define GET_TOKEN() \
	tkn = sc->get(sc, 0); \
	if (!tkn) \
	{ \
		CONS_Alert(CONS_ERROR, "Error parsing DIALOGUE: Unexpected EOF\n"); \
		goto fail; \
	}

#define IS_TOKEN(expected) \
	if (!tkn || strcmp(tkn, expected) != 0) \
	{ \
		if (tkn) \
			CONS_Alert(CONS_ERROR, "Error parsing DIALOGUE: Expected '%s', got '%s'\n", expected, tkn); \
		else \
			CONS_Alert(CONS_ERROR, "Error parsing DIALOGUE: Expected '%s', got EOF\n", expected); \
		goto fail; \
	}

#define EXPECT_TOKEN(expected) \
	GET_TOKEN(); \
	IS_TOKEN(expected)

#define CHECK_TOKEN(check) (strcmp(tkn, check) == 0)

#define EXPECT_NUMBER(what) \
	int num = 0; \
	if (!StringToNumber(tkn, &num)) \
	{ \
		CONS_Alert(CONS_ERROR, "Error parsing " what ": expected a number, got '%s'\n", tkn); \
		goto fail; \
	}

#define IGNORE_FIELD() \
	EXPECT_TOKEN("="); \
	GET_TOKEN(); \
	EXPECT_TOKEN(";")

static void ParseChoice(textpage_t *page, tokenizer_t *sc)
{
	const char *tkn;

	if (page->numchoices == MAX_PROMPT_CHOICES)
		abort();

	page->numchoices++;

	page->choices = Z_Realloc(page->choices, sizeof(promptchoice_t) * page->numchoices, PU_STATIC, NULL);

	promptchoice_t *choice = &page->choices[page->numchoices - 1];

	INT32 choiceid = page->numchoices;

	GET_TOKEN();

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("text"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(choice->text);

			choice->text = Z_StrDup(tkn);

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextpage"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(choice->nextpagename);

			int num = 0;
			if (StringToNumber(tkn, &num))
				choice->nextpage = num;
			else
			{
				choice->nextpage = 0;
				choice->nextpagename = Z_StrDup(tkn);
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextconversation"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(choice->nextpromptname);

			int num = 0;
			if (StringToNumber(tkn, &num))
				choice->nextprompt = num;
			else
			{
				choice->nextprompt = 0;
				choice->nextpromptname = Z_StrDup(tkn);
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nexttag"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(choice->nexttag, tkn, sizeof(choice->nexttag));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("executelinedef"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("choice 'executelinedef'");

			choice->exectag = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("highlighted"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->startchoice = choiceid;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nochoice"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->nochoice = choiceid;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("closedialog"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				choice->endprompt = true;
			else if (CHECK_TOKEN("false"))
				choice->endprompt = false;

			EXPECT_TOKEN(";");
		}
		else
		{
			CONS_Alert(CONS_WARNING, "While parsing choice: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		tkn = sc->get(sc, 0);
	}

	return;

fail:
	while (tkn && !CHECK_TOKEN("}"))
		tkn = sc->get(sc, 0);

	return;
}

static void ParsePicture(textpage_t *page, tokenizer_t *sc)
{
	const char *tkn;

	if (page->numpics == MAX_PROMPT_PICS)
		abort();

	page->numpics++;

	page->pics = Z_Realloc(page->pics, sizeof(cutscene_pic_t) * page->numpics, PU_STATIC, NULL);

	cutscene_pic_t *pic = &page->pics[page->numpics - 1];

	INT32 picid = page->numpics;

	GET_TOKEN();

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("name"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(pic->name, tkn, sizeof(pic->name));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("x"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("picture 'x'");

			pic->xcoord = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("y"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("picture 'y'");

			pic->ycoord = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("duration"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("picture 'duration'");

			if (num < 0)
				num = 0;

			pic->duration = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("hires"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				pic->hires = true;
			else if (CHECK_TOKEN("false"))
				pic->hires = false;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("start"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->pictostart = picid - 1;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("looppoint"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->pictoloop = picid;

			EXPECT_TOKEN(";");
		}
		else
		{
			CONS_Alert(CONS_WARNING, "While parsing pic: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		tkn = sc->get(sc, 0);
	}

	return;

fail:
	while (tkn && !CHECK_TOKEN("}"))
		tkn = sc->get(sc, 0);

	return;
}

#define BUFWRITE(writechr) M_StringBufferWrite((writechr), &buf, &buffer_pos, &buffer_capacity)

static char *EscapeStringChars(const char *string)
{
	size_t buffer_pos = 0;
	size_t buffer_capacity = 0;

	char *buf = NULL;

	while (*string)
	{
		char chr = *string++;

		if (chr == '\\')
		{
			chr = *string++;

			switch (chr)
			{
			case 'n':
				BUFWRITE('\n');
				break;
			case 't':
				BUFWRITE('\t');
				break;
			case '\\':
				BUFWRITE('\\');
				break;
			case '"':
				BUFWRITE('\"');
				break;
			case '\'':
				BUFWRITE('\'');
				break;
			case 'x': {
				int out = 0, c;
				int i = 0;

				chr = *string++;

				for (i = 0; i < 5 && isxdigit(chr); i++)
				{
					c = ((chr <= '9') ? (chr - '0') : (tolower(chr) - 'a' + 10));
					out = (out << 4) | c;
					chr = *string++;
				}

				string--;

				switch (i)
				{
					case 4:
						BUFWRITE((out >> 8) & 0xFF);
						buf++;
						/* FALLTHRU */
					case 2:
						BUFWRITE(out & 0xFF);
						break;
					default:
						CONS_Alert(CONS_WARNING, "While parsing string: escape sequence has wrong size\n");
						goto fail;
				}

				break;
			}
			default:
				if (isdigit(chr))
				{
					int out = 0;
					int i = 0;
					do
					{
						out = 10*out + (chr - '0');
						chr = *string++;
					} while (++i < 3 && isdigit(chr));

					string--;

					if (out > 255)
					{
						CONS_Alert(CONS_WARNING, "While parsing string: escape sequence is too large\n");
						goto fail;
					}

					BUFWRITE((char)out);
				}
				break;
			}
		}
		else
		{
			BUFWRITE(chr);
		}
	}

	return buf;

fail:
	Z_Free(buf);
	return NULL;
}

#undef BUFWRITE

static boolean MatchControlCode(const char *match, char **input)
{
	char *start = *input;
	while (isspace(*start))
	{
		if (*start == '\0')
			return false;
		start++;
	}

	char *end = start;
	while (*end != '}')
	{
		if (*end == '\0')
			return false;
		if (isspace(*end))
			break;
		end++;
	}

	size_t tknlen = end - start;
	size_t matchlen = strlen(match);

	if (tknlen != matchlen)
		return false;

	if (memcmp(match, start, tknlen) == 0)
	{
		while (isspace(*end))
			end++;
		*input = end;
		return true;
	}

	return false;
}

static char *GetControlCodeParam(char **input)
{
	char *start = *input;
	if (*start == '}')
		return NULL;

	char *end = start;
	while (*end != '}')
	{
		if (*end == '\0')
			return NULL;
		end++;
	}

	size_t tknlen = end - start;
	if (!tknlen)
		return NULL;

	while (isspace(start[tknlen - 1]))
	{
		tknlen--;
		if (!tknlen)
			return NULL;
	}

	char *result = Z_Malloc(tknlen + 1, PU_STATIC, NULL);
	memcpy(result, start, tknlen);
	result[tknlen] = '\0';

	*input = end;

	return result;
}

static void GetControlCodeEnd(char **input)
{
	char *start = *input;
	if (*start == '}')
		return;

	char *end = start;
	while (*end != '}')
	{
		if (*end == '\0')
			return;
		end++;
	}

	*input = end;
}

#define WRITE_CHAR(writechr) M_BufferWrite((writechr), buf, buffer_pos, buffer_capacity)
#define WRITE_OP(writeop) { \
	WRITE_CHAR(0xFF); \
	WRITE_CHAR((writeop)); \
}
#define WRITE_NUM(num) { \
	int n = num; \
	WRITE_CHAR((n >> 24) & 0xFF); \
	WRITE_CHAR((n >> 16) & 0xFF); \
	WRITE_CHAR((n >> 8) & 0xFF); \
	WRITE_CHAR((n) & 0xFF); \
}
#define WRITE_STRING(str) { \
	size_t maxlen = strlen(str); \
	if (maxlen > 255) \
		maxlen = 255; \
	WRITE_CHAR(maxlen); \
	for (size_t i = 0; i < maxlen; i++) \
		WRITE_CHAR(str[i]); \
}

#define EXPECTED_NUMBER(which) CONS_Alert(CONS_WARNING, "Expected integer in '%s', got '%s' instead\n", which, param)
#define EXPECTED_PARAM(which) CONS_Alert(CONS_WARNING, "Expected parameter for '%s'\n", which)

static void InterpretControlCode(char **input, UINT8 **buf, size_t *buffer_pos, size_t *buffer_capacity)
{
	if (MatchControlCode("PAUSE", input))
	{
		int pause_frames = 12;

		char *param = GetControlCodeParam(input);
		if (param)
		{
			if (!StringToNumber(param, &pause_frames))
				EXPECTED_NUMBER("PAUSE");

			Z_Free(param);
		}

		WRITE_OP(TP_OP_DELAY);
		WRITE_NUM(pause_frames);
	}
	else if (MatchControlCode("SPEED", input))
	{
		char *param = GetControlCodeParam(input);
		if (param)
		{
			int speed = 0;

			if (!StringToNumber(param, &speed))
				EXPECTED_NUMBER("SPEED");

			Z_Free(param);

			WRITE_OP(TP_OP_SPEED);
			WRITE_NUM(speed);
		}
		else
			EXPECTED_PARAM("SPEED");
	}
	else if (MatchControlCode("NAME", input))
	{
		char *param = GetControlCodeParam(input);
		if (param)
		{
			WRITE_OP(TP_OP_NAME);
			WRITE_STRING(param);

			Z_Free(param);
		}
		else
			EXPECTED_PARAM("NAME");
	}
	else if (MatchControlCode("ICON", input))
	{
		char *param = GetControlCodeParam(input);
		if (param)
		{
			WRITE_OP(TP_OP_ICON);
			WRITE_STRING(param);

			Z_Free(param);
		}
		else
			EXPECTED_PARAM("ICON");
	}
	else if (MatchControlCode("CHARNAME", input))
	{
		WRITE_OP(TP_OP_CHARNAME);
	}
	else if (MatchControlCode("PLAYERNAME", input))
	{
		WRITE_OP(TP_OP_PLAYERNAME);
	}
}

static boolean InterpretCutsceneControlCode(UINT8 chr, UINT8 **buf, size_t *buffer_pos, size_t *buffer_capacity)
{
	if (IsCutsceneSpeedControlChar(chr))
	{
		WRITE_OP(TP_OP_SPEED);
		WRITE_NUM((UINT8)chr - 0xA0);
		return true;
	}
	else if (IsCutsceneDelayControlChar(chr))
	{
		WRITE_OP(TP_OP_DELAY);
		WRITE_NUM((UINT8)chr - 0xAF);
		return true;
	}

	return false;
}

#undef EXPECTED_NUMBER
#undef EXPECTED_PARAM

#undef WRITE_CHAR
#undef WRITE_OP
#undef WRITE_NUM

#define WRITE_TEXTCHAR(writechr) M_BufferWrite((writechr), &buf, &buffer_pos, &buffer_capacity)

static char *ParsePageDialog(char *text, size_t *text_length)
{
	char *input = text;

	size_t buffer_pos = 0;
	size_t buffer_capacity = 0;

	UINT8 *buf = NULL;

	while (*input)
	{
		unsigned char chr = *input;

		if (chr == '\\')
		{
			if (input[1] == '{')
			{
				WRITE_TEXTCHAR(input[1]);
				input += 2;
			}
			else
				WRITE_TEXTCHAR(chr);
		}
		else if (chr == 0xFF)
			; // Just ignore it
		else if (chr == '{')
		{
			input++;

			InterpretControlCode(&input, &buf, &buffer_pos, &buffer_capacity);

			GetControlCodeEnd(&input);
		}
		else
		{
			WRITE_TEXTCHAR(chr);
		}

		input++;
	}

	WRITE_TEXTCHAR('\0');

	*text_length = buffer_pos;

	return (char*)buf;
}

char *P_ConvertSOCPageDialog(char *text, size_t *text_length)
{
	char *input = text;

	size_t buffer_pos = 0;
	size_t buffer_capacity = 0;

	UINT8 *buf = NULL;

	while (*input)
	{
		unsigned char chr = *input;

		if (!InterpretCutsceneControlCode((UINT8)chr, &buf, &buffer_pos, &buffer_capacity))
		{
			if (chr != 0xFF)
				WRITE_TEXTCHAR(chr);
		}

		input++;
	}

	WRITE_TEXTCHAR('\0');

	*text_length = buffer_pos;

	return (char*)buf;
}

#undef WRITE_TEXTCHAR

static void ParsePage(textprompt_t *prompt, tokenizer_t *sc)
{
	const char *tkn;

	if (prompt->numpages == MAX_PAGES)
		abort();

	textpage_t *page = &prompt->page[prompt->numpages];

	page->backcolor = 1; // default to gray
	page->hidehud = 1; // hide appropriate HUD elements

	prompt->numpages++;

	GET_TOKEN();

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("pagename"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(page->pagename);

			page->pagename = Z_StrDup(tkn);

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("name"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(page->name, tkn, sizeof(page->name));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("dialog"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(page->text);

			char *escaped = EscapeStringChars(tkn);
			if (escaped)
			{
				page->text = ParsePageDialog(escaped, &page->textlength);
				Z_Free(escaped);
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("icon"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(page->iconname, tkn, sizeof(page->iconname));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("tag"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(page->tag, tkn, sizeof(page->tag));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("textsound"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			page->textsfx = get_sfx(tkn);

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextpage"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(page->nextpagename);

			int num = 0;
			if (StringToNumber(tkn, &num))
				page->nextpage = num;
			else
			{
				page->nextpage = 0;
				page->nextpagename = Z_StrDup(tkn);
			}

			page->nextpage = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextconversation"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			Z_Free(page->nextpromptname);

			int num = 0;
			if (StringToNumber(tkn, &num))
				page->nextprompt = num;
			else
			{
				page->nextprompt = 0;
				page->nextpromptname = Z_StrDup(tkn);
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nexttag"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(page->nexttag, tkn, sizeof(page->nexttag));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("duration"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'duration'");

			page->timetonext = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("textspeed"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'textspeed'");

			page->textspeed = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("textlines"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'textlines'");

			page->lines = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("iconside"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("right"))
				page->rightside = true;
			else if (CHECK_TOKEN("left"))
				page->rightside = false;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("flipicon"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->iconflip = true;
			else if (CHECK_TOKEN("false"))
				page->iconflip = false;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("displayhud"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("show"))
				page->hidehud = 0;
			else if (CHECK_TOKEN("hide"))
				page->hidehud = 1;
			else if (CHECK_TOKEN("hideall"))
				page->hidehud = 2;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("backcolor"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			page->backcolor = P_ParsePromptBackColor(tkn);

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("choice"))
		{
			EXPECT_TOKEN("{");

			if (page->numchoices == MAX_PROMPT_CHOICES)
			{
				while (!CHECK_TOKEN("}"))
				{
					GET_TOKEN();
				}
			}
			else
			{
				ParseChoice(page, sc);

				if (page->numchoices == MAX_PROMPT_CHOICES)
					CONS_Alert(CONS_WARNING, "Conversation page exceeded max amount of choices; ignoring any more choices\n");
			}
		}
		else if (CHECK_TOKEN("alignchoices"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("left"))
				page->choicesleftside = true;
			else if (CHECK_TOKEN("right"))
				page->choicesleftside = false;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("picture"))
		{
			EXPECT_TOKEN("{");

			if (page->numpics == MAX_PROMPT_PICS)
			{
				while (!CHECK_TOKEN("}"))
				{
					GET_TOKEN();
				}
			}
			else
			{
				ParsePicture(page, sc);

				if (page->numpics == MAX_PROMPT_PICS)
					CONS_Alert(CONS_WARNING, "Conversation page exceeded max amount of pictures; ignoring any more pictures\n");
			}
		}
		else if (CHECK_TOKEN("picturesequence"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("persist"))
				page->picmode = 0;
			else if (CHECK_TOKEN("loop"))
				page->picmode = 1;
			else if (CHECK_TOKEN("hide"))
				page->picmode = 2;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("music"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			strlcpy(page->musswitch, tkn, sizeof(page->musswitch));

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("musictrack"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'musictrack'");

			if (num < 0)
				num = 0;

			page->musswitchflags = ((UINT16)num) & MUSIC_TRACKMASK;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("loopmusic"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->musicloop = 1;
			else if (CHECK_TOKEN("false"))
				page->musicloop = 0;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("executelinedef"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'executelinedef'");

			page->exectag = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("restoremusic"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->restoremusic = true;
			else if (CHECK_TOKEN("false"))
				page->restoremusic = false;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("closedialog"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("true"))
				page->endprompt = true;
			else if (CHECK_TOKEN("false"))
				page->endprompt = false;

			EXPECT_TOKEN(";");
		}
#if 0
		else if (CHECK_TOKEN("metapage"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'metapage'");

			if (num > 0 && num <= prompt->numpages)
				P_SetMetaPage(page, &prompt->page[num - 1]);

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("picsmetapage"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'picsmetapage'");

			if (num > 0 && num <= prompt->numpages)
				P_SetPicsMetaPage(page, &prompt->page[num - 1]);

			EXPECT_TOKEN(";");
		}
#endif
		else
		{
			CONS_Alert(CONS_WARNING, "While parsing page: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		tkn = sc->get(sc, 0);
	}

	return;

fail:
	while (tkn && !CHECK_TOKEN("}"))
		tkn = sc->get(sc, 0);

	return;
}

static void ParseConversation(tokenizer_t *sc)
{
	INT32 id = 0;

	char *name = NULL;

	textprompt_t *prompt = NULL;

	const char *tkn;

	GET_TOKEN();

	prompt = Z_Calloc(sizeof(textprompt_t), PU_STATIC, NULL);

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("id"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			int num = 0;
			if (StringToNumber(tkn, &num))
				id = num;
			else
				name = Z_StrDup(tkn); // Must be named, then

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("page"))
		{
			EXPECT_TOKEN("{");

			if (prompt->numpages == MAX_PAGES)
			{
				while (!CHECK_TOKEN("}"))
				{
					GET_TOKEN();
				}
			}
			else
			{
				ParsePage(prompt, sc);

				if (prompt->numpages == MAX_PAGES)
					CONS_Alert(CONS_WARNING, "Conversation exceeded max amount of pages; ignoring any more pages\n");
			}
		}
		else
		{
			CONS_Alert(CONS_WARNING, "While parsing conversation: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		tkn = sc->get(sc, 0);
	}

	if (!prompt->numpages)
	{
		CONS_Alert(CONS_WARNING, "Conversation has no pages\n");
		goto fail;
	}

	if (name)
	{
		id = P_FindTextPromptSlot(name);

		if (id < 0)
		{
			CONS_Alert(CONS_WARNING, "No more free conversation slots\n");
			goto fail;
		}

		prompt->name = Z_StrDup(name);

		P_FreeTextPrompt(textprompts[id]);

		textprompts[id] = prompt;
	}
	else if (id)
	{
		if (id <= 0 || id > MAX_PROMPTS)
		{
			CONS_Alert(CONS_WARNING, "Conversation ID %d out of range (1 - %d)\n", id, MAX_PROMPTS);
			goto fail;
		}

		--id;

		P_FreeTextPrompt(textprompts[id]);

		textprompts[id] = prompt;
	}
	else
	{
		CONS_Alert(CONS_WARNING, "Conversation has missing ID\n");
fail:
		Z_Free(prompt);
		Z_Free(name);
	}
}

static void ParseDialogue(UINT16 wadNum, UINT16 lumpnum)
{
	char *lumpData = (char *)W_CacheLumpNumPwad(wadNum, lumpnum, PU_STATIC);
	size_t lumpLength = W_LumpLengthPwad(wadNum, lumpnum);
	char *text = (char *)Z_Malloc((lumpLength + 1), PU_STATIC, NULL);
	memmove(text, lumpData, lumpLength);
	text[lumpLength] = '\0';
	Z_Free(lumpData);

	tokenizer_t *sc = Tokenizer_Open(text, 1);
	const char *tkn = sc->get(sc, 0);

	// Look for namespace at the beginning.
	if (strcmp(tkn, "namespace") != 0)
	{
		CONS_Alert(CONS_ERROR, "No namespace at beginning of DIALOGUE text!\n");
		goto fail;
	}

	EXPECT_TOKEN("=");

	// Check if namespace is valid.
	GET_TOKEN();
	if (strcmp(tkn, "srb2") != 0)
		CONS_Alert(CONS_WARNING, "Invalid namespace '%s', only 'srb2' is supported.\n", tkn);

	EXPECT_TOKEN(";");

	GET_TOKEN();
	while (tkn != NULL)
	{
		IS_TOKEN("conversation");

		EXPECT_TOKEN("{");

		ParseConversation(sc);

		tkn = sc->get(sc, 0);
	}

fail:
	Tokenizer_Close(sc);
	Z_Free(text);
}

#undef GET_TOKEN
#undef IS_TOKEN
#undef CHECK_TOKEN
#undef EXPECT_TOKEN
#undef EXPECT_NUMBER

void P_LoadDialogueLumps(UINT16 wadnum)
{
	UINT16 lump = W_CheckNumForNamePwad("DIALOGUE", wadnum, 0);
	while (lump != INT16_MAX)
	{
		ParseDialogue(wadnum, lump);
		lump = W_CheckNumForNamePwad("DIALOGUE", wadnum, lump + 1);
	}
}
