// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006 by Sonic Team Jr.
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
//      stub and replacement "ANSI" C functions for use on Dreamcast/KOS
//
//-----------------------------------------------------------------------------
#include <kos/fs.h>
#include <errno.h>
#ifndef HAVE_LWIP
#include <sys/socket.h>
#endif
#include "../../doomdef.h"
#include "dchelp.h"

int access(const char *path, int amode)
{
	file_t handle = FILEHND_INVALID;

	if (amode == F_OK || amode == R_OK)
		handle=fs_open(path,O_RDONLY);
	else if (amode == (R_OK|W_OK))
		handle=fs_open(path,O_RDWR);
	else if (amode == W_OK)
		handle=fs_open(path,O_WRONLY);

	if (handle != FILEHND_INVALID)
	{
		fs_close(handle);
		return 0;
	}

	return -1;
}

double hypot(double x, double y)
{
	double ax, yx, yx2, yx1;
	if (abs(y) > abs(x)) // |y|>|x|
	{
		ax = abs(y); // |y| => ax
		yx = (x/y);
	}
	else // |x|>|y|
	{
		ax = abs(x); // |x| => ax
		yx = (x/y);
	}
	yx2 = yx*yx; // (x/y)^2
	yx1 = sqrt(1+yx2); // (1 + (x/y)^2)^1/2
	return ax*yx1; // |x|*((1 + (x/y)^2)^1/2)
}

#if !(defined (NONET) || defined (NOMD5))
#ifdef HAVE_LWIP

#include <lwip/lwip.h>

static uint8 ip[4];
static char *h_addr_listtmp[2] = {ip, NULL};
static struct hostent hostenttmp = {NULL, NULL, 0, 1, h_addr_listtmp};

struct hostent *gethostbyname(const char *name)
{
	struct sockaddr_in dnssrv;
	dnssrv.sin_family = AF_INET;
	dnssrv.sin_port = htons(53);
	dnssrv.sin_addr.s_addr = htonl(0x0a030202); ///< what?
	if (lwip_gethostbyname(&dnssrv, name, ip) < 0)
		return NULL;
	else
		return &hostenttmp;
}
#else

struct hostent *gethostbyname(const char *name)
{
	(void)name;
	return NULL;
}

int ioctl(int s, long cmd, void *argp)
{
	return fs_ioctl(s, argp, cmd); //FIONBIO?
}

int select(int maxfdp1, void *readset, void *writeset, void *exceptset,
                void *timeout)
{
	(void)maxfdp1;
	(void)readset;
	(void)writeset;
	(void)exceptset;
	(void)timeout;
	errno = EAFNOSUPPORT;
	return -1;
}

int getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen)
{
	(void)s;
	(void)level; //SOL_SOCKET
	(void)optname; //SO_RCVBUF, SO_ERROR
	(void)optval;
	(void)optlen;
	errno = EAFNOSUPPORT;
	return -1;
}

int setsockopt (int s, int level, int optname, void *optval, socklen_t optlen)
{
	(void)s;
	(void)level; //SOL_SOCKET
	(void)optname; //SO_REUSEADDR, SO_BROADCAST, SO_RCVBUF
	(void)optval;
	(void)optlen;
	errno = EAFNOSUPPORT;
	return -1;
}

#endif
#endif
