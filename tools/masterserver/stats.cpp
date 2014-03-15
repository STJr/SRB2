// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

#ifdef __GNUC__
#include <unistd.h>
#endif
#include <string.h>      // strcat(),...
#include "common.h"
#include "ipcs.h"
#include "stats.h"

//=============================================================================

/*
** CServerStats()
*/
CServerStats::CServerStats()
{
	uptime = time(NULL);
	num_connections = 0;
	num_http_con = 0;
	num_servers = 0;
	snprintf(motd, sizeof motd, "%s%s", "Welcome to the SRB2 Master Server!<br>", SERVERMOTDTEXT);
	num_add = 0;
	num_removal = 0;
	num_retrieval = 0;
	num_autoremoval = 0;
	num_badconnection = 0;
	for (int i = 0; i < NUMLASTSERVERS; i++)
	{
		strcpy(last_server[i].ip, "0.0.0.0");
		strcpy(last_server[i].name, "Non-Existing SRB2 server");
		strcpy(last_server[i].version, "0.0.0");
	}
	snprintf(version, sizeof version, "%s %s", __DATE__, __TIME__);
}

/*
** ~CServerStats()
*/
CServerStats::~CServerStats()
{
}

/*
** getUptime()
*/
const char *CServerStats::getUptime()
{
	char *res = ctime(&uptime);
	res[strlen(res)-1] = '\0'; // remove the '\n' at the end
	return res;
}

/*
** getHours()
*/
int CServerStats::getHours()
{
	return (int)(((time(NULL) - uptime)/(60*60))%24);
}

/*
** getDays()
*/
int CServerStats::getDays()
{
	return (int)((time(NULL) - uptime)/(60*60*24));
}

/*
** getMotd()
*/
const char *CServerStats::getMotd()
{
	return motd;
}

/*
** getLastServers()
*/
const char *CServerStats::getLastServers()
{
	static char res[170*NUMLASTSERVERS];

	tzset();
	res[0] = '\0';
	for (int i = 0; i < NUMLASTSERVERS; i++)
	{
		char str[170];
		char *ct;

		ct = ctime(&last_time[i]);
		ct[strlen(ct)-1] = '\0'; // replace \n with a \0
		snprintf(str, sizeof str,
			"Address: %15s   Name: %-31s   v%-7s   (%s %s)\n000043210000",
			last_server[i].ip, last_server[i].name, last_server[i].version, ct, tzname[0]);
		strcat(res, str);
	}

	return res;
}

/*
** getVersion()
*/
const char *CServerStats::getVersion()
{
	return version;
}

/*
** putMotd()
*/
void CServerStats::putMotd(char *motd)
{
	strcpy(this->motd, motd);
}

/*
** putLastServer():
*/
void CServerStats::putLastServer(msg_server_t *server)
{
	for (int i = NUMLASTSERVERS-1; i > 0; i--)
	{
		memcpy(&last_server[i], &last_server[i-1],
			sizeof (msg_server_t));
		last_time[i] = last_time[i-1];
	}
	memcpy(&last_server[0], server, sizeof (msg_server_t));
	last_time[0] = time(NULL);
}
