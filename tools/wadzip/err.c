#if defined (__WIN32) || defined (WIN32)
#define NEED_ERR
#define LACK_PROGNAME
#define NEED_ENDIAN_STUBS
#define LACK_SYSTYPES
#endif

#ifdef NEED_ERR

/*
 * Implementation of the err/errx/verr/verrx/warn/warnx/vwarn/vwarnx
 * functions from BSD.
 *
 * This file is public-domain; anyone may deal in it without restriction.
 *
 * Written by Graue <graue@oceanbase.org> on January 16, 2006.
 */

/*
err/warn family of functions cheat sheet:

Print:
    last component of program name
    [ if fmt is non-NULL
        ": "
        the formatted error message
    ]
    [ if function name does not end in x
        ": "
        strerror(errno)
    ]
    newline

Then if function name has "err" in it, quit with exit code `eval'.

BSD's -x versions actually print ": " at the end if passed NULL, so I
duplicate that behavior. Passing these functions NULL is kind of useless
though.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef LACK_PROGNAME
char *__progname = "wadzip";
#else
extern char *__progname;
#endif

#define progname __progname

void vwarn(const char *fmt, va_list args)
{
	fputs(progname, stderr);
	if (fmt != NULL)
	{
		fputs(": ", stderr);
		vfprintf(stderr, fmt, args);
	}
	fputs(": ", stderr);
	fputs(strerror(errno), stderr);
	putc('\n', stderr);
}

void vwarnx(const char *fmt, va_list args)
{
	fputs(progname, stderr);
	fputs(": ", stderr);
	if (fmt != NULL)
		vfprintf(stderr, fmt, args);
	putc('\n', stderr);
}

void verr(int eval, const char *fmt, va_list args)
{
	vwarn(fmt, args);
	exit(eval);
}

void verrx(int eval, const char *fmt, va_list args)
{
	vwarnx(fmt, args);
	exit(eval);
}

void warn(const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	vwarn(fmt, argptr);
	va_end(argptr);
}

void warnx(const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	vwarnx(fmt, argptr);
	va_end(argptr);
}

void err(int eval, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	verr(eval, fmt, argptr);
	/* NOTREACHED, so don't worry about va_end() */
}

void errx(int eval, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	verrx(eval, fmt, argptr);
	/* NOTREACHED, so don't worry about va_end() */
}

#endif
