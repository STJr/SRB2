// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2018 by Sonic Team Jr.
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
// DESCRIPTION:
//       stub and replacement "ANSI" C functions for use on Dreamcast/KOS
//
//-----------------------------------------------------------------------------

#ifndef __I_DREAMCAST__
#define __I_DREAMCAST__

struct hostent
{
	char *h_name;			/* Official name of host.  */
	char **h_aliases;		/* Alias list.  */
	int h_addrtype;		/* Host address type.  */
	int h_length;			/* Length of address.  */
	char **h_addr_list;		/* List of addresses from name server.  */
#define	h_addr	h_addr_list[0]	/* Address, for backward compatibility.  */
};

struct hostent *gethostbyname(const char *name);

#ifndef HAVE_LWIP
#define INADDR_NONE     ((uint32) 0xffffffff)
#define INADDR_LOOPBACK ((uint32) 0x7f000001)
#define SOCK_STREAM     1
#define FIONBIO         0
#define SOL_SOCKET      0
#define SO_ERROR        0
#define SO_REUSEADDR    0
#define SO_BROADCAST    0
#define SO_RCVBUF       0
int ioctl(int s, long cmd, void *argp);
int select(int maxfdp1, void *readset, void *writeset, void *exceptset, void *timeout);
int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int s, int level, int optname, void *optval, socklen_t optlen);
#endif
#endif
