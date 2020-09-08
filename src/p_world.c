// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Sonic Team Junior.
// Copyright (C) 2020 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_world.c
/// \brief World state

#include "doomdata.h"
#include "doomtype.h"
#include "doomdef.h"

#include "dehacked.h"

#include "d_main.h"
#include "d_player.h"

#include "g_game.h"

#include "p_local.h"
#include "p_setup.h"
#include "p_spec.h"
#include "p_world.h"

#include "r_data.h"
#include "r_draw.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include "lua_script.h"
#include "lua_hook.h"

world_t *world = NULL;
world_t *localworld = NULL;
world_t *viewworld = NULL;

world_t **worldlist = NULL;
INT32 numworlds = 0;

//
// Initializes a world.
//
world_t *P_InitWorld(void)
{
	world_t *w = Z_Calloc(sizeof(world_t), PU_STATIC, NULL);
	w->gamemap = gamemap;
	w->thlist = Z_Calloc(sizeof(thinker_t) * NUM_THINKERLISTS, PU_STATIC, NULL);
	return w;
}

//
// Initializes a new world, inserting it into the world list.
//
world_t *P_InitNewWorld(void)
{
	world_t *w;

	worldlist = Z_Realloc(worldlist, (numworlds + 1) * sizeof(void *), PU_STATIC, NULL);
	worldlist[numworlds] = P_InitWorld();
	w = worldlist[numworlds];

	numworlds++;
	return w;
}

//
// Sets the current world structures for physics simulation.
// (This was easier than referencing world-> in every part of gamelogic.)
//
void P_SetGameWorld(world_t *w)
{
	world = w;

	numvertexes = w->numvertexes;
	numsegs = w->numsegs;
	numsectors = w->numsectors;
	numsubsectors = w->numsubsectors;
	numnodes = w->numnodes;
	numlines = w->numlines;
	numsides = w->numsides;
	nummapthings = w->nummapthings;

	vertexes = w->vertexes;
	segs = w->segs;
	sectors = w->sectors;
	subsectors = w->subsectors;
	nodes = w->nodes;
	lines = w->lines;
	sides = w->sides;
	mapthings = w->mapthings;

	spawnsectors = w->spawnsectors;
	spawnlines = w->spawnlines;
	spawnsides = w->spawnsides;

	rejectmatrix = w->rejectmatrix;

	blockmaplump = world->blockmaplump;
	blockmap = world->blockmap;
	bmapwidth = world->bmapwidth;
	bmapheight = world->bmapheight;
	bmaporgx = world->bmaporgx;
	bmaporgy = world->bmaporgy;
	blocklinks = world->blocklinks;
	polyblocklinks = world->polyblocklinks;
}

//
// Selects the world for rendering.
//
void P_SetViewWorld(world_t *w)
{
	viewworld = w;
}

//
// Sets a world as visited by a player.
//
void P_SetWorldVisited(player_t *player, world_t *w)
{
	size_t playernum = (size_t)(player - players);
	worldplayerinfo_t *playerinfo = &w->playerinfo[playernum];
	vector3_t *pos = &playerinfo->pos;

	if (!playeringame[playernum] || !player->mo || P_MobjWasRemoved(player->mo))
		return;

	pos->x = player->mo->x;
	pos->y = player->mo->y;
	pos->z = player->mo->z;
	playerinfo->angle = player->mo->angle;

	playerinfo->visited = true;
}

//
// Sets the current world.
//
void P_SetWorld(world_t *w)
{
	if (w == NULL || w == world)
		return;

	P_SetGameWorld(w);
	P_SetViewWorld(w);

	thlist = world->thlist;
	gamemap = w->gamemap;

	P_InitSpecials();
}

//
// Switches a player to a world.
//
void P_SwitchWorld(player_t *player, world_t *w)
{
	size_t playernum = (size_t)(player - players);
	worldplayerinfo_t *playerinfo = &w->playerinfo[playernum];

	if (w == world)
		return;

	if (!playeringame[playernum] || !player->mo || P_MobjWasRemoved(player->mo))
		return;

	P_SetWorldVisited(player, world);

	if (player->followmobj)
	{
		P_RemoveMobj(player->followmobj);
		P_SetTarget(&player->followmobj, NULL);
	}

	world->players--;
	P_SetWorld(w);
	world->players++;

	player->world = world;

	P_UnsetThingPosition(player->mo);
	P_MoveThinkerToWorld(w, THINK_MAIN, (thinker_t *)(player->mo));

	if (playerinfo->visited)
	{
		vector3_t *pos = &playerinfo->pos;
		P_TeleportMove(player->mo, pos->x, pos->y, pos->z);
		P_SetPlayerAngle(player, playerinfo->angle);
	}
	else
		G_MovePlayerToSpawnOrStarpost(playernum);

	P_MapEnd();

	if ((INT32)playernum == consoleplayer)
	{
		localworld = world;
		S_Start();
	}

	P_SetupLevelSky(mapheaderinfo[gamemap-1]->skynum, true);
	P_ResetCamera(player, &camera);
}

void Command_Switchworld_f(void)
{
	INT32 worldnum;
	world_t *w;

    if (COM_Argc() < 2)
		return;

	worldnum = (INT32)atoi(COM_Argv(1));
	if (worldnum < 0 || worldnum >= numworlds)
		return;

	w = worldlist[worldnum];
	CONS_Printf("Switching to world %d (%p)\n", worldnum, w);
	P_SwitchWorld(&players[consoleplayer], w);
}

void Command_Listworlds_f(void)
{
	INT32 worldnum;
	world_t *w;
	worldplayerinfo_t *playerinfo;

    for (worldnum = 0; worldnum < numworlds; worldnum++)
	{
		w = worldlist[worldnum];
		playerinfo = &w->playerinfo[consoleplayer];

		CONS_Printf("World %d (%p)\n", worldnum, w);
		CONS_Printf("Gamemap: %d\n", w->gamemap);
		CONS_Printf("vt %d sg %d sc %d ss %d nd %d ld %d sd %d mt %d\n",
			w->numvertexes, w->numsegs, w->numsectors, w->numsubsectors, w->numnodes, w->numlines, w->numsides, w->nummapthings);

		CONS_Printf("Player has visited: %d\n", playerinfo->visited);
		CONS_Printf("Player position: %d %d %d %d\n",
					playerinfo->pos.x>>FRACBITS,
					playerinfo->pos.y>>FRACBITS,
					playerinfo->pos.z>>FRACBITS,
					AngleFixed(playerinfo->angle)>>FRACBITS);
	}
}

//
// Unloads a world.
//
void P_UnloadWorld(world_t *w)
{
	sector_t *ss;

	LUA_InvalidateLevel(w);

	for (ss = w->sectors; w->sectors+w->numsectors != ss; ss++)
	{
		Z_Free(ss->attached);
		Z_Free(ss->attachedsolid);
	}

	if (w)
		Z_Free(w);
}

//
// Unloads all worlds.
//
void P_UnloadWorldList(void)
{
	INT32 i;

	for (i = 0; i < numworlds; i++)
		P_UnloadWorld(worldlist[i]);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			players[i].world = NULL;
	}

	Z_Free(worldlist);
	numworlds = 0;

	// Clear pointers that would be left dangling by the purge
	R_FlushTranslationColormapCache();

	Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

	world = localworld = NULL;
}

//
// Unloads a player.
//
void P_UnloadWorldPlayer(player_t *player)
{
	if (player->followmobj)
	{
		P_RemoveMobj(player->followmobj);
		P_SetTarget(&player->followmobj, NULL);
	}

	if (player->mo)
	{
		P_RemoveMobj(player->mo);
		P_SetTarget(&player->mo, NULL);
	}
}
