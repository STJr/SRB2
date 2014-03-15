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
/// \brief Win32 Sharing

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <stdio.h>

extern HINSTANCE myInstance;
extern HWND hWndMain;

// debugging CONS_Printf to file
#ifdef LOGMESSAGES
extern HANDLE logstream;
#endif
extern int appActive;

// the MIDI callback is another thread, and Midi volume is delayed here in window proc
VOID I_SetMidiChannelVolume(DWORD dwChannel, DWORD dwVolumePercent);
extern DWORD dwVolumePercent;

VOID I_GetSysMouseEvents(int mouse_state);
extern unsigned int MSHWheelMessage;

extern BOOL nodinput;
extern BOOL win9x;

//faB: midi channel Volume set is delayed by the MIDI stream callback thread, see win_snd.c
#define WM_MSTREAM_UPDATEVOLUME (WM_USER + 101)

// defined in win_sys.c
VOID I_BeginProfile(VOID); //for timing code
DWORD I_EndProfile(VOID);

VOID I_GetLastErrorMsgBox(VOID);
VOID I_LoadingScreen (LPCSTR msg);

void I_RestartSysMouse(void);
void I_DoStartupMouse(void);
BOOL I_SaveMemToFile(const void *pData, size_t iLength, const char *sFileName);
