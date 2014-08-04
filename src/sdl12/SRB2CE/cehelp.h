// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2004 by Sonic Team Jr.
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
//      stub and replacement "ANSI" C functions for use under Windows CE
//
//-----------------------------------------------------------------------------

#ifndef __I_WINCE__
#define __I_WINCE__

#ifdef USEASMCE
#define USEASM // Remline if NASM doesn't work on x86 targets
#endif

char *strerror(int ecode);
int access(const char *path, int amode);
int unlink( const char *filename );
int remove( const char *path );
int rename( const char *oldname, const char *newname );
//CreateDirectoryEx( const char *currectpath, const char *path,SECURITY_ATTRIBUTES)

#ifndef _TIME_T_DEFINED
typedef long time_t;        /* time value */
#define _TIME_T_DEFINED     /* avoid multiple def's of time_t */
#endif

time_t time(time_t *T);

#ifndef __GNUC__
#ifndef _TM_DEFINED
struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };
#define _TM_DEFINED
#endif

struct tm * localtime(const time_t *CLOCK);

time_t mktime (struct tm *tim_p);
#endif

#endif
