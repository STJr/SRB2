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
/// \brief SDL specific part of the OpenGL API for SRB2

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "../doomdef.h"

#ifdef HWRENDER
#include "../hardware/r_opengl/r_opengl.h"
#include "ogl_sdl.h"
#include "../i_system.h"
#include "hwsym_sdl.h"
#include "../m_argv.h"

#ifdef DEBUG_TO_FILE
#include <stdarg.h>
#if defined (_WIN32) && !defined (__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
#else
typedef int (*PFNGLXSWAPINTERVALPROC) (int);
PFNGLXSWAPINTERVALPROC glXSwapIntervalSGIEXT = NULL;
#endif

#ifndef STATIC_OPENGL
PFNglClear pglClear;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;
#endif

#ifdef _PSP
static const Uint32 WOGLFlags = SDL_HWSURFACE|SDL_OPENGL/*|SDL_RESIZABLE*/;
static const Uint32 FOGLFlags = SDL_HWSURFACE|SDL_OPENGL|SDL_FULLSCREEN;
#else
static const Uint32 WOGLFlags = SDL_OPENGL/*|SDL_RESIZABLE*/;
static const Uint32 FOGLFlags = SDL_OPENGL|SDL_FULLSCREEN;
#endif

/**	\brief SDL video display surface
*/
SDL_Surface *vidSurface = NULL;
INT32 oglflags = 0;
void *GLUhandle = NULL;

#ifndef STATIC_OPENGL
void *GetGLFunc(const char *proc)
{
	if (strncmp(proc, "glu", 3) == 0)
	{
		if (GLUhandle)
			return hwSym(proc, GLUhandle);
		else
			return NULL;
	}
	return SDL_GL_GetProcAddress(proc);
}
#endif

boolean LoadGL(void)
{
#ifndef STATIC_OPENGL
	const char *OGLLibname = NULL;
	const char *GLULibname = NULL;

	if (M_CheckParm ("-OGLlib") && M_IsNextParm())
		OGLLibname = M_GetNextParm();

	if (SDL_GL_LoadLibrary(OGLLibname) != 0)
	{
		I_OutputMsg("Could not load OpenGL Library: %s\n"
					"Falling back to Software mode.\n", SDL_GetError());
		if (!M_CheckParm ("-OGLlib"))
			I_OutputMsg("If you know what is the OpenGL library's name, use -OGLlib\n");
		return 0;
	}

#if 0
	GLULibname = "/proc/self/exe";
#elif defined (_WIN32)
	GLULibname = "GLU32.DLL";
#elif defined (__MACH__)
	GLULibname = "/System/Library/Frameworks/OpenGL.framework/Libraries/libGLU.dylib";
#elif defined (macintos)
	GLULibname = "OpenGLLibrary";
#elif defined (__unix__)
	GLULibname = "libGLU.so.1";
#elif defined (__HAIKU__)
	GLULibname = "libGLU.so";
#else
	GLULibname = NULL;
#endif

	if (M_CheckParm ("-GLUlib") && M_IsNextParm())
		GLULibname = M_GetNextParm();

	if (GLULibname)
	{
		GLUhandle = hwOpen(GLULibname);
		if (GLUhandle)
			return SetupGLfunc();
		else
		{
			I_OutputMsg("Could not load GLU Library: %s\n", GLULibname);
			if (!M_CheckParm ("-GLUlib"))
				I_OutputMsg("If you know what is the GLU library's name, use -GLUlib\n");
		}
	}
	else
	{
		I_OutputMsg("Could not load GLU Library\n");
		I_OutputMsg("If you know what is the GLU library's name, use -GLUlib\n");
	}
#endif
	return SetupGLfunc();
}

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height
	\param	isFullscreen	if true, go fullscreen

	\return	if true, changed video mode
*/
boolean OglSdlSurface(INT32 w, INT32 h, boolean isFullscreen)
{
	INT32 cbpp;
	Uint32 OGLFlags;
	const GLvoid *glvendor = NULL, *glrenderer = NULL, *glversion = NULL;

	cbpp = cv_scr_depth.value < 16 ? 16 : cv_scr_depth.value;

	if (vidSurface)
	{
		//Alam: SDL_Video system free vidSurface for me
#ifdef VOODOOSAFESWITCHING
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
	}

	if (isFullscreen)
		OGLFlags = FOGLFlags;
	else
		OGLFlags = WOGLFlags;

	cbpp = SDL_VideoModeOK(w, h, cbpp, OGLFlags);
	if (cbpp < 16)
		return true; //Alam: Let just say we did, ok?

	vidSurface = SDL_SetVideoMode(w, h, cbpp, OGLFlags);
	if (!vidSurface)
		return false;

	glvendor = pglGetString(GL_VENDOR);
	// Get info and extensions.
	//BP: why don't we make it earlier ?
	//Hurdler: we cannot do that before intialising gl context
	glrenderer = pglGetString(GL_RENDERER);
	glversion = pglGetString(GL_VERSION);
	gl_extensions = pglGetString(GL_EXTENSIONS);

	DBG_Printf("Vendor     : %s\n", glvendor);
	DBG_Printf("Renderer   : %s\n", glrenderer);
	DBG_Printf("Version    : %s\n", glversion);
	DBG_Printf("Extensions : %s\n", gl_extensions);
	oglflags = 0;

#ifdef _WIN32
	// BP: disable advenced feature that don't work on somes hardware
	// Hurdler: Now works on G400 with bios 1.6 and certified drivers 6.04
	if (strstr(glrenderer, "810")) oglflags |= GLF_NOZBUFREAD;
#elif defined (unix) || defined (UNIXCOMMON)
	// disable advanced features not working on somes hardware
	if (strstr(glrenderer, "G200")) oglflags |= GLF_NOTEXENV;
	if (strstr(glrenderer, "G400")) oglflags |= GLF_NOTEXENV;
#endif
	DBG_Printf("oglflags   : 0x%X\n", oglflags );

#ifdef USE_PALETTED_TEXTURE
	if (isExtAvailable("GL_EXT_paletted_texture", gl_extensions))
		glColorTableEXT = SDL_GL_GetProcAddress("glColorTableEXT");
	else
		glColorTableEXT = NULL;
#endif

#ifdef USE_WGL_SWAP
	if (isExtAvailable("WGL_EXT_swap_control", gl_extensions))
		wglSwapIntervalEXT = SDL_GL_GetProcAddress("wglSwapIntervalEXT");
	else
		wglSwapIntervalEXT = NULL;
#else
	if (isExtAvailable("GLX_SGI_swap_control", gl_extensions))
		glXSwapIntervalSGIEXT = SDL_GL_GetProcAddress("glXSwapIntervalSGI");
	else
		glXSwapIntervalSGIEXT = NULL;
#endif

	if (isExtAvailable("GL_EXT_texture_filter_anisotropic", gl_extensions))
		pglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 0;

	SetupGLFunc13();

	granisotropicmode_cons_t[1].value = maximumAnisotropy;

	SetModelView(w, h);
	SetStates();
	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	HWR_Startup();
	textureformatGL = cbpp > 16 ? GL_RGBA : GL_RGB5_A1;

	return true;
}

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(boolean waitvbl)
{
	static boolean oldwaitvbl = false;
	if (oldwaitvbl != waitvbl)
	{
#ifdef USE_WGL_SWAP
		if (wglSwapIntervalEXT)
			wglSwapIntervalEXT(waitvbl);
#else
		if (glXSwapIntervalSGIEXT)
			glXSwapIntervalSGIEXT(waitvbl);
#endif
	}
	oldwaitvbl = waitvbl;

	SDL_GL_SwapBuffers();
}

EXPORT void HWRAPI( OglSdlSetPalette) (RGBA_t *palette, RGBA_t *pgamma)
{
	INT32 i = -1;
	UINT32 redgamma = pgamma->s.red, greengamma = pgamma->s.green,
		bluegamma = pgamma->s.blue;

#if 0 // changing the gamma to 127 is a bad idea
	i = SDL_SetGamma(byteasfloat(redgamma), byteasfloat(greengamma), byteasfloat(bluegamma));
#endif
	if (i == 0) redgamma = greengamma = bluegamma = 0x7F; //Alam: cool
	for (i = 0; i < 256; i++)
	{
		myPaletteData[i].s.red   = (UINT8)MIN((palette[i].s.red   * redgamma)  /127, 255);
		myPaletteData[i].s.green = (UINT8)MIN((palette[i].s.green * greengamma)/127, 255);
		myPaletteData[i].s.blue  = (UINT8)MIN((palette[i].s.blue  * bluegamma) /127, 255);
		myPaletteData[i].s.alpha = palette[i].s.alpha;
	}
#ifdef USE_PALETTED_TEXTURE
	if (glColorTableEXT)
	{
		for (i = 0; i < 256; i++)
		{
			palette_tex[(3*i)+0] = palette[i].s.red;
			palette_tex[(3*i)+1] = palette[i].s.green;
			palette_tex[(3*i)+2] = palette[i].s.blue;
		}
		glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
	}
#endif
	// on a chang�de palette, il faut recharger toutes les textures
	// jaja, und noch viel mehr ;-)
	Flush();
}

#endif //HWRENDER
#endif //SDL
