// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  commands.c
/// \brief Various netgame commands, such as kick and ban

#include "commands.h"
#include "d_clisrv.h"
#include "client_connection.h"
#include "net_command.h"
#include "d_netcmd.h"
#include "d_net.h"
#include "i_net.h"
#include "protocol.h"
#include "../byteptr.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../doomstat.h"
#include "../doomdef.h"
#include "../r_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct banreason_s
{
	char *reason;
	struct banreason_s *prev; //-1
	struct banreason_s *next; //+1
} banreason_t;

static banreason_t *reasontail = NULL; //last entry, use prev
static banreason_t *reasonhead = NULL; //1st entry, use next

static boolean bans_loaded = false;

void Ban_Add(const char *reason)
{
	banreason_t *reasonlist = malloc(sizeof(*reasonlist));

	if (!reasonlist)
		return;
	if (!reason)
		reason = "NA";

	reasonlist->next = NULL;
	reasonlist->reason = Z_StrDup(reason);
	if ((reasonlist->prev = reasontail) == NULL)
		reasonhead = reasonlist;
	else
		reasontail->next = reasonlist;
	reasontail = reasonlist;
}

static void Ban_Clear(void)
{
	banreason_t *temp;

	I_ClearBans();

	reasontail = NULL;

	while (reasonhead)
	{
		temp = reasonhead->next;
		Z_Free(reasonhead->reason);
		free(reasonhead);
		reasonhead = temp;
	}
}

void Ban_Load_File(boolean warning)
{
	FILE *f;
	char *address, *mask;
	char buffer[MAX_WADPATH];

	if (!I_ClearBans)
		return;

	bans_loaded = true;

	f = fopen(va("%s"PATHSEP"%s", srb2home, "ban.txt"), "r");

	if (!f)
	{
		if (warning)
			CONS_Alert(CONS_WARNING, M_GetText("Could not open ban.txt for ban list\n"));
		return;
	}

	Ban_Clear();

	for (; fgets(buffer, (int)sizeof(buffer), f);)
	{
		address = strtok(buffer, " \t\r\n");
		mask = strtok(NULL, " \t\r\n");
		if (address[0] == '[')
		{
			size_t len;
			address++;
			len = strlen(address);
			if (address[len-1] == ']')
				address[len-1] = '\0';
		}

		I_SetBanAddress(address, mask);

		Ban_Add(strtok(NULL, "\r\n"));
	}

	fclose(f);
}

void D_SaveBan(void)
{
	FILE *f;
	banreason_t *reasonlist = reasonhead;
	const char *address, *mask;
	const char *path = va("%s"PATHSEP"%s", srb2home, "ban.txt");

	if (!bans_loaded)
	{
		// don't save bans if they were never loaded.
		return;
	}

	if (!reasonhead)
	{
		remove(path);
		return;
	}

	f = fopen(path, "w");

	if (!f)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Could not save ban list into ban.txt\n"));
		return;
	}

	for (size_t i = 0;(address = I_GetBanAddress(i)) != NULL;i++)
	{
		if (!I_GetBanMask || (mask = I_GetBanMask(i)) == NULL)
			fprintf(f, "%s 0", address);
		else
			fprintf(f, "%s %s", address, mask);

		if (reasonlist && reasonlist->reason)
			fprintf(f, " %s\n", reasonlist->reason);
		else
			fprintf(f, " %s\n", "NA");

		if (reasonlist) reasonlist = reasonlist->next;
	}

	fclose(f);
}

void Command_ShowBan(void) //Print out ban list
{
	size_t i;
	const char *address, *mask;
	banreason_t *reasonlist = reasonhead;

	if (I_GetBanAddress)
		CONS_Printf(M_GetText("Ban List:\n"));
	else
		return;

	for (i = 0;(address = I_GetBanAddress(i)) != NULL;i++)
	{
		if (!I_GetBanMask || (mask = I_GetBanMask(i)) == NULL)
			CONS_Printf("%s: %s ", sizeu1(i+1), address);
		else
			CONS_Printf("%s: %s/%s ", sizeu1(i+1), address, mask);

		if (reasonlist && reasonlist->reason)
			CONS_Printf("(%s)\n", reasonlist->reason);
		else
			CONS_Printf("\n");

		if (reasonlist) reasonlist = reasonlist->next;
	}

	if (i == 0 && !address)
		CONS_Printf(M_GetText("(empty)\n"));
}

void Command_ClearBans(void)
{
	if (!I_ClearBans)
		return;

	Ban_Clear();
	D_SaveBan();
}

void Command_Ban(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("Ban <playername/playernum> <reason>: ban and kick a player\n"));
		return;
	}

	if (!netgame) // Don't kick Tails in splitscreen!
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (server || IsPlayerAdmin(consoleplayer))
	{
		UINT8 buf[3 + MAX_REASONLENGTH];
		UINT8 *p = buf;
		const SINT8 pn = nametonum(COM_Argv(1));
		const INT32 node = playernode[(INT32)pn];

		if (pn == -1 || pn == 0)
			return;

		WRITEUINT8(p, pn);

		if (server && I_Ban && !I_Ban(node)) // only the server is allowed to do this right now
		{
			CONS_Alert(CONS_WARNING, M_GetText("Too many bans! Geez, that's a lot of people you're excluding...\n"));
			WRITEUINT8(p, KICK_MSG_GO_AWAY);
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		else
		{
			if (server) // only the server is allowed to do this right now
			{
				Ban_Add(COM_Argv(2));
				D_SaveBan(); // save the ban list
			}

			if (COM_Argc() == 2)
			{
				WRITEUINT8(p, KICK_MSG_BANNED);
				SendNetXCmd(XD_KICK, &buf, 2);
			}
			else
			{
				size_t j = COM_Argc();
				char message[MAX_REASONLENGTH];

				//Steal from the motd code so you don't have to put the reason in quotes.
				strlcpy(message, COM_Argv(2), sizeof message);
				for (size_t i = 3; i < j; i++)
				{
					strlcat(message, " ", sizeof message);
					strlcat(message, COM_Argv(i), sizeof message);
				}

				WRITEUINT8(p, KICK_MSG_CUSTOM_BAN);
				WRITESTRINGN(p, message, MAX_REASONLENGTH);
				SendNetXCmd(XD_KICK, &buf, p - buf);
			}
		}
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));

}

void Command_BanIP(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("banip <ip> <reason>: ban an ip address\n"));
		return;
	}

	if (server) // Only the server can use this, otherwise does nothing.
	{
		char *addrbuf = NULL;
		const char *address = (COM_Argv(1));
		const char *mask = strchr(address, '/');
		const char *reason;

		if (COM_Argc() == 2)
			reason = NULL;
		else
			reason = COM_Argv(2);

		if (mask != NULL)
		{
			addrbuf = Z_Malloc(mask - address + 1, PU_STATIC, NULL);
			memcpy(addrbuf, address, mask - address);
			addrbuf[mask - address] = '\0';
			address = addrbuf;
			mask++;
		}

		if (I_SetBanAddress && I_SetBanAddress(address, mask))
		{
			if (reason)
				CONS_Printf("Banned IP address %s for: %s\n", address, reason);
			else
				CONS_Printf("Banned IP address %s\n", address);

			Ban_Add(reason);
			D_SaveBan();
		}
		else
		{
			CONS_Printf("Unable to apply ban: address in malformed or invalid, or too many bans are applied\n");
		}
		Z_Free(addrbuf);
	}
}

void Command_ReloadBan(void)  //recheck ban.txt
{
	Ban_Load_File(true);
}

void Command_Kick(void)
{
	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("kick <playername/playernum> <reason>: kick a player\n"));
		return;
	}

	if (!netgame) // Don't kick Tails in splitscreen!
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (server || IsPlayerAdmin(consoleplayer))
	{
		UINT8 buf[3 + MAX_REASONLENGTH];
		UINT8 *p = buf;
		const SINT8 pn = nametonum(COM_Argv(1));

		if (pn == -1 || pn == 0)
			return;

		// Special case if we are trying to kick a player who is downloading the game state:
		// trigger a timeout instead of kicking them, because a kick would only
		// take effect after they have finished downloading
		if (server && playernode[pn] != UINT8_MAX && netnodes[playernode[pn]].sendingsavegame)
		{
			Net_ConnectionTimeout(playernode[pn]);
			return;
		}

		WRITESINT8(p, pn);

		if (COM_Argc() == 2)
		{
			WRITEUINT8(p, KICK_MSG_GO_AWAY);
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		else
		{
			size_t j = COM_Argc();
			char message[MAX_REASONLENGTH];

			//Steal from the motd code so you don't have to put the reason in quotes.
			strlcpy(message, COM_Argv(2), sizeof message);
			for (size_t i = 3; i < j; i++)
			{
				strlcat(message, " ", sizeof message);
				strlcat(message, COM_Argv(i), sizeof message);
			}

			WRITEUINT8(p, KICK_MSG_CUSTOM_KICK);
			WRITESTRINGN(p, message, MAX_REASONLENGTH);
			SendNetXCmd(XD_KICK, &buf, p - buf);
		}
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}

void Command_connect(void)
{
	if (COM_Argc() < 2 || *COM_Argv(1) == 0)
	{
		CONS_Printf(M_GetText(
			"Connect <serveraddress> (port): connect to a server\n"
			"Connect ANY: connect to the first lan server found\n"
			//"Connect SELF: connect to your own server.\n"
			));
		return;
	}

	if (Playing() || titledemo)
	{
		CONS_Printf(M_GetText("You cannot connect while in a game. End this game first.\n"));
		return;
	}

	server = false;
/*
	if (!stricmp(COM_Argv(1), "self"))
	{
		servernode = 0;
		server = true;
		/// \bug should be but...
		//SV_SpawnServer();
	}
	else
*/
	{
		// used in menu to connect to a server in the list
		if (netgame && !stricmp(COM_Argv(1), "node"))
		{
			servernode = (SINT8)atoi(COM_Argv(2));
		}
		else if (netgame)
		{
			CONS_Printf(M_GetText("You cannot connect while in a game. End this game first.\n"));
			return;
		}
		else if (I_NetOpenSocket)
		{
			I_NetOpenSocket();
			netgame = true;
			multiplayer = true;

			if (!stricmp(COM_Argv(1), "any"))
				servernode = BROADCASTADDR;
			else if (I_NetMakeNodewPort)
			{
				if (COM_Argc() >= 3) // address AND port
					servernode = I_NetMakeNodewPort(COM_Argv(1), COM_Argv(2));
				else // address only, or address:port
					servernode = I_NetMakeNode(COM_Argv(1));
			}
			else
			{
				CONS_Alert(CONS_ERROR, M_GetText("There is no server identification with this network driver\n"));
				D_CloseConnection();
				return;
			}
		}
		else
			CONS_Alert(CONS_ERROR, M_GetText("There is no network driver\n"));
	}

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	CL_ConnectToServer();
}

void Command_GetPlayerNum(void)
{
	for (INT32 i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
		{
			if (serverplayer == i)
				CONS_Printf(M_GetText("num:%2d  node:%2d  %s\n"), i, playernode[i], player_names[i]);
			else
				CONS_Printf(M_GetText("\x82num:%2d  node:%2d  %s\n"), i, playernode[i], player_names[i]);
		}
}

/** Lists all players and their player numbers.
  *
  * \sa Command_GetPlayerNum
  */
void Command_Nodes(void)
{
	size_t maxlen = 0;
	const char *address;

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		const size_t plen = strlen(player_names[i]);
		if (playeringame[i] && plen > maxlen)
			maxlen = plen;
	}

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			CONS_Printf("%.2u: %*s", i, (int)maxlen, player_names[i]);

			if (playernode[i] != UINT8_MAX)
			{
				CONS_Printf(" - node %.2d", playernode[i]);
				if (I_GetNodeAddress && (address = I_GetNodeAddress(playernode[i])) != NULL)
					CONS_Printf(" - %s", address);
			}

			if (IsPlayerAdmin(i))
				CONS_Printf(M_GetText(" (verified admin)"));

			if (players[i].spectator)
				CONS_Printf(M_GetText(" (spectator)"));

			CONS_Printf("\n");
		}
	}
}
