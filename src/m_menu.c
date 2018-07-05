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
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define MAXSTRINGLENGTH 32
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
description_t description[32] =
{
	{"\x82Sonic\x80 is the fastest of the three, but also the hardest to control. Beginners beware, but experts will find Sonic very powerful.\n\n\x82""Ability:\x80 Speed Thok\nDouble jump to zoom forward with a huge burst of speed.\n\n\x82Tip:\x80 Simply letting go of forward does not slow down in SRB2. To slow down, hold the opposite direction.", "", "sonic"},
	{"\x82Tails\x80 is the most mobile of the three, but has the slowest speed. Because of his mobility, he's well-\nsuited to beginners.\n\n\x82""Ability:\x80 Fly\nDouble jump to start flying for a limited time. Repetitively hit the jump button to ascend.\n\n\x82Tip:\x80 To quickly descend while flying, hit the spin button.", "", "tails"},
	{"\x82Knuckles\x80 is well-\nrounded and can destroy breakable walls simply by touching them, but he can't jump as high as the other two.\n\n\x82""Ability:\x80 Glide & Climb\nDouble jump to glide in the air as long as jump is held. Glide into a wall to climb it.\n\n\x82Tip:\x80 Press spin while climbing to jump off the wall; press jump instead to jump off\nand face away from\nthe wall.", "", "knuckles"},
	{"\x82Sonic & Tails\x80 team up to take on Dr. Eggman!\nControl Sonic while Tails desperately struggles to keep up.\n\nPlayer 2 can control Tails directly by setting the controls in the options menu.\nTails's directional controls are relative to Player 1's camera.\n\nTails can pick up Sonic while flying and carry him around.", "CHRS&T", "sonic&tails"},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""},
	{"???", "", ""}
};
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

static saveinfo_t savegameinfo[MAXSAVEGAMES]; // Extra info about the save games.

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

// Sky Room
static void M_CustomLevelSelect(INT32 choice);
static void M_CustomWarp(INT32 choice);
FUNCNORETURN static ATTRNORETURN void M_UltimateCheat(INT32 choice);
static void M_LoadGameLevelSelect(INT32 choice);
static void M_GetAllEmeralds(INT32 choice);
static void M_DestroyRobots(INT32 choice);
static void M_LevelSelectWarp(INT32 choice);
static void M_Credits(INT32 choice);
static void M_PandorasBox(INT32 choice);
static void M_EmblemHints(INT32 choice);
menu_t SR_MainDef, SR_UnlockChecklistDef;

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
menu_t SP_MainDef, MP_MainDef, OP_MainDef;
menu_t MISC_ScrambleTeamDef, MISC_ChangeTeamDef;

// Single Player
static void M_LoadGame(INT32 choice);
static void M_TimeAttack(INT32 choice);
static void M_NightsAttack(INT32 choice);
static void M_Statistics(INT32 choice);
static void M_ReplayTimeAttack(INT32 choice);
static void M_ChooseTimeAttack(INT32 choice);
static void M_ChooseNightsAttack(INT32 choice);
static void M_ModeAttackRetry(INT32 choice);
static void M_ModeAttackEndGame(INT32 choice);
static void M_SetGuestReplay(INT32 choice);
static void M_ChoosePlayer(INT32 choice);
menu_t SP_GameStatsDef, SP_LevelStatsDef;
static menu_t SP_TimeAttackDef, SP_ReplayDef, SP_GuestReplayDef, SP_GhostDef;
static menu_t SP_NightsAttackDef, SP_NightsReplayDef, SP_NightsGuestReplayDef, SP_NightsGhostDef;

// Multiplayer
#ifndef NONET
static void M_StartServerMenu(INT32 choice);
static void M_ConnectMenu(INT32 choice);
static void M_ConnectIPMenu(INT32 choice);
#endif
static void M_StartSplitServerMenu(INT32 choice);
static void M_StartServer(INT32 choice);
#ifndef NONET
static void M_Refresh(INT32 choice);
static void M_Connect(INT32 choice);
static void M_ChooseRoom(INT32 choice);
#endif
static void M_SetupMultiPlayer(INT32 choice);
static void M_SetupMultiPlayer2(INT32 choice);

// Options
// Split into multiple parts due to size
// Controls
menu_t OP_ControlsDef, OP_ControlListDef, OP_MoveControlsDef;
menu_t OP_MPControlsDef, OP_CameraControlsDef, OP_MiscControlsDef;
menu_t OP_P1ControlsDef, OP_P2ControlsDef, OP_MouseOptionsDef;
menu_t OP_Mouse2OptionsDef, OP_Joystick1Def, OP_Joystick2Def;
static void M_VideoModeMenu(INT32 choice);
static void M_Setup1PControlsMenu(INT32 choice);
static void M_Setup2PControlsMenu(INT32 choice);
static void M_Setup1PJoystickMenu(INT32 choice);
static void M_Setup2PJoystickMenu(INT32 choice);
static void M_AssignJoystick(INT32 choice);
static void M_ChangeControl(INT32 choice);

// Video & Sound
menu_t OP_VideoOptionsDef, OP_VideoModeDef;
#ifdef HWRENDER
menu_t OP_OpenGLOptionsDef, OP_OpenGLFogDef, OP_OpenGLColorDef;
#endif
menu_t OP_SoundOptionsDef;
static void M_ToggleSFX(void);
static void M_ToggleDigital(void);
static void M_ToggleMIDI(void);

//Misc
menu_t OP_DataOptionsDef, OP_ScreenshotOptionsDef, OP_EraseDataDef;
menu_t OP_GameOptionsDef, OP_ServerOptionsDef;
menu_t OP_NetgameOptionsDef, OP_GametypeOptionsDef;
menu_t OP_MonitorToggleDef;
static void M_ScreenshotOptions(INT32 choice);
static void M_EraseData(INT32 choice);

// Drawing functions
static void M_DrawGenericMenu(void);
static void M_DrawCenteredMenu(void);
static void M_DrawSkyRoom(void);
static void M_DrawChecklist(void);
static void M_DrawEmblemHints(void);
static void M_DrawPauseMenu(void);
static void M_DrawServerMenu(void);
static void M_DrawLevelSelectMenu(void);
static void M_DrawImageDef(void);
static void M_DrawLoad(void);
static void M_DrawLevelStats(void);
static void M_DrawGameStats(void);
static void M_DrawTimeAttackMenu(void);
static void M_DrawNightsAttackMenu(void);
static void M_DrawSetupChoosePlayerMenu(void);
static void M_DrawControl(void);
static void M_DrawVideoMode(void);
static void M_DrawMonitorToggles(void);
#ifdef HWRENDER
static void M_OGL_DrawFogMenu(void);
static void M_OGL_DrawColorMenu(void);
#endif
#ifndef NONET
static void M_DrawConnectMenu(void);
static void M_DrawConnectIPMenu(void);
static void M_DrawRoomMenu(void);
#endif
static void M_DrawJoystick(void);
static void M_DrawSetupMultiPlayerMenu(void);

// Handling functions
#ifndef NONET
static boolean M_CancelConnect(void);
#endif
static boolean M_ExitPandorasBox(void);
static boolean M_QuitMultiPlayerMenu(void);
static void M_HandleSoundTest(INT32 choice);
static void M_HandleImageDef(INT32 choice);
static void M_HandleLoadSave(INT32 choice);
static void M_HandleGameStats(INT32 choice);
static void M_HandleLevelStats(INT32 choice);
#ifndef NONET
static void M_HandleConnectIP(INT32 choice);
#endif
static void M_HandleSetupMultiPlayer(INT32 choice);
#ifdef HWRENDER
static void M_HandleFogColor(INT32 choice);
#endif
static void M_HandleVideoMode(INT32 choice);

// Consvar onchange functions
static void Nextmap_OnChange(void);
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
// When you add gametypes here, don't forget to update them in CV_AddValue!
CV_PossibleValue_t gametype_cons_t[] =
{
	{GT_COOP, "Co-op"},

	{GT_COMPETITION, "Competition"},
	{GT_RACE, "Race"},

	{GT_MATCH, "Match"},
	{GT_TEAMMATCH, "Team Match"},

	{GT_TAG, "Tag"},
	{GT_HIDEANDSEEK, "Hide and Seek"},

	{GT_CTF, "CTF"},
	{0, NULL}
};
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
	{IT_CALL   |IT_STRING, NULL, "Secrets",     M_SecretsMenu,      84},
	{IT_CALL   |IT_STRING, NULL, "1  player",   M_SinglePlayerMenu, 92},
	{IT_SUBMENU|IT_STRING, NULL, "multiplayer", &MP_MainDef,       100},
	{IT_CALL   |IT_STRING, NULL, "options",     M_Options,         108},
	{IT_CALL   |IT_STRING, NULL, "quit  game",  M_QuitSRB2,        116},
};

typedef enum
{
	secrets = 0,
	singleplr,
	multiplr,
	options,
	quitdoom
} main_e;

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
	{IT_STRING  | IT_SUBMENU, NULL, "Scramble Teams...", &MISC_ScrambleTeamDef, 16},
	{IT_STRING  | IT_CALL,    NULL, "Switch Map..."    , M_MapChange,           24},

	{IT_CALL | IT_STRING,    NULL, "Continue",             M_SelectableClearMenus,40},
	{IT_CALL | IT_STRING,    NULL, "Player 1 Setup",       M_SetupMultiPlayer,    48}, // splitscreen
	{IT_CALL | IT_STRING,    NULL, "Player 2 Setup",       M_SetupMultiPlayer2,   56}, // splitscreen

	{IT_STRING | IT_CALL,    NULL, "Spectate",             M_ConfirmSpectate,     48},
	{IT_STRING | IT_CALL,    NULL, "Enter Game",           M_ConfirmEnterGame,    48},
	{IT_STRING | IT_SUBMENU, NULL, "Switch Team...",       &MISC_ChangeTeamDef,   48},
	{IT_CALL | IT_STRING,    NULL, "Player Setup",         M_SetupMultiPlayer,    56}, // alone
	{IT_CALL | IT_STRING,    NULL, "Options",              M_Options,             64},

	{IT_CALL | IT_STRING,    NULL, "Return to Title",      M_EndGame,            80},
	{IT_CALL | IT_STRING,    NULL, "Quit Game",            M_QuitSRB2,           88},
};

typedef enum
{
	mpause_scramble = 0,
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

static menuitem_t MISC_ChangeLevelMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",             &cv_newgametype,    30},
	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},
	{IT_WHITESTRING|IT_CALL,         NULL, "Change Level",          M_ChangeLevel,     120},
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
	{IT_STRING | IT_CVAR, NULL, "Rings",              &cv_dummyrings,      20},
	{IT_STRING | IT_CVAR, NULL, "Lives",              &cv_dummylives,      30},
	{IT_STRING | IT_CVAR, NULL, "Continues",          &cv_dummycontinues,  40},

	{IT_STRING | IT_CVAR, NULL, "Gravity",            &cv_gravity,         60},
	{IT_STRING | IT_CVAR, NULL, "Throw Rings",        &cv_ringslinger,     70},

	{IT_STRING | IT_CALL, NULL, "Get All Emeralds",   M_GetAllEmeralds,    90},
	{IT_STRING | IT_CALL, NULL, "Destroy All Robots", M_DestroyRobots,    100},

	{IT_STRING | IT_CALL, NULL, "Ultimate Cheat",     M_UltimateCheat,    130},
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
	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_LevelSelectWarp,     120},
};

static menuitem_t SR_UnlockChecklistMenu[] =
{
	{IT_SUBMENU | IT_STRING,         NULL, "NEXT", &SR_MainDef, 192},
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
	{IT_CALL | IT_STRING,                       NULL, "Start Game",    M_LoadGame,        92},
	{IT_SECRET,                                 NULL, "Record Attack", M_TimeAttack,     100},
	{IT_SECRET,                                 NULL, "NiGHTS Mode",   M_NightsAttack,   108},
	{IT_CALL | IT_STRING | IT_CALL_NOTMODIFIED, NULL, "Statistics",    M_Statistics,     116},
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
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLoadSave, '\0'},     // dummy menuitem for the control func
};

// Single Player Level Select
static menuitem_t SP_LevelSelectMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_LevelSelectWarp,     120},
};

// Single Player Time Attack
static menuitem_t SP_TimeAttackMenu[] =
{
	{IT_STRING|IT_CVAR,        NULL, "Level",      &cv_nextmap,          52},
	{IT_STRING|IT_CVAR,        NULL, "Player",     &cv_chooseskin,       62},

	{IT_DISABLED,              NULL, "Guest Option...", &SP_GuestReplayDef, 100},
	{IT_DISABLED,              NULL, "Replay...",     &SP_ReplayDef,        110},
	{IT_DISABLED,              NULL, "Ghosts...",     &SP_GhostDef,         120},
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

// Single Player Nights Attack
static menuitem_t SP_NightsAttackMenu[] =
{
	{IT_STRING|IT_CVAR,        NULL, "Level",            &cv_nextmap,          44},
	{IT_STRING|IT_CVAR,        NULL, "Show Records For", &cv_dummymares,       54},

	{IT_DISABLED,              NULL, "Guest Option...",  &SP_NightsGuestReplayDef,   108},
	{IT_DISABLED,              NULL, "Replay...",        &SP_NightsReplayDef,        118},
	{IT_DISABLED,              NULL, "Ghosts...",        &SP_NightsGhostDef,         128},
	{IT_WHITESTRING|IT_CALL|IT_CALL_NOTMODIFIED,   NULL, "Start",            M_ChooseNightsAttack, 138},
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
static menuitem_t SP_GameStatsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleGameStats, '\0'},     // dummy menuitem for the control func
};

static menuitem_t SP_LevelStatsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleLevelStats, '\0'},     // dummy menuitem for the control func
};

// A rare case.
// External files modify this menu, so we can't call it static.
// And I'm too lazy to go through and rename it everywhere. ARRGH!
menuitem_t PlayerMenu[32] =
{
	{IT_CALL, NULL, NULL, M_ChoosePlayer, 0},
	{IT_CALL, NULL, NULL, M_ChoosePlayer, 0},
	{IT_CALL, NULL, NULL, M_ChoosePlayer, 0},
	{IT_CALL, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0},
	{IT_DISABLED, NULL, NULL, M_ChoosePlayer, 0}
};

// -----------------------------------
// Multiplayer and all of its submenus
// -----------------------------------
// Prefix: MP_
static menuitem_t MP_MainMenu[] =
{
#ifndef NONET
	{IT_CALL | IT_STRING, NULL, "HOST GAME",              M_StartServerMenu,      10},
	{IT_CALL | IT_STRING, NULL, "JOIN GAME (Search)",	  M_ConnectMenu,		  30},
	{IT_CALL | IT_STRING, NULL, "JOIN GAME (Specify IP)", M_ConnectIPMenu,        40},
#endif
	{IT_CALL | IT_STRING, NULL, "TWO PLAYER GAME",        M_StartSplitServerMenu, 60},

	{IT_CALL | IT_STRING, NULL, "SETUP PLAYER 1",         M_SetupMultiPlayer,     80},
	{IT_CALL | IT_STRING, NULL, "SETUP PLAYER 2",         M_SetupMultiPlayer2,    90},
};

static menuitem_t MP_ServerMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",             &cv_newgametype,    10},
#ifndef NONET
	{IT_STRING|IT_CALL,              NULL, "Room...",               M_RoomMenu,         20},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Server Name",           &cv_servername,     30},
#endif

	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        80},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_StartServer,     130},
};

enum
{
	mp_server_gametype = 0,
#ifndef NONET
	mp_server_room,
	mp_server_name,
#endif
	mp_server_level,
	mp_server_start
};

#ifndef NONET
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

static menuitem_t MP_ConnectIPMenu[] =
{
	{IT_KEYHANDLER | IT_STRING, NULL, "  IP Address:", M_HandleConnectIP, 0},
};
#endif

// Separated splitscreen and normal servers.
static menuitem_t MP_SplitServerMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",             &cv_newgametype,    10},
	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        80},
	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_StartServer,     130},
};

static menuitem_t MP_PlayerSetupMenu[] =
{
	{IT_KEYHANDLER | IT_STRING,   NULL, "Your name",   M_HandleSetupMultiPlayer,   0},
	{IT_KEYHANDLER | IT_STRING,   NULL, "Your color",  M_HandleSetupMultiPlayer,  16},
	{IT_KEYHANDLER | IT_STRING,   NULL, "Your player", M_HandleSetupMultiPlayer,  96}, // Tails 01-18-2001
};

// ------------------------------------
// Options and most (?) of its submenus
// ------------------------------------
// Prefix: OP_
static menuitem_t OP_MainMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Setup Controls...",     &OP_ControlsDef,      10},

	{IT_SUBMENU | IT_STRING, NULL, "Video Options...",      &OP_VideoOptionsDef,  30},
	{IT_SUBMENU | IT_STRING, NULL, "Sound Options...",      &OP_SoundOptionsDef,  40},
	{IT_SUBMENU | IT_STRING, NULL, "Data Options...",       &OP_DataOptionsDef,   50},

	{IT_SUBMENU | IT_STRING, NULL, "Game Options...",       &OP_GameOptionsDef,   70},
	{IT_SUBMENU | IT_STRING, NULL, "Server Options...",     &OP_ServerOptionsDef, 80},
};

static menuitem_t OP_ControlsMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Player 1 Controls...", &OP_P1ControlsDef,  10},
	{IT_SUBMENU | IT_STRING, NULL, "Player 2 Controls...", &OP_P2ControlsDef,  20},

	{IT_STRING  | IT_CVAR, NULL, "Controls per key", &cv_controlperkey, 40},
};

static menuitem_t OP_P1ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup1PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Mouse Options...", &OP_MouseOptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Joystick Options...", &OP_Joystick1Def  ,  30},

	{IT_STRING  | IT_CVAR, NULL, "Camera"  , &cv_chasecam  ,  50},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair , 60},

	{IT_STRING  | IT_CVAR, NULL, "Analog Control", &cv_useranalog,  80},
};

static menuitem_t OP_P2ControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup2PControlsMenu,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Second Mouse Options...", &OP_Mouse2OptionsDef, 20},
	{IT_SUBMENU | IT_STRING, NULL, "Second Joystick Options...", &OP_Joystick2Def  ,  30},

	{IT_STRING  | IT_CVAR, NULL, "Camera"  , &cv_chasecam2 , 50},
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair2, 60},

	{IT_STRING  | IT_CVAR, NULL, "Analog Control", &cv_useranalog2,  80},
};

static menuitem_t OP_ControlListMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Movement Controls...",      &OP_MoveControlsDef,   10},
	{IT_SUBMENU | IT_STRING, NULL, "Multiplayer Controls...",   &OP_MPControlsDef,     20},
	{IT_SUBMENU | IT_STRING, NULL, "Camera Controls...",        &OP_CameraControlsDef, 30},
	{IT_SUBMENU | IT_STRING, NULL, "Miscellaneous Controls...", &OP_MiscControlsDef,   40},
};

static menuitem_t OP_MoveControlsMenu[] =
{
	{IT_CALL | IT_STRING2, NULL, "Forward",      M_ChangeControl, gc_forward    },
	{IT_CALL | IT_STRING2, NULL, "Reverse",      M_ChangeControl, gc_backward   },
	{IT_CALL | IT_STRING2, NULL, "Turn Left",    M_ChangeControl, gc_turnleft   },
	{IT_CALL | IT_STRING2, NULL, "Turn Right",   M_ChangeControl, gc_turnright  },
	{IT_CALL | IT_STRING2, NULL, "Jump",         M_ChangeControl, gc_jump       },
	{IT_CALL | IT_STRING2, NULL, "Spin",         M_ChangeControl, gc_use        },
	{IT_CALL | IT_STRING2, NULL, "Strafe Left",  M_ChangeControl, gc_strafeleft },
	{IT_CALL | IT_STRING2, NULL, "Strafe Right", M_ChangeControl, gc_straferight},
};

static menuitem_t OP_MPControlsMenu[] =
{
	{IT_CALL | IT_STRING2, NULL, "Talk key",         M_ChangeControl, gc_talkkey      },
	{IT_CALL | IT_STRING2, NULL, "Team-Talk key",    M_ChangeControl, gc_teamkey      },
	{IT_CALL | IT_STRING2, NULL, "Rankings/Scores",  M_ChangeControl, gc_scores       },
	{IT_CALL | IT_STRING2, NULL, "Toss Flag",        M_ChangeControl, gc_tossflag     },
	{IT_CALL | IT_STRING2, NULL, "Next Weapon",      M_ChangeControl, gc_weaponnext   },
	{IT_CALL | IT_STRING2, NULL, "Prev Weapon",      M_ChangeControl, gc_weaponprev   },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 1",    M_ChangeControl, gc_wepslot1     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 2",    M_ChangeControl, gc_wepslot2     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 3",    M_ChangeControl, gc_wepslot3     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 4",    M_ChangeControl, gc_wepslot4     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 5",    M_ChangeControl, gc_wepslot5     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 6",    M_ChangeControl, gc_wepslot6     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 7",    M_ChangeControl, gc_wepslot7     },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss",        M_ChangeControl, gc_fire         },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss Normal", M_ChangeControl, gc_firenormal   },
};

static menuitem_t OP_CameraControlsMenu[] =
{
	{IT_CALL | IT_STRING2, NULL, "Look Up",          M_ChangeControl, gc_lookup       },
	{IT_CALL | IT_STRING2, NULL, "Look Down",        M_ChangeControl, gc_lookdown     },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera L",  M_ChangeControl, gc_camleft      },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera R",  M_ChangeControl, gc_camright     },
	{IT_CALL | IT_STRING2, NULL, "Center View",      M_ChangeControl, gc_centerview   },
	{IT_CALL | IT_STRING2, NULL, "Mouselook",        M_ChangeControl, gc_mouseaiming  },
	{IT_CALL | IT_STRING2, NULL, "Reset Camera",     M_ChangeControl, gc_camreset     },
	{IT_CALL | IT_STRING2, NULL, "Toggle Chasecam",  M_ChangeControl, gc_camtoggle    },
};

static menuitem_t OP_MiscControlsMenu[] =
{
	{IT_CALL | IT_STRING2, NULL, "Custom Action 1",  M_ChangeControl, gc_custom1      },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 2",  M_ChangeControl, gc_custom2      },
	{IT_CALL | IT_STRING2, NULL, "Custom Action 3",  M_ChangeControl, gc_custom3      },

	{IT_CALL | IT_STRING2, NULL, "Pause",            M_ChangeControl, gc_pause        },
	{IT_CALL | IT_STRING2, NULL, "Console",          M_ChangeControl, gc_console      },
};

static menuitem_t OP_Joystick1Menu[] =
{
	{IT_STRING | IT_CALL,  NULL, "Select Joystick...", M_Setup1PJoystickMenu,  10},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Turning"  , &cv_turnaxis         ,  30},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Moving"   , &cv_moveaxis         ,  40},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Strafe"   , &cv_sideaxis         ,  50},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Looking"  , &cv_lookaxis         ,  60},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Firing"   , &cv_fireaxis         ,  70},
	{IT_STRING | IT_CVAR,  NULL, "Axis For NFiring"  , &cv_firenaxis        ,  80},
};

static menuitem_t OP_Joystick2Menu[] =
{
	{IT_STRING | IT_CALL,  NULL, "Select Joystick...", M_Setup2PJoystickMenu, 10},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Turning"  , &cv_turnaxis2        , 30},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Moving"   , &cv_moveaxis2        , 40},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Strafe"   , &cv_sideaxis2        , 50},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Looking"  , &cv_lookaxis2        , 60},
	{IT_STRING | IT_CVAR,  NULL, "Axis For Firing"   , &cv_fireaxis2        , 70},
	{IT_STRING | IT_CVAR,  NULL, "Axis For NFiring"  , &cv_firenaxis2       , 80},
};

static menuitem_t OP_JoystickSetMenu[] =
{
	{IT_CALL | IT_NOTHING, "None", NULL, M_AssignJoystick, '0'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '1'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '2'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '3'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '4'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '5'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '6'},
};

static menuitem_t OP_MouseOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse",        &cv_usemouse,         10},


	{IT_STRING | IT_CVAR, NULL, "Always MouseLook", &cv_alwaysfreelook,   30},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove,        40},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse",     &cv_invertmouse,      50},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Speed",    &cv_mousesens,        60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Speed",    &cv_mouseysens,        70},
};

static menuitem_t OP_Mouse2OptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse 2",      &cv_usemouse2,        10},
	{IT_STRING | IT_CVAR, NULL, "Second Mouse Serial Port",
	                                                &cv_mouse2port,       20},
	{IT_STRING | IT_CVAR, NULL, "Always MouseLook", &cv_alwaysfreelook2,  30},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove2,       40},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse",     &cv_invertmouse2,     50},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse X Speed",    &cv_mousesens2,       60},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Y Speed",    &cv_mouseysens2,      70},
};

static menuitem_t OP_VideoOptionsMenu[] =
{
	{IT_STRING | IT_CALL,  NULL,   "Video Modes...",      M_VideoModeMenu,     10},

#ifdef HWRENDER
	{IT_SUBMENU|IT_STRING, NULL,   "3D Card Options...",  &OP_OpenGLOptionsDef,    20},
#endif

#if (defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON) || defined (HAVE_SDL)
	{IT_STRING|IT_CVAR,      NULL, "Fullscreen",          &cv_fullscreen,    30},
#endif

	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                         NULL, "Brightness",          &cv_usegamma,      50},
	{IT_STRING | IT_CVAR,    NULL, "Draw Distance",       &cv_drawdist, 60},
	{IT_STRING | IT_CVAR,    NULL, "NiGHTS Draw Dist",    &cv_drawdist_nights, 70},
	{IT_STRING | IT_CVAR,    NULL, "Precip Draw Dist",    &cv_drawdist_precip, 80},
	{IT_STRING | IT_CVAR,    NULL, "Precip Density",      &cv_precipdensity, 90},

	{IT_STRING | IT_CVAR,    NULL, "Show FPS",            &cv_ticrate,    110},
	{IT_STRING | IT_CVAR,    NULL, "Clear Before Redraw", &cv_homremoval, 120},
	{IT_STRING | IT_CVAR,    NULL, "Vertical Sync",       &cv_vidwait,    130},
};

static menuitem_t OP_VideoModeMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleVideoMode, '\0'},     // dummy menuitem for the control func
};

#ifdef HWRENDER
static menuitem_t OP_OpenGLOptionsMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Field of view",   &cv_grfov,            10},
	{IT_STRING|IT_CVAR,         NULL, "Quality",         &cv_scr_depth,        20},
	{IT_STRING|IT_CVAR,         NULL, "Texture Filter",  &cv_grfiltermode,     30},
	{IT_STRING|IT_CVAR,         NULL, "Anisotropic",     &cv_granisotropicmode,40},
#ifdef _WINDOWS
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
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Sound Volume" , &cv_soundvolume,     10},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Music Volume" , &cv_digmusicvolume,  20},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "MIDI Volume"  , &cv_midimusicvolume, 30},
#ifdef PC_DOS
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "CD Volume"    , &cd_volume,          40},
#endif

	{IT_STRING    | IT_CALL,  NULL,  "Toggle SFX"   , M_ToggleSFX,        50},
	{IT_STRING    | IT_CALL,  NULL,  "Toggle Digital Music", M_ToggleDigital,     60},
	{IT_STRING    | IT_CALL,  NULL,  "Toggle MIDI Music", M_ToggleMIDI,        70},
};

static menuitem_t OP_DataOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Screenshot Options...", M_ScreenshotOptions, 10},

	{IT_STRING | IT_SUBMENU, NULL, "Erase Data...", &OP_EraseDataDef, 30},
};

static menuitem_t OP_ScreenshotOptionsMenu[] =
{
	{IT_STRING|IT_CVAR, NULL, "Storage Location", &cv_screenshot_option, 10},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder", &cv_screenshot_folder, 20},

	{IT_HEADER, NULL, "Screenshots (F8)", NULL, 50},
	{IT_STRING|IT_CVAR, NULL, "Memory Level",      &cv_zlib_memory,      60},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", &cv_zlib_level,       70},
	{IT_STRING|IT_CVAR, NULL, "Strategy",          &cv_zlib_strategy,    80},
	{IT_STRING|IT_CVAR, NULL, "Window Size",       &cv_zlib_window_bits, 90},

	{IT_HEADER, NULL, "Movie Mode (F9)", NULL, 105},
	{IT_STRING|IT_CVAR, NULL, "Capture Mode", &cv_moviemode, 115},

	{IT_STRING|IT_CVAR, NULL, "Region Optimizing", &cv_gif_optimize,  125},
	{IT_STRING|IT_CVAR, NULL, "Downscaling",       &cv_gif_downscale, 135},

	{IT_STRING|IT_CVAR, NULL, "Memory Level",      &cv_zlib_memorya,      125},
	{IT_STRING|IT_CVAR, NULL, "Compression Level", &cv_zlib_levela,       135},
	{IT_STRING|IT_CVAR, NULL, "Strategy",          &cv_zlib_strategya,    145},
	{IT_STRING|IT_CVAR, NULL, "Window Size",       &cv_zlib_window_bitsa, 155},
};

enum
{
	op_screenshot_folder = 1,
	op_screenshot_capture = 8,
	op_screenshot_gif_start = 9,
	op_screenshot_gif_end = 10,
	op_screenshot_apng_start = 11,
	op_screenshot_apng_end = 14,
};

static menuitem_t OP_EraseDataMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Erase Record Data", M_EraseData, 10},
	{IT_STRING | IT_CALL, NULL, "Erase Secrets Data", M_EraseData, 20},

	{IT_STRING | IT_CALL, NULL, "\x85" "Erase ALL Data", M_EraseData, 40},
};

static menuitem_t OP_GameOptionsMenu[] =
{
#ifndef NONET
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                      NULL, "Master server",          &cv_masterserver,     10},
#endif
	{IT_STRING | IT_CVAR, NULL, "Show HUD",               &cv_showhud,     40},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "HUD Visibility",         &cv_translucenthud, 50},
	{IT_STRING | IT_CVAR, NULL, "Timer Display",          &cv_timetic,     60},
#ifdef SEENAMES
	{IT_STRING | IT_CVAR, NULL, "HUD Player Names",       &cv_seenames,    80},
#endif
	{IT_STRING | IT_CVAR, NULL, "Log Hazard Damage",      &cv_hazardlog,   90},

	{IT_STRING | IT_CVAR, NULL, "Console Back Color",     &cons_backcolor, 100},
	{IT_STRING | IT_CVAR, NULL, "Console Text Size",      &cv_constextsize,110},
	{IT_STRING | IT_CVAR, NULL, "Uppercase Console",      &cv_allcaps,     120},

	{IT_STRING | IT_CVAR, NULL, "Title Screen Demos",     &cv_rollingdemos, 140},
};

static menuitem_t OP_ServerOptionsMenu[] =
{
	{IT_STRING | IT_SUBMENU, NULL, "General netgame options...",  &OP_NetgameOptionsDef,  10},
	{IT_STRING | IT_SUBMENU, NULL, "Gametype options...",         &OP_GametypeOptionsDef, 20},
	{IT_STRING | IT_SUBMENU, NULL, "Random Monitor Toggles...",   &OP_MonitorToggleDef,   30},

#ifndef NONET
	{IT_STRING | IT_CVAR | IT_CV_STRING,
	                         NULL, "Server name",                 &cv_servername,         50},
#endif

	{IT_STRING | IT_CVAR,    NULL, "Intermission Timer",          &cv_inttime,            80},
	{IT_STRING | IT_CVAR,    NULL, "Advance to next map",         &cv_advancemap,         90},

#ifndef NONET
	{IT_STRING | IT_CVAR,    NULL, "Max Players",                 &cv_maxplayers,        110},
	{IT_STRING | IT_CVAR,    NULL, "Allow players to join",       &cv_allownewplayer,    120},
	{IT_STRING | IT_CVAR,    NULL, "Allow WAD Downloading",       &cv_downloading,       130},
	{IT_STRING | IT_CVAR,    NULL, "Attempts to Resynch",         &cv_resynchattempts,   140},
#endif
};

static menuitem_t OP_NetgameOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Time Limit",            &cv_timelimit,        10},
	{IT_STRING | IT_CVAR, NULL, "Point Limit",           &cv_pointlimit,       18},
	{IT_STRING | IT_CVAR, NULL, "Overtime Tie-Breaker",  &cv_overtime,         26},

	{IT_STRING | IT_CVAR, NULL, "Special Ring Weapons",  &cv_specialrings,     42},
	{IT_STRING | IT_CVAR, NULL, "Emeralds",              &cv_powerstones,      50},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",            &cv_matchboxes,       58},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn",          &cv_itemrespawn,      66},
	{IT_STRING | IT_CVAR, NULL, "Item Respawn time",     &cv_itemrespawntime,  74},

	{IT_STRING | IT_CVAR, NULL, "Sudden Death",          &cv_suddendeath,      90},
	{IT_STRING | IT_CVAR, NULL, "Player respawn delay",  &cv_respawntime,      98},

	{IT_STRING | IT_CVAR, NULL, "Force Skin #",          &cv_forceskin,          114},
	{IT_STRING | IT_CVAR, NULL, "Restrict skin changes", &cv_restrictskinchange, 122},

	{IT_STRING | IT_CVAR, NULL, "Autobalance Teams",            &cv_autobalance,      138},
	{IT_STRING | IT_CVAR, NULL, "Scramble Teams on Map Change", &cv_scrambleonchange, 146},
};

static menuitem_t OP_GametypeOptionsMenu[] =
{
	{IT_HEADER,           NULL, "CO-OP",                 NULL,                  2},
	{IT_STRING | IT_CVAR, NULL, "Players for exit",      &cv_playersforexit,   10},
	{IT_STRING | IT_CVAR, NULL, "Starting Lives",        &cv_startinglives,    18},

	{IT_HEADER,           NULL, "COMPETITION",           NULL,                 34},
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",            &cv_competitionboxes, 42},
	{IT_STRING | IT_CVAR, NULL, "Countdown Time",        &cv_countdowntime,    50},

	{IT_HEADER,           NULL, "RACE",                  NULL,                 66},
	{IT_STRING | IT_CVAR, NULL, "Number of Laps",        &cv_numlaps,          74},
	{IT_STRING | IT_CVAR, NULL, "Use Map Lap Counts",    &cv_usemapnumlaps,    82},

	{IT_HEADER,           NULL, "MATCH",                 NULL,                 98},
	{IT_STRING | IT_CVAR, NULL, "Scoring Type",          &cv_match_scoring,   106},

	{IT_HEADER,           NULL, "TAG",                   NULL,                122},
	{IT_STRING | IT_CVAR, NULL, "Hide Time",             &cv_hidetime,        130},

	{IT_HEADER,           NULL, "CTF",                   NULL,                146},
	{IT_STRING | IT_CVAR, NULL, "Flag Respawn Time",     &cv_flagtime,        154},
};

static menuitem_t OP_MonitorToggleMenu[] =
{
	// Printing handled by drawing function
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Recycler",          &cv_recycler,      20},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Teleporters",       &cv_teleporters,   30},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Super Ring",        &cv_superring,     40},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Super Sneakers",    &cv_supersneakers, 50},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Invincibility",     &cv_invincibility, 60},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Jump Shield",       &cv_jumpshield,    70},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Elemental Shield",  &cv_watershield,   80},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Attraction Shield", &cv_ringshield,    90},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Force Shield",      &cv_forceshield,  100},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Armageddon Shield", &cv_bombshield,   110},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "1 Up",              &cv_1up,          120},
	{IT_STRING|IT_CVAR|IT_CV_INVISSLIDER, NULL, "Eggman Box",        &cv_eggmanbox,    130},
};

// ==========================================================================
// ALL MENU DEFINITIONS GO HERE
// ==========================================================================

// Main Menu and related
menu_t MainDef = CENTERMENUSTYLE(NULL, MainMenu, NULL, 72);

menu_t MAPauseDef = PAUSEMENUSTYLE(MAPauseMenu, 40, 72);
menu_t SPauseDef = PAUSEMENUSTYLE(SPauseMenu, 40, 72);
menu_t MPauseDef = PAUSEMENUSTYLE(MPauseMenu, 40, 72);

// Misc Main Menu
menu_t MISC_ScrambleTeamDef = DEFAULTMENUSTYLE(NULL, MISC_ScrambleTeamMenu, &MPauseDef, 27, 40);
menu_t MISC_ChangeTeamDef = DEFAULTMENUSTYLE(NULL, MISC_ChangeTeamMenu, &MPauseDef, 27, 40);
menu_t MISC_ChangeLevelDef = MAPICONMENUSTYLE(NULL, MISC_ChangeLevelMenu, &MPauseDef);
menu_t MISC_HelpDef = IMAGEDEF(MISC_HelpMenu);

// Sky Room
menu_t SR_PandoraDef =
{
	"M_PANDRA",
	sizeof (SR_PandorasBox)/sizeof (menuitem_t),
	&SPauseDef,
	SR_PandorasBox,
	M_DrawGenericMenu,
	60, 40,
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
menu_t SR_LevelSelectDef =
{
	0,
	sizeof (SR_LevelSelectMenu)/sizeof (menuitem_t),
	&SR_MainDef,
	SR_LevelSelectMenu,
	M_DrawLevelSelectMenu,
	40, 40,
	0,
	NULL
};
menu_t SR_UnlockChecklistDef =
{
	NULL,
	1,
	&SR_MainDef,
	SR_UnlockChecklistMenu,
	M_DrawChecklist,
	280, 185,
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
menu_t SP_LevelSelectDef = MAPICONMENUSTYLE(NULL, SP_LevelSelectMenu, &SP_LoadDef);

menu_t SP_GameStatsDef =
{
	"M_STATS",
	1,
	&SP_MainDef,
	SP_GameStatsMenu,
	M_DrawGameStats,
	280, 185,
	0,
	NULL
};
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
	sizeof (PlayerMenu)/sizeof (menuitem_t),//player_end,
	&SP_MainDef,
	PlayerMenu,
	M_DrawSetupChoosePlayerMenu,
	24, 32,
	0,
	NULL
};

// Multiplayer
menu_t MP_MainDef = DEFAULTMENUSTYLE("M_MULTI", MP_MainMenu, &MainDef, 60, 40);
menu_t MP_ServerDef = MAPICONMENUSTYLE("M_MULTI", MP_ServerMenu, &MP_MainDef);
#ifndef NONET
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
menu_t MP_ConnectIPDef =
{
	"M_MULTI",
	sizeof (MP_ConnectIPMenu)/sizeof (menuitem_t),
	&MP_MainDef,
	MP_ConnectIPMenu,
	M_DrawConnectIPMenu,
	27,40,
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
menu_t MP_SplitServerDef = MAPICONMENUSTYLE("M_MULTI", MP_SplitServerMenu, &MP_MainDef);
menu_t MP_PlayerSetupDef =
{
	"M_SPLAYR",
	sizeof (MP_PlayerSetupMenu)/sizeof (menuitem_t),
	&MP_MainDef,
	MP_PlayerSetupMenu,
	M_DrawSetupMultiPlayerMenu,
	27, 40,
	0,
	M_QuitMultiPlayerMenu
};

// Options
menu_t OP_MainDef = DEFAULTMENUSTYLE("M_OPTTTL", OP_MainMenu, &MainDef, 60, 30);
menu_t OP_ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_ControlsMenu, &OP_MainDef, 60, 30);
menu_t OP_ControlListDef = DEFAULTMENUSTYLE("M_CONTRO", OP_ControlListMenu, &OP_ControlsDef, 60, 30);
menu_t OP_MoveControlsDef = CONTROLMENUSTYLE(OP_MoveControlsMenu, &OP_ControlListDef);
menu_t OP_MPControlsDef = CONTROLMENUSTYLE(OP_MPControlsMenu, &OP_ControlListDef);
menu_t OP_CameraControlsDef = CONTROLMENUSTYLE(OP_CameraControlsMenu, &OP_ControlListDef);
menu_t OP_MiscControlsDef = CONTROLMENUSTYLE(OP_MiscControlsMenu, &OP_ControlListDef);
menu_t OP_P1ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_P1ControlsMenu, &OP_ControlsDef, 60, 30);
menu_t OP_P2ControlsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_P2ControlsMenu, &OP_ControlsDef, 60, 30);
menu_t OP_MouseOptionsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_MouseOptionsMenu, &OP_P1ControlsDef, 60, 30);
menu_t OP_Mouse2OptionsDef = DEFAULTMENUSTYLE("M_CONTRO", OP_Mouse2OptionsMenu, &OP_P2ControlsDef, 60, 30);
menu_t OP_Joystick1Def = DEFAULTMENUSTYLE("M_CONTRO", OP_Joystick1Menu, &OP_P1ControlsDef, 60, 30);
menu_t OP_Joystick2Def = DEFAULTMENUSTYLE("M_CONTRO", OP_Joystick2Menu, &OP_P2ControlsDef, 60, 30);
menu_t OP_JoystickSetDef =
{
	"M_CONTRO",
	sizeof (OP_JoystickSetMenu)/sizeof (menuitem_t),
	&OP_Joystick1Def,
	OP_JoystickSetMenu,
	M_DrawJoystick,
	50, 40,
	0,
	NULL
};

menu_t OP_VideoOptionsDef = DEFAULTMENUSTYLE("M_VIDEO", OP_VideoOptionsMenu, &OP_MainDef, 60, 30);
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
menu_t OP_SoundOptionsDef = DEFAULTMENUSTYLE("M_SOUND", OP_SoundOptionsMenu, &OP_MainDef, 60, 30);
menu_t OP_GameOptionsDef = DEFAULTMENUSTYLE("M_GAME", OP_GameOptionsMenu, &OP_MainDef, 30, 30);
menu_t OP_ServerOptionsDef = DEFAULTMENUSTYLE("M_SERVER", OP_ServerOptionsMenu, &OP_MainDef, 30, 30);

menu_t OP_NetgameOptionsDef = DEFAULTMENUSTYLE("M_SERVER", OP_NetgameOptionsMenu, &OP_ServerOptionsDef, 30, 30);
menu_t OP_GametypeOptionsDef = DEFAULTMENUSTYLE("M_SERVER", OP_GametypeOptionsMenu, &OP_ServerOptionsDef, 30, 30);
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
menu_t OP_ScreenshotOptionsDef = DEFAULTMENUSTYLE("M_DATA", OP_ScreenshotOptionsMenu, &OP_DataOptionsDef, 30, 30);
menu_t OP_EraseDataDef = DEFAULTMENUSTYLE("M_DATA", OP_EraseDataMenu, &OP_DataOptionsDef, 60, 30);

// ==========================================================================
// CVAR ONCHANGE EVENTS GO HERE
// ==========================================================================
// (there's only a couple anyway)

// Prototypes
static INT32 M_FindFirstMap(INT32 gtype);
static INT32 M_GetFirstLevelInList(void);

// Nextmap.  Used for Time Attack.
static void Nextmap_OnChange(void)
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
		sprintf(tabase,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s",srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.string);
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

		if ((cv_newgametype.value == GT_COOP && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_COOP)) ||
			(cv_newgametype.value == GT_COMPETITION && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_COMPETITION)) ||
			(cv_newgametype.value == GT_RACE && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_RACE)) ||
			((cv_newgametype.value == GT_MATCH || cv_newgametype.value == GT_TEAMMATCH) && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_MATCH)) ||
			((cv_newgametype.value == GT_TAG || cv_newgametype.value == GT_HIDEANDSEEK) && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_TAG)) ||
			(cv_newgametype.value == GT_CTF && !(mapheaderinfo[cv_nextmap.value-1]->typeoflevel & TOL_CTF)))
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

			CV_SetValue(&cv_nextmap, M_FindFirstMap(value));
			CV_AddValue(&cv_nextmap, -1);
			CV_AddValue(&cv_nextmap, 1);
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

// ==========================================================================
// END ORGANIZATION STUFF.
// ==========================================================================

// current menudef
menu_t *currentMenu = &MainDef;

// =========================================================================
// BASIC MENU HANDLING
// =========================================================================

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
	char buf[255];
	size_t len;

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

#ifndef DC
			case KEY_F5: // Video Mode
				if (modeattacking)
					return true;
				M_StartControlPanel();
				M_Options(0);
				M_VideoModeMenu(0);
				return true;
#endif

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
				CV_AddValue(&cv_usegamma, 1);
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
			if (shiftdown && ch >= 32 && ch <= 127)
				ch = shiftxform[ch];
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
			if (currentMenu == &SP_PlayerDef)
			{
				Z_Free(char_notes);
				char_notes = NULL;
			}
			return true;

		case KEY_UPARROW:
			M_PrevOpt();
			S_StartSound(NULL, sfx_menu1);
			if (currentMenu == &SP_PlayerDef)
			{
				Z_Free(char_notes);
				char_notes = NULL;
			}
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				if (currentMenu != &OP_SoundOptionsDef)
					S_StartSound(NULL, sfx_menu1);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				if (currentMenu != &OP_SoundOptionsDef)
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
						S_StartSound(NULL, sfx_menu1);
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

				if (currentMenu == &SP_TimeAttackDef || currentMenu == &SP_NightsAttackDef)
				{
					// D_StartTitle does its own wipe, since GS_TIMEATTACK is now a complete gamestate.
					menuactive = false;
					D_StartTitle();
				}
				else
					M_SetupNextMenu(currentMenu->prevMenu);
			}
			else
				M_ClearMenus(true);

			return true;

		case KEY_BACKSPACE:
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].alphaKey);
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
			if (G_GametypeHasTeams())
				MPauseMenu[mpause_scramble].status = IT_STRING | IT_SUBMENU;
		}

		if (splitscreen)
			MPauseMenu[mpause_psetupsplit].status = MPauseMenu[mpause_psetupsplit2].status = IT_STRING | IT_CALL;
		else
		{
			MPauseMenu[mpause_psetup].status = IT_STRING | IT_CALL;

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

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

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
		OP_VideoOptionsMenu[1].status = IT_DISABLED;
#endif

#ifndef NONET
	CV_RegisterVar(&cv_serversort);
#endif

	//todo put this somewhere better...
	CV_RegisterVar(&cv_allcaps);
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
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv)
{
	INT32 i;
	INT32 range;
	patch_t *p;

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	range = ((cv->value - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

	if (range < 0)
		range = 0;
	if (range > 100)
		range = 100;

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	V_DrawScaledPatch(x - 8, y, 0, W_CachePatchName("M_SLIDEL", PU_CACHE));

	p =  W_CachePatchName("M_SLIDEM", PU_CACHE);
	for (i = 0; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	p = W_CachePatchName("M_SLIDER", PU_CACHE);
	V_DrawScaledPatch(x+SLIDER_RANGE*8, y, 0, p);

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_CACHE);
	V_DrawMappedPatch(x + ((SLIDER_RANGE-1)*8*range)/100, y, 0, p, yellowmap);
}

//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines)
{
	// Solid color textbox.
	V_DrawFill(x+5, y+5, width*8+6, boxlines*8+6, 239);
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
								M_DrawSlider(x, y, cv);
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
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string, 0), y,
									((cv->flags & CV_CHEAT) && !CV_IsSetToDefault(cv) ? V_REDMAP : V_YELLOWMAP), cv->string);
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

				V_DrawString(x-16, y, V_YELLOWMAP, currentMenu->menuitems[i].text);
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
								M_DrawSlider(x, y, cv);
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
		if (skins[j].name[0] != '\0')
		{
			skins_cons_t[j].strvalue = skins[j].name;
			skins_cons_t[j].value = j+1;
		}
		else
		{
			skins_cons_t[j].strvalue = NULL;
			skins_cons_t[j].value = 0;
		}
	}

	CV_SetValue(&cv_chooseskin, cv_chooseskin.value); // This causes crash sometimes?!

	CV_SetValue(&cv_chooseskin, 1);
	CV_AddValue(&cv_chooseskin, -1);
	CV_AddValue(&cv_chooseskin, 1);

	return;
}

// Call before showing any level-select menus
static void M_PrepareLevelSelect(void)
{
	if (levellistmode != LLM_CREATESERVER)
		CV_SetValue(&cv_nextmap, M_GetFirstLevelInList());
	else
		Newgametype_OnChange(); // Make sure to start on an appropriate map if wads have been added
}

//
// M_CanShowLevelInList
//
// Determines whether to show a given map in the various level-select lists.
// Set gt = -1 to ignore gametype.
//
boolean M_CanShowLevelInList(INT32 mapnum, INT32 gt)
{
	// Does the map exist?
	if (!mapheaderinfo[mapnum])
		return false;

	// Does the map have a name?
	if (!mapheaderinfo[mapnum]->lvlttl[0])
		return false;

	switch (levellistmode)
	{
		case LLM_CREATESERVER:
			// Should the map be hidden?
			if (mapheaderinfo[mapnum]->menuflags & LF2_HIDEINMENU)
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

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

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			return true;
		case LLM_RECORDATTACK:
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_RECORDATTACK))
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			if (mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
				return true;

			if (!mapvisited[mapnum])
				return false;

			return true;
		case LLM_NIGHTSATTACK:
			if (!(mapheaderinfo[mapnum]->menuflags & LF2_NIGHTSATTACK))
				return false;

			if (M_MapLocked(mapnum+1))
				return false; // not unlocked

			if (mapheaderinfo[mapnum]->menuflags & LF2_NOVISITNEEDED)
				return true;

			if (!mapvisited[mapnum])
				return false;

			return true;
	}

	// Hmm? Couldn't decide?
	return false;
}

static INT32 M_CountLevelsToShowInList(void)
{
	INT32 mapnum, count = 0;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, -1))
			count++;

	return count;
}

static INT32 M_GetFirstLevelInList(void)
{
	INT32 mapnum;

	for (mapnum = 0; mapnum < NUMMAPS; mapnum++)
		if (M_CanShowLevelInList(mapnum, -1))
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

static void M_PandorasBox(INT32 choice)
{
	(void)choice;
	CV_StealthSetValue(&cv_dummyrings, max(players[consoleplayer].health - 1, 0));
	CV_StealthSetValue(&cv_dummylives, players[consoleplayer].lives);
	CV_StealthSetValue(&cv_dummycontinues, players[consoleplayer].continues);
	M_SetupNextMenu(&SR_PandoraDef);
}

static boolean M_ExitPandorasBox(void)
{
	if (cv_dummyrings.value != max(players[consoleplayer].health - 1, 0))
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
	OP_MainMenu[5].status = (Playing() && !(server || adminplayer == consoleplayer)) ? (IT_GRAYEDOUT) : (IT_STRING|IT_SUBMENU);

	// if the player is playing _at all_, disable the erase data options
	OP_DataOptionsMenu[1].status = (Playing()) ? (IT_GRAYEDOUT) : (IT_STRING|IT_SUBMENU);

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

static void M_GetAllEmeralds(INT32 choice)
{
	(void)choice;

	emeralds = ((EMERALD7)*2)-1;
	M_StartMessage(M_GetText("You now have all 7 emeralds.\nUse them wisely.\nWith great power comes great ring drain.\n"),NULL,MM_NOTHING);

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
		cursaveslot = -1;
		M_SetupChoosePlayer(0);
	}
}

// ========
// SKY ROOM
// ========

UINT8 skyRoomMenuTranslations[MAXUNLOCKABLES];

#define NUMCHECKLIST 8
static void M_DrawChecklist(void)
{
	INT32 i, j = 0;

	for (i = 0; i < MAXUNLOCKABLES; i++)
	{
		if (unlockables[i].name[0] == 0 || unlockables[i].nochecklist
		|| !unlockables[i].conditionset || unlockables[i].conditionset > MAXCONDITIONSETS)
			continue;

		V_DrawString(8, 8+(24*j), V_RETURN8, unlockables[i].name);
		V_DrawString(160, 8+(24*j), V_RETURN8, V_WordWrap(160, 292, 0, unlockables[i].objective));

		if (unlockables[i].unlocked)
			V_DrawString(308, 8+(24*j), V_YELLOWMAP, "Y");
		else
			V_DrawString(308, 8+(24*j), V_YELLOWMAP, "N");

		if (++j >= NUMCHECKLIST)
			break;
	}
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

static void M_DrawLevelSelectMenu(void)
{
	M_DrawGenericMenu();

	if (cv_nextmap.value)
	{
		lumpnum_t lumpnum;
		patch_t *PictureOfLevel;

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

		V_DrawSmallScaledPatch(200, 110, 0, PictureOfLevel);
	}
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
	cursaveslot = -2;
	M_ClearMenus(true);
	F_StartCredits();
}

static void M_CustomLevelSelect(INT32 choice)
{
	INT32 ul = skyRoomMenuTranslations[choice-1];

	SR_LevelSelectDef.prevMenu = currentMenu;
	levellistmode = LLM_LEVELSELECT;
	maplistoption = (UINT8)(unlockables[ul].variable);
	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_PrepareLevelSelect();
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
	levellistmode = LLM_LEVELSELECT;
	maplistoption = 1;
	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No selectable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	SP_LevelSelectDef.prevMenu = currentMenu;

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SP_LevelSelectDef);
}

// ==============
// LOAD GAME MENU
// ==============

static INT32 saveSlotSelected = 0;
static short menumovedir = 0;

static void M_DrawLoadGameData(void)
{
	INT32 ecks;
	INT32 i;

	ecks = SP_LoadDef.x + 24;
	M_DrawTextBox(SP_LoadDef.x-12,144, 24, 4);

	if (saveSlotSelected == NOSAVESLOT) // last slot is play without saving
	{
		if (ultimate_selectable)
		{
			V_DrawCenteredString(ecks + 68, 144, V_ORANGEMAP, "ULTIMATE MODE");
			V_DrawCenteredString(ecks + 68, 156, 0, "NO RINGS, NO ONE-UPS,");
			V_DrawCenteredString(ecks + 68, 164, 0, "NO CONTINUES, ONE LIFE,");
			V_DrawCenteredString(ecks + 68, 172, 0, "FINAL DESTINATION.");
		}
		else
		{
			V_DrawCenteredString(ecks + 68, 144, V_ORANGEMAP, "PLAY WITHOUT SAVING");
			V_DrawCenteredString(ecks + 68, 156, 0, "THIS GAME WILL NOT BE");
			V_DrawCenteredString(ecks + 68, 164, 0, "SAVED, BUT YOU CAN STILL");
			V_DrawCenteredString(ecks + 68, 172, 0, "GET EMBLEMS AND SECRETS.");
		}
		return;
	}

	if (savegameinfo[saveSlotSelected].lives == -42) // Empty
	{
		V_DrawCenteredString(ecks + 68, 160, 0, "NO DATA");
		return;
	}

	if (savegameinfo[saveSlotSelected].lives == -666) // savegame is bad
	{
		V_DrawCenteredString(ecks + 68, 144, V_REDMAP, "CORRUPT SAVE FILE");
		V_DrawCenteredString(ecks + 68, 156, 0, "THIS SAVE FILE");
		V_DrawCenteredString(ecks + 68, 164, 0, "CAN NOT BE LOADED.");
		V_DrawCenteredString(ecks + 68, 172, 0, "DELETE USING BACKSPACE.");
		return;
	}

	// Draw the back sprite, it looks ugly if we don't
	V_DrawScaledPatch(SP_LoadDef.x, 144+8, 0, livesback);
	if (savegameinfo[saveSlotSelected].skincolor == 0)
		V_DrawScaledPatch(SP_LoadDef.x,144+8,0,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].face, PU_CACHE));
	else
	{
		UINT8 *colormap = R_GetTranslationColormap(savegameinfo[saveSlotSelected].skinnum, savegameinfo[saveSlotSelected].skincolor, 0);
		V_DrawMappedPatch(SP_LoadDef.x,144+8,0,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].face, PU_CACHE), colormap);
	}

	V_DrawString(ecks + 12, 152, 0, savegameinfo[saveSlotSelected].playername);

#ifdef SAVEGAMES_OTHERVERSIONS
	if (savegameinfo[saveSlotSelected].gamemap & 16384)
		V_DrawCenteredString(ecks + 68, 144, V_REDMAP, "OUTDATED SAVE FILE!");
#endif

	if (savegameinfo[saveSlotSelected].gamemap & 8192)
		V_DrawString(ecks + 12, 160, V_GREENMAP, "CLEAR!");
	else
		V_DrawString(ecks + 12, 160, 0, va("%s", savegameinfo[saveSlotSelected].levelname));

	// Use the big face pic for lives, duh. :3
	V_DrawScaledPatch(ecks + 12, 175, 0, W_CachePatchName("STLIVEX", PU_HUDGFX));
	V_DrawTallNum(ecks + 40, 172, 0, savegameinfo[saveSlotSelected].lives);

	// Absolute ridiculousness, condensed into another function.
	V_DrawContinueIcon(ecks + 58, 182, 0, savegameinfo[saveSlotSelected].skinnum, savegameinfo[saveSlotSelected].skincolor);
	V_DrawScaledPatch(ecks + 68, 175, 0, W_CachePatchName("STLIVEX", PU_HUDGFX));
	V_DrawTallNum(ecks + 96, 172, 0, savegameinfo[saveSlotSelected].continues);

	for (i = 0; i < 7; ++i)
	{
		if (savegameinfo[saveSlotSelected].numemeralds & (1 << i))
			V_DrawScaledPatch(ecks + 104 + (i * 8), 172, 0, tinyemeraldpics[i]);
	}
}

#define LOADBARHEIGHT SP_LoadDef.y + (LINEHEIGHT * (j+1)) + ymod
#define CURSORHEIGHT  SP_LoadDef.y + (LINEHEIGHT*3) - 1
static void M_DrawLoad(void)
{
	INT32 i, j;
	INT32 ymod = 0, offset = 0;

	M_DrawMenuTitle();

	if (menumovedir != 0) //movement illusion
	{
		ymod = (-(LINEHEIGHT/4))*menumovedir;
		offset = ((menumovedir > 0) ? -1 : 1);
	}

	V_DrawCenteredString(BASEVIDWIDTH/2, 40, 0, "Press backspace to delete a save.");

	for (i = MAXSAVEGAMES + saveSlotSelected - 2 + offset, j = 0;i <= MAXSAVEGAMES + saveSlotSelected + 2 + offset; i++, j++)
	{
		if ((menumovedir < 0 && j == 4) || (menumovedir > 0 && j == 0))
			continue; //this helps give the illusion of movement

		M_DrawSaveLoadBorder(SP_LoadDef.x, LOADBARHEIGHT);

		if ((i%MAXSAVEGAMES) == NOSAVESLOT) // play without saving
		{
			if (ultimate_selectable)
				V_DrawCenteredString(SP_LoadDef.x+92, LOADBARHEIGHT - 1, V_ORANGEMAP, "ULTIMATE MODE");
			else
				V_DrawCenteredString(SP_LoadDef.x+92, LOADBARHEIGHT - 1, V_ORANGEMAP, "PLAY WITHOUT SAVING");
			continue;
		}

		if (savegameinfo[i%MAXSAVEGAMES].lives == -42)
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, V_TRANSLUCENT, "NO DATA");
		else if (savegameinfo[i%MAXSAVEGAMES].lives == -666)
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, V_REDMAP, "CORRUPT SAVE FILE");
		else if (savegameinfo[i%MAXSAVEGAMES].gamemap & 8192)
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, V_GREENMAP, "CLEAR!");
		else
			V_DrawString(SP_LoadDef.x-6, LOADBARHEIGHT - 1, 0, va("%s", savegameinfo[i%MAXSAVEGAMES].levelname));

		//Draw the save slot number on the right side
		V_DrawRightAlignedString(SP_LoadDef.x+192, LOADBARHEIGHT - 1, 0, va("%d",(i%MAXSAVEGAMES) + 1));
	}

	//Draw cursors on both sides.
	V_DrawScaledPatch( 32, CURSORHEIGHT, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
	V_DrawScaledPatch(274, CURSORHEIGHT, 0, W_CachePatchName("M_CURSOR", PU_CACHE));

	M_DrawLoadGameData();

	//finishing the movement illusion
	if (menumovedir)
		menumovedir += ((menumovedir > 0) ? 1 : -1);
	if (abs(menumovedir) > 3)
		menumovedir = 0;
}
#undef LOADBARHEIGHT
#undef CURSORHEIGHT

//
// User wants to load this game
//
static void M_LoadSelect(INT32 choice)
{
	(void)choice;

	if (saveSlotSelected == NOSAVESLOT) //last slot is play without saving
	{
		M_NewGame();
		cursaveslot = -1;
		return;
	}

	if (!FIL_ReadFileOK(va(savegamename, saveSlotSelected)))
	{
		// This slot is empty, so start a new game here.
		M_NewGame();
	}
	else if (savegameinfo[saveSlotSelected].gamemap & 8192) // Completed
		M_LoadGameLevelSelect(saveSlotSelected + 1);
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
#ifdef SAVEGAMES_OTHERVERSIONS
	boolean oldversion = false;
#endif

	sprintf(savename, savegamename, slot);

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
	if (strcmp((const char *)save_p, (const char *)vcheck))
	{
#ifdef SAVEGAMES_OTHERVERSIONS
		oldversion = true;
#else
		BADSAVE // Incompatible versions?
#endif
	}
	save_p += VERSIONSIZE;

	// dearchive all the modifications
	// P_UnArchiveMisc()

	CHECKPOS
	fake = READINT16(save_p);

	if (((fake-1) & 8191) >= NUMMAPS) BADSAVE

	if(!mapheaderinfo[(fake-1) & 8191])
	{
		savegameinfo[slot].levelname[0] = '\0';
		savegameinfo[slot].actnum = 0;
	}
	else
	{
		strcpy(savegameinfo[slot].levelname, mapheaderinfo[(fake-1) & 8191]->lvlttl);
		savegameinfo[slot].actnum = mapheaderinfo[(fake-1) & 8191]->actnum;
	}

#ifdef SAVEGAMES_OTHERVERSIONS
	if (oldversion)
	{
		if (fake == 24) //meh, let's count old Clear! saves too
			fake |= 8192;
		fake |= 16384; // marker for outdated version
	}
#endif
	savegameinfo[slot].gamemap = fake;

	CHECKPOS
	fake = READUINT16(save_p)-357; // emeralds

	savegameinfo[slot].numemeralds = (UINT8)fake;

	CHECKPOS
	READSTRINGN(save_p, temp, sizeof(temp)); // mod it belongs to

	if (strcmp(temp, timeattackfolder)) BADSAVE

	// P_UnArchivePlayer()
	CHECKPOS
	savegameinfo[slot].skincolor = READUINT8(save_p);
	CHECKPOS
	savegameinfo[slot].skinnum = READUINT8(save_p);

	CHECKPOS
	(void)READINT32(save_p); // Score

	CHECKPOS
	savegameinfo[slot].lives = READINT32(save_p); // lives
	CHECKPOS
	savegameinfo[slot].continues = READINT32(save_p); // continues

	if (fake & (1<<10))
	{
		CHECKPOS
		savegameinfo[slot].botskin = READUINT8(save_p);
		if (savegameinfo[slot].botskin-1 >= numskins)
			savegameinfo[slot].botskin = 0;
		CHECKPOS
		savegameinfo[slot].botcolor = READUINT8(save_p); // because why not.
	}
	else
		savegameinfo[slot].botskin = 0;

	if (savegameinfo[slot].botskin)
		snprintf(savegameinfo[slot].playername, 36, "%s & %s",
			skins[savegameinfo[slot].skinnum].realname,
			skins[savegameinfo[slot].botskin-1].realname);
	else
		strcpy(savegameinfo[slot].playername, skins[savegameinfo[slot].skinnum].realname);

	savegameinfo[slot].playername[31] = 0;

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
	UINT32 i;
	char name[256];

	for (i = 0; i < MAXSAVEGAMES; i++)
	{
		snprintf(name, sizeof name, savegamename, i);
		name[sizeof name - 1] = '\0';

		handle = fopen(name, "rb");
		if (handle == NULL)
		{
			savegameinfo[i].lives = -42;
			continue;
		}
		fclose(handle);
		M_ReadSavegameInfo(i);
	}
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

	// Refresh savegame menu info
	M_ReadSaveStrings();
}

static void M_HandleLoadSave(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			++saveSlotSelected;
			if (saveSlotSelected >= MAXSAVEGAMES)
				saveSlotSelected -= MAXSAVEGAMES;
			menumovedir = 1;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			--saveSlotSelected;
			if (saveSlotSelected < 0)
				saveSlotSelected += MAXSAVEGAMES;
			menumovedir = -1;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			if (savegameinfo[saveSlotSelected].lives != -666) // don't allow loading of "bad saves"
				M_LoadSelect(saveSlotSelected);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			S_StartSound(NULL, sfx_menu1);
			// Don't allow people to 'delete' "Play without Saving."
			// Nor allow people to 'delete' slots with no saves in them.
			if (saveSlotSelected != NOSAVESLOT && savegameinfo[saveSlotSelected].lives != -42)
				M_StartMessage(M_GetText("Are you sure you want to delete\nthis save game?\n\n(Press 'Y' to confirm)\n"),M_SaveGameDeleteResponse,MM_YESNO);
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
	// Already there? Out of bounds? Whatever, then!
	if (sslot == saveSlotSelected || sslot >= MAXSAVEGAMES)
		return;

	// Figure out whether to display up movement or down movement
	menumovedir = (saveSlotSelected - sslot) > 0 ? -1 : 1;
	if (abs(saveSlotSelected - sslot) > (MAXSAVEGAMES>>1))
		menumovedir *= -1;

	saveSlotSelected = sslot;
}

// ================
// CHARACTER SELECT
// ================

static void M_SetupChoosePlayer(INT32 choice)
{
	(void)choice;

	if (mapheaderinfo[startmap-1] && mapheaderinfo[startmap-1]->forcecharacter[0] != '\0')
	{
		M_ChoosePlayer(0); //oh for crying out loud just get STARTED, it doesn't matter!
		return;
	}

	if (Playing() == false)
	{
		S_StopMusic();
		S_ChangeMusicInternal("chrsel", true);
	}

	SP_PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&SP_PlayerDef);
	char_scroll = itemOn*128*FRACUNIT; // finish scrolling the menu
	Z_Free(char_notes);
	char_notes = NULL;
}

// Draw the choose player setup menu, had some fun with player anim
static void M_DrawSetupChoosePlayerMenu(void)
{
	const INT32 my = 24;
	patch_t *patch;
	INT32 i, o, j;
	char *picname;

	// Black BG
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
	//V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

	// Character select profile images!1
	M_DrawTextBox(0, my, 16, 20);

	if (abs(itemOn*128*FRACUNIT - char_scroll) > 256*FRACUNIT)
		char_scroll = itemOn*128*FRACUNIT;
	else if (itemOn*128*FRACUNIT - char_scroll > 128*FRACUNIT)
		char_scroll += 48*FRACUNIT;
	else if (itemOn*128*FRACUNIT - char_scroll < -128*FRACUNIT)
		char_scroll -= 48*FRACUNIT;
	else if (itemOn*128*FRACUNIT > char_scroll+16*FRACUNIT)
		char_scroll += 16*FRACUNIT;
	else if (itemOn*128*FRACUNIT < char_scroll-16*FRACUNIT)
		char_scroll -= 16*FRACUNIT;
	else // close enough.
		char_scroll = itemOn*128*FRACUNIT; // just be exact now.
	i = (char_scroll+16*FRACUNIT)/(128*FRACUNIT);
	o = ((char_scroll/FRACUNIT)+16)%128;

	// prev character
	if (i-1 >= 0 && PlayerMenu[i-1].status != IT_DISABLED
	&& o < 32)
	{
		picname = description[i-1].picname;
		if (picname[0] == '\0')
		{
			picname = strtok(Z_StrDup(description[i-1].skinname), "&");
			for (j = 0; j < numskins; j++)
				if (stricmp(skins[j].name, picname) == 0)
				{
					Z_Free(picname);
					picname = skins[j].charsel;
					break;
				}
			if (j == numskins) // AAAAAAAAAA
				picname = skins[0].charsel;
		}
		patch = W_CachePatchName(picname, PU_CACHE);
		if (SHORT(patch->width) >= 256)
			V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT/2, 0, patch, 0, SHORT(patch->height) - 64 + o*2, SHORT(patch->width), SHORT(patch->height));
		else
			V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT, 0, patch, 0, SHORT(patch->height) - 32 + o, SHORT(patch->width), SHORT(patch->height));
		W_UnlockCachedPatch(patch);
	}

	// next character
	if (i+1 < currentMenu->numitems && PlayerMenu[i+1].status != IT_DISABLED
	&& o < 128)
	{
		picname = description[i+1].picname;
		if (picname[0] == '\0')
		{
			picname = strtok(Z_StrDup(description[i+1].skinname), "&");
			for (j = 0; j < numskins; j++)
				if (stricmp(skins[j].name, picname) == 0)
				{
					Z_Free(picname);
					picname = skins[j].charsel;
					break;
				}
			if (j == numskins) // AAAAAAAAAA
				picname = skins[0].charsel;
		}
		patch = W_CachePatchName(picname, PU_CACHE);
		if (SHORT(patch->width) >= 256)
			V_DrawCroppedPatch(8<<FRACBITS, (my + 168 - o)<<FRACBITS, FRACUNIT/2, 0, patch, 0, 0, SHORT(patch->width), o*2);
		else
			V_DrawCroppedPatch(8<<FRACBITS, (my + 168 - o)<<FRACBITS, FRACUNIT, 0, patch, 0, 0, SHORT(patch->width), o);
		W_UnlockCachedPatch(patch);
	}

	// current character
	if (i < currentMenu->numitems && PlayerMenu[i].status != IT_DISABLED)
	{
		picname = description[i].picname;
		if (picname[0] == '\0')
		{
			picname = strtok(Z_StrDup(description[i].skinname), "&");
			for (j = 0; j < numskins; j++)
				if (stricmp(skins[j].name, picname) == 0)
				{
					Z_Free(picname);
					picname = skins[j].charsel;
					break;
				}
			if (j == numskins) // AAAAAAAAAA
				picname = skins[0].charsel;
		}
		patch = W_CachePatchName(picname, PU_CACHE);
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
				V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT/2, 0, patch, 0, (o - 32)*2, SHORT(patch->width), SHORT(patch->height));
			else
				V_DrawCroppedPatch(8<<FRACBITS, (my + 8)<<FRACBITS, FRACUNIT, 0, patch, 0, o - 32, SHORT(patch->width), SHORT(patch->height));
		}
		W_UnlockCachedPatch(patch);
	}

	// draw title (or big pic)
	M_DrawMenuTitle();

	// Character description
	M_DrawTextBox(136, my, 21, 20);
	if (!char_notes)
		char_notes = V_WordWrap(0, 21*8, V_ALLOWLOWERCASE, description[itemOn].notes);
	V_DrawString(146, my + 9, V_RETURN8|V_ALLOWLOWERCASE, char_notes);
}

// Chose the player you want to use Tails 03-02-2002
static void M_ChoosePlayer(INT32 choice)
{
	char *skin1,*skin2;
	INT32 skinnum;
	boolean ultmode = (ultimate_selectable && SP_PlayerDef.prevMenu == &SP_LoadDef && saveSlotSelected == NOSAVESLOT);

	// skip this if forcecharacter
	if (mapheaderinfo[startmap-1] && mapheaderinfo[startmap-1]->forcecharacter[0] == '\0')
	{
		// M_SetupChoosePlayer didn't call us directly, that means we've been properly set up.
		char_scroll = itemOn*128*FRACUNIT; // finish scrolling the menu
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
		cursaveslot = -1;

	lastmapsaved = 0;
	gamecomplete = false;

	G_DeferedInitNew(ultmode, G_BuildMapName(startmap), (UINT8)skinnum, false, fromlevelselect);
	COM_BufAddText("dummyconsvar 1\n"); // G_DeferedInitNew doesn't do this
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

		if (!mapvisited[i])
			continue;

		statsMapList[j++] = i;
	}
	statsMapList[j] = -1;
	statsMax = j - 13 + numextraemblems;
	statsLocation = 0;

	if (statsMax < 0)
		statsMax = 0;

	M_SetupNextMenu(&SP_GameStatsDef);
}

static void M_DrawStatsMaps(int location)
{
	INT32 y = 76, i = -1;
	INT16 mnum;
	extraemblem_t *exemblem;

	V_DrawString(20,  y-12, 0, "LEVEL NAME");
	V_DrawString(248, y-12, 0, "EMBLEMS");

	while (statsMapList[++i] != -1)
	{
		if (location)
		{
			--location;
			continue;
		}

		mnum = statsMapList[i];
		M_DrawMapEmblems(mnum+1, 292, y);

		if (mapheaderinfo[mnum]->actnum != 0)
			V_DrawString(20, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[mnum]->lvlttl, mapheaderinfo[mnum]->actnum));
		else
			V_DrawString(20, y, V_YELLOWMAP, mapheaderinfo[mnum]->lvlttl);

		y += 8;

		if (y >= BASEVIDHEIGHT-8)
			return;
	}

	// Extra Emblems
	for (i = -2; i < numextraemblems; ++i)
	{
		if (location)
		{
			--location;
			continue;
		}

		if (i == -1)
			V_DrawString(20, y, V_GREENMAP, "EXTRA EMBLEMS");
		else if (i >= 0)
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
			return;
	}
}

static void M_DrawLevelStats(void)
{
	M_DrawMenuTitle();
	V_DrawCenteredString(BASEVIDWIDTH/2, 24, V_YELLOWMAP, "PAGE 2 OF 2");

	V_DrawString(72, 48, 0, va("x %d/%d", M_CountEmblems(), numemblems+numextraemblems));
	V_DrawScaledPatch(40, 48-4, 0, W_CachePatchName("EMBLICON", PU_STATIC));

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

		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			statsLocation += (statsLocation+15 >= statsMax) ? statsMax-statsLocation : 15;
			break;

		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			statsLocation -= (statsLocation < 15) ? statsLocation : 15;
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			M_SetupNextMenu(&SP_GameStatsDef);
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

// Handle GAME statistics.
static void M_DrawGameStats(void)
{
	char beststr[40];

	tic_t besttime = 0;
	UINT32 bestscore = 0;
	UINT32 bestrings = 0;

	INT32 i;
	INT32 mapsunfinished[3] = {0, 0, 0};

	M_DrawMenuTitle();
	V_DrawCenteredString(BASEVIDWIDTH/2, 24, V_YELLOWMAP, "PAGE 1 OF 2");

	V_DrawString(32, 60, V_YELLOWMAP, "Total Play Time:");
	V_DrawRightAlignedString(BASEVIDWIDTH-32, 70, 0, va("%i hours, %i minutes, %i seconds",
	                         G_TicsToHours(totalplaytime),
	                         G_TicsToMinutes(totalplaytime, false),
	                         G_TicsToSeconds(totalplaytime)));

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;

		if (!mainrecords[i])
		{
			mapsunfinished[0]++;
			mapsunfinished[1]++;
			mapsunfinished[2]++;
			continue;
		}

		if (mainrecords[i]->score > 0)
			bestscore += mainrecords[i]->score;
		else
			mapsunfinished[0]++;

		if (mainrecords[i]->time > 0)
			besttime += mainrecords[i]->time;
		else
			mapsunfinished[1]++;

		if (mainrecords[i]->rings > 0)
			bestrings += mainrecords[i]->rings;
		else
			mapsunfinished[2]++;

	}

	V_DrawCenteredString(BASEVIDWIDTH/2, 90, 0, "* COMBINED RECORDS *");

	sprintf(beststr, "%u", bestscore);
	V_DrawString(32, 100, V_YELLOWMAP, "SCORE:");
	V_DrawRightAlignedString(BASEVIDWIDTH-32, 100, 0, beststr);
	if (mapsunfinished[0])
		V_DrawRightAlignedString(BASEVIDWIDTH-32, 108, V_REDMAP, va("(%d unfinished)", mapsunfinished[0]));

	sprintf(beststr, "%i:%02i:%02i.%02i", G_TicsToHours(besttime), G_TicsToMinutes(besttime, false), G_TicsToSeconds(besttime), G_TicsToCentiseconds(besttime));
	V_DrawString(32, 120, V_YELLOWMAP, "TIME:");
	V_DrawRightAlignedString(BASEVIDWIDTH-32, 120, 0, beststr);
	if (mapsunfinished[1])
		V_DrawRightAlignedString(BASEVIDWIDTH-32, 128, V_REDMAP, va("(%d unfinished)", mapsunfinished[1]));

	sprintf(beststr, "%u", bestrings);
	V_DrawString(32, 140, V_YELLOWMAP, "RINGS:");
	V_DrawRightAlignedString(BASEVIDWIDTH-32, 140, 0, beststr);
	if (mapsunfinished[2])
		V_DrawRightAlignedString(BASEVIDWIDTH-32, 148, V_REDMAP, va("(%d unfinished)", mapsunfinished[2]));
}

static void M_HandleGameStats(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	switch (choice)
	{
		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			M_SetupNextMenu(&SP_LevelStatsDef);
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
	patch_t *PictureOfLevel, *PictureOfUrFace;
	lumpnum_t lumpnum;
	char beststr[40];

	S_ChangeMusicInternal("racent", true); // Eww, but needed for when user hits escape during demo playback

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
		}
	}

	// DRAW THE SKULL CURSOR
	V_DrawScaledPatch(currentMenu->x - 24, cursory, 0, W_CachePatchName("M_CURSOR", PU_CACHE));
	V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch(208, 32, 0, PictureOfLevel);

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

		V_DrawCenteredString(104, 32, 0, "* LEVEL RECORDS *");

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->score)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%u", mainrecords[cv_nextmap.value-1]->score);

		V_DrawString(104-72, 48, V_YELLOWMAP, "SCORE:");
		V_DrawRightAlignedString(104+72, 48, V_ALLOWLOWERCASE, beststr);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->time)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(mainrecords[cv_nextmap.value-1]->time, true),
			                                 G_TicsToSeconds(mainrecords[cv_nextmap.value-1]->time),
			                                 G_TicsToCentiseconds(mainrecords[cv_nextmap.value-1]->time));

		V_DrawString(104-72, 58, V_YELLOWMAP, "TIME:");
		V_DrawRightAlignedString(104+72, 58, V_ALLOWLOWERCASE, beststr);

		if (!mainrecords[cv_nextmap.value-1] || !mainrecords[cv_nextmap.value-1]->rings)
			sprintf(beststr, "(none)");
		else
			sprintf(beststr, "%hu", mainrecords[cv_nextmap.value-1]->rings);

		V_DrawString(104-72, 68, V_YELLOWMAP, "RINGS:");
		V_DrawRightAlignedString(104+72, 68, V_ALLOWLOWERCASE, beststr);

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
				V_DrawSmallMappedPatch(104+76, yHeight, 0, W_CachePatchName(M_GetEmblemPatch(em), PU_CACHE),
				                       R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
			else
				V_DrawSmallScaledPatch(104+76, yHeight, 0, W_CachePatchName("NEEDIT", PU_CACHE));

			skipThisOne:
			em = M_GetLevelEmblems(-1);
		}
	}

	// ALWAYS DRAW level name and skin even when not on this menu!
	if (currentMenu != &SP_TimeAttackDef)
	{
		consvar_t *ncv;

		x = SP_TimeAttackDef.x;
		y = SP_TimeAttackDef.y;

		for (i = 0; i < 2; ++i)
		{
			ncv = (consvar_t *)SP_TimeAttackMenu[i].itemaction;

			V_DrawString(x, y + SP_TimeAttackMenu[i].alphaKey, V_TRANSLUCENT, SP_TimeAttackMenu[i].text);
			V_DrawString(BASEVIDWIDTH - x - V_StringWidth(ncv->string, 0),
			             y + SP_TimeAttackMenu[i].alphaKey, V_YELLOWMAP|V_TRANSLUCENT, ncv->string);
		}
	}
}

// Going to Time Attack menu...
static void M_TimeAttack(INT32 choice)
{
	(void)choice;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	levellistmode = LLM_RECORDATTACK; // Don't be dependent on cv_newgametype

	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No record-attackable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	M_PatchSkinNameTable();

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SP_TimeAttackDef);
	Nextmap_OnChange();

	itemOn = tastart; // "Start" is selected.

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusicInternal("racent", true);
}

// Drawing function for Nights Attack
void M_DrawNightsAttackMenu(void)
{
	patch_t *PictureOfLevel;
	lumpnum_t lumpnum;
	char beststr[40];

	S_ChangeMusicInternal("racent", true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

	// draw menu (everything else goes on top of it)
	M_DrawGenericMenu();

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch(90, 28, 0, PictureOfLevel);

	// Level record list
	if (cv_nextmap.value)
	{
		emblem_t *em;
		INT32 yHeight;

		UINT8 bestoverall	= G_GetBestNightsGrade(cv_nextmap.value, 0);
		UINT8 bestgrade		= G_GetBestNightsGrade(cv_nextmap.value, cv_dummymares.value);
		UINT32 bestscore	= G_GetBestNightsScore(cv_nextmap.value, cv_dummymares.value);
		tic_t besttime		= G_GetBestNightsTime(cv_nextmap.value, cv_dummymares.value);

		if (P_HasGrades(cv_nextmap.value, 0))
			V_DrawScaledPatch(200, 28 + 8, 0, ngradeletters[bestoverall]);

		if (currentMenu == &SP_NightsAttackDef)
		{
			if (P_HasGrades(cv_nextmap.value, cv_dummymares.value))
			{
				V_DrawString(160-88, 112, V_YELLOWMAP, "BEST GRADE:");
				V_DrawSmallScaledPatch(160 + 86 - (ngradeletters[bestgrade]->width/2),
					112 + 8 - (ngradeletters[bestgrade]->height/2),
					0, ngradeletters[bestgrade]);
			}

			if (!bestscore)
				sprintf(beststr, "(none)");
			else
				sprintf(beststr, "%u", bestscore);

			V_DrawString(160 - 88, 122, V_YELLOWMAP, "BEST SCORE:");
			V_DrawRightAlignedString(160 + 88, 122, V_ALLOWLOWERCASE, beststr);

			if (besttime == UINT32_MAX)
				sprintf(beststr, "(none)");
			else
				sprintf(beststr, "%i:%02i.%02i", G_TicsToMinutes(besttime, true),
																				 G_TicsToSeconds(besttime),
																				 G_TicsToCentiseconds(besttime));

			V_DrawString(160-88, 132, V_YELLOWMAP, "BEST TIME:");
			V_DrawRightAlignedString(160+88, 132, V_ALLOWLOWERCASE, beststr);

			if (cv_dummymares.value == 0) {
				// Draw record emblems.
				em = M_GetLevelEmblems(cv_nextmap.value);
				while (em)
				{
					switch (em->type)
					{
						case ET_NGRADE: yHeight = 112; break;
						case ET_NTIME:  yHeight = 132; break;
						default:
							goto skipThisOne;
					}

					if (em->collected)
						V_DrawSmallMappedPatch(160+88, yHeight, 0, W_CachePatchName(M_GetEmblemPatch(em), PU_CACHE),
																	 R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(em), GTC_CACHE));
					else
						V_DrawSmallScaledPatch(160+88, yHeight, 0, W_CachePatchName("NEEDIT", PU_CACHE));

					skipThisOne:
					em = M_GetLevelEmblems(-1);
				}
			}
		}
		// ALWAYS DRAW level name even when not on this menu!
		else
		{
			consvar_t *ncv;
			INT32 x = SP_NightsAttackDef.x;
			INT32 y = SP_NightsAttackDef.y;

			ncv = (consvar_t *)SP_NightsAttackMenu[0].itemaction;
			V_DrawString(x, y + SP_NightsAttackMenu[0].alphaKey, V_TRANSLUCENT, SP_NightsAttackMenu[0].text);
			V_DrawString(BASEVIDWIDTH - x - V_StringWidth(ncv->string, 0),
									 y + SP_NightsAttackMenu[0].alphaKey, V_YELLOWMAP|V_TRANSLUCENT, ncv->string);
		}
	}
}

// Going to Nights Attack menu...
static void M_NightsAttack(INT32 choice)
{
	(void)choice;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	levellistmode = LLM_NIGHTSATTACK; // Don't be dependent on cv_newgametype

	if (M_CountLevelsToShowInList() == 0)
	{
		M_StartMessage(M_GetText("No NiGHTS-attackable levels found.\n"),NULL,MM_NOTHING);
		return;
	}

	// This is really just to make sure Sonic is the played character, just in case
	M_PatchSkinNameTable();

	M_PrepareLevelSelect();
	M_SetupNextMenu(&SP_NightsAttackDef);
	Nextmap_OnChange();

	itemOn = nastart; // "Start" is selected.

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusicInternal("racent", true);
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
	snprintf(nameofdemo, sizeof nameofdemo, "%s-%s-last", gpath, cv_chooseskin.string);

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
		G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.string, which));
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
	CV_AddValue(&cv_nextmap, -1);
	CV_AddValue(&cv_nextmap, 1);
	M_StartMessage(M_GetText("Guest replay data erased.\n"),NULL,MM_NOTHING);
}

static void M_OverwriteGuest(const char *which, boolean nights)
{
	char *rguest = Z_StrDup(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-guest.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value)));
	UINT8 *buf;
	size_t len;
	if (!nights)
		len = FIL_ReadFile(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%s-%s.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.string, which), &buf);
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
	CV_AddValue(&cv_nextmap, -1);
	CV_AddValue(&cv_nextmap, 1);
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
	S_ChangeMusicInternal("racent", true);
	// Update replay availability.
	CV_AddValue(&cv_nextmap, 1);
	CV_AddValue(&cv_nextmap, -1);
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
	UINT16 i, j;
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
		for (j = 0; gametype_cons_t[j].strvalue; j++)
		{
			if (gametype_cons_t[j].value == serverlist[slindex].info.gametype)
				gt = gametype_cons_t[j].strvalue;
		}

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

//
// FindFirstMap
//
// Finds the first map of a particular gametype
// Defaults to 1 if nothing found.
//
static INT32 M_FindFirstMap(INT32 gtype)
{
	INT32 i;

	for (i = 0; i < NUMMAPS; i++)
	{
		if (mapheaderinfo[i] && (mapheaderinfo[i]->typeoflevel & gtype))
			return i + 1;
	}

	return 1;
}

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
	lumpnum_t lumpnum;
	patch_t *PictureOfLevel;

	M_DrawGenericMenu();

#ifndef NONET
	// Room name
	if (currentMenu == &MP_ServerDef)
	{
		if (ms_RoomId < 0)
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ServerMenu[mp_server_room].alphaKey,
			                         V_YELLOWMAP, (itemOn == mp_server_room) ? "<Select to change>" : "<Offline Mode>");
		else
			V_DrawRightAlignedString(BASEVIDWIDTH - currentMenu->x, currentMenu->y + MP_ServerMenu[mp_server_room].alphaKey,
			                         V_YELLOWMAP, room_list[menuRoomIndex].name);
	}
#endif

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch((BASEVIDWIDTH*3/4)-(SHORT(PictureOfLevel->width)/4), ((BASEVIDHEIGHT*3/4)-(SHORT(PictureOfLevel->height)/4)+10), 0, PictureOfLevel);
}

static void M_MapChange(INT32 choice)
{
	(void)choice;

	levellistmode = LLM_CREATESERVER;

	CV_SetValue(&cv_newgametype, gametype);
	CV_SetValue(&cv_nextmap, gamemap);

	M_PrepareLevelSelect();
	M_SetupNextMenu(&MISC_ChangeLevelDef);
}

static void M_StartSplitServerMenu(INT32 choice)
{
	(void)choice;
	levellistmode = LLM_CREATESERVER;
	M_PrepareLevelSelect();
	M_SetupNextMenu(&MP_SplitServerDef);
}

#ifndef NONET
static void M_StartServerMenu(INT32 choice)
{
	(void)choice;
	levellistmode = LLM_CREATESERVER;
	M_PrepareLevelSelect();
	ms_RoomId = -1;
	M_SetupNextMenu(&MP_ServerDef);

}

// ==============
// CONNECT VIA IP
// ==============

static char setupm_ip[16];

// Connect using IP address Tails 11-19-2002
static void M_ConnectIPMenu(INT32 choice)
{
	(void)choice;
	// modified game check: no longer handled
	// we don't request a restart unless the filelist differs

	M_SetupNextMenu(&MP_ConnectIPDef);
}

// Draw the funky Connect IP menu. Tails 11-19-2002
// So much work for such a little thing!
static void M_DrawConnectIPMenu(void)
{
	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// draw name string
	V_DrawString(128,40, V_MONOSPACE, setupm_ip);

	// draw text cursor for name
	if (itemOn == 0 &&
	    skullAnimCounter < 4)   //blink cursor
		V_DrawCharacter(128+V_StringWidth(setupm_ip, V_MONOSPACE),40,'_',false);
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
	size_t   l;
	boolean  exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			M_ClearMenus(true);
			M_ConnectIP(1);
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			if ((l = strlen(setupm_ip))!=0 && itemOn == 0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l-1] =0;
			}
			break;

		default:
			l = strlen(setupm_ip);
			if (l < 16-1 && (choice == 46 || (choice >= 48 && choice <= 57))) // Rudimentary number and period enforcing
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] =(char)choice;
				setupm_ip[l+1] =0;
			}
			else if (l < 16-1 && choice >= 199 && choice <= 211 && choice != 202 && choice != 206) //numpad too!
			{
				XBOXSTATIC char keypad_translation[] = {'7','8','9','-','4','5','6','+','1','2','3','0','.'};
				choice = keypad_translation[choice - 199];
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_ip[l] =(char)choice;
				setupm_ip[l+1] =0;
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

#define PLBOXW    8
#define PLBOXH    9

static INT32      multi_tics;
static state_t   *multi_state;

// this is set before entering the MultiPlayer setup menu,
// for either player 1 or 2
static char       setupm_name[MAXPLAYERNAME+1];
static player_t  *setupm_player;
static consvar_t *setupm_cvskin;
static consvar_t *setupm_cvcolor;
static consvar_t *setupm_cvname;
static INT32      setupm_fakeskin;
static INT32      setupm_fakecolor;

static void M_DrawSetupMultiPlayerMenu(void)
{
	INT32 mx, my, st, flags = 0;
	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	patch_t *patch;
	UINT8 frame;

	mx = MP_PlayerSetupDef.x;
	my = MP_PlayerSetupDef.y;

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// draw name string
	M_DrawTextBox(mx + 90, my - 8, MAXPLAYERNAME, 1);
	V_DrawString(mx + 98, my, V_ALLOWLOWERCASE, setupm_name);

	// draw skin string
	V_DrawString(mx + 90, my + 96,
	             ((MP_PlayerSetupMenu[2].status & IT_TYPE) == IT_SPACE ? V_TRANSLUCENT : 0)|V_YELLOWMAP|V_ALLOWLOWERCASE,
	             skins[setupm_fakeskin].realname);

	// draw the name of the color you have chosen
	// Just so people don't go thinking that "Default" is Green.
	V_DrawString(208, 72, V_YELLOWMAP|V_ALLOWLOWERCASE, Color_Names[setupm_fakecolor]);

	// draw text cursor for name
	if (!itemOn && skullAnimCounter < 4) // blink cursor
		V_DrawCharacter(mx + 98 + V_StringWidth(setupm_name, 0), my, '_',false);

	// anim the player in the box
	if (--multi_tics <= 0)
	{
		st = multi_state->nextstate;
		if (st != S_NULL)
			multi_state = &states[st];
		multi_tics = multi_state->tics;
		if (multi_tics == -1)
			multi_tics = 15;
	}

	// skin 0 is default player sprite
	if (R_SkinAvailable(skins[setupm_fakeskin].name) != -1)
		sprdef = &skins[R_SkinAvailable(skins[setupm_fakeskin].name)].spritedef;
	else
		sprdef = &skins[0].spritedef;

	if (!sprdef->numframes) // No frames ??
		return; // Can't render!

	frame = multi_state->frame & FF_FRAMEMASK;
	if (frame >= sprdef->numframes) // Walking animation missing
		frame = 0; // Try to use standing frame

	sprframe = &sprdef->spriteframes[frame];
	patch = W_CachePatchNum(sprframe->lumppat[0], PU_CACHE);
	if (sprframe->flip & 1) // Only for first sprite
		flags |= V_FLIP; // This sprite is left/right flipped!

	// draw box around guy
	M_DrawTextBox(mx + 90, my + 8, PLBOXW, PLBOXH);

	// draw player sprite
	if (!setupm_fakecolor) // should never happen but hey, who knows
	{
		if (skins[setupm_fakeskin].flags & SF_HIRES)
		{
			V_DrawSciencePatch((mx+98+(PLBOXW*8/2))<<FRACBITS,
						(my+16+(PLBOXH*8)-12)<<FRACBITS,
						flags, patch,
						skins[setupm_fakeskin].highresscale);
		}
		else
			V_DrawScaledPatch(mx + 98 + (PLBOXW*8/2), my + 16 + (PLBOXH*8) - 12, flags, patch);
	}
	else
	{
		UINT8 *colormap = R_GetTranslationColormap(setupm_fakeskin, setupm_fakecolor, 0);

		if (skins[setupm_fakeskin].flags & SF_HIRES)
		{
			V_DrawFixedPatch((mx+98+(PLBOXW*8/2))<<FRACBITS,
						(my+16+(PLBOXH*8)-12)<<FRACBITS,
						skins[setupm_fakeskin].highresscale,
						flags, patch, colormap);
		}
		else
			V_DrawMappedPatch(mx + 98 + (PLBOXW*8/2), my + 16 + (PLBOXH*8) - 12, flags, patch, colormap);

		Z_Free(colormap);
	}
}

// Handle 1P/2P MP Setup
static void M_HandleSetupMultiPlayer(INT32 choice)
{
	size_t   l;
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
			if (itemOn == 2)       //player skin
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakeskin--;
			}
			else if (itemOn == 1) // player color
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakecolor--;
			}
			break;

		case KEY_RIGHTARROW:
			if (itemOn == 2)       //player skin
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakeskin++;
			}
			else if (itemOn == 1) // player color
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_fakecolor++;
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			if ((l = strlen(setupm_name))!=0 && itemOn == 0)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_name[l-1] =0;
			}
			break;

		default:
			if (choice < 32 || choice > 127 || itemOn != 0)
				break;
			l = strlen(setupm_name);
			if (l < MAXPLAYERNAME)
			{
				S_StartSound(NULL,sfx_menu1); // Tails
				setupm_name[l] =(char)choice;
				setupm_name[l+1] =0;
			}
			break;
	}

	// check skin
	if (setupm_fakeskin < 0)
		setupm_fakeskin = numskins-1;
	if (setupm_fakeskin > numskins-1)
		setupm_fakeskin = 0;

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

	multi_state = &states[mobjinfo[MT_PLAYER].seestate];
	multi_tics = multi_state->tics;
	strcpy(setupm_name, cv_playername.string);

	// set for player 1
	setupm_player = &players[consoleplayer];
	setupm_cvskin = &cv_skin;
	setupm_cvcolor = &cv_playercolor;
	setupm_cvname = &cv_playername;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;
	setupm_fakecolor = setupm_cvcolor->value;

	// disable skin changes if we can't actually change skins
	if (!CanChangeSkin(consoleplayer))
		MP_PlayerSetupMenu[2].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[2].status = (IT_KEYHANDLER|IT_STRING);

	MP_PlayerSetupDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MP_PlayerSetupDef);
}

// start the multiplayer setup menu, for secondary player (splitscreen mode)
static void M_SetupMultiPlayer2(INT32 choice)
{
	(void)choice;

	multi_state = &states[mobjinfo[MT_PLAYER].seestate];
	multi_tics = multi_state->tics;
	strcpy (setupm_name, cv_playername2.string);

	// set for splitscreen secondary player
	setupm_player = &players[secondarydisplayplayer];
	setupm_cvskin = &cv_skin2;
	setupm_cvcolor = &cv_playercolor2;
	setupm_cvname = &cv_playername2;

	// For whatever reason this doesn't work right if you just use ->value
	setupm_fakeskin = R_SkinAvailable(setupm_cvskin->string);
	if (setupm_fakeskin == -1)
		setupm_fakeskin = 0;
	setupm_fakecolor = setupm_cvcolor->value;

	// disable skin changes if we can't actually change skins
	if (splitscreen && !CanChangeSkin(secondarydisplayplayer))
		MP_PlayerSetupMenu[2].status = (IT_GRAYEDOUT);
	else
		MP_PlayerSetupMenu[2].status = (IT_KEYHANDLER | IT_STRING);

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

	M_DrawGenericMenu();

	for (i = 0;i < 8; i++)
	{
		M_DrawSaveLoadBorder(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i);

		if ((setupcontrols_secondaryplayer && (i == cv_usejoystick2.value))
			|| (!setupcontrols_secondaryplayer && (i == cv_usejoystick.value)))
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i,V_GREENMAP,joystickInfo[i]);
		else
			V_DrawString(OP_JoystickSetDef.x, OP_JoystickSetDef.y+LINEHEIGHT*i,0,joystickInfo[i]);
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

	// Unhide the three non-P2 controls
	OP_MPControlsMenu[0].status = IT_CALL|IT_STRING2;
	OP_MPControlsMenu[1].status = IT_CALL|IT_STRING2;
	OP_MPControlsMenu[2].status = IT_CALL|IT_STRING2;
	// Unide the pause/console controls too
	OP_MiscControlsMenu[3].status = IT_CALL|IT_STRING2;
	OP_MiscControlsMenu[4].status = IT_CALL|IT_STRING2;

	OP_ControlListDef.prevMenu = &OP_P1ControlsDef;
	M_SetupNextMenu(&OP_ControlListDef);
}

static void M_Setup2PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = true;
	setupcontrols = gamecontrolbis;
	currentMenu->lastOn = itemOn;

	// Hide the three non-P2 controls
	OP_MPControlsMenu[0].status = IT_GRAYEDOUT2;
	OP_MPControlsMenu[1].status = IT_GRAYEDOUT2;
	OP_MPControlsMenu[2].status = IT_GRAYEDOUT2;
	// Hide the pause/console controls too
	OP_MiscControlsMenu[3].status = IT_GRAYEDOUT2;
	OP_MiscControlsMenu[4].status = IT_GRAYEDOUT2;

	OP_ControlListDef.prevMenu = &OP_P2ControlsDef;
	M_SetupNextMenu(&OP_ControlListDef);
}

// Draws the Customise Controls menu
static void M_DrawControl(void)
{
	char     tmp[50];
	INT32    i;
	INT32    keys[2];

	// draw title, strings and submenu
	M_DrawGenericMenu();

	M_CentreText(30,
		 (setupcontrols_secondaryplayer ? "SET CONTROLS FOR SECONDARY PLAYER" :
		                                  "PRESS ENTER TO CHANGE, BACKSPACE TO CLEAR"));

	for (i = 0;i < currentMenu->numitems;i++)
	{
		if (currentMenu->menuitems[i].status != IT_CONTROL)
			continue;

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
		V_DrawRightAlignedString(BASEVIDWIDTH-currentMenu->x, currentMenu->y + i*8, V_YELLOWMAP, tmp);
	}
}

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

	}

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

// Toggles sound systems in-game.
static void M_ToggleSFX(void)
{
	if (nosound)
	{
		nosound = false;
		I_StartupSound();
		if (nosound) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		M_StartMessage(M_GetText("SFX Enabled\n"), NULL, MM_NOTHING);
	}
	else
	{
		if (sound_disabled)
		{
			sound_disabled = false;
			M_StartMessage(M_GetText("SFX Enabled\n"), NULL, MM_NOTHING);
		}
		else
		{
			sound_disabled = true;
			S_StopSounds();
			M_StartMessage(M_GetText("SFX Disabled\n"), NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleDigital(void)
{
	if (nodigimusic)
	{
		nodigimusic = false;
		I_InitDigMusic();
		if (nodigimusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_StopMusic();
		S_ChangeMusicInternal("lclear", false);
		M_StartMessage(M_GetText("Digital Music Enabled\n"), NULL, MM_NOTHING);
	}
	else
	{
		if (digital_disabled)
		{
			digital_disabled = false;
			M_StartMessage(M_GetText("Digital Music Enabled\n"), NULL, MM_NOTHING);
		}
		else
		{
			digital_disabled = true;
			S_StopMusic();
			M_StartMessage(M_GetText("Digital Music Disabled\n"), NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleMIDI(void)
{
	if (nomidimusic)
	{
		nomidimusic = false;
		I_InitMIDIMusic();
		if (nomidimusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_ChangeMusicInternal("lclear", false);
		M_StartMessage(M_GetText("MIDI Music Enabled\n"), NULL, MM_NOTHING);
	}
	else
	{
		if (music_disabled)
		{
			music_disabled = false;
			M_StartMessage(M_GetText("MIDI Music Enabled\n"), NULL, MM_NOTHING);
		}
		else
		{
			music_disabled = true;
			S_StopMusic();
			M_StartMessage(M_GetText("MIDI Music Disabled\n"), NULL, MM_NOTHING);
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
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 150,
			V_YELLOWMAP, "Other modes may have visual errors.");
		V_DrawCenteredString(BASEVIDWIDTH/2, OP_VideoModeDef.y + 158,
			V_YELLOWMAP, "Use at own risk.");
	}

	// Draw the cursor for the VidMode menu
	i = 41 - 10 + ((vidm_selected / vidm_column_size)*7*13);
	j = OP_VideoModeDef.y + 14 + ((vidm_selected % vidm_column_size)*8);

	V_DrawScaledPatch(i - 8, j, 0,
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
		cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
		sum += cv->value;

		if (!CV_IsSetToDefault(cv))
			cheating = true;
	}

	for (i = 0; i < currentMenu->numitems; ++i)
	{
		cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
		y = currentMenu->y + currentMenu->menuitems[i].alphaKey;

		M_DrawSlider(currentMenu->x + 20, y, cv);

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
			S_StartSound(NULL, sfx_menu1);
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
