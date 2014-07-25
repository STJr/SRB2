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

#include <signal.h>

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef SDL

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
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
#endif

#ifdef HAVE_IMAGE
#include "SDL_icon.xpm"
#endif

#include "../doomdef.h"

#if defined (_WIN32)
#include "SDL_syswm.h"
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

// maximum number of windowed modes (see windowedModes[][])
#define MAXWINMODES (17)

/**	\brief
*/
static INT32 numVidModes = -1;

/**	\brief
*/
static char vidModeName[33][32]; // allow 33 different modes

rendermode_t rendermode=render_soft;

boolean highcolor = false;

// synchronize page flipping with screen refresh
consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_stretch = {"stretch", "Off", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

UINT8 graphics_started = 0; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in VID_PrepareModeList
boolean allow_fullscreen = false;
static SDL_bool disable_fullscreen = SDL_FALSE;
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value
static SDL_bool disable_mouse = SDL_FALSE;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && havefocus)
#define MOUSE_MENU false //(!disable_mouse && cv_usemouse.value && menuactive && !USE_FULLSCREEN)
#define MOUSEBUTTONS_MAX MOUSEBUTTONS

// first entry in the modelist which is not bigger than MAXVIDWIDTHxMAXVIDHEIGHT
static      INT32          firstEntry = 0;

// SDL vars
static      SDL_Surface *vidSurface = NULL;
static      SDL_Surface *bufSurface = NULL;
static      SDL_Surface *icoSurface = NULL;
static      SDL_Color    localPalette[256];
#if 0
static      SDL_Rect   **modeList = NULL;
static       Uint8       BitsPerPixel = 16;
#endif
static       Uint16      realwidth = BASEVIDWIDTH;
static       Uint16      realheight = BASEVIDHEIGHT;
static const Uint32      surfaceFlagsW = 0/*|SDL_RESIZABLE*/;
static const Uint32      surfaceFlagsF = 0;
static       SDL_bool    mousegrabok = SDL_TRUE;
#define HalfWarpMouse(x,y) SDL_WarpMouseInWindow(window, (Uint16)(x/2),(Uint16)(y/2))
static       SDL_bool    videoblitok = SDL_FALSE;
static       SDL_bool    exposevideo = SDL_FALSE;
static       SDL_bool    usesdl2soft = SDL_FALSE;

// SDL2 vars
SDL_Window   *window;
SDL_Renderer *renderer;
static SDL_Texture  *texture;
static SDL_bool      havefocus = SDL_TRUE;

// windowed video modes from which to choose from.
static INT32 windowedModes[MAXWINMODES][2] =
{
	{1920,1200}, // 1.60,6.00
	{1920,1080}, // 1.66
	{1680,1050}, // 1.60,5.25
	{1600, 900}, // 1.66
	{1366, 768}, // 1.66
	{1440, 900}, // 1.60,4.50
	{1280,1024}, // 1.33?
	{1280, 960}, // 1.33,4.00
	{1280, 800}, // 1.60,4.00
	{1280, 720}, // 1.66
	{1152, 864}, // 1.33,3.60
	{1024, 768}, // 1.33,3.20
	{ 800, 600}, // 1.33,2.50
	{ 640, 480}, // 1.33,2.00
	{ 640, 400}, // 1.60,2.00
	{ 320, 240}, // 1.33,1.00
	{ 320, 200}, // 1.60,1.00
};

static void Impl_VideoSetupSDLBuffer(void);
static void Impl_VideoSetupBuffer(void);
static SDL_bool Impl_CreateWindow(SDL_bool fullscreen);
static void Impl_SetWindowName(const char *title);
static void Impl_SetWindowIcon(void);

static void SDLSetMode(INT32 width, INT32 height, SDL_bool fullscreen)
{
	static SDL_bool wasfullscreen = SDL_FALSE;
	Uint32 rmask;
	Uint32 gmask;
	Uint32 bmask;
	Uint32 amask;
	int bpp = 16;
	int sw_texture_format = SDL_PIXELFORMAT_ABGR8888;

	realwidth = vid.width;
	realheight = vid.height;

	if (window)
	{
		if (fullscreen)
		{
			wasfullscreen = SDL_TRUE;
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else if (!fullscreen && wasfullscreen)
		{
			wasfullscreen = SDL_FALSE;
			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowSize(window, width, height);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}
		else if (!wasfullscreen)
		{
			// Reposition window only in windowed mode
			SDL_SetWindowSize(window, width, height);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}
	}
	else
	{
		Impl_CreateWindow(fullscreen);
		Impl_SetWindowIcon();
		wasfullscreen = fullscreen;
		SDL_SetWindowSize(window, width, height);
		if (fullscreen)
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
	}

	if (rendermode == render_opengl)
	{
		int sdlw, sdlh;
		SDL_GetWindowSize(window, &sdlw, &sdlh);
		// Logical fullscreen is not implemented yet for OpenGL, so...
		// Special case handling
		if (fullscreen && width != sdlw && height != sdlh)
		{
			VID_SetMode(VID_GetModeForSize(sdlw, sdlh));
			return;
		}
		OglSdlSurface(vid.width, vid.height);
	}

	if (rendermode == render_soft)
	{
		SDL_RenderClear(renderer);
		SDL_RenderSetLogicalSize(renderer, width, height);
		// Set up Texture
		realwidth = width;
		realheight = height;
		if (texture != NULL)
		{
			SDL_DestroyTexture(texture);
		}

		if (!usesdl2soft)
		{
			sw_texture_format = SDL_PIXELFORMAT_RGB565;
		}
		else
		{
			bpp = 32;
			sw_texture_format = SDL_PIXELFORMAT_RGBA8888;
		}

		texture = SDL_CreateTexture(renderer, sw_texture_format, SDL_TEXTUREACCESS_STREAMING, width, height);

		// Set up SW surface
		if (vidSurface != NULL)
		{
			SDL_FreeSurface(vidSurface);
		}
		SDL_PixelFormatEnumToMasks(sw_texture_format, &bpp, &rmask, &gmask, &bmask, &amask);
		vidSurface = SDL_CreateRGBSurface(0, width, height, bpp, rmask, gmask, bmask, amask);
	}
}

//
//  Translates the SDL key into SRB2 key
//

static INT32 SDLatekey(SDL_Keycode sym)
{
	INT32 rc = sym + 0x80;

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

		case SDLK_NUMLOCKCLEAR:
			rc = KEY_NUMLOCK;
			break;
		case SDLK_SCROLLLOCK:
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

		case SDLK_KP_0:
			rc = KEY_KEYPAD0;
			break;
		case SDLK_KP_1:
			rc = KEY_KEYPAD1;
			break;
		case SDLK_KP_2:
			rc = KEY_KEYPAD2;
			break;
		case SDLK_KP_3:
			rc = KEY_KEYPAD3;
			break;
		case SDLK_KP_4:
			rc = KEY_KEYPAD4;
			break;
		case SDLK_KP_5:
			rc = KEY_KEYPAD5;
			break;
		case SDLK_KP_6:
			rc = KEY_KEYPAD6;
			break;
		case SDLK_KP_7:
			rc = KEY_KEYPAD7;
			break;
		case SDLK_KP_8:
			rc = KEY_KEYPAD8;
			break;
		case SDLK_KP_9:
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

		case SDLK_LMETA:
			rc = KEY_LEFTWIN;
			break;
		case SDLK_RMETA:
			rc = KEY_RIGHTWIN;
			break;

		case SDLK_MENU:
			rc = KEY_MENU;
			break;

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
	SDL_SetWindowGrab(window, SDL_FALSE);
}

void SDLforceUngrabMouse(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO && window != NULL)
	{
		SDL_SetWindowGrab(window, SDL_FALSE);
	}
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}

static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
#if 1
	(void)infoSurface;
	(void)SurfaceText;
	SDL2STUB();
#else
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
#endif
}

static void VID_Command_Info_f (void)
{
#if 0
	SDL2STUB();
#else
#if 0
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
#else
	if (!M_CheckParm("-noblit")) videoblitok = SDL_TRUE;
#endif
	SurfaceInfo(bufSurface, M_GetText("Current Engine Mode"));
	SurfaceInfo(vidSurface, M_GetText("Current Video Mode"));
#endif
}

static void VID_Command_ModeList_f(void)
{
	SDL2STUB();
#if 0
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
#endif
}

static void VID_Command_Mode_f (void)
{
	SDL2STUB();
#if 0
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
#endif
}

#if 0
#if defined(RPC_NO_WINDOWS_H)
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
#endif

static inline void SDLJoyRemap(event_t *event)
{
	(void)event;
}

static INT32 SDLJoyAxis(const Sint16 axis, evtype_t which)
{
	// -32768 to 32767
	INT32 raxis = axis/32;
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

static void Impl_HandleWindowEvent(SDL_WindowEvent evt)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;
	static SDL_bool mousefocus = SDL_TRUE;
	static SDL_bool kbfocus = SDL_TRUE;

	switch (evt.event)
	{
		case SDL_WINDOWEVENT_ENTER:
			mousefocus = SDL_TRUE;
			break;
		case SDL_WINDOWEVENT_LEAVE:
			mousefocus = SDL_FALSE;
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			kbfocus = SDL_TRUE;
			mousefocus = SDL_TRUE;
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			kbfocus = SDL_FALSE;
			mousefocus = SDL_FALSE;
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			break;
	}

	if (mousefocus && kbfocus)
	{
		if (!firsttimeonmouse)
		{
			if (cv_usemouse.value) I_StartupMouse();
		}
		//else firsttimeonmouse = SDL_FALSE;
		if (gamestate == GS_LEVEL)
		{
			if (!paused) I_ResumeSong(0); //resume it
		}
	}
	else if (!mousefocus && !kbfocus)
	{
		if (!disable_mouse)
		{
			SDLforceUngrabMouse();
		}
		if (!netgame && gamestate == GS_LEVEL && !demoplayback && !demorecording && !modeattacking)
		{
			paused = true;
		}
		memset(gamekeydown, 0, NUMKEYS); // TODO this is a scary memset
		if (gamestate == GS_LEVEL)
		{
			I_PauseSong(0);
		}

		if (MOUSE_MENU)
		{
			SDLdoUngrabMouse();
		}
	}

}

static void Impl_HandleKeyboardEvent(SDL_KeyboardEvent evt, Uint32 type)
{
	event_t event;
	if (type == SDL_KEYUP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_KEYDOWN)
	{
		event.type = ev_keydown;
	}
	else
	{
		return;
	}
	event.data1 = SDLatekey(evt.keysym.sym);
	if (event.data1) D_PostEvent(&event);
}

static void Impl_HandleMouseMotionEvent(SDL_MouseMotionEvent evt)
{
	event_t event;
	int wwidth, wheight;

	SDL_GetWindowSize(window, &wwidth, &wheight);

	if ((SDL_GetMouseFocus() != window && SDL_GetKeyboardFocus() != window))
	{
		SDLdoUngrabMouse();
		return;
	}

	if ((evt.x == realwidth/2) && (evt.y == realheight/2))
	{
		return;
	}
	else
	{
		event.data2 = (evt.xrel) * (wwidth / realwidth) * 2;
		event.data3 = -evt.yrel * (wheight / realheight) * 2;
	}
	
	event.type = ev_mouse;
	D_PostEvent(&event);

	HalfWarpMouse(wwidth, wheight);
}

static void Impl_HandleMouseButtonEvent(SDL_MouseButtonEvent evt, Uint32 type)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	/// \todo inputEvent.button.which
	if (USE_MOUSEINPUT)
	{
		if (type == SDL_MOUSEBUTTONUP)
		{
			event.type = ev_keyup;
		}
		else if (type == SDL_MOUSEBUTTONDOWN)
		{
			event.type = ev_keydown;
		}
		else return;
		if (evt.button == SDL_BUTTON_MIDDLE)
			event.data1 = KEY_MOUSE1+2;
		else if (evt.button == SDL_BUTTON_RIGHT)
			event.data1 = KEY_MOUSE1+1;
		else if (evt.button == SDL_BUTTON_LEFT)
			event.data1= KEY_MOUSE1;
		else if (evt.button <= MOUSEBUTTONS)
			event.data1 = KEY_MOUSE1 + evt.which - SDL_BUTTON_LEFT;
		if (event.type == ev_keyup || event.type == ev_keydown)
		{
			D_PostEvent(&event);
		}
	}
}

static void Impl_HandleMouseWheelEvent(SDL_MouseWheelEvent evt)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	if (evt.y > 0)
	{
		event.data1 = KEY_MOUSEWHEELUP;
		event.type = ev_keydown;
	}
	if (evt.y < 0)
	{
		event.data1 = KEY_MOUSEWHEELDOWN;
		event.type = ev_keydown;
	}
	if (evt.y == 0)
	{
		event.data1 = 0;
		event.type = ev_keyup;
	}
	if (event.type == ev_keyup || event.type == ev_keydown)
	{
		D_PostEvent(&event);
	}
}

static void Impl_HandleJoystickAxisEvent(SDL_JoyAxisEvent evt)
{
	event_t event;
	SDL_JoystickID joyid[2];

	// Determine the Joystick IDs for each current open joystick
	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo.dev);

	evt.axis++;
	event.data1 = event.data2 = event.data3 = INT32_MAX;

	if (evt.which == joyid[0])
	{
		event.type = ev_joystick;
	}
	else if (evt.which == joyid[1])
	{
		event.type = ev_joystick2;
	}
	else return;
	//axis
	if (evt.axis > JOYAXISSET*2)
		return;
	//vaule
	if (evt.axis%2)
	{
		event.data1 = evt.axis / 2;
		event.data2 = SDLJoyAxis(evt.value, event.type);
	}
	else
	{
		evt.axis--;
		event.data1 = evt.axis / 2;
		event.data3 = SDLJoyAxis(evt.value, event.type);
	}
	D_PostEvent(&event);
}

static void Impl_HandleJoystickButtonEvent(SDL_JoyButtonEvent evt, Uint32 type)
{
	event_t event;
	SDL_JoystickID joyid[2];

	// Determine the Joystick IDs for each current open joystick
	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo.dev);

	if (evt.which == joyid[0])
	{
		event.data1 = KEY_JOY1;
	}
	else if (evt.which == joyid[1])
	{
		event.data1 = KEY_2JOY1;
	}
	else return;
	if (type == SDL_JOYBUTTONUP)
	{
		event.type = ev_keyup;
	}
	else if (type == SDL_JOYBUTTONDOWN)
	{
		event.type = ev_keydown;
	}
	else return;
	if (evt.button < JOYBUTTONS)
	{
		event.data1 += evt.button;
	}
	else return;

	SDLJoyRemap(&event);
	if (event.type != ev_console) D_PostEvent(&event);
}

void I_GetEvent(void)
{
	SDL_Event evt;

	if (!graphics_started)
	{
		return;
	}

	while (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
			case SDL_WINDOWEVENT:
				Impl_HandleWindowEvent(evt.window);
				break;
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				Impl_HandleKeyboardEvent(evt.key, evt.type);
				break;
			case SDL_MOUSEMOTION:
				Impl_HandleMouseMotionEvent(evt.motion);
				break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				Impl_HandleMouseButtonEvent(evt.button, evt.type);
				break;
			case SDL_MOUSEWHEEL:
				Impl_HandleMouseWheelEvent(evt.wheel);
				break;
			case SDL_JOYAXISMOTION:
				Impl_HandleJoystickAxisEvent(evt.jaxis);
				break;
			case SDL_JOYBUTTONUP:
			case SDL_JOYBUTTONDOWN:
				Impl_HandleJoystickButtonEvent(evt.jbutton, evt.type);
				break;
			case SDL_QUIT:
				I_Quit();
				M_QuitResponse('y');
				break;
		}
	}

	// In order to make wheels act like buttons, we have to set their state to Up.
	// This is because wheel messages don't have an up/down state.
	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;

#if 0
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
				//if (USE_MOUSEINPUT) TODO SDL2 stub
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
			case SDL_QUIT:
				if (!sdlquit)
				{
					sdlquit = SDL_TRUE;
					M_QuitResponse('y');
				}
				break;
#if defined(RPC_NO_WINDOWS_H)
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
					SDLSetMode(realwidth, realheight, vid.bpp*8, surfaceFlagsW);
					if (vidSurface) SDL_SetColors(vidSurface, localPalette, 0, 256);
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
#endif
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

	I_GetMouseEvents();

	I_GetEvent();
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
	if (rendermode == render_none)
		return;
	if (exposevideo)
	{
#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			OglSdlFinishUpdate(cv_vidwait.value);
		}
		else
#endif
		if (rendermode == render_soft)
		{
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
		}
	}
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
	if (rendermode == render_none)
		return; //Alam: No software or OpenGl surface

	if (I_SkipFrame())
		return;

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (render_soft == rendermode && screens[0])
	{
		SDL_Rect rect;

		rect.x = 0;
		rect.y = 0;
		rect.w = vid.width;
		rect.h = vid.height;

		if (!bufSurface) //Double-Check
		{
			Impl_VideoSetupSDLBuffer();
		}
		if (bufSurface)
		{
			SDL_BlitSurface(bufSurface, NULL, vidSurface, &rect);
			// Fury -- there's no way around UpdateTexture, the GL backend uses it anyway
			SDL_LockSurface(vidSurface);
			SDL_UpdateTexture(texture, &rect, vidSurface->pixels, vidSurface->pitch);
			SDL_UnlockSurface(vidSurface);
		}
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
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
	//if (vidSurface) SDL_SetPaletteColors(vidSurface->format->palette, localPalette, 0, 256);
	// Fury -- SDL2 vidSurface is a 32-bit surface buffer copied to the texture. It's not palletized, like bufSurface.
	if (bufSurface) SDL_SetPaletteColors(bufSurface->format->palette, localPalette, 0, 256);
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
#if 0
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
#endif
		if (modeNum > MAXWINMODES)
			return NULL;

		sprintf(&vidModeName[modeNum][0], "%dx%d",
			windowedModes[modeNum][0],
			windowedModes[modeNum][1]);
	//}
	return &vidModeName[modeNum][0];
}

INT32 VID_GetModeForSize(INT32 w, INT32 h)
{
	int i;
	for (i = 0; i < MAXWINMODES; i++)
	{
		if (windowedModes[i][0] == w && windowedModes[i][1] == h)
		{
			return i;
		}
	}
	return -1;
#if 0
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
#endif
}

void VID_PrepareModeList(void)
{
	// Under SDL2, we just use the windowed modes list, and scale in windowed fullscreen.
	allow_fullscreen = true;
#if 0
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
#endif
}

static inline void SDLESSet(void)
{
	SDL2STUB();
}

static void SDLWMSet(void)
{
	SDL2STUB();
#if 0
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
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#endif
	SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
#endif
}

#if 0
static void* SDLGetDirect(void)
{
	// you can not use the video memory in pixels member in fullscreen mode
	return NULL;
}
#endif

INT32 VID_SetMode(INT32 modeNum)
{
	SDLdoUngrabMouse();

	vid.recalc = 1;
	vid.width = windowedModes[modeNum][0];
	vid.height = windowedModes[modeNum][1];
	vid.bpp = 1;

	Impl_SetWindowName("SRB2 "VERSIONSTRING);
	vid.modenum = modeNum; //VID_GetModeForSize(vidSurface->w,vidSurface->h);

	SDLSetMode(windowedModes[modeNum][0], windowedModes[modeNum][1], USE_FULLSCREEN);

	if (render_soft == rendermode)
	{
		if (bufSurface)
		{
			SDL_FreeSurface(bufSurface);
			bufSurface = NULL;
		}

		Impl_VideoSetupBuffer();
	}

	return SDL_TRUE;
}

static SDL_bool Impl_CreateWindow(SDL_bool fullscreen)
{
	int flags = 0;
	if (window != NULL)
	{
		return SDL_FALSE;
	}

	if (fullscreen)
	{
		flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		window = SDL_CreateWindow("SRB2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				realwidth, realheight, flags | SDL_WINDOW_OPENGL);
		if (window != NULL)
		{
			sdlglcontext = SDL_GL_CreateContext(window);
			if (sdlglcontext == NULL)
			{
				SDL_DestroyWindow(window);
				I_Error("Failed to create a GL context: %s\n", SDL_GetError());
			}
			else
			{
				SDL_GL_MakeCurrent(window, sdlglcontext);
			}
		}
		else return SDL_FALSE;
	}
#endif
	if (rendermode == render_soft)
	{
		window = SDL_CreateWindow("SRB2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				realwidth, realheight, flags);
		if (window != NULL)
		{
			renderer = SDL_CreateRenderer(window, -1, (usesdl2soft ? SDL_RENDERER_SOFTWARE : 0) | (cv_vidwait.value && !usesdl2soft ? SDL_RENDERER_PRESENTVSYNC : 0));
			if (renderer != NULL)
			{
				SDL_RenderSetLogicalSize(renderer, BASEVIDWIDTH, BASEVIDHEIGHT);
			}
			else return SDL_FALSE;
		}
		else return SDL_FALSE;
	}

	return SDL_TRUE;
}

static void Impl_SetWindowName(const char *title)
{
	if (window != NULL)
	{
		return;
	}
	SDL2STUB();
	SDL_SetWindowTitle(window, title);
}

static void Impl_SetWindowIcon(void)
{
	if (window == NULL || icoSurface == NULL)
	{
		return;
	}
	SDL2STUB();
	SDL_SetWindowIcon(window, icoSurface);
}

static void Impl_VideoSetupSDLBuffer(void)
{
	if (bufSurface != NULL)
	{
		SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}
	// Set up the SDL palletized buffer (copied to vidbuffer before being rendered to texture)
	if (vid.bpp == 1)
	{
		bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,8,
			(int)vid.rowbytes,0x00000000,0x00000000,0x00000000,0x00000000); // 256 mode
	}
	else if (vid.bpp == 2) // Fury -- don't think this is used at all anymore
	{
		bufSurface = SDL_CreateRGBSurfaceFrom(screens[0],vid.width,vid.height,15,
			(int)vid.rowbytes,0x00007C00,0x000003E0,0x0000001F,0x00000000); // 555 mode
	}
	if (bufSurface)
	{
		SDL_SetPaletteColors(bufSurface->format->palette, localPalette, 0, 256);
	}
	else
	{
		I_Error("%s", M_GetText("No system memory for SDL buffer surface\n"));
	}
}

static void Impl_VideoSetupBuffer(void)
{
	// Set up game's software render buffer
	if (rendermode == render_soft)
	{
		vid.rowbytes = vid.width * vid.bpp;
		vid.direct = NULL;
		vid.buffer = malloc(vid.rowbytes*vid.height*NUMSCREENS);
		if (vid.buffer)
		{
			memset(vid.buffer,0x00,vid.rowbytes*vid.height*NUMSCREENS);
		}
		else
		{
			I_Error("%s", M_GetText("Not enough memory for video buffer\n"));
		}
	}
}

void I_StartupGraphics(void)
{
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
	disable_mouse = M_CheckParm("-nomouse");
	disable_fullscreen = M_CheckParm("-win") ? 1 : 0;

	keyboard_started = true;

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
#endif
	{
		char vd[100]; //stack space for video name
		//CONS_Printf(M_GetText("Starting up with video driver : %s\n"), SDL_VideoDriverName(vd,100));
		if (strncasecmp(vd, "gcvideo", 8) == 0 || strncasecmp(vd, "fbcon", 6) == 0 || strncasecmp(vd, "wii", 4) == 0 || strncasecmp(vd, "psl1ght", 8) == 0)
			framebuffer = SDL_TRUE;
	}
	if (M_CheckParm("-software"))
	{
		rendermode = render_soft;
	}
	if (M_CheckParm("-softblit"))
	{
		usesdl2soft = SDL_TRUE;
	}
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY>>1,SDL_DEFAULT_REPEAT_INTERVAL<<2);
	SDLESSet();
	VID_Command_ModeList_f();
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
		HWD.pfnStartScreenWipe  = hwSym("StartScreenWipe",NULL);
		HWD.pfnEndScreenWipe    = hwSym("EndScreenWipe",NULL);
		HWD.pfnDoScreenWipe     = hwSym("DoScreenWipe",NULL);
		HWD.pfnDrawIntermissionBG=hwSym("DrawIntermissionBG",NULL);
		HWD.pfnMakeScreenTexture= hwSym("MakeScreenTexture",NULL);
		// check gl renderer lib
		if (HWD.pfnGetRenderVersion() != VERSION)
			I_Error("%s", M_GetText("The version of the renderer doesn't match the version of the executable\nBe sure you have installed SRB2 properly.\n"));
		if (!HWD.pfnInit(I_Error)) // let load the OpenGL library
		{
			rendermode = render_soft;
		}
	}
#endif

	// Fury: we do window initialization after GL setup to allow
	// SDL_GL_LoadLibrary to work well on Windows
	
	// Create window
	//Impl_CreateWindow(USE_FULLSCREEN);
	//Impl_SetWindowName("SRB2");
	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));
	
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
	// Window icon
#ifdef HAVE_IMAGE
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif
	Impl_SetWindowIcon();

	VID_SetMode(VID_GetModeForSize(BASEVIDWIDTH, BASEVIDHEIGHT));

	if (M_CheckParm("-nomousegrab"))
		mousegrabok = SDL_FALSE;
#if 0 // defined (_DEBUG)
	else
	{
		char videodriver[4] = {'S','D','L',0};
		if (!M_CheckParm("-mousegrab") &&
		    *strncpy(videodriver, SDL_GetCurrentVideoDriver(), 4) != '\0' &&
		    strncasecmp("x11",videodriver,4) == 0)
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
	}

	// was graphics initialized anyway?
	if (!graphics_started)
		return;
	CONS_Printf("I_ShutdownGraphics: ");
	graphics_started = false;
	CONS_Printf("%s", M_GetText("shut down\n"));
#ifdef HWRENDER
	if (GLUhandle)
		hwClose(GLUhandle);
	if (sdlglcontext)
	{
		SDL_GL_DeleteContext(sdlglcontext);
	}
#endif
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	framebuffer = SDL_FALSE;
}
#endif
