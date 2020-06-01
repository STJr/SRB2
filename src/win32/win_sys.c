// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------
/// \file
/// \brief win32 system i/o
///
///	Startup & Shutdown routines for music,sound,timer,keyboard,...
///	Signal handler to trap errors and exit cleanly.

#include "../doomdef.h"

#ifdef _WINDOWS

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>
#include <direct.h>

#include <mmsystem.h>

#include "../m_misc.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"

#include "../d_net.h"
#include "../g_game.h"

#include "../d_main.h"

#include "../m_argv.h"

#include "../w_wad.h"
#include "../z_zone.h"
#include "../g_input.h"

#include "../keys.h"

#include "../screen.h"

#include "../m_menu.h"

// Wheel support for Win95/WinNT3.51
#include <zmouse.h>

// Taken from Win98/NT4.0
#ifndef SM_MOUSEWHEELPRESENT
#define SM_MOUSEWHEELPRESENT 75
#endif

#ifndef MSH_MOUSEWHEEL
#ifdef UNICODE
#define MSH_MOUSEWHEEL L"MSWHEEL_ROLLMSG"
#else
#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"
#endif
#endif

#include "win_main.h"
#include "win_dbg.h"
#include "../i_joy.h"

#ifndef NOMUMBLE
// Mumble context string
#include "../d_clisrv.h"
#include "../byteptr.h"
#endif

#define DIRECTINPUT_VERSION     0x0500
// Force dinput.h to generate old DX3 headers.
#define DXVERSION_NTCOMPATIBLE  0x0300
#include <dinput.h>
#ifndef IDirectInputEffect_Stop
#define IDirectInputEffect_Stop(p) (p)->lpVtbl->Stop(p)
#endif
#ifndef IDirectInputEffect_SetParameters
#define IDirectInputEffect_SetParameters(p,a,b)   (p)->lpVtbl->SetParameters(p,a,b)
#endif
#ifndef IDirectInputEffect_Release
#define IDirectInputEffect_Release(p)            (p)->lpVtbl->Release(p)
#endif

#include "fabdxlib.h"

/// \brief max number of joystick buttons
#define JOYBUTTONS_MAX 32 // rgbButtons[32]
/// \brief max number of joystick button events
#define JOYBUTTONS_MIN min((JOYBUTTONS),(JOYBUTTONS_MAX))

/// \brief max number of joysick axies
#define JOYAXISSET_MAX 4 // (lX, lY), (lZ,lRx), (lRy, lRz), rglSlider[2] is very diff
/// \brief max number ofjoystick axis events
#define JOYAXISSET_MIN min((JOYAXISSET),(JOYAXISSET_MAX))

/// \brief max number of joystick hats
#define JOYHATS_MAX 4 // rgdwPOV[4];
/// \brief max number of joystick hat events
#define JOYHATS_MIN min((JOYHATS),(JOYHATS_MAX))

/// \brief max number of mouse buttons
#define MOUSEBUTTONS_MAX 8 // 8 bit of BYTE and DIMOFS_BUTTON7
/// \brief max number of muse button events
#define MOUSEBUTTONS_MIN min((MOUSEBUTTONS),(MOUSEBUTTONS_MAX))

// ==================
// DIRECT INPUT STUFF
// ==================
BOOL bDX0300; // if true, we created a DirectInput 0x0300 version
static LPDIRECTINPUTA lpDI = NULL;
static LPDIRECTINPUTDEVICEA lpDIK = NULL;   // Keyboard
static LPDIRECTINPUTDEVICEA lpDIM = NULL;   // mice
static LPDIRECTINPUTDEVICEA lpDIJ = NULL;   // joystick 1P
static LPDIRECTINPUTEFFECT lpDIE[NumberofForces];   // joystick 1Es
static LPDIRECTINPUTDEVICE2A lpDIJA = NULL; // joystick 1I
static LPDIRECTINPUTDEVICEA lpDIJ2 = NULL;  // joystick 2P
static LPDIRECTINPUTEFFECT lpDIE2[NumberofForces];  // joystick 1Es
static LPDIRECTINPUTDEVICE2A lpDIJ2A = NULL;// joystick 2I

// Do not execute cleanup code more than once. See Shutdown_xxx() routines.
UINT8 graphics_started = 0;
UINT8 keyboard_started = 0;
UINT8 sound_started = 0;
static boolean mouse_enabled = false;
static boolean joystick_detected = false;
static boolean joystick2_detected = false;

static VOID I_ShutdownKeyboard(VOID);
static VOID I_GetKeyboardEvents(VOID);
static VOID I_ShutdownJoystick(VOID);
static VOID I_ShutdownJoystick2(VOID);

static boolean entering_con_command = false;

//
// Why would this be system specific?? hmmmm....
//
// it is for virtual reality system, next incoming feature :)
static ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

static ticcmd_t emptycmd2;
ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

// Allocates the base zone memory,
// this function returns a valid pointer and size,
// else it should interrupt the program immediately.
//
// now checks if mem could be allocated, this is still
// prehistoric... there's a lot to do here: memory locking, detection
// of win95 etc...
//

// return free and total memory in the system
UINT32 I_GetFreeMem(UINT32* total)
{
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus(&info);
	if (total)
		*total = (UINT32)info.dwTotalPhys;
	return (UINT32)info.dwAvailPhys;
}

// ---------
// I_Profile
// Two little functions to profile our code using the high resolution timer
// ---------
static LARGE_INTEGER ProfileCount;
VOID I_BeginProfile(VOID)
{
	if (!QueryPerformanceCounter(&ProfileCount))
		I_Error("I_BeginProfile failed"); // can't profile without the high res timer
}

// we're supposed to use this to measure very small amounts of time,
// that's why we return a DWORD and not a 64bit value
DWORD I_EndProfile(VOID)
{
	LARGE_INTEGER CurrTime;
	DWORD ret;
	if (!QueryPerformanceCounter (&CurrTime))
		I_Error("I_EndProfile failed");
	if (CurrTime.QuadPart - ProfileCount.QuadPart > (LONGLONG)0xFFFFFFFFUL)
		I_Error("I_EndProfile overflow");
	ret = (DWORD)(CurrTime.QuadPart - ProfileCount.QuadPart);
	// we can call I_EndProfile() several time, I_BeginProfile() need be called just once
	ProfileCount = CurrTime;

	return ret;
}

// ---------
// I_GetTime
// Use the High Resolution Timer if available,
// else use the multimedia timer which has 1 millisecond precision on Windowz 95,
// but lower precision on Windows NT
// ---------
static long hacktics = 0; // used locally for keyboard repeat keys
static DWORD starttickcount = 0; // hack for win2k time bug

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
			newtics = (int)((currtime.QuadPart - basetime.QuadPart) * NEWTICRATE
				/ frequency.QuadPart);
		}
		else
		{
			currtime.LowPart = timeGetTime();
			if (!basetime.LowPart)
				basetime.LowPart = currtime.LowPart;
			newtics = ((currtime.LowPart - basetime.LowPart)/(1000/NEWTICRATE));
		}
	}
	else
		newtics = (GetTickCount() - starttickcount)/(1000/NEWTICRATE);

	hacktics = newtics; // a local counter for keyboard repeat key
	return newtics;
}

void I_Sleep(void)
{
	if (cv_sleep.value != -1)
		Sleep(cv_sleep.value);
}

// should move to i_video
void I_WaitVBL(INT32 count)
{
	UNREFERENCED_PARAMETER(count);
}

// this is probably to activate the 'loading' disc icon
// it should set a flag, that I_FinishUpdate uses to know
// whether it draws a small 'loading' disc icon on the screen or not
//
// also it should explicitly draw the disc because the screen is
// possibly not refreshed while loading
//
void I_BeginRead(void) {}

// see above, end the 'loading' disc icon, set the flag false
//
void I_EndRead(void) {}

// ===========================================================================================
//                                                                                      EVENTS
// ===========================================================================================
static BOOL I_ReadyConsole(HANDLE ci)
{
	DWORD gotinput;
	if (ci == INVALID_HANDLE_VALUE) return FALSE;
	if (WaitForSingleObject(ci,0) != WAIT_OBJECT_0) return FALSE;
	if (GetFileType(ci) != FILE_TYPE_CHAR) return FALSE;
	if (!GetConsoleMode(ci, &gotinput)) return FALSE;
	return (GetNumberOfConsoleInputEvents(ci, &gotinput) && gotinput);
}

static inline VOID I_GetConsoleEvents(VOID)
{
	event_t ev = {0,0,0,0};
	HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	INPUT_RECORD input;
	DWORD t;

	while (I_ReadyConsole(ci) && ReadConsoleInput(ci, &input, 1, &t) && t)
	{
		ZeroMemory(&ev, sizeof(ev));
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
							/* FALLTHRU */
						default:
							ev.data1 = MapVirtualKey(input.Event.KeyEvent.wVirtualKeyCode,2); // convert in to char
					}
					if (co != INVALID_HANDLE_VALUE && GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &t))
					{
						if (ev.data1 && ev.data1 != KEY_LSHIFT && ev.data1 != KEY_RSHIFT)
						{
#ifdef UNICODE
							WriteConsole(co, &input.Event.KeyEvent.uChar.UnicodeChar, 1, &t, NULL);
#else
							WriteConsole(co, &input.Event.KeyEvent.uChar.AsciiChar, 1, &t, NULL);
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

// ----------
// I_GetEvent
// Post new events for all sorts of user-input
// ----------
void I_GetEvent(void)
{
	I_GetConsoleEvents();
	I_GetKeyboardEvents();
	I_GetMouseEvents();
	I_GetJoystickEvents();
	I_GetJoystick2Events();
}

// ----------
// I_OsPolling
// ----------
void I_OsPolling(void)
{
	MSG msg;
	HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);

	// we need to dispatch messages to the window
	// so the window procedure can respond to messages and PostEvent() for keys
	// during D_SRB2Main startup.
	// this one replaces the main loop of windows since I_OsPolling is called in the main loop
	do
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else // winspec : this is quit message
				I_Quit();
		}
		if (!appActive && !netgame && !I_ReadyConsole(ci))
			WaitMessage();
	} while (!appActive && !netgame && !I_ReadyConsole(ci));

	// this is called by the network synchronization,
	// check keys and allow escaping
	I_GetEvent();

	// reset "emulated keys"
	gamekeydown[KEY_MOUSEWHEELUP] = 0;
	gamekeydown[KEY_MOUSEWHEELDOWN] = 0;
}

// ===========================================================================================
//                                                                              TIMER
// ===========================================================================================

static VOID I_ShutdownTimer(VOID)
{
	timeEndPeriod(1);
}

//
// Installs the timer interrupt handler with timer speed as TICRATE.
//
#define TIMER_ID 1
#define TIMER_RATE (1000/TICRATE)
void I_StartupTimer(void)
{
	// for win2k time bug
	if (M_CheckParm("-gettickcount"))
	{
		starttickcount = GetTickCount();
		CONS_Printf("Using GetTickCount()\n");
	}
	timeBeginPeriod(1);
	I_AddExitFunc(I_ShutdownTimer);
}

// ===========================================================================================
//                                                                   EXIT CODE, ERROR HANDLING
// ===========================================================================================

static int errorcount = 0; // phuck recursive errors
static int shutdowning = false;

//
// Used to trap various signals, to make sure things get shut down cleanly.
//
#ifdef NDEBUG
static void signal_handler(int num)
{
	//static char msg[] = "oh no! back to reality!\r\n";
	const char *sigmsg;
	char sigdef[64];

	D_QuitNetGame(); // Fix server freezes
	CL_AbortDownloadResume();
	I_ShutdownSystem();

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
			sigmsg = "software termination signal from kill";
			break;
		case SIGBREAK:
			sigmsg = "Ctrl-Break sequence";
			break;
		case SIGABRT:
			sigmsg = "abnormal termination triggered by abort call";
			break;
		default:
			sprintf(sigdef, "signal number %d", num);
			sigmsg = sigdef;
	}

#ifdef LOGMESSAGES
	if (logstream)
	{
		I_OutputMsg("signal_handler() error: %s\r\n", sigmsg);
		fclose(logstream);
		logstream = NULL;
	}
#endif

	MessageBoxA(hWndMain, va("signal_handler(): %s", sigmsg), "SRB2 error", MB_OK|MB_ICONERROR);

	signal(num, SIG_DFL); // default signal action
	raise(num);
}
#endif

//
// put an error message (with format) on stderr
//
void I_OutputMsg(const char *fmt, ...)
{
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD bytesWritten;
	va_list argptr;
	char txt[8192];

	va_start(argptr,fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

#ifdef _MSC_VER
	OutputDebugStringA(txt);
#endif

#ifdef LOGMESSAGES
	if (logstream)
	{
		fwrite(txt, strlen(txt), 1, logstream);
		fflush(logstream);
	}
#endif

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
		WriteConsoleA(co, txt, (DWORD)strlen(txt), &bytesWritten, NULL);

		// Next time, output where we left off.
		GetConsoleScreenBufferInfo(co, &csbi);
		coordNextWrite = csbi.dwCursorPosition;

		// Restore what was overwritten.
		if (oldLines && entering_con_command)
			WriteConsole(co, oldLines, oldLength, &bytesWritten, NULL);
		if (oldLines) free(oldLines);
	}
	else // Redirected to a file.
		WriteFile(co, txt, (DWORD)strlen(txt), &bytesWritten, NULL);
}

// display error messy after shutdowngfx
//
void I_Error(const char *error, ...)
{
	va_list argptr;
	char txt[8192];

	// added 11-2-98 recursive error detecting
	if (shutdowning)
	{
		errorcount++;
		// try to shutdown each subsystem separately
		if (errorcount == 5)
			I_ShutdownGraphics();
		if (errorcount == 6)
			I_ShutdownSystem();
		if (errorcount == 7)
		{
			M_SaveConfig(NULL);
			G_SaveGameData();
		}
		if (errorcount > 20)
		{
			// Don't print garbage
			va_start(argptr,error);
			vsprintf(txt, error, argptr);
			va_end(argptr);

			OutputDebugStringA(txt);
			MessageBoxA(hWndMain, txt, "SRB2 Recursive Error", MB_OK|MB_ICONERROR);
			W_Shutdown();
			exit(-1); // recursive errors detected
		}
	}
	shutdowning = true;

	// put message to stderr
	va_start(argptr, error);
	wvsprintfA(txt, error, argptr);
	va_end(argptr);

	CONS_Printf("I_Error(): %s\n", txt); //don't change from CONS_Printf.

	// saving one time is enough!
	if (!errorcount)
	{
		M_SaveConfig(NULL); // save game config, cvars..
		G_SaveGameData();
	}

	// save demo, could be useful for debug
	// NOTE: demos are normally not saved here.
	if (demorecording)
		G_CheckDemoStatus();
	if (metalrecording)
		G_StopMetalRecording(false);

	D_QuitNetGame();
	CL_AbortDownloadResume();
	M_FreePlayerSetupColors();

	// shutdown everything that was started
	I_ShutdownSystem();

#ifdef LOGMESSAGES
	if (logstream)
	{
		fclose(logstream);
		logstream = NULL;
	}
#endif

	MessageBoxA(hWndMain, txt, "SRB2 Error", MB_OK|MB_ICONERROR);

	W_Shutdown();
	exit(-1);
}

static inline VOID ShowEndTxt(HANDLE co)
{
	int i;
	UINT16 j, att = 0;
	int nlflag = 1;
	CONSOLE_SCREEN_BUFFER_INFO backupcon;
	COORD resizewin = {80,-1};
	DWORD bytesWritten;
	CHAR let = 0;
	UINT16 *ptext;
	LPVOID data;
	lumpnum_t endoomnum = W_GetNumForName("ENDOOM");
	//HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);

	/* get the lump with the text */
	data = ptext = W_CacheLumpNum(endoomnum, PU_CACHE);

	backupcon.wAttributes = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE; // Just in case
	GetConsoleScreenBufferInfo(co, &backupcon); //Store old state
	resizewin.Y = backupcon.dwSize.Y;
	if (backupcon.dwSize.X < resizewin.X)
		SetConsoleScreenBufferSize(co, resizewin);

	for (i = 1; i <= 80*25; i++) // print 80x25 text and deal with the attributes too
	{
		j = (UINT16)(*ptext >> 8); // attribute first
		let = (char)(*ptext & 0xff); // text senond
		if (j != att) // attribute changed?
		{
			att = j; // save current attribute
			SetConsoleTextAttribute(co, j); //set fg and bg color for buffer
		}

		WriteConsoleA(co, &let, 1, &bytesWritten, NULL); // now the text

		if (nlflag && !(i % 80) && backupcon.dwSize.X > resizewin.X) // do we need a nl?
		{
			att = backupcon.wAttributes;
			SetConsoleTextAttribute(co, att); // all attributes off
			WriteConsoleA(co, "\n", 1, &bytesWritten, NULL); // newline to console
		}
		ptext++;
	}
	SetConsoleTextAttribute(co, backupcon.wAttributes); // all attributes off
	//if (nlflag)
	//	WriteConsoleA(co, "\n", 1, &bytesWritten, NULL);

	getchar(); //pause!

	Z_Free(data);
}


//
// I_Quit: shutdown everything cleanly, in reverse order of Startup.
//
void I_Quit(void)
{
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode;
	// when recording a demo, should exit using 'q',
	// but sometimes we forget and use Alt+F4, so save here too.
	if (demorecording)
		G_CheckDemoStatus();
	if (metalrecording)
		G_StopMetalRecording(false);

	M_SaveConfig(NULL); // save game config, cvars..
#ifndef NONET
	D_SaveBan(); // save the ban list
#endif
	G_SaveGameData();

	// maybe it needs that the ticcount continues,
	// or something else that will be finished by I_ShutdownSystem(),
	// so do it before.
	D_QuitNetGame();
	CL_AbortDownloadResume();

	M_FreePlayerSetupColors();

	// shutdown everything that was started
	I_ShutdownSystem();

	if (shutdowning || errorcount)
		I_Error("Error detected (%d)", errorcount);

#ifdef LOGMESSAGES
	if (logstream)
	{
		I_OutputMsg("I_Quit(): end of logstream.\n");
		fclose(logstream);
		logstream = NULL;
	}
#endif
	if (!M_CheckParm("-noendtxt") && W_CheckNumForName("ENDOOM")!=LUMPERROR
		&& co != INVALID_HANDLE_VALUE && GetFileType(co) == FILE_TYPE_CHAR
		&& GetConsoleMode(co, &mode))
	{
		printf("\r");
		ShowEndTxt(co);
	}
	fflush(stderr);
	if (myargmalloc)
		free(myargv); // Deallocate allocated memory
	W_Shutdown();
	exit(0);
}

// --------------------------------------------------------------------------
// I_ShowLastError
// Print a GetLastError() error message, and if MB is TRUE, also display it in a MessageBox
// --------------------------------------------------------------------------
VOID I_ShowLastError(BOOL MB)
{
	LPSTR lpMsgBuf = NULL;
	const DWORD LE = GetLastError();

	if (LE == 0xdeadbeef) return; // Not a real error

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, LE, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPVOID)&lpMsgBuf, 0, NULL);

	if (!lpMsgBuf)
	{
		I_OutputMsg("GetLastError: Unknown\n");
		return;
	}

	// Display the string
	if (MB && LE) MessageBoxA(NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION);

	// put it in text console and log if any
	I_OutputMsg("GetLastError: %s", lpMsgBuf);

	// Free the buffer.
	LocalFree(lpMsgBuf);
}

// ===========================================================================================
// CLEAN STARTUP & SHUTDOWN HANDLING, JUST CLOSE EVERYTHING YOU OPENED.
// ===========================================================================================
//
//
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

// Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	int c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}

// Removes a function from the list that need to be called by I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	int c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c < MAX_QUIT_FUNCS - 1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

// ===========================================================================================
// DIRECT INPUT HELPER CODE
// ===========================================================================================

// Create a DirectInputDevice interface,
// create a DirectInputDevice2 interface if possible
static VOID CreateDevice2A(LPDIRECTINPUTA di, REFGUID pguid, LPDIRECTINPUTDEVICEA* lpDEV,
                          LPDIRECTINPUTDEVICE2A* lpDEV2)
{
	HRESULT hr, hr2;
	LPDIRECTINPUTDEVICEA lpdid1;
	LPDIRECTINPUTDEVICE2A lpdid2 = NULL;

	hr = IDirectInput_CreateDevice(di, pguid, &lpdid1, NULL);

	if (SUCCEEDED(hr))
	{
		// get Device2 but only if we are not in DirectInput version 3
		if (!bDX0300 && lpDEV2)
		{
			LPDIRECTINPUTDEVICE2A *rp = &lpdid2;
			LPVOID *tp  = (LPVOID *)rp;
			hr2 = IDirectInputDevice_QueryInterface(lpdid1, &IID_IDirectInputDevice2, tp);
			if (FAILED(hr2))
			{
				CONS_Alert(CONS_ERROR, M_GetText("Could not create IDirectInput device 2"));
				lpdid2 = NULL;
			}
		}
	}
	else
		I_Error("Could not create IDirectInput device");

	*lpDEV = lpdid1;
	if (lpDEV2) // only if we requested it
		*lpDEV2 = lpdid2;
}

// ===========================================================================================
//                                                                          DIRECT INPUT MOUSE
// ===========================================================================================

#define DI_MOUSE_BUFFERSIZE 16 // number of data elements in mouse buffer

//
// Initialise the mouse.
//
static void I_ShutdownMouse(VOID);

void I_StartupMouse(VOID)
{
	// this gets called when cv_usemouse is initted
	// for the win32 version, we want to startup the mouse later
	if (menuactive)
	{
		if (cv_usemouse.value)
			I_DoStartupMouse();
		else
			I_ShutdownMouse();
	}
}

static HANDLE mouse2filehandle = INVALID_HANDLE_VALUE;

static void I_ShutdownMouse2(VOID)
{
	if (mouse2filehandle != INVALID_HANDLE_VALUE)
	{
		event_t event;
		UINT i;

		SetCommMask(mouse2filehandle, 0);

		EscapeCommFunction(mouse2filehandle, CLRDTR);
		EscapeCommFunction(mouse2filehandle, CLRRTS);

		PurgeComm(mouse2filehandle, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

		CloseHandle(mouse2filehandle);

		// emulate the up of all mouse buttons
		for (i = 0; i < MOUSEBUTTONS; i++)
		{
			event.type = ev_keyup;
			event.data1 = KEY_2MOUSE1 + i;
			D_PostEvent(&event);
		}

		mouse2filehandle = INVALID_HANDLE_VALUE;
	}
}

#define MOUSECOMBUFFERSIZE 256
static int handlermouse2x, handlermouse2y, handlermouse2buttons;

static VOID I_PoolMouse2(VOID)
{
	BYTE buffer[MOUSECOMBUFFERSIZE];
	COMSTAT ComStat;
	DWORD dwErrorFlags, dwLength, i;
	CHAR dx, dy;
	static BYTE bytenum, combytes[4];

	ClearCommError(mouse2filehandle, &dwErrorFlags, &ComStat);
	dwLength = min(MOUSECOMBUFFERSIZE, ComStat.cbInQue);

	if (dwLength > 0)
	{
		if (!ReadFile(mouse2filehandle, buffer, dwLength, &dwLength, NULL))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Read Error on secondary mouse port\n"));
			return;
		}

		// parse the mouse packets
		for (i = 0; i < dwLength; i++)
		{
			if ((buffer[i] & 64) == 64)
				bytenum = 0;

			if (bytenum < 4)
				combytes[bytenum] = buffer[i];
			bytenum++;

			if (bytenum == 1)
			{
				handlermouse2buttons &= ~3;
				handlermouse2buttons |= ((combytes[0] & (32+16)) >>4);
			}
			else if (bytenum == 3)
			{
				dx = (CHAR)((combytes[0] &  3) << 6);
				dy = (CHAR)((combytes[0] & 12) << 4);
				dx = (CHAR)(dx + combytes[1]);
				dy = (CHAR)(dy + combytes[2]);
				handlermouse2x += dx;
				handlermouse2y += dy;
			}
			else if (bytenum == 4) // fourth byte (logitech mouses)
			{
				if (buffer[i] & 32)
					handlermouse2buttons |= 4;
				else
					handlermouse2buttons &= ~4;
			}
		}
	}
}

// secondary mouse doesn't use DirectX, therefore forget all about grabbing, acquire, etc.
void I_StartupMouse2(void)
{
	DCB dcb;

	if (mouse2filehandle != INVALID_HANDLE_VALUE)
		I_ShutdownMouse2();

	if (!cv_usemouse2.value)
		return;

	if (mouse2filehandle != INVALID_HANDLE_VALUE)
	{
		// COM file handle
		mouse2filehandle = CreateFileA(cv_mouse2port.string, GENERIC_READ|GENERIC_WRITE,
		                               0, // exclusive access
		                               NULL, // no security attrs
		                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (mouse2filehandle == INVALID_HANDLE_VALUE)
		{
			int e = GetLastError();
			if (e == 5)
				CONS_Alert(CONS_ERROR, M_GetText("Error opening %s!\n"), cv_mouse2port.string);
			else
				CONS_Alert(CONS_ERROR, M_GetText("Can't open %s: error %d\n"), cv_mouse2port.string, e);
			return;
		}
	}

	// buffers
	SetupComm(mouse2filehandle, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);

	// purge buffers
	PurgeComm(mouse2filehandle, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

	// setup port to 1200 7N1
	dcb.DCBlength = sizeof (DCB);

	GetCommState(mouse2filehandle, &dcb);

	dcb.BaudRate = CBR_1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.fBinary = dcb.fParity = TRUE;

	SetCommState(mouse2filehandle, &dcb);

	I_AddExitFunc(I_ShutdownMouse2);
}

#define MAX_MOUSE_BTNS 5
static int center_x, center_y;
static INT old_mparms[3], new_mparms[3] = {0, 0, 1};
static BOOL restore_mouse = FALSE;
static INT old_mouse_state = 0;
UINT MSHWheelMessage = 0;

static VOID I_DoStartupSysMouse(VOID)
{
	boolean valid;
	RECT w_rect;

	valid = SystemParametersInfo(SPI_GETMOUSE, 0, old_mparms, 0);
	if (valid)
	{
		new_mparms[2] = old_mparms[2];
		restore_mouse = SystemParametersInfo(SPI_SETMOUSE, 0, new_mparms, 0);
	}

	if (bAppFullScreen)
	{
		w_rect.top = 0;
		w_rect.left = 0;
	}
	else
	{
		w_rect.top = windowPosY;
		w_rect.left = windowPosX;
	}

	w_rect.bottom = w_rect.top + VIDHEIGHT;
	w_rect.right = w_rect.left + VIDWIDTH;
	center_x = w_rect.left + (VIDWIDTH >> 1);
	center_y = w_rect.top + (VIDHEIGHT >> 1);
	SetCursor(NULL);
	SetCursorPos(center_x, center_y);
	SetCapture(hWndMain);
	ClipCursor(&w_rect);
}

static VOID I_ShutdownSysMouse(VOID)
{
	if (restore_mouse)
		SystemParametersInfo(SPI_SETMOUSE, 0, old_mparms, 0);
	ClipCursor(NULL);
	ReleaseCapture();
}

VOID I_RestartSysMouse(VOID)
{
	if (nodinput)
	{
		I_ShutdownSysMouse();
		I_DoStartupSysMouse();
	}
}

VOID I_GetSysMouseEvents(INT mouse_state)
{
	UINT i;
	event_t event;
	int xmickeys = 0, ymickeys = 0;
	POINT c_pos;

	for (i = 0; i < MAX_MOUSE_BTNS; i++)
	{
		// check if button pressed
		if ((mouse_state & (1 << i)) && !(old_mouse_state & (1 << i)))
		{
			event.type = ev_keydown;
			event.data1 = KEY_MOUSE1 + i;
			D_PostEvent(&event);
		}
		// check if button released
		if (!(mouse_state & (1 << i)) && (old_mouse_state & (1 << i)))
		{
			event.type = ev_keyup;
			event.data1 = KEY_MOUSE1 + i;
			D_PostEvent(&event);
		}
	}
	old_mouse_state = mouse_state;

	// proceed mouse movements
	GetCursorPos(&c_pos);
	xmickeys = c_pos.x - center_x;
	ymickeys = c_pos.y - center_y;

	if (xmickeys || ymickeys)
	{
		event.type  = ev_mouse;
		event.data1 = 0;
		event.data2 = xmickeys;
		event.data3 = -ymickeys;
		D_PostEvent(&event);
		SetCursorPos(center_x, center_y);
	}
}

// This is called just before entering the main game loop,
// when we are going fullscreen and the loading screen has finished.
VOID I_DoStartupMouse(VOID)
{
	DIPROPDWORD dip;

	// mouse detection may be skipped by setting usemouse false
	if (!cv_usemouse.value || M_CheckParm("-nomouse"))
	{
		mouse_enabled = false;
		return;
	}

	if (nodinput)
	{
		CONS_Printf(M_GetText("\tMouse will not use DirectInput.\n"));
		// System mouse input will be initiated by VID_SetMode
		I_AddExitFunc(I_ShutdownMouse);

		MSHWheelMessage = RegisterWindowMessage(MSH_MOUSEWHEEL);
	}
	else if (!lpDIM) // acquire the mouse only once
	{
		CreateDevice2A(lpDI, &GUID_SysMouse, &lpDIM, NULL);

		if (lpDIM)
		{
			if (FAILED(IDirectInputDevice_SetDataFormat(lpDIM, &c_dfDIMouse)))
				I_Error("Couldn't set mouse data format");

			// create buffer for buffered data
			dip.diph.dwSize = sizeof (dip);
			dip.diph.dwHeaderSize = sizeof (dip.diph);
			dip.diph.dwObj = 0;
			dip.diph.dwHow = DIPH_DEVICE;
			dip.dwData = DI_MOUSE_BUFFERSIZE;
			if (FAILED(IDirectInputDevice_SetProperty(lpDIM, DIPROP_BUFFERSIZE, &dip.diph)))
				I_Error("Couldn't set mouse buffer size");

			if (FAILED(IDirectInputDevice_SetCooperativeLevel(lpDIM, hWndMain,
				DISCL_EXCLUSIVE|DISCL_FOREGROUND)))
			{
				I_Error("Couldn't set mouse coop level");
			}
			I_AddExitFunc(I_ShutdownMouse);
		}
		else
			I_Error("Couldn't create mouse input");
	}

	// if re-enabled while running, just set mouse_enabled true again,
	// do not acquire the mouse more than once
	mouse_enabled = true;
}

//
// Shutdown Mouse DirectInput device
//
static void I_ShutdownMouse(void)
{
	int i;
	event_t event;

	CONS_Printf("I_ShutdownMouse()\n");

	if (lpDIM)
	{
		IDirectInputDevice_Unacquire(lpDIM);
		IDirectInputDevice_Release(lpDIM);
		lpDIM = NULL;
	}

	// emulate the up of all mouse buttons
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		event.type = ev_keyup;
		event.data1 = KEY_MOUSE1 + i;
		D_PostEvent(&event);
	}
	if (nodinput)
		I_ShutdownSysMouse();

	mouse_enabled = false;
}

//
// Get buffered data from the mouse
//
void I_GetMouseEvents(void)
{
	DIDEVICEOBJECTDATA rgdod[DI_MOUSE_BUFFERSIZE];
	DWORD dwItems, d;
	HRESULT hr;

	event_t event;
	int xmickeys, ymickeys;

	if (mouse2filehandle != INVALID_HANDLE_VALUE)
	{
		//mouse movement
		static UINT8 lastbuttons2 = 0;

		I_PoolMouse2();
		// post key event for buttons
		if (handlermouse2buttons != lastbuttons2)
		{
			int i, j = 1, k;
			k = handlermouse2buttons ^ lastbuttons2; // only changed bit to 1
			lastbuttons2 = (UINT8)handlermouse2buttons;

			for (i = 0; i < MOUSEBUTTONS; i++, j <<= 1)
				if (k & j)
				{
					if (handlermouse2buttons & j)
						event.type = ev_keydown;
					else
						event.type = ev_keyup;
					event.data1 = KEY_2MOUSE1 + i;
					D_PostEvent(&event);
				}
		}

		if (handlermouse2x || handlermouse2y)
		{
			event.type = ev_mouse2;
			event.data1 = 0;
			event.data2 = handlermouse2x<<1;
			event.data3 = -handlermouse2y<<1;
			handlermouse2x = 0;
			handlermouse2y = 0;

			D_PostEvent(&event);
		}
	}

	if (!mouse_enabled || nodinput)
		return;

getBufferedData:
	dwItems = DI_MOUSE_BUFFERSIZE;
	hr = IDirectInputDevice_GetDeviceData(lpDIM, sizeof (DIDEVICEOBJECTDATA), rgdod, &dwItems, 0);

	// If data stream was interrupted, reacquire the device and try again.
	if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
	{
		hr = IDirectInputDevice_Acquire(lpDIM);
		if (SUCCEEDED(hr))
			goto getBufferedData;
	}

	// We got buffered input, act on it
	if (SUCCEEDED(hr))
	{
		xmickeys = ymickeys = 0;

		// dwItems contains number of elements read (could be 0)
		for (d = 0; d < dwItems; d++)
		{
			if (rgdod[d].dwOfs >= DIMOFS_BUTTON0 &&
				rgdod[d].dwOfs <  DIMOFS_BUTTON0 + MOUSEBUTTONS)
			{
				if (rgdod[d].dwData & 0x80) // Button down
					event.type = ev_keydown;
				else
					event.type = ev_keyup; // Button up

				event.data1 = rgdod[d].dwOfs - DIMOFS_BUTTON0 + KEY_MOUSE1;
				D_PostEvent(&event);
			}
			else if (rgdod[d].dwOfs == DIMOFS_X)
				xmickeys += rgdod[d].dwData;
			else if (rgdod[d].dwOfs == DIMOFS_Y)
				ymickeys += rgdod[d].dwData;

			else if (rgdod[d].dwOfs == DIMOFS_Z)
			{
				// z-axes the wheel
				if ((int)rgdod[d].dwData > 0)
					event.data1 = KEY_MOUSEWHEELUP;
				else
					event.data1 = KEY_MOUSEWHEELDOWN;
				event.type = ev_keydown;
				D_PostEvent(&event);
			}

		}

		if (xmickeys || ymickeys)
		{
			event.type = ev_mouse;
			event.data1 = 0;
			event.data2 = xmickeys;
			event.data3 = -ymickeys;
			D_PostEvent(&event);
		}
	}
}

void I_UpdateMouseGrab(void) {}

// ===========================================================================================
//                                                                       DIRECT INPUT JOYSTICK
// ===========================================================================================

struct DIJoyInfo_s
{
	BYTE X,Y,Z,Rx,Ry,Rz,U,V;
	LONG ForceAxises;
};
typedef struct DIJoyInfo_s DIJoyInfo_t;

// private info
	static BYTE iJoyNum;        // used by enumeration
	static DIJoyInfo_t JoyInfo;
	static BYTE iJoy2Num;
	static DIJoyInfo_t JoyInfo2;

//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//       each force feedback enabled axis
//-----------------------------------------------------------------------------
static BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCEA* pdidoi,
                                VOID* pContext)
{
	DWORD* pdwNumForceFeedbackAxis = (DWORD*) pContext;

	if ((pdidoi->dwFlags & DIDOI_FFACTUATOR) != 0)
		(*pdwNumForceFeedbackAxis)++;

	return DIENUM_CONTINUE;
}


static HRESULT SetupForceTacile(LPDIRECTINPUTDEVICE2A DJI, LPDIRECTINPUTEFFECT *DJE, DWORD FFAXIS, FFType EffectType,REFGUID EffectGUID)
{
	HRESULT hr;
	DIEFFECT eff;
	DWORD rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
	LONG rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf = { 0 }; // LONG lMagnitude
	DIRAMPFORCE rf = {0,0}; // LONG lStart, lEnd;
	DIPERIODIC pf = {0,0,0,0};
	ZeroMemory(&eff, sizeof (eff));
	if (FFAXIS > 2)
		FFAXIS = 2; //up to 2 FFAXIS
	eff.dwSize                  = sizeof (DIEFFECT);
	eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS; // Cartesian and data format offsets
	eff.dwDuration              = INFINITE;
	eff.dwSamplePeriod          = 0;
	eff.dwGain                  = DI_FFNOMINALMAX;
	eff.dwTriggerButton         = DIEB_NOTRIGGER;
	eff.dwTriggerRepeatInterval = 0;
	eff.cAxes                   = FFAXIS;
	eff.rgdwAxes                = rgdwAxes;
	eff.rglDirection            = rglDirection;
	eff.lpEnvelope              = NULL;
	eff.lpvTypeSpecificParams   = NULL;
	if (EffectType == ConstantForce)
	{
		eff.cbTypeSpecificParams    = sizeof (cf);
		eff.lpvTypeSpecificParams   = &cf;
	}
	else if (EffectType == RampForce)
	{
		eff.cbTypeSpecificParams    = sizeof (rf);
		eff.lpvTypeSpecificParams   = &rf;
	}
	else if (EffectType >= SquareForce && SawtoothDownForce >= EffectType)
	{
		eff.cbTypeSpecificParams    = sizeof (pf);
		eff.lpvTypeSpecificParams   = &pf;
	}

	// Create the prepared effect
	if (FAILED(hr = IDirectInputDevice2_CreateEffect(DJI, EffectGUID,
	 &eff, DJE, NULL)))
	{
		return hr;
	}

	if (NULL == *DJE)
		return E_FAIL;

	return hr;
}

static BOOL CALLBACK DIEnumEffectsCallback1(LPCDIEFFECTINFOA pdei, LPVOID pvRef)
{
	LPDIRECTINPUTEFFECT *DJE = pvRef;
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_CONSTANTFORCE)
	{
		if (SUCCEEDED(SetupForceTacile(lpDIJA,DJE, JoyInfo.ForceAxises, ConstantForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_RAMPFORCE)
	{
		if (SUCCEEDED(SetupForceTacile(lpDIJA,DJE, JoyInfo.ForceAxises, RampForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIEnumEffectsCallback2(LPCDIEFFECTINFOA pdei, LPVOID pvRef)
{
	LPDIRECTINPUTEFFECT *DJE = pvRef;
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_CONSTANTFORCE)
	{
		if (SUCCEEDED(SetupForceTacile(lpDIJ2A,DJE, JoyInfo2.ForceAxises, ConstantForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	if (DIEFT_GETTYPE(pdei->dwEffType) == DIEFT_RAMPFORCE)
	{
		if (SUCCEEDED(SetupForceTacile(lpDIJ2A,DJE, JoyInfo2.ForceAxises, RampForce, &pdei->guid)))
			return DIENUM_STOP;
	}
	return DIENUM_CONTINUE;
}

static REFGUID DIETable[] =
{
	&GUID_ConstantForce, //ConstantForce
	&GUID_RampForce,     //RampForce
	&GUID_Square,        //SquareForce
	&GUID_Sine,          //SineForce
	&GUID_Triangle,      //TriangleForce
	&GUID_SawtoothUp,    //SawtoothUpForce
	&GUID_SawtoothDown,  //SawtoothDownForce
	(REFGUID)-1,         //NumberofForces
};

static HRESULT SetupAllForces(LPDIRECTINPUTDEVICE2A DJI, LPDIRECTINPUTEFFECT DJE[], DWORD FFAXIS)
{
	FFType ForceType = EvilForce;
	if (DJI == lpDIJA)
	{
		IDirectInputDevice2_EnumEffects(DJI,DIEnumEffectsCallback1,&DJE[ConstantForce],DIEFT_CONSTANTFORCE);
		IDirectInputDevice2_EnumEffects(DJI,DIEnumEffectsCallback1,&DJE[RampForce],DIEFT_RAMPFORCE);
	}
	else if (DJI == lpDIJA)
	{
		IDirectInputDevice2_EnumEffects(DJI,DIEnumEffectsCallback2,&DJE[ConstantForce],DIEFT_CONSTANTFORCE);
		IDirectInputDevice2_EnumEffects(DJI,DIEnumEffectsCallback2,&DJE[RampForce],DIEFT_RAMPFORCE);
	}
	for (ForceType = SquareForce; ForceType >  NumberofForces && DIETable[ForceType] != (REFGUID)-1; ForceType++)
		if (DIETable[ForceType])
			SetupForceTacile(DJI,&DJE[ForceType], FFAXIS, ForceType, DIETable[ForceType]);
	return S_OK;
}

static inline VOID LimitEffect(LPDIEFFECT eff, FFType EffectType)
{
	LPDICONSTANTFORCE pCF = eff->lpvTypeSpecificParams;
	LPDIPERIODIC pDP= eff->lpvTypeSpecificParams;
	if (eff->rglDirection)
	{
	}
/*	if (eff->dwDuration != INFINITE && eff->dwDuration < 0)
	{
		eff->dwDuration = 0;
	}*/
	if (eff->dwGain != 0)
	{
		if (eff->dwGain > DI_FFNOMINALMAX)
			eff->dwGain = DI_FFNOMINALMAX;
		//else if (eff->dwGain < -DI_FFNOMINALMAX)
		//	eff->dwGain = DI_FFNOMINALMAX;
	}
	if (EffectType == ConstantForce && pCF->lMagnitude)
	{
	}
	else if (EffectType >= SquareForce && SawtoothDownForce >= EffectType && pDP)
	{
	}

}

static HRESULT SetForceTacile(LPDIRECTINPUTEFFECT SDIE, const JoyFF_t *FF,DWORD FFAXIS, FFType EffectType)
{
	DIEFFECT eff;
	HRESULT hr;
	LONG Magnitude;
	LONG rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf = { 0 }; // LONG lMagnitude
	DIRAMPFORCE rf = {0,0}; // LONG lStart, lEnd;
	DIPERIODIC pf = {0,0,0,0};
	if (!FF)
		IDirectInputEffect_Stop(SDIE);
	Magnitude = FF->Magnitude;
	ZeroMemory(&eff, sizeof (eff));
	eff.dwSize                  = sizeof (eff);
	//DIEP_START
	eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS; // Cartesian and data format offsets
	//DIEP_DURATION
	eff.dwDuration              = FF->Duration;
	//DIEP_GAIN
	eff.dwGain                  = FF->Gain;
	//DIEP_DIRECTION
	eff.rglDirection            = rglDirection;
	//DIEP_TYPESPECIFICPARAMS
	if (FFAXIS > 1)
	{
		double dMagnitude;
		dMagnitude                  = (double)Magnitude;
		dMagnitude                  = hypot(dMagnitude, dMagnitude);
		Magnitude                   = (DWORD)dMagnitude;
		rglDirection[0]             = FF->ForceX;
		rglDirection[1]             = FF->ForceY;
	}
	if (EffectType == ConstantForce)
	{
		cf.lMagnitude               = Magnitude;
		eff.cbTypeSpecificParams    = sizeof (cf);
		eff.lpvTypeSpecificParams   = &cf;
	}
	else if (EffectType == RampForce)
	{
		rf.lStart                   = FF->Start;
		rf.lEnd                     = FF->End;
		eff.cbTypeSpecificParams    = sizeof (rf);
		eff.lpvTypeSpecificParams   = &rf;
	}
	else if (EffectType >= SquareForce && SawtoothDownForce >= EffectType)
	{
		pf.dwMagnitude              = Magnitude;
		pf.lOffset                  = FF->Offset;
		pf.dwPhase                  = FF->Phase;
		pf.dwPeriod                 = FF->Period;
		eff.cbTypeSpecificParams    = sizeof (pf);
		eff.lpvTypeSpecificParams   = &pf;
	}

	LimitEffect(&eff, EffectType);

	hr = IDirectInputEffect_SetParameters(SDIE, &eff,
	 DIEP_START|DIEP_DURATION|DIEP_GAIN|DIEP_DIRECTION|DIEP_TYPESPECIFICPARAMS);
	return hr;
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	if (!lpDIJA) return;
	if (FAILED(IDirectInputDevice2_Acquire(lpDIJA)))
		return;
	if (Type == EvilForce)
		IDirectInputDevice2_SendForceFeedbackCommand(lpDIJA,DISFFC_STOPALL);
	if (Type <= EvilForce || Type > NumberofForces || !lpDIE[Type])
		return;
	SetForceTacile(lpDIE[Type], Effect, JoyInfo.ForceAxises, Type);
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	if (!lpDIJ2A) return;
	if (FAILED(IDirectInputDevice2_Acquire(lpDIJ2A)))
		return;
	if (Type == EvilForce)
		IDirectInputDevice2_SendForceFeedbackCommand(lpDIJ2A,DISFFC_STOPALL);
	if (Type <= EvilForce || Type > NumberofForces || !lpDIE2[Type])
		return;
	SetForceTacile(lpDIE2[Type],Effect, JoyInfo2.ForceAxises, Type);
}

// ------------------
// SetDIDwordProperty (HELPER)
// Set a DWORD property on a DirectInputDevice.
// ------------------
static HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICEA pdev,
                                   REFGUID guidProperty,
                                   DWORD dwObject,
                                   DWORD dwHow,
                                   DWORD dwValue)
{
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize       = sizeof (dipdw);
	dipdw.diph.dwHeaderSize = sizeof (dipdw.diph);
	dipdw.diph.dwObj        = dwObject;
	dipdw.diph.dwHow        = dwHow;
	dipdw.dwData            = dwValue;

	return IDirectInputDevice_SetProperty(pdev, guidProperty, &dipdw.diph);
}

#define DIDEADZONE 0000 //2500

// ---------------
// DIEnumJoysticks
// There is no such thing as a 'system' joystick, contrary to mouse,
// we must enumerate and choose one joystick device to use
// ---------------
static BOOL CALLBACK DIEnumJoysticks (LPCDIDEVICEINSTANCEA lpddi,
                                       LPVOID pvRef)   //cv_usejoystick
{
	LPDIRECTINPUTDEVICEA pdev;
	DIPROPRANGE          diprg;
	DIDEVCAPS            caps;
	BOOL                 bUseThisOne = FALSE;

	iJoyNum++;

	//faB: if cv holds a string description of joystick, the value from atoi() is 0
	//     else, the value was probably set by user at console to one of the previously
	//     enumerated joysticks
	if (((consvar_t *)pvRef)->value == iJoyNum || !strcmp(((consvar_t *)pvRef)->string, lpddi->tszProductName))
		bUseThisOne = TRUE;

	//I_OutputMsg(" cv joy is %s\n", ((consvar_t *)pvRef)->string);

	// print out device name
	CONS_Printf("%c%d: %s\n",
	            (bUseThisOne) ? '\2' : ' ',   // show name in white if this is the one we will use
	            iJoyNum,
	            //(GET_DIDEVICE_SUBTYPE(lpddi->dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD) ? "Gamepad " : "Joystick",
	            lpddi->tszProductName); //, lpddi->tszInstanceName);

	// use specified joystick (cv_usejoystick.value in pvRef)
	if (!bUseThisOne)
		return DIENUM_CONTINUE;

	((consvar_t *)pvRef)->value = iJoyNum;
	if (IDirectInput_CreateDevice(lpDI, &lpddi->guidInstance,
	                              &pdev, NULL) != DI_OK)
	{
		// if it failed, then we can't use this joystick for some
		// bizarre reason.  (Maybe the user unplugged it while we
		// were in the middle of enumerating it.)  So continue enumerating
		I_OutputMsg("DIEnumJoysticks(): CreateDevice FAILED\n");
		return DIENUM_CONTINUE;
	}

	// get the Device capabilities
	//
	caps.dwSize = sizeof (DIDEVCAPS_DX3);
	if (FAILED(IDirectInputDevice_GetCapabilities (pdev, &caps)))
	{
		I_OutputMsg("DIEnumJoysticks(): GetCapabilities FAILED\n");
		IDirectInputDevice_Release (pdev);
		return DIENUM_CONTINUE;
	}
	if (!(caps.dwFlags & DIDC_ATTACHED))   // should be, since we enumerate only attached devices
		return DIENUM_CONTINUE;

	Joystick.bJoyNeedPoll = ((caps.dwFlags & DIDC_POLLEDDATAFORMAT) != 0);

	if (caps.dwFlags & DIDC_FORCEFEEDBACK)
		JoyInfo.ForceAxises = 0;
	else
		JoyInfo.ForceAxises = -1;

	Joystick.bGamepadStyle = (GET_DIDEVICE_SUBTYPE(caps.dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD);
	//I_OutputMsg("Gamepad: %d\n", Joystick.bGamepadStyle);


	CONS_Printf(M_GetText("Capabilities: %lu axes, %lu buttons, %lu POVs, poll %u, Gamepad %d\n"), caps.dwAxes, caps.dwButtons, caps.dwPOVs, Joystick.bJoyNeedPoll, Joystick.bGamepadStyle);

	// Set the data format to "simple joystick" - a predefined data format
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing a
	// DIJOYSTATE structure to IDirectInputDevice::GetDeviceState.
	if (IDirectInputDevice_SetDataFormat (pdev, &c_dfDIJoystick) != DI_OK)
	{
		I_OutputMsg("DIEnumJoysticks(): SetDataFormat FAILED\n");
		IDirectInputDevice_Release (pdev);
		return DIENUM_CONTINUE;
	}

	// Set the cooperativity level to let DirectInput know how
	// this device should interact with the system and with other
	// DirectInput applications.
	if (IDirectInputDevice_SetCooperativeLevel (pdev, hWndMain,
	 DISCL_EXCLUSIVE | DISCL_FOREGROUND) != DI_OK)
	{
		I_OutputMsg("DIEnumJoysticks(): SetCooperativeLevel FAILED\n");
		IDirectInputDevice_Release (pdev);
		return DIENUM_CONTINUE;
	}

	// set the range of the joystick axis
	diprg.diph.dwSize       = sizeof (DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof (DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -JOYAXISRANGE;    // value for extreme left
	diprg.lMax              = +JOYAXISRANGE;    // value for extreme right

	diprg.diph.dwObj = DIJOFS_X;    // set the x-axis range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//goto SetPropFail;
		JoyInfo.X = FALSE;
	}
	else JoyInfo.X = TRUE;

	diprg.diph.dwObj = DIJOFS_Y;    // set the y-axis range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
//SetPropFail:
//		I_OutputMsg("DIEnumJoysticks(): SetProperty FAILED\n");
//		IDirectInputDevice_Release (pdev);
//		return DIENUM_CONTINUE;
		JoyInfo.Y = FALSE;
	}
	else JoyInfo.Y = TRUE;

	diprg.diph.dwObj = DIJOFS_Z;    // set the z-axis range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_Z not found\n");
		JoyInfo.Z = FALSE;
	}
	else JoyInfo.Z = TRUE;

	diprg.diph.dwObj = DIJOFS_RX;   // set the x-rudder range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RX (x-rudder) not found\n");
		JoyInfo.Rx = FALSE;
	}
	else JoyInfo.Rx = TRUE;

	diprg.diph.dwObj = DIJOFS_RY;   // set the y-rudder range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RY (y-rudder) not found\n");
		JoyInfo.Ry = FALSE;
	}
	else JoyInfo.Ry = TRUE;

	diprg.diph.dwObj = DIJOFS_RZ;   // set the z-rudder range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RZ (z-rudder) not found\n");
		JoyInfo.Rz = FALSE;
	}
	else JoyInfo.Rz = TRUE;
	diprg.diph.dwObj = DIJOFS_SLIDER(0);   // set the x-misc range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RZ (x-misc) not found\n");
		JoyInfo.U = FALSE;
	}
	else JoyInfo.U = TRUE;

	diprg.diph.dwObj = DIJOFS_SLIDER(1);   // set the y-misc range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RZ (y-misc) not found\n");
		JoyInfo.V = FALSE;
	}
	else JoyInfo.V = TRUE;

	// set X axis dead zone to 25% (to avoid accidental turning)
	if (!Joystick.bGamepadStyle)
	{
		if (JoyInfo.X)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_X,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for X DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.Y)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_Y,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for Y DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.Z)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_Z,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for Z DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.Rx)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_RX,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for RX DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.Ry)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_RY,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for RY DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.Rz)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_RZ,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for RZ DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.U)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(0),
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for U DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo.V)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(1),
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks(): couldn't SetProperty for V DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
	}

	// query for IDirectInputDevice2 - we need this to poll the joystick
	if (bDX0300)
	{
		FFType i = EvilForce;
		// we won't use the poll
		lpDIJA = NULL;
		for (i = 0; i > NumberofForces; i++)
			lpDIE[i] = NULL;
	}
	else
	{
		LPDIRECTINPUTDEVICE2A *rp = &lpDIJA;
		LPVOID *tp  = (LPVOID *)rp;
		if (FAILED(IDirectInputDevice_QueryInterface(pdev, &IID_IDirectInputDevice2, tp)))
		{
			I_OutputMsg("DIEnumJoysticks(): QueryInterface FAILED\n");
			IDirectInputDevice_Release (pdev);
			return DIENUM_CONTINUE;
		}

		if (lpDIJA && JoyInfo.ForceAxises != -1)
		{
			// Since we will be playing force feedback effects, we should disable the
			// auto-centering spring.
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_AUTOCENTER, 0, DIPH_DEVICE, FALSE)))
			{
				//NOP
			}

			// Enumerate and count the axes of the joystick
			if (FAILED(IDirectInputDevice_EnumObjects(pdev, EnumAxesCallback,
				(LPVOID)&JoyInfo.ForceAxises, DIDFT_AXIS)))
			{
				JoyInfo.ForceAxises = -1;
			}
			else
			{
				SetupAllForces(lpDIJA,lpDIE,JoyInfo.ForceAxises);
			}
		}
	}

	// we successfully created an IDirectInputDevice.  So stop looking
	// for another one.
	lpDIJ = pdev;
	return DIENUM_STOP;
}

// --------------
// I_InitJoystick
// This is called everytime the 'use_joystick' variable changes
// It is normally called at least once at startup when the config is loaded
// --------------
void I_InitJoystick(void)
{
	HRESULT hr;

	// cleanup
	I_ShutdownJoystick();

	//joystick detection can be skipped by setting use_joystick to 0
	if (!lpDI || M_CheckParm("-nojoy"))
	{
		CONS_Printf(M_GetText("Joystick disabled\n"));
		return;
	}
	else
		// don't do anything at the registration of the joystick cvar,
		// until config is loaded
		if (!strcmp(cv_usejoystick.string, "0"))
			return;

	// acquire the joystick only once
	if (!lpDIJ)
	{
		joystick_detected = false;

		CONS_Printf(M_GetText("Looking for joystick devices:\n"));
		iJoyNum = 0;
		hr = IDirectInput_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK, DIEnumJoysticks,
			(void *)&cv_usejoystick, // our user parameter is joystick number
			DIEDFL_ATTACHEDONLY);
		if (FAILED(hr))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Joystick initialize failed.\n"));
			cv_usejoystick.value = 0;
			return;
		}

		if (!lpDIJ)
		{
			if (!iJoyNum)
				CONS_Printf(M_GetText("none found\n"));
			else
			{
				CONS_Printf(M_GetText("none used\n"));
				if (cv_usejoystick.value > 0 && cv_usejoystick.value > iJoyNum)
				{
					CONS_Alert(CONS_WARNING, M_GetText("Set the use_joystick variable to one of the enumerated joystick numbers\n"));
				}
			}
			cv_usejoystick.value = 0;
			return;
		}

		I_AddExitFunc(I_ShutdownJoystick);

		// set coop level
		if (FAILED(IDirectInputDevice_SetCooperativeLevel(lpDIJ, hWndMain,
		 DISCL_NONEXCLUSIVE|DISCL_FOREGROUND)))
		{
			I_Error("I_InitJoystick: SetCooperativeLevel FAILED");
		}
	}
	else
		CONS_Printf(M_GetText("Joystick already initialized\n"));

	// we don't unacquire joystick, so let's just pretend we re-acquired it
	joystick_detected = true;
}
//Joystick 2

// ---------------
// DIEnumJoysticks2
// There is no such thing as a 'system' joystick, contrary to mouse,
// we must enumerate and choose one joystick device to use
// ---------------
static BOOL CALLBACK DIEnumJoysticks2 (LPCDIDEVICEINSTANCEA lpddi,
                                        LPVOID pvRef)   //cv_usejoystick
{
	LPDIRECTINPUTDEVICEA pdev;
	DIPROPRANGE          diprg;
	DIDEVCAPS            caps;
	BOOL                 bUseThisOne = FALSE;

	iJoy2Num++;

	//faB: if cv holds a string description of joystick, the value from atoi() is 0
	//     else, the value was probably set by user at console to one of the previsouly
	//     enumerated joysticks
	if (((consvar_t *)pvRef)->value == iJoy2Num || !strcmp(((consvar_t *)pvRef)->string, lpddi->tszProductName))
		bUseThisOne = TRUE;

	//I_OutputMsg(" cv joy2 is %s\n", ((consvar_t *)pvRef)->string);

	// print out device name
	CONS_Printf("%c%d: %s\n",
	            (bUseThisOne) ? '\2' : ' ',   // show name in white if this is the one we will use
	            iJoy2Num,
	            //(GET_DIDEVICE_SUBTYPE(lpddi->dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD) ? "Gamepad " : "Joystick",
	            lpddi->tszProductName); //, lpddi->tszInstanceName);

	// use specified joystick (cv_usejoystick.value in pvRef)
	if (!bUseThisOne)
		return DIENUM_CONTINUE;

	((consvar_t *)pvRef)->value = iJoy2Num;
	if (IDirectInput_CreateDevice (lpDI, &lpddi->guidInstance,
	                               &pdev, NULL) != DI_OK)
	{
		// if it failed, then we can't use this joystick for some
		// bizarre reason.  (Maybe the user unplugged it while we
		// were in the middle of enumerating it.)  So continue enumerating
		I_OutputMsg("DIEnumJoysticks2(): CreateDevice FAILED\n");
		return DIENUM_CONTINUE;
	}


	// get the Device capabilities
	//
	caps.dwSize = sizeof (DIDEVCAPS_DX3);
	if (FAILED(IDirectInputDevice_GetCapabilities (pdev, &caps)))
	{
		I_OutputMsg("DIEnumJoysticks2(): GetCapabilities FAILED\n");
		IDirectInputDevice_Release (pdev);
		return DIENUM_CONTINUE;
	}
	if (!(caps.dwFlags & DIDC_ATTACHED))   // should be, since we enumerate only attached devices
		return DIENUM_CONTINUE;

	Joystick2.bJoyNeedPoll = ((caps.dwFlags & DIDC_POLLEDDATAFORMAT) != 0);

	if (caps.dwFlags & DIDC_FORCEFEEDBACK)
		JoyInfo2.ForceAxises = 0;
	else
		JoyInfo2.ForceAxises = -1;

	Joystick2.bGamepadStyle = (GET_DIDEVICE_SUBTYPE(caps.dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD);
	//I_OutputMsg("Gamepad: %d\n", Joystick2.bGamepadStyle);

	CONS_Printf(M_GetText("Capabilities: %lu axes, %lu buttons, %lu POVs, poll %u, Gamepad %d\n"), caps.dwAxes, caps.dwButtons, caps.dwPOVs, Joystick2.bJoyNeedPoll, Joystick2.bGamepadStyle);

	// Set the data format to "simple joystick" - a predefined data format
	//
	// A data format specifies which controls on a device we
	// are interested in, and how they should be reported.
	//
	// This tells DirectInput that we will be passing a
	// DIJOYSTATE structure to IDirectInputDevice::GetDeviceState.
	if (IDirectInputDevice_SetDataFormat (pdev, &c_dfDIJoystick) != DI_OK)
	{
		I_OutputMsg("DIEnumJoysticks2(): SetDataFormat FAILED\n");
		IDirectInputDevice_Release (pdev);
		return DIENUM_CONTINUE;
	}

	// Set the cooperativity level to let DirectInput know how
	// this device should interact with the system and with other
	// DirectInput applications.
	if (IDirectInputDevice_SetCooperativeLevel (pdev, hWndMain,
	         DISCL_EXCLUSIVE | DISCL_FOREGROUND) != DI_OK)
	{
		I_OutputMsg("DIEnumJoysticks2(): SetCooperativeLevel FAILED\n");
		IDirectInputDevice_Release (pdev);
		return DIENUM_CONTINUE;
	}

	// set the range of the joystick axis
	diprg.diph.dwSize       = sizeof (DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof (DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.lMin              = -JOYAXISRANGE;    // value for extreme left
	diprg.lMax              = +JOYAXISRANGE;    // value for extreme right

	diprg.diph.dwObj = DIJOFS_X;    // set the x-axis range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//goto SetPropFail;
		JoyInfo2.X = FALSE;
	}
	else JoyInfo2.X = TRUE;

	diprg.diph.dwObj = DIJOFS_Y;    // set the y-axis range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
//SetPropFail:
//		I_OutputMsg("DIEnumJoysticks(): SetProperty FAILED\n");
//		IDirectInputDevice_Release (pdev);
//		return DIENUM_CONTINUE;
		JoyInfo2.Y = FALSE;
	}
	else JoyInfo2.Y = TRUE;

	diprg.diph.dwObj = DIJOFS_Z;    // set the z-axis range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_Z not found\n");
		JoyInfo2.Z = FALSE;
	}
	else JoyInfo2.Z = TRUE;

	diprg.diph.dwObj = DIJOFS_RX;   // set the x-rudder range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RX (x-rudder) not found\n");
		JoyInfo2.Rx = FALSE;
	}
	else JoyInfo2.Rx = TRUE;

	diprg.diph.dwObj = DIJOFS_RY;   // set the y-rudder range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RY (y-rudder) not found\n");
		JoyInfo2.Ry = FALSE;
	}
	else JoyInfo2.Ry = TRUE;

	diprg.diph.dwObj = DIJOFS_RZ;   // set the z-rudder range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RZ (z-rudder) not found\n");
		JoyInfo2.Rz = FALSE;
	}
	else JoyInfo2.Rz = TRUE;
	diprg.diph.dwObj = DIJOFS_SLIDER(0);   // set the x-misc range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RZ (x-misc) not found\n");
		JoyInfo2.U = FALSE;
	}
	else JoyInfo2.U = TRUE;

	diprg.diph.dwObj = DIJOFS_SLIDER(1);   // set the y-misc range
	if (FAILED(IDirectInputDevice_SetProperty(pdev, DIPROP_RANGE, &diprg.diph)))
	{
		//I_OutputMsg("DIJOFS_RZ (y-misc) not found\n");
		JoyInfo2.V = FALSE;
	}
	else JoyInfo2.V = TRUE;

	// set X axis dead zone to 25% (to avoid accidental turning)
	if (!Joystick2.bGamepadStyle)
	{
		if (JoyInfo2.X)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_X,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for X DEAD ZONE");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.Y)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_Y,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for Y DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.Z)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_Z,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for Z DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.Rx)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_RX,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for RX DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.Ry)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_RY,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for RY DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.Rz)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_RZ,
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for RZ DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.U)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(0),
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for U DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
		if (JoyInfo2.V)
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_DEADZONE, DIJOFS_SLIDER(1),
			                              DIPH_BYOFFSET, DIDEADZONE)))
			{
				I_OutputMsg("DIEnumJoysticks2(): couldn't SetProperty for V DEAD ZONE\n");
				//IDirectInputDevice_Release (pdev);
				//return DIENUM_CONTINUE;
			}
	}

	// query for IDirectInputDevice2 - we need this to poll the joystick
	if (bDX0300)
	{
		FFType i = EvilForce;
		// we won't use the poll
		lpDIJA = NULL;
		for (i = 0; i > NumberofForces; i++)
			lpDIE[i] = NULL;
	}
	else
	{
		LPDIRECTINPUTDEVICE2A *rp = &lpDIJ2A;
		LPVOID *tp  = (LPVOID *)rp;
		if (FAILED(IDirectInputDevice_QueryInterface(pdev, &IID_IDirectInputDevice2, tp)))
		{
			I_OutputMsg("DIEnumJoysticks2(): QueryInterface FAILED\n");
			IDirectInputDevice_Release (pdev);
			return DIENUM_CONTINUE;
		}

		if (lpDIJ2A && JoyInfo2.ForceAxises != -1)
		{
			// Since we will be playing force feedback effects, we should disable the
			// auto-centering spring.
			if (FAILED(SetDIDwordProperty(pdev, DIPROP_AUTOCENTER, 0, DIPH_DEVICE, FALSE)))
			{
				//NOP
			}

			// Enumerate and count the axes of the joystick
			if (FAILED(IDirectInputDevice_EnumObjects(pdev, EnumAxesCallback,
				(LPVOID)&JoyInfo2.ForceAxises, DIDFT_AXIS)))
			{
				JoyInfo2.ForceAxises = -1;
			}
			else
			{
				SetupAllForces(lpDIJ2A,lpDIE2,JoyInfo2.ForceAxises);
			}
		}
	}

	// we successfully created an IDirectInputDevice.  So stop looking
	// for another one.
	lpDIJ2 = pdev;
	return DIENUM_STOP;
}


// --------------
// I_InitJoystick2
// This is called everytime the 'use_joystick2' variable changes
// It is normally called at least once at startup when the config is loaded
// --------------
void I_InitJoystick2 (void)
{
	HRESULT hr;

	// cleanup
	I_ShutdownJoystick2 ();

	joystick2_detected = false;

	// joystick detection can be skipped by setting use_joystick to 0
	if (!lpDI || M_CheckParm("-nojoy"))
	{
		CONS_Printf(M_GetText("Joystick2 disabled\n"));
		return;
	}
	else
		// don't do anything at the registration of the joystick cvar,
		// until config is loaded
		if (!strcmp(cv_usejoystick2.string, "0"))
			return;

	// acquire the joystick only once
	if (!lpDIJ2)
	{
		joystick2_detected = false;

		CONS_Printf(M_GetText("Looking for joystick devices:\n"));
		iJoy2Num = 0;
		hr = IDirectInput_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK,
		                              DIEnumJoysticks2,
		                              (LPVOID)&cv_usejoystick2,    // our user parameter is joystick number
		                              DIEDFL_ATTACHEDONLY);
		if (FAILED(hr))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Joystick initialize failed.\n"));
			cv_usejoystick2.value = 0;
			return;
		}

		if (!lpDIJ2)
		{
			if (iJoy2Num == 0)
				CONS_Printf(M_GetText("none found\n"));
			else
			{
				CONS_Printf(M_GetText("none used\n"));
				if (cv_usejoystick2.value > 0 &&
				    cv_usejoystick2.value > iJoy2Num)
				{
					CONS_Alert(CONS_WARNING, M_GetText("Set the use_joystick2 variable to one of the enumerated joysticks number\n"));
				}
			}
			cv_usejoystick2.value = 0;
			return;
		}

		I_AddExitFunc (I_ShutdownJoystick2);

		// set coop level
		if (FAILED(IDirectInputDevice_SetCooperativeLevel (lpDIJ2, hWndMain, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
			I_Error("I_InitJoystick2: SetCooperativeLevel FAILED");

		// later
		//if (FAILED(IDirectInputDevice_Acquire (lpDIJ2)))
		//    I_Error("Couldn't acquire Joystick2");

		joystick2_detected = true;
	}
	else
		CONS_Printf(M_GetText("Joystick already initialized\n"));

	//faB: we don't unacquire joystick, so let's just pretend we re-acquired it
	joystick2_detected = true;
}

/**	\brief Joystick 1 buttons states
*/
static UINT64 lastjoybuttons = 0;

/**	\brief Joystick 1 hats state
*/
static UINT64 lastjoyhats = 0;

static VOID I_ShutdownJoystick(VOID)
{
	int i;
	event_t event;

	lastjoybuttons = lastjoyhats = 0;

	event.type = ev_keyup;

	// emulate the up of all joystick buttons
	for (i = 0;i < JOYBUTTONS;i++)
	{
		event.data1 = KEY_JOY1+i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for (i = 0;i < JOYHATS*4;i++)
	{
		event.data1 = KEY_HAT1+i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick;
	for (i = 0;i < JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	if (joystick_detected)
		CONS_Printf("I_ShutdownJoystick()\n");

	for (i = 0; i > NumberofForces; i++)
	{
		if (lpDIE[i])
		{
			IDirectInputEffect_Release(lpDIE[i]);
			lpDIE[i] = NULL;

		}
	}
	if (lpDIJ)
	{
		IDirectInputDevice_Unacquire(lpDIJ);
		IDirectInputDevice_Release(lpDIJ);
		lpDIJ = NULL;
	}
	if (lpDIJA)
	{
		IDirectInputDevice2_Release(lpDIJA);
		lpDIJA = NULL;
	}
	joystick_detected = false;
}

/**	\brief Joystick 2 buttons states
*/
static UINT64 lastjoy2buttons = 0;

/**	\brief Joystick 2 hats state
*/
static UINT64 lastjoy2hats = 0;

static VOID I_ShutdownJoystick2(VOID)
{
	int i;
	event_t event;

	lastjoy2buttons = lastjoy2hats = 0;

	event.type = ev_keyup;

	// emulate the up of all joystick buttons
	for (i = 0;i < JOYBUTTONS;i++)
	{
		event.data1 = KEY_2JOY1+i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for (i = 0;i < JOYHATS*4;i++)
	{
		event.data1 = KEY_2HAT1+i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick2;
	for (i = 0;i < JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	if (joystick2_detected)
		CONS_Printf("I_ShutdownJoystick2()\n");

	for (i = 0; i > NumberofForces; i++)
	{
		if (lpDIE2[i])
		{
			IDirectInputEffect_Release(lpDIE2[i]);
			lpDIE2[i] = NULL;
		}
	}
	if (lpDIJ2)
	{
		IDirectInputDevice_Unacquire(lpDIJ2);
		IDirectInputDevice_Release(lpDIJ2);
		lpDIJ2 = NULL;
	}
	if (lpDIJ2A)
	{
		IDirectInputDevice2_Release(lpDIJ2A);
		lpDIJ2A = NULL;
	}
	joystick2_detected = false;
}

// -------------------
// I_GetJoystickEvents
// Get current joystick axis and button states
// -------------------
void I_GetJoystickEvents(void)
{
	HRESULT hr;
	DIJOYSTATE js; // DirectInput joystick state
	int i;
	UINT64 joybuttons = 0;
	UINT64 joyhats = 0;
	event_t event;

	if (!lpDIJ)
		return;

	// if input is lost then acquire and keep trying
	for (;;)
	{
		// poll the joystick to read the current state
		// if the device doesn't require polling, this function returns almost instantly
		if (lpDIJA)
		{
			hr = IDirectInputDevice2_Poll(lpDIJA);
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				goto acquire;
			else if (FAILED(hr))
			{
				I_OutputMsg("I_GetJoystickEvents(): Poll FAILED\n");
				return;
			}
		}

		// get the input's device state, and put the state in dims
		hr = IDirectInputDevice_GetDeviceState(lpDIJ, sizeof (DIJOYSTATE), &js);

		if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
		{
			// DirectInput is telling us that the input stream has
			// been interrupted.  We aren't tracking any state
			// between polls, so we don't have any special reset
			// that needs to be done.  We just re-acquire and
			// try again.
			goto acquire;
		}
		else if (FAILED(hr))
		{
			I_OutputMsg("I_GetJoystickEvents(): GetDeviceState FAILED\n");
			return;
		}

		break;
acquire:
		if (FAILED(IDirectInputDevice_Acquire(lpDIJ)))
			return;
	}

	// look for as many buttons as g_input code supports, we don't use the others
	for (i = JOYBUTTONS_MIN - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (js.rgbButtons[i])
			joybuttons |= 1;
	}

	for (i = JOYHATS_MIN -1; i >=0; i--)
	{
		if (js.rgdwPOV[i] != 0xffff && js.rgdwPOV[i] != 0xffffffff)
		{
			if     (js.rgdwPOV[i] > 270 * DI_DEGREES || js.rgdwPOV[i] <  90 * DI_DEGREES)
				joyhats |= (UINT64)1<<(0 + 4*(UINT64)i); // UP
			else if (js.rgdwPOV[i] >  90 * DI_DEGREES && js.rgdwPOV[i] < 270 * DI_DEGREES)
				joyhats |= (UINT64)1<<(1 + 4*(UINT64)i); // DOWN
			if     (js.rgdwPOV[i] >   0 * DI_DEGREES && js.rgdwPOV[i] < 180 * DI_DEGREES)
				joyhats |= (UINT64)1<<(3 + 4*(UINT64)i); // LEFT
			else if (js.rgdwPOV[i] > 180 * DI_DEGREES && js.rgdwPOV[i] < 360 * DI_DEGREES)
				joyhats |= (UINT64)1<<(2 + 4*(UINT64)i); // RIGHT
		}
	}

	if (joybuttons != lastjoybuttons)
	{
		UINT64 j = 1; // keep only bits that changed since last time
		UINT64 newbuttons = joybuttons ^ lastjoybuttons;
		lastjoybuttons = joybuttons;

		for (i = 0; i < JOYBUTTONS_MIN; i++, j <<= 1)
		{
			if (newbuttons & j) // button changed state?
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}

	if (joyhats != lastjoyhats)
	{
		UINT64 j = 1; // keep only bits that changed since last time
		UINT64 newhats = joyhats ^ lastjoyhats;
		lastjoyhats = joyhats;

		for (i = 0; i < JOYHATS_MIN*4; i++, j <<= 1)
		{
			if (newhats & j) // button changed state?
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

	// send joystick axis positions
	event.type = ev_joystick;
	event.data1 = event.data2 = event.data3 = 0;

	if (Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo.X)
		{
			if (js.lX < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.lX > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo.Y)
		{
			if (js.lY < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.lY > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo.X)  event.data2 = js.lX; // x axis
		if (JoyInfo.Y)  event.data3 = js.lY; // y axis
	}

	D_PostEvent(&event);
#if JOYAXISSET > 1
	event.data1 = 1;
	event.data2 = event.data3 = 0;

	if (Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo.Z)
		{
			if (js.lZ < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.lZ > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo.Rx)
		{
			if (js.lRx < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.lRx > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo.Z)  event.data2 = js.lZ;  // z axis
		if (JoyInfo.Rx) event.data3 = js.lRx; // rx axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 2
	event.data1 = 2;
	event.data2 = event.data3 = 0;

	if (Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo.Rx)
		{
			if (js.lRy < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.lRy > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo.Rz)
		{
			if (js.lRz < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.lRz > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo.Ry) event.data2 = js.lRy; // ry axis
		if (JoyInfo.Rz) event.data3 = js.lRz; // rz axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 3
	event.data1 = 3;
	event.data2 = event.data3 = 0;
	if (Joystick.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo.U)
		{
			if (js.rglSlider[0] < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.rglSlider[0] > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo.V)
		{
			if (js.rglSlider[1] < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.rglSlider[1] > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo.U)  event.data2 = js.rglSlider[0]; // U axis
		if (JoyInfo.V)  event.data3 = js.rglSlider[1]; // V axis
	}
	D_PostEvent(&event);
#endif
}

// -------------------
// I_GetJoystickEvents
// Get current joystick axis and button states
// -------------------
void I_GetJoystick2Events(void)
{
	HRESULT hr;
	DIJOYSTATE js; // DirectInput joystick state
	int i;
	UINT64 joybuttons = 0;
	UINT64 joyhats = 0;
	event_t event;

	if (!lpDIJ2)
		return;

	// if input is lost then acquire and keep trying
	for (;;)
	{
		// poll the joystick to read the current state
		// if the device doesn't require polling, this function returns almost instantly
		if (lpDIJ2A)
		{
			hr = IDirectInputDevice2_Poll(lpDIJ2A);
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				goto acquire;
			else if (FAILED(hr))
			{
				I_OutputMsg("I_GetJoystick2Events(): Poll FAILED\n");
				return;
			}
		}

		// get the input's device state, and put the state in dims
		hr = IDirectInputDevice_GetDeviceState(lpDIJ2, sizeof (DIJOYSTATE), &js);

		if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
		{
			// DirectInput is telling us that the input stream has
			// been interrupted.  We aren't tracking any state
			// between polls, so we don't have any special reset
			// that needs to be done.  We just re-acquire and
			// try again.
			goto acquire;
		}
		else if (FAILED(hr))
		{
			I_OutputMsg("I_GetJoystickEvents2(): GetDeviceState FAILED\n");
			return;
		}

		break;
acquire:
		if (FAILED(IDirectInputDevice_Acquire(lpDIJ2)))
			return;
	}

	// look for as many buttons as g_input code supports, we don't use the others
	for (i = JOYBUTTONS_MIN - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (js.rgbButtons[i])
			joybuttons |= 1;
	}

	for (i = JOYHATS_MIN -1; i >=0; i--)
	{
		if (js.rgdwPOV[i] != 0xffff && js.rgdwPOV[i] != 0xffffffff)
		{
			if     (js.rgdwPOV[i] > 270 * DI_DEGREES || js.rgdwPOV[i] <  90 * DI_DEGREES)
				joyhats |= (UINT64)1<<(0 + 4*(UINT64)i); // UP
			else if (js.rgdwPOV[i] >  90 * DI_DEGREES && js.rgdwPOV[i] < 270 * DI_DEGREES)
				joyhats |= (UINT64)1<<(1 + 4*(UINT64)i); // DOWN
			if     (js.rgdwPOV[i] >   0 * DI_DEGREES && js.rgdwPOV[i] < 180 * DI_DEGREES)
				joyhats |= (UINT64)1<<(3 + 4*(UINT64)i); // LEFT
			else if (js.rgdwPOV[i] > 180 * DI_DEGREES && js.rgdwPOV[i] < 360 * DI_DEGREES)
				joyhats |= (UINT64)1<<(2 + 4*(UINT64)i); // RIGHT
		}
	}

	if (joybuttons != lastjoy2buttons)
	{
		UINT64 j = 1; // keep only bits that changed since last time
		UINT64 newbuttons = joybuttons ^ lastjoy2buttons;
		lastjoy2buttons = joybuttons;

		for (i = 0; i < JOYBUTTONS_MIN; i++, j <<= 1)
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

	if (joyhats != lastjoy2hats)
	{
		UINT64 j = 1; // keep only bits that changed since last time
		UINT64 newhats = joyhats ^ lastjoy2hats;
		lastjoy2hats = joyhats;

		for (i = 0; i < JOYHATS_MIN*4; i++, j <<= 1)
		{
			if (newhats & j) // button changed state?
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

	// send joystick axis positions
	event.type = ev_joystick2;
	event.data1 = event.data2 = event.data3 = 0;

	if (Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo2.X)
		{
			if (js.lX < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.lX > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo2.Y)
		{
			if (js.lY < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.lY > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo2.X)  event.data2 = js.lX; // x axis
		if (JoyInfo2.Y)  event.data3 = js.lY; // y axis
	}

	D_PostEvent(&event);
#if JOYAXISSET > 1
	event.data1 = 1;
	event.data2 = event.data3 = 0;

	if (Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo2.Z)
		{
			if (js.lZ < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.lZ > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo2.Rx)
		{
			if (js.lRx < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.lRx > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo2.Z)  event.data2 = js.lZ;  // z axis
		if (JoyInfo2.Rx) event.data3 = js.lRx; // rx axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 2
	event.data1 = 2;
	event.data2 = event.data3 = 0;

	if (Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo2.Rx)
		{
			if (js.lRy < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.lRy > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo2.Rz)
		{
			if (js.lRz < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.lRz > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo2.Ry) event.data2 = js.lRy; // ry axis
		if (JoyInfo2.Rz) event.data3 = js.lRz; // rz axis
	}

	D_PostEvent(&event);
#endif
#if JOYAXISSET > 3
	event.data1 = 3;
	event.data2 = event.data3 = 0;
	if (Joystick2.bGamepadStyle)
	{
		// gamepad control type, on or off, live or die
		if (JoyInfo2.U)
		{
			if (js.rglSlider[0] < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (js.rglSlider[0] > JOYAXISRANGE/2)
				event.data2 = 1;
		}
		if (JoyInfo2.V)
		{
			if (js.rglSlider[1] < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (js.rglSlider[1] > JOYAXISRANGE/2)
				event.data3 = 1;
		}
	}
	else
	{
		// analog control style, just send the raw data
		if (JoyInfo2.U)  event.data2 = js.rglSlider[0]; // U axis
		if (JoyInfo2.V)  event.data3 = js.rglSlider[1]; // V axis
	}
	D_PostEvent(&event);
#endif
}

static int numofjoy = 0;
static char joyname[MAX_PATH];
static int needjoy = -1;

static BOOL CALLBACK DIEnumJoysticksCount (LPCDIDEVICEINSTANCEA lpddi,
                                            LPVOID pvRef)   //joyname
{
	numofjoy++;
	if (needjoy == numofjoy && pvRef && pvRef == (void *)joyname && lpddi
		&& lpddi->tszProductName)
	{
		sprintf(joyname,"%s",lpddi->tszProductName);
		return DIENUM_STOP;
	}
	//else I_OutputMsg("DIEnumJoysticksCount need help!\n");
	return DIENUM_CONTINUE;
}

INT32 I_NumJoys(void)
{
	HRESULT hr;
	needjoy = -1;
	numofjoy = 0;
	hr = IDirectInput_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK,
		DIEnumJoysticksCount, (LPVOID)&numofjoy, DIEDFL_ATTACHEDONLY);
	if (FAILED(hr))
	{
		I_OutputMsg("\nI_NumJoys(): EnumDevices FAILED\n");
	}
	return numofjoy;

}

const char *I_GetJoyName(INT32 joyindex)
{
	HRESULT hr;
	needjoy = joyindex;
	numofjoy = 0;
	ZeroMemory(joyname,sizeof (joyname));
	hr = IDirectInput_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK,
		DIEnumJoysticksCount, (LPVOID)joyname, DIEDFL_ATTACHEDONLY);
	if (FAILED(hr))
	{
		I_OutputMsg("\nI_GetJoyName(): EnumDevices FAILED\n");
	}
	if (joyname[0] == 0) return NULL;
	return joyname;
}

#ifndef NOMUMBLE
// Best Mumble positional audio settings:
// Minimum distance 3.0 m
// Bloom 175%
// Maximum distance 80.0 m
// Minimum volume 50%
#define DEG2RAD (0.017453292519943295769236907684883l) // TAU/360 or PI/180
#define MUMBLEUNIT (64.0f) // FRACUNITS in a Meter

static struct {
	UINT32 uiVersion;
	DWORD uiTick;
	float fAvatarPosition[3];
	float fAvatarFront[3];
	float fAvatarTop[3]; // defaults to Y-is-up (only used for leaning)
	wchar_t name[256]; // game name
	float fCameraPosition[3];
	float fCameraFront[3];
	float fCameraTop[3]; // defaults to Y-is-up (only used for leaning)
	wchar_t identity[256]; // player id
	UINT32 context_len;
	unsigned char context[256]; // server/team
	wchar_t description[2048]; // game description
} *mumble = NULL;

static inline void I_SetupMumble(void)
{
	HANDLE hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (!hMap)
		return;

	mumble = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*mumble));
	if (!mumble)
		CloseHandle(hMap);
}

void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
	double angle;
	fixed_t anglef;

	if (!mumble)
		return;

	if(mumble->uiVersion != 2) {
		wcsncpy(mumble->name, L"SRB2 "VERSIONSTRINGW, 256);
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
}
#endif

// ===========================================================================================
//                                                                       DIRECT INPUT KEYBOARD
// ===========================================================================================

static UINT8 ASCIINames[256] =
{
	//  0       1       2       3       4       5       6       7
	//  8       9       A       B       C       D       E       F
	0,      27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0', KEY_MINUS,KEY_EQUALS,KEY_BACKSPACE, KEY_TAB,
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',KEY_ENTER,KEY_LCTRL,'a',    's',
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	'\'',   '`', KEY_LSHIFT,'\\',   'z',    'x',    'c',    'v',
	'b',    'n',    'm',    ',',    '.',    '/', KEY_RSHIFT,'*',
	KEY_LALT,KEY_SPACE,KEY_CAPSLOCK,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_KEYPAD7,
	KEY_KEYPAD8,KEY_KEYPAD9,KEY_MINUSPAD,KEY_KEYPAD4,KEY_KEYPAD5,KEY_KEYPAD6,KEY_PLUSPAD,KEY_KEYPAD1,
	KEY_KEYPAD2,KEY_KEYPAD3,KEY_KEYPAD0,KEY_KPADDEL,0,0,0,      KEY_F11,
	KEY_F12,0,          0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,

	//  0       1       2       3       4       5       6       7
	//  8       9       A       B       C       D       E       F

	0,          0,      0,      0,      0,      0,      0,      0, // 0x80
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,  KEY_ENTER,KEY_RCTRL,0,      0,
	0,          0,      0,      0,      0,      0,      0,      0, // 0xa0
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0, KEY_KPADDEL, 0,KEY_KPADSLASH,0,      0,
	KEY_RALT,   0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,  KEY_HOME, // 0xc0
	KEY_UPARROW,KEY_PGUP,0,KEY_LEFTARROW,0,KEY_RIGHTARROW,0,KEY_END,
	KEY_DOWNARROW,KEY_PGDN, KEY_INS,KEY_DEL,0,0,0,0,
	0,          0,      0,KEY_LEFTWIN,KEY_RIGHTWIN,KEY_MENU, 0, 0,
	0,          0,      0,      0,      0,      0,      0,      0, // 0xe0
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0,
	0,          0,      0,      0,      0,      0,      0,      0
};

// Return a key that has been pushed, or 0 (replace getchar() at game startup)
//
INT32 I_GetKey(void)
{
	event_t *ev;

	if (eventtail != eventhead)
	{
		ev = &events[eventtail];
		eventtail = (eventtail+1) & (MAXEVENTS-1);
		if (ev->type == ev_keydown || ev->type == ev_console)
			return ev->data1;
		else
			return 0;
	}
	return 0;
}

// -----------------
// I_StartupKeyboard
// Installs DirectInput keyboard
// -----------------
#define DI_KEYBOARD_BUFFERSIZE 32 // number of data elements in keyboard buffer

static void I_StartupKeyboard(void)
{
	DIPROPDWORD dip;

	if (dedicated || !lpDI)
		return;

	// make sure the app window has the focus or DirectInput acquire keyboard won't work
	if (hWndMain)
	{
		SetFocus(hWndMain);
		ShowWindow(hWndMain, SW_SHOW);
		UpdateWindow(hWndMain);
	}

	// detect error
	if (lpDIK)
	{
		I_OutputMsg("I_StartupKeyboard(): called twice\n");
		return;
	}

	CreateDevice2A(lpDI, &GUID_SysKeyboard, &lpDIK, NULL);

	if (lpDIK)
	{
		if (FAILED(IDirectInputDevice_SetDataFormat(lpDIK, &c_dfDIKeyboard)))
			I_Error("Couldn't set keyboard data format");

		// create buffer for buffered data
		dip.diph.dwSize = sizeof (dip);
		dip.diph.dwHeaderSize = sizeof (dip.diph);
		dip.diph.dwObj = 0;
		dip.diph.dwHow = DIPH_DEVICE;
		dip.dwData = DI_KEYBOARD_BUFFERSIZE;
		if (FAILED(IDirectInputDevice_SetProperty(lpDIK, DIPROP_BUFFERSIZE, &dip.diph)))
			I_Error("Couldn't set keyboard buffer size");

		if (FAILED(IDirectInputDevice_SetCooperativeLevel(lpDIK, hWndMain,
			DISCL_NONEXCLUSIVE|DISCL_FOREGROUND)))
		{
			I_Error("Couldn't set keyboard coop level");
		}
	}
	else
		I_Error("Couldn't create keyboard input");

	I_AddExitFunc(I_ShutdownKeyboard);
	hacktics = 0; // see definition
	keyboard_started = true;
}

// ------------------
// I_ShutdownKeyboard
// Release DirectInput keyboard.
// ------------------
static VOID I_ShutdownKeyboard(VOID)
{
	if (!keyboard_started)
		return;

	CONS_Printf("I_ShutdownKeyboard()\n");

	if (lpDIK)
	{
		IDirectInputDevice_Unacquire(lpDIK);
		IDirectInputDevice_Release(lpDIK);
		lpDIK = NULL;
	}

	keyboard_started = false;
}

// -------------------
// I_GetKeyboardEvents
// Get buffered data from the keyboard
// -------------------
static VOID I_GetKeyboardEvents(VOID)
{
	static BOOL KeyboardLost = false;

	// simply repeat the last pushed key every xx tics,
	// make more user friendly input for Console and game Menus
#define KEY_REPEAT_DELAY (NEWTICRATE/17) // TICRATE tics, repeat every 1/3 second
	static LONG RepeatKeyTics = 0;
	static int RepeatKeyCode = 0;

	DIDEVICEOBJECTDATA rgdod[DI_KEYBOARD_BUFFERSIZE];
	DWORD dwItems, d;
	HRESULT hr;
	int ch;

	event_t event;
	ZeroMemory(&event,sizeof (event));

	if (!keyboard_started)
		return;

	if (!appActive && RepeatKeyCode) // Stop when lost focus
	{
		event.type = ev_keyup;
		event.data1 = RepeatKeyCode;
		D_PostEvent(&event);
		RepeatKeyCode = 0;
	}
getBufferedData:
	dwItems = DI_KEYBOARD_BUFFERSIZE;
	hr = IDirectInputDevice_GetDeviceData(lpDIK, sizeof (DIDEVICEOBJECTDATA), rgdod, &dwItems, 0);

	// If data stream was interrupted, reacquire the device and try again.
	if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
	{
		// why it succeeds to acquire just after I don't understand.. so I set the flag BEFORE
		KeyboardLost = true;

		hr = IDirectInputDevice_Acquire(lpDIK);
		if (SUCCEEDED(hr))
			goto getBufferedData;
		return;
	}

	// we lost data, get device actual state to recover lost information
	if (hr == DI_BUFFEROVERFLOW)
	{
		/// \note either uncomment or delete block
		//I_Error("DI buffer overflow (keyboard)");
		//I_RecoverKeyboardState ();

		//hr = IDirectInputDevice_GetDeviceState (lpDIM, sizeof (keys), &diMouseState);
	}

	// We got buffered input, act on it
	if (SUCCEEDED(hr))
	{
		// if we previously lost keyboard data, recover its current state
		if (KeyboardLost)
		{
			/// \bug hack simply clears the keys so we don't have the last pressed keys
			/// still active.. to have to re-trigger it is not much trouble for the user.
			ZeroMemory(gamekeydown, NUMKEYS);
			KeyboardLost = false;
		}

		// dwItems contains number of elements read (could be 0)
		for (d = 0; d < dwItems; d++)
		{
			// dwOfs member is DIK_* value
			// dwData member 0x80 bit set press down, clear is release

			if (rgdod[d].dwData & 0x80)
				event.type = ev_keydown;
			else
				event.type = ev_keyup;

			ch = rgdod[d].dwOfs & 0xFF;
			if (ASCIINames[ch])
				event.data1 = ASCIINames[ch];
			else
				event.data1 = 0x80;

			D_PostEvent(&event);
		}

		// Key Repeat
		if (dwItems)
		{
			// new key events, so stop repeating key
			RepeatKeyCode = 0;
			// delay is tripled for first repeating key
			RepeatKeyTics = hacktics + (KEY_REPEAT_DELAY*3);
			if (event.type == ev_keydown) // use the last event!
				RepeatKeyCode = event.data1;
		}
		else
		{
			// no new keys, repeat last pushed key after some time
			if (RepeatKeyCode && hacktics - RepeatKeyTics > KEY_REPEAT_DELAY)
			{
				event.type = ev_keydown;
				event.data1 = RepeatKeyCode;
				D_PostEvent(&event);

				RepeatKeyTics = hacktics;
			}
		}
	}
}

static HINSTANCE DInputDLL = NULL;
typedef HRESULT (WINAPI *DICreateA)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter);
static DICreateA pfnDirectInputCreateA = NULL;

BOOL LoadDirectInput(VOID)
{
	// load dinput.dll
	DInputDLL = LoadLibraryA("DINPUT.DLL");
	if (DInputDLL == NULL)
		return false;
	pfnDirectInputCreateA = (DICreateA)(LPVOID)GetProcAddress(DInputDLL, "DirectInputCreateA");
	if (pfnDirectInputCreateA == NULL)
		return false;
	return true;
}

static inline VOID UnLoadDirectInput(VOID)
{
	if (!DInputDLL)
		return;
	FreeLibrary(DInputDLL);
	pfnDirectInputCreateA = NULL;
	DInputDLL = NULL;
}


//
// Closes DirectInput
//
static VOID I_ShutdownDirectInput(VOID)
{
	if (lpDI)
		IDirectInput_Release(lpDI);
	lpDI = NULL;
	UnLoadDirectInput();
}

// This stuff should get rid of the exception and page faults when
// SRB2 bugs out with an error. Now it should exit cleanly.
//
INT32 I_StartupSystem(void)
{
	HRESULT hr;
	HINSTANCE myInstance = GetModuleHandle(NULL);

	// some 'more global than globals' things to initialize here ?
	graphics_started = keyboard_started = sound_started = cdaudio_started = false;

	I_StartupKeyboard();

#ifdef NDEBUG

#ifdef BUGTRAP
	if(!IsBugTrapLoaded())
	{
#endif
		signal(SIGABRT, signal_handler);
		signal(SIGFPE, signal_handler);
		signal(SIGILL, signal_handler);
		signal(SIGSEGV, signal_handler);
		signal(SIGTERM, signal_handler);
		signal(SIGINT, signal_handler);
#ifdef BUGTRAP
	}
#endif

#endif

#ifndef NOMUMBLE
	I_SetupMumble();
#endif

	if (!pfnDirectInputCreateA)
		return 1;
	// create DirectInput - so that I_StartupKeyboard/Mouse can be called later on
	// from D_SRB2Main just like DOS version
	hr = pfnDirectInputCreateA(myInstance, DIRECTINPUT_VERSION, &lpDI, NULL);

	if (SUCCEEDED(hr))
		bDX0300 = FALSE;
	else
	{
		// try opening DirectX3 interface for NT compatibility
		hr = pfnDirectInputCreateA(myInstance, DXVERSION_NTCOMPATIBLE, &lpDI, NULL);

		if (FAILED(hr))
		{
			const char *sErr;
			switch (hr)
			{
				case DIERR_BETADIRECTINPUTVERSION:
					sErr = "DIERR_BETADIRECTINPUTVERSION";
					break;
				case DIERR_INVALIDPARAM:
					sErr = "DIERR_INVALIDPARAM";
					break;
				case DIERR_OLDDIRECTINPUTVERSION :
					sErr = "DIERR_OLDDIRECTINPUTVERSION";
					break;
				case DIERR_OUTOFMEMORY:
					sErr = "DIERR_OUTOFMEMORY";
					break;
				default:
					sErr = "UNKNOWN";
					break;
			}
			I_Error("Couldn't create DirectInput (reason: %s)", sErr);
		}
		else
			CONS_Printf("\x82%s", M_GetText("Using DirectX3 interface\n"));

		// only use DirectInput3 compatible structures and calls
		bDX0300 = TRUE;
	}
	I_AddExitFunc(I_ShutdownDirectInput);
	return 0;
}

// Closes down everything. This includes restoring the initial
// palette and video mode, and removing whatever mouse, keyboard, and
// timer routines have been installed.
//
/// \bug doesn't restore wave/midi device volume
//
// Shutdown user funcs are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
	int c;

	for (c = MAX_QUIT_FUNCS - 1; c >= 0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
}

// my god how win32 suck
typedef BOOL (WINAPI *p_GetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

void I_GetDiskFreeSpace(INT64* freespace)
{
	static p_GetDiskFreeSpaceExA pfnGetDiskFreeSpaceEx = NULL;
	static boolean testwin95 = false;
	ULARGE_INTEGER usedbytes, lfreespace;

	if (!testwin95)
	{
		pfnGetDiskFreeSpaceEx = (p_GetDiskFreeSpaceExA)(LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA");
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
		*freespace = BytesPerSector * SectorsPerCluster * NumberOfFreeClusters;
	}
}

char *I_GetUserName(void)
{
	static char username[MAXPLAYERNAME+1];
	char *p;
	DWORD i = MAXPLAYERNAME;

	if (!GetUserNameA(username, &i))
	{
		p = getenv("USER");
		if (!p)
		{
			p = getenv("user");
			if (!p)
			{
				p = getenv("USERNAME");
				if (!p)
				{
					p = getenv("username");
					if (!p)
					{
						return NULL;
					}
				}
			}
		}
		strlcpy(username, p, sizeof (username));
	}

	if (!strlen(username))
		return NULL;
	return username;
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{
	UNREFERENCED_PARAMETER(unixright); /// \todo should implement ntright under nt...
	return CreateDirectoryA(dirname, NULL);
}

char * I_GetEnv(const char *name)
{
	return getenv(name);
}

INT32 I_PutEnv(char *variable)
{
	return putenv(variable);
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	return NULL;
}

typedef BOOL (WINAPI *p_IsProcessorFeaturePresent) (DWORD);

const CPUInfoFlags *I_CPUInfo(void)
{
	static CPUInfoFlags WIN_CPUInfo;
	SYSTEM_INFO SI;
	p_IsProcessorFeaturePresent pfnCPUID = (p_IsProcessorFeaturePresent)(LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsProcessorFeaturePresent");

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
	GetSystemInfo(&SI);
	WIN_CPUInfo.CPUs = SI.dwNumberOfProcessors;
	WIN_CPUInfo.IA64 = (SI.dwProcessorType == 2200); // PROCESSOR_INTEL_IA64
	WIN_CPUInfo.AMD64 = (SI.dwProcessorType == 8664); // PROCESSOR_AMD_X8664
	return &WIN_CPUInfo;
}

static void CPUAffinity_OnChange(void);
static consvar_t cv_cpuaffinity = {"cpuaffinity", "-1", CV_CALL, NULL, CPUAffinity_OnChange, 0, NULL, NULL, 0, 0, NULL};

typedef HANDLE (WINAPI *p_GetCurrentProcess) (VOID);
static p_GetCurrentProcess pfnGetCurrentProcess = NULL;
typedef BOOL (WINAPI *p_GetProcessAffinityMask) (HANDLE, PDWORD_PTR, PDWORD_PTR);
static p_GetProcessAffinityMask pfnGetProcessAffinityMask = NULL;
typedef BOOL (WINAPI *p_SetProcessAffinityMask) (HANDLE, DWORD_PTR);
static p_SetProcessAffinityMask pfnSetProcessAffinityMask = NULL;

static inline VOID GetAffinityFuncs(VOID)
{
	HMODULE h = GetModuleHandleA("kernel32.dll");
	pfnGetCurrentProcess = (p_GetCurrentProcess)(LPVOID)GetProcAddress(h, "GetCurrentProcess");
	pfnGetProcessAffinityMask = (p_GetProcessAffinityMask)(LPVOID)GetProcAddress(h, "GetProcessAffinityMask");
	pfnSetProcessAffinityMask = (p_SetProcessAffinityMask)(LPVOID)GetProcAddress(h, "SetProcessAffinityMask");
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
		CV_StealthSetValue(&cv_cpuaffinity, (int)(dwSysMask & cv_cpuaffinity.value));
	}
	else
		CV_StealthSetValue(&cv_cpuaffinity, (int)dwProcMask);
}

void I_RegisterSysCommands(void)
{
	GetAffinityFuncs();
	CV_RegisterVar(&cv_cpuaffinity);
}
#endif
