
// Emacs style mode select   -*- C++ -*-
// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2023 by Sonic Team Junior.
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
/// \file i_video.c
/// \brief SRB2 graphics stuff for SDL

#include <stdlib.h>

#include <signal.h>

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED
#include "SDL.h"

#ifdef _MSC_VER
#include <windows.h>
#pragma warning(default : 4214 4244)
#endif

#ifdef HAVE_TTF
#include "i_ttf.h"
#endif

#ifdef HAVE_IMAGE
#include "SDL_image.h"
#elif defined (__unix__) || (!defined(__APPLE__) && defined (UNIXCOMMON)) // Windows & Mac don't need this, as SDL will do it for us.
#define LOAD_XPM //I want XPM!
#include "IMG_xpm.c" //Alam: I don't want to add SDL_Image.dll/so
#define HAVE_IMAGE //I have SDL_Image, sortof
#endif

#ifdef HAVE_IMAGE
#include "SDL_icon.xpm"
#endif

#include "../doomdef.h"

#ifdef _WIN32
#include "SDL_syswm.h"
#endif

#include "../doomstat.h"
#include "../i_system.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../m_menu.h"
#include "../d_main.h"
#include "../s_sound.h"
#include "../i_joy.h"
#include "../st_stuff.h"
#include "../hu_stuff.h"
#include "../g_game.h"
#include "../i_video.h"
#include "../console.h"
#include "../command.h"
#include "../r_main.h"
#include "../lua_script.h"
#include "../lua_libs.h"
#include "../lua_hook.h"

#include "sdlmain.h"

#ifdef HWRENDER
#include "../hardware/hw_main.h"
#include "../hardware/hw_drv.h"
#include "hwsym_sdl.h"
#include "ogl_sdl.h"
#endif

rendermode_t rendermode = render_soft;
rendermode_t chosenrendermode = render_none; // set by command line arguments

boolean highcolor = false;

static void VidWaitChanged(void);

// synchronize page flipping with screen refresh
consvar_t cv_vidwait = CVAR_INIT ("vid_wait", "On", CV_SAVE | CV_CALL, CV_OnOff, VidWaitChanged);
static consvar_t cv_stretch = CVAR_INIT ("stretch", "Off", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL);
static consvar_t cv_alwaysgrabmouse = CVAR_INIT ("alwaysgrabmouse", "Off", CV_SAVE, CV_OnOff, NULL);

#if defined(__ANDROID__)
static void Impl_SetColorBufferDepth(INT32 red, INT32 green, INT32 blue, INT32 alpha);
#endif

UINT8 graphics_started = 0; // Is used in console.c and screen.c

// To disable fullscreen at startup; is set in VID_PrepareModeList
boolean allow_fullscreen = false;
static SDL_bool disable_fullscreen = SDL_FALSE;

#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value

static SDL_bool disable_mouse = SDL_FALSE;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && havefocus)
#define MOUSE_MENU false //(!disable_mouse && cv_usemouse.value && menuactive && !USE_FULLSCREEN)
#define MOUSEBUTTONS_MAX MOUSEBUTTONS

// Total mouse motion X/Y offsets
static      INT32        mousemovex = 0, mousemovey = 0;

// SDL vars
static      SDL_Surface *vidSurface = NULL;
static      SDL_Surface *bufSurface = NULL;
static      SDL_Color    localPalette[256];
static       SDL_bool    mousegrabok = SDL_TRUE;
static       SDL_bool    wrapmouseok = SDL_FALSE;
#define HalfWarpMouse(x,y) if (wrapmouseok) SDL_WarpMouseInWindow(window, (Uint16)(x/2),(Uint16)(y/2))
static       SDL_bool    usesdl2soft = SDL_FALSE;
static       SDL_bool    borderlesswindow = SDL_FALSE;

Uint16      realwidth = BASEVIDWIDTH;
Uint16      realheight = BASEVIDHEIGHT;

SDL_Window   *window;
SDL_Renderer *renderer;
static SDL_Texture  *texture;
static SDL_bool      havefocus = SDL_TRUE;

static UINT32 refresh_rate;

static boolean video_init = false;

static SDL_bool Impl_CreateWindow(SDL_bool fullscreen);

static void Impl_VideoSetupSurfaces(int width, int height);
static void Impl_VideoSetupBuffer(void);

static void Impl_SetupSoftwareBuffer(void);

static void Impl_InitOpenGL(void);

#if !defined(__ANDROID__) && defined(HAVE_IMAGE)
#define USE_WINDOW_ICON
#endif

#ifdef USE_WINDOW_ICON
static void Impl_SetWindowIcon(void);
static SDL_Surface *icoSurface = NULL;
#endif

static char vidModeName[MAXWINMODES][32];
static const char *fallback_resolution_name = "Fallback";

#define VIDEO_INIT_ERROR(str) { \
	if (!graphics_started) \
		I_Error(str, SDL_GetError()); \
	else \
		CONS_Printf(str "\n", SDL_GetError()); \
}

static SDL_bool Impl_RenderContextCreate(void)
{
	if (rendermode != render_opengl)
	{
		int flags = 0; // Use this to set SDL_RENDERER_* flags now

		if (usesdl2soft)
			flags |= SDL_RENDERER_SOFTWARE;
		else if (cv_vidwait.value)
		{
#if SDL_VERSION_ATLEAST(2, 0, 18)
			// If SDL is new enough, we can turn off vsync later.
			flags |= SDL_RENDERER_PRESENTVSYNC;
#else
			// However, if it isn't, we should just silently turn vid_wait off
			// This is because the renderer will be created before the config
			// is read and vid_wait is set from the user's preferences, and thus
			// vid_wait will have no effect.
			CV_StealthSetValue(&cv_vidwait, 0);
#endif
		}

		if (!renderer)
			renderer = SDL_CreateRenderer(window, -1, flags);

		if (renderer == NULL)
		{
			VIDEO_INIT_ERROR("Couldn't create rendering context: %s");
			return SDL_FALSE;
		}
	}

#ifdef HWRENDER
	if (rendermode == render_opengl && vid.glstate != VID_GL_LIBRARY_ERROR)
	{
		if (sdlglcontext == NULL)
		{
			sdlglcontext = SDL_GL_CreateContext(window);

			if (sdlglcontext == NULL)
			{
				VIDEO_INIT_ERROR("Couldn't create OpenGL context: %s");
				return SDL_FALSE;
			}
		}
	}
#endif

	return SDL_TRUE;
}

static SDL_bool Impl_RenderContextReset(void)
{
	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
		texture = NULL; // Destroying a renderer also destroys all of its textures
	}
	renderer = NULL;

	if (Impl_RenderContextCreate() == SDL_FALSE)
		return SDL_FALSE;

	if (vidSurface != NULL)
	{
		SDL_FreeSurface(vidSurface);
		vidSurface = NULL;
	}

	if (bufSurface != NULL)
	{
		SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

#ifdef HWRENDER
	if (rendermode == render_opengl)
	{
		SDL_GL_MakeCurrent(window, sdlglcontext);
		SDL_GL_SetSwapInterval(cv_vidwait.value ? 1 : 0);

		OglSdlSurface(realwidth, realheight);
		HWR_Startup();
	}
	else
#endif
	{
		SDL_RenderClear(renderer);
		SDL_RenderSetLogicalSize(renderer, realwidth, realheight);
		Impl_VideoSetupSurfaces(realwidth, realheight);
	}

	return SDL_TRUE;
}

static void Impl_VideoSetupSurfaces(int width, int height)
{
	int bpp = 16;
	int sw_texture_format = SDL_PIXELFORMAT_ABGR8888;

#if !defined(__ANDROID__)
	if (!usesdl2soft)
	{
		sw_texture_format = SDL_PIXELFORMAT_RGB565;
	}
	else
#endif
	{
		bpp = 32;
		sw_texture_format = SDL_PIXELFORMAT_RGBA8888;
	}

	if (texture == NULL)
		texture = SDL_CreateTexture(renderer, sw_texture_format, SDL_TEXTUREACCESS_STREAMING, width, height);

	// Set up SW surface
	if (vidSurface == NULL)
	{
		Uint32 rmask;
		Uint32 gmask;
		Uint32 bmask;
		Uint32 amask;

		SDL_PixelFormatEnumToMasks(sw_texture_format, &bpp, &rmask, &gmask, &bmask, &amask);
		vidSurface = SDL_CreateRGBSurface(0, width, height, bpp, rmask, gmask, bmask, amask);
	}
}

static void Impl_SetupSoftwareBuffer(void)
{
	// Set up game's software render buffer
	size_t size;

	vid.rowbytes = vid.width * vid.bpp;
	vid.direct = NULL;

	free(vid.buffer);

	size = vid.rowbytes*vid.height * NUMSCREENS;
	vid.buffer = malloc(size);

	if (vid.buffer)
	{
		// Clear the buffer
		// HACK: Wasn't sure where else to put this.
		memset(vid.buffer, 31, size);
	}
	else
		I_Error("%s", M_GetText("Not enough memory for video buffer\n"));
}

static SDL_Rect src_rect = { 0, 0, 0, 0 };

static SDL_bool SDLSetMode(INT32 width, INT32 height, SDL_bool fullscreen, SDL_bool reposition)
{
	static SDL_bool wasfullscreen = SDL_FALSE;
	int fullscreen_type = SDL_WINDOW_FULLSCREEN_DESKTOP;

	src_rect.w = realwidth = width;
	src_rect.h = realheight = height;

	if (window)
	{
		if (fullscreen)
		{
			wasfullscreen = SDL_TRUE;
			SDL_SetWindowFullscreen(window, fullscreen_type);
		}
		else // windowed mode
		{
			if (wasfullscreen)
			{
				wasfullscreen = SDL_FALSE;
				SDL_SetWindowFullscreen(window, 0);
			}

			SDL_SetWindowSize(window, width, height);

			if (reposition)
			{
				// Reposition window only in windowed mode
				SDL_SetWindowPosition(window,
					SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetWindowDisplayIndex(window)),
					SDL_WINDOWPOS_CENTERED_DISPLAY(SDL_GetWindowDisplayIndex(window))
				);
			}
		}
	}
	else
	{
		if (Impl_CreateWindow(fullscreen) == SDL_FALSE)
			return SDL_FALSE;

		wasfullscreen = fullscreen;
		SDL_SetWindowSize(window, width, height);
		if (fullscreen)
			SDL_SetWindowFullscreen(window, fullscreen_type);
	}

	if (Impl_RenderContextReset() == SDL_FALSE)
		I_Error("Couldn't create or reset rendering context");

	if (vid.buffer)
	{
		free(vid.buffer);
		vid.buffer = NULL;
	}

	return SDL_TRUE;
}

static void VidWaitChanged(void)
{
	if (renderer && rendermode == render_soft)
	{
#if SDL_VERSION_ATLEAST(2, 0, 18)
		SDL_RenderSetVSync(renderer, cv_vidwait.value ? 1 : 0);
#endif
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && sdlglcontext != NULL && SDL_GL_GetCurrentContext() == sdlglcontext)
	{
		SDL_GL_SetSwapInterval(cv_vidwait.value ? 1 : 0);
	}
#endif
}

static UINT32 VID_GetRefreshRate(void)
{
	int index = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode m;

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
	{
		// Video not init yet.
		return 0;
	}

	if (SDL_GetCurrentDisplayMode(index, &m) != 0)
	{
		// Error has occurred.
		return 0;
	}

	return m.refresh_rate;
}

static INT32 Impl_SDL_Scancode_To_Keycode(SDL_Scancode code)
{
	if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z)
	{
		// get lowercase ASCII
		return code - SDL_SCANCODE_A + 'a';
	}
	if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9)
	{
		return code - SDL_SCANCODE_1 + '1';
	}
	else if (code == SDL_SCANCODE_0)
	{
		return '0';
	}
	if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10)
	{
		return KEY_F1 + (code - SDL_SCANCODE_F1);
	}
	switch (code)
	{
		// F11 and F12 are separated from the rest of the function keys
		case SDL_SCANCODE_F11: return KEY_F11;
		case SDL_SCANCODE_F12: return KEY_F12;

		case SDL_SCANCODE_KP_0: return KEY_KEYPAD0;
		case SDL_SCANCODE_KP_1: return KEY_KEYPAD1;
		case SDL_SCANCODE_KP_2: return KEY_KEYPAD2;
		case SDL_SCANCODE_KP_3: return KEY_KEYPAD3;
		case SDL_SCANCODE_KP_4: return KEY_KEYPAD4;
		case SDL_SCANCODE_KP_5: return KEY_KEYPAD5;
		case SDL_SCANCODE_KP_6: return KEY_KEYPAD6;
		case SDL_SCANCODE_KP_7: return KEY_KEYPAD7;
		case SDL_SCANCODE_KP_8: return KEY_KEYPAD8;
		case SDL_SCANCODE_KP_9: return KEY_KEYPAD9;

		case SDL_SCANCODE_RETURN:         return KEY_ENTER;
		case SDL_SCANCODE_ESCAPE:         return KEY_ESCAPE;
		case SDL_SCANCODE_BACKSPACE:      return KEY_BACKSPACE;
		case SDL_SCANCODE_TAB:            return KEY_TAB;
		case SDL_SCANCODE_SPACE:          return KEY_SPACE;
		case SDL_SCANCODE_MINUS:          return KEY_MINUS;
		case SDL_SCANCODE_EQUALS:         return KEY_EQUALS;
		case SDL_SCANCODE_LEFTBRACKET:    return '[';
		case SDL_SCANCODE_RIGHTBRACKET:   return ']';
		case SDL_SCANCODE_BACKSLASH:      return '\\';
		case SDL_SCANCODE_NONUSHASH:      return '#';
		case SDL_SCANCODE_SEMICOLON:      return ';';
		case SDL_SCANCODE_APOSTROPHE:     return '\'';
		case SDL_SCANCODE_GRAVE:          return '`';
		case SDL_SCANCODE_COMMA:          return ',';
		case SDL_SCANCODE_PERIOD:         return '.';
		case SDL_SCANCODE_SLASH:          return '/';
		case SDL_SCANCODE_CAPSLOCK:       return KEY_CAPSLOCK;
		case SDL_SCANCODE_PRINTSCREEN:    return 0; // undefined?
		case SDL_SCANCODE_SCROLLLOCK:     return KEY_SCROLLLOCK;
		case SDL_SCANCODE_PAUSE:          return KEY_PAUSE;
		case SDL_SCANCODE_INSERT:         return KEY_INS;
		case SDL_SCANCODE_HOME:           return KEY_HOME;
		case SDL_SCANCODE_PAGEUP:         return KEY_PGUP;
		case SDL_SCANCODE_DELETE:         return KEY_DEL;
		case SDL_SCANCODE_END:            return KEY_END;
		case SDL_SCANCODE_PAGEDOWN:       return KEY_PGDN;
		case SDL_SCANCODE_RIGHT:          return KEY_RIGHTARROW;
		case SDL_SCANCODE_LEFT:           return KEY_LEFTARROW;
		case SDL_SCANCODE_DOWN:           return KEY_DOWNARROW;
		case SDL_SCANCODE_UP:             return KEY_UPARROW;
		case SDL_SCANCODE_NUMLOCKCLEAR:   return KEY_NUMLOCK;
		case SDL_SCANCODE_KP_DIVIDE:      return KEY_KPADSLASH;
		case SDL_SCANCODE_KP_MULTIPLY:    return '*'; // undefined?
		case SDL_SCANCODE_KP_MINUS:       return KEY_MINUSPAD;
		case SDL_SCANCODE_KP_PLUS:        return KEY_PLUSPAD;
		case SDL_SCANCODE_KP_ENTER:       return KEY_ENTER;
		case SDL_SCANCODE_KP_PERIOD:      return KEY_KPADDEL;
		case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

		case SDL_SCANCODE_LSHIFT: return KEY_LSHIFT;
		case SDL_SCANCODE_RSHIFT: return KEY_RSHIFT;
		case SDL_SCANCODE_LCTRL:  return KEY_LCTRL;
		case SDL_SCANCODE_RCTRL:  return KEY_RCTRL;
		case SDL_SCANCODE_LALT:   return KEY_LALT;
		case SDL_SCANCODE_RALT:   return KEY_RALT;
		case SDL_SCANCODE_LGUI:   return KEY_LEFTWIN;
		case SDL_SCANCODE_RGUI:   return KEY_RIGHTWIN;

		default:                  break;
	}

	return 0;
}

static boolean IgnoreMouse(void)
{
	if (cv_alwaysgrabmouse.value)
		return false;
	if (menuactive)
		return !M_MouseNeeded();
	if (paused || con_destlines || chat_on)
		return true;
	if (gamestate != GS_LEVEL && gamestate != GS_INTERMISSION &&
			gamestate != GS_CONTINUING && gamestate != GS_CUTSCENE)
		return true;
	if (!mousegrabbedbylua)
		return true;
	return false;
}

static void SDLdoGrabMouse(void)
{
	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetWindowGrab(window, SDL_TRUE);
	if (SDL_SetRelativeMouseMode(SDL_TRUE) == 0) // already warps mouse if successful
		wrapmouseok = SDL_TRUE; // TODO: is wrapmouseok or HalfWarpMouse needed anymore?
}

static void SDLdoUngrabMouse(void)
{
	SDL_ShowCursor(SDL_ENABLE);
	SDL_SetWindowGrab(window, SDL_FALSE);
	wrapmouseok = SDL_FALSE;
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void SDLforceUngrabMouse(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO && window != NULL)
		SDLdoUngrabMouse();
}

void I_UpdateMouseGrab(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO && window != NULL
	&& SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window
	&& USE_MOUSEINPUT && !IgnoreMouse())
		SDLdoGrabMouse();
}

void I_SetMouseGrab(boolean grab)
{
	if (grab)
		SDLdoGrabMouse();
	else
		SDLdoUngrabMouse();
}

static void VID_Command_NumModes_f (void)
{
	CONS_Printf(M_GetText("%d video mode(s) available(s)\n"), VID_NumModes());
}

// SDL2 doesn't have SDL_GetVideoSurface or a lot of the SDL_Surface flags that SDL 1.2 had
static void SurfaceInfo(const SDL_Surface *infoSurface, const char *SurfaceText)
{
	INT32 vfBPP;

	if (!infoSurface)
		return;

	if (!SurfaceText)
		SurfaceText = M_GetText("Unknown Surface");

	vfBPP = infoSurface->format?infoSurface->format->BitsPerPixel:0;

	CONS_Printf("\x82" "%s\n", SurfaceText);
	CONS_Printf(M_GetText(" %ix%i at %i bit color\n"), infoSurface->w, infoSurface->h, vfBPP);

	if (infoSurface->flags&SDL_PREALLOC)
		CONS_Printf("%s", M_GetText(" Uses preallocated memory\n"));
	else
		CONS_Printf("%s", M_GetText(" Stored in system memory\n"));
	if (infoSurface->flags&SDL_RLEACCEL)
		CONS_Printf("%s", M_GetText(" Colorkey RLE acceleration blit\n"));
}

static void VID_Command_Info_f (void)
{
	SurfaceInfo(bufSurface, M_GetText("Current Engine Mode"));
	SurfaceInfo(vidSurface, M_GetText("Current Video Mode"));
}

static void VID_Command_ModeList_f(void)
{
	for (INT32 i = 0; i < MAXWINMODES; i++)
		CONS_Printf("%2d: %dx%d\n", i, windowedModes[i][0], windowedModes[i][1]);
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
	{
		if (modenum < 0)
			modenum = 0;
		if (modenum >= MAXWINMODES)
			modenum = MAXWINMODES-1;

		vid.change.width = windowedModes[modenum][0];
		vid.change.height = windowedModes[modenum][1];
		vid.change.set = VID_RESOLUTION_CHANGED;
	}
}

static void Impl_SetFocused(boolean focused)
{
	window_notinfocus = !focused;

	if (window_notinfocus)
	{
		if (! cv_playmusicifunfocused.value)
			S_PauseAudio();
		if (! cv_playsoundsifunfocused.value)
			S_StopSounds();

		memset(gamekeydown, 0, NUMKEYS); // TODO this is a scary memset
	}
	else if (!paused)
		S_ResumeAudio();
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
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			SCR_SetWindowSize(evt.data1, evt.data2);
			break;
	}

	if (mousefocus && kbfocus)
	{
		// Tell game we got focus back, resume music if necessary
		Impl_SetFocused(true);

		if (!firsttimeonmouse && cv_usemouse.value)
			I_StartupMouse();

		if (USE_MOUSEINPUT && !IgnoreMouse())
			SDLdoGrabMouse();
	}
	else if (!mousefocus && !kbfocus)
	{
		// Tell game we lost focus, pause music
		Impl_SetFocused(false);

		if (!disable_mouse)
			SDLforceUngrabMouse();

		if (MOUSE_MENU)
			SDLdoUngrabMouse();
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
	event.key = Impl_SDL_Scancode_To_Keycode(evt.keysym.scancode);

	event.repeated = (evt.repeat != 0);

	if (event.key) D_PostEvent(&event);
}

static void Impl_HandleMouseMotionEvent(SDL_MouseMotionEvent evt)
{
	static boolean firstmove = true;

	if (USE_MOUSEINPUT)
	{
		if ((SDL_GetMouseFocus() != window && SDL_GetKeyboardFocus() != window) || (IgnoreMouse() && !firstmove))
		{
			SDLdoUngrabMouse();
			firstmove = false;
			return;
		}

		// If using relative mouse mode, don't post an event_t just now,
		// add on the offsets so we can make an overall event later.
		if (SDL_GetRelativeMouseMode())
		{
			if (SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window)
			{
				mousemovex += evt.xrel;
				mousemovey += evt.yrel;
				SDL_SetWindowGrab(window, SDL_TRUE);
			}
			firstmove = false;
			return;
		}

		// If the event is from warping the pointer to middle
		// of the screen then ignore it.
		if ((evt.x == realwidth/2) && (evt.y == realheight/2))
		{
			firstmove = false;
			return;
		}

		// Don't send an event_t if not in relative mouse mode anymore,
		// just grab and set relative mode
		// this fixes the stupid camera jerk on mouse entering bug
		// -- Monster Iestyn
		if (SDL_GetMouseFocus() == window && SDL_GetKeyboardFocus() == window)
		{
			SDLdoGrabMouse();
		}
	}

	firstmove = false;
}

static void Impl_HandleMouseButtonEvent(SDL_MouseButtonEvent evt, Uint32 type)
{
	event_t event;

	SDL_memset(&event, 0, sizeof(event_t));

	// Ignore the event if the mouse is not actually focused on the window.
	// This can happen if you used the mouse to restore keyboard focus;
	// this apparently makes a mouse button down event but not a mouse button up event,
	// resulting in whatever key was pressed down getting "stuck" if we don't ignore it.
	// -- Monster Iestyn (28/05/18)
	if (SDL_GetMouseFocus() != window || IgnoreMouse())
		return;

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
			event.key = KEY_MOUSE1+2;
		else if (evt.button == SDL_BUTTON_RIGHT)
			event.key = KEY_MOUSE1+1;
		else if (evt.button == SDL_BUTTON_LEFT)
			event.key = KEY_MOUSE1;
		else if (evt.button == SDL_BUTTON_X1)
			event.key = KEY_MOUSE1+3;
		else if (evt.button == SDL_BUTTON_X2)
			event.key = KEY_MOUSE1+4;
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
		event.key = KEY_MOUSEWHEELUP;
		event.type = ev_keydown;
	}
	if (evt.y < 0)
	{
		event.key = KEY_MOUSEWHEELDOWN;
		event.type = ev_keydown;
	}
	if (evt.y == 0)
	{
		event.key = 0;
		event.type = ev_keyup;
	}
	if (event.type == ev_keyup || event.type == ev_keydown)
	{
		D_PostEvent(&event);
	}
}

static INT32 SDLJoyAxis(const Sint16 axis, evtype_t which)
{
	// -32768 to 32767
	INT32 raxis = axis/32;
	JoyType_t *Joystick_p = (which == ev_joystick2) ? &Joystick2 : &Joystick;
	SDLJoyInfo_t *JoyInfo_p = (which == ev_joystick2) ? &JoyInfo2 : &JoyInfo;

	if (Joystick_p->bGamepadStyle)
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
		raxis = JoyInfo_p->scale!=1?((raxis/JoyInfo_p->scale)*JoyInfo_p->scale):raxis;

	return raxis;
}

static void Impl_HandleJoystickAxisEvent(SDL_JoyAxisEvent evt)
{
	event_t event;

	evt.axis++;
	if (evt.axis > JOYAXISSET*2)
		return;

	event.key = event.x = event.y = INT32_MAX;

	SDL_JoystickID joyid[2];

	// Determine the Joystick IDs for each current open joystick
	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	if (evt.which == joyid[0])
		event.type = ev_joystick;
	else if (evt.which == joyid[1])
		event.type = ev_joystick2;
	else
		return;

	if (evt.axis%2)
	{
		event.key = evt.axis / 2;
		event.x = SDLJoyAxis(evt.value, event.type);
	}
	else
	{
		evt.axis--;
		event.key = evt.axis / 2;
		event.y = SDLJoyAxis(evt.value, event.type);
	}

	D_PostEvent(&event);
}

#if 0
static void Impl_HandleJoystickHatEvent(SDL_JoyHatEvent evt)
{
	event_t event;
	SDL_JoystickID joyid[2];

	// Determine the Joystick IDs for each current open joystick
	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	if (evt.hat >= JOYHATS)
		return; // ignore hats with too high an index

	if (evt.which == joyid[0])
	{
		event.key = KEY_HAT1 + (evt.hat*4);
	}
	else if (evt.which == joyid[1])
	{
		event.key = KEY_2HAT1 + (evt.hat*4);
	}
	else return;

	// NOTE: UNFINISHED
}
#endif

static void Impl_HandleJoystickButtonEvent(SDL_JoyButtonEvent evt, Uint32 type)
{
	event_t event;

	SDL_JoystickID joyid[2];

	// Determine the Joystick IDs for each current open joystick
	joyid[0] = SDL_JoystickInstanceID(JoyInfo.dev);
	joyid[1] = SDL_JoystickInstanceID(JoyInfo2.dev);

	if (evt.which == joyid[0])
	{
		event.key = KEY_JOY1;
	}
	else if (evt.which == joyid[1])
	{
		event.key = KEY_2JOY1;
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
		event.key += evt.button;
	}
	else return;

	D_PostEvent(&event);
}

static void Impl_HandleVideoEvent(SDL_Event *evt)
{
	switch (evt->type)
	{
		case SDL_WINDOWEVENT:
			Impl_HandleWindowEvent(evt->window);
			break;
		default:
			break;
	}
}

void I_GetEvent(void)
{
	SDL_Event evt;

	mousemovex = mousemovey = 0;

	while (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
			default:
				Impl_HandleVideoEvent(&evt);
				break;
			// TODO: Move input code out of this file, desperately
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				Impl_HandleKeyboardEvent(evt.key, evt.type);
				break;
			case SDL_MOUSEMOTION:
				//if (!mouseMotionOnce)
				Impl_HandleMouseMotionEvent(evt.motion);
				//mouseMotionOnce = 1;
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
#if 0
			case SDL_JOYHATMOTION:
				Impl_HandleJoystickHatEvent(evt.jhat)
				break;
#endif
			case SDL_JOYBUTTONUP:
			case SDL_JOYBUTTONDOWN:
				Impl_HandleJoystickButtonEvent(evt.jbutton, evt.type);
				break;
			case SDL_JOYDEVICEADDED:
				{
					INT32 index = evt.jdevice.which;
					SDL_Joystick *newjoy = SDL_JoystickOpen(index);

					CONS_Debug(DBG_GAMELOGIC, "Joystick device index %d added\n", index + 1);

					// Because SDL's device index is unstable, we're going to cheat here a bit:
					// For the first joystick setting that is NOT active:
					// 1. Set cv_usejoystickX.value to the new device index (this does not change what is written to config.cfg)
					// 2. Set OTHERS' cv_usejoystickX.value to THEIR new device index, because it likely changed
					//    * If device doesn't exist, switch cv_usejoystick back to default value (.string)
					//      * BUT: If that default index is being occupied, use ANOTHER cv_usejoystick's default value!
					if (newjoy && (!JoyInfo.dev || !SDL_JoystickGetAttached(JoyInfo.dev))
						&& JoyInfo2.dev != newjoy) // don't override a currently active device
					{
						cv_usejoystick.value = index + 1;

						if (JoyInfo2.dev)
							cv_usejoystick2.value = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
						else if (atoi(cv_usejoystick2.string) != JoyInfo.oldjoy
								&& atoi(cv_usejoystick2.string) != cv_usejoystick.value)
							cv_usejoystick2.value = atoi(cv_usejoystick2.string);
						else if (atoi(cv_usejoystick.string) != JoyInfo.oldjoy
								&& atoi(cv_usejoystick.string) != cv_usejoystick.value)
							cv_usejoystick2.value = atoi(cv_usejoystick.string);
						else // we tried...
							cv_usejoystick2.value = 0;
					}
					else if (newjoy && (!JoyInfo2.dev || !SDL_JoystickGetAttached(JoyInfo2.dev))
						&& JoyInfo.dev != newjoy) // don't override a currently active device
					{
						cv_usejoystick2.value = index + 1;

						if (JoyInfo.dev)
							cv_usejoystick.value = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
						else if (atoi(cv_usejoystick.string) != JoyInfo2.oldjoy
								&& atoi(cv_usejoystick.string) != cv_usejoystick2.value)
							cv_usejoystick.value = atoi(cv_usejoystick.string);
						else if (atoi(cv_usejoystick2.string) != JoyInfo2.oldjoy
								&& atoi(cv_usejoystick2.string) != cv_usejoystick2.value)
							cv_usejoystick.value = atoi(cv_usejoystick2.string);
						else // we tried...
							cv_usejoystick.value = 0;
					}

					// Was cv_usejoystick disabled in settings?
					if (!strcmp(cv_usejoystick.string, "0") || !cv_usejoystick.value)
						cv_usejoystick.value = 0;
					else if (atoi(cv_usejoystick.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
						     && cv_usejoystick.value) // update the cvar ONLY if a device exists
						CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

					if (!strcmp(cv_usejoystick2.string, "0") || !cv_usejoystick2.value)
						cv_usejoystick2.value = 0;
					else if (atoi(cv_usejoystick2.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
					         && cv_usejoystick2.value) // update the cvar ONLY if a device exists
						CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

					// Update all joysticks' init states
					// This is a little wasteful since cv_usejoystick already calls this, but
					// we need to do this in case CV_SetValue did nothing because the string was already same.
					// if the device is already active, this should do nothing, effectively.
					I_InitJoystick();
					I_InitJoystick2();

					CONS_Debug(DBG_GAMELOGIC, "Joystick1 device index: %d\n", JoyInfo.oldjoy);
					CONS_Debug(DBG_GAMELOGIC, "Joystick2 device index: %d\n", JoyInfo2.oldjoy);

					// update the menu
					if (currentMenu == &OP_JoystickSetDef)
						M_SetupJoystickMenu(0);

					if (JoyInfo.dev != newjoy && JoyInfo2.dev != newjoy)
						SDL_JoystickClose(newjoy);
				}
				break;
			case SDL_JOYDEVICEREMOVED:
				if (JoyInfo.dev && !SDL_JoystickGetAttached(JoyInfo.dev))
				{
					CONS_Debug(DBG_GAMELOGIC, "Joystick1 removed, device index: %d\n", JoyInfo.oldjoy);
					I_ShutdownJoystick();
				}

				if (JoyInfo2.dev && !SDL_JoystickGetAttached(JoyInfo2.dev))
				{
					CONS_Debug(DBG_GAMELOGIC, "Joystick2 removed, device index: %d\n", JoyInfo2.oldjoy);
					I_ShutdownJoystick2();
				}

				// Update the device indexes, because they likely changed
				// * If device doesn't exist, switch cv_usejoystick back to default value (.string)
				//   * BUT: If that default index is being occupied, use ANOTHER cv_usejoystick's default value!
				if (JoyInfo.dev)
					cv_usejoystick.value = JoyInfo.oldjoy = I_GetJoystickDeviceIndex(JoyInfo.dev) + 1;
				else if (atoi(cv_usejoystick.string) != JoyInfo2.oldjoy)
					cv_usejoystick.value = atoi(cv_usejoystick.string);
				else if (atoi(cv_usejoystick2.string) != JoyInfo2.oldjoy)
					cv_usejoystick.value = atoi(cv_usejoystick2.string);
				else // we tried...
					cv_usejoystick.value = 0;

				if (JoyInfo2.dev)
					cv_usejoystick2.value = JoyInfo2.oldjoy = I_GetJoystickDeviceIndex(JoyInfo2.dev) + 1;
				else if (atoi(cv_usejoystick2.string) != JoyInfo.oldjoy)
					cv_usejoystick2.value = atoi(cv_usejoystick2.string);
				else if (atoi(cv_usejoystick.string) != JoyInfo.oldjoy)
					cv_usejoystick2.value = atoi(cv_usejoystick.string);
				else // we tried...
					cv_usejoystick2.value = 0;

				// Was cv_usejoystick disabled in settings?
				if (!strcmp(cv_usejoystick.string, "0"))
					cv_usejoystick.value = 0;
				else if (atoi(cv_usejoystick.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
						 && cv_usejoystick.value) // update the cvar ONLY if a device exists
					CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

				if (!strcmp(cv_usejoystick2.string, "0"))
					cv_usejoystick2.value = 0;
				else if (atoi(cv_usejoystick2.string) <= I_NumJoys() // don't mess if we intentionally set higher than NumJoys
						 && cv_usejoystick2.value) // update the cvar ONLY if a device exists
					CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

				CONS_Debug(DBG_GAMELOGIC, "Joystick1 device index: %d\n", JoyInfo.oldjoy);
				CONS_Debug(DBG_GAMELOGIC, "Joystick2 device index: %d\n", JoyInfo2.oldjoy);

				// update the menu
				if (currentMenu == &OP_JoystickSetDef)
					M_SetupJoystickMenu(0);
				break;
			case SDL_QUIT:
				LUA_HookBool(true, HOOK(GameQuit));
				I_Quit();
				break;
		}
	}

	// Send all relative mouse movement as one single mouse event.
	if (mousemovex || mousemovey)
	{
		event_t event;
		int wwidth, wheight;
		SDL_GetWindowSize(window, &wwidth, &wheight);
		//SDL_memset(&event, 0, sizeof(event_t));
		event.type = ev_mouse;
		event.key = 0;
		event.x = (INT32)lround(mousemovex * ((float)wwidth / (float)realwidth));
		event.y = (INT32)lround(mousemovey * ((float)wheight / (float)realheight));
		D_PostEvent(&event);
	}

	// In order to make wheels act like buttons, we have to set their state to Up.
	// This is because wheel messages don't have an up/down state.
	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;
}

void I_StartupMouse(void)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;

	if (disable_mouse)
		return;

	if (!firsttimeonmouse)
	{
		HalfWarpMouse(realwidth, realheight); // warp to center
	}
	else
		firsttimeonmouse = SDL_FALSE;
	if (cv_usemouse.value && !IgnoreMouse())
		SDLdoGrabMouse();
	else
		SDLdoUngrabMouse();
}

//
// I_OsPolling
//
void I_OsPolling(void)
{
	SDL_Keymod mod;

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

	mod = SDL_GetModState();
	/* Handle here so that our state is always synched with the system. */
	shiftdown = ctrldown = altdown = 0;
	capslock = false;
	if (mod & KMOD_LSHIFT) shiftdown |= 1;
	if (mod & KMOD_RSHIFT) shiftdown |= 2;
	if (mod & KMOD_LCTRL)   ctrldown |= 1;
	if (mod & KMOD_RCTRL)   ctrldown |= 2;
	if (mod & KMOD_LALT)     altdown |= 1;
	if (mod & KMOD_RALT)     altdown |= 2;
	if (mod & KMOD_CAPS) capslock = true;
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{

}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{
	if (rendermode == render_none)
		return; //Alam: No software or OpenGl surface

	SCR_CalculateFPS();

	if (marathonmode)
		SCR_DisplayMarathonInfo();

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (cv_showping.value && netgame && consoleplayer != serverplayer)
		SCR_DisplayLocalPing();

	if (rendermode == render_soft && screens[0])
	{
		if (!bufSurface) // Double-check
			Impl_VideoSetupBuffer();

		SDL_BlitSurface(bufSurface, &src_rect, vidSurface, &src_rect);
		// Fury -- there's no way around UpdateTexture, the GL backend uses it anyway
		SDL_LockSurface(vidSurface);
		SDL_UpdateTexture(texture, &src_rect, vidSurface->pixels, vidSurface->pitch);
		SDL_UnlockSurface(vidSurface);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, &src_rect, NULL);
		SDL_RenderPresent(renderer);
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl)
		OglSdlFinishUpdate(cv_vidwait.value);
#endif
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
	return MAXWINMODES;
}

const char *VID_GetModeName(INT32 modeNum)
{
	if (modeNum == -1)
		return fallback_resolution_name;
	else if (modeNum > MAXWINMODES)
		return NULL;

	snprintf(&vidModeName[modeNum][0], 32, "%dx%d", windowedModes[modeNum][0], windowedModes[modeNum][1]);

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
}

void VID_PrepareModeList(void)
{
	// Under SDL2, we just use the windowed modes list, and scale in windowed fullscreen.
	allow_fullscreen = true;
}

void VID_CheckGLLoaded(rendermode_t oldrender)
{
	(void)oldrender;
#ifdef HWRENDER
	if (vid.glstate == VID_GL_LIBRARY_ERROR) // Well, it didn't work the first time anyway.
	{
		rendermode = oldrender;
		if (chosenrendermode == render_opengl) // fallback to software
			rendermode = render_soft;
		if (vid.change.renderer != -1)
		{
			CV_StealthSetValue(&cv_renderer, oldrender);
			vid.change.renderer = 0;
		}
	}
#endif
}

boolean VID_CheckRenderer(void)
{
	boolean rendererchanged = false;

#ifdef HWRENDER
	rendermode_t oldrenderer = rendermode;
#endif

	refresh_rate = VID_GetRefreshRate();

	if (dedicated)
		return 0;

	if (vid.change.renderer != -1)
	{
		rendermode = vid.change.renderer;
		rendererchanged = true;

#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			VID_CheckGLLoaded(oldrenderer);

			// Initialize OpenGL before calling SDLSetMode, because it calls OglSdlSurface.
			if (vid.glstate == VID_GL_LIBRARY_NOTLOADED)
				Impl_InitOpenGL();
			else if (vid.glstate == VID_GL_LIBRARY_ERROR)
				rendererchanged = false;
		}
#endif

		vid.change.renderer = -1;
	}

	if (SDLSetMode(vid.width, vid.height, USE_FULLSCREEN, vid.change.set != VID_RESOLUTION_RESIZED_WINDOW) == SDL_FALSE)
	{
		if (!graphics_started)
		{
			// Guess I'll die
			I_Error("Couldn't initialize video");
		}
		else
		{
			CONS_Printf("Couldn't initialize video\n");
			return false;
		}
	}

	if (rendererchanged)
		vid.recalc = true;

	if (rendermode == render_soft)
	{
		Impl_SetupSoftwareBuffer();
		SCR_SetDrawFuncs();
	}
#ifdef HWRENDER
	else if (rendermode == render_opengl && rendererchanged)
	{
		HWR_Switch();
		V_SetPalette(0);
	}
#endif

	return rendererchanged;
}

#if 0
static void Impl_GetCurrentDisplayMode(INT32 *width, INT32 *height)
{
	int i = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode resolution;

	if (i < 0)
		return;

	if (!SDL_GetCurrentDisplayMode(i, &resolution))
	{
		if ((*width) == 0)
			(*width) = (INT32)(resolution.w);
		if ((*height) == 0)
			(*height) = (INT32)(resolution.h);
	}
}

void VID_GetNativeResolution(INT32 *width, INT32 *height)
{
	INT32 w = 0, h = 0;

	if (!w || !h)
		Impl_GetCurrentDisplayMode(&w, &h);

	if (!w) w = BASEVIDWIDTH;
	if (!h) h = BASEVIDHEIGHT;

	if (width) *width = w;
	if (height) *height = h;
}
#endif

void VID_SetSize(INT32 width, INT32 height)
{
	SDLdoUngrabMouse();

	vid.recalc = true;

	if (width > 0 && height > 0 && SCR_IsValidResolution(width, height))
	{
		vid.width = width;
		vid.height = height;
	}

	VID_CheckRenderer();
}

static SDL_bool Impl_CreateWindow(SDL_bool fullscreen)
{
	int flags = SDL_WINDOW_RESIZABLE;

	if (window != NULL)
		return SDL_TRUE;

	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (borderlesswindow)
		flags |= SDL_WINDOW_BORDERLESS;

#ifdef HWRENDER
	flags |= SDL_WINDOW_OPENGL;

	// Without a 24-bit depth buffer many visuals are ruined by z-fighting.
	// Some GPU drivers may give us a 16-bit depth buffer since the
	// default value for SDL_GL_DEPTH_SIZE is 16.
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif

#if defined(__ANDROID__)
	Impl_SetColorBufferDepth(8, 8, 8, 8);
#endif

	// Create a window
	window = SDL_CreateWindow("SRB2 "VERSIONSTRING, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, realwidth, realheight, flags);
	if (window == NULL)
	{
		VIDEO_INIT_ERROR("Couldn't create window: %s");
		return SDL_FALSE;
	}

	SDL_SetWindowMinimumSize(window, BASEVIDWIDTH, BASEVIDHEIGHT);

#ifdef USE_WINDOW_ICON
	Impl_SetWindowIcon();
#endif

	return SDL_TRUE;
}

#ifdef USE_WINDOW_ICON
static void Impl_SetWindowIcon(void)
{
	if (window && icoSurface)
		SDL_SetWindowIcon(window, icoSurface);
}
#endif

static void Impl_VideoSetupBuffer(void)
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

#ifdef HAVE_GLES
static void Impl_InitGLESDriver(void)
{
	const char *driver_name = NULL;
	int version_major, version_minor;

#ifdef HAVE_GLES2
	driver_name = "opengles2";
	version_major = 2;
	version_minor = 0;
#else
	driver_name = "opengles";
	version_major = 1;
	version_minor = 1;
#endif

	SDL_SetHint(SDL_HINT_RENDER_DRIVER, driver_name);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version_major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version_minor);
}
#endif

#if defined(__ANDROID__)
static void Impl_SetColorBufferDepth(INT32 red, INT32 green, INT32 blue, INT32 alpha)
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, red);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, green);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blue);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alpha);
}
#endif

static void Impl_InitVideoSubSystem(void)
{
	if (video_init)
		return;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
	{
		CONS_Printf(M_GetText("Couldn't initialize SDL's Video System: %s\n"), SDL_GetError());
		return;
	}

#ifdef HAVE_GLES
	Impl_InitGLESDriver();
#endif

#ifdef MOBILE_PLATFORM
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

	video_init = true;
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

	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f, COM_LUA);
	COM_AddCommand ("vid_info", VID_Command_Info_f, COM_LUA);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f, COM_LUA);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f, 0);
	CV_RegisterVar (&cv_vidwait);
	CV_RegisterVar (&cv_stretch);
	CV_RegisterVar (&cv_alwaysgrabmouse);
	disable_mouse = M_CheckParm("-nomouse");
	disable_fullscreen = M_CheckParm("-win") ? 1 : 0;

	// [REDACTED] was also initializing the microphone here for some reason
	// (should really be in system init instead)
	keyboard_started = true;

	// If it wasn't already initialized
	if (!video_init)
		Impl_InitVideoSubSystem();

	const char *vd = SDL_GetCurrentVideoDriver();
	if (vd)
	{
		//CONS_Printf(M_GetText("Starting up with video driver: %s\n"), vd);
		if (
			strncasecmp(vd, "gcvideo", 8) == 0 ||
			strncasecmp(vd, "fbcon", 6) == 0 ||
			strncasecmp(vd, "wii", 4) == 0 ||
			strncasecmp(vd, "psl1ght", 8) == 0
		)
			framebuffer = SDL_TRUE;
	}

	rendermode = render_soft;

	// Renderer choices
	// Takes priority over the config.
	if (M_CheckParm("-renderer"))
	{
		INT32 i = 0;
		CV_PossibleValue_t *renderer_list = cv_renderer_t;
		const char *modeparm = M_GetNextParm();
		while (renderer_list[i].strvalue)
		{
			if (!stricmp(modeparm, renderer_list[i].strvalue))
			{
				chosenrendermode = renderer_list[i].value;
				break;
			}
			i++;
		}
	}

	// Choose Software renderer
	else if (M_CheckParm("-software"))
		chosenrendermode = render_soft;

#ifdef HWRENDER
	// Choose OpenGL renderer
	else if (M_CheckParm("-opengl"))
		chosenrendermode = render_opengl;

	// Don't startup OpenGL
	if (M_CheckParm("-nogl"))
	{
		vid.glstate = VID_GL_LIBRARY_ERROR;
		if (chosenrendermode == render_opengl)
			chosenrendermode = render_none;
	}
#endif

	if (chosenrendermode != render_none)
		rendermode = chosenrendermode;

	usesdl2soft = M_CheckParm("-softblit");
	borderlesswindow = M_CheckParm("-borderless");

#ifdef HWRENDER
	if (rendermode == render_opengl)
		Impl_InitOpenGL();
#endif

	// Window icon
#ifdef USE_WINDOW_ICON
	icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif

	// Fury: we do window initialization after GL setup to allow
	// SDL_GL_LoadLibrary to work well on Windows
	vid.recalc = true;
	vid.direct = NULL;
	vid.bpp = 1;

	// Create window
	VID_SetSize(BASEVIDWIDTH, BASEVIDHEIGHT);

#ifdef HAVE_TTF
	I_ShutdownTTF();
#endif

	if (M_CheckParm("-nomousegrab"))
		mousegrabok = SDL_FALSE;

	VID_Command_Info_f();
	SDLdoUngrabMouse();

	SDL_RaiseWindow(window);

	if (mousegrabok && !disable_mouse)
		SDLdoGrabMouse();

	graphics_started = true;
}

static void Impl_InitOpenGL(void)
{
#ifdef HWRENDER
	if (vid.glstate == VID_GL_LIBRARY_LOADED)
		return;

	HWD.pfnInit             = hwSym("Init",NULL);
	HWD.pfnFinishUpdate     = NULL;
	HWD.pfnDraw2DLine       = hwSym("Draw2DLine",NULL);
	HWD.pfnDrawPolygon      = hwSym("DrawPolygon",NULL);
	HWD.pfnDrawIndexedTriangles = hwSym("DrawIndexedTriangles",NULL);
	HWD.pfnRenderSkyDome    = hwSym("RenderSkyDome",NULL);
	HWD.pfnSetBlend         = hwSym("SetBlend",NULL);
	HWD.pfnClearBuffer      = hwSym("ClearBuffer",NULL);
	HWD.pfnSetTexture       = hwSym("SetTexture",NULL);
	HWD.pfnUpdateTexture    = hwSym("UpdateTexture",NULL);
	HWD.pfnDeleteTexture    = hwSym("DeleteTexture",NULL);
	HWD.pfnReadRect         = hwSym("ReadRect",NULL);
	HWD.pfnGClipRect        = hwSym("GClipRect",NULL);
	HWD.pfnClearMipMapCache = hwSym("ClearMipMapCache",NULL);
	HWD.pfnSetSpecialState  = hwSym("SetSpecialState",NULL);
	HWD.pfnSetPalette       = hwSym("SetPalette",NULL);
	HWD.pfnGetTextureUsed   = hwSym("GetTextureUsed",NULL);
	HWD.pfnDrawModel        = hwSym("DrawModel",NULL);
	HWD.pfnCreateModelVBOs  = hwSym("CreateModelVBOs",NULL);
	HWD.pfnSetTransform     = hwSym("SetTransform",NULL);
	HWD.pfnPostImgRedraw    = hwSym("PostImgRedraw",NULL);
	HWD.pfnFlushScreenTextures=hwSym("FlushScreenTextures",NULL);
	HWD.pfnStartScreenWipe  = hwSym("StartScreenWipe",NULL);
	HWD.pfnEndScreenWipe    = hwSym("EndScreenWipe",NULL);
	HWD.pfnDoScreenWipe     = hwSym("DoScreenWipe",NULL);
	HWD.pfnDrawIntermissionBG=hwSym("DrawIntermissionBG",NULL);
	HWD.pfnMakeScreenTexture= hwSym("MakeScreenTexture",NULL);
	HWD.pfnMakeScreenFinalTexture=hwSym("MakeScreenFinalTexture",NULL);
	HWD.pfnDrawScreenFinalTexture=hwSym("DrawScreenFinalTexture",NULL);
	HWD.pfnCompileShaders   = hwSym("CompileShaders",NULL);
	HWD.pfnCleanShaders     = hwSym("CleanShaders",NULL);
	HWD.pfnSetShader        = hwSym("SetShader",NULL);
	HWD.pfnUnSetShader      = hwSym("UnSetShader",NULL);

	HWD.pfnSetShaderInfo    = hwSym("SetShaderInfo",NULL);
	HWD.pfnLoadCustomShader = hwSym("LoadCustomShader",NULL);

	if (HWD.pfnInit())
		vid.glstate = VID_GL_LIBRARY_LOADED;
	else
	{
		vid.glstate = VID_GL_LIBRARY_ERROR;

		CV_StealthSet(&cv_renderer, "Software");

		rendermode = render_soft;

		vid.change.renderer = -1;
	}
#endif
}

void I_ShutdownGraphics(void)
{
#ifdef USE_WINDOW_ICON
	if (icoSurface)
		SDL_FreeSurface(icoSurface);
	icoSurface = NULL;
#endif

	if (rendermode == render_soft)
	{
		if (vidSurface)
			SDL_FreeSurface(vidSurface);
		vidSurface = NULL;

		if (bufSurface)
			SDL_FreeSurface(bufSurface);
		bufSurface = NULL;
	}

	free(vid.buffer);
	vid.buffer = NULL;

	rendermode = render_none;

	I_OutputMsg("I_ShutdownGraphics(): ");

	// was graphics initialized anyway?
	if (!graphics_started)
	{
		I_OutputMsg("graphics never started\n");
		return;
	}
	graphics_started = false;
	I_OutputMsg("shut down\n");

#ifdef HWRENDER
#ifndef HAVE_GLES
	if (GLUhandle)
		hwClose(GLUhandle);
#endif
	if (sdlglcontext)
	{
		SDL_GL_DeleteContext(sdlglcontext);
	}
#endif
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	framebuffer = SDL_FALSE;
}
#endif

void I_GetCursorPosition(INT32 *x, INT32 *y)
{
	SDL_GetMouseState(x, y);
}

UINT32 I_GetRefreshRate(void)
{
	// Moved to VID_GetRefreshRate.
	// Precalculating it like that won't work as
	// well for windowed mode since you can drag
	// the window around, but very slow PCs might have
	// trouble querying mode over and over again.
	return refresh_rate;
}
