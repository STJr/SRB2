// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.c
/// \brief host/client network commands
///        commands are executed through the command buffer
///	       like console commands, other miscellaneous commands (at the end)

#include "doomdef.h"

#include "console.h"
#include "command.h"
#include "i_system.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "g_input.h"
#include "m_menu.h"
#include "r_local.h"
#include "r_things.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "i_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "byteptr.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "v_video.h"
#include "d_main.h"
#include "m_random.h"
#include "f_finale.h"
#include "mserv.h"
#include "md5.h"
#include "z_zone.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "m_cond.h"
#include "m_anigif.h"

#ifdef NETGAME_DEVMODE
#define CV_RESTRICT CV_NETVAR
#else
#define CV_RESTRICT 0
#endif

// ------
// protos
// ------

static void Got_NameAndColor(UINT8 **cp, INT32 playernum);
static void Got_WeaponPref(UINT8 **cp, INT32 playernum);
static void Got_Mapcmd(UINT8 **cp, INT32 playernum);
static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum);
static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum);
#ifdef DELFILE
static void Got_Delfilecmd(UINT8 **cp, INT32 playernum);
#endif
static void Got_Addfilecmd(UINT8 **cp, INT32 playernum);
static void Got_Pause(UINT8 **cp, INT32 playernum);
static void Got_Suicide(UINT8 **cp, INT32 playernum);
static void Got_RandomSeed(UINT8 **cp, INT32 playernum);
static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum);
static void Got_Teamchange(UINT8 **cp, INT32 playernum);
static void Got_Clearscores(UINT8 **cp, INT32 playernum);

static void PointLimit_OnChange(void);
static void TimeLimit_OnChange(void);
static void NumLaps_OnChange(void);
static void Mute_OnChange(void);

static void Hidetime_OnChange(void);

static void AutoBalance_OnChange(void);
static void TeamScramble_OnChange(void);

static void NetTimeout_OnChange(void);
static void JoinTimeout_OnChange(void);

static void Ringslinger_OnChange(void);
static void Gravity_OnChange(void);
static void ForceSkin_OnChange(void);

static void Name_OnChange(void);
static void Name2_OnChange(void);
static void Skin_OnChange(void);
static void Skin2_OnChange(void);
static void Color_OnChange(void);
static void Color2_OnChange(void);
static void DummyConsvar_OnChange(void);
static void SoundTest_OnChange(void);

#ifdef NETGAME_DEVMODE
static void Fishcake_OnChange(void);
#endif

static void Command_Playdemo_f(void);
static void Command_Timedemo_f(void);
static void Command_Stopdemo_f(void);
static void Command_StartMovie_f(void);
static void Command_StopMovie_f(void);
static void Command_Map_f(void);
static void Command_ResetCamera_f(void);

static void Command_Addfile(void);
static void Command_ListWADS_f(void);
#ifdef DELFILE
static void Command_Delfile(void);
#endif
static void Command_RunSOC(void);
static void Command_Pause(void);
static void Command_Suicide(void);

static void Command_Version_f(void);
#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void);
#endif
static void Command_ShowGametype_f(void);
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void);
static void Command_Playintro_f(void);

static void Command_Displayplayer_f(void);
static void Command_Tunes_f(void);
static void Command_RestartAudio_f(void);

static void Command_ExitLevel_f(void);
static void Command_Showmap_f(void);
static void Command_Mapmd5_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_ServerTeamChange_f(void);

static void Command_Clearscores_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Login_f(void);
static void Got_Login(UINT8 **cp, INT32 playernum);
static void Got_Verification(UINT8 **cp, INT32 playernum);
static void Command_Verify_f(void);
static void Command_MotD_f(void);
static void Got_MotD_f(UINT8 **cp, INT32 playernum);

static void Command_ShowScores_f(void);
static void Command_ShowTime_f(void);

static void Command_Isgamemodified_f(void);
static void Command_Cheats_f(void);
#ifdef _DEBUG
static void Command_Togglemodified_f(void);
#ifdef HAVE_BLUA
static void Command_Archivetest_f(void);
#endif
#endif

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

void SendWeaponPref(void);
void SendWeaponPref2(void);

static CV_PossibleValue_t usemouse_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL}};
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
static CV_PossibleValue_t mouse2port_cons_t[] = {{0, "/dev/gpmdata"}, {1, "/dev/ttyS0"},
	{2, "/dev/ttyS1"}, {3, "/dev/ttyS2"}, {4, "/dev/ttyS3"}, {0, NULL}};
#else
static CV_PossibleValue_t mouse2port_cons_t[] = {{1, "COM1"}, {2, "COM2"}, {3, "COM3"}, {4, "COM4"},
	{0, NULL}};
#endif

#ifdef LJOYSTICK
static CV_PossibleValue_t joyport_cons_t[] = {{1, "/dev/js0"}, {2, "/dev/js1"}, {3, "/dev/js2"},
	{4, "/dev/js3"}, {0, NULL}};
#else
// accept whatever value - it is in fact the joystick device number
#define usejoystick_cons_t NULL
#endif

static CV_PossibleValue_t autobalance_cons_t[] = {{0, "MIN"}, {4, "MAX"}, {0, NULL}};
static CV_PossibleValue_t teamscramble_cons_t[] = {{0, "Off"}, {1, "Random"}, {2, "Points"}, {0, NULL}};

static CV_PossibleValue_t startingliveslimit_cons_t[] = {{1, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t sleeping_cons_t[] = {{-1, "MIN"}, {1000/TICRATE, "MAX"}, {0, NULL}};
static CV_PossibleValue_t competitionboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Teleports"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t matchboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Non-Random"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t chances_cons_t[] = {{0, "MIN"}, {9, "MAX"}, {0, NULL}};
static CV_PossibleValue_t match_scoring_cons_t[] = {{0, "Normal"}, {1, "Classic"}, {0, NULL}};
static CV_PossibleValue_t pause_cons_t[] = {{0, "Server"}, {1, "All"}, {0, NULL}};

static CV_PossibleValue_t timetic_cons_t[] = {{0, "Normal"}, {1, "Tics"}, {2, "Centiseconds"}, {0, NULL}};

#ifdef NETGAME_DEVMODE
static consvar_t cv_fishcake = {"fishcake", "Off", CV_CALL|CV_NOSHOWHELP|CV_RESTRICT, CV_OnOff, Fishcake_OnChange, 0, NULL, NULL, 0, 0, NULL};
#endif
static consvar_t cv_dummyconsvar = {"dummyconsvar", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff,
	DummyConsvar_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_restrictskinchange = {"restrictskinchange", "Yes", CV_NETVAR|CV_CHEAT, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowteamchange = {"allowteamchange", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_startinglives = {"startinglives", "3", CV_NETVAR|CV_CHEAT, startingliveslimit_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t respawntime_cons_t[] = {{0, "MIN"}, {30, "MAX"}, {0, NULL}};
consvar_t cv_respawntime = {"respawndelay", "3", CV_NETVAR|CV_CHEAT, respawntime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_competitionboxes = {"competitionboxes", "Random", CV_NETVAR|CV_CHEAT, competitionboxes_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

#ifdef SEENAMES
static CV_PossibleValue_t seenames_cons_t[] = {{0, "Off"}, {1, "Colorless"}, {2, "Team"}, {3, "Ally/Foe"}, {0, NULL}};
consvar_t cv_seenames = {"seenames", "Ally/Foe", CV_SAVE, seenames_cons_t, 0, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowseenames = {"allowseenames", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif

// these are just meant to be saved to the config
consvar_t cv_playername = {"name", "Sonic", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_playername2 = {"name2", "Tails", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name2_OnChange, 0, NULL, NULL, 0, 0, NULL};
// player colors
consvar_t cv_playercolor = {"color", "Blue", CV_CALL|CV_NOINIT, Color_cons_t, Color_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_playercolor2 = {"color2", "Orange", CV_CALL|CV_NOINIT, Color_cons_t, Color2_OnChange, 0, NULL, NULL, 0, 0, NULL};
// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin = {"skin", DEFAULTSKIN, CV_CALL|CV_NOINIT, NULL, Skin_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_skin2 = {"skin2", DEFAULTSKIN2, CV_CALL|CV_NOINIT, NULL, Skin2_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_skipmapcheck = {"skipmapcheck", "Off", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

INT32 cv_debug;

consvar_t cv_usemouse = {"use_mouse", "On", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usemouse2 = {"use_mouse2", "Off", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse2, 0, NULL, NULL, 0, 0, NULL};

#if defined (DC) || defined (_XBOX) || defined (WMINPUT) || defined (_WII) //joystick 1 and 2
consvar_t cv_usejoystick = {"use_joystick", "1", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "2", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#elif defined (PSP) || defined (GP2X) || defined (_NDS) //only one joystick
consvar_t cv_usejoystick = {"use_joystick", "1", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#else //all esle, no joystick
consvar_t cv_usejoystick = {"use_joystick", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usejoystick2 = {"use_joystick2", "0", CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2, 0, NULL, NULL, 0, 0, NULL};
#endif
#if (defined (LJOYSTICK) || defined (HAVE_SDL))
#ifdef LJOYSTICK
consvar_t cv_joyport = {"joyport", "/dev/js0", CV_SAVE, joyport_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_joyport2 = {"joyport2", "/dev/js0", CV_SAVE, joyport_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: for later
#endif
consvar_t cv_joyscale = {"joyscale", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_joyscale2 = {"joyscale2", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale2, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_joyscale = {"joyscale", "1", CV_SAVE|CV_HIDEN, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: Dummy for save
consvar_t cv_joyscale2 = {"joyscale2", "1", CV_SAVE|CV_HIDEN, NULL, NULL, 0, NULL, NULL, 0, 0, NULL}; //Alam: Dummy for save
#endif
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
consvar_t cv_mouse2port = {"mouse2port", "/dev/gpmdata", CV_SAVE, mouse2port_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mouse2opt = {"mouse2opt", "0", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
#else
consvar_t cv_mouse2port = {"mouse2port", "COM2", CV_SAVE, mouse2port_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif

consvar_t cv_matchboxes = {"matchboxes", "Normal", CV_NETVAR|CV_CHEAT, matchboxes_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_specialrings = {"specialrings", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_powerstones = {"powerstones", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_recycler =      {"tv_recycler",      "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_teleporters =   {"tv_teleporter",    "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_superring =     {"tv_superring",     "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_supersneakers = {"tv_supersneaker",  "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_invincibility = {"tv_invincibility", "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_jumpshield =    {"tv_jumpshield",    "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_watershield =   {"tv_watershield",   "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ringshield =    {"tv_ringshield",    "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_forceshield =   {"tv_forceshield",   "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_bombshield =    {"tv_bombshield",    "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_1up =           {"tv_1up",           "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_eggmanbox =     {"tv_eggman",        "5", CV_NETVAR|CV_CHEAT, chances_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_ringslinger = {"ringslinger", "No", CV_NETVAR|CV_NOSHOWHELP|CV_CALL|CV_CHEAT, CV_YesNo,
	Ringslinger_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_gravity = {"gravity", "0.5", CV_RESTRICT|CV_FLOAT|CV_CALL, NULL, Gravity_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_soundtest = {"soundtest", "0", CV_CALL, NULL, SoundTest_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t minitimelimit_cons_t[] = {{15, "MIN"}, {9999, "MAX"}, {0, NULL}};
consvar_t cv_countdowntime = {"countdowntime", "60", CV_NETVAR|CV_CHEAT, minitimelimit_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_touchtag = {"touchtag", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_hidetime = {"hidetime", "30", CV_NETVAR|CV_CALL, minitimelimit_cons_t, Hidetime_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_autobalance = {"autobalance", "0", CV_NETVAR|CV_CALL, autobalance_cons_t, AutoBalance_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_teamscramble = {"teamscramble", "Off", CV_NETVAR|CV_CALL|CV_NOINIT, teamscramble_cons_t, TeamScramble_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_scrambleonchange = {"scrambleonchange", "Off", CV_NETVAR, teamscramble_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_friendlyfire = {"friendlyfire", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_itemfinder = {"itemfinder", "Off", CV_CALL, CV_OnOff, ItemFinder_OnChange, 0, NULL, NULL, 0, 0, NULL};

// Scoring type options
consvar_t cv_match_scoring = {"matchscoring", "Normal", CV_NETVAR|CV_CHEAT, match_scoring_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_overtime = {"overtime", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_rollingdemos = {"rollingdemos", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_timetic = {"timerres", "Normal", CV_SAVE, timetic_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL}; // use tics in display
consvar_t cv_resetmusic = {"resetmusic", "No", CV_SAVE, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t pointlimit_cons_t[] = {{0, "MIN"}, {999999990, "MAX"}, {0, NULL}};
consvar_t cv_pointlimit = {"pointlimit", "0", CV_NETVAR|CV_CALL|CV_NOINIT, pointlimit_cons_t,
	PointLimit_OnChange, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t timelimit_cons_t[] = {{0, "MIN"}, {30, "MAX"}, {0, NULL}};
consvar_t cv_timelimit = {"timelimit", "0", CV_NETVAR|CV_CALL|CV_NOINIT, timelimit_cons_t,
	TimeLimit_OnChange, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t numlaps_cons_t[] = {{0, "MIN"}, {50, "MAX"}, {0, NULL}};
consvar_t cv_numlaps = {"numlaps", "4", CV_NETVAR|CV_CALL|CV_NOINIT, numlaps_cons_t,
	NumLaps_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_usemapnumlaps = {"usemaplaps", "Yes", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

// log elemental hazards -- not a netvar, is local to current player
consvar_t cv_hazardlog = {"hazardlog", "Yes", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_forceskin = {"forceskin", "-1", CV_NETVAR|CV_CALL|CV_CHEAT, NULL, ForceSkin_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_downloading = {"downloading", "On", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_allowexitlevel = {"allowexitlevel", "No", CV_NETVAR, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_killingdead = {"killingdead", "Off", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_netstat = {"netstat", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL}; // show bandwidth statistics
static CV_PossibleValue_t nettimeout_cons_t[] = {{TICRATE/7, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_nettimeout = {"nettimeout", "350", CV_CALL|CV_SAVE, nettimeout_cons_t, NetTimeout_OnChange, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t jointimeout_cons_t[] = {{5*TICRATE, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_jointimeout = {"jointimeout", "350", CV_CALL|CV_SAVE, jointimeout_cons_t, JoinTimeout_OnChange, 0, NULL, NULL, 0, 0, NULL};
#ifdef NEWPING
consvar_t cv_maxping = {"maxping", "0", CV_SAVE, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif
// Intermission time Tails 04-19-2002
static CV_PossibleValue_t inttime_cons_t[] = {{0, "MIN"}, {3600, "MAX"}, {0, NULL}};
consvar_t cv_inttime = {"inttime", "20", CV_NETVAR, inttime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t advancemap_cons_t[] = {{0, "Off"}, {1, "Next"}, {2, "Random"}, {0, NULL}};
consvar_t cv_advancemap = {"advancemap", "Next", CV_NETVAR, advancemap_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t playersforexit_cons_t[] = {{0, "One"}, {1, "All"}, {0, NULL}};
consvar_t cv_playersforexit = {"playersforexit", "One", CV_NETVAR, playersforexit_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_runscripts = {"runscripts", "Yes", 0, CV_YesNo, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_pause = {"pausepermission", "Server", CV_NETVAR, pause_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_mute = {"mute", "Off", CV_NETVAR|CV_CALL, CV_OnOff, Mute_OnChange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_sleep = {"cpusleep", "-1", CV_SAVE, sleeping_cons_t, NULL, -1, NULL, NULL, 0, 0, NULL};

INT16 gametype = GT_COOP;
boolean splitscreen = false;
boolean circuitmap = false;
INT32 adminplayer = -1;

/// \warning Keep this up-to-date if you add/remove/rename net text commands
const char *netxcmdnames[MAXNETXCMD - 1] =
{
	"NAMEANDCOLOR",
	"WEAPONPREF",
	"KICK",
	"NETVAR",
	"SAY",
	"MAP",
	"EXITLEVEL",
	"ADDFILE",
	"PAUSE",
	"ADDPLAYER",
	"TEAMCHANGE",
	"CLEARSCORES",
	"LOGIN",
	"VERIFIED",
	"RANDOMSEED",
	"RUNSOC",
	"REQADDFILE",
	"DELFILE",
	"SETMOTD",
	"SUICIDE",
#ifdef HAVE_BLUA
	"LUACMD",
	"LUAVAR"
#endif
};

// =========================================================================
//                           SERVER STARTUP
// =========================================================================

/** Registers server commands and variables.
  * Anything required by a dedicated server should probably go here.
  *
  * \sa D_RegisterClientCommands
  */
void D_RegisterServerCommands(void)
{
	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndColor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_REQADDFILE, Got_RequestAddfilecmd);
#ifdef DELFILE
	RegisterNetXCmd(XD_DELFILE, Got_Delfilecmd);
#endif
	RegisterNetXCmd(XD_PAUSE, Got_Pause);
	RegisterNetXCmd(XD_SUICIDE, Got_Suicide);
	RegisterNetXCmd(XD_RUNSOC, Got_RunSOCcmd);
#ifdef HAVE_BLUA
	RegisterNetXCmd(XD_LUACMD, Got_Luacmd);
#endif

	// Remote Administration
	COM_AddCommand("password", Command_Changepassword_f);
	RegisterNetXCmd(XD_LOGIN, Got_Login);
	COM_AddCommand("login", Command_Login_f); // useful in dedicated to kick off remote admin
	COM_AddCommand("verify", Command_Verify_f);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);

	COM_AddCommand("motd", Command_MotD_f);
	RegisterNetXCmd(XD_SETMOTD, Got_MotD_f); // For remote admin

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", Command_ServerTeamChange_f);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddCommand("clearscores", Command_Clearscores_f);
	COM_AddCommand("map", Command_Map_f);

	COM_AddCommand("exitgame", Command_ExitGame_f);
	COM_AddCommand("retry", Command_Retry_f);
	COM_AddCommand("exitlevel", Command_ExitLevel_f);
	COM_AddCommand("showmap", Command_Showmap_f);
	COM_AddCommand("mapmd5", Command_Mapmd5_f);

	COM_AddCommand("addfile", Command_Addfile);
	COM_AddCommand("listwad", Command_ListWADS_f);

#ifdef DELFILE
	COM_AddCommand("delfile", Command_Delfile);
#endif
	COM_AddCommand("runsoc", Command_RunSOC);
	COM_AddCommand("pause", Command_Pause);
	COM_AddCommand("suicide", Command_Suicide);

	COM_AddCommand("gametype", Command_ShowGametype_f);
	COM_AddCommand("version", Command_Version_f);
#ifdef UPDATE_ALERT
	COM_AddCommand("mod_details", Command_ModDetails_f);
#endif
	COM_AddCommand("quit", Command_Quit_f);

	COM_AddCommand("saveconfig", Command_SaveConfig_f);
	COM_AddCommand("loadconfig", Command_LoadConfig_f);
	COM_AddCommand("changeconfig", Command_ChangeConfig_f);
	COM_AddCommand("isgamemodified", Command_Isgamemodified_f); // test
	COM_AddCommand("showscores", Command_ShowScores_f);
	COM_AddCommand("showtime", Command_ShowTime_f);
	COM_AddCommand("cheats", Command_Cheats_f); // test
#ifdef _DEBUG
	COM_AddCommand("togglemodified", Command_Togglemodified_f);
#ifdef HAVE_BLUA
	COM_AddCommand("archivetest", Command_Archivetest_f);
#endif
#endif

	// for master server connection
	AddMServCommands();

	// p_mobj.c
	CV_RegisterVar(&cv_itemrespawntime);
	CV_RegisterVar(&cv_itemrespawn);
	CV_RegisterVar(&cv_flagtime);
	CV_RegisterVar(&cv_suddendeath);

	// misc
	CV_RegisterVar(&cv_friendlyfire);
	CV_RegisterVar(&cv_pointlimit);
	CV_RegisterVar(&cv_numlaps);
	CV_RegisterVar(&cv_usemapnumlaps);

	CV_RegisterVar(&cv_hazardlog);

	CV_RegisterVar(&cv_autobalance);
	CV_RegisterVar(&cv_teamscramble);
	CV_RegisterVar(&cv_scrambleonchange);

	CV_RegisterVar(&cv_touchtag);
	CV_RegisterVar(&cv_hidetime);

	CV_RegisterVar(&cv_inttime);
	CV_RegisterVar(&cv_advancemap);
	CV_RegisterVar(&cv_playersforexit);
	CV_RegisterVar(&cv_timelimit);
	CV_RegisterVar(&cv_playbackspeed);
	CV_RegisterVar(&cv_forceskin);
	CV_RegisterVar(&cv_downloading);

	CV_RegisterVar(&cv_specialrings);
	CV_RegisterVar(&cv_powerstones);
	CV_RegisterVar(&cv_competitionboxes);
	CV_RegisterVar(&cv_matchboxes);

	CV_RegisterVar(&cv_recycler);
	CV_RegisterVar(&cv_teleporters);
	CV_RegisterVar(&cv_superring);
	CV_RegisterVar(&cv_supersneakers);
	CV_RegisterVar(&cv_invincibility);
	CV_RegisterVar(&cv_jumpshield);
	CV_RegisterVar(&cv_watershield);
	CV_RegisterVar(&cv_ringshield);
	CV_RegisterVar(&cv_forceshield);
	CV_RegisterVar(&cv_bombshield);
	CV_RegisterVar(&cv_1up);
	CV_RegisterVar(&cv_eggmanbox);

	CV_RegisterVar(&cv_ringslinger);

	CV_RegisterVar(&cv_startinglives);
	CV_RegisterVar(&cv_countdowntime);
	CV_RegisterVar(&cv_runscripts);
	CV_RegisterVar(&cv_match_scoring);
	CV_RegisterVar(&cv_overtime);
	CV_RegisterVar(&cv_pause);
	CV_RegisterVar(&cv_mute);

	RegisterNetXCmd(XD_RANDOMSEED, Got_RandomSeed);

	CV_RegisterVar(&cv_allowexitlevel);
	CV_RegisterVar(&cv_restrictskinchange);
	CV_RegisterVar(&cv_allowteamchange);
	CV_RegisterVar(&cv_respawntime);
	CV_RegisterVar(&cv_killingdead);

	// d_clisrv
	CV_RegisterVar(&cv_maxplayers);
	CV_RegisterVar(&cv_maxsend);
	CV_RegisterVar(&cv_noticedownload);
	CV_RegisterVar(&cv_downloadspeed);

	COM_AddCommand("ping", Command_Ping_f);
	CV_RegisterVar(&cv_nettimeout);
	CV_RegisterVar(&cv_jointimeout);

	CV_RegisterVar(&cv_skipmapcheck);
	CV_RegisterVar(&cv_sleep);
#ifdef NEWPING
	CV_RegisterVar(&cv_maxping);
#endif

#ifdef SEENAMES
	 CV_RegisterVar(&cv_allowseenames);
#endif

	CV_RegisterVar(&cv_dummyconsvar);
}

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

/** Registers client commands and variables.
  * Nothing needed for a dedicated server should be registered here.
  *
  * \sa D_RegisterServerCommands
  */
void D_RegisterClientCommands(void)
{
	INT32 i;

	for (i = 0; i < MAXSKINCOLORS; i++)
	{
		Color_cons_t[i].value = i;
		Color_cons_t[i].strvalue = Color_Names[i];
	}
	Color_cons_t[MAXSKINCOLORS].value = 0;
	Color_cons_t[MAXSKINCOLORS].strvalue = NULL;

	if (dedicated)
		return;

	COM_AddCommand("numthinkers", Command_Numthinkers_f);
	COM_AddCommand("countmobjs", Command_CountMobjs_f);

	COM_AddCommand("changeteam", Command_Teamchange_f);
	COM_AddCommand("changeteam2", Command_Teamchange2_f);

	COM_AddCommand("playdemo", Command_Playdemo_f);
	COM_AddCommand("timedemo", Command_Timedemo_f);
	COM_AddCommand("stopdemo", Command_Stopdemo_f);
	COM_AddCommand("playintro", Command_Playintro_f);

	COM_AddCommand("resetcamera", Command_ResetCamera_f);

	COM_AddCommand("setcontrol", Command_Setcontrol_f);
	COM_AddCommand("setcontrol2", Command_Setcontrol2_f);

	COM_AddCommand("screenshot", M_ScreenShot);
	COM_AddCommand("startmovie", Command_StartMovie_f);
	COM_AddCommand("stopmovie", Command_StopMovie_f);

	CV_RegisterVar(&cv_screenshot_option);
	CV_RegisterVar(&cv_screenshot_folder);
	CV_RegisterVar(&cv_moviemode);
	// PNG variables
	CV_RegisterVar(&cv_zlib_level);
	CV_RegisterVar(&cv_zlib_memory);
	CV_RegisterVar(&cv_zlib_strategy);
	CV_RegisterVar(&cv_zlib_window_bits);
	// APNG variables
	CV_RegisterVar(&cv_zlib_levela);
	CV_RegisterVar(&cv_zlib_memorya);
	CV_RegisterVar(&cv_zlib_strategya);
	CV_RegisterVar(&cv_zlib_window_bitsa);
	CV_RegisterVar(&cv_apng_delay);
	// GIF variables
	CV_RegisterVar(&cv_gif_optimize);
	CV_RegisterVar(&cv_gif_downscale);

#ifdef WALLSPLATS
	CV_RegisterVar(&cv_splats);
#endif

	// register these so it is saved to config
	CV_RegisterVar(&cv_playername);
	CV_RegisterVar(&cv_playercolor);
	CV_RegisterVar(&cv_skin); // r_things.c (skin NAME)
	// secondary player (splitscreen)
	CV_RegisterVar(&cv_playername2);
	CV_RegisterVar(&cv_playercolor2);
	CV_RegisterVar(&cv_skin2);

#ifdef SEENAMES
	CV_RegisterVar(&cv_seenames);
#endif
	CV_RegisterVar(&cv_rollingdemos);
	CV_RegisterVar(&cv_netstat);

#ifdef NETGAME_DEVMODE
	CV_RegisterVar(&cv_fishcake);
#endif

	// HUD
	CV_RegisterVar(&cv_timetic);
	CV_RegisterVar(&cv_itemfinder);

	// time attack ghost options are also saved to config
	CV_RegisterVar(&cv_ghost_bestscore);
	CV_RegisterVar(&cv_ghost_besttime);
	CV_RegisterVar(&cv_ghost_bestrings);
	CV_RegisterVar(&cv_ghost_last);
	CV_RegisterVar(&cv_ghost_guest);

	COM_AddCommand("displayplayer", Command_Displayplayer_f);
	COM_AddCommand("tunes", Command_Tunes_f);
	COM_AddCommand("restartaudio", Command_RestartAudio_f);
	CV_RegisterVar(&cv_resetmusic);

	// FIXME: not to be here.. but needs be done for config loading
	CV_RegisterVar(&cv_usegamma);

	// m_menu.c
	CV_RegisterVar(&cv_crosshair);
	CV_RegisterVar(&cv_crosshair2);
	CV_RegisterVar(&cv_alwaysfreelook);
	CV_RegisterVar(&cv_alwaysfreelook2);

	// g_input.c
	CV_RegisterVar(&cv_sideaxis);
	CV_RegisterVar(&cv_sideaxis2);
	CV_RegisterVar(&cv_turnaxis);
	CV_RegisterVar(&cv_turnaxis2);
	CV_RegisterVar(&cv_moveaxis);
	CV_RegisterVar(&cv_moveaxis2);
	CV_RegisterVar(&cv_lookaxis);
	CV_RegisterVar(&cv_lookaxis2);
	CV_RegisterVar(&cv_fireaxis);
	CV_RegisterVar(&cv_fireaxis2);
	CV_RegisterVar(&cv_firenaxis);
	CV_RegisterVar(&cv_firenaxis2);

	// WARNING: the order is important when initialising mouse2
	// we need the mouse2port
	CV_RegisterVar(&cv_mouse2port);
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
	CV_RegisterVar(&cv_mouse2opt);
#endif
	CV_RegisterVar(&cv_controlperkey);

	CV_RegisterVar(&cv_usemouse);
	CV_RegisterVar(&cv_usemouse2);
	CV_RegisterVar(&cv_invertmouse);
	CV_RegisterVar(&cv_invertmouse2);
	CV_RegisterVar(&cv_mousesens);
	CV_RegisterVar(&cv_mousesens2);
	CV_RegisterVar(&cv_mouseysens);
	CV_RegisterVar(&cv_mouseysens2);
	CV_RegisterVar(&cv_mousemove);
	CV_RegisterVar(&cv_mousemove2);

	CV_RegisterVar(&cv_usejoystick);
	CV_RegisterVar(&cv_usejoystick2);
#ifdef LJOYSTICK
	CV_RegisterVar(&cv_joyport);
	CV_RegisterVar(&cv_joyport2);
#endif
	CV_RegisterVar(&cv_joyscale);
	CV_RegisterVar(&cv_joyscale2);

	// Analog Control
	CV_RegisterVar(&cv_analog);
	CV_RegisterVar(&cv_analog2);
	CV_RegisterVar(&cv_useranalog);
	CV_RegisterVar(&cv_useranalog2);

	// s_sound.c
	CV_RegisterVar(&cv_soundvolume);
	CV_RegisterVar(&cv_digmusicvolume);
	CV_RegisterVar(&cv_midimusicvolume);
	CV_RegisterVar(&cv_numChannels);

	// i_cdmus.c
	CV_RegisterVar(&cd_volume);
	CV_RegisterVar(&cdUpdate);

	// screen.c
	CV_RegisterVar(&cv_fullscreen);
	CV_RegisterVar(&cv_renderview);
	CV_RegisterVar(&cv_scr_depth);
	CV_RegisterVar(&cv_scr_width);
	CV_RegisterVar(&cv_scr_height);

	CV_RegisterVar(&cv_soundtest);

	// ingame object placing
	COM_AddCommand("objectplace", Command_ObjectPlace_f);
	COM_AddCommand("writethings", Command_Writethings_f);
	CV_RegisterVar(&cv_speed);
	CV_RegisterVar(&cv_opflags);
	CV_RegisterVar(&cv_mapthingnum);
//	CV_RegisterVar(&cv_grid);
//	CV_RegisterVar(&cv_snapto);

	// add cheat commands
	COM_AddCommand("noclip", Command_CheatNoClip_f);
	COM_AddCommand("god", Command_CheatGod_f);
	COM_AddCommand("notarget", Command_CheatNoTarget_f);
	COM_AddCommand("getallemeralds", Command_Getallemeralds_f);
	COM_AddCommand("resetemeralds", Command_Resetemeralds_f);
	COM_AddCommand("setrings", Command_Setrings_f);
	COM_AddCommand("setlives", Command_Setlives_f);
	COM_AddCommand("setcontinues", Command_Setcontinues_f);
	COM_AddCommand("devmode", Command_Devmode_f);
	COM_AddCommand("savecheckpoint", Command_Savecheckpoint_f);
	COM_AddCommand("scale", Command_Scale_f);
	COM_AddCommand("gravflip", Command_Gravflip_f);
	COM_AddCommand("hurtme", Command_Hurtme_f);
	COM_AddCommand("jumptoaxis", Command_JumpToAxis_f);
	COM_AddCommand("charability", Command_Charability_f);
	COM_AddCommand("charspeed", Command_Charspeed_f);
	COM_AddCommand("teleport", Command_Teleport_f);
	COM_AddCommand("rteleport", Command_RTeleport_f);
	COM_AddCommand("skynum", Command_Skynum_f);
	COM_AddCommand("weather", Command_Weather_f);
#ifdef _DEBUG
	COM_AddCommand("causecfail", Command_CauseCfail_f);
#endif
#if defined(HAVE_BLUA) && defined(LUA_ALLOW_BYTECODE)
	COM_AddCommand("dumplua", Command_Dumplua_f);
#endif
}

/** Checks if a name (as received from another player) is okay.
  * A name is okay if it is no fewer than 1 and no more than ::MAXPLAYERNAME
  * chars long (not including NUL), it does not begin or end with a space,
  * it does not contain non-printing characters (according to isprint(), which
  * allows space), it does not start with a digit, and no other player is
  * currently using it.
  * \param name      Name to check.
  * \param playernum Player who wants the name, so we can check if they already
  *                  have it, and let them keep it if so.
  * \sa CleanupPlayerName, SetPlayerName, Got_NameAndColor
  * \author Graue <graue@oceanbase.org>
  */
static boolean IsNameGood(char *name, INT32 playernum)
{
	INT32 ix;

	if (strlen(name) == 0 || strlen(name) > MAXPLAYERNAME)
		return false; // Empty or too long.
	if (name[0] == ' ' || name[strlen(name)-1] == ' ')
		return false; // Starts or ends with a space.
	if (isdigit(name[0]))
		return false; // Starts with a digit.
	if (name[0] == '@' || name[0] == '~')
		return false; // Starts with an admin symbol.

	// Check if it contains a non-printing character.
	// Note: ANSI C isprint() considers space a printing character.
	// Also don't allow semicolons, since they are used as
	// console command separators.

	// Also, anything over 0x80 is disallowed too, since compilers love to
	// differ on whether they're printable characters or not.
	for (ix = 0; name[ix] != '\0'; ix++)
		if (!isprint(name[ix]) || name[ix] == ';' || (UINT8)(name[ix]) >= 0x80)
			return false;

	// Check if a player is currently using the name, case-insensitively.
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		if (ix != playernum && playeringame[ix]
			&& strcasecmp(name, player_names[ix]) == 0)
		{
			// We shouldn't kick people out just because
			// they joined the game with the same name
			// as someone else -- modify the name instead.
			size_t len = strlen(name);

			// Recursion!
			// Slowly strip characters off the end of the
			// name until we no longer have a duplicate.
			if (len > 1)
			{
				name[len-1] = '\0';
				if (!IsNameGood (name, playernum))
					return false;
			}
			else if (len == 1) // Agh!
			{
				// Last ditch effort...
				sprintf(name, "%d", M_RandomKey(10));
				if (!IsNameGood (name, playernum))
					return false;
			}
			else
				return false;
		}
	}

	return true;
}

/** Cleans up a local player's name before sending a name change.
  * Spaces at the beginning or end of the name are removed. Then if the new
  * name is identical to another player's name, ignoring case, the name change
  * is canceled, and the name in cv_playername.value or cv_playername2.value
  * is restored to what it was before.
  *
  * We assume that if playernum is ::consoleplayer or ::secondarydisplayplayer
  * the console variable ::cv_playername or ::cv_playername2 respectively is
  * already set to newname. However, the player name table is assumed to
  * contain the old name.
  *
  * \param playernum Player number who has requested a name change.
  *                  Should be ::consoleplayer or ::secondarydisplayplayer.
  * \param newname   New name for that player; should already be in
  *                  ::cv_playername or ::cv_playername2 if player is the
  *                  console or secondary display player, respectively.
  * \sa cv_playername, cv_playername2, SendNameAndColor, SendNameAndColor2,
  *     SetPlayerName
  * \author Graue <graue@oceanbase.org>
  */
static void CleanupPlayerName(INT32 playernum, const char *newname)
{
	char *buf;
	char *p;
	char *tmpname = NULL;
	INT32 i;
	boolean namefailed = true;

	buf = Z_StrDup(newname);

	do
	{
		p = buf;

		while (*p == ' ')
			p++; // remove leading spaces

		if (strlen(p) == 0)
			break; // empty names not allowed

		if (isdigit(*p))
			break; // names starting with digits not allowed

		if (*p == '@' || *p == '~')
			break; // names that start with @ or ~ (admin symbols) not allowed

		tmpname = p;

		// Remove trailing spaces.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		if (strlen(tmpname) == 0)
			break; // another case of an empty name

		// Truncate name if it's too long (max MAXPLAYERNAME chars
		// excluding NUL).
		if (strlen(tmpname) > MAXPLAYERNAME)
			tmpname[MAXPLAYERNAME] = '\0';

		// Remove trailing spaces again.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		// no stealing another player's name
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (i != playernum && playeringame[i]
				&& strcasecmp(tmpname, player_names[i]) == 0)
			{
				break;
			}
		}

		if (i < MAXPLAYERS)
			break;

		// name is okay then
		namefailed = false;
	} while (0);

	if (namefailed)
		tmpname = player_names[playernum];

	// set consvars whether namefailed or not, because even if it succeeded,
	// spaces may have been removed
	if (playernum == consoleplayer)
		CV_StealthSet(&cv_playername, tmpname);
	else if (playernum == secondarydisplayplayer
		|| (!netgame && playernum == 1))
	{
		CV_StealthSet(&cv_playername2, tmpname);
	}
	else I_Assert(((void)"CleanupPlayerName used on non-local player", 0));

	Z_Free(buf);
}

/** Sets a player's name, if it is good.
  * If the name is not good (indicating a modified or buggy client), it is not
  * set, and if we are the server in a netgame, the player responsible is
  * kicked with a consistency failure.
  *
  * This function prints a message indicating the name change, unless the game
  * is currently showing the intro, e.g. when processing autoexec.cfg.
  *
  * \param playernum Player number who has requested a name change.
  * \param newname   New name for that player. Should be good, but won't
  *                  necessarily be if the client is maliciously modified or
  *                  buggy.
  * \sa CleanupPlayerName, IsNameGood
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(INT32 playernum, char *newname)
{
	if (IsNameGood(newname, playernum))
	{
		if (strcasecmp(newname, player_names[playernum]) != 0)
		{
			if (netgame)
				CONS_Printf(M_GetText("%s renamed to %s\n"),
					player_names[playernum], newname);
			strcpy(player_names[playernum], newname);
		}
	}
	else
	{
		CONS_Printf(M_GetText("Player %d sent a bad name change\n"), playernum+1);
		if (server && netgame)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
	}
}

UINT8 CanChangeSkin(INT32 playernum)
{
	// Of course we can change if we're not playing
	if (!Playing() || !addedtogame)
		return true;

	// Force skin in effect.
	if (client && (cv_forceskin.value != -1) && !(adminplayer == playernum && serverplayer == -1))
		return false;

	// Can change skin in intermission and whatnot.
	if (gamestate != GS_LEVEL)
		return true;

	// Server has skin change restrictions.
	if (cv_restrictskinchange.value)
	{
		if (gametype == GT_COOP)
			return true;

		// Can change skin during initial countdown.
		if ((gametype == GT_RACE || gametype == GT_COMPETITION) && leveltime < 4*TICRATE)
			return true;

		if (G_TagGametype())
		{
			// Can change skin during hidetime.
			if (leveltime < hidetime * TICRATE)
				return true;

			// IT players can always change skins to persue players hiding in character only locations.
			if (players[playernum].pflags & PF_TAGIT)
				return true;
		}

		if (players[playernum].spectator || players[playernum].playerstate == PST_DEAD || players[playernum].playerstate == PST_REBORN)
			return true;

		return false;
	}

	return true;
}

static void ForceAllSkins(INT32 forcedskin)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
			continue;

		SetPlayerSkinByNum(i, forcedskin);

		// If it's me (or my brother), set appropriate skin value in cv_skin/cv_skin2
		if (!dedicated) // But don't do this for dedicated servers, of course.
		{
			if (i == consoleplayer)
				CV_StealthSet(&cv_skin, skins[forcedskin].name);
			else if (i == secondarydisplayplayer)
				CV_StealthSet(&cv_skin2, skins[forcedskin].name);
		}
	}
}

static INT32 snacpending = 0, snac2pending = 0, chmappending = 0;

// name, color, or skin has changed
//
static void SendNameAndColor(void)
{
	XBOXSTATIC char buf[MAXPLAYERNAME+2];
	char *p;

	p = buf;

	// normal player colors
	if (G_GametypeHasTeams())
	{
		if (players[consoleplayer].ctfteam == 1 && cv_playercolor.value != skincolor_redteam)
			CV_StealthSetValue(&cv_playercolor, skincolor_redteam);
		else if (players[consoleplayer].ctfteam == 2 && cv_playercolor.value != skincolor_blueteam)
			CV_StealthSetValue(&cv_playercolor, skincolor_blueteam);
	}

	// never allow the color "none"
	if (!cv_playercolor.value)
	{
		if (players[consoleplayer].skincolor)
			CV_StealthSetValue(&cv_playercolor, players[consoleplayer].skincolor);
		else if (skins[players[consoleplayer].skin].prefcolor)
			CV_StealthSetValue(&cv_playercolor, skins[players[consoleplayer].skin].prefcolor);
		else
			CV_StealthSet(&cv_playercolor, cv_playercolor.defaultvalue);
	}

	if (!strcmp(cv_playername.string, player_names[consoleplayer])
	&& cv_playercolor.value == players[consoleplayer].skincolor
	&& !strcmp(cv_skin.string, skins[players[consoleplayer].skin].name))
		return;

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	// If you're not in a netgame, merely update the skin, color, and name.
	if (!netgame)
	{
		INT32 foundskin;

		CleanupPlayerName(consoleplayer, cv_playername.zstring);
		strcpy(player_names[consoleplayer], cv_playername.zstring);

		players[consoleplayer].skincolor = cv_playercolor.value;

		if (players[consoleplayer].mo)
			players[consoleplayer].mo->color = players[consoleplayer].skincolor;

		if (metalrecording)
		{ // Metal Sonic is Sonic, obviously.
			SetPlayerSkinByNum(consoleplayer, 0);
			CV_StealthSet(&cv_skin, skins[0].name);
		}
		else if ((foundskin = R_SkinAvailable(cv_skin.string)) != -1)
		{
			boolean notsame;

			cv_skin.value = foundskin;

			notsame = (cv_skin.value != players[consoleplayer].skin);

			SetPlayerSkin(consoleplayer, cv_skin.string);
			CV_StealthSet(&cv_skin, skins[cv_skin.value].name);

			if (notsame)
			{
				CV_StealthSetValue(&cv_playercolor, skins[cv_skin.value].prefcolor);

				players[consoleplayer].skincolor = (cv_playercolor.value&0x1F) % MAXSKINCOLORS;

				if (players[consoleplayer].mo)
					players[consoleplayer].mo->color = (UINT8)players[consoleplayer].skincolor;
			}
		}
		else
		{
			cv_skin.value = players[consoleplayer].skin;
			CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
			// will always be same as current
			SetPlayerSkin(consoleplayer, cv_skin.string);
		}

		return;
	}

	snacpending++;

	// Don't change name if muted
	if (cv_mute.value && !(server || adminplayer == consoleplayer))
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
	else // Cleanup name if changing it
		CleanupPlayerName(consoleplayer, cv_playername.zstring);

	// Don't change skin if the server doesn't want you to.
	if (!CanChangeSkin(consoleplayer))
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin.value = R_SkinAvailable(cv_skin.string);
	if (cv_skin.value < 0)
	{
		CV_StealthSet(&cv_skin, DEFAULTSKIN);
		cv_skin.value = 0;
	}

	// Finally write out the complete packet and send it off.
	WRITESTRINGN(p, cv_playername.zstring, MAXPLAYERNAME);
	WRITEUINT8(p, (UINT8)cv_playercolor.value);
	WRITEUINT8(p, (UINT8)cv_skin.value);
	SendNetXCmd(XD_NAMEANDCOLOR, buf, p - buf);
}

// splitscreen
static void SendNameAndColor2(void)
{
	INT32 secondplaya;

	if (!splitscreen && !botingame)
		return; // can happen if skin2/color2/name2 changed

	if (secondarydisplayplayer != consoleplayer)
		secondplaya = secondarydisplayplayer;
	else // HACK
		secondplaya = 1;

	// normal player colors
	if (G_GametypeHasTeams())
	{
		if (players[secondplaya].ctfteam == 1 && cv_playercolor2.value != skincolor_redteam)
			CV_StealthSetValue(&cv_playercolor2, skincolor_redteam);
		else if (players[secondplaya].ctfteam == 2 && cv_playercolor2.value != skincolor_blueteam)
			CV_StealthSetValue(&cv_playercolor2, skincolor_blueteam);
	}

	// never allow the color "none"
	if (!cv_playercolor2.value)
	{
		if (players[secondplaya].skincolor)
			CV_StealthSetValue(&cv_playercolor2, players[secondplaya].skincolor);
		else if (skins[players[secondplaya].skin].prefcolor)
			CV_StealthSetValue(&cv_playercolor2, skins[players[secondplaya].skin].prefcolor);
		else
			CV_StealthSet(&cv_playercolor2, cv_playercolor2.defaultvalue);
	}

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	// If you're not in a netgame, merely update the skin, color, and name.
	if (botingame)
	{
		players[secondplaya].skincolor = botcolor;
		if (players[secondplaya].mo)
			players[secondplaya].mo->color = players[secondplaya].skincolor;
		SetPlayerSkinByNum(secondplaya, botskin-1);
		return;
	}
	else if (!netgame)
	{
		INT32 foundskin;

		CleanupPlayerName(secondplaya, cv_playername2.zstring);
		strcpy(player_names[secondplaya], cv_playername2.zstring);

		// don't use secondarydisplayplayer: the second player must be 1
		players[secondplaya].skincolor = cv_playercolor2.value;
		if (players[secondplaya].mo)
			players[secondplaya].mo->color = players[secondplaya].skincolor;

		if (cv_forceskin.value >= 0 && (netgame || multiplayer)) // Server wants everyone to use the same player
		{
			const INT32 forcedskin = cv_forceskin.value;

			SetPlayerSkinByNum(secondplaya, forcedskin);
			CV_StealthSet(&cv_skin2, skins[forcedskin].name);
		}
		else if ((foundskin = R_SkinAvailable(cv_skin2.string)) != -1)
		{
			boolean notsame;

			cv_skin2.value = foundskin;

			notsame = (cv_skin2.value != players[secondplaya].skin);

			SetPlayerSkin(secondplaya, cv_skin2.string);

			if (notsame)
			{
				CV_StealthSetValue(&cv_playercolor2, skins[players[secondplaya].skin].prefcolor);

				players[secondplaya].skincolor = (cv_playercolor2.value&0x1F) % MAXSKINCOLORS;

				if (players[secondplaya].mo)
					players[secondplaya].mo->color = players[secondplaya].skincolor;
			}
		}
		else
		{
			cv_skin2.value = players[secondplaya].skin;
			CV_StealthSet(&cv_skin2, skins[players[secondplaya].skin].name);
			// will always be same as current
			SetPlayerSkin(secondplaya, cv_skin2.string);
		}
		return;
	}

	// Don't actually send anything because splitscreen isn't actually allowed in netgames anyway!
}

static void Got_NameAndColor(UINT8 **cp, INT32 playernum)
{
	player_t *p = &players[playernum];
	char name[MAXPLAYERNAME+1];
	UINT8 color, skin;

#ifdef PARANOIA
	if (playernum < 0 || playernum > MAXPLAYERS)
		I_Error("There is no player %d!", playernum);
#endif

	if (playernum == consoleplayer)
		snacpending--;
	else if (playernum == secondarydisplayplayer)
		snac2pending--;

#ifdef PARANOIA
	if (snacpending < 0 || snac2pending < 0)
		I_Error("snacpending negative!");
#endif

	READSTRINGN(*cp, name, MAXPLAYERNAME);
	color = READUINT8(*cp);
	skin = READUINT8(*cp);

	// set name
	if (strcasecmp(player_names[playernum], name) != 0)
		SetPlayerName(playernum, name);

	// set color
	p->skincolor = color % MAXSKINCOLORS;
	if (p->mo)
		p->mo->color = (UINT8)p->skincolor;

	// normal player colors
	if (server && (p != &players[consoleplayer] && p != &players[secondarydisplayplayer]))
	{
		boolean kick = false;

		// team colors
		if (G_GametypeHasTeams())
		{
			if (p->ctfteam == 1 && p->skincolor != skincolor_redteam)
				kick = true;
			else if (p->ctfteam == 2 && p->skincolor != skincolor_blueteam)
				kick = true;
		}

		// don't allow color "none"
		if (!p->skincolor)
			kick = true;

		if (kick)
		{
			XBOXSTATIC UINT8 buf[2];
			CONS_Alert(CONS_WARNING, M_GetText("Illegal color change received from %s (team: %d), color: %d)\n"), player_names[playernum], p->ctfteam, p->skincolor);

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
			return;
		}
	}

	// set skin
	if (cv_forceskin.value >= 0 && (netgame || multiplayer)) // Server wants everyone to use the same player
	{
		const INT32 forcedskin = cv_forceskin.value;
		SetPlayerSkinByNum(playernum, forcedskin);

		if (playernum == consoleplayer)
			CV_StealthSet(&cv_skin, skins[forcedskin].name);
		else if (playernum == secondarydisplayplayer)
			CV_StealthSet(&cv_skin2, skins[forcedskin].name);
	}
	else
		SetPlayerSkinByNum(playernum, skin);
}

void SendWeaponPref(void)
{
	XBOXSTATIC UINT8 buf[1];

	buf[0] = 0;
	if (players[consoleplayer].pflags & PF_FLIPCAM)
		buf[0] |= 1;
	if (players[consoleplayer].pflags & PF_ANALOGMODE)
		buf[0] |= 2;
	SendNetXCmd(XD_WEAPONPREF, buf, 1);
}

void SendWeaponPref2(void)
{
	XBOXSTATIC UINT8 buf[1];

	buf[0] = 0;
	if (players[secondarydisplayplayer].pflags & PF_FLIPCAM)
		buf[0] |= 1;
	if (players[secondarydisplayplayer].pflags & PF_ANALOGMODE)
		buf[0] |= 2;
	SendNetXCmd2(XD_WEAPONPREF, buf, 1);
}

static void Got_WeaponPref(UINT8 **cp,INT32 playernum)
{
	UINT8 prefs = READUINT8(*cp);

	players[playernum].pflags &= ~(PF_FLIPCAM|PF_ANALOGMODE);
	if (prefs & 1)
		players[playernum].pflags |= PF_FLIPCAM;
	if (prefs & 2)
		players[playernum].pflags |= PF_ANALOGMODE;
}

void D_SendPlayerConfig(void)
{
	SendNameAndColor();
	if (splitscreen || botingame)
		SendNameAndColor2();
	SendWeaponPref();
	if (splitscreen)
		SendWeaponPref2();
}

// Only works for displayplayer, sorry!
static void Command_ResetCamera_f(void)
{
	P_ResetCamera(&players[displayplayer], &camera);
}

// ========================================================================

// play a demo, add .lmp for external demos
// eg: playdemo demo1 plays the internal game demo
//
// UINT8 *demofile; // demo file buffer
static void Command_Playdemo_f(void)
{
	char name[256];

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("playdemo <demoname>: playback a demo\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	// disconnect from server here?
	if (demoplayback)
		G_StopDemo();
	if (metalplayback)
		G_StopMetalDemo();

	// open the demo file
	strcpy(name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf(M_GetText("Playing back demo '%s'.\n"), name);

	// Internal if no extension, external if one exists
	// If external, convert the file name to a path in SRB2's home directory
	if (FIL_CheckExtension(name))
		G_DoPlayDemo(va("%s"PATHSEP"%s", srb2home, name));
	else
		G_DoPlayDemo(name);
}

static void Command_Timedemo_f(void)
{
	char name[256];

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("timedemo <demoname>: time a demo\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	// disconnect from server here?
	if (demoplayback)
		G_StopDemo();
	if (metalplayback)
		G_StopMetalDemo();

	// open the demo file
	strcpy (name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf(M_GetText("Timing demo '%s'.\n"), name);

	G_TimeDemo(name);
}

// stop current demo
static void Command_Stopdemo_f(void)
{
	G_CheckDemoStatus();
	CONS_Printf(M_GetText("Stopped demo.\n"));
}

static void Command_StartMovie_f(void)
{
	M_StartMovie();
}

static void Command_StopMovie_f(void)
{
	M_StopMovie();
}

INT32 mapchangepending = 0;

/** Runs a map change.
  * The supplied data are assumed to be good. If provided by a user, they will
  * have already been checked in Command_Map_f().
  *
  * Do \b NOT call this function directly from a menu! M_Responder() is called
  * from within the event processing loop, and this function calls
  * SV_SpawnServer(), which calls CL_ConnectToServer(), which gives you "Press
  * ESC to abort", which calls I_GetKey(), which adds an event. In other words,
  * 63 old events will get reexecuted, with ridiculous results. Just don't do
  * it (without setting delay to 1, which is the current solution).
  *
  * \param mapnum          Map number to change to.
  * \param gametype        Gametype to switch to.
  * \param pultmode        Is this 'Ultimate Mode'?
  * \param resetplayers    1 to reset player scores and lives and such, 0 not to.
  * \param delay           Determines how the function will be executed: 0 to do
  *                        it all right now (must not be done from a menu), 1 to
  *                        do step one and prepare step two, 2 to do step two.
  * \param skipprecutscene To skip the precutscence or not?
  * \sa D_GameTypeChanged, Command_Map_f
  * \author Graue <graue@oceanbase.org>
  */
void D_MapChange(INT32 mapnum, INT32 newgametype, boolean pultmode, boolean resetplayers, INT32 delay, boolean skipprecutscene, boolean FLS)
{
	static char buf[2+MAX_WADPATH+1+4];
	static char *buf_p = buf;

	// The supplied data are assumed to be good.
	I_Assert(delay >= 0 && delay <= 2);

	CONS_Debug(DBG_GAMELOGIC, "Map change: mapnum=%d gametype=%d ultmode=%d resetplayers=%d delay=%d skipprecutscene=%d\n",
	           mapnum, newgametype, pultmode, resetplayers, delay, skipprecutscene);

	if (netgame || multiplayer)
		FLS = false;

	if (delay != 2)
	{
		UINT8 flags = 0;
		const char *mapname = G_BuildMapName(mapnum);

		I_Assert(W_CheckNumForName(mapname) != LUMPERROR);

		buf_p = buf;
		if (pultmode)
			flags |= 1;
		if (!resetplayers)
			flags |= 1<<1;
		if (skipprecutscene)
			flags |= 1<<2;
		if (FLS)
			flags |= 1<<3;
		WRITEUINT8(buf_p, flags);

		// new gametype value
		WRITEUINT8(buf_p, newgametype);

		WRITESTRINGN(buf_p, mapname, MAX_WADPATH);
	}

	if (delay == 1)
		mapchangepending = 1;
	else
	{
		mapchangepending = 0;
		// spawn the server if needed
		// reset players if there is a new one
		if (!(adminplayer == consoleplayer))
		{
			if (SV_SpawnServer())
				buf[0] &= ~(1<<1);
			if (!Playing()) // you failed to start a server somehow, so cancel the map change
				return;
		}

		// Kick bot from special stages
		if (botskin)
		{
			if (G_IsSpecialStage(mapnum))
			{
				if (botingame)
				{
					//CL_RemoveSplitscreenPlayer();
					botingame = false;
					playeringame[1] = false;
				}
			}
			else if (!botingame)
			{
				//CL_AddSplitscreenPlayer();
				botingame = true;
				secondarydisplayplayer = 1;
				playeringame[1] = true;
				players[1].bot = 1;
				SendNameAndColor2();
			}
		}

		chmappending++;
		if (netgame)
			WRITEUINT32(buf_p, M_RandomizedSeed()); // random seed
		SendNetXCmd(XD_MAP, buf, buf_p - buf);
	}
}

// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
static void Command_Map_f(void)
{
	const char *mapname;
	size_t i;
	INT32 j, newmapnum;
	boolean newresetplayers;
	INT32 newgametype = gametype;

	// max length of command: map map03 -gametype coop -noresetplayers -force
	//                         1    2       3       4         5           6
	// = 8 arg max
	if (COM_Argc() < 2 || COM_Argc() > 8)
	{
		CONS_Printf(M_GetText("map <mapname> [-gametype <type> [-force]: warp to map\n"));
		return;
	}

	if (client && !(adminplayer == consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	// internal wad lump always: map command doesn't support external files as in doom legacy
	if (W_CheckNumForName(COM_Argv(1)) == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Internal game level '%s' not found\n"), COM_Argv(1));
		return;
	}

	if (!(netgame || multiplayer) && (!modifiedgame || savemoddata))
	{
		if (COM_CheckParm("-force"))
			G_SetGameModified(false);
		else
		{
			CONS_Printf(M_GetText("Sorry, level change disabled in single player.\n"));
			return;
		}
	}

	newresetplayers = !COM_CheckParm("-noresetplayers");

	if (!newresetplayers && !cv_debug)
	{
		CONS_Printf(M_GetText("DEVMODE must be enabled.\n"));
		return;
	}

	mapname = COM_Argv(1);
	if (strlen(mapname) != 5
	|| (newmapnum = M_MapNumber(mapname[3], mapname[4])) == 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Invalid level name %s\n"), mapname);
		return;
	}

	// Ultimate Mode only in SP via menu
	if (netgame || multiplayer)
		ultimatemode = false;

	// new gametype value
	// use current one by default
	i = COM_CheckParm("-gametype");
	if (i)
	{
		if (!multiplayer)
		{
			CONS_Printf(M_GetText("You can't switch gametypes in single player!\n"));
			return;
		}

		for (j = 0; gametype_cons_t[j].strvalue; j++)
			if (!strcasecmp(gametype_cons_t[j].strvalue, COM_Argv(i+1)))
			{
				// Don't do any variable setting here. Wait until you get your
				// map packet first to avoid sending the same info twice!
				newgametype = gametype_cons_t[j].value;

				break;
			}

		if (!gametype_cons_t[j].strvalue) // reached end of the list with no match
		{
			// assume they gave us a gametype number, which is okay too
			for (j = 0; gametype_cons_t[j].strvalue != NULL; j++)
			{
				if (atoi(COM_Argv(i+1)) == gametype_cons_t[j].value)
				{
					newgametype = gametype_cons_t[j].value;
					break;
				}
			}
		}
	}

	// don't use a gametype the map doesn't support
	if (cv_debug || COM_CheckParm("-force") || cv_skipmapcheck.value)
		; // The player wants us to trek on anyway.  Do so.
	// G_TOLFlag handles both multiplayer gametype and ignores it for !multiplayer
	// Alternatively, bail if the map header is completely missing anyway.
	else if (!mapheaderinfo[newmapnum-1]
	 || !(mapheaderinfo[newmapnum-1]->typeoflevel & G_TOLFlag(newgametype)))
	{
		char gametypestring[32] = "Single Player";

		if (multiplayer)
			for (i = 0; gametype_cons_t[i].strvalue != NULL; i++)
				if (gametype_cons_t[i].value == newgametype)
				{
					strcpy(gametypestring, gametype_cons_t[i].strvalue);
					break;
				}

		CONS_Alert(CONS_WARNING, M_GetText("%s doesn't support %s mode!\n(Use -force to override)\n"), mapname, gametypestring);
		return;
	}

	// Prevent warping to locked levels
	// ... unless you're in a dedicated server.  Yes, technically this means you can view any level by
	// running a dedicated server and joining it yourself, but that's better than making dedicated server's
	// lives hell.
	if (!dedicated && M_MapLocked(newmapnum))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You need to unlock this level before you can warp to it!\n"));
		return;
	}

	fromlevelselect = false;
	D_MapChange(newmapnum, newgametype, false, newresetplayers, 0, false, false);
}

/** Receives a map command and changes the map.
  *
  * \param cp        Data buffer.
  * \param playernum Player number responsible for the message. Should be
  *                  ::serverplayer or ::adminplayer.
  * \sa D_MapChange
  */
static void Got_Mapcmd(UINT8 **cp, INT32 playernum)
{
	char mapname[MAX_WADPATH+1];
	UINT8 flags;
	INT32 resetplayer = 1, lastgametype;
	UINT8 skipprecutscene, FLS;

	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal map change received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	if (chmappending)
		chmappending--;

	flags = READUINT8(*cp);

	ultimatemode = ((flags & 1) != 0);
	if (netgame || multiplayer)
		ultimatemode = false;

	resetplayer = ((flags & (1<<1)) == 0);

	lastgametype = gametype;
	gametype = READUINT8(*cp);

	if (gametype != lastgametype)
		D_GameTypeChanged(lastgametype); // emulate consvar_t behavior for gametype

	skipprecutscene = ((flags & (1<<2)) != 0);

	FLS = ((flags & (1<<3)) != 0);

	READSTRINGN(*cp, mapname, MAX_WADPATH);

	if (netgame)
		P_SetRandSeed(READUINT32(*cp));

	if (!skipprecutscene)
	{
		DEBFILE(va("Warping to %s [resetplayer=%d lastgametype=%d gametype=%d cpnd=%d]\n",
			mapname, resetplayer, lastgametype, gametype, chmappending));
		CONS_Printf(M_GetText("Speeding off to level...\n"));
	}
	if (demoplayback && !timingdemo)
		precache = false;

	if (resetplayer)
	{
		if (!FLS || (netgame || multiplayer))
			emeralds = 0;
	}

#ifdef HAVE_BLUA
	LUAh_MapChange();
#endif

	G_InitNew(ultimatemode, mapname, resetplayer, skipprecutscene);
	if (demoplayback && !timingdemo)
		precache = true;
	CON_ToggleOff();
	if (timingdemo)
		G_DoneLevelLoad();

	if (modeattacking)
	{
		SetPlayerSkinByNum(0, cv_chooseskin.value-1);
		players[0].skincolor = skins[players[0].skin].prefcolor;
		CV_StealthSetValue(&cv_playercolor, players[0].skincolor);

		// a copy of color
		if (players[0].mo)
			players[0].mo->color = players[0].skincolor;
	}
	if (metalrecording)
		G_BeginMetal();
	if (demorecording) // Okay, level loaded, character spawned and skinned,
		G_BeginRecording(); // I AM NOW READY TO RECORD.
	demo_start = true;
}

static void Command_Pause(void)
{
	XBOXSTATIC UINT8 buf[2];
	UINT8 *cp = buf;

	if (COM_Argc() > 1)
		WRITEUINT8(cp, (char)(atoi(COM_Argv(1)) != 0));
	else
		WRITEUINT8(cp, (char)(!paused));

	if (dedicated)
		WRITEUINT8(cp, 1);
	else
		WRITEUINT8(cp, 0);

	if (cv_pause.value || server || (adminplayer == consoleplayer))
	{
		if (modeattacking || !(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		{
			CONS_Printf(M_GetText("You can't pause here.\n"));
			return;
		}
		SendNetXCmd(XD_PAUSE, &buf, 2);
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}

static void Got_Pause(UINT8 **cp, INT32 playernum)
{
	UINT8 dedicatedpause = false;
	const char *playername;

	if (netgame && !cv_pause.value && playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal pause command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	if (modeattacking)
		return;

	paused = READUINT8(*cp);
	dedicatedpause = READUINT8(*cp);

	if (!demoplayback)
	{
		if (netgame)
		{
			if (dedicatedpause)
				playername = "SERVER";
			else
				playername = player_names[playernum];

			if (paused)
				CONS_Printf(M_GetText("Game paused by %s\n"), playername);
			else
				CONS_Printf(M_GetText("Game unpaused by %s\n"), playername);
		}

		if (paused)
		{
			if (!menuactive || netgame)
				S_PauseAudio();
		}
		else
			S_ResumeAudio();
	}
}

// Command for stuck characters in netgames, griefing, etc.
static void Command_Suicide(void)
{
	XBOXSTATIC UINT8 buf[4];
	UINT8 *cp = buf;

	WRITEINT32(cp, consoleplayer);

	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (!G_PlatformGametype())
	{
		CONS_Printf(M_GetText("You may only use this in co-op, race, and competition!\n"));
		return;
	}

	// Retry is quicker.  Probably should force people to use it.
	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("You can't use this in Single Player! Use \"retry\" instead.\n"));
		return;
	}

	SendNetXCmd(XD_SUICIDE, &buf, 4);
}

static void Got_Suicide(UINT8 **cp, INT32 playernum)
{
	INT32 suicideplayer = READINT32(*cp);

	// You can't suicide someone else.  Nice try, there.
	if (suicideplayer != playernum || (!G_PlatformGametype()))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal suicide command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	if (players[suicideplayer].mo)
		P_DamageMobj(players[suicideplayer].mo, NULL, NULL, 10000);
}

/** Deals with an ::XD_RANDOMSEED message in a netgame.
  * These messages set the position of the random number LUT and are crucial to
  * correct synchronization.
  *
  * Such a message should only ever come from the ::serverplayer. If it comes
  * from any other player, it is ignored.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer.
  * \author Graue <graue@oceanbase.org>
  */
static void Got_RandomSeed(UINT8 **cp, INT32 playernum)
{
	UINT32 seed;

	seed = READUINT32(*cp);

	if (playernum != serverplayer) // it's not from the server, wtf?
		return;

	P_SetRandSeed(seed);
}

/** Clears all players' scores in a netgame.
  * Only the server or a remote admin can use this command, for obvious reasons.
  *
  * \sa XD_CLEARSCORES, Got_Clearscores
  * \author SSNTails <http://www.ssntails.org>
  */
static void Command_Clearscores_f(void)
{
	if (!(server || (adminplayer == consoleplayer)))
		return;

	SendNetXCmd(XD_CLEARSCORES, NULL, 1);
}

/** Handles an ::XD_CLEARSCORES message, which resets all players' scores in a
  * netgame to zero.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer
  *                  or ::adminplayer.
  * \sa XD_CLEARSCORES, Command_Clearscores_f
  * \author SSNTails <http://www.ssntails.org>
  */
static void Got_Clearscores(UINT8 **cp, INT32 playernum)
{
	INT32 i;

	(void)cp;
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal clear scores command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		players[i].score = 0;

	CONS_Printf(M_GetText("Scores have been reset by the server.\n"));
}

// Team changing functions
static void Command_Teamchange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	//      0         1
	// changeteam  <color>

	if (COM_Argc() <= 1)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(1), "red") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(1), "blue") || !strcasecmp(COM_Argv(1), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(1), "playing") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "spectator or playing");
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == (unsigned)players[consoleplayer].ctfteam ||
			(players[consoleplayer].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[consoleplayer].spectator && !NetPacket.packet.newteam) ||
			(!players[consoleplayer].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You're already on that team!\n"));
		return;
	}

	if (!cv_allowteamchange.value && NetPacket.packet.newteam) // allow swapping to spectator even in locked teams.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The server is not allowing team changes at the moment.\n"));
		return;
	}

	//additional check for hide and seek. Don't allow change of status after hidetime ends.
	if (gametype == GT_HIDEANDSEEK && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void Command_Teamchange2_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	//      0         1
	// changeteam2 <color>

	if (COM_Argc() <= 1)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(1), "red") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(1), "blue") || !strcasecmp(COM_Argv(1), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(1), "playing") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}

	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam2 <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam2 <team>: switch to a new team (%s)\n"), "spectator or playing");
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == (unsigned)players[secondarydisplayplayer].ctfteam ||
			(players[secondarydisplayplayer].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[secondarydisplayplayer].spectator && !NetPacket.packet.newteam) ||
			(!players[secondarydisplayplayer].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You're already on that team!\n"));
		return;
	}

	if (!cv_allowteamchange.value && NetPacket.packet.newteam) // allow swapping to spectator even in locked teams.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The server is not allowing team changes at the moment.\n"));
		return;
	}

	//additional check for hide and seek. Don't allow change of status after hidetime ends.
	if (gametype == GT_HIDEANDSEEK && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd2(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void Command_ServerTeamChange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	if (!(server || (adminplayer == consoleplayer)))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	//        0              1         2
	// serverchangeteam <playernum>  <team>

	if (COM_Argc() < 3)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "it, notit, playing, or spectator");
		else if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_TagGametype())
	{
		if (!strcasecmp(COM_Argv(2), "it") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(2), "notit") || !strcasecmp(COM_Argv(2), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(2), "playing") || !strcasecmp(COM_Argv(2), "3"))
			NetPacket.packet.newteam = 3;
		else if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(2), "red") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(2), "blue") || !strcasecmp(COM_Argv(2), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(2), "playing") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "it, notit, playing, or spectator");
		else if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		return;
	}

	NetPacket.packet.playernum = atoi(COM_Argv(1));

	if (!playeringame[NetPacket.packet.playernum])
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %d!\n"), NetPacket.packet.playernum);
		return;
	}

	if (G_TagGametype())
	{
		if (( (players[NetPacket.packet.playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 1) ||
			(!(players[NetPacket.packet.playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 2) ||
			( players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[NetPacket.packet.playernum].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
	else if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == (unsigned)players[NetPacket.packet.playernum].ctfteam ||
			(players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[NetPacket.packet.playernum].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("That player is already on that team!\n"));
		return;
	}

	//additional check for hide and seek. Don't allow change of status after hidetime ends.
	if (gametype == GT_HIDEANDSEEK && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	NetPacket.packet.verification = true; // This signals that it's a server change

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

//todo: This and the other teamchange functions are getting too long and messy. Needs cleaning.
static void Got_Teamchange(UINT8 **cp, INT32 playernum)
{
	changeteam_union NetPacket;
	boolean error = false;
	NetPacket.value.l = NetPacket.value.b = READINT16(*cp);

	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators()) //Make sure you're in the right gametype.
	{
		// this should never happen unless the client is hacked/buggy
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
	}

	if (NetPacket.packet.verification) // Special marker that the server sent the request
	{
		if (playernum != serverplayer && (playernum != adminplayer))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
			{
				XBOXSTATIC UINT8 buf[2];

				buf[0] = (UINT8)playernum;
				buf[1] = KICK_MSG_CON_FAIL;
				SendNetXCmd(XD_KICK, &buf, 2);
			}
			return;
		}
		playernum = NetPacket.packet.playernum;
	}

	// Prevent multiple changes in one go.
	if (G_TagGametype())
	{
		if (((players[playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 1) ||
			(!(players[playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 2) ||
			(players[playernum].spectator && NetPacket.packet.newteam == 0) ||
			(!players[playernum].spectator && NetPacket.packet.newteam == 3))
			return;
	}
	else if (G_GametypeHasTeams())
	{
		if ((NetPacket.packet.newteam && (NetPacket.packet.newteam == (unsigned)players[playernum].ctfteam)) ||
			(players[playernum].spectator && !NetPacket.packet.newteam))
			return;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[playernum].spectator && NetPacket.packet.newteam == 3))
			return;
	}
	else
	{
		if (playernum != serverplayer && (playernum != adminplayer))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
			{
				XBOXSTATIC UINT8 buf[2];

				buf[0] = (UINT8)playernum;
				buf[1] = KICK_MSG_CON_FAIL;
				SendNetXCmd(XD_KICK, &buf, 2);
			}
		}
		return;
	}

	//Make sure that the right team number is sent. Keep in mind that normal clients cannot change to certain teams in certain gametypes.
	switch (gametype)
	{
	case GT_HIDEANDSEEK:
		//no status changes after hidetime
		if (leveltime >= (hidetime * TICRATE))
		{
			error = true;
			break;
		}
		/* FALLTHRU */
	case GT_TAG:
		switch (NetPacket.packet.newteam)
		{
		case 0:
			break;
		case 1: case 2:
			if (!NetPacket.packet.verification)
				error = true; //Only admin can change player's IT status' in tag.
			break;
		case 3: //Join game via console.
			if (!NetPacket.packet.verification && !cv_allowteamchange.value)
				error = true;
			break;
		}

		break;
	default:
#ifdef PARANOIA
		if (!G_GametypeHasTeams() && !G_GametypeHasSpectators())
			I_Error("Invalid gametype after initial checks!");
#endif

		if (!cv_allowteamchange.value)
		{
			if (!NetPacket.packet.verification && NetPacket.packet.newteam)
				error = true; //Only admin can change status, unless changing to spectator.
		}
		break; //Otherwise, you don't need special permissions.
	}

	if (server && ((NetPacket.packet.newteam < 0 || NetPacket.packet.newteam > 3) || error))
	{
		XBOXSTATIC UINT8 buf[2];

		buf[0] = (UINT8)playernum;
		buf[1] = KICK_MSG_CON_FAIL;
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		SendNetXCmd(XD_KICK, &buf, 2);
	}

	//Safety first!
	if (players[playernum].mo)
	{
		if (!players[playernum].spectator)
			P_DamageMobj(players[playernum].mo, NULL, NULL, 10000);
		else
		{
			P_RemoveMobj(players[playernum].mo);
			players[playernum].mo = NULL;
			players[playernum].playerstate = PST_REBORN;
		}
	}
	else
		players[playernum].playerstate = PST_REBORN;

	//Now that we've done our error checking and killed the player
	//if necessary, put the player on the correct team/status.
	if (G_TagGametype())
	{
		if (!NetPacket.packet.newteam)
		{
			players[playernum].spectator = true;
			players[playernum].pflags &= ~PF_TAGIT;
			players[playernum].pflags &= ~PF_TAGGED;
		}
		else if (NetPacket.packet.newteam != 3) // .newteam == 1 or 2.
		{
			players[playernum].spectator = false;
			players[playernum].pflags &= ~PF_TAGGED;//Just in case.

			if (NetPacket.packet.newteam == 1) //Make the player IT.
				players[playernum].pflags |= PF_TAGIT;
			else
				players[playernum].pflags &= ~PF_TAGIT;
		}
		else // Just join the game.
		{
			players[playernum].spectator = false;

			//If joining after hidetime in normal tag, default to being IT.
			if (gametype == GT_TAG && (leveltime > (hidetime * TICRATE)))
			{
				NetPacket.packet.newteam = 1; //minor hack, causes the "is it" message to be printed later.
				players[playernum].pflags |= PF_TAGIT; //make the player IT.
			}
		}
	}
	else if (G_GametypeHasTeams())
	{
		if (!NetPacket.packet.newteam)
		{
			players[playernum].ctfteam = 0;
			players[playernum].spectator = true;
		}
		else
		{
			players[playernum].ctfteam = NetPacket.packet.newteam;
			players[playernum].spectator = false;
		}
	}
	else if (G_GametypeHasSpectators())
	{
		if (!NetPacket.packet.newteam)
			players[playernum].spectator = true;
		else
			players[playernum].spectator = false;
	}

	if (NetPacket.packet.autobalance)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.scrambled)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 1)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("%s is now IT!\n"), player_names[playernum]);
		else
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 2)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("%s is no longer IT!\n"), player_names[playernum]);
		else
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 3)
		CONS_Printf(M_GetText("%s entered the game.\n"), player_names[playernum]);
	else
		CONS_Printf(M_GetText("%s became a spectator.\n"), player_names[playernum]);

	//reset view if you are changed, or viewing someone who was changed.
	if (playernum == consoleplayer || displayplayer == playernum)
		displayplayer = consoleplayer;

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam)
		{
			if (playernum == consoleplayer) //CTF and Team Match colors.
				CV_SetValue(&cv_playercolor, NetPacket.packet.newteam + 5);
			else if (playernum == secondarydisplayplayer)
				CV_SetValue(&cv_playercolor2, NetPacket.packet.newteam + 5);
		}
	}

	// Clear player score and rings if a spectator.
	if (players[playernum].spectator)
	{
		players[playernum].score = 0;
		players[playernum].health = 1;
		if (players[playernum].mo)
			players[playernum].mo->health = 1;
	}

	// In tag, check to see if you still have a game.
	if (G_TagGametype())
		P_CheckSurvivors();
}

//
// Attempts to make password system a little sane without
// rewriting the entire goddamn XD_file system
//
#include "md5.h"
static void D_MD5PasswordPass(const UINT8 *buffer, size_t len, const char *salt, void *dest)
{
#ifdef NOMD5
	(void)buffer;
	(void)len;
	(void)salt;
	memset(dest, 0, 16);
#else
	XBOXSTATIC char tmpbuf[256];
	const size_t sl = strlen(salt);

	if (len > 256-sl)
		len = 256-sl;
	memcpy(tmpbuf, buffer, len);
	memmove(&tmpbuf[len], salt, sl);
	//strcpy(&tmpbuf[len], salt);
	len += strlen(salt);
	if (len < 256)
		memset(&tmpbuf[len],0,256-len);

	// Yes, we intentionally md5 the ENTIRE buffer regardless of size...
	md5_buffer(tmpbuf, 256, dest);
#endif
}

#define BASESALT "basepasswordstorage"
static UINT8 adminpassmd5[16];
static boolean adminpasswordset = false;

void D_SetPassword(const char *pw)
{
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &adminpassmd5);
	adminpasswordset = true;
}

// Remote Administration
static void Command_Changepassword_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	if (client) // cannot change remotely
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("password <password>: change remote admin password\n"));
		return;
	}

	D_SetPassword(COM_Argv(1));
	CONS_Printf(M_GetText("Password set.\n"));
#endif
}

static void Command_Login_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	XBOXSTATIC UINT8 finalmd5[16];
	const char *pw;

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("login <password>: Administrator login\n"));
		return;
	}

	pw = COM_Argv(1);

	// Do the base pass to get what the server has (or should?)
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &finalmd5);

	// Do the final pass to get the comparison the server will come up with
	D_MD5PasswordPass(finalmd5, 16, va("PNUM%02d", consoleplayer), &finalmd5);

	CONS_Printf(M_GetText("Sending login... (Notice only given if password is correct.)\n"));

	SendNetXCmd(XD_LOGIN, finalmd5, 16);
#endif
}

static void Got_Login(UINT8 **cp, INT32 playernum)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	(void)cp;
	(void)playernum;
#else
	UINT8 sentmd5[16], finalmd5[16];

	READMEM(*cp, sentmd5, 16);

	if (client)
		return;

	if (!adminpasswordset)
	{
		CONS_Printf(M_GetText("Password from %s failed (no password set).\n"), player_names[playernum]);
		return;
	}

	// Do the final pass to compare with the sent md5
	D_MD5PasswordPass(adminpassmd5, 16, va("PNUM%02d", playernum), &finalmd5);

	if (!memcmp(sentmd5, finalmd5, 16))
	{
		CONS_Printf(M_GetText("%s passed authentication.\n"), player_names[playernum]);
		COM_BufInsertText(va("verify %d\n", playernum)); // do this immediately
	}
	else
		CONS_Printf(M_GetText("Password from %s failed.\n"), player_names[playernum]);
#endif
}

static void Command_Verify_f(void)
{
	XBOXSTATIC char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("verify <node>: give admin privileges to a node\n"));
		return;
	}

	strlcpy(buf, COM_Argv(1), sizeof (buf));

	playernum = atoi(buf);

	temp = buf;

	WRITEUINT8(temp, playernum);

	if (playeringame[playernum])
		SendNetXCmd(XD_VERIFIED, buf, 1);
}

static void Got_Verification(UINT8 **cp, INT32 playernum)
{
	INT16 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal verification received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	adminplayer = num;

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are now a server administrator.\n"));
}

static void Command_MotD_f(void)
{
	size_t i, j;
	char *mymotd;

	if ((j = COM_Argc()) < 2)
	{
		CONS_Printf(M_GetText("motd <message>: Set a message that clients see upon join.\n"));
		return;
	}

	if (!(server || (adminplayer == consoleplayer)))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);

	strlcpy(mymotd, COM_Argv(1), sizeof motd);
	for (i = 2; i < j; i++)
	{
		strlcat(mymotd, " ", sizeof motd);
		strlcat(mymotd, COM_Argv(i), sizeof motd);
	}

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
		{
			Z_Free(mymotd);
			return;
		}

	if ((netgame || multiplayer) && client)
		SendNetXCmd(XD_SETMOTD, mymotd, sizeof(motd));
	else
	{
		strcpy(motd, mymotd);
		CONS_Printf(M_GetText("Message of the day set.\n"));
	}

	Z_Free(mymotd);
}

static void Got_MotD_f(UINT8 **cp, INT32 playernum)
{
	char *mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);
	INT32 i;
	boolean kick = false;

	READSTRINGN(*cp, mymotd, sizeof(motd));

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
			kick = true;

	if ((playernum != serverplayer && playernum != adminplayer) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal motd change received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}

		Z_Free(mymotd);
		return;
	}

	strcpy(motd, mymotd);

	CONS_Printf(M_GetText("Message of the day set.\n"));

	Z_Free(mymotd);
}

static void Command_RunSOC(void)
{
	const char *fn;
	XBOXSTATIC char buf[255];
	size_t length = 0;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("runsoc <socfile.soc> or <lumpname>: run a soc\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	if (netgame && !(server || consoleplayer == adminplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (!(netgame || multiplayer))
	{
		if (!P_RunSOC(fn))
			CONS_Printf(M_GetText("Could not find SOC.\n"));
		else
			G_SetGameModified(multiplayer);
		return;
	}

	nameonly(strcpy(buf, fn));
	length = strlen(buf)+1;

	SendNetXCmd(XD_RUNSOC, buf, length);
}

static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTFOUND;

	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal runsoc command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	READSTRINGN(*cp, filename, 255);

	// Maybe add md5 support?
	if (strstr(filename, ".soc") != NULL)
	{
		ncs = findfile(filename,NULL,true);

		if (ncs != FS_FOUND)
		{
			Command_ExitGame_f();
			if (ncs == FS_NOTFOUND)
			{
				CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server.\n"), filename);
				M_StartMessage(va("The server added a file\n(%s)\nthat you do not have.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
			}
			else
			{
				CONS_Printf(M_GetText("Unknown error finding soc file (%s) the server added.\n"), filename);
				M_StartMessage(va("Unknown error trying to load a file\nthat the server added\n(%s).\n\nPress ESC\n",filename), NULL, MM_NOTHING);
			}
			return;
		}
	}

	P_RunSOC(filename);
	G_SetGameModified(true);
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  */
static void Command_Addfile(void)
{
	const char *fn, *p;
	XBOXSTATIC char buf[256];
	char *buf_p = buf;
	INT32 i;
	int musiconly; // W_VerifyNMUSlumps isn't boolean

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("addfile <wadfile.wad>: load wad file\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	// Disallow non-printing characters and semicolons.
	for (i = 0; fn[i] != '\0'; i++)
		if (!isprint(fn[i]) || fn[i] == ';')
			return;

	musiconly = W_VerifyNMUSlumps(fn);

	if (!musiconly)
	{
		// ... But only so long as they contain nothing more then music and sprites.
		if (netgame && !(server || adminplayer == consoleplayer))
		{
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
			return;
		}
		G_SetGameModified(multiplayer);
	}

	// Add file on your client directly if it is trivial, or you aren't in a netgame.
	if (!(netgame || multiplayer) || musiconly)
	{
		P_AddWadFile(fn, NULL);
		return;
	}

	p = fn+strlen(fn);
	while(--p >= fn)
		if (*p == '\\' || *p == '/' || *p == ':')
			break;
	++p;
	// check total packet size and no of files currently loaded
	{
		size_t packetsize = 0;
		serverinfo_pak *dummycheck = NULL;

		// Shut the compiler up.
		(void)dummycheck;

		// See W_LoadWadFile in w_wad.c
		for (i = 0; i < numwadfiles; i++)
			packetsize += nameonlylength(wadfiles[i]->filename) + 22;

		packetsize += nameonlylength(fn) + 22;

		if ((numwadfiles >= MAX_WADFILES)
		|| (packetsize > sizeof(dummycheck->fileneeded)))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Too many files loaded to add %s\n"), fn);
			return;
		}
	}

	WRITESTRINGN(buf_p,p,240);

	// calculate and check md5
	{
		UINT8 md5sum[16];
#ifdef NOMD5
		memset(md5sum,0,16);
#else
		FILE *fhandle;

		if ((fhandle = W_OpenWadFile(&fn, true)) != NULL)
		{
			tic_t t = I_GetTime();
			CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",fn);
			md5_stream(fhandle, md5sum);
			CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f second\n", fn, (float)(I_GetTime() - t)/TICRATE);
			fclose(fhandle);
		}
		else // file not found
			return;

		for (i = 0; i < numwadfiles; i++)
		{
			if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
			{
				CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), fn);
				return;
			}
		}
#endif
		WRITEMEM(buf_p, md5sum, 16);
	}

	if (adminplayer == consoleplayer) // Request to add file
		SendNetXCmd(XD_REQADDFILE, buf, buf_p - buf);
	else
		SendNetXCmd(XD_ADDFILE, buf, buf_p - buf);
}

#ifdef DELFILE
/** removes the last added pwad at runtime.
  * Searches for sounds, maps, music and images to remove
  */
static void Command_Delfile(void)
{
	if (gamestate == GS_LEVEL)
	{
		CONS_Printf(M_GetText("You must NOT be in a level to use this.\n"));
		return;
	}

	if (netgame && !(server || adminplayer == consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (numwadfiles <= mainwads)
	{
		CONS_Printf(M_GetText("No additional WADs are loaded.\n"));
		return;
	}

	if (!(netgame || multiplayer))
	{
		P_DelWadFile();
		if (mainwads == numwadfiles && modifiedgame)
			modifiedgame = false;
		return;
	}

	SendNetXCmd(XD_DELFILE, NULL, 0);
}
#endif

static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTFOUND;
	UINT8 md5sum[16];
	boolean kick = false;
	boolean toomany = false;
	INT32 i;
	size_t packetsize = 0;
	serverinfo_pak *dummycheck = NULL;

	// Shut the compiler up.
	(void)dummycheck;

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	// Only the server processes this message.
	if (client)
		return;

	// Disallow non-printing characters and semicolons.
	for (i = 0; filename[i] != '\0'; i++)
		if (!isprint(filename[i]) || filename[i] == ';')
			kick = true;

	if ((playernum != serverplayer && playernum != adminplayer) || kick)
	{
		XBOXSTATIC UINT8 buf[2];

		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);

		buf[0] = (UINT8)playernum;
		buf[1] = KICK_MSG_CON_FAIL;
		SendNetXCmd(XD_KICK, &buf, 2);
		return;
	}

	// See W_LoadWadFile in w_wad.c
	for (i = 0; i < numwadfiles; i++)
		packetsize += nameonlylength(wadfiles[i]->filename) + 22;

	packetsize += nameonlylength(filename) + 22;

	if ((numwadfiles >= MAX_WADFILES)
	|| (packetsize > sizeof(dummycheck->fileneeded)))
		toomany = true;
	else
		ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || toomany)
	{
		char message[256];

		if (toomany)
			sprintf(message, M_GetText("Too many files loaded to add %s\n"), filename);
		else if (ncs == FS_NOTFOUND)
			sprintf(message, M_GetText("The server doesn't have %s\n"), filename);
		else if (ncs == FS_MD5SUMBAD)
			sprintf(message, M_GetText("Checksum mismatch on %s\n"), filename);
		else
			sprintf(message, M_GetText("Unknown error finding wad file (%s)\n"), filename);

		CONS_Printf("%s",message);

		if (adminplayer)
			COM_BufAddText(va("sayto %d %s", adminplayer, message));

		return;
	}

	COM_BufAddText(va("addfile %s\n", filename));
}

#ifdef DELFILE
static void Got_Delfilecmd(UINT8 **cp, INT32 playernum)
{
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal delfile command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}
	(void)cp;

	if (numwadfiles <= mainwads) //sanity
		return;

	P_DelWadFile();
	if (mainwads == numwadfiles && modifiedgame)
		modifiedgame = false;
}
#endif

static void Got_Addfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTFOUND;
	UINT8 md5sum[16];

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || !P_AddWadFile(filename, NULL))
	{
		Command_ExitGame_f();
		if (ncs == FS_FOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you have too many files added.\nRestart the game to clear loaded files\nand play on this server."), filename);
			M_StartMessage(va("The server added a file \n(%s)\nbut you have too many files added.\nRestart the game to clear loaded files.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else if (ncs == FS_NOTFOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server."), filename);
			M_StartMessage(va("The server added a file \n(%s)\nthat you do not have.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else if (ncs == FS_MD5SUMBAD)
		{
			CONS_Printf(M_GetText("Checksum mismatch while loading %s.\nMake sure you have the copy of\nthis file that the server has.\n"), filename);
			M_StartMessage(va("Checksum mismatch while loading \n%s.\nThe server seems to have a\ndifferent version of this file.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else
		{
			CONS_Printf(M_GetText("Unknown error finding wad file (%s) the server added.\n"), filename);
			M_StartMessage(va("Unknown error trying to load a file\nthat the server added \n(%s).\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		return;
	}

	G_SetGameModified(true);
}

static void Command_ListWADS_f(void)
{
	INT32 i = numwadfiles;
	char *tempname;
	CONS_Printf(M_GetText("There are %d wads loaded:\n"),numwadfiles);
	for (i--; i >= 0; i--)
	{
		nameonly(tempname = va("%s", wadfiles[i]->filename));
		if (!i)
			CONS_Printf("\x82 IWAD\x80: %s\n", tempname);
		else if (i <= mainwads)
			CONS_Printf("\x82 * %.2d\x80: %s\n", i, tempname);
		else
			CONS_Printf("   %.2d: %s\n", i, tempname);
	}
}

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

/** Prints program version.
  */
static void Command_Version_f(void)
{
#ifdef DEVELOP
	CONS_Printf("Sonic Robo Blast 2 %s-%s (%s %s)\n", compbranch, comprevision, compdate, comptime);
#else
	CONS_Printf("Sonic Robo Blast 2 %s (%s %s %s)\n", VERSIONSTRING, compdate, comptime, comprevision);
#endif
}

#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void)
{
	CONS_Printf(M_GetText("Mod ID: %d\nMod Version: %d\nCode Base:%d\n"), MODID, MODVERSION, CODEBASE);
}
#endif

// Returns current gametype being used.
//
static void Command_ShowGametype_f(void)
{
	INT32 j;
	const char *gametypestr = NULL;

	if (!(netgame || multiplayer)) // print "Single player" instead of "Co-op"
	{
		CONS_Printf(M_GetText("Current gametype is %s\n"), M_GetText("Single player"));
		return;
	}
	// find name string for current gametype
	for (j = 0; gametype_cons_t[j].strvalue; j++)
	{
		if (gametype_cons_t[j].value == gametype)
		{
			gametypestr = gametype_cons_t[j].strvalue;
			break;
		}
	}
	if (gametypestr)
		CONS_Printf(M_GetText("Current gametype is %s\n"), gametypestr);
	else // string for current gametype was not found above (should never happen)
		CONS_Printf(M_GetText("Unknown gametype set (%d)\n"), gametype);
}

/** Plays the intro.
  */
static void Command_Playintro_f(void)
{
	if (netgame)
		return;

	F_StartIntro();
}

/** Quits the game immediately.
  */
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void)
{
	I_Quit();
}

void ItemFinder_OnChange(void)
{
	if (!cv_itemfinder.value)
		return; // it's fine.

	if (!M_SecretUnlocked(SECRET_ITEMFINDER))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
	else if (netgame || multiplayer)
	{
		CONS_Printf(M_GetText("This only works in single player.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
}

/** Deals with a pointlimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the pointlimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * We don't check immediately for the pointlimit having been reached,
  * because you would get "caught" when turning it up in the menu.
  * \sa cv_pointlimit, TimeLimit_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void PointLimit_OnChange(void)
{
	// Don't allow pointlimit in Single Player/Co-Op/Race!
	if (server && Playing() && G_PlatformGametype())
	{
		if (cv_pointlimit.value)
			CV_StealthSetValue(&cv_pointlimit, 0);
		return;
	}

	if (cv_pointlimit.value)
	{
		CONS_Printf(M_GetText("Levels will end after %s scores %d point%s.\n"),
			G_GametypeHasTeams() ? M_GetText("a team") : M_GetText("someone"),
			cv_pointlimit.value,
			cv_pointlimit.value > 1 ? "s" : "");
	}
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("Point limit disabled\n"));
}

static void NumLaps_OnChange(void)
{
	// Just don't be verbose
	if (gametype == GT_RACE)
		CONS_Printf(M_GetText("Number of laps set to %d\n"), cv_numlaps.value);
}

static void NetTimeout_OnChange(void)
{
	connectiontimeout = (tic_t)cv_nettimeout.value;
}

static void JoinTimeout_OnChange(void)
{
	jointimeout = (tic_t)cv_jointimeout.value;
}

UINT32 timelimitintics = 0;

/** Deals with a timelimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the timelimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_timelimit, PointLimit_OnChange
  */
static void TimeLimit_OnChange(void)
{
	// Don't allow timelimit in Single Player/Co-Op/Race!
	if (server && Playing() && cv_timelimit.value != 0 && G_PlatformGametype())
	{
		CV_SetValue(&cv_timelimit, 0);
		return;
	}

	if (cv_timelimit.value != 0)
	{
		CONS_Printf(M_GetText("Levels will end after %d minute%s.\n"),cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s"); // Graue 11-17-2003
		timelimitintics = cv_timelimit.value * 60 * TICRATE;

		//add hidetime for tag too!
		if (G_TagGametype())
			timelimitintics += hidetime * TICRATE;

		// Note the deliberate absence of any code preventing
		//   pointlimit and timelimit from being set simultaneously.
		// Some people might like to use them together. It works.
	}
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("Time limit disabled\n"));
}

/** Adjusts certain settings to match a changed gametype.
  *
  * \param lastgametype The gametype we were playing before now.
  * \sa D_MapChange
  * \author Graue <graue@oceanbase.org>
  * \todo Get rid of the hardcoded stuff, ugly stuff, etc.
  */
void D_GameTypeChanged(INT32 lastgametype)
{
	if (netgame)
	{
		INT32 j;
		const char *oldgt = NULL, *newgt = NULL;
		for (j = 0; gametype_cons_t[j].strvalue; j++)
		{
			if (gametype_cons_t[j].value == lastgametype)
				oldgt = gametype_cons_t[j].strvalue;
			if (gametype_cons_t[j].value == gametype)
				newgt = gametype_cons_t[j].strvalue;
		}
		if (oldgt && newgt)
			CONS_Printf(M_GetText("Gametype was changed from %s to %s\n"), oldgt, newgt);
	}
	// Only do the following as the server, not as remote admin.
	// There will always be a server, and this only needs to be done once.
	if (server && (multiplayer || netgame))
	{
		if (gametype == GT_COMPETITION || gametype == GT_COOP)
			CV_SetValue(&cv_itemrespawn, 0);
		else if (!cv_itemrespawn.changed)
			CV_SetValue(&cv_itemrespawn, 1);

		switch (gametype)
		{
			case GT_MATCH:
			case GT_TEAMMATCH:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for match: timelimit 10 mins, no pointlimit
					CV_SetValue(&cv_pointlimit, 0);
					CV_SetValue(&cv_timelimit, 10);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
			case GT_TAG:
			case GT_HIDEANDSEEK:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for tag: 5 mins, no pointlimit
					// Note that tag mode also uses an alternate timing mechanism in tandem with timelimit.
					CV_SetValue(&cv_timelimit, 5);
					CV_SetValue(&cv_pointlimit, 0);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
			case GT_CTF:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for CTF: no timelimit, pointlimit 5
					CV_SetValue(&cv_timelimit, 0);
					CV_SetValue(&cv_pointlimit, 5);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
		}
	}
	else if (!multiplayer && !netgame)
	{
		gametype = GT_COOP;
		// These shouldn't matter anymore
		//CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue);
		//CV_SetValue(&cv_itemrespawn, 0);
	}

	// reset timelimit and pointlimit in race/coop, prevent stupid cheats
	if (server)
	{
		if (G_PlatformGametype())
		{
			if (cv_timelimit.value)
				CV_SetValue(&cv_timelimit, 0);
			if (cv_pointlimit.value)
				CV_SetValue(&cv_pointlimit, 0);
		}
		else if ((cv_pointlimit.changed || cv_timelimit.changed) && cv_pointlimit.value)
		{
			if (lastgametype != GT_CTF && gametype == GT_CTF)
				CV_SetValue(&cv_pointlimit, cv_pointlimit.value / 500);
			else if (lastgametype == GT_CTF && gametype != GT_CTF)
				CV_SetValue(&cv_pointlimit, cv_pointlimit.value * 500);
		}
	}

	// When swapping to a gametype that supports spectators,
	// make everyone a spectator initially.
	if (!splitscreen && (G_GametypeHasSpectators()))
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				players[i].ctfteam = 0;
				players[i].spectator = true;
			}
	}

	// don't retain teams in other modes or between changes from ctf to team match.
	// also, stop any and all forms of team scrambling that might otherwise take place.
	if (G_GametypeHasTeams())
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].ctfteam = 0;

		if (server || (adminplayer == consoleplayer))
		{
			CV_StealthSetValue(&cv_teamscramble, 0);
			teamscramble = 0;
		}
	}
}

static void Ringslinger_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA) && !netgame && cv_ringslinger.value && !cv_debug)
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_ringslinger, 0);
		return;
	}

	if (cv_ringslinger.value) // Only if it's been turned on
		G_SetGameModified(multiplayer);
}

static void Gravity_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA) && !netgame && !cv_debug
		&& strcmp(cv_gravity.string, cv_gravity.defaultvalue))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}
#ifndef NETGAME_GRAVITY
	if(netgame)
	{
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}
#endif

	if (!CV_IsSetToDefault(&cv_gravity))
		G_SetGameModified(multiplayer);
	gravity = cv_gravity.value;
}

static void SoundTest_OnChange(void)
{
	if (cv_soundtest.value < 0)
	{
		CV_SetValue(&cv_soundtest, NUMSFX-1);
		return;
	}

	if (cv_soundtest.value >= NUMSFX)
	{
		CV_SetValue(&cv_soundtest, 0);
		return;
	}

	S_StopSounds();
	S_StartSound(NULL, cv_soundtest.value);
}

static void AutoBalance_OnChange(void)
{
	autobalance = (INT16)cv_autobalance.value;
}

static void TeamScramble_OnChange(void)
{
	INT16 i = 0, j = 0, playercount = 0;
	boolean repick = true;
	INT32 blue = 0, red = 0;
	INT32 maxcomposition = 0;
	INT16 newteam = 0;
	INT32 retries = 0;
	boolean success = false;

	// Don't trigger outside level or intermission!
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		return;

	if (!cv_teamscramble.value)
		teamscramble = 0;

	if (!G_GametypeHasTeams() && (server || consoleplayer == adminplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		CV_StealthSetValue(&cv_teamscramble, 0);
		return;
	}

	// If a team scramble is already in progress, do not allow another one to be started!
	if (teamscramble)
		return;

retryscramble:

	// Clear related global variables. These will get used again in p_tick.c/y_inter.c as the teams are scrambled.
	memset(&scrambleplayers, 0, sizeof(scrambleplayers));
	memset(&scrambleteams, 0, sizeof(scrambleplayers));
	scrambletotal = scramblecount = 0;
	blue = red = maxcomposition = newteam = playercount = 0;
	repick = true;

	// Put each player's node in the array.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			scrambleplayers[playercount] = i;
			playercount++;
		}
	}

	if (playercount < 2)
	{
		CV_StealthSetValue(&cv_teamscramble, 0);
		return; // Don't scramble one or zero players.
	}

	// Randomly place players on teams.
	if (cv_teamscramble.value == 1)
	{
		maxcomposition = playercount / 2;

		// Now randomly assign players to teams.
		// If the teams get out of hand, assign the rest to the other team.
		for (i = 0; i < playercount; i++)
		{
			if (repick)
				newteam = (INT16)((M_RandomByte() % 2) + 1);

			// One team has the most players they can get, assign the rest to the other team.
			if (red == maxcomposition || blue == maxcomposition)
			{
				if (red == maxcomposition)
					newteam = 2;
				else if (blue == maxcomposition)
					newteam = 1;

				repick = false;
			}

			scrambleteams[i] = newteam;

			if (newteam == 1)
				red++;
			else
				blue++;
		}
	}
	else if (cv_teamscramble.value == 2) // Same as before, except split teams based on current score.
	{
		// Now, sort the array based on points scored.
		for (i = 1; i < playercount; i++)
		{
			for (j = i; j < playercount; j++)
			{
				INT16 tempplayer = 0;

				if ((players[scrambleplayers[i-1]].score > players[scrambleplayers[j]].score))
				{
					tempplayer = scrambleplayers[i-1];
					scrambleplayers[i-1] = scrambleplayers[j];
					scrambleplayers[j] = tempplayer;
				}
			}
		}

		// Now assign players to teams based on score. Scramble in pairs.
		// If there is an odd number, one team will end up with the unlucky slob who has no points. =(
		for (i = 0; i < playercount; i++)
		{
			if (repick)
			{
				newteam = (INT16)((M_RandomByte() % 2) + 1);
				repick = false;
			}
			else
			{
				// We will only randomly pick the team for the first guy.
				// Otherwise, just alternate back and forth, distributing players.
				if (newteam == 1)
					newteam = 2;
				else
					newteam = 1;
			}

			scrambleteams[i] = newteam;
		}
	}

	// Check to see if our random selection actually
	// changed anybody. If not, we run through and try again.
	for (i = 0; i < playercount; i++)
	{
		if (players[scrambleplayers[i]].ctfteam != scrambleteams[i])
			success = true;
	}

	if (!success && retries < 5)
	{
		retries++;
		goto retryscramble; //try again
	}

	// Display a witty message, but only during scrambles specifically triggered by an admin.
	if (cv_teamscramble.value)
	{
		scrambletotal = playercount;
		teamscramble = (INT16)cv_teamscramble.value;

		if (!(gamestate == GS_INTERMISSION && cv_scrambleonchange.value))
			CONS_Printf(M_GetText("Teams will be scrambled next round.\n"));
	}
}

static void Hidetime_OnChange(void)
{
	if (Playing() && G_TagGametype() && leveltime >= (hidetime * TICRATE))
	{
		// Don't allow hidetime changes after it expires.
		CV_StealthSetValue(&cv_hidetime, hidetime);
		return;
	}
	hidetime = cv_hidetime.value;

	//uh oh, gotta change timelimitintics now too
	if (G_TagGametype())
		timelimitintics = (cv_timelimit.value * 60 * TICRATE) + (hidetime * TICRATE);
}

static void Command_Showmap_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		if (mapheaderinfo[gamemap-1]->actnum)
			CONS_Printf("%s (%d): %s %d\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum);
		else
			CONS_Printf("%s (%d): %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl);
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_Mapmd5_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		INT32 i;
		char md5tmp[33];
		for (i = 0; i < 16; ++i)
			sprintf(&md5tmp[i*2], "%02x", mapmd5[i]);
		CONS_Printf("%s: %s\n", G_BuildMapName(gamemap), md5tmp);
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_ExitLevel_f(void)
{
	if (!(netgame || (multiplayer && gametype != GT_COOP)) && !cv_debug)
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
	else if (!(server || (adminplayer == consoleplayer)))
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
	else if (gamestate != GS_LEVEL || demoplayback)
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum)
{
	(void)cp;

	// Ignore duplicate XD_EXITLEVEL commands.
	if (gameaction == ga_completed)
		return;

	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal exitlevel command received from %s\n"), player_names[playernum]);
		if (server)
		{
			XBOXSTATIC UINT8 buf[2];

			buf[0] = (UINT8)playernum;
			buf[1] = KICK_MSG_CON_FAIL;
			SendNetXCmd(XD_KICK, &buf, 2);
		}
		return;
	}

	G_ExitLevel();
}

/** Prints the number of the displayplayer.
  *
  * \todo Possibly remove this; it was useful for debugging at one point.
  */
static void Command_Displayplayer_f(void)
{
	CONS_Printf(M_GetText("Displayplayer is %d\n"), displayplayer);
}

static void Command_Tunes_f(void)
{
	const char *tunearg;
	UINT16 tunenum, track = 0;
	const size_t argc = COM_Argc();

	if (argc < 2) //tunes slot ...
	{
		CONS_Printf("tunes <name/num> [track] [speed] / <-show> / <-default> / <-none>:\n");
		CONS_Printf(M_GetText("Play an arbitrary music lump. If a map number is used, 'MAP##M' is played.\n"));
		CONS_Printf(M_GetText("If the format supports multiple songs, you can specify which one to play.\n\n"));
		CONS_Printf(M_GetText("* With \"-show\", shows the currently playing tune and track.\n"));
		CONS_Printf(M_GetText("* With \"-default\", returns to the default music for the map.\n"));
		CONS_Printf(M_GetText("* With \"-none\", any music playing will be stopped.\n"));
		return;
	}

	tunearg = COM_Argv(1);
	tunenum = (UINT16)atoi(tunearg);
	track = 0;

	if (!strcasecmp(tunearg, "-show"))
	{
		CONS_Printf(M_GetText("The current tune is: %s [track %d]\n"),
			mapmusname, (mapmusflags & MUSIC_TRACKMASK));
		return;
	}
	if (!strcasecmp(tunearg, "-none"))
	{
		S_StopMusic();
		return;
	}
	else if (!strcasecmp(tunearg, "-default"))
	{
		tunearg = mapheaderinfo[gamemap-1]->musname;
		track = mapheaderinfo[gamemap-1]->mustrack;
	}
	else if (!tunearg[2] && toupper(tunearg[0]) >= 'A' && toupper(tunearg[0]) <= 'Z')
		tunenum = (UINT16)M_MapNumber(tunearg[0], tunearg[1]);

	if (tunenum && tunenum >= 1036)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Valid music slots are 1 to 1035.\n"));
		return;
	}
	if (!tunenum && strlen(tunearg) > 6) // This is automatic -- just show the error just in case
		CONS_Alert(CONS_NOTICE, M_GetText("Music name too long - truncated to six characters.\n"));

	if (argc > 2)
		track = (UINT16)atoi(COM_Argv(2))-1;

	if (tunenum)
		snprintf(mapmusname, 7, "%sM", G_BuildMapName(tunenum));
	else
		strncpy(mapmusname, tunearg, 7);
	mapmusname[6] = 0;
	mapmusflags = (track & MUSIC_TRACKMASK);

	S_ChangeMusic(mapmusname, mapmusflags, true);

	if (argc > 3)
	{
		float speed = (float)atof(COM_Argv(3));
		if (speed > 0.0f)
			S_SpeedMusic(speed);
	}
}

static void Command_RestartAudio_f(void)
{
	if (dedicated)  // No point in doing anything if game is a dedicated server.
		return;

	S_StopMusic();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_StartupSound();
	I_InitMusic();

// These must be called or no sound and music until manually set.

	I_SetSfxVolume(cv_soundvolume.value);
	I_SetDigMusicVolume(cv_digmusicvolume.value);
	I_SetMIDIMusicVolume(cv_midimusicvolume.value);
	if (Playing()) // Gotta make sure the player is in a level
		P_RestoreMusic(&players[consoleplayer]);

}

/** Quits a game and returns to the title screen.
  *
  */
void Command_ExitGame_f(void)
{
	INT32 i;

	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	cv_debug = 0;
	emeralds = 0;

	if (!modeattacking)
		D_StartTitle();
}

void Command_Retry_f(void)
{
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("This only works in single player.\n"));
	else if (!&players[consoleplayer] || players[consoleplayer].lives <= 1)
		CONS_Printf(M_GetText("You can't retry without any lives remaining!\n"));
	else if (G_IsSpecialStage(gamemap))
		CONS_Printf(M_GetText("You can't retry special stages!\n"));
	else
	{
		M_ClearMenus(true);
		G_SetRetryFlag();
	}
}

#ifdef NETGAME_DEVMODE
// Allow the use of devmode in netgames.
static void Fishcake_OnChange(void)
{
	cv_debug = cv_fishcake.value;
	// consvar_t's get changed to default when registered
	// so don't make modifiedgame always on!
	if (cv_debug)
	{
		G_SetGameModified(multiplayer);
	}

	else if (cv_debug != cv_fishcake.value)
		CV_SetValue(&cv_fishcake, cv_debug);
}
#endif

/** Reports to the console whether or not the game has been modified.
  *
  * \todo Make it obvious, so a console command won't be necessary.
  * \sa modifiedgame
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Isgamemodified_f(void)
{
	if (savemoddata)
		CONS_Printf(M_GetText("modifiedgame is true, but you can save emblem and time data in this mod.\n"));
	else if (modifiedgame)
		CONS_Printf(M_GetText("modifiedgame is true, secrets will not be unlocked\n"));
	else
		CONS_Printf(M_GetText("modifiedgame is false, you can unlock secrets\n"));
}

static void Command_Cheats_f(void)
{
	if (COM_CheckParm("off"))
	{
		if (!(server || (adminplayer == consoleplayer)))
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		else
			CV_ResetCheatNetVars();
		return;
	}

	if (CV_CheatsEnabled())
	{
		CONS_Printf(M_GetText("At least one CHEAT-marked variable has been changed -- Cheats are enabled.\n"));
		if (server || (adminplayer == consoleplayer))
			CONS_Printf(M_GetText("Type CHEATS OFF to reset all cheat variables to default.\n"));
	}
	else
		CONS_Printf(M_GetText("No CHEAT-marked variables are changed -- Cheats are disabled.\n"));
}

#ifdef _DEBUG
static void Command_Togglemodified_f(void)
{
	modifiedgame = !modifiedgame;
}

#ifdef HAVE_BLUA
extern UINT8 *save_p;
static void Command_Archivetest_f(void)
{
	UINT8 *buf;
	UINT32 i, wrote;
	thinker_t *th;
	if (gamestate != GS_LEVEL)
	{
		CONS_Printf("This command only works in-game, you dummy.\n");
		return;
	}

	// assign mobjnum
	i = 1;
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
		if (th->function.acp1 == (actionf_p1)P_MobjThinker)
			((mobj_t *)th)->mobjnum = i++;

	// allocate buffer
	buf = save_p = ZZ_Alloc(1024);

	// test archive
	CONS_Printf("LUA_Archive...\n");
	LUA_Archive();
	WRITEUINT8(save_p, 0x7F);
	wrote = (UINT32)(save_p-buf);

	// clear Lua state, so we can really see what happens!
	CONS_Printf("Clearing state!\n");
	LUA_ClearExtVars();

	// test unarchive
	save_p = buf;
	CONS_Printf("LUA_UnArchive...\n");
	LUA_UnArchive();
	i = READUINT8(save_p);
	if (i != 0x7F || wrote != (UINT32)(save_p-buf))
		CONS_Printf("Savegame corrupted. (write %u, read %u)\n", wrote, (UINT32)(save_p-buf));

	// free buffer
	Z_Free(buf);
	CONS_Printf("Done. No crash.\n");
}
#endif
#endif

/** Makes a change to ::cv_forceskin take effect immediately.
  *
  * \todo Move the enforcement code out of SendNameAndColor() so this hack
  *       isn't needed.
  * \sa Command_SetForcedSkin_f, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void ForceSkin_OnChange(void)
{
	if ((server || adminplayer == consoleplayer) && (cv_forceskin.value < -1 || cv_forceskin.value >= numskins))
	{
		if (cv_forceskin.value == -2)
			CV_SetValue(&cv_forceskin, numskins-1);
		else
		{
			// hack because I can't restrict this and still allow added skins to be used with forceskin.
			if (!menuactive)
				CONS_Printf(M_GetText("Valid skin numbers are 0 to %d (-1 disables)\n"), numskins - 1);
			CV_SetValue(&cv_forceskin, -1);
		}
		return;
	}

	// NOT in SP, silly!
	if (!(netgame || multiplayer))
		return;

	if (cv_forceskin.value < 0)
		CONS_Printf("The server has lifted the forced skin restrictions.\n");
	else
	{
		CONS_Printf("The server is restricting all players to skin \"%s\".\n",skins[cv_forceskin.value].name);
		ForceAllSkins(cv_forceskin.value);
	}
}

//Allows the player's name to be changed if cv_mute is off.
static void Name_OnChange(void)
{
	if (cv_mute.value && !(server || adminplayer == consoleplayer))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
	}
	else
		SendNameAndColor();

}

static void Name2_OnChange(void)
{
	if (cv_mute.value) //Secondary player can't be admin.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername2, player_names[secondarydisplayplayer]);
	}
	else
		SendNameAndColor2();
}

/** Sends a skin change for the console player, unless that player is moving.
  * \sa cv_skin, Skin2_OnChange, Color_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want

	if (!(cv_debug || devparm) && !(multiplayer || netgame) // In single player.
		&& (gamestate != GS_WAITINGPLAYERS)) // allows command line -warp x +skin y
	{
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
		return;
	}

	if (CanChangeSkin(consoleplayer) && !P_PlayerMoving(consoleplayer))
		SendNameAndColor();
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
	}
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_skin2, Skin_OnChange, Color2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin2_OnChange(void)
{
	if (!Playing() || !splitscreen)
		return; // do whatever you want

	if (CanChangeSkin(secondarydisplayplayer) && !P_PlayerMoving(secondarydisplayplayer))
		SendNameAndColor2();
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin2, skins[players[secondarydisplayplayer].skin].name);
	}
}

/** Sends a color change for the console player, unless that player is moving.
  * \sa cv_playercolor, Color2_OnChange, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want

	if (!(cv_debug || devparm) && !(multiplayer || netgame)) // In single player.
	{
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
		return;
	}

	if (!P_PlayerMoving(consoleplayer))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor();
	}
	else
	{
		CV_StealthSetValue(&cv_playercolor,
			players[consoleplayer].skincolor);
	}
}

/** Sends a color change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_playercolor2, Color_OnChange, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color2_OnChange(void)
{
	if (!Playing() || !splitscreen)
		return; // do whatever you want

	if (!P_PlayerMoving(secondarydisplayplayer))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor2();
	}
	else
	{
		CV_StealthSetValue(&cv_playercolor2,
			players[secondarydisplayplayer].skincolor);
	}
}

/** Displays the result of the chat being muted or unmuted.
  * The server or remote admin should already know and be able to talk
  * regardless, so this is only displayed to clients.
  *
  * \sa cv_mute
  * \author Graue <graue@oceanbase.org>
  */
static void Mute_OnChange(void)
{
	if (server || (adminplayer == consoleplayer))
		return;

	if (cv_mute.value)
		CONS_Printf(M_GetText("Chat has been muted.\n"));
	else
		CONS_Printf(M_GetText("Chat is no longer muted.\n"));
}

/** Hack to clear all changed flags after game start.
  * A lot of code (written by dummies, obviously) uses COM_BufAddText() to run
  * commands and change consvars, especially on game start. This is problematic
  * because CV_ClearChangedFlags() needs to get called on game start \b after
  * all those commands are run.
  *
  * Here's how it's done: the last thing in COM_BufAddText() is "dummyconsvar
  * 1", so we end up here, where dummyconsvar is reset to 0 and all the changed
  * flags are set to 0.
  *
  * \todo Fix the aforementioned code and make this hack unnecessary.
  * \sa cv_dummyconsvar
  * \author Graue <graue@oceanbase.org>
  */
static void DummyConsvar_OnChange(void)
{
	if (cv_dummyconsvar.value == 1)
	{
		CV_SetValue(&cv_dummyconsvar, 0);
		CV_ClearChangedFlags();
	}
}

static void Command_ShowScores_f(void)
{
	UINT8 i;

	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			// FIXME: %lu? what's wrong with %u? ~Callum (produces warnings...)
			CONS_Printf(M_GetText("%s's score is %u\n"), player_names[i], players[i].score);
	}
	CONS_Printf(M_GetText("The pointlimit is %d\n"), cv_pointlimit.value);

}

static void Command_ShowTime_f(void)
{
	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	CONS_Printf(M_GetText("The current time is %f.\nThe timelimit is %f\n"), (double)leveltime/TICRATE, (double)timelimitintics/TICRATE);
}
