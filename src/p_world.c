// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by Sonic Team Junior.
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

#include "r_main.h" // R_PointInSubsector
#include "r_data.h"
#include "r_draw.h"
#include "r_sky.h"
#include "r_patch.h"
#include "r_fps.h"

#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_video.h"

#include "lua_script.h"
#include "lua_hook.h"

#ifdef HWRENDER
#include "hardware/hw_glob.h"
#endif

world_t *world = NULL;
world_t *baseworld = NULL;
world_t *localworld = NULL;
world_t *viewworld = NULL;

world_t **worldlist = NULL;
INT32 numworlds = 0;

//
// Creates a world.
//
world_t *World_Create(INT16 mapnum)
{
	world_t *w = Z_Calloc(sizeof(world_t), PU_STATIC, NULL);
	w->gamemap = mapnum;
	if (!mapheaderinfo[mapnum-1])
		P_AllocMapHeader(mapnum-1);
	w->header = mapheaderinfo[mapnum-1];
	w->thlist = Z_Calloc(sizeof(thinker_t) * NUM_THINKERLISTS, PU_STATIC, NULL);
	P_InitCachedActions(w);
	return w;
}

//
// Initializes a new world, inserting it into the world list.
//
world_t *World_PushNew(INT16 mapnum)
{
	worldlist = Z_Realloc(worldlist, (numworlds + 1) * sizeof(void *), PU_STATIC, NULL);
	worldlist[numworlds] = World_Create(mapnum);

	world_t *w = worldlist[numworlds];

	if (!numworlds)
		baseworld = w;

	numworlds++;
	return w;
}

//
// Sets the current world structures for physics simulation.
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

	tags_sectors = world->tags_sectors;
	tags_lines = world->tags_lines;
	tags_mapthings = world->tags_mapthings;
}

//
// Selects the world for rendering.
//
void P_SetViewWorld(world_t *w)
{
	viewworld = w;
}

//
// Sets the current world.
//
void P_SetWorld(world_t *w)
{
	if (w == NULL)
		return;

	P_SetGameWorld(w);

	worldmapheader = w->header;

	thlist = w->thlist;
	gamemap = w->gamemap;
}

//
// Gets a player's current world.
//
world_t *P_GetPlayerWorld(player_t *player)
{
	return (world_t *)player->world;
}

//
// Detaches a world from a player.
//
void P_DetachPlayerWorld(player_t *player)
{
	world_t *w = P_GetPlayerWorld(player);
	if (w != NULL)
		w->players--;

	player->world = NULL;

	if (player->mo && !P_MobjWasRemoved(player->mo) && player->mo->world != NULL)
	{
		R_RemoveMobjInterpolator(player->mo);
		player->mo->world = NULL;
	}
}

//
// Switches a player between worlds.
//
void P_SwitchPlayerWorld(player_t *player, world_t *newworld)
{
	P_DetachPlayerWorld(player);

	player->world = newworld;

	if (player->mo && !P_MobjWasRemoved(player->mo) && player->mo->world == NULL)
	{
		player->mo->world = newworld;
		R_AddMobjInterpolator(player->mo);
	}

	newworld->players++;
}

//
// Loads a new world, or switches to one.
//
void P_RoamIntoWorld(player_t *player, INT32 mapnum)
{
	world_t *w = NULL;
	INT32 i;

	for (i = 0; i < numworlds; i++)
	{
		if (worldlist[i]->gamemap == mapnum)
		{
			w = worldlist[i];
			break;
		}
	}

	if (w == P_GetPlayerWorld(player))
		return;
	else if (w)
		P_SwitchWorld(player, w, NULL);
	else
		D_AddWorld(mapnum);
}

boolean P_TransferCarriedPlayers(player_t *player, world_t *w)
{
	boolean anycarried = false;

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		player_t *carry = &players[i];
		if (carry == player)
			continue;

		if (carry->powers[pw_carry] == CR_PLAYER
		&& carry->mo->tracer && !P_MobjWasRemoved(carry->mo->tracer)
		&& carry->mo->tracer == player->mo)
		{
			mobj_t *tails = player->mo;

			location_t location;
			location.x = tails->x;
			location.y = tails->y;
			if (carry->mo->eflags & MFE_VERTICALFLIP)
				location.z = tails->z + tails->height + 12*carry->mo->scale;
			else
				location.z = tails->z - carry->mo->height - 12*carry->mo->scale;
			location.angle = tails->angle;

			P_SwitchWorld(carry, w, &location);

			tails->z += tails->height*3*P_MobjFlip(tails);

			anycarried = true;
		}
	}

	return anycarried;
}

void P_MovePlayerToLocation(player_t *player, location_t *location)
{
	mobj_t *mobj = player->mo;
	I_Assert(mobj != NULL);

	mobj->x = location->x;
	mobj->y = location->y;
	P_SetThingPosition(mobj);

	sector_t *sector = R_PointInWorldSubsector(location->world ? location->world : mobj->world, mobj->x, mobj->y)->sector;
	fixed_t floor = P_GetSectorFloorZAt(sector, mobj->x, mobj->y);
	fixed_t ceiling = P_GetSectorCeilingZAt(sector, mobj->x, mobj->y);

	fixed_t z = location->z;
	if (z <= floor)
	{
		mobj->eflags |= MFE_ONGROUND;
		z = floor;
	}

	mobj->floorz = floor;
	mobj->ceilingz = ceiling;

	mobj->z = z;

	mobj->angle = location->angle;

	P_AfterPlayerSpawn((size_t)(player - players));
}

//
// Switches a player to a world.
//
void P_SwitchWorld(player_t *player, world_t *w, location_t *location)
{
	size_t playernum = (size_t)(player - players);
	boolean local = (INT32)playernum == consoleplayer;
	boolean resetplayer = player->powers[pw_carry] != CR_PLAYER;

	if (!playeringame[playernum] || !player->mo || P_MobjWasRemoved(player->mo))
		return;

	world_t *curworld = world;

	P_MapStart();

	if (P_GetPlayerWorld(player))
		P_RemoveMobjConnections(player->mo, P_GetPlayerWorld(player));

	if (player->followmobj)
	{
		P_RemoveMobj(player->followmobj);
		P_SetTarget(&player->followmobj, NULL);
	}

	P_SwitchPlayerWorld(player, w);

	P_SetWorld(w);

	if (local || splitscreen)
		P_InitLocalSpecials();

	P_UnsetThingPosition(player->mo);
	P_MoveThinkerToWorld(w, THINK_MOBJ, (thinker_t *)(player->mo));

	if (!location)
		G_MovePlayerToSpawnOrStarpost(playernum);
	else
	{
		location->world = w;
		P_MovePlayerToLocation(player, location);
	}

	if (local && !splitscreen)
		localworld = world;

	P_SetupSkyTexture(w->skynum);

	if (player == &players[displayplayer])
		P_ResetCamera(player, (splitscreen && playernum == 1) ? &camera2 : &camera);

	if (P_TransferCarriedPlayers(player, w))
		resetplayer = false;

	if (resetplayer)
		P_ResetPlayer(player);

	P_MapEnd();

	if (local || splitscreen)
	{
		S_SetMapMusic(worldmapheader);
		S_StopMusic();
		S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);
	}

	// Ensure consistency
	if (!local)
		P_SetWorld(curworld);
}

void Command_Switchworld_f(void)
{
	if (COM_Argc() < 2)
		return;

	INT32 worldnum = (INT32)atoi(COM_Argv(1));
	if (worldnum < 0 || worldnum >= numworlds)
		return;

	CONS_Printf("Switching to world %d\n", worldnum);

	player_t *player = &players[consoleplayer];
	world_t *w = worldlist[worldnum];
	location_t *coords_to_move_to = NULL;

	// where u at
	location_t location;
	location.x = player->mo->x;
	location.y = player->mo->y;
	location.z = player->mo->z;
	location.angle = player->mo->angle;

	boolean use_args = false;

	INT32 i = COM_CheckParm("-x");
	if (i)
	{
		location.x = atoi(COM_Argv(i + 1))<<FRACBITS;
		use_args = true;
	}

	i = COM_CheckParm("-y");
	if (i)
	{
		location.y = atoi(COM_Argv(i + 1))<<FRACBITS;
		use_args = true;
	}

	subsector_t *ss = R_PointInWorldSubsector(player->world, location.x, location.y);
	i = COM_CheckParm("-z");
	if (i)
	{
		location.z = atoi(COM_Argv(i + 1))<<FRACBITS;
		use_args = true;
	}
	else if (use_args)
		location.z = (player->mo->eflags & MFE_VERTICALFLIP) ? ss->sector->ceilingheight : ss->sector->floorheight;

	i = COM_CheckParm("-ang");
	if (i)
		location.angle = FixedAngle(atoi(COM_Argv(i + 1))<<FRACBITS);

	if (use_args)
		coords_to_move_to = &location;

	if (netgame)
		SendWorldSwitch(worldnum, coords_to_move_to, false);
	else
		P_SwitchWorld(&players[consoleplayer], w, coords_to_move_to);
}

void Command_Listworlds_f(void)
{
    for (INT32 i = 0; i < numworlds; i++)
	{
		world_t *w = worldlist[i];
		CONS_Printf("World %d\n", i);
		CONS_Printf("  Gamemap: %d\n", w->gamemap);
		CONS_Printf("  Player count: %d\n", w->players);
	}
}

//
// Unloads sector attachments.
//
static void P_UnloadSectorAttachments(sector_t *s, size_t ns)
{
	sector_t *ss;

	for (ss = s; s+ns != ss; ss++)
	{
		Z_Free(ss->attached);
		Z_Free(ss->attachedsolid);
	}
}

//
// Unloads a world.
//
void World_Delete(world_t *w)
{
	if (w == NULL)
		return;

	LUA_InvalidateLevel(w);
	P_UnloadSectorAttachments(w->sectors, w->numsectors);

	if (w->extrasubsectors)
	{
		HWR_FreeExtraSubsectors(w->extrasubsectors);
		w->extrasubsectors = NULL;
	}

	if (w->sky_dome)
		HWR_FreeSkyDome(w->sky_dome);

	Z_Free(w->thlist);
	Z_Free(w);

	// TODO: Actually free world data here!
}

//
// Unloads all worlds.
//
void World_UnloadAll(void)
{
	INT32 i;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			player_t *player = &players[i];

			player->world = NULL;

			if (player->mo && !P_MobjWasRemoved(player->mo))
				player->mo->world = NULL;
		}
	}

	for (i = 0; i < numworlds; i++)
	{
		world_t *w = worldlist[i];
		if (w == NULL)
			continue;

		World_Delete(w);
	}

	Z_Free(worldlist);

	worldlist = NULL;
	numworlds = 0;

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

	world = localworld = viewworld = NULL;
}

boolean P_MobjIsConnected(mobj_t *mobj1, mobj_t *mobj2)
{
	return mobj2 && !P_MobjWasRemoved(mobj2) && P_GetMobjWorld(mobj1) == P_GetMobjWorld(mobj2);
}

void P_RemoveMobjConnections(mobj_t *mobj, world_t *w)
{
	thinker_t *th;
	mobj_t *check;

	for (th = w->thlist[THINK_MOBJ].next; th != &w->thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		check = (mobj_t *)th;

		if (check->target == mobj)
			P_SetTarget(&mobj->target, NULL);
		if (check->tracer == mobj)
			P_SetTarget(&mobj->tracer, NULL);
		if (check->hnext == mobj)
			P_SetTarget(&mobj->hnext, NULL);
		if (check->hprev == mobj)
			P_SetTarget(&mobj->hprev, NULL);
	}
}

world_t *P_GetMobjWorld(mobj_t *mobj)
{
	return (world_t *)mobj->world;
}

void P_InitCachedActions(world_t *w)
{
	actioncache_t *head = &w->actioncachehead;

	memset(head, 0x00, sizeof(actioncache_t));

	w->actioncachehead.prev = w->actioncachehead.next = head;
}

void P_RunCachedActions(world_t *w)
{
	actioncache_t *ac;
	actioncache_t *next;

	for (ac = w->actioncachehead.next; ac != &w->actioncachehead; ac = next)
	{
		var1 = states[ac->statenum].var1;
		var2 = states[ac->statenum].var2;
		astate = &states[ac->statenum];
		if (ac->mobj && !P_MobjWasRemoved(ac->mobj)) // just in case...
			states[ac->statenum].action.acp1(ac->mobj);
		next = ac->next;
		Z_Free(ac);
	}
}

void P_AddCachedAction(world_t *w, mobj_t *mobj, INT32 statenum)
{
	actioncache_t *newaction = Z_Calloc(sizeof(actioncache_t), PU_LEVEL, NULL);
	newaction->mobj = mobj;
	newaction->statenum = statenum;
	w->actioncachehead.prev->next = newaction;
	newaction->next = &w->actioncachehead;
	newaction->prev = w->actioncachehead.prev;
	w->actioncachehead.prev = newaction;
}
