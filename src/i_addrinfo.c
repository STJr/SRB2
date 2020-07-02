// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2011-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_addrinfo.c
/// \brief getaddr stubs, for lesser OSes
///
///        Some systems/SDKs, like PSL1GHT v2 does not have addrinfo functions
///        So a stub is needed
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef _WIN32
#ifdef USE_WINSOCK2
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif
#elif !defined (__DJGPP__)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "i_addrinfo.h"

/*
 * This addrinfo stub have a testcases:
 * To test the stub, use this command to compile the stub testcase:
 * gcc -g -DTESTCASE -Dtest_stub -Wall -W i_addrinfo.c -o stub
 * To test the real getaddrinfo API, use this command to get real one:
 * gcc -g -DTESTCASE -Utest_stub -Wall -W i_addrinfo.c -o real
 * For Win32,you need the WinSock library, version 1.1 or 2.2
 * for 1.1, add -lwsock32
 * i686-w64-mingw32-gcc -g -DTESTCASE -Dtest_stub -UUSE_WINSOCK2 -Wall -W i_addrinfo.c -o stub.exe -lwsock32
 * for 2.2, add -DUSE_WINSOCK2 -lws2_32
 * i686-w64-mingw32-gcc -g -DTESTCASE -Utest_stub -DUSE_WINSOCK2 -Wall -W i_addrinfo.c -o real.exe -lws2_32
 */

#ifndef I_getaddrinfo

#if !defined (_MSC_VER) || (_MSC_VER >= 1800) // MSVC 2013 and forward
#include <stdbool.h>
#else
typedef char bool;
#endif

#ifdef _WIN32
// it seems windows doesn't define that... maybe some other OS? OS/2
static int inet_aton(const char *cp, struct in_addr *addr)
{
	if( cp == NULL || addr == NULL )
	{
		return(0);
	}
	if (strcmp(cp, "255.255.255.225") == 0)
	{
		addr->s_addr = htonl(INADDR_BROADCAST);
		return 0;
	}
	return (addr->s_addr = inet_addr(cp)) != htonl(INADDR_NONE);
}
#endif

#ifdef USE_WINSOCK2
static HMODULE ipv6dll = NULL;
typedef int (WSAAPI *p_getaddrinfo) (const char *, const char *,
                                     const struct my_addrinfo *,
                                     struct my_addrinfo **);
typedef void (WSAAPI *p_freeaddrinfo) (struct my_addrinfo *);

static p_getaddrinfo WS_getaddrinfo = NULL;
static p_freeaddrinfo WS_freeaddrinfo = NULL;

static HMODULE WS_getfunctions(HMODULE tmp)
{
	if (tmp != NULL)
	{
		WS_getaddrinfo = (p_getaddrinfo)(LPVOID)GetProcAddress(tmp, "getaddrinfo");
		if (WS_getaddrinfo == NULL)
			return NULL;
		WS_freeaddrinfo = (p_freeaddrinfo)(LPVOID)GetProcAddress(tmp, "freeaddrinfo");
		if (WS_freeaddrinfo == NULL)
		{
			WS_getaddrinfo = NULL;
			return NULL;
		}
	}
	return tmp;
}

static void WS_addrinfosetup(void)
{
	if (WS_getaddrinfo && WS_freeaddrinfo)
		return; // already have the functions
	// why not hold it into ipv6dll? becase we already link with ws2_32, silly!
	if (WS_getfunctions(GetModuleHandleA("ws2_32.dll")) == NULL)
		ipv6dll = WS_getfunctions(LoadLibraryA("wship6.dll"));
}

void WS_addrinfocleanup(void)
{
	if (ipv6dll)
		FreeLibrary(ipv6dll);
	ipv6dll = NULL;
	WS_getaddrinfo = NULL;
	WS_freeaddrinfo = NULL;
}
#else
void WS_addrinfocleanup(void) {}
#endif

int I_getaddrinfo(const char *node, const char *service,
                  const struct my_addrinfo *hints,
                  struct my_addrinfo **res)
{
	struct hostent *nodename = NULL;
	bool hostlookup = true, passivemode = false;
	int flags = 0;
	int socktype = 0;
	int sockport = 0;
	struct my_addrinfo *ai;
	struct sockaddr_in *addr;
	struct in_addr ipv4;
	size_t addrlen = 1;
	size_t ailen = 1, i = 0, j;
	size_t famsize = sizeof(struct sockaddr_in);
#ifdef USE_WINSOCK2
	WS_addrinfosetup();
	if (WS_getaddrinfo)
		return WS_getaddrinfo(node, service, hints, res);
#endif

	// param check
	if (node == NULL && service == NULL)
		return EAI_NONAME;
	//note: testcase show NULL as res crashes libc
	if (res == NULL)
		return -1;

	// check hints
	if (hints)
	{
		// no support for IPv6
		if (hints->ai_family != AF_INET && hints->ai_family != AF_UNSPEC)
			return -1; //EAI_FAMILY
#ifdef AI_NUMERICHOST
		// do not look up hostnames
		if ((hints->ai_flags & AI_NUMERICHOST) == AI_NUMERICHOST)
		{
			hostlookup = false;
			flags |= AI_NUMERICHOST;
		}
#endif
#ifdef AI_PASSIVE
		// return loopback or any address?
		if ((hints->ai_flags & AI_PASSIVE) == AI_PASSIVE)
		{
			passivemode = true;
			flags |= AI_PASSIVE;
		}
#endif
		// why no support for canon names? lazy
		//AI_CANONNAME
		// what kind of socket?
		socktype = hints->ai_socktype;
	}
	else
	{
#ifdef AI_V4MAPPED
		flags |= AI_V4MAPPED;
#endif
#ifdef AI_ADDRCONFIG
		flags |= AI_ADDRCONFIG;
#endif
	}

	// we need gethostbyname to convert numeric strings
	if (node && hostlookup)
	{
		if (node == NULL)
			return -1;
		nodename = gethostbyname(node);
		if (nodename == NULL)
		{
			if (inet_aton(node, &ipv4) != 0)
				addrlen = 1;
			else
				return -1;
		}
		else while (nodename->h_addr_list[addrlen] != NULL)
			addrlen++;
	}
	else if (node)
	{
		if (inet_aton(node, &ipv4) != 0)
			addrlen = 1;
		else if (!hostlookup)
			return EAI_NONAME;
	}
	else
		addrlen = 1;

	if (service) //AI_NUMERICSERV
		sockport = atoi(service);

	if (socktype == 0)
		ailen = addrlen*3;
	else
		ailen = addrlen;

	ai = calloc(ailen, sizeof(struct my_addrinfo));
	if  (ai == NULL)
		return -1;
	else
		*res = ai;

	addr = calloc(ailen*2, famsize);
	if (addr == NULL)
	{
		free(ai);
		return -1;
	}
	else
		ai->ai_addr = memset(addr, 0x00, ailen*ai->ai_addrlen);

	for (i = 0; i < ailen; i++)
	{
		ai = *res+i;
		ai->ai_flags = flags;
		ai->ai_family = AF_INET;
		ai->ai_socktype = socktype;
		switch (ai->ai_socktype)
		{
			case SOCK_STREAM:
				ai->ai_protocol = IPPROTO_TCP;
				break;
			case SOCK_DGRAM:
				ai->ai_protocol = IPPROTO_UDP;
				break;
			default:
				ai->ai_protocol = 0;
		}
		ai->ai_addrlen = famsize;
		ai->ai_addr = (struct sockaddr *)&addr[i];
		//ai_cannonname
		ai->ai_next = ai+1;
	}
	ai->ai_next = NULL;
	ai = *res;

	for (i = 0, j = 0; i < ailen; i++, j++)
	{
		ai = *res+i;
		addr[i].sin_port = htons((UINT16)sockport);
		if (nodename)
		{
			memcpy(&addr[i].sin_addr, nodename->h_addr_list[j], ai->ai_addrlen);
			addr[i].sin_family = nodename->h_addrtype;
		}
		else if (node)
		{
			memcpy(&addr[i].sin_addr, &ipv4, ai->ai_addrlen);
			addr[i].sin_family = AF_INET;
		}
		else
		{
			if (passivemode)
				addr[i].sin_addr.s_addr = htonl(INADDR_ANY);
			else
				addr[i].sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			addr[i].sin_family = AF_INET;
		}
		if (socktype == 0)
		{
			ai->ai_socktype = SOCK_STREAM;
			ai->ai_protocol = IPPROTO_TCP;
			memcpy(&addr[i+1], &addr[i], ai->ai_addrlen);
			i++; ai++;
			ai->ai_socktype = SOCK_DGRAM;
			ai->ai_protocol = IPPROTO_UDP;
			memcpy(&addr[i+1], &addr[i], ai->ai_addrlen);
			i++; ai++;
			ai->ai_socktype = SOCK_RAW;
			ai->ai_protocol = IPPROTO_IP;
		}
	}
	return 0;
}
#endif

#ifndef I_freeaddrinfo
void I_freeaddrinfo(struct my_addrinfo *res)
{
#ifdef USE_WINSOCK2
	if (WS_freeaddrinfo)
	{
		WS_freeaddrinfo(res);
		return;
	}
#endif
	if (!res)
		return;
	free(res->ai_addr);
	free(res);
}
#endif

#ifdef TESTCASE
#include <stdio.h>

#ifdef USE_WINSOCK2
#define inet_ntop inet_ntopA
#define HAVE_NTOP
static const char* inet_ntopA(int af, const void *cp, char *buf, socklen_t len)
{
	DWORD Dlen = len, AFlen = 0;
	SOCKADDR_STORAGE any;
	LPSOCKADDR anyp = (LPSOCKADDR)&any;
	LPSOCKADDR_IN any4 = (LPSOCKADDR_IN)&any;
	LPSOCKADDR_IN6 any6 = (LPSOCKADDR_IN6)&any;

	if (!buf)
	{
		WSASetLastError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	if (af != AF_INET && af != AF_INET6)
	{
		WSASetLastError(WSAEAFNOSUPPORT);
		return NULL;
	}

	ZeroMemory(&any, sizeof(SOCKADDR_STORAGE));
	any.ss_family = af;

	switch (af)
	{
		case AF_INET:
		{
			CopyMemory(&any4->sin_addr, cp, sizeof(IN_ADDR));
			AFlen = sizeof(SOCKADDR_IN);
			break;
		}
		case AF_INET6:
		{
			CopyMemory(&any6->sin6_addr, cp, sizeof(IN6_ADDR));
			AFlen = sizeof(SOCKADDR_IN6);
			break;
		}
	}

	if (WSAAddressToStringA(anyp, AFlen, NULL, buf, &Dlen) == SOCKET_ERROR)
		return NULL;
	return buf;
}
#elif defined (_WIN32)
// w32api, ws2tcpip.h, r1.12
static inline char* gai_strerror(int ecode)
{
        static char message[1024+1];
        DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_IGNORE_INSERTS
                      | FORMAT_MESSAGE_MAX_WIDTH_MASK;
        DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        FormatMessageA(dwFlags, NULL, ecode, dwLanguageId, message, 1024, NULL);
        return message;
}
#else
#define HAVE_NTOP
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

static inline void printflags(int ai_flags)
{
	printf("flags: %x(", ai_flags);
	if (ai_flags == 0)
		printf("NONE|");
#ifdef AI_PASSIVE
	if ((ai_flags & AI_PASSIVE) == AI_PASSIVE)
		printf("AI_PASSIVE|");
	ai_flags &= ~AI_PASSIVE;
#endif
#ifdef AI_CANONNAME
	if ((ai_flags & AI_CANONNAME) == AI_CANONNAME)
		printf("AI_CANONNAME|");
	ai_flags &= ~AI_CANONNAME;
#endif
#ifdef AI_NUMERICHOST
	if ((ai_flags & AI_NUMERICHOST) == AI_NUMERICHOST)
		printf("AI_NUMERICHOST|");
	ai_flags &= ~AI_NUMERICHOST;
#endif
#ifdef AI_V4MAPPED
	if ((ai_flags & AI_V4MAPPED) == AI_V4MAPPED)
		printf("AI_V4MAPPED|");
	ai_flags &= ~AI_V4MAPPED;
#endif
#ifdef AI_ALL
	if ((ai_flags & AI_ALL) == AI_ALL)
		printf("AI_ALL|");
	ai_flags &= ~AI_ALL;
#endif
#ifdef AI_ADDRCONFIG
	if ((ai_flags & AI_ADDRCONFIG) == AI_ADDRCONFIG)
		printf("AI_ADDRCONFIG|");
	ai_flags &= ~AI_ADDRCONFIG;
#endif
#ifdef AI_IDN
	if ((ai_flags & AI_IDN) == AI_IDN)
		printf("AI_IDN|");
	ai_flags &= ~AI_IDN;
#endif
#ifdef AI_CANONIDN
	if ((ai_flags & AI_CANONIDN) == AI_CANONIDN)
		printf("AI_CANONIDN|");
	ai_flags &= ~AI_CANONIDN;
#endif
#ifdef AI_IDN_ALLOW_UNASSIGNED
	if ((ai_flags & AI_IDN_ALLOW_UNASSIGNED) == AI_IDN_ALLOW_UNASSIGNED)
		printf("AI_IDN_ALLOW_UNASSIGNED|");
	ai_flags &= ~AI_IDN_ALLOW_UNASSIGNED;
#endif
#ifdef AI_IDN_USE_STD3_ASCII_RULES
	if ((ai_flags & AI_IDN_USE_STD3_ASCII_RULES) == AI_IDN_USE_STD3_ASCII_RULES)
		printf("AI_IDN_USE_STD3_ASCII_RULES|");
	ai_flags &= ~AI_PASSIVE;
#endif
#ifdef AI_NUMERICSERV
	if ((ai_flags & AI_NUMERICSERV) == AI_NUMERICSERV)
		printf("AI_NUMERICSERV|");
	ai_flags &= ~AI_NUMERICSERV;
#endif
	printf("%x", ai_flags);
	printf(")\n");
}

static inline void printsocktype(int ai_socktype)
{
	printf("socktype: %d(", ai_socktype);
	switch (ai_socktype)
	{
		case 0:
			printf("SOCK_ Woe32");
			break;
		case SOCK_STREAM:
			printf("SOCK_STREAM");
			break;
		case SOCK_DGRAM:
			printf("SOCK_DGRAM");
			break;
		case SOCK_RAW:
			printf("SOCK_RAW");
			break;
		default:
			printf("SOCK_ERROR\n");
			exit(-1);
	}
	printf(")\n");
}

static inline void printprotocol(int ai_protocol)
{
	printf("protocol: %d(", ai_protocol);
	switch (ai_protocol)
	{
		case IPPROTO_IP:
			printf("IPPROTO_IP");
			break;
		case IPPROTO_TCP:
			printf("IPPROTO_TCP");
			break;
		case IPPROTO_UDP:
			printf("IPPROTO_UDP");
			break;
		default:
			printf("IPPROTO_ERROR\n");
			exit(-1);
	}
	printf(")\n");
}

static inline void printaddr(int ai_family, const struct sockaddr *ai_addr)
{
	char str[INET_ADDRSTRLEN+1] = "FAKE";
	printf("family: %d(", ai_family);
	switch (ai_family)
	{
		case AF_UNSPEC:
			printf("AF_UNSPEC");
			break;
		case AF_INET:
			printf("AF_INET");
			break;
#ifdef AF_INET6
		case AF_INET6:
			printf("AF_INET6");
			break;
#endif
		default:
			printf("AF_ERROR\n");
			exit(-1);
	}
	printf(")\n");
	if (ai_family == AF_INET && ai_addr)
	{
		const struct sockaddr_in *ip4 = (void *)ai_addr;
#ifdef HAVE_NTOP
		inet_ntop(ai_family, &ip4->sin_addr, str, INET_ADDRSTRLEN);
#else
		strncpy(str, inet_ntoa(ip4->sin_addr), INET_ADDRSTRLEN);
#endif
		printf("port: %d\n", ntohs(ip4->sin_port));
	}
	printf("addr: %s\n", str);
}

void printaddrinfo(const struct my_addrinfo *data)
{
	if(data == NULL)
	{
		printf("no data\n");
		return;
	}
	else
		printf("starting addrinfo dump\n");
	while (data != NULL)
	{
		printflags(data->ai_flags);
		printsocktype(data->ai_socktype);
		printaddr(data->ai_family, data->ai_addr);
		printprotocol(data->ai_protocol);
		//addrlen
		data = data->ai_next;
	}
}

int main(int argc, char **argv)
{
	struct my_addrinfo *res = NULL, hints;
	int gaie;

	(void)argc;
	(void)argv;
	memset(&hints, 0x00, sizeof(struct my_addrinfo));
	hints.ai_family = AF_INET;

#ifdef _WIN32
	{
#ifdef USE_WINSOCK2
		const WORD VerNeed = MAKEWORD(2,2);
#else
		const WORD VerNeed = MAKEWORD(1,1);
#endif
		WSADATA WSAData;
		WSAStartup(VerNeed, &WSAData);
	}
#endif
	printf("-----------------------------------------------------------\n");
#ifndef _WIN32 //NULL as res crashes Win32
	gaie = I_getaddrinfo(NULL, NULL, &hints, NULL);
	if (gaie != EAI_NONAME)
		printf("NULLs test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	I_freeaddrinfo(res);
#endif
#if 0 //NULL as res crashes
	gaie = I_getaddrinfo("localhost", "5029", NULL, NULL);
    if (gaie == 0)
		printf("NULL crashes: %s(%d)\n", gai_strerror(gaie), gaie);
	I_freeaddrinfo(res);
#endif
	gaie = I_getaddrinfo("localhost", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo(NULL, "5029", &hints, &res);
    //if (gaie != 0)
		printf("NULLs test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("localhost", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	printf("-----------------------------------------------------------\n");
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	gaie = I_getaddrinfo("localhost", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo(NULL, "5029", &hints, &res);
    //if (gaie != 0)
		printf("NULLs test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("localhost", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	printf("-----------------------------------------------------------\n");
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	gaie = I_getaddrinfo("localhost", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo(NULL, "5029", &hints, &res);
    //if (gaie != 0)
		printf("NULLs test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("localhost", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	printf("-----------------------------------------------------------\n");
	hints.ai_flags = AI_NUMERICHOST;
	gaie = I_getaddrinfo("localhost", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo(NULL, "5029", &hints, &res);
    //if (gaie != 0)
		printf("NULLs test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("localhost", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	printf("-----------------------------------------------------------\n");
	gaie = I_getaddrinfo("127.0.0.1", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("127.0.0.1", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	printf("-----------------------------------------------------------\n");
	gaie = I_getaddrinfo("255.255.255.255", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("255.255.255.255", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	printf("-----------------------------------------------------------\n");
	hints.ai_flags = AI_PASSIVE;
	gaie = I_getaddrinfo("0.0.0.0", NULL, &hints, &res);
    //if (gaie != 0)
		printf("NULL service test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
	gaie = I_getaddrinfo("0.0.0.0", "5029", &hints, &res);
    //if (gaie != 0)
		printf("no hints test returned: %s(%d)\n", gai_strerror(gaie), gaie);
	printaddrinfo(res);
	I_freeaddrinfo(res); res = NULL;
    return 0;
}
#endif
