// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  deh_tables.h
/// \brief Define DeHackEd tables.

#ifndef __DEH_TABLES_H__
#define __DEH_TABLES_H__

#include "doomdef.h" // Constants
#include "d_think.h" // actionf_t
#include "info.h" // Mobj, state, sprite, etc constants
#include "lua_script.h"

// Free slot names
// The crazy word-reading stuff uses these.
extern char *FREE_STATES[NUMSTATEFREESLOTS];
extern char *FREE_MOBJS[NUMMOBJFREESLOTS];
extern char *FREE_SKINCOLORS[NUMCOLORFREESLOTS];
extern bitarray_t used_spr[BIT_ARRAY_SIZE(NUMSPRITEFREESLOTS)]; // Sprite freeslots in use

#define initfreeslots() {\
	memset(FREE_STATES, 0, sizeof(FREE_STATES));\
	memset(FREE_MOBJS, 0, sizeof(FREE_MOBJS));\
	memset(FREE_SKINCOLORS, 0, sizeof(FREE_SKINCOLORS));\
	memset(used_spr, 0, sizeof(used_spr));\
	memset(actionsoverridden, LUA_REFNIL, sizeof(actionsoverridden));\
}

struct flickytypes_s {
	const char *name;
	const mobjtype_t type;
};

#define MAXFLICKIES 64

/** Action pointer for reading actions from Dehacked lumps.
  */
typedef struct
{
	actionf_t action; ///< Function pointer corresponding to the actual action.
	const char *name; ///< Name of the action in ALL CAPS.
} actionpointer_t;

struct int_const_s {
	const char *n;
	// has to be able to hold both fixed_t and angle_t, so drastic measure!!
	lua_Integer v;
};

extern const char NIGHTSGRADE_LIST[];
extern struct flickytypes_s FLICKYTYPES[];
extern actionpointer_t actionpointers[]; // Array mapping action names to action functions.
extern const char *const STATE_LIST[];
extern const char *const MOBJTYPE_LIST[];
extern const char *const MOBJFLAG_LIST[];
extern const char *const MOBJFLAG2_LIST[]; // \tMF2_(\S+).*// (.+) --> \t"\1", // \2
extern const char *const MOBJEFLAG_LIST[];
extern const char *const MAPTHINGFLAG_LIST[];
extern const char *const PLAYERFLAG_LIST[];
extern const char *const GAMETYPERULE_LIST[];
extern const char *const ML_LIST[]; // Linedef flags
extern const char *const MSF_LIST[]; // Sector flags
extern const char *const SSF_LIST[]; // Sector special flags
extern const char *const SD_LIST[]; // Sector damagetype
extern const char *const TO_LIST[]; // Sector triggerer
extern const char *COLOR_ENUMS[];
extern const char *const POWERS_LIST[];
extern const char *const HUDITEMS_LIST[];
extern const char *const MENUTYPES_LIST[];

extern struct int_const_s const INT_CONST[];

// Moved to this file because it can't work compile-time otherwise
void DEH_TableCheck(void);

#endif
