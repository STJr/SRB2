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
/// \brief Windows specific part of the OpenGL API for Doom Legacy
///
///	TODO:
///	- check if windowed mode works
///	- support different pixel formats


#if defined (_WIN32)

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#include <time.h>
#undef GETTEXT
#include "r_opengl.h"


// **************************************************************************
//                                                                    GLOBALS
// **************************************************************************

#ifdef DEBUG_TO_FILE
static unsigned long nb_frames = 0;
static clock_t my_clock;
FILE *gllogstream;
#endif

static  HDC     hDC           = NULL;       // the window's device context
static  HGLRC   hGLRC         = NULL;       // the OpenGL rendering context
static  HWND    hWnd          = NULL;
static  BOOL    WasFullScreen = FALSE;
static void UnSetRes(void);

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
#endif

PFNglClear pglClear;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;

#define MAX_VIDEO_MODES   32
static  vmode_t     video_modes[MAX_VIDEO_MODES];
INT32     oglflags = 0;

// **************************************************************************
//                                                                  FUNCTIONS
// **************************************************************************

// -----------------+
// APIENTRY DllMain : DLL Entry Point,
//                  : open/close debug log
// Returns          :
// -----------------+
BOOL WINAPI DllMain(HINSTANCE hinstDLL, // handle to DLL module
                    DWORD fdwReason,    // reason for calling function
                    LPVOID lpvReserved) // reserved
{
	// Perform actions based on the reason for calling.
	UNREFERENCED_PARAMETER(lpvReserved);
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.
#ifdef DEBUG_TO_FILE
			gllogstream = fopen("ogllog.txt", "wt");
			if (gllogstream == NULL)
				return FALSE;
#endif
			DisableThreadLibraryCalls(hinstDLL);
			break;

		case DLL_THREAD_ATTACH:
			// Do thread-specific initialization.
			break;

		case DLL_THREAD_DETACH:
			// Do thread-specific cleanup.
			break;

		case DLL_PROCESS_DETACH:
			// Perform any necessary cleanup.
#ifdef DEBUG_TO_FILE
			if (gllogstream)
			{
				fclose(gllogstream);
				gllogstream  = NULL;
			}
#endif
			break;
	}

	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#ifdef STATIC_OPENGL
#define pwglGetProcAddress wglGetProcAddress;
#define pwglCreateContext wglCreateContext;
#define pwglDeleteContext wglDeleteContext;
#define pwglMakeCurrent wglMakeCurrent;
#else
static HMODULE OGL32, GLU32;
typedef void *(WINAPI *PFNwglGetProcAddress) (const char *);
static PFNwglGetProcAddress pwglGetProcAddress;
typedef HGLRC (WINAPI *PFNwglCreateContext) (HDC hdc);
static PFNwglCreateContext pwglCreateContext;
typedef BOOL (WINAPI *PFNwglDeleteContext) (HGLRC hglrc);
static PFNwglDeleteContext pwglDeleteContext;
typedef BOOL (WINAPI *PFNwglMakeCurrent) (HDC hdc, HGLRC hglrc);
static PFNwglMakeCurrent pwglMakeCurrent;
#endif

#ifndef STATIC_OPENGL
void *GetGLFunc(const char *proc)
{
	void *func = NULL;
	if (strncmp(proc, "glu", 3) == 0)
	{
		if (GLU32)
			func = GetProcAddress(GLU32, proc);
		else
			return NULL;
	}
	if (pwglGetProcAddress)
		func = pwglGetProcAddress(proc);
	if (!func)
		func = GetProcAddress(OGL32, proc);
	return func;
}
#endif

boolean LoadGL(void)
{
#ifndef STATIC_OPENGL
	OGL32 = LoadLibrary("OPENGL32.DLL");

	if (!OGL32)
		return 0;

	GLU32 = LoadLibrary("GLU32.DLL");

	pwglGetProcAddress = GetGLFunc("wglGetProcAddress");
	pwglCreateContext = GetGLFunc("wglCreateContext");
	pwglDeleteContext = GetGLFunc("wglDeleteContext");
	pwglMakeCurrent = GetGLFunc("wglMakeCurrent");
#endif
	return SetupGLfunc();
}

// -----------------+
// SetupPixelFormat : Set the device context's pixel format
// Note             : Because we currently use only the desktop's BPP format, all the
//                  : video modes in Doom Legacy OpenGL are of the same BPP, thus the
//                  : PixelFormat is set only once.
//                  : Setting the pixel format more than once on the same window
//                  : doesn't work. (ultimately for different pixel formats, we
//                  : should close the window, and re-create it)
// -----------------+
int SetupPixelFormat(INT32 WantColorBits, INT32 WantStencilBits, INT32 WantDepthBits)
{
	static DWORD iLastPFD = 0;
	INT32 nPixelFormat;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof (PIXELFORMATDESCRIPTOR),  // size
		1,                              // version
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,                  // color type
		32 /*WantColorBits*/,           // cColorBits : prefered color depth
		0, 0,                           // cRedBits, cRedShift
		0, 0,                           // cGreenBits, cGreenShift
		0, 0,                           // cBlueBits, cBlueShift
		0, 0,                           // cAlphaBits, cAlphaShift
		0,                              // cAccumBits
		0, 0, 0, 0,                     // cAccum Red/Green/Blue/Alpha Bits
		0,                              // cDepthBits (0,16,24,32)
		0,                              // cStencilBits
		0,                              // cAuxBuffers
		PFD_MAIN_PLANE,                 // iLayerType
		0,                              // reserved, must be zero
		0, 0, 0,                        // dwLayerMask, dwVisibleMask, dwDamageMask
	};

	DWORD iPFD = (WantColorBits<<16) | (WantStencilBits<<8) | WantDepthBits;

	pfd.cDepthBits = (BYTE)WantDepthBits;
	pfd.cStencilBits = (BYTE)WantStencilBits;

	if (iLastPFD)
	{
		GL_DBG_Printf("WARNING : SetPixelFormat() called twise not supported by all drivers !\n");
	}

	// set the pixel format only if different than the current
	if (iPFD == iLastPFD)
		return 2;
	else
		iLastPFD = iPFD;

	GL_DBG_Printf("SetupPixelFormat() - %d ColorBits - %d StencilBits - %d DepthBits\n",
	           WantColorBits, WantStencilBits, WantDepthBits);

	nPixelFormat = ChoosePixelFormat(hDC, &pfd);

	if (nPixelFormat == 0)
		GL_DBG_Printf("ChoosePixelFormat() FAILED\n");

	if (SetPixelFormat(hDC, nPixelFormat, &pfd) == 0)
	{
		GL_DBG_Printf("SetPixelFormat() FAILED\n");
		return 0;
	}

	return 1;
}


// -----------------+
// SetRes           : Set a display mode
// Notes            : pcurrentmode is actually not used
// -----------------+
static INT32 WINAPI SetRes(viddef_t *lvid, vmode_t *pcurrentmode)
{
	LPCSTR renderer;
	BOOL WantFullScreen = !(lvid->u.windowed);  //(lvid->u.windowed ? 0 : CDS_FULLSCREEN);

	UNREFERENCED_PARAMETER(pcurrentmode);
	GL_DBG_Printf ("SetMode(): %dx%d %d bits (%s)\n",
	            lvid->width, lvid->height, lvid->bpp*8,
	            WantFullScreen ? "fullscreen" : "windowed");

	hWnd = lvid->WndParent;

	// BP : why flush texture ?
	//      if important flush also the first one (white texture) and restore it !
	Flush();    // Flush textures.

// TODO: if not fullscreen, skip display stuff and just resize viewport stuff ...

	// Exit previous mode
	//if (hGLRC) //Hurdler: TODO: check if this is valid
	//    UnSetRes();
	if (WasFullScreen)
		ChangeDisplaySettings(NULL, CDS_FULLSCREEN); //switch in and out of fullscreen

		// Change display settings.
	if (WantFullScreen)
	{
		DEVMODE dm;
		ZeroMemory(&dm, sizeof (dm));
		dm.dmSize       = sizeof (dm);
		dm.dmPelsWidth  = lvid->width;
		dm.dmPelsHeight = lvid->height;
		dm.dmBitsPerPel = lvid->bpp*8;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
		if (ChangeDisplaySettings(&dm, CDS_TEST) != DISP_CHANGE_SUCCESSFUL)
			return -2;
		if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) !=DISP_CHANGE_SUCCESSFUL)
			return -3;

		SetWindowLong(hWnd, GWL_STYLE, WS_POPUP|WS_VISIBLE);
		// faB : book says to put these, surely for windowed mode
		//WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, lvid->width, lvid->height, SWP_NOACTIVATE|SWP_NOZORDER);
	}
	else
	{
		RECT bounds;
		INT32 x, y;
		INT32 w = lvid->width, h = lvid->height;
		GetWindowRect(hWnd, &bounds);
		bounds.right = bounds.left+w;
		bounds.bottom = bounds.top+h;
		AdjustWindowRectEx(&bounds, GetWindowLong(hWnd, GWL_STYLE), (GetMenu(hWnd) != NULL), 0);
		w = bounds.right-bounds.left;
		h = bounds.bottom-bounds.top;
		x = (GetSystemMetrics(SM_CXSCREEN)-w)/2;
		y = (GetSystemMetrics(SM_CYSCREEN)-h)/2;
		SetWindowPos(hWnd, NULL, x, y, w, h, SWP_NOACTIVATE|SWP_NOZORDER);
	}

	if (!hDC)
		hDC = GetDC(hWnd);
	if (!hDC)
	{
		GL_DBG_Printf("GetDC() FAILED\n");
		return 0;
	}

	{
		INT32 res;

		// Set res.
		res = SetupPixelFormat(lvid->bpp*8, 0, 16);

		if (res == 0)
			return 0;
		else if (res == 1)
		{
			// Exit previous mode
			if (hGLRC)
				UnSetRes();
			hGLRC = pwglCreateContext(hDC);
			if (!hGLRC)
			{
				GL_DBG_Printf("pwglCreateContext() FAILED\n");
				return 0;
			}
			if (!pwglMakeCurrent(hDC, hGLRC))
			{
				GL_DBG_Printf("wglMakeCurrent() FAILED\n");
				return 0;
			}
		}
	}

	gl_extensions = pglGetString(GL_EXTENSIONS);
	// Get info and extensions.
	//BP: why don't we make it earlier ?
	//Hurdler: we cannot do that before intialising gl context
	renderer = (LPCSTR)pglGetString(GL_RENDERER);
	GL_DBG_Printf("Vendor     : %s\n", pglGetString(GL_VENDOR));
	GL_DBG_Printf("Renderer   : %s\n", renderer);
	GL_DBG_Printf("Version    : %s\n", pglGetString(GL_VERSION));
	GL_DBG_Printf("Extensions : %s\n", gl_extensions);

	// BP: disable advenced feature that don't work on somes hardware
	// Hurdler: Now works on G400 with bios 1.6 and certified drivers 6.04
	if (strstr(renderer, "810"))   oglflags |= GLF_NOZBUFREAD;
	GL_DBG_Printf("oglflags   : 0x%X\n", oglflags);

#ifdef USE_WGL_SWAP
	if (isExtAvailable("WGL_EXT_swap_control",gl_extensions))
		wglSwapIntervalEXT = GetGLFunc("wglSwapIntervalEXT");
	else
		wglSwapIntervalEXT = NULL;
#endif

	if (isExtAvailable("GL_EXT_texture_filter_anisotropic",gl_extensions))
		pglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 0;

	SetupGLFunc13();

	screen_depth = (GLbyte)(lvid->bpp*8);
	if (screen_depth > 16)
		textureformatGL = GL_RGBA;
	else
		textureformatGL = GL_RGB5_A1;

	SetModelView(lvid->width, lvid->height);
	SetStates();
	// we need to clear the depth buffer. Very important!!!
	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	lvid->buffer = NULL;    // unless we use the software view
	lvid->direct = NULL;    // direct access to video memory, old DOS crap

	WasFullScreen = WantFullScreen;

	return 1;               // on renvoie une valeur pour dire que cela s'est bien pass�
}


// -----------------+
// UnSetRes         : Restore the original display mode
// -----------------+
static void UnSetRes(void)
{
	GL_DBG_Printf("UnSetRes()\n");

	pwglMakeCurrent(hDC, NULL);
	pwglDeleteContext(hGLRC);
	hGLRC = NULL;
	if (WasFullScreen)
		ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
}


// -----------------+
// GetModeList      : Return the list of available display modes.
// Returns          : pvidmodes   - points to list of detected OpenGL video modes
//                  : numvidmodes - number of detected OpenGL video modes
// -----------------+
EXPORT void HWRAPI(GetModeList) (vmode_t** pvidmodes, INT32 *numvidmodes)
{
	INT32  i;

#if 1
	INT32 iMode;
/*
	faB test code

	Commented out because there might be a problem with combo (Voodoo2 + other card),
	we need to get the 3D card's display modes only.
*/
	(*pvidmodes) = &video_modes[0];

	// Get list of device modes
	for (i = 0,iMode = 0; iMode < MAX_VIDEO_MODES; i++)
	{
		DEVMODE Tmp;
		ZeroMemory(&Tmp, sizeof (Tmp));
		Tmp.dmSize = sizeof (Tmp);
		if (!EnumDisplaySettings(NULL, i, &Tmp))
			break;

		// add video mode
		if (Tmp.dmBitsPerPel == 16 &&
			 (iMode == 0 || !
			  (Tmp.dmPelsWidth == video_modes[iMode-1].width &&
			   Tmp.dmPelsHeight == video_modes[iMode-1].height)
			 )
			)
		{
			video_modes[iMode].pnext = &video_modes[iMode+1];
			video_modes[iMode].windowed = 0;                    // fullscreen is the default
			video_modes[iMode].misc = 0;
			video_modes[iMode].name = malloc(12 * sizeof (CHAR));
			sprintf(video_modes[iMode].name, "%dx%d", (INT32)Tmp.dmPelsWidth, (INT32)Tmp.dmPelsHeight);
			GL_DBG_Printf ("Mode: %s\n", video_modes[iMode].name);
			video_modes[iMode].width = Tmp.dmPelsWidth;
			video_modes[iMode].height = Tmp.dmPelsHeight;
			video_modes[iMode].bytesperpixel = Tmp.dmBitsPerPel/8;
			video_modes[iMode].rowbytes = Tmp.dmPelsWidth * video_modes[iMode].bytesperpixel;
			video_modes[iMode].pextradata = NULL;
			video_modes[iMode].setmode = SetRes;
			iMode++;
		}
	}
	(*numvidmodes) = iMode;
#else

	// classic video modes (fullscreen/windowed)
	// Added some. Tails
	INT32 res[][2] = {
					{ 320, 200},
					{ 320, 240},
					{ 400, 300},
					{ 512, 384},
					{ 640, 400},
					{ 640, 480},
					{ 800, 600},
					{ 960, 600},
					{1024, 768},
					{1152, 864},
					{1280, 800},
					{1280, 960},
					{1280,1024},
					{1600,1000},
					{1680,1050},
					{1920,1200},
};

	HDC bpphdc;
	INT32 iBitsPerPel;

	GL_DBG_Printf ("HWRAPI GetModeList()\n");

	bpphdc = GetDC(NULL); // on obtient le bpp actuel
	iBitsPerPel = GetDeviceCaps(bpphdc, BITSPIXEL);

	ReleaseDC(NULL, bpphdc);

	(*pvidmodes) = &video_modes[0];
	(*numvidmodes) = sizeof (res) / sizeof (res[0]);
	for (i = 0; i < (*numvidmodes); i++)
	{
		video_modes[i].pnext = &video_modes[i+1];
		video_modes[i].windowed = 0; // fullscreen is the default
		video_modes[i].misc = 0;
		video_modes[i].name = malloc(12 * sizeof (CHAR));
		sprintf(video_modes[i].name, "%dx%d", res[i][0], res[i][1]);
		GL_DBG_Printf ("Mode: %s\n", video_modes[i].name);
		video_modes[i].width = res[i][0];
		video_modes[i].height = res[i][1];
		video_modes[i].bytesperpixel = iBitsPerPel/8;
		video_modes[i].rowbytes = res[i][0] * video_modes[i].bytesperpixel;
		video_modes[i].pextradata = NULL;
		video_modes[i].setmode = SetRes;
	}
#endif
	video_modes[(*numvidmodes)-1].pnext = NULL;
}


// -----------------+
// Shutdown         : Shutdown OpenGL, restore the display mode
// -----------------+
EXPORT void HWRAPI(Shutdown) (void)
{
#ifdef DEBUG_TO_FILE
	long nb_centiemes;

	GL_DBG_Printf ("HWRAPI Shutdown()\n");
	nb_centiemes = ((clock()-my_clock)*100)/CLOCKS_PER_SEC;
	GL_DBG_Printf("Nb frames: %li;  Nb sec: %2.2f  ->  %2.1f fps\n",
					nb_frames, nb_centiemes/100.0f, (100*nb_frames)/(double)nb_centiemes);
#endif

	Flush();

	// Exit previous mode
	if (hGLRC)
		UnSetRes();

	if (hDC)
	{
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
	FreeLibrary(GLU32);
	FreeLibrary(OGL32);
	GL_DBG_Printf ("HWRAPI Shutdown(DONE)\n");
}

// -----------------+
// FinishUpdate     : Swap front and back buffers
// -----------------+
EXPORT void HWRAPI(FinishUpdate) (INT32 waitvbl)
{
#ifdef USE_WGL_SWAP
	static INT32 oldwaitvbl = 0;
#else
	UNREFERENCED_PARAMETER(waitvbl);
#endif
	// GL_DBG_Printf ("FinishUpdate()\n");
#ifdef DEBUG_TO_FILE
	if ((++nb_frames)==2)  // on ne commence pas � la premi�re frame
		my_clock = clock();
#endif

#ifdef USE_WGL_SWAP
	if (oldwaitvbl != waitvbl && wglSwapIntervalEXT)
		wglSwapIntervalEXT(waitvbl);
	oldwaitvbl = waitvbl;
#endif

	SwapBuffers(hDC);
}


// -----------------+
// SetPalette       : Set the color lookup table for paletted textures
//                  : in OpenGL, we store values for conversion of paletted graphics when
//                  : they are downloaded to the 3D card.
// -----------------+
EXPORT void HWRAPI(SetPalette) (RGBA_t *pal)
{
	size_t palsize = (sizeof(RGBA_t) * 256);
	// on a palette change, you have to reload all of the textures
	if (memcmp(&myPaletteData, pal, palsize))
	{
		memcpy(&myPaletteData, pal, palsize);
		Flush();
	}
}

#endif
