// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
// Copyright (C)      2020 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mserv.c
/// \brief Commands used to communicate with the master server

#if !defined (UNDER_CE)
#include <time.h>
#endif

#include "doomstat.h"
#include "doomdef.h"
#include "command.h"
#include "mserv.h"
#include "m_menu.h"
#include "z_zone.h"

/* HTTP */
int  HMS_in_use (void);
int  HMS_fetch_rooms (int joining);
int  HMS_register (void);
void HMS_unlist (void);
int  HMS_update (void);
void HMS_list_servers (void);
int  HMS_fetch_servers (msg_server_t *list, int room);
int  HMS_compare_mod_version (char *buffer, size_t size_of_buffer);

static time_t MSLastPing;

#ifndef NONET
static void Command_Listserv_f(void);
#endif
static void MasterServer_OnChange(void);
static void ServerName_OnChange(void);

consvar_t cv_masterserver = {"masterserver", "https://mb.srb2.org/MS/0", CV_SAVE|CV_CALL, NULL, MasterServer_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_servername = {"servername", "SRB2Kart server", CV_SAVE|CV_CALL|CV_NOINIT, NULL, ServerName_OnChange, 0, NULL, NULL, 0, 0, NULL};

char *ms_API;
INT16 ms_RoomId = -1;

static enum { MSCS_NONE, MSCS_WAITING, MSCS_REGISTERED, MSCS_FAILED } con_state = MSCS_NONE;

UINT16 current_port = 0;

// Room list is an external variable now.
// Avoiding having to get info ten thousand times...
msg_rooms_t room_list[NUM_LIST_ROOMS+1]; // +1 for easy test

/** Adds variables and commands relating to the master server.
  *
  * \sa cv_masterserver, cv_servername,
  *     Command_Listserv_f
  */
void AddMServCommands(void)
{
#ifndef NONET
	CV_RegisterVar(&cv_masterserver);
	CV_RegisterVar(&cv_masterserver_debug);
	CV_RegisterVar(&cv_servername);
	COM_AddCommand("listserv", Command_Listserv_f);
#endif
}

static void WarnGUI (void)
{
	M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n\nCheck the console for details.\n"), NULL, MM_NOTHING);
}

#define NUM_LIST_SERVER MAXSERVERLIST
const msg_server_t *GetShortServersList(INT32 room)
{
	static msg_server_t server_list[NUM_LIST_SERVER+1]; // +1 for easy test

	if (HMS_fetch_servers(server_list, room))
		return server_list;
	else
	{
		WarnGUI();
		return NULL;
	}
}

INT32 GetRoomsList(boolean hosting)
{
	if (HMS_fetch_rooms( ! hosting ))
		return 1;
	else
	{
		WarnGUI();
		return -1;
	}
}

#ifdef UPDATE_ALERT
const char *GetMODVersion(void)
{
	static char buffer[16];
	int c;

	c = HMS_compare_mod_version(buffer, sizeof buffer);

	if (c > 0)
		return buffer;
	else
	{
		if (! c)
			WarnGUI();

		return NULL;
	}
}

// Console only version of the above (used before game init)
void GetMODVersion_Console(void)
{
	char buffer[16];

	if (HMS_compare_mod_version(buffer, sizeof buffer) > 0)
		I_Error(UPDATE_ALERT_STRING_CONSOLE, VERSIONSTRING, buffer);
}
#endif

#ifndef NONET
/** Gets a list of game servers. Called from console.
  */
static void Command_Listserv_f(void)
{
	CONS_Printf(M_GetText("Retrieving server list...\n"));

	{
		HMS_list_servers();
	}
}
#endif

void RegisterServer(void)
{
	CONS_Printf(M_GetText("Registering this server on the Master Server...\n"));

	{
		if (HMS_register())
			con_state = MSCS_REGISTERED;
		else
			con_state = MSCS_FAILED;
	}

	time(&MSLastPing);
}

static void UpdateServer(void)
{
	if (( con_state == MSCS_REGISTERED && HMS_update() ))
	{
		time(&MSLastPing);
	}
	else
	{
		con_state = MSCS_FAILED;
		RegisterServer();
	}
}

static inline void SendPingToMasterServer(void)
{
// Here, have a simpler MS Ping... - Cue
	if(time(NULL) > (MSLastPing+(60*2)) && con_state != MSCS_NONE)
	{
		UpdateServer();
	}
}

void UnregisterServer(void)
{
	if (con_state == MSCS_REGISTERED)
	{
		CONS_Printf(M_GetText("Removing this server from the Master Server...\n"));

		HMS_unlist();
	}

	con_state = MSCS_NONE;
}

void MasterClient_Ticker(void)
{
	if (server && ms_RoomId > 0)
		SendPingToMasterServer();
}

static void ServerName_OnChange(void)
{
	if (con_state == MSCS_REGISTERED)
		UpdateServer();
}

static void MasterServer_OnChange(void)
{
	boolean auto_register;

	auto_register = ( con_state != MSCS_NONE );

	if (ms_API)
	{
		UnregisterServer();
		Z_Free(ms_API);
	}

	ms_API = Z_StrDup(cv_masterserver.string);

	if (auto_register)
		RegisterServer();
}
