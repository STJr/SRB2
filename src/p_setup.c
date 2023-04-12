// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_setup.c
/// \brief Do all the WAD I/O, get map description, set up initial state and misc. LUTs

#include "doomdef.h"
#include "d_main.h"
#include "byteptr.h"
#include "g_game.h"

#include "p_local.h"
#include "p_setup.h"
#include "p_spec.h"
#include "p_saveg.h"

#include "i_time.h"
#include "i_sound.h" // for I_PlayCD()..
#include "i_video.h" // for I_FinishUpdate()..
#include "r_sky.h"
#include "i_system.h"

#include "r_data.h"
#include "r_things.h" // for R_AddSpriteDefs
#include "r_textures.h"
#include "r_patch.h"
#include "r_picformats.h"
#include "r_sky.h"
#include "r_draw.h"
#include "r_fps.h" // R_ResetViewInterpolation in level load

#include "s_sound.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_splats.h"

#include "hu_stuff.h"
#include "console.h"

#include "m_misc.h"
#include "m_fixed.h"
#include "m_random.h"

#include "dehacked.h" // for map headers
#include "r_main.h"
#include "m_cond.h" // for emblems

#include "m_argv.h"

#include "p_polyobj.h"

#include "v_video.h"

#include "filesrch.h" // refreshdirmenu

#include "lua_hud.h" // level title

#include "f_finale.h" // wipes

#include "md5.h" // map MD5

// for MapLoad hook
#include "lua_script.h"
#include "lua_hook.h"

#ifdef _WIN32
#include <malloc.h>
#include <math.h>
#endif
#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#include "hardware/hw_model.h"
#endif

#include "p_slopes.h"

#include "fastcmp.h" // textmap parsing

#include "taglist.h"

//
// Map MD5, calculated on level load.
// Sent to clients in PT_SERVERINFO.
//
unsigned char mapmd5[16];

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

boolean udmf;
size_t numvertexes, numsegs, numsectors, numsubsectors, numnodes, numlines, numsides, nummapthings;
vertex_t *vertexes;
seg_t *segs;
sector_t *sectors;
subsector_t *subsectors;
node_t *nodes;
line_t *lines;
side_t *sides;
mapthing_t *mapthings;
sector_t *spawnsectors;
line_t *spawnlines;
side_t *spawnsides;
INT32 numstarposts;
UINT16 bossdisabled;
boolean stoppedclock;
boolean levelloading;
UINT8 levelfadecol;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
INT32 bmapwidth, bmapheight; // size in mapblocks

INT32 *blockmap; // INT32 for large maps
// offsets in blockmap are from here
INT32 *blockmaplump; // Big blockmap

// origin of block map
fixed_t bmaporgx, bmaporgy;
// for thing chains
mobj_t **blocklinks;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed LineOf Sight calculation.
// Without special effect, this could be used as a PVS lookup as well.
//
UINT8 *rejectmatrix;

// Maintain single and multi player starting spots.
INT32 numdmstarts, numcoopstarts, numredctfstarts, numbluectfstarts;

mapthing_t *deathmatchstarts[MAX_DM_STARTS];
mapthing_t *playerstarts[MAXPLAYERS];
mapthing_t *bluectfstarts[MAXPLAYERS];
mapthing_t *redctfstarts[MAXPLAYERS];

// Maintain waypoints
mobj_t *waypoints[NUMWAYPOINTSEQUENCES][WAYPOINTSEQUENCESIZE];
UINT16 numwaypoints[NUMWAYPOINTSEQUENCES];

void P_AddWaypoint(UINT8 sequence, UINT8 id, mobj_t *waypoint)
{
	waypoints[sequence][id] = waypoint;
	if (id >= numwaypoints[sequence])
		numwaypoints[sequence] = id + 1;
}

static void P_ResetWaypoints(void)
{
	UINT16 sequence, id;
	for (sequence = 0; sequence < NUMWAYPOINTSEQUENCES; sequence++)
	{
		for (id = 0; id < numwaypoints[sequence]; id++)
			waypoints[sequence][id] = NULL;

		numwaypoints[sequence] = 0;
	}
}

mobj_t *P_GetFirstWaypoint(UINT8 sequence)
{
	return waypoints[sequence][0];
}

mobj_t *P_GetLastWaypoint(UINT8 sequence)
{
	return waypoints[sequence][numwaypoints[sequence] - 1];
}

mobj_t *P_GetPreviousWaypoint(mobj_t *current, boolean wrap)
{
	UINT8 sequence = current->threshold;
	UINT8 id = current->health;

	if (id == 0)
	{
		if (!wrap)
			return NULL;

		id = numwaypoints[sequence] - 1;
	}
	else
		id--;

	return waypoints[sequence][id];
}

mobj_t *P_GetNextWaypoint(mobj_t *current, boolean wrap)
{
	UINT8 sequence = current->threshold;
	UINT8 id = current->health;

	if (id == numwaypoints[sequence] - 1)
	{
		if (!wrap)
			return NULL;

		id = 0;
	}
	else
		id++;

	return waypoints[sequence][id];
}

mobj_t *P_GetClosestWaypoint(UINT8 sequence, mobj_t *mo)
{
	UINT8 wp;
	mobj_t *mo2, *result = NULL;
	fixed_t bestdist = 0;
	fixed_t curdist;

	for (wp = 0; wp < numwaypoints[sequence]; wp++)
	{
		mo2 = waypoints[sequence][wp];

		if (!mo2)
			continue;

		curdist = P_AproxDistance(P_AproxDistance(mo->x - mo2->x, mo->y - mo2->y), mo->z - mo2->z);

		if (result && curdist > bestdist)
			continue;

		result = mo2;
		bestdist = curdist;
	}

	return result;
}

// Return true if all waypoints are in the same location
boolean P_IsDegeneratedWaypointSequence(UINT8 sequence)
{
	mobj_t *first, *waypoint;
	UINT8 wp;

	if (numwaypoints[sequence] <= 1)
		return true;

	first = waypoints[sequence][0];

	for (wp = 1; wp < numwaypoints[sequence]; wp++)
	{
		waypoint = waypoints[sequence][wp];

		if (!waypoint)
			continue;

		if (waypoint->x != first->x)
			return false;

		if (waypoint->y != first->y)
			return false;

		if (waypoint->z != first->z)
			return false;
	}

	return true;
}


/** Logs an error about a map being corrupt, then terminate.
  * This allows reporting highly technical errors for usefulness, without
  * confusing a novice map designer who simply needs to run ZenNode.
  *
  * If logging is disabled in this compile, or the log file is not opened, the
  * full technical details are printed in the I_Error() message.
  *
  * \param msg The message to log. This message can safely result from a call
  *            to va(), since that function is not used here.
  * \todo Fix the I_Error() message. On some implementations the logfile may
  *       not be called log.txt.
  * \sa CON_LogMessage, I_Error
  */
FUNCNORETURN static ATTRNORETURN void CorruptMapError(const char *msg)
{
	// don't use va() because the calling function probably uses it
	char mapnum[10];

	sprintf(mapnum, "%hd", gamemap);
	CON_LogMessage("Map ");
	CON_LogMessage(mapnum);
	CON_LogMessage(" is corrupt: ");
	CON_LogMessage(msg);
	CON_LogMessage("\n");
	I_Error("Invalid or corrupt map.\nLook in log file or text console for technical details.");
}

/** Sets a header's flickies to be equivalent to the original Freed Animals
  *
  * \param i The header to set flickies for
  */
void P_SetDemoFlickies(INT16 i)
{
	mapheaderinfo[i]->numFlickies = 5;
	mapheaderinfo[i]->flickies = Z_Realloc(mapheaderinfo[i]->flickies, 5*sizeof(mobjtype_t), PU_STATIC, NULL);
	mapheaderinfo[i]->flickies[0] = MT_FLICKY_02/*MT_BUNNY*/;
	mapheaderinfo[i]->flickies[1] = MT_FLICKY_01/*MT_BIRD*/;
	mapheaderinfo[i]->flickies[2] = MT_FLICKY_12/*MT_MOUSE*/;
	mapheaderinfo[i]->flickies[3] = MT_FLICKY_11/*MT_COW*/;
	mapheaderinfo[i]->flickies[4] = MT_FLICKY_03/*MT_CHICKEN*/;
}

/** Clears a header's flickies
  *
  * \param i The header to clear flickies for
  */
void P_DeleteFlickies(INT16 i)
{
	if (mapheaderinfo[i]->flickies)
		Z_Free(mapheaderinfo[i]->flickies);
	mapheaderinfo[i]->flickies = NULL;
	mapheaderinfo[i]->numFlickies = 0;
}

#define NUMLAPS_DEFAULT 4

/** Clears the data from a single map header.
  *
  * \param i Map number to clear header for.
  * \sa P_ClearMapHeaderInfo
  */
static void P_ClearSingleMapHeaderInfo(INT16 i)
{
	const INT16 num = (INT16)(i-1);
	mapheaderinfo[num]->lvlttl[0] = '\0';
	mapheaderinfo[num]->selectheading[0] = '\0';
	mapheaderinfo[num]->subttl[0] = '\0';
	mapheaderinfo[num]->ltzzpatch[0] = '\0';
	mapheaderinfo[num]->ltzztext[0] = '\0';
	mapheaderinfo[num]->ltactdiamond[0] = '\0';
	mapheaderinfo[num]->actnum = 0;
	mapheaderinfo[num]->typeoflevel = 0;
	mapheaderinfo[num]->nextlevel = (INT16)(i + 1);
	mapheaderinfo[num]->marathonnext = 0;
	mapheaderinfo[num]->startrings = 0;
	mapheaderinfo[num]->sstimer = 90;
	mapheaderinfo[num]->ssspheres = 1;
	mapheaderinfo[num]->gravity = FRACUNIT/2;
	mapheaderinfo[num]->keywords[0] = '\0';
	snprintf(mapheaderinfo[num]->musname, 7, "%sM", G_BuildMapName(i));
	mapheaderinfo[num]->musname[6] = 0;
	mapheaderinfo[num]->mustrack = 0;
	mapheaderinfo[num]->muspos = 0;
	mapheaderinfo[num]->musinterfadeout = 0;
	mapheaderinfo[num]->musintername[0] = 0;
	mapheaderinfo[num]->muspostbossname[0] = 0;
	mapheaderinfo[num]->muspostbosstrack = 0;
	mapheaderinfo[num]->muspostbosspos = 0;
	mapheaderinfo[num]->muspostbossfadein = 0;
	mapheaderinfo[num]->musforcereset = -1;
	mapheaderinfo[num]->forcecharacter[0] = '\0';
	mapheaderinfo[num]->weather = 0;
	mapheaderinfo[num]->skynum = 1;
	mapheaderinfo[num]->skybox_scalex = 16;
	mapheaderinfo[num]->skybox_scaley = 16;
	mapheaderinfo[num]->skybox_scalez = 16;
	mapheaderinfo[num]->interscreen[0] = '#';
	mapheaderinfo[num]->runsoc[0] = '#';
	mapheaderinfo[num]->scriptname[0] = '#';
	mapheaderinfo[num]->precutscenenum = 0;
	mapheaderinfo[num]->cutscenenum = 0;
	mapheaderinfo[num]->countdown = 0;
	mapheaderinfo[num]->palette = UINT16_MAX;
	mapheaderinfo[num]->numlaps = NUMLAPS_DEFAULT;
	mapheaderinfo[num]->unlockrequired = -1;
	mapheaderinfo[num]->levelselect = 0;
	mapheaderinfo[num]->bonustype = 0;
	mapheaderinfo[num]->maxbonuslives = -1;
	mapheaderinfo[num]->levelflags = 0;
	mapheaderinfo[num]->menuflags = 0;
#if 1 // equivalent to "FlickyList = DEMO"
	P_SetDemoFlickies(num);
#else // equivalent to "FlickyList = NONE"
	P_DeleteFlickies(num);
#endif
	P_DeleteGrades(num);
	mapheaderinfo[num]->customopts = NULL;
	mapheaderinfo[num]->numCustomOptions = 0;
}

/** Allocates a new map-header structure.
  *
  * \param i Index of header to allocate.
  */
void P_AllocMapHeader(INT16 i)
{
	if (!mapheaderinfo[i])
	{
		mapheaderinfo[i] = Z_Malloc(sizeof(mapheader_t), PU_STATIC, NULL);
		mapheaderinfo[i]->flickies = NULL;
		mapheaderinfo[i]->grades = NULL;
	}
	P_ClearSingleMapHeaderInfo(i + 1);
}

/** NiGHTS Grades are a special structure,
  * we initialize them here.
  *
  * \param i Index of header to allocate grades for
  * \param mare The mare we're adding grades for
  * \param grades the string from DeHackEd, we work with it ourselves
  */
void P_AddGradesForMare(INT16 i, UINT8 mare, char *gtext)
{
	INT32 g;
	char *spos = gtext;

	CONS_Debug(DBG_SETUP, "Map %d Mare %d: ", i+1, (UINT16)mare+1);

	if (mapheaderinfo[i]->numGradedMares < mare+1)
	{
		mapheaderinfo[i]->numGradedMares = mare+1;
		mapheaderinfo[i]->grades = Z_Realloc(mapheaderinfo[i]->grades, sizeof(nightsgrades_t) * mapheaderinfo[i]->numGradedMares, PU_STATIC, NULL);
	}

	for (g = 0; g < 6; ++g)
	{
		// Allow "partial" grading systems
		if (spos != NULL)
		{
			mapheaderinfo[i]->grades[mare].grade[g] = atoi(spos);
			CONS_Debug(DBG_SETUP, "%u ", atoi(spos));
			// Grab next comma
			spos = strchr(spos, ',');
			if (spos)
				++spos;
		}
		else
		{
			// Grade not reachable
			mapheaderinfo[i]->grades[mare].grade[g] = UINT32_MAX;
		}
	}

	CONS_Debug(DBG_SETUP, "\n");
}

/** And this removes the grades safely.
  *
  * \param i The header to remove grades from
  */
void P_DeleteGrades(INT16 i)
{
	if (mapheaderinfo[i]->grades)
		Z_Free(mapheaderinfo[i]->grades);

	mapheaderinfo[i]->grades = NULL;
	mapheaderinfo[i]->numGradedMares = 0;
}

/** And this fetches the grades
  *
  * \param pscore The player's score.
  * \param map The game map.
  * \param mare The mare to test.
  */
UINT8 P_GetGrade(UINT32 pscore, INT16 map, UINT8 mare)
{
	INT32 i;

	// Determining the grade
	if (mapheaderinfo[map-1] && mapheaderinfo[map-1]->grades && mapheaderinfo[map-1]->numGradedMares >= mare + 1)
	{
		INT32 pgrade = 0;
		for (i = 0; i < 6; ++i)
		{
			if (pscore >= mapheaderinfo[map-1]->grades[mare].grade[i])
				++pgrade;
		}
		return (UINT8)pgrade;
	}
	return 0;
}

UINT8 P_HasGrades(INT16 map, UINT8 mare)
{
	// Determining the grade
	// Mare 0 is treated as overall and is true if ANY grades exist
	if (mapheaderinfo[map-1] && mapheaderinfo[map-1]->grades
		&& (mare == 0 || mapheaderinfo[map-1]->numGradedMares >= mare))
		return true;
	return false;
}

UINT32 P_GetScoreForGrade(INT16 map, UINT8 mare, UINT8 grade)
{
	// Get the score for the grade... if it exists
	if (grade == GRADE_F || grade > GRADE_S || !P_HasGrades(map, mare)) return 0;

	return mapheaderinfo[map-1]->grades[mare].grade[grade-1];
}

UINT32 P_GetScoreForGradeOverall(INT16 map, UINT8 grade)
{
	UINT8 mares;
	INT32 i;
	UINT32 score = 0;
	mares = mapheaderinfo[map-1]->numGradedMares;
	for (i = 0; i < mares; ++i)
			score += P_GetScoreForGrade(map, i, grade);
	return score;
}

//
// levelflats
//
#define MAXLEVELFLATS 256

size_t numlevelflats;
levelflat_t *levelflats;
levelflat_t *foundflats;

//SoM: Other files want this info.
size_t P_PrecacheLevelFlats(void)
{
	lumpnum_t lump;
	size_t i;

	//SoM: 4/18/2000: New flat code to make use of levelflats.
	flatmemory = 0;
	for (i = 0; i < numlevelflats; i++)
	{
		if (levelflats[i].type == LEVELFLAT_FLAT)
		{
			lump = levelflats[i].u.flat.lumpnum;
			if (devparm)
				flatmemory += W_LumpLength(lump);
			R_GetFlat(lump);
		}
	}
	return flatmemory;
}

/*
levelflat refers to an array of level flats,
or NULL if we want to allocate it now.
*/
static INT32
Ploadflat (levelflat_t *levelflat, const char *flatname, boolean resize)
{
#ifndef NO_PNG_LUMPS
	UINT8         buffer[8];
#endif

	lumpnum_t    flatnum;
	int       texturenum;
	UINT8     *flatpatch;
	size_t    lumplength;

	size_t i;

	// Scan through the already found flats, return if it matches.
	for (i = 0; i < numlevelflats; i++)
	{
		if (strnicmp(levelflat[i].name, flatname, 8) == 0)
			return i;
	}

	if (resize)
	{
		// allocate new flat memory
		levelflats = Z_Realloc(levelflats, (numlevelflats + 1) * sizeof(*levelflats), PU_LEVEL, NULL);
		levelflat  = levelflats + numlevelflats;
	}
	else
	{
		if (numlevelflats >= MAXLEVELFLATS)
			I_Error("Too many flats in level\n");

		levelflat += numlevelflats;
	}

	// Store the name.
	strlcpy(levelflat->name, flatname, sizeof (levelflat->name));
	strupr(levelflat->name);

	/* If we can't find a flat, try looking for a texture! */
	if (( flatnum = R_GetFlatNumForName(levelflat->name) ) == LUMPERROR)
	{
		if (( texturenum = R_CheckTextureNumForName(levelflat->name) ) == -1)
		{
			// check for REDWALL
			if (( texturenum = R_CheckTextureNumForName("REDWALL") ) != -1)
				goto texturefound;
			// check for REDFLR
			else if (( flatnum = R_GetFlatNumForName("REDFLR") ) != LUMPERROR)
				goto flatfound;
			// nevermind
			levelflat->type = LEVELFLAT_NONE;
		}
		else
		{
texturefound:
			levelflat->type = LEVELFLAT_TEXTURE;
			levelflat->u.texture.    num = texturenum;
			levelflat->u.texture.lastnum = texturenum;
			/* start out unanimated */
			levelflat->u.texture.basenum = -1;
		}
	}
	else
	{
flatfound:
		/* This could be a flat, patch, or PNG. */
		flatpatch = W_CacheLumpNum(flatnum, PU_CACHE);
		lumplength = W_LumpLength(flatnum);
		if (Picture_CheckIfDoomPatch((softwarepatch_t *)flatpatch, lumplength))
			levelflat->type = LEVELFLAT_PATCH;
		else
		{
#ifndef NO_PNG_LUMPS
			/*
			Only need eight bytes for PNG headers.
			FIXME: Put this elsewhere.
			*/
			W_ReadLumpHeader(flatnum, buffer, 8, 0);
			if (Picture_IsLumpPNG(buffer, lumplength))
				levelflat->type = LEVELFLAT_PNG;
			else
#endif/*NO_PNG_LUMPS*/
				levelflat->type = LEVELFLAT_FLAT;/* phew */
		}
		if (flatpatch)
			Z_Free(flatpatch);

		levelflat->u.flat.    lumpnum = flatnum;
		levelflat->u.flat.baselumpnum = LUMPERROR;
	}

#ifndef ZDEBUG
	CONS_Debug(DBG_SETUP, "flat #%03d: %s\n", atoi(sizeu1(numlevelflats)), levelflat->name);
#endif

	return ( numlevelflats++ );
}

// Auxiliary function. Find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
INT32 P_AddLevelFlat(const char *flatname, levelflat_t *levelflat)
{
	return Ploadflat(levelflat, flatname, false);
}

// help function for Lua and $$$.sav reading
// same as P_AddLevelFlat, except this is not setup so we must realloc levelflats to fit in the new flat
// no longer a static func in lua_maplib.c because p_saveg.c also needs it
//
INT32 P_AddLevelFlatRuntime(const char *flatname)
{
	return Ploadflat(levelflats, flatname, true);
}

// help function for $$$.sav checking
// this simply returns the flat # for the name given
//
INT32 P_CheckLevelFlat(const char *flatname)
{
	size_t i;
	levelflat_t *levelflat = levelflats;

	//
	//  scan through the already found flats
	//
	for (i = 0; i < numlevelflats; i++, levelflat++)
		if (strnicmp(levelflat->name,flatname,8)==0)
			break;

	if (i == numlevelflats)
		return 0; // ??? flat was not found, this should not happen!

	// level flat id
	return (INT32)i;
}

//
// P_ReloadRings
// Used by NiGHTS, clears all ring/sphere/hoop/etc items and respawns them
//
void P_ReloadRings(void)
{
	mobj_t *mo;
	thinker_t *th;
	size_t i, numHoops = 0;
	// Okay, if you have more than 4000 hoops in your map,
	// you're insane.
	mapthing_t *hoopsToRespawn[4096];
	mapthing_t *mt = mapthings;

	// scan the thinkers to find rings/spheres/hoops to unset
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;

		if (mo->type == MT_HOOPCENTER)
		{
			// Hoops give me a headache
			if (mo->threshold == 4242) // Dead hoop
			{
				hoopsToRespawn[numHoops++] = mo->spawnpoint;
				P_RemoveMobj(mo);
			}
			continue;
		}
		if (!(mo->type == MT_RING || mo->type == MT_COIN
			|| mo->type == MT_BLUESPHERE || mo->type == MT_BOMBSPHERE
			|| mo->type == MT_NIGHTSCHIP || mo->type == MT_NIGHTSSTAR))
			continue;

		// Don't auto-disintegrate things being pulled to us
		if (mo->flags2 & MF2_NIGHTSPULL)
			continue;

		P_RemoveMobj(mo);
	}

	// Reiterate through mapthings
	for (i = 0; i < nummapthings; i++, mt++)
	{
		// Notice an omission? We handle hoops differently.
		if (mt->type == mobjinfo[MT_RING].doomednum || mt->type == mobjinfo[MT_COIN].doomednum
			|| mt->type == mobjinfo[MT_REDTEAMRING].doomednum || mt->type == mobjinfo[MT_BLUETEAMRING].doomednum
			|| mt->type == mobjinfo[MT_BLUESPHERE].doomednum || mt->type == mobjinfo[MT_BOMBSPHERE].doomednum)
		{
			mt->mobj = NULL;
			P_SetBonusTime(P_SpawnMapThing(mt));
		}
		else if (mt->type >= 600 && mt->type <= 611) // Item patterns
		{
			mt->mobj = NULL;
			P_SpawnItemPattern(mt, true);
		}
	}
	for (i = 0; i < numHoops; i++)
	{
		P_SpawnHoop(hoopsToRespawn[i]);
	}
}

void P_SwitchSpheresBonusMode(boolean bonustime)
{
	mobj_t *mo;
	thinker_t *th;

	// scan the thinkers to find spheres to switch
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;

		if (mo->type != MT_BLUESPHERE && mo->type != MT_NIGHTSCHIP
			&& mo->type != MT_FLINGBLUESPHERE && mo->type != MT_FLINGNIGHTSCHIP)
			continue;

		if (!mo->health)
			continue;

		P_SetMobjState(mo, ((bonustime) ? mo->info->raisestate : mo->info->spawnstate));
	}
}

#ifdef SCANTHINGS
void P_ScanThings(INT16 mapnum, INT16 wadnum, INT16 lumpnum)
{
	size_t i, n;
	UINT8 *data, *datastart;
	UINT16 type, maprings;
	INT16 tol;
	UINT32 flags;

	tol = mapheaderinfo[mapnum-1]->typeoflevel;
	if (!(tol & TOL_SP))
		return;
	flags = mapheaderinfo[mapnum-1]->levelflags;

	n = W_LumpLengthPwad(wadnum, lumpnum) / (5 * sizeof (INT16));
	//CONS_Printf("%u map things found!\n", n);

	maprings = 0;
	data = datastart = W_CacheLumpNumPwad(wadnum, lumpnum, PU_STATIC);
	for (i = 0; i < n; i++)
	{
		data += 3 * sizeof (INT16); // skip x y position, angle
		type = READUINT16(data) & 4095;
		data += sizeof (INT16); // skip options

		switch (type)
		{
		case 300: // MT_RING
		case 1800: // MT_COIN
		case 308: // red team ring
		case 309: // blue team ring
			maprings++;
			break;
		case 400: // MT_SUPERRINGBOX
		case 414: // red ring box
		case 415: // blue ring box
		case 603: // 10 diagonal rings
			maprings += 10;
			break;
		case 600: // 5 vertical rings
		case 601: // 5 vertical rings
		case 602: // 5 diagonal rings
			maprings += 5;
			break;
		case 604: // 8 circle rings
		case 609: // 16 circle rings & wings
			maprings += 8;
			break;
		case 605: // 16 circle rings
			maprings += 16;
			break;
		case 608: // 8 circle rings & wings
			maprings += 4;
			break;
		}
	}
	Z_Free(datastart);

	if (maprings)
		CONS_Printf("%s has %u rings\n", G_BuildMapName(mapnum), maprings);
}
#endif

static void P_SpawnEmeraldHunt(void)
{
	INT32 emer[3], num[MAXHUNTEMERALDS], i, randomkey;
	fixed_t x, y, z;

	for (i = 0; i < numhuntemeralds; i++)
		num[i] = i;

	for (i = 0; i < 3; i++)
	{
		// generate random index, shuffle afterwards
		randomkey = P_RandomKey(numhuntemeralds--);
		emer[i] = num[randomkey];
		num[randomkey] = num[numhuntemeralds];
		num[numhuntemeralds] = emer[i];

		// spawn emerald
		x = huntemeralds[emer[i]]->x<<FRACBITS;
		y = huntemeralds[emer[i]]->y<<FRACBITS;
		z = P_GetMapThingSpawnHeight(MT_EMERHUNT, huntemeralds[emer[i]], x, y);
		P_SetMobjStateNF(P_SpawnMobj(x, y, z, MT_EMERHUNT),
			mobjinfo[MT_EMERHUNT].spawnstate+i);
	}
}

static void P_SpawnMapThings(boolean spawnemblems)
{
	size_t i;
	mapthing_t *mt;

        // Spawn axis points first so they are at the front of the list for fast searching.
	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		switch (mt->type)
		{
			case 1700: // MT_AXIS
			case 1701: // MT_AXISTRANSFER
			case 1702: // MT_AXISTRANSFERLINE
				mt->mobj = NULL;
				P_SpawnMapThing(mt);
				break;
			default:
				break;
		}
	}

	numhuntemeralds = 0;

	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		if (mt->type == 1700 // MT_AXIS
			|| mt->type == 1701 // MT_AXISTRANSFER
			|| mt->type == 1702) // MT_AXISTRANSFERLINE
			continue; // These were already spawned

		if (!spawnemblems && mt->type == mobjinfo[MT_EMBLEM].doomednum)
			continue;

		mt->mobj = NULL;

		if (mt->type >= 600 && mt->type <= 611) // item patterns
			P_SpawnItemPattern(mt, false);
		else if (mt->type == 1713) // hoops
			P_SpawnHoop(mt);
		else // Everything else
			P_SpawnMapThing(mt);
	}

	// random emeralds for hunt
	if (numhuntemeralds)
		P_SpawnEmeraldHunt();
}

// Experimental groovy write function!
/*void P_WriteThings(void)
{
	size_t i, length;
	mapthing_t *mt;
	UINT8 *savebuffer, *savebuf_p;
	INT16 temp;

	savebuf_p = savebuffer = (UINT8 *)malloc(nummapthings * sizeof (mapthing_t));

	if (!savebuf_p)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No more free memory for thing writing!\n"));
		return;
	}

	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		WRITEINT16(savebuf_p, mt->x);
		WRITEINT16(savebuf_p, mt->y);

		WRITEINT16(savebuf_p, mt->angle);

		temp = (INT16)(mt->type + ((INT16)mt->extrainfo << 12));
		WRITEINT16(savebuf_p, temp);
		WRITEUINT16(savebuf_p, mt->options);
	}

	length = savebuf_p - savebuffer;

	FIL_WriteFile(va("newthings%d.lmp", gamemap), savebuffer, length);
	free(savebuffer);
	savebuf_p = NULL;

	CONS_Printf(M_GetText("newthings%d.lmp saved.\n"), gamemap);
}*/

//
// MAP LOADING FUNCTIONS
//

static void P_LoadVertices(UINT8 *data)
{
	mapvertex_t *mv = (mapvertex_t *)data;
	vertex_t *v = vertexes;
	size_t i;

	// Copy and convert vertex coordinates, internal representation as fixed.
	for (i = 0; i < numvertexes; i++, v++, mv++)
	{
		v->x = SHORT(mv->x)<<FRACBITS;
		v->y = SHORT(mv->y)<<FRACBITS;
		v->floorzset = v->ceilingzset = false;
		v->floorz = v->ceilingz = 0;
	}
}

static void P_InitializeSector(sector_t *ss)
{
	memset(&ss->soundorg, 0, sizeof(ss->soundorg));

	ss->validcount = 0;

	ss->thinglist = NULL;

	ss->floordata = NULL;
	ss->ceilingdata = NULL;
	ss->lightingdata = NULL;
	ss->fadecolormapdata = NULL;

	ss->heightsec = -1;
	ss->camsec = -1;

	ss->floorlightsec = ss->ceilinglightsec = -1;
	ss->crumblestate = CRUMBLE_NONE;

	ss->touching_thinglist = NULL;

	ss->linecount = 0;
	ss->lines = NULL;

	ss->ffloors = NULL;
	ss->attached = NULL;
	ss->attachedsolid = NULL;
	ss->numattached = 0;
	ss->maxattached = 1;
	ss->lightlist = NULL;
	ss->numlights = 0;
	ss->moved = true;

	ss->extra_colormap = NULL;

	ss->gravityptr = NULL;

	ss->cullheight = NULL;

	ss->floorspeed = ss->ceilspeed = 0;

	ss->preciplist = NULL;
	ss->touching_preciplist = NULL;

	ss->f_slope = NULL;
	ss->c_slope = NULL;
	ss->hasslope = false;

	ss->spawn_lightlevel = ss->lightlevel;

	ss->spawn_extra_colormap = NULL;
}

static void P_LoadSectors(UINT8 *data)
{
	mapsector_t *ms = (mapsector_t *)data;
	sector_t *ss = sectors;
	size_t i;

	// For each counted sector, copy the sector raw data from our cache pointer ms, to the global table pointer ss.
	for (i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

		ss->floorpic = P_AddLevelFlat(ms->floorpic, foundflats);
		ss->ceilingpic = P_AddLevelFlat(ms->ceilingpic, foundflats);

		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		Tag_FSet(&ss->tags, SHORT(ms->tag));

		ss->floor_xoffs = ss->floor_yoffs = 0;
		ss->ceiling_xoffs = ss->ceiling_yoffs = 0;

		ss->floorpic_angle = ss->ceilingpic_angle = 0;

		ss->floorlightlevel = ss->ceilinglightlevel = 0;
		ss->floorlightabsolute = ss->ceilinglightabsolute = false;

		ss->colormap_protected = false;

		ss->gravity = FRACUNIT;

		ss->flags = MSF_FLIPSPECIAL_FLOOR;
		ss->specialflags = 0;
		ss->damagetype = SD_NONE;
		ss->triggertag = 0;
		ss->triggerer = TO_PLAYER;

		ss->friction = ORIG_FRICTION;

		P_InitializeSector(ss);
	}
}

static void P_InitializeLinedef(line_t *ld)
{
	vertex_t *v1 = ld->v1;
	vertex_t *v2 = ld->v2;
	UINT8 j;

	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	ld->angle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

	ld->bbox[BOXLEFT] = min(v1->x, v2->x);
	ld->bbox[BOXRIGHT] = max(v1->x, v2->x);
	ld->bbox[BOXBOTTOM] = min(v1->y, v2->y);
	ld->bbox[BOXTOP] = max(v1->y, v2->y);

	if (!ld->dx)
		ld->slopetype = ST_VERTICAL;
	else if (!ld->dy)
		ld->slopetype = ST_HORIZONTAL;
	else if ((ld->dy > 0) == (ld->dx > 0))
		ld->slopetype = ST_POSITIVE;
	else
		ld->slopetype = ST_NEGATIVE;

	ld->frontsector = ld->backsector = NULL;

	ld->validcount = 0;
	ld->polyobj = NULL;

	ld->text = NULL;
	ld->callcount = 0;

	// cph 2006/09/30 - fix sidedef errors right away.
	// cph 2002/07/20 - these errors are fatal if not fixed, so apply them
	for (j = 0; j < 2; j++)
		if (ld->sidenum[j] != 0xffff && ld->sidenum[j] >= (UINT16)numsides)
		{
			ld->sidenum[j] = 0xffff;
			CONS_Debug(DBG_SETUP, "P_InitializeLinedef: Linedef %s has out-of-range sidedef number\n", sizeu1((size_t)(ld - lines)));
		}

	// killough 11/98: fix common wad errors (missing sidedefs):
	if (ld->sidenum[0] == 0xffff)
	{
		ld->sidenum[0] = 0;  // Substitute dummy sidedef for missing right side
		// cph - print a warning about the bug
		CONS_Debug(DBG_SETUP, "P_InitializeLinedef: Linedef %s missing first sidedef\n", sizeu1((size_t)(ld - lines)));
	}

	if ((ld->sidenum[1] == 0xffff) && (ld->flags & ML_TWOSIDED))
	{
		ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side
		// cph - print a warning about the bug
		CONS_Debug(DBG_SETUP, "P_InitializeLinedef: Linedef %s has two-sided flag set, but no second sidedef\n", sizeu1((size_t)(ld - lines)));
	}

	if (ld->sidenum[0] != 0xffff)
	{
		sides[ld->sidenum[0]].special = ld->special;
		sides[ld->sidenum[0]].line = ld;
	}
	if (ld->sidenum[1] != 0xffff)
	{
		sides[ld->sidenum[1]].special = ld->special;
		sides[ld->sidenum[1]].line = ld;
	}
}

static void P_SetLinedefV1(size_t i, UINT16 vertex_num)
{
	if (vertex_num >= numvertexes)
	{
		CONS_Debug(DBG_SETUP, "P_SetLinedefV1: linedef %s has out-of-range v1 num %u\n", sizeu1(i), vertex_num);
		vertex_num = 0;
	}
	lines[i].v1 = &vertexes[vertex_num];
}

static void P_SetLinedefV2(size_t i, UINT16 vertex_num)
{
	if (vertex_num >= numvertexes)
	{
		CONS_Debug(DBG_SETUP, "P_SetLinedefV2: linedef %s has out-of-range v2 num %u\n", sizeu1(i), vertex_num);
		vertex_num = 0;
	}
	lines[i].v2 = &vertexes[vertex_num];
}

static void P_LoadLinedefs(UINT8 *data)
{
	maplinedef_t *mld = (maplinedef_t *)data;
	line_t *ld = lines;
	size_t i;

	for (i = 0; i < numlines; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);
		ld->special = SHORT(mld->special);
		Tag_FSet(&ld->tags, SHORT(mld->tag));
		memset(ld->args, 0, NUMLINEARGS*sizeof(*ld->args));
		memset(ld->stringargs, 0x00, NUMLINESTRINGARGS*sizeof(*ld->stringargs));
		ld->alpha = FRACUNIT;
		ld->executordelay = 0;
		P_SetLinedefV1(i, SHORT(mld->v1));
		P_SetLinedefV2(i, SHORT(mld->v2));

		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);

		P_InitializeLinedef(ld);
	}
}

static void P_SetSidedefSector(size_t i, UINT16 sector_num)
{
	// cph 2006/09/30 - catch out-of-range sector numbers; use sector 0 instead
	if (sector_num >= numsectors)
	{
		CONS_Debug(DBG_SETUP, "P_SetSidedefSector: sidedef %s has out-of-range sector num %u\n", sizeu1(i), sector_num);
		sector_num = 0;
	}
	sides[i].sector = &sectors[sector_num];
}

static void P_InitializeSidedef(side_t *sd)
{
	if (!sd->line)
	{
		CONS_Debug(DBG_SETUP, "P_LoadSidedefs: Sidedef %s is not used by any linedef\n", sizeu1((size_t)(sd - sides)));
		sd->line = &lines[0];
		sd->special = sd->line->special;
	}

	sd->text = NULL;
	sd->colormap_data = NULL;
}

static void P_LoadSidedefs(UINT8 *data)
{
	mapsidedef_t *msd = (mapsidedef_t*)data;
	side_t *sd = sides;
	size_t i;

	for (i = 0; i < numsides; i++, sd++, msd++)
	{
		INT16 textureoffset = SHORT(msd->textureoffset);
		boolean isfrontside;

		P_InitializeSidedef(sd);

		isfrontside = sd->line->sidenum[0] == i;

		// Repeat count for midtexture
		if (((sd->line->flags & (ML_TWOSIDED|ML_WRAPMIDTEX)) == (ML_TWOSIDED|ML_WRAPMIDTEX))
			&& !(sd->special >= 300 && sd->special < 500)) // exempt linedef exec specials
		{
			sd->repeatcnt = (INT16)(((UINT16)textureoffset) >> 12);
			sd->textureoffset = (((UINT16)textureoffset) & 2047) << FRACBITS;
		}
		else
		{
			sd->repeatcnt = 0;
			sd->textureoffset = textureoffset << FRACBITS;
		}
		sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

		P_SetSidedefSector(i, SHORT(msd->sector));

		// Special info stored in texture fields!
		switch (sd->special)
		{
			case 606: //SoM: 4/4/2000: Just colormap transfer
			case 447: // Change colormap of tagged sectors! -- Monster Iestyn 14/06/18
			case 455: // Fade colormaps! mazmazz 9/12/2018 (:flag_us:)
				// SoM: R_CreateColormap will only create a colormap in software mode...
				// Perhaps we should just call it instead of doing the calculations here.
				sd->colormap_data = R_CreateColormapFromLinedef(msd->toptexture, msd->midtexture, msd->bottomtexture);
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				break;

			case 413: // Change music
			{
				char process[8+1];

				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->bottomtexture[0] != '-' || msd->bottomtexture[1] != '\0')
				{
					M_Memcpy(process,msd->bottomtexture,8);
					process[8] = '\0';
					sd->bottomtexture = get_number(process);
				}

				if (!(msd->midtexture[0] == '-' && msd->midtexture[1] == '\0') || msd->midtexture[1] != '\0')
				{
					M_Memcpy(process,msd->midtexture,8);
					process[8] = '\0';
					sd->midtexture = get_number(process);
				}

				sd->text = Z_Malloc(7, PU_LEVEL, NULL);
				if (isfrontside && !(msd->toptexture[0] == '-' && msd->toptexture[1] == '\0'))
				{
					M_Memcpy(process,msd->toptexture,8);
					process[8] = '\0';

					// If they type in O_ or D_ and their music name, just shrug,
					// then copy the rest instead.
					if ((process[0] == 'O' || process[0] == 'D') && process[7])
						M_Memcpy(sd->text, process+2, 6);
					else // Assume it's a proper music name.
						M_Memcpy(sd->text, process, 6);
					sd->text[6] = 0;
				}
				else
					sd->text[0] = 0;
				break;
			}

			case 4: // Speed pad parameters
			{
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->toptexture[0] != '-' || msd->toptexture[1] != '\0')
				{
					char process[8+1];
					M_Memcpy(process,msd->toptexture,8);
					process[8] = '\0';
					sd->toptexture = get_number(process);
				}
				break;
			}

			case 414: // Play SFX
			{
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->toptexture[0] != '-' || msd->toptexture[1] != '\0')
				{
					char process[8 + 1];
					M_Memcpy(process, msd->toptexture, 8);
					process[8] = '\0';
					sd->text = Z_Malloc(strlen(process) + 1, PU_LEVEL, NULL);
					M_Memcpy(sd->text, process, strlen(process) + 1);
				}
				break;
			}

			case 9: // Mace parameters
			case 14: // Bustable block parameters
			case 15: // Fan particle spawner parameters
			{
				char process[8*3+1];
				memset(process,0,8*3+1);
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->toptexture[0] == '-' && msd->toptexture[1] == '\0')
					break;
				else
					M_Memcpy(process,msd->toptexture,8);
				if (msd->midtexture[0] != '-' || msd->midtexture[1] != '\0')
					M_Memcpy(process+strlen(process), msd->midtexture, 8);
				if (msd->bottomtexture[0] != '-' || msd->bottomtexture[1] != '\0')
					M_Memcpy(process+strlen(process), msd->bottomtexture, 8);
				sd->toptexture = get_number(process);
				break;
			}

			case 331: // Trigger linedef executor: Skin - Continuous
			case 332: // Trigger linedef executor: Skin - Each time
			case 333: // Trigger linedef executor: Skin - Once
			case 334: // Trigger linedef executor: Object dye - Continuous
			case 335: // Trigger linedef executor: Object dye - Each time
			case 336: // Trigger linedef executor: Object dye - Once
			case 425: // Calls P_SetMobjState on calling mobj
			case 434: // Custom Power
			case 442: // Calls P_SetMobjState on mobjs of a given type in the tagged sectors
			case 443: // Calls a named Lua function
			case 459: // Control text prompt (named tag)
			case 461: // Spawns an object on the map based on texture offsets
			case 463: // Colorizes an object
			{
				char process[8*3+1];
				memset(process,0,8*3+1);
				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->toptexture[0] == '-' && msd->toptexture[1] == '\0')
					break;
				else
					M_Memcpy(process,msd->toptexture,8);
				if (msd->midtexture[0] != '-' || msd->midtexture[1] != '\0')
					M_Memcpy(process+strlen(process), msd->midtexture, 8);
				if (msd->bottomtexture[0] != '-' || msd->bottomtexture[1] != '\0')
					M_Memcpy(process+strlen(process), msd->bottomtexture, 8);
				sd->text = Z_Malloc(strlen(process)+1, PU_LEVEL, NULL);
				M_Memcpy(sd->text, process, strlen(process)+1);
				break;
			}

			case 259: // Custom FOF
				if (!isfrontside)
				{
					if ((msd->toptexture[0] >= '0' && msd->toptexture[0] <= '9')
						|| (msd->toptexture[0] >= 'A' && msd->toptexture[0] <= 'F'))
						sd->toptexture = axtoi(msd->toptexture);
					else
						I_Error("Custom FOF (line id %s) needs a value in the linedef's back side upper texture field.", sizeu1(sd->line - lines));

					sd->midtexture = R_TextureNumForName(msd->midtexture);
					sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
					break;
				}
				// FALLTHRU
			default: // normal cases
				if (msd->toptexture[0] == '#')
				{
					char *col = msd->toptexture;
					sd->toptexture =
						((col[1]-'0')*100 + (col[2]-'0')*10 + col[3]-'0')+1;
					if (col[4]) // extra num for blendmode
						sd->toptexture += (col[4]-'0')*1000;
					sd->bottomtexture = sd->toptexture;
					sd->midtexture = R_TextureNumForName(msd->midtexture);
				}
				else
				{
					sd->midtexture = R_TextureNumForName(msd->midtexture);
					sd->toptexture = R_TextureNumForName(msd->toptexture);
					sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
				}
				break;
		}
	}
}

static void P_LoadThings(UINT8 *data)
{
	mapthing_t *mt;
	size_t i;

	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		mt->x = READINT16(data);
		mt->y = READINT16(data);

		mt->angle = READINT16(data);
		mt->type = READUINT16(data);
		mt->options = READUINT16(data);
		mt->extrainfo = (UINT8)(mt->type >> 12);
		Tag_FSet(&mt->tags, 0);
		mt->scale = FRACUNIT;
		memset(mt->args, 0, NUMMAPTHINGARGS*sizeof(*mt->args));
		memset(mt->stringargs, 0x00, NUMMAPTHINGSTRINGARGS*sizeof(*mt->stringargs));
		mt->pitch = mt->roll = 0;

		mt->type &= 4095;

		if (mt->type == 1705 || (mt->type == 750 && mt->extrainfo))
			mt->z = mt->options; // NiGHTS Hoops use the full flags bits to set the height.
		else
			mt->z = mt->options >> ZSHIFT;

		mt->mobj = NULL;
	}
}

// Stores positions for relevant map data spread through a TEXTMAP.
UINT32 mapthingsPos[UINT16_MAX];
UINT32 linesPos[UINT16_MAX];
UINT32 sidesPos[UINT16_MAX];
UINT32 vertexesPos[UINT16_MAX];
UINT32 sectorsPos[UINT16_MAX];

// Determine total amount of map data in TEXTMAP.
static boolean TextmapCount(size_t size)
{
	const char *tkn = M_TokenizerRead(0);
	UINT8 brackets = 0;

	nummapthings = 0;
	numlines = 0;
	numsides = 0;
	numvertexes = 0;
	numsectors = 0;

	// Look for namespace at the beginning.
	if (!fastcmp(tkn, "namespace"))
	{
		CONS_Alert(CONS_ERROR, "No namespace at beginning of lump!\n");
		return false;
	}

	// Check if namespace is valid.
	tkn = M_TokenizerRead(0);
	if (!fastcmp(tkn, "srb2"))
		CONS_Alert(CONS_WARNING, "Invalid namespace '%s', only 'srb2' is supported.\n", tkn);

	while ((tkn = M_TokenizerRead(0)) && M_TokenizerGetEndPos() < size)
	{
		// Avoid anything inside bracketed stuff, only look for external keywords.
		if (brackets)
		{
			if (fastcmp(tkn, "}"))
				brackets--;
		}
		else if (fastcmp(tkn, "{"))
			brackets++;
		// Check for valid fields.
		else if (fastcmp(tkn, "thing"))
			mapthingsPos[nummapthings++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "linedef"))
			linesPos[numlines++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "sidedef"))
			sidesPos[numsides++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "vertex"))
			vertexesPos[numvertexes++] = M_TokenizerGetEndPos();
		else if (fastcmp(tkn, "sector"))
			sectorsPos[numsectors++] = M_TokenizerGetEndPos();
		else
			CONS_Alert(CONS_NOTICE, "Unknown field '%s'.\n", tkn);
	}

	if (brackets)
	{
		CONS_Alert(CONS_ERROR, "Unclosed brackets detected in textmap lump.\n");
		return false;
	}

	return true;
}

static void ParseTextmapVertexParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "x"))
		vertexes[i].x = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "y"))
		vertexes[i].y = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "zfloor"))
	{
		vertexes[i].floorz = FLOAT_TO_FIXED(atof(val));
		vertexes[i].floorzset = true;
	}
	else if (fastcmp(param, "zceiling"))
	{
		vertexes[i].ceilingz = FLOAT_TO_FIXED(atof(val));
		vertexes[i].ceilingzset = true;
	}
}

typedef struct textmap_colormap_s {
	boolean used;
	INT32 lightcolor;
	UINT8 lightalpha;
	INT32 fadecolor;
	UINT8 fadealpha;
	UINT8 fadestart;
	UINT8 fadeend;
	UINT8 flags;
} textmap_colormap_t;

textmap_colormap_t textmap_colormap = { false, 0, 25, 0, 25, 0, 31, 0 };

typedef enum
{
    PD_A = 1,
    PD_B = 1<<1,
    PD_C = 1<<2,
    PD_D = 1<<3,
} planedef_t;

typedef struct textmap_plane_s {
    UINT8 defined;
    fixed_t a, b, c, d;
} textmap_plane_t;

textmap_plane_t textmap_planefloor = {0, 0, 0, 0, 0};
textmap_plane_t textmap_planeceiling = {0, 0, 0, 0, 0};

static void ParseTextmapSectorParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "heightfloor"))
		sectors[i].floorheight = atol(val) << FRACBITS;
	else if (fastcmp(param, "heightceiling"))
		sectors[i].ceilingheight = atol(val) << FRACBITS;
	if (fastcmp(param, "texturefloor"))
		sectors[i].floorpic = P_AddLevelFlat(val, foundflats);
	else if (fastcmp(param, "textureceiling"))
		sectors[i].ceilingpic = P_AddLevelFlat(val, foundflats);
	else if (fastcmp(param, "lightlevel"))
		sectors[i].lightlevel = atol(val);
	else if (fastcmp(param, "lightfloor"))
		sectors[i].floorlightlevel = atol(val);
	else if (fastcmp(param, "lightfloorabsolute") && fastcmp("true", val))
		sectors[i].floorlightabsolute = true;
	else if (fastcmp(param, "lightceiling"))
		sectors[i].ceilinglightlevel = atol(val);
	else if (fastcmp(param, "lightceilingabsolute") && fastcmp("true", val))
		sectors[i].ceilinglightabsolute = true;
	else if (fastcmp(param, "id"))
		Tag_FSet(&sectors[i].tags, atol(val));
	else if (fastcmp(param, "moreids"))
	{
		const char* id = val;
		while (id)
		{
			Tag_Add(&sectors[i].tags, atol(id));
			if ((id = strchr(id, ' ')))
				id++;
		}
	}
	else if (fastcmp(param, "xpanningfloor"))
		sectors[i].floor_xoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "ypanningfloor"))
		sectors[i].floor_yoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "xpanningceiling"))
		sectors[i].ceiling_xoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "ypanningceiling"))
		sectors[i].ceiling_yoffs = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "rotationfloor"))
		sectors[i].floorpic_angle = FixedAngle(FLOAT_TO_FIXED(atof(val)));
	else if (fastcmp(param, "rotationceiling"))
		sectors[i].ceilingpic_angle = FixedAngle(FLOAT_TO_FIXED(atof(val)));
	else if (fastcmp(param, "floorplane_a"))
	{
		textmap_planefloor.defined |= PD_A;
		textmap_planefloor.a = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "floorplane_b"))
	{
		textmap_planefloor.defined |= PD_B;
		textmap_planefloor.b = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "floorplane_c"))
	{
		textmap_planefloor.defined |= PD_C;
		textmap_planefloor.c = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "floorplane_d"))
	{
		textmap_planefloor.defined |= PD_D;
		textmap_planefloor.d = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_a"))
	{
		textmap_planeceiling.defined |= PD_A;
		textmap_planeceiling.a = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_b"))
	{
		textmap_planeceiling.defined |= PD_B;
		textmap_planeceiling.b = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_c"))
	{
		textmap_planeceiling.defined |= PD_C;
		textmap_planeceiling.c = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "ceilingplane_d"))
	{
		textmap_planeceiling.defined |= PD_D;
		textmap_planeceiling.d = FLOAT_TO_FIXED(atof(val));
	}
	else if (fastcmp(param, "lightcolor"))
	{
		textmap_colormap.used = true;
		textmap_colormap.lightcolor = atol(val);
	}
	else if (fastcmp(param, "lightalpha"))
	{
		textmap_colormap.used = true;
		textmap_colormap.lightalpha = atol(val);
	}
	else if (fastcmp(param, "fadecolor"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadecolor = atol(val);
	}
	else if (fastcmp(param, "fadealpha"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadealpha = atol(val);
	}
	else if (fastcmp(param, "fadestart"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadestart = atol(val);
	}
	else if (fastcmp(param, "fadeend"))
	{
		textmap_colormap.used = true;
		textmap_colormap.fadeend = atol(val);
	}
	else if (fastcmp(param, "colormapfog") && fastcmp("true", val))
	{
		textmap_colormap.used = true;
		textmap_colormap.flags |= CMF_FOG;
	}
	else if (fastcmp(param, "colormapfadesprites") && fastcmp("true", val))
	{
		textmap_colormap.used = true;
		textmap_colormap.flags |= CMF_FADEFULLBRIGHTSPRITES;
	}
	else if (fastcmp(param, "colormapprotected") && fastcmp("true", val))
		sectors[i].colormap_protected = true;
	else if (fastcmp(param, "flipspecial_nofloor") && fastcmp("true", val))
		sectors[i].flags &= ~MSF_FLIPSPECIAL_FLOOR;
	else if (fastcmp(param, "flipspecial_ceiling") && fastcmp("true", val))
		sectors[i].flags |= MSF_FLIPSPECIAL_CEILING;
	else if (fastcmp(param, "triggerspecial_touch") && fastcmp("true", val))
		sectors[i].flags |= MSF_TRIGGERSPECIAL_TOUCH;
	else if (fastcmp(param, "triggerspecial_headbump") && fastcmp("true", val))
		sectors[i].flags |= MSF_TRIGGERSPECIAL_HEADBUMP;
	else if (fastcmp(param, "triggerline_plane") && fastcmp("true", val))
		sectors[i].flags |= MSF_TRIGGERLINE_PLANE;
	else if (fastcmp(param, "triggerline_mobj") && fastcmp("true", val))
		sectors[i].flags |= MSF_TRIGGERLINE_MOBJ;
	else if (fastcmp(param, "invertprecip") && fastcmp("true", val))
		sectors[i].flags |= MSF_INVERTPRECIP;
	else if (fastcmp(param, "gravityflip") && fastcmp("true", val))
		sectors[i].flags |= MSF_GRAVITYFLIP;
	else if (fastcmp(param, "heatwave") && fastcmp("true", val))
		sectors[i].flags |= MSF_HEATWAVE;
	else if (fastcmp(param, "noclipcamera") && fastcmp("true", val))
		sectors[i].flags |= MSF_NOCLIPCAMERA;
	else if (fastcmp(param, "outerspace") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_OUTERSPACE;
	else if (fastcmp(param, "doublestepup") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_DOUBLESTEPUP;
	else if (fastcmp(param, "nostepdown") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_NOSTEPDOWN;
	else if (fastcmp(param, "speedpad") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_SPEEDPAD;
	else if (fastcmp(param, "starpostactivator") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_STARPOSTACTIVATOR;
	else if (fastcmp(param, "exit") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_EXIT;
	else if (fastcmp(param, "specialstagepit") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_SPECIALSTAGEPIT;
	else if (fastcmp(param, "returnflag") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_RETURNFLAG;
	else if (fastcmp(param, "redteambase") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_REDTEAMBASE;
	else if (fastcmp(param, "blueteambase") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_BLUETEAMBASE;
	else if (fastcmp(param, "fan") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_FAN;
	else if (fastcmp(param, "supertransform") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_SUPERTRANSFORM;
	else if (fastcmp(param, "forcespin") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_FORCESPIN;
	else if (fastcmp(param, "zoomtubestart") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_ZOOMTUBESTART;
	else if (fastcmp(param, "zoomtubeend") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_ZOOMTUBEEND;
	else if (fastcmp(param, "finishline") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_FINISHLINE;
	else if (fastcmp(param, "ropehang") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_ROPEHANG;
	else if (fastcmp(param, "jumpflip") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_JUMPFLIP;
	else if (fastcmp(param, "gravityoverride") && fastcmp("true", val))
		sectors[i].specialflags |= SSF_GRAVITYOVERRIDE;
	else if (fastcmp(param, "friction"))
		sectors[i].friction = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "gravity"))
		sectors[i].gravity = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "damagetype"))
	{
		if (fastcmp(val, "Generic"))
			sectors[i].damagetype = SD_GENERIC;
		if (fastcmp(val, "Water"))
			sectors[i].damagetype = SD_WATER;
		if (fastcmp(val, "Fire"))
			sectors[i].damagetype = SD_FIRE;
		if (fastcmp(val, "Lava"))
			sectors[i].damagetype = SD_LAVA;
		if (fastcmp(val, "Electric"))
			sectors[i].damagetype = SD_ELECTRIC;
		if (fastcmp(val, "Spike"))
			sectors[i].damagetype = SD_SPIKE;
		if (fastcmp(val, "DeathPitTilt"))
			sectors[i].damagetype = SD_DEATHPITTILT;
		if (fastcmp(val, "DeathPitNoTilt"))
			sectors[i].damagetype = SD_DEATHPITNOTILT;
		if (fastcmp(val, "Instakill"))
			sectors[i].damagetype = SD_INSTAKILL;
		if (fastcmp(val, "SpecialStage"))
			sectors[i].damagetype = SD_SPECIALSTAGE;
	}
	else if (fastcmp(param, "triggertag"))
		sectors[i].triggertag = atol(val);
	else if (fastcmp(param, "triggerer"))
	{
		if (fastcmp(val, "Player"))
			sectors[i].triggerer = TO_PLAYER;
		if (fastcmp(val, "AllPlayers"))
			sectors[i].triggerer = TO_ALLPLAYERS;
		if (fastcmp(val, "Mobj"))
			sectors[i].triggerer = TO_MOBJ;
	}
}

static void ParseTextmapSidedefParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "offsetx"))
		sides[i].textureoffset = atol(val)<<FRACBITS;
	else if (fastcmp(param, "offsety"))
		sides[i].rowoffset = atol(val)<<FRACBITS;
	else if (fastcmp(param, "texturetop"))
		sides[i].toptexture = R_TextureNumForName(val);
	else if (fastcmp(param, "texturebottom"))
		sides[i].bottomtexture = R_TextureNumForName(val);
	else if (fastcmp(param, "texturemiddle"))
		sides[i].midtexture = R_TextureNumForName(val);
	else if (fastcmp(param, "sector"))
		P_SetSidedefSector(i, atol(val));
	else if (fastcmp(param, "repeatcnt"))
		sides[i].repeatcnt = atol(val);
}

static void ParseTextmapLinedefParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "id"))
		Tag_FSet(&lines[i].tags, atol(val));
	else if (fastcmp(param, "moreids"))
	{
		const char* id = val;
		while (id)
		{
			Tag_Add(&lines[i].tags, atol(id));
			if ((id = strchr(id, ' ')))
				id++;
		}
	}
	else if (fastcmp(param, "special"))
		lines[i].special = atol(val);
	else if (fastcmp(param, "v1"))
		P_SetLinedefV1(i, atol(val));
	else if (fastcmp(param, "v2"))
		P_SetLinedefV2(i, atol(val));
	else if (fastncmp(param, "stringarg", 9) && strlen(param) > 9)
	{
		size_t argnum = atol(param + 9);
		if (argnum >= NUMLINESTRINGARGS)
			return;
		lines[i].stringargs[argnum] = Z_Malloc(strlen(val) + 1, PU_LEVEL, NULL);
		M_Memcpy(lines[i].stringargs[argnum], val, strlen(val) + 1);
	}
	else if (fastncmp(param, "arg", 3) && strlen(param) > 3)
	{
		size_t argnum = atol(param + 3);
		if (argnum >= NUMLINEARGS)
			return;
		lines[i].args[argnum] = atol(val);
	}
	else if (fastcmp(param, "sidefront"))
		lines[i].sidenum[0] = atol(val);
	else if (fastcmp(param, "sideback"))
		lines[i].sidenum[1] = atol(val);
	else if (fastcmp(param, "alpha"))
		lines[i].alpha = FLOAT_TO_FIXED(atof(val));
	else if (fastcmp(param, "blendmode") || fastcmp(param, "renderstyle"))
	{
		if (fastcmp(val, "translucent"))
			lines[i].blendmode = AST_COPY;
		else if (fastcmp(val, "add"))
			lines[i].blendmode = AST_ADD;
		else if (fastcmp(val, "subtract"))
			lines[i].blendmode = AST_SUBTRACT;
		else if (fastcmp(val, "reversesubtract"))
			lines[i].blendmode = AST_REVERSESUBTRACT;
		else if (fastcmp(val, "modulate"))
			lines[i].blendmode = AST_MODULATE;
		if (fastcmp(val, "fog"))
			lines[i].blendmode = AST_FOG;
	}
	else if (fastcmp(param, "executordelay"))
		lines[i].executordelay = atol(val);

	// Flags
	else if (fastcmp(param, "blocking") && fastcmp("true", val))
		lines[i].flags |= ML_IMPASSIBLE;
	else if (fastcmp(param, "blockmonsters") && fastcmp("true", val))
		lines[i].flags |= ML_BLOCKMONSTERS;
	else if (fastcmp(param, "twosided") && fastcmp("true", val))
		lines[i].flags |= ML_TWOSIDED;
	else if (fastcmp(param, "dontpegtop") && fastcmp("true", val))
		lines[i].flags |= ML_DONTPEGTOP;
	else if (fastcmp(param, "dontpegbottom") && fastcmp("true", val))
		lines[i].flags |= ML_DONTPEGBOTTOM;
	else if (fastcmp(param, "skewtd") && fastcmp("true", val))
		lines[i].flags |= ML_SKEWTD;
	else if (fastcmp(param, "noclimb") && fastcmp("true", val))
		lines[i].flags |= ML_NOCLIMB;
	else if (fastcmp(param, "noskew") && fastcmp("true", val))
		lines[i].flags |= ML_NOSKEW;
	else if (fastcmp(param, "midpeg") && fastcmp("true", val))
		lines[i].flags |= ML_MIDPEG;
	else if (fastcmp(param, "midsolid") && fastcmp("true", val))
		lines[i].flags |= ML_MIDSOLID;
	else if (fastcmp(param, "wrapmidtex") && fastcmp("true", val))
		lines[i].flags |= ML_WRAPMIDTEX;
	/*else if (fastcmp(param, "effect6") && fastcmp("true", val))
		lines[i].flags |= ML_EFFECT6;*/
	else if (fastcmp(param, "nonet") && fastcmp("true", val))
		lines[i].flags |= ML_NONET;
	else if (fastcmp(param, "netonly") && fastcmp("true", val))
		lines[i].flags |= ML_NETONLY;
	else if (fastcmp(param, "bouncy") && fastcmp("true", val))
		lines[i].flags |= ML_BOUNCY;
	else if (fastcmp(param, "transfer") && fastcmp("true", val))
		lines[i].flags |= ML_TFERLINE;
}

static void ParseTextmapThingParameter(UINT32 i, const char *param, const char *val)
{
	if (fastcmp(param, "id"))
		Tag_FSet(&mapthings[i].tags, atol(val));
	else if (fastcmp(param, "moreids"))
	{
		const char* id = val;
		while (id)
		{
			Tag_Add(&mapthings[i].tags, atol(id));
			if ((id = strchr(id, ' ')))
				id++;
		}
	}
	else if (fastcmp(param, "x"))
		mapthings[i].x = atol(val);
	else if (fastcmp(param, "y"))
		mapthings[i].y = atol(val);
	else if (fastcmp(param, "height"))
		mapthings[i].z = atol(val);
	else if (fastcmp(param, "angle"))
		mapthings[i].angle = atol(val);
	else if (fastcmp(param, "pitch"))
		mapthings[i].pitch = atol(val);
	else if (fastcmp(param, "roll"))
		mapthings[i].roll = atol(val);
	else if (fastcmp(param, "type"))
		mapthings[i].type = atol(val);
	else if (fastcmp(param, "scale") || fastcmp(param, "scalex") || fastcmp(param, "scaley"))
		mapthings[i].scale = FLOAT_TO_FIXED(atof(val));
	// Flags
	else if (fastcmp(param, "flip") && fastcmp("true", val))
		mapthings[i].options |= MTF_OBJECTFLIP;

	else if (fastncmp(param, "stringarg", 9) && strlen(param) > 9)
	{
		size_t argnum = atol(param + 9);
		if (argnum >= NUMMAPTHINGSTRINGARGS)
			return;
		mapthings[i].stringargs[argnum] = Z_Malloc(strlen(val) + 1, PU_LEVEL, NULL);
		M_Memcpy(mapthings[i].stringargs[argnum], val, strlen(val) + 1);
	}
	else if (fastncmp(param, "arg", 3) && strlen(param) > 3)
	{
		size_t argnum = atol(param + 3);
		if (argnum >= NUMMAPTHINGARGS)
			return;
		mapthings[i].args[argnum] = atol(val);
	}
}

/** From a given position table, run a specified parser function through a {}-encapsuled text.
  *
  * \param Position of the data to parse, in the textmap.
  * \param Structure number (mapthings, sectors, ...).
  * \param Parser function pointer.
  */
static void TextmapParse(UINT32 dataPos, size_t num, void (*parser)(UINT32, const char *, const char *))
{
	const char *param, *val;

	M_TokenizerSetEndPos(dataPos);
	param = M_TokenizerRead(0);
	if (!fastcmp(param, "{"))
	{
		CONS_Alert(CONS_WARNING, "Invalid UDMF data capsule!\n");
		return;
	}

	while (true)
	{
		param = M_TokenizerRead(0);
		if (fastcmp(param, "}"))
			break;
		val = M_TokenizerRead(1);
		parser(num, param, val);
	}
}

/** Provides a fix to the flat alignment coordinate transform from standard Textmaps.
 */
static void TextmapFixFlatOffsets(sector_t *sec)
{
	if (sec->floorpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->floorpic_angle>>ANGLETOFINESHIFT);
		fixed_t ps = FINESINE  (sec->floorpic_angle>>ANGLETOFINESHIFT);
		fixed_t xoffs = sec->floor_xoffs;
		fixed_t yoffs = sec->floor_yoffs;
		sec->floor_xoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
		sec->floor_yoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
	}

	if (sec->ceilingpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->ceilingpic_angle>>ANGLETOFINESHIFT);
		fixed_t ps = FINESINE  (sec->ceilingpic_angle>>ANGLETOFINESHIFT);
		fixed_t xoffs = sec->ceiling_xoffs;
		fixed_t yoffs = sec->ceiling_yoffs;
		sec->ceiling_xoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
		sec->ceiling_yoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
	}
}

static void TextmapUnfixFlatOffsets(sector_t *sec)
{
	if (sec->floorpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->floorpic_angle >> ANGLETOFINESHIFT);
		fixed_t ps = FINESINE(sec->floorpic_angle >> ANGLETOFINESHIFT);
		fixed_t xoffs = sec->floor_xoffs;
		fixed_t yoffs = sec->floor_yoffs;
		sec->floor_xoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
		sec->floor_yoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
	}

	if (sec->ceilingpic_angle)
	{
		fixed_t pc = FINECOSINE(sec->ceilingpic_angle >> ANGLETOFINESHIFT);
		fixed_t ps = FINESINE(sec->ceilingpic_angle >> ANGLETOFINESHIFT);
		fixed_t xoffs = sec->ceiling_xoffs;
		fixed_t yoffs = sec->ceiling_yoffs;
		sec->ceiling_xoffs = (FixedMul(xoffs, ps) % MAXFLATSIZE) + (FixedMul(yoffs, pc) % MAXFLATSIZE);
		sec->ceiling_yoffs = (FixedMul(xoffs, pc) % MAXFLATSIZE) - (FixedMul(yoffs, ps) % MAXFLATSIZE);
	}
}

static INT32 P_ColorToRGBA(INT32 color, UINT8 alpha)
{
	UINT8 r = (color >> 16) & 0xFF;
	UINT8 g = (color >> 8) & 0xFF;
	UINT8 b = color & 0xFF;
	return R_PutRgbaRGBA(r, g, b, alpha);
}

static INT32 P_RGBAToColor(INT32 rgba)
{
	UINT8 r = R_GetRgbaR(rgba);
	UINT8 g = R_GetRgbaG(rgba);
	UINT8 b = R_GetRgbaB(rgba);
	return (r << 16) | (g << 8) | b;
}

typedef struct
{
	mapthing_t *teleport;
	mapthing_t *altview;
	mapthing_t *angleanchor;
} sectorspecialthings_t;

static void P_WriteTextmap(void)
{
	size_t i, j;
	FILE *f;
	char *filepath = va(pandf, srb2home, "TEXTMAP");
	mtag_t firsttag;
	mapthing_t *wmapthings;
	vertex_t *wvertexes;
	sector_t *wsectors;
	line_t *wlines;
	side_t *wsides;
	mtag_t freetag;
	sectorspecialthings_t *specialthings;

	f = fopen(filepath, "w");
	if (!f)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Couldn't save map file %s\n"), filepath);
		return;
	}

	wmapthings = Z_Calloc(nummapthings * sizeof(*mapthings), PU_LEVEL, NULL);
	wvertexes = Z_Calloc(numvertexes * sizeof(*vertexes), PU_LEVEL, NULL);
	wsectors = Z_Calloc(numsectors * sizeof(*sectors), PU_LEVEL, NULL);
	wlines = Z_Calloc(numlines * sizeof(*lines), PU_LEVEL, NULL);
	wsides = Z_Calloc(numsides * sizeof(*sides), PU_LEVEL, NULL);
	specialthings = Z_Calloc(numsectors * sizeof(*sectors), PU_LEVEL, NULL);

	memcpy(wmapthings, mapthings, nummapthings * sizeof(*mapthings));
	memcpy(wvertexes, vertexes, numvertexes * sizeof(*vertexes));
	memcpy(wsectors, sectors, numsectors * sizeof(*sectors));
	memcpy(wlines, lines, numlines * sizeof(*lines));
	memcpy(wsides, sides, numsides * sizeof(*sides));

	for (i = 0; i < nummapthings; i++)
		if (mapthings[i].tags.count)
			wmapthings[i].tags.tags = memcpy(Z_Malloc(mapthings[i].tags.count * sizeof(mtag_t), PU_LEVEL, NULL), mapthings[i].tags.tags, mapthings[i].tags.count * sizeof(mtag_t));

	for (i = 0; i < numsectors; i++)
		if (sectors[i].tags.count)
			wsectors[i].tags.tags = memcpy(Z_Malloc(sectors[i].tags.count*sizeof(mtag_t), PU_LEVEL, NULL), sectors[i].tags.tags, sectors[i].tags.count*sizeof(mtag_t));

	for (i = 0; i < numlines; i++)
		if (lines[i].tags.count)
			wlines[i].tags.tags = memcpy(Z_Malloc(lines[i].tags.count * sizeof(mtag_t), PU_LEVEL, NULL), lines[i].tags.tags, lines[i].tags.count * sizeof(mtag_t));

	freetag = Tag_NextUnused(0);

	for (i = 0; i < nummapthings; i++)
	{
		subsector_t *ss;
		INT32 s;

		if (wmapthings[i].type != 751 && wmapthings[i].type != 752 && wmapthings[i].type != 758)
			continue;

		ss = R_PointInSubsector(wmapthings[i].x << FRACBITS, wmapthings[i].y << FRACBITS);

		if (!ss)
			continue;

		s = ss->sector - sectors;

		switch (wmapthings[i].type)
		{
			case 751:
				if (!specialthings[s].teleport)
					specialthings[s].teleport = &wmapthings[i];
				break;
			case 752:
				if (!specialthings[s].altview)
					specialthings[s].altview = &wmapthings[i];
				break;
			case 758:
				if (!specialthings[s].angleanchor)
					specialthings[s].angleanchor = &wmapthings[i];
				break;
			default:
				break;
		}
	}

	for (i = 0; i < numlines; i++)
	{
		INT32 s;

		switch (wlines[i].special)
		{
			case 1:
				TAG_ITER_SECTORS(Tag_FGet(&wlines[i].tags), s)
				{
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s applies custom gravity to sector %d. Changes to this gravity at runtime will not be reflected in the converted map. Use linedef type 469 for this.\n"), sizeu1(i), s);
					wsectors[s].gravity = FixedDiv(lines[i].frontsector->floorheight >> FRACBITS, 1000);
				}
				break;
			case 2:
				CONS_Alert(CONS_WARNING, M_GetText("Custom exit linedef %s detected. Changes to the next map at runtime will not be reflected in the converted map. Use linedef type 468 for this.\n"), sizeu1(i));
				wlines[i].args[0] = lines[i].frontsector->floorheight >> FRACBITS;
				wlines[i].args[2] = lines[i].frontsector->ceilingheight >> FRACBITS;
				break;
			case 5:
			case 50:
			case 51:
				CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has type %d, which is not supported in UDMF.\n"), sizeu1(i), wlines[i].special);
				break;
			case 61:
				if (wlines[i].flags & ML_MIDSOLID)
					continue;
				if (!wlines[i].args[1])
					continue;
				CONS_Alert(CONS_WARNING, M_GetText("Linedef %s with crusher type 61 rises twice as fast on spawn. This behavior is not supported in UDMF.\n"), sizeu1(i));
				break;
			case 76:
				if (freetag == (mtag_t)MAXTAGS)
				{
					CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 76 cannot be converted.\n"), sizeu1(i));
					break;
				}
				TAG_ITER_SECTORS(wlines[i].args[0], s)
					for (j = 0; (unsigned)j < wsectors[s].linecount; j++)
					{
						line_t *line = wsectors[s].lines[j] - lines + wlines;
						if (line->special < 100 || line->special >= 300)
							continue;
						Tag_Add(&line->tags, freetag);
					}
				wlines[i].args[0] = freetag;
				freetag = Tag_NextUnused(freetag);
				break;
			case 259:
				if (wlines[i].args[3] & FOF_QUICKSAND)
					CONS_Alert(CONS_WARNING, M_GetText("Quicksand properties of custom FOF on linedef %s cannot be converted. Use linedef type 75 instead.\n"), sizeu1(i));
				if (wlines[i].args[3] & FOF_BUSTUP)
					CONS_Alert(CONS_WARNING, M_GetText("Bustable properties of custom FOF on linedef %s cannot be converted. Use linedef type 74 instead.\n"), sizeu1(i));
				break;
			case 412:
				if ((s = Tag_Iterate_Sectors(wlines[i].args[0], 0)) < 0)
					break;
				if (!specialthings[s].teleport)
					break;
				if (freetag == (mtag_t)MAXTAGS)
				{
					CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 412 cannot be converted.\n"), sizeu1(i));
					break;
				}
				Tag_Add(&specialthings[s].teleport->tags, freetag);
				wlines[i].args[0] = freetag;
				freetag = Tag_NextUnused(freetag);
				break;
			case 422:
				if ((s = Tag_Iterate_Sectors(wlines[i].args[0], 0)) < 0)
					break;
				if (!specialthings[s].altview)
					break;
				if (freetag == (mtag_t)MAXTAGS)
				{
					CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 422 cannot be converted.\n"), sizeu1(i));
					break;
				}
				Tag_Add(&specialthings[s].altview->tags, freetag);
				wlines[i].args[0] = freetag;
				specialthings[s].altview->pitch = wlines[i].args[2];
				freetag = Tag_NextUnused(freetag);
				break;
			case 447:
				CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has change colormap action, which cannot be converted automatically. Tag arg0 to a sector with the desired colormap.\n"), sizeu1(i));
				if (wlines[i].flags & ML_TFERLINE)
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s mixes front and back colormaps, which is not supported in UDMF. Copy one colormap to the target sector first, then mix in the second one.\n"), sizeu1(i));
				break;
			case 455:
				CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has fade colormap action, which cannot be converted automatically. Tag arg0 to a sector with the desired colormap.\n"), sizeu1(i));
				if (wlines[i].flags & ML_TFERLINE)
					CONS_Alert(CONS_WARNING, M_GetText("Linedef %s specifies starting colormap for the fade, which is not supported in UDMF. Change the colormap with linedef type 447 instead.\n"), sizeu1(i));
				break;
			case 457:
				if ((s = Tag_Iterate_Sectors(wlines[i].args[0], 0)) < 0)
					break;
				if (!specialthings[s].angleanchor)
					break;
				if (freetag == (mtag_t)MAXTAGS)
				{
					CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 457 cannot be converted.\n"), sizeu1(i));
					break;
				}
				Tag_Add(&specialthings[s].angleanchor->tags, freetag);
				wlines[i].args[0] = freetag;
				freetag = Tag_NextUnused(freetag);
				break;
			case 606:
				if (wlines[i].args[0] == MTAG_GLOBAL)
				{
					sector_t *sec = wlines[i].frontsector - sectors + wsectors;
					sec->extra_colormap = wsides[wlines[i].sidenum[0]].colormap_data;
				}
				else
				{
					TAG_ITER_SECTORS(wlines[i].args[0], s)
					{
						if (wsectors[s].colormap_protected)
							continue;

						wsectors[s].extra_colormap = wsides[wlines[i].sidenum[0]].colormap_data;
						if (freetag == (mtag_t)MAXTAGS)
						{
							CONS_Alert(CONS_WARNING, M_GetText("No unused tag found. Linedef %s with type 606 cannot be converted.\n"), sizeu1(i));
							break;
						}
						Tag_Add(&wsectors[s].tags, freetag);
						wlines[i].args[1] = freetag;
						freetag = Tag_NextUnused(freetag);
						break;
					}
				}
				break;
			default:
				break;
		}

		if (wlines[i].special >= 300 && wlines[i].special < 400 && wlines[i].flags & ML_WRAPMIDTEX)
			CONS_Alert(CONS_WARNING, M_GetText("Linedef executor trigger linedef %s has disregard order flag, which is not supported in UDMF.\n"), sizeu1(i));
	}

	for (i = 0; i < numsectors; i++)
	{
		if (Tag_Find(&wsectors[i].tags, LE_CAPSULE0))
			CONS_Alert(CONS_WARNING, M_GetText("Sector %s has reserved tag %d, which is not supported in UDMF. Use arg3 of the boss mapthing instead.\n"), sizeu1(i), LE_CAPSULE0);
		if (Tag_Find(&wsectors[i].tags, LE_CAPSULE1))
			CONS_Alert(CONS_WARNING, M_GetText("Sector %s has reserved tag %d, which is not supported in UDMF. Use arg3 of the boss mapthing instead.\n"), sizeu1(i), LE_CAPSULE1);
		if (Tag_Find(&wsectors[i].tags, LE_CAPSULE2))
			CONS_Alert(CONS_WARNING, M_GetText("Sector %s has reserved tag %d, which is not supported in UDMF. Use arg3 of the boss mapthing instead.\n"), sizeu1(i), LE_CAPSULE2);

		switch (GETSECSPECIAL(wsectors[i].special, 1))
		{
			case 9:
			case 10:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has ring drainer effect, which is not supported in UDMF. Use linedef type 462 instead.\n"), sizeu1(i));
				break;
			case 15:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has bouncy FOF effect, which is not supported in UDMF. Use linedef type 76 instead.\n"), sizeu1(i));
				break;
			default:
				break;
		}

		switch (GETSECSPECIAL(wsectors[i].special, 2))
		{
			case 6:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has emerald check trigger type, which is not supported in UDMF. Use linedef types 337-339 instead.\n"), sizeu1(i));
				break;
			case 7:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has NiGHTS mare trigger type, which is not supported in UDMF. Use linedef types 340-342 instead.\n"), sizeu1(i));
				break;
			case 9:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has Egg Capsule type, which is not supported in UDMF. Use linedef type 464 instead.\n"), sizeu1(i));
				break;
			case 10:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has special stage time/spheres requirements effect, which is not supported in UDMF. Use the SpecialStageTime and SpecialStageSpheres level header options instead.\n"), sizeu1(i));
				break;
			case 11:
				CONS_Alert(CONS_WARNING, M_GetText("Sector %s has custom global gravity effect, which is not supported in UDMF. Use the Gravity level header option instead.\n"), sizeu1(i));
				break;
			default:
				break;
		}
	}

	fprintf(f, "namespace = \"srb2\";\n");
	for (i = 0; i < nummapthings; i++)
	{
		fprintf(f, "thing // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		firsttag = Tag_FGet(&wmapthings[i].tags);
		if (firsttag != 0)
			fprintf(f, "id = %d;\n", firsttag);
		if (wmapthings[i].tags.count > 1)
		{
			fprintf(f, "moreids = \"");
			for (j = 1; j < wmapthings[i].tags.count; j++)
			{
				if (j > 1)
					fprintf(f, " ");
				fprintf(f, "%d", wmapthings[i].tags.tags[j]);
			}
			fprintf(f, "\";\n");
		}
		fprintf(f, "x = %d;\n", wmapthings[i].x);
		fprintf(f, "y = %d;\n", wmapthings[i].y);
		if (wmapthings[i].z != 0)
			fprintf(f, "height = %d;\n", wmapthings[i].z);
		fprintf(f, "angle = %d;\n", wmapthings[i].angle);
		if (wmapthings[i].pitch != 0)
			fprintf(f, "pitch = %d;\n", wmapthings[i].pitch);
		if (wmapthings[i].roll != 0)
			fprintf(f, "roll = %d;\n", wmapthings[i].roll);
		if (wmapthings[i].type != 0)
			fprintf(f, "type = %d;\n", wmapthings[i].type);
		if (wmapthings[i].scale != FRACUNIT)
			fprintf(f, "scale = %f;\n", FIXED_TO_FLOAT(wmapthings[i].scale));
		if (wmapthings[i].options & MTF_OBJECTFLIP)
			fprintf(f, "flip = true;\n");
		for (j = 0; j < NUMMAPTHINGARGS; j++)
			if (wmapthings[i].args[j] != 0)
				fprintf(f, "arg%s = %d;\n", sizeu1(j), wmapthings[i].args[j]);
		for (j = 0; j < NUMMAPTHINGSTRINGARGS; j++)
			if (mapthings[i].stringargs[j])
				fprintf(f, "stringarg%s = \"%s\";\n", sizeu1(j), mapthings[i].stringargs[j]);
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	for (i = 0; i < numvertexes; i++)
	{
		fprintf(f, "vertex // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "x = %f;\n", FIXED_TO_FLOAT(wvertexes[i].x));
		fprintf(f, "y = %f;\n", FIXED_TO_FLOAT(wvertexes[i].y));
		if (wvertexes[i].floorzset)
			fprintf(f, "zfloor = %f;\n", FIXED_TO_FLOAT(wvertexes[i].floorz));
		if (wvertexes[i].ceilingzset)
			fprintf(f, "zceiling = %f;\n", FIXED_TO_FLOAT(wvertexes[i].ceilingz));
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	for (i = 0; i < numlines; i++)
	{
		fprintf(f, "linedef // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "v1 = %s;\n", sizeu1(wlines[i].v1 - vertexes));
		fprintf(f, "v2 = %s;\n", sizeu1(wlines[i].v2 - vertexes));
		fprintf(f, "sidefront = %d;\n", wlines[i].sidenum[0]);
		if (wlines[i].sidenum[1] != 0xffff)
			fprintf(f, "sideback = %d;\n", wlines[i].sidenum[1]);
		firsttag = Tag_FGet(&wlines[i].tags);
		if (firsttag != 0)
			fprintf(f, "id = %d;\n", firsttag);
		if (wlines[i].tags.count > 1)
		{
			fprintf(f, "moreids = \"");
			for (j = 1; j < wlines[i].tags.count; j++)
			{
				if (j > 1)
					fprintf(f, " ");
				fprintf(f, "%d", wlines[i].tags.tags[j]);
			}
			fprintf(f, "\";\n");
		}
		if (wlines[i].special != 0)
			fprintf(f, "special = %d;\n", wlines[i].special);
		for (j = 0; j < NUMLINEARGS; j++)
			if (wlines[i].args[j] != 0)
				fprintf(f, "arg%s = %d;\n", sizeu1(j), wlines[i].args[j]);
		for (j = 0; j < NUMLINESTRINGARGS; j++)
			if (lines[i].stringargs[j])
				fprintf(f, "stringarg%s = \"%s\";\n", sizeu1(j), lines[i].stringargs[j]);
		if (wlines[i].alpha != FRACUNIT)
			fprintf(f, "alpha = %f;\n", FIXED_TO_FLOAT(wlines[i].alpha));
		if (wlines[i].blendmode != AST_COPY)
		{
			switch (wlines[i].blendmode)
			{
				case AST_ADD:
					fprintf(f, "renderstyle = \"add\";\n");
					break;
				case AST_SUBTRACT:
					fprintf(f, "renderstyle = \"subtract\";\n");
					break;
				case AST_REVERSESUBTRACT:
					fprintf(f, "renderstyle = \"reversesubtract\";\n");
					break;
				case AST_MODULATE:
					fprintf(f, "renderstyle = \"modulate\";\n");
					break;
				case AST_FOG:
					fprintf(f, "renderstyle = \"fog\";\n");
					break;
				default:
					break;
			}
		}
		if (wlines[i].executordelay != 0 && wlines[i].backsector)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Linedef %s has an executor delay. Changes to the delay at runtime will not be reflected in the converted map. Use linedef type 465 for this.\n"), sizeu1(i));
			fprintf(f, "executordelay = %d;\n", (wlines[i].backsector->ceilingheight >> FRACBITS) + (wlines[i].backsector->floorheight >> FRACBITS));
		}
		if (wlines[i].flags & ML_IMPASSIBLE)
			fprintf(f, "blocking = true;\n");
		if (wlines[i].flags & ML_BLOCKMONSTERS)
			fprintf(f, "blockmonsters = true;\n");
		if (wlines[i].flags & ML_TWOSIDED)
			fprintf(f, "twosided = true;\n");
		if (wlines[i].flags & ML_DONTPEGTOP)
			fprintf(f, "dontpegtop = true;\n");
		if (wlines[i].flags & ML_DONTPEGBOTTOM)
			fprintf(f, "dontpegbottom = true;\n");
		if (wlines[i].flags & ML_SKEWTD)
			fprintf(f, "skewtd = true;\n");
		if (wlines[i].flags & ML_NOCLIMB)
			fprintf(f, "noclimb = true;\n");
		if (wlines[i].flags & ML_NOSKEW)
			fprintf(f, "noskew = true;\n");
		if (wlines[i].flags & ML_MIDPEG)
			fprintf(f, "midpeg = true;\n");
		if (wlines[i].flags & ML_MIDSOLID)
			fprintf(f, "midsolid = true;\n");
		if (wlines[i].flags & ML_WRAPMIDTEX)
			fprintf(f, "wrapmidtex = true;\n");
		if (wlines[i].flags & ML_NONET)
			fprintf(f, "nonet = true;\n");
		if (wlines[i].flags & ML_NETONLY)
			fprintf(f, "netonly = true;\n");
		if (wlines[i].flags & ML_BOUNCY)
			fprintf(f, "bouncy = true;\n");
		if (wlines[i].flags & ML_TFERLINE)
			fprintf(f, "transfer = true;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	for (i = 0; i < numsides; i++)
	{
		fprintf(f, "sidedef // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "sector = %s;\n", sizeu1(wsides[i].sector - sectors));
		if (wsides[i].textureoffset != 0)
			fprintf(f, "offsetx = %d;\n", wsides[i].textureoffset >> FRACBITS);
		if (wsides[i].rowoffset != 0)
			fprintf(f, "offsety = %d;\n", wsides[i].rowoffset >> FRACBITS);
		if (wsides[i].toptexture > 0 && wsides[i].toptexture < numtextures)
			fprintf(f, "texturetop = \"%.*s\";\n", 8, textures[wsides[i].toptexture]->name);
		if (wsides[i].bottomtexture > 0 && wsides[i].bottomtexture < numtextures)
			fprintf(f, "texturebottom = \"%.*s\";\n", 8, textures[wsides[i].bottomtexture]->name);
		if (wsides[i].midtexture > 0 && wsides[i].midtexture < numtextures)
			fprintf(f, "texturemiddle = \"%.*s\";\n", 8, textures[wsides[i].midtexture]->name);
		if (wsides[i].repeatcnt != 0)
			fprintf(f, "repeatcnt = %d;\n", wsides[i].repeatcnt);
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	for (i = 0; i < numsectors; i++)
	{
		fprintf(f, "sector // %s\n", sizeu1(i));
		fprintf(f, "{\n");
		fprintf(f, "heightfloor = %d;\n", wsectors[i].floorheight >> FRACBITS);
		fprintf(f, "heightceiling = %d;\n", wsectors[i].ceilingheight >> FRACBITS);
		if (wsectors[i].floorpic != -1)
			fprintf(f, "texturefloor = \"%s\";\n", levelflats[wsectors[i].floorpic].name);
		if (wsectors[i].ceilingpic != -1)
			fprintf(f, "textureceiling = \"%s\";\n", levelflats[wsectors[i].ceilingpic].name);
		fprintf(f, "lightlevel = %d;\n", wsectors[i].lightlevel);
		if (wsectors[i].floorlightlevel != 0)
			fprintf(f, "lightfloor = %d;\n", wsectors[i].floorlightlevel);
		if (wsectors[i].floorlightabsolute)
			fprintf(f, "lightfloorabsolute = true;\n");
		if (wsectors[i].ceilinglightlevel != 0)
			fprintf(f, "lightceiling = %d;\n", wsectors[i].ceilinglightlevel);
		if (wsectors[i].ceilinglightabsolute)
			fprintf(f, "lightceilingabsolute = true;\n");
		firsttag = Tag_FGet(&wsectors[i].tags);
		if (firsttag != 0)
			fprintf(f, "id = %d;\n", firsttag);
		if (wsectors[i].tags.count > 1)
		{
			fprintf(f, "moreids = \"");
			for (j = 1; j < wsectors[i].tags.count; j++)
			{
				if (j > 1)
					fprintf(f, " ");
				fprintf(f, "%d", wsectors[i].tags.tags[j]);
			}
			fprintf(f, "\";\n");
		}
		sector_t tempsec = wsectors[i];
		TextmapUnfixFlatOffsets(&tempsec);
		if (tempsec.floor_xoffs != 0)
			fprintf(f, "xpanningfloor = %f;\n", FIXED_TO_FLOAT(tempsec.floor_xoffs));
		if (tempsec.floor_yoffs != 0)
			fprintf(f, "ypanningfloor = %f;\n", FIXED_TO_FLOAT(tempsec.floor_yoffs));
		if (tempsec.ceiling_xoffs != 0)
			fprintf(f, "xpanningceiling = %f;\n", FIXED_TO_FLOAT(tempsec.ceiling_xoffs));
		if (tempsec.ceiling_yoffs != 0)
			fprintf(f, "ypanningceiling = %f;\n", FIXED_TO_FLOAT(tempsec.ceiling_yoffs));
		if (wsectors[i].floorpic_angle != 0)
			fprintf(f, "rotationfloor = %f;\n", FIXED_TO_FLOAT(AngleFixed(wsectors[i].floorpic_angle)));
		if (wsectors[i].ceilingpic_angle != 0)
			fprintf(f, "rotationceiling = %f;\n", FIXED_TO_FLOAT(AngleFixed(wsectors[i].ceilingpic_angle)));
        if (wsectors[i].extra_colormap)
		{
			INT32 lightcolor = P_RGBAToColor(wsectors[i].extra_colormap->rgba);
			UINT8 lightalpha = R_GetRgbaA(wsectors[i].extra_colormap->rgba);
			INT32 fadecolor = P_RGBAToColor(wsectors[i].extra_colormap->fadergba);
			UINT8 fadealpha = R_GetRgbaA(wsectors[i].extra_colormap->fadergba);

			if (lightcolor != 0)
				fprintf(f, "lightcolor = %d;\n", lightcolor);
			if (lightalpha != 25)
				fprintf(f, "lightalpha = %d;\n", lightalpha);
			if (fadecolor != 0)
				fprintf(f, "fadecolor = %d;\n", fadecolor);
			if (fadealpha != 25)
				fprintf(f, "fadealpha = %d;\n", fadealpha);
			if (wsectors[i].extra_colormap->fadestart != 0)
				fprintf(f, "fadestart = %d;\n", wsectors[i].extra_colormap->fadestart);
			if (wsectors[i].extra_colormap->fadeend != 31)
				fprintf(f, "fadeend = %d;\n", wsectors[i].extra_colormap->fadeend);
			if (wsectors[i].extra_colormap->flags & CMF_FOG)
				fprintf(f, "colormapfog = true;\n");
			if (wsectors[i].extra_colormap->flags & CMF_FADEFULLBRIGHTSPRITES)
				fprintf(f, "colormapfadesprites = true;\n");
		}
		if (wsectors[i].colormap_protected)
			fprintf(f, "colormapprotected = true;\n");
		if (!(wsectors[i].flags & MSF_FLIPSPECIAL_FLOOR))
			fprintf(f, "flipspecial_nofloor = true;\n");
		if (wsectors[i].flags & MSF_FLIPSPECIAL_CEILING)
			fprintf(f, "flipspecial_ceiling = true;\n");
		if (wsectors[i].flags & MSF_TRIGGERSPECIAL_TOUCH)
			fprintf(f, "triggerspecial_touch = true;\n");
		if (wsectors[i].flags & MSF_TRIGGERSPECIAL_HEADBUMP)
			fprintf(f, "triggerspecial_headbump = true;\n");
		if (wsectors[i].flags & MSF_TRIGGERLINE_PLANE)
			fprintf(f, "triggerline_plane = true;\n");
		if (wsectors[i].flags & MSF_TRIGGERLINE_MOBJ)
			fprintf(f, "triggerline_mobj = true;\n");
		if (wsectors[i].flags & MSF_INVERTPRECIP)
			fprintf(f, "invertprecip = true;\n");
		if (wsectors[i].flags & MSF_GRAVITYFLIP)
			fprintf(f, "gravityflip = true;\n");
		if (wsectors[i].flags & MSF_HEATWAVE)
			fprintf(f, "heatwave = true;\n");
		if (wsectors[i].flags & MSF_NOCLIPCAMERA)
			fprintf(f, "noclipcamera = true;\n");
		if (wsectors[i].specialflags & SSF_OUTERSPACE)
			fprintf(f, "outerspace = true;\n");
		if (wsectors[i].specialflags & SSF_DOUBLESTEPUP)
			fprintf(f, "doublestepup = true;\n");
		if (wsectors[i].specialflags & SSF_NOSTEPDOWN)
			fprintf(f, "nostepdown = true;\n");
		if (wsectors[i].specialflags & SSF_SPEEDPAD)
			fprintf(f, "speedpad = true;\n");
		if (wsectors[i].specialflags & SSF_STARPOSTACTIVATOR)
			fprintf(f, "starpostactivator = true;\n");
		if (wsectors[i].specialflags & SSF_EXIT)
			fprintf(f, "exit = true;\n");
		if (wsectors[i].specialflags & SSF_SPECIALSTAGEPIT)
			fprintf(f, "specialstagepit = true;\n");
		if (wsectors[i].specialflags & SSF_RETURNFLAG)
			fprintf(f, "returnflag = true;\n");
		if (wsectors[i].specialflags & SSF_REDTEAMBASE)
			fprintf(f, "redteambase = true;\n");
		if (wsectors[i].specialflags & SSF_BLUETEAMBASE)
			fprintf(f, "blueteambase = true;\n");
		if (wsectors[i].specialflags & SSF_FAN)
			fprintf(f, "fan = true;\n");
		if (wsectors[i].specialflags & SSF_SUPERTRANSFORM)
			fprintf(f, "supertransform = true;\n");
		if (wsectors[i].specialflags & SSF_FORCESPIN)
			fprintf(f, "forcespin = true;\n");
		if (wsectors[i].specialflags & SSF_ZOOMTUBESTART)
			fprintf(f, "zoomtubestart = true;\n");
		if (wsectors[i].specialflags & SSF_ZOOMTUBEEND)
			fprintf(f, "zoomtubeend = true;\n");
		if (wsectors[i].specialflags & SSF_FINISHLINE)
			fprintf(f, "finishline = true;\n");
		if (wsectors[i].specialflags & SSF_ROPEHANG)
			fprintf(f, "ropehang = true;\n");
		if (wsectors[i].specialflags & SSF_JUMPFLIP)
			fprintf(f, "jumpflip = true;\n");
		if (wsectors[i].specialflags & SSF_GRAVITYOVERRIDE)
			fprintf(f, "gravityoverride = true;\n");
		if (wsectors[i].friction != ORIG_FRICTION)
			fprintf(f, "friction = %f;\n", FIXED_TO_FLOAT(wsectors[i].friction));
		if (wsectors[i].gravity != FRACUNIT)
			fprintf(f, "gravity = %f;\n", FIXED_TO_FLOAT(wsectors[i].gravity));
		if (wsectors[i].damagetype != SD_NONE)
		{
			switch (wsectors[i].damagetype)
			{
				case SD_GENERIC:
					fprintf(f, "damagetype = \"Generic\";\n");
					break;
				case SD_WATER:
					fprintf(f, "damagetype = \"Water\";\n");
					break;
				case SD_FIRE:
					fprintf(f, "damagetype = \"Fire\";\n");
					break;
				case SD_LAVA:
					fprintf(f, "damagetype = \"Lava\";\n");
					break;
				case SD_ELECTRIC:
					fprintf(f, "damagetype = \"Electric\";\n");
					break;
				case SD_SPIKE:
					fprintf(f, "damagetype = \"Spike\";\n");
					break;
				case SD_DEATHPITTILT:
					fprintf(f, "damagetype = \"DeathPitTilt\";\n");
					break;
				case SD_DEATHPITNOTILT:
					fprintf(f, "damagetype = \"DeathPitNoTilt\";\n");
					break;
				case SD_INSTAKILL:
					fprintf(f, "damagetype = \"Instakill\";\n");
					break;
				case SD_SPECIALSTAGE:
					fprintf(f, "damagetype = \"SpecialStage\";\n");
					break;
				default:
					break;
			}
		}
		if (wsectors[i].triggertag != 0)
			fprintf(f, "triggertag = %d;\n", wsectors[i].triggertag);
		if (wsectors[i].triggerer != 0)
		{
			switch (wsectors[i].triggerer)
			{
				case TO_PLAYER:
					fprintf(f, "triggerer = \"Player\";\n");
					break;
				case TO_ALLPLAYERS:
					fprintf(f, "triggerer = \"AllPlayers\";\n");
					break;
				case TO_MOBJ:
					fprintf(f, "triggerer = \"Mobj\";\n");
					break;
				default:
					break;
			}
		}
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	fclose(f);

	for (i = 0; i < nummapthings; i++)
		if (wmapthings[i].tags.count)
			Z_Free(wmapthings[i].tags.tags);

	for (i = 0; i < numsectors; i++)
		if (wsectors[i].tags.count)
			Z_Free(wsectors[i].tags.tags);

	for (i = 0; i < numlines; i++)
		if (wlines[i].tags.count)
			Z_Free(wlines[i].tags.tags);

	Z_Free(wmapthings);
	Z_Free(wvertexes);
	Z_Free(wsectors);
	Z_Free(wlines);
	Z_Free(wsides);
	Z_Free(specialthings);
}

/** Loads the textmap data, after obtaining the elements count and allocating their respective space.
  */
static void P_LoadTextmap(void)
{
	UINT32 i;

	vertex_t   *vt;
	sector_t   *sc;
	line_t     *ld;
	side_t     *sd;
	mapthing_t *mt;

	CONS_Alert(CONS_NOTICE, "UDMF support is still a work-in-progress; its specs and features are prone to change until it is fully implemented.\n");

	/// Given the UDMF specs, some fields are given a default value.
	/// If an element's field has a default value set, it is omitted
	/// from the textmap, and therefore we have to account for it by
	/// preemptively setting that value beforehand.

	for (i = 0, vt = vertexes; i < numvertexes; i++, vt++)
	{
		// Defaults.
		vt->x = vt->y = INT32_MAX;
		vt->floorzset = vt->ceilingzset = false;
		vt->floorz = vt->ceilingz = 0;

		TextmapParse(vertexesPos[i], i, ParseTextmapVertexParameter);

		if (vt->x == INT32_MAX)
			I_Error("P_LoadTextmap: vertex %s has no x value set!\n", sizeu1(i));
		if (vt->y == INT32_MAX)
			I_Error("P_LoadTextmap: vertex %s has no y value set!\n", sizeu1(i));
	}

	for (i = 0, sc = sectors; i < numsectors; i++, sc++)
	{
		// Defaults.
		sc->floorheight = 0;
		sc->ceilingheight = 0;

		sc->floorpic = 0;
		sc->ceilingpic = 0;

		sc->lightlevel = 255;

		sc->special = 0;
		Tag_FSet(&sc->tags, 0);

		sc->floor_xoffs = sc->floor_yoffs = 0;
		sc->ceiling_xoffs = sc->ceiling_yoffs = 0;

		sc->floorpic_angle = sc->ceilingpic_angle = 0;

		sc->floorlightlevel = sc->ceilinglightlevel = 0;
		sc->floorlightabsolute = sc->ceilinglightabsolute = false;

		sc->colormap_protected = false;

		sc->gravity = FRACUNIT;

		sc->flags = MSF_FLIPSPECIAL_FLOOR;
		sc->specialflags = 0;
		sc->damagetype = SD_NONE;
		sc->triggertag = 0;
		sc->triggerer = TO_PLAYER;

		sc->friction = ORIG_FRICTION;

		textmap_colormap.used = false;
		textmap_colormap.lightcolor = 0;
		textmap_colormap.lightalpha = 25;
		textmap_colormap.fadecolor = 0;
		textmap_colormap.fadealpha = 25;
		textmap_colormap.fadestart = 0;
		textmap_colormap.fadeend = 31;
		textmap_colormap.flags = 0;

		textmap_planefloor.defined = 0;
		textmap_planeceiling.defined = 0;

		TextmapParse(sectorsPos[i], i, ParseTextmapSectorParameter);

		P_InitializeSector(sc);
		if (textmap_colormap.used)
		{
			INT32 rgba = P_ColorToRGBA(textmap_colormap.lightcolor, textmap_colormap.lightalpha);
			INT32 fadergba = P_ColorToRGBA(textmap_colormap.fadecolor, textmap_colormap.fadealpha);
			sc->extra_colormap = sc->spawn_extra_colormap = R_CreateColormap(rgba, fadergba, textmap_colormap.fadestart, textmap_colormap.fadeend, textmap_colormap.flags);
		}

		if (textmap_planefloor.defined == (PD_A|PD_B|PD_C|PD_D))
        {
			sc->f_slope = MakeViaEquationConstants(textmap_planefloor.a, textmap_planefloor.b, textmap_planefloor.c, textmap_planefloor.d);
			sc->hasslope = true;
        }

		if (textmap_planeceiling.defined == (PD_A|PD_B|PD_C|PD_D))
        {
			sc->c_slope = MakeViaEquationConstants(textmap_planeceiling.a, textmap_planeceiling.b, textmap_planeceiling.c, textmap_planeceiling.d);
			sc->hasslope = true;
        }

		TextmapFixFlatOffsets(sc);
	}

	for (i = 0, ld = lines; i < numlines; i++, ld++)
	{
		// Defaults.
		ld->v1 = ld->v2 = NULL;
		ld->flags = 0;
		ld->special = 0;
		Tag_FSet(&ld->tags, 0);

		memset(ld->args, 0, NUMLINEARGS*sizeof(*ld->args));
		memset(ld->stringargs, 0x00, NUMLINESTRINGARGS*sizeof(*ld->stringargs));
		ld->alpha = FRACUNIT;
		ld->executordelay = 0;
		ld->sidenum[0] = 0xffff;
		ld->sidenum[1] = 0xffff;

		TextmapParse(linesPos[i], i, ParseTextmapLinedefParameter);

		if (!ld->v1)
			I_Error("P_LoadTextmap: linedef %s has no v1 value set!\n", sizeu1(i));
		if (!ld->v2)
			I_Error("P_LoadTextmap: linedef %s has no v2 value set!\n", sizeu1(i));
		if (ld->sidenum[0] == 0xffff)
			I_Error("P_LoadTextmap: linedef %s has no sidefront value set!\n", sizeu1(i));

		P_InitializeLinedef(ld);
	}

	for (i = 0, sd = sides; i < numsides; i++, sd++)
	{
		// Defaults.
		sd->textureoffset = 0;
		sd->rowoffset = 0;
		sd->toptexture = R_TextureNumForName("-");
		sd->midtexture = R_TextureNumForName("-");
		sd->bottomtexture = R_TextureNumForName("-");
		sd->sector = NULL;
		sd->repeatcnt = 0;

		TextmapParse(sidesPos[i], i, ParseTextmapSidedefParameter);

		if (!sd->sector)
			I_Error("P_LoadTextmap: sidedef %s has no sector value set!\n", sizeu1(i));

		P_InitializeSidedef(sd);
	}

	for (i = 0, mt = mapthings; i < nummapthings; i++, mt++)
	{
		// Defaults.
		mt->x = mt->y = 0;
		mt->angle = mt->pitch = mt->roll = 0;
		mt->type = 0;
		mt->options = 0;
		mt->z = 0;
		mt->extrainfo = 0;
		Tag_FSet(&mt->tags, 0);
		mt->scale = FRACUNIT;
		memset(mt->args, 0, NUMMAPTHINGARGS*sizeof(*mt->args));
		memset(mt->stringargs, 0x00, NUMMAPTHINGSTRINGARGS*sizeof(*mt->stringargs));
		mt->mobj = NULL;

		TextmapParse(mapthingsPos[i], i, ParseTextmapThingParameter);
	}
}

static void P_ProcessLinedefsAfterSidedefs(void)
{
	size_t i = numlines;
	register line_t *ld = lines;
	for (; i--; ld++)
	{
		ld->frontsector = sides[ld->sidenum[0]].sector; //e6y: Can't be -1 here
		ld->backsector = ld->sidenum[1] != 0xffff ? sides[ld->sidenum[1]].sector : 0;

		if (udmf)
			continue;

		switch (ld->special)
		{
		// Compile linedef 'text' from both sidedefs 'text' for appropriate specials.
		case 331: // Trigger linedef executor: Skin - Continuous
		case 332: // Trigger linedef executor: Skin - Each time
		case 333: // Trigger linedef executor: Skin - Once
		case 443: // Calls a named Lua function
			if (sides[ld->sidenum[0]].text)
			{
				size_t len = strlen(sides[ld->sidenum[0]].text) + 1;
				if (ld->sidenum[1] != 0xffff && sides[ld->sidenum[1]].text)
					len += strlen(sides[ld->sidenum[1]].text);
				ld->text = Z_Malloc(len, PU_LEVEL, NULL);
				M_Memcpy(ld->text, sides[ld->sidenum[0]].text, strlen(sides[ld->sidenum[0]].text) + 1);
				if (ld->sidenum[1] != 0xffff && sides[ld->sidenum[1]].text)
					M_Memcpy(ld->text + strlen(ld->text) + 1, sides[ld->sidenum[1]].text, strlen(sides[ld->sidenum[1]].text) + 1);
			}
			break;
		case 447: // Change colormap
		case 455: // Fade colormap
			if (ld->flags & ML_DONTPEGBOTTOM) // alternate alpha (by texture offsets)
			{
				extracolormap_t *exc = R_CopyColormap(sides[ld->sidenum[0]].colormap_data, false);
				INT16 alpha = max(min(sides[ld->sidenum[0]].textureoffset >> FRACBITS, 25), -25);
				INT16 fadealpha = max(min(sides[ld->sidenum[0]].rowoffset >> FRACBITS, 25), -25);

				// If alpha is negative, set "subtract alpha" flag and store absolute value
				if (alpha < 0)
				{
					alpha *= -1;
					ld->args[2] |= TMCF_SUBLIGHTA;
				}
				if (fadealpha < 0)
				{
					fadealpha *= -1;
					ld->args[2] |= TMCF_SUBFADEA;
				}

				exc->rgba = R_GetRgbaRGB(exc->rgba) + R_PutRgbaA(alpha);
				exc->fadergba = R_GetRgbaRGB(exc->fadergba) + R_PutRgbaA(fadealpha);

				if (!(sides[ld->sidenum[0]].colormap_data = R_GetColormapFromList(exc)))
				{
					exc->colormap = R_CreateLightTable(exc);
					R_AddColormapToList(exc);
					sides[ld->sidenum[0]].colormap_data = exc;
				}
				else
					Z_Free(exc);
			}
			break;
		}
	}
}

static boolean P_LoadMapData(const virtres_t *virt)
{
	virtlump_t *virtvertexes = NULL, *virtsectors = NULL, *virtsidedefs = NULL, *virtlinedefs = NULL, *virtthings = NULL;

	// Count map data.
	if (udmf) // Count how many entries for each type we got in textmap.
	{
		virtlump_t *textmap = vres_Find(virt, "TEXTMAP");
		M_TokenizerOpen((char *)textmap->data);
		if (!TextmapCount(textmap->size))
		{
			M_TokenizerClose();
			return false;
		}
	}
	else
	{
		virtthings   = vres_Find(virt, "THINGS");
		virtvertexes = vres_Find(virt, "VERTEXES");
		virtsectors  = vres_Find(virt, "SECTORS");
		virtsidedefs = vres_Find(virt, "SIDEDEFS");
		virtlinedefs = vres_Find(virt, "LINEDEFS");

		if (!virtthings)
			I_Error("THINGS lump not found");
		if (!virtvertexes)
			I_Error("VERTEXES lump not found");
		if (!virtsectors)
			I_Error("SECTORS lump not found");
		if (!virtsidedefs)
			I_Error("SIDEDEFS lump not found");
		if (!virtlinedefs)
			I_Error("LINEDEFS lump not found");

		// Traditional doom map format just assumes the number of elements from the lump sizes.
		numvertexes  = virtvertexes->size / sizeof (mapvertex_t);
		numsectors   = virtsectors->size  / sizeof (mapsector_t);
		numsides     = virtsidedefs->size / sizeof (mapsidedef_t);
		numlines     = virtlinedefs->size / sizeof (maplinedef_t);
		nummapthings = virtthings->size   / (5 * sizeof (INT16));
	}

	if (numvertexes <= 0)
		I_Error("Level has no vertices");
	if (numsectors <= 0)
		I_Error("Level has no sectors");
	if (numsides <= 0)
		I_Error("Level has no sidedefs");
	if (numlines <= 0)
		I_Error("Level has no linedefs");

	vertexes  = Z_Calloc(numvertexes * sizeof (*vertexes), PU_LEVEL, NULL);
	sectors   = Z_Calloc(numsectors * sizeof (*sectors), PU_LEVEL, NULL);
	sides     = Z_Calloc(numsides * sizeof (*sides), PU_LEVEL, NULL);
	lines     = Z_Calloc(numlines * sizeof (*lines), PU_LEVEL, NULL);
	mapthings = Z_Calloc(nummapthings * sizeof (*mapthings), PU_LEVEL, NULL);

	// Allocate a big chunk of memory as big as our MAXLEVELFLATS limit.
	//Fab : FIXME: allocate for whatever number of flats - 512 different flats per level should be plenty
	foundflats = calloc(MAXLEVELFLATS, sizeof (*foundflats));
	if (foundflats == NULL)
		I_Error("Ran out of memory while loading sectors\n");

	numlevelflats = 0;

	// Load map data.
	if (udmf)
	{
		P_LoadTextmap();
		M_TokenizerClose();
	}
	else
	{
		P_LoadVertices(virtvertexes->data);
		P_LoadSectors(virtsectors->data);
		P_LoadLinedefs(virtlinedefs->data);
		P_LoadSidedefs(virtsidedefs->data);
		P_LoadThings(virtthings->data);
	}

	P_ProcessLinedefsAfterSidedefs();

	R_ClearTextureNumCache(true);

	// set the sky flat num
	skyflatnum = P_AddLevelFlat(SKYFLATNAME, foundflats);

	// copy table for global usage
	levelflats = M_Memcpy(Z_Calloc(numlevelflats * sizeof (*levelflats), PU_LEVEL, NULL), foundflats, numlevelflats * sizeof (levelflat_t));
	free(foundflats);

	// search for animated flats and set up
	P_SetupLevelFlatAnims();

	return true;
}

static void P_InitializeSubsector(subsector_t *ss)
{
	ss->sector = NULL;
	ss->validcount = 0;
}

static inline void P_LoadSubsectors(UINT8 *data)
{
	mapsubsector_t *ms = (mapsubsector_t*)data;
	subsector_t *ss = subsectors;
	size_t i;

	for (i = 0; i < numsubsectors; i++, ss++, ms++)
	{
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = (UINT16)SHORT(ms->firstseg);
		P_InitializeSubsector(ss);
	}
}

static void P_LoadNodes(UINT8 *data)
{
	UINT8 j, k;
	mapnode_t *mn = (mapnode_t*)data;
	node_t *no = nodes;
	size_t i;

	for (i = 0; i < numnodes; i++, no++, mn++)
	{
		no->x = SHORT(mn->x)<<FRACBITS;
		no->y = SHORT(mn->y)<<FRACBITS;
		no->dx = SHORT(mn->dx)<<FRACBITS;
		no->dy = SHORT(mn->dy)<<FRACBITS;
		for (j = 0; j < 2; j++)
		{
			no->children[j] = SHORT(mn->children[j]);
			for (k = 0; k < 4; k++)
				no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
		}
	}
}

/** Computes the length of a seg in fracunits.
  *
  * \param seg Seg to compute length for.
  * \return Length in fracunits.
  */
static fixed_t P_SegLength(seg_t *seg)
{
	INT64 dx = (seg->v2->x - seg->v1->x)>>1;
	INT64 dy = (seg->v2->y - seg->v1->y)>>1;
	return FixedHypot(dx, dy)<<1;
}

#ifdef HWRENDER
/** Computes the length of a seg as a float.
  * This is needed for OpenGL.
  *
  * \param seg Seg to compute length for.
  * \return Length as a float.
  */
static inline float P_SegLengthFloat(seg_t *seg)
{
	float dx, dy;

	// make a vector (start at origin)
	dx = FIXED_TO_FLOAT(seg->v2->x - seg->v1->x);
	dy = FIXED_TO_FLOAT(seg->v2->y - seg->v1->y);

	return (float)hypot(dx, dy);
}
#endif

static void P_InitializeSeg(seg_t *seg)
{
	if (seg->linedef)
	{
		UINT16 side = seg->linedef->sidenum[seg->side];

		if (side == 0xffff)
			I_Error("P_InitializeSeg: Seg %s refers to side %d of linedef %s, which doesn't exist!\n", sizeu1((size_t)(seg - segs)), seg->side, sizeu1((size_t)(seg->linedef - lines)));

		seg->sidedef = &sides[side];

		seg->frontsector = seg->sidedef->sector;
		seg->backsector = (seg->linedef->flags & ML_TWOSIDED) ? sides[seg->linedef->sidenum[seg->side ^ 1]].sector : NULL;
	}

#ifdef HWRENDER
	seg->pv1 = seg->pv2 = NULL;

	//Hurdler: 04/12/2000: for now, only used in hardware mode
	seg->lightmaps = NULL; // list of static lightmap for this seg
#endif

	seg->numlights = 0;
	seg->rlights = NULL;
	seg->polyseg = NULL;
	seg->dontrenderme = false;
}

static void P_LoadSegs(UINT8 *data)
{
	mapseg_t *ms = (mapseg_t*)data;
	seg_t *seg = segs;
	size_t i;

	for (i = 0; i < numsegs; i++, seg++, ms++)
	{
		seg->v1 = &vertexes[SHORT(ms->v1)];
		seg->v2 = &vertexes[SHORT(ms->v2)];

		seg->side = SHORT(ms->side);

		seg->offset = (SHORT(ms->offset)) << FRACBITS;

		seg->angle = (SHORT(ms->angle)) << FRACBITS;

		seg->linedef = &lines[SHORT(ms->linedef)];

		seg->length = P_SegLength(seg);
#ifdef HWRENDER
		seg->flength = (rendermode == render_opengl) ? P_SegLengthFloat(seg) : 0;
#endif

		seg->glseg = false;
		P_InitializeSeg(seg);
	}
}

typedef enum {
	NT_DOOM,
	NT_XNOD,
	NT_ZNOD,
	NT_XGLN,
	NT_ZGLN,
	NT_XGL2,
	NT_ZGL2,
	NT_XGL3,
	NT_ZGL3,
	NT_UNSUPPORTED,
	NUMNODETYPES
} nodetype_t;

// Find out the BSP format.
static nodetype_t P_GetNodetype(const virtres_t *virt, UINT8 **nodedata)
{
	boolean supported[NUMNODETYPES] = {0};
	nodetype_t nodetype = NT_UNSUPPORTED;
	char signature[4 + 1];

	if (udmf)
	{
		*nodedata = vres_Find(virt, "ZNODES")->data;
		supported[NT_XGLN] = supported[NT_XGL3] = true;
	}
	else
	{
		virtlump_t *virtsegs = vres_Find(virt, "SEGS");
		virtlump_t *virtssectors;

		if (virtsegs && virtsegs->size)
		{
			*nodedata = vres_Find(virt, "NODES")->data;
			return NT_DOOM; // Traditional map format BSP tree.
		}

		virtssectors = vres_Find(virt, "SSECTORS");

		if (virtssectors && virtssectors->size)
		{ // Possibly GL nodes: NODES ignored, SSECTORS takes precedence as nodes lump, (It is confusing yeah) and has a signature.
			*nodedata = virtssectors->data;
			supported[NT_XGLN] = supported[NT_ZGLN] = supported[NT_XGL3] = true;
		}
		else
		{ // Possibly ZDoom extended nodes: SSECTORS is empty, NODES has a signature.
			*nodedata = vres_Find(virt, "NODES")->data;
			supported[NT_XNOD] = supported[NT_ZNOD] = true;
		}
	}

	M_Memcpy(signature, *nodedata, 4);
	signature[4] = '\0';
	(*nodedata) += 4;

	if (!strcmp(signature, "XNOD"))
		nodetype = NT_XNOD;
	else if (!strcmp(signature, "ZNOD"))
		nodetype = NT_ZNOD;
	else if (!strcmp(signature, "XGLN"))
		nodetype = NT_XGLN;
	else if (!strcmp(signature, "ZGLN"))
		nodetype = NT_ZGLN;
	else if (!strcmp(signature, "XGL3"))
		nodetype = NT_XGL3;

	return supported[nodetype] ? nodetype : NT_UNSUPPORTED;
}

// Extended node formats feature additional vertices; useful for OpenGL, but totally useless in gamelogic.
static boolean P_LoadExtraVertices(UINT8 **data)
{
	UINT32 origvrtx = READUINT32((*data));
	UINT32 xtrvrtx = READUINT32((*data));
	line_t* ld = lines;
	vertex_t *oldpos = vertexes;
	ssize_t offset;
	size_t i;

	if (numvertexes != origvrtx) // If native vertex count doesn't match node original vertex count, bail out (broken data?).
	{
		CONS_Alert(CONS_WARNING, "Vertex count in map data and nodes differ!\n");
		return false;
	}

	if (!xtrvrtx)
		return true;

	// If extra vertexes were generated, reallocate the vertex array and fix the pointers.
	numvertexes += xtrvrtx;
	vertexes = Z_Realloc(vertexes, numvertexes*sizeof(*vertexes), PU_LEVEL, NULL);
	offset = (size_t)(vertexes - oldpos);

	for (i = 0, ld = lines; i < numlines; i++, ld++)
	{
		ld->v1 += offset;
		ld->v2 += offset;
	}

	// Read extra vertex data.
	for (i = origvrtx; i < numvertexes; i++)
	{
		vertexes[i].x = READFIXED((*data));
		vertexes[i].y = READFIXED((*data));
	}

	return true;
}

static boolean P_LoadExtendedSubsectorsAndSegs(UINT8 **data, nodetype_t nodetype)
{
	size_t i, k;
	INT16 m;
	seg_t *seg;

	// Subsectors
	numsubsectors = READUINT32((*data));
	subsectors = Z_Calloc(numsubsectors*sizeof(*subsectors), PU_LEVEL, NULL);

	for (i = 0; i < numsubsectors; i++)
		subsectors[i].numlines = READUINT32((*data));

	// Segs
	numsegs = READUINT32((*data));
	segs = Z_Calloc(numsegs*sizeof(*segs), PU_LEVEL, NULL);

	for (i = 0, k = 0; i < numsubsectors; i++)
	{
		subsectors[i].firstline = k;
		P_InitializeSubsector(&subsectors[i]);

		switch (nodetype)
		{
		case NT_XGLN:
		case NT_XGL3:
			for (m = 0; m < subsectors[i].numlines; m++, k++)
			{
				UINT32 vertexnum = READUINT32((*data));
				UINT16 linenum;

				if (vertexnum >= numvertexes)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid vertex %d!\n", sizeu1(k), m, vertexnum);

				segs[k - 1 + ((m == 0) ? subsectors[i].numlines : 0)].v2 = segs[k].v1 = &vertexes[vertexnum];

				READUINT32((*data)); // partner, can be ignored by software renderer

				linenum = (nodetype == NT_XGL3) ? READUINT32((*data)) : READUINT16((*data));
				if (linenum != 0xFFFF && linenum >= numlines)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %s has invalid linedef %d!\n", sizeu1(k), sizeu2(i), linenum);
				segs[k].glseg = (linenum == 0xFFFF);
				segs[k].linedef = (linenum == 0xFFFF) ? NULL : &lines[linenum];
				segs[k].side = READUINT8((*data));
			}
			while (segs[subsectors[i].firstline].glseg)
			{
				subsectors[i].firstline++;
				if (subsectors[i].firstline == k)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Subsector %s does not have any valid segs!", sizeu1(i));
			}
			break;

		case NT_XNOD:
			for (m = 0; m < subsectors[i].numlines; m++, k++)
			{
				UINT32 v1num = READUINT32((*data));
				UINT32 v2num = READUINT32((*data));
				UINT16 linenum = READUINT16((*data));

				if (v1num >= numvertexes)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid v1 %d!\n", sizeu1(k), m, v1num);
				if (v2num >= numvertexes)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid v2 %d!\n", sizeu1(k), m, v2num);
				if (linenum >= numlines)
					I_Error("P_LoadExtendedSubsectorsAndSegs: Seg %s in subsector %d has invalid linedef %d!\n", sizeu1(k), m, linenum);

				segs[k].v1 = &vertexes[v1num];
				segs[k].v2 = &vertexes[v2num];
				segs[k].linedef = &lines[linenum];
				segs[k].side = READUINT8((*data));
				segs[k].glseg = false;
			}
			break;

		default:
			return false;
		}
	}

	for (i = 0, seg = segs; i < numsegs; i++, seg++)
	{
		vertex_t *v1 = seg->v1;
		vertex_t *v2 = seg->v2;
		P_InitializeSeg(seg);
		seg->angle = R_PointToAngle2(v1->x, v1->y, v2->x, v2->y);
		if (seg->linedef)
		{
			vertex_t *v = (seg->side == 1) ? seg->linedef->v2 : seg->linedef->v1;
			segs[i].offset = FixedHypot(v1->x - v->x, v1->y - v->y);
		}
		seg->length = P_SegLength(seg);
#ifdef HWRENDER
		seg->flength = (rendermode == render_opengl) ? P_SegLengthFloat(seg) : 0;
#endif
	}

	return true;
}

// Auxiliary function: Shrink node ID from 32-bit to 16-bit.
static UINT16 ShrinkNodeID(UINT32 x) {
	UINT16 mask = (x >> 16) & 0xC000;
	UINT16 result = x;
	return result | mask;
}

static void P_LoadExtendedNodes(UINT8 **data, nodetype_t nodetype)
{
	node_t *mn;
	size_t i, j, k;
	boolean xgl3 = (nodetype == NT_XGL3);

	numnodes = READINT32((*data));
	nodes = Z_Calloc(numnodes*sizeof(*nodes), PU_LEVEL, NULL);

	for (i = 0, mn = nodes; i < numnodes; i++, mn++)
	{
		// Splitter
		mn->x = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);
		mn->y = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);
		mn->dx = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);
		mn->dy = xgl3 ? READINT32((*data)) : (READINT16((*data)) << FRACBITS);

		// Bounding boxes
		for (j = 0; j < 2; j++)
			for (k = 0; k < 4; k++)
				mn->bbox[j][k] = READINT16((*data)) << FRACBITS;

		//Children
		mn->children[0] = ShrinkNodeID(READUINT32((*data))); /// \todo Use UINT32 for node children in a future, instead?
		mn->children[1] = ShrinkNodeID(READUINT32((*data)));
	}
}

static void P_LoadMapBSP(const virtres_t *virt)
{
	UINT8 *nodedata = NULL;
	nodetype_t nodetype = P_GetNodetype(virt, &nodedata);

	switch (nodetype)
	{
	case NT_DOOM:
	{
		virtlump_t *virtssectors = vres_Find(virt, "SSECTORS");
		virtlump_t* virtnodes = vres_Find(virt, "NODES");
		virtlump_t *virtsegs = vres_Find(virt, "SEGS");

		numsubsectors = virtssectors->size / sizeof(mapsubsector_t);
		numnodes      = virtnodes->size    / sizeof(mapnode_t);
		numsegs       = virtsegs->size     / sizeof(mapseg_t);

		if (numsubsectors <= 0)
			I_Error("Level has no subsectors (did you forget to run it through a nodesbuilder?)");
		if (numnodes <= 0)
			I_Error("Level has no nodes (does your map have at least 2 sectors?)");
		if (numsegs <= 0)
			I_Error("Level has no segs");

		subsectors = Z_Calloc(numsubsectors * sizeof(*subsectors), PU_LEVEL, NULL);
		nodes      = Z_Calloc(numnodes * sizeof(*nodes), PU_LEVEL, NULL);
		segs       = Z_Calloc(numsegs * sizeof(*segs), PU_LEVEL, NULL);

		P_LoadSubsectors(virtssectors->data);
		P_LoadNodes(virtnodes->data);
		P_LoadSegs(virtsegs->data);
		break;
	}
	case NT_XNOD:
	case NT_XGLN:
	case NT_XGL3:
		if (!P_LoadExtraVertices(&nodedata))
			return;
		if (!P_LoadExtendedSubsectorsAndSegs(&nodedata, nodetype))
			return;
		P_LoadExtendedNodes(&nodedata, nodetype);
		break;
	default:
		CONS_Alert(CONS_WARNING, "Unsupported BSP format detected.\n");
		return;
	}
	return;
}

// Split from P_LoadBlockMap for convenience
// -- Monster Iestyn 08/01/18
static void P_ReadBlockMapLump(INT16 *wadblockmaplump, size_t count)
{
	size_t i;
	blockmaplump = Z_Calloc(sizeof (*blockmaplump) * count, PU_LEVEL, NULL);

	// killough 3/1/98: Expand wad blockmap into larger internal one,
	// by treating all offsets except -1 as unsigned and zero-extending
	// them. This potentially doubles the size of blockmaps allowed,
	// because Doom originally considered the offsets as always signed.

	blockmaplump[0] = SHORT(wadblockmaplump[0]);
	blockmaplump[1] = SHORT(wadblockmaplump[1]);
	blockmaplump[2] = (INT32)(SHORT(wadblockmaplump[2])) & 0xffff;
	blockmaplump[3] = (INT32)(SHORT(wadblockmaplump[3])) & 0xffff;

	for (i = 4; i < count; i++)
	{
		INT16 t = SHORT(wadblockmaplump[i]);          // killough 3/1/98
		blockmaplump[i] = t == -1 ? (INT32)-1 : (INT32) t & 0xffff;
	}
}

// This needs to be a separate function
// because making both the WAD and PK3 loading code use
// the same functions is trickier than it looks for blockmap
// -- Monster Iestyn 09/01/18
static boolean P_LoadBlockMap(UINT8 *data, size_t count)
{
	if (!count || count >= 0x20000)
		return false;

	//CONS_Printf("Reading blockmap lump for pk3...\n");

	// no need to malloc anything, assume the data is uncompressed for now
	count /= 2;
	P_ReadBlockMapLump((INT16 *)data, count);

	bmaporgx = blockmaplump[0]<<FRACBITS;
	bmaporgy = blockmaplump[1]<<FRACBITS;
	bmapwidth = blockmaplump[2];
	bmapheight = blockmaplump[3];

	// clear out mobj chains
	count = sizeof (*blocklinks)* bmapwidth*bmapheight;
	blocklinks = Z_Calloc(count, PU_LEVEL, NULL);
	blockmap = blockmaplump+4;

	// haleyjd 2/22/06: setup polyobject blockmap
	count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
	polyblocklinks = Z_Calloc(count, PU_LEVEL, NULL);
	return true;
}

static boolean LineInBlock(fixed_t cx1, fixed_t cy1, fixed_t cx2, fixed_t cy2, fixed_t bx1, fixed_t by1)
{
	fixed_t bbox[4];
	line_t testline;
	vertex_t vtest;

	bbox[BOXRIGHT] = bx1 + MAPBLOCKUNITS;
	bbox[BOXTOP] = by1 + MAPBLOCKUNITS;
	bbox[BOXLEFT] = bx1;
	bbox[BOXBOTTOM] = by1;

	// Trivial rejection
	if (cx1 < bbox[BOXLEFT] && cx2 < bbox[BOXLEFT])
		return false;

	if (cx1 > bbox[BOXRIGHT] && cx2 > bbox[BOXRIGHT])
		return false;

	if (cy1 < bbox[BOXBOTTOM] && cy2 < bbox[BOXBOTTOM])
		return false;

	if (cy1 > bbox[BOXTOP] && cy2 > bbox[BOXTOP])
		return false;

	// Rats, guess we gotta check
	// if the line intersects
	// any sides of the block.
	cx1 <<= FRACBITS;
	cy1 <<= FRACBITS;
	cx2 <<= FRACBITS;
	cy2 <<= FRACBITS;
	bbox[BOXTOP] <<= FRACBITS;
	bbox[BOXBOTTOM] <<= FRACBITS;
	bbox[BOXLEFT] <<= FRACBITS;
	bbox[BOXRIGHT] <<= FRACBITS;

	testline.v1 = &vtest;

	testline.v1->x = cx1;
	testline.v1->y = cy1;
	testline.dx = cx2 - cx1;
	testline.dy = cy2 - cy1;

	if ((testline.dx > 0) ^ (testline.dy > 0))
		testline.slopetype = ST_NEGATIVE;
	else
		testline.slopetype = ST_POSITIVE;

	return P_BoxOnLineSide(bbox, &testline) == -1;
}

//
// killough 10/98:
//
// Rewritten to use faster algorithm.
//
// SSN Edit: Killough's wasn't accurate enough, sometimes excluding
// blocks that the line did in fact exist in, so now we use
// a fail-safe approach that puts a 'box' around each line.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
static void P_CreateBlockMap(void)
{
	register size_t i;
	fixed_t minx = INT32_MAX, miny = INT32_MAX, maxx = INT32_MIN, maxy = INT32_MIN;
	// First find limits of map

	for (i = 0; i < numvertexes; i++)
	{
		if (vertexes[i].x>>FRACBITS < minx)
			minx = vertexes[i].x>>FRACBITS;
		else if (vertexes[i].x>>FRACBITS > maxx)
			maxx = vertexes[i].x>>FRACBITS;
		if (vertexes[i].y>>FRACBITS < miny)
			miny = vertexes[i].y>>FRACBITS;
		else if (vertexes[i].y>>FRACBITS > maxy)
			maxy = vertexes[i].y>>FRACBITS;
	}

	// Save blockmap parameters
	bmaporgx = minx << FRACBITS;
	bmaporgy = miny << FRACBITS;
	bmapwidth = ((maxx-minx) >> MAPBTOFRAC) + 1;
	bmapheight = ((maxy-miny) >> MAPBTOFRAC)+ 1;

	// Compute blockmap, which is stored as a 2d array of variable-sized lists.
	//
	// Pseudocode:
	//
	// For each linedef:
	//
	//   Map the starting and ending vertices to blocks.
	//
	//   Starting in the starting vertex's block, do:
	//
	//     Add linedef to current block's list, dynamically resizing it.
	//
	//     If current block is the same as the ending vertex's block, exit loop.
	//
	//     Move to an adjacent block by moving towards the ending block in
	//     either the x or y direction, to the block which contains the linedef.

	{
		typedef struct
		{
			INT32 n, nalloc;
			INT32 *list;
		} bmap_t; // blocklist structure

		size_t tot = bmapwidth * bmapheight; // size of blockmap
		bmap_t *bmap = calloc(tot, sizeof (*bmap)); // array of blocklists
		boolean straight;

		if (bmap == NULL) I_Error("%s: Out of memory making blockmap", "P_CreateBlockMap");

		for (i = 0; i < numlines; i++)
		{
			// starting coordinates
			INT32 x = (lines[i].v1->x>>FRACBITS) - minx;
			INT32 y = (lines[i].v1->y>>FRACBITS) - miny;
			INT32 bxstart, bxend, bystart, byend, v2x, v2y, curblockx, curblocky;

			v2x = lines[i].v2->x>>FRACBITS;
			v2y = lines[i].v2->y>>FRACBITS;

			// Draw a "box" around the line.
			bxstart = (x >> MAPBTOFRAC);
			bystart = (y >> MAPBTOFRAC);

			v2x -= minx;
			v2y -= miny;

			bxend = ((v2x) >> MAPBTOFRAC);
			byend = ((v2y) >> MAPBTOFRAC);

			if (bxend < bxstart)
			{
				INT32 temp = bxstart;
				bxstart = bxend;
				bxend = temp;
			}

			if (byend < bystart)
			{
				INT32 temp = bystart;
				bystart = byend;
				byend = temp;
			}

			// Catch straight lines
			// This fixes the error where straight lines
			// directly on a blockmap boundary would not
			// be included in the proper blocks.
			if (lines[i].v1->y == lines[i].v2->y)
			{
				straight = true;
				bystart--;
				byend++;
			}
			else if (lines[i].v1->x == lines[i].v2->x)
			{
				straight = true;
				bxstart--;
				bxend++;
			}
			else
				straight = false;

			// Now we simply iterate block-by-block until we reach the end block.
			for (curblockx = bxstart; curblockx <= bxend; curblockx++)
			for (curblocky = bystart; curblocky <= byend; curblocky++)
			{
				size_t b = curblocky * bmapwidth + curblockx;

				if (b >= tot)
					continue;

				if (!straight && !(LineInBlock((fixed_t)x, (fixed_t)y, (fixed_t)v2x, (fixed_t)v2y, (fixed_t)(curblockx << MAPBTOFRAC), (fixed_t)(curblocky << MAPBTOFRAC))))
					continue;

				// Increase size of allocated list if necessary
				if (bmap[b].n >= bmap[b].nalloc)
				{
					// Graue 02-29-2004: make code more readable, don't realloc a null pointer
					// (because it crashes for me, and because the comp.lang.c FAQ says so)
					if (bmap[b].nalloc == 0)
						bmap[b].nalloc = 8;
					else
						bmap[b].nalloc *= 2;
					bmap[b].list = Z_Realloc(bmap[b].list, bmap[b].nalloc * sizeof (*bmap->list), PU_CACHE, &bmap[b].list);
					if (!bmap[b].list)
						I_Error("Out of Memory in P_CreateBlockMap");
				}

				// Add linedef to end of list
				bmap[b].list[bmap[b].n++] = (INT32)i;
			}
		}

		// Compute the total size of the blockmap.
		//
		// Compression of empty blocks is performed by reserving two offset words
		// at tot and tot+1.
		//
		// 4 words, unused if this routine is called, are reserved at the start.
		{
			size_t count = tot + 6; // we need at least 1 word per block, plus reserved's

			for (i = 0; i < tot; i++)
				if (bmap[i].n)
					count += bmap[i].n + 2; // 1 header word + 1 trailer word + blocklist

			// Allocate blockmap lump with computed count
			blockmaplump = Z_Calloc(sizeof (*blockmaplump) * count, PU_LEVEL, NULL);
		}

		// Now compress the blockmap.
		{
			size_t ndx = tot += 4; // Advance index to start of linedef lists
			bmap_t *bp = bmap; // Start of uncompressed blockmap

			blockmaplump[ndx++] = 0; // Store an empty blockmap list at start
			blockmaplump[ndx++] = -1; // (Used for compression)

			for (i = 4; i < tot; i++, bp++)
				if (bp->n) // Non-empty blocklist
				{
					blockmaplump[blockmaplump[i] = (INT32)(ndx++)] = 0; // Store index & header
					do
						blockmaplump[ndx++] = bp->list[--bp->n]; // Copy linedef list
					while (bp->n);
					blockmaplump[ndx++] = -1; // Store trailer
					Z_Free(bp->list); // Free linedef list
				}
				else // Empty blocklist: point to reserved empty blocklist
					blockmaplump[i] = (INT32)tot;

			free(bmap); // Free uncompressed blockmap
		}
	}
	{
		size_t count = sizeof (*blocklinks) * bmapwidth * bmapheight;
		// clear out mobj chains (copied from from P_LoadBlockMap)
		blocklinks = Z_Calloc(count, PU_LEVEL, NULL);
		blockmap = blockmaplump + 4;

		// haleyjd 2/22/06: setup polyobject blockmap
		count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
		polyblocklinks = Z_Calloc(count, PU_LEVEL, NULL);
	}
}

// PK3 version
// -- Monster Iestyn 09/01/18
static void P_LoadReject(UINT8 *data, size_t count)
{
	if (!count) // zero length, someone probably used ZDBSP
	{
		rejectmatrix = NULL;
		CONS_Debug(DBG_SETUP, "P_LoadReject: REJECT lump has size 0, will not be loaded\n");
	}
	else
	{
		rejectmatrix = Z_Malloc(count, PU_LEVEL, NULL); // allocate memory for the reject matrix
		M_Memcpy(rejectmatrix, data, count); // copy the data into it
	}
}

static void P_LoadMapLUT(const virtres_t *virt)
{
	virtlump_t* virtblockmap = vres_Find(virt, "BLOCKMAP");
	virtlump_t* virtreject   = vres_Find(virt, "REJECT");

	// Lookup tables
	if (virtreject)
		P_LoadReject(virtreject->data, virtreject->size);
	else
		rejectmatrix = NULL;

	if (!(virtblockmap && P_LoadBlockMap(virtblockmap->data, virtblockmap->size)))
		P_CreateBlockMap();
}

//
// P_LinkMapData
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
static void P_LinkMapData(void)
{
	size_t i, j;
	line_t *li;
	sector_t *sector;
	subsector_t *ss = subsectors;
	size_t sidei;
	seg_t *seg;
	fixed_t bbox[4];

	// look up sector number for each subsector
	for (i = 0; i < numsubsectors; i++, ss++)
	{
		if (ss->firstline >= numsegs)
			CorruptMapError(va("P_LinkMapData: ss->firstline invalid "
				"(subsector %s, firstline refers to %d of %s)", sizeu1(i), ss->firstline,
				sizeu2(numsegs)));
		seg = &segs[ss->firstline];
		sidei = (size_t)(seg->sidedef - sides);
		if (!seg->sidedef)
			CorruptMapError(va("P_LinkMapData: seg->sidedef is NULL "
				"(subsector %s, firstline is %d)", sizeu1(i), ss->firstline));
		if (seg->sidedef - sides < 0 || seg->sidedef - sides > (UINT16)numsides)
			CorruptMapError(va("P_LinkMapData: seg->sidedef refers to sidedef %s of %s "
				"(subsector %s, firstline is %d)", sizeu1(sidei), sizeu2(numsides),
				sizeu3(i), ss->firstline));
		if (!seg->sidedef->sector)
			CorruptMapError(va("P_LinkMapData: seg->sidedef->sector is NULL "
				"(subsector %s, firstline is %d, sidedef is %s)", sizeu1(i), ss->firstline,
				sizeu1(sidei)));
		ss->sector = seg->sidedef->sector;
	}

	// count number of lines in each sector
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->frontsector->linecount++;

		if (li->backsector && li->backsector != li->frontsector)
			li->backsector->linecount++;
	}

	// allocate linebuffers for each sector
	for (i = 0, sector = sectors; i < numsectors; i++, sector++)
	{
		if (sector->linecount == 0) // no lines found?
		{
			sector->lines = NULL;
			CONS_Debug(DBG_SETUP, "P_LinkMapData: sector %s has no lines\n", sizeu1(i));
		}
		else
		{
			sector->lines = Z_Calloc(sector->linecount * sizeof(line_t*), PU_LEVEL, NULL);

			// zero the count, since we'll later use this to track how many we've recorded
			sector->linecount = 0;
		}
	}

	// iterate through lines, assigning them to sectors' linebuffers,
	// and recalculate the counts in the process
	for (i = 0, li = lines; i < numlines; i++, li++)
	{
		li->frontsector->lines[li->frontsector->linecount++] = li;

		if (li->backsector && li->backsector != li->frontsector)
			li->backsector->lines[li->backsector->linecount++] = li;
	}

	// set soundorg's position for each sector
	for (i = 0, sector = sectors; i < numsectors; i++, sector++)
	{
		M_ClearBox(bbox);

		if (sector->linecount != 0)
		{
			for (j = 0; j < sector->linecount; j++)
			{
				li = sector->lines[j];
				M_AddToBox(bbox, li->v1->x, li->v1->y);
				M_AddToBox(bbox, li->v2->x, li->v2->y);
			}
		}

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (((bbox[BOXRIGHT]>>FRACBITS) + (bbox[BOXLEFT]>>FRACBITS))/2)<<FRACBITS;
		sector->soundorg.y = (((bbox[BOXTOP]>>FRACBITS) + (bbox[BOXBOTTOM]>>FRACBITS))/2)<<FRACBITS;
		sector->soundorg.z = sector->floorheight; // default to sector's floor height
	}
}

// For maps in binary format, add multi-tags from linedef specials. This must be done
// before any linedef specials have been processed.
static void P_AddBinaryMapTagsFromLine(sector_t *sector, line_t *line)
{
	Tag_Add(&sector->tags, Tag_FGet(&line->tags));
	if (line->flags & ML_EFFECT6) {
		if (sides[line->sidenum[0]].textureoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[0]].textureoffset / FRACUNIT);
		if (sides[line->sidenum[0]].rowoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[0]].rowoffset / FRACUNIT);
	}
	if (line->flags & ML_TFERLINE) {
		if (sides[line->sidenum[1]].textureoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[1]].textureoffset / FRACUNIT);
		if (sides[line->sidenum[1]].rowoffset)
			Tag_Add(&sector->tags, (INT32)sides[line->sidenum[1]].rowoffset / FRACUNIT);
	}
}

static void P_AddBinaryMapTags(void)
{
	size_t i;

	for (i = 0; i < numlines; i++) {
		// 97: Apply Tag to Front Sector
		// 98: Apply Tag to Back Sector
		// 99: Apply Tag to Front and Back Sectors
		if (lines[i].special == 97 || lines[i].special == 99)
			P_AddBinaryMapTagsFromLine(lines[i].frontsector, &lines[i]);
		if (lines[i].special == 98 || lines[i].special == 99)
			P_AddBinaryMapTagsFromLine(lines[i].backsector, &lines[i]);
	}

	// Run this loop after the 97-99 loop to ensure that 96 can search through all of the
	// 97-99-applied tags.
	for (i = 0; i < numlines; i++) {
		size_t j;
		mtag_t tag, target_tag;
		mtag_t offset_tags[4];

		// 96: Apply Tag to Tagged Sectors
		if (lines[i].special != 96)
			continue;

		tag = Tag_FGet(&lines[i].frontsector->tags);
		target_tag = Tag_FGet(&lines[i].tags);
		memset(offset_tags, 0, sizeof(mtag_t)*4);
		if (lines[i].flags & ML_EFFECT6) {
			offset_tags[0] = (INT32)sides[lines[i].sidenum[0]].textureoffset / FRACUNIT;
			offset_tags[1] = (INT32)sides[lines[i].sidenum[0]].rowoffset / FRACUNIT;
		}
		if (lines[i].flags & ML_TFERLINE) {
			offset_tags[2] = (INT32)sides[lines[i].sidenum[1]].textureoffset / FRACUNIT;
			offset_tags[3] = (INT32)sides[lines[i].sidenum[1]].rowoffset / FRACUNIT;
		}

		for (j = 0; j < numsectors; j++) {
			boolean matches_target_tag = target_tag && Tag_Find(&sectors[j].tags, target_tag);
			size_t k;
			for (k = 0; k < 4; k++) {
				if (lines[i].flags & ML_WRAPMIDTEX) {
					if (matches_target_tag || (offset_tags[k] && Tag_Find(&sectors[j].tags, offset_tags[k]))) {
						Tag_Add(&sectors[j].tags, tag);
						break;
					}
				} else if (matches_target_tag) {
					if (k == 0)
						Tag_Add(&sectors[j].tags, tag);
					if (offset_tags[k])
						Tag_Add(&sectors[j].tags, offset_tags[k]);
				}
			}
		}
	}

	for (i = 0; i < nummapthings; i++)
	{
		switch (mapthings[i].type)
		{
		case 291:
		case 322:
		case 750:
		case 760:
		case 761:
		case 762:
			Tag_FSet(&mapthings[i].tags, mapthings[i].angle);
			break;
		case 290:
		case 292:
		case 294:
		case 780:
			Tag_FSet(&mapthings[i].tags, mapthings[i].extrainfo);
			break;
		default:
			break;
		}
	}
}

static void P_WriteConstant(INT32 constant, char **target)
{
	char buffer[12];
	sprintf(buffer, "%d", constant);
	*target = Z_Malloc(strlen(buffer) + 1, PU_LEVEL, NULL);
	M_Memcpy(*target, buffer, strlen(buffer) + 1);
}

static line_t *P_FindPointPushLine(taglist_t *list)
{
	INT32 i, l;

	for (i = 0; i < list->count; i++)
	{
		mtag_t tag = list->tags[i];
		TAG_ITER_LINES(tag, l)
		{
			if (Tag_FGet(&lines[l].tags) != tag)
				continue;

			if (lines[l].special != 547)
				continue;

			return &lines[l];
		}
	}

	return NULL;
}

static void P_SetBinaryFOFAlpha(line_t *line)
{
	if (sides[line->sidenum[0]].toptexture > 0)
	{
		line->args[1] = sides[line->sidenum[0]].toptexture;
		if (sides[line->sidenum[0]].toptexture >= 1001)
		{
			line->args[2] = (sides[line->sidenum[0]].toptexture/1000);
			line->args[1] %= 1000;
		}
	}
	else
	{
		line->args[1] = 128;
		line->args[2] = TMB_TRANSLUCENT;
	}
}

static INT32 P_GetFOFFlags(INT32 oldflags)
{
	INT32 result = 0;
	if (oldflags & FF_OLD_EXISTS)
		result |= FOF_EXISTS;
	if (oldflags & FF_OLD_BLOCKPLAYER)
		result |= FOF_BLOCKPLAYER;
	if (oldflags & FF_OLD_BLOCKOTHERS)
		result |= FOF_BLOCKOTHERS;
	if (oldflags & FF_OLD_RENDERSIDES)
		result |= FOF_RENDERSIDES;
	if (oldflags & FF_OLD_RENDERPLANES)
		result |= FOF_RENDERPLANES;
	if (oldflags & FF_OLD_SWIMMABLE)
		result |= FOF_SWIMMABLE;
	if (oldflags & FF_OLD_NOSHADE)
		result |= FOF_NOSHADE;
	if (oldflags & FF_OLD_CUTSOLIDS)
		result |= FOF_CUTSOLIDS;
	if (oldflags & FF_OLD_CUTEXTRA)
		result |= FOF_CUTEXTRA;
	if (oldflags & FF_OLD_CUTSPRITES)
		result |= FOF_CUTSPRITES;
	if (oldflags & FF_OLD_BOTHPLANES)
		result |= FOF_BOTHPLANES;
	if (oldflags & FF_OLD_EXTRA)
		result |= FOF_EXTRA;
	if (oldflags & FF_OLD_TRANSLUCENT)
		result |= FOF_TRANSLUCENT;
	if (oldflags & FF_OLD_FOG)
		result |= FOF_FOG;
	if (oldflags & FF_OLD_INVERTPLANES)
		result |= FOF_INVERTPLANES;
	if (oldflags & FF_OLD_ALLSIDES)
		result |= FOF_ALLSIDES;
	if (oldflags & FF_OLD_INVERTSIDES)
		result |= FOF_INVERTSIDES;
	if (oldflags & FF_OLD_DOUBLESHADOW)
		result |= FOF_DOUBLESHADOW;
	if (oldflags & FF_OLD_FLOATBOB)
		result |= FOF_FLOATBOB;
	if (oldflags & FF_OLD_NORETURN)
		result |= FOF_NORETURN;
	if (oldflags & FF_OLD_CRUMBLE)
		result |= FOF_CRUMBLE;
	if (oldflags & FF_OLD_GOOWATER)
		result |= FOF_GOOWATER;
	if (oldflags & FF_OLD_MARIO)
		result |= FOF_MARIO;
	if (oldflags & FF_OLD_BUSTUP)
		result |= FOF_BUSTUP;
	if (oldflags & FF_OLD_QUICKSAND)
		result |= FOF_QUICKSAND;
	if (oldflags & FF_OLD_PLATFORM)
		result |= FOF_PLATFORM;
	if (oldflags & FF_OLD_REVERSEPLATFORM)
		result |= FOF_REVERSEPLATFORM;
	if (oldflags & FF_OLD_RIPPLE)
		result |= FOF_RIPPLE;
	if (oldflags & FF_OLD_COLORMAPONLY)
		result |= FOF_COLORMAPONLY;
	return result;
}

static INT32 P_GetFOFBusttype(INT32 oldflags)
{
	if (oldflags & FF_OLD_SHATTER)
		return TMFB_TOUCH;
	if (oldflags & FF_OLD_SPINBUST)
		return TMFB_SPIN;
	if (oldflags & FF_OLD_STRONGBUST)
		return TMFB_STRONG;
	return TMFB_REGULAR;
}

static void P_ConvertBinaryLinedefTypes(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		mtag_t tag = Tag_FGet(&lines[i].tags);

		switch (lines[i].special)
		{
		case 2: //Custom exit
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMEF_SKIPTALLY;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[1] |= TMEF_EMERALDCHECK;
			break;
		case 3: //Zoom tube parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_MIDSOLID);
			break;
		case 4: //Speed pad parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[1] |= TMSP_NOTELEPORT;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[1] |= TMSP_FORCESPIN;
			P_WriteConstant(sides[lines[i].sidenum[0]].toptexture ? sides[lines[i].sidenum[0]].toptexture : sfx_spdpad, &lines[i].stringargs[0]);
			break;
		case 7: //Sector flat alignment
			lines[i].args[0] = tag;
			if ((lines[i].flags & (ML_NETONLY|ML_NONET)) == (ML_NETONLY|ML_NONET))
			{
				CONS_Alert(CONS_WARNING, M_GetText("Flat alignment linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"), tag);
				lines[i].special = 0;
			}
			else if (lines[i].flags & ML_NETONLY)
				lines[i].args[1] = TMP_CEILING;
			else if (lines[i].flags & ML_NONET)
				lines[i].args[1] = TMP_FLOOR;
			else
				lines[i].args[1] = TMP_BOTH;
			lines[i].flags &= ~(ML_NETONLY|ML_NONET);

			if (lines[i].flags & ML_EFFECT6) // Set offset through x and y texture offsets
			{
				angle_t flatangle = InvAngle(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y));
				fixed_t xoffs = sides[lines[i].sidenum[0]].textureoffset;
				fixed_t yoffs = sides[lines[i].sidenum[0]].rowoffset;

				//If no tag is given, apply to front sector
				if (lines[i].args[0] == 0)
					P_ApplyFlatAlignment(lines[i].frontsector, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				else
				{
					INT32 s;
					TAG_ITER_SECTORS(lines[i].args[0], s)
						P_ApplyFlatAlignment(sectors + s, flatangle, xoffs, yoffs, lines[i].args[1] != TMP_CEILING, lines[i].args[1] != TMP_FLOOR);
				}
				lines[i].special = 0;
			}
			break;
		case 8: //Special sector properties
		{
			INT32 s;

			lines[i].args[0] = tag;
			TAG_ITER_SECTORS(tag, s)
			{
				if (lines[i].flags & ML_NOCLIMB)
				{
					sectors[s].flags &= ~MSF_FLIPSPECIAL_FLOOR;
					sectors[s].flags |= MSF_FLIPSPECIAL_CEILING;
				}
				else if (lines[i].flags & ML_MIDSOLID)
					sectors[s].flags |= MSF_FLIPSPECIAL_BOTH;

				if (lines[i].flags & ML_MIDPEG)
					sectors[s].flags |= MSF_TRIGGERSPECIAL_TOUCH;
				if (lines[i].flags & ML_NOSKEW)
					sectors[s].flags |= MSF_TRIGGERSPECIAL_HEADBUMP;

				if (lines[i].flags & ML_SKEWTD)
					sectors[s].flags |= MSF_INVERTPRECIP;
			}

			if (GETSECSPECIAL(lines[i].frontsector->special, 4) != 12)
				lines[i].special = 0;

			break;
		}
		case 10: //Culling plane
			lines[i].args[0] = tag;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 11: //Rope hang parameters
			lines[i].args[0] = (lines[i].flags & ML_NOCLIMB) ? 0 : sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_SKEWTD);
			break;
		case 13: //Heat wave effect
		{
			INT32 s;

			TAG_ITER_SECTORS(tag, s)
				sectors[s].flags |= MSF_HEATWAVE;

			break;
		}
		case 14: //Bustable block parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_SKEWTD);
			if (sides[lines[i].sidenum[0]].toptexture)
				P_WriteConstant(sides[lines[i].sidenum[0]].toptexture, &lines[i].stringargs[0]);
			break;
		case 16: //Minecart parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 20: //PolyObject first line
		{
			INT32 check = -1;
			INT32 paramline = -1;

			TAG_ITER_LINES(tag, check)
			{
				if (lines[check].special == 22)
				{
					paramline = check;
					break;
				}
			}

			//PolyObject ID
			lines[i].args[0] = tag;

			//Default: Invisible planes
			lines[i].args[3] |= TMPF_INVISIBLEPLANES;

			//Linedef executor tag
			lines[i].args[4] = 32000 + lines[i].args[0];

			if (paramline == -1)
				break; // no extra settings to apply, let's leave it

			//Parent ID
			lines[i].args[1] = lines[paramline].frontsector->special;
			//Translucency
			lines[i].args[2] = (lines[paramline].flags & ML_DONTPEGTOP)
						? (sides[lines[paramline].sidenum[0]].textureoffset >> FRACBITS)
						: ((lines[paramline].frontsector->floorheight >> FRACBITS) / 100);

			//Flags
			if (lines[paramline].flags & ML_SKEWTD)
				lines[i].args[3] |= TMPF_NOINSIDES;
			if (lines[paramline].flags & ML_NOSKEW)
				lines[i].args[3] |= TMPF_INTANGIBLE;
			if (lines[paramline].flags & ML_MIDPEG)
				lines[i].args[3] |= TMPF_PUSHABLESTOP;
			if (lines[paramline].flags & ML_MIDSOLID)
				lines[i].args[3] &= ~TMPF_INVISIBLEPLANES;
			/*if (lines[paramline].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMPF_DONTCLIPPLANES;*/
			if (lines[paramline].flags & ML_EFFECT6)
				lines[i].args[3] |= TMPF_SPLAT;
			if (lines[paramline].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMPF_EXECUTOR;

			break;
		}
		case 30: //Polyobject - waving flag
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 31: //Polyobject - displacement by front sector
			lines[i].args[0] = tag;
			lines[i].args[1] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			break;
		case 32: //Polyobject - angular displacement by front sector
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : 128;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset ? sides[lines[i].sidenum[0]].rowoffset >> FRACBITS : 90;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMPR_DONTROTATEOTHERS;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMPR_ROTATEPLAYERS;
			break;
		case 50: //Instantly lower floor on level load
		case 51: //Instantly raise ceiling on level load
			lines[i].args[0] = tag;
			break;
		case 52: //Continuously falling sector
			lines[i].args[0] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 53: //Continuous floor/ceiling mover
		case 54: //Continuous floor mover
		case 55: //Continuous ceiling mover
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 53) ? TMP_BOTH : lines[i].special - 54;
			lines[i].args[2] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[3] = lines[i].args[2];
			lines[i].args[4] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[5] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 53;
			break;
		case 56: //Continuous two-speed floor/ceiling mover
		case 57: //Continuous two-speed floor mover
		case 58: //Continuous two-speed ceiling mover
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 56) ? TMP_BOTH : lines[i].special - 57;
			lines[i].args[2] = abs(lines[i].dx) >> FRACBITS;
			lines[i].args[3] = abs(lines[i].dy) >> FRACBITS;
			lines[i].args[4] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[5] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 56;
			break;
		case 59: //Activate moving platform
		case 60: //Activate moving platform (adjustable speed)
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 60) ? P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS : 8;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[4] = (lines[i].flags & ML_NOCLIMB) ? 1 : 0;
			lines[i].special = 60;
			break;
		case 61: //Crusher (Ceiling to floor)
		case 62: //Crusher (Floor to ceiling)
			lines[i].args[0] = tag;
			lines[i].args[1] = lines[i].special - 61;
			if (lines[i].flags & ML_MIDSOLID)
			{
				lines[i].args[2] = abs(lines[i].dx) >> FRACBITS;
				lines[i].args[3] = lines[i].args[2];
			}
			else
			{
				lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> (FRACBITS + 1);
				lines[i].args[3] = lines[i].args[2] / 4;
			}
			lines[i].special = 61;
			break;
		case 63: //Fake floor/ceiling planes
			lines[i].args[0] = tag;
			break;
		case 64: //Appearing/disappearing FOF
			lines[i].args[0] = (lines[i].flags & ML_BLOCKMONSTERS) ? 0 : tag;
			lines[i].args[1] = (lines[i].flags & ML_BLOCKMONSTERS) ? tag : Tag_FGet(&lines[i].frontsector->tags);
			lines[i].args[2] = lines[i].dx >> FRACBITS;
			lines[i].args[3] = lines[i].dy >> FRACBITS;
			lines[i].args[4] = lines[i].frontsector->floorheight >> FRACBITS;
			lines[i].args[5] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 66: //Move floor by displacement
		case 67: //Move ceiling by displacement
		case 68: //Move floor and ceiling by displacement
			lines[i].args[0] = tag;
			lines[i].args[1] = lines[i].special - 66;
			lines[i].args[2] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] *= -1;
			lines[i].special = 66;
			break;
		case 76: //Make FOF bouncy
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			break;
		case 100: //FOF: solid, opaque, shadowcasting
		case 101: //FOF: solid, opaque, non-shadowcasting
		case 102: //FOF: solid, translucent
		case 103: //FOF: solid, sides only
		case 104: //FOF: solid, no sides
		case 105: //FOF: solid, invisible
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 102)
			{
				if (lines[i].flags & ML_NOCLIMB)
					lines[i].args[3] |= TMFA_INSIDES;
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 105)
				lines[i].args[3] |= TMFA_NOPLANES|TMFA_NOSIDES;
			else if (lines[i].special == 104)
				lines[i].args[3] |= TMFA_NOSIDES;
			else if (lines[i].special == 103)
				lines[i].args[3] |= TMFA_NOPLANES;
			if (lines[i].special != 100 && (lines[i].special != 104 || !(lines[i].flags & ML_NOCLIMB)))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[3] |= TMFA_SPLAT;

			//Tangibility
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= TMFT_DONTBLOCKPLAYER;

			lines[i].special = 100;
			break;
		case 120: //FOF: water, opaque
		case 121: //FOF: water, translucent
		case 122: //FOF: water, opaque, no sides
		case 123: //FOF: water, translucent, no sides
		case 124: //FOF: goo water, translucent
		case 125: //FOF: goo water, translucent, no sides
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 120 || lines[i].special == 122)
				lines[i].args[1] = 255;
			else
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFW_SPLAT;
			}

			//No sides?
			if (lines[i].special == 122 || lines[i].special == 123 || lines[i].special == 125)
				lines[i].args[3] |= TMFW_NOSIDES;

			//Flags
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMFW_DOUBLESHADOW;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMFW_COLORMAPONLY;
			if (!(lines[i].flags & ML_WRAPMIDTEX))
				lines[i].args[3] |= TMFW_NORIPPLE;

			//Goo?
			if (lines[i].special >= 124)
				lines[i].args[3] |= TMFW_GOOWATER;

			//Splat rendering?
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[3] |= TMFW_SPLAT;

			lines[i].special = 120;
			break;
		case 140: //FOF: intangible from bottom, opaque
		case 141: //FOF: intangible from bottom, translucent
		case 142: //FOF: intangible from bottom, translucent, no sides
		case 143: //FOF: intangible from top, opaque
		case 144: //FOF: intangible from top, translucent
		case 145: //FOF: intangible from top, translucent, no sides
		case 146: //FOF: only tangible from sides
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 141 || lines[i].special == 142 || lines[i].special == 144 || lines[i].special == 145)
			{
				if (lines[i].flags & ML_NOCLIMB)
					lines[i].args[3] |= TMFA_INSIDES;
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 142 || lines[i].special == 145)
				lines[i].args[3] |= TMFA_NOSIDES;
			else if (lines[i].special == 146)
				lines[i].args[3] |= TMFA_NOPLANES;
			if (lines[i].special != 146 && (lines[i].flags & ML_NOCLIMB))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[3] |= TMFA_SPLAT;

			//Tangibility
			if (lines[i].special <= 142)
				lines[i].args[4] |= TMFT_INTANGIBLEBOTTOM;
			else if (lines[i].special <= 145)
				lines[i].args[4] |= TMFT_INTANGIBLETOP;
			else
				lines[i].args[4] |= TMFT_INTANGIBLEBOTTOM|TMFT_INTANGIBLETOP;

			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= TMFT_DONTBLOCKPLAYER;

			lines[i].special = 100;
			break;
		case 150: //FOF: Air bobbing
		case 151: //FOF: Air bobbing (adjustable)
		case 152: //FOF: Reverse air bobbing (adjustable)
		case 153: //FOF: Dynamically sinking platform
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 150) ? 16 : (P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS);

			//Flags
			if (lines[i].special == 152)
				lines[i].args[2] |= TMFB_REVERSE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMFB_SPINDASH;
			if (lines[i].special == 153)
				lines[i].args[2] |= TMFB_DYNAMIC;

			lines[i].special = 150;
			break;
		case 160: //FOF: Water bobbing
			lines[i].args[0] = tag;
			break;
		case 170: //FOF: Crumbling, respawn
		case 171: //FOF: Crumbling, no respawn
		case 172: //FOF: Crumbling, respawn, intangible from bottom
		case 173: //FOF: Crumbling, no respawn, intangible from bottom
		case 174: //FOF: Crumbling, respawn, intangible from bottom, translucent
		case 175: //FOF: Crumbling, no respawn, intangible from bottom, translucent
		case 176: //FOF: Crumbling, respawn, floating, bobbing
		case 177: //FOF: Crumbling, no respawn, floating, bobbing
		case 178: //FOF: Crumbling, respawn, floating
		case 179: //FOF: Crumbling, no respawn, floating
		case 180: //FOF: Crumbling, respawn, air bobbing
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special >= 174 && lines[i].special <= 175)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[4] |= TMFC_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			if (lines[i].special >= 172 && lines[i].special <= 175)
			{
				lines[i].args[3] |= TMFT_INTANGIBLEBOTTOM;
				if (lines[i].flags & ML_NOCLIMB)
					lines[i].args[4] |= TMFC_NOSHADE;
			}

			if (lines[i].special % 2 == 1)
				lines[i].args[4] |= TMFC_NORETURN;
			if (lines[i].special == 176 || lines[i].special == 177 || lines[i].special == 180)
				lines[i].args[4] |= TMFC_AIRBOB;
			if (lines[i].special >= 176 && lines[i].special <= 179)
				lines[i].args[4] |= TMFC_FLOATBOB;
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[4] |= TMFC_SPLAT;

			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[3] |= TMFT_DONTBLOCKPLAYER;

			lines[i].special = 170;
			break;
		case 190: // FOF: Rising, solid, opaque, shadowcasting
		case 191: // FOF: Rising, solid, opaque, non-shadowcasting
		case 192: // FOF: Rising, solid, translucent
		case 193: // FOF: Rising, solid, invisible
		case 194: // FOF: Rising, intangible from bottom, opaque
		case 195: // FOF: Rising, intangible from bottom, translucent
			lines[i].args[0] = tag;

			//Translucency
			if (lines[i].special == 192 || lines[i].special == 195)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 193)
				lines[i].args[3] |= TMFA_NOPLANES|TMFA_NOSIDES;
			if (lines[i].special >= 194)
				lines[i].args[3] |= TMFA_INSIDES;
			if (lines[i].special != 190 && (lines[i].special <= 193 || lines[i].flags & ML_NOCLIMB))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[3] |= TMFA_SPLAT;

			//Tangibility
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_DONTBLOCKOTHERS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= TMFT_DONTBLOCKPLAYER;
			if (lines[i].special >= 194)
				lines[i].args[4] |= TMFT_INTANGIBLEBOTTOM;

			//Speed
			lines[i].args[5] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;

			//Flags
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[6] |= TMFR_REVERSE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[6] |= TMFR_SPINDASH;

			lines[i].special = 190;
			break;
		case 200: //FOF: Light block
		case 201: //FOF: Half light block
			lines[i].args[0] = tag;
			if (lines[i].special == 201)
				lines[i].args[1] = 1;
			lines[i].special = 200;
			break;
		case 202: //FOF: Fog block
		case 223: //FOF: Intangible, invisible
			lines[i].args[0] = tag;
			break;
		case 220: //FOF: Intangible, opaque
		case 221: //FOF: Intangible, translucent
		case 222: //FOF: Intangible, sides only
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 221)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= TMFA_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Appearance
			if (lines[i].special == 222)
				lines[i].args[3] |= TMFA_NOPLANES;
			if (lines[i].special == 221)
				lines[i].args[3] |= TMFA_INSIDES;
			if (lines[i].special != 220 && !(lines[i].flags & ML_NOCLIMB))
				lines[i].args[3] |= TMFA_NOSHADE;
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[3] |= TMFA_SPLAT;

			lines[i].special = 220;
            break;
		case 250: //FOF: Mario block
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMFM_BRICK;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[1] |= TMFM_INVISIBLE;
			break;
		case 251: //FOF: Thwomp block
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_WRAPMIDTEX) //Custom speeds
			{
				lines[i].args[1] = lines[i].dy >> FRACBITS;
				lines[i].args[2] = lines[i].dx >> FRACBITS;
			}
			else
			{
				lines[i].args[1] = 80;
				lines[i].args[2] = 16;
			}
			if (lines[i].flags & ML_MIDSOLID)
				P_WriteConstant(sides[lines[i].sidenum[0]].textureoffset >> FRACBITS, &lines[i].stringargs[0]);
			break;
		case 252: //FOF: Shatter block
		case 253: //FOF: Shatter block, translucent
		case 254: //FOF: Bustable block
		case 255: //FOF: Spin-bustable block
		case 256: //FOF: Spin-bustable block, translucent
			lines[i].args[0] = tag;

			//Alpha
			if (lines[i].special == 253 || lines[i].special == 256)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[4] |= TMFB_SPLAT;
			}
			else
				lines[i].args[1] = 255;

			//Bustable type
			if (lines[i].special <= 253)
				lines[i].args[3] = TMFB_TOUCH;
			else if (lines[i].special >= 255)
				lines[i].args[3] = TMFB_SPIN;
			else if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] = TMFB_STRONG;
			else
				lines[i].args[3] = TMFB_REGULAR;

			//Flags
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMFB_PUSHABLES;
			if (lines[i].flags & ML_WRAPMIDTEX)
			{
				lines[i].args[4] |= TMFB_EXECUTOR;
				lines[i].args[5] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			}
			if (lines[i].special == 252 && lines[i].flags & ML_NOCLIMB)
				lines[i].args[4] |= TMFB_ONLYBOTTOM;
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[4] |= TMFB_SPLAT;

			lines[i].special = 254;
			break;
		case 257: //FOF: Quicksand
			lines[i].args[0] = tag;
			if (!(lines[i].flags & ML_WRAPMIDTEX))
				lines[i].args[1] = 1; //No ripple effect
			lines[i].args[2] = lines[i].dx >> FRACBITS; //Sinking speed
			lines[i].args[3] = lines[i].dy >> FRACBITS; //Friction
			break;
		case 258: //FOF: Laser
			lines[i].args[0] = tag;

			//Alpha
			P_SetBinaryFOFAlpha(&lines[i]);

			//Flags
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMFL_NOBOSSES;
			//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
			if (lines[i].flags & ML_EFFECT6 || lines[i].args[1] == 256)
				lines[i].args[3] |= TMFL_SPLAT;

			break;
		case 259: //Custom FOF
			if (lines[i].sidenum[1] == 0xffff)
				I_Error("Custom FOF (tag %d) found without a linedef back side!", tag);

			lines[i].args[0] = tag;
			lines[i].args[3] = P_GetFOFFlags(sides[lines[i].sidenum[1]].toptexture);
			if (lines[i].flags & ML_EFFECT6)
				lines[i].args[3] |= FOF_SPLAT;
			lines[i].args[4] = P_GetFOFBusttype(sides[lines[i].sidenum[1]].toptexture);
			if (sides[lines[i].sidenum[1]].toptexture & FF_OLD_SHATTERBOTTOM)
				lines[i].args[4] |= TMFB_ONLYBOTTOM;
			if (lines[i].args[3] & FOF_TRANSLUCENT)
			{
				P_SetBinaryFOFAlpha(&lines[i]);

				//Replicate old hack: Translucent FOFs set to full opacity cut cyan pixels
				if (lines[i].args[1] == 256)
					lines[i].args[3] |= FOF_SPLAT;
			}
			else
				lines[i].args[1] = 255;
			break;
		case 300: //Trigger linedef executor - Continuous
		case 301: //Trigger linedef executor - Each time
		case 302: //Trigger linedef executor - Once
			if (lines[i].special == 302)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 301)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].special = 300;
			break;
		case 303: //Ring count - Continuous
		case 304: //Ring count - Once
			lines[i].args[0] = (lines[i].special == 304) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMC_LTE;
			else if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[2] = TMC_GTE;
			else
				lines[i].args[2] = TMC_EQUAL;
			lines[i].args[3] = !!(lines[i].flags & ML_MIDSOLID);
			lines[i].special = 303;
			break;
		case 305: //Character ability - Continuous
		case 306: //Character ability - Each time
		case 307: //Character ability - Once
			if (lines[i].special == 307)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 306)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = (P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS) / 10;
			lines[i].special = 305;
			break;
		case 308: //Race only - once
			lines[i].args[0] = TMT_ONCE;
			lines[i].args[1] = GTR_RACE;
			lines[i].args[2] = TMF_HASANY;
			break;
		case 309: //CTF red team - continuous
		case 310: //CTF red team - each time
		case 311: //CTF blue team - continuous
		case 312: //CTF blue team - each time
			if (lines[i].special % 2 == 0)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = (lines[i].special > 310) ? TMT_BLUE : TMT_RED;
			lines[i].special = 309;
			break;
		case 313: //No more enemies - once
			lines[i].args[0] = tag;
			break;
		case 314: //Number of pushables - Continuous
		case 315: //Number of pushables - Once
			lines[i].args[0] = (lines[i].special == 315) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMC_GTE;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[2] = TMC_LTE;
			else
				lines[i].args[2] = TMC_EQUAL;
			lines[i].special = 314;
			break;
		case 317: //Condition set trigger - Continuous
		case 318: //Condition set trigger - Once
			lines[i].args[0] = (lines[i].special == 318) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 317;
			break;
		case 319: //Unlockable trigger - Continuous
		case 320: //Unlockable trigger - Once
			lines[i].args[0] = (lines[i].special == 320) ? TMT_ONCE : TMT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].special = 319;
			break;
		case 321: //Trigger after X calls - Continuous
		case 322: //Trigger after X calls - Each time
			if (lines[i].special % 2 == 0)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMXT_EACHTIMEENTERANDEXIT : TMXT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMXT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
			{
				lines[i].args[2] = 1;
				lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			}
			else
				lines[i].args[2] = lines[i].args[3] = 0;
			lines[i].special = 321;
			break;
		case 323: //NiGHTSerize - Each time
		case 324: //NiGHTSerize - Once
		case 325: //DeNiGHTSerize - Each time
		case 326: //DeNiGHTSerize - Once
		case 327: //NiGHTS lap - Each time
		case 328: //NiGHTS lap - Once
		case 329: //Ideya capture touch - Each time
		case 330: //Ideya capture touch - Once
			lines[i].args[0] = (lines[i].special + 1) % 2;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] = TMC_LTE;
			else if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[3] = TMC_GTE;
			else
				lines[i].args[3] = TMC_EQUAL;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] = TMC_LTE;
			else if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] = TMC_GTE;
			else
				lines[i].args[4] = TMC_EQUAL;
			if (lines[i].flags & ML_DONTPEGBOTTOM)
				lines[i].args[5] = TMNP_SLOWEST;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[5] = TMNP_TRIGGERER;
			else
				lines[i].args[5] = TMNP_FASTEST;
			if (lines[i].special % 2 == 0)
				lines[i].special--;
			if (lines[i].special == 323)
			{
				if (lines[i].flags & ML_TFERLINE)
					lines[i].args[6] = TMN_FROMNONIGHTS;
				else if (lines[i].flags & ML_DONTPEGTOP)
					lines[i].args[6] = TMN_FROMNIGHTS;
				else
					lines[i].args[6] = TMN_ALWAYS;

				if (lines[i].flags & ML_MIDPEG)
					lines[i].args[7] |= TMN_BONUSLAPS;
				if (lines[i].flags & ML_BOUNCY)
					lines[i].args[7] |= TMN_LEVELCOMPLETION;
			}
			else if (lines[i].special == 325)
			{
				if (lines[i].flags & ML_TFERLINE)
					lines[i].args[6] = TMD_NOBODYNIGHTS;
				else if (lines[i].flags & ML_DONTPEGTOP)
					lines[i].args[6] = TMD_SOMEBODYNIGHTS;
				else
					lines[i].args[6] = TMD_ALWAYS;

				lines[i].args[7] = !!(lines[i].flags & ML_MIDPEG);
			}
			else if (lines[i].special == 327)
				lines[i].args[6] = !!(lines[i].flags & ML_MIDPEG);
			else
			{
				if (lines[i].flags & ML_DONTPEGTOP)
					lines[i].args[6] = TMS_ALWAYS;
				else if (lines[i].flags & ML_BOUNCY)
					lines[i].args[6] = TMS_IFNOTENOUGH;
				else
					lines[i].args[6] = TMS_IFENOUGH;

				if (lines[i].flags & ML_MIDPEG)
					lines[i].args[7] |= TMI_BONUSLAPS;
				if (lines[i].flags & ML_TFERLINE)
					lines[i].args[7] |= TMI_ENTER;
			}
			break;
		case 331: // Player skin - continuous
		case 332: // Player skin - each time
		case 333: // Player skin - once
			if (lines[i].special == 333)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 332)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			if (lines[i].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(lines[i].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], lines[i].text, strlen(lines[i].text) + 1);
			}
			lines[i].special = 331;
			break;
		case 334: // Object dye - continuous
		case 335: // Object dye - each time
		case 336: // Object dye - once
			if (lines[i].special == 336)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 335)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			lines[i].special = 334;
			break;
		case 337: //Emerald check - continuous
		case 338: //Emerald check - each time
		case 339: //Emerald check - once
			if (lines[i].special == 339)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 338)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = EMERALD1|EMERALD2|EMERALD3|EMERALD4|EMERALD5|EMERALD6|EMERALD7;
			lines[i].args[2] = TMF_HASALL;
			lines[i].special = 337;
			break;
		case 340: //NiGHTS mare - continuous
		case 341: //NiGHTS mare - each time
		case 342: //NiGHTS mare - once
			if (lines[i].special == 342)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 341)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMC_LTE;
			else if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[2] = TMC_GTE;
			else
				lines[i].args[2] = TMC_EQUAL;
			lines[i].special = 340;
			break;
		case 343: //Gravity check - continuous
		case 344: //Gravity check - each time
		case 345: //Gravity check - once
			if (lines[i].special == 345)
				lines[i].args[0] = TMT_ONCE;
			else if (lines[i].special == 344)
				lines[i].args[0] = (lines[i].flags & ML_BOUNCY) ? TMT_EACHTIMEENTERANDEXIT : TMT_EACHTIMEENTER;
			else
				lines[i].args[0] = TMT_CONTINUOUS;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[1] = TMG_TEMPREVERSE;
			else if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] = TMG_REVERSE;
			lines[i].special = 343;
			break;
		case 400: //Set tagged sector's floor height/texture
		case 401: //Set tagged sector's ceiling height/texture
			lines[i].args[0] = tag;
			lines[i].args[1] = lines[i].special - 400;
			lines[i].args[2] = !(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 400;
			break;
		case 402: //Copy light level
			lines[i].args[0] = tag;
			lines[i].args[1] = 0;
			break;
		case 403: //Move tagged sector's floor
		case 404: //Move tagged sector's ceiling
			lines[i].args[0] = tag;
			lines[i].args[1] = lines[i].special - 403;
			lines[i].args[2] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[3] = (lines[i].flags & ML_BLOCKMONSTERS) ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : 0;
			lines[i].args[4] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 403;
			break;
		case 405: //Move floor according to front texture offsets
		case 407: //Move ceiling according to front texture offsets
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 405) ? TMP_FLOOR : TMP_CEILING;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[4] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].special = 405;
			break;
		case 408: //Set flats
			lines[i].args[0] = tag;
			if ((lines[i].flags & (ML_NOCLIMB|ML_MIDSOLID)) == (ML_NOCLIMB|ML_MIDSOLID))
			{
				CONS_Alert(CONS_WARNING, M_GetText("Set flats linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"), tag);
				lines[i].special = 0;
			}
			else if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] = TMP_CEILING;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[1] = TMP_FLOOR;
			else
				lines[i].args[1] = TMP_BOTH;
			break;
		case 409: //Change tagged sector's tag
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] = TMT_ADD;
			else if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[2] = TMT_REMOVE;
			else
				lines[i].args[2] = TMT_REPLACEFIRST;
			break;
		case 410: //Change front sector's tag
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] = TMT_ADD;
			else if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[1] = TMT_REMOVE;
			else
				lines[i].args[1] = TMT_REPLACEFIRST;
			break;
		case 411: //Stop plane movement
			lines[i].args[0] = tag;
			break;
		case 412: //Teleporter
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[1] |= TMT_SILENT;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[1] |= TMT_KEEPANGLE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[1] |= TMT_KEEPMOMENTUM;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[1] |= TMT_RELATIVE;
			lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[4] = lines[i].frontsector->ceilingheight >> FRACBITS;
			break;
		case 413: //Change music
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[0] |= TMM_ALLPLAYERS;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[0] |= TMM_OFFSET;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[0] |= TMM_FADE;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[0] |= TMM_NORELOAD;
			if (lines[i].flags & ML_BOUNCY)
				lines[i].args[0] |= TMM_FORCERESET;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[0] |= TMM_NOLOOP;
			lines[i].args[1] = sides[lines[i].sidenum[0]].midtexture;
			lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[4] = (lines[i].sidenum[1] != 0xffff) ? sides[lines[i].sidenum[1]].textureoffset >> FRACBITS : 0;
			lines[i].args[5] = (lines[i].sidenum[1] != 0xffff) ? sides[lines[i].sidenum[1]].rowoffset >> FRACBITS : -1;
			lines[i].args[6] = sides[lines[i].sidenum[0]].bottomtexture;
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			break;
		case 414: //Play sound effect
			lines[i].args[2] = tag;
			if (tag != 0)
			{
				if (lines[i].flags & ML_WRAPMIDTEX)
				{
					lines[i].args[0] = TMSS_TAGGEDSECTOR;
					lines[i].args[1] = TMSL_EVERYONE;
				}
				else
				{
					lines[i].args[0] = TMSS_NOWHERE;
					lines[i].args[1] = TMSL_TAGGEDSECTOR;
				}
			}
			else
			{
				if (lines[i].flags & ML_NOCLIMB)
				{
					lines[i].args[0] = TMSS_NOWHERE;
					lines[i].args[1] = TMSL_TRIGGERER;
				}
				else if (lines[i].flags & ML_MIDSOLID)
				{
					lines[i].args[0] = TMSS_NOWHERE;
					lines[i].args[1] = TMSL_EVERYONE;
				}
				else if (lines[i].flags & ML_BLOCKMONSTERS)
				{
					lines[i].args[0] = TMSS_TRIGGERSECTOR;
					lines[i].args[1] = TMSL_EVERYONE;
				}
				else
				{
					lines[i].args[0] = TMSS_TRIGGERMOBJ;
					lines[i].args[1] = TMSL_EVERYONE;
				}
			}
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			break;
		case 415: //Run script
		{
			INT32 scrnum;

			lines[i].stringargs[0] = Z_Malloc(9, PU_LEVEL, NULL);
			strcpy(lines[i].stringargs[0], G_BuildMapName(gamemap));
			lines[i].stringargs[0][0] = 'S';
			lines[i].stringargs[0][1] = 'C';
			lines[i].stringargs[0][2] = 'R';

			scrnum = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (scrnum < 0 || scrnum > 999)
			{
				scrnum = 0;
				lines[i].stringargs[0][5] = lines[i].stringargs[0][6] = lines[i].stringargs[0][7] = '0';
			}
			else
			{
				lines[i].stringargs[0][5] = (char)('0' + (char)((scrnum / 100)));
				lines[i].stringargs[0][6] = (char)('0' + (char)((scrnum % 100) / 10));
				lines[i].stringargs[0][7] = (char)('0' + (char)(scrnum % 10));
			}
			lines[i].stringargs[0][8] = '\0';
			break;
		}
		case 416: //Start adjustable flickering light
		case 417: //Start adjustable pulsating light
		case 602: //Adjustable pulsating light
		case 603: //Adjustable flickering light
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = lines[i].frontsector->lightlevel;
			if ((lines[i].flags & ML_NOCLIMB) && lines[i].backsector)
				lines[i].args[4] = lines[i].backsector->lightlevel;
			else
				lines[i].args[3] = 1;
			break;
		case 418: //Start adjustable blinking light (unsynchronized)
		case 419: //Start adjustable blinking light (synchronized)
		case 604: //Adjustable blinking light (unsynchronized)
		case 605: //Adjustable blinking light (synchronized)
			lines[i].args[0] = tag;
			lines[i].args[1] = abs(lines[i].dx) >> FRACBITS;
			lines[i].args[2] = abs(lines[i].dy) >> FRACBITS;
			lines[i].args[3] = lines[i].frontsector->lightlevel;
			if ((lines[i].flags & ML_NOCLIMB) && lines[i].backsector)
				lines[i].args[5] = lines[i].backsector->lightlevel;
			else
				lines[i].args[4] |= TMB_USETARGET;
			if (lines[i].special % 2 == 1)
			{
				lines[i].args[4] |= TMB_SYNC;
				lines[i].special--;
			}
			break;
		case 420: //Fade light level
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_DONTPEGBOTTOM)
			{
				lines[i].args[1] = max(sides[lines[i].sidenum[0]].textureoffset >> FRACBITS, 0);
				// failsafe: if user specifies Back Y Offset and NOT Front Y Offset, use the Back Offset
				// to be consistent with other light and fade specials
				lines[i].args[2] = ((lines[i].sidenum[1] != 0xFFFF && !(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS)) ?
					max(min(sides[lines[i].sidenum[1]].rowoffset >> FRACBITS, 255), 0)
					: max(min(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS, 255), 0));
			}
			else
			{
				lines[i].args[1] = lines[i].frontsector->lightlevel;
				lines[i].args[2] = abs(P_AproxDistance(lines[i].dx, lines[i].dy)) >> FRACBITS;
			}
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMF_TICBASED;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMF_OVERRIDE;
			break;
		case 421: //Stop lighting effect
			lines[i].args[0] = tag;
			break;
		case 422: //Switch to cut-away view
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = (lines[i].flags & ML_NOCLIMB) ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : 0;
			break;
		case 423: //Change sky
		case 424: //Change weather
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 425: //Change object state
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			break;
		case 426: //Stop object
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 427: //Award score
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 428: //Start platform movement
			lines[i].args[0] = tag;
			lines[i].args[1] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[4] = (lines[i].flags & ML_NOCLIMB) ? 1 : 0;
			break;
		case 429: //Crush ceiling once
		case 430: //Crush floor once
		case 431: //Crush floor and ceiling once
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 429) ? TMP_CEILING : ((lines[i].special == 430) ? TMP_FLOOR : TMP_BOTH);
			if (lines[i].special == 430 || lines[i].flags & ML_MIDSOLID)
			{
				lines[i].args[2] = abs(lines[i].dx) >> FRACBITS;
				lines[i].args[3] = lines[i].args[2];
			}
			else
			{
				lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> (FRACBITS + 1);
				lines[i].args[3] = lines[i].args[2] / 4;
			}
			lines[i].special = 429;
			break;
		case 432: //Enable/disable 2D mode
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 433: //Enable/disable gravity flip
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].args[1] = !!(lines[i].flags & ML_SKEWTD);
			lines[i].args[2] = !!(lines[i].flags & ML_BLOCKMONSTERS);
			break;
		case 434: //Award power-up
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			if (lines[i].sidenum[1] != 0xffff && lines[i].flags & ML_BLOCKMONSTERS) // read power from back sidedef
			{
				lines[i].stringargs[1] = Z_Malloc(strlen(sides[lines[i].sidenum[1]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[1], sides[lines[i].sidenum[1]].text, strlen(sides[lines[i].sidenum[1]].text) + 1);
			}
			else
				P_WriteConstant((lines[i].flags & ML_NOCLIMB) ? -1 : (sides[lines[i].sidenum[0]].textureoffset >> FRACBITS), &lines[i].stringargs[1]);
			break;
		case 435: //Change plane scroller direction
			lines[i].args[0] = tag;
			lines[i].args[1] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			break;
		case 436: //Shatter FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 437: //Disable player control
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 438: //Change object size
			lines[i].args[0] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			break;
		case 439: //Change tagged linedef's textures
			lines[i].args[0] = tag;
			lines[i].args[1] = TMSD_FRONTBACK;
			lines[i].args[2] = !!(lines[i].flags & ML_NOCLIMB);
			lines[i].args[3] = !!(lines[i].flags & ML_EFFECT6);
			break;
		case 441: //Condition set trigger
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			break;
		case 442: //Change object type state
			lines[i].args[0] = tag;
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			if (lines[i].sidenum[1] == 0xffff)
				lines[i].args[1] = 1;
			else
			{
				lines[i].args[1] = 0;
				if (sides[lines[i].sidenum[1]].text)
				{
					lines[i].stringargs[1] = Z_Malloc(strlen(sides[lines[i].sidenum[1]].text) + 1, PU_LEVEL, NULL);
					M_Memcpy(lines[i].stringargs[1], sides[lines[i].sidenum[1]].text, strlen(sides[lines[i].sidenum[1]].text) + 1);
				}
			}
			break;
		case 443: //Call Lua function
			if (lines[i].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(lines[i].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], lines[i].text, strlen(lines[i].text) + 1);
			}
			else
				CONS_Alert(CONS_WARNING, "Linedef %s is missing the hook name of the Lua function to call! (This should be given in the front texture fields)\n", sizeu1(i));
			break;
		case 444: //Earthquake
			lines[i].args[0] = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 445: //Make FOF disappear/reappear
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 446: //Make FOF crumble
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMFR_NORETURN;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[2] |= TMFR_CHECKFLAG;
			break;
		case 447: //Change colormap
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[2] |= TMCF_RELATIVE;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[2] |= TMCF_SUBLIGHTR|TMCF_SUBFADER;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMCF_SUBLIGHTG|TMCF_SUBFADEG;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[2] |= TMCF_SUBLIGHTB|TMCF_SUBFADEB;
			break;
		case 448: //Change skybox
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if ((lines[i].flags & (ML_MIDSOLID|ML_BLOCKMONSTERS)) == ML_MIDSOLID) // Solid Midtexture is on but Block Enemies is off?
			{
				CONS_Alert(CONS_WARNING,
					M_GetText("Skybox switch linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"),
					tag);
				lines[i].special = 0;
				break;
			}
			else if ((lines[i].flags & (ML_MIDSOLID|ML_BLOCKMONSTERS)) == (ML_MIDSOLID|ML_BLOCKMONSTERS))
				lines[i].args[2] = TMS_CENTERPOINT;
			else if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[2] = TMS_BOTH;
			else
				lines[i].args[2] = TMS_VIEWPOINT;
			lines[i].args[3] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 449: //Enable bosses with parameters
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 450: //Execute linedef executor (specific tag)
			lines[i].args[0] = tag;
			break;
		case 451: //Execute linedef executor (random tag in range)
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 452: //Set FOF translucency
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = lines[i].sidenum[1] != 0xffff ? (sides[lines[i].sidenum[1]].textureoffset >> FRACBITS) : (P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS);
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] |= TMST_RELATIVE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMST_DONTDOTRANSLUCENT;
			break;
		case 453: //Fade FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = lines[i].sidenum[1] != 0xffff ? (sides[lines[i].sidenum[1]].textureoffset >> FRACBITS) : (lines[i].dx >> FRACBITS);
			lines[i].args[3] = lines[i].sidenum[1] != 0xffff ? (sides[lines[i].sidenum[1]].rowoffset >> FRACBITS) : (abs(lines[i].dy) >> FRACBITS);
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[4] |= TMFT_RELATIVE;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[4] |= TMFT_OVERRIDE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMFT_TICBASED;
			if (lines[i].flags & ML_BOUNCY)
				lines[i].args[4] |= TMFT_IGNORECOLLISION;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= TMFT_GHOSTFADE;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[4] |= TMFT_DONTDOTRANSLUCENT;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[4] |= TMFT_DONTDOEXISTS;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[4] |= (TMFT_DONTDOLIGHTING|TMFT_DONTDOCOLORMAP);
			if (lines[i].flags & ML_TFERLINE)
				lines[i].args[4] |= TMFT_USEEXACTALPHA;
			break;
		case 454: //Stop fading FOF
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = !!(lines[i].flags & ML_BLOCKMONSTERS);
			break;
		case 455: //Fade colormap
		{
			INT32 speed = (INT32)((((lines[i].flags & ML_DONTPEGBOTTOM) || !sides[lines[i].sidenum[0]].rowoffset) && lines[i].sidenum[1] != 0xFFFF) ?
				abs(sides[lines[i].sidenum[1]].rowoffset >> FRACBITS)
				: abs(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS));

			lines[i].args[0] = tag;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[2] = speed;
			else
				lines[i].args[2] = (256 + speed - 1)/speed;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] |= TMCF_RELATIVE;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMCF_SUBLIGHTR|TMCF_SUBFADER;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMCF_SUBLIGHTG|TMCF_SUBFADEG;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[3] |= TMCF_SUBLIGHTB|TMCF_SUBFADEB;
			if (lines[i].flags & ML_BOUNCY)
				lines[i].args[3] |= TMCF_FROMBLACK;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMCF_OVERRIDE;
			break;
		}
		case 456: //Stop fading colormap
			lines[i].args[0] = tag;
			break;
		case 457: //Track object's angle
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = (lines[i].sidenum[1] != 0xffff) ? sides[lines[i].sidenum[1]].textureoffset >> FRACBITS : 0;
			lines[i].args[4] = !!(lines[i].flags & ML_NOSKEW);
			break;
		case 459: //Control text prompt
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_BLOCKMONSTERS)
				lines[i].args[2] |= TMP_CLOSE;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[2] |= TMP_RUNPOSTEXEC;
			if (lines[i].flags & ML_TFERLINE)
				lines[i].args[2] |= TMP_CALLBYNAME;
			if (lines[i].flags & ML_NOSKEW)
				lines[i].args[2] |= TMP_KEEPCONTROLS;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[2] |= TMP_KEEPREALTIME;
			/*if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[2] |= TMP_ALLPLAYERS;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[2] |= TMP_FREEZETHINKERS;*/
			lines[i].args[3] = (lines[i].sidenum[1] != 0xFFFF) ? sides[lines[i].sidenum[1]].textureoffset >> FRACBITS : tag;
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			break;
		case 460: //Award rings
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			break;
		case 461: //Spawn object
			lines[i].args[0] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[1] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[2] = lines[i].frontsector->floorheight >> FRACBITS;
			lines[i].args[3] = (lines[i].flags & ML_SKEWTD) ? AngleFixed(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y)) >> FRACBITS : 0;
			if (lines[i].flags & ML_NOCLIMB)
			{
				if (lines[i].sidenum[1] != 0xffff) // Make sure the linedef has a back side
				{
					lines[i].args[4] = 1;
					lines[i].args[5] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
					lines[i].args[6] = sides[lines[i].sidenum[1]].rowoffset >> FRACBITS;
					lines[i].args[7] = lines[i].frontsector->ceilingheight >> FRACBITS;
				}
				else
				{
					CONS_Alert(CONS_WARNING, "Linedef Type %d - Spawn Object: Linedef is set for random range but has no back side.\n", lines[i].special);
					lines[i].args[4] = 0;
				}
			}
			else
				lines[i].args[4] = 0;
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			break;
		case 463: //Dye object
			if (sides[lines[i].sidenum[0]].text)
			{
				lines[i].stringargs[0] = Z_Malloc(strlen(sides[lines[i].sidenum[0]].text) + 1, PU_LEVEL, NULL);
				M_Memcpy(lines[i].stringargs[0], sides[lines[i].sidenum[0]].text, strlen(sides[lines[i].sidenum[0]].text) + 1);
			}
			break;
		case 464: //Trigger egg capsule
			lines[i].args[0] = tag;
			lines[i].args[1] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 466: //Set level failure state
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 467: //Set light level
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = TML_SECTOR;
			lines[i].args[3] = !!(lines[i].flags & ML_MIDPEG);
			break;
		case 480: //Polyobject - door slide
		case 481: //Polyobject - door move
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].sidenum[1] != 0xffff)
				lines[i].args[3] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
			break;
		case 482: //Polyobject - move
		case 483: //Polyobject - move, override
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			lines[i].args[3] = lines[i].special == 483;
			lines[i].special = 482;
			break;
		case 484: //Polyobject - rotate right
		case 485: //Polyobject - rotate right, override
		case 486: //Polyobject - rotate left
		case 487: //Polyobject - rotate left, override
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].args[2] == 360)
				lines[i].args[3] |= TMPR_CONTINUOUS;
			else if (lines[i].args[2] == 0)
				lines[i].args[2] = 360;
			if (lines[i].special < 486)
				lines[i].args[2] *= -1;
			if (lines[i].flags & ML_NOCLIMB)
				lines[i].args[3] |= TMPR_DONTROTATEOTHERS;
			else if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMPR_ROTATEPLAYERS;
			if (lines[i].special % 2 == 1)
				lines[i].args[3] |= TMPR_OVERRIDE;
			lines[i].special = 484;
			break;
		case 488: //Polyobject - move by waypoints
			lines[i].args[0] = tag;
			lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] = PWR_WRAP;
			else if (lines[i].flags & ML_NOSKEW)
				lines[i].args[3] = PWR_COMEBACK;
			else
				lines[i].args[3] = PWR_STOP;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[4] |= PWF_REVERSE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= PWF_LOOP;
			break;
		case 489: //Polyobject - turn invisible, intangible
		case 490: //Polyobject - turn visible, tangible
			lines[i].args[0] = tag;
			lines[i].args[1] = 491 - lines[i].special;
			if (!(lines[i].flags & ML_NOCLIMB))
				lines[i].args[2] = lines[i].args[1];
			lines[i].special = 489;
			break;
		case 491: //Polyobject - set translucency
			lines[i].args[0] = tag;
			// If Front X Offset is specified, use that. Else, use floorheight.
			lines[i].args[1] = (sides[lines[i].sidenum[0]].textureoffset ? sides[lines[i].sidenum[0]].textureoffset : lines[i].frontsector->floorheight) >> FRACBITS;
			// If DONTPEGBOTTOM, specify raw translucency value. Else, take it out of 1000.
			if (!(lines[i].flags & ML_DONTPEGBOTTOM))
				lines[i].args[1] /= 100;
			lines[i].args[2] = !!(lines[i].flags & ML_MIDPEG);
			break;
		case 492: //Polyobject - fade translucency
			lines[i].args[0] = tag;
			// If Front X Offset is specified, use that. Else, use floorheight.
			lines[i].args[1] = (sides[lines[i].sidenum[0]].textureoffset ? sides[lines[i].sidenum[0]].textureoffset : lines[i].frontsector->floorheight) >> FRACBITS;
			// If DONTPEGBOTTOM, specify raw translucency value. Else, take it out of 1000.
			if (!(lines[i].flags & ML_DONTPEGBOTTOM))
				lines[i].args[1] /= 100;
			// allow Back Y Offset to be consistent with other fade specials
			lines[i].args[2] = (lines[i].sidenum[1] != 0xffff && !sides[lines[i].sidenum[0]].rowoffset) ?
				abs(sides[lines[i].sidenum[1]].rowoffset >> FRACBITS)
				: abs(sides[lines[i].sidenum[0]].rowoffset >> FRACBITS);
			if (lines[i].flags & ML_MIDPEG)
				lines[i].args[3] |= TMPF_RELATIVE;
			if (lines[i].flags & ML_WRAPMIDTEX)
				lines[i].args[3] |= TMPF_OVERRIDE;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[3] |= TMPF_TICBASED;
			if (lines[i].flags & ML_BOUNCY)
				lines[i].args[3] |= TMPF_IGNORECOLLISION;
			if (lines[i].flags & ML_SKEWTD)
				lines[i].args[3] |= TMPF_GHOSTFADE;
			break;
		case 500: //Scroll front wall left
		case 501: //Scroll front wall right
			lines[i].args[0] = 0;
			lines[i].args[1] = (lines[i].special == 500) ? -1 : 1;
			lines[i].args[2] = 0;
			lines[i].special = 500;
			break;
		case 502: //Scroll tagged wall
		case 503: //Scroll tagged wall (accelerative)
		case 504: //Scroll tagged wall (displacement)
			lines[i].args[0] = tag;
			if (lines[i].flags & ML_MIDPEG)
			{
				if (lines[i].sidenum[1] == 0xffff)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line special %d (line #%s) missing back side!\n", lines[i].special, sizeu1(i));
					lines[i].special = 0;
					break;
				}
				lines[i].args[1] = 1;
			}
			else
				lines[i].args[1] = 0;
			if (lines[i].flags & ML_NOSKEW)
			{
				lines[i].args[2] = sides[lines[i].sidenum[0]].textureoffset >> (FRACBITS - SCROLL_SHIFT);
				lines[i].args[3] = sides[lines[i].sidenum[0]].rowoffset >> (FRACBITS - SCROLL_SHIFT);
			}
			else
			{
				lines[i].args[2] = lines[i].dx >> FRACBITS;
				lines[i].args[3] = lines[i].dy >> FRACBITS;
			}
			lines[i].args[4] = lines[i].special - 502;
			lines[i].special = 502;
			break;
		case 505: //Scroll front wall by front side offsets
		case 506: //Scroll front wall by back side offsets
		case 507: //Scroll back wall by front side offsets
		case 508: //Scroll back wall by back side offsets
			lines[i].args[0] = lines[i].special >= 507;
			if (lines[i].special % 2 == 0)
			{
				if (lines[i].sidenum[1] == 0xffff)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line special %d (line #%s) missing back side!\n", lines[i].special, sizeu1(i));
					lines[i].special = 0;
					break;
				}
				lines[i].args[1] = sides[lines[i].sidenum[1]].textureoffset >> FRACBITS;
				lines[i].args[2] = sides[lines[i].sidenum[1]].rowoffset >> FRACBITS;
			}
			else
			{
				lines[i].args[1] = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
				lines[i].args[2] = sides[lines[i].sidenum[0]].rowoffset >> FRACBITS;
			}
			lines[i].special = 500;
			break;
		case 510: //Scroll floor texture
		case 511: //Scroll floor texture (accelerative)
		case 512: //Scroll floor texture (displacement)
		case 513: //Scroll ceiling texture
		case 514: //Scroll ceiling texture (accelerative)
		case 515: //Scroll ceiling texture (displacement)
		case 520: //Carry objects on floor
		case 521: //Carry objects on floor (accelerative)
		case 522: //Carry objects on floor (displacement)
		case 523: //Carry objects on ceiling
		case 524: //Carry objects on ceiling (accelerative)
		case 525: //Carry objects on ceiling (displacement)
		case 530: //Scroll floor texture and carry objects
		case 531: //Scroll floor texture and carry objects (accelerative)
		case 532: //Scroll floor texture and carry objects (displacement)
		case 533: //Scroll ceiling texture and carry objects
		case 534: //Scroll ceiling texture and carry objects (accelerative)
		case 535: //Scroll ceiling texture and carry objects (displacement)
			lines[i].args[0] = tag;
			lines[i].args[1] = ((lines[i].special % 10) < 3) ? TMP_FLOOR : TMP_CEILING;
			lines[i].args[2] = ((lines[i].special - 510)/10 + 1) % 3;
			lines[i].args[3] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
			lines[i].args[4] = (lines[i].special % 10) % 3;
			if (lines[i].args[2] != TMS_SCROLLONLY && !(lines[i].flags & ML_NOCLIMB))
				lines[i].args[4] |= TMST_NONEXCLUSIVE;
			lines[i].special = 510;
			break;
		case 540: //Floor friction
		{
			INT32 s;
			fixed_t strength; // friction value of sector
			fixed_t friction; // friction value to be applied during movement

			strength = sides[lines[i].sidenum[0]].textureoffset >> FRACBITS;
			if (strength > 0) // sludge
				strength = strength*2; // otherwise, the maximum sludginess value is +967...

			// The following might seem odd. At the time of movement,
			// the move distance is multiplied by 'friction/0x10000', so a
			// higher friction value actually means 'less friction'.
			friction = ORIG_FRICTION - (0x1EB8*strength)/0x80; // ORIG_FRICTION is 0xE800

			TAG_ITER_SECTORS(tag, s)
				sectors[s].friction = friction;
			break;
		}
		case 541: //Wind
		case 542: //Upwards wind
		case 543: //Downwards wind
		case 544: //Current
		case 545: //Upwards current
		case 546: //Downwards current
			lines[i].args[0] = tag;
			switch ((lines[i].special - 541) % 3)
			{
				case 0:
					lines[i].args[1] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
					break;
				case 1:
					lines[i].args[2] = R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
					break;
				case 2:
					lines[i].args[2] = -R_PointToDist2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y) >> FRACBITS;
					break;
			}
			lines[i].args[3] = (lines[i].special >= 544) ? p_current : p_wind;
			if (lines[i].flags & ML_MIDSOLID)
				lines[i].args[4] |= TMPF_SLIDE;
			if (!(lines[i].flags & ML_NOCLIMB))
				lines[i].args[4] |= TMPF_NONEXCLUSIVE;
			lines[i].special = 541;
			break;
		case 600: //Floor lighting
		case 601: //Ceiling lighting
			lines[i].args[0] = tag;
			lines[i].args[1] = (lines[i].special == 601) ? TMP_CEILING : TMP_FLOOR;
			lines[i].special = 600;
			break;
		case 606: //Colormap
			lines[i].args[0] = tag;
			break;
		case 700: //Slope front sector floor
		case 701: //Slope front sector ceiling
		case 702: //Slope front sector floor and ceiling
		case 703: //Slope front sector floor and back sector ceiling
		case 710: //Slope back sector floor
		case 711: //Slope back sector ceiling
		case 712: //Slope back sector floor and ceiling
		case 713: //Slope back sector floor and front sector ceiling
		{
			boolean frontfloor = (lines[i].special == 700 || lines[i].special == 702 || lines[i].special == 703);
			boolean backfloor = (lines[i].special == 710 || lines[i].special == 712 || lines[i].special == 713);
			boolean frontceil = (lines[i].special == 701 || lines[i].special == 702 || lines[i].special == 713);
			boolean backceil = (lines[i].special == 711 || lines[i].special == 712 || lines[i].special == 703);

			lines[i].args[0] = backfloor ? TMS_BACK : (frontfloor ? TMS_FRONT : TMS_NONE);
			lines[i].args[1] = backceil ? TMS_BACK : (frontceil ? TMS_FRONT : TMS_NONE);

			if (lines[i].flags & ML_NETONLY)
				lines[i].args[2] |= TMSL_NOPHYSICS;
			if (lines[i].flags & ML_NONET)
				lines[i].args[2] |= TMSL_DYNAMIC;
			if (lines[i].flags & ML_TFERLINE)
				lines[i].args[2] |= TMSL_COPY;

			lines[i].special = 700;
			break;
		}
		case 704: //Slope front sector floor by 3 tagged vertices
		case 705: //Slope front sector ceiling by 3 tagged vertices
		case 714: //Slope back sector floor by 3 tagged vertices
		case 715: //Slope back sector ceiling  by 3 tagged vertices
		{
			if (lines[i].special == 704)
				lines[i].args[0] = TMSP_FRONTFLOOR;
			else if (lines[i].special == 705)
				lines[i].args[0] = TMSP_FRONTCEILING;
			else if (lines[i].special == 714)
				lines[i].args[0] = TMSP_BACKFLOOR;
			else if (lines[i].special == 715)
				lines[i].args[0] = TMSP_BACKCEILING;

			lines[i].args[1] = tag;

			if (lines[i].flags & ML_EFFECT6)
			{
				UINT8 side = lines[i].special >= 714;

				if (side == 1 && lines[i].sidenum[1] == 0xffff)
					CONS_Debug(DBG_GAMELOGIC, "P_ConvertBinaryMap: Line special %d (line #%s) missing 2nd side!\n", lines[i].special, sizeu1(i));
				else
				{
					lines[i].args[2] = sides[lines[i].sidenum[side]].textureoffset >> FRACBITS;
					lines[i].args[3] = sides[lines[i].sidenum[side]].rowoffset >> FRACBITS;
				}
			}
			else
			{
				lines[i].args[2] = lines[i].args[1];
				lines[i].args[3] = lines[i].args[1];
			}

			if (lines[i].flags & ML_NETONLY)
				lines[i].args[4] |= TMSL_NOPHYSICS;
			if (lines[i].flags & ML_NONET)
				lines[i].args[4] |= TMSL_DYNAMIC;

			lines[i].special = 704;
			break;
		}
		case 720: //Copy front side floor slope
		case 721: //Copy front side ceiling slope
		case 722: //Copy front side floor and ceiling slope
			if (lines[i].special != 721)
				lines[i].args[0] = tag;
			if (lines[i].special != 720)
				lines[i].args[1] = tag;
			lines[i].special = 720;
			break;
		case 723: //Copy back side floor slope
		case 724: //Copy back side ceiling slope
		case 725: //Copy back side floor and ceiling slope
			if (lines[i].special != 724)
				lines[i].args[2] = tag;
			if (lines[i].special != 723)
				lines[i].args[3] = tag;
			lines[i].special = 720;
			break;
		case 730: //Copy front side floor slope to back side
		case 731: //Copy front side ceiling slope to back side
		case 732: //Copy front side floor and ceiling slope to back side
			if (lines[i].special != 731)
				lines[i].args[4] |= TMSC_FRONTTOBACKFLOOR;
			if (lines[i].special != 730)
				lines[i].args[4] |= TMSC_FRONTTOBACKCEILING;
			lines[i].special = 720;
			break;
		case 733: //Copy back side floor slope to front side
		case 734: //Copy back side ceiling slope to front side
		case 735: //Copy back side floor and ceiling slope to front side
			if (lines[i].special != 734)
				lines[i].args[4] |= TMSC_BACKTOFRONTFLOOR;
			if (lines[i].special != 733)
				lines[i].args[4] |= TMSC_BACKTOFRONTCEILING;
			lines[i].special = 720;
			break;
		case 799: //Set dynamic slope vertex to front sector height
			lines[i].args[0] = !!(lines[i].flags & ML_NOCLIMB);
			break;
		case 909: //Fog wall
			lines[i].blendmode = AST_FOG;
			break;
		default:
			break;
		}

		// Set alpha for translucent walls
		if (lines[i].special >= 900 && lines[i].special < 909)
			lines[i].alpha = ((909 - lines[i].special) << FRACBITS)/10;

		// Set alpha for additive/subtractive/reverse subtractive walls
		if (lines[i].special >= 910 && lines[i].special <= 939)
			lines[i].alpha = ((10 - lines[i].special % 10) << FRACBITS)/10;

		if (lines[i].special >= 910 && lines[i].special <= 919) // additive
			lines[i].blendmode = AST_ADD;

		if (lines[i].special >= 920 && lines[i].special <= 929) // subtractive
			lines[i].blendmode = AST_SUBTRACT;

		if (lines[i].special >= 930 && lines[i].special <= 939) // reverse subtractive
			lines[i].blendmode = AST_REVERSESUBTRACT;

		if (lines[i].special == 940) // modulate
			lines[i].blendmode = AST_MODULATE;

		//Linedef executor delay
		if (lines[i].special >= 400 && lines[i].special < 500)
		{
			//Dummy value to indicate that this executor is delayed.
			//The real value is taken from the back sector at runtime.
			if (lines[i].flags & ML_DONTPEGTOP)
				lines[i].executordelay = 1;
		}
	}
}

static void P_ConvertBinarySectorTypes(void)
{
	size_t i;

	for (i = 0; i < numsectors; i++)
	{
		mtag_t tag = Tag_FGet(&sectors[i].tags);

		switch(GETSECSPECIAL(sectors[i].special, 1))
		{
			case 1: //Damage
				sectors[i].damagetype = SD_GENERIC;
				break;
			case 2: //Damage (Water)
				sectors[i].damagetype = SD_WATER;
				break;
			case 3: //Damage (Fire)
			{
				size_t j;
				boolean isLava = false;

				for (j = 0; j < sectors[i].linecount; j++)
				{
					line_t *line = sectors[i].lines[j];

					if (line->frontsector != &sectors[i])
						continue;

					if (line->flags & ML_BLOCKMONSTERS)
						continue;

					if (line->special == 120 || (line->special == 259 && (line->args[2] & FOF_SWIMMABLE)))
					{
						isLava = true;
						break;
					}
				}
				sectors[i].damagetype = isLava ? SD_LAVA : SD_FIRE;
				break;
			}
			case 4: //Damage (Electric)
				sectors[i].damagetype = SD_ELECTRIC;
				break;
			case 5: //Spikes
				sectors[i].damagetype = SD_SPIKE;
				break;
			case 6: //Death pit (camera tilt)
				sectors[i].damagetype = SD_DEATHPITTILT;
				break;
			case 7: //Death pit (no camera tilt)
				sectors[i].damagetype = SD_DEATHPITNOTILT;
				break;
			case 8: //Instakill
				sectors[i].damagetype = SD_INSTAKILL;
				break;
			case 11: //Special stage damage
				sectors[i].damagetype = SD_SPECIALSTAGE;
				break;
			case 12: //Space countdown
				sectors[i].specialflags |= SSF_OUTERSPACE;
				break;
			case 13: //Ramp sector
				sectors[i].specialflags |= SSF_DOUBLESTEPUP;
				break;
			case 14: //Non-ramp sector
				sectors[i].specialflags |= SSF_NOSTEPDOWN;
				break;
			case 15: //Bouncy FOF
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated bouncy FOF sector type detected. Please use linedef type 76 instead.\n"));
				break;
			default:
				break;
		}

		switch(GETSECSPECIAL(sectors[i].special, 2))
		{
			case 1: //Trigger linedef executor (pushable objects)
				sectors[i].triggertag = tag;
				sectors[i].flags |= MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_MOBJ;
				break;
			case 2: //Trigger linedef executor (Anywhere in sector, all players)
				sectors[i].triggertag = tag;
				sectors[i].flags &= ~MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_ALLPLAYERS;
				break;
			case 3: //Trigger linedef executor (Floor touch, all players)
				sectors[i].triggertag = tag;
				sectors[i].flags |= MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_ALLPLAYERS;
				break;
			case 4: //Trigger linedef executor (Anywhere in sector)
				sectors[i].triggertag = tag;
				sectors[i].flags &= ~MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_PLAYER;
				break;
			case 5: //Trigger linedef executor (Floor touch)
				sectors[i].triggertag = tag;
				sectors[i].flags |= MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_PLAYER;
				break;
			case 6: //Trigger linedef executor (Emerald check)
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated emerald check sector type detected. Please use linedef types 337-339 instead.\n"));
				sectors[i].triggertag = tag;
				sectors[i].flags &= ~MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_PLAYEREMERALDS;
				break;
			case 7: //Trigger linedef executor (NiGHTS mare)
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated NiGHTS mare sector type detected. Please use linedef types 340-342 instead.\n"));
				sectors[i].triggertag = tag;
				sectors[i].flags &= ~MSF_TRIGGERLINE_PLANE;
				sectors[i].triggerer = TO_PLAYERNIGHTS;
				break;
			case 8: //Check for linedef executor on FOFs
				sectors[i].flags |= MSF_TRIGGERLINE_MOBJ;
				break;
			case 10: //Special stage time/spheres requirements
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated sector type for special stage requirements detected. Please use the SpecialStageTime and SpecialStageSpheres level header options instead.\n"));
				break;
			case 11: //Custom global gravity
				CONS_Alert(CONS_WARNING, M_GetText("Deprecated sector type for global gravity detected. Please use the Gravity level header option instead.\n"));
				break;
			default:
				break;
		}

		switch(GETSECSPECIAL(sectors[i].special, 3))
		{
			case 5: //Speed pad
				sectors[i].specialflags |= SSF_SPEEDPAD;
				break;
			case 6: //Gravity flip on jump (think VVVVVV)
				sectors[i].specialflags |= SSF_JUMPFLIP;
				break;
			default:
				break;
		}

		switch(GETSECSPECIAL(sectors[i].special, 4))
		{
			case 1: //Star post activator
				sectors[i].specialflags |= SSF_STARPOSTACTIVATOR;
				break;
			case 2: //Exit/Special Stage pit/Return flag
				sectors[i].specialflags |= SSF_EXIT|SSF_SPECIALSTAGEPIT|SSF_RETURNFLAG;
				break;
			case 3: //Red team base
				sectors[i].specialflags |= SSF_REDTEAMBASE;
				break;
			case 4: //Blue team base
				sectors[i].specialflags |= SSF_BLUETEAMBASE;
				break;
			case 5: //Fan sector
				sectors[i].specialflags |= SSF_FAN;
				break;
			case 6: //Super Sonic transform
				sectors[i].specialflags |= SSF_SUPERTRANSFORM;
				break;
			case 7: //Force spin
				sectors[i].specialflags |= SSF_FORCESPIN;
				break;
			case 8: //Zoom tube start
				sectors[i].specialflags |= SSF_ZOOMTUBESTART;
				break;
			case 9: //Zoom tube end
				sectors[i].specialflags |= SSF_ZOOMTUBEEND;
				break;
			case 10: //Circuit finish line
				sectors[i].specialflags |= SSF_FINISHLINE;
				break;
			case 11: //Rope hang
				sectors[i].specialflags |= SSF_ROPEHANG;
				break;
			case 12: //Intangible to the camera
				sectors[i].flags |= MSF_NOCLIPCAMERA;
				break;
			default:
				break;
		}
	}
}

static void P_ConvertBinaryThingTypes(void)
{
	size_t i;
	mobjtype_t mobjtypeofthing[4096] = {0};
	mobjtype_t mobjtype;

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
		if (mobjinfo[i].doomednum < 0 || mobjinfo[i].doomednum >= 4096)
			continue;

		mobjtypeofthing[mobjinfo[i].doomednum] = (mobjtype_t)i;
	}

	for (i = 0; i < nummapthings; i++)
	{
		mobjtype = mobjtypeofthing[mapthings[i].type];
		if (mobjtype)
		{
			if (mobjinfo[mobjtype].flags & MF_BOSS)
			{
				INT32 paramoffset = mapthings[i].extrainfo*LE_PARAMWIDTH;
				mapthings[i].args[0] = mapthings[i].extrainfo;
				mapthings[i].args[1] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
				mapthings[i].args[2] = LE_BOSSDEAD + paramoffset;
				mapthings[i].args[3] = LE_ALLBOSSESDEAD + paramoffset;
				mapthings[i].args[4] = LE_PINCHPHASE + paramoffset;
			}
			if (mobjinfo[mobjtype].flags & MF_NIGHTSITEM)
			{
				if (mapthings[i].options & MTF_OBJECTSPECIAL)
					mapthings[i].args[0] |= TMNI_BONUSONLY;
				if (mapthings[i].options & MTF_AMBUSH)
					mapthings[i].args[0] |= TMNI_REVEAL;
			}
			if (mobjinfo[mobjtype].flags & MF_PUSHABLE)
			{
				if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_AMBUSH)) == (MTF_OBJECTSPECIAL|MTF_AMBUSH))
					mapthings[i].args[0] = TMP_CLASSIC;
				else if (mapthings[i].options & MTF_OBJECTSPECIAL)
					mapthings[i].args[0] = TMP_SLIDE;
				else if (mapthings[i].options & MTF_AMBUSH)
					mapthings[i].args[0] = TMP_IMMOVABLE;
				else
					mapthings[i].args[0] = TMP_NORMAL;
			}
			if ((mobjinfo[mobjtype].flags & MF_SPRING) && mobjinfo[mobjtype].painchance == 3)
				mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			if (mobjinfo[mobjtype].flags & MF_MONITOR)
			{
				if ((mapthings[i].options & MTF_EXTRA) && mapthings[i].angle & 16384)
					mapthings[i].args[0] = mapthings[i].angle & 16383;

				if (mobjinfo[mobjtype].speed != 0)
				{
					if (mapthings[i].options & MTF_OBJECTSPECIAL)
						mapthings[i].args[1] = TMMR_STRONG;
					else if (mapthings[i].options & MTF_AMBUSH)
						mapthings[i].args[1] = TMMR_WEAK;
					else
						mapthings[i].args[1] = TMMR_SAME;
				}
			}
		}

		if (mapthings[i].type >= 1 && mapthings[i].type <= 35) //Player starts
		{
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			continue;
		}
		else if (mapthings[i].type >= 2200 && mapthings[i].type <= 2217) //Flickies
		{
			mapthings[i].args[0] = mapthings[i].angle;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].args[1] |= TMFF_AIMLESS;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[1] |= TMFF_STATIONARY;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[1] |= TMFF_HOP;
			if (mapthings[i].type == 2207)
				mapthings[i].args[2] = mapthings[i].extrainfo;
			continue;
		}

		switch (mapthings[i].type)
		{
		case 102: //SDURF
		case 1805: //Puma
			mapthings[i].args[0] = mapthings[i].angle;
			break;
		case 110: //THZ Turret
			mapthings[i].args[0] = LE_TURRET;
			break;
		case 111: //Pop-up Turret
			mapthings[i].args[0] = mapthings[i].angle;
			break;
		case 103: //Buzz (Gold)
		case 104: //Buzz (Red)
		case 105: //Jetty-syn Bomber
		case 106: //Jetty-syn Gunner
		case 117: //Robo-Hood
		case 126: //Crushstacean
		case 128: //Bumblebore
		case 132: //Cacolantern
		case 138: //Banpyura
		case 1602: //Pian
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 119: //Egg Guard
			if ((mapthings[i].options & (MTF_EXTRA|MTF_OBJECTSPECIAL)) == MTF_OBJECTSPECIAL)
				mapthings[i].args[0] = TMGD_LEFT;
			else if ((mapthings[i].options & (MTF_EXTRA|MTF_OBJECTSPECIAL)) == MTF_EXTRA)
				mapthings[i].args[0] = TMGD_RIGHT;
			else
				mapthings[i].args[0] = TMGD_BACK;
			mapthings[i].args[1] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 127: //Hive Elemental
			mapthings[i].args[0] = mapthings[i].extrainfo;
			break;
		case 135: //Pterabyte Spawner
			mapthings[i].args[0] = mapthings[i].extrainfo + 1;
			break;
		case 136: //Pyre Fly
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 201: //Egg Slimer
			mapthings[i].args[5] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 203: //Egg Colosseum
			mapthings[i].args[5] = LE_BOSS4DROP + mapthings[i].extrainfo * LE_PARAMWIDTH;
			break;
		case 204: //Fang
			mapthings[i].args[4] = LE_BOSS4DROP + mapthings[i].extrainfo*LE_PARAMWIDTH;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].args[5] |= TMF_GRAYSCALE;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[5] |= TMF_SKIPINTRO;
			break;
		case 206: //Brak Eggman (Old)
			mapthings[i].args[5] = LE_BRAKPLATFORM + mapthings[i].extrainfo*LE_PARAMWIDTH;
			break;
		case 207: //Metal Sonic (Race)
		case 2104: //Amy Cameo
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 208: //Metal Sonic (Battle)
			mapthings[i].args[5] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 209: //Brak Eggman
			mapthings[i].args[5] = LE_BRAKVILEATACK + mapthings[i].extrainfo*LE_PARAMWIDTH;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].args[6] |= TMB_NODEATHFLING;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[6] |= TMB_BARRIER;
			break;
		case 292: //Boss waypoint
			mapthings[i].args[0] = mapthings[i].angle;
			mapthings[i].args[1] = mapthings[i].options & 7;
			break;
		case 294: //Fang waypoint
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 300: //Ring
		case 301: //Bounce ring
		case 302: //Rail ring
		case 303: //Infinity ring
		case 304: //Automatic ring
		case 305: //Explosion ring
		case 306: //Scatter ring
		case 307: //Grenade ring
		case 308: //Red team ring
		case 309: //Blue team ring
		case 312: //Emerald token
		case 320: //Emerald hunt location
		case 321: //Match chaos emerald spawn
		case 330: //Bounce ring panel
		case 331: //Rail ring panel
		case 332: //Automatic ring panel
		case 333: //Explosion ring panel
		case 334: //Scatter ring panel
		case 335: //Grenade ring panel
		case 520: //Bomb sphere
		case 521: //Spikeball
		case 1706: //Blue sphere
		case 1800: //Coin
			mapthings[i].args[0] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 322: //Emblem
			mapthings[i].args[1] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 409: //Extra life monitor
			mapthings[i].args[2] = !(mapthings[i].options & (MTF_AMBUSH|MTF_OBJECTSPECIAL));
			break;
		case 500: //Air bubble patch
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 502: //Star post
			if (mapthings[i].extrainfo)
				// Allow thing Parameter to define star post num too!
				// For starposts above param 15 (the 16th), add 360 to the angle like before and start parameter from 1 (NOT 0)!
				// So the 16th starpost is angle=0 param=15, the 17th would be angle=360 param=1.
				// This seems more intuitive for mappers to use, since most SP maps won't have over 16 consecutive star posts.
				mapthings[i].args[0] = mapthings[i].extrainfo + (mapthings[i].angle/360) * 15;
			else
				// Old behavior if Parameter is 0; add 360 to the angle for each consecutive star post.
				mapthings[i].args[0] = (mapthings[i].angle/360);
			mapthings[i].args[1] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 522: //Wall spike
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				mapthings[i].args[0] = mobjinfo[MT_WALLSPIKE].speed + mapthings[i].angle/360;
				mapthings[i].args[1] = (16 - mapthings[i].extrainfo) * mapthings[i].args[0]/16;
				if (mapthings[i].options & MTF_EXTRA)
					mapthings[i].args[2] |= TMSF_RETRACTED;
			}
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[2] |= TMSF_INTANGIBLE;
			break;
		case 523: //Spike
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
			{
				mapthings[i].args[0] = mobjinfo[MT_SPIKE].speed + mapthings[i].angle;
				mapthings[i].args[1] = (16 - mapthings[i].extrainfo) * mapthings[i].args[0]/16;
				if (mapthings[i].options & MTF_EXTRA)
					mapthings[i].args[2] |= TMSF_RETRACTED;
			}
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[2] |= TMSF_INTANGIBLE;
			break;
		case 540: //Fan
			mapthings[i].args[0] = mapthings[i].angle;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[1] |= TMF_INVISIBLE;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[1] |= TMF_NODISTANCECHECK;
			break;
		case 541: //Gas jet
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 543: //Balloon
			if (mapthings[i].angle > 0)
				P_WriteConstant(((mapthings[i].angle - 1) % (numskincolors - 1)) + 1, &mapthings[i].stringargs[0]);
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 555: //Diagonal yellow spring
		case 556: //Diagonal red spring
		case 557: //Diagonal blue spring
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[0] |= TMDS_NOGRAVITY;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[0] |= TMDS_ROTATEEXTRA;
			break;
		case 558: //Horizontal yellow spring
		case 559: //Horizontal red spring
		case 560: //Horizontal blue spring
			mapthings[i].args[0] = !(mapthings[i].options & MTF_AMBUSH);
			break;
		case 700: //Water ambience A
		case 701: //Water ambience B
		case 702: //Water ambience C
		case 703: //Water ambience D
		case 704: //Water ambience E
		case 705: //Water ambience F
		case 706: //Water ambience G
		case 707: //Water ambience H
			mapthings[i].args[0] = 35;
			P_WriteConstant(sfx_amwtr1 + mapthings[i].type - 700, &mapthings[i].stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 708: //Disco ambience
			mapthings[i].args[0] = 512;
			P_WriteConstant(sfx_ambint, &mapthings[i].stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 709: //Volcano ambience
			mapthings[i].args[0] = 220;
			P_WriteConstant(sfx_ambin2, &mapthings[i].stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 710: //Machine ambience
			mapthings[i].args[0] = 24;
			P_WriteConstant(sfx_ambmac, &mapthings[i].stringargs[0]);
			mapthings[i].type = 700;
			break;
		case 750: //Slope vertex
			mapthings[i].args[0] = mapthings[i].extrainfo;
			break;
		case 753: //Zoom tube waypoint
			mapthings[i].args[0] = mapthings[i].angle >> 8;
			mapthings[i].args[1] = mapthings[i].angle & 255;
			break;
		case 754: //Push point
		case 755: //Pull point
		{
			subsector_t *ss = R_PointInSubsector(mapthings[i].x << FRACBITS, mapthings[i].y << FRACBITS);
			sector_t *s;
			line_t *line;

			if (!ss)
			{
				CONS_Debug(DBG_GAMELOGIC, "Push/pull point: Placed outside of map bounds!\n");
				break;
			}

			s = ss->sector;
			line = P_FindPointPushLine(&s->tags);

			if (!line)
			{
				CONS_Debug(DBG_GAMELOGIC, "Push/pull point: Unable to find line of type 547 tagged to sector %s!\n", sizeu1((size_t)(s - sectors)));
				break;
			}

			mapthings[i].args[0] = mapthings[i].angle;
			mapthings[i].args[1] = P_AproxDistance(line->dx >> FRACBITS, line->dy >> FRACBITS);
			if (mapthings[i].type == 755)
				mapthings[i].args[1] *= -1;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[2] |= TMPP_NOZFADE;
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[2] |= TMPP_PUSHZ;
			if (!(line->flags & ML_NOCLIMB))
				mapthings[i].args[2] |= TMPP_NONEXCLUSIVE;
			mapthings[i].type = 754;
			break;
		}
		case 756: //Blast linedef executor
			mapthings[i].args[0] = mapthings[i].angle;
			break;
		case 757: //Fan particle generator
		{
			INT32 j = Tag_FindLineSpecial(15, mapthings[i].angle);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Particle generator (mapthing #%s) needs to be tagged to a #15 parameter line (trying to find tag %d).\n", sizeu1(i), mapthings[i].angle);
				break;
			}
			mapthings[i].args[0] = mapthings[i].z;
			mapthings[i].args[1] = R_PointToDist2(lines[j].v1->x, lines[j].v1->y, lines[j].v2->x, lines[j].v2->y) >> FRACBITS;
			mapthings[i].args[2] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			mapthings[i].args[3] = sides[lines[j].sidenum[0]].rowoffset >> FRACBITS;
			mapthings[i].args[4] = lines[j].backsector ? sides[lines[j].sidenum[1]].textureoffset >> FRACBITS : 0;
			mapthings[i].args[6] = mapthings[i].angle;
			if (sides[lines[j].sidenum[0]].toptexture)
				P_WriteConstant(sides[lines[j].sidenum[0]].toptexture, &mapthings[i].stringargs[0]);
			break;
		}
		case 762: //PolyObject spawn point (crush)
		{
			INT32 check = -1;
			INT32 firstline = -1;
			mtag_t tag = Tag_FGet(&mapthings[i].tags);

			TAG_ITER_LINES(tag, check)
			{
				if (lines[check].special == 20)
				{
					firstline = check;
					break;
				}
			}

			if (firstline != -1)
				lines[firstline].args[3] |= TMPF_CRUSH;

			mapthings[i].type = 761;
			break;
		}
		case 780: //Skybox
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 799: //Tutorial plant
			mapthings[i].args[0] = mapthings[i].extrainfo;
			break;
		case 1002: //Dripping water
			mapthings[i].args[0] = mapthings[i].angle;
			break;
		case 1007: //Kelp
		case 1008: //Stalagmite (DSZ1)
		case 1011: //Stalagmite (DSZ2)
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_OBJECTSPECIAL);
			break;
		case 1102: //Eggman Statue
			mapthings[i].args[1] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 1104: //Mace spawnpoint
		case 1105: //Chain with maces spawnpoint
		case 1106: //Chained spring spawnpoint
		case 1107: //Chain spawnpoint
		case 1109: //Firebar spawnpoint
		case 1110: //Custom mace spawnpoint
		{
			mtag_t tag = (mtag_t)mapthings[i].angle;
			INT32 j = Tag_FindLineSpecial(9, tag);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Chain/mace setup: Unable to find parameter line 9 (tag %d)!\n", tag);
				break;
			}

			mapthings[i].angle = lines[j].frontsector->ceilingheight >> FRACBITS;
			mapthings[i].pitch = lines[j].frontsector->floorheight >> FRACBITS;
			mapthings[i].args[0] = lines[j].dx >> FRACBITS;
			mapthings[i].args[1] = mapthings[i].extrainfo;
			mapthings[i].args[3] = lines[j].dy >> FRACBITS;
			mapthings[i].args[4] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			mapthings[i].args[7] = -sides[lines[j].sidenum[0]].rowoffset >> FRACBITS;
			if (lines[j].backsector)
			{
				mapthings[i].roll = lines[j].backsector->ceilingheight >> FRACBITS;
				mapthings[i].args[2] = sides[lines[j].sidenum[1]].rowoffset >> FRACBITS;
				mapthings[i].args[5] = lines[j].backsector->floorheight >> FRACBITS;
				mapthings[i].args[6] = sides[lines[j].sidenum[1]].textureoffset >> FRACBITS;
			}
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[8] |= TMM_DOUBLESIZE;
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[8] |= TMM_SILENT;
			if (lines[j].flags & ML_NOCLIMB)
				mapthings[i].args[8] |= TMM_ALLOWYAWCONTROL;
			if (lines[j].flags & ML_SKEWTD)
				mapthings[i].args[8] |= TMM_SWING;
			if (lines[j].flags & ML_NOSKEW)
				mapthings[i].args[8] |= TMM_MACELINKS;
			if (lines[j].flags & ML_MIDPEG)
				mapthings[i].args[8] |= TMM_CENTERLINK;
			if (lines[j].flags & ML_MIDSOLID)
				mapthings[i].args[8] |= TMM_CLIP;
			if (lines[j].flags & ML_WRAPMIDTEX)
				mapthings[i].args[8] |= TMM_ALWAYSTHINK;
			if (mapthings[i].type == 1110)
			{
				P_WriteConstant(sides[lines[j].sidenum[0]].toptexture, &mapthings[i].stringargs[0]);
				P_WriteConstant(lines[j].backsector ? sides[lines[j].sidenum[1]].toptexture : MT_NULL, &mapthings[i].stringargs[1]);
			}
			break;
		}
		case 1101: //Torch
		case 1119: //Candle
		case 1120: //Candle pricket
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		case 1121: //Flame holder
			if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[0] |= TMFH_NOFLAME;
			if (mapthings[i].options & MTF_EXTRA)
				mapthings[i].args[0] |= TMFH_CORONA;
			break;
		case 1127: //Spectator EggRobo
			if (mapthings[i].options & MTF_AMBUSH)
				mapthings[i].args[0] = TMED_LEFT;
			else if (mapthings[i].options & MTF_OBJECTSPECIAL)
				mapthings[i].args[0] = TMED_RIGHT;
			else
				mapthings[i].args[0] = TMED_NONE;
			break;
		case 1200: //Tumbleweed (Big)
		case 1201: //Tumbleweed (Small)
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1202: //Rock spawner
		{
			mtag_t tag = (mtag_t)mapthings[i].angle;
			INT32 j = Tag_FindLineSpecial(12, tag);

			if (j == -1)
			{
				CONS_Debug(DBG_GAMELOGIC, "Rock spawner: Unable to find parameter line 12 (tag %d)!\n", tag);
				break;
			}
			mapthings[i].angle = AngleFixed(R_PointToAngle2(lines[j].v2->x, lines[j].v2->y, lines[j].v1->x, lines[j].v1->y)) >> FRACBITS;
			mapthings[i].args[0] = P_AproxDistance(lines[j].dx, lines[j].dy) >> FRACBITS;
			mapthings[i].args[1] = sides[lines[j].sidenum[0]].textureoffset >> FRACBITS;
			mapthings[i].args[2] = !!(lines[j].flags & ML_NOCLIMB);
			P_WriteConstant(MT_ROCKCRUMBLE1 + (sides[lines[j].sidenum[0]].rowoffset >> FRACBITS), &mapthings[i].stringargs[0]);
			break;
		}
		case 1221: //Minecart saloon door
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1229: //Minecart switch point
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1300: //Flame jet (horizontal)
		case 1301: //Flame jet (vertical)
			mapthings[i].args[0] = (mapthings[i].angle >> 13)*TICRATE/2;
			mapthings[i].args[1] = ((mapthings[i].angle >> 10) & 7)*TICRATE/2;
			mapthings[i].args[2] = 80 - 5*mapthings[i].extrainfo;
			mapthings[i].args[3] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1304: //Lavafall
			mapthings[i].args[0] = mapthings[i].angle;
			mapthings[i].args[1] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1305: //Rollout Rock
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1500: //Glaregoyle
		case 1501: //Glaregoyle (Up)
		case 1502: //Glaregoyle (Down)
		case 1503: //Glaregoyle (Long)
			if (mapthings[i].angle >= 360)
				mapthings[i].args[1] = 7*(mapthings[i].angle/360) + 1;
			break;
		case 1700: //Axis
			mapthings[i].args[2] = mapthings[i].angle & 16383;
			mapthings[i].args[3] = !!(mapthings[i].angle & 16384);
			/* FALLTHRU */
		case 1701: //Axis transfer
		case 1702: //Axis transfer line
			mapthings[i].args[0] = mapthings[i].extrainfo;
			mapthings[i].args[1] = mapthings[i].options;
			break;
		case 1703: //Ideya drone
			mapthings[i].args[0] = mapthings[i].angle & 0xFFF;
			mapthings[i].args[1] = mapthings[i].extrainfo*32;
			mapthings[i].args[2] = ((mapthings[i].angle & 0xF000) >> 12)*32;
			if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_EXTRA)) == (MTF_OBJECTSPECIAL|MTF_EXTRA))
				mapthings[i].args[3] = TMDA_BOTTOM;
			else if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_EXTRA)) == MTF_OBJECTSPECIAL)
				mapthings[i].args[3] = TMDA_TOP;
			else if ((mapthings[i].options & (MTF_OBJECTSPECIAL|MTF_EXTRA)) == MTF_EXTRA)
				mapthings[i].args[3] = TMDA_MIDDLE;
			else
				mapthings[i].args[3] = TMDA_BOTTOMOFFSET;
			mapthings[i].args[4] = !!(mapthings[i].options & MTF_AMBUSH);
			break;
		case 1704: //NiGHTS bumper
			mapthings[i].pitch = 30 * (((mapthings[i].options & 15) + 9) % 12);
			mapthings[i].options &= ~0xF;
			break;
		case 1705: //Hoop
		case 1713: //Hoop (Customizable)
		{
			UINT16 oldangle = mapthings[i].angle;
			mapthings[i].angle = (mapthings[i].extrainfo == 1) ? oldangle - 90  : ((oldangle >> 8)*360)/256;
			mapthings[i].pitch = (mapthings[i].extrainfo == 1) ? oldangle / 360 : ((oldangle & 255)*360)/256;
			mapthings[i].args[0] = (mapthings[i].type == 1705) ? 96 : (mapthings[i].options & 0xF)*16 + 32;
			mapthings[i].options &= ~0xF;
			mapthings[i].type = 1713;
			break;
		}
		case 1710: //Ideya capture
			mapthings[i].args[0] = mapthings[i].extrainfo;
			mapthings[i].args[1] = mapthings[i].angle;
			break;
		case 1714: //Ideya anchor point
			mapthings[i].args[0] = mapthings[i].extrainfo;
			break;
		case 1806: //King Bowser
			mapthings[i].args[0] = LE_KOOPA;
			break;
		case 1807: //Axe
			mapthings[i].args[0] = LE_AXE;
			break;
		case 2000: //Smashing spikeball
			mapthings[i].args[0] = mapthings[i].angle;
			break;
		case 2006: //Jack-o'-lantern 1
		case 2007: //Jack-o'-lantern 2
		case 2008: //Jack-o'-lantern 3
			mapthings[i].args[0] = !!(mapthings[i].options & MTF_EXTRA);
			break;
		default:
			break;
		}
	}
}

static void P_ConvertBinaryLinedefFlags(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (!!(lines[i].flags & ML_DONTPEGBOTTOM) ^ !!(lines[i].flags & ML_MIDPEG))
			lines[i].flags |= ML_MIDPEG;
		else
			lines[i].flags &= ~ML_MIDPEG;

		if (lines[i].special >= 100 && lines[i].special < 300)
		{
			if (lines[i].flags & ML_DONTPEGTOP)
				lines[i].flags |= ML_SKEWTD;
			else
				lines[i].flags &= ~ML_SKEWTD;

			if ((lines[i].flags & ML_TFERLINE) && lines[i].frontsector)
			{
				size_t j;

				for (j = 0; j < lines[i].frontsector->linecount; j++)
				{
					if (lines[i].frontsector->lines[j]->flags & ML_DONTPEGTOP)
						lines[i].frontsector->lines[j]->flags |= ML_SKEWTD;
					else
						lines[i].frontsector->lines[j]->flags &= ~ML_SKEWTD;
				}
			}
		}

	}
}

//For maps in binary format, converts setup of specials to UDMF format.
static void P_ConvertBinaryMap(void)
{
	P_ConvertBinaryLinedefTypes();
	P_ConvertBinarySectorTypes();
	P_ConvertBinaryThingTypes();
	P_ConvertBinaryLinedefFlags();
	if (M_CheckParm("-writetextmap"))
		P_WriteTextmap();
}

/** Compute MD5 message digest for bytes read from memory source
  *
  * The resulting message digest number will be written into the 16 bytes
  * beginning at RESBLOCK.
  *
  * \param filename path of file
  * \param resblock resulting MD5 checksum
  * \return 0 if MD5 checksum was made, and is at resblock, 1 if error was found
  */
static INT32 P_MakeBufferMD5(const char *buffer, size_t len, void *resblock)
{
#ifdef NOMD5
	(void)buffer;
	(void)len;
	memset(resblock, 0x00, 16);
	return 1;
#else
	tic_t t = I_GetTime();
	CONS_Debug(DBG_SETUP, "Making MD5\n");
	if (md5_buffer(buffer, len, resblock) == NULL)
		return 1;
	CONS_Debug(DBG_SETUP, "MD5 calc took %f seconds\n", (float)(I_GetTime() - t)/NEWTICRATE);
	return 0;
#endif
}

static void P_MakeMapMD5(virtres_t *virt, void *dest)
{
	unsigned char resmd5[16];

	if (udmf)
	{
		virtlump_t *textmap = vres_Find(virt, "TEXTMAP");
		P_MakeBufferMD5((char*)textmap->data, textmap->size, resmd5);
	}
	else
	{
		unsigned char linemd5[16];
		unsigned char sectormd5[16];
		unsigned char thingmd5[16];
		unsigned char sidedefmd5[16];
		UINT8 i;

		// Create a hash for the current map
		// get the actual lumps!
		virtlump_t* virtlines   = vres_Find(virt, "LINEDEFS");
		virtlump_t* virtsectors = vres_Find(virt, "SECTORS");
		virtlump_t* virtmthings = vres_Find(virt, "THINGS");
		virtlump_t* virtsides   = vres_Find(virt, "SIDEDEFS");

		P_MakeBufferMD5((char*)virtlines->data,   virtlines->size, linemd5);
		P_MakeBufferMD5((char*)virtsectors->data, virtsectors->size,  sectormd5);
		P_MakeBufferMD5((char*)virtmthings->data, virtmthings->size,   thingmd5);
		P_MakeBufferMD5((char*)virtsides->data,   virtsides->size, sidedefmd5);

		for (i = 0; i < 16; i++)
			resmd5[i] = (linemd5[i] + sectormd5[i] + thingmd5[i] + sidedefmd5[i]) & 0xFF;
	}

	M_Memcpy(dest, &resmd5, 16);
}

static boolean P_LoadMapFromFile(void)
{
	virtres_t *virt = vres_GetMap(lastloadedmaplumpnum);
	virtlump_t *textmap = vres_Find(virt, "TEXTMAP");
	size_t i;
	udmf = textmap != NULL;

	if (!P_LoadMapData(virt))
		return false;
	P_LoadMapBSP(virt);
	P_LoadMapLUT(virt);

	P_LinkMapData();

	if (!udmf)
		P_AddBinaryMapTags();

	Taglist_InitGlobalTables();

	if (!udmf)
		P_ConvertBinaryMap();

	// Copy relevant map data for NetArchive purposes.
	spawnsectors = Z_Calloc(numsectors * sizeof(*sectors), PU_LEVEL, NULL);
	spawnlines = Z_Calloc(numlines * sizeof(*lines), PU_LEVEL, NULL);
	spawnsides = Z_Calloc(numsides * sizeof(*sides), PU_LEVEL, NULL);

	memcpy(spawnsectors, sectors, numsectors * sizeof(*sectors));
	memcpy(spawnlines, lines, numlines * sizeof(*lines));
	memcpy(spawnsides, sides, numsides * sizeof(*sides));

	for (i = 0; i < numsectors; i++)
		if (sectors[i].tags.count)
			spawnsectors[i].tags.tags = memcpy(Z_Malloc(sectors[i].tags.count*sizeof(mtag_t), PU_LEVEL, NULL), sectors[i].tags.tags, sectors[i].tags.count*sizeof(mtag_t));

	P_MakeMapMD5(virt, &mapmd5);

	vres_Free(virt);
	return true;
}

//
// LEVEL INITIALIZATION FUNCTIONS
//

/** Sets up a sky texture to use for the level.
  * The sky texture is used instead of F_SKY1.
  */
void P_SetupLevelSky(INT32 skynum, boolean global)
{
	char skytexname[12];

	sprintf(skytexname, "SKY%d", skynum);
	skytexture = R_TextureNumForName(skytexname);
	levelskynum = skynum;

	// Global change
	if (global)
		globallevelskynum = levelskynum;

	// Don't go beyond for dedicated servers
	if (dedicated)
		return;

	// scale up the old skies, if needed
	R_SetupSkyDraw();
}

static const char *maplumpname;
lumpnum_t lastloadedmaplumpnum; // for comparative savegame

//
// P_LevelInitStuff
//
// Some player initialization for map start.
//
static void P_InitLevelSettings(void)
{
	INT32 i;
	boolean canresetlives = true;

	leveltime = 0;

	modulothing = 0;

	// special stage tokens, emeralds, and ring total
	tokenbits = 0;
	runemeraldmanager = false;
	emeraldspawndelay = 60*TICRATE;
	if ((netgame || multiplayer) && !G_IsSpecialStage(gamemap))
		nummaprings = -1;
	else
		nummaprings = mapheaderinfo[gamemap-1]->startrings;

	// emerald hunt
	hunt1 = hunt2 = hunt3 = NULL;

	// map time limit
	if (mapheaderinfo[gamemap-1]->countdown)
	{
		tic_t maxtime = 0;
		countdowntimer = mapheaderinfo[gamemap-1]->countdown * TICRATE;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;
			if (players[i].starposttime > maxtime)
				maxtime = players[i].starposttime;
		}
		countdowntimer -= maxtime;
	}
	else
		countdowntimer = 0;
	countdowntimeup = false;

	// clear ctf pointers
	redflag = blueflag = NULL;
	rflagpoint = bflagpoint = NULL;

	// circuit, race and competition stuff
	circuitmap = false;
	numstarposts = 0;
	ssspheres = timeinmap = 0;

	// Assume Special Stages were failed in unless proven otherwise - via P_GiveEmerald or emerald touchspecial
	// Normal stages will default to be OK, until a Lua script / linedef executor says otherwise.
	stagefailed = G_IsSpecialStage(gamemap);

	// Reset temporary record data
	memset(&ntemprecords, 0, sizeof(nightsdata_t));

	// earthquake camera
	memset(&quake,0,sizeof(struct quake));

	if ((netgame || multiplayer) && G_GametypeUsesCoopStarposts() && cv_coopstarposts.value == 2)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].lives > 0)
			{
				canresetlives = false;
				break;
			}
		}
	}

	countdown = countdown2 = exitfadestarted = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		G_PlayerReborn(i, true);

		if (canresetlives && (netgame || multiplayer) && playeringame[i] && (G_CompetitionGametype() || players[i].lives <= 0))
		{
			// In Co-Op, replenish a user's lives if they are depleted.
			players[i].lives = cv_startinglives.value;
		}

		// obliteration station...
		players[i].numboxes = players[i].totalring =\
		 players[i].laps = players[i].marescore = players[i].lastmarescore =\
		 players[i].mare = players[i].exiting = 0;

		players[i].drillmeter = 40*20;

		// hit these too
		players[i].pflags &= ~(PF_GAMETYPEOVER);
	}

	if (botingame)
		CV_SetValue(&cv_analog[1], true);
}

// Respawns all the mapthings and mobjs in the map from the already loaded map data.
void P_RespawnThings(void)
{
	// Search through all the thinkers.
	thinker_t *think;
	INT32 i, viewid = -1, centerid = -1; // for skyboxes

	// check if these are any of the normal viewpoint/centerpoint mobjs in the level or not
	if (skyboxmo[0] || skyboxmo[1])
		for (i = 0; i < 16; i++)
		{
			if (skyboxmo[0] && skyboxmo[0] == skyboxviewpnts[i])
				viewid = i; // save id just in case
			if (skyboxmo[1] && skyboxmo[1] == skyboxcenterpnts[i])
				centerid = i; // save id just in case
		}

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;
		P_RemoveMobj((mobj_t *)think);
	}

	P_InitLevelSettings();

	localaiming = 0;
	localaiming2 = 0;

	P_SpawnMapThings(true);

	// restore skybox viewpoint/centerpoint if necessary, set them to defaults if we can't do that
	skyboxmo[0] = skyboxviewpnts[(viewid >= 0) ? viewid : 0];
	skyboxmo[1] = skyboxcenterpnts[(centerid >= 0) ? centerid : 0];
}

static void P_RunLevelScript(const char *scriptname)
{
	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_SCRIPTISFILE))
	{
		lumpnum_t lumpnum;
		char newname[9];

		strncpy(newname, scriptname, 8);

		newname[8] = '\0';

		lumpnum = W_CheckNumForName(newname);

		if (lumpnum == LUMPERROR || W_LumpLength(lumpnum) == 0)
		{
			CONS_Debug(DBG_SETUP, "SOC Error: script lump %s not found/not valid.\n", newname);
			return;
		}

		COM_BufInsertText(W_CacheLumpNum(lumpnum, PU_CACHE));
	}
	else
	{
		COM_BufAddText(va("exec %s\n", scriptname));
	}
	COM_BufExecute(); // Run it!
}

static void P_ForceCharacter(const char *forcecharskin)
{
	if (netgame)
	{
		char skincmd[33];
		if (splitscreen)
		{
			sprintf(skincmd, "skin2 %s\n", forcecharskin);
			CV_Set(&cv_skin2, forcecharskin);
		}

		sprintf(skincmd, "skin %s\n", forcecharskin);
		COM_BufAddText(skincmd);
	}
	else
	{
		if (splitscreen)
		{
			SetPlayerSkin(secondarydisplayplayer, forcecharskin);
			if ((unsigned)cv_playercolor2.value != skins[players[secondarydisplayplayer].skin].prefcolor)
			{
				CV_StealthSetValue(&cv_playercolor2, skins[players[secondarydisplayplayer].skin].prefcolor);
				players[secondarydisplayplayer].skincolor = skins[players[secondarydisplayplayer].skin].prefcolor;
			}
		}

		SetPlayerSkin(consoleplayer, forcecharskin);
		// normal player colors in single player
		if ((unsigned)cv_playercolor.value != skins[players[consoleplayer].skin].prefcolor)
		{
			CV_StealthSetValue(&cv_playercolor, skins[players[consoleplayer].skin].prefcolor);
			players[consoleplayer].skincolor = skins[players[consoleplayer].skin].prefcolor;
		}
	}
}

static void P_ResetSpawnpoints(void)
{
	UINT8 i;

	numdmstarts = numredctfstarts = numbluectfstarts = 0;

	// reset the player starts
	for (i = 0; i < MAXPLAYERS; i++)
		playerstarts[i] = bluectfstarts[i] = redctfstarts[i] = NULL;

	for (i = 0; i < MAX_DM_STARTS; i++)
		deathmatchstarts[i] = NULL;

	for (i = 0; i < 2; i++)
		skyboxmo[i] = NULL;

	for (i = 0; i < 16; i++)
		skyboxviewpnts[i] = skyboxcenterpnts[i] = NULL;
}

static void P_LoadRecordGhosts(void)
{
	const size_t glen = strlen(srb2home)+1+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char *gpath = malloc(glen);
	INT32 i;

	if (!gpath)
		return;

	sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));

	// Best Score ghost
	if (cv_ghost_bestscore.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_bestscore.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-score-best.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-score-best.lmp", gpath, skins[i].name));
		}
	}

	// Best Time ghost
	if (cv_ghost_besttime.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_besttime.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-time-best.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-time-best.lmp", gpath, skins[i].name));
		}
	}

	// Best Rings ghost
	if (cv_ghost_bestrings.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_bestrings.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-rings-best.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-rings-best.lmp", gpath, skins[i].name));
		}
	}

	// Last ghost
	if (cv_ghost_last.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_last.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-last.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-last.lmp", gpath, skins[i].name));
		}
	}

	// Guest ghost
	if (cv_ghost_guest.value && FIL_FileExists(va("%s-guest.lmp", gpath)))
		G_AddGhost(va("%s-guest.lmp", gpath));

	free(gpath);
}

static void P_LoadNightsGhosts(void)
{
	const size_t glen = strlen(srb2home)+1+strlen("replay")+1+strlen(timeattackfolder)+1+strlen("MAPXX")+1;
	char *gpath = malloc(glen);
	INT32 i;

	if (!gpath)
		return;

	sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));

	// Best Score ghost
	if (cv_ghost_bestscore.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_bestscore.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-score-best.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-score-best.lmp", gpath, skins[i].name));
		}
	}

	// Best Time ghost
	if (cv_ghost_besttime.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_besttime.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-time-best.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-time-best.lmp", gpath, skins[i].name));
		}
	}

	// Last ghost
	if (cv_ghost_last.value)
	{
		for (i = 0; i < numskins; ++i)
		{
			if (cv_ghost_last.value == 1 && players[consoleplayer].skin != i)
				continue;

			if (FIL_FileExists(va("%s-%s-last.lmp", gpath, skins[i].name)))
				G_AddGhost(va("%s-%s-last.lmp", gpath, skins[i].name));
		}
	}

	// Guest ghost
	if (cv_ghost_guest.value && FIL_FileExists(va("%s-guest.lmp", gpath)))
		G_AddGhost(va("%s-guest.lmp", gpath));

	free(gpath);
}

static void P_InitTagGametype(void)
{
	UINT8 i;
	INT32 realnumplayers = 0;
	INT32 playersactive[MAXPLAYERS];

	//I just realized how problematic this code can be.
	//D_NumPlayers() will not always cover the scope of the netgame.
	//What if one player is node 0 and the other node 31?
	//The solution? Make a temp array of all players that are currently playing and pick from them.
	//Future todo? When a player leaves, shift all nodes down so D_NumPlayers() can be used as intended?
	//Also, you'd never have to loop through all 32 players slots to find anything ever again.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !(players[i].spectator || players[i].quittime))
		{
			playersactive[realnumplayers] = i; //stores the player's node in the array.
			realnumplayers++;
		}
	}

	if (!realnumplayers) //this should also fix the dedicated crash bug. You only pick a player if one exists to be picked.
	{
		CONS_Printf(M_GetText("No player currently available to become IT. Awaiting available players.\n"));
		return;
	}

	i = P_RandomKey(realnumplayers);
	players[playersactive[i]].pflags |= PF_TAGIT; //choose our initial tagger before map starts.

	// Taken and modified from G_DoReborn()
	// Remove the player so he can respawn elsewhere.
	// first disassociate the corpse
	if (players[playersactive[i]].mo)
		P_RemoveMobj(players[playersactive[i]].mo);

	G_SpawnPlayer(playersactive[i]); //respawn the lucky player in his dedicated spawn location.
}

static void P_SetupCamera(void)
{
	if (players[displayplayer].mo && (server || addedtogame))
	{
		camera.x = players[displayplayer].mo->x;
		camera.y = players[displayplayer].mo->y;
		camera.z = players[displayplayer].mo->z;
		camera.angle = players[displayplayer].mo->angle;
		camera.subsector = R_PointInSubsector(camera.x, camera.y); // make sure camera has a subsector set -- Monster Iestyn (12/11/18)
	}
	else
	{
		mapthing_t *thing;

		if (gametyperules & GTR_DEATHMATCHSTARTS)
			thing = deathmatchstarts[0];
		else
			thing = playerstarts[0];

		if (thing)
		{
			camera.x = thing->x;
			camera.y = thing->y;
			camera.z = thing->z;
			camera.angle = FixedAngle((fixed_t)thing->angle << FRACBITS);
			camera.subsector = R_PointInSubsector(camera.x, camera.y); // make sure camera has a subsector set -- Monster Iestyn (12/11/18)
		}
	}
}

static void P_InitCamera(void)
{
	if (!dedicated)
	{
		P_SetupCamera();

		// Salt: CV_ClearChangedFlags() messes with your settings :(
		/*if (!cv_cam_height.changed)
			CV_Set(&cv_cam_height, cv_cam_height.defaultvalue);
		if (!cv_cam2_height.changed)
			CV_Set(&cv_cam2_height, cv_cam2_height.defaultvalue);

		if (!cv_cam_dist.changed)
			CV_Set(&cv_cam_dist, cv_cam_dist.defaultvalue);
		if (!cv_cam2_dist.changed)
			CV_Set(&cv_cam2_dist, cv_cam2_dist.defaultvalue);*/

			// Though, I don't think anyone would care about cam_rotate being reset back to the only value that makes sense :P
		if (!cv_cam_rotate.changed)
			CV_Set(&cv_cam_rotate, cv_cam_rotate.defaultvalue);
		if (!cv_cam2_rotate.changed)
			CV_Set(&cv_cam2_rotate, cv_cam2_rotate.defaultvalue);

		if (!cv_analog[0].changed)
			CV_SetValue(&cv_analog[0], 0);
		if (!cv_analog[1].changed)
			CV_SetValue(&cv_analog[1], 0);

		displayplayer = consoleplayer; // Start with your OWN view, please!
	}

	if (twodlevel)
	{
		CV_SetValue(&cv_analog[0], false);
		CV_SetValue(&cv_analog[1], false);
	}
	else
	{
		if (cv_useranalog[0].value)
			CV_SetValue(&cv_analog[0], true);

		if ((splitscreen && cv_useranalog[1].value) || botingame)
			CV_SetValue(&cv_analog[1], true);
	}
}

static void P_RunSpecialStageWipe(void)
{
	tic_t starttime = I_GetTime();
	tic_t endtime = starttime + (3*TICRATE)/2;
	tic_t nowtime;

	S_StartSound(NULL, sfx_s3kaf);

	// Fade music! Time it to S3KAF: 0.25 seconds is snappy.
	if (RESETMUSIC ||
		strnicmp(S_MusicName(),
		(mapmusflags & MUSIC_RELOADRESET) ? mapheaderinfo[gamemap - 1]->musname : mapmusname, 7))
		S_FadeOutStopMusic(MUSICRATE/4); //FixedMul(FixedDiv(F_GetWipeLength(wipedefs[wipe_speclevel_towhite])*NEWTICRATERATIO, NEWTICRATE), MUSICRATE)

	F_WipeStartScreen();
	wipestyleflags |= (WSF_FADEOUT|WSF_TOWHITE);

#ifdef HWRENDER
	// uh..........
	if (rendermode == render_opengl)
		F_WipeColorFill(0);
#endif

	F_WipeEndScreen();
	F_RunWipe(wipedefs[wipe_speclevel_towhite], false);

	I_OsPolling();
	I_FinishUpdate(); // page flip or blit buffer
	if (moviemode)
		M_SaveFrame();

	nowtime = lastwipetic;

	// Hold on white for extra effect.
	while (nowtime < endtime)
	{
		// wait loop
		while (!((nowtime = I_GetTime()) - lastwipetic))
		{
			I_Sleep(cv_sleep.value);
			I_UpdateTime(cv_timescale.value);
		}
		lastwipetic = nowtime;
		if (moviemode) // make sure we save frames for the white hold too
			M_SaveFrame();
	}
}

static void P_RunLevelWipe(void)
{
	F_WipeStartScreen();
	wipestyleflags |= WSF_FADEOUT;

#ifdef HWRENDER
	// uh..........
	if (rendermode == render_opengl)
		F_WipeColorFill(31);
#endif

	F_WipeEndScreen();
	// for titlemap: run a specific wipe if specified
	// needed for exiting time attack
	if (wipetypepre != INT16_MAX)
		F_RunWipe(
		(wipetypepre >= 0 && F_WipeExists(wipetypepre)) ? wipetypepre : wipedefs[wipe_level_toblack],
			false);
	wipetypepre = -1;
}

static void P_InitPlayers(void)
{
	UINT8 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		// Start players with pity shields if possible
		players[i].pity = -1;

		players[i].mo = NULL;

		if (!G_PlatformGametype())
			G_DoReborn(i);
		else // gametype is GT_COOP or GT_RACE
		{
			G_SpawnPlayer(i);
			if (players[i].starposttime)
				P_ClearStarPost(players[i].starpostnum);
		}
	}
}

static void P_WriteLetter(void)
{
	char *buf, *b;

	if (!unlockables[28].unlocked) // pandora's box
		return;

	if (modeattacking)
		return;

#ifndef DEVELOP
	if (modifiedgame)
		return;
#endif

	if (netgame || multiplayer)
		return;

	if (gamemap != 0x1d35 - 016464)
		return;

	P_SpawnMobj(0640370000, 0x11000000, 0x3180000, MT_LETTER)->angle = ANGLE_90;

	if (textprompts[199]->page[1].backcolor == 259)
		return;

	buf = W_CacheLumpName("WATERMAP", PU_STATIC);
	b = buf;

	while ((*b != 65) && (b - buf < 256))
	{
		*b = (*b - 65) & 255;
		b++;
	}
	*b = '\0';

	Z_Free(textprompts[199]->page[1].text);
	textprompts[199]->page[1].text = Z_StrDup(buf);
	textprompts[199]->page[1].lines = 4;
	textprompts[199]->page[1].backcolor = 259;
	Z_Free(buf);
}

static void P_InitGametype(void)
{
	UINT8 i;

	P_InitPlayers();

	// restore time in netgame (see also g_game.c)
	if ((netgame || multiplayer) && G_GametypeUsesCoopStarposts() && cv_coopstarposts.value == 2)
	{
		// is this a hack? maybe
		tic_t maxstarposttime = 0;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].starposttime > maxstarposttime)
				maxstarposttime = players[i].starposttime;
		}
		leveltime = maxstarposttime;
	}

	P_WriteLetter();

	if (modeattacking == ATTACKING_RECORD && !demoplayback)
		P_LoadRecordGhosts();
	else if (modeattacking == ATTACKING_NIGHTS && !demoplayback)
		P_LoadNightsGhosts();

	if (G_TagGametype())
		P_InitTagGametype();
	else if (((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE) && server)
		CV_StealthSetValue(&cv_numlaps,
		(cv_basenumlaps.value)
			? cv_basenumlaps.value
			: mapheaderinfo[gamemap - 1]->numlaps);
}

/** Loads a level from a lump or external wad.
  *
  * \param fromnetsave If true, skip some stuff because we're loading a netgame snapshot.
  * \todo Clean up, refactor, split up; get rid of the bloat.
  */
boolean P_LoadLevel(boolean fromnetsave, boolean reloadinggamestate)
{
	// use gamemap to get map number.
	// 99% of the things already did, so.
	// Map header should always be in place at this point
	INT32 i, ranspecialwipe = 0;
	sector_t *ss;
	levelloading = true;

	// This is needed. Don't touch.
	maptol = mapheaderinfo[gamemap-1]->typeoflevel;
	gametyperules = gametypedefaultrules[gametype];

	CON_Drawer(); // let the user know what we are going to do
	I_FinishUpdate(); // page flip or blit buffer

	// Reset the palette
	if (rendermode != render_none)
		V_SetPaletteLump("PLAYPAL");

	// Initialize sector node list.
	P_Initsecnode();

	if (netgame || multiplayer)
		cv_debug = botskin = 0;

	if (metalplayback)
		G_StopMetalDemo();

	// Clear CECHO messages
	HU_ClearCEcho();

	if (mapheaderinfo[gamemap-1]->runsoc[0] != '#')
		P_RunSOC(mapheaderinfo[gamemap-1]->runsoc);

	if (cv_runscripts.value && mapheaderinfo[gamemap-1]->scriptname[0] != '#')
		P_RunLevelScript(mapheaderinfo[gamemap-1]->scriptname);

	P_InitLevelSettings();

	postimgtype = postimgtype2 = postimg_none;

	if (mapheaderinfo[gamemap-1]->forcecharacter[0] != '\0')
		P_ForceCharacter(mapheaderinfo[gamemap-1]->forcecharacter);

	if (!dedicated)
	{
		// chasecam on in first-person gametypes and 2D
		boolean chase = (!(gametyperules & GTR_FIRSTPERSON)) || (maptol & TOL_2D);

		// Salt: CV_ClearChangedFlags() messes with your settings :(
		/*if (!cv_cam_speed.changed)
			CV_Set(&cv_cam_speed, cv_cam_speed.defaultvalue);*/

		CV_UpdateCamDist();
		CV_UpdateCam2Dist();

		if (!cv_chasecam.changed)
			CV_SetValue(&cv_chasecam, chase);

		// same for second player
		if (!cv_chasecam2.changed)
			CV_SetValue(&cv_chasecam2, chase);
	}

	// Initial height of PointOfView
	// will be set by player think.
	players[consoleplayer].viewz = 1;

	// Cancel all d_main.c fadeouts (keep fade in though).
	if (reloadinggamestate)
		wipegamestate = gamestate; // Don't fade if reloading the gamestate
	else
		wipegamestate = FORCEWIPEOFF;
	wipestyleflags = 0;

	// Special stage & record attack retry fade to white
	// This is handled BEFORE sounds are stopped.
	if (G_GetModeAttackRetryFlag())
	{
		if (modeattacking && !demoplayback)
		{
			ranspecialwipe = 2;
			wipestyleflags |= (WSF_FADEOUT|WSF_TOWHITE);
		}
		G_ClearModeAttackRetryFlag();
	}
	else if (rendermode != render_none && G_IsSpecialStage(gamemap))
	{
		P_RunSpecialStageWipe();
		ranspecialwipe = 1;
	}

	// Make sure all sounds are stopped before Z_FreeTags.
	S_StopSounds();
	S_ClearSfx();

	// Fade out music here. Deduct 2 tics so the fade volume actually reaches 0.
	// But don't halt the music! S_Start will take care of that. This dodges a MIDI crash bug.
	if (!(reloadinggamestate || titlemapinaction) && (RESETMUSIC ||
		strnicmp(S_MusicName(),
			(mapmusflags & MUSIC_RELOADRESET) ? mapheaderinfo[gamemap-1]->musname : mapmusname, 7)))
	{
		S_FadeMusic(0, FixedMul(
			FixedDiv((F_GetWipeLength(wipedefs[wipe_level_toblack])-2)*NEWTICRATERATIO, NEWTICRATE), MUSICRATE));
	}

	// Let's fade to black here
	// But only if we didn't do the special stage wipe
	if (rendermode != render_none && !(ranspecialwipe || reloadinggamestate))
		P_RunLevelWipe();

	if (!(reloadinggamestate || titlemapinaction))
	{
		if (ranspecialwipe == 2)
		{
			pausedelay = -3; // preticker plus one
			S_StartSound(NULL, sfx_s3k73);
		}

		// Print "SPEEDING OFF TO [ZONE] [ACT 1]..."
		if (rendermode != render_none)
		{
			// Don't include these in the fade!
			char tx[64];
			V_DrawSmallString(1, 191, V_ALLOWLOWERCASE|V_TRANSLUCENT|V_SNAPTOLEFT|V_SNAPTOBOTTOM, M_GetText("Speeding off to..."));
			snprintf(tx, 63, "%s%s%s",
				mapheaderinfo[gamemap-1]->lvlttl,
				(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE) ? "" : " Zone",
				(mapheaderinfo[gamemap-1]->actnum > 0) ? va(" %d",mapheaderinfo[gamemap-1]->actnum) : "");
			V_DrawSmallString(1, 195, V_ALLOWLOWERCASE|V_TRANSLUCENT|V_SNAPTOLEFT|V_SNAPTOBOTTOM, tx);
			I_UpdateNoVsync();
		}

		// As oddly named as this is, this handles music only.
		// We should be fine starting it here.
		// Don't do this during titlemap, because the menu code handles music by itself.
		S_Start();
	}

	levelfadecol = (ranspecialwipe) ? 0 : 31;

	// Close text prompt before freeing the old level
	F_EndTextPrompt(false, true);

	LUA_InvalidateLevel();

	for (ss = sectors; sectors+numsectors != ss; ss++)
	{
		Z_Free(ss->attached);
		Z_Free(ss->attachedsolid);
	}

	// Clear pointers that would be left dangling by the purge
	R_FlushTranslationColormapCache();

#ifdef HWRENDER
	// Free GPU textures before freeing patches.
	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED))
		HWR_ClearAllTextures();
#endif

	Patch_FreeTag(PU_PATCH_LOWPRIORITY);
	Patch_FreeTag(PU_PATCH_ROTATED);
	Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

	R_InitializeLevelInterpolators();

	P_InitThinkers();
	R_InitMobjInterpolators();
	P_InitCachedActions();

	if (!fromnetsave && savedata.lives > 0)
	{
		numgameovers = savedata.numgameovers;
		players[consoleplayer].continues = savedata.continues;
		players[consoleplayer].lives = savedata.lives;
		players[consoleplayer].score = savedata.score;
		if ((botingame = ((botskin = savedata.botskin) != 0)))
			botcolor = skins[botskin-1].prefcolor;
		emeralds = savedata.emeralds;
		savedata.lives = 0;
	}

	// internal game map
	maplumpname = G_BuildMapName(gamemap);
	lastloadedmaplumpnum = W_CheckNumForMap(maplumpname);
	if (lastloadedmaplumpnum == LUMPERROR)
		I_Error("Map %s not found.\n", maplumpname);

	R_ReInitColormaps(mapheaderinfo[gamemap-1]->palette);
	CON_SetupBackColormap();

	// SRB2 determines the sky texture to be used depending on the map header.
	P_SetupLevelSky(mapheaderinfo[gamemap-1]->skynum, true);

	P_ResetSpawnpoints();

	P_ResetWaypoints();

	P_MapStart(); // tmthing can be used starting from this point

	P_InitSlopes();

	if (!P_LoadMapFromFile())
		return false;

	// init anything that P_SpawnSlopes/P_LoadThings needs to know
	P_InitSpecials();

	P_SpawnSlopes(fromnetsave);

	P_SpawnMapThings(!fromnetsave);
	skyboxmo[0] = skyboxviewpnts[0];
	skyboxmo[1] = skyboxcenterpnts[0];

	for (numcoopstarts = 0; numcoopstarts < MAXPLAYERS; numcoopstarts++)
		if (!playerstarts[numcoopstarts])
			break;

	// set up world state
	P_SpawnSpecials(fromnetsave);

	if (!fromnetsave) //  ugly hack for P_NetUnArchiveMisc (and P_LoadNetGame)
		P_SpawnPrecipitation();

#ifdef HWRENDER // not win32 only 19990829 by Kin
	gl_maploaded = false;

	// Lactozilla: Free extrasubsectors regardless of renderer.
	HWR_FreeExtraSubsectors();

	// Create plane polygons.
	if (rendermode == render_opengl)
		HWR_LoadLevel();
#endif

	// oh god I hope this helps
	// (addendum: apparently it does!
	//  none of this needs to be done because it's not the beginning of the map when
	//  a netgame save is being loaded, and could actively be harmful by messing with
	//  the client's view of the data.)
	if (!fromnetsave)
		P_InitGametype();

	if (!reloadinggamestate)
	{
		P_InitCamera();
		localaiming = 0;
		localaiming2 = 0;
	}

	// clear special respawning que
	iquehead = iquetail = 0;

	// Remove the loading shit from the screen
	if (rendermode != render_none && !(titlemapinaction || reloadinggamestate))
		F_WipeColorFill(levelfadecol);

	if (precache || dedicated)
		R_PrecacheLevel();

	nextmapoverride = 0;
	skipstats = 0;

	if (!(netgame || multiplayer || demoplayback) && (!modifiedgame || savemoddata))
		mapvisited[gamemap-1] |= MV_VISITED;
	else if (netgame || multiplayer)
		mapvisited[gamemap-1] |= MV_MP; // you want to record that you've been there this session, but not permanently

	levelloading = false;

	P_RunCachedActions();

	P_MapEnd(); // tmthing is no longer needed from this point onwards

	// Took me 3 hours to figure out why my progression kept on getting overwritten with the titlemap...
	if (!titlemapinaction)
	{
		if (!lastmaploaded) // Start a new game?
		{
			// I'd love to do this in the menu code instead of here, but everything's a mess and I can't guarantee saving proper player struct info before the first act's started. You could probably refactor it, but it'd be a lot of effort. Easier to just work off known good code. ~toast 22/06/2020
			if (!(ultimatemode || netgame || multiplayer || demoplayback || demorecording || metalrecording || modeattacking || marathonmode)
			&& (!modifiedgame || savemoddata) && cursaveslot > 0)
				G_SaveGame((UINT32)cursaveslot, gamemap);
			// If you're looking for saving sp file progression (distinct from G_SaveGameOver), check G_DoCompleted.
		}
		lastmaploaded = gamemap; // HAS to be set after saving!!
	}

	if (!fromnetsave) // uglier hack
	{ // to make a newly loaded level start on the second frame.
		INT32 buf = gametic % BACKUPTICS;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				G_CopyTiccmd(&players[i].cmd, &netcmds[buf][i], 1);
		}
		P_PreTicker(2);
		P_MapStart(); // just in case MapLoad modifies tmthing
		LUA_HookInt(gamemap, HOOK(MapLoad));
		P_MapEnd(); // just in case MapLoad modifies tmthing
	}

	// No render mode or reloading gamestate, stop here.
	if (rendermode == render_none || reloadinggamestate)
		return true;

	R_ResetViewInterpolation(0);
	R_ResetViewInterpolation(0);
	R_UpdateMobjInterpolators();

	// Title card!
	G_StartTitleCard();

	// Can the title card actually run, though?
	if (!WipeStageTitle)
		return true;
	if (ranspecialwipe == 2)
		return true;

	// If so...
	G_PreLevelTitleCard();

	return true;
}

//
// P_RunSOC
//
// Runs a SOC file or a lump, depending on if ".SOC" exists in the filename
//
boolean P_RunSOC(const char *socfilename)
{
	lumpnum_t lump;

	if (strstr(socfilename, ".soc") != NULL)
		return P_AddWadFile(socfilename);

	lump = W_CheckNumForName(socfilename);
	if (lump == LUMPERROR)
		return false;

	CONS_Printf(M_GetText("Loading SOC lump: %s\n"), socfilename);
	DEH_LoadDehackedLump(lump);

	return true;
}

// Auxiliary function for PK3 loading - looks for sound replacements.
// NOTE: it does not really add any new sound entry or anything.
void P_LoadSoundsRange(UINT16 wadnum, UINT16 first, UINT16 num)
{
	size_t j;
	lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo + first;
	for (; num > 0; num--, lumpinfo++)
	{
		// Let's check whether it's replacing an existing sound or it's a brand new one.
		for (j = 1; j < NUMSFX; j++)
		{
			if (S_sfx[j].name && !strnicmp(S_sfx[j].name, lumpinfo->name + 2, 6))
			{
				// the sound will be reloaded when needed,
				// since sfx->data will be NULL
				CONS_Debug(DBG_SETUP, "Sound %.8s replaced\n", lumpinfo->name);

				I_FreeSfx(&S_sfx[j]);
				break; // there shouldn't be two sounds with the same name, so stop looking
			}
		}
	}
}

// Auxiliary function for PK3 loading - looks for music and music replacements.
// NOTE: does nothing but print debug messages. The code is handled somewhere else.
void P_LoadMusicsRange(UINT16 wadnum, UINT16 first, UINT16 num)
{
	lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo + first;
	char *name;
	for (; num > 0; num--, lumpinfo++)
	{
		name = lumpinfo->name;
		if (name[0] == 'O' && name[1] == '_')
		{
			CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
		}
		else if (name[0] == 'D' && name[1] == '_')
		{
			CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
		}
	}
	return;
}

// Auxiliary function - input a folder name and gives us the resource markers positions.
static lumpinfo_t* FindFolder(const char *folName, UINT16 *start, UINT16 *end, lumpinfo_t *lumpinfo, UINT16 *pnumlumps, size_t *pi)
{
	UINT16 numlumps = *pnumlumps;
	size_t i = *pi;
	if (!stricmp(lumpinfo->fullname, folName))
	{
		lumpinfo++;
		*start = ++i;
		for (; i < numlumps; i++, lumpinfo++)
			if (strnicmp(lumpinfo->fullname, folName, strlen(folName)))
				break;
		lumpinfo--;
		*end = i-- - *start;
		*pi = i;
		*pnumlumps = numlumps;
		return lumpinfo;
	}
	return lumpinfo;
}

//
// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
static boolean P_LoadAddon(UINT16 numlumps)
{
	const UINT16 wadnum = (UINT16)(numwadfiles-1);

	size_t i, j, sreplaces = 0, mreplaces = 0, digmreplaces = 0;
	char *name;
	lumpinfo_t *lumpinfo;

	//boolean texturechange = false; ///\todo Useless; broken when back-frontporting PK3 changes?
	boolean mapsadded = false;
	boolean replacedcurrentmap = false;

	// Vars to help us with the position start and amount of each resource type.
	// Useful for PK3s since they use folders.
	// WADs use markers for some resources, but others such as sounds are checked lump-by-lump anyway.
//	UINT16 luaPos, luaNum = 0;
//	UINT16 socPos, socNum = 0;
	UINT16 sfxPos = 0, sfxNum = 0;
	UINT16 musPos = 0, musNum = 0;
//	UINT16 sprPos, sprNum = 0;
	UINT16 texPos = 0, texNum = 0;
//	UINT16 patPos, patNum = 0;
//	UINT16 flaPos, flaNum = 0;
//	UINT16 mapPos, mapNum = 0;

	if (numlumps == INT16_MAX)
	{
		refreshdirmenu |= REFRESHDIR_NOTLOADED;
		return false;
	}

	switch(wadfiles[wadnum]->type)
	{
	case RET_PK3:
	case RET_FOLDER:
		// Look for the lumps that act as resource delimitation markers.
		lumpinfo = wadfiles[wadnum]->lumpinfo;
		for (i = 0; i < numlumps; i++, lumpinfo++)
		{
//			lumpinfo = FindFolder("Lua/",      &luaPos, &luaNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("SOC/",      &socPos, &socNum, lumpinfo, &numlumps, &i);
			lumpinfo = FindFolder("Sounds/",   &sfxPos, &sfxNum, lumpinfo, &numlumps, &i);
			lumpinfo = FindFolder("Music/",    &musPos, &musNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Sprites/",  &sprPos, &sprNum, lumpinfo, &numlumps, &i);
			lumpinfo = FindFolder("Textures/", &texPos, &texNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Patches/",  &patPos, &patNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Flats/",    &flaPos, &flaNum, lumpinfo, &numlumps, &i);
//			lumpinfo = FindFolder("Maps/",     &mapPos, &mapNum, lumpinfo, &numlumps, &i);
		}

		// Update the detected resources.
		// Note: ALWAYS load Lua scripts first, SOCs right after, and the remaining resources afterwards.
//		if (luaNum) // Lua scripts.
//			P_LoadLuaScrRange(wadnum, luaPos, luaNum);
//		if (socNum) // SOCs.
//			P_LoadDehackRange(wadnum, socPos, socNum);
		if (sfxNum) // Sounds. TODO: Function currently only updates already existing sounds, the rest is handled somewhere else.
			P_LoadSoundsRange(wadnum, sfxPos, sfxNum);
		if (musNum) // Music. TODO: Useless function right now.
			P_LoadMusicsRange(wadnum, musPos, musNum);
//		if (sprNum) // Sprites.
//			R_LoadSpritsRange(wadnum, sprPos, sprNum);
//		if (texNum) // Textures. TODO: R_LoadTextures() does the folder positioning once again. New function maybe?
//			R_LoadTextures();
//		if (mapNum) // Maps. TODO: Actually implement the map WAD loading code, lulz.
//			P_LoadWadMapRange(wadnum, mapPos, mapNum);
		break;
	default:
		lumpinfo = wadfiles[wadnum]->lumpinfo;
		for (i = 0; i < numlumps; i++, lumpinfo++)
		{
			name = lumpinfo->name;
			if (name[0] == 'D')
			{
				if (name[1] == 'S')
				{
					for (j = 1; j < NUMSFX; j++)
					{
						if (S_sfx[j].name && !strnicmp(S_sfx[j].name, name + 2, 6))
						{
							// the sound will be reloaded when needed,
							// since sfx->data will be NULL
							CONS_Debug(DBG_SETUP, "Sound %.8s replaced\n", name);

							I_FreeSfx(&S_sfx[j]);

							sreplaces++;
							break; // there shouldn't be two sounds with the same name, so stop looking
						}
					}
				}
				else if (name[1] == '_')
				{
					CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
					mreplaces++;
				}
			}
			else if (name[0] == 'O' && name[1] == '_')
			{
				CONS_Debug(DBG_SETUP, "Music %.8s replaced\n", name);
				digmreplaces++;
			}
		}
		break;
	}
	if (!devparm && sreplaces)
		CONS_Printf(M_GetText("%s sounds replaced\n"), sizeu1(sreplaces));
	if (!devparm && mreplaces)
		CONS_Printf(M_GetText("%s midi musics replaced\n"), sizeu1(mreplaces));
	if (!devparm && digmreplaces)
		CONS_Printf(M_GetText("%s digital musics replaced\n"), sizeu1(digmreplaces));

#ifdef HWRENDER
	// Free GPU textures before freeing patches.
	if (rendermode == render_opengl && (vid.glstate == VID_GL_LIBRARY_LOADED))
		HWR_ClearAllTextures();
#endif

	//
	// search for sprite replacements
	//
	Patch_FreeTag(PU_SPRITE);
	Patch_FreeTag(PU_PATCH_ROTATED);
	R_AddSpriteDefs(wadnum);

	// Reload it all anyway, just in case they
	// added some textures but didn't insert a
	// TEXTURES/etc. list.
	R_LoadTexturesPwad(wadnum); // numtexture changes

	// Reload ANIMDEFS
	P_InitPicAnims();

	// Flush and reload HUD graphics
	ST_UnloadGraphics();
	HU_LoadGraphics();
	ST_LoadGraphics();

	//
	// look for skins
	//
	R_AddSkins(wadnum, false); // faB: wadfile index in wadfiles[]
	R_PatchSkins(wadnum, false); // toast: PATCH PATCH
	ST_ReloadSkinFaceGraphics();

	//
	// edit music defs
	//
	S_LoadMusicDefs(wadnum);

	//
	// search for maps
	//
	lumpinfo = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		if (name[0] == 'M' && name[1] == 'A' && name[2] == 'P') // Ignore the headers
		{
			INT16 num;
			if (name[5]!='\0')
				continue;
			num = (INT16)M_MapNumber(name[3], name[4]);

			//If you replaced the map you're on, end the level when done.
			if (num == gamemap)
				replacedcurrentmap = true;

			CONS_Printf("%s\n", name);
			mapsadded = true;
		}
	}
	if (!mapsadded)
		CONS_Printf(M_GetText("No maps added\n"));

	R_LoadSpriteInfoLumps(wadnum, numlumps);

#ifdef HWRENDER
	HWR_ReloadModels();
#endif

	// reload status bar (warning should have valid player!)
	if (gamestate == GS_LEVEL)
		ST_Start();

	// Prevent savefile cheating
	if (cursaveslot > 0)
		cursaveslot = 0;

	if (replacedcurrentmap && gamestate == GS_LEVEL && (netgame || multiplayer))
	{
		CONS_Printf(M_GetText("Current map %d replaced by added file, ending the level to ensure consistency.\n"), gamemap);
		if (server)
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}

	return true;
}

boolean P_AddWadFile(const char *wadfilename)
{
	return D_CheckPathAllowed(wadfilename, "tried to add file") &&
		P_LoadAddon(W_InitFile(wadfilename, false, false));
}

boolean P_AddFolder(const char *folderpath)
{
	return D_CheckPathAllowed(folderpath, "tried to add folder") &&
		P_LoadAddon(W_InitFolder(folderpath, false, false));
}
