/*
    SDL_main.c, placed in the public domain by Sam Lantinga  4/13/98

    The WinMain function -- calls your program's main() function
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <malloc.h>			/* For _alloca() */

#include <tchar.h>
# define DIR_SEPERATOR TEXT("/")
# include <direct.h>

/* Include the SDL main definition header */
#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif
#include "SDL.h"
#include "SDL_main.h"
#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif
#include "../../win32/win_dbg.h"
#define USE_MESSAGEBOX

#ifdef main
# ifndef _WIN32_WCE_EMULATION
#  undef main
# endif /* _WIN32_WCE_EMULATION */
#endif /* main */

/* The standard output files */
#define STDOUT_FILE	TEXT("stdout.txt")
#define STDERR_FILE	TEXT("stderr.txt")

#ifndef NO_STDIO_REDIRECT
  static TCHAR stdoutPath[MAX_PATH];
  static TCHAR stderrPath[MAX_PATH];
#endif

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	int argc;

	argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		/* Skip leading whitespace */
		while ( isspace(*bufp) ) {
			++bufp;
		}
		/* Skip over argument */
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && (*bufp != '"') ) {
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}

/* Show an error message */
static void ShowError(const char *title, const char *message)
{
/* If USE_MESSAGEBOX is defined, you need to link with user32.lib */
#ifdef USE_MESSAGEBOX
	MessageBoxA(NULL,
		message,
		title,
		MB_ICONEXCLAMATION|MB_OK);
#else
	fprintf(stderr, "%s: %s\n", title, message);
#endif
}

/* Pop up an out of memory message, returns to Windows */
static BOOL OutOfMemory(void)
{
	ShowError("Fatal Error", "Out of memory - aborting");
	return FALSE;
}

/* Remove the output files if there was no output written */
static void __cdecl cleanup_output(void)
{
#ifndef NO_STDIO_REDIRECT
	FILE *file;
	int empty;
#endif

	/* Flush the output in case anything is queued */
	fclose(stdout);
	fclose(stderr);

#ifndef NO_STDIO_REDIRECT
	/* See if the files have any output in them */
	if ( stdoutPath[0] ) {
		file = _tfopen(stdoutPath, TEXT("rb"));
		if ( file ) {
			empty = (fgetc(file) == EOF) ? 1 : 0;
			fclose(file);
			if ( empty ) {
				_tremove(stdoutPath);
			}
		}
	}
	if ( stderrPath[0] ) {
		file = _tfopen(stderrPath, TEXT("rb"));
		if ( file ) {
			empty = (fgetc(file) == EOF) ? 1 : 0;
			fclose(file);
			if ( empty ) {
				_tremove(stderrPath);
			}
		}
	}
#endif
}

#if defined(_MSC_VER)
/* The VC++ compiler needs main defined */
#define console_main main
#endif

/* This is where execution begins [console apps] */
int console_main(int argc, char *argv[])
{
	size_t n;
	int st;
	char *bufp, *appname;

	/* Get the class name from argv[0] */
	appname = argv[0];
	if ( (bufp=strrchr(argv[0], '\\')) != NULL ) {
		appname = bufp+1;
	} else
	if ( (bufp=strrchr(argv[0], '/')) != NULL ) {
		appname = bufp+1;
	}

	if ( (bufp=strrchr(appname, '.')) == NULL )
		n = strlen(appname);
	else
		n = (bufp-appname);

	bufp = (char *)alloca(n+1);
	if ( bufp == NULL ) {
		return OutOfMemory();
	}
	strncpy(bufp, appname, n);
	bufp[n] = '\0';
	appname = bufp;

	/* Load SDL dynamic link library */
	if ( SDL_Init(SDL_INIT_NOPARACHUTE) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		return(FALSE);
	}
	atexit(cleanup_output);
	atexit(SDL_Quit);

#ifndef DISABLE_VIDEO
#if 0
	/* Create and register our class *
	   DJM: If we do this here, the user nevers gets a chance to
	   putenv(SDL_WINDOWID).  This is already called later by
	   the (DIB|DX5)_CreateWindow function, so it should be
	   safe to comment it out here.
	if ( SDL_RegisterApp(appname, CS_BYTEALIGNCLIENT,
	                     GetModuleHandle(NULL)) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		exit(1);
	}*/
#else
	/* Sam:
	   We still need to pass in the application handle so that
	   DirectInput will initialize properly when SDL_RegisterApp()
	   is called later in the video initialization.
	 */
	SDL_SetModuleHandle(GetModuleHandle(NULL));
#endif /* 0 */
#endif /* !DISABLE_VIDEO */

	/* Run the application main() code */
	st = SDL_main(argc, argv);

	/* Exit cleanly, calling atexit() functions */
	//exit(0);
	cleanup_output();
	SDL_Quit();

	/* Hush little compiler, don't you cry... */
	return st;
}

/* This is where execution begins [windowed apps] */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
	HINSTANCE handle;
	int Result = -1;
	char **argv;
	int argc;
	LPSTR cmdline;
	LPSTR bufp;
#ifndef NO_STDIO_REDIRECT
	FILE *newfp;
#endif

	/* Start up DDHELP.EXE before opening any files, so DDHELP doesn't
	   keep them open.  This is a hack.. hopefully it will be fixed
	   someday.  DDHELP.EXE starts up the first time DDRAW.DLL is loaded.
	 */
	hPrev = hInst = NULL;
	sw = 0;
	handle = LoadLibrary(TEXT("DDRAW.DLL"));
	if ( handle != NULL ) {
		FreeLibrary(handle);
	}

#ifndef NO_STDIO_REDIRECT
	_tgetcwd( stdoutPath, sizeof( stdoutPath ) );
	_tcscat( stdoutPath, DIR_SEPERATOR STDOUT_FILE );

	/* Redirect standard input and standard output */
	newfp = _tfreopen(stdoutPath, TEXT("w"), stdout);

	if ( newfp == NULL ) {	/* This happens on NT */
#if !defined(stdout)
		stdout = _tfopen(stdoutPath, TEXT("w"));
#else
		newfp = _tfopen(stdoutPath, TEXT("w"));
		if ( newfp ) {
			*stdout = *newfp;
		}
#endif
	}

	_tgetcwd( stderrPath, sizeof( stderrPath ) );
	_tcscat( stderrPath, DIR_SEPERATOR STDERR_FILE );

	newfp = _tfreopen(stderrPath, TEXT("w"), stderr);
	if ( newfp == NULL ) {	/* This happens on NT */
#if !defined(stderr)
		stderr = _tfopen(stderrPath, TEXT("w"));
#else
		newfp = _tfopen(stderrPath, TEXT("w"));
		if ( newfp ) {
			*stderr = *newfp;
		}
#endif
	}

	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);	/* Line buffered */
	setbuf(stderr, NULL);			/* No buffering */
#endif /* !NO_STDIO_REDIRECT */

	szCmdLine = NULL;
	/* Grab the command line (use alloca() on Windows) */
	bufp = GetCommandLineA();
	cmdline = (LPSTR)alloca(strlen(bufp)+1);
	if ( cmdline == NULL ) {
		return OutOfMemory();
	}
	strcpy(cmdline, bufp);

	/* Parse it into argv and argc */
	argc = ParseCommandLine(cmdline, NULL);
	argv = (char **)alloca((argc+1)*(sizeof *argv));
	if ( argv == NULL ) {
		return OutOfMemory();
	}
	ParseCommandLine(cmdline, argv);

#ifdef BUGTRAP
	/* Try BugTrap. */
	if(InitBugTrap())
		Result = console_main(argc, argv);
	else
	{
#endif

		/* Run the main program (after a little SDL initialization) */
		 __try
		{
			Result = console_main(argc, argv);
		}
		__except ( RecordExceptionInfo(GetExceptionInformation()))
		{
			SetUnhandledExceptionFilter(EXCEPTION_CONTINUE_SEARCH); //Do nothing here.
		}

#ifdef BUGTRAP
	}	/* BT failure clause. */

	/* This is safe even if BT didn't start. */
	ShutdownBugTrap();
#endif

	return Result;
}
