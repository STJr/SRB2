// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_world.h
/// \brief World state

#ifndef __P_WORLD__
#define __P_WORLD__

#include "p_setup.h"
#include "r_state.h"
#include "p_polyobj.h"
#include "p_slopes.h"
#include "taglist.h"
#include "doomstat.h"

// Player spawn spots for deathmatch.
#define MAX_DM_STARTS 64

#define WAYPOINTSEQUENCESIZE 256
#define NUMWAYPOINTSEQUENCES 256

//
// A "world" is the environment that players interact with
// A "map" is what defines how the world is built (what you edit in a level editor)
// And a "level" both describes metadata (level headers), and contains many worlds
//
typedef struct
{
	INT32 gamemap;
	thinker_t *thlist;

	INT32 players;
	boolean loading;

	size_t numvertexes, numsegs, numsectors, numsubsectors, numnodes, numlines, numsides, nummapthings;
	vertex_t *vertexes;
	seg_t *segs;
	sector_t *sectors;
	subsector_t *subsectors;
	node_t *nodes;
	line_t *lines;
	side_t *sides;
	mapthing_t *mapthings;

	extrasubsector_t *extrasubsectors;
	size_t numextrasubsectors;

	sector_t *spawnsectors;
	line_t *spawnlines;
	side_t *spawnsides;

	size_t numflats;
	levelflat_t *flats;

	pslope_t *slopelist;
	UINT16 slopecount;

	mobj_t *shields[MAXPLAYERS*2];
	INT32 numshields;

	mobj_t *overlaycap;

	actioncache_t actioncachehead;

	mapheader_t *header;

	fixed_t gravity;

	INT32 skynum; // used for keeping track of the current sky
	UINT8 weather;

	// the texture number of the sky texture
	INT32 skytexture;

	// Needed to store the number of the dummy sky flat.
	// Used for rendering, as well as tracking projectiles etc.
	INT32 skyflatnum;

	// Player spawn spots.
	mapthing_t *playerstarts[MAXPLAYERS]; // Cooperative
	mapthing_t *bluectfstarts[MAXPLAYERS]; // CTF
	mapthing_t *redctfstarts[MAXPLAYERS]; // CTF
	mapthing_t *deathmatchstarts[MAX_DM_STARTS];

	// Maintain single and multi player starting spots.
	INT32 numdmstarts, numcoopstarts, numredctfstarts, numbluectfstarts;

	mobj_t *skyboxmo[2]; // current skybox mobjs: 0 = viewpoint, 1 = centerpoint
	mobj_t *skyboxviewpnts[16]; // array of MT_SKYBOX viewpoint mobjs
	mobj_t *skyboxcenterpnts[16]; // array of MT_SKYBOX centerpoint mobjs

	mobj_t *waypoints[NUMWAYPOINTSEQUENCES][WAYPOINTSEQUENCESIZE];
	UINT16 numwaypoints[NUMWAYPOINTSEQUENCES];

	mapthing_t *itemrespawnque[ITEMQUESIZE];
	tic_t itemrespawntime[ITEMQUESIZE];
	size_t iquehead, iquetail;

	struct quake quake;

	// Emerald locations
	mobj_t *emerald_hunt_locations[NUM_EMERALD_HUNT_LOCATIONS];

	// All that boring blockmap stuff
	UINT8 *rejectmatrix; // for fast sight rejection
	INT32 *blockmaplump; // offsets in blockmap are from here
	INT32 *blockmap; // Big blockmap
	INT32 bmapwidth;
	INT32 bmapheight; // in mapblocks
	fixed_t bmaporgx;
	fixed_t bmaporgy; // origin of block map
	mobj_t **blocklinks; // for thing chains

	// The Polyobjects
	polyobj_t *PolyObjects;
	INT32 numPolyObjects;
	polymaplink_t **polyblocklinks; // Polyobject Blockmap -- initialized in P_LoadBlockMap
	polymaplink_t *po_bmap_freelist; // free list of blockmap links

	// Bit array of whether a tag exists for sectors/lines/things.
	bitarray_t tags_available[BIT_ARRAY_SIZE (MAXTAGS)];

	size_t num_tags;

	// Taggroups are used to list elements of the same tag, for iteration.
	// Since elements can now have multiple tags, it means an element may appear
	// in several taggroups at the same time. These are built on level load.
	taggroup_t* tags_sectors[MAXTAGS + 1];
	taggroup_t* tags_lines[MAXTAGS + 1];
	taggroup_t* tags_mapthings[MAXTAGS + 1];

	void **interpolators;
	size_t interpolators_len;
	size_t interpolators_size;

	void **interpolated_mobjs;
	size_t interpolated_mobjs_len;
	size_t interpolated_mobjs_capacity;

	boolean interpolated_level_this_frame;

	void *sky_dome;
} world_t;

extern world_t *world;
extern world_t *baseworld;
extern world_t *localworld;
extern world_t *viewworld;

extern world_t **worldlist;
extern INT32 numworlds;

typedef struct
{
	world_t *world;
	fixed_t x, y, z;
	angle_t angle;
} location_t;

world_t *P_InitWorld(void);
world_t *P_InitNewWorld(void);

void P_UnloadWorld(world_t *w);
void P_UnloadWorldList(void);
void P_UnloadWorldPlayer(player_t *player);

void P_SetGameWorld(world_t *w);
void P_SetViewWorld(world_t *w);

void P_SetWorld(world_t *w);

void P_RoamIntoWorld(player_t *player, INT32 mapnum);
void P_SwitchWorld(player_t *player, world_t *w, location_t *location);
void P_MovePlayerToLocation(player_t *player, location_t *location);

void P_InitCachedActions(world_t *w);
void P_RunCachedActions(world_t *w);
void P_AddCachedAction(world_t *w, mobj_t *mobj, INT32 statenum);

world_t *P_GetPlayerWorld(player_t *player);

void P_DetachPlayerWorld(player_t *player);
void P_SwitchPlayerWorld(player_t *player, world_t *newworld);

boolean P_TransferCarriedPlayers(player_t *player, world_t *w);
boolean P_MobjIsConnected(mobj_t *mobj1, mobj_t *mobj2);
void P_RemoveMobjConnections(mobj_t *mobj, world_t *w);
world_t *P_GetMobjWorld(mobj_t *mobj);

void Command_Switchworld_f(void);
void Command_Listworlds_f(void);

void P_SetupWorldSky(INT32 skynum, world_t *w);
INT32 P_AddLevelFlatForWorld(world_t *w, const char *flatname);

#endif
