// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2016 by Sonic Team Junior.
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

#include "i_sound.h" // for I_PlayCD()..
#include "i_video.h" // for I_FinishUpdate()..
#include "r_sky.h"
#include "i_system.h"

#include "r_data.h"
#include "r_things.h"
#include "r_sky.h"
#include "r_draw.h"

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

// wipes
#include "f_finale.h"

#include "md5.h" // map MD5

// for LUAh_MapLoad
#include "lua_script.h"
#include "lua_hook.h"

#if defined (_WIN32) || defined (_WIN32_WCE)
#include <malloc.h>
#include <math.h>
#endif
#ifdef HWRENDER
#include "hardware/hw_main.h"
#include "hardware/hw_light.h"
#endif

#ifdef ESLOPE
#include "p_slopes.h"
#endif

//
// Map MD5, calculated on level load.
// Sent to clients in PT_SERVERINFO.
//
unsigned char mapmd5[16];

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

size_t numvertexes, numsegs, numsectors, numsubsectors, numnodes, numlines, numsides, nummapthings;
vertex_t *vertexes;
seg_t *segs;
sector_t *sectors;
subsector_t *subsectors;
node_t *nodes;
line_t *lines;
side_t *sides;
mapthing_t *mapthings;
INT32 numstarposts;
boolean levelloading;

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

#define NUMLAPS_DEFAULT 4

/** Clears the data from a single map header.
  *
  * \param i Map number to clear header for.
  * \sa P_ClearMapHeaderInfo
  */
static void P_ClearSingleMapHeaderInfo(INT16 i)
{
	const INT16 num = (INT16)(i-1);
	DEH_WriteUndoline("LEVELNAME", mapheaderinfo[num]->lvlttl, UNDO_NONE);
	mapheaderinfo[num]->lvlttl[0] = '\0';
	DEH_WriteUndoline("SUBTITLE", mapheaderinfo[num]->subttl, UNDO_NONE);
	mapheaderinfo[num]->subttl[0] = '\0';
	DEH_WriteUndoline("ACT", va("%d", mapheaderinfo[num]->actnum), UNDO_NONE);
	mapheaderinfo[num]->actnum = 0;
	DEH_WriteUndoline("TYPEOFLEVEL", va("%d", mapheaderinfo[num]->typeoflevel), UNDO_NONE);
	mapheaderinfo[num]->typeoflevel = 0;
	DEH_WriteUndoline("NEXTLEVEL", va("%d", mapheaderinfo[num]->nextlevel), UNDO_NONE);
	mapheaderinfo[num]->nextlevel = (INT16)(i + 1);
	DEH_WriteUndoline("MUSIC", mapheaderinfo[num]->musname, UNDO_NONE);
	snprintf(mapheaderinfo[num]->musname, 7, "%sM", G_BuildMapName(i));
	mapheaderinfo[num]->musname[6] = 0;
	DEH_WriteUndoline("MUSICTRACK", va("%d", mapheaderinfo[num]->mustrack), UNDO_NONE);
	mapheaderinfo[num]->mustrack = 0;
	DEH_WriteUndoline("FORCECHARACTER", va("%d", mapheaderinfo[num]->forcecharacter), UNDO_NONE);
	mapheaderinfo[num]->forcecharacter[0] = '\0';
	DEH_WriteUndoline("WEATHER", va("%d", mapheaderinfo[num]->weather), UNDO_NONE);
	mapheaderinfo[num]->weather = 0;
	DEH_WriteUndoline("SKYNUM", va("%d", mapheaderinfo[num]->skynum), UNDO_NONE);
	mapheaderinfo[num]->skynum = 1;
	DEH_WriteUndoline("SKYBOXSCALEX", va("%d", mapheaderinfo[num]->skybox_scalex), UNDO_NONE);
	mapheaderinfo[num]->skybox_scalex = 16;
	DEH_WriteUndoline("SKYBOXSCALEY", va("%d", mapheaderinfo[num]->skybox_scaley), UNDO_NONE);
	mapheaderinfo[num]->skybox_scaley = 16;
	DEH_WriteUndoline("SKYBOXSCALEZ", va("%d", mapheaderinfo[num]->skybox_scalez), UNDO_NONE);
	mapheaderinfo[num]->skybox_scalez = 16;
	DEH_WriteUndoline("INTERSCREEN", mapheaderinfo[num]->interscreen, UNDO_NONE);
	mapheaderinfo[num]->interscreen[0] = '#';
	DEH_WriteUndoline("RUNSOC", mapheaderinfo[num]->runsoc, UNDO_NONE);
	mapheaderinfo[num]->runsoc[0] = '#';
	DEH_WriteUndoline("SCRIPTNAME", mapheaderinfo[num]->scriptname, UNDO_NONE);
	mapheaderinfo[num]->scriptname[0] = '#';
	DEH_WriteUndoline("PRECUTSCENENUM", va("%d", mapheaderinfo[num]->precutscenenum), UNDO_NONE);
	mapheaderinfo[num]->precutscenenum = 0;
	DEH_WriteUndoline("CUTSCENENUM", va("%d", mapheaderinfo[num]->cutscenenum), UNDO_NONE);
	mapheaderinfo[num]->cutscenenum = 0;
	DEH_WriteUndoline("COUNTDOWN", va("%d", mapheaderinfo[num]->countdown), UNDO_NONE);
	mapheaderinfo[num]->countdown = 0;
	DEH_WriteUndoline("PALLETE", va("%u", mapheaderinfo[num]->palette), UNDO_NONE);
	mapheaderinfo[num]->palette = UINT16_MAX;
	DEH_WriteUndoline("NUMLAPS", va("%u", mapheaderinfo[num]->numlaps), UNDO_NONE);
	mapheaderinfo[num]->numlaps = NUMLAPS_DEFAULT;
	DEH_WriteUndoline("UNLOCKABLE", va("%s", mapheaderinfo[num]->unlockrequired), UNDO_NONE);
	mapheaderinfo[num]->unlockrequired = -1;
	DEH_WriteUndoline("LEVELSELECT", va("%d", mapheaderinfo[num]->levelselect), UNDO_NONE);
	mapheaderinfo[num]->levelselect = 0;
	DEH_WriteUndoline("BONUSTYPE", va("%d", mapheaderinfo[num]->bonustype), UNDO_NONE);
	mapheaderinfo[num]->bonustype = 0;
	DEH_WriteUndoline("LEVELFLAGS", va("%d", mapheaderinfo[num]->levelflags), UNDO_NONE);
	mapheaderinfo[num]->levelflags = 0;
	DEH_WriteUndoline("MENUFLAGS", va("%d", mapheaderinfo[num]->menuflags), UNDO_NONE);
	mapheaderinfo[num]->menuflags = 0;
	// TODO grades support for delfile (pfft yeah right)
	P_DeleteGrades(num);
	// an even further impossibility, delfile custom opts support
	mapheaderinfo[num]->customopts = NULL;
	mapheaderinfo[num]->numCustomOptions = 0;

	DEH_WriteUndoline(va("# uload for map %d", i), NULL, UNDO_DONE);
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

/** Loads the vertexes for a level.
  *
  * \param lump VERTEXES lump number.
  * \sa ML_VERTEXES
  */
static inline void P_LoadVertexes(lumpnum_t lumpnum)
{
	UINT8 *data;
	size_t i;
	mapvertex_t *ml;
	vertex_t *li;

	// Determine number of lumps:
	//  total lump length / vertex record length.
	numvertexes = W_LumpLength(lumpnum) / sizeof (mapvertex_t);

	if (numvertexes <= 0)
		I_Error("Level has no vertices"); // instead of crashing

	// Allocate zone memory for buffer.
	vertexes = Z_Calloc(numvertexes * sizeof (*vertexes), PU_LEVEL, NULL);

	// Load data into cache.
	data = W_CacheLumpNum(lumpnum, PU_STATIC);

	ml = (mapvertex_t *)data;
	li = vertexes;

	// Copy and convert vertex coordinates, internal representation as fixed.
	for (i = 0; i < numvertexes; i++, li++, ml++)
	{
		li->x = SHORT(ml->x)<<FRACBITS;
		li->y = SHORT(ml->y)<<FRACBITS;
	}

	// Free buffer memory.
	Z_Free(data);
}

//
// Computes the line length in fracunits, the OpenGL render needs this
//

/** Computes the length of a seg in fracunits.
  * This is needed for splats.
  *
  * \param seg Seg to compute length for.
  * \return Length in fracunits.
  */
fixed_t P_SegLength(seg_t *seg)
{
	fixed_t dx, dy;

	// make a vector (start at origin)
	dx = seg->v2->x - seg->v1->x;
	dy = seg->v2->y - seg->v1->y;

	return FixedHypot(dx, dy);
}

#ifdef HWRENDER
static inline float P_SegLengthf(seg_t *seg)
{
	float dx, dy;

	// make a vector (start at origin)
	dx = FIXED_TO_FLOAT(seg->v2->x - seg->v1->x);
	dy = FIXED_TO_FLOAT(seg->v2->y - seg->v1->y);

	return (float)hypot(dx, dy);
}
#endif

/** Loads the SEGS resource from a level.
  *
  * \param lump Lump number of the SEGS resource.
  * \sa ::ML_SEGS
  */
static void P_LoadSegs(lumpnum_t lumpnum)
{
	UINT8 *data;
	size_t i;
	INT32 linedef, side;
	mapseg_t *ml;
	seg_t *li;
	line_t *ldef;

	numsegs = W_LumpLength(lumpnum) / sizeof (mapseg_t);
	if (numsegs <= 0)
		I_Error("Level has no segs"); // instead of crashing
	segs = Z_Calloc(numsegs * sizeof (*segs), PU_LEVEL, NULL);
	data = W_CacheLumpNum(lumpnum, PU_STATIC);

	ml = (mapseg_t *)data;
	li = segs;
	for (i = 0; i < numsegs; i++, li++, ml++)
	{
		li->v1 = &vertexes[SHORT(ml->v1)];
		li->v2 = &vertexes[SHORT(ml->v2)];

#ifdef HWRENDER // not win32 only 19990829 by Kin
		// used for the hardware render
		if (rendermode != render_soft && rendermode != render_none)
		{
			li->flength = P_SegLengthf(li);
			//Hurdler: 04/12/2000: for now, only used in hardware mode
			li->lightmaps = NULL; // list of static lightmap for this seg
		}
#endif

		li->angle = (SHORT(ml->angle))<<FRACBITS;
		li->offset = (SHORT(ml->offset))<<FRACBITS;
		linedef = SHORT(ml->linedef);
		ldef = &lines[linedef];
		li->linedef = ldef;
		li->side = side = SHORT(ml->side);
		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;
		if (ldef-> flags & ML_TWOSIDED)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
			li->backsector = 0;

		li->numlights = 0;
		li->rlights = NULL;
	}

	Z_Free(data);
}

/** Loads the SSECTORS resource from a level.
  *
  * \param lump Lump number of the SSECTORS resource.
  * \sa ::ML_SSECTORS
  */
static inline void P_LoadSubsectors(lumpnum_t lumpnum)
{
	void *data;
	size_t i;
	mapsubsector_t *ms;
	subsector_t *ss;

	numsubsectors = W_LumpLength(lumpnum) / sizeof (mapsubsector_t);
	if (numsubsectors <= 0)
		I_Error("Level has no subsectors (did you forget to run it through a nodesbuilder?)");
	ss = subsectors = Z_Calloc(numsubsectors * sizeof (*subsectors), PU_LEVEL, NULL);
	data = W_CacheLumpNum(lumpnum,PU_STATIC);

	ms = (mapsubsector_t *)data;

	for (i = 0; i < numsubsectors; i++, ss++, ms++)
	{
		ss->sector = NULL;
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = SHORT(ms->firstseg);
#ifdef FLOORSPLATS
		ss->splats = NULL;
#endif
		ss->validcount = 0;
	}

	Z_Free(data);
}

//
// P_LoadSectors
//

//
// levelflats
//
#define MAXLEVELFLATS 256

size_t numlevelflats;
levelflat_t *levelflats;

//SoM: Other files want this info.
size_t P_PrecacheLevelFlats(void)
{
	lumpnum_t lump;
	size_t i, flatmemory = 0;

	//SoM: 4/18/2000: New flat code to make use of levelflats.
	for (i = 0; i < numlevelflats; i++)
	{
		lump = levelflats[i].lumpnum;
		if (devparm)
			flatmemory += W_LumpLength(lump);
		R_GetFlat(lump);
	}
	return flatmemory;
}

// help function for P_LoadSectors, find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
//
INT32 P_AddLevelFlat(const char *flatname, levelflat_t *levelflat)
{
	size_t i;

	//
	//  first scan through the already found flats
	//
	for (i = 0; i < numlevelflats; i++, levelflat++)
		if (strnicmp(levelflat->name,flatname,8)==0)
			break;

	// that flat was already found in the level, return the id
	if (i == numlevelflats)
	{
		// store the name
		strlcpy(levelflat->name, flatname, sizeof (levelflat->name));
		strupr(levelflat->name);

		// store the flat lump number
		levelflat->lumpnum = R_GetFlatNumForName(flatname);

#ifndef ZDEBUG
		CONS_Debug(DBG_SETUP, "flat #%03d: %s\n", atoi(sizeu1(numlevelflats)), levelflat->name);
#endif

		numlevelflats++;

		if (numlevelflats >= MAXLEVELFLATS)
			I_Error("Too many flats in level\n");
	}

	// level flat id
	return (INT32)i;
}

// help function for Lua and $$$.sav reading
// same as P_AddLevelFlat, except this is not setup so we must realloc levelflats to fit in the new flat
// no longer a static func in lua_maplib.c because p_saveg.c also needs it
//
INT32 P_AddLevelFlatRuntime(const char *flatname)
{
	size_t i;
	levelflat_t *levelflat = levelflats;

	//
	//  first scan through the already found flats
	//
	for (i = 0; i < numlevelflats; i++, levelflat++)
		if (strnicmp(levelflat->name,flatname,8)==0)
			break;

	// that flat was already found in the level, return the id
	if (i == numlevelflats)
	{
		// allocate new flat memory
		levelflats = Z_Realloc(levelflats, (numlevelflats + 1) * sizeof(*levelflats), PU_LEVEL, NULL);
		levelflat = levelflats+i;

		// store the name
		strlcpy(levelflat->name, flatname, sizeof (levelflat->name));
		strupr(levelflat->name);

		// store the flat lump number
		levelflat->lumpnum = R_GetFlatNumForName(flatname);

#ifndef ZDEBUG
		CONS_Debug(DBG_SETUP, "flat #%03d: %s\n", atoi(sizeu1(numlevelflats)), levelflat->name);
#endif

		numlevelflats++;
	}

	// level flat id
	return (INT32)i;
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

static void P_LoadSectors(lumpnum_t lumpnum)
{
	UINT8 *data;
	size_t i;
	mapsector_t *ms;
	sector_t *ss;
	levelflat_t *foundflats;

	numsectors = W_LumpLength(lumpnum) / sizeof (mapsector_t);
	if (numsectors <= 0)
		I_Error("Level has no sectors");
	sectors = Z_Calloc(numsectors*sizeof (*sectors), PU_LEVEL, NULL);
	data = W_CacheLumpNum(lumpnum,PU_STATIC);

	//Fab : FIXME: allocate for whatever number of flats
	//           512 different flats per level should be plenty

	foundflats = calloc(MAXLEVELFLATS, sizeof (*foundflats));
	if (foundflats == NULL)
		I_Error("Ran out of memory while loading sectors\n");

	numlevelflats = 0;

	ms = (mapsector_t *)data;
	ss = sectors;
	for (i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

		//
		//  flats
		//
		ss->floorpic = P_AddLevelFlat(ms->floorpic, foundflats);
		ss->ceilingpic = P_AddLevelFlat(ms->ceilingpic, foundflats);

		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->nexttag = ss->firsttag = -1;
		ss->spawn_nexttag = ss->spawn_firsttag = -1;

		memset(&ss->soundorg, 0, sizeof(ss->soundorg));
		ss->validcount = 0;

		ss->thinglist = NULL;
		ss->touching_thinglist = NULL;
		ss->preciplist = NULL;
		ss->touching_preciplist = NULL;

		ss->floordata = NULL;
		ss->ceilingdata = NULL;
		ss->lightingdata = NULL;

		ss->linecount = 0;
		ss->lines = NULL;

		ss->heightsec = -1;
		ss->camsec = -1;
		ss->floorlightsec = -1;
		ss->ceilinglightsec = -1;
		ss->crumblestate = 0;
		ss->ffloors = NULL;
		ss->lightlist = NULL;
		ss->numlights = 0;
		ss->attached = NULL;
		ss->attachedsolid = NULL;
		ss->numattached = 0;
		ss->maxattached = 1;
		ss->moved = true;

		ss->extra_colormap = NULL;

		ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
		ss->spawn_flr_xoffs = ss->spawn_ceil_xoffs = ss->spawn_flr_yoffs = ss->spawn_ceil_yoffs = 0;
		ss->floorpic_angle = ss->ceilingpic_angle = 0;
		ss->spawn_flrpic_angle = ss->spawn_ceilpic_angle = 0;
		ss->bottommap = ss->midmap = ss->topmap = -1;
		ss->gravity = NULL;
		ss->cullheight = NULL;
		ss->verticalflip = false;
		ss->flags = 0;
		ss->flags |= SF_FLIPSPECIAL_FLOOR;

		ss->floorspeed = 0;
		ss->ceilspeed = 0;

#ifdef HWRENDER // ----- for special tricks with HW renderer -----
		ss->pseudoSector = false;
		ss->virtualFloor = false;
		ss->virtualCeiling = false;
		ss->sectorLines = NULL;
		ss->stackList = NULL;
		ss->lineoutLength = -1.0l;
#endif // ----- end special tricks -----
	}

	Z_Free(data);

	// set the sky flat num
	skyflatnum = P_AddLevelFlat(SKYFLATNAME, foundflats);

	// copy table for global usage
	levelflats = M_Memcpy(Z_Calloc(numlevelflats * sizeof (*levelflats), PU_LEVEL, NULL), foundflats, numlevelflats * sizeof (levelflat_t));
	free(foundflats);

	// search for animated flats and set up
	P_SetupLevelFlatAnims();
}

//
// P_LoadNodes
//
static void P_LoadNodes(lumpnum_t lumpnum)
{
	UINT8 *data;
	size_t i;
	UINT8 j, k;
	mapnode_t *mn;
	node_t *no;

	numnodes = W_LumpLength(lumpnum) / sizeof (mapnode_t);
	if (numnodes <= 0)
		I_Error("Level has no nodes");
	nodes = Z_Calloc(numnodes * sizeof (*nodes), PU_LEVEL, NULL);
	data = W_CacheLumpNum(lumpnum, PU_STATIC);

	mn = (mapnode_t *)data;
	no = nodes;

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

	Z_Free(data);
}

//
// P_ReloadRings
// Used by NiGHTS, clears all ring/wing/etc items and respawns them
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

	// scan the thinkers to find rings/wings/hoops to unset
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
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
		if (!(mo->type == MT_RING || mo->type == MT_NIGHTSWING || mo->type == MT_COIN || mo->type == MT_BLUEBALL))
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
		if (mt->type == 300 || mt->type == 308 || mt->type == 309
		 || mt->type == 1706 || (mt->type >= 600 && mt->type <= 609)
		 || mt->type == 1800)
		{
			mt->mobj = NULL;

			// Z for objects Tails 05-26-2002
			mt->z = (INT16)(R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)
				->sector->floorheight>>FRACBITS);

			P_SpawnHoopsAndRings (mt);
		}
	}
	for (i = 0; i < numHoops; i++)
	{
		P_SpawnHoopsAndRings(hoopsToRespawn[i]);
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

//
// P_LoadThings
//
static void P_PrepareThings(lumpnum_t lumpnum)
{
	size_t i;
	mapthing_t *mt;
	UINT8 *data, *datastart;

	nummapthings = W_LumpLength(lumpnum) / (5 * sizeof (INT16));
	mapthings = Z_Calloc(nummapthings * sizeof (*mapthings), PU_LEVEL, NULL);

	// Spawn axis points first so they are
	// at the front of the list for fast searching.
	data = datastart = W_CacheLumpNum(lumpnum, PU_LEVEL);
	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		mt->x = READINT16(data);
		mt->y = READINT16(data);
		mt->angle = READINT16(data);
		mt->type = READUINT16(data);
		mt->options = READUINT16(data);
		mt->extrainfo = (UINT8)(mt->type >> 12);

		mt->type &= 4095;

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
	Z_Free(datastart);

}

static void P_LoadThings(void)
{
	size_t i;
	mapthing_t *mt;

	// Loading the things lump itself into memory is now handled in P_PrepareThings, above

	mt = mapthings;
	numhuntemeralds = 0;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		sector_t *mtsector = R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)->sector;

		// Z for objects
		mt->z = (INT16)(
#ifdef ESLOPE
				mtsector->f_slope ? P_GetZAt(mtsector->f_slope, mt->x << FRACBITS, mt->y << FRACBITS) :
#endif
				mtsector->floorheight)>>FRACBITS;

		if (mt->type == 1700 // MT_AXIS
			|| mt->type == 1701 // MT_AXISTRANSFER
			|| mt->type == 1702) // MT_AXISTRANSFERLINE
			continue; // These were already spawned

		mt->mobj = NULL;
		P_SpawnMapThing(mt);
	}

	// random emeralds for hunt
	if (numhuntemeralds)
	{
		INT32 emer1, emer2, emer3;
		INT32 timeout = 0; // keeps from getting stuck

		emer1 = emer2 = emer3 = 0;

		//increment spawn numbers because zero is valid.
		emer1 = (P_RandomKey(numhuntemeralds)) + 1;
		while (timeout++ < 100)
		{
			emer2 = (P_RandomKey(numhuntemeralds)) + 1;

			if (emer2 != emer1)
				break;
		}

		timeout = 0;
		while (timeout++ < 100)
		{
			emer3 = (P_RandomKey(numhuntemeralds)) + 1;

			if (emer3 != emer2 && emer3 != emer1)
				break;
		}

		//decrement spawn values to the actual number because zero is valid.
		if (emer1)
			P_SpawnMobj(huntemeralds[emer1 - 1]->x<<FRACBITS,
				huntemeralds[emer1 - 1]->y<<FRACBITS,
				huntemeralds[emer1 - 1]->z<<FRACBITS, MT_EMERHUNT);

		if (emer2)
			P_SpawnMobj(huntemeralds[emer2 - 1]->x<<FRACBITS,
				huntemeralds[emer2 - 1]->y<<FRACBITS,
				huntemeralds[emer2 - 1]->z<<FRACBITS, MT_EMERHUNT);

		if (emer3)
			P_SpawnMobj(huntemeralds[emer3 - 1]->x<<FRACBITS,
				huntemeralds[emer3 - 1]->y<<FRACBITS,
				huntemeralds[emer3 - 1]->z<<FRACBITS, MT_EMERHUNT);
	}

	if (metalrecording) // Metal Sonic gets no rings to distract him.
		return;

	// Run through the list of mapthings again to spawn hoops and rings
	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		if (mt->type == 300 || mt->type == 308 || mt->type == 309
		 || mt->type == 1706 || (mt->type >= 600 && mt->type <= 609)
		 || mt->type == 1705 || mt->type == 1713 || mt->type == 1800)
		{
			mt->mobj = NULL;

			// Z for objects Tails 05-26-2002
			mt->z = (INT16)(R_PointInSubsector(mt->x << FRACBITS, mt->y << FRACBITS)
				->sector->floorheight>>FRACBITS);

			P_SpawnHoopsAndRings (mt);
		}
	}
}

static inline void P_SpawnEmblems(void)
{
	INT32 i, color;
	mobj_t *emblemmobj;

	for (i = 0; i < numemblems; i++)
	{
		if (emblemlocations[i].level != gamemap || emblemlocations[i].type > ET_SKIN)
			continue;

		emblemmobj = P_SpawnMobj(emblemlocations[i].x<<FRACBITS, emblemlocations[i].y<<FRACBITS,
			emblemlocations[i].z<<FRACBITS, MT_EMBLEM);

		I_Assert(emblemlocations[i].sprite >= 'A' && emblemlocations[i].sprite <= 'Z');
		P_SetMobjStateNF(emblemmobj, emblemmobj->info->spawnstate + (emblemlocations[i].sprite - 'A'));

		emblemmobj->health = i+1;
		color = M_GetEmblemColor(&emblemlocations[i]);

		emblemmobj->color = (UINT8)color;

		if (emblemlocations[i].collected
			|| (emblemlocations[i].type == ET_SKIN && emblemlocations[i].var != players[0].skin))
		{
			P_UnsetThingPosition(emblemmobj);
			emblemmobj->flags |= MF_NOCLIP;
			emblemmobj->flags &= ~MF_SPECIAL;
			emblemmobj->flags |= MF_NOBLOCKMAP;
			emblemmobj->frame |= (tr_trans50<<FF_TRANSSHIFT);
			P_SetThingPosition(emblemmobj);
		}
		else
			emblemmobj->frame &= ~FF_TRANSMASK;
	}
}

static void P_SpawnSecretItems(boolean loademblems)
{
	// Now let's spawn those funky emblem things! Tails 12-08-2002
	if (netgame || multiplayer || (modifiedgame && !savemoddata)) // No cheating!!
		return;

	if (loademblems)
		P_SpawnEmblems();
}

// Experimental groovy write function!
void P_WriteThings(lumpnum_t lumpnum)
{
	size_t i, length;
	mapthing_t *mt;
	UINT8 *data;
	UINT8 *savebuffer, *savebuf_p;
	INT16 temp;

	data = W_CacheLumpNum(lumpnum, PU_LEVEL);

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

	Z_Free(data);

	length = savebuf_p - savebuffer;

	FIL_WriteFile(va("newthings%d.lmp", gamemap), savebuffer, length);
	free(savebuffer);
	savebuf_p = NULL;

	CONS_Printf(M_GetText("newthings%d.lmp saved.\n"), gamemap);
}

//
// P_LoadLineDefs
//
static void P_LoadLineDefs(lumpnum_t lumpnum)
{
	UINT8 *data;
	size_t i;
	maplinedef_t *mld;
	line_t *ld;
	vertex_t *v1, *v2;

	numlines = W_LumpLength(lumpnum) / sizeof (maplinedef_t);
	if (numlines <= 0)
		I_Error("Level has no linedefs");
	lines = Z_Calloc(numlines * sizeof (*lines), PU_LEVEL, NULL);
	data = W_CacheLumpNum(lumpnum, PU_STATIC);

	mld = (maplinedef_t *)data;
	ld = lines;
	for (i = 0; i < numlines; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);
		ld->special = SHORT(mld->special);
		ld->tag = SHORT(mld->tag);
		v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
		v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
		ld->dx = v2->x - v1->x;
		ld->dy = v2->y - v1->y;

#ifdef WALLSPLATS
		ld->splats = NULL;
#endif

		if (!ld->dx)
			ld->slopetype = ST_VERTICAL;
		else if (!ld->dy)
			ld->slopetype = ST_HORIZONTAL;
		else if (FixedDiv(ld->dy, ld->dx) > 0)
			ld->slopetype = ST_POSITIVE;
		else
			ld->slopetype = ST_NEGATIVE;

		if (v1->x < v2->x)
		{
			ld->bbox[BOXLEFT] = v1->x;
			ld->bbox[BOXRIGHT] = v2->x;
		}
		else
		{
			ld->bbox[BOXLEFT] = v2->x;
			ld->bbox[BOXRIGHT] = v1->x;
		}

		if (v1->y < v2->y)
		{
			ld->bbox[BOXBOTTOM] = v1->y;
			ld->bbox[BOXTOP] = v2->y;
		}
		else
		{
			ld->bbox[BOXBOTTOM] = v2->y;
			ld->bbox[BOXTOP] = v1->y;
		}

		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);

		{
			// cph 2006/09/30 - fix sidedef errors right away.
			// cph 2002/07/20 - these errors are fatal if not fixed, so apply them
			UINT8 j;

			for (j=0; j < 2; j++)
			{
				if (ld->sidenum[j] != 0xffff && ld->sidenum[j] >= (UINT16)numsides)
				{
					ld->sidenum[j] = 0xffff;
					CONS_Debug(DBG_SETUP, "P_LoadLineDefs: linedef %s has out-of-range sidedef number\n", sizeu1(numlines-i-1));
				}
			}
		}

		ld->frontsector = ld->backsector = NULL;
		ld->validcount = 0;
		ld->firsttag = ld->nexttag = -1;
		ld->callcount = 0;
		// killough 11/98: fix common wad errors (missing sidedefs):

		if (ld->sidenum[0] == 0xffff)
		{
			ld->sidenum[0] = 0;  // Substitute dummy sidedef for missing right side
			// cph - print a warning about the bug
			CONS_Debug(DBG_SETUP, "P_LoadLineDefs: linedef %s missing first sidedef\n", sizeu1(numlines-i-1));
		}

		if ((ld->sidenum[1] == 0xffff) && (ld->flags & ML_TWOSIDED))
		{
			ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side
			// cph - print a warning about the bug
			CONS_Debug(DBG_SETUP, "P_LoadLineDefs: linedef %s has two-sided flag set, but no second sidedef\n", sizeu1(numlines-i-1));
		}

		if (ld->sidenum[0] != 0xffff && ld->special)
			sides[ld->sidenum[0]].special = ld->special;
		if (ld->sidenum[1] != 0xffff && ld->special)
			sides[ld->sidenum[1]].special = ld->special;

#ifdef POLYOBJECTS
		ld->polyobj = NULL;
#endif
	}

	Z_Free(data);
}

static void P_LoadLineDefs2(void)
{
	size_t i = numlines;
	register line_t *ld = lines;
	for (;i--;ld++)
	{
		ld->frontsector = sides[ld->sidenum[0]].sector; //e6y: Can't be -1 here
		ld->backsector  = ld->sidenum[1] != 0xffff ? sides[ld->sidenum[1]].sector : 0;

		// Repeat count for midtexture
		if ((ld->flags & ML_EFFECT5) && (ld->sidenum[1] != 0xffff))
		{
			sides[ld->sidenum[0]].repeatcnt = (INT16)(((unsigned)sides[ld->sidenum[0]].textureoffset >> FRACBITS) >> 12);
			sides[ld->sidenum[0]].textureoffset = (((unsigned)sides[ld->sidenum[0]].textureoffset >> FRACBITS) & 2047) << FRACBITS;
			sides[ld->sidenum[1]].repeatcnt = (INT16)(((unsigned)sides[ld->sidenum[1]].textureoffset >> FRACBITS) >> 12);
			sides[ld->sidenum[1]].textureoffset = (((unsigned)sides[ld->sidenum[1]].textureoffset >> FRACBITS) & 2047) << FRACBITS;
		}

		// Compile linedef 'text' from both sidedefs 'text' for appropriate specials.
		switch(ld->special)
		{
		case 443: // Calls a named Lua function
			if (sides[ld->sidenum[0]].text)
			{
				size_t len = strlen(sides[ld->sidenum[0]].text)+1;
				if (ld->sidenum[1] != 0xffff && sides[ld->sidenum[1]].text)
					len += strlen(sides[ld->sidenum[1]].text);
				ld->text = Z_Malloc(len, PU_LEVEL, NULL);
				M_Memcpy(ld->text, sides[ld->sidenum[0]].text, strlen(sides[ld->sidenum[0]].text)+1);
				if (ld->sidenum[1] != 0xffff && sides[ld->sidenum[1]].text)
					M_Memcpy(ld->text+strlen(ld->text)+1, sides[ld->sidenum[1]].text, strlen(sides[ld->sidenum[1]].text)+1);
			}
			break;
		}
	}

	// Optimize sidedefs
	if (M_CheckParm("-compress"))
	{
		side_t *newsides;
		size_t numnewsides = 0;
		size_t z;

		for (i = 0; i < numsides; i++)
		{
			size_t j, k;
			if (sides[i].sector == NULL)
				continue;

			for (k = numlines, ld = lines; k--; ld++)
			{
				if (ld->sidenum[0] == i)
					ld->sidenum[0] = (UINT16)numnewsides;

				if (ld->sidenum[1] == i)
					ld->sidenum[1] = (UINT16)numnewsides;
			}

			for (j = i+1; j < numsides; j++)
			{
				if (sides[j].sector == NULL)
					continue;

				if (!memcmp(&sides[i], &sides[j], sizeof(side_t)))
				{
					// Find the linedefs that belong to this one
					for (k = numlines, ld = lines; k--; ld++)
					{
						if (ld->sidenum[0] == j)
							ld->sidenum[0] = (UINT16)numnewsides;

						if (ld->sidenum[1] == j)
							ld->sidenum[1] = (UINT16)numnewsides;
					}
					sides[j].sector = NULL; // Flag for deletion
				}
			}
			numnewsides++;
		}

		// We're loading crap into this block anyhow, so no point in zeroing it out.
		newsides = Z_Malloc(numnewsides * sizeof(*newsides), PU_LEVEL, NULL);

		// Copy the sides to their new block of memory.
		for (i = 0, z = 0; i < numsides; i++)
		{
			if (sides[i].sector != NULL)
				M_Memcpy(&newsides[z++], &sides[i], sizeof(side_t));
		}

		CONS_Debug(DBG_SETUP, "Old sides is %s, new sides is %s\n", sizeu1(numsides), sizeu1(numnewsides));

		Z_Free(sides);
		sides = newsides;
		numsides = numnewsides;
	}
}

//
// P_LoadSideDefs
//
static inline void P_LoadSideDefs(lumpnum_t lumpnum)
{
	numsides = W_LumpLength(lumpnum) / sizeof (mapsidedef_t);
	if (numsides <= 0)
		I_Error("Level has no sidedefs");
	sides = Z_Calloc(numsides * sizeof (*sides), PU_LEVEL, NULL);
}

// Delay loading texture names until after loaded linedefs.

static void P_LoadSideDefs2(lumpnum_t lumpnum)
{
	UINT8 *data = W_CacheLumpNum(lumpnum, PU_STATIC);
	UINT16 i;
	INT32 num;

	for (i = 0; i < numsides; i++)
	{
		register mapsidedef_t *msd = (mapsidedef_t *)data + i;
		register side_t *sd = sides + i;
		register sector_t *sec;

		sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
		sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;

		{ /* cph 2006/09/30 - catch out-of-range sector numbers; use sector 0 instead */
			UINT16 sector_num = SHORT(msd->sector);

			if (sector_num >= numsectors)
			{
				CONS_Debug(DBG_SETUP, "P_LoadSideDefs2: sidedef %u has out-of-range sector num %u\n", i, sector_num);
				sector_num = 0;
			}
			sd->sector = sec = &sectors[sector_num];
		}

		// refined to allow colormaps to work as wall textures if invalid as colormaps
		// but valid as textures.

		sd->sector = sec = &sectors[SHORT(msd->sector)];

		// Colormaps!
		switch (sd->special)
		{
			case 63: // variable colormap via 242 linedef
			case 606: //SoM: 4/4/2000: Just colormap transfer
				// SoM: R_CreateColormap will only create a colormap in software mode...
				// Perhaps we should just call it instead of doing the calculations here.
				if (rendermode == render_soft || rendermode == render_none)
				{
					if (msd->toptexture[0] == '#' || msd->bottomtexture[0] == '#')
					{
						sec->midmap = R_CreateColormap(msd->toptexture, msd->midtexture,
							msd->bottomtexture);
						sd->toptexture = sd->bottomtexture = 0;
					}
					else
					{
						if ((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
							sd->toptexture = 0;
						else
							sd->toptexture = num;
						if ((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
							sd->midtexture = 0;
						else
							sd->midtexture = num;
						if ((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
							sd->bottomtexture = 0;
						else
							sd->bottomtexture = num;
					}
					break;
				}
#ifdef HWRENDER
				else
				{
					// for now, full support of toptexture only
					if ((msd->toptexture[0] == '#' && msd->toptexture[1] && msd->toptexture[2] && msd->toptexture[3] && msd->toptexture[4] && msd->toptexture[5] && msd->toptexture[6])
						|| (msd->bottomtexture[0] == '#' && msd->bottomtexture[1] && msd->bottomtexture[2] && msd->bottomtexture[3] && msd->bottomtexture[4] && msd->bottomtexture[5] && msd->bottomtexture[6]))
					{
						char *col;

						sec->midmap = R_CreateColormap(msd->toptexture, msd->midtexture,
							msd->bottomtexture);
						sd->toptexture = sd->bottomtexture = 0;
#define HEX2INT(x) (x >= '0' && x <= '9' ? x - '0' : x >= 'a' && x <= 'f' ? x - 'a' + 10 : x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0)
#define ALPHA2INT(x) (x >= 'a' && x <= 'z' ? x - 'a' : x >= 'A' && x <= 'Z' ? x - 'A' : x >= '0' && x <= '9' ? 25 : 0)
						sec->extra_colormap = &extra_colormaps[sec->midmap];

						if (msd->toptexture[0] == '#' && msd->toptexture[1] && msd->toptexture[2] && msd->toptexture[3] && msd->toptexture[4] && msd->toptexture[5] && msd->toptexture[6])
						{
							col = msd->toptexture;

							sec->extra_colormap->rgba =
								(HEX2INT(col[1]) << 4) + (HEX2INT(col[2]) << 0) +
								(HEX2INT(col[3]) << 12) + (HEX2INT(col[4]) << 8) +
								(HEX2INT(col[5]) << 20) + (HEX2INT(col[6]) << 16);

							// alpha
							if (msd->toptexture[7])
								sec->extra_colormap->rgba += (ALPHA2INT(col[7]) << 24);
							else
								sec->extra_colormap->rgba += (25 << 24);
						}
						else
							sec->extra_colormap->rgba = 0;

						if (msd->bottomtexture[0] == '#' && msd->bottomtexture[1] && msd->bottomtexture[2] && msd->bottomtexture[3] && msd->bottomtexture[4] && msd->bottomtexture[5] && msd->bottomtexture[6])
						{
							col = msd->bottomtexture;

							sec->extra_colormap->fadergba =
								(HEX2INT(col[1]) << 4) + (HEX2INT(col[2]) << 0) +
								(HEX2INT(col[3]) << 12) + (HEX2INT(col[4]) << 8) +
								(HEX2INT(col[5]) << 20) + (HEX2INT(col[6]) << 16);

							// alpha
							if (msd->bottomtexture[7])
								sec->extra_colormap->fadergba += (ALPHA2INT(col[7]) << 24);
							else
								sec->extra_colormap->fadergba += (25 << 24);
						}
						else
							sec->extra_colormap->fadergba = 0x19000000; // default alpha, (25 << 24)
#undef ALPHA2INT
#undef HEX2INT
					}
					else
					{
						if ((num = R_CheckTextureNumForName(msd->toptexture)) == -1)
							sd->toptexture = 0;
						else
							sd->toptexture = num;

						if ((num = R_CheckTextureNumForName(msd->midtexture)) == -1)
							sd->midtexture = 0;
						else
							sd->midtexture = num;

						if ((num = R_CheckTextureNumForName(msd->bottomtexture)) == -1)
							sd->bottomtexture = 0;
						else
							sd->bottomtexture = num;
					}
					break;
				}
#endif

			case 413: // Change music
			{
				char process[8+1];

				sd->toptexture = sd->midtexture = sd->bottomtexture = 0;
				if (msd->bottomtexture[0] != '-' || msd->bottomtexture[1] != '\0')
				{
					M_Memcpy(process,msd->bottomtexture,8);
					process[8] = '\0';
					sd->bottomtexture = get_number(process)-1;
				}
				M_Memcpy(process,msd->toptexture,8);
				process[8] = '\0';
				sd->text = Z_Malloc(7, PU_LEVEL, NULL);

				// If they type in O_ or D_ and their music name, just shrug,
				// then copy the rest instead.
				if ((process[0] == 'O' || process[0] == 'D') && process[7])
					M_Memcpy(sd->text, process+2, 6);
				else // Assume it's a proper music name.
					M_Memcpy(sd->text, process, 6);
				sd->text[6] = 0;
				break;
			}
			case 414: // Play SFX
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

			case 425: // Calls P_SetMobjState on calling mobj
			case 434: // Custom Power
			case 442: // Calls P_SetMobjState on mobjs of a given type in the tagged sectors
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

			case 443: // Calls a named Lua function
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

			default: // normal cases
				if (msd->toptexture[0] == '#')
				{
					char *col = msd->toptexture;
					sd->toptexture = sd->bottomtexture =
						((col[1]-'0')*100 + (col[2]-'0')*10 + col[3]-'0') + 1;
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

	Z_Free(data);
	R_ClearTextureNumCache(true);
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

#ifdef POLYOBJECTS
		// haleyjd 2/22/06: setup polyobject blockmap
		count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
		polyblocklinks = Z_Calloc(count, PU_LEVEL, NULL);
#endif
	}
}

//
// P_LoadBlockMap
//
// Levels might not have a blockmap, so if one does not exist
// this should return false.
static boolean P_LoadBlockMap(lumpnum_t lumpnum)
{
#if 0
	(void)lumpnum;
	return false;
#else
	size_t count;
	const char *lumpname = W_CheckNameForNum(lumpnum);

	// Check if the lump exists, and if it's named "BLOCKMAP"
	if (!lumpname || memcmp(lumpname, "BLOCKMAP", 8) != 0)
	{
		return false;
	}

	count = W_LumpLength(lumpnum);

	if (!count || count >= 0x20000)
		return false;

	{
		size_t i;
		INT16 *wadblockmaplump = malloc(count); //INT16 *wadblockmaplump = W_CacheLumpNum (lump, PU_LEVEL);

		if (wadblockmaplump) W_ReadLump(lumpnum, wadblockmaplump);
		else return false;
		count /= 2;
		blockmaplump = Z_Calloc(sizeof (*blockmaplump) * count, PU_LEVEL, 0);

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

		free(wadblockmaplump);

		bmaporgx = blockmaplump[0]<<FRACBITS;
		bmaporgy = blockmaplump[1]<<FRACBITS;
		bmapwidth = blockmaplump[2];
		bmapheight = blockmaplump[3];
	}

	// clear out mobj chains
	count = sizeof (*blocklinks)* bmapwidth*bmapheight;
	blocklinks = Z_Calloc(count, PU_LEVEL, NULL);
	blockmap = blockmaplump+4;

#ifdef POLYOBJECTS
	// haleyjd 2/22/06: setup polyobject blockmap
	count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
	polyblocklinks = Z_Calloc(count, PU_LEVEL, NULL);
#endif
	return true;
/* Original
		blockmaplump = W_CacheLumpNum(lump, PU_LEVEL);
		blockmap = blockmaplump+4;
		count = W_LumpLength (lump)/2;

		for (i = 0; i < count; i++)
			blockmaplump[i] = SHORT(blockmaplump[i]);

		bmaporgx = blockmaplump[0]<<FRACBITS;
		bmaporgy = blockmaplump[1]<<FRACBITS;
		bmapwidth = blockmaplump[2];
		bmapheight = blockmaplump[3];
	}

	// clear out mobj chains
	count = sizeof (*blocklinks)*bmapwidth*bmapheight;
	blocklinks = Z_Calloc(count, PU_LEVEL, NULL);
	return true;
	*/
#endif
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
static void P_GroupLines(void)
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
			CorruptMapError(va("P_GroupLines: ss->firstline invalid "
				"(subsector %s, firstline refers to %d of %s)", sizeu1(i), ss->firstline,
				sizeu2(numsegs)));
		seg = &segs[ss->firstline];
		sidei = (size_t)(seg->sidedef - sides);
		if (!seg->sidedef)
			CorruptMapError(va("P_GroupLines: seg->sidedef is NULL "
				"(subsector %s, firstline is %d)", sizeu1(i), ss->firstline));
		if (seg->sidedef - sides < 0 || seg->sidedef - sides > (UINT16)numsides)
			CorruptMapError(va("P_GroupLines: seg->sidedef refers to sidedef %s of %s "
				"(subsector %s, firstline is %d)", sizeu1(sidei), sizeu2(numsides),
				sizeu3(i), ss->firstline));
		if (!seg->sidedef->sector)
			CorruptMapError(va("P_GroupLines: seg->sidedef->sector is NULL "
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
			CONS_Debug(DBG_SETUP, "P_GroupLines: sector %s has no lines\n", sizeu1(i));
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

//
// P_LoadReject
//
// Detect if the REJECT lump is valid,
// if not, rejectmatrix will be NULL
static void P_LoadReject(lumpnum_t lumpnum)
{
	size_t count;
	const char *lumpname = W_CheckNameForNum(lumpnum);

	// Check if the lump exists, and if it's named "REJECT"
	if (!lumpname || memcmp(lumpname, "REJECT\0\0", 8) != 0)
	{
		rejectmatrix = NULL;
		CONS_Debug(DBG_SETUP, "P_LoadReject: No valid REJECT lump found\n");
		return;
	}

	count = W_LumpLength(lumpnum);

	if (!count) // zero length, someone probably used ZDBSP
	{
		rejectmatrix = NULL;
		CONS_Debug(DBG_SETUP, "P_LoadReject: REJECT lump has size 0, will not be loaded\n");
	}
	else
		rejectmatrix = W_CacheLumpNum(lumpnum, PU_LEVEL);
}

#if 0
static char *levellumps[] =
{
	"label",        // ML_LABEL,    A separator, name, MAPxx
	"THINGS",       // ML_THINGS,   Enemies, items..
	"LINEDEFS",     // ML_LINEDEFS, Linedefs, from editing
	"SIDEDEFS",     // ML_SIDEDEFS, Sidedefs, from editing
	"VERTEXES",     // ML_VERTEXES, Vertices, edited and BSP splits generated
	"SEGS",         // ML_SEGS,     Linesegs, from linedefs split by BSP
	"SSECTORS",     // ML_SSECTORS, Subsectors, list of linesegs
	"NODES",        // ML_NODES,    BSP nodes
	"SECTORS",      // ML_SECTORS,  Sectors, from editing
	"REJECT",       // ML_REJECT,   LUT, sector-sector visibility
};

/** Checks a lump and returns whether it is a valid start-of-level marker.
  *
  * \param lumpnum Lump number to check.
  * \return True if the lump is a valid level marker, false if not.
  */
static inline boolean P_CheckLevel(lumpnum_t lumpnum)
{
	UINT16 file, lump;
	size_t i;

	for (i = ML_THINGS; i <= ML_REJECT; i++)
	{
		file = WADFILENUM(lumpnum);
		lump = LUMPNUM(lumpnum+1);
		if (file > numwadfiles || lump < LUMPNUM(lumpnum) || lump > wadfiles[file]->numlumps ||
			memcmp(wadfiles[file]->lumpinfo[lump].name, levellumps[i], 8) != 0)
		return false;
	}
	return true; // all right
}
#endif

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
static void P_LevelInitStuff(void)
{
	INT32 i;

	leveltime = 0;

	localaiming = 0;
	localaiming2 = 0;

	// special stage tokens, emeralds, and ring total
	tokenbits = 0;
	runemeraldmanager = false;
	nummaprings = 0;

	// emerald hunt
	hunt1 = hunt2 = hunt3 = NULL;

	// map time limit
	if (mapheaderinfo[gamemap-1]->countdown)
		countdowntimer = mapheaderinfo[gamemap-1]->countdown * TICRATE;
	else
		countdowntimer = 0;
	countdowntimeup = false;

	// clear ctf pointers
	redflag = blueflag = NULL;
	rflagpoint = bflagpoint = NULL;

	// circuit, race and competition stuff
	circuitmap = false;
	numstarposts = 0;
	totalrings = timeinmap = 0;

	// special stage
	stagefailed = false;
	// Reset temporary record data
	memset(&ntemprecords, 0, sizeof(nightsdata_t));

	// earthquake camera
	memset(&quake,0,sizeof(struct quake));

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if ((netgame || multiplayer) && (gametype == GT_COMPETITION || players[i].lives <= 0))
		{
			// In Co-Op, replenish a user's lives if they are depleted.
			players[i].lives = cv_startinglives.value;
		}

		players[i].realtime = countdown = countdown2 = 0;

		players[i].gotcontinue = false;

		players[i].xtralife = players[i].deadtimer = players[i].numboxes = players[i].totalring = players[i].laps = 0;
		players[i].health = 1;
		players[i].aiming = 0;
		players[i].pflags &= ~PF_TIMEOVER;

		players[i].losstime = 0;
		players[i].timeshit = 0;

		players[i].marescore = players[i].lastmarescore = players[i].maxlink = 0;
		players[i].startedtime = players[i].finishedtime = players[i].finishedrings = 0;
		players[i].lastmare = players[i].marebegunat = 0;

		// Don't show anything
		players[i].textvar = players[i].texttimer = 0;

		players[i].linkcount = players[i].linktimer = 0;
		players[i].flyangle = players[i].anotherflyangle = 0;
		players[i].nightstime = players[i].mare = 0;
		P_SetTarget(&players[i].capsule, NULL);
		players[i].drillmeter = 40*20;

		players[i].exiting = 0;
		P_ResetPlayer(&players[i]);

		players[i].mo = NULL;

		// we must unset axis details too
		players[i].axis1 = players[i].axis2 = NULL;

		// and this stupid flag as a result
		players[i].pflags &= ~PF_TRANSFERTOCLOSEST;
	}
}

//
// P_LoadThingsOnly
//
// "Reloads" a level, but only reloads all of the mobjs.
//
void P_LoadThingsOnly(void)
{
	// Search through all the thinkers.
	mobj_t *mo;
	thinker_t *think;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;

		if (mo)
			P_RemoveMobj(mo);
	}

	P_LevelInitStuff();

	P_PrepareThings(lastloadedmaplumpnum + ML_THINGS);
	P_LoadThings();

	P_SpawnSecretItems(true);
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

static void P_MakeMapMD5(lumpnum_t maplumpnum, void *dest)
{
	unsigned char linemd5[16];
	unsigned char sectormd5[16];
	unsigned char thingmd5[16];
	unsigned char sidedefmd5[16];
	unsigned char resmd5[16];
	UINT8 i;

	// Create a hash for the current map
	// get the actual lumps!
	UINT8 *datalines   = W_CacheLumpNum(maplumpnum + ML_LINEDEFS, PU_CACHE);
	UINT8 *datasectors = W_CacheLumpNum(maplumpnum + ML_SECTORS, PU_CACHE);
	UINT8 *datathings  = W_CacheLumpNum(maplumpnum + ML_THINGS, PU_CACHE);
	UINT8 *datasides   = W_CacheLumpNum(maplumpnum + ML_SIDEDEFS, PU_CACHE);

	P_MakeBufferMD5((char*)datalines,   W_LumpLength(maplumpnum + ML_LINEDEFS), linemd5);
	P_MakeBufferMD5((char*)datasectors, W_LumpLength(maplumpnum + ML_SECTORS),  sectormd5);
	P_MakeBufferMD5((char*)datathings,  W_LumpLength(maplumpnum + ML_THINGS),   thingmd5);
	P_MakeBufferMD5((char*)datasides,   W_LumpLength(maplumpnum + ML_SIDEDEFS), sidedefmd5);

	Z_Free(datalines);
	Z_Free(datasectors);
	Z_Free(datathings);
	Z_Free(datasides);

	for (i = 0; i < 16; i++)
		resmd5[i] = (linemd5[i] + sectormd5[i] + thingmd5[i] + sidedefmd5[i]) & 0xFF;

	M_Memcpy(dest, &resmd5, 16);
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

	if (!gpath)
		return;

	sprintf(gpath,"%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s", srb2home, timeattackfolder, G_BuildMapName(gamemap));

	// Best Score ghost
	if (cv_ghost_bestscore.value && FIL_FileExists(va("%s-score-best.lmp", gpath)))
			G_AddGhost(va("%s-score-best.lmp", gpath));

	// Best Time ghost
	if (cv_ghost_besttime.value && FIL_FileExists(va("%s-time-best.lmp", gpath)))
			G_AddGhost(va("%s-time-best.lmp", gpath));

	// Last ghost
	if (cv_ghost_last.value && FIL_FileExists(va("%s-last.lmp", gpath)))
		G_AddGhost(va("%s-last.lmp", gpath));

	// Guest ghost
	if (cv_ghost_guest.value && FIL_FileExists(va("%s-guest.lmp", gpath)))
		G_AddGhost(va("%s-guest.lmp", gpath));

	free(gpath);
}

/** Loads a level from a lump or external wad.
  *
  * \param skipprecip If true, don't spawn precipitation.
  * \todo Clean up, refactor, split up; get rid of the bloat.
  */
boolean P_SetupLevel(boolean skipprecip)
{
	// use gamemap to get map number.
	// 99% of the things already did, so.
	// Map header should always be in place at this point
	INT32 i, loadprecip = 1, ranspecialwipe = 0;
	INT32 loademblems = 1;
	INT32 fromnetsave = 0;
	boolean loadedbm = false;
	sector_t *ss;
	boolean chase;

	levelloading = true;

	// This is needed. Don't touch.
	maptol = mapheaderinfo[gamemap-1]->typeoflevel;

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

	P_LevelInitStuff();

	postimgtype = postimgtype2 = postimg_none;

	if (mapheaderinfo[gamemap-1]->forcecharacter[0] != '\0'
	&& atoi(mapheaderinfo[gamemap-1]->forcecharacter) != 255)
		P_ForceCharacter(mapheaderinfo[gamemap-1]->forcecharacter);

	// chasecam on in chaos, race, coop
	// chasecam off in match, tag, capture the flag
	chase = (gametype == GT_RACE || gametype == GT_COMPETITION || gametype == GT_COOP)
		|| (maptol & TOL_2D);

	if (!dedicated)
	{
		if (!cv_cam_speed.changed)
			CV_Set(&cv_cam_speed, cv_cam_speed.defaultvalue);

		if (!cv_chasecam.changed)
			CV_SetValue(&cv_chasecam, chase);

		// same for second player
		if (!cv_chasecam2.changed)
			CV_SetValue(&cv_chasecam2, chase);
	}

	// Initial height of PointOfView
	// will be set by player think.
	players[consoleplayer].viewz = 1;

	// Special stage fade to white
	// This is handled BEFORE sounds are stopped.
	if (rendermode != render_none && G_IsSpecialStage(gamemap))
	{
		tic_t starttime = I_GetTime();
		tic_t endtime = starttime + (3*TICRATE)/2;
		tic_t nowtime;

		S_StartSound(NULL, sfx_s3kaf);

		F_WipeStartScreen();
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0);

		F_WipeEndScreen();
		F_RunWipe(wipedefs[wipe_speclevel_towhite], false);

		nowtime = lastwipetic;
		// Hold on white for extra effect.
		while (nowtime < endtime)
		{
			// wait loop
			while (!((nowtime = I_GetTime()) - lastwipetic))
				I_Sleep();
			lastwipetic = nowtime;
			if (moviemode) // make sure we save frames for the white hold too
				M_SaveFrame();
		}

		ranspecialwipe = 1;
	}

	// Make sure all sounds are stopped before Z_FreeTags.
	S_StopSounds();
	S_ClearSfx();

	// As oddly named as this is, this handles music only.
	// We should be fine starting it here.
	S_Start();

	// Let's fade to black here
	// But only if we didn't do the special stage wipe
	if (rendermode != render_none && !ranspecialwipe)
	{
		F_WipeStartScreen();
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

		F_WipeEndScreen();
		F_RunWipe(wipedefs[wipe_level_toblack], false);
	}

	// Print "SPEEDING OFF TO [ZONE] [ACT 1]..."
	if (rendermode != render_none)
	{
		// Don't include these in the fade!
		char tx[64];
		V_DrawSmallString(1, 191, V_ALLOWLOWERCASE, M_GetText("Speeding off to..."));
		snprintf(tx, 63, "%s%s%s",
			mapheaderinfo[gamemap-1]->lvlttl,
			(mapheaderinfo[gamemap-1]->levelflags & LF_NOZONE) ? "" : " ZONE",
			(mapheaderinfo[gamemap-1]->actnum > 0) ? va(", Act %d",mapheaderinfo[gamemap-1]->actnum) : "");
		V_DrawSmallString(1, 195, V_ALLOWLOWERCASE, tx);
		I_UpdateNoVsync();
	}

#ifdef HAVE_BLUA
	LUA_InvalidateLevel();
#endif

	for (ss = sectors; sectors+numsectors != ss; ss++)
	{
		Z_Free(ss->attached);
		Z_Free(ss->attachedsolid);
	}

	// Clear pointers that would be left dangling by the purge
	R_FlushTranslationColormapCache();

	Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

#if defined (WALLSPLATS) || defined (FLOORSPLATS)
	// clear the splats from previous level
	R_ClearLevelSplats();
#endif

	P_InitThinkers();
	P_InitCachedActions();

	/// \note for not spawning precipitation, etc. when loading netgame snapshots
	if (skipprecip)
	{
		fromnetsave = 1;
		loadprecip = 0;
		loademblems = 0;
	}

	// internal game map
	lastloadedmaplumpnum = W_GetNumForName(maplumpname = G_BuildMapName(gamemap));

	R_ReInitColormaps(mapheaderinfo[gamemap-1]->palette);
	CON_SetupBackColormap();

	// SRB2 determines the sky texture to be used depending on the map header.
	P_SetupLevelSky(mapheaderinfo[gamemap-1]->skynum, true);

	P_MakeMapMD5(lastloadedmaplumpnum, &mapmd5);

	// note: most of this ordering is important
	loadedbm = P_LoadBlockMap(lastloadedmaplumpnum + ML_BLOCKMAP);

	P_LoadVertexes(lastloadedmaplumpnum + ML_VERTEXES);
	P_LoadSectors(lastloadedmaplumpnum + ML_SECTORS);

	P_LoadSideDefs(lastloadedmaplumpnum + ML_SIDEDEFS);

	P_LoadLineDefs(lastloadedmaplumpnum + ML_LINEDEFS);
	if (!loadedbm)
		P_CreateBlockMap(); // Graue 02-29-2004
	P_LoadSideDefs2(lastloadedmaplumpnum + ML_SIDEDEFS);

	R_MakeColormaps();
	P_LoadLineDefs2();
	P_LoadSubsectors(lastloadedmaplumpnum + ML_SSECTORS);
	P_LoadNodes(lastloadedmaplumpnum + ML_NODES);
	P_LoadSegs(lastloadedmaplumpnum + ML_SEGS);
	P_LoadReject(lastloadedmaplumpnum + ML_REJECT);
	P_GroupLines();

	numdmstarts = numredctfstarts = numbluectfstarts = 0;

	// reset the player starts
	for (i = 0; i < MAXPLAYERS; i++)
		playerstarts[i] = NULL;

	for (i = 0; i < 2; i++)
		skyboxmo[i] = NULL;

	P_MapStart();

	P_PrepareThings(lastloadedmaplumpnum + ML_THINGS);

#ifdef ESLOPE
	P_ResetDynamicSlopes();
#endif

	P_LoadThings();

	P_SpawnSecretItems(loademblems);

	for (numcoopstarts = 0; numcoopstarts < MAXPLAYERS; numcoopstarts++)
		if (!playerstarts[numcoopstarts])
			break;

	// set up world state
	P_SpawnSpecials(fromnetsave);

	if (loadprecip) //  ugly hack for P_NetUnArchiveMisc (and P_LoadNetGame)
		P_SpawnPrecipitation();

	globalweather = mapheaderinfo[gamemap-1]->weather;

#ifdef HWRENDER // not win32 only 19990829 by Kin
	if (rendermode != render_soft && rendermode != render_none)
	{
#ifdef ALAM_LIGHTING
		// BP: reset light between levels (we draw preview frame lights on current frame)
		HWR_ResetLights();
#endif
		// Correct missing sidedefs & deep water trick
		HWR_CorrectSWTricks();
		HWR_CreatePlanePolygons((INT32)numnodes - 1);
	}
#endif

	// oh god I hope this helps
	// (addendum: apparently it does!
	//  none of this needs to be done because it's not the beginning of the map when
	//  a netgame save is being loaded, and could actively be harmful by messing with
	//  the client's view of the data.)
	if (fromnetsave)
		goto netgameskip;
	// ==========

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
		{
			players[i].pflags &= ~PF_NIGHTSMODE;

			// Start players with pity shields if possible
			players[i].pity = -1;

			if (!G_PlatformGametype())
			{
				players[i].mo = NULL;
				G_DoReborn(i);
			}
			else // gametype is GT_COOP or GT_RACE
			{
				players[i].mo = NULL;

				if (players[i].starposttime)
				{
					G_SpawnPlayer(i, true);
					P_ClearStarPost(players[i].starpostnum);
				}
				else
					G_SpawnPlayer(i, false);
			}
		}

	if (modeattacking == ATTACKING_RECORD && !demoplayback)
		P_LoadRecordGhosts();
	else if (modeattacking == ATTACKING_NIGHTS && !demoplayback)
		P_LoadNightsGhosts();

	if (G_TagGametype())
	{
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
			if (playeringame[i] && !players[i].spectator)
			{
				playersactive[realnumplayers] = i; //stores the player's node in the array.
				realnumplayers++;
			}
		}

		if (realnumplayers) //this should also fix the dedicated crash bug. You only pick a player if one exists to be picked.
		{
			i = P_RandomKey(realnumplayers);
			players[playersactive[i]].pflags |= PF_TAGIT; //choose our initial tagger before map starts.

			// Taken and modified from G_DoReborn()
			// Remove the player so he can respawn elsewhere.
			// first dissasociate the corpse
			if (players[playersactive[i]].mo)
				P_RemoveMobj(players[playersactive[i]].mo);

			G_SpawnPlayer(playersactive[i], false); //respawn the lucky player in his dedicated spawn location.
		}
		else
			CONS_Printf(M_GetText("No player currently available to become IT. Awaiting available players.\n"));

	}
	else if (gametype == GT_RACE && server && cv_usemapnumlaps.value)
		CV_StealthSetValue(&cv_numlaps, mapheaderinfo[gamemap - 1]->numlaps);

	// ===========
	// landing point for netgames.
	netgameskip:

	if (!dedicated)
	{
		if (players[displayplayer].mo && (server || addedtogame))
		{
			camera.x = players[displayplayer].mo->x;
			camera.y = players[displayplayer].mo->y;
			camera.z = players[displayplayer].mo->z;
			camera.angle = players[displayplayer].mo->angle;
		}
		else
		{
			mapthing_t *thing;

			switch (gametype)
			{
			case GT_MATCH:
			case GT_TAG:
				thing = deathmatchstarts[0];
				break;

			default:
				thing = playerstarts[0];
				break;
			}

			if (thing)
			{
				camera.x = thing->x;
				camera.y = thing->y;
				camera.z = thing->z;
				camera.angle = FixedAngle((fixed_t)thing->angle << FRACBITS);
			}
		}

		if (!cv_cam_height.changed)
			CV_Set(&cv_cam_height, cv_cam_height.defaultvalue);

		if (!cv_cam_dist.changed)
			CV_Set(&cv_cam_dist, cv_cam_dist.defaultvalue);

		if (!cv_cam_rotate.changed)
			CV_Set(&cv_cam_rotate, cv_cam_rotate.defaultvalue);

		if (!cv_cam2_height.changed)
			CV_Set(&cv_cam2_height, cv_cam2_height.defaultvalue);

		if (!cv_cam2_dist.changed)
			CV_Set(&cv_cam2_dist, cv_cam2_dist.defaultvalue);

		if (!cv_cam2_rotate.changed)
			CV_Set(&cv_cam2_rotate, cv_cam2_rotate.defaultvalue);

		if (!cv_analog.changed)
			CV_SetValue(&cv_analog, 0);
		if (!cv_analog2.changed)
			CV_SetValue(&cv_analog2, 0);

#ifdef HWRENDER
		if (rendermode != render_soft && rendermode != render_none)
			CV_Set(&cv_grfov, cv_grfov.defaultvalue);
#endif

		displayplayer = consoleplayer; // Start with your OWN view, please!
	}

	if (cv_useranalog.value)
		CV_SetValue(&cv_analog, true);

	if (splitscreen && cv_useranalog2.value)
		CV_SetValue(&cv_analog2, true);
	else if (botingame)
		CV_SetValue(&cv_analog2, true);

	if (twodlevel)
	{
		CV_SetValue(&cv_analog2, false);
		CV_SetValue(&cv_analog, false);
	}

	// clear special respawning que
	iquehead = iquetail = 0;

	// Fab : 19-07-98 : start cd music for this level (note: can be remapped)
	I_PlayCD((UINT8)(gamemap), false);

	// preload graphics
#ifdef HWRENDER // not win32 only 19990829 by Kin
	if (rendermode != render_soft && rendermode != render_none)
	{
		HWR_PrepLevelCache(numtextures);
	}
#endif

	P_MapEnd();

	// Remove the loading shit from the screen
	if (rendermode != render_none)
		V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, (ranspecialwipe) ? 0 : 31);

	if (precache || dedicated)
		R_PrecacheLevel();

	nextmapoverride = 0;
	skipstats = false;

	if (!(netgame || multiplayer) && (!modifiedgame || savemoddata))
		mapvisited[gamemap-1] |= MV_VISITED;

	levelloading = false;

	P_RunCachedActions();

	if (!(netgame || multiplayer || demoplayback || demorecording || metalrecording || modeattacking || players[consoleplayer].lives <= 0)
		&& (!modifiedgame || savemoddata) && cursaveslot >= 0 && !ultimatemode
		&& !(mapheaderinfo[gamemap-1]->menuflags & LF2_HIDEINMENU)
		&& (!G_IsSpecialStage(gamemap)) && gamemap != lastmapsaved && (mapheaderinfo[gamemap-1]->actnum < 2 || gamecomplete))
		G_SaveGame((UINT32)cursaveslot);

	if (savedata.lives > 0)
	{
		players[consoleplayer].continues = savedata.continues;
		players[consoleplayer].lives = savedata.lives;
		players[consoleplayer].score = savedata.score;
		botskin = savedata.botskin;
		botcolor = savedata.botcolor;
		botingame = (savedata.botskin != 0);
		emeralds = savedata.emeralds;
		savedata.lives = 0;
	}

	skyVisible = skyVisible1 = skyVisible2 = true; // assume the skybox is visible on level load.
	if (loadprecip) // uglier hack
	{ // to make a newly loaded level start on the second frame.
		INT32 buf = gametic % BACKUPTICS;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
				G_CopyTiccmd(&players[i].cmd, &netcmds[buf][i], 1);
		}
		P_PreTicker(2);
#ifdef HAVE_BLUA
		LUAh_MapLoad();
#endif
	}

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
		return P_AddWadFile(socfilename, NULL);

	lump = W_CheckNumForName(socfilename);
	if (lump == LUMPERROR)
		return false;

	CONS_Printf(M_GetText("Loading SOC lump: %s\n"), socfilename);
	DEH_LoadDehackedLump(lump);

	return true;
}

//
// Add a wadfile to the active wad files,
// replace sounds, musics, patches, textures, sprites and maps
//
boolean P_AddWadFile(const char *wadfilename, char **firstmapname)
{
	size_t i, j, sreplaces = 0, mreplaces = 0, digmreplaces = 0;
	UINT16 numlumps, wadnum;
	INT16 firstmapreplaced = 0, num;
	char *name;
	lumpinfo_t *lumpinfo;
	boolean texturechange = false;
	boolean replacedcurrentmap = false;

	if ((numlumps = W_LoadWadFile(wadfilename)) == INT16_MAX)
	{
		CONS_Printf(M_GetText("Errors occurred while loading %s; not added.\n"), wadfilename);
		return false;
	}
	else wadnum = (UINT16)(numwadfiles-1);

	//
	// search for sound replacements
	//
	lumpinfo = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		if (name[0] == 'D')
		{
			if (name[1] == 'S') for (j = 1; j < NUMSFX; j++)
			{
				if (S_sfx[j].name && !strnicmp(S_sfx[j].name, name + 2, 6))
				{
					// the sound will be reloaded when needed,
					// since sfx->data will be NULL
					CONS_Debug(DBG_SETUP, "Sound %.8s replaced\n", name);

					I_FreeSfx(&S_sfx[j]);

					sreplaces++;
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
#if 0
		//
		// search for texturechange replacements
		//
		else if (!memcmp(name, "TEXTURE1", 8) || !memcmp(name, "TEXTURE2", 8)
			|| !memcmp(name, "PNAMES", 6))
#endif
			texturechange = true;
	}
	if (!devparm && sreplaces)
		CONS_Printf(M_GetText("%s sounds replaced\n"), sizeu1(sreplaces));
	if (!devparm && mreplaces)
		CONS_Printf(M_GetText("%s midi musics replaced\n"), sizeu1(mreplaces));
	if (!devparm && digmreplaces)
		CONS_Printf(M_GetText("%s digital musics replaced\n"), sizeu1(digmreplaces));

	//
	// search for sprite replacements
	//
	R_AddSpriteDefs(wadnum);

	// Reload it all anyway, just in case they
	// added some textures but didn't insert a
	// TEXTURE1/PNAMES/etc. list.
	if (texturechange) // initialized in the sound check
		R_LoadTextures(); // numtexture changes
	else
		R_FlushTextureCache(); // just reload it from file

	// Reload ANIMATED / ANIMDEFS
	P_InitPicAnims();

	// Flush and reload HUD graphics
	ST_UnloadGraphics();
	HU_LoadGraphics();
	ST_LoadGraphics();
	ST_ReloadSkinFaceGraphics();

	//
	// look for skins
	//
	R_AddSkins(wadnum); // faB: wadfile index in wadfiles[]

	//
	// search for maps
	//
	lumpinfo = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < numlumps; i++, lumpinfo++)
	{
		name = lumpinfo->name;
		num = firstmapreplaced;

		if (name[0] == 'M' && name[1] == 'A' && name[2] == 'P') // Ignore the headers
		{
			if (name[5]!='\0')
				continue;
			num = (INT16)M_MapNumber(name[3], name[4]);

			//If you replaced the map you're on, end the level when done.
			if (num == gamemap)
				replacedcurrentmap = true;

			CONS_Printf("%s\n", name);
		}

		if (num && (num < firstmapreplaced || !firstmapreplaced))
		{
			firstmapreplaced = num;
			if (firstmapname)
				*firstmapname = name;
		}
	}
	if (!firstmapreplaced)
		CONS_Printf(M_GetText("No maps added\n"));

	// reload status bar (warning should have valid player!)
	if (gamestate == GS_LEVEL)
		ST_Start();

	// Prevent savefile cheating
	if (cursaveslot >= 0)
		cursaveslot = -1;

	if (replacedcurrentmap && gamestate == GS_LEVEL && (netgame || multiplayer))
	{
		CONS_Printf(M_GetText("Current map %d replaced by added file, ending the level to ensure consistency.\n"), gamemap);
		if (server)
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);
	}

	return true;
}

#ifdef DELFILE
boolean P_DelWadFile(void)
{
	sfxenum_t i;
	const UINT16 wadnum = (UINT16)(numwadfiles - 1);
	const lumpnum_t lumpnum = numwadfiles<<16;
	//lumpinfo_t *lumpinfo = wadfiles[wadnum]->lumpinfo;
	R_DelSkins(wadnum); // only used by DELFILE
	R_DelSpriteDefs(wadnum); // only used by DELFILE
	for (i = 0; i < NUMSFX; i++)
	{
		if (S_sfx[i].lumpnum != LUMPERROR && S_sfx[i].lumpnum >= lumpnum)
		{
			S_StopSoundByNum(i);
			S_RemoveSoundFx(i);
			if (S_sfx[i].lumpnum != LUMPERROR)
			{
				I_FreeSfx(&S_sfx[i]);
				S_sfx[i].lumpnum = LUMPERROR;
			}
		}
	}
	W_UnloadWadFile(wadnum); // only used by DELFILE
	R_LoadTextures();
	return false;
}
#endif
