// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  g_demo.h
/// \brief Demo recording and playback

#ifndef __G_DEMO__
#define __G_DEMO__

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"

// ======================================
// DEMO playback/recording related stuff.
// ======================================

// demoplaying back and demo recording
extern boolean demoplayback, titledemo, demorecording, timingdemo;
extern tic_t demostarttime;
extern UINT16 demoversion;

typedef enum
{
	DFILE_OVERRIDE_NONE = 0, // Show errors normally
	DFILE_OVERRIDE_LOAD, // Forcefully load demo, add files beforehand
	DFILE_OVERRIDE_SKIP, // Forcefully load demo, skip file list
} demo_file_override_e;

extern demo_file_override_e demofileoverride;
extern consvar_t cv_resyncdemo;

// Quit after playing a demo from cmdline.
extern boolean singledemo;
extern boolean demo_start;
extern boolean demo_forwardmove_rng;
extern boolean demosynced;

extern mobj_t *metalplayback;

// Only called by startup code.
void G_RecordDemo(const char *name);
void G_RecordMetal(void);
void G_BeginRecording(void);
void G_BeginMetal(void);

// Only called by shutdown code.
void G_SetDemoTime(UINT32 ptime, UINT32 pscore, UINT16 prings);
UINT8 G_CmpDemoTime(char *oldname, char *newname);

typedef enum
{
	GHC_NORMAL = 0,
	GHC_SUPER,
	GHC_FIREFLOWER,
	GHC_INVINCIBLE,
	GHC_NIGHTSSKIN, // not actually a colour
	GHC_RETURNSKIN // ditto
} ghostcolor_t;

// G_CheckDemoExtraFiles: checks if our loaded WAD list matches the demo's.
typedef enum
{
	DFILE_ERROR_NONE = 0, // No file error
	DFILE_ERROR_NOTLOADED, // Files are not loaded, but can be without a restart.
	DFILE_ERROR_OUTOFORDER, // Files are loaded, but out of order.
	DFILE_ERROR_INCOMPLETEOUTOFORDER, // Some files are loaded out of order, but others are not.
	DFILE_ERROR_CANNOTLOAD, // Files are missing and cannot be loaded.
	DFILE_ERROR_EXTRAFILES, // Extra files outside of the replay's file list are loaded.
	DFILE_ERROR_NOTDEMO = UINT8_MAX, // This replay isn't even a replay...
} demo_file_error_e;

// Record/playback tics
void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum);
void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum);
void G_GhostAddThok(void);
void G_GhostAddSpin(void);
void G_GhostAddRev(void);
void G_GhostAddColor(ghostcolor_t color);
void G_GhostAddFlip(void);
void G_GhostAddScale(fixed_t scale);
void G_GhostAddHit(mobj_t *victim);
void G_WriteGhostTic(mobj_t *ghost);
void G_ConsGhostTic(void);
void G_GhostTicker(void);
void G_ReadMetalTic(mobj_t *metal);
void G_WriteMetalTic(mobj_t *metal);
void G_SaveMetal(UINT8 **buffer);
void G_LoadMetal(UINT8 **buffer);

void G_DeferedPlayDemo(const char *demo);
void G_DoPlayDemo(char *defdemoname);
void G_TimeDemo(const char *name);
void G_AddGhost(char *defdemoname);
void G_FreeGhosts(void);
void G_DoPlayMetal(void);
void G_DoneLevelLoad(void);
void G_StopMetalDemo(void);
ATTRNORETURN void FUNCNORETURN G_StopMetalRecording(boolean kill);
void G_StopDemo(void);
boolean G_CheckDemoStatus(void);
INT32 G_ConvertOldFrameFlags(INT32 frame);
UINT8 G_CheckDemoForError(char *defdemoname);

#endif // __G_DEMO__
