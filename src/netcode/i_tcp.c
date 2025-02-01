// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2025 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_tcp.c
/// \brief TCP driver, socket code.
///        This is not really OS-dependent because all OSes have the same socket API.
///        Just use ifdef for OS-dependent parts.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __GNUC__
#include <unistd.h>
#endif

#ifndef NO_IPV6
	#define HAVE_IPV6
#endif

#ifdef _WIN32
	#define USE_WINSOCK
	#if defined (_WIN64) || defined (HAVE_IPV6)
		#define USE_WINSOCK2
	#else //_WIN64/HAVE_IPV6
		#define USE_WINSOCK1
	#endif
#endif //WIN32 OS

#ifdef USE_WINSOCK2
	#include <ws2tcpip.h>
#endif

#include "../doomdef.h"
#include "../z_zone.h"

#ifdef USE_WINSOCK1
	#include <winsock.h>
#else
	#ifndef USE_WINSOCK
		#include <arpa/inet.h>
		#ifdef __APPLE_CC__
			#ifndef _BSD_SOCKLEN_T_
				#define _BSD_SOCKLEN_T_
			#endif //_BSD_SOCKLEN_T_
		#endif //__APPLE_CC__
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <netdb.h>
		#include <sys/ioctl.h>
	#endif //normal BSD API

	#include <errno.h>
	#include <time.h>

	#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
		#include <sys/time.h>
	#endif // UNIXCOMMON
#endif

#ifdef USE_WINSOCK
	// some undefined under win32
	#undef errno
	//#define errno WSAGetLastError() //Alam_GBC: this is the correct way, right?
	#define errno h_errno // some very strange things happen when not using h_error?!?
	#ifdef EWOULDBLOCK
	#undef EWOULDBLOCK
	#endif
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#ifdef EMSGSIZE
	#undef EMSGSIZE
	#endif
	#define EMSGSIZE WSAEMSGSIZE
	#ifdef ECONNREFUSED
	#undef ECONNREFUSED
	#endif
	#define ECONNREFUSED WSAECONNREFUSED
	#ifdef ETIMEDOUT
	#undef ETIMEDOUT
	#endif
	#define ETIMEDOUT WSAETIMEDOUT
	#ifdef EHOSTUNREACH
	#undef EHOSTUNREACH
	#endif
	#define EHOSTUNREACH WSAEHOSTUNREACH
	#ifdef ENETUNREACH
	#undef ENETUNREACH
	#endif
	#define ENETUNREACH WSAENETUNREACH
	#ifndef IOC_VENDOR
	#define IOC_VENDOR 0x18000000
	#endif
	#ifndef _WSAIOW
	#define _WSAIOW(x,y) (IOC_IN|(x)|(y))
	#endif
	#ifndef SIO_UDP_CONNRESET
	#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
	#endif
	#ifndef AI_ADDRCONFIG
	#define AI_ADDRCONFIG 0x00000400
	#endif
	#ifndef STATUS_INVALID_PARAMETER
	#define STATUS_INVALID_PARAMETER 0xC000000D
	#endif
#endif // USE_WINSOCK

typedef union
{
	struct sockaddr     any;
	struct sockaddr_in  ip4;
#ifdef HAVE_IPV6
	struct sockaddr_in6 ip6;
#endif
} mysockaddr_t;

	#ifdef HAVE_MINIUPNPC
	#include "miniupnpc/miniwget.h"
	#include "miniupnpc/miniupnpc.h"
	#include "miniupnpc/upnpcommands.h"
	static boolean UPNP_support = true;
	#endif // HAVE_MINIUPNC

#include "../i_system.h"
#include "i_net.h"
#include "d_net.h"
#include "d_netfil.h"
#include "i_tcp.h"
#include "../m_argv.h"
#include "../i_threads.h"

#include "../doomstat.h"

// win32
#ifdef USE_WINSOCK
	// winsock stuff (in winsock a socket is not a file)
	#define ioctl ioctlsocket
	#define close closesocket
#endif

#include "i_addrinfo.h"
#define DEFAULTPORT "5029"

#ifdef USE_WINSOCK
	typedef SOCKET SOCKET_TYPE;
	#define ERRSOCKET (SOCKET_ERROR)
#else
	#if defined (__unix__) || defined (__APPLE__) || defined (__HAIKU__)
		typedef int SOCKET_TYPE;
	#else
		typedef unsigned long SOCKET_TYPE;
	#endif
	#define ERRSOCKET (-1)
#endif

#define IPV6_MULTICAST_ADDRESS "ff15::57e1:1a12"

// define socklen_t in DOS/Windows if it is not already defined
#ifdef USE_WINSOCK1
	typedef int socklen_t;
#endif
static SOCKET_TYPE mysockets[MAXNETNODES+1] = {ERRSOCKET};
static size_t mysocketses = 0;
static int myfamily[MAXNETNODES+1] = {0};
static SOCKET_TYPE nodesocket[MAXNETNODES+1] = {ERRSOCKET};
static mysockaddr_t clientaddress[MAXNETNODES+1];
static mysockaddr_t broadcastaddress[MAXNETNODES+1];
static size_t broadcastaddresses = 0;
static boolean nodeconnected[MAXNETNODES+1];
static mysockaddr_t *banned;
static UINT8 *bannedmask;

static size_t numbans = 0;
static boolean SOCK_bannednode[MAXNETNODES+1]; /// \note do we really need the +1?
static boolean init_tcp_driver = false;

static const char *serverport_name = DEFAULTPORT;
static const char *clientport_name;/* any port */

#ifdef USE_WINSOCK
// stupid microsoft makes things complicated
static char *get_WSAErrorStr(int e)
{
	static char buf[256]; // allow up to 255 bytes

	buf[0] = '\0';

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		(DWORD)e,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)buf,
		sizeof (buf),
		NULL);

	if (!buf[0]) // provide a fallback error message if no message is available for some reason
		sprintf(buf, "Unknown error");

	return buf;
}
#undef strerror
#define strerror get_WSAErrorStr
#endif

#ifdef USE_WINSOCK2
#define inet_ntop inet_ntopA
#define HAVE_NTOP
static const char* inet_ntopA(short af, const void *cp, char *buf, socklen_t len)
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
#elif !defined (USE_WINSOCK1)
#define HAVE_NTOP
#endif

#ifdef HAVE_MINIUPNPC // based on old XChat patch
static void I_ShutdownUPnP(void);
static void I_InitUPnP(void);
static I_mutex upnp_mutex;
static struct UPNPUrls urls;
static struct IGDdatas data;
static char lanaddr[64];
struct upnpdata
{
	int upnpc_started;
};
static struct upnpdata *upnpuser;
static void init_upnpc_once(struct upnpdata *upnpdata);

static void I_InitUPnP(void)
{
	upnpuser = malloc(sizeof *upnpuser);
	upnpuser->upnpc_started = 0;
	I_spawn_thread("init_upnpc_once", (I_thread_fn)init_upnpc_once, upnpuser);
}

static void
init_upnpc_once(struct upnpdata *upnpuserdata)
{

	if (upnpuserdata->upnpc_started != 0)
		return;

	I_lock_mutex(&upnp_mutex);
	const char * const deviceTypes[] = {
		"urn:schemas-upnp-org:device:InternetGatewayDevice:2",
		"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
		0
	};
	struct UPNPDev * devlist = NULL;
	int upnp_error = -2;
	int scope_id = 0;
	int status_code = 0;

	memset(&urls, 0, sizeof(struct UPNPUrls));
	memset(&data, 0, sizeof(struct IGDdatas));

	I_OutputMsg(M_GetText("Looking for UPnP Internet Gateway Device\n"));
	devlist = upnpDiscoverDevices(deviceTypes, 500, NULL, NULL, 0, false, 2, &upnp_error, 0);
	if (devlist)
	{
		struct UPNPDev *dev = devlist;
		char * descXML;
		int descXMLsize = 0;
		while (dev)
		{
			if (strstr (dev->st, "InternetGatewayDevice"))
				break;
			dev = dev->pNext;
		}
		if (!dev)
			dev = devlist; /* defaulting to first device */

		I_OutputMsg(M_GetText("Found UPnP device:\n desc: %s\n st: %s\n"),
		           dev->descURL, dev->st);

#if (MINIUPNPC_API_VERSION >= 18)
		UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr), NULL, 0);
#else
		UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
#endif
		I_OutputMsg(M_GetText("Local LAN IP address: %s\n"), lanaddr);
		descXML = miniwget(dev->descURL, &descXMLsize, scope_id, &status_code);
		if (descXML)
		{
			parserootdesc(descXML, descXMLsize, &data);
			free(descXML);
			descXML = NULL;
			GetUPNPUrls(&urls, &data, dev->descURL, status_code);
			I_AddExitFunc(I_ShutdownUPnP);
		}
		freeUPNPDevlist(devlist);
	}
	else if (upnp_error == UPNPDISCOVER_SOCKET_ERROR)
	{
		I_OutputMsg(M_GetText("No UPnP devices discovered\n"));
	}
	I_unlock_mutex(upnp_mutex);
	upnpuserdata->upnpc_started =1;
}

static inline void I_UPnP_add(const char * addr, const char *port, const char * servicetype)
{
	if (!urls.controlURL || urls.controlURL[0] == '\0')
		return;
	I_lock_mutex(&upnp_mutex);
	if (addr == NULL)
		addr = lanaddr;
	UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
	                    port, port, addr, "SRB2", servicetype, NULL, NULL);
	I_unlock_mutex(upnp_mutex);
}

static inline void I_UPnP_rem(const char *port, const char * servicetype)
{
	if (!urls.controlURL || urls.controlURL[0] == '\0')
		return;
	I_lock_mutex(&upnp_mutex);
	UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype,
	                       port, servicetype, NULL);
	I_unlock_mutex(upnp_mutex);
}

static void I_ShutdownUPnP(void)
{
	I_UPnP_rem(serverport_name, "UDP");
	I_lock_mutex(&upnp_mutex);
	FreeUPNPUrls(&urls);
	I_unlock_mutex(upnp_mutex);
}
#endif

static const char *SOCK_AddrToStr(mysockaddr_t *sk)
{
	static char s[64]; // 255.255.255.255:65535 or
	// [ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65535
#ifdef HAVE_NTOP
#ifdef HAVE_IPV6
	int v6 = (sk->any.sa_family == AF_INET6);
#else
	int v6 = 0;
#endif
	void *addr;

	if(sk->any.sa_family == AF_INET)
		addr = &sk->ip4.sin_addr;
#ifdef HAVE_IPV6
	else if(sk->any.sa_family == AF_INET6)
		addr = &sk->ip6.sin6_addr;
#endif
	else
		addr = NULL;

	if(addr == NULL)
		sprintf(s, "No address");
	else if(inet_ntop(sk->any.sa_family, addr, &s[v6], sizeof (s) - v6) == NULL)
		sprintf(s, "Unknown family type, error #%u", errno);
#ifdef HAVE_IPV6
	else if(sk->any.sa_family == AF_INET6)
	{
		s[0] = '[';
		strcat(s, "]");

		if (sk->ip6.sin6_port != 0)
			strcat(s, va(":%d", ntohs(sk->ip6.sin6_port)));
	}
#endif
	else if(sk->any.sa_family == AF_INET  && sk->ip4.sin_port  != 0)
		strcat(s, va(":%d", ntohs(sk->ip4.sin_port)));

#else
	if (sk->any.sa_family == AF_INET)
	{
		strcpy(s, inet_ntoa(sk->ip4.sin_addr));
		if (sk->ip4.sin_port != 0) strcat(s, va(":%d", ntohs(sk->ip4.sin_port)));
	}
	else
		sprintf(s, "Unknown type");
#endif
	return s;
}

static const char *SOCK_GetNodeAddress(INT32 node)
{
	if (node == 0)
		return "self";
	if (!nodeconnected[node])
		return NULL;
	return SOCK_AddrToStr(&clientaddress[node]);
}

static const char *SOCK_GetBanAddress(size_t ban)
{
	if (ban >= numbans)
		return NULL;
	return SOCK_AddrToStr(&banned[ban]);
}

static const char *SOCK_GetBanMask(size_t ban)
{
	static char s[16]; //255.255.255.255 netmask? no, just CDIR for only
	if (ban >= numbans)
		return NULL;
	if (sprintf(s,"%d",bannedmask[ban]) > 0)
		return s;
	return NULL;
}

#ifdef HAVE_IPV6
static boolean SOCK_cmpipv6(mysockaddr_t *a, mysockaddr_t *b, UINT8 mask)
{
	UINT8 bitmask;
	I_Assert(mask <= 128);
	if (memcmp(&a->ip6.sin6_addr.s6_addr, &b->ip6.sin6_addr.s6_addr, mask / 8) != 0)
		return false;
	if (mask % 8 == 0)
		return true;
	bitmask = 255 << (mask % 8);
	return (a->ip6.sin6_addr.s6_addr[mask / 8] & bitmask) == (b->ip6.sin6_addr.s6_addr[mask / 8] & bitmask);
}
#endif

static boolean SOCK_cmpaddr(mysockaddr_t *a, mysockaddr_t *b, UINT8 mask)
{
	UINT32 bitmask = INADDR_NONE;

	if (a->any.sa_family != b->any.sa_family)
		return false;

	if (mask && mask < 32)
		bitmask = htonl((UINT32)(-1) << (32 - mask));

	if (b->any.sa_family == AF_INET)
		return (a->ip4.sin_addr.s_addr & bitmask) == (b->ip4.sin_addr.s_addr & bitmask)
			&& (b->ip4.sin_port == 0 || (a->ip4.sin_port == b->ip4.sin_port));
#ifdef HAVE_IPV6
	else if (b->any.sa_family == AF_INET6)
		return SOCK_cmpipv6(a, b, mask)
			&& (b->ip6.sin6_port == 0 || (a->ip6.sin6_port == b->ip6.sin6_port));
#endif
	else
		return false;
}

// This is a hack. For some reason, nodes aren't being freed properly.
// This goes through and cleans up what nodes were supposed to be freed.
/** \warning This function causes the file downloading to stop if someone joins.
  *          How? Because it removes nodes that are connected but not in game,
  *          which is exactly what clients downloading a file are.
  */
static void cleanupnodes(void)
{
	if (!Playing())
		return;

	// Why can't I start at zero?
	for (SINT8 j = 1; j < MAXNETNODES; j++)
		if (!(netnodes[j].ingame || SendingFile(j)))
			nodeconnected[j] = false;
}

static SINT8 getfreenode(void)
{
	cleanupnodes();

	for (SINT8 j = 0; j < MAXNETNODES; j++)
		if (!nodeconnected[j])
		{
			nodeconnected[j] = true;
			return j;
		}

	/** \warning No free node? Just in case a node might not have been freed properly,
	  *          look if there are connected nodes that aren't in game, and forget them.
	  *          It's dirty, and might result in a poor guy having to restart
	  *          downloading a needed wad, but it's better than not letting anyone join...
	  */
	/*I_Error("No more free nodes!!1!11!11!!1111\n");
	for (SINT8 j = 1; j < MAXNETNODES; j++)
		if (!netnodes[j].ingame)
			return j;*/

	return -1;
}

#ifdef _DEBUG
void Command_Numnodes(void)
{
	INT32 connected = 0;
	INT32 ingame = 0;

	for (INT32 i = 1; i < MAXNETNODES; i++)
	{
		if (!(nodeconnected[i] || netnodes[i].ingame))
			continue;

		if (nodeconnected[i])
			connected++;
		if (netnodes[i].ingame)
			ingame++;

		CONS_Printf("%2d - ", i);
		if (netnodes[i].player != -1)
			CONS_Printf("player %.2d", netnodes[i].player);
		else
			CONS_Printf("         ");
		if (nodeconnected[i])
			CONS_Printf(" - connected");
		else
			CONS_Printf(" -          ");
		if (netnodes[i].ingame)
			CONS_Printf(" - ingame");
		else
			CONS_Printf(" -       ");
		CONS_Printf(" - %s\n", I_GetNodeAddress(i));
	}

	CONS_Printf("\n"
				"Connected: %d\n"
				"Ingame:    %d\n",
				connected, ingame);
}
#endif

// Returns true if a packet was received from a new node, false in all other cases
static boolean SOCK_Get(void)
{
	size_t i;
	int j;
	ssize_t c;
	mysockaddr_t fromaddress;
	socklen_t fromlen;

	for (size_t n = 0; n < mysocketses; n++)
	{
		fromlen = (socklen_t)sizeof(fromaddress);
		c = recvfrom(mysockets[n], (char *)&doomcom->data, MAXPACKETLENGTH, 0,
			(void *)&fromaddress, &fromlen);
		if (c != ERRSOCKET)
		{
			// find remote node number
			for (j = 1; j <= MAXNETNODES; j++) //include LAN
			{
				if (SOCK_cmpaddr(&fromaddress, &clientaddress[j], 0))
				{
					doomcom->remotenode = (INT16)j; // good packet from a game player
					doomcom->datalength = (INT16)c;
					nodesocket[j] = mysockets[n];
					return false;
				}
			}
			// not found

			// find a free slot
			j = getfreenode();
			if (j > 0)
			{
				M_Memcpy(&clientaddress[j], &fromaddress, fromlen);
				nodesocket[j] = mysockets[n];
				DEBFILE(va("New node detected: node:%d address:%s\n", j,
						SOCK_GetNodeAddress(j)));
				doomcom->remotenode = (INT16)j; // good packet from a game player
				doomcom->datalength = (INT16)c;

				// check if it's a banned dude so we can send a refusal later
				for (i = 0; i < numbans; i++)
				{
					if (SOCK_cmpaddr(&fromaddress, &banned[i], bannedmask[i]))
					{
						SOCK_bannednode[j] = true;
						DEBFILE("This dude has been banned\n");
						break;
					}
				}
				if (i == numbans)
					SOCK_bannednode[j] = false;
				return true;
			}
			else
				DEBFILE("New node detected: No more free slots\n");
		}
	}

	doomcom->remotenode = -1; // no packet
	return false;
}

static inline ssize_t SOCK_SendToAddr(SOCKET_TYPE socket, mysockaddr_t *sockaddr)
{
	socklen_t d4 = (socklen_t)sizeof(struct sockaddr_in);
#ifdef HAVE_IPV6
	socklen_t d6 = (socklen_t)sizeof(struct sockaddr_in6);
#endif
	socklen_t d, da = (socklen_t)sizeof(mysockaddr_t);

	switch (sockaddr->any.sa_family)
	{
		case AF_INET:  d = d4; break;
#ifdef HAVE_IPV6
		case AF_INET6: d = d6; break;
#endif
		default:       d = da; break;
	}

	return sendto(socket, (char *)&doomcom->data, doomcom->datalength, 0, &sockaddr->any, d);
}

#define ALLOWEDERROR(x) ((x) == ECONNREFUSED || (x) == EWOULDBLOCK || (x) == EHOSTUNREACH || (x) == ENETUNREACH)

static void SOCK_Send(void)
{
	ssize_t c = ERRSOCKET;

	if (!nodeconnected[doomcom->remotenode])
		return;

	if (doomcom->remotenode == BROADCASTADDR)
	{
		for (size_t i = 0; i < mysocketses; i++)
		{
			for (size_t j = 0; j < broadcastaddresses; j++)
			{
				if (myfamily[i] == broadcastaddress[j].any.sa_family)
				{
					c = SOCK_SendToAddr(mysockets[i], &broadcastaddress[j]);
					if (c == ERRSOCKET && !ALLOWEDERROR(errno))
						break;
				}
			}
		}
	}
	else if (nodesocket[doomcom->remotenode] == (SOCKET_TYPE)ERRSOCKET)
	{
		for (size_t i = 0; i < mysocketses; i++)
		{
			if (myfamily[i] == clientaddress[doomcom->remotenode].any.sa_family)
			{
				c = SOCK_SendToAddr(mysockets[i], &clientaddress[doomcom->remotenode]);
				if (c == ERRSOCKET && !ALLOWEDERROR(errno))
					break;
			}
		}
	}
	else
	{
		c = SOCK_SendToAddr(nodesocket[doomcom->remotenode], &clientaddress[doomcom->remotenode]);
	}

	if (c == ERRSOCKET)
	{
		int e = errno; // save error code so it can't be modified later
		if (!ALLOWEDERROR(e))
			I_Error("SOCK_Send, error sending to node %d (%s) #%u, %s", doomcom->remotenode,
				SOCK_GetNodeAddress(doomcom->remotenode), e, strerror(e));
	}
}

#undef ALLOWEDERROR

static void SOCK_FreeNodenum(INT32 numnode)
{
	// can't disconnect from self :)
	if (!numnode || numnode > MAXNETNODES)
		return;

	DEBFILE(va("Free node %d (%s)\n", numnode, SOCK_GetNodeAddress(numnode)));

	nodeconnected[numnode] = false;
	nodesocket[numnode] = ERRSOCKET;

	// put invalid address
	memset(&clientaddress[numnode], 0, sizeof (clientaddress[numnode]));
}

//
// UDPsocket
//

// allocate a socket
static SOCKET_TYPE UDP_Bind(int family, struct sockaddr *addr, socklen_t addrlen)
{
	SOCKET_TYPE s = socket(family, SOCK_DGRAM, IPPROTO_UDP);
	int opt;
	int rc;
	int e = 0; // save error code so it can't be modified later code
	socklen_t opts;
#ifdef FIONBIO
	unsigned long trueval = true;
#endif
	mysockaddr_t straddr;
	socklen_t len = sizeof(straddr);

	if (s == (SOCKET_TYPE)ERRSOCKET)
		return (SOCKET_TYPE)ERRSOCKET;
#ifdef USE_WINSOCK
	{ // Alam_GBC: disable the new UDP connection reset behavior for Win2k and up
#ifdef USE_WINSOCK2
		DWORD dwBytesReturned = 0;
		BOOL bfalse = FALSE;
		rc = WSAIoctl(s, SIO_UDP_CONNRESET, &bfalse, sizeof(bfalse),
		         NULL, 0, &dwBytesReturned, NULL, NULL);
#else
		unsigned long falseval = false;
		rc = ioctl(s, SIO_UDP_CONNRESET, &falseval);
#endif
		if (rc == -1)
		{
			e = errno;
			I_OutputMsg("SIO_UDP_CONNRESET failed: #%u, %s\n", e, strerror(e));
		}
	}
#endif

	memcpy(&straddr, addr, addrlen);
	I_OutputMsg("Binding to %s\n", SOCK_AddrToStr(&straddr));

	if (family == AF_INET)
	{
		if (straddr.ip4.sin_addr.s_addr == htonl(INADDR_ANY))
		{
			opt = true;
			opts = (socklen_t)sizeof(opt);
			rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, opts);
			if (rc <= -1)
			{
				e = errno;
				I_OutputMsg("setting SO_REUSEADDR failed: #%u, %s\n", e, strerror(e));
			}
		}
		// make it broadcastable
		opt = true;
		opts = (socklen_t)sizeof(opt);
		rc = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&opt, opts);
		if (rc <= -1)
		{
			e = errno;
			CONS_Alert(CONS_WARNING, M_GetText("Could not get broadcast rights\n")); // I do not care anymore
			I_OutputMsg("setting SO_BROADCAST failed: #%u, %s\n", e, strerror(e));
		}
	}
#ifdef HAVE_IPV6
	else if (family == AF_INET6)
	{
		if (memcmp(&straddr.ip6.sin6_addr, &in6addr_any, sizeof(in6addr_any)) == 0) //IN6_ARE_ADDR_EQUAL
		{
			opt = true;
			opts = (socklen_t)sizeof(opt);
			rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, opts);
			if (rc <= -1)
			{
				e = errno;
				I_OutputMsg("setting SO_REUSEADDR failed: #%u, %s\n", e, strerror(e));
			}
		}
#ifdef IPV6_V6ONLY
		// make it IPv6 ony
		opt = true;
		opts = (socklen_t)sizeof(opt);
		rc = setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, opts);
		if (rc <= -1)
		{
			e = errno;
			CONS_Alert(CONS_WARNING, M_GetText("Could not limit IPv6 bind\n")); // I do not care anymore
			I_OutputMsg("setting IPV6_V6ONLY failed: #%u, %s\n", e, strerror(e));
		}
#endif
	}
#endif

	rc = bind(s, addr, addrlen);
	if (rc == ERRSOCKET)
	{
		e = errno;
		close(s);
		I_OutputMsg("Binding failed: #%u, %s\n", e, strerror(e));
		return (SOCKET_TYPE)ERRSOCKET;
	}

#ifdef HAVE_IPV6
	if (family == AF_INET6)
	{
		// we need to set all of this *after* binding to an address!
		if (memcmp(&straddr.ip6.sin6_addr, &in6addr_any, sizeof(in6addr_any)) == 0) //IN6_ARE_ADDR_EQUAL
		{
			struct ipv6_mreq maddr;

			inet_pton(AF_INET6, IPV6_MULTICAST_ADDRESS, &maddr.ipv6mr_multiaddr);
			maddr.ipv6mr_interface = 0;
			rc = setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char *)&maddr, sizeof(maddr));
			if (rc <= -1)
			{
				e = errno;
				CONS_Alert(CONS_WARNING, M_GetText("Could not register multicast address\n"));
				if (e == ENODEV)
				{
					close(s);
					I_OutputMsg("Binding failed: no IPv6 device\n");
					return (SOCKET_TYPE)ERRSOCKET;
				}
				I_OutputMsg("setting IPV6_JOIN_GROUP failed: #%u, %s \n", e, strerror(e));
			}
		}
	}
#endif

#ifdef FIONBIO
	// make it non blocking
	rc = ioctl(s, FIONBIO, &trueval);
	if (rc == -1)
	{
		e = errno;
		close(s);
		I_OutputMsg("FIOBIO failed: #%u, %s\n", e, strerror(e));
		return (SOCKET_TYPE)ERRSOCKET;
	}
#endif

	opt = 0;
	opts = (socklen_t)sizeof(opt);
	rc = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, &opts);
	if (rc <= -1)
	{
		e = errno;
		I_OutputMsg("getting SO_RCVBUF failed: #%u, %s\n", e, strerror(e));
	}
	CONS_Printf(M_GetText("Network system buffer: %dKb\n"), opt>>10);

	if (opt < 64<<10) // 64k
	{
		opt = 64<<10;
		opts = (socklen_t)sizeof(opt);
		rc = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, opts);
		if (rc <= -1)
		{
			e = errno;
			I_OutputMsg("setting SO_RCVBUF failed: #%u, %s\n", e, strerror(e));
		}
		opt = 0;
		rc = getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&opt, &opts);
		if (rc <= -1)
		{
			e = errno;
			I_OutputMsg("getting SO_RCVBUF failed: #%u, %s\n", e, strerror(e));
		}
		if (opt <= 64<<10)
			CONS_Alert(CONS_WARNING, M_GetText("Can't set buffer length to 64k, file transfer will be bad\n"));
		else
			CONS_Printf(M_GetText("Network system buffer set to: %dKb\n"), opt>>10);
	}

	rc = getsockname(s, &straddr.any, &len);
	if (rc != 0)
	{
		e = errno;
		CONS_Alert(CONS_WARNING, M_GetText("Failed to get port number\n"));
		I_OutputMsg("getsockname failed: #%u, %s\n", e, strerror(e));
	}
	else
	{
		if (family == AF_INET)
			current_port = (UINT16)ntohs(straddr.ip4.sin_port);
#ifdef HAVE_IPV6
		else if (family == AF_INET6)
			current_port = (UINT16)ntohs(straddr.ip6.sin6_port);
#endif
	}

	return s;
}

static boolean UDP_Socket(void)
{
	size_t s;
	struct my_addrinfo *ai, *runp, hints;
	int gaie;
#ifdef HAVE_IPV6
	const INT32 b_ipv6 = !M_CheckParm("-noipv6");
#endif
	const char *serv;


	for (s = 0; s < mysocketses; s++)
		mysockets[s] = ERRSOCKET;
	for (s = 0; s < MAXNETNODES+1; s++)
		nodesocket[s] = ERRSOCKET;
	s = 0;

	memset(&hints, 0x00, sizeof (hints));
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (serverrunning)
		serv = serverport_name;
	else
		serv = clientport_name;

	if (M_CheckParm("-bindaddr"))
	{
		while (M_IsNextParm())
		{
			gaie = I_getaddrinfo(M_GetNextParm(), serv, &hints, &ai);
			if (gaie == 0)
			{
				runp = ai;
				while (runp != NULL && s < MAXNETNODES+1)
				{
					mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
					if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
					{
						myfamily[s] = hints.ai_family;
						s++;
					}
					runp = runp->ai_next;
				}
				I_freeaddrinfo(ai);
			}
		}
	}
	else
	{
		gaie = I_getaddrinfo("0.0.0.0", serv, &hints, &ai);
		if (gaie == 0)
		{
			runp = ai;
			while (runp != NULL && s < MAXNETNODES+1)
			{
				mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
				if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
				{
					myfamily[s] = hints.ai_family;
					s++;
#ifdef HAVE_MINIUPNPC
					if (UPNP_support)
					{
						I_UPnP_rem(serverport_name, "UDP");
						I_UPnP_add(NULL, serverport_name, "UDP");
					}
#endif
				}
				runp = runp->ai_next;
			}
			I_freeaddrinfo(ai);
		}
	}
#ifdef HAVE_IPV6
	if (b_ipv6)
	{
		hints.ai_family = AF_INET6;
		if (M_CheckParm("-bindaddr6"))
		{
			while (M_IsNextParm())
			{
				gaie = I_getaddrinfo(M_GetNextParm(), serv, &hints, &ai);
				if (gaie == 0)
				{
					runp = ai;
					while (runp != NULL && s < MAXNETNODES+1)
					{
						mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
						if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
						{
							myfamily[s] = hints.ai_family;
							s++;
						}
						runp = runp->ai_next;
					}
					I_freeaddrinfo(ai);
				}
			}
		}
		else
		{
			gaie = I_getaddrinfo("::", serv, &hints, &ai);
			if (gaie == 0)
			{
				runp = ai;
				while (runp != NULL && s < MAXNETNODES+1)
				{
					mysockets[s] = UDP_Bind(runp->ai_family, runp->ai_addr, (socklen_t)runp->ai_addrlen);
					if (mysockets[s] != (SOCKET_TYPE)ERRSOCKET)
					{
						myfamily[s] = hints.ai_family;
						s++;
					}
					runp = runp->ai_next;
				}
				I_freeaddrinfo(ai);
			}
		}
	}
#endif

	mysocketses = s;
	if (s == 0) // no sockets?
		return false;

	s = 0;

	// ip + udp
	packetheaderlength = 20 + 8; // for stats

	clientaddress[s].any.sa_family = AF_INET;
	clientaddress[s].ip4.sin_port = htons(0);
	clientaddress[s].ip4.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //GetLocalAddress(); // my own ip
	s++;

	s = 0;

	// setup broadcast adress to BROADCASTADDR entry
	broadcastaddress[s].any.sa_family = AF_INET;
	broadcastaddress[s].ip4.sin_port = htons(atoi(DEFAULTPORT));
	broadcastaddress[s].ip4.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	s++;

#ifdef HAVE_IPV6
	if (b_ipv6)
	{
		broadcastaddress[s].any.sa_family = AF_INET6;
		broadcastaddress[s].ip6.sin6_port = htons(atoi(DEFAULTPORT));
		broadcastaddress[s].ip6.sin6_flowinfo = 0;
		inet_pton(AF_INET6, IPV6_MULTICAST_ADDRESS, &broadcastaddress[s].ip6.sin6_addr);
		broadcastaddress[s].ip6.sin6_scope_id = 0;
		s++;
	}
#endif

	broadcastaddresses = s;

	doomcom->extratics = 1; // internet is very high ping

	return true;
}

boolean I_InitTcpDriver(void)
{
	boolean tcp_was_up = init_tcp_driver;
	if (!init_tcp_driver)
	{
#ifdef USE_WINSOCK
#ifdef USE_WINSOCK2
		const WORD VerNeed = MAKEWORD(2,2);
#else
		const WORD VerNeed = MAKEWORD(1,1);
#endif
		WSADATA WSAData;
		const int WSAresult = WSAStartup(VerNeed, &WSAData);
		if (WSAresult != 0)
		{
			LPCSTR WSError = NULL;
			switch (WSAresult)
			{
				case WSASYSNOTREADY:
					WSError = "The underlying network subsystem is not ready for network communication";
					break;
				case WSAEINPROGRESS:
					WSError = "A blocking Windows Sockets 1.1 operation is in progress";
					break;
				case WSAEPROCLIM:
					WSError = "Limit on the number of tasks supported by the Windows Sockets implementation has been reached";
					break;
				case WSAEFAULT:
					WSError = "WSAData is not a valid pointer? What kind of setup do you have?";
					break;
				default:
					WSError = va("Error code %u",WSAresult);
					break;
			}
			if (WSAresult != WSAVERNOTSUPPORTED)
				CONS_Debug(DBG_NETPLAY, "WinSock(TCP/IP) error: %s\n",WSError);
		}
#ifdef USE_WINSOCK2
		if(LOBYTE(WSAData.wVersion) != 2 ||
			HIBYTE(WSAData.wVersion) != 2)
		{
			WSACleanup();
			CONS_Debug(DBG_NETPLAY, "No WinSock(TCP/IP) 2.2 driver detected\n");
		}
#else
		if (LOBYTE(WSAData.wVersion) != 1 ||
			HIBYTE(WSAData.wVersion) != 1)
		{
			WSACleanup();
			CONS_Debug(DBG_NETPLAY, "No WinSock(TCP/IP) 1.1 driver detected\n");
		}
#endif
		CONS_Debug(DBG_NETPLAY, "WinSock description: %s\n",WSAData.szDescription);
		CONS_Debug(DBG_NETPLAY, "WinSock System Status: %s\n",WSAData.szSystemStatus);
#endif
		init_tcp_driver = true;
	}

	if (!tcp_was_up && init_tcp_driver)
	{
		I_AddExitFunc(I_ShutdownTcpDriver);
#ifdef HAVE_MINIUPNPC
		if (M_CheckParm("-useUPnP"))
			I_InitUPnP();
		else
			UPNP_support = false;
#endif
	}
	return init_tcp_driver;
}

static void SOCK_CloseSocket(void)
{
	for (size_t i=0; i < mysocketses; i++)
	{
		if (mysockets[i] != (SOCKET_TYPE)ERRSOCKET)
			close(mysockets[i]);
		mysockets[i] = ERRSOCKET;
	}
	mysocketses = 0;
}

void I_ShutdownTcpDriver(void)
{
	SOCK_CloseSocket();

	CONS_Printf("I_ShutdownTcpDriver: ");
#ifdef USE_WINSOCK
	WS_addrinfocleanup();
	WSACleanup();
#endif
	CONS_Printf("shut down\n");
	init_tcp_driver = false;
}

static SINT8 SOCK_NetMakeNodewPort(const char *address, const char *port)
{
	SINT8 newnode = -1;
	struct my_addrinfo *ai = NULL, *runp, hints;
	int gaie;
	size_t i;

	 if (!port || !port[0])
		port = DEFAULTPORT;

	DEBFILE(va("Creating new node: %s@%s\n", address, port));

	memset (&hints, 0x00, sizeof (hints));
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	gaie = I_getaddrinfo(address, port, &hints, &ai);
	if (gaie == 0)
	{
		newnode = getfreenode();
	}
	if (newnode == -1)
	{
		I_freeaddrinfo(ai);
		return -1;
	}
	else
		runp = ai;

	while (runp != NULL)
	{
		// test ip address of server
		for (i = 0; i < mysocketses; ++i)
		{
			if (runp->ai_addr->sa_family == myfamily[i])
			{
				memcpy(&clientaddress[newnode], runp->ai_addr, runp->ai_addrlen);
				break;
			}
		}

		if (i < mysocketses)
			runp = runp->ai_next;
		else
			break;
	}
	I_freeaddrinfo(ai);
	return newnode;
}

static boolean SOCK_OpenSocket(void)
{
	memset(clientaddress, 0, sizeof (clientaddress));

	nodeconnected[0] = true; // always connected to self
	for (size_t i = 1; i < MAXNETNODES; i++)
		nodeconnected[i] = false;
	nodeconnected[BROADCASTADDR] = true;
	I_NetSend = SOCK_Send;
	I_NetGet = SOCK_Get;
	I_NetCloseSocket = SOCK_CloseSocket;
	I_NetFreeNodenum = SOCK_FreeNodenum;
	I_NetMakeNodewPort = SOCK_NetMakeNodewPort;

	// build the socket but close it first
	SOCK_CloseSocket();
	return UDP_Socket();
}

static boolean SOCK_Ban(INT32 node)
{
	if (node > MAXNETNODES)
		return false;

	banned = Z_Realloc(banned, sizeof(*banned) * (numbans+1), PU_STATIC, NULL);
	bannedmask = Z_Realloc(bannedmask, sizeof(*bannedmask) * (numbans+1), PU_STATIC, NULL);
	M_Memcpy(&banned[numbans], &clientaddress[node], sizeof (mysockaddr_t));
	if (banned[numbans].any.sa_family == AF_INET)
	{
		banned[numbans].ip4.sin_port = 0;
		bannedmask[numbans] = 32;
	}
#ifdef HAVE_IPV6
	else if (banned[numbans].any.sa_family == AF_INET6)
	{
		banned[numbans].ip6.sin6_port = 0;
		bannedmask[numbans] = 64;
	}
#endif
	numbans++;
	return true;
}

static boolean SOCK_SetBanAddress(const char *address, const char *mask)
{
	struct my_addrinfo *ai, *runp, hints;
	int gaie;

	if (!address)
		return false;

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags = 0;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	gaie = I_getaddrinfo(address, "0", &hints, &ai);
	if (gaie != 0)
		return false;

	runp = ai;

	while(runp != NULL)
	{
		banned = Z_Realloc(banned, sizeof(*banned) * (numbans+1), PU_STATIC, NULL);
		bannedmask = Z_Realloc(bannedmask, sizeof(*bannedmask) * (numbans+1), PU_STATIC, NULL);
		memcpy(&banned[numbans], runp->ai_addr, runp->ai_addrlen);

		if (mask)
			bannedmask[numbans] = (UINT8)atoi(mask);
#ifdef HAVE_IPV6
		else if (runp->ai_family == AF_INET6)
			bannedmask[numbans] = 64;
#endif
		else
			bannedmask[numbans] = 32;

		if (bannedmask[numbans] > 32 && runp->ai_family == AF_INET)
			bannedmask[numbans] = 32;
#ifdef HAVE_IPV6
		else if (bannedmask[numbans] > 128 && runp->ai_family == AF_INET6)
			bannedmask[numbans] = 64;
#endif
		numbans++;
		runp = runp->ai_next;
	}

	I_freeaddrinfo(ai);

	return true;
}

static void SOCK_ClearBans(void)
{
	numbans = 0;
	Z_Free(banned);
	banned = NULL;
	Z_Free(bannedmask);
	bannedmask = NULL;
}

boolean I_InitTcpNetwork(void)
{
	char serverhostname[255];
	const char *urlparam = NULL;
	boolean ret = false;
	// initilize the OS's TCP/IP stack
	if (!I_InitTcpDriver())
		return false;

	if (M_CheckParm("-port") || M_CheckParm("-serverport"))
	// Combined -udpport and -clientport into -port
	// As it was really redundant having two seperate parms that does the same thing
	/* Sorry Steel, I'm adding these back. But -udpport is a stupid name. */
	{
		/*
		If it's NULL, that's okay! Because then
		we'll get a random port from getaddrinfo.
		*/
		serverport_name = M_GetNextParm();
	}
	if (M_CheckParm("-clientport"))
		clientport_name = M_GetNextParm();

	// parse network game options,
	if (M_CheckParm("-server") || dedicated)
	{
		server = true;

		// If a number of clients (i.e. nodes) is specified, the server will wait for the clients
		// to connect before starting.
		// If no number is specified here, the server starts with 1 client, and others can join
		// in-game.
		// Since Boris has implemented join in-game, there is no actual need for specifying a
		// particular number here.
		// FIXME: for dedicated server, numnodes needs to be set to 0 upon start
		if (dedicated)
			doomcom->numnodes = 0;
/*		else if (M_IsNextParm())
			doomcom->numnodes = (INT16)atoi(M_GetNextParm());*/
		else
			doomcom->numnodes = 1;

		if (doomcom->numnodes < 0)
			doomcom->numnodes = 0;
		if (doomcom->numnodes > MAXNETNODES)
			doomcom->numnodes = MAXNETNODES;

		// server
		servernode = 0;
		// FIXME:
		// ??? and now ?
		// server on a big modem ??? 4*isdn
		hardware_MAXPACKETLENGTH = INETPACKETLENGTH;

		ret = true;
	}
	else if ((urlparam = M_GetUrlProtocolArg()) != NULL || M_CheckParm("-connect"))
	{
		if (urlparam != NULL)
			strlcpy(serverhostname, urlparam, sizeof(serverhostname));
		else if (M_IsNextParm())
			strlcpy(serverhostname, M_GetNextParm(), sizeof(serverhostname));
		else
			serverhostname[0] = 0; // assuming server in the LAN, use broadcast to detect it

		// server address only in ip
		if (serverhostname[0])
		{
			COM_BufAddText("connect \"");
			COM_BufAddText(serverhostname);
			COM_BufAddText("\"\n");

			// probably modem
			hardware_MAXPACKETLENGTH = INETPACKETLENGTH;
		}
		else
		{
			// so we're on a LAN
			COM_BufAddText("connect any\n");

			hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
		}
	}

	I_NetOpenSocket = SOCK_OpenSocket;
	I_Ban = SOCK_Ban;
	I_ClearBans = SOCK_ClearBans;
	I_GetNodeAddress = SOCK_GetNodeAddress;
	I_GetBanAddress = SOCK_GetBanAddress;
	I_GetBanMask = SOCK_GetBanMask;
	I_SetBanAddress = SOCK_SetBanAddress;
	bannednode = SOCK_bannednode;

	return ret;
}

boolean Net_IsNodeIPv6(INT32 node)
{
#ifdef NO_IPV6
	return false;
#else
	return clientaddress[node].any.sa_family == AF_INET6;
#endif
}

#include "i_addrinfo.c"
