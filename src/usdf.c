// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  usdf.c
/// \brief USDF-SRB2 parsing

#include "usdf.h"

#include "doomstat.h"

#include "p_dialogscript.h"
#include "m_tokenizer.h"

#include "deh_soc.h"
#include "m_writebuffer.h"
#include "z_zone.h"
#include "w_wad.h"

#include <errno.h>

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

INT32 P_ParsePromptBackColor(const char *color)
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
		if (strcmp(color, all_colors[i].name) == 0)
			return all_colors[i].id;
	}

	return -1;
}

void USDFParseError(int line, int alerttype, const char *format, ...)
{
	char string[8192];

	va_list argptr;
	va_start(argptr, format);
	vsnprintf(string, sizeof string, format, argptr);
	va_end(argptr);

	if (line >= 1)
		CONS_Alert(alerttype, "While parsing DIALOGUE, line %d: %s\n", line, string);
	else
		CONS_Alert(alerttype, "%s\n", string);
}

#define GET_TOKEN() \
	tkn = sc->get(sc, 0); \
	if (!tkn) \
	{ \
		USDFParseError(sc->line, CONS_ERROR, "Unexpected EOF"); \
		goto fail; \
	}

#define IS_TOKEN(expected) \
	if (!tkn || stricmp(tkn, expected) != 0) \
	{ \
		if (tkn) \
			USDFParseError(sc->line, CONS_ERROR, "Expected '%s', got '%s'", expected, tkn); \
		else \
			USDFParseError(sc->line, CONS_ERROR, "Expected '%s', got EOF", expected); \
		goto fail; \
	}

#define EXPECT_TOKEN(expected) \
	GET_TOKEN(); \
	IS_TOKEN(expected)

#define CHECK_TOKEN(check) (stricmp(tkn, check) == 0)

#define EXPECT_NUMBER(what) \
	int num = 0; \
	if (!M_StringToNumber(tkn, &num)) \
	{ \
		USDFParseError(sc->line, CONS_ERROR, "In " what ": expected a number, got '%s'", tkn); \
		goto fail; \
	}

enum {
	PARSE_STATUS_OK,
	PARSE_STATUS_FAIL,
	PARSE_STATUS_EOF
};

static int ExitBlock(tokenizer_t *sc, int bracket)
{
	const char *tkn = sc->token[0];
	if (!tkn)
		return PARSE_STATUS_EOF;
	else if (strcmp(tkn, "}") == 0)
		return PARSE_STATUS_FAIL;

	INT32 curbracket = bracket;

	while (true)
	{
		if (strcmp(tkn, "{") == 0)
			curbracket++;
		else if (strcmp(tkn, "}") == 0)
		{
			curbracket--;
			if (curbracket < bracket)
				break;
		}

		tkn = sc->get(sc, 0);
		if (!tkn)
			return PARSE_STATUS_EOF;
	}

	return PARSE_STATUS_FAIL;
}

static int SkipBlock(tokenizer_t *sc)
{
	const char *tkn = sc->token[0];
	if (!tkn)
		return PARSE_STATUS_EOF;

	int bracket = 0;

	while (true)
	{
		if (strcmp(tkn, "{") == 0)
			bracket++;
		else if (strcmp(tkn, "}") == 0)
		{
			bracket--;
			if (bracket <= 0)
				break;
		}

		tkn = sc->get(sc, 0);
		if (!tkn)
			return PARSE_STATUS_EOF;
	}

	return PARSE_STATUS_FAIL;
}

#define BUFWRITE(writechr) M_BufferWrite(&buf, (UINT8)(writechr))

static char *EscapeStringChars(const char *string, int tokenizer_line)
{
	writebuffer_t buf;

	M_BufferInit(&buf);

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
			case '{':
				BUFWRITE('\\');
				BUFWRITE('{');
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
						/* FALLTHRU */
					case 2:
						BUFWRITE(out & 0xFF);
						break;
					default:
						USDFParseError(tokenizer_line, CONS_WARNING, "In string: escape sequence has wrong size");
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
						USDFParseError(tokenizer_line, CONS_WARNING, "In string: escape sequence is too large");
						goto fail;
					}

					BUFWRITE((char)out);
				}
				else
					USDFParseError(tokenizer_line, CONS_WARNING, "In string: unknown escape sequence '\\%c'", chr);
				break;
			}
		}
		else
			BUFWRITE(chr);
	}

	return (char*)buf.data;

fail:
	M_BufferFree(&buf);

	return NULL;
}

#undef BUFWRITE

#define WRITE_TEXTCHAR(writechr) M_BufferWrite(&buf, (UINT8)(writechr))

static char *ParseText(char *text, size_t *text_length, boolean parse_scripts, int tokenizer_line)
{
	char *input = text;

	writebuffer_t buf;

	M_BufferInit(&buf);

	while (*input)
	{
		unsigned char chr = *input;

		if (chr == '\\')
		{
			unsigned char nextchar = input[1];
			if (nextchar == '{')
			{
				WRITE_TEXTCHAR(nextchar);
				input++;
			}
			else
				WRITE_TEXTCHAR(chr);
		}
		else if (chr == '{')
		{
			input++;

			if (parse_scripts)
				P_ParseDialogScriptCommand(&input, &buf, tokenizer_line);
			else
				P_ParseDialogNonScriptCommand(&input, &buf, tokenizer_line);
		}
		else if (chr == 0xFF)
			; // Just ignore it
		else
			WRITE_TEXTCHAR(chr);

		input++;
	}

	*text_length = buf.pos;

	return (char*)buf.data;
}

char *P_ConvertSOCPageDialog(char *text, size_t *text_length)
{
	char *input = text;

	writebuffer_t buf;

	M_BufferInit(&buf);

	while (*input)
	{
		unsigned char chr = *input;

		if (!P_WriteDialogScriptForCutsceneTextCode((UINT8)chr, &buf))
		{
			if (chr != 0xFF)
				WRITE_TEXTCHAR(chr);
		}

		input++;
	}

	WRITE_TEXTCHAR('\0');

	*text_length = buf.pos;

	return (char*)buf.data;
}

#undef WRITE_TEXTCHAR

#define IGNORE_FIELD() { \
	GET_TOKEN(); \
	if (CHECK_TOKEN("{")) { \
		if (SkipBlock(sc) == PARSE_STATUS_EOF) \
			return PARSE_STATUS_EOF; \
	} else { \
		if (CHECK_TOKEN("=")) {\
			GET_TOKEN(); \
		} \
	} \
}

static int ParseChoice(textpage_t *page, tokenizer_t *sc, int bracket)
{
	if (page->numchoices == MAX_PROMPT_CHOICES)
		return PARSE_STATUS_FAIL;

	page->numchoices++;
	page->choices = Z_Realloc(page->choices, sizeof(promptchoice_t) * page->numchoices, PU_STATIC, NULL);

	promptchoice_t *choice = &page->choices[page->numchoices - 1];

	INT32 choiceid = page->numchoices;

	const char *tkn;
	writebuffer_t buf;

	GET_TOKEN();

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("text"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			M_BufferInit(&buf);

			size_t total_length = 0;

			Z_Free(choice->text);
			choice->text = NULL;

			while (true)
			{
				char *escaped = EscapeStringChars(tkn, sc->line);
				if (escaped)
				{
					size_t parsed_length = 0;
					char *parsed = ParseText(escaped, &parsed_length, false, sc->line);

					M_BufferMemWrite(&buf, (UINT8 *)parsed, parsed_length);
					total_length += parsed_length;

					Z_Free(escaped);
				}

				EXPECT_TOKEN("\"");

				GET_TOKEN();

				if (CHECK_TOKEN(";"))
					break;
				else
				{
					IS_TOKEN("\"");
					GET_TOKEN();
				}
			}

			choice->text = Z_Realloc(buf.data, total_length + 1, PU_STATIC, NULL);
			choice->text[total_length] = '\0';

			buf.data = NULL;
		}
		else if (CHECK_TOKEN("nextpage"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("\""))
			{
				GET_TOKEN();

				Z_Free(choice->nextpagename);
				choice->nextpage = 0;
				choice->nextpagename = Z_StrDup(tkn);

				EXPECT_TOKEN("\"");
			}
			else
			{
				EXPECT_NUMBER("choice 'nextpage'");

				Z_Free(choice->nextpagename);

				choice->nextpage = num;
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextconversation"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("\""))
			{
				GET_TOKEN();

				Z_Free(choice->nextpromptname);
				choice->nextprompt = 0;
				choice->nextpromptname = Z_StrDup(tkn);

				EXPECT_TOKEN("\"");
			}
			else
			{
				EXPECT_NUMBER("choice 'nextconversation'");

				Z_Free(choice->nextpromptname);

				choice->nextprompt = num;
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nexttag"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			strlcpy(choice->nexttag, tkn, sizeof(choice->nexttag));

			EXPECT_TOKEN("\"");
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
			USDFParseError(sc->line, CONS_WARNING, "In choice: Unknown token '%s'", tkn);
			IGNORE_FIELD();
		}

		GET_TOKEN();
	}

	return PARSE_STATUS_OK;

fail:
	return ExitBlock(sc, bracket);
}

static int ParsePicture(textpage_t *page, tokenizer_t *sc, int bracket)
{
	if (page->numpics == MAX_PROMPT_PICS)
		return PARSE_STATUS_FAIL;

	page->numpics++;
	page->pics = Z_Realloc(page->pics, sizeof(cutscene_pic_t) * page->numpics, PU_STATIC, NULL);

	cutscene_pic_t *pic = &page->pics[page->numpics - 1];

	INT32 picid = page->numpics;

	const char *tkn;

	GET_TOKEN();

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("name"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			strlcpy(pic->name, tkn, sizeof(pic->name));

			EXPECT_TOKEN("\"");
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
			USDFParseError(sc->line, CONS_WARNING, "In picture: Unknown token '%s'", tkn);
			IGNORE_FIELD();
		}

		GET_TOKEN();
	}

	return PARSE_STATUS_OK;

fail:
	return ExitBlock(sc, bracket);
}

static int ParsePage(textprompt_t *prompt, tokenizer_t *sc, int bracket)
{
	if (prompt->numpages == MAX_PAGES)
		return PARSE_STATUS_FAIL;

	textpage_t *page = &prompt->page[prompt->numpages];
	prompt->numpages++;

	P_InitTextPromptPage(page);

	const char *tkn;
	writebuffer_t buf;

	GET_TOKEN();

	while (!CHECK_TOKEN("}"))
	{
		if (CHECK_TOKEN("pagename"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			Z_Free(page->pagename);

			page->pagename = Z_StrDup(tkn);

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("name"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			M_BufferInit(&buf);

			size_t total_length = 0;

			Z_Free(page->name);
			page->name = NULL;

			while (true)
			{
				char *escaped = EscapeStringChars(tkn, sc->line);
				if (escaped)
				{
					size_t parsed_length = 0;
					char *parsed = ParseText(escaped, &parsed_length, false, sc->line);

					M_BufferMemWrite(&buf, (UINT8 *)parsed, parsed_length);
					total_length += parsed_length;

					Z_Free(escaped);
				}

				EXPECT_TOKEN("\"");

				GET_TOKEN();

				if (CHECK_TOKEN(";"))
					break;
				else
				{
					IS_TOKEN("\"");
					GET_TOKEN();
				}
			}

			page->name = Z_Realloc(buf.data, total_length + 1, PU_STATIC, NULL);
			page->name[total_length] = '\0';

			buf.data = NULL;
		}
		else if (CHECK_TOKEN("dialog"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			M_BufferInit(&buf);

			size_t total_length = 0;

			Z_Free(page->text);
			page->text = NULL;
			page->textlength = 0;

			while (true)
			{
				char *escaped = EscapeStringChars(tkn, sc->line);
				if (escaped)
				{
					size_t parsed_length = 0;
					char *parsed = ParseText(escaped, &parsed_length, true, sc->line);

					M_BufferMemWrite(&buf, (UINT8 *)parsed, parsed_length);
					total_length += parsed_length;

					Z_Free(escaped);
				}

				EXPECT_TOKEN("\"");

				GET_TOKEN();

				if (CHECK_TOKEN(";"))
					break;
				else
				{
					IS_TOKEN("\"");
					GET_TOKEN();
				}
			}

			page->text = Z_Realloc(buf.data, total_length + 1, PU_STATIC, NULL);
			page->text[total_length] = '\0';
			page->textlength = total_length;

			buf.data = NULL;
		}
		else if (CHECK_TOKEN("icon"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			strlcpy(page->iconname, tkn, sizeof(page->iconname));

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("tag"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			strlcpy(page->tag, tkn, sizeof(page->tag));

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("textsound"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			page->textsfx = get_sfx(tkn);

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextpage"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("\""))
			{
				GET_TOKEN();

				Z_Free(page->nextpagename);
				page->nextpage = 0;
				page->nextpagename = Z_StrDup(tkn);

				EXPECT_TOKEN("\"");
			}
			else
			{
				EXPECT_NUMBER("page 'nextpage'");

				Z_Free(page->nextpagename);

				page->nextpage = num;
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nextconversation"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			if (CHECK_TOKEN("\""))
			{
				GET_TOKEN();

				Z_Free(page->nextpromptname);
				page->nextprompt = 0;
				page->nextpromptname = Z_StrDup(tkn);

				EXPECT_TOKEN("\"");
			}
			else
			{
				EXPECT_NUMBER("page 'nextconversation'");

				Z_Free(page->nextpromptname);

				page->nextprompt = num;
			}

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("nexttag"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			strlcpy(page->nexttag, tkn, sizeof(page->nexttag));

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("duration"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'duration'");

			if (num < 0)
				num = 0;

			page->timetonext = num;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("textspeed"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'textspeed'");

			if (num < 1)
				num = 1;

			page->textspeed = num - 1;

			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("textlines"))
		{
			EXPECT_TOKEN("=");

			GET_TOKEN();

			EXPECT_NUMBER("page 'textlines'");

			if (num < 1)
				num = 1;

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
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			if (CHECK_TOKEN("show"))
				page->hidehud = 0;
			else if (CHECK_TOKEN("hide"))
				page->hidehud = 1;
			else if (CHECK_TOKEN("hideall"))
				page->hidehud = 2;

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("backcolor"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			page->backcolor = P_ParsePromptBackColor(tkn);

			EXPECT_TOKEN("\"");
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
				if (ParseChoice(page, sc, bracket + 1) != PARSE_STATUS_OK)
				{
					page->numchoices--;
					goto fail;
				}

				if (page->numchoices == MAX_PROMPT_CHOICES)
					USDFParseError(sc->line, CONS_WARNING, "Conversation page exceeded max amount of choices; ignoring any more choices");
			}
		}
		else if (CHECK_TOKEN("alignchoices"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			if (CHECK_TOKEN("left"))
				page->choicesleftside = true;
			else if (CHECK_TOKEN("right"))
				page->choicesleftside = false;

			EXPECT_TOKEN("\"");
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
				if (ParsePicture(page, sc, bracket + 1) != PARSE_STATUS_OK)
				{
					page->numpics--;
					goto fail;
				}

				if (page->numpics == MAX_PROMPT_PICS)
					USDFParseError(sc->line, CONS_WARNING, "Conversation page exceeded max amount of pictures; ignoring any more pictures");
			}
		}
		else if (CHECK_TOKEN("picturesequence"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			if (CHECK_TOKEN("persist"))
				page->picmode = 0;
			else if (CHECK_TOKEN("loop"))
				page->picmode = 1;
			else if (CHECK_TOKEN("hide"))
				page->picmode = 2;

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("music"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			strlcpy(page->musswitch, tkn, sizeof(page->musswitch));

			EXPECT_TOKEN("\"");
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
		else
		{
			USDFParseError(sc->line, CONS_WARNING, "In page: Unknown token '%s'", tkn);
			IGNORE_FIELD();
		}

		GET_TOKEN();
	}

	return PARSE_STATUS_OK;

fail:
	M_BufferFree(&buf);

	return ExitBlock(sc, bracket);
}

#undef IGNORE_FIELD

static int ParseConversation(tokenizer_t *sc, int bracket, int tokenizer_line)
{
	int parse_status = PARSE_STATUS_OK;

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

			if (CHECK_TOKEN("\""))
			{
				GET_TOKEN();

				name = Z_StrDup(tkn);

				EXPECT_TOKEN("\"");
			}
			else
			{
				EXPECT_NUMBER("conversation 'id'");

				id = num;
			}

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
				if (ParsePage(prompt, sc, bracket + 1) != PARSE_STATUS_OK)
				{
					prompt->numpages--;
					goto fail;
				}

				if (prompt->numpages == MAX_PAGES)
					USDFParseError(sc->line, CONS_WARNING, "Conversation exceeded max amount of pages; ignoring any more pages");
			}
		}
		else
		{
			USDFParseError(sc->line, CONS_WARNING, "In conversation: Unknown token '%s'", tkn);
			GET_TOKEN();
			if (CHECK_TOKEN("{"))
			{
				if (SkipBlock(sc) == PARSE_STATUS_EOF)
				{
					parse_status = PARSE_STATUS_EOF;
					goto ignore;
				}
			}
			else
			{
				EXPECT_TOKEN("=");
				GET_TOKEN();
				EXPECT_TOKEN(";");
			}
		}

		GET_TOKEN();
	}

	// Now do some verifications
	if (!prompt->numpages)
	{
		USDFParseError(tokenizer_line, CONS_WARNING, "Conversation has no pages");
		goto ignore;
	}

	if (name)
	{
		id = P_FindTextPromptSlot(name);

		if (id < 0)
		{
			CONS_Alert(CONS_WARNING, "No more free conversation slots\n");
			goto ignore;
		}

		prompt->name = Z_StrDup(name);

		P_FreeTextPrompt(textprompts[id]);

		textprompts[id] = prompt;

		return parse_status;
	}
	else if (id)
	{
		if (id <= 0 || id > MAX_PROMPTS)
		{
			CONS_Alert(CONS_WARNING, "Conversation ID %d out of range (1 - %d)\n", id, MAX_PROMPTS);
			goto ignore;
		}

		--id;

		P_FreeTextPrompt(textprompts[id]);

		textprompts[id] = prompt;

		return parse_status;
	}
	else
	{
		CONS_Alert(CONS_WARNING, "Conversation has missing ID\n");
		goto ignore;
	}

fail:
	parse_status = ExitBlock(sc, bracket);

ignore:
	Z_Free(prompt);
	Z_Free(name);

	return parse_status;
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
	if (!CHECK_TOKEN("namespace"))
	{
		CONS_Alert(CONS_ERROR, "No namespace at beginning of DIALOGUE text!\n");
		goto fail;
	}

	EXPECT_TOKEN("=");

	// Check if namespace is valid.
	EXPECT_TOKEN("\"");
	GET_TOKEN();
	if (strcmp(tkn, "srb2") != 0)
		CONS_Alert(CONS_WARNING, "Invalid namespace '%s', only 'srb2' is supported.\n", tkn);
	EXPECT_TOKEN("\"");
	EXPECT_TOKEN(";");

	GET_TOKEN();

	while (tkn != NULL)
	{
		IS_TOKEN("conversation");

		int tokenizer_line = sc->line;

		EXPECT_TOKEN("{");

		ParseConversation(sc, 1, tokenizer_line);

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
		char *name = W_GetFullLumpPathName(wadnum, lump);
		CONS_Printf(M_GetText("Loading DIALOGUE from %s\n"), name);
		free(name);

		ParseDialogue(wadnum, lump);

		lump = W_CheckNumForNamePwad("DIALOGUE", wadnum, lump + 1);
	}
}
