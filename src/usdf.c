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
#include "m_misc.h"
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
		if (strcmp(word, all_colors[i].name) == 0)
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
	if (!tkn || stricmp(tkn, expected) != 0) \
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

#define CHECK_TOKEN(check) (stricmp(tkn, check) == 0)

#define EXPECT_NUMBER(what) \
	int num = 0; \
	if (!StringToNumber(tkn, &num)) \
	{ \
		CONS_Alert(CONS_ERROR, "Error parsing " what ": expected a number, got '%s'\n", tkn); \
		goto fail; \
	}

enum {
	PARSE_STATUS_OK,
	PARSE_STATUS_FAIL,
	PARSE_STATUS_EOF
};

static int ExitBlock(tokenizer_t *sc, INT32 bracket)
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

	INT32 bracket = 0;

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

#define IGNORE_FIELD() { \
	GET_TOKEN(); \
	if (CHECK_TOKEN("{")) { \
		if (SkipBlock(sc) == PARSE_STATUS_EOF) \
			return PARSE_STATUS_EOF; \
	} else { \
		if (CHECK_TOKEN("=")) {\
			GET_TOKEN(); \
		} \
		if (CHECK_TOKEN(";")) { \
			GET_TOKEN(); \
		} \
	} \
}

static int ParseChoice(textpage_t *page, tokenizer_t *sc, INT32 bracket)
{
	const char *tkn;

	if (page->numchoices == MAX_PROMPT_CHOICES)
		return PARSE_STATUS_FAIL;

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
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			Z_Free(choice->text);

			choice->text = Z_StrDup(tkn);

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
			CONS_Alert(CONS_WARNING, "While parsing choice: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		GET_TOKEN();
	}

	return PARSE_STATUS_OK;

fail:
	return ExitBlock(sc, bracket);
}

static int ParsePicture(textpage_t *page, tokenizer_t *sc, INT32 bracket)
{
	const char *tkn;

	if (page->numpics == MAX_PROMPT_PICS)
		return PARSE_STATUS_FAIL;

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
			CONS_Alert(CONS_WARNING, "While parsing pic: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		GET_TOKEN();
	}

	return PARSE_STATUS_OK;

fail:
	return ExitBlock(sc, bracket);
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
	if (MatchControlCode("DELAY", input))
	{
		int delay_frames = 12;

		char *param = GetControlCodeParam(input);
		if (param)
		{
			if (!StringToNumber(param, &delay_frames))
				EXPECTED_NUMBER("DELAY");

			Z_Free(param);
		}

		if (delay_frames > 0)
		{
			WRITE_OP(TP_OP_DELAY);
			WRITE_NUM(delay_frames);
		}
	}
	else if (MatchControlCode("PAUSE", input))
	{
		int pause_frames = 12;

		char *param = GetControlCodeParam(input);
		if (param)
		{
			if (!StringToNumber(param, &pause_frames))
				EXPECTED_NUMBER("PAUSE");

			Z_Free(param);
		}

		if (pause_frames > 0)
		{
			WRITE_OP(TP_OP_PAUSE);
			WRITE_NUM(pause_frames);
		}
	}
	else if (MatchControlCode("SPEED", input))
	{
		char *param = GetControlCodeParam(input);
		if (param)
		{
			int speed = 1;

			if (StringToNumber(param, &speed))
			{
				if (speed < 0)
					speed = 0;

				speed--;
			}
			else
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
	if (chr >= 0xA0 && chr <= 0xAF)
	{
		WRITE_OP(TP_OP_SPEED);
		WRITE_NUM(chr - 0xA0);
		return true;
	}
	else if (chr >= 0xB0 && chr <= (0xB0+TICRATE-1))
	{
		WRITE_OP(TP_OP_DELAY);
		WRITE_NUM(chr - 0xAF);
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

static int ParsePage(textprompt_t *prompt, tokenizer_t *sc, INT32 bracket)
{
	const char *tkn;

	if (prompt->numpages == MAX_PAGES)
		return PARSE_STATUS_FAIL;

	textpage_t *page = &prompt->page[prompt->numpages];

	P_InitTextPromptPage(page);

	prompt->numpages++;

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

			strlcpy(page->name, tkn, sizeof(page->name));

			EXPECT_TOKEN("\"");
			EXPECT_TOKEN(";");
		}
		else if (CHECK_TOKEN("dialog"))
		{
			EXPECT_TOKEN("=");
			EXPECT_TOKEN("\"");

			GET_TOKEN();

			UINT8 *buffer = NULL;
			size_t buffer_pos = 0;
			size_t buffer_capacity = 0;

			size_t total_text_length = 0;

			Z_Free(page->text);
			page->textlength = 0;

			while (true)
			{
				char *escaped = EscapeStringChars(tkn);
				if (escaped)
				{
					size_t parsed_length = 0;
					char *parsed = ParsePageDialog(escaped, &parsed_length);

					M_BufferMemWrite((UINT8 *)parsed, parsed_length, &buffer, &buffer_pos, &buffer_capacity);
					total_text_length += parsed_length;

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

			page->text = Z_Malloc(total_text_length + 1, PU_STATIC, NULL);
			memcpy(page->text, buffer, total_text_length);
			page->text[total_text_length] = '\0';
			page->textlength = total_text_length;

			Z_Free(buffer);
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
					CONS_Alert(CONS_WARNING, "Conversation page exceeded max amount of choices; ignoring any more choices\n");
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
					CONS_Alert(CONS_WARNING, "Conversation page exceeded max amount of pictures; ignoring any more pictures\n");
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
			CONS_Alert(CONS_WARNING, "While parsing page: Unknown token '%s'\n", tkn);
			IGNORE_FIELD();
		}

		GET_TOKEN();
	}

	return PARSE_STATUS_OK;

fail:
	return ExitBlock(sc, bracket);
}

#undef IGNORE_FIELD

static int ParseConversation(tokenizer_t *sc, INT32 bracket)
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
					CONS_Alert(CONS_WARNING, "Conversation exceeded max amount of pages; ignoring any more pages\n");
			}
		}
		else
		{
			CONS_Alert(CONS_WARNING, "While parsing conversation: Unknown token '%s'\n", tkn);
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
		CONS_Alert(CONS_WARNING, "Conversation has no pages\n");
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

		EXPECT_TOKEN("{");

		ParseConversation(sc, 1);

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
