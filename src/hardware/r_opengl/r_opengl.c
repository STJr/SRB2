// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2006 by Sonic Team Junior.
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
/// \brief OpenGL API for Sonic Robo Blast 2

#if defined (_WIN32)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#undef GETTEXT
#ifdef __GNUC__
#include <unistd.h>
#endif

#include <stdarg.h>
#include <math.h>
#ifndef SHUFFLE
#ifndef KOS_GL_COMPATIBILITY
#define SHUFFLE
#endif
#endif
#include "r_opengl.h"

#if defined (HWRENDER) && !defined (NOROPENGL)
// for KOS: GL_TEXTURE_ENV, glAlphaFunc, glColorMask, glPolygonOffset, glReadPixels, GL_ALPHA_TEST, GL_POLYGON_OFFSET_FILL

struct GLRGBAFloat
{
	GLfloat red;
	GLfloat green;
	GLfloat blue;
	GLfloat alpha;
};
typedef struct GLRGBAFloat GLRGBAFloat;

// ==========================================================================
//                                                                  CONSTANTS
// ==========================================================================

// With OpenGL 1.1+, the first texture should be 1
#define NOTEXTURE_NUM     1     // small white texture
#define FIRST_TEX_AVAIL   (NOTEXTURE_NUM + 1)

#define      N_PI_DEMI               (M_PIl/2.0f) //(1.5707963268f)

#define      ASPECT_RATIO            (1.0f)  //(320.0f/200.0f)
#define      FAR_CLIPPING_PLANE      32768.0f // Draw further! Tails 01-21-2001
static float NEAR_CLIPPING_PLANE =   NZCLIP_PLANE;

// **************************************************************************
//                                                                    GLOBALS
// **************************************************************************


static  GLuint      NextTexAvail    = FIRST_TEX_AVAIL;
static  GLuint      tex_downloaded  = 0;
static  GLfloat     fov             = 90.0f;
static  GLuint      pal_col         = 0;
static  FRGBAFloat  const_pal_col;
static  FBITFIELD   CurrentPolyFlags;

static  FTextureInfo*  gr_cachetail = NULL;
static  FTextureInfo*  gr_cachehead = NULL;

RGBA_t  myPaletteData[256];
GLint   screen_width    = 0;               // used by Draw2DLine()
GLint   screen_height   = 0;
GLbyte  screen_depth    = 0;
GLint   textureformatGL = 0;
GLint maximumAnisotropy = 0;
#ifndef KOS_GL_COMPATIBILITY
static GLboolean MipMap = GL_FALSE;
#endif
static GLint min_filter = GL_LINEAR;
static GLint mag_filter = GL_LINEAR;
static GLint anisotropic_filter = 0;
static FTransform  md2_transform;

const GLubyte *gl_extensions = NULL;

//Hurdler: 04/10/2000: added for the kick ass coronas as Boris wanted;-)
#ifndef MINI_GL_COMPATIBILITY
static GLdouble    modelMatrix[16];
static GLdouble    projMatrix[16];
static GLint       viewport[4];
#endif


#ifdef USE_PALETTED_TEXTURE
	PFNGLCOLORTABLEEXTPROC  glColorTableEXT = NULL;
	GLubyte                 palette_tex[256*3];
#endif

// Yay for arbitrary  numbers! NextTexAvail is buggy for some reason.
// Sryder:	NextTexAvail is broken for these because palette changes or changes to the texture filter or antialiasing
//			flush all of the stored textures, leaving them unavailable at times such as between levels
//			These need to start at 0 and be set to their number, and be reset to 0 when deleted so that intel GPUs
//			can know when the textures aren't there, as textures are always considered resident in their virtual memory
// TODO:	Store them in a more normal way
#define SCRTEX_SCREENTEXTURE 65535
#define SCRTEX_STARTSCREENWIPE 65534
#define SCRTEX_ENDSCREENWIPE 65533
#define SCRTEX_FINALSCREENTEXTURE 65532
static GLuint screentexture = 0;
static GLuint startScreenWipe = 0;
static GLuint endScreenWipe = 0;
static GLuint finalScreenTexture = 0;
#if 0
GLuint screentexture = FIRST_TEX_AVAIL;
#endif

// shortcut for ((float)1/i)
static const GLfloat byte2float[256] = {
	0.000000f, 0.003922f, 0.007843f, 0.011765f, 0.015686f, 0.019608f, 0.023529f, 0.027451f,
	0.031373f, 0.035294f, 0.039216f, 0.043137f, 0.047059f, 0.050980f, 0.054902f, 0.058824f,
	0.062745f, 0.066667f, 0.070588f, 0.074510f, 0.078431f, 0.082353f, 0.086275f, 0.090196f,
	0.094118f, 0.098039f, 0.101961f, 0.105882f, 0.109804f, 0.113725f, 0.117647f, 0.121569f,
	0.125490f, 0.129412f, 0.133333f, 0.137255f, 0.141176f, 0.145098f, 0.149020f, 0.152941f,
	0.156863f, 0.160784f, 0.164706f, 0.168627f, 0.172549f, 0.176471f, 0.180392f, 0.184314f,
	0.188235f, 0.192157f, 0.196078f, 0.200000f, 0.203922f, 0.207843f, 0.211765f, 0.215686f,
	0.219608f, 0.223529f, 0.227451f, 0.231373f, 0.235294f, 0.239216f, 0.243137f, 0.247059f,
	0.250980f, 0.254902f, 0.258824f, 0.262745f, 0.266667f, 0.270588f, 0.274510f, 0.278431f,
	0.282353f, 0.286275f, 0.290196f, 0.294118f, 0.298039f, 0.301961f, 0.305882f, 0.309804f,
	0.313726f, 0.317647f, 0.321569f, 0.325490f, 0.329412f, 0.333333f, 0.337255f, 0.341176f,
	0.345098f, 0.349020f, 0.352941f, 0.356863f, 0.360784f, 0.364706f, 0.368627f, 0.372549f,
	0.376471f, 0.380392f, 0.384314f, 0.388235f, 0.392157f, 0.396078f, 0.400000f, 0.403922f,
	0.407843f, 0.411765f, 0.415686f, 0.419608f, 0.423529f, 0.427451f, 0.431373f, 0.435294f,
	0.439216f, 0.443137f, 0.447059f, 0.450980f, 0.454902f, 0.458824f, 0.462745f, 0.466667f,
	0.470588f, 0.474510f, 0.478431f, 0.482353f, 0.486275f, 0.490196f, 0.494118f, 0.498039f,
	0.501961f, 0.505882f, 0.509804f, 0.513726f, 0.517647f, 0.521569f, 0.525490f, 0.529412f,
	0.533333f, 0.537255f, 0.541177f, 0.545098f, 0.549020f, 0.552941f, 0.556863f, 0.560784f,
	0.564706f, 0.568627f, 0.572549f, 0.576471f, 0.580392f, 0.584314f, 0.588235f, 0.592157f,
	0.596078f, 0.600000f, 0.603922f, 0.607843f, 0.611765f, 0.615686f, 0.619608f, 0.623529f,
	0.627451f, 0.631373f, 0.635294f, 0.639216f, 0.643137f, 0.647059f, 0.650980f, 0.654902f,
	0.658824f, 0.662745f, 0.666667f, 0.670588f, 0.674510f, 0.678431f, 0.682353f, 0.686275f,
	0.690196f, 0.694118f, 0.698039f, 0.701961f, 0.705882f, 0.709804f, 0.713726f, 0.717647f,
	0.721569f, 0.725490f, 0.729412f, 0.733333f, 0.737255f, 0.741177f, 0.745098f, 0.749020f,
	0.752941f, 0.756863f, 0.760784f, 0.764706f, 0.768627f, 0.772549f, 0.776471f, 0.780392f,
	0.784314f, 0.788235f, 0.792157f, 0.796078f, 0.800000f, 0.803922f, 0.807843f, 0.811765f,
	0.815686f, 0.819608f, 0.823529f, 0.827451f, 0.831373f, 0.835294f, 0.839216f, 0.843137f,
	0.847059f, 0.850980f, 0.854902f, 0.858824f, 0.862745f, 0.866667f, 0.870588f, 0.874510f,
	0.878431f, 0.882353f, 0.886275f, 0.890196f, 0.894118f, 0.898039f, 0.901961f, 0.905882f,
	0.909804f, 0.913726f, 0.917647f, 0.921569f, 0.925490f, 0.929412f, 0.933333f, 0.937255f,
	0.941177f, 0.945098f, 0.949020f, 0.952941f, 0.956863f, 0.960784f, 0.964706f, 0.968628f,
	0.972549f, 0.976471f, 0.980392f, 0.984314f, 0.988235f, 0.992157f, 0.996078f, 1.000000f
};

float byteasfloat(UINT8 fbyte)
{
	return (float)(byte2float[fbyte]*2.0f);
}

static I_Error_t I_Error_GL = NULL;

#ifndef MINI_GL_COMPATIBILITY
static boolean gl13 = false; // whether we can use opengl 1.3 functions
#endif


// -----------------+
// DBG_Printf       : Output error messages to debug log if DEBUG_TO_FILE is defined,
//                  : else do nothing
// Returns          :
// -----------------+
FUNCPRINTF void DBG_Printf(const char *lpFmt, ...)
{
#ifdef DEBUG_TO_FILE
	char    str[4096] = "";
	va_list arglist;

	va_start (arglist, lpFmt);
	vsnprintf (str, 4096, lpFmt, arglist);
	va_end   (arglist);
	if (gllogstream)
		fwrite(str, strlen(str), 1, gllogstream);
#else
	(void)lpFmt;
#endif
}

#ifdef STATIC_OPENGL
/* 1.0 functions */
/* Miscellaneous */
#define pglClearColor glClearColor
//glClear
#define pglColorMask glColorMask
#define pglAlphaFunc glAlphaFunc
#define pglBlendFunc glBlendFunc
#define pglCullFace glCullFace
#define pglPolygonMode glPolygonMode
#define pglPolygonOffset glPolygonOffset
#define pglScissor glScissor
#define pglEnable glEnable
#define pglDisable glDisable
#ifndef MINI_GL_COMPATIBILITY
#define pglGetDoublev glGetDoublev
#endif
//glGetIntegerv
//glGetString
#ifdef KOS_GL_COMPATIBILITY
#define pglHint glHint
#endif

/* Depth Buffer */
#define pglClearDepth glClearDepth
#define pglDepthFunc glDepthFunc
#define pglDepthMask glDepthMask
#define pglDepthRange glDepthRange

/* Transformation */
#define pglMatrixMode glMatrixMode
#define pglViewport glViewport
#define pglPushMatrix glPushMatrix
#define pglPopMatrix glPopMatrix
#define pglLoadIdentity glLoadIdentity
#ifdef MINI_GL_COMPATIBILITY
#define pglMultMatrixf glMultMatrixf
#else
#define pglMultMatrixd glMultMatrixd
#endif
#define pglRotatef glRotatef
#define pglScalef glScalef
#define pglTranslatef glTranslatef

/* Drawing Functions */
#define pglBegin glBegin
#define pglEnd glEnd
#define pglVertex3f glVertex3f
#define pglNormal3f glNormal3f
#define pglColor4f glColor4f
#define pglColor4fv glColor4fv
#define pglTexCoord2f glTexCoord2f

/* Lighting */
#define pglShadeModel glShadeModel
#define pglLightfv glLightfv
#define pglLightModelfv glLightModelfv
#define pglMaterialfv glMaterialfv

/* Raster functions */
#define pglPixelStorei glPixelStorei
#define pglReadPixels glReadPixels

/* Texture mapping */
#define pglTexEnvi glTexEnvi
#define pglTexParameteri glTexParameteri
#define pglTexImage2D glTexImage2D

/* Fog */
#define pglFogf glFogf
#define pglFogfv glFogfv

/* 1.1 functions */
/* texture objects */ //GL_EXT_texture_object
#define pglDeleteTextures glDeleteTextures
#define pglBindTexture glBindTexture
/* texture mapping */ //GL_EXT_copy_texture
#ifndef KOS_GL_COMPATIBILITY
#define pglCopyTexImage2D glCopyTexImage2D
#define pglCopyTexSubImage2D glCopyTexSubImage2D
#endif

#else //!STATIC_OPENGL

/* 1.0 functions */
/* Miscellaneous */
typedef void (APIENTRY * PFNglClearColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static PFNglClearColor pglClearColor;
//glClear
typedef void (APIENTRY * PFNglColorMask) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static PFNglColorMask pglColorMask;
typedef void (APIENTRY * PFNglAlphaFunc) (GLenum func, GLclampf ref);
static PFNglAlphaFunc pglAlphaFunc;
typedef void (APIENTRY * PFNglBlendFunc) (GLenum sfactor, GLenum dfactor);
static PFNglBlendFunc pglBlendFunc;
typedef void (APIENTRY * PFNglCullFace) (GLenum mode);
static PFNglCullFace pglCullFace;
typedef void (APIENTRY * PFNglPolygonMode) (GLenum face, GLenum mode);
static PFNglPolygonMode pglPolygonMode;
typedef void (APIENTRY * PFNglPolygonOffset) (GLfloat factor, GLfloat units);
static PFNglPolygonOffset pglPolygonOffset;
typedef void (APIENTRY * PFNglScissor) (GLint x, GLint y, GLsizei width, GLsizei height);
static PFNglScissor pglScissor;
typedef void (APIENTRY * PFNglEnable) (GLenum cap);
static PFNglEnable pglEnable;
typedef void (APIENTRY * PFNglDisable) (GLenum cap);
static PFNglDisable pglDisable;
#ifndef MINI_GL_COMPATIBILITY
typedef void (APIENTRY * PFNglGetDoublev) (GLenum pname, GLdouble *params);
static PFNglGetDoublev pglGetDoublev;
#endif
//glGetIntegerv
//glGetString

/* Depth Buffer */
typedef void (APIENTRY * PFNglClearDepth) (GLclampd depth);
static PFNglClearDepth pglClearDepth;
typedef void (APIENTRY * PFNglDepthFunc) (GLenum func);
static PFNglDepthFunc pglDepthFunc;
typedef void (APIENTRY * PFNglDepthMask) (GLboolean flag);
static PFNglDepthMask pglDepthMask;
typedef void (APIENTRY * PFNglDepthRange) (GLclampd near_val, GLclampd far_val);
static PFNglDepthRange pglDepthRange;

/* Transformation */
typedef void (APIENTRY * PFNglMatrixMode) (GLenum mode);
static PFNglMatrixMode pglMatrixMode;
typedef void (APIENTRY * PFNglViewport) (GLint x, GLint y, GLsizei width, GLsizei height);
static PFNglViewport pglViewport;
typedef void (APIENTRY * PFNglPushMatrix) (void);
static PFNglPushMatrix pglPushMatrix;
typedef void (APIENTRY * PFNglPopMatrix) (void);
static PFNglPopMatrix pglPopMatrix;
typedef void (APIENTRY * PFNglLoadIdentity) (void);
static PFNglLoadIdentity pglLoadIdentity;
#ifdef MINI_GL_COMPATIBILITY
typedef void (APIENTRY * PFNglMultMatrixf) (const GLfloat *m);
static PFNglMultMatrixf pglMultMatrixf;
#else
typedef void (APIENTRY * PFNglMultMatrixd) (const GLdouble *m);
static PFNglMultMatrixd pglMultMatrixd;
#endif
typedef void (APIENTRY * PFNglRotatef) (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static PFNglRotatef pglRotatef;
typedef void (APIENTRY * PFNglScalef) (GLfloat x, GLfloat y, GLfloat z);
static PFNglScalef pglScalef;
typedef void (APIENTRY * PFNglTranslatef) (GLfloat x, GLfloat y, GLfloat z);
static PFNglTranslatef pglTranslatef;

/* Drawing Functions */
typedef void (APIENTRY * PFNglBegin) (GLenum mode);
static PFNglBegin pglBegin;
typedef void (APIENTRY * PFNglEnd) (void);
static PFNglEnd pglEnd;
typedef void (APIENTRY * PFNglVertex3f) (GLfloat x, GLfloat y, GLfloat z);
static PFNglVertex3f pglVertex3f;
typedef void (APIENTRY * PFNglNormal3f) (GLfloat x, GLfloat y, GLfloat z);
static PFNglNormal3f pglNormal3f;
typedef void (APIENTRY * PFNglColor4f) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static PFNglColor4f pglColor4f;
typedef void (APIENTRY * PFNglColor4fv) (const GLfloat *v);
static PFNglColor4fv pglColor4fv;
typedef void (APIENTRY * PFNglTexCoord2f) (GLfloat s, GLfloat t);
static PFNglTexCoord2f pglTexCoord2f;

/* Lighting */
typedef void (APIENTRY * PFNglShadeModel) (GLenum mode);
static PFNglShadeModel pglShadeModel;
typedef void (APIENTRY * PFNglLightfv) (GLenum light, GLenum pname, GLfloat *params);
static PFNglLightfv pglLightfv;
typedef void (APIENTRY * PFNglLightModelfv) (GLenum pname, GLfloat *params);
static PFNglLightModelfv pglLightModelfv;
typedef void (APIENTRY * PFNglMaterialfv) (GLint face, GLenum pname, GLfloat *params);
static PFNglMaterialfv pglMaterialfv;

/* Raster functions */
typedef void (APIENTRY * PFNglPixelStorei) (GLenum pname, GLint param);
static PFNglPixelStorei pglPixelStorei;
typedef void (APIENTRY  * PFNglReadPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static PFNglReadPixels pglReadPixels;

/* Texture mapping */
typedef void (APIENTRY * PFNglTexEnvi) (GLenum target, GLenum pname, GLint param);
static PFNglTexEnvi pglTexEnvi;
typedef void (APIENTRY * PFNglTexParameteri) (GLenum target, GLenum pname, GLint param);
static PFNglTexParameteri pglTexParameteri;
typedef void (APIENTRY * PFNglTexImage2D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static PFNglTexImage2D pglTexImage2D;

/* Fog */
typedef void (APIENTRY * PFNglFogf) (GLenum pname, GLfloat param);
static PFNglFogf pglFogf;
typedef void (APIENTRY * PFNglFogfv) (GLenum pname, const GLfloat *params);
static PFNglFogfv pglFogfv;

/* 1.1 functions */
/* texture objects */ //GL_EXT_texture_object
typedef void (APIENTRY * PFNglDeleteTextures) (GLsizei n, const GLuint *textures);
static PFNglDeleteTextures pglDeleteTextures;
typedef void (APIENTRY * PFNglBindTexture) (GLenum target, GLuint texture);
static PFNglBindTexture pglBindTexture;
/* texture mapping */ //GL_EXT_copy_texture
typedef void (APIENTRY * PFNglCopyTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static PFNglCopyTexImage2D pglCopyTexImage2D;
typedef void (APIENTRY * PFNglCopyTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static PFNglCopyTexSubImage2D pglCopyTexSubImage2D;
#endif
/* GLU functions */
typedef GLint (APIENTRY * PFNgluBuild2DMipmaps) (GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data);
static PFNgluBuild2DMipmaps pgluBuild2DMipmaps;

#ifndef MINI_GL_COMPATIBILITY
/* 1.3 functions for multitexturing */
typedef void (APIENTRY *PFNglActiveTexture) (GLenum);
static PFNglActiveTexture pglActiveTexture;
typedef void (APIENTRY *PFNglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
static PFNglMultiTexCoord2f pglMultiTexCoord2f;
#endif

#ifndef MINI_GL_COMPATIBILITY
/* 1.2 Parms */
/* GL_CLAMP_TO_EDGE_EXT */
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_MIN_LOD
#define GL_TEXTURE_MIN_LOD 0x813A
#endif
#ifndef GL_TEXTURE_MAX_LOD
#define GL_TEXTURE_MAX_LOD 0x813B
#endif

/* 1.3 GL_TEXTUREi */
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif

#endif

#ifdef MINI_GL_COMPATIBILITY
#undef GL_CLAMP_TO_EDGE
#undef GL_TEXTURE_MIN_LOD
#undef GL_TEXTURE_MAX_LOD
#endif

boolean SetupGLfunc(void)
{
#ifndef STATIC_OPENGL
#define GETOPENGLFUNC(func, proc) \
	func = GetGLFunc(#proc); \
	if (!func) \
	{ \
		DBG_Printf("failed to get OpenGL function: %s", #proc); \
	} \

	GETOPENGLFUNC(pglClearColor, glClearColor)

	GETOPENGLFUNC(pglClear , glClear)
	GETOPENGLFUNC(pglColorMask , glColorMask)
	GETOPENGLFUNC(pglAlphaFunc , glAlphaFunc)
	GETOPENGLFUNC(pglBlendFunc , glBlendFunc)
	GETOPENGLFUNC(pglCullFace , glCullFace)
	GETOPENGLFUNC(pglPolygonMode , glPolygonMode)
	GETOPENGLFUNC(pglPolygonOffset , glPolygonOffset)
	GETOPENGLFUNC(pglScissor , glScissor)
	GETOPENGLFUNC(pglEnable , glEnable)
	GETOPENGLFUNC(pglDisable , glDisable)
#ifndef MINI_GL_COMPATIBILITY
	GETOPENGLFUNC(pglGetDoublev , glGetDoublev)
#endif
	GETOPENGLFUNC(pglGetIntegerv , glGetIntegerv)
	GETOPENGLFUNC(pglGetString , glGetString)

	GETOPENGLFUNC(pglClearDepth , glClearDepth)
	GETOPENGLFUNC(pglDepthFunc , glDepthFunc)
	GETOPENGLFUNC(pglDepthMask , glDepthMask)
	GETOPENGLFUNC(pglDepthRange , glDepthRange)

	GETOPENGLFUNC(pglMatrixMode , glMatrixMode)
	GETOPENGLFUNC(pglViewport , glViewport)
	GETOPENGLFUNC(pglPushMatrix , glPushMatrix)
	GETOPENGLFUNC(pglPopMatrix , glPopMatrix)
	GETOPENGLFUNC(pglLoadIdentity , glLoadIdentity)
#ifdef MINI_GL_COMPATIBILITY
	GETOPENGLFUNC(pglMultMatrixf , glMultMatrixf)
#else
	GETOPENGLFUNC(pglMultMatrixd , glMultMatrixd)
#endif
	GETOPENGLFUNC(pglRotatef , glRotatef)
	GETOPENGLFUNC(pglScalef , glScalef)
	GETOPENGLFUNC(pglTranslatef , glTranslatef)

	GETOPENGLFUNC(pglBegin , glBegin)
	GETOPENGLFUNC(pglEnd , glEnd)
	GETOPENGLFUNC(pglVertex3f , glVertex3f)
	GETOPENGLFUNC(pglNormal3f , glNormal3f)
	GETOPENGLFUNC(pglColor4f , glColor4f)
	GETOPENGLFUNC(pglColor4fv , glColor4fv)
	GETOPENGLFUNC(pglTexCoord2f , glTexCoord2f)

	GETOPENGLFUNC(pglShadeModel , glShadeModel)
	GETOPENGLFUNC(pglLightfv, glLightfv)
	GETOPENGLFUNC(pglLightModelfv , glLightModelfv)
	GETOPENGLFUNC(pglMaterialfv , glMaterialfv)

	GETOPENGLFUNC(pglPixelStorei , glPixelStorei)
	GETOPENGLFUNC(pglReadPixels , glReadPixels)

	GETOPENGLFUNC(pglTexEnvi , glTexEnvi)
	GETOPENGLFUNC(pglTexParameteri , glTexParameteri)
	GETOPENGLFUNC(pglTexImage2D , glTexImage2D)

	GETOPENGLFUNC(pglFogf , glFogf)
	GETOPENGLFUNC(pglFogfv , glFogfv)

	GETOPENGLFUNC(pglDeleteTextures , glDeleteTextures)
	GETOPENGLFUNC(pglBindTexture , glBindTexture)

	GETOPENGLFUNC(pglCopyTexImage2D , glCopyTexImage2D)
	GETOPENGLFUNC(pglCopyTexSubImage2D , glCopyTexSubImage2D)

#undef GETOPENGLFUNC

	pgluBuild2DMipmaps = GetGLFunc("gluBuild2DMipmaps");

#endif
	return true;
}

// This has to be done after the context is created so the version number can be obtained
boolean SetupGLFunc13(void)
{
#ifdef MINI_GL_COMPATIBILITY
	return false;
#else
	const GLubyte *version = pglGetString(GL_VERSION);
	int glmajor, glminor;

	gl13 = false;
	// Parse the GL version
	if (version != NULL)
	{
		if (sscanf((const char*)version, "%d.%d", &glmajor, &glminor) == 2)
		{
			// Look, we gotta prepare for the inevitable arrival of GL 2.0 code...
			if (glmajor == 1 && glminor >= 3)
				gl13 = true;
			else if (glmajor > 1)
				gl13 = true;
		}
	}

	if (gl13)
	{
		pglActiveTexture = GetGLFunc("glActiveTexture");
		pglMultiTexCoord2f = GetGLFunc("glMultiTexCoord2f");
	}
	else if (isExtAvailable("GL_ARB_multitexture", gl_extensions))
	{
		// Get the functions
		pglActiveTexture  = GetGLFunc("glActiveTextureARB");
		pglMultiTexCoord2f  = GetGLFunc("glMultiTexCoord2fARB");

		gl13 = true; // This is now true, so the new fade mask stuff can be done, if OpenGL version is less than 1.3, it still uses the old fade stuff.
		DBG_Printf("GL_ARB_multitexture support: enabled\n");

	}
	else
		DBG_Printf("GL_ARB_multitexture support: disabled\n");
	return true;
#endif
}

// -----------------+
// SetNoTexture     : Disable texture
// -----------------+
static void SetNoTexture(void)
{
	// Set small white texture.
	if (tex_downloaded != NOTEXTURE_NUM)
	{
		pglBindTexture(GL_TEXTURE_2D, NOTEXTURE_NUM);
		tex_downloaded = NOTEXTURE_NUM;
	}
}

static void GLPerspective(GLdouble fovy, GLdouble aspect)
{
#ifdef MINI_GL_COMPATIBILITY
	GLfloat m[4][4] =
#else
	GLdouble m[4][4] =
#endif
	{
		{ 1.0f, 0.0f, 0.0f, 0.0f},
		{ 0.0f, 1.0f, 0.0f, 0.0f},
		{ 0.0f, 0.0f, 1.0f,-1.0f},
		{ 0.0f, 0.0f, 0.0f, 0.0f},
	};
	const GLdouble zNear = NEAR_CLIPPING_PLANE;
	const GLdouble zFar = FAR_CLIPPING_PLANE;
	const GLdouble radians = (GLdouble)(fovy / 2.0f * M_PIl / 180.0f);
	const GLdouble sine = sin(radians);
	const GLdouble deltaZ = zFar - zNear;
	GLdouble cotangent;

	if ((deltaZ == 0.0f) || (sine == 0.0f) || (aspect == 0.0f)) {
		return;
	}
	cotangent = cos(radians) / sine;

	m[0][0] = cotangent / aspect;
	m[1][1] = cotangent;
	m[2][2] = -(zFar + zNear) / deltaZ;
	m[3][2] = -2.0f * zNear * zFar / deltaZ;
#ifdef MINI_GL_COMPATIBILITY
	pglMultMatrixf(&m[0][0]);
#else
	pglMultMatrixd(&m[0][0]);
#endif
}

#ifndef MINI_GL_COMPATIBILITY
static void GLProject(GLdouble objX, GLdouble objY, GLdouble objZ,
                      GLdouble* winX, GLdouble* winY, GLdouble* winZ)
{
	GLdouble in[4], out[4];
	int i;

	for (i=0; i<4; i++)
	{
		out[i] =
			objX * modelMatrix[0*4+i] +
			objY * modelMatrix[1*4+i] +
			objZ * modelMatrix[2*4+i] +
			modelMatrix[3*4+i];
	}
	for (i=0; i<4; i++)
	{
		in[i] =
			out[0] * projMatrix[0*4+i] +
			out[1] * projMatrix[1*4+i] +
			out[2] * projMatrix[2*4+i] +
			out[3] * projMatrix[3*4+i];
	}
	if (in[3] == 0.0f) return;
	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];
	/* Map x, y and z to range 0-1 */
	in[0] = in[0] * 0.5f + 0.5f;
	in[1] = in[1] * 0.5f + 0.5f;
	in[2] = in[2] * 0.5f + 0.5f;

	/* Map x,y to viewport */
	in[0] = in[0] * viewport[2] + viewport[0];
	in[1] = in[1] * viewport[3] + viewport[1];

	*winX=in[0];
	*winY=in[1];
	*winZ=in[2];
}
#endif

// -----------------+
// SetModelView     :
// -----------------+
void SetModelView(GLint w, GLint h)
{
//	DBG_Printf("SetModelView(): %dx%d\n", (int)w, (int)h);

	// The screen textures need to be flushed if the width or height change so that they be remade for the correct size
	if (screen_width != w || screen_height != h)
		FlushScreenTextures();

	screen_width = w;
	screen_height = h;

	pglViewport(0, 0, w, h);
#ifdef GL_ACCUM_BUFFER_BIT
	pglClear(GL_ACCUM_BUFFER_BIT);
#endif

	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();

	pglMatrixMode(GL_MODELVIEW);
	pglLoadIdentity();

	GLPerspective(fov, ASPECT_RATIO);
	//pglScalef(1.0f, 320.0f/200.0f, 1.0f);  // gr_scalefrustum (ORIGINAL_ASPECT)

	// added for new coronas' code (without depth buffer)
#ifndef MINI_GL_COMPATIBILITY
	pglGetIntegerv(GL_VIEWPORT, viewport);
	pglGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
#endif
}


// -----------------+
// SetStates        : Set permanent states
// -----------------+
void SetStates(void)
{
	// Bind little white RGBA texture to ID NOTEXTURE_NUM.
	/*
	FUINT Data[8*8];
	INT32 i;
	*/
#ifdef GL_LIGHT_MODEL_AMBIENT
	GLfloat LightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
#endif

//	DBG_Printf("SetStates()\n");

	// Hurdler: not necessary, is it?
	pglShadeModel(GL_SMOOTH);      // iterate vertice colors
	//pglShadeModel(GL_FLAT);

	pglEnable(GL_TEXTURE_2D);      // two-dimensional texturing
#ifndef KOS_GL_COMPATIBILITY
	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	pglAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
	//pglBlendFunc(GL_ONE, GL_ZERO); // copy pixel to frame buffer (opaque)
	pglEnable(GL_BLEND);           // enable color blending

#ifndef KOS_GL_COMPATIBILITY
	pglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif

	//pglDisable(GL_DITHER);         // faB: ??? (undocumented in OpenGL 1.1)
	                              // Hurdler: yes, it is!
	pglEnable(GL_DEPTH_TEST);    // check the depth buffer
	pglDepthMask(GL_TRUE);             // enable writing to depth buffer
	pglClearDepth(1.0f);
	pglDepthRange(0.0f, 1.0f);
	pglDepthFunc(GL_LEQUAL);

	// this set CurrentPolyFlags to the acctual configuration
	CurrentPolyFlags = 0xffffffff;
	SetBlend(0);

	/*
	for (i = 0; i < 64; i++)
		Data[i] = 0xffFFffFF;       // white pixel
	*/

	tex_downloaded = (GLuint)-1;
	SetNoTexture();
	//pglBindTexture(GL_TEXTURE_2D, NOTEXTURE_NUM);
	//tex_downloaded = NOTEXTURE_NUM;
	//pglTexImage2D(GL_TEXTURE_2D, 0, 4, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);

#ifndef KOS_GL_COMPATIBILITY
	pglPolygonOffset(-1.0f, -1.0f);
#endif

	//pglEnable(GL_CULL_FACE);
	//pglCullFace(GL_FRONT);
	//pglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//pglPolygonMode(GL_FRONT, GL_LINE);

	//glFogi(GL_FOG_MODE, GL_EXP);
	//pglHint(GL_FOG_HINT, GL_FASTEST);
	//pglFogfv(GL_FOG_COLOR, fogcolor);
	//pglFogf(GL_FOG_DENSITY, 0.0005f);

	// Lighting for models
#ifdef GL_LIGHT_MODEL_AMBIENT
	pglLightModelfv(GL_LIGHT_MODEL_AMBIENT, LightDiffuse);
	pglEnable(GL_LIGHT0);
#endif

	// bp : when no t&l :)
	pglLoadIdentity();
	pglScalef(1.0f, 1.0f, -1.0f);
#ifndef MINI_GL_COMPATIBILITY
	pglGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
#endif
}


// -----------------+
// Flush            : flush OpenGL textures
//                  : Clear list of downloaded mipmaps
// -----------------+
void Flush(void)
{
	//DBG_Printf ("HWR_Flush()\n");

	while (gr_cachehead)
	{
		// ceci n'est pas du tout necessaire vu que tu les a charger normalement et
		// donc il sont dans ta liste !
#if 0
		//Hurdler: 25/04/2000: now support colormap in hardware mode
		FTextureInfo    *tmp = gr_cachehead->nextskin;

		// The memory should be freed in the main code
		while (tmp)
		{
			pglDeleteTextures(1, &tmp->downloaded);
			tmp->downloaded = 0;
			tmp = tmp->nextcolormap;
		}
#endif
		pglDeleteTextures(1, (GLuint *)&gr_cachehead->downloaded);
		gr_cachehead->downloaded = 0;
		gr_cachehead = gr_cachehead->nextmipmap;
	}
	gr_cachetail = gr_cachehead = NULL; //Hurdler: well, gr_cachehead is already NULL
	NextTexAvail = FIRST_TEX_AVAIL;
#if 0
	if (screentexture != FIRST_TEX_AVAIL)
	{
		pglDeleteTextures(1, &screentexture);
		screentexture = FIRST_TEX_AVAIL;
	}
#endif

	tex_downloaded = 0;
}


// -----------------+
// isExtAvailable   : Look if an OpenGL extension is available
// Returns          : true if extension available
// -----------------+
INT32 isExtAvailable(const char *extension, const GLubyte *start)
{
	GLubyte         *where, *terminator;

	if (!extension || !start) return 0;
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return 1;
		start = terminator;
	}
	return 0;
}


// -----------------+
// Init             : Initialise the OpenGL interface API
// Returns          :
// -----------------+
EXPORT boolean HWRAPI(Init) (I_Error_t FatalErrorFunction)
{
	I_Error_GL = FatalErrorFunction;
	DBG_Printf ("%s %s\n", DRIVER_STRING, VERSIONSTRING);
	return LoadGL();
}


// -----------------+
// ClearMipMapCache : Flush OpenGL textures from memory
// -----------------+
EXPORT void HWRAPI(ClearMipMapCache) (void)
{
	// DBG_Printf ("HWR_Flush(exe)\n");
	Flush();
}


// -----------------+
// ReadRect         : Read a rectangle region of the truecolor framebuffer
//                  : store pixels as 16bit 565 RGB
// Returns          : 16bit 565 RGB pixel array stored in dst_data
// -----------------+
EXPORT void HWRAPI(ReadRect) (INT32 x, INT32 y, INT32 width, INT32 height,
                                INT32 dst_stride, UINT16 * dst_data)
{
#ifdef KOS_GL_COMPATIBILITY
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	(void)dst_stride;
	(void)dst_data;
#else
	INT32 i;
	// DBG_Printf ("ReadRect()\n");
	if (dst_stride == width*3)
	{
		GLubyte*top = (GLvoid*)dst_data, *bottom = top + dst_stride * (height - 1);
		GLubyte *row = malloc(dst_stride);
		if (!row) return;
		pglPixelStorei(GL_PACK_ALIGNMENT, 1);
		pglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, dst_data);
		pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for(i = 0; i < height/2; i++)
		{
			memcpy(row, top, dst_stride);
			memcpy(top, bottom, dst_stride);
			memcpy(bottom, row, dst_stride);
			top += dst_stride;
			bottom -= dst_stride;
		}
		free(row);
	}
	else
	{
		INT32 j;
		GLubyte *image = malloc(width*height*3*sizeof (*image));
		if (!image) return;
		pglPixelStorei(GL_PACK_ALIGNMENT, 1);
		pglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
		pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		for (i = height-1; i >= 0; i--)
		{
			for (j = 0; j < width; j++)
			{
				dst_data[(height-1-i)*width+j] =
				(UINT16)(
				                 ((image[(i*width+j)*3]>>3)<<11) |
				                 ((image[(i*width+j)*3+1]>>2)<<5) |
				                 ((image[(i*width+j)*3+2]>>3)));
			}
		}
		free(image);
	}
#endif
}


// -----------------+
// GClipRect        : Defines the 2D hardware clipping window
// -----------------+
EXPORT void HWRAPI(GClipRect) (INT32 minx, INT32 miny, INT32 maxx, INT32 maxy, float nearclip)
{
	// DBG_Printf ("GClipRect(%d, %d, %d, %d)\n", minx, miny, maxx, maxy);

	pglViewport(minx, screen_height-maxy, maxx-minx, maxy-miny);
	NEAR_CLIPPING_PLANE = nearclip;

	//pglScissor(minx, screen_height-maxy, maxx-minx, maxy-miny);
	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();
	GLPerspective(fov, ASPECT_RATIO);
	pglMatrixMode(GL_MODELVIEW);

	// added for new coronas' code (without depth buffer)
#ifndef MINI_GL_COMPATIBILITY
	pglGetIntegerv(GL_VIEWPORT, viewport);
	pglGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
#endif
}


// -----------------+
// ClearBuffer      : Clear the color/alpha/depth buffer(s)
// -----------------+
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask,
                                    FBOOLEAN DepthMask,
                                    FRGBAFloat * ClearColor)
{
	// DBG_Printf ("ClearBuffer(%d)\n", alpha);
	GLbitfield ClearMask = 0;

	if (ColorMask)
	{
		if (ClearColor)
			pglClearColor(ClearColor->red,
			              ClearColor->green,
			              ClearColor->blue,
			              ClearColor->alpha);
		ClearMask |= GL_COLOR_BUFFER_BIT;
	}
	if (DepthMask)
	{
		pglClearDepth(1.0f);     //Hurdler: all that are permanen states
		pglDepthRange(0.0f, 1.0f);
		pglDepthFunc(GL_LEQUAL);
		ClearMask |= GL_DEPTH_BUFFER_BIT;
	}

	SetBlend(DepthMask ? PF_Occlude | CurrentPolyFlags : CurrentPolyFlags&~PF_Occlude);

	pglClear(ClearMask);
}


// -----------------+
// HWRAPI Draw2DLine: Render a 2D line
// -----------------+
EXPORT void HWRAPI(Draw2DLine) (F2DCoord * v1,
                                   F2DCoord * v2,
                                   RGBA_t Color)
{
	GLRGBAFloat c;

	// DBG_Printf ("DrawLine() (%f %f %f) %d\n", v1->x, -v1->y, -v1->z, v1->argb);
#ifdef MINI_GL_COMPATIBILITY
	GLfloat px1, px2, px3, px4;
	GLfloat py1, py2, py3, py4;
	GLfloat dx, dy;
	GLfloat angle;
#endif

	// BP: we should reflect the new state in our variable
	//SetBlend(PF_Modulated|PF_NoTexture);

	pglDisable(GL_TEXTURE_2D);

	c.red   = byte2float[Color.s.red];
	c.green = byte2float[Color.s.green];
	c.blue  = byte2float[Color.s.blue];
	c.alpha = byte2float[Color.s.alpha];

#ifndef MINI_GL_COMPATIBILITY
	pglColor4fv(&c.red);    // is in RGBA float format
	pglBegin(GL_LINES);
		pglVertex3f(v1->x, -v1->y, 1.0f);
		pglVertex3f(v2->x, -v2->y, 1.0f);
	pglEnd();
#else
	if (v2->x != v1->x)
		angle = (float)atan((v2->y-v1->y)/(v2->x-v1->x));
	else
		angle = N_PI_DEMI;
	dx = (float)sin(angle) / (float)screen_width;
	dy = (float)cos(angle) / (float)screen_height;

	px1 = v1->x - dx;  py1 = v1->y + dy;
	px2 = v2->x - dx;  py2 = v2->y + dy;
	px3 = v2->x + dx;  py3 = v2->y - dy;
	px4 = v1->x + dx;  py4 = v1->y - dy;

	pglColor4f(c.red, c.green, c.blue, c.alpha);
	pglBegin(GL_TRIANGLE_FAN);
		pglVertex3f(px1, -py1, 1);
		pglVertex3f(px2, -py2, 1);
		pglVertex3f(px3, -py3, 1);
		pglVertex3f(px4, -py4, 1);
	pglEnd();
#endif

	pglEnable(GL_TEXTURE_2D);
}

static void Clamp2D(GLenum pname)
{
	pglTexParameteri(GL_TEXTURE_2D, pname, GL_CLAMP); // fallback clamp
#ifdef GL_CLAMP_TO_EDGE
	pglTexParameteri(GL_TEXTURE_2D, pname, GL_CLAMP_TO_EDGE);
#endif
}


// -----------------+
// SetBlend         : Set render mode
// -----------------+
// PF_Masked - we could use an ALPHA_TEST of GL_EQUAL, and alpha ref of 0,
//             is it faster when pixels are discarded ?
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags)
{
	FBITFIELD Xor;
	Xor = CurrentPolyFlags^PolyFlags;
	if (Xor & (PF_Blending|PF_RemoveYWrap|PF_ForceWrapX|PF_ForceWrapY|PF_Occlude|PF_NoTexture|PF_Modulated|PF_NoDepthTest|PF_Decal|PF_Invisible|PF_NoAlphaTest))
	{
		if (Xor&(PF_Blending)) // if blending mode must be changed
		{
			switch (PolyFlags & PF_Blending) {
				case PF_Translucent & PF_Blending:
					pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha = level of transparency
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
					break;
				case PF_Masked & PF_Blending:
					// Hurdler: does that mean lighting is only made by alpha src?
					// it sounds ok, but not for polygonsmooth
					pglBlendFunc(GL_SRC_ALPHA, GL_ZERO);                // 0 alpha = holes in texture
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_GREATER, 0.5f);
#endif
					break;
				case PF_Additive & PF_Blending:
#ifdef ATI_RAGE_PRO_COMPATIBILITY
					pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // alpha = level of transparency
#else
					pglBlendFunc(GL_SRC_ALPHA, GL_ONE);                 // src * alpha + dest
#endif
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
					break;
				case PF_Environment & PF_Blending:
					pglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
					break;
				case PF_Substractive & PF_Blending:
					// good for shadow
					// not realy but what else ?
					pglBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
					break;
				case PF_Fog & PF_Fog:
					// Sryder: Fog
					// multiplies input colour by input alpha, and destination colour by input colour, then adds them
					pglBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_NOTEQUAL, 0.0f);
#endif
					break;
				default : // must be 0, otherwise it's an error
					// No blending
					pglBlendFunc(GL_ONE, GL_ZERO);   // the same as no blending
#ifndef KOS_GL_COMPATIBILITY
					pglAlphaFunc(GL_GREATER, 0.5f);
#endif
					break;
			}
		}
#ifndef KOS_GL_COMPATIBILITY
		if (Xor & PF_NoAlphaTest)
		{
			if (PolyFlags & PF_NoAlphaTest)
				pglDisable(GL_ALPHA_TEST);
			else
				pglEnable(GL_ALPHA_TEST);      // discard 0 alpha pixels (holes in texture)
		}

		if (Xor & PF_Decal)
		{
			if (PolyFlags & PF_Decal)
				pglEnable(GL_POLYGON_OFFSET_FILL);
			else
				pglDisable(GL_POLYGON_OFFSET_FILL);
		}
#endif
		if (Xor&PF_NoDepthTest)
		{
			if (PolyFlags & PF_NoDepthTest)
				pglDepthFunc(GL_ALWAYS); //pglDisable(GL_DEPTH_TEST);
			else
				pglDepthFunc(GL_LEQUAL); //pglEnable(GL_DEPTH_TEST);
		}

		if (Xor&PF_RemoveYWrap)
		{
			if (PolyFlags & PF_RemoveYWrap)
				Clamp2D(GL_TEXTURE_WRAP_T);
		}

		if (Xor&PF_ForceWrapX)
		{
			if (PolyFlags & PF_ForceWrapX)
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}

		if (Xor&PF_ForceWrapY)
		{
			if (PolyFlags & PF_ForceWrapY)
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

#ifdef KOS_GL_COMPATIBILITY
		if (Xor&PF_Modulated && !(PolyFlags & PF_Modulated))
			pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
#else
		if (Xor&PF_Modulated)
		{
#if defined (__unix__) || defined (UNIXCOMMON)
			if (oglflags & GLF_NOTEXENV)
			{
				if (!(PolyFlags & PF_Modulated))
					pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
#endif
			if (PolyFlags & PF_Modulated)
			{   // mix texture colour with Surface->FlatColor
				pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
			else
			{   // colour from texture is unchanged before blending
				pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
#endif

		if (Xor & PF_Occlude) // depth test but (no) depth write
		{
			if (PolyFlags&PF_Occlude)
			{
				pglDepthMask(1);
			}
			else
				pglDepthMask(0);
		}
		////Hurdler: not used if we don't define POLYSKY
		if (Xor & PF_Invisible)
		{
			if (PolyFlags&PF_Invisible)
				pglBlendFunc(GL_ZERO, GL_ONE);         // transparent blending
			else
			{   // big hack: (TODO: manage that better)
				// we test only for PF_Masked because PF_Invisible is only used
				// (for now) with it (yeah, that's crappy, sorry)
				if ((PolyFlags&PF_Blending)==PF_Masked)
					pglBlendFunc(GL_SRC_ALPHA, GL_ZERO);
			}
		}
		if (PolyFlags & PF_NoTexture)
		{
			SetNoTexture();
		}
	}
	CurrentPolyFlags = PolyFlags;
}


// -----------------+
// SetTexture       : The mipmap becomes the current texture source
// -----------------+
EXPORT void HWRAPI(SetTexture) (FTextureInfo *pTexInfo)
{
	if (!pTexInfo)
	{
		SetNoTexture();
		return;
	}
	else if (pTexInfo->downloaded)
	{
		if (pTexInfo->downloaded != tex_downloaded)
		{
			pglBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);
			tex_downloaded = pTexInfo->downloaded;
		}
	}
	else
	{
		// Download a mipmap
#ifdef KOS_GL_COMPATIBILITY
		static GLushort tex[2048*2048];
#else
		static RGBA_t   tex[2048*2048];
#endif
		const GLvoid   *ptex = tex;
		INT32             w, h;

		//DBG_Printf ("DownloadMipmap %d %x\n",NextTexAvail,pTexInfo->grInfo.data);

		w = pTexInfo->width;
		h = pTexInfo->height;

#ifdef USE_PALETTED_TEXTURE
		if (glColorTableEXT &&
			(pTexInfo->grInfo.format == GR_TEXFMT_P_8) &&
			!(pTexInfo->flags & TF_CHROMAKEYED))
		{
			// do nothing here.
			// Not a problem with MiniGL since we don't use paletted texture
		}
		else
#endif
#ifdef KOS_GL_COMPATIBILITY
		if ((pTexInfo->grInfo.format == GR_TEXFMT_P_8) ||
			(pTexInfo->grInfo.format == GR_TEXFMT_AP_88))
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					if ((*pImgData == HWR_PATCHES_CHROMAKEY_COLORINDEX) &&
					    (pTexInfo->flags & TF_CHROMAKEYED))
					{
						tex[w*j+i] = 0;
					}
					else
					{
						if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88 && !(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i] = 0;
						else
							tex[w*j+i] = (myPaletteData[*pImgData].s.alpha>>4)<<12;

						tex[w*j+i] |= (myPaletteData[*pImgData].s.red  >>4)<<8;
						tex[w*j+i] |= (myPaletteData[*pImgData].s.green>>4)<<4;
						tex[w*j+i] |= (myPaletteData[*pImgData].s.blue >>4);
					}

					pImgData++;

					if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88)
					{
						if (!(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i] |= ((*pImgData)>>4)<<12;
						pImgData++;
					}

				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_RGBA)
		{
			// corona test : passed as ARGB 8888, which is not in glide formats
			// Hurdler: not used for coronas anymore, just for dynamic lighting
			const RGBA_t *pImgData = (const RGBA_t *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i]  = (pImgData->s.alpha>>4)<<12;
					tex[w*j+i] |= (pImgData->s.red  >>4)<<8;
					tex[w*j+i] |= (pImgData->s.green>>4)<<4;
					tex[w*j+i] |= (pImgData->s.blue >>4);
					pImgData++;
				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					const GLubyte sID = (*pImgData)>>4;
					tex[w*j+i] = sID<<8 | sID<<4 | sID;
					pImgData++;
					tex[w*j+i] |= ((*pImgData)>>4)<<12;
					pImgData++;
				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_8) // Used for fade masks
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i]  = (pImgData>>4)<<12;
					tex[w*j+i] |= (255>>4)<<8;
					tex[w*j+i] |= (255>>4)<<4;
					tex[w*j+i] |= (255>>4);
					pImgData++;
				}
			}
		}
		else
			DBG_Printf ("SetTexture(bad format) %ld\n", pTexInfo->grInfo.format);
#else
		if ((pTexInfo->grInfo.format == GR_TEXFMT_P_8) ||
			(pTexInfo->grInfo.format == GR_TEXFMT_AP_88))
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					if ((*pImgData == HWR_PATCHES_CHROMAKEY_COLORINDEX) &&
					    (pTexInfo->flags & TF_CHROMAKEYED))
					{
						tex[w*j+i].s.red   = 0;
						tex[w*j+i].s.green = 0;
						tex[w*j+i].s.blue  = 0;
						tex[w*j+i].s.alpha = 0;
						pTexInfo->flags |= TF_TRANSPARENT; // there is a hole in it
					}
					else
					{
						tex[w*j+i].s.red   = myPaletteData[*pImgData].s.red;
						tex[w*j+i].s.green = myPaletteData[*pImgData].s.green;
						tex[w*j+i].s.blue  = myPaletteData[*pImgData].s.blue;
						tex[w*j+i].s.alpha = myPaletteData[*pImgData].s.alpha;
					}

					pImgData++;

					if (pTexInfo->grInfo.format == GR_TEXFMT_AP_88)
					{
						if (!(pTexInfo->flags & TF_CHROMAKEYED))
							tex[w*j+i].s.alpha = *pImgData;
						pImgData++;
					}

				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_RGBA)
		{
			// corona test : passed as ARGB 8888, which is not in glide formats
			// Hurdler: not used for coronas anymore, just for dynamic lighting
			ptex = pTexInfo->grInfo.data;
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i].s.red   = *pImgData;
					tex[w*j+i].s.green = *pImgData;
					tex[w*j+i].s.blue  = *pImgData;
					pImgData++;
					tex[w*j+i].s.alpha = *pImgData;
					pImgData++;
				}
			}
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_8) // Used for fade masks
		{
			const GLubyte *pImgData = (const GLubyte *)pTexInfo->grInfo.data;
			INT32 i, j;

			for (j = 0; j < h; j++)
			{
				for (i = 0; i < w; i++)
				{
					tex[w*j+i].s.red   = 255; // 255 because the fade mask is modulated with the screen texture, so alpha affects it while the colours don't
					tex[w*j+i].s.green = 255;
					tex[w*j+i].s.blue  = 255;
					tex[w*j+i].s.alpha = *pImgData;
					pImgData++;
				}
			}
		}
		else
			DBG_Printf ("SetTexture(bad format) %ld\n", pTexInfo->grInfo.format);
#endif

		pTexInfo->downloaded = NextTexAvail++;
		tex_downloaded = pTexInfo->downloaded;
		pglBindTexture(GL_TEXTURE_2D, pTexInfo->downloaded);

		// disable texture filtering on any texture that has holes so there's no dumb borders or blending issues
		if (pTexInfo->flags & TF_TRANSPARENT)
		{
#ifdef KOS_GL_COMPATIBILITY
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
#else
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
		}
		else
		{
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		}

#ifdef KOS_GL_COMPATIBILITY
		pglTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB4444, w, h, 0, GL_ARGB4444, GL_UNSIGNED_BYTE, ptex);
#else
#ifdef MINI_GL_COMPATIBILITY
		//if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
			//pglTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		//else
			if (MipMap)
				pgluBuild2DMipmaps(GL_TEXTURE_2D, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
#else
#ifdef USE_PALETTED_TEXTURE
			//Hurdler: not really supported and not tested recently
		if (glColorTableEXT &&
			(pTexInfo->grInfo.format == GR_TEXFMT_P_8) &&
			!(pTexInfo->flags & TF_CHROMAKEYED))
		{
			glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
			pglTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, w, h, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pTexInfo->grInfo.data);
		}
		else
#endif
		if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		{
			//pglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			if (MipMap)
			{
				pgluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
#ifdef GL_TEXTURE_MIN_LOD
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
#endif
#ifdef GL_TEXTURE_MAX_LOD
				if (pTexInfo->flags & TF_TRANSPARENT)
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0); // No mippmaps on transparent stuff
				else
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4);
#endif
				//pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
		else if (pTexInfo->grInfo.format == GR_TEXFMT_ALPHA_8)
		{
			//pglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
			if (MipMap)
			{
				pgluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
#ifdef GL_TEXTURE_MIN_LOD
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
#endif
#ifdef GL_TEXTURE_MAX_LOD
				if (pTexInfo->flags & TF_TRANSPARENT)
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0); // No mippmaps on transparent stuff
				else
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4);
#endif
				//pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
		else
		{
			if (MipMap)
			{
				pgluBuild2DMipmaps(GL_TEXTURE_2D, textureformatGL, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
				// Control the mipmap level of detail
#ifdef GL_TEXTURE_MIN_LOD
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0); // the lower the number, the higer the detail
#endif
#ifdef GL_TEXTURE_MAX_LOD
				if (pTexInfo->flags & TF_TRANSPARENT)
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0); // No mippmaps on transparent stuff
				else
					pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 5);
#endif
			}
			else
				pglTexImage2D(GL_TEXTURE_2D, 0, textureformatGL, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptex);
		}
#endif
#endif

		if (pTexInfo->flags & TF_WRAPX)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		else
			Clamp2D(GL_TEXTURE_WRAP_S);

		if (pTexInfo->flags & TF_WRAPY)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		else
			Clamp2D(GL_TEXTURE_WRAP_T);

		if (maximumAnisotropy)
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropic_filter);

		pTexInfo->nextmipmap = NULL;
		if (gr_cachetail)
		{ // insertion en fin de liste
			gr_cachetail->nextmipmap = pTexInfo;
			gr_cachetail = pTexInfo;
		}
		else // initialisation de la liste
			gr_cachetail = gr_cachehead =  pTexInfo;
	}
#ifdef MINI_GL_COMPATIBILITY
	switch (pTexInfo->flags)
	{
		case 0 :
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			break;
		default:
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
	}
#endif
}


// -----------------+
// DrawPolygon      : Render a polygon, set the texture, set render mode
// -----------------+
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo  *pSurf,
                                    //FTextureInfo  *pTexInfo,
                                    FOutVector    *pOutVerts,
                                    FUINT         iNumPts,
                                    FBITFIELD     PolyFlags)
{
	FUINT i;
#ifndef MINI_GL_COMPATIBILITY
	FUINT j;
#endif
	GLRGBAFloat c = {0,0,0,0};

#ifdef MINI_GL_COMPATIBILITY
	if (PolyFlags & PF_Corona)
		PolyFlags &= ~PF_NoDepthTest;
#else
	if ((PolyFlags & PF_Corona) && (oglflags & GLF_NOZBUFREAD))
		PolyFlags &= ~(PF_NoDepthTest|PF_Corona);
#endif

	SetBlend(PolyFlags);    //TODO: inline (#pragma..)

	// If Modulated, mix the surface colour to the texture
	if ((CurrentPolyFlags & PF_Modulated) && pSurf)
	{
		if (pal_col)
		{ // hack for non-palettized mode
			c.red   = (const_pal_col.red  +byte2float[pSurf->FlatColor.s.red])  /2.0f;
			c.green = (const_pal_col.green+byte2float[pSurf->FlatColor.s.green])/2.0f;
			c.blue  = (const_pal_col.blue +byte2float[pSurf->FlatColor.s.blue]) /2.0f;
			c.alpha = byte2float[pSurf->FlatColor.s.alpha];
		}
		else
		{
			c.red   = byte2float[pSurf->FlatColor.s.red];
			c.green = byte2float[pSurf->FlatColor.s.green];
			c.blue  = byte2float[pSurf->FlatColor.s.blue];
			c.alpha = byte2float[pSurf->FlatColor.s.alpha];
		}

#ifdef MINI_GL_COMPATIBILITY
		pglColor4f(c.red, c.green, c.blue, c.alpha);
#else
		pglColor4fv(&c.red);    // is in RGBA float format
#endif
	}

	// this test is added for new coronas' code (without depth buffer)
	// I think I should do a separate function for drawing coronas, so it will be a little faster
#ifndef MINI_GL_COMPATIBILITY
	if (PolyFlags & PF_Corona) // check to see if we need to draw the corona
	{
		//rem: all 8 (or 8.0f) values are hard coded: it can be changed to a higher value
		GLfloat     buf[8][8];
		GLdouble    cx, cy, cz;
		GLdouble    px = 0.0f, py = 0.0f, pz = -1.0f;
		GLfloat     scalef = 0.0f;

		cx = (pOutVerts[0].x + pOutVerts[2].x) / 2.0f; // we should change the coronas' ...
		cy = (pOutVerts[0].y + pOutVerts[2].y) / 2.0f; // ... code so its only done once.
		cz = pOutVerts[0].z;

		// I dont know if this is slow or not
		GLProject(cx, cy, cz, &px, &py, &pz);
		//DBG_Printf("Projection: (%f, %f, %f)\n", px, py, pz);

		if ((pz <  0.0l) ||
		    (px < -8.0l) ||
		    (py < viewport[1]-8.0l) ||
		    (px > viewport[2]+8.0l) ||
		    (py > viewport[1]+viewport[3]+8.0l))
			return;

		// the damned slow glReadPixels functions :(
		pglReadPixels((INT32)px-4, (INT32)py, 8, 8, GL_DEPTH_COMPONENT, GL_FLOAT, buf);
		//DBG_Printf("DepthBuffer: %f %f\n", buf[0][0], buf[3][3]);

		for (i = 0; i < 8; i++)
			for (j = 0; j < 8; j++)
				scalef += (pz > buf[i][j]+0.00005f) ? 0 : 1;

		// quick test for screen border (not 100% correct, but looks ok)
		if (px < 4) scalef -= (GLfloat)(8*(4-px));
		if (py < viewport[1]+4) scalef -= (GLfloat)(8*(viewport[1]+4-py));
		if (px > viewport[2]-4) scalef -= (GLfloat)(8*(4-(viewport[2]-px)));
		if (py > viewport[1]+viewport[3]-4) scalef -= (GLfloat)(8*(4-(viewport[1]+viewport[3]-py)));

		scalef /= 64;
		//DBG_Printf("Scale factor: %f\n", scalef);

		if (scalef < 0.05f)
			return;

		c.alpha *= scalef; // change the alpha value (it seems better than changing the size of the corona)
		pglColor4fv(&c.red);
	}
#endif
	if (PolyFlags & PF_MD2)
		return;

	pglBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < iNumPts; i++)
	{
		pglTexCoord2f(pOutVerts[i].sow, pOutVerts[i].tow);
		//Hurdler: test code: -pOutVerts[i].z => pOutVerts[i].z
		pglVertex3f(pOutVerts[i].x, pOutVerts[i].y, pOutVerts[i].z);
		//pglVertex3f(pOutVerts[i].x, pOutVerts[i].y, -pOutVerts[i].z);
	}
	pglEnd();

	if (PolyFlags & PF_RemoveYWrap)
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (PolyFlags & PF_ForceWrapX)
		Clamp2D(GL_TEXTURE_WRAP_S);

	if (PolyFlags & PF_ForceWrapY)
		Clamp2D(GL_TEXTURE_WRAP_T);
}


// ==========================================================================
//
// ==========================================================================
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, INT32 Value)
{
	switch (IdState)
	{

#if 0
		case 77:
		{
			//08/01/00: Hurdler this is a test for mirror
			if (!Value)
				ClearBuffer(false, true, 0); // clear depth buffer
			break;
		}
#endif

		case HWD_SET_PALETTECOLOR:
		{
			pal_col = Value;
			const_pal_col.blue  = byte2float[((Value>>16)&0xff)];
			const_pal_col.green = byte2float[((Value>>8)&0xff)];
			const_pal_col.red   = byte2float[((Value)&0xff)];
			break;
		}

		case HWD_SET_FOG_COLOR:
		{
			GLfloat fogcolor[4];

			fogcolor[0] = byte2float[((Value>>16)&0xff)];
			fogcolor[1] = byte2float[((Value>>8)&0xff)];
			fogcolor[2] = byte2float[((Value)&0xff)];
			fogcolor[3] = 0x0;
			pglFogfv(GL_FOG_COLOR, fogcolor);
			break;
		}
		case HWD_SET_FOG_DENSITY:
			pglFogf(GL_FOG_DENSITY, Value*1200/(500*1000000.0f));
			break;

		case HWD_SET_FOG_MODE:
			if (Value)
			{
				pglEnable(GL_FOG);
				// experimental code
				/*
				switch (Value)
				{
					case 1:
						glFogi(GL_FOG_MODE, GL_LINEAR);
						pglFogf(GL_FOG_START, -1000.0f);
						pglFogf(GL_FOG_END, 2000.0f);
						break;
					case 2:
						glFogi(GL_FOG_MODE, GL_EXP);
						break;
					case 3:
						glFogi(GL_FOG_MODE, GL_EXP2);
						break;
				}
				*/
			}
			else
				pglDisable(GL_FOG);
			break;

		case HWD_SET_POLYGON_SMOOTH:
#ifdef KOS_GL_COMPATIBILITY // GL_POLYGON_SMOOTH_HINT
			if (Value)
				pglHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
			else
				pglHint(GL_POLYGON_SMOOTH_HINT,GL_FASTEST);
#else
			if (Value)
				pglEnable(GL_POLYGON_SMOOTH);
			else
				pglDisable(GL_POLYGON_SMOOTH);
#endif
			break;

		case HWD_SET_TEXTUREFILTERMODE:
			switch (Value)
			{
#ifdef KOS_GL_COMPATIBILITY
				case HWD_SET_TEXTUREFILTER_TRILINEAR:
				case HWD_SET_TEXTUREFILTER_BILINEAR:
					min_filter = mag_filter = GL_FILTER_BILINEAR;
					break;
				case HWD_SET_TEXTUREFILTER_POINTSAMPLED:
					min_filter = mag_filter = GL_FILTER_NONE;
				case HWD_SET_TEXTUREFILTER_MIXED1:
					min_filter = GL_FILTER_NONE;
					mag_filter = GL_LINEAR;
				case HWD_SET_TEXTUREFILTER_MIXED2:
					min_filter = GL_LINEAR;
					mag_filter = GL_FILTER_NONE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED3:
					min_filter = GL_FILTER_BILINEAR;
					mag_filter = GL_FILTER_NONE;
					break;
#elif !defined (MINI_GL_COMPATIBILITY)
				case HWD_SET_TEXTUREFILTER_TRILINEAR:
					min_filter = GL_LINEAR_MIPMAP_LINEAR;
					mag_filter = GL_LINEAR;
					MipMap = GL_TRUE;
					break;
				case HWD_SET_TEXTUREFILTER_BILINEAR:
					min_filter = mag_filter = GL_LINEAR;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_POINTSAMPLED:
					min_filter = mag_filter = GL_NEAREST;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED1:
					min_filter = GL_NEAREST;
					mag_filter = GL_LINEAR;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED2:
					min_filter = GL_LINEAR;
					mag_filter = GL_NEAREST;
					MipMap = GL_FALSE;
					break;
				case HWD_SET_TEXTUREFILTER_MIXED3:
					min_filter = GL_LINEAR_MIPMAP_LINEAR;
					mag_filter = GL_NEAREST;
					MipMap = GL_TRUE;
					break;
#endif
				default:
#ifdef KOS_GL_COMPATIBILITY
					min_filter = mag_filter = GL_FILTER_NONE;
#else
					mag_filter = GL_LINEAR;
					min_filter = GL_NEAREST;
#endif
			}
			if (!pgluBuild2DMipmaps)
			{
				MipMap = GL_FALSE;
				min_filter = GL_LINEAR;
			}
			Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		case HWD_SET_TEXTUREANISOTROPICMODE:
			anisotropic_filter = min(Value,maximumAnisotropy);
			if (maximumAnisotropy)
				Flush(); //??? if we want to change filter mode by texture, remove this
			break;

		default:
			break;
	}
}

static  void DrawMD2Ex(INT32 *gl_cmd_buffer, md2_frame_t *frame, INT32 duration, INT32 tics, md2_frame_t *nextframe, FTransform *pos, float scale, UINT8 flipped, UINT8 *color)
{
	INT32     val, count, pindex;
	GLfloat s, t;
	GLfloat ambient[4];
	GLfloat diffuse[4];

	float pol = 0.0f;
	float scalex = scale, scaley = scale, scalez = scale;

	// Because Otherwise, scaling the screen negatively vertically breaks the lighting
#ifndef KOS_GL_COMPATIBILITY
	GLfloat LightPos[] = {0.0f, 1.0f, 0.0f, 0.0f};
#endif

	if (duration != 0 && duration != -1 && tics != -1) // don't interpolate if instantaneous or infinite in length
	{
		UINT32 newtime = (duration - tics); // + 1;

		pol = (newtime)/(float)duration;

		if (pol > 1.0f)
			pol = 1.0f;

		if (pol < 0.0f)
			pol = 0.0f;
	}

	if (color)
	{
		ambient[0] = (color[0]/255.0f);
		ambient[1] = (color[1]/255.0f);
		ambient[2] = (color[2]/255.0f);
		ambient[3] = (color[3]/255.0f);
		diffuse[0] = (color[0]/255.0f);
		diffuse[1] = (color[1]/255.0f);
		diffuse[2] = (color[2]/255.0f);
		diffuse[3] = (color[3]/255.0f);

		if (ambient[0] > 0.75f)
			ambient[0] = 0.75f;
		if (ambient[1] > 0.75f)
			ambient[1] = 0.75f;
		if (ambient[2] > 0.75f)
			ambient[2] = 0.75f;
	}

	pglEnable(GL_CULL_FACE);

	// pos->flip is if the screen is flipped too
	if (flipped != pos->flip) // If either are active, but not both, invert the model's culling
	{
		pglCullFace(GL_FRONT);
	}
	else
	{
		pglCullFace(GL_BACK);
	}

#ifndef KOS_GL_COMPATIBILITY
	pglLightfv(GL_LIGHT0, GL_POSITION, LightPos);
#endif

	pglShadeModel(GL_SMOOTH);
	if (color)
	{
#ifdef GL_LIGHT_MODEL_AMBIENT
		pglEnable(GL_LIGHTING);
		pglMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
		pglMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
#endif
		if (color[3] < 255)
			SetBlend(PF_Translucent|PF_Modulated|PF_Clip);
		else
			SetBlend(PF_Masked|PF_Modulated|PF_Occlude|PF_Clip);
	}

	pglPushMatrix(); // should be the same as glLoadIdentity
	//Hurdler: now it seems to work
	pglTranslatef(pos->x, pos->z, pos->y);
	if (flipped)
		scaley = -scaley;
	pglRotatef(pos->angley, 0.0f, -1.0f, 0.0f);
	pglRotatef(pos->anglex, -1.0f, 0.0f, 0.0f);

	val = *gl_cmd_buffer++;

	while (val != 0)
	{
		if (val < 0)
		{
			pglBegin(GL_TRIANGLE_FAN);
			count = -val;
		}
		else
		{
			pglBegin(GL_TRIANGLE_STRIP);
			count = val;
		}

		while (count--)
		{
			s = *(float *) gl_cmd_buffer++;
			t = *(float *) gl_cmd_buffer++;
			pindex = *gl_cmd_buffer++;

			pglTexCoord2f(s, t);

			if (!nextframe || pol == 0.0f)
			{
				pglNormal3f(frame->vertices[pindex].normal[0],
				            frame->vertices[pindex].normal[1],
				            frame->vertices[pindex].normal[2]);

				pglVertex3f(frame->vertices[pindex].vertex[0]*scalex/2.0f,
				            frame->vertices[pindex].vertex[1]*scaley/2.0f,
				            frame->vertices[pindex].vertex[2]*scalez/2.0f);
			}
			else
			{
				// Interpolate
				float px1 = frame->vertices[pindex].vertex[0]*scalex/2.0f;
				float px2 = nextframe->vertices[pindex].vertex[0]*scalex/2.0f;
				float py1 = frame->vertices[pindex].vertex[1]*scaley/2.0f;
				float py2 = nextframe->vertices[pindex].vertex[1]*scaley/2.0f;
				float pz1 = frame->vertices[pindex].vertex[2]*scalez/2.0f;
				float pz2 = nextframe->vertices[pindex].vertex[2]*scalez/2.0f;
				float nx1 = frame->vertices[pindex].normal[0];
				float nx2 = nextframe->vertices[pindex].normal[0];
				float ny1 = frame->vertices[pindex].normal[1];
				float ny2 = nextframe->vertices[pindex].normal[1];
				float nz1 = frame->vertices[pindex].normal[2];
				float nz2 = nextframe->vertices[pindex].normal[2];

				pglNormal3f((nx1 + pol * (nx2 - nx1)),
				            (ny1 + pol * (ny2 - ny1)),
				            (nz1 + pol * (nz2 - nz1)));
				pglVertex3f((px1 + pol * (px2 - px1)),
				            (py1 + pol * (py2 - py1)),
				            (pz1 + pol * (pz2 - pz1)));
			}
		}

		pglEnd();

		val = *gl_cmd_buffer++;
	}
	pglPopMatrix(); // should be the same as glLoadIdentity
	if (color)
		pglDisable(GL_LIGHTING);
	pglShadeModel(GL_FLAT);
	pglDisable(GL_CULL_FACE);
}

// -----------------+
// HWRAPI DrawMD2   : Draw an MD2 model with glcommands
// -----------------+
EXPORT void HWRAPI(DrawMD2i) (INT32 *gl_cmd_buffer, md2_frame_t *frame, INT32 duration, INT32 tics, md2_frame_t *nextframe, FTransform *pos, float scale, UINT8 flipped, UINT8 *color)
{
	DrawMD2Ex(gl_cmd_buffer, frame, duration, tics,  nextframe, pos, scale, flipped, color);
}

EXPORT void HWRAPI(DrawMD2) (INT32 *gl_cmd_buffer, md2_frame_t *frame, FTransform *pos, float scale)
{
	DrawMD2Ex(gl_cmd_buffer, frame, 0, 0,  NULL, pos, scale, false, NULL);
}


// -----------------+
// SetTransform     :
// -----------------+
EXPORT void HWRAPI(SetTransform) (FTransform *stransform)
{
	static INT32 special_splitscreen;
	pglLoadIdentity();
	if (stransform)
	{
		// keep a trace of the transformation for md2
		memcpy(&md2_transform, stransform, sizeof (md2_transform));

		if (stransform->flip)
			pglScalef(stransform->scalex, -stransform->scaley, -stransform->scalez);
		else
			pglScalef(stransform->scalex, stransform->scaley, -stransform->scalez);

		pglRotatef(stransform->anglex       , 1.0f, 0.0f, 0.0f);
		pglRotatef(stransform->angley+270.0f, 0.0f, 1.0f, 0.0f);
		pglTranslatef(-stransform->x, -stransform->z, -stransform->y);

		pglMatrixMode(GL_PROJECTION);
		pglLoadIdentity();
		special_splitscreen = (stransform->splitscreen && stransform->fovxangle == 90.0f);
		if (special_splitscreen)
			GLPerspective(53.13l, 2*ASPECT_RATIO);  // 53.13 = 2*atan(0.5)
		else
			GLPerspective(stransform->fovxangle, ASPECT_RATIO);
#ifndef MINI_GL_COMPATIBILITY
		pglGetDoublev(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
#endif
		pglMatrixMode(GL_MODELVIEW);
	}
	else
	{
		pglScalef(1.0f, 1.0f, -1.0f);

		pglMatrixMode(GL_PROJECTION);
		pglLoadIdentity();
		if (special_splitscreen)
			GLPerspective(53.13l, 2*ASPECT_RATIO);  // 53.13 = 2*atan(0.5)
		else
			//Hurdler: is "fov" correct?
			GLPerspective(fov, ASPECT_RATIO);
#ifndef MINI_GL_COMPATIBILITY
		pglGetDoublev(GL_PROJECTION_MATRIX, projMatrix); // added for new coronas' code (without depth buffer)
#endif
		pglMatrixMode(GL_MODELVIEW);
	}

#ifndef MINI_GL_COMPATIBILITY
	pglGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); // added for new coronas' code (without depth buffer)
#endif
}

EXPORT INT32  HWRAPI(GetTextureUsed) (void)
{
	FTextureInfo*   tmp = gr_cachehead;
	INT32             res = 0;

	while (tmp)
	{
		res += tmp->height*tmp->width*(screen_depth/8);
		tmp = tmp->nextmipmap;
	}
	return res;
}

EXPORT INT32  HWRAPI(GetRenderVersion) (void)
{
	return VERSION;
}

#ifdef SHUFFLE
EXPORT void HWRAPI(PostImgRedraw) (float points[SCREENVERTS][SCREENVERTS][2])
{
	INT32 x, y;
	float float_x, float_y, float_nextx, float_nexty;
	float xfix, yfix;
	INT32 texsize = 2048;

	// Use a power of two texture, dammit
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;

	// X/Y stretch fix for all resolutions(!)
	xfix = (float)(texsize)/((float)((screen_width)/(float)(SCREENVERTS-1)));
	yfix = (float)(texsize)/((float)((screen_height)/(float)(SCREENVERTS-1)));

	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_BLEND);
	pglBegin(GL_QUADS);

		// Draw a black square behind the screen texture,
		// so nothing shows through the edges
		pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		pglVertex3f(-16.0f, -16.0f, 6.0f);
		pglVertex3f(-16.0f, 16.0f, 6.0f);
		pglVertex3f(16.0f, 16.0f, 6.0f);
		pglVertex3f(16.0f, -16.0f, 6.0f);

		for(x=0;x<SCREENVERTS-1;x++)
		{
			for(y=0;y<SCREENVERTS-1;y++)
			{
				// Used for texture coordinates
				// Annoying magic numbers to scale the square texture to
				// a non-square screen..
				float_x = (float)(x/(xfix));
				float_y = (float)(y/(yfix));
				float_nextx = (float)(x+1)/(xfix);
				float_nexty = (float)(y+1)/(yfix);

				// Attach the squares together.
				pglTexCoord2f( float_x, float_y);
				pglVertex3f(points[x][y][0], points[x][y][1], 4.4f);

				pglTexCoord2f( float_x, float_nexty);
				pglVertex3f(points[x][y+1][0], points[x][y+1][1], 4.4f);

				pglTexCoord2f( float_nextx, float_nexty);
				pglVertex3f(points[x+1][y+1][0], points[x+1][y+1][1], 4.4f);

				pglTexCoord2f( float_nextx, float_y);
				pglVertex3f(points[x+1][y][0], points[x+1][y][1], 4.4f);
			}
		}
	pglEnd();
	pglEnable(GL_DEPTH_TEST);
	pglEnable(GL_BLEND);
}
#endif //SHUFFLE

// Sryder:	This needs to be called whenever the screen changes resolution in order to reset the screen textures to use
//			a new size
EXPORT void HWRAPI(FlushScreenTextures) (void)
{
	pglDeleteTextures(1, &screentexture);
	pglDeleteTextures(1, &startScreenWipe);
	pglDeleteTextures(1, &endScreenWipe);
	pglDeleteTextures(1, &finalScreenTexture);
	screentexture = 0;
	startScreenWipe = 0;
	endScreenWipe = 0;
	finalScreenTexture = 0;
}

// Create Screen to fade from
EXPORT void HWRAPI(StartScreenWipe) (void)
{
	INT32 texsize = 2048;
	boolean firstTime = (startScreenWipe == 0);

	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;

	// Create screen texture
	if (firstTime)
		startScreenWipe = SCRTEX_STARTSCREENWIPE;
	pglBindTexture(GL_TEXTURE_2D, startScreenWipe);

	if (firstTime)
	{
#ifdef KOS_GL_COMPATIBILITY
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_FILTER_NONE);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_FILTER_NONE);
#else
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
#endif
	}
	else
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);
#endif

	tex_downloaded = startScreenWipe;
}

// Create Screen to fade to
EXPORT void HWRAPI(EndScreenWipe)(void)
{
	INT32 texsize = 2048;
	boolean firstTime = (endScreenWipe == 0);

	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;

	// Create screen texture
	if (firstTime)
		endScreenWipe = SCRTEX_ENDSCREENWIPE;
	pglBindTexture(GL_TEXTURE_2D, endScreenWipe);

	if (firstTime)
	{
#ifdef KOS_GL_COMPATIBILITY
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_FILTER_NONE);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_FILTER_NONE);
#else
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
#endif
	}
	else
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);
#endif


	tex_downloaded = endScreenWipe;
}


// Draw the last scene under the intermission
EXPORT void HWRAPI(DrawIntermissionBG)(void)
{
	float xfix, yfix;
	INT32 texsize = 2048;

	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	pglBindTexture(GL_TEXTURE_2D, screentexture);
	pglBegin(GL_QUADS);

		pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		// Bottom left
		pglTexCoord2f(0.0f, 0.0f);
		pglVertex3f(-1.0f, -1.0f, 1.0f);

		// Top left
		pglTexCoord2f(0.0f, yfix);
		pglVertex3f(-1.0f, 1.0f, 1.0f);

		// Top right
		pglTexCoord2f(xfix, yfix);
		pglVertex3f(1.0f, 1.0f, 1.0f);

		// Bottom right
		pglTexCoord2f(xfix, 0.0f);
		pglVertex3f(1.0f, -1.0f, 1.0f);

	pglEnd();

	tex_downloaded = screentexture;
}

// Do screen fades!
EXPORT void HWRAPI(DoScreenWipe)(float alpha)
{
	INT32 texsize = 2048;
	float xfix, yfix;

#ifndef MINI_GL_COMPATIBILITY
	INT32 fademaskdownloaded = tex_downloaded; // the fade mask that has been set
#endif

	// Use a power of two texture, dammit
	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	SetBlend(PF_Modulated|PF_NoDepthTest|PF_Clip|PF_NoZClip);

	// Draw the original screen
	pglBindTexture(GL_TEXTURE_2D, startScreenWipe);
	pglBegin(GL_QUADS);
		pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// Bottom left
		pglTexCoord2f(0.0f, 0.0f);
		pglVertex3f(-1.0f, -1.0f, 1.0f);

		// Top left
		pglTexCoord2f(0.0f, yfix);
		pglVertex3f(-1.0f, 1.0f, 1.0f);

		// Top right
		pglTexCoord2f(xfix, yfix);
		pglVertex3f(1.0f, 1.0f, 1.0f);

		// Bottom right
		pglTexCoord2f(xfix, 0.0f);
		pglVertex3f(1.0f, -1.0f, 1.0f);

	pglEnd();

	SetBlend(PF_Modulated|PF_Translucent|PF_NoDepthTest|PF_Clip|PF_NoZClip);

#ifndef MINI_GL_COMPATIBILITY
	if (gl13)
	{
		// Draw the end screen that fades in
		pglActiveTexture(GL_TEXTURE0);
		pglEnable(GL_TEXTURE_2D);
		pglBindTexture(GL_TEXTURE_2D, endScreenWipe);
		pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		pglActiveTexture(GL_TEXTURE1);
		pglEnable(GL_TEXTURE_2D);
		pglBindTexture(GL_TEXTURE_2D, fademaskdownloaded);

		pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		pglBegin(GL_QUADS);
			pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			// Bottom left
			pglMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
			pglMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
			pglVertex3f(-1.0f, -1.0f, 1.0f);

			// Top left
			pglMultiTexCoord2f(GL_TEXTURE0, 0.0f, yfix);
			pglMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
			pglVertex3f(-1.0f, 1.0f, 1.0f);

			// Top right
			pglMultiTexCoord2f(GL_TEXTURE0, xfix, yfix);
			pglMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
			pglVertex3f(1.0f, 1.0f, 1.0f);

			// Bottom right
			pglMultiTexCoord2f(GL_TEXTURE0, xfix, 0.0f);
			pglMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
			pglVertex3f(1.0f, -1.0f, 1.0f);
		pglEnd();

		pglDisable(GL_TEXTURE_2D); // disable the texture in the 2nd texture unit
		pglActiveTexture(GL_TEXTURE0);
		tex_downloaded = endScreenWipe;
	}
	else
	{
#endif
	// Draw the end screen that fades in
	pglBindTexture(GL_TEXTURE_2D, endScreenWipe);
	pglBegin(GL_QUADS);
		pglColor4f(1.0f, 1.0f, 1.0f, alpha);

		// Bottom left
		pglTexCoord2f(0.0f, 0.0f);
		pglVertex3f(-1.0f, -1.0f, 1.0f);

		// Top left
		pglTexCoord2f(0.0f, yfix);
		pglVertex3f(-1.0f, 1.0f, 1.0f);

		// Top right
		pglTexCoord2f(xfix, yfix);
		pglVertex3f(1.0f, 1.0f, 1.0f);

		// Bottom right
		pglTexCoord2f(xfix, 0.0f);
		pglVertex3f(1.0f, -1.0f, 1.0f);
	pglEnd();
	tex_downloaded = endScreenWipe;
#ifndef MINI_GL_COMPATIBILITY
	}
#endif
}


// Create a texture from the screen.
EXPORT void HWRAPI(MakeScreenTexture) (void)
{
	INT32 texsize = 2048;
	boolean firstTime = (screentexture == 0);

	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;

	// Create screen texture
	if (firstTime)
		screentexture = SCRTEX_SCREENTEXTURE;
	pglBindTexture(GL_TEXTURE_2D, screentexture);

	if (firstTime)
	{
#ifdef KOS_GL_COMPATIBILITY
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_FILTER_NONE);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_FILTER_NONE);
#else
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
#endif
	}
	else
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);
#endif

	tex_downloaded = screentexture;
}

EXPORT void HWRAPI(MakeScreenFinalTexture) (void)
{
	INT32 texsize = 2048;
	boolean firstTime = (finalScreenTexture == 0);

	// Use a power of two texture, dammit
	if(screen_width <= 512)
		texsize = 512;
	else if(screen_width <= 1024)
		texsize = 1024;

	// Create screen texture
	if (firstTime)
		finalScreenTexture = SCRTEX_FINALSCREENTEXTURE;
	pglBindTexture(GL_TEXTURE_2D, finalScreenTexture);

	if (firstTime)
	{
#ifdef KOS_GL_COMPATIBILITY
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_FILTER_NONE);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_FILTER_NONE);
#else
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
		Clamp2D(GL_TEXTURE_WRAP_S);
		Clamp2D(GL_TEXTURE_WRAP_T);
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texsize, texsize, 0);
#endif
	}
	else
#ifndef KOS_GL_COMPATIBILITY
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texsize, texsize);
#endif

	tex_downloaded = finalScreenTexture;

}

EXPORT void HWRAPI(DrawScreenFinalTexture)(int width, int height)
{
	float xfix, yfix;
	float origaspect, newaspect;
	float xoff = 1, yoff = 1; // xoffset and yoffset for the polygon to have black bars around the screen
	FRGBAFloat clearColour;
	INT32 texsize = 2048;

	if(screen_width <= 1024)
		texsize = 1024;
	if(screen_width <= 512)
		texsize = 512;

	xfix = 1/((float)(texsize)/((float)((screen_width))));
	yfix = 1/((float)(texsize)/((float)((screen_height))));

	origaspect = (float)screen_width / screen_height;
	newaspect = (float)width / height;
	if (origaspect < newaspect)
	{
		xoff = origaspect / newaspect;
		yoff = 1;
	}
	else if (origaspect > newaspect)
	{
		xoff = 1;
		yoff = newaspect / origaspect;
	}

	pglViewport(0, 0, width, height);

	clearColour.red = clearColour.green = clearColour.blue = 0;
	clearColour.alpha = 1;
	ClearBuffer(true, false, &clearColour);
	pglBindTexture(GL_TEXTURE_2D, finalScreenTexture);
	pglBegin(GL_QUADS);

		pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		// Bottom left
		pglTexCoord2f(0.0f, 0.0f);
		pglVertex3f(-xoff, -yoff, 1.0f);

		// Top left
		pglTexCoord2f(0.0f, yfix);
		pglVertex3f(-xoff, yoff, 1.0f);

		// Top right
		pglTexCoord2f(xfix, yfix);
		pglVertex3f(xoff, yoff, 1.0f);

		// Bottom right
		pglTexCoord2f(xfix, 0.0f);
		pglVertex3f(xoff, -yoff, 1.0f);

	pglEnd();

	tex_downloaded = finalScreenTexture;
}

#endif //HWRENDER
