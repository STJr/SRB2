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
    #define SOUND SOUND_MIXER
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
#endif

// Use generic SDL interface.
#ifndef SOUND
#define SOUND SOUND_SDL
#endif

#else // No SDL.

// Use FMOD?
#ifdef HAVE_FMOD
    #define SOUND SOUND_FMOD
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
#endif

#ifdef _WIN32
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


#include "version.h"
#include "doomtype.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES // fixes M_PI errors in r_plane.c for Visual Studio
#include <math.h>

#ifdef GETTEXT
#include <libintl.h>
#endif
#include <locale.h> // locale should not be dependent on GETTEXT -- 11/01/20 Monster Iestyn

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#if defined (_WIN32) || defined (__DJGPP__)
#include <io.h>
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
extern char logfilename[1024];
#endif

/* A mod name to further distinguish versions. */
#define SRB2APPLICATION "SRB2"

//#define DEVELOP // Disable this for release builds to remove excessive cheat commands and enable MD5 checking and stuff, all in one go. :3
#ifdef DEVELOP
#define VERSIONSTRING "Development EXE"
// most interface strings are ignored in development mode.
// we use comprevision and compbranch instead.
// VERSIONSTRING_RC is for the resource-definition script used by windows builds
#else
#ifdef BETAVERSION
#define VERSIONSTRING "v"SRB2VERSION" "BETAVERSION
#define VERSIONSTRING_RC SRB2VERSION " " BETAVERSION "\0"
#else
#define VERSIONSTRING "v"SRB2VERSION
#define VERSIONSTRING_RC SRB2VERSION "\0"
#endif
// Hey! If you change this, add 1 to the MODVERSION below!
// Otherwise we can't force updates!
#endif

#define VERSIONSTRINGW WSTRING (VERSIONSTRING)

/* A custom URL protocol for server links. */
#define SERVER_URL_PROTOCOL "srb2://"

// Does this version require an added patch file?
// Comment or uncomment this as necessary.
#define USE_PATCH_DTA

// Use .kart extension addons
//#define USE_KART

// Modification options
// If you want to take advantage of the Master Server's ability to force clients to update
// to the latest version, fill these out.  Otherwise, just comment out UPDATE_ALERT and leave
// the other options the same.

// Comment out this line to completely disable update alerts (recommended for testing, but not for release)
#ifndef BETAVERSION
#define UPDATE_ALERT
#endif

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
#define CODEBASE 220

// To version config.cfg, MAJOREXECVERSION is set equal to MODVERSION automatically.
// Increment MINOREXECVERSION whenever a config change is needed that does not correspond
// to an increment in MODVERSION. This might never happen in practice.
// If MODVERSION increases, set MINOREXECVERSION to 0.
#define MAJOREXECVERSION MODVERSION
#define MINOREXECVERSION 0
// (It would have been nice to use VERSION and SUBVERSION but those are zero'd out for DEVELOP builds)

// Macros
#define GETMAJOREXECVERSION(v) (v & 0xFFFF)
#define GETMINOREXECVERSION(v) (v >> 16)
#define GETEXECVERSION(major,minor) (major + (minor << 16))
#define EXECVERSION GETEXECVERSION(MAJOREXECVERSION, MINOREXECVERSION)

// =========================================================================

// The maximum number of players, multiplayer/networking.
// NOTE: it needs more than this to increase the number of players...

#define MAXPLAYERS 32
#define MAXSKINS 32
#define PLAYERSMASK (MAXPLAYERS-1)
#define MAXPLAYERNAME 21

#define COLORRAMPSIZE 16
#define MAXCOLORNAME 32
#define NUMCOLORFREESLOTS 1024

typedef struct skincolor_s
{
	char name[MAXCOLORNAME+1];  // Skincolor name
	UINT8 ramp[COLORRAMPSIZE];  // Colormap ramp
	UINT16 invcolor;            // Signpost color
	UINT8 invshade;             // Signpost color shade
	UINT16 chatcolor;           // Chat color
	boolean accessible;         // Accessible by the color command + setup menu
} skincolor_t;

typedef enum
{
	SKINCOLOR_NONE = 0,

	// Greyscale ranges
	SKINCOLOR_WHITE,
	SKINCOLOR_BONE,
	SKINCOLOR_CLOUDY,
	SKINCOLOR_GREY,
	SKINCOLOR_SILVER,
	SKINCOLOR_CARBON,
	SKINCOLOR_JET,
	SKINCOLOR_BLACK,

	// Desaturated
	SKINCOLOR_AETHER,
	SKINCOLOR_SLATE,
	SKINCOLOR_BLUEBELL,
	SKINCOLOR_PINK,
	SKINCOLOR_YOGURT,
	SKINCOLOR_BROWN,
	SKINCOLOR_BRONZE,
	SKINCOLOR_TAN,
	SKINCOLOR_BEIGE,
	SKINCOLOR_MOSS,
	SKINCOLOR_AZURE,
	SKINCOLOR_LAVENDER,

	// Viv's vivid colours (toast 21/07/17)
	SKINCOLOR_RUBY,
	SKINCOLOR_SALMON,
	SKINCOLOR_RED,
	SKINCOLOR_CRIMSON,
	SKINCOLOR_FLAME,
	SKINCOLOR_KETCHUP,
	SKINCOLOR_PEACHY,
	SKINCOLOR_QUAIL,
	SKINCOLOR_SUNSET,
	SKINCOLOR_COPPER,
	SKINCOLOR_APRICOT,
	SKINCOLOR_ORANGE,
	SKINCOLOR_RUST,
	SKINCOLOR_GOLD,
	SKINCOLOR_SANDY,
	SKINCOLOR_YELLOW,
	SKINCOLOR_OLIVE,
	SKINCOLOR_LIME,
	SKINCOLOR_PERIDOT,
	SKINCOLOR_APPLE,
	SKINCOLOR_GREEN,
	SKINCOLOR_FOREST,
	SKINCOLOR_EMERALD,
	SKINCOLOR_MINT,
	SKINCOLOR_SEAFOAM,
	SKINCOLOR_AQUA,
	SKINCOLOR_TEAL,
	SKINCOLOR_WAVE,
	SKINCOLOR_CYAN,
	SKINCOLOR_SKY,
	SKINCOLOR_CERULEAN,
	SKINCOLOR_ICY,
	SKINCOLOR_SAPPHIRE, // sweet mother, i cannot weave – slender aphrodite has overcome me with longing for a girl
	SKINCOLOR_CORNFLOWER,
	SKINCOLOR_BLUE,
	SKINCOLOR_COBALT,
	SKINCOLOR_VAPOR,
	SKINCOLOR_DUSK,
	SKINCOLOR_PASTEL,
	SKINCOLOR_PURPLE,
	SKINCOLOR_BUBBLEGUM,
	SKINCOLOR_MAGENTA,
	SKINCOLOR_NEON,
	SKINCOLOR_VIOLET,
	SKINCOLOR_LILAC,
	SKINCOLOR_PLUM,
	SKINCOLOR_RASPBERRY,
	SKINCOLOR_ROSY,

	FIRSTSUPERCOLOR,

	// Super special awesome Super flashing colors!
	SKINCOLOR_SUPERSILVER1 = FIRSTSUPERCOLOR,
	SKINCOLOR_SUPERSILVER2,
	SKINCOLOR_SUPERSILVER3,
	SKINCOLOR_SUPERSILVER4,
	SKINCOLOR_SUPERSILVER5,

	SKINCOLOR_SUPERRED1,
	SKINCOLOR_SUPERRED2,
	SKINCOLOR_SUPERRED3,
	SKINCOLOR_SUPERRED4,
	SKINCOLOR_SUPERRED5,

	SKINCOLOR_SUPERORANGE1,
	SKINCOLOR_SUPERORANGE2,
	SKINCOLOR_SUPERORANGE3,
	SKINCOLOR_SUPERORANGE4,
	SKINCOLOR_SUPERORANGE5,

	SKINCOLOR_SUPERGOLD1,
	SKINCOLOR_SUPERGOLD2,
	SKINCOLOR_SUPERGOLD3,
	SKINCOLOR_SUPERGOLD4,
	SKINCOLOR_SUPERGOLD5,

	SKINCOLOR_SUPERPERIDOT1,
	SKINCOLOR_SUPERPERIDOT2,
	SKINCOLOR_SUPERPERIDOT3,
	SKINCOLOR_SUPERPERIDOT4,
	SKINCOLOR_SUPERPERIDOT5,

	SKINCOLOR_SUPERSKY1,
	SKINCOLOR_SUPERSKY2,
	SKINCOLOR_SUPERSKY3,
	SKINCOLOR_SUPERSKY4,
	SKINCOLOR_SUPERSKY5,

	SKINCOLOR_SUPERPURPLE1,
	SKINCOLOR_SUPERPURPLE2,
	SKINCOLOR_SUPERPURPLE3,
	SKINCOLOR_SUPERPURPLE4,
	SKINCOLOR_SUPERPURPLE5,

	SKINCOLOR_SUPERRUST1,
	SKINCOLOR_SUPERRUST2,
	SKINCOLOR_SUPERRUST3,
	SKINCOLOR_SUPERRUST4,
	SKINCOLOR_SUPERRUST5,

	SKINCOLOR_SUPERTAN1,
	SKINCOLOR_SUPERTAN2,
	SKINCOLOR_SUPERTAN3,
	SKINCOLOR_SUPERTAN4,
	SKINCOLOR_SUPERTAN5,

	SKINCOLOR_FIRSTFREESLOT,
	SKINCOLOR_LASTFREESLOT = SKINCOLOR_FIRSTFREESLOT + NUMCOLORFREESLOTS - 1,

	MAXSKINCOLORS,

	NUMSUPERCOLORS = ((SKINCOLOR_FIRSTFREESLOT - FIRSTSUPERCOLOR)/5)
} skincolornum_t;

extern UINT16 numskincolors;

extern skincolor_t skincolors[MAXSKINCOLORS];

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define TICRATE 35
#define NEWTICRATERATIO 1 // try 4 for 140 fps :)
#define NEWTICRATE (TICRATE*NEWTICRATERATIO)

#define MUSICRATE 1000 // sound timing is calculated by milliseconds

#define RING_DIST 512*FRACUNIT // how close you need to be to a ring to attract it

#define PUSHACCEL (2*FRACUNIT) // Acceleration for MF2_SLIDEPUSH items.

// Special linedef executor tag numbers!
enum {
	LE_PINCHPHASE      =    -2, // A boss entered pinch phase (and, in most cases, is preparing their pinch phase attack!)
	LE_ALLBOSSESDEAD   =    -3, // All bosses in the map are dead (Egg capsule raise)
	LE_BOSSDEAD        =    -4, // A boss in the map died (Chaos mode boss tally)
	LE_BOSS4DROP       =    -5, // CEZ boss dropped its cage (also subtract the number of hitpoints it's lost)
	LE_BRAKVILEATACK   =    -6, // Brak's doing his LOS attack, oh noes
	LE_TURRET          = 32000, // THZ turret
	LE_BRAKPLATFORM    =  4200, // v2.0 Black Eggman destroys platform
	LE_CAPSULE2        =   682, // Egg Capsule
	LE_CAPSULE1        =   681, // Egg Capsule
	LE_CAPSULE0        =   680, // Egg Capsule
	LE_KOOPA           =   650, // Distant cousin to Gay Bowser
	LE_AXE             =   649, // MKB Axe object
	LE_PARAMWIDTH      =  -100  // If an object that calls LinedefExecute has a nonzero parameter value, this times the parameter will be subtracted. (Mostly for the purpose of coexisting bosses...)
};

// Name of local directory for config files and savegames
#if (((defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON)) && !defined (__CYGWIN__)) && !defined (__APPLE__)
#define DEFAULTDIR ".srb2"
#else
#define DEFAULTDIR "srb2"
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
extern char savegamename[256];
extern char liveeventbackup[256];

// m_misc.h
#ifdef GETTEXT
#define M_GetText(String) gettext(String)
#else
// If no translations are to be used, make a stub
// M_GetText function that just returns the string.
#define M_GetText(x) (x)
#endif
void M_StartupLocale(void);
extern void *(*M_Memcpy)(void* dest, const void* src, size_t n) FUNCNONNULL;
char *va(const char *format, ...) FUNCPRINTF;
char *M_GetToken(const char *inputString);
void M_UnGetToken(void);
UINT32 M_GetTokenPos(void);
void M_SetTokenPos(UINT32 newPos);
char *sizeu1(size_t num);
char *sizeu2(size_t num);
char *sizeu3(size_t num);
char *sizeu4(size_t num);
char *sizeu5(size_t num);

// d_main.c
extern int    VERSION;
extern int SUBVERSION;
extern boolean devparm; // development mode (-debug)
// d_netcmd.c
extern INT32 cv_debug;

#define DBG_BASIC       0x0001
#define DBG_DETAILED    0x0002
#define DBG_PLAYER      0x0004
#define DBG_RENDER      0x0008
#define DBG_NIGHTSBASIC 0x0010
#define DBG_NIGHTS      0x0020
#define DBG_POLYOBJ     0x0040
#define DBG_GAMELOGIC   0x0080
#define DBG_NETPLAY     0x0100
#define DBG_MEMORY      0x0200
#define DBG_SETUP       0x0400
#define DBG_LUA         0x0800
#define DBG_RANDOMIZER  0x1000
#define DBG_VIEWMORPH   0x2000

// =======================
// Misc stuff for later...
// =======================

#define ANG2RAD(angle) ((float)((angle)*M_PI)/ANGLE_180)

// Modifier key variables, accessible anywhere
extern UINT8 shiftdown, ctrldown, altdown;
extern boolean capslock;

// if we ever make our alloc stuff...
#define ZZ_Alloc(x) Z_Malloc(x, PU_STATIC, NULL)
#define ZZ_Calloc(x) Z_Calloc(x, PU_STATIC, NULL)

// i_system.c, replace getchar() once the keyboard has been appropriated
INT32 I_GetKey(void);

#ifndef min // Double-Check with WATTCP-32's cdefs.h
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max // Double-Check with WATTCP-32's cdefs.h
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

// Max gamepad/joysticks that can be detected/used.
#define MAX_JOYSTICKS 4

#ifndef M_PIl
#define M_PIl 3.1415926535897932384626433832795029L
#endif

// Floating point comparison epsilons from float.h
#ifndef FLT_EPSILON
#define FLT_EPSILON 1.1920928955078125e-7f
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16l
#endif

// An assert-type mechanism.
#ifdef PARANOIA
#define I_Assert(e) ((e) ? (void)0 : I_Error("assert failed: %s, file %s, line %d", #e, __FILE__, __LINE__))
#else
#define I_Assert(e) ((void)0)
#endif

// The character that separates pathnames. Forward slash on
// most systems, but reverse solidus (\) on Windows.
#if defined (_WIN32)
	#define PATHSEP "\\"
#else
	#define PATHSEP "/"
#endif

#define PUNCTUATION "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"

// Compile date and time and revision.
extern const char *compdate, *comptime, *comprevision, *compbranch;

// Disabled code and code under testing
// None of these that are disabled in the normal build are guaranteed to work perfectly
// Compile them at your own risk!

///	Allows the use of devmode in multiplayer. AKA "fishcake"
//#define NETGAME_DEVMODE

///	Allows gravity changes in netgames, no questions asked.
//#define NETGAME_GRAVITY

///	Dumps the contents of a network save game upon consistency failure for debugging.
//#define DUMPCONSISTENCY

///	See name of player in your crosshair
#define SEENAMES

///	Who put weights on my recycler?  ... Inuyasha did.
///	\note	XMOD port.
//#define WEIGHTEDRECYCLER

///	Allow loading of savegames between different versions of the game.
///	\note	XMOD port.
///	    	Most modifications should probably enable this.
//#define SAVEGAME_OTHERVERSIONS

///	Shuffle's incomplete OpenGL sorting code.
#define SHUFFLE // This has nothing to do with sorting, why was it disabled?

///	Allow the use of the SOC RESETINFO command.
///	\note	Builds that are tight on memory should disable this.
///	    	This stops the game from storing backups of the states, sprites, and mobjinfo tables.
///	    	Though this info is compressed under normal circumstances, it's still a lot of extra
///	    	memory that never gets touched.
#define ALLOW_RESETDATA

/// Experimental tweaks to analog mode. (Needs a lot of work before it's ready for primetime.)
//#define REDSANALOG

/// Backwards compatibility with musicslots.
/// \note	You should leave this enabled unless you're working with a future SRB2 version.
#define MUSICSLOT_COMPATIBILITY

/// Experimental attempts at preventing MF_PAPERCOLLISION objects from getting stuck in walls.
//#define PAPER_COLLISIONCORRECTION

/// FINALLY some real clipping that doesn't make walls dissappear AND speeds the game up
/// (that was the original comment from SRB2CB, sadly it is a lie and actually slows game down)
/// on the bright side it fixes some weird issues with translucent walls
/// \note	SRB2CB port.
///      	SRB2CB itself ported this from PrBoom+
#define NEWCLIP

/// OpenGL shaders
#define GL_SHADERS

/// Handle touching sector specials in P_PlayerAfterThink instead of P_PlayerThink.
/// \note   Required for proper collision with moving sloped surfaces that have sector specials on them.
#define SECTORSPECIALSAFTERTHINK

/// Cache patches in Lua in a way that renderer switching will work flawlessly.
//#define LUA_PATCH_SAFETY

/// Sprite rotation
#define ROTSPRITE
#define ROTANGLES 72 // Needs to be a divisor of 360 (45, 60, 90, 120...)
#define ROTANGDIFF (360 / ROTANGLES)

/// PNG support
#ifndef HAVE_PNG
#define NO_PNG_LUMPS
#endif

/// Render flats on walls
#define WALLFLATS

/// Maintain compatibility with older 2.2 demos
#define OLD22DEMOCOMPAT

#if defined (HAVE_CURL) && ! defined (NONET)
#define MASTERSERVER
#else
#undef UPDATE_ALERT
#endif

#endif // __DOOMDEF__
