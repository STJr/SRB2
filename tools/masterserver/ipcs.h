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

#ifndef _IPCS_H_
#define _IPCS_H_

#include "common.h"

#if defined (_WIN32) || defined ( __OS2__)
#include <io.h>
#include <sys/types.h>
typedef int socklen_t;
#if defined (__OS2__)
#include <netinet/in.h>
#endif
#endif
#ifdef _WIN32
#include <winsock.h>
#define close closesocket
#else
#include <arpa/inet.h>   // inet_addr(),...
#endif

#ifndef SOCKET
#define SOCKET u_int
#endif

// ================================ DEFINITIONS ===============================

#define PACKET_SIZE 1024
#define MAX_CLIENT    512

#ifndef _WIN32
#define NO_ERROR                      0
#define SOCKET_ERROR               -201
#endif
#define BIND_ERROR                 -202
#define CONNECT_ERROR              -203
#define LISTEN_ERROR               -204
#define ACCEPT_ERROR               -205
#define WRITE_ERROR                -210
#define READ_ERROR                 -211
#define CLOSE_ERROR                -212
#define GETHOSTBYNAME_ERROR        -220
#define SELECT_ERROR               -230
#define TIMEOUT_ERROR              -231
#define MALLOC_ERROR               -301

#define INVALID_MSG                   -1
#define ACCEPT_MSG                   100
#define ADD_SERVER_MSG               101
#define ADD_CLIENT_MSG               102
#define REMOVE_SERVER_MSG            103
#define ADD_SERVERv2_MSG             104
#define GET_SERVER_MSG               200
#define SEND_SERVER_MSG              201
#define GET_LOGFILE_MSG              202
#define SEND_FILE_MSG                203
#define ERASE_LOGFILE_MSG            204
#define GET_SHORT_SERVER_MSG         205
#define SEND_SHORT_SERVER_MSG        206
#define ASK_SERVER_MSG               206
#define ANSWER_ASK_SERVER_MSG        207
#define GET_MOTD_MSG                 208
#define SEND_MOTD_MSG                209
#define GET_ROOMS_MSG				 210
#define SEND_ROOMS_MSG				 211
#define GET_ROOMS_HOST_MSG			 212
#define GET_VERSION_MSG				 213
#define SEND_VERSION_MSG			 214
#define GET_BANNED_MSG				 215
#define PING_SERVER_MSG				 216

#define UDP_RECV_MSG                 300
#define TIMEOUT_MSG                  301
#define HTTP_REQUEST_MSG       875770417    // "4321"
#define SEND_HTTP_REQUEST_MSG  875770418    // "4322"
#define TEXT_REQUEST_MSG       825373494    // "1236"
#define SEND_TEXT_REQUEST_MSG  825373495    // "1237"
#define RSS92_REQUEST_MSG      825373496    // "1238"
#define SEND_RSS92_REQUEST_MSG 825373497    // "1239"
#define RSS10_REQUEST_MSG      825373744    // "1240"
#define SEND_RSS10_REQUEST_MSG 825373745    // "1241"
#define ADD_PSERVER_MSG        0xabacab81    // this number just need to be different than the others
#define REMOVE_PSERVER_MSG     0xabacab82

// Sent FROM Client
#define LIVE_AUTH_USER				 600
#define LIVE_AUTH_KEY				 601
#define LIVE_GET_USER				 602
#define LIVE_UPDATE_LOCATION		 603
#define LIVE_UPDATE_PUBLIC_KEY		 604
#define LIVE_AUTH_PUBLIC_KEY		 605

// Sent TO Client
#define LIVE_INVALID_KEY			 800
#define LIVE_INVALID_USER			 801
#define LIVE_AUTHORISED_KEY			 802
#define	LIVE_SEND_USER				 803
#define LIVE_VALIDATED_USER			 804

// Location Types
#define LIVE_LOCATION_SP			 100
#define LIVE_LOCATION_MENU			 101
#define LIVE_LOCATION_MP_JOIN		 102
#define LIVE_LOCATION_MP_HOST		 103
#define LIVE_LOCATION_MP_LOCAL		 104
#define LIVE_LOCATION_MP_PRIVATE	 105

#define HEADER_SIZE ((UINT32)sizeof (UINT32)*4)

#define HEADER_MSG_POS      0
#define IP_MSG_POS         16
#define PORT_MSG_POS       32
#define HOSTNAME_MSG_POS   40

#if defined(_MSC_VER)
#pragma pack(1)
#endif

// Keep this structure 8 bytes aligned (current size is 80)
typedef struct
{
	char header[16]; // information such as password
	char ip[16];
	char port[8];
	char name[32];
	INT32 room;
	char version[8]; // format is: x.yy.z (like 1.30.2 or 1.31)
	char key[32]; // Secret key for linking dedicated servers to accounts
} ATTRPACK msg_server_t;

typedef struct
{
	char header[16];
	UINT32 id;
	char name[32];
	char motd[256];
} ATTRPACK msg_rooms_t;

typedef struct
{
	char header[16];
	char ipstart[16];
	char ipend[16];
	char endstamp[32];
	char reason[256];
	UINT8 hostonly;
} ATTRPACK msg_ban_t;

typedef struct
{
	char header[16];
	INT32 id;
	char username[100];
	char password[32];
} ATTRPACK msg_live_auth_t;

typedef struct
{
	char header[16];
	INT32 uid;
	char username[100];
	INT32 location_type;
	char location_ip[32];
	INT32 location_port;
	INT32 lastseen_type;
	char lastseen_data1[256];
	char lastseen_data2[256];
	char lastseen_data3[256];
} ATTRPACK msg_live_user_t;

typedef struct
{
	char header[16];
	UINT8 location_type;
	char location_ip[32];
	INT32 location_port;
	char location_data1[256];
	char location_data2[256];
	char location_data3[256];
} ATTRPACK msg_live_updatelocation_t;

typedef struct
{
	char header[16];
	char publickey[256];
	char username[256];
} ATTRPACK msg_live_validateuser_t;

typedef struct
{
	char header[16];
	char username[256];
	UINT8 keytype;
	char keydata[256];
} ATTRPACK msg_live_update_key_t;


typedef struct
{
	UINT32 id;
	INT32 type;
	INT32 room;
	INT32 length;
	char buffer[PACKET_SIZE];
} ATTRPACK msg_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

class CSocket
{
protected:
	sockaddr_in addr;
	msg_t msg;
	fd_set rset;
public:
	int getIP(const char *);
	CSocket();
	~CSocket();
};

class CServerSocket : public CSocket
{
private:
	sockaddr_in udp_addr;
	sockaddr_in udp_in_addr;
	SOCKET udp_fd;
	SOCKET accept_fd;
	size_t num_clients;
	SOCKET client_fd[MAX_CLIENT];
	sockaddr_in client_addr[MAX_CLIENT];

public:
	int deleteClient(size_t id);
	int listen(const char *str_port);
	int accept(void);
	int read(msg_t *msg);
	const char *getUdpIP(void);
	const char *getUdpPort(bool);
	int write(msg_t *msg);
	int writeUDP(const char *data, size_t length, const char *ip, UINT16 port);
	const char *getClientIP(size_t id);
	const char *getClientPort(size_t id);
	CServerSocket(void);
	~CServerSocket(void);
};

class CClientSocket : public CSocket
{
private:
	SOCKET socket_fd;
public:
	int connect(const char *ip_addr, const char *str_port);
	int read(msg_t *msg);
	int write(msg_t *msg);
	CClientSocket(void);
	~CClientSocket(void);
};

// ================================== PROTOS ==================================

// ================================== EXTERNS =================================

#endif
