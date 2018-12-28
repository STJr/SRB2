// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  screen.h
/// \brief Handles multiple resolutions, 8bpp/16bpp(highcolor) modes

#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "command.h"

#if (defined (_WIN32) || defined (_WIN32_WCE)) && !defined (__CYGWIN__) && !defined (_XBOX)
#if defined (_WIN32_WCE) && defined (__GNUC__)
#include <sys/wcetypes.h>
#else
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#define DNWH HWND
#else
#define DNWH void * // unused in DOS version
#endif

// quickhack for V_Init()... to be cleaned up
#if defined (DC) || defined (_WIN32_WCE) || defined (PSP) || defined (NOPOSTPROCESSING)
#define NUMSCREENS 2
#else
#define NUMSCREENS 5
#endif

// Size of statusbar.
#define ST_HEIGHT 32
#define ST_WIDTH 320

// used now as a maximum video mode size for extra vesa modes.

// we try to re-allocate a minimum of buffers for stability of the memory,
// so all the small-enough tables based on screen size, are allocated once
// and for all at the maximum size.
#if defined (_WIN32_WCE) || defined (DC) || defined (_PSP) || defined (_NDS)
#define MAXVIDWIDTH 320
#define MAXVIDHEIGHT 200
#elif defined (GP2X)
#define MAXVIDWIDTH 320 //720
#define MAXVIDHEIGHT 240 //576
#elif defined (WII) // Wii, VGA/640x480
#define MAXVIDWIDTH 640
#define MAXVIDHEIGHT 480
#else
#define MAXVIDWIDTH 1920 // don't set this too high because actually
#define MAXVIDHEIGHT 1200 // lots of tables are allocated with the MAX size.
#endif
#define BASEVIDWIDTH 320 // NEVER CHANGE THIS! This is the original
#define BASEVIDHEIGHT 200 // resolution of the graphics.

// global video state
typedef struct viddef_s
{
	INT32 modenum; // vidmode num indexes videomodes list

	UINT8 *buffer; // invisible screens buffer
	size_t rowbytes; // bytes per scanline of the VIDEO mode
	INT32 width; // PIXELS per scanline
	INT32 height;
	union { // don't need numpages for OpenGL, so we can use it for fullscreen/windowed mode
		INT32 numpages; // always 1, page flipping todo
		INT32 windowed; // windowed or fullscren mode?
	} u;
	INT32 recalc; // if true, recalc vid-based stuff
	UINT8 *direct; // linear frame buffer, or vga base mem.
	INT32 dupx, dupy; // scale 1, 2, 3 value for menus & overlays
	INT32/*fixed_t*/ fdupx, fdupy; // same as dupx, dupy, but exact value when aspect ratio isn't 320/200
	INT32 bpp; // BYTES per pixel: 1 = 256color, 2 = highcolor

	INT32 baseratio; // Used to get the correct value for lighting walls

	// for Win32 version
	DNWH WndParent; // handle of the application's window
	UINT8 smalldupx, smalldupy; // factor for a little bit of scaling
	UINT8 meddupx, meddupy; // factor for moderate, but not full, scaling
#ifdef HWRENDER
	INT32/*fixed_t*/ fsmalldupx, fsmalldupy;
	INT32/*fixed_t*/ fmeddupx, fmeddupy;
#endif
} viddef_t;
#define VIDWIDTH vid.width
#define VIDHEIGHT vid.height

// internal additional info for vesa modes only
typedef struct
{
	INT32 vesamode; // vesa mode number plus LINEAR_MODE bit
	void *plinearmem; // linear address of start of frame buffer
} vesa_extra_t;
// a video modes from the video modes list,
// note: video mode 0 is always standard VGA320x200.
typedef struct vmode_s
{
	struct vmode_s *pnext;
	char *name;
	UINT32 width, height;
	UINT32 rowbytes; // bytes per scanline
	UINT32 bytesperpixel; // 1 for 256c, 2 for highcolor
	INT32 windowed; // if true this is a windowed mode
	INT32 numpages;
	vesa_extra_t *pextradata; // vesa mode extra data
#if defined (_WIN32) && !defined (_XBOX)
	INT32 (WINAPI *setmode)(viddef_t *lvid, struct vmode_s *pcurrentmode);
#else
	INT32 (*setmode)(viddef_t *lvid, struct vmode_s *pcurrentmode);
#endif
	INT32 misc; // misc for display driver (r_opengl.dll etc)
} vmode_t;

#define NUMSPECIALMODES  4
extern vmode_t specialmodes[NUMSPECIALMODES];

// ---------------------------------------------
// color mode dependent drawer function pointers
// ---------------------------------------------

extern void (*wallcolfunc)(void);
extern void (*colfunc)(void);
extern void (*basecolfunc)(void);
extern void (*fuzzcolfunc)(void);
extern void (*transcolfunc)(void);
extern void (*shadecolfunc)(void);
extern void (*spanfunc)(void);
extern void (*basespanfunc)(void);
extern void (*splatfunc)(void);
extern void (*transtransfunc)(void);
extern void (*twosmultipatchfunc)(void);
extern void (*twosmultipatchtransfunc)(void);

// -----
// CPUID
// -----
extern boolean R_ASM;
extern boolean R_486;
extern boolean R_586;
extern boolean R_MMX;
extern boolean R_3DNow;
extern boolean R_MMXExt;
extern boolean R_SSE2;

// ----------------
// screen variables
// ----------------
extern viddef_t vid;
extern INT32 setmodeneeded; // mode number to set if needed, or 0

extern INT32 scr_bpp;
extern UINT8 *scr_borderpatch; // patch used to fill the view borders

extern consvar_t cv_scr_width, cv_scr_height, cv_scr_depth, cv_renderview, cv_fullscreen;
// wait for page flipping to end or not
extern consvar_t cv_vidwait;

// quick fix for tall/short skies, depending on bytesperpixel
extern void (*walldrawerfunc)(void);

// Change video mode, only at the start of a refresh.
void SCR_SetMode(void);
void SCR_SetupDrawRoutines(void);
// Recalc screen size dependent stuff
void SCR_Recalc(void);
// Check parms once at startup
void SCR_CheckDefaultMode(void);
// Set the mode number which is saved in the config
void SCR_SetDefaultMode(void);

void SCR_Startup (void);

FUNCMATH boolean SCR_IsAspectCorrect(INT32 width, INT32 height);

// move out to main code for consistency
void SCR_DisplayTicRate(void);
#undef DNWH
#endif //__SCREEN_H__
