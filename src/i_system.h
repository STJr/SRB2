// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_system.h
/// \brief System specific interface stuff.

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "d_ticcmd.h"
#include "d_event.h"

#ifdef __GNUG__
#pragma interface
#endif

/**	\brief max quit functions
*/
#define MAX_QUIT_FUNCS     16


/**	\brief Graphic system had started up
*/
extern UINT8 graphics_started;

/**	\brief Keyboard system is up and run
*/
extern UINT8 keyboard_started;

/**	\brief	The I_GetFreeMem function

	\param	total	total memory in the system

	\return	free memory in the system
*/
UINT32 I_GetFreeMem(UINT32 *total);

/**	\brief  Called by D_SRB2Loop, returns current time in tics.
*/
tic_t I_GetTime(void);

int I_GetTimeMicros(void);// provides microsecond counter for render stats

/**	\brief	The I_Sleep function

	\return	void
*/
void I_Sleep(void);

/**	\brief Get events

	Called by D_SRB2Loop,
	called before processing each tic in a frame.
	Quick syncronous operations are performed here.
	Can call D_PostEvent.
*/
void I_GetEvent(void);

/**	\brief Asynchronous interrupt functions should maintain private queues
	that are read by the synchronous functions
	to be converted into events.
*/
void I_OsPolling(void);

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.

/**	\brief Input for the first player
*/
ticcmd_t *I_BaseTiccmd(void);

/**	\brief Input for the sencond player
*/
ticcmd_t *I_BaseTiccmd2(void);

/**	\brief Called by M_Responder when quit is selected, return exit code 0
*/
void I_Quit(void) FUNCNORETURN;

typedef enum
{
	EvilForce = -1,
	//Constant
	ConstantForce = 0,
	//Ramp
	RampForce,
	//Periodics
	SquareForce,
	SineForce,
	TriangleForce,
	SawtoothUpForce,
	SawtoothDownForce,
	//MAX
	NumberofForces,
} FFType;

typedef struct JoyFF_s
{
	INT32 ForceX; ///< The X of the Force's Vel
	INT32 ForceY; ///< The Y of the Force's Vel
	//All
	UINT32 Duration; ///< The total duration of the effect, in microseconds
	INT32 Gain; //< /The gain to be applied to the effect, in the range from 0 through 10,000.
	//All, CONSTANTFORCE -10,000 to 10,000
	INT32 Magnitude; ///< Magnitude of the effect, in the range from 0 through 10,000.
	//RAMPFORCE
	INT32 Start; ///< Magnitude at the start of the effect, in the range from -10,000 through 10,000.
	INT32 End; ///< Magnitude at the end of the effect, in the range from -10,000 through 10,000.
	//PERIODIC
	INT32 Offset; ///< Offset of the effect.
	UINT32 Phase; ///< Position in the cycle of the periodic effect at which playback begins, in the range from 0 through 35,999
	UINT32 Period; ///< Period of the effect, in microseconds.
} JoyFF_t;

/**	\brief	Forcefeedback for the first joystick

	\param	Type   what kind of Effect
	\param	Effect Effect Info

	\return	void
*/

void I_Tactile(FFType Type, const JoyFF_t *Effect);

/**	\brief	Forcefeedback for the second joystick

	\param	Type   what kind of Effect
	\param	Effect Effect Info

	\return	void
*/
void I_Tactile2(FFType Type, const JoyFF_t *Effect);

/**	\brief to set up the first joystick scale
*/
void I_JoyScale(void);

/**	\brief to set up the second joystick scale
*/
void I_JoyScale2(void);

// Called by D_SRB2Main.

/**	\brief to startup the first joystick
*/
void I_InitJoystick(void);

/**	\brief to startup the second joystick
*/
void I_InitJoystick2(void);

/**	\brief return the number of joystick on the system
*/
INT32 I_NumJoys(void);

/**	\brief	The *I_GetJoyName function

	\param	joyindex	which joystick

	\return	joystick name
*/
const char *I_GetJoyName(INT32 joyindex);

#ifndef NOMUMBLE
#include "p_mobj.h" // mobj_t
#include "s_sound.h" // listener_t
/** \brief to update Mumble of Player Postion
*/
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener);
#endif

/**	\brief Startup the first mouse
*/
void I_StartupMouse(void);

/**	\brief Startup the second mouse
*/
void I_StartupMouse2(void);

/**	\brief  setup timer irq and user timer routine.
*/
void I_StartupTimer(void);

/**	\brief sample quit function
*/
typedef void (*quitfuncptr)();

/**	\brief	add a list of functions to call at program cleanup

	\param	(*func)()	funcction to call at program cleanup

	\return	void
*/
void I_AddExitFunc(void (*func)());

/**	\brief	The I_RemoveExitFunc function

	\param	(*func)()	function to remove from the list

	\return	void
*/
void I_RemoveExitFunc(void (*func)());

/**	\brief Setup signal handler, plus stuff for trapping errors and cleanly exit.
*/
INT32 I_StartupSystem(void);

/**	\brief Shutdown systems
*/
void I_ShutdownSystem(void);

/**	\brief	The I_GetDiskFreeSpace function

	\param	freespace	a INT64 pointer to hold the free space amount

	\return	void
*/
void I_GetDiskFreeSpace(INT64 *freespace);

/**	\brief find out the user's name
*/
char *I_GetUserName(void);

/**	\brief	The I_mkdir function

	\param	dirname	string of mkidr
	\param	unixright	unix right

	\return status of new folder
*/
INT32 I_mkdir(const char *dirname, INT32 unixright);

typedef struct {
	int FPU        : 1; ///< FPU availabile
	int CPUID      : 1; ///< CPUID instruction
	int RDTSC      : 1; ///< RDTSC instruction
	int MMX        : 1; ///< MMX features
	int MMXExt     : 1; ///< MMX Ext. features
	int CMOV       : 1; ///< Pentium Pro's "cmov"
	int AMD3DNow   : 1; ///< 3DNow features
	int AMD3DNowExt: 1; ///< 3DNow! Ext. features
	int SSE        : 1; ///< SSE features
	int SSE2       : 1; ///< SSE2 features
	int SSE3       : 1; ///< SSE3 features
	int IA64       : 1; ///< Running on IA64
	int AMD64      : 1; ///< Running on AMD64
	int AltiVec    : 1; ///< AltiVec features
	int FPPE       : 1; ///< floating-point precision error
	int PFC        : 1; ///< TBD?
	int cmpxchg    : 1; ///< ?
	int cmpxchg16b : 1; ///< ?
	int cmp8xchg16 : 1; ///< ?
	int FPE        : 1; ///< FPU Emu
	int DEP        : 1; ///< Data excution prevent
	int PPCMM64    : 1; ///< PowerPC Movemem 64bit ok?
	int ALPHAbyte  : 1; ///< ?
	int PAE        : 1; ///< Physical Address Extension
	int CPUs       : 8;
} CPUInfoFlags;


/**	\brief Info about CPU
		\return CPUInfo in bits
*/
const CPUInfoFlags *I_CPUInfo(void);

/**	\brief Find main WAD
		\return path to main WAD
*/
const char *I_LocateWad(void);

/**	\brief First Joystick's events
*/
void I_GetJoystickEvents(void);

/**	\brief Second Joystick's events
*/
void I_GetJoystick2Events(void);

/**	\brief Mouses events
*/
void I_GetMouseEvents(void);

/**	\brief Checks if the mouse needs to be grabbed
*/
void I_UpdateMouseGrab(void);

char *I_GetEnv(const char *name);

INT32 I_PutEnv(char *variable);

/** \brief Put data in system clipboard
*/
INT32 I_ClipboardCopy(const char *data, size_t size);

/** \brief Retrieve data from system clipboard
*/
const char *I_ClipboardPaste(void);

void I_RegisterSysCommands(void);

#endif
