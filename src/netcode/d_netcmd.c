// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.c
/// \brief host/client network commands
///        commands are executed through the command buffer
///	       like console commands, other miscellaneous commands (at the end)

#include "../doomdef.h"

#include "../console.h"
#include "../command.h"
#include "../i_time.h"
#include "../i_system.h"
#include "../g_game.h"
#include "../hu_stuff.h"
#include "../g_input.h"
#include "../m_menu.h"
#include "../r_local.h"
#include "../r_skins.h"
#include "../p_local.h"
#include "../p_setup.h"
#include "../s_sound.h"
#include "../i_sound.h"
#include "../m_misc.h"
#include "../am_map.h"
#include "../byteptr.h"
#include "d_netfil.h"
#include "../p_spec.h"
#include "../m_cheat.h"
#include "d_clisrv.h"
#include "server_connection.h"
#include "net_command.h"
#include "d_net.h"
#include "../v_video.h"
#include "../d_main.h"
#include "../m_random.h"
#include "../f_finale.h"
#include "../filesrch.h"
#include "mserv.h"
#include "../z_zone.h"
#include "../lua_script.h"
#include "../lua_hook.h"
#include "../m_cond.h"
#include "../m_anigif.h"
#include "../md5.h"
#include "../m_perfstats.h"
#include "../u_list.h"

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
static void Got_RequestAddfoldercmd(UINT8 **cp, INT32 playernum);
static void Got_Addfilecmd(UINT8 **cp, INT32 playernum);
static void Got_Addfoldercmd(UINT8 **cp, INT32 playernum);
static void Got_Pause(UINT8 **cp, INT32 playernum);
static void Got_Suicide(UINT8 **cp, INT32 playernum);
static void Got_RandomSeed(UINT8 **cp, INT32 playernum);
static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum);
static void Got_Teamchange(UINT8 **cp, INT32 playernum);
static void Got_Clearscores(UINT8 **cp, INT32 playernum);

static void PointLimit_OnChange(void);
static void TimeLimit_OnChange(void);
static void NumLaps_OnChange(void);
static void BaseNumLaps_OnChange(void);
static void Mute_OnChange(void);

static void Hidetime_OnChange(void);

static void AutoBalance_OnChange(void);
static void TeamScramble_OnChange(void);

static void NetTimeout_OnChange(void);
static void JoinTimeout_OnChange(void);

static void CoopStarposts_OnChange(void);
static void CoopLives_OnChange(void);
static void ExitMove_OnChange(void);

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

static boolean Skin_CanChange(const char *valstr);
static boolean Skin2_CanChange(const char *valstr);

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
static void Command_Addfolder(void);
static void Command_ListWADS_f(void);
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

static void Command_ExitLevel_f(void);
static void Command_Showmap_f(void);
static void Command_Mapmd5_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_ServerTeamChange_f(void);

static void Command_MutePlayer_f(void);
static void Command_UnmutePlayer_f(void);
static void Got_MutePlayer(UINT8 **cp, INT32 playernum);

static void Command_Clearscores_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Clearpassword_f(void);
static void Command_Login_f(void);
static void Got_Verification(UINT8 **cp, INT32 playernum);
static void Got_Removal(UINT8 **cp, INT32 playernum);
static void Command_Verify_f(void);
static void Command_RemoveAdmin_f(void);
static void Command_MotD_f(void);
static void Got_MotD_f(UINT8 **cp, INT32 playernum);

static void Command_ShowScores_f(void);
static void Command_ShowTime_f(void);

static void Command_Isgamemodified_f(void);
static void Command_Cheats_f(void);
#ifdef _DEBUG
static void Command_Togglemodified_f(void);
static void Command_Archivetest_f(void);
#endif

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

void SendWeaponPref(void);
void SendWeaponPref2(void);

static CV_PossibleValue_t usemouse_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL}};
#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
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

static CV_PossibleValue_t teamscramble_cons_t[] = {{0, "Off"}, {1, "Random"}, {2, "Points"}, {0, NULL}};

static CV_PossibleValue_t startingliveslimit_cons_t[] = {{1, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t sleeping_cons_t[] = {{0, "MIN"}, {1000/TICRATE, "MAX"}, {0, NULL}};
static CV_PossibleValue_t competitionboxes_cons_t[] = {{0, "Normal"}, {1, "Mystery"}, //{2, "Teleport"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t matchboxes_cons_t[] = {{0, "Normal"}, {1, "Mystery"}, {2, "Unchanging"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t chances_cons_t[] = {{0, "MIN"}, {9, "MAX"}, {0, NULL}};
static CV_PossibleValue_t pause_cons_t[] = {{0, "Server"}, {1, "All"}, {0, NULL}};

consvar_t cv_showinput = CVAR_INIT ("showinput", "Off", CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_showinputjoy = CVAR_INIT ("showinputjoy", "Off", CV_ALLOWLUA, CV_OnOff, NULL);

#ifdef NETGAME_DEVMODE
static consvar_t cv_fishcake = CVAR_INIT ("fishcake", "Off", CV_CALL|CV_NOSHOWHELP|CV_RESTRICT, CV_OnOff, Fishcake_OnChange);
#endif
static consvar_t cv_dummyconsvar = CVAR_INIT ("dummyconsvar", "Off", CV_CALL|CV_NOSHOWHELP, CV_OnOff, DummyConsvar_OnChange);

consvar_t cv_restrictskinchange = CVAR_INIT ("restrictskinchange", "Yes", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, CV_YesNo, NULL);
consvar_t cv_allowteamchange = CVAR_INIT ("allowteamchange", "Yes", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_YesNo, NULL);

consvar_t cv_startinglives = CVAR_INIT ("startinglives", "3", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, startingliveslimit_cons_t, NULL);

static CV_PossibleValue_t respawntime_cons_t[] = {{1, "MIN"}, {30, "MAX"}, {0, "Off"}, {0, NULL}};
consvar_t cv_respawntime = CVAR_INIT ("respawndelay", "3", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, respawntime_cons_t, NULL);

consvar_t cv_competitionboxes = CVAR_INIT ("competitionboxes", "Mystery", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, competitionboxes_cons_t, NULL);

static CV_PossibleValue_t seenames_cons_t[] = {{0, "Off"}, {1, "Colorless"}, {2, "Team"}, {3, "Ally/Foe"}, {0, NULL}};
consvar_t cv_seenames = CVAR_INIT ("seenames", "Ally/Foe", CV_SAVE|CV_ALLOWLUA, seenames_cons_t, 0);
consvar_t cv_allowseenames = CVAR_INIT ("allowseenames", "Yes", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_YesNo, NULL);

// names
consvar_t cv_playername = CVAR_INIT ("name", "Sonic", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name_OnChange);
consvar_t cv_playername2 = CVAR_INIT ("name2", "Tails", CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name2_OnChange);
// player colors
UINT16 lastgoodcolor = SKINCOLOR_BLUE, lastgoodcolor2 = SKINCOLOR_BLUE;
consvar_t cv_playercolor = CVAR_INIT ("color", "Blue", CV_CALL|CV_NOINIT|CV_ALLOWLUA, Color_cons_t, Color_OnChange);
consvar_t cv_playercolor2 = CVAR_INIT ("color2", "Orange", CV_CALL|CV_NOINIT|CV_ALLOWLUA, Color_cons_t, Color2_OnChange);
// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin = CVAR_INIT_WITH_CALLBACKS ("skin", DEFAULTSKIN, CV_CALL|CV_NOINIT|CV_ALLOWLUA, NULL, Skin_OnChange, Skin_CanChange);
consvar_t cv_skin2 = CVAR_INIT_WITH_CALLBACKS ("skin2", DEFAULTSKIN2, CV_CALL|CV_NOINIT|CV_ALLOWLUA, NULL, Skin2_OnChange, Skin2_CanChange);

// saved versions of the above six
consvar_t cv_defaultplayercolor = CVAR_INIT ("defaultcolor", "Blue", CV_SAVE, Color_cons_t, NULL);
consvar_t cv_defaultplayercolor2 = CVAR_INIT ("defaultcolor2", "Orange", CV_SAVE, Color_cons_t, NULL);
consvar_t cv_defaultskin = CVAR_INIT ("defaultskin", DEFAULTSKIN, CV_SAVE, NULL, NULL);
consvar_t cv_defaultskin2 = CVAR_INIT ("defaultskin2", DEFAULTSKIN2, CV_SAVE, NULL, NULL);

consvar_t cv_skipmapcheck = CVAR_INIT ("skipmapcheck", "Off", CV_SAVE, CV_OnOff, NULL);

INT32 cv_debug;

consvar_t cv_usemouse = CVAR_INIT ("use_mouse", "On", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse);
consvar_t cv_usemouse2 = CVAR_INIT ("use_mouse2", "Off", CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse2);

consvar_t cv_usejoystick = CVAR_INIT ("use_gamepad", "1", CV_SAVE|CV_CALL, usejoystick_cons_t, I_InitJoystick);
consvar_t cv_usejoystick2 = CVAR_INIT ("use_gamepad2", "2", CV_SAVE|CV_CALL, usejoystick_cons_t, I_InitJoystick2);
#if (defined (LJOYSTICK) || defined (HAVE_SDL))
#ifdef LJOYSTICK
consvar_t cv_joyport = CVAR_INIT ("padport", "/dev/js0", CV_SAVE, joyport_cons_t, NULL);
consvar_t cv_joyport2 = CVAR_INIT ("padport2", "/dev/js0", CV_SAVE, joyport_cons_t, NULL); //Alam: for later
#endif
consvar_t cv_joyscale = CVAR_INIT ("padscale", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale);
consvar_t cv_joyscale2 = CVAR_INIT ("padscale2", "1", CV_SAVE|CV_CALL, NULL, I_JoyScale2);
#else
consvar_t cv_joyscale = CVAR_INIT ("padscale", "1", CV_SAVE|CV_HIDEN, NULL, NULL); //Alam: Dummy for save
consvar_t cv_joyscale2 = CVAR_INIT ("padscale2", "1", CV_SAVE|CV_HIDEN, NULL, NULL); //Alam: Dummy for save
#endif
#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
consvar_t cv_mouse2port = CVAR_INIT ("mouse2port", "/dev/gpmdata", CV_SAVE, mouse2port_cons_t, NULL);
consvar_t cv_mouse2opt = CVAR_INIT ("mouse2opt", "0", CV_SAVE, NULL, NULL);
#else
consvar_t cv_mouse2port = CVAR_INIT ("mouse2port", "COM2", CV_SAVE, mouse2port_cons_t, NULL);
#endif

consvar_t cv_matchboxes = CVAR_INIT ("matchboxes", "Normal", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, matchboxes_cons_t, NULL);
consvar_t cv_specialrings = CVAR_INIT ("specialrings", "On", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_powerstones = CVAR_INIT ("powerstones", "On", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);

consvar_t cv_recycler =      CVAR_INIT ("tv_recycler",      "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_teleporters =   CVAR_INIT ("tv_teleporter",    "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_superring =     CVAR_INIT ("tv_superring",     "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_supersneakers = CVAR_INIT ("tv_supersneaker",  "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_invincibility = CVAR_INIT ("tv_invincibility", "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_jumpshield =    CVAR_INIT ("tv_jumpshield",    "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_watershield =   CVAR_INIT ("tv_watershield",   "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_ringshield =    CVAR_INIT ("tv_ringshield",    "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_forceshield =   CVAR_INIT ("tv_forceshield",   "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_bombshield =    CVAR_INIT ("tv_bombshield",    "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_1up =           CVAR_INIT ("tv_1up",           "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);
consvar_t cv_eggmanbox =     CVAR_INIT ("tv_eggman",        "5", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, chances_cons_t, NULL);

consvar_t cv_ringslinger = CVAR_INIT ("ringslinger", "No", CV_NETVAR|CV_NOSHOWHELP|CV_CALL|CV_CHEAT|CV_ALLOWLUA, CV_YesNo, Ringslinger_OnChange);
consvar_t cv_gravity = CVAR_INIT ("gravity", "0.5", CV_RESTRICT|CV_FLOAT|CV_CALL|CV_ALLOWLUA, NULL, Gravity_OnChange);

consvar_t cv_soundtest = CVAR_INIT ("soundtest", "0", CV_CALL, NULL, SoundTest_OnChange);

static CV_PossibleValue_t minitimelimit_cons_t[] = {{1, "MIN"}, {9999, "MAX"}, {0, NULL}};
consvar_t cv_countdowntime = CVAR_INIT ("countdowntime", "60", CV_SAVE|CV_NETVAR|CV_CHEAT|CV_ALLOWLUA, minitimelimit_cons_t, NULL);

consvar_t cv_touchtag = CVAR_INIT ("touchtag", "Off", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_hidetime = CVAR_INIT ("hidetime", "30", CV_SAVE|CV_NETVAR|CV_CALL|CV_ALLOWLUA, minitimelimit_cons_t, Hidetime_OnChange);

consvar_t cv_autobalance = CVAR_INIT ("autobalance", "Off", CV_SAVE|CV_NETVAR|CV_CALL|CV_ALLOWLUA, CV_OnOff, AutoBalance_OnChange);
consvar_t cv_teamscramble = CVAR_INIT ("teamscramble", "Off", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT|CV_ALLOWLUA, teamscramble_cons_t, TeamScramble_OnChange);
consvar_t cv_scrambleonchange = CVAR_INIT ("scrambleonchange", "Off", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, teamscramble_cons_t, NULL);

consvar_t cv_friendlyfire = CVAR_INIT ("friendlyfire", "Off", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_itemfinder = CVAR_INIT ("itemfinder", "Off", CV_CALL|CV_ALLOWLUA, CV_OnOff, ItemFinder_OnChange);

// Scoring type options
consvar_t cv_overtime = CVAR_INIT ("overtime", "Yes", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_YesNo, NULL);

consvar_t cv_rollingdemos = CVAR_INIT ("rollingdemos", "On", CV_SAVE, CV_OnOff, NULL);

static CV_PossibleValue_t timetic_cons_t[] = {{0, "Classic"}, {1, "Centiseconds"}, {2, "Mania"}, {3, "Tics"}, {0, NULL}};
consvar_t cv_timetic = CVAR_INIT ("timerres", "Classic", CV_SAVE, timetic_cons_t, NULL);

static CV_PossibleValue_t powerupdisplay_cons_t[] = {{0, "Never"}, {1, "First-person only"}, {2, "Always"}, {0, NULL}};
consvar_t cv_powerupdisplay = CVAR_INIT ("powerupdisplay", "First-person only", CV_SAVE, powerupdisplay_cons_t, NULL);

static CV_PossibleValue_t pointlimit_cons_t[] = {{1, "MIN"}, {MAXSCORE, "MAX"}, {0, "None"}, {0, NULL}};
consvar_t cv_pointlimit = CVAR_INIT ("pointlimit", "None", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT|CV_ALLOWLUA, pointlimit_cons_t, PointLimit_OnChange);
static CV_PossibleValue_t timelimit_cons_t[] = {{1, "MIN"}, {30, "MAX"}, {0, "None"}, {0, NULL}};
consvar_t cv_timelimit = CVAR_INIT ("timelimit", "None", CV_SAVE|CV_NETVAR|CV_CALL|CV_NOINIT|CV_ALLOWLUA, timelimit_cons_t, TimeLimit_OnChange);
static CV_PossibleValue_t numlaps_cons_t[] = {{1, "MIN"}, {50, "MAX"}, {0, NULL}};
consvar_t cv_numlaps = CVAR_INIT ("numlaps", "4", CV_NETVAR|CV_CALL|CV_NOINIT|CV_ALLOWLUA, numlaps_cons_t, NumLaps_OnChange);
static CV_PossibleValue_t basenumlaps_cons_t[] = {{1, "MIN"}, {50, "MAX"}, {0, "Map default"}, {0, NULL}};
consvar_t cv_basenumlaps = CVAR_INIT ("basenumlaps", "Map default", CV_SAVE|CV_NETVAR|CV_CALL|CV_CHEAT|CV_ALLOWLUA, basenumlaps_cons_t, BaseNumLaps_OnChange);

// Point and time limits for every gametype
INT32 pointlimits[NUMGAMETYPES];
INT32 timelimits[NUMGAMETYPES];

// log elemental hazards -- not a netvar, is local to current player
consvar_t cv_hazardlog = CVAR_INIT ("hazardlog", "Yes", 0, CV_YesNo, NULL);

consvar_t cv_forceskin = CVAR_INIT ("forceskin", "None", CV_NETVAR|CV_CALL|CV_CHEAT|CV_ALLOWLUA, NULL, ForceSkin_OnChange);
consvar_t cv_downloading = CVAR_INIT ("downloading", "On", 0, CV_OnOff, NULL);
consvar_t cv_allowexitlevel = CVAR_INIT ("allowexitlevel", "No", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, CV_YesNo, NULL);

consvar_t cv_killingdead = CVAR_INIT ("killingdead", "Off", CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);

consvar_t cv_netstat = CVAR_INIT ("netstat", "Off", 0, CV_OnOff, NULL); // show bandwidth statistics
static CV_PossibleValue_t nettimeout_cons_t[] = {{TICRATE/7, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_nettimeout = CVAR_INIT ("nettimeout", "350", CV_CALL|CV_SAVE, nettimeout_cons_t, NetTimeout_OnChange);
static CV_PossibleValue_t jointimeout_cons_t[] = {{5*TICRATE, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_jointimeout = CVAR_INIT ("jointimeout", "350", CV_CALL|CV_SAVE|CV_NETVAR, jointimeout_cons_t, JoinTimeout_OnChange);
consvar_t cv_maxping = CVAR_INIT ("maxping", "0", CV_SAVE|CV_NETVAR, CV_Unsigned, NULL);

static CV_PossibleValue_t pingtimeout_cons_t[] = {{8, "MIN"}, {120, "MAX"}, {0, NULL}};
consvar_t cv_pingtimeout = CVAR_INIT ("pingtimeout", "10", CV_SAVE|CV_NETVAR, pingtimeout_cons_t, NULL);

// show your ping on the HUD next to framerate. Defaults to warning only (shows up if your ping is > maxping)
static CV_PossibleValue_t showping_cons_t[] = {{0, "Off"}, {1, "Always"}, {2, "Warning"}, {0, NULL}};
consvar_t cv_showping = CVAR_INIT ("showping", "Warning", CV_SAVE, showping_cons_t, NULL);

// Intermission time Tails 04-19-2002
static CV_PossibleValue_t inttime_cons_t[] = {{0, "MIN"}, {3600, "MAX"}, {0, NULL}};
consvar_t cv_inttime = CVAR_INIT ("inttime", "10", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, inttime_cons_t, NULL);

static CV_PossibleValue_t coopstarposts_cons_t[] = {{0, "Per-player"}, {1, "Shared"}, {2, "Teamwork"}, {0, NULL}};
consvar_t cv_coopstarposts = CVAR_INIT ("coopstarposts", "Per-player", CV_SAVE|CV_NETVAR|CV_CALL|CV_ALLOWLUA, coopstarposts_cons_t, CoopStarposts_OnChange);

static CV_PossibleValue_t cooplives_cons_t[] = {{0, "Infinite"}, {1, "Per-player"}, {2, "Avoid Game Over"}, {3, "Single pool"}, {0, NULL}};
consvar_t cv_cooplives = CVAR_INIT ("cooplives", "Avoid Game Over", CV_SAVE|CV_NETVAR|CV_CALL|CV_CHEAT|CV_ALLOWLUA, cooplives_cons_t, CoopLives_OnChange);

static CV_PossibleValue_t advancemap_cons_t[] = {{0, "Off"}, {1, "Next"}, {2, "Random"}, {0, NULL}};
consvar_t cv_advancemap = CVAR_INIT ("advancemap", "Next", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, advancemap_cons_t, NULL);

static CV_PossibleValue_t playersforexit_cons_t[] = {{0, "One"}, {1, "1/4"}, {2, "Half"}, {3, "3/4"}, {4, "All"}, {0, NULL}};
consvar_t cv_playersforexit = CVAR_INIT ("playersforexit", "All", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, playersforexit_cons_t, NULL);

consvar_t cv_exitmove = CVAR_INIT ("exitmove", "On", CV_SAVE|CV_NETVAR|CV_CALL|CV_ALLOWLUA, CV_OnOff, ExitMove_OnChange);

consvar_t cv_runscripts = CVAR_INIT ("runscripts", "Yes", CV_ALLOWLUA, CV_YesNo, NULL);

consvar_t cv_pause = CVAR_INIT ("pausepermission", "Server", CV_SAVE|CV_NETVAR|CV_ALLOWLUA, pause_cons_t, NULL);
consvar_t cv_mute = CVAR_INIT ("mute", "Off", CV_NETVAR|CV_CALL|CV_ALLOWLUA, CV_OnOff, Mute_OnChange);

consvar_t cv_sleep = CVAR_INIT ("cpusleep", "1", CV_SAVE, sleeping_cons_t, NULL);

static CV_PossibleValue_t perfstats_cons_t[] = {
	{0, "Off"}, {1, "Rendering"}, {2, "Logic"}, {3, "ThinkFrame"}, {4, "PreThinkFrame"}, {5, "PostThinkFrame"}, {0, NULL}};
consvar_t cv_perfstats = CVAR_INIT ("perfstats", "Off", CV_CALL, perfstats_cons_t, PS_PerfStats_OnChange);
static CV_PossibleValue_t ps_samplesize_cons_t[] = {
	{1, "MIN"}, {1000, "MAX"}, {0, NULL}};
consvar_t cv_ps_samplesize = CVAR_INIT ("ps_samplesize", "175", CV_CALL, ps_samplesize_cons_t, PS_SampleSize_OnChange);
static CV_PossibleValue_t ps_descriptor_cons_t[] = {
	{1, "Average"}, {2, "SD"}, {3, "Minimum"}, {4, "Maximum"}, {0, NULL}};
consvar_t cv_ps_descriptor = CVAR_INIT ("ps_descriptor", "Average", 0, ps_descriptor_cons_t, NULL);

consvar_t cv_freedemocamera = CVAR_INIT("freedemocamera", "Off", CV_SAVE, CV_OnOff, NULL);

// NOTE: this should be in hw_main.c, but we can't put it there as it breaks dedicated build
consvar_t cv_glallowshaders = CVAR_INIT ("gr_allowcustomshaders", "On", CV_NETVAR, CV_OnOff, NULL);

char timedemo_name[256];
boolean timedemo_csv;
char timedemo_csv_id[256];
boolean timedemo_quit;

INT16 gametype = GT_COOP;
UINT32 gametyperules = 0;
INT16 gametypecount = (GT_CTF + 1);

boolean splitscreen = false;
boolean circuitmap = false;
INT32 adminplayers[MAXPLAYERS];

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
	"ADDFOLDER",
	"PAUSE",
	"ADDPLAYER",
	"TEAMCHANGE",
	"CLEARSCORES",
	"VERIFIED",
	"RANDOMSEED",
	"RUNSOC",
	"REQADDFILE",
	"REQADDFOLDER",
	"SETMOTD",
	"SUICIDE",
	"LUACMD",
	"LUAVAR",
	"LUAFILE"
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
	INT32 i;

	for (i = 0; i < NUMGAMETYPES; i++)
	{
		gametype_cons_t[i].value = i;
		gametype_cons_t[i].strvalue = Gametype_Names[i];
	}
	gametype_cons_t[NUMGAMETYPES].value = 0;
	gametype_cons_t[NUMGAMETYPES].strvalue = NULL;

	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndColor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_ADDFOLDER, Got_Addfoldercmd);
	RegisterNetXCmd(XD_REQADDFILE, Got_RequestAddfilecmd);
	RegisterNetXCmd(XD_REQADDFOLDER, Got_RequestAddfoldercmd);
	RegisterNetXCmd(XD_PAUSE, Got_Pause);
	RegisterNetXCmd(XD_SUICIDE, Got_Suicide);
	RegisterNetXCmd(XD_RUNSOC, Got_RunSOCcmd);
	RegisterNetXCmd(XD_LUACMD, Got_Luacmd);
	RegisterNetXCmd(XD_LUAFILE, Got_LuaFile);

	// Remote Administration
	COM_AddCommand("password", Command_Changepassword_f, COM_LUA);
	COM_AddCommand("clearpassword", Command_Clearpassword_f, COM_LUA);
	COM_AddCommand("login", Command_Login_f, COM_LUA); // useful in dedicated to kick off remote admin
	COM_AddCommand("promote", Command_Verify_f, COM_LUA);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);
	COM_AddCommand("demote", Command_RemoveAdmin_f, COM_LUA);
	RegisterNetXCmd(XD_DEMOTED, Got_Removal);

	COM_AddCommand("motd", Command_MotD_f, COM_LUA);
	RegisterNetXCmd(XD_SETMOTD, Got_MotD_f); // For remote admin

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", Command_ServerTeamChange_f, COM_LUA);

	RegisterNetXCmd(XD_MUTEPLAYER, Got_MutePlayer);
	COM_AddCommand("muteplayer", Command_MutePlayer_f, COM_LUA);
	COM_AddCommand("unmuteplayer", Command_UnmutePlayer_f, COM_LUA);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddCommand("clearscores", Command_Clearscores_f, COM_LUA);
	COM_AddCommand("map", Command_Map_f, COM_LUA);

	COM_AddCommand("exitgame", Command_ExitGame_f, COM_LUA);
	COM_AddCommand("retry", Command_Retry_f, COM_LUA);
	COM_AddCommand("exitlevel", Command_ExitLevel_f, COM_LUA);
	COM_AddCommand("showmap", Command_Showmap_f, COM_LUA);
	COM_AddCommand("mapmd5", Command_Mapmd5_f, COM_LUA);

	COM_AddCommand("addfolder", Command_Addfolder, COM_LUA);
	COM_AddCommand("addfile", Command_Addfile, COM_LUA);
	COM_AddCommand("listwad", Command_ListWADS_f, COM_LUA);

	COM_AddCommand("runsoc", Command_RunSOC, COM_LUA);
	COM_AddCommand("pause", Command_Pause, COM_LUA);
	COM_AddCommand("suicide", Command_Suicide, COM_LUA);

	COM_AddCommand("gametype", Command_ShowGametype_f, COM_LUA);
	COM_AddCommand("version", Command_Version_f, COM_LUA);
#ifdef UPDATE_ALERT
	COM_AddCommand("mod_details", Command_ModDetails_f, COM_LUA);
#endif
	COM_AddCommand("quit", Command_Quit_f, COM_LUA);

	COM_AddCommand("saveconfig", Command_SaveConfig_f, 0);
	COM_AddCommand("loadconfig", Command_LoadConfig_f, 0);
	COM_AddCommand("changeconfig", Command_ChangeConfig_f, 0);
	COM_AddCommand("isgamemodified", Command_Isgamemodified_f, COM_LUA); // test
	COM_AddCommand("showscores", Command_ShowScores_f, COM_LUA);
	COM_AddCommand("showtime", Command_ShowTime_f, COM_LUA);
	COM_AddCommand("cheats", Command_Cheats_f, COM_LUA); // test
#ifdef _DEBUG
	COM_AddCommand("togglemodified", Command_Togglemodified_f, COM_LUA);
	COM_AddCommand("archivetest", Command_Archivetest_f, COM_LUA);
#endif

	COM_AddCommand("downloads", Command_Downloads_f, COM_LUA);

	// for master server connection
	AddMServCommands();

	CV_RegisterVar(&cv_glallowshaders);

	// p_mobj.c
	CV_RegisterVar(&cv_itemrespawntime);
	CV_RegisterVar(&cv_itemrespawn);
	CV_RegisterVar(&cv_flagtime);

	// misc
	CV_RegisterVar(&cv_friendlyfire);
	CV_RegisterVar(&cv_pointlimit);
	CV_RegisterVar(&cv_numlaps);
	CV_RegisterVar(&cv_basenumlaps);

	CV_RegisterVar(&cv_hazardlog);

	CV_RegisterVar(&cv_autobalance);
	CV_RegisterVar(&cv_teamscramble);
	CV_RegisterVar(&cv_scrambleonchange);

	CV_RegisterVar(&cv_touchtag);
	CV_RegisterVar(&cv_hidetime);

	CV_RegisterVar(&cv_inttime);
	CV_RegisterVar(&cv_advancemap);
	CV_RegisterVar(&cv_playersforexit);
	CV_RegisterVar(&cv_exitmove);
	CV_RegisterVar(&cv_timelimit);
	CV_RegisterVar(&cv_playbackspeed);
	CV_RegisterVar(&cv_forceskin);
	CV_RegisterVar(&cv_downloading);

	CV_RegisterVar(&cv_coopstarposts);
	CV_RegisterVar(&cv_cooplives);

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
	CV_RegisterVar(&cv_joindelay);
	CV_RegisterVar(&cv_rejointimeout);
	CV_RegisterVar(&cv_resynchattempts);
	CV_RegisterVar(&cv_maxsend);
	CV_RegisterVar(&cv_noticedownload);
	CV_RegisterVar(&cv_downloadspeed);
	CV_RegisterVar(&cv_allownewplayer);
	CV_RegisterVar(&cv_showjoinaddress);
	CV_RegisterVar(&cv_blamecfail);
	CV_RegisterVar(&cv_dedicatedidletime);
	CV_RegisterVar(&cv_idletime);
	CV_RegisterVar(&cv_idleaction);
	CV_RegisterVar(&cv_httpsource);

	COM_AddCommand("ping", Command_Ping_f, COM_LUA);
	CV_RegisterVar(&cv_nettimeout);
	CV_RegisterVar(&cv_jointimeout);

	CV_RegisterVar(&cv_skipmapcheck);
	CV_RegisterVar(&cv_sleep);
	CV_RegisterVar(&cv_maxping);
	CV_RegisterVar(&cv_pingtimeout);
	CV_RegisterVar(&cv_showping);

	CV_RegisterVar(&cv_allowseenames);

	// Other filesrch.c consvars are defined in D_RegisterClientCommands
	CV_RegisterVar(&cv_addons_option);
	CV_RegisterVar(&cv_addons_folder);

	CV_RegisterVar(&cv_dummyconsvar);

	CV_RegisterVar(&cv_chatspamprotection);
	CV_RegisterVar(&cv_chatspamspeed);
	CV_RegisterVar(&cv_chatspamburst);
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
		Color_cons_t[i].strvalue = skincolors[i].name;
	}
	Color_cons_t[MAXSKINCOLORS].value = 0;
	Color_cons_t[MAXSKINCOLORS].strvalue = NULL;

	// Set default player names
	// Monster Iestyn (12/08/19): not sure where else I could have actually put this, but oh well
	for (i = 0; i < MAXPLAYERS; i++)
		sprintf(player_names[i], "Player %d", 1 + i);

	CV_RegisterVar(&cv_gravity);
	CV_RegisterVar(&cv_tailspickup);
	CV_RegisterVar(&cv_allowmlook);
	CV_RegisterVar(&cv_flipcam);
	CV_RegisterVar(&cv_flipcam2);
	CV_RegisterVar(&cv_movebob);

	if (dedicated)
		return;

	COM_AddCommand("numthinkers", Command_Numthinkers_f, COM_LUA);
	COM_AddCommand("countmobjs", Command_CountMobjs_f, COM_LUA);

	COM_AddCommand("changeteam", Command_Teamchange_f, COM_LUA);
	COM_AddCommand("changeteam2", Command_Teamchange2_f, COM_LUA);

	COM_AddCommand("playdemo", Command_Playdemo_f, 0);
	COM_AddCommand("timedemo", Command_Timedemo_f, 0);
	COM_AddCommand("stopdemo", Command_Stopdemo_f, COM_LUA);
	COM_AddCommand("playintro", Command_Playintro_f, COM_LUA);
	CV_RegisterVar(&cv_resyncdemo);

	COM_AddCommand("resetcamera", Command_ResetCamera_f, COM_LUA);

	COM_AddCommand("setcontrol", Command_Setcontrol_f, 0);
	COM_AddCommand("setcontrol2", Command_Setcontrol2_f, 0);

	COM_AddCommand("screenshot", M_ScreenShot, COM_LUA);
	COM_AddCommand("startmovie", Command_StartMovie_f, COM_LUA);
	COM_AddCommand("stopmovie", Command_StopMovie_f, COM_LUA);

	CV_RegisterVar(&cv_screenshot_option);
	CV_RegisterVar(&cv_screenshot_folder);
	CV_RegisterVar(&cv_screenshot_colorprofile);
	CV_RegisterVar(&cv_moviemode);
	CV_RegisterVar(&cv_movie_option);
	CV_RegisterVar(&cv_movie_folder);
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
	CV_RegisterVar(&cv_apng_downscale);
	// GIF variables
	CV_RegisterVar(&cv_gif_optimize);
	CV_RegisterVar(&cv_gif_downscale);
	CV_RegisterVar(&cv_gif_dynamicdelay);
	CV_RegisterVar(&cv_gif_localcolortable);

	// register these so it is saved to config
	CV_RegisterVar(&cv_playername);
	CV_RegisterVar(&cv_playercolor);
	CV_RegisterVar(&cv_skin); // r_things.c (skin NAME)
	// secondary player (splitscreen)
	CV_RegisterVar(&cv_playername2);
	CV_RegisterVar(&cv_playercolor2);
	CV_RegisterVar(&cv_skin2);
	// saved versions of the above six
	CV_RegisterVar(&cv_defaultplayercolor);
	CV_RegisterVar(&cv_defaultskin);
	CV_RegisterVar(&cv_defaultplayercolor2);
	CV_RegisterVar(&cv_defaultskin2);

	CV_RegisterVar(&cv_seenames);
	CV_RegisterVar(&cv_rollingdemos);
	CV_RegisterVar(&cv_netstat);
	CV_RegisterVar(&cv_netticbuffer);

#ifdef NETGAME_DEVMODE
	CV_RegisterVar(&cv_fishcake);
#endif

	// HUD
	CV_RegisterVar(&cv_timetic);
	CV_RegisterVar(&cv_powerupdisplay);
	CV_RegisterVar(&cv_itemfinder);
	CV_RegisterVar(&cv_showinput);
	CV_RegisterVar(&cv_showinputjoy);

	// time attack ghost options are also saved to config
	CV_RegisterVar(&cv_ghost_bestscore);
	CV_RegisterVar(&cv_ghost_besttime);
	CV_RegisterVar(&cv_ghost_bestrings);
	CV_RegisterVar(&cv_ghost_last);
	CV_RegisterVar(&cv_ghost_guest);

	COM_AddCommand("displayplayer", Command_Displayplayer_f, COM_LUA);

	// FIXME: not to be here.. but needs be done for config loading
	CV_RegisterVar(&cv_globalgamma);
	CV_RegisterVar(&cv_globalsaturation);

	CV_RegisterVar(&cv_rhue);
	CV_RegisterVar(&cv_yhue);
	CV_RegisterVar(&cv_ghue);
	CV_RegisterVar(&cv_chue);
	CV_RegisterVar(&cv_bhue);
	CV_RegisterVar(&cv_mhue);

	CV_RegisterVar(&cv_rgamma);
	CV_RegisterVar(&cv_ygamma);
	CV_RegisterVar(&cv_ggamma);
	CV_RegisterVar(&cv_cgamma);
	CV_RegisterVar(&cv_bgamma);
	CV_RegisterVar(&cv_mgamma);

	CV_RegisterVar(&cv_rsaturation);
	CV_RegisterVar(&cv_ysaturation);
	CV_RegisterVar(&cv_gsaturation);
	CV_RegisterVar(&cv_csaturation);
	CV_RegisterVar(&cv_bsaturation);
	CV_RegisterVar(&cv_msaturation);

	// m_menu.c
	CV_RegisterVar(&cv_compactscoreboard);
	CV_RegisterVar(&cv_chatheight);
	CV_RegisterVar(&cv_chatwidth);
	CV_RegisterVar(&cv_chattime);
	CV_RegisterVar(&cv_chatbacktint);
	CV_RegisterVar(&cv_consolechat);
	CV_RegisterVar(&cv_chatnotifications);
	CV_RegisterVar(&cv_crosshair);
	CV_RegisterVar(&cv_crosshair2);
	CV_RegisterVar(&cv_alwaysfreelook);
	CV_RegisterVar(&cv_alwaysfreelook2);
	CV_RegisterVar(&cv_chasefreelook);
	CV_RegisterVar(&cv_chasefreelook2);
	CV_RegisterVar(&cv_tutorialprompt);
	CV_RegisterVar(&cv_showfocuslost);
	CV_RegisterVar(&cv_pauseifunfocused);

	CV_RegisterVar(&cv_instantretry);

	// g_input.c
	CV_RegisterVar(&cv_sideaxis);
	CV_RegisterVar(&cv_sideaxis2);
	CV_RegisterVar(&cv_turnaxis);
	CV_RegisterVar(&cv_turnaxis2);
	CV_RegisterVar(&cv_moveaxis);
	CV_RegisterVar(&cv_moveaxis2);
	CV_RegisterVar(&cv_lookaxis);
	CV_RegisterVar(&cv_lookaxis2);
	CV_RegisterVar(&cv_jumpaxis);
	CV_RegisterVar(&cv_jumpaxis2);
	CV_RegisterVar(&cv_spinaxis);
	CV_RegisterVar(&cv_spinaxis2);
	CV_RegisterVar(&cv_shieldaxis);
	CV_RegisterVar(&cv_shieldaxis2);
	CV_RegisterVar(&cv_fireaxis);
	CV_RegisterVar(&cv_fireaxis2);
	CV_RegisterVar(&cv_firenaxis);
	CV_RegisterVar(&cv_firenaxis2);
	CV_RegisterVar(&cv_deadzone);
	CV_RegisterVar(&cv_deadzone2);
	CV_RegisterVar(&cv_digitaldeadzone);
	CV_RegisterVar(&cv_digitaldeadzone2);

	// filesrch.c
	//CV_RegisterVar(&cv_addons_option); // These two are now defined
	//CV_RegisterVar(&cv_addons_folder); // in D_RegisterServerCommands
	CV_RegisterVar(&cv_addons_md5);
	CV_RegisterVar(&cv_addons_showall);
	CV_RegisterVar(&cv_addons_search_type);
	CV_RegisterVar(&cv_addons_search_case);

	// WARNING: the order is important when initialising mouse2
	// we need the mouse2port
	CV_RegisterVar(&cv_mouse2port);
#if defined (__unix__) || defined (__APPLE__) || defined (UNIXCOMMON)
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
	CV_RegisterVar(&cv_analog[0]);
	CV_RegisterVar(&cv_analog[1]);
	CV_RegisterVar(&cv_useranalog[0]);
	CV_RegisterVar(&cv_useranalog[1]);

	// deez New User eXperiences
	CV_RegisterVar(&cv_directionchar[0]);
	CV_RegisterVar(&cv_directionchar[1]);
	CV_RegisterVar(&cv_autobrake);
	CV_RegisterVar(&cv_autobrake2);

	// hi here's some new controls
	CV_RegisterVar(&cv_cam_shiftfacing[0]);
	CV_RegisterVar(&cv_cam_shiftfacing[1]);
	CV_RegisterVar(&cv_cam_turnfacing[0]);
	CV_RegisterVar(&cv_cam_turnfacing[1]);
	CV_RegisterVar(&cv_cam_turnfacingability[0]);
	CV_RegisterVar(&cv_cam_turnfacingability[1]);
	CV_RegisterVar(&cv_cam_turnfacingspindash[0]);
	CV_RegisterVar(&cv_cam_turnfacingspindash[1]);
	CV_RegisterVar(&cv_cam_turnfacinginput[0]);
	CV_RegisterVar(&cv_cam_turnfacinginput[1]);
	CV_RegisterVar(&cv_cam_centertoggle[0]);
	CV_RegisterVar(&cv_cam_centertoggle[1]);
	CV_RegisterVar(&cv_cam_lockedinput[0]);
	CV_RegisterVar(&cv_cam_lockedinput[1]);
	CV_RegisterVar(&cv_cam_lockonboss[0]);
	CV_RegisterVar(&cv_cam_lockonboss[1]);

	// s_sound.c
	CV_RegisterVar(&cv_soundvolume);
	CV_RegisterVar(&cv_closedcaptioning);
	CV_RegisterVar(&cv_digmusicvolume);
	CV_RegisterVar(&cv_midimusicvolume);
	CV_RegisterVar(&cv_numChannels);

	// screen.c
	CV_RegisterVar(&cv_fullscreen);
	CV_RegisterVar(&cv_renderer);
	CV_RegisterVar(&cv_scr_depth);
	CV_RegisterVar(&cv_scr_width);
	CV_RegisterVar(&cv_scr_height);
	CV_RegisterVar(&cv_scr_width_w);
	CV_RegisterVar(&cv_scr_height_w);

	CV_RegisterVar(&cv_soundtest);

	CV_RegisterVar(&cv_perfstats);
	CV_RegisterVar(&cv_ps_samplesize);
	CV_RegisterVar(&cv_ps_descriptor);

	// ingame object placing
	COM_AddCommand("objectplace", Command_ObjectPlace_f, COM_LUA);
	COM_AddCommand("writethings", Command_Writethings_f, COM_LUA);
	CV_RegisterVar(&cv_speed);
	CV_RegisterVar(&cv_opflags);
	CV_RegisterVar(&cv_ophoopflags);
	CV_RegisterVar(&cv_mapthingnum);

	CV_RegisterVar(&cv_freedemocamera);

	// add cheat commands
	COM_AddCommand("noclip", Command_CheatNoClip_f, COM_LUA);
	COM_AddCommand("god", Command_CheatGod_f, COM_LUA);
	COM_AddCommand("notarget", Command_CheatNoTarget_f, COM_LUA);
	COM_AddCommand("getallemeralds", Command_Getallemeralds_f, COM_LUA);
	COM_AddCommand("resetemeralds", Command_Resetemeralds_f, COM_LUA);
	COM_AddCommand("setrings", Command_Setrings_f, COM_LUA);
	COM_AddCommand("setlives", Command_Setlives_f, COM_LUA);
	COM_AddCommand("setcontinues", Command_Setcontinues_f, COM_LUA);
	COM_AddCommand("devmode", Command_Devmode_f, COM_LUA);
	COM_AddCommand("savecheckpoint", Command_Savecheckpoint_f, COM_LUA);
	COM_AddCommand("scale", Command_Scale_f, COM_LUA);
	COM_AddCommand("gravflip", Command_Gravflip_f, COM_LUA);
	COM_AddCommand("hurtme", Command_Hurtme_f, COM_LUA);
	COM_AddCommand("jumptoaxis", Command_JumpToAxis_f, COM_LUA);
	COM_AddCommand("charability", Command_Charability_f, COM_LUA);
	COM_AddCommand("charspeed", Command_Charspeed_f, COM_LUA);
	COM_AddCommand("teleport", Command_Teleport_f, COM_LUA);
	COM_AddCommand("rteleport", Command_RTeleport_f, COM_LUA);
	COM_AddCommand("skynum", Command_Skynum_f, COM_LUA);
	COM_AddCommand("weather", Command_Weather_f, COM_LUA);
	COM_AddCommand("toggletwod", Command_Toggletwod_f, COM_LUA);
#ifdef _DEBUG
	COM_AddCommand("causecfail", Command_CauseCfail_f, COM_LUA);
#endif
#ifdef LUA_ALLOW_BYTECODE
	COM_AddCommand("dumplua", Command_Dumplua_f, COM_LUA);
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
boolean EnsurePlayerNameIsGood(char *name, INT32 playernum)
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
				if (!EnsurePlayerNameIsGood (name, playernum))
					return false;
			}
			else if (len == 1) // Agh!
			{
				// Last ditch effort...
				sprintf(name, "%d", M_RandomKey(10));
				if (!EnsurePlayerNameIsGood (name, playernum))
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
void CleanupPlayerName(INT32 playernum, const char *newname)
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

		do
		{
			/* from EnsurePlayerNameIsGood */
			if (!isprint(*p) || *p == ';' || (UINT8)*p >= 0x80)
				break;
		}
		while (*++p) ;

		if (*p)/* bad char found */
			break;

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
  * \sa CleanupPlayerName, EnsurePlayerNameIsGood
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(INT32 playernum, char *newname)
{
	if (EnsurePlayerNameIsGood(newname, playernum))
	{
		if (strcasecmp(newname, player_names[playernum]) != 0)
		{
			if (netgame)
				HU_AddChatText(va("\x82*%s renamed to %s", player_names[playernum], newname), false);

			player_name_changes[playernum]++;

			strcpy(player_names[playernum], newname);
		}
	}
	else
	{
		CONS_Printf(M_GetText("Player %d sent a bad name change\n"), playernum+1);
		if (server && netgame)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
	}
}

UINT8 CanChangeSkin(INT32 playernum)
{
	// Of course we can change if we're not playing
	if (!Playing() || !addedtogame)
		return true;

	// Force skin in effect.
	if ((cv_forceskin.value != -1) || (mapheaderinfo[gamemap-1] && mapheaderinfo[gamemap-1]->forcecharacter[0] != '\0'))
		return false;

	// Can change skin in intermission and whatnot.
	if (gamestate != GS_LEVEL)
		return true;

	// Server has skin change restrictions.
	if (cv_restrictskinchange.value)
	{
		if (gametyperules & GTR_FRIENDLY)
			return true;

		// Can change skin during initial countdown.
		if ((gametyperules & GTR_RACE) && leveltime < 4*TICRATE)
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
	for (INT32 i = 0; i < MAXPLAYERS; ++i)
	{
		if (playeringame[i])
			SetPlayerSkinByNum(i, forcedskin);
	}
}

static INT32 snacpending = 0, snac2pending = 0, chmappending = 0;

static void SetSkinLocal(INT32 playernum, INT32 skinnum)
{
	if (metalrecording && playernum == consoleplayer)
	{
		// Starring Metal Sonic as themselves, obviously.
		SetPlayerSkinByNum(playernum, 5);
		return;
	}

	if (skinnum != -1 && R_SkinUsable(playernum, skinnum))
		SetPlayerSkinByNum(playernum, skinnum);
	else
		SetPlayerSkinByNum(playernum, GetPlayerDefaultSkin(playernum));
}

static void SetColorLocal(INT32 playernum, UINT16 color)
{
	players[playernum].skincolor = color;

	if (players[playernum].mo && !players[playernum].powers[pw_dye])
		players[playernum].mo->color = P_GetPlayerColor(&players[playernum]);
}

// name, color, or skin has changed
//
static void SendNameAndColor(void)
{
	char buf[MAXPLAYERNAME+7];
	char *p;

	p = buf;

	// don't allow inaccessible colors
	if (!skincolors[cv_playercolor.value].accessible)
	{
		if (players[consoleplayer].skincolor && skincolors[players[consoleplayer].skincolor].accessible)
			CV_StealthSetValue(&cv_playercolor, players[consoleplayer].skincolor);
		else if (skincolors[atoi(cv_playercolor.defaultvalue)].accessible)
			CV_StealthSet(&cv_playercolor, cv_playercolor.defaultvalue);
		else if (skins[players[consoleplayer].skin]->prefcolor && skincolors[skins[players[consoleplayer].skin]->prefcolor].accessible)
			CV_StealthSetValue(&cv_playercolor, skins[players[consoleplayer].skin]->prefcolor);
		else {
			UINT16 i = 0;
			while (i<numskincolors && !skincolors[i].accessible) i++;
			CV_StealthSetValue(&cv_playercolor, (i != numskincolors) ? i : SKINCOLOR_BLUE);
		}
	}

	if (!strcmp(cv_playername.string, player_names[consoleplayer])
	&& cv_playercolor.value == players[consoleplayer].skincolor
	&& !strcmp(cv_skin.string, skins[players[consoleplayer].skin]->name))
		return;

	players[consoleplayer].availabilities = R_GetSkinAvailabilities();

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	// If you're not in a netgame, merely update the skin, color, and name.
	if (!netgame)
	{
		CleanupPlayerName(consoleplayer, cv_playername.zstring);
		strcpy(player_names[consoleplayer], cv_playername.zstring);

		SetColorLocal(consoleplayer, cv_playercolor.value);

		if (splitscreen || (!pickedchar && stricmp(cv_skin.string, skins[consoleplayer]->name) != 0))
			SetSkinLocal(consoleplayer, R_SkinAvailable(cv_skin.string));
		else
			SetSkinLocal(consoleplayer, pickedchar);
		return;
	}

	snacpending++;

	// Don't change name if muted
	if (player_name_changes[consoleplayer] >= MAXNAMECHANGES)
	{
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
		HU_AddChatText("\x85*You must wait to change your name again", false);
	}
	else if ((cv_mute.value || players[consoleplayer].muted) && !(server || IsPlayerAdmin(consoleplayer)))
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
	else // Cleanup name if changing it
		CleanupPlayerName(consoleplayer, cv_playername.zstring);

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin.value = R_SkinAvailable(cv_skin.string);
	if ((cv_skin.value < 0) || !R_SkinUsable(consoleplayer, cv_skin.value))
	{
		INT32 defaultSkinNum = GetPlayerDefaultSkin(consoleplayer);
		CV_StealthSet(&cv_skin, skins[defaultSkinNum]->name);
		cv_skin.value = defaultSkinNum;
	}

	// Finally write out the complete packet and send it off.
	WRITESTRINGN(p, cv_playername.zstring, MAXPLAYERNAME);
	WRITEUINT32(p, (UINT32)players[consoleplayer].availabilities);
	WRITEUINT16(p, (UINT16)cv_playercolor.value);
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

	if (!skincolors[cv_playercolor2.value].accessible)
	{
		if (players[secondplaya].skincolor && skincolors[players[secondplaya].skincolor].accessible)
			CV_StealthSetValue(&cv_playercolor2, players[secondplaya].skincolor);
		else if (skincolors[atoi(cv_playercolor2.defaultvalue)].accessible)
			CV_StealthSet(&cv_playercolor, cv_playercolor2.defaultvalue);
		else if (skins[players[secondplaya].skin]->prefcolor && skincolors[skins[players[secondplaya].skin]->prefcolor].accessible)
			CV_StealthSetValue(&cv_playercolor2, skins[players[secondplaya].skin]->prefcolor);
		else {
			UINT16 i = 0;
			while (i<numskincolors && !skincolors[i].accessible) i++;
			CV_StealthSetValue(&cv_playercolor2, (i != numskincolors) ? i : SKINCOLOR_BLUE);
		}
	}

	players[secondplaya].availabilities = R_GetSkinAvailabilities();

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	if (botingame)
	{
		SetColorLocal(secondplaya, botcolor);
		SetPlayerSkinByNum(secondplaya, botskin-1);
		return;
	}
	else if (!netgame)
	{
		// If you're not in a netgame, merely update the skin, color, and name.
		CleanupPlayerName(secondplaya, cv_playername2.zstring);
		strcpy(player_names[secondplaya], cv_playername2.zstring);

		SetColorLocal(secondplaya, cv_playercolor2.value);

		if (cv_forceskin.value >= 0)
			SetSkinLocal(secondplaya, cv_forceskin.value);
		else
			SetSkinLocal(secondplaya, R_SkinAvailable(cv_skin2.string));
		return;
	}

	// Don't actually send anything because splitscreen isn't actually allowed in netgames anyway!
}

static void Got_NameAndColor(UINT8 **cp, INT32 playernum)
{
	player_t *p = &players[playernum];
	char name[MAXPLAYERNAME+1];
	UINT16 color;
	UINT8 skin;

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
	p->availabilities = READUINT32(*cp);
	color = READUINT16(*cp);
	skin = READUINT8(*cp);

	// set name
	if (player_name_changes[playernum] < MAXNAMECHANGES)
	{
		if (strcasecmp(player_names[playernum], name) != 0)
			SetPlayerName(playernum, name);
	}

	// set color
	p->skincolor = color % numskincolors;
	if (p->mo)
		p->mo->color = P_GetPlayerColor(p);

	// normal player colors
	if (server && (p != &players[consoleplayer] && p != &players[secondarydisplayplayer]))
	{
		boolean kick = false;
		UINT32 unlockShift = 0;
		UINT32 i;

		// don't allow inaccessible colors
		if (skincolors[p->skincolor].accessible == false)
			kick = true;

		// availabilities
		for (i = 0; i < MAXUNLOCKABLES; i++)
		{
			if (unlockables[i].type != SECRET_SKIN)
			{
				continue;
			}

			unlockShift++;
		}

		// If they set an invalid bit to true, then likely a modified client
		if (unlockShift < 32) // 32 is the max the data type allows
		{
			UINT32 illegalMask = UINT32_MAX;

			for (i = 0; i < unlockShift; i++)
			{
				illegalMask &= ~(1 << i);
			}

			if ((p->availabilities & illegalMask) != 0)
			{
				kick = true;
			}
		}

		if (kick)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal color change received from %s (team: %d), color: %d)\n"), player_names[playernum], p->ctfteam, p->skincolor);
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
			return;
		}
	}

	// set skin
	INT32 forcedskin = R_GetForcedSkin(playernum);
	if (forcedskin != -1 && (netgame || multiplayer)) // Server wants everyone to use the same player (or the level is forcing one.)
		SetPlayerSkinByNum(playernum, forcedskin);
	else
		SetPlayerSkinByNum(playernum, skin);
}

void SendWeaponPref(void)
{
	UINT8 buf[1];

	buf[0] = 0;
	if (cv_flipcam.value)
		buf[0] |= 1;
	if (cv_analog[0].value && cv_directionchar[0].value != 2)
		buf[0] |= 2;
	if (cv_directionchar[0].value == 1)
		buf[0] |= 4;
	if (cv_autobrake.value)
		buf[0] |= 8;
	SendNetXCmd(XD_WEAPONPREF, buf, 1);
}

void SendWeaponPref2(void)
{
	UINT8 buf[1];

	buf[0] = 0;
	if (cv_flipcam2.value)
		buf[0] |= 1;
	if (cv_analog[1].value && cv_directionchar[1].value != 2)
		buf[0] |= 2;
	if (cv_directionchar[1].value == 1)
		buf[0] |= 4;
	if (cv_autobrake2.value)
		buf[0] |= 8;
	SendNetXCmd2(XD_WEAPONPREF, buf, 1);
}

static void Got_WeaponPref(UINT8 **cp,INT32 playernum)
{
	UINT8 prefs = READUINT8(*cp);

	players[playernum].pflags &= ~(PF_FLIPCAM|PF_ANALOGMODE|PF_DIRECTIONCHAR|PF_AUTOBRAKE);
	if (prefs & 1)
		players[playernum].pflags |= PF_FLIPCAM;
	if (prefs & 2)
		players[playernum].pflags |= PF_ANALOGMODE;
	if (prefs & 4)
		players[playernum].pflags |= PF_DIRECTIONCHAR;
	if (prefs & 8)
		players[playernum].pflags |= PF_AUTOBRAKE;
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

	if (COM_Argc() < 2)
	{
		CONS_Printf("playdemo <demoname> [-addfiles / -force]:\n");
		CONS_Printf(M_GetText(
					"Play back a demo file. The full path from your SRB2 directory must be given.\n\n"

					"* With \"-addfiles\", any required files are added from a list contained within the demo file.\n"
					"* With \"-force\", the demo is played even if the necessary files have not been added.\n"));
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

	demofileoverride = DFILE_OVERRIDE_NONE;
	if (strcmp(COM_Argv(2), "-addfiles") == 0)
	{
		demofileoverride = DFILE_OVERRIDE_LOAD;
	}
	else if (strcmp(COM_Argv(2), "-force") == 0)
	{
		demofileoverride = DFILE_OVERRIDE_SKIP;
	}

	// Internal if no extension, external if one exists
	// If external, convert the file name to a path in SRB2's home directory
	if (FIL_CheckExtension(name))
		G_DoPlayDemo(va("%s"PATHSEP"%s", srb2home, name));
	else
		G_DoPlayDemo(name);
}

static void Command_Timedemo_f(void)
{
	size_t i = 0;

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("timedemo <demoname> [-csv [<trialid>]] [-quit]: time a demo\n"));
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
	strcpy (timedemo_name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	// print timedemo results as CSV?
	i = COM_CheckParm("-csv");
	timedemo_csv = (i > 0);
	if (COM_CheckParm("-quit") != i + 1)
		strcpy(timedemo_csv_id, COM_Argv(i + 1)); // user-defined string to identify row
	else
		timedemo_csv_id[0] = 0;

	// exit after the timedemo?
	timedemo_quit = (COM_CheckParm("-quit") > 0);

	CONS_Printf(M_GetText("Timing demo '%s'.\n"), timedemo_name);

	G_TimeDemo(timedemo_name);
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
	if (mapnum != -1)
	{
		CV_SetValue(&cv_nextmap, mapnum);
		// Kick bot from special stages
		if (botskin)
		{
			if (G_IsSpecialStage(mapnum) || (mapheaderinfo[mapnum-1] && (mapheaderinfo[mapnum-1]->typeoflevel & TOL_NIGHTS)))
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
	}
	CONS_Debug(DBG_GAMELOGIC, "Map change: mapnum=%d gametype=%d ultmode=%d resetplayers=%d delay=%d skipprecutscene=%d\n",
	           mapnum, newgametype, pultmode, resetplayers, delay, skipprecutscene);
	if ((netgame || multiplayer) && !((gametype == newgametype) && (gametypedefaultrules[newgametype] & GTR_CAMPAIGN)))
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
		if (!IsPlayerAdmin(consoleplayer))
		{
			SV_SpawnServer();
			if (!Playing()) // you failed to start a server somehow, so cancel the map change
				return;
		}

		chmappending++;
		if (netgame)
			WRITEUINT32(buf_p, M_RandomizedSeed()); // random seed
		SendNetXCmd(XD_MAP, buf, buf_p - buf);
	}
}

static char *
ConcatCommandArgv (int start, int end)
{
	char *final;

	size_t size;

	int i;
	char *p;

	size = 0;

	for (i = start; i < end; ++i)
	{
		/*
		one space after each argument, but terminating
		character on final argument
		*/
		size += strlen(COM_Argv(i)) + 1;
	}

	final = ZZ_Alloc(size);
	p = final;

	--end;/* handle the final argument separately */
	for (i = start; i < end; ++i)
	{
		p += sprintf(p, "%s ", COM_Argv(i));
	}
	/* at this point "end" is actually the last argument's position */
	strcpy(p, COM_Argv(end));

	return final;
}

//
// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
// Largely rewritten by James.
//
static void Command_Map_f(void)
{
	size_t first_option;
	size_t option_force;
	size_t option_gametype;
	const char *gametypename;
	boolean newresetplayers;
	boolean prevent_cheat;
	boolean set_cheated;

	INT32 newmapnum;

	char   *    mapname;
	char   *realmapname = NULL;

	INT32 newgametype = gametype;

	INT32 d;

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	option_force    =   COM_CheckPartialParm("-f") || (cv_debug || devparm);
	option_gametype =   COM_CheckPartialParm("-g");
	newresetplayers = ! COM_CheckParm("-noresetplayers");

	prevent_cheat = !( usedCheats ) && !( option_force || cv_debug );
	set_cheated = false;

	if (!( netgame || multiplayer ))
	{
		if (prevent_cheat)
		{
			/* May want to be more descriptive? */
			CONS_Printf(M_GetText("Cheats must be enabled to level change in single player.\n"));
			return;
		}
		else
		{
			set_cheated = true;
		}
	}

	if (!newresetplayers)
	{
		if (prevent_cheat)
		{
			CONS_Printf(M_GetText("Cheats must be enabled to use -noresetplayers.\n"));
			return;
		}
		else
		{
			set_cheated = true;
		}
	}

	if (option_gametype)
	{
		if (!multiplayer)
		{
			CONS_Printf(M_GetText(
				"You can't switch gametypes in single player!\n"));
			return;
		}
		else if (COM_Argc() < option_gametype + 2)/* no argument after? */
		{
			CONS_Alert(CONS_ERROR,
					"No gametype name follows parameter '%s'.\n",
					COM_Argv(option_gametype));
			return;
		}
	}

	if (!( first_option = COM_FirstOption() ))
	{
		first_option = COM_Argc();
	}

	if (first_option < 2)
	{
		/* I'm going over the fucking lines and I DON'T CAREEEEE */
		CONS_Printf("map <name / [MAP]code / number> [-gametype <type>] [-force]:\n");
		CONS_Printf(M_GetText(
					"Warp to a map, by its name, two character code, with optional \"MAP\" prefix, or by its number (though why would you).\n"
					"All parameters are case-insensitive and may be abbreviated.\n"));
		return;
	}

	mapname = ConcatCommandArgv(1, first_option);

	newmapnum = G_FindMapByNameOrCode(mapname, &realmapname);

	if (newmapnum == 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Could not find any map described as '%s'.\n"), mapname);
		Z_Free(mapname);
		return;
	}

	// new gametype value
	// use current one by default
	if (option_gametype)
	{
		gametypename = COM_Argv(option_gametype + 1);

		newgametype = G_GetGametypeByName(gametypename);

		if (newgametype == -1) // reached end of the list with no match
		{
			/* Did they give us a gametype number? That's okay too! */
			if (isdigit(gametypename[0]))
			{
				d = atoi(gametypename);
				if (d >= 0 && d < gametypecount)
					newgametype = d;
				else
				{
					CONS_Alert(CONS_ERROR,
							"Gametype number %d is out of range. Use a number between"
							" 0 and %d inclusive. ...Or just use the name. :v\n",
							d,
							gametypecount-1);
					Z_Free(realmapname);
					Z_Free(mapname);
					return;
				}
			}
			else
			{
				CONS_Alert(CONS_ERROR,
						"'%s' is not a gametype.\n",
						gametypename);
				Z_Free(realmapname);
				Z_Free(mapname);
				return;
			}
		}
	}

	// don't use a gametype the map doesn't support
	// G_TOLFlag handles both multiplayer gametype and ignores it for !multiplayer
	if (!(
			mapheaderinfo[newmapnum-1] &&
			mapheaderinfo[newmapnum-1]->typeoflevel & G_TOLFlag(newgametype)
	))
	{
		if (prevent_cheat && !cv_skipmapcheck.value)
		{
			CONS_Alert(CONS_WARNING, M_GetText("%s (%s) doesn't support %s mode!\n(Use -force to override)\n"), realmapname, G_BuildMapName(newmapnum),
				(multiplayer ? gametype_cons_t[newgametype].strvalue : "Single Player"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
		else
		{
			// The player wants us to trek on anyway.  Do so.
			fromlevelselect = false;
			set_cheated = ((gametypedefaultrules[newgametype] & GTR_CAMPAIGN) == GTR_CAMPAIGN);
		}
	}
	else
	{
		fromlevelselect =
			( netgame || multiplayer ) &&
			newgametype == gametype    &&
			(gametypedefaultrules[newgametype] & GTR_CAMPAIGN);
	}

	// Prevent warping to locked levels
	if (M_CampaignWarpIsCheat(newgametype, newmapnum, serverGamedata))
	{
		if (prevent_cheat)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Cheats must be enabled to warp to a locked level!\n"));
			Z_Free(realmapname);
			Z_Free(mapname);
			return;
		}
		else
		{
			set_cheated = true;
		}
	}

	// Ultimate Mode only in SP via menu
	if (netgame || multiplayer)
		ultimatemode = false;

	if (tutorialmode && tutorialgcs)
	{
		G_CopyControls(gamecontrol, gamecontroldefault[gcs_custom], gcl_tutorial_full, num_gcl_tutorial_full); // using gcs_custom as temp storage
		CV_SetValue(&cv_usemouse, tutorialusemouse);
		CV_SetValue(&cv_alwaysfreelook, tutorialfreelook);
		CV_SetValue(&cv_mousemove, tutorialmousemove);
		CV_SetValue(&cv_analog[0], tutorialanalog);
	}
	tutorialmode = false; // warping takes us out of tutorial mode

	if (set_cheated && !usedCheats)
	{
		G_SetUsedCheats(false);
	}

	D_MapChange(newmapnum, newgametype, false, newresetplayers, 0, false, fromlevelselect);

	Z_Free(realmapname);
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
	INT16 mapnumber;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal map change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
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

	if (gametype < 0 || gametype >= gametypecount)
		gametype = lastgametype;
	else
		G_SetGametype(gametype);

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

	if (modeattacking)
	{
		SetPlayerSkinByNum(0, cv_chooseskin.value-1);
		players[0].skincolor = skins[players[0].skin]->prefcolor;
	}

	mapnumber = M_MapNumber(mapname[3], mapname[4]);
	LUA_HookInt(mapnumber, HOOK(MapChange));

	G_InitNew(ultimatemode, mapname, resetplayer, skipprecutscene, FLS);
	if (demoplayback && !timingdemo)
		precache = true;
	if (timingdemo)
		G_DoneLevelLoad();

	if (metalrecording)
		G_BeginMetal();
	if (demorecording) // Okay, level loaded, character spawned and skinned,
		G_BeginRecording(); // I AM NOW READY TO RECORD.
	demo_start = true;
}

static void Command_Pause(void)
{
	UINT8 buf[2];
	UINT8 *cp = buf;

	if (COM_Argc() > 1)
		WRITEUINT8(cp, (char)(atoi(COM_Argv(1)) != 0));
	else
		WRITEUINT8(cp, (char)(!paused));

	if (dedicated)
		WRITEUINT8(cp, 1);
	else
		WRITEUINT8(cp, 0);

	if (cv_pause.value || server || (IsPlayerAdmin(consoleplayer)))
	{
		if (modeattacking || !(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_WAITINGPLAYERS) || (marathonmode && gamestate == GS_INTERMISSION))
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

	if (netgame && !cv_pause.value && playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal pause command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
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

	I_UpdateMouseGrab();
}

// Command for stuck characters in netgames, griefing, etc.
static void Command_Suicide(void)
{
	UINT8 buf[4];
	UINT8 *cp = buf;

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

	WRITEINT32(cp, consoleplayer);
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
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	if (players[suicideplayer].mo)
		P_DamageMobj(players[suicideplayer].mo, NULL, NULL, 1, DMG_INSTAKILL);
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
	if (!(server || (IsPlayerAdmin(consoleplayer))))
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
	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal clear scores command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		players[i].score = players[i].recordscore = 0;

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
	if ((gametyperules & GTR_HIDEFROZEN) && leveltime >= (hidetime * TICRATE))
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
	if ((gametyperules & GTR_HIDEFROZEN) && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd2(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void MutePlayer(boolean mute)
{
	UINT8 data[2];
	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("muteplayer <playername/playernum>: mute a player\n"));
		return;
	}

	data[0] = nametonum(COM_Argv(1));
	if (data[0] >= MAXPLAYERS || !playeringame[data[0]])
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %u!\n"), (unsigned int)data[0]);
		return;
	}

	if (players[data[0]].muted && mute)
	{
		CONS_Printf(M_GetText("%s is already muted!\n"), player_names[data[0]]);
		return;
	}
	else if (!players[data[0]].muted && !mute)
	{
		CONS_Printf(M_GetText("%s is not muted!\n"), player_names[data[0]]);
		return;
	}

	data[1] = mute;
	SendNetXCmd(XD_MUTEPLAYER, &data, sizeof(data));
}

static void Command_MutePlayer_f(void)
{
	MutePlayer(true);
}

static void Command_UnmutePlayer_f(void)
{
	MutePlayer(false);
}

static void Got_MutePlayer(UINT8 **cp, INT32 playernum)
{
	UINT8 player = READUINT8(*cp);
	UINT8 muted = READUINT8(*cp);
	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal mute received from player %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	if (player >= MAXPLAYERS || !playeringame[player])
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal mute received from player %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	if (!players[player].muted && muted)
	{
		if (player == consoleplayer)
			CONS_Printf(M_GetText("You have been muted.\n"));
		else
			CONS_Printf(M_GetText("%s has been muted.\n"), player_names[player]);
	}
	else if (players[player].muted && !muted)
	{
		if (player == consoleplayer)
			CONS_Printf(M_GetText("You are no longer muted.\n"));
		else
			CONS_Printf(M_GetText("%s is no longer muted.\n"), player_names[player]);
	}

	players[player].muted = muted;
}

static void Command_ServerTeamChange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	//        0              1         2
	// serverchangeteam <playernum>  <team>

	if (COM_Argc() < 3)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("serverchangeteam <playername/playernum> <team>: switch player to a new team (%s)\n"), "it, notit, playing, or spectator");
		else if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playername/playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playername/playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
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
			CONS_Printf(M_GetText("serverchangeteam <playername/playernum> <team>: switch player to a new team (%s)\n"), "it, notit, playing, or spectator");
		else if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playername/playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playername/playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		return;
	}

	NetPacket.packet.playernum = nametonum(COM_Argv(1));

	if (NetPacket.packet.playernum == -1 || !playeringame[NetPacket.packet.playernum])
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %s!\n"), COM_Argv(1));
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
	if ((gametyperules & GTR_HIDEFROZEN) && leveltime >= (hidetime * TICRATE))
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
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
	}

	if (NetPacket.packet.verification) // Special marker that the server sent the request
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
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
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		}
		return;
	}

	// Don't switch team, just go away, please, go awaayyyy, aaauuauugghhhghgh
	if (!LUA_HookTeamSwitch(&players[playernum], NetPacket.packet.newteam, players[playernum].spectator, NetPacket.packet.autobalance, NetPacket.packet.scrambled))
		return;

	//no status changes after hidetime
	if ((gametyperules & GTR_HIDEFROZEN) && (leveltime >= (hidetime * TICRATE)))
		error = true;

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
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
	}

	//Safety first!
	if (players[playernum].mo)
	{
		if (!players[playernum].spectator)
			P_DamageMobj(players[playernum].mo, NULL, NULL, 1, DMG_INSTAKILL);
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
			players[playernum].pflags &= ~PF_GAMETYPEOVER;
		}
		else if (NetPacket.packet.newteam != 3) // .newteam == 1 or 2.
		{
			players[playernum].spectator = false;
			players[playernum].pflags &= ~PF_GAMETYPEOVER; //Just in case.

			if (NetPacket.packet.newteam == 1) //Make the player IT.
				players[playernum].pflags |= PF_TAGIT;
			else
				players[playernum].pflags &= ~PF_TAGIT;
		}
		else // Just join the game.
		{
			players[playernum].spectator = false;

			//If joining after hidetime in normal tag, default to being IT.
			if (((gametyperules & (GTR_TAG|GTR_HIDEFROZEN)) == GTR_TAG) && (leveltime > (hidetime * TICRATE)))
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
	{
		// Call ViewpointSwitch hooks here.
		// The viewpoint was forcibly changed.
		if (displayplayer != consoleplayer) // You're already viewing yourself. No big deal.
			LUA_HookViewpointSwitch(&players[consoleplayer], &players[consoleplayer], true);
		displayplayer = consoleplayer;
	}

	// In tag, check to see if you still have a game.
	if (G_TagGametype())
		P_CheckSurvivors();
}

//
// Attempts to make password system a little sane without
// rewriting the entire goddamn XD_file system
//
#define BASESALT "basepasswordstorage"

void D_SetPassword(const char *pw)
{
	adminpassmd5 = Z_Realloc(adminpassmd5, sizeof(*adminpassmd5) * ++adminpasscount, PU_STATIC, NULL);
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &adminpassmd5[adminpasscount-1]);
}

void D_ClearPassword(void)
{
	Z_Free(adminpassmd5);
	adminpassmd5 = NULL;
	adminpasscount = 0;
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
		CONS_Printf(M_GetText("password <password>: add remote admin password\n"));
		return;
	}

	D_SetPassword(COM_Argv(1));
	CONS_Printf(M_GetText("Password added.\n"));
#endif
}

// Remote Administration
static void Command_Clearpassword_f(void)
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

	D_ClearPassword();
	CONS_Printf(M_GetText("Passwords cleared.\n"));
#endif
}

static void Command_Login_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	const char *pw;

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("login <password>: Administrator login\n"));
		return;
	}

	pw = COM_Argv(1);

	// Do the base pass to get what the server has (or should?)
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &netbuffer->u.md5sum);

	// Do the final pass to get the comparison the server will come up with
	D_MD5PasswordPass(netbuffer->u.md5sum, 16, va("PNUM%02d", consoleplayer), &netbuffer->u.md5sum);

	CONS_Printf(M_GetText("Sending login... (Notice only given if password is correct.)\n"));

	netbuffer->packettype = PT_LOGIN;
	HSendPacket(servernode, true, 0, 16);
#endif
}

boolean IsPlayerAdmin(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			return true;

	return false;
}

void SetAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playernum == adminplayers[i])
			return; // Player is already admin

		if (adminplayers[i] == -1)
		{
			adminplayers[i] = playernum; // Set the player to a free spot
			break; // End the loop now. If it keeps going, the same player might get assigned to two slots.
		}


	}
}

void ClearAdminPlayers(void)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		adminplayers[i] = -1;
}

void RemoveAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			adminplayers[i] = -1;
}

static void Command_Verify_f(void)
{
	char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("promote <playername/playernum>: give admin privileges to a player\n"));
		return;
	}

	playernum = nametonum(COM_Argv(1));
	if (playernum == -1)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %s!\n"), COM_Argv(1));
		return;
	}

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
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	SetAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are now a server administrator.\n"));
}

static void Command_RemoveAdmin_f(void)
{
	char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("demote <playername/playernum>: remove admin privileges from a player\n"));
		return;
	}

	playernum = nametonum(COM_Argv(1));
	if (playernum == -1)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %s!\n"), COM_Argv(1));
		return;
	}

	temp = buf;

	WRITEUINT8(temp, playernum);

	if (playeringame[playernum])
		SendNetXCmd(XD_DEMOTED, buf, 1);
}

static void Got_Removal(UINT8 **cp, INT32 playernum)
{
	INT16 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal demotion received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	RemoveAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are no longer a server administrator.\n"));
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

	if (!(server || (IsPlayerAdmin(consoleplayer))))
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
		SendNetXCmd(XD_SETMOTD, mymotd, i); // send the actual size of the motd string, not the full buffer's size
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

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal motd change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
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
	char buf[255];
	size_t length = 0;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("runsoc <socfile.soc> or <lumpname>: run a soc\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
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
	filestatus_t ncs = FS_NOTCHECKED;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal runsoc command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
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

// C++ would make this SO much simpler!
typedef struct addedfile_s
{
	struct addedfile_s *next;
	struct addedfile_s *prev;
	char *value;
} addedfile_t;

static boolean AddedFileContains(addedfile_t *list, const char *value)
{
	addedfile_t *node;
	for (node = list; node; node = node->next)
	{
		if (!strcmp(value, node->value))
			return true;
	}

	return false;
}

static void AddedFilesAdd(addedfile_t **list, const char *value)
{
	addedfile_t *item = Z_Calloc(sizeof(addedfile_t), PU_STATIC, NULL);
	item->value = Z_StrDup(value);
	ListAdd(item, (listitem_t**)list);
}

static void AddedFilesRemove(void *pItem, addedfile_t **itemHead)
{
	addedfile_t *item = (addedfile_t *)pItem;

	if (item == *itemHead) // Start of list
	{
		*itemHead = item->next;

		if (*itemHead)
			(*itemHead)->prev = NULL;
	}
	else if (item->next == NULL) // end of list
	{
		item->prev->next = NULL;
	}
	else // Somewhere in between
	{
		item->prev->next = item->next;
		item->next->prev = item->prev;
	}

	Z_Free(item->value);
	Z_Free(item);
}

static void AddedFilesClearList(addedfile_t **itemHead)
{
	addedfile_t *item;
	addedfile_t *next;
	for (item = *itemHead; item; item = next)
	{
		next = item->next;
		AddedFilesRemove(item, itemHead);
	}
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  */
static void Command_Addfile(void)
{
	size_t argc = COM_Argc(); // amount of arguments total
	size_t curarg; // current argument index

	addedfile_t *addedfiles = NULL; // list of filenames already processed

	if (argc < 2)
	{
		CONS_Printf(M_GetText("addfile <filename.pk3/wad/lua/soc> [filename2...] [...]: Load add-ons\n"));
		return;
	}

	// start at one to skip command name
	for (curarg = 1; curarg < argc; curarg++)
	{
		const char *fn, *p;
		char buf[256];
		char *buf_p = buf;
		INT32 i;
		int musiconly; // W_VerifyNMUSlumps isn't boolean
		boolean fileadded = false;

		fn = COM_Argv(curarg);

		// For the amount of filenames previously processed...
		fileadded = AddedFileContains(addedfiles, fn);
		if (fileadded) // If this is one of them, don't try to add it.
		{
			CONS_Alert(CONS_WARNING, M_GetText("Already processed %s, skipping\n"), fn);
			continue;
		}

		// Disallow non-printing characters and semicolons.
		for (i = 0; fn[i] != '\0'; i++)
			if (!isprint(fn[i]) || fn[i] == ';')
			{
				AddedFilesClearList(&addedfiles);
				return;
			}

		musiconly = W_VerifyNMUSlumps(fn, false);

		if (musiconly == -1)
		{
			AddedFilesAdd(&addedfiles, fn);
			continue;
		}

		if (!musiconly)
		{
			// ... But only so long as they contain nothing more then music and sprites.
			if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
			{
				CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
				continue;
			}
			G_SetGameModified(multiplayer);
		}

		// Add file on your client directly if it is trivial, or you aren't in a netgame.
		if (!(netgame || multiplayer) || musiconly)
		{
			P_AddWadFile(fn);
			AddedFilesAdd(&addedfiles, fn);
			continue;
		}

		p = fn+strlen(fn);
		while(--p >= fn)
			if (*p == '\\' || *p == '/' || *p == ':')
				break;
		++p;

		// check no of files currently loaded
		// See W_LoadWadFile in w_wad.c
		if (numwadfiles >= MAX_WADFILES)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Too many files loaded to add %s\n"), fn);
			AddedFilesClearList(&addedfiles);
			return;
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
				continue;

			for (i = 0; i < numwadfiles; i++)
			{
				if (wadfiles[i]->type == RET_FOLDER)
					continue;

				if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
				{
					CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), fn);
					continue;
				}
			}
#endif
			WRITEMEM(buf_p, md5sum, 16);
		}

		AddedFilesAdd(&addedfiles, fn);

		if (IsPlayerAdmin(consoleplayer) && (!server)) // Request to add file
			SendNetXCmd(XD_REQADDFILE, buf, buf_p - buf);
		else
			SendNetXCmd(XD_ADDFILE, buf, buf_p - buf);
	}

	AddedFilesClearList(&addedfiles);
}

static void Command_Addfolder(void)
{
	size_t argc = COM_Argc(); // amount of arguments total
	size_t curarg; // current argument index

	addedfile_t *addedfolders = NULL; // list of filenames already processed

	if (argc < 2)
	{
		CONS_Printf(M_GetText("addfolder <path> [path2...] [...]: Load add-ons\n"));
		return;
	}

	// start at one to skip command name
	for (curarg = 1; curarg < argc; curarg++)
	{
		const char *fn, *p;
		char *fullpath;
		char buf[256];
		char *buf_p = buf;
		INT32 i, stat;
		boolean folderadded = false;

		fn = COM_Argv(curarg);

		// For the amount of filenames previously processed...
		folderadded = AddedFileContains(addedfolders, fn);
		if (folderadded) // If we've added this one, skip to the next one.
		{
			CONS_Alert(CONS_WARNING, M_GetText("Already processed %s, skipping\n"), fn);
			continue;
		}

		// Disallow non-printing characters and semicolons.
		for (i = 0; fn[i] != '\0'; i++)
			if (!isprint(fn[i]) || fn[i] == ';')
			{
				AddedFilesClearList(&addedfolders);
				return;
			}

		// Add file on your client directly if you aren't in a netgame.
		if (!(netgame || multiplayer))
		{
			P_AddFolder(fn);
			AddedFilesAdd(&addedfolders, fn);
			continue;
		}

		p = fn+strlen(fn);
		while(--p >= fn)
			if (*p == '\\' || *p == '/' || *p == ':')
				break;
		++p;

		// Don't add an empty path.
		if (M_IsStringEmpty(fn))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Folder name is empty, skipping\n"));
			continue;
		}

		// check no of files currently loaded
		// See W_LoadWadFile in w_wad.c
		if (numwadfiles >= MAX_WADFILES)
		{
			CONS_Alert(CONS_ERROR, M_GetText("Too many files loaded to add %s\n"), fn);
			AddedFilesClearList(&addedfolders);
			return;
		}

		// Check if the path is valid.
		stat = W_IsPathToFolderValid(fn);

		if (stat == 0)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Path %s is invalid, skipping\n"), fn);
			continue;
		}
		else if (stat < 0)
		{
#ifndef AVOID_ERRNO
			CONS_Alert(CONS_WARNING, M_GetText("Error accessing %s (%s), skipping\n"), fn, strerror(direrror));
#else
			CONS_Alert(CONS_WARNING, M_GetText("Error accessing %s, skipping\n"), fn);
#endif
			continue;
		}

		// Get the full path for this folder.
		fullpath = W_GetFullFolderPath(fn);

		if (fullpath == NULL)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Path %s is invalid, skipping\n"), fn);
			continue;
		}

		// Check if the folder is already added.
		for (i = 0; i < numwadfiles; i++)
		{
			if (wadfiles[i]->type != RET_FOLDER)
				continue;

			if (samepaths(wadfiles[i]->path, fullpath) > 0)
			{
				CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), fn);
				continue;
			}
		}

		Z_Free(fullpath);

		AddedFilesAdd(&addedfolders, fn);

		WRITESTRINGN(buf_p,p,240);

		if (IsPlayerAdmin(consoleplayer) && (!server)) // Request to add file
			SendNetXCmd(XD_REQADDFOLDER, buf, buf_p - buf);
		else
			SendNetXCmd(XD_ADDFOLDER, buf, buf_p - buf);
	}
}

static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTCHECKED;
	UINT8 md5sum[16];
	boolean kick = false;
	boolean toomany = false;
	INT32 i,j;

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	// Only the server processes this message.
	if (client)
		return;

	// Disallow non-printing characters and semicolons.
	for (i = 0; filename[i] != '\0'; i++)
		if (!isprint(filename[i]) || filename[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	if (numwadfiles >= MAX_WADFILES)
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

		for (j = 0; j < MAXPLAYERS; j++)
			if (adminplayers[j])
				COM_BufAddText(va("sayto %d %s", adminplayers[j], message));

		return;
	}

	COM_BufAddText(va("addfile %s\n", filename));
}

static void Got_RequestAddfoldercmd(UINT8 **cp, INT32 playernum)
{
	char path[241];
	filestatus_t ncs = FS_NOTCHECKED;
	boolean kick = false;
	boolean toomany = false;
	INT32 i,j;

	READSTRINGN(*cp, path, 240);

	/// \todo Integrity checks.

	// Only the server processes this message.
	if (client)
		return;

	// Disallow non-printing characters and semicolons.
	for (i = 0; path[i] != '\0'; i++)
		if (!isprint(path[i]) || path[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfolder command received from %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	if (numwadfiles >= MAX_WADFILES)
		toomany = true;
	else
		ncs = findfolder(path);

	if (ncs != FS_FOUND || toomany)
	{
		char message[256];

		if (toomany)
			sprintf(message, M_GetText("Too many files loaded to add %s\n"), path);
		else if (ncs == FS_NOTFOUND)
			sprintf(message, M_GetText("The server doesn't have %s\n"), path);
		else
			sprintf(message, M_GetText("Unknown error finding folder (%s)\n"), path);

		CONS_Printf("%s",message);

		for (j = 0; j < MAXPLAYERS; j++)
			if (adminplayers[j])
				COM_BufAddText(va("sayto %d %s", adminplayers[j], message));

		return;
	}

	COM_BufAddText(va("addfolder \"%s\"\n", path));
}

static void Got_Addfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[MAX_WADPATH+1];
	filestatus_t ncs = FS_NOTCHECKED;
	UINT8 md5sum[16];

	READSTRINGN(*cp, filename, MAX_WADPATH);
	READMEM(*cp, md5sum, 16);

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || !P_AddWadFile(filename))
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

static void Got_Addfoldercmd(UINT8 **cp, INT32 playernum)
{
	char path[241];
	filestatus_t ncs = FS_NOTCHECKED;

	READSTRINGN(*cp, path, 240);

	/// \todo Integrity checks.

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfolder command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
		return;
	}

	ncs = findfolder(path);

	if (ncs != FS_FOUND || !P_AddFolder(path))
	{
		Command_ExitGame_f();
		if (ncs == FS_FOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you have too many files added.\nRestart the game to clear loaded files\nand play on this server."), path);
			M_StartMessage(va("The server added a folder \n(%s)\nbut you have too many files added.\nRestart the game to clear loaded files.\n\nPress ESC\n",path), NULL, MM_NOTHING);
		}
		else if (ncs == FS_NOTFOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server."), path);
			M_StartMessage(va("The server added a folder \n(%s)\nthat you do not have.\n\nPress ESC\n",path), NULL, MM_NOTHING);
		}
		else
		{
			CONS_Printf(M_GetText("Unknown error finding folder (%s) the server added.\n"), path);
			M_StartMessage(va("Unknown error trying to load a folder\nthat the server added \n(%s).\n\nPress ESC\n",path), NULL, MM_NOTHING);
		}
		return;
	}

	G_SetGameModified(true);
}

static void Command_ListWADS_f(void)
{
	INT32 i = numwadfiles;
	char *tempname;

#ifdef ENFORCE_WAD_LIMIT
	CONS_Printf(M_GetText("There are %d/%d files loaded:\n"),numwadfiles,MAX_WADFILES);
#else
	CONS_Printf(M_GetText("There are %d files loaded:\n"),numwadfiles);
#endif

	for (i--; i >= 0; i--)
	{
		nameonly(tempname = va("%s", wadfiles[i]->filename));
		if (!i)
			CONS_Printf("\x82 IWAD\x80: %s\n", tempname);
		else if (i < mainwads)
			CONS_Printf("\x82 * %.2d\x80: %s\n", i, tempname);
		else if (!wadfiles[i]->important)
			CONS_Printf("\x86   %.2d: %s\n", i, tempname);
		else if (wadfiles[i]->type == RET_FOLDER)
			CONS_Printf("\x82 * %.2d\x84: %s\n", i, tempname);
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
	CONS_Printf("Sonic Robo Blast 2 %s %s %s (%s %s) ", compbranch, comprevision, compnote, compdate, comptime);
#else
	CONS_Printf("Sonic Robo Blast 2 %s (%s %s %s %s) ", VERSIONSTRING, compdate, comptime, comprevision, compbranch);
#endif

	// Base library
#if defined( HAVE_SDL)
	CONS_Printf("SDL ");
#elif defined(_WINDOWS)
	CONS_Printf("DD ");
#endif

	// OS
	CONS_Printf("%s ", I_GetSysName());

	// Bitness
	if (sizeof(void*) == 4)
		CONS_Printf("32-bit ");
	else if (sizeof(void*) == 8)
		CONS_Printf("64-bit ");
	else // 16-bit? 128-bit?
		CONS_Printf("Bits Unknown ");

	// Debug build
#ifdef _DEBUG
	CONS_Printf("\x85" "DEBUG " "\x80");
#endif

	// DEVELOP build
#ifdef DEVELOP
	CONS_Printf("\x87" "DEVELOP " "\x80");
#endif

	CONS_Printf("\n");
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
	const char *gametypestr = NULL;

	if (!(netgame || multiplayer)) // print "Single player" instead of "Co-op"
	{
		CONS_Printf(M_GetText("Current gametype is %s\n"), M_GetText("Single player"));
		return;
	}

	// get name string for current gametype
	if (gametype >= 0 && gametype < gametypecount)
		gametypestr = Gametype_Names[gametype];

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

	if (dirmenu)
		closefilemenu(true);

	F_StartIntro();
}

/** Quits the game immediately.
  */
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void)
{
	LUA_HookBool(true, HOOK(GameQuit));
	I_Quit();
}

void ItemFinder_OnChange(void)
{
	if (!cv_itemfinder.value)
		return; // it's fine.

	if (!M_SecretUnlocked(SECRET_ITEMFINDER, clientGamedata))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
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
	if (server && Playing() && !(gametyperules & GTR_POINTLIMIT))
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
	if ((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE)
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

static void CoopStarposts_OnChange(void)
{
	INT32 i;

	if (!(netgame || multiplayer) || !G_GametypeUsesCoopStarposts())
		return;

	switch (cv_coopstarposts.value)
	{
		case 0:
			CONS_Printf(M_GetText("Starposts are now per-player.\n"));
			break;
		case 1:
			CONS_Printf(M_GetText("Starposts are now shared between players.\n"));
			break;
		case 2:
			CONS_Printf(M_GetText("Players now only spawn when starposts are hit.\n"));
			return;
	}

	if (G_IsSpecialStage(gamemap))
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (!players[i].spectator)
			continue;

		if (players[i].lives <= 0)
			continue;

		break;
	}

	if (i == MAXPLAYERS)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (!players[i].spectator)
			continue;

		if (players[i].lives <= 0 && (cv_cooplives.value == 1))
			continue;

		P_SpectatorJoinGame(&players[i]);
	}
}

static void CoopLives_OnChange(void)
{
	INT32 i;

	if (!(netgame || multiplayer) || !G_GametypeUsesCoopLives())
		return;

	switch (cv_cooplives.value)
	{
		case 0:
			CONS_Printf(M_GetText("Players can now respawn indefinitely.\n"));
			break;
		case 1:
			CONS_Printf(M_GetText("Lives are now per-player.\n"));
			return;
		case 2:
			CONS_Printf(M_GetText("Players can now steal lives to avoid game over.\n"));
			break;
		case 3:
			CONS_Printf(M_GetText("Lives are now shared between players.\n"));
			break;
	}

	if (cv_coopstarposts.value == 2)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (!players[i].spectator)
			continue;

		if (players[i].lives > 0)
			continue;

		P_SpectatorJoinGame(&players[i]);
	}
}

static void ExitMove_OnChange(void)
{
	UINT8 i;

	if (!(netgame || multiplayer) || !(gametyperules & GTR_FRIENDLY))
		return;

	if (cv_exitmove.value)
	{
		for (i = 0; i < MAXPLAYERS; ++i)
			if (playeringame[i] && players[i].mo)
			{
				if (players[i].mo->target && players[i].mo->target->type == MT_SIGN)
					P_SetTarget(&players[i].mo->target, NULL);

				if (players[i].pflags & PF_FINISHED && !(players[i].exiting))
					P_GiveFinishFlags(&players[i]);
			}

		CONS_Printf(M_GetText("Players can now move after completing the level.\n"));
	}
	else
		CONS_Printf(M_GetText("Players can no longer move after completing the level.\n"));
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
	if (server && Playing() && cv_timelimit.value != 0 && !(gametyperules & GTR_TIMELIMIT))
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
		const char *oldgt = NULL, *newgt = NULL;

		if (lastgametype >= 0 && lastgametype < gametypecount)
			oldgt = Gametype_Names[lastgametype];
		if (gametype >= 0 && lastgametype < gametypecount)
			newgt = Gametype_Names[gametype];

		if (oldgt && newgt)
			CONS_Printf(M_GetText("Gametype was changed from %s to %s\n"), oldgt, newgt);
	}
	// Only do the following as the server, not as remote admin.
	// There will always be a server, and this only needs to be done once.
	if (server && (multiplayer || netgame))
	{
		if (gametype == GT_COMPETITION)
			CV_SetValue(&cv_itemrespawn, 0);
		else if (!cv_itemrespawn.changed || lastgametype == GT_COMPETITION)
			CV_SetValue(&cv_itemrespawn, 1);

		switch (gametype)
		{
			case GT_COOP:
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
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
			default:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					CV_SetValue(&cv_timelimit, timelimits[gametype]);
					CV_SetValue(&cv_pointlimit, pointlimits[gametype]);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
		}
	}
	else if (!multiplayer && !netgame)
	{
		G_SetGametype(GT_COOP);
	}

	// reset timelimit and pointlimit in race/coop, prevent stupid cheats
	if (server)
	{
		if (!(gametyperules & GTR_POINTLIMIT))
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
	// Averted with GTR_NOSPECTATORSPAWN.
	if (!splitscreen && (G_GametypeHasSpectators()))
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				players[i].ctfteam = 0;
				players[i].spectator = (gametyperules & GTR_NOSPECTATORSPAWN) ? false : true;
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

		if (server || (IsPlayerAdmin(consoleplayer)))
		{
			CV_StealthSetValue(&cv_teamscramble, 0);
			teamscramble = 0;
		}
	}
}

static void Ringslinger_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA, serverGamedata) && !netgame && cv_ringslinger.value && !cv_debug)
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_ringslinger, 0);
		return;
	}

	if (cv_ringslinger.value) // Only if it's been turned on
		G_SetUsedCheats(false);
}

static void Gravity_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA, serverGamedata) && !netgame && !cv_debug
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
		G_SetUsedCheats(false);
	gravity = cv_gravity.value;
}

static void SoundTest_OnChange(void)
{
	INT32 sfxfreeint = (INT32)sfxfree;
	if (cv_soundtest.value < 0)
	{
		CV_SetValue(&cv_soundtest, sfxfreeint-1);
		return;
	}

	if (cv_soundtest.value >= sfxfreeint)
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

	if (!G_GametypeHasTeams() && (server || IsPlayerAdmin(consoleplayer)))
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
				else //if (blue == maxcomposition)
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
			else if (i != 2) // Mystic's secret sauce - ABBA is better than ABAB, so team B doesn't get worse players all around
			{
				// We will only randomly pick the team for the first guy.
				// Otherwise, just alternate back and forth, distributing players.
				newteam = 3 - newteam;
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
	if (!(server || (IsPlayerAdmin(consoleplayer))))
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
	else if (( gamestate != GS_LEVEL && gamestate != GS_CREDITS ) || demoplayback)
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else if (usedCheats)
		SendNetXCmd(XD_EXITLEVEL, NULL, 0); // Does it matter if it's a cheat if we've used one already?
	else if (!(splitscreen || multiplayer || cv_debug))
		CONS_Printf(M_GetText("Cheats must be enabled to force a level exit in single player.\n"));
	else
	{
		if (G_IsSpecialStage(gamemap) // If you wanna give up that emerald then go right ahead!
			|| gamestate == GS_CREDITS) // Somebody hasn't heard of the credits skip button...
		{
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			return;
		}

		// Allow exiting without cheating if at least one player beat the level
		// Consistent with just setting playersforexit to one
		if (splitscreen || multiplayer)
		{
			INT32 i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator || players[i].bot)
					continue;
				if (players[i].quittime > 30 * TICRATE)
					continue;
				if (players[i].lives <= 0)
					continue;

				if ((players[i].pflags & PF_FINISHED) || players[i].exiting)
				{
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);
					return;
				}
			}
		}

		// Only consider it a cheat if we're not allowed to go to the next map
		if (M_CampaignWarpIsCheat(gametype, G_GetNextMap(true, true) + 1, serverGamedata))
			CONS_Alert(CONS_NOTICE, M_GetText("Cheats must be enabled to force exit to a locked level!\n"));
		else
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}
}

static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum)
{
	(void)cp;

	// Ignore duplicate XD_EXITLEVEL commands.
	if (gameaction == ga_completed)
		return;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal exitlevel command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
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

/** Quits a game and returns to the title screen.
  *
  */
void Command_ExitGame_f(void)
{
	INT32 i;

	LUA_HookBool(false, HOOK(GameQuit));

	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	players[consoleplayer].availabilities = players[1].availabilities = R_GetSkinAvailabilities(); // players[1] is supposed to be for 2p

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	cv_debug = 0;
	emeralds = 0;
	automapactive = false;
	memset(&luabanks, 0, sizeof(luabanks));

	if (dirmenu)
		closefilemenu(true);

	if (!modeattacking)
		D_StartTitle();
}

void Command_Retry_f(void)
{
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("This only works in single player.\n"));
	else if (players[consoleplayer].lives <= 1)
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
		G_SetUsedCheats(false);
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
		CONS_Printf(M_GetText("modifiedgame is true, but you can save time data in this mod.\n"));
	else if (modifiedgame)
		CONS_Printf(M_GetText("modifiedgame is true, time data can't be saved\n"));
	else
		CONS_Printf(M_GetText("modifiedgame is false, you can save time data\n"));
}

static void Command_Cheats_f(void)
{
	if (COM_CheckParm("off"))
	{
		if (!(server || (IsPlayerAdmin(consoleplayer))))
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		else
			CV_ResetCheatNetVars();
		return;
	}
	else if (COM_CheckParm("on"))
	{
		if (!(server || (IsPlayerAdmin(consoleplayer))))
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		else
			G_SetUsedCheats(false);
		return;
	}

	if (usedCheats)
		CONS_Printf(M_GetText("Cheats are enabled, the game cannot be saved.\n"));
	else
		CONS_Printf(M_GetText("Cheats are disabled, the game can be saved.\n"));

	if (CV_CheatsEnabled())
	{
		CONS_Printf(M_GetText("At least one CHEAT-marked variable has been changed.\n"));
		if (server || (IsPlayerAdmin(consoleplayer)))
			CONS_Printf(M_GetText("Type CHEATS OFF to reset all cheat variables to default.\n"));
	}
	else
		CONS_Printf(M_GetText("No CHEAT-marked variables are changed.\n"));
}

#ifdef _DEBUG
static void Command_Togglemodified_f(void)
{
	modifiedgame = !modifiedgame;
}

static void Command_Archivetest_f(void)
{
	UINT32 i, wrote;
	thinker_t *th;
	save_t savebuffer;
	if (gamestate != GS_LEVEL)
	{
		CONS_Printf("This command only works in-game, you dummy.\n");
		return;
	}

	// assign mobjnum
	i = 1;
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		if (th->function.acp1 != (actionf_p1)P_RemoveThinkerDelayed)
			((mobj_t *)th)->mobjnum = i++;

	// allocate buffer
	savebuffer.size = 1024;
	savebuffer.buf = malloc(savebuffer.size);
	savebuffer.pos = 0;

	// test archive
	CONS_Printf("LUA_Archive...\n");
	LUA_Archive(&savebuffer);
	P_WriteUINT8(&savebuffer, 0x7F);
	wrote = savebuffer.pos;

	// clear Lua state, so we can really see what happens!
	CONS_Printf("Clearing state!\n");
	LUA_ClearExtVars();

	// test unarchive
	CONS_Printf("LUA_UnArchive...\n");
	LUA_UnArchive(&savebuffer);
	i = P_ReadUINT8(&savebuffer);
	if (i != 0x7F || wrote != (UINT32)(savebuffer.pos))
		CONS_Printf("Savegame corrupted. (write %u, read %u)\n", wrote, (UINT32)(savebuffer.pos));

	// free buffer
	free(savebuffer.buf);
	CONS_Printf("Done. No crash.\n");
}
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
	if ((server || IsPlayerAdmin(consoleplayer)) && (cv_forceskin.value < -1 || cv_forceskin.value >= numskins))
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
	{
		CONS_Printf("The server has lifted the forced skin restrictions.\n");
		if (Playing())
			D_SendPlayerConfig();
	}
	else
	{
		CONS_Printf("The server is restricting all players to skin \"%s\".\n",skins[cv_forceskin.value]->name);
		if (Playing())
			ForceAllSkins(cv_forceskin.value);
	}
}

//Allows the player's name to be changed if cv_mute is off.
static void Name_OnChange(void)
{
	if ((cv_mute.value || players[consoleplayer].muted) && !(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
	}
	else
		SendNameAndColor();
}

static void Name2_OnChange(void)
{
	if (cv_mute.value || players[consoleplayer].muted) //Secondary player can't be admin.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername2, player_names[secondarydisplayplayer]);
	}
	else
		SendNameAndColor2();
}

static boolean Skin_CanChange(const char *valstr)
{
	if (!Playing())
		return true; // do whatever you want

	// You already are that skin.
	if (stricmp(skins[players[consoleplayer].skin]->name, valstr) == 0)
		return false;

	if (!(multiplayer || netgame)) // In single player.
		return true;

	if (CanChangeSkin(consoleplayer) && !P_PlayerMoving(consoleplayer))
		return true;
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		return false;
	}
}

static boolean Skin2_CanChange(const char *valstr)
{
	if (!Playing() || !splitscreen)
		return true; // do whatever you want

	// You already are that skin.
	if (stricmp(skins[players[secondarydisplayplayer].skin]->name, valstr) == 0)
		return false;

	if (CanChangeSkin(secondarydisplayplayer) && !P_PlayerMoving(secondarydisplayplayer))
		return true;
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		return false;
	}
}

/** Sends a skin change for the console player, unless that player is moving.
  * \sa cv_skin, Skin2_OnChange, Color_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(void)
{
	pickedchar = R_SkinAvailable(cv_skin.string);

	if (!Playing())
		return;

	if (!(multiplayer || netgame)) // In single player.
	{
		if (!(cv_debug || devparm)
		&& (gamestate != GS_WAITINGPLAYERS)) // allows command line -warp x +skin y
		{
			CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin]->name);
			return;
		}

		// Just do it here if devmode is enabled
		SetSkinLocal(consoleplayer, R_SkinAvailable(cv_skin.string));
		return;
	}

	SendNameAndColor();
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_skin2, Skin_OnChange, Color2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin2_OnChange(void)
{
	if (!Playing() || !splitscreen)
		return;

	SendNameAndColor2();
}

/** Sends a color change for the console player, unless that player is moving.
  * \sa cv_playercolor, Color2_OnChange, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color_OnChange(void)
{
	if (!Playing())
	{
		if (!cv_playercolor.value || !skincolors[cv_playercolor.value].accessible)
			CV_StealthSetValue(&cv_playercolor, lastgoodcolor);
	}
	else
	{
		if (!(multiplayer || netgame)) // In single player.
		{
			// Just do it here if devmode is enabled
			if (cv_debug || devparm)
				SetColorLocal(consoleplayer, cv_playercolor.value);
			return;
		}

		if (!P_PlayerMoving(consoleplayer) && skincolors[players[consoleplayer].skincolor].accessible == true)
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
	lastgoodcolor = cv_playercolor.value;
}

/** Sends a color change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_playercolor2, Color_OnChange, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color2_OnChange(void)
{
	if (!Playing() || !splitscreen)
	{
		if (!cv_playercolor2.value || !skincolors[cv_playercolor2.value].accessible)
			CV_StealthSetValue(&cv_playercolor2, lastgoodcolor2);
	}
	else
	{
		if (!P_PlayerMoving(secondarydisplayplayer) && skincolors[players[secondarydisplayplayer].skincolor].accessible == true)
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
	lastgoodcolor2 = cv_playercolor2.value;
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
	if (server || (IsPlayerAdmin(consoleplayer)))
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

static void BaseNumLaps_OnChange(void)
{
	if ((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE)
	{
		if (cv_basenumlaps.value)
			CONS_Printf(M_GetText("Number of laps will be changed to map defaults next round.\n"));
		else
			CONS_Printf(M_GetText("Number of laps will be changed to %d next round.\n"), cv_basenumlaps.value);
	}
}
