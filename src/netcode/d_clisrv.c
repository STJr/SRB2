// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_clisrv.c
/// \brief SRB2 Network game communication and protocol, all OS independent parts.

#include <time.h>
#ifdef __GNUC__
#include <unistd.h> //for unlink
#endif

#include "../i_time.h"
#include "i_net.h"
#include "../i_system.h"
#include "../i_video.h"
#include "d_net.h"
#include "../d_main.h"
#include "../g_game.h"
#include "../st_stuff.h"
#include "../hu_stuff.h"
#include "../keys.h"
#include "../m_menu.h"
#include "../console.h"
#include "d_netfil.h"
#include "../byteptr.h"
#include "../p_saveg.h"
#include "../z_zone.h"
#include "../p_local.h"
#include "../m_misc.h"
#include "../am_map.h"
#include "../m_random.h"
#include "mserv.h"
#include "../y_inter.h"
#include "../r_local.h"
#include "../m_argv.h"
#include "../p_setup.h"
#include "../lzf.h"
#include "../lua_script.h"
#include "../lua_hook.h"
#include "../lua_libs.h"
#include "../md5.h"
#include "../m_perfstats.h"
#include "server_connection.h"
#include "client_connection.h"
#include "tic_command.h"
#include "net_command.h"
#include "gamestate.h"
#include "commands.h"
#include "protocol.h"

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// Server:
//   maketic is the tic that hasn't had control made for it yet
//   nettics is the tic for each node
//   firstticstosend is the lowest value of nettics
// Client:
//   neededtic is the tic needed by the client to run the game
//   firstticstosend is used to optimize a condition
// Normally maketic >= gametic > 0

boolean server = true; // true or false but !server == client
boolean serverrunning = false;
INT32 serverplayer = 0;
char motd[254], server_context[8]; // Message of the Day, Unique Context (even without Mumble support)

netnode_t netnodes[MAXNETNODES];

// Server specific vars
UINT8 playernode[MAXPLAYERS];

UINT16 pingmeasurecount = 1;
UINT32 realpingtable[MAXPLAYERS]; //the base table of ping where an average will be sent to everyone.
UINT32 playerpingtable[MAXPLAYERS]; //table of player latency values.
static INT32 pingtimeout[MAXPLAYERS];
tic_t servermaxping = 800; // server's max ping. Defaults to 800

tic_t maketic;

INT16 consistancy[BACKUPTICS];

// true when a player is connecting or disconnecting so that the gameplay has stopped in its tracks
boolean hu_stopped = false;

UINT8 (*adminpassmd5)[16];
UINT32 adminpasscount = 0;

tic_t neededtic;
SINT8 servernode = 0; // the number of the server node

boolean acceptnewnode = true;

UINT16 software_MAXPACKETLENGTH;

static tic_t gametime = 0;

static CV_PossibleValue_t netticbuffer_cons_t[] = {{0, "MIN"}, {3, "MAX"}, {0, NULL}};
consvar_t cv_netticbuffer = CVAR_INIT ("netticbuffer", "1", CV_SAVE, netticbuffer_cons_t, NULL);

static CV_PossibleValue_t resynchattempts_cons_t[] = {{1, "MIN"}, {20, "MAX"}, {0, "No"}, {0, NULL}};
consvar_t cv_resynchattempts = CVAR_INIT ("resynchattempts", "10", CV_SAVE|CV_NETVAR, resynchattempts_cons_t, NULL);

consvar_t cv_blamecfail = CVAR_INIT ("blamecfail", "Off", CV_SAVE|CV_NETVAR, CV_OnOff, NULL);

static CV_PossibleValue_t playbackspeed_cons_t[] = {{1, "MIN"}, {10, "MAX"}, {0, NULL}};
consvar_t cv_playbackspeed = CVAR_INIT ("playbackspeed", "1", 0, playbackspeed_cons_t, NULL);

consvar_t cv_idletime = CVAR_INIT ("idletime", "0", CV_SAVE, CV_Unsigned, NULL);
consvar_t cv_idlespectate = CVAR_INIT ("idlespectate", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_dedicatedidletime = CVAR_INIT ("dedicatedidletime", "10", CV_SAVE, CV_Unsigned, NULL);

consvar_t cv_httpsource = CVAR_INIT ("http_source", "", CV_SAVE, NULL, NULL);

void ResetNode(INT32 node)
{
	memset(&netnodes[node], 0, sizeof(*netnodes));
	netnodes[node].player = -1;
	netnodes[node].player2 = -1;
}

void CL_Reset(void)
{
	if (metalrecording)
		G_StopMetalRecording(false);
	if (metalplayback)
		G_StopMetalDemo();
	if (demorecording)
		G_CheckDemoStatus();

	// reset client/server code
	DEBFILE(va("\n-=-=-=-=-=-=-= Client reset =-=-=-=-=-=-=-\n\n"));

	if (servernode > 0 && servernode < MAXNETNODES)
	{
		netnodes[(UINT8)servernode].ingame = false;
		Net_CloseConnection(servernode);
	}
	D_CloseConnection(); // netgame = false
	multiplayer = false;
	servernode = 0;
	server = true;
	doomcom->numnodes = 1;
	doomcom->numslots = 1;
	SV_StopServer();
	SV_ResetServer();

	// make sure we don't leave any fileneeded gunk over from a failed join
	FreeFileNeeded();
	fileneedednum = 0;

	firstconnectattempttime = 0;
	serverisfull = false;
	connectiontimeout = (tic_t)cv_nettimeout.value; //reset this temporary hack

	filedownload.remaining = 0;
	filedownload.http_failed = false;
	filedownload.http_running = false;
	filedownload.http_source[0] = '\0';

	// D_StartTitle should get done now, but the calling function will handle it
}

//
// CL_ClearPlayer
//
// Clears the player data so that a future client can use this slot
//
void CL_ClearPlayer(INT32 playernum)
{
	if (players[playernum].mo)
		P_RemoveMobj(players[playernum].mo);
	memset(&players[playernum], 0, sizeof (player_t));
	memset(playeraddress[playernum], 0, sizeof(*playeraddress));
}

// Xcmd XD_ADDPLAYER
static void Got_AddPlayer(UINT8 **p, INT32 playernum)
{
	INT16 node, newplayernum;
	boolean splitscreenplayer;
	boolean rejoined;
	player_t *newplayer;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		// protect against hacked/buggy client
		CONS_Alert(CONS_WARNING, M_GetText("Illegal add player command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	node = READUINT8(*p);
	newplayernum = READUINT8(*p);
	splitscreenplayer = newplayernum & 0x80;
	newplayernum &= ~0x80;

	rejoined = playeringame[newplayernum];

	if (!rejoined)
	{
		// Clear player before joining, lest some things get set incorrectly
		// HACK: don't do this for splitscreen, it relies on preset values
		if (!splitscreen && !botingame)
			CL_ClearPlayer(newplayernum);
		playeringame[newplayernum] = true;
		G_AddPlayer(newplayernum);
		if (newplayernum+1 > doomcom->numslots)
			doomcom->numslots = (INT16)(newplayernum+1);

		if (server && I_GetNodeAddress)
		{
			char addressbuffer[64];
			const char *address = I_GetNodeAddress(node);
			if (address) // MI: fix msvcrt.dll!_mbscat crash?
			{
				strcpy(addressbuffer, address);
				strcpy(playeraddress[newplayernum],
						I_NetSplitAddress(addressbuffer, NULL));
			}
		}
	}

	newplayer = &players[newplayernum];

	newplayer->jointime = 0;
	newplayer->quittime = 0;
	newplayer->lastinputtime = 0;

	READSTRINGN(*p, player_names[newplayernum], MAXPLAYERNAME);

	// the server is creating my player
	if (node == mynode)
	{
		playernode[newplayernum] = 0; // for information only
		if (!splitscreenplayer)
		{
			consoleplayer = newplayernum;
			displayplayer = newplayernum;
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning me\n");
			ticcmd_oldangleturn[0] = newplayer->oldrelangleturn;
		}
		else
		{
			secondarydisplayplayer = newplayernum;
			DEBFILE("spawning my brother\n");
			if (botingame)
				newplayer->bot = 1;
			ticcmd_oldangleturn[1] = newplayer->oldrelangleturn;
		}
		P_ForceLocalAngle(newplayer, (angle_t)(newplayer->angleturn << 16));
		D_SendPlayerConfig();
		addedtogame = true;

		if (rejoined)
		{
			if (newplayer->mo)
			{
				newplayer->viewheight = 41*newplayer->height/48;

				if (newplayer->mo->eflags & MFE_VERTICALFLIP)
					newplayer->viewz = newplayer->mo->z + newplayer->mo->height - newplayer->viewheight;
				else
					newplayer->viewz = newplayer->mo->z + newplayer->viewheight;
			}

			// wake up the status bar
			ST_Start();
			// wake up the heads up text
			HU_Start();

			if (camera.chase && !splitscreenplayer)
				P_ResetCamera(newplayer, &camera);
			if (camera2.chase && splitscreenplayer)
				P_ResetCamera(newplayer, &camera2);
		}
	}

	if (netgame)
	{
		char joinmsg[256];

		if (rejoined)
			strcpy(joinmsg, M_GetText("\x82*%s has rejoined the game (player %d)"));
		else
			strcpy(joinmsg, M_GetText("\x82*%s has joined the game (player %d)"));
		strcpy(joinmsg, va(joinmsg, player_names[newplayernum], newplayernum));

		// Merge join notification + IP to avoid clogging console/chat
		if (server && cv_showjoinaddress.value && I_GetNodeAddress)
		{
			const char *address = I_GetNodeAddress(node);
			if (address)
				strcat(joinmsg, va(" (%s)", address));
		}

		HU_AddChatText(joinmsg, false);
	}

	if (server && multiplayer && motd[0] != '\0')
		COM_BufAddText(va("sayto %d %s\n", newplayernum, motd));

	if (!rejoined)
		LUA_HookInt(newplayernum, HOOK(PlayerJoin));
}

static void UnlinkPlayerFromNode(INT32 playernum)
{
	INT32 node = playernode[playernum];

	if (node == UINT8_MAX)
		return;

	playernode[playernum] = UINT8_MAX;

	netnodes[node].numplayers--;
	if (netnodes[node].numplayers <= 0)
	{
		netnodes[node].ingame = false;
		Net_CloseConnection(node);
		ResetNode(node);
	}
}

static void PT_ClientQuit(SINT8 node, INT32 netconsole)
{
	if (client)
		return;

	if (netnodes[node].ingame && netconsole != -1 && playeringame[netconsole])
		SendKicksForNode(node, KICK_MSG_PLAYER_QUIT | KICK_MSG_KEEP_BODY);

	Net_CloseConnection(node);
	netnodes[node].ingame = false;
	netnodes[node].player = -1;
	netnodes[node].player2 = -1;
}

static void Got_KickCmd(UINT8 **p, INT32 playernum)
{
	INT32 pnum, msg;
	char buf[3 + MAX_REASONLENGTH];
	char *reason = buf;
	kickreason_t kickreason = KR_KICK;
	boolean keepbody;

	pnum = READUINT8(*p);
	msg = READUINT8(*p);
	keepbody = (msg & KICK_MSG_KEEP_BODY) != 0;
	msg &= ~KICK_MSG_KEEP_BODY;

	if (pnum == serverplayer && IsPlayerAdmin(playernum))
	{
		CONS_Printf(M_GetText("Server is being shut down remotely. Goodbye!\n"));

		if (server)
			COM_BufAddText("quit\n");

		return;
	}

	// Is playernum authorized to make this kick?
	if (playernum != serverplayer && !IsPlayerAdmin(playernum)
		&& !(playernode[playernum] != UINT8_MAX && netnodes[playernode[playernum]].numplayers == 2
		&& netnodes[playernode[playernum]].player2 == pnum))
	{
		// We received a kick command from someone who isn't the
		// server or admin, and who isn't in splitscreen removing
		// player 2. Thus, it must be someone with a modified
		// binary, trying to kick someone but without having
		// authorization.

		// We deal with this by changing the kick reason to
		// "consistency failure" and kicking the offending user
		// instead.

		// Note: Splitscreen in netgames is broken because of
		// this. Only the server has any idea of which players
		// are using splitscreen on the same computer, so
		// clients cannot always determine if a kick is
		// legitimate.

		CONS_Alert(CONS_WARNING, M_GetText("Illegal kick command received from %s for player %d\n"), player_names[playernum], pnum);

		// In debug, print a longer message with more details.
		// TODO Callum: Should we translate this?
/*
		CONS_Debug(DBG_NETPLAY,
			"So, you must be asking, why is this an illegal kick?\n"
			"Well, let's take a look at the facts, shall we?\n"
			"\n"
			"playernum (this is the guy who did it), he's %d.\n"
			"pnum (the guy he's trying to kick) is %d.\n"
			"playernum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"pnum's node is %d.\n"
			"That node has %d players.\n"
			"Player 2 on that node is %d.\n"
			"\n"
			"If you think this is a bug, please report it, including all of the details above.\n",
				playernum, pnum,
				playernode[playernum], netnodes[playernode[playernum]].numplayers,
				netnodes[playernode[playernum]].player2,
				playernode[pnum], netnodes[playernode[pnum]].numplayers,
				netnodes[playernode[pnum]].player2);
*/
		pnum = playernum;
		msg = KICK_MSG_CON_FAIL;
		keepbody = true;
	}

	//CONS_Printf("\x82%s ", player_names[pnum]);

	switch (msg)
	{
		case KICK_MSG_GO_AWAY:
			if (!players[pnum].quittime)
				HU_AddChatText(va("\x82*%s has been kicked (No reason given)", player_names[pnum]), false);
			kickreason = KR_KICK;
			break;
		case KICK_MSG_PING_HIGH:
			HU_AddChatText(va("\x82*%s left the game (Broke ping limit)", player_names[pnum]), false);
			kickreason = KR_PINGLIMIT;
			break;
		case KICK_MSG_CON_FAIL:
			HU_AddChatText(va("\x82*%s left the game (Synch failure)", player_names[pnum]), false);
			kickreason = KR_SYNCH;

			if (M_CheckParm("-consisdump")) // Helps debugging some problems
			{
				CONS_Printf(M_GetText("Player kicked is #%d, dumping consistency...\n"), pnum);

				for (INT32 i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i])
						continue;
					CONS_Printf("-------------------------------------\n");
					CONS_Printf("Player %d: %s\n", i, player_names[i]);
					CONS_Printf("Skin: %d\n", players[i].skin);
					CONS_Printf("Color: %d\n", players[i].skincolor);
					CONS_Printf("Speed: %d\n",players[i].speed>>FRACBITS);
					if (players[i].mo)
					{
						if (!players[i].mo->skin)
							CONS_Printf("Mobj skin: NULL!\n");
						else
							CONS_Printf("Mobj skin: %s\n", ((skin_t *)players[i].mo->skin)->name);
						CONS_Printf("Position: %d, %d, %d\n", players[i].mo->x, players[i].mo->y, players[i].mo->z);
						if (!players[i].mo->state)
							CONS_Printf("State: S_NULL\n");
						else
							CONS_Printf("State: %d\n", (statenum_t)(players[i].mo->state-states));
					}
					else
						CONS_Printf("Mobj: NULL\n");
					CONS_Printf("-------------------------------------\n");
				}
			}
			break;
		case KICK_MSG_TIMEOUT:
			HU_AddChatText(va("\x82*%s left the game (Connection timeout)", player_names[pnum]), false);
			kickreason = KR_TIMEOUT;
			break;
		case KICK_MSG_PLAYER_QUIT:
			if (netgame && !players[pnum].quittime) // not splitscreen/bots or soulless body
				HU_AddChatText(va("\x82*%s left the game", player_names[pnum]), false);
			kickreason = KR_LEAVE;
			break;
		case KICK_MSG_BANNED:
			HU_AddChatText(va("\x82*%s has been banned (No reason given)", player_names[pnum]), false);
			kickreason = KR_BAN;
			break;
		case KICK_MSG_CUSTOM_KICK:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			HU_AddChatText(va("\x82*%s has been kicked (%s)", player_names[pnum], reason), false);
			kickreason = KR_KICK;
			break;
		case KICK_MSG_CUSTOM_BAN:
			READSTRINGN(*p, reason, MAX_REASONLENGTH+1);
			HU_AddChatText(va("\x82*%s has been banned (%s)", player_names[pnum], reason), false);
			kickreason = KR_BAN;
			break;
		case KICK_MSG_IDLE:
			HU_AddChatText(va("\x82*%s has left the game (Inactive for too long)", player_names[pnum]), false);
			kickreason = KR_TIMEOUT;
			break;
	}

	// If a verified admin banned someone, the server needs to know about it.
	// If the playernum isn't zero (the server) then the server needs to record the ban.
	if (server && playernum && (msg == KICK_MSG_BANNED || msg == KICK_MSG_CUSTOM_BAN))
	{
		if (I_Ban && !I_Ban(playernode[(INT32)pnum]))
			CONS_Alert(CONS_WARNING, M_GetText("Too many bans! Geez, that's a lot of people you're excluding...\n"));
		else
			Ban_Add(msg == KICK_MSG_CUSTOM_BAN ? reason : NULL);
	}

	if (pnum == consoleplayer)
	{
		LUA_HookBool(false, HOOK(GameQuit));
#ifdef DUMPCONSISTENCY
		if (msg == KICK_MSG_CON_FAIL) SV_SavedGame();
#endif
		D_QuitNetGame();
		CL_Reset();
		D_StartTitle();
		if (msg == KICK_MSG_CON_FAIL)
			M_StartMessage(M_GetText("Server closed connection\n(synch failure)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_PING_HIGH)
			M_StartMessage(M_GetText("Server closed connection\n(Broke ping limit)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_IDLE)
			M_StartMessage(M_GetText("Server closed connection\n(Inactive for too long)\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_BANNED)
			M_StartMessage(M_GetText("You have been banned by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_KICK)
			M_StartMessage(va(M_GetText("You have been kicked\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else if (msg == KICK_MSG_CUSTOM_BAN)
			M_StartMessage(va(M_GetText("You have been banned\n(%s)\nPress ESC\n"), reason), NULL, MM_NOTHING);
		else
			M_StartMessage(M_GetText("You have been kicked by the server\n\nPress ESC\n"), NULL, MM_NOTHING);
	}
	else if (keepbody)
	{
		if (server)
			UnlinkPlayerFromNode(pnum);
		players[pnum].quittime = 1;
	}
	else
		CL_RemovePlayer(pnum, kickreason);
}

// If in a special stage, redistribute the player's
// spheres across the remaining players.
// I feel like this shouldn't even be in this file at all, but well.
static void RedistributeSpecialStageSpheres(INT32 playernum)
{
	if (!G_IsSpecialStage(gamemap) || D_NumPlayers() <= 1)
		return;

	INT32 count = D_NumPlayers() - 1;
	INT32 spheres = players[playernum].spheres;
	INT32 rings = players[playernum].rings;

	while (spheres || rings)
	{
		INT32 sincrement = max(spheres / count, 1);
		INT32 rincrement = max(rings / count, 1);

		INT32 n;
		for (INT32 i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || i == playernum)
				continue;

			n = min(spheres, sincrement);
			P_GivePlayerSpheres(&players[i], n);
			spheres -= n;

			n = min(rings, rincrement);
			P_GivePlayerRings(&players[i], n);
			rings -= n;
		}
	}
}

//
// CL_RemovePlayer
//
// Removes a player from the current game
//
void CL_RemovePlayer(INT32 playernum, kickreason_t reason)
{
	// Sanity check: exceptional cases (i.e. c-fails) can cause multiple
	// kick commands to be issued for the same player.
	if (!playeringame[playernum])
		return;

	if (server)
		UnlinkPlayerFromNode(playernum);

	if (gametyperules & GTR_TEAMFLAGS)
		P_PlayerFlagBurst(&players[playernum], false); // Don't take the flag with you!

	RedistributeSpecialStageSpheres(playernum);

	LUA_HookPlayerQuit(&players[playernum], reason); // Lua hook for player quitting

	// don't look through someone's view who isn't there
	if (playernum == displayplayer)
	{
		// Call ViewpointSwitch hooks here.
		// The viewpoint was forcibly changed.
		LUA_HookViewpointSwitch(&players[consoleplayer], &players[consoleplayer], true);
		displayplayer = consoleplayer;
	}

	// Reset player data
	CL_ClearPlayer(playernum);

	// remove avatar of player
	playeringame[playernum] = false;
	while (!playeringame[doomcom->numslots-1] && doomcom->numslots > 1)
		doomcom->numslots--;

	// Reset the name
	sprintf(player_names[playernum], "Player %d", playernum+1);

	player_name_changes[playernum] = 0;

	if (IsPlayerAdmin(playernum))
	{
		RemoveAdminPlayer(playernum); // don't stay admin after you're gone
	}

	LUA_InvalidatePlayer(&players[playernum]);

	if (G_TagGametype()) //Check if you still have a game. Location flexible. =P
		P_CheckSurvivors();
	else if (gametyperules & GTR_RACE)
		P_CheckRacers();
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(void)
{
	mousegrabbedbylua = true;
	I_UpdateMouseGrab();

	if (!netgame || !netbuffer)
		return;

	DEBFILE("===========================================================================\n"
	        "                  Quitting Game, closing connection\n"
	        "===========================================================================\n");

	// abort send/receive of files
	CloseNetFile();
	RemoveAllLuaFileTransfers();
	waitingforluafiletransfer = false;
	waitingforluafilecommand = false;

	if (server)
	{
		netbuffer->packettype = PT_SERVERSHUTDOWN;
		for (INT32 i = 0; i < MAXNETNODES; i++)
			if (netnodes[i].ingame)
				HSendPacket(i, true, 0, 0);
#ifdef MASTERSERVER
		if (serverrunning && ms_RoomId > 0)
			UnregisterServer();
#endif
	}
	else if (servernode > 0 && servernode < MAXNETNODES && netnodes[(UINT8)servernode].ingame)
	{
		netbuffer->packettype = PT_CLIENTQUIT;
		HSendPacket(servernode, true, 0, 0);
	}

	D_CloseConnection();
	ClearAdminPlayers();

	DEBFILE("===========================================================================\n"
	        "                         Log finish\n"
	        "===========================================================================\n");
#ifdef DEBUGFILE
	if (debugfile)
	{
		fclose(debugfile);
		debugfile = NULL;
	}
#endif
}

void CL_HandleTimeout(void)
{
	LUA_HookBool(false, HOOK(GameQuit));
	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();
	M_StartMessage(M_GetText("Server Timeout\n\nPress Esc\n"), NULL, MM_NOTHING);
}

void CL_AddSplitscreenPlayer(void)
{
	if (cl_mode == CL_CONNECTED)
		CL_SendJoin();
}

void CL_RemoveSplitscreenPlayer(void)
{
	if (cl_mode != CL_CONNECTED)
		return;

	SendKick(secondarydisplayplayer, KICK_MSG_PLAYER_QUIT);
}

void SV_ResetServer(void)
{
	// +1 because this command will be executed in com_executebuffer in
	// tryruntic so gametic will be incremented, anyway maketic > gametic
	// is not an issue

	maketic = gametic + 1;
	neededtic = maketic;
	tictoclear = maketic;

	joindelay = 0;

	for (INT32 i = 0; i < MAXNETNODES; i++)
		ResetNode(i);

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		LUA_InvalidatePlayer(&players[i]);
		playeringame[i] = false;
		playernode[i] = UINT8_MAX;
		memset(playeraddress[i], 0, sizeof(*playeraddress));
		sprintf(player_names[i], "Player %d", i + 1);
		adminplayers[i] = -1; // Populate the entire adminplayers array with -1.
	}

	memset(player_name_changes, 0, sizeof player_name_changes);

	mynode = 0;
	cl_packetmissed = false;
	cl_redownloadinggamestate = false;

	if (dedicated)
	{
		netnodes[0].ingame = true;
		serverplayer = 0;
	}
	else
		serverplayer = consoleplayer;

	if (server)
		servernode = 0;

	doomcom->numslots = 0;

	// clear server_context
	memset(server_context, '-', 8);

	CV_RevertNetVars();

	// Ensure synched when creating a new server
	M_CopyGameData(serverGamedata, clientGamedata);

	DEBFILE("\n-=-=-=-=-=-=-= Server Reset =-=-=-=-=-=-=-\n\n");
}

static inline void SV_GenContext(void)
{
	// generate server_context, as exactly 8 bytes of randomly mixed A-Z and a-z
	// (hopefully M_Random is initialized!! if not this will be awfully silly!)
	for (UINT8 i = 0; i < 8; i++)
	{
		const char a = M_RandomKey(26*2);
		if (a < 26) // uppercase
			server_context[i] = 'A'+a;
		else // lowercase
			server_context[i] = 'a'+(a-26);
	}
}

void SV_SpawnServer(void)
{
	if (demoplayback)
		G_StopDemo(); // reset engine parameter
	if (metalplayback)
		G_StopMetalDemo();

	if (!serverrunning)
	{
		CONS_Printf(M_GetText("Starting Server....\n"));
		serverrunning = true;
		SV_ResetServer();
		SV_GenContext();
		if (netgame && I_NetOpenSocket)
		{
			I_NetOpenSocket();
#ifdef MASTERSERVER
			if (ms_RoomId > 0)
				RegisterServer();
#endif
		}

		// non dedicated server just connect to itself
		if (!dedicated)
			CL_ConnectToServer();
		else doomcom->numslots = 1;
	}
}

// called at singleplayer start and stopdemo
void SV_StartSinglePlayerServer(void)
{
	server = true;
	netgame = false;
	multiplayer = false;
	G_SetGametype(GT_COOP);

	// no more tic the game with this settings!
	SV_StopServer();

	if (splitscreen)
		multiplayer = true;
}

void SV_StopServer(void)
{
	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = wipegamestate = GS_NULL;

	localtextcmd[0] = 0;
	localtextcmd2[0] = 0;

	for (tic_t i = firstticstosend; i < firstticstosend + BACKUPTICS; i++)
		D_Clearticcmd(i);

	consoleplayer = 0;
	cl_mode = CL_SEARCHING;
	maketic = gametic+1;
	neededtic = maketic;
	serverrunning = false;
}

/** Called when a PT_SERVERSHUTDOWN packet is received
  *
  * \param node The packet sender (should be the server)
  *
  */
static void PT_ServerShutdown(SINT8 node)
{
	if (node != servernode || server || cl_mode == CL_SEARCHING)
		return;

	(void)node;
	LUA_HookBool(false, HOOK(GameQuit));
	D_QuitNetGame();
	CL_Reset();
	D_StartTitle();
	M_StartMessage(M_GetText("Server has shutdown\n\nPress Esc\n"), NULL, MM_NOTHING);
}

static void PT_Login(SINT8 node, INT32 netconsole)
{
	(void)node;

	if (client)
		return;

#ifndef NOMD5
	UINT8 finalmd5[16];/* Well, it's the cool thing to do? */
	UINT32 i;

	if (doomcom->datalength < 16)/* ignore partial sends */
		return;

	if (adminpasscount == 0)
	{
		CONS_Printf(M_GetText("Password from %s failed (no password set).\n"), player_names[netconsole]);
		return;
	}

	for (i = 0; i < adminpasscount; i++)
	{
		// Do the final pass to compare with the sent md5
		D_MD5PasswordPass(adminpassmd5[i], 16, va("PNUM%02d", netconsole), &finalmd5);

		if (!memcmp(netbuffer->u.md5sum, finalmd5, 16))
		{
			CONS_Printf(M_GetText("%s passed authentication.\n"), player_names[netconsole]);
			COM_BufInsertText(va("promote %d\n", netconsole)); // do this immediately
			return;
		}
	}

	CONS_Printf(M_GetText("Password from %s failed.\n"), player_names[netconsole]);
#else
	(void)netconsole;
#endif
}

/** Add a login for HTTP downloads. If the
  * user/password is missing, remove it.
  *
  * \sa Command_list_http_logins
  */
static void Command_set_http_login (void)
{
	HTTP_login  *login;
	HTTP_login **prev_next;

	if (COM_Argc() < 2)
	{
		CONS_Printf(
				"set_http_login <URL> [user:password]: Set or remove a login to "
				"authenticate HTTP downloads.\n"
		);
		return;
	}

	login = CURLGetLogin(COM_Argv(1), &prev_next);

	if (COM_Argc() == 2)
	{
		if (login)
		{
			(*prev_next) = login->next;
			CONS_Printf("Login for '%s' removed.\n", login->url);
			Z_Free(login);
		}
	}
	else
	{
		if (login)
			Z_Free(login->auth);
		else
		{
			login = ZZ_Alloc(sizeof *login);
			login->url = Z_StrDup(COM_Argv(1));
		}

		login->auth = Z_StrDup(COM_Argv(2));

		login->next = curl_logins;
		curl_logins = login;
	}
}

/** List logins for HTTP downloads.
  *
  * \sa Command_set_http_login
  */
static void Command_list_http_logins (void)
{
	HTTP_login *login;

	for (
			login = curl_logins;
			login;
			login = login->next
	){
		CONS_Printf(
				"'%s' -> '%s'\n",
				login->url,
				login->auth
		);
	}
}

static void PT_AskLuaFile(SINT8 node)
{
	if (server && luafiletransfers && luafiletransfers->nodestatus[node] == LFTNS_ASKED)
		AddLuaFileToSendQueue(node, luafiletransfers->realfilename);
}

static void PT_HasLuaFile(SINT8 node)
{
	if (server && luafiletransfers && luafiletransfers->nodestatus[node] == LFTNS_SENDING)
		SV_HandleLuaFileSent(node);
}

static void PT_SendingLuaFile(SINT8 node)
{
	(void)node;

	if (client)
		CL_PrepareDownloadLuaFile();
}

/*
Ping Update except better:
We call this once per second and check for people's pings. If their ping happens to be too high, we increment some timer and kick them out.
If they're not lagging, decrement the timer by 1. Of course, reset all of this if they leave.
*/

static inline void PingUpdate(void)
{
	boolean laggers[MAXPLAYERS];
	UINT8 numlaggers = 0;
	memset(laggers, 0, sizeof(boolean) * MAXPLAYERS);

	netbuffer->packettype = PT_PING;

	//check for ping limit breakage.
	if (cv_maxping.value)
	{
		for (INT32 i = 1; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].quittime
			&& (realpingtable[i] / pingmeasurecount > (unsigned)cv_maxping.value))
			{
				if (players[i].jointime > 30 * TICRATE)
					laggers[i] = true;
				numlaggers++;
			}
			else
				pingtimeout[i] = 0;
		}

		//kick lagging players... unless everyone but the server's ping sucks.
		//in that case, it is probably the server's fault.
		if (numlaggers < D_NumPlayers() - 1)
		{
			for (INT32 i = 1; i < MAXPLAYERS; i++)
			{
				if (playeringame[i] && laggers[i])
				{
					pingtimeout[i]++;
					// ok your net has been bad for too long, you deserve to die.
					if (pingtimeout[i] > cv_pingtimeout.value)
					{
						pingtimeout[i] = 0;
						SendKick(i, KICK_MSG_PING_HIGH | KICK_MSG_KEEP_BODY);
					}
				}
				/*
					you aren't lagging,
					but you aren't free yet.
					In case you'll keep spiking,
					we just make the timer go back down. (Very unstable net must still get kicked).
				*/
				else
					pingtimeout[i] = (pingtimeout[i] == 0 ? 0 : pingtimeout[i]-1);
			}
		}
	}

	//make the ping packet and clear server data for next one
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		netbuffer->u.pingtable[i] = realpingtable[i] / pingmeasurecount;
		//server takes a snapshot of the real ping for display.
		//otherwise, pings fluctuate a lot and would be odd to look at.
		playerpingtable[i] = realpingtable[i] / pingmeasurecount;
		realpingtable[i] = 0; //Reset each as we go.
	}

	// send the server's maxping as last element of our ping table. This is useful to let us know when we're about to get kicked.
	netbuffer->u.pingtable[MAXPLAYERS] = cv_maxping.value;

	//send out our ping packets
	for (INT32 i = 0; i < MAXNETNODES; i++)
		if (netnodes[i].ingame)
			HSendPacket(i, true, 0, sizeof(INT32) * (MAXPLAYERS+1));

	pingmeasurecount = 1; //Reset count
}

static void PT_Ping(SINT8 node, INT32 netconsole)
{
	// Only accept PT_PING from the server.
	if (node != servernode)
	{
		CONS_Alert(CONS_WARNING, M_GetText("%s received from non-host %d\n"), "PT_PING", node);
		if (server)
			SendKick(netconsole, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	//Update client ping table from the server.
	if (client)
	{
		for (INT32 i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				playerpingtable[i] = (tic_t)netbuffer->u.pingtable[i];

		servermaxping = (tic_t)netbuffer->u.pingtable[MAXPLAYERS];
	}
}

static void PT_BasicKeepAlive(SINT8 node, INT32 netconsole)
{
	if (client)
		return;

	// This should probably still timeout though, as the node should always have a player 1 number
	if (netconsole == -1)
		return;

	// If a client sends this it should mean they are done receiving the savegame
	netnodes[node].sendingsavegame = false;

	// As long as clients send keep alives, the server can keep running, so reset the timeout
	/// \todo Use a separate cvar for that kind of timeout?
	netnodes[node].freezetimeout = I_GetTime() + connectiontimeout;
	return;
}

// Confusing, but this DOESN'T send PT_NODEKEEPALIVE, it sends PT_BASICKEEPALIVE
// Used during wipes to tell the server that a node is still connected
static void CL_SendClientKeepAlive(void)
{
	netbuffer->packettype = PT_BASICKEEPALIVE;

	HSendPacket(servernode, false, 0, 0);
}

static void SV_SendServerKeepAlive(void)
{
	for (INT32 n = 1; n < MAXNETNODES; n++)
	{
		if (netnodes[n].ingame)
		{
			netbuffer->packettype = PT_BASICKEEPALIVE;
			HSendPacket(n, false, 0, 0);
		}
	}
}

/** Handles a packet received from a node that isn't in game
  *
  * \param node The packet sender
  * \todo Choose a better name, as the packet can also come from the server apparently?
  * \sa HandlePacketFromPlayer
  * \sa GetPackets
  *
  */
static void HandlePacketFromAwayNode(SINT8 node)
{
	if (node != servernode)
		DEBFILE(va("Received packet from unknown host %d\n", node));

	switch (netbuffer->packettype)
	{
		case PT_ASKINFOVIAMS   : PT_AskInfoViaMS   (node    ); break;
		case PT_SERVERINFO     : PT_ServerInfo     (node    ); break;
		case PT_TELLFILESNEEDED: PT_TellFilesNeeded(node    ); break;
		case PT_MOREFILESNEEDED: PT_MoreFilesNeeded(node    ); break;
		case PT_ASKINFO        : PT_AskInfo        (node    ); break;
		case PT_SERVERREFUSE   : PT_ServerRefuse   (node    ); break;
		case PT_SERVERCFG      : PT_ServerCFG      (node    ); break;
		case PT_FILEFRAGMENT   : PT_FileFragment   (node, -1); break;
		case PT_FILEACK        : PT_FileAck        (node    ); break;
		case PT_FILERECEIVED   : PT_FileReceived   (node    ); break;
		case PT_REQUESTFILE    : PT_RequestFile    (node    ); break;
		case PT_CLIENTQUIT     : PT_ClientQuit     (node, -1); break;
		case PT_SERVERTICS     : PT_ServerTics     (node, -1); break;
		case PT_CLIENTJOIN     : PT_ClientJoin     (node    ); break;
		case PT_SERVERSHUTDOWN : PT_ServerShutdown (node    ); break;
		case PT_CLIENTCMD      :                               break; // This is not an "unknown packet"
		case PT_PLAYERINFO     :                               break; // This is not an "unknown packet"

		default:
			DEBFILE(va("unknown packet received (%d) from unknown host\n",netbuffer->packettype));
			Net_CloseConnection(node);
	}
}

/** Handles a packet received from a node that is in game
  *
  * \param node The packet sender
  * \todo Choose a better name
  * \sa HandlePacketFromAwayNode
  * \sa GetPackets
  *
  */
static void HandlePacketFromPlayer(SINT8 node)
{
	INT32 netconsole;

	if (dedicated && node == 0)
		netconsole = 0;
	else
		netconsole = netnodes[node].player;

#ifdef PARANOIA
	if (netconsole >= MAXPLAYERS)
		I_Error("bad table nodetoplayer: node %d player %d", doomcom->remotenode, netconsole);
#endif

	switch (netbuffer->packettype)
	{
		// SERVER RECEIVE
		case PT_CLIENTCMD:
		case PT_CLIENT2CMD:
		case PT_CLIENTMIS:
		case PT_CLIENT2MIS:
		case PT_NODEKEEPALIVE:
		case PT_NODEKEEPALIVEMIS:
			PT_ClientCmd(node, netconsole);
			break;
		case PT_BASICKEEPALIVE     : PT_BasicKeepAlive     (node, netconsole); break;
		case PT_TEXTCMD            : PT_TextCmd            (node, netconsole); break;
		case PT_TEXTCMD2           : PT_TextCmd            (node, netconsole); break;
		case PT_LOGIN              : PT_Login              (node, netconsole); break;
		case PT_CLIENTQUIT         : PT_ClientQuit         (node, netconsole); break;
		case PT_CANRECEIVEGAMESTATE: PT_CanReceiveGamestate(node            ); break;
		case PT_ASKLUAFILE         : PT_AskLuaFile         (node            ); break;
		case PT_HASLUAFILE         : PT_HasLuaFile         (node            ); break;
		case PT_RECEIVEDGAMESTATE  : PT_ReceivedGamestate  (node            ); break;
		case PT_SERVERINFO         : PT_ServerInfo         (node            ); break;

		// CLIENT RECEIVE
		case PT_SERVERTICS         : PT_ServerTics         (node, netconsole); break;
		case PT_PING               : PT_Ping               (node, netconsole); break;
		case PT_FILEFRAGMENT       : PT_FileFragment       (node, netconsole); break;
		case PT_FILEACK            : PT_FileAck            (node            ); break;
		case PT_FILERECEIVED       : PT_FileReceived       (node            ); break;
		case PT_WILLRESENDGAMESTATE: PT_WillResendGamestate(node            ); break;
		case PT_SENDINGLUAFILE     : PT_SendingLuaFile     (node            ); break;
		case PT_SERVERSHUTDOWN     : PT_ServerShutdown     (node            ); break;
		case PT_SERVERCFG          :                                           break;
		case PT_CLIENTJOIN         :                                           break;

		default:
			DEBFILE(va("UNKNOWN PACKET TYPE RECEIVED %d from host %d\n",
				netbuffer->packettype, node));
	}
}

/**	Handles all received packets, if any
  *
  * \todo Add details to this description (lol)
  *
  */
void GetPackets(void)
{
	while (HGetPacket())
	{
		SINT8 node = doomcom->remotenode;

		// Packet received from someone already playing
		if (netnodes[node].ingame)
			HandlePacketFromPlayer(node);
		// Packet received from someone not playing
		else
			HandlePacketFromAwayNode(node);
	}
}

boolean TryRunTics(tic_t realtics)
{
	// the machine has lagged but it is not so bad
	if (realtics > TICRATE/7)
	{
		if (server)
			realtics = 1;
		else
			realtics = TICRATE/7;
	}

	if (singletics)
		realtics = 1;

	if (realtics >= 1)
	{
		COM_BufTicker();
		if (mapchangepending)
			D_MapChange(-1, 0, ultimatemode, false, 2, false, fromlevelselect); // finish the map change
	}

	NetUpdate();

	if (demoplayback)
	{
		neededtic = gametic + realtics;
		// start a game after a demo
		maketic += realtics;
		firstticstosend = maketic;
		tictoclear = firstticstosend;
	}

	GetPackets();

#ifdef DEBUGFILE
	if (debugfile && (realtics || neededtic > gametic))
	{
		fprintf(debugfile, "------------ Tryruntic: REAL:%d NEED:%d GAME:%d LOAD: %d\n",
			realtics, neededtic, gametic, debugload);
		debugload = 100000;
	}
#endif

	if (neededtic > gametic)
	{
		if (realtics)
			hu_stopped = false;

		if (advancedemo)
		{
			if (timedemo_quit)
				COM_ImmedExecute("quit");
			else
				D_StartTitle();
		}
		else
			// run the count * tics
			while (neededtic > gametic)
			{
				boolean update_stats = !(paused || P_AutoPause());

				DEBFILE(va("============ Running tic %d (local %d)\n", gametic, localgametic));

				if (update_stats)
					PS_START_TIMING(ps_tictime);

				G_Ticker((gametic % NEWTICRATERATIO) == 0);
				ExtraDataTicker();
				gametic++;
				consistancy[gametic%BACKUPTICS] = Consistancy();

				if (update_stats)
				{
					PS_STOP_TIMING(ps_tictime);
					PS_UpdateTickStats();
				}

				// Leave a certain amount of tics present in the net buffer as long as we've ran at least one tic this frame.
				if (client && gamestate == GS_LEVEL && leveltime > 3 && neededtic <= gametic + cv_netticbuffer.value)
					break;
			}

		return true;
	}
	else
	{
		if (realtics)
			hu_stopped = true;

		return false;
	}
}

static void UpdatePingTable(void)
{
	if (server)
	{
		if (netgame && !(gametime % 35)) // update once per second.
			PingUpdate();
		// update node latency values so we can take an average later.
		for (INT32 i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && playernode[i] != UINT8_MAX)
				realpingtable[i] += G_TicsToMilliseconds(GetLag(playernode[i]));
		pingmeasurecount++;
	}
}

// Handle idle and disconnected player timers
static void IdleUpdate(void)
{
	INT32 i;
	if (!server || !netgame)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && playernode[i] != UINT8_MAX && !players[i].quittime && !players[i].spectator && !players[i].bot && gamestate == GS_LEVEL)
		{
			if (players[i].cmd.forwardmove || players[i].cmd.sidemove || players[i].cmd.buttons)
				players[i].lastinputtime = 0;
			else
				players[i].lastinputtime++;

			if (cv_idletime.value && !IsPlayerAdmin(i) && i != serverplayer && !(players[i].pflags & PF_FINISHED) && players[i].lastinputtime > (tic_t)cv_idletime.value * TICRATE * 60)
			{
				players[i].lastinputtime = 0;
				if (cv_idlespectate.value && G_GametypeHasSpectators())
				{
					changeteam_union NetPacket;
					UINT16 usvalue;
					NetPacket.value.l = NetPacket.value.b = 0;
					NetPacket.packet.newteam = 0;
					NetPacket.packet.playernum = i;
					NetPacket.packet.verification = true; // This signals that it's a server change
					usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
					SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
				}
				else
				{
					SendKick(i, KICK_MSG_IDLE | KICK_MSG_KEEP_BODY);
				}
			}
		}
		else
		{
			players[i].lastinputtime = 0;

			if (players[i].quittime && playeringame[i])
			{
				players[i].quittime++;

				if (players[i].quittime == 30 * TICRATE && G_TagGametype())
					P_CheckSurvivors();

				if (server && players[i].quittime >= (tic_t)FixedMul(cv_rejointimeout.value, 60 * TICRATE)
				&& !(players[i].quittime % TICRATE))
				{
					if (D_NumNodes(true) > 0)
						SendKick(i, KICK_MSG_PLAYER_QUIT);
					else // If the server is empty, don't send a NetXCmd - that would wake an idling dedicated server
						CL_RemovePlayer(i, KICK_MSG_PLAYER_QUIT);
				}
			}
		}
	}
}

// Handle timeouts to prevent definitive freezes from happenning
static void HandleNodeTimeouts(void)
{
	if (server)
	{
		for (INT32 i = 1; i < MAXNETNODES; i++)
			if (netnodes[i].ingame && netnodes[i].freezetimeout < I_GetTime())
				Net_ConnectionTimeout(i);

		// In case the cvar value was lowered
		if (joindelay)
			joindelay = min(joindelay - 1, 3 * (tic_t)cv_joindelay.value * TICRATE);
	}
}

// Keep the network alive while not advancing tics!
void NetKeepAlive(void)
{
	tic_t nowtime;
	INT32 realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	// return if there's no time passed since the last call
	if (realtics <= 0) // nothing new to update
		return;

	UpdatePingTable();

	GetPackets();

	IdleUpdate();

#ifdef MASTERSERVER
	MasterClient_Ticker();
#endif

	if (client)
	{
		// send keep alive
		CL_SendClientKeepAlive();
		// No need to check for resynch because we aren't running any tics
	}
	else
	{
		SV_SendServerKeepAlive();
	}

	// No else because no tics are being run and we can't resynch during this

	Net_AckTicker();
	HandleNodeTimeouts();
	FileSendTicker();
}

void NetUpdate(void)
{
	static tic_t resptime = 0;
	tic_t nowtime;
	INT32 realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	if (realtics <= 0) // nothing new to update
		return;

	if (realtics > 5)
	{
		if (server)
			realtics = 1;
		else
			realtics = 5;
	}

	if (server && dedicated && gamestate == GS_LEVEL)
 	{
		const tic_t dedicatedidletime = cv_dedicatedidletime.value * TICRATE;
		static tic_t dedicatedidletimeprev = 0;
		static tic_t dedicatedidle = 0;

		if (dedicatedidletime > 0)
		{
			INT32 i;

			for (i = 1; i < MAXNETNODES; ++i)
				if (netnodes[i].ingame)
				{
					if (dedicatedidle >= dedicatedidletime)
					{
						CONS_Printf("DEDICATED: Awakening from idle (Node %d detected...)\n", i);
						dedicatedidle = 0;
					}
					break;
				}

			if (i == MAXNETNODES)
			{
				if (leveltime == 2)
				{
					// On next tick...
					dedicatedidle = dedicatedidletime-1;
				}
				else if (dedicatedidle >= dedicatedidletime)
				{
					if (D_GetExistingTextcmd(gametic, 0) || D_GetExistingTextcmd(gametic+1, 0))
					{
						CONS_Printf("DEDICATED: Awakening from idle (Netxcmd detected...)\n");
						dedicatedidle = 0;
					}
					else
					{
						realtics = 0;
					}
				}
				else if ((dedicatedidle += realtics) >= dedicatedidletime)
				{
					const char *idlereason = "at round start";
					if (leveltime > 3)
						idlereason = va("for %d seconds", dedicatedidle/TICRATE);

					CONS_Printf("DEDICATED: No nodes %s, idling...\n", idlereason);
					realtics = 0;
					dedicatedidle = dedicatedidletime;
				}
			}
		}
		else
		{
			if (dedicatedidletimeprev > 0 && dedicatedidle >= dedicatedidletimeprev)
			{
				CONS_Printf("DEDICATED: Awakening from idle (Idle disabled...)\n");
			}
			dedicatedidle = 0;
		}

		dedicatedidletimeprev = dedicatedidletime;
 	}

	gametime = nowtime;

	UpdatePingTable();

	if (client)
		maketic = neededtic;

	Local_Maketic(realtics);

	if (server)
		CL_SendClientCmd(); // send it

	GetPackets(); // get packet from client or from server

	IdleUpdate();

	// The client sends the command after receiving from the server
	// The server sends it before because this is better in single player

#ifdef MASTERSERVER
	MasterClient_Ticker(); // Acking the Master Server
#endif

	if (client)
	{
		// If the client just finished redownloading the game state, load it
		if (cl_redownloadinggamestate && fileneeded[0].status == FS_FOUND)
			CL_ReloadReceivedSavegame();

		CL_SendClientCmd(); // Send tic cmd
		hu_redownloadinggamestate = cl_redownloadinggamestate;
	}
	else
	{
		if (!demoplayback && realtics > 0)
		{
			hu_redownloadinggamestate = false;

			firstticstosend = gametic;
			for (INT32 i = 0; i < MAXNETNODES; i++)
				if (netnodes[i].ingame)
				{
					if (netnodes[i].tic < firstticstosend)
						firstticstosend = netnodes[i].tic;

					if (maketic + realtics >= netnodes[i].tic + BACKUPTICS - TICRATE)
						Net_ConnectionTimeout(i);
				}

			// Don't erase tics not acknowledged
			INT32 counts = realtics;
			if (maketic + counts >= firstticstosend + BACKUPTICS)
				counts = firstticstosend+BACKUPTICS-maketic-1;

			for (INT32 i = 0; i < counts; i++)
				SV_Maketic(); // Create missed tics and increment maketic

			for (; tictoclear < firstticstosend; tictoclear++) // Clear only when acknowledged
				D_Clearticcmd(tictoclear);                    // Clear the maketic the new tic

			SV_SendTics();

			neededtic = maketic; // The server is a client too
		}
	}

	Net_AckTicker();
	HandleNodeTimeouts();

	nowtime /= NEWTICRATERATIO;

	if (nowtime != resptime)
	{
		resptime = nowtime;
#ifdef HAVE_THREADS
		I_lock_mutex(&m_menu_mutex);
#endif
		M_Ticker();
#ifdef HAVE_THREADS
		I_unlock_mutex(m_menu_mutex);
#endif
		CON_Ticker();
	}

	FileSendTicker();
}

// called one time at init
void D_ClientServerInit(void)
{
	DEBFILE(va("- - -== SRB2 v%d.%.2d.%d "VERSIONSTRING" debugfile ==- - -\n",
		VERSION/100, VERSION%100, SUBVERSION));

	COM_AddCommand("getplayernum", Command_GetPlayerNum, COM_LUA);
	COM_AddCommand("kick", Command_Kick, COM_LUA);
	COM_AddCommand("ban", Command_Ban, COM_LUA);
	COM_AddCommand("banip", Command_BanIP, COM_LUA);
	COM_AddCommand("clearbans", Command_ClearBans, COM_LUA);
	COM_AddCommand("showbanlist", Command_ShowBan, COM_LUA);
	COM_AddCommand("reloadbans", Command_ReloadBan, COM_LUA);
	COM_AddCommand("connect", Command_connect, COM_LUA);
	COM_AddCommand("nodes", Command_Nodes, COM_LUA);
	COM_AddCommand("set_http_login", Command_set_http_login, 0);
	COM_AddCommand("list_http_logins", Command_list_http_logins, 0);
	COM_AddCommand("resendgamestate", Command_ResendGamestate, COM_LUA);
#ifdef PACKETDROP
	COM_AddCommand("drop", Command_Drop, COM_LUA);
	COM_AddCommand("droprate", Command_Droprate, COM_LUA);
#endif
#ifdef _DEBUG
	COM_AddCommand("numnodes", Command_Numnodes, COM_LUA);
#endif

	RegisterNetXCmd(XD_KICK, Got_KickCmd);
	RegisterNetXCmd(XD_ADDPLAYER, Got_AddPlayer);
#ifdef DUMPCONSISTENCY
	CV_RegisterVar(&cv_dumpconsistency);
#endif
	Ban_Load_File(false);

	gametic = 0;
	localgametic = 0;

	// do not send anything before the real begin
	SV_StopServer();
	SV_ResetServer();
	if (dedicated)
		SV_SpawnServer();
}

SINT8 nametonum(const char *name)
{
	INT32 playernum;

	if (!strcmp(name, "0"))
		return 0;

	playernum = (SINT8)atoi(name);

	if (playernum < 0 || playernum >= MAXPLAYERS)
		return -1;

	if (playernum)
	{
		if (playeringame[playernum])
			return (SINT8)playernum;
		else
			return -1;
	}

	for (INT32 i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && !stricmp(player_names[i], name))
			return (SINT8)i;

	CONS_Printf(M_GetText("There is no player named \"%s\"\n"), name);

	return -1;
}

// Is there a game running?
boolean Playing(void)
{
	return (server && serverrunning) || (client && cl_mode == CL_CONNECTED);
}

/** Returns the number of players playing.
  * \return Number of players. Can be zero if we're running a ::dedicated
  *         server.
  * \author Graue <graue@oceanbase.org>
  */
INT32 D_NumPlayers(void)
{
	INT32 num = 0;
	for (INT32 ix = 0; ix < MAXPLAYERS; ix++)
		if (playeringame[ix])
			num++;
	return num;
}

/** Returns the number of currently-connected nodes in a netgame.
  * Not necessarily equivalent to D_NumPlayers() minus D_NumBots().
  *
  * \param skiphost Skip the server's own node.
  */
INT32 D_NumNodes(boolean skiphost)
{
	INT32 num = 0;
	for (INT32 ix = skiphost ? 1 : 0; ix < MAXNETNODES; ix++)
		if (netnodes[ix].ingame)
			num++;
	return num;
}

/** Similar to the above, but counts only bots.
  * Purpose is to remove bots from both the player count and the
  * max player count on the server view
*/
INT32 D_NumBots(void)
{
	INT32 num = 0, ix;
	for (ix = 0; ix < MAXPLAYERS; ix++)
		if (playeringame[ix] && players[ix].bot)
			num++;
	return num;
}


//
// Consistancy
//
// Note: It is called consistAncy on purpose.
//
INT16 Consistancy(void)
{
	UINT32 ret = 0;
#ifdef MOBJCONSISTANCY
	thinker_t *th;
	mobj_t *mo;
#endif

	DEBFILE(va("TIC %u ", gametic));

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			ret ^= 0xCCCC;
		else if (!players[i].mo);
		else
		{
			ret += players[i].mo->x;
			ret -= players[i].mo->y;
			ret += players[i].powers[pw_shield];
			ret *= i+1;
		}
	}
	// I give up
	// Coop desynching enemies is painful
	if (!G_PlatformGametype())
		ret += P_GetRandSeed();

#ifdef MOBJCONSISTANCY
	if (gamestate == GS_LEVEL)
	{
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo = (mobj_t *)th;

			if (mo->flags & (MF_SPECIAL | MF_SOLID | MF_PUSHABLE | MF_BOSS | MF_MISSILE | MF_SPRING | MF_MONITOR | MF_FIRE | MF_ENEMY | MF_PAIN | MF_STICKY))
			{
				ret -= mo->type;
				ret += mo->x;
				ret -= mo->y;
				ret += mo->z;
				ret -= mo->momx;
				ret += mo->momy;
				ret -= mo->momz;
				ret += mo->angle;
				ret -= mo->flags;
				ret += mo->flags2;
				ret -= mo->eflags;
				if (mo->target)
				{
					ret += mo->target->type;
					ret -= mo->target->x;
					ret += mo->target->y;
					ret -= mo->target->z;
					ret += mo->target->momx;
					ret -= mo->target->momy;
					ret += mo->target->momz;
					ret -= mo->target->angle;
					ret += mo->target->flags;
					ret -= mo->target->flags2;
					ret += mo->target->eflags;
					ret -= mo->target->state - states;
					ret += mo->target->tics;
					ret -= mo->target->sprite;
					ret += mo->target->frame;
				}
				else
					ret ^= 0x3333;
				if (mo->tracer && mo->tracer->type != MT_OVERLAY)
				{
					ret += mo->tracer->type;
					ret -= mo->tracer->x;
					ret += mo->tracer->y;
					ret -= mo->tracer->z;
					ret += mo->tracer->momx;
					ret -= mo->tracer->momy;
					ret += mo->tracer->momz;
					ret -= mo->tracer->angle;
					ret += mo->tracer->flags;
					ret -= mo->tracer->flags2;
					ret += mo->tracer->eflags;
					ret -= mo->tracer->state - states;
					ret += mo->tracer->tics;
					ret -= mo->tracer->sprite;
					ret += mo->tracer->frame;
				}
				else
					ret ^= 0xAAAA;
				ret -= mo->state - states;
				ret += mo->tics;
				ret -= mo->sprite;
				ret += mo->frame;
			}
		}
	}
#endif

	DEBFILE(va("Consistancy = %u\n", (ret & 0xFFFF)));

	return (INT16)(ret & 0xFFFF);
}

tic_t GetLag(INT32 node)
{
	return gametic - netnodes[node].tic;
}

void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest)
{
#ifdef NOMD5
	(void)buffer;
	(void)len;
	(void)salt;
	memset(dest, 0, 16);
#else
	char tmpbuf[256];
	const size_t sl = strlen(salt);

	if (len > 256-sl)
		len = 256-sl;

	memcpy(tmpbuf, buffer, len);
	memmove(&tmpbuf[len], salt, sl);
	//strcpy(&tmpbuf[len], salt);
	len += strlen(salt);
	if (len < 256)
		memset(&tmpbuf[len],0,256-len);

	// Yes, we intentionally md5 the ENTIRE buffer regardless of size...
	md5_buffer(tmpbuf, 256, dest);
#endif
}
