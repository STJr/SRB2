// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2019 by Sonic Team Junior.
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
#include "r_patch.h"
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

#include "filesrch.h" // refreshdirmenu

#ifdef HAVE_BLUA
#include "lua_hud.h" // level title
#endif

#include "f_finale.h" // wipes

#include "md5.h" // map MD5

// for LUAh_MapLoad
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
	mapheaderinfo[num]->startrings = 0;
	snprintf(mapheaderinfo[num]->musname, 7, "%sM", G_BuildMapName(i));
	mapheaderinfo[num]->musname[6] = 0;
	mapheaderinfo[num]->mustrack = 0;
	mapheaderinfo[num]->muspos = 0;
	mapheaderinfo[num]->musinterfadeout = 0;
	mapheaderinfo[num]->musintername[0] = '\0';
	mapheaderinfo[num]->muspostbossname[6] = 0;
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

// Loads the vertexes for a level.
static inline void P_LoadRawVertexes(UINT8 *data)
{
	mapvertex_t *ml = (mapvertex_t *)data;
	vertex_t *li = vertexes;
	size_t i;

	// Copy and convert vertex coordinates, internal representation as fixed.
	for (i = 0; i < numvertexes; i++, li++, ml++)
	{
		li->x = SHORT(ml->x)<<FRACBITS;
		li->y = SHORT(ml->y)<<FRACBITS;
	}
}

/** Computes the length of a seg in fracunits.
  *
  * \param seg Seg to compute length for.
  * \return Length in fracunits.
  */
fixed_t P_SegLength(seg_t *seg)
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

// Loads the SEGS resource from a level.
static void P_LoadRawSegs(UINT8 *data)
{
	INT32 linedef, side;
	mapseg_t *ml = (mapseg_t*)data;
	seg_t *li = segs;
	line_t *ldef;
	size_t i;

	for (i = 0; i < numsegs; i++, li++, ml++)
	{
		li->v1 = &vertexes[SHORT(ml->v1)];
		li->v2 = &vertexes[SHORT(ml->v2)];

		li->length = P_SegLength(li);
#ifdef HWRENDER
		if (rendermode == render_opengl)
		{
			li->flength = P_SegLengthFloat(li);
			//Hurdler: 04/12/2000: for now, only used in hardware mode
			li->lightmaps = NULL; // list of static lightmap for this seg
		}
		li->pv1 = li->pv2 = NULL;
#endif

		li->angle = (SHORT(ml->angle))<<FRACBITS;
		li->offset = (SHORT(ml->offset))<<FRACBITS;
		linedef = SHORT(ml->linedef);
		ldef = &lines[linedef];
		li->linedef = ldef;
		li->side = side = SHORT(ml->side);
		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;
		if (ldef->flags & ML_TWOSIDED)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
			li->backsector = 0;

		li->numlights = 0;
		li->rlights = NULL;
	}
}

// Loads the SSECTORS resource from a level.
static inline void P_LoadRawSubsectors(void *data)
{
	mapsubsector_t *ms = (mapsubsector_t*)data;
	subsector_t *ss = subsectors;
	size_t i;

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
}

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
Ploadflat (levelflat_t *levelflat, const char *flatname)
{
#ifndef NO_PNG_LUMPS
	UINT8         buffer[8];
#endif

	lumpnum_t    flatnum;
	int       texturenum;

	size_t i;

	if (levelflat)
	{
		// Scan through the already found flats, return if it matches.
		for (i = 0; i < numlevelflats; i++)
		{
			if (strnicmp(levelflat[i].name, flatname, 8) == 0)
				return i;
		}
	}

#ifndef ZDEBUG
	CONS_Debug(DBG_SETUP, "flat #%03d: %s\n", atoi(sizeu1(numlevelflats)), levelflat->name);
#endif

	if (numlevelflats >= MAXLEVELFLATS)
		I_Error("Too many flats in level\n");

	if (levelflat)
		levelflat += numlevelflats;
	else
	{
		// allocate new flat memory
		levelflats = Z_Realloc(levelflats, (numlevelflats + 1) * sizeof(*levelflats), PU_LEVEL, NULL);
		levelflat  = levelflats + numlevelflats;
	}

	// Store the name.
	strlcpy(levelflat->name, flatname, sizeof (levelflat->name));
	strupr(levelflat->name);

	/* If we can't find a flat, try looking for a texture! */
	if (( flatnum = R_GetFlatNumForName(flatname) ) == LUMPERROR)
	{
		if (( texturenum = R_CheckTextureNumForName(flatname) ) == -1)
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
		if (R_CheckIfPatch(flatnum))
			levelflat->type = LEVELFLAT_PATCH;
		else
		{
#ifndef NO_PNG_LUMPS
			/*
			Only need eight bytes for PNG headers.
			FIXME: Put this elsewhere.
			*/
			W_ReadLumpHeader(flatnum, buffer, 8, 0);
			if (R_IsLumpPNG(buffer, W_LumpLength(flatnum)))
				levelflat->type = LEVELFLAT_PNG;
			else
#endif/*NO_PNG_LUMPS*/
				levelflat->type = LEVELFLAT_FLAT;/* phew */
		}

		levelflat->u.flat.    lumpnum = flatnum;
		levelflat->u.flat.baselumpnum = LUMPERROR;
	}

	return ( numlevelflats++ );
}

// Auxiliary function. Find a flat in the active wad files,
// allocate an id for it, and set the levelflat (to speedup search)
INT32 P_AddLevelFlat(const char *flatname, levelflat_t *levelflat)
{
	return Ploadflat(levelflat, flatname);
}

// help function for Lua and $$$.sav reading
// same as P_AddLevelFlat, except this is not setup so we must realloc levelflats to fit in the new flat
// no longer a static func in lua_maplib.c because p_saveg.c also needs it
//
INT32 P_AddLevelFlatRuntime(const char *flatname)
{
	return Ploadflat(levelflats, flatname);
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

// Sets up the ingame sectors structures.
static void P_LoadRawSectors(UINT8 *data)
{
	mapsector_t *ms = (mapsector_t *)data;
	sector_t *ss = sectors;
	levelflat_t *foundflats;
	size_t i;

	// Allocate a big chunk of memory as big as our MAXLEVELFLATS limit.
	//Fab : FIXME: allocate for whatever number of flats - 512 different flats per level should be plenty
	foundflats = calloc(MAXLEVELFLATS, sizeof (*foundflats));
	if (foundflats == NULL)
		I_Error("Ran out of memory while loading sectors\n");

	numlevelflats = 0;

	// For each counted sector, copy the sector raw data from our cache pointer ms, to the global table pointer ss.
	for (i = 0; i < numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;

		ss->floorpic = P_AddLevelFlat(ms->floorpic, foundflats);
		ss->ceilingpic = P_AddLevelFlat(ms->ceilingpic, foundflats);

		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->nexttag = ss->firsttag = -1;

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
		ss->spawn_extra_colormap = NULL;

		ss->floor_xoffs = ss->ceiling_xoffs = ss->floor_yoffs = ss->ceiling_yoffs = 0;
		ss->floorpic_angle = ss->ceilingpic_angle = 0;
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
static void P_LoadRawNodes(UINT8 *data)
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
		 || mt->type == mobjinfo[MT_BLUESPHERE].doomednum || mt->type == mobjinfo[MT_BOMBSPHERE].doomednum
		 || (mt->type >= 600 && mt->type <= 609)) // circles and diagonals
		{
			mt->mobj = NULL;

			P_SpawnHoopsAndRings(mt, true);
		}
	}
	for (i = 0; i < numHoops; i++)
	{
		P_SpawnHoopsAndRings(hoopsToRespawn[i], false);
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

static void P_PrepareRawThings(UINT8 *data)
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

		mt->type &= 4095;

		if (mt->type == 1705 || (mt->type == 750 && mt->extrainfo))
			mt->z = mt->options; // NiGHTS Hoops use the full flags bits to set the height.
		else
			mt->z = mt->options >> ZSHIFT;
	}
}

static void P_SpawnEmeraldHunt(void)
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
	if (emer1--)
		P_SpawnMobj(huntemeralds[emer1]->x<<FRACBITS,
			huntemeralds[emer1]->y<<FRACBITS,
			huntemeralds[emer1]->z<<FRACBITS, MT_EMERHUNT);

	if (emer2--)
		P_SetMobjStateNF(P_SpawnMobj(huntemeralds[emer2]->x<<FRACBITS,
			huntemeralds[emer2]->y<<FRACBITS,
			huntemeralds[emer2]->z<<FRACBITS, MT_EMERHUNT),
		mobjinfo[MT_EMERHUNT].spawnstate+1);

	if (emer3--)
		P_SetMobjStateNF(P_SpawnMobj(huntemeralds[emer3]->x<<FRACBITS,
			huntemeralds[emer3]->y<<FRACBITS,
			huntemeralds[emer3]->z<<FRACBITS, MT_EMERHUNT),
		mobjinfo[MT_EMERHUNT].spawnstate+2);
}

static void P_LoadThings(boolean loademblems)
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

		if (!loademblems && mt->type == mobjinfo[MT_EMBLEM].doomednum)
			continue;

		mt->mobj = NULL;
		P_SpawnMapThing(mt);
	}

	// random emeralds for hunt
	if (numhuntemeralds)
		P_SpawnEmeraldHunt();

	if (metalrecording) // Metal Sonic gets no rings to distract him.
		return;

	// Run through the list of mapthings again to spawn hoops and rings
	mt = mapthings;
	for (i = 0; i < nummapthings; i++, mt++)
	{
		if (mt->type == mobjinfo[MT_RING].doomednum || mt->type == mobjinfo[MT_COIN].doomednum
		 || mt->type == mobjinfo[MT_REDTEAMRING].doomednum || mt->type == mobjinfo[MT_BLUETEAMRING].doomednum
		 || mt->type == mobjinfo[MT_BLUESPHERE].doomednum || mt->type == mobjinfo[MT_BOMBSPHERE].doomednum
		 || (mt->type >= 600 && mt->type <= 609) // circles and diagonals
		 || mt->type == 1705 || mt->type == 1713) // hoops
		{
			mt->mobj = NULL;
			P_SpawnHoopsAndRings(mt, false);
		}
	}
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

static void P_LoadRawLineDefs(UINT8 *data)
{
	maplinedef_t *mld = (maplinedef_t *)data;
	line_t *ld = lines;
	size_t i;

	for (i = 0; i < numlines; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);
		ld->special = SHORT(mld->special);
		ld->tag = SHORT(mld->tag);
		ld->v1 = &vertexes[SHORT(mld->v1)];
		ld->v2 = &vertexes[SHORT(mld->v2)];

		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);
	}
}

static void P_SetupLines(void)
{
	line_t *ld = lines;
	size_t i;

	for (i = 0; i < numlines; i++, ld++)
	{
		vertex_t *v1 = ld->v1;
		vertex_t *v2 = ld->v2;

#ifdef WALLSPLATS
		ld->splats = NULL;
#endif

#ifdef POLYOBJECTS
		ld->polyobj = NULL;
#endif

		ld->dx = v2->x - v1->x;
		ld->dy = v2->y - v1->y;

		if (!ld->dx)
			ld->slopetype = ST_VERTICAL;
		else if (!ld->dy)
			ld->slopetype = ST_HORIZONTAL;
		else if ((ld->dy > 0) == (ld->dx > 0))
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

		{
			// cph 2006/09/30 - fix sidedef errors right away.
			// cph 2002/07/20 - these errors are fatal if not fixed, so apply them
			UINT8 j;

			for (j=0; j < 2; j++)
				if (ld->sidenum[j] != 0xffff && ld->sidenum[j] >= (UINT16)numsides)
				{
					ld->sidenum[j] = 0xffff;
					CONS_Debug(DBG_SETUP, "P_LoadRawLineDefs: linedef %s has out-of-range sidedef number\n", sizeu1(numlines-i-1));
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
			CONS_Debug(DBG_SETUP, "Linedef %s missing first sidedef\n", sizeu1(numlines-i-1));
		}

		if ((ld->sidenum[1] == 0xffff) && (ld->flags & ML_TWOSIDED))
		{
			ld->flags &= ~ML_TWOSIDED;  // Clear 2s flag for missing left side
			// cph - print a warning about the bug
			CONS_Debug(DBG_SETUP, "Linedef %s has two-sided flag set, but no second sidedef\n", sizeu1(numlines-i-1));
		}

		if (ld->sidenum[0] != 0xffff && ld->special)
			sides[ld->sidenum[0]].special = ld->special;
		if (ld->sidenum[1] != 0xffff && ld->special)
			sides[ld->sidenum[1]].special = ld->special;
	}
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
		if ((ld->flags & ML_EFFECT5) && (ld->sidenum[1] != 0xffff)
			&& !(ld->special >= 300 && ld->special < 500)) // exempt linedef exec specials
		{
			sides[ld->sidenum[0]].repeatcnt = (INT16)(((unsigned)sides[ld->sidenum[0]].textureoffset >> FRACBITS) >> 12);
			sides[ld->sidenum[0]].textureoffset = (((unsigned)sides[ld->sidenum[0]].textureoffset >> FRACBITS) & 2047) << FRACBITS;
			sides[ld->sidenum[1]].repeatcnt = (INT16)(((unsigned)sides[ld->sidenum[1]].textureoffset >> FRACBITS) >> 12);
			sides[ld->sidenum[1]].textureoffset = (((unsigned)sides[ld->sidenum[1]].textureoffset >> FRACBITS) & 2047) << FRACBITS;
		}

		// Compile linedef 'text' from both sidedefs 'text' for appropriate specials.
		switch(ld->special)
		{
		case 331: // Trigger linedef executor: Skin - Continuous
		case 332: // Trigger linedef executor: Skin - Each time
		case 333: // Trigger linedef executor: Skin - Once
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

static void P_LoadRawSideDefs2(void *data)
{
	UINT16 i;

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
				CONS_Debug(DBG_SETUP, "P_LoadRawSideDefs2: sidedef %u has out-of-range sector num %u\n", i, sector_num);
				sector_num = 0;
			}
			sd->sector = sec = &sectors[sector_num];
		}

		sd->sector = sec = &sectors[SHORT(msd->sector)];

		sd->colormap_data = NULL;

		// Colormaps!
		switch (sd->special)
		{
			case 63: // variable colormap via 242 linedef
			case 606: //SoM: 4/4/2000: Just colormap transfer
			case 447: // Change colormap of tagged sectors! -- Monster Iestyn 14/06/18
			case 455: // Fade colormaps! mazmazz 9/12/2018 (:flag_us:)
				// SoM: R_CreateColormap will only create a colormap in software mode...
				// Perhaps we should just call it instead of doing the calculations here.
				sd->colormap_data = R_CreateColormap(msd->toptexture, msd->midtexture,
					msd->bottomtexture);
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

				// always process if back sidedef, because we need that - symbol
 				sd->text = Z_Malloc(7, PU_LEVEL, NULL);
				if (i == 1 || msd->toptexture[0] != '-' || msd->toptexture[1] != '\0')
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

			case 9: // Mace parameters
			case 14: // Bustable block parameters
			case 15: // Fan particle spawner parameters
			case 425: // Calls P_SetMobjState on calling mobj
			case 434: // Custom Power
			case 442: // Calls P_SetMobjState on mobjs of a given type in the tagged sectors
			case 461: // Spawns an object on the map based on texture offsets
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
			case 443: // Calls a named Lua function
			case 459: // Control text prompt (named tag)
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
static boolean P_LoadRawBlockMap(UINT8 *data, size_t count)
{
#if 0
	(void)data;
	(void)count;
	(void)lumpname;
	return false;
#else

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

#ifdef POLYOBJECTS
	// haleyjd 2/22/06: setup polyobject blockmap
	count = sizeof(*polyblocklinks) * bmapwidth * bmapheight;
	polyblocklinks = Z_Calloc(count, PU_LEVEL, NULL);
#endif
	return true;
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

// PK3 version
// -- Monster Iestyn 09/01/18
static void P_LoadRawReject(UINT8 *data, size_t count)
{
	if (!count) // zero length, someone probably used ZDBSP
	{
		rejectmatrix = NULL;
		CONS_Debug(DBG_SETUP, "P_LoadRawReject: REJECT lump has size 0, will not be loaded\n");
	}
	else
	{
		rejectmatrix = Z_Malloc(count, PU_LEVEL, NULL); // allocate memory for the reject matrix
		M_Memcpy(rejectmatrix, data, count); // copy the data into it
	}
}

static void P_LoadMapBSP(const virtres_t* virt)
{
	virtlump_t* virtssectors = vres_Find(virt, "SSECTORS");
	virtlump_t* virtsegs     = vres_Find(virt, "SEGS");
	virtlump_t* virtnodes    = vres_Find(virt, "NODES");

	numsubsectors = virtssectors->size / sizeof(mapsubsector_t);
	numnodes      = virtnodes->size    / sizeof(mapnode_t);
	numsegs       = virtsegs->size     / sizeof(mapseg_t);

	if (numsubsectors <= 0)
		I_Error("Level has no subsectors (did you forget to run it through a nodesbuilder?)");
	if (numnodes <= 0)
		I_Error("Level has no nodes");
	if (numsegs <= 0)
		I_Error("Level has no segs");

	subsectors = Z_Calloc(numsubsectors * sizeof(*subsectors), PU_LEVEL, NULL);
	nodes      = Z_Calloc(numnodes * sizeof(*nodes), PU_LEVEL, NULL);
	segs       = Z_Calloc(numsegs * sizeof(*segs), PU_LEVEL, NULL);

	// Nodes
	P_LoadRawSubsectors(virtssectors->data);
	P_LoadRawNodes(virtnodes->data);
	P_LoadRawSegs(virtsegs->data);
}

static void P_LoadMapLUT(const virtres_t* virt)
{
	virtlump_t* virtblockmap = vres_Find(virt, "BLOCKMAP");
	virtlump_t* virtreject   = vres_Find(virt, "REJECT");

	// Lookup tables
	if (virtreject)
		P_LoadRawReject(virtreject->data, virtreject->size);
	else
		rejectmatrix = NULL;

	if (!(virtblockmap && P_LoadRawBlockMap(virtblockmap->data, virtblockmap->size)))
		P_CreateBlockMap();
}

static void P_LoadMapData(const virtres_t* virt)
{
	virtlump_t* virtvertexes = NULL, * virtsectors = NULL, * virtsidedefs = NULL, * virtlinedefs = NULL, * virtthings = NULL;
#ifdef UDMF
	virtlump_t* textmap = vres_Find(virt, "TEXTMAP");

	// Count map data.
	if (textmap)
	{
		nummapthings = 0;
		numlines = 0;
		numsides = 0;
		numvertexes = 0;
		numsectors = 0;

		// Count how many entries for each type we got in textmap.
		//TextmapCount(vtextmap->data, vtextmap->size);
	}
	else
#endif
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

#ifdef UDMF
	if (textmap)
	{

	}
	else
#endif
	{
		// Strict map data
		P_LoadRawVertexes(virtvertexes->data);
		P_LoadRawSectors(virtsectors->data);
		P_LoadRawLineDefs(virtlinedefs->data);
		P_SetupLines();
		P_LoadRawSideDefs2(virtsidedefs->data);
	}
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
	boolean canresetlives = true;

	leveltime = 0;

	localaiming = 0;
	localaiming2 = 0;
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

	// special stage
	stagefailed = true; // assume failed unless proven otherwise - P_GiveEmerald or emerald touchspecial
	// Reset temporary record data
	memset(&ntemprecords, 0, sizeof(nightsdata_t));

	// earthquake camera
	memset(&quake,0,sizeof(struct quake));

	if ((netgame || multiplayer) && gametype == GT_COOP && cv_coopstarposts.value == 2)
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

		if (canresetlives && (netgame || multiplayer) && playeringame[i] && (gametype == GT_COMPETITION || players[i].lives <= 0))
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
		CV_SetValue(&cv_analog2, true);
}

//
// P_LoadThingsOnly
//
// "Reloads" a level, but only reloads all of the mobjs.
//
void P_LoadThingsOnly(void)
{
	// Search through all the thinkers.
	thinker_t *think;
	INT32 i, viewid = -1, centerid = -1; // for skyboxes
	virtres_t* virt = vres_GetMap(lastloadedmaplumpnum);
	virtlump_t* vth = vres_Find(virt, "THINGS");

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

	P_LevelInitStuff();

	P_PrepareRawThings(vth->data);
	P_LoadThings(true);

	vres_Free(virt);

	// restore skybox viewpoint/centerpoint if necessary, set them to defaults if we can't do that
	skyboxmo[0] = skyboxviewpnts[(viewid >= 0) ? viewid : 0];
	skyboxmo[1] = skyboxcenterpnts[(centerid >= 0) ? centerid : 0];
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

static void P_MakeMapMD5(virtres_t* virt, void *dest)
{
	unsigned char linemd5[16];
	unsigned char sectormd5[16];
	unsigned char thingmd5[16];
	unsigned char sidedefmd5[16];
	unsigned char resmd5[16];
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
			camera.subsector = R_PointInSubsector(camera.x, camera.y); // make sure camera has a subsector set -- Monster Iestyn (12/11/18)
		}
	}
}

static boolean CanSaveLevel(INT32 mapnum)
{
	if (ultimatemode) // never save in ultimate (probably redundant with cursaveslot also being checked)
		return false;

	if (G_IsSpecialStage(mapnum) // don't save in special stages
		|| mapnum == lastmaploaded) // don't save if the last map loaded was this one
		return false;

	// Any levels that have the savegame flag can save normally.
	// If the game is complete for this save slot, then any level can save!
	// On the other side of the spectrum, if lastmaploaded is 0, then the save file has only just been created and needs to save ASAP!
	return (mapheaderinfo[mapnum-1]->levelflags & LF_SAVEGAME || gamecomplete || !lastmaploaded);
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

	if (mapheaderinfo[gamemap-1]->forcecharacter[0] != '\0')
		P_ForceCharacter(mapheaderinfo[gamemap-1]->forcecharacter);

	// chasecam on in chaos, race, coop
	// chasecam off in match, tag, capture the flag
	chase = (gametype == GT_RACE || gametype == GT_COMPETITION || gametype == GT_COOP)
		|| (maptol & TOL_2D);

	if (!dedicated)
	{
		// Salt: CV_ClearChangedFlags() messes with your settings :(
		/*if (!cv_cam_speed.changed)
			CV_Set(&cv_cam_speed, cv_cam_speed.defaultvalue);*/

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
	wipegamestate = FORCEWIPEOFF;
	wipestyleflags = 0;

	// Special stage fade to white
	// This is handled BEFORE sounds are stopped.
	if (modeattacking && !demoplayback && (pausedelay == INT32_MIN))
		ranspecialwipe = 2;
	else if (rendermode != render_none && G_IsSpecialStage(gamemap))
	{
		tic_t starttime = I_GetTime();
		tic_t endtime = starttime + (3*TICRATE)/2;
		tic_t nowtime;

		S_StartSound(NULL, sfx_s3kaf);

		// Fade music! Time it to S3KAF: 0.25 seconds is snappy.
		if (RESETMUSIC ||
			strnicmp(S_MusicName(),
				(mapmusflags & MUSIC_RELOADRESET) ? mapheaderinfo[gamemap-1]->musname : mapmusname, 7))
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
				I_Sleep();
			lastwipetic = nowtime;
			if (moviemode) // make sure we save frames for the white hold too
				M_SaveFrame();
		}

		ranspecialwipe = 1;
	}

	if (G_GetModeAttackRetryFlag())
	{
		if (modeattacking)
			wipestyleflags |= (WSF_FADEOUT|WSF_TOWHITE);
		G_ClearModeAttackRetryFlag();
	}

	// Make sure all sounds are stopped before Z_FreeTags.
	S_StopSounds();
	S_ClearSfx();

	// Fade out music here. Deduct 2 tics so the fade volume actually reaches 0.
	// But don't halt the music! S_Start will take care of that. This dodges a MIDI crash bug.
	if (!titlemapinaction && (RESETMUSIC ||
		strnicmp(S_MusicName(),
			(mapmusflags & MUSIC_RELOADRESET) ? mapheaderinfo[gamemap-1]->musname : mapmusname, 7)))
		S_FadeMusic(0, FixedMul(
			FixedDiv((F_GetWipeLength(wipedefs[wipe_level_toblack])-2)*NEWTICRATERATIO, NEWTICRATE), MUSICRATE));

	// Let's fade to black here
	// But only if we didn't do the special stage wipe
	if (rendermode != render_none && !ranspecialwipe)
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

	if (!titlemapinaction)
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
	else if (savedata.lives > 0)
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
	//lastloadedmaplumpnum = LUMPERROR;
	lastloadedmaplumpnum = W_CheckNumForName(maplumpname);

	if (lastloadedmaplumpnum == INT16_MAX)
		I_Error("Map %s not found.\n", maplumpname);

	R_ReInitColormaps(mapheaderinfo[gamemap-1]->palette);
	CON_SetupBackColormap();

	// SRB2 determines the sky texture to be used depending on the map header.
	P_SetupLevelSky(mapheaderinfo[gamemap-1]->skynum, true);

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

	P_MapStart();

	if (lastloadedmaplumpnum)
	{
		virtres_t* virt = vres_GetMap(lastloadedmaplumpnum);

		P_LoadMapData(virt);
		P_LoadMapBSP(virt);
		P_LoadMapLUT(virt);

		P_LoadLineDefs2();
		P_GroupLines();

		// Copy relevant map data for NetArchive purposes.
		spawnsectors = Z_Calloc(numsectors * sizeof (*sectors), PU_LEVEL, NULL);
		spawnlines = Z_Calloc(numlines * sizeof (*lines), PU_LEVEL, NULL);
		spawnsides = Z_Calloc(numsides * sizeof (*sides), PU_LEVEL, NULL);

		memcpy(spawnsectors, sectors, numsectors * sizeof (*sectors));
		memcpy(spawnlines, lines, numlines * sizeof (*lines));
		memcpy(spawnsides, sides, numsides * sizeof (*sides));

		P_PrepareRawThings(vres_Find(virt, "THINGS")->data);

		P_MakeMapMD5(virt, &mapmd5);

		vres_Free(virt);
	}

	// init gravity, tag lists,
	// anything that P_ResetDynamicSlopes/P_LoadThings needs to know
	P_InitSpecials();

#ifdef ESLOPE
	P_ResetDynamicSlopes(fromnetsave);
#endif

	P_LoadThings(loademblems);
	skyboxmo[0] = skyboxviewpnts[0];
	skyboxmo[1] = skyboxcenterpnts[0];

	for (numcoopstarts = 0; numcoopstarts < MAXPLAYERS; numcoopstarts++)
		if (!playerstarts[numcoopstarts])
			break;

	// set up world state
	P_SpawnSpecials(fromnetsave);

	if (loadprecip) //  ugly hack for P_NetUnArchiveMisc (and P_LoadNetGame)
		P_SpawnPrecipitation();

#ifdef HWRENDER // not win32 only 19990829 by Kin
	if (rendermode == render_opengl)
	{
		// Lactozilla (December 8, 2019)
		// Level setup used to free EVERY mipmap from memory.
		// Even mipmaps that aren't related to level textures.
		// Presumably, the hardware render code used to store textures as level data.
		// Meaning, they had memory allocated and marked with the PU_LEVEL tag.
		// Level textures are only reloaded after R_LoadTextures, which is
		// when the texture list is loaded.
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

	// restore time in netgame (see also g_game.c)
	if ((netgame || multiplayer) && gametype == GT_COOP && cv_coopstarposts.value == 2)
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

	if (unlockables[27].unlocked && !modeattacking // pandora's box
#ifndef DEVELOP
	&& !modifiedgame
#endif
	&& !(netgame || multiplayer) && gamemap == 0x1d35-016464)
	{
		P_SpawnMobj(0640370000, 0x11000000, 0x3180000, MT_LETTER)->angle = ANGLE_90;
		if (textprompts[199]->page[1].backcolor != 259)
		{
			char *buf = W_CacheLumpName("WATERMAP", PU_STATIC), *b = buf;
			while ((*b != 65) && (b-buf < 256)) { *b = (*b - 65)&255; b++; } *b = '\0';
			Z_Free(textprompts[199]->page[1].text);
			textprompts[199]->page[1].text = Z_StrDup(buf);
			textprompts[199]->page[1].lines = 4;
			textprompts[199]->page[1].backcolor = 259;
			Z_Free(buf);
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
	else if (gametype == GT_RACE && server)
		CV_StealthSetValue(&cv_numlaps,
			(cv_basenumlaps.value)
			? cv_basenumlaps.value
			: mapheaderinfo[gamemap - 1]->numlaps);

	// ===========
	// landing point for netgames.
	netgameskip:

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

		if (!cv_analog.changed)
			CV_SetValue(&cv_analog, 0);
		if (!cv_analog2.changed)
			CV_SetValue(&cv_analog2, 0);

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

	P_MapEnd();

	// Remove the loading shit from the screen
	if (rendermode != render_none && !titlemapinaction)
		F_WipeColorFill(levelfadecol);

	if (precache || dedicated)
		R_PrecacheLevel();

	nextmapoverride = 0;
	skipstats = 0;

	if (!(netgame || multiplayer) && (!modifiedgame || savemoddata))
		mapvisited[gamemap-1] |= MV_VISITED;
	else
		mapvisited[gamemap-1] |= MV_MP; // you want to record that you've been there this session, but not permanently

	levelloading = false;

	P_RunCachedActions();

	if (!(netgame || multiplayer || demoplayback || demorecording || metalrecording || modeattacking || players[consoleplayer].lives <= 0)
		&& (!modifiedgame || savemoddata) && cursaveslot > 0 && CanSaveLevel(gamemap))
		G_SaveGame((UINT32)cursaveslot);

	lastmaploaded = gamemap; // HAS to be set after saving!!

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

	// No render mode, stop here.
	if (rendermode == render_none)
		return true;

	// Title card!
	G_StartTitleCard();

	// Can the title card actually run, though?
	if (!WipeStageTitle)
		return true;
	if (ranspecialwipe == 2)
		return true;

	// If so...
	if ((!(mapheaderinfo[gamemap-1]->levelflags & LF_NOTITLECARD)) && (*mapheaderinfo[gamemap-1]->lvlttl != '\0'))
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
	if (!stricmp(lumpinfo->name2, folName))
	{
		lumpinfo++;
		*start = ++i;
		for (; i < numlumps; i++, lumpinfo++)
			if (strnicmp(lumpinfo->name2, folName, strlen(folName)))
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
boolean P_AddWadFile(const char *wadfilename)
{
	size_t i, j, sreplaces = 0, mreplaces = 0, digmreplaces = 0;
	UINT16 numlumps, wadnum;
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

	// Init file.
	if ((numlumps = W_InitFile(wadfilename, false)) == INT16_MAX)
	{
		refreshdirmenu |= REFRESHDIR_NOTLOADED;
		CONS_Printf(M_GetText("Errors occurred while loading %s; not added.\n"), wadfilename);
		return false;
	}
	else
		wadnum = (UINT16)(numwadfiles-1);

	switch(wadfiles[wadnum]->type)
	{
	case RET_PK3:
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
#ifdef HAVE_BLUA
//		if (luaNum) // Lua scripts.
//			P_LoadLuaScrRange(wadnum, luaPos, luaNum);
#endif
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


	//
	// search for sprite replacements
	//
	R_AddSpriteDefs(wadnum);

	// Reload it all anyway, just in case they
	// added some textures but didn't insert a
	// TEXTURES/etc. list.
	R_LoadTextures(); // numtexture changes

	// Reload ANIMDEFS
	P_InitPicAnims();

	// Flush and reload HUD graphics
	ST_UnloadGraphics();
	HU_LoadGraphics();
	ST_LoadGraphics();

	//
	// look for skins
	//
	R_AddSkins(wadnum); // faB: wadfile index in wadfiles[]
	R_PatchSkins(wadnum); // toast: PATCH PATCH
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
