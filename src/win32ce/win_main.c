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
/// \brief Win32 WinMain Entry Point
///
///	Win32 Sonic Robo Blast 2
///
///	NOTE:
///		To compile WINDOWS SRB2 version : define a '_WINDOWS' symbol.
///		to do this go to Project/Settings/ menu, click C/C++ tab, in
///		'Preprocessor definitions:' add '_WINDOWS'

#include "../doomdef.h"
#include <stdio.h>

#include "../doomstat.h"  // netgame
#include "resource.h"

#include "../m_argv.h"
#include "../d_main.h"
#include "../i_system.h"

#include "../keys.h"    //hack quick test

#include "../console.h"

#include "fabdxlib.h"
#include "win_main.h"
#include "win_dbg.h"
#include "../i_sound.h" // midi pause/unpause
#include "../g_input.h" // KEY_MOUSEWHEELxxx

// MSWheel support for Win95/NT3.51
#include <zmouse.h>

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 523
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 524
#endif
#ifndef MK_XBUTTON1
#define MK_XBUTTON1 32
#endif
#ifndef MK_XBUTTON2
#define MK_XBUTTON2 64
#endif

typedef BOOL (WINAPI *MyFunc)(VOID);

HINSTANCE myInstance = NULL;
HWND hWndMain = NULL;
static HCURSOR windowCursor = NULL; // main window cursor

boolean appActive = false; // app window is active

#ifdef LOGMESSAGES
// this is were I log debug text, cons_printf, I_error ect for window port debugging
HANDLE logstream;
#endif

BOOL nodinput = FALSE;

static LRESULT CALLBACK MainWndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	event_t ev; //Doom input event
	int mouse_keys;

	// judgecutor:
	// Response MSH Mouse Wheel event

	if (message == MSHWheelMessage)
	{
			message = WM_MOUSEWHEEL;
		if (win9x)
			wParam <<= 16;
	}


	switch (message)
	{
		case WM_CREATE:
			nodinput = M_CheckParm("-nodinput");
			break;

		case WM_ACTIVATEAPP:           // Handle task switching
			appActive = (int)wParam;
			// pause music when alt-tab
			if (appActive  && !paused)
				I_ResumeSong(0);
			else if (!paused)
				I_PauseSong(0);
			{
				HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
				if (ci != INVALID_HANDLE_VALUE && GetFileType(ci) == FILE_TYPE_CHAR)
					appActive = true;
			}
			InvalidateRect (hWnd, NULL, TRUE);
			break;

		//for MIDI music
		case WM_MSTREAM_UPDATEVOLUME:
			I_SetMidiChannelVolume((DWORD)wParam, dwVolumePercent);
			break;

		case WM_PAINT:
			if (!appActive && !bAppFullScreen && !netgame)
				// app becomes inactive (if windowed)
			{
				// Paint "Game Paused" in the middle of the screen
				PAINTSTRUCT ps;
				RECT        rect;
				HDC hdc = BeginPaint (hWnd, &ps);
				GetClientRect (hWnd, &rect);
				DrawText (hdc, TEXT("Game Paused"), -1, &rect,
					DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				EndPaint (hWnd, &ps);
				return 0;
			}
			break;

		//case WM_RBUTTONDOWN:
		//case WM_LBUTTONDOWN:

		case WM_MOVE:
			if (bAppFullScreen)
			{
				SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				return 0;
			}
			else
			{
				windowPosX = (SHORT) LOWORD(lParam);    // horizontal position
				windowPosY = (SHORT) HIWORD(lParam);    // vertical position
				break;
			}
			break;

			// This is where switching windowed/fullscreen is handled. DirectDraw
			// objects must be destroyed, recreated, and artwork reloaded.

		case WM_DISPLAYCHANGE:
		case WM_SIZE:
			break;

		case WM_SETCURSOR:
			if (bAppFullScreen)
				SetCursor(NULL);
			else
				SetCursor(windowCursor);
			return TRUE;

		case WM_KEYUP:
			ev.type = ev_keyup;
			goto handleKeyDoom;
			break;

		case WM_KEYDOWN:
			ev.type = ev_keydown;

	handleKeyDoom:
			ev.data1 = 0;
			if (wParam == VK_PAUSE)
			// intercept PAUSE key
			{
				ev.data1 = KEY_PAUSE;
			}
			else if (!keyboard_started)
			// post some keys during the game startup
			// (allow escaping from network synchronization, or pressing enter after
			//  an error message in the console)
			{
				switch (wParam)
				{
					case VK_ESCAPE: ev.data1 = KEY_ESCAPE;  break;
					case VK_RETURN: ev.data1 = KEY_ENTER;   break;
					default: ev.data1 = MapVirtualKey((DWORD)wParam,2); // convert in to char
				}
			}

			if (ev.data1)
				D_PostEvent (&ev);

			return 0;
			break;

		// judgecutor:
		// Handle mouse events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_MOUSEMOVE:
			if (nodinput)
			{
				mouse_keys = 0;
				if (wParam & MK_LBUTTON)
					mouse_keys |= 1;
				if (wParam & MK_RBUTTON)
					mouse_keys |= 2;
				if (wParam & MK_MBUTTON)
					mouse_keys |= 4;
				if (wParam & MK_XBUTTON1)
					mouse_keys |= 8;
				if (wParam & MK_XBUTTON2)
					mouse_keys |= 16;
				I_GetSysMouseEvents(mouse_keys);
			}
			break;


		case WM_MOUSEWHEEL:
			//CONS_Printf("MW_WHEEL dispatched.\n");
			ev.type = ev_keydown;
			if ((INT16)HIWORD(wParam) > 0)
				ev.data1 = KEY_MOUSEWHEELUP;
			else
				ev.data1 = KEY_MOUSEWHEELDOWN;
			D_PostEvent(&ev);
			break;

		case WM_SETTEXT:
			COM_BufAddText((LPCSTR)lParam);
			return TRUE;
			break;

		case WM_CLOSE:
			PostQuitMessage(0);         //to quit while in-game
			ev.data1 = KEY_ESCAPE;      //to exit network synchronization
			ev.type = ev_keydown;
			D_PostEvent (&ev);
			return 0;
		case WM_DESTROY:
			//faB: main app loop will exit the loop and proceed with I_Quit()
			PostQuitMessage(0);
			break;

		default:
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


static inline VOID OpenTextConsole(void)
{
	HANDLE ci, co;
	const BOOL tco = M_CheckParm("-console") != 0;
	dedicated = M_CheckParm("-dedicated") != 0;
	if (!(dedicated || tco))
		return;
	FreeConsole();
	AllocConsole(); //Let get the real console HANDLE, because Mingw's Bash is bad!
	ci = CreateFile(TEXT("CONIN$") ,               GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	co = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ci != (HANDLE)-1)
	{
		const DWORD CM = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT;
		SetStdHandle(STD_INPUT_HANDLE,ci);
		if(GetFileType(ci) == FILE_TYPE_CHAR)
			SetConsoleMode(ci,CM); //default mode but no ENABLE_MOUSE_INPUT
	}
	if(co != (HANDLE)-1)
	{
		SetStdHandle(STD_OUTPUT_HANDLE,co);
		SetStdHandle(STD_ERROR_HANDLE,co); //maybe logstream?
	}
}

//
// Do that Windows initialization stuff...
//
static HWND OpenMainWindow (HINSTANCE hInstance, int nCmdShow, LPSTR wTitle)
{
	HWND        hWnd;
	WNDCLASS    wc;

	// Set up and register window class
	nCmdShow = 0;
	wc.style         = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
	wc.lpfnWndProc   = MainWndproc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DLICON1));
	windowCursor     = LoadCursor(NULL, IDC_WAIT); //LoadCursor(hInstance, MAKEINTRESOURCE(IDC_DLCURSOR1));
	wc.hCursor       = windowCursor;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = TEXT("SRB2WC");
	if (!RegisterClass(&wc))
		return (HANDLE)-1;

	// Create a window
	// CreateWindowEx - seems to create just the interior, not the borders

	hWnd = CreateWindowExA(
#ifdef _DEBUG
	       0,     //ExStyle
#else
	       dedicated ? 0:WS_EX_TOPMOST,     //ExStyle
#endif
	       "SRB2WC",                         //Classname
	       wTitle,                           //Windowname
	       WS_CAPTION|WS_POPUP|WS_SYSMENU,   //dwStyle       //WS_VISIBLE|WS_POPUP for bAppFullScreen
	       0,
	       0,
	       dedicated ? 0:BASEVIDWIDTH,  //GetSystemMetrics(SM_CXSCREEN),
	       dedicated ? 0:BASEVIDHEIGHT,  //GetSystemMetrics(SM_CYSCREEN),
	       NULL,                             //hWnd Parent
	       NULL,                             //hMenu Menu
	       hInstance,
	       NULL);

	return hWnd;
}


static inline BOOL tlErrorMessage(const TCHAR *err)
{
	/* make the cursor visible */
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	//
	// warn user if there is one
	//
	printf("Error %s..\n", err);
	fflush(stdout);

	MessageBox(hWndMain, err, TEXT("ERROR"), MB_OK);
	return FALSE;
}


// ------------------
// Command line stuff
// ------------------
#define         MAXCMDLINEARGS          64
static  char *  myWargv[MAXCMDLINEARGS+1];
static  char    myCmdline[512];

static VOID     GetArgcArgv (LPSTR cmdline)
{
	LPSTR   token;
	size_t  i = 0, len;
	char    cSep = ' ';
	BOOL    bCvar = FALSE, prevCvar = FALSE;

	// split arguments of command line into argv
	strncpy (myCmdline, cmdline, 511);      // in case window's cmdline is in protected memory..for strtok
	len = strlen (myCmdline);

	myargc = 0;
	while (myargc < MAXCMDLINEARGS)
	{
		// get token
		while (myCmdline[i] == cSep)
			i++;
		if (i >= len)
			break;
		token = myCmdline + i;
		if (myCmdline[i] == '"')
		{
			cSep = '"';
			i++;
			if (!prevCvar)    //cvar leave the "" in
				token++;
		}
		else
			cSep = ' ';

		//cvar
		if (myCmdline[i] == '+' && cSep == ' ')   //a + begins a cvarname, but not after quotes
			bCvar = TRUE;
		else
			bCvar = FALSE;

		while (myCmdline[i] &&
				myCmdline[i] != cSep)
			i++;

		if (myCmdline[i] == '"')
		{
			 cSep = ' ';
			 if (prevCvar)
				 i++;       // get ending " quote in arg
		}

		prevCvar = bCvar;

		if (myCmdline + i > token)
		{
			myWargv[myargc++] = token;
		}

		if (!myCmdline[i] || i >= len)
			break;

		myCmdline[i++] = '\0';
	}
	myWargv[myargc] = NULL;

	// m_argv.c uses myargv[], we used myWargv because we fill the arguments ourselves
	// and myargv is just a pointer, so we set it to point myWargv
	myargv = myWargv;
}


static inline VOID MakeCodeWritable(VOID)
{
#ifdef USEASM
	// Disable write-protection of code segment
	DWORD OldRights;
	BYTE *pBaseOfImage = (BYTE *)GetModuleHandle(NULL);
	IMAGE_OPTIONAL_HEADER *pHeader = (IMAGE_OPTIONAL_HEADER *)
		(pBaseOfImage + ((IMAGE_DOS_HEADER*)pBaseOfImage)->e_lfanew +
		sizeof (IMAGE_NT_SIGNATURE) + sizeof (IMAGE_FILE_HEADER));
	if (!VirtualProtect(pBaseOfImage+pHeader->BaseOfCode,pHeader->SizeOfCode,PAGE_EXECUTE_READWRITE,&OldRights))
		I_Error("Could not make code writable\n");
#endif
}




// -----------------------------------------------------------------------------
// HandledWinMain : called by exception handler
// -----------------------------------------------------------------------------
static int WINAPI HandledWinMain(HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPSTR lpCmdLine,
                                 int nCmdShow)
{
	int             i;
	LPSTR          args;

	lpCmdLine = NULL;
	hPrevInstance = NULL;
#ifdef LOGMESSAGES
	// DEBUG!!! - set logstream to NULL to disable debug log
	logstream = INVALID_HANDLE_VALUE;

	logstream = CreateFile (TEXT("log.txt"), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
	                        FILE_ATTRIBUTE_NORMAL, NULL);  //file flag writethrough?
#endif

	// fill myargc,myargv for m_argv.c retrieval of cmdline arguments
	CONS_Printf("GetArgcArgv() ...\n");
	args = GetCommandLineA();
	CONS_Printf("lpCmdLine is '%s'\n", args);
	GetArgcArgv(args);
	// Create a text console window
	OpenTextConsole();

	CONS_Printf("Myargc: %d\n", myargc);
	for (i = 0; i < myargc; i++)
		CONS_Printf("myargv[%d] : '%s'\n", i, myargv[i]);

	// store for later use, will we need it ?
	myInstance = hInstance;

	// open a dummy window, both OpenGL and DirectX need one.
	if ((hWndMain = OpenMainWindow(hInstance,nCmdShow,
				va("SRB2 "VERSIONSTRING))) == (HANDLE)-1)
	{
		tlErrorMessage(TEXT("Couldn't open window"));
		return FALSE;
	}

	// currently starts DirectInput
	CONS_Printf("I_StartupSystem() ...\n");
	I_StartupSystem();
	MakeCodeWritable();

	// startup SRB2
	CONS_Printf("D_SRB2Main() ...\n");
	D_SRB2Main();
	CONS_Printf("Entering main app loop...\n");
	// never return
	D_SRB2Loop();

	// back to Windoze
	return 0;
}

// -----------------------------------------------------------------------------
// Exception handler calls WinMain for catching exceptions
// -----------------------------------------------------------------------------
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR     lpCmdLine,
                    int       nCmdShow)
{
	int Result = -1;
#if 1
	HMODULE h = GetModuleHandleA("kernel32.dll");
	MyFunc pfnIsDebuggerPresent = NULL;
	if (h)
		pfnIsDebuggerPresent = (MyFunc)GetProcAddress(h,"IsDebuggerPresent");
	if (!pfnIsDebuggerPresent || !pfnIsDebuggerPresent())
		LoadLibrary("exchndl.dll");
#endif
#ifdef NO_SEH_MINGW
	__try1(RecordExceptionInfo(GetExceptionInformation()/*, "main thread", lpCmdLine*/))
#else
	__try
#endif
	{
		Result = HandledWinMain (hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
#ifdef NO_SEH_MINGW
	__except1
#else
	__except (RecordExceptionInfo(GetExceptionInformation()/*, "main thread", lpCmdLine*/))
#endif
	{
		SetUnhandledExceptionFilter(EXCEPTION_CONTINUE_SEARCH); //Do nothing here.
	}

	return Result;
}
