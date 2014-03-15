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

#include <time.h>
#include <string.h>
#include "ipcs.h"
#include "common.h"

//=============================================================================

static CClientSocket client_socket;
//static msg_t msg;
FILE *logfile;

//=============================================================================

static msg_t *getData(int argc, char *argv[])
{
	static msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;

#if defined (_WIN32)
	strcpy(info->header.buffer, "");
#else
	strcpy(info->header.buffer, getpass("Enter password: "));
#endif

	// default values if one of the param is not entered
	strcpy(info->ip,       "88.86.106.169");
	strcpy(info->port,     "5029");
	strcpy(info->name,     "Cueball's Dedicated Server");
	strcpy(info->version,  "1.09.4");
	msg.type = ADD_PSERVER_MSG;
	msg.length = sizeof (msg_server_t);

	for (int i = 0; i < argc; i++)
	{
		// first: extra parameters (must be after all -param)
		if (!strcasecmp(argv[i], "get"))
		{
			msg.type = GET_SERVER_MSG;
			msg.length = 0;
		}
		else if (!strcasecmp(argv[i], "log"))
		{
			msg.type = GET_LOGFILE_MSG;
			msg.length = sizeof (msg_server_t);
		}
		else if (!strcasecmp(argv[i], "erase"))
		{
			msg.type = ERASE_LOGFILE_MSG;
			msg.length = sizeof (msg_server_t);
		}
		else if (!strcasecmp(argv[i], "remove"))
		{
			msg.type = REMOVE_PSERVER_MSG;
			msg.length = sizeof (msg_server_t);
		}

		if (i >= argc-1)
			continue;

		// second: parameter requiring extra parameters
		if (strcasecmp(argv[i], "-ip") == 0)
			strcpy(info->ip, argv[i+1]);
		else if (strcasecmp(argv[i], "-port") == 0)
			strcpy(info->port, argv[i+1]);
		else if (strcasecmp(argv[i], "-hostname") == 0)
			strcpy(info->name, argv[i+1]);
		else if (strcasecmp(argv[i], "-version") == 0)
			strcpy(info->version, argv[i+1]);
		else if (strcasecmp(argv[i], "-pass") == 0)
			strcpy(info->header.buffer, argv[i+1]);
	}
	return &msg;
}

int main(int argc, char *argv[])
{
	if (argc == 2)
	{
		printf("encrypt: %s\n", pCrypt(argv[1], "04"));
		return 0;
	}
	else if (argc > 2)
	{
		if (client_socket.connect(argv[1], argv[2]) < 0)
		{
			conPrintf(RED, "Connect failed.\n");
			return -1;
		}

		msg_t *msg = getData(argc, argv);

		if (client_socket.write(msg) < 0)
		{
			dbgPrintf(RED, "Write failed.\n");
			return -1;
		}

		switch (ntohl(msg->type))
		{
		case GET_SERVER_MSG:
		case GET_LOGFILE_MSG:
			while (client_socket.read(msg) >= 0)
			{
				if (ntohl(msg->length) == 0)
					break;
				printf("%s", msg->buffer);
			}
			break;

		case REMOVE_PSERVER_MSG:
		default:
			break;
		}
	}
	return 0;
}
