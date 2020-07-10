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
/// \brief hardware and software level, screen and video i/o, refresh,
///	setup ... a big mess. Got to clean that up!

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <sys/socket.h>

#include <netinet/in.h>
//#include <errnos.h>
#include <signal.h>

#include <go32.h>
#include <pc.h>
#include <dpmi.h>
#include <dos.h>
#include <sys/nearptr.h>

#include "../doomdef.h"
#include "../i_system.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "vid_vesa.h"
#include "../i_video.h"


//dosstuff -newly added
static unsigned long dascreen;
static int gfx_use_vesa1;

boolean    highcolor;

#define SCREENDEPTH   1     // bytes per pixel, do NOT change.

rendermode_t    rendermode=render_soft;

//
// I_OsPolling
//
void I_OsPolling(void)
{
	I_GetEvent();
	//i dont think i have to do anything else here
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
	// what is this?
}


//profile stuff ---------------------------------------------------------
//added:16-01-98:wanted to profile the VID_BlitLinearScreen() asm code.
//#define TIMING      //uncomment this to enable profiling
#ifdef TIMING
#include "../p5prof.h"
static   INT64 mycount;
static   INT64 mytotal = 0;
static   unsigned long  nombre = NEWTICRATE*10;
//static   char runtest[10][80];
#endif
//profile stuff ---------------------------------------------------------

//
// I_FinishUpdate
//
static void I_BlitScreenVesa1(void);   //see later
void I_FinishUpdate (void)
{
	if (marathonmode)
		SCR_DisplayMarathonInfo();

	// draw captions if enabled
	if (cv_closedcaptioning.value)
		SCR_ClosedCaptions();

	// draw FPS if enabled
	if (cv_ticrate.value)
		SCR_DisplayTicRate();

	if (cv_showping.value && netgame && consoleplayer != serverplayer)
		SCR_DisplayLocalPing();

	//blast it to the screen
	// this code sucks
	//memcpy(dascreen,screens[0],screenwidth*screenheight);

	//added:03-01-98: I tried to I_WaitVBL(1) here, but it slows down
	//  the game when the view becomes complicated, it looses ticks
	if (cv_vidwait.value)
		I_WaitVBL(1);


//added:16-01-98:profile screen blit.
#ifdef TIMING
	ProfZeroTimer();
#endif
	//added:08-01-98: support vesa1 bank change, without Allegro's BITMAP screen.
	if ( gfx_use_vesa1 )
	{
		I_Error("Banked screen update not finished for dynamic res\n");
		I_BlitScreenVesa1();    //blast virtual to physical screen.
	}
	else
	{
		//added:16-01-98:use quickie asm routine, last 2 args are
		//                   src and dest rowbytes
		//                   (memcpy is as fast as this one...)
		VID_BlitLinearScreen(screens[0], vid.direct,
		                     vid.width*vid.bpp, vid.height,
		                     vid.width*vid.bpp, vid.rowbytes );
	}
#ifdef TIMING
	RDMSR(0x10,&mycount);
	mytotal += mycount;   //64bit add

	if (nombre--==0)
		I_Error("ScreenBlit CPU Spy reports: 0x%d %d\n", *((int *)&mytotal+1),
		        (int)mytotal );
#endif

}

//
// I_UpdateNoVsync
//
void I_UpdateNoVsync(void)
{
	int real_vidwait = cv_vidwait.value;
	cv_vidwait.value = 0;
	I_FinishUpdate();
	cv_vidwait.value = real_vidwait;
}

//
// I_ReadScreen
//
void I_ReadScreen (UINT8 *scr)
{
	VID_BlitLinearScreen(screens[0], scr,
	                     vid.width*vid.bpp, vid.height,
	                     vid.width*vid.bpp, vid.rowbytes );
}


void I_SetPalette (RGBA_t *palette)
{
	int i;

	outportb(0x3c8,0);
	for (i=0;i<256;i++,palette++)
	{
		outportb(0x3c9,palette->s.red>>2);
		outportb(0x3c9,palette->s.green>>2);
		outportb(0x3c9,palette->s.blue>>2);
	}
}


//added 29-12-1997
/*==========================================================================*/
// I_BlastScreen : copy the virtual screen buffer to the physical screen mem
//                 using bank switching if needed.
/*==========================================================================*/
static void I_BlitScreenVesa1(void)
{
#define VIDBANKSIZE     (1<<16)
#define VIDBANKSIZEMASK (VIDBANKSIZE-1)   // defines ahoy!

	__dpmi_regs r;
	UINT8 *p_src;
	long     i;
	long     virtualsize;

	// virtual screen buffer size
	virtualsize = vid.rowbytes * vid.height * SCREENDEPTH;

	p_src  = screens[0];

	for (i=0; virtualsize > 0; i++ )
	{
		r.x.ax = 0x4f05;
		r.x.bx = 0x0;
		r.x.cx = 0x0;
		r.x.dx = i;
		__dpmi_int(0x10,&r);      //set bank

		M_Memcpy((UINT8 *)dascreen,p_src,(virtualsize < VIDBANKSIZE) ? virtualsize : VIDBANKSIZE );

		p_src += VIDBANKSIZE;
		virtualsize -= VIDBANKSIZE;
	}

}


//added:08-01-98: now we use Allegro's set_gfx_mode, but we want to
//                restore the exact text mode that was before.
static INT16  myOldVideoMode;
static inline void I_SaveOldVideoMode(void)
{
	__dpmi_regs r;
	r.x.ax = 0x4f03;                 // Return current video mode
	__dpmi_int(0x10,&r);
	if ( r.x.ax != 0x4f )
		myOldVideoMode = -1;
	else
		myOldVideoMode = r.x.bx;
}


//
//  Close the screen, restore previous video mode.
//
void I_ShutdownGraphics (void)
{
	__dpmi_regs r;

	rendermode = render_none;
	if ( !graphics_started )
		return;

	// free the last video mode screen buffers
	if (vid.buffer)
		free (vid.buffer);

	/* Restore old video mode */
	if (myOldVideoMode!=-1)
	{
		/* Restore old video mode */
		r.x.ax = 0x4f02;                 // Set Super VGA video mode
		r.x.bx = myOldVideoMode;
		__dpmi_int(0x10,&r);

		// Boris: my s3 don't do a cls because "win95" :<
		clrscr();
	}
	else  // no vesa put the normal video mode
	{
		r.x.ax = 0x03;
		__dpmi_int(0x10,&r);
	}

	graphics_started = false;
}


//added:08-01-98:
//  Set VESA1 video mode, coz Allegro set_gfx_mode a larger screenwidth...
//
#if 0
int set_vesa1_mode( int width, int height )
{
	__dpmi_regs r;

	// setup video mode.
	r.x.ax = 0x4f02;
	if ( ( width==320 )&&( height==200 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x13;                             // 320x 200x1 (256 colors)
	else
	if ( ( width==320 )&&( height==240 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x154;                            // 320x 240x1 (256 colors)
	else
	if ( ( width==320 )&&( height==400 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x155;                            // 320x 400x1 (256 colors)
	else
	if ( ( width==640 )&&( height==400 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x100;                            // 640x 400x1 (256 colors)
	else
	if ( ( width==640 )&&( height==480 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x101;                            // 640x 480x1 (256 colors)
	else
	if ( ( width==800 )&&( height==600 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x103;                            // 800x 600x1 (256 colors)
	else
	if ( ( width==1024)&&( height==768 ) && ( SCREENDEPTH==1 ) )
		r.x.bx   = 0x105;                            //1024x 768x1 (256 colors)
	else
		I_Error("I_SetVesa1Mode: video mode not supported.");

	// enter graphics mode.
	__dpmi_int(0x10,&r);

	if ( r.x.ax != 0x4f )
		I_Error("I_SetVesa1Mode: init video mode failed !");

	return 0;
}
#endif


//added:08-01-98: now uses Allegro to setup Linear Frame Buffer video modes.
//
//  Initialize video mode, setup dynamic screen size variables,
//  and allocate screens.
//
void I_StartupGraphics(void)
{
	//added:26-01-98: VID_Init() must be done only once,
	//                use VID_SetMode() to change vid mode while in the game.
	if ( graphics_started )
		return;

	// remember the exact screen mode we were...
	I_SaveOldVideoMode();

	CONS_Printf("Vid_Init...");

	// 0 for 256 color, else use highcolor modes
	highcolor = M_CheckParm ("-highcolor");

	VID_Init();

	//gfx_use_vesa1 = false;

	//added:03-01-98: register exit code for graphics
	I_AddExitFunc(I_ShutdownGraphics);
	graphics_started = true;

}

void VID_StartupOpenGL(void) {}
