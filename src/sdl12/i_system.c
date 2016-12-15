// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Changes by Graue <graue@oceanbase.org> are in the public domain.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 system stuff for SDL

#ifndef _WIN32_WCE
#include <signal.h>
#endif

#ifdef _XBOX
#include "SRB2XBOX/xboxhelp.h"
#endif

#if defined (_WIN32) && !defined (_XBOX)
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include "../doomtype.h"
#ifndef _WIN32_WCE
typedef BOOL (WINAPI *p_GetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *p_IsProcessorFeaturePresent) (DWORD);
typedef DWORD (WINAPI *p_timeGetTime) (void);
typedef UINT (WINAPI *p_timeEndPeriod) (UINT);
typedef HANDLE (WINAPI *p_OpenFileMappingA) (DWORD, BOOL, LPCSTR);
typedef LPVOID (WINAPI *p_MapViewOfFile) (HANDLE, DWORD, DWORD, DWORD, SIZE_T);
typedef HANDLE (WINAPI *p_GetCurrentProcess) (VOID);
typedef BOOL (WINAPI *p_GetProcessAffinityMask) (HANDLE, PDWORD_PTR, PDWORD_PTR);
typedef BOOL (WINAPI *p_SetProcessAffinityMask) (HANDLE, DWORD_PTR);
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#elif defined (_MSC_VER)
#include <direct.h>
#endif
#if defined (__unix__) || defined (UNIXCOMMON)
#include <fcntl.h>
#endif

#ifdef _arch_dreamcast
#include <arch/gdb.h>
#include <arch/timer.h>
#include <conio/conio.h>
#include <dc/pvr.h>
void __set_fpscr(long); // in libgcc / kernel's startup.s?
#else
#include <stdio.h>
#if defined (_WIN32) && !defined (_WIN32_WCE) && !defined (_XBOX)
#include <conio.h>
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL

#include "SDL.h"

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#if SDL_VERSION_ATLEAST(1,2,7) && !defined (DC)
#include "SDL_cpuinfo.h" // 1.2.7 or greater
#define HAVE_SDLCPUINFO
#endif

#ifdef _PSP
//#include <pspiofilemgr.h>
#elif !defined(_PS3)
#if defined (__unix__) || defined(__APPLE__) || (defined (UNIXCOMMON) && !defined (_arch_dreamcast) && !defined (__HAIKU__) && !defined (_WII))
#if defined (__linux__)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
/*For meminfo*/
#include <sys/types.h>
#ifdef FREEBSD
#include <kvm.h>
#endif
#include <nlist.h>
#include <sys/vmmeter.h>
#endif
#endif
#endif

#ifndef _PS3
#if defined (__linux__) || (defined (UNIXCOMMON) && !defined (_arch_dreamcast) && !defined (_PSP) && !defined (__HAIKU__) && !defined (_WII))
#ifndef NOTERMIOS
#include <termios.h>
#include <sys/ioctl.h> // ioctl
#define HAVE_TERMIOS
#endif
#endif
#endif

#ifndef NOMUMBLE
#if defined (__linux__) && !defined(_PS3) // need -lrt
#include <sys/mman.h>
#ifdef MAP_FAILED
#define HAVE_SHM
#endif
#include <wchar.h>
#endif

#if defined (_WIN32) && !defined (_WIN32_WCE) && !defined (_XBOX)
#define HAVE_MUMBLE
#define WINMUMBLE
#elif defined (HAVE_SHM)
#define HAVE_MUMBLE
#endif
#endif // NOMUMBLE

#ifdef _WIN32_WCE
#include "SRB2CE/cehelp.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

// Locations for searching the srb2.srb
#ifdef _arch_dreamcast
#define DEFAULTWADLOCATION1 "/cd"
#define DEFAULTWADLOCATION2 "/pc"
#define DEFAULTWADLOCATION3 "/pc/home/alam/srb2code/data"
#define DEFAULTSEARCHPATH1 "/cd"
#define DEFAULTSEARCHPATH2 "/pc"
//#define DEFAULTSEARCHPATH3 "/pc/home/alam/srb2code/data"
#elif defined (GP2X)
#define DEFAULTWADLOCATION1 "/mnt/sd"
#define DEFAULTWADLOCATION2 "/mnt/sd/SRB2"
#define DEFAULTWADLOCATION3 "/tmp/mnt/sd"
#define DEFAULTWADLOCATION4 "/tmp/mnt/sd/SRB2"
#define DEFAULTSEARCHPATH1 "/mnt/sd"
#define DEFAULTSEARCHPATH2 "/tmp/mnt/sd"
#elif defined (_WII)
#define NOCWD
#define NOHOME
#define NEED_SDL_GETENV
#define DEFAULTWADLOCATION1 "sd:/srb2wii"
#define DEFAULTWADLOCATION2 "usb:/srb2wii"
#define DEFAULTSEARCHPATH1 "sd:/srb2wii"
#define DEFAULTSEARCHPATH2 "usb:/srb2wii"
// PS3: TODO: this will need modification most likely
#elif defined (_PS3)
#define NOCWD
#define NOHOME
#define DEFAULTWADLOCATION1 "/dev_hdd0/game/SRB2-PS3_/USRDIR/etc"
#define DEFAULTWADLOCATION2 "/dev_usb/SRB2PS3"
#define DEFAULTSEARCHPATH1 "/dev_hdd0/game/SRB2-PS3_/USRDIR/etc"
#define DEFAULTSEARCHPATH2 "/dev_usb/SRB2PS3"
#elif defined (_PSP)
#define NOCWD
#define NOHOME
#define DEFAULTWADLOCATION1 "host0:/bin/Resources"
#define DEFAULTWADLOCATION2 "ms0:/PSP/GAME/SRB2PSP"
#define DEFAULTSEARCHPATH1 "host0:/"
#define DEFAULTSEARCHPATH2 "ms0:/PSP/GAME/SRB2PSP"
#elif defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#define DEFAULTWADLOCATION1 "/usr/local/share/games/SRB2"
#define DEFAULTWADLOCATION2 "/usr/local/games/SRB2"
#define DEFAULTWADLOCATION3 "/usr/share/games/SRB2"
#define DEFAULTWADLOCATION4 "/usr/games/SRB2"
#define DEFAULTSEARCHPATH1 "/usr/local/games"
#define DEFAULTSEARCHPATH2 "/usr/games"
#define DEFAULTSEARCHPATH3 "/usr/local"
#elif defined (_XBOX)
#define NOCWD
#ifdef __GNUC__
#include <openxdk/debug.h>
#endif
#define DEFAULTWADLOCATION1 "c:\\srb2"
#define DEFAULTWADLOCATION2 "d:\\srb2"
#define DEFAULTWADLOCATION3 "e:\\srb2"
#define DEFAULTWADLOCATION4 "f:\\srb2"
#define DEFAULTWADLOCATION5 "g:\\srb2"
#define DEFAULTWADLOCATION6 "h:\\srb2"
#define DEFAULTWADLOCATION7 "i:\\srb2"
#elif defined (_WIN32_WCE)
#define NOCWD
#define NOHOME
#define DEFAULTWADLOCATION1 "\\Storage Card\\SRB2DEMO"
#define DEFAULTSEARCHPATH1 "\\Storage Card"
#elif defined (_WIN32)
#define DEFAULTWADLOCATION1 "c:\\games\\srb2"
#define DEFAULTWADLOCATION2 "\\games\\srb2"
#define DEFAULTSEARCHPATH1 "c:\\games"
#define DEFAULTSEARCHPATH2 "\\games"
#endif

/**	\brief WAD file to look for
*/
#define WADKEYWORD1 "srb2.srb"
#define WADKEYWORD2 "srb2.wad"
/**	\brief holds wad path
*/
static char returnWadPath[256];

//Alam_GBC: SDL

#include "../doomdef.h"
#include "../m_misc.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../screen.h" //vid.WndParent
#include "../d_net.h"
#include "../g_game.h"
#include "../filesrch.h"
#include "endtxt.h"
#include "sdlmain.h"

#include "../i_joy.h"

#include "../m_argv.h"

#ifdef MAC_ALERT
#include "macosx/mac_alert.h"
#endif

#include "../d_main.h"

#if !defined(NOMUMBLE) && defined(HAVE_MUMBLE)
// Mumble context string
#include "../d_clisrv.h"
#include "../byteptr.h"
#endif

/**	\brief	The JoyReset function

	\param	JoySet	Joystick info to reset

	\return	void
*/
static void JoyReset(SDLJoyInfo_t *JoySet)
{
	if (JoySet->dev)
	{
#ifdef GP2X //GP2X's SDL does an illegal free on the 1st joystick...
		if (SDL_JoystickIndex(JoySet->dev) != 0)
#endif
		SDL_JoystickClose(JoySet->dev);
	}
	JoySet->dev = NULL;
	JoySet->oldjoy = -1;
	JoySet->axises = JoySet->buttons = JoySet->hats = JoySet->balls = 0;
	//JoySet->scale
}

/**	\brief First joystick up and running
*/
static INT32 joystick_started  = 0;

/**	\brief SDL info about joystick 1
*/
SDLJoyInfo_t JoyInfo;


/**	\brief Second joystick up and running
*/
static INT32 joystick2_started = 0;

/**	\brief SDL inof about joystick 2
*/
SDLJoyInfo_t JoyInfo2;

#ifdef HAVE_TERMIOS
static INT32 fdmouse2 = -1;
static INT32 mouse2_started = 0;
#endif

SDL_bool consolevent = SDL_FALSE;
SDL_bool framebuffer = SDL_FALSE;

UINT8 keyboard_started = false;

#if 0
static void signal_handler(INT32 num)
{
	//static char msg[] = "oh no! back to reality!\r\n";
	char *      sigmsg;
	char        sigdef[32];

	switch (num)
	{
	case SIGINT:
		sigmsg = "interrupt";
		break;
	case SIGILL:
		sigmsg = "illegal instruction - invalid function image";
		break;
	case SIGFPE:
		sigmsg = "floating point exception";
		break;
	case SIGSEGV:
		sigmsg = "segment violation";
		break;
	case SIGTERM:
		sigmsg = "Software termination signal from kill";
		break;
#if !(defined (__unix_) || defined (UNIXCOMMON))
	case SIGBREAK:
		sigmsg = "Ctrl-Break sequence";
		break;
#endif
	case SIGABRT:
		sigmsg = "abnormal termination triggered by abort call";
		break;
	default:
		sprintf(sigdef,"signal number %d", num);
		sigmsg = sigdef;
	}

	I_OutputMsg("signal_handler() error: %s\n", sigmsg);
	signal(num, SIG_DFL);               //default signal action
	raise(num);
	I_Quit();
}
#endif

#if defined (NDEBUG) && !defined (DC) && !defined (_WIN32_WCE)
FUNCNORETURN static ATTRNORETURN void quit_handler(int num)
{
	signal(num, SIG_DFL); //default signal action
	raise(num);
	I_Quit();
}
#endif

#ifdef HAVE_TERMIOS
// TERMIOS console code from Quake3: thank you!
SDL_bool stdin_active = SDL_TRUE;

typedef struct
{
	size_t cursor;
	char buffer[256];
} feild_t;

feild_t tty_con;

// when printing general stuff to stdout stderr (Sys_Printf)
//   we need to disable the tty console stuff
// this increments so we can recursively disable
static INT32 ttycon_hide = 0;
// some key codes that the terminal may be using
// TTimo NOTE: I'm not sure how relevant this is
static INT32 tty_erase;
static INT32 tty_eof;

static struct termios tty_tc;

// =============================================================
// tty console routines
// NOTE: if the user is editing a line when something gets printed to the early console then it won't look good
//   so we provide tty_Clear and tty_Show to be called before and after a stdout or stderr output
// =============================================================

// flush stdin, I suspect some terminals are sending a LOT of garbage
// FIXME TTimo relevant?
#if 0
static inline void tty_FlushIn(void)
{
	char key;
	while (read(STDIN_FILENO, &key, 1)!=-1);
}
#endif

// do a backspace
// TTimo NOTE: it seems on some terminals just sending '\b' is not enough
//   so for now, in any case we send "\b \b" .. yeah well ..
//   (there may be a way to find out if '\b' alone would work though)
static void tty_Back(void)
{
	char key;
	ssize_t d;
	key = '\b';
	d = write(STDOUT_FILENO, &key, 1);
	key = ' ';
	d = write(STDOUT_FILENO, &key, 1);
	key = '\b';
	d = write(STDOUT_FILENO, &key, 1);
	(void)d;
}

static void tty_Clear(void)
{
	size_t i;
	if (tty_con.cursor>0)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			tty_Back();
		}
	}

}

// clear the display of the line currently edited
// bring cursor back to beginning of line
static inline void tty_Hide(void)
{
	//I_Assert(consolevent);
	if (ttycon_hide)
	{
		ttycon_hide++;
		return;
	}
	tty_Clear();
	ttycon_hide++;
}

// show the current line
// FIXME TTimo need to position the cursor if needed??
static inline void tty_Show(void)
{
	size_t i;
	ssize_t d;
	//I_Assert(consolevent);
	I_Assert(ttycon_hide>0);
	ttycon_hide--;
	if (ttycon_hide == 0 && tty_con.cursor)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			d = write(STDOUT_FILENO, tty_con.buffer+i, 1);
		}
	}
	(void)d;
}

// never exit without calling this, or your terminal will be left in a pretty bad state
static void I_ShutdownConsole(void)
{
	if (consolevent)
	{
		I_OutputMsg("Shutdown tty console\n");
		consolevent = SDL_FALSE;
		tcsetattr (STDIN_FILENO, TCSADRAIN, &tty_tc);
	}
}

static void I_StartupConsole(void)
{
	struct termios tc;

	// TTimo
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=390 (404)
	// then SIGTTIN or SIGTOU is emitted, if not catched, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

#if !defined(GP2X) //read is bad on GP2X
	consolevent = !M_CheckParm("-noconsole");
#endif
	framebuffer = M_CheckParm("-framebuffer");

	if (framebuffer)
		consolevent = SDL_FALSE;

	if (!consolevent) return;

	if (isatty(STDIN_FILENO)!=1)
	{
		I_OutputMsg("stdin is not a tty, tty console mode failed\n");
		consolevent = SDL_FALSE;
		return;
	}
	memset(&tty_con, 0x00, sizeof(tty_con));
	tcgetattr (0, &tty_tc);
	tty_erase = tty_tc.c_cc[VERASE];
	tty_eof = tty_tc.c_cc[VEOF];
	tc = tty_tc;
	/*
	 ECHO: don't echo input characters
	 ICANON: enable canonical mode.  This  enables  the  special
	  characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	  STATUS, and WERASE, and buffers by lines.
	 ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	  DSUSP are received, generate the corresponding signal
	*/
	tc.c_lflag &= ~(ECHO | ICANON);
	/*
	 ISTRIP strip off bit 8
	 INPCK enable input parity checking
	 */
	tc.c_iflag &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN] = 0; //1?
	tc.c_cc[VTIME] = 0;
	tcsetattr (0, TCSADRAIN, &tc);
}

void I_GetConsoleEvents(void)
{
	// we use this when sending back commands
	event_t ev = {0,0,0,0};
	char key = 0;
	ssize_t d;

	if (!consolevent)
		return;

	ev.type = ev_console;
	if (read(STDIN_FILENO, &key, 1) == -1 || !key)
		return;

	// we have something
	// backspace?
	// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
	if ((key == tty_erase) || (key == 127) || (key == 8))
	{
		if (tty_con.cursor > 0)
		{
			tty_con.cursor--;
			tty_con.buffer[tty_con.cursor] = '\0';
			tty_Back();
		}
		ev.data1 = KEY_BACKSPACE;
	}
	else if (key < ' ') // check if this is a control char
	{
		if (key == '\n')
		{
			tty_Clear();
			tty_con.cursor = 0;
			ev.data1 = KEY_ENTER;
		}
		else return;
	}
	else
	{
		// push regular character
		ev.data1 = tty_con.buffer[tty_con.cursor] = key;
		tty_con.cursor++;
		// print the current line (this is differential)
		d = write(STDOUT_FILENO, &key, 1);
	}
	if (ev.data1) D_PostEvent(&ev);
	//tty_FlushIn();
	(void)d;
}

#elif defined (_WIN32) && !(defined (_XBOX) || defined (_WIN32_WCE))
static BOOL I_ReadyConsole(HANDLE ci)
{
	DWORD gotinput;
	if (ci == INVALID_HANDLE_VALUE) return FALSE;
	if (WaitForSingleObject(ci,0) != WAIT_OBJECT_0) return FALSE;
	if (GetFileType(ci) != FILE_TYPE_CHAR) return FALSE;
	if (!GetConsoleMode(ci, &gotinput)) return FALSE;
	return (GetNumberOfConsoleInputEvents(ci, &gotinput) && gotinput);
}

static boolean entering_con_command = false;

void I_GetConsoleEvents(void)
{
	event_t ev = {0,0,0,0};
	HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	INPUT_RECORD input;
	DWORD t;

	while (I_ReadyConsole(ci) && ReadConsoleInput(ci, &input, 1, &t) && t)
	{
		memset(&ev,0x00,sizeof (ev));
		switch (input.EventType)
		{
			case KEY_EVENT:
				if (input.Event.KeyEvent.bKeyDown)
				{
					ev.type = ev_console;
					entering_con_command = true;
					switch (input.Event.KeyEvent.wVirtualKeyCode)
					{
						case VK_ESCAPE:
						case VK_TAB:
							ev.data1 = KEY_NULL;
							break;
						case VK_SHIFT:
							ev.data1 = KEY_LSHIFT;
							break;
						case VK_RETURN:
							entering_con_command = false;
							// Fall through.
						default:
							ev.data1 = MapVirtualKey(input.Event.KeyEvent.wVirtualKeyCode,2); // convert in to char
					}
					if (co != INVALID_HANDLE_VALUE && GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &t))
					{
						if (ev.data1 && ev.data1 != KEY_LSHIFT && ev.data1 != KEY_RSHIFT)
						{
#ifdef _UNICODE
							WriteConsole(co, &input.Event.KeyEvent.uChar.UnicodeChar, 1, &t, NULL);
#else
							WriteConsole(co, &input.Event.KeyEvent.uChar.AsciiChar, 1 , &t, NULL);
#endif
						}
						if (input.Event.KeyEvent.wVirtualKeyCode == VK_BACK
							&& GetConsoleScreenBufferInfo(co,&CSBI))
						{
							WriteConsoleOutputCharacterA(co, " ",1, CSBI.dwCursorPosition, &t);
						}
					}
				}
				else
				{
					ev.type = ev_keyup;
					switch (input.Event.KeyEvent.wVirtualKeyCode)
					{
						case VK_SHIFT:
							ev.data1 = KEY_LSHIFT;
							break;
						default:
							break;
					}
				}
				if (ev.data1) D_PostEvent(&ev);
				break;
			case MOUSE_EVENT:
			case WINDOW_BUFFER_SIZE_EVENT:
			case MENU_EVENT:
			case FOCUS_EVENT:
				break;
		}
	}
}

static void I_StartupConsole(void)
{
	HANDLE ci, co;
	const INT32 ded = M_CheckParm("-dedicated");
#ifdef SDLMAIN
	BOOL gotConsole = FALSE;
	if (M_CheckParm("-console") || ded)
		gotConsole = AllocConsole();
#else
	BOOL gotConsole = TRUE;
	if (M_CheckParm("-detachconsole"))
	{
		FreeConsole();
		gotConsole = AllocConsole();
	}
#ifdef _DEBUG
	else if (M_CheckParm("-noconsole") && !ded)
#else
	else if (!M_CheckParm("-console") && !ded)
#endif
	{
		FreeConsole();
		gotConsole = FALSE;
	}
#endif

	if (gotConsole)
	{
		SetConsoleTitleA("SRB2 Console");
		consolevent = SDL_TRUE;
	}

	//Let get the real console HANDLE, because Mingw's Bash is bad!
	ci = CreateFile(TEXT("CONIN$") ,               GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	co = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ci != INVALID_HANDLE_VALUE)
	{
		const DWORD CM = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT;
		SetStdHandle(STD_INPUT_HANDLE, ci);
		if (GetFileType(ci) == FILE_TYPE_CHAR)
			SetConsoleMode(ci, CM); //default mode but no ENABLE_MOUSE_INPUT
	}
	if (co != INVALID_HANDLE_VALUE)
	{
		SetStdHandle(STD_OUTPUT_HANDLE, co);
		SetStdHandle(STD_ERROR_HANDLE, co);
	}
}
static inline void I_ShutdownConsole(void){}
#else
void I_GetConsoleEvents(void){}
static inline void I_StartupConsole(void)
{
#ifdef _arch_dreamcast
	char title[] = "SRB2 for Dreamcast!\n";
	__set_fpscr(0x00040000); /* ignore FPU underflow */
	//printf("\nHello world!\n\n");
	pvr_init_defaults();
	conio_init(CONIO_TTY_PVR, CONIO_INPUT_LINE);
	conio_set_theme(CONIO_THEME_MATRIX);
	conio_clear();
	conio_putstr(title);
	//printf("\nHello world!\n\n");
#endif
#ifdef _DEBUG
	consolevent = !M_CheckParm("-noconsole");
#else
	consolevent = M_CheckParm("-console");
#endif

	framebuffer = M_CheckParm("-framebuffer");

	if (framebuffer)
		consolevent = SDL_FALSE;
}
static inline void I_ShutdownConsole(void){}
#endif

//
// StartupKeyboard
//
void I_StartupKeyboard (void)
{
#if defined (NDEBUG) && !defined (DC)
#ifdef SIGILL
//	signal(SIGILL , signal_handler);
#endif
#ifdef SIGINT
	signal(SIGINT , quit_handler);
#endif
#ifdef SIGSEGV
//	signal(SIGSEGV , signal_handler);
#endif
#ifdef SIGBREAK
	signal(SIGBREAK , quit_handler);
#endif
#ifdef SIGABRT
//	signal(SIGABRT , signal_handler);
#endif
#ifdef SIGTERM
	signal(SIGTERM , quit_handler);
#endif
#endif
}

//
//I_OutputMsg
//
void I_OutputMsg(const char *fmt, ...)
{
	size_t len;
	XBOXSTATIC char txt[8192];
	va_list  argptr;

#ifdef _arch_dreamcast
	if (!keyboard_started) conio_printf(fmt);
#endif

	va_start(argptr,fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

#ifdef HAVE_TTF
	if (TTF_WasInit()) I_TTFDrawText(currentfont, solid, DEFAULTFONTFGR, DEFAULTFONTFGG, DEFAULTFONTFGB,  DEFAULTFONTFGA,
	DEFAULTFONTBGR, DEFAULTFONTBGG, DEFAULTFONTBGB, DEFAULTFONTBGA, txt);
#endif

#if defined (_WIN32) && !defined (_XBOX) && defined (_MSC_VER)
	OutputDebugStringA(txt);
#endif

	len = strlen(txt);

#ifdef LOGMESSAGES
	if (logstream)
	{
		size_t d = fwrite(txt, len, 1, logstream);
		fflush(logstream);
		(void)d;
	}
#endif

#if defined (_WIN32) && !defined (_XBOX) && !defined(_WIN32_WCE)
#ifdef DEBUGFILE
	if (debugfile != stderr)
#endif
	{
		HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD bytesWritten;

		if (co == INVALID_HANDLE_VALUE)
			return;

		if (GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &bytesWritten))
		{
			static COORD coordNextWrite = {0,0};
			LPVOID oldLines = NULL;
			INT oldLength;
			CONSOLE_SCREEN_BUFFER_INFO csbi;

			// Save the lines that we're going to obliterate.
			GetConsoleScreenBufferInfo(co, &csbi);
			oldLength = csbi.dwSize.X * (csbi.dwCursorPosition.Y - coordNextWrite.Y) + csbi.dwCursorPosition.X - coordNextWrite.X;

			if (oldLength > 0)
			{
				LPVOID blank = malloc(oldLength);
				if (!blank) return;
				memset(blank, ' ', oldLength); // Blank out.
				oldLines = malloc(oldLength*sizeof(TCHAR));
				if (!oldLines)
				{
					free(blank);
					return;
				}

				ReadConsoleOutputCharacter(co, oldLines, oldLength, coordNextWrite, &bytesWritten);

				// Move to where we what to print - which is where we would've been,
				// had console input not been in the way,
				SetConsoleCursorPosition(co, coordNextWrite);

				WriteConsoleA(co, blank, oldLength, &bytesWritten, NULL);
				free(blank);

				// And back to where we want to print again.
				SetConsoleCursorPosition(co, coordNextWrite);
			}

			// Actually write the string now!
			WriteConsoleA(co, txt, (DWORD)len, &bytesWritten, NULL);

			// Next time, output where we left off.
			GetConsoleScreenBufferInfo(co, &csbi);
			coordNextWrite = csbi.dwCursorPosition;

			// Restore what was overwritten.
			if (oldLines && entering_con_command)
				WriteConsole(co, oldLines, oldLength, &bytesWritten, NULL);
			if (oldLines) free(oldLines);
		}
		else // Redirected to a file.
			WriteFile(co, txt, (DWORD)len, &bytesWritten, NULL);
	}
#else
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Hide();
	}
#endif

	if (!framebuffer)
		fprintf(stderr, "%s", txt);
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Show();
	}
#endif

	// 2004-03-03 AJR Since not all messages end in newline, some were getting displayed late.
	if (!framebuffer)
		fflush(stderr);

#endif
}

//
// I_GetKey
//
INT32 I_GetKey (void)
{
	// Warning: I_GetKey empties the event queue till next keypress
	event_t *ev;
	INT32 rc = 0;

	// return the first keypress from the event queue
	for (; eventtail != eventhead; eventtail = (eventtail+1)&(MAXEVENTS-1))
	{
		ev = &events[eventtail];
		if (ev->type == ev_keydown || ev->type == ev_console)
		{
			rc = ev->data1;
			continue;
		}
	}

	return rc;
}

//
// I_JoyScale
//
void I_JoyScale(void)
{
#ifdef GP2X
	if (JoyInfo.dev && SDL_JoystickIndex(JoyInfo.dev) == 0)
		Joystick.bGamepadStyle = true;
	else
#endif
	Joystick.bGamepadStyle = cv_joyscale.value==0;
	JoyInfo.scale = Joystick.bGamepadStyle?1:cv_joyscale.value;
}

void I_JoyScale2(void)
{
#ifdef GP2X
	if (JoyInfo2.dev && SDL_JoystickIndex(JoyInfo2.dev) == 0)
		Joystick.bGamepadStyle = true;
	else
#endif
	Joystick2.bGamepadStyle = cv_joyscale2.value==0;
	JoyInfo2.scale = Joystick2.bGamepadStyle?1:cv_joyscale2.value;
}

/**	\brief Joystick 1 buttons states
*/
static UINT64 lastjoybuttons = 0;

/**	\brief Joystick 1 hats state
*/
static UINT64 lastjoyhats = 0;

/**	\brief	Shuts down joystick 1


	\return void


*/
static void I_ShutdownJoystick(void)
{
	INT32 i;
	event_t event;
	event.type=ev_keyup;
	event.data2 = 0;
	event.data3 = 0;

	lastjoybuttons = lastjoyhats = 0;

	// emulate the up of all joystick buttons
	for (i=0;i<JOYBUTTONS;i++)
	{
		event.data1=KEY_JOY1+i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for (i=0;i<JOYHATS*4;i++)
	{
		event.data1=KEY_HAT1+i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick;
	for (i=0;i<JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	joystick_started = 0;
	JoyReset(&JoyInfo);
	if (!joystick_started && !joystick2_started && SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		if (cv_usejoystick.value == 0)
		{
			I_OutputMsg("I_Joystick: SDL's Joystick system has been shutdown\n");
		}
	}
}

void I_GetJoystickEvents(void)
{
	static event_t event = {0,0,0,0};
	INT32 i = 0;
	UINT64 joyhats = 0;
#if 0
	UINT64 joybuttons = 0;
	Sint16 axisx, axisy;
#endif

	if (!joystick_started) return;

	if (!JoyInfo.dev) //I_ShutdownJoystick();
		return;

#if 0
	//faB: look for as much buttons as g_input code supports,
	//  we don't use the others
	for (i = JoyInfo.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo.dev,i))
			joybuttons |= 1;
	}

	if (joybuttons != lastjoybuttons)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoybuttons;
		lastjoybuttons = joybuttons;

		for (i = 0; i < JOYBUTTONS; i++, j <<= 1)
		{
			if (newbuttons & j) // button changed state?
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
#ifdef _PSP
				if (i == 12)
					event.data1 = KEY_ESCAPE;
				else
#endif
				event.data1 = KEY_JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}
#endif

	for (i = JoyInfo.hats - 1; i >= 0; i--)
	{
		Uint8 hat = SDL_JoystickGetHat(JoyInfo.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=(UINT64)0x1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=(UINT64)0x1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=(UINT64)0x1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=(UINT64)0x1<<(3 + 4*i);
	}

	if (joyhats != lastjoyhats)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoyhats;
		lastjoyhats = joyhats;

		for (i = 0; i < JOYHATS*4; i++, j <<= 1)
		{
			if (newhats & j) // hat changed state?
			{
				if (joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_HAT1 + i;
				D_PostEvent(&event);
			}
		}
	}

#if 0
	// send joystick axis positions
	event.type = ev_joystick;

	for (i = JOYAXISSET - 1; i >= 0; i--)
	{
		event.data1 = i;
		if (i*2 + 1 <= JoyInfo.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 0);
		else axisx = 0;
		if (i*2 + 2 <= JoyInfo.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 1);
		else axisy = 0;

#ifdef _arch_dreamcast // -128 to 127
		axisx = axisx*8;
		axisy = axisy*8;
#else // -32768 to 32767
		axisx = axisx/32;
		axisy = axisy/32;
#endif

		if (Joystick.bGamepadStyle)
		{
			// gamepad control type, on or off, live or die
			if (axisx < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (axisx > (JOYAXISRANGE/2))
				event.data2 = 1;
			else event.data2 = 0;
			if (axisy < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (axisy > (JOYAXISRANGE/2))
				event.data3 = 1;
			else event.data3 = 0;
		}
		else
		{

			axisx = JoyInfo.scale?((axisx/JoyInfo.scale)*JoyInfo.scale):axisx;
			axisy = JoyInfo.scale?((axisy/JoyInfo.scale)*JoyInfo.scale):axisy;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= axisx && axisx <= SDL_JDEADZONE) axisx = 0;
			if (-SDL_JDEADZONE <= axisy && axisy <= SDL_JDEADZONE) axisy = 0;
#endif

			// analog control style , just send the raw data
			event.data2 = axisx; // x axis
			event.data3 = axisy; // y axis
		}
		D_PostEvent(&event);
	}
#endif
}

/**	\brief	Open joystick handle

	\param	fname	name of joystick

	\return	axises


*/
static int joy_open(const char *fname)
{
	int joyindex = atoi(fname);
	int num_joy = 0;
	int i;

	if (joystick_started == 0 && joystick2_started == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf(M_GetText("Couldn't initialize joystick: %s\n"), SDL_GetError());
			return -1;
		}
		else
		{
			num_joy = SDL_NumJoysticks();
		}

		if (num_joy < joyindex)
		{
			CONS_Printf(M_GetText("Cannot use joystick #%d/(%s), it doesn't exist\n"),joyindex,fname);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#%d/(%s)\n", i+1, SDL_JoystickName(i));
			I_ShutdownJoystick();
			return -1;
		}
	}
	else
	{
		JoyReset(&JoyInfo);
		//I_ShutdownJoystick();
		//joy_open(fname);
	}

	num_joy = SDL_NumJoysticks();

	if (joyindex <= 0 || num_joy == 0 || JoyInfo.oldjoy == joyindex)
	{
//		I_OutputMsg("Unable to use that joystick #(%s), non-number\n",fname);
		if (num_joy != 0)
		{
			CONS_Printf(M_GetText("Found %d joysticks on this system\n"), num_joy);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#%d/(%s)\n", i+1, SDL_JoystickName(i));
		}
		else
			CONS_Printf("%s", M_GetText("Found no joysticks on this system\n"));
		if (joyindex <= 0 || num_joy == 0) return 0;
	}

	JoyInfo.dev = SDL_JoystickOpen(joyindex-1);
	CONS_Printf(M_GetText("Joystick: %s\n"), SDL_JoystickName(joyindex-1));

	if (JoyInfo.dev == NULL)
	{
		CONS_Printf(M_GetText("Couldn't open joystick: %s\n"), SDL_GetError());
		I_ShutdownJoystick();
		return -1;
	}
	else
	{
		JoyInfo.axises = SDL_JoystickNumAxes(JoyInfo.dev);
		if (JoyInfo.axises > JOYAXISSET*2)
			JoyInfo.axises = JOYAXISSET*2;
/*		if (joyaxes<2)
		{
			I_OutputMsg("Not enought axes?\n");
			I_ShutdownJoystick();
			return 0;
		}*/

		JoyInfo.buttons = SDL_JoystickNumButtons(JoyInfo.dev);
		if (JoyInfo.buttons > JOYBUTTONS)
			JoyInfo.buttons = JOYBUTTONS;

#ifdef DC
		JoyInfo.hats = 0;
#else
		JoyInfo.hats = SDL_JoystickNumHats(JoyInfo.dev);
		if (JoyInfo.hats > JOYHATS)
			JoyInfo.hats = JOYHATS;

		JoyInfo.balls = SDL_JoystickNumBalls(JoyInfo.dev);
#endif

		//Joystick.bGamepadStyle = !stricmp(SDL_JoystickName(SDL_JoystickIndex(JoyInfo.dev)), "pad");

		return JoyInfo.axises;
	}
}

//Joystick2

/**	\brief Joystick 2 buttons states
*/
static UINT64 lastjoy2buttons = 0;

/**	\brief Joystick 2 hats state
*/
static UINT64 lastjoy2hats = 0;

/**	\brief	Shuts down joystick 2


	\return	void
*/
static void I_ShutdownJoystick2(void)
{
	INT32 i;
	event_t event;
	event.type = ev_keyup;
	event.data2 = 0;
	event.data3 = 0;

	lastjoy2buttons = lastjoy2hats = 0;

	// emulate the up of all joystick buttons
	for (i = 0; i < JOYBUTTONS; i++)
	{
		event.data1 = KEY_2JOY1 + i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for (i = 0; i < JOYHATS*4; i++)
	{
		event.data1 = KEY_2HAT1 + i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick2;
	for (i = 0; i < JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	JoyReset(&JoyInfo2);
	if (!joystick_started && !joystick2_started && SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		if (cv_usejoystick2.value == 0)
		{
			DEBFILE("I_Joystick2: SDL's Joystick system has been shutdown\n");
		}
	}
}

void I_GetJoystick2Events(void)
{
	static event_t event = {0,0,0,0};
	INT32 i = 0;
	UINT64 joyhats = 0;
#if 0
	INT64 joybuttons = 0;
	INT32 axisx, axisy;
#endif

	if (!joystick2_started)
		return;

	if (!JoyInfo2.dev) //I_ShutdownJoystick2();
		return;


#if 0
	//faB: look for as much buttons as g_input code supports,
	//  we don't use the others
	for (i = JoyInfo2.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo2.dev,i))
			joybuttons |= 1;
	}

	if (joybuttons != lastjoy2buttons)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoy2buttons;
		lastjoy2buttons = joybuttons;

		for (i = 0; i < JOYBUTTONS; i++, j <<= 1)
		{
			if (newbuttons & j) // button changed state?
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}
#endif

	for (i = JoyInfo2.hats - 1; i >= 0; i--)
	{
		Uint8 hat = SDL_JoystickGetHat(JoyInfo2.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=(UINT64)0x1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=(UINT64)0x1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=(UINT64)0x1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=(UINT64)0x1<<(3 + 4*i);
	}

	if (joyhats != lastjoy2hats)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoy2hats;
		lastjoy2hats = joyhats;

		for (i = 0; i < JOYHATS*4; i++, j <<= 1)
		{
			if (newhats & j) // hat changed state?
			{
				if (joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2HAT1 + i;
				D_PostEvent(&event);
			}
		}
	}

#if 0
	// send joystick axis positions
	event.type = ev_joystick2;

	for (i = JOYAXISSET - 1; i >= 0; i--)
	{
		event.data1 = i;
		if (i*2 + 1 <= JoyInfo2.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 0);
		else axisx = 0;
		if (i*2 + 2 <= JoyInfo2.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 1);
		else axisy = 0;

#ifdef _arch_dreamcast // -128 to 127
		axisx = axisx*8;
		axisy = axisy*8;
#else // -32768 to 32767
		axisx = axisx/32;
		axisy = axisy/32;
#endif

		if (Joystick2.bGamepadStyle)
		{
			// gamepad control type, on or off, live or die
			if (axisx < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (axisx > (JOYAXISRANGE/2))
				event.data2 = 1;
			else
				event.data2 = 0;
			if (axisy < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (axisy > (JOYAXISRANGE/2))
				event.data3 = 1;
			else
				event.data3 = 0;
		}
		else
		{

			axisx = JoyInfo2.scale?((axisx/JoyInfo2.scale)*JoyInfo2.scale):axisx;
			axisy = JoyInfo2.scale?((axisy/JoyInfo2.scale)*JoyInfo2.scale):axisy;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= axisx && axisx <= SDL_JDEADZONE) axisx = 0;
			if (-SDL_JDEADZONE <= axisy && axisy <= SDL_JDEADZONE) axisy = 0;
#endif

			// analog control style , just send the raw data
			event.data2 = axisx; // x axis
			event.data3 = axisy; // y axis
		}
		D_PostEvent(&event);
	}
#endif

}

/**	\brief	Open joystick handle

	\param	fname	name of joystick

	\return	axises


*/
static int joy_open2(const char *fname)
{
	int joyindex = atoi(fname);
	int num_joy = 0;
	int i;

	if (joystick_started == 0 && joystick2_started == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf(M_GetText("Couldn't initialize joystick: %s\n"), SDL_GetError());
			return -1;
		}
		else
			num_joy = SDL_NumJoysticks();

		if (num_joy < joyindex)
		{
			CONS_Printf(M_GetText("Cannot use joystick #%d/(%s), it doesn't exist\n"),joyindex,fname);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#%d/(%s)\n", i+1, SDL_JoystickName(i));
			I_ShutdownJoystick2();
			return -1;
		}
	}
	else
	{
		JoyReset(&JoyInfo2);
		//I_ShutdownJoystick();
		//joy_open(fname);
	}

	num_joy = SDL_NumJoysticks();

	if (joyindex <= 0 || num_joy == 0 || JoyInfo2.oldjoy == joyindex)
	{
//		I_OutputMsg("Unable to use that joystick #(%s), non-number\n",fname);
		if (num_joy != 0)
		{
			CONS_Printf(M_GetText("Found %d joysticks on this system\n"), num_joy);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#%d/(%s)\n", i+1, SDL_JoystickName(i));
		}
		else
			CONS_Printf("%s", M_GetText("Found no joysticks on this system\n"));
		if (joyindex <= 0 || num_joy == 0) return 0;
	}

	JoyInfo2.dev = SDL_JoystickOpen(joyindex-1);
	CONS_Printf(M_GetText("Joystick2: %s\n"), SDL_JoystickName(joyindex-1));

	if (!JoyInfo2.dev)
	{
		CONS_Printf(M_GetText("Couldn't open joystick2: %s\n"), SDL_GetError());
		I_ShutdownJoystick2();
		return -1;
	}
	else
	{
		JoyInfo2.axises = SDL_JoystickNumAxes(JoyInfo2.dev);
		if (JoyInfo2.axises > JOYAXISSET*2)
			JoyInfo2.axises = JOYAXISSET*2;
/*		if (joyaxes < 2)
		{
			I_OutputMsg("Not enought axes?\n");
			I_ShutdownJoystick2();
			return 0;
		}*/

		JoyInfo2.buttons = SDL_JoystickNumButtons(JoyInfo2.dev);
		if (JoyInfo2.buttons > JOYBUTTONS)
			JoyInfo2.buttons = JOYBUTTONS;

#ifdef DC
		JoyInfo2.hats = 0;
#else
		JoyInfo2.hats = SDL_JoystickNumHats(JoyInfo2.dev);
		if (JoyInfo2.hats > JOYHATS)
			JoyInfo2.hats = JOYHATS;

		JoyInfo2.balls = SDL_JoystickNumBalls(JoyInfo2.dev);
#endif

		//Joystick.bGamepadStyle = !stricmp(SDL_JoystickName(SDL_JoystickIndex(JoyInfo2.dev)), "pad");

		return JoyInfo2.axises;
	}
}

//
// I_InitJoystick
//
void I_InitJoystick(void)
{
	I_ShutdownJoystick();
	if (!strcmp(cv_usejoystick.string, "0") || M_CheckParm("-nojoy"))
		return;
	if (joy_open(cv_usejoystick.string) != -1)
		JoyInfo.oldjoy = atoi(cv_usejoystick.string);
	else
	{
		cv_usejoystick.value = 0;
		return;
	}
	joystick_started = 1;
}

void I_InitJoystick2(void)
{
	I_ShutdownJoystick2();
	if (!strcmp(cv_usejoystick2.string, "0") || M_CheckParm("-nojoy"))
		return;
	if (joy_open2(cv_usejoystick2.string) != -1)
		JoyInfo2.oldjoy = atoi(cv_usejoystick2.string);
	else
	{
		cv_usejoystick2.value = 0;
		return;
	}
	joystick2_started = 1;
}

static void I_ShutdownInput(void)
{
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		JoyReset(&JoyInfo);
		JoyReset(&JoyInfo2);
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}

}

INT32 I_NumJoys(void)
{
	INT32 numjoy = 0;
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != -1)
			numjoy = SDL_NumJoysticks();
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	else
		numjoy = SDL_NumJoysticks();
	return numjoy;
}

const char *I_GetJoyName(INT32 joyindex)
{
	const char *joyname = "NA";
	joyindex--; //SDL's Joystick System starts at 0, not 1
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != -1)
			joyname = SDL_JoystickName(joyindex);
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	else
		joyname = SDL_JoystickName(joyindex);
	return joyname;
}

#ifndef NOMUMBLE
#ifdef HAVE_MUMBLE
// Best Mumble positional audio settings:
// Minimum distance 3.0 m
// Bloom 175%
// Maximum distance 80.0 m
// Minimum volume 50%
#define DEG2RAD (0.017453292519943295769236907684883l) // TAU/360 or PI/180
#define MUMBLEUNIT (64.0f) // FRACUNITS in a Meter

static struct {
#ifdef WINMUMBLE
	UINT32 uiVersion;
	DWORD uiTick;
#else
	Uint32 uiVersion;
	Uint32 uiTick;
#endif
	float fAvatarPosition[3];
	float fAvatarFront[3];
	float fAvatarTop[3]; // defaults to Y-is-up (only used for leaning)
	wchar_t name[256]; // game name
	float fCameraPosition[3];
	float fCameraFront[3];
	float fCameraTop[3]; // defaults to Y-is-up (only used for leaning)
	wchar_t identity[256]; // player id
#ifdef WINMUMBLE
	UINT32 context_len;
#else
	Uint32 context_len;
#endif
	unsigned char context[256]; // server/team
	wchar_t description[2048]; // game description
} *mumble = NULL;
#endif // HAVE_MUMBLE

static void I_SetupMumble(void)
{
#ifdef WINMUMBLE
	HANDLE hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (!hMap)
		return;

	mumble = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*mumble));
	if (!mumble)
		CloseHandle(hMap);
#elif defined (HAVE_SHM)
	int shmfd;
	char memname[256];

	snprintf(memname, 256, "/MumbleLink.%d", getuid());
	shmfd = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);

	if(shmfd < 0)
		return;

	mumble = mmap(NULL, sizeof(*mumble), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (mumble == MAP_FAILED)
		mumble = NULL;
#endif
}

void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
#ifdef HAVE_MUMBLE
	double angle;
	fixed_t anglef;

	if (!mumble)
		return;

	if(mumble->uiVersion != 2) {
		wcsncpy(mumble->name, L"SRB2 "VERSIONSTRING, 256);
		wcsncpy(mumble->description, L"Sonic Robo Blast 2 with integrated Mumble Link support.", 2048);
		mumble->uiVersion = 2;
	}
	mumble->uiTick++;

	if (!netgame || gamestate != GS_LEVEL) { // Zero out, but never delink.
		mumble->fAvatarPosition[0] = mumble->fAvatarPosition[1] = mumble->fAvatarPosition[2] = 0.0f;
		mumble->fAvatarFront[0] = 1.0f;
		mumble->fAvatarFront[1] = mumble->fAvatarFront[2] = 0.0f;
		mumble->fCameraPosition[0] = mumble->fCameraPosition[1] = mumble->fCameraPosition[2] = 0.0f;
		mumble->fCameraFront[0] = 1.0f;
		mumble->fCameraFront[1] = mumble->fCameraFront[2] = 0.0f;
		return;
	}

	{
		UINT8 *p = mumble->context;
		WRITEMEM(p, server_context, 8);
		WRITEINT16(p, gamemap);
		mumble->context_len = p - mumble->context;
	}

	if (mobj) {
		mumble->fAvatarPosition[0] = FIXED_TO_FLOAT(mobj->x) / MUMBLEUNIT;
		mumble->fAvatarPosition[1] = FIXED_TO_FLOAT(mobj->z) / MUMBLEUNIT;
		mumble->fAvatarPosition[2] = FIXED_TO_FLOAT(mobj->y) / MUMBLEUNIT;

		anglef = AngleFixed(mobj->angle);
		angle = FIXED_TO_FLOAT(anglef)*DEG2RAD;
		mumble->fAvatarFront[0] = (float)cos(angle);
		mumble->fAvatarFront[1] = 0.0f;
		mumble->fAvatarFront[2] = (float)sin(angle);
	} else {
		mumble->fAvatarPosition[0] = mumble->fAvatarPosition[1] = mumble->fAvatarPosition[2] = 0.0f;
		mumble->fAvatarFront[0] = 1.0f;
		mumble->fAvatarFront[1] = mumble->fAvatarFront[2] = 0.0f;
	}

	mumble->fCameraPosition[0] = FIXED_TO_FLOAT(listener.x) / MUMBLEUNIT;
	mumble->fCameraPosition[1] = FIXED_TO_FLOAT(listener.z) / MUMBLEUNIT;
	mumble->fCameraPosition[2] = FIXED_TO_FLOAT(listener.y) / MUMBLEUNIT;

	anglef = AngleFixed(listener.angle);
	angle = FIXED_TO_FLOAT(anglef)*DEG2RAD;
	mumble->fCameraFront[0] = (float)cos(angle);
	mumble->fCameraFront[1] = 0.0f;
	mumble->fCameraFront[2] = (float)sin(angle);
#else
	(void)mobj;
	(void)listener;
#endif // HAVE_MUMBLE
}
#undef WINMUMBLE
#endif // NOMUMBLE

#ifdef HAVE_TERMIOS

void I_GetMouseEvents(void)
{
	static UINT8 mdata[5];
	static INT32 i = 0,om2b = 0;
	INT32 di, j, mlp, button;
	event_t event;
	const INT32 mswap[8] = {0, 4, 1, 5, 2, 6, 3, 7};

	if (!mouse2_started) return;
	for (mlp = 0; mlp < 20; mlp++)
	{
		for (; i < 5; i++)
		{
			di = read(fdmouse2, mdata+i, 1);
			if (di == -1) return;
		}
		if ((mdata[0] & 0xf8) != 0x80)
		{
			for (j = 1; j < 5; j++)
				if ((mdata[j] & 0xf8) == 0x80)
					for (i = 0; i < 5-j; i++) // shift
						mdata[i] = mdata[i+j];
			if (i < 5) continue;
		}
		else
		{
			button = mswap[~mdata[0] & 0x07];
			for (j = 0; j < MOUSEBUTTONS; j++)
			{
				if (om2b & (1<<j))
				{
					if (!(button & (1<<j))) //keyup
					{
						event.type = ev_keyup;
						event.data1 = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
				else
				{
					if (button & (1<<j))
					{
						event.type = ev_keydown;
						event.data1 = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
			}
			event.data2 = ((SINT8)mdata[1])+((SINT8)mdata[3]);
			event.data3 = ((SINT8)mdata[2])+((SINT8)mdata[4]);
			if (event.data2 && event.data3)
			{
				event.type = ev_mouse2;
				event.data1 = 0;
				D_PostEvent(&event);
			}
		}
		i = 0;
	}
}

//
// I_ShutdownMouse2
//
static void I_ShutdownMouse2(void)
{
	if (fdmouse2 != -1) close(fdmouse2);
	mouse2_started = 0;
}
#elif defined (_WIN32) && !defined (_XBOX)

static HANDLE mouse2filehandle = INVALID_HANDLE_VALUE;

static void I_ShutdownMouse2(void)
{
	event_t event;
	INT32 i;

	if (mouse2filehandle == INVALID_HANDLE_VALUE)
		return;

	SetCommMask(mouse2filehandle, 0);

	EscapeCommFunction(mouse2filehandle, CLRDTR);
	EscapeCommFunction(mouse2filehandle, CLRRTS);

	PurgeComm(mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT |
	          PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle(mouse2filehandle);

	// emulate the up of all mouse buttons
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		event.type = ev_keyup;
		event.data1 = KEY_2MOUSE1+i;
		D_PostEvent(&event);
	}

	mouse2filehandle = INVALID_HANDLE_VALUE;
}

#define MOUSECOMBUFFERSIZE 256
static INT32 handlermouse2x,handlermouse2y,handlermouse2buttons;

static void I_PoolMouse2(void)
{
	UINT8 buffer[MOUSECOMBUFFERSIZE];
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	DWORD dwLength;
	char dx,dy;

	static INT32 bytenum;
	static UINT8 combytes[4];
	DWORD i;

	ClearCommError(mouse2filehandle, &dwErrorFlags, &ComStat);
	dwLength = min(MOUSECOMBUFFERSIZE, ComStat.cbInQue);

	if (dwLength <= 0)
		return;

	if (!ReadFile(mouse2filehandle, buffer, dwLength, &dwLength, NULL))
	{
		CONS_Alert(CONS_WARNING, "%s", M_GetText("Read Error on secondary mouse port\n"));
		return;
	}

	// parse the mouse packets
	for (i = 0; i < dwLength; i++)
	{
		if ((buffer[i] & 64)== 64)
			bytenum = 0;

		if (bytenum < 4)
			combytes[bytenum] = buffer[i];
		bytenum++;

		if (bytenum == 1)
		{
			handlermouse2buttons &= ~3;
			handlermouse2buttons |= ((combytes[0] & (32+16)) >> 4);
		}
		else if (bytenum == 3)
		{
			dx = (char)((combytes[0] &  3) << 6);
			dy = (char)((combytes[0] & 12) << 4);
			dx = (char)(dx + combytes[1]);
			dy = (char)(dy + combytes[2]);
			handlermouse2x+= dx;
			handlermouse2y+= dy;
		}
		else if (bytenum == 4) // fourth UINT8 (logitech mouses)
		{
			if (buffer[i] & 32)
				handlermouse2buttons |= 4;
			else
				handlermouse2buttons &= ~4;
		}
	}
}

void I_GetMouseEvents(void)
{
	static UINT8 lastbuttons2 = 0; //mouse movement
	event_t event;

	if (mouse2filehandle == INVALID_HANDLE_VALUE)
		return;

	I_PoolMouse2();
	// post key event for buttons
	if (handlermouse2buttons != lastbuttons2)
	{
		INT32 i, j = 1, k;
		k = (handlermouse2buttons ^ lastbuttons2); // only changed bit to 1
		lastbuttons2 = (UINT8)handlermouse2buttons;

		for (i = 0; i < MOUSEBUTTONS; i++, j <<= 1)
			if (k & j)
			{
				if (handlermouse2buttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2MOUSE1+i;
				D_PostEvent(&event);
			}
	}

	if (handlermouse2x != 0 || handlermouse2y != 0)
	{
		event.type = ev_mouse2;
		event.data1 = 0;
//		event.data1 = buttons; // not needed
		event.data2 = handlermouse2x << 1;
		event.data3 = -handlermouse2y << 1;
		handlermouse2x = 0;
		handlermouse2y = 0;

		D_PostEvent(&event);
	}
}
#else
void I_GetMouseEvents(void){};
#endif

//
// I_StartupMouse2
//
void I_StartupMouse2(void)
{
#ifdef HAVE_TERMIOS
	struct termios m2tio;
	size_t i;
	INT32 dtr = -1, rts = -1;;
	I_ShutdownMouse2();
	if (cv_usemouse2.value == 0) return;
	if ((fdmouse2 = open(cv_mouse2port.string, O_RDONLY|O_NONBLOCK|O_NOCTTY)) == -1)
	{
		CONS_Printf(M_GetText("Error opening %s!\n"), cv_mouse2port.string);
		return;
	}
	tcflush(fdmouse2, TCIOFLUSH);
	m2tio.c_iflag = IGNBRK;
	m2tio.c_oflag = 0;
	m2tio.c_cflag = CREAD|CLOCAL|HUPCL|CS8|CSTOPB|B1200;
	m2tio.c_lflag = 0;
	m2tio.c_cc[VTIME] = 0;
	m2tio.c_cc[VMIN] = 1;
	tcsetattr(fdmouse2, TCSANOW, &m2tio);
	for (i = 0; i < strlen(cv_mouse2opt.string); i++)
	{
		if (toupper(cv_mouse2opt.string[i]) == 'D')
		{
			if (cv_mouse2opt.string[i+1] == '-')
				dtr = 0;
			else
				dtr = 1;
		}
		if (toupper(cv_mouse2opt.string[i]) == 'R')
		{
			if (cv_mouse2opt.string[i+1] == '-')
				rts = 0;
			else
				rts = 1;
		}
		if (dtr != -1 || rts != -1)
		{
			INT32 c;
			if (!ioctl(fdmouse2, TIOCMGET, &c))
			{
				if (!dtr)
					c &= ~TIOCM_DTR;
				else if (dtr > 0)
					c |= TIOCM_DTR;
			}
			if (!rts)
				c &= ~TIOCM_RTS;
			else if (rts > 0)
				c |= TIOCM_RTS;
			ioctl(fdmouse2, TIOCMSET, &c);
		}
	}
	mouse2_started = 1;
	I_AddExitFunc(I_ShutdownMouse2);
#elif defined (_WIN32) && !defined (_XBOX)
	DCB dcb;

	if (mouse2filehandle != INVALID_HANDLE_VALUE)
		I_ShutdownMouse2();

	if (cv_usemouse2.value == 0)
		return;

	if (mouse2filehandle == INVALID_HANDLE_VALUE)
	{
		// COM file handle
		mouse2filehandle = CreateFileA(cv_mouse2port.string, GENERIC_READ | GENERIC_WRITE,
		                               0,                     // exclusive access
		                               NULL,                  // no security attrs
		                               OPEN_EXISTING,
		                               FILE_ATTRIBUTE_NORMAL,
		                               NULL);
		if (mouse2filehandle == INVALID_HANDLE_VALUE)
		{
			INT32 e = GetLastError();
			if (e == 5)
				CONS_Alert(CONS_ERROR, M_GetText("Can't open %s: Access denied\n"), cv_mouse2port.string);
			else
				CONS_Alert(CONS_ERROR, M_GetText("Can't open %s: error %d\n"), cv_mouse2port.string, e);
			return;
		}
	}

	// getevent when somthing happens
	//SetCommMask(mouse2filehandle, EV_RXCHAR);

	// buffers
	SetupComm(mouse2filehandle, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);

	// purge buffers
	PurgeComm(mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT
	          | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// setup port to 1200 7N1
	dcb.DCBlength = sizeof (DCB);

	GetCommState(mouse2filehandle, &dcb);

	dcb.BaudRate = CBR_1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	SetCommState(mouse2filehandle, &dcb);
	I_AddExitFunc(I_ShutdownMouse2);
#endif
}

//
// I_Tactile
//
void I_Tactile(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	(void)pFFType;
	(void)FFEffect;
}

void I_Tactile2(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	(void)pFFType;
	(void)FFEffect;
}

/**	\brief empty ticcmd for player 1
*/
static ticcmd_t emptycmd;

ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

/**	\brief empty ticcmd for player 2
*/
static ticcmd_t emptycmd2;

ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
static HMODULE winmm = NULL;
static DWORD starttickcount = 0; // hack for win2k time bug
static p_timeGetTime pfntimeGetTime = NULL;

// ---------
// I_GetTime
// Use the High Resolution Timer if available,
// else use the multimedia timer which has 1 millisecond precision on Windowz 95,
// but lower precision on Windows NT
// ---------

tic_t I_GetTime(void)
{
	tic_t newtics = 0;

	if (!starttickcount) // high precision timer
	{
		LARGE_INTEGER currtime; // use only LowPart if high resolution counter is not available
		static LARGE_INTEGER basetime = {{0, 0}};

		// use this if High Resolution timer is found
		static LARGE_INTEGER frequency;

		if (!basetime.LowPart)
		{
			if (!QueryPerformanceFrequency(&frequency))
				frequency.QuadPart = 0;
			else
				QueryPerformanceCounter(&basetime);
		}

		if (frequency.LowPart && QueryPerformanceCounter(&currtime))
		{
			newtics = (INT32)((currtime.QuadPart - basetime.QuadPart) * NEWTICRATE
				/ frequency.QuadPart);
		}
		else if (pfntimeGetTime)
		{
			currtime.LowPart = pfntimeGetTime();
			if (!basetime.LowPart)
				basetime.LowPart = currtime.LowPart;
			newtics = ((currtime.LowPart - basetime.LowPart)/(1000/NEWTICRATE));
		}
	}
	else
		newtics = (GetTickCount() - starttickcount)/(1000/NEWTICRATE);

	return newtics;
}

static void I_ShutdownTimer(void)
{
	pfntimeGetTime = NULL;
	if (winmm)
	{
		p_timeEndPeriod pfntimeEndPeriod = (p_timeEndPeriod)GetProcAddress(winmm, "timeEndPeriod");
		if (pfntimeEndPeriod)
			pfntimeEndPeriod(1);
		FreeLibrary(winmm);
		winmm = NULL;
	}
}
#else
//
// I_GetTime
// returns time in 1/TICRATE second tics
//
tic_t I_GetTime (void)
{
#ifdef _arch_dreamcast
	static Uint64 basetime = 0;
	       Uint64 ticks = timer_ms_gettime64(); //using timer_ms_gettime64 instand of SDL_GetTicks for the Dreamcast
#else
	static Uint32 basetime = 0;
	       Uint32 ticks = SDL_GetTicks();
#endif

	if (!basetime)
		basetime = ticks;

	ticks -= basetime;

	ticks = (ticks*TICRATE);

#if 0 //#ifdef _WIN32_WCE
	ticks = (ticks/10);
#else
	ticks = (ticks/1000);
#endif

	return (tic_t)ticks;
}
#endif

//
//I_StartupTimer
//
void I_StartupTimer(void)
{
#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
	// for win2k time bug
	if (M_CheckParm("-gettickcount"))
	{
		starttickcount = GetTickCount();
		CONS_Printf("%s", M_GetText("Using GetTickCount()\n"));
	}
	winmm = LoadLibraryA("winmm.dll");
	if (winmm)
	{
		p_timeEndPeriod pfntimeBeginPeriod = (p_timeEndPeriod)GetProcAddress(winmm, "timeBeginPeriod");
		if (pfntimeBeginPeriod)
			pfntimeBeginPeriod(1);
		pfntimeGetTime = (p_timeGetTime)GetProcAddress(winmm, "timeGetTime");
	}
	I_AddExitFunc(I_ShutdownTimer);
#elif 0 //#elif !defined (_arch_dreamcast) && !defined(GP2X) // the DC have it own timer and GP2X have broken pthreads?
	if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
		I_Error("SRB2: Needs SDL_Timer, Error: %s", SDL_GetError());
#endif
}



void I_Sleep(void)
{
#if !(defined (_arch_dreamcast) || defined (_XBOX))
	if (cv_sleep.value != -1)
		SDL_Delay(cv_sleep.value);
#endif
}

INT32 I_StartupSystem(void)
{
	SDL_version SDLcompiled;
	const SDL_version *SDLlinked;
#ifdef _XBOX
#ifdef __GNUC__
	char DP[] ="      Sonic Robo Blast 2!\n";
	debugPrint(DP);
#endif
	unlink("e:/Games/SRB2/stdout.txt");
	freopen("e:/Games/SRB2/stdout.txt", "w+", stdout);
	unlink("e:/Games/SRB2/stderr.txt");
	freopen("e:/Games/SRB2/stderr.txt", "w+", stderr);
#endif
#ifdef _arch_dreamcast
#ifdef _DEBUG
	//gdb_init();
#endif
	printf(__FILE__":%i\n",__LINE__);
#ifdef _DEBUG
	//gdb_breakpoint();
#endif
#endif
	SDL_VERSION(&SDLcompiled)
	SDLlinked = SDL_Linked_Version();
	I_StartupConsole();
	I_OutputMsg("Compiled for SDL version: %d.%d.%d\n",
	 SDLcompiled.major, SDLcompiled.minor, SDLcompiled.patch);
	I_OutputMsg("Linked with SDL version: %d.%d.%d\n",
	 SDLlinked->major, SDLlinked->minor, SDLlinked->patch);
#if 0 //#ifdef GP2X //start up everything
	if (SDL_Init(SDL_INIT_NOPARACHUTE|SDL_INIT_EVERYTHING) < 0)
#else
	if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
#endif
		I_Error("SRB2: SDL System Error: %s", SDL_GetError()); //Alam: Oh no....
#ifndef NOMUMBLE
	I_SetupMumble();
#endif
	return 0;
}


//
// I_Quit
//
void I_Quit(void)
{
	static SDL_bool quiting = SDL_FALSE;

	/* prevent recursive I_Quit() */
	if (quiting) goto death;
	SDLforceUngrabMouse();
	quiting = SDL_FALSE;
	I_ShutdownConsole();
	M_SaveConfig(NULL); //save game config, cvars..
#ifndef NONET
	D_SaveBan(); // save the ban list
#endif
	G_SaveGameData(); // Tails 12-08-2002
	//added:16-02-98: when recording a demo, should exit using 'q' key,
	//        but sometimes we forget and use 'F10'.. so save here too.

	if (demorecording)
		G_CheckDemoStatus();
	if (metalrecording)
		G_StopMetalRecording();

	D_QuitNetGame();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
	I_ShutdownInput();
	I_ShutdownSystem();
#ifndef _arch_dreamcast
	SDL_Quit();
#endif
	/* if option -noendtxt is set, don't print the text */
	if (!M_CheckParm("-noendtxt") && W_CheckNumForName("ENDOOM") != LUMPERROR)
	{
		printf("\r");
		ShowEndTxt();
	}
death:
	W_Shutdown();
#ifdef GP2X
	chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#endif
	exit(0);
}

void I_WaitVBL(INT32 count)
{
	count = 1;
	SDL_Delay(count);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

//
// I_Error
//
/**	\brief phuck recursive errors
*/
static INT32 errorcount = 0;

/**	\brief recursive error detecting
*/
static boolean shutdowning = false;

void I_Error(const char *error, ...)
{
	va_list argptr;
#if (defined (MAC_ALERT) || defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__))) && !defined (_XBOX)
	char buffer[8192];
#endif

	// recursive error detecting
	if (shutdowning)
	{
		errorcount++;
		if (errorcount == 1)
			SDLforceUngrabMouse();
		// try to shutdown each subsystem separately
		if (errorcount == 2)
			I_ShutdownMusic();
		if (errorcount == 3)
			I_ShutdownSound();
		if (errorcount == 4)
			I_ShutdownCD();
		if (errorcount == 5)
			I_ShutdownGraphics();
		if (errorcount == 6)
			I_ShutdownInput();
		if (errorcount == 7)
			I_ShutdownSystem();
#ifndef _arch_dreamcast
		if (errorcount == 8)
			SDL_Quit();
#endif
		if (errorcount == 9)
		{
			M_SaveConfig(NULL);
			G_SaveGameData();
		}
		if (errorcount > 20)
		{
#ifdef MAC_ALERT
			va_start(argptr, error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
			// 2004-03-03 AJR Since the Mac user is most likely double clicking to run the game, give them a panel.
			MacShowAlert("Recursive Error", buffer, "Quit", NULL, NULL);
#elif (defined (_WIN32) || (defined (_WIN32_WCE)) && !defined (__GNUC__)) && !defined (_XBOX)
			va_start(argptr,error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
#ifndef _WIN32_WCE
			{
				HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
				DWORD bytesWritten;
				if (co != INVALID_HANDLE_VALUE)
				{
					if (GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &bytesWritten))
						WriteConsoleA(co, buffer, (DWORD)strlen(buffer), NULL, NULL);
					else
						WriteFile(co, buffer, (DWORD)strlen(buffer), &bytesWritten, NULL);
				}
			}
#endif
			OutputDebugStringA(buffer);
			MessageBoxA(vid.WndParent, buffer, "SRB2 Recursive Error", MB_OK|MB_ICONERROR);
#else
			// Don't print garbage
			va_start(argptr, error);
			if (!framebuffer)
				vfprintf (stderr, error, argptr);
			va_end(argptr);
#endif
			W_Shutdown();
#ifdef GP2X
			chdir("/usr/gp2x");
			execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#endif
			exit(-1); // recursive errors detected
		}
	}
	shutdowning = true;
	I_ShutdownConsole();
#ifndef MAC_ALERT
	// Message first.
	va_start(argptr,error);
	if (!framebuffer)
	{
		fprintf(stderr, "Error: ");
		vfprintf(stderr,error,argptr);
		fprintf(stderr, "\n");
	}
	va_end(argptr);

	if (!framebuffer)
		fflush(stderr);
#endif
	M_SaveConfig(NULL); // save game config, cvars..
#ifndef NONET
	D_SaveBan(); // save the ban list
#endif
	G_SaveGameData(); // Tails 12-08-2002

	// Shutdown. Here might be other errors.
	if (demorecording)
		G_CheckDemoStatus();
	if (metalrecording)
		G_StopMetalRecording();

	D_QuitNetGame();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
	I_ShutdownInput();
	I_ShutdownSystem();
#ifndef _arch_dreamcast
	SDL_Quit();
#endif
#ifdef MAC_ALERT
	va_start(argptr, error);
	vsprintf(buffer, error, argptr);
	va_end(argptr);
	// 2004-03-03 AJR Since the Mac user is most likely double clicking to run the game, give them a panel.
	MacShowAlert("Critical Error", buffer, "Quit", NULL, NULL);
#endif
	W_Shutdown();
#if defined (PARANOIA) && defined (__CYGWIN__)
		*(INT32 *)2 = 4; //Alam: Debug!
#endif
#ifdef GP2X
	chdir("/usr/gp2x");
	execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
#endif
	exit(-1);
}

/**	\brief quit function table
*/
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS]; /* initialized to all bits 0 */

//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	INT32 c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}


//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	INT32 c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c < MAX_QUIT_FUNCS-1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

//
//  Closes down everything. This includes restoring the initial
//  palette and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE: Shutdown user funcs are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
	INT32 c;

	for (c = MAX_QUIT_FUNCS-1; c >= 0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
#ifdef  LOGMESSAGES
	if (logstream)
	{
		fclose(logstream);
		logstream = NULL;
	}
#endif

}

void I_GetDiskFreeSpace(INT64 *freespace)
{
#if defined (_arch_dreamcast) || defined (_PSP)
	*freespace = 0;
#elif defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#if defined (SOLARIS) || defined (__HAIKU__) || defined (_WII) || defined (_PS3)
	*freespace = INT32_MAX;
	return;
#else // Both Linux and BSD have this, apparently.
	struct statfs stfs;
	if (statfs(".", &stfs) == -1)
	{
		*freespace = INT32_MAX;
		return;
	}
	*freespace = stfs.f_bavail * stfs.f_bsize;
#endif
#elif (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
	static p_GetDiskFreeSpaceExA pfnGetDiskFreeSpaceEx = NULL;
	static boolean testwin95 = false;
	ULARGE_INTEGER usedbytes, lfreespace;

	if (!testwin95)
	{
		pfnGetDiskFreeSpaceEx = (p_GetDiskFreeSpaceExA)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA");
		testwin95 = true;
	}
	if (pfnGetDiskFreeSpaceEx)
	{
		if (pfnGetDiskFreeSpaceEx(NULL, &lfreespace, &usedbytes, NULL))
			*freespace = lfreespace.QuadPart;
		else
			*freespace = INT32_MAX;
	}
	else
	{
		DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
		GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
		                 &NumberOfFreeClusters, &TotalNumberOfClusters);
		*freespace = BytesPerSector*SectorsPerCluster*NumberOfFreeClusters;
	}
#else // Dummy for platform independent; 1GB should be enough
	*freespace = 1024*1024*1024;
#endif
}

char *I_GetUserName(void)
{
#ifdef GP2X
	static char username[MAXPLAYERNAME] = "GP2XUSER";
	return username;
#elif defined (PSP)
	static char username[MAXPLAYERNAME] = "PSPUSER";
	return username;
#elif !(defined (_WIN32_WCE) || defined (_XBOX))
	static char username[MAXPLAYERNAME];
	char *p;
#ifdef _WIN32
	DWORD i = MAXPLAYERNAME;

	if (!GetUserNameA(username, &i))
#endif
	{
		p = I_GetEnv("USER");
		if (!p)
		{
			p = I_GetEnv("user");
			if (!p)
			{
				p = I_GetEnv("USERNAME");
				if (!p)
				{
					p = I_GetEnv("username");
					if (!p)
					{
						return NULL;
					}
				}
			}
		}
		strncpy(username, p, MAXPLAYERNAME);
	}


	if (strcmp(username, "") != 0)
		return username;
#endif
	return NULL; // dummy for platform independent version
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{
//[segabor]
#if defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (__CYGWIN__) || defined (__OS2__)
	return mkdir(dirname, unixright);
#elif (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__))) && !defined (_XBOX)
	UNREFERENCED_PARAMETER(unixright); /// \todo should implement ntright under nt...
	return CreateDirectoryA(dirname, NULL);
#else
	(void)dirname;
	(void)unixright;
	return false;
#endif
}

char *I_GetEnv(const char *name)
{
#ifdef NEED_SDL_GETENV
	return SDL_getenv(name);
#elif defined(_WIN32_WCE)
	(void)name;
	return NULL;
#else
	return getenv(name);
#endif
}

INT32 I_PutEnv(char *variable)
{
#ifdef NEED_SDL_GETENV
	return SDL_putenv(variable);
#elif defined(_WIN32_WCE)
	return ((variable)?-1:0);
#else
	return putenv(variable);
#endif
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

char *I_ClipboardPaste(void)
{
	return NULL;
}

/**	\brief	The isWadPathOk function

	\param	path	string path to check

	\return if true, wad file found


*/
static boolean isWadPathOk(const char *path)
{
	char *wad3path = malloc(256);

	if (!wad3path)
		return false;

	sprintf(wad3path, pandf, path, WADKEYWORD1);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	sprintf(wad3path, pandf, path, WADKEYWORD2);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	free(wad3path);
	return false;
}

static void pathonly(char *s)
{
	size_t j;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			if (s[j] == ':') s[j+1] = 0;
			else s[j] = 0;
			return;
		}
}

/**	\brief	search for srb2.srb in the given path

	\param	searchDir	starting path

	\return	WAD path if not NULL


*/
static const char *searchWad(const char *searchDir)
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, WADKEYWORD1);
	fstemp = filesearch(tempsw,searchDir,NULL,true,20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}

	strcpy(tempsw, WADKEYWORD2);
	fstemp = filesearch(tempsw, searchDir, NULL, true, 20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}
	return NULL;
}

/**	\brief go through all possible paths and look for srb2.srb

  \return path to srb2.srb if any
*/
static const char *locateWad(void)
{
	const char *envstr;
	const char *WadPath;

	I_OutputMsg("SRB2WADDIR");
	// does SRB2WADDIR exist?
	if (((envstr = I_GetEnv("SRB2WADDIR")) != NULL) && isWadPathOk(envstr))
		return envstr;

#if defined(_WIN32_WCE) || defined(_PS3) || defined(_PSP)
	// examine argv[0]
	strcpy(returnWadPath, myargv[0]);
	pathonly(returnWadPath);
	I_PutEnv(va("HOME=%s",returnWadPath));
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif

#ifndef NOCWD
	I_OutputMsg(",.");
	// examine current dir
	strcpy(returnWadPath, ".");
	if (isWadPathOk(returnWadPath))
		return NULL;
#endif

	// examine default dirs
#ifdef DEFAULTWADLOCATION1
	I_OutputMsg(","DEFAULTWADLOCATION1);
	strcpy(returnWadPath, DEFAULTWADLOCATION1);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION2
	I_OutputMsg(","DEFAULTWADLOCATION2);
	strcpy(returnWadPath, DEFAULTWADLOCATION2);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION3
	I_OutputMsg(","DEFAULTWADLOCATION3);
	strcpy(returnWadPath, DEFAULTWADLOCATION3);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION4
	I_OutputMsg(","DEFAULTWADLOCATION4);
	strcpy(returnWadPath, DEFAULTWADLOCATION4);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION5
	I_OutputMsg(","DEFAULTWADLOCATION5);
	strcpy(returnWadPath, DEFAULTWADLOCATION5);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION6
	I_OutputMsg(","DEFAULTWADLOCATION6);
	strcpy(returnWadPath, DEFAULTWADLOCATION6);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION7
	I_OutputMsg(","DEFAULTWADLOCATION7);
	strcpy(returnWadPath, DEFAULTWADLOCATION7);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifndef NOHOME
	// find in $HOME
	I_OutputMsg(",HOME");
	if ((envstr = I_GetEnv("HOME")) != NULL)
	{
		WadPath = searchWad(envstr);
		if (WadPath)
			return WadPath;
	}
#endif
#ifdef DEFAULTSEARCHPATH1
	// find in /usr/local
	I_OutputMsg(", in:"DEFAULTSEARCHPATH1);
	WadPath = searchWad(DEFAULTSEARCHPATH1);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH2
	// find in /usr/games
	I_OutputMsg(", in:"DEFAULTSEARCHPATH2);
	WadPath = searchWad(DEFAULTSEARCHPATH2);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH3
	// find in ???
	I_OutputMsg(", in:"DEFAULTSEARCHPATH3);
	WadPath = searchWad(DEFAULTSEARCHPATH3);
	if (WadPath)
		return WadPath;
#endif
	// if nothing was found
	return NULL;
}

const char *I_LocateWad(void)
{
	const char *waddir;

	I_OutputMsg("Looking for WADs in: ");
	waddir = locateWad();
	I_OutputMsg("\n");

	if (waddir)
	{
		// change to the directory where we found srb2.srb
#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
		SetCurrentDirectoryA(waddir);
#elif !defined (_WIN32_WCE) && !defined (_PS3)
		if (chdir(waddir) == -1)
			I_OutputMsg("Couldn't change working directory\n");
#endif
	}
	return waddir;
}

#ifdef LINUX
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"
#endif

// quick fix for compil
UINT32 I_GetFreeMem(UINT32 *total)
{
#if defined (_arch_dreamcast)
	//Dreamcast!
	if (total)
		*total = 16<<20;
	return 8<<20;
#elif defined (_PSP)
	// PSP
	if (total)
		*total = 32<<20;
	return 16<<20;
#elif defined (FREEBSD)
	struct vmmeter sum;
	kvm_t *kd;
	struct nlist namelist[] =
	{
#define X_SUM   0
		{"_cnt"},
		{NULL}
	};
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
	{
		*total = 0L;
		return 0;
	}
	if (kvm_nlist(kd, namelist) != 0)
	{
		kvm_close (kd);
		*total = 0L;
		return 0;
	}
	if (kvm_read(kd, namelist[X_SUM].n_value, &sum,
		sizeof (sum)) != sizeof (sum))
	{
		kvm_close(kd);
		*total = 0L;
		return 0;
	}
	kvm_close(kd);

	if (total)
		*total = sum.v_page_count * sum.v_page_size;
	return sum.v_free_count * sum.v_page_size;
#elif defined (SOLARIS)
	/* Just guess */
	if (total)
		*total = 32 << 20;
	return 32 << 20;
#elif defined (LINUX)
	/* Linux */
	char buf[1024];
	char *memTag;
	UINT32 freeKBytes;
	UINT32 totalKBytes;
	INT32 n;
	INT32 meminfo_fd = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if (n < 0)
	{
		// Error
		*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if (NULL == (memTag = strstr(buf, MEMTOTAL)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMTOTAL);
	totalKBytes = atoi(memTag);

	if (NULL == (memTag = strstr(buf, MEMFREE)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMFREE);
	freeKBytes = atoi(memTag);

	if (total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#elif (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__))) && !defined (_XBOX)
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if (total)
		*total = (UINT32)info.dwTotalPhys;
	return (UINT32)info.dwAvailPhys;
#elif defined (__OS2__)
	UINT32 pr_arena;

	if (total)
		DosQuerySysInfo( QSV_TOTPHYSMEM, QSV_TOTPHYSMEM,
							(PVOID) total, sizeof (UINT32));
	DosQuerySysInfo( QSV_MAXPRMEM, QSV_MAXPRMEM,
				(PVOID) &pr_arena, sizeof (UINT32));

	return pr_arena;
#else
	// Guess 48 MB.
	if (total)
		*total = 48<<20;
	return 48<<20;
#endif /* LINUX */
}

const CPUInfoFlags *I_CPUInfo(void)
{
#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
	static CPUInfoFlags WIN_CPUInfo;
	SYSTEM_INFO SI;
	p_IsProcessorFeaturePresent pfnCPUID = (p_IsProcessorFeaturePresent)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsProcessorFeaturePresent");

	ZeroMemory(&WIN_CPUInfo,sizeof (WIN_CPUInfo));
	if (pfnCPUID)
	{
		WIN_CPUInfo.FPPE       = pfnCPUID( 0); //PF_FLOATING_POINT_PRECISION_ERRATA
		WIN_CPUInfo.FPE        = pfnCPUID( 1); //PF_FLOATING_POINT_EMULATED
		WIN_CPUInfo.cmpxchg    = pfnCPUID( 2); //PF_COMPARE_EXCHANGE_DOUBLE
		WIN_CPUInfo.MMX        = pfnCPUID( 3); //PF_MMX_INSTRUCTIONS_AVAILABLE
		WIN_CPUInfo.PPCMM64    = pfnCPUID( 4); //PF_PPC_MOVEMEM_64BIT_OK
		WIN_CPUInfo.ALPHAbyte  = pfnCPUID( 5); //PF_ALPHA_BYTE_INSTRUCTIONS
		WIN_CPUInfo.SSE        = pfnCPUID( 6); //PF_XMMI_INSTRUCTIONS_AVAILABLE
		WIN_CPUInfo.AMD3DNow   = pfnCPUID( 7); //PF_3DNOW_INSTRUCTIONS_AVAILABLE
		WIN_CPUInfo.RDTSC      = pfnCPUID( 8); //PF_RDTSC_INSTRUCTION_AVAILABLE
		WIN_CPUInfo.PAE        = pfnCPUID( 9); //PF_PAE_ENABLED
		WIN_CPUInfo.SSE2       = pfnCPUID(10); //PF_XMMI64_INSTRUCTIONS_AVAILABLE
		//WIN_CPUInfo.blank    = pfnCPUID(11); //PF_SSE_DAZ_MODE_AVAILABLE
		WIN_CPUInfo.DEP        = pfnCPUID(12); //PF_NX_ENABLED
		WIN_CPUInfo.SSE3       = pfnCPUID(13); //PF_SSE3_INSTRUCTIONS_AVAILABLE
		WIN_CPUInfo.cmpxchg16b = pfnCPUID(14); //PF_COMPARE_EXCHANGE128
		WIN_CPUInfo.cmp8xchg16 = pfnCPUID(15); //PF_COMPARE64_EXCHANGE128
		WIN_CPUInfo.PFC        = pfnCPUID(16); //PF_CHANNELS_ENABLED
	}
#ifdef HAVE_SDLCPUINFO
	else
	{
		WIN_CPUInfo.RDTSC       = SDL_HasRDTSC();
		WIN_CPUInfo.MMX         = SDL_HasMMX();
		WIN_CPUInfo.AMD3DNow    = SDL_Has3DNow();
		WIN_CPUInfo.SSE         = SDL_HasSSE();
		WIN_CPUInfo.SSE2        = SDL_HasSSE2();
		WIN_CPUInfo.AltiVec     = SDL_HasAltiVec();
	}
	WIN_CPUInfo.MMXExt      = SDL_HasMMXExt();
	WIN_CPUInfo.AMD3DNowExt = SDL_Has3DNowExt();
#endif
	GetSystemInfo(&SI);
	WIN_CPUInfo.CPUs = SI.dwNumberOfProcessors;
	WIN_CPUInfo.IA64 = (SI.dwProcessorType == 2200); // PROCESSOR_INTEL_IA64
	WIN_CPUInfo.AMD64 = (SI.dwProcessorType == 8664); // PROCESSOR_AMD_X8664
	return &WIN_CPUInfo;
#elif defined (HAVE_SDLCPUINFO)
	static CPUInfoFlags SDL_CPUInfo;
	memset(&SDL_CPUInfo,0,sizeof (CPUInfoFlags));
	SDL_CPUInfo.RDTSC       = SDL_HasRDTSC();
	SDL_CPUInfo.MMX         = SDL_HasMMX();
	SDL_CPUInfo.MMXExt      = SDL_HasMMXExt();
	SDL_CPUInfo.AMD3DNow    = SDL_Has3DNow();
	SDL_CPUInfo.AMD3DNowExt = SDL_Has3DNowExt();
	SDL_CPUInfo.SSE         = SDL_HasSSE();
	SDL_CPUInfo.SSE2        = SDL_HasSSE2();
	SDL_CPUInfo.AltiVec     = SDL_HasAltiVec();
	return &SDL_CPUInfo;
#else
	return NULL; /// \todo CPUID asm
#endif
}

#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
static void CPUAffinity_OnChange(void);
static consvar_t cv_cpuaffinity = {"cpuaffinity", "-1", CV_SAVE | CV_CALL, NULL, CPUAffinity_OnChange, 0, NULL, NULL, 0, 0, NULL};

static p_GetCurrentProcess pfnGetCurrentProcess = NULL;
static p_GetProcessAffinityMask pfnGetProcessAffinityMask = NULL;
static p_SetProcessAffinityMask pfnSetProcessAffinityMask = NULL;

static inline VOID GetAffinityFuncs(VOID)
{
	HMODULE h = GetModuleHandleA("kernel32.dll");
	pfnGetCurrentProcess = (p_GetCurrentProcess)GetProcAddress(h, "GetCurrentProcess");
	pfnGetProcessAffinityMask = (p_GetProcessAffinityMask)GetProcAddress(h, "GetProcessAffinityMask");
	pfnSetProcessAffinityMask = (p_SetProcessAffinityMask)GetProcAddress(h, "SetProcessAffinityMask");
}

static void CPUAffinity_OnChange(void)
{
	DWORD_PTR dwProcMask, dwSysMask;
	HANDLE selfpid;

	if (!pfnGetCurrentProcess || !pfnGetProcessAffinityMask || !pfnSetProcessAffinityMask)
		return;
	else
		selfpid = pfnGetCurrentProcess();

	pfnGetProcessAffinityMask(selfpid, &dwProcMask, &dwSysMask);

	/* If resulting mask is zero, don't change anything and fall back to
	 * actual mask.
	 */
	if(dwSysMask & cv_cpuaffinity.value)
	{
		pfnSetProcessAffinityMask(selfpid, dwSysMask & cv_cpuaffinity.value);
		CV_StealthSetValue(&cv_cpuaffinity, (INT32)(dwSysMask & cv_cpuaffinity.value));
	}
	else
		CV_StealthSetValue(&cv_cpuaffinity, (INT32)dwProcMask);
}
#endif

void I_RegisterSysCommands(void)
{
#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
	GetAffinityFuncs();
	CV_RegisterVar(&cv_cpuaffinity);
#endif
}
#endif
