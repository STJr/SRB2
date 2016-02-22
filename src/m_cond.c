// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2014 by Matthew "Inuyasha" Walsh.
// Copyright (C) 2012-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  m_cond.c
/// \brief Unlockable condition system for SRB2 version 2.1

#include "m_cond.h"
#include "doomstat.h"
#include "z_zone.h"

#include "hu_stuff.h" // CEcho
#include "v_video.h" // video flags

#include "g_game.h" // record info
#include "r_things.h" // numskins
#include "r_draw.h" // R_GetColorByName

// Map triggers for linedef executors
// 32 triggers, one bit each
UINT32 unlocktriggers;

// The meat of this system lies in condition sets
conditionset_t conditionSets[MAXCONDITIONSETS];

// Default Emblem locations
emblem_t emblemlocations[MAXEMBLEMS] =
{
	// CHECKER MOUNTAIN 1
	{ET_GLOBAL,  -1377,  -3168,  1152, 2, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   -826,   3731,  1880, 2, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   -802,   5132,  4999, 2, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   3584,   4332,    97, 2, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   6883,   6372,  -256, 2, 'U', SKINCOLOR_NONE, 0, "", 0},

	{ET_RINGS, 0,0,0,  2, 'U', SKINCOLOR_NONE,          200, "", 0},

	// MAINFRAME METROPOLIS 1
	{ET_GLOBAL,  -1285,   2430,   512, 5, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   -780,   3238,  1024, 5, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   3937,  11969,  1312, 5, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   4306,  22139,  1344, 5, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  12288,  26630,  1792, 5, 'U', SKINCOLOR_NONE, 0, "", 0},

	{ET_RINGS, 0,0,0,  5, 'U', SKINCOLOR_NONE,          200, "", 0},

	// FROZEN FACTORY 1
	{ET_GLOBAL,   1496,   -979,   664, 8, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  -7041,    229,   604, 8, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   4712,   1194,  1885, 8, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   7196,  -1371,   394, 8, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   4368,   4027,   204, 8, 'U', SKINCOLOR_NONE, 0, "", 0},

	{ET_RINGS, 0,0,0,  8, 'U', SKINCOLOR_NONE,          200, "", 0},

	// WEATHER FACTORY 1
	{ET_GLOBAL,   2967,   9370,   538, 11, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   6481,  -9606,   534, 11, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   8387,  -6214,  1536, 11, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  14081, -14851,  1093, 11, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  14913,  -5994,  1832, 11, 'U', SKINCOLOR_NONE, 0, "", 0},

	{ET_RINGS, 0,0,0,  11, 'U', SKINCOLOR_NONE,          200, "", 0},

	// STORMY STREETS
	{ET_GLOBAL,   2871,  -4263,   576, 23, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  12907,  -1908,   178, 23, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  11774, -11384,   163, 23, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  18921,  -9762,  1326, 23, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,   2871,  -4263,  1326, 23, 'U', SKINCOLOR_NONE, 0, "", 0},

	{ET_RINGS, 0,0,0,  23, 'U', SKINCOLOR_NONE,          200, "", 0},

	// SKY ISLANDS
	{ET_GLOBAL,   1031,   1409,   417, 25, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,    576,   5354,   304, 25, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,    799,  13381,  -168, 25, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL,  -4931,   2423,   973, 25, 'U', SKINCOLOR_NONE, 0, "", 0},
	{ET_GLOBAL, -12292,  -1412,   827, 25, 'U', SKINCOLOR_NONE, 0, "", 0},

	{ET_RINGS, 0,0,0,  25, 'U', SKINCOLOR_NONE,          200, "", 0},
};

// Default Extra Emblems
extraemblem_t extraemblems[MAXEXTRAEMBLEMS] =
{
	{"Game Complete",  "Complete 1P Mode",                    10, 'X', SKINCOLOR_BLUE, 0},
	{"Perfect Bonus",  "Perfect Bonus on a non-secret stage", 30, 'P', SKINCOLOR_GOLD, 0},
};

// Default Unlockables
unlockable_t unlockables[MAXUNLOCKABLES] =
{
	// Name, Objective, Menu Height, ConditionSet, Unlock Type, Variable, NoCecho, NoChecklist
	/* 01 */ {"Record Attack",     "Complete Checker Mountain Zone, Act 1", 0, 1, SECRET_RECORDATTACK,  0,  true,  true, 0},

	/* 02 */ {"Play Credits",      "Complete 1P Mode", 30, 10, SECRET_CREDITS,   0,  true,  true, 0},
	/* 03 */ {"Sound Test",        "Complete 1P Mode", 40, 10, SECRET_SOUNDTEST, 0, false, false, 0},

	/* 04 */ {"EXTRA LEVELS", "", 60, 0, SECRET_HEADER, 0, true, true, 0},

	/* 05 */ {"Stormy Streets",     "Collect 5 Chaos Coins",    70, 40, SECRET_WARP,         23, false, false, 0},
	/* 06 */ {"Sky Islands",        "Collect 20 Chaos Coins",    80, 47, SECRET_WARP,         25, false, false, 0},

	/* 07 */ {"BONUS LEVELS", "", 100, 0, SECRET_HEADER, 0, true, true, 0},

	/* 08 */ {"Bonus Level Select", "Collect 7 Chaos Coins",	110, 41, SECRET_LEVELSELECT, 3, false, true, 0},
	/* 09 */ {"Grove",             "Collect 7 Chaos Coins",   0, 41, SECRET_NONE,         52, false, true, 0},
	/* 10 */ {"Old HUB 1",         "Collect 9 Chaos Coins",   0, 42, SECRET_NONE,         50, false, true, 0},
	/* 11 */ {"Ancient Waterway",  "Collect 10 Chaos Coins",  0, 43, SECRET_NONE,         54, false, true, 0},
	/* 12 */ {"Grumpy Canyon",     "Collect 12 Chaos Coins",  0, 44, SECRET_NONE,         56, false, true, 0},
	/* 13 */ {"Lava Mountain",     "Collect 16 Chaos Coins",  0, 45, SECRET_NONE,         57, false, true, 0},
	/* 14 */ {"untitled",          "Collect 18 Chaos Coins",  0, 46, SECRET_NONE,         55, false, true, 0},
	/* 15 */ {"Old HUB 2",         "Collect 25 Chaos Coins",  0, 48, SECRET_NONE,         51, false, true, 0},
	/* 16 */ {"Tako Desert",       "Collect 26 Chaos Coins",  0, 49, SECRET_NONE,         53, false, true, 0},
	/* 17 */ {"A Big Mistake",     "Collect 28 Chaos Coins",  0, 50, SECRET_NONE,         58, false, true, 0},

	/* 18 */ {"Pandora's Box", "Collect All Emblems",  0, 51, SECRET_PANDORA,     0, false, false, 0},
	/* 19 */ {"Level Select",  "Collect All Emblems", 20, 51, SECRET_LEVELSELECT, 1, false,  true, 0},
	/* 20 */ {"Scrapped Level Set 1", "Collect 7-16 Chaos Coins", 0, 41, SECRET_NONE, 0, true, false, 0},
	/* 21 */ {"Scrapped Level Set 2", "Collect 18-28 Chaos Coins", 0, 46, SECRET_NONE, 0, true, false, 0},
};

// Default number of emblems and extra emblems
INT32 numemblems = 36;
INT32 numextraemblems = 2;

// DEFAULT CONDITION SETS FOR SRB2 2.1:
void M_SetupDefaultConditionSets(void)
{
	memset(conditionSets, 0, sizeof(conditionSets));

	// --   1: Complete CMZ1
	M_AddRawCondition(1, 1, UC_MAPBEATEN, 2, 0, 0);

	// --  10: Complete the game
	M_AddRawCondition(10, 1, UC_GAMECLEAR, 1, 0, 0);

	// --  11: Complete the game with all emeralds
	M_AddRawCondition(11, 1, UC_ALLEMERALDS, 1, 0, 0);

	// --  30: Perfect Bonus
	M_AddRawCondition(30, 1,  UC_MAPPERFECT,  2, 0, 0);
	M_AddRawCondition(30, 2,  UC_MAPPERFECT,  5, 0, 0);
	M_AddRawCondition(30, 3,  UC_MAPPERFECT,  8, 0, 0);
	M_AddRawCondition(30, 4,  UC_MAPPERFECT, 11, 0, 0);
	M_AddRawCondition(30, 5,  UC_MAPPERFECT, 23, 0, 0);
	M_AddRawCondition(30, 6,  UC_MAPPERFECT, 25, 0, 0);
	
	// --  40: Find 5 emblems
	M_AddRawCondition(40, 1, UC_TOTALEMBLEMS,  5, 0, 0);

	// --  41: Find 7 emblems
	M_AddRawCondition(41, 1, UC_TOTALEMBLEMS,  7, 0, 0);

	// --  42: Find 9 emblems
	M_AddRawCondition(42, 1, UC_TOTALEMBLEMS,  9, 0, 0);

	// --  43: Find 10 emblems
	M_AddRawCondition(43, 1, UC_TOTALEMBLEMS, 10, 0, 0);

	// --  44: Find 12 emblems
	M_AddRawCondition(44, 1, UC_TOTALEMBLEMS, 12, 0, 0);

	// --  45: Find 16 emblems
	M_AddRawCondition(45, 1, UC_TOTALEMBLEMS, 16, 0, 0);

	// --  46: Find 18 emblems
	M_AddRawCondition(46, 1, UC_TOTALEMBLEMS, 18, 0, 0);

	// --  47: Find 20 emblems
	M_AddRawCondition(47, 1, UC_TOTALEMBLEMS, 20, 0, 0);

	// --  48: Find 25 emblems
	M_AddRawCondition(48, 1, UC_TOTALEMBLEMS, 25, 0, 0);

	// --  49: Find 26 emblems
	M_AddRawCondition(49, 1, UC_TOTALEMBLEMS, 26, 0, 0);

	// --  50: Find 28 emblems
	M_AddRawCondition(50, 1, UC_TOTALEMBLEMS, 28, 0, 0);

	// --  51: Find 38 (all) emblems
	M_AddRawCondition(51, 1, UC_TOTALEMBLEMS, 38, 0, 0);
}

void M_AddRawCondition(UINT8 set, UINT8 id, conditiontype_t c, INT32 r, INT16 x1, INT16 x2)
{
	condition_t *cond;
	UINT32 num, wnum;

	I_Assert(set && set <= MAXCONDITIONSETS);

	wnum = conditionSets[set - 1].numconditions;
	num = ++conditionSets[set - 1].numconditions;

	conditionSets[set - 1].condition = Z_Realloc(conditionSets[set - 1].condition, sizeof(condition_t)*num, PU_STATIC, 0);

	cond = conditionSets[set - 1].condition;

	cond[wnum].id = id;
	cond[wnum].type = c;
	cond[wnum].requirement = r;
	cond[wnum].extrainfo1 = x1;
	cond[wnum].extrainfo2 = x2;
}

void M_ClearConditionSet(UINT8 set)
{
	if (conditionSets[set - 1].numconditions)
	{
		Z_Free(conditionSets[set - 1].condition);
		conditionSets[set - 1].condition = NULL;
		conditionSets[set - 1].numconditions = 0;
	}
	conditionSets[set - 1].achieved = false;
}

// Clear ALL secrets.
void M_ClearSecrets(void)
{
	INT32 i;

	memset(mapvisited, 0, sizeof(mapvisited));

	for (i = 0; i < MAXEMBLEMS; ++i)
		emblemlocations[i].collected = false;
	for (i = 0; i < MAXEXTRAEMBLEMS; ++i)
		extraemblems[i].collected = false;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
		unlockables[i].unlocked = false;
	for (i = 0; i < MAXCONDITIONSETS; ++i)
		conditionSets[i].achieved = false;

	timesBeaten = timesBeatenWithEmeralds = timesBeatenUltimate = 0;

	// Re-unlock any always unlocked things
	M_SilentUpdateUnlockablesAndEmblems();
}

// ----------------------
// Condition set checking
// ----------------------
static UINT8 M_CheckCondition(condition_t *cn)
{
	switch (cn->type)
	{
		case UC_PLAYTIME: // Requires total playing time >= x
			return (totalplaytime >= (unsigned)cn->requirement);
		case UC_GAMECLEAR: // Requires game beaten >= x times
			return (timesBeaten >= (unsigned)cn->requirement);
		case UC_ALLEMERALDS: // Requires game beaten with all 7 emeralds >= x times
			return (timesBeatenWithEmeralds >= (unsigned)cn->requirement);
		case UC_ULTIMATECLEAR: // Requires game beaten on ultimate >= x times (in other words, never)
			return (timesBeatenUltimate >= (unsigned)cn->requirement);
		case UC_OVERALLSCORE: // Requires overall score >= x
			return (M_GotHighEnoughScore(cn->requirement));
		case UC_OVERALLTIME: // Requires overall time <= x
			return (M_GotLowEnoughTime(cn->requirement));
		case UC_OVERALLRINGS: // Requires overall rings >= x
			return (M_GotHighEnoughRings(cn->requirement));
		case UC_MAPVISITED: // Requires map x to be visited
			return ((mapvisited[cn->requirement - 1] & MV_VISITED) == MV_VISITED);
		case UC_MAPBEATEN: // Requires map x to be beaten
			return ((mapvisited[cn->requirement - 1] & MV_BEATEN) == MV_BEATEN);
		case UC_MAPALLEMERALDS: // Requires map x to be beaten with all emeralds in possession
			return ((mapvisited[cn->requirement - 1] & MV_ALLEMERALDS) == MV_ALLEMERALDS);
		case UC_MAPULTIMATE: // Requires map x to be beaten on ultimate
			return ((mapvisited[cn->requirement - 1] & MV_ULTIMATE) == MV_ULTIMATE);
		case UC_MAPPERFECT: // Requires map x to be beaten with a perfect bonus
			return ((mapvisited[cn->requirement - 1] & MV_PERFECT) == MV_PERFECT);
		case UC_MAPSCORE: // Requires score on map >= x
			return (G_GetBestScore(cn->extrainfo1) >= (unsigned)cn->requirement);
		case UC_MAPTIME: // Requires time on map <= x
			return (G_GetBestTime(cn->extrainfo1) <= (unsigned)cn->requirement);
		case UC_MAPRINGS: // Requires rings on map >= x
			return (G_GetBestRings(cn->extrainfo1) >= cn->requirement);
		case UC_NIGHTSSCORE:
			return (G_GetBestNightsScore(cn->extrainfo1, (UINT8)cn->extrainfo2) >= (unsigned)cn->requirement);
		case UC_NIGHTSTIME:
			return (G_GetBestNightsTime(cn->extrainfo1, (UINT8)cn->extrainfo2) <= (unsigned)cn->requirement);
		case UC_NIGHTSGRADE:
			return (G_GetBestNightsGrade(cn->extrainfo1, (UINT8)cn->extrainfo2) >= cn->requirement);
		case UC_TRIGGER: // requires map trigger set
			return !!(unlocktriggers & (1 << cn->requirement));
		case UC_TOTALEMBLEMS: // Requires number of emblems >= x
			return (M_GotEnoughEmblems(cn->requirement));
		case UC_EMBLEM: // Requires emblem x to be obtained
			return emblemlocations[cn->requirement-1].collected;
		case UC_EXTRAEMBLEM: // Requires extra emblem x to be obtained
			return extraemblems[cn->requirement-1].collected;
		case UC_CONDITIONSET: // requires condition set x to already be achieved
			return M_Achieved(cn->requirement-1);
	}
	return false;
}

static UINT8 M_CheckConditionSet(conditionset_t *c)
{
	UINT32 i;
	UINT32 lastID = 0;
	condition_t *cn;
	UINT8 achievedSoFar = true;

	for (i = 0; i < c->numconditions; ++i)
	{
		cn = &c->condition[i];

		// If the ID is changed and all previous statements of the same ID were true
		// then this condition has been successfully achieved
		if (lastID && lastID != cn->id && achievedSoFar)
			return true;

		// Skip future conditions with the same ID if one fails, for obvious reasons
		else if (lastID && lastID == cn->id && !achievedSoFar)
			continue;

		lastID = cn->id;
		achievedSoFar = M_CheckCondition(cn);
	}

	return achievedSoFar;
}

void M_CheckUnlockConditions(void)
{
	INT32 i;
	conditionset_t *c;

	for (i = 0; i < MAXCONDITIONSETS; ++i)
	{
		c = &conditionSets[i];
		if (!c->numconditions || c->achieved)
			continue;

		c->achieved = (M_CheckConditionSet(c));
	}
}

UINT8 M_UpdateUnlockablesAndExtraEmblems(void)
{
	INT32 i;
	char cechoText[992] = "";
	UINT8 cechoLines = 0;

	if (modifiedgame && !savemoddata)
		return false;

	M_CheckUnlockConditions();

	// Go through extra emblems
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected || !extraemblems[i].conditionset)
			continue;
		if ((extraemblems[i].collected = M_Achieved(extraemblems[i].conditionset - 1)) != false)
		{
			strcat(cechoText, va(M_GetText("Got \"%s\" emblem!\\"), extraemblems[i].name));
			++cechoLines;
		}
	}

	// Fun part: if any of those unlocked we need to go through the
	// unlock conditions AGAIN just in case an emblem reward was reached
	if (cechoLines)
		M_CheckUnlockConditions();

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].unlocked || !unlockables[i].conditionset)
			continue;
		if ((unlockables[i].unlocked = M_Achieved(unlockables[i].conditionset - 1)) != false)
		{
			if (unlockables[i].nocecho)
				continue;
			strcat(cechoText, va(M_GetText("\"%s\" unlocked!\\"), unlockables[i].name));
			++cechoLines;
		}
	}

	// Announce
	if (cechoLines)
	{
		char slashed[1024] = "";
		for (i = 0; (i < 21) && (i < 24 - cechoLines); ++i)
			slashed[i] = '\\';
		slashed[i] = 0;

		strcat(slashed, cechoText);

		HU_SetCEchoFlags(V_YELLOWMAP|V_RETURN8);
		HU_SetCEchoDuration(6);
		HU_DoCEcho(slashed);
		return true;
	}
	return false;
}

// Used when loading gamedata to make sure all unlocks are synched with conditions
void M_SilentUpdateUnlockablesAndEmblems(void)
{
	INT32 i;
	boolean checkAgain = false;

	// Just in case they aren't to sync
	M_CheckUnlockConditions();
	M_CheckLevelEmblems();

	// Go through extra emblems
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected || !extraemblems[i].conditionset)
			continue;
		if ((extraemblems[i].collected = M_Achieved(extraemblems[i].conditionset - 1)) != false)
			checkAgain = true;
	}

	// check again if extra emblems unlocked, blah blah, etc
	if (checkAgain)
		M_CheckUnlockConditions();

	// Go through unlockables
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].unlocked || !unlockables[i].conditionset)
			continue;
		unlockables[i].unlocked = M_Achieved(unlockables[i].conditionset - 1);
	}
}

// Emblem unlocking shit
UINT8 M_CheckLevelEmblems(void)
{
	INT32 i;
	INT32 valToReach;
	INT16 levelnum;
	UINT8 res;
	UINT8 somethingUnlocked = 0;

	// Update Score, Time, Rings emblems
	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].type <= ET_SKIN || emblemlocations[i].collected)
			continue;

		levelnum = emblemlocations[i].level;
		valToReach = emblemlocations[i].var;

		switch (emblemlocations[i].type)
		{
			case ET_SCORE: // Requires score on map >= x
				res = (G_GetBestScore(levelnum) >= (unsigned)valToReach);
				break;
			case ET_TIME: // Requires time on map <= x
				res = (G_GetBestTime(levelnum) <= (unsigned)valToReach);
				break;
			case ET_RINGS: // Requires rings on map >= x
				res = (G_GetBestRings(levelnum) >= valToReach);
				break;
			case ET_NGRADE: // Requires NiGHTS grade on map >= x
				res = (G_GetBestNightsGrade(levelnum, 0) >= valToReach);
				break;
			case ET_NTIME: // Requires NiGHTS time on map <= x
				res = (G_GetBestNightsTime(levelnum, 0) <= (unsigned)valToReach);
				break;
			default: // unreachable but shuts the compiler up.
				continue;
		}

		emblemlocations[i].collected = res;
		if (res)
			++somethingUnlocked;
	}
	return somethingUnlocked;
}

// -------------------
// Quick unlock checks
// -------------------
UINT8 M_AnySecretUnlocked(void)
{
	INT32 i;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (!unlockables[i].nocecho && unlockables[i].unlocked)
			return true;
	}
	return false;
}

UINT8 M_SecretUnlocked(INT32 type)
{
	INT32 i;
	for (i = 0; i < MAXUNLOCKABLES; ++i)
	{
		if (unlockables[i].type == type && unlockables[i].unlocked)
			return true;
	}
	return false;
}

UINT8 M_MapLocked(INT32 mapnum)
{
	if (!mapheaderinfo[mapnum-1] || mapheaderinfo[mapnum-1]->unlockrequired < 0)
		return false;
	if (!unlockables[mapheaderinfo[mapnum-1]->unlockrequired].unlocked)
		return true;
	return false;
}

INT32 M_CountEmblems(void)
{
	INT32 found = 0, i;
	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].collected)
			found++;
	}
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected)
			found++;
	}
	return found;
}

// --------------------------------------
// Quick functions for calculating things
// --------------------------------------

// Theoretically faster than using M_CountEmblems()
// Stops when it reaches the target number of emblems.
UINT8 M_GotEnoughEmblems(INT32 number)
{
	INT32 i, gottenemblems = 0;
	for (i = 0; i < numemblems; ++i)
	{
		if (emblemlocations[i].collected)
			if (++gottenemblems >= number) return true;
	}
	for (i = 0; i < numextraemblems; ++i)
	{
		if (extraemblems[i].collected)
			if (++gottenemblems >= number) return true;
	}
	return false;
}

UINT8 M_GotHighEnoughScore(INT32 tscore)
{
	INT32 mscore = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; ++i)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;
		if (!mainrecords[i])
			continue;

		if ((mscore += mainrecords[i]->score) > tscore)
			return true;
	}
	return false;
}

UINT8 M_GotLowEnoughTime(INT32 tictime)
{
	INT32 curtics = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; ++i)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;

		if (!mainrecords[i] || !mainrecords[i]->time)
			return false;
		else if ((curtics += mainrecords[i]->time) > tictime)
			return false;
	}
	return true;
}

UINT8 M_GotHighEnoughRings(INT32 trings)
{
	INT32 mrings = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; ++i)
	{
		if (!mapheaderinfo[i] || !(mapheaderinfo[i]->menuflags & LF2_RECORDATTACK))
			continue;
		if (!mainrecords[i])
			continue;

		if ((mrings += mainrecords[i]->rings) > trings)
			return true;
	}
	return false;
}

// ----------------
// Misc Emblem shit
// ----------------

// Returns pointer to an emblem if an emblem exists for that level.
// Pass -1 mapnum to continue from last emblem.
// NULL if not found.
// note that this goes in reverse!!
emblem_t *M_GetLevelEmblems(INT32 mapnum)
{
	static INT32 map = -1;
	static INT32 i = -1;

	if (mapnum > 0)
	{
		map = mapnum;
		i = numemblems;
	}

	while (--i >= 0)
	{
		if (emblemlocations[i].level == map)
			return &emblemlocations[i];
	}
	return NULL;
}

skincolors_t M_GetEmblemColor(emblem_t *em)
{
	if (!em || em->color >= MAXSKINCOLORS)
		return SKINCOLOR_NONE;
	return em->color;
}

const char *M_GetEmblemPatch(emblem_t *em)
{
	static char pnamebuf[7] = "GOTITn";

	I_Assert(em->sprite >= 'A' && em->sprite <= 'Z');
	pnamebuf[5] = em->sprite;
	return pnamebuf;
}

skincolors_t M_GetExtraEmblemColor(extraemblem_t *em)
{
	if (!em || em->color >= MAXSKINCOLORS)
		return SKINCOLOR_NONE;
	return em->color;
}

const char *M_GetExtraEmblemPatch(extraemblem_t *em)
{
	static char pnamebuf[7] = "GOTITn";

	I_Assert(em->sprite >= 'A' && em->sprite <= 'Z');
	pnamebuf[5] = em->sprite;
	return pnamebuf;
}
