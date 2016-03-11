// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  doomstat.h
/// \brief All the global variables that store the internal state.
///
///        Theoretically speaking, the internal state of the engine
///        should be found by looking at the variables collected
///        here, and every relevant module will have to include
///        this header file. In practice... things are a bit messy.

#ifndef __DOOMSTAT__
#define __DOOMSTAT__

// We need globally shared data structures, for defining the global state variables.
#include "doomdata.h"

// We need the player data structure as well.
#include "d_player.h"

// =============================
// Selected map etc.
// =============================

// Selected by user.
extern INT16 gamemap;
extern char mapmusname[7];
extern UINT16 mapmusflags;
#define MUSIC_TRACKMASK   0x0FFF // ----************
#define MUSIC_RELOADRESET 0x8000 // *---------------
// Use other bits if necessary.

extern INT16 maptol;
extern UINT8 globalweather;
extern INT32 curWeather;
extern INT32 cursaveslot;
extern INT16 lastmapsaved;
extern boolean gamecomplete;

#define PRECIP_NONE  0
#define PRECIP_STORM 1
#define PRECIP_SNOW  2
#define PRECIP_RAIN  3
#define PRECIP_BLANK 4
#define PRECIP_STORM_NORAIN 5
#define PRECIP_STORM_NOSTRIKES 6

// Set if homebrew PWAD stuff has been added.
extern boolean modifiedgame;
extern UINT16 mainwads;
extern boolean savemoddata; // This mod saves time/emblem data.
extern boolean disableSpeedAdjust; // Don't alter the duration of player states if true
extern boolean imcontinuing; // Temporary flag while continuing
extern boolean metalrecording;

#define ATTACKING_NONE   0
#define ATTACKING_RECORD 1
#define ATTACKING_NIGHTS 2
extern UINT8 modeattacking;

// menu demo things
extern UINT8  numDemos;
extern UINT32 demoDelayTime;
extern UINT32 demoIdleTime;

// Netgame? only true in a netgame
extern boolean netgame;
extern boolean addedtogame; // true after the server has added you
// Only true if >1 player. netgame => multiplayer but not (multiplayer=>netgame)
extern boolean multiplayer;

extern INT16 gametype;
extern boolean splitscreen;
extern boolean circuitmap; // Does this level have 'circuit mode'?
extern boolean fromlevelselect;

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean nomidimusic; // defined in d_main.c
extern boolean nosound;
extern boolean nodigimusic;
extern boolean music_disabled;
extern boolean sound_disabled;
extern boolean digital_disabled;

// =========================
// Status flags for refresh.
// =========================
//

extern boolean menuactive; // Menu overlaid?
extern UINT8 paused; // Game paused?
extern UINT8 window_notinfocus; // are we in focus? (backend independant -- handles auto pausing and display of "focus lost" message)

extern boolean nodrawers;
extern boolean noblit;
extern boolean lastdraw;
extern postimg_t postimgtype;
extern INT32 postimgparam;
extern postimg_t postimgtype2;
extern INT32 postimgparam2;

extern INT32 viewwindowx, viewwindowy;
extern INT32 viewwidth, scaledviewwidth;

extern boolean gamedataloaded;

// Player taking events, and displaying.
extern INT32 consoleplayer;
extern INT32 displayplayer;
extern INT32 secondarydisplayplayer; // for splitscreen

// Maps of special importance
extern INT16 spstage_start;
extern INT16 sstage_start;
extern INT16 sstage_end;

extern boolean looptitle;
extern boolean useNightsSS;

// CTF colors.
extern UINT8 skincolor_redteam, skincolor_blueteam, skincolor_redring, skincolor_bluering;

extern tic_t countdowntimer;
extern boolean countdowntimeup;

typedef struct
{
	UINT8 numpics;
	char picname[8][8];
	UINT8 pichires[8];
	char *text;
	UINT16 xcoord[8];
	UINT16 ycoord[8];
	UINT16 picduration[8];
	UINT8 musicloop;
	UINT16 textxpos;
	UINT16 textypos;

	char   musswitch[7];
	UINT16 musswitchflags;

	UINT8 fadecolor; // Color number for fade, 0 means don't do the first fade
	UINT8 fadeinid;  // ID of the first fade, to a color -- ignored if fadecolor is 0
	UINT8 fadeoutid; // ID of the second fade, to the new screen
} scene_t; // TODO: It would probably behoove us to implement subsong/track selection here, too, but I'm lazy -SH

typedef struct
{
	scene_t scene[128]; // 128 scenes per cutscene.
	INT32 numscenes; // Number of scenes in this cutscene
} cutscene_t;

extern cutscene_t *cutscenes[128];

// For the Custom Exit linedef.
extern INT16 nextmapoverride;
extern boolean skipstats;

extern UINT32 totalrings; //  Total # of rings in a level

// Fun extra stuff
extern INT16 lastmap; // Last level you were at (returning from special stages).
extern mobj_t *redflag, *blueflag; // Pointers to physical flags
extern mapthing_t *rflagpoint, *bflagpoint; // Pointers to the flag spawn locations
#define GF_REDFLAG 1
#define GF_BLUEFLAG 2

// A single point in space.
typedef struct
{
	fixed_t x, y, z;
} mappoint_t;

extern struct quake
{
	// camera offsets and duration
	fixed_t x,y,z;
	UINT16 time;

	// location, radius, and intensity...
	mappoint_t *epicenter;
	fixed_t radius, intensity;
} quake;

// NiGHTS grades
typedef struct
{
	UINT32 grade[6]; // D, C, B, A, S, X (F: failed to reach any of these)
} nightsgrades_t;

// Custom Lua values
// (This is not ifdeffed so the map header structure can stay identical, just in case.)
typedef struct
{
	char option[32]; // 31 usable characters
	char value[256]; // 255 usable characters. If this seriously isn't enough then wtf.
} customoption_t;

/** Map header information.
  */
typedef struct
{
	// The original eight, plus one.
	char lvlttl[22];       ///< Level name without "Zone". (21 character limit instead of 32, 21 characters can display on screen max anyway)
	char subttl[33];       ///< Subtitle for level
	UINT8 actnum;          ///< Act number or 0 for none.
	UINT16 typeoflevel;    ///< Combination of typeoflevel flags.
	INT16 nextlevel;       ///< Map number of next level, or 1100-1102 to end.
	char musname[7];       ///< Music track to play. "" for no music.
	UINT16 mustrack;       ///< Subsong to play. Only really relevant for music modules and specific formats supported by GME. 0 to ignore.
	char forcecharacter[17];  ///< (SKINNAMESIZE+1) Skin to switch to or "" to disable.
	UINT8 weather;         ///< 0 = sunny day, 1 = storm, 2 = snow, 3 = rain, 4 = blank, 5 = thunder w/o rain, 6 = rain w/o lightning, 7 = heat wave.
	INT16 skynum;          ///< Sky number to use.
	INT16 skybox_scalex;   ///< Skybox X axis scale. (0 = no movement, 1 = 1:1 movement, 16 = 16:1 slow movement, -4 = 1:4 fast movement, etc.)
	INT16 skybox_scaley;   ///< Skybox Y axis scale.
	INT16 skybox_scalez;   ///< Skybox Z axis scale.

	// Extra information.
	char interscreen[8];  ///< 320x200 patch to display at intermission.
	char runsoc[33];      ///< SOC to execute at start of level (32 character limit instead of 63)
	char scriptname[33];  ///< Script to use when the map is switched to. (32 character limit instead of 191)
	UINT8 precutscenenum; ///< Cutscene number to play BEFORE a level starts.
	UINT8 cutscenenum;    ///< Cutscene number to use, 0 for none.
	INT16 countdown;      ///< Countdown until level end?
	UINT16 palette;       ///< PAL lump to use on this map
	UINT8 numlaps;        ///< Number of laps in circuit mode, unless overridden.
	SINT8 unlockrequired; ///< Is an unlockable required to play this level? -1 if no.
	UINT8 levelselect;    ///< Is this map available in the level select? If so, which map list is it available in?
	SINT8 bonustype;      ///< What type of bonus does this level have? (-1 for null.)

	UINT8 levelflags;     ///< LF_flags:  merged eight booleans into one UINT8 for space, see below
	UINT8 menuflags;      ///< LF2_flags: options that affect record attack / nights mode menus

	// NiGHTS stuff.
	UINT8 numGradedMares;   ///< Internal. For grade support.
	nightsgrades_t *grades; ///< NiGHTS grades. Allocated dynamically for space reasons. Be careful.

	// Lua stuff.
	// (This is not ifdeffed so the map header structure can stay identical, just in case.)
	UINT8 numCustomOptions;     ///< Internal. For Lua custom value support.
	customoption_t *customopts; ///< Custom options. Allocated dynamically for space reasons. Be careful.
} mapheader_t;

// level flags
#define LF_SCRIPTISFILE   1 ///< True if the script is a file, not a lump.
#define LF_SPEEDMUSIC     2 ///< Speed up act music for super sneakers
#define LF_NOSSMUSIC      4 ///< Disable Super Sonic music
#define LF_NORELOAD       8 ///< Don't reload level on death
#define LF_NOZONE        16 ///< Don't include "ZONE" on level title

#define LF2_HIDEINMENU     1 ///< Hide in the multiplayer menu
#define LF2_HIDEINSTATS    2 ///< Hide in the statistics screen
#define LF2_RECORDATTACK   4 ///< Show this map in Time Attack
#define LF2_NIGHTSATTACK   8 ///< Show this map in NiGHTS mode menu
#define LF2_NOVISITNEEDED 16 ///< Available in time attack/nights mode without visiting the level

extern mapheader_t* mapheaderinfo[NUMMAPS];

enum TypeOfLevel
{
	TOL_SP          = 0x01, ///< Single Player
	TOL_COOP        = 0x02, ///< Cooperative
	TOL_COMPETITION = 0x04, ///< Competition
	TOL_RACE        = 0x08, ///< Race
// Single Player default = 15

	TOL_MATCH       = 0x10, ///< Match
	TOL_TAG         = 0x20, ///< Tag
// Match/Tag default = 48

	TOL_CTF         = 0x40, ///< Capture the Flag
// CTF default = 64

	TOL_CUSTOM      = 0x80, ///< Custom (Lua-scripted, etc.)

	TOL_2D     = 0x0100, ///< 2D
	TOL_MARIO  = 0x0200, ///< Mario
	TOL_NIGHTS = 0x0400, ///< NiGHTS
	TOL_ERZ3   = 0x0800, ///< ERZ3
	TOL_XMAS   = 0x1000  ///< Christmas NiGHTS
};

// Gametypes
enum GameType
{
	GT_COOP = 0, // also used in single player
	GT_COMPETITION, // Classic "Race"
	GT_RACE,

	GT_MATCH,
	GT_TEAMMATCH,

	GT_TAG,
	GT_HIDEANDSEEK,

	GT_CTF, // capture the flag

	NUMGAMETYPES
};
// If you alter this list, update gametype_cons_t in m_menu.c

extern tic_t totalplaytime;

extern UINT8 stagefailed;

// Emeralds stored as bits to throw savegame hackers off.
extern UINT16 emeralds;
#define EMERALD1 1
#define EMERALD2 2
#define EMERALD3 4
#define EMERALD4 8
#define EMERALD5 16
#define EMERALD6 32
#define EMERALD7 64
#define ALL7EMERALDS(v) ((v & (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7)) == (EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7))

extern INT32 nummaprings; //keep track of spawned rings/coins

/** Time attack information, currently a very small structure.
  */
typedef struct
{
	tic_t time;   ///< Time in which the level was finished.
	UINT32 score; ///< Score when the level was finished.
	UINT16 rings; ///< Rings when the level was finished.
} recorddata_t;

/** Setup for one NiGHTS map.
  * These are dynamically allocated because I am insane
  */
#define GRADE_F 0
#define GRADE_E 1
#define GRADE_D 2
#define GRADE_C 3
#define GRADE_B 4
#define GRADE_A 5
#define GRADE_S 6

typedef struct
{
	// 8 mares, 1 overall (0)
	UINT8	nummares;
	UINT32	score[9];
	UINT8	grade[9];
	tic_t	time[9];
} nightsdata_t;

extern nightsdata_t *nightsrecords[NUMMAPS];
extern recorddata_t *mainrecords[NUMMAPS];

// mapvisited is now a set of flags that says what we've done in the map.
#define MV_VISITED      1
#define MV_BEATEN       2
#define MV_ALLEMERALDS  4
#define MV_ULTIMATE     8
#define MV_PERFECT     16
#define MV_MAX         31 // used in gamedata check
extern UINT8 mapvisited[NUMMAPS];

// Temporary holding place for nights data for the current map
nightsdata_t ntemprecords;

extern UINT32 token; ///< Number of tokens collected in a level
extern UINT32 tokenlist; ///< List of tokens collected
extern INT32 tokenbits; ///< Used for setting token bits
extern INT32 sstimer; ///< Time allotted in the special stage
extern UINT32 bluescore; ///< Blue Team Scores
extern UINT32 redscore;  ///< Red Team Scores

// Eliminates unnecessary searching.
extern boolean CheckForBustableBlocks;
extern boolean CheckForBouncySector;
extern boolean CheckForQuicksand;
extern boolean CheckForMarioBlocks;
extern boolean CheckForFloatBob;
extern boolean CheckForReverseGravity;

// Powerup durations
extern UINT16 invulntics;
extern UINT16 sneakertics;
extern UINT16 flashingtics;
extern UINT16 tailsflytics;
extern UINT16 underwatertics;
extern UINT16 spacetimetics;
extern UINT16 extralifetics;

extern UINT8 introtoplay;
extern UINT8 creditscutscene;

extern UINT8 use1upSound;
extern UINT8 maxXtraLife; // Max extra lives from rings

extern mobj_t *hunt1, *hunt2, *hunt3; // Emerald hunt locations

// For racing
extern UINT32 countdown;
extern UINT32 countdown2;

extern fixed_t gravity;

//for CTF balancing
extern INT16 autobalance;
extern INT16 teamscramble;
extern INT16 scrambleplayers[MAXPLAYERS]; //for CTF team scramble
extern INT16 scrambleteams[MAXPLAYERS]; //for CTF team scramble
extern INT16 scrambletotal; //for CTF team scramble
extern INT16 scramblecount; //for CTF team scramble

extern INT32 cheats;

extern tic_t hidetime;

extern UINT32 timesBeaten; // # of times the game has been beaten.
extern UINT32 timesBeatenWithEmeralds;
extern UINT32 timesBeatenUltimate;

// ===========================
// Internal parameters, fixed.
// ===========================
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

extern tic_t gametic;
#define localgametic leveltime

// Player spawn spots.
extern mapthing_t *playerstarts[MAXPLAYERS]; // Cooperative
extern mapthing_t *bluectfstarts[MAXPLAYERS]; // CTF
extern mapthing_t *redctfstarts[MAXPLAYERS]; // CTF

// =====================================
// Internal parameters, used for engine.
// =====================================

#if defined (macintosh)
#define DEBFILE(msg) I_OutputMsg(msg)
extern FILE *debugfile;
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if (debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
extern FILE *debugfile;
#else
#define DEBFILE(msg) {}
extern FILE *debugfile;
#endif
#endif

#ifdef DEBUGFILE
extern INT32 debugload;
#endif

// if true, load all graphics at level load
extern boolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;

// debug flag to cancel adaptiveness
extern boolean singletics;

// =============
// Netgame stuff
// =============

#include "d_clisrv.h"

extern consvar_t cv_timetic; // display high resolution timer
extern consvar_t cv_forceskin; // force clients to use the server's skin
extern consvar_t cv_downloading; // allow clients to downloading WADs.
extern ticcmd_t netcmds[MAXPLAYERS];
extern INT32 adminplayer, serverplayer;

/// \note put these in d_clisrv outright?

#endif //__DOOMSTAT__
