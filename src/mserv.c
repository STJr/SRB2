// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mserv.c
/// \brief Commands used for communicate with the master server

#ifdef __GNUC__
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#endif

#if !defined (UNDER_CE)
#include <time.h>
#endif

#if (defined (NOMD5) || defined (NOMSERV)) && !defined (NONET)
#define NONET
#endif

#ifndef NONET

#ifndef NO_IPV6
#define HAVE_IPV6
#endif

#if (defined (_WIN32) || defined (_WIN32_WCE)) && !defined (_XBOX)
#define RPC_NO_WINDOWS_H
#ifdef HAVE_IPV6
#include <ws2tcpip.h>
#else
#include <winsock.h>     // socket(),...
#endif //!HAVE_IPV6
#else
#ifdef __OS2__
#include <sys/types.h>
#endif // __OS2__

#ifdef HAVE_LWIP
#include <lwip/inet.h>
#include <kos/net.h>
#include <lwip/lwip.h>
#define ioctl lwip_ioctl
#else
#include <arpa/inet.h>
#ifdef __APPLE_CC__
#ifndef _BSD_SOCKLEN_T_
#define _BSD_SOCKLEN_T_
#endif
#endif
#include <sys/socket.h> // socket(),...
#include <netinet/in.h> // sockaddr_in
#ifdef _PS3
#include <net/select.h>
#elif !defined(_arch_dreamcast)
#include <netdb.h> // getaddrinfo(),...
#include <sys/ioctl.h>
#endif
#endif

#ifdef _arch_dreamcast
#include "sdl12/SRB2DC/dchelp.h"
#endif

#include <sys/time.h> // timeval,... (TIMEOUT)
#include <errno.h>
#endif // _WIN32/_WIN32_WCE

#ifdef __OS2__
#include <errno.h>
#endif // __OS2__
#endif // !NONET

#include "doomstat.h"
#include "doomdef.h"
#include "command.h"
#include "i_net.h"
#include "console.h"
#include "mserv.h"
#include "d_net.h"
#include "i_tcp.h"
#include "i_system.h"
#include "byteptr.h"
#include "m_menu.h"
#include "m_argv.h" // Alam is going to kill me <3
#include "m_misc.h" //  GetRevisionString()

#ifdef _WIN32_WCE
#include "sdl12/SRB2CE/cehelp.h"
#endif

#include "i_addrinfo.h"

// ================================ DEFINITIONS ===============================

#define PACKET_SIZE 1024


#define  MS_NO_ERROR            	   0
#define  MS_SOCKET_ERROR        	-201
#define  MS_CONNECT_ERROR       	-203
#define  MS_WRITE_ERROR         	-210
#define  MS_READ_ERROR          	-211
#define  MS_CLOSE_ERROR         	-212
#define  MS_GETHOSTBYNAME_ERROR 	-220
#define  MS_GETHOSTNAME_ERROR   	-221
#define  MS_TIMEOUT_ERROR       	-231

// see master server code for the values
#define ADD_SERVER_MSG          	101
#define REMOVE_SERVER_MSG       	103
#define ADD_SERVERv2_MSG        	104
#define GET_SERVER_MSG          	200
#define GET_SHORT_SERVER_MSG    	205
#define ASK_SERVER_MSG          	206
#define ANSWER_ASK_SERVER_MSG   	207
#define ASK_SERVER_MSG          	206
#define ANSWER_ASK_SERVER_MSG   	207
#define GET_MOTD_MSG            	208
#define SEND_MOTD_MSG           	209
#define GET_ROOMS_MSG           	210
#define SEND_ROOMS_MSG          	211
#define GET_ROOMS_HOST_MSG      	212
#define GET_VERSION_MSG         	213
#define SEND_VERSION_MSG        	214
#define GET_BANNED_MSG          	215 // Someone's been baaaaaad!
#define PING_SERVER_MSG         	216

#define HEADER_SIZE (sizeof (INT32)*4)

#define HEADER_MSG_POS    0
#define IP_MSG_POS       16
#define PORT_MSG_POS     32
#define HOSTNAME_MSG_POS 40


#if defined(_MSC_VER)
#pragma pack(1)
#endif

/** A message to be exchanged with the master server.
  */
typedef struct
{
	INT32 id;                  ///< Unused?
	INT32 type;                ///< Type of message.
	INT32 room;                ///< Because everyone needs a roomie.
	UINT32 length;             ///< Length of the message.
	char buffer[PACKET_SIZE]; ///< Actual contents of the message.
} ATTRPACK msg_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct Copy_CVarMS_t
{
	char ip[64];
	char port[8];
	char name[64];
} Copy_CVarMS_s;
static Copy_CVarMS_s registered_server;
static time_t MSLastPing;

#if defined(_MSC_VER)
#pragma pack(1)
#endif
typedef struct
{
	char ip[16];         // Big enough to hold a full address.
	UINT16 port;
	UINT8 padding1[2];
	tic_t time;
} ATTRPACK ms_holepunch_packet_t;
#if defined(_MSC_VER)
#pragma pack()
#endif

// win32 or djgpp
#if defined (_WIN32) || defined (_WIN32_WCE) || defined (__DJGPP__)
#define ioctl ioctlsocket
#define close closesocket
#ifdef WATTCP
#define strerror strerror_s
#endif
#if defined (_WIN32) || defined (_WIN32_WCE)
#undef errno
#define errno h_errno // some very strange things happen when not using h_error
#endif
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0x00000400
#endif
#endif

#ifndef NONET
static void Command_Listserv_f(void);
#endif
static void MasterServer_OnChange(void);
static void ServerName_OnChange(void);

#define DEF_PORT "28900"
consvar_t cv_masterserver = {"masterserver", "ms.srb2.org:"DEF_PORT, CV_SAVE, NULL, MasterServer_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_servername = {"servername", "SRB2 server", CV_SAVE, NULL, ServerName_OnChange, 0, NULL, NULL, 0, 0, NULL};

INT16 ms_RoomId = -1;

static enum { MSCS_NONE, MSCS_WAITING, MSCS_REGISTERED, MSCS_FAILED } con_state = MSCS_NONE;

static INT32 msnode = -1;
UINT16 current_port = 0;

#if (defined (_WIN32) || defined (_WIN32_WCE) || defined (_WIN32)) && !defined (NONET)
typedef SOCKET SOCKET_TYPE;
#define ERRSOCKET (SOCKET_ERROR)
#else
#if (defined (__unix__) && !defined (MSDOS)) || defined (__APPLE__) || defined (__HAIKU__) || defined (_PS3)
typedef int SOCKET_TYPE;
#else
typedef unsigned long SOCKET_TYPE;
#endif
#define ERRSOCKET (-1)
#endif

#if (defined (WATTCP) && !defined (__libsocket_socklen_t)) || defined (_WIN32)
typedef int socklen_t;
#endif

#ifndef NONET
static SOCKET_TYPE socket_fd = ERRSOCKET; // WINSOCK socket
static struct timeval select_timeout;
static fd_set wset;
static size_t recvfull(SOCKET_TYPE s, char *buf, size_t len, int flags);
#endif

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
	CV_RegisterVar(&cv_servername);
	COM_AddCommand("listserv", Command_Listserv_f);
#endif
}

/** Closes the connection to the master server.
  *
  * \todo Fix for Windows?
  */
static void CloseConnection(void)
{
#ifndef NONET
	if (socket_fd != (SOCKET_TYPE)ERRSOCKET)
		close(socket_fd);
	socket_fd = ERRSOCKET;
#endif
}

//
// MS_Write():
//
static INT32 MS_Write(msg_t *msg)
{
#ifdef NONET
	(void)msg;
	return MS_WRITE_ERROR;
#else
	size_t len;

	if (msg->length == 0)
		msg->length = (INT32)strlen(msg->buffer);
	len = msg->length + HEADER_SIZE;

	msg->type = htonl(msg->type);
	msg->length = htonl(msg->length);
	msg->room = htonl(msg->room);

	if ((size_t)send(socket_fd, (char *)msg, (int)len, 0) != len)
		return MS_WRITE_ERROR;
	return 0;
#endif
}

//
// MS_Read():
//
static INT32 MS_Read(msg_t *msg)
{
#ifdef NONET
	(void)msg;
	return MS_READ_ERROR;
#else
	if (recvfull(socket_fd, (char *)msg, HEADER_SIZE, 0) != HEADER_SIZE)
		return MS_READ_ERROR;

	msg->type = ntohl(msg->type);
	msg->length = ntohl(msg->length);
	msg->room = ntohl(msg->room);

	if (!msg->length) // fix a bug in Windows 2000
		return 0;

	if (recvfull(socket_fd, (char *)msg->buffer, msg->length, 0) != msg->length)
		return MS_READ_ERROR;
	return 0;
#endif
}

#ifndef NONET
/** Gets a list of game servers from the master server.
  */
static INT32 GetServersList(void)
{
	msg_t msg;
	INT32 count = 0;

	msg.type = GET_SERVER_MSG;
	msg.length = 0;
	msg.room = 0;
	if (MS_Write(&msg) < 0)
		return MS_WRITE_ERROR;

	while (MS_Read(&msg) >= 0)
	{
		if (!msg.length)
		{
			if (!count)
				CONS_Alert(CONS_NOTICE, M_GetText("No servers currently running.\n"));
			return MS_NO_ERROR;
		}
		count++;
		CONS_Printf("%s",msg.buffer);
	}

	return MS_READ_ERROR;
}
#endif

//
// MS_Connect()
//
static INT32 MS_Connect(const char *ip_addr, const char *str_port, INT32 async)
{
#ifdef NONET
	(void)ip_addr;
	(void)str_port;
	(void)async;
#else
	struct my_addrinfo *ai, *runp, hints;
	int gaie;

	memset (&hints, 0x00, sizeof(hints));
#ifdef AI_ADDRCONFIG
	hints.ai_flags = AI_ADDRCONFIG;
#endif
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//I_InitTcpNetwork(); this is already done on startup in D_SRB2Main()
	if (!I_InitTcpDriver()) // this is done only if not already done
		return MS_SOCKET_ERROR;

	gaie = I_getaddrinfo(ip_addr, str_port, &hints, &ai);
	if (gaie != 0)
		return MS_GETHOSTBYNAME_ERROR;
	else
		runp = ai;

	while (runp != NULL)
	{
		socket_fd = socket(runp->ai_family, runp->ai_socktype, runp->ai_protocol);
		if (socket_fd != (SOCKET_TYPE)ERRSOCKET)
		{
			if (async) // do asynchronous connection
			{
#ifdef FIONBIO
#ifdef WATTCP
				char res = 1;
#else
				unsigned long res = 1;
#endif

				ioctl(socket_fd, FIONBIO, &res);
#endif

				if (connect(socket_fd, runp->ai_addr, (socklen_t)runp->ai_addrlen) == ERRSOCKET)
				{
#ifdef _WIN32 // humm, on win32/win64 it doesn't work with EINPROGRESS (stupid windows)
					if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
					if (errno != EINPROGRESS)
#endif
					{
						con_state = MSCS_FAILED;
						CloseConnection();
						I_freeaddrinfo(ai);
						return MS_CONNECT_ERROR;
					}
				}
				con_state = MSCS_WAITING;
				FD_ZERO(&wset);
				FD_SET(socket_fd, &wset);
				select_timeout.tv_sec = 0, select_timeout.tv_usec = 0;
				I_freeaddrinfo(ai);
				return 0;
			}
			else if (connect(socket_fd, runp->ai_addr, (socklen_t)runp->ai_addrlen) != ERRSOCKET)
			{
				I_freeaddrinfo(ai);
				return 0;
			}
		}
		runp = runp->ai_next;
	}
	I_freeaddrinfo(ai);
#endif
	return MS_CONNECT_ERROR;
}

#define NUM_LIST_SERVER MAXSERVERLIST
const msg_server_t *GetShortServersList(INT32 room)
{
	static msg_server_t server_list[NUM_LIST_SERVER+1]; // +1 for easy test
	msg_t msg;
	INT32 i;

	// we must be connected to the master server before writing to it
	if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n"), NULL, MM_NOTHING);
		return NULL;
	}

	msg.type = GET_SHORT_SERVER_MSG;
	msg.length = 0;
	msg.room = room;
	if (MS_Write(&msg) < 0)
		return NULL;

	for (i = 0; i < NUM_LIST_SERVER && MS_Read(&msg) >= 0; i++)
	{
		if (!msg.length)
		{
			server_list[i].header.buffer[0] = 0;
			CloseConnection();
			return server_list;
		}
		M_Memcpy(&server_list[i], msg.buffer, sizeof (msg_server_t));
		server_list[i].header.buffer[0] = 1;
	}
	CloseConnection();
	if (i == NUM_LIST_SERVER)
	{
		server_list[i].header.buffer[0] = 0;
		return server_list;
	}
	else
		return NULL;
}

INT32 GetRoomsList(boolean hosting)
{
	static msg_ban_t banned_info[1];
	msg_t msg;
	INT32 i;

	// we must be connected to the master server before writing to it
	if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n"), NULL, MM_NOTHING);
		return -1;
	}

	if (hosting)
		msg.type = GET_ROOMS_HOST_MSG;
	else
		msg.type = GET_ROOMS_MSG;
	msg.length = 0;
	msg.room = 0;
	if (MS_Write(&msg) < 0)
	{
		room_list[0].id = 1;
		strcpy(room_list[0].motd,"Master Server Offline.");
		strcpy(room_list[0].name,"Offline");
		return -1;
	}

	for (i = 0; i < NUM_LIST_ROOMS && MS_Read(&msg) >= 0; i++)
	{
		if(msg.type == GET_BANNED_MSG)
		{
			char banmsg[1000];
			M_Memcpy(&banned_info[0], msg.buffer, sizeof (msg_ban_t));
			if (hosting)
				sprintf(banmsg, M_GetText("You have been banned from\nhosting netgames.\n\nUnder the following IP Range:\n%s - %s\n\nFor the following reason:\n%s\n\nYour ban will expire on:\n%s"),banned_info[0].ipstart,banned_info[0].ipend,banned_info[0].reason,banned_info[0].endstamp);
			else
				sprintf(banmsg, M_GetText("You have been banned from\njoining netgames.\n\nUnder the following IP Range:\n%s - %s\n\nFor the following reason:\n%s\n\nYour ban will expire on:\n%s"),banned_info[0].ipstart,banned_info[0].ipend,banned_info[0].reason,banned_info[0].endstamp);
			M_StartMessage(banmsg, NULL, MM_NOTHING);
			ms_RoomId = -1;
			return -2;
		}
		if (!msg.length)
		{
			room_list[i].header.buffer[0] = 0;
			CloseConnection();
			return 1;
		}
		M_Memcpy(&room_list[i], msg.buffer, sizeof (msg_rooms_t));
		room_list[i].header.buffer[0] = 1;
	}
	CloseConnection();
	if (i == NUM_LIST_ROOMS)
	{
		room_list[i].header.buffer[0] = 0;
		return 1;
	}
	else
	{
		room_list[0].id = 1;
		strcpy(room_list[0].motd,M_GetText("Master Server Offline."));
		strcpy(room_list[0].name,M_GetText("Offline"));
		return -1;
	}
}

#ifdef UPDATE_ALERT
const char *GetMODVersion(void)
{
	static msg_t msg;

	// we must be connected to the master server before writing to it
	if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n"), NULL, MM_NOTHING);
		return NULL;
	}

	msg.type = GET_VERSION_MSG;
	msg.length = sizeof MODVERSION;
	msg.room = MODID; // Might as well use it for something.
	sprintf(msg.buffer,"%d",MODVERSION);
	if (MS_Write(&msg) < 0)
		return NULL;

	MS_Read(&msg);
	CloseConnection();

	if(strcmp(msg.buffer,"NULL") != 0)
	{
		return msg.buffer;
	}
	else
		return NULL;
}

// Console only version of the above (used before game init)
void GetMODVersion_Console(void)
{
	static msg_t msg;

	// we must be connected to the master server before writing to it
	if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}

	msg.type = GET_VERSION_MSG;
	msg.length = sizeof MODVERSION;
	msg.room = MODID; // Might as well use it for something.
	sprintf(msg.buffer,"%d",MODVERSION);
	if (MS_Write(&msg) < 0)
		return;

	MS_Read(&msg);
	CloseConnection();

	if(strcmp(msg.buffer,"NULL") != 0)
		I_Error(UPDATE_ALERT_STRING_CONSOLE, VERSIONSTRING, msg.buffer);
}
#endif

#ifndef NONET
/** Gets a list of game servers. Called from console.
  */
static void Command_Listserv_f(void)
{
	if (con_state == MSCS_WAITING)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Not yet connected to the Master Server.\n"));
		return;
	}

	CONS_Printf(M_GetText("Retrieving server list...\n"));

	if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}

	if (GetServersList())
		CONS_Alert(CONS_ERROR, M_GetText("Cannot get server list\n"));

	CloseConnection();
}
#endif

FUNCMATH static const char *int2str(INT32 n)
{
	INT32 i;
	static char res[16];

	res[15] = '\0';
	res[14] = (char)((char)(n%10)+'0');
	for (i = 13; (n /= 10); i--)
		res[i] = (char)((char)(n%10)+'0');

	return &res[i+1];
}

#ifndef NONET
static INT32 ConnectionFailed(void)
{
	con_state = MSCS_FAILED;
	CONS_Alert(CONS_ERROR, M_GetText("Connection to Master Server failed\n"));
	CloseConnection();
	return MS_CONNECT_ERROR;
}
#endif

/** Tries to register the local game server on the master server.
  */
static INT32 AddToMasterServer(boolean firstadd)
{
#ifdef NONET
	(void)firstadd;
#else
	static INT32 retry = 0;
	int i, res;
	socklen_t j;
	msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;
	INT32 room = -1;
	fd_set tset;
	time_t timestamp = time(NULL);
	UINT32 signature, tmp;
	const char *insname;

	M_Memcpy(&tset, &wset, sizeof (tset));
	res = select(255, NULL, &tset, NULL, &select_timeout);
	if (res != ERRSOCKET && !res)
	{
		if (retry++ > 30) // an about 30 second timeout
		{
			retry = 0;
			CONS_Alert(CONS_ERROR, M_GetText("Master Server timed out\n"));
			MSLastPing = timestamp;
			return ConnectionFailed();
		}
		return MS_CONNECT_ERROR;
	}
	retry = 0;
	if (res == ERRSOCKET)
	{
		if (MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Master Server socket error #%u: %s\n"), errno, strerror(errno));
			MSLastPing = timestamp;
			return ConnectionFailed();
		}
	}

	// so, the socket is writable, but what does that mean, that the connection is
	// ok, or bad... let see that!
	j = (socklen_t)sizeof (i);
	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (char *)&i, &j);
	if (i) // it was bad
	{
		CONS_Alert(CONS_ERROR, M_GetText("Master Server socket error #%u: %s\n"), errno, strerror(errno));
		MSLastPing = timestamp;
		return ConnectionFailed();
	}

#ifdef PARANOIA
	if (ms_RoomId <= 0)
		I_Error("Attmepted to host in room \"All\"!\n");
#endif
	room = ms_RoomId;

	for(signature = 0, insname = cv_servername.string; *insname; signature += *insname++);
	tmp = (UINT32)(signature * (size_t)&MSLastPing);
	signature *= tmp;
	signature &= 0xAAAAAAAA;
	M_Memcpy(&info->header.signature, &signature, sizeof (UINT32));

	strcpy(info->ip, "");
	strcpy(info->port, int2str(current_port));
	strcpy(info->name, cv_servername.string);
	M_Memcpy(&info->room, & room, sizeof (INT32));
#if VERSION > 0 || SUBVERSION > 0
	sprintf(info->version, "%d.%d.%d", VERSION/100, VERSION%100, SUBVERSION);
#else // Trunk build, send revision info
	strcpy(info->version, GetRevisionString());
#endif
	strcpy(registered_server.name, cv_servername.string);

	if(firstadd)
		msg.type = ADD_SERVER_MSG;
	else
		msg.type = PING_SERVER_MSG;

	msg.length = (UINT32)sizeof (msg_server_t);
	msg.room = 0;
	if (MS_Write(&msg) < 0)
	{
		MSLastPing = timestamp;
		return ConnectionFailed();
	}

	if(con_state != MSCS_REGISTERED)
		CONS_Printf(M_GetText("Master Server update successful.\n"));

	MSLastPing = timestamp;
	con_state = MSCS_REGISTERED;
	CloseConnection();
#endif
	return MS_NO_ERROR;
}

static INT32 RemoveFromMasterSever(void)
{
	msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;

	strcpy(info->header.buffer, "");
	strcpy(info->ip, "");
	strcpy(info->port, int2str(current_port));
	strcpy(info->name, registered_server.name);
	sprintf(info->version, "%d.%d.%d", VERSION/100, VERSION%100, SUBVERSION);

	msg.type = REMOVE_SERVER_MSG;
	msg.length = (UINT32)sizeof (msg_server_t);
	msg.room = 0;
	if (MS_Write(&msg) < 0)
		return MS_WRITE_ERROR;

	return MS_NO_ERROR;
}

const char *GetMasterServerPort(void)
{
	const char *t = cv_masterserver.string;

	while ((*t != ':') && (*t != '\0'))
		t++;

	if (*t)
		return ++t;
	else
		return DEF_PORT;
}

/** Gets the IP address of the master server. Actually, it seems to just
  * return the hostname, instead; the lookup is done elsewhere.
  *
  * \return Hostname of the master server, without port number on the end.
  * \todo Rename function?
  */
const char *GetMasterServerIP(void)
{
	static char str_ip[64];
	char *t = str_ip;

	if (strstr(cv_masterserver.string, "srb2.ssntails.org:28910")
	 || strstr(cv_masterserver.string, "srb2.servegame.org:28910")
	 || strstr(cv_masterserver.string, "srb2.servegame.org:28900")
	   )
	{
		// replace it with the current default one
		CV_Set(&cv_masterserver, cv_masterserver.defaultvalue);
	}

	strcpy(t, cv_masterserver.string);

	while ((*t != ':') && (*t != '\0'))
		t++;
	*t = '\0';

	return str_ip;
}

void MSOpenUDPSocket(void)
{
#ifndef NONET
	if (I_NetMakeNodewPort)
	{
		// If it's already open, there's nothing to do.
		if (msnode < 0)
			msnode = I_NetMakeNodewPort(GetMasterServerIP(), GetMasterServerPort());
	}
	else
#endif
		msnode = -1;
}

void MSCloseUDPSocket(void)
{
	if (msnode != INT16_MAX) I_NetFreeNodenum(msnode);
	msnode = -1;
}

void RegisterServer(void)
{
	if (con_state == MSCS_REGISTERED || con_state == MSCS_WAITING)
			return;

	CONS_Printf(M_GetText("Registering this server on the Master Server...\n"));

	strcpy(registered_server.ip, GetMasterServerIP());
	strcpy(registered_server.port, GetMasterServerPort());

	if (MS_Connect(registered_server.ip, registered_server.port, 1))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}
	MSOpenUDPSocket();

	// keep the TCP connection open until AddToMasterServer() is completed;
}

static inline void SendPingToMasterServer(void)
{
/*	static tic_t next_time = 0;
	tic_t cur_time;
	char *inbuffer = (char*)netbuffer;

	cur_time = I_GetTime();
	if (!netgame)
		UnregisterServer();
	else if (cur_time > next_time) // ping every 2 second if possible
	{
		next_time = cur_time+2*TICRATE;

		if (con_state == MSCS_WAITING)
			AddToMasterServer();

		if (con_state != MSCS_REGISTERED)
			return;

		// cur_time is just a dummy data to send
		WRITEUINT32(inbuffer, cur_time);
		doomcom->datalength = sizeof (cur_time);
		doomcom->remotenode = (INT16)msnode;
		I_NetSend();
	}
*/

// Here, have a simpler MS Ping... - Cue
	if(time(NULL) > (MSLastPing+(60*2)) && con_state != MSCS_NONE)
	{
		//CONS_Debug(DBG_NETPLAY, "%ld (current time) is greater than %d (Last Ping Time)\n", time(NULL), MSLastPing);
		if(MSLastPing < 1)
			AddToMasterServer(true);
		else
			AddToMasterServer(false);
	}
}

void SendAskInfoViaMS(INT32 node, tic_t asktime)
{
	const char *address;
	UINT16 port;
	char *inip;
	ms_holepunch_packet_t mshpp;

	MSOpenUDPSocket();

	// This must be called after calling MSOpenUDPSocket, due to the
	// static buffer.
	address = I_GetNodeAddress(node);

	// no address?
	if (!address)
		return;

	// Copy the IP address into the buffer.
	inip = mshpp.ip;
	while(*address && *address != ':') *inip++ = *address++;
	*inip = '\0';

	// Get the port.
	port = (UINT16)(*address++ ? atoi(address) : 0);
	mshpp.port = SHORT(port);

	// Set the time for ping calculation.
	mshpp.time = LONG(asktime);

	// Send to the MS.
	M_Memcpy(netbuffer, &mshpp, sizeof(mshpp));
	doomcom->datalength = sizeof(ms_holepunch_packet_t);
	doomcom->remotenode = (INT16)msnode;
	I_NetSend();
}

void UnregisterServer(void)
{
	if (con_state != MSCS_REGISTERED)
	{
		con_state = MSCS_NONE;
		CloseConnection();
		return;
	}

	con_state = MSCS_NONE;

	CONS_Printf(M_GetText("Removing this server from the Master Server...\n"));

	if (MS_Connect(registered_server.ip, registered_server.port, 0))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}

	if (RemoveFromMasterSever() < 0)
		CONS_Alert(CONS_ERROR, M_GetText("Cannot remove this server from the Master Server\n"));

	CloseConnection();
	MSCloseUDPSocket();
	MSLastPing = 0;
}

void MasterClient_Ticker(void)
{
	if (server && ms_RoomId > 0)
		SendPingToMasterServer();
}

static void ServerName_OnChange(void)
{
	UnregisterServer();
	RegisterServer();
}

static void MasterServer_OnChange(void)
{
	UnregisterServer();
	RegisterServer();
}

#ifndef NONET
// Like recv, but waits until we've got enough data to fill the buffer.
static size_t recvfull(SOCKET_TYPE s, char *buf, size_t len, int flags)
{
	/* Total received. */
	size_t totallen = 0;

	while(totallen < len)
	{
		ssize_t ret = (ssize_t)recv(s, buf + totallen, (int)(len - totallen), flags);

		/* Error. */
		if(ret == -1)
			return (size_t)-1;

		totallen += ret;
	}

	return totallen;
}
#endif
