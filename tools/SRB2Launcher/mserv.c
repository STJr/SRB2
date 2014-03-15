// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// MSERV SDK
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Adapted by Oogaland.
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
//
//
// DESCRIPTION:
//		Commands used to communicate with the master server
//
//-----------------------------------------------------------------------------


#ifdef WIN32
#include <windows.h>	 // socket(),...
#else
#include <unistd.h>
#endif




#include "launcher.h"

#include "mserv.h"
#include "i_tcp.h"





// ================================ DEFINITIONS ===============================

#define	PACKET_SIZE 1024

#define  MS_NO_ERROR				   0
#define  MS_SOCKET_ERROR			-201
#define  MS_CONNECT_ERROR			-203
#define  MS_WRITE_ERROR 			-210
#define  MS_READ_ERROR				-211
#define  MS_CLOSE_ERROR 			-212
#define  MS_GETHOSTBYNAME_ERROR 	-220
#define  MS_GETHOSTNAME_ERROR		-221
#define  MS_TIMEOUT_ERROR			-231

// see master server code for the values
#define GET_SERVER_MSG				 200


#define HEADER_SIZE ((long)sizeof(long)*3)

#define HEADER_MSG_POS		0
#define IP_MSG_POS		   16
#define PORT_MSG_POS	   32
#define HOSTNAME_MSG_POS   40

#ifndef SOCKET
#define SOCKET int
#endif

typedef struct {
	long	id;
	long	type;
	long	length;
	char	buffer[PACKET_SIZE];
} msg_t;


// win32 or djgpp
#if defined( WIN32) || defined( __DJGPP__ )
#define ioctl ioctlsocket
#define close closesocket
#endif

#if defined( WIN32) || defined( __OS2__)
// it seems windows doesn't define that... maybe some other OS? OS/2
int inet_aton(char *hostname, struct in_addr *addr)
{
	return ( (addr->s_addr=inet_addr(hostname)) != INADDR_NONE );
}	
#endif



enum { MSCS_NONE, MSCS_WAITING, MSCS_REGISTERED, MSCS_FAILED } con_state = MSCS_NONE;


static SOCKET				socket_fd = -1;  // TCP/IP socket
static struct sockaddr_in	addr;
static struct timeval		select_timeout;
static fd_set				wset;

int	MS_Connect(char *ip_addr, char *str_port, int async);
static int	MS_Read(msg_t *msg);
static int	MS_Write(msg_t *msg);
static int	MS_GetIP(char *);

void ExtractServerInfo(char *serverout, struct SERVERLIST *serverlist);










void CloseConnection(void)
{
	if(socket_fd > 0) close(socket_fd);
	socket_fd = -1;
}




/*
** MS_GetIP()
*/
static int MS_GetIP(char *hostname)
{
	struct hostent *host_ent;

	if (!inet_aton(hostname, &addr.sin_addr)) {
		//TODO: only when we are connected to Internet, or use a non bloking call
		host_ent = gethostbyname(hostname);
		if (host_ent==NULL)
			return MS_GETHOSTBYNAME_ERROR;
		memcpy(&addr.sin_addr, host_ent->h_addr_list[0], sizeof(struct in_addr));
	}
	return 0;
}






/*
** MS_Connect()
*/
int MS_Connect(char *ip_addr, char *str_port, int async)
{
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	I_InitTcpDriver(); // this is done only if not already done

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return MS_SOCKET_ERROR;

	if (MS_GetIP(ip_addr)==MS_GETHOSTBYNAME_ERROR)
		return MS_GETHOSTBYNAME_ERROR;
	addr.sin_port = htons((u_short)atoi(str_port));

	if (async) // do asynchronous connection
	{
		int res = 1;

		ioctl(socket_fd, FIONBIO, &res);
		res = connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr));
		if (res < 0)
		{
			// humm, on win32 it doesn't work with EINPROGRESS (stupid windows)
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				con_state = MSCS_FAILED;
				CloseConnection();
				return MS_CONNECT_ERROR;
			}
		}
		con_state = MSCS_WAITING;
		FD_ZERO(&wset);
		FD_SET(socket_fd, &wset);
		select_timeout.tv_sec = 0, select_timeout.tv_usec = 0;
	}
	else
	{
		if (connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
			return MS_CONNECT_ERROR;
	}

	return 0;
}






/*
 * MS_Write():
 */
static int MS_Write(msg_t *msg)
{
	int len;

	if (msg->length < 0)
		msg->length = strlen(msg->buffer);
	len = msg->length+HEADER_SIZE;

	//msg->id = htonl(msg->id);
	msg->type = htonl(msg->type);
	msg->length = htonl(msg->length);

	if (send(socket_fd, (char*)msg, len, 0) != len)
		return MS_WRITE_ERROR;

	return 0;
}






/*
 * MS_Read():
 */
static int MS_Read(msg_t *msg)
{
	if (recv(socket_fd, (char*)msg, HEADER_SIZE, 0) != HEADER_SIZE)
		return MS_READ_ERROR;

	msg->type = ntohl(msg->type);
	msg->length = ntohl(msg->length);

	if (!msg->length) //Hurdler: fix a bug in Windows 2000
		return 0;

	if (recv(socket_fd, (char*)msg->buffer, msg->length, 0) != msg->length)
		return MS_READ_ERROR;

	return 0;
}








/***************************************************************************/









/* GetServerListEx */
EXPORT int __stdcall GetServerListEx(char *host, char *str_port, struct SERVERLIST serverlist[], short max_servers)
{
	msg_t				msg;
	int					count = 0;


	/* Attempt to connect to list server. */
	MS_Connect(host, str_port, 0);

	/* Poll the list server. If it fails, depart with an error code of -1. */
	msg.type = GET_SERVER_MSG;
	msg.length = 0;
	if (MS_Write(&msg) < 0)
		return -1;



	/* Get a description of each server in turn. */
	/* What we get is exactly the same as the output to the console when using LISTSERV. */
	while (MS_Read(&msg) >= 0)
	{
		if(msg.length == 0 || count >= max_servers)
		{
			CloseConnection();
			return count;
		}
		
		ExtractServerInfo(msg.buffer, &serverlist[count]);

		count++;
	}


	CloseConnection();
	return -2;
}











/* GetServerList */
/* Warning: Large kludge follows! This function is only included for backwards-compatibility. */
/* Use GetServerListVB or GetServerListEx instead. */
EXPORT int __stdcall GetServerList(char *host, char *str_port,

						 struct SERVERLIST *serverlist1,struct SERVERLIST *serverlist2,struct SERVERLIST *serverlist3,
						 struct SERVERLIST *serverlist4,struct SERVERLIST *serverlist5,struct SERVERLIST *serverlist6,
						 struct SERVERLIST *serverlist7,struct SERVERLIST *serverlist8,struct SERVERLIST *serverlist9,
						 struct SERVERLIST *serverlist10,struct SERVERLIST *serverlist11,struct SERVERLIST *serverlist12,
						 struct SERVERLIST *serverlist13,struct SERVERLIST *serverlist14,struct SERVERLIST *serverlist15,
						 struct SERVERLIST *serverlist16)
{
	msg_t	msg;
	int 	count = 0;
	struct SERVERLIST *serverlist[16];


	/* Attempt to connect to list server. */
	MS_Connect(host, str_port, 0);

	/* Poll the list server. If it fails, bomb with an error code of -1. */
	msg.type = GET_SERVER_MSG;
	msg.length = 0;
	if (MS_Write(&msg) < 0)
		return -1;

	serverlist[0] = serverlist1;
	serverlist[1] = serverlist2;
	serverlist[2] = serverlist3;
	serverlist[3] = serverlist4;
	serverlist[4] = serverlist5;
	serverlist[5] = serverlist6;
	serverlist[6] = serverlist7;
	serverlist[7] = serverlist8;
	serverlist[8] = serverlist9;
	serverlist[9] = serverlist10;
	serverlist[10] = serverlist11;
	serverlist[11] = serverlist12;
	serverlist[12] = serverlist13;
	serverlist[13] = serverlist14;
	serverlist[14] = serverlist15;
	serverlist[15] = serverlist16;
	



	while (MS_Read(&msg) >= 0 && count < 16)
	{
		if(msg.length == 0 || count >= 16)
		{
			CloseConnection();
			return count;
		}
		
		ExtractServerInfo(msg.buffer, serverlist[count]);

		count++;
	}


	CloseConnection();


	return -2;
}



void ExtractServerInfo(char *serverout, struct SERVERLIST *serverlist)
{
	char *lines[5];
	int i;

	i=0;
	lines[0] = strtok(serverout, "\r\n");
	for(i=1; i<5; i++)
	{
		lines[i] = strtok(NULL, "\r\n");
	}
	
	strcpy(serverlist->ip, strstr(lines[0], ": ")+2);
	strcpy(serverlist->port, strstr(lines[1], ": ")+2);
	strcpy(serverlist->name, strstr(lines[2], ": ")+2);
	strcpy(serverlist->version, strstr(lines[3], ": ")+2);
	strcpy(serverlist->perm, strstr(lines[4], ": ")+2);
}
