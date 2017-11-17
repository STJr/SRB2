// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2011-2016 by Matthew "Inuyasha" Walsh.
// Copyright (C) 1999-2016 by Sonic Team Junior.
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

// Condition Sets
#include "m_cond.h"

// And just some randomness for the exits.
#include "m_random.h"

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
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

const char *quitmsg[NUM_QUITMESSAGES];

// Stuff for customizing the player select screen Tails 09-22-2003
// A rare case.
// External files modify this menu, so we can't call it static.
// And I'm too lazy to go through and rename it everywhere. ARRGH!
description_t description[32] =
{
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0},
	{false, "???", "", "", 0, 0}
};
INT16 char_on = -1, startchar = 1;
static char *char_notes = NULL;
static fixed_t char_scroll = 0;

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

static char joystickInfo[8][25];
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

//
// PROTOTYPES
//

static void M_GoBack(INT32 choice);
static void M_StopMessage(INT32 choice);

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
static void M_PandorasBox(INT32 choice);
static void M_EmblemHints(INT32 choice);
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
static void M_QuitSRB2(INT32 choice);
menu_t SP_MainDef, OP_MainDef;
menu_t MISC_ScrambleTeamDef, MISC_ChangeTeamDef;

// Single Player
static void M_LoadGame(INT32 choice);
static void M_TimeAttackLevelSelect(INT32 choice);
static void M_TimeAttack(INT32 choice);
static void M_NightsAttackLevelSelect(INT32 choice);
static void M_NightsAttack(INT32 choice);
static void M_Statistics(INT32 choice);
static void M_ReplayTimeAttack(INT32 choice);
static void M_ChooseTimeAttack(INT32 choice);
static void M_ChooseNightsAttack(INT32 choice);
static void M_ModeAttackRetry(INT32 choice);
static void M_ModeAttackEndGame(INT32 choice);
static void M_SetGuestReplay(INT32 choice);
static void M_HandleChoosePlayerMenu(INT32 choice);
static void M_ChoosePlayer(INT32 choice);
menu_t SP_LevelStatsDef;
static menu_t SP_TimeAttackDef, SP_ReplayDef, SP_GuestReplayDef, SP_GhostDef;
static menu_t SP_NightsAttackDef, SP_NightsReplayDef, SP_NightsGuestReplayDef, SP_NightsGhostDef;

// Multiplayer
static void M_SetupMultiPlayer(INT32 choice);
static void M_SetupMultiPlayer2(INT32 choice);
static void M_StartSplitServerMenu(INT32 choice);
static void M_StartServer(INT32 choice);
static void M_ServerOptions(INT32 choice);
#ifndef NONET
static void M_StartServerMenu(INT32 choice);
static void M_ConnectMenu(INT32 choice);
static void M_Refresh(INT32 choice);
static void M_Connect(INT32 choice);
static void M_ChooseRoom(INT32 choice);
menu_t MP_MainDef;
#endif

// Options
// Split into multiple parts due to size
// Controls
menu_t OP_ChangeControlsDef;
menu_t OP_MPControlsDef, OP_CameraControlsDef, OP_MiscControlsDef;
menu_t OP_P1ControlsDef, OP_P2ControlsDef, OP_MouseOptionsDef;
menu_t OP_Mouse2OptionsDef, OP_Joystick1Def, OP_Joystick2Def;
static void M_VideoModeMenu(INT32 choice);
static void M_SoundMenu(INT32 choice);
static void M_Setup1PControlsMenu(INT32 choice);
static void M_Setup2PControlsMenu(INT32 choice);
static void M_Setup1PJoystickMenu(INT32 choice);
static void M_Setup2PJoystickMenu(INT32 choice);
static void M_AssignJoystick(INT32 choice);
static void M_ChangeControl(INT32 choice);

// Video & Sound
menu_t OP_VideoOptionsDef, OP_VideoModeDef, OP_ColorOptionsDef;
#ifdef HWRENDER
menu_t OP_OpenGLOptionsDef, OP_OpenGLFogDef, OP_OpenGLColorDef;
#endif
menu_t OP_SoundOptionsDef;
static void M_ToggleSFX(INT32 choice);
static void M_ToggleDigital(INT32 choice);
static void M_ToggleMIDI(INT32 choice);

//Misc
menu_t OP_DataOptionsDef, OP_ScreenshotOptionsDef, OP_EraseDataDef;
menu_t OP_ServerOptionsDef;
menu_t OP_MonitorToggleDef;
static void M_ScreenshotOptions(INT32 choice);
static void M_EraseData(INT32 choice);

static void M_Addons(INT32 choice);
static void M_AddonsOptions(INT32 choice);
static patch_t *addonsp[NUM_EXT+6];
static UINT8 addonsresponselimit = 0;

#define numaddonsshown 4

static void M_DrawLevelPlatterHeader(INT32 y, const char *header, boolean headerhighlight, boolean allowlowercase);

// Drawing functions
static void M_DrawGenericMenu(void);
static void M_DrawGenericScrollMenu(void);
static void M_DrawCenteredMenu(void);
static void M_DrawAddons(void);
static void M_DrawSkyRoom(void);
static void M_DrawChecklist(void);
static void M_DrawEmblemHints(void);
static void M_DrawPauseMenu(void);
static void M_DrawServerMenu(void);
static void M_DrawLevelPlatterMenu(void);
static void M_DrawImageDef(void);
static void M_DrawLoad(void);
static void M_DrawLevelStats(void);
static void M_DrawTimeAttackMenu(void);
static void M_DrawNightsAttackMenu(void);
static void M_DrawSetupChoosePlayerMenu(void);
static void M_DrawControl(void);
static void M_DrawMainVideoMenu(void);
static void M_DrawVideoMode(void);
static void M_DrawColorMenu(void);
static void M_DrawSoundMenu(void);
static void M_DrawScreenshotMenu(void);
static void M_DrawMonitorToggles(void);
#ifdef HWRENDER
static void M_OGL_DrawFogMenu(void);
static void M_OGL_DrawColorMenu(void);
#endif
#ifndef NONET
static void M_DrawScreenshotMenu(void);
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
#ifndef NONET
static boolean M_CancelConnect(void);
static void M_HandleConnectIP(INT32 choice);
#endif
static void M_HandleSetupMultiPlayer(INT32 choice);
#ifdef HWRENDER
static void M_HandleFogColor(INT32 choice);
#endif
static void M_HandleVideoMode(INT32 choice);

static void M_ResetCvars(void);

// Consvar onchange functions
static void Newgametype_OnChange(void);
static void Dummymares_OnChange(void);

// ==========================================================================
// CONSOLE VARIABLES AND THEIR POSSIBLE VALUES GO HERE.
// ==========================================================================

static CV_PossibleValue_t map_cons_t[] = {
	{1,"MIN"},
	{NUMMAPS, "MAX"}
};
consvar_t cv_nextmap = {"nextmap", "1", CV_HIDEN|CV_CALL, map_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = {"chooseskin", DEFAULTSKIN, CV_HIDEN|CV_CALL, skins_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

// This gametype list is integral for many different reasons.
// When you add gametypes here, don't forget to update them in dehacked.c and doomstat.h!
CV_PossibleValue_t gametype_cons_t[NUMGAMETYPES+1];

consvar_t cv_newgametype = {"newgametype", "Co-op", CV_HIDEN|CV_CALL, gametype_cons_t, Newgametype_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t serversort_cons_t[] = {
	{0,"Ping"},
	{1,"Modified State"},
	{2,"Most Players"},
	{3,"Least Players"},
	{4,"Max Players"},
	{5,"Gametype"},
	{0,NULL}
};
consvar_t cv_serversort = {"serversort", "Ping", CV_HIDEN | CV_CALL, serversort_cons_t, M_SortServerList, 0, NULL, NULL, 0, 0, NULL};

// autorecord demos for time attack
static consvar_t cv_autorecord = {"autorecord", "Yes", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

CV_PossibleValue_t ghost_cons_t[] = {{0, "Hide"}, {1, "Show"}, {2, "Show All"}, {0, NULL}};
CV_PossibleValue_t ghost2_cons_t[] = {{0, "Hide"}, {1, "Show"}, {0, NULL}};

consvar_t cv_ghost_bestscore = {"ghost_bestscore", "Show", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_besttime  = {"ghost_besttime",  "Show", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_bestrings = {"ghost_bestrings", "Show", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_last      = {"ghost_last",      "Show", CV_SAVE, ghost_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ghost_guest     = {"ghost_guest",     "Show", CV_SAVE, ghost2_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

//Console variables used solely in the menu system.
//todo: add a way to use non-console variables in the menu
//      or make these consvars legitimate like color or skin.
static CV_PossibleValue_t dummyteam_cons_t[] = {{0, "Spectator"}, {1, "Red"}, {2, "Blue"}, {0, NULL}};
static CV_PossibleValue_t dummyscramble_cons_t[] = {{0, "Random"}, {1, "Points"}, {0, NULL}};
static CV_PossibleValue_t ringlimit_cons_t[] = {{0, "MIN"}, {9999, "MAX"}, {0, NULL}};
static CV_PossibleValue_t liveslimit_cons_t[] = {{0, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t dummymares_cons_t[] = {
	{-1, "END"}, {0,"Overall"}, {1,"Mare 1"}, {2,"Mare 2"}, {3,"Mare 3"}, {4,"Mare 4"}, {5,"Mare 5"}, {6,"Mare 6"}, {7,"Mare 7"}, {8,"Mare 8"}, {0,NULL}
};

static consvar_t cv_dummyteam = {"dummyteam", "Spectator", CV_HIDEN, dummyteam_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_dummyscramble = {"dummyscramble", "Random", CV_HIDEN, dummyscramble_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_dummyrings = {"dummyrings", "0", CV_HIDEN, ringlimit_cons_t,	NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_dummylives = {"dummylives", "0", CV_HIDEN, liveslimit_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_dummycontinues = {"dummycontinues", "0", CV_HIDEN, liveslimit_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_dummymares = {"dummymares", "Overall", CV_HIDEN|CV_CALL, dummymares_cons_t, Dummymares_OnChange, 0, NULL, NULL, 0, 0, NULL};

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
	{IT_STRING|IT_CALL,    NULL, "Secrets",     M_SecretsMenu,           76},
	{IT_STRING|IT_CALL,    NULL, "1  player",   M_SinglePlayerMenu,      84},
#ifndef NONET
	{IT_STRING|IT_SUBMENU, NULL, "multiplayer", &MP_MainDef,             92},
#else
	{IT_STRING|IT_CALL,    NULL, "multiplayer", M_StartSplitServerMenu,  92},
#endif
	{IT_STRING|IT_CALL,    NULL, "options",     M_Options,              100},
	{IT_CALL   |IT_STRING, NULL, "addons",      M_Addons,               108},
	{IT_STRING|IT_CALL,    NULL, "quit  game",  M_QuitSRB2,             116},
};

typedef enum
{
	secrets = 0,
	singleplr,
	multiplr,
	options,
	addons,
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
	{IT_CALL | IT_STRING,    NULL, "Continue",             M_SelectableClearMenus,48},
	{IT_CALL | IT_STRING,    NULL, "Retry",                M_ModeAttackRetry,     56},
	{IT_CALL | IT_STRING,    NULL, "Abort",                M_ModeAttackEndGame,   64},
};

typedef enum
{
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

static const gtdesc_t gametypedesc[] =
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
	{IT_STRING|IT_SUBMENU,NULL, "Secrets Checklist", &SR_UnlockChecklistDef, 0},
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

static menuitem_t SR_EmblemHintMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Emblem Radar", &cv_itemfinder, 10},
	{IT_WHITESTRING|IT_SUBMENU, NULL, "Back",         &SPauseDef,     20}
};

// --------------------------------
// 1 Player and all of its submenus
// --------------------------------
// Prefix: SP_

// Single Player Main
static menuitem_t SP_MainMenu[] =
{
	{IT_CALL | IT_STRING,                       NULL, "Start Game",    M_LoadGame,                 92},
	{IT_SECRET,                                 NULL, "Record Attack", M_TimeAttack,              100},
	{IT_SECRET,                                 NULL, "NiGHTS Mode",   M_NightsAttack,            108},
	{IT_CALL | IT_STRING | IT_CALL_NOTMODIFIED, NULL, "Statistics",    M_Statistics,              116},
};

enum
{
	sploadgame,
	sprecordattack,
	spnightsmode,
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
	{IT_STRING|IT_CALL,        NULL, "Level Select...", &M_TimeAttackLevelSelect,   52},
	{IT_STRING|IT_CVAR,        NULL, "Character",       &cv_chooseskin,             62},

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
	{IT_STRING|IT_CALL,        NULL, "Level Select...",  &M_NightsAttackLevelSelect,  52},
	{IT_STRING|IT_CVAR,        NULL, "Show Records For", &cv_dummymares,              62},

	{IT_DISABLED,              NULL, "Guest Option...",  &SP_NightsGuestReplayDef,    100},
	{IT_DISABLED,              NULL, "Replay...",        &SP_NightsReplayDef,         110},
	{IT_DISABLED,              NULL, "Ghosts...",        &SP_NightsGhostDef,          120},
	{IT_WHITESTRING|IT_CALL|IT_CALL_NOTMODIFIED, NULL, "Start", M_ChooseNightsAttack, 130},
};

enum
{
	nalevel,
	narecords,

	naguest,
	nareplay,
	naghost,
	nastart
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
	{IT_HEADER, NULL, "Host a game", NULL, 0},
	{IT_STRING|IT_CALL,       NULL, "Internet/LAN...",       M_StartServerMenu,      12},
	{IT_STRING|IT_CALL,       NULL, "Splitscreen...",        M_StartSplitServerMenu, 22},
	{IT_HEADER, NULL, "Join a game", NULL, 40},
	{IT_STRING|IT_CALL,       NULL, "Server browser...",     M_ConnectMenu,          52},
	{IT_STRING|IT_KEYHANDLER, NULL, "Specify IPv4 address:", M_HandleConnectIP,      62},
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

static menuitem_t MP_RoomMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "<Offline Mode>", M_ChooseRoom,   9},
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

	{IT_SUBMENU | IT_STRING, NULL, "Video Options...",     &OP_VideoOptionsDef, 50},
	{IT_CALL    | IT_STRING, NULL, "Sound Options...",     M_SoundMenu,         60},

	{IT_CALL    | IT_STRING, NULL, "Server Options...",    M_ServerOptions,     80},

	{IT_SUBMENU | IT_STRING, NULL, "Data Options...",      &OP_DataOptionsDef, 100},
};

static menuitem_t OP_P1ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup1PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Mouse Options...", &OP_MouseOptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Gamepad Options...", &OP_Joystick1Def  ,  30},

	{IT_STRING  | IT_CVAR, NULL, "Third-person Camera"  , &cv_chasecam , 50},
	{IT_STRING  | IT_CVAR, NULL, "Flip Camera with Gravity"  , &cv_flipcam , 60},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair, 70},

	//{IT_STRING  | IT_CVAR, NULL, "Analog Control", &cv_useranalog,  90},
	{IT_STRING  | IT_CVAR, NULL, "Character angle", &cv_directionchar,  90},
	{IT_STRING  | IT_CVAR, NULL, "Automatic braking", &cv_autobrake,  100},
};

static menuitem_t OP_P2ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup2PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Second Mouse Options...", &OP_Mouse2OptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Second Gamepad Options...", &OP_Joystick2Def  ,  30},

	{IT_STRING  | IT_CVAR, NULL, "Third-person Camera"  , &cv_chasecam2 , 50},
	{IT_STRING  | IT_CVAR, NULL, "Flip Camera with Gravity"  , &cv_flipcam2 , 60},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair2, 70},

	//{IT_STRING  | IT_CVAR, NULL, "Analog Control", &cv_useranalog2,  90},
	{IT_STRING  | IT_CVAR, NULL, "Character angle", &cv_directionchar2,  90},
	{IT_STRING  | IT_CVAR, NULL, "Automatic braking", &cv_autobrake2,  100},
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
	{IT_CALL | IT_STRING2, NULL, "Spin",             M_ChangeControl, gc_use     },
	{IT_HEADER, NULL, "Camera", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Camera Up",        M_ChangeControl, gc_lookup      },
	{IT_CALL | IT_STRING2, NULL, "Camera Down",      M_ChangeControl, gc_lookdown    },
	{IT_CALL | IT_STRING2, NULL, "Camera Left",      M_ChangeControl, gc_turnleft    },
	{IT_CALL | IT_STRING2, NULL, "Camera Right",     M_ChangeControl, gc_turnright   },
	{IT_CALL | IT_STRING2, NULL, "Center View",      M_ChangeControl, gc_centerview  },
	{IT_CALL | IT_STRING2, NULL, "Toggle Mouselook", M_ChangeControl, gc_mouseaiming },
	{IT_CALL | IT_STRING2, NULL, "Toggle Third-Person", M_ChangeControl, gc_camtoggle},
	{IT_CALL | IT_STRING2, NULL, "Reset Camera",     M_ChangeControl, gc_camreset    },
	{IT_HEADER, NULL, "Meta", NULL, 0},
	{IT_SPACE, NULL, NULL, NULL, 0}, // padding
	{IT_CALL | IT_STRING2, NULL, "Game Status",
    M_ChangeControl, gc_scores      },
	{IT_CALL | IT_STRING2, NULL, "Pause",            M_ChangeControl, gc_pause       },
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
};

static menuitem_t OP_JoystickSetMenu[] =
{
	{IT_CALL | IT_NOTHING, "None", NULL, M_AssignJoystick, '0'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '1'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '2'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '3'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '4'},
};

static menuitem_t OP_MouseOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse",        &cv_usemouse,         10},


	{IT_STRING | IT_CVAR, NULL, "Always Mouselook", &cv_alwaysfreelook,   30},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove,        40},
	{IT_STRING | IT_CVAR, NULL, "Invert Y Axis",     &cv_invertmouse,      50},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Sensitivity",    &cv_mousesens,        60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Sensitivity",    &cv_mouseysens,        70},
};

static menuitem_t OP_Mouse2OptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse 2",      &cv_usemouse2,        10},
	{IT_STRING | IT_CVAR, NULL, "Second Mouse Serial Port",
	                                                &cv_mouse2port,       20},
	{IT_STRING | IT_CVAR, NULL, "Always Mouselook", &cv_alwaysfreelook2,  30},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove2,       40},
	{IT_STRING | IT_CVAR, NULL, "Invert Y Axis",     &cv_invertmouse2,     50},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Sensitivity",    &cv_mousesens2,       60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Sensitivity",    &cv_mouseysens2,      70},
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
	{IT_SUBMENU|IT_STRING, NULL, "OpenGL Options...", &OP_OpenGLOptionsDef,          21},
#endif

	{IT_HEADER, NULL, "Color Profile", NULL, 30},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Brightness (F11)", &cv_globalgamma,      36},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Saturation", &cv_globalsaturation, 41},
	{IT_SUBMENU|IT_STRING, NULL, "Advanced Settings...",     &OP_ColorOptionsDef,  46},

	{IT_HEADER, NULL, "Heads-Up Display", NULL, 55},
	{IT_STRING | IT_CVAR, NULL, "Show HUD",                  &cv_showhud,          61},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "HUD Transparency",          &cv_translucenthud,   66},
	{IT_STRING | IT_CVAR, NULL, "Time Display",              &cv_timetic,          71},
#ifdef SEENAMES
	{IT_STRING | IT_CVAR, NULL, "Show player names",         &cv_seenames,         76},
#endif

	{IT_HEADER, NULL, "Console", NULL, 85},
	{IT_STRING | IT_CVAR, NULL, "Background color",          &cons_backcolor,      91},
	{IT_STRING | IT_CVAR, NULL, "Text Size",                 &cv_constextsize,     96},

	{IT_HEADER, NULL, "Level", NULL, 105},
	{IT_STRING | IT_CVAR, NULL, "Draw Distance",             &cv_drawdist,        111},
	{IT_STRING | IT_CVAR, NULL, "NiGHTS Draw Dist.",         &cv_drawdist_nights, 116},
	{IT_STRING | IT_CVAR, NULL, "Weather Draw Dist.",        &cv_drawdist_precip, 121},
	{IT_STRING | IT_CVAR, NULL, "Weather Density",           &cv_precipdensity,   126},

	{IT_HEADER, NULL, "Diagnostic", NULL, 135},
	{IT_STRING | IT_CVAR, NULL, "Show FPS",                  &cv_ticrate,         141},
	{IT_STRING | IT_CVAR, NULL, "Clear Before Redraw",       &cv_homremoval,      146},
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
	{IT_STRING|IT_CVAR,         NULL, "Field of view",   &cv_grfov,            10},
	{IT_STRING|IT_CVAR,         NULL, "Quality",         &cv_scr_depth,        20},
	{IT_STRING|IT_CVAR,         NULL, "Texture Filter",  &cv_grfiltermode,     30},
	{IT_STRING|IT_CVAR,         NULL, "Anisotropic",     &cv_granisotropicmode,40},
#if defined (_WINDOWS) && (!((defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)))
	{IT_STRING|IT_CVAR,         NULL, "Fullscreen",      &cv_fullscreen,       50},
#endif
#ifdef ALAM_LIGHTING
	{IT_SUBMENU|IT_STRING,      NULL, "Lighting...",     &OP_OpenGLLightingDef,     70},
#endif
	{IT_SUBMENU|IT_STRING,      NULL, "Fog...",          &OP_OpenGLFogDef,          80},
	{IT_SUBMENU|IT_STRING,      NULL, "Gamma...",        &OP_OpenGLColorDef,        90},
};

#ifdef ALAM_LIGHTING
static menuitem_t OP_OpenGLLightingMenu[] =
{
	{IT_STRING|IT_CVAR, NULL, "Coronas",          &cv_grcoronas,          0},
	{IT_STRING|IT_CVAR, NULL, "Coronas size",     &cv_grcoronasize,      10},
	{IT_STRING|IT_CVAR, NULL, "Dynamic lighting", &cv_grdynamiclighting, 20},
	{IT_STRING|IT_CVAR, NULL, "Static lighting",  &cv_grstaticlighting,  30},
};
#endif

static menuitem_t OP_OpenGLFogMenu[] =
{
	{IT_STRING|IT_CVAR,       NULL, "Fog",         &cv_grfog,        10},
	{IT_STRING|IT_KEYHANDLER, NULL, "Fog color",   M_HandleFogColor, 20},
	{IT_STRING|IT_CVAR,       NULL, "Fog density", &cv_grfogdensity, 30},
	{IT_STRING|IT_CVAR,       NULL, "Software Fog",&cv_grsoftwarefog,40},
};

static menuitem_t OP_OpenGLColorMenu[] =
{
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "red",   &cv_grgammared,   10},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "green", &cv_grgammagreen, 20},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "blue",  &cv_grgammablue,  30},
};
#endif

static menuitem_t OP_SoundOptionsMenu[] =
{
	{IT_STRING | IT_KEYHANDLER,  NULL,  "Sound Effects", M_ToggleSFX, 10},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Sound Volume", &cv_soundvolume, 20},

	{IT_STRING | IT_KEYHANDLER,  NULL,  "Digital Music", M_ToggleDigital, 40},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "Digital Music Volume", &cv_digmusicvolume,  50},

	{IT_STRING | IT_KEYHANDLER,  NULL,  "MIDI Music", M_ToggleMIDI, 70},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER, NULL, "MIDI Music Volume", &cv_midimusicvolume, 80},

	{IT_STRING | IT_CVAR, NULL, "Closed Captioning", &cv_closedcaptioning, 100},
};

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
	{IT_STRING|IT_CVAR, NULL, "Storage Location",  &cv_screenshot_option,          11},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder", &cv_screenshot_folder, 16},

	{IT_HEADER, NULL, "Screenshots (F8)", NULL, 30},
	{IT_STRING|IT_CVAR, NULL, "Memory Level",      &cv_zlib_memory,                36},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", &cv_zlib_level,                 41},
	{IT_STRING|IT_CVAR, NULL, "Strategy",          &cv_zlib_strategy,              46},
	{IT_STRING|IT_CVAR, NULL, "Window Size",       &cv_zlib_window_bits,           51},

	{IT_HEADER, NULL, "Movie Mode (F9)", NULL, 60},
	{IT_STRING|IT_CVAR, NULL, "Capture Mode",      &cv_moviemode,                  66},

	{IT_STRING|IT_CVAR, NULL, "Region Optimizing", &cv_gif_optimize,               71},
	{IT_STRING|IT_CVAR, NULL, "Downscaling",       &cv_gif_downscale,              76},

	{IT_STRING|IT_CVAR, NULL, "Memory Level",      &cv_zlib_memorya,               71},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", &cv_zlib_levela,                76},
	{IT_STRING|IT_CVAR, NULL, "Strategy",          &cv_zlib_strategya,             81},
	{IT_STRING|IT_CVAR, NULL, "Window Size",       &cv_zlib_window_bitsa,          86},
};

enum
{
	op_screenshot_colorprofile = 1,
	op_screenshot_folder = 3,
	op_screenshot_capture = 10,
	op_screenshot_gif_start = 11,
	op_screenshot_gif_end = 12,
	op_screenshot_apng_start = 13,
	op_screenshot_apng_end = 16,
};

static menuitem_t OP_EraseDataMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Erase Record Data", M_EraseData, 10},
	{IT_STRING | IT_CALL, NULL, "Erase Secrets Data", M_EraseData, 20},

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
	                         NULL, "Server name",                      &cv_servername,          7},
	{IT_STRING | IT_CVAR,    NULL, "Max Players",                      &cv_maxplayers,          21},
	{IT_STRING | IT_CVAR,    NULL, "Allow Add-on Downloading",         &cv_downloading,         26},
	{IT_STRING | IT_CVAR,    NULL, "Allow players to join",            &cv_allownewplayer,      31},
#endif
	{IT_STRING | IT_CVAR,    NULL, "Map progression",                  &cv_advancemap,          36},
	{IT_STRING | IT_CVAR,    NULL, "Intermission Timer",               &cv_inttime,             41},

	{IT_HEADER, NULL, "Characters", NULL, 50},
	{IT_STRING | IT_CVAR,    NULL, "Force a character",                &cv_forceskin,           56},
	{IT_STRING | IT_CVAR,    NULL, "Restrict character changes",       &cv_restrictskinchange,  61},

	{IT_HEADER, NULL, "Items", NULL, 70},
	{IT_STRING | IT_CVAR,    NULL, "Item respawn delay",               &cv_itemrespawntime,     76},
	{IT_STRING | IT_SUBMENU, NULL, "Mystery Item Monitor Toggles...",  &OP_MonitorToggleDef,    81},

	{IT_HEADER, NULL, "Cooperative", NULL, 90},
	{IT_STRING | IT_CVAR,    NULL, "Players required for exit",        &cv_playersforexit,      96},
	{IT_STRING | IT_CVAR,    NULL, "Starposts",                        &cv_coopstarposts,      101},
	{IT_STRING | IT_CVAR,    NULL, "Life sharing",                     &cv_cooplives,          106},

	{IT_HEADER, NULL, "Race, Competition", NULL, 115},
	{IT_STRING | IT_CVAR,    NULL, "Level completion countdown",       &cv_countdowntime,      121},
	{IT_STRING | IT_CVAR,    NULL, "Item Monitors",                    &cv_competitionboxes,   126},

	{IT_HEADER, NULL, "Ringslinger (Match, CTF, Tag, H&S)", NULL, 135},
	{IT_STRING | IT_CVAR,    NULL, "Time Limit",                       &cv_timelimit,          141},
	{IT_STRING | IT_CVAR,    NULL, "Score Limit",                      &cv_pointlimit,         146},
	{IT_STRING | IT_CVAR,    NULL, "Overtime on Tie",                  &cv_overtime,           151},
	{IT_STRING | IT_CVAR,    NULL, "Player respawn delay",             &cv_respawntime,        156},

	{IT_STRING | IT_CVAR,    NULL, "Item Monitors",                    &cv_matchboxes,         166},
	{IT_STRING | IT_CVAR,    NULL, "Weapon Rings",                     &cv_specialrings,       171},
	{IT_STRING | IT_CVAR,    NULL, "Power Stones",                     &cv_powerstones,        176},

	{IT_STRING | IT_CVAR,    NULL, "Flag respawn delay",               &cv_flagtime,           186},
	{IT_STRING | IT_CVAR,    NULL, "Hiding time",                      &cv_hidetime,           191},

	{IT_HEADER, NULL, "Teams", NULL, 200},
	{IT_STRING | IT_CVAR,    NULL, "Autobalance sizes",                &cv_autobalance,        206},
	{IT_STRING | IT_CVAR,    NULL, "Scramble on Map Change",           &cv_scrambleonchange,   211},

#ifndef NONET
	{IT_HEADER, NULL, "Advanced", NULL, 220},
	{IT_STRING | IT_CVAR | IT_CV_STRING, NULL, "Master server",        &cv_masterserver,        226},
	{IT_STRING | IT_CVAR,    NULL, "Attempts to resynchronise",        &cv_resynchattempts,     240},
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
menu_t MainDef = CENTERMENUSTYLE(NULL, MainMenu, NULL, 72);

menu_t MISC_AddonsDef =
{
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
menu_t MISC_ScrambleTeamDef = DEFAULTMENUSTYLE(NULL, MISC_ScrambleTeamMenu, &MPauseDef, 27, 40);
menu_t MISC_ChangeTeamDef = DEFAULTMENUSTYLE(NULL, MISC_ChangeTeamMenu, &MPauseDef, 27, 40);

// MP Gametype and map change menu
menu_t MISC_ChangeLevelDef =
{
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

// Sky Room
menu_t SR_PandoraDef =
{
	"M_PANDRA",
	sizeof (SR_PandorasBox)/sizeof (menuitem_t),
	&SPauseDef,
	SR_PandorasBox,
	M_DrawGenericMenu,
	60, 30,
	0,
	M_ExitPandorasBox
};
menu_t SR_MainDef =
{
	"M_SECRET",
	sizeof (SR_MainMenu)/sizeof (menuitem_t),
	&MainDef,
	SR_MainMenu,
	M_DrawSkyRoom,
	60, 40,
	0,
	NULL
};

menu_t SR_LevelSelectDef = MAPPLATTERMENUSTYLE(NULL, SR_LevelSelectMenu);

menu_t SR_UnlockChecklistDef =
{
	"M_SECRET",
	1,
	&SR_MainDef,
	SR_UnlockChecklistMenu,
	M_DrawChecklist,
	30, 30,
	0,
	NULL
};
menu_t SR_EmblemHintDef =
{
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
menu_t SP_MainDef = CENTERMENUSTYLE(NULL, SP_MainMenu, &MainDef, 72);
menu_t SP_LoadDef =
{
	"M_PICKG",
	1,
	&SP_MainDef,
	SP_LoadGameMenu,
	M_DrawLoad,
	68, 46,
	0,
	NULL
};

menu_t SP_LevelSelectDef = MAPPLATTERMENUSTYLE(NULL, SP_LevelSelectMenu);

menu_t SP_LevelStatsDef =
{
	"M_STATS",
	1,
	&SP_MainDef,
	SP_LevelStatsMenu,
	M_DrawLevelStats,
	280, 185,
	0,
	NULL
};

menu_t SP_TimeAttackLevelSelectDef = MAPPLATTERMENUSTYLE("M_ATTACK", SP_TimeAttackLevelSelectMenu);

static menu_t SP_TimeAttackDef =
{
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
	"M_ATTACK",
	sizeof(SP_GhostMenu)/sizeof(menuitem_t),
	&SP_TimeAttackDef,
	SP_GhostMenu,
	M_DrawTimeAttackMenu,
	32, 120,
	0,
	NULL
};

menu_t SP_NightsAttackLevelSelectDef = MAPPLATTERMENUSTYLE("M_NIGHTS", SP_NightsAttackLevelSelectMenu);

static menu_t SP_NightsAttackDef =
{
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
	"M_NIGHTS",
	sizeof(SP_NightsGhostMenu)/sizeof(menuitem_t),
	&SP_NightsAttackDef,
	SP_NightsGhostMenu,
	M_DrawNightsAttackMenu,
	32, 120,
	0,
	NULL
};


menu_t SP_PlayerDef =
{
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
menu_t OP_MainDef = DEFAULTMENUSTYLE("M_OPTTTL", OP_MainMenu, &MainDef, 50, 30);
menu_t OP_ChangeControlsDef = CONTROLMENUSTYLE(OP_ChangeControlsMenu, &OP_MainDef);
menu_t OP_P1ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_P1ControlsMenu, &OP_MainDef, 50, 30);
menu_t OP_P2ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_P2ControlsMenu, &OP_MainDef, 50, 30);
menu_t OP_MouseOptionsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_MouseOptionsMenu, &OP_P1ControlsDef, 35, 30);
menu_t OP_Mouse2OptionsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_Mouse2OptionsMenu, &OP_P2ControlsDef, 35, 30);
menu_t OP_Joystick1Def = DEFAULTMENUSTYLE("M_CONTRO", OP_Joystick1Menu, &OP_P1ControlsDef, 50, 30);
menu_t OP_Joystick2Def = DEFAULTMENUSTYLE("M_CONTRO", OP_Joystick2Menu, &OP_P2ControlsDef, 50, 30);
menu_t OP_JoystickSetDef =
{
	"M_CONTRO",
	sizeof (OP_JoystickSetMenu)/sizeof (menuitem_t),
	&OP_Joystick1Def,
	OP_JoystickSetMenu,
	M_DrawJoystick,
	60, 40,
	0,
	NULL
};

menu_t OP_VideoOptionsDef =
{
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
	"M_VIDEO",
	sizeof (OP_ColorOptionsMenu)/sizeof (menuitem_t),
	&OP_VideoOptionsDef,
	OP_ColorOptionsMenu,
	M_DrawColorMenu,
	30, 30,
	0,
	NULL
};
menu_t OP_SoundOptionsDef =
{
	"M_SOUND",
	sizeof (OP_SoundOptionsMenu)/sizeof (menuitem_t),
	&OP_MainDef,
	OP_SoundOptionsMenu,
	M_DrawSoundMenu,
	30, 30,
	0,
	NULL
};

menu_t OP_ServerOptionsDef = DEFAULTSCROLLMENUSTYLE("M_SERVER", OP_ServerOptionsMenu, &OP_MainDef, 30, 30);

menu_t OP_MonitorToggleDef =
{
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
menu_t OP_OpenGLOptionsDef = DEFAULTMENUSTYLE("M_VIDEO", OP_OpenGLOptionsMenu, &OP_VideoOptionsDef, 30, 30);
#ifdef ALAM_LIGHTING
menu_t OP_OpenGLLightingDef = DEFAULTMENUSTYLE("M_VIDEO", OP_OpenGLLightingMenu, &OP_OpenGLOptionsDef, 60, 40);
#endif
menu_t OP_OpenGLFogDef =
{
	"M_VIDEO",
	sizeof (OP_OpenGLFogMenu)/sizeof (menuitem_t),
	&OP_OpenGLOptionsDef,
	OP_OpenGLFogMenu,
	M_OGL_DrawFogMenu,
	60, 40,
	0,
	NULL
};
menu_t OP_OpenGLColorDef =
{
	"M_VIDEO",
	sizeof (OP_OpenGLColorMenu)/sizeof (menuitem_t),
	&OP_OpenGLOptionsDef,
	OP_OpenGLColorMenu,
	M_OGL_DrawColorMenu,
	60, 40,
	0,
	NULL
};
#endif
menu_t OP_DataOptionsDef = DEFAULTMENUSTYLE("M_DATA", OP_DataOptionsMenu, &OP_MainDef, 60, 30);

menu_t OP_ScreenshotOptionsDef =
{
	"M_DATA",
	sizeof (OP_ScreenshotOptionsMenu)/sizeof (menuitem_t),
	&OP_DataOptionsDef,
	OP_ScreenshotOptionsMenu,
	M_DrawScreenshotMenu,
	30, 30,
	0,
	NULL
};

menu_t OP_AddonsOptionsDef = DEFAULTMENUSTYLE("M_ADDONS", OP_AddonsOptionsMenu, &OP_DataOptionsDef, 30, 30);

menu_t OP_EraseDataDef = DEFAULTMENUSTYLE("M_DATA", OP_EraseDataMenu, &OP_DataOptionsDef, 60, 30);

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
		sprintf(tabase,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));
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
		if (FIL_FileExists(va("%s-guest.lmp", tabase))) {
			SP_NightsReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			SP_NightsGuestReplayMenu[3].status = IT_WHITESTRING|IT_CALL;
			active = true;
		}
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
		{
			INT32 value = 0;

			switch (cv_newgametype.value)
			{
				case GT_COOP:
					value = TOL_COOP;
					break;
				case GT_COMPETITION:
					value = TOL_COMPETITION;
					break;
				case GT_RACE:
					value = TOL_RACE;
					break;
				case GT_MATCH:
				case GT_TEAMMATCH:
					value = TOL_MATCH;
					break;
				case GT_TAG:
				case GT_HIDEANDSEEK:
					value = TOL_TAG;
					break;
				case GT_CTF:
					value = TOL_CTF;
					break;
			}

			CV_SetValue(&cv_nextmap, M_GetFirstLevelInList(value));
		}
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

// ==========================================================================
// END ORGANIZATION STUFF.
// ==========================================================================

// current menudef
menu_t *currentMenu = &MainDef;

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
			MSCloseUDPSocket();		// Clean up so we can re-open the connection later.
			netgame = false;
			multiplayer = false;
		}

		if ((currentMenu->prevMenu == &MainDef) && (currentMenu == &SP_TimeAttackDef || currentMenu == &SP_NightsAttackDef))
		{
			// D_StartTitle does its own wipe, since GS_TIMEATTACK is now a complete gamestate.
			Z_Free(levelselect.rows);
			levelselect.rows = NULL;
			menuactive = false;
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

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_INVISSLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		CV_SetValue(cv,cv->value+(choice*2-1));
	}
	else if (cv->flags & CV_FLOAT)
	{
		char s[20];
		sprintf(s,"%f",FIXED_TO_FLOAT(cv->value)+(choice*2-1)*(1.0f/16.0f));
		CV_Set(cv,s);
	}
	else
		CV_AddValue(cv,choice*2-1);
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
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);
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
	} while (oldItemOn != itemOn && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);
}

// lock out further input in a tic when important buttons are pressed
// (in other words -- stop bullshit happening by mashing buttons in fades)
static boolean noFurtherInput = false;

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	INT32 ch = -1;
//	INT32 i;
	static tic_t joywait = 0, mousewait = 0;
	static INT32 pmousex = 0, pmousey = 0;
	static INT32 lastx = 0, lasty = 0;
	void (*routine)(INT32 choice); // for some casting problem

	if (dedicated || (demoplayback && titledemo)
	|| gamestate == GS_INTRO || gamestate == GS_CUTSCENE || gamestate == GS_GAMEEND
	|| gamestate == GS_CREDITS || gamestate == GS_EVALUATION)
		return false;

	if (noFurtherInput)
	{
		// Ignore input after enter/escape/other buttons
		// (but still allow shift keyup so caps doesn't get stuck)
		return false;
	}
	else if (ev->type == ev_keydown)
	{
		ch = ev->data1;

		// added 5-2-98 remap virtual keys (mouse & joystick buttons)
		switch (ch)
		{
			case KEY_MOUSE1:
			case KEY_JOY1:
			case KEY_JOY1 + 2:
				ch = KEY_ENTER;
				break;
			case KEY_JOY1 + 3:
				ch = 'n';
				break;
			case KEY_MOUSE1 + 1:
			case KEY_JOY1 + 1:
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
	else if (menuactive)
	{
		if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			if (ev->data3 == -1)
			{
				ch = KEY_UPARROW;
				joywait = I_GetTime() + NEWTICRATE/7;
			}
			else if (ev->data3 == 1)
			{
				ch = KEY_DOWNARROW;
				joywait = I_GetTime() + NEWTICRATE/7;
			}

			if (ev->data2 == -1)
			{
				ch = KEY_LEFTARROW;
				joywait = I_GetTime() + NEWTICRATE/17;
			}
			else if (ev->data2 == 1)
			{
				ch = KEY_RIGHTARROW;
				joywait = I_GetTime() + NEWTICRATE/17;
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
	}

	if (ch == -1)
		return false;

	// F-Keys
	if (!menuactive)
	{
		noFurtherInput = true;
		switch (ch)
		{
			case KEY_F1: // Help key
				if (modeattacking)
					return true;
				M_StartControlPanel();
				currentMenu = &MISC_HelpDef;
				itemOn = 0;
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
				currentMenu = &OP_SoundOptionsDef;
				itemOn = 0;
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
				{
					HU_clearChatChars();
					chat_on = false;
				}
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
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER)
			{
				if (routine)
					routine(ch);
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
					if (((currentMenu->menuitems[itemOn].status & IT_CALLTYPE) & IT_CALL_NOTMODIFIED) && modifiedgame && !savemoddata)
					{
						S_StartSound(NULL, sfx_skid);
						M_StartMessage(M_GetText("This cannot be done in a modified game.\n\n(Press a key)\n"), NULL, MM_NOTHING);
						return true;
					}
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
			// Why _does_ backspace go back anyway?
			//currentMenu->lastOn = itemOn;
			//if (currentMenu->prevMenu)
			//	M_SetupNextMenu(currentMenu->prevMenu);
			return false;

		default:
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
	if (currentMenu == &MessageDef)
		menuactive = true;

	if (menuactive)
	{
		// now that's more readable with a faded background (yeah like Quake...)
		if (!WipeInAction)
			V_DrawFadeScreen();

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
	if (window_notinfocus)
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
		MainMenu[secrets].status = (M_AnySecretUnlocked()) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		currentMenu = &MainDef;
		itemOn = singleplr;
	}
	else if (modeattacking)
	{
		currentMenu = &MAPauseDef;
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

			SPauseMenu[spause_pandora].status = (M_SecretUnlocked(SECRET_PANDORA)) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

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
		SPauseMenu[spause_levelselect].status = (gamecomplete) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

		// And emblem hints.
		SPauseMenu[spause_hints].status = (M_SecretUnlocked(SECRET_EMBLEMHINTS)) ? (IT_STRING | IT_CALL) : (IT_DISABLED);

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

		if ((server || adminplayer == consoleplayer))
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
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
	INT16 i;

	if (currentMenu->quitroutine)
	{
		// If you're going from a menu to itself, why are you running the quitroutine? You're not quitting it! -SH
		if (currentMenu != menudef && !currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}
	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = currentMenu->numitems - 1;

	// the curent item can be disabled,
	// this code go up until an enabled item found
	if ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE)
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if ((currentMenu->menuitems[i].status & IT_TYPE) != IT_SPACE)
			{
				itemOn = i;
				break;
			}
		}
	}
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
}

//
// M_Init
//
void M_Init(void)
{
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

#ifdef HWRENDER
	// Permanently hide some options based on render mode
	if (rendermode == render_soft)
		OP_VideoOptionsMenu[4].status = IT_DISABLED;
	else if (rendermode == render_opengl)
		OP_ScreenshotOptionsMenu[op_screenshot_colorprofile].status = IT_GRAYEDOUT;
#endif

#ifndef NONET
	CV_RegisterVar(&cv_serversort);
#endif
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

	V_DrawScaledPatch(xx, y, 0, p = W_CachePatchNum(leftlump,PU_CACHE));
	xx += SHORT(p->width) - SHORT(p->leftoffset);
	for (i = 0; i < 16; i++)
	{
		V_DrawScaledPatch(xx, y, V_WRAPX, W_CachePatchNum(centerlump[i & 1], PU_CACHE));
		xx += 8;
	}
	V_DrawScaledPatch(xx, y, 0, W_CachePatchNum(rightlump, PU_CACHE));

	xx = (cv->value - cv->PossibleValue[0].value) * (15*8) /
		(cv->PossibleValue[1].value - cv->PossibleValue[0].value);

	V_DrawScaledPatch((x + 8) + xx, y, 0, W_CachePatchNum(cursorlump, PU_CACHE));
}

//  A smaller 'Thermo', with range given as percents (0-100)
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv, boolean ontop)
{
	INT32 i;
	INT32 range;
	patch_t *p;

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	V_DrawScaledPatch(x, y, 0, W_CachePatchName("M_SLIDEL", PU_CACHE));

	p =  W_CachePatchName("M_SLIDEM", PU_CACHE);
	for (i = 1; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	if (ontop)
	{
		V_DrawCharacter(x - 6 - (skullAnimCounter/5), y,
			'\x1C' | V_YELLOWMAP, false);
		V_DrawCharacter(x+i*8 + 8 + (skullAnimCounter/5), y,
			'\x1D' | V_YELLOWMAP, false);
	}

	p = W_CachePatchName("M_SLIDER", PU_CACHE);
	V_DrawScaledPatch(x+i*8, y, 0, p);

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_CACHE);

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	if ((range = atoi(cv->defaultvalue)) != cv->value)
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
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TL], PU_CACHE));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_L], PU_CACHE);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BL], PU_CACHE));

	// draw middle
	V_DrawFlatFill(x + boff, y + boff, width*step, boxlines*step, st_borderpatchnum);

	cx += boff;
	cy = y;
	while (width > 0)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_T], PU_CACHE));
		V_DrawScaledPatch(cx, y + boff + boxlines*step, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_B], PU_CACHE));
		width--;
		cx += step;
	}

	// draw right side
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TR], PU_CACHE));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_R], PU_CACHE);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BR], PU_CACHE));
*/
}

static fixed_t staticalong = 0;

static void M_DrawStaticBox(fixed_t x, fixed_t y, INT32 flags, fixed_t w, fixed_t h)
{
	patch_t *patch;
	fixed_t sw, pw;

	patch = W_CachePatchName("LSSTATIC", PU_CACHE);
	pw = SHORT(patch->width) - (sw = w*2); //FixedDiv(w, scale); -- for scale FRACUNIT/2

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
static void M_DrawSaveLoadBorder(INT32 x,INT32 y)
{
	INT32 i;

	V_DrawScaledPatch (x-8,y+7,0,W_CachePatchName("M_LSLEFT",PU_CACHE));

	for (i = 0;i < 24;i++)
	{
		V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSCNTR",PU_CACHE));
		x += 8;
	}

	V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSRGHT",PU_CACHE));
}

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
			V_DrawSmallMappedPatch(x, y, 0, W_CachePatchName(M_GetEmblemPatch(emblem), PU_CACHE),
			                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
		else
			V_DrawSmallScaledPatch(x, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));

		emblem = M_GetLevelEmblems(-1);
		x -= 12;
	}
}

static void M_DrawMenuTitle(void)
{
	if (currentMenu->menutitlepic)
	{
		patch_t *p = W_CachePatchName(currentMenu->menutitlepic, PU_CACHE);

		if (p->height > 24) // title is larger than normal
		{
			INT32 xtitle = (BASEVIDWIDTH - (SHORT(p->width)/2))/2;
			INT32 ytitle = (30 - (SHORT(p->height)/2))/2;

			if (xtitle < 0)
				xtitle = 0;
			if (ytitle < 0)
				ytitle = 0;

			V_DrawSmallScaledPatch(xtitle, ytitle, 0, p);
		}
		else
		{
			INT32 xtitle = (BASEVIDWIDTH - SHORT(p->width))/2;
			INT32 ytitle = (30 - SHORT(p->height))/2;

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
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
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
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
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
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

#define scrollareaheight 72

// note that alphakey is multiplied by 2 for scrolling menus to allow greater usage in UINT8 range.
static void M_DrawGenericScrollMenu(void)
{
	INT32 x, y, i, max, bottom, tempcentery, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

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
				V_DrawString(x, y, V_TRANSLUCENT, currentMenu->menuitems[i].text);
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
		W_CachePatchName("M_CURSOR", PU_CACHE));
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
				V_DrawSmallMappedPatch(40, 44 + (i*8), 0, W_CachePatchName(M_GetEmblemPatch(emblem), PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(40, 44 + (i*8), 0, W_CachePatchName("NEEDIT", PU_CACHE));

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
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
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
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(x - V_StringWidth(currentMenu->menuitems[itemOn].text, 0)/2 - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
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

			if (mapvisited[mapnum]) // MV_MP
				return true;

			if (mapnum+1 == spstage_start)
				return true;

			// intentional fallthrough
		case LLM_RECORDATTACK:
		case LLM_NIGHTSATTACK:
			if (mapvisited[mapnum] & MV_MAX)
				return true;

			if (mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
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

			return false;

		case LLM_LEVELSELECT:
			if (mapheaderinfo[mapnum]->levelselect != maplistoption)
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
			const INT32 actnum = mapheaderinfo[mapnum]->actnum;
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

	if (levselp[0][0]) // never going to have some provided but not all, saves individually checking
	{
		W_UnlockCachedPatch(levselp[0][0]);
		W_UnlockCachedPatch(levselp[0][1]);
		W_UnlockCachedPatch(levselp[0][2]);

		W_UnlockCachedPatch(levselp[1][0]);
		W_UnlockCachedPatch(levselp[1][1]);
		W_UnlockCachedPatch(levselp[1][2]);
	}

	levselp[0][0] = W_CachePatchName("SLCT1LVL", PU_STATIC);
	levselp[0][1] = W_CachePatchName("SLCT2LVL", PU_STATIC);
	levselp[0][2] = W_CachePatchName("BLANKLVL", PU_STATIC);

	levselp[1][0] = W_CachePatchName("SLCT1LVW", PU_STATIC);
	levselp[1][1] = W_CachePatchName("SLCT2LVW", PU_STATIC);
	levselp[1][2] = W_CachePatchName("BLANKLVW", PU_STATIC);

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

	switch (choice)
	{
		case KEY_DOWNARROW:
			lsrow++;
			if (lsrow == levelselect.numrows)
				lsrow = 0;

			lsoffs[0] = lsvseperation(lsrow);

			if (levelselect.rows[lsrow].header[0])
				lshli = lsrow;
			// no else needed - headerless lines associate upwards, so moving down to a row without a header is identity

			S_StartSound(NULL,sfx_s3kb7);

			ifselectvalnextmap(lscol) else ifselectvalnextmap(0)
			break;

		case KEY_UPARROW:
			lsoffs[0] = -lsvseperation(lsrow);

			lsrow--;
			if (lsrow == UINT8_MAX)
				lsrow = levelselect.numrows-1;

			if (levelselect.rows[lsrow].header[0])
				lshli = lsrow;
			else
			{
				UINT8 iter = lsrow;
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
				else if (!lsoffs[0]) //  prevent sound spam
				{
					lsoffs[0] = -8;
					S_StartSound(NULL,sfx_s3kb2);
				}
				break;
			}
			// intentionall fallthrough
		case KEY_RIGHTARROW:
			if (levellistmode == LLM_CREATESERVER && !lsrow)
			{
				CV_AddValue(&cv_newgametype, 1);
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
			else if (!lsoffs[1]) //  prevent sound spam
			{
				lsoffs[1] = 8;
				S_StartSound(NULL,sfx_s3kb7);
			}
			break;

		case KEY_LEFTARROW:
			if (levellistmode == LLM_CREATESERVER && !lsrow)
			{
				CV_AddValue(&cv_newgametype, -1);
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
			else if (!lsoffs[1]) //  prevent sound spam
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
			patch = W_CachePatchName(va("%sW", G_BuildMapName(map)), PU_CACHE);
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
			patch = W_CachePatchName(va("%sP", G_BuildMapName(map)), PU_CACHE);
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

static void M_DrawLevelPlatterMenu(void)
{
	UINT8 iter = lsrow, sizeselect = (lswide(lsrow) ? 1 : 0);
	INT32 y = lsbasey + lsoffs[0] - getheadingoffset(lsrow);
	const INT32 cursorx = (sizeselect ? 0 : (lscol*lshseperation));

	if (gamestate == GS_TIMEATTACK)
		V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

	// finds row at top of the screen
	while (y > 0)
	{
		iter = ((iter == 0) ? levelselect.numrows-1 : iter-1);
		y -= lsvseperation(iter);
	}

	// draw from top to bottom
	while (y < 200)
	{
		M_DrawLevelPlatterRow(iter, y);
		y += lsvseperation(iter);
		iter = ((iter == levelselect.numrows-1) ? 0 : iter+1);
	}

	// draw cursor box
	if (levellistmode != LLM_CREATESERVER || lsrow)
		V_DrawSmallScaledPatch(lsbasex + cursorx + lsoffs[1], lsbasey+lsoffs[0], 0, (levselp[sizeselect][((skullAnimCounter/4) ? 1 : 0)]));

#if 0
	if (levelselect.rows[lsrow].maplist[lscol] > 0)
		V_DrawScaledPatch(lsbasex + cursorx-17, lsbasey+50+lsoffs[0], 0, W_CachePatchName("M_CURSOR", PU_CACHE));
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
		V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

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
		y += 8; //SHORT(hu_font[0]->height);
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
	pic_t *pictest = (pic_t *)W_CachePatchName(currentMenu->menuitems[itemOn].text,PU_CACHE);
	if (!pictest->zero)
		V_DrawScaledPic(0,0,0,W_GetNumForName(currentMenu->menuitems[itemOn].text));
	else
	{
		patch_t *patch = W_CachePatchName(currentMenu->menuitems[itemOn].text,PU_CACHE);
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

#define LOCATIONSTRING "Visit \x83SRB2.ORG/MODS\x80 to get & make add-ons!"

static void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

	/*if (cv_addons_option.value == 0)
		pathname = srb2home; usehome ? srb2home : srb2path;
	else if (cv_addons_option.value == 1)
		pathname = srb2home;
	else if (cv_addons_option.value == 2)
		pathname = srb2path;
	else*/
	if (cv_addons_option.value == 3 && *cv_addons_folder.string != '\0')
		pathname = cv_addons_folder.string;

	strlcpy(menupath, pathname, 1024);
	menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath) + 1;

	if (menupath[menupathindex[menudepthleft]-2] != '/')
	{
		menupath[menupathindex[menudepthleft]-1] = '/';
		menupath[menupathindex[menudepthleft]] = 0;
	}
	else
		--menupathindex[menudepthleft];

	if (!preparefilemenu(false))
	{
		M_StartMessage(M_GetText("No files/folders found.\n\n"LOCATIONSTRING"\n\n(Press a key)\n"),NULL,MM_NOTHING);
		return;
	}
	else
		dir_on[menudepthleft] = 0;

	if (addonsp[0]) // never going to have some provided but not all, saves individually checking
	{
		size_t i;
		for (i = 0; i < NUM_EXT+6; i++)
			W_UnlockCachedPatch(addonsp[i]);
	}

	addonsp[EXT_FOLDER] = W_CachePatchName("M_FFLDR", PU_STATIC);
	addonsp[EXT_UP] = W_CachePatchName("M_FBACK", PU_STATIC);
	addonsp[EXT_NORESULTS] = W_CachePatchName("M_FNOPE", PU_STATIC);
	addonsp[EXT_TXT] = W_CachePatchName("M_FTXT", PU_STATIC);
	addonsp[EXT_CFG] = W_CachePatchName("M_FCFG", PU_STATIC);
	addonsp[EXT_WAD] = W_CachePatchName("M_FWAD", PU_STATIC);
	addonsp[EXT_PK3] = W_CachePatchName("M_FPK3", PU_STATIC);
	addonsp[EXT_SOC] = W_CachePatchName("M_FSOC", PU_STATIC);
	addonsp[EXT_LUA] = W_CachePatchName("M_FLUA", PU_STATIC);
	addonsp[NUM_EXT] = W_CachePatchName("M_FUNKN", PU_STATIC);
	addonsp[NUM_EXT+1] = W_CachePatchName("M_FSEL1", PU_STATIC);
	addonsp[NUM_EXT+2] = W_CachePatchName("M_FSEL2", PU_STATIC);
	addonsp[NUM_EXT+3] = W_CachePatchName("M_FLOAD", PU_STATIC);
	addonsp[NUM_EXT+4] = W_CachePatchName("M_FSRCH", PU_STATIC);
	addonsp[NUM_EXT+5] = W_CachePatchName("M_FSAVE", PU_STATIC);

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

	if (menupath[0] == '.')
		strlcpy(header, va("SRB2 folder%s", menupath+1), 1024);
	else
		strcpy(header, menupath);

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
		addonsresponselimit = 0;

		if (refreshdirmenu & REFRESHDIR_NOTLOADED)
		{
			char *message = NULL;
			S_StartSound(NULL, sfx_lose);
			if (refreshdirmenu & REFRESHDIR_MAX)
				message = va("\x82%s\x80\nMaximum number of add-ons reached.\nThis file could not be loaded.\nIf you want to play with this add-on, restart the game to clear existing ones.\n\n(Press a key)\n", dirmenu[dir_on[menudepthleft]]+DIR_STRING);
			else
				message = va("\x82%s\x80\nThe file was not loaded.\nCheck the console log for more information.\n\n(Press a key)\n", dirmenu[dir_on[menudepthleft]]+DIR_STRING);
			M_StartMessage(message,NULL,MM_NOTHING);
			return true;
		}

		if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_skid);
			M_StartMessage(va("\x82%s\x80\nThe file was loaded with %s.\nCheck the console log for more information.\n\n(Press a key)\n", dirmenu[dir_on[menudepthleft]]+DIR_STRING, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings")),NULL,MM_NOTHING);
			return true;
		}

		S_StartSound(NULL, sfx_strpst);
	}

	return false;
}

#define offs 1

#ifdef __GNUC__
#pragma GCC optimize ("0")
#endif

static void M_DrawAddons(void)
{
	INT32 x, y;
	ssize_t i, max;

	// hack - need to refresh at end of frame to handle addfile...
	if (refreshdirmenu & M_AddonsRefresh())
		return M_DrawMessageMenu();

	if (addonsresponselimit)
		addonsresponselimit--;

	V_DrawCenteredString(BASEVIDWIDTH/2, 4+offs, 0, (Playing()
	? "\x85""Adding files mid-game may cause problems."
	: LOCATIONSTRING));

	if (numwadfiles <= mainwads+1)
		y = 0;
	else if (numwadfiles >= MAX_WADFILES)
		y = FRACUNIT;
	else
	{
		x = FixedDiv((numwadfiles - mainwads+1)<<FRACBITS, (MAX_WADFILES - mainwads+1)<<FRACBITS);
		y = FixedDiv(((packetsizetally-mainwadstally)<<FRACBITS), (((MAXFILENEEDED*sizeof(UINT8)-mainwadstally)-(5+22))<<FRACBITS)); // 5+22 = (a.ext + checksum length) is minimum addition to packet size tally
		if (x > y)
			y = x;
		if (y > FRACUNIT) // happens because of how we're shrinkin' it a little
			y = FRACUNIT;
	}

	M_DrawTemperature(BASEVIDWIDTH - 19 - 5, y);

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y + offs;

	//M_DrawLevelPlatterHeader(y - 16, M_AddonsHeaderPath(), true, true); -- wanted different width
	V_DrawString(x-21, (y - 16) + (lsheadingheight - 12), V_YELLOWMAP|V_ALLOWLOWERCASE, M_AddonsHeaderPath());
	V_DrawFill(x-21, (y - 16) + (lsheadingheight - 3), (MAXSTRINGLENGTH*8+6 - 1), 1, yellowmap[3]);
	V_DrawFill(x-21 + (MAXSTRINGLENGTH*8+6 - 1), (y - 16) + (lsheadingheight - 3), 1, 1, 26);
	V_DrawFill(x-21, (y - 16) + (lsheadingheight - 2), MAXSTRINGLENGTH*8+6, 1, 26);

	V_DrawFill(x - 21, y - 1, MAXSTRINGLENGTH*8+6, (BASEVIDHEIGHT - currentMenu->y + 1 + offs) - (y - 1), 159);

	// get bottom...
	max = dir_on[menudepthleft] + numaddonsshown + 1;
	if (max > (ssize_t)sizedirmenu)
		max = sizedirmenu;

	// then top...
	i = max - (2*numaddonsshown + 1);

	// then adjust!
	if (i < 0)
	{
		if ((max -= i) > (ssize_t)sizedirmenu)
			max = sizedirmenu;
		i = 0;
	}

	if (i != 0)
		V_DrawString(19, y+4 - (skullAnimCounter/5), V_YELLOWMAP, "\x1A");

	for (; i < max; i++)
	{
		UINT32 flags = V_ALLOWLOWERCASE;
		if (y > BASEVIDHEIGHT) break;
		if (dirmenu[i])
#define type (UINT8)(dirmenu[i][DIR_TYPE])
		{
			if (type & EXT_LOADED)
			flags |= V_TRANSLUCENT;

			V_DrawSmallScaledPatch(x-(16+4), y, (flags & V_TRANSLUCENT), addonsp[((UINT8)(dirmenu[i][DIR_TYPE]) & ~EXT_LOADED)]);

			if (type & EXT_LOADED)
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[NUM_EXT+3]);

			if ((size_t)i == dir_on[menudepthleft])
			{
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[NUM_EXT+1+((skullAnimCounter/4) ? 1 : 0)]);
				flags = V_ALLOWLOWERCASE|V_YELLOWMAP;
			}

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

	if (max != (ssize_t)sizedirmenu)
		V_DrawString(19, y-12 + (skullAnimCounter/5), V_YELLOWMAP, "\x1B");

	y = BASEVIDHEIGHT - currentMenu->y + offs;

	M_DrawTextBox(x - (21 + 5), y, MAXSTRINGLENGTH, 1);
	if (menusearch[0])
		V_DrawString(x - 18, y + 8, V_ALLOWLOWERCASE, menusearch+1);
	else
		V_DrawString(x - 18, y + 8, V_ALLOWLOWERCASE|V_TRANSLUCENT, "Type to search...");
	if (skullAnimCounter < 4)
		V_DrawCharacter(x - 18 + V_StringWidth(menusearch+1, 0), y + 8,
			'_' | 0x80, false);

	x -= (21 + 5 + 16);
	V_DrawSmallScaledPatch(x, y + 4, (menusearch[0] ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+4]);

#define CANSAVE (!modifiedgame || savemoddata)
	x = BASEVIDWIDTH - x - 16;
	V_DrawSmallScaledPatch(x, y + 4, (CANSAVE ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+5]);

	if (modifiedgame)
		V_DrawSmallScaledPatch(x, y + 4, 0, addonsp[NUM_EXT+3]);
#undef CANSAVE
}

#ifdef __GNUC__
#pragma GCC reset_options
#endif

#undef offs

static void M_AddonExec(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	S_StartSound(NULL, sfx_zoom);
	COM_BufAddText(va("exec %s%s", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
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

	if (addonsresponselimit)
		return;

	if (M_ChangeStringAddons(choice))
	{
		if (!preparefilemenu(true))
		{
			UNEXIST;
			return;
		}
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
									M_StartMessage(va("\x82%s\x80\nThis folder is empty.\n\n(Press a key)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING);
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
								M_StartMessage(va("\x82%s\x80\nThis folder is too deep to navigate to!\n\n(Press a key)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING);
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
							M_StartMessage(va("\x82%s\x80\nThis file may not be a console script.\nAttempt to run anyways? \n\n(Press 'Y' to confirm)\n", dirmenu[dir_on[menudepthleft]]+DIR_STRING),M_AddonExec,MM_YESNO);
							break;
						case EXT_CFG:
							M_AddonExec(KEY_ENTER);
							break;
						case EXT_LUA:
#ifndef HAVE_BLUA
							S_StartSound(NULL, sfx_lose);
							M_StartMessage(va("\x82%s\x80\nThis copy of SRB2 was compiled\nwithout support for .lua files.\n\n(Press a key)\n", dirmenu[dir_on[menudepthleft]]+DIR_STRING),NULL,MM_NOTHING);
							break;
#endif
						// else intentional fallthrough
						case EXT_SOC:
						case EXT_WAD:
						case EXT_PK3:
							COM_BufAddText(va("addfile %s%s", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
							addonsresponselimit = 5;
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
		for (; sizedirmenu > 0; sizedirmenu--)
		{
			Z_Free(dirmenu[sizedirmenu-1]);
			dirmenu[sizedirmenu-1] = NULL;
		}

		Z_Free(dirmenu);
		dirmenu = NULL;

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
	CV_StealthSetValue(&cv_dummyrings, max(players[consoleplayer].rings, 0));
	CV_StealthSetValue(&cv_dummylives, players[consoleplayer].lives);
	CV_StealthSetValue(&cv_dummycontinues, players[consoleplayer].continues);
	SR_PandorasBox[6].status = ((players[consoleplayer].charflags & SF_SUPER)
#ifndef DEVELOP
	|| cv_skin.value == 1
#endif
	) ? (IT_GRAYEDOUT) : (IT_STRING | IT_CALL);
	SR_PandorasBox[7].status = (emeralds == ((EMERALD7)*2)-1) ? (IT_GRAYEDOUT) : (IT_STRING | IT_CALL);
	M_SetupNextMenu(&SR_PandoraDef);
}

static boolean M_ExitPandorasBox(void)
{
	if (cv_dummyrings.value != max(players[consoleplayer].rings, 0))
		COM_ImmedExecute(va("setrings %d", cv_dummyrings.value));
	if (cv_dummylives.value != players[consoleplayer].lives)
		COM_ImmedExecute(va("setlives %d", cv_dummylives.value));
	if (cv_dummycontinues.value != players[consoleplayer].continues)
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
	OP_MainMenu[5].status = (Playing() && !(server || adminplayer == consoleplayer)) ? (IT_GRAYEDOUT) : (IT_STRING|IT_CALL);

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
//		CONS_Alert(CONS_WARNING, "Internal game map '%s' not found\n", G_BuildMapName(cv_nextmap.value));
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
		|| !unlockables[i].conditionset || unlockables[i].conditionset > MAXCONDITIONSETS)
			continue;

		V_DrawString(currentMenu->x, y, ((unlockables[i].unlocked) ? V_GREENMAP : V_TRANSLUCENT), ((unlockables[i].unlocked || !unlockables[i].nochecklist) ? unlockables[i].name : M_CreateSecretMenuOption(unlockables[i].name)));

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
					addy(8);
					V_DrawString(currentMenu->x, y,
						V_ALLOWLOWERCASE,
						va("\x1E %s", unlockables[i].objective));
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
												beat = va("Beat %s in %d:%d.%d", level,
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
											beat = va("Get a total time of less than %d:%d.%d",
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
													beat = va("Beat %s, mare %d in %d:%d.%d", level, cond[condnum].extrainfo2,
													G_TicsToMinutes(cond[condnum].requirement, true),
													G_TicsToSeconds(cond[condnum].requirement),
													G_TicsToCentiseconds(cond[condnum].requirement));
												else
													beat = va("Beat %s in %d:%d.%d",
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
	(void)choice;
	SR_EmblemHintMenu[0].status = (M_SecretUnlocked(SECRET_ITEMFINDER)) ? (IT_CVAR|IT_STRING) : (IT_SECRET);
	M_SetupNextMenu(&SR_EmblemHintDef);
	itemOn = 1; // always start on back.
}

static void M_DrawEmblemHints(void)
{
	INT32 i, j = 0;
	UINT32 collected = 0;
	emblem_t *emblem;
	const char *hint;

	for (i = 0; i < numemblems; i++)
	{
		emblem = &emblemlocations[i];
		if (emblem->level != gamemap || emblem->type > ET_SKIN)
			continue;

		if (emblem->collected)
		{
			collected = V_GREENMAP;
			V_DrawMappedPatch(12, 12+(28*j), 0, W_CachePatchName(M_GetEmblemPatch(emblem), PU_CACHE),
				R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(emblem), GTC_CACHE));
		}
		else
		{
			collected = 0;
			V_DrawScaledPatch(12, 12+(28*j), 0, W_CachePatchName("NEEDIT", PU_CACHE));
		}

		if (emblem->hint[0])
			hint = emblem->hint;
		else
			hint = M_GetText("No hints available.");
		hint = V_WordWrap(40, BASEVIDWIDTH-12, 0, hint);
		V_DrawString(40, 8+(28*j), V_RETURN8|V_ALLOWLOWERCASE|collected, hint);

		if (++j >= NUMHINTS)
			break;
	}
	if (!j)
		V_DrawCenteredString(160, 48, V_YELLOWMAP, "No hidden emblems on this map.");

	M_DrawGenericMenu();
}

static void M_DrawSkyRoom(void)
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
}

static void M_HandleSoundTest(INT32 choice)
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

		case KEY_RIGHTARROW:
			CV_AddValue(&cv_soundtest, 1);
			break;
		case KEY_LEFTARROW:
			CV_AddValue(&cv_soundtest, -1);
			break;
		case KEY_ENTER:
			S_StopSounds();
			S_StartSound(NULL, cv_soundtest.value);
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
		SR_MainMenu[i].alphaKey = (UINT8)unlockables[ul].height;

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
					SR_MainMenu[i].status = IT_STRING|IT_KEYHANDLER;
					SR_MainMenu[i].itemaction = M_HandleSoundTest;
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
	SP_MainMenu[sprecordattack].status =
		(M_SecretUnlocked(SECRET_RECORDATTACK)) ? IT_CALL|IT_STRING : IT_SECRET;
	SP_MainMenu[spnightsmode].status =
		(M_SecretUnlocked(SECRET_NIGHTSMODE)) ? IT_CALL|IT_STRING : IT_SECRET;

	M_SetupNextMenu(&SP_MainDef);
}

static void M_LoadGameLevelSelect(INT32 choice)
{
	(void)choice;

	SP_LevelSelectDef.prevMenu = currentMenu;
	levellistmode = LLM_LEVELSELECT;
	maplistoption = 1;

	if (!M_PrepareLevelPlatter(-1, true))
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&SP_LevelSelectDef);
}

// ==============
// LOAD GAME MENU
// ==============

static INT32 saveSlotSelected = 1;
static INT32 loadgamescroll = 0;
static UINT8 loadgameoffset = 0;

static void M_DrawLoadGameData(void)
{
	INT32 i, savetodraw, x, y, hsep = 90;
	skin_t *charskin = NULL;

	if (vid.width != BASEVIDWIDTH*vid.dupx)
		hsep = (hsep*vid.width)/(BASEVIDWIDTH*vid.dupx);

	for (i = -2; i <= 2; i++)
	{
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
#ifndef PERFECTSAVE // disabled, don't touch
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
					col = (charskin->prefcolor - 1)*2;
					col = Color_Index[Color_Opposite[col]-1][Color_Opposite[col+1]];
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
						patch = W_CachePatchNum(lumpnum, PU_CACHE);
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

		if ((savegameinfo[savetodraw].lives == -42)
		|| (savegameinfo[savetodraw].lives == -666))
			continue;

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

		y -= 13;

		// character heads, lives, and continues
		{
			spritedef_t *sprdef;
			spriteframe_t *sprframe;
			patch_t *patch;
			UINT8 *colormap = NULL;

			INT32 tempx = (x+40)<<FRACBITS, tempy = y<<FRACBITS, flip = 0, calc;

			// botskin first
			if (savegameinfo[savetodraw].botskin)
			{
				skin_t *charbotskin = &skins[savegameinfo[savetodraw].botskin-1];
				sprdef = &charbotskin->sprites[SPR2_SIGN];
				if (!sprdef->numframes)
					goto skipbot;
				colormap = R_GetTranslationColormap(savegameinfo[savetodraw].botskin, charbotskin->prefcolor, 0);
				sprframe = &sprdef->spriteframes[0];
				patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);

				V_DrawFixedPatch(
					tempx + (18<<FRACBITS),
					tempy -  (4<<FRACBITS),
					charbotskin->highresscale,
					0, patch, colormap);

				Z_Free(colormap);

				tempx -= (15<<FRACBITS);
				flip = V_FLIP;
			}
skipbot:
			// signpost image
			if (!charskin) // shut up compiler
				goto skipsign;
			sprdef = &charskin->sprites[SPR2_SIGN];
			colormap = R_GetTranslationColormap(savegameinfo[savetodraw].skinnum, charskin->prefcolor, 0);
			if (!sprdef->numframes)
				goto skipsign;
			sprframe = &sprdef->spriteframes[0];
			patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
			if ((calc = SHORT(patch->topoffset) - 42) > 0)
				tempy += ((4+calc)<<FRACBITS);

			V_DrawFixedPatch(
				tempx,
				tempy,
				charskin->highresscale,
				flip, patch, colormap);

skipsign:
			y += 25;

			tempx = x + 10;
			if (savegameinfo[savetodraw].lives != 0x7f
			&& savegameinfo[savetodraw].lives > 9)
				tempx -= 4;

			if (!charskin) // shut up compiler
				goto skiplife;

			// lives
			sprdef = &charskin->sprites[SPR2_LIFE];
			if (!sprdef->numframes)
				goto skiplife;
			sprframe = &sprdef->spriteframes[0];
			patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);

			V_DrawFixedPatch(
				(tempx + 4)<<FRACBITS,
				(y + 6)<<FRACBITS,
				charskin->highresscale/2,
				0, patch, colormap);
skiplife:
			if (colormap)
				Z_Free(colormap);

			patch = W_CachePatchName("STLIVEX", PU_CACHE);

			V_DrawScaledPatch(tempx + 9, y + 2, 0, patch);
			tempx += 16;
			if (savegameinfo[savetodraw].lives == 0x7f)
				V_DrawCharacter(tempx, y + 1, '\x16', false);
			else
				V_DrawString(tempx, y, 0, va("%d", savegameinfo[savetodraw].lives));

			tempx = x + 47;
			if (savegameinfo[savetodraw].continues > 9)
				tempx -= 4;

			// continues
			if (savegameinfo[savetodraw].continues > 0)
			{
				V_DrawSmallScaledPatch(tempx, y, 0, W_CachePatchName("CONTSAVE", PU_CACHE));
				V_DrawScaledPatch(tempx + 9, y + 2, 0, patch);
				V_DrawString(tempx + 16, y, 0, va("%d", savegameinfo[savetodraw].continues));
			}
			else
			{
				V_DrawSmallScaledPatch(tempx, y, 0, W_CachePatchName("CONTNONE", PU_CACHE));
				V_DrawScaledPatch(tempx + 9, y + 2, 0, W_CachePatchName("STNONEX", PU_CACHE));
				V_DrawString(tempx + 16, y, V_GRAYMAP, "0");
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
	(void)READINT32(save_p); // Score
	CHECKPOS
	savegameinfo[slot].continues = READINT32(save_p); // continues

	// File end marker check
	CHECKPOS
	if (READUINT8(save_p) != 0x1d) BADSAVE;

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

	if (savselp[0]) // never going to have some provided but not all, saves individually checking
	{
		W_UnlockCachedPatch(savselp[0]);
		W_UnlockCachedPatch(savselp[1]);
		W_UnlockCachedPatch(savselp[2]);

		W_UnlockCachedPatch(savselp[3]);
		W_UnlockCachedPatch(savselp[4]);
		W_UnlockCachedPatch(savselp[5]);
	}

	savselp[0] = W_CachePatchName("SAVEBACK", PU_STATIC);
	savselp[1] = W_CachePatchName("SAVENONE", PU_STATIC);
	savselp[2] = W_CachePatchName("ULTIMATE", PU_STATIC);

	savselp[3] = W_CachePatchName("GAMEDONE", PU_STATIC);
	savselp[4] = W_CachePatchName("BLACXLVL", PU_STATIC);
	savselp[5] = W_CachePatchName("BLANKLVL", PU_STATIC);
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
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
		Z_Free(savegameinfo);
		savegameinfo = NULL;
	}
}

//
// Selected from SRB2 menu
//
static void M_LoadGame(INT32 choice)
{
	(void)choice;

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

static void M_SetupChoosePlayer(INT32 choice)
{
	INT32 skinnum;
	UINT8 i;
	UINT8 firstvalid = 255;
	UINT8 lastvalid = 0;
	boolean allowed = false;
	char *name;
	(void)choice;

	SP_PlayerMenu[0].status &= ~IT_DYBIGSPACE; // Correcting a hack that may be made below.

	for (i = 0; i < 32; i++) // Handle charsels, availability, and unlocks.
	{
		if (description[i].used) // If the character's disabled through SOC, there's nothing we can do for it.
		{
			name = strtok(Z_StrDup(description[i].skinname), "&");
			skinnum = R_SkinAvailable(name);
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

				if (description[i].picname[0] == '\0')
					strncpy(description[i].picname, skins[skinnum].charsel, 8);
			}
			// else -- Technically, character select icons without corresponding skins get bundled away behind this too. Sucks to be them.
			Z_Free(name);
		}
	}

	if ((firstvalid != 255)
		&& !(mapheaderinfo[startmap-1]
			&& (mapheaderinfo[startmap-1]->forcecharacter[0] != '\0')
			)
		)
	{ // One last bit of order we can't do in the iteration above.
		description[firstvalid].prev = lastvalid;
		description[lastvalid].next = firstvalid;
	}
	else // We're being forced into a specific character, so might as well.
	{
		SP_PlayerMenu[0].status |= IT_DYBIGSPACE; // This is a dummy flag hack to make a non-IT_CALL character in slot 0 not softlock the game.
		M_ChoosePlayer(0);
		return;
	}


	if (Playing() == false)
	{
		S_StopMusic();
		S_ChangeMusicInternal("_chsel", true);
	}

	SP_PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SP_PlayerDef);
	if (!allowed)
	{
		char_on = firstvalid;
		if (startchar > 0 && startchar < 32)
		{
			INT16 workchar = startchar;
			while (workchar--)
				char_on = description[char_on].next;
		}
	}
	char_scroll = 0; // finish scrolling the menu
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

	switch (choice)
	{
		case KEY_DOWNARROW:
			if ((selectval = description[char_on].next) != char_on)
			{
				S_StartSound(NULL,sfx_s3kb7);
				char_on = selectval;
				char_scroll = -128*FRACUNIT;
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
				char_scroll = 128*FRACUNIT;
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
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

// Draw the choose player setup menu, had some fun with player anim
static void M_DrawSetupChoosePlayerMenu(void)
{
	const INT32 my = 24;
	patch_t *patch;
	INT32 i, o;
	UINT8 prev, next;

	// Black BG
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	//V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

	// Character select profile images!1
	M_DrawTextBox(0, my, 16, 20);

	if (abs(char_scroll) > FRACUNIT)
		char_scroll -= (char_scroll>>2);
	else // close enough.
		char_scroll = 0; // just be exact now.

	o = (char_scroll >> FRACBITS) + 16;

	if (o < 0) // A little hacky...
	{
		i = description[char_on].prev;
		o += 128;
	}
	else
		i = char_on;

	// Get prev character...
	prev = description[i].prev;

	if (prev != i) // If there's more than one character available...
	{
		// Let's get the next character now.
		next = description[i].next;

		// Draw prev character if it's visible and its number isn't greater than the current one or there's more than two
		if (o < 32)
		{
			patch = W_CachePatchName(description[prev].picname, PU_CACHE);
			if (SHORT(patch->width) >= 256)
				V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT/2, 0, patch, 0, SHORT(patch->height) + 2*(o-32), SHORT(patch->width), 64 - 2*o);
			else
				V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT, 0, patch, 0, SHORT(patch->height) + o - 32, SHORT(patch->width), 32 - o);
			W_UnlockCachedPatch(patch);
		}

		// Draw next character if it's visible and its number isn't less than the current one or there's more than two
		if (o < 128) // (next != i) was previously a part of this, but it's implicitly true if (prev != i) is true.
		{
			patch = W_CachePatchName(description[next].picname, PU_CACHE);
			if (SHORT(patch->width) >= 256)
				V_DrawCroppedPatch(8<<FRACBITS, (my + 168 - o)<<FRACBITS, FRACUNIT/2, 0, patch, 0, 0, SHORT(patch->width), 2*o);
			else
				V_DrawCroppedPatch(8<<FRACBITS, (my + 168 - o)<<FRACBITS, FRACUNIT, 0, patch, 0, 0, SHORT(patch->width), o);
			W_UnlockCachedPatch(patch);
		}
	}

	patch = W_CachePatchName(description[i].picname, PU_CACHE);
	if (o >= 0 && o <= 32)
	{
		if (SHORT(patch->width) >= 256)
			V_DrawSmallScaledPatch(8, my + 40 - o, 0, patch);
		else
			V_DrawScaledPatch(8, my + 40 - o, 0, patch);
	}
	else
	{
		if (SHORT(patch->width) >= 256)
			V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT/2, 0, patch, 0, (o-32)*2, SHORT(patch->width), SHORT(patch->height) - 2*(o-32));
		else
			V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT, 0, patch, 0, (o-32), SHORT(patch->width), SHORT(patch->height) - (o-32));
	}
	W_UnlockCachedPatch(patch);

	// draw title (or big pic)
	M_DrawMenuTitle();

	// Character description
	M_DrawTextBox(136, my, 21, 20);
	V_DrawString(146, my + 9, V_RETURN8|V_ALLOWLOWERCASE, char_notes);
}

// Chose the player you want to use Tails 03-02-2002
static void M_ChoosePlayer(INT32 choice)
{
	char *skin1,*skin2;
	INT32 skinnum;
	boolean ultmode = (ultimate_selectable && SP_PlayerDef.prevMenu == &SP_LoadDef && saveSlotSelected == NOSAVESLOT);

	// skip this if forcecharacter or no characters available
	if (!(SP_PlayerMenu[0].status & IT_DYBIGSPACE))
	{
		// M_SetupChoosePlayer didn't call us directly, that means we've been properly set up.
		char_scroll = 0; // finish scrolling the menu
		M_DrawSetupChoosePlayerMenu(); // draw the finally selected character one last time for the fadeout
	}
	M_ClearMenus(true);

	skin1 = strtok(description[choice].skinname, "&");
	skin2 = strtok(NULL, "&");

	if (skin2) {
		// this character has a second skin
		skinnum = R_SkinAvailable(skin1);
		botskin = (UINT8)(R_SkinAvailable(skin2)+1);
		botingame = true;

		botcolor = skins[botskin-1].prefcolor;

		// undo the strtok
		description[choice].skinname[strlen(skin1)] = '&';
	} else {
		skinnum = R_SkinAvailable(description[choice].skinname);
		botingame = false;
		botskin = 0;
		botcolor = 0;
	}

	if (startmap != spstage_start)
		cursaveslot = 0;

	//lastmapsaved = 0;
	gamecomplete = false;

	G_DeferedInitNew(ultmode, G_BuildMapName(startmap), (UINT8)skinnum, false, fromlevelselect);
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
			V_DrawString(20, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[mnum]->lvlttl, mapheaderinfo[mnum]->actnum));
		else
			V_DrawString(20, y, V_YELLOWMAP, mapheaderinfo[mnum]->lvlttl);

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
				V_DrawSmallMappedPatch(292, y, 0, W_CachePatchName(M_GetExtraEmblemPatch(exemblem), PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetExtraEmblemColor(exemblem), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(292, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));

			V_DrawString(20, y, V_YELLOWMAP, va("%s", exemblem->description));
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
	V_DrawSmallScaledPatch(20, 64, 0, W_CachePatchName("EMBLICON", PU_STATIC));

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
	INT32 i, x, y, cursory = 0;
	UINT16 dispstatus;
	patch_t *PictureOfUrFace;

	S_ChangeMusicInternal("_inter", true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

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
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	// Character face!
	if (W_CheckNumForName(skins[cv_chooseskin.value-1].charsel) != LUMPERROR)
	{
		PictureOfUrFace = W_CachePatchName(skins[cv_chooseskin.value-1].charsel, PU_CACHE);
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

		M_DrawLevelPlatterHeader(32-lsheadingheight/2, cv_nextmap.string, true, false);

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

		V_DrawSmallScaledPatch(208, 32+lsheadingheight, 0, PictureOfLevel);

		V_DrawString(104 - 72, 32+lsheadingheight/2, 0, "* LEVEL RECORDS *");

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->score)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%u", mainrecords[cv_nextmap.value-1]->score);

		V_DrawString(104-72, 48+lsheadingheight/2, V_YELLOWMAP, "SCORE:");
		V_DrawRightAlignedString(104+72, 48+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->time)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(mainrecords[cv_nextmap.value-1]->time, true),
			                                 G_TicsToSeconds(mainrecords[cv_nextmap.value-1]->time),
			                                 G_TicsToCentiseconds(mainrecords[cv_nextmap.value-1]->time));

		V_DrawString(104-72, 58+lsheadingheight/2, V_YELLOWMAP, "TIME:");
		V_DrawRightAlignedString(104+72, 58+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->rings)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%hu", mainrecords[cv_nextmap.value-1]->rings);

		V_DrawString(104-72, 68+lsheadingheight/2, V_YELLOWMAP, "RINGS:");
		V_DrawRightAlignedString(104+72, 68+lsheadingheight/2, V_ALLOWLOWERCASE, beststr);

		// Draw record emblems.
		em = M_GetLevelEmblems(cv_nextmap.value);
		while (em)
		{
			switch (em->type)
			{
				case ET_SCORE: yHeight = 48; break;
				case ET_TIME:  yHeight = 58; break;
				case ET_RINGS: yHeight = 68; break;
				default:
					goto skipThisOne;
			}

			if (em->collected)
				V_DrawSmallMappedPatch(104+76, yHeight+lsheadingheight/2, 0, W_CachePatchName(M_GetEmblemPatch(em), PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(104+76, yHeight+lsheadingheight/2, 0, W_CachePatchName("NEEDIT", PU_CACHE));

			skipThisOne:
			em = M_GetLevelEmblems(-1);
		}
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

	M_SetupNextMenu(&SP_TimeAttackDef);
	if (!M_CanShowLevelInList(cv_nextmap.value-1, -1) && levelselect.rows[0].maplist[0])
		CV_SetValue(&cv_nextmap, levelselect.rows[0].maplist[0]);
	else
		Nextmap_OnChange();

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusicInternal("_inter", true);

	itemOn = tastart; // "Start" is selected.
}

// Drawing function for Nights Attack
void M_DrawNightsAttackMenu(void)
{
	INT32 i, x, y, cursory = 0;
	UINT16 dispstatus;

	S_ChangeMusicInternal("_inter", true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

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
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	// Level record list
	if (cv_nextmap.value)
	{
		emblem_t *em;
		INT32 yHeight;
		patch_t *PictureOfLevel;
		lumpnum_t lumpnum;
		char beststr[40];

		UINT8 bestoverall	= G_GetBestNightsGrade(cv_nextmap.value, 0);
		UINT8 bestgrade		= G_GetBestNightsGrade(cv_nextmap.value, cv_dummymares.value);
		UINT32 bestscore	= G_GetBestNightsScore(cv_nextmap.value, cv_dummymares.value);
		tic_t besttime		= G_GetBestNightsTime(cv_nextmap.value, cv_dummymares.value);

		M_DrawLevelPlatterHeader(32-lsheadingheight/2, cv_nextmap.string, true, false);

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

		V_DrawSmallScaledPatch(208, 32+lsheadingheight, 0, PictureOfLevel);

		V_DrawString(104 - 72, 32+lsheadingheight/2, 0, "* LEVEL RECORDS *");

		if (P_HasGrades(cv_nextmap.value, 0))
			V_DrawScaledPatch(235, 135, 0, ngradeletters[bestoverall]);

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
					case ET_NGRADE: yHeight = 48; break;
					case ET_NTIME:  yHeight = 68; break;
					default:
						goto skipThisOne;
				}

				if (em->collected)
					V_DrawSmallMappedPatch(104+76, yHeight+lsheadingheight/2, 0, W_CachePatchName(M_GetEmblemPatch(em), PU_CACHE),
																 R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
				else
					V_DrawSmallScaledPatch(104+76, yHeight+lsheadingheight/2, 0, W_CachePatchName("NEEDIT", PU_CACHE));

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

	M_SetupNextMenu(&SP_NightsAttackDef);
	if (!M_CanShowLevelInList(cv_nextmap.value-1, -1) && levelselect.rows[0].maplist[0])
		CV_SetValue(&cv_nextmap, levelselect.rows[0].maplist[0]);
	else
		Nextmap_OnChange();

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusicInternal("_inter", true);

	itemOn = nastart; // "Start" is selected.
}

// Player has selected the "START" from the nights attack screen
static void M_ChooseNightsAttack(INT32 choice)
{
	char nameofdemo[256];
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	modeattacking = ATTACKING_NIGHTS;

	I_mkdir(va("%s"PATHSEP"replay", srb2home), 0755);
	I_mkdir(va("%s"PATHSEP"replay"PATHSEP"%s", srb2home, timeattackfolder), 0755);

	snprintf(nameofdemo, sizeof nameofdemo, "replay"PATHSEP"%s"PATHSEP"%s-last", timeattackfolder, G_BuildMapName(cv_nextmap.value));

	if (!cv_autorecord.value)
		remove(va("%s"PATHSEP"%s.lmp", srb2home, nameofdemo));
	else
		G_RecordDemo(nameofdemo);

	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), 0, false, false);
}

// Player has selected the "START" from the time attack screen
static void M_ChooseTimeAttack(INT32 choice)
{
	char *gpath;
	const size_t glen = strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char nameofdemo[256];
	(void)choice;
	emeralds = 0;
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
			which = "guest";
			break;
		}
		// srb2/replay/main/map01-score-best.lmp
		G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), which));
	}
}

static void M_EraseGuest(INT32 choice)
{
	const char *rguest = va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value));
	(void)choice;
	if (FIL_FileExists(rguest))
		remove(rguest);
	if (currentMenu == &SP_NightsGuestReplayDef)
		M_SetupNextMenu(&SP_NightsAttackDef);
	else
		M_SetupNextMenu(&SP_TimeAttackDef);
	Nextmap_OnChange();
	M_StartMessage(M_GetText("Guest replay data erased.\n"),NULL,MM_NOTHING);
}

static void M_OverwriteGuest(const char *which, boolean nights)
{
	char *rguest = Z_StrDup(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
	UINT8 *buf;
	size_t len;
	if (!nights)
		len = FIL_ReadFile(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), skins[cv_chooseskin.value-1].name, which), &buf);
	else
		len = FIL_ReadFile(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), which), &buf);
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
	M_OverwriteGuest("time-best", currentMenu == &SP_NightsGuestReplayDef);
}

static void M_OverwriteGuest_Score(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("score-best", currentMenu == &SP_NightsGuestReplayDef);
}

static void M_OverwriteGuest_Rings(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("rings-best", false);
}

static void M_OverwriteGuest_Last(INT32 choice)
{
	(void)choice;
	M_OverwriteGuest("last", currentMenu == &SP_NightsGuestReplayDef);
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

static void M_ModeAttackRetry(INT32 choice)
{
	(void)choice;
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
		break;
	case ATTACKING_NIGHTS:
		currentMenu = &SP_NightsAttackDef;
		break;
	}
	itemOn = currentMenu->lastOn;
	G_SetGamestate(GS_TIMEATTACK);
	modeattacking = ATTACKING_NONE;
	S_ChangeMusicInternal("_inter", true);
	Nextmap_OnChange();
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
	const char *rmotd;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	V_DrawString(currentMenu->x - 16, currentMenu->y, V_YELLOWMAP, M_GetText("Select a room"));

	M_DrawTextBox(144, 24, 20, 20);

	if (itemOn == 0)
		rmotd = M_GetText("Don't connect to the Master Server.");
	else
		rmotd = room_list[itemOn-1].motd;

	rmotd = V_WordWrap(0, 20*8, 0, rmotd);
	V_DrawString(144+8, 32, V_ALLOWLOWERCASE|V_RETURN8, rmotd);
}

static void M_DrawConnectMenu(void)
{
	UINT16 i;
	const char *gt = "Unknown";
	INT32 numPages = (serverlistcount+(SERVERS_PER_PAGE-1))/SERVERS_PER_PAGE;

	for (i = FIRSTSERVERLINE; i < min(localservercount, SERVERS_PER_PAGE)+FIRSTSERVERLINE; i++)
		MP_ConnectMenu[i].status = IT_STRING | IT_SPACE;

	if (!numPages)
		numPages = 1;

	// Room name
	if (ms_RoomId < 0)
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ConnectMenu[mp_connect_room].alphaKey,
		                         V_YELLOWMAP, (itemOn == mp_connect_room) ? "<Select to change>" : "<Offline Mode>");
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
		UINT32 globalflags = ((serverlist[slindex].info.numberofplayer >= serverlist[slindex].info.maxplayer) ? V_TRANSLUCENT : 0)
			|((itemOn == FIRSTSERVERLINE+i) ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE;

		V_DrawString(currentMenu->x, S_LINEY(i), globalflags, serverlist[slindex].info.servername);

		// Don't use color flags intentionally, the global yellow color will auto override the text color code
		if (serverlist[slindex].info.modifiedgame)
			V_DrawSmallString(currentMenu->x+202, S_LINEY(i)+8, globalflags, "\x85" "Mod");
		if (serverlist[slindex].info.cheatsenabled)
			V_DrawSmallString(currentMenu->x+222, S_LINEY(i)+8, globalflags, "\x83" "Cheats");

		V_DrawSmallString(currentMenu->x, S_LINEY(i)+8, globalflags,
		                     va("Ping: %u", (UINT32)LONG(serverlist[slindex].info.time)));

		gt = "Unknown";
		if (serverlist[slindex].info.gametype < NUMGAMETYPES)
			gt = Gametype_Names[serverlist[slindex].info.gametype];

		V_DrawSmallString(currentMenu->x+46,S_LINEY(i)+8, globalflags,
		                         va("Players: %02d/%02d", serverlist[slindex].info.numberofplayer, serverlist[slindex].info.maxplayer));

		V_DrawSmallString(currentMenu->x+112, S_LINEY(i)+8, globalflags, va("Gametype: %s", gt));

		MP_ConnectMenu[i+FIRSTSERVERLINE].status = IT_STRING | IT_CALL;
	}

	localservercount = serverlistcount;

	M_DrawGenericMenu();
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
SERVER_LIST_ENTRY_COMPARATOR(gametype)

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
		qsort(serverlist, serverlistcount, sizeof(serverelem_t), ServerListEntryComparator_gametype);
		break;
	}
#endif
}

#ifndef NONET
#ifdef UPDATE_ALERT
static int M_CheckMODVersion(void)
{
	char updatestring[500];
	const char *updatecheck = GetMODVersion();
	if(updatecheck)
	{
		sprintf(updatestring, UPDATE_ALERT_STRING, VERSIONSTRING, updatecheck);
		M_StartMessage(updatestring, NULL, MM_NOTHING);
		return false;
	} else
		return true;
}
#endif

static void M_ConnectMenu(INT32 choice)
{
	(void)choice;
	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	// first page of servers
	serverlistpage = 0;
	M_SetupNextMenu(&MP_ConnectDef);
	itemOn = 0;
	M_Refresh(0);
}

static UINT32 roomIds[NUM_LIST_ROOMS];

static void M_RoomMenu(INT32 choice)
{
	INT32 i;

	(void)choice;

	// Display a little "please wait" message.
	M_DrawTextBox(52, BASEVIDHEIGHT/2-10, 25, 3);
	V_DrawCenteredString(BASEVIDWIDTH/2, BASEVIDHEIGHT/2, 0, "Fetching room info...");
	V_DrawCenteredString(BASEVIDWIDTH/2, (BASEVIDHEIGHT/2)+12, 0, "Please wait.");
	I_OsPolling();
	I_UpdateNoBlit();
	if (rendermode == render_soft)
		I_FinishUpdate(); // page flip or blit buffer

	if (GetRoomsList(currentMenu == &MP_ServerDef) < 0)
		return;

#ifdef UPDATE_ALERT
	if (!M_CheckMODVersion())
		return;
#endif

	for (i = 1; i < NUM_LIST_ROOMS+1; ++i)
		MP_RoomMenu[i].status = IT_DISABLED;
	memset(roomIds, 0, sizeof(roomIds));

	for (i = 0; room_list[i].header.buffer[0]; i++)
	{
		if(*room_list[i].name != '\0')
		{
			MP_RoomMenu[i+1].text = room_list[i].name;
			roomIds[i] = room_list[i].id;
			MP_RoomMenu[i+1].status = IT_STRING|IT_CALL;
		}
	}

	MP_RoomDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_RoomDef);
}

static void M_ChooseRoom(INT32 choice)
{
	if (choice == 0)
		ms_RoomId = -1;
	else
	{
		ms_RoomId = roomIds[choice-1];
		menuRoomIndex = choice - 1;
	}

	serverlistpage = 0;
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
			                         V_YELLOWMAP, (itemOn == mp_server_room) ? "<Select to change>" : "<Offline Mode>");
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
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

		V_DrawSmallScaledPatch(319 - (currentMenu->x + (SHORT(PictureOfLevel->width)/2)), currentMenu->y + imgheight, 0, PictureOfLevel);
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
		OP_ServerOptionsMenu[34].status = IT_GRAYEDOUT; // Master server
		OP_ServerOptionsMenu[35].status = IT_GRAYEDOUT; // Attempts to resynchronise
	}
	else
	{
		OP_ServerOptionsMenu[ 1].status = IT_STRING | IT_CVAR | IT_CV_STRING;
		OP_ServerOptionsMenu[ 2].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[ 3].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[ 4].status = IT_STRING | IT_CVAR;
		OP_ServerOptionsMenu[34].status = (netgame
			? IT_GRAYEDOUT
			: (IT_STRING | IT_CVAR | IT_CV_STRING));
		OP_ServerOptionsMenu[35].status = IT_STRING | IT_CVAR;
	}
#endif

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

static char setupm_ip[16];

// Draw the funky Connect IP menu. Tails 11-19-2002
// So much work for such a little thing!
static void M_DrawMPMainMenu(void)
{
	INT32 x = currentMenu->x;
	INT32 y = currentMenu->y;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

#if MAXPLAYERS == 32
	V_DrawRightAlignedString(BASEVIDWIDTH-x, y+12,
		((itemOn == 1) ? V_YELLOWMAP : 0), "(2-32 players)");
#else
Update the maxplayers label...
#endif

	V_DrawRightAlignedString(BASEVIDWIDTH-x, y+22,
		((itemOn == 2) ? V_YELLOWMAP : 0), "(2 players)");

	V_DrawRightAlignedString(BASEVIDWIDTH-x, y+116,
		((itemOn == 8) ? V_YELLOWMAP : 0), "(splitscreen)");

	y += 62;

	V_DrawFill(x+5, y+4+5, /*16*8 + 6,*/ BASEVIDWIDTH - 2*(x+5), 8+6, 159);

	// draw name string
	V_DrawString(x+8,y+12, V_MONOSPACE, setupm_ip);

	// draw text cursor for name
	if (itemOn == 5 //0
	    && skullAnimCounter < 4)   //blink cursor
		V_DrawCharacter(x+8+V_StringWidth(setupm_ip, V_MONOSPACE),y+12,'_',false);
}

// Tails 11-19-2002
static void M_ConnectIP(INT32 choice)
{
	(void)choice;
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
			M_ClearMenus(true);
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
			if (setupm_ip[0])
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[0] = 0;
			}
			break;

		default:
			l = strlen(setupm_ip);
			if (l >= 16-1)
				break;

			if (choice == 46 || (choice >= 48 && choice <= 57)) // Rudimentary number and period enforcing
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
static char       setupm_name[MAXPLAYERNAME+1];
static player_t  *setupm_player;
static consvar_t *setupm_cvskin;
static consvar_t *setupm_cvcolor;
static consvar_t *setupm_cvname;
static consvar_t *setupm_cvdefaultskin;
static consvar_t *setupm_cvdefaultcolor;
static consvar_t *setupm_cvdefaultname;
static INT32      setupm_fakeskin;
static INT32      setupm_fakecolor;

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
	             ((MP_PlayerSetupMenu[2].status & IT_TYPE) == IT_SPACE ? V_TRANSLUCENT : 0)|(itemOn == 1 ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE,
	             skins[setupm_fakeskin].realname);

	if (itemOn == 1 && (MP_PlayerSetupMenu[2].status & IT_TYPE) != IT_SPACE)
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

	if (!setupm_fakecolor || !sprdef->numframes) // should never happen but hey, who knows
		goto faildraw;

	// ok, draw player sprite for sure now
	colormap = R_GetTranslationColormap(setupm_fakeskin, setupm_fakecolor, 0);

	if (multi_frame >= sprdef->numframes)
		multi_frame = 0;

	sprframe = &sprdef->spriteframes[multi_frame];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
	if (sprframe->flip & 1) // Only for first sprite
		flags |= V_FLIP; // This sprite is left/right flipped!

#define chary (y+64)

	V_DrawFixedPatch(
		x<<FRACBITS,
		chary<<FRACBITS,
		FixedDiv(skins[setupm_fakeskin].highresscale, skins[setupm_fakeskin].shieldscale),
		flags, patch, colormap);

	Z_Free(colormap);
	goto colordraw;

faildraw:
	sprdef = &sprites[SPR_UNKN];
	if (!sprdef->numframes) // No frames ??
		return; // Can't render!

	sprframe = &sprdef->spriteframes[0];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
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
	             (itemOn == 2 ? V_YELLOWMAP : 0)|V_ALLOWLOWERCASE,
	             Color_Names[setupm_fakecolor]);

	if (itemOn == 2 && (MP_PlayerSetupMenu[2].status & IT_TYPE) != IT_SPACE)
	{
		V_DrawCharacter(BASEVIDWIDTH - x - 10 - V_StringWidth(Color_Names[setupm_fakecolor], V_ALLOWLOWERCASE) - (skullAnimCounter/5), y,
			'\x1C' | V_YELLOWMAP, false);
		V_DrawCharacter(BASEVIDWIDTH - x + 2 + (skullAnimCounter/5), y,
			'\x1D' | V_YELLOWMAP, false);
	}

	y += 11;

#define indexwidth 8
	{
		const INT32 colwidth = (282-charw)/(2*indexwidth);
		INT32 i = -colwidth;
		INT16 col = setupm_fakecolor - colwidth;
		INT32 w = indexwidth;
		UINT8 h;

		while (col < 1)
			col += MAXSKINCOLORS-1;
		while (i <= colwidth)
		{
			if (!(i++))
				w = charw;
			else
				w = indexwidth;
			for (h = 0; h < 16; h++)
				V_DrawFill(x, y+h, w, 1, Color_Index[col-1][h]);
			if (++col >= MAXSKINCOLORS)
				col -= MAXSKINCOLORS-1;
			x += w;
		}
	}
#undef charw
#undef indexwidth

	x = MP_PlayerSetupDef.x;
	y += 20;

	V_DrawString(x, y,
		((R_SkinAvailable(setupm_cvdefaultskin->string) != setupm_fakeskin
		|| setupm_cvdefaultcolor->value != setupm_fakecolor
		|| strcmp(setupm_name, setupm_cvdefaultname->string))
			? 0
			: V_TRANSLUCENT)
		| ((itemOn == 3) ? V_YELLOWMAP : 0),
		"Save as default");
	if (itemOn == 3)
		cursory = y;

	V_DrawScaledPatch(x - 17, cursory, 0,
		W_CachePatchName("M_CURSOR", PU_CACHE));
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
				setupm_fakecolor--;
			}
			break;

		case KEY_ENTER:
			if (itemOn == 3
			&& (R_SkinAvailable(setupm_cvdefaultskin->string) != setupm_fakeskin
			|| setupm_cvdefaultcolor->value != setupm_fakecolor
			|| strcmp(setupm_name, setupm_cvdefaultname->string)))
			{
				S_StartSound(NULL,sfx_strpst);
				// you know what? always putting these in the buffer won't hurt anything.
				COM_BufAddText (va("%s \"%s\"\n",setupm_cvdefaultskin->name,skins[setupm_fakeskin].name));
				COM_BufAddText (va("%s %d\n",setupm_cvdefaultcolor->name,setupm_fakecolor));
				COM_BufAddText (va("%s %s\n",setupm_cvdefaultname->name,setupm_name));
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
				setupm_fakecolor++;
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
			if (l < MAXPLAYERNAME-1)
			{
				setupm_name[l] = (char)choice;
				setupm_name[l+1] = 0;
			}
			break;
	}

	// check color
	if (setupm_fakecolor < 1)
		setupm_fakecolor = MAXSKINCOLORS-1;
	if (setupm_fakecolor > MAXSKINCOLORS-1)
		setupm_fakecolor = 1;

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
	setupm_cvdefaultname = &cv_defaultplayername;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;
	setupm_fakecolor = setupm_cvcolor->value;

	// disable skin changes if we can't actually change skins
	if (!CanChangeSkin(consoleplayer))
		MP_PlayerSetupMenu[1].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[1].status = (IT_KEYHANDLER|IT_STRING);

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
	setupm_cvdefaultname = &cv_defaultplayername2;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;
	setupm_fakecolor = setupm_cvcolor->value;

	// disable skin changes if we can't actually change skins
	if (splitscreen && !CanChangeSkin(secondarydisplayplayer))
		MP_PlayerSetupMenu[1].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[1].status = (IT_KEYHANDLER | IT_STRING);

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
	// you know what? always putting these in the buffer won't hurt anything.
	COM_BufAddText (va("%s \"%s\"\n",setupm_cvskin->name,skins[setupm_fakeskin].name));
	COM_BufAddText (va("%s %d\n",setupm_cvcolor->name,setupm_fakecolor));
	return true;
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
		eschoice = M_GetText("Secrets data");
	else
		eschoice = M_GetText("ALL game data");

	M_StartMessage(va(esstr, eschoice),M_EraseDataResponse,MM_YESNO);
}

static void M_ScreenshotOptions(INT32 choice)
{
	(void)choice;
	Screenshot_option_Onchange();
	Moviemode_mode_Onchange();

	M_SetupNextMenu(&OP_ScreenshotOptionsDef);
}

// =============
// JOYSTICK MENU
// =============

// Start the controls menu, setting it up for either the console player,
// or the secondary splitscreen player

static void M_DrawJoystick(void)
{
	INT32 i;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i <= 4; i++) // See MAX_JOYSTICKS
	{
		M_DrawSaveLoadBorder(OP_JoystickSetDef.x+4, OP_JoystickSetDef.y+1+LINEHEIGHT*i);

		if ((setupcontrols_secondaryplayer && (i == cv_usejoystick2.value))
			|| (!setupcontrols_secondaryplayer && (i == cv_usejoystick.value)))
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i,V_GREENMAP,joystickInfo[i]);
		else
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i,0,joystickInfo[i]);

		if (i == itemOn)
		{
			V_DrawScaledPatch(currentMenu->x - 24, OP_JoystickSetDef.y+LINEHEIGHT*i, 0,
				W_CachePatchName("M_CURSOR", PU_CACHE));
		}
	}
}

static void M_SetupJoystickMenu(INT32 choice)
{
	INT32 i = 0;
	const char *joyname = "None";
	const char *joyNA = "Unavailable";
	INT32 n = I_NumJoys();
	(void)choice;

	strcpy(joystickInfo[i], joyname);

	for (i = 1; i < 8; i++)
	{
		if (i <= n && (joyname = I_GetJoyName(i)) != NULL)
		{
			strncpy(joystickInfo[i], joyname, 24);
			joystickInfo[i][24] = '\0';
		}
		else
			strcpy(joystickInfo[i], joyNA);
	}

	M_SetupNextMenu(&OP_JoystickSetDef);
}

static void M_Setup1PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = false;
	OP_JoystickSetDef.prevMenu = &OP_Joystick1Def;
	M_SetupJoystickMenu(choice);
}

static void M_Setup2PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = true;
	OP_JoystickSetDef.prevMenu = &OP_Joystick2Def;
	M_SetupJoystickMenu(choice);
}

static void M_AssignJoystick(INT32 choice)
{
	if (setupcontrols_secondaryplayer)
		CV_SetValue(&cv_usejoystick2, choice);
	else
		CV_SetValue(&cv_usejoystick, choice);
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

	// Unhide the five non-P2 controls and their headers
	OP_ChangeControlsMenu[18+0].status = IT_HEADER;
	OP_ChangeControlsMenu[18+1].status = IT_SPACE;
	// ...
	OP_ChangeControlsMenu[18+2].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+3].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[18+4].status = IT_CALL|IT_STRING2;
	// ...
	OP_ChangeControlsMenu[23+0].status = IT_HEADER;
	OP_ChangeControlsMenu[23+1].status = IT_SPACE;
	// ...
	OP_ChangeControlsMenu[23+2].status = IT_CALL|IT_STRING2;
	OP_ChangeControlsMenu[23+3].status = IT_CALL|IT_STRING2;

	OP_ChangeControlsDef.prevMenu = &OP_P1ControlsDef;
	M_SetupNextMenu(&OP_ChangeControlsDef);
}

static void M_Setup2PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = true;
	setupcontrols = gamecontrolbis;
	currentMenu->lastOn = itemOn;

	// Hide the five non-P2 controls and their headers
	OP_ChangeControlsMenu[18+0].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+1].status = IT_GRAYEDOUT2;
	// ...
	OP_ChangeControlsMenu[18+2].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+3].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[18+4].status = IT_GRAYEDOUT2;
	// ...
	OP_ChangeControlsMenu[23+0].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[23+1].status = IT_GRAYEDOUT2;
	// ...
	OP_ChangeControlsMenu[23+2].status = IT_GRAYEDOUT2;
	OP_ChangeControlsMenu[23+3].status = IT_GRAYEDOUT2;

	OP_ChangeControlsDef.prevMenu = &OP_P2ControlsDef;
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
		W_CachePatchName("M_CURSOR", PU_CACHE));
}

#undef controlbuffer

static INT32 controltochange;

static void M_ChangecontrolResponse(event_t *ev)
{
	INT32        control;
	INT32        found;
	INT32        ch = ev->data1;

	// ESCAPE cancels
	if (ch != KEY_ESCAPE)
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
			G_CheckDoubleUsage(ch);
			setupcontrols[control][found] = ch;
		}
		S_StartSound(NULL, sfx_strpst);
	}
	else
		S_StartSound(NULL, sfx_skid);

	M_StopMessage(0);
}

static void M_ChangeControl(INT32 choice)
{
	static char tmp[55];

	controltochange = currentMenu->menuitems[choice].alphaKey;
	sprintf(tmp, M_GetText("Hit the new key for\n%s\nESC for Cancel"),
		currentMenu->menuitems[choice].text);

	M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
}

// =====
// SOUND
// =====

static void M_SoundMenu(INT32 choice)
{
	(void)choice;

	OP_SoundOptionsMenu[6].status = ((nosound || sound_disabled) ? IT_GRAYEDOUT : (IT_STRING | IT_CVAR));
	M_SetupNextMenu(&OP_SoundOptionsDef);
}

void M_DrawSoundMenu(void)
{
	const char* onstring = "ON";
	const char* offstring = "OFF";
	INT32 lengthstring;
	M_DrawGenericMenu();

	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x,
		currentMenu->y+currentMenu->menuitems[0].alphaKey,
		(nosound ? V_REDMAP : V_YELLOWMAP),
		((nosound || sound_disabled) ? offstring : onstring));

	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x,
		currentMenu->y+currentMenu->menuitems[2].alphaKey,
		(nodigimusic ? V_REDMAP : V_YELLOWMAP),
		((nodigimusic || digital_disabled) ? offstring : onstring));

	V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x,
		currentMenu->y+currentMenu->menuitems[4].alphaKey,
		(nomidimusic ? V_REDMAP : V_YELLOWMAP),
		((nomidimusic || music_disabled) ? offstring : onstring));

	if (itemOn == 0)
		lengthstring = ((nosound || sound_disabled) ? 3 : 2);
	else if (itemOn == 2)
		lengthstring = ((nodigimusic || digital_disabled) ? 3 : 2);
	else if (itemOn == 4)
		lengthstring = ((nomidimusic || music_disabled) ? 3 : 2);
	else
		return;

	V_DrawCharacter(BASEVIDWIDTH - currentMenu->x - 10 - (lengthstring*8) - (skullAnimCounter/5), currentMenu->y+currentMenu->menuitems[itemOn].alphaKey,
		'\x1C' | V_YELLOWMAP, false);
	V_DrawCharacter(BASEVIDWIDTH - currentMenu->x + 2 + (skullAnimCounter/5), currentMenu->y+currentMenu->menuitems[itemOn].alphaKey,
		'\x1D' | V_YELLOWMAP, false);
}

// Toggles sound systems in-game.
static void M_ToggleSFX(INT32 choice)
{
	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn++;
			return;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn = currentMenu->numitems-1;
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

	if (nosound)
	{
		nosound = false;
		I_StartupSound();
		if (nosound) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_StartSound(NULL, sfx_strpst);
		OP_SoundOptionsMenu[6].status = IT_STRING | IT_CVAR;
		//M_StartMessage(M_GetText("SFX Enabled\n"), NULL, MM_NOTHING);
	}
	else
	{
		if (sound_disabled)
		{
			sound_disabled = false;
			S_StartSound(NULL, sfx_strpst);
			OP_SoundOptionsMenu[6].status = IT_STRING | IT_CVAR;
			//M_StartMessage(M_GetText("SFX Enabled\n"), NULL, MM_NOTHING);
		}
		else
		{
			sound_disabled = true;
			S_StopSounds();
			OP_SoundOptionsMenu[6].status = IT_GRAYEDOUT;
			//M_StartMessage(M_GetText("SFX Disabled\n"), NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleDigital(INT32 choice)
{
	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn++;
			return;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn--;
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

	if (nodigimusic)
	{
		nodigimusic = false;
		I_InitDigMusic();
		if (nodigimusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_StopMusic();
		if (Playing())
			P_RestoreMusic(&players[consoleplayer]);
		else
			S_ChangeMusicInternal("_clear", false);
		//M_StartMessage(M_GetText("Digital Music Enabled\n"), NULL, MM_NOTHING);
	}
	else
	{
		if (digital_disabled)
		{
			digital_disabled = false;
			if (Playing())
				P_RestoreMusic(&players[consoleplayer]);
			else
				S_ChangeMusicInternal("_clear", false);
			//M_StartMessage(M_GetText("Digital Music Enabled\n"), NULL, MM_NOTHING);
		}
		else
		{
			digital_disabled = true;
			S_StopMusic();
			//M_StartMessage(M_GetText("Digital Music Disabled\n"), NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleMIDI(INT32 choice)
{
	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn++;
			return;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn--;
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

	if (nomidimusic)
	{
		nomidimusic = false;
		I_InitMIDIMusic();
		if (nomidimusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		if (Playing())
			P_RestoreMusic(&players[consoleplayer]);
		else
			S_ChangeMusicInternal("_clear", false);
		//M_StartMessage(M_GetText("MIDI Music Enabled\n"), NULL, MM_NOTHING);
	}
	else
	{
		if (music_disabled)
		{
			music_disabled = false;
			if (Playing())
				P_RestoreMusic(&players[consoleplayer]);
			else
				S_ChangeMusicInternal("_clear", false);
			//M_StartMessage(M_GetText("MIDI Music Enabled\n"), NULL, MM_NOTHING);
		}
		else
		{
			music_disabled = true;
			S_StopMusic();
			//M_StartMessage(M_GetText("MIDI Music Disabled\n"), NULL, MM_NOTHING);
		}
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
		W_CachePatchName("M_CURSOR", PU_CACHE));
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
		W_CachePatchName("M_CURSOR", PU_CACHE));
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
		V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, y, V_REDMAP, "ON");
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
	if (!(netgame || cv_debug))
	{
		S_ResetCaptions();

		mrand = M_RandomKey(sizeof(quitsounds)/sizeof(INT32));
		if (quitsounds[mrand]) S_StartSound(NULL, quitsounds[mrand]);

		//added : 12-02-98: do that instead of I_WaitVbl which does not work
		ptime = I_GetTime() + NEWTICRATE*2; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
		while (ptime > I_GetTime())
		{
			V_DrawScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_CACHE)); // Demo 3 Quit Screen Tails 06-16-2001
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

#ifdef HWRENDER
// =====================================================================
// OpenGL specific options
// =====================================================================

#define FOG_COLOR_ITEM  1
// ===================
// M_OGL_DrawFogMenu()
// ===================
static void M_OGL_DrawFogMenu(void)
{
	INT32 mx, my;

	mx = currentMenu->x;
	my = currentMenu->y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(BASEVIDWIDTH - mx - V_StringWidth(cv_grfogcolor.string, 0),
		my + currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, V_YELLOWMAP, cv_grfogcolor.string);
	// blink cursor on FOG_COLOR_ITEM if selected
	if (itemOn == FOG_COLOR_ITEM && skullAnimCounter < 4)
		V_DrawCharacter(BASEVIDWIDTH - mx,
			my + currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, '_' | 0x80,false);
}

// =====================
// M_OGL_DrawColorMenu()
// =====================
static void M_OGL_DrawColorMenu(void)
{
	INT32 mx, my;

	mx = currentMenu->x;
	my = currentMenu->y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(mx, my + currentMenu->menuitems[0].alphaKey - 10,
		V_YELLOWMAP, "Gamma correction");
}

//===================
// M_HandleFogColor()
//===================
static void M_HandleFogColor(INT32 choice)
{
	size_t i, l;
	char temp[8];
	boolean exitmenu = false; // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn++;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn--;
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			S_StartSound(NULL, sfx_menu1);
			strcpy(temp, cv_grfogcolor.string);
			strcpy(cv_grfogcolor.zstring, "000000");
			l = strlen(temp)-1;
			for (i = 0; i < l; i++)
				cv_grfogcolor.zstring[i + 6 - l] = temp[i];
			break;

		default:
			if ((choice >= '0' && choice <= '9') || (choice >= 'a' && choice <= 'f')
				|| (choice >= 'A' && choice <= 'F'))
			{
				S_StartSound(NULL, sfx_menu1);
				strcpy(temp, cv_grfogcolor.string);
				strcpy(cv_grfogcolor.zstring, "000000");
				l = strlen(temp);
				for (i = 0; i < l; i++)
					cv_grfogcolor.zstring[5 - i] = temp[l - i];
				cv_grfogcolor.zstring[5] = (char)choice;
			}
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
#endif
