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

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "common.h"

// ================================== GLOBALS =================================

static const char *fatal_error_msg[NUM_FATAL_ERROR] =
{
	"Error: signal()",
	"Error: select()",
	"Error: read()",
	"Error: write()",
};

// used by xxxPrintf() functions as temporary variable
static char str[1024] ="";
static va_list arglist;
#ifdef _WIN32
static size_t len = 0;
static HANDLE co = INVALID_HANDLE_VALUE;
static DWORD bytesWritten = 0;
static CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
#endif

// ================================= FUNCTIONS ================================

/*
 * clearScreen():
 */
void clearScreen()
{
#ifdef _WIN32
#else
	printf("\033[0m \033[2J \033[0;0H");
#endif
}

/*
 * fatalError():
 */
void fatalError(fatal_error_t num)
{
	clearScreen();
	printf("\n");
	perror(fatal_error_msg[num]);
	printf("\n");
	exit(-1);
}


/*
 * dbgPrintf():
 */
#ifdef _WIN32
void dbgPrintf(DWORDLONG col, const char *lpFmt, ...)
#else
void dbgPrintf(const char *col, const char *lpFmt, ...)
#endif
{
#if defined (__DEBUG__)
	va_start(arglist, lpFmt);
	vsnprintf(str, sizeof str, lpFmt, arglist);
	va_end(arglist);

#ifdef _WIN32
	len = strlen(str);
	co = GetStdHandle(STD_OUTPUT_HANDLE);

	if (co == INVALID_HANDLE_VALUE)
		return;

	if (col == DEFCOL)
	{
		if (!GetConsoleScreenBufferInfo(co, &ConsoleScreenBufferInfo))
			ConsoleScreenBufferInfo.wAttributes =
				FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

		SetConsoleTextAttribute(co, (WORD)col);
	}

	if (GetFileType(co) == FILE_TYPE_CHAR)
		WriteConsoleA(co, str, (DWORD)len, &bytesWritten, NULL);
	else
		WriteFile(co, str, (DWORD)len, &bytesWritten, NULL);

	if (col != DEFCOL)
		SetConsoleTextAttribute(co,
			ConsoleScreenBufferInfo.wAttributes);
#else
	printf("%s%s", col, str);
	fflush(stdout);
#endif
#else
	(void)col;
	(void)lpFmt;
#endif
}

/*
 * conPrintf()
 */
#ifdef _WIN32
void conPrintf(DWORDLONG col, const char *lpFmt, ...)
#else
void conPrintf(const char *col, const char *lpFmt, ...)
#endif
{
	va_start(arglist, lpFmt);
	vsnprintf(str, sizeof str, lpFmt, arglist);
	va_end(arglist);

#ifdef _WIN32
	len = strlen(str);
	co = GetStdHandle(STD_OUTPUT_HANDLE);

	if (col == DEFCOL)
	{
		if (!GetConsoleScreenBufferInfo(co, &ConsoleScreenBufferInfo))
			ConsoleScreenBufferInfo.wAttributes =
				FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;

		SetConsoleTextAttribute(co, (WORD)col);
	}

	if (co == INVALID_HANDLE_VALUE)
		return;

	if (GetFileType(co) == FILE_TYPE_CHAR)
		WriteConsoleA(co, str, (DWORD)len, &bytesWritten, NULL);
	else
		WriteFile(co, str, (DWORD)len, &bytesWritten, NULL);

	if (col != DEFCOL)
		SetConsoleTextAttribute(co,
			ConsoleScreenBufferInfo.wAttributes);
#else
	printf("%s%s", col, str);
	fflush(stdout);
#endif
}

/*
 * logPrintf():
 */
void logPrintf(FILE *f, const char *lpFmt, ...)
{
	char *ct;
	time_t t;

	va_start(arglist, lpFmt);
	vsnprintf(str, sizeof str, lpFmt, arglist);
	va_end(arglist);

	t = time(NULL);
	ct = ctime(&t);
	ct[strlen(ct)-1] = '\0';
	fprintf(f, "%s: %s", ct, str);
	fflush(f);
#if defined (__DEBUG__)
#ifdef _WIN32
	len = strlen(str);
	printf("%s", str);
#else
	printf("%s%s", DEFCOL, str);
#endif
	fflush(stdout);
#endif
}

/*
 * openFile():
 */
FILE *openFile(const char *filename)
{
	return fopen(filename, "a+t");
}

void strrand(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}
