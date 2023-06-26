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

	thlist = w->thlist;
	gamemap = w->gamemap;
	gravity = w->gravity;
}

//
// Detaches a world from a player.
//
void P_DetachPlayerWorld(player_t *player)
{
	if (player->world != NULL)
		((world_t *)player->world)->players--;

	player->world = NULL;
	if (player->mo && !P_MobjWasRemoved(player->mo))
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
	if (player->mo && !P_MobjWasRemoved(player->mo))
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

	if (w == player->world)
		return;
	else if (w)
		P_SwitchWorld(player, w);
	else
		D_MapChange(mapnum, gametype, true, false, false, 0, false, false);
}

boolean P_TransferCarriedPlayers(player_t *player, world_t *w)
{
	boolean anycarried = false;
	INT32 i;

	// Lactozilla: Transfer carried players
	for (i = 0; i < MAXPLAYERS; i++)
	{
		player_t *carry;

		if (!playeringame[i])
			continue;

		carry = &players[i];
		if (carry == player)
			continue;

		if (carry->powers[pw_carry] == CR_PLAYER
		&& carry->mo->tracer && !P_MobjWasRemoved(carry->mo->tracer)
		&& carry->mo->tracer == player->mo)
		{
			mobj_t *tails = player->mo;

			P_SwitchWorld(carry, w);

			tails->z += tails->height*3*P_MobjFlip(tails);

			// Set player position
			P_UnsetThingPosition(carry->mo);
			carry->mo->x = tails->x;
			carry->mo->y = tails->y;
			if (carry->mo->eflags & MFE_VERTICALFLIP)
				carry->mo->z = tails->z + tails->height + 12*carry->mo->scale;
			else
				carry->mo->z = tails->z - carry->mo->height - 12*carry->mo->scale;
			P_SetThingPosition(carry->mo);

			anycarried = true;
		}
	}

	return anycarried;
}

//
// Switches a player to a world.
//
void P_SwitchWorld(player_t *player, world_t *w)
{
	size_t playernum = (size_t)(player - players);
	boolean local = ((INT32)playernum == consoleplayer);
	boolean resetplayer = (player->powers[pw_carry] != CR_PLAYER);

	if (!playeringame[playernum] || !player->mo || P_MobjWasRemoved(player->mo))
		return;

	if (player->world)
		P_RemoveMobjConnections(player->mo, player->world);

	if (player->followmobj)
	{
		P_RemoveMobj(player->followmobj);
		P_SetTarget(&player->followmobj, NULL);
	}

	P_SwitchPlayerWorld(player, w);

	P_SetWorld(w);
	if (local || splitscreen)
		P_InitSpecials();

	P_UnsetThingPosition(player->mo);
	P_MoveThinkerToWorld(w, THINK_MOBJ, (thinker_t *)(player->mo));
	G_MovePlayerToSpawnOrStarpost(playernum);

	if (local && !splitscreen)
	{
		localworld = world;
		S_Start();
		P_SetupSkyTexture(w->skynum);
	}

	if (player == &players[displayplayer])
		P_ResetCamera(player, (splitscreen && playernum == 1) ? &camera2 : &camera);

	if (P_TransferCarriedPlayers(player, w))
		resetplayer = false;

	if (resetplayer)
		P_ResetPlayer(player);
	P_MapEnd();

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_LoadLevel();
#endif
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

	if (netgame)
		SendWorldSwitch(worldnum, false);
	else
		P_SwitchWorld(&players[consoleplayer], w);
}

void Command_Listworlds_f(void)
{
	INT32 worldnum;
	world_t *w;

    for (worldnum = 0; worldnum < numworlds; worldnum++)
	{
		w = worldlist[worldnum];

		CONS_Printf("World %d (%p)\n", worldnum, w);
		CONS_Printf("Gamemap: %d\n", w->gamemap);
		CONS_Printf("vt %d sg %d sc %d ss %d nd %d ld %d sd %d mt %d\n",
			w->numvertexes, w->numsegs, w->numsectors, w->numsubsectors, w->numnodes, w->numlines, w->numsides, w->nummapthings);
		CONS_Printf("Player count: %d\n", w->players);
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
void P_UnloadWorld(world_t *w)
{
	if (w == NULL)
		return;

	LUA_InvalidateLevel(w);
	P_UnloadSectorAttachments(w->sectors, w->numsectors);

	if (world->extrasubsectors)
	{
		HWR_FreeExtraSubsectors(world->extrasubsectors);
		world->extrasubsectors = NULL;
	}

	if (world->sky_dome)
		HWR_FreeSkyDome(world->sky_dome);
}

//
// Unloads all worlds.
//
void P_UnloadWorldList(void)
{
	INT32 i;

	if (numworlds > 1)
		P_UnloadSectorAttachments(sectors, numsectors);
	else
	{
		for (i = 0; i < numworlds; i++)
		{
			if (worldlist[i])
				P_UnloadWorld(worldlist[i]);
		}
	}

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

	if (numworlds > 1)
	{
		for (i = 0; i < numworlds; i++)
		{
			world_t *w = worldlist[i];

			if (w == NULL)
				continue;

			Z_Free(w->thlist);
			Z_Free(w);
		}

		Z_Free(worldlist);
	}

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

//
// Unloads a player.
//
void P_UnloadWorldPlayer(player_t *player)
{
	P_DetachPlayerWorld(player);

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

boolean P_MobjIsConnected(mobj_t *mobj1, mobj_t *mobj2)
{
	return (mobj2 && !P_MobjWasRemoved(mobj2) && mobj1->world == mobj2->world);
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
