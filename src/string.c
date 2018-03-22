// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2006      by Graue.
// Copyright (C) 2006-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  string.c
/// \brief String functions that we need but are missing on some
///        operating systems.

#include <stddef.h>
#include <string.h>
#include "doomdef.h"

#if !defined (__APPLE__)

// Like the OpenBSD version, but it doesn't check for src not being a valid
// C string.
size_t strlcat(char *dst, const char *src, size_t siz)
{
	size_t dstlen, n = siz;
	char *p = dst;

	dstlen = strlen(dst);
	n -= dstlen;
	p += dstlen;

	while (n > 1 && *src != '\0')
	{
		*p++ = *src++;
		n--;
	}

	if (n >= 1)
		*p = '\0';

	return strlen(src) + dstlen;
}

size_t strlcpy(char *dst, const char *src, size_t siz)
{
	if (siz == 0)
		return strlen(dst);

	dst[0] = '\0';
	return strlcat(dst, src, siz);
}

#endif

#if ! (defined (_GNU_SOURCE) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 11)
char * strchrnul (const char *s, int c)
{
	char *p;

	// Grudgingly not discarding const.
	p = strchr(s, c);
	if (p == NULL)
		p = strchr(s, 0);

	return p;
}
#endif
