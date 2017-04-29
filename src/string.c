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

#if 1 // i don't know what specific OSes are missing this, oh well

// stack overflow, eep...
char* stristr(char* haystack, const char* needle)
{
    char* p1 = haystack ;
    const char* p2 = needle ;
    char* r = ((*p2 == 0) ? haystack : 0);

    while (*p1 != 0 && *p2 != 0)
    {
        if (tolower(*p1) == tolower(*p2))
        {
            if (r == 0)
                r = p1;

            p2++;
        }
        else
        {
            p2 = needle;
            if (tolower(*p1) == tolower(*p2))
            {
                r = p1;
                p2++;
            }
            else
                r = 0;
        }

        p1++;
    }

    return ((*p2 == 0) ? r : 0);
}

#endif