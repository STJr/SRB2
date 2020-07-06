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
/// \brief OpenGL API for Doom Legacy

#ifndef _R_OPENGL_H_
#define _R_OPENGL_H_

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#include "SDL_opengl.h" //Alam_GBC: Simple, yes?

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#else
#include <GL/gl.h>
#include <GL/glu.h>

#ifdef STATIC_OPENGL // Because of the 1.3 functions, you'll need GLext to compile it if static
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>
#endif
#endif

#define  _CREATE_DLL_  // necessary for Unix AND Windows
#include "../../doomdef.h"
#include "../hw_drv.h"

// ==========================================================================
//                                                                DEFINITIONS
// ==========================================================================

#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#define MAX(x,y) (((x)>(y)) ? (x) : (y))

#undef DEBUG_TO_FILE            // maybe defined in previous *.h
#define DEBUG_TO_FILE           // output debugging msgs to ogllog.txt

// todo: find some way of getting SDL to log to ogllog.txt, without
// interfering with r_opengl.dll
#ifdef HAVE_SDL
#undef DEBUG_TO_FILE
#endif
//#if defined(HAVE_SDL) && !defined(_DEBUG)
//#undef DEBUG_TO_FILE
//#endif

#ifdef DEBUG_TO_FILE
extern FILE             *gllogstream;
#endif

// ==========================================================================
//                                                                     PROTOS
// ==========================================================================

boolean LoadGL(void);
void *GetGLFunc(const char *proc);
boolean SetupGLfunc(void);
void SetupGLFunc4(void);
void Flush(void);
INT32 isExtAvailable(const char *extension, const GLubyte *start);
void SetModelView(GLint w, GLint h);
void SetStates(void);

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifdef USE_WGL_SWAP
typedef BOOL (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int (APIENTRY *PFNWGLEXTGETSWAPINTERVALPROC) (void);
extern PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT;
extern PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT;
#endif

#ifdef STATIC_OPENGL
#define pglClear glClear
#define pglGetIntegerv glGetIntegerv
#define pglGetString glGetString
#else
/* 1.0 Miscellaneous functions */
typedef void (APIENTRY * PFNglClear) (GLbitfield mask);
extern PFNglClear pglClear;
typedef void (APIENTRY * PFNglGetIntegerv) (GLenum pname, GLint *params);
extern PFNglGetIntegerv pglGetIntegerv;
typedef const GLubyte* (APIENTRY  * PFNglGetString) (GLenum name);
extern PFNglGetString pglGetString;
#if 0
typedef void (APIENTRY * PFNglEnableClientState) (GLenum cap); // redefined in r_opengl.c
static PFNglEnableClientState pglEnableClientState;
#endif
#endif

// ==========================================================================
//                                                                     GLOBAL
// ==========================================================================

extern const GLubyte	*gl_version;
extern const GLubyte	*gl_renderer;
extern const GLubyte	*gl_extensions;

extern RGBA_t			myPaletteData[];
extern GLint			screen_width;
extern GLint			screen_height;
extern GLbyte			screen_depth;
extern GLint			maximumAnisotropy;

/**	\brief OpenGL flags for video driver
*/
extern INT32            oglflags;
extern GLint            textureformatGL;

typedef enum
{
	GLF_NOZBUFREAD = 0x01,
	GLF_NOTEXENV   = 0x02,
} oglflags_t;

#endif
