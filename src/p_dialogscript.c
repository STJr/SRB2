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
#include "m_misc.h" // M_BufferMemWrite
#include "z_zone.h"
#include "w_wad.h"

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

boolean P_DialogPreprocessOpcode(dialog_t *dialog, UINT8 **cptr, UINT8 **buffer, size_t *buffer_pos, size_t *buffer_capacity)
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
		case TP_OP_CHARNAME:
		case TP_OP_PLAYERNAME:
			// Nothing else to read
			break;
		}
	}

	return code - baseptr;
}

#undef READ_NUM
