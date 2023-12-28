// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  server_connection.c
/// \brief Server-side part of connection handling

#include "server_connection.h"
#include "i_net.h"
#include "d_clisrv.h"
#include "d_netfil.h"
#include "mserv.h"
#include "net_command.h"
#include "gamestate.h"
#include "../byteptr.h"
#include "../g_game.h"
#include "../g_state.h"
#include "../p_setup.h"
#include "../p_tick.h"
#include "../command.h"
#include "../doomstat.h"

// Minimum timeout for sending the savegame
// The actual timeout will be longer depending on the savegame length
tic_t jointimeout = (10*TICRATE);

// Incremented by cv_joindelay when a client joins, decremented each tic.
// If higher than cv_joindelay * 2 (3 joins in a short timespan), joins are temporarily disabled.
tic_t joindelay = 0;

// Minimum timeout for sending the savegame
// The actual timeout will be longer depending on the savegame length
char playeraddress[MAXPLAYERS][64];

consvar_t cv_showjoinaddress = CVAR_INIT ("showjoinaddress", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);

consvar_t cv_allownewplayer = CVAR_INIT ("allowjoin", "On", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);

static CV_PossibleValue_t maxplayers_cons_t[] = {{2, "MIN"}, {32, "MAX"}, {0, NULL}};
consvar_t cv_maxplayers = CVAR_INIT ("maxplayers", "8", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, maxplayers_cons_t, NULL);

static CV_PossibleValue_t joindelay_cons_t[] = {{1, "MIN"}, {3600, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_joindelay = CVAR_INIT ("joindelay", "10", CV_SAVE|CV_NETVAR, joindelay_cons_t, NULL);

static CV_PossibleValue_t rejointimeout_cons_t[] = {{1, "MIN"}, {60 * FRACUNIT, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_rejointimeout = CVAR_INIT ("rejointimeout", "2", CV_SAVE|CV_NETVAR|CV_FLOAT, rejointimeout_cons_t, NULL);

static INT32 FindRejoinerNum(SINT8 node)
{
	char addressbuffer[64];
	const char *nodeaddress;
	const char *strippednodeaddress;

	// Make sure there is no dead dress before proceeding to the stripping
	if (!I_GetNodeAddress)
		return -1;
	nodeaddress = I_GetNodeAddress(node);
	if (!nodeaddress)
		return -1;

	// Strip the address of its port
	strcpy(addressbuffer, nodeaddress);
	strippednodeaddress = I_NetSplitAddress(addressbuffer, NULL);

	// Check if any player matches the stripped address
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && playeraddress[i][0] && playernode[i] == UINT8_MAX
		&& !strcmp(playeraddress[i], strippednodeaddress))
			return i;
	}

	return -1;
}

static UINT8
GetRefuseReason (INT32 node)
{
	if (!node || FindRejoinerNum(node) != -1)
		return 0;
	else if (bannednode && bannednode[node])
		return REFUSE_BANNED;
	else if (!cv_allownewplayer.value)
		return REFUSE_JOINS_DISABLED;
	else if (D_NumPlayers() >= cv_maxplayers.value)
		return REFUSE_SLOTS_FULL;
	else
		return 0;
}

static void SV_SendServerInfo(INT32 node, tic_t servertime)
{
	UINT8 *p;

	netbuffer->packettype = PT_SERVERINFO;
	netbuffer->u.serverinfo._255 = 255;
	netbuffer->u.serverinfo.packetversion = PACKETVERSION;
	netbuffer->u.serverinfo.version = VERSION;
	netbuffer->u.serverinfo.subversion = SUBVERSION;
	strncpy(netbuffer->u.serverinfo.application, SRB2APPLICATION,
			sizeof netbuffer->u.serverinfo.application);
	// return back the time value so client can compute their ping
	netbuffer->u.serverinfo.time = (tic_t)LONG(servertime);
	netbuffer->u.serverinfo.leveltime = (tic_t)LONG(leveltime);

	// Exclude bots from both counts
	netbuffer->u.serverinfo.numberofplayer = (UINT8)D_NumNodes();
	netbuffer->u.serverinfo.maxplayer = (UINT8)(cv_maxplayers.value - D_NumBots());

	netbuffer->u.serverinfo.refusereason = GetRefuseReason(node);

	strncpy(netbuffer->u.serverinfo.gametypename, Gametype_Names[gametype],
			sizeof netbuffer->u.serverinfo.gametypename);
	netbuffer->u.serverinfo.modifiedgame = (UINT8)modifiedgame;
	netbuffer->u.serverinfo.cheatsenabled = CV_CheatsEnabled();
	netbuffer->u.serverinfo.flags = (dedicated ? SV_DEDICATED : 0);
	strncpy(netbuffer->u.serverinfo.servername, cv_servername.string,
		MAXSERVERNAME);
	strncpy(netbuffer->u.serverinfo.mapname, G_BuildMapName(gamemap), 7);

	M_Memcpy(netbuffer->u.serverinfo.mapmd5, mapmd5, 16);

	memset(netbuffer->u.serverinfo.maptitle, 0, sizeof netbuffer->u.serverinfo.maptitle);

	if (mapheaderinfo[gamemap-1] && *mapheaderinfo[gamemap-1]->lvlttl)
	{
		char *read = mapheaderinfo[gamemap-1]->lvlttl, *writ = netbuffer->u.serverinfo.maptitle;
		while (writ < (netbuffer->u.serverinfo.maptitle+32) && *read != '\0')
		{
			if (!(*read & 0x80))
			{
				*writ = toupper(*read);
				writ++;
			}
			read++;
		}
		*writ = '\0';
		//strncpy(netbuffer->u.serverinfo.maptitle, (char *)mapheaderinfo[gamemap-1]->lvlttl, 33);
	}
	else
		strncpy(netbuffer->u.serverinfo.maptitle, "UNKNOWN", 32);

	if (mapheaderinfo[gamemap-1] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE))
		netbuffer->u.serverinfo.iszone = 1;
	else
		netbuffer->u.serverinfo.iszone = 0;

	if (mapheaderinfo[gamemap-1])
		netbuffer->u.serverinfo.actnum = mapheaderinfo[gamemap-1]->actnum;

	p = PutFileNeeded(0);

	HSendPacket(node, false, 0, p - ((UINT8 *)&netbuffer->u));
}

static void SV_SendPlayerInfo(INT32 node)
{
	netbuffer->packettype = PT_PLAYERINFO;

	for (UINT8 i = 0; i < MAXPLAYERS; i++)
	{
		if (playernode[i] == UINT8_MAX || !netnodes[playernode[i]].ingame)
		{
			netbuffer->u.playerinfo[i].num = 255; // This slot is empty.
			continue;
		}

		netbuffer->u.playerinfo[i].num = i;
		strncpy(netbuffer->u.playerinfo[i].name, (const char *)&player_names[i], MAXPLAYERNAME+1);
		netbuffer->u.playerinfo[i].name[MAXPLAYERNAME] = '\0';

		//fetch IP address
		//No, don't do that, you fuckface.
		memset(netbuffer->u.playerinfo[i].address, 0, 4);

		if (G_GametypeHasTeams())
		{
			if (!players[i].ctfteam)
				netbuffer->u.playerinfo[i].team = 255;
			else
				netbuffer->u.playerinfo[i].team = (UINT8)players[i].ctfteam;
		}
		else
		{
			if (players[i].spectator)
				netbuffer->u.playerinfo[i].team = 255;
			else
				netbuffer->u.playerinfo[i].team = 0;
		}

		netbuffer->u.playerinfo[i].score = LONG(players[i].score);
		netbuffer->u.playerinfo[i].timeinserver = SHORT((UINT16)(players[i].jointime / TICRATE));
		netbuffer->u.playerinfo[i].skin = (UINT8)(players[i].skin
#ifdef DEVELOP // it's safe to do this only because PLAYERINFO isn't read by the game itself
		% 3
#endif
		);

		// Extra data
		netbuffer->u.playerinfo[i].data = 0; //players[i].skincolor;

		if (players[i].pflags & PF_TAGIT)
			netbuffer->u.playerinfo[i].data |= 0x20;

		if (players[i].gotflag)
			netbuffer->u.playerinfo[i].data |= 0x40;

		if (players[i].powers[pw_super])
			netbuffer->u.playerinfo[i].data |= 0x80;
	}

	HSendPacket(node, false, 0, sizeof(plrinfo_pak) * MAXPLAYERS);
}

/** Sends a PT_SERVERCFG packet
  *
  * \param node The destination
  * \return True if the packet was successfully sent
  *
  */
static boolean SV_SendServerConfig(INT32 node)
{
	boolean waspacketsent;

	netbuffer->packettype = PT_SERVERCFG;

	netbuffer->u.servercfg.serverplayer = (UINT8)serverplayer;
	netbuffer->u.servercfg.totalslotnum = (UINT8)(doomcom->numslots);
	netbuffer->u.servercfg.gametic = (tic_t)LONG(gametic);
	netbuffer->u.servercfg.clientnode = (UINT8)node;
	netbuffer->u.servercfg.gamestate = (UINT8)gamestate;
	netbuffer->u.servercfg.gametype = (UINT8)gametype;
	netbuffer->u.servercfg.modifiedgame = (UINT8)modifiedgame;
	netbuffer->u.servercfg.usedCheats = (UINT8)usedCheats;

	memcpy(netbuffer->u.servercfg.server_context, server_context, 8);

	{
		const size_t len = sizeof (serverconfig_pak);

#ifdef DEBUGFILE
		if (debugfile)
		{
			fprintf(debugfile, "ServerConfig Packet about to be sent, size of packet:%s to node:%d\n",
				sizeu1(len), node);
		}
#endif

		waspacketsent = HSendPacket(node, true, 0, len);
	}

#ifdef DEBUGFILE
	if (debugfile)
	{
		if (waspacketsent)
		{
			fprintf(debugfile, "ServerConfig Packet was sent\n");
		}
		else
		{
			fprintf(debugfile, "ServerConfig Packet could not be sent right now\n");
		}
	}
#endif

	return waspacketsent;
}

// Adds a node to the game (player will follow at map change or at savegame....)
static inline void SV_AddNode(INT32 node)
{
	netnodes[node].tic = gametic;
	netnodes[node].supposedtic = gametic;
	// little hack because the server connects to itself and puts
	// nodeingame when connected not here
	if (node)
		netnodes[node].ingame = true;
}

static void SV_AddPlayer(SINT8 node, const char *name)
{
	INT32 n;
	UINT8 buf[2 + MAXPLAYERNAME];
	UINT8 *p;
	INT32 newplayernum;

	newplayernum = FindRejoinerNum(node);
	if (newplayernum == -1)
	{
		// search for a free playernum
		// we can't use playeringame since it is not updated here
		for (newplayernum = dedicated ? 1 : 0; newplayernum < MAXPLAYERS; newplayernum++)
		{
			if (playeringame[newplayernum])
				continue;
			for (n = 0; n < MAXNETNODES; n++)
				if (netnodes[n].player == newplayernum || netnodes[n].player2 == newplayernum)
					break;
			if (n == MAXNETNODES)
				break;
		}
	}

	// should never happen since we check the playernum
	// before accepting the join
	I_Assert(newplayernum < MAXPLAYERS);

	playernode[newplayernum] = (UINT8)node;

	p = buf + 2;
	buf[0] = (UINT8)node;
	buf[1] = newplayernum;
	if (netnodes[node].numplayers < 1)
	{
		netnodes[node].player = newplayernum;
	}
	else
	{
		netnodes[node].player2 = newplayernum;
		buf[1] |= 0x80;
	}
	WRITESTRINGN(p, name, MAXPLAYERNAME);
	netnodes[node].numplayers++;

	SendNetXCmd(XD_ADDPLAYER, &buf, p - buf);

	DEBFILE(va("Server added player %d node %d\n", newplayernum, node));
}

static void SV_SendRefuse(INT32 node, const char *reason)
{
	strcpy(netbuffer->u.serverrefuse.reason, reason);

	netbuffer->packettype = PT_SERVERREFUSE;
	HSendPacket(node, true, 0, strlen(netbuffer->u.serverrefuse.reason) + 1);
	Net_CloseConnection(node);
}

static const char *
GetRefuseMessage (SINT8 node, INT32 rejoinernum)
{
	clientconfig_pak *cc = &netbuffer->u.clientcfg;

	boolean rejoining = (rejoinernum != -1);

	if (!node)/* server connecting to itself */
		return NULL;

	if (
			cc->modversion != MODVERSION ||
			strncmp(cc->application, SRB2APPLICATION,
				sizeof cc->application)
	){
		return/* this is probably client's fault */
			"Incompatible.";
	}
	else if (bannednode && bannednode[node])
	{
		return
			"You have been banned\n"
			"from the server.";
	}
	else if (cc->localplayers != 1)
	{
		return
			"Wrong player count.";
	}

	if (!rejoining)
	{
		if (!cv_allownewplayer.value)
		{
			return
				"The server is not accepting\n"
				"joins for the moment.";
		}
		else if (D_NumPlayers() >= cv_maxplayers.value)
		{
			return va(
					"Maximum players reached: %d",
					cv_maxplayers.value);
		}
	}

	if (luafiletransfers)
	{
		return
			"The serveris broadcasting a file\n"
			"requested by a Lua script.\n"
			"Please wait a bit and then\n"
			"try rejoining.";
	}

	if (netgame)
	{
		const tic_t th = 2 * cv_joindelay.value * TICRATE;

		if (joindelay > th)
		{
			return va(
					"Too many people are connecting.\n"
					"Please wait %d seconds and then\n"
					"try rejoining.",
					(joindelay - th) / TICRATE);
		}
	}

	return NULL;
}

/** Called when a PT_CLIENTJOIN packet is received
  *
  * \param node The packet sender
  *
  */
void PT_ClientJoin(SINT8 node)
{
	char names[MAXSPLITSCREENPLAYERS][MAXPLAYERNAME + 1];
	INT32 numplayers = netbuffer->u.clientcfg.localplayers;
	INT32 rejoinernum;

	// Ignore duplicate packets
	if (client || netnodes[node].ingame)
		return;

	rejoinernum = FindRejoinerNum(node);

	const char *refuse = GetRefuseMessage(node, rejoinernum);
	if (refuse)
	{
		SV_SendRefuse(node, refuse);
		return;
	}

	for (INT32 i = 0; i < numplayers; i++)
	{
		strlcpy(names[i], netbuffer->u.clientcfg.names[i], MAXPLAYERNAME + 1);
		if (!EnsurePlayerNameIsGood(names[i], rejoinernum))
		{
			SV_SendRefuse(node, "Bad player name");
			return;
		}
	}

	SV_AddNode(node);

	if (!SV_SendServerConfig(node))
	{
		/// \note Shouldn't SV_SendRefuse be called before ResetNode?
		ResetNode(node);
		SV_SendRefuse(node, M_GetText("Server couldn't send info, please try again"));
		/// \todo fix this !!!
		return;
	}
	DEBFILE("new node joined\n");

	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
	{
		SV_SendSaveGame(node, false); // send a complete game state
		DEBFILE("send savegame\n");
	}

	// Splitscreen can allow 2 players in one node
	for (INT32 i = 0; i < numplayers; i++)
		SV_AddPlayer(node, names[i]);

	joindelay += cv_joindelay.value * TICRATE;
}

void PT_AskInfoViaMS(SINT8 node)
{
	Net_CloseConnection(node);
}

void PT_TellFilesNeeded(SINT8 node)
{
	if (server && serverrunning)
	{
		UINT8 *p;
		INT32 firstfile = netbuffer->u.filesneedednum;

		netbuffer->packettype = PT_MOREFILESNEEDED;
		netbuffer->u.filesneededcfg.first = firstfile;
		netbuffer->u.filesneededcfg.more = 0;

		p = PutFileNeeded(firstfile);

		HSendPacket(node, false, 0, p - ((UINT8 *)&netbuffer->u));
	}
	else // Shouldn't get this if you aren't the server...?
		Net_CloseConnection(node);
}

void PT_AskInfo(SINT8 node)
{
	if (server && serverrunning)
	{
		SV_SendServerInfo(node, (tic_t)LONG(netbuffer->u.askinfo.time));
		SV_SendPlayerInfo(node); // Send extra info
	}
	Net_CloseConnection(node);
}
