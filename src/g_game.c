// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_game.c
/// \brief game loop functions, events handling

#include "doomdef.h"
#include "console.h"
#include "d_main.h"
#include "d_player.h"
#include "d_clisrv.h"
#include "f_finale.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "i_system.h"
#include "am_map.h"
#include "m_random.h"
#include "p_local.h"
#include "r_draw.h"
#include "r_main.h"
#include "s_sound.h"
#include "g_game.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_argv.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "z_zone.h"
#include "i_video.h"
#include "byteptr.h"
#include "i_joy.h"
#include "r_local.h"
#include "r_things.h"
#include "y_inter.h"
#include "v_video.h"
#include "dehacked.h" // get_number (for ghost thok)
#include "lua_hook.h"
#include "b_bot.h"
#include "m_cond.h" // condition sets
#include "md5.h" // demo checksums

#ifdef HAVE_BLUA
#include "lua_hud.h"
#endif

gameaction_t gameaction;
gamestate_t gamestate = GS_NULL;
UINT8 ultimatemode = false;

boolean botingame;
UINT8 botskin;
UINT8 botcolor;

JoyType_t Joystick;
JoyType_t Joystick2;

// 1024 bytes is plenty for a savegame
#define SAVEGAMESIZE (1024)

char gamedatafilename[64] = "gamedata.dat";
char timeattackfolder[64] = "main";
char customversionstring[32] = "\0";

static void G_DoCompleted(void);
static void G_DoStartContinue(void);
static void G_DoContinued(void);
static void G_DoWorldDone(void);

char   mapmusname[7]; // Music name
UINT16 mapmusflags; // Track and reset bit
UINT32 mapmusposition; // Position to jump to

INT16 gamemap = 1;
UINT32 maptol;
UINT8 globalweather = 0;
INT32 curWeather = PRECIP_NONE;
INT32 cursaveslot = 0; // Auto-save 1p savegame slot
//INT16 lastmapsaved = 0; // Last map we auto-saved at
INT16 lastmaploaded = 0; // Last map the game loaded
boolean gamecomplete = false;

UINT8 numgameovers = 0; // for startinglives balance
SINT8 startinglivesbalance[maxgameovers+1] = {3, 5, 7, 9, 12, 15, 20, 25, 30, 40, 50, 75, 99, 0x7F};

UINT16 mainwads = 0;
boolean modifiedgame; // Set if homebrew PWAD stuff has been added.
boolean savemoddata = false;
UINT8 paused;
UINT8 modeattacking = ATTACKING_NONE;
boolean disableSpeedAdjust = false;
boolean imcontinuing = false;
boolean runemeraldmanager = false;
UINT16 emeraldspawndelay = 60*TICRATE;

// menu demo things
UINT8  numDemos      = 0;
UINT32 demoDelayTime = 15*TICRATE;
UINT32 demoIdleTime  = 3*TICRATE;

boolean timingdemo; // if true, exit with report on completion
boolean nodrawers; // for comparative timing purposes
boolean noblit; // for comparative timing purposes
tic_t demostarttime; // for comparative timing purposes

boolean netgame; // only true if packets are broadcast
boolean multiplayer;
boolean playeringame[MAXPLAYERS];
boolean addedtogame;
player_t players[MAXPLAYERS];

INT32 consoleplayer; // player taking events and displaying
INT32 displayplayer; // view being displayed
INT32 secondarydisplayplayer; // for splitscreen

tic_t gametic;
tic_t levelstarttic; // gametic at level start
UINT32 ssspheres; // old special stage
INT16 lastmap; // last level you were at (returning from special stages)
tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)

INT16 spstage_start;
INT16 sstage_start, sstage_end, smpstage_start, smpstage_end;

INT16 titlemap = 0;
boolean hidetitlepics = false;
INT16 bootmap; //bootmap for loading a map on startup

INT16 tutorialmap = 0; // map to load for tutorial
boolean tutorialmode = false; // are we in a tutorial right now?
INT32 tutorialgcs = gcs_custom; // which control scheme is loaded?
INT32 tutorialusemouse = 0; // store cv_usemouse user value
INT32 tutorialfreelook = 0; // store cv_alwaysfreelook user value
INT32 tutorialmousemove = 0; // store cv_mousemove user value
INT32 tutorialanalog = 0; // store cv_analog user value

boolean looptitle = false;

UINT8 skincolor_redteam = SKINCOLOR_RED;
UINT8 skincolor_blueteam = SKINCOLOR_BLUE;
UINT8 skincolor_redring = SKINCOLOR_SALMON;
UINT8 skincolor_bluering = SKINCOLOR_CORNFLOWER;

tic_t countdowntimer = 0;
boolean countdowntimeup = false;
boolean exitfadestarted = false;

cutscene_t *cutscenes[128];
textprompt_t *textprompts[MAX_PROMPTS];

INT16 nextmapoverride;
UINT8 skipstats;

// Pointers to each CTF flag
mobj_t *redflag;
mobj_t *blueflag;
// Pointers to CTF spawn location
mapthing_t *rflagpoint;
mapthing_t *bflagpoint;

struct quake quake;

// Map Header Information
mapheader_t* mapheaderinfo[NUMMAPS] = {NULL};

static boolean exitgame = false;
static boolean retrying = false;
static boolean retryingmodeattack = false;

UINT8 stagefailed; // Used for GEMS BONUS? Also to see if you beat the stage.

UINT16 emeralds;
INT32 luabanks[NUM_LUABANKS]; // yes, even in non HAVE_BLUA
UINT32 token; // Number of tokens collected in a level
UINT32 tokenlist; // List of tokens collected
boolean gottoken; // Did you get a token? Used for end of act
INT32 tokenbits; // Used for setting token bits

// Old Special Stage
INT32 sstimer; // Time allotted in the special stage

tic_t totalplaytime;
boolean gamedataloaded = false;

// Time attack data for levels
// These are dynamically allocated for space reasons now
recorddata_t *mainrecords[NUMMAPS]   = {NULL};
nightsdata_t *nightsrecords[NUMMAPS] = {NULL};
UINT8 mapvisited[NUMMAPS];

// Temporary holding place for nights data for the current map
nightsdata_t ntemprecords;

UINT32 bluescore, redscore; // CTF and Team Match team scores

// ring count... for PERFECT!
INT32 nummaprings = 0;

// Elminates unnecessary searching.
boolean CheckForBustableBlocks;
boolean CheckForBouncySector;
boolean CheckForQuicksand;
boolean CheckForMarioBlocks;
boolean CheckForFloatBob;
boolean CheckForReverseGravity;

// Powerup durations
UINT16 invulntics = 20*TICRATE;
UINT16 sneakertics = 20*TICRATE;
UINT16 flashingtics = 3*TICRATE;
UINT16 tailsflytics = 8*TICRATE;
UINT16 underwatertics = 30*TICRATE;
UINT16 spacetimetics = 11*TICRATE + (TICRATE/2);
UINT16 extralifetics = 4*TICRATE;
UINT16 nightslinktics = 2*TICRATE;

INT32 gameovertics = 11*TICRATE;

UINT8 ammoremovaltics = 2*TICRATE;

UINT8 use1upSound = 0;
UINT8 maxXtraLife = 2; // Max extra lives from rings

UINT8 introtoplay;
UINT8 creditscutscene;

// Emerald locations
mobj_t *hunt1;
mobj_t *hunt2;
mobj_t *hunt3;

UINT32 countdown, countdown2; // for racing

fixed_t gravity;

INT16 autobalance; //for CTF team balance
INT16 teamscramble; //for CTF team scramble
INT16 scrambleplayers[MAXPLAYERS]; //for CTF team scramble
INT16 scrambleteams[MAXPLAYERS]; //for CTF team scramble
INT16 scrambletotal; //for CTF team scramble
INT16 scramblecount; //for CTF team scramble

INT32 cheats; //for multiplayer cheat commands

tic_t hidetime;

// Grading
UINT32 timesBeaten;
UINT32 timesBeatenWithEmeralds;
UINT32 timesBeatenUltimate;

static char demoname[64];
boolean demorecording;
boolean demoplayback;
boolean titledemo; // Title Screen demo can be cancelled by any key
static UINT8 *demobuffer = NULL;
static UINT8 *demo_p, *demotime_p;
static UINT8 *demoend;
static UINT8 demoflags;
static UINT16 demoversion;
boolean singledemo; // quit after playing a demo from cmdline
boolean demo_start; // don't start playing demo right away
boolean demosynced = true; // console warning message

boolean metalrecording; // recording as metal sonic
mobj_t *metalplayback;
static UINT8 *metalbuffer = NULL;
static UINT8 *metal_p;
static UINT16 metalversion;

// extra data stuff (events registered this frame while recording)
static struct {
	UINT8 flags; // EZT flags

	// EZT_COLOR
	UINT8 color, lastcolor;

	// EZT_SCALE
	fixed_t scale, lastscale;

	// EZT_HIT
	UINT16 hits;
	mobj_t **hitlist;
} ghostext;

// Your naming conventions are stupid and useless.
// There is no conflict here.
typedef struct demoghost {
	UINT8 checksum[16];
	UINT8 *buffer, *p, color, fadein;
	UINT16 version;
	mobj_t oldmo, *mo;
	struct demoghost *next;
} demoghost;
demoghost *ghosts = NULL;

boolean precache = true; // if true, load all graphics at start

INT16 prevmap, nextmap;

static UINT8 *savebuffer;

// Analog Control
static void UserAnalog_OnChange(void);
static void UserAnalog2_OnChange(void);
static void Analog_OnChange(void);
static void Analog2_OnChange(void);
static void DirectionChar_OnChange(void);
static void DirectionChar2_OnChange(void);
static void AutoBrake_OnChange(void);
static void AutoBrake2_OnChange(void);
void SendWeaponPref(void);
void SendWeaponPref2(void);

static CV_PossibleValue_t crosshair_cons_t[] = {{0, "Off"}, {1, "Cross"}, {2, "Angle"}, {3, "Point"}, {0, NULL}};
static CV_PossibleValue_t joyaxis_cons_t[] = {{0, "None"},
{1, "X-Axis"}, {2, "Y-Axis"}, {-1, "X-Axis-"}, {-2, "Y-Axis-"},
#if JOYAXISSET > 1
{3, "Z-Axis"}, {4, "X-Rudder"}, {-3, "Z-Axis-"}, {-4, "X-Rudder-"},
#endif
#if JOYAXISSET > 2
{5, "Y-Rudder"}, {6, "Z-Rudder"}, {-5, "Y-Rudder-"}, {-6, "Z-Rudder-"},
#endif
#if JOYAXISSET > 3
{7, "U-Axis"}, {8, "V-Axis"}, {-7, "U-Axis-"}, {-8, "V-Axis-"},
#endif
 {0, NULL}};
#if JOYAXISSET > 4
"More Axis Sets"
#endif

// don't mind me putting these here, I was lazy to figure out where else I could put those without blowing up the compiler.

// it automatically becomes compact with 20+ players, but if you like it, I guess you can turn that on!
consvar_t cv_compactscoreboard= {"compactscoreboard", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// chat timer thingy
static CV_PossibleValue_t chattime_cons_t[] = {{5, "MIN"}, {999, "MAX"}, {0, NULL}};
consvar_t cv_chattime = {"chattime", "8", CV_SAVE, chattime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// chatwidth
static CV_PossibleValue_t chatwidth_cons_t[] = {{64, "MIN"}, {150, "MAX"}, {0, NULL}};
consvar_t cv_chatwidth = {"chatwidth", "128", CV_SAVE, chatwidth_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// chatheight
static CV_PossibleValue_t chatheight_cons_t[] = {{6, "MIN"}, {22, "MAX"}, {0, NULL}};
consvar_t cv_chatheight= {"chatheight", "8", CV_SAVE, chatheight_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// chat notifications (do you want to hear beeps? I'd understand if you didn't.)
consvar_t cv_chatnotifications= {"chatnotifications", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// chat spam protection (why would you want to disable that???)
consvar_t cv_chatspamprotection= {"chatspamprotection", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// minichat text background
consvar_t cv_chatbacktint = {"chatbacktint", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// old shit console chat. (mostly exists for stuff like terminal, not because I cared if anyone liked the old chat.)
static CV_PossibleValue_t consolechat_cons_t[] = {{0, "Window"}, {1, "Console"}, {2, "Window (Hidden)"}, {0, NULL}};
consvar_t cv_consolechat = {"chatmode", "Window", CV_SAVE, consolechat_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

// Pause game upon window losing focus
consvar_t cv_pauseifunfocused = {"pauseifunfocused", "Yes", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_crosshair = {"crosshair", "Cross", CV_SAVE, crosshair_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_crosshair2 = {"crosshair2", "Cross", CV_SAVE, crosshair_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invertmouse = {"invertmouse", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_alwaysfreelook = {"alwaysmlook", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invertmouse2 = {"invertmouse2", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_alwaysfreelook2 = {"alwaysmlook2", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasefreelook = {"chasemlook", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_chasefreelook2 = {"chasemlook2", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mousemove = {"mousemove", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mousemove2 = {"mousemove2", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

// previously "analog", "analog2", "useranalog", and "useranalog2", invalidating 2.1-era copies of config.cfg
// changed because it'd be nice to see people try out our actually good controls with gamepads now autobrake exists
consvar_t cv_analog = {"sessionanalog", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, Analog_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_analog2 = {"sessionanalog2", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, Analog2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_useranalog = {"configanalog", "Off", CV_SAVE|CV_CALL|CV_NOSHOWHELP, CV_OnOff, UserAnalog_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_useranalog2 = {"configanalog2", "Off", CV_SAVE|CV_CALL|CV_NOSHOWHELP, CV_OnOff, UserAnalog2_OnChange, 0, NULL, NULL, 0, 0, NULL};

// deez New User eXperiences
static CV_PossibleValue_t directionchar_cons_t[] = {{0, "Camera"}, {1, "Movement"}, {0, NULL}};
consvar_t cv_directionchar = {"directionchar", "Movement", CV_SAVE|CV_CALL, directionchar_cons_t, DirectionChar_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_directionchar2 = {"directionchar2", "Movement", CV_SAVE|CV_CALL, directionchar_cons_t, DirectionChar2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_autobrake = {"autobrake", "On", CV_SAVE|CV_CALL, CV_OnOff, AutoBrake_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_autobrake2 = {"autobrake2", "On", CV_SAVE|CV_CALL, CV_OnOff, AutoBrake2_OnChange, 0, NULL, NULL, 0, 0, NULL};

typedef enum
{
	AXISNONE = 0,
	AXISTURN,
	AXISMOVE,
	AXISLOOK,
	AXISSTRAFE,
	AXISDEAD, //Axises that don't want deadzones
	AXISJUMP,
	AXISSPIN,
	AXISFIRE,
	AXISFIRENORMAL,
} axis_input_e;

consvar_t cv_turnaxis = {"joyaxis_turn", "X-Rudder", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_moveaxis = {"joyaxis_move", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_sideaxis = {"joyaxis_side", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_lookaxis = {"joyaxis_look", "Y-Rudder-", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_jumpaxis = {"joyaxis_jump", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_spinaxis = {"joyaxis_spin", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_fireaxis = {"joyaxis_fire", "Z-Axis-", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_firenaxis = {"joyaxis_firenormal", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_turnaxis2 = {"joyaxis2_turn", "X-Rudder", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_moveaxis2 = {"joyaxis2_move", "Y-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_sideaxis2 = {"joyaxis2_side", "X-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_lookaxis2 = {"joyaxis2_look", "Y-Rudder-", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_jumpaxis2 = {"joyaxis2_jump", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_spinaxis2 = {"joyaxis2_spin", "None", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_fireaxis2 = {"joyaxis2_fire", "Z-Axis-", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_firenaxis2 = {"joyaxis2_firenormal", "Z-Axis", CV_SAVE, joyaxis_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

#ifdef SEENAMES
player_t *seenplayer; // player we're aiming at right now
#endif

// now automatically allocated in D_RegisterClientCommands
// so that it doesn't have to be updated depending on the value of MAXPLAYERS
char player_names[MAXPLAYERS][MAXPLAYERNAME+1];

INT16 rw_maximums[NUM_WEAPONS] =
{
	800, // MAX_INFINITY
	400, // MAX_AUTOMATIC
	100, // MAX_BOUNCE
	50,  // MAX_SCATTER
	100, // MAX_GRENADE
	50,  // MAX_EXPLOSION
	50   // MAX_RAIL
};

// Allocation for time and nights data
void G_AllocMainRecordData(INT16 i)
{
	if (!mainrecords[i])
		mainrecords[i] = Z_Malloc(sizeof(recorddata_t), PU_STATIC, NULL);
	memset(mainrecords[i], 0, sizeof(recorddata_t));
}

void G_AllocNightsRecordData(INT16 i)
{
	if (!nightsrecords[i])
		nightsrecords[i] = Z_Malloc(sizeof(nightsdata_t), PU_STATIC, NULL);
	memset(nightsrecords[i], 0, sizeof(nightsdata_t));
}

// MAKE SURE YOU SAVE DATA BEFORE CALLING THIS
void G_ClearRecords(void)
{
	INT16 i;
	for (i = 0; i < NUMMAPS; ++i)
	{
		if (mainrecords[i])
		{
			Z_Free(mainrecords[i]);
			mainrecords[i] = NULL;
		}
		if (nightsrecords[i])
		{
			Z_Free(nightsrecords[i]);
			nightsrecords[i] = NULL;
		}
	}
}

// For easy retrieval of records
UINT32 G_GetBestScore(INT16 map)
{
	if (!mainrecords[map-1])
		return 0;

	return mainrecords[map-1]->score;
}

tic_t G_GetBestTime(INT16 map)
{
	if (!mainrecords[map-1] || mainrecords[map-1]->time <= 0)
		return (tic_t)UINT32_MAX;

	return mainrecords[map-1]->time;
}

UINT16 G_GetBestRings(INT16 map)
{
	if (!mainrecords[map-1])
		return 0;

	return mainrecords[map-1]->rings;
}

UINT32 G_GetBestNightsScore(INT16 map, UINT8 mare)
{
	if (!nightsrecords[map-1])
		return 0;

	return nightsrecords[map-1]->score[mare];
}

tic_t G_GetBestNightsTime(INT16 map, UINT8 mare)
{
	if (!nightsrecords[map-1] || nightsrecords[map-1]->time[mare] <= 0)
		return (tic_t)UINT32_MAX;

	return nightsrecords[map-1]->time[mare];
}

UINT8 G_GetBestNightsGrade(INT16 map, UINT8 mare)
{
	if (!nightsrecords[map-1])
		return 0;

	return nightsrecords[map-1]->grade[mare];
}

// For easy adding of NiGHTS records
void G_AddTempNightsRecords(UINT32 pscore, tic_t ptime, UINT8 mare)
{
	ntemprecords.score[mare] = pscore;
	ntemprecords.grade[mare] = P_GetGrade(pscore, gamemap, mare - 1);
	ntemprecords.time[mare] = ptime;

	// Update nummares
	// Note that mare "0" is overall, mare "1" is the first real mare
	if (ntemprecords.nummares < mare)
		ntemprecords.nummares = mare;
}

//
// G_UpdateRecordReplays
//
// Update replay files/data, etc. for Record Attack
// See G_SetNightsRecords for NiGHTS Attack.
//
static void G_UpdateRecordReplays(void)
{
	const size_t glen = strlen(srb2home)+1+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char *gpath;
	char lastdemo[256], bestdemo[256];
	UINT8 earnedEmblems;

	// Record new best time
	if (!mainrecords[gamemap-1])
		G_AllocMainRecordData(gamemap-1);

	if (players[consoleplayer].score > mainrecords[gamemap-1]->score)
		mainrecords[gamemap-1]->score = players[consoleplayer].score;

	if ((mainrecords[gamemap-1]->time == 0) || (players[consoleplayer].realtime < mainrecords[gamemap-1]->time))
		mainrecords[gamemap-1]->time = players[consoleplayer].realtime;

	if ((UINT16)(players[consoleplayer].rings) > mainrecords[gamemap-1]->rings)
		mainrecords[gamemap-1]->rings = (UINT16)(players[consoleplayer].rings);

	// Save demo!
	bestdemo[255] = '\0';
	lastdemo[255] = '\0';
	G_SetDemoTime(players[consoleplayer].realtime, players[consoleplayer].score, (UINT16)(players[consoleplayer].rings));
	G_CheckDemoStatus();

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));
	snprintf(lastdemo, 255, "%s-%s-last.lmp", gpath, skins[cv_chooseskin.value-1].name);

	if (FIL_FileExists(lastdemo))
	{
		UINT8 *buf;
		size_t len = FIL_ReadFile(lastdemo, &buf);

		snprintf(bestdemo, 255, "%s-%s-time-best.lmp", gpath, skins[cv_chooseskin.value-1].name);
		if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & 1)
		{ // Better time, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD TIME!"), M_GetText("Saved replay as"), bestdemo);
		}

		snprintf(bestdemo, 255, "%s-%s-score-best.lmp", gpath, skins[cv_chooseskin.value-1].name);
		if (!FIL_FileExists(bestdemo) || (G_CmpDemoTime(bestdemo, lastdemo) & (1<<1)))
		{ // Better score, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW HIGH SCORE!"), M_GetText("Saved replay as"), bestdemo);
		}

		snprintf(bestdemo, 255, "%s-%s-rings-best.lmp", gpath, skins[cv_chooseskin.value-1].name);
		if (!FIL_FileExists(bestdemo) || (G_CmpDemoTime(bestdemo, lastdemo) & (1<<2)))
		{ // Better rings, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW MOST RINGS!"), M_GetText("Saved replay as"), bestdemo);
		}

		//CONS_Printf("%s '%s'\n", M_GetText("Saved replay as"), lastdemo);

		Z_Free(buf);
	}
	free(gpath);

	// Check emblems when level data is updated
	if ((earnedEmblems = M_CheckLevelEmblems()))
		CONS_Printf(M_GetText("\x82" "Earned %hu emblem%s for Record Attack records.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");

	// Update timeattack menu's replay availability.
	Nextmap_OnChange();
}

void G_SetNightsRecords(void)
{
	INT32 i;
	UINT32 totalscore = 0;
	tic_t totaltime = 0;
	UINT8 earnedEmblems;

	const size_t glen = strlen(srb2home)+1+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char *gpath;
	char lastdemo[256], bestdemo[256];

	if (!ntemprecords.nummares)
		return;

	// Set overall
	{
		UINT8 totalrank = 0, realrank = 0;

		for (i = 1; i <= ntemprecords.nummares; ++i)
		{
			totalscore += ntemprecords.score[i];
			totalrank += ntemprecords.grade[i];
			totaltime += ntemprecords.time[i];
		}

		// Determine overall grade
		realrank = (UINT8)((FixedDiv((fixed_t)totalrank << FRACBITS, ntemprecords.nummares << FRACBITS) + (FRACUNIT/2)) >> FRACBITS);

		// You need ALL rainbow As to get a rainbow A overall
		if (realrank == GRADE_S && (totalrank / ntemprecords.nummares) != GRADE_S)
			realrank = GRADE_A;

		ntemprecords.score[0] = totalscore;
		ntemprecords.grade[0] = realrank;
		ntemprecords.time[0] = totaltime;
	}

	// Now take all temp records and put them in the actual records
	{
		nightsdata_t *maprecords;

		if (!nightsrecords[gamemap-1])
			G_AllocNightsRecordData(gamemap-1);
		maprecords = nightsrecords[gamemap-1];

		if (maprecords->nummares != ntemprecords.nummares)
			maprecords->nummares = ntemprecords.nummares;

		for (i = 0; i < ntemprecords.nummares + 1; ++i)
		{
			if (maprecords->score[i] < ntemprecords.score[i])
				maprecords->score[i] = ntemprecords.score[i];
			if (maprecords->grade[i] < ntemprecords.grade[i])
				maprecords->grade[i] = ntemprecords.grade[i];
			if (!maprecords->time[i] || maprecords->time[i] > ntemprecords.time[i])
				maprecords->time[i] = ntemprecords.time[i];
		}
	}

	memset(&ntemprecords, 0, sizeof(nightsdata_t));

	// Save demo!
	bestdemo[255] = '\0';
	lastdemo[255] = '\0';
	G_SetDemoTime(totaltime, totalscore, 0);
	G_CheckDemoStatus();

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));
	snprintf(lastdemo, 255, "%s-last.lmp", gpath);

	if (FIL_FileExists(lastdemo))
	{
		UINT8 *buf;
		size_t len = FIL_ReadFile(lastdemo, &buf);

		snprintf(bestdemo, 255, "%s-time-best.lmp", gpath);
		if (!FIL_FileExists(bestdemo) || G_CmpDemoTime(bestdemo, lastdemo) & 1)
		{ // Better time, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW RECORD TIME!"), M_GetText("Saved replay as"), bestdemo);
		}

		snprintf(bestdemo, 255, "%s-score-best.lmp", gpath);
		if (!FIL_FileExists(bestdemo) || (G_CmpDemoTime(bestdemo, lastdemo) & (1<<1)))
		{ // Better score, save this demo.
			if (FIL_FileExists(bestdemo))
				remove(bestdemo);
			FIL_WriteFile(bestdemo, buf, len);
			CONS_Printf("\x83%s\x80 %s '%s'\n", M_GetText("NEW HIGH SCORE!"), M_GetText("Saved replay as"), bestdemo);
		}

		//CONS_Printf("%s '%s'\n", M_GetText("Saved replay as"), lastdemo);

		Z_Free(buf);
	}
	free(gpath);

	if ((earnedEmblems = M_CheckLevelEmblems()))
		CONS_Printf(M_GetText("\x82" "Earned %hu emblem%s for NiGHTS records.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");

	// If the mare count changed, this will update the score display
	Nextmap_OnChange();
}

// for consistency among messages: this modifies the game and removes savemoddata.
void G_SetGameModified(boolean silent)
{
	if (modifiedgame && !savemoddata)
		return;

	modifiedgame = true;
	savemoddata = false;

	if (!silent)
		CONS_Alert(CONS_NOTICE, M_GetText("Game must be restarted to record statistics.\n"));

	// If in record attack recording, cancel it.
	if (modeattacking)
		M_EndModeAttackRun();
}

/** Builds an original game map name from a map number.
  * The complexity is due to MAPA0-MAPZZ.
  *
  * \param map Map number.
  * \return Pointer to a static buffer containing the desired map name.
  * \sa M_MapNumber
  */
const char *G_BuildMapName(INT32 map)
{
	static char mapname[10] = "MAPXX"; // internal map name (wad resource name)

	I_Assert(map > 0);
	I_Assert(map <= NUMMAPS);

	if (map < 100)
		sprintf(&mapname[3], "%.2d", map);
	else
	{
		mapname[3] = (char)('A' + (char)((map - 100) / 36));
		if ((map - 100) % 36 < 10)
			mapname[4] = (char)('0' + (char)((map - 100) % 36));
		else
			mapname[4] = (char)('A' + (char)((map - 100) % 36) - 10);
		mapname[5] = '\0';
	}

	return mapname;
}

/** Clips the console player's mouse aiming to the current view.
  * Used whenever the player view is changed manually.
  *
  * \param aiming Pointer to the vertical angle to clip.
  * \return Short version of the clipped angle for building a ticcmd.
  */
INT16 G_ClipAimingPitch(INT32 *aiming)
{
	INT32 limitangle;

	limitangle = ANGLE_90 - 1;

	if (*aiming > limitangle)
		*aiming = limitangle;
	else if (*aiming < -limitangle)
		*aiming = -limitangle;

	return (INT16)((*aiming)>>16);
}

INT16 G_SoftwareClipAimingPitch(INT32 *aiming)
{
	INT32 limitangle;

	// note: the current software mode implementation doesn't have true perspective
	limitangle = ANGLE_90 - ANG10; // Some viewing fun, but not too far down...

	if (*aiming > limitangle)
		*aiming = limitangle;
	else if (*aiming < -limitangle)
		*aiming = -limitangle;

	return (INT16)((*aiming)>>16);
}

static INT32 JoyAxis(axis_input_e axissel)
{
	INT32 retaxis;
	INT32 axisval;
	boolean flp = false;

	//find what axis to get
	switch (axissel)
	{
		case AXISTURN:
			axisval = cv_turnaxis.value;
			break;
		case AXISMOVE:
			axisval = cv_moveaxis.value;
			break;
		case AXISLOOK:
			axisval = cv_lookaxis.value;
			break;
		case AXISSTRAFE:
			axisval = cv_sideaxis.value;
			break;
		case AXISJUMP:
			axisval = cv_jumpaxis.value;
			break;
		case AXISSPIN:
			axisval = cv_spinaxis.value;
			break;
		case AXISFIRE:
			axisval = cv_fireaxis.value;
			break;
		case AXISFIRENORMAL:
			axisval = cv_firenaxis.value;
			break;
		default:
			return 0;
	}

	if (axisval < 0) //odd -axises
	{
		axisval = -axisval;
		flp = true;
	}
	if (axisval > JOYAXISSET*2 || axisval == 0) //not there in array or None
		return 0;

	if (axisval%2)
	{
		axisval /= 2;
		retaxis = joyxmove[axisval];
	}
	else
	{
		axisval--;
		axisval /= 2;
		retaxis = joyymove[axisval];
	}

	if (retaxis < (-JOYAXISRANGE))
		retaxis = -JOYAXISRANGE;
	if (retaxis > (+JOYAXISRANGE))
		retaxis = +JOYAXISRANGE;
	if (!Joystick.bGamepadStyle && axissel < AXISDEAD)
	{
		const INT32 jdeadzone = JOYAXISRANGE/4;
		if (-jdeadzone < retaxis && retaxis < jdeadzone)
			return 0;
	}
	if (flp) retaxis = -retaxis; //flip it around
	return retaxis;
}

static INT32 Joy2Axis(axis_input_e axissel)
{
	INT32 retaxis;
	INT32 axisval;
	boolean flp = false;

	//find what axis to get
	switch (axissel)
	{
		case AXISTURN:
			axisval = cv_turnaxis2.value;
			break;
		case AXISMOVE:
			axisval = cv_moveaxis2.value;
			break;
		case AXISLOOK:
			axisval = cv_lookaxis2.value;
			break;
		case AXISSTRAFE:
			axisval = cv_sideaxis2.value;
			break;
		case AXISJUMP:
			axisval = cv_jumpaxis2.value;
			break;
		case AXISSPIN:
			axisval = cv_spinaxis2.value;
			break;
		case AXISFIRE:
			axisval = cv_fireaxis2.value;
			break;
		case AXISFIRENORMAL:
			axisval = cv_firenaxis2.value;
			break;
		default:
			return 0;
	}


	if (axisval < 0) //odd -axises
	{
		axisval = -axisval;
		flp = true;
	}

	if (axisval > JOYAXISSET*2 || axisval == 0) //not there in array or None
		return 0;

	if (axisval%2)
	{
		axisval /= 2;
		retaxis = joy2xmove[axisval];
	}
	else
	{
		axisval--;
		axisval /= 2;
		retaxis = joy2ymove[axisval];
	}

	if (retaxis < (-JOYAXISRANGE))
		retaxis = -JOYAXISRANGE;
	if (retaxis > (+JOYAXISRANGE))
		retaxis = +JOYAXISRANGE;
	if (!Joystick2.bGamepadStyle && axissel < AXISDEAD)
	{
		const INT32 jdeadzone = JOYAXISRANGE/4;
		if (-jdeadzone < retaxis && retaxis < jdeadzone)
			return 0;
	}
	if (flp) retaxis = -retaxis; //flip it around
	return retaxis;
}


//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
// set secondaryplayer true to build player 2's ticcmd in splitscreen mode
//
INT32 localaiming, localaiming2;
angle_t localangle, localangle2;

static fixed_t forwardmove[2] = {25<<FRACBITS>>16, 50<<FRACBITS>>16};
static fixed_t sidemove[2] = {25<<FRACBITS>>16, 50<<FRACBITS>>16}; // faster!
static fixed_t angleturn[3] = {640, 1280, 320}; // + slow turn

void G_BuildTiccmd(ticcmd_t *cmd, INT32 realtics)
{
	boolean forcestrafe = false;
	boolean forcefullinput = false;
	INT32 tspeed, forward, side, axis, altaxis, i;
	const INT32 speed = 1;
	// these ones used for multiple conditions
	boolean turnleft, turnright, strafelkey, straferkey, movefkey, movebkey, mouseaiming, analogjoystickmove, gamepadjoystickmove, thisjoyaiming;
	player_t *player = &players[consoleplayer];
	camera_t *thiscam = &camera;

	static INT32 turnheld; // for accelerative turning
	static boolean keyboard_look; // true if lookup/down using keyboard
	static boolean resetdown; // don't cam reset every frame
	static boolean joyaiming; // check the last frame's value if we need to reset the camera

	G_CopyTiccmd(cmd, I_BaseTiccmd(), 1); // empty, or external driver

	// why build a ticcmd if we're paused?
	// Or, for that matter, if we're being reborn.
	// ...OR if we're blindfolded. No looking into the floor.
	if (paused || P_AutoPause() || (gamestate == GS_LEVEL && (player->playerstate == PST_REBORN || ((gametyperules & GTR_TAG)
	&& (leveltime < hidetime * TICRATE) && (player->pflags & PF_TAGIT)))))
	{
		cmd->angleturn = (INT16)(localangle >> 16);
		cmd->aiming = G_ClipAimingPitch(&localaiming);
		return;
	}

	turnright = PLAYER1INPUTDOWN(gc_turnright);
	turnleft = PLAYER1INPUTDOWN(gc_turnleft);

	straferkey = PLAYER1INPUTDOWN(gc_straferight);
	strafelkey = PLAYER1INPUTDOWN(gc_strafeleft);
	movefkey = PLAYER1INPUTDOWN(gc_forward);
	movebkey = PLAYER1INPUTDOWN(gc_backward);

	mouseaiming = (PLAYER1INPUTDOWN(gc_mouseaiming)) ^
		((cv_chasecam.value && !player->spectator) ? cv_chasefreelook.value : cv_alwaysfreelook.value);
	analogjoystickmove = cv_usejoystick.value && !Joystick.bGamepadStyle;
	gamepadjoystickmove = cv_usejoystick.value && Joystick.bGamepadStyle;

	thisjoyaiming = (cv_chasecam.value && !player->spectator) ? cv_chasefreelook.value : cv_alwaysfreelook.value;

	// Reset the vertical look if we're no longer joyaiming
	if (!thisjoyaiming && joyaiming)
		localaiming = 0;
	joyaiming = thisjoyaiming;

	axis = JoyAxis(AXISTURN);
	if (gamepadjoystickmove && axis != 0)
	{
		turnright = turnright || (axis > 0);
		turnleft = turnleft || (axis < 0);
	}
	forward = side = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		turnheld += realtics;
	else
		turnheld = 0;

	if (turnheld < SLOWTURNTICS)
		tspeed = 2; // slow turn
	else
		tspeed = speed;

	// let movement keys cancel each other out
	if (cv_analog.value) // Analog
	{
		if (turnright)
			cmd->angleturn = (INT16)(cmd->angleturn - angleturn[tspeed]);
		if (turnleft)
			cmd->angleturn = (INT16)(cmd->angleturn + angleturn[tspeed]);
	}
	if (twodlevel
		|| (player->mo && (player->mo->flags2 & MF2_TWOD))
		|| (!demoplayback && (player->pflags & PF_SLIDING)))
			forcefullinput = true;
	if (twodlevel
		|| (player->mo && (player->mo->flags2 & MF2_TWOD))
		|| (!demoplayback && ((player->powers[pw_carry] == CR_NIGHTSMODE)
		|| (player->pflags & (PF_SLIDING|PF_FORCESTRAFE))))) // Analog
			forcestrafe = true;
	if (forcestrafe)
	{
		if (turnright)
			side += sidemove[speed];
		if (turnleft)
			side -= sidemove[speed];

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			side += ((axis * sidemove[1]) >> 10);
		}
	}
	else if (cv_analog.value) // Analog
	{
		if (turnright)
			cmd->buttons |= BT_CAMRIGHT;
		if (turnleft)
			cmd->buttons |= BT_CAMLEFT;
	}
	else
	{
		if (turnright)
			cmd->angleturn = (INT16)(cmd->angleturn - angleturn[tspeed]);
		else if (turnleft)
			cmd->angleturn = (INT16)(cmd->angleturn + angleturn[tspeed]);

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE should be 1023 (divide by 1024)
			cmd->angleturn = (INT16)(cmd->angleturn - ((axis * angleturn[1]) >> 10)); // ANALOG!
		}
	}

	axis = JoyAxis(AXISSTRAFE);
	if (gamepadjoystickmove && axis != 0)
	{
		if (axis < 0)
			side += sidemove[speed];
		else if (axis > 0)
			side -= sidemove[speed];
	}
	else if (analogjoystickmove && axis != 0)
	{
		// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
		side += ((axis * sidemove[1]) >> 10);
	}

	// forward with key or button
	axis = JoyAxis(AXISMOVE);
	altaxis = JoyAxis(AXISLOOK);
	if (movefkey || (gamepadjoystickmove && axis < 0)
		|| ((player->powers[pw_carry] == CR_NIGHTSMODE)
			&& (PLAYER1INPUTDOWN(gc_lookup) || (gamepadjoystickmove && altaxis < 0))))
		forward = forwardmove[speed];
	if (movebkey || (gamepadjoystickmove && axis > 0)
		|| ((player->powers[pw_carry] == CR_NIGHTSMODE)
			&& (PLAYER1INPUTDOWN(gc_lookdown) || (gamepadjoystickmove && altaxis > 0))))
		forward -= forwardmove[speed];

	if (analogjoystickmove && axis != 0)
		forward -= ((axis * forwardmove[1]) >> 10); // ANALOG!

	// some people strafe left & right with mouse buttons
	// those people are weird
	if (straferkey)
		side += sidemove[speed];
	if (strafelkey)
		side -= sidemove[speed];

	if (PLAYER1INPUTDOWN(gc_weaponnext))
		cmd->buttons |= BT_WEAPONNEXT; // Next Weapon
	if (PLAYER1INPUTDOWN(gc_weaponprev))
		cmd->buttons |= BT_WEAPONPREV; // Previous Weapon

#if NUM_WEAPONS > 10
"Add extra inputs to g_input.h/gamecontrols_e"
#endif
	//use the four avaliable bits to determine the weapon.
	cmd->buttons &= ~BT_WEAPONMASK;
	for (i = 0; i < NUM_WEAPONS; ++i)
		if (PLAYER1INPUTDOWN(gc_wepslot1 + i))
		{
			cmd->buttons |= (UINT16)(i + 1);
			break;
		}

	// fire with any button/key
	axis = JoyAxis(AXISFIRE);
	if (PLAYER1INPUTDOWN(gc_fire) || (cv_usejoystick.value && axis > 0))
		cmd->buttons |= BT_ATTACK;

	// fire normal with any button/key
	axis = JoyAxis(AXISFIRENORMAL);
	if (PLAYER1INPUTDOWN(gc_firenormal) || (cv_usejoystick.value && axis > 0))
		cmd->buttons |= BT_FIRENORMAL;

	if (PLAYER1INPUTDOWN(gc_tossflag))
		cmd->buttons |= BT_TOSSFLAG;

	// Lua scriptable buttons
	if (PLAYER1INPUTDOWN(gc_custom1))
		cmd->buttons |= BT_CUSTOM1;
	if (PLAYER1INPUTDOWN(gc_custom2))
		cmd->buttons |= BT_CUSTOM2;
	if (PLAYER1INPUTDOWN(gc_custom3))
		cmd->buttons |= BT_CUSTOM3;

	// use with any button/key
	axis = JoyAxis(AXISSPIN);
	if (PLAYER1INPUTDOWN(gc_use) || (cv_usejoystick.value && axis > 0))
		cmd->buttons |= BT_USE;

	if (PLAYER1INPUTDOWN(gc_camreset))
	{
		if (camera.chase && !resetdown)
			P_ResetCamera(&players[displayplayer], &camera);
		resetdown = true;
	}
	else
		resetdown = false;

	// jump button
	axis = JoyAxis(AXISJUMP);
	if (PLAYER1INPUTDOWN(gc_jump) || (cv_usejoystick.value && axis > 0))
		cmd->buttons |= BT_JUMP;

	// player aiming shit, ahhhh...
	{
		INT32 player_invert = cv_invertmouse.value ? -1 : 1;
		INT32 screen_invert =
			(player->mo && (player->mo->eflags & MFE_VERTICALFLIP)
			 && (!camera.chase || player->pflags & PF_FLIPCAM)) //because chasecam's not inverted
			 ? -1 : 1; // set to -1 or 1 to multiply

		// mouse look stuff (mouse look is not the same as mouse aim)
		if (mouseaiming)
		{
			keyboard_look = false;

			// looking up/down
			localaiming += (mlooky<<19)*player_invert*screen_invert;
		}

		axis = JoyAxis(AXISLOOK);
		if (analogjoystickmove && joyaiming && axis != 0 && cv_lookaxis.value != 0)
			localaiming += (axis<<16) * screen_invert;

		// spring back if not using keyboard neither mouselookin'
		if (!keyboard_look && cv_lookaxis.value == 0 && !joyaiming && !mouseaiming)
			localaiming = 0;

		if (!(player->powers[pw_carry] == CR_NIGHTSMODE))
		{
			if (PLAYER1INPUTDOWN(gc_lookup) || (gamepadjoystickmove && axis < 0))
			{
				localaiming += KB_LOOKSPEED * screen_invert;
				keyboard_look = true;
			}
			else if (PLAYER1INPUTDOWN(gc_lookdown) || (gamepadjoystickmove && axis > 0))
			{
				localaiming -= KB_LOOKSPEED * screen_invert;
				keyboard_look = true;
			}
			else if (PLAYER1INPUTDOWN(gc_centerview))
				localaiming = 0;
		}

		// accept no mlook for network games
		if (!cv_allowmlook.value)
			localaiming = 0;

		cmd->aiming = G_ClipAimingPitch(&localaiming);
	}

	if (!mouseaiming && cv_mousemove.value)
		forward += mousey;

	if ((!demoplayback && (player->pflags & PF_SLIDING))) // Analog for mouse
		side += mousex*2;
	else if (cv_analog.value)
	{
		if (mousex)
		{
			if (mousex > 0)
				cmd->buttons |= BT_CAMRIGHT;
			else
				cmd->buttons |= BT_CAMLEFT;
		}
	}
	else
		cmd->angleturn = (INT16)(cmd->angleturn - (mousex*8));

	mousex = mousey = mlooky = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;
	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	// No additional acceleration when moving forward/backward and strafing simultaneously.
	// do this AFTER we cap to MAXPLMOVE so people can't find ways to cheese around this.
	if (!forcefullinput && forward && side)
	{
		angle_t angle = R_PointToAngle2(0, 0, side << FRACBITS, forward << FRACBITS);
		INT32 maxforward = abs(P_ReturnThrustY(NULL, angle, MAXPLMOVE));
		INT32 maxside = abs(P_ReturnThrustX(NULL, angle, MAXPLMOVE));
		forward = max(min(forward, maxforward), -maxforward);
		side = max(min(side, maxside), -maxside);
	}

	//Silly hack to make 2d mode *somewhat* playable with no chasecam.
	if ((twodlevel || (player->mo && player->mo->flags2 & MF2_TWOD)) && !camera.chase)
	{
		INT32 temp = forward;
		forward = side;
		side = temp;
	}

	cmd->forwardmove = (SINT8)(cmd->forwardmove + forward);
	cmd->sidemove = (SINT8)(cmd->sidemove + side);

	if (cv_analog.value) {
		if (player->awayviewtics)
			cmd->angleturn = (INT16)(player->awayviewmobj->angle >> 16);
		else
			cmd->angleturn = (INT16)(thiscam->angle >> 16);
	}
	else
	{
		localangle += (cmd->angleturn<<16);
		cmd->angleturn = (INT16)(localangle >> 16);
	}

	//Reset away view if a command is given.
	if ((cmd->forwardmove || cmd->sidemove || cmd->buttons)
		&& displayplayer != consoleplayer)
		displayplayer = consoleplayer;
}

// like the g_buildticcmd 1 but using mouse2, gamcontrolbis, ...
void G_BuildTiccmd2(ticcmd_t *cmd, INT32 realtics)
{
	boolean forcestrafe = false;
	boolean forcefullinput = false;
	INT32 tspeed, forward, side, axis, altaxis, i;
	const INT32 speed = 1;
	// these ones used for multiple conditions
	boolean turnleft, turnright, strafelkey, straferkey, movefkey, movebkey, mouseaiming, analogjoystickmove, gamepadjoystickmove, thisjoyaiming;
	player_t *player = &players[secondarydisplayplayer];
	camera_t *thiscam = (player->bot == 2 ? &camera : &camera2);

	static INT32 turnheld; // for accelerative turning
	static boolean keyboard_look; // true if lookup/down using keyboard
	static boolean resetdown; // don't cam reset every frame
	static boolean joyaiming; // check the last frame's value if we need to reset the camera

	G_CopyTiccmd(cmd,  I_BaseTiccmd2(), 1); // empty, or external driver

	//why build a ticcmd if we're paused?
	// Or, for that matter, if we're being reborn.
	if (paused || P_AutoPause() || player->playerstate == PST_REBORN)
	{
		cmd->angleturn = (INT16)(localangle2 >> 16);
		cmd->aiming = G_ClipAimingPitch(&localaiming2);
		return;
	}

	turnright = PLAYER2INPUTDOWN(gc_turnright);
	turnleft = PLAYER2INPUTDOWN(gc_turnleft);

	straferkey = PLAYER2INPUTDOWN(gc_straferight);
	strafelkey = PLAYER2INPUTDOWN(gc_strafeleft);
	movefkey = PLAYER2INPUTDOWN(gc_forward);
	movebkey = PLAYER2INPUTDOWN(gc_backward);

	mouseaiming = (PLAYER2INPUTDOWN(gc_mouseaiming)) ^
		((cv_chasecam2.value && !player->spectator) ? cv_chasefreelook2.value : cv_alwaysfreelook2.value);
	analogjoystickmove = cv_usejoystick2.value && !Joystick2.bGamepadStyle;
	gamepadjoystickmove = cv_usejoystick2.value && Joystick2.bGamepadStyle;

	thisjoyaiming = (cv_chasecam2.value && !player->spectator) ? cv_chasefreelook2.value : cv_alwaysfreelook2.value;

	// Reset the vertical look if we're no longer joyaiming
	if (!thisjoyaiming && joyaiming)
		localaiming2 = 0;
	joyaiming = thisjoyaiming;

	axis = Joy2Axis(AXISTURN);
	if (gamepadjoystickmove && axis != 0)
	{
		turnright = turnright || (axis > 0);
		turnleft = turnleft || (axis < 0);
	}
	forward = side = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (turnleft || turnright)
		turnheld += realtics;
	else
		turnheld = 0;

	if (turnheld < SLOWTURNTICS)
		tspeed = 2; // slow turn
	else
		tspeed = speed;

	// let movement keys cancel each other out
	if (cv_analog2.value) // Analog
	{
		if (turnright)
			cmd->angleturn = (INT16)(cmd->angleturn - angleturn[tspeed]);
		if (turnleft)
			cmd->angleturn = (INT16)(cmd->angleturn + angleturn[tspeed]);
	}
	if (twodlevel
		|| (player->mo && (player->mo->flags2 & MF2_TWOD))
		|| (!demoplayback && (player->pflags & PF_SLIDING)))
			forcefullinput = true;
	if (twodlevel
		|| (player->mo && (player->mo->flags2 & MF2_TWOD))
		|| player->climbing
		|| (player->powers[pw_carry] == CR_NIGHTSMODE)
		|| (player->pflags & (PF_SLIDING|PF_FORCESTRAFE))) // Analog
			forcestrafe = true;
	if (forcestrafe) // Analog
	{
		if (turnright)
			side += sidemove[speed];
		if (turnleft)
			side -= sidemove[speed];

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
			side += ((axis * sidemove[1]) >> 10);
		}
	}
	else if (cv_analog2.value) // Analog
	{
		if (turnright)
			cmd->buttons |= BT_CAMRIGHT;
		if (turnleft)
			cmd->buttons |= BT_CAMLEFT;
	}
	else
	{
		if (turnright)
			cmd->angleturn = (INT16)(cmd->angleturn - angleturn[tspeed]);
		else if (turnleft)
			cmd->angleturn = (INT16)(cmd->angleturn + angleturn[tspeed]);

		if (analogjoystickmove && axis != 0)
		{
			// JOYAXISRANGE should be 1023 (divide by 1024)
			cmd->angleturn = (INT16)(cmd->angleturn - ((axis * angleturn[1]) >> 10)); // ANALOG!
		}
	}

	axis = Joy2Axis(AXISSTRAFE);
	if (gamepadjoystickmove && axis != 0)
	{
		if (axis < 0)
			side += sidemove[speed];
		else if (axis > 0)
			side -= sidemove[speed];
	}
	else if (analogjoystickmove && axis != 0)
	{
		// JOYAXISRANGE is supposed to be 1023 (divide by 1024)
		side += ((axis * sidemove[1]) >> 10);
	}

	// forward with key or button
	axis = Joy2Axis(AXISMOVE);
	altaxis = Joy2Axis(AXISLOOK);
	if (movefkey || (gamepadjoystickmove && axis < 0)
		|| ((player->powers[pw_carry] == CR_NIGHTSMODE)
			&& (PLAYER2INPUTDOWN(gc_lookup) || (gamepadjoystickmove && altaxis < 0))))
		forward = forwardmove[speed];
	if (movebkey || (gamepadjoystickmove && axis > 0)
		|| ((player->powers[pw_carry] == CR_NIGHTSMODE)
			&& (PLAYER2INPUTDOWN(gc_lookdown) || (gamepadjoystickmove && altaxis > 0))))
		forward -= forwardmove[speed];

	if (analogjoystickmove && axis != 0)
		forward -= ((axis * forwardmove[1]) >> 10); // ANALOG!

	// some people strafe left & right with mouse buttons
	// those people are (still) weird
	if (straferkey)
		side += sidemove[speed];
	if (strafelkey)
		side -= sidemove[speed];

	if (PLAYER2INPUTDOWN(gc_weaponnext))
		cmd->buttons |= BT_WEAPONNEXT; // Next Weapon
	if (PLAYER2INPUTDOWN(gc_weaponprev))
		cmd->buttons |= BT_WEAPONPREV; // Previous Weapon

	//use the four avaliable bits to determine the weapon.
	cmd->buttons &= ~BT_WEAPONMASK;
	for (i = 0; i < NUM_WEAPONS; ++i)
		if (PLAYER2INPUTDOWN(gc_wepslot1 + i))
		{
			cmd->buttons |= (UINT16)(i + 1);
			break;
		}

	// fire with any button/key
	axis = Joy2Axis(AXISFIRE);
	if (PLAYER2INPUTDOWN(gc_fire) || (cv_usejoystick2.value && axis > 0))
		cmd->buttons |= BT_ATTACK;

	// fire normal with any button/key
	axis = Joy2Axis(AXISFIRENORMAL);
	if (PLAYER2INPUTDOWN(gc_firenormal) || (cv_usejoystick2.value && axis > 0))
		cmd->buttons |= BT_FIRENORMAL;

	if (PLAYER2INPUTDOWN(gc_tossflag))
		cmd->buttons |= BT_TOSSFLAG;

	// Lua scriptable buttons
	if (PLAYER2INPUTDOWN(gc_custom1))
		cmd->buttons |= BT_CUSTOM1;
	if (PLAYER2INPUTDOWN(gc_custom2))
		cmd->buttons |= BT_CUSTOM2;
	if (PLAYER2INPUTDOWN(gc_custom3))
		cmd->buttons |= BT_CUSTOM3;

	// use with any button/key
	axis = Joy2Axis(AXISSPIN);
	if (PLAYER2INPUTDOWN(gc_use) || (cv_usejoystick2.value && axis > 0))
		cmd->buttons |= BT_USE;

	if (PLAYER2INPUTDOWN(gc_camreset))
	{
		if (camera2.chase && !resetdown)
			P_ResetCamera(&players[secondarydisplayplayer], &camera2);
		resetdown = true;
	}
	else
		resetdown = false;

	// jump button
	axis = Joy2Axis(AXISJUMP);
	if (PLAYER2INPUTDOWN(gc_jump) || (cv_usejoystick2.value && axis > 0))
		cmd->buttons |= BT_JUMP;

	// player aiming shit, ahhhh...
	{
		INT32 player_invert = cv_invertmouse2.value ? -1 : 1;
		INT32 screen_invert =
			(player->mo && (player->mo->eflags & MFE_VERTICALFLIP)
			 && (!camera2.chase || player->pflags & PF_FLIPCAM)) //because chasecam's not inverted
			 ? -1 : 1; // set to -1 or 1 to multiply

		// mouse look stuff (mouse look is not the same as mouse aim)
		if (mouseaiming)
		{
			keyboard_look = false;

			// looking up/down
			localaiming2 += (mlook2y<<19)*player_invert*screen_invert;
		}

		axis = Joy2Axis(AXISLOOK);
		if (analogjoystickmove && joyaiming && axis != 0 && cv_lookaxis2.value != 0)
			localaiming2 += (axis<<16) * screen_invert;

		// spring back if not using keyboard neither mouselookin'
		if (!keyboard_look && cv_lookaxis2.value == 0 && !joyaiming && !mouseaiming)
			localaiming2 = 0;

		if (!(player->powers[pw_carry] == CR_NIGHTSMODE))
		{
			if (PLAYER2INPUTDOWN(gc_lookup) || (gamepadjoystickmove && axis < 0))
			{
				localaiming2 += KB_LOOKSPEED * screen_invert;
				keyboard_look = true;
			}
			else if (PLAYER2INPUTDOWN(gc_lookdown) || (gamepadjoystickmove && axis > 0))
			{
				localaiming2 -= KB_LOOKSPEED * screen_invert;
				keyboard_look = true;
			}
			else if (PLAYER2INPUTDOWN(gc_centerview))
				localaiming2 = 0;
		}

		// accept no mlook for network games
		if (!cv_allowmlook.value)
			localaiming2 = 0;

		cmd->aiming = G_ClipAimingPitch(&localaiming2);
	}

	if (!mouseaiming && cv_mousemove2.value)
		forward += mouse2y;

	if (player->climbing
		|| (player->pflags & PF_SLIDING)) // Analog for mouse
		side += mouse2x*2;
	else if (cv_analog2.value)
	{
		if (mouse2x)
		{
			if (mouse2x > 0)
				cmd->buttons |= BT_CAMRIGHT;
			else
				cmd->buttons |= BT_CAMLEFT;
		}
	}
	else
		cmd->angleturn = (INT16)(cmd->angleturn - (mouse2x*8));

	mouse2x = mouse2y = mlook2y = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;
	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	// No additional acceleration when moving forward/backward and strafing simultaneously.
	// do this AFTER we cap to MAXPLMOVE so people can't find ways to cheese around this.
	if (!forcefullinput && forward && side)
	{
		angle_t angle = R_PointToAngle2(0, 0, side << FRACBITS, forward << FRACBITS);
		INT32 maxforward = abs(P_ReturnThrustY(NULL, angle, MAXPLMOVE));
		INT32 maxside = abs(P_ReturnThrustX(NULL, angle, MAXPLMOVE));
		forward = max(min(forward, maxforward), -maxforward);
		side = max(min(side, maxside), -maxside);
	}

	//Silly hack to make 2d mode *somewhat* playable with no chasecam.
	if ((twodlevel || (player->mo && player->mo->flags2 & MF2_TWOD)) && !camera2.chase)
	{
		INT32 temp = forward;
		forward = side;
		side = temp;
	}

	cmd->forwardmove = (SINT8)(cmd->forwardmove + forward);
	cmd->sidemove = (SINT8)(cmd->sidemove + side);

	if (player->bot == 1) {
		if (!player->powers[pw_tailsfly] && (cmd->forwardmove || cmd->sidemove || cmd->buttons))
		{
			player->bot = 2; // A player-controlled bot. Returns to AI when it respawns.
			CV_SetValue(&cv_analog2, true);
		}
		else
		{
			G_CopyTiccmd(cmd,  I_BaseTiccmd2(), 1); // empty, or external driver
			B_BuildTiccmd(player, cmd);
		}
	}

	if (cv_analog2.value) {
		if (player->awayviewtics)
			cmd->angleturn = (INT16)(player->awayviewmobj->angle >> 16);
		else
			cmd->angleturn = (INT16)(thiscam->angle >> 16);
	}
	else
	{
		localangle2 += (cmd->angleturn<<16);
		cmd->angleturn = (INT16)(localangle2 >> 16);
	}
}

// User has designated that they want
// analog ON, so tell the game to stop
// fudging with it.
static void UserAnalog_OnChange(void)
{
	if (cv_useranalog.value)
		CV_SetValue(&cv_analog, 1);
	else
		CV_SetValue(&cv_analog, 0);
}

static void UserAnalog2_OnChange(void)
{
	if (botingame)
		return;
	if (cv_useranalog2.value)
		CV_SetValue(&cv_analog2, 1);
	else
		CV_SetValue(&cv_analog2, 0);
}

static void Analog_OnChange(void)
{
	if (!cv_cam_dist.string)
		return;

	// cameras are not initialized at this point

	if (!cv_chasecam.value && cv_analog.value) {
		CV_SetValue(&cv_analog, 0);
		return;
	}

	SendWeaponPref();
}

static void Analog2_OnChange(void)
{
	if (!(splitscreen || botingame) || !cv_cam2_dist.string)
		return;

	// cameras are not initialized at this point

	if (!cv_chasecam2.value && cv_analog2.value) {
		CV_SetValue(&cv_analog2, 0);
		return;
	}

	SendWeaponPref2();
}

static void DirectionChar_OnChange(void)
{
	SendWeaponPref();
}

static void DirectionChar2_OnChange(void)
{
	SendWeaponPref2();
}

static void AutoBrake_OnChange(void)
{
	SendWeaponPref();
}

static void AutoBrake2_OnChange(void)
{
	SendWeaponPref2();
}

//
// G_DoLoadLevel
//
void G_DoLoadLevel(boolean resetplayer)
{
	INT32 i;

	// Make sure objectplace is OFF when you first start the level!
	OP_ResetObjectplace();
	demosynced = true;

	levelstarttic = gametic; // for time calculation

	if (wipegamestate == GS_LEVEL)
		wipegamestate = -1; // force a wipe

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();

	// cleanup
	if (titlemapinaction == TITLEMAP_LOADING)
	{
		if (W_CheckNumForName(G_BuildMapName(gamemap)) == LUMPERROR)
		{
			titlemap = 0; // let's not infinite recursion ok
			Command_ExitGame_f();
			return;
		}

		titlemapinaction = TITLEMAP_RUNNING;
	}
	else
		titlemapinaction = TITLEMAP_OFF;

	G_SetGamestate(GS_LEVEL);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (resetplayer || (playeringame[i] && players[i].playerstate == PST_DEAD))
			players[i].playerstate = PST_REBORN;
	}

	// Setup the level.
	if (!P_SetupLevel(false)) // this never returns false?
	{
		// fail so reset game stuff
		Command_ExitGame_f();
		return;
	}

	if (!resetplayer)
		P_FindEmerald();

	displayplayer = consoleplayer; // view the guy you are playing
	if (!splitscreen && !botingame)
		secondarydisplayplayer = consoleplayer;

	gameaction = ga_nothing;
#ifdef PARANOIA
	Z_CheckHeap(-2);
#endif

	if (camera.chase)
		P_ResetCamera(&players[displayplayer], &camera);
	if (camera2.chase && splitscreen)
		P_ResetCamera(&players[secondarydisplayplayer], &camera2);

	// clear cmd building stuff
	memset(gamekeydown, 0, sizeof (gamekeydown));
	for (i = 0;i < JOYAXISSET; i++)
	{
		joyxmove[i] = joyymove[i] = 0;
		joy2xmove[i] = joy2ymove[i] = 0;
	}
	mousex = mousey = 0;
	mouse2x = mouse2y = 0;

	// clear hud messages remains (usually from game startup)
	CON_ClearHUD();
}

//
// Start the title card.
//
void G_StartTitleCard(void)
{
	wipestyleflags |= WSF_FADEIN;
	wipestyleflags &= ~WSF_FADEOUT;

	// The title card has been disabled for this map.
	// Oh well.
	if ((mapheaderinfo[gamemap-1]->levelflags & LF_NOTITLECARD) || (gametyperules & GTR_NOTITLECARD))
	{
		WipeStageTitle = false;
		return;
	}

	// clear the hud
	CON_ClearHUD();

	// prepare status bar
	ST_startTitleCard();

	// start the title card
	WipeStageTitle = (!titlemapinaction);
}

//
// Run the title card before fading in to the level.
//
void G_PreLevelTitleCard(void)
{
	tic_t starttime = I_GetTime();
	tic_t endtime = starttime + (PRELEVELTIME*NEWTICRATERATIO);
	tic_t nowtime = starttime;
	tic_t lasttime = starttime;
	while (nowtime < endtime)
	{
		// draw loop
		while (!((nowtime = I_GetTime()) - lasttime))
			I_Sleep();
		lasttime = nowtime;

		ST_runTitleCard();
		ST_preLevelTitleCardDrawer();
		I_FinishUpdate(); // page flip or blit buffer

		if (moviemode)
			M_SaveFrame();
		if (takescreenshot) // Only take screenshots after drawing.
			M_DoScreenShot();
	}
}

INT32 pausedelay = 0;
boolean pausebreakkey = false;
static INT32 camtoggledelay, camtoggledelay2 = 0;

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
boolean G_Responder(event_t *ev)
{
	// any other key pops up menu if in demos
	if (gameaction == ga_nothing && !singledemo &&
		((demoplayback && !modeattacking && !titledemo) || gamestate == GS_TITLESCREEN))
	{
		if (ev->type == ev_keydown && ev->data1 != 301 && !(gamestate == GS_TITLESCREEN && finalecount < TICRATE))
		{
			M_StartControlPanel();
			return true;
		}
		return false;
	}
	else if (demoplayback && titledemo)
	{
		// Title demo uses intro responder
		if (F_IntroResponder(ev))
		{
			// stop the title demo
			G_CheckDemoStatus();
			return true;
		}
		return false;
	}

	if (gamestate == GS_LEVEL)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event
		if (AM_Responder(ev))
			return true; // automap ate it
		// map the event (key/mouse/joy) to a gamecontrol
	}
	// Intro
	else if (gamestate == GS_INTRO)
	{
		if (F_IntroResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}
	else if (gamestate == GS_CUTSCENE)
	{
		if (HU_Responder(ev))
			return true; // chat ate the event

		if (F_CutsceneResponder(ev))
		{
			D_StartTitle();
			return true;
		}
	}
	else if (gamestate == GS_CREDITS || gamestate == GS_ENDING) // todo: keep ending here?
	{
		if (HU_Responder(ev))
			return true; // chat ate the event

		if (F_CreditResponder(ev))
		{
			// Skip credits for everyone
			if (!netgame || server || IsPlayerAdmin(consoleplayer))
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			return true;
		}
	}
	else if (gamestate == GS_CONTINUING)
	{
		if (F_ContinueResponder(ev))
			return true;
	}
	// Demo End
	else if (gamestate == GS_GAMEEND)
		return true;
	else if (gamestate == GS_INTERMISSION || gamestate == GS_EVALUATION)
		if (HU_Responder(ev))
			return true; // chat ate the event

	// allow spy mode changes even during the demo
	if (gamestate == GS_LEVEL && ev->type == ev_keydown
		&& (ev->data1 == KEY_F12 || ev->data1 == gamecontrol[gc_viewpoint][0] || ev->data1 == gamecontrol[gc_viewpoint][1]))
	{
		if (splitscreen || !netgame)
			displayplayer = consoleplayer;
		else
		{
			// spy mode
			do
			{
				displayplayer++;
				if (displayplayer == MAXPLAYERS)
					displayplayer = 0;

				if (!playeringame[displayplayer])
					continue;

				if (players[displayplayer].spectator)
					continue;

				if (G_GametypeHasTeams())
				{
					if (players[consoleplayer].ctfteam
					 && players[displayplayer].ctfteam != players[consoleplayer].ctfteam)
						continue;
				}
				else if (gametype == GT_HIDEANDSEEK)
				{
					if (players[consoleplayer].pflags & PF_TAGIT)
						continue;
				}
				// Other Tag-based gametypes?
				else if (G_TagGametype())
				{
					if (!players[consoleplayer].spectator
					 && (players[consoleplayer].pflags & PF_TAGIT) != (players[displayplayer].pflags & PF_TAGIT))
						continue;
				}
				else if (G_GametypeHasSpectators() && G_RingSlingerGametype())
				{
					if (!players[consoleplayer].spectator)
						continue;
				}

				break;
			} while (displayplayer != consoleplayer);

			// change statusbar also if playing back demo
			if (singledemo)
				ST_changeDemoView();

			// tell who's the view
			CONS_Printf(M_GetText("Viewpoint: %s\n"), player_names[displayplayer]);

			return true;
		}
	}

	// update keys current state
	G_MapEventsToControls(ev);

	switch (ev->type)
	{
		case ev_keydown:
			if (ev->data1 == gamecontrol[gc_pause][0]
				|| ev->data1 == gamecontrol[gc_pause][1]
				|| ev->data1 == KEY_PAUSE)
			{
				if (modeattacking && !demoplayback && (gamestate == GS_LEVEL))
				{
					pausebreakkey = (ev->data1 == KEY_PAUSE);
					if (menuactive || pausedelay < 0 || leveltime < 2)
						return true;

					if (pausedelay < 1+(NEWTICRATE/2))
						pausedelay = 1+(NEWTICRATE/2);
					else if (++pausedelay > 1+(NEWTICRATE/2)+(NEWTICRATE/3))
					{
						G_SetModeAttackRetryFlag();
						return true;
					}
					pausedelay++; // counteract subsequent subtraction this frame
				}
				else
				{
					INT32 oldpausedelay = pausedelay;
					pausedelay = (NEWTICRATE/7);
					if (!oldpausedelay)
					{
						// command will handle all the checks for us
						COM_ImmedExecute("pause");
						return true;
					}
				}
			}
			if (ev->data1 == gamecontrol[gc_camtoggle][0]
				|| ev->data1 == gamecontrol[gc_camtoggle][1])
			{
				if (!camtoggledelay)
				{
					camtoggledelay = NEWTICRATE / 7;
					CV_SetValue(&cv_chasecam, cv_chasecam.value ? 0 : 1);
				}
			}
			if (ev->data1 == gamecontrolbis[gc_camtoggle][0]
				|| ev->data1 == gamecontrolbis[gc_camtoggle][1])
			{
				if (!camtoggledelay2)
				{
					camtoggledelay2 = NEWTICRATE / 7;
					CV_SetValue(&cv_chasecam2, cv_chasecam2.value ? 0 : 1);
				}
			}
			return true;

		case ev_keyup:
			return false; // always let key up events filter down

		case ev_mouse:
			return true; // eat events

		case ev_joystick:
			return true; // eat events

		case ev_joystick2:
			return true; // eat events


		default:
			break;
	}

	return false;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(boolean run)
{
	UINT32 i;
	INT32 buf;

	P_MapStart();
	// do player reborns if needed
	if (gamestate == GS_LEVEL)
	{
		// Or, alternatively, retry.
		if (!(netgame || multiplayer) && G_GetRetryFlag())
		{
			G_ClearRetryFlag();

			if (modeattacking)
			{
				pausedelay = INT32_MIN;
				M_ModeAttackRetry(0);
			}
			else
			{
				// Costs a life to retry ... unless the player in question is dead already.
				if (G_GametypeUsesLives() && players[consoleplayer].playerstate == PST_LIVE && players[consoleplayer].lives != INFLIVES)
					players[consoleplayer].lives -= 1;

				G_DoReborn(consoleplayer);
			}
		}

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].playerstate == PST_REBORN)
				G_DoReborn(i);
	}
	P_MapEnd();

	// do things to change the game state
	while (gameaction != ga_nothing)
		switch (gameaction)
		{
			case ga_completed: G_DoCompleted(); break;
			case ga_startcont: G_DoStartContinue(); break;
			case ga_continued: G_DoContinued(); break;
			case ga_worlddone: G_DoWorldDone(); break;
			case ga_nothing: break;
			default: I_Error("gameaction = %d\n", gameaction);
		}

	buf = gametic % BACKUPTICS;

	// read/write demo and check turbo cheat
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			G_CopyTiccmd(&players[i].cmd, &netcmds[buf][i], 1);
	}

	// do main actions
	switch (gamestate)
	{
		case GS_LEVEL:
			if (titledemo)
				F_TitleDemoTicker();
			P_Ticker(run); // tic the game
			ST_Ticker(run);
			F_TextPromptTicker();
			AM_Ticker();
			HU_Ticker();
			break;

		case GS_INTERMISSION:
			if (run)
				Y_Ticker();
			HU_Ticker();
			break;

		case GS_TIMEATTACK:
			F_MenuPresTicker(run);
			break;

		case GS_INTRO:
			if (run)
				F_IntroTicker();
			break;

		case GS_ENDING:
			if (run)
				F_EndingTicker();
			HU_Ticker();
			break;

		case GS_CUTSCENE:
			if (run)
				F_CutsceneTicker();
			HU_Ticker();
			break;

		case GS_GAMEEND:
			if (run)
				F_GameEndTicker();
			break;

		case GS_EVALUATION:
			if (run)
				F_GameEvaluationTicker();
			HU_Ticker();
			break;

		case GS_CONTINUING:
			if (run)
				F_ContinueTicker();
			break;

		case GS_CREDITS:
			if (run)
				F_CreditTicker();
			HU_Ticker();
			break;

		case GS_TITLESCREEN:
			if (titlemapinaction) P_Ticker(run); // then intentionally fall through
			/* FALLTHRU */
		case GS_WAITINGPLAYERS:
			F_MenuPresTicker(run);
			F_TitleScreenTicker(run);
			break;

		case GS_DEDICATEDSERVER:
		case GS_NULL:
			break; // do nothing
	}

	if (run)
	{
		if (pausedelay && pausedelay != INT32_MIN)
		{
			if (pausedelay > 0)
				pausedelay--;
			else
				pausedelay++;
		}

		if (camtoggledelay)
			camtoggledelay--;

		if (camtoggledelay2)
			camtoggledelay2--;
	}
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_PlayerFinishLevel
// Called when a player completes a level.
//
static inline void G_PlayerFinishLevel(INT32 player)
{
	player_t *p;

	p = &players[player];

	memset(p->powers, 0, sizeof (p->powers));
	p->ringweapons = 0;

	p->mo->flags2 &= ~MF2_SHADOW; // cancel invisibility
	P_FlashPal(p, 0, 0); // Resets
	p->starpostscale = 0;
	p->starpostangle = 0;
	p->starposttime = 0;
	p->starpostx = 0;
	p->starposty = 0;
	p->starpostz = 0;
	p->starpostnum = 0;

	if (rendermode == render_soft)
		V_SetPaletteLump(GetPalette()); // Reset the palette
}

//
// G_PlayerReborn
// Called after a player dies. Almost everything is cleared and initialized.
//
void G_PlayerReborn(INT32 player, boolean betweenmaps)
{
	player_t *p;
	INT32 score;
	INT32 lives;
	INT32 continues;
	fixed_t camerascale;
	fixed_t shieldscale;
	UINT8 charability;
	UINT8 charability2;
	fixed_t normalspeed;
	fixed_t runspeed;
	UINT8 thrustfactor;
	UINT8 accelstart;
	UINT8 acceleration;
	INT32 charflags;
	INT32 pflags;
	UINT32 thokitem;
	UINT32 spinitem;
	UINT32 revitem;
	UINT32 followitem;
	fixed_t actionspd;
	fixed_t mindash;
	fixed_t maxdash;
	INT32 ctfteam;
	INT32 starposttime;
	INT16 starpostx;
	INT16 starposty;
	INT16 starpostz;
	INT32 starpostnum;
	INT32 starpostangle;
	fixed_t starpostscale;
	fixed_t jumpfactor;
	fixed_t height;
	fixed_t spinheight;
	INT32 exiting;
	INT16 numboxes;
	INT16 totalring;
	UINT8 laps;
	UINT8 mare;
	UINT8 skincolor;
	INT32 skin;
	UINT32 availabilities;
	tic_t jointime;
	boolean spectator;
	boolean outofcoop;
	INT16 bot;
	SINT8 pity;
	INT16 rings;
	INT16 spheres;

	score = players[player].score;
	lives = players[player].lives;
	continues = players[player].continues;
	ctfteam = players[player].ctfteam;
	exiting = players[player].exiting;
	jointime = players[player].jointime;
	spectator = players[player].spectator;
	outofcoop = players[player].outofcoop;
	pflags = (players[player].pflags & (PF_FLIPCAM|PF_ANALOGMODE|PF_DIRECTIONCHAR|PF_AUTOBRAKE|PF_TAGIT|PF_GAMETYPEOVER));

	if (!betweenmaps)
		pflags |= (players[player].pflags & PF_FINISHED);

	// As long as we're not in multiplayer, carry over cheatcodes from map to map
	if (!(netgame || multiplayer))
		pflags |= (players[player].pflags & (PF_GODMODE|PF_NOCLIP|PF_INVIS));

	numboxes = players[player].numboxes;
	laps = players[player].laps;
	totalring = players[player].totalring;

	skincolor = players[player].skincolor;
	skin = players[player].skin;
	availabilities = players[player].availabilities;
	camerascale = players[player].camerascale;
	shieldscale = players[player].shieldscale;
	charability = players[player].charability;
	charability2 = players[player].charability2;
	normalspeed = players[player].normalspeed;
	runspeed = players[player].runspeed;
	thrustfactor = players[player].thrustfactor;
	accelstart = players[player].accelstart;
	acceleration = players[player].acceleration;
	charflags = players[player].charflags;

	starposttime = players[player].starposttime;
	starpostx = players[player].starpostx;
	starposty = players[player].starposty;
	starpostz = players[player].starpostz;
	starpostnum = players[player].starpostnum;
	starpostangle = players[player].starpostangle;
	starpostscale = players[player].starpostscale;
	jumpfactor = players[player].jumpfactor;
	height = players[player].height;
	spinheight = players[player].spinheight;
	thokitem = players[player].thokitem;
	spinitem = players[player].spinitem;
	revitem = players[player].revitem;
	followitem = players[player].followitem;
	actionspd = players[player].actionspd;
	mindash = players[player].mindash;
	maxdash = players[player].maxdash;

	mare = players[player].mare;
	bot = players[player].bot;
	pity = players[player].pity;

	if (betweenmaps || !G_IsSpecialStage(gamemap))
	{
		rings = (ultimatemode ? 0 : mapheaderinfo[gamemap-1]->startrings);
		spheres = 0;
	}
	else
	{
		rings = players[player].rings;
		spheres = players[player].spheres;
	}

	p = &players[player];
	memset(p, 0, sizeof (*p));

	p->score = score;
	p->lives = lives;
	p->continues = continues;
	p->pflags = pflags;
	p->ctfteam = ctfteam;
	p->jointime = jointime;
	p->spectator = spectator;
	p->outofcoop = outofcoop;

	// save player config truth reborn
	p->skincolor = skincolor;
	p->skin = skin;
	p->availabilities = availabilities;
	p->camerascale = camerascale;
	p->shieldscale = shieldscale;
	p->charability = charability;
	p->charability2 = charability2;
	p->normalspeed = normalspeed;
	p->runspeed = runspeed;
	p->thrustfactor = thrustfactor;
	p->accelstart = accelstart;
	p->acceleration = acceleration;
	p->charflags = charflags;
	p->thokitem = thokitem;
	p->spinitem = spinitem;
	p->revitem = revitem;
	p->followitem = followitem;
	p->actionspd = actionspd;
	p->mindash = mindash;
	p->maxdash = maxdash;

	p->starposttime = starposttime;
	p->starpostx = starpostx;
	p->starposty = starposty;
	p->starpostz = starpostz;
	p->starpostnum = starpostnum;
	p->starpostangle = starpostangle;
	p->starpostscale = starpostscale;
	p->jumpfactor = jumpfactor;
	p->height = height;
	p->spinheight = spinheight;
	p->exiting = exiting;

	p->numboxes = numboxes;
	p->laps = laps;
	p->totalring = totalring;

	p->mare = mare;
	if (bot)
		p->bot = 1; // reset to AI-controlled
	p->pity = pity;
	p->rings = rings;
	p->spheres = spheres;

	// Don't do anything immediately
	p->pflags |= PF_USEDOWN;
	p->pflags |= PF_ATTACKDOWN;
	p->pflags |= PF_JUMPDOWN;

	p->playerstate = PST_LIVE;
	p->panim = PA_IDLE; // standing animation

	//if ((netgame || multiplayer) && !p->spectator) -- moved into P_SpawnPlayer to account for forced changes there
		//p->powers[pw_flashing] = flashingtics-1; // Babysitting deterrent

	// Check to make sure their color didn't change somehow...
	if (G_GametypeHasTeams())
	{
		if (p->ctfteam == 1 && p->skincolor != skincolor_redteam)
		{
			if (p == &players[consoleplayer])
				CV_SetValue(&cv_playercolor, skincolor_redteam);
			else if (p == &players[secondarydisplayplayer])
				CV_SetValue(&cv_playercolor2, skincolor_redteam);
		}
		else if (p->ctfteam == 2 && p->skincolor != skincolor_blueteam)
		{
			if (p == &players[consoleplayer])
				CV_SetValue(&cv_playercolor, skincolor_blueteam);
			else if (p == &players[secondarydisplayplayer])
				CV_SetValue(&cv_playercolor2, skincolor_blueteam);
		}
	}

	if (betweenmaps)
		return;

	if (p-players == consoleplayer)
	{
		if (mapmusflags & MUSIC_RELOADRESET)
		{
			strncpy(mapmusname, mapheaderinfo[gamemap-1]->musname, 7);
			mapmusname[6] = 0;
			mapmusflags = (mapheaderinfo[gamemap-1]->mustrack & MUSIC_TRACKMASK);
			mapmusposition = mapheaderinfo[gamemap-1]->muspos;
		}

		// This is in S_Start, but this was not here previously.
		// if (RESETMUSIC)
		// 	S_StopMusic();
		S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);
	}

	if (gametype == GT_COOP)
		P_FindEmerald(); // scan for emeralds to hunt for

	// If NiGHTS, find lowest mare to start with.
	p->mare = P_FindLowestMare();

	CONS_Debug(DBG_NIGHTS, M_GetText("Current mare is %d\n"), p->mare);

	if (p->mare == 255)
		p->mare = 0;
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
static boolean G_CheckSpot(INT32 playernum, mapthing_t *mthing)
{
	fixed_t x;
	fixed_t y;
	INT32 i;

	// maybe there is no player start
	if (!mthing)
		return false;

	if (!players[playernum].mo)
	{
		// first spawn of level
		for (i = 0; i < playernum; i++)
			if (playeringame[i] && players[i].mo
				&& players[i].mo->x == mthing->x << FRACBITS
				&& players[i].mo->y == mthing->y << FRACBITS)
			{
				return false;
			}
		return true;
	}

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	if (!P_CheckPosition(players[playernum].mo, x, y))
		return false;

	return true;
}

//
// G_SpawnPlayer
// Spawn a player in a spot appropriate for the gametype --
// or a not-so-appropriate spot, if it initially fails
// due to a lack of starts open or something.
//
void G_SpawnPlayer(INT32 playernum, boolean starpost)
{
	mapthing_t *spawnpoint;

	if (!playeringame[playernum])
		return;

	P_SpawnPlayer(playernum);

	if (starpost) //Don't even bother with looking for a place to spawn.
	{
		P_MovePlayerToStarpost(playernum);
#ifdef HAVE_BLUA
		LUAh_PlayerSpawn(&players[playernum]); // Lua hook for player spawning :)
#endif
		return;
	}

	// -- CTF --
	// Order: CTF->DM->Coop
	if ((gametyperules & GTR_TEAMFLAGS) && players[playernum].ctfteam)
	{
		if (!(spawnpoint = G_FindCTFStart(playernum)) // find a CTF start
		&& !(spawnpoint = G_FindMatchStart(playernum))) // find a DM start
			spawnpoint = G_FindCoopStart(playernum); // fallback
	}

	// -- DM/Tag/CTF-spectator/etc --
	// Order: DM->CTF->Coop
	else if (gametype == GT_MATCH || gametype == GT_TEAMMATCH || gametype == GT_CTF
	 || ((gametype == GT_TAG || gametype == GT_HIDEANDSEEK) && !(players[playernum].pflags & PF_TAGIT)))
	{
		if (!(spawnpoint = G_FindMatchStart(playernum)) // find a DM start
		&& !(spawnpoint = G_FindCTFStart(playernum))) // find a CTF start
			spawnpoint = G_FindCoopStart(playernum); // fallback
	}

	// -- Other game modes --
	// Order: Coop->DM->CTF
	else
	{
		if (!(spawnpoint = G_FindCoopStart(playernum)) // find a Co-op start
		&& !(spawnpoint = G_FindMatchStart(playernum))) // find a DM start
			spawnpoint = G_FindCTFStart(playernum); // fallback
	}

	//No spawns found.  ANYWHERE.
	if (!spawnpoint)
	{
		if (nummapthings)
		{
			if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
				CONS_Alert(CONS_ERROR, M_GetText("No player spawns found, spawning at the first mapthing!\n"));
			spawnpoint = &mapthings[0];
		}
		else
		{
			if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
				CONS_Alert(CONS_ERROR, M_GetText("No player spawns found, spawning at the origin!\n"));
			//P_MovePlayerToSpawn handles this fine if the spawnpoint is NULL.
		}
	}
	P_MovePlayerToSpawn(playernum, spawnpoint);

#ifdef HAVE_BLUA
	LUAh_PlayerSpawn(&players[playernum]); // Lua hook for player spawning :)
#endif

}

mapthing_t *G_FindCTFStart(INT32 playernum)
{
	INT32 i,j;

	if (!numredctfstarts && !numbluectfstarts) //why even bother, eh?
	{
		if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
			CONS_Alert(CONS_WARNING, M_GetText("No CTF starts in this map!\n"));
		return NULL;
	}

	if ((!players[playernum].ctfteam && numredctfstarts && (!numbluectfstarts || P_RandomChance(FRACUNIT/2))) || players[playernum].ctfteam == 1) //red
	{
		if (!numredctfstarts)
		{
			if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
				CONS_Alert(CONS_WARNING, M_GetText("No Red Team starts in this map!\n"));
			return NULL;
		}

		for (j = 0; j < 32; j++)
		{
			i = P_RandomKey(numredctfstarts);
			if (G_CheckSpot(playernum, redctfstarts[i]))
				return redctfstarts[i];
		}

		if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Red Team starts!\n"));
		return NULL;
	}
	else if (!players[playernum].ctfteam || players[playernum].ctfteam == 2) //blue
	{
		if (!numbluectfstarts)
		{
			if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
				CONS_Alert(CONS_WARNING, M_GetText("No Blue Team starts in this map!\n"));
			return NULL;
		}

		for (j = 0; j < 32; j++)
		{
			i = P_RandomKey(numbluectfstarts);
			if (G_CheckSpot(playernum, bluectfstarts[i]))
				return bluectfstarts[i];
		}
		if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Blue Team starts!\n"));
		return NULL;
	}
	//should never be reached but it gets stuff to shut up
	return NULL;
}

mapthing_t *G_FindMatchStart(INT32 playernum)
{
	INT32 i, j;

	if (numdmstarts)
	{
		for (j = 0; j < 64; j++)
		{
			i = P_RandomKey(numdmstarts);
			if (G_CheckSpot(playernum, deathmatchstarts[i]))
				return deathmatchstarts[i];
		}
		if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
			CONS_Alert(CONS_WARNING, M_GetText("Could not spawn at any Deathmatch starts!\n"));
		return NULL;
	}

	if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
		CONS_Alert(CONS_WARNING, M_GetText("No Deathmatch starts in this map!\n"));
	return NULL;
}

mapthing_t *G_FindCoopStart(INT32 playernum)
{
	if (numcoopstarts)
	{
		//if there's 6 players in a map with 3 player starts, this spawns them 1/2/3/1/2/3.
		if (G_CheckSpot(playernum, playerstarts[playernum % numcoopstarts]))
			return playerstarts[playernum % numcoopstarts];

		//Don't bother checking to see if the player 1 start is open.
		//Just spawn there.
		return playerstarts[0];
	}

	if (playernum == consoleplayer || (splitscreen && playernum == secondarydisplayplayer))
		CONS_Alert(CONS_WARNING, M_GetText("No Co-op starts in this map!\n"));
	return NULL;
}

// Go back through all the projectiles and remove all references to the old
// player mobj, replacing them with the new one.
void G_ChangePlayerReferences(mobj_t *oldmo, mobj_t *newmo)
{
	thinker_t *th;
	mobj_t *mo2;

	I_Assert((oldmo != NULL) && (newmo != NULL));

	// scan all thinkers
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo2 = (mobj_t *)th;

		if (!(mo2->flags & MF_MISSILE))
			continue;

		if (mo2->target == oldmo)
		{
			P_SetTarget(&mo2->target, newmo);
			mo2->flags2 |= MF2_BEYONDTHEGRAVE; // this mobj belongs to a player who has reborn
		}
	}
}

//
// G_DoReborn
//
void G_DoReborn(INT32 playernum)
{
	player_t *player = &players[playernum];
	boolean resetlevel = false;
	INT32 i;

	if (modeattacking)
	{
		M_EndModeAttackRun();
		return;
	}

	// Make sure objectplace is OFF when you first start the level!
	OP_ResetObjectplace();

	if (player->bot && playernum != consoleplayer)
	{ // Bots respawn next to their master.
		mobj_t *oldmo = NULL;

		// first dissasociate the corpse
		if (player->mo)
		{
			oldmo = player->mo;
			// Don't leave your carcass stuck 10-billion feet in the ground!
			P_RemoveMobj(player->mo);
		}

		B_RespawnBot(playernum);
		if (oldmo)
			G_ChangePlayerReferences(oldmo, players[playernum].mo);

		return;
	}

	if (countdowntimeup || (!(netgame || multiplayer) && gametype == GT_COOP))
		resetlevel = true;
	else if (gametype == GT_COOP && (netgame || multiplayer) && !G_IsSpecialStage(gamemap))
	{
		boolean notgameover = true;

		if (cv_cooplives.value != 0 && player->lives <= 0) // consider game over first
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				if (players[i].exiting || players[i].lives > 0)
					break;
			}

			if (i == MAXPLAYERS)
			{
				notgameover = false;
				if (!countdown2)
				{
					// They're dead, Jim.
					//nextmapoverride = spstage_start;
					nextmapoverride = gamemap;
					countdown2 = TICRATE;
					skipstats = 2;

					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (playeringame[i])
							players[i].score = 0;
					}

					//emeralds = 0;
					tokenbits = 0;
					tokenlist = 0;
					token = 0;
				}
			}
		}

		if (notgameover && cv_coopstarposts.value == 2)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				if (players[i].playerstate != PST_DEAD && !players[i].spectator && players[i].mo && players[i].mo->health)
					break;
			}
			if (i == MAXPLAYERS)
				resetlevel = true;
		}
	}

	if (resetlevel)
	{
		// reload the level from scratch
		if (countdowntimeup)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				players[i].starpostscale = 0;
				players[i].starpostangle = 0;
				players[i].starposttime = 0;
				players[i].starpostx = 0;
				players[i].starposty = 0;
				players[i].starpostz = 0;
				players[i].starpostnum = 0;
			}
		}
		if (!countdowntimeup && (mapheaderinfo[gamemap-1]->levelflags & LF_NORELOAD))
		{
			P_LoadThingsOnly();

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				players[i].playerstate = PST_REBORN;
				P_ClearStarPost(players[i].starpostnum);
			}

			// Do a wipe
			wipegamestate = -1;
			wipestyleflags = WSF_CROSSFADE;

			if (camera.chase)
				P_ResetCamera(&players[displayplayer], &camera);
			if (camera2.chase && splitscreen)
				P_ResetCamera(&players[secondarydisplayplayer], &camera2);

			// clear cmd building stuff
			memset(gamekeydown, 0, sizeof (gamekeydown));
			for (i = 0; i < JOYAXISSET; i++)
			{
				joyxmove[i] = joyymove[i] = 0;
				joy2xmove[i] = joy2ymove[i] = 0;
			}
			mousex = mousey = 0;
			mouse2x = mouse2y = 0;

			// clear hud messages remains (usually from game startup)
			CON_ClearHUD();

			// Starpost support
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				G_SpawnPlayer(i, (players[i].starposttime));
			}

			// restore time in netgame (see also p_setup.c)
			if ((netgame || multiplayer) && gametype == GT_COOP && cv_coopstarposts.value == 2)
			{
				// is this a hack? maybe
				tic_t maxstarposttime = 0;
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i] && players[i].starposttime > maxstarposttime)
						maxstarposttime = players[i].starposttime;
				}
				leveltime = maxstarposttime;
			}
		}
		else
		{
#ifdef HAVE_BLUA
			LUAh_MapChange(gamemap);
#endif
			G_DoLoadLevel(true);
			if (metalrecording)
				G_BeginMetal();
			return;
		}
	}
	else
	{
		// respawn at the start
		mobj_t *oldmo = NULL;

		// Not resetting map, so return to level music
		if (!countdown2
		&& player->lives <= 0
		&& cv_cooplives.value == 1) // not allowed for life steal because no way to come back from zero group lives without addons, which should call this anyways
			P_RestoreMultiMusic(player);

		// first dissasociate the corpse
		if (player->mo)
		{
			oldmo = player->mo;
			// Don't leave your carcass stuck 10-billion feet in the ground!
			P_RemoveMobj(player->mo);
		}

		G_SpawnPlayer(playernum, (player->starposttime));
		if (oldmo)
			G_ChangePlayerReferences(oldmo, players[playernum].mo);
	}
}

void G_AddPlayer(INT32 playernum)
{
	INT32 countplayers = 0, notexiting = 0;

	player_t *p = &players[playernum];

	// Go through the current players and make sure you have the latest starpost set
	if (G_PlatformGametype() && (netgame || multiplayer))
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (players[i].bot) // ignore dumb, stupid tails
				continue;

			countplayers++;

			if (!players[i].exiting)
				notexiting++;

			if (!(cv_coopstarposts.value && (gametype == GT_COOP) && (p->starpostnum < players[i].starpostnum)))
				continue;

			p->starpostscale = players[i].starpostscale;
			p->starposttime = players[i].starposttime;
			p->starpostx = players[i].starpostx;
			p->starposty = players[i].starposty;
			p->starpostz = players[i].starpostz;
			p->starpostangle = players[i].starpostangle;
			p->starpostnum = players[i].starpostnum;
		}
	}

	p->jointime = 0;
	p->playerstate = PST_REBORN;

	p->height = mobjinfo[MT_PLAYER].height;

	if (G_GametypeUsesLives() || ((netgame || multiplayer) && gametype == GT_COOP))
		p->lives = cv_startinglives.value;

	if ((countplayers && !notexiting) || G_IsSpecialStage(gamemap))
		P_DoPlayerExit(p);
}

boolean G_EnoughPlayersFinished(void)
{
	UINT8 numneeded = (G_IsSpecialStage(gamemap) ? 4 : cv_playersforexit.value);
	INT32 total = 0;
	INT32 exiting = 0;
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator || players[i].bot)
			continue;
		if (players[i].lives <= 0)
			continue;

		total++;
		if ((players[i].pflags & PF_FINISHED) || players[i].exiting)
			exiting++;
	}

	if (exiting)
		return exiting * 4 / total >= numneeded;
	else
		return false;
}

void G_ExitLevel(void)
{
	if (gamestate == GS_LEVEL)
	{
		gameaction = ga_completed;
		lastdraw = true;

		// If you want your teams scrambled on map change, start the process now.
		// The teams will scramble at the start of the next round.
		if (cv_scrambleonchange.value && G_GametypeHasTeams())
		{
			if (server)
				CV_SetValue(&cv_teamscramble, cv_scrambleonchange.value);
		}

		if (gametyperules & GTR_ROUNDENDMESSAGE)
			CONS_Printf(M_GetText("The round has ended.\n"));

		// Remove CEcho text on round end.
		HU_ClearCEcho();
	}
	else if (gamestate == GS_ENDING)
	{
		F_StartCredits();
	}
	else if (gamestate == GS_CREDITS)
	{
		F_StartGameEvaluation();
	}
}

// See also the enum GameType in doomstat.h
const char *Gametype_Names[NUMGAMETYPES] =
{
	"Co-op", // GT_COOP
	"Competition", // GT_COMPETITION
	"Race", // GT_RACE

	"Match", // GT_MATCH
	"Team Match", // GT_TEAMMATCH

	"Tag", // GT_TAG
	"Hide & Seek", // GT_HIDEANDSEEK

	"CTF" // GT_CTF
};

// For dehacked
const char *Gametype_ConstantNames[NUMGAMETYPES] =
{
	"GT_COOP", // GT_COOP
	"GT_COMPETITION", // GT_COMPETITION
	"GT_RACE", // GT_RACE

	"GT_MATCH", // GT_MATCH
	"GT_TEAMMATCH", // GT_TEAMMATCH

	"GT_TAG", // GT_TAG
	"GT_HIDEANDSEEK", // GT_HIDEANDSEEK

	"GT_CTF" // GT_CTF
};

// Game type rules
UINT32 gametypedefaultrules[NUMGAMETYPES] =
{
	// Co-op
	GTR_PLATFORM|GTR_LIVES|GTR_CHASECAM|GTR_EMERALDHUNT|GTR_SPAWNENEMIES|GTR_ALLOWEXIT,
	// Competition
	GTR_PLATFORM|GTR_LIVES|GTR_RACE|GTR_CHASECAM|GTR_SPAWNENEMIES|GTR_ALLOWEXIT|GTR_ROUNDENDMESSAGE,
	// Race
	GTR_PLATFORM|GTR_RACE|GTR_CHASECAM|GTR_SPAWNENEMIES|GTR_ALLOWEXIT|GTR_ROUNDENDMESSAGE,

	// Match
	GTR_RINGSLINGER|GTR_SPECTATORS|GTR_TIMELIMIT|GTR_EMERALDS|GTR_PITYSHIELD|GTR_DEATHPENALTY|GTR_ROUNDENDMESSAGE,
	// Team Match
	GTR_RINGSLINGER|GTR_TEAMS|GTR_SPECTATORS|GTR_TIMELIMIT|GTR_PITYSHIELD|GTR_ROUNDENDMESSAGE,

	// Tag
	GTR_RINGSLINGER|GTR_TAG|GTR_SPECTATORS|GTR_TIMELIMIT|GTR_HIDETIME|GTR_BLINDFOLDED|GTR_ROUNDENDMESSAGE,
	// Hide and Seek
	GTR_RINGSLINGER|GTR_TAG|GTR_SPECTATORS|GTR_TIMELIMIT|GTR_HIDETIME|GTR_BLINDFOLDED|GTR_ROUNDENDMESSAGE,

	// CTF
	GTR_RINGSLINGER|GTR_TEAMS|GTR_SPECTATORS|GTR_TIMELIMIT|GTR_EMERALDS|GTR_TEAMFLAGS|GTR_PITYSHIELD|GTR_ROUNDENDMESSAGE,
};

//
// G_SetGametype
//
// Set a new gametype, also setting gametype rules accordingly. Yay!
//
void G_SetGametype(INT16 gtype)
{
	gametype = gtype;
	gametyperules = gametypedefaultrules[gametype];
}

//
// G_AddGametype
//
// Add a gametype. Returns the new gametype number.
//
INT16 G_AddGametype(UINT32 rules)
{
	INT16 newgtype = gametypecount;
	gametypecount++;

	// Set gametype rules.
	gametypedefaultrules[newgtype] = rules;
	Gametype_Names[newgtype] = "???";

	// Update gametype_cons_t accordingly.
	G_UpdateGametypeSelections();

	return newgtype;
}

//
// G_UpdateGametypeSelections
//
// Updates gametype_cons_t.
//
void G_UpdateGametypeSelections(void)
{
	INT32 i;
	for (i = 0; i < gametypecount; i++)
	{
		gametype_cons_t[i].value = i;
		gametype_cons_t[i].strvalue = Gametype_Names[i];
	}
	gametype_cons_t[NUMGAMETYPES].value = 0;
	gametype_cons_t[NUMGAMETYPES].strvalue = NULL;
}

//
// G_SetGametypeDescription
//
// Set a description for the specified gametype.
// (Level platter)
//
void G_SetGametypeDescription(INT16 gtype, char *descriptiontext, UINT8 leftcolor, UINT8 rightcolor)
{
	strncpy(gametypedesc[gtype].notes, descriptiontext, 441);
	gametypedesc[gtype].col[0] = leftcolor;
	gametypedesc[gtype].col[1] = rightcolor;
}

UINT32 gametypetol[NUMGAMETYPES] =
{
	TOL_COOP, // Co-op
	TOL_COMPETITION, // Competition
	TOL_RACE, // Race

	TOL_MATCH, // Match
	TOL_MATCH, // Team Match

	TOL_TAG, // Tag
	TOL_TAG, // Hide and Seek

	TOL_CTF, // CTF
};

//
// G_AddTOL
//
// Adds a type of level.
//
void G_AddTOL(UINT32 newtol, const char *tolname)
{
	TYPEOFLEVEL[numtolinfo].name = Z_StrDup(tolname);
	TYPEOFLEVEL[numtolinfo].flag = newtol;
	numtolinfo++;

	TYPEOFLEVEL[numtolinfo].name = NULL;
	TYPEOFLEVEL[numtolinfo].flag = 0;
}

//
// G_AddTOL
//
// Assigns a type of level to a gametype.
//
void G_AddGametypeTOL(INT16 gtype, UINT32 newtol)
{
	gametypetol[gtype] = newtol;
}

//
// G_GetGametypeByName
//
// Returns the number for the given gametype name string, or -1 if not valid.
//
INT32 G_GetGametypeByName(const char *gametypestr)
{
	INT32 i;

	for (i = 0; i < NUMGAMETYPES; i++)
		if (!stricmp(gametypestr, Gametype_Names[i]))
			return i;

	return -1; // unknown gametype
}

//
// G_IsSpecialStage
//
// Returns TRUE if
// the given map is a special stage.
//
boolean G_IsSpecialStage(INT32 mapnum)
{
	if (gametype != GT_COOP || modeattacking == ATTACKING_RECORD)
		return false;
	if (mapnum >= sstage_start && mapnum <= sstage_end)
		return true;
	if (mapnum >= smpstage_start && mapnum <= smpstage_end)
		return true;

	return false;
}

//
// G_GametypeUsesLives
//
// Returns true if the current gametype uses
// the lives system.  False otherwise.
//
boolean G_GametypeUsesLives(void)
{
	 // Coop, Competitive
	//if ((gametype == GT_COOP || gametype == GT_COMPETITION)
	if ((gametyperules & GTR_LIVES)
	 && !(modeattacking || metalrecording) // No lives in Time Attack
	 && !G_IsSpecialStage(gamemap)
	 && !(maptol & TOL_NIGHTS)) // No lives in NiGHTS
		return true;
	return false;
}

//
// G_GametypeHasTeams
//
// Returns true if the current gametype uses
// Red/Blue teams.  False otherwise.
//
boolean G_GametypeHasTeams(void)
{
	return (gametyperules & GTR_TEAMS);
	//return (gametype == GT_TEAMMATCH || gametype == GT_CTF);
}

//
// G_GametypeHasSpectators
//
// Returns true if the current gametype supports
// spectators.  False otherwise.
//
boolean G_GametypeHasSpectators(void)
{
	return (gametyperules & GTR_SPECTATORS);
	//return (gametype != GT_COOP && gametype != GT_COMPETITION && gametype != GT_RACE);
}

//
// G_RingSlingerGametype
//
// Returns true if the current gametype supports firing rings.
// ANY gametype can be a ringslinger gametype, just flick a switch.
//
boolean G_RingSlingerGametype(void)
{
	return ((gametyperules & GTR_RINGSLINGER) || (cv_ringslinger.value));
	//return ((gametype != GT_COOP && gametype != GT_COMPETITION && gametype != GT_RACE) || (cv_ringslinger.value));
}

//
// G_PlatformGametype
//
// Returns true if a gametype is a more traditional platforming-type.
//
boolean G_PlatformGametype(void)
{
	return (gametyperules & GTR_PLATFORM);
	//return (gametype == GT_COOP || gametype == GT_RACE || gametype == GT_COMPETITION);
}

//
// G_TagGametype
//
// For Jazz's Tag/HnS modes that have a lot of special cases..
//
boolean G_TagGametype(void)
{
	return (gametyperules & GTR_TAG);
	//return (gametype == GT_TAG || gametype == GT_HIDEANDSEEK);
}

/** Get the typeoflevel flag needed to indicate support of a gametype.
  * In single-player, this always returns TOL_SP.
  * \param gametype The gametype for which support is desired.
  * \return The typeoflevel flag to check for that gametype.
  * \author Graue <graue@oceanbase.org>
  */
INT16 G_TOLFlag(INT32 pgametype)
{
	if (!multiplayer)                 return TOL_SP;
	return gametypetol[pgametype];
#if 0
	if (pgametype == GT_COOP)         return TOL_COOP;
	if (pgametype == GT_COMPETITION)  return TOL_COMPETITION;
	if (pgametype == GT_RACE)         return TOL_RACE;
	if (pgametype == GT_MATCH)        return TOL_MATCH;
	if (pgametype == GT_TEAMMATCH)    return TOL_MATCH;
	if (pgametype == GT_TAG)          return TOL_TAG;
	if (pgametype == GT_HIDEANDSEEK)  return TOL_TAG;
	if (pgametype == GT_CTF)          return TOL_CTF;

	CONS_Alert(CONS_ERROR, M_GetText("Unknown gametype! %d\n"), pgametype);
	return INT16_MAX;
#endif
}

/** Select a random map with the given typeoflevel flags.
  * If no map has those flags, this arbitrarily gives you map 1.
  * \param tolflags The typeoflevel flags to insist on. Other bits may
  *                 be on too, but all of these must be on.
  * \return A random map with those flags, 1-based, or 1 if no map
  *         has those flags.
  * \author Graue <graue@oceanbase.org>
  */
static INT16 RandMap(UINT32 tolflags, INT16 pprevmap)
{
	INT16 *okmaps = Z_Malloc(NUMMAPS * sizeof(INT16), PU_STATIC, NULL);
	INT32 numokmaps = 0;
	INT16 ix;

	// Find all the maps that are ok and and put them in an array.
	for (ix = 0; ix < NUMMAPS; ix++)
		if (mapheaderinfo[ix] && (mapheaderinfo[ix]->typeoflevel & tolflags) == tolflags
		 && ix != pprevmap // Don't pick the same map.
		 && (dedicated || !M_MapLocked(ix+1)) // Don't pick locked maps.
		)
			okmaps[numokmaps++] = ix;

	if (numokmaps == 0)
		ix = 0; // Sorry, none match. You get MAP01.
	else
		ix = okmaps[M_RandomKey(numokmaps)];

	Z_Free(okmaps);

	return ix;
}

//
// G_UpdateVisited
//
static void G_UpdateVisited(void)
{
	boolean spec = G_IsSpecialStage(gamemap);
	// Update visitation flags?
	if ((!modifiedgame || savemoddata) // Not modified
		&& !multiplayer && !demoplayback && (gametype == GT_COOP) // SP/RA/NiGHTS mode
		&& !(spec && stagefailed)) // Not failed the special stage
	{
		UINT8 earnedEmblems;

		// Update visitation flags
		mapvisited[gamemap-1] |= MV_BEATEN;
		// eh, what the hell
		if (ultimatemode)
			mapvisited[gamemap-1] |= MV_ULTIMATE;
		// may seem incorrect but IS possible in what the main game uses as mp special stages, and nummaprings will be -1 in NiGHTS
		if (nummaprings > 0 && players[consoleplayer].rings >= nummaprings)
		{
			mapvisited[gamemap-1] |= MV_PERFECT;
			if (modeattacking)
				mapvisited[gamemap-1] |= MV_PERFECTRA;
		}
		if (!spec)
		{
			// not available to special stages because they can only really be done in one order in an unmodified game, so impossible for first six and trivial for seventh
			if (ALL7EMERALDS(emeralds))
				mapvisited[gamemap-1] |= MV_ALLEMERALDS;
		}

		if (modeattacking == ATTACKING_RECORD)
			G_UpdateRecordReplays();
		else if (modeattacking == ATTACKING_NIGHTS)
			G_SetNightsRecords();

		if ((earnedEmblems = M_CompletionEmblems()))
			CONS_Printf(M_GetText("\x82" "Earned %hu emblem%s for level completion.\n"), (UINT16)earnedEmblems, earnedEmblems > 1 ? "s" : "");
	}
}

//
// G_DoCompleted
//
static void G_DoCompleted(void)
{
	INT32 i;
	boolean spec = G_IsSpecialStage(gamemap);

	tokenlist = 0; // Reset the list

	if (modeattacking && pausedelay)
		pausedelay = 0;

	gameaction = ga_nothing;

	if (metalplayback)
		G_StopMetalDemo();
	if (metalrecording)
		G_StopMetalRecording(false);

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
			G_PlayerFinishLevel(i); // take away cards and stuff

	if (automapactive)
		AM_Stop();

	S_StopSounds();

	prevmap = (INT16)(gamemap-1);

	// go to next level
	// nextmap is 0-based, unlike gamemap
	if (nextmapoverride != 0)
		nextmap = (INT16)(nextmapoverride-1);
	else
		nextmap = (INT16)(mapheaderinfo[gamemap-1]->nextlevel-1);

	// Remember last map for when you come out of the special stage.
	if (!spec)
		lastmap = nextmap;

	// If nextmap is actually going to get used, make sure it points to
	// a map of the proper gametype -- skip levels that don't support
	// the current gametype. (Helps avoid playing boss levels in Race,
	// for instance).
	if (!token && !spec
		&& (nextmap >= 0 && nextmap < NUMMAPS))
	{
		register INT16 cm = nextmap;
		INT16 tolflag = G_TOLFlag(gametype);
		UINT8 visitedmap[(NUMMAPS+7)/8];

		memset(visitedmap, 0, sizeof (visitedmap));

		while (!mapheaderinfo[cm] || !(mapheaderinfo[cm]->typeoflevel & tolflag))
		{
			visitedmap[cm/8] |= (1<<(cm&7));
			if (!mapheaderinfo[cm])
				cm = -1; // guarantee error execution
			else
				cm = (INT16)(mapheaderinfo[cm]->nextlevel-1);

			if (cm >= NUMMAPS || cm < 0) // out of range (either 1100-1102 or error)
			{
				cm = nextmap; //Start the loop again so that the error checking below is executed.

				//Make sure the map actually exists before you try to go to it!
				if ((W_CheckNumForName(G_BuildMapName(cm + 1)) == LUMPERROR))
				{
					CONS_Alert(CONS_ERROR, M_GetText("Next map given (MAP %d) doesn't exist! Reverting to MAP01.\n"), cm+1);
					cm = 0;
					break;
				}
			}

			if (visitedmap[cm/8] & (1<<(cm&7))) // smells familiar
			{
				// We got stuck in a loop, came back to the map we started on
				// without finding one supporting the current gametype.
				// Thus, print a warning, and just use this map anyways.
				CONS_Alert(CONS_WARNING, M_GetText("Can't find a compatible map after map %d; using map %d anyway\n"), prevmap+1, cm+1);
				break;
			}
		}
		nextmap = cm;
	}

	if (nextmap < 0 || (nextmap >= NUMMAPS && nextmap < 1100-1) || nextmap > 1103-1)
		I_Error("Followed map %d to invalid map %d\n", prevmap + 1, nextmap + 1);

	// wrap around in race
	if (nextmap >= 1100-1 && nextmap <= 1102-1 && (gametyperules & GTR_RACE))
		nextmap = (INT16)(spstage_start-1);

	if ((gottoken = (gametype == GT_COOP && token)))
	{
		token--;

		for (i = 0; i < 7; i++)
			if (!(emeralds & (1<<i)))
			{
				nextmap = ((netgame || multiplayer) ? smpstage_start : sstage_start) + i - 1; // to special stage!
				break;
			}

		if (i == 7)
			gottoken = false;
	}

	if (spec && !gottoken)
		nextmap = lastmap; // Exiting from a special stage? Go back to the game. Tails 08-11-2001

	automapactive = false;

	if (gametype != GT_COOP)
	{
		if (cv_advancemap.value == 0) // Stay on same map.
			nextmap = prevmap;
		else if (cv_advancemap.value == 2) // Go to random map.
			nextmap = RandMap(G_TOLFlag(gametype), prevmap);
	}

	// We are committed to this map now.
	// We may as well allocate its header if it doesn't exist
	// (That is, if it's a real map)
	if (nextmap < NUMMAPS && !mapheaderinfo[nextmap])
		P_AllocMapHeader(nextmap);

	if ((skipstats && !modeattacking) || (spec && modeattacking && stagefailed))
	{
		G_UpdateVisited();
		G_AfterIntermission();
	}
	else
	{
		G_SetGamestate(GS_INTERMISSION);
		Y_StartIntermission();
		G_UpdateVisited();
	}
}

void G_AfterIntermission(void)
{
	Y_CleanupScreenBuffer();

	if (modeattacking)
	{
		M_EndModeAttackRun();
		return;
	}

	HU_ClearCEcho();

	if (mapheaderinfo[gamemap-1]->cutscenenum && !modeattacking && skipstats <= 1) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1]->cutscenenum-1, false, false);
	else
	{
		if (nextmap < 1100-1)
			G_NextLevel();
		else
			G_EndGame();
	}
}

//
// G_NextLevel (WorldDone)
//
// init next level or go to the final scene
// called by end of intermission screen (y_inter)
//
void G_NextLevel(void)
{
	gameaction = ga_worlddone;
}

static void G_DoWorldDone(void)
{
	if (server)
	{
		if (gametype == GT_COOP)
			// don't reset player between maps
			D_MapChange(nextmap+1, gametype, ultimatemode, false, 0, false, false);
		else
			// resetplayer in match/chaos/tag/CTF/race for more equality
			D_MapChange(nextmap+1, gametype, ultimatemode, true, 0, false, false);
	}

	gameaction = ga_nothing;
}

//
// G_UseContinue
//
void G_UseContinue(void)
{
	if (gamestate == GS_LEVEL && !netgame && !multiplayer)
	{
		gameaction = ga_startcont;
		lastdraw = true;
	}
}

static void G_DoStartContinue(void)
{
	I_Assert(!netgame && !multiplayer);

	G_PlayerFinishLevel(consoleplayer); // take away cards and stuff

	F_StartContinue();
	gameaction = ga_nothing;
}

//
// G_Continue
//
// re-init level, used by continue and possibly countdowntimeup
//
void G_Continue(void)
{
	if (!netgame && !multiplayer)
		gameaction = ga_continued;
}

static void G_DoContinued(void)
{
	player_t *pl = &players[consoleplayer];
	I_Assert(!netgame && !multiplayer);
	I_Assert(pl->continues > 0);

	pl->continues--;

	// Reset score
	pl->score = 0;

	// Allow tokens to come back
	tokenlist = 0;
	token = 0;

	if (!(netgame || multiplayer || demoplayback || demorecording || metalrecording || modeattacking) && (!modifiedgame || savemoddata) && cursaveslot > 0)
		G_SaveGameOver((UINT32)cursaveslot, true);

	// Reset # of lives
	pl->lives = (ultimatemode) ? 1 : startinglivesbalance[numgameovers];

	D_MapChange(gamemap, gametype, ultimatemode, false, 0, false, false);

	gameaction = ga_nothing;
}

//
// G_EndGame (formerly Y_EndGame)
// Frankly this function fits better in g_game.c than it does in y_inter.c
//
// ...Gee, (why) end the game?
// Because G_AfterIntermission and F_EndCutscene would
// both do this exact same thing *in different ways* otherwise,
// which made it so that you could only unlock Ultimate mode
// if you had a cutscene after the final level and crap like that.
// This function simplifies it so only one place has to be updated
// when something new is added.
void G_EndGame(void)
{
	// Only do evaluation and credits in coop games.
	if (gametype == GT_COOP)
	{
		if (nextmap == 1103-1) // end game with ending
		{
			F_StartEnding();
			return;
		}
		if (nextmap == 1102-1) // end game with credits
		{
			F_StartCredits();
			return;
		}
		if (nextmap == 1101-1) // end game with evaluation
		{
			F_StartGameEvaluation();
			return;
		}
	}

	// 1100 or competitive multiplayer, so go back to title screen.
	D_StartTitle();
}

//
// G_LoadGameSettings
//
// Sets a tad of default info we need.
void G_LoadGameSettings(void)
{
	// defaults
	spstage_start = 1;
	sstage_start = 50;
	sstage_end = 56; // 7 special stages in vanilla SRB2
	sstage_end++; // plus one weirdo
	smpstage_start = 60;
	smpstage_end = 66; // 7 multiplayer special stages too

	// initialize free sfx slots for skin sounds
	S_InitRuntimeSounds();
}

// G_LoadGameData
// Loads the main data file, which stores information such as emblems found, etc.
void G_LoadGameData(void)
{
	size_t length;
	INT32 i, j;
	UINT8 modded = false;
	UINT8 rtemp;

	//For records
	UINT32 recscore;
	tic_t  rectime;
	UINT16 recrings;

	UINT8 recmares;
	INT32 curmare;

	// Clear things so previously read gamedata doesn't transfer
	// to new gamedata
	G_ClearRecords(); // main and nights records
	M_ClearSecrets(); // emblems, unlocks, maps visited, etc
	totalplaytime = 0; // total play time (separate from all)

	if (M_CheckParm("-nodata"))
		return; // Don't load.

	// Allow saving of gamedata beyond this point
	gamedataloaded = true;

	if (M_CheckParm("-gamedata") && M_IsNextParm())
	{
		strlcpy(gamedatafilename, M_GetNextParm(), sizeof gamedatafilename);
	}

	if (M_CheckParm("-resetdata"))
		return; // Don't load (essentially, reset).

	length = FIL_ReadFile(va(pandf, srb2home, gamedatafilename), &savebuffer);
	if (!length) // Aw, no game data. Their loss!
		return;

	save_p = savebuffer;

	// Version check
	if (READUINT32(save_p) != 0xFCAFE211)
	{
		const char *gdfolder = "the SRB2 folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		Z_Free(savebuffer);
		save_p = NULL;
		I_Error("Game data is from another version of SRB2.\nDelete %s(maybe in %s) and try again.", gamedatafilename, gdfolder);
	}

	totalplaytime = READUINT32(save_p);

	modded = READUINT8(save_p);

	// Aha! Someone's been screwing with the save file!
	if ((modded && !savemoddata))
		goto datacorrupt;
	else if (modded != true && modded != false)
		goto datacorrupt;

	// TODO put another cipher on these things? meh, I don't care...
	for (i = 0; i < NUMMAPS; i++)
		if ((mapvisited[i] = READUINT8(save_p)) > MV_MAX)
			goto datacorrupt;

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			emblemlocations[j+i].collected = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXEXTRAEMBLEMS;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXEXTRAEMBLEMS; ++j)
			extraemblems[j+i].collected = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			unlockables[j+i].unlocked = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		rtemp = READUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			conditionSets[j+i].achieved = ((rtemp >> j) & 1);
		i += j;
	}

	timesBeaten = READUINT32(save_p);
	timesBeatenWithEmeralds = READUINT32(save_p);
	timesBeatenUltimate = READUINT32(save_p);

	// Main records
	for (i = 0; i < NUMMAPS; ++i)
	{
		recscore = READUINT32(save_p);
		rectime  = (tic_t)READUINT32(save_p);
		recrings = READUINT16(save_p);
		save_p++; // compat

		if (recrings > 10000 || recscore > MAXSCORE)
			goto datacorrupt;

		if (recscore || rectime || recrings)
		{
			G_AllocMainRecordData((INT16)i);
			mainrecords[i]->score = recscore;
			mainrecords[i]->time = rectime;
			mainrecords[i]->rings = recrings;
		}
	}

	// Nights records
	for (i = 0; i < NUMMAPS; ++i)
	{
		if ((recmares = READUINT8(save_p)) == 0)
			continue;

		G_AllocNightsRecordData((INT16)i);

		for (curmare = 0; curmare < (recmares+1); ++curmare)
		{
			nightsrecords[i]->score[curmare] = READUINT32(save_p);
			nightsrecords[i]->grade[curmare] = READUINT8(save_p);
			nightsrecords[i]->time[curmare] = (tic_t)READUINT32(save_p);

			if (nightsrecords[i]->grade[curmare] > GRADE_S)
				goto datacorrupt;
		}

		nightsrecords[i]->nummares = recmares;
	}

	// done
	Z_Free(savebuffer);
	save_p = NULL;

	// Silent update unlockables in case they're out of sync with conditions
	M_SilentUpdateUnlockablesAndEmblems();

	return;

	// Landing point for corrupt gamedata
	datacorrupt:
	{
		const char *gdfolder = "the SRB2 folder";
		if (strcmp(srb2home,"."))
			gdfolder = srb2home;

		Z_Free(savebuffer);
		save_p = NULL;

		I_Error("Corrupt game data file.\nDelete %s(maybe in %s) and try again.", gamedatafilename, gdfolder);
	}
}

// G_SaveGameData
// Saves the main data file, which stores information such as emblems found, etc.
void G_SaveGameData(void)
{
	size_t length;
	INT32 i, j;
	UINT8 btemp;

	INT32 curmare;

	if (!gamedataloaded)
		return; // If never loaded (-nodata), don't save

	save_p = savebuffer = (UINT8 *)malloc(GAMEDATASIZE);
	if (!save_p)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for saving game data\n"));
		return;
	}

	if (modifiedgame && !savemoddata)
	{
		free(savebuffer);
		save_p = savebuffer = NULL;
		return;
	}

	// Version test
	WRITEUINT32(save_p, 0xFCAFE211);

	WRITEUINT32(save_p, totalplaytime);

	btemp = (UINT8)(savemoddata || modifiedgame);
	WRITEUINT8(save_p, btemp);

	// TODO put another cipher on these things? meh, I don't care...
	for (i = 0; i < NUMMAPS; i++)
		WRITEUINT8(save_p, (mapvisited[i] & MV_MAX));

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			btemp |= (emblemlocations[j+i].collected << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXEXTRAEMBLEMS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEXTRAEMBLEMS; ++j)
			btemp |= (extraemblems[j+i].collected << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			btemp |= (unlockables[j+i].unlocked << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			btemp |= (conditionSets[j+i].achieved << j);
		WRITEUINT8(save_p, btemp);
		i += j;
	}

	WRITEUINT32(save_p, timesBeaten);
	WRITEUINT32(save_p, timesBeatenWithEmeralds);
	WRITEUINT32(save_p, timesBeatenUltimate);

	// Main records
	for (i = 0; i < NUMMAPS; i++)
	{
		if (mainrecords[i])
		{
			WRITEUINT32(save_p, mainrecords[i]->score);
			WRITEUINT32(save_p, mainrecords[i]->time);
			WRITEUINT16(save_p, mainrecords[i]->rings);
		}
		else
		{
			WRITEUINT32(save_p, 0);
			WRITEUINT32(save_p, 0);
			WRITEUINT16(save_p, 0);
		}
		WRITEUINT8(save_p, 0); // compat
	}

	// NiGHTS records
	for (i = 0; i < NUMMAPS; i++)
	{
		if (!nightsrecords[i] || !nightsrecords[i]->nummares)
		{
			WRITEUINT8(save_p, 0);
			continue;
		}

		WRITEUINT8(save_p, nightsrecords[i]->nummares);

		for (curmare = 0; curmare < (nightsrecords[i]->nummares + 1); ++curmare)
		{
			WRITEUINT32(save_p, nightsrecords[i]->score[curmare]);
			WRITEUINT8(save_p, nightsrecords[i]->grade[curmare]);
			WRITEUINT32(save_p, nightsrecords[i]->time[curmare]);
		}
	}

	length = save_p - savebuffer;

	FIL_WriteFile(va(pandf, srb2home, gamedatafilename), savebuffer, length);
	free(savebuffer);
	save_p = savebuffer = NULL;
}

#define VERSIONSIZE 16

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
void G_LoadGame(UINT32 slot, INT16 mapoverride)
{
	size_t length;
	char vcheck[VERSIONSIZE];
	char savename[255];

	// memset savedata to all 0, fixes calling perfectly valid saves corrupt because of bots
	memset(&savedata, 0, sizeof(savedata));

#ifdef SAVEGAME_OTHERVERSIONS
	//Oh christ.  The force load response needs access to mapoverride too...
	startonmapnum = mapoverride;
#endif

	sprintf(savename, savegamename, slot);

	length = FIL_ReadFile(savename, &savebuffer);
	if (!length)
	{
		CONS_Printf(M_GetText("Couldn't read file %s\n"), savename);
		return;
	}

	save_p = savebuffer;

	memset(vcheck, 0, sizeof (vcheck));
	sprintf(vcheck, "version %d", VERSION);
	if (strcmp((const char *)save_p, (const char *)vcheck))
	{
#ifdef SAVEGAME_OTHERVERSIONS
		M_StartMessage(M_GetText("Save game from different version.\nYou can load this savegame, but\nsaving afterwards will be disabled.\n\nDo you want to continue anyway?\n\n(Press 'Y' to confirm)\n"),
		               M_ForceLoadGameResponse, MM_YESNO);
		//Freeing done by the callback function of the above message
#else
		M_ClearMenus(true); // so ESC backs out to title
		M_StartMessage(M_GetText("Save game from different version\n\nPress ESC\n"), NULL, MM_NOTHING);
		Command_ExitGame_f();
		Z_Free(savebuffer);
		save_p = savebuffer = NULL;

		// no cheating!
		memset(&savedata, 0, sizeof(savedata));
#endif
		return; // bad version
	}
	save_p += VERSIONSIZE;

//	if (demoplayback) // reset game engine
//		G_StopDemo();

//	paused = false;
//	automapactive = false;

	// dearchive all the modifications
	if (!P_LoadGame(mapoverride))
	{
		M_ClearMenus(true); // so ESC backs out to title
		M_StartMessage(M_GetText("Savegame file corrupted\n\nPress ESC\n"), NULL, MM_NOTHING);
		Command_ExitGame_f();
		Z_Free(savebuffer);
		save_p = savebuffer = NULL;

		// no cheating!
		memset(&savedata, 0, sizeof(savedata));
		return;
	}

	// done
	Z_Free(savebuffer);
	save_p = savebuffer = NULL;

//	gameaction = ga_nothing;
//	G_SetGamestate(GS_LEVEL);
	displayplayer = consoleplayer;
	multiplayer = splitscreen = false;

//	G_DeferedInitNew(sk_medium, G_BuildMapName(1), 0, 0, 1);
	if (setsizeneeded)
		R_ExecuteSetViewSize();

	M_ClearMenus(true);
	CON_ToggleOff();
}

//
// G_SaveGame
// Saves your game.
//
void G_SaveGame(UINT32 slot)
{
	boolean saved;
	char savename[256] = "";
	const char *backup;

	sprintf(savename, savegamename, slot);
	backup = va("%s",savename);

	// save during evaluation or credits? game's over, folks!
	if (gamestate == GS_ENDING || gamestate == GS_CREDITS || gamestate == GS_EVALUATION)
		gamecomplete = true;

	gameaction = ga_nothing;
	{
		char name[VERSIONSIZE];
		size_t length;

		save_p = savebuffer = (UINT8 *)malloc(SAVEGAMESIZE);
		if (!save_p)
		{
			CONS_Alert(CONS_ERROR, M_GetText("No more free memory for saving game data\n"));
			return;
		}

		memset(name, 0, sizeof (name));
		sprintf(name, "version %d", VERSION);
		WRITEMEM(save_p, name, VERSIONSIZE);

		P_SaveGame();

		length = save_p - savebuffer;
		saved = FIL_WriteFile(backup, savebuffer, length);
		free(savebuffer);
		save_p = savebuffer = NULL;
	}

	gameaction = ga_nothing;

	if (cv_debug && saved)
		CONS_Printf(M_GetText("Game saved.\n"));
	else if (!saved)
		CONS_Alert(CONS_ERROR, M_GetText("Error while writing to %s for save slot %u, base: %s\n"), backup, slot, savegamename);
}

#define BADSAVE goto cleanup;
#define CHECKPOS if (save_p >= end_p) BADSAVE
void G_SaveGameOver(UINT32 slot, boolean modifylives)
{
	boolean saved = false;
	size_t length;
	char vcheck[VERSIONSIZE];
	char savename[255];
	const char *backup;

	sprintf(savename, savegamename, slot);
	backup = va("%s",savename);

	length = FIL_ReadFile(savename, &savebuffer);
	if (!length)
	{
		CONS_Printf(M_GetText("Couldn't read file %s\n"), savename);
		return;
	}

	{
		char temp[sizeof(timeattackfolder)];
		UINT8 *end_p = savebuffer + length;
		UINT8 *lives_p;
		SINT8 pllives;

		save_p = savebuffer;
		// Version check
		memset(vcheck, 0, sizeof (vcheck));
		sprintf(vcheck, "version %d", VERSION);
		if (strcmp((const char *)save_p, (const char *)vcheck)) BADSAVE
		save_p += VERSIONSIZE;

		// P_UnArchiveMisc()
		(void)READINT16(save_p);
		CHECKPOS
		(void)READUINT16(save_p); // emeralds
		CHECKPOS
		READSTRINGN(save_p, temp, sizeof(temp)); // mod it belongs to
		if (strcmp(temp, timeattackfolder)) BADSAVE

		// P_UnArchivePlayer()
		CHECKPOS
		(void)READUINT16(save_p);
		CHECKPOS

		WRITEUINT8(save_p, numgameovers);
		CHECKPOS

		lives_p = save_p;
		pllives = READSINT8(save_p); // lives
		CHECKPOS
		if (modifylives && pllives < startinglivesbalance[numgameovers])
		{
			pllives = startinglivesbalance[numgameovers];
			WRITESINT8(lives_p, pllives);
		}

		(void)READINT32(save_p); // Score
		CHECKPOS
		(void)READINT32(save_p); // continues

		// File end marker check
		CHECKPOS
		switch (READUINT8(save_p))
		{
			case 0xb7:
				{
					UINT8 i, banksinuse;
					CHECKPOS
					banksinuse = READUINT8(save_p);
					CHECKPOS
					if (banksinuse > NUM_LUABANKS)
						BADSAVE
					for (i = 0; i < banksinuse; i++)
					{
						(void)READINT32(save_p);
						CHECKPOS
					}
					if (READUINT8(save_p) != 0x1d)
						BADSAVE
				}
			case 0x1d:
				break;
			default:
				BADSAVE
		}

		// done
		saved = FIL_WriteFile(backup, savebuffer, length);
	}

cleanup:
	if (cv_debug && saved)
		CONS_Printf(M_GetText("Game saved.\n"));
	else if (!saved)
		CONS_Alert(CONS_ERROR, M_GetText("Error while writing to %s for save slot %u, base: %s\n"), backup, slot, savegamename);
	Z_Free(savebuffer);
	save_p = savebuffer = NULL;

}
#undef CHECKPOS
#undef BADSAVE

//
// G_DeferedInitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//
void G_DeferedInitNew(boolean pultmode, const char *mapname, INT32 pickedchar, boolean SSSG, boolean FLS)
{
	UINT8 color = skins[pickedchar].prefcolor;
	paused = false;

	if (demoplayback)
		COM_BufAddText("stopdemo\n");
	ghosts = NULL;

	// this leave the actual game if needed
	SV_StartSinglePlayerServer();

	if (savedata.lives > 0)
	{
		if ((botingame = ((botskin = savedata.botskin) != 0)))
			botcolor = skins[botskin-1].prefcolor;
	}
	else if (splitscreen != SSSG)
	{
		splitscreen = SSSG;
		SplitScreen_OnChange();
	}

	color = skins[pickedchar].prefcolor;
	SetPlayerSkinByNum(consoleplayer, pickedchar);
	CV_StealthSet(&cv_skin, skins[pickedchar].name);
	CV_StealthSetValue(&cv_playercolor, color);

	if (mapname)
		D_MapChange(M_MapNumber(mapname[3], mapname[4]), gametype, pultmode, true, 1, false, FLS);
}

//
// This is the map command interpretation something like Command_Map_f
//
// called at: map cmd execution, doloadgame, doplaydemo
void G_InitNew(UINT8 pultmode, const char *mapname, boolean resetplayer, boolean skipprecutscene, boolean FLS)
{
	INT32 i;

	Y_CleanupScreenBuffer();

	if (paused)
	{
		paused = false;
		S_ResumeAudio();
	}

	if (netgame || multiplayer) // Nice try, haxor.
		pultmode = false;

	if (!demoplayback && !netgame) // Netgame sets random seed elsewhere, demo playback sets seed just before us!
		P_SetRandSeed(M_RandomizedSeed()); // Use a more "Random" random seed

	if (resetplayer)
	{
		// Clear a bunch of variables
		numgameovers = tokenlist = token = sstimer = redscore = bluescore = lastmap = 0;
		countdown = countdown2 = exitfadestarted = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			players[i].playerstate = PST_REBORN;
			players[i].starpostscale = players[i].starpostangle = players[i].starpostnum = players[i].starposttime = 0;
			players[i].starpostx = players[i].starposty = players[i].starpostz = 0;

			if (netgame || multiplayer)
			{
				if (!FLS || (players[i].lives < 1))
					players[i].lives = cv_startinglives.value;
				players[i].continues = 0;
			}
			else
			{
				players[i].lives = (pultmode) ? 1 : startinglivesbalance[0];
				players[i].continues = (pultmode) ? 0 : 1;
			}

			if (!((netgame || multiplayer) && (FLS)))
				players[i].score = 0;

			// The latter two should clear by themselves, but just in case
			players[i].pflags &= ~(PF_TAGIT|PF_GAMETYPEOVER|PF_FULLSTASIS);

			// Clear cheatcodes too, just in case.
			players[i].pflags &= ~(PF_GODMODE|PF_NOCLIP|PF_INVIS);

			players[i].xtralife = 0;
		}

		// Reset unlockable triggers
		unlocktriggers = 0;

		// clear itemfinder, just in case
		if (!dedicated)	// except in dedicated servers, where it is not registered and can actually I_Error debug builds
			CV_StealthSetValue(&cv_itemfinder, 0);
	}

	// internal game map
	// well this check is useless because it is done before (d_netcmd.c::command_map_f)
	// but in case of for demos....
	if (W_CheckNumForName(mapname) == LUMPERROR)
	{
		I_Error("Internal game map '%s' not found\n", mapname);
		Command_ExitGame_f();
		return;
	}

	gamemap = (INT16)M_MapNumber(mapname[3], mapname[4]); // get xx out of MAPxx

	// gamemap changed; we assume that its map header is always valid,
	// so make it so
	if(!mapheaderinfo[gamemap-1])
		P_AllocMapHeader(gamemap-1);

	maptol = mapheaderinfo[gamemap-1]->typeoflevel;
	globalweather = mapheaderinfo[gamemap-1]->weather;

	// Don't carry over custom music change to another map.
	mapmusflags |= MUSIC_RELOADRESET;

	ultimatemode = pultmode;
	automapactive = false;
	imcontinuing = false;

	if (!skipprecutscene && mapheaderinfo[gamemap-1]->precutscenenum && !modeattacking) // Start a custom cutscene.
		F_StartCustomCutscene(mapheaderinfo[gamemap-1]->precutscenenum-1, true, resetplayer);
	else
		G_DoLoadLevel(resetplayer);

	if (netgame)
	{
		char *title = G_BuildMapTitle(gamemap);

		CONS_Printf(M_GetText("Map is now \"%s"), G_BuildMapName(gamemap));
		if (title)
		{
			CONS_Printf(": %s", title);
			Z_Free(title);
		}
		CONS_Printf("\"\n");
	}
}


char *G_BuildMapTitle(INT32 mapnum)
{
	char *title = NULL;

	if (!mapheaderinfo[mapnum-1])
		P_AllocMapHeader(mapnum-1);

	if (strcmp(mapheaderinfo[mapnum-1]->lvlttl, ""))
	{
		size_t len = 1;
		const char *zonetext = NULL;
		const INT32 actnum = mapheaderinfo[mapnum-1]->actnum;

		len += strlen(mapheaderinfo[mapnum-1]->lvlttl);
		if (!(mapheaderinfo[mapnum-1]->levelflags & LF_NOZONE))
		{
			zonetext = M_GetText("Zone");
			len += strlen(zonetext) + 1;	// ' ' + zonetext
		}
		if (actnum > 0)
			len += 1 + 11;			// ' ' + INT32

		title = Z_Malloc(len, PU_STATIC, NULL);

		sprintf(title, "%s", mapheaderinfo[mapnum-1]->lvlttl);
		if (zonetext) sprintf(title + strlen(title), " %s", zonetext);
		if (actnum > 0) sprintf(title + strlen(title), " %d", actnum);
	}

	return title;
}

static void measurekeywords(mapsearchfreq_t *fr,
		struct searchdim **dimp, UINT8 *cuntp,
		const char *s, const char *q, boolean wanttable)
{
	char *qp;
	char *sp;
	if (wanttable)
		(*dimp) = Z_Realloc((*dimp), 255 * sizeof (struct searchdim),
				PU_STATIC, NULL);
	for (qp = strtok(va("%s", q), " ");
			qp && fr->total < 255;
			qp = strtok(0, " "))
	{
		if (( sp = strcasestr(s, qp) ))
		{
			if (wanttable)
			{
				(*dimp)[(*cuntp)].pos = sp - s;
				(*dimp)[(*cuntp)].siz = strlen(qp);
			}
			(*cuntp)++;
			fr->total++;
		}
	}
	if (wanttable)
		(*dimp) = Z_Realloc((*dimp), (*cuntp) * sizeof (struct searchdim),
				PU_STATIC, NULL);
}

static void writesimplefreq(mapsearchfreq_t *fr, INT32 *frc,
		INT32 mapnum, UINT8 pos, UINT8 siz)
{
	fr[(*frc)].mapnum = mapnum;
	fr[(*frc)].matchd = ZZ_Alloc(sizeof (struct searchdim));
	fr[(*frc)].matchd[0].pos = pos;
	fr[(*frc)].matchd[0].siz = siz;
	fr[(*frc)].matchc = 1;
	fr[(*frc)].total = 1;
	(*frc)++;
}

INT32 G_FindMap(const char *mapname, char **foundmapnamep,
		mapsearchfreq_t **freqp, INT32 *freqcp)
{
	INT32 newmapnum = 0;
	INT32 mapnum;
	INT32 apromapnum = 0;

	size_t      mapnamelen;
	char   *realmapname = NULL;
	char   *newmapname = NULL;
	char   *apromapname = NULL;
	char   *aprop = NULL;

	mapsearchfreq_t *freq;
	boolean wanttable;
	INT32 freqc;
	UINT8 frequ;

	INT32 i;

	mapnamelen = strlen(mapname);

	/* Count available maps; how ugly. */
	for (i = 0, freqc = 0; i < NUMMAPS; ++i)
	{
		if (mapheaderinfo[i])
			freqc++;
	}

	freq = ZZ_Calloc(freqc * sizeof (mapsearchfreq_t));

	wanttable = !!( freqp );

	freqc = 0;
	for (i = 0, mapnum = 1; i < NUMMAPS; ++i, ++mapnum)
		if (mapheaderinfo[i])
	{
		if (!( realmapname = G_BuildMapTitle(mapnum) ))
			continue;

		aprop = realmapname;

		/* Now that we found a perfect match no need to fucking guess. */
		if (strnicmp(realmapname, mapname, mapnamelen) == 0)
		{
			if (wanttable)
			{
				writesimplefreq(freq, &freqc, mapnum, 0, mapnamelen);
			}
			if (newmapnum == 0)
			{
				newmapnum = mapnum;
				newmapname = realmapname;
				realmapname = 0;
				Z_Free(apromapname);
				if (!wanttable)
					break;
			}
		}
		else
		if (apromapnum == 0 || wanttable)
		{
			/* LEVEL 1--match keywords verbatim */
			if (( aprop = strcasestr(realmapname, mapname) ))
			{
				if (wanttable)
				{
					writesimplefreq(freq, &freqc,
							mapnum, aprop - realmapname, mapnamelen);
				}
				if (apromapnum == 0)
				{
					apromapnum = mapnum;
					apromapname = realmapname;
					realmapname = 0;
				}
			}
			else/* ...match individual keywords */
			{
				freq[freqc].mapnum = mapnum;
				measurekeywords(&freq[freqc],
						&freq[freqc].matchd, &freq[freqc].matchc,
						realmapname, mapname, wanttable);
				if (freq[freqc].total)
					freqc++;
			}
		}

		Z_Free(realmapname);/* leftover old name */
	}

	if (newmapnum == 0)/* no perfect match--try a substring */
	{
		newmapnum = apromapnum;
		newmapname = apromapname;
	}

	if (newmapnum == 0)/* calculate most queries met! */
	{
		frequ = 0;
		for (i = 0; i < freqc; ++i)
		{
			if (freq[i].total > frequ)
			{
				frequ = freq[i].total;
				newmapnum = freq[i].mapnum;
			}
		}
		if (newmapnum)
		{
			newmapname = G_BuildMapTitle(newmapnum);
		}
	}

	if (freqp)
		(*freqp) = freq;
	else
		Z_Free(freq);

	if (freqcp)
		(*freqcp) = freqc;

	if (foundmapnamep)
		(*foundmapnamep) = newmapname;
	else
		Z_Free(newmapname);

	return newmapnum;
}

void G_FreeMapSearch(mapsearchfreq_t *freq, INT32 freqc)
{
	INT32 i;
	for (i = 0; i < freqc; ++i)
	{
		Z_Free(freq[i].matchd);
	}
	Z_Free(freq);
}

//
// DEMO RECORDING
//

#define DEMOVERSION 0x000c
#define DEMOHEADER  "\xF0" "SRB2Replay" "\x0F"

#define DF_GHOST        0x01 // This demo contains ghost data too!
#define DF_RECORDATTACK 0x02 // This demo is from record attack and contains its final completion time, score, and rings!
#define DF_NIGHTSATTACK 0x04 // This demo is from NiGHTS attack and contains its time left, score, and mares!
#define DF_ATTACKMASK   0x06 // This demo is from ??? attack and contains ???
#define DF_ATTACKSHIFT  1

// For demos
#define ZT_FWD     0x01
#define ZT_SIDE    0x02
#define ZT_ANGLE   0x04
#define ZT_BUTTONS 0x08
#define ZT_AIMING  0x10
#define DEMOMARKER 0x80 // demoend
#define METALDEATH 0x44
#define METALSNICE 0x69

static ticcmd_t oldcmd;

// For Metal Sonic and time attack ghosts
#define GZT_XYZ    0x01
#define GZT_MOMXY  0x02
#define GZT_MOMZ   0x04
#define GZT_ANGLE  0x08
#define GZT_FRAME  0x10 // Animation frame
#define GZT_SPR2   0x20 // Player animations
#define GZT_EXTRA  0x40
#define GZT_FOLLOW 0x80 // Followmobj

// GZT_EXTRA flags
#define EZT_THOK   0x01 // Spawned a thok object
#define EZT_SPIN   0x02 // Because one type of thok object apparently wasn't enough
#define EZT_REV    0x03 // And two types wasn't enough either yet
#define EZT_THOKMASK 0x03
#define EZT_COLOR  0x04 // Changed color (Super transformation, Mario fireflowers/invulnerability, etc.)
#define EZT_FLIP   0x08 // Reversed gravity
#define EZT_SCALE  0x10 // Changed size
#define EZT_HIT    0x20 // Damaged a mobj
#define EZT_SPRITE 0x40 // Changed sprite set completely out of PLAY (NiGHTS, SOCs, whatever)
#define EZT_HEIGHT 0x80 // Changed height

// GZT_FOLLOW flags
#define FZT_SPAWNED 0x01 // just been spawned
#define FZT_SKIN 0x02 // has skin
#define FZT_LINKDRAW 0x04 // has linkdraw (combine with spawned only)
#define FZT_COLORIZED 0x08 // colorized (ditto)
#define FZT_SCALE 0x10 // different scale to object
// spare FZT slots 0x20 to 0x80

static mobj_t oldmetal, oldghost;

void G_SaveMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	WRITEUINT32(*buffer, metal_p - metalbuffer);
}

void G_LoadMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	G_DoPlayMetal();
	metal_p = metalbuffer + READUINT32(*buffer);
}

ticcmd_t *G_CopyTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n)
{
	return M_Memcpy(dest, src, n*sizeof(*src));
}

ticcmd_t *G_MoveTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
	{
		dest[i].forwardmove = src[i].forwardmove;
		dest[i].sidemove = src[i].sidemove;
		dest[i].angleturn = SHORT(src[i].angleturn);
		dest[i].aiming = (INT16)SHORT(src[i].aiming);
		dest[i].buttons = (UINT16)SHORT(src[i].buttons);
	}
	return dest;
}

void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	UINT8 ziptic;
	(void)playernum;

	if (!demo_p || !demo_start)
		return;
	ziptic = READUINT8(demo_p);

	if (ziptic & ZT_FWD)
		oldcmd.forwardmove = READSINT8(demo_p);
	if (ziptic & ZT_SIDE)
		oldcmd.sidemove = READSINT8(demo_p);
	if (ziptic & ZT_ANGLE)
		oldcmd.angleturn = READINT16(demo_p);
	if (ziptic & ZT_BUTTONS)
		oldcmd.buttons = (oldcmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) | (READUINT16(demo_p) & ~(BT_CAMLEFT|BT_CAMRIGHT));
	if (ziptic & ZT_AIMING)
		oldcmd.aiming = READINT16(demo_p);

	G_CopyTiccmd(cmd, &oldcmd, 1);

	if (!(demoflags & DF_GHOST) && *demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	(void)playernum;

	if (!demo_p)
		return;
	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	if (cmd->forwardmove != oldcmd.forwardmove)
	{
		WRITEUINT8(demo_p,cmd->forwardmove);
		oldcmd.forwardmove = cmd->forwardmove;
		ziptic |= ZT_FWD;
	}

	if (cmd->sidemove != oldcmd.sidemove)
	{
		WRITEUINT8(demo_p,cmd->sidemove);
		oldcmd.sidemove = cmd->sidemove;
		ziptic |= ZT_SIDE;
	}

	if (cmd->angleturn != oldcmd.angleturn)
	{
		WRITEINT16(demo_p,cmd->angleturn);
		oldcmd.angleturn = cmd->angleturn;
		ziptic |= ZT_ANGLE;
	}

	if (cmd->buttons != oldcmd.buttons)
	{
		WRITEUINT16(demo_p,cmd->buttons);
		oldcmd.buttons = cmd->buttons;
		ziptic |= ZT_BUTTONS;
	}

	if (cmd->aiming != oldcmd.aiming)
	{
		WRITEINT16(demo_p,cmd->aiming);
		oldcmd.aiming = cmd->aiming;
		ziptic |= ZT_AIMING;
	}

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (!(demoflags & DF_GHOST) && ziptic_p > demoend - 9)
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

void G_GhostAddThok(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_THOK;
}

void G_GhostAddSpin(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_SPIN;
}

void G_GhostAddRev(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_REV;
}

void G_GhostAddFlip(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags |= EZT_FLIP;
}

void G_GhostAddColor(ghostcolor_t color)
{
	if (!demorecording || !(demoflags & DF_GHOST))
		return;
	if (ghostext.lastcolor == (UINT8)color)
	{
		ghostext.flags &= ~EZT_COLOR;
		return;
	}
	ghostext.flags |= EZT_COLOR;
	ghostext.color = (UINT8)color;
}

void G_GhostAddScale(fixed_t scale)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	if (ghostext.lastscale == scale)
	{
		ghostext.flags &= ~EZT_SCALE;
		return;
	}
	ghostext.flags |= EZT_SCALE;
	ghostext.scale = scale;
}

void G_GhostAddHit(mobj_t *victim)
{
	if (!demorecording || !(demoflags & DF_GHOST))
		return;
	ghostext.flags |= EZT_HIT;
	ghostext.hits++;
	ghostext.hitlist = Z_Realloc(ghostext.hitlist, ghostext.hits * sizeof(mobj_t *), PU_LEVEL, NULL);
	ghostext.hitlist[ghostext.hits-1] = victim;
}

void G_WriteGhostTic(mobj_t *ghost)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	UINT32 i;
	fixed_t height;

	if (!demo_p)
		return;
	if (!(demoflags & DF_GHOST))
		return; // No ghost data to write.

	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	#define MAXMOM (0xFFFF<<8)

	// GZT_XYZ is only useful if you've moved 256 FRACUNITS or more in a single tic.
	if (abs(ghost->x-oldghost.x) > MAXMOM
	|| abs(ghost->y-oldghost.y) > MAXMOM
	|| abs(ghost->z-oldghost.z) > MAXMOM)
	{
		oldghost.x = ghost->x;
		oldghost.y = ghost->y;
		oldghost.z = ghost->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldghost.x);
		WRITEFIXED(demo_p,oldghost.y);
		WRITEFIXED(demo_p,oldghost.z);
	}
	else
	{
		// For moving normally:
		// Store one full byte of movement, plus one byte of fractional movement.
		INT16 momx = (INT16)((ghost->x-oldghost.x)>>8);
		INT16 momy = (INT16)((ghost->y-oldghost.y)>>8);
		if (momx != oldghost.momx
		|| momy != oldghost.momy)
		{
			oldghost.momx = momx;
			oldghost.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEINT16(demo_p,momx);
			WRITEINT16(demo_p,momy);
		}
		momx = (INT16)((ghost->z-oldghost.z)>>8);
		if (momx != oldghost.momz)
		{
			oldghost.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEINT16(demo_p,momx);
		}

		// This SHOULD set oldghost.x/y/z to match ghost->x/y/z
		// but it keeps the fractional loss of one byte,
		// so it will hopefully be made up for in future tics.
		oldghost.x += oldghost.momx<<8;
		oldghost.y += oldghost.momy<<8;
		oldghost.z += oldghost.momz<<8;
	}

	#undef MAXMOM

	// Only store the 8 most relevant bits of angle
	// because exact values aren't too easy to discern to begin with when only 8 angles have different sprites
	// and it does not affect this mode of movement at all anyway.
	if (ghost->player && ghost->player->drawangle>>24 != oldghost.angle)
	{
		oldghost.angle = ghost->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldghost.angle);
	}

	// Store the sprite frame.
	if ((ghost->frame & FF_FRAMEMASK) != oldghost.frame)
	{
		oldghost.frame = (ghost->frame & FF_FRAMEMASK);
		ziptic |= GZT_FRAME;
		WRITEUINT8(demo_p,oldghost.frame);
	}

	if (ghost->sprite == SPR_PLAY
	&& ghost->sprite2 != oldghost.sprite2)
	{
		oldghost.sprite2 = ghost->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demo_p,oldghost.sprite2);
	}

	// Check for sprite set changes
	if (ghost->sprite != oldghost.sprite)
	{
		oldghost.sprite = ghost->sprite;
		ghostext.flags |= EZT_SPRITE;
	}

	if ((height = FixedDiv(ghost->height, ghost->scale)) != oldghost.height)
	{
		oldghost.height = height;
		ghostext.flags |= EZT_HEIGHT;
	}

	if (ghostext.flags)
	{
		ziptic |= GZT_EXTRA;

		if (ghostext.color == ghostext.lastcolor)
			ghostext.flags &= ~EZT_COLOR;
		if (ghostext.scale == ghostext.lastscale)
			ghostext.flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext.flags);
		if (ghostext.flags & EZT_COLOR)
		{
			WRITEUINT8(demo_p,ghostext.color);
			ghostext.lastcolor = ghostext.color;
		}
		if (ghostext.flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext.scale);
			ghostext.lastscale = ghostext.scale;
		}
		if (ghostext.flags & EZT_HIT)
		{
			WRITEUINT16(demo_p,ghostext.hits);
			for (i = 0; i < ghostext.hits; i++)
			{
				mobj_t *mo = ghostext.hitlist[i];
				//WRITEUINT32(demo_p,UINT32_MAX); // reserved for some method of determining exactly which mobj this is. (mobjnum doesn't work here.)
				WRITEUINT32(demo_p,mo->type);
				WRITEUINT16(demo_p,(UINT16)mo->health);
				WRITEFIXED(demo_p,mo->x);
				WRITEFIXED(demo_p,mo->y);
				WRITEFIXED(demo_p,mo->z);
				WRITEANGLE(demo_p,mo->angle);
			}
			Z_Free(ghostext.hitlist);
			ghostext.hits = 0;
			ghostext.hitlist = NULL;
		}
		if (ghostext.flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldghost.sprite);
		if (ghostext.flags & EZT_HEIGHT)
		{
			height >>= FRACBITS;
			WRITEINT16(demo_p, height);
		}
		ghostext.flags = 0;
	}

	if (ghost->player && ghost->player->followmobj && !(ghost->player->followmobj->sprite == SPR_NULL || (ghost->player->followmobj->flags2 & MF2_DONTDRAW))) // bloats tails runs but what can ya do
	{
		INT16 temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (ghost->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldghost.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,ghost->player->followmobj->info->height>>FRACBITS);
			if (ghost->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (ghost->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)(ghost->player->followmobj->skin))-skins));
			oldghost.flags2 |= MF2_AMBUSH;
		}

		if (ghost->player->followmobj->scale != ghost->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,ghost->player->followmobj->scale);
		}

		temp = (INT16)((ghost->player->followmobj->x-ghost->x)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((ghost->player->followmobj->y-ghost->y)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((ghost->player->followmobj->z-ghost->z)>>8);
		WRITEINT16(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demo_p,ghost->player->followmobj->sprite2);
		WRITEUINT16(demo_p,ghost->player->followmobj->sprite);
		WRITEUINT8(demo_p,(ghost->player->followmobj->frame & FF_FRAMEMASK));
		WRITEUINT8(demo_p,ghost->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldghost.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (demo_p >= demoend - (13 + 9 + 9))
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

// Uses ghost data to do consistency checks on your position.
// This fixes desynchronising demos when fighting eggman.
void G_ConsGhostTic(void)
{
	UINT8 ziptic;
	UINT16 px,py,pz,gx,gy,gz;
	mobj_t *testmo;

	if (!demo_p || !demo_start)
		return;
	if (!(demoflags & DF_GHOST))
		return; // No ghost data to use.

	testmo = players[0].mo;

	// Grab ghost data.
	ziptic = READUINT8(demo_p);
	if (ziptic & GZT_XYZ)
	{
		oldghost.x = READFIXED(demo_p);
		oldghost.y = READFIXED(demo_p);
		oldghost.z = READFIXED(demo_p);
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldghost.momx = READINT16(demo_p)<<8;
			oldghost.momy = READINT16(demo_p)<<8;
		}
		if (ziptic & GZT_MOMZ)
			oldghost.momz = READINT16(demo_p)<<8;
		oldghost.x += oldghost.momx;
		oldghost.y += oldghost.momy;
		oldghost.z += oldghost.momz;
	}
	if (ziptic & GZT_ANGLE)
		demo_p++;
	if (ziptic & GZT_FRAME)
		demo_p++;
	if (ziptic & GZT_SPR2)
		demo_p++;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		UINT8 xziptic = READUINT8(demo_p);
		if (xziptic & EZT_COLOR)
			demo_p++;
		if (xziptic & EZT_SCALE)
			demo_p += sizeof(fixed_t);
		if (xziptic & EZT_HIT)
		{ // Resync mob damage.
			UINT16 i, count = READUINT16(demo_p);
			thinker_t *th;
			mobj_t *mobj;

			UINT32 type;
			UINT16 health;
			fixed_t x;
			fixed_t y;
			fixed_t z;

			for (i = 0; i < count; i++)
			{
				//demo_p += 4; // reserved.
				type = READUINT32(demo_p);
				health = READUINT16(demo_p);
				x = READFIXED(demo_p);
				y = READFIXED(demo_p);
				z = READFIXED(demo_p);
				demo_p += sizeof(angle_t); // angle, unnecessary for cons.

				mobj = NULL;
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;
					mobj = (mobj_t *)th;
					if (mobj->type == (mobjtype_t)type && mobj->x == x && mobj->y == y && mobj->z == z)
						break;
				}
				if (th != &thlist[THINK_MOBJ] && mobj->health != health) // Wasn't damaged?! This is desync! Fix it!
				{
					if (demosynced)
						CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced!\n"));
					demosynced = false;
					P_DamageMobj(mobj, players[0].mo, players[0].mo, 1, 0);
				}
			}
		}
		if (xziptic & EZT_SPRITE)
			demo_p += sizeof(UINT16);
		if (xziptic & EZT_HEIGHT)
			demo_p += sizeof(INT16);
	}

	if (ziptic & GZT_FOLLOW)
	{ // Even more...
		UINT8 followtic = READUINT8(demo_p);
		if (followtic & FZT_SPAWNED)
		{
			demo_p += sizeof(INT16);
			if (followtic & FZT_SKIN)
				demo_p++;
		}
		if (followtic & FZT_SCALE)
			demo_p += sizeof(fixed_t);
		demo_p += sizeof(INT16);
		demo_p += sizeof(INT16);
		demo_p += sizeof(INT16);
		if (followtic & FZT_SKIN)
			demo_p++;
		demo_p += sizeof(UINT16);
		demo_p++;
		demo_p++;
	}

	// Re-synchronise
	px = testmo->x>>FRACBITS;
	py = testmo->y>>FRACBITS;
	pz = testmo->z>>FRACBITS;
	gx = oldghost.x>>FRACBITS;
	gy = oldghost.y>>FRACBITS;
	gz = oldghost.z>>FRACBITS;

	if (px != gx || py != gy || pz != gz)
	{
		if (demosynced)
			CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced!\n"));
		demosynced = false;

		P_UnsetThingPosition(testmo);
		testmo->x = oldghost.x;
		testmo->y = oldghost.y;
		P_SetThingPosition(testmo);
		testmo->z = oldghost.z;
	}

	if (*demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_GhostTicker(void)
{
	demoghost *g,*p;
	for(g = ghosts, p = NULL; g; g = g->next)
	{
		// Skip normal demo data.
		UINT8 ziptic = READUINT8(g->p);
		UINT8 xziptic = 0;
		if (ziptic & ZT_FWD)
			g->p++;
		if (ziptic & ZT_SIDE)
			g->p++;
		if (ziptic & ZT_ANGLE)
			g->p += 2;
		if (ziptic & ZT_BUTTONS)
			g->p += 2;
		if (ziptic & ZT_AIMING)
			g->p += 2;

		// Grab ghost data.
		ziptic = READUINT8(g->p);
		if (ziptic & GZT_XYZ)
		{
			g->oldmo.x = READFIXED(g->p);
			g->oldmo.y = READFIXED(g->p);
			g->oldmo.z = READFIXED(g->p);
		}
		else
		{
			if (ziptic & GZT_MOMXY)
			{
				g->oldmo.momx = READINT16(g->p)<<8;
				g->oldmo.momy = READINT16(g->p)<<8;
			}
			if (ziptic & GZT_MOMZ)
				g->oldmo.momz = READINT16(g->p)<<8;
			g->oldmo.x += g->oldmo.momx;
			g->oldmo.y += g->oldmo.momy;
			g->oldmo.z += g->oldmo.momz;
		}
		if (ziptic & GZT_ANGLE)
			g->mo->angle = READUINT8(g->p)<<24;
		if (ziptic & GZT_FRAME)
			g->oldmo.frame = READUINT8(g->p);
		if (ziptic & GZT_SPR2)
			g->oldmo.sprite2 = READUINT8(g->p);

		// Update ghost
		P_UnsetThingPosition(g->mo);
		g->mo->x = g->oldmo.x;
		g->mo->y = g->oldmo.y;
		g->mo->z = g->oldmo.z;
		P_SetThingPosition(g->mo);
		g->mo->frame = g->oldmo.frame | tr_trans30<<FF_TRANSSHIFT;
		if (g->fadein)
		{
			g->mo->frame += (((--g->fadein)/6)<<FF_TRANSSHIFT); // this calc never exceeds 9 unless g->fadein is bad, and it's only set once, so...
			g->mo->flags2 &= ~MF2_DONTDRAW;
		}
		g->mo->sprite2 = g->oldmo.sprite2;

		if (ziptic & GZT_EXTRA)
		{ // But wait, there's more!
			xziptic = READUINT8(g->p);
			if (xziptic & EZT_COLOR)
			{
				g->color = READUINT8(g->p);
				switch(g->color)
				{
				default:
				case GHC_RETURNSKIN:
					g->mo->skin = g->oldmo.skin;
					/* FALLTHRU */
				case GHC_NORMAL: // Go back to skin color
					g->mo->color = g->oldmo.color;
					break;
				// Handled below
				case GHC_SUPER:
				case GHC_INVINCIBLE:
					break;
				case GHC_FIREFLOWER: // Fireflower
					g->mo->color = SKINCOLOR_WHITE;
					break;
				case GHC_NIGHTSSKIN: // not actually a colour
					g->mo->skin = &skins[DEFAULTNIGHTSSKIN];
					break;
				}
			}
			if (xziptic & EZT_FLIP)
				g->mo->eflags ^= MFE_VERTICALFLIP;
			if (xziptic & EZT_SCALE)
			{
				g->mo->destscale = READFIXED(g->p);
				if (g->mo->destscale != g->mo->scale)
					P_SetScale(g->mo, g->mo->destscale);
			}
			if (xziptic & EZT_THOKMASK)
			{ // Let's only spawn ONE of these per frame, thanks.
				mobj_t *mobj;
				INT32 type = -1;
				if (g->mo->skin)
				{
					skin_t *skin = (skin_t *)g->mo->skin;
					switch (xziptic & EZT_THOKMASK)
					{
					case EZT_THOK:
						type = skin->thokitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].painchance : (UINT32)skin->thokitem;
						break;
					case EZT_SPIN:
						type = skin->spinitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].damage : (UINT32)skin->spinitem;
						break;
					case EZT_REV:
						type = skin->revitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].raisestate : (UINT32)skin->revitem;
						break;
					}
				}
				if (type != MT_NULL)
				{
					if (type == MT_GHOST)
					{
						mobj = P_SpawnGhostMobj(g->mo); // does a large portion of the work for us
						mobj->frame = (mobj->frame & ~FF_FRAMEMASK)|tr_trans60<<FF_TRANSSHIFT; // P_SpawnGhostMobj sets trans50, we want trans60
					}
					else
					{
						mobj = P_SpawnMobjFromMobj(g->mo, 0, 0, -FixedDiv(FixedMul(g->mo->info->height, g->mo->scale) - g->mo->height,3*FRACUNIT), MT_THOK);
						mobj->sprite = states[mobjinfo[type].spawnstate].sprite;
						mobj->frame = (states[mobjinfo[type].spawnstate].frame & FF_FRAMEMASK) | tr_trans60<<FF_TRANSSHIFT;
						mobj->color = g->mo->color;
						mobj->skin = g->mo->skin;
						P_SetScale(mobj, (mobj->destscale = g->mo->scale));

						if (type == MT_THOK) // spintrail-specific modification for MT_THOK
						{
							mobj->frame = FF_TRANS80;
							mobj->fuse = mobj->tics;
						}
						mobj->tics = -1; // nope.
					}
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_UnsetThingPosition(mobj);
					mobj->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
					P_SetThingPosition(mobj);
					if (!mobj->fuse)
						mobj->fuse = 8;
					P_SetTarget(&mobj->target, g->mo);
				}
			}
			if (xziptic & EZT_HIT)
			{ // Spawn hit poofs for killing things!
				UINT16 i, count = READUINT16(g->p), health;
				UINT32 type;
				fixed_t x,y,z;
				angle_t angle;
				mobj_t *poof;
				for (i = 0; i < count; i++)
				{
					//g->p += 4; // reserved
					type = READUINT32(g->p);
					health = READUINT16(g->p);
					x = READFIXED(g->p);
					y = READFIXED(g->p);
					z = READFIXED(g->p);
					angle = READANGLE(g->p);
					if (!(mobjinfo[type].flags & MF_SHOOTABLE)
					|| !(mobjinfo[type].flags & (MF_ENEMY|MF_MONITOR))
					|| health != 0 || i >= 4) // only spawn for the first 4 hits per frame, to prevent ghosts from splode-spamming too bad.
						continue;
					poof = P_SpawnMobj(x, y, z, MT_GHOST);
					poof->angle = angle;
					poof->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
					poof->health = 0;
					P_SetMobjStateNF(poof, S_XPLD1);
				}
			}
			if (xziptic & EZT_SPRITE)
				g->mo->sprite = READUINT16(g->p);
			if (xziptic & EZT_HEIGHT)
			{
				fixed_t temp = READINT16(g->p)<<FRACBITS;
				g->mo->height = FixedMul(temp, g->mo->scale);
			}
		}

		// Tick ghost colors (Super and Mario Invincibility flashing)
		switch(g->color)
		{
		case GHC_SUPER: // Super (P_DoSuperStuff)
			if (g->mo->skin)
			{
				skin_t *skin = (skin_t *)g->mo->skin;
				g->mo->color = skin->supercolor;
			}
			else
				g->mo->color = SKINCOLOR_SUPERGOLD1;
			g->mo->color += abs( ( (signed)( (unsigned)leveltime >> 1 ) % 9) - 4);
			break;
		case GHC_INVINCIBLE: // Mario invincibility (P_CheckInvincibilityTimer)
			g->mo->color = (UINT8)(SKINCOLOR_RUBY + (leveltime % (MAXSKINCOLORS - SKINCOLOR_RUBY))); // Passes through all saturated colours
			break;
		default:
			break;
		}

#define follow g->mo->tracer
		if (ziptic & GZT_FOLLOW)
		{ // Even more...
			UINT8 followtic = READUINT8(g->p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(g->mo, 0, 0, 0, MT_GHOST));
				P_SetTarget(&follow->tracer, g->mo);
				follow->tics = -1;
				temp = READINT16(g->p)<<FRACBITS;
				follow->height = FixedMul(follow->scale, temp);

				if (followtic & FZT_LINKDRAW)
					follow->flags2 |= MF2_LINKDRAW;

				if (followtic & FZT_COLORIZED)
					follow->colorized = true;

				if (followtic & FZT_SKIN)
					follow->skin = &skins[READUINT8(g->p)];
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(g->p);
				else
					follow->destscale = g->mo->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale);

				P_UnsetThingPosition(follow);
				temp = READINT16(g->p)<<8;
				follow->x = g->mo->x + temp;
				temp = READINT16(g->p)<<8;
				follow->y = g->mo->y + temp;
				temp = READINT16(g->p)<<8;
				follow->z = g->mo->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = READUINT8(g->p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(g->p);
				follow->frame = (READUINT8(g->p)) | (g->mo->frame & FF_TRANSMASK);
				follow->angle = g->mo->angle;
				follow->color = READUINT8(g->p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}
		// Demo ends after ghost data.
		if (*g->p == DEMOMARKER)
		{
			g->mo->momx = g->mo->momy = g->mo->momz = 0;
#if 1 // freeze frame (maybe more useful for time attackers)
			g->mo->colorized = true;
			if (follow)
				follow->colorized = true;
#else // dissapearing act
			g->mo->fuse = TICRATE;
			if (follow)
				follow->fuse = TICRATE;
#endif
			if (p)
				p->next = g->next;
			else
				ghosts = g->next;
			Z_Free(g);
			continue;
		}
		p = g;
#undef follow
	}
}

void G_ReadMetalTic(mobj_t *metal)
{
	UINT8 ziptic;
	UINT8 xziptic = 0;

	if (!metal_p)
		return;

	if (!metal->health)
	{
		G_StopMetalDemo();
		return;
	}

	switch (*metal_p)
	{
		case METALSNICE:
			break;
		case METALDEATH:
			if (metal->tracer)
				P_RemoveMobj(metal->tracer);
			P_KillMobj(metal, NULL, NULL, 0);
			/* FALLTHRU */
		case DEMOMARKER:
		default:
			// end of demo data stream
			G_StopMetalDemo();
			return;
	}
	metal_p++;

	ziptic = READUINT8(metal_p);

	// Read changes from the tic
	if (ziptic & GZT_XYZ)
	{
		P_TeleportMove(metal, READFIXED(metal_p), READFIXED(metal_p), READFIXED(metal_p));
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldmetal.momx = READINT16(metal_p)<<8;
			oldmetal.momy = READINT16(metal_p)<<8;
		}
		if (ziptic & GZT_MOMZ)
			oldmetal.momz = READINT16(metal_p)<<8;
		oldmetal.x += oldmetal.momx;
		oldmetal.y += oldmetal.momy;
		oldmetal.z += oldmetal.momz;
	}
	if (ziptic & GZT_ANGLE)
		metal->angle = READUINT8(metal_p)<<24;
	if (ziptic & GZT_FRAME)
		oldmetal.frame = READUINT32(metal_p);
	if (ziptic & GZT_SPR2)
		oldmetal.sprite2 = READUINT8(metal_p);

	// Set movement, position, and angle
	// oldmetal contains where you're supposed to be.
	metal->momx = oldmetal.momx;
	metal->momy = oldmetal.momy;
	metal->momz = oldmetal.momz;
	P_UnsetThingPosition(metal);
	metal->x = oldmetal.x;
	metal->y = oldmetal.y;
	metal->z = oldmetal.z;
	P_SetThingPosition(metal);
	metal->frame = oldmetal.frame;
	metal->sprite2 = oldmetal.sprite2;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		xziptic = READUINT8(metal_p);
		if (xziptic & EZT_FLIP)
		{
			metal->eflags ^= MFE_VERTICALFLIP;
			metal->flags2 ^= MF2_OBJECTFLIP;
		}
		if (xziptic & EZT_SCALE)
		{
			metal->destscale = READFIXED(metal_p);
			if (metal->destscale != metal->scale)
				P_SetScale(metal, metal->destscale);
		}
		if (xziptic & EZT_THOKMASK)
		{ // Let's only spawn ONE of these per frame, thanks.
			mobj_t *mobj;
			INT32 type = -1;
			if (metal->skin)
			{
				skin_t *skin = (skin_t *)metal->skin;
				switch (xziptic & EZT_THOKMASK)
				{
				case EZT_THOK:
					type = skin->thokitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].painchance : (UINT32)skin->thokitem;
					break;
				case EZT_SPIN:
					type = skin->spinitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].damage : (UINT32)skin->spinitem;
					break;
				case EZT_REV:
					type = skin->revitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].raisestate : (UINT32)skin->revitem;
					break;
				}
			}
			if (type != MT_NULL)
			{
				if (type == MT_GHOST)
				{
					mobj = P_SpawnGhostMobj(metal); // does a large portion of the work for us
				}
				else
				{
					mobj = P_SpawnMobjFromMobj(metal, 0, 0, -FixedDiv(FixedMul(metal->info->height, metal->scale) - metal->height,3*FRACUNIT), MT_THOK);
					mobj->sprite = states[mobjinfo[type].spawnstate].sprite;
					mobj->frame = states[mobjinfo[type].spawnstate].frame;
					mobj->angle = metal->angle;
					mobj->color = metal->color;
					mobj->skin = metal->skin;
					P_SetScale(mobj, (mobj->destscale = metal->scale));

					if (type == MT_THOK) // spintrail-specific modification for MT_THOK
					{
						mobj->frame = FF_TRANS70;
						mobj->fuse = mobj->tics;
					}
					mobj->tics = -1; // nope.
				}
				mobj->floorz = mobj->z;
				mobj->ceilingz = mobj->z+mobj->height;
				P_UnsetThingPosition(mobj);
				mobj->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
				P_SetThingPosition(mobj);
				if (!mobj->fuse)
					mobj->fuse = 8;
				P_SetTarget(&mobj->target, metal);
			}
		}
		if (xziptic & EZT_SPRITE)
			metal->sprite = READUINT16(metal_p);
		if (xziptic & EZT_HEIGHT)
		{
			fixed_t temp = READINT16(metal_p)<<FRACBITS;
			metal->height = FixedMul(temp, metal->scale);
		}
	}

#define follow metal->tracer
		if (ziptic & GZT_FOLLOW)
		{ // Even more...
			UINT8 followtic = READUINT8(metal_p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(metal, 0, 0, 0, MT_GHOST));
				P_SetTarget(&follow->tracer, metal);
				follow->tics = -1;
				temp = READINT16(metal_p)<<FRACBITS;
				follow->height = FixedMul(follow->scale, temp);

				if (followtic & FZT_LINKDRAW)
					follow->flags2 |= MF2_LINKDRAW;

				if (followtic & FZT_COLORIZED)
					follow->colorized = true;

				if (followtic & FZT_SKIN)
					follow->skin = &skins[READUINT8(metal_p)];
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(metal_p);
				else
					follow->destscale = metal->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale);

				P_UnsetThingPosition(follow);
				temp = READINT16(metal_p)<<8;
				follow->x = metal->x + temp;
				temp = READINT16(metal_p)<<8;
				follow->y = metal->y + temp;
				temp = READINT16(metal_p)<<8;
				follow->z = metal->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = READUINT8(metal_p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(metal_p);
				follow->frame = READUINT32(metal_p); // NOT & FF_FRAMEMASK here, so 32 bits
				follow->angle = metal->angle;
				follow->color = READUINT8(metal_p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}
#undef follow
}

void G_WriteMetalTic(mobj_t *metal)
{
	UINT8 ziptic = 0;
	UINT8 *ziptic_p;
	fixed_t height;

	if (!demo_p) // demo_p will be NULL until the race start linedef executor is activated!
		return;

	WRITEUINT8(demo_p, METALSNICE);
	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	#define MAXMOM (0xFFFF<<8)

	// GZT_XYZ is only useful if you've moved 256 FRACUNITS or more in a single tic.
	if (abs(metal->x-oldmetal.x) > MAXMOM
	|| abs(metal->y-oldmetal.y) > MAXMOM
	|| abs(metal->z-oldmetal.z) > MAXMOM)
	{
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldmetal.x);
		WRITEFIXED(demo_p,oldmetal.y);
		WRITEFIXED(demo_p,oldmetal.z);
	}
	else
	{
		// For moving normally:
		// Store one full byte of movement, plus one byte of fractional movement.
		INT16 momx = (INT16)((metal->x-oldmetal.x)>>8);
		INT16 momy = (INT16)((metal->y-oldmetal.y)>>8);
		if (momx != oldmetal.momx
		|| momy != oldmetal.momy)
		{
			oldmetal.momx = momx;
			oldmetal.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEINT16(demo_p,momx);
			WRITEINT16(demo_p,momy);
		}
		momx = (INT16)((metal->z-oldmetal.z)>>8);
		if (momx != oldmetal.momz)
		{
			oldmetal.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEINT16(demo_p,momx);
		}

		// This SHOULD set oldmetal.x/y/z to match metal->x/y/z
		// but it keeps the fractional loss of one byte,
		// so it will hopefully be made up for in future tics.
		oldmetal.x += oldmetal.momx<<8;
		oldmetal.y += oldmetal.momy<<8;
		oldmetal.z += oldmetal.momz<<8;
	}

	#undef MAXMOM

	// Only store the 8 most relevant bits of angle
	// because exact values aren't too easy to discern to begin with when only 8 angles have different sprites
	// and it does not affect movement at all anyway.
	if (metal->player && metal->player->drawangle>>24 != oldmetal.angle)
	{
		oldmetal.angle = metal->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldmetal.angle);
	}

	// Store the sprite frame.
	if ((metal->frame & FF_FRAMEMASK) != oldmetal.frame)
	{
		oldmetal.frame = metal->frame; // NOT & FF_FRAMEMASK here, so 32 bits
		ziptic |= GZT_FRAME;
		WRITEUINT32(demo_p,oldmetal.frame);
	}

	if (metal->sprite == SPR_PLAY
	&& metal->sprite2 != oldmetal.sprite2)
	{
		oldmetal.sprite2 = metal->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demo_p,oldmetal.sprite2);
	}

	// Check for sprite set changes
	if (metal->sprite != oldmetal.sprite)
	{
		oldmetal.sprite = metal->sprite;
		ghostext.flags |= EZT_SPRITE;
	}

	if ((height = FixedDiv(metal->height, metal->scale)) != oldmetal.height)
	{
		oldmetal.height = height;
		ghostext.flags |= EZT_HEIGHT;
	}

	if (ghostext.flags & ~(EZT_COLOR|EZT_HIT)) // these two aren't handled by metal ever
	{
		ziptic |= GZT_EXTRA;

		if (ghostext.scale == ghostext.lastscale)
			ghostext.flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext.flags);
		if (ghostext.flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext.scale);
			ghostext.lastscale = ghostext.scale;
		}
		if (ghostext.flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldmetal.sprite);
		if (ghostext.flags & EZT_HEIGHT)
		{
			height >>= FRACBITS;
			WRITEINT16(demo_p, height);
		}
		ghostext.flags = 0;
	}

	if (metal->player && metal->player->followmobj && !(metal->player->followmobj->sprite == SPR_NULL || (metal->player->followmobj->flags2 & MF2_DONTDRAW)))
	{
		INT16 temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (metal->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldmetal.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,metal->player->followmobj->info->height>>FRACBITS);
			if (metal->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (metal->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)(metal->player->followmobj->skin))-skins));
			oldmetal.flags2 |= MF2_AMBUSH;
		}

		if (metal->player->followmobj->scale != metal->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,metal->player->followmobj->scale);
		}

		temp = (INT16)((metal->player->followmobj->x-metal->x)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((metal->player->followmobj->y-metal->y)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((metal->player->followmobj->z-metal->z)>>8);
		WRITEINT16(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demo_p,metal->player->followmobj->sprite2);
		WRITEUINT16(demo_p,metal->player->followmobj->sprite);
		WRITEUINT32(demo_p,metal->player->followmobj->frame); // NOT & FF_FRAMEMASK here, so 32 bits
		WRITEUINT8(demo_p,metal->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldmetal.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (demo_p >= demoend - 32)
	{
		G_StopMetalRecording(false); // no more space
		return;
	}
}

//
// G_RecordDemo
//
void G_RecordDemo(const char *name)
{
	INT32 maxsize;

	strcpy(demoname, name);
	strcat(demoname, ".lmp");
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
//	if (demobuffer)
//		free(demobuffer);
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;

	demorecording = true;
}

void G_RecordMetal(void)
{
	INT32 maxsize;
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;
	metalrecording = true;
}

void G_BeginRecording(void)
{
	UINT8 i;
	char name[16];
	player_t *player = &players[consoleplayer];

	if (demo_p)
		return;
	memset(name,0,sizeof(name));

	demo_p = demobuffer;
	demoflags = DF_GHOST|(modeattacking<<DF_ATTACKSHIFT);

	// Setup header.
	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);

	// demo checksum
	demo_p += 16;

	// game data
	M_Memcpy(demo_p, "PLAY", 4); demo_p += 4;
	WRITEINT16(demo_p,gamemap);
	M_Memcpy(demo_p, mapmd5, 16); demo_p += 16;

	WRITEUINT8(demo_p,demoflags);
	switch ((demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
	case ATTACKING_NONE: // 0
		break;
	case ATTACKING_RECORD: // 1
		demotime_p = demo_p;
		WRITEUINT32(demo_p,UINT32_MAX); // time
		WRITEUINT32(demo_p,0); // score
		WRITEUINT16(demo_p,0); // rings
		break;
	case ATTACKING_NIGHTS: // 2
		demotime_p = demo_p;
		WRITEUINT32(demo_p,UINT32_MAX); // time
		WRITEUINT32(demo_p,0); // score
		break;
	default: // 3
		break;
	}

	WRITEUINT32(demo_p,P_GetInitSeed());

	// Name
	for (i = 0; i < 16 && cv_playername.string[i]; i++)
		name[i] = cv_playername.string[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;

	// Skin
	for (i = 0; i < 16 && cv_skin.string[i]; i++)
		name[i] = cv_skin.string[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;

	// Color
	for (i = 0; i < 16 && cv_playercolor.string[i]; i++)
		name[i] = cv_playercolor.string[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;

	// Stats
	WRITEUINT8(demo_p,player->charability);
	WRITEUINT8(demo_p,player->charability2);
	WRITEUINT8(demo_p,player->actionspd>>FRACBITS);
	WRITEUINT8(demo_p,player->mindash>>FRACBITS);
	WRITEUINT8(demo_p,player->maxdash>>FRACBITS);
	WRITEUINT8(demo_p,player->normalspeed>>FRACBITS);
	WRITEUINT8(demo_p,player->runspeed>>FRACBITS);
	WRITEUINT8(demo_p,player->thrustfactor);
	WRITEUINT8(demo_p,player->accelstart);
	WRITEUINT8(demo_p,player->acceleration);
	WRITEUINT8(demo_p,player->height>>FRACBITS);
	WRITEUINT8(demo_p,player->spinheight>>FRACBITS);
	WRITEUINT8(demo_p,player->camerascale>>FRACBITS);
	WRITEUINT8(demo_p,player->shieldscale>>FRACBITS);

	// Trying to convert it back to % causes demo desync due to precision loss.
	// Don't do it.
	WRITEFIXED(demo_p, player->jumpfactor);

	// And mobjtype_t is best with UINT32 too...
	WRITEUINT32(demo_p, player->followitem);

	// Save pflag data - see SendWeaponPref()
	{
		UINT8 buf = 0;
		pflags_t pflags = 0;
		if (cv_flipcam.value)
		{
			buf |= 0x01;
			pflags |= PF_FLIPCAM;
		}
		if (cv_analog.value)
		{
			buf |= 0x02;
			pflags |= PF_ANALOGMODE;
		}
		if (cv_directionchar.value)
		{
			buf |= 0x04;
			pflags |= PF_DIRECTIONCHAR;
		}
		if (cv_autobrake.value)
		{
			buf |= 0x08;
			pflags |= PF_AUTOBRAKE;
		}
		if (cv_usejoystick.value)
			buf |= 0x10;
		CV_SetValue(&cv_showinputjoy, !!(cv_usejoystick.value));

		WRITEUINT8(demo_p,buf);
		player->pflags = pflags;
	}

	// Save netvar data
	CV_SaveNetVars(&demo_p);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));
	memset(&ghostext,0,sizeof(ghostext));
	ghostext.lastcolor = ghostext.color = GHC_NORMAL;
	ghostext.lastscale = ghostext.scale = FRACUNIT;

	if (player->mo)
	{
		oldghost.x = player->mo->x;
		oldghost.y = player->mo->y;
		oldghost.z = player->mo->z;
		oldghost.angle = player->mo->angle>>24;

		// preticker started us gravity flipped
		if (player->mo->eflags & MFE_VERTICALFLIP)
			ghostext.flags |= EZT_FLIP;
	}
}

void G_BeginMetal(void)
{
	mobj_t *mo = players[consoleplayer].mo;

#if 0
	if (demo_p)
		return;
#endif

	demo_p = demobuffer;

	// Write header.
	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);

	// demo checksum
	demo_p += 16;

	M_Memcpy(demo_p, "METL", 4); demo_p += 4;

	memset(&ghostext,0,sizeof(ghostext));
	ghostext.lastscale = ghostext.scale = FRACUNIT;

	// Set up our memory.
	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	oldmetal.angle = mo->angle>>24;
}

void G_SetDemoTime(UINT32 ptime, UINT32 pscore, UINT16 prings)
{
	if (!demorecording || !demotime_p)
		return;
	if (demoflags & DF_RECORDATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, pscore);
		WRITEUINT16(demotime_p, prings);
		demotime_p = NULL;
	}
	else if (demoflags & DF_NIGHTSATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, pscore);
		demotime_p = NULL;
	}
}

// Returns bitfield:
// 1 == new demo has lower time
// 2 == new demo has higher score
// 4 == new demo has higher rings
UINT8 G_CmpDemoTime(char *oldname, char *newname)
{
	UINT8 *buffer,*p;
	UINT8 flags;
	UINT32 oldtime, newtime, oldscore, newscore;
	UINT16 oldrings, newrings, oldversion;
	size_t bufsize ATTRUNUSED;
	UINT8 c;
	UINT16 s ATTRUNUSED;
	UINT8 aflags = 0;

	// load the new file
	FIL_DefaultExtension(newname, ".lmp");
	bufsize = FIL_ReadFile(newname, &buffer);
	I_Assert(bufsize != 0);
	p = buffer;

	// read demo header
	I_Assert(!memcmp(p, DEMOHEADER, 12));
	p += 12; // DEMOHEADER
	c = READUINT8(p); // VERSION
	I_Assert(c == VERSION);
	c = READUINT8(p); // SUBVERSION
	I_Assert(c == SUBVERSION);
	s = READUINT16(p);
	I_Assert(s == DEMOVERSION);
	p += 16; // demo checksum
	I_Assert(!memcmp(p, "PLAY", 4));
	p += 4; // PLAY
	p += 2; // gamemap
	p += 16; // map md5
	flags = READUINT8(p); // demoflags

	aflags = flags & (DF_RECORDATTACK|DF_NIGHTSATTACK);
	I_Assert(aflags);
	if (flags & DF_RECORDATTACK)
	{
		newtime = READUINT32(p);
		newscore = READUINT32(p);
		newrings = READUINT16(p);
	}
	else if (flags & DF_NIGHTSATTACK)
	{
		newtime = READUINT32(p);
		newscore = READUINT32(p);
		newrings = 0;
	}
	else // appease compiler
		return 0;

	Z_Free(buffer);

	// load old file
	FIL_DefaultExtension(oldname, ".lmp");
	if (!FIL_ReadFile(oldname, &buffer))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), oldname);
		return UINT8_MAX;
	}
	p = buffer;

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 12; // DEMOHEADER
	p++; // VERSION
	p++; // SUBVERSION
	oldversion = READUINT16(p);
	switch(oldversion) // demoversion
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	p += 16; // demo checksum
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 4; // "PLAY"
	if (oldversion <= 0x0008)
		p++; // gamemap
	else
		p += 2; // gamemap
	p += 16; // mapmd5
	flags = READUINT8(p);
	if (!(flags & aflags))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' not from same game mode. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	if (flags & DF_RECORDATTACK)
	{
		oldtime = READUINT32(p);
		oldscore = READUINT32(p);
		oldrings = READUINT16(p);
	}
	else if (flags & DF_NIGHTSATTACK)
	{
		oldtime = READUINT32(p);
		oldscore = READUINT32(p);
		oldrings = 0;
	}
	else // appease compiler
		return UINT8_MAX;

	Z_Free(buffer);

	c = 0;
	if (newtime < oldtime
	|| (newtime == oldtime && (newscore > oldscore || newrings > oldrings)))
		c |= 1; // Better time
	if (newscore > oldscore
	|| (newscore == oldscore && newtime < oldtime))
		c |= 1<<1; // Better score
	if (newrings > oldrings
	|| (newrings == oldrings && newtime < oldtime))
		c |= 1<<2; // Better rings
	return c;
}

//
// G_PlayDemo
//
void G_DeferedPlayDemo(const char *name)
{
	COM_BufAddText("playdemo \"");
	COM_BufAddText(name);
	COM_BufAddText("\"\n");
}

//
// Start a demo from a .LMP file or from a wad resource
//
void G_DoPlayDemo(char *defdemoname)
{
	UINT8 i;
	lumpnum_t l;
	char skin[17],color[17],*n,*pdemoname;
	UINT8 version,subversion,charability,charability2,thrustfactor,accelstart,acceleration;
	pflags_t pflags;
	UINT32 randseed, followitem;
	fixed_t camerascale,shieldscale,actionspd,mindash,maxdash,normalspeed,runspeed,jumpfactor,height,spinheight;
	char msg[1024];

	skin[16] = '\0';
	color[16] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(defdemoname))
	{
		//FIL_DefaultExtension(defdemoname, ".lmp");
		if (!FIL_ReadFile(defdemoname, &demobuffer))
		{
			snprintf(msg, 1024, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			CONS_Alert(CONS_ERROR, "%s", msg);
			gameaction = ga_nothing;
			M_StartMessage(msg, NULL, MM_NOTHING);
			return;
		}
		demo_p = demobuffer;
	}
	// load demo resource from WAD
	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		snprintf(msg, 1024, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		gameaction = ga_nothing;
		M_StartMessage(msg, NULL, MM_NOTHING);
		return;
	}
	else // it's an internal demo
		demobuffer = demo_p = W_CacheLumpNum(l, PU_STATIC);

	// read demo header
	gameaction = ga_nothing;
	demoplayback = true;
	if (memcmp(demo_p, DEMOHEADER, 12))
	{
		snprintf(msg, 1024, M_GetText("%s is not a SRB2 replay file.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 12; // DEMOHEADER

	version = READUINT8(demo_p);
	subversion = READUINT8(demo_p);
	demoversion = READUINT16(demo_p);
	switch(demoversion)
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		snprintf(msg, 1024, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 16; // demo checksum
	if (memcmp(demo_p, "PLAY", 4))
	{
		snprintf(msg, 1024, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 4; // "PLAY"
	gamemap = READINT16(demo_p);
	demo_p += 16; // mapmd5

	demoflags = READUINT8(demo_p);
	modeattacking = (demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT;
	CON_ToggleOff();

	hu_demoscore = 0;
	hu_demotime = UINT32_MAX;
	hu_demorings = 0;

	switch (modeattacking)
	{
	case ATTACKING_NONE: // 0
		break;
	case ATTACKING_RECORD: // 1
		hu_demotime  = READUINT32(demo_p);
		hu_demoscore = READUINT32(demo_p);
		hu_demorings = READUINT16(demo_p);
		break;
	case ATTACKING_NIGHTS: // 2
		hu_demotime  = READUINT32(demo_p);
		hu_demoscore = READUINT32(demo_p);
		break;
	default: // 3
		modeattacking = ATTACKING_NONE;
		break;
	}

	// Random seed
	randseed = READUINT32(demo_p);

	// Player name
	M_Memcpy(player_names[0],demo_p,16);
	demo_p += 16;

	// Skin
	M_Memcpy(skin,demo_p,16);
	demo_p += 16;

	// Color
	M_Memcpy(color,demo_p,16);
	demo_p += 16;

	charability = READUINT8(demo_p);
	charability2 = READUINT8(demo_p);
	actionspd = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	mindash = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	maxdash = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	normalspeed = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	runspeed = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	thrustfactor = READUINT8(demo_p);
	accelstart = READUINT8(demo_p);
	acceleration = READUINT8(demo_p);
	height = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	spinheight = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	camerascale = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	shieldscale = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	jumpfactor = READFIXED(demo_p);
	followitem = READUINT32(demo_p);

	// pflag data
	{
		UINT8 buf = READUINT8(demo_p);
		pflags = 0;
		if (buf & 0x01)
			pflags |= PF_FLIPCAM;
		if (buf & 0x02)
			pflags |= PF_ANALOGMODE;
		if (buf & 0x04)
			pflags |= PF_DIRECTIONCHAR;
		if (buf & 0x08)
			pflags |= PF_AUTOBRAKE;
		CV_SetValue(&cv_showinputjoy, !!(buf & 0x10));
	}

	// net var data
	CV_LoadNetVars(&demo_p);

	// Sigh ... it's an empty demo.
	if (*demo_p == DEMOMARKER)
	{
		snprintf(msg, 1024, M_GetText("%s contains no data to be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}

	Z_Free(pdemoname);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));

	if (VERSION != version || SUBVERSION != subversion)
		CONS_Alert(CONS_WARNING, M_GetText("Demo version does not match game version. Desyncs may occur.\n"));

	// didn't start recording right away.
	demo_start = false;

	// Set skin
	SetPlayerSkin(0, skin);

#ifdef HAVE_BLUA
	LUAh_MapChange(gamemap);
#endif
	displayplayer = consoleplayer = 0;
	memset(playeringame,0,sizeof(playeringame));
	playeringame[0] = true;
	P_SetRandSeed(randseed);
	G_InitNew(false, G_BuildMapName(gamemap), true, true, false);

	// Set color
	for (i = 0; i < MAXSKINCOLORS; i++)
		if (!stricmp(Color_Names[i],color))
		{
			players[0].skincolor = i;
			break;
		}
	CV_StealthSetValue(&cv_playercolor, players[0].skincolor);
	if (players[0].mo)
	{
		players[0].mo->color = players[0].skincolor;
		oldghost.x = players[0].mo->x;
		oldghost.y = players[0].mo->y;
		oldghost.z = players[0].mo->z;
	}

	// Set saved attribute values
	// No cheat checking here, because even if they ARE wrong...
	// it would only break the replay if we clipped them.
	players[0].camerascale = camerascale;
	players[0].shieldscale = shieldscale;
	players[0].charability = charability;
	players[0].charability2 = charability2;
	players[0].actionspd = actionspd;
	players[0].mindash = mindash;
	players[0].maxdash = maxdash;
	players[0].normalspeed = normalspeed;
	players[0].runspeed = runspeed;
	players[0].thrustfactor = thrustfactor;
	players[0].accelstart = accelstart;
	players[0].acceleration = acceleration;
	players[0].height = height;
	players[0].spinheight = spinheight;
	players[0].jumpfactor = jumpfactor;
	players[0].followitem = followitem;
	players[0].pflags = pflags;

	demo_start = true;
}

void G_AddGhost(char *defdemoname)
{
	INT32 i;
	lumpnum_t l;
	char name[17],skin[17],color[17],*n,*pdemoname,md5[16];
	demoghost *gh;
	UINT8 flags;
	UINT8 *buffer,*p;
	mapthing_t *mthing;
	UINT16 count, ghostversion;

	name[16] = '\0';
	skin[16] = '\0';
	color[16] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(defdemoname))
	{
		//FIL_DefaultExtension(defdemoname, ".lmp");
		if (!FIL_ReadFileTag(defdemoname, &buffer, PU_LEVEL))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			Z_Free(pdemoname);
			return;
		}
		p = buffer;
	}
	// load demo resource from WAD
	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		Z_Free(pdemoname);
		return;
	}
	else // it's an internal demo
		buffer = p = W_CacheLumpNum(l, PU_LEVEL);

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Not a SRB2 replay.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 12; // DEMOHEADER
	p++; // VERSION
	p++; // SUBVERSION
	ghostversion = READUINT16(p);
	switch(ghostversion)
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo version incompatible.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}
	M_Memcpy(md5, p, 16); p += 16; // demo checksum
	for (gh = ghosts; gh; gh = gh->next)
		if (!memcmp(md5, gh->checksum, 16)) // another ghost in the game already has this checksum?
		{ // Don't add another one, then!
			CONS_Debug(DBG_SETUP, "Rejecting duplicate ghost %s (MD5 was matched)\n", pdemoname);
			Z_Free(pdemoname);
			Z_Free(buffer);
			return;
		}
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo format unacceptable.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 4; // "PLAY"
	if (ghostversion <= 0x0008)
		p++; // gamemap
	else
		p += 2; // gamemap
	p += 16; // mapmd5 (possibly check for consistency?)
	flags = READUINT8(p);
	if (!(flags & DF_GHOST))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: No ghost data in this demo.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}
	switch ((flags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
	case ATTACKING_NONE: // 0
		break;
	case ATTACKING_RECORD: // 1
		p += 10; // demo time, score, and rings
		break;
	case ATTACKING_NIGHTS: // 2
		p += 8; // demo time left, score
		break;
	default: // 3
		break;
	}

	p += 4; // random seed

	// Player name (TODO: Display this somehow if it doesn't match cv_playername!)
	M_Memcpy(name, p,16);
	p += 16;

	// Skin
	M_Memcpy(skin, p,16);
	p += 16;

	// Color
	M_Memcpy(color, p,16);
	p += 16;

	// Ghosts do not have a player structure to put this in.
	p++; // charability
	p++; // charability2
	p++; // actionspd
	p++; // mindash
	p++; // maxdash
	p++; // normalspeed
	p++; // runspeed
	p++; // thrustfactor
	p++; // accelstart
	p++; // acceleration
	p++; // height
	p++; // spinheight
	p++; // camerascale
	p++; // shieldscale
	p += 4; // jumpfactor
	p += 4; // followitem

	p++; // pflag data

	// net var data
	count = READUINT16(p);
	while (count--)
	{
		p += 2;
		SKIPSTRING(p);
		p++;
	}

	if (*p == DEMOMARKER)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Replay is empty.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	gh = Z_Calloc(sizeof(demoghost), PU_LEVEL, NULL);
	gh->next = ghosts;
	gh->buffer = buffer;
	M_Memcpy(gh->checksum, md5, 16);
	gh->p = p;

	ghosts = gh;

	gh->version = ghostversion;
	mthing = playerstarts[0];
	I_Assert(mthing);
	{ // A bit more complex than P_SpawnPlayer because ghosts aren't solid and won't just push themselves out of the ceiling.
		fixed_t z,f,c;
		gh->mo = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS, 0, MT_GHOST);
		gh->mo->angle = FixedAngle(mthing->angle*FRACUNIT);
		f = gh->mo->floorz;
		c = gh->mo->ceilingz - mobjinfo[MT_PLAYER].height;
		if (!!(mthing->options & MTF_AMBUSH) ^ !!(mthing->options & MTF_OBJECTFLIP))
		{
			z = c;
			if (mthing->options >> ZSHIFT)
				z -= ((mthing->options >> ZSHIFT) << FRACBITS);
			if (z < f)
				z = f;
		}
		else
		{
			z = f;
			if (mthing->options >> ZSHIFT)
				z += ((mthing->options >> ZSHIFT) << FRACBITS);
			if (z > c)
				z = c;
		}
		gh->mo->z = z;
	}

	gh->oldmo.x = gh->mo->x;
	gh->oldmo.y = gh->mo->y;
	gh->oldmo.z = gh->mo->z;

	// Set skin
	gh->mo->skin = &skins[0];
	for (i = 0; i < numskins; i++)
		if (!stricmp(skins[i].name,skin))
		{
			gh->mo->skin = &skins[i];
			break;
		}
	gh->oldmo.skin = gh->mo->skin;

	// Set color
	gh->mo->color = ((skin_t*)gh->mo->skin)->prefcolor;
	for (i = 0; i < MAXSKINCOLORS; i++)
		if (!stricmp(Color_Names[i],color))
		{
			gh->mo->color = (UINT8)i;
			break;
		}
	gh->oldmo.color = gh->mo->color;

	gh->mo->state = states+S_PLAY_STND;
	gh->mo->sprite = gh->mo->state->sprite;
	gh->mo->sprite2 = (gh->mo->state->frame & FF_FRAMEMASK);
	//gh->mo->frame = tr_trans30<<FF_TRANSSHIFT;
	gh->mo->flags2 |= MF2_DONTDRAW;
	gh->fadein = (9-3)*6; // fade from invisible to trans30 over as close to 35 tics as possible
	gh->mo->tics = -1;

	CONS_Printf(M_GetText("Added ghost %s from %s\n"), name, pdemoname);
	Z_Free(pdemoname);
}

//
// G_TimeDemo
// NOTE: name is a full filename for external demos
//
static INT32 restorecv_vidwait;

void G_TimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw");
	noblit = M_CheckParm("-noblit");
	restorecv_vidwait = cv_vidwait.value;
	if (cv_vidwait.value)
		CV_Set(&cv_vidwait, "0");
	timingdemo = true;
	singletics = true;
	framecount = 0;
	demostarttime = I_GetTime();
	G_DeferedPlayDemo(name);
}

void G_DoPlayMetal(void)
{
	lumpnum_t l;
	mobj_t *mo = NULL;
	thinker_t *th;

	// it's an internal demo
	if ((l = W_CheckNumForName(va("%sMS",G_BuildMapName(gamemap)))) == LUMPERROR)
	{
		CONS_Alert(CONS_WARNING, M_GetText("No bot recording for this map.\n"));
		return;
	}
	else
		metalbuffer = metal_p = W_CacheLumpNum(l, PU_STATIC);

	// find metal sonic
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;
		if (mo->type != MT_METALSONIC_RACE)
			continue;

		break;
	}
	if (th == &thlist[THINK_MOBJ])
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to find bot entity.\n"));
		Z_Free(metalbuffer);
		return;
	}

	// read demo header
    metal_p += 12; // DEMOHEADER
	metal_p++; // VERSION
	metal_p++; // SUBVERSION
	metalversion = READUINT16(metal_p);
	switch(metalversion)
	{
	case DEMOVERSION: // latest always supported
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, format version incompatible.\n"));
		Z_Free(metalbuffer);
		return;
	}
	metal_p += 16; // demo checksum
	if (memcmp(metal_p, "METL", 4))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, wasn't recorded in Metal format.\n"));
		Z_Free(metalbuffer);
		return;
	} metal_p += 4; // "METL"

	// read initial tic
	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	metalplayback = mo;
}

void G_DoneLevelLoad(void)
{
	CONS_Printf(M_GetText("Loaded level in %f sec\n"), (double)(I_GetTime() - demostarttime) / TICRATE);
	framecount = 0;
	demostarttime = I_GetTime();
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

// Stops metal sonic's demo. Separate from other functions because metal + replays can coexist
void G_StopMetalDemo(void)
{

	// Metal Sonic finishing doesn't end the game, dammit.
	Z_Free(metalbuffer);
	metalbuffer = NULL;
	metalplayback = NULL;
	metal_p = NULL;
}

// Stops metal sonic recording.
ATTRNORETURN void FUNCNORETURN G_StopMetalRecording(boolean kill)
{
	boolean saved = false;
	if (demo_p)
	{
		UINT8 *p = demobuffer+16; // checksum position
		if (kill)
			WRITEUINT8(demo_p, METALDEATH); // add the metal death marker
		else
			WRITEUINT8(demo_p, DEMOMARKER); // add the demo end marker
#ifdef NOMD5
		{
			UINT8 i;
			for (i = 0; i < 16; i++, p++)
				*p = P_RandomByte(); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
		}
#else
		md5_buffer((char *)p+16, demo_p - (p+16), (void *)p); // make a checksum of everything after the checksum in the file.
#endif
		saved = FIL_WriteFile(va("%sMS.LMP", G_BuildMapName(gamemap)), demobuffer, demo_p - demobuffer); // finally output the file.
	}
	free(demobuffer);
	metalrecording = false;
	if (saved)
		I_Error("Saved to %sMS.LMP", G_BuildMapName(gamemap));
	I_Error("Failed to save demo!");
}

// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void G_StopDemo(void)
{
	Z_Free(demobuffer);
	demobuffer = NULL;
	demoplayback = false;
	titledemo = false;
	timingdemo = false;
	singletics = false;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // cleanup

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;
	SV_StopServer();
	SV_ResetServer();
}

boolean G_CheckDemoStatus(void)
{
	boolean saved;

	while (ghosts)
	{
		demoghost *next = ghosts->next;
		Z_Free(ghosts);
		ghosts = next;
	}
	ghosts = NULL;


	// DO NOT end metal sonic demos here

	if (timingdemo)
	{
		INT32 demotime;
		double f1, f2;
		demotime = I_GetTime() - demostarttime;
		if (!demotime)
			return true;
		G_StopDemo();
		timingdemo = false;
		f1 = (double)demotime;
		f2 = (double)framecount*TICRATE;

		CONS_Printf(M_GetText("timed %u gametics in %d realtics - %u frames\n%f seconds, %f avg fps\n"),
			leveltime,demotime,(UINT32)framecount,f1/TICRATE,f2/f1);

		// CSV-readable timedemo results, for external parsing
		if (timedemo_csv)
		{
			FILE *f;
			const char *csvpath = va("%s"PATHSEP"%s", srb2home, "timedemo.csv");
			const char *header = "id,demoname,seconds,avgfps,leveltime,demotime,framecount,ticrate,rendermode,vidmode,vidwidth,vidheight,procbits\n";
			const char *rowformat = "\"%s\",\"%s\",%f,%f,%u,%d,%u,%u,%u,%u,%u,%u,%u\n";
			boolean headerrow = !FIL_FileExists(csvpath);
			UINT8 procbits = 0;

			// Bitness
			if (sizeof(void*) == 4)
				procbits = 32;
			else if (sizeof(void*) == 8)
				procbits = 64;

			f = fopen(csvpath, "a+");

			if (f)
			{
				if (headerrow)
					fputs(header, f);
				fprintf(f, rowformat,
					timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
				fclose(f);
				CONS_Printf("Timedemo results saved to '%s'\n", csvpath);
			}
			else
			{
				// Just print the CSV output to console
				CON_LogMessage(header);
				CONS_Printf(rowformat,
					timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
			}
		}

		if (restorecv_vidwait != cv_vidwait.value)
			CV_SetValue(&cv_vidwait, restorecv_vidwait);
		D_AdvanceDemo();
		return true;
	}

	if (demoplayback)
	{
		if (singledemo)
			I_Quit();
		G_StopDemo();

		if (modeattacking)
			M_EndModeAttackRun();
		else
			D_AdvanceDemo();

		return true;
	}

	if (demorecording)
	{
		UINT8 *p = demobuffer+16; // checksum position
#ifdef NOMD5
		UINT8 i;
		WRITEUINT8(demo_p, DEMOMARKER); // add the demo end marker
		for (i = 0; i < 16; i++, p++)
			*p = P_RandomByte(); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
#else
		WRITEUINT8(demo_p, DEMOMARKER); // add the demo end marker
		md5_buffer((char *)p+16, demo_p - (p+16), p); // make a checksum of everything after the checksum in the file.
#endif
		saved = FIL_WriteFile(va(pandf, srb2home, demoname), demobuffer, demo_p - demobuffer); // finally output the file.
		free(demobuffer);
		demorecording = false;

		if (modeattacking != ATTACKING_RECORD)
		{
			if (saved)
				CONS_Printf(M_GetText("Demo %s recorded\n"), demoname);
			else
				CONS_Alert(CONS_WARNING, M_GetText("Demo %s not saved\n"), demoname);
		}
		return true;
	}

	return false;
}

//
// G_SetGamestate
//
// Use this to set the gamestate, please.
//
void G_SetGamestate(gamestate_t newstate)
{
	gamestate = newstate;
}

/* These functions handle the exitgame flag. Before, when the user
   chose to end a game, it happened immediately, which could cause
   crashes if the game was in the middle of something. Now, a flag
   is set, and the game can then be stopped when it's safe to do
   so.
*/

// Used as a callback function.
void G_SetExitGameFlag(void)
{
	exitgame = true;
}

void G_ClearExitGameFlag(void)
{
	exitgame = false;
}

boolean G_GetExitGameFlag(void)
{
	return exitgame;
}

// Same deal with retrying.
void G_SetRetryFlag(void)
{
	retrying = true;
}

void G_ClearRetryFlag(void)
{
	retrying = false;
}

boolean G_GetRetryFlag(void)
{
	return retrying;
}

void G_SetModeAttackRetryFlag(void)
{
	retryingmodeattack = true;
	G_SetRetryFlag();
}

void G_ClearModeAttackRetryFlag(void)
{
	retryingmodeattack = false;
}

boolean G_GetModeAttackRetryFlag(void)
{
	return retryingmodeattack;
}

// Time utility functions
INT32 G_TicsToHours(tic_t tics)
{
	return tics/(3600*TICRATE);
}

INT32 G_TicsToMinutes(tic_t tics, boolean full)
{
	if (full)
		return tics/(60*TICRATE);
	else
		return tics/(60*TICRATE)%60;
}

INT32 G_TicsToSeconds(tic_t tics)
{
	return (tics/TICRATE)%60;
}

INT32 G_TicsToCentiseconds(tic_t tics)
{
	return (INT32)((tics%TICRATE) * (100.00f/TICRATE));
}

INT32 G_TicsToMilliseconds(tic_t tics)
{
	return (INT32)((tics%TICRATE) * (1000.00f/TICRATE));
}

