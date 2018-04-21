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
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 graphics stuff for SDL

#include <stdlib.h>

#ifndef _WIN32_WCE
#include <signal.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#if SDL_VERSION_ATLEAST(1,2,9) && defined (_arch_dreamcast)
#define HAVE_DCSDL
#include "SDL_dreamcast.h"
#endif

#if SDL_VERSION_ATLEAST(1,2,9) && defined (GP2X)
#define HAVE_GP2XSDL
#include "SDL_gp2x.h"
#endif

#if SDL_VERSION_ATLEAST(1,3,0)
#define SDLK_EQUALS SDLK_KP_EQUALSAS400
#define SDLK_LMETA SDLK_LGUI
#define SDLK_RMETA SDLK_RGUI
#else
#define HAVE_SDLMETAKEYS
#endif

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef HAVE_IMAGE
#include "SDL_image.h"
#elseif !(defined (DC) || defined (_WIN32_WCE) || defined (PSP) || defined(GP2X))
#define LOAD_XPM //I want XPM!
#include "IMG_xpm.c" //Alam: I don't want to add SDL_Image.dll/so
#define HAVE_IMAGE //I have SDL_Image, sortof
#endif

#ifdef HAVE_IMAGE
#include "SDL_icon.xpm"
#endif

#include "../doomdef.h"

#if defined (_WIN32) && !defined (_XBOX)
#include "SDL_syswm.h"
#endif

#ifdef _arch_dreamcast
#include <conio/conio.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
//#include "SRB2DC/VMU.xbm"
//#include <dc/pvr.h>
//#define malloc pvr_mem_malloc
//#define free pvr_mem_free
#endif

#if defined (_XBOX) && defined (__GNUC__)
#include <openxdk/debug.h>
#endif

#include "../doomstat.h"
#include "../i_system.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../m_menu.h"
#include "../d_main.h"
#include "../s_sound.h"
#include "../i_sound.h"  // midi pause/unpause
#include "../i_joy.h"
#include "../st_stuff.h"
#include "../g_game.h"
#include "../i_video.h"
#include "../console.h"
#include "../command.h"
#include "sdlmain.h"
#ifdef HWRENDER
#include "../hardware/hw_main.h"
#include "../hardware/hw_drv.h"
// For dynamic referencing of HW rendering functions
#include "hwsym_sdl.h"
#include "ogl_sdl.h"
#endif

#ifdef REMOTE_DEBUGGING
#ifdef _WII
#include <debug.h>
#endif
#endif

#ifdef HAVE_FILTER
#define FILTERS
#include "filter/filters.h"
#endif

// maximum number of windowed modes (see windowedModes[][])
#if defined (_WIN32_WCE) || defined (DC) || defined (PSP) || defined(GP2X)
#define MAXWINMODES (1)
#elif defined (WII)
#define MAXWINMODES (8)
#elif defined (_PS3)
#define MAXWINMODES (26)
#else
#define MAXWINMODES (27)
#endif

/**	\brief
*/
static INT32 numVidModes = -1;

/**	\brief
*/
static char vidModeName[33][32]; // allow 33 different modes

rendermode_t rendermode=render_soft;

boolean highcolor = false;

// synchronize page flipping with screen refresh
#if defined(DC) || (defined(GP2X) && !defined(HAVE_GP2XSDL))
consvar_t cv_vidwait = {"vid_wait", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
static consvar_t cv_stretch = {"stretch", "Off", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

UINT8 graphics_started = 0; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in VID_PrepareModeList
boolean allow_fullscreen = false;
static SDL_bool disable_fullscreen = SDL_FALSE;
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value
static SDL_bool disable_mouse = SDL_FALSE;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && SDL_GetAppState() & SDL_APPACTIVE)
#define MOUSE_MENU false //(!disable_mouse && cv_usemouse.value && menuactive && !USE_FULLSCREEN)
#define MOUSEBUTTONS_MAX MOUSEBUTTONS

// first entry in the modelist which is not bigger than MAXVIDWIDTHxMAXVIDHEIGHT
static      INT32          firstEntry = 0;

// SDL vars
#ifndef HWRENDER //[segabor] !!! I had problem compiling this source with gcc 3.3
static      SDL_Surface *vidSurface = NULL;
#endif
static      SDL_Surface *bufSurface = NULL;
static      SDL_Surface *icoSurface = NULL;
static      SDL_Color    localPalette[256];
static      SDL_Rect   **modeList = NULL;
#ifdef DC
static       Uint8       BitsPerPixel = 15;
#else
static       Uint8       BitsPerPixel = 16;
#endif
static       Uint16      realwidth = BASEVIDWIDTH;
static       Uint16      realheight = BASEVIDHEIGHT;
#ifdef _WIN32_WCE
static const Uint32      surfaceFlagsW = SDL_HWPALETTE; //Can't handle WinCE changing sides
#else
static const Uint32      surfaceFlagsW = SDL_HWPALETTE/*|SDL_RESIZABLE*/;
#endif
#ifdef _PSP
static const Uint32      surfaceFlagsF = SDL_HWSURFACE|SDL_FULLSCREEN;
#else
static const Uint32      surfaceFlagsF = SDL_HWPALETTE|SDL_FULLSCREEN;
#endif
static       SDL_bool    mousegrabok = SDL_TRUE;
#define HalfWarpMouse(x,y) SDL_WarpMouse((Uint16)(x/2),(Uint16)(y/2))
#if defined (_WIN32_WCE) || defined (DC) || defined (PSP) || defined(GP2X)
static       SDL_bool    videoblitok = SDL_TRUE;
#else
static       SDL_bool    videoblitok = SDL_FALSE;
#endif
static       SDL_bool    exposevideo = SDL_FALSE;

// windowed video modes from which to choose from.
static INT32 windowedModes[MAXWINMODES][2] =
{
#if !(defined (_WIN32_WCE) || defined (DC) || defined (PSP) || defined (GP2X))
#ifndef WII
#ifndef _PS3
	{1920,1200}, // 1.60,6.00
#endif
	{1680,1050}, // 1.60,5.25
	{1600,1200}, // 1.33,5.00
	{1600,1000}, // 1.60,5.00
	{1536,1152}, // 1.33,4.80
	{1536, 960}, // 1.60,4.80
	{1440, 900}, // 1.60,4.50
	{1400,1050}, // 1.33,4.375
	{1400, 875}, // 1.60,4.375
	{1360, 850}, // 1.60,4.25
	{1280, 960}, // 1.33,4.00
	{1280, 800}, // 1.60,4.00
	{1152, 864}, // 1.33,3.60
	{1120, 700}, // 1.60,3.50
	{1024, 768}, // 1.33,3.20
	{ 960, 720}, // 1.33,3.00
	{ 960, 600}, // 1.60,3.00
	{ 800, 600}, // 1.33,2.50
	{ 800, 500}, // 1.60,2.50
#endif
	{ 640, 480}, // 1.33,2.00
	{ 640, 400}, // 1.60,2.00
	{ 576, 432}, // 1.33,1.80
	{ 512, 384}, // 1.33,1.60
	{ 416, 312}, // 1.33,1.30
	{ 400, 300}, // 1.33,1.25
	{ 320, 240}, // 1.33,1.00
#endif
	{ 320, 200}, // 1.60,1.00
};

static void SDLSetMode(INT32 width, INT32 height, INT32 bpp, Uint32 flags)
{
	const char *SDLVD = I_GetEnv("SDL_VIDEODRIVER");
#ifdef _WIN32_WCE
	if (bpp < 16)
		bpp = 16; // 256 mode poo
#endif
#ifdef _WII
	bpp = 16; // 8-bit mode poo
#endif
#ifdef DC
	if (bpp < 15)
		bpp = 15;
	height = 240;
#endif
#ifdef PSP
	bpp = 16;
#endif
#ifdef GP2X
	bpp = 16;
#ifdef HAVE_GP2XSDL
	height = 240;
#endif
#endif
#ifdef FILTERS
	bpp = Setupf2x(width, height, bpp);
#endif
	if (SDLVD && strncasecmp(SDLVD,"glSDL",6) == 0) //for glSDL videodriver
		vidSurface = SDL_SetVideoMode(width, height,0,SDL_DOUBLEBUF);
#ifdef _WII // don't want it to use HWSURFACE, so make it first here
	else if (SDL_VideoModeOK(width, height, bpp, flags|SDL_SWSURFACE|SDL_DOUBLEBUF) >= bpp) // SDL Wii uses double buffering
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_SWSURFACE|SDL_DOUBLEBUF);
#endif
	else if (cv_vidwait.value && videoblitok && SDL_VideoModeOK(width, height, bpp, flags|SDL_HWSURFACE|SDL_DOUBLEBUF) >= bpp)
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_HWSURFACE|SDL_DOUBLEBUF);
	else if (videoblitok && SDL_VideoModeOK(width, height, bpp, flags|SDL_HWSURFACE) >= bpp)
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_HWSURFACE);
	else if (SDL_VideoModeOK(width, height, bpp, flags|SDL_SWSURFACE) >= bpp)
		vidSurface = SDL_SetVideoMode(width, height, bpp, flags|SDL_SWSURFACE);
	else return;
	realwidth = (Uint16)width;
	realheight = (Uint16)height;
#ifdef HAVE_DCSDL
	//SDL_DC_SetWindow(320,200);
	SDL_DC_EmulateMouse(SDL_FALSE);
	SDL_DC_EmulateKeyboard(SDL_TRUE);
#endif
#ifdef HAVE_GP2XSDL
	SDL_ShowCursor(SDL_DISABLE); //For GP2X Open2x
#endif
#ifdef FILTERS
	if (vidSurface && preSurface && f2xSurface)
	{
		vid.width = width/2;
		vid.height = height/2;
	}
#endif
}

//
//  Translates the SDL key into SRB2 key
//

static INT32 SDLatekey(SDLKey sym)
{
	INT32 rc = sym + 0x80;

#ifdef _WIN32_WCE
	if (sym == SDLK_KP8)
		sym = SDLK_UP;
	else if (sym == SDLK_KP4)
		sym = SDLK_LEFT;
	else if (sym == SDLK_KP6)
		sym = SDLK_RIGHT;
	else if (sym == SDLK_KP2)
		sym = SDLK_DOWN;
#endif

	switch (sym)
	{
		case SDLK_LEFT:
			rc = KEY_LEFTARROW;
			break;
		case SDLK_RIGHT:
			rc = KEY_RIGHTARROW;
			break;
		case SDLK_DOWN:
			rc = KEY_DOWNARROW;
			break;
		case SDLK_UP:
			rc = KEY_UPARROW;
			break;

		case SDLK_ESCAPE:
			rc = KEY_ESCAPE;
			break;
		case SDLK_SPACE:
			rc = KEY_SPACE;
			break;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			rc = KEY_ENTER;
			break;
		case SDLK_TAB:
			rc = KEY_TAB;
			break;
		case SDLK_F1:
			rc = KEY_F1;
			break;
		case SDLK_F2:
			rc = KEY_F2;
			break;
		case SDLK_F3:
			rc = KEY_F3;
			break;
		case SDLK_F4:
			rc = KEY_F4;
			break;
		case SDLK_F5:
			rc = KEY_F5;
			break;
		case SDLK_F6:
			rc = KEY_F6;
			break;
		case SDLK_F7:
			rc = KEY_F7;
			break;
		case SDLK_F8:
			rc = KEY_F8;
			break;
		case SDLK_F9:
			rc = KEY_F9;
			break;
		case SDLK_F10:
			rc = KEY_F10;
			break;
		case SDLK_F11:
			rc = KEY_F11;
			break;
		case SDLK_F12:
			rc = KEY_F12;
			break;

		case SDLK_BACKSPACE:
			rc = KEY_BACKSPACE;
			break;
		case SDLK_DELETE:
			rc = KEY_DEL;
			break;

		case SDLK_KP_EQUALS: //Alam & Logan: WTF? Mac KB haves one! XD
		case SDLK_PAUSE:
			rc = KEY_PAUSE;
			break;

		case SDLK_EQUALS:
		case SDLK_PLUS:
			rc = KEY_EQUALS;
			break;

		case SDLK_MINUS:
			rc = KEY_MINUS;
			break;

		case SDLK_LSHIFT:
			rc = KEY_LSHIFT;
			break;

		case SDLK_RSHIFT:
			rc = KEY_RSHIFT;
			break;

		case SDLK_CAPSLOCK:
			rc = KEY_CAPSLOCK;
			break;

		case SDLK_LCTRL:
			rc = KEY_LCTRL;
			break;
		case SDLK_RCTRL:
			rc = KEY_RCTRL;
			break;

		case SDLK_LALT:
			rc = KEY_LALT;
			break;
		case SDLK_RALT:
			rc = KEY_RALT;
			break;

		case SDLK_NUMLOCK:
			rc = KEY_NUMLOCK;
			break;
		case SDLK_SCROLLOCK:
			rc = KEY_SCROLLLOCK;
			break;

		case SDLK_PAGEUP:
			rc = KEY_PGUP;
			break;
		case SDLK_PAGEDOWN:
			rc = KEY_PGDN;
			break;
		case SDLK_END:
			rc = KEY_END;
			break;
		case SDLK_HOME:
			rc = KEY_HOME;
			break;
		case SDLK_INSERT:
			rc = KEY_INS;
			break;

		case SDLK_KP0:
			rc = KEY_KEYPAD0;
			break;
		case SDLK_KP1:
			rc = KEY_KEYPAD1;
			break;
		case SDLK_KP2:
			rc = KEY_KEYPAD2;
			break;
		case SDLK_KP3:
			rc = KEY_KEYPAD3;
			break;
		case SDLK_KP4:
			rc = KEY_KEYPAD4;
			break;
		case SDLK_KP5:
			rc = KEY_KEYPAD5;
			break;
		case SDLK_KP6:
			rc = KEY_KEYPAD6;
			break;
		case SDLK_KP7:
			rc = KEY_KEYPAD7;
			break;
		case SDLK_KP8:
			rc = KEY_KEYPAD8;
			break;
		case SDLK_KP9:
			rc = KEY_KEYPAD9;
			break;

		case SDLK_KP_PERIOD:
			rc = KEY_KPADDEL;
			break;
		case SDLK_KP_DIVIDE:
			rc = KEY_KPADSLASH;
			break;
		case SDLK_KP_MULTIPLY:
			rc = '*';
			break;
		case SDLK_KP_MINUS:
			rc = KEY_MINUSPAD;
			break;
		case SDLK_KP_PLUS:
			rc = KEY_PLUSPAD;
			break;

#ifndef _arch_dreamcast
		case SDLK_LSUPER:
#ifdef HAVE_SDLMETAKEYS
		case SDLK_LMETA:
#endif
			rc = KEY_LEFTWIN;
			break;
		case SDLK_RSUPER:
#ifdef HAVE_SDLMETAKEYS
		case SDLK_RMETA:
#endif
			rc = KEY_RIGHTWIN;
			break;

		case SDLK_MENU:
			rc = KEY_MENU;
			break;
#endif

		default:
			if (sym >= SDLK_SPACE && sym <= SDLK_DELETE)
				rc = sym - SDLK_SPACE + ' ';
			else if (sym >= 'A' && sym <= 'Z')
				rc = sym - 'A' + 'a';
			else if (sym)
			{
				I_OutputMsg("Unknown Keycode %i, Name: %s\n",sym, SDL_GetKeyName( sym ));
			}
			else if (!sym) rc = 0;
			break;
	}

	return rc;
}

static void SDLdoUngrabMouse(void)
{
	if (SDL_GRAB_ON == SDL_WM_GrabInput(SDL_GRAB_QUERY))
	{
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}
}

void SDLforceUngrabMouse(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO)
		SDL_WM_GrabInput(SDL_GRAB_OFF);
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}

static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
	INT32 vfBPP;
	const SDL_Surface *VidSur = SDL_GetVideoSurface();

	if (!infoSurface)
		return;

	if (!SurfaceText)
		SurfaceText = M_GetText("Unknown Surface");

	vfBPP = infoSurface->format?infoSurface->format->BitsPerPixel:0;

	CONS_Printf("\x82" "%s\n", SurfaceText);
	CONS_Printf(M_GetText(" %ix%i at %i bit color\n"), infoSurface->w, infoSurface->h, vfBPP);

	if (infoSurface->flags&SDL_HWSURFACE)
		CONS_Printf("%s", M_GetText(" Stored in video memory\n"));
	else if (infoSurface->flags&SDL_OPENGL)
		CONS_Printf("%s", M_GetText(" Stored in an OpenGL context\n"));
	else if (infoSurface->flags&SDL_PREALLOC)
		CONS_Printf("%s", M_GetText(" Uses preallocated memory\n"));
	else
		CONS_Printf("%s", M_GetText(" Stored in system memory\n"));

	if (infoSurface->flags&SDL_ASYNCBLIT)
		CONS_Printf("%s", M_GetText(" Uses asynchronous blits if possible\n"));
	else
		CONS_Printf("%s", M_GetText(" Uses synchronous blits if possible\n"));

	if (infoSurface->flags&SDL_ANYFORMAT)
		CONS_Printf("%s", M_GetText(" Allows any pixel-format\n"));

	if (infoSurface->flags&SDL_HWPALETTE)
		CONS_Printf("%s", M_GetText(" Has exclusive palette access\n"));
	else if (VidSur == infoSurface)
		CONS_Printf("%s", M_GetText(" Has nonexclusive palette access\n"));

	if (infoSurface->flags&SDL_DOUBLEBUF)
		CONS_Printf("%s", M_GetText(" Double buffered\n"));
	else if (VidSur == infoSurface)
		CONS_Printf("%s", M_GetText(" No hardware flipping\n"));

	if (infoSurface->flags&SDL_FULLSCREEN)
		CONS_Printf("%s", M_GetText(" Full screen\n"));
	else if (infoSurface->flags&SDL_RESIZABLE)
		CONS_Printf("%s", M_GetText(" Resizable window\n"));
	else if (VidSur == infoSurface)
		CONS_Printf("%s", M_GetText(" Nonresizable window\n"));

	if (infoSurface->flags&SDL_HWACCEL)
		CONS_Printf("%s", M_GetText(" Uses hardware acceleration blit\n"));
	if (infoSurface->flags&SDL_SRCCOLORKEY)
		CONS_Printf("%s", M_GetText(" Use colorkey blitting\n"));
	if (infoSurface->flags&SDL_RLEACCEL)
		CONS_Printf("%s", M_GetText(" Colorkey RLE acceleration blit\n"));
	if (infoSurface->flags&SDL_SRCALPHA)
		CONS_Printf("%s", M_GetText(" Use alpha blending acceleration blit\n"));

}

static void VID_Command_Info_f (void)
{
	const SDL_VideoInfo *videoInfo;
	videoInfo = SDL_GetVideoInfo(); //Alam: Double-Check
	if (videoInfo)
	{
		CONS_Printf("%s", M_GetText("Video Interface Capabilities:\n"));
		if (videoInfo->hw_available)
			CONS_Printf("%s", M_GetText(" Hardware surfaces\n"));
		if (videoInfo->wm_available)
			CONS_Printf("%s", M_GetText(" Window manager\n"));
		//UnusedBits1  :6
		//UnusedBits2  :1
		if (videoInfo->blit_hw)
			CONS_Printf("%s", M_GetText(" Accelerated blits HW-2-HW\n"));
		if (videoInfo->blit_hw_CC)
			CONS_Printf("%s", M_GetText(" Accelerated blits HW-2-HW with Colorkey\n"));
		if (videoInfo->wm_available)
			CONS_Printf("%s", M_GetText(" Accelerated blits HW-2-HW with Alpha\n"));
		if (videoInfo->blit_sw)
		{
			CONS_Printf("%s", M_GetText(" Accelerated blits SW-2-HW\n"));
			if (!M_CheckParm("-noblit")) videoblitok = SDL_TRUE;
		}
		if (videoInfo->blit_sw_CC)
			CONS_Printf("%s", M_GetText(" Accelerated blits SW-2-HW with Colorkey\n"));
		if (videoInfo->blit_sw_A)
			CONS_Printf("%s", M_GetText(" Accelerated blits SW-2-HW with Alpha\n"));
		if (videoInfo->blit_fill)
			CONS_Printf("%s", M_GetText(" Accelerated Color filling\n"));
		//UnusedBits3  :16
		if (videoInfo->video_mem)
			CONS_Printf(M_GetText(" There is %i KB of video memory\n"), videoInfo->video_mem);
		else
			CONS_Printf("%s", M_GetText(" There no video memory for SDL\n"));
		//*vfmt
	}
	SurfaceInfo(bufSurface, M_GetText("Current Engine Mode"));
#ifdef FILTERS
	SurfaceInfo(preSurface, M_GetText("Prebuffer Mode"));
	SurfaceInfo(f2xSurface, M_GetText("Postbuffer Mode"));
#endif
	SurfaceInfo(vidSurface, M_GetText("Current Video Mode"));
}

static void VID_Command_ModeList_f(void)
{
#if !defined (DC) && !defined (_WIN32_WCE) && !defined (_PSP) &&  !defined(GP2X)
	INT32 i;
#ifdef HWRENDER
	if (rendermode == render_opengl)
		modeList = SDL_ListModes(NULL, SDL_OPENGL|SDL_FULLSCREEN);
	else
#endif
	modeList = SDL_ListModes(NULL, surfaceFlagsF|SDL_HWSURFACE); //Alam: At least hardware surface

	if (modeList == (SDL_Rect **)0 && cv_fullscreen.value)
	{
		CONS_Printf("%s", M_GetText("No video modes present\n"));
		cv_fullscreen.value = 0;
	}
	else if (modeList != (SDL_Rect **)0)
	{
		numVidModes = 0;
		if (modeList == (SDL_Rect **)-1)
			numVidModes = -1; // should not happen with fullscreen modes
		else while (modeList[numVidModes])
			numVidModes++;
	}
	CONS_Printf(M_GetText("Found %d FullScreen Video Modes:\n"), numVidModes);
	for (i=0 ; i<numVidModes; i++)
	{ // fullscreen modes
		INT32 modeNum = firstEntry + i;
		if (modeNum >= numVidModes)
			break;

		CONS_Printf(M_GetText("%dx%d and "),
				modeList[modeNum]->w,
				modeList[modeNum]->h);
	}
	CONS_Printf("%s", M_GetText("None\n"));
#endif
}

static void VID_Command_Mode_f (void)
{
	INT32 modenum;

	if (COM_Argc()!= 2)
	{
		CONS_Printf(M_GetText("vid_mode <modenum> : set video mode, current video mode %i\n"), vid.modenum);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes())
		CONS_Printf(M_GetText("Video mode not present\n"));
	else
		setmodeneeded = modenum+1; // request vid mode change
}

#if defined(RPC_NO_WINDOWS_H) && !defined(_WIN32_WCE)
static VOID MainWndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hWnd);
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(wParam);
	switch (message)
	{
		case WM_SETTEXT:
			COM_BufAddText((LPCSTR)lParam);
			break;
	}
}
#endif

#if 0 //#ifdef _WIN32
	// Disable Composition in Vista DWM (Desktop Window Manager) ----------------
static HMODULE DMdll = NULL;
typedef HRESULT (CALLBACK *P_DwmIsCompositionEnabled) (BOOL *pfEnabled);
static P_DwmIsCompositionEnabled pfnDwmIsCompositionEnabled = NULL;
typedef HRESULT (CALLBACK *P_DwmEnableComposition) (BOOL   fEnable);
static P_DwmEnableComposition pfnDwmEnableComposition = NULL;
static BOOL AeroWasEnabled = FALSE;

static inline VOID UnloadDM(VOID)
{
	pfnDwmEnableComposition = NULL;
	pfnDwmIsCompositionEnabled = NULL;
	if (DMdll) FreeLibrary(DMdll);
	DMdll = NULL;
}

static inline BOOL LoadDM(VOID)
{
	if (DMdll)
		return TRUE;

	DMdll = LoadLibraryA("dwmapi.dll");
	if (DMdll)
		I_OutputMsg("dmwapi.dll loaded, Vista's Desktop Window Manager API\n");
	else
		return FALSE;

	pfnDwmIsCompositionEnabled = (P_DwmIsCompositionEnabled)GetProcAddress(DMdll, "DwmIsCompositionEnabled");
	if (pfnDwmIsCompositionEnabled)
		I_OutputMsg("Composition Aero API found, DwmIsCompositionEnabled\n");

	pfnDwmEnableComposition = (P_DwmEnableComposition)GetProcAddress(DMdll, "DwmEnableComposition");
	if (pfnDwmEnableComposition)
		I_OutputMsg("Composition Aero API found, DwmEnableComposition\n");

	return TRUE;
}

static inline VOID DisableAero(VOID)
{
	BOOL pfnDwmEnableCompositiond = FALSE;
	AeroWasEnabled = FALSE;

	if (!LoadDM())
		return;

	if (pfnDwmIsCompositionEnabled && SUCCEEDED(pfnDwmIsCompositionEnabled(&pfnDwmEnableCompositiond)))
		I_OutputMsg("Got the result of DwmIsCompositionEnabled, %i\n", pfnDwmEnableCompositiond);
	else
		return;

	if ((AeroWasEnabled = pfnDwmEnableCompositiond))
		I_OutputMsg("Disable the Aero rendering\n");
	else
		return;

	if (pfnDwmEnableComposition && SUCCEEDED(pfnDwmEnableComposition(FALSE)))
		I_OutputMsg("Aero rendering disabled\n");
	else
		I_OutputMsg("We failed to disable the Aero rendering\n");
}

static inline VOID ResetAero(VOID)
{
	if (pfnDwmEnableComposition && AeroWasEnabled)
	{
		if (SUCCEEDED(pfnDwmEnableComposition(AeroWasEnabled)))
			I_OutputMsg("Aero rendering setting restored\n");
		else
			I_OutputMsg("We failed to restore Aero rendering\n");
	}
	UnloadDM();
}
#endif

static inline void SDLJoyRemap(event_t *event)
{
#if defined(GP2X)
#define GP2X_BUTTON_UP              (0)
#define GP2X_BUTTON_DOWN            (4)
#define GP2X_BUTTON_LEFT            (2)
#define GP2X_BUTTON_RIGHT           (6)
#define GP2X_BUTTON_UPLEFT          (1)
#define GP2X_BUTTON_UPRIGHT         (7)
#define GP2X_BUTTON_DOWNLEFT        (3)
#define GP2X_BUTTON_DOWNRIGHT       (5)
#define GP2X_BUTTON_CLICK           (18)
#define GP2X_BUTTON_A               (12)
#define GP2X_BUTTON_B               (13)
#define GP2X_BUTTON_X               (14)
#define GP2X_BUTTON_Y               (15)
#define GP2X_BUTTON_L               (10)
#define GP2X_BUTTON_R               (11)
#define GP2X_BUTTON_START           (8)
#define GP2X_BUTTON_SELECT          (9)
#define GP2X_BUTTON_VOLUP           (16)
#define GP2X_BUTTON_VOLDOWN         (17)
	if ((event->type == ev_keydown || event->type == ev_keyup) && (KEY_JOY1 <= event->data1 && event->data1 <= KEY_JOY1+JOYBUTTONS))
	{
		INT32 button = event->data1-KEY_JOY1;
		if (button <= 7)
		{
			static UINT8 DPAD = 0;
			if (event->type == ev_keyup)
			{
				event->type = ev_console;
				DPAD &= ~(1<<button);
			}
			else
			{
				event->type = ev_joystick;
				DPAD |= 1<<button;
			}
			event->data2 = event->data3 = INT32_MAX;
			if ((DPAD & (1<<GP2X_BUTTON_UP)) && ((DPAD & (1<<GP2X_BUTTON_UPLEFT)) || (DPAD & (1<<GP2X_BUTTON_UPRIGHT))))
				button = GP2X_BUTTON_UP;
			if ((DPAD & (1<<GP2X_BUTTON_LEFT)) && ((DPAD & (1<<GP2X_BUTTON_UPLEFT)) || (DPAD & (1<<GP2X_BUTTON_DOWNLEFT))))
				button = GP2X_BUTTON_LEFT;
			if ((DPAD & (1<<GP2X_BUTTON_DOWN)) && ((DPAD & (1<<GP2X_BUTTON_DOWNLEFT)) || (DPAD & (1<<GP2X_BUTTON_DOWNRIGHT))))
				button = GP2X_BUTTON_DOWN;
			if ((DPAD & (1<<GP2X_BUTTON_RIGHT)) && ((DPAD & (1<<GP2X_BUTTON_UPRIGHT)) || (DPAD & (1<<GP2X_BUTTON_DOWNRIGHT))))
				button = GP2X_BUTTON_RIGHT;
			if (DPAD == 0)
			{
				event->type = ev_joystick;
				event->data2 = event->data3 = 0;
			}
			else switch (button)
			{
				case GP2X_BUTTON_UP:
					event->data3 = -1;
					break;
				case GP2X_BUTTON_DOWN:
					event->data3 = 1;
					break;
				case GP2X_BUTTON_LEFT:
					event->data2 = -1;
					break;
				case GP2X_BUTTON_RIGHT:
					event->data2 = 1;
					break;
				case GP2X_BUTTON_UPLEFT:
					event->data2 = -1;
					event->data3 = -1;
					break;
				case GP2X_BUTTON_UPRIGHT:
					event->data2 = 1;
					event->data3 = -1;
					break;
				case GP2X_BUTTON_DOWNLEFT:
					event->data2 = -1;
					event->data3 = 1;
					break;
				case GP2X_BUTTON_DOWNRIGHT:
					event->data2 = 1;
					event->data3 = 1;
				default:
					break;
			}
			event->data1 = 0;
			return;
		}
		else switch (button)
		{
			case GP2X_BUTTON_CLICK:
				event->data1 = KEY_ENTER;
				break;
			case GP2X_BUTTON_A:
				event->data1 = KEY_JOY1+0;
				break;
			case GP2X_BUTTON_B:
				event->data1 = KEY_JOY1+2;
				break;
			case GP2X_BUTTON_X:
				event->data1 = KEY_JOY1+3;
				break;
			case GP2X_BUTTON_Y:
				event->data1 = KEY_JOY1+1;
				break;
			case GP2X_BUTTON_L:
				event->data1 = KEY_JOY1+4;
				break;
			case GP2X_BUTTON_R:
				event->data1 = KEY_JOY1+5;
				break;
			case GP2X_BUTTON_START:
				event->data1 = KEY_ESCAPE;
				break;
			case GP2X_BUTTON_SELECT:
				event->data1 = KEY_JOY1+8;
				break;
			case GP2X_BUTTON_VOLUP:
				event->data1 = KEY_JOY1+6;
				break;
			case GP2X_BUTTON_VOLDOWN:
				event->data1 = KEY_JOY1+7;
				break;
			default:
				break;
		}
		//I_OutputMsg("Button %i: event key %i and type: %i\n", button, event->data1, event->type);
	}
#elif defined(_PSP)
	if (event->data1 > KEY_JOY1 + 9 + 2) // All button after D-Pad and Select/Start
		event->data1 -= 4; // remap D-pad to Hats, offset of -4
	else if (event->data1 == KEY_JOY1 + 6) // Down
		event->data1 = KEY_HAT1+1;
	else if (event->data1 == KEY_JOY1 + 7) // Left
		event->data1 = KEY_HAT1+2;
	else if (event->data1 == KEY_JOY1 + 8) // Up
		event->data1 = KEY_HAT1+0;
	else if (event->data1 == KEY_JOY1 + 9) // Right
		event->data1 = KEY_HAT1+3;
	else if (event->data1 == KEY_JOY1 + 10) // Select
		event->data1 = KEY_TAB;
	else if (event->data1 == KEY_JOY1 + 11) // Start
		event->data1 = KEY_ESCAPE;
#else
	(void)event;
#endif
}

static INT32 SDLJoyAxis(const Sint16 axis, evtype_t which)
{
#ifdef _arch_dreamcast // -128 to 127 SDL for DC have give us a smaller range
	INT32 raxis = axis*8;
#else // -32768 to 32767
	INT32 raxis = axis/32;
#endif
	if (which == ev_joystick)
	{
		if (Joystick.bGamepadStyle)
		{
			// gamepad control type, on or off, live or die
			if (raxis < -(JOYAXISRANGE/2))
				raxis = -1;
			else if (raxis > (JOYAXISRANGE/2))
				raxis = 1;
			else
				raxis = 0;
		}
		else
		{
			raxis = JoyInfo.scale!=1?((raxis/JoyInfo.scale)*JoyInfo.scale):raxis;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= raxis && raxis <= SDL_JDEADZONE)
				raxis = 0;
#endif
		}
	}
	else if (which == ev_joystick2)
	{
		if (Joystick2.bGamepadStyle)
		{
			// gamepad control type, on or off, live or die
			if (raxis < -(JOYAXISRANGE/2))
				raxis = -1;
			else if (raxis > (JOYAXISRANGE/2))
				raxis = 1;
			else raxis = 0;
		}
		else
		{
			raxis = JoyInfo2.scale!=1?((raxis/JoyInfo2.scale)*JoyInfo2.scale):raxis;

#ifdef SDL_JDEADZONE
			if (-SDL_JDEADZONE <= raxis && raxis <= SDL_JDEADZONE)
				raxis = 0;
#endif
		}
	}
	return raxis;
}

void I_GetEvent(void)
{
	SDL_Event inputEvent;
	static SDL_bool sdlquit = SDL_FALSE; //Alam: once, just once
	event_t event;

	if (!graphics_started)
		return;

	memset(&inputEvent, 0x00, sizeof(inputEvent));
	while (SDL_PollEvent(&inputEvent))
	{
		memset(&event,0x00,sizeof (event_t));
		switch (inputEvent.type)
		{
			case SDL_ACTIVEEVENT:
				if (inputEvent.active.state  & (SDL_APPACTIVE|SDL_APPINPUTFOCUS))
				{
					// pause music when alt-tab
					if (inputEvent.active.gain /*&& !paused */)
					{
						static SDL_bool firsttimeonmouse = SDL_TRUE;
						if (!firsttimeonmouse)
						{
							if (cv_usemouse.value) I_StartupMouse();
						}
						else firsttimeonmouse = SDL_FALSE;
						//if (!netgame && !con_destlines) paused = false;
						if (gamestate == GS_LEVEL)
							if (!paused) I_ResumeSong(0); //resume it
					}
					else /*if (!paused)*/
					{
						if (!disable_mouse)
							SDLforceUngrabMouse();
						if (!netgame && gamestate == GS_LEVEL) paused = true;
						memset(gamekeydown, 0, NUMKEYS);
						//S_PauseSound();
						if (gamestate == GS_LEVEL)
							I_PauseSong(0); //pause it
					}
				}
				if (MOUSE_MENU)
				{
					SDLdoUngrabMouse();
					break;
				}
				if ((SDL_APPMOUSEFOCUS&inputEvent.active.state) && USE_MOUSEINPUT && inputEvent.active.gain)
					HalfWarpMouse(realwidth, realheight);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				/// \todo inputEvent.key.which?
				if (inputEvent.type == SDL_KEYUP)
					event.type = ev_keyup;
				else if (inputEvent.type == SDL_KEYDOWN)
					event.type = ev_keydown;
				else break;
				event.data1 = SDLatekey(inputEvent.key.keysym.sym);
				if (event.data1) D_PostEvent(&event);
				break;
			case SDL_MOUSEMOTION:
				/// \todo inputEvent.motion.which
				if (MOUSE_MENU)
				{
					SDLdoUngrabMouse();
					break;
				}
				if (USE_MOUSEINPUT)
				{
					// If the event is from warping the pointer back to middle
					// of the screen then ignore it.
					if ((inputEvent.motion.x == realwidth/2) &&
					    (inputEvent.motion.y == realheight/2))
					{
						break;
					}
					else
					{
						event.data2 = +inputEvent.motion.xrel;
						event.data3 = -inputEvent.motion.yrel;
					}
					event.type = ev_mouse;
					D_PostEvent(&event);
					// Warp the pointer back to the middle of the window
					//  or we cannot move any further if it's at a border.
					if ((inputEvent.motion.x < (realwidth/2 )-(realwidth/4 )) ||
					    (inputEvent.motion.y < (realheight/2)-(realheight/4)) ||
					    (inputEvent.motion.x > (realwidth/2 )+(realwidth/4 )) ||
					    (inputEvent.motion.y > (realheight/2)+(realheight/4) ) )
					{
						//if (SDL_GRAB_ON == SDL_WM_GrabInput(SDL_GRAB_QUERY) || !mousegrabok)
							HalfWarpMouse(realwidth, realheight);
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				/// \todo inputEvent.button.which
				if (USE_MOUSEINPUT)
				{
					if (inputEvent.type == SDL_MOUSEBUTTONUP)
						event.type = ev_keyup;
					else if (inputEvent.type == SDL_MOUSEBUTTONDOWN)
						event.type = ev_keydown;
					else break;
					if (inputEvent.button.button==SDL_BUTTON_WHEELUP || inputEvent.button.button==SDL_BUTTON_WHEELDOWN)
					{
						if (inputEvent.type == SDL_MOUSEBUTTONUP)
							event.data1 = 0; //Alam: dumb! this could be a real button with some mice
						else
							event.data1 = KEY_MOUSEWHEELUP + inputEvent.button.button - SDL_BUTTON_WHEELUP;
					}
					else if (inputEvent.button.button == SDL_BUTTON_MIDDLE)
						event.data1 = KEY_MOUSE1+2;
					else if (inputEvent.button.button == SDL_BUTTON_RIGHT)
						event.data1 = KEY_MOUSE1+1;
					else if (inputEvent.button.button <= MOUSEBUTTONS)
						event.data1 = KEY_MOUSE1 + inputEvent.button.button - SDL_BUTTON_LEFT;
					if (event.data1) D_PostEvent(&event);
				}
				break;
			case SDL_JOYAXISMOTION:
				inputEvent.jaxis.which++;
				inputEvent.jaxis.axis++;
				event.data1 = event.data2 = event.data3 = INT32_MAX;
				if (cv_usejoystick.value == inputEvent.jaxis.which)
				{
					event.type = ev_joystick;
				}
				else if (cv_usejoystick.value == inputEvent.jaxis.which)
				{
					event.type = ev_joystick2;
				}
				else break;
				//axis
				if (inputEvent.jaxis.axis > JOYAXISSET*2)
					break;
				//vaule
				if (inputEvent.jaxis.axis%2)
				{
					event.data1 = inputEvent.jaxis.axis / 2;
					event.data2 = SDLJoyAxis(inputEvent.jaxis.value, event.type);
				}
				else
				{
					inputEvent.jaxis.axis--;
					event.data1 = inputEvent.jaxis.axis / 2;
					event.data3 = SDLJoyAxis(inputEvent.jaxis.value, event.type);
				}
				D_PostEvent(&event);
				break;
			case SDL_JOYBALLMOTION:
			case SDL_JOYHATMOTION:
				break; //NONE
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				inputEvent.jbutton.which++;
				if (cv_usejoystick.value == inputEvent.jbutton.which)
					event.data1 = KEY_JOY1;
				else if (cv_usejoystick.value == inputEvent.jbutton.which)
					event.data1 = KEY_2JOY1;
				else break;
				if (inputEvent.type == SDL_JOYBUTTONUP)
					event.type = ev_keyup;
				else if (inputEvent.type == SDL_JOYBUTTONDOWN)
					event.type = ev_keydown;
				else break;
				if (inputEvent.jbutton.button < JOYBUTTONS)
					event.data1 += inputEvent.jbutton.button;
				else
					break;
				SDLJoyRemap(&event);
				if (event.type != ev_console) D_PostEvent(&event);
				break;
#ifndef  _WIN32_WCE
			case SDL_QUIT:
				if (!sdlquit)
				{
					sdlquit = SDL_TRUE;
					M_QuitResponse('y');
				}
				break;
#endif
#if defined(RPC_NO_WINDOWS_H) && !defined(_WIN32_WCE)
			case SDL_SYSWMEVENT:
				MainWndproc(inputEvent.syswm.msg->hwnd,
					inputEvent.syswm.msg->msg,
					inputEvent.syswm.msg->wParam,
					inputEvent.syswm.msg->lParam);
				break;
#endif
			case SDL_VIDEORESIZE:
				if (gamestate == GS_LEVEL || gamestate == GS_TITLESCREEN || gamestate == GS_EVALUATION)
				    setmodeneeded = VID_GetModeForSize(inputEvent.resize.w,inputEvent.resize.h)+1;
				if (render_soft == rendermode)
				{
#ifdef FILTERS
					INT32 filtervalue = cv_filter.value;
					if (blitfilter) CV_SetValue(&cv_filter,1);
#endif
					SDLSetMode(realwidth, realheight, vid.bpp*8, surfaceFlagsW);
					if (vidSurface) SDL_SetColors(vidSurface, localPalette, 0, 256);
#ifdef FILTERS
					CV_SetValue(&cv_filter,filtervalue);
#endif
				}
				else
					SDLSetMode(realwidth, realheight, vid.bpp*8, surfaceFlagsW);
				if (!vidSurface)
					I_Error("Could not reset vidmode: %s\n",SDL_GetError());
				break;
			case SDL_VIDEOEXPOSE:
				exposevideo = SDL_TRUE;
				break;
			default:
				break;
		}
	}
	//reset wheel like in win32, I don't understand it but works
	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;
}

void I_StartupMouse(void)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;

	if (disable_mouse)
		return;

	if (!firsttimeonmouse)
		HalfWarpMouse(realwidth, realheight); // warp to center
	else
		firsttimeonmouse = SDL_FALSE;
	if (cv_usemouse.value)
		return;
	else
		SDLdoUngrabMouse();
}

//
// I_OsPolling
//
void I_OsPolling(void)
{
	if (consolevent)
		I_GetConsoleEvents();
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == SDL_INIT_JOYSTICK)
	{
		SDL_JoystickUpdate();
		I_GetJoystickEvents();
		I_GetJoystick2Events();
	}
#ifdef _arch_dreamcast
	//vmu_set_icon(VMU_bits);
#endif

	I_GetMouseEvents();

	I_GetEvent();
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
	if (!vidSurface)
		return;
#ifdef HWRENDER
	if (rendermode != render_soft)
		OglSdlFinishUpdate(cv_vidwait.value);
	else
#endif
	if (vidSurface->flags&SDL_DOUBLEBUF)
		SDL_Flip(vidSurface);
	else if (exposevideo)
		SDL_UpdateRect(vidSurface, 0, 0, 0, 0);
	exposevideo = SDL_FALSE;
}

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame
// from PrBoom's src/SDL/i_video.c
static inline boolean I_SkipFrame(void)
{
	static boolean skip = false;

	if (render_soft != rendermode)
		return false;

	skip = !skip;

#if 0 //(defined (GP2X) || defined (PSP) || defined (_arch_dreamcast))
	return skip;
#endif

	switch (gamestate)
	{
		case GS_LEVEL:
			if (!paused)
				return false;
		case GS_TIMEATTACK:
		case GS_WAITINGPLAYERS:
			return skip; // Skip odd frames
		default:
			return false;
	}
}

static inline SDL_bool SDLmatchVideoformat(void)
{
	const SDL_PixelFormat *vidformat = vidSurface->format;
	const INT32 vfBPP = vidformat?vidformat->BitsPerPixel:0;
	return (((vfBPP == 8 && vid.bpp == 1 &&
	 !vidformat->Rmask && !vidformat->Gmask && !vidformat->Bmask) ||
	 (vfBPP == 15 && vid.bpp == 2 && vidformat->Rmask == 0x7C00 &&
	 vidformat->Gmask == 0x03E0 && vidformat->Bmask == 0x001F )) &&
	 !vidformat->Amask && (vidSurface->flags & SDL_RLEACCEL) == 0);
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
	if (!vidSurface)
		return; //Alam: No software or OpenGl surface

	if (I_SkipFrame())
		return;

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (render_soft == rendermode && screens[0])
	{
		SDL_Rect *dstrect = NULL;
		SDL_Rect rect = {0, 0, 0, 0};
		SDL_PixelFormat *vidformat = vidSurface->format;
		int lockedsf = 0, blited = 0;

		rect.w = (Sint16)vid.width;
		rect.h = (Sint16)vid.height;

		if (vidSurface->h > vid.height)
			rect.y = (Sint16)((vidSurface->h-vid.height)/2);

		dstrect = &rect;


		if (!bufSurface && !vid.direct) //Double-Check
		{
			if (vid.bpp == 1) bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,8,
				(int)vid.rowbytes,0x00000000,0x00000000,0x00000000,0x00000000); // 256 mode
			else if (vid.bpp == 2) bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,15,
				(int)vid.rowbytes,0x00007C00,0x000003E0,0x0000001F,0x00000000); // 555 mode
			if (bufSurface) SDL_SetColors(bufSurface, localPalette, 0, 256);
			else I_OutputMsg("No system memory for SDL buffer surface\n");
		}

#ifdef FILTERS
		FilterBlit(bufSurface);
		if (f2xSurface) //Alam: filter!
		{
			//I_OutputMsg("2x Filter Code\n");
			blited = SDL_BlitSurface(f2xSurface,NULL,vidSurface,NULL);
		}
		else
#endif
#if 0
		if (SDLmatchVideoformat() && !vid.direct)//Alam: DOS Way
		{
			if (SDL_MUSTLOCK(vidSurface)) lockedsf = SDL_LockSurface(vidSurface);
			if (lockedsf == 0)
			{
				if (vidSurface->pixels > vid.height)
				{
					UINT8 *ptr = vidSurface->pixels;
					size_t half_excess = vidSurface->pitch*(vidSurface->height-vid.height)/2;
					memset(ptr, 0x1F, half_excess);
					ptr += half_excess;
					VID_BlitLinearScreen(screens[0], ptr, vid.width*vid.bpp, vid.height,
					                     vid.rowbytes, vidSurface->pitch);
					ptr += vid.height*vidSurface->pitch;
					memset(ptr, 0x1F, half_excess);
				}
				else
				VID_BlitLinearScreen(screens[0], vidSurface->pixels, vid.width*vid.bpp,
				                     vid.height, vid.rowbytes, vidSurface->pitch );
				if (SDL_MUSTLOCK(vidSurface)) SDL_UnlockSurface(vidSurface);
			}
		}
		else
#endif
		if (bufSurface) //Alam: New Way to send video data
		{
			blited = SDL_BlitSurface(bufSurface,NULL,vidSurface,dstrect);
		}
		else if (vid.bpp == 1 && !vid.direct)
		{
			Uint8 *bP,*vP; //Src, Dst
			Uint16 bW, vW; // Pitch Remainder
			Sint32 pH, pW; //Height, Width
			bP = (Uint8 *)screens[0];
			bW = (Uint16)(vid.rowbytes - vid.width);
			//I_OutputMsg("Old Copy Code\n");
			if (SDL_MUSTLOCK(vidSurface)) lockedsf = SDL_LockSurface(vidSurface);
			vP = (Uint8 *)vidSurface->pixels;
			vW = (Uint16)(vidSurface->pitch - vidSurface->w*vidformat->BytesPerPixel);
			if (vidSurface->h > vid.height)
				vP += vidSurface->pitch*(vidSurface->h-vid.height)/2;
			if (lockedsf == 0 && vidSurface->pixels)
			{
				if (vidformat->BytesPerPixel == 2)
				{
					for (pH=0;pH < vidSurface->h;pH++)
					{
						for (pW=0;pW < vidSurface->w;pW++)
						{
							*((Uint16 *)(void *)vP) = (Uint16)SDL_MapRGB(vidformat,
								localPalette[*bP].r,localPalette[*bP].g,localPalette[*bP].b);
							bP++;
							vP += 2;
						}
						bP += bW;
						vP += vW;
					}
				}
				else if (vidformat->BytesPerPixel == 3)
				{
					for (pH=0;pH < vidSurface->h;pH++)
					{
						for (pW=0;pW < vidSurface->w;pW++)
						{
							*((Uint32 *)(void *)vP) = SDL_MapRGB(vidformat,
								localPalette[*bP].r,localPalette[*bP].g,localPalette[*bP].b);
							bP++;
							vP += 3;
						}
						bP += bW;
						vP += vW;
					}
				}
				else if (vidformat->BytesPerPixel == 4)
				{
					for (pH=0;pH < vidSurface->h;pH++)
					{
						for (pW=0;pW < vidSurface->w;pW++)
						{
							*((Uint32 *)(void *)vP) = SDL_MapRGB(vidformat,
								localPalette[*bP].r,localPalette[*bP].g,localPalette[*bP].b);
							bP++;
							vP += 4;
						}
						bP += bW;
						vP += vW;
					}
				}
				else
				{
					;//NOP
				}
			}
			if (SDL_MUSTLOCK(vidSurface)) SDL_UnlockSurface(vidSurface);
		}
		else /// \todo 15t15,15tN, others?
		{
			;//NOP
		}

#ifdef HAVE_GP2XSDL
		//if (blited == 0)
			SDL_GP2X_WaitForBlitter();
#endif

		if (lockedsf == 0 && blited == 0 && vidSurface->flags&SDL_DOUBLEBUF)
			SDL_Flip(vidSurface);
		else if (blited != -2 && lockedsf == 0) //Alam: -2 for Win32 Direct, yea, i know
			SDL_UpdateRect(vidSurface, rect.x, rect.y, 0, 0); //Alam: almost always
		else
			I_OutputMsg("%s\n",SDL_GetError());
	}
#ifdef HWRENDER
	else
	{
		OglSdlFinishUpdate(cv_vidwait.value);
	}
#endif
	exposevideo = SDL_FALSE;
}

//
// I_UpdateNoVsync
//
void I_UpdateNoVsync(void)
{
	INT32 real_vidwait = cv_vidwait.value;
	cv_vidwait.value = 0;
	I_FinishUpdate();
	cv_vidwait.value = real_vidwait;
}

//
// I_ReadScreen
//
void I_ReadScreen(UINT8 *scr)
{
	if (rendermode != render_soft)
		I_Error ("I_ReadScreen: called while in non-software mode");
	else
		VID_BlitLinearScreen(screens[0], scr,
			vid.width*vid.bpp, vid.height,
			vid.rowbytes, vid.rowbytes);
}

//
// I_SetPalette
//
void I_SetPalette(RGBA_t *palette)
{
	size_t i;
	for (i=0; i<256; i++)
	{
		localPalette[i].r = palette[i].s.red;
		localPalette[i].g = palette[i].s.green;
		localPalette[i].b = palette[i].s.blue;
	}
	if (vidSurface) SDL_SetColors(vidSurface, localPalette, 0, 256);
	if (bufSurface) SDL_SetColors(bufSurface, localPalette, 0, 256);
}

// return number of fullscreen + X11 modes
INT32 VID_NumModes(void)
{
	if (USE_FULLSCREEN && numVidModes != -1)
		return numVidModes - firstEntry;
	else
		return MAXWINMODES;
}

const char *VID_GetModeName(INT32 modeNum)
{
	if (USE_FULLSCREEN && numVidModes != -1) // fullscreen modes
	{
		modeNum += firstEntry;
		if (modeNum >= numVidModes)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			modeList[modeNum]->w,
			modeList[modeNum]->h);
	}
	else // windowed modes
	{
		if (modeNum > MAXWINMODES)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			windowedModes[modeNum][0],
			windowedModes[modeNum][1]);
	}
	return &vidModeName[modeNum][0];
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	INT32 matchMode = -1, i;
	VID_PrepareModeList();
	if (USE_FULLSCREEN && numVidModes != -1)
	{
		for (i=firstEntry; i<numVidModes; i++)
		{
			if (modeList[i]->w == w &&
			    modeList[i]->h == h)
			{
				matchMode = i;
				break;
			}
		}
		if (-1 == matchMode) // use smaller mode
		{
			w -= w%BASEVIDWIDTH;
			h -= h%BASEVIDHEIGHT;
			for (i=firstEntry; i<numVidModes; i++)
			{
				if (modeList[i]->w == w &&
				    modeList[i]->h == h)
				{
					matchMode = i;
					break;
				}
			}
			if (-1 == matchMode) // use smallest mode
				matchMode = numVidModes-1;
		}
		matchMode -= firstEntry;
	}
	else
	{
		for (i=0; i<MAXWINMODES; i++)
		{
			if (windowedModes[i][0] == w &&
			    windowedModes[i][1] == h)
			{
				matchMode = i;
				break;
			}
		}
		if (-1 == matchMode) // use smaller mode
		{
			w -= w%BASEVIDWIDTH;
			h -= h%BASEVIDHEIGHT;
			for (i=0; i<MAXWINMODES; i++)
			{
				if (windowedModes[i][0] == w &&
				    windowedModes[i][1] == h)
				{
					matchMode = i;
					break;
				}
			}
			if (-1 == matchMode) // use smallest mode
				matchMode = MAXWINMODES-1;
		}
	}
	return matchMode;
}

void VID_PrepareModeList(void)
{
	INT32 i;

	firstEntry = 0;

#ifdef HWRENDER
	if (rendermode == render_opengl)
		modeList = SDL_ListModes(NULL, SDL_OPENGL|SDL_FULLSCREEN);
	else
#endif
	modeList = SDL_ListModes(NULL, surfaceFlagsF|SDL_HWSURFACE); //Alam: At least hardware surface

	if (disable_fullscreen?0:cv_fullscreen.value) // only fullscreen needs preparation
	{
		if (-1 != numVidModes)
		{
			for (i=0; i<numVidModes; i++)
			{
				if (modeList[i]->w <= MAXVIDWIDTH &&
					modeList[i]->h <= MAXVIDHEIGHT)
				{
					firstEntry = i;
					break;
				}
			}
		}
	}
	allow_fullscreen = true;
}

static inline void SDLESSet(void)
{
#ifdef HAVE_DCSDL
	INT32 j;
	SDL_DC_SetVideoDriver(SDL_DC_DIRECT_VIDEO); //SDL_DC_DMA_VIDEO
	for (j=0;j<4;j++)
	{
		SDL_DC_MapKey(j,SDL_DC_START,SDLK_ESCAPE);
		SDL_DC_MapKey(j,SDL_DC_A,SDLK_UNKNOWN);
		SDL_DC_MapKey(j,SDL_DC_B,SDLK_UNKNOWN);
		SDL_DC_MapKey(j,SDL_DC_X,SDLK_UNKNOWN);
		SDL_DC_MapKey(j,SDL_DC_Y,SDLK_UNKNOWN);
		SDL_DC_MapKey(j,SDL_DC_L,SDLK_UNKNOWN);
		SDL_DC_MapKey(j,SDL_DC_R,SDLK_UNKNOWN);
		//SDL_DC_MapKey(j,SDL_DC_LEFT,SDLK_UNKNOWN);
		//SDL_DC_MapKey(j,SDL_DC_RIGHT,SDLK_UNKNOWN);
		//SDL_DC_MapKey(j,SDL_DC_UP,SDLK_UNKNOWN);
		//SDL_DC_MapKey(j,SDL_DC_DOWN,SDLK_UNKNOWN);
	}
	//SDL_DC_MapKey(0,SDL_DC_L,SDLK_LEFTBRACKET);
	//SDL_DC_MapKey(0,SDL_DC_R,SDLK_RIGHTBRACKET);
	//SDL_DC_MapKey(0,SDL_DC_START,SDLK_UNKNOWN);
	//SDL_DC_MapKey(1,SDL_DC_L,SDLK_z);
	//SDL_DC_MapKey(1,SDL_DC_R,SDLK_x);
#endif
#ifdef HAVE_GP2XSDL
	SDL_GP2X_MiniDisplay(0,0);
	//SDL_GP2X_DenyGfxMemory(NULL, 0);
	SDL_GP2X_AllowGfxMemory(NULL, 0);
#endif
}

static void SDLWMSet(void)
{
#ifdef RPC_NO_WINDOWS_H
	SDL_SysWMinfo SDLWM;
	memset(&SDLWM,0,sizeof (SDL_SysWMinfo));
	SDL_VERSION(&SDLWM.version)
	if (SDL_GetWMInfo(&SDLWM))
		vid.WndParent = SDLWM.window;
	else
		vid.WndParent = INVALID_HANDLE_VALUE;
	if (vid.WndParent != INVALID_HANDLE_VALUE)
	{
		SetFocus(vid.WndParent);
		ShowWindow(vid.WndParent, SW_SHOW);
	}
#ifndef _WIN32_WCE
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#endif
#endif
	SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
}

static void* SDLGetDirect(void)
{
#if 0 //#ifndef __MACH__ // Do not directly access the MacOSX's OpenGL memory
	if (!SDL_MUSTLOCK(vidSurface) && SDLmatchVideoformat())
	{
		vid.rowbytes = vidSurface->pitch;
		return vidSurface->pixels;
	}
#endif // you can not use the video memory in pixels member in fullscreen mode
	return NULL;
}

INT32 VID_SetMode(INT32 modeNum)
{
#ifdef _WIN32_WCE
	(void)modeNum;
#else
	SDLdoUngrabMouse();
	vid.recalc = true;
	BitsPerPixel = (Uint8)cv_scr_depth.value;
	//vid.bpp = BitsPerPixel==8?1:2;
	// Window title
	SDL_WM_SetCaption("SRB2 "VERSIONSTRING, "SRB2");

	if (render_soft == rendermode)
	{
		//Alam: SDL_Video system free vidSurface for me
		if (vid.buffer) free(vid.buffer);
		vid.buffer = NULL;
		if (bufSurface) SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

	if (USE_FULLSCREEN)
	{
		if (numVidModes != -1)
		{
			modeNum += firstEntry;
			vid.width = modeList[modeNum]->w;
			vid.height = modeList[modeNum]->h;
		}
		else
		{
			vid.width = windowedModes[modeNum][0];
			vid.height = windowedModes[modeNum][1];
		}
		if (render_soft == rendermode)
		{
			SDLSetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsF);

			if (!vidSurface)
			{
				cv_fullscreen.value = 0;
				modeNum = VID_GetModeForSize(vid.width,vid.height);
				vid.width = windowedModes[modeNum][0];
				vid.height = windowedModes[modeNum][1];
				SDLSetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsW);
				if (!vidSurface)
					I_Error("Could not set vidmode: %s\n",SDL_GetError());
			}
		}
#ifdef HWRENDER
		else // (render_soft != rendermode)
		{
			if (!OglSdlSurface(vid.width, vid.height, true))
			{
				cv_fullscreen.value = 0;
				modeNum = VID_GetModeForSize(vid.width,vid.height);
				vid.width = windowedModes[modeNum][0];
				vid.height = windowedModes[modeNum][1];
				if (!OglSdlSurface(vid.width, vid.height,false))
					I_Error("Could not set vidmode: %s\n",SDL_GetError());
			}

			realwidth = (Uint16)vid.width;
			realheight = (Uint16)vid.height;
		}
#endif
	}
	else //(cv_fullscreen.value)
	{
		vid.width = windowedModes[modeNum][0];
		vid.height = windowedModes[modeNum][1];

		if (render_soft == rendermode)
		{
			SDLSetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsW);
			if (!vidSurface)
				I_Error("Could not set vidmode: %s\n",SDL_GetError());
		}
#ifdef HWRENDER
		else //(render_soft != rendermode)
		{
			if (!OglSdlSurface(vid.width, vid.height, false))
				I_Error("Could not set vidmode: %s\n",SDL_GetError());
			realwidth = (Uint16)vid.width;
			realheight = (Uint16)vid.height;
		}
#endif
	}

	vid.modenum = VID_GetModeForSize(vidSurface->w,vidSurface->h);

	if (render_soft == rendermode)
	{
		vid.rowbytes = vid.width*vid.bpp;
		vid.direct = SDLGetDirect();
		vid.buffer = malloc(vid.rowbytes*vid.height*NUMSCREENS);
		if (vid.buffer) memset(vid.buffer,0x00,vid.rowbytes*vid.height*NUMSCREENS);
		else I_Error ("Not enough memory for video buffer\n");
	}

#if 0 // broken
	if (!cv_stretch.value && (float)vid.width/vid.height != ((float)BASEVIDWIDTH/BASEVIDHEIGHT))
		vid.height = (INT32)(vid.width * ((float)BASEVIDHEIGHT/BASEVIDWIDTH));// Adjust the height to match
#endif
#endif
	I_StartupMouse();

	SDLWMSet();

	return true;
}

void I_StartupGraphics(void)
{
	static char SDLNOMOUSE[] = "SDL_NOMOUSE=1";
	static char SDLVIDEOMID[] = "SDL_VIDEO_CENTERED=center";

	if (dedicated)
	{
		rendermode = render_none;
		return;
	}
	if (graphics_started)
		return;

	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f);
	COM_AddCommand ("vid_info", VID_Command_Info_f);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f);
	CV_RegisterVar (&cv_vidwait);
	CV_RegisterVar (&cv_stretch);
#ifdef FILTERS
	CV_RegisterVar (&cv_filter);
#endif
#ifdef _PSP // pitch is 0, mod of 0 crash
	disable_mouse = true;
#else
	disable_mouse = M_CheckParm("-nomouse");
#endif
	if (disable_mouse)
		I_PutEnv(SDLNOMOUSE);
	if (!I_GetEnv("SDL_VIDEO_CENTERED"))
		I_PutEnv(SDLVIDEOMID);
	disable_fullscreen = M_CheckParm("-win");

	keyboard_started = true;
#ifdef _arch_dreamcast
	conio_shutdown();
#endif

#if !defined(HAVE_TTF)
#ifdef _WIN32 // Initialize Audio as well, otherwise Win32's DirectX can not use audio
	if (SDL_InitSubSystem(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
#else //SDL_OpenAudio will do SDL_InitSubSystem(SDL_INIT_AUDIO)
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
#endif
	{
#ifdef _WIN32
		if (SDL_WasInit(SDL_INIT_AUDIO)==0)
			CONS_Printf(M_GetText("Couldn't initialize SDL's Audio System with Video System: %s\n"), SDL_GetError());
		if (SDL_WasInit(SDL_INIT_VIDEO)==0)
#endif
		{
			CONS_Printf(M_GetText("Couldn't initialize SDL's Video System: %s\n"), SDL_GetError());
			return;
		}
	}
#ifdef REMOTE_DEBUGGING
#ifdef _WII
	_break(); // break for debugger
#endif
#endif
#endif
	{
		char vd[100]; //stack space for video name
		CONS_Printf(M_GetText("Starting up with video driver : %s\n"), SDL_VideoDriverName(vd,100));
		if (strncasecmp(vd, "gcvideo", 8) == 0 || strncasecmp(vd, "fbcon", 6) == 0 || strncasecmp(vd, "wii", 4) == 0 || strncasecmp(vd, "psl1ght", 8) == 0)
			framebuffer = SDL_TRUE;
	}
	if (M_CheckParm("-software"))
		rendermode = render_soft;
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY>>1,SDL_DEFAULT_REPEAT_INTERVAL<<2);
	SDLESSet();
	VID_Command_ModeList_f();
	vid.buffer = NULL;  // For software mode
	vid.width = BASEVIDWIDTH; // Default size for startup
	vid.height = BASEVIDHEIGHT; // BitsPerPixel is the SDL interface's
	vid.recalc = true; // Set up the console stufff
	vid.direct = NULL; // Maybe direct access?
	vid.bpp = 1; // This is the game engine's Bpp
	vid.WndParent = NULL; //For the window?

#ifdef HAVE_TTF
	I_ShutdownTTF();
#endif

	// Window title
#ifdef _WIN32_WCE
	SDL_WM_SetCaption("SRB2 "VERSIONSTRING, "SRB2");
#else
	SDL_WM_SetCaption("SRB2: Starting up", "SRB2");
#endif

	// Window icon
#ifdef HAVE_IMAGE
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif
	SDL_WM_SetIcon(icoSurface, NULL);

#ifdef _WIN32
	//DisableAero(); //also disable Aero on Vista
#endif

#ifdef HWRENDER
	if (M_CheckParm("-opengl") || rendermode == render_opengl)
	{
		rendermode = render_opengl;
		HWD.pfnInit             = hwSym("Init",NULL);
		HWD.pfnFinishUpdate     = NULL;
		HWD.pfnDraw2DLine       = hwSym("Draw2DLine",NULL);
		HWD.pfnDrawPolygon      = hwSym("DrawPolygon",NULL);
		HWD.pfnSetBlend         = hwSym("SetBlend",NULL);
		HWD.pfnClearBuffer      = hwSym("ClearBuffer",NULL);
		HWD.pfnSetTexture       = hwSym("SetTexture",NULL);
		HWD.pfnReadRect         = hwSym("ReadRect",NULL);
		HWD.pfnGClipRect        = hwSym("GClipRect",NULL);
		HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache",NULL);
		HWD.pfnSetSpecialState  = hwSym("SetSpecialState",NULL);
		HWD.pfnSetPalette       = hwSym("SetPalette",NULL);
		HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed",NULL);
		HWD.pfnDrawMD2          = hwSym("DrawMD2",NULL);
		HWD.pfnDrawMD2i         = hwSym("DrawMD2i",NULL);
		HWD.pfnSetTransform     = hwSym("SetTransform",NULL);
		HWD.pfnGetRenderVersion = hwSym("GetRenderVersion",NULL);
#ifdef SHUFFLE
		HWD.pfnPostImgRedraw    = hwSym("PostImgRedraw",NULL);
#endif
		HWD.pfnFlushScreenTextures=hwSym("FlushScreenTextures",NULL);
		HWD.pfnStartScreenWipe  = hwSym("StartScreenWipe",NULL);
		HWD.pfnEndScreenWipe    = hwSym("EndScreenWipe",NULL);
		HWD.pfnDoScreenWipe     = hwSym("DoScreenWipe",NULL);
		HWD.pfnDrawIntermissionBG=hwSym("DrawIntermissionBG",NULL);
		HWD.pfnMakeScreenTexture= hwSym("MakeScreenTexture",NULL);
		// check gl renderer lib
		if (HWD.pfnGetRenderVersion() != VERSION)
			I_Error("%s", M_GetText("The version of the renderer doesn't match the version of the executable\nBe sure you have installed SRB2 properly.\n"));
#if 1 //#ifdef  _WIN32_WCE
		vid.width = BASEVIDWIDTH;
		vid.height = BASEVIDHEIGHT;
#else
		vid.width = 640; // hack to make voodoo cards work in 640x480
		vid.height = 480;
#endif
		if (HWD.pfnInit(I_Error)) // let load the OpenGL library
		{
			/*
			* We want at least 1 bit R, G, and B,
			* and at least 16 bpp. Why 1 bit? May be more?
			*/
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			if (!OglSdlSurface(vid.width, vid.height, (USE_FULLSCREEN)))
				if (!OglSdlSurface(vid.width, vid.height, !(USE_FULLSCREEN)))
					rendermode = render_soft;
		}
		else
			rendermode = render_soft;
	}
#else
	rendermode = render_soft; //force software mode when there no HWRENDER code
#endif
	if (render_soft == rendermode)
	{
#if defined(_WII)
		vid.width = 640;
		vid.height = 480;
#elif defined(_PS3)
		vid.width = 720;
		vid.height = 480;
#else
		vid.width = BASEVIDWIDTH;
		vid.height = BASEVIDHEIGHT;
#endif
		SDLSetMode(vid.width, vid.height, BitsPerPixel, surfaceFlagsW);
		if (!vidSurface)
		{
			CONS_Printf(M_GetText("Could not set vidmode: %s\n") ,SDL_GetError());
			vid.rowbytes = 0;
			graphics_started = true;
			return;
		}
		vid.rowbytes = vid.width * vid.bpp;
		vid.direct = SDLGetDirect();
		vid.buffer = malloc(vid.rowbytes*vid.height*NUMSCREENS);
		if (vid.buffer) memset(vid.buffer,0x00,vid.rowbytes*vid.height*NUMSCREENS);
		else CONS_Printf("%s", M_GetText("Not enough memory for video buffer\n"));
	}
	if (M_CheckParm("-nomousegrab"))
		mousegrabok = SDL_FALSE;
#ifdef _DEBUG
	else
	{
		char videodriver[4] = {'S','D','L',0};
		if (!M_CheckParm("-mousegrab") &&
		    SDL_VideoDriverName(videodriver,4) &&
		    strncasecmp("X11",videodriver,4) == 0)
			mousegrabok = SDL_FALSE; //X11's XGrabPointer not good
	}
#endif
	realwidth = (Uint16)vid.width;
	realheight = (Uint16)vid.height;

	VID_Command_Info_f();
	if (!disable_mouse) SDL_ShowCursor(SDL_DISABLE);
	SDLdoUngrabMouse();

	SDLWMSet();

	graphics_started = true;
}

void I_ShutdownGraphics(void)
{
	const rendermode_t oldrendermode = rendermode;

	rendermode = render_none;
	if (icoSurface) SDL_FreeSurface(icoSurface);
	icoSurface = NULL;
	if (render_soft == oldrendermode)
	{
		vidSurface = NULL; //Alam: SDL_Video system free vidSurface for me
		if (vid.buffer) free(vid.buffer);
		vid.buffer = NULL;
		if (bufSurface) SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
#ifdef FILTERS
		if (preSurface) SDL_FreeSurface(preSurface);
		preSurface = NULL;
		if (f2xSurface) SDL_FreeSurface(f2xSurface);
		f2xSurface = NULL;
#endif
	}

	// was graphics initialized anyway?
	if (!graphics_started)
		return;
	CONS_Printf("I_ShutdownGraphics: ");
#ifdef _WIN32
	//ResetAero();
#endif
	graphics_started = false;
	CONS_Printf("%s", M_GetText("shut down\n"));
#ifdef HWRENDER
	if (GLUhandle)
		hwClose(GLUhandle);
#endif
#ifndef _arch_dreamcast
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
	framebuffer = SDL_FALSE;
}
#endif
