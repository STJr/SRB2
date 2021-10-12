// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2011-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_addrinfo.h
/// \brief getaddrinfo stub

#ifndef __I_ADDRINFO__
#define __I_ADDRINFO__

#ifdef __GNUG__
#pragma interface
#endif

#ifndef AI_PASSIVE
#define AI_PASSIVE     0x01
#endif
#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 0x04
#endif
#ifndef AI_V4MAPPED
#define AI_V4MAPPED    0x08
#endif
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG  0x20
#endif

#ifdef _WIN32
#ifndef EAI_NONAME
#define EAI_NONAME WSAHOST_NOT_FOUND
#endif
#endif

#ifndef EAI_NONAME
#define EAI_NONAME -2
#endif

#ifdef _WIN32 // already use the stub for Win32
// w32api, ws2tcpip.h, r1.12
struct my_addrinfo {
        int     ai_flags;
        int     ai_family;
        int     ai_socktype;
        int     ai_protocol;
        size_t  ai_addrlen;
        char   *ai_canonname;
        struct sockaddr  *ai_addr;
        struct my_addrinfo  *ai_next;
};
#else
#define my_addrinfo addrinfo
#endif

void WS_addrinfocleanup(void);

#ifndef my_addrinfo
void I_freeaddrinfo(struct my_addrinfo *res);
int I_getaddrinfo(const char *node, const char *service,
                         const struct my_addrinfo *hints,
                         struct my_addrinfo **res);
#elif !defined (test_stub)
#define I_getaddrinfo getaddrinfo
#define I_freeaddrinfo freeaddrinfo
#endif


#endif
