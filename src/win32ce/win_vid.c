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
/// \brief win32 video driver for Doom Legacy

#include "../doomdef.h"

#include <stdlib.h>
#include <stdarg.h>

#include "../d_clisrv.h"
#include "../i_system.h"
#include "../m_argv.h"
#include "../v_video.h"
#include "../st_stuff.h"
#include "../i_video.h"
#include "../z_zone.h"
#include "fabdxlib.h"

#include "win_main.h"
#include "../command.h"
#include "../screen.h"

#ifdef HWRENDER
#include "win_dll.h" // loading the render DLL
#include "../hardware/hw_drv.h" // calling driver init & shutdown
#include "../hardware/hw_main.h" // calling HWR module init & shutdown
#endif

// -------
// Globals
// -------

// this is the CURRENT rendermode!! very important: used by w_wad, and much other code
rendermode_t rendermode = render_soft;

// synchronize page flipping with screen refresh
consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_stretch = {"stretch", "On", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

boolean highcolor;

static BOOL bDIBMode; // means we are using DIB instead of DirectDraw surfaces
static BITMAPINFO* bmiMain = NULL;
static HDC hDCMain = NULL;

// -----------------
// Video modes stuff
// -----------------

#define MAX_EXTRA_MODES 36
static vmode_t extra_modes[MAX_EXTRA_MODES] = {{NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0}};
static char names[MAX_EXTRA_MODES][10];

static int numvidmodes; // total number of DirectDraw display modes
static vmode_t *pvidmodes; // start of videomodes list.
static vmode_t *pcurrentmode; // the current active videomode.
static BOOL bWinParm;
static int WINAPI VID_SetWindowedDisplayMode(viddef_t *lvid, vmode_t *pcurrentmode);

// this holds description of the startup video mode,
// the resolution is 320x200, windowed on the desktop
#define NUMSPECIALMODES  1
static char winmode1[] ="320x200W"; // W to make sure it's the windowed mode
static vmode_t specialmodes[NUMSPECIALMODES] =
{
	{
		NULL,
		winmode1, // hehe
		320, 200, //(200.0/320.0)*(320.0/240.0),
		320, 1, // rowbytes, bytes per pixel
		1, 2, // windowed (TRUE), numpages
		NULL,
		VID_SetWindowedDisplayMode, 0
	}
};

// ------
// Protos
// ------
static void VID_Command_NumModes_f(void);
static void VID_Command_ModeInfo_f(void);
static void VID_Command_ModeList_f(void);
static void VID_Command_Mode_f(void);
static int WINAPI VID_SetDirectDrawMode(viddef_t *lvid, vmode_t *pcurrentmode);
static vmode_t *VID_GetModePtr(int modenum);
static void VID_Init(void);
static BOOL VID_FreeAndAllocVidbuffer(viddef_t *lvid);

// -----------------
// I_StartupGraphics
// Initialize video mode, setup dynamic screen size variables,
// and allocate screens.
// -----------------
void I_StartupGraphics(void)
{
	if (graphics_started)
		return;

#ifdef HWRENDER
	if (M_CheckParm("-opengl"))
		rendermode = render_opengl;
	else
		rendermode = render_soft;
#endif

	if (dedicated)
		rendermode = render_none;
	else
		VID_Init();

	// register exit code for graphics
	I_AddExitFunc(I_ShutdownGraphics);
	if (!dedicated) graphics_started = true;
}

// ------------------
// I_ShutdownGraphics
// Close the screen, restore previous video mode.
// ------------------
void I_ShutdownGraphics(void)
{
	if (!graphics_started)
		return;

	CONS_Printf("I_ShutdownGraphics()\n");

	//FreeConsole();

	// release windowed startup stuff
	if (hDCMain)
	{
		ReleaseDC(hWndMain, hDCMain);
		hDCMain = NULL;
	}
	if (bmiMain)
	{
		GlobalFree(bmiMain);
		bmiMain = NULL;
	}

#ifdef HWRENDER
	if (rendermode != render_soft)
	{
		HWR_Shutdown(); // free stuff from the hardware renderer
		HWD.pfnShutdown(); // close 3d card display
		Shutdown3DDriver(); // free the driver DLL
	}
#endif

	// free the last video mode screen buffers
	if (vid.buffer)
	{
		GlobalFree(vid.buffer);
		vid.buffer = NULL;
	}

#ifdef HWRENDER
	if (rendermode == render_soft)
#endif
		CloseDirectDraw();

	graphics_started = false;
}

// --------------
// I_UpdateNoBlit
// --------------
void I_UpdateNoBlit(void)
{
	// what is this?
}

// --------------
// I_FinishUpdate
// --------------
void I_FinishUpdate(void)
{
	int i;

	if (rendermode == render_none)
		return;

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	// display a graph of ticrate
	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	//
	if (bDIBMode)
	{
		// paranoia
		if (!hDCMain || !bmiMain || !vid.buffer)
			return;
		// main game loop, still in a window (-win parm)
		SetDIBitsToDevice(hDCMain, 0, 0, 320, 200, 0, 0, 0, 200, vid.buffer, bmiMain,
			DIB_RGB_COLORS);
	}
	else
#ifdef HWRENDER
	if (rendermode != render_soft)
		HWD.pfnFinishUpdate(cv_vidwait.value);
	else
#endif
	{
		// DIRECT DRAW
		// copy virtual screen to real screen
		// can fail when not active (alt-tab)
		if (LockScreen())
		{
			/// \todo use directX blit here!!? a blit might use hardware with access
			/// to main memory on recent hardware, and software blit of directX may be
			/// optimized for p2 or mmx??
			VID_BlitLinearScreen(screens[0], ScreenPtr, vid.width*vid.bpp, vid.height,
				vid.width*vid.bpp, ScreenPitch);

			UnlockScreen();

			// swap screens
			ScreenFlip(cv_vidwait.value);
		}
	}
}

//
// This is meant to be called only by CONS_Printf() while game startup
//
void I_LoadingScreen(LPCSTR msg)
{
	RECT rect;

	// paranoia
	if (!hDCMain || !bmiMain || !vid.buffer)
		return;

	GetClientRect(vid.WndParent, &rect);

	SetDIBitsToDevice(hDCMain, 0, 0, 320, 200, 0, 0, 0, 200, vid.buffer, bmiMain, DIB_RGB_COLORS);

	if (msg)
	{
		if (rect.bottom - rect.top > 32)
			rect.top = rect.bottom - 32; // put msg on bottom of window
		SetBkMode(hDCMain, TRANSPARENT);
		SetTextColor(hDCMain, RGB(0x00, 0x00, 0x00));
		DrawTextA(hDCMain, msg, -1, &rect, DT_WORDBREAK|DT_CENTER);
	}
}

// ------------
// I_ReadScreen
// ------------
void I_ReadScreen(UINT8 *scr)
{
	// DEBUGGING
	if (rendermode != render_soft)
		I_Error("I_ReadScreen: called while in non-software mode");
	VID_BlitLinearScreen(screens[0], scr, vid.width*vid.bpp, vid.height, vid.width*vid.bpp,
		vid.rowbytes);
}

// ------------
// I_SetPalette
// ------------
void I_SetPalette(RGBA_t *palette)
{
	int i;

	if (bDIBMode)
	{
		// set palette in RGBQUAD format, NOT THE SAME ORDER as PALETTEENTRY, grmpf!
		RGBQUAD *pColors;
		pColors = (RGBQUAD *)((char *)bmiMain + bmiMain->bmiHeader.biSize);
		ZeroMemory(pColors, sizeof (RGBQUAD)*256);
		for (i = 0; i < 256; i++, pColors++, palette++)
		{
			pColors->rgbRed = palette->s.red;
			pColors->rgbGreen = palette->s.green;
			pColors->rgbBlue = palette->s.blue;
		}
	}
	else
#ifdef HWRENDER
	 if (rendermode == render_soft)
#endif
	{
		PALETTEENTRY mainpal[256];

		// this clears the 'flag' for each color in palette
		ZeroMemory(mainpal, sizeof mainpal);

		// set palette in PALETTEENTRY format
		for (i = 0; i < 256; i++, palette++)
		{
			mainpal[i].peRed = palette->s.red;
			mainpal[i].peGreen = palette->s.green;
			mainpal[i].peBlue = palette->s.blue;
		}
		SetDDPalette(mainpal);         // set DirectDraw palette
	}
}

//
// return number of video modes in pvidmodes list
//
INT32 VID_NumModes(void)
{
	return numvidmodes - NUMSPECIALMODES; //faB: dont accept the windowed mode 0
}

// return a video mode number from the dimensions
// returns any available video mode if the mode was not found
INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	vmode_t *pv = pvidmodes;
	int modenum = 0;

	// skip the special modes so that it finds only fullscreen modes
	for (; pv && modenum < NUMSPECIALMODES; pv = pv->pnext, ++modenum);
	for (; pv; pv = pv->pnext, ++modenum)
		if (pv->width == (unsigned)w && pv->height == (unsigned)h)
			return modenum;

	// if not found, return the first mode available,
	// preferably a full screen mode (all modes after the 'specialmodes')
	if (numvidmodes > NUMSPECIALMODES)
		return NUMSPECIALMODES; // use first full screen mode

	return 0; // no fullscreen mode, use windowed mode
}

//
// Enumerate DirectDraw modes available
//
static int nummodes = 0;
static BOOL GetExtraModesCallback(int width, int height, int bpp)
{
	CONS_Printf("mode %d x %d x %d bpp\n", width, height, bpp);

	// skip all unwanted modes
	if (highcolor && bpp != 15)
		goto skip;
	if (!highcolor && bpp != 8)
		goto skip;

	if (bpp > 16 || width > MAXVIDWIDTH || height > MAXVIDHEIGHT)
		goto skip;

	// check if we have space for this mode
	if (nummodes >= MAX_EXTRA_MODES)
	{
		CONS_Printf("mode skipped (too many)\n");
		return FALSE;
	}

	// store mode info
	extra_modes[nummodes].pnext = &extra_modes[nummodes+1];
	memset(names[nummodes], 0, 10);
	snprintf(names[nummodes], 9, "%dx%d", width, height);

	extra_modes[nummodes].name = names[nummodes];
	extra_modes[nummodes].width = width;
	extra_modes[nummodes].height = height;

	// exactly, the current FinishUdpate() gets the rowbytes itself after locking the video buffer
	// so for now we put anything here
	extra_modes[nummodes].rowbytes = width;
	extra_modes[nummodes].windowed = false;
	extra_modes[nummodes].misc = 0; // unused
	extra_modes[nummodes].pextradata = NULL;
	extra_modes[nummodes].setmode = VID_SetDirectDrawMode;

	extra_modes[nummodes].numpages = 2; // double-buffer (but this value is unused)

	extra_modes[nummodes].bytesperpixel = (bpp+1)>>3;

	nummodes++;
skip:
	return TRUE;
}

//
// Collect info about DirectDraw display modes we use
//
static inline void VID_GetExtraModes(void)
{
	nummodes = 0;
	EnumDirectDrawDisplayModes(GetExtraModesCallback);

	// add the extra modes (not 320x200) at the start of the mode list (if there are any)
	if (nummodes)
	{
		extra_modes[nummodes-1].pnext = NULL;
		pvidmodes = &extra_modes[0];
		numvidmodes += nummodes;
	}
}

// ---------------
// WindowMode_Init
// Add windowed modes to the start of the list,
// mode 0 is used for windowed console startup (works on all computers with no DirectX)
// ---------------
static void WindowMode_Init(void)
{
	specialmodes[NUMSPECIALMODES-1].pnext = pvidmodes;
	pvidmodes = &specialmodes[0];
	numvidmodes += NUMSPECIALMODES;
}

// *************************************************************************************
// VID_Init
// Initialize Video modes subsystem
// *************************************************************************************
static void VID_Init(void)
{
	vmode_t *pv;
	int iMode;

	// if '-win' is specified on the command line, do not add DirectDraw modes
	bWinParm = M_CheckParm("-win");

	COM_AddCommand("vid_nummodes", VID_Command_NumModes_f);
	COM_AddCommand("vid_modeinfo", VID_Command_ModeInfo_f);
	COM_AddCommand("vid_modelist", VID_Command_ModeList_f);
	COM_AddCommand("vid_mode", VID_Command_Mode_f);

	CV_RegisterVar(&cv_vidwait);
	CV_RegisterVar(&cv_stretch);

	// setup the videmodes list,
	// note that mode 0 must always be VGA mode 0x13
	pvidmodes = pcurrentmode = NULL;
	numvidmodes = 0;

	// store the main window handle in viddef struct
	SetWindowPos(hWndMain, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOSIZE|SWP_NOMOVE);
	vid.WndParent = hWndMain;
	vid.buffer = NULL;

	// we startup in windowed mode using DIB bitmap
	// we will use DirectDraw when switching fullScreen and entering main game loop
	bDIBMode = TRUE;
	bAppFullScreen = FALSE;

#ifdef HWRENDER
	// initialize the appropriate display device
	if (rendermode != render_soft)
	{
		const char *drvname = NULL;

		switch (rendermode)
		{
			case render_opengl:
				drvname = "r_opengl.dll";
				break;
			default:
				I_Error("Unknown hardware render mode");
		}

		// load the DLL
		if (drvname && Init3DDriver(drvname))
		{
			int hwdversion = HWD.pfnGetRenderVersion();
			if (hwdversion != VERSION)
				CONS_Printf("WARNING: This r_opengl version is not supported, use it at your own risk.\n");

			// perform initialisations
			HWD.pfnInit(I_Error);
			// get available display modes for the device
			HWD.pfnGetModeList(&pvidmodes, &numvidmodes);
		}
		else
		{
			switch (rendermode)
			{
				case render_opengl:
					I_Error("Error initializing OpenGL");
				default:
					break;
			}
			rendermode = render_soft;
		}
	}

	if (rendermode == render_soft)
#endif
		if (!bWinParm)
		{
			if (!CreateDirectDrawInstance())
				I_Error("Error initializing DirectDraw");
			// get available display modes for the device
			VID_GetExtraModes();
		}

	// the game boots in 320x200 standard VGA, but
	// we need a highcolor mode to run the game in highcolor
	if (highcolor && !numvidmodes)
		I_Error("Cannot run in highcolor - No 15bit highcolor DirectX video mode found.");

	// add windowed mode at the start of the list, very important!
	WindowMode_Init();

	if (!numvidmodes)
		I_Error("No display modes available.");

	// DEBUG
	for (iMode = 0, pv = pvidmodes; pv; pv = pv->pnext, iMode++)
		CONS_Printf("#%02d: %dx%dx%dbpp (desc: '%s')\n", iMode, pv->width, pv->height,
			pv->bytesperpixel, pv->name);

	// set the startup screen in a window
	VID_SetMode(0);
}

// --------------------------
// VID_SetWindowedDisplayMode
// Display the startup 320x200 console screen into a window on the desktop,
// switching to fullscreen display only when we will enter the main game loop.
// - we can display error message boxes for startup errors
// - we can set the last used resolution only once, when entering the main game loop
// --------------------------
static int WINAPI VID_SetWindowedDisplayMode(viddef_t *lvid, vmode_t *pcurrentmode)
{
	int iScrWidth, iScrHeight, iWinWidth, iWinHeight;

	pcurrentmode = NULL;
#ifdef DEBUG
	CONS_Printf("VID_SetWindowedDisplayMode()\n");
#endif

	lvid->u.numpages = 1; // not used
	lvid->direct = NULL; // DOS remains
	lvid->buffer = NULL;

	// allocate screens
	if (!VID_FreeAndAllocVidbuffer(lvid))
		return -1;

	// lvid->buffer should be NULL here!

	bmiMain = (void *)GlobalAlloc(GPTR, sizeof (BITMAPINFO) + (sizeof (RGBQUAD)*256));
	if (!bmiMain)
		I_Error("VID_SWDM(): No mem");

	// setup a BITMAPINFO to allow copying our video buffer to the desktop,
	// with color conversion as needed
	bmiMain->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	bmiMain->bmiHeader.biWidth = lvid->width;
	bmiMain->bmiHeader.biHeight= -(lvid->height);
	bmiMain->bmiHeader.biPlanes = 1;
	bmiMain->bmiHeader.biBitCount = 8;
	bmiMain->bmiHeader.biCompression = BI_RGB;

	// center window on the desktop
	iScrWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	iScrHeight = GetSystemMetrics(SM_CYFULLSCREEN);

	iWinWidth = lvid->width;
	iWinWidth += GetSystemMetrics(SM_CXFIXEDFRAME) * 2;

	iWinHeight = lvid->height;
	iWinHeight += GetSystemMetrics(SM_CYCAPTION);
	iWinHeight += GetSystemMetrics(SM_CYFIXEDFRAME) * 2;

	if (devparm)
		MoveWindow(hWndMain, (iScrWidth - iWinWidth), (iScrHeight - iWinHeight), iWinWidth, iWinHeight, TRUE);
	else
		MoveWindow(hWndMain, (iScrWidth - iWinWidth)>>1, (iScrHeight - iWinHeight)>>1, iWinWidth, iWinHeight, TRUE);

	SetFocus(hWndMain);
	ShowWindow(hWndMain, SW_SHOW);

	hDCMain = GetDC(hWndMain);
	if (!hDCMain)
		I_Error("VID_SWDM(): GetDC FAILED");

	return 1;
}

// ========================================================================
// Returns a vmode_t from the video modes list, given a video mode number.
// ========================================================================
vmode_t *VID_GetModePtr(int modenum)
{
	vmode_t *pv;

	pv = pvidmodes;
	if (!pv)
		I_Error("VID_error: No video mode found\n");

	while (modenum--)
	{
		pv = pv->pnext;
		if (!pv)
			I_Error("VID_error: Mode not available\n");
	}
	return pv;
}

//
// return the name of a video mode
//
const char *VID_GetModeName(INT32 modenum)
{
	return (VID_GetModePtr(modenum))->name;
}

// ========================================================================
// Sets a video mode
// ========================================================================
INT32 VID_SetMode(INT32 modenum)
{
	int stat;
	vmode_t *pnewmode;
	vmode_t *poldmode;

	if (dedicated)
		return 0;

	CONS_Printf("VID_SetMode(%d)\n", modenum);

	// if mode 0 (windowed) we must not be fullscreen already,
	// if other mode, check it is not mode 0 and existing
	if (modenum >= numvidmodes)
	{
		if (!pcurrentmode)
			modenum = 0; // revert to the default base vid mode
		else
			I_Error("Unknown video mode: %d\n", modenum);
	}
	else if (bAppFullScreen && modenum < NUMSPECIALMODES)
		I_Error("Tried to switch from fullscreen back to windowed mode %d\n", modenum);

	pnewmode = VID_GetModePtr(modenum);

	// dont switch to the same display mode
	if (pnewmode == pcurrentmode)
		return 1;

	// initialize the new mode
	poldmode = pcurrentmode;
	pcurrentmode = pnewmode;

	// initialize vidbuffer size for setmode
	vid.width = pcurrentmode->width;
	vid.height = pcurrentmode->height;
	vid.rowbytes = pcurrentmode->rowbytes;
	vid.bpp = pcurrentmode->bytesperpixel;
	if (modenum) // if not 320x200 windowed mode, it's actually a hack
	{
		if (rendermode == render_opengl)
		{
			// don't accept depth < 16 for OpenGL mode (too much ugly)
			if (cv_scr_depth.value < 16)
				CV_SetValue(&cv_scr_depth,  16);
			vid.bpp = cv_scr_depth.value/8;
			vid.u.windowed = (bWinParm || !cv_fullscreen.value);
			pcurrentmode->bytesperpixel = vid.bpp;
			pcurrentmode->windowed = vid.u.windowed;
		}
	}

	stat = (*pcurrentmode->setmode)(&vid, pcurrentmode);

	if (stat == -1)
		I_Error("Not enough mem for VID_SetMode\n");
	else if (stat == -2)
		I_Error("Couldn't set video mode because it failed the test\n");
	else if (stat == -3)
		I_Error("Couldn't set video mode because it failed the change?\n");
	else if (!stat)
		I_Error("Couldn't set video mode %d (%dx%d %d bits)\n", modenum, vid.width, vid.height, (vid.bpp*8));// hardware could not setup mode
	else
		CONS_Printf(M_GetText("Mode changed to %d (%s)\n"), modenum, pcurrentmode->name);

	vid.modenum = modenum;

	// tell game engine to recalc all tables and realloc buffers based on new values
	vid.recalc = 1;

	if (modenum < NUMSPECIALMODES)
	{
		// we are in startup windowed mode
		bAppFullScreen = FALSE;
		bDIBMode = TRUE;
	}
	else
	{
		// we switch to fullscreen
		bAppFullScreen = TRUE;
		bDIBMode = FALSE;
#ifdef HWRENDER
		if (rendermode != render_soft)
		{
			// purge all patch graphics stored in software format
			//Z_FreeTags (PU_PURGELEVEL, PU_PURGELEVEL+100);
			HWR_Startup();
		}
#endif
	}

	I_RestartSysMouse();
	return 1;
}

// ========================================================================
// Free the video buffer of the last video mode,
// allocate a new buffer for the video mode to set.
// ========================================================================
static BOOL VID_FreeAndAllocVidbuffer(viddef_t *lvid)
{
	const DWORD vidbuffersize = (lvid->width * lvid->height * lvid->bpp * NUMSCREENS);

	// free allocated buffer for previous video mode
	if (lvid->buffer)
		GlobalFree(lvid->buffer);

	// allocate & clear the new screen buffer
	lvid->buffer = GlobalAlloc(GPTR, vidbuffersize);
	if (!lvid->buffer)
		return FALSE;

	ZeroMemory(lvid->buffer, vidbuffersize);
#ifdef DEBUG
	CONS_Printf("VID_FreeAndAllocVidbuffer done, vidbuffersize: %x\n",vidbuffersize);
#endif
	return TRUE;
}

// ========================================================================
// Set video mode routine for DirectDraw display modes
// Out: 1 ok,
//              0 hardware could not set mode,
//     -1 no mem
// ========================================================================
static int WINAPI VID_SetDirectDrawMode(viddef_t *lvid, vmode_t *pcurrentmode)
{
	pcurrentmode = NULL;
#ifdef DEBUG
	CONS_Printf("VID_SetDirectDrawMode...\n");
#endif

	// DD modes do double-buffer page flipping, but the game engine doesn't need this..
	lvid->u.numpages = 2;

	// release ddraw surfaces etc..
	ReleaseChtuff();

	// clean up any old vid buffer lying around, alloc new if needed
	if (!VID_FreeAndAllocVidbuffer(lvid))
		return -1; // no mem

	// should clear video mem here

	// note use lvid->bpp instead of 8...will this be needed? will we support other than 256color
	// in software ?
	if (!InitDirectDrawe(hWndMain, lvid->width, lvid->height, 8, TRUE)) // TRUE currently always full screen
		return 0;               // could not set mode

	// this is NOT used with DirectDraw modes, game engine should never use this directly
	// but rather render to memory bitmap buffer
	lvid->direct = NULL;

	if (!cv_stretch.value && (float)vid.width/vid.height != ((float)BASEVIDWIDTH/BASEVIDHEIGHT))
		vid.height = (int)(vid.width * ((float)BASEVIDHEIGHT/BASEVIDWIDTH));// Adjust the height to match

	return 1;
}

// ========================================================================
//                     VIDEO MODE CONSOLE COMMANDS
// ========================================================================

// vid_nummodes
//
static void VID_Command_NumModes_f(void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}

// vid_modeinfo <modenum>
//
static void VID_Command_ModeInfo_f(void)
{
	vmode_t *pv;
	int modenum;

	if (COM_Argc() != 2)
		modenum = vid.modenum; // describe the current mode
	else
		modenum = atoi(COM_Argv(1)); // the given mode number

	if (modenum >= VID_NumModes() || modenum < NUMSPECIALMODES) // don't accept the windowed mode 0
	{
		CONS_Printf(M_GetText("Video mode not present\n"));
		return;
	}

	pv = VID_GetModePtr(modenum);

	CONS_Printf("%s\n", VID_GetModeName(modenum));
	CONS_Printf(M_GetText("width: %d\nheight: %d\n"),
		pv->width, pv->height);
	if (rendermode == render_soft)
		CONS_Printf(M_GetText("bytes per scanline: %d\nbytes per pixel: %d\nnumpages: %d\n"),
			pv->rowbytes, pv->bytesperpixel, pv->numpages);
}

// vid_modelist
//
static void VID_Command_ModeList_f(void)
{
	int i, nummodes;
	const char *pinfo;
	vmode_t *pv;

	nummodes = VID_NumModes();
	for (i = NUMSPECIALMODES; i <= nummodes; i++)
	{
		pv = VID_GetModePtr(i);
		pinfo = VID_GetModeName(i);

		if (pv->bytesperpixel == 1)
			CONS_Printf("%d: %s\n", i, pinfo);
		else
			CONS_Printf("%d: %s (hicolor)\n", i, pinfo);
	}
}

// vid_mode <modenum>
//
static void VID_Command_Mode_f(void)
{
	int modenum;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("vid_mode <modenum> : set video mode, current video mode %i\n"), vid.modenum);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes() || modenum < NUMSPECIALMODES) // don't accept the windowed mode 0
		CONS_Printf(M_GetText("Video mode not present\n"));
	else
		setmodeneeded = modenum + 1; // request vid mode change
}
