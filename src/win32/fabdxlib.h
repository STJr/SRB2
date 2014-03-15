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
/// \brief faB's DirectX library v1.0

#ifndef _H_FABDXLIB_
#define _H_FABDXLIB_

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#ifdef __MINGW32__
//#define NONAMELESSUNION
#endif
#ifdef _MSC_VER
#pragma warning(disable :  4201)
#endif
#include <ddraw.h>
#if (defined (DIRECTDRAW_VERSION) && (DIRECTDRAW_VERSION >= 0x0700))
#undef DUMMYUNIONNAMEN
#endif
// format of function in app called with width,height
typedef BOOL (*APPENUMMODESCALLBACK)(int width, int height, int bpp);


// globals
extern IDirectDraw2*                            DDr2;
extern IDirectDrawSurface*                      ScreenReal;
extern IDirectDrawSurface*                      ScreenVirtual;
extern IDirectDrawPalette*                      DDPalette;

extern BOOL                                     bAppFullScreen;                             // main code might need this to know the current
                                                                                           // fullscreen or windowed state

extern int                                      windowPosX;                             // current position in windowed mode
extern int                                      windowPosY;

extern int                                      ScreenWidth;
extern int                                      ScreenHeight;
extern BOOL                                     ScreenLocked;                   // Screen surface is being locked
extern int                                      ScreenPitch;                    // offset from one line to the next
extern LPBYTE                                   ScreenPtr;                              // memory of the surface

extern BOOL                                     bDX0300;

BOOL    EnumDirectDrawDisplayModes (APPENUMMODESCALLBACK appFunc);
BOOL    CreateDirectDrawInstance (VOID);

int     InitDirectDrawe (HWND appWin, int width, int height, int bpp, int fullScr);
VOID    CloseDirectDraw (VOID);

VOID    ReleaseChtuff (VOID);

VOID    ClearSurface (IDirectDrawSurface* surface, int color);
BOOL    ScreenFlip (int wait);
VOID    TextPrint (int x, int y, LPCSTR message);

VOID    CreateDDPalette (PALETTEENTRY* colorTable);
VOID    DestroyDDPalette (VOID);
VOID    SetDDPalette (PALETTEENTRY* pal);
VOID    RestoreDDPalette(VOID);

VOID    WaitVbl (VOID);

boolean LockScreen (VOID);
VOID    UnlockScreen (VOID);


#endif /* _H_FABDXLIB_ */
