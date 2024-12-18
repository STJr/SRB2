// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  net_command.c
/// \brief Net command handling

#include "net_command.h"
#include "tic_command.h"
#include "gamestate.h"
#include "server_connection.h"
#include "d_clisrv.h"
#include "i_net.h"
#include "../byteptr.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../doomtype.h"

typedef struct textcmdbuf_s textcmdbuf_t;

struct textcmdbuf_s
{
	textcmdbuf_t *next;
	UINT8 cmd[MAXTEXTCMD];
};

static textcmdbuf_t *textcmdbuf;
static textcmdbuf_t *textcmdbuf2;
textcmdtic_t *textcmds[TEXTCMD_HASH_SIZE] = {NULL};
UINT8 localtextcmd[MAXTEXTCMD];
UINT8 localtextcmd2[MAXTEXTCMD]; // splitscreen
static void (*listnetxcmd[MAXNETXCMD])(UINT8 **p, INT32 playernum);

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum))
{
#ifdef PARANOIA
	if (id >= MAXNETXCMD)
		I_Error("Command id %d too big", id);
	if (listnetxcmd[id] != 0)
		I_Error("Command id %d already used", id);
#endif
	listnetxcmd[id] = cmd_f;
}

static void WriteNetXCmd(UINT8 *cmd, netxcmd_t id, const void *param, size_t nparam)
{
	cmd[0]++;
	cmd[cmd[0]] = (UINT8)id;
	if (param && nparam)
	{
		M_Memcpy(&cmd[cmd[0]+1], param, nparam);
		cmd[0] = (UINT8)(cmd[0] + (UINT8)nparam);
	}
}

void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam)
{
	if (localtextcmd[0]+2+nparam > MAXTEXTCMD)
	{
		textcmdbuf_t *buf = textcmdbuf;

		if (2+nparam > MAXTEXTCMD)
		{
			CONS_Alert(CONS_ERROR, M_GetText("packet too large to fit NetXCmd, cannot add netcmd %d! (size: %s, max: %d)\n"), id, sizeu1(2+nparam), MAXTEXTCMD);
			return;
		}

		// for future reference: if (cv_debug) != debug disabled.
		CONS_Alert(CONS_NOTICE, M_GetText("NetXCmd buffer full, delaying netcmd %d... (size: %d, needed: %s)\n"), id, localtextcmd[0], sizeu1(nparam));
		if (buf == NULL)
		{
			textcmdbuf = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			textcmdbuf->cmd[0] = 0;
			textcmdbuf->next = NULL;
			WriteNetXCmd(textcmdbuf->cmd, id, param, nparam);
			return;
		}

		while (buf->next != NULL)
			buf = buf->next;

		if (buf->cmd[0]+2+nparam > MAXTEXTCMD)
		{
			buf->next = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			buf->next->cmd[0] = 0;
			buf->next->next = NULL;
			WriteNetXCmd(buf->next->cmd, id, param, nparam);
		}
		else
		{
			WriteNetXCmd(buf->cmd, id, param, nparam);
		}
		return;
	}
	WriteNetXCmd(localtextcmd, id, param, nparam);
}

// splitscreen player
void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam)
{
	if (localtextcmd2[0]+2+nparam > MAXTEXTCMD)
	{
		textcmdbuf_t *buf = textcmdbuf2;

		if (2+nparam > MAXTEXTCMD)
		{
			CONS_Alert(CONS_ERROR, M_GetText("packet too large to fit NetXCmd, cannot add netcmd %d! (size: %s, max: %d)\n"), id, sizeu1(2+nparam), MAXTEXTCMD);
			return;
		}

		// for future reference: if (cv_debug) != debug disabled.
		CONS_Alert(CONS_NOTICE, M_GetText("NetXCmd buffer full, delaying netcmd %d... (size: %d, needed: %s)\n"), id, localtextcmd2[0], sizeu1(nparam));
		if (buf == NULL)
		{
			textcmdbuf2 = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			textcmdbuf2->cmd[0] = 0;
			textcmdbuf2->next = NULL;
			WriteNetXCmd(textcmdbuf2->cmd, id, param, nparam);
			return;
		}

		while (buf->next != NULL)
			buf = buf->next;

		if (buf->cmd[0]+2+nparam > MAXTEXTCMD)
		{
			buf->next = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			buf->next->cmd[0] = 0;
			buf->next->next = NULL;
			WriteNetXCmd(buf->next->cmd, id, param, nparam);
		}
		else
		{
			WriteNetXCmd(buf->cmd, id, param, nparam);
		}
		return;
	}
	WriteNetXCmd(localtextcmd2, id, param, nparam);
}

UINT8 GetFreeXCmdSize(void)
{
	// -1 for the size and another -1 for the ID.
	return (UINT8)(localtextcmd[0] - 2);
}

// Frees all textcmd memory for the specified tic
void D_FreeTextcmd(tic_t tic)
{
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t *textcmdtic = *tctprev;

	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}

	if (textcmdtic)
	{
		// Remove this tic from the list.
		*tctprev = textcmdtic->next;

		// Free all players.
		for (INT32 i = 0; i < TEXTCMD_HASH_SIZE; i++)
		{
			textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[i];

			while (textcmdplayer)
			{
				textcmdplayer_t *tcpnext = textcmdplayer->next;
				Z_Free(textcmdplayer);
				textcmdplayer = tcpnext;
			}
		}

		// Free this tic's own memory.
		Z_Free(textcmdtic);
	}
}

// Gets the buffer for the specified ticcmd, or NULL if there isn't one
UINT8* D_GetExistingTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	while (textcmdtic && textcmdtic->tic != tic) textcmdtic = textcmdtic->next;

	// Do we have an entry for the tic? If so, look for player.
	if (textcmdtic)
	{
		textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
		while (textcmdplayer && textcmdplayer->playernum != playernum) textcmdplayer = textcmdplayer->next;

		if (textcmdplayer) return textcmdplayer->cmd;
	}

	return NULL;
}

// Gets the buffer for the specified ticcmd, creating one if necessary
UINT8* D_GetTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer_t *textcmdplayer, **tcpprev;

	// Look for the tic.
	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}

	// If we don't have an entry for the tic, make it.
	if (!textcmdtic)
	{
		textcmdtic = *tctprev = Z_Calloc(sizeof (textcmdtic_t), PU_STATIC, NULL);
		textcmdtic->tic = tic;
	}

	tcpprev = &textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer = *tcpprev;

	// Look for the player.
	while (textcmdplayer && textcmdplayer->playernum != playernum)
	{
		tcpprev = &textcmdplayer->next;
		textcmdplayer = textcmdplayer->next;
	}

	// If we don't have an entry for the player, make it.
	if (!textcmdplayer)
	{
		textcmdplayer = *tcpprev = Z_Calloc(sizeof (textcmdplayer_t), PU_STATIC, NULL);
		textcmdplayer->playernum = playernum;
	}

	return textcmdplayer->cmd;
}

void ExtraDataTicker(void)
{
	for (INT32 i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] || i == 0)
		{
			UINT8 *bufferstart = D_GetExistingTextcmd(gametic, i);

			if (bufferstart)
			{
				UINT8 *curpos = bufferstart;
				UINT8 *bufferend = &curpos[curpos[0]+1];

				curpos++;
				while (curpos < bufferend)
				{
					if (*curpos < MAXNETXCMD && listnetxcmd[*curpos])
					{
						const UINT8 id = *curpos;
						curpos++;
						DEBFILE(va("executing x_cmd %s ply %u ", netxcmdnames[id - 1], i));
						(listnetxcmd[id])(&curpos, i);
						DEBFILE("done\n");
					}
					else
					{
						if (server)
						{
							SendKick(i, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
							DEBFILE(va("player %d kicked [gametic=%u] reason as follows:\n", i, gametic));
						}
						CONS_Alert(CONS_WARNING, M_GetText("Got unknown net command [%s]=%d (max %d)\n"), sizeu1(curpos - bufferstart), *curpos, bufferstart[0]);
						break;
					}
				}
			}
		}

	// If you are a client, you can safely forget the net commands for this tic
	// If you are the server, you need to remember them until every client has been acknowledged,
	// because if you need to resend a PT_SERVERTICS packet, you will need to put the commands in it
	if (client)
		D_FreeTextcmd(gametic);
}

// used at txtcmds received to check packetsize bound
size_t TotalTextCmdPerTic(tic_t tic)
{
	size_t total = 1; // num of textcmds in the tic (ntextcmd byte)

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 *textcmd = D_GetExistingTextcmd(tic, i);
		if ((!i || playeringame[i]) && textcmd)
			total += 2 + textcmd[0]; // "+2" for size and playernum
	}

	return total;
}

void PT_TextCmd(SINT8 node, INT32 netconsole)
{
	if (client)
		return;

	// splitscreen special
	if (netbuffer->packettype == PT_TEXTCMD2)
		netconsole = netnodes[node].player2;

	if (netconsole < 0 || netconsole >= MAXPLAYERS)
		Net_UnAcknowledgePacket(node);
	else
	{
		size_t j;
		tic_t tic = maketic;
		UINT8 *textcmd;

		// ignore if the textcmd has a reported size of zero
		// this shouldn't be sent at all
		if (!netbuffer->u.textcmd[0])
		{
			DEBFILE(va("GetPacket: Textcmd with size 0 detected! (node %u, player %d)\n",
				node, netconsole));
			Net_UnAcknowledgePacket(node);
			return;
		}

		// ignore if the textcmd size var is actually larger than it should be
		// BASEPACKETSIZE + 1 (for size) + textcmd[0] should == datalength
		if (netbuffer->u.textcmd[0] > (size_t)doomcom->datalength-BASEPACKETSIZE-1)
		{
			DEBFILE(va("GetPacket: Bad Textcmd packet size! (expected %d, actual %s, node %u, player %d)\n",
			netbuffer->u.textcmd[0], sizeu1((size_t)doomcom->datalength-BASEPACKETSIZE-1),
				node, netconsole));
			Net_UnAcknowledgePacket(node);
			return;
		}

		// check if tic that we are making isn't too large else we cannot send it :(
		// doomcom->numslots+1 "+1" since doomcom->numslots can change within this time and sent time
		j = software_MAXPACKETLENGTH
			- (netbuffer->u.textcmd[0]+2+BASESERVERTICSSIZE
			+ (doomcom->numslots+1)*sizeof(ticcmd_t));

		// search a tic that have enougth space in the ticcmd
		while ((textcmd = D_GetExistingTextcmd(tic, netconsole)),
			(TotalTextCmdPerTic(tic) > j || netbuffer->u.textcmd[0] + (textcmd ? textcmd[0] : 0) > MAXTEXTCMD)
			&& tic < firstticstosend + BACKUPTICS)
			tic++;

		if (tic >= firstticstosend + BACKUPTICS)
		{
			DEBFILE(va("GetPacket: Textcmd too long (max %s, used %s, mak %d, "
				"tosend %u, node %u, player %d)\n", sizeu1(j), sizeu2(TotalTextCmdPerTic(maketic)),
				maketic, firstticstosend, node, netconsole));
			Net_UnAcknowledgePacket(node);
			return;
		}

		// Make sure we have a buffer
		if (!textcmd) textcmd = D_GetTextcmd(tic, netconsole);

		DEBFILE(va("textcmd put in tic %u at position %d (player %d) ftts %u mk %u\n",
			tic, textcmd[0]+1, netconsole, firstticstosend, maketic));

		M_Memcpy(&textcmd[textcmd[0]+1], netbuffer->u.textcmd+1, netbuffer->u.textcmd[0]);
		textcmd[0] += (UINT8)netbuffer->u.textcmd[0];
	}
}

void SV_WriteNetCommandsForTic(tic_t tic, UINT8 **buf)
{
	UINT8 *numcmds;

	numcmds = (*buf)++;
	*numcmds = 0;
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 *cmd = D_GetExistingTextcmd(tic, i);
		INT32 size = cmd ? cmd[0] : 0;

		if ((!i || playeringame[i]) && size)
		{
			(*numcmds)++;
			WRITEUINT8(*buf, i);
			M_Memcpy(*buf, cmd, size + 1);
			*buf += size + 1;
		}
	}
}

void CL_CopyNetCommandsFromServerPacket(tic_t tic, UINT8 **buf)
{
	UINT8 numcmds = *(*buf)++;

	for (UINT32 i = 0; i < numcmds; i++)
	{
		INT32 playernum = *(*buf)++; // playernum
		size_t size = (*buf)[0]+1;

		if (tic >= gametic) // Don't copy old net commands
			M_Memcpy(D_GetTextcmd(tic, playernum), *buf, size);
		*buf += size;
	}
}

void CL_SendNetCommands(void)
{
	// Send extra data if needed
	if (localtextcmd[0])
	{
		netbuffer->packettype = PT_TEXTCMD;
		M_Memcpy(netbuffer->u.textcmd,localtextcmd, localtextcmd[0]+1);
		// All extra data have been sent
		if (HSendPacket(servernode, true, 0, localtextcmd[0]+1)) // Send can fail...
		{
			localtextcmd[0] = 0;
			if (textcmdbuf != NULL)
			{
				textcmdbuf_t *buf = textcmdbuf;
				M_Memcpy(localtextcmd, textcmdbuf->cmd, textcmdbuf->cmd[0]+1);
				textcmdbuf = textcmdbuf->next;
				Z_Free(buf);
			}
		}
	}

	// Send extra data if needed for player 2 (splitscreen)
	if (localtextcmd2[0])
	{
		netbuffer->packettype = PT_TEXTCMD2;
		M_Memcpy(netbuffer->u.textcmd, localtextcmd2, localtextcmd2[0]+1);
		// All extra data have been sent
		if (HSendPacket(servernode, true, 0, localtextcmd2[0]+1)) // Send can fail...
		{
			localtextcmd2[0] = 0;
			if (textcmdbuf2 != NULL)
			{
				textcmdbuf_t *buf = textcmdbuf2;
				M_Memcpy(localtextcmd2, textcmdbuf2->cmd, textcmdbuf2->cmd[0]+1);
				textcmdbuf2 = textcmdbuf2->next;
				Z_Free(buf);
			}
		}
	}
}

void SendKick(UINT8 playernum, UINT8 msg)
{
	UINT8 buf[2];

	if (!(server && cv_rejointimeout.value))
		msg &= ~KICK_MSG_KEEP_BODY;

	buf[0] = playernum;
	buf[1] = msg;
	SendNetXCmd(XD_KICK, &buf, 2);
}

void SendKicksForNode(SINT8 node, UINT8 msg)
{
	if (!netnodes[node].ingame)
		return;

	for (INT32 playernum = netnodes[node].player; playernum != -1; playernum = netnodes[node].player2)
		if (playernum != -1 && playeringame[playernum])
			SendKick(playernum, msg);
}
