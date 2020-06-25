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
/// \file  g_game.h
/// \brief Game loop, events handling.

#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"
#include "g_demo.h"

extern char gamedatafilename[64];
extern char timeattackfolder[64];
extern char customversionstring[32];
#define GAMEDATASIZE (4*8192)

#ifdef SEENAMES
extern player_t *seenplayer;
#endif
extern char player_names[MAXPLAYERS][MAXPLAYERNAME+1];

extern player_t players[MAXPLAYERS];
extern boolean playeringame[MAXPLAYERS];

// gametic at level start
extern tic_t levelstarttic;

// for modding?
extern INT16 prevmap, nextmap;
extern INT32 gameovertics;
extern UINT8 ammoremovaltics;
extern tic_t timeinmap; // Ticker for time spent in level (used for levelcard display)
extern INT16 rw_maximums[NUM_WEAPONS];
extern INT32 pausedelay;
extern boolean pausebreakkey;

extern boolean promptactive;

extern consvar_t cv_pauseifunfocused;

// used in game menu
extern consvar_t cv_tutorialprompt;
extern consvar_t cv_chatwidth, cv_chatnotifications, cv_chatheight, cv_chattime, cv_consolechat, cv_chatbacktint, cv_chatspamprotection, cv_compactscoreboard;
extern consvar_t cv_crosshair, cv_crosshair2;
extern consvar_t cv_invertmouse, cv_alwaysfreelook, cv_chasefreelook, cv_mousemove;
extern consvar_t cv_invertmouse2, cv_alwaysfreelook2, cv_chasefreelook2, cv_mousemove2;

extern consvar_t cv_useranalog[2], cv_analog[2];
extern consvar_t cv_directionchar[2];

typedef enum {
	CS_LEGACY,
	CS_LMAOGALOG,
	CS_STANDARD,
	CS_SIMPLE = CS_LMAOGALOG|CS_STANDARD,
} controlstyle_e;
#define G_ControlStyle(ssplayer) (cv_directionchar[(ssplayer)-1].value == 3 ? CS_LMAOGALOG : ((cv_analog[(ssplayer)-1].value ? CS_LMAOGALOG : 0) | (cv_directionchar[(ssplayer)-1].value ? CS_STANDARD : 0)))
#define P_ControlStyle(player) ((((player)->pflags & PF_ANALOGMODE) ? CS_LMAOGALOG : 0) | (((player)->pflags & PF_DIRECTIONCHAR) ? CS_STANDARD : 0))

extern consvar_t cv_autobrake, cv_autobrake2;
extern consvar_t cv_sideaxis,cv_turnaxis,cv_moveaxis,cv_lookaxis,cv_jumpaxis,cv_spinaxis,cv_fireaxis,cv_firenaxis,cv_deadzone,cv_digitaldeadzone;
extern consvar_t cv_sideaxis2,cv_turnaxis2,cv_moveaxis2,cv_lookaxis2,cv_jumpaxis2,cv_spinaxis2,cv_fireaxis2,cv_firenaxis2,cv_deadzone2,cv_digitaldeadzone2;
extern consvar_t cv_ghost_bestscore, cv_ghost_besttime, cv_ghost_bestrings, cv_ghost_last, cv_ghost_guest;

// hi here's some new controls
extern consvar_t cv_cam_shiftfacing[2], cv_cam_turnfacing[2],
	cv_cam_turnfacingability[2], cv_cam_turnfacingspindash[2], cv_cam_turnfacinginput[2],
	cv_cam_centertoggle[2], cv_cam_lockedinput[2], cv_cam_lockonboss[2];

typedef enum
{
	LOCK_BOSS = 1<<0,
	LOCK_ENEMY = 1<<1,
	LOCK_INTERESTS = 1<<2,
} lockassist_e;


// mouseaiming (looking up/down with the mouse or keyboard)
#define KB_LOOKSPEED (1<<25)
#define MAXPLMOVE (50)
#define SLOWTURNTICS (6)

// build an internal map name MAPxx from map number
const char *G_BuildMapName(INT32 map);

extern INT16 ticcmd_oldangleturn[2];
extern boolean ticcmd_centerviewdown[2]; // For simple controls, lock the camera behind the player
extern mobj_t *ticcmd_ztargetfocus[2]; // Locking onto an object?
void G_BuildTiccmd(ticcmd_t *cmd, INT32 realtics, UINT8 ssplayer);

// copy ticcmd_t to and fro the normal way
ticcmd_t *G_CopyTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n);
// copy ticcmd_t to and fro network packets
ticcmd_t *G_MoveTiccmd(ticcmd_t* dest, const ticcmd_t* src, const size_t n);

// clip the console player aiming to the view
INT16 G_ClipAimingPitch(INT32 *aiming);
INT16 G_SoftwareClipAimingPitch(INT32 *aiming);

extern angle_t localangle, localangle2;
extern INT32 localaiming, localaiming2; // should be an angle_t but signed

//
// GAME
//
void G_ChangePlayerReferences(mobj_t *oldmo, mobj_t *newmo);
void G_DoReborn(INT32 playernum);
void G_PlayerReborn(INT32 player, boolean betweenmaps);
void G_InitNew(UINT8 pultmode, const char *mapname, boolean resetplayer,
	boolean skipprecutscene, boolean FLS);
char *G_BuildMapTitle(INT32 mapnum);

struct searchdim
{
	UINT8 pos;
	UINT8 siz;
};

typedef struct
{
	INT16  mapnum;
	UINT8  matchc;
	struct searchdim *matchd;/* offset that a pattern was matched */
	UINT8  keywhc;
	struct searchdim *keywhd;/* ...in KEYWORD */
	UINT8  total;/* total hits */
}
mapsearchfreq_t;

INT32 G_FindMap(const char *query, char **foundmapnamep,
		mapsearchfreq_t **freqp, INT32 *freqc);
void G_FreeMapSearch(mapsearchfreq_t *freq, INT32 freqc);

/* Match map name by search + 2 digit map code or map number. */
INT32 G_FindMapByNameOrCode(const char *query, char **foundmapnamep);

// XMOD spawning
mapthing_t *G_FindCTFStart(INT32 playernum);
mapthing_t *G_FindMatchStart(INT32 playernum);
mapthing_t *G_FindCoopStart(INT32 playernum);
mapthing_t *G_FindMapStart(INT32 playernum);
void G_MovePlayerToSpawnOrStarpost(INT32 playernum);
void G_SpawnPlayer(INT32 playernum);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1, but a warp test can start elsewhere
void G_DeferedInitNew(boolean pultmode, const char *mapname, INT32 pickedchar,
	boolean SSSG, boolean FLS);
void G_DoLoadLevel(boolean resetplayer);
void G_StartTitleCard(void);
void G_PreLevelTitleCard(void);
boolean G_IsTitleCardAvailable(void);

// Can be called by the startup code or M_Responder, calls P_SetupLevel.
void G_LoadGame(UINT32 slot, INT16 mapoverride);

void G_SaveGameData(void);

void G_SaveGame(UINT32 slot);

void G_SaveGameOver(UINT32 slot, boolean modifylives);

extern UINT32 gametypedefaultrules[NUMGAMETYPES];
extern UINT32 gametypetol[NUMGAMETYPES];
extern INT16 gametyperankings[NUMGAMETYPES];

void G_SetGametype(INT16 gametype);
INT16 G_AddGametype(UINT32 rules);
void G_AddGametypeConstant(INT16 gtype, const char *newgtconst);
void G_UpdateGametypeSelections(void);
void G_AddTOL(UINT32 newtol, const char *tolname);
void G_AddGametypeTOL(INT16 gtype, UINT32 newtol);
void G_SetGametypeDescription(INT16 gtype, char *descriptiontext, UINT8 leftcolor, UINT8 rightcolor);

INT32 G_GetGametypeByName(const char *gametypestr);
boolean G_IsSpecialStage(INT32 mapnum);
boolean G_GametypeUsesLives(void);
boolean G_GametypeUsesCoopLives(void);
boolean G_GametypeUsesCoopStarposts(void);
boolean G_GametypeHasTeams(void);
boolean G_GametypeHasSpectators(void);
boolean G_RingSlingerGametype(void);
boolean G_PlatformGametype(void);
boolean G_CoopGametype(void);
boolean G_TagGametype(void);
boolean G_CompetitionGametype(void);
boolean G_EnoughPlayersFinished(void);
void G_ExitLevel(void);
void G_NextLevel(void);
void G_Continue(void);
void G_UseContinue(void);
void G_AfterIntermission(void);
void G_EndGame(void); // moved from y_inter.c/h and renamed

void G_Ticker(boolean run);
boolean G_Responder(event_t *ev);

void G_AddPlayer(INT32 playernum);

void G_SetExitGameFlag(void);
void G_ClearExitGameFlag(void);
boolean G_GetExitGameFlag(void);

void G_SetRetryFlag(void);
void G_ClearRetryFlag(void);
boolean G_GetRetryFlag(void);

void G_SetModeAttackRetryFlag(void);
void G_ClearModeAttackRetryFlag(void);
boolean G_GetModeAttackRetryFlag(void);

void G_LoadGameData(void);
void G_LoadGameSettings(void);

void G_SetGameModified(boolean silent);

void G_SetGamestate(gamestate_t newstate);

// Gamedata record shit
void G_AllocMainRecordData(INT16 i);
void G_AllocNightsRecordData(INT16 i);
void G_ClearRecords(void);

UINT32 G_GetBestScore(INT16 map);
tic_t G_GetBestTime(INT16 map);
UINT16 G_GetBestRings(INT16 map);
UINT32 G_GetBestNightsScore(INT16 map, UINT8 mare);
tic_t G_GetBestNightsTime(INT16 map, UINT8 mare);
UINT8 G_GetBestNightsGrade(INT16 map, UINT8 mare);

void G_AddTempNightsRecords(UINT32 pscore, tic_t ptime, UINT8 mare);
void G_SetNightsRecords(void);

FUNCMATH INT32 G_TicsToHours(tic_t tics);
FUNCMATH INT32 G_TicsToMinutes(tic_t tics, boolean full);
FUNCMATH INT32 G_TicsToSeconds(tic_t tics);
FUNCMATH INT32 G_TicsToCentiseconds(tic_t tics);
FUNCMATH INT32 G_TicsToMilliseconds(tic_t tics);

// Don't split up TOL handling
UINT32 G_TOLFlag(INT32 pgametype);

#endif
