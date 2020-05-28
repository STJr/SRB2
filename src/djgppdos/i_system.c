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
/// \brief Misc. stuff
///
///	Startup & Shutdown routines for music,sound,timer,keyboard,
///	Signal handler to trap errors and exit cleanly.

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <io.h>
#include <stdarg.h>
#include <sys/time.h>
#include <fcntl.h>

#ifdef DJGPP
 #include <dpmi.h>
 #include <go32.h>
 #include <pc.h>
 #include <dos.h>
 #include <crt0.h>
 #include <sys/segments.h>
 #include <sys/nearptr.h>

 #include <keys.h>
#endif


#include "../doomdef.h"
#include "../m_misc.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../d_net.h"
#include "../g_game.h"

#include "../d_main.h"

#include "../m_argv.h"

#include "../w_wad.h"
#include "../z_zone.h"
#include "../g_input.h"

#include "../console.h"

#include "../m_menu.h"

#ifdef __GNUG__
 #pragma implementation "../i_system.h"
#endif

#include "../i_joy.h"

//### let's try with Allegro ###
#define  alleg_mouse_unused
//#define  alleg_timer_unused
#define  alleg_keyboard_unused
#define  ALLEGRO_NO_KEY_DEFINES
//#define  alleg_joystick_unused
#define  alleg_gfx_driver_unused
#define  alleg_palette_unused
#define  alleg_graphics_unused
#define  alleg_vidmem_unused
#define  alleg_flic_unused
#define  alleg_sound_unused
#define  alleg_file_unused
#define  alleg_datafile_unused
#define  alleg_math_unused
#define  alleg_gui_unused
#include <allegro.h>
//### end of Allegro include ###

#ifndef DOXYGEN

#ifndef MAX_JOYSTICKS
#define MAX_JOYSTICKS 4
#endif

#ifndef MAX_JOYSTICK_AXIS
#define MAX_JOYSTICK_AXIS 3
#endif

#ifndef MAX_JOYSTICK_STICKS
#define MAX_JOYSTICK_STICKS 4
#endif

#ifndef MAX_JOYSTICK_BUTTONS
#define MAX_JOYSTICK_BUTTONS 12
#endif

#endif

#if ALLEGRO_VERSION == 4
static char JOYFILE[] = "allegro4.cfg";
#elif ALLEGRO_VERSION == 3
static char JOYFILE[] = "allegro.cfg";
#endif

/// \brief max number of joystick buttons
#define JOYBUTTONS_MAX MAX_JOYSTICK_BUTTONS // <allegro/joystick.h>
/// \brief max number of joystick button events
#define JOYBUTTONS_MIN min((JOYBUTTONS),(JOYBUTTONS_MAX))

/// \brief max number of joysick axies
#define JOYAXISSET_MAX MAX_JOYSTICK_STICKS
// \brief max number ofjoystick axis events
#define JOYAXISSET_MIN min((JOYAXISSET),(JOYAXISSET_MAX))

/// \brief max number of joystick hats
#define JOYHATS_MAX MAX_JOYSTICK_STICKS
/// \brief max number of joystick hat events
#define JOYHATS_MIN min((JOYHATS),(JOYHATS_MAX))

/// \brief max number of mouse buttons
#define MOUSEBUTTONS_MAX 16 // 16 bit of BL
/// \brief max number of muse button events
#define MOUSEBUTTONS_MIN min((MOUSEBUTTONS),(MOUSEBUTTONS_MAX))

// Do not execute cleanup code more than once. See Shutdown_xxx() routines.
UINT8 graphics_started = false;
UINT8 keyboard_started = false;
UINT8 sound_started    = false;
static UINT8 timer_started    = false;

/* Mouse stuff */
static UINT8 mouse_detected   = false;
static UINT8 wheel_detected   = false;

static volatile tic_t ticcount;   //returned by I_GetTime(), updated by timer interrupt


void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	// UNUSED.
	Type = EvilForce;
	Effect = NULL;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	// UNUSED.
	Type = EvilForce;
	Effect = NULL;
}

static ticcmd_t        emptycmd;
ticcmd_t *      I_BaseTiccmd(void)
{
	return &emptycmd;
}

static ticcmd_t        emptycmd2;
ticcmd_t *      I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

void I_SetupMumble(void)
{
}

#ifndef NOMUMBLE
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
	(void)mobj;
	(void)listener;
}
#endif

//
//  Allocates the base zone memory,
//  this function returns a valid pointer and size,
//  else it should interrupt the program immediately.
//
//added:11-02-98: now checks if mem could be allocated, this is still
//    prehistoric... there's a lot to do here: memory locking, detection
//    of win95 etc...
//

#if ALLEGRO_VERSION == 3
extern int os_type;
extern int windows_version;
extern int windows_sub_version;
extern int i_love_bill;
#elif ALLEGRO_VERSION == 4
#endif

static void I_DetectOS (void)
{
#if ALLEGRO_VERSION == 3
	char buf[16];
	__dpmi_regs r;
	union REGS regs;

	/* check which OS we are running under */
	r.x.ax = 0x1600;
	__dpmi_int(0x2F, &r);
	if ((r.h.al != 0) && (r.h.al != 1) && (r.h.al != 0x80) && (r.h.al != 0xFF))
	{
		/* win 3.1 or 95 */
		if (r.h.al == 4)
		{
			if (r.h.ah < 10)
			{
				os_type = OSTYPE_WIN95;
			}
			else
			{
				os_type = OSTYPE_WIN98;
			}
		}
		else
		{
			os_type = OSTYPE_WIN3;
		}

		windows_version = r.h.al;
		windows_sub_version = r.h.ah;
		i_love_bill = TRUE;
	}
	else
	{
		if (_get_dos_version(1) == 0x0532)
		{
			/* win NT */
			os_type = OSTYPE_WINNT;
			windows_version = 0x100;
			windows_sub_version = 0;
			i_love_bill = TRUE;
		}
		else
		{
			/* see if OS/2 is present */
			r.x.ax = 0x4010;
			__dpmi_int(0x2F, &r);
			if (r.x.ax != 0x4010)
			{
				if (r.x.ax == 0x0000)
				{
					/* OS/2 Warp 3 */
					os_type = OSTYPE_WARP;
					i_love_bill = TRUE;
				}
				else
				{
					/* no Warp, but previous OS/2 is available */
					os_type = OSTYPE_OS2;
					i_love_bill = TRUE;
				}
			}
			else
			{
				/* check if running under Linux DOSEMU */
				dosmemget(0xFFFF5, 10, buf);
				buf[8] = 0;
				if (!strcmp(buf, "02/25/93"))
				{
					regs.x.ax = 0;
					int86(0xE6, &regs, &regs);
					if (regs.x.ax == 0xAA55)
					{
						os_type = OSTYPE_DOSEMU;
						windows_version = -1;
						windows_sub_version = -1;
						i_love_bill = TRUE;     /* (evil chortle) */
					}
				}
				else
				{
					/* check if running under OpenDOS */
					r.x.ax = 0x4452;
					__dpmi_int(0x21, &r);
					if ((r.x.ax >= 0x1072) && !(r.x.flags & 1))
					{
						os_type = OSTYPE_OPENDOS;
						/* now check for OpenDOS EMM386.EXE */
						r.x.ax = 0x12FF;
						r.x.bx = 0x0106;
						__dpmi_int(0x2F, &r);
						if ((r.x.ax == 0x0) && (r.x.bx == 0xEDC0))
						{
							i_love_bill = TRUE;
						}
					}
				}
			}
		}
	}
#elif ALLEGRO_VERSION == 4
/// \todo: add Allegro 4 version
#endif
}

UINT32 I_GetFreeMem(UINT32 *total)
{
	__dpmi_free_mem_info     info;

	__dpmi_get_free_memory_information(&info);
	if ( total )
		*total = info.total_number_of_physical_pages<<12; // <<12 for convert page to byte
	return info.total_number_of_free_pages<<12;
}


/*==========================================================================*/
// I_GetTime ()
/*==========================================================================*/
tic_t I_GetTime (void)
{
	return ticcount;
}


void I_Sleep(void)
{
	if (cv_sleep.value > 0)
		rest(cv_sleep.value);
}


static UINT8 joystick_detected = false;
static UINT8 joystick2_detected = false;

//
// I_Init
//


FUNCINLINE static ATTRINLINE int I_WaitJoyButton (int js)
{
	CON_Drawer ();
	I_FinishUpdate ();        // page flip or blit buffer

	do
	{
		if (I_GetKey())
			return false;
		poll_joystick();
	} while (!(joy[js].button[0].b || joy[js].button[1].b));

	return true;
}

/**	\brief Joystick 1 buttons states
*/
static INT64 lastjoybuttons = 0;
/**	\brief Joystick 1 hats state
*/
static INT64 lastjoyhats = 0;

void I_InitJoystick (void)
{
	//init the joystick
	if (joystick_detected && !joystick2_detected)
		remove_joystick();
	joystick_detected=0;
	if (M_CheckParm("-nojoy"))
		return;
	load_joystick_data(JOYFILE);
	if (cv_usejoystick.value)
	{
		if (cv_usejoystick.value > MAX_JOYSTICKS)
			cv_usejoystick.value = MAX_JOYSTICKS;
		if (install_joystick(JOY_TYPE_AUTODETECT) == 0)
		{
			int js = cv_usejoystick.value -1;
			// only gamepadstyle joysticks
			if (joy[js].stick[0].flags & JOYFLAG_DIGITAL) Joystick.bGamepadStyle=true;

			while (joy[js].flags & JOYFLAG_CALIBRATE)
			{
				const char *msg = calibrate_joystick_name(js);
				CONS_Printf("%s, and press a button\n", msg);
				if (I_WaitJoyButton(js))
					calibrate_joystick(js);
				else
				{
					if (joystick_detected && !joystick2_detected)
						remove_joystick();
					joystick_detected=0;
					CV_SetValue(&cv_usejoystick, 0);
					return;
				}
			}
			joystick_detected=1;
			save_joystick_data(JOYFILE);
		}
		else
		{
			CONS_Printf("\2No Joystick detected.\n");
		}
	}
	else
	{
		int i;
		event_t event;
		event.type=ev_keyup;
		event.data2 = 0;
		event.data3 = 0;

		lastjoybuttons = lastjoyhats = 0;

		// emulate the up of all joystick buttons
		for (i=0;i<JOYBUTTONS;i++)
		{
			event.data1=KEY_JOY1+i;
			D_PostEvent(&event);
		}

		// emulate the up of all joystick hats
		for (i=0;i<JOYHATS*4;i++)
		{
			event.data1=KEY_HAT1+i;
			D_PostEvent(&event);
		}

		// reset joystick position
		event.type = ev_joystick;
		for (i=0;i<JOYAXISSET; i++)
		{
			event.data1 = i;
			D_PostEvent(&event);
		}
	}
}

/**	\brief Joystick 2 buttons states
*/
static INT64 lastjoy2buttons = 0;
/**	\brief Joystick 2 hats state
*/
static INT64 lastjoy2hats = 0;

void I_InitJoystick2 (void)
{
	//init the joystick
	if (joystick2_detected && !joystick_detected)
		remove_joystick();
	joystick2_detected=0;
	if (M_CheckParm("-nojoy"))
		return;
	if (cv_usejoystick2.value)
	{
		if (cv_usejoystick2.value > MAX_JOYSTICKS)
			cv_usejoystick2.value = MAX_JOYSTICKS;
		if (install_joystick(JOY_TYPE_AUTODETECT) == 0)
		{
			int js = cv_usejoystick2.value -1;
			// only gamepadstyle joysticks
			load_joystick_data(JOYFILE);
			if (joy[js].stick[0].flags & JOYFLAG_DIGITAL) Joystick2.bGamepadStyle=true;

			while (joy[js].flags & JOYFLAG_CALIBRATE)
			{
				const char *msg = calibrate_joystick_name(js);
				CONS_Printf("%s, and press a button\n", msg);
				if (I_WaitJoyButton(js))
					calibrate_joystick(js);
				else
				{
					if (joystick2_detected && !joystick_detected)
						remove_joystick();
					joystick2_detected=0;
					CV_SetValue(&cv_usejoystick2, 0);
					return;
				}
			}
			joystick2_detected=1;
			save_joystick_data(JOYFILE);
		}
		else
		{
			CONS_Printf("\2No Joystick detected.\n");
		}
	}
	else
	{
		int i;
		event_t event;
		event.type=ev_keyup;
		event.data2 = 0;
		event.data3 = 0;

		lastjoy2buttons = lastjoy2hats = 0;

		// emulate the up of all joystick buttons
		for (i=0;i<JOYBUTTONS;i++)
		{
			event.data1=KEY_2JOY1+i;
			D_PostEvent(&event);
		}

		// emulate the up of all joystick hats
		for (i=0;i<JOYHATS*4;i++)
		{
			event.data1=KEY_2HAT1+i;
			D_PostEvent(&event);
		}

		// reset joystick position
		event.type = ev_joystick2;
		for (i=0;i<JOYAXISSET; i++)
		{
			event.data1 = i;
			D_PostEvent(&event);
		}
	}
}

//added:18-02-98: put an error message (with format) on stderr
void I_OutputMsg (const char *error, ...)
{
	va_list     argptr;

	va_start (argptr,error);
	vfprintf (stderr,error,argptr);
	va_end (argptr);

	// dont flush the message!
}

static int errorcount=0; // fuck recursive errors
static int shutdowning=false;

//
// I_Error
//
//added 31-12-97 : display error messy after shutdowngfx
void I_Error (const char *error, ...)
{
	va_list     argptr;
	// added 11-2-98 recursive error detecting

	M_SaveConfig (NULL);   //save game config, cvars..
#ifndef NONET
	D_SaveBan(); // save the ban list
#endif
	G_SaveGameData(); // Tails 12-08-2002
	if (demorecording)
		G_CheckDemoStatus();
	D_QuitNetGame ();
	M_FreePlayerSetupColors();

	if (shutdowning)
	{
		errorcount++;
		if (errorcount==5)
			I_ShutdownGraphics();
		if (errorcount==6)
			I_ShutdownSystem();
		if (errorcount>7)
			exit(-1);       // recursive errors detected
	}
	shutdowning=true;

	//added:18-02-98: save one time is enough!
	if (!errorcount)
	{
		M_SaveConfig (NULL);   //save game config, cvars..
		G_SaveGameData(); // Tails 12-08-2002
	}

	//added:16-02-98: save demo, could be useful for debug
	//                NOTE: demos are normally not saved here.
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	I_ShutdownGraphics();
	I_ShutdownSystem();

	// put message to stderr
	va_start (argptr,error);
	fprintf (stderr, "Error: ");
	vfprintf (stderr,error,argptr);
#ifdef DEBUGFILE
	if (debugfile)
	{
		fprintf (debugfile,"I_Error :");
		vfprintf (debugfile,error,argptr);
	}
#endif

	va_end (argptr);

	fprintf (stderr, "\nPress ENTER");
	fflush( stderr );
	getchar();
	W_Shutdown();
	exit(-1);
}


//
// I_Quit : shutdown everything cleanly, in reverse order of Startup.
//
void I_Quit (void)
{
	UINT8 *endoom = NULL;

	//added:16-02-98: when recording a demo, should exit using 'q' key,
	//        but sometimes we forget and use 'F10'.. so save here too.
	M_SaveConfig (NULL);   //save game config, cvars..
#ifndef NONET
	D_SaveBan(); // save the ban list
#endif
	G_SaveGameData(); // Tails 12-08-2002
	if (demorecording)
		G_CheckDemoStatus();
	D_QuitNetGame ();
	M_FreePlayerSetupColors();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	I_ShutdownGraphics();
	I_ShutdownSystem();

	if (W_CheckNumForName("ENDOOM")!=LUMPERROR) endoom = W_CacheLumpName("ENDOOM",PU_CACHE);


	//added:03-01-98: maybe it needs that the ticcount continues,
	// or something else that will be finished by ShutdownSystem()
	// so I do it before.

	if (endoom)
	{
		puttext(1,1,80,25,endoom);
		gotoxy(1,24);
		Z_Free(endoom);
	}

	if (shutdowning || errorcount)
		I_Error("Error detected (%d)",errorcount);

	fflush(stderr);
	W_Shutdown();
	exit(0);
}


//added:12-02-98: does want to work!!!! rhaaahahha
void I_WaitVBL(INT32 count)
{
	while (count-->0);
	{
		do { } while (inportb(0x3DA) & 8);
		do { } while (!(inportb(0x3DA) & 8));
	}
}

//  Fab: this is probably to activate the 'loading' disc icon
//       it should set a flag, that I_FinishUpdate uses to know
//       whether it draws a small 'loading' disc icon on the screen or not
//
//  also it should explicitly draw the disc because the screen is
//  possibly not refreshed while loading
//
void I_BeginRead (void)
{
}

//  Fab: see above, end the 'loading' disc icon, set the flag false
//
void I_EndRead (void)
{
}

#define MOUSE2
/* Secondary Mouse*/
#ifdef MOUSE2
static _go32_dpmi_seginfo oldmouseinfo,newmouseinfo;
static boolean mouse2_started=0;
static UINT16  mouse2port;
static UINT8   mouse2irq;
static volatile int     handlermouse2buttons;
static volatile int     handlermouse2x,handlermouse2y;
// internal use
static volatile int     bytenum;
static volatile UINT8   combytes[8];

//
// support a secondary mouse without mouse driver !
//
// take from the PC-GPE by Mark Feldman
static void I_MicrosoftMouseIntHandler(void)
{
	char   dx,dy;
	UINT8  inbyte;

	// Get the port byte
	inbyte = inportb(mouse2port);

	// Make sure we are properly "synched"
	if ((inbyte & 64)== 64 || bytenum>7)
		bytenum = 0;

	// Store the byte and adjust bytenum
	combytes[bytenum] = inbyte;
	bytenum++;

	// Have we received all 3 bytes?
	if (bytenum==3)
	{
		// Yes, so process them
		dx = ((combytes[0] & 3) << 6) + combytes[1];
		dy = ((combytes[0] & 12) << 4) + combytes[2];
		handlermouse2x+= dx;
		handlermouse2y+= dy;
		handlermouse2buttons = (combytes[0] & (32+16)) >>4;
	}
	else if (bytenum==4) // for logitech 3 buttons
	{
		if (combytes[3] & 32)
			handlermouse2buttons |= 4;
		else
			handlermouse2buttons &= ~4;
	}

	// Acknowledge the interrupt
	outportb(0x20,0x20);
}
#ifndef DOXYGEN
static END_OF_FUNCTION(I_MicrosoftMouseIntHandler);
#endif

// wait ms milliseconde
FUNCINLINE static ATTRINLINE void I_Delay(int ms)
{
	tic_t  starttime;

	if (timer_started)
	{
		starttime=I_GetTime()+(NEWTICRATE*ms)/1000;
		while (starttime>=I_GetTime())
			I_Sleep();
	}
	else
		delay(ms);
}

//
//  Removes the mouse2 handler.
//
static void I_ShutdownMouse2(void)
{
	event_t event;
	int i;

	if ( !mouse2_started )
		return;

	outportb(mouse2port+4,0x00);   // shutdown mouse (DTR & RTS = 0)
	I_Delay(1);
	outportb(mouse2port+1,0x00);   // disable COM interuption

	asm("cli");
	_go32_dpmi_set_protected_mode_interrupt_vector(mouse2irq, &oldmouseinfo);
	_go32_dpmi_free_iret_wrapper(&newmouseinfo);
	asm("sti");

	handlermouse2x=handlermouse2y=handlermouse2buttons=0;
	// emulate the up of all mouse buttons
	for (i=0;i<MOUSEBUTTONS;i++)
	{
		event.type=ev_keyup;
		event.data1=KEY_2MOUSE1+i;
		D_PostEvent(&event);
	}

	mouse2_started=false;
}

static UINT8  ComIrq[4]={0x0c,0x0b,0x0c,0x0b};
static UINT16 ComPort[4]={0x3F8,0x2F8,0x3E8,0x2E8};
//
//  Installs the mouse2 handler.
//
void I_StartupMouse2(void)
{
	tic_t i;
	boolean  found;
	__dpmi_regs r;

	if ( mouse2_started )
		I_ShutdownMouse2();

	if (!cv_usemouse2.value)
		return;

	handlermouse2x=handlermouse2y=handlermouse2buttons=0;

	mouse2irq =ComIrq[cv_mouse2port.value-1];
	mouse2port=ComPort[cv_mouse2port.value-1];
	CONS_Printf("Using %s (irq %d, port 0x%x)\n",cv_mouse2port.string,mouse2irq-8,mouse2port);
	r.x.ax=0x24;
	__dpmi_int(0x33,&r);
	if (r.h.cl+8==mouse2irq)
	{
		CONS_Printf("Irq conflict with mouse 1\n"
					"Use mouse2port to change the port\n");
		return;
	}

	// install irq wrapper
	asm("cli");
	_go32_dpmi_get_protected_mode_interrupt_vector(mouse2irq, &oldmouseinfo);
	newmouseinfo.pm_selector=_go32_my_cs();
	newmouseinfo.pm_offset=(int)I_MicrosoftMouseIntHandler;
	_go32_dpmi_allocate_iret_wrapper(&newmouseinfo);
	_go32_dpmi_set_protected_mode_interrupt_vector(mouse2irq, &newmouseinfo);

	LOCK_VARIABLE(bytenum);
	LOCK_VARIABLE(handlermouse2x);
	LOCK_VARIABLE(handlermouse2y);
	LOCK_VARIABLE(handlermouse2buttons);
	LOCK_VARIABLE(mouse2port);
	_go32_dpmi_lock_data(&combytes,sizeof (combytes));
	LOCK_FUNCTION(I_MicrosoftMouseIntHandler);
	asm("sti");

	outportb(mouse2port+4,0   );   // shutdown mouse (DTR & RTS = 0)
	I_Delay(1);
	outportb(mouse2port+1,0   );   // disable COM interuption
	I_Delay(1);
	outportb(mouse2port+3,0x80);   // change status of port +0 et +1
	I_Delay(1);                    // for baudrate programmation
	outportb(mouse2port  ,0x60);   // 1200 LSB
	I_Delay(1);
	outportb(mouse2port+1,0   );   // 1200 MSB
	I_Delay(1);
	outportb(mouse2port+3,0x02);   // set port protocol 7N1
	I_Delay(1);
	outportb(mouse2port+1,0x01);   // enable COM interuption
	I_Delay(1);
	outportb(0x21,0x0);

	// wait to be sure the mouse have shutdown
	I_Delay(100);

	outportb(mouse2port+4,0x0b);   // restart mouse
	i=I_GetTime()+NEWTICRATE;
	found=cv_usemouse2.value==2;
	while (I_GetTime()<i || !found)
		if (combytes[0]!='M')
			found=true;

	if (found || cv_usemouse2.value==2)
	{
		CONS_Printf("Microsoft compatible Secondary Mouse detected\n");

		//register shutdown mouse2 code.
		I_AddExitFunc(I_ShutdownMouse2);
		mouse2_started = true;
	}
	else
	{
		CONS_Printf("Secondary Mouse not found\n");
		// remove irq wraper
		I_ShutdownMouse2();
	}
}
#endif

//  Initialise the mouse. Doesnt need to be shutdown.
//
void I_StartupMouse (void)
{
	__dpmi_regs r;

	// mouse detection may be skipped by setting usemouse false
	if (cv_usemouse.value == 0)
	{
		mouse_detected=false;
		I_ShutdownMouse2();
		return;
	}

	//detect mouse presence
	r.x.ax=0;
	__dpmi_int(0x33,&r);

	//added:03-01-98:
	if ( r.x.ax == 0 && cv_usemouse.value != 2)
	{
		mouse_detected=false;
		CONS_Printf("\2I_StartupMouse: mouse not present.\n");
	}
	else
	{
		mouse_detected=true;

		// Check for CTMOUSE wheel
		r.x.ax = 0x11;
		__dpmi_int(0x33,&r);
		if ( r.x.ax == 0x574D && r.x.cx & 0x1) // check for "MW" in AX and wheel mouse in CX
		{
			wheel_detected=true;
		}

		//hide cursor
		r.x.ax=0x02;
		__dpmi_int(0x33,&r);

		//reset mickey count
		r.x.ax=0x0b;
		__dpmi_int(0x33,&r);
	}
}

void I_GetJoystickEvents(void)
{
	event_t event;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	int s = 0, i;
	int js = cv_usejoystick.value - 1;

	if (!joystick_detected)
		return;

	// I assume that true is 1
	for (i = JOYBUTTONS_MIN - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (joy[js].button[i].b)
			joybuttons |= 1;
	}

	for (i = JOYHATS_MIN -1; i >=0;)
	{
		if (joy[js].stick[s].flags &  JOYFLAG_DIGITAL)
		{
			if (joy[js].stick[s].axis[1].d1) joyhats |= 1<<(0 + 4*i);
			if (joy[js].stick[s].axis[1].d2) joyhats |= 1<<(1 + 4*i);
			if (joy[js].stick[s].axis[0].d1) joyhats |= 1<<(2 + 4*i);
			if (joy[js].stick[s].axis[0].d2) joyhats |= 1<<(3 + 4*i);
			i--;
		}
		if (s == JOYHATS_MAX) i = -1;
		s++;
	}

	// post key event for buttons
	if (joybuttons!=lastjoybuttons)
	{
		INT64 j = 1; // only changed bit to 1
		INT64 k = (joybuttons ^ lastjoybuttons);
		lastjoybuttons=joybuttons;

		for (i=0;i<JOYBUTTONS && i<JOYBUTTONS_MAX;i++,j<<=1)
			if (k & j)          // test the eatch bit and post the corresponding event
			{
				if (joybuttons & j)
					event.type=ev_keydown;
				else
					event.type=ev_keyup;
				event.data1=KEY_JOY1+i;
				D_PostEvent(&event);
			}
	}

	// post key event for hats
	if (joyhats!=lastjoyhats)
	{
		INT64 j = 1; // only changed bit to 1
		INT64 k = (joyhats ^ lastjoyhats);
		lastjoyhats=joyhats;

		for (i=0;i<JOYHATS && i<JOYHATS_MAX;i++,j<<=1)
			if (k & j)          // test the eatch bit and post the corresponding event
			{
				if (joyhats & j)
					event.type=ev_keydown;
				else
					event.type=ev_keyup;
				event.data1=KEY_HAT1+i;
				D_PostEvent(&event);
			}
	}

	event.type=ev_joystick;
	s = 0;

	for (i = JOYAXISSET_MIN -1; i >=0;)
	{
		event.data1 = i;
		event.data2 = event.data3 = 0;
		if (joy[js].stick[s].flags &  JOYFLAG_DIGITAL)
		{
			if (joy[js].stick[s].axis[0].d1)
				event.data2=-1;
			if (joy[js].stick[s].axis[0].d2)
				event.data2=1;
			if (joy[js].stick[s].axis[1].d1)
				event.data3=-1;
			if (joy[js].stick[s].axis[1].d2)
				event.data3=1;
			D_PostEvent(&event);
			i++;
		}
		else if (joy[js].stick[s].flags &  JOYFLAG_ANALOGUE)
		{
			event.data2 = joy[js].stick[s].axis[0].pos*32;
			event.data3 = joy[js].stick[s].axis[1].pos*32;
			D_PostEvent(&event);
			i++;
		}
		if (s == JOYAXISSET_MAX*2) i = -1;
		s++;
	}
}

void I_GetJoystick2Events(void)
{
	event_t event;
	INT64 joybuttons= 0;
	INT64 joyhats = 0;
	int s = 0, i;
	int js = cv_usejoystick2.value - 1;

	if (!joystick2_detected)
		return;

	// I assume that true is 1
	for (i = JOYBUTTONS_MIN - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (joy[js].button[i].b)
			joybuttons |= 1;
	}

	for (i = JOYHATS_MIN -1; i >=0;)
	{
		if (joy[js].stick[s].flags &  JOYFLAG_DIGITAL)
		{
			if (joy[js].stick[s].axis[1].d1) joyhats |= 1<<(0 + 4*i);
			if (joy[js].stick[s].axis[1].d2) joyhats |= 1<<(1 + 4*i);
			if (joy[js].stick[s].axis[0].d1) joyhats |= 1<<(2 + 4*i);
			if (joy[js].stick[s].axis[0].d2) joyhats |= 1<<(3 + 4*i);
			i--;
		}
		if (s == JOYHATS_MAX) i = -1;
		s++;
	}

	// post key event for buttons
	if (joybuttons!=lastjoy2buttons)
	{
		INT64 j = 1; // only changed bit to 1
		INT64 k = (joybuttons ^ lastjoy2buttons);
		lastjoy2buttons=joybuttons;

		for (i=0;i<JOYBUTTONS && i<JOYBUTTONS_MAX;i++,j<<=1)
			if (k & j)          // test the eatch bit and post the corresponding event
			{
				if (joybuttons & j)
					event.type=ev_keydown;
				else
					event.type=ev_keyup;
				event.data1=KEY_2JOY1+i;
				D_PostEvent(&event);
			}
	}

	// post key event for hats
	if (joyhats!=lastjoy2hats)
	{
		INT64 j=1; // only changed bit to 1
		INT64 k = (joyhats ^ lastjoy2hats);
		lastjoy2hats=joyhats;

		for (i=0;i<JOYHATS && i<JOYHATS_MAX;i++,j<<=1)
			if (k & j)          // test the eatch bit and post the corresponding event
			{
				if (joyhats & j)
					event.type=ev_keydown;
				else
					event.type=ev_keyup;
				event.data1=KEY_2HAT1+i;
				D_PostEvent(&event);
			}
	}

	event.type=ev_joystick2;
	s = 0;

	for (i = JOYAXISSET_MIN - 1; i >=0;)
	{
		event.data1 = i;
		event.data2 = event.data3 = 0;
		if (joy[js].stick[s].flags &  JOYFLAG_DIGITAL)
		{
			if (joy[js].stick[s].axis[0].d1)
				event.data2=-1;
			if (joy[js].stick[s].axis[0].d2)
				event.data2=1;
			if (joy[js].stick[s].axis[1].d1)
				event.data3=-1;
			if (joy[js].stick[s].axis[1].d2)
				event.data3=1;
			D_PostEvent(&event);
			i++;
		}
		else if (joy[js].stick[s].flags &  JOYFLAG_ANALOGUE)
		{
			event.data2 = joy[js].stick[s].axis[0].pos*32;
			event.data3 = joy[js].stick[s].axis[1].pos*32;
			D_PostEvent(&event);
			i++;
		}
		if (s == JOYAXISSET_MAX*2) i = -1;
		s++;
	}
}

void I_GetMouseEvents(void)
{
	//mouse movement
	event_t event;
	int xmickeys,ymickeys,buttons,wheels = 0;
	static int lastbuttons=0;
	__dpmi_regs r;

	if (!mouse_detected)
		return;

	r.x.ax=0x0b;           // ask the mouvement not the position
	__dpmi_int(0x33,&r);
	xmickeys=(INT16)r.x.cx;
	ymickeys=(INT16)r.x.dx;
	r.x.ax=0x03;
	__dpmi_int(0x33,&r);
	buttons=r.h.bl;
	if (wheel_detected)
		wheels=(signed char)r.h.bh;

	// post key event for buttons
	if (buttons!=lastbuttons)
	{
		int j=1,k,i;
		k=(buttons ^ lastbuttons); // only changed bit to 1
		lastbuttons=buttons;

		for (i=0;i<MOUSEBUTTONS;i++,j<<=1)
			if (k & j)
			{
				if (buttons & j)
					event.type=ev_keydown;
				else
					event.type=ev_keyup;
				event.data1=KEY_MOUSE1+i;
				D_PostEvent(&event);
			}
	}

	event.type=ev_keyup;
	event.data1 = 0;
	if (wheels > 0)
		event.data1 = KEY_MOUSEWHEELUP;
	else if (wheels < 0)
		event.data1 = KEY_MOUSEWHEELDOWN;
	if (event.data1)
			D_PostEvent(&event);


	if ((xmickeys!=0)||(ymickeys!=0))
	{
		event.type=ev_mouse;
		event.data1=0;
		//event.data1=buttons;    // not needed
		event.data2=xmickeys;
		event.data3=-ymickeys;

		D_PostEvent(&event);
	}

	//reset wheel like in win32, I don't understand it but works
	gamekeydown[KEY_MOUSEWHEELDOWN] = gamekeydown[KEY_MOUSEWHEELUP] = 0;

}

void I_GetEvent (void)
{
#ifdef MOUSE2
	// mouse may be disabled during the game by setting usemouse false
	if (mouse2_started)
	{
		event_t event;
		//mouse movement
		static UINT8 lastbuttons2=0;

		// post key event for buttons
		if (handlermouse2buttons!=lastbuttons2)
		{
			int j=1,k,i;
			k=(handlermouse2buttons ^ lastbuttons2); // only changed bit to 1
			lastbuttons2=handlermouse2buttons;

			for (i=0;i<MOUSEBUTTONS;i++,j<<=1)
				if (k & j)
				{
					if (handlermouse2buttons & j)
						event.type=ev_keydown;
					else
						event.type=ev_keyup;
					event.data1=KEY_2MOUSE1+i;
					D_PostEvent(&event);
				}
		}

		if ((handlermouse2x!=0)||(handlermouse2y!=0))
		{
			event.type=ev_mouse2;
			event.data1=0;
			//event.data1=buttons;    // not needed
			event.data2=handlermouse2x<<1;
			event.data3=-handlermouse2y<<1;

			D_PostEvent(&event);
			handlermouse2x=0;
			handlermouse2y=0;
		}

	}
#endif

	//mouse
	I_GetMouseEvents();

	//joystick
	if (joystick_detected || joystick2_detected)
		poll_joystick();

	//joystick1
	I_GetJoystickEvents();

	//joystick2
	I_GetJoystick2Events();
}

INT32 I_NumJoys(void)
{
	return MAX_JOYSTICKS;
}

const char *I_GetJoyName(INT32 joyindex)
{
#if MAX_JOYSTICKS > 8
"More Joystick Names?"
#endif

	     if (joyindex == 1) return "Joystick A";
	else if (joyindex == 2) return "Joystick B";
	else if (joyindex == 3) return "Joystick C";
	else if (joyindex == 4) return "Joystick D";
	else if (joyindex == 5) return "Joystick E";
	else if (joyindex == 6) return "Joystick F";
	else if (joyindex == 7) return "Joystick G";
	else if (joyindex == 8) return "Joystick H";
	else return NULL;

}

//
//  Timer user routine called at ticrate.
//
static void I_TimerISR (void)
{
	//  IO_PlayerInput();      // old doom did that
	ticcount++;

}
#ifndef DOXYGEN
static END_OF_FUNCTION(I_TimerISR);
#endif


//added:08-01-98: we don't use allegro_exit() so we have to do it ourselves.
static inline void I_ShutdownTimer (void)
{
	if ( !timer_started )
		return;
	remove_timer();
}


//
//  Installs the timer interrupt handler with timer speed as NEWTICRATE.
//
void I_StartupTimer(void)
{
	ticcount = 0;

	//lock this from being swapped to disk! BEFORE INSTALLING
	LOCK_VARIABLE(ticcount);
	LOCK_FUNCTION(I_TimerISR);

	if ( install_timer() != 0 )
		I_Error("I_StartupTimer: could not install timer.");

	if ( install_int_ex( I_TimerISR, BPS_TO_TIMER(NEWTICRATE) ) != 0 )
		//should never happen since we use only one.
		I_Error("I_StartupTimer: no room for callback routine.");

	//added:08-01-98: remove the timer explicitly because we don't use
	//                Allegro 's allegro_exit() shutdown code.
	I_AddExitFunc(I_ShutdownTimer);
	timer_started = true;
}


//added:07-02-98:
//
//
static UINT8 ASCIINames[128] =
{
//        0           1              2            3
//        4           5              6            7
//        8           9              A            B
//        C           D              E            F
          0,         27,           '1',         '2',
        '3',        '4',           '5',         '6',
        '7',        '8',           '9',         '0',
  KEY_MINUS, KEY_EQUALS, KEY_BACKSPACE,     KEY_TAB,
        'q',        'w',           'e',        'r',
        't',        'y',           'u',        'i',
        'o',        'p',           '[',        ']',
  KEY_ENTER,  KEY_LCTRL,           'a',        's',
        'd',        'f',           'g',        'h',
        'j',        'k',           'l',        ';',
       '\'',        '`',    KEY_LSHIFT,       '\\',
        'z',        'x',           'c',        'v',
        'b',        'n',           'm',        ',',
        '.',        '/',    KEY_RSHIFT,        '*',
   KEY_LALT,  KEY_SPACE,  KEY_CAPSLOCK,     KEY_F1,
     KEY_F2,     KEY_F3,        KEY_F4,     KEY_F5,
     KEY_F6,     KEY_F7,        KEY_F8,     KEY_F9,
    KEY_F10,KEY_NUMLOCK,KEY_SCROLLLOCK,KEY_KEYPAD7,
KEY_KEYPAD8,KEY_KEYPAD9,  KEY_MINUSPAD,KEY_KEYPAD4,
KEY_KEYPAD5,KEY_KEYPAD6,   KEY_PLUSPAD,KEY_KEYPAD1,
KEY_KEYPAD2,KEY_KEYPAD3,   KEY_KEYPAD0,KEY_KPADDEL,
          0,          0,             0,    KEY_F11,
    KEY_F12,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0,
          0,          0,             0,          0
};

static volatile int pausepressed=0;
static volatile char nextkeyextended;

static void I_KeyboardHandler(void)
{
	UINT8 ch;
	event_t       event;

	ch=inportb(0x60);

	if (pausepressed>0)
		pausepressed--;
	else if (ch==0xE1) // pause key
	{
		event.type=ev_keydown;
		event.data1=KEY_PAUSE;
		D_PostEvent(&event);
		pausepressed=5;
	}
	else if (ch==0xE0) // extended key handled at next call
	{
		nextkeyextended=1;
	}
	else
	{
		if ((ch&0x80)==0)
			event.type=ev_keydown;
		else
			event.type=ev_keyup;

		ch&=0x7f;

		if (nextkeyextended)
		{
			nextkeyextended=0;

			if (ch==70)  // crtl-break
			{
				asm ("movb $0x79, %%al\ncall ___djgpp_hw_exception"
				     : : :"%eax","%ebx","%ecx","%edx","%esi","%edi","memory");
			}

			// remap lonely keypad slash
			if (ch==53)
				event.data1 = KEY_KPADSLASH;
			else if (ch>=91 && ch<=93) // remap the bill gates keys...
				event.data1 = ch + 0x80;    // leftwin, rightwin, menu
			else if (ch>=71 && ch<=83) // remap non-keypad extended keys to a value<128, but
				event.data1 = 0x80 + ch + 30; // make them different than the KEYPAD keys.
			else if (ch==28)
				event.data1 = KEY_ENTER;    // keypad enter -> return key
			else if (ch==29)
				event.data1 = KEY_RCTRL;     // rctrl -> lctrl
			else if (ch==56)
				event.data1 = KEY_RALT;      // ralt -> lalt
			else
				ch = 0;
			if (ch)
				D_PostEvent(&event);
		}
		else
		{
			if (ASCIINames[ch]!=0)
				event.data1=ASCIINames[ch];
			else
				event.data1=ch+0x80;
			D_PostEvent(&event);
		}
	}

	outportb(0x20,0x20);
}
#ifndef DOXYGEN
static END_OF_FUNCTION(I_KeyboardHandler);
#endif

//  Return a key that has been pushed, or 0
//  (replace getchar() at game startup)
//
INT32 I_GetKey (void)
{
	int rc=0;
	if ( keyboard_started )
	{
		event_t   *ev;

		// return the first keypress from the event queue
		for ( ; eventtail != eventhead ; eventtail = (eventtail+1)&(MAXEVENTS-1) )
		{
			ev = &events[eventtail];
			if (ev->type == ev_keydown || ev->type == ev_console)
			{
				rc = ev->data1;
				continue;
			}
		}
		return rc;
	}

	// keyboard not started use the bios call trouth djgpp
	if (_conio_kbhit())
	{
		rc=getch();
		if (rc==0) rc=getch()+256;
	}
	else
		rc = 0;

	return rc;
}

/* Keyboard handler stuff */
static _go32_dpmi_seginfo oldkeyinfo,newkeyinfo;

//
//  Removes the keyboard handler.
//
static inline void I_ShutdownKeyboard(void)
{
	if ( !keyboard_started )
		return;

	asm("cli");
	_go32_dpmi_set_protected_mode_interrupt_vector(9, &oldkeyinfo);
	_go32_dpmi_free_iret_wrapper(&newkeyinfo);
	asm("sti");

	keyboard_started=false;
}

//
//  Installs the keyboard handler.
//
void I_StartupKeyboard(void)
{
	if (keyboard_started)
		return;

	nextkeyextended=0;

	asm("cli");
	_go32_dpmi_get_protected_mode_interrupt_vector(9, &oldkeyinfo);
	newkeyinfo.pm_offset=(int)I_KeyboardHandler;
	newkeyinfo.pm_selector=_go32_my_cs();
	_go32_dpmi_allocate_iret_wrapper(&newkeyinfo);
	_go32_dpmi_set_protected_mode_interrupt_vector(9, &newkeyinfo);

	LOCK_VARIABLE(nextkeyextended);
	LOCK_VARIABLE(pausepressed);
	_go32_dpmi_lock_data(ASCIINames,sizeof (ASCIINames));
	LOCK_FUNCTION(I_KeyboardHandler);

	_go32_dpmi_lock_data(events,sizeof (events));
	LOCK_VARIABLE(eventhead);
	LOCK_FUNCTION(D_PostEvent);

	asm("sti");

	//added:08-01-98:register shutdown keyboard code.
	I_AddExitFunc(I_ShutdownKeyboard);
	keyboard_started = true;
}




//added:08-01-98:
//
//  Clean Startup & Shutdown handling, as does Allegro.
//  We need this services for ourselves too, and we don't want to mix
//  with Allegro, because someone might not use Allegro.
//  (all 'exit' was renamed to 'quit')
//
#define MAX_QUIT_FUNCS     16
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS] =
			{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


//added:08-01-98:
//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	int c;

	for (c=0; c<MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}


//added:08-01-98:
//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	int c;

	for (c=0; c<MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c<MAX_QUIT_FUNCS-1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}



 //added:03-01-98:
//
// signal_handler:
//  Used to trap various signals, to make sure things get shut down cleanly.
//
static inline void exception_handler(int num)
{
	static char msg[255];
	sprintf(msg,"Sonic Robo Blast 2 "VERSIONSTRING"\r\n"
	        "This is a error of SRB2, try to send the following info to programmers\r\n");

	//D_QuitNetGame ();  //say 'byebye' to other players when your machine
						// crashes?... hmm... do they have to die with you???

	I_ShutdownSystem();

	_write(STDERR_FILENO, msg, strlen(msg));

	signal(num, SIG_DFL);
	raise(num);
	/// \todo write it in a log !!
}

static inline void break_handler(int num)
{
static char msg[] = "Oh no! Back to reality!\r\n";

	//D_QuitNetGame ();  //say 'byebye' to other players when your machine
						// crashes?... hmm... do they have to die with you???

	I_ShutdownSystem();

	_write(STDERR_FILENO, msg, sizeof (msg)-1);

	signal(num, SIG_DFL);
	raise(num);
}


//added:08-01-98: now this replaces allegro_init()
//
// REMEMBER: THIS ROUTINE MUST BE STARTED IN i_main.c BEFORE D_SRB2Main()
//
// This stuff should get rid of the exception and page faults when
// SRB2 bugs out with an error. Now it should exit cleanly.
//
INT32 I_StartupSystem(void)
{
	I_DetectOS();
	check_cpu();

	// some 'more globals than globals' things to initialize here ?
	graphics_started = false;
	keyboard_started = false;
	sound_started = false;
	timer_started = false;
	cdaudio_started = false;

	// check for OS type and version here ?


	signal(SIGABRT, exception_handler);
	signal(SIGFPE , exception_handler);
	signal(SIGILL , exception_handler);
	signal(SIGSEGV, exception_handler);
	signal(SIGINT , break_handler);
	signal(SIGKILL, break_handler);
	signal(SIGQUIT, break_handler);

	return 0;
}


//added:08-01-98:
//
//  Closes down everything. This includes restoring the initial
//  pallete and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE : Shutdown user funcs. are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
	int c;

	for (c=MAX_QUIT_FUNCS-1; c>=0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
}

void I_GetDiskFreeSpace(INT64 *freespace)
{
	struct diskfree_t df;
	if (_dos_getdiskfree(0,&df))
		*freespace = (unsigned long)df.avail_clusters *
		             (unsigned long)df.bytes_per_sector *
		             (unsigned long)df.sectors_per_cluster;
	else
		*freespace = INT32_MAX;
}

char *I_GetUserName(void)
{
	static char username[MAXPLAYERNAME];
	char  *p;
	if ((p=getenv("USER"))==NULL)
		if ((p=getenv("user"))==NULL)
			if ((p=getenv("USERNAME"))==NULL)
				if ((p=getenv("username"))==NULL)
					return NULL;
	strncpy(username,p,MAXPLAYERNAME);

	if (strcmp(username,"")==0 )
		return NULL;
	return username;
}

INT32 I_mkdir(const char *pdirname, INT32 unixright)
{
	return mkdir(pdirname, unixright);
}

char * I_GetEnv(const char *name)
{
	return getenv(name);
}

INT32 I_PutEnv(char *variable)
{
	return putenv(variable);
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

char *I_ClipboardPaste(void)
{
	return NULL;
}

const CPUInfoFlags *I_CPUInfo(void)
{
	static CPUInfoFlags DOS_CPUInfo;
	memset(&DOS_CPUInfo,0,sizeof (DOS_CPUInfo));
#if ALLEGRO_VERSION == 3
	if (!cpu_cpuid) return NULL;
	DOS_CPUInfo.CPUID       = true;
	DOS_CPUInfo.MMX         = cpu_mmx;
	DOS_CPUInfo.AMD3DNow    = cpu_3dnow;
#else
	DOS_CPUInfo.CPUID       = ((cpu_capabilities&CPU_ID)       ==       CPU_ID);
	DOS_CPUInfo.FPU         = ((cpu_capabilities&CPU_FPU)      ==      CPU_FPU);
#ifdef CPU_IA64
	DOS_CPUInfo.IA64        = ((cpu_capabilities&CPU_IA64)     ==     CPU_IA64);
#endif
#ifdef CPU_AMD64
	DOS_CPUInfo.AMD64       = ((cpu_capabilities&CPU_AMD64)    ==    CPU_AMD64);
#endif
	DOS_CPUInfo.MMX         = ((cpu_capabilities&CPU_MMX)      ==      CPU_MMX);
	DOS_CPUInfo.MMXExt      = ((cpu_capabilities&CPU_MMXPLUS)  ==  CPU_MMXPLUS);
	DOS_CPUInfo.SSE         = ((cpu_capabilities&CPU_SSE)      ==      CPU_SSE);
	DOS_CPUInfo.SSE2        = ((cpu_capabilities&CPU_SSE2)     ==     CPU_SSE2);
#ifdef CPU_SEE3
	DOS_CPUInfo.SSE3        = ((cpu_capabilities&CPU_SSE3)     ==     CPU_SSE3);
#endif
	DOS_CPUInfo.AMD3DNow    = ((cpu_capabilities&CPU_3DNOW)    ==    CPU_3DNOW);
	DOS_CPUInfo.AMD3DNowExt = ((cpu_capabilities&CPU_ENH3DNOW) == CPU_ENH3DNOW);
	DOS_CPUInfo.CMOV        = ((cpu_capabilities&CPU_CMOV)     ==     CPU_CMOV);
#endif
	return &DOS_CPUInfo;
}

void I_RegisterSysCommands(void) {}
