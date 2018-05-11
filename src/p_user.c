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
/// \file  p_user.c
/// \brief New stuff?
///        Player related stuff.
///        Bobbing POV/weapon, movement.
///        Pending weapon.

#include "doomdef.h"
#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "s_sound.h"
#include "r_things.h"
#include "d_think.h"
#include "r_sky.h"
#include "p_setup.h"
#include "m_random.h"
#include "m_misc.h"
#include "i_video.h"
#include "p_slopes.h"
#include "p_spec.h"
#include "r_splats.h"
#include "z_zone.h"
#include "w_wad.h"
#include "hu_stuff.h"
// We need to affect the NiGHTS hud
#include "st_stuff.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "b_bot.h"
// Objectplace
#include "m_cheat.h"

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_light.h"
#include "hardware/hw_main.h"
#endif

#if 0
static void P_NukeAllPlayers(player_t *player);
#endif

//
// Movement.
//

// 16 pixels of bob
#define MAXBOB (0x10 << FRACBITS)

static boolean onground;

//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, FINECOSINE(angle));

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)))
		mo->momy += FixedMul(move, FINESINE(angle));
}

#if 0
static inline void P_ThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, FINECOSINE(angle));
	mo->momy += FixedMul(move, FINESINE(angle));
}

static inline void P_VectorInstaThrust(fixed_t xa, fixed_t xb, fixed_t xc, fixed_t ya, fixed_t yb, fixed_t yc,
	fixed_t za, fixed_t zb, fixed_t zc, fixed_t momentum, mobj_t *mo)
{
	fixed_t a1, b1, c1, a2, b2, c2, i, j, k;

	a1 = xb - xa;
	b1 = yb - ya;
	c1 = zb - za;
	a2 = xb - xc;
	b2 = yb - yc;
	c2 = zb - zc;
/*
	// Convert to unit vectors...
	a1 = FixedDiv(a1,FixedSqrt(FixedMul(a1,a1) + FixedMul(b1,b1) + FixedMul(c1,c1)));
	b1 = FixedDiv(b1,FixedSqrt(FixedMul(a1,a1) + FixedMul(b1,b1) + FixedMul(c1,c1)));
	c1 = FixedDiv(c1,FixedSqrt(FixedMul(c1,c1) + FixedMul(c1,c1) + FixedMul(c1,c1)));

	a2 = FixedDiv(a2,FixedSqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
	b2 = FixedDiv(b2,FixedSqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
	c2 = FixedDiv(c2,FixedSqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
*/
	// Calculate the momx, momy, and momz
	i = FixedMul(momentum, FixedMul(b1, c2) - FixedMul(c1, b2));
	j = FixedMul(momentum, FixedMul(c1, a2) - FixedMul(a1, c2));
	k = FixedMul(momentum, FixedMul(a1, b2) - FixedMul(a1, c2));

	mo->momx = i;
	mo->momy = j;
	mo->momz = k;
}
#endif

//
// P_InstaThrust
// Moves the given origin along a given angle instantly.
//
// FIXTHIS: belongs in another file, not here
//
void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, FINECOSINE(angle));

	if (!(twodlevel || (mo->flags2 & MF2_TWOD)))
		mo->momy = FixedMul(move,FINESINE(angle));
}

void P_InstaThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, FINECOSINE(angle));
	mo->momy = FixedMul(move, FINESINE(angle));
}

// Returns a location (hard to explain - go see how it is used)
fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move)
{
	(void)mo;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, FINECOSINE(angle));
}
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move)
{
	(void)mo;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, FINESINE(angle));
}

//
// P_AutoPause
// Returns true when gameplay should be halted even if the game isn't necessarily paused.
//
boolean P_AutoPause(void)
{
	// Don't pause even on menu-up or focus-lost in netgames or record attack
	if (netgame || modeattacking)
		return false;

	return (menuactive || window_notinfocus);
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight(player_t *player)
{
	INT32 angle;
	fixed_t bob;
	fixed_t pviewheight;
	mobj_t *mo = player->mo;

	// Regular movement bobbing.
	// Should not be calculated when not on ground (FIXTHIS?)
	// OPTIMIZE: tablify angle
	// Note: a LUT allows for effects
	//  like a ramp with low health.

	player->bob = (FixedMul(player->rmomx,player->rmomx)
		+ FixedMul(player->rmomy,player->rmomy))>>2;

	if (player->bob > FixedMul(MAXBOB, mo->scale))
		player->bob = FixedMul(MAXBOB, mo->scale);

	if (!P_IsObjectOnGround(mo))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			player->viewz = mo->z + mo->height - player->viewheight;
			if (player->viewz < mo->floorz + FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->floorz + FixedMul(FRACUNIT, mo->scale);
		}
		else
		{
			player->viewz = mo->z + player->viewheight;
			if (player->viewz > mo->ceilingz - FixedMul(FRACUNIT, mo->scale))
				player->viewz = mo->ceilingz - FixedMul(FRACUNIT, mo->scale);
		}
		return;
	}

	angle = (FINEANGLES/20*localgametic)&FINEMASK;
	bob = FixedMul(player->bob/2, FINESINE(angle));

	// move viewheight
	pviewheight = FixedMul(cv_viewheight.value << FRACBITS, mo->scale); // default eye view height

	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > pviewheight)
		{
			player->viewheight = pviewheight;
			player->deltaviewheight = 0;
		}

		if (player->viewheight < pviewheight/2)
		{
			player->viewheight = pviewheight/2;
			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 1;
		}

		if (player->deltaviewheight)
		{
			player->deltaviewheight += FixedMul(FRACUNIT/4, mo->scale);
			if (!player->deltaviewheight)
				player->deltaviewheight = 1;
		}
	}

	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->viewz = mo->z + mo->height - player->viewheight - bob;
	else
		player->viewz = mo->z + player->viewheight + bob;

	if (player->viewz > mo->ceilingz-FixedMul(4*FRACUNIT, mo->scale))
		player->viewz = mo->ceilingz-FixedMul(4*FRACUNIT, mo->scale);
	if (player->viewz < mo->floorz+FixedMul(4*FRACUNIT, mo->scale))
		player->viewz = mo->floorz+FixedMul(4*FRACUNIT, mo->scale);
}

/** Decides if a player is moving.
  * \param pnum The player number to test.
  * \return True if the player is considered to be moving.
  * \author Graue <graue@oceanbase.org>
  */
boolean P_PlayerMoving(INT32 pnum)
{
	player_t *p = &players[pnum];

	if (!Playing())
		return false;

	if (p->jointime < 5*TICRATE || p->playerstate == PST_DEAD || p->playerstate == PST_REBORN || p->spectator)
		return false;

	return gamestate == GS_LEVEL && p->mo && p->mo->health > 0
		&& (abs(p->rmomx) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| abs(p->rmomy) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| abs(p->mo->momz) >= FixedMul(FRACUNIT/2, p->mo->scale)
			|| p->climbing || p->powers[pw_tailsfly]
			|| (p->pflags & PF_JUMPED) || (p->pflags & PF_SPINNING));
}

// P_GetNextEmerald
//
// Gets the number (0 based) of the next emerald to obtain
//
UINT8 P_GetNextEmerald(void)
{
	if (!useNightsSS) // In order
	{
		if (!(emeralds & EMERALD1)) return 0;
		if (!(emeralds & EMERALD2)) return 1;
		if (!(emeralds & EMERALD3)) return 2;
		if (!(emeralds & EMERALD4)) return 3;
		if (!(emeralds & EMERALD5)) return 4;
		if (!(emeralds & EMERALD6)) return 5;
		return 6;
	}
	else // Depends on stage
	{
		if (gamemap < sstage_start || gamemap > sstage_end)
			return 0;
		return (UINT8)(gamemap - sstage_start);
	}
}

//
// P_GiveEmerald
//
// Award an emerald upon completion
// of a special stage.
//
void P_GiveEmerald(boolean spawnObj)
{
	INT32 i;
	UINT8 em;

	S_StartSound(NULL, sfx_cgot); // Got the emerald!
	em = P_GetNextEmerald();
	emeralds |= (1 << em);

	if (spawnObj)
	{
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				P_SetMobjState(P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD),
				mobjinfo[MT_GOTEMERALD].spawnstate + em);

	}
}

//
// P_ResetScore
//
// This is called when your chain is reset.
void P_ResetScore(player_t *player)
{
	// Formally a host for Chaos mode behavior

	player->scoreadd = 0;
}

//
// P_FindLowestMare
//
// Returns the lowest open mare available
//
UINT8 P_FindLowestMare(void)
{
	thinker_t *th;
	mobj_t *mo2;
	UINT8 mare = UINT8_MAX;

	if (gametype == GT_RACE || gametype == GT_COMPETITION)
		return 0;

	// scan the thinkers
	// to find the egg capsule with the lowest mare
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_EGGCAPSULE && mo2->health > 0)
		{
			const UINT8 threshold = (UINT8)mo2->threshold;
			if (mare == 255)
				mare = threshold;
			else if (threshold < mare)
				mare = threshold;
		}
	}

	CONS_Debug(DBG_NIGHTS, "Lowest mare found: %d\n", mare);

	return mare;
}

//
// P_TransferToNextMare
//
// Transfers the player to the next Mare.
// (Finds the lowest mare # for capsules that have not been destroyed).
// Returns true if successful, false if there is no other mare.
//
boolean P_TransferToNextMare(player_t *player)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	INT32 lowestaxisnum = -1;
	UINT8 mare = P_FindLowestMare();
	fixed_t dist1, dist2 = 0;

	if (mare == 255)
		return false;

	CONS_Debug(DBG_NIGHTS, "Mare is %d\n", mare);

	player->mare = mare;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_AXIS)
		{
			if (mo2->threshold == mare)
			{
				if (closestaxis == NULL)
				{
					closestaxis = mo2;
					lowestaxisnum = mo2->health;
					dist2 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;
				}
				else if (mo2->health < lowestaxisnum)
				{
					dist1 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;

					if (dist1 < dist2)
					{
						closestaxis = mo2;
						lowestaxisnum = mo2->health;
						dist2 = dist1;
					}
				}
			}
		}
	}

	if (closestaxis == NULL)
		return false;

	P_SetTarget(&player->mo->target, closestaxis);
	return true;
}

//
// P_FindAxis
//
// Given a mare and axis number, returns
// the mobj for that axis point.
static mobj_t *P_FindAxis(INT32 mare, INT32 axisnum)
{
	thinker_t *th;
	mobj_t *mo2;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		// Axis things are only at beginning of list.
		if (!(mo2->flags2 & MF2_AXIS))
			return NULL;

		if (mo2->type == MT_AXIS)
		{
			if (mo2->health == axisnum && mo2->threshold == mare)
				return mo2;
		}
	}

	return NULL;
}

//
// P_FindAxisTransfer
//
// Given a mare and axis number, returns
// the mobj for that axis transfer point.
static mobj_t *P_FindAxisTransfer(INT32 mare, INT32 axisnum, mobjtype_t type)
{
	thinker_t *th;
	mobj_t *mo2;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		// Axis things are only at beginning of list.
		if (!(mo2->flags2 & MF2_AXIS))
			return NULL;

		if (mo2->type == type)
		{
			if (mo2->health == axisnum && mo2->threshold == mare)
				return mo2;
		}
	}

	return NULL;
}

//
// P_TransferToAxis
//
// Finds the CLOSEST axis with the number specified.
void P_TransferToAxis(player_t *player, INT32 axisnum)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis;
	INT32 mare = player->mare;
	fixed_t dist1, dist2 = 0;

	CONS_Debug(DBG_NIGHTS, "Transferring to axis %d\nLeveltime: %u...\n", axisnum, leveltime);

	closestaxis = NULL;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_AXIS)
		{
			if (mo2->health == axisnum && mo2->threshold == mare)
			{
				if (closestaxis == NULL)
				{
					closestaxis = mo2;
					dist2 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;
				}
				else
				{
					dist1 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;

					if (dist1 < dist2)
					{
						closestaxis = mo2;
						dist2 = dist1;
					}
				}
			}
		}
	}

	if (!closestaxis)
	{
		CONS_Debug(DBG_NIGHTS, "ERROR: Specified axis point to transfer to not found!\n%d\n", axisnum);
	}
	else
	{
		CONS_Debug(DBG_NIGHTS, "Transferred to axis %d, mare %d\n", closestaxis->health, closestaxis->threshold);
	}

	P_SetTarget(&player->mo->target, closestaxis);
}

//
// P_DeNightserizePlayer
//
// Whoops! Ran out of NiGHTS time!
//
static void P_DeNightserizePlayer(player_t *player)
{
	thinker_t *th;
	mobj_t *mo2;

	player->pflags &= ~PF_NIGHTSMODE;

	//if (player->mo->tracer)
		//P_RemoveMobj(player->mo->tracer);

	player->powers[pw_underwater] = 0;
	player->pflags &= ~(PF_USEDOWN|PF_JUMPDOWN|PF_ATTACKDOWN|PF_STARTDASH|PF_GLIDING|PF_JUMPED|PF_THOKKED|PF_SPINNING|PF_DRILLING|PF_TRANSFERTOCLOSEST);
	player->secondjump = 0;
	player->jumping = 0;
	player->homing = 0;
	player->climbing = 0;
	player->mo->fuse = 0;
	player->speed = 0;
	P_SetTarget(&player->mo->target, NULL);
	P_SetTarget(&player->axis1, P_SetTarget(&player->axis2, NULL));

	player->mo->flags &= ~MF_NOGRAVITY;

	player->mo->flags2 &= ~MF2_DONTDRAW;

	// Restore aiming angle
	if (player == &players[consoleplayer])
		localaiming = 0;
	else if (player == &players[secondarydisplayplayer])
		localaiming2 = 0;

	// If you screwed up, kiss your score goodbye.
	player->marescore = 0;

	if (player->mo->tracer)
		P_RemoveMobj(player->mo->tracer);
	P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
	player->pflags |= PF_NIGHTSFALL;

	// If in a special stage, add some preliminary exit time.
	if (G_IsSpecialStage(gamemap))
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].pflags & PF_NIGHTSMODE)
				players[i].nightstime = 1; // force everyone else to fall too.
		player->exiting = 3*TICRATE;
		stagefailed = true; // NIGHT OVER
	}

	// Check to see if the player should be killed.
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;
		if (!(mo2->type == MT_NIGHTSDRONE))
			continue;

		if (mo2->flags & MF_AMBUSH)
			P_DamageMobj(player->mo, NULL, NULL, 10000);

		break;
	}

	// Restore from drowning music
	P_RestoreMusic(player);
}
//
// P_NightserizePlayer
//
// NiGHTS Time!
void P_NightserizePlayer(player_t *player, INT32 nighttime)
{
	INT32 oldmare;

	// Bots can't be super, silly!1 :P
	if (player->bot)
		return;

	if (!(player->pflags & PF_NIGHTSMODE))
	{
		P_SetTarget(&player->mo->tracer, P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_NIGHTSCHAR));
		player->mo->tracer->destscale = player->mo->scale;
		P_SetScale(player->mo->tracer, player->mo->scale);
		player->mo->tracer->eflags = (player->mo->tracer->eflags & ~MFE_VERTICALFLIP)|(player->mo->eflags & MFE_VERTICALFLIP);
		player->mo->height = player->mo->tracer->height;
	}

	player->pflags &= ~(PF_USEDOWN|PF_JUMPDOWN|PF_ATTACKDOWN|PF_STARTDASH|PF_GLIDING|PF_JUMPED|PF_THOKKED|PF_SPINNING|PF_DRILLING);
	player->homing = 0;
	player->mo->fuse = 0;
	player->speed = 0;
	player->climbing = 0;
	player->secondjump = 0;

	player->powers[pw_shield] = SH_NONE;

	player->mo->flags |= MF_NOGRAVITY;

	player->mo->flags2 |= MF2_DONTDRAW;

	player->nightstime = player->startedtime = nighttime*TICRATE;
	player->bonustime = false;

	P_RestoreMusic(player);
	P_SetMobjState(player->mo->tracer, S_SUPERTRANS1);

	if (gametype == GT_RACE || gametype == GT_COMPETITION)
	{
		if (player->drillmeter < 48*20)
			player->drillmeter = 48*20;
	}
	else
	{
		if (player->drillmeter < 40*20)
			player->drillmeter = 40*20;
	}

	oldmare = player->mare;

	if (P_TransferToNextMare(player) == false)
	{
		INT32 i;
		INT32 total_rings = 0;

		P_SetTarget(&player->mo->target, NULL);

		if (G_IsSpecialStage(gamemap))
		{
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i]/* && players[i].pflags & PF_NIGHTSMODE*/)
					total_rings += players[i].health-1;
		}

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || !players[i].mo || players[i].spectator)
				continue;

			players[i].texttimer = (3 * TICRATE) - 10;
			players[i].textvar = 4; // Score and grades
			players[i].lastmare = players[i].mare;
			if (G_IsSpecialStage(gamemap))
			{
				players[i].finishedrings = (INT16)total_rings;
				P_AddPlayerScore(player, total_rings * 50);
			}
			else
			{
				players[i].finishedrings = (INT16)(players[i].health - 1);
				P_AddPlayerScore(&players[i], (players[i].health - 1) * 50);
			}

			// Add score to leaderboards now
			if (!(netgame||multiplayer) && P_IsLocalPlayer(&players[i]))
				G_AddTempNightsRecords(players[i].marescore, leveltime - player->marebegunat, players[i].mare + 1);

			// transfer scores anyway
			players[i].lastmarescore = players[i].marescore;
			players[i].marescore = 0;

			players[i].mo->health = players[i].health = 1;
			P_DoPlayerExit(&players[i]);
		}
	}
	else if (oldmare != player->mare)
	{
		/// \todo Handle multi-mare special stages.
		// Ring bonus
		P_AddPlayerScore(player, (player->health - 1) * 50);

		player->lastmare = (UINT8)oldmare;
		player->texttimer = 4*TICRATE;
		player->textvar = 4; // Score and grades
		player->finishedrings = (INT16)(player->health - 1);

		// Add score to temp leaderboards
		if (!(netgame||multiplayer) && P_IsLocalPlayer(player))
			G_AddTempNightsRecords(player->marescore, leveltime - player->marebegunat, (UINT8)(oldmare + 1));

		// Starting a new mare, transfer scores
		player->lastmarescore = player->marescore;
		player->marescore = 0;
		player->marebegunat = leveltime;

		player->mo->health = player->health = 1;
	}
	else
	{
		player->textvar = 5; // Nothing, just tells it to go to the GET n RINGS/SPHERES text in a bit
		player->texttimer = 40;

		// Don't show before title card
		// Not consistency safe, but this only affects drawing
		if (timeinmap + 40 < 110)
			player->texttimer = (UINT8)(110 - timeinmap);
	}

	player->pflags |= PF_NIGHTSMODE;
}

//
// P_PlayerInPain
//
// Is player in pain??
// Checks for painstate and pw_flashing, if both found return true
//
boolean P_PlayerInPain(player_t *player)
{
	// no silly, sliding isn't pain
	if (!(player->pflags & PF_SLIDING) && player->mo->state == &states[player->mo->info->painstate] && player->powers[pw_flashing])
		return true;

	return false;
}

//
// P_DoPlayerPain
//
// Player was hit,
// put them in pain.
//
void P_DoPlayerPain(player_t *player, mobj_t *source, mobj_t *inflictor)
{
	angle_t ang;
	fixed_t fallbackspeed;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->mo->z--;
	else
		player->mo->z++;

	if (player->mo->eflags & MFE_UNDERWATER)
		P_SetObjectMomZ(player->mo, FixedDiv(10511*FRACUNIT,2600*FRACUNIT), false);
	else
		P_SetObjectMomZ(player->mo, FixedDiv(69*FRACUNIT,10*FRACUNIT), false);

	if (inflictor)
	{
		ang = R_PointToAngle2(inflictor->x-inflictor->momx, inflictor->y - inflictor->momy, player->mo->x - player->mo->momx, player->mo->y - player->mo->momy);

		// explosion and rail rings send you farther back, making it more difficult
		// to recover
		if ((inflictor->flags2 & MF2_SCATTER) && source)
		{
			fixed_t dist = P_AproxDistance(P_AproxDistance(source->x-player->mo->x, source->y-player->mo->y), source->z-player->mo->z);

			dist = FixedMul(128*FRACUNIT, inflictor->scale) - dist/4;

			if (dist < FixedMul(4*FRACUNIT, inflictor->scale))
				dist = FixedMul(4*FRACUNIT, inflictor->scale);

			fallbackspeed = dist;
		}
		else if (inflictor->flags2 & MF2_EXPLOSION)
		{
			if (inflictor->flags2 & MF2_RAILRING)
				fallbackspeed = FixedMul(38*FRACUNIT, inflictor->scale); // 7x
			else
				fallbackspeed = FixedMul(30*FRACUNIT, inflictor->scale); // 5x
		}
		else if (inflictor->flags2 & MF2_RAILRING)
			fallbackspeed = FixedMul(45*FRACUNIT, inflictor->scale); // 4x
		else
			fallbackspeed = FixedMul(4*FRACUNIT, inflictor->scale); // the usual amount of force
	}
	else
	{
		ang = R_PointToAngle2(player->mo->momx, player->mo->momy, 0, 0);
		fallbackspeed = FixedMul(4*FRACUNIT, player->mo->scale);
	}

	P_InstaThrust(player->mo, ang, fallbackspeed);

	if (player->pflags & PF_ROPEHANG)
		P_SetTarget(&player->mo->tracer, NULL);

	// Point penalty for hitting a hazard during tag.
	// Discourages players from intentionally hurting themselves to avoid being tagged.
	if (gametype == GT_TAG && (!(player->pflags & PF_TAGGED) && !(player->pflags & PF_TAGIT)))
	{
		if (player->score >= 50)
			player->score -= 50;
		else
			player->score = 0;
	}

	P_ResetPlayer(player);
	P_SetPlayerMobjState(player->mo, player->mo->info->painstate);
	player->powers[pw_flashing] = flashingtics;

	if (player->timeshit != UINT8_MAX)
		++player->timeshit;
}

//
// P_ResetPlayer
//
// Useful when you want to kill everything the player is doing.
void P_ResetPlayer(player_t *player)
{
	player->pflags &= ~(PF_ROPEHANG|PF_ITEMHANG|PF_MACESPIN|PF_SPINNING|PF_JUMPED|PF_GLIDING|PF_THOKKED|PF_CARRIED);
	player->jumping = 0;
	player->secondjump = 0;
	player->glidetime = 0;
	player->homing = 0;
	player->climbing = 0;
	player->powers[pw_tailsfly] = 0;
	player->onconveyor = 0;
	player->skidtime = 0;
	if (player-players == consoleplayer && botingame)
		CV_SetValue(&cv_analog2, true);
}

//
// P_GivePlayerRings
//
// Gives rings to the player, and does any special things required.
// Call this function when you want to increment the player's health.
//
void P_GivePlayerRings(player_t *player, INT32 num_rings)
{
	if (player->bot)
		player = &players[consoleplayer];

	if (!player->mo)
		return;

	player->mo->health += num_rings;
	player->health += num_rings;

	if (!G_IsSpecialStage(gamemap) || !useNightsSS)
		player->totalring += num_rings;

	// Can only get up to 9999 rings, sorry!
	if (player->mo->health > 10000)
	{
		player->mo->health = 10000;
		player->health = 10000;
	}
	else if (player->mo->health < 1)
	{
		player->mo->health = 1;
		player->health = 1;
	}

	// Now extra life bonuses are handled here instead of in P_MovePlayer, since why not?
	if (!ultimatemode && !modeattacking && !G_IsSpecialStage(gamemap) && G_GametypeUsesLives())
	{
		INT32 gainlives = 0;

		while (player->xtralife < maxXtraLife && player->health > 100 * (player->xtralife+1))
		{
			++gainlives;
			++player->xtralife;
		}

		if (gainlives)
		{
			P_GivePlayerLives(player, gainlives);
			P_PlayLivesJingle(player);
		}
	}
}

//
// P_GivePlayerLives
//
// Gives the player an extra life.
// Call this function when you want to add lives to the player.
//
void P_GivePlayerLives(player_t *player, INT32 numlives)
{
	player->lives += numlives;

	if (player->lives > 99)
		player->lives = 99;
	else if (player->lives < 1)
		player->lives = 1;
}

//
// P_DoSuperTransformation
//
// Transform into Super Sonic!
void P_DoSuperTransformation(player_t *player, boolean giverings)
{
	player->powers[pw_super] = 1;
	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC) && P_IsLocalPlayer(player))
	{
		S_StopMusic();
		S_ChangeMusicInternal("supers", true);
	}

	S_StartSound(NULL, sfx_supert); //let all players hear it -mattw_cfi

	// Transformation animation
	P_SetPlayerMobjState(player->mo, S_PLAY_SUPERTRANS1);

	player->mo->momx = player->mo->momy = player->mo->momz = 0;

	if (giverings)
	{
		player->mo->health = 51;
		player->health = player->mo->health;
	}

	// Just in case.
	if (!(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC))
	{
		player->powers[pw_extralife] = 0;
		player->powers[pw_invulnerability] = 0;
		player->powers[pw_sneakers] = 0;
	}

	if (gametype != GT_COOP)
	{
		HU_SetCEchoFlags(0);
		HU_SetCEchoDuration(5);
		HU_DoCEcho(va("%s\\is now super.\\\\\\\\", player_names[player-players]));
	}

	P_PlayerFlagBurst(player, false);
}
// Adds to the player's score
void P_AddPlayerScore(player_t *player, UINT32 amount)
{
	UINT32 oldscore;

	if (player->bot)
		player = &players[consoleplayer];

	// NiGHTS does it different!
	if (gamestate == GS_LEVEL && mapheaderinfo[gamemap-1]->typeoflevel & TOL_NIGHTS)
	{
		if ((netgame || multiplayer) && G_IsSpecialStage(gamemap))
		{ // Pseudo-shared score for multiplayer special stages.
			INT32 i;
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && players[i].pflags & PF_NIGHTSMODE)
				{
					oldscore = players[i].marescore;

					// Don't go above MAXSCORE.
					if (players[i].marescore + amount < MAXSCORE)
						players[i].marescore += amount;
					else
						players[i].marescore = MAXSCORE;

					// Continues are worthless in netgames.
					// If that stops being the case uncomment this.
/*					if (!ultimatemode && players[i].marescore > 50000
					&& oldscore < 50000)
					{
						players[i].continues += 1;
						players[i].gotcontinue = true;
						if (P_IsLocalPlayer(player))
							S_StartSound(NULL, sfx_flgcap);
					} */
				}
		}
		else
		{
			oldscore = player->marescore;

			// Don't go above MAXSCORE.
			if (player->marescore + amount < MAXSCORE)
				player->marescore += amount;
			else
				player->marescore = MAXSCORE;

			if (!ultimatemode && !(netgame || multiplayer) && G_IsSpecialStage(gamemap)
			&& player->marescore >= 50000 && oldscore < 50000)
			{
				player->continues += 1;
				player->gotcontinue = true;
				if (P_IsLocalPlayer(player))
					S_StartSound(NULL, sfx_flgcap);
			}
		}

		if (gametype == GT_COOP)
			return;
	}

	oldscore = player->score;

	// Don't go above MAXSCORE.
	if (player->score + amount < MAXSCORE)
		player->score += amount;
	else
		player->score = MAXSCORE;

	// check for extra lives every 50000 pts
	if (!ultimatemode && !modeattacking && player->score > oldscore && player->score % 50000 < amount && (gametype == GT_COMPETITION || gametype == GT_COOP))
	{
		P_GivePlayerLives(player, (player->score/50000) - (oldscore/50000));
		P_PlayLivesJingle(player);
	}

	// In team match, all awarded points are incremented to the team's running score.
	if (gametype == GT_TEAMMATCH)
	{
		if (player->ctfteam == 1)
			redscore += amount;
		else if (player->ctfteam == 2)
			bluescore += amount;
	}
}

//
// P_PlayLivesJingle
//
void P_PlayLivesJingle(player_t *player)
{
	if (player && !P_IsLocalPlayer(player))
		return;

	if (use1upSound)
		S_StartSound(NULL, sfx_oneup);
	else if (mariomode)
		S_StartSound(NULL, sfx_marioa);
	else
	{
		if (player)
			player->powers[pw_extralife] = extralifetics + 1;
		S_StopMusic(); // otherwise it won't restart if this is done twice in a row
		S_ChangeMusicInternal("xtlife", false);
	}
}

//
// P_RestoreMusic
//
// Restores music after some special music change
//
void P_RestoreMusic(player_t *player)
{
	if (!P_IsLocalPlayer(player)) // Only applies to a local player
		return;

	if (player->powers[pw_extralife] > 1)
		return;
	S_SpeedMusic(1.0f);
	if (player->powers[pw_super] && !(mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC))
		S_ChangeMusicInternal("supers", true);
	else if (player->powers[pw_invulnerability] > 1)
		S_ChangeMusicInternal((mariomode) ? "minvnc" : "invinc", false);
	else if (player->powers[pw_sneakers] > 1 && !player->powers[pw_super])
	{
		if (mapheaderinfo[gamemap-1]->levelflags & LF_SPEEDMUSIC)
		{
			S_SpeedMusic(1.4f);
			S_ChangeMusic(mapmusname, mapmusflags, true);
		}
		else
			S_ChangeMusicInternal("shoes", true);
	}
	else
		S_ChangeMusic(mapmusname, mapmusflags, true);
}

//
// P_IsObjectInGoop
//
// Returns true if the object is inside goop water.
// (Spectators and objects otherwise without gravity cannot have goop gravity!)
//
boolean P_IsObjectInGoop(mobj_t *mo)
{
	if (mo->player && mo->player->spectator)
		return false;

	if (mo->flags & MF_NOGRAVITY)
		return false;

	return ((mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER)) == (MFE_UNDERWATER|MFE_GOOWATER));
}

//
// P_IsObjectOnGround
//
// Returns true if the player is
// on the ground. Takes reverse
// gravity and FOFs into account.
//
boolean P_IsObjectOnGround(mobj_t *mo)
{
	if (P_IsObjectInGoop(mo))
	{
/*
		// It's a crazy hack that checking if you're on the ground
		// would actually CHANGE your position and momentum,
		if (mo->z < mo->floorz)
		{
			mo->z = mo->floorz;
			mo->momz = 0;
		}
		else if (mo->z + mo->height > mo->ceilingz)
		{
			mo->z = mo->ceilingz - mo->height;
			mo->momz = 0;
		}
*/
		// but I don't want you to ever 'stand' while submerged in goo.
		// You're in constant vertical momentum, even if you get stuck on something.
		// No exceptions.
		return false;
	}

	if (mo->eflags & MFE_VERTICALFLIP)
	{
		if (mo->z+mo->height >= mo->ceilingz)
			return true;
	}
	else
	{
		if (mo->z <= mo->floorz)
			return true;
	}

	return false;
}

//
// P_IsObjectOnGroundIn
//
// Returns true if the player is
// on the ground in a specific sector. Takes reverse
// gravity and FOFs into account.
//
boolean P_IsObjectOnGroundIn(mobj_t *mo, sector_t *sec)
{
	ffloor_t *rover;

	// Is the object in reverse gravity?
	if (mo->eflags & MFE_VERTICALFLIP)
	{
		// Detect if the player is on the ceiling.
		if (mo->z+mo->height >= P_GetSpecialTopZ(mo, sec, sec))
			return true;
		// Otherwise, detect if the player is on the bottom of a FOF.
		else
		{
			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				// If the FOF doesn't exist, continue.
				if (!(rover->flags & FF_EXISTS))
					continue;

				// If the FOF is configured to let the object through, continue.
				if (!((rover->flags & FF_BLOCKPLAYER && mo->player)
					|| (rover->flags & FF_BLOCKOTHERS && !mo->player)))
					continue;

				// If the the platform is intangible from below, continue.
				if (rover->flags & FF_PLATFORM)
					continue;

				// If the FOF is a water block, continue. (Unnecessary check?)
				if (rover->flags & FF_SWIMMABLE)
					continue;

				// Actually check if the player is on the suitable FOF.
				if (mo->z+mo->height == P_GetSpecialBottomZ(mo, sectors + rover->secnum, sec))
					return true;
			}
		}
	}
	// Nope!
	else
	{
		// Detect if the player is on the floor.
		if (mo->z <= P_GetSpecialBottomZ(mo, sec, sec))
			return true;
		// Otherwise, detect if the player is on the top of a FOF.
		else
		{
			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				// If the FOF doesn't exist, continue.
				if (!(rover->flags & FF_EXISTS))
					continue;

				// If the FOF is configured to let the object through, continue.
				if (!((rover->flags & FF_BLOCKPLAYER && mo->player)
					|| (rover->flags & FF_BLOCKOTHERS && !mo->player)))
					continue;

				// If the the platform is intangible from above, continue.
				if (rover->flags & FF_REVERSEPLATFORM)
					continue;

				// If the FOF is a water block, continue. (Unnecessary check?)
				if (rover->flags & FF_SWIMMABLE)
					continue;

				// Actually check if the player is on the suitable FOF.
				if (mo->z == P_GetSpecialTopZ(mo, sectors + rover->secnum, sec))
					return true;
			}
		}
	}

	return false;
}

//
// P_SetObjectMomZ
//
// Sets the player momz appropriately.
// Takes reverse gravity into account.
//
void P_SetObjectMomZ(mobj_t *mo, fixed_t value, boolean relative)
{
	if (mo->eflags & MFE_VERTICALFLIP)
		value = -value;

	if (mo->scale != FRACUNIT)
		value = FixedMul(value, mo->scale);

	if (relative)
		mo->momz += value;
	else
		mo->momz = value;
}

//
// P_GetPlayerHeight
//
// Returns the height
// of the player.
//
fixed_t P_GetPlayerHeight(player_t *player)
{
	return FixedMul(player->mo->info->height, player->mo->scale);
}

//
// P_GetPlayerSpinHeight
//
// Returns the 'spin height'
// of the player.
//
fixed_t P_GetPlayerSpinHeight(player_t *player)
{
	return FixedMul(FixedMul(player->mo->info->height, player->mo->scale),2*FRACUNIT/3);
}

//
// P_IsLocalPlayer
//
// Returns true if player is
// on the local machine.
//
boolean P_IsLocalPlayer(player_t *player)
{
	return ((splitscreen && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer]);
}

//
// P_SpawnShieldOrb
//
// Spawns the shield orb on the player
// depending on which shield they are
// supposed to have.
//
void P_SpawnShieldOrb(player_t *player)
{
	mobjtype_t orbtype;
	thinker_t *th;
	mobj_t *shieldobj, *ov;

#ifdef PARANOIA
	if (!player->mo)
		I_Error("P_SpawnShieldOrb: player->mo is NULL!\n");
#endif

	if (player->powers[pw_shield] & SH_FORCE)
		orbtype = MT_BLUEORB;
	else switch (player->powers[pw_shield] & SH_NOSTACK)
	{
	case SH_JUMP:
		orbtype = MT_WHITEORB;
		break;
	case SH_ATTRACT:
		orbtype = MT_YELLOWORB;
		break;
	case SH_ELEMENTAL:
		orbtype = MT_GREENORB;
		break;
	case SH_BOMB:
		orbtype = MT_BLACKORB;
		break;
	case SH_PITY:
		orbtype = MT_PITYORB;
		break;
	default:
		return;
	}

	// blaze through the thinkers to see if an orb already exists!
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		shieldobj = (mobj_t *)th;

		if (shieldobj->type == orbtype && shieldobj->target == player->mo)
			P_RemoveMobj(shieldobj); //kill the old one(s)
	}

	shieldobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, orbtype);
	P_SetTarget(&shieldobj->target, player->mo);
	shieldobj->color = (UINT8)shieldobj->info->painchance;

	if (shieldobj->info->seestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		P_SetTarget(&ov->target, shieldobj);
		P_SetMobjState(ov, shieldobj->info->seestate);
	}
	if (shieldobj->info->meleestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		P_SetTarget(&ov->target, shieldobj);
		P_SetMobjState(ov, shieldobj->info->meleestate);
	}
	if (shieldobj->info->missilestate)
	{
		ov = P_SpawnMobj(shieldobj->x, shieldobj->y, shieldobj->z, MT_OVERLAY);
		P_SetTarget(&ov->target, shieldobj);
		P_SetMobjState(ov, shieldobj->info->missilestate);
	}
	if (player->powers[pw_shield] & SH_FORCE)
	{
		//Copy and pasted from P_ShieldLook in p_mobj.c
		shieldobj->movecount = (player->powers[pw_shield] & 0xFF);
		if (shieldobj->movecount < 1)
		{
			if (shieldobj->info->painstate)
				P_SetMobjState(shieldobj,shieldobj->info->painstate);
			else
				shieldobj->flags2 |= MF2_SHADOW;
		}
	}
}

//
// P_SpawnGhostMobj
//
// Spawns a ghost object on the player
//
mobj_t *P_SpawnGhostMobj(mobj_t *mobj)
{
	mobj_t *ghost;

	ghost = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_GHOST);

	P_SetScale(ghost, mobj->scale);
	ghost->destscale = mobj->scale;

	if (mobj->eflags & MFE_VERTICALFLIP)
	{
		ghost->eflags |= MFE_VERTICALFLIP;
		ghost->z += mobj->height - ghost->height;
	}

	ghost->color = mobj->color;

	ghost->angle = mobj->angle;
	ghost->sprite = mobj->sprite;
	ghost->frame = mobj->frame;
	ghost->tics = -1;
	ghost->frame &= ~FF_TRANSMASK;
	ghost->frame |= tr_trans50<<FF_TRANSSHIFT;
	ghost->fuse = ghost->info->damage;
	ghost->skin = mobj->skin;

	if (mobj->flags2 & MF2_OBJECTFLIP)
		ghost->flags |= MF2_OBJECTFLIP;

	return ghost;
}

//
// P_SpawnThokMobj
//
// Spawns the appropriate thok object on the player
//
void P_SpawnThokMobj(player_t *player)
{
	mobj_t *mobj;
	mobjtype_t type = player->thokitem;
	fixed_t zheight;

	if (player->skincolor == 0)
		return;

	if (player->spectator)
		return;

	if (type == MT_GHOST)
		mobj = P_SpawnGhostMobj(player->mo); // virtually does everything here for us
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			zheight = player->mo->z + player->mo->height + FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT) - FixedMul(mobjinfo[type].height, player->mo->scale);
		else
			zheight = player->mo->z - FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT);

		if (!(player->mo->eflags & MFE_VERTICALFLIP) && zheight < player->mo->floorz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->floorz;
		else if (player->mo->eflags & MFE_VERTICALFLIP && zheight + FixedMul(mobjinfo[type].height, player->mo->scale) > player->mo->ceilingz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->ceilingz - FixedMul(mobjinfo[type].height, player->mo->scale);

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);

		// set to player's angle, just in case
		mobj->angle = player->mo->angle;

		// color and skin
		mobj->color = player->mo->color;
		mobj->skin = player->mo->skin;

		// vertical flip
		if (player->mo->eflags & MFE_VERTICALFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->eflags |= (player->mo->eflags & MFE_VERTICALFLIP);

		// scale
		P_SetScale(mobj, player->mo->scale);
		mobj->destscale = player->mo->scale;
	}

	P_SetTarget(&mobj->target, player->mo); // the one thing P_SpawnGhostMobj doesn't do
	if (demorecording)
		G_GhostAddThok();
}

//
// P_SpawnSpinMobj
//
// Spawns the appropriate spin object on the player
//
void P_SpawnSpinMobj(player_t *player, mobjtype_t type)
{
	mobj_t *mobj;
	fixed_t zheight;

	if (player->skincolor == 0)
		return;

	if (player->spectator)
		return;

	if (type == MT_GHOST)
		mobj = P_SpawnGhostMobj(player->mo); // virtually does everything here for us
	else
	{
		if (player->mo->eflags & MFE_VERTICALFLIP)
			zheight = player->mo->z + player->mo->height + FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT) - FixedMul(mobjinfo[type].height, player->mo->scale);
		else
			zheight = player->mo->z - FixedDiv(P_GetPlayerHeight(player) - player->mo->height, 3*FRACUNIT);

		if (!(player->mo->eflags & MFE_VERTICALFLIP) && zheight < player->mo->floorz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->floorz;
		else if (player->mo->eflags & MFE_VERTICALFLIP && zheight + FixedMul(mobjinfo[type].height, player->mo->scale) > player->mo->ceilingz && !(mobjinfo[type].flags & MF_NOCLIPHEIGHT))
			zheight = player->mo->ceilingz - FixedMul(mobjinfo[type].height, player->mo->scale);

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);

		// set to player's angle, just in case
		mobj->angle = player->mo->angle;

		// color and skin
		mobj->color = player->mo->color;
		mobj->skin = player->mo->skin;

		// vertical flip
		if (player->mo->eflags & MFE_VERTICALFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP;
		mobj->eflags |= (player->mo->eflags & MFE_VERTICALFLIP);

		// scale
		P_SetScale(mobj, player->mo->scale);
		mobj->destscale = player->mo->scale;
	}

	P_SetTarget(&mobj->target, player->mo); // the one thing P_SpawnGhostMobj doesn't do
}

//
// P_DoPlayerExit
//
// Player exits the map via sector trigger
void P_DoPlayerExit(player_t *player)
{
	if (player->exiting)
		return;

	if (cv_allowexitlevel.value == 0 && !G_PlatformGametype())
		return;
	else if (gametype == GT_RACE || gametype == GT_COMPETITION) // If in Race Mode, allow
	{
		if (!countdown) // a 60-second wait ala Sonic 2.
			countdown = cv_countdowntime.value*TICRATE + 1; // Use cv_countdowntime

		player->exiting = 3*TICRATE;

		if (!countdown2)
			countdown2 = (8 + cv_countdowntime.value)*TICRATE + 1; // 8 sec more than countdowntime -- 11 is too much

		if (P_CheckRacers())
			player->exiting = (14*TICRATE)/5 + 1;
	}
	else
		player->exiting = (14*TICRATE)/5 + 2; // Accidental death safeguard???

	//player->pflags &= ~PF_GLIDING;
	if (player->climbing)
	{
		player->climbing = 0;
		player->pflags |= PF_JUMPED;
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
	}
	player->powers[pw_underwater] = 0;
	player->powers[pw_spacetime] = 0;
	P_RestoreMusic(player);

	if (playeringame[player-players] && netgame && !circuitmap)
		CONS_Printf(M_GetText("%s has completed the level.\n"), player_names[player-players]);
}

#define SPACESPECIAL 12
boolean P_InSpaceSector(mobj_t *mo) // Returns true if you are in space
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	if (GETSECSPECIAL(sector->special, 1) == SPACESPECIAL)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (GETSECSPECIAL(rover->master->frontsector->special, 1) != SPACESPECIAL)
				continue;
#ifdef ESLOPE
			topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, mo->x, mo->y) : *rover->topheight;
			bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, mo->x, mo->y) : *rover->bottomheight;
#else
			topheight = *rover->topheight;
			bottomheight = *rover->bottomheight;
#endif

			if (mo->z + (mo->height/2) > topheight)
				continue;

			if (mo->z + (mo->height/2) < bottomheight)
				continue;

			return true;
		}
	}

	return false; // No vacuum here, Captain!
}

boolean P_InQuicksand(mobj_t *mo) // Returns true if you are in quicksand
{
	sector_t *sector = mo->subsector->sector;
	fixed_t topheight, bottomheight;

	fixed_t flipoffset = ((mo->eflags & MFE_VERTICALFLIP) ? (mo->height/2) : 0);

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_QUICKSAND))
				continue;

#ifdef ESLOPE
			topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, mo->x, mo->y) : *rover->topheight;
			bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, mo->x, mo->y) : *rover->bottomheight;
#else
			topheight = *rover->topheight;
			bottomheight = *rover->bottomheight;
#endif

			if (mo->z + flipoffset > topheight)
				continue;

			if (mo->z + (mo->height/2) + flipoffset < bottomheight)
				continue;

			return true;
		}
	}

	return false; // No sand here, Captain!
}

static void P_CheckBustableBlocks(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;

	if ((netgame || multiplayer) && player->spectator)
		return;

	oldx = player->mo->x;
	oldy = player->mo->y;

	P_UnsetThingPosition(player->mo);
	player->mo->x += player->mo->momx;
	player->mo->y += player->mo->momy;
	P_SetThingPosition(player->mo);

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
			break;

		if (node->m_sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t topheight, bottomheight;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS)) continue;

				if ((rover->flags & FF_BUSTUP)/* && !rover->master->frontsector->crumblestate*/)
				{
					// If it's an FF_SPINBUST, you have to either be jumping, or coming down
					// onto the top from a spin.
					if (rover->flags & FF_SPINBUST && ((!(player->pflags & PF_JUMPED) && !(player->pflags & PF_SPINNING)) || (player->pflags & PF_STARTDASH)))
						continue;

					// if it's not an FF_SHATTER, you must be spinning (and not jumping)
					// or have Knuckles's abilities (or Super Sonic)
					// ...or are drilling in NiGHTS (or Metal Sonic)
					if (!(rover->flags & FF_SHATTER) && !(rover->flags & FF_SPINBUST)
						&& !((player->pflags & PF_SPINNING) && !(player->pflags & PF_JUMPED))
						&& (player->charability != CA_GLIDEANDCLIMB && !player->powers[pw_super])
						&& !(player->pflags & PF_DRILLING) && !metalrecording)
						continue;

					// Only Knuckles can break this rock...
					if (!(rover->flags & FF_SHATTER) && (rover->flags & FF_ONLYKNUX) && !(player->charability == CA_GLIDEANDCLIMB))
						continue;

					topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
					bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

					// Height checks
					if (rover->flags & FF_SHATTERBOTTOM)
					{
						if (player->mo->z+player->mo->momz + player->mo->height < bottomheight)
							continue;

						if (player->mo->z+player->mo->height > bottomheight)
							continue;
					}
					else if (rover->flags & FF_SPINBUST)
					{
						if (player->mo->z+player->mo->momz > topheight)
							continue;

						if (player->mo->z + player->mo->height < bottomheight)
							continue;
					}
					else if (rover->flags & FF_SHATTER)
					{
						if (player->mo->z + player->mo->momz > topheight)
							continue;

						if (player->mo->z+player->mo->momz + player->mo->height < bottomheight)
							continue;
					}
					else
					{
						if (player->mo->z >= topheight)
							continue;

						if (player->mo->z + player->mo->height < bottomheight)
							continue;
					}

					// Impede the player's fall a bit
					if (((rover->flags & FF_SPINBUST) || (rover->flags & FF_SHATTER)) && player->mo->z >= topheight)
						player->mo->momz >>= 1;
					else if (rover->flags & FF_SHATTER)
					{
						player->mo->momx >>= 1;
						player->mo->momy >>= 1;
					}

					//if (metalrecording)
					//	G_RecordBustup(rover);

					EV_CrumbleChain(node->m_sector, rover);

					// Run a linedef executor??
					if (rover->master->flags & ML_EFFECT5)
						P_LinedefExecute((INT16)(P_AproxDistance(rover->master->dx, rover->master->dy)>>FRACBITS), player->mo, node->m_sector);

					goto bustupdone;
				}
			}
		}
	}
bustupdone:
	P_UnsetThingPosition(player->mo);
	player->mo->x = oldx;
	player->mo->y = oldy;
	P_SetThingPosition(player->mo);
}

static void P_CheckBouncySectors(player_t *player)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;
	fixed_t oldz;
#ifdef ESLOPE
	vector3_t momentum;
#endif

	oldx = player->mo->x;
	oldy = player->mo->y;
	oldz = player->mo->z;

	P_UnsetThingPosition(player->mo);
	player->mo->x += player->mo->momx;
	player->mo->y += player->mo->momy;
	player->mo->z += player->mo->momz;
	P_SetThingPosition(player->mo);

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
			break;

		if (node->m_sector->ffloors)
		{
			ffloor_t *rover;
			boolean top = true;
			fixed_t topheight, bottomheight;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS))
					continue; // FOFs should not be bouncy if they don't even "exist"

				if (GETSECSPECIAL(rover->master->frontsector->special, 1) != 15)
					continue; // this sector type is required for FOFs to be bouncy

				topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
				bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);

				if (player->mo->z > topheight)
					continue;

				if (player->mo->z + player->mo->height < bottomheight)
					continue;

				if (oldz < P_GetFOFTopZ(player->mo, node->m_sector, rover, oldx, oldy, NULL)
						&& oldz + player->mo->height > P_GetFOFBottomZ(player->mo, node->m_sector, rover, oldx, oldy, NULL))
					top = false;

				{
					fixed_t linedist;

					linedist = P_AproxDistance(rover->master->v1->x-rover->master->v2->x, rover->master->v1->y-rover->master->v2->y);

					linedist = FixedDiv(linedist,100*FRACUNIT);

					if (top)
					{
						fixed_t newmom;

#ifdef ESLOPE
						pslope_t *slope;
						if (abs(oldz - topheight) < abs(oldz + player->mo->height - bottomheight)) { // Hit top
							slope = *rover->t_slope;
						} else { // Hit bottom
							slope = *rover->b_slope;
						}

						momentum.x = player->mo->momx;
						momentum.y = player->mo->momy;
						momentum.z = player->mo->momz*2;

						if (slope)
							P_ReverseQuantizeMomentumToSlope(&momentum, slope);

						newmom = momentum.z = -FixedMul(momentum.z,linedist)/2;
#else
						newmom = -FixedMul(player->mo->momz,linedist);
#endif

						if (abs(newmom) < (linedist*2))
						{
							goto bouncydone;
						}

						if (!(rover->master->flags & ML_BOUNCY))
						{
							if (newmom > 0)
							{
								if (newmom < 8*FRACUNIT)
									newmom = 8*FRACUNIT;
							}
							else if (newmom > -8*FRACUNIT && newmom != 0)
								newmom = -8*FRACUNIT;
						}

						if (newmom > P_GetPlayerHeight(player)/2)
							newmom = P_GetPlayerHeight(player)/2;
						else if (newmom < -P_GetPlayerHeight(player)/2)
							newmom = -P_GetPlayerHeight(player)/2;

#ifdef ESLOPE
						momentum.z = newmom*2;

						if (slope)
							P_QuantizeMomentumToSlope(&momentum, slope);

						player->mo->momx = momentum.x;
						player->mo->momy = momentum.y;
						player->mo->momz = momentum.z/2;
#else
						player->mo->momz = newmom;
#endif

						if (player->pflags & PF_SPINNING)
						{
							player->pflags &= ~PF_SPINNING;
							player->pflags |= PF_JUMPED;
							player->pflags |= PF_THOKKED;
						}
					}
					else
					{
						player->mo->momx = -FixedMul(player->mo->momx,linedist);
						player->mo->momy = -FixedMul(player->mo->momy,linedist);

						if (player->pflags & PF_SPINNING)
						{
							player->pflags &= ~PF_SPINNING;
							player->pflags |= PF_JUMPED;
							player->pflags |= PF_THOKKED;
						}
					}

					if ((player->pflags & PF_SPINNING) && player->speed < FixedMul(1<<FRACBITS, player->mo->scale) && player->mo->momz)
					{
						player->pflags &= ~PF_SPINNING;
						player->pflags |= PF_JUMPED;
					}

					goto bouncydone;
				}
			}
		}
	}
bouncydone:
	P_UnsetThingPosition(player->mo);
	player->mo->x = oldx;
	player->mo->y = oldy;
	player->mo->z = oldz;
	P_SetThingPosition(player->mo);
}

static void P_CheckQuicksand(player_t *player)
{
	ffloor_t *rover;
	fixed_t sinkspeed, friction;
	fixed_t topheight, bottomheight;

	if (!(player->mo->subsector->sector->ffloors && player->mo->momz <= 0))
		return;

	for (rover = player->mo->subsector->sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->flags & FF_EXISTS)) continue;

		if (!(rover->flags & FF_QUICKSAND))
			continue;

#ifdef ESLOPE
		topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, player->mo->x, player->mo->y) : *rover->topheight;
		bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, player->mo->x, player->mo->y) : *rover->bottomheight;
#else
		topheight = *rover->topheight;
		bottomheight = *rover->bottomheight;
#endif

		if (topheight >= player->mo->z && bottomheight < player->mo->z + player->mo->height)
		{
			sinkspeed = abs(rover->master->v1->x - rover->master->v2->x)>>1;

			sinkspeed = FixedDiv(sinkspeed,TICRATE*FRACUNIT);

			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				fixed_t ceilingheight = P_GetCeilingZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z += sinkspeed;

				if (player->mo->z + player->mo->height >= ceilingheight)
					player->mo->z = ceilingheight - player->mo->height;
			}
			else
			{
				fixed_t floorheight = P_GetFloorZ(player->mo, player->mo->subsector->sector, player->mo->x, player->mo->y, NULL);

				player->mo->z -= sinkspeed;

				if (player->mo->z <= floorheight)
					player->mo->z = floorheight;
			}

			friction = abs(rover->master->v1->y - rover->master->v2->y)>>6;

			player->mo->momx = FixedMul(player->mo->momx, friction);
			player->mo->momy = FixedMul(player->mo->momy, friction);
		}
	}
}

//
// P_CheckSneakerAndLivesTimer
//
// Restores music from sneaker and life fanfares
//
static void P_CheckSneakerAndLivesTimer(player_t *player)
{
	if ((player->powers[pw_underwater] <= 11*TICRATE + 1)
	&& (player->powers[pw_underwater] > 1))
		return; // don't restore music if drowning music is playing

	if (player->powers[pw_extralife] == 1) // Extra Life!
		P_RestoreMusic(player);

	if (player->powers[pw_sneakers] == 1)
		P_RestoreMusic(player);
}

//
// P_CheckUnderwaterAndSpaceTimer
//
// Restores music from underwater and space warnings, and handles number generation
//
static void P_CheckUnderwaterAndSpaceTimer(player_t *player)
{
	fixed_t height;
	mobj_t *numbermobj = NULL;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		height = player->mo->z - FixedMul(8*FRACUNIT - mobjinfo[MT_DROWNNUMBERS].height, player->mo->scale);
	else
		height = player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, player->mo->scale);

	if (player->powers[pw_underwater] == 11*TICRATE + 1 || player->powers[pw_spacetime] == 11*TICRATE + 1)
	{
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		P_SetMobjState(numbermobj, numbermobj->info->spawnstate+5);
	}
	else if (player->powers[pw_underwater] == 9*TICRATE + 1 || player->powers[pw_spacetime] == 9*TICRATE + 1)
	{
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		P_SetMobjState(numbermobj, numbermobj->info->spawnstate+4);
	}
	else if (player->powers[pw_underwater] == 7*TICRATE + 1 || player->powers[pw_spacetime] == 7*TICRATE + 1)
	{
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		P_SetMobjState(numbermobj, numbermobj->info->spawnstate+3);
	}
	else if (player->powers[pw_underwater] == 5*TICRATE + 1 || player->powers[pw_spacetime] == 5*TICRATE + 1)
	{
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		P_SetMobjState(numbermobj, numbermobj->info->spawnstate+2);
	}
	else if (player->powers[pw_underwater] == 3*TICRATE + 1 || player->powers[pw_spacetime] == 3*TICRATE + 1)
	{
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		P_SetMobjState(numbermobj, numbermobj->info->spawnstate+1);
	}
	else if (player->powers[pw_underwater] == 1*TICRATE + 1 || player->powers[pw_spacetime] == 1*TICRATE + 1)
	{
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, height, MT_DROWNNUMBERS);
		//P_SetMobjState(numbermobj, numbermobj->info->spawnstate+0);
	}
	// Underwater timer runs out
	else if (player->powers[pw_underwater] == 1)
	{
		mobj_t *killer;

		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusname, mapmusflags, true);

		killer = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_NULL);
		killer->threshold = 42; // Special flag that it was drowning which killed you.
		P_DamageMobj(player->mo, killer, killer, 10000);
	}
	else if (player->powers[pw_spacetime] == 1)
	{
		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusname, mapmusflags, true);

		P_DamageMobj(player->mo, NULL, NULL, 10000);
	}

	if (numbermobj)
	{
		P_SetTarget(&numbermobj->target, player->mo);
		numbermobj->threshold = 40;
		S_StartSound(player->mo, sfx_dwnind);
		numbermobj->destscale = player->mo->scale;
		P_SetScale(numbermobj, player->mo->scale);
	}

	if (!(player->mo->eflags & MFE_UNDERWATER) && player->powers[pw_underwater])
	{
		if (player->powers[pw_underwater] <= 12*TICRATE + 1)
			P_RestoreMusic(player);

		player->powers[pw_underwater] = 0;
	}

	if (player->powers[pw_spacetime] > 1 && !P_InSpaceSector(player->mo))
	{
		P_RestoreMusic(player);
		player->powers[pw_spacetime] = 0;
	}

	// Underwater audio cues
	if (P_IsLocalPlayer(player) && !player->bot)
	{
		if (player->powers[pw_underwater] == 11*TICRATE + 1
		&& player == &players[consoleplayer])
		{
			S_StopMusic();
			S_ChangeMusicInternal("drown", false);
		}

		if (player->powers[pw_underwater] == 25*TICRATE + 1)
			S_StartSound(NULL, sfx_wtrdng);
		else if (player->powers[pw_underwater] == 20*TICRATE + 1)
			S_StartSound(NULL, sfx_wtrdng);
		else if (player->powers[pw_underwater] == 15*TICRATE + 1)
			S_StartSound(NULL, sfx_wtrdng);
	}

	if (player->exiting)
	{
		if (player->powers[pw_underwater] > 1)
			player->powers[pw_underwater] = 0;

		player->powers[pw_spacetime] = 0;
	}
}

//
// P_CheckInvincibilityTimer
//
// Restores music from invincibility, and handles invincibility sparkles
//
static void P_CheckInvincibilityTimer(player_t *player)
{
	if (!player->powers[pw_invulnerability])
		return;

	if (mariomode && !player->powers[pw_super])
		player->mo->color = (UINT8)(1 + (leveltime % (MAXSKINCOLORS-1)));
	else if (leveltime % (TICRATE/7) == 0)
	{
		mobj_t *sparkle = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_IVSP);
		sparkle->destscale = player->mo->scale;
		P_SetScale(sparkle, player->mo->scale);
	}

	// Resume normal music stuff.
	if (player->powers[pw_invulnerability] == 1)
	{
		if (!player->powers[pw_super])
		{
			if (mariomode)
			{
				if (player->powers[pw_shield] & SH_FIREFLOWER)
				{
					player->mo->color = SKINCOLOR_WHITE;
					G_GhostAddColor(GHC_FIREFLOWER);
				}
				else
				{
					player->mo->color = player->skincolor;
					G_GhostAddColor(GHC_NORMAL);
				}
			}

			// If you had a shield, restore its visual significance
			P_SpawnShieldOrb(player);
		}

		if ((player->powers[pw_underwater] <= 11*TICRATE + 1)
		&& (player->powers[pw_underwater] > 1))
			return; // don't restore music if drowning music is playing

		if (!player->powers[pw_super] || (mapheaderinfo[gamemap-1]->levelflags & LF_NOSSMUSIC))
			P_RestoreMusic(player);
	}
}

//
// P_DoBubbleBreath
//
// Handles bubbles spawned by the player
//
static void P_DoBubbleBreath(player_t *player)
{
	fixed_t zh;
	mobj_t *bubble = NULL;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		zh = player->mo->z + player->mo->height - FixedDiv(player->mo->height,5*(FRACUNIT/4));
	else
		zh = player->mo->z + FixedDiv(player->mo->height,5*(FRACUNIT/4));

	if (!(player->mo->eflags & MFE_UNDERWATER) || ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL && !(player->pflags & PF_NIGHTSMODE)) || player->spectator)
		return;

	if (P_RandomChance(FRACUNIT/16))
		bubble = P_SpawnMobj(player->mo->x, player->mo->y, zh, MT_SMALLBUBBLE);
	else if (P_RandomChance(3*FRACUNIT/256))
		bubble = P_SpawnMobj(player->mo->x, player->mo->y, zh, MT_MEDIUMBUBBLE);
	if (bubble)
	{
		bubble->threshold = 42;
		bubble->destscale = player->mo->scale;
		P_SetScale(bubble, bubble->destscale);
	}

	if (player->pflags & PF_NIGHTSMODE) // NiGHTS Super doesn't spawn flight bubbles
		return;

	// Tails stirs up the water while flying in it
	if (player->powers[pw_tailsfly] && (leveltime & 1) && player->charability != CA_SWIM)
	{
		fixed_t radius = (3*player->mo->radius)>>1;
		angle_t fa = ((leveltime%45)*FINEANGLES/8) & FINEMASK;
		fixed_t stirwaterx = FixedMul(FINECOSINE(fa),radius);
		fixed_t stirwatery = FixedMul(FINESINE(fa),radius);
		fixed_t stirwaterz;

		if (player->mo->eflags & MFE_VERTICALFLIP)
			stirwaterz = player->mo->z + player->mo->height - FixedDiv(player->mo->height,3*FRACUNIT/2);
		else
			stirwaterz = player->mo->z + FixedDiv(player->mo->height,3*FRACUNIT/2);

		bubble = P_SpawnMobj(
			player->mo->x + stirwaterx,
			player->mo->y + stirwatery,
			stirwaterz, MT_SMALLBUBBLE);
		bubble->destscale = player->mo->scale;
		P_SetScale(bubble,bubble->destscale);

		bubble = P_SpawnMobj(
			player->mo->x - stirwaterx,
			player->mo->y - stirwatery,
			stirwaterz, MT_SMALLBUBBLE);
		bubble->destscale = player->mo->scale;
		P_SetScale(bubble,bubble->destscale);
	}
}

//
// P_DoPlayerHeadSigns
//
// Spawns "IT" and "GOT FLAG" signs for Tag and CTF respectively
//
static void P_DoPlayerHeadSigns(player_t *player)
{
	if (G_TagGametype())
	{
		// If you're "IT", show a big "IT" over your head for others to see.
		if (player->pflags & PF_TAGIT)
		{
			if (!(player == &players[consoleplayer] || player == &players[secondarydisplayplayer] || player == &players[displayplayer])) // Don't display it on your own view.
			{
				if (!(player->mo->eflags & MFE_VERTICALFLIP))
					P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height, MT_TAG);
				else
					P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z - mobjinfo[MT_TAG].height, MT_TAG)->eflags |= MFE_VERTICALFLIP;
			}
		}
	}
	else if (gametype == GT_CTF)
	{
		if (player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)) // If you have the flag (duh).
		{
			// Spawn a got-flag message over the head of the player that
			// has it (but not on your own screen if you have the flag).
			if (splitscreen || player != &players[consoleplayer])
			{
				if (player->gotflag & GF_REDFLAG)
				{
					if (!(player->mo->eflags & MFE_VERTICALFLIP))
						P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy,
							player->mo->z+P_GetPlayerHeight(player)+FixedMul(16*FRACUNIT, player->mo->scale)+player->mo->momz, MT_GOTFLAG);
					else
						P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy,
							player->mo->z+player->mo->height-P_GetPlayerHeight(player)-mobjinfo[MT_GOTFLAG].height-FixedMul(16*FRACUNIT, player->mo->scale)+player->mo->momz, MT_GOTFLAG)->eflags |= MFE_VERTICALFLIP;
				}
				if (player->gotflag & GF_BLUEFLAG)
				{
					if (!(player->mo->eflags & MFE_VERTICALFLIP))
						P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy,
							player->mo->z+P_GetPlayerHeight(player)+FixedMul(16*FRACUNIT, player->mo->scale)+player->mo->momz, MT_GOTFLAG2);
					else
						P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy,
							player->mo->z+player->mo->height-P_GetPlayerHeight(player)-mobjinfo[MT_GOTFLAG2].height-FixedMul(16*FRACUNIT, player->mo->scale)+player->mo->momz, MT_GOTFLAG2)->eflags |= MFE_VERTICALFLIP;
				}
			}
		}
	}
}

//
// P_DoClimbing
//
// Handles player climbing
//
static void P_DoClimbing(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	fixed_t platx;
	fixed_t platy;
	subsector_t *glidesector;
	boolean climb = true;

	platx = P_ReturnThrustX(player->mo, player->mo->angle, player->mo->radius + FixedMul(8*FRACUNIT, player->mo->scale));
	platy = P_ReturnThrustY(player->mo, player->mo->angle, player->mo->radius + FixedMul(8*FRACUNIT, player->mo->scale));

	glidesector = R_IsPointInSubsector(player->mo->x + platx, player->mo->y + platy);

	if (!glidesector || glidesector->sector != player->mo->subsector->sector)
	{
		boolean floorclimb = false;
		boolean thrust = false;
		boolean boostup = false;
		boolean skyclimber = false;
		fixed_t floorheight, ceilingheight; // ESLOPE

		if (!glidesector)
			floorclimb = true;
		else
		{
#ifdef ESLOPE
			floorheight = glidesector->sector->f_slope ? P_GetZAt(glidesector->sector->f_slope, player->mo->x, player->mo->y)
													   : glidesector->sector->floorheight;
			ceilingheight = glidesector->sector->c_slope ? P_GetZAt(glidesector->sector->c_slope, player->mo->x, player->mo->y)
														 : glidesector->sector->ceilingheight;
#else
			floorheight = glidesector->sector->floorheight;
			ceilingheight = glidesector->sector->ceilingheight;
#endif

			if (glidesector->sector->ffloors)
			{
				ffloor_t *rover;
				fixed_t topheight, bottomheight; // ESLOPE

				for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER) || (rover->flags & FF_BUSTUP))
						continue;

					floorclimb = true;

#ifdef ESLOPE
					topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, player->mo->x, player->mo->y) : *rover->topheight;
					bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, player->mo->x, player->mo->y) : *rover->bottomheight;
#else
					topheight = *rover->topheight;
					bottomheight = *rover->bottomheight;
#endif

					// Only supports rovers that are moving like an 'elevator', not just the top or bottom.
					if (rover->master->frontsector->floorspeed && rover->master->frontsector->ceilspeed == 42)
					{
						if ((!(player->mo->eflags & MFE_VERTICALFLIP) && (bottomheight < player->mo->z+player->mo->height)
							&& (topheight >= player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale)))
						|| ((player->mo->eflags & MFE_VERTICALFLIP) && (topheight > player->mo->z)
							&& (bottomheight <= player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale))))
						{
							if (cmd->forwardmove != 0)
								player->mo->momz += rover->master->frontsector->floorspeed;
							else
							{
								player->mo->momz = rover->master->frontsector->floorspeed;
								climb = false;
							}
						}
					}

					// Gravity is flipped, so the comments are, too.
					if (player->mo->eflags & MFE_VERTICALFLIP)
					{
						// Trying to climb down past the bottom of the FOF
						if ((topheight >= player->mo->z + player->mo->height) && ((player->mo->z + player->mo->height + player->mo->momz) >= topheight))
						{
							fixed_t bottomheight2;
							ffloor_t *roverbelow;
							boolean foundfof = false;
							floorclimb = true;
							boostup = false;

							// Is there a FOF directly below this one that we can move onto?
							for (roverbelow = glidesector->sector->ffloors; roverbelow; roverbelow = roverbelow->next)
							{
								if (!(roverbelow->flags & FF_EXISTS) || !(roverbelow->flags & FF_BLOCKPLAYER) || (roverbelow->flags & FF_BUSTUP))
									continue;

								if (roverbelow == rover)
									continue;

#ifdef ESLOPE
								bottomheight2 = *roverbelow->b_slope ? P_GetZAt(*roverbelow->b_slope, player->mo->x, player->mo->y) : *roverbelow->bottomheight;
#else
								bottomheight2 = *roverbelow->bottomheight;
#endif

								if (bottomheight2 < topheight + FixedMul(16*FRACUNIT, player->mo->scale))
									foundfof = true;
							}

							if (!foundfof)
								player->mo->momz = 0;
						}

						// Below the FOF
						if (topheight <= player->mo->z)
						{
							floorclimb = false;
							boostup = false;
							thrust = false;
						}

						// Above the FOF
						if (bottomheight > player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale))
						{
							floorclimb = false;
							thrust = true;
							boostup = true;
						}
					}
					else
					{
						// Trying to climb down past the bottom of a FOF
						if ((bottomheight <= player->mo->z) && ((player->mo->z + player->mo->momz) <= bottomheight))
						{
							fixed_t topheight2;
							ffloor_t *roverbelow;
							boolean foundfof = false;
							floorclimb = true;
							boostup = false;

							// Is there a FOF directly below this one that we can move onto?
							for (roverbelow = glidesector->sector->ffloors; roverbelow; roverbelow = roverbelow->next)
							{
								if (!(roverbelow->flags & FF_EXISTS) || !(roverbelow->flags & FF_BLOCKPLAYER) || (roverbelow->flags & FF_BUSTUP))
									continue;

								if (roverbelow == rover)
									continue;

#ifdef ESLOPE
								topheight2 = *roverbelow->t_slope ? P_GetZAt(*roverbelow->t_slope, player->mo->x, player->mo->y) : *roverbelow->topheight;
#else
								topheight2 = *roverbelow->topheight;
#endif

								if (topheight2 > bottomheight - FixedMul(16*FRACUNIT, player->mo->scale))
									foundfof = true;
							}

							if (!foundfof)
								player->mo->momz = 0;
						}

						// Below the FOF
						if (bottomheight >= player->mo->z + player->mo->height)
						{
							floorclimb = false;
							boostup = false;
							thrust = false;
						}

						// Above the FOF
						if (topheight < player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale))
						{
							floorclimb = false;
							thrust = true;
							boostup = true;
						}
					}

					if (floorclimb)
					{
						if (rover->flags & FF_CRUMBLE && !(netgame && player->spectator))
							EV_StartCrumble(rover->master->frontsector, rover, (rover->flags & FF_FLOATBOB), player, rover->alpha, !(rover->flags & FF_NORETURN));
						break;
					}
				}
			}

			// Gravity is flipped, so are comments.
			if (player->mo->eflags & MFE_VERTICALFLIP)
			{
				// Trying to climb down past the upper texture area
				if ((floorheight >= player->mo->z + player->mo->height) && ((player->mo->z + player->mo->height + player->mo->momz) >= floorheight))
				{
					boolean foundfof = false;
					floorclimb = true;

					// Is there a FOF directly below that we can move onto?
					if (glidesector->sector->ffloors)
					{
						fixed_t bottomheight;
						ffloor_t *rover;
						for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
						{
							if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER) || (rover->flags & FF_BUSTUP))
								continue;

#ifdef ESLOPE
							bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, player->mo->x, player->mo->y) : *rover->bottomheight;
#else
							bottomheight = *rover->bottomheight;
#endif

							if (bottomheight < floorheight + FixedMul(16*FRACUNIT, player->mo->scale))
							{
								foundfof = true;
								break;
							}
						}
					}

					if (!foundfof)
						player->mo->momz = 0;
				}

				// Reached the top of the lower texture area
				if (!floorclimb && ceilingheight > player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale)
					&& (glidesector->sector->ceilingpic == skyflatnum || floorheight < (player->mo->z - FixedMul(8*FRACUNIT, player->mo->scale))))
				{
					thrust = true;
					boostup = true;
					// Play climb-up animation here
				}
			}
			else
			{
				// Trying to climb down past the upper texture area
				if ((ceilingheight <= player->mo->z) && ((player->mo->z + player->mo->momz) <= ceilingheight))
				{
					boolean foundfof = false;
					floorclimb = true;

					// Is there a FOF directly below that we can move onto?
					if (glidesector->sector->ffloors)
					{
						fixed_t topheight;
						ffloor_t *rover;
						for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
						{
							if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER) || (rover->flags & FF_BUSTUP))
								continue;

#ifdef ESLOPE
							topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, player->mo->x, player->mo->y) : *rover->topheight;
#else
							topheight = *rover->topheight;
#endif

							if (topheight > ceilingheight - FixedMul(16*FRACUNIT, player->mo->scale))
							{
								foundfof = true;
								break;
							}
						}
					}

					if (!foundfof)
						player->mo->momz = 0;
				}

				// Allow climbing from a FOF or lower texture onto the upper texture and vice versa.
				if (player->mo->z > ceilingheight - FixedMul(16*FRACUNIT, player->mo->scale))
				{
					floorclimb = true;
					thrust = false;
					boostup = false;
				}

				// Reached the top of the lower texture area
				if (!floorclimb && floorheight < player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale)
					&& (glidesector->sector->ceilingpic == skyflatnum || ceilingheight > (player->mo->z + player->mo->height + FixedMul(8*FRACUNIT, player->mo->scale))))
				{
					thrust = true;
					boostup = true;
					// Play climb-up animation here
				}
			}

			// Trying to climb on the sky
			if ((ceilingheight < player->mo->z) && glidesector->sector->ceilingpic == skyflatnum)
			{
				skyclimber = true;
			}

			// Climbing on the lower texture area?
			if ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + FixedMul(16*FRACUNIT, player->mo->scale) < floorheight)
				|| ((player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height <= floorheight))
			{
				floorclimb = true;

				if (glidesector->sector->floorspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->floorspeed;
					else
					{
						player->mo->momz = glidesector->sector->floorspeed;
						climb = false;
					}
				}
			}
			// Climbing on the upper texture area?
			else if ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z >= ceilingheight)
				|| ((player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height - FixedMul(16*FRACUNIT, player->mo->scale) > ceilingheight))
			{
				floorclimb = true;

				if (glidesector->sector->ceilspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->ceilspeed;
					else
					{
						player->mo->momz = glidesector->sector->ceilspeed;
						climb = false;
					}
				}
			}
		}

		if (player->lastsidehit != -1 && player->lastlinehit != -1)
		{
			thinker_t *think;
			scroll_t *scroller;
			angle_t sideangle;
			fixed_t dx, dy;

			for (think = thinkercap.next; think != &thinkercap; think = think->next)
			{
				if (think->function.acp1 != (actionf_p1)T_Scroll)
					continue;

				scroller = (scroll_t *)think;

				if (scroller->type != sc_side)
					continue;

				if (scroller->affectee != player->lastsidehit)
					continue;

				if (scroller->accel)
				{
					dx = scroller->vdx;
					dy = scroller->vdy;
				}
				else
				{
					dx = scroller->dx;
					dy = scroller->dy;
				}

				if (cmd->forwardmove != 0)
				{
					player->mo->momz += dy;
					climb = true;
				}
				else
				{
					player->mo->momz = dy;
					climb = false;
				}

				sideangle = R_PointToAngle2(lines[player->lastlinehit].v2->x,lines[player->lastlinehit].v2->y,lines[player->lastlinehit].v1->x,lines[player->lastlinehit].v1->y);

				if (cmd->sidemove != 0)
				{
					P_Thrust(player->mo, sideangle, dx);
					climb = true;
				}
				else
				{
					P_InstaThrust(player->mo, sideangle, dx);
					climb = false;
				}
			}
		}

		if (cmd->sidemove != 0 || cmd->forwardmove != 0)
			climb = true;
		else
			climb = false;

		if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
			&& !(player->mo->state >= &states[S_PLAY_CLIMB2] && player->mo->state <= &states[S_PLAY_CLIMB5]))
			P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB2);
		else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state != &states[S_PLAY_CLIMB1])
			P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB1);

		if (!floorclimb)
		{
			if (boostup)
				P_SetObjectMomZ(player->mo, 2*FRACUNIT, true);
			if (thrust)
				P_InstaThrust(player->mo, player->mo->angle, FixedMul(4*FRACUNIT, player->mo->scale)); // Lil' boost up.

			player->climbing = 0;
			player->pflags |= PF_JUMPED;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		}

		if (skyclimber)
		{
			player->climbing = 0;
			player->pflags |= PF_JUMPED;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		}
	}
	else
	{
		player->climbing = 0;
		player->pflags |= PF_JUMPED;
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
	}

	if (cmd->sidemove != 0 || cmd->forwardmove != 0)
		climb = true;
	else
		climb = false;

	if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
		&& !(player->mo->state >= &states[S_PLAY_CLIMB2] && player->mo->state <= &states[S_PLAY_CLIMB5]))
		P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB2);
	else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state != &states[S_PLAY_CLIMB1])
		P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB1);

	if (cmd->buttons & BT_USE && !(player->pflags & PF_JUMPSTASIS))
	{
		player->climbing = 0;
		player->pflags |= PF_JUMPED;
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		P_SetObjectMomZ(player->mo, 4*FRACUNIT, false);
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(-4*FRACUNIT, player->mo->scale));
	}

	if (!demoplayback || P_AnalogMove(player))
	{
		if (player == &players[consoleplayer])
			localangle = player->mo->angle;
		else if (player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;
	}

	if (player->climbing == 0)
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);

	if (player->climbing && P_IsObjectOnGround(player->mo))
	{
		P_ResetPlayer(player);
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}
}

//
// PIT_CheckSolidsTeeter
// AKA: PIT_HacksToStopPlayersTeeteringOnGargoyles
//

static mobj_t *teeterer; // the player checking for teetering
static boolean solidteeter; // saves whether the player can teeter on this or not
static fixed_t highesttop; // highest floor found so far
// reserved for standing on multiple objects
static boolean couldteeter;
static fixed_t teeterxl, teeterxh;
static fixed_t teeteryl, teeteryh;

static boolean PIT_CheckSolidsTeeter(mobj_t *thing)
{
	fixed_t blockdist;
	fixed_t tiptop = FixedMul(MAXSTEPMOVE, teeterer->scale);
	fixed_t thingtop = thing->z + thing->height;
	fixed_t teeterertop = teeterer->z + teeterer->height;

	if (!teeterer || !thing)
		return true;

	if (!(thing->flags & MF_SOLID))
		return true;

	if (thing->flags & MF_NOCLIP)
		return true;

	if (thing == teeterer)
		return true;

	if (thing->player && cv_tailspickup.value && gametype != GT_HIDEANDSEEK)
		return true;

	blockdist = teeterer->radius + thing->radius;

	if (abs(thing->x - teeterer->x) >= blockdist || abs(thing->y - teeterer->y) >= blockdist)
		return true; // didn't hit it

	if (teeterer->eflags & MFE_VERTICALFLIP)
	{
		if (thingtop < teeterer->z)
			return true;
		if (thing->z > highesttop)
			return true;
		highesttop = thing->z;
		if (thing->z > teeterertop + tiptop)
		{
			solidteeter = true;
			return true;
		}
	}
	else
	{
		if (thing->z > teeterertop)
			return true;
		if (thingtop < highesttop)
			return true;
		highesttop = thingtop;
		if (thingtop < teeterer->z - tiptop)
		{
			solidteeter = true;
			return true;
		}
	}

	// are you standing on this?
	if ((teeterer->eflags & MFE_VERTICALFLIP && thing->z - FixedMul(FRACUNIT, teeterer->scale) == teeterertop)
	|| (!(teeterer->eflags & MFE_VERTICALFLIP) && thingtop + FixedMul(FRACUNIT, teeterer->scale) == teeterer->z))
	{
		fixed_t teeterdist = thing->radius - FixedMul(5*FRACUNIT, teeterer->scale);
		// how far are you from the edge?
		if (abs(teeterer->x - thing->x) > teeterdist || abs(teeterer->y - thing->y) > teeterdist)
		{
			if (couldteeter) // player is standing on another object already, see if we can stand on both and not teeter!
			{
				if (thing->x - teeterdist < teeterxl)
					teeterxl = thing->x - teeterdist;
				if (thing->x + teeterdist > teeterxh)
					teeterxh = thing->x + teeterdist;
				if (thing->y - teeterdist < teeteryl)
					teeteryl = thing->y - teeterdist;
				if (thing->y + teeterdist > teeteryh)
					teeteryh = thing->y + teeterdist;

				if (teeterer->x < teeterxl)
					return true;
				if (teeterer->x > teeterxh)
					return true;
				if (teeterer->y < teeteryl)
					return true;
				if (teeterer->y > teeteryh)
					return true;

				solidteeter = false; // you can stop teetering now!
				couldteeter = false; // just in case...
				return false;
			}
			else
			{
				// too far! don't change teeter status though
				// save teeter x/y limits to bring up later
				teeterxl = thing->x - teeterdist;
				teeterxh = thing->x + teeterdist;
				teeteryl = thing->y - teeterdist;
				teeteryh = thing->y + teeterdist;
			}
			couldteeter = true;
			return true;
		}
		solidteeter = false;
		couldteeter = false;
		return false; // you're definitely not teetering, that's the end of the matter
	}
	solidteeter = false;
	return true; // you're not teetering but it's not neccessarily over, YET
}

//
// P_DoTeeter
//
// Handles player teetering
//
static void P_DoTeeter(player_t *player)
{
	boolean teeter = false;
	boolean roverfloor; // solid 3d floors?
	fixed_t floorheight, ceilingheight;
	fixed_t topheight, bottomheight; // for 3d floor usage
	const fixed_t tiptop = FixedMul(MAXSTEPMOVE, player->mo->scale); // Distance you have to be above the ground in order to teeter.

	if (player->mo->standingslope && player->mo->standingslope->zdelta >= (FRACUNIT/2)) // Always teeter if the slope is too steep.
		teeter = true;
	else // Let's do some checks...
	{
		UINT8 i;
		sector_t *sec;
		fixed_t highestceilingheight = INT32_MIN;
		fixed_t lowestfloorheight = INT32_MAX;

		teeter = false;
		roverfloor = false;
		for (i = 0; i < 4; i++)
		{
			ffloor_t *rover;

#define xsign ((i & 1) ? -1 : 1) // 0 -> 1 | 1 -> -1 | 2 -> 1 | 3 -> -1
#define ysign ((i & 2) ? 1 : -1) // 0 -> 1 | 1 -> 1 | 2 -> -1 | 3 -> -1
			fixed_t checkx = player->mo->x + (xsign*FixedMul(5*FRACUNIT, player->mo->scale));
			fixed_t checky = player->mo->y + (ysign*FixedMul(5*FRACUNIT, player->mo->scale));
#undef xsign
#undef ysign

			sec = R_PointInSubsector(checkx, checky)->sector;

			ceilingheight = sec->ceilingheight;
			floorheight = sec->floorheight;
#ifdef ESLOPE
			if (sec->c_slope)
				ceilingheight = P_GetZAt(sec->c_slope, checkx, checky);
			if (sec->f_slope)
				floorheight = P_GetZAt(sec->f_slope, checkx, checky);
#endif
			highestceilingheight = (ceilingheight > highestceilingheight) ? ceilingheight : highestceilingheight;
			lowestfloorheight = (floorheight < lowestfloorheight) ? floorheight : lowestfloorheight;

			if (!(sec->ffloors))
				continue; // move on to the next subsector

			for (rover = sec->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS)) continue;

#ifdef ESLOPE
				topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, player->mo->x, player->mo->y) : *rover->topheight;
				bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, player->mo->x, player->mo->y) : *rover->bottomheight;
#else
				topheight = *rover->topheight;
				bottomheight = *rover->bottomheight;
#endif

				if (P_CheckSolidLava(player->mo, rover))
					;
				else if (!(rover->flags & FF_BLOCKPLAYER || rover->flags & FF_QUICKSAND))
					continue; // intangible 3d floor

				if (player->mo->eflags & MFE_VERTICALFLIP)
				{
					if (bottomheight > ceilingheight) // Above the ceiling
						continue;

					if (bottomheight > player->mo->z + player->mo->height + tiptop
						|| (topheight < player->mo->z
						&& player->mo->z + player->mo->height < ceilingheight - tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
				else
				{
					if (topheight < floorheight) // Below the floor
						continue;

					if (topheight < player->mo->z - tiptop
						|| (bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}
			break; // break out of loop now, we're done
		}

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (!teeter && !roverfloor && (highestceilingheight > player->mo->ceilingz + tiptop))
					teeter = true;
		}
		else
		{
			if (!teeter && !roverfloor && (lowestfloorheight < player->mo->floorz - tiptop))
					teeter = true;
		}
	}

	{
		INT32 bx, by, xl, xh, yl, yh;

		yh = (unsigned)(player->mo->y + player->mo->radius - bmaporgy)>>MAPBLOCKSHIFT;
		yl = (unsigned)(player->mo->y - player->mo->radius - bmaporgy)>>MAPBLOCKSHIFT;
		xh = (unsigned)(player->mo->x + player->mo->radius - bmaporgx)>>MAPBLOCKSHIFT;
		xl = (unsigned)(player->mo->x - player->mo->radius - bmaporgx)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);

	// Polyobjects
#ifdef POLYOBJECTS
		validcount++;

		for (by = yl; by <= yh; by++)
			for (bx = xl; bx <= xh; bx++)
			{
				INT32 offset;
				polymaplink_t *plink; // haleyjd 02/22/06

				if (bx < 0 || by < 0 || bx >= bmapwidth || by >= bmapheight)
					continue;

				offset = by*bmapwidth + bx;

				// haleyjd 02/22/06: consider polyobject lines
				plink = polyblocklinks[offset];

				while (plink)
				{
					polyobj_t *po = plink->po;

					if (po->validcount != validcount) // if polyobj hasn't been checked
					{
						sector_t *polysec;
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!(po->flags & POF_SOLID))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						if (!P_MobjInsidePolyobj(po, player->mo))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						// We're inside it! Yess...
						polysec = po->lines[0]->backsector;

						if (po->flags & POF_CLIPPLANES)
						{
							polytop = polysec->ceilingheight;
							polybottom = polysec->floorheight;
						}
						else
						{
							polytop = INT32_MAX;
							polybottom = INT32_MIN;
						}

						if (player->mo->eflags & MFE_VERTICALFLIP)
						{
							if (polybottom > player->mo->ceilingz) // Above the ceiling
							{
								plink = (polymaplink_t *)(plink->link.next);
								continue;
							}

							if (polybottom > player->mo->z + player->mo->height + tiptop
							|| (polytop < player->mo->z
							&& player->mo->z + player->mo->height < player->mo->ceilingz - tiptop))
								teeter = true;
							else
							{
								teeter = false;
								break;
							}
						}
						else
						{
							if (polytop < player->mo->floorz) // Below the floor
							{
								plink = (polymaplink_t *)(plink->link.next);
								continue;
							}

							if (polytop < player->mo->z - tiptop
							|| (polybottom > player->mo->z + player->mo->height
							&& player->mo->z > player->mo->floorz + tiptop))
								teeter = true;
							else
							{
								teeter = false;
								break;
							}
						}
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
#endif
		if (teeter) // only bother with objects as a last resort if you were already teetering
		{
			mobj_t *oldtmthing = tmthing;
			tmthing = teeterer = player->mo;
			teeterxl = teeterxh = player->mo->x;
			teeteryl = teeteryh = player->mo->y;
			couldteeter = false;
			solidteeter = teeter;
			for (by = yl; by <= yh; by++)
				for (bx = xl; bx <= xh; bx++)
				{
					highesttop = INT32_MIN;
					if (!P_BlockThingsIterator(bx, by, PIT_CheckSolidsTeeter))
						goto teeterdone; // we've found something that stops us teetering at all, how about we stop already
				}
teeterdone:
			teeter = solidteeter;
			tmthing = oldtmthing; // restore old tmthing, goodness knows what the game does with this before mobj thinkers
		}
	}
	if (teeter)
	{
		if ((player->mo->state == &states[S_PLAY_STND] || player->mo->state == &states[S_PLAY_TAP1] || player->mo->state == &states[S_PLAY_TAP2] || player->mo->state == &states[S_PLAY_SUPERSTAND]))
			P_SetPlayerMobjState(player->mo, S_PLAY_TEETER1);
	}
	else if ((player->mo->state == &states[S_PLAY_TEETER1] || player->mo->state == &states[S_PLAY_TEETER2] || player->mo->state == &states[S_PLAY_SUPERTEETER]))
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
}

//
// P_SetWeaponDelay
//
// Sets weapon delay.  Properly accounts for Knux's firing rate bonus.
//
static void P_SetWeaponDelay(player_t *player, INT32 delay)
{
	player->weapondelay = delay;

	if (player->skin == 2) // Knuckles
	{
		// Multiply before dividing.
		// Loss of precision can make a surprisingly large difference.
		player->weapondelay *= 2;
		player->weapondelay /= 3;
	}
}

//
// P_DoFiring()
//
// Handles firing ring weapons and Mario fireballs.
//
static void P_DoFiring(player_t *player, ticcmd_t *cmd)
{
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (cmd->buttons & BT_ATTACK || cmd->buttons & BT_FIRENORMAL)
	{
		if (!(player->pflags & PF_ATTACKDOWN) && player->powers[pw_shield] & SH_FIREFLOWER && !player->climbing)
		{
			player->pflags |= PF_ATTACKDOWN;
			P_SpawnPlayerMissile(player->mo, MT_FIREBALL, 0);
			S_StartSound(player->mo, sfx_mario7);
		}
		else if (G_RingSlingerGametype() && (!G_TagGametype() || player->pflags & PF_TAGIT)
		  && !player->weapondelay && !player->climbing
		  && !(player->pflags & PF_ATTACKDOWN))
		{
			mobj_t *mo = NULL;
			player->pflags |= PF_ATTACKDOWN;

			if (cmd->buttons & BT_FIRENORMAL) // No powers, just a regular ring.
				goto firenormal; //code repetition sucks.
			// Bounce ring
			else if (player->currentweapon == WEP_BOUNCE && player->powers[pw_bouncering])
			{
				if (player->health <= 1)
					return;
				P_SetWeaponDelay(player, TICRATE/4);

				mo = P_SpawnPlayerMissile(player->mo, MT_THROWNBOUNCE, MF2_BOUNCERING);

				if (mo)
					mo->fuse = 3*TICRATE; // Bounce Ring time

				player->powers[pw_bouncering]--;
				player->mo->health--;
				player->health--;
			}
			// Rail ring
			else if (player->currentweapon == WEP_RAIL && player->powers[pw_railring])
			{
				if (player->health <= 1)
					return;
				P_SetWeaponDelay(player, (3*TICRATE)/2);

				mo = P_SpawnPlayerMissile(player->mo, MT_REDRING, MF2_RAILRING|MF2_DONTDRAW);

				// Rail has no unique thrown object, therefore its sound plays here.
				S_StartSound(player->mo, sfx_rail1);

				player->powers[pw_railring]--;
				player->mo->health--;
				player->health--;
			}
			// Automatic
			else if (player->currentweapon == WEP_AUTO && player->powers[pw_automaticring])
			{
				if (player->health <= 1)
					return;
				player->pflags &= ~PF_ATTACKDOWN;
				P_SetWeaponDelay(player, 2);

				mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATIC, MF2_AUTOMATIC);

				player->powers[pw_automaticring]--;
				player->mo->health--;
				player->health--;
			}
			// Explosion
			else if (player->currentweapon == WEP_EXPLODE && player->powers[pw_explosionring])
			{
				if (player->health <= 1)
					return;
				P_SetWeaponDelay(player, (3*TICRATE)/2);

				mo = P_SpawnPlayerMissile(player->mo, MT_THROWNEXPLOSION, MF2_EXPLOSION);

				player->powers[pw_explosionring]--;
				player->mo->health--;
				player->health--;
			}
			// Grenade
			else if (player->currentweapon == WEP_GRENADE && player->powers[pw_grenadering])
			{
				if (player->health <= 1)
					return;
				P_SetWeaponDelay(player, TICRATE/3);

				mo = P_SpawnPlayerMissile(player->mo, MT_THROWNGRENADE, MF2_EXPLOSION);

				if (mo)
				{
					//P_InstaThrust(mo, player->mo->angle, FixedMul(mo->info->speed, player->mo->scale));
					mo->fuse = mo->info->mass;
				}

				player->powers[pw_grenadering]--;
				player->mo->health--;
				player->health--;
			}
			// Scatter
			// Note: Ignores MF2_RAILRING
			else if (player->currentweapon == WEP_SCATTER && player->powers[pw_scatterring])
			{
				fixed_t oldz = player->mo->z;
				angle_t shotangle = player->mo->angle;
				angle_t oldaiming = player->aiming;

				if (player->health <= 1)
					return;
				P_SetWeaponDelay(player, (2*TICRATE)/3);

				// Center
				mo = P_SpawnPlayerMissile(player->mo, MT_THROWNSCATTER, MF2_SCATTER);
				if (mo)
					shotangle = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y);

				// Left
				mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle-ANG2, true, MF2_SCATTER);

				// Right
				mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle+ANG2, true, MF2_SCATTER);

				// Down
				player->mo->z += FixedMul(12*FRACUNIT, player->mo->scale);
				player->aiming += ANG1;
				mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle, true, MF2_SCATTER);

				// Up
				player->mo->z -= FixedMul(24*FRACUNIT, player->mo->scale);
				player->aiming -= ANG2;
				mo = P_SPMAngle(player->mo, MT_THROWNSCATTER, shotangle, true, MF2_SCATTER);

				player->mo->z = oldz;
				player->aiming = oldaiming;

				player->powers[pw_scatterring]--;
				player->mo->health--;
				player->health--;
				return;
			}
			// No powers, just a regular ring.
			else
			{
firenormal:
				// Infinity ring was selected.
				// Mystic wants this ONLY to happen specifically if it's selected,
				// and to not be able to get around it EITHER WAY with firenormal.

				// Infinity Ring
				if (player->currentweapon == 0
				&& player->powers[pw_infinityring])
				{
					P_SetWeaponDelay(player, TICRATE/4);

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNINFINITY, 0);

					player->powers[pw_infinityring]--;
				}
				// Red Ring
				else
				{
					if (player->health <= 1)
						return;
					P_SetWeaponDelay(player, TICRATE/4);

					mo = P_SpawnPlayerMissile(player->mo, MT_REDRING, 0);

					if (mo)
						P_ColorTeamMissile(mo, player);

					player->mo->health--;
					player->health--;
				}
			}

			if (mo)
			{
				if (mo->flags & MF_MISSILE && mo->flags2 & MF2_RAILRING)
				{
					const boolean nblockmap = !(mo->flags & MF_NOBLOCKMAP);
					for (i = 0; i < 256; i++)
					{
						if (nblockmap)
						{
							P_UnsetThingPosition(mo);
							mo->flags |= MF_NOBLOCKMAP;
							P_SetThingPosition(mo);
						}

						if (i&1)
							P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

						if (P_RailThinker(mo))
							break; // mobj was removed (missile hit a wall) or couldn't move
					}

					// Other rail sound plays at contact point.
					S_StartSound(mo, sfx_rail2);
				}
			}
		}
		return;
	}

	// Not holding any firing buttons anymore.
	// Release the grenade / whatever.
	player->pflags &= ~PF_ATTACKDOWN;
}

//
// P_DoSuperStuff()
//
// Handle related superform functionality.
//
static void P_DoSuperStuff(player_t *player)
{
	mobj_t *spark;
	ticcmd_t *cmd = &player->cmd;
	if (player->mo->state >= &states[S_PLAY_SUPERTRANS1] && player->mo->state <= &states[S_PLAY_SUPERTRANS9])
		return; // don't do anything right now, we're in the middle of transforming!

	if (player->pflags & PF_NIGHTSMODE)
		return; // NiGHTS Super doesn't mix with normal super

	// Does player have all emeralds? If so, flag the "Ready For Super!"
	if ((ALL7EMERALDS(emeralds) || ALL7EMERALDS(player->powers[pw_emeralds])) && player->health > 50)
		player->pflags |= PF_SUPERREADY;
	else
		player->pflags &= ~PF_SUPERREADY;

	if (player->powers[pw_super])
	{
		// If you're super and not Sonic, de-superize!
		if (!((ALL7EMERALDS(emeralds)) && (player->charflags & SF_SUPER)) && !(ALL7EMERALDS(player->powers[pw_emeralds])))
		{
			player->powers[pw_super] = 0;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			P_RestoreMusic(player);
			P_SpawnShieldOrb(player);

			// Restore color
			if (player->powers[pw_shield] & SH_FIREFLOWER)
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}
			else
			{
				player->mo->color = player->skincolor;
				G_GhostAddColor(GHC_NORMAL);
			}

			if (gametype != GT_COOP)
			{
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
			}
			return;
		}

		// Deplete one ring every second while super
		if ((leveltime % TICRATE == 0) && !(player->exiting))
		{
			player->health--;
			player->mo->health--;
		}

		// future todo: a skin option for this, and possibly more colors
		switch (player->skin)
		{
			case 1:  /* Tails    */ player->mo->color = SKINCOLOR_TSUPER1; break;
			case 2:  /* Knux     */ player->mo->color = SKINCOLOR_KSUPER1; break;
			default: /* everyone */ player->mo->color = SKINCOLOR_SUPER1; break;
		}
		player->mo->color += abs( ( (signed)( (unsigned)leveltime >> 1 ) % 9) - 4);

		if ((cmd->forwardmove != 0 || cmd->sidemove != 0 || player->pflags & (PF_CARRIED|PF_ROPEHANG|PF_ITEMHANG|PF_MACESPIN))
		&& !(leveltime % TICRATE) && (player->mo->momx || player->mo->momy))
		{
			spark = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SUPERSPARK);
			spark->destscale = player->mo->scale;
			P_SetScale(spark, player->mo->scale);
		}

		G_GhostAddColor(GHC_SUPER);

		// Ran out of rings while super!
		if (player->health <= 1 || player->exiting)
		{
			player->powers[pw_emeralds] = 0; // lost the power stones
			P_SpawnGhostMobj(player->mo);

			player->powers[pw_super] = 0;

			// Restore color
			if (player->powers[pw_shield] & SH_FIREFLOWER)
			{
				player->mo->color = SKINCOLOR_WHITE;
				G_GhostAddColor(GHC_FIREFLOWER);
			}
			else
			{
				player->mo->color = player->skincolor;
				G_GhostAddColor(GHC_NORMAL);
			}

			if (gametype != GT_COOP)
				player->powers[pw_flashing] = flashingtics-1;

			if (player->mo->health > 0)
			{
				if ((player->pflags & PF_JUMPED) || (player->pflags & PF_SPINNING))
					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
				else if (player->panim == PA_RUN)
					P_SetPlayerMobjState(player->mo, S_PLAY_SPD1);
				else if (player->panim == PA_WALK)
					P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
				else
					P_SetPlayerMobjState(player->mo, S_PLAY_STND);

				if (!player->exiting)
				{
					player->health = 1;
					player->mo->health = 1;
				}
			}

			// Inform the netgame that the champion has fallen in the heat of battle.
			if (gametype != GT_COOP)
			{
				S_StartSound(NULL, sfx_s3k66); //let all players hear it.
				HU_SetCEchoFlags(0);
				HU_SetCEchoDuration(5);
				HU_DoCEcho(va("%s\\is no longer super.\\\\\\\\", player_names[player-players]));
			}

			// Resume normal music if you're the console player
			P_RestoreMusic(player);

			// If you had a shield, restore its visual significance.
			P_SpawnShieldOrb(player);
		}
	}
}

//
// P_SuperReady
//
// Returns true if player is ready to turn super, duh
//
boolean P_SuperReady(player_t *player)
{
	if ((player->pflags & PF_SUPERREADY) && !player->powers[pw_super] && !player->powers[pw_tailsfly]
	&& !(player->powers[pw_shield] & SH_NOSTACK)
	&& !player->powers[pw_invulnerability]
	&& !(maptol & TOL_NIGHTS) // don't turn 'regular super' in nights levels
	&& player->pflags & PF_JUMPED
	&& ((player->charflags & SF_SUPER) || ALL7EMERALDS(player->powers[pw_emeralds])))
		return true;

	return false;
}

//
// P_DoJump
//
// Jump routine for the player
//
void P_DoJump(player_t *player, boolean soundandstate)
{
	fixed_t factor;
	const fixed_t dist6 = FixedMul(FixedDiv(player->speed, player->mo->scale), player->actionspd)/20;

	if (player->pflags & PF_JUMPSTASIS)
		return;

	if (!player->jumpfactor)
		return;

	if (player->climbing)
	{
		// Jump this high.
		if (player->powers[pw_super])
			player->mo->momz = 5*FRACUNIT;
		else if (player->mo->eflags & MFE_UNDERWATER)
			player->mo->momz = 2*FRACUNIT;
		else
			player->mo->momz = 15*(FRACUNIT/4);

		player->mo->angle = player->mo->angle - ANGLE_180; // Turn around from the wall you were climbing.

		if (!demoplayback || P_AnalogMove(player))
		{
			if (player == &players[consoleplayer])
				localangle = player->mo->angle; // Adjust the local control angle.
			else if (player == &players[secondarydisplayplayer])
				localangle2 = player->mo->angle;
		}

		player->climbing = 0; // Stop climbing, duh!
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(6*FRACUNIT, player->mo->scale)); // Jump off the wall.
	}
	// Quicksand jumping.
	else if (P_InQuicksand(player->mo))
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;
		player->mo->momz += (39*(FRACUNIT/4))>>1;
		if (player->mo->momz >= 6*FRACUNIT)
			player->mo->momz = 6*FRACUNIT; //max momz in quicksand
		else if (player->mo->momz < 0) // still descending?
			player->mo->momz = (39*(FRACUNIT/4))>>1; // just default to the jump height.
	}
	else if (!(player->pflags & PF_JUMPED)) // Spin Attack
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;

		// Jump this high.
		if (player->pflags & PF_CARRIED)
		{
			player->mo->momz = 9*FRACUNIT;
			player->pflags &= ~PF_CARRIED;
			if (player-players == consoleplayer && botingame)
				CV_SetValue(&cv_analog2, true);
		}
		else if (player->pflags & PF_ITEMHANG)
		{
			player->mo->momz = 9*FRACUNIT;
			player->pflags &= ~PF_ITEMHANG;
		}
		else if (player->pflags & PF_ROPEHANG)
		{
			player->mo->momz = 12*FRACUNIT;
			player->pflags &= ~PF_ROPEHANG;
			P_SetTarget(&player->mo->tracer, NULL);
		}
		else if (player->mo->eflags & MFE_GOOWATER)
		{
			player->mo->momz = 7*FRACUNIT;
			if (player->charability == CA_JUMPBOOST && onground)
			{
				if (player->charability2 == CA2_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}
		else if (maptol & TOL_NIGHTS)
			player->mo->momz = 24*FRACUNIT;
		else if (player->powers[pw_super])
		{
			if (player->charability == CA_FLOAT)
				player->mo->momz = 28*FRACUNIT; //Obscene jump height anyone?
			else if (player->charability == CA_SLOWFALL)
				player->mo->momz = 37*(FRACUNIT/2); //Less obscene because during super, floating propells oneself upward.
			else // Default super jump momentum.
				player->mo->momz = 13*FRACUNIT;

			// Add a boost for super characters with float/slowfall and multiability.
			if (player->charability2 == CA2_MULTIABILITY &&
				(player->charability == CA_FLOAT || player->charability == CA_SLOWFALL))
				player->mo->momz += 2*FRACUNIT;
			else if (player->charability == CA_JUMPBOOST)
			{
				if (player->charability2 == CA2_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}
		else if (player->charability2 == CA2_MULTIABILITY &&
			(player->charability == CA_DOUBLEJUMP || player->charability == CA_FLOAT || player->charability == CA_SLOWFALL))
		{
			// Multiability exceptions, since some abilities cannot effectively use it and need a boost.
			if (player->charability == CA_DOUBLEJUMP)
				player->mo->momz = 23*(FRACUNIT/2); // Increased jump height instead of infinite jumps.
			else if (player->charability == CA_FLOAT || player->charability == CA_SLOWFALL)
				player->mo->momz = 12*FRACUNIT; // Increased jump height due to ineffective repeat.
		}
		else
		{
			player->mo->momz = 39*(FRACUNIT/4); // Default jump momentum.
			if (player->charability == CA_JUMPBOOST && onground)
			{
				if (player->charability2 == CA2_MULTIABILITY)
					player->mo->momz += FixedMul(FRACUNIT/4, dist6);
				else
					player->mo->momz += FixedMul(FRACUNIT/8, dist6);
			}
		}

		// Reduce player momz by 58.5% when underwater.
		if (player->mo->eflags & MFE_UNDERWATER)
			player->mo->momz = FixedMul(player->mo->momz, FixedDiv(117*FRACUNIT, 200*FRACUNIT));

		player->jumping = 1;
	}

	factor = player->jumpfactor;

	if (twodlevel || (player->mo->flags2 & MF2_TWOD))
		factor += player->jumpfactor / 10;

	P_SetObjectMomZ(player->mo, FixedMul(factor, player->mo->momz), false); // Custom height

	// set just an eensy above the ground
	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		player->mo->z--;
		if (player->mo->pmomz < 0)
			player->mo->momz += player->mo->pmomz; // Add the platform's momentum to your jump.
		else
			player->mo->pmomz = 0;
	}
	else
	{
		player->mo->z++;
		if (player->mo->pmomz > 0)
			player->mo->momz += player->mo->pmomz; // Add the platform's momentum to your jump.
		else
			player->mo->pmomz = 0;
	}
	player->mo->eflags &= ~MFE_APPLYPMOMZ;

	player->pflags |= PF_JUMPED;

	if (soundandstate)
	{
		if (!player->spectator)
			S_StartSound(player->mo, sfx_jump); // Play jump sound!

		if (!(player->charability2 == CA2_SPINDASH))
			P_SetPlayerMobjState(player->mo, S_PLAY_SPRING);
		else
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
	}
}

//
// P_DoSpinDash
//
// Player spindash handling
//
static void P_DoSpinDash(player_t *player, ticcmd_t *cmd)
{
	if (player->pflags & PF_STASIS)
		return;

#ifdef HAVE_BLUA
	if (cmd->buttons & BT_USE)
	{
		if (LUAh_SpinSpecial(player))
			return;
	}
#endif

	// Spinning and Spindashing
	if ((player->charability2 == CA2_SPINDASH) && !(player->pflags & PF_SLIDING) && !player->exiting
		&& !P_PlayerInPain(player)) // subsequent revs
	{
		if ((cmd->buttons & BT_USE) && player->speed < FixedMul(5<<FRACBITS, player->mo->scale) && !player->mo->momz && onground && !(player->pflags & PF_USEDOWN) && !(player->pflags & PF_SPINNING)
#ifdef ESLOPE
			&& (!player->mo->standingslope || (player->mo->standingslope->flags & SL_NOPHYSICS) || abs(player->mo->standingslope->zdelta) < FRACUNIT/2)
#endif
			)
		{
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
			player->pflags |= PF_STARTDASH|PF_SPINNING;
			player->dashspeed = FixedMul(FRACUNIT, player->mo->scale);
			player->dashtime = 0;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			player->pflags |= PF_USEDOWN;
		}
		else if ((cmd->buttons & BT_USE) && (player->pflags & PF_STARTDASH))
		{
			player->dashspeed += FixedMul(FRACUNIT, player->mo->scale);

			if (!(player->dashtime++ % 5))
			{
				if (!player->spectator && player->dashspeed < FixedMul(player->maxdash, player->mo->scale))
					S_StartSound(player->mo, sfx_spndsh); // Make the rev sound!

				// Now spawn the color thok circle.
				P_SpawnSpinMobj(player, player->revitem);
				if (demorecording)
					G_GhostAddRev();
			}
		}
		// If not moving up or down, and travelling faster than a speed of four while not holding
		// down the spin button and not spinning.
		// AKA Just go into a spin on the ground, you idiot. ;)
		else if ((cmd->buttons & BT_USE || ((twodlevel || (player->mo->flags2 & MF2_TWOD)) && cmd->forwardmove < -20))
			&& !player->climbing && !player->mo->momz && onground && (player->speed > FixedMul(5<<FRACBITS, player->mo->scale)
#ifdef ESLOPE
			|| (player->mo->standingslope && (!(player->mo->standingslope->flags & SL_NOPHYSICS)) && abs(player->mo->standingslope->zdelta) >= FRACUNIT/2)
#endif
			) && !(player->pflags & PF_USEDOWN) && !(player->pflags & PF_SPINNING))
		{
			player->pflags |= PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			if (!player->spectator)
				S_StartSound(player->mo, sfx_spin);
			player->pflags |= PF_USEDOWN;
		}
	}

	// Rolling normally
	if (onground && player->pflags & PF_SPINNING && !(player->pflags & PF_STARTDASH)
		&& player->speed < FixedMul(5*FRACUNIT,player->mo->scale)
#ifdef ESLOPE
			&& (!player->mo->standingslope || (player->mo->standingslope->flags & SL_NOPHYSICS) || abs(player->mo->standingslope->zdelta) < FRACUNIT/2)
#endif
			)
	{
		if (GETSECSPECIAL(player->mo->subsector->sector->special, 4) == 7 || (player->mo->ceilingz - player->mo->floorz < P_GetPlayerHeight(player)))
			P_InstaThrust(player->mo, player->mo->angle, FixedMul(10*FRACUNIT, player->mo->scale));
		else
		{
			player->skidtime = 0;
			player->pflags &= ~PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
		}
	}

	// Catapult the player from a spindash rev!
	if (onground && !(player->pflags & PF_USEDOWN) && player->dashspeed && (player->pflags & PF_STARTDASH) && (player->pflags & PF_SPINNING))
	{
		if (player->powers[pw_ingoop])
			player->dashspeed = 0;

		player->pflags &= ~PF_STARTDASH;
		if (!((gametype == GT_RACE || gametype == GT_COMPETITION) && leveltime < 4*TICRATE))
		{
			P_InstaThrust(player->mo, player->mo->angle, player->dashspeed); // catapult forward ho!!
			if (!player->spectator)
				S_StartSound(player->mo, sfx_zoom);
		}
		player->dashspeed = 0;
	}

	if (onground && (player->pflags & PF_SPINNING) && !(player->panim == PA_ROLL))
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
}

//
// P_DoJumpShield
//
// Jump Shield Activation
//
void P_DoJumpShield(player_t *player)
{
	if (player->pflags & PF_THOKKED)
		return;

	player->pflags &= ~PF_JUMPED;
	P_DoJump(player, false);
	player->pflags &= ~PF_JUMPED;
	player->secondjump = 0;
	player->jumping = 0;
	player->pflags |= PF_THOKKED;
	player->pflags &= ~PF_SPINNING;
	P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
	S_StartSound(player->mo, sfx_wdjump);
}

//
// P_Telekinesis
//
// Morph's fancy stuff-moving character ability
// +ve thrust pushes away, -ve thrust pulls in
//
void P_Telekinesis(player_t *player, fixed_t thrust, fixed_t range)
{
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist = 0;
	angle_t an;

	if (player->powers[pw_super]) // increase range when super
		range *= 2;

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2 == player->mo)
			continue;

		if (!((mo2->flags & MF_SHOOTABLE && mo2->flags & MF_ENEMY) || mo2->type == MT_EGGGUARD || mo2->player))
			continue;

		dist = P_AproxDistance(P_AproxDistance(player->mo->x-mo2->x, player->mo->y-mo2->y), player->mo->z-mo2->z);

		if (range < dist)
			continue;

		if (!P_CheckSight(player->mo, mo2))
			continue; // if your psychic powers can't "see" it don't bother

		an = R_PointToAngle2(player->mo->x, player->mo->y, mo2->x, mo2->y);

		if (mo2->health > 0)
		{
			P_Thrust(mo2, an, thrust);

			if (mo2->type == MT_GOLDBUZZ || mo2->type == MT_REDBUZZ)
				mo2->tics += 8;
		}
	}

	P_SpawnThokMobj(player);
	player->pflags |= PF_THOKKED;
}

//
// P_DoJumpStuff
//
// Handles player jumping
//
static void P_DoJumpStuff(player_t *player, ticcmd_t *cmd)
{
	if (player->pflags & PF_JUMPSTASIS)
		return;

	if (cmd->buttons & BT_USE && !(player->pflags & PF_JUMPDOWN) && !player->exiting && !P_PlayerInPain(player))
	{
		if (onground || player->climbing || player->pflags & (PF_CARRIED|PF_ITEMHANG|PF_ROPEHANG))
		{}
		else if (player->pflags & PF_MACESPIN && player->mo->tracer)
		{}
		else if (!(player->pflags & PF_SLIDING) && ((gametype != GT_CTF) || (!player->gotflag)))
		{
#ifdef HAVE_BLUA
			if (!LUAh_JumpSpinSpecial(player))
#endif
			switch (player->charability)
			{
				case CA_TELEKINESIS:
					if (player->pflags & PF_JUMPED)
					{
						if (!(player->pflags & PF_THOKKED) || (player->charability2 == CA2_MULTIABILITY))
						{
							P_Telekinesis(player,
								-FixedMul(player->actionspd, player->mo->scale), // -ve thrust (pulling towards player)
								FixedMul(384*FRACUNIT, player->mo->scale));
						}
					}
					break;
				case CA_AIRDRILL:
					if (player->pflags & PF_JUMPED)
					{
						if (player->pflags & PF_THOKKED) // speed up falling down
						{
							if (player->secondjump < 42)
								player->secondjump ++;
						}
					}
					break;
				default:
					break;
			}
		}
	}

	if (player->charability == CA_AIRDRILL)
	{
		if (player->pflags & PF_JUMPED)
		{
			if (player->flyangle > 0 && player->pflags & PF_THOKKED)
			{
				player->flyangle--;

				P_SetObjectMomZ(player->mo, ((player->flyangle-24 - player->secondjump*3)*((player->actionspd>>FRACBITS)/12 + 1)<<FRACBITS)/7, false);

				P_SpawnThokMobj(player);

				if ((player->mo->eflags & MFE_UNDERWATER))
					P_InstaThrust(player->mo, player->mo->angle, FixedMul(player->normalspeed, player->mo->scale)*(80-player->flyangle - (player->actionspd>>FRACBITS)/2)/80);
				else
					P_InstaThrust(player->mo, player->mo->angle, ((FixedMul(player->normalspeed - player->actionspd/4, player->mo->scale))*2)/3);
			}
		}
	}

	if (cmd->buttons & BT_JUMP && !player->exiting && !P_PlayerInPain(player))
	{
#ifdef HAVE_BLUA
		if (LUAh_JumpSpecial(player))
			;
		else
#endif
		if (player->pflags & PF_JUMPDOWN) // all situations below this require jump button not to be pressed already
			;
		else
		// Jump S3&K style while in quicksand.
		if (P_InQuicksand(player->mo))
		{
			P_DoJump(player, true);
			player->secondjump = 0;
			player->pflags &= ~PF_THOKKED;
		}
		else
		// can't jump while in air, can't jump while jumping
		if (onground || player->climbing || player->pflags & (PF_CARRIED|PF_ITEMHANG|PF_ROPEHANG))
		{
			P_DoJump(player, true);
			player->secondjump = 0;
			player->pflags &= ~PF_THOKKED;
		}
		else if (player->pflags & PF_MACESPIN && player->mo->tracer)
		{
			player->pflags &= ~PF_MACESPIN;
			player->powers[pw_flashing] = TICRATE/4;
		}
		else if (player->pflags & PF_SLIDING || (gametype == GT_CTF && player->gotflag))
			;
		else if (P_SuperReady(player))
		{
			// If you can turn super and aren't already,
			// and you don't have a shield, do it!
			P_DoSuperTransformation(player, false);
		}
		else if (player->pflags & PF_JUMPED)
		{
#ifdef HAVE_BLUA
			if (!LUAh_AbilitySpecial(player))
#endif
			switch (player->charability)
			{
				case CA_THOK:
				case CA_HOMINGTHOK:
				case CA_JUMPTHOK: // Credit goes to CZ64 and Sryder13 for the original
					// Now it's Sonic's abilities turn!
					// THOK!
					if (!(player->pflags & PF_THOKKED) || (player->charability2 == CA2_MULTIABILITY))
					{
						// Catapult the player
						fixed_t actionspd = player->actionspd;
						if (player->mo->eflags & MFE_UNDERWATER)
							actionspd >>= 1;
						if ((player->charability == CA_JUMPTHOK) && !(player->pflags & PF_THOKKED))
						{
							player->pflags &= ~PF_JUMPED;
							P_DoJump(player, false);
						}
						P_InstaThrust(player->mo, player->mo->angle, FixedMul(actionspd, player->mo->scale));

						if (maptol & TOL_2D)
						{
							player->mo->momx /= 2;
							player->mo->momy /= 2;
						}
						else if (player->charability == CA_HOMINGTHOK)
						{
							player->mo->momx /= 3;
							player->mo->momy /= 3;
						}

						if (player->mo->info->attacksound && !player->spectator)
							S_StartSound(player->mo, player->mo->info->attacksound); // Play the THOK sound

						P_SpawnThokMobj(player);

						if (player->charability == CA_HOMINGTHOK && !player->homing)
						{
							if (P_LookForEnemies(player))
							{
								if (player->mo->tracer)
									player->homing = 3*TICRATE;
							}
						}

						player->pflags &= ~(PF_SPINNING|PF_STARTDASH);
						player->pflags |= PF_THOKKED;
					}
					break;

				case CA_FLY:
				case CA_SWIM: // Swim
					// If currently in the air from a jump, and you pressed the
					// button again and have the ability to fly, do so!
					if (player->charability == CA_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
						; // Can't do anything if you're a fish out of water!
					else if (!(player->pflags & PF_THOKKED) && !(player->powers[pw_tailsfly]))
					{
						P_SetPlayerMobjState(player->mo, S_PLAY_ABL1); // Change to the flying animation

						player->powers[pw_tailsfly] = tailsflytics + 1; // Set the fly timer

						player->pflags &= ~(PF_JUMPED|PF_SPINNING|PF_STARTDASH);
						player->pflags |= PF_THOKKED;
					}
					break;
				case CA_GLIDEANDCLIMB:
					// Now Knuckles-type abilities are checked.
					// If you can turn super and aren't already,
					// and you don't have a shield, do it!
					if (!(player->pflags & PF_THOKKED) || player->charability2 == CA2_MULTIABILITY)
					{
						INT32 glidespeed = player->actionspd;

						player->pflags |= PF_GLIDING|PF_THOKKED;
						player->glidetime = 0;

						if (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]))
						{
							// Glide at double speed while super.
							glidespeed *= 2;
							player->pflags &= ~PF_THOKKED;
						}

						P_SetPlayerMobjState(player->mo, S_PLAY_ABL1);
						P_InstaThrust(player->mo, player->mo->angle, FixedMul(glidespeed, player->mo->scale));
						player->pflags &= ~(PF_SPINNING|PF_STARTDASH);
					}
					break;
				case CA_DOUBLEJUMP: // Double-Jump
					if (!(player->pflags & PF_THOKKED))
					{
						player->pflags &= ~PF_JUMPED;
						P_DoJump(player, true);

						// Allow infinite double jumping if super.
						if (!player->powers[pw_super])
							player->pflags |= PF_THOKKED;
					}
					break;
				case CA_FLOAT: // Float
				case CA_SLOWFALL: // Slow descent hover
					if (!player->secondjump)
						player->secondjump = 1;
					break;
				case CA_TELEKINESIS:
					if (!(player->pflags & PF_THOKKED) || player->charability2 == CA2_MULTIABILITY)
					{
						P_Telekinesis(player,
							FixedMul(player->actionspd, player->mo->scale), // +ve thrust (pushing away from player)
							FixedMul(384*FRACUNIT, player->mo->scale));
					}
					break;
				case CA_FALLSWITCH:
					if (!(player->pflags & PF_THOKKED) || player->charability2 == CA2_MULTIABILITY)
					{
						player->mo->momz = -player->mo->momz;
						P_SpawnThokMobj(player);
						player->pflags |= PF_THOKKED;
					}
					break;

				case CA_AIRDRILL:
					if (!(player->pflags & PF_THOKKED) || player->charability2 == CA2_MULTIABILITY)
					{
						player->flyangle = 56 + (60-(player->actionspd>>FRACBITS))/3;
						player->pflags |= PF_THOKKED;
						S_StartSound(player->mo, sfx_spndsh);
					}
					break;
				default:
					break;
			}
		}
		else if (player->pflags & PF_THOKKED)
		{
#ifdef HAVE_BLUA
			if (!LUAh_AbilitySpecial(player))
#endif
			switch (player->charability)
			{
				case CA_FLY:
				case CA_SWIM: // Swim
					if (player->charability == CA_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
						; // Can't do anything if you're a fish out of water!
					else if (player->powers[pw_tailsfly]) // If currently flying, give an ascend boost.
					{
						if (!player->fly1)
							player->fly1 = 20;
						else
							player->fly1 = 2;

						if (player->charability == CA_SWIM)
							player->fly1 /= 2;

						// Slow down!
						if (player->speed > FixedMul(8*FRACUNIT, player->mo->scale) && player->speed > FixedMul(player->normalspeed>>1, player->mo->scale))
							P_Thrust(player->mo, R_PointToAngle2(0,0,player->mo->momx,player->mo->momy), FixedMul(-4*FRACUNIT, player->mo->scale));
					}
					break;
				default:
					break;
			}
		}
		else if ((player->powers[pw_shield] & SH_NOSTACK) == SH_JUMP && !player->powers[pw_super])
			P_DoJumpShield(player);
	}

	if (cmd->buttons & BT_JUMP)
	{
		player->pflags |= PF_JUMPDOWN;

		if ((gametype != GT_CTF || !player->gotflag) && !player->exiting)
		{
			if (player->secondjump == 1)
			{
				if (player->charability == CA_FLOAT)
					player->mo->momz = 0;
				else if (player->charability == CA_SLOWFALL)
				{
					if (player->powers[pw_super])
					{
						if (P_MobjFlip(player->mo)*player->mo->momz < gravity*16)
						player->mo->momz = P_MobjFlip(player->mo)*gravity*16; //Float upward 4x as fast while super.
					}
					else if (P_MobjFlip(player->mo)*player->mo->momz < -gravity*4)
						player->mo->momz = P_MobjFlip(player->mo)*-gravity*4;
				}
				player->pflags &= ~PF_SPINNING;
			}
		}
	}
	else // If not pressing the jump button
	{
		player->pflags &= ~PF_JUMPDOWN;

		// Repeat abilities, but not double jump!
		if ((player->charability2 == CA2_MULTIABILITY && player->charability != CA_DOUBLEJUMP)
			|| (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds])))
			player->secondjump = 0;
		else if (player->charability == CA_FLOAT && player->secondjump == 1)
			player->secondjump = 2;


		// If letting go of the jump button while still on ascent, cut the jump height.
		if (player->pflags & PF_JUMPED && P_MobjFlip(player->mo)*player->mo->momz > 0 && player->jumping == 1)
		{
			player->mo->momz >>= 1;
			player->jumping = 0;
		}
	}
}

boolean P_AnalogMove(player_t *player)
{
	return player->pflags & PF_ANALOGMODE;
}

//
// P_GetPlayerControlDirection
//
// Determines if the player is pressing in the direction they are moving
//
// 0 = no controls pressed/no movement
// 1 = pressing in the direction of movement
// 2 = pressing in the opposite direction of movement
//
INT32 P_GetPlayerControlDirection(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	angle_t controllerdirection, controlplayerdirection;
	camera_t *thiscam;
	angle_t dangle;
	fixed_t tempx = 0, tempy = 0;
	angle_t tempangle, origtempangle;

	if (splitscreen && player == &players[secondarydisplayplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	if (!cmd->forwardmove && !cmd->sidemove)
		return 0;

	if (!player->mo->momx && !player->mo->momy)
		return 0;

	if (twodlevel || player->mo->flags2 & MF2_TWOD)
	{
		if (!cmd->sidemove)
			return 0;
		if (!player->mo->momx)
			return 0;
		origtempangle = tempangle = 0; // relative to the axis rather than the player!
		controlplayerdirection = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	}
	else if (P_AnalogMove(player) && thiscam->chase)
	{
		if (player->awayviewtics)
			origtempangle = tempangle = player->awayviewmobj->angle;
		else
			origtempangle = tempangle = thiscam->angle;
		controlplayerdirection = player->mo->angle;
	}
	else
	{
		origtempangle = tempangle = player->mo->angle;
		controlplayerdirection = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);
	}

	// Calculate the angle at which the controls are pointing
	// to figure out the proper mforward and mbackward.
	tempangle >>= ANGLETOFINESHIFT;
	if (!(twodlevel || player->mo->flags2 & MF2_TWOD)) // in 2d mode, sidemove is treated as the forwards/backwards direction
	{
		tempx += FixedMul(cmd->forwardmove*FRACUNIT,FINECOSINE(tempangle));
		tempy += FixedMul(cmd->forwardmove*FRACUNIT,FINESINE(tempangle));

		tempangle = origtempangle-ANGLE_90;
		tempangle >>= ANGLETOFINESHIFT;
	}
	tempx += FixedMul(cmd->sidemove*FRACUNIT,FINECOSINE(tempangle));
	tempy += FixedMul(cmd->sidemove*FRACUNIT,FINESINE(tempangle));

	controllerdirection = R_PointToAngle2(0, 0, tempx, tempy);

	dangle = controllerdirection - controlplayerdirection;

	if (dangle > ANGLE_180) //flip to keep to one side
		dangle = InvAngle(dangle);

	if (dangle > ANGLE_90)
		return 2; // Controls pointing backwards from player
	else
		return 1; // Controls pointing in player's general direction
}

// Control scheme for 2d levels.
static void P_2dMovement(player_t *player)
{
	ticcmd_t *cmd;
	INT32 topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0;
	angle_t movepushangle = 0;
	fixed_t normalspd = FixedMul(player->normalspeed, player->mo->scale);

	cmd = &player->cmd;

	if (player->exiting || player->pflags & PF_STASIS)
	{
		cmd->forwardmove = cmd->sidemove = 0;
		if (player->pflags & PF_GLIDING)
		{
			if (!player->skidtime)
				player->pflags &= ~PF_GLIDING;
			else if (player->exiting)
			{
				player->pflags &= ~PF_GLIDING;
				P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
				player->skidtime = 0;
			}
		}
		if (player->pflags & PF_SPINNING && !player->exiting)
		{
			player->pflags &= ~PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}
	}

	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 2) // Wind/Current
	{
		//if (player->mo->z > player->mo->watertop || player->mo->z + player->mo->height < player->mo->waterbottom)
		if (!(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
			player->cmomx = player->cmomy = 0;
	}
	else if (player->onconveyor == 4 && !P_IsObjectOnGround(player->mo)) // Actual conveyor belt
		player->cmomx = player->cmomy = 0;
	else if (player->onconveyor != 2 && player->onconveyor != 4
#ifdef POLYOBJECTS
				&& player->onconveyor != 1
#endif
	)
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on absolute-value-of-a-number formula
	player->speed = abs(player->rmomx);

	if (player->pflags & PF_GLIDING)
	{
		// Angle fix.
		if (player->mo->angle < ANGLE_180 && player->mo->angle > ANGLE_90)
			player->mo->angle = ANGLE_180;
		else if (player->mo->angle < ANGLE_90 && player->mo->angle > 0)
			player->mo->angle = 0;

		if (cmd->sidemove > 0 && player->mo->angle != 0 && player->mo->angle >= ANGLE_180)
			player->mo->angle += 1280<<FRACBITS;
		else if (cmd->sidemove < 0 && player->mo->angle != ANGLE_180 && (player->mo->angle > ANGLE_180 || player->mo->angle == 0))
			player->mo->angle -= 1280<<FRACBITS;
		else if (cmd->sidemove == 0)
		{
			if (player->mo->angle >= ANGLE_270)
				player->mo->angle += 1280<<FRACBITS;
			else if (player->mo->angle < ANGLE_270 && player->mo->angle > ANGLE_180)
				player->mo->angle -= 1280<<FRACBITS;
		}
	}
	else if (cmd->sidemove && !(player->climbing) && !P_PlayerInPain(player))
	{
		if (cmd->sidemove > 0)
			player->mo->angle = 0;
		else if (cmd->sidemove < 0)
			player->mo->angle = ANGLE_180;
	}

	if (player == &players[consoleplayer])
		localangle = player->mo->angle;
	else if (player == &players[secondarydisplayplayer])
		localangle2 = player->mo->angle;

	if (player->pflags & PF_GLIDING)
		movepushangle = player->mo->angle;
	else
	{
		if (cmd->sidemove > 0)
			movepushangle = 0;
		else if (cmd->sidemove < 0)
			movepushangle = ANGLE_180;
		else
			movepushangle = player->mo->angle;
	}

	// Do not let the player control movement if not onground.
	onground = P_IsObjectOnGround(player->mo);

	player->aiming = cmd->aiming<<FRACBITS;

	// Set the player speeds.
	if (maptol & TOL_2D)
		normalspd = FixedMul(normalspd, 2*FRACUNIT/3);

	if (player->powers[pw_super] || player->powers[pw_sneakers])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart/2 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/2;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER) && !(player->pflags & PF_SLIDING))
		{
			topspeed = normalspd;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd * 2;
	}
	else
	{
		thrustfactor = player->thrustfactor;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd/2;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER) && !(player->pflags & PF_SLIDING))
		{
			topspeed = normalspd/2;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd;
	}

//////////////////////////////////////
	if (player->climbing)
	{
		if (cmd->forwardmove != 0)
			P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT,10*FRACUNIT), false);

		if (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]))
			player->mo->momz *= 2;

		player->mo->momx = 0;
	}
	else if (cmd->sidemove != 0 && !(player->pflags & PF_GLIDING || player->exiting
		|| (P_PlayerInPain(player) && !onground)))
	{
		movepushforward = abs(cmd->sidemove) * (thrustfactor * acceleration);

		// allow very small movement while in air for gameplay
		if (!onground)
			movepushforward >>= 1; // Proper air movement

		// Allow a bit of movement while spinning
		if (player->pflags & PF_SPINNING)
		{
			if (!(player->pflags & PF_STARTDASH))
				movepushforward = movepushforward/48;
			else
				movepushforward = 0;
		}

		movepushforward = FixedMul(movepushforward, player->mo->scale);
		if (player->rmomx < topspeed && cmd->sidemove > 0) // Sonic's Speed
			P_Thrust(player->mo, movepushangle, movepushforward);
		else if (player->rmomx > -topspeed && cmd->sidemove < 0)
			P_Thrust(player->mo, movepushangle, movepushforward);
	}
}

//#define OLD_MOVEMENT_CODE 1
static void P_3dMovement(player_t *player)
{
	ticcmd_t *cmd;
	angle_t movepushangle, movepushsideangle; // Analog
	INT32 topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0, movepushside = 0;
	INT32 mforward = 0, mbackward = 0;
	angle_t dangle; // replaces old quadrants bits
	fixed_t normalspd = FixedMul(player->normalspeed, player->mo->scale);
	boolean analogmove = false;
	fixed_t oldMagnitude, newMagnitude;
#ifdef ESLOPE
	vector3_t totalthrust;

	totalthrust.x = totalthrust.y = 0; // I forget if this is needed
	totalthrust.z = FRACUNIT*P_MobjFlip(player->mo)/3; // A bit of extra push-back on slopes
#endif // ESLOPE

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	analogmove = P_AnalogMove(player);

	cmd = &player->cmd;

	if (player->exiting || player->pflags & PF_STASIS)
	{
		cmd->forwardmove = cmd->sidemove = 0;
		if (player->pflags & PF_GLIDING)
		{
			if (!player->skidtime)
				player->pflags &= ~PF_GLIDING;
			else if (player->exiting)
			{
				player->pflags &= ~PF_GLIDING;
				P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
				player->skidtime = 0;
			}
		}
		if (player->pflags & PF_SPINNING && !player->exiting)
		{
			player->pflags &= ~PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}
	}

	if (analogmove)
	{
		movepushangle = (cmd->angleturn<<16 /* not FRACBITS */);
	}
	else
	{
		movepushangle = player->mo->angle;
	}
	movepushsideangle = movepushangle-ANGLE_90;

	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 2) // Wind/Current
	{
		//if (player->mo->z > player->mo->watertop || player->mo->z + player->mo->height < player->mo->waterbottom)
		if (!(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
			player->cmomx = player->cmomy = 0;
	}
	else if (player->onconveyor == 4 && !P_IsObjectOnGround(player->mo)) // Actual conveyor belt
		player->cmomx = player->cmomy = 0;
	else if (player->onconveyor != 2 && player->onconveyor != 4
#ifdef POLYOBJECTS
				&& player->onconveyor != 1
#endif
	)
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on distance-of-a-line formula
	player->speed = P_AproxDistance(player->rmomx, player->rmomy);

	// Monster Iestyn - 04-11-13
	// Quadrants are stupid, excessive and broken, let's do this a much simpler way!
	// Get delta angle from rmom angle and player angle first
	dangle = R_PointToAngle2(0,0, player->rmomx, player->rmomy) - player->mo->angle;
	if (dangle > ANGLE_180) //flip to keep to one side
		dangle = InvAngle(dangle);

	// now use it to determine direction!
	if (dangle <= ANGLE_45) // angles 0-45 or 315-360
		mforward = 1; // going forwards
	else if (dangle >= ANGLE_135) // angles 135-225
		mbackward = 1; // going backwards

	// anything else will leave both at 0, so no need to do anything else

	// When sliding, don't allow forward/back
	if (player->pflags & PF_SLIDING)
		cmd->forwardmove = 0;

	// Do not let the player control movement if not onground.
	onground = P_IsObjectOnGround(player->mo);

	player->aiming = cmd->aiming<<FRACBITS;

	// Set the player speeds.
	if (player->pflags & PF_SLIDING)
	{
		normalspd = FixedMul(36<<FRACBITS, player->mo->scale);
		thrustfactor = 5;
		acceleration = 96 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * 40;
		topspeed = normalspd;
	}
	else if (player->bot)
	{ // Bot steals player 1's stats
		normalspd = FixedMul(players[consoleplayer].normalspeed, player->mo->scale);
		thrustfactor = players[consoleplayer].thrustfactor;
		acceleration = players[consoleplayer].accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * players[consoleplayer].acceleration;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd/2;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER))
		{
			topspeed = normalspd/2;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd;
	}
	else if (player->powers[pw_super] || player->powers[pw_sneakers])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart/2 + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration/2;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER))
		{
			topspeed = normalspd;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd * 2;
	}
	else
	{
		thrustfactor = player->thrustfactor;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;

		if (player->powers[pw_tailsfly])
			topspeed = normalspd/2;
		else if (player->mo->eflags & (MFE_UNDERWATER|MFE_GOOWATER))
		{
			topspeed = normalspd/2;
			acceleration = 2*acceleration/3;
		}
		else
			topspeed = normalspd;
	}

	// Better maneuverability while flying
	if(player->powers[pw_tailsfly])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart + (FixedDiv(player->speed, player->mo->scale)>>FRACBITS) * player->acceleration;
	}

	// Forward movement
	if (player->climbing)
	{
		if (cmd->forwardmove)
		{
			P_SetObjectMomZ(player->mo, FixedDiv(cmd->forwardmove*FRACUNIT, 10*FRACUNIT), false);
			if (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]))
				player->mo->momz *= 2;
		}
	}
	else if (!analogmove
		&& cmd->forwardmove != 0 && !(player->pflags & PF_GLIDING || player->exiting
		|| (P_PlayerInPain(player) && !onground)))
	{
		movepushforward = cmd->forwardmove * (thrustfactor * acceleration);

		// allow very small movement while in air for gameplay
		if (!onground)
			movepushforward >>= 2; // proper air movement

		// Allow a bit of movement while spinning
		if (player->pflags & PF_SPINNING)
		{
			if ((mforward && cmd->forwardmove > 0) || (mbackward && cmd->forwardmove < 0))
				movepushforward = 0;
			else if (!(player->pflags & PF_STARTDASH))
				movepushforward = FixedDiv(movepushforward, 16*FRACUNIT);
			else
				movepushforward = 0;
		}

		movepushforward = FixedMul(movepushforward, player->mo->scale);

#ifdef ESLOPE
		totalthrust.x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
#else
		P_Thrust(player->mo, movepushangle, movepushforward);
#endif
	}
	// Sideways movement
	if (player->climbing)
	{
		if (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]))
			P_InstaThrust(player->mo, player->mo->angle-ANGLE_90, FixedMul(FixedDiv(cmd->sidemove*FRACUNIT, 5*FRACUNIT), player->mo->scale));
		else
			P_InstaThrust(player->mo, player->mo->angle-ANGLE_90, FixedMul(FixedDiv(cmd->sidemove*FRACUNIT, 10*FRACUNIT), player->mo->scale));
	}
	// Analog movement control
	else if (analogmove)
	{
		if (!(player->pflags & PF_GLIDING || player->exiting || P_PlayerInPain(player)))
		{
			angle_t controldirection;

			// Calculate the angle at which the controls are pointing
			// to figure out the proper mforward and mbackward.
			// (Why was it so complicated before? ~Red)
			controldirection = R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT)+movepushangle;

			movepushforward = max(abs(cmd->sidemove), abs(cmd->forwardmove)) * (thrustfactor * acceleration);

			// allow very small movement while in air for gameplay
			if (!onground)
				movepushforward >>= 2; // proper air movement

			// Allow a bit of movement while spinning
			if (player->pflags & PF_SPINNING)
			{
				// Stupid little movement prohibitor hack
				// that REALLY shouldn't belong in analog code.
				if ((mforward && cmd->forwardmove > 0) || (mbackward && cmd->forwardmove < 0))
					movepushforward = 0;
				else if (!(player->pflags & PF_STARTDASH))
					movepushforward = FixedDiv(movepushforward, 16*FRACUNIT);
				else
					movepushforward = 0;
			}

			movepushsideangle = controldirection;

			movepushforward = FixedMul(movepushforward, player->mo->scale);

#ifdef ESLOPE
			totalthrust.x += P_ReturnThrustX(player->mo, controldirection, movepushforward);
			totalthrust.y += P_ReturnThrustY(player->mo, controldirection, movepushforward);
#else
			P_Thrust(player->mo, controldirection, movepushforward);
#endif
		}
	}
	else if (cmd->sidemove && !(player->pflags & PF_GLIDING) && !player->exiting && !P_PlayerInPain(player))
	{
		movepushside = cmd->sidemove * (thrustfactor * acceleration);

		if (!onground)
		{
			movepushside >>= 2;

			// Reduce movepushslide even more if over "max" flight speed
			if (player->powers[pw_tailsfly] && player->speed > topspeed)
				movepushside >>= 2;
		}

		// Allow a bit of movement while spinning
		if (player->pflags & PF_SPINNING)
		{
			if (!(player->pflags & PF_STARTDASH))
				movepushside = FixedDiv(movepushside,16*FRACUNIT);
			else
				movepushside = 0;
		}

		// Finally move the player now that his speed/direction has been decided.
		movepushside = FixedMul(movepushside, player->mo->scale);

#ifdef ESLOPE
		totalthrust.x += P_ReturnThrustX(player->mo, movepushsideangle, movepushside);
		totalthrust.y += P_ReturnThrustY(player->mo, movepushsideangle, movepushside);
#else
		P_Thrust(player->mo, movepushsideangle, movepushside);
#endif
	}

#ifdef ESLOPE
	if ((totalthrust.x || totalthrust.y)
		&& player->mo->standingslope && (!(player->mo->standingslope->flags & SL_NOPHYSICS)) && abs(player->mo->standingslope->zdelta) > FRACUNIT/2) {
		// Factor thrust to slope, but only for the part pushing up it!
		// The rest is unaffected.
		angle_t thrustangle = R_PointToAngle2(0, 0, totalthrust.x, totalthrust.y)-player->mo->standingslope->xydirection;

		if (player->mo->standingslope->zdelta < 0) { // Direction goes down, so thrustangle needs to face toward
			if (thrustangle < ANGLE_90 || thrustangle > ANGLE_270) {
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		} else { // Direction goes up, so thrustangle needs to face away
			if (thrustangle > ANGLE_90 && thrustangle < ANGLE_270) {
				P_QuantizeMomentumToSlope(&totalthrust, player->mo->standingslope);
			}
		}
	}

	player->mo->momx += totalthrust.x;
	player->mo->momy += totalthrust.y;
#endif

	// Time to ask three questions:
	// 1) Are we over topspeed?
	// 2) If "yes" to 1, were we moving over topspeed to begin with?
	// 3) If "yes" to 2, are we now going faster?

	// If "yes" to 3, normalize to our initial momentum; this will allow thoks to stay as fast as they normally are.
	// If "no" to 3, ignore it; the player might be going too fast, but they're slowing down, so let them.
	// If "no" to 2, normalize to topspeed, so we can't suddenly run faster than it of our own accord.
	// If "no" to 1, we're not reaching any limits yet, so ignore this entirely!
	// -Shadow Hog
	newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
	if (newMagnitude > topspeed)
	{
		fixed_t tempmomx, tempmomy;
		if (oldMagnitude > topspeed)
		{
			if (newMagnitude > oldMagnitude)
			{
				tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), oldMagnitude);
				tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), oldMagnitude);
				player->mo->momx = tempmomx + player->cmomx;
				player->mo->momy = tempmomy + player->cmomy;
			}
			// else do nothing
		}
		else
		{
			tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), topspeed);
			tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), topspeed);
			player->mo->momx = tempmomx + player->cmomx;
			player->mo->momy = tempmomy + player->cmomy;
		}
	}
}

//
// P_SpectatorMovement
//
// Control for spectators in multiplayer
//
static void P_SpectatorMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	player->mo->angle = (cmd->angleturn<<16 /* not FRACBITS */);

	ticruned++;
	if (!(cmd->angleturn & TICCMD_RECEIVED))
		ticmiss++;

	if (player->mo->z > player->mo->ceilingz - player->mo->height)
		player->mo->z = player->mo->ceilingz - player->mo->height;
	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	if (cmd->buttons & BT_JUMP)
		player->mo->z += FRACUNIT*16;
	else if (cmd->buttons & BT_USE)
		player->mo->z -= FRACUNIT*16;

	// Aiming needed for SEENAMES, etc.
	// We may not need to fire as a spectator, but this is still handy!
	player->aiming = cmd->aiming<<FRACBITS;

	player->mo->momx = player->mo->momy = player->mo->momz = 0;
	if (cmd->forwardmove != 0)
	{
		P_Thrust(player->mo, player->mo->angle, cmd->forwardmove*(FRACUNIT/2));

		// Quake-style flying spectators :D
		player->mo->momz += FixedMul(cmd->forwardmove*(FRACUNIT/2), AIMINGTOSLOPE(player->aiming));
	}
	if (cmd->sidemove != 0)
	{
		P_Thrust(player->mo, player->mo->angle-ANGLE_90, cmd->sidemove*(FRACUNIT/2));
	}
}

//
// P_ShootLine
//
// Fun and fancy
// graphical indicator
// for building/debugging
// NiGHTS levels!
static void P_ShootLine(mobj_t *source, mobj_t *dest, fixed_t height)
{
	mobj_t *mo;
	INT32 i;
	fixed_t temp;
	INT32 speed, seesound;

	temp = dest->z;
	dest->z = height;

	seesound = mobjinfo[MT_REDRING].seesound;
	speed = mobjinfo[MT_REDRING].speed;
	mobjinfo[MT_REDRING].seesound = sfx_None;
	mobjinfo[MT_REDRING].speed = 20*FRACUNIT;

	mo = P_SpawnXYZMissile(source, dest, MT_REDRING, source->x, source->y, height);

	dest->z = temp;
	if (mo)
	{
		mo->flags2 |= MF2_RAILRING;
		mo->flags2 |= MF2_DONTDRAW;
		mo->flags |= MF_NOCLIPHEIGHT;
		mo->flags |= MF_NOCLIP;
		mo->flags &= ~MF_MISSILE;
		mo->fuse = 3;
	}

	for (i = 0; i < 32; i++)
	{
		if (mo)
		{
			if (!(mo->flags & MF_NOBLOCKMAP))
			{
				P_UnsetThingPosition(mo);
				mo->flags |= MF_NOBLOCKMAP;
				P_SetThingPosition(mo);
			}
			if (i&1)
				P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

			P_UnsetThingPosition(mo);
			mo->x += mo->momx;
			mo->y += mo->momy;
			mo->z += mo->momz;
			P_SetThingPosition(mo);
		}
		else
		{
			mobjinfo[MT_REDRING].seesound = seesound;
			mobjinfo[MT_REDRING].speed = speed;
			return;
		}
	}
	mobjinfo[MT_REDRING].seesound = seesound;
	mobjinfo[MT_REDRING].speed = speed;
}

#define MAXDRILLSPEED 14000
#define MAXNORMALSPEED 6250 //6000

static void P_NightsTransferPoints(player_t *player, fixed_t xspeed, fixed_t radius)
{
	if (player->pflags & PF_TRANSFERTOCLOSEST)
	{
		const angle_t fa = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
		P_InstaThrust(player->mo, fa, xspeed/10);
	}
	else
	{
		const angle_t fa = player->angle_pos>>ANGLETOFINESHIFT;
		const angle_t faold = player->old_angle_pos>>ANGLETOFINESHIFT;
		player->mo->momx = FixedMul(FINECOSINE(fa),radius) - FixedMul(FINECOSINE(faold),radius);
		player->mo->momy = FixedMul(FINESINE(fa),radius) - FixedMul(FINESINE(faold),radius);
	}

	if (player->exiting)
		return;

	// You're welcome, Rob. (Now with slightly less horrendous hacking  -Red
	player->mo->tracer->flags &= ~MF_NOCLIP;
	player->mo->tracer->z = player->mo->z;
	if (!P_TryMove(player->mo->tracer, player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, true)) {
		player->mo->tracer->flags |= MF_NOCLIP;
		return;
	}
	player->mo->tracer->flags |= MF_NOCLIP;
	{
		const INT32 sequence = player->mo->target->threshold;
		mobj_t *transfer1 = NULL;
		mobj_t *transfer2 = NULL;
		mobj_t *axis;
		mobj_t *mo2;
		thinker_t *th;
		line_t transfer1line;
		line_t transfer2line;
		boolean transfer1last = false;
		boolean transfer2last = false;
		vertex_t vertices[4];
		fixed_t truexspeed = xspeed*(!(player->pflags & PF_TRANSFERTOCLOSEST) && player->mo->target->flags & MF_AMBUSH ? -1 : 1);

		// Find next waypoint
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
				continue;

			mo2 = (mobj_t *)th;

			// Axis things are only at beginning of list.
			if (!(mo2->flags2 & MF2_AXIS))
				break;

			if ((mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE)
				&& mo2->threshold == sequence)
			{
				if (player->pflags & PF_TRANSFERTOCLOSEST)
				{
					if (mo2->health == player->axis1->health)
						transfer1 = mo2;
					else if (mo2->health == player->axis2->health)
						transfer2 = mo2;
				}
				else
				{
					if (mo2->health == player->mo->target->health)
						transfer1 = mo2;
					else if (mo2->health == player->mo->target->health + 1)
						transfer2 = mo2;
				}
			}
		}

		// It might be possible that one wasn't found.
		// Is it because we're at the end of the track?
		// Look for a wrapper point.
		if (!transfer1)
		{
			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
					continue;

				mo2 = (mobj_t *)th;

				// Axis things are only at beginning of list.
				if (!(mo2->flags2 & MF2_AXIS))
					break;

				if (mo2->threshold == sequence && (mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
				{
					if (!transfer1)
					{
						transfer1 = mo2;
						transfer1last = true;
					}
					else if (mo2->health > transfer1->health)
					{
						transfer1 = mo2;
						transfer1last = true;
					}
				}
			}
		}
		if (!transfer2)
		{
			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
					continue;

				mo2 = (mobj_t *)th;

				// Axis things are only at beginning of list.
				if (!(mo2->flags2 & MF2_AXIS))
					break;

				if (mo2->threshold == sequence && (mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
				{
					if (!transfer2)
					{
						transfer2 = mo2;
						transfer2last = true;
					}
					else if (mo2->health > transfer2->health)
					{
						transfer2 = mo2;
						transfer2last = true;
					}
				}
			}
		}

		if (!(transfer1 && transfer2)) // We can't continue...
			I_Error("Mare does not form a complete circuit!\n");

		transfer1line.v1 = &vertices[0];
		transfer1line.v2 = &vertices[1];
		transfer2line.v1 = &vertices[2];
		transfer2line.v2 = &vertices[3];

		if (cv_debug && (leveltime % TICRATE == 0))
		{
			CONS_Debug(DBG_NIGHTS, "Transfer1 : %d\n", transfer1->health);
			CONS_Debug(DBG_NIGHTS, "Transfer2 : %d\n", transfer2->health);
		}
		//CONS_Debug(DBG_NIGHTS, "Xspeed : %d", truexspeed);

		//CONS_Debug(DBG_NIGHTS, "T1 is at %d, %d\n", transfer1->x>>FRACBITS, transfer1->y>>FRACBITS);
		//CONS_Debug(DBG_NIGHTS, "T2 is at %d, %d\n", transfer2->x>>FRACBITS, transfer2->y>>FRACBITS);
		//CONS_Debug(DBG_NIGHTS, "Distance from T1: %d\n", P_AproxDistance(transfer1->x - player->mo->x, transfer1->y - player->mo->y)>>FRACBITS);
		//CONS_Debug(DBG_NIGHTS, "Distance from T2: %d\n", P_AproxDistance(transfer2->x - player->mo->x, transfer2->y - player->mo->y)>>FRACBITS);

		// Transfer1 is closer to the player than transfer2
		if (P_AproxDistance(transfer1->x - player->mo->x, transfer1->y - player->mo->y)>>FRACBITS
			< P_AproxDistance(transfer2->x - player->mo->x, transfer2->y - player->mo->y)>>FRACBITS)
		{
			//CONS_Debug(DBG_NIGHTS, " must be < 0 to transfer\n");

			if (transfer1->type == MT_AXISTRANSFERLINE)
			{
				if (transfer1last)
					axis = P_FindAxis(transfer1->threshold, transfer1->health-2);
				else if (player->pflags & PF_TRANSFERTOCLOSEST)
					axis = P_FindAxis(transfer1->threshold, transfer1->health-1);
				else
					axis = P_FindAxis(transfer1->threshold, transfer1->health);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #1\n");
					return;
				}

				//CONS_Debug(DBG_NIGHTS, "Drawing a line from %d to ", axis->health);

				transfer1line.v1->x = axis->x;
				transfer1line.v1->y = axis->y;

				transfer1line.v2->x = transfer1->x;
				transfer1line.v2->y = transfer1->y;

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(axis, transfer1, player->mo->z);

				//CONS_Debug(DBG_NIGHTS, "closest %d\n", transfer1->health);

				transfer1line.dx = transfer1line.v2->x - transfer1line.v1->x;
				transfer1line.dy = transfer1line.v2->y - transfer1line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer1line)
						!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer1line)
						&& truexspeed < 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->pflags & PF_TRANSFERTOCLOSEST)
					{
						player->pflags &= ~PF_TRANSFERTOCLOSEST;
						P_TransferToAxis(player, transfer1->health - 1);
					}
					else
					{
						player->pflags |= PF_TRANSFERTOCLOSEST;
						P_SetTarget(&player->axis2, transfer1);
						P_SetTarget(&player->axis1, P_FindAxisTransfer(transfer1->threshold, transfer1->health-1, MT_AXISTRANSFERLINE));//P_FindAxis(transfer1->threshold, axis->health-2);
					}
				}
			}
			else
			{
				// Transfer1
				if (transfer1last)
					axis = P_FindAxis(transfer1->threshold, 1);
				else
					axis = P_FindAxis(transfer1->threshold, transfer1->health);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #2\n");
					return;
				}

				//CONS_Debug(DBG_NIGHTS, "Drawing a line from %d to ", axis->health);

				transfer1line.v1->x = axis->x;
				transfer1line.v1->y = axis->y;

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(transfer1, P_FindAxis(transfer1->threshold, transfer1->health-1), player->mo->z);

				//axis = P_FindAxis(transfer1->threshold, transfer1->health-1);

				//CONS_Debug(DBG_NIGHTS, "%d\n", axis->health);

				transfer1line.v2->x = transfer1->x;
				transfer1line.v2->y = transfer1->y;

				transfer1line.dx = transfer1line.v2->x - transfer1line.v1->x;
				transfer1line.dy = transfer1line.v2->y - transfer1line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer1line)
					!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer1line)
						&& truexspeed < 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->mo->target->health < transfer1->health)
					{
						// Find the next axis with a ->health
						// +1 from the current axis.
						if (transfer1last)
							P_TransferToAxis(player, transfer1->health - 1);
						else
							P_TransferToAxis(player, transfer1->health);
					}
					else if (player->mo->target->health >= transfer1->health)
					{
						// Find the next axis with a ->health
						// -1 from the current axis.
						P_TransferToAxis(player, transfer1->health - 1);
					}
				}
			}
		}
		else
		{
			//CONS_Debug(DBG_NIGHTS, " must be > 0 to transfer\n");
			if (transfer2->type == MT_AXISTRANSFERLINE)
			{
				if (transfer2last)
					axis = P_FindAxis(transfer2->threshold, 1);
				else if (player->pflags & PF_TRANSFERTOCLOSEST)
					axis = P_FindAxis(transfer2->threshold, transfer2->health);
				else
					axis = P_FindAxis(transfer2->threshold, transfer2->health - 1);

				if (!axis)
					axis = P_FindAxis(transfer2->threshold, 1);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #3\n");
					return;
				}

				//CONS_Debug(DBG_NIGHTS, "Drawing a line from %d to ", axis->health);

				transfer2line.v1->x = axis->x;
				transfer2line.v1->y = axis->y;

				transfer2line.v2->x = transfer2->x;
				transfer2line.v2->y = transfer2->y;

				//CONS_Debug(DBG_NIGHTS, "closest %d\n", transfer2->health);

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(axis, transfer2, player->mo->z);

				transfer2line.dx = transfer2line.v2->x - transfer2line.v1->x;
				transfer2line.dy = transfer2line.v2->y - transfer2line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer2line)
						!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer2line)
						&& truexspeed > 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->pflags & PF_TRANSFERTOCLOSEST)
					{
						player->pflags &= ~PF_TRANSFERTOCLOSEST;

						if (!P_FindAxis(transfer2->threshold, transfer2->health))
							transfer2last = true;

						if (transfer2last)
							P_TransferToAxis(player, 1);
						else
							P_TransferToAxis(player, transfer2->health);
					}
					else
					{
						player->pflags |= PF_TRANSFERTOCLOSEST;
						P_SetTarget(&player->axis1, transfer2);
						P_SetTarget(&player->axis2, P_FindAxisTransfer(transfer2->threshold, transfer2->health+1, MT_AXISTRANSFERLINE));//P_FindAxis(transfer2->threshold, axis->health + 2);
					}
				}
			}
			else
			{
				// Transfer2
				if (transfer2last)
					axis = P_FindAxis(transfer2->threshold, 1);
				else
					axis = P_FindAxis(transfer2->threshold, transfer2->health);

				if (!axis)
					axis = P_FindAxis(transfer2->threshold, 1);

				if (!axis)
				{
					CONS_Debug(DBG_NIGHTS, "Unable to find an axis - error code #4\n");
					return;
				}

				//CONS_Debug(DBG_NIGHTS, "Drawing a line from %d to ", axis->health);

				transfer2line.v1->x = axis->x;
				transfer2line.v1->y = axis->y;

				if (cv_debug & DBG_NIGHTS)
					P_ShootLine(transfer2, P_FindAxis(transfer2->threshold, transfer2->health-1), player->mo->z);

				//axis = P_FindAxis(transfer2->threshold, transfer2->health-1);

				//CONS_Debug(DBG_NIGHTS, "%d\n", axis->health);

				transfer2line.v2->x = transfer2->x;
				transfer2line.v2->y = transfer2->y;

				transfer2line.dx = transfer2line.v2->x - transfer2line.v1->x;
				transfer2line.dy = transfer2line.v2->y - transfer2line.v1->y;

				if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer2line)
					!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer2line)
						&& truexspeed > 0)
				{
					if (cv_debug & DBG_NIGHTS)
					{
						HU_SetCEchoDuration(1);
						HU_DoCEcho("transfer!");
						HU_SetCEchoDuration(5);
						S_StartSound(NULL, sfx_strpst);
					}
					if (player->mo->target->health < transfer2->health)
					{
						if (!P_FindAxis(transfer2->threshold, transfer2->health))
							transfer2last = true;

						if (transfer2last)
							P_TransferToAxis(player, 1);
						else
							P_TransferToAxis(player, transfer2->health);
					}
					else if (player->mo->target->health >= transfer2->health)
						P_TransferToAxis(player, transfer2->health - 1);
				}
			}
		}
	}
}

//
// P_DoNiGHTSCapsule
//
// Handles blowing up the Ideya (emerald) capsule for NiGHTS mode
//
static void P_DoNiGHTSCapsule(player_t *player)
{
	INT32 i;

	if ((player->pflags & PF_NIGHTSMODE) && (player->mo->tracer->state < &states[S_NIGHTSHURT1]
		|| player->mo->tracer->state > &states[S_NIGHTSHURT32]))
		P_SetMobjState(player->mo->tracer, S_NIGHTSHURT1);

	if (abs(player->mo->x-player->capsule->x) <= 2*FRACUNIT)
	{
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->capsule->x;
		P_SetThingPosition(player->mo);
		player->mo->momx = 0;
	}

	if (abs(player->mo->y-player->capsule->y) <= 2*FRACUNIT)
	{
		P_UnsetThingPosition(player->mo);
		player->mo->y = player->capsule->y;
		P_SetThingPosition(player->mo);
		player->mo->momy = 0;
	}

	if (abs(player->mo->z - (player->capsule->z+(player->capsule->height/3))) <= 2*FRACUNIT)
	{
		player->mo->z = player->capsule->z+(player->capsule->height/3);
		player->mo->momz = 0;
	}

	if (player->mo->x > player->capsule->x)
		player->mo->momx = -2*FRACUNIT;
	else if (player->mo->x < player->capsule->x)
		player->mo->momx = 2*FRACUNIT;

	if (player->mo->y > player->capsule->y)
		player->mo->momy = -2*FRACUNIT;
	else if (player->mo->y < player->capsule->y)
		player->mo->momy = 2*FRACUNIT;

	if (player->mo->z > player->capsule->z+(player->capsule->height/3))
		player->mo->momz = -2*FRACUNIT;
	else if (player->mo->z < player->capsule->z+(player->capsule->height/3))
		player->mo->momz = 2*FRACUNIT;

	if (G_IsSpecialStage(gamemap))
	{ // In special stages, share rings. Everyone gives up theirs to the capsule player always, because we can't have any individualism here!
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && (&players[i] != player) && players[i].mo->health > 1)
			{
				player->mo->health += players[i].mo->health-1;
				player->health = player->mo->health;
				players[i].mo->health = 1;
				players[i].health = players[i].mo->health;
			}
	}

	// Time to blow it up!
	if (player->mo->x == player->capsule->x
		&& player->mo->y == player->capsule->y
		&& player->mo->z == player->capsule->z+(player->capsule->height/3))
	{
		if (player->mo->health > 1)
		{
			player->mo->health--;
			player->health--;
			player->capsule->health--;
			player->capsule->extravalue1++;

			// Spawn a 'pop' for every 5 rings you deposit
			if (!(player->capsule->extravalue1 % 5))
				S_StartSound(P_SpawnMobj(player->capsule->x + ((P_SignedRandom()/2)<<FRACBITS),
				player->capsule->y + ((P_SignedRandom()/2)<<FRACBITS),
				player->capsule->z + (player->capsule->height/2) + ((P_SignedRandom()/2)<<FRACBITS),
				MT_EXPLODE),sfx_pop);

			if (player->capsule->health <= 0)
			{
				player->capsule->flags &= ~MF_NOGRAVITY;
				player->capsule->momz = 5*FRACUNIT;
				player->capsule->reactiontime = 0;
				player->capsule->extravalue1 = -1;

				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && !player->exiting && players[i].mare == player->mare)
					{
						players[i].bonustime = true;
						players[i].texttimer = 4*TICRATE;
						players[i].textvar = 1; // Time Bonus
						players[i].finishedtime = players[i].nightstime;
						if (!G_IsSpecialStage(gamemap))
							P_AddPlayerScore(&players[i], (players[i].finishedtime/TICRATE) * 100);
						P_FlashPal(&players[i], PAL_WHITE, 8);
					}

				if (G_IsSpecialStage(gamemap))
				{
					// The Chaos Emerald begins to orbit us!
					mobj_t *emmo;
					UINT8 em = P_GetNextEmerald();
					tic_t lowest_time;

					/*for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i] || players[i].spectator || !players[i].mo || !players[i].mo->tracer)
							continue;

						emmo = P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z + players[i].mo->info->height, MT_GOTEMERALD);
						P_SetTarget(&emmo->target, players[i].mo);
						P_SetMobjState(emmo, mobjinfo[MT_GOTEMERALD].meleestate + em);
					}*/

					if (player->mo->tracer)
					{
						// Only give it to ONE person, and THAT player has to get to the goal!
						emmo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->info->height, MT_GOTEMERALD);
						P_SetTarget(&emmo->target, player->mo);
						P_SetMobjState(emmo, mobjinfo[MT_GOTEMERALD].meleestate + em);
						P_SetTarget(&player->mo->tracer->target, emmo);
					}

					// Okay, we're doing this down here because we're handling time weirdly for co-op special stages
					// and because P_AddPlayerScore gives score to everyone in co-op special stages.
					// Find the player with the lowest time remaining and award points based on that time instead.
					lowest_time = player->finishedtime;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i] && players[i].pflags & PF_NIGHTSMODE)
							if (players[i].finishedtime < lowest_time)
								lowest_time = players[i].finishedtime;
					P_AddPlayerScore(player, (lowest_time/TICRATE) * 100);
				}
				else
				{
					fixed_t z;

					z = player->capsule->z + player->capsule->height/2;
					for (i = 0; i < 16; i++)
						P_SpawnMobj(player->capsule->x, player->capsule->y, z, MT_BIRD);
				}
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].mare == player->mare)
						P_SetTarget(&players[i].capsule, NULL); // Remove capsule from everyone now that it is dead!
				S_StartScreamSound(player->mo, sfx_ngdone);
			}
		}
		else
		{
			S_StartScreamSound(player->mo, sfx_lose);
			player->texttimer = 4*TICRATE;
			player->textvar = 3; // Get more rings!
			player->capsule->reactiontime = 0;
			player->capsule->extravalue1 = -1;
		}
	}
	else
		player->capsule->extravalue1 = -1;
}

//
// P_NiGHTSMovement
//
// Movement code for NiGHTS!
//
static void P_NiGHTSMovement(player_t *player)
{
	fixed_t drillamt = 0;
	boolean still = false, moved = false, backwardaxis = false, firstdrill;
	INT16 newangle = 0;
	fixed_t xspeed, yspeed;
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	fixed_t newx, newy, radius;
	angle_t movingangle;
	ticcmd_t *cmd = &player->cmd;
	INT32 thrustfactor;
	INT32 i;
	statenum_t flystate = S_NIGHTSFLY1A;

	player->pflags &= ~PF_DRILLING;

	firstdrill = false;

	if (player->drillmeter > 96*20)
		player->drillmeter = 96*20;

	if (player->drilldelay)
		player->drilldelay--;

	if (!(cmd->buttons & BT_JUMP))
	{
		// Always have just a TINY bit of drill power.
		if (player->drillmeter <= 0)
			player->drillmeter = TICRATE/10;
	}

	if (!player->mo->tracer)
	{
		P_DeNightserizePlayer(player);
		return;
	}

	if (G_IsSpecialStage(gamemap))
	{
		boolean capsule = false;
		// NiGHTS special stages have a pseudo-shared timer, so check if ANYONE is feeding the capsule.
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] /*&& players[i].pflags & PF_NIGHTSMODE*/
			&& (players[i].capsule && players[i].capsule->reactiontime))
				capsule = true;
		if (!capsule
		&& !(player->mo->tracer->state >= &states[S_SUPERTRANS1]
			&& player->mo->tracer->state <= &states[S_SUPERTRANS9])
		&& !player->exiting)
			player->nightstime--;
	}
	else if (gametype != GT_RACE && gametype != GT_COMPETITION
	&& !(player->mo->tracer->state >= &states[S_SUPERTRANS1] && player->mo->tracer->state <= &states[S_SUPERTRANS9])
	&& !(player->capsule && player->capsule->reactiontime)
	&& !player->exiting)
		player->nightstime--;

	if (!player->nightstime)
	{
		P_DeNightserizePlayer(player);
		S_StartScreamSound(player->mo, sfx_s3k66);
//		S_StopSoundByNum(sfx_timeup); // Kill the "out of time" music, if it's playing. Dummied out, as some on the dev team thought it wasn't Sonic-y enough (Mystic, notably). Uncomment to restore. -SH
		P_RestoreMusic(player); // I have my doubts that this is the right place for this...

		return;
	}
	else if (P_IsLocalPlayer(player) && player->nightstime == 10*TICRATE)
//		S_StartSound(NULL, sfx_timeup); // that creepy "out of time" music from NiGHTS. Dummied out, as some on the dev team thought it wasn't Sonic-y enough (Mystic, notably). Uncomment to restore. -SH
		S_ChangeMusicInternal("drown",false);


	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	if (player->mo->z+player->mo->height > player->mo->ceilingz)
		player->mo->z = player->mo->ceilingz - player->mo->height;

	newx = P_ReturnThrustX(player->mo, player->mo->angle, 3*FRACUNIT)+player->mo->x;
	newy = P_ReturnThrustY(player->mo, player->mo->angle, 3*FRACUNIT)+player->mo->y;

	if (!player->mo->target)
	{
		fixed_t dist1, dist2 = 0;

		// scan the thinkers
		// to find the closest axis point
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type == MT_AXIS)
			{
				if (mo2->threshold == player->mare)
				{
					if (closestaxis == NULL)
					{
						closestaxis = mo2;
						dist2 = R_PointToDist2(newx, newy, mo2->x, mo2->y)-mo2->radius;
					}
					else
					{
						dist1 = R_PointToDist2(newx, newy, mo2->x, mo2->y)-mo2->radius;

						if (dist1 < dist2)
						{
							closestaxis = mo2;
							dist2 = dist1;
						}
					}
				}
			}
		}

		P_SetTarget(&player->mo->target, closestaxis);
	}

	if (!player->mo->target) // Uh-oh!
	{
		CONS_Debug(DBG_NIGHTS, "No axis points found!\n");
		return;
	}

	// The 'ambush' flag says you should rotate
	// the other way around the axis.
	if (player->mo->target->flags & MF_AMBUSH)
		backwardaxis = true;

	player->angle_pos = R_PointToAngle2(player->mo->target->x, player->mo->target->y, player->mo->x, player->mo->y);

	player->old_angle_pos = player->angle_pos;

	radius = player->mo->target->radius;

	player->mo->flags |= MF_NOGRAVITY;
	player->mo->flags2 |= MF2_DONTDRAW;
	P_SetScale(player->mo->tracer, player->mo->scale);

	if (player->mo->eflags & MFE_VERTICALFLIP)
		cmd->forwardmove = (SINT8)(-cmd->forwardmove);

	if (!(player->pflags & PF_TRANSFERTOCLOSEST))
	{
		fixed_t realdist = R_PointToDist2(player->mo->x, player->mo->y, player->mo->target->x, player->mo->target->y);
		// teleport player to correct radius if neccessary
		if (realdist>>FRACBITS != radius>>FRACBITS)
		{
			CONS_Debug(DBG_NIGHTS, "Aligning player with axis\n");
			P_UnsetThingPosition(player->mo);
			if (realdist == 0) // other method won't work if we're exactly on the target lol
			{
				const angle_t fa = player->old_angle_pos>>ANGLETOFINESHIFT;
				player->mo->x = player->mo->target->x + FixedMul(FINECOSINE(fa), radius);
				player->mo->y = player->mo->target->y + FixedMul(FINESINE(fa), radius);
			}
			else
			{
				player->mo->x = player->mo->target->x + FixedMul(FixedDiv(player->mo->x - player->mo->target->x, realdist), radius);
				player->mo->y = player->mo->target->y + FixedMul(FixedDiv(player->mo->y - player->mo->target->y, realdist), radius);
			}
			P_SetThingPosition(player->mo);
		}
	}

	// Currently reeling from being hit.
	if (player->powers[pw_flashing] > (2*flashingtics)/3)
	{
		{
			const angle_t fa = (FixedAngle(player->flyangle*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
			const fixed_t speed = FixedDiv(player->speed*FRACUNIT,50*FRACUNIT);

			xspeed = FixedMul(FINECOSINE(fa),speed);
			yspeed = FixedMul(FINESINE(fa),speed);
		}

		if (!(player->pflags & PF_TRANSFERTOCLOSEST))
		{
			xspeed = FixedMul(xspeed, FixedDiv(1024*FRACUNIT, player->mo->target->radius));

			if (backwardaxis)
				xspeed *= -1;

			player->angle_pos += FixedAngleC(FixedDiv(xspeed,5*FRACUNIT),40*FRACUNIT);
		}

		player->mo->momz = 0;

		P_NightsTransferPoints(player, xspeed, radius);
		return;
	}

	if (player->mo->tracer->state >= &states[S_SUPERTRANS1]
		&& player->mo->tracer->state <= &states[S_SUPERTRANS9])
	{
		player->mo->momx = player->mo->momy = player->mo->momz = 0;
		return;
	}

	if (player->exiting > 0 && player->exiting < 2*TICRATE)
	{
		player->mo->momx = player->mo->momy = 0;

		if (gametype != GT_RACE && gametype != GT_COMPETITION)
			P_SetObjectMomZ(player->mo, 30*FRACUNIT, false);

		player->mo->tracer->angle += ANGLE_11hh;

		if (!(player->mo->tracer->state  >= &states[S_NIGHTSDRONE1]
			&& player->mo->tracer->state <= &states[S_NIGHTSDRONE2]))
			P_SetMobjState(player->mo->tracer, S_NIGHTSDRONE1);

		player->mo->tracer->flags |= MF_NOCLIPHEIGHT;
		player->mo->flags |= MF_NOCLIPHEIGHT;
		return;
	}

	// Spawn the little sparkles on each side of the player.
	if (leveltime & 1)
	{
		mobj_t *firstmobj;
		mobj_t *secondmobj;
		fixed_t spawndist = FixedMul(16*FRACUNIT, player->mo->scale);
		fixed_t z = player->mo->z + player->mo->height/2;

		if (player->mo->eflags & MFE_VERTICALFLIP)
			z -= FixedMul(mobjinfo[MT_NIGHTSPARKLE].height, player->mo->scale);

		firstmobj = P_SpawnMobj(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle+ANGLE_90, spawndist), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle+ANGLE_90, spawndist), z, MT_NIGHTSPARKLE);
		secondmobj = P_SpawnMobj(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle-ANGLE_90, spawndist), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle-ANGLE_90, spawndist), z, MT_NIGHTSPARKLE);

		firstmobj->destscale = secondmobj->destscale = player->mo->scale;
		P_SetTarget(&firstmobj->target, player->mo);
		P_SetScale(firstmobj, player->mo->scale);
		P_SetTarget(&secondmobj->target, player->mo);
		P_SetScale(secondmobj, player->mo->scale);

		// Superloop turns sparkles red
		if (player->powers[pw_nights_superloop])
		{
			P_SetMobjState(firstmobj, mobjinfo[MT_NIGHTSPARKLE].seestate);
			P_SetMobjState(secondmobj, mobjinfo[MT_NIGHTSPARKLE].seestate);
		}
	}

	// Paraloop helper is now separate from sparkles
	// It also spawns every tic to avoid failed paraloops
	{
		mobj_t *helpermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height/2, MT_NIGHTSLOOPHELPER);
		helpermobj->fuse = player->mo->fuse = leveltime;
		P_SetTarget(&helpermobj->target, player->mo);
		P_SetScale(helpermobj, player->mo->scale);
	}

	if (player->bumpertime)
	{
		player->jumping = 1;
		player->pflags |= PF_DRILLING;
		newangle = (INT16)player->flyangle;
	}
	else if (cmd->buttons & BT_JUMP && player->drillmeter && player->drilldelay == 0)
	{
		if (!player->jumping)
			firstdrill = true;

		player->jumping = 1;
		player->pflags |= PF_DRILLING;
	}
	else
	{
		player->jumping = 0;

		if (cmd->sidemove != 0)
			moved = true;

		if (player->drillmeter & 1)
			player->drillmeter++; // I'll be nice and give them one.
	}

	if (cmd->forwardmove != 0)
		moved = true;

	if (!player->bumpertime)
	{
		if (moved)
		{
			if (player->pflags & PF_DRILLING)
				drillamt += 100*FRACUNIT;
			else
			{
				const fixed_t fforward = abs(cmd->forwardmove)*FRACUNIT;
				const fixed_t fside = abs(cmd->sidemove)*FRACUNIT;
				const fixed_t dist = FixedHypot(fforward, fside);

				drillamt += dist > 50*FRACUNIT ? 50*FRACUNIT : dist;

				// Accel up from 50 * 2.5
				drillamt = FixedMul(drillamt, 5*FRACUNIT/2);
			}
			if ((player->pflags & PF_DRILLING && player->speed < MAXDRILLSPEED) || player->speed < MAXNORMALSPEED)
				player->speed += FixedInt(drillamt);
		}

		if (player->pflags & PF_DRILLING)
		{
			if (player->speed < MAXDRILLSPEED)
				player->speed += 150;

			if (--player->drillmeter == 0)
				player->drilldelay = TICRATE*2;
		}

		if (player->speed < 0)
			player->speed = 0;

		if (!cmd->forwardmove)
		{
			if (cmd->sidemove > 0)
				newangle = 0;
			else if (cmd->sidemove < 0)
				newangle = 180;
		}
		else if (!cmd->sidemove)
		{
			if (cmd->forwardmove > 0)
				newangle = 90;
			else if (cmd->forwardmove < 0)
				newangle = 270;
		}
		else // AngleFixed(R_PointToAngle2()) results in slight inaccuracy! Don't use it unless movement is on both axises.
			newangle = (INT16)FixedInt(AngleFixed(R_PointToAngle2(0,0, cmd->sidemove*FRACUNIT, cmd->forwardmove*FRACUNIT)));

		if (newangle < 0 && moved)
			newangle = (INT16)(360+newangle);
	}

	if (player->pflags & PF_DRILLING)
		thrustfactor = 2;
	else
	{
		thrustfactor = 8;

		// Decelerate while turning normally.
		if (moved && player->flyangle != newangle && player->speed > 12000)
			player->speed -= 60;
	}

	for (i = 0; i < thrustfactor; i++)
	{
		if (moved && player->flyangle != newangle)
		{
			INT32 anglediff = (((newangle-player->flyangle)+360)%360);
			INT32 angledif2 = (((player->flyangle-newangle)+360)%360);

			// player->flyangle is the one to move
			// newangle is the "move to"
			if (anglediff == 0 && angledif2 == 0)
				break;

			if (anglediff>angledif2)
				player->flyangle--;
			else // if (anglediff<angledif2)
				player->flyangle++;
		}

		// Buff out negatives, >360 angles...
		player->flyangle = ((player->flyangle + 360) % 360);
	}

	if (!(player->speed)
		&& cmd->forwardmove == 0)
		still = true;

	// No more bumper braking
	if (!player->bumpertime
	 && ((cmd->buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)
	  || (cmd->buttons & BT_USE)))
	{
		if (!(player->pflags & PF_SKIDDOWN))
			S_StartSound(player->mo, sfx_ngskid);

		// You can tap the button to only slow down a bit,
		// or hold it to slow to a crawl as before, your choice.
		if (player->speed > 8000)
			player->speed -= 2000;
		else if (player->speed > 1000)
			player->speed -= (player->speed/4);
		else {
			player->speed -= 60;
			if (player->speed < 0)
			{
				player->speed = 0;
				still = true;
			}
		}

		player->pflags |= PF_SKIDDOWN;
	}
	else
		player->pflags &= ~PF_SKIDDOWN;

	{
		const angle_t fa = (FixedAngle(player->flyangle*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
		const fixed_t speed = FixedDiv(player->speed*FRACUNIT,50*FRACUNIT);
		xspeed = FixedMul(FINECOSINE(fa),speed);
		yspeed = FixedMul(FINESINE(fa),speed);
	}

	if (!(player->pflags & PF_TRANSFERTOCLOSEST))
	{
		xspeed = FixedMul(xspeed, FixedDiv(1024*FRACUNIT, player->mo->target->radius));

		if (backwardaxis)
			xspeed *= -1;

		player->angle_pos += FixedAngleC(FixedDiv(xspeed,5*FRACUNIT),40*FRACUNIT);
	}

	P_NightsTransferPoints(player, xspeed, radius);

	if (still)
		player->mo->momz = -FRACUNIT;
	else
		player->mo->momz = yspeed/11;

	if (player->mo->momz > 20*FRACUNIT)
		player->mo->momz = 20*FRACUNIT;
	else if (player->mo->momz < -20*FRACUNIT)
		player->mo->momz = -20*FRACUNIT;

	// You can create splashes as you fly across water.
	if (((!(player->mo->eflags & MFE_VERTICALFLIP)
		&& player->mo->z + P_GetPlayerHeight(player) >= player->mo->watertop && player->mo->z <= player->mo->watertop)
	|| (player->mo->eflags & MFE_VERTICALFLIP
		&& player->mo->z + player->mo->height - P_GetPlayerHeight(player) <= player->mo->waterbottom && player->mo->z + player->mo->height >= player->mo->waterbottom))
	&& player->speed > 9000 && leveltime % (TICRATE/7) == 0 && !player->spectator)
	{
		mobj_t *water = P_SpawnMobj(player->mo->x, player->mo->y,
			((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[MT_SPLISH].height, player->mo->scale) : player->mo->watertop), MT_SPLISH);
		if (player->mo->eflags & MFE_GOOWATER)
			S_StartSound(water, sfx_ghit);
		else
			S_StartSound(water, sfx_wslap);
		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			water->flags2 |= MF2_OBJECTFLIP;
			water->eflags |= MFE_VERTICALFLIP;
		}
		water->destscale = player->mo->scale;
		P_SetScale(water, player->mo->scale);
	}

	if (player->mo->momx || player->mo->momy)
		player->mo->angle = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);

	if (still)
	{
		player->anotherflyangle = 0;
		movingangle = 0;
	}
	else
	{
		INT32 neg = 1;

		// Special cases to prevent the angle from being
		// calculated incorrectly when wrapped.
		if (backwardaxis && (player->old_angle_pos > ANG350 && player->angle_pos < ANG10))
			neg = -1;
		else if (backwardaxis ^ (player->old_angle_pos < ANG10 && player->angle_pos > ANG350))
			neg = -1;
		else if (player->angle_pos > player->old_angle_pos)
			neg = -1;

		movingangle = R_PointToAngle2(0, 0, neg*R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->momz);
		player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
	}

	// NiGHTS flying state
	// Yep, I just ripped out almost 1000 lines of code.
	if      ((player->anotherflyangle >=  12 && player->anotherflyangle <=  33)  // +x +y
	      || (player->anotherflyangle >= 147 && player->anotherflyangle <= 168)) // -x +y
		flystate = S_NIGHTSFLY2A;
	else if ((player->anotherflyangle >=  34 && player->anotherflyangle <=  56)  // +x +y
	      || (player->anotherflyangle >= 124 && player->anotherflyangle <= 146)) // -x +y
		flystate = S_NIGHTSFLY3A;
	else if ((player->anotherflyangle >=  57 && player->anotherflyangle <=  79)  // +x +y
	      || (player->anotherflyangle >= 102 && player->anotherflyangle <= 123)) // -x +y
		flystate = S_NIGHTSFLY4A;
	else if  (player->anotherflyangle >=  80 && player->anotherflyangle <= 101)
		flystate = S_NIGHTSFLY5A;
	else if ((player->anotherflyangle >= 192 && player->anotherflyangle <= 213)  // -x -y
	      || (player->anotherflyangle >= 327 && player->anotherflyangle <= 348)) // +x -y
		flystate = S_NIGHTSFLY6A;
	else if ((player->anotherflyangle >= 214 && player->anotherflyangle <= 236)  // -x -y
	      || (player->anotherflyangle >= 305 && player->anotherflyangle <= 326)) // +x -y
		flystate = S_NIGHTSFLY7A;
	else if ((player->anotherflyangle >= 237 && player->anotherflyangle <= 258)  // -x -y
	      || (player->anotherflyangle >= 282 && player->anotherflyangle <= 304)) // +x -y
		flystate = S_NIGHTSFLY8A;
	else if  (player->anotherflyangle >= 259 && player->anotherflyangle <= 281)
		flystate = S_NIGHTSFLY9A;
	else
		flystate = S_NIGHTSFLY1A;

	if (player->mo->eflags & MFE_VERTICALFLIP)
	{
		if (flystate >= S_NIGHTSFLY2A && flystate <= S_NIGHTSFLY5A)
			flystate += 24; // shift to S_NIGHTSFLY6A
		else if (flystate >= S_NIGHTSFLY6A && flystate <= S_NIGHTSFLY9A)
			flystate -= 24; // shift to S_NIGHTSFLY2A
	}

	if (player->pflags & PF_DRILLING)
	{
		const statenum_t drillstate = flystate + 2;

		if (!(player->mo->tracer->state >= &states[drillstate]
		 && player->mo->tracer->state <= &states[drillstate+4]))
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
			 && player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
			{
				const INT32 framenum = player->mo->tracer->state->frame & 3;

				if (framenum == 3) // Drilld special case
					P_SetMobjStateNF(player->mo->tracer, drillstate);
				else
					P_SetMobjStateNF(player->mo->tracer, drillstate+framenum+1);
			}
			else
				P_SetMobjStateNF(player->mo->tracer, drillstate);
		}
	}
	else
		P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? flystate : flystate+1);

	if (player == &players[consoleplayer])
		localangle = player->mo->angle;
	else if (player == &players[secondarydisplayplayer])
		localangle2 = player->mo->angle;

	if (still)
	{
		P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRONE1);
		player->mo->tracer->angle = player->mo->angle;
	}

	// Check for crushing in our new location
	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height)
		&& !(player->mo->flags & MF_NOCLIP))
	{
		// Flashing won't run out until you STOP being crushed.
		if (player->powers[pw_flashing] == 1)
			player->powers[pw_flashing] = 3;
		else
			P_DamageMobj(player->mo, NULL, NULL, 1);
	}

	if (movingangle >= ANGLE_90 && movingangle <= ANGLE_180)
		movingangle = movingangle - ANGLE_180;
	else if (movingangle >= ANGLE_180 && movingangle <= ANGLE_270)
		movingangle = movingangle - ANGLE_180;
	else if (movingangle >= ANGLE_270)
		movingangle = InvAngle(movingangle);

	if (player == &players[consoleplayer])
		localaiming = movingangle;
	else if (player == &players[secondarydisplayplayer])
		localaiming2 = movingangle;

	player->mo->tracer->angle = player->mo->angle;

	if ((player->pflags & PF_DRILLING) && !player->bumpertime)
	{
		if (firstdrill)
		{
			S_StartSound(player->mo, sfx_drill1);
			player->drilltimer = 32;
		}
		else if (--player->drilltimer <= 0)
		{
			player->drilltimer = 10;
			S_StartSound(player->mo, sfx_drill2);
		}
	}

	if (objectplacing)
		OP_NightsObjectplace(player);
}

// May be used in future for CTF
#if 0
static void P_PlayerDropWeapon(player_t *player)
{
	mobj_t *mo = NULL;

	if (player->powers[pw_homingring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_HOMINGRING);
		player->powers[pw_homingring] = 0;
	}
	else if (player->powers[pw_railring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_RAILRING);
		player->powers[pw_railring] = 0;
	}
	else if (player->powers[pw_automaticring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_AUTOMATICRING);
		player->powers[pw_automaticring] = 0;
	}
	else if (player->powers[pw_explosionring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_EXPLOSIONRING);
		player->powers[pw_explosionring] = 0;
	}
	else if (player->powers[pw_scatterring])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_SCATTERRING);
		player->powers[pw_scatterring] = 0;
	}
	else if (player->powers[pw_grenadering])
	{
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+(60*FRACUNIT), MT_GRENADERING);
		player->powers[pw_grenadering] = 0;
	}

	if (mo)
	{
		player->mo->health--;
		P_InstaThrust(mo, player->mo->angle-ANGLE_180, 8*FRACUNIT);
		P_SetObjectMomZ(mo, 4*FRACUNIT, false);
		mo->flags2 |= MF2_DONTRESPAWN;
		mo->flags &= ~MF_NOGRAVITY;
		mo->flags &= ~MF_NOCLIPHEIGHT;
		mo->fuse = 12*TICRATE;
	}
}
#endif

void P_BlackOw(player_t *player)
{
	INT32 i;
	S_StartSound (player->mo, sfx_bkpoof); // Sound the BANG!

	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && P_AproxDistance(player->mo->x - players[i].mo->x,
			player->mo->y - players[i].mo->y) < 1536*FRACUNIT)
			P_FlashPal(&players[i], PAL_NUKE, 10);

	P_NukeEnemies(player->mo, player->mo, 1536*FRACUNIT); // Search for all nearby enemies and nuke their pants off!
	player->powers[pw_shield] = player->powers[pw_shield] & SH_STACK;
}

void P_ElementalFireTrail(player_t *player)
{
	fixed_t newx;
	fixed_t newy;
	fixed_t ground;
	mobj_t *flame;
	angle_t travelangle;
	INT32 i;

	I_Assert(player != NULL);
	I_Assert(player->mo != NULL);
	I_Assert(!P_MobjWasRemoved(player->mo));

	if (player->mo->eflags & MFE_VERTICALFLIP)
		ground = player->mo->ceilingz - FixedMul(mobjinfo[MT_SPINFIRE].height, player->mo->scale);
	else
		ground = player->mo->floorz;

	travelangle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);

	for (i = 0; i < 2; i++)
	{
		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(24*FRACUNIT, player->mo->scale));
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ((i&1) ? -1 : 1)*ANGLE_135, FixedMul(24*FRACUNIT, player->mo->scale));
#ifdef ESLOPE
		if (player->mo->standingslope)
		{
			ground = P_GetZAt(player->mo->standingslope, newx, newy);
			if (player->mo->eflags & MFE_VERTICALFLIP)
				ground -= FixedMul(mobjinfo[MT_SPINFIRE].height, player->mo->scale);
		}
#endif
		flame = P_SpawnMobj(newx, newy, ground, MT_SPINFIRE);
		P_SetTarget(&flame->target, player->mo);
		flame->angle = travelangle;
		flame->fuse = TICRATE*6;
		flame->destscale = player->mo->scale;
		P_SetScale(flame, player->mo->scale);
		flame->eflags = (flame->eflags & ~MFE_VERTICALFLIP)|(player->mo->eflags & MFE_VERTICALFLIP);

		flame->momx = 8;
		P_XYMovement(flame);
		if (P_MobjWasRemoved(flame))
			continue;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (flame->z + flame->height < flame->ceilingz)
				P_RemoveMobj(flame);
		}
		else if (flame->z > flame->floorz)
			P_RemoveMobj(flame);
	}
}

static void P_SkidStuff(player_t *player)
{
	fixed_t pmx = player->rmomx + player->cmomx;
	fixed_t pmy = player->rmomy + player->cmomy;

	// Knuckles glides into the dirt.
	if (player->pflags & PF_GLIDING && player->skidtime)
	{
		// Fell off a ledge...
		if (!onground)
		{
			player->skidtime = 0;
			player->pflags &= ~(PF_GLIDING|PF_JUMPED);
			P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
		}
		// Get up and brush yourself off, idiot.
		else if (player->glidetime > 15)
		{
			P_ResetPlayer(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
		}
		// Didn't stop yet? Skid FOREVER!
		else if (player->skidtime == 1)
			player->skidtime = 3*TICRATE+1;
		// Spawn a particle every 3 tics.
		else if (!(player->skidtime % 3))
		{
			mobj_t *particle = P_SpawnMobj(player->mo->x + P_RandomRange(-player->mo->radius, player->mo->radius), player->mo->y + P_RandomRange(-player->mo->radius, player->mo->radius),
				player->mo->z + (player->mo->eflags & MFE_VERTICALFLIP ? player->mo->height - mobjinfo[MT_PARTICLE].height : 0),
				MT_PARTICLE);
			particle->tics = 10;

			particle->eflags |= player->mo->eflags & MFE_VERTICALFLIP;
			P_SetScale(particle, player->mo->scale >> 2);
			particle->destscale = player->mo->scale << 2;
			particle->scalespeed = FixedMul(particle->scalespeed, player->mo->scale); // scale the scaling speed!
			P_SetObjectMomZ(particle, FRACUNIT, false);
			S_StartSound(player->mo, sfx_s3k7e); // the proper "Knuckles eats dirt" sfx.
		}
	}
	// Skidding!
	else if (onground && !(player->mo->eflags & MFE_GOOWATER) && !(player->pflags & (PF_JUMPED|PF_SPINNING|PF_SLIDING)) && !(player->charflags & SF_NOSKID))
	{
		if (player->skidtime)
		{
			// Spawn a particle every 3 tics.
			if (!(player->skidtime % 3))
			{
				mobj_t *particle = P_SpawnMobj(player->mo->x, player->mo->y,
					player->mo->z + (player->mo->eflags & MFE_VERTICALFLIP ? player->mo->height - mobjinfo[MT_PARTICLE].height : 0),
					MT_PARTICLE);
				particle->tics = 10;

				particle->eflags |= player->mo->eflags & MFE_VERTICALFLIP;
				P_SetScale(particle, player->mo->scale >> 2);
				particle->destscale = player->mo->scale << 2;
				particle->scalespeed = FixedMul(particle->scalespeed, player->mo->scale); // scale the scaling speed!
				P_SetObjectMomZ(particle, FRACUNIT, false);
			}
		}
		else if (P_AproxDistance(pmx, pmy) >= FixedMul(player->runspeed/2, player->mo->scale) // if you were moving faster than half your run speed last frame
		&& (player->mo->momx != pmx || player->mo->momy != pmy) // and you are moving differently this frame
		&& P_GetPlayerControlDirection(player) == 2) // and your controls are pointing in the opposite direction to your movement
		{ // check for skidding
			angle_t mang = R_PointToAngle2(0,0,pmx,pmy); // movement angle
			angle_t pang = R_PointToAngle2(pmx,pmy,player->mo->momx,player->mo->momy); // push angle
			angle_t dang = mang - pang; // delta angle

			if (dang > ANGLE_180) // Make delta angle always positive, invert it if it's negative.
				dang = InvAngle(dang);

			// If your push angle is more than this close to a full 180 degrees, trigger a skid.
			if (dang > ANGLE_157h)
			{
				player->skidtime = TICRATE/2;
				S_StartSound(player->mo, sfx_skid);
				if (player->panim != PA_WALK)
					P_SetPlayerMobjState(player->mo, S_PLAY_RUN4); // this switches to S_PLAY_SUPERWALK1 for superanims
				player->mo->tics = player->skidtime;
			}
		}
	}
	else {
		if (player->skidtime) {
			player->skidtime = 0;
			S_StopSound(player->mo);
		}
	}
}

//
// P_MovePlayer
static void P_MovePlayer(player_t *player)
{
	ticcmd_t *cmd;
	INT32 i;

	fixed_t runspd;

	if (countdowntimeup)
		return;

	if (player->mo->state >= &states[S_PLAY_SUPERTRANS1] && player->mo->state <= &states[S_PLAY_SUPERTRANS9])
	{
		player->mo->momx = player->mo->momy = player->mo->momz = 0;
		return;
	}

	cmd = &player->cmd;
	runspd = FixedMul(player->runspeed, player->mo->scale);

	// Control relinquishing stuff!
	if (player->powers[pw_ingoop])
		player->pflags |= PF_FULLSTASIS;
	else if (player->pflags & PF_GLIDING && player->skidtime)
		player->pflags |= PF_FULLSTASIS;
	else if (player->powers[pw_nocontrol])
	{
		player->pflags |= PF_STASIS;
		if (!(player->powers[pw_nocontrol] & (1<<15)))
			player->pflags |= PF_JUMPSTASIS;
	}
	// note: don't unset stasis here

	if (!player->spectator && G_TagGametype())
	{
		// If we have stasis already here, it's because it's forced on us
		// by a linedef executor or what have you
		boolean forcestasis = false;

		//During hide time, taggers cannot move.
		if (leveltime < hidetime * TICRATE)
		{
			if (player->pflags & PF_TAGIT)
				forcestasis = true;
		}
		else if (gametype == GT_HIDEANDSEEK)
		{
			if (!(player->pflags & PF_TAGIT))
			{
				forcestasis = true;
				if (player->pflags & PF_TAGGED) // Already hit.
					player->powers[pw_flashing] = 5;
			}
		}

		if (forcestasis)
		{
			player->pflags |= PF_FULLSTASIS;
			// If you're in stasis in tag, you don't drown.
			if (player->powers[pw_underwater] <= 12*TICRATE + 1)
				P_RestoreMusic(player);
			player->powers[pw_underwater] = player->powers[pw_spacetime] = 0;
		}
	}

	if (player->spectator)
	{
		P_SpectatorMovement(player);
		return;
	}

	// Locate the capsule for this mare.
	else if (maptol & TOL_NIGHTS)
	{
		if (!player->capsule && !player->bonustime)
		{
			thinker_t *th;
			mobj_t *mo2;

			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

				mo2 = (mobj_t *)th;

				if (mo2->type == MT_EGGCAPSULE
					&& mo2->threshold == player->mare)
					P_SetTarget(&player->capsule, mo2);
			}
		}
		else if (player->capsule && player->capsule->reactiontime > 0 && player == &players[player->capsule->reactiontime-1])
		{
			P_DoNiGHTSCapsule(player);
			return;
		}

		// Test revamped NiGHTS movement.
		if (player->pflags & PF_NIGHTSMODE)
		{
			P_NiGHTSMovement(player);
			// No more goto blockchecking, let's just do all that here =D
			if (CheckForBustableBlocks)
				P_CheckBustableBlocks(player);
			if (CheckForBouncySector)
				P_CheckBouncySectors(player);
			if (CheckForQuicksand)
				P_CheckQuicksand(player);
			return;
		}

		if (player->pflags & PF_NIGHTSFALL && P_IsObjectOnGround(player->mo))
		{
			if (G_IsSpecialStage(gamemap))
			{
				if (player == &players[displayplayer]) // only play the sound for yourself landing
					S_StartSound(NULL, sfx_s3k6a);
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i])
						players[i].exiting = (14*TICRATE)/5 + 1;
			}
			else if (player->health > 1)
				P_DamageMobj(player->mo, NULL, NULL, 1);
			player->pflags &= ~PF_NIGHTSFALL;
		}
	}

	//////////////////////
	// MOVEMENT CODE	//
	//////////////////////

	if (twodlevel || player->mo->flags2 & MF2_TWOD) // 2d-level, so special control applies.
		P_2dMovement(player);
	else
	{
		if (!player->climbing && (!P_AnalogMove(player)))
			player->mo->angle = (cmd->angleturn<<16 /* not FRACBITS */);

		ticruned++;
		if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
			ticmiss++;

		P_3dMovement(player);
	}

	if (maptol & TOL_2D)
		runspd = FixedMul(runspd, 2*FRACUNIT/3);

	P_SkidStuff(player);

	/////////////////////////
	// MOVEMENT ANIMATIONS //
	/////////////////////////

	if ((cmd->forwardmove != 0 || cmd->sidemove != 0) || (player->powers[pw_super] && player->mo->z > player->mo->floorz))
	{
		// If the player is moving fast enough,
		// break into a run!
		if (player->speed >= runspd && player->panim == PA_WALK && !player->skidtime && (onground || player->powers[pw_super]))
			P_SetPlayerMobjState (player->mo, S_PLAY_SPD1);

		// Otherwise, just walk.
		else if ((player->rmomx || player->rmomy) && player->panim == PA_IDLE)
			P_SetPlayerMobjState (player->mo, S_PLAY_RUN1);
	}

	// If your running animation is playing, and you're
	// going too slow, switch back to the walking frames.
	if (player->panim == PA_RUN && player->speed < runspd)
		P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);

	// If Springing, but travelling DOWNWARD, change back!
	if (player->mo->state == &states[S_PLAY_SPRING] && P_MobjFlip(player->mo)*player->mo->momz < 0)
		P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
	// If Springing but on the ground, change back!
	else if (onground && (player->mo->state == &states[S_PLAY_SPRING] || player->panim == PA_FALL || player->mo->state == &states[S_PLAY_CARRY]) && !player->mo->momz)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

	// If you are stopped and are still walking, stand still!
	if (!player->mo->momx && !player->mo->momy && !player->mo->momz && player->panim == PA_WALK)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);


//////////////////
//GAMEPLAY STUFF//
//////////////////

	// Make sure you're not "jumping" on the ground
	if (onground && player->pflags & PF_JUMPED && !(player->pflags & PF_GLIDING)
	&& P_MobjFlip(player->mo)*player->mo->momz < 0)
	{
		player->pflags &= ~PF_JUMPED;
		player->jumping = 0;
		player->secondjump = 0;
		player->pflags &= ~PF_THOKKED;
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}

	// Cap the speed limit on a spindash
	// Up the 60*FRACUNIT number to boost faster, you speed demon you!
	if (player->dashspeed > FixedMul(player->maxdash, player->mo->scale))
		player->dashspeed = FixedMul(player->maxdash, player->mo->scale);
	else if (player->dashspeed > 0 && player->dashspeed < FixedMul(player->mindash, player->mo->scale))
		player->dashspeed = FixedMul(player->mindash, player->mo->scale);

	if (!(player->charability == CA_GLIDEANDCLIMB) || player->gotflag) // If you can't glide, then why the heck would you be gliding?
	{
		if (player->pflags & PF_GLIDING || player->climbing)
		{
			if (onground)
				P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
			else
			{
				player->pflags |= PF_JUMPED;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}
		}
		player->pflags &= ~PF_GLIDING;
		player->glidetime = 0;
		player->climbing = 0;
	}

	// Glide MOMZ
	// AKA my own gravity. =)
	if (player->pflags & PF_GLIDING)
	{
		fixed_t leeway;
		fixed_t glidespeed = player->actionspd;

		if (player->powers[pw_super])
			glidespeed *= 2;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if (player->mo->momz > FixedMul(2*FRACUNIT, player->mo->scale))
				player->mo->momz -= FixedMul(3*(FRACUNIT/4), player->mo->scale);
		}
		else
		{
			if (player->mo->momz < FixedMul(-2*FRACUNIT, player->mo->scale))
				player->mo->momz += FixedMul(3*(FRACUNIT/4), player->mo->scale);
		}

		// Strafing while gliding.
		leeway = FixedAngle(cmd->sidemove*(FRACUNIT/2));

		if (player->skidtime) // ground gliding
		{
			fixed_t speed = FixedMul(glidespeed, FRACUNIT - (FRACUNIT>>2));
			if (player->mo->eflags & MFE_UNDERWATER)
				speed >>= 1;
			speed = FixedMul(speed - player->glidetime*FRACUNIT, player->mo->scale);
			if (speed < 0)
				speed = 0;
			P_InstaThrust(player->mo, player->mo->angle-leeway, speed);
		}
		else if (player->mo->eflags & MFE_UNDERWATER)
			P_InstaThrust(player->mo, player->mo->angle-leeway, FixedMul((glidespeed>>1) + player->glidetime*750, player->mo->scale));
		else
			P_InstaThrust(player->mo, player->mo->angle-leeway, FixedMul(glidespeed + player->glidetime*1500, player->mo->scale));

		player->glidetime++;

		if (!(player->pflags & PF_JUMPDOWN)) // If not holding the jump button
		{
			P_ResetPlayer(player); // down, stop gliding.
			if (onground)
				P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
			else if ((player->charability2 == CA2_MULTIABILITY)
				|| (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]) && player->charability == CA_GLIDEANDCLIMB))
			{
				player->pflags |= PF_JUMPED;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}
			else
			{
				player->pflags |= PF_THOKKED;
				player->mo->momx >>= 1;
				player->mo->momy >>= 1;
				P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
			}
		}
	}
	else if (player->climbing) // 'Deceleration' for climbing on walls.
	{
		if (player->mo->momz > 0)
		{
			player->mo->momz -= FixedMul(FRACUNIT/2, player->mo->scale);
			if (player->mo->momz < 0)
				player->mo->momz = 0;
		}
		else if (player->mo->momz < 0)
		{
			player->mo->momz += FixedMul(FRACUNIT/2, player->mo->scale);
			if (player->mo->momz > 0)
				player->mo->momz = 0;
		}
	}

	// If you're running fast enough, you can create splashes as you run in shallow water.
	if (!player->climbing
	&& ((!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z + player->mo->height >= player->mo->watertop && player->mo->z <= player->mo->watertop)
		|| (player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height >= player->mo->waterbottom && player->mo->z <= player->mo->waterbottom))
	&& (player->speed > runspd || (player->pflags & PF_STARTDASH))
	&& leveltime % (TICRATE/7) == 0 && player->mo->momz == 0 && !(player->pflags & PF_SLIDING) && !player->spectator)
	{
		mobj_t *water = P_SpawnMobj(player->mo->x, player->mo->y,
			((player->mo->eflags & MFE_VERTICALFLIP) ? player->mo->waterbottom - FixedMul(mobjinfo[MT_SPLISH].height, player->mo->scale) : player->mo->watertop), MT_SPLISH);
		if (player->mo->eflags & MFE_GOOWATER)
			S_StartSound(water, sfx_ghit);
		else
			S_StartSound(water, sfx_wslap);
		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			water->flags2 |= MF2_OBJECTFLIP;
			water->eflags |= MFE_VERTICALFLIP;
		}
		water->destscale = player->mo->scale;
		P_SetScale(water, player->mo->scale);
	}

	// Little water sound while touching water - just a nicety.
	if ((player->mo->eflags & MFE_TOUCHWATER) && !(player->mo->eflags & MFE_UNDERWATER) && !player->spectator)
	{
		if (P_RandomChance(FRACUNIT/2) && leveltime % TICRATE == 0)
			S_StartSound(player->mo, sfx_floush);
	}

	////////////////
	//TAILS FLYING//
	////////////////

	if (!(player->charability == CA_FLY || player->charability == CA_SWIM)) // why are you flying when you cannot fly?!
	{
		if (player->powers[pw_tailsfly]
		|| (player->mo->state >= &states[S_PLAY_SPC1] && player->mo->state <= &states[S_PLAY_SPC4]))
		{
			if (onground)
				P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
			else
			{
				player->pflags |= PF_JUMPED;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}
		}
		player->powers[pw_tailsfly] = 0;
	}

	if (player->gotflag && player->powers[pw_tailsfly])
		player->powers[pw_tailsfly] = 1;

	// If not in a fly position, don't think you're flying!
	if (player->panim != PA_ABILITY)
		player->powers[pw_tailsfly] = 0;

	if (player->charability == CA_FLY || (player->charability == CA_SWIM && player->mo->eflags & MFE_UNDERWATER))
	{
		// Fly counter for Tails.
		if (player->powers[pw_tailsfly])
		{
			const fixed_t actionspd = player->actionspd/100;

			if (player->charability2 == CA2_MULTIABILITY)
			{
				// Adventure-style flying by just holding the button down
				if (cmd->buttons & BT_JUMP && !(player->pflags & PF_STASIS) && !player->exiting)
					P_SetObjectMomZ(player->mo, actionspd/4, true);
			}
			else
			{
				// Classic flying
				if (player->fly1)
				{
					if (P_MobjFlip(player->mo)*player->mo->momz < FixedMul(5*actionspd, player->mo->scale))
						P_SetObjectMomZ(player->mo, actionspd/2, true);

					player->fly1--;
				}
			}

			// Tails Put-Put noise
			if (player->charability == CA_FLY && player->bot != 1 && leveltime % 10 == 0 && !player->spectator)
				S_StartSound(player->mo, sfx_putput);

			// Descend
			if (cmd->buttons & BT_USE && !(player->pflags & PF_STASIS) && !player->exiting)
				if (P_MobjFlip(player->mo)*player->mo->momz > -FixedMul(5*actionspd, player->mo->scale))
					P_SetObjectMomZ(player->mo, -actionspd/2, true);

		}
		else
		{
			// Tails-gets-tired Stuff
			if (player->panim == PA_ABILITY)
				P_SetPlayerMobjState(player->mo, S_PLAY_SPC4);

			if (player->charability == CA_FLY && (leveltime % 10 == 0)
				&& player->mo->state >= &states[S_PLAY_SPC1]
				&& player->mo->state <= &states[S_PLAY_SPC4]
				&& !player->spectator)
				S_StartSound(player->mo, sfx_pudpud);
		}
	}

	// End your chain if you're on the ground or climbing a wall.
	// But not if invincible! Allow for some crazy long chains with it.
	// Also keep in mind the PF_JUMPED check.
	// If we lacked this, stepping up while jumping up would reset score.
	// (for instance, when climbing up off a wall.)
	if ((onground || player->climbing) && !(player->pflags & PF_JUMPED) && player->powers[pw_invulnerability] <= 1)
		P_ResetScore(player);

	// Show the "THOK!" graphic when spinning quickly across the ground. (even applies to non-spinners, in the case of zoom tubes)
	if (player->pflags & PF_SPINNING && player->speed > FixedMul(15<<FRACBITS, player->mo->scale) && !(player->pflags & PF_JUMPED))
	{
		P_SpawnSpinMobj(player, player->spinitem);
		if (demorecording)
			G_GhostAddSpin();
	}


	////////////////////////////
	//SPINNING AND SPINDASHING//
	////////////////////////////

	// If the player isn't on the ground, make sure they aren't in a "starting dash" position.
	if (!onground)
	{
		player->pflags &= ~PF_STARTDASH;
		player->dashspeed = 0;
	}

	if ((player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL
	&& (player->pflags & PF_SPINNING) && player->speed > FixedMul(4<<FRACBITS, player->mo->scale) && onground && (leveltime & 1)
	&& !(player->mo->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
		P_ElementalFireTrail(player);

	P_DoSpinDash(player, cmd);

	// jumping
	P_DoJumpStuff(player, cmd);

	// If you're not spinning, you'd better not be spindashing!
	if (!(player->pflags & PF_SPINNING))
		player->pflags &= ~PF_STARTDASH;

	//////////////////
	//ANALOG CONTROL//
	//////////////////

	// This really looks like it should be moved to P_3dMovement. -Red
	if (P_AnalogMove(player)
		&& (cmd->forwardmove != 0 || cmd->sidemove != 0) && !player->climbing && !twodlevel && !(player->mo->flags2 & MF2_TWOD))
	{
		// If travelling slow enough, face the way the controls
		// point and not your direction of movement.
		if (player->speed < FixedMul(5*FRACUNIT, player->mo->scale) || player->pflags & PF_GLIDING || !onground)
		{
			angle_t tempangle;

			tempangle = (cmd->angleturn << 16);

#ifdef REDSANALOG // Ease to it. Chillax. ~Red
			tempangle += R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT);
			{
				fixed_t tweenvalue = max(abs(cmd->forwardmove), abs(cmd->sidemove));

				if (tweenvalue < 10 && (cmd->buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)) {
					tempangle = (cmd->angleturn << 16);
					tweenvalue = 16;
				}

				tweenvalue *= tweenvalue*tweenvalue*1536;

				//if (player->pflags & PF_GLIDING)
					//tweenvalue >>= 1;

				tempangle -= player->mo->angle;

				if (tempangle < ANGLE_180 && tempangle > tweenvalue)
					player->mo->angle += tweenvalue;
				else if (tempangle >= ANGLE_180 && InvAngle(tempangle) > tweenvalue)
					player->mo->angle -= tweenvalue;
				else
					player->mo->angle += tempangle;
			}
#else
			// Less math this way ~Red
			player->mo->angle = R_PointToAngle2(0, 0, cmd->forwardmove*FRACUNIT, -cmd->sidemove*FRACUNIT)+tempangle;
#endif
		}
		// Otherwise, face the direction you're travelling.
		else if (player->panim == PA_WALK || player->panim == PA_RUN || player->panim == PA_ROLL
		|| ((player->mo->state >= &states[S_PLAY_ABL1] && player->mo->state <= &states[S_PLAY_SPC4]) && player->charability == CA_FLY))
			player->mo->angle = R_PointToAngle2(0, 0, player->rmomx, player->rmomy);

		// Update the local angle control.
		if (player == &players[consoleplayer])
			localangle = player->mo->angle;
		else if (player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;
	}

	///////////////////////////
	//BOMB SHIELD ACTIVATION,//
	//HOMING, AND OTHER COOL //
	//STUFF!                 //
	///////////////////////////

	if (cmd->buttons & BT_USE) // Spin button effects
	{
		if (player->pflags & PF_JUMPED) // If the player is jumping
		{
			if (!(player->pflags & PF_USEDOWN)) // If the player is not holding down BT_USE
			{
				// Jump shield activation
				if (!P_PlayerInPain(player) // If the player is not in pain
				&& !player->climbing // If the player is not climbing
				&& !(player->pflags & (PF_GLIDING|PF_SLIDING|PF_THOKKED)) // If the player is not gliding or sliding and hasn't used their ability
				&& !onground) // If the player isn't on the ground
				{
					if ((player->powers[pw_shield] & SH_NOSTACK) == SH_JUMP && !player->powers[pw_super])
						P_DoJumpShield(player);
					else if (player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]) && player->charability == CA_FLY)
					{
						P_DoJumpShield(player);
						player->mo->momz *= 2;
					}
				}
				// Bomb shield activation
				if ((player->powers[pw_shield] & SH_NOSTACK) == SH_BOMB)
				{
					// Don't let Super Sonic or invincibility use it
					if (!(player->powers[pw_super] || player->powers[pw_invulnerability]))
						P_BlackOw(player);
				}
			}
			// Super Sonic move
			if (player->skin == 0 && player->powers[pw_super] && player->speed > FixedMul(5<<FRACBITS, player->mo->scale)
			&& P_MobjFlip(player->mo)*player->mo->momz <= 0)
			{
				if (player->panim == PA_ROLL || player->mo->state == &states[S_PLAY_PAIN])
					P_SetPlayerMobjState(player->mo, S_PLAY_SUPERWALK1);

				player->mo->momz = 0;
				player->pflags &= ~PF_SPINNING;
				player->jumping = 0; // don't cut jump height after bouncing off something
			}
		}
	}

	// HOMING option.
	if (player->charability == CA_HOMINGTHOK)
	{
		// If you've got a target, chase after it!
		if (player->homing && player->mo->tracer)
		{
			P_SpawnThokMobj(player);
			P_HomingAttack(player->mo, player->mo->tracer);

			// But if you don't, then stop homing.
			if (player->mo->tracer->health <= 0 || (player->mo->tracer->flags2 & MF2_FRET))
			{
				if (player->mo->eflags & MFE_UNDERWATER)
					P_SetObjectMomZ(player->mo, FixedDiv(457*FRACUNIT,72*FRACUNIT), false);
				else
					P_SetObjectMomZ(player->mo, 10*FRACUNIT, false);

				player->mo->momx = player->mo->momy = player->homing = 0;

				if (player->mo->tracer->flags2 & MF2_FRET)
					P_InstaThrust(player->mo, player->mo->angle, -(player->speed>>3));

				if (!(player->mo->tracer->flags & MF_BOSS))
					player->pflags &= ~PF_THOKKED;
			}
		}

		// If you're not jumping, then you obviously wouldn't be homing.
		if (!(player->pflags & PF_JUMPED))
			player->homing = 0;
	}
	else
		player->homing = 0;

	if (player->climbing == 1)
		P_DoClimbing(player);

	if (player->climbing > 1)
	{
		P_InstaThrust(player->mo, player->mo->angle, FixedMul(4*FRACUNIT, player->mo->scale)); // Shove up against the wall
		player->climbing--;
	}

	if (!player->climbing)
	{
		player->lastsidehit = -1;
		player->lastlinehit = -1;
	}

	// Make sure you're not teetering when you shouldn't be.
	if ((player->mo->state == &states[S_PLAY_TEETER1] || player->mo->state == &states[S_PLAY_TEETER2] || player->mo->state == &states[S_PLAY_SUPERTEETER])
	&& (player->mo->momx || player->mo->momy || player->mo->momz))
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

	// Check for teeter!
	if (!(player->mo->momz || player->mo->momx || player->mo->momy) && !(player->mo->eflags & MFE_GOOWATER)
	&& player->panim == PA_IDLE && !(player->pflags & (PF_CARRIED|PF_ITEMHANG|PF_ROPEHANG)))
		P_DoTeeter(player);

	// Toss a flag
	if (G_GametypeHasTeams() && (cmd->buttons & BT_TOSSFLAG) && !(player->powers[pw_super]) && !(player->tossdelay))
	{
		if (!(player->gotflag & (GF_REDFLAG|GF_BLUEFLAG)))
			P_PlayerEmeraldBurst(player, true); // Toss emeralds
		else
			P_PlayerFlagBurst(player, true);
	}

	// check for fire
	if (!player->exiting)
		P_DoFiring(player, cmd);

	{
		fixed_t oldheight = player->mo->height;

		// Less height while spinning. Good for spinning under things...?
		if ((player->mo->state == &states[player->mo->info->painstate] || player->mo->state == &states[S_PLAY_SUPERHIT])
		|| (player->charability2 == CA2_SPINDASH && (player->pflags & (PF_SPINNING|PF_JUMPED)))
		|| player->powers[pw_tailsfly] || player->pflags & PF_GLIDING
		|| (player->charability == CA_FLY && (player->mo->state >= &states[S_PLAY_SPC1] && player->mo->state <= &states[S_PLAY_SPC4])))
			player->mo->height = P_GetPlayerSpinHeight(player);
		else
			player->mo->height = P_GetPlayerHeight(player);

		if (player->mo->eflags & MFE_VERTICALFLIP && player->mo->height != oldheight) // adjust z height for reverse gravity, similar to how it's done for scaling
			player->mo->z -= player->mo->height - oldheight;
	}

	// Crush test...
	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height)
		&& !(player->mo->flags & MF_NOCLIP))
	{
		if ((player->charability2 == CA2_SPINDASH) && !(player->pflags & PF_SPINNING))
		{
			player->pflags |= PF_SPINNING;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		}
		else if (player->mo->ceilingz - player->mo->floorz < player->mo->height)
		{
			if ((netgame || multiplayer) && player->spectator)
				P_DamageMobj(player->mo, NULL, NULL, 42000); // Respawn crushed spectators
			else
			{
				mobj_t *killer = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_NULL);
				killer->threshold = 44; // Special flag that it was crushing which killed you.
				P_DamageMobj(player->mo, killer, killer, 10000);
			}

			if (player->playerstate == PST_DEAD)
				return;
		}
	}

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none && cv_grfovchange.value)
	{
		fixed_t speed;
		const fixed_t runnyspeed = 20*FRACUNIT;

		speed = R_PointToDist2(player->rmomx, player->rmomy, 0, 0);

		if (speed > player->normalspeed-5*FRACUNIT)
			speed = player->normalspeed-5*FRACUNIT;

		if (speed >= runnyspeed)
			player->fovadd = speed-runnyspeed;
		else
			player->fovadd = 0;

		if (player->fovadd < 0)
			player->fovadd = 0;
	}
	else
		player->fovadd = 0;
#endif

#ifdef FLOORSPLATS
	if (cv_shadow.value && rendermode == render_soft)
		R_AddFloorSplat(player->mo->subsector, player->mo, "SHADOW", player->mo->x,
			player->mo->y, player->mo->floorz, SPLATDRAWMODE_OPAQUE);
#endif

	// Look for blocks to bust up
	// Because of FF_SHATTER, we should look for blocks constantly,
	// not just when spinning or playing as Knuckles
	if (CheckForBustableBlocks)
		P_CheckBustableBlocks(player);

	// Special handling for
	// gliding in 2D mode
	if ((twodlevel || player->mo->flags2 & MF2_TWOD) && player->pflags & PF_GLIDING && player->charability == CA_GLIDEANDCLIMB
		&& !(player->mo->flags & MF_NOCLIP))
	{
		msecnode_t *node; // only place it's being used in P_MovePlayer now
		fixed_t oldx;
		fixed_t oldy;
		fixed_t floorz, ceilingz;

		oldx = player->mo->x;
		oldy = player->mo->y;

		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);

		for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
		{
			if (!node->m_sector)
				break;

			if (node->m_sector->ffloors)
			{
				ffloor_t *rover;
				fixed_t topheight, bottomheight;

				for (rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_BLOCKPLAYER))
						continue;

					topheight = P_GetFOFTopZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
					bottomheight = P_GetFOFBottomZ(player->mo, node->m_sector, rover, player->mo->x, player->mo->y, NULL);
					if (topheight > player->mo->z && bottomheight < player->mo->z)
					{
						P_ResetPlayer(player);
						S_StartSound(player->mo, sfx_s3k4a);
						player->climbing = 5;
						player->mo->momx = player->mo->momy = player->mo->momz = 0;
						break;
					}
				}
			}

			floorz = P_GetFloorZ(player->mo, node->m_sector, player->mo->x, player->mo->y, NULL);
			ceilingz = P_GetCeilingZ(player->mo, node->m_sector, player->mo->x, player->mo->y, NULL);

			if (player->mo->z+player->mo->height > ceilingz
				&& node->m_sector->ceilingpic == skyflatnum)
				continue;

			if (floorz > player->mo->z || ceilingz < player->mo->z)
			{
				P_ResetPlayer(player);
				S_StartSound(player->mo, sfx_s3k4a);
				player->climbing = 5;
				player->mo->momx = player->mo->momy = player->mo->momz = 0;
				break;
			}
		}
		P_UnsetThingPosition(player->mo);
		player->mo->x = oldx;
		player->mo->y = oldy;
		P_SetThingPosition(player->mo);
	}

	// Check for a BOUNCY sector!
	if (CheckForBouncySector)
		P_CheckBouncySectors(player);

	// Look for Quicksand!
	if (CheckForQuicksand)
		P_CheckQuicksand(player);
}

static void P_DoZoomTube(player_t *player)
{
	INT32 sequence;
	fixed_t speed;
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	boolean reverse;

	player->mo->height = P_GetPlayerSpinHeight(player);

	if (player->speed > 0)
		reverse = false;
	else
		reverse = true;

	player->powers[pw_flashing] = 1;

	speed = abs(player->speed);

	sequence = player->mo->tracer->threshold;

	// change slope
	dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - player->mo->z);

	if (dist < 1)
		dist = 1;

	player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
	player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
	player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - player->mo->z, dist), (speed));

	// Calculate the distance between the player and the waypoint
	// 'dist' already equals this.

	// Will the player go past the waypoint?
	if (speed > dist)
	{
		speed -= dist;
		// If further away, set XYZ of player to waypoint location
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->tracer->x;
		player->mo->y = player->mo->tracer->y;
		player->mo->z = player->mo->tracer->z;
		P_SetThingPosition(player->mo);

		// ugh, duh!!
		player->mo->floorz = player->mo->subsector->sector->floorheight;
		player->mo->ceilingz = player->mo->subsector->sector->ceilingheight;

		CONS_Debug(DBG_GAMELOGIC, "Looking for next waypoint...\n");

		// Find next waypoint
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type != MT_TUBEWAYPOINT)
				continue;

			if (mo2->threshold == sequence)
			{
				if ((reverse && mo2->health == player->mo->tracer->health - 1)
					|| (!reverse && mo2->health == player->mo->tracer->health + 1))
				{
					waypoint = mo2;
					break;
				}
			}
		}

		if (waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			P_SetTarget(&player->mo->tracer, waypoint);

			// calculate MOMX/MOMY/MOMZ for next waypoint

			// change slope
			dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - player->mo->z);

			if (dist < 1)
				dist = 1;

			player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
			player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
			player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - player->mo->z, dist), (speed));
		}
		else
		{
			P_SetTarget(&player->mo->tracer, NULL); // Else, we just let them fly.

			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found, releasing from track...\n");
		}
	}

	// change angle
	if (player->mo->tracer)
	{
		player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->tracer->x, player->mo->tracer->y);

		if (player == &players[consoleplayer])
			localangle = player->mo->angle;
		else if (player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;
	}
}

//
// P_DoRopeHang
//
// Kinda like P_DoZoomTube
// but a little different.
//
static void P_DoRopeHang(player_t *player)
{
	INT32 sequence;
	fixed_t speed;
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	fixed_t playerz;

	player->mo->height = P_GetPlayerHeight(player);

	// Play the 'clink' sound only if the player is moving.
	if (!(leveltime & 7) && player->speed)
		S_StartSound(player->mo, sfx_s3k55);

	playerz = player->mo->z + player->mo->height;

	speed = abs(player->speed);

	sequence = player->mo->tracer->threshold;

	// change slope
	dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - playerz);

	if (dist < 1)
		dist = 1;

	player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
	player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
	player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - playerz, dist), (speed));

	if (player->cmd.buttons & BT_USE && !(player->pflags & PF_STASIS)) // Drop off of the rope
	{
		P_SetTarget(&player->mo->tracer, NULL);

		player->pflags |= PF_JUMPED;
		player->pflags &= ~PF_ROPEHANG;

		if (!(player->pflags & PF_SLIDING) && (player->pflags & PF_JUMPED)
		&& !(player->panim == PA_ROLL) && player->charability2 == CA2_SPINDASH)
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		return;
	}

	// If not allowed to move, we're done here.
	if (!speed)
		return;

	// Calculate the distance between the player and the waypoint
	// 'dist' already equals this.

	// Will the player go past the waypoint?
	if (speed > dist)
	{
		speed -= dist;
		// If further away, set XYZ of player to waypoint location
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->tracer->x;
		player->mo->y = player->mo->tracer->y;
		player->mo->z = player->mo->tracer->z - player->mo->height;
		playerz = player->mo->tracer->z;
		P_SetThingPosition(player->mo);

		CONS_Debug(DBG_GAMELOGIC, "Looking for next waypoint...\n");

		// Find next waypoint
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type != MT_TUBEWAYPOINT)
				continue;

			if (mo2->threshold == sequence)
			{
				if (mo2->health == player->mo->tracer->health + 1)
				{
					waypoint = mo2;
					break;
				}
			}
		}

		if (!(player->mo->tracer->flags & MF_SLIDEME) && !waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found, wrapping to start...\n");

			// Wrap around back to first waypoint
			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
					continue;

				mo2 = (mobj_t *)th;

				if (mo2->type != MT_TUBEWAYPOINT)
					continue;

				if (mo2->threshold == sequence)
				{
					if (mo2->health == 0)
					{
						waypoint = mo2;
						break;
					}
				}
			}
		}

		if (waypoint)
		{
			CONS_Debug(DBG_GAMELOGIC, "Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			P_SetTarget(&player->mo->tracer, waypoint);

			// calculate MOMX/MOMY/MOMZ for next waypoint
			// change slope
			dist = P_AproxDistance(P_AproxDistance(player->mo->tracer->x - player->mo->x, player->mo->tracer->y - player->mo->y), player->mo->tracer->z - playerz);

			if (dist < 1)
				dist = 1;

			player->mo->momx = FixedMul(FixedDiv(player->mo->tracer->x - player->mo->x, dist), (speed));
			player->mo->momy = FixedMul(FixedDiv(player->mo->tracer->y - player->mo->y, dist), (speed));
			player->mo->momz = FixedMul(FixedDiv(player->mo->tracer->z - playerz, dist), (speed));
		}
		else
		{
			if (player->mo->tracer->flags & MF_SLIDEME)
			{
				player->pflags |= PF_JUMPED;
				player->pflags &= ~PF_ROPEHANG;

				if (!(player->pflags & PF_SLIDING) && (player->pflags & PF_JUMPED)
				&& !(player->panim == PA_ROLL) && player->charability2 == CA2_SPINDASH)
					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}

			P_SetTarget(&player->mo->tracer, NULL);

			CONS_Debug(DBG_GAMELOGIC, "Next waypoint not found!\n");
		}
	}
}

#if 0
//
// P_NukeAllPlayers
//
// Hurts all players
// source = guy who gets the credit
//
static void P_NukeAllPlayers(player_t *player)
{
	mobj_t *mo;
	thinker_t *think;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;

		if (!mo->player)
			continue;

		if (mo->health <= 0) // dead
			continue;

		if (mo == player->mo)
			continue;

		P_DamageMobj(mo, player->mo, player->mo, 1);
	}

	CONS_Printf(M_GetText("%s caused a world of pain.\n"), player_names[player-players]);

	return;
}
#endif

//
// P_NukeEnemies
// Looks for something you can hit - Used for bomb shield
//
void P_NukeEnemies(mobj_t *inflictor, mobj_t *source, fixed_t radius)
{
	const fixed_t ns = 60 << FRACBITS;
	mobj_t *mo;
	angle_t fa;
	thinker_t *think;
	INT32 i;

	for (i = 0; i < 16; i++)
	{
		fa = (i*(FINEANGLES/16));
		mo = P_SpawnMobj(inflictor->x, inflictor->y, inflictor->z, MT_SUPERSPARK);
		if (!P_MobjWasRemoved(mo))
		{
			mo->momx = FixedMul(FINESINE(fa),ns);
			mo->momy = FixedMul(FINECOSINE(fa),ns);
		}
	}

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;

		if (!(mo->flags & MF_SHOOTABLE) && !(mo->type == MT_EGGGUARD || mo->type == MT_MINUS))
			continue;

		if (mo->flags & MF_MONITOR)
			continue; // Monitors cannot be 'nuked'.

		if (!G_RingSlingerGametype() && mo->type == MT_PLAYER)
			continue; // Don't hurt players in Co-Op!

		if (abs(inflictor->x - mo->x) > radius || abs(inflictor->y - mo->y) > radius || abs(inflictor->z - mo->z) > radius)
			continue; // Workaround for possible integer overflow in the below -Red

		if (P_AproxDistance(P_AproxDistance(inflictor->x - mo->x, inflictor->y - mo->y), inflictor->z - mo->z) > radius)
			continue;

		if (mo->type == MT_MINUS && !(mo->flags & (MF_SPECIAL|MF_SHOOTABLE)))
			mo->flags |= MF_SPECIAL|MF_SHOOTABLE;

		if (mo->type == MT_EGGGUARD && mo->tracer) //nuke Egg Guard's shield!
			P_KillMobj(mo->tracer, inflictor, source);

		if (mo->flags & MF_BOSS || mo->type == MT_PLAYER) //don't OHKO bosses nor players!
			P_DamageMobj(mo, inflictor, source, 1);
		else
			P_DamageMobj(mo, inflictor, source, 1000);
	}
}

//
// P_LookForEnemies
// Looks for something you can hit - Used for homing attack
// Includes monitors and springs!
//
boolean P_LookForEnemies(player_t *player)
{
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closestmo = NULL;
	angle_t an;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;
		if (!(mo->flags & (MF_ENEMY|MF_BOSS|MF_MONITOR|MF_SPRING)))
			continue; // not a valid enemy

		if (mo->health <= 0) // dead
			continue;

		if (mo == player->mo)
			continue;

		if (mo->flags2 & MF2_FRET)
			continue;

		if ((mo->flags & (MF_ENEMY|MF_BOSS)) && !(mo->flags & MF_SHOOTABLE)) // don't aim at something you can't shoot at anyway (see Egg Guard or Minus)
			continue;

		if (mo->type == MT_DETON) // Don't be STUPID, Sonic!
			continue;

		if (((mo->z > player->mo->z+FixedMul(MAXSTEPMOVE, player->mo->scale)) && !(player->mo->eflags & MFE_VERTICALFLIP))
		|| ((mo->z+mo->height < player->mo->z+player->mo->height-FixedMul(MAXSTEPMOVE, player->mo->scale)) && (player->mo->eflags & MFE_VERTICALFLIP))) // Reverse gravity check - Flame.
			continue; // Don't home upwards!

		if (P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > FixedMul(RING_DIST, player->mo->scale))
			continue; // out of range

		if ((twodlevel || player->mo->flags2 & MF2_TWOD)
		&& abs(player->mo->y-mo->y) > player->mo->radius)
			continue; // not in your 2d plane

		if (mo->type == MT_PLAYER) // Don't chase after other players!
			continue;

		if (closestmo && P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > P_AproxDistance(P_AproxDistance(player->mo->x-closestmo->x,
			player->mo->y-closestmo->y), player->mo->z-closestmo->z))
			continue;

		an = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y) - player->mo->angle;

		if (an > ANGLE_90 && an < ANGLE_270)
			continue; // behind back

		if (!P_CheckSight(player->mo, mo))
			continue; // out of sight

		closestmo = mo;
	}

	if (closestmo)
	{
		// Found a target monster
		P_SetTarget(&player->mo->target, P_SetTarget(&player->mo->tracer, closestmo));
		player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, closestmo->x, closestmo->y);
		return true;
	}

	return false;
}

void P_HomingAttack(mobj_t *source, mobj_t *enemy) // Home in on your target
{
	fixed_t dist;
	fixed_t ns = 0;

	if (!enemy)
		return;

	if (!(enemy->health))
		return;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, enemy->x, enemy->y);
	if (source->player && (!demoplayback || P_AnalogMove(source->player)))
	{
		if (source->player == &players[consoleplayer])
			localangle = source->angle;
		else if (source->player == &players[secondarydisplayplayer])
			localangle2 = source->angle;
	}

	// change slope
	dist = P_AproxDistance(P_AproxDistance(enemy->x - source->x, enemy->y - source->y),
		enemy->z - source->z);

	if (dist < 1)
		dist = 1;

	if (source->type == MT_DETON && enemy->player) // For Deton Chase (Unused)
		ns = FixedDiv(FixedMul(enemy->player->normalspeed, enemy->scale), FixedDiv(20*FRACUNIT,17*FRACUNIT));
	else if (source->type != MT_PLAYER)
	{
		if (source->threshold == 32000)
			ns = FixedMul(source->info->speed/2, source->scale);
		else
			ns = FixedMul(source->info->speed, source->scale);
	}
	else if (source->player)
		ns = FixedDiv(FixedMul(source->player->actionspd, source->scale), 3*FRACUNIT/2);

	source->momx = FixedMul(FixedDiv(enemy->x - source->x, dist), ns);
	source->momy = FixedMul(FixedDiv(enemy->y - source->y, dist), ns);
	source->momz = FixedMul(FixedDiv(enemy->z - source->z, dist), ns);
}

// Search for emeralds
void P_FindEmerald(void)
{
	thinker_t *th;
	mobj_t *mo2;

	hunt1 = hunt2 = hunt3 = NULL;

	// scan the remaining thinkers
	// to find all emeralds
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2->type == MT_EMERHUNT)
		{
			if (!hunt1)
				hunt1 = mo2;
			else if (!hunt2)
				hunt2 = mo2;
			else if (!hunt3)
				hunt3 = mo2;
		}
	}
	return;
}

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//

static void P_DeathThink(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	player->deltaviewheight = 0;

	if (player->deadtimer < INT32_MAX)
		player->deadtimer++;

	// continue logic
	if (!(netgame || multiplayer) && player->lives <= 0)
	{
		if (player->deadtimer > TICRATE && (cmd->buttons & BT_USE || cmd->buttons & BT_JUMP) && player->continues > 0)
			G_UseContinue();
		else if (player->deadtimer >= gameovertics)
			G_UseContinue(); // Even if we don't have one this handles ending the game
	}

	// Force respawn if idle for more than 30 seconds in shooter modes.
	if (player->deadtimer > 30*TICRATE && !G_PlatformGametype())
		player->playerstate = PST_REBORN;
	else if (player->lives > 0 && !G_IsSpecialStage(gamemap)) // Don't allow "click to respawn" in special stages!
	{
		// Respawn with jump button, force respawn time (3 second default, cheat protected) in shooter modes.
		if ((cmd->buttons & BT_JUMP) && player->deadtimer > cv_respawntime.value*TICRATE
			&& gametype != GT_RACE && gametype != GT_COOP)
			player->playerstate = PST_REBORN;

		// Instant respawn in race or if you're spectating.
		if ((cmd->buttons & BT_JUMP) && (gametype == GT_RACE || player->spectator))
			player->playerstate = PST_REBORN;

		// One second respawn in coop.
		if ((cmd->buttons & BT_JUMP) && player->deadtimer > TICRATE && (gametype == GT_COOP || gametype == GT_COMPETITION))
			player->playerstate = PST_REBORN;

		// Single player auto respawn
		if (!(netgame || multiplayer) && player->deadtimer > 5*TICRATE)
			player->playerstate = PST_REBORN;
	}
	else if ((netgame || multiplayer) && player->deadtimer == 8*TICRATE)
	{
		// In a net/multiplayer game, and out of lives
		if (gametype == GT_COMPETITION)
		{
			INT32 i;

			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && !players[i].exiting && players[i].lives > 0)
					break;

			if (i == MAXPLAYERS)
			{
				// Everyone's either done with the race, or dead.
				if (!countdown2 || countdown2 > 1*TICRATE)
					countdown2 = 1*TICRATE;
			}
		}

		// In a coop game, and out of lives
		if (gametype == GT_COOP)
		{
			INT32 i;

			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && (players[i].exiting || players[i].lives > 0))
					break;

			if (i == MAXPLAYERS)
			{
				// They're dead, Jim.
				//nextmapoverride = spstage_start;
				nextmapoverride = gamemap;
				countdown2 = 1*TICRATE;
				skipstats = true;

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i])
						players[i].score = 0;
				}

				//emeralds = 0;
				tokenbits = 0;
				tokenlist = 0;
				token = 0;
			}
		}
	}

	if (gametype == GT_RACE || gametype == GT_COMPETITION || (gametype == GT_COOP && (multiplayer || netgame)))
	{
		// Keep time rolling in race mode
		if (!(countdown2 && !countdown) && !player->exiting && !(player->pflags & PF_TIMEOVER))
		{
			if (gametype == GT_RACE || gametype == GT_COMPETITION)
			{
				if (leveltime >= 4*TICRATE)
					player->realtime = leveltime - 4*TICRATE;
				else
					player->realtime = 0;
			}
			else
				player->realtime = leveltime;
		}

		// Return to level music
		if (player->lives <= 0)
		{
			if (netgame)
			{
				if (player->deadtimer == gameovertics && P_IsLocalPlayer(player))
					S_ChangeMusic(mapmusname, mapmusflags, true);
			}
			else if (multiplayer) // local multiplayer only
			{
				if (player->deadtimer != gameovertics)
					;
				// Restore the other player's music once we're dead for long enough
				// -- that is, as long as they aren't dead too
				else if (player == &players[displayplayer] && players[secondarydisplayplayer].lives > 0)
					P_RestoreMusic(&players[secondarydisplayplayer]);
				else if (player == &players[secondarydisplayplayer] && players[displayplayer].lives > 0)
					P_RestoreMusic(&players[displayplayer]);
			}
		}
	}

	if (!player->mo)
		return;

	P_CalcHeight(player);
}

//
// P_MoveCamera: make sure the camera is not outside the world and looks at the player avatar
//

camera_t camera, camera2; // Two cameras.. one for split!

static void CV_CamRotate_OnChange(void)
{
	if (cv_cam_rotate.value < 0)
		CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value + 360);
	else if (cv_cam_rotate.value > 359)
		CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value % 360);
}

static void CV_CamRotate2_OnChange(void)
{
	if (cv_cam2_rotate.value < 0)
		CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value + 360);
	else if (cv_cam2_rotate.value > 359)
		CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value % 360);
}

static CV_PossibleValue_t CV_CamSpeed[] = {{0, "MIN"}, {1*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t rotation_cons_t[] = {{1, "MIN"}, {45, "MAX"}, {0, NULL}};
static CV_PossibleValue_t CV_CamRotate[] = {{-720, "MIN"}, {720, "MAX"}, {0, NULL}};

consvar_t cv_cam_dist = {"cam_dist", "128", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_height = {"cam_height", "20", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_still = {"cam_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_speed = {"cam_speed", "0.25", CV_FLOAT, CV_CamSpeed, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_rotate = {"cam_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_rotspeed = {"cam_rotspeed", "10", 0, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_dist = {"cam2_dist", "128", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_height = {"cam2_height", "20", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_still = {"cam2_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_speed = {"cam2_speed", "0.25", CV_FLOAT, CV_CamSpeed, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_rotate = {"cam2_rotate", "0", CV_CALL|CV_NOINIT, CV_CamRotate, CV_CamRotate2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_rotspeed = {"cam2_rotspeed", "10", 0, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

fixed_t t_cam_dist = -42;
fixed_t t_cam_height = -42;
fixed_t t_cam_rotate = -42;
fixed_t t_cam2_dist = -42;
fixed_t t_cam2_height = -42;
fixed_t t_cam2_rotate = -42;

#define MAXCAMERADIST 140*FRACUNIT // Max distance the camera can be in front of the player (2D mode)

void P_ResetCamera(player_t *player, camera_t *thiscam)
{
	tic_t tries = 0;
	fixed_t x, y, z;

	if (!player->mo)
		return;

	if (thiscam->chase && player->mo->health <= 0)
		return;

	thiscam->chase = true;
	x = player->mo->x - P_ReturnThrustX(player->mo, thiscam->angle, player->mo->radius);
	y = player->mo->y - P_ReturnThrustY(player->mo, thiscam->angle, player->mo->radius);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		z = player->mo->z + player->mo->height - (cv_viewheight.value<<FRACBITS) - 16*FRACUNIT;
	else
		z = player->mo->z + (cv_viewheight.value<<FRACBITS);

	// set bits for the camera
	thiscam->x = x;
	thiscam->y = y;
	thiscam->z = z;

	if (!(thiscam == &camera && (cv_cam_still.value || cv_analog.value))
	&& !(thiscam == &camera2 && (cv_cam2_still.value || cv_analog2.value)))
	{
		thiscam->angle = player->mo->angle;
		thiscam->aiming = 0;
	}
	thiscam->relativex = 0;

	thiscam->subsector = R_PointInSubsector(thiscam->x,thiscam->y);

	thiscam->radius = 20*FRACUNIT;
	thiscam->height = 16*FRACUNIT;

	while (!P_MoveChaseCamera(player,thiscam,true) && ++tries < 2*TICRATE);
}

boolean P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean resetcalled)
{
	angle_t angle = 0, focusangle = 0, focusaiming = 0;
	fixed_t x, y, z, dist, checkdist, viewpointx, viewpointy, camspeed, camdist, camheight, pviewheight;
	INT32 camrotate;
	boolean camstill, cameranoclip;
	mobj_t *mo;
	subsector_t *newsubsec;
	fixed_t f1, f2;

	cameranoclip = (player->pflags & (PF_NOCLIP|PF_NIGHTSMODE)) || (player->mo->flags & (MF_NOCLIP|MF_NOCLIPHEIGHT)); // Noclipping player camera noclips too!!

	if (!(player->climbing || (player->pflags & PF_NIGHTSMODE) || player->playerstate == PST_DEAD))
	{
		if (player->spectator) // force cam off for spectators
			return true;

		if (!cv_chasecam.value && thiscam == &camera)
			return true;

		if (!cv_chasecam2.value && thiscam == &camera2)
			return true;
	}

	if (!thiscam->chase && !resetcalled)
	{
		if (player == &players[consoleplayer])
			focusangle = localangle;
		else if (player == &players[secondarydisplayplayer])
			focusangle = localangle2;
		else
			focusangle = player->mo->angle;
		if (thiscam == &camera)
			camrotate = cv_cam_rotate.value;
		else if (thiscam == &camera2)
			camrotate = cv_cam2_rotate.value;
		else
			camrotate = 0;
		thiscam->angle = focusangle + FixedAngle(camrotate*FRACUNIT);
		P_ResetCamera(player, thiscam);
		return true;
	}

	if (!player || !player->mo)
		return true;

	mo = player->mo;

	thiscam->radius = FixedMul(20*FRACUNIT, mo->scale);
	thiscam->height = FixedMul(16*FRACUNIT, mo->scale);

	if (!mo)
		return true;

	// Don't run while respawning from a starpost
	// Inu 4/8/13 Why not?!
//	if (leveltime > 0 && timeinmap <= 0)
//		return true;

	if (player->pflags & PF_NIGHTSMODE)
	{
		focusangle = player->mo->angle;
		focusaiming = 0;
	}
	else if (player == &players[consoleplayer])
	{
		focusangle = localangle;
		focusaiming = localaiming;
	}
	else if (player == &players[secondarydisplayplayer])
	{
		focusangle = localangle2;
		focusaiming = localaiming2;
	}
	else
	{
		focusangle = player->mo->angle;
		focusaiming = player->aiming;
	}

	if (P_CameraThinker(player, thiscam, resetcalled))
		return true;

	if (thiscam == &camera)
	{
		camspeed = cv_cam_speed.value;
		camstill = cv_cam_still.value;
		camrotate = cv_cam_rotate.value;
		camdist = FixedMul(cv_cam_dist.value, mo->scale);
		camheight = FixedMul(cv_cam_height.value, mo->scale);
	}
	else // Camera 2
	{
		camspeed = cv_cam2_speed.value;
		camstill = cv_cam2_still.value;
		camrotate = cv_cam2_rotate.value;
		camdist = FixedMul(cv_cam2_dist.value, mo->scale);
		camheight = FixedMul(cv_cam2_height.value, mo->scale);
	}

#ifdef REDSANALOG
	if (P_AnalogMove(player) && (player->cmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)) {
		camstill = true;

		if (camspeed < 4*FRACUNIT/5)
			camspeed = 4*FRACUNIT/5;
	}
#endif // REDSANALOG

	if (mo->eflags & MFE_VERTICALFLIP)
		camheight += thiscam->height;

	if (twodlevel || (mo->flags2 & MF2_TWOD))
		angle = ANGLE_90;
	else if (camstill || resetcalled || player->playerstate == PST_DEAD)
		angle = thiscam->angle;
	else if (player->pflags & PF_NIGHTSMODE) // NiGHTS Level
	{
		if ((player->pflags & PF_TRANSFERTOCLOSEST) && player->axis1 && player->axis2)
		{
			angle = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
			angle += ANGLE_90;
		}
		else if (player->mo->target)
		{
			if (player->mo->target->flags & MF_AMBUSH)
				angle = R_PointToAngle2(player->mo->target->x, player->mo->target->y, player->mo->x, player->mo->y);
			else
				angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->target->x, player->mo->target->y);
		}
	}
	else if (P_AnalogMove(player)) // Analog
		angle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);
	else if (demoplayback)
	{
		angle = focusangle;
		focusangle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);
		if (player == &players[consoleplayer])
		{
			if (focusangle >= localangle)
				localangle += abs((signed)(focusangle - localangle))>>5;
			else
				localangle -= abs((signed)(focusangle - localangle))>>5;
		}
	}
	else
		angle = focusangle + FixedAngle(camrotate*FRACUNIT);

	if (!resetcalled && (cv_analog.value || demoplayback) && ((thiscam == &camera && t_cam_rotate != -42) || (thiscam == &camera2
		&& t_cam2_rotate != -42)))
	{
		angle = FixedAngle(camrotate*FRACUNIT);
		thiscam->angle = angle;
	}

	if (!objectplacing && !(twodlevel || (mo->flags2 & MF2_TWOD)) && !(player->pflags & PF_NIGHTSMODE) && displayplayer == consoleplayer)
	{
#ifdef REDSANALOG
		if ((player->cmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) == (BT_CAMLEFT|BT_CAMRIGHT)); else
#endif
		if (player->cmd.buttons & BT_CAMLEFT)
		{
			if (thiscam == &camera)
			{
				if (cv_analog.value || demoplayback)
					angle -= FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam_rotate, camrotate == 0 ? 358
						: camrotate - 2);
			}
			else
			{
				if (cv_analog2.value)
					angle -= FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam2_rotate, camrotate == 0 ? 358
						: camrotate - 2);
			}
		}
		else if (player->cmd.buttons & BT_CAMRIGHT)
		{
			if (thiscam == &camera)
			{
				if (cv_analog.value || demoplayback)
					angle += FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam_rotate, camrotate + 2);
			}
			else
			{
				if (cv_analog2.value)
					angle += FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam2_rotate, camrotate + 2);
			}
		}
	}

	// sets ideal cam pos
	if (twodlevel || (mo->flags2 & MF2_TWOD))
		dist = 480<<FRACBITS;
	else if (player->pflags & PF_NIGHTSMODE)
		dist = 320<<FRACBITS;
	else
	{
		dist = camdist;

		if (player->climbing || player->exiting || player->playerstate == PST_DEAD || (player->pflags & (PF_MACESPIN|PF_ITEMHANG|PF_ROPEHANG)))
			dist <<= 1;
	}

	checkdist = dist;

	if (checkdist < 128*FRACUNIT)
		checkdist = 128*FRACUNIT;

	x = mo->x - FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
	y = mo->y - FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);

#if 0
	if (twodlevel || (mo->flags2 & MF2_TWOD))
	{
		// Camera doesn't ALWAYS need to move, only when running...
		if (abs(player->mo->momx) > 10)
		{
			// Move the camera all smooth-like, not jerk it around...
			if (mo->momx > 0)
			{
				if (thiscam->relativex < MAXCAMERADIST)
					thiscam->relativex += 4*FRACUNIT;
			}
			else if (mo->momx < 0)
			{
				if (thiscam->relativex > -MAXCAMERADIST)
					thiscam->relativex -= 4*FRACUNIT;
			}
		}
		else // If speed is less than required, start moving the camera back.
		{
			if (thiscam->relativex > 0)
				thiscam->relativex -= 4*FRACUNIT;
			else if (thiscam->relativex < 0)
				thiscam->relativex += 4*FRACUNIT;
		}

		// Add the relative x to the global x
		x += thiscam->relativex;
		y += mo->momy << 1;
	}
#endif // bad 2D camera code

	pviewheight = FixedMul(cv_viewheight.value<<FRACBITS, mo->scale);

	if (mo->eflags & MFE_VERTICALFLIP)
		z = mo->z + mo->height - pviewheight - camheight;
	else
		z = mo->z + pviewheight + camheight;

	// move camera down to move under lower ceilings
	newsubsec = R_IsPointInSubsector(((mo->x>>FRACBITS) + (thiscam->x>>FRACBITS))<<(FRACBITS-1), ((mo->y>>FRACBITS) + (thiscam->y>>FRACBITS))<<(FRACBITS-1));

	if (!newsubsec)
		newsubsec = thiscam->subsector;

	if (newsubsec)
	{
		fixed_t myfloorz, myceilingz;
		fixed_t midz = thiscam->z + (thiscam->z - mo->z)/2;
		fixed_t midx = ((mo->x>>FRACBITS) + (thiscam->x>>FRACBITS))<<(FRACBITS-1);
		fixed_t midy = ((mo->y>>FRACBITS) + (thiscam->y>>FRACBITS))<<(FRACBITS-1);

		// Cameras use the heightsec's heights rather then the actual sector heights.
		// If you can see through it, why not move the camera through it too?
		if (newsubsec->sector->camsec >= 0)
		{
			myfloorz = sectors[newsubsec->sector->camsec].floorheight;
			myceilingz = sectors[newsubsec->sector->camsec].ceilingheight;
		}
		else if (newsubsec->sector->heightsec >= 0)
		{
			myfloorz = sectors[newsubsec->sector->heightsec].floorheight;
			myceilingz = sectors[newsubsec->sector->heightsec].ceilingheight;
		}
		else
		{
			myfloorz = P_CameraGetFloorZ(thiscam, newsubsec->sector, midx, midy, NULL);
			myceilingz = P_CameraGetCeilingZ(thiscam, newsubsec->sector, midx, midy, NULL);
		}

		// Check list of fake floors and see if floorz/ceilingz need to be altered.
		if (newsubsec->sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t delta1, delta2;
			INT32 thingtop = midz + thiscam->height;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				fixed_t topheight, bottomheight;
				if (!(rover->flags & FF_BLOCKOTHERS) || !(rover->flags & FF_EXISTS) || !(rover->flags & FF_RENDERALL) || GETSECSPECIAL(rover->master->frontsector->special, 4) == 12)
					continue;

				topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);
				bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);

				delta1 = midz - (bottomheight
					+ ((topheight - bottomheight)/2));
				delta2 = thingtop - (bottomheight
					+ ((topheight - bottomheight)/2));
				if (topheight > myfloorz && abs(delta1) < abs(delta2))
					myfloorz = topheight;
				if (bottomheight < myceilingz && abs(delta1) >= abs(delta2))
					myceilingz = bottomheight;
			}
		}

#ifdef POLYOBJECTS
	// Check polyobjects and see if floorz/ceilingz need to be altered
	{
		INT32 xl, xh, yl, yh, bx, by;
		validcount++;

		xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
		xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
		yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
		yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

		BMBOUNDFIX(xl, xh, yl, yh);

		for (by = yl; by <= yh; by++)
			for (bx = xl; bx <= xh; bx++)
			{
				INT32 offset;
				polymaplink_t *plink; // haleyjd 02/22/06

				if (bx < 0 || by < 0 || bx >= bmapwidth || by >= bmapheight)
					continue;

				offset = by*bmapwidth + bx;

				// haleyjd 02/22/06: consider polyobject lines
				plink = polyblocklinks[offset];

				while (plink)
				{
					polyobj_t *po = plink->po;

					if (po->validcount != validcount) // if polyobj hasn't been checked
					{
						sector_t *polysec;
						fixed_t delta1, delta2, thingtop;
						fixed_t polytop, polybottom;

						po->validcount = validcount;

						if (!P_PointInsidePolyobj(po, x, y) || !(po->flags & POF_SOLID))
						{
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						// We're inside it! Yess...
						polysec = po->lines[0]->backsector;

						if (GETSECSPECIAL(polysec->special, 4) == 12)
						{ // Camera noclip polyobj.
							plink = (polymaplink_t *)(plink->link.next);
							continue;
						}

						if (po->flags & POF_CLIPPLANES)
						{
							polytop = polysec->ceilingheight;
							polybottom = polysec->floorheight;
						}
						else
						{
							polytop = INT32_MAX;
							polybottom = INT32_MIN;
						}

						thingtop = midz + thiscam->height;
						delta1 = midz - (polybottom + ((polytop - polybottom)/2));
						delta2 = thingtop - (polybottom + ((polytop - polybottom)/2));

						if (polytop > myfloorz && abs(delta1) < abs(delta2))
							myfloorz = polytop;

						if (polybottom < myceilingz && abs(delta1) >= abs(delta2))
							myceilingz = polybottom;
					}
					plink = (polymaplink_t *)(plink->link.next);
				}
			}
	}
#endif

		// crushed camera
		if (myceilingz <= myfloorz + thiscam->height && !resetcalled && !cameranoclip)
		{
			P_ResetCamera(player, thiscam);
			return true;
		}

		// camera fit?
		if (myceilingz != myfloorz
			&& myceilingz - thiscam->height < z)
		{
/*			// no fit
			if (!resetcalled && !cameranoclip)
			{
				P_ResetCamera(player, thiscam);
				return true;
			}
*/
			z = myceilingz - thiscam->height-FixedMul(11*FRACUNIT, mo->scale);
			// is the camera fit is there own sector
		}

		// Make the camera a tad smarter with 3d floors
		if (newsubsec->sector->ffloors && !cameranoclip)
		{
			ffloor_t *rover;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				fixed_t topheight, bottomheight;
				if ((rover->flags & FF_BLOCKOTHERS) && (rover->flags & FF_RENDERALL) && (rover->flags & FF_EXISTS) && GETSECSPECIAL(rover->master->frontsector->special, 4) != 12)
				{
					topheight = P_CameraGetFOFTopZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);
					bottomheight = P_CameraGetFOFBottomZ(thiscam, newsubsec->sector, rover, midx, midy, NULL);

					if (bottomheight - thiscam->height < z
						&& midz < bottomheight)
						z = bottomheight - thiscam->height-FixedMul(11*FRACUNIT, mo->scale);

					else if (topheight + thiscam->height > z
						&& midz > topheight)
						z = topheight;

					if ((mo->z >= topheight && midz < bottomheight)
						|| ((mo->z < bottomheight && mo->z+mo->height < topheight) && midz >= topheight))
					{
						// Can't see
						if (!resetcalled)
							P_ResetCamera(player, thiscam);
						return true;
					}
				}
			}
		}
	}

	if (mo->type == MT_EGGTRAP)
		z = mo->z + 128*FRACUNIT + pviewheight + camheight;

	if (thiscam->z < thiscam->floorz && !cameranoclip)
		thiscam->z = thiscam->floorz;

	// point viewed by the camera
	// this point is just 64 unit forward the player
	dist = FixedMul(64 << FRACBITS, mo->scale);
	viewpointx = mo->x + FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
	viewpointy = mo->y + FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);

	if (!camstill && !resetcalled && !paused)
		thiscam->angle = R_PointToAngle2(thiscam->x, thiscam->y, viewpointx, viewpointy);

	viewpointx = mo->x + FixedMul(FINECOSINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);
	viewpointy = mo->y + FixedMul(FINESINE((angle>>ANGLETOFINESHIFT) & FINEMASK), dist);

/*
	if (twodlevel || (mo->flags2 & MF2_TWOD))
		thiscam->angle = angle;
*/
	// follow the player
	/*if (player->playerstate != PST_DEAD && (camspeed) != 0)
	{
		if (P_AproxDistance(mo->x - thiscam->x, mo->y - thiscam->y) > (checkdist + P_AproxDistance(mo->momx, mo->momy)) * 4
			|| abs(mo->z - thiscam->z) > checkdist * 3)
		{
			if (!resetcalled)
				P_ResetCamera(player, thiscam);
			return true;
		}
	}*/

	if (twodlevel || (mo->flags2 & MF2_TWOD))
	{
		thiscam->momx = x-thiscam->x;
		thiscam->momy = y-thiscam->y;
		thiscam->momz = z-thiscam->z;
	}
	else
	{
		thiscam->momx = FixedMul(x - thiscam->x, camspeed);
		thiscam->momy = FixedMul(y - thiscam->y, camspeed);

		if (GETSECSPECIAL(thiscam->subsector->sector->special, 1) == 6
			&& thiscam->z < thiscam->subsector->sector->floorheight + 256*FRACUNIT
			&& FixedMul(z - thiscam->z, camspeed) < 0)
		{
			thiscam->momz = 0; // Don't go down a death pit
		}
		else
			thiscam->momz = FixedMul(z - thiscam->z, camspeed);
	}

	// compute aming to look the viewed point
	f1 = viewpointx-thiscam->x;
	f2 = viewpointy-thiscam->y;
	dist = FixedHypot(f1, f2);

	if (mo->eflags & MFE_VERTICALFLIP)
		angle = R_PointToAngle2(0, thiscam->z + thiscam->height, dist, mo->z + mo->height - P_GetPlayerHeight(player));
	else
		angle = R_PointToAngle2(0, thiscam->z, dist, mo->z + P_GetPlayerHeight(player));
	if (player->playerstate != PST_DEAD && !(player->pflags & PF_NIGHTSMODE && player->exiting))
		angle += (focusaiming < ANGLE_180 ? focusaiming/2 : InvAngle(InvAngle(focusaiming)/2)); // overcomplicated version of '((signed)focusaiming)/2;'

	if (twodlevel || (mo->flags2 & MF2_TWOD) || !camstill) // Keep the view still...
	{
		G_ClipAimingPitch((INT32 *)&angle);
		dist = thiscam->aiming - angle;
		thiscam->aiming -= (dist>>3);
	}

	// Make player translucent if camera is too close (only in single player).
	if (!(multiplayer || netgame) && !splitscreen)
	{
		fixed_t vx = 0, vy = 0;
		if (player->awayviewtics) {
			vx = player->awayviewmobj->x;
			vy = player->awayviewmobj->y;
		}
		else
		{
			vx = thiscam->x;
			vy = thiscam->y;
		}

		if (P_AproxDistance(vx - player->mo->x, vy - player->mo->y) < FixedMul(48*FRACUNIT, mo->scale))
			player->mo->flags2 |= MF2_SHADOW;
		else
			player->mo->flags2 &= ~MF2_SHADOW;
	}
	else
		player->mo->flags2 &= ~MF2_SHADOW;

/*	if (!resetcalled && (player->pflags & PF_NIGHTSMODE && player->exiting))
	{
		// Don't let the camera match your movement.
		thiscam->momz = 0;

		// Only let the camera go a little bit upwards.
		if (mo->eflags & MFE_VERTICALFLIP && thiscam->aiming < ANGLE_315 && thiscam->aiming > ANGLE_180)
			thiscam->aiming = ANGLE_315;
		else if (!(mo->eflags & MFE_VERTICALFLIP) && thiscam->aiming > ANGLE_45 && thiscam->aiming < ANGLE_180)
			thiscam->aiming = ANGLE_45;
	}
	else */if (!resetcalled && (player->playerstate == PST_DEAD || player->playerstate == PST_REBORN))
	{
		// Don't let the camera match your movement.
		thiscam->momz = 0;

		// Only let the camera go a little bit downwards.
		if (!(mo->eflags & MFE_VERTICALFLIP) && thiscam->aiming < ANGLE_337h && thiscam->aiming > ANGLE_180)
			thiscam->aiming = ANGLE_337h;
		else if (mo->eflags & MFE_VERTICALFLIP && thiscam->aiming > ANGLE_22h && thiscam->aiming < ANGLE_180)
			thiscam->aiming = ANGLE_22h;
	}

	return (x == thiscam->x && y == thiscam->y && z == thiscam->z && angle == thiscam->aiming);
}

static boolean P_SpectatorJoinGame(player_t *player)
{
	if (!G_GametypeHasSpectators() && G_IsSpecialStage(gamemap) && useNightsSS) // Special Stage spectators should NEVER be allowed to rejoin the game
	{
		if (P_IsLocalPlayer(player))
			CONS_Printf(M_GetText("You cannot enter the game while a special stage is in progress.\n"));
		player->powers[pw_flashing] += 2*TICRATE; //to prevent message spam.
	}

	else if (!cv_allowteamchange.value)
	{
		if (P_IsLocalPlayer(player))
			CONS_Printf(M_GetText("Server does not allow team change.\n"));
		player->powers[pw_flashing] += 2*TICRATE; //to prevent message spam.
	}

	// Team changing in Team Match and CTF
	// Pressing fire assigns you to a team that needs players if allowed.
	// Partial code reproduction from p_tick.c autobalance code.
	else if (G_GametypeHasTeams())
	{
		INT32 changeto = 0;
		INT32 z, numplayersred = 0, numplayersblue = 0;

		//find a team by num players, score, or random if all else fails.
		for (z = 0; z < MAXPLAYERS; ++z)
			if (playeringame[z])
			{
				if (players[z].ctfteam == 1)
					++numplayersred;
				else if (players[z].ctfteam == 2)
					++numplayersblue;
			}
		// for z

		if (numplayersblue > numplayersred)
			changeto = 1;
		else if (numplayersred > numplayersblue)
			changeto = 2;
		else if (bluescore > redscore)
			changeto = 1;
		else if (redscore > bluescore)
			changeto = 2;
		else
			changeto = (P_RandomFixed() & 1) + 1;

		if (player->mo)
		{
			P_RemoveMobj(player->mo);
			player->mo = NULL;
		}
		player->spectator = false;
		player->ctfteam = changeto;
		player->playerstate = PST_REBORN;

		//Reset away view
		if (P_IsLocalPlayer(player) && displayplayer != consoleplayer)
			displayplayer = consoleplayer;

		if (changeto == 1)
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[player-players], '\x85', M_GetText("Red team"), '\x80');
		else if (changeto == 2)
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[player-players], '\x84', M_GetText("Blue team"), '\x80');

		return true; // no more player->mo, cannot continue.
	}
	// Joining in game from firing.
	else
	{
		// Exception for hide and seek. Don't join a game when you simply
		// respawn in place and sit there for the rest of the round.
		if (!(gametype == GT_HIDEANDSEEK && leveltime > (hidetime * TICRATE)))
		{
			if (player->mo)
			{
				P_RemoveMobj(player->mo);
				player->mo = NULL;
			}
			player->spectator = false;
			player->playerstate = PST_REBORN;

			if (gametype == GT_TAG)
			{
				//Make joining players "it" after hidetime.
				if (leveltime > (hidetime * TICRATE))
				{
					CONS_Printf(M_GetText("%s is now IT!\n"), player_names[player-players]); // Tell everyone who is it!
					player->pflags |= PF_TAGIT;
				}

				P_CheckSurvivors();
			}

			//Reset away view
			if (P_IsLocalPlayer(player) && displayplayer != consoleplayer)
				displayplayer = consoleplayer;

			CONS_Printf(M_GetText("%s entered the game.\n"), player_names[player-players]);
			return true; // no more player->mo, cannot continue.
		}
		else
		{
			if (P_IsLocalPlayer(player))
				CONS_Printf(M_GetText("You must wait until next round to enter the game.\n"));
			player->powers[pw_flashing] += 2*TICRATE; //to prevent message spam.
		}
	}
	return false;
}

static void P_CalcPostImg(player_t *player)
{
	sector_t *sector = player->mo->subsector->sector;
	postimg_t *type;
	INT32 *param;
	fixed_t pviewheight;

	if (player->mo->eflags & MFE_VERTICALFLIP)
		pviewheight = player->mo->z + player->mo->height - player->viewheight;
	else
		pviewheight = player->mo->z + player->viewheight;

	if (player->awayviewtics)
	{
		sector = player->awayviewmobj->subsector->sector;
		pviewheight = player->awayviewmobj->z + 20*FRACUNIT;
	}

	if (splitscreen && player == &players[secondarydisplayplayer])
	{
		type = &postimgtype2;
		param = &postimgparam2;
	}
	else
	{
		type = &postimgtype;
		param = &postimgparam;
	}

	// see if we are in heat (no, not THAT kind of heat...)

	if (P_FindSpecialLineFromTag(13, sector->tag, -1) != -1)
		*type = postimg_heat;
	else if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;
		fixed_t bottomheight;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

#ifdef ESLOPE
			topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, player->mo->x, player->mo->y) : *rover->topheight;
			bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, player->mo->x, player->mo->y) : *rover->bottomheight;
#else
			topheight = *rover->topheight;
			bottomheight = *rover->bottomheight;
#endif

			if (pviewheight >= topheight || pviewheight <= bottomheight)
				continue;

			if (P_FindSpecialLineFromTag(13, rover->master->frontsector->tag, -1) != -1)
				*type = postimg_heat;
		}
	}

	// see if we are in water (water trumps heat)
	if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;
		fixed_t bottomheight;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKPLAYER)
				continue;

#ifdef ESLOPE
			topheight = *rover->t_slope ? P_GetZAt(*rover->t_slope, player->mo->x, player->mo->y) : *rover->topheight;
			bottomheight = *rover->b_slope ? P_GetZAt(*rover->b_slope, player->mo->x, player->mo->y) : *rover->bottomheight;
#else
			topheight = *rover->topheight;
			bottomheight = *rover->bottomheight;
#endif

			if (pviewheight >= topheight || pviewheight <= bottomheight)
				continue;

			*type = postimg_water;
		}
	}

	if (player->mo->eflags & MFE_VERTICALFLIP)
		*type = postimg_flip;

#if 1
	(void)param;
#else
	// Motion blur
	if (player->speed > (35<<FRACBITS))
	{
		*type = postimg_motion;
		*param = (player->speed - 32)/4;

		if (*param > 5)
			*param = 5;
	}
#endif
}

void P_DoPityCheck(player_t *player)
{
	// No pity outside of match or CTF.
	if (player->spectator
		|| !(gametype == GT_MATCH || gametype == GT_TEAMMATCH || gametype == GT_CTF))
		return;

	// Apply pity shield if available.
	if ((player->pity >= 3 || player->pity < 0) && player->powers[pw_shield] == SH_NONE)
	{
		if (player->pity > 0)
			S_StartSound(player->mo, mobjinfo[MT_PITYSHIELDICO].seesound);

		player->pity = 0;
		player->powers[pw_shield] = SH_PITY;
		P_SpawnShieldOrb(player);
	}
}

//
// P_PlayerThink
//

boolean playerdeadview; // show match/chaos/tag/capture the flag rankings while in death view

void P_PlayerThink(player_t *player)
{
	ticcmd_t *cmd;
	const size_t playeri = (size_t)(player - players);

#ifdef PARANOIA
	if (!player->mo)
		I_Error("p_playerthink: players[%s].mo == NULL", sizeu1(playeri));
#endif

	// todo: Figure out what is actually causing these problems in the first place...
	if ((player->health <= 0 || player->mo->health <= 0) && player->playerstate == PST_LIVE) //you should be DEAD!
	{
		CONS_Debug(DBG_GAMELOGIC, "P_PlayerThink: Player %s in PST_LIVE with 0 health. (\"Zombie bug\")\n", sizeu1(playeri));
		player->playerstate = PST_DEAD;
	}

	if (player->bot)
	{
		if (player->playerstate == PST_LIVE && B_CheckRespawn(player))
			player->playerstate = PST_REBORN;
		if (player->playerstate == PST_REBORN)
			return;
	}

#ifdef SEENAMES
	if (netgame && player == &players[displayplayer] && !(leveltime % (TICRATE/5)))
	{
		seenplayer = NULL;

		if (cv_seenames.value && cv_allowseenames.value &&
			!(G_TagGametype() && (player->pflags & PF_TAGIT)))
		{
			mobj_t *mo = P_SpawnNameFinder(player->mo, MT_NAMECHECK);

			if (mo)
			{
				short int i;
				mo->flags |= MF_NOCLIPHEIGHT;
				for (i = 0; i < 32; i++)
				{
					// Debug drawing
//					if (i&1)
//						P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);
					if (P_RailThinker(mo))
						break; // mobj was removed (missile hit a wall) or couldn't move
				}
			}
		}
	}
#endif
	if (player->pflags & PF_GLIDING)
	{
		if (player->panim != PA_ABILITY)
			P_SetPlayerMobjState(player->mo, S_PLAY_ABL1);
	}
	else if ((player->pflags & PF_JUMPED) && !player->powers[pw_super] && player->panim != PA_ROLL && player->charability2 == CA2_SPINDASH)
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);

	if (player->flashcount)
		player->flashcount--;

	if (player->awayviewtics)
		player->awayviewtics--;

	/// \note do this in the cheat code
	if (player->pflags & PF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else
		player->mo->flags &= ~MF_NOCLIP;

	cmd = &player->cmd;

	// Add some extra randomization.
	if (cmd->forwardmove)
		P_RandomFixed();

#ifdef PARANOIA
	if (player->playerstate == PST_REBORN)
		I_Error("player %s is in PST_REBORN\n", sizeu1(playeri));
#endif

	if (gametype == GT_RACE || gametype == GT_COMPETITION)
	{
		INT32 i;

		// Check if all the players in the race have finished. If so, end the level.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (!players[i].exiting && players[i].lives > 0)
					break;
			}
		}

		if (i == MAXPLAYERS && player->exiting == 3*TICRATE) // finished
			player->exiting = (14*TICRATE)/5 + 1;

		// If 10 seconds are left on the timer,
		// begin the drown music for countdown!
		if (countdown == 11*TICRATE - 1)
		{
			if (P_IsLocalPlayer(player))
				S_ChangeMusicInternal("drown", false);
		}

		// If you've hit the countdown and you haven't made
		//  it to the exit, you're a goner!
		else if (countdown == 1 && !player->exiting && player->lives > 0)
		{
			if (netgame && player->health > 0)
				CONS_Printf(M_GetText("%s ran out of time.\n"), player_names[player-players]);

			player->pflags |= PF_TIMEOVER;

			if (player->pflags & PF_NIGHTSMODE)
			{
				P_DeNightserizePlayer(player);
				S_StartScreamSound(player->mo, sfx_s3k66);
			}

			player->lives = 2; // Don't start the game over music!
			P_DamageMobj(player->mo, NULL, NULL, 10000);
			player->lives = 0;

			if (player->playerstate == PST_DEAD)
				return;
		}
	}

	// If it is set, start subtracting
	// Don't allow it to go back to 0
	if (player->exiting > 1 && player->exiting < 3*TICRATE)
		player->exiting--;

	if (player->exiting && countdown2)
		player->exiting = 5;

	if (player->exiting == 2 || countdown2 == 2)
	{
		if (cv_playersforexit.value) // Count to be sure everyone's exited
		{
			INT32 i;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator || players[i].bot)
					continue;
				if (players[i].lives <= 0)
					continue;

				if (!players[i].exiting || players[i].exiting > 3)
					break;
			}

			if (i == MAXPLAYERS)
			{
				if (server)
					SendNetXCmd(XD_EXITLEVEL, NULL, 0);
			}
			else
				player->exiting = 3;
		}
		else
		{
			if (server)
				SendNetXCmd(XD_EXITLEVEL, NULL, 0);
		}
	}

	// check water content, set stuff in mobj
	P_MobjCheckWater(player->mo);

#ifndef SECTORSPECIALSAFTERTHINK
#ifdef POLYOBJECTS
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
#endif
	player->onconveyor = 0;
	// check special sectors : damage & secrets

	if (!player->spectator)
		P_PlayerInSpecialSector(player);
#endif

	if (player->playerstate == PST_DEAD)
	{
		player->mo->flags2 &= ~MF2_SHADOW;
		// show the multiplayer rankings while dead
		if (player == &players[displayplayer])
			playerdeadview = true;

		P_DeathThink(player);

		return;
	}

	// Make sure spectators always have a score and ring count of 0.
	if (player->spectator)
	{
		player->score = 0;
		player->mo->health = 1;
		player->health = 1;
	}

	if ((netgame || multiplayer) && player->lives <= 0)
	{
		// In Co-Op, replenish a user's lives if they are depleted.
		// of course, this is just a cheap hack, meh...
		player->lives = cv_startinglives.value;
	}

	if (player == &players[displayplayer])
		playerdeadview = false;

	if ((gametype == GT_RACE || gametype == GT_COMPETITION) && leveltime < 4*TICRATE)
	{
		cmd->buttons &= BT_USE; // Remove all buttons except BT_USE
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
	}

	// Synchronizes the "real" amount of time spent in the level.
	if (!player->exiting)
	{
		if (gametype == GT_RACE || gametype == GT_COMPETITION)
		{
			if (leveltime >= 4*TICRATE)
				player->realtime = leveltime - 4*TICRATE;
			else
				player->realtime = 0;
		}
		else
			player->realtime = leveltime;
	}

	if ((netgame || splitscreen) && player->spectator && cmd->buttons & BT_ATTACK && !player->powers[pw_flashing])
	{
		if (P_SpectatorJoinGame(player))
			return; // player->mo was removed.
	}

	// Even if not NiGHTS, pull in nearby objects when walking around as John Q. Elliot.
	if (!objectplacing && !((netgame || multiplayer) && player->spectator)
	&& maptol & TOL_NIGHTS && (!(player->pflags & PF_NIGHTSMODE) || player->powers[pw_nights_helper]))
	{
		thinker_t *th;
		mobj_t *mo2;
		fixed_t x = player->mo->x;
		fixed_t y = player->mo->y;
		fixed_t z = player->mo->z;

		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (!(mo2->type == MT_NIGHTSWING || mo2->type == MT_RING || mo2->type == MT_COIN
			   || mo2->type == MT_BLUEBALL))
				continue;

			if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > FixedMul(128*FRACUNIT, player->mo->scale))
				continue;

			// Yay! The thing's in reach! Pull it in!
			mo2->flags |= MF_NOCLIP|MF_NOCLIPHEIGHT;
			mo2->flags2 |= MF2_NIGHTSPULL;
			P_SetTarget(&mo2->tracer, player->mo);
		}
	}

	if (player->linktimer && !player->powers[pw_nights_linkfreeze])
	{
		if (--player->linktimer <= 0) // Link timer
			player->linkcount = 0;
	}

	// Move around.
	// Reactiontime is used to prevent movement
	//  for a bit after a teleport.
	if (player->mo->reactiontime)
		player->mo->reactiontime--;
	else if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT)
	{
		if (player->pflags & PF_ROPEHANG)
		{
			if (!P_AnalogMove(player))
				player->mo->angle = (cmd->angleturn<<16 /* not FRACBITS */);

			ticruned++;
			if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
				ticmiss++;

			P_DoRopeHang(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_CARRY);
			P_DoJumpStuff(player, &player->cmd);
		}
		else
		{
			P_DoZoomTube(player);
			if (!(player->panim == PA_ROLL) && player->charability2 == CA2_SPINDASH)
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		}
		player->rmomx = player->rmomy = 0; // no actual momentum from your controls
		P_ResetScore(player);
	}
	else
		P_MovePlayer(player);

	if (!player->mo)
		return; // P_MovePlayer removed player->mo.

	// Unset statis flags after moving.
	// In other words, if you manually set stasis via code,
	// it lasts for one tic.
	player->pflags &= ~PF_FULLSTASIS;

#ifdef POLYOBJECTS
	if (player->onconveyor == 1)
			player->cmomy = player->cmomx = 0;
#endif

	P_DoSuperStuff(player);
	P_CheckSneakerAndLivesTimer(player);
	P_DoBubbleBreath(player); // Spawn Sonic's bubbles
	P_CheckUnderwaterAndSpaceTimer(player); // Display the countdown drown numbers!
	P_CheckInvincibilityTimer(player); // Spawn Invincibility Sparkles
	P_DoPlayerHeadSigns(player); // Spawn Tag/CTF signs over player's head

#if 1
	// "Blur" a bit when you have speed shoes and are going fast enough
	if ((player->powers[pw_super] || player->powers[pw_sneakers]) && (player->speed + abs(player->mo->momz)) > FixedMul(20*FRACUNIT,player->mo->scale))
	{
		mobj_t *gmobj = P_SpawnGhostMobj(player->mo);
		gmobj->fuse = 2;
		if (leveltime & 1)
		{
			gmobj->frame &= ~FF_TRANSMASK;
			gmobj->frame |= tr_trans70<<FF_TRANSSHIFT;
		}

		// Hide the mobj from our sights if we're the displayplayer and chasecam is off,
		// or secondarydisplayplayer and chasecam2 is off.
		// Why not just not spawn the mobj?  Well, I'd rather only flirt with
		// consistency so much...
		if ((player == &players[displayplayer] && !camera.chase)
		|| (splitscreen && player == &players[secondarydisplayplayer] && !camera2.chase))
			gmobj->flags2 |= MF2_DONTDRAW;
	}
#endif

	// check for use
	if (!(player->pflags & PF_NIGHTSMODE))
	{
		if (cmd->buttons & BT_USE)
			player->pflags |= PF_USEDOWN;
		else
			player->pflags &= ~PF_USEDOWN;
	}
	else if (player->mo->tracer) // match tracer's position with yours when NiGHTS
	{
		P_UnsetThingPosition(player->mo->tracer);
		player->mo->tracer->x = player->mo->x;
		player->mo->tracer->y = player->mo->y;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			player->mo->tracer->z = player->mo->z + player->mo->height - player->mo->tracer->height;
		else
			player->mo->tracer->z = player->mo->z;
		player->mo->tracer->floorz = player->mo->floorz;
		player->mo->tracer->ceilingz = player->mo->ceilingz;
		P_SetThingPosition(player->mo->tracer);
	}

	// Counters, time dependent power ups.
	// Time Bonus & Ring Bonus count settings

	// Strength counts up to diminish fade.
	if (player->powers[pw_sneakers] && player->powers[pw_sneakers] < UINT16_MAX)
		player->powers[pw_sneakers]--;

	if (player->powers[pw_invulnerability] && player->powers[pw_invulnerability] < UINT16_MAX)
		player->powers[pw_invulnerability]--;

	if (player->powers[pw_flashing] && player->powers[pw_flashing] < UINT16_MAX && ((player->pflags & PF_NIGHTSMODE) || player->powers[pw_flashing] < flashingtics))
		player->powers[pw_flashing]--;

	if (player->powers[pw_tailsfly] && player->powers[pw_tailsfly] < UINT16_MAX && player->charability != CA_SWIM && !(player->powers[pw_super] && ALL7EMERALDS(player->powers[pw_emeralds]))) // tails fly counter
		player->powers[pw_tailsfly]--;

	if (player->powers[pw_underwater] && (player->pflags & PF_GODMODE || (player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL))
	{
		if (player->powers[pw_underwater] <= 12*TICRATE+1)
			P_RestoreMusic(player); //incase they were about to drown

		player->powers[pw_underwater] = 0;
	}
	else if (player->powers[pw_underwater] && !(maptol & TOL_NIGHTS) && !((netgame || multiplayer) && player->spectator)) // underwater timer
		player->powers[pw_underwater]--;

	if (player->powers[pw_spacetime] && (player->pflags & PF_GODMODE || (player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL))
		player->powers[pw_spacetime] = 0;
	else if (player->powers[pw_spacetime] && !(maptol & TOL_NIGHTS) && !((netgame || multiplayer) && player->spectator)) // underwater timer
		player->powers[pw_spacetime]--;

	if (player->powers[pw_gravityboots] && player->powers[pw_gravityboots] < UINT16_MAX)
		player->powers[pw_gravityboots]--;

	if (player->powers[pw_extralife] && player->powers[pw_extralife] < UINT16_MAX)
		player->powers[pw_extralife]--;

	if (player->powers[pw_nights_linkfreeze] && player->powers[pw_nights_linkfreeze] < UINT16_MAX)
		player->powers[pw_nights_linkfreeze]--;

	if (player->powers[pw_nights_superloop] && player->powers[pw_nights_superloop] < UINT16_MAX)
		player->powers[pw_nights_superloop]--;

	if (player->powers[pw_nights_helper] && player->powers[pw_nights_helper] < UINT16_MAX)
		player->powers[pw_nights_helper]--;

	if (player->powers[pw_nocontrol] & ((1<<15)-1) && player->powers[pw_nocontrol] < UINT16_MAX)
		player->powers[pw_nocontrol]--;
	else
		player->powers[pw_nocontrol] = 0;

	//pw_super acts as a timer now
	if (player->powers[pw_super])
		player->powers[pw_super]++;

	if (player->powers[pw_ingoop])
	{
		if (player->mo->state == &states[S_PLAY_STND])
			player->mo->tics = 2;

		player->powers[pw_ingoop]--;
	}

	if (player->bumpertime)
		player->bumpertime--;

	if (player->skidtime)
		player->skidtime--;

	if (player->weapondelay)
		player->weapondelay--;

	if (player->tossdelay)
		player->tossdelay--;

	if (player->homing)
		player->homing--;

	if (player->texttimer)
	{
		--player->texttimer;
		if (!player->texttimer && !player->exiting && player->textvar >= 4)
		{
			player->texttimer = 4*TICRATE;
			player->textvar = 2; // GET n RINGS!

			if (player->capsule && player->capsule->health != player->capsule->spawnpoint->angle)
				player->textvar++; // GET n MORE RINGS!
		}
	}

	if (player->losstime && !player->powers[pw_flashing])
		player->losstime--;

	// Flash player after being hit.
	if (!(player->pflags & PF_NIGHTSMODE))
	{
		if (player->powers[pw_flashing] > 0 && player->powers[pw_flashing] < flashingtics && (leveltime & 1))
			player->mo->flags2 |= MF2_DONTDRAW;
		else
			player->mo->flags2 &= ~MF2_DONTDRAW;
	}
	else if (player->mo->tracer)
	{
		if (player->powers[pw_flashing] & 1)
			player->mo->tracer->flags2 |= MF2_DONTDRAW;
		else
			player->mo->tracer->flags2 &= ~MF2_DONTDRAW;
	}

	player->pflags &= ~PF_SLIDING;

/*
//	Colormap verification
	{
		INT32 i,j;
		sector_t *controlsec;
		for (j=0; j<numsectors; j++)
		{
			controlsec = NULL;
			// Does this sector have a water linedef?
			for (i=0; i<numlines;i++)
			{
				if ((lines[i].special == 121 || lines[i].special == 123)
				&& lines[i].tag == sectors[j].tag)
				{
					controlsec = lines[i].frontsector;
					break;
				}
			}

			if (i < numlines && controlsec)
			{
				// Does this sector have a colormap?
				for (i=0; i<numlines;i++)
				{
					if (lines[i].special == 606 && lines[i].tag == controlsec->tag)
						break;
				}

				if (i == numlines)
					CONS_Debug(DBG_GAMELOGIC, "%d, %d\n", j, sectors[j].tag);
			}
		}

		I_Error("I'm done!\n");
	}*/
}

//
// P_PlayerAfterThink
//
// Thinker for player after all other thinkers have run
//
void P_PlayerAfterThink(player_t *player)
{
	ticcmd_t *cmd;
	INT32 oldweapon = player->currentweapon;
	camera_t *thiscam = NULL; // if not one of the displayed players, just don't bother

#ifdef PARANOIA
	if (!player->mo)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("P_PlayerAfterThink: players[%s].mo == NULL", sizeu1(playeri));
	}
#endif

	cmd = &player->cmd;

#ifdef SECTORSPECIALSAFTERTHINK
#ifdef POLYOBJECTS
	if (player->onconveyor != 1 || !P_IsObjectOnGround(player->mo))
#endif
	player->onconveyor = 0;
	// check special sectors : damage & secrets

	if (!player->spectator)
		P_PlayerInSpecialSector(player);
#endif

	if (splitscreen && player == &players[secondarydisplayplayer])
		thiscam = &camera2;
	else if (player == &players[displayplayer])
		thiscam = &camera;

	if (player->playerstate == PST_DEAD)
	{
		// camera may still move when guy is dead
		//if (!netgame)
		{
			if (thiscam && thiscam->chase)
				P_MoveChaseCamera(player, thiscam, false);
		}
		return;
	}

	if (player->pflags & PF_NIGHTSMODE)
	{
		player->powers[pw_gravityboots] = 0;
		//player->mo->eflags &= ~MFE_VERTICALFLIP;
	}

	if (!(player->pflags & PF_WPNDOWN))
	{
		if (cmd->buttons & BT_WEAPONNEXT)
		{
			player->currentweapon++;
			player->currentweapon %= NUM_WEAPONS;
			player->pflags |= PF_WPNDOWN;
		}

		if (cmd->buttons & BT_WEAPONPREV)
		{
			player->currentweapon--;
			if (player->currentweapon < 0)
				player->currentweapon = NUM_WEAPONS - 1;
			player->pflags |= PF_WPNDOWN;

			if (player->currentweapon == WEP_RAIL && (!(player->ringweapons & RW_RAIL) || !player->powers[pw_railring]))
				player->currentweapon--;
			if (player->currentweapon == WEP_EXPLODE && (!(player->ringweapons & RW_EXPLODE) || !player->powers[pw_explosionring]))
				player->currentweapon--;
			if (player->currentweapon == WEP_GRENADE && (!(player->ringweapons & RW_GRENADE) || !player->powers[pw_grenadering]))
				player->currentweapon--;
			if (player->currentweapon == WEP_SCATTER && (!(player->ringweapons & RW_SCATTER) || !player->powers[pw_scatterring]))
				player->currentweapon--;
			if (player->currentweapon == WEP_BOUNCE && (!(player->ringweapons & RW_BOUNCE) || !player->powers[pw_bouncering]))
				player->currentweapon--;
			if (player->currentweapon == WEP_AUTO && (!(player->ringweapons & RW_AUTO) || !player->powers[pw_automaticring]))
				player->currentweapon = 0;
		}

		if (cmd->buttons & BT_WEAPONMASK)
		{
			//Read the bits to determine individual weapon ring selection.
			INT32 weapon = (cmd->buttons & BT_WEAPONMASK);

			switch (weapon)
			{
			case 1: //normal / infinity
				player->currentweapon = 0;
				player->pflags |= PF_WPNDOWN;
				break;
			case 2: //automatic
				if ((player->ringweapons & RW_AUTO) && player->powers[pw_automaticring])
				{
					player->currentweapon = WEP_AUTO;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 3: //bounce
				if ((player->ringweapons & RW_BOUNCE) && player->powers[pw_bouncering])
				{
					player->currentweapon = WEP_BOUNCE;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 4: //scatter
				if ((player->ringweapons & RW_SCATTER) && player->powers[pw_scatterring])
				{
					player->currentweapon = WEP_SCATTER;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 5: //grenade
				if ((player->ringweapons & RW_GRENADE) && player->powers[pw_grenadering])
				{
					player->currentweapon = WEP_GRENADE;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 6: //explosion
				if ((player->ringweapons & RW_EXPLODE) && player->powers[pw_explosionring])
				{
					player->currentweapon = WEP_EXPLODE;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			case 7: //rail
				if ((player->ringweapons & RW_RAIL) && player->powers[pw_railring])
				{
					player->currentweapon = WEP_RAIL;
					player->pflags |= PF_WPNDOWN;
				}
				break;
			}
		}
	}

	if (!(cmd->buttons & (BT_WEAPONNEXT|BT_WEAPONPREV|BT_WEAPONMASK)))
		player->pflags &= ~PF_WPNDOWN;

	// Weapon cycling if out of ammo for a certain weapon
	if (player->currentweapon == WEP_AUTO && (!(player->ringweapons & RW_AUTO) || !player->powers[pw_automaticring]))
		player->currentweapon++;
	if (player->currentweapon == WEP_BOUNCE && (!(player->ringweapons & RW_BOUNCE) || !player->powers[pw_bouncering]))
		player->currentweapon++;
	if (player->currentweapon == WEP_SCATTER && (!(player->ringweapons & RW_SCATTER) || !player->powers[pw_scatterring]))
		player->currentweapon++;
	if (player->currentweapon == WEP_GRENADE && (!(player->ringweapons & RW_GRENADE) || !player->powers[pw_grenadering]))
		player->currentweapon++;
	if (player->currentweapon == WEP_EXPLODE && (!(player->ringweapons & RW_EXPLODE) || !player->powers[pw_explosionring]))
		player->currentweapon++;
	if (player->currentweapon == WEP_RAIL && (!(player->ringweapons & RW_RAIL) || !player->powers[pw_railring]))
		player->currentweapon = 0;

	// If you're out of rings, but have Infinity Rings left, switch to that.
	if (player->currentweapon != 0 && player->health <= 1 && player->powers[pw_infinityring])
		player->currentweapon = 0;

	if (P_IsLocalPlayer(player) && (player->pflags & PF_WPNDOWN) && player->currentweapon != oldweapon)
		S_StartSound(NULL, sfx_wepchg);

	if (player->pflags & PF_GLIDING)
	{
		if (player->panim != PA_ABILITY)
			P_SetPlayerMobjState(player->mo, S_PLAY_ABL1);
	}
	else if (player->pflags & PF_SLIDING)
		P_SetPlayerMobjState(player->mo, player->mo->info->painstate);
	else if (player->pflags & PF_JUMPED
	&& ((!player->powers[pw_super] && player->panim != PA_ROLL)
	|| player->mo->state == &states[player->mo->info->painstate])
	&& player->charability2 == CA2_SPINDASH)
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);

	if (player->pflags & PF_CARRIED && player->mo->tracer)
	{
		player->mo->height = FixedDiv(P_GetPlayerHeight(player), FixedDiv(14*FRACUNIT,10*FRACUNIT));

		// State check for the carrier - Flame
		// You are an IDIOT, those aren't even the right frames! >_> - JTE
		if (player->mo->tracer->player
			&& !(player->mo->tracer->state >= &states[S_PLAY_ABL1]
			&& player->mo->tracer->state <= &states[S_PLAY_SPC4]))
				player->pflags &= ~PF_CARRIED;

		if (player->mo->eflags & MFE_VERTICALFLIP)
		{
			if ((player->mo->tracer->z + player->mo->tracer->height + player->mo->height + FixedMul(FRACUNIT, player->mo->scale)) <= player->mo->tracer->ceilingz
				&& (player->mo->tracer->eflags & MFE_VERTICALFLIP)) // Reverse gravity check for the carrier - Flame
				player->mo->z = player->mo->tracer->z + player->mo->tracer->height + FixedMul(FRACUNIT, player->mo->scale);
			else
				player->pflags &= ~PF_CARRIED;
		}
		else
		{
			if ((player->mo->tracer->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale)) >= player->mo->tracer->floorz
				&& !(player->mo->tracer->eflags & MFE_VERTICALFLIP)) // Correct gravity check for the carrier - Flame
				player->mo->z = player->mo->tracer->z - player->mo->height - FixedMul(FRACUNIT, player->mo->scale);
			else
				player->pflags &= ~PF_CARRIED;
		}

		if (player->mo->tracer->health <= 0)
			player->pflags &= ~PF_CARRIED;
		else
		{
			P_TryMove(player->mo, player->mo->tracer->x, player->mo->tracer->y, true);
			player->mo->momx = player->mo->tracer->momx;
			player->mo->momy = player->mo->tracer->momy;
			player->mo->momz = player->mo->tracer->momz;
		}

		if (gametype == GT_COOP)
		{
			player->mo->angle = player->mo->tracer->angle;

			if (!demoplayback || P_AnalogMove(player))
			{
				if (player == &players[consoleplayer])
					localangle = player->mo->angle;
				else if (player == &players[secondarydisplayplayer])
					localangle2 = player->mo->angle;
			}
		}

		if (P_AproxDistance(player->mo->x - player->mo->tracer->x, player->mo->y - player->mo->tracer->y) > player->mo->radius)
			player->pflags &= ~PF_CARRIED;

		P_SetPlayerMobjState(player->mo, S_PLAY_CARRY);

		if (player-players == consoleplayer && botingame)
			CV_SetValue(&cv_analog2, !(player->pflags & PF_CARRIED));
	}
	else if (player->pflags & PF_ITEMHANG && player->mo->tracer)
	{
		// tracer is what you're hanging onto
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->tracer->x;
		player->mo->y = player->mo->tracer->y;
		if (player->mo->eflags & MFE_VERTICALFLIP)
			player->mo->z = player->mo->tracer->z + player->mo->tracer->height - FixedDiv(player->mo->height, 3*FRACUNIT);
		else
			player->mo->z = player->mo->tracer->z - FixedDiv(player->mo->height, 3*FRACUNIT/2);
		player->mo->momx = player->mo->momy = player->mo->momz = 0;
		P_SetThingPosition(player->mo);
		P_SetPlayerMobjState(player->mo, S_PLAY_CARRY);

		// Controllable missile
		if (player->mo->tracer->type == MT_BLACKEGGMAN_MISSILE)
		{
			if (cmd->forwardmove > 0)
				player->mo->tracer->momz += FixedMul(FRACUNIT/4, player->mo->tracer->scale);
			else if (cmd->forwardmove < 0)
				player->mo->tracer->momz -= FixedMul(FRACUNIT/4, player->mo->tracer->scale);

			player->mo->tracer->angle = player->mo->angle;
			P_InstaThrust(player->mo->tracer, player->mo->tracer->angle, FixedMul(player->mo->tracer->info->speed, player->mo->tracer->scale));

			if (player->mo->z <= player->mo->floorz
				|| player->mo->tracer->health <= 0)
			{
				player->pflags &= ~PF_ITEMHANG;
				P_SetTarget(&player->mo->tracer, NULL);
			}
		}
	}
	else if (player->pflags & PF_MACESPIN && player->mo->tracer && player->mo->tracer->target)
	{
		player->mo->height = P_GetPlayerSpinHeight(player);
		// tracer is what you're hanging onto....
		player->mo->momx = (player->mo->tracer->x - player->mo->x)*2;
		player->mo->momy = (player->mo->tracer->y - player->mo->y)*2;
		player->mo->momz = (player->mo->tracer->z - (player->mo->height-player->mo->tracer->height/2) - player->mo->z)*2;
		P_TeleportMove(player->mo, player->mo->tracer->x, player->mo->tracer->y, player->mo->tracer->z - (player->mo->height-player->mo->tracer->height/2));
		player->pflags |= PF_JUMPED;
		player->secondjump = 0;
		player->pflags &= ~PF_THOKKED;

		if (cmd->forwardmove > 0)
			player->mo->tracer->target->lastlook += 2;
		else if (cmd->forwardmove < 0 && player->mo->tracer->target->lastlook > player->mo->tracer->target->movecount)
			player->mo->tracer->target->lastlook -= 2;

		if (!(player->mo->tracer->target->flags & MF_SLIDEME) // Noclimb on chain parameters gives this
		&& !(twodlevel || player->mo->flags2 & MF2_TWOD)) // why on earth would you want to turn them in 2D mode?
		{
			player->mo->tracer->target->health += cmd->sidemove;
			player->mo->angle += cmd->sidemove<<ANGLETOFINESHIFT; // 2048 --> ANGLE_MAX

			if (!demoplayback || P_AnalogMove(player))
			{
				if (player == &players[consoleplayer])
					localangle = player->mo->angle; // Adjust the local control angle.
				else if (player == &players[secondarydisplayplayer])
					localangle2 = player->mo->angle;
			}
		}
	}

	if (thiscam)
	{
		if (!thiscam->chase) // bob view only if looking through the player's eyes
		{
			P_CalcHeight(player);
			P_CalcPostImg(player);
		}
		else
		{
			// defaults to make sure 1st person cam doesn't do anything weird on startup
			player->deltaviewheight = 0;
			player->viewheight = FixedMul(cv_viewheight.value << FRACBITS, player->mo->scale);
			if (player->mo->eflags & MFE_VERTICALFLIP)
				player->viewz = player->mo->z + player->mo->height - player->viewheight;
			else
				player->viewz = player->mo->z + player->viewheight;
			if (server || addedtogame)
				P_MoveChaseCamera(player, thiscam, false); // calculate the camera movement
		}
	}

	// spectator invisibility and nogravity.
	if ((netgame || multiplayer) && player->spectator)
	{
		player->mo->flags2 |= MF2_DONTDRAW;
		player->mo->flags |= MF_NOGRAVITY;
	}

	if (P_IsObjectOnGround(player->mo))
		player->mo->pmomz = 0;
}
