// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by Matthew "Kaito Sinclaire" Walsh.
// Copyright (C) 2012-2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cond.h
/// \brief Unlockable condition system for SRB2 version 2.1

#include "doomdef.h"

// --------
// Typedefs
// --------

// DEHackEd structure for each is listed after the condition type
// [required] <optional>
typedef enum
{
	UC_PLAYTIME,        // PLAYTIME [tics]
	UC_GAMECLEAR,       // GAMECLEAR <x times>
	UC_ALLEMERALDS,     // ALLEMERALDS <x times>
	UC_ULTIMATECLEAR,   // ULTIMATECLEAR <x times>
	UC_OVERALLSCORE,    // OVERALLSCORE [score to beat]
	UC_OVERALLTIME,     // OVERALLTIME [time to beat, tics]
	UC_OVERALLRINGS,    // OVERALLRINGS [rings to beat]
	UC_MAPVISITED,      // MAPVISITED [map number]
	UC_MAPBEATEN,       // MAPBEATEN [map number]
	UC_MAPALLEMERALDS,  // MAPALLEMERALDS [map number]
	UC_MAPULTIMATE,     // MAPULTIMATE [map number]
	UC_MAPPERFECT,      // MAPPERFECT [map number]
	UC_MAPSCORE,        // MAPSCORE [map number] [score to beat]
	UC_MAPTIME,         // MAPTIME [map number] [time to beat, tics]
	UC_MAPRINGS,        // MAPRINGS [map number] [rings to beat]
	UC_NIGHTSSCORE,     // NIGHTSSCORE [map number] <mare, omit or "0" for overall> [score to beat]
	UC_NIGHTSTIME,      // NIGHTSTIME [map number] <mare, omit "0" overall> [time to beat, tics]
	UC_NIGHTSGRADE,     // NIGHTSGRADE [map number] <mare, omit "0" overall> [grade]
	UC_TRIGGER,         // TRIGGER [trigger number]
	UC_TOTALEMBLEMS,    // TOTALEMBLEMS [number of emblems]
	UC_EMBLEM,          // EMBLEM [emblem number]
	UC_EXTRAEMBLEM,     // EXTRAEMBLEM [extra emblem number]
	UC_CONDITIONSET,    // CONDITIONSET [condition set number]
} conditiontype_t;

// Condition Set information
typedef struct
{
	UINT32 id;           /// <- The ID of this condition.
	                     ///    In an unlock condition, all conditions with the same ID
	                     ///    must be true to fulfill the unlockable requirements.
	                     ///    Only one ID set needs to be true, however.
	conditiontype_t type;/// <- The type of condition
	INT32 requirement;   /// <- The requirement for this variable.
	INT16 extrainfo1;    /// <- Extra information for the condition when needed.
	INT16 extrainfo2;    /// <- Extra information for the condition when needed.
} condition_t;
typedef struct
{
	UINT32 numconditions;   /// <- number of conditions.
	condition_t *condition; /// <- All conditionals to be checked.
	UINT8 achieved;         /// <- Whether this conditional has been achieved already or not.
	                        ///    (Conditional checking is skipped if true -- it's assumed you can't relock an unlockable)
} conditionset_t;

// Emblem information
#define ET_GLOBAL 0 // Emblem with a position in space
#define ET_SKIN   1 // Skin specific emblem with a position in space, var == skin
#define ET_MAP    2 // Beat the map
#define ET_SCORE  3 // Get the score
#define ET_TIME   4 // Get the time
#define ET_RINGS  5 // Get the rings
#define ET_NGRADE 6 // Get the grade
#define ET_NTIME  7 // Get the time (NiGHTS mode)

// Global emblem flags
#define GE_NIGHTSPULL 1 // sun off the nights track - loop it
#define GE_NIGHTSITEM 2 // moon on the nights track - find it

// Map emblem flags
#define ME_ALLEMERALDS 1
#define ME_ULTIMATE    2
#define ME_PERFECT     4

typedef struct
{
	UINT8 type;      ///< Emblem type
	INT16 tag;       ///< Tag of emblem mapthing
	INT16 level;     ///< Level on which this emblem can be found.
	UINT8 sprite;    ///< emblem sprite to use, 0 - 25
	UINT16 color;    ///< skincolor to use
	INT32 var;       ///< If needed, specifies information on the target amount to achieve (or target skin)
	char hint[110];  ///< Hint for emblem hints menu
	UINT8 collected; ///< Do you have this emblem?
} emblem_t;
typedef struct
{
	char name[20];          ///< Name of the goal (used in the "emblem awarded" cecho)
	char description[40];   ///< Description of goal (used in statistics)
	UINT8 conditionset;     ///< Condition set that awards this emblem.
	UINT8 showconditionset; ///< Condition set that shows this emblem.
	UINT8 sprite;           ///< emblem sprite to use, 0 - 25
	UINT16 color;           ///< skincolor to use
	UINT8 collected;        ///< Do you have this emblem?
} extraemblem_t;

// Unlockable information
typedef struct
{
	char name[64];
	char objective[64];
	UINT16 height; // menu height
	UINT8 conditionset;
	UINT8 showconditionset;
	INT16 type;
	INT16 variable;
	UINT8 nocecho;
	UINT8 nochecklist;
	UINT8 unlocked;
} unlockable_t;

#define SECRET_NONE         -6 // Does nil.  Use with levels locked by UnlockRequired
#define SECRET_ITEMFINDER	-5 // Enables Item Finder/Emblem Radar
#define SECRET_EMBLEMHINTS	-4 // Enables Emblem Hints
#define SECRET_PANDORA		-3 // Enables Pandora's Box
#define SECRET_RECORDATTACK	-2 // Enables Record Attack on the main menu
#define SECRET_NIGHTSMODE	-1 // Enables NiGHTS Mode on the main menu
#define SECRET_HEADER		 0 // Does nothing on its own, just serves as a header for the menu
#define SECRET_LEVELSELECT	 1 // Selectable level select
#define SECRET_WARP			 2 // Selectable warp
#define SECRET_SOUNDTEST	 3 // Sound Test
#define SECRET_CREDITS		 4 // Enables Credits

// If you have more secrets than these variables allow in your game,
// you seriously need to get a life.
#define MAXCONDITIONSETS 128
#define MAXEMBLEMS       512
#define MAXEXTRAEMBLEMS   16
#define MAXUNLOCKABLES    32

extern conditionset_t conditionSets[MAXCONDITIONSETS];
extern emblem_t emblemlocations[MAXEMBLEMS];
extern extraemblem_t extraemblems[MAXEXTRAEMBLEMS];
extern unlockable_t unlockables[MAXUNLOCKABLES];

extern INT32 numemblems;
extern INT32 numextraemblems;

extern UINT32 unlocktriggers;

// Condition set setup
void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2);

// Clearing secrets
void M_ClearConditionSet(UINT8 set);
void M_ClearSecrets(void);

// Updating conditions and unlockables
void M_CheckUnlockConditions(void);
UINT8 M_UpdateUnlockablesAndExtraEmblems(void);
void M_SilentUpdateUnlockablesAndEmblems(void);
UINT8 M_CheckLevelEmblems(void);
UINT8 M_CompletionEmblems(void);

// Checking unlockable status
UINT8 M_AnySecretUnlocked(void);
UINT8 M_SecretUnlocked(INT32 type);
UINT8 M_MapLocked(INT32 mapnum);
INT32 M_CountEmblems(void);

// Emblem shit
emblem_t *M_GetLevelEmblems(INT32 mapnum);
skincolornum_t M_GetEmblemColor(emblem_t *em);
const char *M_GetEmblemPatch(emblem_t *em, boolean big);
skincolornum_t M_GetExtraEmblemColor(extraemblem_t *em);
const char *M_GetExtraEmblemPatch(extraemblem_t *em, boolean big);

// If you're looking to compare stats for unlocks or what not, use these
// They stop checking upon reaching the target number so they
// should be (theoretically?) slightly faster.
UINT8 M_GotEnoughEmblems(INT32 number);
UINT8 M_GotHighEnoughScore(INT32 tscore);
UINT8 M_GotLowEnoughTime(INT32 tictime);
UINT8 M_GotHighEnoughRings(INT32 trings);

#define M_Achieved(a) ((a) >= MAXCONDITIONSETS || conditionSets[a].achieved)
