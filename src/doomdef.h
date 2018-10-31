// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  doomdef.h
/// \brief Internally used data structures for virtually everything,
///        key definitions, lots of other stuff.

#ifndef __DOOMDEF__
#define __DOOMDEF__

// Sound system select
// This should actually be in the makefile,
// but I can't stand that gibberish. D:
#define SOUND_DUMMY   0
#define SOUND_SDL     1
#define SOUND_MIXER   2
#define SOUND_FMOD    3

#ifndef SOUND
#ifdef HAVE_SDL

// Use Mixer interface?
#ifdef HAVE_MIXER
    //#if !defined(DC) && !defined(_WIN32_WCE) && !defined(_XBOX) && !defined(GP2X)
    #define SOUND SOUND_MIXER
    #define NOHS // No HW3SOUND
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
    //#endif
#endif

// Use generic SDL interface.
#ifndef SOUND
#define SOUND SOUND_SDL
#endif

#else // No SDL.

// Use FMOD?
#ifdef HAVE_FMOD
    #define SOUND SOUND_FMOD
    #define NOHS // No HW3SOUND
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
#else
    // No more interfaces. :(
    #define SOUND SOUND_DUMMY
#endif

#endif
#endif

#ifdef _WINDOWS
#define NONET
#if !defined (HWRENDER) && !defined (NOHW)
#define HWRENDER
#endif
// judgecutor: 3D sound support
#if !defined(HW3SOUND) && !defined (NOHS)
#define HW3SOUND
#endif
#endif

#if defined (_WIN32) || defined (_WIN32_WCE)
#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4127 4152 4213 4514)
#ifdef _WIN64
#pragma warning(disable : 4306)
#endif
#endif
// warning level 4
// warning C4127: conditional expression is constant
// warning C4152: nonstandard extension, function/data pointer conversion in expression
// warning C4213: nonstandard extension used : cast on l-value

#if defined (_WIN32_WCE) && defined (DEBUG) && defined (ARM)
#if defined (ARMV4) || defined (ARMV4I)
//#pragma warning(disable : 1166)
// warning LNK1166: cannot adjust code at offset=
#endif
#endif


#include "doomtype.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES // fixes M_PI errors in r_plane.c for Visual Studio
#include <math.h>

#ifdef GETTEXT
#include <libintl.h>
#include <locale.h>
#endif

#if !defined (_WIN32_WCE)
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <ctype.h>

#if ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (__DJGPP__)) && !defined (_XBOX)
#include <io.h>
#endif

#ifdef PC_DOS
#include <conio.h>
#endif

//#define NOMD5

// Uncheck this to compile debugging code
//#define RANGECHECK
//#ifndef PARANOIA
//#define PARANOIA // do some tests that never fail but maybe
// turn this on by make etc.. DEBUGMODE = 1 or use the Debug profile in the VC++ projects
//#endif
#if defined (_WIN32) || (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (macintosh)
#define LOGMESSAGES // write message in log.txt
#endif

#ifdef LOGMESSAGES
extern FILE *logstream;
#endif

//#define DEVELOP // Disable this for release builds to remove excessive cheat commands and enable MD5 checking and stuff, all in one go. :3
#ifdef DEVELOP
#define VERSION    0 // Game version
#define SUBVERSION 0 // more precise version number
#define VERSIONSTRING "Development EXE"
#define VERSIONSTRINGW L"Development EXE"
// most interface strings are ignored in development mode.
// we use comprevision and compbranch instead.
#else
#define VERSION    201 // Game version
#define SUBVERSION 20  // more precise version number
#define VERSIONSTRING "v2.1.20"
#define VERSIONSTRINGW L"v2.1.20"
// Hey! If you change this, add 1 to the MODVERSION below!
// Otherwise we can't force updates!
#endif

// Does this version require an added patch file?
// Comment or uncomment this as necessary.
#define USE_PATCH_DTA

// Modification options
// If you want to take advantage of the Master Server's ability to force clients to update
// to the latest version, fill these out.  Otherwise, just comment out UPDATE_ALERT and leave
// the other options the same.

// Comment out this line to completely disable update alerts (recommended for testing, but not for release)
#define UPDATE_ALERT

// The string used in the alert that pops up in the event of an update being available.
// Please change to apply to your modification (we don't want everyone asking where your mod is on SRB2.org!).
#define UPDATE_ALERT_STRING \
"A new update is available for SRB2.\n"\
"Please visit SRB2.org to download it.\n"\
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"\
"\n"\
"This update is required for online\n"\
"play using the Master Server.\n"\
"You will not be able to connect to\n"\
"the Master Server until you update to\n"\
"the newest version of the game.\n"\
"\n"\
"(Press a key)\n"

// The string used in the I_Error alert upon trying to host through command line parameters.
// Generally less filled with newlines, since Windows gives you lots more room to work with.
#define UPDATE_ALERT_STRING_CONSOLE \
"A new update is available for SRB2.\n"\
"Please visit SRB2.org to download it.\n"\
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"\
"\n"\
"This update is required for online play using the Master Server.\n"\
"You will not be able to connect to the Master Server\n"\
"until you update to the newest version of the game.\n"

// For future use, the codebase is the version of SRB2 that the modification is based on,
// and should not be changed unless you have merged changes between versions of SRB2
// (such as 2.0.4 to 2.0.5, etc) into your working copy.
// Will always resemble the versionstring, 205 = 2.0.5, 210 = 2.1, etc.
#define CODEBASE 210

// The Modification ID; must be obtained from Rob ( https://mb.srb2.org/private.php?do=newpm&u=546 ).
// DO NOT try to set this otherwise, or your modification will be unplayable through the Master Server.
// "12" is the default mod ID for version 2.1
#define MODID 12

// The Modification Version, starting from 1. Do not follow your version string for this,
// it's only for detection of the version the player is using so the MS can alert them of an update.
// Only set it higher, not lower, obviously.
// Note that we use this to help keep internal testing in check; this is why v2.1.0 is not version "1".
#define MODVERSION 25

// =========================================================================

// The maximum number of players, multiplayer/networking.
// NOTE: it needs more than this to increase the number of players...

#define MAXPLAYERS 32
#define MAXSKINS MAXPLAYERS
#define PLAYERSMASK (MAXPLAYERS-1)
#define MAXPLAYERNAME 21

typedef enum
{
	SKINCOLOR_NONE = 0,
	SKINCOLOR_WHITE,
	SKINCOLOR_SILVER,
	SKINCOLOR_GREY,
	SKINCOLOR_BLACK,
	SKINCOLOR_CYAN,
	SKINCOLOR_TEAL,
	SKINCOLOR_STEELBLUE,
	SKINCOLOR_BLUE,
	SKINCOLOR_PEACH,
	SKINCOLOR_TAN,
	SKINCOLOR_PINK,
	SKINCOLOR_LAVENDER,
	SKINCOLOR_PURPLE,
	SKINCOLOR_ORANGE,
	SKINCOLOR_ROSEWOOD,
	SKINCOLOR_BEIGE,
	SKINCOLOR_BROWN,
	SKINCOLOR_RED,
	SKINCOLOR_DARKRED,
	SKINCOLOR_NEONGREEN,
	SKINCOLOR_GREEN,
	SKINCOLOR_ZIM,
	SKINCOLOR_OLIVE,
	SKINCOLOR_YELLOW,
	SKINCOLOR_GOLD,

	// Careful! MAXSKINCOLORS cannot be greater than 0x20!
	MAXSKINCOLORS,

	// Super special awesome Super flashing colors!
	SKINCOLOR_SUPER1 = MAXSKINCOLORS,
	SKINCOLOR_SUPER2,
	SKINCOLOR_SUPER3,
	SKINCOLOR_SUPER4,
	SKINCOLOR_SUPER5,

	// Super Tails
	SKINCOLOR_TSUPER1,
	SKINCOLOR_TSUPER2,
	SKINCOLOR_TSUPER3,
	SKINCOLOR_TSUPER4,
	SKINCOLOR_TSUPER5,

	// Super Knuckles
	SKINCOLOR_KSUPER1,
	SKINCOLOR_KSUPER2,
	SKINCOLOR_KSUPER3,
	SKINCOLOR_KSUPER4,
	SKINCOLOR_KSUPER5,

	MAXTRANSLATIONS
} skincolors_t;

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define TICRATE 35
#define NEWTICRATERATIO 1 // try 4 for 140 fps :)
#define NEWTICRATE (TICRATE*NEWTICRATERATIO)

#define RING_DIST 512*FRACUNIT // how close you need to be to a ring to attract it

#define PUSHACCEL (2*FRACUNIT) // Acceleration for MF2_SLIDEPUSH items.

// Special linedef executor tag numbers!
enum {
	LE_PINCHPHASE      = -2, // A boss entered pinch phase (and, in most cases, is preparing their pinch phase attack!)
	LE_ALLBOSSESDEAD   = -3, // All bosses in the map are dead (Egg capsule raise)
	LE_BOSSDEAD        = -4, // A boss in the map died (Chaos mode boss tally)
	LE_BOSS4DROP       = -5,  // CEZ boss dropped its cage
	LE_BRAKVILEATACK   = -6  // Brak's doing his LOS attack, oh noes
};

// Name of local directory for config files and savegames
#if !defined(_arch_dreamcast) && !defined(_WIN32_WCE) && !defined(GP2X) && !defined(_WII) && !defined(_PS3)
#if (((defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON)) && !defined (__CYGWIN__)) && !defined (__APPLE__)
#define DEFAULTDIR ".srb2"
#else
#define DEFAULTDIR "srb2"
#endif
#endif

#include "g_state.h"

// commonly used routines - moved here for include convenience

/**	\brief	The I_Error function

	\param	error	the error message

	\return	void


*/
void I_Error(const char *error, ...) FUNCIERROR;

/**	\brief	write a message to stderr (use before I_Quit) for when you need to quit with a msg, but need
 the return code 0 of I_Quit();

	\param	error	message string

	\return	void
*/
void I_OutputMsg(const char *error, ...) FUNCPRINTF;

// console.h
typedef enum
{
	CONS_NOTICE,
	CONS_WARNING,
	CONS_ERROR
} alerttype_t;

void CONS_Printf(const char *fmt, ...) FUNCPRINTF;
void CONS_Alert(alerttype_t level, const char *fmt, ...) FUNCDEBUG;
void CONS_Debug(INT32 debugflags, const char *fmt, ...) FUNCDEBUG;

// For help debugging functions.
#define POTENTIALLYUNUSED CONS_Alert(CONS_WARNING, "(%s:%d) Unused code appears to be used.\n", __FILE__, __LINE__)

#include "m_swap.h"

// Things that used to be in dstrings.h
#define SAVEGAMENAME "srb2sav"
char savegamename[256];

// m_misc.h
#ifdef GETTEXT
#define M_GetText(String) gettext(String)
void M_StartupLocale(void);
#else
// If no translations are to be used, make a stub
// M_GetText function that just returns the string.
#define M_GetText(x) (x)
#endif
extern void *(*M_Memcpy)(void* dest, const void* src, size_t n) FUNCNONNULL;
char *va(const char *format, ...) FUNCPRINTF;
char *M_GetToken(const char *inputString);
char *sizeu1(size_t num);
char *sizeu2(size_t num);
char *sizeu3(size_t num);
char *sizeu4(size_t num);
char *sizeu5(size_t num);

// d_main.c
extern boolean devparm; // development mode (-debug)
// d_netcmd.c
extern INT32 cv_debug;

#define DBG_BASIC       0x0001
#define DBG_DETAILED    0x0002
#define DBG_RANDOMIZER  0x0004
#define DBG_RENDER      0x0008
#define DBG_NIGHTSBASIC 0x0010
#define DBG_NIGHTS      0x0020
#define DBG_POLYOBJ     0x0040
#define DBG_GAMELOGIC   0x0080
#define DBG_NETPLAY     0x0100
#define DBG_MEMORY      0x0200
#define DBG_SETUP       0x0400
#define DBG_LUA         0x0800

// =======================
// Misc stuff for later...
// =======================

// Modifier key variables, accessible anywhere
extern UINT8 shiftdown, ctrldown, altdown;

// if we ever make our alloc stuff...
#define ZZ_Alloc(x) Z_Malloc(x, PU_STATIC, NULL)

// i_system.c, replace getchar() once the keyboard has been appropriated
INT32 I_GetKey(void);

#ifndef min // Double-Check with WATTCP-32's cdefs.h
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max // Double-Check with WATTCP-32's cdefs.h
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

// An assert-type mechanism.
#ifdef PARANOIA
#define I_Assert(e) ((e) ? (void)0 : I_Error("assert failed: %s, file %s, line %d", #e, __FILE__, __LINE__))
#else
#define I_Assert(e) ((void)0)
#endif

// The character that separates pathnames. Forward slash on
// most systems, but reverse solidus (\) on Windows and DOS.
#if defined (PC_DOS) || defined (_WIN32)
	#define PATHSEP "\\"
#else
	#define PATHSEP "/"
#endif

// Compile date and time and revision.
extern const char *compdate, *comptime, *comprevision, *compbranch;

// Disabled code and code under testing
// None of these that are disabled in the normal build are guaranteed to work perfectly
// Compile them at your own risk!

/// Kalaron/Eternity Engine slope code (SRB2CB ported)
#define ESLOPE

#ifdef ESLOPE
/// Backwards compatibility with SRB2CB's slope linedef types.
///	\note	A simple shim that prints a warning.
#define ESLOPE_TYPESHIM
#endif

///	Delete file while the game is running.
///	\note	EXTREMELY buggy, tends to crash game.
//#define DELFILE

///	Allows the use of devmode in multiplayer. AKA "fishcake"
//#define NETGAME_DEVMODE

///	Allows gravity changes in netgames, no questions asked.
//#define NETGAME_GRAVITY

///	Dumps the contents of a network save game upon consistency failure for debugging.
//#define DUMPCONSISTENCY

///	Polyobject fake flat code
#define POLYOBJECTS_PLANES

///	Improved way of dealing with ping values and a ping limit.
#define NEWPING

///	See name of player in your crosshair
#define SEENAMES

///	Who put weights on my recycler?  ... Inuyasha did.
///	\note	XMOD port.
//#define WEIGHTEDRECYCLER

///	Allow loading of savegames between different versions of the game.
///	\note	XMOD port.
///	    	Most modifications should probably enable this.
//#define SAVEGAME_OTHERVERSIONS

#if !defined (_NDS) && !defined (_PSP)
///	Shuffle's incomplete OpenGL sorting code.
#define SHUFFLE // This has nothing to do with sorting, why was it disabled?
#endif

#if !defined (_NDS) && !defined (_PSP)
///	Allow the use of the SOC RESETINFO command.
///	\note	Builds that are tight on memory should disable this.
///	    	This stops the game from storing backups of the states, sprites, and mobjinfo tables.
///	    	Though this info is compressed under normal circumstances, it's still a lot of extra
///	    	memory that never gets touched.
#define ALLOW_RESETDATA
#endif

#ifndef NONET
///	Display a connection screen on join attempts.
#define CLIENT_LOADINGSCREEN
#endif

/// Experimental tweaks to analog mode. (Needs a lot of work before it's ready for primetime.)
//#define REDSANALOG

/// Backwards compatibility with musicslots.
/// \note	You should leave this enabled unless you're working with a future SRB2 version.
#define MUSICSLOT_COMPATIBILITY

/// Handle touching sector specials in P_PlayerAfterThink instead of P_PlayerThink.
/// \note   Required for proper collision with moving sloped surfaces that have sector specials on them.
//#define SECTORSPECIALSAFTERTHINK

#endif // __DOOMDEF__
