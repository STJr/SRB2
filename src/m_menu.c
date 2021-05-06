// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_menu.c
/// \brief XMOD's extremely revamped menu system.

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "m_menu.h"

#include "doomdef.h"
#include "d_main.h"
#include "d_netcmd.h"
#include "console.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"
#include "m_argv.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"
#include "i_threads.h"

// Addfile
#include "filesrch.h"

#include "v_video.h"
#include "i_video.h"
#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"
#include "p_setup.h"
#include "f_finale.h"
#include "lua_hook.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "d_net.h"
#include "mserv.h"
#include "m_misc.h"
#include "m_anigif.h"
#include "byteptr.h"
#include "st_stuff.h"
#include "i_sound.h"
#include "fastcmp.h"

#include "i_joy.h" // for joystick menu controls

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#if defined(HAVE_SDL)
#include "SDL.h"
#if SDL_VERSION_ATLEAST(2,0,0)
#include "sdl/sdlmain.h" // JOYSTICK_HOTPLUG
#endif
#endif

#if defined (__GNUC__) && (__GNUC__ >= 4)
#define FIXUPO0
#endif

#define SKULLXOFF -32
#define LINEHEIGHT 16
#define STRINGHEIGHT 8
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 8
#define SLIDER_RANGE 9
#define SLIDER_WIDTH 78
#define SERVERS_PER_PAGE 11

typedef enum
{
	QUITMSG = 0,
	QUITMSG1,
	QUITMSG2,
	QUITMSG3,
	QUITMSG4,
	QUITMSG5,
	QUITMSG6,
	QUITMSG7,

	QUIT2MSG,
	QUIT2MSG1,
	QUIT2MSG2,
	QUIT2MSG3,
	QUIT2MSG4,
	QUIT2MSG5,
	QUIT2MSG6,

	QUIT3MSG,
	QUIT3MSG1,
	QUIT3MSG2,
	QUIT3MSG3,
	QUIT3MSG4,
	QUIT3MSG5,
	QUIT3MSG6,
	NUM_QUITMESSAGES
} text_enum;

#ifdef HAVE_THREADS
I_mutex m_menu_mutex;
#endif

M_waiting_mode_t m_waiting_mode = M_NOT_WAITING;

const char *quitmsg[NUM_QUITMESSAGES];

// Stuff for customizing the player select screen Tails 09-22-2003
description_t description[MAXSKINS];
INT16 char_on = -1, startchar = 0;
static char *char_notes = NULL;

boolean menuactive = false;
boolean fromlevelselect = false;

typedef enum
{
	LLM_CREATESERVER,
	LLM_LEVELSELECT,
	LLM_RECORDATTACK,
	LLM_NIGHTSATTACK
} levellist_mode_t;

levellist_mode_t levellistmode = LLM_CREATESERVER;
UINT8 maplistoption = 0;

static char joystickInfo[MAX_JOYSTICKS+1][29];
#ifndef NONET
static UINT32 serverlistpage;
#endif

static UINT8 numsaves = 0;
static saveinfo_t* savegameinfo = NULL; // Extra info about the save games.
static patch_t *savselp[7];

INT16 startmap; // Mario, NiGHTS, or just a plain old normal game?

static INT16 itemOn = 1; // menu item skull is on, Hack by Tails 09-18-2002
static INT16 skullAnimCounter = 10; // skull animation counter

static  boolean setupcontrols_secondaryplayer;
static  INT32   (*setupcontrols)[2];  // pointer to the gamecontrols of the player being edited

// shhh... what am I doing... nooooo!
static INT32 vidm_testingmode = 0;
static INT32 vidm_previousmode;
static INT32 vidm_selected = 0;
static INT32 vidm_nummodes;
static INT32 vidm_column_size;

// new menus
static tic_t recatkdrawtimer = 0;
static tic_t ntsatkdrawtimer = 0;

static tic_t charseltimer = 0;
static fixed_t char_scroll = 0;
#define charscrollamt 128*FRACUNIT

static tic_t keydown = 0;

//
// PROTOTYPES
//

static void M_GoBack(INT32 choice);
static void M_StopMessage(INT32 choice);
static boolean stopstopmessage = false;

#ifndef NONET
static void M_HandleServerPage(INT32 choice);
static void M_RoomMenu(INT32 choice);
#endif

// Prototyping is fun, innit?
// ==========================================================================
// NEEDED FUNCTION PROTOTYPES GO HERE
// ==========================================================================

// the haxor message menu
menu_t MessageDef;

menu_t SPauseDef;

// Level Select
static levelselect_t levelselect = {0, NULL};
static UINT8 levelselectselect[3];
static patch_t *levselp[2][3];
static INT32 lsoffs[2];

#define lsrow levelselectselect[0]
#define lscol levelselectselect[1]
#define lshli levelselectselect[2]

#define lshseperation 101
#define lsbasevseperation (62*vid.height)/(BASEVIDHEIGHT*vid.dupy) //62
#define lsheadingheight 16
#define getheadingoffset(row) (levelselect.rows[row].header[0] ? lsheadingheight : 0)
#define lsvseperation(row) lsbasevseperation + getheadingoffset(row)
#define lswide(row) levelselect.rows[row].mapavailable[3]

#define lsbasex 19
#define lsbasey 59+lsheadingheight

// Sky Room
static void M_CustomLevelSelect(INT32 choice);
static void M_CustomWarp(INT32 choice);
FUNCNORETURN static ATTRNORETURN void M_UltimateCheat(INT32 choice);
static void M_LoadGameLevelSelect(INT32 choice);
static void M_AllowSuper(INT32 choice);
static void M_GetAllEmeralds(INT32 choice);
static void M_DestroyRobots(INT32 choice);
static void M_LevelSelectWarp(INT32 choice);
static void M_Credits(INT32 choice);
static void M_SoundTest(INT32 choice);
static void M_PandorasBox(INT32 choice);
static void M_EmblemHints(INT32 choice);
static void M_HandleEmblemHints(INT32 choice);
UINT32 hintpage = 1;
static void M_HandleChecklist(INT32 choice);
menu_t SR_MainDef, SR_UnlockChecklistDef;

static UINT8 check_on;

// Misc. Main Menu
static void M_SinglePlayerMenu(INT32 choice);
static void M_Options(INT32 choice);
static void M_SelectableClearMenus(INT32 choice);
static void M_Retry(INT32 choice);
static void M_EndGame(INT32 choice);
static void M_MapChange(INT32 choice);
static void M_ChangeLevel(INT32 choice);
static void M_ConfirmSpectate(INT32 choice);
static void M_ConfirmEnterGame(INT32 choice);
static void M_ConfirmTeamScramble(INT32 choice);
static void M_ConfirmTeamChange(INT32 choice);
static void M_SecretsMenu(INT32 choice);
static void M_SetupChoosePlayer(INT32 choice);
static UINT8 M_SetupChoosePlayerDirect(INT32 choice);
static void M_QuitSRB2(INT32 choice);
menu_t SP_MainDef, OP_MainDef;
menu_t MISC_ScrambleTeamDef, MISC_ChangeTeamDef;

// Single Player
static void M_StartTutorial(INT32 choice);
static void M_LoadGame(INT32 choice);
static void M_HandleTimeAttackLevelSelect(INT32 choice);
static void M_TimeAttackLevelSelect(INT32 choice);
static void M_TimeAttack(INT32 choice);
static void M_NightsAttackLevelSelect(INT32 choice);
static void M_NightsAttack(INT32 choice);
static void M_Statistics(INT32 choice);
static void M_ReplayTimeAttack(INT32 choice);
static void M_ChooseTimeAttack(INT32 choice);
static void M_ChooseNightsAttack(INT32 choice);
static void M_ModeAttackEndGame(INT32 choice);
static void M_SetGuestReplay(INT32 choice);
static void M_HandleChoosePlayerMenu(INT32 choice);
static void M_ChoosePlayer(INT32 choice);
static void M_MarathonLiveEventBackup(INT32 choice);
static void M_Marathon(INT32 choice);
static void M_HandleMarathonChoosePlayer(INT32 choice);
static void M_StartMarathon(INT32 choice);
menu_t SP_LevelStatsDef;
static menu_t SP_TimeAttackDef, SP_ReplayDef, SP_GuestReplayDef, SP_GhostDef;
static menu_t SP_NightsAttackDef, SP_NightsReplayDef, SP_NightsGuestReplayDef, SP_NightsGhostDef;
static menu_t SP_MarathonDef;

// Multiplayer
static void M_SetupMultiPlayer(INT32 choice);
static void M_SetupMultiPlayer2(INT32 choice);
static void M_StartSplitServerMenu(INT32 choice);
static void M_StartServer(INT32 choice);
static void M_ServerOptions(INT32 choice);
#ifndef NONET
static void M_StartServerMenu(INT32 choice);
static void M_ConnectMenu(INT32 choice);
static void M_ConnectMenuModChecks(INT32 choice);
static void M_Refresh(INT32 choice);
static void M_Connect(INT32 choice);
static void M_ChooseRoom(INT32 choice);
menu_t MP_MainDef;
#endif

// Options
// Split into multiple parts due to size
// Controls
menu_t OP_ChangeControlsDef;
menu_t OP_MPControlsDef, OP_MiscControlsDef;
menu_t OP_P1ControlsDef, OP_P2ControlsDef, OP_MouseOptionsDef;
menu_t OP_Mouse2OptionsDef, OP_Joystick1Def, OP_Joystick2Def;
menu_t OP_CameraOptionsDef, OP_Camera2OptionsDef;
menu_t OP_PlaystyleDef;
static void M_VideoModeMenu(INT32 choice);
static void M_Setup1PControlsMenu(INT32 choice);
static void M_Setup2PControlsMenu(INT32 choice);
static void M_Setup1PJoystickMenu(INT32 choice);
static void M_Setup2PJoystickMenu(INT32 choice);
static void M_Setup1PPlaystyleMenu(INT32 choice);
static void M_Setup2PPlaystyleMenu(INT32 choice);
static void M_AssignJoystick(INT32 choice);
static void M_ChangeControl(INT32 choice);

// Video & Sound
static void M_VideoOptions(INT32 choice);
menu_t OP_VideoOptionsDef, OP_VideoModeDef, OP_ColorOptionsDef;
#ifdef HWRENDER
static void M_OpenGLOptionsMenu(void);
menu_t OP_OpenGLOptionsDef;
#ifdef ALAM_LIGHTING
menu_t OP_OpenGLLightingDef;
#endif // ALAM_LIGHTING
#endif // HWRENDER
menu_t OP_SoundOptionsDef;
menu_t OP_SoundAdvancedDef;

//Misc
menu_t OP_DataOptionsDef, OP_ScreenshotOptionsDef, OP_EraseDataDef;
menu_t OP_ServerOptionsDef;
menu_t OP_MonitorToggleDef;
static void M_ScreenshotOptions(INT32 choice);
static void M_SetupScreenshotMenu(void);
static void M_EraseData(INT32 choice);

static void M_Addons(INT32 choice);
static void M_AddonsOptions(INT32 choice);
static patch_t *addonsp[NUM_EXT+5];

#define addonmenusize 9 // number of items actually displayed in the addons menu view, formerly (2*numaddonsshown + 1)
#define numaddonsshown 4 // number of items to each side of the currently selected item, unless at top/bottom ends of directory

static void M_DrawLevelPlatterHeader(INT32 y, const char *header, boolean headerhighlight, boolean allowlowercase);

// Drawing functions
static void M_DrawGenericMenu(void);
static void M_DrawGenericScrollMenu(void);
static void M_DrawCenteredMenu(void);
static void M_DrawAddons(void);
static void M_DrawChecklist(void);
static void M_DrawSoundTest(void);
static void M_DrawEmblemHints(void);
static void M_DrawPauseMenu(void);
static void M_DrawServerMenu(void);
static void M_DrawLevelPlatterMenu(void);
static void M_DrawImageDef(void);
static void M_DrawLoad(void);
static void M_DrawLevelStats(void);
static void M_DrawTimeAttackMenu(void);
static void M_DrawNightsAttackMenu(void);
static void M_DrawMarathon(void);
static void M_DrawSetupChoosePlayerMenu(void);
static void M_DrawControlsDefMenu(void);
static void M_DrawCameraOptionsMenu(void);
static void M_DrawPlaystyleMenu(void);
static void M_DrawControl(void);
static void M_DrawMainVideoMenu(void);
static void M_DrawVideoMode(void);
static void M_DrawColorMenu(void);
static void M_DrawScreenshotMenu(void);
static void M_DrawMonitorToggles(void);
#ifndef NONET
static void M_DrawConnectMenu(void);
static void M_DrawMPMainMenu(void);
static void M_DrawRoomMenu(void);
#endif
static void M_DrawJoystick(void);
static void M_DrawSetupMultiPlayerMenu(void);

// Handling functions
static boolean M_ExitPandorasBox(void);
static boolean M_QuitMultiPlayerMenu(void);
static void M_HandleAddons(INT32 choice);
static void M_HandleLevelPlatter(INT32 choice);
static void M_HandleSoundTest(INT32 choice);
static void M_HandleImageDef(INT32 choice);
static void M_HandleLoadSave(INT32 choice);
static void M_HandleLevelStats(INT32 choice);
static void M_HandlePlaystyleMenu(INT32 choice);
#ifndef NONET
static boolean M_CancelConnect(void);
static void M_HandleConnectIP(INT32 choice);
#endif
static void M_HandleSetupMultiPlayer(INT32 choice);
static void M_HandleVideoMode(INT32 choice);

static void M_ResetCvars(void);

// Consvar onchange functions
static void Newgametype_OnChange(void);
static void Dummymares_OnChange(void);

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

consvar_t cv_showfocuslost = CVAR_INIT ("showfocuslost", "Yes", CV_SAVE, CV_YesNo, NULL);

static CV_PossibleValue_t map_cons_t[] = {
	{1,"MIN"},
	{NUMMAPS, "MAX"},
	{0,NULL}
};
consvar_t cv_nextmap = CVAR_INIT ("nextmap", "1", CV_HIDEN|CV_CALL, map_cons_t, Nextmap_OnChange);

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = CVAR_INIT ("chooseskin", DEFAULTSKIN, CV_HIDEN|CV_CALL, skins_cons_t, Nextmap_OnChange);

// This gametype list is integral for many different reasons.
// When you add gametypes here, don't forget to update them in deh_tables.c and doomstat.h!
CV_PossibleValue_t gametype_cons_t[NUMGAMETYPES+1];

consvar_t cv_newgametype = CVAR_INIT ("newgametype", "Co-op", CV_HIDEN|CV_CALL, gametype_cons_t, Newgametype_OnChange);

static CV_PossibleValue_t serversort_cons_t[] = {
	{0,"Ping"},
	{1,"Modified State"},
	{2,"Most Players"},
	{3,"Least Players"},
	{4,"Max Player Slots"},
	{5,"Gametype"},
	{0,NULL}
};
consvar_t cv_serversort = CVAR_INIT ("serversort", "Ping", CV_HIDEN | CV_CALL, serversort_cons_t, M_SortServerList);

// first time memory
consvar_t cv_tutorialprompt = CVAR_INIT ("tutorialprompt", "On", CV_SAVE, CV_OnOff, NULL);

// autorecord demos for time attack
static consvar_t cv_autorecord = CVAR_INIT ("autorecord", "Yes", 0, CV_YesNo, NULL);

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_bestscore = CVAR_INIT ("ghost_bestscore", "Show", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_besttime  = CVAR_INIT ("ghost_besttime",  "Show", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_bestrings = CVAR_INIT ("ghost_bestrings", "Show", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_last      = CVAR_INIT ("ghost_last",      "Show", CV_SAVE, ghost_cons_t, NULL);
consvar_t cv_ghost_guest     = CVAR_INIT ("ghost_guest",     "Show", CV_SAVE, ghost2_cons_t, NULL);

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.
static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static CV_PossibleValue_t ringlimit_cons_t[] = {{0, "MIN"}, {9999, "MAX"}, {0, NULL}};
static CV_PossibleValue_t liveslimit_cons_t[] = {{1, "MIN"}, {99, "MAX"}, {-1, "Infinite"}, {0, NULL}};
static CV_PossibleValue_t contlimit_cons_t[] = {{0, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t dummymares_cons_t[] = {
	{-1, "END"}, {0,"Overall"}, {1,"Mare 1"}, {2,"Mare 2"}, {3,"Mare 3"}, {4,"Mare 4"}, {5,"Mare 5"}, {6,"Mare 6"}, {7,"Mare 7"}, {8,"Mare 8"}, {0,NULL}
};

static consvar_t cv_dummyteam = CVAR_INIT ("dummyteam", "Spectator", CV_HIDEN, dummyteam_cons_t, NULL);
static consvar_t cv_dummyscramble = CVAR_INIT ("dummyscramble", "Random", CV_HIDEN, dummyscramble_cons_t, NULL);
static consvar_t cv_dummyrings = CVAR_INIT ("dummyrings", "0", CV_HIDEN, ringlimit_cons_t,	NULL);
static consvar_t cv_dummylives = CVAR_INIT ("dummylives", "0", CV_HIDEN, liveslimit_cons_t, NULL);
static consvar_t cv_dummycontinues = CVAR_INIT ("dummycontinues", "0", CV_HIDEN, contlimit_cons_t, NULL);
static consvar_t cv_dummymares = CVAR_INIT ("dummymares", "Overall", CV_HIDEN|CV_CALL, dummymares_cons_t, Dummymares_OnChange);

CV_PossibleValue_t marathon_cons_t[] = {{0, "Standard"}, {1, "Live Event Backup"}, {2, "Ultimate"}, {0, NULL}};
CV_PossibleValue_t loadless_cons_t[] = {{0, "Realtime"}, {1, "In-game"}, {0, NULL}};

consvar_t cv_dummymarathon = CVAR_INIT ("dummymarathon", "Standard", CV_HIDEN, marathon_cons_t, NULL);
consvar_t cv_dummycutscenes = CVAR_INIT ("dummycutscenes", "Off", CV_HIDEN, CV_OnOff, NULL);
consvar_t cv_dummyloadless = CVAR_INIT ("dummyloadless", "In-game", CV_HIDEN, loadless_cons_t, NULL);

// ==========================================================================
// ORGANIZATION START.
// ==========================================================================
// Note: Never should we be jumping from one category of menu options to another
//       without first going to the Main Menu.
// Note: Ignore the above if you're working with the Pause menu.
// Note: (Prefix)_MainMenu should be the target of all Main Menu options that
//       point to submenus.

// ---------
// Main Menu
// ---------
static menuitem_t MainMenu[] =
{
	{IT_STRING|IT_CALL,    NULL, "1  Player",   M_SinglePlayerMenu,      76},
#ifndef NONET
	{IT_STRING|IT_SUBMENU, NULL, "Multiplayer", &MP_MainDef,             84},
#else
	{IT_STRING|IT_CALL,    NULL, "Multiplayer", M_StartSplitServerMenu,  84},
#endif
	{IT_STRING|IT_CALL,    NULL, "Extras",      M_SecretsMenu,           92},
	{IT_CALL   |IT_STRING, NULL, "Addons",      M_Addons,               100},
	{IT_STRING|IT_CALL,    NULL, "Options",     M_Options,              108},
	{IT_STRING|IT_CALL,    NULL, "Quit  Game",  M_QuitSRB2,             116},
};

typedef enum
{
	singleplr = 0,
	multiplr,
	secrets,
	addons,
	options,
	quitdoom
} main_e;

static menuitem_t MISC_AddonsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleAddons, 0},     // dummy menuitem for the control func
};

// ---------------------------------
// Pause Menu Mode Attacking Edition
// ---------------------------------
static menuitem_t MAPauseMenu[] =
{
	{IT_CALL | IT_STRING,    NULL, "Emblem Hints...",      M_EmblemHints,         32},

	{IT_CALL | IT_STRING,    NULL, "Continue",             M_SelectableClearMenus,48},
	{IT_CALL | IT_STRING,    NULL, "Retry",                M_ModeAttackRetry,     56},
	{IT_CALL | IT_STRING,    NULL, "Abort",                M_ModeAttackEndGame,   64},
};

typedef enum
{
	mapause_hints,
	mapause_continue,
	mapause_retry,
	mapause_abort
} mapause_e;

// ---------------------
// Pause Menu MP Edition
// ---------------------
static menuitem_t MPauseMenu[] =
{
	{IT_STRING | IT_CALL,    NULL, "Add-ons...",                M_Addons,               8},
	{IT_STRING | IT_SUBMENU, NULL, "Scramble Teams...",         &MISC_ScrambleTeamDef, 16},
	{IT_STRING | IT_CALL,    NULL, "Switch Gametype/Level...",  M_MapChange,           24},

	{IT_STRING | IT_CALL,    NULL, "Continue",                  M_SelectableClearMenus,40},
	{IT_STRING | IT_CALL,    NULL, "Player 1 Setup",            M_SetupMultiPlayer,    48}, // splitscreen
	{IT_STRING | IT_CALL,    NULL, "Player 2 Setup",            M_SetupMultiPlayer2,   56}, // splitscreen

	{IT_STRING | IT_CALL,    NULL, "Spectate",                  M_ConfirmSpectate,     48},
	{IT_STRING | IT_CALL,    NULL, "Enter Game",                M_ConfirmEnterGame,    48},
	{IT_STRING | IT_SUBMENU, NULL, "Switch Team...",            &MISC_ChangeTeamDef,   48},
	{IT_STRING | IT_CALL,    NULL, "Player Setup",              M_SetupMultiPlayer,    56}, // alone
	{IT_STRING | IT_CALL,    NULL, "Options",                   M_Options,             64},

	{IT_STRING | IT_CALL,    NULL, "Return to Title",           M_EndGame,             80},
	{IT_STRING | IT_CALL,    NULL, "Quit Game",                 M_QuitSRB2,            88},
};

typedef enum
{
	mpause_addons = 0,
	mpause_scramble,
	mpause_switchmap,

	mpause_continue,
	mpause_psetupsplit,
	mpause_psetupsplit2,
	mpause_spectate,
	mpause_entergame,
	mpause_switchteam,
	mpause_psetup,
	mpause_options,

	mpause_title,
	mpause_quit
} mpause_e;

// ---------------------
// Pause Menu SP Edition
// ---------------------
static menuitem_t SPauseMenu[] =
{
	// Pandora's Box will be shifted up if both options are available
	{IT_CALL | IT_STRING,    NULL, "Pandora's Box...",     M_PandorasBox,         16},
	{IT_CALL | IT_STRING,    NULL, "Emblem Hints...",      M_EmblemHints,         24},
	{IT_CALL | IT_STRING,    NULL, "Level Select...",      M_LoadGameLevelSelect, 32},

	{IT_CALL | IT_STRING,    NULL, "Continue",             M_SelectableClearMenus,48},
	{IT_CALL | IT_STRING,    NULL, "Retry",                M_Retry,               56},
	{IT_CALL | IT_STRING,    NULL, "Options",              M_Options,             64},

	{IT_CALL | IT_STRING,    NULL, "Return to Title",      M_EndGame,             80},
	{IT_CALL | IT_STRING,    NULL, "Quit Game",            M_QuitSRB2,            88},
};

typedef enum
{
	spause_pandora = 0,
	spause_hints,
	spause_levelselect,

	spause_continue,
	spause_retry,
	spause_options,

	spause_title,
	spause_quit
} spause_e;

// -----------------
// Misc menu options
// -----------------
// Prefix: MISC_
static menuitem_t MISC_ScrambleTeamMenu[] =
{
	{IT_STRING|IT_CVAR,      NULL, "Scramble Method", &cv_dummyscramble,     30},
	{IT_WHITESTRING|IT_CALL, NULL, "Confirm",         M_ConfirmTeamScramble, 90},
};

static menuitem_t MISC_ChangeTeamMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Select Team",             &cv_dummyteam,    30},
	{IT_WHITESTRING|IT_CALL,         NULL, "Confirm",           M_ConfirmTeamChange,    90},
};

gtdesc_t gametypedesc[NUMGAMETYPES] =
{
	{{ 54,  54}, "Play through the single-player campaign with your friends, teaming up to beat Dr Eggman's nefarious challenges!"},
	{{103, 103}, "Speed your way through the main acts, competing in several different categories to see who's the best."},
	{{190, 190}, "There's not much to it - zoom through the level faster than everyone else."},
	{{ 66,  66}, "Sling rings at your foes in a free-for-all battle. Use the special weapon rings to your advantage!"},
	{{153,  37}, "Sling rings at your foes in a color-coded battle. Use the special weapon rings to your advantage!"},
	{{123, 123}, "Whoever's IT has to hunt down everyone else. If you get caught, you have to turn on your former friends!"},
	{{150, 150}, "Try and find a good hiding place in these maps - we dare you."},
	{{ 37, 153}, "Steal the flag from the enemy's base and bring it back to your own, but watch out - they could just as easily steal yours!"},
};

static menuitem_t MISC_ChangeLevelMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelPlatter, 0},     // dummy menuitem for the control func
};

static menuitem_t MISC_HelpMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPN01", M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPN02", M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPN03", M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPM01", M_HandleImageDef, 0},
	{IT_KEYHANDLER | IT_NOTHING, NULL, "HELPM02", M_HandleImageDef, 0},
};

// --------------------------------
// Sky Room and all of its submenus
// --------------------------------
// Prefix: SR_

// Pause Menu Pandora's Box Options
static menuitem_t SR_PandorasBox[] =
{
	{IT_STRING | IT_CALL, NULL, "Mid-game add-ons...", M_Addons,             0},

	{IT_STRING | IT_CVAR, NULL, "Rings",               &cv_dummyrings,      20},
	{IT_STRING | IT_CVAR, NULL, "Lives",               &cv_dummylives,      30},
	{IT_STRING | IT_CVAR, NULL, "Continues",           &cv_dummycontinues,  40},

	{IT_STRING | IT_CVAR, NULL, "Gravity",             &cv_gravity,         60},
	{IT_STRING | IT_CVAR, NULL, "Throw Rings",         &cv_ringslinger,     70},

	{IT_STRING | IT_CALL, NULL, "Enable Super form",   M_AllowSuper,        90},
	{IT_STRING | IT_CALL, NULL, "Get All Emeralds",    M_GetAllEmeralds,   100},
	{IT_STRING | IT_CALL, NULL, "Destroy All Robots",  M_DestroyRobots,    110},

	{IT_STRING | IT_CALL, NULL, "Ultimate Cheat",      M_UltimateCheat,    130},
};

// Sky Room Custom Unlocks
static menuitem_t SR_MainMenu[] =
{
	{IT_STRING|IT_SUBMENU,NULL, "Extras Checklist", &SR_UnlockChecklistDef, 0},
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom1
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom2
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom3
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom4
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom5
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom6
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom7
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom8
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom9
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom10
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom11
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom12
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom13
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom14
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom15
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom16
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom17
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom18
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom19
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom20
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom21
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom22
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom23
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom24
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom25
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom26
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom27
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom28
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom29
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom30
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom31
	{IT_DISABLED,         NULL, "",   NULL,                 0}, // Custom32

};

static menuitem_t SR_LevelSelectMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelPlatter, 0},     // dummy menuitem for the control func
};

static menuitem_t SR_UnlockChecklistMenu[] =
{
	{IT_KEYHANDLER | IT_STRING, NULL, "", M_HandleChecklist, 0},
};

static menuitem_t SR_SoundTestMenu[] =
{
	{IT_KEYHANDLER | IT_STRING, NULL, "", M_HandleSoundTest, 0},
};

static menuitem_t SR_EmblemHintMenu[] =
{
	{IT_STRING | IT_ARROWS,  NULL, "Page",    M_HandleEmblemHints, 10},
	{IT_STRING|IT_CVAR,      NULL, "Emblem Radar", &cv_itemfinder, 20},
	{IT_WHITESTRING|IT_CALL, NULL, "Back",         M_GoBack,       30}
};

// --------------------------------
// 1 Player and all of its submenus
// --------------------------------
// Prefix: SP_

// Single Player Main
static menuitem_t SP_MainMenu[] =
{
	// Note: If changing the positions here, also change them in M_SinglePlayerMenu()
	{IT_CALL | IT_STRING,                       NULL, "Start Game",    M_LoadGame,                 76},
	{IT_SECRET,                                 NULL, "Record Attack", M_TimeAttack,               84},
	{IT_SECRET,                                 NULL, "NiGHTS Mode",   M_NightsAttack,             92},
	{IT_SECRET,                                 NULL, "Marathon Run",  M_Marathon,                100},
	{IT_CALL | IT_STRING,                       NULL, "Tutorial",      M_StartTutorial,           108},
	{IT_CALL | IT_STRING | IT_CALL_NOTMODIFIED, NULL, "Statistics",    M_Statistics,              116}
};

enum
{
	spstartgame,
	sprecordattack,
	spnightsmode,
	spmarathon,
	sptutorial,
	spstatistics
};

// Single Player Load Game
static menuitem_t SP_LoadGameMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLoadSave, 0},     // dummy menuitem for the control func
};

// Single Player Level Select
static menuitem_t SP_LevelSelectMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelPlatter, 0},     // dummy menuitem for the control func
};

// Single Player Time Attack Level Select
static menuitem_t SP_TimeAttackLevelSelectMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelPlatter, 0},     // dummy menuitem for the control func
};

// Single Player Time Attack
static menuitem_t SP_TimeAttackMenu[] =
{
	{IT_STRING|IT_KEYHANDLER,  NULL, "Level Select...", M_HandleTimeAttackLevelSelect,   62},
	{IT_STRING|IT_CVAR,        NULL, "Character",       &cv_chooseskin,             72},

	{IT_DISABLED,              NULL, "Guest Option...", &SP_GuestReplayDef, 100},
	{IT_DISABLED,              NULL, "Replay...",       &SP_ReplayDef,      110},
	{IT_DISABLED,              NULL, "Ghosts...",       &SP_GhostDef,       120},
	{IT_WHITESTRING|IT_CALL|IT_CALL_NOTMODIFIED,   NULL, "Start",         M_ChooseTimeAttack,   130},
};

enum
{
	talevel,
	taplayer,

	taguest,
	tareplay,
	taghost,
	tastart
};

static menuitem_t SP_ReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Score", M_ReplayTimeAttack, 0},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Time",  M_ReplayTimeAttack, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Rings", M_ReplayTimeAttack,16},

	{IT_WHITESTRING|IT_CALL, NULL, "Replay Last",       M_ReplayTimeAttack,29},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Guest",      M_ReplayTimeAttack,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",           &SP_TimeAttackDef, 50}
};

static menuitem_t SP_NightsReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Score", M_ReplayTimeAttack, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Best Time",  M_ReplayTimeAttack,16},

	{IT_WHITESTRING|IT_CALL, NULL, "Replay Last",       M_ReplayTimeAttack,29},
	{IT_WHITESTRING|IT_CALL, NULL, "Replay Guest",      M_ReplayTimeAttack,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",           &SP_NightsAttackDef, 50}
};

static menuitem_t SP_GuestReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Score as Guest", M_SetGuestReplay, 0},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Time as Guest",  M_SetGuestReplay, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Rings as Guest", M_SetGuestReplay,16},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Last as Guest",       M_SetGuestReplay,24},

	{IT_WHITESTRING|IT_CALL, NULL, "Delete Guest Replay",      M_SetGuestReplay,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",                &SP_TimeAttackDef, 50}
};

static menuitem_t SP_NightsGuestReplayMenu[] =
{
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Score as Guest", M_SetGuestReplay, 8},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Best Time as Guest",  M_SetGuestReplay,16},
	{IT_WHITESTRING|IT_CALL, NULL, "Save Last as Guest",       M_SetGuestReplay,24},

	{IT_WHITESTRING|IT_CALL, NULL, "Delete Guest Replay",      M_SetGuestReplay,37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",                &SP_NightsAttackDef, 50}
};

static menuitem_t SP_GhostMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Best Score", &cv_ghost_bestscore, 0},
	{IT_STRING|IT_CVAR,         NULL, "Best Time",  &cv_ghost_besttime,  8},
	{IT_STRING|IT_CVAR,         NULL, "Best Rings", &cv_ghost_bestrings,16},
	{IT_STRING|IT_CVAR,         NULL, "Last",       &cv_ghost_last,     24},

	{IT_STRING|IT_CVAR,         NULL, "Guest",      &cv_ghost_guest,    37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",       &SP_TimeAttackDef,  50}
};

static menuitem_t SP_NightsGhostMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Best Score", &cv_ghost_bestscore, 8},
	{IT_STRING|IT_CVAR,         NULL, "Best Time",  &cv_ghost_besttime, 16},
	{IT_STRING|IT_CVAR,         NULL, "Last",       &cv_ghost_last,     24},

	{IT_STRING|IT_CVAR,         NULL, "Guest",      &cv_ghost_guest,    37},

	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",       &SP_NightsAttackDef,  50}
};

// Single Player Nights Attack Level Select
static menuitem_t SP_NightsAttackLevelSelectMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelPlatter, 0},     // dummy menuitem for the control func
};

// Single Player Nights Attack
static menuitem_t SP_NightsAttackMenu[] =
{
	{IT_STRING|IT_KEYHANDLER,        NULL, "Level Select...",  &M_HandleTimeAttackLevelSelect,  52},
	{IT_STRING|IT_CVAR,        NULL, "Character",       &cv_chooseskin,             62},
	{IT_STRING|IT_CVAR,        NULL, "Show Records For", &cv_dummymares,              72},

	{IT_DISABLED,              NULL, "Guest Option...",  &SP_NightsGuestReplayDef,    100},
	{IT_DISABLED,              NULL, "Replay...",        &SP_NightsReplayDef,         110},
	{IT_DISABLED,              NULL, "Ghosts...",        &SP_NightsGhostDef,          120},
	{IT_WHITESTRING|IT_CALL|IT_CALL_NOTMODIFIED, NULL, "Start", M_ChooseNightsAttack, 130},
};

enum
{
	nalevel,
	nachar,
	narecords,

	naguest,
	nareplay,
	naghost,
	nastart
};

// Marathon
static menuitem_t SP_MarathonMenu[] =
{
	{IT_STRING|IT_KEYHANDLER,  NULL, "Character", M_HandleMarathonChoosePlayer,  90},
	{IT_STRING|IT_CVAR,        NULL, "Category",  &cv_dummymarathon,            100},
	{IT_STRING|IT_CVAR,        NULL, "Timer",     &cv_dummyloadless,            110},
	{IT_STRING|IT_CVAR,        NULL, "Cutscenes", &cv_dummycutscenes,           120},
	{IT_WHITESTRING|IT_CALL,   NULL, "Start",     M_StartMarathon,              130},
};

enum
{
	marathonplayer,
	marathonultimate,
	marathonloadless,
	marathoncutscenes,
	marathonstart
};

// Statistics
static menuitem_t SP_LevelStatsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelStats, 0},     // dummy menuitem for the control func
};

// Player menu dummy
static menuitem_t SP_PlayerMenu[] =
{
	{IT_NOTHING | IT_KEYHANDLER, NULL, "", M_HandleChoosePlayerMenu, 0},     // dummy menuitem for the control func
};

// -----------------------------------
// Multiplayer and all of its submenus
// -----------------------------------
// Prefix: MP_

// Separated splitscreen and normal servers.
static menuitem_t MP_SplitServerMenu[] =
{
	{IT_STRING|IT_CALL,              NULL, "Select Gametype/Level...", M_MapChange,         100},
#ifdef NONET // In order to keep player setup accessible.
	{IT_STRING|IT_CALL,              NULL, "Player 1 setup...",        M_SetupMultiPlayer,  110},
	{IT_STRING|IT_CALL,              NULL, "Player 2 setup...",        M_SetupMultiPlayer2, 120},
#endif
	{IT_STRING|IT_CALL,              NULL, "More Options...",          M_ServerOptions,     130},
	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                    M_StartServer,       140},
};

#ifndef NONET

static menuitem_t MP_MainMenu[] =
{
	{IT_HEADER, NULL, "Join a game", NULL, 0},
	{IT_STRING|IT_CALL,       NULL, "Server browser...",     M_ConnectMenuModChecks,          12},
	{IT_STRING|IT_KEYHANDLER, NULL, "Specify IPv4 address:", M_HandleConnectIP,      22},
	{IT_HEADER, NULL, "Host a game", NULL, 54},
	{IT_STRING|IT_CALL,       NULL, "Internet/LAN...",       M_StartServerMenu,      66},
	{IT_STRING|IT_CALL,       NULL, "Splitscreen...",        M_StartSplitServerMenu, 76},
	{IT_HEADER, NULL, "Player setup", NULL, 94},
	{IT_STRING|IT_CALL,       NULL, "Player 1...",           M_SetupMultiPlayer,    106},
	{IT_STRING|IT_CALL,       NULL, "Player 2... ",          M_SetupMultiPlayer2,   116},
};

static menuitem_t MP_ServerMenu[] =
{
	{IT_STRING|IT_CALL,              NULL, "Room...",                  M_RoomMenu,          10},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Server Name",              &cv_servername,      20},
	{IT_STRING|IT_CVAR,              NULL, "Max Players",              &cv_maxplayers,      46},
	{IT_STRING|IT_CVAR,              NULL, "Allow Add-on Downloading", &cv_downloading,     56},
	{IT_STRING|IT_CALL,              NULL, "Select Gametype/Level...", M_MapChange,        100},
	{IT_STRING|IT_CALL,              NULL, "More Options...",          M_ServerOptions,    130},
	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                    M_StartServer,      140},
};

enum
{
	mp_server_room = 0,
	mp_server_name,
	mp_server_maxpl,
	mp_server_waddl,
	mp_server_levelgt,
	mp_server_options,
	mp_server_start
};

static menuitem_t MP_ConnectMenu[] =
{
	{IT_STRING | IT_CALL,       NULL, "Room...",  M_RoomMenu,         4},
	{IT_STRING | IT_CVAR,       NULL, "Sort By",  &cv_serversort,     12},
	{IT_STRING | IT_KEYHANDLER, NULL, "Page",     M_HandleServerPage, 20},
	{IT_STRING | IT_CALL,       NULL, "Refresh",  M_Refresh,          28},

	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,          48-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,          60-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,          72-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,          84-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,          96-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,         108-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,         120-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,         132-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,         144-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,         156-4},
	{IT_STRING | IT_SPACE, NULL, "",              M_Connect,         168-4},
};

enum
{
	mp_connect_room,
	mp_connect_sort,
	mp_connect_page,
	mp_connect_refresh,
	FIRSTSERVERLINE
};

menuitem_t MP_RoomMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "<Unlisted Mode>", M_ChooseRoom,   9},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  18},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  27},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  36},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  45},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  54},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  63},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  72},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  81},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  90},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom,  99},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 108},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 117},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 126},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 135},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 144},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 153},
	{IT_DISABLED,         NULL, "",               M_ChooseRoom, 162},
};

#endif

static menuitem_t MP_PlayerSetupMenu[] =
{
	{IT_KEYHANDLER, NULL, "", M_HandleSetupMultiPlayer, 0}, // name
	{IT_KEYHANDLER, NULL, "", M_HandleSetupMultiPlayer, 0}, // skin
	{IT_KEYHANDLER, NULL, "", M_HandleSetupMultiPlayer, 0}, // colour
	{IT_KEYHANDLER, NULL, "", M_HandleSetupMultiPlayer, 0}, // default
};

// ------------------------------------
// Options and most (?) of its submenus
// ------------------------------------
// Prefix: OP_
static menuitem_t OP_MainMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Player 1 Controls...", &OP_P1ControlsDef,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Player 2 Controls...", &OP_P2ControlsDef,   20},
	{IT_CVAR    | IT_STRING, NULL, "Controls per key",     &cv_controlperkey,   30},

	{IT_CALL    | IT_STRING, NULL, "Video Options...",     M_VideoOptions,      50},
	{IT_SUBMENU | IT_STRING, NULL, "Sound Options...",     &OP_SoundOptionsDef, 60},

	{IT_CALL    | IT_STRING, NULL, "Server Options...",    M_ServerOptions,     80},

	{IT_SUBMENU | IT_STRING, NULL, "Data Options...",      &OP_DataOptionsDef, 100},
};

static menuitem_t OP_P1ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup1PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Mouse Options...", &OP_MouseOptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Gamepad Options...", &OP_Joystick1Def  ,  30},

	{IT_SUBMENU | IT_STRING, NULL, "Camera Options...", &OP_CameraOptionsDef,	50},

	{IT_STRING  | IT_CVAR, NULL, "Automatic braking", &cv_autobrake,  70},
	{IT_CALL    | IT_STRING, NULL, "Play Style...", M_Setup1PPlaystyleMenu, 80},
};

static menuitem_t OP_P2ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup2PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Second Mouse Options...", &OP_Mouse2OptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Second Gamepad Options...", &OP_Joystick2Def  ,  30},

	{IT_SUBMENU | IT_STRING, NULL, "Camera Options...", &OP_Camera2OptionsDef,	50},

	{IT_STRING  | IT_CVAR, NULL, "Automatic braking", &cv_autobrake2,  70},
	{IT_CALL    | IT_STRING, NULL, "Play Style...", M_Setup2PPlaystyleMenu, 80},
};

static menuitem_t OP_ChangeControlsMenu[] =
{
	{IT_HEADER, NULL, "Movement", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Move Forward",     M_ChangeControl, gc_forward     },
	{IT_CALL | IT_STRING2, NULL, "Move Backward",    M_ChangeControl, gc_backward    },
	{IT_CALL | IT_STRING2, NULL, "Move Left",        M_ChangeControl, gc_strafeleft  },
	{IT_CALL | IT_STRING2, NULL, "Move Right",       M_ChangeControl, gc_straferight },
	{IT_CALL | IT_STRING2, NULL, "Jump",             M_ChangeControl, gc_jump      },
	{IT_CALL | IT_STRING2, NULL, "Spin",             M_ChangeControl, gc_spin     },
	{IT_HEADER, NULL, "Camera", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Look Up",        M_ChangeControl, gc_lookup      },
	{IT_CALL | IT_STRING2, NULL, "Look Down",      M_ChangeControl, gc_lookdown    },
	{IT_CALL | IT_STRING2, NULL, "Look Left",      M_ChangeControl, gc_turnleft    },
	{IT_CALL | IT_STRING2, NULL, "Look Right",     M_ChangeControl, gc_turnright   },
	{IT_CALL | IT_STRING2, NULL, "Center View",      M_ChangeControl, gc_centerview  },
	{IT_CALL | IT_STRING2, NULL, "Toggle Mouselook", M_ChangeControl, gc_mouseaiming },
	{IT_CALL | IT_STRING2, NULL, "Toggle Third-Person", M_ChangeControl, gc_camtoggle},
	{IT_CALL | IT_STRING2, NULL, "Reset Camera",     M_ChangeControl, gc_camreset    },
	{IT_HEADER, NULL, "Meta", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Game Status",
    M_ChangeControl, gc_scores      },
	{IT_CALL | IT_STRING2, NULL, "Pause / Run Retry", M_ChangeControl, gc_pause      },
	{IT_CALL | IT_STRING2, NULL, "Screenshot",            M_ChangeControl, gc_screenshot },
	{IT_CALL | IT_STRING2, NULL, "Toggle GIF Recording",  M_ChangeControl, gc_recordgif  },
	{IT_CALL | IT_STRING2, NULL, "Open/Close Menu (ESC)", M_ChangeControl, gc_systemmenu },
	{IT_CALL | IT_STRING2, NULL, "Change Viewpoint",      M_ChangeControl, gc_viewpoint  },
	{IT_CALL | IT_STRING2, NULL, "Console",          M_ChangeControl, gc_console     },
	{IT_HEADER, NULL, "Multiplayer", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Talk",             M_ChangeControl, gc_talkkey     },
	{IT_CALL | IT_STRING2, NULL, "Talk (Team only)", M_ChangeControl, gc_teamkey     },
	{IT_HEADER, NULL, "Ringslinger (Match, CTF, Tag, H&S)", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Fire",             M_ChangeControl, gc_fire        },
	{IT_CALL | IT_STRING2, NULL, "Fire Normal",      M_ChangeControl, gc_firenormal  },
	{IT_CALL | IT_STRING2, NULL, "Toss Flag",        M_ChangeControl, gc_tossflag    },
	{IT_CALL | IT_STRING2, NULL, "Next Weapon",      M_ChangeControl, gc_weaponnext  },
	{IT_CALL | IT_STRING2, NULL, "Prev Weapon",      M_ChangeControl, gc_weaponprev  },
	{IT_CALL | IT_STRING2, NULL, "Normal / Infinity",   M_ChangeControl, gc_wepslot1    },
	{IT_CALL | IT_STRING2, NULL, "Automatic",        M_ChangeControl, gc_wepslot2    },
	{IT_CALL | IT_STRING2, NULL, "Bounce",           M_ChangeControl, gc_wepslot3    },
	{IT_CALL | IT_STRING2, NULL, "Scatter",          M_ChangeControl, gc_wepslot4    },
	{IT_CALL | IT_STRING2, NULL, "Grenade",          M_ChangeControl, gc_wepslot5    },
	{IT_CALL | IT_STRING2, NULL, "Explosion",        M_ChangeControl, gc_wepslot6    },
	{IT_CALL | IT_STRING2, NULL, "Rail",             M_ChangeControl, gc_wepslot7    },
	{IT_HEADER, NULL, "Add-ons", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Custom Action 1",  M_ChangeControl, gc_custom1     },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 2",  M_ChangeControl, gc_custom2     },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 3",  M_ChangeControl, gc_custom3     },
};

static menuitem_t OP_Joystick1Menu[] =
{
	{IT_STRING | IT_CALL,  NULL, "Select Gamepad...", M_Setup1PJoystickMenu, 10},
	{IT_STRING | IT_CVAR,  NULL, "Move \x17 Axis"    , &cv_moveaxis         , 30},
	{IT_STRING | IT_CVAR,  NULL, "Move \x18 Axis"    , &cv_sideaxis         , 40},
	{IT_STRING | IT_CVAR,  NULL, "Camera \x17 Axis"  , &cv_lookaxis         , 50},
	{IT_STRING | IT_CVAR,  NULL, "Camera \x18 Axis"  , &cv_turnaxis         , 60},
	{IT_STRING | IT_CVAR,  NULL, "Jump Axis"         , &cv_jumpaxis         , 70},
	{IT_STRING | IT_CVAR,  NULL, "Spin Axis"         , &cv_spinaxis         , 80},
	{IT_STRING | IT_CVAR,  NULL, "Fire Axis"         , &cv_fireaxis         , 90},
	{IT_STRING | IT_CVAR,  NULL, "Fire Normal Axis"  , &cv_firenaxis        ,100},

	{IT_STRING | IT_CVAR, NULL, "First-Person Vert-Look", &cv_alwaysfreelook, 120},
	{IT_STRING | IT_CVAR, NULL, "Third-Person Vert-Look", &cv_chasefreelook,  130},
	{IT_STRING | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Analog Deadzone", &cv_deadzone, 140},
	{IT_STRING | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Digital Deadzone", &cv_digitaldeadzone, 150},
};

static menuitem_t OP_Joystick2Menu[] =
{
	{IT_STRING | IT_CALL,  NULL, "Select Gamepad...", M_Setup2PJoystickMenu, 10},
	{IT_STRING | IT_CVAR,  NULL, "Move \x17 Axis"    , &cv_moveaxis2        , 30},
	{IT_STRING | IT_CVAR,  NULL, "Move \x18 Axis"    , &cv_sideaxis2        , 40},
	{IT_STRING | IT_CVAR,  NULL, "Camera \x17 Axis"  , &cv_lookaxis2        , 50},
	{IT_STRING | IT_CVAR,  NULL, "Camera \x18 Axis"  , &cv_turnaxis2        , 60},
	{IT_STRING | IT_CVAR,  NULL, "Jump Axis"         , &cv_jumpaxis2        , 70},
	{IT_STRING | IT_CVAR,  NULL, "Spin Axis"         , &cv_spinaxis2        , 80},
	{IT_STRING | IT_CVAR,  NULL, "Fire Axis"         , &cv_fireaxis2        , 90},
	{IT_STRING | IT_CVAR,  NULL, "Fire Normal Axis"  , &cv_firenaxis2       ,100},

	{IT_STRING | IT_CVAR, NULL, "First-Person Vert-Look", &cv_alwaysfreelook2,120},
	{IT_STRING | IT_CVAR, NULL, "Third-Person Vert-Look", &cv_chasefreelook2, 130},
	{IT_STRING | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Analog Deadzone", &cv_deadzone2,140},
	{IT_STRING | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Digital Deadzone", &cv_digitaldeadzone2,150},
};

static menuitem_t OP_JoystickSetMenu[1+MAX_JOYSTICKS];

static menuitem_t OP_MouseOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse",        &cv_usemouse,         10},


	{IT_STRING | IT_CVAR, NULL, "First-Person MouseLook", &cv_alwaysfreelook,   30},
	{IT_STRING | IT_CVAR, NULL, "Third-Person MouseLook", &cv_chasefreelook,   40},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove,        50},
	{IT_STRING | IT_CVAR, NULL, "Invert Y Axis",     &cv_invertmouse,      60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Sensitivity",    &cv_mousesens,        70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Sensitivity",    &cv_mouseysens,        80},
};

static menuitem_t OP_Mouse2OptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse 2",      &cv_usemouse2,        10},
	{IT_STRING | IT_CVAR, NULL, "Second Mouse Serial Port",
	                                                &cv_mouse2port,       20},
	{IT_STRING | IT_CVAR, NULL, "First-Person MouseLook", &cv_alwaysfreelook2,  30},
	{IT_STRING | IT_CVAR, NULL, "Third-Person MouseLook", &cv_chasefreelook2,  40},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove2,       50},
	{IT_STRING | IT_CVAR, NULL, "Invert Y Axis",     &cv_invertmouse2,     60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Sensitivity",    &cv_mousesens2,       70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Sensitivity",    &cv_mouseysens2,      80},
};

static menuitem_t OP_CameraOptionsMenu[] =
{
	{IT_HEADER,            NULL, "General Toggles", NULL, 0},
	{IT_STRING  | IT_CVAR, NULL, "Third-person Camera"  , &cv_chasecam , 6},
	{IT_STRING  | IT_CVAR, NULL, "Flip Camera with Gravity"  , &cv_flipcam , 11},
	{IT_STRING  | IT_CVAR, NULL, "Orbital Looking"  , &cv_cam_orbit , 16},
	{IT_STRING  | IT_CVAR, NULL, "Downhill Slope Adjustment", &cv_cam_adjust, 21},

	{IT_HEADER,                                NULL, "Camera Positioning", NULL, 30},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Distance", &cv_cam_savedist[0][0], 36},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Height", &cv_cam_saveheight[0][0], 41},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Camera Spacial Speed", &cv_cam_speed, 46},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Rotation Speed", &cv_cam_turnmultiplier, 51},

	{IT_HEADER,            NULL, "Display Options", NULL, 60},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair, 66},
};

static menuitem_t OP_Camera2OptionsMenu[] =
{
	{IT_HEADER,            NULL, "General Toggles", NULL, 0},
	{IT_STRING  | IT_CVAR, NULL, "Third-person Camera"  , &cv_chasecam2 , 6},
	{IT_STRING  | IT_CVAR, NULL, "Flip Camera with Gravity"  , &cv_flipcam2 , 11},
	{IT_STRING  | IT_CVAR, NULL, "Orbital Looking"  , &cv_cam2_orbit , 16},
	{IT_STRING  | IT_CVAR, NULL, "Downhill Slope Adjustment", &cv_cam2_adjust, 21},

	{IT_HEADER,                                NULL, "Camera Positioning", NULL, 30},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Distance", &cv_cam_savedist[0][1], 36},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Height", &cv_cam_saveheight[0][1], 41},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Camera Spacial Speed", &cv_cam2_speed, 46},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Rotation Speed", &cv_cam2_turnmultiplier, 51},

	{IT_HEADER,            NULL, "Display Options", NULL, 60},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair2, 66},
};

static menuitem_t OP_CameraExtendedOptionsMenu[] =
{
	{IT_HEADER,            NULL, "General Toggles", NULL, 0},
	{IT_STRING  | IT_CVAR, NULL, "Third-person Camera"  , &cv_chasecam , 6},
	{IT_STRING  | IT_CVAR, NULL, "Flip Camera with Gravity"  , &cv_flipcam , 11},
	{IT_STRING  | IT_CVAR, NULL, "Orbital Looking"  , &cv_cam_orbit , 16},
	{IT_STRING  | IT_CVAR, NULL, "Downhill Slope Adjustment", &cv_cam_adjust, 21},

	{IT_HEADER,                                NULL, "Camera Positioning", NULL, 30},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Distance", &cv_cam_savedist[1][0], 36},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Height", &cv_cam_saveheight[1][0], 41},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Camera Spacial Speed", &cv_cam_speed, 46},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Rotation Speed", &cv_cam_turnmultiplier, 51},

	{IT_HEADER,                           NULL, "Automatic Camera Options", NULL, 60},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Shift to player angle", &cv_cam_shiftfacing[0],  66},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to player angle", &cv_cam_turnfacing[0],  71},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to ability", &cv_cam_turnfacingability[0],  76},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to spindash", &cv_cam_turnfacingspindash[0],  81},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to input", &cv_cam_turnfacinginput[0],  86},

	{IT_HEADER,            NULL, "Locked Camera Options", NULL, 95},
	{IT_STRING  | IT_CVAR, NULL, "Lock button behavior", &cv_cam_centertoggle[0],  101},
	{IT_STRING  | IT_CVAR, NULL, "Sideways movement", &cv_cam_lockedinput[0],  106},
	{IT_STRING  | IT_CVAR, NULL, "Targeting assist", &cv_cam_lockonboss[0],  111},

	{IT_HEADER,            NULL, "Display Options", NULL, 120},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair, 126},
};

static menuitem_t OP_Camera2ExtendedOptionsMenu[] =
{
	{IT_HEADER,            NULL, "General Toggles", NULL, 0},
	{IT_STRING  | IT_CVAR, NULL, "Third-person Camera"  , &cv_chasecam2 , 6},
	{IT_STRING  | IT_CVAR, NULL, "Flip Camera with Gravity"  , &cv_flipcam2 , 11},
	{IT_STRING  | IT_CVAR, NULL, "Orbital Looking"  , &cv_cam2_orbit , 16},
	{IT_STRING  | IT_CVAR, NULL, "Downhill Slope Adjustment", &cv_cam2_adjust, 21},

	{IT_HEADER,                                NULL, "Camera Positioning", NULL, 30},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Distance", &cv_cam_savedist[1][1], 36},
	{IT_STRING  | IT_CVAR | IT_CV_INTEGERSTEP, NULL, "Camera Height", &cv_cam_saveheight[1][1], 41},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Camera Spacial Speed", &cv_cam2_speed, 46},
	{IT_STRING  | IT_CVAR | IT_CV_FLOATSLIDER, NULL, "Rotation Speed", &cv_cam2_turnmultiplier, 51},

	{IT_HEADER,                           NULL, "Automatic Camera Options", NULL, 60},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Shift to player angle", &cv_cam_shiftfacing[1],  66},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to player angle", &cv_cam_turnfacing[1],  71},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to ability", &cv_cam_turnfacingability[1],  76},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to spindash", &cv_cam_turnfacingspindash[1],  81},
	{IT_STRING  | IT_CVAR | IT_CV_SLIDER, NULL, "Turn to input", &cv_cam_turnfacinginput[1],  86},

	{IT_HEADER,            NULL, "Locked Camera Options", NULL, 95},
	{IT_STRING  | IT_CVAR, NULL, "Lock button behavior", &cv_cam_centertoggle[1],  101},
	{IT_STRING  | IT_CVAR, NULL, "Sideways movement", &cv_cam_lockedinput[1],  106},
	{IT_STRING  | IT_CVAR, NULL, "Targeting assist", &cv_cam_lockonboss[1],  111},

	{IT_HEADER,            NULL, "Display Options", NULL, 120},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair2, 126},
};

enum
{
	op_video_resolution = 1,
#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	op_video_fullscreen,
#endif
	op_video_vsync,
	op_video_renderer,
};

static menuitem_t OP_VideoOptionsMenu[] =
{
	{IT_HEADER, NULL, "Screen", NULL, 0},
	{IT_STRING | IT_CALL,  NULL, "Set Resolution...",       M_VideoModeMenu,          6},

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	{IT_STRING|IT_CVAR,      NULL, "Fullscreen",             &cv_fullscreen,         11},
#endif
	{IT_STRING | IT_CVAR, NULL, "Vertical Sync",                &cv_vidwait,         16},
#ifdef HWRENDER
	{IT_STRING | IT_CVAR, NULL, "Renderer",                     &cv_renderer,        21},
#else
	{IT_TRANSTEXT | IT_PAIR, "Renderer", "Software",            &cv_renderer,           21},
#endif

	{IT_HEADER, NULL, "Color Profile", NULL, 30},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness (F11)", &cv_globalgamma,36},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", &cv_globalsaturation, 41},
	{IT_SUBMENU|IT_STRING, NULL, "Advanced Settings...",     &OP_ColorOptionsDef,  46},

	{IT_HEADER, NULL, "Heads-Up Display", NULL, 55},
	{IT_STRING | IT_CVAR, NULL, "Show HUD",                  &cv_showhud,          61},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "HUD Transparency",          &cv_translucenthud,   66},
	{IT_STRING | IT_CVAR, NULL, "Score/Time/Rings",          &cv_timetic,          71},
	{IT_STRING | IT_CVAR, NULL, "Show Powerups",             &cv_powerupdisplay,   76},
	{IT_STRING | IT_CVAR, NULL, "Local ping display",		&cv_showping,			81}, // shows ping next to framerate if we want to.
	{IT_STRING | IT_CVAR, NULL, "Show player names",         &cv_seenames,         86},

	{IT_HEADER, NULL, "Console", NULL, 95},
	{IT_STRING | IT_CVAR, NULL, "Background color",          &cons_backcolor,      101},
	{IT_STRING | IT_CVAR, NULL, "Text Size",                 &cv_constextsize,    106},

	{IT_HEADER, NULL, "Chat", NULL, 115},
	{IT_STRING | IT_CVAR, NULL, "Chat Mode",            		 	 &cv_consolechat,  121},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Chat Box Width",    &cv_chatwidth,     126},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Chat Box Height",   &cv_chatheight,    131},
	{IT_STRING | IT_CVAR, NULL, "Message Fadeout Time",              &cv_chattime,    136},
	{IT_STRING | IT_CVAR, NULL, "Chat Notifications",           	 &cv_chatnotifications,  141},
	{IT_STRING | IT_CVAR, NULL, "Spam Protection",           		 &cv_chatspamprotection,  146},
	{IT_STRING | IT_CVAR, NULL, "Chat background tint",           	 &cv_chatbacktint,  151},

	{IT_HEADER, NULL, "Level", NULL, 160},
	{IT_STRING | IT_CVAR, NULL, "Draw Distance",             &cv_drawdist,        166},
	{IT_STRING | IT_CVAR, NULL, "Weather Draw Dist.",        &cv_drawdist_precip, 171},
	{IT_STRING | IT_CVAR, NULL, "NiGHTS Hoop Draw Dist.",    &cv_drawdist_nights, 176},

	{IT_HEADER, NULL, "Diagnostic", NULL, 184},
	{IT_STRING | IT_CVAR, NULL, "Show FPS",                  &cv_ticrate,         190},
	{IT_STRING | IT_CVAR, NULL, "Clear Before Redraw",       &cv_homremoval,      195},
	{IT_STRING | IT_CVAR, NULL, "Show \"FOCUS LOST\"",       &cv_showfocuslost,   200},

#ifdef HWRENDER
	{IT_HEADER, NULL, "Renderer", NULL, 208},
	{IT_CALL | IT_STRING, NULL, "OpenGL Options...",         M_OpenGLOptionsMenu, 214},
#endif
};

static menuitem_t OP_VideoModeMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleVideoMode, 0},     // dummy menuitem for the control func
};

static menuitem_t OP_ColorOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Reset to defaults", M_ResetCvars, 0},

	{IT_HEADER, NULL, "Red", NULL, 9},
	{IT_DISABLED, NULL, NULL, NULL, 35},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",          &cv_rhue,         15},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",   &cv_rsaturation,  20},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",   &cv_rgamma,       25},

	{IT_HEADER, NULL, "Yellow", NULL, 34},
	{IT_DISABLED, NULL, NULL, NULL, 73},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",          &cv_yhue,         40},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",   &cv_ysaturation,  45},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",   &cv_ygamma,       50},

	{IT_HEADER, NULL, "Green", NULL, 59},
	{IT_DISABLED, NULL, NULL, NULL, 112},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",          &cv_ghue,         65},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",   &cv_gsaturation,  70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",   &cv_ggamma,       75},

	{IT_HEADER, NULL, "Cyan", NULL, 84},
	{IT_DISABLED, NULL, NULL, NULL, 255},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",          &cv_chue,         90},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",   &cv_csaturation,  95},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",   &cv_cgamma,      100},

	{IT_HEADER, NULL, "Blue", NULL, 109},
	{IT_DISABLED, NULL, NULL, NULL, 152},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",          &cv_bhue,        115},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",   &cv_bsaturation, 120},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",   &cv_bgamma,      125},

	{IT_HEADER, NULL, "Magenta", NULL, 134},
	{IT_DISABLED, NULL, NULL, NULL, 181},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Hue",          &cv_mhue,        140},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation",   &cv_msaturation, 145},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness",   &cv_mgamma,      150},
};

#ifdef HWRENDER
static menuitem_t OP_OpenGLOptionsMenu[] =
{
	{IT_HEADER, NULL, "3D Models", NULL, 0},
	{IT_STRING|IT_CVAR,         NULL, "Models",              &cv_glmodels,             12},
	{IT_STRING|IT_CVAR,         NULL, "Frame interpolation", &cv_glmodelinterpolation, 22},
	{IT_STRING|IT_CVAR,         NULL, "Ambient lighting",    &cv_glmodellighting,      32},

	{IT_HEADER, NULL, "General", NULL, 51},
	{IT_STRING|IT_CVAR,         NULL, "Shaders",             &cv_glshaders,            63},
	{IT_STRING|IT_CVAR,         NULL, "Lack of perspective", &cv_glshearing,           73},
	{IT_STRING|IT_CVAR,         NULL, "Field of view",       &cv_fov,                  83},

	{IT_HEADER, NULL, "Miscellaneous", NULL, 102},
	{IT_STRING|IT_CVAR,         NULL, "Bit depth",           &cv_scr_depth,           114},
	{IT_STRING|IT_CVAR,         NULL, "Texture filter",      &cv_glfiltermode,        124},
	{IT_STRING|IT_CVAR,         NULL, "Anisotropic",         &cv_glanisotropicmode,   134},
#ifdef ALAM_LIGHTING
	{IT_SUBMENU|IT_STRING,      NULL, "Lighting...",         &OP_OpenGLLightingDef,   144},
#endif
#if defined (_WINDOWS) && (!((defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)))
	{IT_STRING|IT_CVAR,         NULL, "Fullscreen",          &cv_fullscreen,          154},
#endif
};

#ifdef ALAM_LIGHTING
static menuitem_t OP_OpenGLLightingMenu[] =
{
	{IT_STRING|IT_CVAR, NULL, "Coronas",          &cv_glcoronas,          0},
	{IT_STRING|IT_CVAR, NULL, "Coronas size",     &cv_glcoronasize,      10},
	{IT_STRING|IT_CVAR, NULL, "Dynamic lighting", &cv_gldynamiclighting, 20},
	{IT_STRING|IT_CVAR, NULL, "Static lighting",  &cv_glstaticlighting,  30},
};
#endif // ALAM_LIGHTING

#endif

static menuitem_t OP_SoundOptionsMenu[] =
{
	{IT_HEADER, NULL, "Game Audio", NULL, 0},
	{IT_STRING | IT_CVAR,  NULL,  "Sound Effects", &cv_gamesounds, 6},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Sound Volume", &cv_soundvolume, 11},

	{IT_STRING | IT_CVAR,  NULL,  "Digital Music", &cv_gamedigimusic, 21},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Digital Music Volume", &cv_digmusicvolume,  26},

	{IT_STRING | IT_CVAR,  NULL,  "MIDI Music", &cv_gamemidimusic, 36},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "MIDI Music Volume", &cv_midimusicvolume, 41},

	{IT_STRING | IT_CVAR,  NULL,  "Music Preference", &cv_musicpref, 51},

	{IT_HEADER, NULL, "Miscellaneous", NULL, 61},
	{IT_STRING | IT_CVAR, NULL, "Closed Captioning", &cv_closedcaptioning, 67},
	{IT_STRING | IT_CVAR, NULL, "Reset Music Upon Dying", &cv_resetmusic, 72},
	{IT_STRING | IT_CVAR, NULL, "Default 1-Up sound", &cv_1upsound, 77},

	{IT_STRING | IT_SUBMENU, NULL, "Advanced Settings...", &OP_SoundAdvancedDef, 87},
};

#ifdef HAVE_OPENMPT
#define OPENMPT_MENUOFFSET 32
#else
#define OPENMPT_MENUOFFSET 0
#endif

#ifdef HAVE_MIXERX
#define MIXERX_MENUOFFSET 81
#else
#define MIXERX_MENUOFFSET 0
#endif

static menuitem_t OP_SoundAdvancedMenu[] =
{
#ifdef HAVE_OPENMPT
	{IT_HEADER, NULL, "OpenMPT Settings", NULL, 0},
	{IT_STRING | IT_CVAR, NULL, "Instrument Filter", &cv_modfilter, 12},
#endif

#ifdef HAVE_MIXERX
	{IT_HEADER, NULL, "MIDI Settings", NULL, OPENMPT_MENUOFFSET},
	{IT_STRING | IT_CVAR, NULL, "MIDI Player", &cv_midiplayer, OPENMPT_MENUOFFSET+12},
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, "FluidSynth Sound Font File", &cv_midisoundfontpath, OPENMPT_MENUOFFSET+24},
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, "TiMidity++ Config Folder", &cv_miditimiditypath, OPENMPT_MENUOFFSET+51},
#endif

	{IT_HEADER, NULL, "Miscellaneous", NULL, OPENMPT_MENUOFFSET+MIXERX_MENUOFFSET},
	{IT_STRING | IT_CVAR, NULL, "Play Sound Effects if Unfocused", &cv_playsoundsifunfocused, OPENMPT_MENUOFFSET+MIXERX_MENUOFFSET+12},
	{IT_STRING | IT_CVAR, NULL, "Play Music if Unfocused", &cv_playmusicifunfocused, OPENMPT_MENUOFFSET+MIXERX_MENUOFFSET+22},
	{IT_STRING | IT_CVAR, NULL, "Let Levels Force Reset Music", &cv_resetmusicbyheader, OPENMPT_MENUOFFSET+MIXERX_MENUOFFSET+32},
};

#undef OPENMPT_MENUOFFSET
#undef MIXERX_MENUOFFSET

static menuitem_t OP_DataOptionsMenu[] =
{
	{IT_STRING | IT_CALL,    NULL, "Add-on Options...",     M_AddonsOptions,     10},
	{IT_STRING | IT_CALL,    NULL, "Screenshot Options...", M_ScreenshotOptions, 20},

	{IT_STRING | IT_SUBMENU, NULL, "\x85" "Erase Data...",  &OP_EraseDataDef,    40},
};

static menuitem_t OP_ScreenshotOptionsMenu[] =
{
	{IT_HEADER, NULL, "General", NULL, 0},
	{IT_STRING|IT_CVAR, NULL, "Use color profile", &cv_screenshot_colorprofile,     6},

	{IT_HEADER, NULL, "Screenshots (F8)", NULL, 16},
	{IT_STRING|IT_CVAR, NULL, "Storage Location",  &cv_screenshot_option,          22},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder", &cv_screenshot_folder, 27},
	{IT_STRING|IT_CVAR, NULL, "Memory Level",      &cv_zlib_memory,                42},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", &cv_zlib_level,                 47},
	{IT_STRING|IT_CVAR, NULL, "Strategy",          &cv_zlib_strategy,              52},
	{IT_STRING|IT_CVAR, NULL, "Window Size",       &cv_zlib_window_bits,           57},

	{IT_HEADER, NULL, "Movie Mode (F9)", NULL, 64},
	{IT_STRING|IT_CVAR, NULL, "Storage Location",  &cv_movie_option,               70},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder", &cv_movie_folder, 	   75},
	{IT_STRING|IT_CVAR, NULL, "Capture Mode",      &cv_moviemode,                  90},

	{IT_STRING|IT_CVAR, NULL, "Downscaling",       &cv_gif_downscale,              95},
	{IT_STRING|IT_CVAR, NULL, "Region Optimizing", &cv_gif_optimize,              100},
	{IT_STRING|IT_CVAR, NULL, "Local Color Table", &cv_gif_localcolortable,       105},

	{IT_STRING|IT_CVAR, NULL, "Downscaling",       &cv_apng_downscale,             95},
	{IT_STRING|IT_CVAR, NULL, "Memory Level",      &cv_zlib_memorya,              100},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", &cv_zlib_levela,               105},
	{IT_STRING|IT_CVAR, NULL, "Strategy",          &cv_zlib_strategya,            110},
	{IT_STRING|IT_CVAR, NULL, "Window Size",       &cv_zlib_window_bitsa,         115},
};

enum
{
	op_screenshot_colorprofile = 1,
	op_screenshot_storagelocation = 3,
	op_screenshot_folder = 4,
	op_movie_folder = 11,
	op_screenshot_capture = 12,
	op_screenshot_gif_start = 13,
	op_screenshot_gif_end = 15,
	op_screenshot_apng_start = 16,
	op_screenshot_apng_end = 20,
};

static menuitem_t OP_EraseDataMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Erase Record Data", M_EraseData, 10},
	{IT_STRING | IT_CALL, NULL, "Erase Extras Data", M_EraseData, 20},

	{IT_STRING | IT_CALL, NULL, "\x85" "Erase ALL Data", M_EraseData, 40},
};

static menuitem_t OP_AddonsOptionsMenu[] =
{
	{IT_HEADER,                      NULL, "Menu",                        NULL,                     0},
	{IT_STRING|IT_CVAR,              NULL, "Location",                    &cv_addons_option,       12},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder",               &cv_addons_folder,       22},
	{IT_STRING|IT_CVAR,              NULL, "Identify add-ons via",        &cv_addons_md5,          50},
	{IT_STRING|IT_CVAR,              NULL, "Show unsupported file types", &cv_addons_showall,      60},

	{IT_HEADER,                      NULL, "Search",                      NULL,                    78},
	{IT_STRING|IT_CVAR,              NULL, "Matching",                    &cv_addons_search_type,  90},
	{IT_STRING|IT_CVAR,              NULL, "Case-sensitive",              &cv_addons_search_case, 100},
};

enum
{
	op_addons_folder = 2,
};

static menuitem_t OP_ServerOptionsMenu[] =
{
	{IT_HEADER, NULL, "General", NULL, 0},
#ifndef NONET
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                         NULL, "Server name",                      &cv_servername,           7},
	{IT_STRING | IT_CVAR,    NULL, "Max Players",                      &cv_maxplayers,          21},
	{IT_STRING | IT_CVAR,    NULL, "Allow Add-on Downloading",         &cv_downloading,         26},
	{IT_STRING | IT_CVAR,    NULL, "Allow players to join",            &cv_allownewplayer,      31},
	{IT_STRING | IT_CVAR,    NULL, "Minutes for reconnecting",         &cv_rejointimeout,       36},
#endif
	{IT_STRING | IT_CVAR,    NULL, "Map progression",                  &cv_advancemap,          41},
	{IT_STRING | IT_CVAR,    NULL, "Intermission Timer",               &cv_inttime,             46},

	{IT_HEADER, NULL, "Characters", NULL, 55},
	{IT_STRING | IT_CVAR,    NULL, "Force a character",                &cv_forceskin,           61},
	{IT_STRING | IT_CVAR,    NULL, "Restrict character changes",       &cv_restrictskinchange,  66},

	{IT_HEADER, NULL, "Items", NULL, 75},
	{IT_STRING | IT_CVAR,    NULL, "Item respawn delay",               &cv_itemrespawntime,     81},
	{IT_STRING | IT_SUBMENU, NULL, "Mystery Item Monitor Toggles...",  &OP_MonitorToggleDef,    86},

	{IT_HEADER, NULL, "Cooperative", NULL, 95},
	{IT_STRING | IT_CVAR,    NULL, "Players required for exit",        &cv_playersforexit,     101},
	{IT_STRING | IT_CVAR,    NULL, "Starposts",                        &cv_coopstarposts,      106},
	{IT_STRING | IT_CVAR,    NULL, "Life sharing",                     &cv_cooplives,          111},
	{IT_STRING | IT_CVAR,    NULL, "Post-goal free roaming",           &cv_exitmove,           116},

	{IT_HEADER, NULL, "Race, Competition", NULL, 125},
	{IT_STRING | IT_CVAR,    NULL, "Level completion countdown",       &cv_countdowntime,      131},
	{IT_STRING | IT_CVAR,    NULL, "Item Monitors",                    &cv_competitionboxes,   136},

	{IT_HEADER, NULL, "Ringslinger (Match, CTF, Tag, H&S)", NULL, 145},
	{IT_STRING | IT_CVAR,    NULL, "Time Limit",                       &cv_timelimit,          151},
	{IT_STRING | IT_CVAR,    NULL, "Score Limit",                      &cv_pointlimit,         156},
	{IT_STRING | IT_CVAR,    NULL, "Overtime on Tie",                  &cv_overtime,           161},
	{IT_STRING | IT_CVAR,    NULL, "Player respawn delay",             &cv_respawntime,        166},

	{IT_STRING | IT_CVAR,    NULL, "Item Monitors",                    &cv_matchboxes,         176},
	{IT_STRING | IT_CVAR,    NULL, "Weapon Rings",                     &cv_specialrings,       181},
	{IT_STRING | IT_CVAR,    NULL, "Power Stones",                     &cv_powerstones,        186},

	{IT_STRING | IT_CVAR,    NULL, "Flag respawn delay",               &cv_flagtime,           196},
	{IT_STRING | IT_CVAR,    NULL, "Hiding time",                      &cv_hidetime,           201},

	{IT_HEADER, NULL, "Teams", NULL, 210},
	{IT_STRING | IT_CVAR,    NULL, "Autobalance sizes",                &cv_autobalance,        216},
	{IT_STRING | IT_CVAR,    NULL, "Scramble on Map Change",           &cv_scrambleonchange,   221},

#ifndef NONET
	{IT_HEADER, NULL, "Advanced", NULL, 230},
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, "Master server",        &cv_masterserver,       236},

	{IT_STRING | IT_CVAR,    NULL, "Join delay",                       &cv_joindelay,          251},
	{IT_STRING | IT_CVAR,    NULL, "Attempts to resynchronise",        &cv_resynchattempts,    256},

	{IT_STRING | IT_CVAR,    NULL, "Show IP Address of Joiners",       &cv_showjoinaddress,    261},
#endif
};

static menuitem_t OP_MonitorToggleMenu[] =
{
	// Printing handled by drawing function
	{IT_STRING|IT_CALL, NULL, "Reset to defaults", M_ResetCvars, 15},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Recycler",          &cv_recycler,      30},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Teleport",          &cv_teleporters,   40},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Super Ring",        &cv_superring,     50},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Super Sneakers",    &cv_supersneakers, 60},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Invincibility",     &cv_invincibility, 70},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Whirlwind Shield",  &cv_jumpshield,    80},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Elemental Shield",  &cv_watershield,   90},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Attraction Shield", &cv_ringshield,   100},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Force Shield",      &cv_forceshield,  110},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Armageddon Shield", &cv_bombshield,   120},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "1 Up",              &cv_1up,          130},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Eggman Box",        &cv_eggmanbox,    140},
};

// ==========================================================================
// ALL MENU DEFINITIONS GO HERE
// ==========================================================================

// Main Menu and related
menu_t MainDef = CENTERMENUSTYLE(MN_MAIN, NULL, MainMenu, NULL, 72);

menu_t MISC_AddonsDef =
{
	MN_AD_MAIN,
	NULL,
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	&MainDef,
	MISC_AddonsMenu,
	M_DrawAddons,
	50, 28,
	0,
	NULL
};

menu_t MAPauseDef = PAUSEMENUSTYLE(MAPauseMenu, 40, 72);
menu_t SPauseDef = PAUSEMENUSTYLE(SPauseMenu, 40, 72);
menu_t MPauseDef = PAUSEMENUSTYLE(MPauseMenu, 40, 72);

// Misc Main Menu
menu_t MISC_ScrambleTeamDef = DEFAULTMENUSTYLE(MN_SPECIAL, NULL, MISC_ScrambleTeamMenu, &MPauseDef, 27, 40);
menu_t MISC_ChangeTeamDef = DEFAULTMENUSTYLE(MN_SPECIAL, NULL, MISC_ChangeTeamMenu, &MPauseDef, 27, 40);

// MP Gametype and map change menu
menu_t MISC_ChangeLevelDef =
{
	MN_SPECIAL,
	NULL,
	sizeof (MISC_ChangeLevelMenu)/sizeof (menuitem_t),
	&MainDef,  // Doesn't matter.
	MISC_ChangeLevelMenu,
	M_DrawLevelPlatterMenu,
	0, 0,
	0,
	NULL
};

menu_t MISC_HelpDef = IMAGEDEF(MISC_HelpMenu);

static INT32 highlightflags, recommendedflags, warningflags;


// Sky Room
menu_t SR_PandoraDef =
{
	MTREE2(MN_SR_MAIN, MN_SR_PANDORA),
	"M_PANDRA",
	sizeof (SR_PandorasBox)/sizeof (menuitem_t),
	&SPauseDef,
	SR_PandorasBox,
	M_DrawGenericMenu,
	60, 30,
	0,
	M_ExitPandorasBox
};

menu_t SR_MainDef = DEFAULTMENUSTYLE(MN_SR_MAIN, "M_SECRET", SR_MainMenu, &MainDef, 60, 40);

menu_t SR_LevelSelectDef = MAPPLATTERMENUSTYLE(
	MTREE2(MN_SR_MAIN, MN_SR_LEVELSELECT),
	NULL, SR_LevelSelectMenu);

menu_t SR_UnlockChecklistDef =
{
	MTREE2(MN_SR_MAIN, MN_SR_UNLOCKCHECKLIST),
	"M_SECRET",
	1,
	&SR_MainDef,
	SR_UnlockChecklistMenu,
	M_DrawChecklist,
	30, 30,
	0,
	NULL
};

menu_t SR_SoundTestDef =
{
	MTREE2(MN_SR_MAIN, MN_SR_SOUNDTEST),
	NULL,
	sizeof (SR_SoundTestMenu)/sizeof (menuitem_t),
	&SR_MainDef,
	SR_SoundTestMenu,
	M_DrawSoundTest,
	60, 150,
	0,
	NULL
};

menu_t SR_EmblemHintDef =
{
	MTREE2(MN_SR_MAIN, MN_SR_EMBLEMHINT),
	NULL,
	sizeof (SR_EmblemHintMenu)/sizeof (menuitem_t),
	&SPauseDef,
	SR_EmblemHintMenu,
	M_DrawEmblemHints,
	60, 150,
	0,
	NULL
};

// Single Player
menu_t SP_MainDef = //CENTERMENUSTYLE(NULL, SP_MainMenu, &MainDef, 72);
{
	MN_SP_MAIN,
	NULL,
	sizeof(SP_MainMenu)/sizeof(menuitem_t),
	&MainDef,
	SP_MainMenu,
	M_DrawCenteredMenu,
	BASEVIDWIDTH/2, 72,
	0,
	NULL
};

menu_t SP_LoadDef =
{
	MTREE2(MN_SP_MAIN, MN_SP_LOAD),
	"M_PICKG",
	1,
	&SP_MainDef,
	SP_LoadGameMenu,
	M_DrawLoad,
	68, 46,
	0,
	NULL
};

menu_t SP_LevelSelectDef = MAPPLATTERMENUSTYLE(
	MTREE4(MN_SP_MAIN, MN_SP_LOAD, MN_SP_PLAYER, MN_SP_LEVELSELECT),
	NULL, SP_LevelSelectMenu);

menu_t SP_LevelStatsDef =
{
	MTREE2(MN_SP_MAIN, MN_SP_LEVELSTATS),
	"M_STATS",
	1,
	&SP_MainDef,
	SP_LevelStatsMenu,
	M_DrawLevelStats,
	280, 185,
	0,
	NULL
};

menu_t SP_TimeAttackLevelSelectDef = MAPPLATTERMENUSTYLE(
	MTREE3(MN_SP_MAIN, MN_SP_TIMEATTACK, MN_SP_TIMEATTACK_LEVELSELECT),
	"M_ATTACK", SP_TimeAttackLevelSelectMenu);

static menu_t SP_TimeAttackDef =
{
	MTREE2(MN_SP_MAIN, MN_SP_TIMEATTACK),
	"M_ATTACK",
	sizeof (SP_TimeAttackMenu)/sizeof (menuitem_t),
	&MainDef,  // Doesn't matter.
	SP_TimeAttackMenu,
	M_DrawTimeAttackMenu,
	32, 40,
	0,
	NULL
};
static menu_t SP_ReplayDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_TIMEATTACK, MN_SP_REPLAY),
	"M_ATTACK",
	sizeof(SP_ReplayMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_ReplayMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_GuestReplayDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_TIMEATTACK, MN_SP_GUESTREPLAY),
	"M_ATTACK",
	sizeof(SP_GuestReplayMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_GuestReplayMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_GhostDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_TIMEATTACK, MN_SP_GHOST),
	"M_ATTACK",
	sizeof(SP_GhostMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_GhostMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};

menu_t SP_NightsAttackLevelSelectDef = MAPPLATTERMENUSTYLE(
	MTREE3(MN_SP_MAIN, MN_SP_NIGHTSATTACK, MN_SP_NIGHTS_LEVELSELECT),
	 "M_NIGHTS", SP_NightsAttackLevelSelectMenu);

static menu_t SP_NightsAttackDef =
{
	MTREE2(MN_SP_MAIN, MN_SP_NIGHTSATTACK),
	"M_NIGHTS",
	sizeof (SP_NightsAttackMenu)/sizeof (menuitem_t),
	&MainDef,  // Doesn't matter.
	SP_NightsAttackMenu,
	M_DrawNightsAttackMenu,
	32, 40,
	0,
	NULL
};
static menu_t SP_NightsReplayDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_NIGHTSATTACK, MN_SP_NIGHTS_REPLAY),
	"M_NIGHTS",
	sizeof(SP_NightsReplayMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsReplayMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_NightsGuestReplayDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_NIGHTSATTACK, MN_SP_NIGHTS_GUESTREPLAY),
	"M_NIGHTS",
	sizeof(SP_NightsGuestReplayMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsGuestReplayMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};
static menu_t SP_NightsGhostDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_NIGHTSATTACK, MN_SP_NIGHTS_GHOST),
	"M_NIGHTS",
	sizeof(SP_NightsGhostMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsGhostMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};

static menu_t SP_MarathonDef =
{
	MTREE2(MN_SP_MAIN, MN_SP_MARATHON),
	"M_RATHON",
	sizeof(SP_MarathonMenu)/sizeof(menuitem_t),
	&MainDef,  // Doesn't matter.
	SP_MarathonMenu,
	M_DrawMarathon,
	32, 40,
	0,
	NULL
};

menu_t SP_PlayerDef =
{
	MTREE3(MN_SP_MAIN, MN_SP_LOAD, MN_SP_PLAYER),
	"M_PICKP",
	sizeof (SP_PlayerMenu)/sizeof (menuitem_t),
	&SP_MainDef,
	SP_PlayerMenu,
	M_DrawSetupChoosePlayerMenu,
	24, 32,
	0,
	NULL
};

// Multiplayer

menu_t MP_SplitServerDef =
{
	MTREE2(MN_MP_MAIN, MN_MP_SPLITSCREEN),
	"M_MULTI",
	sizeof (MP_SplitServerMenu)/sizeof (menuitem_t),
#ifndef NONET
	&MP_MainDef,
#else
	&MainDef,
#endif
	MP_SplitServerMenu,
	M_DrawServerMenu,
	27, 30 - 50,
	0,
	NULL
};

#ifndef NONET

menu_t MP_MainDef =
{
	MN_MP_MAIN,
	"M_MULTI",
	sizeof (MP_MainMenu)/sizeof (menuitem_t),
	&MainDef,
	MP_MainMenu,
	M_DrawMPMainMenu,
	27, 40,
	0,
	M_CancelConnect
};

menu_t MP_ServerDef =
{
	MTREE2(MN_MP_MAIN, MN_MP_SERVER),
	"M_MULTI",
	sizeof (MP_ServerMenu)/sizeof (menuitem_t),
	&MP_MainDef,
	MP_ServerMenu,
	M_DrawServerMenu,
	27, 30,
	0,
	NULL
};

menu_t MP_ConnectDef =
{
	MTREE2(MN_MP_MAIN, MN_MP_CONNECT),
	"M_MULTI",
	sizeof (MP_ConnectMenu)/sizeof (menuitem_t),
	&MP_MainDef,
	MP_ConnectMenu,
	M_DrawConnectMenu,
	27,24,
	0,
	M_CancelConnect
};

menu_t MP_RoomDef =
{
	MTREE2(MN_MP_MAIN, MN_MP_ROOM),
	"M_MULTI",
	sizeof (MP_RoomMenu)/sizeof (menuitem_t),
	&MP_ConnectDef,
	MP_RoomMenu,
	M_DrawRoomMenu,
	27, 32,
	0,
	NULL
};
#endif

menu_t MP_PlayerSetupDef =
{
#ifdef NONET
	MTREE2(MN_MP_MAIN, MN_MP_PLAYERSETUP),
#else
	MTREE3(MN_MP_MAIN, MN_MP_SPLITSCREEN, MN_MP_PLAYERSETUP),
#endif
	"M_SPLAYR",
	sizeof (MP_PlayerSetupMenu)/sizeof (menuitem_t),
	&MainDef, // doesn't matter
	MP_PlayerSetupMenu,
	M_DrawSetupMultiPlayerMenu,
	19, 22,
	0,
	M_QuitMultiPlayerMenu
};

// Options
menu_t OP_MainDef = DEFAULTMENUSTYLE(
	MN_OP_MAIN,
	"M_OPTTTL", OP_MainMenu, &MainDef, 50, 30);

menu_t OP_ChangeControlsDef = CONTROLMENUSTYLE(
	MTREE3(MN_OP_MAIN, 0, MN_OP_CHANGECONTROLS), // second level set on runtime
	OP_ChangeControlsMenu, &OP_MainDef);

menu_t OP_P1ControlsDef = {
	MTREE2(MN_OP_MAIN, MN_OP_P1CONTROLS),
	"M_CONTRO",
	sizeof(OP_P1ControlsMenu)/sizeof(menuitem_t),
	&OP_MainDef,
	OP_P1ControlsMenu,
	M_DrawControlsDefMenu,
	50, 30, 0, NULL};
menu_t OP_P2ControlsDef = {
	MTREE2(MN_OP_MAIN, MN_OP_P2CONTROLS),
	"M_CONTRO",
	sizeof(OP_P2ControlsMenu)/sizeof(menuitem_t),
	&OP_MainDef,
	OP_P2ControlsMenu,
	M_DrawControlsDefMenu,
	50, 30, 0, NULL};

menu_t OP_MouseOptionsDef = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_P1CONTROLS, MN_OP_P1MOUSE),
	"M_CONTRO", OP_MouseOptionsMenu, &OP_P1ControlsDef, 35, 30);
menu_t OP_Mouse2OptionsDef = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_P2CONTROLS, MN_OP_P2MOUSE),
	"M_CONTRO", OP_Mouse2OptionsMenu, &OP_P2ControlsDef, 35, 30);

menu_t OP_Joystick1Def = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_P1CONTROLS, MN_OP_P1JOYSTICK),
	"M_CONTRO", OP_Joystick1Menu, &OP_P1ControlsDef, 50, 30);
menu_t OP_Joystick2Def = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_P2CONTROLS, MN_OP_P2JOYSTICK),
	"M_CONTRO", OP_Joystick2Menu, &OP_P2ControlsDef, 50, 30);

menu_t OP_JoystickSetDef =
{
	MTREE4(MN_OP_MAIN, 0, 0, MN_OP_JOYSTICKSET), // second and third level set on runtime
	"M_CONTRO",
	sizeof (OP_JoystickSetMenu)/sizeof (menuitem_t),
	&OP_Joystick1Def,
	OP_JoystickSetMenu,
	M_DrawJoystick,
	60, 40,
	0,
	NULL
};

menu_t OP_CameraOptionsDef = {
	MTREE3(MN_OP_MAIN, MN_OP_P1CONTROLS, MN_OP_P1CAMERA),
	"M_CONTRO",
	sizeof (OP_CameraOptionsMenu)/sizeof (menuitem_t),
	&OP_P1ControlsDef,
	OP_CameraOptionsMenu,
	M_DrawCameraOptionsMenu,
	35, 30,
	0,
	NULL
};
menu_t OP_Camera2OptionsDef = {
	MTREE3(MN_OP_MAIN, MN_OP_P2CONTROLS, MN_OP_P2CAMERA),
	"M_CONTRO",
	sizeof (OP_Camera2OptionsMenu)/sizeof (menuitem_t),
	&OP_P2ControlsDef,
	OP_Camera2OptionsMenu,
	M_DrawCameraOptionsMenu,
	35, 30,
	0,
	NULL
};

static menuitem_t OP_PlaystyleMenu[] = {{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandlePlaystyleMenu, 0}};

menu_t OP_PlaystyleDef = {
	MTREE3(MN_OP_MAIN, MN_OP_P1CONTROLS, MN_OP_PLAYSTYLE), ///@TODO the second level should be set in runtime
	NULL,
	1,
	&OP_P1ControlsDef,
	OP_PlaystyleMenu,
	M_DrawPlaystyleMenu,
	0, 0, 0, NULL
};

static void M_VideoOptions(INT32 choice)
{
	(void)choice;

	OP_VideoOptionsMenu[op_video_renderer].status = (IT_TRANSTEXT | IT_PAIR);
	OP_VideoOptionsMenu[op_video_renderer].patch = "Renderer";
	OP_VideoOptionsMenu[op_video_renderer].text = "Software";

#ifdef HWRENDER
	if (vid.glstate != VID_GL_LIBRARY_ERROR)
	{
		OP_VideoOptionsMenu[op_video_renderer].status = (IT_STRING | IT_CVAR);
		OP_VideoOptionsMenu[op_video_renderer].patch = NULL;
		OP_VideoOptionsMenu[op_video_renderer].text = "Renderer";
	}
#endif

	M_SetupNextMenu(&OP_VideoOptionsDef);
}

menu_t OP_VideoOptionsDef =
{
	MTREE2(MN_OP_MAIN, MN_OP_VIDEO),
	"M_VIDEO",
	sizeof (OP_VideoOptionsMenu)/sizeof (menuitem_t),
	&OP_MainDef,
	OP_VideoOptionsMenu,
	M_DrawMainVideoMenu,
	30, 30,
	0,
	NULL
};
menu_t OP_VideoModeDef =
{
	MTREE3(MN_OP_MAIN, MN_OP_VIDEO, MN_OP_VIDEOMODE),
	"M_VIDEO",
	1,
	&OP_VideoOptionsDef,
	OP_VideoModeMenu,
	M_DrawVideoMode,
	48, 26,
	0,
	NULL
};
menu_t OP_ColorOptionsDef =
{
	MTREE3(MN_OP_MAIN, MN_OP_VIDEO, MN_OP_COLOR),
	"M_VIDEO",
	sizeof (OP_ColorOptionsMenu)/sizeof (menuitem_t),
	&OP_VideoOptionsDef,
	OP_ColorOptionsMenu,
	M_DrawColorMenu,
	30, 30,
	0,
	NULL
};
menu_t OP_SoundOptionsDef = DEFAULTSCROLLMENUSTYLE(
	MTREE2(MN_OP_MAIN, MN_OP_SOUND),
	"M_SOUND", OP_SoundOptionsMenu, &OP_MainDef, 30, 30);
menu_t OP_SoundAdvancedDef = DEFAULTMENUSTYLE(
	MTREE2(MN_OP_MAIN, MN_OP_SOUND),
	"M_SOUND", OP_SoundAdvancedMenu, &OP_SoundOptionsDef, 30, 30);

menu_t OP_ServerOptionsDef = DEFAULTSCROLLMENUSTYLE(
	MTREE2(MN_OP_MAIN, MN_OP_SERVER),
	"M_SERVER", OP_ServerOptionsMenu, &OP_MainDef, 30, 30);

menu_t OP_MonitorToggleDef =
{
	MTREE3(MN_OP_MAIN, MN_OP_SOUND, MN_OP_MONITORTOGGLE),
	"M_SERVER",
	sizeof (OP_MonitorToggleMenu)/sizeof (menuitem_t),
	&OP_ServerOptionsDef,
	OP_MonitorToggleMenu,
	M_DrawMonitorToggles,
	30, 30,
	0,
	NULL
};

#ifdef HWRENDER
static void M_OpenGLOptionsMenu(void)
{
	if (rendermode == render_opengl)
		M_SetupNextMenu(&OP_OpenGLOptionsDef);
	else
		M_StartMessage(M_GetText("You must be in OpenGL mode\nto access this menu.\n\n(Press a key)\n"), NULL, MM_NOTHING);
}

menu_t OP_OpenGLOptionsDef = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_VIDEO, MN_OP_OPENGL),
	"M_VIDEO", OP_OpenGLOptionsMenu, &OP_VideoOptionsDef, 30, 30);
#ifdef ALAM_LIGHTING
menu_t OP_OpenGLLightingDef = DEFAULTMENUSTYLE(
	MTREE4(MN_OP_MAIN, MN_OP_VIDEO, MN_OP_OPENGL, MN_OP_OPENGL_LIGHTING),
	"M_VIDEO", OP_OpenGLLightingMenu, &OP_OpenGLOptionsDef, 60, 40);
#endif // ALAM_LIGHTING
#endif // HWRENDER

menu_t OP_DataOptionsDef = DEFAULTMENUSTYLE(
	MTREE2(MN_OP_MAIN, MN_OP_DATA),
	"M_DATA", OP_DataOptionsMenu, &OP_MainDef, 60, 30);

menu_t OP_ScreenshotOptionsDef =
{
	MTREE3(MN_OP_MAIN, MN_OP_DATA, MN_OP_SCREENSHOTS),
	"M_SCREEN",
	sizeof (OP_ScreenshotOptionsMenu)/sizeof (menuitem_t),
	&OP_DataOptionsDef,
	OP_ScreenshotOptionsMenu,
	M_DrawScreenshotMenu,
	30, 30,
	0,
	NULL
};

menu_t OP_AddonsOptionsDef = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_DATA, MN_OP_ADDONS),
	"M_ADDONS", OP_AddonsOptionsMenu, &OP_DataOptionsDef, 30, 30);

menu_t OP_EraseDataDef = DEFAULTMENUSTYLE(
	MTREE3(MN_OP_MAIN, MN_OP_DATA, MN_OP_ERASEDATA),
	"M_DATA", OP_EraseDataMenu, &OP_DataOptionsDef, 60, 30);

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

// Prototypes
static INT32 M_GetFirstLevelInList(INT32 gt);
static boolean M_CanShowLevelOnPlatter(INT32 mapnum, INT32 gt);

// Nextmap.  Used for Level select.
void Nextmap_OnChange(void)
{
	char *leveltitle;
	char tabase[256];
#ifdef OLDNREPLAYNAME
	char tabaseold[256];
#endif
	short i;
	boolean active;

	// Update the string in the consvar.
	Z_Free(cv_nextmap.zstring);
	leveltitle = G_BuildMapTitle(cv_nextmap.value);
	cv_nextmap.string = cv_nextmap.zstring = leveltitle ? leveltitle : Z_StrDup(G_BuildMapName(cv_nextmap.value));

	if (currentMenu == &SP_NightsAttackDef)
	{
		CV_StealthSetValue(&cv_dummymares, 0);
		// Hide the record changing CVAR if only one mare is available.
		if (!nightsrecords[cv_nextmap.value-1] || nightsrecords[cv_nextmap.value-1]->nummares < 2)
			SP_NightsAttackMenu[narecords].status = IT_DISABLED;
		else
			SP_NightsAttackMenu[narecords].status = IT_STRING|IT_CVAR;

		// Do the replay things.
		active = false;
		SP_NightsAttackMenu[naguest].status = IT_DISABLED;
		SP_NightsAttackMenu[nareplay].status = IT_DISABLED;
		SP_NightsAttackMenu[naghost].status = IT_DISABLED;

		// Check if file exists, if not, disable REPLAY option
		sprintf(tabase,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), skins[cv_chooseskin.value-1].name);

#ifdef OLDNREPLAYNAME
		sprintf(tabaseold,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));
#endif

		for (i = 0; i < 4; i++) {
			SP_NightsReplayMenu[i].status = IT_DISABLED;
			SP_NightsGuestReplayMenu[i].status = IT_DISABLED;
		}

		if (FIL_FileExists(va("%s-score-best.lmp", tabase))) {
			SP_NightsReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-time-best.lmp", tabase))) {
			SP_NightsReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-last.lmp", tabase))) {
			SP_NightsReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value))))  {
			SP_NightsReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}

		// Old style name compatibility
#ifdef OLDNREPLAYNAME
		if (FIL_FileExists(va("%s-score-best.lmp", tabaseold))) {
			SP_NightsReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-time-best.lmp", tabaseold))) {
			SP_NightsReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-last.lmp", tabaseold))) {
			SP_NightsReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
#endif

		if (active) {
			SP_NightsAttackMenu[naguest].status = IT_WHITESTRING|IT_SUBMENU;
			SP_NightsAttackMenu[nareplay].status = IT_WHITESTRING|IT_SUBMENU;
			SP_NightsAttackMenu[naghost].status = IT_WHITESTRING|IT_SUBMENU;
		}

		else if(itemOn == nareplay) // Reset lastOn so replay isn't still selected when not available.
		{
			currentMenu->lastOn = itemOn;
			itemOn = nastart;
		}
	}
	else if (currentMenu == &SP_TimeAttackDef)
	{
		active = false;
		SP_TimeAttackMenu[taguest].status = IT_DISABLED;
		SP_TimeAttackMenu[tareplay].status = IT_DISABLED;
		SP_TimeAttackMenu[taghost].status = IT_DISABLED;

		// Check if file exists, if not, disable REPLAY option
		sprintf(tabase,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), skins[cv_chooseskin.value-1].name);
		for (i = 0; i < 5; i++) {
			SP_ReplayMenu[i].status = IT_DISABLED;
			SP_GuestReplayMenu[i].status = IT_DISABLED;
		}
		if (FIL_FileExists(va("%s-time-best.lmp", tabase))) {
			SP_ReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[0].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-score-best.lmp", tabase))) {
			SP_ReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[1].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-rings-best.lmp", tabase))) {
			SP_ReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[2].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s-last.lmp", tabase))) {
			SP_ReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)))) {
			SP_ReplayMenu[4].status = IT_WHITESTRING|IT_CALL;
			SP_GuestReplayMenu[4].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
		if (active) {
			SP_TimeAttackMenu[taguest].status = IT_WHITESTRING|IT_SUBMENU;
			SP_TimeAttackMenu[tareplay].status = IT_WHITESTRING|IT_SUBMENU;
			SP_TimeAttackMenu[taghost].status = IT_WHITESTRING|IT_SUBMENU;
		}
		else if(itemOn == tareplay) // Reset lastOn so replay isn't still selected when not available.
		{
			currentMenu->lastOn = itemOn;
			itemOn = tastart;
		}

		if (mapheaderinfo[cv_nextmap.value-1] && mapheaderinfo[cv_nextmap.value-1]->forcecharacter[0] != '\0')
			CV_Set(&cv_chooseskin, mapheaderinfo[cv_nextmap.value-1]->forcecharacter);
	}
}

static void Dummymares_OnChange(void)
{
	if (!nightsrecords[cv_nextmap.value-1])
	{
		CV_StealthSetValue(&cv_dummymares, 0);
		return;
	}
	else
	{
		UINT8 mares = nightsrecords[cv_nextmap.value-1]->nummares;

		if (cv_dummymares.value < 0)
			CV_StealthSetValue(&cv_dummymares, mares);
		else if (cv_dummymares.value > mares)
			CV_StealthSetValue(&cv_dummymares, 0);
	}
}

// Newgametype.  Used for gametype changes.
static void Newgametype_OnChange(void)
{
	if (menuactive)
	{
		if(!mapheaderinfo[cv_nextmap.value-1])
			P_AllocMapHeader((INT16)(cv_nextmap.value-1));

		if (!M_CanShowLevelOnPlatter(cv_nextmap.value-1, cv_newgametype.value))
			CV_SetValue(&cv_nextmap, M_GetFirstLevelInList(cv_newgametype.value));
	}
}

void Screenshot_option_Onchange(void)
{
	OP_ScreenshotOptionsMenu[op_screenshot_folder].status =
		(cv_screenshot_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

void Moviemode_mode_Onchange(void)
{
	INT32 i, cstart, cend;
	for (i = op_screenshot_gif_start; i <= op_screenshot_apng_end; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_DISABLED;

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			cstart = op_screenshot_gif_start;
			cend = op_screenshot_gif_end;
			break;
		case MM_APNG:
			cstart = op_screenshot_apng_start;
			cend = op_screenshot_apng_end;
			break;
		default:
			return;
	}
	for (i = cstart; i <= cend; ++i)
		OP_ScreenshotOptionsMenu[i].status = IT_STRING|IT_CVAR;
}

void Addons_option_Onchange(void)
{
	OP_AddonsOptionsMenu[op_addons_folder].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

void Moviemode_option_Onchange(void)
{
	OP_ScreenshotOptionsMenu[op_movie_folder].status =
		(cv_movie_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}

// ==========================================================================
// END ORGANIZATION STUFF.
// ==========================================================================

// current menudef
menu_t *currentMenu = &MainDef;

// =========================================================================
// MENU PRESENTATION PARAMETER HANDLING (BACKGROUNDS)
// =========================================================================

// menu IDs are equal to current/prevMenu in most cases, except MN_SPECIAL when we don't want to operate on Message, Pause, etc.
UINT32 prevMenuId = 0;
UINT32 activeMenuId = 0;

menupres_t menupres[NUMMENUTYPES];

void M_InitMenuPresTables(void)
{
	INT32 i;

	// Called in d_main before SOC can get to the tables
	// Set menupres defaults
	for (i = 0; i < NUMMENUTYPES; i++)
	{
		// so-called "undefined"
		menupres[i].fadestrength = -1;
		menupres[i].hidetitlepics = -1; // inherits global hidetitlepics
		menupres[i].ttmode = TTMODE_NONE;
		menupres[i].ttscale = UINT8_MAX;
		menupres[i].ttname[0] = 0;
		menupres[i].ttx = INT16_MAX;
		menupres[i].tty = INT16_MAX;
		menupres[i].ttloop = INT16_MAX;
		menupres[i].tttics = UINT16_MAX;
		menupres[i].enterwipe = -1;
		menupres[i].exitwipe = -1;
		menupres[i].bgcolor = -1;
		menupres[i].titlescrollxspeed = INT32_MAX;
		menupres[i].titlescrollyspeed = INT32_MAX;
		menupres[i].bghide = true;
		// default true
		menupres[i].enterbubble = true;
		menupres[i].exitbubble = true;

		if (i != MN_MAIN)
		{
			menupres[i].muslooping = true;
		}
		if (i == MN_SP_TIMEATTACK)
			strncpy(menupres[i].musname, "_recat", 7);
		else if (i == MN_SP_NIGHTSATTACK)
			strncpy(menupres[i].musname, "_nitat", 7);
		else if (i == MN_SP_MARATHON)
			strncpy(menupres[i].musname, "spec8", 6);
		else if (i == MN_SP_PLAYER || i == MN_SR_PLAYER)
			strncpy(menupres[i].musname, "_chsel", 7);
		else if (i == MN_SR_SOUNDTEST)
		{
			*menupres[i].musname = '\0';
			menupres[i].musstop = true;
		}
	}
}

// ====================================
// TREE ITERATION
// ====================================

// UINT32 menutype - current menutype_t
// INT32 level - current level up the tree, higher means younger
// INT32 *retval - Return value
// void *input - Pointer to input of any type
//
// return true - stop iterating
// return false - continue
typedef boolean (*menutree_iterator)(UINT32, INT32, INT32 *, void **, boolean fromoldest);

// HACK: Used in the ChangeMusic iterator because we only allow
// a single input. Maybe someday use this struct program-wide.
typedef struct
{
	char musname[7];
	UINT16 mustrack;
	boolean muslooping;
} menupresmusic_t;

static INT32 M_IterateMenuTree(menutree_iterator itfunc, void *input)
{
	INT32 i, retval = 0;
	UINT32 bitmask, menutype;

	for (i = NUMMENULEVELS; i >= 0; i--)
	{
		bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
		menutype = (activeMenuId & bitmask) >> (MENUBITS*i);
		if (itfunc(menutype, i, &retval, &input, false))
			break;
	}

	return retval;
}

// ====================================
// ITERATORS
// ====================================

static boolean MIT_GetMenuAtLevel(UINT32 menutype, INT32 level, INT32 *retval, void **input, boolean fromoldest)
{
	INT32 *inputptr = (INT32*)*input;
	INT32 targetlevel = *inputptr;
	if (menutype)
	{
		if (level == targetlevel || targetlevel < 0)
		{
			*retval = menutype;
			return true;
		}
	}
	else if (targetlevel >= 0)
	{
		// offset targetlevel by failed attempts; this should only happen in beginning of iteration
		if (fromoldest)
			(*inputptr)++;
		else
			(*inputptr)--; // iterating backwards, so count from highest
	}
	return false;
}

static boolean MIT_SetCurBackground(UINT32 menutype, INT32 level, INT32 *retval, void **input, boolean fromoldest)
{
	char *defaultname = (char*)*input;

	(void)retval;
	(void)fromoldest;

	if (!menutype) // if there's nothing in this level, do nothing
		return false;

	if (menupres[menutype].bgcolor >= 0)
	{
		curbgcolor = menupres[menutype].bgcolor;
		return true;
	}
	else if (menupres[menutype].bghide && titlemapinaction) // hide the background
	{
		curbghide = true;
		return true;
	}
	else if (menupres[menutype].bgname[0])
	{
		strncpy(curbgname, menupres[menutype].bgname, 8);
		curbgxspeed = menupres[menutype].titlescrollxspeed != INT32_MAX ? menupres[menutype].titlescrollxspeed : titlescrollxspeed;
		curbgyspeed = menupres[menutype].titlescrollyspeed != INT32_MAX ? menupres[menutype].titlescrollyspeed : titlescrollyspeed;
		return true;
	}
	else if (!level)
	{
		if (M_GetYoungestChildMenu() == MN_SP_PLAYER || !defaultname || !defaultname[0])
			curbgcolor = 31;
		else if (titlemapinaction) // hide the background by default in titlemap
			curbghide = true;
		else
		{
			strncpy(curbgname, defaultname, 9);
			curbgxspeed = (gamestate == GS_TIMEATTACK) ? 0 : titlescrollxspeed;
			curbgyspeed = (gamestate == GS_TIMEATTACK) ? 0 : titlescrollyspeed;
		}
	}
	return false;
}

static boolean MIT_ChangeMusic(UINT32 menutype, INT32 level, INT32 *retval, void **input, boolean fromoldest)
{
	menupresmusic_t *defaultmusic = (menupresmusic_t*)*input;

	(void)retval;
	(void)fromoldest;

	if (!menutype) // if there's nothing in this level, do nothing
		return false;

	if (menupres[menutype].musname[0])
	{
		S_ChangeMusic(menupres[menutype].musname, menupres[menutype].mustrack, menupres[menutype].muslooping);
		return true;
	}
	else if (menupres[menutype].musstop)
	{
		S_StopMusic();
		return true;
	}
	else if (menupres[menutype].musignore)
		return true;
	else if (!level && defaultmusic && defaultmusic->musname[0])
		S_ChangeMusic(defaultmusic->musname, defaultmusic->mustrack, defaultmusic->muslooping);
	return false;
}

static boolean MIT_SetCurFadeValue(UINT32 menutype, INT32 level, INT32 *retval, void **input, boolean fromoldest)
{
	UINT8 defaultvalue = *(UINT8*)*input;

	(void)retval;
	(void)fromoldest;

	if (!menutype) // if there's nothing in this level, do nothing
		return false;

	if (menupres[menutype].fadestrength >= 0)
	{
		curfadevalue = (menupres[menutype].fadestrength % 32);
		return true;
	}
	else if (!level)
		curfadevalue = (gamestate == GS_TIMEATTACK) ? 0 : (defaultvalue % 32);
	return false;
}

static boolean MIT_SetCurTitlePics(UINT32 menutype, INT32 level, INT32 *retval, void **input, boolean fromoldest)
{
	(void)input;
	(void)retval;
	(void)fromoldest;

	if (!menutype) // if there's nothing in this level, do nothing
		return false;

	if (menupres[menutype].hidetitlepics >= 0)
	{
		curhidepics = menupres[menutype].hidetitlepics;
		return true;
	}
	else if (menupres[menutype].ttmode == TTMODE_USER)
	{
		if (menupres[menutype].ttname[0])
		{
			curhidepics = menupres[menutype].hidetitlepics;
			curttmode = menupres[menutype].ttmode;
			curttscale = (menupres[menutype].ttscale != UINT8_MAX ? menupres[menutype].ttscale : ttscale);
			strncpy(curttname, menupres[menutype].ttname, 9);
			curttx = (menupres[menutype].ttx != INT16_MAX ? menupres[menutype].ttx : ttx);
			curtty = (menupres[menutype].tty != INT16_MAX ? menupres[menutype].tty : tty);
			curttloop = (menupres[menutype].ttloop != INT16_MAX ? menupres[menutype].ttloop : ttloop);
			curtttics = (menupres[menutype].tttics != UINT16_MAX ? menupres[menutype].tttics : tttics);
		}
		else
			curhidepics = menupres[menutype].hidetitlepics;
		return true;
	}
	else if (menupres[menutype].ttmode != TTMODE_NONE)
	{
		curhidepics = menupres[menutype].hidetitlepics;
		curttmode = menupres[menutype].ttmode;
		curttscale = (menupres[menutype].ttscale != UINT8_MAX ? menupres[menutype].ttscale : ttscale);
		return true;
	}
	else if (!level)
	{
		curhidepics = hidetitlepics;
		curttmode = ttmode;
		curttscale = ttscale;
		strncpy(curttname, ttname, 9);
		curttx = ttx;
		curtty = tty;
		curttloop = ttloop;
		curtttics = tttics;
	}
	return false;
}

// ====================================
// TREE RETRIEVAL
// ====================================

UINT8 M_GetYoungestChildMenu(void) // aka the active menu
{
	INT32 targetlevel = -1;
	return M_IterateMenuTree(MIT_GetMenuAtLevel, &targetlevel);
}

// ====================================
// EFFECTS
// ====================================

void M_ChangeMenuMusic(const char *defaultmusname, boolean defaultmuslooping)
{
	menupresmusic_t defaultmusic;

	if (!defaultmusname)
		defaultmusname = "";

	strncpy(defaultmusic.musname, defaultmusname, 7);
	defaultmusic.musname[6] = 0;
	defaultmusic.mustrack = 0;
	defaultmusic.muslooping = defaultmuslooping;

	M_IterateMenuTree(MIT_ChangeMusic, &defaultmusic);
}

void M_SetMenuCurBackground(const char *defaultname)
{
	char name[9];
	strncpy(name, defaultname, 8);
	M_IterateMenuTree(MIT_SetCurBackground, &name);
}

void M_SetMenuCurFadeValue(UINT8 defaultvalue)
{
	M_IterateMenuTree(MIT_SetCurFadeValue, &defaultvalue);
}

void M_SetMenuCurTitlePics(void)
{
	M_IterateMenuTree(MIT_SetCurTitlePics, NULL);
}

// ====================================
// MENU STATE
// ====================================

static INT32 exitlevel, enterlevel, anceslevel;
static INT16 exittype, entertype;
static INT16 exitwipe, enterwipe;
static boolean exitbubble, enterbubble;
static INT16 exittag, entertag;

static void M_HandleMenuPresState(menu_t *newMenu)
{
	INT32 i;
	UINT32 bitmask;
	SINT8 prevtype, activetype, menutype;

	if (!newMenu)
		return;

	// Look for MN_SPECIAL here, because our iterators can't look at new menu IDs
	for (i = 0; i <= NUMMENULEVELS; i++)
	{
		bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
		menutype = (newMenu->menuid & bitmask) >> (MENUBITS*i);
		prevtype = (currentMenu->menuid & bitmask) >> (MENUBITS*i);
		if (menutype == MN_SPECIAL || prevtype == MN_SPECIAL)
			return;
	}

	if (currentMenu && newMenu && currentMenu->menuid == newMenu->menuid) // same menu?
		return;

	exittype = entertype = exitlevel = enterlevel = anceslevel = exitwipe = enterwipe = -1;
	exitbubble = enterbubble = true;

	prevMenuId = currentMenu ? currentMenu->menuid : 0;
	activeMenuId = newMenu ? newMenu->menuid : 0;

	// Set defaults for presentation values
	strncpy(curbgname, "TITLESKY", 9);
	curfadevalue = 16;
	curhidepics = hidetitlepics;
	curbgcolor = -1;
	curbgxspeed = titlescrollxspeed;
	curbgyspeed = titlescrollyspeed;
	curbghide = (gamestate != GS_TIMEATTACK); // show in time attack, hide in other menus

	curttmode = ttmode;
	curttscale = ttscale;
	strncpy(curttname, ttname, 9);
	curttx = ttx;
	curtty = tty;
	curttloop = ttloop;
	curtttics = tttics;

	// don't do the below during the in-game menus
	if (gamestate != GS_TITLESCREEN && gamestate != GS_TIMEATTACK)
		return;

	M_SetMenuCurFadeValue(16);
	M_SetMenuCurTitlePics();

	// Loop through both menu IDs in parallel and look for type changes
	// The youngest child in activeMenuId is the entered menu
	// The youngest child in prevMenuId is the exited menu

	// 0. Get the type and level of each menu, and level of common ancestor
	// 1. Get the wipes for both, then run the exit wipe
	// 2. Change music (so that execs can change it again later)
	// 3. Run each exit exec on the prevMenuId up to the common ancestor (UNLESS NoBubbleExecs)
	// 4. Run each entrance exec on the activeMenuId down from the common ancestor (UNLESS NoBubbleExecs)
	// 5. Run the entrance wipe

	// Get the parameters for each menu
	for (i = NUMMENULEVELS; i >= 0; i--)
	{
		bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
		prevtype = (prevMenuId & bitmask) >> (MENUBITS*i);
		activetype = (activeMenuId & bitmask) >> (MENUBITS*i);

		if (prevtype && (exittype < 0))
		{
			exittype = prevtype;
			exitlevel = i;
			exitwipe = menupres[exittype].exitwipe;
			exitbubble = menupres[exittype].exitbubble;
			exittag = menupres[exittype].exittag;
		}

		if (activetype && (entertype < 0))
		{
			entertype = activetype;
			enterlevel = i;
			enterwipe = menupres[entertype].enterwipe;
			enterbubble = menupres[entertype].enterbubble;
			entertag = menupres[entertype].entertag;
		}

		if (prevtype && activetype && prevtype == activetype && anceslevel < 0)
		{
			anceslevel = i;
			break;
		}
	}

	// if no common ancestor (top menu), force a wipe. Look for a specified wipe first.
	// Don't force a wipe if you're actually going to/from the main menu
	if (anceslevel < 0 && exitwipe < 0 && newMenu != &MainDef && currentMenu != &MainDef)
	{
		for (i = NUMMENULEVELS; i >= 0; i--)
		{
			bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
			prevtype = (prevMenuId & bitmask) >> (MENUBITS*i);

			if (menupres[prevtype].exitwipe >= 0)
			{
				exitwipe = menupres[prevtype].exitwipe;
				break;
			}
		}

		if (exitwipe < 0)
			exitwipe = menupres[MN_MAIN].exitwipe;
	}

	// do the same for enter wipe
	if (anceslevel < 0 && enterwipe < 0 && newMenu != &MainDef && currentMenu != &MainDef)
	{
		for (i = NUMMENULEVELS; i >= 0; i--)
		{
			bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
			activetype = (activeMenuId & bitmask) >> (MENUBITS*i);

			if (menupres[activetype].enterwipe >= 0)
			{
				exitwipe = menupres[activetype].enterwipe;
				break;
			}
		}

		if (enterwipe < 0)
			enterwipe = menupres[MN_MAIN].enterwipe;
	}

	// Change the music
	M_ChangeMenuMusic("_title", false);

	// Run the linedef execs
	if (titlemapinaction)
	{
		// Run the exit tags
		if (enterlevel <= exitlevel) // equals is an edge case
		{
			if (exitbubble)
			{
				for (i = exitlevel; i > anceslevel; i--) // don't run the common ancestor's exit tag
				{
					bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
					menutype = (prevMenuId & bitmask) >> (MENUBITS*i);
					if (menupres[menutype].exittag)
						P_LinedefExecute(menupres[menutype].exittag, players[displayplayer].mo, NULL);
				}
			}
			else if (exittag)
				P_LinedefExecute(exittag, players[displayplayer].mo, NULL);
		}

		// Run the enter tags
		if (enterlevel >= exitlevel) // equals is an edge case
		{
			if (enterbubble)
			{
				for (i = anceslevel+1; i <= enterlevel; i++) // don't run the common ancestor's enter tag
				{
					bitmask = ((1 << MENUBITS) - 1) << (MENUBITS*i);
					menutype = (activeMenuId & bitmask) >> (MENUBITS*i);
					if (menupres[menutype].entertag)
						P_LinedefExecute(menupres[menutype].entertag, players[displayplayer].mo, NULL);
				}
			}
			else if (entertag)
				P_LinedefExecute(entertag, players[displayplayer].mo, NULL);
		}
	}


	// Set the wipes for next frame
	if (
		(exitwipe >= 0 && enterlevel <= exitlevel) ||
		(enterwipe >= 0 && enterlevel >= exitlevel) ||
		(anceslevel < 0 && newMenu != &MainDef && currentMenu != &MainDef)
	)
	{
		if (gamestate == GS_TIMEATTACK)
			wipetypepre = ((exitwipe && enterlevel <= exitlevel) || anceslevel < 0) ? exitwipe : -1; // force default
		else
			// HACK: INT16_MAX signals to not wipe
			// because 0 is a valid index and -1 means default
			wipetypepre = ((exitwipe && enterlevel <= exitlevel) || anceslevel < 0) ? exitwipe : INT16_MAX;
		wipetypepost = ((enterwipe && enterlevel >= exitlevel) || anceslevel < 0) ? enterwipe : INT16_MAX;
		wipegamestate = FORCEWIPE;

		// If just one of the above is a force not-wipe,
		// mirror the other wipe.
		if (wipetypepre != INT16_MAX && wipetypepost == INT16_MAX)
			wipetypepost = wipetypepre;
		else if (wipetypepost != INT16_MAX && wipetypepre == INT16_MAX)
			wipetypepre = wipetypepost;

		// D_Display runs the next step of processing
	}
}

// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

static void M_GoBack(INT32 choice)
{
	(void)choice;

	if (currentMenu->prevMenu)
	{
		//If we entered the game search menu, but didn't enter a game,
		//make sure the game doesn't still think we're in a netgame.
		if (!Playing() && netgame && multiplayer)
		{
			netgame = multiplayer = false;
		}

		if ((currentMenu->prevMenu == &MainDef) && (currentMenu == &SP_TimeAttackDef || currentMenu == &SP_NightsAttackDef || currentMenu == &SP_MarathonDef))
		{
			// D_StartTitle does its own wipe, since GS_TIMEATTACK is now a complete gamestate.

			if (levelselect.rows)
			{
				Z_Free(levelselect.rows);
				levelselect.rows = NULL;
			}

			menuactive = false;
			wipetypepre = menupres[M_GetYoungestChildMenu()].exitwipe;
			I_UpdateMouseGrab();
			D_StartTitle();
		}
		else
			M_SetupNextMenu(currentMenu->prevMenu);
	}
	else
		M_ClearMenus(true);
}

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

	if (choice == -1)
	{
		if (cv == &cv_playercolor)
		{
			SINT8 skinno = R_SkinAvailable(cv_chooseskin.string);
			if (skinno != -1)
				CV_SetValue(cv,skins[skinno].prefcolor);
			return;
		}
		CV_Set(cv,cv->defaultvalue);
		return;
	}

	choice = (choice<<1) - 1;

	if (cv->flags & CV_FLOAT)
	{
		if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
			||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_INVISSLIDER)
			||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD)
			|| !(currentMenu->menuitems[itemOn].status & IT_CV_INTEGERSTEP))
		{
			char s[20];
			float n = FIXED_TO_FLOAT(cv->value)+(choice)*(1.0f/16.0f);
			sprintf(s,"%ld%s",(long)n,M_Ftrim(n));
			CV_Set(cv,s);
		}
		else
			CV_SetValue(cv,FIXED_TO_FLOAT(cv->value)+(choice));
	}
	else
		CV_AddValue(cv,choice);
}

static boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
	char buf[MAXSTRINGLENGTH];
	size_t len;

	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				M_Memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					M_Memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}
	return false;
}

// resets all cvars on a menu - assumes that all that have itemactions are cvars
static void M_ResetCvars(void)
{
	INT32 i;
	consvar_t *cv;
	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (!(currentMenu->menuitems[i].status & IT_CVAR) || !(cv = (consvar_t *)currentMenu->menuitems[i].itemaction))
			continue;
		CV_SetValue(cv, atoi(cv->defaultvalue));
	}
}

static void M_NextOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop
	do
	{
		if (itemOn + 1 > currentMenu->numitems - 1)
			itemOn = 0;
		else
			itemOn++;
	} while (oldItemOn != itemOn && ( (currentMenu->menuitems[itemOn].status & IT_TYPE) & IT_SPACE ));
}

static void M_PrevOpt(void)
{
	INT16 oldItemOn = itemOn; // prevent infinite loop
	do
	{
		if (!itemOn)
			itemOn = currentMenu->numitems - 1;
		else
			itemOn--;
	} while (oldItemOn != itemOn && ( (currentMenu->menuitems[itemOn].status & IT_TYPE) & IT_SPACE ));
}

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

static void Command_Manual_f(void)
{
	if (modeattacking)
		return;
	M_StartControlPanel();
	currentMenu = &MISC_HelpDef;
	itemOn = 0;
}

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	INT32 ch = -1;
//	INT32 i;
	static tic_t joywait = 0, mousewait = 0;
	static INT32 pjoyx = 0, pjoyy = 0;
	static INT32 pmousex = 0, pmousey = 0;
	static INT32 lastx = 0, lasty = 0;
	void (*routine)(INT32 choice); // for some casting problem

	if (dedicated || (demoplayback && titledemo)
	|| gamestate == GS_INTRO || gamestate == GS_ENDING || gamestate == GS_CUTSCENE
	|| gamestate == GS_CREDITS || gamestate == GS_EVALUATION || gamestate == GS_GAMEEND)
		return false;

	if (gamestate == GS_TITLESCREEN && finalecount < TICRATE)
		return false;

	if (CON_Ready())
		return false;

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}
	else if (menuactive)
	{
		if (ev->type == ev_keydown)
		{
			keydown++;
			ch = ev->data1;

			// added 5-2-98 remap virtual keys (mouse & joystick buttons)
			switch (ch)
			{
				case KEY_MOUSE1:
				case KEY_JOY1:
					ch = KEY_ENTER;
					break;
				case KEY_JOY1 + 3:
					ch = 'n';
					break;
				case KEY_MOUSE1 + 1:
				case KEY_JOY1 + 1:
					ch = KEY_ESCAPE;
					break;
				case KEY_JOY1 + 2:
					ch = KEY_BACKSPACE;
					break;
				case KEY_HAT1:
					ch = KEY_UPARROW;
					break;
				case KEY_HAT1 + 1:
					ch = KEY_DOWNARROW;
					break;
				case KEY_HAT1 + 2:
					ch = KEY_LEFTARROW;
					break;
				case KEY_HAT1 + 3:
					ch = KEY_RIGHTARROW;
					break;
			}
		}
		else if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			const INT32 jdeadzone = (JOYAXISRANGE * cv_digitaldeadzone.value) / FRACUNIT;
			if (ev->data3 != INT32_MAX)
			{
				if (Joystick.bGamepadStyle || abs(ev->data3) > jdeadzone)
				{
					if (ev->data3 < 0 && pjoyy >= 0)
					{
						ch = KEY_UPARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					else if (ev->data3 > 0 && pjoyy <= 0)
					{
						ch = KEY_DOWNARROW;
						joywait = I_GetTime() + NEWTICRATE/7;
					}
					pjoyy = ev->data3;
				}
				else
					pjoyy = 0;
			}

			if (ev->data2 != INT32_MAX)
			{
				if (Joystick.bGamepadStyle || abs(ev->data2) > jdeadzone)
				{
					if (ev->data2 < 0 && pjoyx >= 0)
					{
						ch = KEY_LEFTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					else if (ev->data2 > 0 && pjoyx <= 0)
					{
						ch = KEY_RIGHTARROW;
						joywait = I_GetTime() + NEWTICRATE/17;
					}
					pjoyx = ev->data2;
				}
				else
					pjoyx = 0;
			}
		}
		else if (ev->type == ev_mouse && mousewait < I_GetTime())
		{
			pmousey += ev->data3;
			if (pmousey < lasty-30)
			{
				ch = KEY_DOWNARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty -= 30;
			}
			else if (pmousey > lasty + 30)
			{
				ch = KEY_UPARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousey = lasty += 30;
			}

			pmousex += ev->data2;
			if (pmousex < lastx - 30)
			{
				ch = KEY_LEFTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx -= 30;
			}
			else if (pmousex > lastx+30)
			{
				ch = KEY_RIGHTARROW;
				mousewait = I_GetTime() + NEWTICRATE/7;
				pmousex = lastx += 30;
			}
		}
		else if (ev->type == ev_keyup) // Preserve event for other responders
			keydown = 0;
	}
	else if (ev->type == ev_keydown) // Preserve event for other responders
		ch = ev->data1;

	if (ch == -1)
		return false;
	else if (ch == gamecontrol[gc_systemmenu][0] || ch == gamecontrol[gc_systemmenu][1]) // allow remappable ESC key
		ch = KEY_ESCAPE;

	// F-Keys
	if (!menuactive)
	{
		noFurtherInput = true;
		switch (ch)
		{
			case KEY_F1: // Help key
				Command_Manual_f();
				return true;

			case KEY_F2: // Empty
				return true;

			case KEY_F3: // Toggle HUD
				CV_SetValue(&cv_showhud, !cv_showhud.value);
				return true;

			case KEY_F4: // Sound Volume
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				// Uncomment the below if you want the menu to reset to the top each time like before. M_SetupNextMenu will fix it automatically.
				//OP_SoundOptionsDef.lastOn = 0;
				M_SetupNextMenu(&OP_SoundOptionsDef);
				return true;

			case KEY_F5: // Video Mode
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_VideoModeMenu(0);
				return true;

			case KEY_F6: // Empty
				return true;

			case KEY_F7: // Options
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_SetupNextMenu(&OP_MainDef);
				return true;

			// Screenshots on F8 now handled elsewhere
			// Same with Moviemode on F9

			case KEY_F10: // Quit SRB2
				M_QuitSRB2(0);
				return true;

			case KEY_F11: // Gamma Level
				CV_AddValue(&cv_globalgamma, 1);
				return true;

			// Spymode on F12 handled in game logic

			case KEY_ESCAPE: // Pop up menu
				if (chat_on)
					HU_clearChatChars();
				else
					M_StartControlPanel();
				return true;
		}
		noFurtherInput = false; // turns out we didn't care
		return false;
	}

	routine = currentMenu->menuitems[itemOn].itemaction;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		if (shiftdown && ch >= 32 && ch <= 127)
			ch = shiftxform[ch];
		routine(ch);
		return true;
	}

	if (currentMenu->menuitems[itemOn].status == IT_MSGHANDLER)
	{
		if (currentMenu->menuitems[itemOn].alphaKey != MM_EVENTHANDLER)
		{
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER || ch == KEY_DEL)
			{
				if (routine)
					routine(ch);
				if (stopstopmessage)
					stopstopmessage = false;
				else
					M_StopMessage(0);
				noFurtherInput = true;
				return true;
			}
			return true;
		}
		else
		{
			// dirty hack: for customising controls, I want only buttons/keys, not moves
			if (ev->type == ev_mouse || ev->type == ev_mouse2 || ev->type == ev_joystick
				|| ev->type == ev_joystick2)
				return true;
			if (routine)
			{
				void (*otherroutine)(event_t *sev) = currentMenu->menuitems[itemOn].itemaction;
				otherroutine(ev); //Alam: what a hack
			}
			return true;
		}
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING)
		{
			if (M_ChangeStringCvar(ch))
				return true;
			else
				routine = NULL;
		}
		else
			routine = M_ChangeCvar;
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_menu1);
			return true;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_menu1);
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				S_StartSound(NULL, sfx_menu1);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				S_StartSound(NULL, sfx_menu1);
				routine(1);
			}
			return true;

		case KEY_ENTER:
			noFurtherInput = true;
			currentMenu->lastOn = itemOn;
			if (routine)
			{
				if (((currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_CALL
				 || (currentMenu->menuitems[itemOn].status & IT_TYPE)==IT_SUBMENU)
                 && (currentMenu->menuitems[itemOn].status & IT_CALLTYPE))
				{
#ifndef DEVELOP
					if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && modifiedgame && !savemoddata)
					{
						S_StartSound(NULL, sfx_skid);
						M_StartMessage(M_GetText("This cannot be done in a modified game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
						return true;
					}
#endif
				}
				S_StartSound(NULL, sfx_menu1);
				switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
				{
					case IT_CVAR:
					case IT_ARROWS:
						routine(1); // right arrow
						break;
					case IT_CALL:
						routine(itemOn);
						break;
					case IT_SUBMENU:
						currentMenu->lastOn = itemOn;
						M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction);
						break;
				}
			}
			return true;

		case KEY_ESCAPE:
			noFurtherInput = true;
			currentMenu->lastOn = itemOn;

			M_GoBack(0);

			return true;

		case KEY_BACKSPACE:
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].alphaKey);
				S_StartSound(NULL, sfx_shldls);
				return true;
			}

			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

				if (cv == &cv_chooseskin
					|| cv == &cv_nextmap
					|| cv == &cv_newgametype)
					return true;

				if (currentMenu != &OP_SoundOptionsDef || itemOn > 3)
					S_StartSound(NULL, sfx_menu1);
				routine(-1);
				return true;
			}

			// Why _does_ backspace go back anyway?
			//currentMenu->lastOn = itemOn;
			//if (currentMenu->prevMenu)
			//	M_SetupNextMenu(currentMenu->prevMenu);
			return false;

		default:
			CON_Responder(ev);
			break;
	}

	return true;
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	boolean wipe = WipeInAction;

	if (currentMenu == &MessageDef)
		menuactive = true;

	if (menuactive)
	{
		// now that's more readable with a faded background (yeah like Quake...)
		if (!wipe && (curfadevalue || (gamestate != GS_TITLESCREEN && gamestate != GS_TIMEATTACK)))
			V_DrawFadeScreen(0xFF00, (gamestate != GS_TITLESCREEN && gamestate != GS_TIMEATTACK) ? 16 : curfadevalue);

		if (currentMenu->drawroutine)
			currentMenu->drawroutine(); // call current menu Draw routine

		// Draw version down in corner
		// ... but only in the MAIN MENU.  I'm a picky bastard.
		if (currentMenu == &MainDef)
		{
			if (customversionstring[0] != '\0')
			{
				V_DrawThinString(vid.dupx, vid.height - 17*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT, "Mod version:");
				V_DrawThinString(vid.dupx, vid.height - 9*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, customversionstring);
			}
			else
			{
#ifdef DEVELOP // Development -- show revision / branch info
				V_DrawThinString(vid.dupx, vid.height - 17*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, compbranch);
				V_DrawThinString(vid.dupx, vid.height - 9*vid.dupy,  V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, comprevision);
#else // Regular build
				V_DrawThinString(vid.dupx, vid.height - 9*vid.dupy, V_NOSCALESTART|V_TRANSLUCENT|V_ALLOWLOWERCASE, va("%s", VERSIONSTRING));
#endif
			}
		}
	}

	// focus lost notification goes on top of everything, even the former everything
	if (window_notinfocus && cv_showfocuslost.value)
	{
		M_DrawTextBox((BASEVIDWIDTH/2) - (60), (BASEVIDHEIGHT/2) - (16), 13, 2);
		if (gamestate == GS_LEVEL && (P_AutoPause() || paused))
			V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), V_YELLOWMAP, "Game Paused");
		else
			V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2) - (4), V_YELLOWMAP, "Focus Lost");
	}
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	// time attack HACK
	if (modeattacking && demoplayback)
	{
		G_CheckDemoStatus();
		return;
	}

	// intro might call this repeatedly
	if (menuactive)
	{
		CON_ToggleOff(); // move away console
		return;
	}

	menuactive = true;

	if (!Playing())
	{
		// Secret menu!
		MainMenu[singleplr].alphaKey = (M_AnySecretUnlocked()) ? 76 : 84;
		MainMenu[multiplr].alphaKey = (M_AnySecretUnlocked()) ? 84 : 92;
		MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		currentMenu = &MainDef;
		itemOn = singleplr;
	}
	else if (modeattacking)
	{
		currentMenu = &MAPauseDef;
		MAPauseMenu[mapause_hints].status = (M_SecretUnlocked(SECRET_EMBLEMHINTS)) ? (IT_STRING | IT_CALL) : (IT_DISABLED);
		itemOn = mapause_continue;
	}
	else if (!(netgame || multiplayer)) // Single Player
	{
		if (gamestate != GS_LEVEL || ultimatemode) // intermission, so gray out stuff.
		{
			SPauseMenu[spause_pandora].status = (M_SecretUnlocked(SECRET_PANDORA)) ? (IT_GRAYEDOUT) : (IT_DISABLED);
			SPauseMenu[spause_retry].status = IT_GRAYEDOUT;
		}
		else
		{
			INT32 numlives = 2;

			SPauseMenu[spause_pandora].status = (M_SecretUnlocked(SECRET_PANDORA) && !marathonmode) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

			if (&players[consoleplayer])
			{
				numlives = players[consoleplayer].lives;
				if (players[consoleplayer].playerstate != PST_LIVE)
					++numlives;
			}

			// The list of things that can disable retrying is (was?) a little too complex
			// for me to want to use the short if statement syntax
			if (numlives <= 1 || G_IsSpecialStage(gamemap))
				SPauseMenu[spause_retry].status = (IT_GRAYEDOUT);
			else
				SPauseMenu[spause_retry].status = (IT_STRING | IT_CALL);
		}

		// We can always use level select though. :33
		SPauseMenu[spause_levelselect].status = (gamecomplete == 1) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		// And emblem hints.
		SPauseMenu[spause_hints].status = (M_SecretUnlocked(SECRET_EMBLEMHINTS) && !marathonmode) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		// Shift up Pandora's Box if both pandora and levelselect are active
		/*if (SPauseMenu[spause_pandora].status != (IT_DISABLED)
		 && SPauseMenu[spause_levelselect].status != (IT_DISABLED))
			SPauseMenu[spause_pandora].alphaKey = 24;
		else
			SPauseMenu[spause_pandora].alphaKey = 32;*/

		currentMenu = &SPauseDef;
		itemOn = spause_continue;
	}
	else // multiplayer
	{
		MPauseMenu[mpause_switchmap].status = IT_DISABLED;
		MPauseMenu[mpause_addons].status = IT_DISABLED;
		MPauseMenu[mpause_scramble].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit].status = IT_DISABLED;
		MPauseMenu[mpause_psetupsplit2].status = IT_DISABLED;
		MPauseMenu[mpause_spectate].status = IT_DISABLED;
		MPauseMenu[mpause_entergame].status = IT_DISABLED;
		MPauseMenu[mpause_switchteam].status = IT_DISABLED;
		MPauseMenu[mpause_psetup].status = IT_DISABLED;

		if ((server || IsPlayerAdmin(consoleplayer)))
		{
			MPauseMenu[mpause_switchmap].status = IT_STRING | IT_CALL;
			MPauseMenu[mpause_addons].status = IT_STRING | IT_CALL;
			if (G_GametypeHasTeams())
				MPauseMenu[mpause_scramble].status = IT_STRING | IT_SUBMENU;
		}

		if (splitscreen)
		{
			MPauseMenu[mpause_psetupsplit].status = MPauseMenu[mpause_psetupsplit2].status = IT_STRING | IT_CALL;
			MPauseMenu[mpause_psetup].text = "Player 1 Setup";
		}
		else
		{
			MPauseMenu[mpause_psetup].status = IT_STRING | IT_CALL;
			MPauseMenu[mpause_psetup].text = "Player Setup";

			if (G_GametypeHasTeams())
				MPauseMenu[mpause_switchteam].status = IT_STRING | IT_SUBMENU;
			else if (G_GametypeHasSpectators())
				MPauseMenu[((&players[consoleplayer] && players[consoleplayer].spectator) ? mpause_entergame : mpause_spectate)].status = IT_STRING | IT_CALL;
			else // in this odd case, we still want something to be on the menu even if it's useless
				MPauseMenu[mpause_spectate].status = IT_GRAYEDOUT;
		}

		currentMenu = &MPauseDef;
		itemOn = mpause_continue;
	}

	CON_ToggleOff(); // move away console
}

void M_EndModeAttackRun(void)
{
	G_ClearModeAttackRetryFlag();
	M_ModeAttackEndGame(0);
}

//
// M_ClearMenus
//
void M_ClearMenus(boolean callexitmenufunc)
{
	if (!menuactive)
		return;

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

	// Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));

	if (currentMenu == &MessageDef) // Oh sod off!
		currentMenu = &MainDef; // Not like it matters
	menuactive = false;
	hidetitlemap = false;

	I_UpdateMouseGrab();
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
	INT16 i;

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	if (currentMenu == &MP_RoomDef || currentMenu == &MP_ConnectDef)
	{
		I_lock_mutex(&ms_QueryId_mutex);
		{
			ms_QueryId++;
		}
		I_unlock_mutex(ms_QueryId_mutex);
	}

	if (currentMenu == &MP_ConnectDef)
	{
		I_lock_mutex(&ms_ServerList_mutex);
		{
			if (ms_ServerList)
			{
				free(ms_ServerList);
				ms_ServerList = NULL;
			}
		}
		I_unlock_mutex(ms_ServerList_mutex);
	}
#endif/*HAVE_THREADS*/

	if (currentMenu->quitroutine)
	{
		// If you're going from a menu to itself, why are you running the quitroutine? You're not quitting it! -SH
		if (currentMenu != menudef && !currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}

	M_HandleMenuPresState(menudef);

	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = currentMenu->numitems - 1;

	// the curent item can be disabled,
	// this code go up until an enabled item found
	if (( (currentMenu->menuitems[itemOn].status & IT_TYPE) & IT_SPACE ))
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if (!( (currentMenu->menuitems[i].status & IT_TYPE) & IT_SPACE ))
			{
				itemOn = i;
				break;
			}
		}
	}

	hidetitlemap = false;
}

// Guess I'll put this here, idk
boolean M_MouseNeeded(void)
{
	return (currentMenu == &MessageDef && currentMenu->prevMenu == &OP_ChangeControlsDef);
}

//
// M_Ticker
//
void M_Ticker(void)
{
	// reset input trigger
	noFurtherInput = false;

	if (dedicated)
		return;

	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8;

	//added : 30-01-98 : test mode for five seconds
	if (vidm_testingmode > 0)
	{
		// restore the previous video mode
		if (--vidm_testingmode == 0)
			setmodeneeded = vidm_previousmode + 1;
	}

	if (currentMenu == &OP_ScreenshotOptionsDef)
		M_SetupScreenshotMenu();

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	I_lock_mutex(&ms_ServerList_mutex);
	{
		if (ms_ServerList)
		{
			CL_QueryServerList(ms_ServerList);
			free(ms_ServerList);
			ms_ServerList = NULL;
		}
	}
	I_unlock_mutex(ms_ServerList_mutex);
#endif
}

//
// M_Init
//
void M_Init(void)
{
	int i;

	COM_AddCommand("manual", Command_Manual_f);

	CV_RegisterVar(&cv_nextmap);
	CV_RegisterVar(&cv_newgametype);
	CV_RegisterVar(&cv_chooseskin);
	CV_RegisterVar(&cv_autorecord);

	if (dedicated)
		return;

	// Menu hacks
	CV_RegisterVar(&cv_dummyteam);
	CV_RegisterVar(&cv_dummyscramble);
	CV_RegisterVar(&cv_dummyrings);
	CV_RegisterVar(&cv_dummylives);
	CV_RegisterVar(&cv_dummycontinues);
	CV_RegisterVar(&cv_dummymares);
	CV_RegisterVar(&cv_dummymarathon);
	CV_RegisterVar(&cv_dummyloadless);
	CV_RegisterVar(&cv_dummycutscenes);

	quitmsg[QUITMSG] = M_GetText("Eggman's tied explosives\nto your girlfriend, and\nwill activate them if\nyou press the 'Y' key!\nPress 'N' to save her!\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG1] = M_GetText("What would Tails say if\nhe saw you quitting the game?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG2] = M_GetText("Hey!\nWhere do ya think you're goin'?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG3] = M_GetText("Forget your studies!\nPlay some more!\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG4] = M_GetText("You're trying to say you\nlike Sonic 2K6 better than\nthis, right?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG5] = M_GetText("Don't leave yet -- there's a\nsuper emerald around that corner!\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG6] = M_GetText("You'd rather work than play?\n\n(Press 'Y' to quit)");
	quitmsg[QUITMSG7] = M_GetText("Go ahead and leave. See if I care...\n*sniffle*\n\n(Press 'Y' to quit)");

	quitmsg[QUIT2MSG] = M_GetText("If you leave now,\nEggman will take over the world!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG1] = M_GetText("Don't quit!\nThere are animals\nto save!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG2] = M_GetText("Aw c'mon, just bop\na few more robots!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG3] = M_GetText("Did you get all those Chaos Emeralds?\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG4] = M_GetText("If you leave, I'll use\nmy spin attack on you!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG5] = M_GetText("Don't go!\nYou might find the hidden\nlevels!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT2MSG6] = M_GetText("Hit the 'N' key, Sonic!\nThe 'N' key!\n\n(Press 'Y' to quit)");

	quitmsg[QUIT3MSG] = M_GetText("Are you really going to give up?\nWe certainly would never give you up.\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG1] = M_GetText("Come on, just ONE more netgame!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG2] = M_GetText("Press 'N' to unlock\nthe Ultimate Cheat!\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG3] = M_GetText("Why don't you go back and try\njumping on that house to\nsee what happens?\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG4] = M_GetText("Every time you press 'Y', an\nSRB2 Developer cries...\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG5] = M_GetText("You'll be back to play soon, though...\n......right?\n\n(Press 'Y' to quit)");
	quitmsg[QUIT3MSG6] = M_GetText("Aww, is Egg Rock Zone too\ndifficult for you?\n\n(Press 'Y' to quit)");

	/*
	Well the menu sucks for forcing us to have an item set
	at all if every item just calls the same function, and
	nothing more. Now just automate the definition.
	*/
	for (i = 0; i <= MAX_JOYSTICKS; ++i)
	{
		OP_JoystickSetMenu[i].status = ( IT_NOTHING|IT_CALL );
		OP_JoystickSetMenu[i].itemaction = M_AssignJoystick;
	}

#ifndef NONET
	CV_RegisterVar(&cv_serversort);
#endif
}

void M_InitCharacterTables(void)
{
	UINT8 i;

	// Setup description table
	for (i = 0; i < MAXSKINS; i++)
	{
		description[i].used = false;
		strcpy(description[i].notes, "???");
		strcpy(description[i].picname, "");
		strcpy(description[i].nametag, "");
		strcpy(description[i].skinname, "");
		strcpy(description[i].displayname, "");
		description[i].prev = description[i].next = 0;
		description[i].charpic = NULL;
		description[i].namepic = NULL;
		description[i].oppositecolor = description[i].tagtextcolor = description[i].tagoutlinecolor = 0;
	}
}

// ==========================================================================
// SPECIAL MENU OPTION DRAW ROUTINES GO HERE
// ==========================================================================

// Converts a string into question marks.
// Used for the secrets menu, to hide yet-to-be-unlocked stuff.
static const char *M_CreateSecretMenuOption(const char *str)
{
	static char qbuf[32];
	int i;

	for (i = 0; i < 31; ++i)
	{
		if (!str[i])
		{
			qbuf[i] = '\0';
			return qbuf;
		}
		else if (str[i] != ' ')
			qbuf[i] = '?';
		else
			qbuf[i] = ' ';
	}

	qbuf[31] = '\0';
	return qbuf;
}

static void M_DrawThermo(INT32 x, INT32 y, consvar_t *cv)
{
	INT32 xx = x, i;
	lumpnum_t leftlump, rightlump, centerlump[2], cursorlump;
	patch_t *p;

	leftlump = W_GetNumForName("M_THERML");
	rightlump = W_GetNumForName("M_THERMR");
	centerlump[0] = W_GetNumForName("M_THERMM");
	centerlump[1] = W_GetNumForName("M_THERMM");
	cursorlump = W_GetNumForName("M_THERMO");

	V_DrawScaledPatch(xx, y, 0, p = W_CachePatchNum(leftlump,PU_PATCH));
	xx += p->width - p->leftoffset;
	for (i = 0; i < 16; i++)
	{
		V_DrawScaledPatch(xx, y, V_WRAPX, W_CachePatchNum(centerlump[i & 1], PU_PATCH));
		xx += 8;
	}
	V_DrawScaledPatch(xx, y, 0, W_CachePatchNum(rightlump, PU_PATCH));

	xx = (cv->value - cv->PossibleValue[0].value) * (15*8) /
		(cv->PossibleValue[1].value - cv->PossibleValue[0].value);

	V_DrawScaledPatch((x + 8) + xx, y, 0, W_CachePatchNum(cursorlump, PU_PATCH));
}

//  A smaller 'Thermo', with range given as percents (0-100)
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv, boolean ontop)
{
	INT32 i;
	INT32 range;
	patch_t *p;

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	V_DrawScaledPatch(x, y, 0, W_CachePatchName("M_SLIDEL", PU_PATCH));

	p =  W_CachePatchName("M_SLIDEM", PU_PATCH);
	for (i = 1; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	if (ontop)
	{
		V_DrawCharacter(x - 6 - (skullAnimCounter/5), y,
			'\x1C' | V_YELLOWMAP, false);
		V_DrawCharacter(x+i*8 + 8 + (skullAnimCounter/5), y,
			'\x1D' | V_YELLOWMAP, false);
	}

	p = W_CachePatchName("M_SLIDER", PU_PATCH);
	V_DrawScaledPatch(x+i*8, y, 0, p);

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_PATCH);

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	if (cv->flags & CV_FLOAT)
		range = (INT32)(atof(cv->defaultvalue)*FRACUNIT);
	else
		range = atoi(cv->defaultvalue);

	if (range != cv->value)
	{
		range = ((range - cv->PossibleValue[0].value) * 100 /
		 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

		if (range < 0)
			range = 0;
		else if (range > 100)
			range = 100;

		V_DrawMappedPatch(x + 2 + (SLIDER_RANGE*8*range)/100, y, V_TRANSLUCENT, p, yellowmap);
	}

	range = ((cv->value - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

	if (range < 0)
		range = 0;
	else if (range > 100)
		range = 100;

	V_DrawMappedPatch(x + 2 + (SLIDER_RANGE*8*range)/100, y, 0, p, yellowmap);
}

//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines)
{
	// Solid color textbox.
	V_DrawFill(x+5, y+5, width*8+6, boxlines*8+6, 159);
	//V_DrawFill(x+8, y+8, width*8, boxlines*8, 31);
/*
	patch_t *p;
	INT32 cx, cy, n;
	INT32 step, boff;

	step = 8;
	boff = 8;

	// draw left side
	cx = x;
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TL], PU_PATCH));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_L], PU_PATCH);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BL], PU_PATCH));

	// draw middle
	V_DrawFlatFill(x + boff, y + boff, width*step, boxlines*step, st_borderpatchnum);

	cx += boff;
	cy = y;
	while (width > 0)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_T], PU_PATCH));
		V_DrawScaledPatch(cx, y + boff + boxlines*step, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_B], PU_PATCH));
		width--;
		cx += step;
	}

	// draw right side
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TR], PU_PATCH));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_R], PU_PATCH);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BR], PU_PATCH));
*/
}

static fixed_t staticalong = 0;

static void M_DrawStaticBox(fixed_t x, fixed_t y, INT32 flags, fixed_t w, fixed_t h)
{
	patch_t *patch;
	fixed_t sw, pw;

	patch = W_CachePatchName("LSSTATIC", PU_PATCH);
	pw = patch->width - (sw = w*2); //FixedDiv(w, scale); -- for scale FRACUNIT/2

	/*if (pw > 0) -- model code for modders providing weird LSSTATIC
	{
		if (staticalong > pw)
			staticalong -= pw;
	}
	else
		staticalong = 0;*/

	if (staticalong > pw) // simplified for base LSSTATIC
		staticalong -= pw;

	V_DrawCroppedPatch(x<<FRACBITS, y<<FRACBITS, FRACUNIT/2, flags, patch, staticalong, 0, sw, h*2); // FixedDiv(h, scale)); -- for scale FRACUNIT/2

	staticalong += sw; //M_RandomRange(sw/2, 2*sw); -- turns out less randomisation looks better because immediately adjacent frames can't end up close to each other

	W_UnlockCachedPatch(patch);
}

//
// Draw border for the savegame description
//
#if 0 // once used for joysticks and savegames, now no longer
static void M_DrawSaveLoadBorder(INT32 x,INT32 y)
{
	INT32 i;

	V_DrawScaledPatch (x-8,y+7,0,W_CachePatchName("M_LSLEFT",PU_PATCH));

	for (i = 0;i < 24;i++)
	{
		V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSCNTR",PU_PATCH));
		x += 8;
	}

	V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSRGHT",PU_PATCH));
}
#endif

// horizontally centered text
static void M_CentreText(INT32 y, const char *string)
{
	INT32 x;
	//added : 02-02-98 : centre on 320, because V_DrawString centers on vid.width...
	x = (BASEVIDWIDTH - V_StringWidth(string, V_OLDSPACING))>>1;
	V_DrawString(x,y,V_OLDSPACING,string);
}

//
// M_DrawMapEmblems
//
// used by pause & statistics to draw a row of emblems for a map
//
static void M_DrawMapEmblems(INT32 mapnum, INT32 x, INT32 y)
{
	UINT8 lasttype = UINT8_MAX, curtype;
	emblem_t *emblem = M_GetLevelEmblems(mapnum);

	while (emblem)
	{
		switch (emblem->type)
		{
			case ET_SCORE: case ET_TIME: case ET_RINGS:
				curtype = 1; break;
			case ET_NGRADE: case ET_NTIME:
				curtype = 2; break;
			case ET_MAP:
				curtype = 3; break;
			default:
				curtype = 0; break;
		}

		// Shift over if emblem is of a different discipline
		if (lasttype != UINT8_MAX && lasttype != curtype)
			x -= 4;
		lasttype = curtype;

		if (emblem->collected)
			V_DrawSmallMappedPatch(x, y, 0, W_CachePatchName(M_GetEmblemPatch(emblem, false), PU_PATCH),
			                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
		else
			V_DrawSmallScaledPatch(x, y, 0, W_CachePatchName("NEEDIT", PU_PATCH));

		emblem = M_GetLevelEmblems(-1);
		x -= 12;
	}
}

static void M_DrawMenuTitle(void)
{
	if (currentMenu->menutitlepic)
	{
		patch_t *p = W_CachePatchName(currentMenu->menutitlepic, PU_PATCH);

		if (p->height > 24) // title is larger than normal
		{
			INT32 xtitle = (BASEVIDWIDTH - (p->width/2))/2;
			INT32 ytitle = (30 - (p->height/2))/2;

			if (xtitle < 0)
				xtitle = 0;
			if (ytitle < 0)
				ytitle = 0;

			V_DrawSmallScaledPatch(xtitle, ytitle, 0, p);
		}
		else
		{
			INT32 xtitle = (BASEVIDWIDTH - p->width)/2;
			INT32 ytitle = (30 - p->height)/2;

			if (xtitle < 0)
				xtitle = 0;
			if (ytitle < 0)
				ytitle = 0;

			V_DrawScaledPatch(xtitle, ytitle, 0, p);
		}
	}
}

static void M_DrawGenericMenu(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH);
						V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH));
					}
				}
				/* FALLTHRU */
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								if (i == itemOn)
								{
									V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
											'\x1C' | V_YELLOWMAP, false);
									V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											'\x1D' | V_YELLOWMAP, false);
								}
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				/* FALLTHRU */
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_PATCH), graymap);
				y += LINEHEIGHT;
				break;
			case IT_TRANSTEXT:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				/* FALLTHRU */
			case IT_TRANSTEXT2:
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;

				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_HEADERTEXT: // draws 16 pixels to the left, in yellow text
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;

				//V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
				M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), currentMenu->menuitems[i].text, true, false);
				y += SMALLLINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

const char *PlaystyleNames[4] = {"Strafe", "Standard", "Simple", "Old Analog??"};
const char *PlaystyleDesc[4] = {
	// Legacy
	"The play style used for\n"
	"old-school SRB2.\n"
	"\n"
	"This play style is identical\n"
	"to Standard, except that the\n"
	"player always looks in the\n"
	"direction of the camera."
	,

	// Standard
	"The default play style,\n"
	"designed for full control\n"
	"with a keyboard and mouse.\n"
	"\n"
	"The camera rotates only when\n"
	"you tell it to. The player\n"
	"looks in the direction they're\n"
	"moving, but acts in the direction\n"
	"the camera is facing.\n"
	"\n"
	"Mastery of this play style will\n"
	"open up the highest level of play!"
	,

	// Simple
	"A play style designed for\n"
	"gamepads and hassle-free play.\n"
	"\n"
	"The camera rotates automatically\n"
	"as you move, and the player faces\n"
	"and acts in the direction\n"
	"they're moving.\n"
	"\n"
	"Hold \x82" "Center View\x80 to lock the\n"
	"camera behind the player!\n"
	,

	// Old Analog
	"I see.\n"
	"\n"
	"You really liked the old analog mode,\n"
	"so when 2.2 came out, you opened up\n"
	"your config file and brought it back.\n"
	"\n"
	"That's absolutely valid, but I implore\n"
	"you to try the new Simple play style\n"
	"instead!"
};

static UINT8 playstyle_activeplayer = 0, playstyle_currentchoice = 0;

static void M_DrawControlsDefMenu(void)
{
	UINT8 opt = 0;

	M_DrawGenericMenu();

	if (currentMenu == &OP_P1ControlsDef)
	{
		opt = cv_directionchar[0].value ? 1 : 0;
		opt = playstyle_currentchoice = cv_useranalog[0].value ? 3 - opt : opt;

		if (opt == 2)
		{
			OP_CameraOptionsDef.menuitems = OP_CameraExtendedOptionsMenu;
			OP_CameraOptionsDef.numitems = sizeof (OP_CameraExtendedOptionsMenu) / sizeof (menuitem_t);
		}
		else
		{
			OP_CameraOptionsDef.menuitems = OP_CameraOptionsMenu;
			OP_CameraOptionsDef.numitems = sizeof (OP_CameraOptionsMenu) / sizeof (menuitem_t);
		}
	}
	else
	{
		opt = cv_directionchar[1].value ? 1 : 0;
		opt = playstyle_currentchoice = cv_useranalog[1].value ? 3 - opt : opt;

		if (opt == 2)
		{
			OP_Camera2OptionsDef.menuitems = OP_Camera2ExtendedOptionsMenu;
			OP_Camera2OptionsDef.numitems = sizeof (OP_Camera2ExtendedOptionsMenu) / sizeof (menuitem_t);
		}
		else
		{
			OP_Camera2OptionsDef.menuitems = OP_Camera2OptionsMenu;
			OP_Camera2OptionsDef.numitems = sizeof (OP_Camera2OptionsMenu) / sizeof (menuitem_t);
		}
	}

	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + 80, V_YELLOWMAP, PlaystyleNames[opt]);
}

#define scrollareaheight 72

// note that alphakey is multiplied by 2 for scrolling menus to allow greater usage in UINT8 range.
static void M_DrawGenericScrollMenu(void)
{
	INT32 x, y, i, max, bottom, tempcentery, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	if (currentMenu->menuitems[currentMenu->numitems-1].alphaKey < scrollareaheight)
		tempcentery = currentMenu->y; // Not tall enough to scroll, but this thinker is used in case it becomes so
	else if ((currentMenu->menuitems[itemOn].alphaKey*2 - currentMenu->menuitems[0].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[0].alphaKey*2;
	else if ((currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 - currentMenu->menuitems[itemOn].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 + 2*scrollareaheight;
	else
		tempcentery = currentMenu->y - currentMenu->menuitems[itemOn].alphaKey*2 + scrollareaheight;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status != IT_DISABLED && currentMenu->menuitems[i].alphaKey*2 + tempcentery >= currentMenu->y)
			break;
	}

	for (bottom = currentMenu->numitems; bottom > 0; bottom--)
	{
		if (currentMenu->menuitems[bottom-1].status != IT_DISABLED)
			break;
	}

	for (max = bottom; max > 0; max--)
	{
		if (currentMenu->menuitems[max-1].status != IT_DISABLED && currentMenu->menuitems[max-1].alphaKey*2 + tempcentery <= (currentMenu->y + 2*scrollareaheight))
			break;
	}

	if (i)
		V_DrawString(currentMenu->x - 20, currentMenu->y - (skullAnimCounter/5), V_YELLOWMAP, "\x1A"); // up arrow
	if (max != bottom)
		V_DrawString(currentMenu->x - 20, currentMenu->y + 2*scrollareaheight + (skullAnimCounter/5), V_YELLOWMAP, "\x1B"); // down arrow

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (; i < max; i++)
	{
		y = currentMenu->menuitems[i].alphaKey*2 + tempcentery;
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
			case IT_DYBIGSPACE:
			case IT_BIGSLIDER:
			case IT_STRING2:
			case IT_DYLITLSPACE:
			case IT_GRAYPATCH:
			case IT_TRANSTEXT2:
				// unsupported
				break;
			case IT_NOTHING:
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (i != itemOn && (currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
#if 1
								if (y + 12 > (currentMenu->y + 2*scrollareaheight))
									break;
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
#else // cool new string type stuff, not ready for limelight
								if (i == itemOn)
								{
									V_DrawFill(x-2, y-1, MAXSTRINGLENGTH*8 + 4, 8+3, 159);
									V_DrawString(x, y, V_ALLOWLOWERCASE, cv->string);
									if (skullAnimCounter < 4)
										V_DrawCharacter(x + V_StringWidth(cv->string, 0), y, '_' | 0x80, false);
								}
								else
									V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
									V_YELLOWMAP|V_ALLOWLOWERCASE, cv->string);
#endif
								break;
							default:
								V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								if (i == itemOn)
								{
									V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
											'\x1C' | V_YELLOWMAP, false);
									V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											'\x1D' | V_YELLOWMAP, false);
								}
								break;
						}
						break;
					}
					break;
			case IT_TRANSTEXT:
				switch (currentMenu->menuitems[i].status & IT_TYPE)
				{
					case IT_PAIR:
						V_DrawString(x, y,
								V_TRANSLUCENT, currentMenu->menuitems[i].patch);
						V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
								V_TRANSLUCENT, currentMenu->menuitems[i].text);
						break;
					default:
						V_DrawString(x, y,
								V_TRANSLUCENT, currentMenu->menuitems[i].text);
				}
				break;
			case IT_QUESTIONMARKS:
				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				break;
			case IT_HEADERTEXT:
				//V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
				M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), currentMenu->menuitems[i].text, true, false);
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}

static void M_DrawPauseMenu(void)
{
	if (!netgame && !multiplayer && (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		emblem_t *emblem_detail[3] = {NULL, NULL, NULL};
		char emblem_text[3][20];
		INT32 i;

		M_DrawTextBox(27, 16, 32, 6);

		// Draw any and all emblems at the top.
		M_DrawMapEmblems(gamemap, 272, 28);

		if (mapheaderinfo[gamemap-1]->actnum != 0)
			V_DrawString(40, 28, V_YELLOWMAP, va("%s %d", mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum));
		else
			V_DrawString(40, 28, V_YELLOWMAP, mapheaderinfo[gamemap-1]->lvlttl);

		// Set up the detail boxes.
		{
			emblem_t *emblem = M_GetLevelEmblems(gamemap);
			while (emblem)
			{
				INT32 emblemslot;
				char targettext[9], currenttext[9];

				switch (emblem->type)
				{
					case ET_SCORE:
						snprintf(targettext, 9, "%d", emblem->var);
						snprintf(currenttext, 9, "%u", G_GetBestScore(gamemap));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 0;
						break;
					case ET_TIME:
						emblemslot = emblem->var; // dumb hack
						snprintf(targettext, 9, "%i:%02i.%02i",
							G_TicsToMinutes((tic_t)emblemslot, false),
							G_TicsToSeconds((tic_t)emblemslot),
							G_TicsToCentiseconds((tic_t)emblemslot));

						emblemslot = (INT32)G_GetBestTime(gamemap); // dumb hack pt ii
						if ((tic_t)emblemslot == UINT32_MAX)
							snprintf(currenttext, 9, "-:--.--");
						else
							snprintf(currenttext, 9, "%i:%02i.%02i",
								G_TicsToMinutes((tic_t)emblemslot, false),
								G_TicsToSeconds((tic_t)emblemslot),
								G_TicsToCentiseconds((tic_t)emblemslot));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 1;
						break;
					case ET_RINGS:
						snprintf(targettext, 9, "%d", emblem->var);
						snprintf(currenttext, 9, "%u", G_GetBestRings(gamemap));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 2;
						break;
					case ET_NGRADE:
						snprintf(targettext, 9, "%u", P_GetScoreForGrade(gamemap, 0, emblem->var));
						snprintf(currenttext, 9, "%u", G_GetBestNightsScore(gamemap, 0));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 1;
						break;
					case ET_NTIME:
						emblemslot = emblem->var; // dumb hack pt iii
						snprintf(targettext, 9, "%i:%02i.%02i",
							G_TicsToMinutes((tic_t)emblemslot, false),
							G_TicsToSeconds((tic_t)emblemslot),
							G_TicsToCentiseconds((tic_t)emblemslot));

						emblemslot = (INT32)G_GetBestNightsTime(gamemap, 0); // dumb hack pt iv
						if ((tic_t)emblemslot == UINT32_MAX)
							snprintf(currenttext, 9, "-:--.--");
						else
							snprintf(currenttext, 9, "%i:%02i.%02i",
								G_TicsToMinutes((tic_t)emblemslot, false),
								G_TicsToSeconds((tic_t)emblemslot),
								G_TicsToCentiseconds((tic_t)emblemslot));

						targettext[8] = 0;
						currenttext[8] = 0;

						emblemslot = 2;
						break;
					default:
						goto bademblem;
				}
				if (emblem_detail[emblemslot])
					goto bademblem;

				emblem_detail[emblemslot] = emblem;
				snprintf(emblem_text[emblemslot], 20, "%8s /%8s", currenttext, targettext);
				emblem_text[emblemslot][19] = 0;

				bademblem:
				emblem = M_GetLevelEmblems(-1);
			}
		}
		for (i = 0; i < 3; ++i)
		{
			emblem_t *emblem = emblem_detail[i];
			if (!emblem)
				continue;

			if (emblem->collected)
				V_DrawSmallMappedPatch(40, 44 + (i*8), 0, W_CachePatchName(M_GetEmblemPatch(emblem, false), PU_PATCH),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(40, 44 + (i*8), 0, W_CachePatchName("NEEDIT", PU_PATCH));

			switch (emblem->type)
			{
				case ET_SCORE:
				case ET_NGRADE:
					V_DrawString(56, 44 + (i*8), V_YELLOWMAP, "SCORE:");
					break;
				case ET_TIME:
				case ET_NTIME:
					V_DrawString(56, 44 + (i*8), V_YELLOWMAP, "TIME:");
					break;
				case ET_RINGS:
					V_DrawString(56, 44 + (i*8), V_YELLOWMAP, "RINGS:");
					break;
			}
			V_DrawRightAlignedString(284, 44 + (i*8), V_MONOSPACE, emblem_text[i]);
		}
	}

	M_DrawGenericMenu();
}

static void M_DrawCenteredMenu(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH);
						V_DrawScaledPatch((BASEVIDWIDTH - p->width)/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_PATCH));
					}
				}
				/* FALLTHRU */
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawCenteredString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch(currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch(currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0), y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								if (i == itemOn)
								{
									V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
											'\x1C' | V_YELLOWMAP, false);
									V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											'\x1D' | V_YELLOWMAP, false);
								}
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
				/* FALLTHRU */
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_QUESTIONMARKS:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;

				V_DrawCenteredString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_PATCH), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
	}
	else
	{
		V_DrawScaledPatch(x - V_StringWidth(currentMenu->menuitems[itemOn].text, 0)/2 - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_PATCH));
		V_DrawCenteredString(x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

// ==========================================================================
// Extraneous menu patching functions
// ==========================================================================

//
// M_PatchSkinNameTable
//
// Like M_PatchLevelNameTable, but for cv_chooseskin
//
static void M_PatchSkinNameTable(void)
{
	INT32 j;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	for (j = 0; j < MAXSKINS; j++)
	{
		if (skins[j].name[0] != '\0' && R_SkinUsable(-1, j))
		{
			skins_cons_t[j].strvalue = skins[j].realname;
			skins_cons_t[j].value = j+1;
		}
		else
		{
			skins_cons_t[j].strvalue = NULL;
			skins_cons_t[j].value = 0;
		}
	}

	CV_SetValue(&cv_chooseskin, 1);
	Nextmap_OnChange();

	return;
}

//
// M_LevelAvailableOnPlatter
//
// Okay, you know that the level SHOULD show up on the platter already.
// The only question is whether it should be as a question mark,
// (hinting as to its existence), or as its pure, unfettered self.
//
static boolean M_LevelAvailableOnPlatter(INT32 mapnum)
{
	if (M_MapLocked(mapnum+1))
		return false; // not unlocked

	switch (levellistmode)
	{
		case LLM_CREATESERVER:
			if (!(mapheaderinfo[mapnum]->typeoflevel & TOL_COOP))
				return true;

			if (mapnum+1 == spstage_start)
				return true;

#ifndef DEVELOP
			if (mapvisited[mapnum]) // MV_MP
#endif
				return true;

			/* FALLTHRU */
		case LLM_RECORDATTACK:
		case LLM_NIGHTSATTACK:
#ifndef DEVELOP
			if (mapvisited[mapnum] & MV_MAX)
				return true;

			if (mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
#endif
				return true;

			return false;
		case LLM_LEVELSELECT:
		default:
			return true;
	}
	return true;
}

//
// M_CanShowLevelOnPlatter
//
// Determines whether to show a given map in the various level-select lists.
// Set gt = -1 to ignore gametype.
//
static boolean M_CanShowLevelOnPlatter(INT32 mapnum, INT32 gt)
{
	// Does the map exist?
	if (!mapheaderinfo[mapnum])
		return false;

	// Does the map have a name?
	if (!mapheaderinfo[mapnum]->lvlttl[0])
		return false;

	/*if (M_MapLocked(mapnum+1))
		return false; // not unlocked*/

	switch (levellistmode)
	{
		case LLM_CREATESERVER:
			// Should the map be hidden?
			if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
				return false;

			if (G_IsSpecialStage(mapnum+1))
				return false;

			if (gt == GT_COOP && (mapheaderinfo[mapnum]->typeoflevel & TOL_COOP))
				return true;

			if (gt == GT_COMPETITION && (mapheaderinfo[mapnum]->typeoflevel & TOL_COMPETITION))
				return true;

			if (gt == GT_CTF && (mapheaderinfo[mapnum]->typeoflevel & TOL_CTF))
				return true;

			if ((gt == GT_MATCH || gt == GT_TEAMMATCH) && (mapheaderinfo[mapnum]->typeoflevel & TOL_MATCH))
				return true;

			if ((gt == GT_TAG || gt == GT_HIDEANDSEEK) && (mapheaderinfo[mapnum]->typeoflevel & TOL_TAG))
				return true;

			if (gt == GT_RACE && (mapheaderinfo[mapnum]->typeoflevel & TOL_RACE))
				return true;

			if (gt >= 0 && gt < gametypecount && (mapheaderinfo[mapnum]->typeoflevel & gametypetol[gt]))
				return true;

			return false;

		case LLM_LEVELSELECT:
			if (!(mapheaderinfo[mapnum]->levelselect & maplistoption))
				return false;

			return true;
		case LLM_RECORDATTACK:
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_RECORDATTACK))
				return false;

			return true;
		case LLM_NIGHTSATTACK:
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_NIGHTSATTACK))
				return false;

			return true;
	}

	// Hmm? Couldn't decide?
	return false;
}

#if 0
static INT32 M_CountLevelsToShowOnPlatter(INT32 gt)
{
	INT32 mapnum, count = 0;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelOnPlatter(mapnum, gt))
			count++;

	return count;
}
#endif

#if 0
static boolean M_SetNextMapOnPlatter(void)
{
	INT32 row, col = 0;
	while (col < 3)
	{
		row = 0;
		while (row < levelselect.numrows)
		{
			if (levelselect.rows[row].maplist[col] == cv_nextmap.value)
			{
				lsrow = row;
				lscol = col;
				return true;
			}
			row++;
		}
		col++;
	}
	return true;
}
#endif

static boolean M_GametypeHasLevels(INT32 gt)
{
	INT32 mapnum;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelOnPlatter(mapnum, gt))
			return true;

	return false;
}

static INT32 M_CountRowsToShowOnPlatter(INT32 gt)
{
	INT32 mapnum = 0, prevmapnum = 0, col = 0, rows = 0;

	while (mapnum < NUMMAPS)
	{
		if (M_CanShowLevelOnPlatter(mapnum, gt))
		{
			if (rows == 0)
				rows++;
			else
			{
				if (col == 2
				|| (mapheaderinfo[prevmapnum]->menuflags & LF2_WIDEICON)
				|| (mapheaderinfo[mapnum]->menuflags & LF2_WIDEICON)
				|| !(fastcmp(mapheaderinfo[mapnum]->selectheading, mapheaderinfo[prevmapnum]->selectheading)))
				{
					col = 0;
					rows++;
				}
				else
					col++;
			}
			prevmapnum = mapnum;
		}
		mapnum++;
	}

	if (levellistmode == LLM_CREATESERVER)
		rows++;

	return rows;
}

//
// M_CacheLevelPlatter
//
// Cache every patch used by the level platter.
//
static void M_CacheLevelPlatter(void)
{
	levselp[0][0] = W_CachePatchName("SLCT1LVL", PU_PATCH);
	levselp[0][1] = W_CachePatchName("SLCT2LVL", PU_PATCH);
	levselp[0][2] = W_CachePatchName("BLANKLVL", PU_PATCH);

	levselp[1][0] = W_CachePatchName("SLCT1LVW", PU_PATCH);
	levselp[1][1] = W_CachePatchName("SLCT2LVW", PU_PATCH);
	levselp[1][2] = W_CachePatchName("BLANKLVW", PU_PATCH);
}

//
// M_PrepareLevelPlatter
//
// Prepares a tasty dish of zones and acts!
// Call before any attempt to access a level platter.
//
static boolean M_PrepareLevelPlatter(INT32 gt, boolean nextmappick)
{
	INT32 numrows = M_CountRowsToShowOnPlatter(gt);
	INT32 mapnum = 0, prevmapnum = 0, col = 0, row = 0, startrow = 0;

	if (!numrows)
		return false;

	if (levelselect.rows)
		Z_Free(levelselect.rows);
	levelselect.rows = NULL;

	levelselect.numrows = numrows;
	levelselect.rows = Z_Realloc(levelselect.rows, numrows*sizeof(levelselectrow_t), PU_STATIC, NULL);
	if (!levelselect.rows)
		I_Error("Insufficient memory to prepare level platter");

	// done here so lsrow and lscol can be set if cv_nextmap is on the platter
	lsrow = lscol = lshli = lsoffs[0] = lsoffs[1] = 0;

	if (levellistmode == LLM_CREATESERVER)
	{
		sprintf(levelselect.rows[0].header, "Gametype");
		lswide(0) = true;
		levelselect.rows[row].mapavailable[2] = levelselect.rows[row].mapavailable[1] = levelselect.rows[row].mapavailable[0] = false;
		startrow = row = 1;

		Z_Free(char_notes);
		char_notes = NULL;
	}

	while (mapnum < NUMMAPS)
	{
		if (M_CanShowLevelOnPlatter(mapnum, gt))
		{
			const UINT8 actnum = mapheaderinfo[mapnum]->actnum;
			const boolean headingisname = (fastcmp(mapheaderinfo[mapnum]->selectheading, mapheaderinfo[mapnum]->lvlttl));
			const boolean wide = (mapheaderinfo[mapnum]->menuflags & LF2_WIDEICON);

			// preparing next position to drop mapnum into
			if (levelselect.rows[startrow].maplist[0])
			{
				if (col == 2 // no more space on the row?
				|| wide
				|| (mapheaderinfo[prevmapnum]->menuflags & LF2_WIDEICON)
				|| !(fastcmp(mapheaderinfo[mapnum]->selectheading, mapheaderinfo[prevmapnum]->selectheading))) // a new heading is starting?
				{
					col = 0;
					row++;
				}
				else
					col++;
			}

			levelselect.rows[row].maplist[col] = mapnum+1; // putting the map on the platter
			levelselect.rows[row].mapavailable[col] = M_LevelAvailableOnPlatter(mapnum);

			if ((lswide(row) = wide)) // intentionally assignment
			{
				levelselect.rows[row].maplist[2] = levelselect.rows[row].maplist[1] = levelselect.rows[row].maplist[0];
				levelselect.rows[row].mapavailable[2] = levelselect.rows[row].mapavailable[1] = levelselect.rows[row].mapavailable[0];
			}

			if (nextmappick && cv_nextmap.value == mapnum+1) // A little quality of life improvement.
			{
				lsrow = row;
				lscol = col;
			}

			// individual map name
			if (levelselect.rows[row].mapavailable[col])
			{
				if (headingisname)
				{
					if (actnum)
						sprintf(levelselect.rows[row].mapnames[col], "ACT %d", actnum);
					else
						sprintf(levelselect.rows[row].mapnames[col], "THE ACT");
				}
				else if (wide)
				{
					// Yes, with LF2_WIDEICON it'll continue on over into the next 17+1 char block. That's alright; col is always zero, the string is contiguous, and the maximum length is lvlttl[22] + ' ' + ZONE + ' ' + INT32, which is about 39 or so - barely crossing into the third column.
					char* mapname = G_BuildMapTitle(mapnum+1);
					strcpy(levelselect.rows[row].mapnames[col], (const char *)mapname);
					Z_Free(mapname);
				}
				else
				{
					char mapname[22+1+11]; // lvlttl[22] + ' ' + INT32

					if (actnum)
						sprintf(mapname, "%s %d", mapheaderinfo[mapnum]->lvlttl, actnum);
					else
						strcpy(mapname, mapheaderinfo[mapnum]->lvlttl);

					if (strlen(mapname) >= 17)
						strcpy(mapname+17-3, "...");

					strcpy(levelselect.rows[row].mapnames[col], (const char *)mapname);
				}
			}
			else
				sprintf(levelselect.rows[row].mapnames[col], "???");

			// creating header text
			if (!col && ((row == startrow) || !(fastcmp(mapheaderinfo[mapnum]->selectheading, mapheaderinfo[levelselect.rows[row-1].maplist[0]-1]->selectheading))))
			{
				if (!levelselect.rows[row].mapavailable[col])
					sprintf(levelselect.rows[row].header, "???");
				else
				{
					sprintf(levelselect.rows[row].header, "%s", mapheaderinfo[mapnum]->selectheading);
					if (!(mapheaderinfo[mapnum]->levelflags & LF_NOZONE) && headingisname)
					{
						sprintf(levelselect.rows[row].header + strlen(levelselect.rows[row].header), " ZONE");
					}
				}
			}

			prevmapnum = mapnum;
		}

		mapnum++;
	}

#ifdef SYMMETRICAL_PLATTER
	// horizontally space out rows with missing right sides
	for (; row >= 0; row--)
	{
		if (!levelselect.rows[row].maplist[2] // no right side
		&& levelselect.rows[row].maplist[0] && levelselect.rows[row].maplist[1]) // all the left filled in
		{
			levelselect.rows[row].maplist[2] = levelselect.rows[row].maplist[1];
			STRBUFCPY(levelselect.rows[row].mapnames[2], levelselect.rows[row].mapnames[1]);
			levelselect.rows[row].mapavailable[2] = levelselect.rows[row].mapavailable[1];

			levelselect.rows[row].maplist[1] = -1; // diamond
			levelselect.rows[row].mapnames[1][0] = '\0';
			levelselect.rows[row].mapavailable[1] = false;
		}
	}
#endif

	M_CacheLevelPlatter();

	return true;
}

#define ifselectvalnextmapnobrace(column) if ((selectval = levelselect.rows[lsrow].maplist[column]) && levelselect.rows[lsrow].mapavailable[column])\
			{\
				CV_SetValue(&cv_nextmap, selectval);

#define ifselectvalnextmap(column) ifselectvalnextmapnobrace(column)}

//
// M_HandleLevelPlatter
//
// Reacts to your key inputs. Basically a mini menu thinker.
//
static void M_HandleLevelPlatter(INT32 choice)
{
	boolean exitmenu = false;  // exit to previous menu
	INT32 selectval;
	UINT8 iter;

	switch (choice)
	{
		case KEY_DOWNARROW:
			if (lsrow == levelselect.numrows-1)
			{
				if (levelselect.numrows < 3)
				{
					if (!lsoffs[0]) // prevent sound spam
					{
						lsoffs[0] = -8;
						S_StartSound(NULL,sfx_s3kb7);
					}
					return;
				}
				lsrow = UINT8_MAX;
			}
			lsrow++;

			lsoffs[0] = lsvseperation(lsrow);

			if (levelselect.rows[lsrow].header[0])
				lshli = lsrow;
			// no else needed - headerless lines associate upwards, so moving down to a row without a header is identity

			S_StartSound(NULL,sfx_s3kb7);

			ifselectvalnextmap(lscol) else ifselectvalnextmap(0)
			break;

		case KEY_UPARROW:
			iter = lsrow;
			if (!lsrow)
			{
				if (levelselect.numrows < 3)
				{
					if (!lsoffs[0]) // prevent sound spam
					{
						lsoffs[0] = 8;
						S_StartSound(NULL,sfx_s3kb7);
					}
					return;
				}
				lsrow = levelselect.numrows;
			}
			lsrow--;

			lsoffs[0] = -lsvseperation(iter);

			if (levelselect.rows[lsrow].header[0])
				lshli = lsrow;
			else
			{
				iter = lsrow;
				do
					iter = ((iter == 0) ? levelselect.numrows-1 : iter-1);
				while ((iter != lsrow) && !(levelselect.rows[iter].header[0]));
				lshli = iter;
			}

			S_StartSound(NULL,sfx_s3kb7);

			ifselectvalnextmap(lscol) else ifselectvalnextmap(0)
			break;

		case KEY_ENTER:
			if (!(levellistmode == LLM_CREATESERVER && !lsrow))
			{
				ifselectvalnextmapnobrace(lscol)
					lsoffs[0] = lsoffs[1] = 0;
					S_StartSound(NULL,sfx_menu1);
					if (gamestate == GS_TIMEATTACK)
						M_SetupNextMenu(currentMenu->prevMenu);
					else if (currentMenu == &MISC_ChangeLevelDef)
					{
						if (currentMenu->prevMenu && currentMenu->prevMenu != &MPauseDef)
							M_SetupNextMenu(currentMenu->prevMenu);
						else
							M_ChangeLevel(0);
						Z_Free(levelselect.rows);
						levelselect.rows = NULL;
					}
					else
						M_LevelSelectWarp(0);
					Nextmap_OnChange();
				}
				else if (!lsoffs[0]) // prevent sound spam
				{
					lsoffs[0] = -8;
					S_StartSound(NULL,sfx_s3kb2);
				}
				break;
			}
			/* FALLTHRU */
		case KEY_RIGHTARROW:
			if (levellistmode == LLM_CREATESERVER && !lsrow)
			{
				INT32 startinggametype = cv_newgametype.value;
				do
					CV_AddValue(&cv_newgametype, 1);
				while (cv_newgametype.value != startinggametype && !M_GametypeHasLevels(cv_newgametype.value));
				S_StartSound(NULL,sfx_menu1);
				lscol = 0;

				Z_Free(char_notes);
				char_notes = NULL;

				if (!M_PrepareLevelPlatter(cv_newgametype.value, false))
					I_Error("Unidentified level platter failure!");
			}
			else if (lscol < 2)
			{
				lscol++;

				lsoffs[1] = (lswide(lsrow) ? 8 : -lshseperation);
				S_StartSound(NULL,sfx_s3kb7);

				ifselectvalnextmap(lscol) else ifselectvalnextmap(0)
			}
			else if (!lsoffs[1]) // prevent sound spam
			{
				lsoffs[1] = 8;
				S_StartSound(NULL,sfx_s3kb7);
			}
			break;

		case KEY_LEFTARROW:
			if (levellistmode == LLM_CREATESERVER && !lsrow)
			{
				INT32 startinggametype = cv_newgametype.value;
				do
					CV_AddValue(&cv_newgametype, -1);
				while (cv_newgametype.value != startinggametype && !M_GametypeHasLevels(cv_newgametype.value));
				S_StartSound(NULL,sfx_menu1);
				lscol = 0;

				Z_Free(char_notes);
				char_notes = NULL;

				if (!M_PrepareLevelPlatter(cv_newgametype.value, false))
					I_Error("Unidentified level platter failure!");
			}
			else if (lscol > 0)
			{
				lscol--;

				lsoffs[1] = (lswide(lsrow) ? -8 : lshseperation);
				S_StartSound(NULL,sfx_s3kb7);

				ifselectvalnextmap(lscol) else ifselectvalnextmap(0)
			}
			else if (!lsoffs[1]) // prevent sound spam
			{
				lsoffs[1] = -8;
				S_StartSound(NULL,sfx_s3kb7);
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}

	if (exitmenu)
	{
		if (gamestate != GS_TIMEATTACK)
		{
			Z_Free(levelselect.rows);
			levelselect.rows = NULL;
		}

		if (currentMenu->prevMenu)
		{
			M_SetupNextMenu(currentMenu->prevMenu);
			Nextmap_OnChange();
		}
		else
			M_ClearMenus(true);

		Z_Free(char_notes);
		char_notes = NULL;
	}
}

void M_DrawLevelPlatterHeader(INT32 y, const char *header, boolean headerhighlight, boolean allowlowercase)
{
	y += lsheadingheight - 12;
	V_DrawString(19, y, (headerhighlight ? V_YELLOWMAP : 0)|(allowlowercase ? V_ALLOWLOWERCASE : 0), header);
	y += 9;
	V_DrawFill(19, y, 281, 1, (headerhighlight ? yellowmap[3] : 3));
	V_DrawFill(300, y, 1, 1, 26);
	y++;
	V_DrawFill(19, y, 282, 1, 26);
}

static void M_DrawLevelPlatterWideMap(UINT8 row, UINT8 col, INT32 x, INT32 y, boolean highlight)
{
	patch_t *patch;

	INT32 map = levelselect.rows[row].maplist[col];
	if (map <= 0)
		return;

	//  A 564x100 image of the level as entry MAPxxW
	if (!(levelselect.rows[row].mapavailable[col]))
	{
		V_DrawSmallScaledPatch(x, y, 0, levselp[1][2]);
		M_DrawStaticBox(x, y, V_80TRANS, 282, 50);
	}
	else
	{
		if (W_CheckNumForName(va("%sW", G_BuildMapName(map))) != LUMPERROR)
			patch = W_CachePatchName(va("%sW", G_BuildMapName(map)), PU_PATCH);
		else
			patch = levselp[1][2]; // don't static to indicate that it's just a normal level

		V_DrawSmallScaledPatch(x, y, 0, patch);
	}

	V_DrawFill(x, y+50, 282, 8,
		((mapheaderinfo[map-1]->unlockrequired < 0)
		? 159 : 63));

	V_DrawString(x, y+50, (highlight ? V_YELLOWMAP : 0), levelselect.rows[row].mapnames[col]);
}

static void M_DrawLevelPlatterMap(UINT8 row, UINT8 col, INT32 x, INT32 y, boolean highlight)
{
	patch_t *patch;

	INT32 map = levelselect.rows[row].maplist[col];
	if (map <= 0)
		return;

	//  A 160x100 image of the level as entry MAPxxP
	if (!(levelselect.rows[row].mapavailable[col]))
	{
		V_DrawSmallScaledPatch(x, y, 0, levselp[0][2]);
		M_DrawStaticBox(x, y, V_80TRANS, 80, 50);
	}
	else
	{
		if (W_CheckNumForName(va("%sP", G_BuildMapName(map))) != LUMPERROR)
			patch = W_CachePatchName(va("%sP", G_BuildMapName(map)), PU_PATCH);
		else
			patch = levselp[0][2]; // don't static to indicate that it's just a normal level

		V_DrawSmallScaledPatch(x, y, 0, patch);
	}

	V_DrawFill(x, y+50, 80, 8,
		((mapheaderinfo[map-1]->unlockrequired < 0)
		? 159 : 63));

	if (strlen(levelselect.rows[row].mapnames[col]) > 6) // "AERIAL GARDEN" vs "ACT 18" - "THE ACT" intentionally compressed
		V_DrawThinString(x, y+50, (highlight ? V_YELLOWMAP : 0), levelselect.rows[row].mapnames[col]);
	else
		V_DrawString(x, y+50, (highlight ? V_YELLOWMAP : 0), levelselect.rows[row].mapnames[col]);
}

static void M_DrawLevelPlatterRow(UINT8 row, INT32 y)
{
	UINT8 col;
	const boolean rowhighlight = (row == lsrow);
	if (levelselect.rows[row].header[0])
	{
		M_DrawLevelPlatterHeader(y, levelselect.rows[row].header, (rowhighlight || (row == lshli)), false);
		y += lsheadingheight;
	}

	if (levellistmode == LLM_CREATESERVER && !row)
	{
		if (!char_notes)
			char_notes = V_WordWrap(0, 282 - 8, V_ALLOWLOWERCASE, gametypedesc[cv_newgametype.value].notes);

		V_DrawFill(lsbasex, y, 282, 50, 27);
		V_DrawString(lsbasex + 4, y + 4, V_RETURN8|V_ALLOWLOWERCASE, char_notes);

		V_DrawFill(lsbasex,     y+50, 141, 8, gametypedesc[cv_newgametype.value].col[0]);
		V_DrawFill(lsbasex+141, y+50, 141, 8, gametypedesc[cv_newgametype.value].col[1]);

		V_DrawString(lsbasex, y+50, 0, gametype_cons_t[cv_newgametype.value].strvalue);

		if (!lsrow)
		{
			V_DrawCharacter(lsbasex - 10 - (skullAnimCounter/5), y+25,
				'\x1C' | V_YELLOWMAP, false);
			V_DrawCharacter(lsbasex+282 + 2 + (skullAnimCounter/5), y+25,
				'\x1D' | V_YELLOWMAP, false);
		}
	}
	else if (lswide(row))
		M_DrawLevelPlatterWideMap(row, 0, lsbasex, y, rowhighlight);
	else
	{
		for (col = 0; col < 3; col++)
			M_DrawLevelPlatterMap(row, col, lsbasex+(col*lshseperation), y, (rowhighlight && (col == lscol)));
	}
}

// new menus
static void M_DrawRecordAttackForeground(void)
{
	patch_t *fg = W_CachePatchName("RECATKFG", PU_PATCH);
	patch_t *clock = W_CachePatchName("RECCLOCK", PU_PATCH);
	angle_t fa;

	INT32 i;
	INT32 height = (fg->height / 2);
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);

	for (i = -12; i < (BASEVIDHEIGHT/height) + 12; i++)
	{
		INT32 y = ((i*height) - (height - ((recatkdrawtimer*2)%height)));
		// don't draw above the screen
		{
			INT32 sy = FixedMul(y, dupz<<FRACBITS) >> FRACBITS;
			if (vid.height != BASEVIDHEIGHT * dupz)
				sy += (vid.height - (BASEVIDHEIGHT * dupz)) / 2;
			if ((sy+height) < 0)
				continue;
		}
		V_DrawFixedPatch(0, y<<FRACBITS, FRACUNIT/2, V_SNAPTOLEFT, fg, NULL);
		V_DrawFixedPatch(BASEVIDWIDTH<<FRACBITS, y<<FRACBITS, FRACUNIT/2, V_SNAPTORIGHT|V_FLIP, fg, NULL);
		// don't draw below the screen
		if (y > vid.height)
			break;
	}

	// draw clock
	fa = (FixedAngle(((recatkdrawtimer * 4) % 360)<<FRACBITS)>>ANGLETOFINESHIFT) & FINEMASK;
	V_DrawSciencePatch(160<<FRACBITS, (80<<FRACBITS) + (4*FINESINE(fa)), 0, clock, FRACUNIT);

	// Increment timer.
	recatkdrawtimer++;
}

// NiGHTS Attack background.
static void M_DrawNightsAttackMountains(void)
{
	static INT32 bgscrollx;
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy);
	patch_t *background = W_CachePatchName(curbgname, PU_PATCH);
	INT16 w = background->width;
	INT32 x = FixedInt(-bgscrollx) % w;
	INT32 y = BASEVIDHEIGHT - (background->height * 2);

	if (vid.height != BASEVIDHEIGHT * dupz)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 158);
	V_DrawFill(0, y+50, vid.width, BASEVIDHEIGHT, V_SNAPTOLEFT|31);

	V_DrawScaledPatch(x, y, V_SNAPTOLEFT, background);
	x += w;
	if (x < BASEVIDWIDTH)
		V_DrawScaledPatch(x, y, V_SNAPTOLEFT, background);

	bgscrollx += (FRACUNIT/2);
	if (bgscrollx > w<<FRACBITS)
		bgscrollx &= 0xFFFF;
}

// NiGHTS Attack foreground.
static void M_DrawNightsAttackBackground(void)
{
	INT32 x, y = 0;
	INT32 i;

	// top
	patch_t *backtopfg = W_CachePatchName("NTSATKT1", PU_PATCH);
	patch_t *fronttopfg = W_CachePatchName("NTSATKT2", PU_PATCH);
	INT32 backtopwidth = backtopfg->width;
	//INT32 backtopheight = backtopfg->height;
	INT32 fronttopwidth = fronttopfg->width;
	//INT32 fronttopheight = fronttopfg->height;

	// bottom
	patch_t *backbottomfg = W_CachePatchName("NTSATKB1", PU_PATCH);
	patch_t *frontbottomfg = W_CachePatchName("NTSATKB2", PU_PATCH);
	INT32 backbottomwidth = backbottomfg->width;
	INT32 backbottomheight = backbottomfg->height;
	INT32 frontbottomwidth = frontbottomfg->width;
	INT32 frontbottomheight = frontbottomfg->height;

	// background
	M_DrawNightsAttackMountains();

	// back top foreground patch
	x = 0-(ntsatkdrawtimer%backtopwidth);
	V_DrawScaledPatch(x, y, V_SNAPTOTOP|V_SNAPTOLEFT, backtopfg);
	for (i = 0; i < 3; i++)
	{
		x += (backtopwidth);
		if (x >= vid.width)
			break;
		V_DrawScaledPatch(x, y, V_SNAPTOTOP|V_SNAPTOLEFT, backtopfg);
	}

	// front top foreground patch
	x = 0-((ntsatkdrawtimer*2)%fronttopwidth);
	V_DrawScaledPatch(x, y, V_SNAPTOTOP|V_SNAPTOLEFT, fronttopfg);
	for (i = 0; i < 3; i++)
	{
		x += (fronttopwidth);
		if (x >= vid.width)
			break;
		V_DrawScaledPatch(x, y, V_SNAPTOTOP|V_SNAPTOLEFT, fronttopfg);
	}

	// back bottom foreground patch
	x = 0-(ntsatkdrawtimer%backbottomwidth);
	y = BASEVIDHEIGHT - backbottomheight;
	V_DrawScaledPatch(x, y, V_SNAPTOBOTTOM|V_SNAPTOLEFT, backbottomfg);
	for (i = 0; i < 3; i++)
	{
		x += (backbottomwidth);
		if (x >= vid.width)
			break;
		V_DrawScaledPatch(x, y, V_SNAPTOBOTTOM|V_SNAPTOLEFT, backbottomfg);
	}

	// front bottom foreground patch
	x = 0-((ntsatkdrawtimer*2)%frontbottomwidth);
	y = BASEVIDHEIGHT - frontbottomheight;
	V_DrawScaledPatch(x, y, V_SNAPTOBOTTOM|V_SNAPTOLEFT, frontbottomfg);
	for (i = 0; i < 3; i++)
	{
		x += (frontbottomwidth);
		if (x >= vid.width)
			break;
		V_DrawScaledPatch(x, y, V_SNAPTOBOTTOM|V_SNAPTOLEFT, frontbottomfg);
	}

	// Increment timer.
	ntsatkdrawtimer++;
}

// NiGHTS Attack floating Super Sonic.
static patch_t *ntssupersonic[2];
static void M_DrawNightsAttackSuperSonic(void)
{
	const UINT8 *colormap = R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_YELLOW, GTC_CACHE);
	INT32 timer = (ntsatkdrawtimer/4) % 2;
	angle_t fa = (FixedAngle(((ntsatkdrawtimer * 4) % 360)<<FRACBITS)>>ANGLETOFINESHIFT) & FINEMASK;
	V_DrawFixedPatch(235<<FRACBITS, (120<<FRACBITS) - (8*FINESINE(fa)), FRACUNIT, 0, ntssupersonic[timer], colormap);
}

static void M_DrawLevelPlatterMenu(void)
{
	UINT8 iter = lsrow, sizeselect = (lswide(lsrow) ? 1 : 0);
	INT32 y = lsbasey + lsoffs[0] - getheadingoffset(lsrow);
	const INT32 cursorx = (sizeselect ? 0 : (lscol*lshseperation));

	if (currentMenu->prevMenu == &SP_TimeAttackDef)
	{
		M_SetMenuCurBackground("RECATKBG");

		curbgxspeed = 0;
		curbgyspeed = 18;

		if (curbgcolor >= 0)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, curbgcolor);
		else if (!curbghide || !titlemapinaction)
		{
			F_SkyScroll(curbgxspeed, curbgyspeed, curbgname);
			// Draw and animate foreground
			if (!strncmp("RECATKBG", curbgname, 8))
				M_DrawRecordAttackForeground();
		}

		if (curfadevalue)
			V_DrawFadeScreen(0xFF00, curfadevalue);
	}

	if (currentMenu->prevMenu == &SP_NightsAttackDef)
	{
		M_SetMenuCurBackground("NTSATKBG");

		if (curbgcolor >= 0)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, curbgcolor);
		else if (!curbghide || !titlemapinaction)
		{
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 158);
			M_DrawNightsAttackMountains();
		}
		if (curfadevalue)
			V_DrawFadeScreen(0xFF00, curfadevalue);
	}

	// finds row at top of the screen
	while (y > -8)
	{
		if (iter == 0)
		{
			if (levelselect.numrows < 3)
				break;
			iter = levelselect.numrows;
		}
		iter--;
		y -= lsvseperation(iter);
	}

	// draw from top to bottom
	while (y < (vid.height/vid.dupy))
	{
		M_DrawLevelPlatterRow(iter, y);
		y += lsvseperation(iter);
		if (iter == levelselect.numrows-1)
		{
			if (levelselect.numrows < 3)
				break;
			iter = UINT8_MAX;
		}
		iter++;
	}

	// draw cursor box
	if (levellistmode != LLM_CREATESERVER || lsrow)
		V_DrawSmallScaledPatch(lsbasex + cursorx + lsoffs[1], lsbasey+lsoffs[0], 0, (levselp[sizeselect][((skullAnimCounter/4) ? 1 : 0)]));

#if 0
	if (levelselect.rows[lsrow].maplist[lscol] > 0)
		V_DrawScaledPatch(lsbasex + cursorx-17, lsbasey+50+lsoffs[0], 0, W_CachePatchName("M_CURSOR", PU_PATCH));
#endif

	// handle movement of cursor box
	if (lsoffs[0] > 1 || lsoffs[0] < -1)
		lsoffs[0] = 2*lsoffs[0]/3;
	else
		lsoffs[0] = 0;

	if (lsoffs[1] > 1 || lsoffs[1] < -1)
		lsoffs[1] = 2*lsoffs[1]/3;
	else
		lsoffs[1] = 0;

	M_DrawMenuTitle();
}

//
// M_CanShowLevelInList
//
// Determines whether to show a given map in level-select lists where you don't want to see locked levels.
// Set gt = -1 to ignore gametype.
//
boolean M_CanShowLevelInList(INT32 mapnum, INT32 gt)
{
	return (M_CanShowLevelOnPlatter(mapnum, gt) && M_LevelAvailableOnPlatter(mapnum));
}

static INT32 M_GetFirstLevelInList(INT32 gt)
{
	INT32 mapnum;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, gt))
			return mapnum + 1;

	return 1;
}

// ==================================================
// MESSAGE BOX (aka: a hacked, cobbled together menu)
// ==================================================
static void M_DrawMessageMenu(void);

// Because this is just a hack-ish 'menu', I'm not putting this with the others
static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0,NULL, NULL, NULL,0}
};

menu_t MessageDef =
{
	MN_SPECIAL,
	NULL,               // title
	1,                  // # of menu items
	NULL,               // previous menu       (TO HACK)
	MessageMenu,        // menuitem_t ->
	M_DrawMessageMenu,  // drawing routine ->
	0, 0,               // x, y                (TO HACK)
	0,                  // lastOn, flags       (TO HACK)
	NULL
};


void M_StartMessage(const char *string, void *routine,
	menumessagetype_t itemtype)
{
	size_t max = 0, start = 0, i, strlines;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	// Rudementary word wrapping.
	// Simple and effective. Does not handle nonuniform letter sizes, colors, etc. but who cares.
	strlines = 0;
	for (i = 0; message[i]; i++)
	{
		if (message[i] == ' ')
		{
			start = i;
			max += 4;
		}
		else if (message[i] == '\n')
		{
			strlines = i;
			start = 0;
			max = 0;
			continue;
		}
		else
			max += 8;

		// Start trying to wrap if presumed length exceeds the screen width.
		if (max >= BASEVIDWIDTH && start > 0)
		{
			message[start] = '\n';
			max -= (start-strlines)*8;
			strlines = start;
			start = 0;
		}
	}

	start = 0;
	max = 0;

	M_StartControlPanel(); // can't put menuactive to true

	if (currentMenu == &MessageDef) // Prevent recursion
		MessageDef.prevMenu = &MainDef;
	else
		MessageDef.prevMenu = currentMenu;

	MessageDef.menuitems[0].text     = message;
	MessageDef.menuitems[0].alphaKey = (UINT8)itemtype;
	if (!routine && itemtype != MM_NOTHING) itemtype = MM_NOTHING;
	switch (itemtype)
	{
		case MM_NOTHING:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = M_StopMessage;
			break;
		case MM_YESNO:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
		case MM_EVENTHANDLER:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
	}
	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i;
				i = (size_t)-1; //added : 07-02-98 : damned!
				start++;
				break;
			}
		}

		if (i == strlen(message+start))
			start += i;
	}

	MessageDef.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	MessageDef.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	MessageDef.lastOn = (INT16)((strlines<<8)+max);

	//M_SetupNextMenu();
	currentMenu = &MessageDef;
	itemOn = 0;
}

#define MAXMSGLINELEN 256

static void M_DrawMessageMenu(void)
{
	INT32 y = currentMenu->y;
	size_t i, start = 0;
	INT16 max;
	char string[MAXMSGLINELEN];
	INT32 mlines;
	const char *msg = currentMenu->menuitems[0].text;

	mlines = currentMenu->lastOn>>8;
	max = (INT16)((UINT8)(currentMenu->lastOn & 0xFF)*8);

	// hack: draw RA background in RA menus
	if (gamestate == GS_TIMEATTACK)
	{
		if (curbgcolor >= 0)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, curbgcolor);
		else if (!curbghide || !titlemapinaction)
		{
			if (levellistmode == LLM_NIGHTSATTACK)
			{
				V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 158);
				M_DrawNightsAttackMountains();
			}
			else
			{
				F_SkyScroll(curbgxspeed, curbgyspeed, curbgname);
				if (!strncmp("RECATKBG", curbgname, 8))
					M_DrawRecordAttackForeground();
			}
		}
		if (curfadevalue)
			V_DrawFadeScreen(0xFF00, curfadevalue);
	}

	M_DrawTextBox(currentMenu->x, y - 8, (max+7)>>3, mlines);

	while (*(msg+start))
	{
		size_t len = strlen(msg+start);

		for (i = 0; i < len; i++)
		{
			if (*(msg+start+i) == '\n')
			{
				memset(string, 0, MAXMSGLINELEN);
				if (i >= MAXMSGLINELEN)
				{
					CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
					return;
				}
				else
				{
					strncpy(string,msg+start, i);
					string[i] = '\0';
					start += i;
					i = (size_t)-1; //added : 07-02-98 : damned!
					start++;
				}
				break;
			}
		}

		if (i == strlen(msg+start))
		{
			if (i >= MAXMSGLINELEN)
			{
				CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
				return;
			}
			else
			{
				strcpy(string, msg + start);
				start += i;
			}
		}

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string, 0))/2,y,V_ALLOWLOWERCASE,string);
		y += 8; //hu_font[0]->height;
	}
}

// default message handler
static void M_StopMessage(INT32 choice)
{
	(void)choice;
	if (menuactive)
		M_SetupNextMenu(MessageDef.prevMenu);
}

// =========
// IMAGEDEFS
// =========

// Draw an Image Def.  Aka, Help images.
// Defines what image is used in (menuitem_t)->text.
// You can even put multiple images in one menu!
static void M_DrawImageDef(void)
{
	// Grr.  Need to autodetect for pic_ts.
	pic_t *pictest = (pic_t *)W_CacheLumpName(currentMenu->menuitems[itemOn].text,PU_CACHE);
	if (!pictest->zero)
		V_DrawScaledPic(0,0,0,W_GetNumForName(currentMenu->menuitems[itemOn].text));
	else
	{
		patch_t *patch = W_CachePatchName(currentMenu->menuitems[itemOn].text,PU_PATCH);
		if (patch->width <= BASEVIDWIDTH)
			V_DrawScaledPatch(0,0,0,patch);
		else
			V_DrawSmallScaledPatch(0,0,0,patch);
	}

	if (currentMenu->numitems > 1)
		V_DrawString(0,192,V_TRANSLUCENT, va("PAGE %d of %hd", itemOn+1, currentMenu->numitems));
}

// Handles the ImageDefs.  Just a specialized function that
// uses left and right movement.
static void M_HandleImageDef(INT32 choice)
{
	switch (choice)
	{
		case KEY_RIGHTARROW:
			if (currentMenu->numitems == 1)
				break;

			S_StartSound(NULL, sfx_menu1);
			if (itemOn >= (INT16)(currentMenu->numitems-1))
				itemOn = 0;
            else itemOn++;
			break;

		case KEY_LEFTARROW:
			if (currentMenu->numitems == 1)
				break;

			S_StartSound(NULL, sfx_menu1);
			if (!itemOn)
				itemOn = currentMenu->numitems - 1;
			else itemOn--;
			break;

		case KEY_ESCAPE:
		case KEY_ENTER:
			M_ClearMenus(true);
			break;
	}
}

// ======================
// MISC MAIN MENU OPTIONS
// ======================

static void M_AddonsOptions(INT32 choice)
{
	(void)choice;
	Addons_option_Onchange();

	M_SetupNextMenu(&OP_AddonsOptionsDef);
}

#define LOCATIONSTRING1 "Visit \x83SRB2.ORG/ADDONS\x80 to get & make addons!"
//#define LOCATIONSTRING2 "Visit \x88SRB2.ORG/ADDONS\x80 to get & make addons!"

static void M_LoadAddonsPatches(void)
{
	addonsp[EXT_FOLDER] = W_CachePatchName("M_FFLDR", PU_PATCH);
	addonsp[EXT_UP] = W_CachePatchName("M_FBACK", PU_PATCH);
	addonsp[EXT_NORESULTS] = W_CachePatchName("M_FNOPE", PU_PATCH);
	addonsp[EXT_TXT] = W_CachePatchName("M_FTXT", PU_PATCH);
	addonsp[EXT_CFG] = W_CachePatchName("M_FCFG", PU_PATCH);
	addonsp[EXT_WAD] = W_CachePatchName("M_FWAD", PU_PATCH);
#ifdef USE_KART
	addonsp[EXT_KART] = W_CachePatchName("M_FKART", PU_PATCH);
#endif
	addonsp[EXT_PK3] = W_CachePatchName("M_FPK3", PU_PATCH);
	addonsp[EXT_SOC] = W_CachePatchName("M_FSOC", PU_PATCH);
	addonsp[EXT_LUA] = W_CachePatchName("M_FLUA", PU_PATCH);
	addonsp[NUM_EXT] = W_CachePatchName("M_FUNKN", PU_PATCH);
	addonsp[NUM_EXT+1] = W_CachePatchName("M_FSEL", PU_PATCH);
	addonsp[NUM_EXT+2] = W_CachePatchName("M_FLOAD", PU_PATCH);
	addonsp[NUM_EXT+3] = W_CachePatchName("M_FSRCH", PU_PATCH);
	addonsp[NUM_EXT+4] = W_CachePatchName("M_FSAVE", PU_PATCH);
}

static void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

	// If M_GetGameypeColor() is ever ported from Kart, then remove this.
	highlightflags = V_YELLOWMAP;
	recommendedflags = V_GREENMAP;
	warningflags = V_REDMAP;

#if 1
	if (cv_addons_option.value == 0)
		pathname = usehome ? srb2home : srb2path;
	else if (cv_addons_option.value == 1)
		pathname = srb2home;
	else if (cv_addons_option.value == 2)
		pathname = srb2path;
	else
#endif
	if (cv_addons_option.value == 3 && *cv_addons_folder.string != '\0')
		pathname = cv_addons_folder.string;

	strlcpy(menupath, pathname, 1024);
	menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath) + 1;

	if (menupath[menupathindex[menudepthleft]-2] != PATHSEP[0])
	{
		menupath[menupathindex[menudepthleft]-1] = PATHSEP[0];
		menupath[menupathindex[menudepthleft]] = 0;
	}
	else
		--menupathindex[menudepthleft];

	if (!preparefilemenu(false))
	{
		M_StartMessage(va("No files/folders found.\n\n%s\n\n(Press a key)\n",LOCATIONSTRING1),NULL,MM_NOTHING);
			// (recommendedflags == V_SKYMAP ? LOCATIONSTRING2 : LOCATIONSTRING1))
		return;
	}
	else
		dir_on[menudepthleft] = 0;

	M_LoadAddonsPatches();

	MISC_AddonsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_AddonsDef);
}

#define width 4
#define vpadding 27
#define h (BASEVIDHEIGHT-(2*vpadding))
#define NUMCOLOURS 8 // when toast's coding it's british english hacker fucker
static void M_DrawTemperature(INT32 x, fixed_t t)
{
	INT32 y;

	// bounds check
	if (t > FRACUNIT)
		t = FRACUNIT;
	/*else if (t < 0) -- not needed
		t = 0;*/

	// scale
	if (t > 1)
		t = (FixedMul(h<<FRACBITS, t)>>FRACBITS);

	// border
	V_DrawFill(x - 1, vpadding, 1, h, 3);
	V_DrawFill(x + width, vpadding, 1, h, 3);
	V_DrawFill(x - 1, vpadding-1, width+2, 1, 3);
	V_DrawFill(x - 1, vpadding+h, width+2, 1, 3);

	// bar itself
	y = h;
	if (t)
		for (t = h - t; y > 0; y--)
		{
			UINT8 colours[NUMCOLOURS] = {42, 40, 58, 222, 65, 90, 97, 98};
			UINT8 c;
			if (y <= t) break;
			if (y+vpadding >= BASEVIDHEIGHT/2)
				c = 113;
			else
				c = colours[(NUMCOLOURS*(y-1))/(h/2)];
			V_DrawFill(x, y-1 + vpadding, width, 1, c);
		}

	// fill the rest of the backing
	if (y)
		V_DrawFill(x, vpadding, width, y, 27);
}
#undef width
#undef vpadding
#undef h
#undef NUMCOLOURS

static char *M_AddonsHeaderPath(void)
{
	UINT32 len;
	static char header[1024];

	strlcpy(header, va("%s folder%s", cv_addons_option.string, menupath+menupathindex[menudepth-1]-1), 1024);
	len = strlen(header);
	if (len > 34)
	{
		len = len-34;
		header[len] = header[len+1] = header[len+2] = '.';
	}
	else
		len = 0;

	return header+len;
}

#define UNEXIST S_StartSound(NULL, sfx_lose);\
		M_SetupNextMenu(MISC_AddonsDef.prevMenu);\
		M_StartMessage(va("\x82%s\x80\nThis folder no longer exists!\nAborting to main menu.\n\n(Press a key)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING)

#define CLEARNAME Z_Free(refreshdirname);\
					refreshdirname = NULL

static void M_AddonsClearName(INT32 choice)
{
	(void)choice;
	CLEARNAME;
}

// returns whether to do message draw
static boolean M_AddonsRefresh(void)
{
	if ((refreshdirmenu & REFRESHDIR_NORMAL) && !preparefilemenu(true))
	{
		UNEXIST;
		return true;
	}

	if (refreshdirmenu & REFRESHDIR_ADDFILE)
	{
		char *message = NULL;

		if (refreshdirmenu & REFRESHDIR_NOTLOADED)
		{
			S_StartSound(NULL, sfx_lose);
			if (refreshdirmenu & REFRESHDIR_MAX)
				message = va("%c%s\x80\nMaximum number of add-ons reached.\nA file could not be loaded.\nIf you wish to play with this add-on, restart the game to clear existing ones.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			else
				message = va("%c%s\x80\nA file was not loaded.\nCheck the console log for more information.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
		}
		else if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_skid);
			message = va("%c%s\x80\nA file was loaded with %s.\nCheck the console log for more information.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings"));
		}

		if (message)
		{
			M_StartMessage(message,M_AddonsClearName,MM_NOTHING);
			return true;
		}

		S_StartSound(NULL, sfx_strpst);
		CLEARNAME;
	}

	return false;
}

static void M_DrawAddons(void)
{
	INT32 x, y;
	size_t i, m;
	size_t t, b; // top and bottom item #s to draw in directory
	const UINT8 *flashcol = NULL;
	UINT8 hilicol;

	// hack - need to refresh at end of frame to handle addfile...
	if (refreshdirmenu & M_AddonsRefresh())
	{
		M_DrawMessageMenu();
		return;
	}

	if (Playing())
		V_DrawCenteredString(BASEVIDWIDTH/2, 5, warningflags, "Adding files mid-game may cause problems.");
	else
		V_DrawCenteredString(BASEVIDWIDTH/2, 5, 0, LOCATIONSTRING1);
			// (recommendedflags == V_SKYMAP ? LOCATIONSTRING2 : LOCATIONSTRING1)

	if (numwadfiles <= mainwads+1)
		y = 0;
	else if (numwadfiles >= MAX_WADFILES)
		y = FRACUNIT;
	else
	{
		x = FixedDiv(((ssize_t)(numwadfiles) - (ssize_t)(mainwads+1))<<FRACBITS, ((ssize_t)MAX_WADFILES - (ssize_t)(mainwads+1))<<FRACBITS);
		y = FixedDiv((((ssize_t)packetsizetally-(ssize_t)mainwadstally)<<FRACBITS), ((((ssize_t)MAXFILENEEDED*sizeof(UINT8)-(ssize_t)mainwadstally)-(5+22))<<FRACBITS)); // 5+22 = (a.ext + checksum length) is minimum addition to packet size tally
		if (x > y)
			y = x;
		if (y > FRACUNIT) // happens because of how we're shrinkin' it a little
			y = FRACUNIT;
	}

	M_DrawTemperature(BASEVIDWIDTH - 19 - 5, y);

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y + 1;

	hilicol = 0; // white

#define boxwidth (MAXSTRINGLENGTH*8+6)

	// draw the file path and the top white + black lines of the box
	V_DrawString(x-21, (y - 16) + (lsheadingheight - 12), highlightflags|V_ALLOWLOWERCASE, M_AddonsHeaderPath());
	V_DrawFill(x-21, (y - 16) + (lsheadingheight - 3), boxwidth, 1, hilicol);
	V_DrawFill(x-21, (y - 16) + (lsheadingheight - 2), boxwidth, 1, 30);

	m = (BASEVIDHEIGHT - currentMenu->y + 2) - (y - 1);
	// addons menu back color
	V_DrawFill(x-21, y - 1, boxwidth, m, 159);

	// The directory is too small for a scrollbar, so just draw a tall white line
	if (sizedirmenu <= addonmenusize)
	{
		t = 0; // first item
		b = sizedirmenu - 1; // last item
		i = 0; // "scrollbar" at "top" position
	}
	else
	{
		size_t q = m;
		m = (addonmenusize * m)/sizedirmenu; // height of scroll bar
		if (dir_on[menudepthleft] <= numaddonsshown) // all the way up
		{
			t = 0; // first item
			b = addonmenusize - 1; //9th item
			i = 0; // scrollbar at top position
		}
		else if (dir_on[menudepthleft] >= sizedirmenu - (numaddonsshown + 1)) // all the way down
		{
			t = sizedirmenu - addonmenusize; // # 9th last
			b = sizedirmenu - 1; // last item
			i = q-m; // scrollbar at bottom position
		}
		else // somewhere in the middle
		{
			t = dir_on[menudepthleft] - numaddonsshown; // 4 items above
			b = dir_on[menudepthleft] + numaddonsshown; // 4 items below
			i = (t * (q-m))/(sizedirmenu - addonmenusize); // calculate position of scrollbar
		}
	}

	// draw the scrollbar!
	V_DrawFill((x-21) + boxwidth-1, (y - 1) + i, 1, m, hilicol);

#undef boxwidth

	// draw up arrow that bobs up and down
	if (t != 0)
		V_DrawString(19, y+4 - (skullAnimCounter/5), highlightflags, "\x1A");

	// make the selection box flash yellow
	if (skullAnimCounter < 4)
		flashcol = V_GetStringColormap(highlightflags);

	// draw icons and item names
	for (i = t; i <= b; i++)
	{
		UINT32 flags = V_ALLOWLOWERCASE;
		if (y > BASEVIDHEIGHT) break;
		if (dirmenu[i])
#define type (UINT8)(dirmenu[i][DIR_TYPE])
		{
			if (type & EXT_LOADED)
			{
				flags |= V_TRANSLUCENT;
				V_DrawSmallScaledPatch(x-(16+4), y, V_TRANSLUCENT, addonsp[(type & ~EXT_LOADED)]);
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[NUM_EXT+2]);
			}
			else
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[(type & ~EXT_LOADED)]);

			// draw selection box for the item currently selected
			if ((size_t)i == dir_on[menudepthleft])
			{
				V_DrawFixedPatch((x-(16+4))<<FRACBITS, (y)<<FRACBITS, FRACUNIT/2, 0, addonsp[NUM_EXT+1], flashcol);
				flags = V_ALLOWLOWERCASE|highlightflags;
			}

			// draw name of the item, use ... if too long
#define charsonside 14
			if (dirmenu[i][DIR_LEN] > (charsonside*2 + 3))
				V_DrawString(x, y+4, flags, va("%.*s...%s", charsonside, dirmenu[i]+DIR_STRING, dirmenu[i]+DIR_STRING+dirmenu[i][DIR_LEN]-(charsonside+1)));
#undef charsonside
			else
				V_DrawString(x, y+4, flags, dirmenu[i]+DIR_STRING);
		}
#undef type
		y += 16;
	}

	// draw down arrow that bobs down and up
	if (b != sizedirmenu)
		V_DrawString(19, y-12 + (skullAnimCounter/5), highlightflags, "\x1B");

	// draw search box
	y = BASEVIDHEIGHT - currentMenu->y + 1;

	M_DrawTextBox(x - (21 + 5), y, MAXSTRINGLENGTH, 1);
	if (menusearch[0])
		V_DrawString(x - 18, y + 8, V_ALLOWLOWERCASE, menusearch+1);
	else
		V_DrawString(x - 18, y + 8, V_ALLOWLOWERCASE|V_TRANSLUCENT, "Type to search...");
	if (skullAnimCounter < 4)
		V_DrawCharacter(x - 18 + V_StringWidth(menusearch+1, 0), y + 8,
			'_' | 0x80, false);

	// draw search icon
	x -= (21 + 5 + 16);
	V_DrawSmallScaledPatch(x, y + 4, (menusearch[0] ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+3]);

	// draw save icon
	x = BASEVIDWIDTH - x - 16;
	V_DrawSmallScaledPatch(x, y + 4, ((!modifiedgame || savemoddata) ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+4]);

	if (modifiedgame)
		V_DrawSmallScaledPatch(x, y + 4, 0, addonsp[NUM_EXT+2]);
}

static void M_AddonExec(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	S_StartSound(NULL, sfx_zoom);
	COM_BufAddText(va("exec \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
}

#define len menusearch[0]
static boolean M_ChangeStringAddons(INT32 choice)
{
	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_DEL:
			if (len)
			{
				len = menusearch[1] = 0;
				return true;
			}
			break;
		case KEY_BACKSPACE:
			if (len)
			{
				menusearch[1+--len] = 0;
				return true;
			}
			break;
		default:
			if (choice >= 32 && choice <= 127)
			{
				if (len < MAXSTRINGLENGTH - 1)
				{
					menusearch[1+len++] = (char)choice;
					menusearch[1+len] = 0;
					return true;
				}
			}
			break;
	}
	return false;
}
#undef len

static void M_HandleAddons(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	if (M_ChangeStringAddons(choice))
	{
		char *tempname = NULL;
		if (dirmenu && dirmenu[dir_on[menudepthleft]])
			tempname = Z_StrDup(dirmenu[dir_on[menudepthleft]]+DIR_STRING); // don't need to I_Error if can't make - not important, just QoL
#if 0 // much slower
		if (!preparefilemenu(true))
		{
			UNEXIST;
			return;
		}
#else // streamlined
		searchfilemenu(tempname);
#endif
	}

	switch (choice)
	{
		case KEY_DOWNARROW:
			if (dir_on[menudepthleft] < sizedirmenu-1)
				dir_on[menudepthleft]++;
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_UPARROW:
			if (dir_on[menudepthleft])
				dir_on[menudepthleft]--;
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_PGDN:
			{
				UINT8 i;
				for (i = numaddonsshown; i && (dir_on[menudepthleft] < sizedirmenu-1); i--)
					dir_on[menudepthleft]++;
			}
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_PGUP:
			{
				UINT8 i;
				for (i = numaddonsshown; i && (dir_on[menudepthleft]); i--)
					dir_on[menudepthleft]--;
			}
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_ENTER:
			{
				boolean refresh = true;
				if (!dirmenu[dir_on[menudepthleft]])
					S_StartSound(NULL, sfx_lose);
				else
				{
					switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
					{
						case EXT_FOLDER:
							strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
							if (menudepthleft)
							{
								menupathindex[--menudepthleft] = strlen(menupath);
								menupath[menupathindex[menudepthleft]] = 0;

								if (!preparefilemenu(false))
								{
									S_StartSound(NULL, sfx_skid);
									M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
									menupath[menupathindex[++menudepthleft]] = 0;

									if (!preparefilemenu(true))
									{
										UNEXIST;
										return;
									}
								}
								else
								{
									S_StartSound(NULL, sfx_menu1);
									dir_on[menudepthleft] = 1;
								}
								refresh = false;
							}
							else
							{
								S_StartSound(NULL, sfx_lose);
								M_StartMessage(va("%c%s\x80\nThis folder is too deep to navigate to!\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
								menupath[menupathindex[menudepthleft]] = 0;
							}
							break;
						case EXT_UP:
							S_StartSound(NULL, sfx_menu1);
							menupath[menupathindex[++menudepthleft]] = 0;
							if (!preparefilemenu(false))
							{
								UNEXIST;
								return;
							}
							break;
						case EXT_TXT:
							M_StartMessage(va("%c%s\x80\nThis file may not be a console script.\nAttempt to run anyways? \n\n(Press 'Y' to confirm)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),M_AddonExec,MM_YESNO);
							break;
						case EXT_CFG:
							M_AddonExec(KEY_ENTER);
							break;
						case EXT_LUA:
						case EXT_SOC:
						case EXT_WAD:
#ifdef USE_KART
						case EXT_KART:
#endif
						case EXT_PK3:
							COM_BufAddText(va("addfile \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
							break;
						default:
							S_StartSound(NULL, sfx_lose);
					}
				}
				if (refresh)
					refreshdirmenu |= REFRESHDIR_NORMAL;
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		closefilemenu(true);

		// secrets disabled by addfile...
		MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

static void M_PandorasBox(INT32 choice)
{
	(void)choice;
	if (maptol & TOL_NIGHTS)
		CV_StealthSetValue(&cv_dummyrings, max(players[consoleplayer].spheres, 0));
	else
		CV_StealthSetValue(&cv_dummyrings, max(players[consoleplayer].rings, 0));
	if (players[consoleplayer].lives == INFLIVES)
		CV_StealthSet(&cv_dummylives, "Infinite");
	else
		CV_StealthSetValue(&cv_dummylives, max(players[consoleplayer].lives, 1));
	CV_StealthSetValue(&cv_dummycontinues, players[consoleplayer].continues);
	SR_PandorasBox[3].status = (continuesInSession) ? (IT_STRING | IT_CVAR) : (IT_GRAYEDOUT);
	SR_PandorasBox[6].status = (players[consoleplayer].charflags & SF_SUPER) ? (IT_GRAYEDOUT) : (IT_STRING | IT_CALL);
	SR_PandorasBox[7].status = (emeralds == ((EMERALD7)*2)-1) ? (IT_GRAYEDOUT) : (IT_STRING | IT_CALL);
	M_SetupNextMenu(&SR_PandoraDef);
}

static boolean M_ExitPandorasBox(void)
{
	if (cv_dummyrings.value != max(players[consoleplayer].rings, 0))
	{
		if (maptol & TOL_NIGHTS)
			COM_ImmedExecute(va("setspheres %d", cv_dummyrings.value));
		else
			COM_ImmedExecute(va("setrings %d", cv_dummyrings.value));
	}
	if (cv_dummylives.value != players[consoleplayer].lives)
		COM_ImmedExecute(va("setlives %d", cv_dummylives.value));
	if (continuesInSession && cv_dummycontinues.value != players[consoleplayer].continues)
		COM_ImmedExecute(va("setcontinues %d", cv_dummycontinues.value));
	return true;
}

static void M_ChangeLevel(INT32 choice)
{
	char mapname[6];
	(void)choice;

	strlcpy(mapname, G_BuildMapName(cv_nextmap.value), sizeof (mapname));
	strlwr(mapname);
	mapname[5] = '\0';

	M_ClearMenus(true);
	COM_BufAddText(va("map %s -gametype \"%s\"\n", mapname, cv_newgametype.string));
}

static void M_ConfirmSpectate(INT32 choice)
{
	(void)choice;
	// We allow switching to spectator even if team changing is not allowed
	M_ClearMenus(true);
	COM_ImmedExecute("changeteam spectator");
}

static void M_ConfirmEnterGame(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value)
	{
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\nPress a key.\n"), NULL, MM_NOTHING);
		return;
	}
	M_ClearMenus(true);
	COM_ImmedExecute("changeteam playing");
}

static void M_ConfirmTeamScramble(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);

	switch (cv_dummyscramble.value)
	{
		case 0:
			COM_ImmedExecute("teamscramble 1");
			break;
		case 1:
			COM_ImmedExecute("teamscramble 2");
			break;
	}
}

static void M_ConfirmTeamChange(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value && cv_dummyteam.value)
	{
		M_StartMessage(M_GetText("The server is not allowing\nteam changes at this time.\nPress a key.\n"), NULL, MM_NOTHING);
		return;
	}

	M_ClearMenus(true);

	switch (cv_dummyteam.value)
	{
		case 0:
			COM_ImmedExecute("changeteam spectator");
			break;
		case 1:
			COM_ImmedExecute("changeteam red");
			break;
		case 2:
			COM_ImmedExecute("changeteam blue");
			break;
	}
}

static void M_Options(INT32 choice)
{
	(void)choice;

	// if the player is not admin or server, disable server options
	OP_MainMenu[5].status = (Playing() && !(server || IsPlayerAdmin(consoleplayer))) ? (IT_GRAYEDOUT) : (IT_STRING|IT_CALL);

	// if the player is playing _at all_, disable the erase data options
	OP_DataOptionsMenu[2].status = (Playing()) ? (IT_GRAYEDOUT) : (IT_STRING|IT_SUBMENU);

	OP_MainDef.prevMenu = currentMenu;
	M_SetupNextMenu(&OP_MainDef);
}

static void M_RetryResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	if (!&players[consoleplayer] || netgame || multiplayer) // Should never happen!
		return;

	M_ClearMenus(true);
	G_SetRetryFlag();
}

static void M_Retry(INT32 choice)
{
	(void)choice;
	if (marathonmode)
	{
		M_RetryResponse(KEY_ENTER);
		return;
	}
	M_StartMessage(M_GetText("Retry this act from the last starpost?\n\n(Press 'Y' to confirm)\n"),M_RetryResponse,MM_YESNO);
}

static void M_SelectableClearMenus(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);
}

// ======
// CHEATS
// ======

static void M_UltimateCheat(INT32 choice)
{
	(void)choice;
	LUAh_GameQuit(true);
	I_Quit();
}

static void M_AllowSuper(INT32 choice)
{
	(void)choice;

	players[consoleplayer].charflags |= SF_SUPER;
	M_StartMessage(M_GetText("You are now capable of turning super.\nRemember to get all the emeralds!\n"),NULL,MM_NOTHING);
	SR_PandorasBox[6].status = IT_GRAYEDOUT;

	G_SetGameModified(multiplayer);
}

static void M_GetAllEmeralds(INT32 choice)
{
	(void)choice;

	emeralds = ((EMERALD7)*2)-1;
	M_StartMessage(M_GetText("You now have all 7 emeralds.\nUse them wisely.\nWith great power comes great ring drain.\n"),NULL,MM_NOTHING);
	SR_PandorasBox[7].status = IT_GRAYEDOUT;

	G_SetGameModified(multiplayer);
}

static void M_DestroyRobotsResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Destroy all robots
	P_DestroyRobots();

	G_SetGameModified(multiplayer);
}

static void M_DestroyRobots(INT32 choice)
{
	(void)choice;

	M_StartMessage(M_GetText("Do you want to destroy all\nrobots in the current level?\n\n(Press 'Y' to confirm)\n"),M_DestroyRobotsResponse,MM_YESNO);
}

static void M_LevelSelectWarp(INT32 choice)
{
	boolean fromloadgame = (currentMenu == &SP_LevelSelectDef);

	(void)choice;

	if (W_CheckNumForName(G_BuildMapName(cv_nextmap.value)) == LUMPERROR)
	{
		CONS_Alert(CONS_WARNING, "Internal game map '%s' not found\n", G_BuildMapName(cv_nextmap.value));
		return;
	}

	startmap = (INT16)(cv_nextmap.value);

	fromlevelselect = true;

	if (fromloadgame)
		G_LoadGame((UINT32)cursaveslot, startmap);
	else
	{
		cursaveslot = 0;
		M_SetupChoosePlayer(0);
	}
}

// ========
// SKY ROOM
// ========

UINT8 skyRoomMenuTranslations[MAXUNLOCKABLES];

static boolean checklist_cangodown; // uuuueeerggghhhh HACK

static void M_HandleChecklist(INT32 choice)
{
	INT32 j;
	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			if ((check_on != MAXUNLOCKABLES) && checklist_cangodown)
			{
				for (j = check_on+1; j < MAXUNLOCKABLES; j++)
				{
					if (!unlockables[j].name[0])
						continue;
					// if (unlockables[j].nochecklist)
					//	continue;
					if (!unlockables[j].conditionset)
						continue;
					if (unlockables[j].conditionset > MAXCONDITIONSETS)
						continue;
					if (!unlockables[j].unlocked && unlockables[j].showconditionset && !M_Achieved(unlockables[j].showconditionset))
						continue;
					if (unlockables[j].conditionset == unlockables[check_on].conditionset)
						continue;
					break;
				}
				if (j != MAXUNLOCKABLES)
					check_on = j;
			}
			return;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			if (check_on)
			{
				for (j = check_on-1; j > -1; j--)
				{
					if (!unlockables[j].name[0])
						continue;
					// if (unlockables[j].nochecklist)
					//	continue;
					if (!unlockables[j].conditionset)
						continue;
					if (unlockables[j].conditionset > MAXCONDITIONSETS)
						continue;
					if (!unlockables[j].unlocked && unlockables[j].showconditionset && !M_Achieved(unlockables[j].showconditionset))
						continue;
					if (j && unlockables[j].conditionset == unlockables[j-1].conditionset)
						continue;
					break;
				}
				if (j != -1)
					check_on = j;
			}
			return;

		case KEY_ESCAPE:
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu);
			else
				M_ClearMenus(true);
			return;
		default:
			break;
	}
}

#define addy(add) { y += add; if ((y - currentMenu->y) > (scrollareaheight*2)) goto finishchecklist; }

static void M_DrawChecklist(void)
{
	INT32 i = check_on, j = 0, y = currentMenu->y;
	UINT32 condnum, previd, maxcond;
	condition_t *cond;

	// draw title (or big pic)
	M_DrawMenuTitle();

	if (check_on)
		V_DrawString(10, y-(skullAnimCounter/5), V_YELLOWMAP, "\x1A");

	while (i < MAXUNLOCKABLES)
	{
		if (unlockables[i].name[0] == 0 //|| unlockables[i].nochecklist
		|| !unlockables[i].conditionset || unlockables[i].conditionset > MAXCONDITIONSETS
		|| (!unlockables[i].unlocked && unlockables[i].showconditionset && !M_Achieved(unlockables[i].showconditionset)))
		{
			i += 1;
			continue;
		}

		V_DrawString(currentMenu->x, y, ((unlockables[i].unlocked) ? V_GREENMAP : V_TRANSLUCENT)|V_ALLOWLOWERCASE, ((unlockables[i].unlocked || !unlockables[i].nochecklist) ? unlockables[i].name : M_CreateSecretMenuOption(unlockables[i].name)));

		for (j = i+1; j < MAXUNLOCKABLES; j++)
		{
			if (!(unlockables[j].name[0] == 0 //|| unlockables[j].nochecklist
			|| !unlockables[j].conditionset || unlockables[j].conditionset > MAXCONDITIONSETS))
				break;
		}
		if ((j != MAXUNLOCKABLES) && (unlockables[i].conditionset == unlockables[j].conditionset))
			addy(8)
		else
		{
			if ((maxcond = conditionSets[unlockables[i].conditionset-1].numconditions))
			{
				cond = conditionSets[unlockables[i].conditionset-1].condition;
				previd = cond[0].id;
				addy(2);

				if (unlockables[i].objective[0] != '/')
				{
					addy(16);
					V_DrawString(currentMenu->x, y-8,
						V_ALLOWLOWERCASE,
						va("\x1E %s", unlockables[i].objective));
					y -= 8;
				}
				else
				{
					for (condnum = 0; condnum < maxcond; condnum++)
					{
						const char *beat = "!";

						if (cond[condnum].id != previd)
						{
							addy(8);
							V_DrawString(currentMenu->x + 4, y, V_YELLOWMAP, "OR");
						}

						addy(8);

						switch (cond[condnum].type)
						{
							case UC_PLAYTIME:
								{
									UINT32 hours = G_TicsToHours(cond[condnum].requirement);
									UINT32 minutes = G_TicsToMinutes(cond[condnum].requirement, false);
									UINT32 seconds = G_TicsToSeconds(cond[condnum].requirement);

#define getplural(field) ((field == 1) ? "" : "s")
									if (hours)
									{
										if (minutes)
											beat = va("Play the game for %d hour%s %d minute%s", hours, getplural(hours), minutes, getplural(minutes));
										else
											beat = va("Play the game for %d hour%s", hours, getplural(hours));
									}
									else
									{
										if (minutes && seconds)
											beat = va("Play the game for %d minute%s %d second%s", minutes, getplural(minutes), seconds, getplural(seconds));
										else if (minutes)
											beat = va("Play the game for %d minute%s", minutes, getplural(minutes));
										else
											beat = va("Play the game for %d second%s", seconds, getplural(seconds));
									}
#undef getplural
								}
								break;
							case UC_MAPVISITED:
							case UC_MAPBEATEN:
							case UC_MAPALLEMERALDS:
							case UC_MAPULTIMATE:
							case UC_MAPPERFECT:
								{
									char *title = G_BuildMapTitle(cond[condnum].requirement);

									if (title)
									{
										const char *level = ((M_MapLocked(cond[condnum].requirement) || !((mapheaderinfo[cond[condnum].requirement-1]->menuflags & LF2_NOVISITNEEDED) || (mapvisited[cond[condnum].requirement-1] & MV_MAX))) ? M_CreateSecretMenuOption(title) : title);

										switch (cond[condnum].type)
										{
											case UC_MAPVISITED:
												beat = va("Visit %s", level);
												break;
											case UC_MAPALLEMERALDS:
												beat = va("Beat %s with all emeralds", level);
												break;
											case UC_MAPULTIMATE:
												beat = va("Beat %s in Ultimate mode", level);
												break;
											case UC_MAPPERFECT:
												beat = va("Get all rings in %s", level);
												break;
											case UC_MAPBEATEN:
											default:
												beat = va("Beat %s", level);
												break;
										}
										Z_Free(title);
									}
								}
								break;
							case UC_MAPSCORE:
							case UC_MAPTIME:
							case UC_MAPRINGS:
								{
									char *title = G_BuildMapTitle(cond[condnum].extrainfo1);

									if (title)
									{
										const char *level = ((M_MapLocked(cond[condnum].extrainfo1) || !((mapheaderinfo[cond[condnum].extrainfo1-1]->menuflags & LF2_NOVISITNEEDED) || (mapvisited[cond[condnum].extrainfo1-1] & MV_MAX))) ? M_CreateSecretMenuOption(title) : title);

										switch (cond[condnum].type)
										{
											case UC_MAPSCORE:
												beat = va("Get %d points in %s", cond[condnum].requirement, level);
												break;
											case UC_MAPTIME:
												beat = va("Beat %s in %d:%02d.%02d", level,
												G_TicsToMinutes(cond[condnum].requirement, true),
												G_TicsToSeconds(cond[condnum].requirement),
												G_TicsToCentiseconds(cond[condnum].requirement));
												break;
											case UC_MAPRINGS:
												beat = va("Get %d rings in %s", cond[condnum].requirement, level);
												break;
											default:
												break;
										}
										Z_Free(title);
									}
								}
								break;
							case UC_OVERALLSCORE:
							case UC_OVERALLTIME:
							case UC_OVERALLRINGS:
								{
									switch (cond[condnum].type)
									{
										case UC_OVERALLSCORE:
											beat = va("Get %d points over all maps", cond[condnum].requirement);
											break;
										case UC_OVERALLTIME:
											beat = va("Get a total time of less than %d:%02d.%02d",
											G_TicsToMinutes(cond[condnum].requirement, true),
											G_TicsToSeconds(cond[condnum].requirement),
											G_TicsToCentiseconds(cond[condnum].requirement));
											break;
										case UC_OVERALLRINGS:
											beat = va("Get %d rings over all maps", cond[condnum].requirement);
											break;
										default:
											break;
									}
								}
								break;
							case UC_GAMECLEAR:
							case UC_ALLEMERALDS:
								{
									const char *emeraldtext = ((cond[condnum].type == UC_ALLEMERALDS) ? " with all emeralds" : "");
									if (cond[condnum].requirement != 1)
										beat = va("Beat the game %d times%s",
										cond[condnum].requirement, emeraldtext);
									else
										beat = va("Beat the game%s",
										emeraldtext);
								}
								break;
							case UC_TOTALEMBLEMS:
								beat = va("Collect %s%d emblems", ((numemblems+numextraemblems == cond[condnum].requirement) ? "all " : ""), cond[condnum].requirement);
								break;
							case UC_NIGHTSTIME:
							case UC_NIGHTSSCORE:
							case UC_NIGHTSGRADE:
								{
									char *title = G_BuildMapTitle(cond[condnum].extrainfo1);

									if (title)
									{
										const char *level = ((M_MapLocked(cond[condnum].extrainfo1) || !((mapheaderinfo[cond[condnum].extrainfo1-1]->menuflags & LF2_NOVISITNEEDED) || (mapvisited[cond[condnum].extrainfo1-1] & MV_MAX))) ? M_CreateSecretMenuOption(title) : title);

										switch (cond[condnum].type)
										{
											case UC_NIGHTSSCORE:
												if (cond[condnum].extrainfo2)
													beat = va("Get %d points in %s, mare %d", cond[condnum].requirement, level, cond[condnum].extrainfo2);
												else
													beat = va("Get %d points in %s", cond[condnum].requirement, level);
												break;
											case UC_NIGHTSTIME:
												if (cond[condnum].extrainfo2)
													beat = va("Beat %s, mare %d in %d:%02d.%02d", level, cond[condnum].extrainfo2,
													G_TicsToMinutes(cond[condnum].requirement, true),
													G_TicsToSeconds(cond[condnum].requirement),
													G_TicsToCentiseconds(cond[condnum].requirement));
												else
													beat = va("Beat %s in %d:%02d.%02d",
													level,
													G_TicsToMinutes(cond[condnum].requirement, true),
													G_TicsToSeconds(cond[condnum].requirement),
													G_TicsToCentiseconds(cond[condnum].requirement));
												break;
											case UC_NIGHTSGRADE:
												{
													char grade = ('F' - (char)cond[condnum].requirement);
													if (grade < 'A')
														grade = 'A';
													if (cond[condnum].extrainfo2)
														beat = va("Get grade %c in %s, mare %d", grade, level, cond[condnum].extrainfo2);
													else
														beat = va("Get grade %c in %s", grade, level);
												}
											break;
											default:
												break;
										}
										Z_Free(title);
									}
								}
								break;
							case UC_TRIGGER:
							case UC_EMBLEM:
							case UC_CONDITIONSET:
							default:
								y -= 8; // Nope, not showing this.
								break;
						}
						if (beat[0] != '!')
						{
							V_DrawString(currentMenu->x, y, 0, "\x1E");
							V_DrawString(currentMenu->x+12, y, V_ALLOWLOWERCASE, beat);
						}
						previd = cond[condnum].id;
					}
				}
			}
			addy(12);
		}
		i = j;

		/*V_DrawString(160, 8+(24*j), V_RETURN8, V_WordWrap(160, 292, 0, unlockables[i].objective));

		if (unlockables[i].unlocked)
			V_DrawString(308, 8+(24*j), V_YELLOWMAP, "Y");
		else
			V_DrawString(308, 8+(24*j), V_YELLOWMAP, "N");*/
	}

finishchecklist:
	if ((checklist_cangodown = ((y - currentMenu->y) > (scrollareaheight*2)))) // haaaaaaacks.
		V_DrawString(10, currentMenu->y+(scrollareaheight*2)+(skullAnimCounter/5), V_YELLOWMAP, "\x1B");
}

#define NUMHINTS 5

static void M_EmblemHints(INT32 choice)
{
	INT32 i;
	UINT32 local = 0;
	emblem_t *emblem;
	for (i = 0; i < numemblems; i++)
	{
		emblem = &emblemlocations[i];
		if (emblem->level != gamemap || emblem->type > ET_SKIN)
			continue;
		if (++local > NUMHINTS*2)
			break;
	}

	(void)choice;
	SR_EmblemHintMenu[0].status = (local > NUMHINTS*2) ? (IT_STRING | IT_ARROWS) : (IT_DISABLED);
	SR_EmblemHintMenu[1].status = (M_SecretUnlocked(SECRET_ITEMFINDER)) ? (IT_CVAR|IT_STRING) : (IT_SECRET);
	hintpage = 1;
	SR_EmblemHintDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SR_EmblemHintDef);
	itemOn = 2; // always start on back.
}

static void M_DrawEmblemHints(void)
{
	INT32 i, j = 0, x, y, left_hints = NUMHINTS, pageflag = 0;
	UINT32 collected = 0, totalemblems = 0, local = 0;
	emblem_t *emblem;
	const char *hint;

	for (i = 0; i < numemblems; i++)
	{
		emblem = &emblemlocations[i];
		if (emblem->level != gamemap || emblem->type > ET_SKIN)
			continue;

		local++;
	}

	x = (local > NUMHINTS ? 4 : 12);
	y = 8;

	if (local > NUMHINTS){
		if (local > ((hintpage-1)*NUMHINTS*2) && local < ((hintpage)*NUMHINTS*2)){
			if (NUMHINTS % 2 == 1)
				left_hints = (local - ((hintpage-1)*NUMHINTS*2)  + 1) / 2;
			else
				left_hints = (local - ((hintpage-1)*NUMHINTS*2)) / 2;
		}else{
			left_hints = NUMHINTS;
		}
	}

	if (local > NUMHINTS*2){
		if (itemOn == 0){
			pageflag = V_YELLOWMAP;
		}
		V_DrawString(currentMenu->x + 40, currentMenu->y + 10, pageflag, va("%d of %d",hintpage, local/(NUMHINTS*2) + 1));
	}

	// If there are more than 1 page's but less than 2 pages' worth of emblems on the last possible page,
	// put half (rounded up) of the hints on the left, and half (rounded down) on the right


	if (!local)
		V_DrawCenteredString(160, 48, V_YELLOWMAP, "No hidden emblems on this map.");
	else for (i = 0; i < numemblems; i++)
	{
		emblem = &emblemlocations[i];
		if (emblem->level != gamemap || emblem->type > ET_SKIN)
			continue;

		totalemblems++;

		if (totalemblems >= ((hintpage-1)*(NUMHINTS*2) + 1) && totalemblems < (hintpage*NUMHINTS*2)+1){

			if (emblem->collected)
			{
				collected = V_GREENMAP;
				V_DrawMappedPatch(x, y+4, 0, W_CachePatchName(M_GetEmblemPatch(emblem, false), PU_PATCH),
					R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
			}
			else
			{
				collected = 0;
				V_DrawScaledPatch(x, y+4, 0, W_CachePatchName("NEEDIT", PU_PATCH));
			}

			if (emblem->hint[0])
				hint = emblem->hint;
			else
				hint = M_GetText("No hint available for this emblem.");
			hint = V_WordWrap(40, BASEVIDWIDTH-12, 0, hint);
			//always draw tiny if we have more than NUMHINTS*2, visually more appealing
			if (local > NUMHINTS)
				V_DrawThinString(x+28, y, V_RETURN8|V_ALLOWLOWERCASE|collected, hint);
			else
				V_DrawString(x+28, y, V_RETURN8|V_ALLOWLOWERCASE|collected, hint);

			y += 28;

			// If there are more than 1 page's but less than 2 pages' worth of emblems on the last possible page,
			// put half (rounded up) of the hints on the left, and half (rounded down) on the right

			if (++j == left_hints)
			{
				x = 4+(BASEVIDWIDTH/2);
				y = 8;
			}
			else if (j >= NUMHINTS*2)
				break;
		}
	}

	M_DrawGenericMenu();
}


static void M_HandleEmblemHints(INT32 choice)
{
	INT32 i;
	emblem_t *emblem;
	UINT32 stageemblems = 0;

	for (i = 0; i < numemblems; i++)
	{
		emblem = &emblemlocations[i];
		if (emblem->level != gamemap || emblem->type > ET_SKIN)
			continue;

		stageemblems++;
	}


	if (choice == 0){
		if (hintpage > 1){
			hintpage--;
		}
	}else{
		if (hintpage < ((stageemblems-1)/(NUMHINTS*2) + 1)){
			hintpage++;
		}
	}

}

/*static void M_DrawSkyRoom(void)
{
	INT32 i, y = 0;

	M_DrawGenericMenu();

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		if (currentMenu->menuitems[i].status == (IT_STRING|IT_KEYHANDLER))
		{
			y = currentMenu->menuitems[i].alphaKey;
			break;
		}
	}
	if (!y)
		return;

	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + y, V_YELLOWMAP, cv_soundtest.string);
	if (i == itemOn)
	{
		V_DrawCharacter(BASEVIDWIDTH - currentMenu->x - 10 - V_StringWidth(cv_soundtest.string, 0) - (skullAnimCounter/5), currentMenu->y + y,
			'\x1C' | V_YELLOWMAP, false);
		V_DrawCharacter(BASEVIDWIDTH - currentMenu->x + 2 + (skullAnimCounter/5), currentMenu->y + y,
			'\x1D' | V_YELLOWMAP, false);
	}
	if (cv_soundtest.value)
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + y + 8, V_YELLOWMAP, S_sfx[cv_soundtest.value].name);
}*/

static musicdef_t *curplaying = NULL;
static INT32 st_sel = 0, st_cc = 0;
static tic_t st_time = 0;
static patch_t* st_radio[9];
static patch_t* st_launchpad[4];

static void M_CacheSoundTest(void)
{
	UINT8 i;
	char buf[8];

	STRBUFCPY(buf, "M_RADIOn");
	for (i = 0; i < 9; i++)
	{
		buf[7] = (char)('0'+i);
		st_radio[i] = W_CachePatchName(buf, PU_PATCH);
	}

	STRBUFCPY(buf, "M_LPADn");
	for (i = 0; i < 4; i++)
	{
		buf[6] = (char)('0'+i);
		st_launchpad[i] = W_CachePatchName(buf, PU_PATCH);
	}
}

static void M_SoundTest(INT32 choice)
{
	INT32 ul = skyRoomMenuTranslations[choice-1];

	soundtestpage = (UINT8)(unlockables[ul].variable);
	if (!soundtestpage)
		soundtestpage = 1;

	if (!S_PrepareSoundTest())
	{
		M_StartMessage(M_GetText("No selectable tracks found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_CacheSoundTest();

	curplaying = NULL;
	st_time = 0;

	st_sel = 0;

	st_cc = cv_closedcaptioning.value; // hack;
	cv_closedcaptioning.value = 1; // hack

	M_SetupNextMenu(&SR_SoundTestDef);
}

static void M_DrawSoundTest(void)
{
	INT32 x, y, i;
	fixed_t hscale = FRACUNIT/2, vscale = FRACUNIT/2, bounce = 0;
	UINT8 frame[4] = {0, 0, -1, SKINCOLOR_RUBY};

	// let's handle the ticker first. ideally we'd tick this somewhere else, BUT...
	if (curplaying)
	{
		if (curplaying == &soundtestsfx)
		{
			if (cv_soundtest.value)
			{
				frame[1] = (2-st_time);
				frame[2] = ((cv_soundtest.value - 1) % 9);
				frame[3] += (((cv_soundtest.value - 1) / 9) % (FIRSTSUPERCOLOR - frame[3]));
				if (st_time < 2)
					st_time++;
			}
		}
		else
		{
			if (curplaying->stoppingtics && st_time >= curplaying->stoppingtics)
			{
				curplaying = NULL;
				st_time = 0;
			}
			else
			{
				fixed_t work, bpm = curplaying->bpm;
				angle_t ang;
				//bpm = FixedDiv((60*TICRATE)<<FRACBITS, bpm); -- bake this in on load

				work = st_time<<FRACBITS;
				work %= bpm;

				if (st_time >= (FRACUNIT>>1)) // prevent overflow jump - takes about 15 minutes of loop on the same song to reach
					st_time = (work>>FRACBITS);

				work = FixedDiv(work*180, bpm);
				frame[0] = 8-(work/(20<<FRACBITS));
				if (frame[0] > 8) // VERY small likelihood for the above calculation to wrap, but it turns out it IS possible lmao
					frame[0] = 0;
				ang = (FixedAngle(work)>>ANGLETOFINESHIFT) & FINEMASK;
				bounce = (FINESINE(ang) - FRACUNIT/2);
				hscale -= bounce/16;
				vscale += bounce/16;

				st_time++;
			}
		}
	}

	x = 90<<FRACBITS;
	y = (BASEVIDHEIGHT-32)<<FRACBITS;

	V_DrawStretchyFixedPatch(x, y,
		hscale, vscale,
		0, st_radio[frame[0]], NULL);

	V_DrawFixedPatch(x, y, FRACUNIT/2, 0, st_launchpad[0], NULL);

	for (i = 0; i < 9; i++)
	{
		if (i == frame[2])
		{
			UINT8 *colmap = R_GetTranslationColormap(TC_RAINBOW, frame[3], GTC_CACHE);
			V_DrawFixedPatch(x, y + (frame[1]<<FRACBITS), FRACUNIT/2, 0, st_launchpad[frame[1]+1], colmap);
		}
		else
			V_DrawFixedPatch(x, y, FRACUNIT/2, 0, st_launchpad[1], NULL);

		if ((i % 3) == 2)
		{
			x -= ((2*28) + 25)<<(FRACBITS-1);
			y -= ((2*7) - 11)<<(FRACBITS-1);
		}
		else
		{
			x += 28<<(FRACBITS-1);
			y += 7<<(FRACBITS-1);
		}
	}

	y = (BASEVIDWIDTH-(vid.width/vid.dupx))/2;

	V_DrawFill(y, 20, vid.width/vid.dupx, 24, 159);
	{
		static fixed_t st_scroll = -1;
		const char* titl;
		x = 16;
		V_DrawString(x, 10, 0, "NOW PLAYING:");
		if (curplaying)
		{
			if (curplaying->alttitle[0])
				titl = va("%s - %s - ", curplaying->title, curplaying->alttitle);
			else
				titl = va("%s - ", curplaying->title);
		}
		else
			titl = "None - ";

		i = V_LevelNameWidth(titl);

		if (++st_scroll >= i)
			st_scroll %= i;

		x -= st_scroll;

		while (x < BASEVIDWIDTH-y)
			x += i;
		while (x > y)
		{
			x -= i;
			V_DrawLevelTitle(x, 22, 0, titl);
		}

		if (curplaying)
			V_DrawRightAlignedThinString(BASEVIDWIDTH-16, 46, V_ALLOWLOWERCASE, curplaying->authors);
	}

	V_DrawFill(165, 60, 140, 112, 159);

	{
		INT32 t, b, q, m = 112;

		if (numsoundtestdefs <= 7)
		{
			t = 0;
			b = numsoundtestdefs - 1;
			i = 0;
		}
		else
		{
			q = m;
			m = (5*m)/numsoundtestdefs;
			if (st_sel < 3)
			{
				t = 0;
				b = 6;
				i = 0;
			}
			else if (st_sel >= numsoundtestdefs-4)
			{
				t = numsoundtestdefs - 7;
				b = numsoundtestdefs - 1;
				i = q-m;
			}
			else
			{
				t = st_sel - 3;
				b = st_sel + 3;
				i = (t * (q-m))/(numsoundtestdefs - 7);
			}
		}

		V_DrawFill(165+140-1, 60 + i, 1, m, 0);

		if (t != 0)
			V_DrawString(165+140+4, 60+4 - (skullAnimCounter/5), V_YELLOWMAP, "\x1A");

		if (b != numsoundtestdefs - 1)
			V_DrawString(165+140+4, 60+112-12 + (skullAnimCounter/5), V_YELLOWMAP, "\x1B");

		x = 169;
		y = 64;

		while (t <= b)
		{
			if (t == st_sel)
				V_DrawFill(165, y-4, 140-1, 16, 155);
			if (!soundtestdefs[t]->allowed)
			{
				V_DrawString(x, y, (t == st_sel ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE, "???");
			}
			else if (soundtestdefs[t] == &soundtestsfx)
			{
				const char *sfxstr = va("SFX %s", cv_soundtest.string);
				V_DrawString(x, y, (t == st_sel ? V_YELLOWMAP : 0), sfxstr);
				if (t == st_sel)
				{
					V_DrawCharacter(x - 10 - (skullAnimCounter/5), y,
						'\x1C' | V_YELLOWMAP, false);
					V_DrawCharacter(x + 2 + V_StringWidth(sfxstr, 0) + (skullAnimCounter/5), y,
						'\x1D' | V_YELLOWMAP, false);
				}

				if (curplaying == soundtestdefs[t])
				{
					sfxstr = (cv_soundtest.value) ? S_sfx[cv_soundtest.value].name : "N/A";
					i = V_StringWidth(sfxstr, 0);
					V_DrawFill(165+140-9-i, y-4, i+8, 16, 150);
					V_DrawRightAlignedString(165+140-5, y, V_YELLOWMAP, sfxstr);
				}
			}
			else
			{
				V_DrawString(x, y, (t == st_sel ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE, soundtestdefs[t]->title);
				if (curplaying == soundtestdefs[t])
				{
					V_DrawFill(165+140-9, y-4, 8, 16, 150);
					//V_DrawCharacter(165+140-8, y, '\x19' | V_YELLOWMAP, false);
					V_DrawFixedPatch((165+140-9)<<FRACBITS, (y<<FRACBITS)-(bounce*4), FRACUNIT, 0, hu_font['\x19'-HU_FONTSTART], V_GetStringColormap(V_YELLOWMAP));
				}
			}
			t++;
			y += 16;
		}
	}
}

static void M_HandleSoundTest(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			if (st_sel++ >= numsoundtestdefs-1)
				st_sel = 0;
			{
				cv_closedcaptioning.value = st_cc; // hack
				S_StartSound(NULL, sfx_menu1);
				cv_closedcaptioning.value = 1; // hack
			}
			break;
		case KEY_UPARROW:
			if (!st_sel--)
				st_sel = numsoundtestdefs-1;
			{
				cv_closedcaptioning.value = st_cc; // hack
				S_StartSound(NULL, sfx_menu1);
				cv_closedcaptioning.value = 1; // hack
			}
			break;
		case KEY_PGDN:
			if (st_sel < numsoundtestdefs-1)
			{
				st_sel += 3;
				if (st_sel >= numsoundtestdefs-1)
					st_sel = numsoundtestdefs-1;
				cv_closedcaptioning.value = st_cc; // hack
				S_StartSound(NULL, sfx_menu1);
				cv_closedcaptioning.value = 1; // hack
			}
			break;
		case KEY_PGUP:
			if (st_sel)
			{
				st_sel -= 3;
				if (st_sel < 0)
					st_sel = 0;
				cv_closedcaptioning.value = st_cc; // hack
				S_StartSound(NULL, sfx_menu1);
				cv_closedcaptioning.value = 1; // hack
			}
			break;
		case KEY_BACKSPACE:
			if (curplaying)
			{
				S_StopSounds();
				S_StopMusic();
				curplaying = NULL;
				st_time = 0;
				cv_closedcaptioning.value = st_cc; // hack
				S_StartSound(NULL, sfx_skid);
				cv_closedcaptioning.value = 1; // hack
			}
			break;
		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_RIGHTARROW:
			if (soundtestdefs[st_sel] == &soundtestsfx && soundtestdefs[st_sel]->allowed)
			{
				S_StopSounds();
				S_StopMusic();
				curplaying = soundtestdefs[st_sel];
				st_time = 0;
				CV_AddValue(&cv_soundtest, 1);
			}
			break;
		case KEY_LEFTARROW:
			if (soundtestdefs[st_sel] == &soundtestsfx && soundtestdefs[st_sel]->allowed)
			{
				S_StopSounds();
				S_StopMusic();
				curplaying = soundtestdefs[st_sel];
				st_time = 0;
				CV_AddValue(&cv_soundtest, -1);
			}
			break;
		case KEY_ENTER:
			S_StopSounds();
			S_StopMusic();
			st_time = 0;
			if (soundtestdefs[st_sel]->allowed)
			{
				curplaying = soundtestdefs[st_sel];
				if (curplaying == &soundtestsfx)
				{
					// S_StopMusic() -- is this necessary?
					if (cv_soundtest.value)
						S_StartSound(NULL, cv_soundtest.value);
				}
				else
					S_ChangeMusicInternal(curplaying->name, !curplaying->stoppingtics);
			}
			else
			{
				curplaying = NULL;
				S_StartSound(NULL, sfx_lose);
			}
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		Z_Free(soundtestdefs);
		soundtestdefs = NULL;

		cv_closedcaptioning.value = st_cc; // undo hack

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// Entering secrets menu
static void M_SecretsMenu(INT32 choice)
{
	INT32 i, j, ul;
	UINT8 done[MAXUNLOCKABLES];
	UINT16 curheight;

	(void)choice;

	// Clear all before starting
	for (i = 1; i < MAXUNLOCKABLES+1; ++i)
		SR_MainMenu[i].status = IT_DISABLED;

	memset(skyRoomMenuTranslations, 0, sizeof(skyRoomMenuTranslations));
	memset(done, 0, sizeof(done));

	for (i = 1; i < MAXUNLOCKABLES+1; ++i)
	{
		curheight = UINT16_MAX;
		ul = -1;

		// Autosort unlockables
		for (j = 0; j < MAXUNLOCKABLES; ++j)
		{
			if (!unlockables[j].height || done[j] || unlockables[j].type < 0)
				continue;

			if (unlockables[j].height < curheight)
			{
				curheight = unlockables[j].height;
				ul = j;
			}
		}
		if (ul < 0)
			break;

		done[ul] = true;

		skyRoomMenuTranslations[i-1] = (UINT8)ul;
		SR_MainMenu[i].text = unlockables[ul].name;
		SR_MainMenu[i].alphaKey = (UINT16)unlockables[ul].height;

		if (unlockables[ul].type == SECRET_HEADER)
		{
			SR_MainMenu[i].status = IT_HEADER;
			continue;
		}

		SR_MainMenu[i].status = IT_SECRET;

		if (unlockables[ul].unlocked)
		{
			switch (unlockables[ul].type)
			{
				case SECRET_LEVELSELECT:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_CustomLevelSelect;
					break;
				case SECRET_WARP:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_CustomWarp;
					break;
				case SECRET_CREDITS:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_Credits;
					break;
				case SECRET_SOUNDTEST:
					SR_MainMenu[i].status = IT_STRING|IT_CALL;
					SR_MainMenu[i].itemaction = M_SoundTest;
				default:
					break;
			}
		}
	}

	M_SetupNextMenu(&SR_MainDef);
}

// ==================
// NEW GAME FUNCTIONS
// ==================

INT32 ultimate_selectable = false;

static void M_NewGame(void)
{
	fromlevelselect = false;

	startmap = spstage_start;
	CV_SetValue(&cv_newgametype, GT_COOP); // Graue 09-08-2004

	M_SetupChoosePlayer(0);
}

static void M_CustomWarp(INT32 choice)
{
	INT32 ul = skyRoomMenuTranslations[choice-1];

	startmap = (INT16)(unlockables[ul].variable);

	M_SetupChoosePlayer(0);
}

static void M_Credits(INT32 choice)
{
	(void)choice;
	cursaveslot = -1;
	M_ClearMenus(true);
	F_StartCredits();
}

static void M_CustomLevelSelect(INT32 choice)
{
	INT32 ul = skyRoomMenuTranslations[choice-1];

	SR_LevelSelectDef.prevMenu = currentMenu;
	levellistmode = LLM_LEVELSELECT;
	maplistoption = (UINT8)(unlockables[ul].variable);

	if (!M_PrepareLevelPlatter(-1, true))
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&SR_LevelSelectDef);
}

// ==================
// SINGLE PLAYER MENU
// ==================

static void M_SinglePlayerMenu(INT32 choice)
{
	(void)choice;


	// Reset the item positions, to avoid them sinking farther down every time the menu is opened if one is unavailable
	// Note that they're reset, not simply "not moved again", in case mid-game add-ons re-enable an option
	SP_MainMenu[spstartgame]   .alphaKey = 76;
	SP_MainMenu[sprecordattack].alphaKey = 84;
	SP_MainMenu[spnightsmode]  .alphaKey = 92;
	SP_MainMenu[spmarathon]    .alphaKey = 100;
	//SP_MainMenu[sptutorial]  .alphaKey = 108; // Not needed
	//SP_MainMenu[spstatistics].alphaKey = 116; // Not needed


	levellistmode = LLM_RECORDATTACK;
	if (M_GametypeHasLevels(-1))
		SP_MainMenu[sprecordattack].status = (M_SecretUnlocked(SECRET_RECORDATTACK)) ? IT_CALL|IT_STRING : IT_SECRET;
	else // If Record Attack is nonexistent in the current add-on...
	{
		SP_MainMenu[sprecordattack].status = IT_NOTHING|IT_DISABLED; // ...hide and disable the Record Attack option...
		SP_MainMenu[spstartgame].alphaKey += 8; // ...and lower Start Game by 8 pixels to close the gap
	}


	levellistmode = LLM_NIGHTSATTACK;
	if (M_GametypeHasLevels(-1))
		SP_MainMenu[spnightsmode].status = (M_SecretUnlocked(SECRET_NIGHTSMODE)) ? IT_CALL|IT_STRING : IT_SECRET;
	else // If NiGHTS Mode is nonexistent in the current add-on...
	{
		SP_MainMenu[spnightsmode].status = IT_NOTHING|IT_DISABLED; // ...hide and disable the NiGHTS Mode option...
		// ...and lower the above options' display positions by 8 pixels to close the gap
		SP_MainMenu[spstartgame]   .alphaKey += 8;
		SP_MainMenu[sprecordattack].alphaKey += 8;
	}


	// If the FIRST stage immediately leads to the ending, or itself (which gets converted to the title screen in G_DoCompleted for marathonmode only), there's no point in having this option on the menu. You should use Record Attack in that circumstance, although if marathonnext is set this behaviour can be overridden if you make some weird mod that requires multiple playthroughs of the same map in sequence and has some in-level mechanism to break the cycle.
	if (mapheaderinfo[spmarathon_start-1]
		&& !mapheaderinfo[spmarathon_start-1]->marathonnext
		&& (mapheaderinfo[spmarathon_start-1]->nextlevel == spmarathon_start
			|| mapheaderinfo[spmarathon_start-1]->nextlevel >= 1100))
	{
		SP_MainMenu[spmarathon].status = IT_NOTHING|IT_DISABLED; // Hide and disable the Marathon Run option...
		// ...and lower the above options' display positions by 8 pixels to close the gap
		SP_MainMenu[spstartgame]   .alphaKey += 8;
		SP_MainMenu[sprecordattack].alphaKey += 8;
		SP_MainMenu[spnightsmode]  .alphaKey += 8;
	}
	else // Otherwise, if Marathon Run is allowed and Record Attack is unlocked, unlock Marathon Run!
		SP_MainMenu[spmarathon].status = (M_SecretUnlocked(SECRET_RECORDATTACK)) ? IT_CALL|IT_STRING|IT_CALL_NOTMODIFIED : IT_SECRET;


	if (tutorialmap) // If there's a tutorial available in the current add-on...
		SP_MainMenu[sptutorial].status = IT_CALL | IT_STRING; // ...always unlock Tutorial
	else // But if there's no tutorial available in the current add-on...
	{
		SP_MainMenu[sptutorial].status = IT_NOTHING|IT_DISABLED; // ...hide and disable the Tutorial option...
		// ...and lower the above options' display positions by 8 pixels to close the gap
		SP_MainMenu[spstartgame]   .alphaKey += 8;
		SP_MainMenu[sprecordattack].alphaKey += 8;
		SP_MainMenu[spnightsmode]  .alphaKey += 8;
		SP_MainMenu[spmarathon]    .alphaKey += 8;
	}


	M_SetupNextMenu(&SP_MainDef);
}

static void M_LoadGameLevelSelect(INT32 choice)
{
	(void)choice;

	SP_LevelSelectDef.prevMenu = currentMenu;
	levellistmode = LLM_LEVELSELECT;
	maplistoption = 1+2;

	if (!M_PrepareLevelPlatter(-1, true))
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&SP_LevelSelectDef);
}

void M_TutorialSaveControlResponse(INT32 ch)
{
	if (ch == 'y' || ch == KEY_ENTER)
	{
		G_CopyControls(gamecontrol, gamecontroldefault[tutorialgcs], gcl_tutorial_full, num_gcl_tutorial_full);
		CV_Set(&cv_usemouse, cv_usemouse.defaultvalue);
		CV_Set(&cv_alwaysfreelook, cv_alwaysfreelook.defaultvalue);
		CV_Set(&cv_mousemove, cv_mousemove.defaultvalue);
		CV_Set(&cv_analog[0], cv_analog[0].defaultvalue);
		S_StartSound(NULL, sfx_itemup);
	}
	else
		S_StartSound(NULL, sfx_menu1);
}

static void M_TutorialControlResponse(INT32 ch)
{
	if (ch != KEY_ESCAPE)
	{
		G_CopyControls(gamecontroldefault[gcs_custom], gamecontrol, NULL, 0); // using gcs_custom as temp storage for old controls
		if (ch == 'y' || ch == KEY_ENTER)
		{
			tutorialgcs = gcs_fps;
			tutorialusemouse = cv_usemouse.value;
			tutorialfreelook = cv_alwaysfreelook.value;
			tutorialmousemove = cv_mousemove.value;
			tutorialanalog = cv_analog[0].value;

			G_CopyControls(gamecontrol, gamecontroldefault[tutorialgcs], gcl_tutorial_full, num_gcl_tutorial_full);
			CV_Set(&cv_usemouse, cv_usemouse.defaultvalue);
			CV_Set(&cv_alwaysfreelook, cv_alwaysfreelook.defaultvalue);
			CV_Set(&cv_mousemove, cv_mousemove.defaultvalue);
			CV_Set(&cv_analog[0], cv_analog[0].defaultvalue);

			//S_StartSound(NULL, sfx_itemup);
		}
		else
		{
			tutorialgcs = gcs_custom;
			S_StartSound(NULL, sfx_menu1);
		}
		M_StartTutorial(INT32_MAX);
	}
	else
		S_StartSound(NULL, sfx_menu1);

	MessageDef.prevMenu = &SP_MainDef; // if FirstPrompt -> ControlsPrompt -> ESC, we would go to the main menu unless we force this
}

// Starts up the tutorial immediately (tbh I wasn't sure where else to put this)
static void M_StartTutorial(INT32 choice)
{
	if (!tutorialmap)
		return; // no map to go to, don't bother

	if (choice != INT32_MAX && G_GetControlScheme(gamecontrol, gcl_tutorial_check, num_gcl_tutorial_check) != gcs_fps)
	{
		M_StartMessage("Do you want to try the \202recommended \202movement controls\x80?\n\nWe will set them just for this tutorial.\n\nPress 'Y' or 'Enter' to confirm\nPress 'N' or any key to keep \nyour current controls.\n",M_TutorialControlResponse,MM_YESNO);
		return;
	}
	else if (choice != INT32_MAX)
		tutorialgcs = gcs_custom;

	CV_SetValue(&cv_tutorialprompt, 0); // first-time prompt

	tutorialmode = true; // turn on tutorial mode

	emeralds = 0;
	memset(&luabanks, 0, sizeof(luabanks));
	M_ClearMenus(true);
	gamecomplete = 0;
	cursaveslot = 0;
	G_DeferedInitNew(false, G_BuildMapName(tutorialmap), 0, false, false);
}

// ==============
// LOAD GAME MENU
// ==============

static INT32 saveSlotSelected = 1;
static INT32 loadgamescroll = 0;
static UINT8 loadgameoffset = 0;

static void M_CacheLoadGameData(void)
{
	savselp[0] = W_CachePatchName("SAVEBACK", PU_PATCH);
	savselp[1] = W_CachePatchName("SAVENONE", PU_PATCH);
	savselp[2] = W_CachePatchName("ULTIMATE", PU_PATCH);

	savselp[3] = W_CachePatchName("GAMEDONE", PU_PATCH);
	savselp[4] = W_CachePatchName("BLACXLVL", PU_PATCH);
	savselp[5] = W_CachePatchName("BLANKLVL", PU_PATCH);
}

static void M_DrawLoadGameData(void)
{
	INT32 i, prev_i = 1, savetodraw, x, y, hsep = 90;
	skin_t *charskin = NULL;

	if (vid.width != BASEVIDWIDTH*vid.dupx)
		hsep = (hsep*vid.width)/(BASEVIDWIDTH*vid.dupx);

	for (i = 2; prev_i; i = -(i + ((UINT32)i >> 31))) // draws from outwards in; 2, -2, 1, -1, 0
	{
		prev_i = i;
		savetodraw = (saveSlotSelected + i + numsaves)%numsaves;
		x = (BASEVIDWIDTH/2 - 42 + loadgamescroll) + (i*hsep);
		y = 33 + 9;

		{
			INT32 diff = x - (BASEVIDWIDTH/2 - 42);
			if (diff < 0)
				diff = -diff;
			diff = (42 - diff)/3 - loadgameoffset;
			if (diff < 0)
				diff = 0;
			y -= diff;
		}

		if (savetodraw == 0)
		{
			V_DrawSmallScaledPatch(x, y, 0,
				savselp[((ultimate_selectable) ? 2 : 1)]);
			x += 2;
			y += 1;
			V_DrawString(x, y,
				((savetodraw == saveSlotSelected) ? V_YELLOWMAP : 0),
				"NO FILE");
			if (savetodraw == saveSlotSelected)
				V_DrawFill(x, y+9, 80, 1, yellowmap[3]);
			y += 11;
			V_DrawSmallScaledPatch(x, y, 0, savselp[4]);
			M_DrawStaticBox(x, y, V_80TRANS, 80, 50);
			y += 41;
			if (ultimate_selectable)
				V_DrawRightAlignedThinString(x + 79, y, V_REDMAP, "ULTIMATE.");
			else
				V_DrawRightAlignedThinString(x + 79, y, V_GRAYMAP, "DON'T SAVE!");

			continue;
		}

		savetodraw--;

		if (savegameinfo[savetodraw].lives > 0)
			charskin = &skins[savegameinfo[savetodraw].skinnum];

		// signpost background
		{
			UINT8 col;
			if (savegameinfo[savetodraw].lives == -666)
			{
				V_DrawSmallScaledPatch(x+2, y+64, 0, savselp[5]);
			}
#ifdef PERFECTSAVE // disabled on request
			else if ((savegameinfo[savetodraw].skinnum == 1)
			&& (savegameinfo[savetodraw].lives == 99)
			&& (savegameinfo[savetodraw].gamemap & 8192)
			&& (savegameinfo[savetodraw].numgameovers == 0)
			&& (savegameinfo[savetodraw].numemeralds == ((1<<7) - 1))) // perfect save
			{
				V_DrawFill(x+6, y+64, 72, 50, 134);
				V_DrawFill(x+6, y+74, 72, 30, 201);
				V_DrawFill(x+6, y+84, 72, 10, 1);
			}
#endif
			else
			{
				if (savegameinfo[savetodraw].lives == -42)
					col = 26;
				else if (savegameinfo[savetodraw].botskin == 3) // & knuckles
					col = 105;
				else if (savegameinfo[savetodraw].botskin) // tailsbot or custom
					col = 134;
				else
				{
					if (charskin->prefoppositecolor)
					{
						col = charskin->prefoppositecolor;
						col = skincolors[col].ramp[skincolors[skincolors[col].invcolor].invshade];
					}
					else
					{
						col = charskin->prefcolor;
						col = skincolors[skincolors[col].invcolor].ramp[skincolors[col].invshade];
					}
				}

				V_DrawFill(x+6, y+64, 72, 50, col);
			}
		}

		V_DrawSmallScaledPatch(x, y, 0, savselp[0]);
		x += 2;
		y += 1;
		V_DrawString(x, y,
			((savetodraw == saveSlotSelected-1) ? V_YELLOWMAP : 0),
			va("FILE %d", savetodraw+1));
		if (savetodraw == saveSlotSelected-1)
				V_DrawFill(x, y+9, 80, 1, yellowmap[3]);
		y += 11;

		// level image area
		{
			if ((savegameinfo[savetodraw].lives == -42)
			|| (savegameinfo[savetodraw].lives == -666))
			{
				V_DrawFill(x, y, 80, 50, 31);
				M_DrawStaticBox(x, y, V_80TRANS, 80, 50);
			}
			else
			{
				patch_t *patch;
				if (savegameinfo[savetodraw].gamemap & 8192)
					patch = savselp[3];
				else
				{
					lumpnum_t lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName((savegameinfo[savetodraw].gamemap) & 8191)));
					if (lumpnum != LUMPERROR)
						patch = W_CachePatchNum(lumpnum, PU_PATCH);
					else
						patch = savselp[5];
				}
				V_DrawSmallScaledPatch(x, y, 0, patch);
			}

			y += 41;

			if (savegameinfo[savetodraw].lives == -42)
				V_DrawRightAlignedThinString(x + 79, y, V_GRAYMAP, "NEW GAME");
			else if (savegameinfo[savetodraw].lives == -666)
				V_DrawRightAlignedThinString(x + 79, y, V_REDMAP, "CAN'T LOAD!");
			else if (savegameinfo[savetodraw].gamemap & 8192)
				V_DrawRightAlignedThinString(x + 79, y, V_GREENMAP, "CLEAR!");
			else
				V_DrawRightAlignedThinString(x + 79, y, V_YELLOWMAP, savegameinfo[savetodraw].levelname);
		}

		if (savegameinfo[savetodraw].lives == -42)
		{
			if (!useContinues)
				V_DrawRightAlignedThinString(x + 80, y+1+60+16, V_GRAYMAP, "00000000");
			continue;
		}

		if (savegameinfo[savetodraw].lives == -666)
		{
			if (!useContinues)
				V_DrawRightAlignedThinString(x + 80, y+1+60+16, V_REDMAP, "????????");
			continue;
		}

		y += 64;

		// tiny emeralds
		{
			INT32 j, workx = x + 6;
			for (j = 0; j < 7; ++j)
			{
				if (savegameinfo[savetodraw].numemeralds & (1 << j))
					V_DrawScaledPatch(workx, y, 0, emeraldpics[1][j]);
				workx += 10;
			}
		}

		y -= 4;

		// character heads, lives, and continues/score
		{
			spritedef_t *sprdef;
			spriteframe_t *sprframe;
			patch_t *patch;
			UINT8 *colormap = NULL;

			INT32 tempx = (x+40)<<FRACBITS, flip = 0;

			// botskin first
			if (savegameinfo[savetodraw].botskin)
			{
				skin_t *charbotskin = &skins[savegameinfo[savetodraw].botskin-1];
				sprdef = &charbotskin->sprites[SPR2_SIGN];
				if (!sprdef->numframes)
					goto skipbot;
				colormap = R_GetTranslationColormap(savegameinfo[savetodraw].botskin-1, charbotskin->prefcolor, GTC_CACHE);
				sprframe = &sprdef->spriteframes[0];
				patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);

				V_DrawFixedPatch(
					tempx + (18<<FRACBITS),
					y<<FRACBITS,
					charbotskin->highresscale,
					0, patch, colormap);

				tempx -= (20<<FRACBITS);
				//flip = V_FLIP;
			}
skipbot:
			// signpost image
			if (!charskin) // shut up compiler
				goto skipsign;
			sprdef = &charskin->sprites[SPR2_SIGN];
			colormap = R_GetTranslationColormap(savegameinfo[savetodraw].skinnum, charskin->prefcolor, GTC_CACHE);
			if (!sprdef->numframes)
				goto skipsign;
			sprframe = &sprdef->spriteframes[0];
			patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);

			V_DrawFixedPatch(
				tempx,
				y<<FRACBITS,
				charskin->highresscale,
				flip, patch, colormap);

skipsign:
			y += 16;

			tempx = x;
			if (useContinues)
			{
				tempx += 10;
				if (savegameinfo[savetodraw].lives != INFLIVES
				&& savegameinfo[savetodraw].lives > 9)
					tempx -= 4;
			}

			if (!charskin) // shut up compiler
				goto skiplife;

			// lives
			sprdef = &charskin->sprites[SPR2_LIFE];
			if (!sprdef->numframes)
				goto skiplife;
			sprframe = &sprdef->spriteframes[0];
			patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);

			V_DrawFixedPatch(
				(tempx + 4)<<FRACBITS,
				(y + 6)<<FRACBITS,
				charskin->highresscale/2,
				0, patch, colormap);
skiplife:

			patch = W_CachePatchName("STLIVEX", PU_PATCH);

			V_DrawScaledPatch(tempx + 9, y + 2, 0, patch);
			tempx += 16;
			if (savegameinfo[savetodraw].lives == INFLIVES)
				V_DrawCharacter(tempx, y + 1, '\x16', false);
			else
				V_DrawString(tempx, y, 0, va("%d", savegameinfo[savetodraw].lives));

			if (!useContinues)
			{
				INT32 workingscorenum = savegameinfo[savetodraw].continuescore;
				char workingscorestr[11] = " 000000000\0";
				SINT8 j = 9;
				// Change the above two lines if MAXSCORE ever changes from 8 digits long.
				workingscorestr[0] = '\x86'; // done here instead of in initialiser 'cuz compiler complains
				if (!workingscorenum)
					j--; // just so ONE digit is not greyed out
				else
				{
					while (workingscorenum)
					{
						workingscorestr[j--] = '0' + (workingscorenum % 10);
						workingscorenum /= 10;
					}
				}
				workingscorestr[j] = (savegameinfo[savetodraw].continuescore == MAXSCORE) ? '\x83' : '\x80';
				V_DrawRightAlignedThinString(x + 80, y+1, 0, workingscorestr);
			}
			else
			{
				tempx = x + 47;
				if (savegameinfo[savetodraw].continuescore > 9)
					tempx -= 4;

				// continues
				if (savegameinfo[savetodraw].continuescore > 0)
				{
					V_DrawSmallScaledPatch(tempx, y, 0, W_CachePatchName("CONTSAVE", PU_PATCH));
					V_DrawScaledPatch(tempx + 9, y + 2, 0, patch);
					V_DrawString(tempx + 16, y, 0, va("%d", savegameinfo[savetodraw].continuescore));
				}
				else
				{
					V_DrawSmallScaledPatch(tempx, y, 0, W_CachePatchName("CONTNONE", PU_PATCH));
					V_DrawScaledPatch(tempx + 9, y + 2, 0, W_CachePatchName("STNONEX", PU_PATCH));
					V_DrawString(tempx + 16, y, V_GRAYMAP, "0");
				}
			}
		}
	}
}

static void M_DrawLoad(void)
{
	M_DrawMenuTitle();

	if (loadgamescroll > 1 || loadgamescroll < -1)
		loadgamescroll = 2*loadgamescroll/3;
	else
		loadgamescroll = 0;

	if (loadgameoffset > 1)
		loadgameoffset = 2*loadgameoffset/3;
	else
		loadgameoffset = 0;

	M_DrawLoadGameData();
}

//
// User wants to load this game
//
static void M_LoadSelect(INT32 choice)
{
	(void)choice;

	if (saveSlotSelected == NOSAVESLOT) //last slot is play without saving
	{
		M_NewGame();
		cursaveslot = 0;
		return;
	}

	if (!FIL_ReadFileOK(va(savegamename, saveSlotSelected)))
	{
		// This slot is empty, so start a new game here.
		M_NewGame();
	}
	else if (savegameinfo[saveSlotSelected-1].gamemap & 8192) // Completed
		M_LoadGameLevelSelect(0);
	else
		G_LoadGame((UINT32)saveSlotSelected, 0);

	cursaveslot = saveSlotSelected;
}

#define VERSIONSIZE 16
#define BADSAVE { savegameinfo[slot].lives = -666; Z_Free(savebuffer); return; }
#define CHECKPOS if (save_p >= end_p) BADSAVE
// Reads the save file to list lives, level, player, etc.
// Tails 05-29-2003
static void M_ReadSavegameInfo(UINT32 slot)
{
	size_t length;
	char savename[255];
	UINT8 *savebuffer;
	UINT8 *end_p; // buffer end point, don't read past here
	UINT8 *save_p;
	INT32 fake; // Dummy variable
	char temp[sizeof(timeattackfolder)];
	char vcheck[VERSIONSIZE];

	sprintf(savename, savegamename, slot);

	slot--;

	length = FIL_ReadFile(savename, &savebuffer);
	if (length == 0)
	{
		savegameinfo[slot].lives = -42;
		return;
	}

	end_p = savebuffer + length;

	// skip the description field
	save_p = savebuffer;

	// Version check
	memset(vcheck, 0, sizeof (vcheck));
	sprintf(vcheck, "version %d", VERSION);
	if (strcmp((const char *)save_p, (const char *)vcheck)) BADSAVE
	save_p += VERSIONSIZE;

	// dearchive all the modifications
	// P_UnArchiveMisc()

	CHECKPOS
	fake = READINT16(save_p);

	if (((fake-1) & 8191) >= NUMMAPS) BADSAVE

	if(!mapheaderinfo[(fake-1) & 8191])
		savegameinfo[slot].levelname[0] = '\0';
	else
	{
		strlcpy(savegameinfo[slot].levelname, mapheaderinfo[(fake-1) & 8191]->lvlttl, 17+1);

		if (strlen(mapheaderinfo[(fake-1) & 8191]->lvlttl) >= 17)
			strcpy(savegameinfo[slot].levelname+17-3, "...");
	}

	savegameinfo[slot].gamemap = fake;

	CHECKPOS
	savegameinfo[slot].numemeralds = READUINT16(save_p)-357; // emeralds

	CHECKPOS
	READSTRINGN(save_p, temp, sizeof(temp)); // mod it belongs to

	if (strcmp(temp, timeattackfolder)) BADSAVE

	// P_UnArchivePlayer()
	CHECKPOS
	fake = READUINT16(save_p);
	savegameinfo[slot].skinnum = fake & ((1<<5) - 1);
	if (savegameinfo[slot].skinnum >= numskins
	|| !R_SkinUsable(-1, savegameinfo[slot].skinnum))
		BADSAVE
	savegameinfo[slot].botskin = fake >> 5;
	if (savegameinfo[slot].botskin-1 >= numskins
	|| !R_SkinUsable(-1, savegameinfo[slot].botskin-1))
		BADSAVE

	CHECKPOS
	savegameinfo[slot].numgameovers = READUINT8(save_p); // numgameovers
	CHECKPOS
	savegameinfo[slot].lives = READSINT8(save_p); // lives
	CHECKPOS
	savegameinfo[slot].continuescore = READINT32(save_p); // score
	CHECKPOS
	fake = READINT32(save_p); // continues
	if (useContinues)
		savegameinfo[slot].continuescore = fake;

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
	Z_Free(savebuffer);
}
#undef CHECKPOS
#undef BADSAVE

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//  and put it in savegamestrings global variable
//
static void M_ReadSaveStrings(void)
{
	FILE *handle;
	SINT8 i;
	char name[256];
	boolean nofile[MAXSAVEGAMES-1];
	SINT8 tolerance = 3; // empty slots at any time
	UINT8 lastseen = 0;

	loadgamescroll = 0;
	loadgameoffset = 14;

	for (i = 1; (i < MAXSAVEGAMES); i++) // slot 0 is no save
	{
		snprintf(name, sizeof name, savegamename, i);
		name[sizeof name - 1] = '\0';

		handle = fopen(name, "rb");
		if ((nofile[i-1] = (handle == NULL)))
			continue;
		fclose(handle);
		lastseen = i;
	}

	if (savegameinfo)
		Z_Free(savegameinfo);
	savegameinfo = NULL;

	if (lastseen < saveSlotSelected)
		lastseen = saveSlotSelected;

	i = lastseen;

	for (; (lastseen > 0 && tolerance); lastseen--)
	{
		if (nofile[lastseen-1])
			tolerance--;
	}

	if ((i += tolerance+1) > MAXSAVEGAMES) // show 3 empty slots at minimum
		i = MAXSAVEGAMES;

	numsaves = i;
	savegameinfo = Z_Realloc(savegameinfo, numsaves*sizeof(saveinfo_t), PU_STATIC, NULL);
	if (!savegameinfo)
		I_Error("Insufficient memory to prepare save platter");

	for (; i > 0; i--)
	{
		if (nofile[i-1] == true)
		{
			savegameinfo[i-1].lives = -42;
			continue;
		}
		M_ReadSavegameInfo(i);
	}

	M_CacheLoadGameData();
}

//
// User wants to delete this game
//
static void M_SaveGameDeleteResponse(INT32 ch)
{
	char name[256];

	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// delete savegame
	snprintf(name, sizeof name, savegamename, saveSlotSelected);
	name[sizeof name - 1] = '\0';
	remove(name);

	BwehHehHe();
	M_ReadSaveStrings(); // reload the menu
}

static void M_SaveGameUltimateResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	S_StartSound(NULL, sfx_menu1);
	M_LoadSelect(saveSlotSelected);
	SP_PlayerDef.prevMenu = MessageDef.prevMenu;
	MessageDef.prevMenu = &SP_PlayerDef;
}

static void M_HandleLoadSave(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_s3kb7);
			++saveSlotSelected;
			if (saveSlotSelected >= numsaves)
				saveSlotSelected -= numsaves;
			loadgamescroll = 90;
			break;

		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_s3kb7);
			--saveSlotSelected;
			if (saveSlotSelected < 0)
				saveSlotSelected += numsaves;
			loadgamescroll = -90;
			break;

		case KEY_ENTER:
			if (ultimate_selectable && saveSlotSelected == NOSAVESLOT)
			{
				loadgamescroll = 0;
				S_StartSound(NULL, sfx_skid);
				M_StartMessage("Are you sure you want to play\n\x85ultimate mode\x80? It isn't remotely fair,\nand you don't even get an emblem for it.\n\n(Press 'Y' to confirm)\n",M_SaveGameUltimateResponse,MM_YESNO);
			}
			else if (saveSlotSelected != NOSAVESLOT && savegameinfo[saveSlotSelected-1].lives == -42 && !(!modifiedgame || savemoddata))
			{
				loadgamescroll = 0;
				S_StartSound(NULL, sfx_skid);
				M_StartMessage(M_GetText("This cannot be done in a modified game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
			}
			else if (saveSlotSelected == NOSAVESLOT || savegameinfo[saveSlotSelected-1].lives != -666) // don't allow loading of "bad saves"
			{
				loadgamescroll = 0;
				S_StartSound(NULL, sfx_menu1);
				M_LoadSelect(saveSlotSelected);
			}
			else if (!loadgameoffset)
			{
				S_StartSound(NULL, sfx_lose);
				loadgameoffset = 14;
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			// Don't allow people to 'delete' "Play without Saving."
			// Nor allow people to 'delete' slots with no saves in them.
			if (saveSlotSelected != NOSAVESLOT && savegameinfo[saveSlotSelected-1].lives != -42)
			{
				loadgamescroll = 0;
				S_StartSound(NULL, sfx_skid);
				M_StartMessage(va("Are you sure you want to delete\nsave file %d?\n\n(Press 'Y' to confirm)\n", saveSlotSelected),M_SaveGameDeleteResponse,MM_YESNO);
			}
			else if (!loadgameoffset)
			{
				if (saveSlotSelected == NOSAVESLOT && ultimate_selectable)
				{
					ultimate_selectable = false;
					S_StartSound(NULL, sfx_strpst);
				}
				else
					S_StartSound(NULL, sfx_lose);
				loadgameoffset = 14;
			}
			break;
	}
	if (exitmenu)
	{
		// Is this a hack?
		charseltimer = 0;
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
		Z_Free(savegameinfo);
		savegameinfo = NULL;
	}
}

static void M_FirstTimeResponse(INT32 ch)
{
	S_StartSound(NULL, sfx_menu1);

	if (ch == KEY_ESCAPE)
		return;

	if (ch != 'y' && ch != KEY_ENTER)
	{
		CV_SetValue(&cv_tutorialprompt, 0);
		M_ReadSaveStrings();
		MessageDef.prevMenu = &SP_LoadDef; // calls M_SetupNextMenu
	}
	else
	{
		M_StartTutorial(0);
		MessageDef.prevMenu = &MessageDef; // otherwise, the controls prompt won't fire
	}
}

//
// Selected from SRB2 menu
//
static void M_LoadGame(INT32 choice)
{
	(void)choice;

	if (tutorialmap && cv_tutorialprompt.value)
	{
		M_StartMessage("Do you want to \x82play a brief Tutorial\x80?\n\nWe highly recommend this because \nthe controls are slightly different \nfrom other games.\n\nPress 'Y' or 'Enter' to go\nPress 'N' or any key to skip\n",
			M_FirstTimeResponse, MM_YESNO);
		return;
	}

	M_ReadSaveStrings();
	M_SetupNextMenu(&SP_LoadDef);
}

//
// Used by cheats to force the save menu to a specific spot.
//
void M_ForceSaveSlotSelected(INT32 sslot)
{
	loadgameoffset = 14;

	// Already there? Whatever, then!
	if (sslot == saveSlotSelected)
		return;

	loadgamescroll = 90;
	if (saveSlotSelected <= numsaves/2)
		loadgamescroll = -loadgamescroll;

	saveSlotSelected = sslot;
}

// ================
// CHARACTER SELECT
// ================

static void M_CacheCharacterSelectEntry(INT32 i, INT32 skinnum)
{
	if (!(description[i].picname[0]))
	{
		if (skins[skinnum].sprites[SPR2_XTRA].numframes > XTRA_CHARSEL)
		{
			spritedef_t *sprdef = &skins[skinnum].sprites[SPR2_XTRA];
			spriteframe_t *sprframe = &sprdef->spriteframes[XTRA_CHARSEL];
			description[i].charpic = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);
		}
		else
			description[i].charpic = W_CachePatchName("MISSING", PU_PATCH);
	}
	else
		description[i].charpic = W_CachePatchName(description[i].picname, PU_PATCH);

	if (description[i].nametag[0])
		description[i].namepic = W_CachePatchName(description[i].nametag, PU_PATCH);
}

static UINT8 M_SetupChoosePlayerDirect(INT32 choice)
{
	INT32 skinnum;
	UINT8 i;
	UINT8 firstvalid = 255, lastvalid = 255;
	boolean allowed = false;
	char *and;
	(void)choice;

	if (!mapheaderinfo[startmap-1] || mapheaderinfo[startmap-1]->forcecharacter[0] == '\0')
	{
		for (i = 0; i < MAXSKINS; i++) // Handle charsels, availability, and unlocks.
		{
			if (description[i].used) // If the character's disabled through SOC, there's nothing we can do for it.
			{
				and = strchr(description[i].skinname, '&');
				if (and)
				{
					char firstskin[SKINNAMESIZE+1];
					if (mapheaderinfo[startmap-1] && mapheaderinfo[startmap-1]->typeoflevel & TOL_NIGHTS) // skip tagteam characters for NiGHTS levels
						continue;
					strncpy(firstskin, description[i].skinname, (and - description[i].skinname));
					firstskin[(and - description[i].skinname)] = '\0';
					description[i].skinnum[0] = R_SkinAvailable(firstskin);
					description[i].skinnum[1] = R_SkinAvailable(and+1);
				}
				else
				{
					description[i].skinnum[0] = R_SkinAvailable(description[i].skinname);
					description[i].skinnum[1] = -1;
				}
				skinnum = description[i].skinnum[0];
				if ((skinnum != -1) && (R_SkinUsable(-1, skinnum)))
				{
					// Handling order.
					if (firstvalid == 255)
						firstvalid = i;
					else
					{
						description[i].prev = lastvalid;
						description[lastvalid].next = i;
					}
					lastvalid = i;

					if (i == char_on)
						allowed = true;

					M_CacheCharacterSelectEntry(i, skinnum);
				}
				// else -- Technically, character select icons without corresponding skins get bundled away behind this too. Sucks to be them.
			}
		}
	}

	if (firstvalid == lastvalid) // We're being forced into a specific character, so might as well just skip it.
	{
		return firstvalid;
	}

	// One last bit of order we can't do in the iteration above.
	description[firstvalid].prev = lastvalid;
	description[lastvalid].next = firstvalid;

	if (!allowed)
	{
		char_on = firstvalid;
		if (startchar > 0 && startchar < MAXSKINS)
		{
			INT16 workchar = startchar;
			while (workchar--)
				char_on = description[char_on].next;
		}
	}

	return MAXSKINS;
}

static void M_SetupChoosePlayer(INT32 choice)
{
	UINT8 skinset = M_SetupChoosePlayerDirect(choice);
	if (skinset != MAXSKINS)
	{
		M_ChoosePlayer(skinset);
		return;
	}

	M_ChangeMenuMusic("_chsel", true);

	/* the menus suck -James */
	if (currentMenu == &SP_LoadDef)/* from save states */
	{
		SP_PlayerDef.menuid = MTREE3(MN_SP_MAIN, MN_SP_LOAD, MN_SP_PLAYER);
	}
	else/* from Secret level select */
	{
		SP_PlayerDef.menuid = MTREE2(MN_SR_MAIN, MN_SR_PLAYER);
	}

	SP_PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SP_PlayerDef);

	// finish scrolling the menu
	char_scroll = 0;
	charseltimer = 0;

	Z_Free(char_notes);
	char_notes = V_WordWrap(0, 21*8, V_ALLOWLOWERCASE, description[char_on].notes);
}

//
// M_HandleChoosePlayerMenu
//
// Reacts to your key inputs. Basically a mini menu thinker.
//
static void M_HandleChoosePlayerMenu(INT32 choice)
{
	boolean exitmenu = false;  // exit to previous menu
	INT32 selectval;

	if (keydown > 1)
		return;

	switch (choice)
	{
		case KEY_DOWNARROW:
			if ((selectval = description[char_on].next) != char_on)
			{
				S_StartSound(NULL,sfx_s3kb7);
				char_on = selectval;
				char_scroll = -charscrollamt;
				Z_Free(char_notes);
				char_notes = V_WordWrap(0, 21*8, V_ALLOWLOWERCASE, description[char_on].notes);
			}
			else if (!char_scroll)
			{
				S_StartSound(NULL,sfx_s3kb7);
				char_scroll = 16*FRACUNIT;
			}
			break;

		case KEY_UPARROW:
			if ((selectval = description[char_on].prev) != char_on)
			{
				S_StartSound(NULL,sfx_s3kb7);
				char_on = selectval;
				char_scroll = charscrollamt;
				Z_Free(char_notes);
				char_notes = V_WordWrap(0, 21*8, V_ALLOWLOWERCASE, description[char_on].notes);
			}
			else if (!char_scroll)
			{
				S_StartSound(NULL,sfx_s3kb7);
				char_scroll = -16*FRACUNIT;
			}
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			char_scroll = 0; // finish scrolling the menu
			M_DrawSetupChoosePlayerMenu(); // draw the finally selected character one last time for the fadeout
			// Is this a hack?
			charseltimer = 0;
			M_ChoosePlayer(char_on);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}

	if (exitmenu)
	{
		// Is this a hack?
		charseltimer = 0;
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// Draw the choose player setup menu, had some fun with player anim
//define CHOOSEPLAYER_DRAWHEADER

static void M_DrawSetupChoosePlayerMenu(void)
{
	const INT32 my = 16;

	skin_t *charskin = &skins[0];
	INT32 skinnum = 0;
	UINT16 col;
	UINT8 *colormap = NULL;
	INT32 prev = -1, next = -1;

	patch_t *charbg = W_CachePatchName("CHARBG", PU_PATCH);
	patch_t *charfg = W_CachePatchName("CHARFG", PU_PATCH);
	INT16 bgheight = charbg->height;
	INT16 fgheight = charfg->height;
	INT16 bgwidth = charbg->width;
	INT16 fgwidth = charfg->width;
	INT32 x, y;
	INT32 w = (vid.width/vid.dupx);

	if (abs(char_scroll) > FRACUNIT)
		char_scroll -= (char_scroll>>2);
	else // close enough.
		char_scroll = 0; // just be exact now.

	// Get prev character...
	prev = description[char_on].prev;
	// If there's more than one character available...
	if (prev != char_on)
		// Let's get the next character now.
		next = description[char_on].next;
	else
		// No there isn't.
		prev = -1;

	// Find skin number from description[]
	skinnum = description[char_on].skinnum[0];
	charskin = &skins[skinnum];

	// Use the opposite of the character's skincolor
	col = description[char_on].oppositecolor;
	if (!col)
		col = skincolors[charskin->prefcolor].invcolor;

	// Make the translation colormap
	colormap = R_GetTranslationColormap(TC_DEFAULT, col, GTC_CACHE);

	// Don't render the title map
	hidetitlemap = true;
	charseltimer++;

	// Background and borders
	V_DrawFill(0, 0, bgwidth, vid.height, V_SNAPTOTOP|colormap[101]);
	{
		INT32 sw = (BASEVIDWIDTH * vid.dupx);
		INT32 bw = (vid.width - sw) / 2;
		col = colormap[106];
		if (bw)
			V_DrawFill(0, 0, bw, vid.height, V_NOSCALESTART|col);
	}

	y = (charseltimer%32);
	V_DrawMappedPatch(0, y-bgheight, V_SNAPTOTOP, charbg, colormap);
	V_DrawMappedPatch(0, y, V_SNAPTOTOP, charbg, colormap);
	V_DrawMappedPatch(0, y+bgheight, V_SNAPTOTOP, charbg, colormap);
	V_DrawMappedPatch(0, -y, V_SNAPTOTOP, charfg, colormap);
	V_DrawMappedPatch(0, -y+fgheight, V_SNAPTOTOP, charfg, colormap);
	V_DrawFill(fgwidth, 0, vid.width, vid.height, V_SNAPTOTOP|colormap[106]);

	// Character pictures
	{
		x = 8;
		y = (my+16) - FixedInt(char_scroll);
		V_DrawScaledPatch(x, y, 0, description[char_on].charpic);
		if (prev != -1)
			V_DrawScaledPatch(x, y - 144, 0, description[prev].charpic);
		if (next != -1)
			V_DrawScaledPatch(x, y + 144, 0, description[next].charpic);
	}

	// Character description
	{
		INT32 flags = V_ALLOWLOWERCASE|V_RETURN8;
		x = 146;
		y = my + 9;
		V_DrawString(x, y, flags, char_notes);
	}

	// Name tags
	{
		INT32 ox, oxsh = FixedInt(FixedMul(BASEVIDWIDTH*FRACUNIT, FixedDiv(char_scroll, 128*FRACUNIT))), txsh;
		patch_t *curpatch = NULL, *prevpatch = NULL, *nextpatch = NULL;
		const char *curtext = NULL, *prevtext = NULL, *nexttext = NULL;
		UINT16 curtextcolor = 0, prevtextcolor = 0, nexttextcolor = 0;
		UINT16 curoutlinecolor = 0, prevoutlinecolor = 0, nextoutlinecolor = 0;

		// Name tag
		curtext = description[char_on].displayname;
		curtextcolor = description[char_on].tagtextcolor;
		curoutlinecolor = description[char_on].tagoutlinecolor;
		if (curtext[0] == '\0')
			curpatch = description[char_on].namepic;
		if (!curtextcolor)
			curtextcolor = charskin->prefcolor;
		if (!curoutlinecolor)
			curoutlinecolor = col = skincolors[charskin->prefcolor].invcolor;

		txsh = oxsh;
		ox = 8 + ((description[char_on].charpic)->width)/2;
		y = my + 144;

		// cur
		{
			x = ox - txsh;
			if (curpatch)
				x -= curpatch->width / 2;

			if (curtext[0] != '\0')
			{
				V_DrawNameTag(
					x, y, V_CENTERNAMETAG, FRACUNIT,
					R_GetTranslationColormap(TC_DEFAULT, curtextcolor, GTC_CACHE),
					R_GetTranslationColormap(TC_DEFAULT, curoutlinecolor, GTC_CACHE),
					curtext
				);
			}
			else if (curpatch)
				V_DrawScaledPatch(x, y, 0, curpatch);
		}

		if (char_scroll)
		{
			// prev
			if ((prev != -1) && char_scroll < 0)
			{
				prevtext = description[prev].displayname;
				prevtextcolor = description[prev].tagtextcolor;
				prevoutlinecolor = description[prev].tagoutlinecolor;
				if (prevtext[0] == '\0')
					prevpatch = description[prev].namepic;
				charskin = &skins[description[prev].skinnum[0]];
				if (!prevtextcolor)
					prevtextcolor = charskin->prefcolor;
				if (!prevoutlinecolor)
					prevoutlinecolor = col = skincolors[charskin->prefcolor].invcolor;

				x = (ox - txsh) - w;
				if (prevpatch)
					x -= prevpatch->width / 2;

				if (prevtext[0] != '\0')
				{
					V_DrawNameTag(
						x, y, V_CENTERNAMETAG, FRACUNIT,
						R_GetTranslationColormap(TC_DEFAULT, prevtextcolor, GTC_CACHE),
						R_GetTranslationColormap(TC_DEFAULT, prevoutlinecolor, GTC_CACHE),
						prevtext
					);
				}
				else if (prevpatch)
					V_DrawScaledPatch(x, y, 0, prevpatch);
			}
			// next
			else if ((next != -1) && char_scroll > 0)
			{
				nexttext = description[next].displayname;
				nexttextcolor = description[next].tagtextcolor;
				nextoutlinecolor = description[next].tagoutlinecolor;
				if (nexttext[0] == '\0')
					nextpatch = description[next].namepic;
				charskin = &skins[description[next].skinnum[0]];
				if (!nexttextcolor)
					nexttextcolor = charskin->prefcolor;
				if (!nextoutlinecolor)
					nextoutlinecolor = col = skincolors[charskin->prefcolor].invcolor;

				x = (ox - txsh) + w;
				if (nextpatch)
					x -= nextpatch->width / 2;

				if (nexttext[0] != '\0')
				{
					V_DrawNameTag(
						x, y, V_CENTERNAMETAG, FRACUNIT,
						R_GetTranslationColormap(TC_DEFAULT, nexttextcolor, GTC_CACHE),
						R_GetTranslationColormap(TC_DEFAULT, nextoutlinecolor, GTC_CACHE),
						nexttext
					);
				}
				else if (nextpatch)
					V_DrawScaledPatch(x, y, 0, nextpatch);
			}
		}
	}

	// Alternative menu header
#ifdef CHOOSEPLAYER_DRAWHEADER
	{
		patch_t *header = W_CachePatchName("M_PICKP", PU_PATCH);
		INT32 xtitle = 146;
		INT32 ytitle = (35 - header->height) / 2;
		V_DrawFixedPatch(xtitle<<FRACBITS, ytitle<<FRACBITS, FRACUNIT/2, 0, header, NULL);
	}
#endif // CHOOSEPLAYER_DRAWHEADER

	M_DrawMenuTitle();
}

// Chose the player you want to use Tails 03-02-2002
static void M_ChoosePlayer(INT32 choice)
{
	boolean ultmode = (currentMenu == &SP_MarathonDef) ? (cv_dummymarathon.value == 2) : (ultimate_selectable && SP_PlayerDef.prevMenu == &SP_LoadDef && saveSlotSelected == NOSAVESLOT);
	UINT8 skinnum;

	// skip this if forcecharacter or no characters available
	if (choice == 255)
	{
		skinnum = botskin = 0;
		botingame = false;
	}
	// M_SetupChoosePlayer didn't call us directly, that means we've been properly set up.
	else
	{
		skinnum = description[choice].skinnum[0];

		if ((botingame = (description[choice].skinnum[1] != -1))) {
			// this character has a second skin
			botskin = (UINT8)(description[choice].skinnum[1]+1);
			botcolor = skins[description[choice].skinnum[1]].prefcolor;
		}
		else
			botskin = botcolor = 0;
	}

	M_ClearMenus(true);

	if (!marathonmode && startmap != spstage_start)
		cursaveslot = 0;

	//lastmapsaved = 0;
	gamecomplete = 0;

	G_DeferedInitNew(ultmode, G_BuildMapName(startmap), skinnum, false, fromlevelselect);
	COM_BufAddText("dummyconsvar 1\n"); // G_DeferedInitNew doesn't do this

	if (levelselect.rows)
		Z_Free(levelselect.rows);
	levelselect.rows = NULL;

	if (savegameinfo)
		Z_Free(savegameinfo);
	savegameinfo = NULL;
}

// ===============
// STATISTICS MENU
// ===============

static INT32 statsLocation;
static INT32 statsMax;
static INT16 statsMapList[NUMMAPS+1];

static void M_Statistics(INT32 choice)
{
	INT16 i, j = 0;

	(void)choice;

	memset(statsMapList, 0, sizeof(statsMapList));

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!mapheaderinfo[i] || mapheaderinfo[i]->lvlttl[0] == '\0')
			continue;

		if (!(mapheaderinfo[i]->typeoflevel & TOL_SP) || (mapheaderinfo[i]->menuflags & LF2_HIDEINSTATS))
			continue;

		if (!(mapvisited[i] & MV_MAX))
			continue;

		statsMapList[j++] = i;
	}
	statsMapList[j] = -1;
	statsMax = j - 11 + numextraemblems;
	statsLocation = 0;

	if (statsMax < 0)
		statsMax = 0;

	M_SetupNextMenu(&SP_LevelStatsDef);
}

static void M_DrawStatsMaps(int location)
{
	INT32 y = 80, i = -1;
	INT16 mnum;
	extraemblem_t *exemblem;
	boolean dotopname = true, dobottomarrow = (location < statsMax);

	if (location)
		V_DrawString(10, y-(skullAnimCounter/5), V_YELLOWMAP, "\x1A");

	while (statsMapList[++i] != -1)
	{
		if (location)
		{
			--location;
			continue;
		}
		else if (dotopname)
		{
			V_DrawString(20,  y, V_GREENMAP, "LEVEL NAME");
			V_DrawString(248, y, V_GREENMAP, "EMBLEMS");
			y += 8;
			dotopname = false;
		}

		mnum = statsMapList[i];
		M_DrawMapEmblems(mnum+1, 292, y);

		if (mapheaderinfo[mnum]->actnum != 0)
			V_DrawString(20, y, V_YELLOWMAP|V_ALLOWLOWERCASE, va("%s %d", mapheaderinfo[mnum]->lvlttl, mapheaderinfo[mnum]->actnum));
		else
			V_DrawString(20, y, V_YELLOWMAP|V_ALLOWLOWERCASE, mapheaderinfo[mnum]->lvlttl);

		y += 8;

		if (y >= BASEVIDHEIGHT-8)
			goto bottomarrow;
	}
	if (dotopname && !location)
	{
		V_DrawString(20,  y, V_GREENMAP, "LEVEL NAME");
		V_DrawString(248, y, V_GREENMAP, "EMBLEMS");
		y += 8;
	}
	else if (location)
		--location;

	// Extra Emblems
	for (i = -2; i < numextraemblems; ++i)
	{
		if (i == -1)
		{
			V_DrawString(20, y, V_GREENMAP, "EXTRA EMBLEMS");
			if (location)
			{
				y += 8;
				location++;
			}
		}
		if (location)
		{
			--location;
			continue;
		}

		if (i >= 0)
		{
			exemblem = &extraemblems[i];

			if (exemblem->collected)
				V_DrawSmallMappedPatch(292, y, 0, W_CachePatchName(M_GetExtraEmblemPatch(exemblem, false), PU_PATCH),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetExtraEmblemColor(exemblem), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(292, y, 0, W_CachePatchName("NEEDIT", PU_PATCH));

			V_DrawString(20, y, V_YELLOWMAP|V_ALLOWLOWERCASE,
				(!exemblem->collected && exemblem->showconditionset && !M_Achieved(exemblem->showconditionset))
				? M_CreateSecretMenuOption(exemblem->description)
				: exemblem->description);
		}

		y += 8;

		if (y >= BASEVIDHEIGHT-8)
			goto bottomarrow;
	}
bottomarrow:
	if (dobottomarrow)
		V_DrawString(10, y-8 + (skullAnimCounter/5), V_YELLOWMAP, "\x1B");
}

static void M_DrawLevelStats(void)
{
	char beststr[40];

	tic_t besttime = 0;
	UINT32 bestscore = 0;
	UINT32 bestrings = 0;

	INT32 i;
	INT32 mapsunfinished = 0;
	boolean bestunfinished[3] = {false, false, false};

	M_DrawMenuTitle();

	V_DrawString(20, 24, V_YELLOWMAP, "Total Play Time:");
	V_DrawCenteredString(BASEVIDWIDTH/2, 32, 0, va("%i hours, %i minutes, %i seconds",
	                         G_TicsToHours(totalplaytime),
	                         G_TicsToMinutes(totalplaytime, false),
	                         G_TicsToSeconds(totalplaytime)));

	for (i = 0; i < NUMMAPS; i++)
	{
		boolean mapunfinished = false;

		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;

		if (!mainrecords[i])
		{
			mapsunfinished++;
			bestunfinished[0] = bestunfinished[1] = bestunfinished[2] = true;
			continue;
		}

		if (mainrecords[i]->score > 0)
			bestscore += mainrecords[i]->score;
		else
			mapunfinished = bestunfinished[0] = true;

		if (mainrecords[i]->time > 0)
			besttime += mainrecords[i]->time;
		else
			mapunfinished = bestunfinished[1] = true;

		if (mainrecords[i]->rings > 0)
			bestrings += mainrecords[i]->rings;
		else
			mapunfinished = bestunfinished[2] = true;

		if (mapunfinished)
			mapsunfinished++;
	}

	V_DrawString(20, 48, 0, "Combined records:");

	if (mapsunfinished)
		V_DrawString(20, 56, V_REDMAP, va("(%d unfinished)", mapsunfinished));
	else
		V_DrawString(20, 56, V_GREENMAP, "(complete)");

	V_DrawString(36, 64, 0, va("x %d/%d", M_CountEmblems(), numemblems+numextraemblems));
	V_DrawSmallScaledPatch(20, 64, 0, W_CachePatchName("EMBLICON", PU_PATCH));

	sprintf(beststr, "%u", bestscore);
	V_DrawString(BASEVIDWIDTH/2, 48, V_YELLOWMAP, "SCORE:");
	V_DrawRightAlignedString(BASEVIDWIDTH-16, 48, (bestunfinished[0] ? V_REDMAP : 0), beststr);

	sprintf(beststr, "%i:%02i:%02i.%02i", G_TicsToHours(besttime), G_TicsToMinutes(besttime, false), G_TicsToSeconds(besttime), G_TicsToCentiseconds(besttime));
	V_DrawString(BASEVIDWIDTH/2, 56, V_YELLOWMAP, "TIME:");
	V_DrawRightAlignedString(BASEVIDWIDTH-16, 56, (bestunfinished[1] ? V_REDMAP : 0), beststr);

	sprintf(beststr, "%u", bestrings);
	V_DrawString(BASEVIDWIDTH/2, 64, V_YELLOWMAP, "RINGS:");
	V_DrawRightAlignedString(BASEVIDWIDTH-16, 64, (bestunfinished[2] ? V_REDMAP : 0), beststr);

	M_DrawStatsMaps(statsLocation);
}

// Handle statistics.
static void M_HandleLevelStats(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			if (statsLocation < statsMax)
				++statsLocation;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			if (statsLocation)
				--statsLocation;
			break;

		case KEY_PGDN:
			S_StartSound(NULL, sfx_menu1);
			statsLocation += (statsLocation+13 >= statsMax) ? statsMax-statsLocation : 13;
			break;

		case KEY_PGUP:
			S_StartSound(NULL, sfx_menu1);
			statsLocation -= (statsLocation < 13) ? statsLocation : 13;
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// ===========
// MODE ATTACK
// ===========

// Drawing function for Time Attack
void M_DrawTimeAttackMenu(void)
{
	INT32 i, x, y, empatx, empaty, cursory = 0;
	UINT16 dispstatus;
	patch_t *PictureOfUrFace;	// my WHAT
	patch_t *empatch;

	M_SetMenuCurBackground("RECATKBG");

	curbgxspeed = 0;
	curbgyspeed = 18;

	M_ChangeMenuMusic("_recat", true); // Eww, but needed for when user hits escape during demo playback

	if (curbgcolor >= 0)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, curbgcolor);
	else if (!curbghide || !titlemapinaction)
	{
		F_SkyScroll(curbgxspeed, curbgyspeed, curbgname);
		// Draw and animate foreground
		if (!strncmp("RECATKBG", curbgname, 8))
			M_DrawRecordAttackForeground();
	}
	if (curfadevalue)
		V_DrawFadeScreen(0xFF00, curfadevalue);

	M_DrawMenuTitle();

	// draw menu (everything else goes on top of it)
	// Sadly we can't just use generic mode menus because we need some extra hacks
	x = currentMenu->x;
	y = currentMenu->y;

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		dispstatus = (currentMenu->menuitems[i].status & IT_DISPLAY);
		if (dispstatus != IT_STRING && dispstatus != IT_WHITESTRING)
			continue;

		y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
		if (i == itemOn)
			cursory = y;

		V_DrawString(x, y, (dispstatus == IT_WHITESTRING) ? V_YELLOWMAP : 0 , currentMenu->menuitems[i].text);

		// Cvar specific handling
		if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_CVAR)
		{
			consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
			INT32 soffset = 0;

			// hack to keep the menu from overlapping the player icon
			if (currentMenu != &SP_TimeAttackDef)
				soffset = 80;

			// Should see nothing but strings
			V_DrawString(BASEVIDWIDTH - x - soffset - V_StringWidth(cv->string, 0), y, V_YELLOWMAP, cv->string);
			if (i == itemOn)
			{
				V_DrawCharacter(BASEVIDWIDTH - x - soffset - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
					'\x1C' | V_YELLOWMAP, false);
				V_DrawCharacter(BASEVIDWIDTH - x - soffset + 2 + (skullAnimCounter/5), y,
					'\x1D' | V_YELLOWMAP, false);
			}
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_PATCH));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	// Character face!
	{
		if (skins[cv_chooseskin.value-1].sprites[SPR2_XTRA].numframes > XTRA_CHARSEL)
		{
			spritedef_t *sprdef = &skins[cv_chooseskin.value-1].sprites[SPR2_XTRA];
			spriteframe_t *sprframe = &sprdef->spriteframes[XTRA_CHARSEL];
			PictureOfUrFace = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);
		}
		else
			PictureOfUrFace = W_CachePatchName("MISSING", PU_PATCH);

		if (PictureOfUrFace->width >= 256)
			V_DrawTinyScaledPatch(224, 120, 0, PictureOfUrFace);
		else
			V_DrawSmallScaledPatch(224, 120, 0, PictureOfUrFace);
	}

	// Level record list
	if (cv_nextmap.value)
	{
		emblem_t *em;
		INT32 yHeight;
		patch_t *PictureOfLevel;
		lumpnum_t lumpnum;
		char beststr[40];
		char reqscore[40], reqtime[40], reqrings[40];

		strcpy(reqscore, "\0");
		strcpy(reqtime, "\0");
		strcpy(reqrings, "\0");

		M_DrawLevelPlatterHeader(32-lsheadingheight/2, cv_nextmap.string, true, false);

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

		y = 32+lsheadingheight;
		V_DrawSmallScaledPatch(216, y, 0, PictureOfLevel);


		if (currentMenu == &SP_TimeAttackDef)
		{
			if (itemOn == talevel)
			{
				/* Draw arrows !! */
				y = y + 25 - 4;
				V_DrawCharacter(216 - 10 - (skullAnimCounter/5), y,
						'\x1C' | V_YELLOWMAP, false);
				V_DrawCharacter(216 + 80 + 2 + (skullAnimCounter/5), y,
						'\x1D' | V_YELLOWMAP, false);
			}
			// Draw press ESC to exit string on main record attack menu
			V_DrawString(104-72, 180, V_TRANSLUCENT, M_GetText("Press ESC to exit"));
		}

		em = M_GetLevelEmblems(cv_nextmap.value);
		// Draw record emblems.
		while (em)
		{
			switch (em->type)
			{
				case ET_SCORE:
					yHeight = 33;
					sprintf(reqscore, "(%u)", em->var);
					break;
				case ET_TIME:
					yHeight = 53;
					sprintf(reqtime, "(%i:%02i.%02i)", G_TicsToMinutes((tic_t)em->var, true),
										G_TicsToSeconds((tic_t)em->var),
										G_TicsToCentiseconds((tic_t)em->var));
					break;
				case ET_RINGS:
					yHeight = 73;
					sprintf(reqrings, "(%u)", em->var);
					break;
				default:
					goto skipThisOne;
			}

			empatch = W_CachePatchName(M_GetEmblemPatch(em, true), PU_PATCH);

			empatx = empatch->leftoffset / 2;
			empaty = empatch->topoffset / 2;

			if (em->collected)
				V_DrawSmallMappedPatch(104+76+empatx, yHeight+lsheadingheight/2+empaty, 0, empatch,
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(104+76, yHeight+lsheadingheight/2, 0, W_CachePatchName("NEEDITL", PU_PATCH));

			skipThisOne:
			em = M_GetLevelEmblems(-1);
		}

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->score)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%u", mainrecords[cv_nextmap.value-1]->score);

		V_DrawString(104-72, 33+lsheadingheight/2, V_YELLOWMAP, "SCORE:");
		V_DrawRightAlignedString(104+64, 33+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);
		V_DrawRightAlignedString(104+72, 43+lsheadingheight/2, V_ALLOWLOWERCASE, reqscore);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->time)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(mainrecords[cv_nextmap.value-1]->time, true),
			                                 G_TicsToSeconds(mainrecords[cv_nextmap.value-1]->time),
			                                 G_TicsToCentiseconds(mainrecords[cv_nextmap.value-1]->time));

		V_DrawString(104-72, 53+lsheadingheight/2, V_YELLOWMAP, "TIME:");
		V_DrawRightAlignedString(104+64, 53+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);
		V_DrawRightAlignedString(104+72, 63+lsheadingheight/2, V_ALLOWLOWERCASE, reqtime);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->rings)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%hu", mainrecords[cv_nextmap.value-1]->rings);

		V_DrawString(104-72, 73+lsheadingheight/2, V_YELLOWMAP, "RINGS:");

		V_DrawRightAlignedString(104+64, 73+lsheadingheight/2, V_ALLOWLOWERCASE|((mapvisited[cv_nextmap.value-1] & MV_PERFECTRA) ? V_YELLOWMAP : 0), beststr);

		V_DrawRightAlignedString(104+72, 83+lsheadingheight/2, V_ALLOWLOWERCASE, reqrings);
	}

	// ALWAYS DRAW level and skin even when not on this menu!
	if (currentMenu != &SP_TimeAttackDef)
	{
		consvar_t *ncv;

		x = SP_TimeAttackDef.x;
		y = SP_TimeAttackDef.y;

		V_DrawString(x, y + SP_TimeAttackMenu[talevel].alphaKey, V_TRANSLUCENT, SP_TimeAttackMenu[talevel].text);

		ncv = (consvar_t *)SP_TimeAttackMenu[taplayer].itemaction;
		V_DrawString(x, y + SP_TimeAttackMenu[taplayer].alphaKey, V_TRANSLUCENT, SP_TimeAttackMenu[taplayer].text);
		V_DrawString(BASEVIDWIDTH - x - V_StringWidth(ncv->string, 0), y + SP_TimeAttackMenu[taplayer].alphaKey, V_YELLOWMAP|V_TRANSLUCENT, ncv->string);
	}
}

static void M_HandleTimeAttackLevelSelect(INT32 choice)
{
	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			break;
		case KEY_UPARROW:
			M_PrevOpt();
			break;

		case KEY_LEFTARROW:
			CV_AddValue(&cv_nextmap, -1);
			break;
		case KEY_RIGHTARROW:
			CV_AddValue(&cv_nextmap, 1);
			break;

		case KEY_ENTER:
			if (levellistmode == LLM_NIGHTSATTACK)
				M_NightsAttackLevelSelect(0);
			else
				M_TimeAttackLevelSelect(0);
			break;

		case KEY_ESCAPE:
			noFurtherInput = true;
			M_GoBack(0);
			return;

		default:
			return;
	}
	S_StartSound(NULL, sfx_menu1);
}

static void M_TimeAttackLevelSelect(INT32 choice)
{
	(void)choice;
	SP_TimeAttackLevelSelectDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SP_TimeAttackLevelSelectDef);
}

// Going to Time Attack menu...
static void M_TimeAttack(INT32 choice)
{
	(void)choice;

	SP_TimeAttackDef.prevMenu = &MainDef;
	levellistmode = LLM_RECORDATTACK; // Don't be dependent on cv_newgametype

	if (!M_PrepareLevelPlatter(-1, true))
	{
		M_StartMessage(M_GetText("No record-attackable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_PatchSkinNameTable();

	G_SetGamestate(GS_TIMEATTACK); // do this before M_SetupNextMenu so that menu meta state knows that we're switching
	titlemapinaction = TITLEMAP_OFF; // Nope don't give us HOMs please
	M_SetupNextMenu(&SP_TimeAttackDef);
	if (!M_CanShowLevelInList(cv_nextmap.value-1, -1) && levelselect.rows[0].maplist[0])
		CV_SetValue(&cv_nextmap, levelselect.rows[0].maplist[0]);
	else
		Nextmap_OnChange();

	itemOn = tastart; // "Start" is selected.
}

// Drawing function for Nights Attack
void M_DrawNightsAttackMenu(void)
{
	INT32 i, x, y, cursory = 0;
	UINT16 dispstatus;

	M_SetMenuCurBackground("NTSATKBG");

	M_ChangeMenuMusic("_nitat", true); // Eww, but needed for when user hits escape during demo playback

	M_DrawNightsAttackBackground();
	if (curfadevalue)
		V_DrawFadeScreen(0xFF00, curfadevalue);

	M_DrawMenuTitle();

	// draw menu (everything else goes on top of it)
	// Sadly we can't just use generic mode menus because we need some extra hacks
	x = currentMenu->x;
	y = currentMenu->y;

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		dispstatus = (currentMenu->menuitems[i].status & IT_DISPLAY);
		if (dispstatus != IT_STRING && dispstatus != IT_WHITESTRING)
			continue;

		y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
		if (i == itemOn)
			cursory = y;

		V_DrawString(x, y, (dispstatus == IT_WHITESTRING) ? V_YELLOWMAP : 0 , currentMenu->menuitems[i].text);

		// Cvar specific handling
		if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_CVAR)
		{
			consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
			INT32 soffset = 0;

			// hack to keep the menu from overlapping the overall grade icon
			if (currentMenu != &SP_NightsAttackDef)
				soffset = 80;

			// Should see nothing but strings
			V_DrawString(BASEVIDWIDTH - x - soffset - V_StringWidth(cv->string, 0), y, V_YELLOWMAP, cv->string);
			if (i == itemOn)
			{
				V_DrawCharacter(BASEVIDWIDTH - x - soffset - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
					'\x1C' | V_YELLOWMAP, false);
				V_DrawCharacter(BASEVIDWIDTH - x - soffset + 2 + (skullAnimCounter/5), y,
					'\x1D' | V_YELLOWMAP, false);
			}
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_PATCH));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	// Level record list
	if (cv_nextmap.value)
	{
		emblem_t *em;
		INT32 yHeight;
		INT32 xpos;
		patch_t *PictureOfLevel;
		lumpnum_t lumpnum;
		char beststr[40];

		//UINT8 bestoverall	= G_GetBestNightsGrade(cv_nextmap.value, 0);
		UINT8 bestgrade		= G_GetBestNightsGrade(cv_nextmap.value, cv_dummymares.value);
		UINT32 bestscore	= G_GetBestNightsScore(cv_nextmap.value, cv_dummymares.value);
		tic_t besttime		= G_GetBestNightsTime(cv_nextmap.value, cv_dummymares.value);

		M_DrawLevelPlatterHeader(32-lsheadingheight/2, cv_nextmap.string, true, false);

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

		y = 32+lsheadingheight;
		V_DrawSmallScaledPatch(208, y, 0, PictureOfLevel);

		// Draw press ESC to exit string on main nights attack menu
		if (currentMenu == &SP_NightsAttackDef)
		{
			if (itemOn == nalevel)
			{
				/* Draw arrows !! */
				y = y + 25 - 4;
				V_DrawCharacter(208 - 10 - (skullAnimCounter/5), y,
						'\x1C' | V_YELLOWMAP, false);
				V_DrawCharacter(208 + 80 + 2 + (skullAnimCounter/5), y,
						'\x1D' | V_YELLOWMAP, false);
			}
			// Draw press ESC to exit string on main record attack menu
			V_DrawString(104-72, 180, V_TRANSLUCENT, M_GetText("Press ESC to exit"));
		}

		// Super Sonic
		M_DrawNightsAttackSuperSonic();
		//if (P_HasGrades(cv_nextmap.value, 0))
		//	V_DrawScaledPatch(235 - (((ngradeletters[bestoverall])->width)*3)/2, 135, 0, ngradeletters[bestoverall]);

		if (P_HasGrades(cv_nextmap.value, cv_dummymares.value))
			{//make bigger again
			V_DrawString(104 - 72, 48+lsheadingheight/2, V_YELLOWMAP, "BEST GRADE:");
			V_DrawSmallScaledPatch(104 + 72 - (ngradeletters[bestgrade]->width/2),
				48+lsheadingheight/2 + 8 - (ngradeletters[bestgrade]->height/2),
				0, ngradeletters[bestgrade]);
		}

		if (!bestscore)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%u", bestscore);

		V_DrawString(104 - 72, 58+lsheadingheight/2, V_YELLOWMAP, "BEST SCORE:");
		V_DrawRightAlignedString(104 + 72, 58+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);

		if (besttime == UINT32_MAX)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(besttime, true),
																			 G_TicsToSeconds(besttime),
																			 G_TicsToCentiseconds(besttime));

		V_DrawString(104 - 72, 68+lsheadingheight/2, V_YELLOWMAP, "BEST TIME:");
		V_DrawRightAlignedString(104 + 72, 68+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);

		if (cv_dummymares.value == 0) {
			// Draw record emblems.
			em = M_GetLevelEmblems(cv_nextmap.value);
			while (em)
			{
				switch (em->type)
				{
					case ET_NGRADE:
						xpos = 104+38;
						yHeight = 48;
						break;
					case ET_NTIME:
						xpos = 104+76;
						yHeight = 68;
						break;
					default:
						goto skipThisOne;
				}

				if (em->collected)
					V_DrawSmallMappedPatch(xpos, yHeight+lsheadingheight/2, 0, W_CachePatchName(M_GetEmblemPatch(em, false), PU_PATCH),
																 R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
				else
					V_DrawSmallScaledPatch(xpos, yHeight+lsheadingheight/2, 0, W_CachePatchName("NEEDIT", PU_PATCH));

				skipThisOne:
				em = M_GetLevelEmblems(-1);
			}
		}
	}

	// ALWAYS DRAW level even when not on this menu!
	if (currentMenu != &SP_NightsAttackDef)
		V_DrawString(SP_NightsAttackDef.x, SP_NightsAttackDef.y + SP_TimeAttackMenu[nalevel].alphaKey, V_TRANSLUCENT, SP_NightsAttackMenu[nalevel].text);
}

static void M_NightsAttackLevelSelect(INT32 choice)
{
	(void)choice;
	SP_NightsAttackLevelSelectDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SP_NightsAttackLevelSelectDef);
}

// Going to Nights Attack menu...
static void M_NightsAttack(INT32 choice)
{
	(void)choice;

	SP_NightsAttackDef.prevMenu = &MainDef;
	levellistmode = LLM_NIGHTSATTACK; // Don't be dependent on cv_newgametype

	if (!M_PrepareLevelPlatter(-1, true))
	{
		M_StartMessage(M_GetText("No NiGHTS-attackable levels found.\n"),NULL,MM_NOTHING);
		return;
	}
	// This is really just to make sure Sonic is the played character, just in case
	M_PatchSkinNameTable();

	ntssupersonic[0] = W_CachePatchName("NTSSONC1", PU_PATCH);
	ntssupersonic[1] = W_CachePatchName("NTSSONC2", PU_PATCH);

	G_SetGamestate(GS_TIMEATTACK); // do this before M_SetupNextMenu so that menu meta state knows that we're switching
	titlemapinaction = TITLEMAP_OFF; // Nope don't give us HOMs please
	M_SetupNextMenu(&SP_NightsAttackDef);
	if (!M_CanShowLevelInList(cv_nextmap.value-1, -1) && levelselect.rows[0].maplist[0])
		CV_SetValue(&cv_nextmap, levelselect.rows[0].maplist[0]);
	else
		Nextmap_OnChange();

	itemOn = nastart; // "Start" is selected.
}

// Player has selected the "START" from the nights attack screen
static void M_ChooseNightsAttack(INT32 choice)
{
	char *gpath;
	const size_t glen = strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char nameofdemo[256];
	(void)choice;
	emeralds = 0;
	memset(&luabanks, 0, sizeof(luabanks));
	M_ClearMenus(true);
	modeattacking = ATTACKING_NIGHTS;

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"replay"PATHSEP"%s"PATHSEP"%s", timeattackfolder, G_BuildMapName(cv_nextmap.value));
	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-last", gpath, skins[cv_chooseskin.value-1].name);

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), (UINT8)(cv_chooseskin.value-1), false, false);
}

// Player has selected the "START" from the time attack screen
static void M_ChooseTimeAttack(INT32 choice)
{
	char *gpath;
	const size_t glen = strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char nameofdemo[256];
	(void)choice;
	emeralds = 0;
	memset(&luabanks, 0, sizeof(luabanks));
	M_ClearMenus(true);
	modeattacking = ATTACKING_RECORD;

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	if ((gpath = malloc(glen)) == NULL)
		I_Error("Out of memory for replay filepath\n");

	sprintf(gpath,"replay"PATHSEP"%s"PATHSEP"%s", timeattackfolder, G_BuildMapName(cv_nextmap.value));
	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-last", gpath, skins[cv_chooseskin.value-1].name);

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), (UINT8)(cv_chooseskin.value-1), false, false);
}

// Player has selected the "REPLAY" from the time attack screen
static void M_ReplayTimeAttack(INT32 choice)
{
	const char *which;
	char *demoname;
	M_ClearMenus(true);
	modeattacking = ATTACKING_RECORD; // set modeattacking before G_DoPlayDemo so the map loader knows

	if (currentMenu == &SP_ReplayDef)
	{
		switch(choice) {
		default:
		case 0: // best score
			which = "score-best";
			break;
		case 1: // best time
			which = "time-best";
			break;
		case 2: // best rings
			which = "rings-best";
			break;
		case 3: // last
			which = "last";
			break;
		case 4: // guest
			// srb2/replay/main/map01-guest.lmp
			G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
			return;
		}
		// srb2/replay/main/map01-sonic-time-best.lmp
		G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), skins[cv_chooseskin.value-1].name, which));
	}
	else if (currentMenu == &SP_NightsReplayDef)
	{
		switch(choice) {
		default:
		case 0: // best score
			which = "score-best";
			break;
		case 1: // best time
			which = "time-best";
			break;
		case 2: // last
			which = "last";
			break;
		case 3: // guest
			G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
			return;
		}

		demoname = va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), skins[cv_chooseskin.value-1].name, which);

#ifdef OLDNREPLAYNAME // Check for old style named NiGHTS replay if a new style replay doesn't exist.
		if (!FIL_FileExists(demoname))
			demoname = va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), which);
#endif

		G_DoPlayDemo(demoname);
	}
}

static void M_EraseGuest(INT32 choice)
{
	const char *rguest = va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));

	if (choice == 'y' || choice == KEY_ENTER)
	{
		if (FIL_FileExists(rguest))
			remove(rguest);
	}
	M_SetupNextMenu(currentMenu->prevMenu->prevMenu);
	Nextmap_OnChange();
	M_StartMessage(M_GetText("Guest replay data erased.\n"),NULL,MM_NOTHING);
}

static void M_OverwriteGuest(const char *which)
{
	char *rguest = Z_StrDup(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
	UINT8 *buf;
	size_t len;
	len = FIL_ReadFile(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), skins[cv_chooseskin.value-1].name, which), &buf);

	if (!len) {
		return;
	}
	if (FIL_FileExists(rguest)) {
		M_StopMessage(0);
		remove(rguest);
	}
	FIL_WriteFile(rguest, buf, len);
	Z_Free(rguest);
	if (currentMenu == &SP_NightsGuestReplayDef)
		M_SetupNextMenu(&SP_NightsAttackDef);
	else
		M_SetupNextMenu(&SP_TimeAttackDef);
	Nextmap_OnChange();
	M_StartMessage(M_GetText("Guest replay data saved.\n"),NULL,MM_NOTHING);
}

static void M_OverwriteGuest_Time(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("time-best");
}

static void M_OverwriteGuest_Score(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("score-best");
}

static void M_OverwriteGuest_Rings(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("rings-best");
}

static void M_OverwriteGuest_Last(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("last");
}

static void M_SetGuestReplay(INT32 choice)
{
	void (*which)(INT32);
	if (currentMenu == &SP_NightsGuestReplayDef && choice >= 2)
		choice++; // skip best rings
	switch(choice)
	{
	case 0: // best score
		which = M_OverwriteGuest_Score;
		break;
	case 1: // best time
		which = M_OverwriteGuest_Time;
		break;
	case 2: // best rings
		which = M_OverwriteGuest_Rings;
		break;
	case 3: // last
		which = M_OverwriteGuest_Last;
		break;
	case 4: // guest
	default:
		M_StartMessage(M_GetText("Are you sure you want to\ndelete the guest replay data?\n\n(Press 'Y' to confirm)\n"),M_EraseGuest,MM_YESNO);
		return;
	}
	if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value))))
		M_StartMessage(M_GetText("Are you sure you want to\noverwrite the guest replay data?\n\n(Press 'Y' to confirm)\n"),which,MM_YESNO);
	else
		which(0);
}

void M_ModeAttackRetry(INT32 choice)
{
	(void)choice;
	// todo -- maybe seperate this out and G_SetRetryFlag() here instead? is just calling this from the menu 100% safe?
	G_CheckDemoStatus(); // Cancel recording
	if (modeattacking == ATTACKING_RECORD)
		M_ChooseTimeAttack(0);
	else if (modeattacking == ATTACKING_NIGHTS)
		M_ChooseNightsAttack(0);
}

static void M_ModeAttackEndGame(INT32 choice)
{
	(void)choice;
	G_CheckDemoStatus(); // Cancel recording

	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
		Command_ExitGame_f();

	M_StartControlPanel();
	switch(modeattacking)
	{
	default:
	case ATTACKING_RECORD:
		currentMenu = &SP_TimeAttackDef;
		wipetypepost = menupres[MN_SP_TIMEATTACK].enterwipe;
		break;
	case ATTACKING_NIGHTS:
		currentMenu = &SP_NightsAttackDef;
		wipetypepost = menupres[MN_SP_NIGHTSATTACK].enterwipe;
		break;
	}
	itemOn = currentMenu->lastOn;
	G_SetGamestate(GS_TIMEATTACK);
	modeattacking = ATTACKING_NONE;
	M_ChangeMenuMusic("_title", true);
	Nextmap_OnChange();
}

static void M_MarathonLiveEventBackup(INT32 choice)
{
	if (choice == 'y' || choice == KEY_ENTER)
	{
		marathonmode = MA_INIT;
		G_LoadGame(MARATHONSLOT, 0);
		cursaveslot = MARATHONSLOT;
		if (!(marathonmode & MA_RUNNING))
			marathonmode = 0;
		return;
	}

	M_StopMessage(0);
	stopstopmessage = true;

	if (choice == KEY_DEL)
	{
		if (FIL_FileExists(liveeventbackup)) // just in case someone deleted it while we weren't looking.
			remove(liveeventbackup);
		BwehHehHe();
		M_StartMessage("Live event backup erased.\n",M_Marathon,MM_NOTHING);
		return;
	}

	M_Marathon(-1);
}

// Going to Marathon menu...
static void M_Marathon(INT32 choice)
{
	UINT8 skinset;
	INT32 mapnum = 0;

	if (choice != -1 && FIL_FileExists(liveeventbackup))
	{
		M_StartMessage(\
			"\x82Live event backup detected.\n\x80\
			Do you want to resurrect the last run?\n\
			(Fs in chat if we crashed on stream.)\n\
			\n\
			Press 'Y' or 'Enter' to resume,\n\
			'Del' to delete, or any other\n\
			key to continue to Marathon Run.",M_MarathonLiveEventBackup,MM_YESNO);
		return;
	}

	fromlevelselect = false;

	startmap = spmarathon_start;
	CV_SetValue(&cv_newgametype, GT_COOP); // Graue 09-08-2004

	skinset = M_SetupChoosePlayerDirect(-1);

	SP_MarathonMenu[marathonplayer].status = (skinset == MAXSKINS) ? IT_KEYHANDLER|IT_STRING : IT_NOTHING|IT_DISABLED;

	while (mapnum < NUMMAPS)
	{
		if (mapheaderinfo[mapnum])
		{
			if (mapheaderinfo[mapnum]->cutscenenum || mapheaderinfo[mapnum]->precutscenenum)
				break;
		}
		mapnum++;
	}

	SP_MarathonMenu[marathoncutscenes].status = (mapnum < NUMMAPS) ? IT_CVAR|IT_STRING : IT_NOTHING|IT_DISABLED;

	M_ChangeMenuMusic("spec8", true);

	SP_MarathonDef.prevMenu = &MainDef;
	G_SetGamestate(GS_TIMEATTACK); // do this before M_SetupNextMenu so that menu meta state knows that we're switching
	titlemapinaction = TITLEMAP_OFF; // Nope don't give us HOMs please
	M_SetupNextMenu(&SP_MarathonDef);
	itemOn = marathonstart; // "Start" is selected.
	recatkdrawtimer = 50-8;
	char_scroll = 0;
}

static void M_HandleMarathonChoosePlayer(INT32 choice)
{
	INT32 selectval;

	if (keydown > 1)
		return;

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			break;
		case KEY_UPARROW:
			M_PrevOpt();
			break;

		case KEY_LEFTARROW:
			if ((selectval = description[char_on].prev) == char_on)
				return;
			char_on = selectval;
			break;
		case KEY_RIGHTARROW:
			if ((selectval = description[char_on].next) == char_on)
				return;
			char_on = selectval;
			break;

		case KEY_ESCAPE:
			noFurtherInput = true;
			M_GoBack(0);
			return;

		default:
			return;
	}
	S_StartSound(NULL, sfx_menu1);
}

static void M_StartMarathon(INT32 choice)
{
	(void)choice;
	marathontime = 0;
	marathonmode = MA_RUNNING|MA_INIT;
	cursaveslot = (cv_dummymarathon.value == 1) ? MARATHONSLOT : 0;
	if (!cv_dummycutscenes.value)
		marathonmode |= MA_NOCUTSCENES;
	if (cv_dummyloadless.value)
		marathonmode |= MA_INGAME;
	M_ChoosePlayer(char_on);
}

// Drawing function for Marathon menu
void M_DrawMarathon(void)
{
	INT32 i, x, y, cursory = 0, cnt, soffset = 0, w;
	UINT16 dispstatus;
	consvar_t *cv;
	const char *cvstring;
	char *work;
	angle_t fa;
	INT32 dupz = (vid.dupx < vid.dupy ? vid.dupx : vid.dupy), xspan = (vid.width/dupz), yspan = (vid.height/dupz), diffx = (xspan - BASEVIDWIDTH)/2, diffy = (yspan - BASEVIDHEIGHT)/2, maxy = BASEVIDHEIGHT + diffy;

	curbgxspeed = 0;
	curbgyspeed = 18;

	M_ChangeMenuMusic("spec8", true); // Eww, but needed for when user hits escape during demo playback

	V_DrawFill(-diffx, -diffy, diffx+(BASEVIDWIDTH-190)/2, yspan, 158);
	V_DrawFill((BASEVIDWIDTH-190)/2, -diffy, 190, yspan, 31);
	V_DrawFill((BASEVIDWIDTH+190)/2, -diffy, diffx+(BASEVIDWIDTH-190)/2, yspan, 158);
	//M_DrawRecordAttackForeground();
	if (curfadevalue)
		V_DrawFadeScreen(0xFF00, curfadevalue);

	x = (((BASEVIDWIDTH-82)/2)+11)<<FRACBITS;
	y = (((BASEVIDHEIGHT-82)/2)+12-10)<<FRACBITS;

	cnt = (36*(recatkdrawtimer<<FRACBITS))/TICRATE;
	fa = (FixedAngle(cnt)>>ANGLETOFINESHIFT) & FINEMASK;
	y -= (10*FINECOSINE(fa));

	recatkdrawtimer++;

	soffset = cnt = (recatkdrawtimer%50);
	if (!useBlackRock)
	{
		if (cnt > 8)
			cnt = 8;
		V_DrawFixedPatch(x+(6<<FRACBITS), y, FRACUNIT/2, (cnt&~1)<<(V_ALPHASHIFT-1), W_CachePatchName("RECCLOCK", PU_PATCH), NULL);
	}
	else if (cnt > 8)
	{
		cnt = 8;
		V_DrawFixedPatch(x, y, FRACUNIT, V_TRANSLUCENT, W_CachePatchName("ENDEGRK5", PU_PATCH), NULL);
	}
	else
	{
		V_DrawFixedPatch(x, y, FRACUNIT, cnt<<V_ALPHASHIFT, W_CachePatchName("ROID0000", PU_PATCH), NULL);
		V_DrawFixedPatch(x, y, FRACUNIT, V_TRANSLUCENT, W_CachePatchName("ENDEGRK5", PU_PATCH), NULL);
		V_DrawFixedPatch(x, y, FRACUNIT, cnt<<V_ALPHASHIFT, W_CachePatchName("ENDEGRK0", PU_PATCH), NULL);
	}

	{
		UINT8 col;
		i = 0;
		w = (((8-cnt)+1)/3)+1;
		w *= w;
		cursory = 0;
		while (i < cnt)
		{
			i++;
			col = 158+((cnt-i)/3);
			if (col >= 160)
				col = 253;
			V_DrawFill(((BASEVIDWIDTH-190)/2)-cursory-w, -diffy, w, yspan, col);
			V_DrawFill(((BASEVIDWIDTH+190)/2)+cursory,   -diffy, w, yspan, col);
			cursory += w;
			w *= 2;
		}
	}

	w = char_scroll + (((8-cnt)*(8-cnt))<<(FRACBITS-5));
	if (soffset == 50-1)
		w += FRACUNIT/2;

	{
		patch_t *fg = W_CachePatchName("RECATKFG", PU_PATCH);
		INT32 trans = V_60TRANS+((cnt&~3)<<(V_ALPHASHIFT-2));
		INT32 height = fg->height / 2;
		char patchname[7] = "CEMGx0";

		dupz = (w*7)/6; //(w*42*120)/(360*6); -- I don't know why this works but I'm not going to complain.
		dupz = ((dupz>>FRACBITS) % height);
		y = height/2;
		while (y+dupz >= -diffy)
			y -= height;
		while (y-2-dupz < maxy)
		{
			V_DrawFixedPatch(((BASEVIDWIDTH-190)<<(FRACBITS-1)), (y-2-dupz)<<FRACBITS, FRACUNIT/2, trans, fg, NULL);
			V_DrawFixedPatch(((BASEVIDWIDTH+190)<<(FRACBITS-1)), (y+dupz)<<FRACBITS, FRACUNIT/2, trans|V_FLIP, fg, NULL);
			y += height;
		}

		trans = V_40TRANS+((cnt&~1)<<(V_ALPHASHIFT-1));

		for (i = 0; i < 7; ++i)
		{
			fa = (FixedAngle(w)>>ANGLETOFINESHIFT) & FINEMASK;
			x = (BASEVIDWIDTH<<(FRACBITS-1)) + (60*FINESINE(fa));
			y = ((BASEVIDHEIGHT+16-20)<<(FRACBITS-1)) - (60*FINECOSINE(fa));
			w += (360<<FRACBITS)/7;

			patchname[4] = 'A'+(char)i;
			V_DrawFixedPatch(x, y, FRACUNIT, trans, W_CachePatchName(patchname, PU_PATCH), NULL);
		}

		height = 18; // prevents the need for the next line
		//dupz = (w*height)/18;
		dupz = ((w>>FRACBITS) % height);
		y = dupz+(height/4);
		x = 105+dupz;
		while (y >= -diffy)
		{
			x -= height;
			y -= height;
		}
		while (y-dupz < maxy && x < (xspan/2))
		{
			V_DrawFill((BASEVIDWIDTH/2)-x-height, -diffy, height, diffy+y+height, 153);
			V_DrawFill((BASEVIDWIDTH/2)+x, (maxy-y)-height, height, height+y, 153);
			y += height;
			x += height;
		}
	}

	if (!soffset)
	{
		char_scroll += (360<<FRACBITS)/42; // like a clock, ticking at 42bpm!
		if (char_scroll >= 360<<FRACBITS)
			char_scroll -= 360<<FRACBITS;
		if (recatkdrawtimer > (10*TICRATE))
			recatkdrawtimer -= (10*TICRATE);
	}

	M_DrawMenuTitle();

	// draw menu (everything else goes on top of it)
	// Sadly we can't just use generic mode menus because we need some extra hacks
	x = currentMenu->x;
	y = currentMenu->y;

	dispstatus = (currentMenu->menuitems[marathonplayer].status & IT_DISPLAY);
	if (dispstatus == IT_STRING || dispstatus == IT_WHITESTRING)
	{
		soffset = 68;
		if (description[char_on].charpic->width >= 256)
			V_DrawTinyScaledPatch(224, 120, 0, description[char_on].charpic);
		else
			V_DrawSmallScaledPatch(224, 120, 0, description[char_on].charpic);
	}
	else
		soffset = 0;

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		dispstatus = (currentMenu->menuitems[i].status & IT_DISPLAY);
		if (dispstatus != IT_STRING && dispstatus != IT_WHITESTRING)
			continue;

		y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
		if (i == itemOn)
			cursory = y;

		V_DrawString(x, y, (dispstatus == IT_WHITESTRING) ? V_YELLOWMAP : 0 , currentMenu->menuitems[i].text);

		cv = NULL;
		cvstring = NULL;
		work = NULL;
		if ((currentMenu->menuitems[i].status & IT_TYPE) == IT_CVAR)
		{
			cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
			cvstring = cv->string;
		}
		else if (i == marathonplayer)
		{
			if (description[char_on].displayname[0])
			{
				work = Z_StrDup(description[char_on].displayname);
				cnt = 0;
				while (work[cnt])
				{
					if (work[cnt] == '\n')
						work[cnt] = ' ';
					cnt++;
				}
				cvstring = work;
			}
			else
				cvstring = description[char_on].skinname;
		}

		// Cvar specific handling
		if (cvstring)
		{
			INT32 flags = V_YELLOWMAP;
			if (cv == &cv_dummymarathon && cv->value == 2) // ultimate_selectable
				flags = V_REDMAP;

			// Should see nothing but strings
			if (cv == &cv_dummymarathon && cv->value == 1)
			{
				w = V_ThinStringWidth(cvstring, 0);
				V_DrawThinString(BASEVIDWIDTH - x - soffset - w, y+1, flags, cvstring);
			}
			else
			{
				w = V_StringWidth(cvstring, 0);
				V_DrawString(BASEVIDWIDTH - x - soffset - w, y, flags, cvstring);
			}
			if (i == itemOn)
			{
				V_DrawCharacter(BASEVIDWIDTH - x - soffset - 10 - w - (skullAnimCounter/5), y,
					'\x1C' | V_YELLOWMAP, false);
				V_DrawCharacter(BASEVIDWIDTH - x - soffset + 2 + (skullAnimCounter/5), y,
					'\x1D' | V_YELLOWMAP, false);
			}
			if (work)
				Z_Free(work);
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_PATCH));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	// Draw press ESC to exit string on main record attack menu
	V_DrawString(104-72, 180, V_TRANSLUCENT, M_GetText("Press ESC to exit"));
}

// ========
// END GAME
// ========

static void M_ExitGameResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	//Command_ExitGame_f();
	G_SetExitGameFlag();
	M_ClearMenus(true);
}

static void M_EndGame(INT32 choice)
{
	(void)choice;
	if (demoplayback || demorecording)
		return;

	if (!Playing())
		return;

	M_StartMessage(M_GetText("Are you sure you want to end the game?\n\n(Press 'Y' to confirm)\n"), M_ExitGameResponse, MM_YESNO);
}

//===========================================================================
// Connect Menu
//===========================================================================

#define SERVERHEADERHEIGHT 44
#define SERVERLINEHEIGHT 12

#define S_LINEY(n) currentMenu->y + SERVERHEADERHEIGHT + (n * SERVERLINEHEIGHT)

#ifndef NONET
static UINT32 localservercount;

static void M_HandleServerPage(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_BACKSPACE:
		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_ENTER:
		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			if ((serverlistpage + 1) * SERVERS_PER_PAGE < serverlistcount)
				serverlistpage++;
			break;
		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			if (serverlistpage > 0)
				serverlistpage--;
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

static void M_Connect(INT32 choice)
{
	// do not call menuexitfunc
	M_ClearMenus(false);

	COM_BufAddText(va("connect node %d\n", serverlist[choice-FIRSTSERVERLINE + serverlistpage * SERVERS_PER_PAGE].node));
}

static void M_Refresh(INT32 choice)
{
	(void)choice;

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Searching for servers...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

	// note: this is the one case where 0 is a valid room number
	// because it corresponds to "All"
	CL_UpdateServerList(!(ms_RoomId < 0), ms_RoomId);

	// first page of servers
	serverlistpage = 0;
}

static INT32 menuRoomIndex = 0;

static void M_DrawRoomMenu(void)
{
	static int frame = -12;
	int dot_frame;
	char text[4];

	const char *rmotd;
	const char *waiting_message;

	int dots;

	if (m_waiting_mode)
	{
		dot_frame = frame / 4;
		dots = dot_frame + 3;

		strcpy(text, "   ");

		if (dots > 0)
		{
			if (dot_frame < 0)
				dot_frame = 0;

			strncpy(&text[dot_frame], "...", min(dots, 3 - dot_frame));
		}

		if (++frame == 12)
			frame = -12;

		currentMenu->menuitems[0].text = text;
	}

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	V_DrawString(currentMenu->x - 16, currentMenu->y, V_YELLOWMAP, M_GetText("Select a room"));

	if (m_waiting_mode == M_NOT_WAITING)
	{
		M_DrawTextBox(144, 24, 20, 20);

		if (itemOn == 0)
			rmotd = M_GetText("Don't connect to the Master Server.");
		else
			rmotd = room_list[itemOn-1].motd;

		rmotd = V_WordWrap(0, 20*8, 0, rmotd);
		V_DrawString(144+8, 32, V_ALLOWLOWERCASE|V_RETURN8, rmotd);
	}

	if (m_waiting_mode)
	{
		// Display a little "please wait" message.
		M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
		if (m_waiting_mode == M_WAITING_VERSION)
			waiting_message = "Checking for updates...";
		else
			waiting_message = "Fetching room info...";
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, waiting_message);
		V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	}
}

static void M_DrawConnectMenu(void)
{
	UINT16 i;
	char *gt;
	INT32 numPages = (serverlistcount+(SERVERS_PER_PAGE-1))/SERVERS_PER_PAGE;

	for (i = FIRSTSERVERLINE; i < min(localservercount, SERVERS_PER_PAGE)+FIRSTSERVERLINE; i++)
		MP_ConnectMenu[i].status = IT_STRING | IT_SPACE;

	if (!numPages)
		numPages = 1;

	// Room name
	if (ms_RoomId < 0)
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_room].alphaKey,
		                         V_YELLOWMAP, (itemOn == mp_connect_room) ? "<Select to change>" : "<Unlisted Mode>");
	else
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_room].alphaKey,
		                         V_YELLOWMAP, room_list[menuRoomIndex].name);

	// Page num
	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_page].alphaKey,
	                         V_YELLOWMAP, va("%u of %d", serverlistpage+1, numPages));

	// Horizontal line!
	V_DrawFill(1, currentMenu->y+40, 318, 1, 0);

	if (serverlistcount <= 0)
		V_DrawString(currentMenu->x,currentMenu->y+SERVERHEADERHEIGHT, 0, "No servers found");
	else
	for (i = 0; i < min(serverlistcount - serverlistpage * SERVERS_PER_PAGE, SERVERS_PER_PAGE); i++)
	{
		INT32 slindex = i + serverlistpage * SERVERS_PER_PAGE;
		UINT32 globalflags = (serverlist[slindex].info.refusereason ? V_TRANSLUCENT : 0)
			|((itemOn == FIRSTSERVERLINE+i) ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE;

		V_DrawString(currentMenu->x, S_LINEY(i), globalflags, serverlist[slindex].info.servername);

		// Don't use color flags intentionally, the global yellow color will auto override the text color code
		if (serverlist[slindex].info.modifiedgame)
			V_DrawSmallString(currentMenu->x+202, S_LINEY(i)+8, globalflags, "\x85" "Mod");
		if (serverlist[slindex].info.cheatsenabled)
			V_DrawSmallString(currentMenu->x+222, S_LINEY(i)+8, globalflags, "\x83" "Cheats");

		V_DrawSmallString(currentMenu->x, S_LINEY(i)+8, globalflags,
		                     va("Ping: %u", (UINT32)LONG(serverlist[slindex].info.time)));

		gt = serverlist[slindex].info.gametypename;

		V_DrawSmallString(currentMenu->x+46,S_LINEY(i)+8, globalflags,
		                         va("Players: %02d/%02d", serverlist[slindex].info.numberofplayer, serverlist[slindex].info.maxplayer));

		if (strlen(gt) > 11)
			gt = va("Gametype: %.11s...", gt);
		else
			gt = va("Gametype: %s", gt);

		V_DrawSmallString(currentMenu->x+112, S_LINEY(i)+8, globalflags, gt);

		MP_ConnectMenu[i+FIRSTSERVERLINE].status = IT_STRING | IT_CALL;
	}

	localservercount = serverlistcount;

	M_DrawGenericMenu();

	if (m_waiting_mode)
	{
		// Display a little "please wait" message.
		M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
		V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Searching for servers...");
		V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	}
}

static boolean M_CancelConnect(void)
{
	D_CloseConnection();
	return true;
}

// Ascending order, not descending.
// The casts are safe as long as the caller doesn't do anything stupid.
#define SERVER_LIST_ENTRY_COMPARATOR(key) \
static int ServerListEntryComparator_##key(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sa->info.key != sb->info.key) \
		return sa->info.key - sb->info.key; \
	return strcmp(sa->info.servername, sb->info.servername); \
}

// This does descending instead of ascending.
#define SERVER_LIST_ENTRY_COMPARATOR_REVERSE(key) \
static int ServerListEntryComparator_##key##_reverse(const void *entry1, const void *entry2) \
{ \
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2; \
	if (sb->info.key != sa->info.key) \
		return sb->info.key - sa->info.key; \
	return strcmp(sb->info.servername, sa->info.servername); \
}

SERVER_LIST_ENTRY_COMPARATOR(time)
SERVER_LIST_ENTRY_COMPARATOR(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(numberofplayer)
SERVER_LIST_ENTRY_COMPARATOR_REVERSE(maxplayer)

static int ServerListEntryComparator_gametypename(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;
	int c;
	if (( c = strcasecmp(sa->info.gametypename, sb->info.gametypename) ))
		return c;
	return strcmp(sa->info.servername, sb->info.servername); \
}

// Special one for modified state.
static int ServerListEntryComparator_modified(const void *entry1, const void *entry2)
{
	const serverelem_t *sa = (const serverelem_t*)entry1, *sb = (const serverelem_t*)entry2;

	// Modified acts as 2 points, cheats act as one point.
	int modstate_a = (sa->info.cheatsenabled ? 1 : 0) | (sa->info.modifiedgame ? 2 : 0);
	int modstate_b = (sb->info.cheatsenabled ? 1 : 0) | (sb->info.modifiedgame ? 2 : 0);

	if (modstate_a != modstate_b)
		return modstate_a - modstate_b;

	// Default to strcmp.
	return strcmp(sa->info.servername, sb->info.servername);
}
#endif

void M_SortServerList(void)
{
#ifndef NONET
	switch(cv_serversort.value)
	{
	case 0:		// Ping.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_time);
		break;
	case 1:		// Modified state.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_modified);
		break;
	case 2:		// Most players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer_reverse);
		break;
	case 3:		// Least players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_numberofplayer);
		break;
	case 4:		// Max players.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_maxplayer_reverse);
		break;
	case 5:		// Gametype.
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametypename);
		break;
	}
#endif
}

#ifndef NONET
#ifdef UPDATE_ALERT
static boolean M_CheckMODVersion(int id)
{
	char updatestring[500];
	const char *updatecheck = GetMODVersion(id);
	if(updatecheck)
	{
		sprintf(updatestring, UPDATE_ALERT_STRING, VERSIONSTRING, updatecheck);
		M_StartMessage(updatestring, NULL, MM_NOTHING);
		return false;
	} else
		return true;
}
#endif/*UPDATE_ALERT*/

#if defined (MASTERSERVER) && defined (HAVE_THREADS)
static void
Check_new_version_thread (int *id)
{
	int hosting;
	int okay;

	okay = 0;

#ifdef UPDATE_ALERT
	if (M_CheckMODVersion(*id))
#endif
	{
		I_lock_mutex(&ms_QueryId_mutex);
		{
			okay = ( *id == ms_QueryId );
		}
		I_unlock_mutex(ms_QueryId_mutex);

		if (okay)
		{
			I_lock_mutex(&m_menu_mutex);
			{
				m_waiting_mode = M_WAITING_ROOMS;
				hosting = ( currentMenu->prevMenu == &MP_ServerDef );
			}
			I_unlock_mutex(m_menu_mutex);

			GetRoomsList(hosting, *id);
		}
	}
#ifdef UPDATE_ALERT
	else
	{
		I_lock_mutex(&ms_QueryId_mutex);
		{
			okay = ( *id == ms_QueryId );
		}
		I_unlock_mutex(ms_QueryId_mutex);
	}
#endif

	if (okay)
	{
		I_lock_mutex(&m_menu_mutex);
		{
			if (m_waiting_mode)
			{
				m_waiting_mode = M_NOT_WAITING;
				MP_RoomMenu[0].text = "<Offline Mode>";
			}
		}
		I_unlock_mutex(m_menu_mutex);
	}

	free(id);
}
#endif/*defined (MASTERSERVER) && defined (HAVE_THREADS)*/

static void M_ConnectMenu(INT32 choice)
{
	(void)choice;
	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	// first page of servers
	serverlistpage = 0;
	if (ms_RoomId < 0)
	{
		M_RoomMenu(0); // Select a room instead of staring at an empty list
		// This prevents us from returning to the modified game alert.
		currentMenu->prevMenu = &MP_MainDef;
	}
	else
		M_SetupNextMenu(&MP_ConnectDef);
	itemOn = 0;
	M_Refresh(0);
}

static void M_ConnectMenuModChecks(INT32 choice)
{
	(void)choice;
	// okay never mind we want to COMMUNICATE to the player pre-emptively instead of letting them try and then get confused when it doesn't work

	if (modifiedgame)
	{
		M_StartMessage(M_GetText("You have add-ons loaded.\nYou won't be able to join netgames!\n\nTo play online, restart the game\nand don't load any addons.\nSRB2 will automatically add\neverything you need when you join.\n\n(Press a key)\n"),M_ConnectMenu,MM_EVENTHANDLER);
		return;
	}

	M_ConnectMenu(-1);
}

UINT32 roomIds[NUM_LIST_ROOMS];

static void M_RoomMenu(INT32 choice)
{
	INT32 i;
#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	int *id;
#endif

	(void)choice;

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Fetching room info...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

	for (i = 1; i < NUM_LIST_ROOMS+1; ++i)
		MP_RoomMenu[i].status = IT_DISABLED;
	memset(roomIds, 0, sizeof(roomIds));

	MP_RoomDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_RoomDef);

#ifdef MASTERSERVER
#ifdef HAVE_THREADS
#ifdef UPDATE_ALERT
	m_waiting_mode = M_WAITING_VERSION;
#else/*UPDATE_ALERT*/
	m_waiting_mode = M_WAITING_ROOMS;
#endif/*UPDATE_ALERT*/

	MP_RoomMenu[0].text = "";

	id = malloc(sizeof *id);

	I_lock_mutex(&ms_QueryId_mutex);
	{
		*id = ms_QueryId;
	}
	I_unlock_mutex(ms_QueryId_mutex);

	I_spawn_thread("check-new-version",
			(I_thread_fn)Check_new_version_thread, id);
#else/*HAVE_THREADS*/
#ifdef UPDATE_ALERT
	if (M_CheckMODVersion(0))
#endif/*UPDATE_ALERT*/
	{
		GetRoomsList(currentMenu->prevMenu == &MP_ServerDef, 0);
	}
#endif/*HAVE_THREADS*/
#endif/*MASTERSERVER*/
}

static void M_ChooseRoom(INT32 choice)
{
#if defined (MASTERSERVER) && defined (HAVE_THREADS)
	I_lock_mutex(&ms_QueryId_mutex);
	{
		ms_QueryId++;
	}
	I_unlock_mutex(ms_QueryId_mutex);
#endif

	if (choice == 0)
		ms_RoomId = -1;
	else
	{
		ms_RoomId = roomIds[choice-1];
		menuRoomIndex = choice - 1;
	}

	serverlistpage = 0;
	/*
	We were on the Multiplayer menu? That means that we must have been trying to
	view the server browser, but we hadn't selected a room yet. So we need to go
	to the browser next, not back there.
	*/
	if (currentMenu->prevMenu == &MP_MainDef)
		M_SetupNextMenu(&MP_ConnectDef);
	else
		M_SetupNextMenu(currentMenu->prevMenu);

	if (currentMenu == &MP_ConnectDef)
		M_Refresh(0);
}
#endif //NONET

//===========================================================================
// Start Server Menu
//===========================================================================

static void M_StartServer(INT32 choice)
{
	boolean StartSplitScreenGame = (currentMenu == &MP_SplitServerDef);

	(void)choice;
	if (!StartSplitScreenGame)
		netgame = true;

	multiplayer = true;

	// Still need to reset devmode
	cv_debug = 0;

	if (demoplayback)
		G_StopDemo();
	if (metalrecording)
		G_StopMetalDemo();

	if (!StartSplitScreenGame)
	{
		D_MapChange(cv_nextmap.value, cv_newgametype.value, false, 1, 1, false, false);
		COM_BufAddText("dummyconsvar 1\n");
	}
	else // split screen
	{
		paused = false;
		SV_StartSinglePlayerServer();
		if (!splitscreen)
		{
			splitscreen = true;
			SplitScreen_OnChange();
		}
		D_MapChange(cv_nextmap.value, cv_newgametype.value, false, 1, 1, false, false);
	}

	M_ClearMenus(true);
}

static void M_DrawServerMenu(void)
{
	M_DrawGenericMenu();

#ifndef NONET
	// Room name
	if (currentMenu == &MP_ServerDef)
	{
		M_DrawLevelPlatterHeader(currentMenu->y - lsheadingheight/2, "Server settings", true, false);
		if (ms_RoomId < 0)
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ServerMenu[mp_server_room].alphaKey,
			                         V_YELLOWMAP, (itemOn == mp_server_room) ? "<Select to change>" : "<Unlisted Mode>");
		else
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ServerMenu[mp_server_room].alphaKey,
			                         V_YELLOWMAP, room_list[menuRoomIndex].name);
	}
#endif

	if (cv_nextmap.value)
	{
#ifndef NONET
#define imgheight MP_ServerMenu[mp_server_levelgt].alphaKey
#else
#define imgheight 100
#endif
		patch_t *PictureOfLevel;
		lumpnum_t lumpnum;
		char headerstr[40];

		sprintf(headerstr, "%s - %s", cv_newgametype.string, cv_nextmap.string);

		M_DrawLevelPlatterHeader(currentMenu->y + imgheight - 10 - lsheadingheight/2, (const char *)headerstr, true, false);

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_PATCH);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_PATCH);

		V_DrawSmallScaledPatch(319 - (currentMenu->x + (PictureOfLevel->width / 2)), currentMenu->y + imgheight, 0, PictureOfLevel);
	}
}

static void M_MapChange(INT32 choice)
{
	(void)choice;

	MISC_ChangeLevelDef.prevMenu = currentMenu;
	levellistmode = LLM_CREATESERVER;

	if (Playing() && !(M_CanShowLevelOnPlatter(cv_nextmap.value-1, cv_newgametype.value)) && (M_CanShowLevelOnPlatter(gamemap-1, cv_newgametype.value)))
		CV_SetValue(&cv_nextmap, gamemap);

	if (!M_PrepareLevelPlatter(cv_newgametype.value, (currentMenu == &MPauseDef)))
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&MISC_ChangeLevelDef);
}

static void M_StartSplitServerMenu(INT32 choice)
{
	(void)choice;
	levellistmode = LLM_CREATESERVER;
	Newgametype_OnChange();
	M_SetupNextMenu(&MP_SplitServerDef);
}

static void M_ServerOptions(INT32 choice)
{
	(void)choice;

#ifndef NONET
	if ((splitscreen && !netgame) || currentMenu == &MP_SplitServerDef)
	{
		OP_ServerOptionsMenu[ 1].status = IT_GRAYEDOUT; // Server name
		OP_ServerOptionsMenu[ 2].status = IT_GRAYEDOUT; // Max players
		OP_ServerOptionsMenu[ 3].status = IT_GRAYEDOUT; // Allow add-on downloading
		OP_ServerOptionsMenu[ 4].status = IT_GRAYEDOUT; // Allow players to join
		OP_ServerOptionsMenu[36].status = IT_GRAYEDOUT; // Master server
		OP_ServerOptionsMenu[37].status = IT_GRAYEDOUT; // Minimum delay between joins
		OP_ServerOptionsMenu[38].status = IT_GRAYEDOUT; // Attempts to resynchronise
	}
	else
	{
		OP_ServerOptionsMenu[ 1].status = IT_STRING | IT_CVAR | IT_CV_STRING;
		OP_ServerOptionsMenu[ 2].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[ 3].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[ 4].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[36].status = (netgame
			? IT_GRAYEDOUT
			: (IT_STRING | IT_CVAR | IT_CV_STRING));
		OP_ServerOptionsMenu[37].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[38].status = IT_STRING | IT_CVAR;
	}
#endif

	/* Disable fading because of different menu head. */
	if (currentMenu == &OP_MainDef)/* from Options menu */
		OP_ServerOptionsDef.menuid = MTREE2(MN_OP_MAIN, MN_OP_SERVER);
	else/* from Multiplayer menu */
		OP_ServerOptionsDef.menuid = MTREE2(MN_MP_MAIN, MN_MP_SERVER_OPTIONS);

	OP_ServerOptionsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&OP_ServerOptionsDef);
}

#ifndef NONET
static void M_StartServerMenu(INT32 choice)
{
	(void)choice;
	ms_RoomId = -1;
	levellistmode = LLM_CREATESERVER;
	Newgametype_OnChange();
	M_SetupNextMenu(&MP_ServerDef);
	itemOn = 1;
}

// ==============
// CONNECT VIA IP
// ==============

static char setupm_ip[28];

// Draw the funky Connect IP menu. Tails 11-19-2002
// So much work for such a little thing!
static void M_DrawMPMainMenu(void)
{
	INT32 x = currentMenu->x;
	INT32 y = currentMenu->y;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	V_DrawRightAlignedString(BASEVIDWIDTH-x, y+66,
		((itemOn == 4) ? V_YELLOWMAP : 0), va("(2-%d players)", MAXPLAYERS));

	V_DrawRightAlignedString(BASEVIDWIDTH-x, y+76,
		((itemOn == 5) ? V_YELLOWMAP : 0), "(2 players)");

	V_DrawRightAlignedString(BASEVIDWIDTH-x, y+116,
		((itemOn == 8) ? V_YELLOWMAP : 0), "(splitscreen)");

	y += 22;

	V_DrawFill(x+5, y+4+5, /*16*8 + 6,*/ BASEVIDWIDTH - 2*(x+5), 8+6, 159);

	// draw name string
	V_DrawString(x+8,y+12, V_ALLOWLOWERCASE, setupm_ip);

	// draw text cursor for name
	if (itemOn == 2 //0
	    && skullAnimCounter < 4)   //blink cursor
		V_DrawCharacter(x+8+V_StringWidth(setupm_ip, V_ALLOWLOWERCASE),y+12,'_',false);
}

// Tails 11-19-2002
static void M_ConnectIP(INT32 choice)
{
	(void)choice;

	if (*setupm_ip == 0)
	{
		M_StartMessage("You must specify an IP address.\n", NULL, MM_NOTHING);
		return;
	}

	M_ClearMenus(true);

	COM_BufAddText(va("connect \"%s\"\n", setupm_ip));

	// A little "please wait" message.
	M_DrawTextBox(56, BASEVIDHEIGHT/2-12, 24, 2);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Connecting to server...");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer
}

// Tails 11-19-2002
static void M_HandleConnectIP(INT32 choice)
{
	size_t l;
	boolean exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL,sfx_menu1); // Tails
			break;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL,sfx_menu1); // Tails
			break;

		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			M_ConnectIP(1);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			if ((l = strlen(setupm_ip)) != 0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l-1] = 0;
			}
			break;

		case KEY_DEL:
			if (setupm_ip[0] && !shiftdown) // Shift+Delete is used for something else.
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[0] = 0;
			}
			if (!shiftdown) // Shift+Delete is used for something else.
				break;

			/* FALLTHRU */
		default:
			l = strlen(setupm_ip);

			if ( ctrldown ) {
				switch (choice) {
					case 'v':
					case 'V': // ctrl+v, pasting
					{
						const char *paste = I_ClipboardPaste();

						if (paste != NULL) {
							strncat(setupm_ip, paste, 28-1 - l); // Concat the ip field with clipboard
							if (strlen(paste) != 0) // Don't play sound if nothing was pasted
								S_StartSound(NULL,sfx_menu1); // Tails
						}

						break;
					}
					case KEY_INS:
					case 'c':
					case 'C': // ctrl+c, ctrl+insert, copying
						I_ClipboardCopy(setupm_ip, l);
						S_StartSound(NULL,sfx_menu1); // Tails
						break;

					case 'x':
					case 'X': // ctrl+x, cutting
						I_ClipboardCopy(setupm_ip, l);
						S_StartSound(NULL,sfx_menu1); // Tails
						setupm_ip[0] = 0;
						break;

					default: // otherwise do nothing
						break;
				}
				break; // don't check for typed keys
			}

			if ( shiftdown ) {
				switch (choice) {
					case KEY_INS: // shift+insert, pasting
						{
							const char *paste = I_ClipboardPaste();

							if (paste != NULL) {
								strncat(setupm_ip, paste, 28-1 - l); // Concat the ip field with clipboard
								if (strlen(paste) != 0) // Don't play sound if nothing was pasted
									S_StartSound(NULL,sfx_menu1); // Tails
							}

							break;
						}
					case KEY_DEL: // shift+delete, cutting
						I_ClipboardCopy(setupm_ip, l);
						S_StartSound(NULL,sfx_menu1); // Tails
						setupm_ip[0] = 0;
						break;
					default: // otherwise do nothing.
						break;
				}
			}

			if (l >= 28-1)
				break;

			// Rudimentary number and period enforcing - also allows letters so hostnames can be used instead
			if ((choice >= '-' && choice <= ':') || (choice >= 'A' && choice <= 'Z') || (choice >= 'a' && choice <= 'z'))
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] = (char)choice;
				setupm_ip[l+1] = 0;
			}
			else if (choice >= 199 && choice <= 211 && choice != 202 && choice != 206) //numpad too!
			{
				char keypad_translation[] = {'7','8','9','-','4','5','6','+','1','2','3','0','.'};
				choice = keypad_translation[choice - 199];
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] = (char)choice;
				setupm_ip[l+1] = 0;
			}

			break;
	}

	if (exitmenu)
	{
		currentMenu->lastOn = itemOn;
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}
#endif //!NONET

// ========================
// MULTIPLAYER PLAYER SETUP
// ========================
// Tails 03-02-2002

static UINT8      multi_tics;
static UINT8      multi_frame;
static UINT8      multi_spr2;

// this is set before entering the MultiPlayer setup menu,
// for either player 1 or 2
static char         setupm_name[MAXPLAYERNAME+1];
static player_t    *setupm_player;
static consvar_t   *setupm_cvskin;
static consvar_t   *setupm_cvcolor;
static consvar_t   *setupm_cvname;
static consvar_t   *setupm_cvdefaultskin;
static consvar_t   *setupm_cvdefaultcolor;
static INT32        setupm_fakeskin;
static menucolor_t *setupm_fakecolor;

static void M_DrawSetupMultiPlayerMenu(void)
{
	INT32 x, y, cursory = 0, flags = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	UINT8 *colormap;

	x = MP_PlayerSetupDef.x;
	y = MP_PlayerSetupDef.y;

	// use generic drawer for cursor, items and title
	//M_DrawGenericMenu();

	// draw title (or big pic)
	M_DrawMenuTitle();

	M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), "Name", true, false);
	if (itemOn == 0)
		cursory = y;
	y += 11;

	// draw name string
	V_DrawFill(x, y, 282/*(MAXPLAYERNAME+1)*8+6*/, 14, 159);
	V_DrawString(x + 8, y + 3, V_ALLOWLOWERCASE, setupm_name);
	if (skullAnimCounter < 4 && itemOn == 0)
		V_DrawCharacter(x + 8 + V_StringWidth(setupm_name, V_ALLOWLOWERCASE), y + 3,
			'_' | 0x80, false);

	y += 20;

	M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), "Character", true, false);
	if (itemOn == 1)
		cursory = y;

	// draw skin string
	V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
	             ((MP_PlayerSetupMenu[1].status & IT_TYPE) == IT_SPACE ? V_TRANSLUCENT : 0)|(itemOn == 1 ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE,
	             skins[setupm_fakeskin].realname);

	if (itemOn == 1 && (MP_PlayerSetupMenu[1].status & IT_TYPE) != IT_SPACE)
	{
		V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(skins[setupm_fakeskin].realname, V_ALLOWLOWERCASE) - (skullAnimCounter/5), y,
			'\x1C' | V_YELLOWMAP, false);
		V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
			'\x1D' | V_YELLOWMAP, false);
	}

	x = BASEVIDWIDTH/2;
	y += 11;

	// anim the player in the box
	if (--multi_tics <= 0)
	{
		multi_frame++;
		multi_tics = 4;
	}

#define charw 74

	// draw box around character
	V_DrawFill(x-(charw/2), y, charw, 84, 159);

	sprdef = &skins[setupm_fakeskin].sprites[multi_spr2];

	if (!setupm_fakecolor->color || !sprdef->numframes) // should never happen but hey, who knows
		goto faildraw;

	// ok, draw player sprite for sure now
	colormap = R_GetTranslationColormap(setupm_fakeskin, setupm_fakecolor->color, GTC_CACHE);

	if (multi_frame >= sprdef->numframes)
		multi_frame = 0;

	sprframe = &sprdef->spriteframes[multi_frame];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);
	if (sprframe->flip & 1) // Only for first sprite
		flags |= V_FLIP; // This sprite is left/right flipped!

#define chary (y+64)

	V_DrawFixedPatch(
		x<<FRACBITS,
		chary<<FRACBITS,
		FixedDiv(skins[setupm_fakeskin].highresscale, skins[setupm_fakeskin].shieldscale),
		flags, patch, colormap);

	goto colordraw;

faildraw:
	sprdef = &sprites[SPR_UNKN];
	if (!sprdef->numframes) // No frames ??
		return; // Can't render!

	sprframe = &sprdef->spriteframes[0];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_PATCH);
	if (sprframe->flip & 1) // Only for first sprite
		flags |= V_FLIP; // This sprite is left/right flipped!

	V_DrawScaledPatch(x, chary, flags, patch);

#undef chary

colordraw:
	x = MP_PlayerSetupDef.x;
	y += 75;

	M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), "Color", true, false);
	if (itemOn == 2)
		cursory = y;

	// draw color string
	V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
	             ((MP_PlayerSetupMenu[2].status & IT_TYPE) == IT_SPACE ? V_TRANSLUCENT : 0)|(itemOn == 2 ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE,
	             skincolors[setupm_fakecolor->color].name);

	if (itemOn == 2 && (MP_PlayerSetupMenu[2].status & IT_TYPE) != IT_SPACE)
	{
		V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(skincolors[setupm_fakecolor->color].name, V_ALLOWLOWERCASE) - (skullAnimCounter/5), y,
			'\x1C' | V_YELLOWMAP, false);
		V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
			'\x1D' | V_YELLOWMAP, false);
	}

	y += 11;

#define indexwidth 8
	{
		const INT32 numcolors = (282-charw)/(2*indexwidth); // Number of colors per side
		INT32 w = indexwidth; // Width of a singular color block
		menucolor_t *mc = setupm_fakecolor->prev; // Last accessed color
		UINT8 h;
		INT16 i;

		// Draw color in the middle
		x += numcolors*w;
		for (h = 0; h < 16; h++)
			V_DrawFill(x, y+h, charw, 1, skincolors[setupm_fakecolor->color].ramp[h]);

		//Draw colors from middle to left
		for (i=0; i<numcolors; i++) {
			x -= w;
			// Find accessible color before this one
			while (!skincolors[mc->color].accessible)
				mc = mc->prev;
			for (h = 0; h < 16; h++)
				V_DrawFill(x, y+h, w, 1, skincolors[mc->color].ramp[h]);
			mc = mc->prev;
		}

		// Draw colors from middle to right
		mc = setupm_fakecolor->next;
		x += numcolors*w + charw;
		for (i=0; i<numcolors; i++) {
			// Find accessible color after this one
			while (!skincolors[mc->color].accessible)
				mc = mc->next;
			for (h = 0; h < 16; h++)
				V_DrawFill(x, y+h, w, 1, skincolors[mc->color].ramp[h]);
			x += w;
			mc = mc->next;
		}
	}
#undef charw
#undef indexwidth

	x = MP_PlayerSetupDef.x;
	y += 20;

	V_DrawString(x, y,
		((R_SkinAvailable(setupm_cvdefaultskin->string) != setupm_fakeskin
		|| setupm_cvdefaultcolor->value != setupm_fakecolor->color)
			? 0
			: V_TRANSLUCENT)
		| ((itemOn == 3) ? V_YELLOWMAP : 0),
		"Save as default");
	if (itemOn == 3)
		cursory = y;

	V_DrawScaledPatch(x - 17, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}

// Handle 1P/2P MP Setup
static void M_HandleSetupMultiPlayer(INT32 choice)
{
	size_t   l;
	INT32 prev_setupm_fakeskin;
	boolean  exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			M_NextOpt();
			S_StartSound(NULL,sfx_menu1); // Tails
			break;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL,sfx_menu1); // Tails
			break;

		case KEY_LEFTARROW:
			if (itemOn == 1)       //player skin
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				prev_setupm_fakeskin = setupm_fakeskin;
				do
				{
					setupm_fakeskin--;
					if (setupm_fakeskin < 0)
						setupm_fakeskin = numskins-1;
				}
				while ((prev_setupm_fakeskin != setupm_fakeskin) && !(R_SkinUsable(-1, setupm_fakeskin)));
				multi_spr2 = P_GetSkinSprite2(&skins[setupm_fakeskin], SPR2_WALK, NULL);
			}
			else if (itemOn == 2) // player color
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakecolor = setupm_fakecolor->prev;
			}
			break;

		case KEY_ENTER:
			if (itemOn == 3
			&& (R_SkinAvailable(setupm_cvdefaultskin->string) != setupm_fakeskin
			|| setupm_cvdefaultcolor->value != setupm_fakecolor->color))
			{
				S_StartSound(NULL,sfx_strpst);
				// you know what? always putting these in the buffer won't hurt anything.
				COM_BufAddText (va("%s \"%s\"\n",setupm_cvdefaultskin->name,skins[setupm_fakeskin].name));
				COM_BufAddText (va("%s %d\n",setupm_cvdefaultcolor->name,setupm_fakecolor->color));
				break;
			}
			/* FALLTHRU */
		case KEY_RIGHTARROW:
			if (itemOn == 1)       //player skin
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				prev_setupm_fakeskin = setupm_fakeskin;
				do
				{
					setupm_fakeskin++;
					if (setupm_fakeskin > numskins-1)
						setupm_fakeskin = 0;
				}
				while ((prev_setupm_fakeskin != setupm_fakeskin) && !(R_SkinUsable(-1, setupm_fakeskin)));
				multi_spr2 = P_GetSkinSprite2(&skins[setupm_fakeskin], SPR2_WALK, NULL);
			}
			else if (itemOn == 2) // player color
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakecolor = setupm_fakecolor->next;
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			if (itemOn == 0 && (l = strlen(setupm_name))!=0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_name[l-1] = 0;
			}
			else if (itemOn == 2)
			{
				UINT16 col = skins[setupm_fakeskin].prefcolor;
				if ((setupm_fakecolor->color != col) && skincolors[col].accessible)
				{
					S_StartSound(NULL,sfx_menu1); // Tails
					for (setupm_fakecolor=menucolorhead;;setupm_fakecolor=setupm_fakecolor->next)
						if (setupm_fakecolor->color == col || setupm_fakecolor == menucolortail)
							break;
				}
			}
			break;
			break;

		case KEY_DEL:
			if (itemOn == 0 && (l = strlen(setupm_name))!=0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_name[0] = 0;
			}
			break;

		default:
			if (itemOn != 0 || choice < 32 || choice > 127)
				break;
			S_StartSound(NULL,sfx_menu1); // Tails
			l = strlen(setupm_name);
			if (l < MAXPLAYERNAME)
			{
				setupm_name[l] = (char)choice;
				setupm_name[l+1] = 0;
			}
			break;
	}

	// check color
	if (itemOn == 2 && !skincolors[setupm_fakecolor->color].accessible) {
		if (choice == KEY_LEFTARROW)
			while (!skincolors[setupm_fakecolor->color].accessible)
				setupm_fakecolor = setupm_fakecolor->prev;
		else if (choice == KEY_RIGHTARROW || choice == KEY_ENTER)
			while (!skincolors[setupm_fakecolor->color].accessible)
				setupm_fakecolor = setupm_fakecolor->next;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// start the multiplayer setup menu
static void M_SetupMultiPlayer(INT32 choice)
{
	(void)choice;

	multi_frame = 0;
	multi_tics = 4;
	strcpy(setupm_name, cv_playername.string);

	// set for player 1
	setupm_player = &players[consoleplayer];
	setupm_cvskin = &cv_skin;
	setupm_cvcolor = &cv_playercolor;
	setupm_cvname = &cv_playername;
	setupm_cvdefaultskin = &cv_defaultskin;
	setupm_cvdefaultcolor = &cv_defaultplayercolor;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;

	for (setupm_fakecolor=menucolorhead;;setupm_fakecolor=setupm_fakecolor->next)
		if (setupm_fakecolor->color == setupm_cvcolor->value || setupm_fakecolor == menucolortail)
			break;

	// disable skin changes if we can't actually change skins
	if (!CanChangeSkin(consoleplayer))
		MP_PlayerSetupMenu[1].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[1].status = (IT_KEYHANDLER|IT_STRING);

	// ditto with colour
	if (Playing() && G_GametypeHasTeams())
		MP_PlayerSetupMenu[2].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[2].status = (IT_KEYHANDLER|IT_STRING);

	multi_spr2 = P_GetSkinSprite2(&skins[setupm_fakeskin], SPR2_WALK, NULL);

	MP_PlayerSetupDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_PlayerSetupDef);
}

// start the multiplayer setup menu, for secondary player (splitscreen mode)
static void M_SetupMultiPlayer2(INT32 choice)
{
	(void)choice;

	multi_frame = 0;
	multi_tics = 4;
	strcpy (setupm_name, cv_playername2.string);

	// set for splitscreen secondary player
	setupm_player = &players[secondarydisplayplayer];
	setupm_cvskin = &cv_skin2;
	setupm_cvcolor = &cv_playercolor2;
	setupm_cvname = &cv_playername2;
	setupm_cvdefaultskin = &cv_defaultskin2;
	setupm_cvdefaultcolor = &cv_defaultplayercolor2;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;

	for (setupm_fakecolor=menucolorhead;;setupm_fakecolor=setupm_fakecolor->next)
		if (setupm_fakecolor->color == setupm_cvcolor->value || setupm_fakecolor == menucolortail)
			break;

	// disable skin changes if we can't actually change skins
	if (splitscreen && !CanChangeSkin(secondarydisplayplayer))
		MP_PlayerSetupMenu[1].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[1].status = (IT_KEYHANDLER | IT_STRING);

	// ditto with colour
	if (Playing() && G_GametypeHasTeams())
		MP_PlayerSetupMenu[2].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[2].status = (IT_KEYHANDLER|IT_STRING);

	multi_spr2 = P_GetSkinSprite2(&skins[setupm_fakeskin], SPR2_WALK, NULL);

	MP_PlayerSetupDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_PlayerSetupDef);
}

static boolean M_QuitMultiPlayerMenu(void)
{
	size_t l;
	// send name if changed
	if (strcmp(setupm_name, setupm_cvname->string))
	{
		// remove trailing whitespaces
		for (l= strlen(setupm_name)-1;
		    (signed)l >= 0 && setupm_name[l] ==' '; l--)
			setupm_name[l] =0;
		COM_BufAddText (va("%s \"%s\"\n",setupm_cvname->name,setupm_name));
	}
	COM_BufAddText (va("%s \"%s\"\n",setupm_cvskin->name,skins[setupm_fakeskin].name));
	// send color if changed
	if (setupm_fakecolor->color != setupm_cvcolor->value)
		COM_BufAddText (va("%s %d\n",setupm_cvcolor->name,setupm_fakecolor->color));
	return true;
}

void M_AddMenuColor(UINT16 color) {
	menucolor_t *c;

	if (color >= numskincolors) {
		CONS_Printf("M_AddMenuColor: color %d does not exist.",color);
		return;
	}

	c = (menucolor_t *)malloc(sizeof(menucolor_t));
	c->color = color;
	if (menucolorhead == NULL) {
		c->next = c;
		c->prev = c;
		menucolorhead = c;
		menucolortail = c;
	} else {
		c->next = menucolorhead;
		c->prev = menucolortail;
		menucolortail->next = c;
		menucolorhead->prev = c;
		menucolortail = c;
	}
}

void M_MoveColorBefore(UINT16 color, UINT16 targ) {
	menucolor_t *look, *c = NULL, *t = NULL;

	if (color == targ)
		return;
	if (color >= numskincolors) {
		CONS_Printf("M_MoveColorBefore: color %d does not exist.",color);
		return;
	}
	if (targ >= numskincolors) {
		CONS_Printf("M_MoveColorBefore: target color %d does not exist.",targ);
		return;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			c = look;
		else if (look->color == targ)
			t = look;
		if (c != NULL && t != NULL)
			break;
		if (look==menucolortail)
			return;
	}

	if (c == t->prev)
		return;

	if (t==menucolorhead)
		menucolorhead = c;
	if (c==menucolortail)
		menucolortail = c->prev;

	c->prev->next = c->next;
	c->next->prev = c->prev;

	c->prev = t->prev;
	c->next = t;
	t->prev->next = c;
	t->prev = c;
}

void M_MoveColorAfter(UINT16 color, UINT16 targ) {
	menucolor_t *look, *c = NULL, *t = NULL;

	if (color == targ)
		return;
	if (color >= numskincolors) {
		CONS_Printf("M_MoveColorAfter: color %d does not exist.\n",color);
		return;
	}
	if (targ >= numskincolors) {
		CONS_Printf("M_MoveColorAfter: target color %d does not exist.\n",targ);
		return;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			c = look;
		else if (look->color == targ)
			t = look;
		if (c != NULL && t != NULL)
			break;
		if (look==menucolortail)
			return;
	}

	if (t == c->prev)
		return;

	if (t==menucolortail)
		menucolortail = c;
	else if (c==menucolortail)
		menucolortail = c->prev;

	c->prev->next = c->next;
	c->next->prev = c->prev;

	c->next = t->next;
	c->prev = t;
	t->next->prev = c;
	t->next = c;
}

UINT16 M_GetColorBefore(UINT16 color) {
	menucolor_t *look;

	if (color >= numskincolors) {
		CONS_Printf("M_GetColorBefore: color %d does not exist.\n",color);
		return 0;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			return look->prev->color;
		if (look==menucolortail)
			return 0;
	}
}

UINT16 M_GetColorAfter(UINT16 color) {
	menucolor_t *look;

	if (color >= numskincolors) {
		CONS_Printf("M_GetColorAfter: color %d does not exist.\n",color);
		return 0;
	}

	for (look=menucolorhead;;look=look->next) {
		if (look->color == color)
			return look->next->color;
		if (look==menucolortail)
			return 0;
	}
}

void M_InitPlayerSetupColors(void) {
	UINT8 i;
	numskincolors = SKINCOLOR_FIRSTFREESLOT;
	menucolorhead = menucolortail = NULL;
	for (i=0; i<numskincolors; i++)
		M_AddMenuColor(i);
}

void M_FreePlayerSetupColors(void) {
	menucolor_t *look = menucolorhead, *tmp;

	if (menucolorhead==NULL)
		return;

	while (true) {
		if (look != menucolortail) {
			tmp = look;
			look = look->next;
			free(tmp);
		} else {
			free(look);
			return;
		}
	}

	menucolorhead = menucolortail = NULL;
}

// =================
// DATA OPTIONS MENU
// =================
static UINT8 erasecontext = 0;

static void M_EraseDataResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Delete the data
	if (erasecontext != 1)
		G_ClearRecords();
	if (erasecontext != 0)
		M_ClearSecrets();
	if (erasecontext == 2)
	{
		totalplaytime = 0;
		F_StartIntro();
	}
	BwehHehHe();
	M_ClearMenus(true);
}

static void M_EraseData(INT32 choice)
{
	const char *eschoice, *esstr = M_GetText("Are you sure you want to erase\n%s?\n\n(Press 'Y' to confirm)\n");

	erasecontext = (UINT8)choice;

	if (choice == 0)
		eschoice = M_GetText("Record Attack data");
	else if (choice == 1)
		eschoice = M_GetText("Extras data");
	else
		eschoice = M_GetText("ALL game data");

	M_StartMessage(va(esstr, eschoice),M_EraseDataResponse,MM_YESNO);
}

static void M_ScreenshotOptions(INT32 choice)
{
	(void)choice;
	Screenshot_option_Onchange();
	Moviemode_mode_Onchange();

	M_SetupScreenshotMenu();
	M_SetupNextMenu(&OP_ScreenshotOptionsDef);
}

static void M_SetupScreenshotMenu(void)
{
	menuitem_t *item = &OP_ScreenshotOptionsMenu[op_screenshot_colorprofile];

#ifdef HWRENDER
	// Hide some options based on render mode
	if (rendermode == render_opengl)
	{
		item->status = IT_GRAYEDOUT;
		if ((currentMenu == &OP_ScreenshotOptionsDef) && (itemOn == op_screenshot_colorprofile)) // Can't select that
			itemOn = op_screenshot_storagelocation;
	}
	else
#endif
		item->status = (IT_STRING | IT_CVAR);
}

// =============
// JOYSTICK MENU
// =============

// Start the controls menu, setting it up for either the console player,
// or the secondary splitscreen player

static void M_DrawJoystick(void)
{
	INT32 i, compareval2, compareval;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i <= MAX_JOYSTICKS; i++) // See MAX_JOYSTICKS
	{
		M_DrawTextBox(OP_JoystickSetDef.x-8, OP_JoystickSetDef.y+LINEHEIGHT*i-12, 28, 1);
		//M_DrawSaveLoadBorder(OP_JoystickSetDef.x+4, OP_JoystickSetDef.y+1+LINEHEIGHT*i);

#ifdef JOYSTICK_HOTPLUG
		if (atoi(cv_usejoystick2.string) > I_NumJoys())
			compareval2 = atoi(cv_usejoystick2.string);
		else
			compareval2 = cv_usejoystick2.value;

		if (atoi(cv_usejoystick.string) > I_NumJoys())
			compareval = atoi(cv_usejoystick.string);
		else
			compareval = cv_usejoystick.value;
#else
		compareval2 = cv_usejoystick2.value;
		compareval = cv_usejoystick.value;
#endif

		if ((setupcontrols_secondaryplayer && (i == compareval2))
			|| (!setupcontrols_secondaryplayer && (i == compareval)))
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i-4,V_GREENMAP,joystickInfo[i]);
		else
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i-4,0,joystickInfo[i]);

		if (i == itemOn)
		{
			V_DrawScaledPatch(currentMenu->x - 24, OP_JoystickSetDef.y+LINEHEIGHT*i-4, 0,
				W_CachePatchName("M_CURSOR", PU_PATCH));
		}
	}
}

void M_SetupJoystickMenu(INT32 choice)
{
	INT32 i = 0;
	const char *joyNA = "Unavailable";
	INT32 n = I_NumJoys();
	(void)choice;

	strcpy(joystickInfo[i], "None");

	for (i = 1; i <= MAX_JOYSTICKS; i++)
	{
		if (i <= n && (I_GetJoyName(i)) != NULL)
			strncpy(joystickInfo[i], I_GetJoyName(i), 28);
		else
			strcpy(joystickInfo[i], joyNA);

#ifdef JOYSTICK_HOTPLUG
		// We use cv_usejoystick.string as the USER-SET var
		// and cv_usejoystick.value as the INTERNAL var
		//
		// In practice, if cv_usejoystick.string == 0, this overrides
		// cv_usejoystick.value and always disables
		//
		// Update cv_usejoystick.string here so that the user can
		// properly change this value.
		if (i == cv_usejoystick.value)
			CV_SetValue(&cv_usejoystick, i);
		if (i == cv_usejoystick2.value)
			CV_SetValue(&cv_usejoystick2, i);
#endif
	}

	M_SetupNextMenu(&OP_JoystickSetDef);
}

static void M_Setup1PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = false;
	OP_JoystickSetDef.prevMenu = &OP_Joystick1Def;
	OP_JoystickSetDef.menuid &= ~(((1 << MENUBITS) - 1) << MENUBITS);
	OP_JoystickSetDef.menuid &= ~(((1 << MENUBITS) - 1) << (MENUBITS*2));
	OP_JoystickSetDef.menuid |= MN_OP_P1CONTROLS << MENUBITS;
	OP_JoystickSetDef.menuid |= MN_OP_P1JOYSTICK << (MENUBITS*2);
	M_SetupJoystickMenu(choice);
}

static void M_Setup2PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = true;
	OP_JoystickSetDef.prevMenu = &OP_Joystick2Def;
	OP_JoystickSetDef.menuid &= ~(((1 << MENUBITS) - 1) << MENUBITS);
	OP_JoystickSetDef.menuid &= ~(((1 << MENUBITS) - 1) << (MENUBITS*2));
	OP_JoystickSetDef.menuid |= MN_OP_P2CONTROLS << MENUBITS;
	OP_JoystickSetDef.menuid |= MN_OP_P2JOYSTICK << (MENUBITS*2);
	M_SetupJoystickMenu(choice);
}

static void M_AssignJoystick(INT32 choice)
{
#ifdef JOYSTICK_HOTPLUG
	INT32 oldchoice, oldstringchoice;
	INT32 numjoys = I_NumJoys();

	if (setupcontrols_secondaryplayer)
	{
		oldchoice = oldstringchoice = atoi(cv_usejoystick2.string) > numjoys ? atoi(cv_usejoystick2.string) : cv_usejoystick2.value;
		CV_SetValue(&cv_usejoystick2, choice);

		// Just in case last-minute changes were made to cv_usejoystick.value,
		// update the string too
		// But don't do this if we're intentionally setting higher than numjoys
		if (choice <= numjoys)
		{
			CV_SetValue(&cv_usejoystick2, cv_usejoystick2.value);

			// reset this so the comparison is valid
			if (oldchoice > numjoys)
				oldchoice = cv_usejoystick2.value;

			if (oldchoice != choice)
			{
				if (choice && oldstringchoice > numjoys) // if we did not select "None", we likely selected a used device
					CV_SetValue(&cv_usejoystick2, (oldstringchoice > numjoys ? oldstringchoice : oldchoice));

				if (oldstringchoice ==
					(atoi(cv_usejoystick2.string) > numjoys ? atoi(cv_usejoystick2.string) : cv_usejoystick2.value))
					M_StartMessage("This gamepad is used by another\n"
					               "player. Reset the gamepad\n"
					               "for that player first.\n\n"
					               "(Press a key)\n", NULL, MM_NOTHING);
			}
		}
	}
	else
	{
		oldchoice = oldstringchoice = atoi(cv_usejoystick.string) > numjoys ? atoi(cv_usejoystick.string) : cv_usejoystick.value;
		CV_SetValue(&cv_usejoystick, choice);

		// Just in case last-minute changes were made to cv_usejoystick.value,
		// update the string too
		// But don't do this if we're intentionally setting higher than numjoys
		if (choice <= numjoys)
		{
			CV_SetValue(&cv_usejoystick, cv_usejoystick.value);

			// reset this so the comparison is valid
			if (oldchoice > numjoys)
				oldchoice = cv_usejoystick.value;

			if (oldchoice != choice)
			{
				if (choice && oldstringchoice > numjoys) // if we did not select "None", we likely selected a used device
					CV_SetValue(&cv_usejoystick, (oldstringchoice > numjoys ? oldstringchoice : oldchoice));

				if (oldstringchoice ==
					(atoi(cv_usejoystick.string) > numjoys ? atoi(cv_usejoystick.string) : cv_usejoystick.value))
					M_StartMessage("This gamepad is used by another\n"
					               "player. Reset the gamepad\n"
					               "for that player first.\n\n"
					               "(Press a key)\n", NULL, MM_NOTHING);
			}
		}
	}
#else
	if (setupcontrols_secondaryplayer)
		CV_SetValue(&cv_usejoystick2, choice);
	else
		CV_SetValue(&cv_usejoystick, choice);
#endif
}

// =============
// CONTROLS MENU
// =============

static void M_Setup1PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = false;
	setupcontrols = gamecontrol;        // was called from main Options (for console player, then)
	currentMenu->lastOn = itemOn;

	// Unhide the nine non-P2 controls and their headers
	//OP_ChangeControlsMenu[18+0].status = IT_HEADER;
	//OP_ChangeControlsMenu[18+1].status = IT_SPACE;
	// ...
	OP_ChangeControlsMenu[18+2].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+3].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+4].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+5].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+6].status = IT_CALL|IT_STRING2;
	//OP_ChangeControlsMenu[18+7].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+8].status = IT_CALL|IT_STRING2;
	// ...
	OP_ChangeControlsMenu[27+0].status = IT_HEADER;
	OP_ChangeControlsMenu[27+1].status = IT_SPACE;
	// ...
	OP_ChangeControlsMenu[27+2].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[27+3].status = IT_CALL|IT_STRING2;

	OP_ChangeControlsDef.prevMenu = &OP_P1ControlsDef;
	OP_ChangeControlsDef.menuid &= ~(((1 << MENUBITS) - 1) << MENUBITS); // remove second level
	OP_ChangeControlsDef.menuid |= MN_OP_P1CONTROLS << MENUBITS; // combine second level
	M_SetupNextMenu(&OP_ChangeControlsDef);
}

static void M_Setup2PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = true;
	setupcontrols = gamecontrolbis;
	currentMenu->lastOn = itemOn;

	// Hide the nine non-P2 controls and their headers
	//OP_ChangeControlsMenu[18+0].status = IT_GRAYEDOUT2;
	//OP_ChangeControlsMenu[18+1].status = IT_GRAYEDOUT2;
	// ...
	OP_ChangeControlsMenu[18+2].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+3].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+4].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+5].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+6].status = IT_GRAYEDOUT2;
	//OP_ChangeControlsMenu[18+7].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+8].status = IT_GRAYEDOUT2;
	// ...
	OP_ChangeControlsMenu[27+0].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[27+1].status = IT_GRAYEDOUT2;
	// ...
	OP_ChangeControlsMenu[27+2].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[27+3].status = IT_GRAYEDOUT2;

	OP_ChangeControlsDef.prevMenu = &OP_P2ControlsDef;
	OP_ChangeControlsDef.menuid &= ~(((1 << MENUBITS) - 1) << MENUBITS); // remove second level
	OP_ChangeControlsDef.menuid |= MN_OP_P2CONTROLS << MENUBITS; // combine second level
	M_SetupNextMenu(&OP_ChangeControlsDef);
}

#define controlheight 18

// Draws the Customise Controls menu
static void M_DrawControl(void)
{
	char     tmp[50];
	INT32    x, y, i, max, cursory = 0, iter;
	INT32    keys[2];

	x = currentMenu->x;
	y = currentMenu->y;

	/*i = itemOn - (controlheight/2);
	if (i < 0)
		i = 0;
	*/

	iter = (controlheight/2);
	for (i = itemOn; ((iter || currentMenu->menuitems[i].status == IT_GRAYEDOUT2) && i > 0); i--)
	{
		if (currentMenu->menuitems[i].status != IT_GRAYEDOUT2)
			iter--;
	}
	if (currentMenu->menuitems[i].status == IT_GRAYEDOUT2)
		i--;

	iter += (controlheight/2);
	for (max = itemOn; (iter && max < currentMenu->numitems); max++)
	{
		if (currentMenu->menuitems[max].status != IT_GRAYEDOUT2)
			iter--;
	}

	if (iter)
	{
		iter += (controlheight/2);
		for (i = itemOn; ((iter || currentMenu->menuitems[i].status == IT_GRAYEDOUT2) && i > 0); i--)
		{
			if (currentMenu->menuitems[i].status != IT_GRAYEDOUT2)
				iter--;
		}
	}

	/*max = i + controlheight;
	if (max > currentMenu->numitems)
	{
		max = currentMenu->numitems;
		if (max < controlheight)
			i = 0;
		else
			i = max - controlheight;
	}*/

	// draw title (or big pic)
	M_DrawMenuTitle();

	if (tutorialmode && tutorialgcs)
	{
		if ((gametic / TICRATE) % 2)
			M_CentreText(30, "\202EXIT THE TUTORIAL TO CHANGE THE CONTROLS");
		else
			M_CentreText(30, "EXIT THE TUTORIAL TO CHANGE THE CONTROLS");
	}
	else
		M_CentreText(30,
		    (setupcontrols_secondaryplayer ? "SET CONTROLS FOR SECONDARY PLAYER" :
		                                     "PRESS ENTER TO CHANGE, BACKSPACE TO CLEAR"));

	if (i)
		V_DrawString(currentMenu->x - 16, y-(skullAnimCounter/5), V_YELLOWMAP, "\x1A"); // up arrow
	if (max != currentMenu->numitems)
		V_DrawString(currentMenu->x - 16, y+(SMALLLINEHEIGHT*(controlheight-1))+(skullAnimCounter/5), V_YELLOWMAP, "\x1B"); // down arrow

	for (; i < max; i++)
	{
		if (currentMenu->menuitems[i].status == IT_GRAYEDOUT2)
			continue;

		if (i == itemOn)
			cursory = y;

		if (currentMenu->menuitems[i].status == IT_CONTROL)
		{
			V_DrawString(x, y, ((i == itemOn) ? V_YELLOWMAP : 0), currentMenu->menuitems[i].text);
			keys[0] = setupcontrols[currentMenu->menuitems[i].alphaKey][0];
			keys[1] = setupcontrols[currentMenu->menuitems[i].alphaKey][1];

			tmp[0] ='\0';
			if (keys[0] == KEY_NULL && keys[1] == KEY_NULL)
			{
				strcpy(tmp, "---");
			}
			else
			{
				if (keys[0] != KEY_NULL)
					strcat (tmp, G_KeynumToString (keys[0]));

				if (keys[0] != KEY_NULL && keys[1] != KEY_NULL)
					strcat(tmp," or ");

				if (keys[1] != KEY_NULL)
					strcat (tmp, G_KeynumToString (keys[1]));


			}
			V_DrawRightAlignedString(BASEVIDWIDTH-currentMenu->x, y, V_YELLOWMAP, tmp);
		}
		/*else if (currentMenu->menuitems[i].status == IT_GRAYEDOUT2)
			V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);*/
		else if ((currentMenu->menuitems[i].status == IT_HEADER) && (i != max-1))
			M_DrawLevelPlatterHeader(y, currentMenu->menuitems[i].text, true, false);

		y += SMALLLINEHEIGHT;
	}

	V_DrawScaledPatch(currentMenu->x - 20, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}

#undef controlbuffer

static INT32 controltochange;
static char controltochangetext[33];

static void M_ChangecontrolResponse(event_t *ev)
{
	INT32        control;
	INT32        found;
	INT32        ch = ev->data1;

	// ESCAPE cancels; dummy out PAUSE
	if (ch != KEY_ESCAPE && ch != KEY_PAUSE)
	{

		switch (ev->type)
		{
			// ignore mouse/joy movements, just get buttons
			case ev_mouse:
			case ev_mouse2:
			case ev_joystick:
			case ev_joystick2:
				ch = KEY_NULL;      // no key
			break;

			// keypad arrows are converted for the menu in cursor arrows
			// so use the event instead of ch
			case ev_keydown:
				ch = ev->data1;
			break;

			default:
			break;
		}

		control = controltochange;

		// check if we already entered this key
		found = -1;
		if (setupcontrols[control][0] ==ch)
			found = 0;
		else if (setupcontrols[control][1] ==ch)
			found = 1;
		if (found >= 0)
		{
			// replace mouse and joy clicks by double clicks
			if (ch >= KEY_MOUSE1 && ch <= KEY_MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_MOUSE1+KEY_DBLMOUSE1;
			else if (ch >= KEY_JOY1 && ch <= KEY_JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_JOY1+KEY_DBLJOY1;
			else if (ch >= KEY_2MOUSE1 && ch <= KEY_2MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_2MOUSE1+KEY_DBL2MOUSE1;
			else if (ch >= KEY_2JOY1 && ch <= KEY_2JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_2JOY1+KEY_DBL2JOY1;
		}
		else
		{
			// check if change key1 or key2, or replace the two by the new
			found = 0;
			if (setupcontrols[control][0] == KEY_NULL)
				found++;
			if (setupcontrols[control][1] == KEY_NULL)
				found++;
			if (found == 2)
			{
				found = 0;
				setupcontrols[control][1] = KEY_NULL;  //replace key 1,clear key2
			}
			(void)G_CheckDoubleUsage(ch, true);
			setupcontrols[control][found] = ch;
		}
		S_StartSound(NULL, sfx_strpst);
	}
	else if (ch == KEY_PAUSE)
	{
		// This buffer assumes a 125-character message plus a 32-character control name (per controltochangetext buffer size)
		static char tmp[158];
		menu_t *prev = currentMenu->prevMenu;

		if (controltochange == gc_pause)
			sprintf(tmp, M_GetText("The \x82Pause Key \x80is enabled, but \nit cannot be used to retry runs \nduring Record Attack. \n\nHit another key for\n%s\nESC for Cancel"),
				controltochangetext);
		else
			sprintf(tmp, M_GetText("The \x82Pause Key \x80is enabled, but \nit is not configurable. \n\nHit another key for\n%s\nESC for Cancel"),
				controltochangetext);

		M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
		currentMenu->prevMenu = prev;

		S_StartSound(NULL, sfx_s3k42);
		return;
	}
	else
		S_StartSound(NULL, sfx_skid);

	M_StopMessage(0);
}

static void M_ChangeControl(INT32 choice)
{
	// This buffer assumes a 35-character message (per below) plus a max control name limit of 32 chars (per controltochangetext)
	// If you change the below message, then change the size of this buffer!
	static char tmp[68];

	if (tutorialmode && tutorialgcs) // don't allow control changes if temp control override is active
		return;

	controltochange = currentMenu->menuitems[choice].alphaKey;
	sprintf(tmp, M_GetText("Hit the new key for\n%s\nESC for Cancel"),
		currentMenu->menuitems[choice].text);
	strlcpy(controltochangetext, currentMenu->menuitems[choice].text, 33);

	M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
}

static void M_Setup1PPlaystyleMenu(INT32 choice)
{
	(void)choice;

	playstyle_activeplayer = 0;
	OP_PlaystyleDef.prevMenu = &OP_P1ControlsDef;
	M_SetupNextMenu(&OP_PlaystyleDef);
}

static void M_Setup2PPlaystyleMenu(INT32 choice)
{
	(void)choice;

	playstyle_activeplayer = 1;
	OP_PlaystyleDef.prevMenu = &OP_P2ControlsDef;
	M_SetupNextMenu(&OP_PlaystyleDef);
}

static void M_DrawPlaystyleMenu(void)
{
	size_t i;

	for (i = 0; i < 4; i++)
	{
		if (i != 3)
			V_DrawCenteredString((i+1)*BASEVIDWIDTH/4, 20, (i == playstyle_currentchoice) ? V_YELLOWMAP : 0, PlaystyleNames[i]);

		if (i == playstyle_currentchoice)
		{
			V_DrawScaledPatch((i+1)*BASEVIDWIDTH/4 - 8, 10, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
			V_DrawString(30, 50, V_ALLOWLOWERCASE, PlaystyleDesc[i]);
		}
	}
}

static void M_HandlePlaystyleMenu(INT32 choice)
{
	switch (choice)
	{
	case KEY_ESCAPE:
	case KEY_BACKSPACE:
		M_SetupNextMenu(currentMenu->prevMenu);
		break;

	case KEY_ENTER:
		S_StartSound(NULL, sfx_menu1);
		CV_SetValue((playstyle_activeplayer ? &cv_directionchar[1] : &cv_directionchar[0]), playstyle_currentchoice ? 1 : 0);
		CV_SetValue((playstyle_activeplayer ? &cv_useranalog[1] : &cv_useranalog[0]), playstyle_currentchoice/2);

		if (playstyle_activeplayer)
			CV_UpdateCam2Dist();
		else
			CV_UpdateCamDist();

		M_SetupNextMenu(currentMenu->prevMenu);
		break;

	case KEY_LEFTARROW:
		S_StartSound(NULL, sfx_menu1);
		playstyle_currentchoice = (playstyle_currentchoice+2)%3;
		break;

	case KEY_RIGHTARROW:
		S_StartSound(NULL, sfx_menu1);
		playstyle_currentchoice = (playstyle_currentchoice+1)%3;
		break;
	}
}

static void M_DrawCameraOptionsMenu(void)
{
	M_DrawGenericScrollMenu();

	if (gamestate == GS_LEVEL && (paused || P_AutoPause()))
	{
		if (currentMenu == &OP_Camera2OptionsDef && splitscreen && camera2.chase)
			P_MoveChaseCamera(&players[secondarydisplayplayer], &camera2, false);
		if (currentMenu == &OP_CameraOptionsDef && camera.chase)
			P_MoveChaseCamera(&players[displayplayer], &camera, false);
	}
}

// ===============
// VIDEO MODE MENU
// ===============

//added : 30-01-98:
#define MAXCOLUMNMODES   12     //max modes displayed in one column
#define MAXMODEDESCS     (MAXCOLUMNMODES*3)

static modedesc_t modedescs[MAXMODEDESCS];

static void M_VideoModeMenu(INT32 choice)
{
	INT32 i, j, vdup, nummodes, width, height;
	const char *desc;

	(void)choice;

	memset(modedescs, 0, sizeof(modedescs));

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	VID_PrepareModeList(); // FIXME: hack
#endif
	vidm_nummodes = 0;
	vidm_selected = 0;
	nummodes = VID_NumModes();

#ifdef _WINDOWS
	// clean that later: skip windowed mode 0, video modes menu only shows FULL SCREEN modes
	if (nummodes <= NUMSPECIALMODES)
		i = 0; // unless we have nothing
	else
		i = NUMSPECIALMODES;
#else
	// DOS does not skip mode 0, because mode 0 is ALWAYS present
	i = 0;
#endif
	for (; i < nummodes && vidm_nummodes < MAXMODEDESCS; i++)
	{
		desc = VID_GetModeName(i);
		if (desc)
		{
			vdup = 0;

			// when a resolution exists both under VGA and VESA, keep the
			// VESA mode, which is always a higher modenum
			for (j = 0; j < vidm_nummodes; j++)
			{
				if (!strcmp(modedescs[j].desc, desc))
				{
					// mode(0): 320x200 is always standard VGA, not vesa
					if (modedescs[j].modenum)
					{
						modedescs[j].modenum = i;
						vdup = 1;

						if (i == vid.modenum)
							vidm_selected = j;
					}
					else
						vdup = 1;

					break;
				}
			}

			if (!vdup)
			{
				modedescs[vidm_nummodes].modenum = i;
				modedescs[vidm_nummodes].desc = desc;

				if (i == vid.modenum)
					vidm_selected = vidm_nummodes;

				// Pull out the width and height
				sscanf(desc, "%u%*c%u", &width, &height);

				// Show multiples of 320x200 as green.
				if (SCR_IsAspectCorrect(width, height))
					modedescs[vidm_nummodes].goodratio = 1;

				vidm_nummodes++;
			}
		}
	}

	vidm_column_size = (vidm_nummodes+2) / 3;

	M_SetupNextMenu(&OP_VideoModeDef);
}

static void M_DrawMainVideoMenu(void)
{
	M_DrawGenericScrollMenu();
	if (itemOn < 8) // where it starts to go offscreen; change this number if you change the layout of the video menu
	{
		INT32 y = currentMenu->y+currentMenu->menuitems[1].alphaKey*2;
		if (itemOn == 7)
			y -= 10;
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, y,
		(SCR_IsAspectCorrect(vid.width, vid.height) ? V_GREENMAP : V_YELLOWMAP),
			va("%dx%d", vid.width, vid.height));
	}
}

// Draw the video modes list, a-la-Quake
static void M_DrawVideoMode(void)
{
	INT32 i, j, row, col;

	// draw title
	M_DrawMenuTitle();

	V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y,
		V_YELLOWMAP, "Choose mode, reselect to change default");

	row = 41;
	col = OP_VideoModeDef.y + 14;
	for (i = 0; i < vidm_nummodes; i++)
	{
		if (i == vidm_selected)
			V_DrawString(row, col, V_YELLOWMAP, modedescs[i].desc);
		// Show multiples of 320x200 as green.
		else
			V_DrawString(row, col, (modedescs[i].goodratio) ? V_GREENMAP : 0, modedescs[i].desc);

		col += 8;
		if ((i % vidm_column_size) == (vidm_column_size-1))
		{
			row += 7*13;
			col = OP_VideoModeDef.y + 14;
		}
	}

	if (vidm_testingmode > 0)
	{
		INT32 testtime = (vidm_testingmode/TICRATE) + 1;

		M_CentreText(OP_VideoModeDef.y + 116,
			va("Previewing mode %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		M_CentreText(OP_VideoModeDef.y + 138,
			"Press ENTER again to keep this mode");
		M_CentreText(OP_VideoModeDef.y + 150,
			va("Wait %d second%s", testtime, (testtime > 1) ? "s" : ""));
		M_CentreText(OP_VideoModeDef.y + 158,
			"or press ESC to return");

	}
	else
	{
		M_CentreText(OP_VideoModeDef.y + 116,
			va("Current mode is %c%dx%d",
				(SCR_IsAspectCorrect(vid.width, vid.height)) ? 0x83 : 0x80,
				vid.width, vid.height));
		M_CentreText(OP_VideoModeDef.y + 124,
			va("Default mode is %c%dx%d",
				(SCR_IsAspectCorrect(cv_scr_width.value, cv_scr_height.value)) ? 0x83 : 0x80,
				cv_scr_width.value, cv_scr_height.value));

		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 138,
			V_GREENMAP, "Green modes are recommended.");
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 146,
			V_YELLOWMAP, "Other modes may have visual errors.");
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 158,
			V_YELLOWMAP, "Larger modes may have performance issues.");
	}

	// Draw the cursor for the VidMode menu
	i = 41 - 10 + ((vidm_selected / vidm_column_size)*7*13);
	j = OP_VideoModeDef.y + 14 + ((vidm_selected % vidm_column_size)*8);

	V_DrawScaledPatch(i - 8, j, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}

// Just M_DrawGenericScrollMenu but showing a backing behind the headers.
static void M_DrawColorMenu(void)
{
	INT32 x, y, i, max, tempcentery, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	V_DrawFill(19       , y-4, 47, 1,  35);
	V_DrawFill(19+(  47), y-4, 47, 1,  73);
	V_DrawFill(19+(2*47), y-4, 47, 1, 112);
	V_DrawFill(19+(3*47), y-4, 47, 1, 255);
	V_DrawFill(19+(4*47), y-4, 47, 1, 152);
	V_DrawFill(19+(5*47), y-4, 46, 1, 181);

	V_DrawFill(300, y-4, 1, 1, 26);
	V_DrawFill( 19, y-3, 282, 1, 26);

	if ((currentMenu->menuitems[itemOn].alphaKey*2 - currentMenu->menuitems[0].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[0].alphaKey*2;
	else if ((currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 - currentMenu->menuitems[itemOn].alphaKey*2) <= scrollareaheight)
		tempcentery = currentMenu->y - currentMenu->menuitems[currentMenu->numitems-1].alphaKey*2 + 2*scrollareaheight;
	else
		tempcentery = currentMenu->y - currentMenu->menuitems[itemOn].alphaKey*2 + scrollareaheight;

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (currentMenu->menuitems[i].status != IT_DISABLED && currentMenu->menuitems[i].alphaKey*2 + tempcentery >= currentMenu->y)
			break;
	}

	for (max = currentMenu->numitems; max > 0; max--)
	{
		if (currentMenu->menuitems[max].status != IT_DISABLED && currentMenu->menuitems[max-1].alphaKey*2 + tempcentery <= (currentMenu->y + 2*scrollareaheight))
			break;
	}

	if (i)
		V_DrawString(currentMenu->x - 20, currentMenu->y - (skullAnimCounter/5), V_YELLOWMAP, "\x1A"); // up arrow
	if (max != currentMenu->numitems)
		V_DrawString(currentMenu->x - 20, currentMenu->y + 2*scrollareaheight + (skullAnimCounter/5), V_YELLOWMAP, "\x1B"); // down arrow

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (; i < max; i++)
	{
		y = currentMenu->menuitems[i].alphaKey*2 + tempcentery;
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
			case IT_DYBIGSPACE:
			case IT_BIGSLIDER:
			case IT_STRING2:
			case IT_DYLITLSPACE:
			case IT_GRAYPATCH:
			case IT_TRANSTEXT2:
				// unsupported
				break;
			case IT_NOTHING:
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (i != itemOn && (currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv, (i == itemOn));
							case IT_CV_NOPRINT: // color use this
							case IT_CV_INVISSLIDER: // monitor toggles use this
								break;
							case IT_CV_STRING:
								if (y + 12 > (currentMenu->y + 2*scrollareaheight))
									break;
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string, 0), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								V_DrawRightAlignedString(BASEVIDWIDTH - x, y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
								if (i == itemOn)
								{
									V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(cv->string, 0) - (skullAnimCounter/5), y,
											'\x1C' | V_YELLOWMAP, false);
									V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
											'\x1D' | V_YELLOWMAP, false);
								}
								break;
						}
						break;
					}
					break;
			case IT_TRANSTEXT:
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
				break;
			case IT_QUESTIONMARKS:
				V_DrawString(x, y, V_TRANSLUCENT|V_OLDSPACING, M_CreateSecretMenuOption(currentMenu->menuitems[i].text));
				break;
			case IT_HEADERTEXT:
				//V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
				V_DrawFill(19, y, 281, 9, currentMenu->menuitems[i+1].alphaKey);
				V_DrawFill(300, y, 1, 9, 26);
				M_DrawLevelPlatterHeader(y - (lsheadingheight - 12), currentMenu->menuitems[i].text, false, false);
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_PATCH));
}

// special menuitem key handler for video mode list
static void M_HandleVideoMode(INT32 ch)
{
	if (vidm_testingmode > 0) switch (ch)
	{
		// change back to the previous mode quickly
		case KEY_ESCAPE:
			setmodeneeded = vidm_previousmode + 1;
			vidm_testingmode = 0;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			vidm_testingmode = 0; // stop testing
	}

	else switch (ch)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			if (++vidm_selected >= vidm_nummodes)
				vidm_selected = 0;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			if (--vidm_selected < 0)
				vidm_selected = vidm_nummodes - 1;
			break;

		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_selected -= vidm_column_size;
			if (vidm_selected < 0)
				vidm_selected = (vidm_column_size*3) + vidm_selected;
			if (vidm_selected >= vidm_nummodes)
				vidm_selected = vidm_nummodes - 1;
			break;

		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_selected += vidm_column_size;
			if (vidm_selected >= (vidm_column_size*3))
				vidm_selected %= vidm_column_size;
			if (vidm_selected >= vidm_nummodes)
				vidm_selected = vidm_nummodes - 1;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			if (vid.modenum == modedescs[vidm_selected].modenum)
				SCR_SetDefaultMode();
			else
			{
				vidm_testingmode = 15*TICRATE;
				vidm_previousmode = vid.modenum;
				if (!setmodeneeded) // in case the previous setmode was not finished
					setmodeneeded = modedescs[vidm_selected].modenum + 1;
			}
			break;

		case KEY_ESCAPE: // this one same as M_Responder
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu);
			else
				M_ClearMenus(true);
			break;

		default:
			break;
	}
}

static void M_DrawScreenshotMenu(void)
{
	M_DrawGenericScrollMenu();
#ifdef HWRENDER
	if ((rendermode == render_opengl) && (itemOn < 7)) // where it starts to go offscreen; change this number if you change the layout of the screenshot menu
	{
		INT32 y = currentMenu->y+currentMenu->menuitems[op_screenshot_colorprofile].alphaKey*2;
		if (itemOn == 6)
			y -= 10;
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, y, V_REDMAP, "Yes");
	}
#endif
}

// ===============
// Monitor Toggles
// ===============
static void M_DrawMonitorToggles(void)
{
	INT32 i, y;
	INT32 sum = 0;
	consvar_t *cv;
	boolean cheating = false;

	M_DrawGenericMenu();

	// Assumes all are cvar type.
	for (i = 0; i < currentMenu->numitems; ++i)
	{
		if (!(currentMenu->menuitems[i].status & IT_CVAR) || !(cv = (consvar_t *)currentMenu->menuitems[i].itemaction))
			continue;
		sum += cv->value;

		if (!CV_IsSetToDefault(cv))
			cheating = true;
	}

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		if (!(currentMenu->menuitems[i].status & IT_CVAR) || !(cv = (consvar_t *)currentMenu->menuitems[i].itemaction))
			continue;
		y = currentMenu->y + currentMenu->menuitems[i].alphaKey;

		M_DrawSlider(currentMenu->x + 20, y, cv, (i == itemOn));

		if (!cv->value)
			V_DrawRightAlignedString(312, y, V_OLDSPACING|((i == itemOn) ? V_YELLOWMAP : 0), "None");
		else
			V_DrawRightAlignedString(312, y, V_OLDSPACING|((i == itemOn) ? V_YELLOWMAP : 0), va("%3d%%", (cv->value*100)/sum));
	}

	if (cheating)
		V_DrawCenteredString(BASEVIDWIDTH/2, currentMenu->y, V_REDMAP, "* MODIFIED, CHEATS ENABLED *");
}

// =========
// Quit Game
// =========
static INT32 quitsounds[] =
{
	// holy shit we're changing things up!
	sfx_itemup, // Tails 11-09-99
	sfx_jump, // Tails 11-09-99
	sfx_skid, // Inu 04-03-13
	sfx_spring, // Tails 11-09-99
	sfx_pop,
	sfx_spdpad, // Inu 04-03-13
	sfx_wdjump, // Inu 04-03-13
	sfx_mswarp, // Inu 04-03-13
	sfx_splash, // Tails 11-09-99
	sfx_floush, // Tails 11-09-99
	sfx_gloop, // Tails 11-09-99
	sfx_s3k66, // Inu 04-03-13
	sfx_s3k6a, // Inu 04-03-13
	sfx_s3k73, // Inu 04-03-13
	sfx_chchng // Tails 11-09-99
};

void M_QuitResponse(INT32 ch)
{
	tic_t ptime;
	INT32 mrand;

	if (ch != 'y' && ch != KEY_ENTER)
		return;
	LUAh_GameQuit(true);
	if (!(netgame || cv_debug))
	{
		S_ResetCaptions();
		marathonmode = 0;

		mrand = M_RandomKey(sizeof(quitsounds)/sizeof(INT32));
		if (quitsounds[mrand]) S_StartSound(NULL, quitsounds[mrand]);

		//added : 12-02-98: do that instead of I_WaitVbl which does not work
		ptime = I_GetTime() + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
		while (ptime > I_GetTime())
		{
			V_DrawScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_PATCH)); // Demo 3 Quit Screen Tails 06-16-2001
			I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
			I_Sleep();
		}
	}
	I_Quit();
}

static void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	(void)choice;
	M_StartMessage(quitmsg[M_RandomKey(NUM_QUITMESSAGES)], M_QuitResponse, MM_YESNO);
}
