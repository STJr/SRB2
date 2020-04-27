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
#include "i_threads.h"
#include "mserv.h"
#include "m_menu.h"
#include "z_zone.h"

static time_t MSLastPing;

static inline void SendPingToMasterServer(void);

#ifndef NONET
static void Command_Listserv_f(void);
#endif
static void MasterServer_OnChange(void);
static void ServerName_OnChange(void);

static CV_PossibleValue_t masterserver_update_rate_cons_t[] = {
	{2,  "MIN"},
	{60, "MAX"},
	{0}
};

consvar_t cv_masterserver = {"masterserver", "https://mb.srb2.org/MS/0", CV_SAVE|CV_CALL, NULL, MasterServer_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_servername = {"servername", "SRB2Kart server", CV_SAVE|CV_CALL|CV_NOINIT, NULL, ServerName_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_masterserver_update_rate = {"masterserver_update_rate", "15", CV_SAVE|CV_CALL|CV_NOINIT, masterserver_update_rate_cons_t, SendPingToMasterServer, 0, NULL, NULL, 0, 0, NULL};

char *ms_API;
INT16 ms_RoomId = -1;

#ifdef HAVE_THREADS
int           ms_QueryId;
I_mutex       ms_QueryId_mutex;

msg_server_t *ms_ServerList;
I_mutex       ms_ServerList_mutex;
#endif

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
	CV_RegisterVar(&cv_masterserver_update_rate);
	CV_RegisterVar(&cv_masterserver_debug);
	CV_RegisterVar(&cv_servername);
	COM_AddCommand("listserv", Command_Listserv_f);
#endif
}

static void WarnGUI (void)
{
#ifdef HAVE_THREADS
	I_lock_mutex(&m_menu_mutex);
#endif
	M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n\nCheck the console for details.\n"), NULL, MM_NOTHING);
#ifdef HAVE_THREADS
	I_unlock_mutex(m_menu_mutex);
#endif
}

#define NUM_LIST_SERVER MAXSERVERLIST
msg_server_t *GetShortServersList(INT32 room, int id)
{
	msg_server_t *server_list;

	// +1 for easy test
	server_list = malloc(( NUM_LIST_SERVER + 1 ) * sizeof *server_list);

	if (HMS_fetch_servers(server_list, room, id))
		return server_list;
	else
	{
		free(server_list);
		WarnGUI();
		return NULL;
	}
}

INT32 GetRoomsList(boolean hosting, int id)
{
	if (HMS_fetch_rooms( ! hosting, id))
		return 1;
	else
	{
		WarnGUI();
		return -1;
	}
}

#ifdef UPDATE_ALERT
char *GetMODVersion(int id)
{
	char *buffer;
	int c;

	(void)id;

	buffer = malloc(16);

	c = HMS_compare_mod_version(buffer, 16);

#ifdef HAVE_THREADS
	I_lock_mutex(&ms_QueryId_mutex);
	{
		if (id != ms_QueryId)
			c = -1;
	}
	I_unlock_mutex(ms_QueryId_mutex);
#endif

	if (c > 0)
		return buffer;
	else
	{
		free(buffer);

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
	if(time(NULL) > (MSLastPing+(60*cv_masterserver_update_rate.value)) && con_state != MSCS_NONE)
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
