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

#ifdef _WINDOWS

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
#include "../screen.h" // for BASEVID*

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

typedef BOOL (WINAPI *p_IsDebuggerPresent)(VOID);

HWND hWndMain = NULL;
static HCURSOR windowCursor = NULL; // main window cursor

static LPCSTR wClassName = "SRB2WC";

INT appActive = false; // app window is active

#ifdef LOGMESSAGES
FILE *logstream;
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
		wParam <<= 16;
	}

	//I_OutputMsg("MainWndproc: %p,%i,%i,%i",hWnd, message, wParam, (UINT)lParam);

	switch (message)
	{
		case WM_CREATE:
			nodinput = M_CheckParm("-nodinput");
			if (!nodinput && !LoadDirectInput())
				nodinput = true;
			break;

		case WM_ACTIVATEAPP:           // Handle task switching
			appActive = (int)wParam;

			//coming back from alt-tab?  reset the palette.
			if (appActive)
				vid.recalc = true;

			// pause music when alt-tab
			if (appActive  && !paused)
				I_ResumeSong(0);
			else if (!paused)
				I_PauseSong(0);
			{
				HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
				DWORD mode;
				if (ci != INVALID_HANDLE_VALUE && GetFileType(ci) == FILE_TYPE_CHAR && GetConsoleMode(ci, &mode))
					appActive = true;
			}
			InvalidateRect (hWnd, NULL, TRUE);
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

		case WM_QUERYNEWPALETTE:
			RestoreDDPalette();
			return TRUE;

		case WM_PALETTECHANGED:
			if((HWND)wParam != hWnd)
				RestoreDDPalette();
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
					case VK_SHIFT:  ev.data1 = KEY_LSHIFT;  break;
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
				I_GetSysMouseEvents(mouse_keys);
			}
			break;

		case WM_XBUTTONUP:
			if (nodinput)
			{
				ev.type = ev_keyup;
				ev.data1 = KEY_MOUSE1 + 3 + HIWORD(wParam);
				D_PostEvent(&ev);
				return TRUE;
			}
			break;
		case WM_XBUTTONDOWN:
			if (nodinput)
			{
				ev.type = ev_keydown;
				ev.data1 = KEY_MOUSE1 + 3 + HIWORD(wParam);
				D_PostEvent(&ev);
				return TRUE;
			}
			break;
		case WM_MOUSEWHEEL:
			//I_OutputMsg("MW_WHEEL dispatched.\n");
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

		case WM_SYSCOMMAND:
			// Don't allow the keyboard to activate the menu.
			if(wParam == SC_KEYMENU)
				return 0;

			break;

		default:
			break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static inline VOID OpenTextConsole(VOID)
{
	HANDLE ci, co;
	BOOL console;

#ifdef _DEBUG
	console = M_CheckParm("-noconsole") == 0;
#else
	console = M_CheckParm("-console") != 0;
#endif

	dedicated = M_CheckParm("-dedicated") != 0;

	if (M_CheckParm("-detachconsole"))
	{
		if (FreeConsole())
		{
			I_OutputMsg("Detatched console.\n");
			console = TRUE; //lets get back a console
		}
		else
		{
			I_OutputMsg("No console to detatch.\n");
			I_ShowLastError(FALSE);
		}
	}

	if (dedicated || console)
	{
		if (AllocConsole()) //Let get the real console HANDLEs, because Mingw's Bash is bad!
		{
			SetConsoleTitleA("SRB2 Console");
			CONS_Printf(M_GetText("Hello, it's me, SRB2's Console Window\n"));
		}
		else
		{
			I_OutputMsg("We have a console already.\n");
			I_ShowLastError(FALSE);
			return;
		}
	}
	else
		return;

	ci = CreateFile(TEXT("CONIN$") ,               GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ci != INVALID_HANDLE_VALUE)
	{
		HANDLE sih = GetStdHandle(STD_INPUT_HANDLE);
		if (sih != ci)
		{
			I_OutputMsg("Old STD_INPUT_HANDLE: %p\nNew STD_INPUT_HANDLE: %p\n", sih, ci);
			SetStdHandle(STD_INPUT_HANDLE,ci);
		}
		else
			I_OutputMsg("STD_INPUT_HANDLE already set at %p\n", ci);

		if (GetFileType(ci) == FILE_TYPE_CHAR)
		{
#if 0
			const DWORD CM = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT; //default mode but no ENABLE_MOUSE_INPUT
			if (SetConsoleMode(ci,CM))
			{
				I_OutputMsg("Disabled mouse input on the console\n");
			}
			else
			{
				I_OutputMsg("Could not disable mouse input on the console\n");
				I_ShowLastError(FALSE);
			}
#endif
		}
		else
			I_OutputMsg("Handle CONIN$ in not a Console HANDLE\n");
	}
	else
	{
		I_OutputMsg("Could not get a CONIN$ HANDLE\n");
		I_ShowLastError(FALSE);
	}

	co = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (co != INVALID_HANDLE_VALUE)
	{
		HANDLE soh = GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE seh = GetStdHandle(STD_ERROR_HANDLE);
		if (soh != co)
		{
			I_OutputMsg("Old STD_OUTPUT_HANDLE: %p\nNew STD_OUTPUT_HANDLE: %p\n", soh, co);
			SetStdHandle(STD_OUTPUT_HANDLE,co);
		}
		else
			I_OutputMsg("STD_OUTPUT_HANDLE already set at %p\n", co);
		if (seh != co)
		{
			I_OutputMsg("Old STD_ERROR_HANDLE: %p\nNew STD_ERROR_HANDLE: %p\n", seh, co);
			SetStdHandle(STD_ERROR_HANDLE,co);
		}
		else
			I_OutputMsg("STD_ERROR_HANDLE already set at %p\n", co);
	}
	else
		I_OutputMsg("Could not get a CONOUT$ HANDLE\n");
}


//
// Do that Windows initialization stuff...
//
static HWND OpenMainWindow (HINSTANCE hInstance, LPSTR wTitle)
{
	const LONG	styles = WS_CAPTION|WS_POPUP|WS_SYSMENU, exstyles = 0;
	HWND        hWnd;
	WNDCLASSEXA wc;
	RECT		bounds;

	// Set up and register window class
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
	wc.lpfnWndProc   = MainWndproc;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DLICON1));
	windowCursor     = LoadCursor(NULL, IDC_WAIT); //LoadCursor(hInstance, MAKEINTRESOURCE(IDC_DLCURSOR1));
	wc.hCursor       = windowCursor;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = wClassName;

	if (!RegisterClassExA(&wc))
	{
		I_OutputMsg("Error doing RegisterClassExA\n");
		I_ShowLastError(TRUE);
		return INVALID_HANDLE_VALUE;
	}

	// Create a window
	// CreateWindowEx - seems to create just the interior, not the borders

	bounds.left = 0;
	bounds.right = dedicated ? 0 : specialmodes[0].width;
	bounds.top = 0;
	bounds.bottom = dedicated ? 0 : specialmodes[0].height;

	AdjustWindowRectEx(&bounds, styles, FALSE, exstyles);

	hWnd = CreateWindowExA(
	       exstyles,                                 //ExStyle
	       wClassName,                        //Classname
	       wTitle,                            //Windowname
	       styles,    //dwStyle       //WS_VISIBLE|WS_POPUP for bAppFullScreen
	       0,
	       0,
	       bounds.right - bounds.left,        //GetSystemMetrics(SM_CXSCREEN),
	       bounds.bottom - bounds.top,        //GetSystemMetrics(SM_CYSCREEN),
	       NULL,                              //hWnd Parent
	       NULL,                              //hMenu Menu
	       hInstance,
	       NULL);

	if (hWnd == INVALID_HANDLE_VALUE)
	{
		I_OutputMsg("Error doing CreateWindowExA\n");
		I_ShowLastError(TRUE);
	}

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
	LPSTR   tokenstr;
	size_t  i = 0, len;
	char    cSep = ' ';
	BOOL    bCvar = FALSE, prevCvar = FALSE;

	// split arguments of command line into argv
	strlcpy (myCmdline, cmdline, sizeof(myCmdline));      // in case window's cmdline is in protected memory..for strtok
	len = strlen (myCmdline);

	myargc = 0;
	while (myargc < MAXCMDLINEARGS)
	{
		// get token
		while (myCmdline[i] == cSep)
			i++;
		if (i >= len)
			break;
		tokenstr = myCmdline + i;
		if (myCmdline[i] == '"')
		{
			cSep = '"';
			i++;
			if (!prevCvar)    //cvar leave the "" in
				tokenstr++;
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
				i++; // get ending " quote in arg
		}

		prevCvar = bCvar;

		if (myCmdline + i > tokenstr)
		{
			myWargv[myargc++] = tokenstr;
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
#ifdef USEASM // Disable write-protection of code segment
	DWORD OldRights;
	const DWORD NewRights = PAGE_EXECUTE_READWRITE;
	PBYTE pBaseOfImage = (PBYTE)GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER dosH =(PIMAGE_DOS_HEADER)pBaseOfImage;
	PIMAGE_NT_HEADERS ntH = (PIMAGE_NT_HEADERS)(pBaseOfImage + dosH->e_lfanew);
	PIMAGE_OPTIONAL_HEADER oH = (PIMAGE_OPTIONAL_HEADER)
		((PBYTE)ntH + sizeof (IMAGE_NT_SIGNATURE) + sizeof (IMAGE_FILE_HEADER));
	LPVOID pA = pBaseOfImage+oH->BaseOfCode;
	SIZE_T pS = oH->SizeOfCode;
#if 1 // try to find the text section
	PIMAGE_SECTION_HEADER ntS = IMAGE_FIRST_SECTION (ntH);
	WORD s;
	for (s = 0; s < ntH->FileHeader.NumberOfSections; s++)
	{
		if (memcmp (ntS[s].Name, ".text\0\0", 8) == 0)
		{
			pA = pBaseOfImage+ntS[s].VirtualAddress;
			pS = ntS[s].Misc.VirtualSize;
			break;
		}
	}
#endif

	if (!VirtualProtect(pA,pS,NewRights,&OldRights))
		I_Error("Could not make code writable\n");
#endif
}

// -----------------------------------------------------------------------------
// HandledWinMain : called by exception handler
// -----------------------------------------------------------------------------
static int WINAPI HandledWinMain(HINSTANCE hInstance)
{
	LPSTR          args;

#ifdef LOGMESSAGES
	// DEBUG!!! - set logstream to NULL to disable debug log
	// Replace WIN32 filehandle with standard C calls, because WIN32 doesn't handle \n properly.
	logstream = fopen(va("%s"PATHSEP"%s", srb2home, "log.txt"), "wt");
#endif

	// fill myargc,myargv for m_argv.c retrieval of cmdline arguments
	args = GetCommandLineA();
	CONS_Printf("Command line arguments: '%s'\n", args);
	GetArgcArgv(args);
	// Create a text console window
	OpenTextConsole();

#ifdef _DEBUG
	{
		int i;
		CONS_Printf("Myargc: %d\n", myargc);
		for (i = 0; i < myargc; i++)
			CONS_Printf("myargv[%d] : '%s'\n", i, myargv[i]);
	}
#endif

	// open a dummy window, both OpenGL and DirectX need one.
	if ((hWndMain = OpenMainWindow(hInstance, va("SRB2 "VERSIONSTRING))) == INVALID_HANDLE_VALUE)
	{
		tlErrorMessage(TEXT("Couldn't open window"));
		return FALSE;
	}

	// currently starts DirectInput
	CONS_Printf("I_StartupSystem() ...\n");
	I_StartupSystem();
	MakeCodeWritable();

	// startup SRB2
	CONS_Printf("Setting up SRB2...\n");
	D_SRB2Main();
	CONS_Printf("Entering main game loop...\n");
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

#if 0
	// Win95 and NT <4 don't have this, so link at runtime.
	p_IsDebuggerPresent pfnIsDebuggerPresent = (p_IsDebuggerPresent)GetProcAddress(GetModuleHandleA("kernel32.dll"),"IsDebuggerPresent");
#endif

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

#if 0
#ifdef BUGTRAP
	// Try BugTrap first.
	if((!pfnIsDebuggerPresent || !pfnIsDebuggerPresent()) && InitBugTrap())
		Result = HandledWinMain(hInstance);
	else
	{
#endif
		// Try Dr MinGW's exception handler.
		if (!pfnIsDebuggerPresent || !pfnIsDebuggerPresent())
#endif
			LoadLibraryA("exchndl.dll");

#ifndef __MINGW32__
		prevExceptionFilter = SetUnhandledExceptionFilter(RecordExceptionInfo);
#endif

		Result = HandledWinMain(hInstance);
#ifdef BUGTRAP
	}	// BT failure clause.

	// This is safe even if BT didn't start.
	ShutdownBugTrap();
#endif

	return Result;
}
#endif //_WINDOWS
