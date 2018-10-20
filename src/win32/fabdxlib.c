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
//
//-----------------------------------------------------------------------------
/// \file
/// \brief faB's DirectX library v1.0
///	- converted to C for Doom Legacy

#include "../doomdef.h"

#ifdef _WINDOWS

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <windowsx.h>
#include "../i_system.h"
#include "dx_error.h"

#include "fabdxlib.h"

#define NT4COMPAT   //always defined, always compatible


// globals

IDirectDraw2*              DDr2 = NULL;
IDirectDrawSurface*        ScreenReal = NULL;    // DirectDraw primary surface
IDirectDrawSurface*        ScreenVirtual = NULL; // DirectDraw back surface
IDirectDrawPalette*        DDPalette = NULL;     // The primary surface palette
static IDirectDrawClipper *windclip = NULL;      // clipper for windowed mode

BOOL                       bAppFullScreen;       // true for fullscreen exclusive mode,

int                        windowPosX = 0;       // current position in windowed mode
int                        windowPosY = 0;

int                        ScreenWidth;
int                        ScreenHeight;
BOOL                       ScreenLocked;         // Screen surface is being locked
int                        ScreenPitch;          // offset from one line to the next
LPBYTE                     ScreenPtr;            // memory of the surface


//
// CreateNewSurface
//
static inline IDirectDrawSurface* CreateNewSurface(int dwWidth,
                                                   int dwHeight,
                                                   int dwSurfaceCaps)
{
	DDSURFACEDESC       ddsd;
	HRESULT             hr;
	LPDIRECTDRAWSURFACE psurf;

	ZeroMemory(&ddsd, sizeof (ddsd));
	ddsd.dwSize = sizeof (ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;

	ddsd.ddsCaps.dwCaps = dwSurfaceCaps;

	ddsd.dwHeight = dwHeight;
	ddsd.dwWidth = dwWidth;

	hr = IDirectDraw2_CreateSurface (DDr2, &ddsd, &psurf, NULL);

	if (hr == DD_OK)
	{
		//DDCOLORKEY ddck;
		IDirectDrawSurface_Restore(psurf);

		//hr = IDirectDrawSurface_GetColorKey(DDCKEY_SRCBLT, &ddck);
		//psurf->SetColorKey(DDCKEY_SRCBLT, &ddck);
	}
	else
		psurf = NULL;

	return psurf;
}

//
// wow! from 320x200x8 up to 1600x1200x32 thanks Banshee! :)
//
static HRESULT WINAPI myEnumModesCallback (LPDDSURFACEDESC surf, LPVOID lpContext)
{
	APPENUMMODESCALLBACK pfnContext = lpContext;

	if (pfnContext) pfnContext(surf->dwWidth,
		surf->dwHeight,surf->ddpfPixelFormat.
#ifdef DUMMYUNIONNAMEN
		DUMMYUNIONNAMEN(1).
#endif
		dwRGBBitCount
		);

		/*I_OutputMsg("%dx%dx%d bpp %d refresh\n",
		surf->dwWidth,
		surf->dwHeight,
		surf->ddpfPixelFormat.dwRGBBitCount,
	surf->dwRefreshRate);*/

	return  DDENUMRET_OK;
}


//
// Application call here to enumerate display modes
//
BOOL EnumDirectDrawDisplayModes (APPENUMMODESCALLBACK appFunc)
{
	LPVOID lpappFunc = appFunc;

	if (DDr2 == NULL)
		return FALSE;

	// enumerate display modes
	// Carl: DirectX 3.x apparently does not support VGA modes. Who cares. :)
	// faB: removed DDEDM_REFRESHRATES, detects too many modes, plus we don't care of refresh rate.
	if (bDX0300)
		IDirectDraw2_EnumDisplayModes (DDr2, 0 /*| DDEDM_REFRESHRATES*/,
		                               NULL, lpappFunc, myEnumModesCallback);
	else
		IDirectDraw2_EnumDisplayModes (DDr2, DDEDM_STANDARDVGAMODES /*| DDEDM_REFRESHRATES*/,
		                               NULL, lpappFunc, myEnumModesCallback);
	return TRUE;
}

static HINSTANCE DDrawDLL = NULL;
typedef HRESULT(WINAPI *DDCreate)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);
static DDCreate pfnDirectDrawCreate = NULL;

static inline BOOL LoadDirectDraw(VOID)
{
	// load ddraw.dll
	DDrawDLL = LoadLibraryA("DDRAW.DLL");
	if (DDrawDLL == NULL)
		return false;
	pfnDirectDrawCreate = (DDCreate)GetProcAddress(DDrawDLL, "DirectDrawCreate");
	if (pfnDirectDrawCreate == NULL)
		return false;
	return true;
}

static inline VOID UnLoadDirectDraw(VOID)
{
	if (!DDrawDLL)
		return;
	FreeLibrary(DDrawDLL);
	pfnDirectDrawCreate = NULL;
	DDrawDLL = NULL;
}

//
// Create the DirectDraw object for later
//
BOOL CreateDirectDrawInstance (VOID)
{
	HRESULT hr;
	IDirectDraw* DDr;
	IDirectDraw** rp = &DDr;
	IDirectDraw2** rp2 = &DDr2;
	LPVOID *tp = (LPVOID *)rp2;

	if (!LoadDirectDraw())
		return FALSE;
	//
	// create an instance of DirectDraw object
	//
	if (FAILED(hr = pfnDirectDrawCreate(NULL, rp, NULL)))
		I_Error("DirectDrawCreate FAILED: %s", DXErrorToString(hr));

	// change interface to IDirectDraw2
	if (FAILED(hr = IDirectDraw_QueryInterface(DDr, &IID_IDirectDraw2, tp)))
		I_Error("Failed to query DirectDraw2 interface: %s", DXErrorToString(hr));

	// release the interface we don't need
	IDirectDraw_Release (DDr);
	return TRUE;
}


//
// - returns true if DirectDraw was initialized properly
//
int  InitDirectDrawe (HWND appWin, int width, int height, int bpp, int fullScr)
{
	DDSURFACEDESC ddsd; // DirectDraw surface description for allocating
	DDSCAPS       ddscaps;
	HRESULT       ddrval;

	DWORD         dwStyle;
	RECT          rect;

	// enumerate directdraw devices
	//if (FAILED(DirectDrawEnumerate (myEnumDDDevicesCallback, NULL)))
	//      I_Error("Error with DirectDrawEnumerate");

	if (!DDr2)
		CreateDirectDrawInstance();

	// remember what screen mode we are in
	bAppFullScreen = fullScr;
	ScreenHeight = height;
	ScreenWidth = width;

	if (bAppFullScreen)
	{
		// Change window attributes
		dwStyle = WS_POPUP | WS_VISIBLE;
		SetWindowLong (appWin, GWL_STYLE, dwStyle);
		SetWindowPos(appWin, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE |
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

		// Get exclusive mode
		ddrval = IDirectDraw2_SetCooperativeLevel(DDr2, appWin, DDSCL_EXCLUSIVE |
		                                          DDSCL_FULLSCREEN |
		                                          DDSCL_ALLOWREBOOT);
		if (ddrval != DD_OK)
			I_Error("SetCooperativeLevel FAILED: %s\n", DXErrorToString(ddrval));

		// Switch from windows desktop to fullscreen

#ifdef NT4COMPAT
		ddrval = IDirectDraw2_SetDisplayMode(DDr2, width, height, bpp, 0, 0);
#else
		ddrval = IDirectDraw2_SetDisplayMode(DDr2, width, height, bpp, 0, DDSDM_STANDARDVGAMODE);
#endif
		if (ddrval != DD_OK)
			I_Error("SetDisplayMode FAILED: %s\n", DXErrorToString(ddrval));

		// This is not really needed, except in certain cases. One case
		// is while using MFC. When the desktop is initally at 16bpp, a mode
		// switch to 8bpp somehow causes the MFC window to not fully initialize
		// and a CreateSurface will fail with DDERR_NOEXCLUSIVEMODE. This will
		// ensure that the window is initialized properly after a mode switch.

		ShowWindow(appWin, SW_SHOW);

		// Create the primary surface with 1 back buffer. Always zero the
		// DDSURFACEDESC structure and set the dwSize member!

		ZeroMemory(&ddsd, sizeof (ddsd));
		ddsd.dwSize = sizeof (ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

		// for fullscreen we use page flipping, for windowed mode, we blit the hidden surface to
		// the visible surface, in both cases we have a visible (or 'real') surface, and a hidden
		// (or 'virtual', or 'backbuffer') surface.
		ddsd.dwBackBufferCount = 2;

		ddrval = IDirectDraw2_CreateSurface(DDr2,&ddsd, &ScreenReal, NULL);
		if (ddrval != DD_OK)
			I_Error("CreateSurface Primary Screen FAILED");

		// Get a pointer to the back buffer

		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		ddrval = IDirectDrawSurface_GetAttachedSurface(ScreenReal,&ddscaps, &ScreenVirtual);
		if (ddrval != DD_OK)
			I_Error("GetAttachedSurface FAILED");
	}
	else
	{
		rect.top = 0;
		rect.left = 0;
		rect.bottom = height;
		rect.right = width;

		// Change window attributes

		dwStyle = GetWindowStyle(appWin);
		dwStyle &= ~WS_POPUP;
		dwStyle |= WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;

		SetWindowLong(appWin, GWL_STYLE, dwStyle);

		// Resize the window so that the client area is the requested width/height

		AdjustWindowRectEx(&rect, GetWindowStyle(appWin), GetMenu(appWin) != NULL,
		                   GetWindowExStyle(appWin));

		// Just in case the window was moved off the visible area of the
		// screen.

		SetWindowPos(appWin, NULL, 0, 0, rect.right-rect.left,
		             rect.bottom-rect.top, SWP_NOMOVE | SWP_NOZORDER |
		             SWP_NOACTIVATE);

		SetWindowPos(appWin, HWND_NOTOPMOST, 0, 0, 0, 0,
		             SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

		// Exclusive mode is normal since it's in windowed mode and needs
		// to cooperate with GDI

		ddrval = IDirectDraw2_SetCooperativeLevel(DDr2,appWin, DDSCL_NORMAL);
		if (ddrval != DD_OK)
			I_Error("SetCooperativeLevel FAILED");

		// Always zero the DDSURFACEDESC structure and set the dwSize member!

		ZeroMemory(&ddsd, sizeof (ddsd));
		ddsd.dwSize = sizeof (ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		// Create the primary surface

		ddrval = IDirectDraw2_CreateSurface(DDr2,&ddsd, &ScreenReal, NULL);
		if (ddrval != DD_OK)
			I_Error("CreateSurface Primary Screen FAILED");

		// Create a back buffer for offscreen rendering, this will be used to
		// blt to the primary

		ScreenVirtual = CreateNewSurface(width, height, DDSCAPS_OFFSCREENPLAIN |
		                                 DDSCAPS_SYSTEMMEMORY);
		if (ScreenVirtual == NULL)
			I_Error("CreateSurface Secondary Screen FAILED");

		/// \todo get the desktop bit depth, and build a lookup table
		/// for quick conversions of 8bit color indexes 0-255 to desktop colors
		/// eg: 256 entries of equivalent of palette colors 0-255 in 15,16,24,32 bit format
		/// when blit virtual to real, convert pixels through lookup table..

		// Use a clipper object for clipping when in windowed mode
		// (make sure our drawing doesn't affect other windows)

		ddrval = IDirectDraw2_CreateClipper (DDr2, 0, &windclip, 0);
		if (ddrval != DD_OK)
			I_Error("CreateClipper FAILED");

		// Associate the clipper with the window.
		ddrval = IDirectDrawClipper_SetHWnd (windclip,0, appWin);
		if (ddrval != DD_OK)
			I_Error("Clipper -> SetHWnd  FAILED");

		// Attach the clipper to the surface.
		ddrval = IDirectDrawSurface_SetClipper (ScreenReal,windclip);
		if (ddrval != DD_OK)
			I_Error("PrimaryScreen -> SetClipperClipper  FAILED");
	}

	return TRUE;
}


//
// Free all memory
//
VOID CloseDirectDraw (VOID)
{
	ReleaseChtuff();
	if (DDr2)
	{
		IDirectDraw2_Release(DDr2);
		DDr2 = NULL;
	}
	UnLoadDirectDraw();
}


//
// Release DirectDraw stuff before display mode change
//
VOID ReleaseChtuff (VOID)
{
	if (!DDr2)
		return;
	if (windclip)
	{
		IDirectDrawClipper_Release(windclip);
		windclip = NULL;
	}
	if (DDPalette)
	{
		IDirectDrawPalette_Release(DDPalette);
		DDPalette = NULL;
	}
	// If the app is fullscreen, the back buffer is attached to the
	// primary. Releasing the primary buffer will also release any
	// attached buffers, so explicitly releasing the back buffer is not
	// necessary.

	if (!bAppFullScreen && ScreenVirtual)
	{
		IDirectDrawSurface_Release(ScreenVirtual);   // release hidden surface
		ScreenVirtual = NULL;
	}
	if (ScreenReal)
	{
		IDirectDrawSurface_Release(ScreenReal);                      // and attached backbuffers for bAppFullScreen mode
		ScreenReal = NULL;
	}
}


//
// Clear the surface to color
//
VOID ClearSurface(IDirectDrawSurface* surface, int color)
{
	DDBLTFX ddbltfx;

	// Use the blter to do a color fill to clear the back buffer
	ddbltfx.dwSize = sizeof (ddbltfx);
	ddbltfx.
#ifdef DUMMYUNIONNAMEN
	 DUMMYUNIONNAMEN(5).
#endif
	 dwFillColor = color;
	IDirectDrawSurface_Blt(surface,NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);

}

//
// Flip the real page with virtual page
// - in bAppFullScreen mode, do page flipping
// - in windowed mode, copy the hidden surface to the visible surface
//
// waitflip : if not 0, wait for page flip to end
BOOL ScreenFlip(int waitflip)
{
	HRESULT hr;
	RECT rect;

	if (bAppFullScreen)
	{
		//hr = IDirectDrawSurface_GetFlipStatus (ScreenReal, DDGFS_);

		// In full-screen exclusive mode, do a hardware flip.
		hr = IDirectDrawSurface_Flip(ScreenReal, NULL, DDFLIP_WAIT | (waitflip ? 0 : DDFLIP_NOVSYNC));   //return immediately

		// If the surface was lost, restore it.
		if (hr == DDERR_SURFACELOST)
		{
			IDirectDrawSurface_Restore(ScreenReal);

			// The restore worked, so try the flip again.
			hr = IDirectDrawSurface_Flip(ScreenReal, 0, DDFLIP_WAIT | (waitflip ? 0 : DDFLIP_NOVSYNC));
		}
	}
	else
	{
		rect.left = windowPosX;
		rect.top = windowPosY;
		rect.right = windowPosX + ScreenWidth - 1;
		rect.bottom = windowPosY + ScreenHeight - 1;

		// Copy the back buffer to front.
		hr = IDirectDrawSurface_Blt(ScreenReal, &rect, ScreenVirtual, 0, DDBLT_WAIT, 0);

		if (hr != DD_OK)
		{
			// If the surfaces were lost, restore them.
			if (IDirectDrawSurface_IsLost(ScreenReal) == DDERR_SURFACELOST)
				IDirectDrawSurface_Restore(ScreenReal);

			if (IDirectDrawSurface_IsLost(ScreenVirtual) == DDERR_SURFACELOST)
				IDirectDrawSurface_Restore(ScreenVirtual);

			// Retry the copy.
			hr = IDirectDrawSurface_Blt(ScreenReal,&rect, ScreenVirtual, 0, DDBLT_WAIT, 0);
		}
	}

	if (hr != DD_OK)
		I_Error("ScreenFlip() : couldn't Flip surfaces because %s", DXErrorToString(hr));

	return FALSE;
}

//
// Print a text to the surface
//
VOID TextPrint(int x, int y, LPCSTR message)
{
	HRESULT hr;
	HDC hdc = NULL;

	// Get the device context handle.
	hr = IDirectDrawSurface_GetDC(ScreenVirtual,&hdc);
	if (hr != DD_OK)
		return;

	// Write the message.
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 255));
	TextOutA(hdc, x, y, message, (int)strlen(message));

	// Release the device context.
	hr = IDirectDrawSurface_ReleaseDC(ScreenVirtual,hdc);
}

//
// Lock surface before multiple drawings by hand, for speed
//
boolean LockScreen(VOID)
{
	DDSURFACEDESC ddsd;
	HRESULT ddrval;

	ZeroMemory(&ddsd, sizeof (ddsd));
	ddsd.dwSize = sizeof (ddsd);

	// attempt to Lock the surface
	ddrval = IDirectDrawSurface_Lock(ScreenVirtual, NULL, &ddsd, DDLOCK_WAIT, NULL);

	// Always, always check for errors with DirectX!
	// If the surface was lost, restore it.
	if (ddrval == DDERR_SURFACELOST)
	{
		ddrval = IDirectDrawSurface_Restore(ScreenReal);

		// now retry to get the lock
		ddrval = IDirectDrawSurface_Lock(ScreenVirtual, NULL, &ddsd, DDLOCK_WAIT, NULL);
	}

	if (ddrval == DD_OK)
	{
		ScreenLocked = TRUE;
		ScreenPtr    = (LPBYTE)ddsd.lpSurface;
		ScreenPitch = ddsd.
#ifdef DUMMYUNIONNAMEN
		 DUMMYUNIONNAMEN(1).
#endif
		 lPitch;
	}
	else
	{
		ScreenLocked = FALSE;
		ScreenPtr = NULL;
		ScreenPitch = 0;
		//I_Error("LockScreen() : couldn't restore the surface.");
		return false;
	}
	return true;
}

//
// Unlock surface
//
VOID UnlockScreen(VOID)
{
	if (DD_OK != IDirectDrawSurface_Unlock(ScreenVirtual,NULL))
		I_Error("Couldn't UnLock the renderer!");

	ScreenLocked = FALSE;
	ScreenPtr    = NULL;
	ScreenPitch = 0;
}

// Blit virtual screen to real screen
//faB: note: testing 14/03/1999, see if it is faster than memcopy of virtual to
/*
static LPDIRECTDRAWSURFACE lpDDS = NULL;
VOID BlitScreen(VOID)
{
	HRESULT hr;

	if (!lpDDS)
		I_Error("lpDDS NULL");

	hr = IDirectDrawSurface_BltFast(ScreenVirtual,
	                                    0, 0,    // Upper left xy of destination
	                                    lpDDS, // Source surface
	                                    NULL,        // Source rectangle = entire surface
	                                    DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
	if (FAILED(hr))
		I_Error("BltFast FAILED");
}

VOID MakeScreen(int width, int height, BYTE* lpSurface)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;

	// Initialize the surface description.
	ZeroMemory (&ddsd, sizeof ddsd);
	ZeroMemory (&ddsd.ddpfPixelFormat, sizeof (DDPIXELFORMAT));
	ddsd.dwSize = sizeof ddsd;
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | //DDSD_LPSURFACE |
	               DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
	ddsd.dwWidth = width;
	ddsd.dwHeight= height;
	ddsd.lPitch  = width;
	ddsd.lpSurface = lpSurface;
	ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;

	// Set up the pixel format for 8-bit
	ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags= DDPF_RGB | DDPF_PALETTEINDEXED8;
	ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

	//
	ddsd.ddpfPixelFormat.dwRGBBitCount = (DWORD)DEPTH*8;
	ddsd.ddpfPixelFormat.dwRBitMask    = 0x00FF0000;
	ddsd.ddpfPixelFormat.dwGBitMask    = 0x0000FF00;
	ddsd.ddpfPixelFormat.dwBBitMask    = 0x000000FF;

	// Create the surface
	hr = IDirectDraw2_CreateSurface(DDr2, &ddsd, &lpDDS, NULL);
	if (FAILED(hr))
		I_Error("MakeScreen FAILED: %s",DDError(hr));
	//ddsd.lpSurface = lpSurface;
}
*/

//
// Create a palette object
//
VOID CreateDDPalette (PALETTEENTRY* colorTable)
{
	HRESULT  ddrval;
	ddrval = IDirectDraw2_CreatePalette(DDr2,DDPCAPS_8BIT|DDPCAPS_ALLOW256, colorTable, &DDPalette, NULL);
	if (ddrval != DD_OK)
		I_Error("couldn't CreatePalette");
};


//
// Free the palette object
//
VOID DestroyDDPalette (VOID)
{
	if (DDPalette)
	{
		IDirectDrawPalette_Release(DDPalette);
		DDPalette = NULL;
	}
}

//
// Set a a full palette of 256 PALETTEENTRY entries
//
VOID SetDDPalette(PALETTEENTRY* pal)
{
	// create palette first time
	if (DDPalette == NULL)
		CreateDDPalette(pal);
	else
		IDirectDrawPalette_SetEntries(DDPalette, 0, 0, 256, pal);
	// setting the same palette to the same surface again does not increase
	// the reference count
	IDirectDrawSurface_SetPalette(ScreenReal, DDPalette);
}

//
// Wait for vsync, gross
//
VOID WaitVbl(VOID)
{
	IDirectDraw2_WaitForVerticalBlank(DDr2, DDWAITVB_BLOCKBEGIN, NULL);
}


//
// Restore the palette. Useful when we regain focus.
//
VOID RestoreDDPalette(VOID)
{
	if (DDPalette)
		IDirectDrawSurface_SetPalette(ScreenReal, DDPalette);
}
#endif
