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
extern UINT32 mapmusposition;
#define MUSIC_TRACKMASK   0x0FFF // ----************
#define MUSIC_RELOADRESET 0x8000 // *---------------
#define MUSIC_FORCERESET  0x4000 // -*--------------
// Use other bits if necessary.

extern UINT32 maptol;
extern UINT8 globalweather;
extern INT32 curWeather;
extern INT32 cursaveslot;
//extern INT16 lastmapsaved;
extern INT16 lastmaploaded;
extern UINT8 gamecomplete;

// Extra abilities/settings for skins (combinable stuff)
typedef enum
{
	MA_RUNNING     = 1,    // In action
	MA_INIT        = 1<<1, // Initialisation
	MA_NOCUTSCENES = 1<<2, // No cutscenes
	MA_INGAME      = 1<<3  // Timer ignores loads
} marathonmode_t;

extern marathonmode_t marathonmode;
extern tic_t marathontime;

#define maxgameovers 13
extern UINT8 numgameovers;
extern SINT8 startinglivesbalance[maxgameovers+1];

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
extern UINT32 gametyperules;
extern INT16 gametypecount;

extern boolean splitscreen;
extern boolean circuitmap; // Does this level have 'circuit mode'?
extern boolean fromlevelselect;

// ========================================
// Internal parameters for sound rendering.
// ========================================

extern boolean midi_disabled;
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
extern INT16 spstage_start, spmarathon_start;
extern INT16 sstage_start, sstage_end, smpstage_start, smpstage_end;

extern INT16 titlemap;
extern boolean hidetitlepics;
extern INT16 bootmap; //bootmap for loading a map on startup

extern INT16 tutorialmap; // map to load for tutorial
extern boolean tutorialmode; // are we in a tutorial right now?
extern INT32 tutorialgcs; // which control scheme is loaded?
extern INT32 tutorialusemouse; // store cv_usemouse user value
extern INT32 tutorialfreelook; // store cv_alwaysfreelook user value
extern INT32 tutorialmousemove; // store cv_mousemove user value
extern INT32 tutorialanalog; // store cv_analog[0] user value

extern boolean looptitle;

// CTF colors.
extern UINT16 skincolor_redteam, skincolor_blueteam, skincolor_redring, skincolor_bluering;

extern tic_t countdowntimer;
extern boolean countdowntimeup;
extern boolean exitfadestarted;

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
	UINT32 musswitchposition;

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

// Reserve prompt space for tutorials
#define TUTORIAL_PROMPT 201 // one-based
#define TUTORIAL_AREAS 6
#define TUTORIAL_AREA_PROMPTS 5
#define MAX_PROMPTS (TUTORIAL_PROMPT+TUTORIAL_AREAS*TUTORIAL_AREA_PROMPTS*3) // 3 control modes
#define MAX_PAGES 128

#define PROMPT_PIC_PERSIST 0
#define PROMPT_PIC_LOOP 1
#define PROMPT_PIC_DESTROY 2
#define MAX_PROMPT_PICS 8
typedef struct
{
	UINT8 numpics;
	UINT8 picmode; // sequence mode after displaying last pic, 0 = persist, 1 = loop, 2 = destroy
	UINT8 pictoloop; // if picmode == loop, which pic to loop to?
	UINT8 pictostart; // initial pic number to show
	char picname[MAX_PROMPT_PICS][8];
	UINT8 pichires[MAX_PROMPT_PICS];
	UINT16 xcoord[MAX_PROMPT_PICS]; // gfx
	UINT16 ycoord[MAX_PROMPT_PICS]; // gfx
	UINT16 picduration[MAX_PROMPT_PICS];

	char   musswitch[7];
	UINT16 musswitchflags;
	UINT8 musicloop;

	char tag[33]; // page tag
	char name[34]; // narrator name, extra char for color
	char iconname[8]; // narrator icon lump
	boolean rightside; // narrator side, false = left, true = right
	boolean iconflip; // narrator flip icon horizontally
	UINT8 hidehud; // hide hud, 0 = show all, 1 = hide depending on prompt position (top/bottom), 2 = hide all
	UINT8 lines; // # of lines to show. If name is specified, name takes one of the lines. If 0, defaults to 4.
	INT32 backcolor; // see CON_SetupBackColormap: 0-11, INT32_MAX for user-defined (CONS_BACKCOLOR)
	UINT8 align; // text alignment, 0 = left, 1 = right, 2 = center
	UINT8 verticalalign; // vertical text alignment, 0 = top, 1 = bottom, 2 = middle
	UINT8 textspeed; // text speed, delay in tics between characters.
	sfxenum_t textsfx; // sfx_ id for printing text
	UINT8 nextprompt; // next prompt to jump to, one-based. 0 = current prompt
	UINT8 nextpage; // next page to jump to, one-based. 0 = next page within prompt->numpages
	char nexttag[33]; // next tag to jump to. If set, this overrides nextprompt and nextpage.
	INT32 timetonext; // time in tics to jump to next page automatically. 0 = don't jump automatically
	char *text;
} textpage_t;

typedef struct
{
	textpage_t page[MAX_PAGES];
	INT32 numpages; // Number of pages in this prompt
} textprompt_t;

extern textprompt_t *textprompts[MAX_PROMPTS];

// For the Custom Exit linedef.
extern INT16 nextmapoverride;
extern UINT8 skipstats;

extern UINT32 ssspheres; //  Total # of spheres in a level

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
	UINT32 typeoflevel;    ///< Combination of typeoflevel flags.
	INT16 nextlevel;       ///< Map number of next level, or 1100-1102 to end.
	INT16 marathonnext;    ///< See nextlevel, but for Marathon mode. Necessary to support hub worlds ala SUGOI.
	char keywords[33];     ///< Keywords separated by space to search for. 32 characters.
	char musname[7];       ///< Music track to play. "" for no music.
	UINT16 mustrack;       ///< Subsong to play. Only really relevant for music modules and specific formats supported by GME. 0 to ignore.
	UINT32 muspos;    ///< Music position to jump to.
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
	SINT8 maxbonuslives;  ///< How many bonus lives to award at Intermission? (-1 for unlimited.)

	UINT16 levelflags;     ///< LF_flags:  merged booleans into one UINT16 for space, see below
	UINT8 menuflags;      ///< LF2_flags: options that affect record attack / nights mode menus

	char selectheading[22]; ///< Level select heading. Allows for controllable grouping.
	UINT16 startrings;      ///< Number of rings players start with.
	INT32 sstimer;          ///< Timer for special stages.
	UINT32 ssspheres;       ///< Sphere requirement in special stages.
	fixed_t gravity;        ///< Map-wide gravity.

	// Title card.
	char ltzzpatch[8];      ///< Zig zag patch.
	char ltzztext[8];       ///< Zig zag text.
	char ltactdiamond[8];   ///< Act diamond.

	// Freed animals stuff.
	UINT8 numFlickies;     ///< Internal. For freed flicky support.
	mobjtype_t *flickies;  ///< List of freeable flickies in this level. Allocated dynamically for space reasons. Be careful.

	// NiGHTS stuff.
	UINT8 numGradedMares;   ///< Internal. For grade support.
	nightsgrades_t *grades; ///< NiGHTS grades. Allocated dynamically for space reasons. Be careful.

	// Music stuff.
	UINT32 musinterfadeout;  ///< Fade out level music on intermission screen in milliseconds
	char musintername[7];    ///< Intermission screen music.

	char muspostbossname[7];    ///< Post-bossdeath music.
	UINT16 muspostbosstrack;    ///< Post-bossdeath track.
	UINT32 muspostbosspos;      ///< Post-bossdeath position
	UINT32 muspostbossfadein;   ///< Post-bossdeath fade-in milliseconds.

	SINT8 musforcereset; ///< Force resetmusic (-1 for default; 0 for force off; 1 for force on)

	// Lua stuff.
	// (This is not ifdeffed so the map header structure can stay identical, just in case.)
	UINT8 numCustomOptions;     ///< Internal. For Lua custom value support.
	customoption_t *customopts; ///< Custom options. Allocated dynamically for space reasons. Be careful.
} mapheader_t;

// level flags
#define LF_SCRIPTISFILE       (1<<0) ///< True if the script is a file, not a lump.
#define LF_SPEEDMUSIC         (1<<1) ///< Speed up act music for super sneakers
#define LF_NOSSMUSIC          (1<<2) ///< Disable Super Sonic music
#define LF_NORELOAD           (1<<3) ///< Don't reload level on death
#define LF_NOZONE             (1<<4) ///< Don't include "ZONE" on level title
#define LF_SAVEGAME           (1<<5) ///< Save the game upon loading this level
#define LF_MIXNIGHTSCOUNTDOWN (1<<6) ///< Play sfx_timeup instead of music change for NiGHTS countdown
#define LF_WARNINGTITLE       (1<<7) ///< WARNING! WARNING! WARNING! WARNING!

#define LF_NOTITLECARDFIRST        (1<<8)
#define LF_NOTITLECARDRESPAWN      (1<<9)
#define LF_NOTITLECARDRECORDATTACK (1<<10)
#define LF_NOTITLECARD  (LF_NOTITLECARDFIRST|LF_NOTITLECARDRESPAWN|LF_NOTITLECARDRECORDATTACK) ///< Don't start the title card at all

#define LF2_HIDEINMENU     1 ///< Hide in the multiplayer menu
#define LF2_HIDEINSTATS    2 ///< Hide in the statistics screen
#define LF2_RECORDATTACK   4 ///< Show this map in Time Attack
#define LF2_NIGHTSATTACK   8 ///< Show this map in NiGHTS mode menu
#define LF2_NOVISITNEEDED 16 ///< Available in time attack/nights mode without visiting the level
#define LF2_WIDEICON      32 ///< If you're in a circumstance where it fits, use a wide map icon

extern mapheader_t* mapheaderinfo[NUMMAPS];

// Gametypes
#define NUMGAMETYPEFREESLOTS 128
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

	GT_FIRSTFREESLOT,
	GT_LASTFREESLOT = GT_FIRSTFREESLOT + NUMGAMETYPEFREESLOTS - 1,
	NUMGAMETYPES
};
// If you alter this list, update dehacked.c, MISC_ChangeGameTypeMenu in m_menu.c, and Gametype_Names in g_game.c

// Gametype rules
enum GameTypeRules
{
	GTR_CAMPAIGN         = 1,     // Linear Co-op map progression, don't allow random maps
	GTR_RINGSLINGER      = 1<<1,  // Outside of Co-op, Competition, and Race (overriden by cv_ringslinger)
	GTR_SPECTATORS       = 1<<2,  // Outside of Co-op, Competition, and Race
	GTR_LIVES            = 1<<3,  // Co-op and Competition
	GTR_TEAMS            = 1<<4,  // Team Match, CTF
	GTR_FIRSTPERSON      = 1<<5,  // First person camera
	GTR_POWERSTONES      = 1<<6,  // Power stones (Match and CTF)
	GTR_TEAMFLAGS        = 1<<7,  // Gametype has team flags (CTF)
	GTR_FRIENDLY         = 1<<8,  // Co-op
	GTR_SPECIALSTAGES    = 1<<9,  // Allow special stages
	GTR_EMERALDTOKENS    = 1<<10, // Spawn emerald tokens
	GTR_EMERALDHUNT      = 1<<11, // Emerald Hunt
	GTR_RACE             = 1<<12, // Race and Competition
	GTR_TAG              = 1<<13, // Tag and Hide and Seek
	GTR_POINTLIMIT       = 1<<14, // Ringslinger point limit
	GTR_TIMELIMIT        = 1<<15, // Ringslinger time limit
	GTR_OVERTIME         = 1<<16, // Allow overtime
	GTR_HURTMESSAGES     = 1<<17, // Hit and death messages
	GTR_FRIENDLYFIRE     = 1<<18, // Always allow friendly fire
	GTR_STARTCOUNTDOWN   = 1<<19, // Hide time countdown (Tag and Hide and Seek)
	GTR_HIDEFROZEN       = 1<<20, // Frozen after hide time (Hide and Seek, but not Tag)
	GTR_BLINDFOLDED      = 1<<21, // Blindfolded view (Tag and Hide and Seek)
	GTR_RESPAWNDELAY     = 1<<22, // Respawn delay
	GTR_PITYSHIELD       = 1<<23, // Award pity shield
	GTR_DEATHPENALTY     = 1<<24, // Death score penalty
	GTR_NOSPECTATORSPAWN = 1<<25, // Use with GTR_SPECTATORS, spawn in the map instead of with the spectators
	GTR_DEATHMATCHSTARTS = 1<<26, // Use deathmatch starts
	GTR_SPAWNINVUL       = 1<<27, // Babysitting deterrent
	GTR_SPAWNENEMIES     = 1<<28, // Spawn enemies
	GTR_ALLOWEXIT        = 1<<29, // Allow exit sectors
	GTR_NOTITLECARD      = 1<<30, // Don't show the title card
	GTR_CUTSCENES        = 1<<31, // Play cutscenes, ending, credits, and evaluation
};

// String names for gametypes
extern const char *Gametype_Names[NUMGAMETYPES];
extern const char *Gametype_ConstantNames[NUMGAMETYPES];

// Point and time limits for every gametype
extern INT32 pointlimits[NUMGAMETYPES];
extern INT32 timelimits[NUMGAMETYPES];

// TypeOfLevel things
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

	// 0x80 was here

	TOL_2D     = 0x0100, ///< 2D
	TOL_MARIO  = 0x0200, ///< Mario
	TOL_NIGHTS = 0x0400, ///< NiGHTS
	TOL_ERZ3   = 0x0800, ///< ERZ3
	TOL_XMAS   = 0x1000, ///< Christmas NiGHTS
};

#define MAXTOL             (1<<31)
#define NUMBASETOLNAMES    (19)
#define NUMTOLNAMES        (NUMBASETOLNAMES + NUMGAMETYPEFREESLOTS)

typedef struct
{
	const char *name;
	UINT32 flag;
} tolinfo_t;
extern tolinfo_t TYPEOFLEVEL[NUMTOLNAMES];
extern UINT32 lastcustomtol;

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

#define NUM_LUABANKS 16 // please only make this number go up between versions, never down. you'll break saves otherwise. also, must fit in UINT8
extern INT32 luabanks[NUM_LUABANKS];

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
#define MV_PERFECTRA   32
#define MV_MAX         63 // used in gamedata check, update whenever MV's are added
#define MV_MP         128
extern UINT8 mapvisited[NUMMAPS];

// Temporary holding place for nights data for the current map
extern nightsdata_t ntemprecords;

extern UINT32 token; ///< Number of tokens collected in a level
extern UINT32 tokenlist; ///< List of tokens collected
extern boolean gottoken; ///< Did you get a token? Used for end of act
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
extern UINT16 nightslinktics;

extern UINT8 introtoplay;
extern UINT8 creditscutscene;
extern UINT8 useBlackRock;

extern UINT8 use1upSound;
extern UINT8 maxXtraLife; // Max extra lives from rings
extern UINT8 useContinues;
#define continuesInSession (!multiplayer && (ultimatemode || (useContinues && !marathonmode) || (!modeattacking && !(cursaveslot > 0))))

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

#define WAYPOINTSEQUENCESIZE 256
#define NUMWAYPOINTSEQUENCES 256
extern mobj_t *waypoints[NUMWAYPOINTSEQUENCES][WAYPOINTSEQUENCESIZE];
extern UINT16 numwaypoints[NUMWAYPOINTSEQUENCES];

void P_AddWaypoint(UINT8 sequence, UINT8 id, mobj_t *waypoint);
mobj_t *P_GetFirstWaypoint(UINT8 sequence);
mobj_t *P_GetLastWaypoint(UINT8 sequence);
mobj_t *P_GetPreviousWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetNextWaypoint(mobj_t *current, boolean wrap);
mobj_t *P_GetClosestWaypoint(UINT8 sequence, mobj_t *mo);
boolean P_IsDegeneratedWaypointSequence(UINT8 sequence);

// =====================================
// Internal parameters, used for engine.
// =====================================

#if defined (macintosh)
#define DEBFILE(msg) I_OutputMsg(msg)
#else
#define DEBUGFILE
#ifdef DEBUGFILE
#define DEBFILE(msg) { if (debugfile) { fputs(msg, debugfile); fflush(debugfile); } }
#else
#define DEBFILE(msg) {}
#endif
#endif

#ifdef DEBUGFILE
extern FILE *debugfile;
extern INT32 debugload;
#endif

// if true, load all graphics at level load
extern boolean precache;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern gamestate_t wipegamestate;
extern INT16 wipetypepre;
extern INT16 wipetypepost;

// debug flag to cancel adaptiveness
extern boolean singletics;

// =============
// Netgame stuff
// =============

#include "d_clisrv.h"

extern consvar_t cv_timetic; // display high resolution timer
extern consvar_t cv_powerupdisplay; // display powerups
extern consvar_t cv_showinputjoy; // display joystick in time attack
extern consvar_t cv_forceskin; // force clients to use the server's skin
extern consvar_t cv_downloading; // allow clients to downloading WADs.
extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];
extern INT32 serverplayer;
extern INT32 adminplayers[MAXPLAYERS];

/// \note put these in d_clisrv outright?

#endif //__DOOMSTAT__
