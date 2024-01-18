// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_dialogscript.c
/// \brief Text prompt script interpreter

#include "p_dialogscript.h"

#include "d_player.h" // player_t
#include "r_skins.h" // skins
#include "g_game.h" // player_names
#include "m_misc.h"
#include "m_writebuffer.h"
#include "usdf.h"
#include "z_zone.h"
#include "w_wad.h"

static boolean GetCommand(char **command_start, char **command_end, char *start)
{
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

	*command_start = start;
	*command_end = end;

	return true;
}

static boolean MatchCommand(const char *match, size_t matchlen, char *tkn, size_t tknlen, char **input)
{
	if (tknlen != matchlen)
		return false;

	char *cmp = tkn;
	char *cmpEnd = cmp + tknlen;
	while (cmp < cmpEnd)
	{
		if (toupper(*cmp) != *match)
			return false;
		cmp++;
		match++;
	}

	while (isspace(*cmpEnd))
		cmpEnd++;
	*input = cmpEnd;
	return true;
}

static char *GetCommandParam(char **input)
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

static void GetCommandEnd(char **input)
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

#define WRITE_CHAR(writechr) M_BufferWrite(bufptr, (UINT8)(writechr))
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
	M_BufferMemWrite(bufptr, (UINT8*)str, maxlen); \
}

#define EXPECTED_NUMBER(which) USDFParseError(tokenizer_line, CONS_WARNING, "Expected integer for command '%s'; got '%s' instead", which, param)
#define EXPECTED_PARAM(which) USDFParseError(tokenizer_line, CONS_WARNING, "Expected parameter for command '%s'", which)
#define IS_COMMAND(match) MatchCommand(match, sizeof(match) - 1, command_start, cmd_len, input)

static boolean CheckIfNonScriptCommand(char *command_start, size_t cmd_len, char **input, writebuffer_t *bufptr, int tokenizer_line);

void P_ParseDialogScriptCommand(char **input, writebuffer_t *bufptr, int tokenizer_line)
{
	char *command_start = NULL;
	char *command_end = NULL;

	if (!GetCommand(&command_start, &command_end, *input))
	{
		GetCommandEnd(input);
		return;
	}

	size_t cmd_len = command_end - command_start;

	if (IS_COMMAND("DELAY"))
	{
		int delay_frames = 12;

		char *param = GetCommandParam(input);
		if (param)
		{
			if (!M_StringToNumber(param, &delay_frames))
				EXPECTED_NUMBER("DELAY");

			Z_Free(param);
		}

		if (delay_frames > 0)
		{
			WRITE_OP(TP_OP_DELAY);
			WRITE_NUM(delay_frames);
		}
	}
	else if (IS_COMMAND("PAUSE"))
	{
		int pause_frames = 12;

		char *param = GetCommandParam(input);
		if (param)
		{
			if (!M_StringToNumber(param, &pause_frames))
				EXPECTED_NUMBER("PAUSE");

			Z_Free(param);
		}

		if (pause_frames > 0)
		{
			WRITE_OP(TP_OP_PAUSE);
			WRITE_NUM(pause_frames);
		}
	}
	else if (IS_COMMAND("SPEED"))
	{
		char *param = GetCommandParam(input);
		if (param)
		{
			int speed = 1;

			if (M_StringToNumber(param, &speed))
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
	else if (IS_COMMAND("NAME"))
	{
		char *param = GetCommandParam(input);
		if (param)
		{
			WRITE_OP(TP_OP_NAME);
			WRITE_STRING(param);

			Z_Free(param);
		}
		else
			EXPECTED_PARAM("NAME");
	}
	else if (IS_COMMAND("ICON"))
	{
		char *param = GetCommandParam(input);
		if (param)
		{
			WRITE_OP(TP_OP_ICON);
			WRITE_STRING(param);

			Z_Free(param);
		}
		else
			EXPECTED_PARAM("ICON");
	}
	else if (IS_COMMAND("CHARNAME"))
	{
		WRITE_OP(TP_OP_CHARNAME);
	}
	else if (IS_COMMAND("PLAYERNAME"))
	{
		WRITE_OP(TP_OP_PLAYERNAME);
	}
	else if (IS_COMMAND("NEXTPAGE"))
	{
		WRITE_OP(TP_OP_NEXTPAGE);
	}
	else if (IS_COMMAND("WAIT"))
	{
		WRITE_OP(TP_OP_WAIT);
	}
	else if (!CheckIfNonScriptCommand(command_start, cmd_len, input, bufptr, tokenizer_line))
		USDFParseError(tokenizer_line, CONS_WARNING, "Unknown command %.*s", (int)cmd_len, command_start);

	GetCommandEnd(input);
}

void P_ParseDialogNonScriptCommand(char **input, writebuffer_t *bufptr, int tokenizer_line)
{
	char *command_start = NULL;
	char *command_end = NULL;

	if (!GetCommand(&command_start, &command_end, *input))
	{
		GetCommandEnd(input);
		return;
	}

	size_t cmd_len = command_end - command_start;

	if (!CheckIfNonScriptCommand(command_start, cmd_len, input, bufptr, tokenizer_line))
		USDFParseError(tokenizer_line, CONS_WARNING, "Unknown command %.*s", (int)cmd_len, command_start);

	GetCommandEnd(input);
}

static int ParsePromptTextColor(const char *color)
{
	const char *text_colors[] = {
		"white",
		"magenta",
		"yellow",
		"green",
		"blue",
		"red",
		"gray",
		"orange",
		"sky",
		"purple",
		"aqua",
		"peridot",
		"azure",
		"brown",
		"rosy",
		"invert"
	};

	for (size_t i = 0; i < sizeof(text_colors) / sizeof(text_colors[0]); i++)
	{
		if (stricmp(color, text_colors[i]) == 0)
			return (int)(i + 128);
	}

	return -1;
}

static boolean CheckIfNonScriptCommand(char *command_start, size_t cmd_len, char **input, writebuffer_t *bufptr, int tokenizer_line)
{
	if (IS_COMMAND("COLOR"))
	{
		char *param = GetCommandParam(input);
		if (param)
		{
			int color = ParsePromptTextColor(param);
			if (color == -1)
				USDFParseError(tokenizer_line, CONS_WARNING, "Unknown text color '%s'", param);
			else
				WRITE_CHAR((UINT8)color);

			Z_Free(param);
		}
		else
			EXPECTED_PARAM("COLOR");
	}
	else
		return false;

	return true;
}

#undef IS_COMMAND

boolean P_WriteDialogScriptForCutsceneTextCode(UINT8 chr, writebuffer_t *bufptr)
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

#define READ_NUM(where) { \
	int temp = 0; \
	temp |= (*code) << 24; code++; \
	temp |= (*code) << 16; code++; \
	temp |= (*code) << 8; code++; \
	temp |= (*code); code++; \
	where = temp; \
}

static char *ReadString(const UINT8 **code)
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

const UINT8 *P_DialogRunOpcode(const UINT8 *code, dialog_t *dialog, textwriter_t *writer)
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
		case TP_OP_PAUSE:
			READ_NUM(num);
			writer->textcount = num;
			writer->numtowrite = 0;
			writer->paused = true;
			break;
		case TP_OP_NEXTPAGE:
			dialog->gotonext = true;
			break;
		case TP_OP_WAIT:
			dialog->paused = true;
			writer->numtowrite = 0;
			break;
		case TP_OP_NAME: {
			char *speaker = ReadString(&code);
			if (speaker)
			{
				P_SetDialogSpeaker(dialog, speaker);
				Z_Free(speaker);
			}
			break;
		}
		case TP_OP_ICON: {
			char *icon = ReadString(&code);
			if (icon)
			{
				P_SetDialogIcon(dialog, icon);
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

boolean P_DialogPreprocessOpcode(dialog_t *dialog, UINT8 **cptr, writebuffer_t *buf)
{
	UINT8 *code = *cptr;
	if (*code++ != TP_OP_CONTROL)
		return false;

	player_t *player = dialog->player;

	switch (*code)
	{
		case TP_OP_CHARNAME: {
			char charname[256];
			strlcpy(charname, skins[player->skin]->realname, sizeof(charname));
			M_BufferMemWrite(buf, (UINT8 *)charname, strlen(charname));
			break;
		}
		case TP_OP_PLAYERNAME: {
			char playername[256];
			if (netgame)
				strlcpy(playername, player_names[player-players], sizeof(playername));
			else
				strlcpy(playername, skins[player->skin]->realname, sizeof(playername));
			M_BufferMemWrite(buf, (UINT8 *)playername, strlen(playername));
			break;
		}
		default:
			return false;
	}

	*cptr = code;

	return true;
}

int P_DialogSkipOpcode(const UINT8 *code)
{
	const UINT8 *baseptr = code;

	switch (*code++)
	{
		case TP_OP_SPEED:
		case TP_OP_DELAY:
		case TP_OP_PAUSE:
			code += 4;
			break;
		case TP_OP_NAME:
		case TP_OP_ICON: {
			UINT8 n = *code++;
			code += n;
			break;
		case TP_OP_NEXTPAGE:
		case TP_OP_WAIT:
		case TP_OP_CHARNAME:
		case TP_OP_PLAYERNAME:
			// Nothing else to read
			break;
		}
	}

	return code - baseptr;
}

#undef READ_NUM
