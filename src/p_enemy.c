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
/// \file  p_enemy.c
/// \brief Enemy thinking, AI
///        Action Pointer Functions that are associated with states/frames

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
#include "m_misc.h"
#include "r_things.h"
#include "i_video.h"
#include "lua_hook.h"

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

#ifdef HAVE_BLUA
boolean LUA_CallAction(const char *action, mobj_t *actor);
#endif

player_t *stplyr;
INT32 var1;
INT32 var2;

//
// P_NewChaseDir related LUT.
//
static dirtype_t opposite[] =
{
	DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
	DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

static dirtype_t diags[] =
{
	DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};

//Real Prototypes to A_*
void A_Fall(mobj_t *actor);
void A_Look(mobj_t *actor);
void A_Chase(mobj_t *actor);
void A_FaceStabChase(mobj_t *actor);
void A_JetJawRoam(mobj_t *actor);
void A_JetJawChomp(mobj_t *actor);
void A_PointyThink(mobj_t *actor);
void A_CheckBuddy(mobj_t *actor);
void A_HoodThink(mobj_t *actor);
void A_ArrowCheck(mobj_t *actor);
void A_SnailerThink(mobj_t *actor);
void A_SharpChase(mobj_t *actor);
void A_SharpSpin(mobj_t *actor);
void A_VultureVtol(mobj_t *actor);
void A_VultureCheck(mobj_t *actor);
void A_SkimChase(mobj_t *actor);
void A_FaceTarget(mobj_t *actor);
void A_FaceTracer(mobj_t *actor);
void A_LobShot(mobj_t *actor);
void A_FireShot(mobj_t *actor);
void A_SuperFireShot(mobj_t *actor);
void A_BossFireShot(mobj_t *actor);
void A_Boss7FireMissiles(mobj_t *actor);
void A_Boss1Laser(mobj_t *actor);
void A_FocusTarget(mobj_t *actor);
void A_Boss4Reverse(mobj_t *actor);
void A_Boss4SpeedUp(mobj_t *actor);
void A_Boss4Raise(mobj_t *actor);
void A_SkullAttack(mobj_t *actor);
void A_BossZoom(mobj_t *actor);
void A_BossScream(mobj_t *actor);
void A_Scream(mobj_t *actor);
void A_Pain(mobj_t *actor);
void A_1upThinker(mobj_t *actor);
void A_MonitorPop(mobj_t *actor);
void A_GoldMonitorPop(mobj_t *actor);
void A_GoldMonitorRestore(mobj_t *actor);
void A_GoldMonitorSparkle(mobj_t *actor);
void A_Explode(mobj_t *actor);
void A_BossDeath(mobj_t *actor);
void A_CustomPower(mobj_t *actor);
void A_GiveWeapon(mobj_t *actor);
void A_RingBox(mobj_t *actor);
void A_Invincibility(mobj_t *actor);
void A_SuperSneakers(mobj_t *actor);
void A_AwardScore(mobj_t *actor);
void A_ExtraLife(mobj_t *actor);
void A_GiveShield(mobj_t *actor);
void A_GravityBox(mobj_t *actor);
void A_ScoreRise(mobj_t *actor);
void A_ParticleSpawn(mobj_t *actor);
void A_BunnyHop(mobj_t *actor);
void A_BubbleSpawn(mobj_t *actor);
void A_FanBubbleSpawn(mobj_t *actor);
void A_BubbleRise(mobj_t *actor);
void A_BubbleCheck(mobj_t *actor);
void A_AttractChase(mobj_t *actor);
void A_DropMine(mobj_t *actor);
void A_FishJump(mobj_t *actor);
void A_ThrownRing(mobj_t *actor);
void A_SetSolidSteam(mobj_t *actor);
void A_UnsetSolidSteam(mobj_t *actor);
void A_SignPlayer(mobj_t *actor);
void A_OverlayThink(mobj_t *actor);
void A_JetChase(mobj_t *actor);
void A_JetbThink(mobj_t *actor);
void A_JetgShoot(mobj_t *actor);
void A_JetgThink(mobj_t *actor);
void A_ShootBullet(mobj_t *actor);
void A_MinusDigging(mobj_t *actor);
void A_MinusPopup(mobj_t *actor);
void A_MinusCheck(mobj_t *actor);
void A_ChickenCheck(mobj_t *actor);
void A_MouseThink(mobj_t *actor);
void A_DetonChase(mobj_t *actor);
void A_CapeChase(mobj_t *actor);
void A_RotateSpikeBall(mobj_t *actor);
void A_SlingAppear(mobj_t *actor);
void A_UnidusBall(mobj_t *actor);
void A_RockSpawn(mobj_t *actor);
void A_SetFuse(mobj_t *actor);
void A_CrawlaCommanderThink(mobj_t *actor);
void A_RingExplode(mobj_t *actor);
void A_OldRingExplode(mobj_t *actor);
void A_MixUp(mobj_t *actor);
void A_RecyclePowers(mobj_t *actor);
void A_Boss2TakeDamage(mobj_t *actor);
void A_Boss7Chase(mobj_t *actor);
void A_GoopSplat(mobj_t *actor);
void A_Boss2PogoSFX(mobj_t *actor);
void A_Boss2PogoTarget(mobj_t *actor);
void A_EggmanBox(mobj_t *actor);
void A_TurretFire(mobj_t *actor);
void A_SuperTurretFire(mobj_t *actor);
void A_TurretStop(mobj_t *actor);
void A_SparkFollow(mobj_t *actor);
void A_BuzzFly(mobj_t *actor);
void A_GuardChase(mobj_t *actor);
void A_EggShield(mobj_t *actor);
void A_SetReactionTime(mobj_t *actor);
void A_Boss1Spikeballs(mobj_t *actor);
void A_Boss3TakeDamage(mobj_t *actor);
void A_Boss3Path(mobj_t *actor);
void A_LinedefExecute(mobj_t *actor);
void A_PlaySeeSound(mobj_t *actor);
void A_PlayAttackSound(mobj_t *actor);
void A_PlayActiveSound(mobj_t *actor);
void A_SmokeTrailer(mobj_t *actor);
void A_SpawnObjectAbsolute(mobj_t *actor);
void A_SpawnObjectRelative(mobj_t *actor);
void A_ChangeAngleRelative(mobj_t *actor);
void A_ChangeAngleAbsolute(mobj_t *actor);
void A_PlaySound(mobj_t *actor);
void A_FindTarget(mobj_t *actor);
void A_FindTracer(mobj_t *actor);
void A_SetTics(mobj_t *actor);
void A_SetRandomTics(mobj_t *actor);
void A_ChangeColorRelative(mobj_t *actor);
void A_ChangeColorAbsolute(mobj_t *actor);
void A_MoveRelative(mobj_t *actor);
void A_MoveAbsolute(mobj_t *actor);
void A_Thrust(mobj_t *actor);
void A_ZThrust(mobj_t *actor);
void A_SetTargetsTarget(mobj_t *actor);
void A_SetObjectFlags(mobj_t *actor);
void A_SetObjectFlags2(mobj_t *actor);
void A_RandomState(mobj_t *actor);
void A_RandomStateRange(mobj_t *actor);
void A_DualAction(mobj_t *actor);
void A_RemoteAction(mobj_t *actor);
void A_ToggleFlameJet(mobj_t *actor);
void A_OrbitNights(mobj_t *actor);
void A_GhostMe(mobj_t *actor);
void A_SetObjectState(mobj_t *actor);
void A_SetObjectTypeState(mobj_t *actor);
void A_KnockBack(mobj_t *actor);
void A_PushAway(mobj_t *actor);
void A_RingDrain(mobj_t *actor);
void A_SplitShot(mobj_t *actor);
void A_MissileSplit(mobj_t *actor);
void A_MultiShot(mobj_t *actor);
void A_InstaLoop(mobj_t *actor);
void A_Custom3DRotate(mobj_t *actor);
void A_SearchForPlayers(mobj_t *actor);
void A_CheckRandom(mobj_t *actor);
void A_CheckTargetRings(mobj_t *actor);
void A_CheckRings(mobj_t *actor);
void A_CheckTotalRings(mobj_t *actor);
void A_CheckHealth(mobj_t *actor);
void A_CheckRange(mobj_t *actor);
void A_CheckHeight(mobj_t *actor);
void A_CheckTrueRange(mobj_t *actor);
void A_CheckThingCount(mobj_t *actor);
void A_CheckAmbush(mobj_t *actor);
void A_CheckCustomValue(mobj_t *actor);
void A_CheckCusValMemo(mobj_t *actor);
void A_SetCustomValue(mobj_t *actor);
void A_UseCusValMemo(mobj_t *actor);
void A_RelayCustomValue(mobj_t *actor);
void A_CusValAction(mobj_t *actor);
void A_ForceStop(mobj_t *actor);
void A_ForceWin(mobj_t *actor);
void A_SpikeRetract(mobj_t *actor);
void A_InfoState(mobj_t *actor);
void A_Repeat(mobj_t *actor);
void A_SetScale(mobj_t *actor);
void A_RemoteDamage(mobj_t *actor);
void A_HomingChase(mobj_t *actor);
void A_TrapShot(mobj_t *actor);
//for p_enemy.c
void A_Boss1Chase(mobj_t *actor);
void A_Boss2Chase(mobj_t *actor);
void A_Boss2Pogo(mobj_t *actor);
void A_BossJetFume(mobj_t *actor);
void A_VileTarget(mobj_t *actor);
void A_VileAttack(mobj_t *actor);
void A_VileFire(mobj_t *actor);
void A_BrakChase(mobj_t *actor);
void A_BrakFireShot(mobj_t *actor);
void A_BrakLobShot(mobj_t *actor);
void A_NapalmScatter(mobj_t *actor);
void A_SpawnFreshCopy(mobj_t *actor);
void A_FlickySpawn(mobj_t *actor);
void A_FlickyAim(mobj_t *actor);
void A_FlickyFly(mobj_t *actor);
void A_FlickySoar(mobj_t *actor);
void A_FlickyCoast(mobj_t *actor);
void A_FlickyHop(mobj_t *actor);
void A_FlickyFlounder(mobj_t *actor);
void A_FlickyCheck(mobj_t *actor);
void A_FlickyHeightCheck(mobj_t *actor);
void A_FlickyFlutter(mobj_t *actor);
void A_FlameParticle(mobj_t *actor);

//
// ENEMY THINKING
// Enemies are always spawned with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players, but some can be made preaware.
//

//
// P_CheckMeleeRange
//
boolean P_CheckMeleeRange(mobj_t *actor)
{
	mobj_t *pl;
	fixed_t dist;

	if (!actor->target)
		return false;

	pl = actor->target;
	dist = P_AproxDistance(pl->x-actor->x, pl->y-actor->y);

	if (dist >= FixedMul(MELEERANGE - 20*FRACUNIT, actor->scale) + pl->radius)
		return false;

	// check height now, so that damn crawlas cant attack
	// you if you stand on a higher ledge.
	if ((pl->z > actor->z + actor->height) || (actor->z > pl->z + pl->height))
		return false;

	if (!P_CheckSight(actor, actor->target))
		return false;

	return true;
}

// P_CheckMeleeRange for Jettysyn Bomber.
boolean P_JetbCheckMeleeRange(mobj_t *actor)
{
	mobj_t *pl;
	fixed_t dist;

	if (!actor->target)
		return false;

	pl = actor->target;
	dist = P_AproxDistance(pl->x-actor->x, pl->y-actor->y);

	if (dist >= (actor->radius + pl->radius)*2)
		return false;

	if (actor->eflags & MFE_VERTICALFLIP)
	{
		if (pl->z < actor->z + actor->height + FixedMul(40<<FRACBITS, actor->scale))
			return false;
	}
	else
	{
		if (pl->z + pl->height > actor->z - FixedMul(40<<FRACBITS, actor->scale))
			return false;
	}

	return true;
}

// P_CheckMeleeRange for CastleBot FaceStabber.
boolean P_FaceStabCheckMeleeRange(mobj_t *actor)
{
	mobj_t *pl;
	fixed_t dist;

	if (!actor->target)
		return false;

	pl = actor->target;
	dist = P_AproxDistance(pl->x-actor->x, pl->y-actor->y);

	if (dist >= (actor->radius + pl->radius)*4)
		return false;

	if ((pl->z > actor->z + actor->height) || (actor->z > pl->z + pl->height))
		return false;

	if (!P_CheckSight(actor, actor->target))
		return false;

	return true;
}

// P_CheckMeleeRange for Skim.
boolean P_SkimCheckMeleeRange(mobj_t *actor)
{
	mobj_t *pl;
	fixed_t dist;

	if (!actor->target)
		return false;

	pl = actor->target;
	dist = P_AproxDistance(pl->x-actor->x, pl->y-actor->y);

	if (dist >= FixedMul(MELEERANGE - 20*FRACUNIT, actor->scale) + pl->radius)
		return false;

	if (actor->eflags & MFE_VERTICALFLIP)
	{
		if (pl->z < actor->z + actor->height + FixedMul(24<<FRACBITS, actor->scale))
			return false;
	}
	else
	{
		if (pl->z + pl->height > actor->z - FixedMul(24<<FRACBITS, actor->scale))
			return false;
	}

	return true;
}

//
// P_CheckMissileRange
//
boolean P_CheckMissileRange(mobj_t *actor)
{
	fixed_t dist;

	if (!actor->target)
		return false;

	if (actor->reactiontime)
		return false; // do not attack yet

	if (!P_CheckSight(actor, actor->target))
		return false;

	// OPTIMIZE: get this from a global checksight
	dist = P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) - FixedMul(64*FRACUNIT, actor->scale);

	if (!actor->info->meleestate)
		dist -= FixedMul(128*FRACUNIT, actor->scale); // no melee attack, so fire more

	dist >>= FRACBITS;

	if (actor->type == MT_EGGMOBILE)
		dist >>= 1;

	if (dist > 200)
		dist = 200;

	if (actor->type == MT_EGGMOBILE && dist > 160)
		dist = 160;

	if (P_RandomByte() < dist)
		return false;

	return true;
}

/** Checks for water in a sector.
  * Used by Skim movements.
  *
  * \param x X coordinate on the map.
  * \param y Y coordinate on the map.
  * \return True if there's water at this location, false if not.
  * \sa ::MT_SKIM
  */
static boolean P_WaterInSector(mobj_t *mobj, fixed_t x, fixed_t y)
{
	sector_t *sector;

	sector = R_PointInSubsector(x, y)->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE))
				continue;

			if (*rover->topheight >= mobj->floorz && *rover->topheight <= mobj->z)
				return true; // we found water!!
		}
	}

	return false;
}

static const fixed_t xspeed[NUMDIRS] = {FRACUNIT, 46341>>(16-FRACBITS), 0, -(46341>>(16-FRACBITS)), -FRACUNIT, -(46341>>(16-FRACBITS)), 0, 46341>>(16-FRACBITS)};
static const fixed_t yspeed[NUMDIRS] = {0, 46341>>(16-FRACBITS), FRACUNIT, 46341>>(16-FRACBITS), 0, -(46341>>(16-FRACBITS)), -FRACUNIT, -(46341>>(16-FRACBITS))};

/** Moves an actor in its current direction.
  *
  * \param actor Actor object to move.
  * \return False if the move is blocked, otherwise true.
  */
boolean P_Move(mobj_t *actor, fixed_t speed)
{
	fixed_t tryx, tryy;
	dirtype_t movedir = actor->movedir;

	if (movedir == DI_NODIR || !actor->health)
		return false;

	I_Assert(movedir < NUMDIRS);

	tryx = actor->x + FixedMul(speed*xspeed[movedir], actor->scale);
	if (twodlevel || actor->flags2 & MF2_TWOD)
		tryy = actor->y;
	else
		tryy = actor->y + FixedMul(speed*yspeed[movedir], actor->scale);

	if (actor->type == MT_SKIM && !P_WaterInSector(actor, tryx, tryy)) // bail out if sector lacks water
		return false;

	if (!P_TryMove(actor, tryx, tryy, false))
	{
		if (actor->flags & MF_FLOAT && floatok)
		{
			// must adjust height
			if (actor->z < tmfloorz)
				actor->z += FixedMul(FLOATSPEED, actor->scale);
			else
				actor->z -= FixedMul(FLOATSPEED, actor->scale);

			if (actor->type == MT_JETJAW && actor->z + actor->height > actor->watertop)
				actor->z = actor->watertop - actor->height;

			actor->flags2 |= MF2_INFLOAT;
			return true;
		}

		return false;
	}
	else
		actor->flags2 &= ~MF2_INFLOAT;

	return true;
}

/** Attempts to move an actor on in its current direction.
  * If the move succeeds, the actor's move count is reset
  * randomly to a value from 0 to 15.
  *
  * \param actor Actor to move.
  * \return True if the move succeeds, false if the move is blocked.
  */
static boolean P_TryWalk(mobj_t *actor)
{
	if (!P_Move(actor, actor->info->speed))
		return false;
	actor->movecount = P_RandomByte() & 15;
	return true;
}

void P_NewChaseDir(mobj_t *actor)
{
	fixed_t deltax, deltay;
	dirtype_t d[3];
	dirtype_t tdir = DI_NODIR, olddir, turnaround;

	I_Assert(actor->target != NULL);
	I_Assert(!P_MobjWasRemoved(actor->target));

	olddir = actor->movedir;

	if (olddir >= NUMDIRS)
		olddir = DI_NODIR;

	if (olddir != DI_NODIR)
		turnaround = opposite[olddir];
	else
		turnaround = olddir;

	deltax = actor->target->x - actor->x;
	deltay = actor->target->y - actor->y;

	if (deltax > FixedMul(10*FRACUNIT, actor->scale))
		d[1] = DI_EAST;
	else if (deltax < -FixedMul(10*FRACUNIT, actor->scale))
		d[1] = DI_WEST;
	else
		d[1] = DI_NODIR;

	if (twodlevel || actor->flags2 & MF2_TWOD)
		d[2] = DI_NODIR;
	if (deltay < -FixedMul(10*FRACUNIT, actor->scale))
		d[2] = DI_SOUTH;
	else if (deltay > FixedMul(10*FRACUNIT, actor->scale))
		d[2] = DI_NORTH;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		dirtype_t newdir = diags[((deltay < 0)<<1) + (deltax > 0)];

		actor->movedir = newdir;
		if ((newdir != turnaround) && P_TryWalk(actor))
			return;
	}

	// try other directions
	if (P_RandomChance(25*FRACUNIT/32) || abs(deltay) > abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] == turnaround)
		d[1] = DI_NODIR;
	if (d[2] == turnaround)
		d[2] = DI_NODIR;

	if (d[1] != DI_NODIR)
	{
		actor->movedir = d[1];

		if (P_TryWalk(actor))
			return; // either moved forward or attacked
	}

	if (d[2] != DI_NODIR)
	{
		actor->movedir = d[2];

		if (P_TryWalk(actor))
			return;
	}

	// there is no direct path to the player, so pick another direction.
	if (olddir != DI_NODIR)
	{
		actor->movedir =olddir;

		if (P_TryWalk(actor))
			return;
	}

	// randomly determine direction of search
	if (P_RandomChance(FRACUNIT/2))
	{
		for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
		{
			if (tdir != turnaround)
			{
				actor->movedir = tdir;

				if (P_TryWalk(actor))
					return;
			}
		}
	}
	else
	{
		for (tdir = DI_SOUTHEAST; tdir >= DI_EAST; tdir--)
		{
			if (tdir != turnaround)
			{
				actor->movedir = tdir;

				if (P_TryWalk(actor))
					return;
			}
		}
	}

	if (turnaround != DI_NODIR)
	{
		actor->movedir = turnaround;

		if (P_TryWalk(actor))
			return;
	}

	actor->movedir = (angle_t)DI_NODIR; // cannot move
}

/** Looks for players to chase after, aim at, or whatever.
  *
  * \param actor     The object looking for flesh.
  * \param allaround Look all around? If false, only players in a 180-degree
  *                  range in front will be spotted.
  * \param dist      If > 0, checks distance
  * \return True if a player is found, otherwise false.
  * \sa P_SupermanLook4Players
  */
boolean P_LookForPlayers(mobj_t *actor, boolean allaround, boolean tracer, fixed_t dist)
{
	INT32 c = 0, stop;
	player_t *player;
	angle_t an;

	// BP: first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_RandomByte();

	actor->lastlook %= MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for (; ; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
	{
		// done looking
		if (actor->lastlook == stop)
			return false;

		if (!playeringame[actor->lastlook])
			continue;

		if (c++ == 2)
			return false;

		player = &players[actor->lastlook];

		if ((netgame || multiplayer) && player->spectator)
			continue;

		if (player->pflags & PF_INVIS)
			continue; // ignore notarget

		if (!player->mo || P_MobjWasRemoved(player->mo))
			continue;

		if (player->mo->health <= 0)
			continue; // dead

		if (dist > 0
			&& P_AproxDistance(P_AproxDistance(player->mo->x - actor->x, player->mo->y - actor->y), player->mo->z - actor->z) > dist)
			continue; // Too far away

		if (!allaround)
		{
			an = R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y) - actor->angle;
			if (an > ANGLE_90 && an < ANGLE_270)
			{
				dist = P_AproxDistance(player->mo->x - actor->x, player->mo->y - actor->y);
				// if real close, react anyway
				if (dist > FixedMul(MELEERANGE, actor->scale))
					continue; // behind back
			}
		}

		if (!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if (tracer)
			P_SetTarget(&actor->tracer, player->mo);
		else
			P_SetTarget(&actor->target, player->mo);
		return true;
	}

	//return false;
}

/** Looks for a player with a ring shield.
  * Used by rings.
  *
  * \param actor Ring looking for a shield to be attracted to.
  * \return True if a player with ring shield is found, otherwise false.
  * \sa A_AttractChase
  */
static boolean P_LookForShield(mobj_t *actor)
{
	INT32 c = 0, stop;
	player_t *player;

	// BP: first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_RandomByte();

	actor->lastlook %= MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for (; ; actor->lastlook = ((actor->lastlook + 1) & PLAYERSMASK))
	{
		// done looking
		if (actor->lastlook == stop)
			return false;

		if (!playeringame[actor->lastlook])
			continue;

		if (c++ == 2)
			return false;

		player = &players[actor->lastlook];

		if (!player->mo || player->mo->health <= 0)
			continue; // dead

		//When in CTF, don't pull rings that you cannot pick up.
		if ((actor->type == MT_REDTEAMRING && player->ctfteam != 1) ||
			(actor->type == MT_BLUETEAMRING && player->ctfteam != 2))
			continue;

		if ((player->powers[pw_shield] & SH_PROTECTELECTRIC)
			&& (P_AproxDistance(P_AproxDistance(actor->x-player->mo->x, actor->y-player->mo->y), actor->z-player->mo->z) < FixedMul(RING_DIST, player->mo->scale)))
		{
			P_SetTarget(&actor->tracer, player->mo);
			return true;
		}
	}

	//return false;
}

#ifdef WEIGHTEDRECYCLER
// Compares players to see who currently has the "best" items, etc.
static int P_RecycleCompare(const void *p1, const void *p2)
{
	player_t *player1 = &players[*(const UINT8 *)p1];
	player_t *player2 = &players[*(const UINT8 *)p2];

	// Non-shooting gametypes
	if (!G_PlatformGametype())
	{
		// Invincibility.
		if (player1->powers[pw_invulnerability] > player2->powers[pw_invulnerability]) return -1;
		else if (player2->powers[pw_invulnerability] > player1->powers[pw_invulnerability]) return 1;

		// One has a shield, the other doesn't.
		if (player1->powers[pw_shield] && !player2->powers[pw_shield]) return -1;
		else if (player2->powers[pw_shield] && !player1->powers[pw_shield]) return 1;

		// Sneakers.
		if (player1->powers[pw_sneakers] > player2->powers[pw_sneakers]) return -1;
		else if (player2->powers[pw_sneakers] > player1->powers[pw_sneakers]) return 1;
	}
	else // Match, Team Match, CTF, Tag, Etc.
	{
		UINT8 player1_em = M_CountBits((UINT32)player1->powers[pw_emeralds], 7);
		UINT8 player2_em = M_CountBits((UINT32)player2->powers[pw_emeralds], 7);

		UINT8 player1_rw = M_CountBits((UINT32)player1->ringweapons, NUM_WEAPONS-1);
		UINT8 player2_rw = M_CountBits((UINT32)player2->ringweapons, NUM_WEAPONS-1);

		UINT16 player1_am = player1->powers[pw_infinityring]          // max 800
		                  + player1->powers[pw_automaticring]         // max 300
		                  + (player1->powers[pw_bouncering]    * 3)   // max 100
		                  + (player1->powers[pw_explosionring] * 6)   // max 50
		                  + (player1->powers[pw_scatterring]   * 3)   // max 100
		                  + (player1->powers[pw_grenadering]   * 6)   // max 50
		                  + (player1->powers[pw_railring]      * 6);  // max 50
		UINT16 player2_am = player2->powers[pw_infinityring]          // max 800
		                  + player2->powers[pw_automaticring]         // max 300
		                  + (player2->powers[pw_bouncering]    * 3)   // max 100
		                  + (player2->powers[pw_explosionring] * 6)   // max 50
		                  + (player2->powers[pw_scatterring]   * 3)   // max 100
		                  + (player2->powers[pw_grenadering]   * 6)   // max 50
		                  + (player2->powers[pw_railring]      * 6);  // max 50

		// Super trumps everything.
		if (player1->powers[pw_super] && !player2->powers[pw_super]) return -1;
		else if (player2->powers[pw_super] && !player1->powers[pw_super]) return 1;

		// Emerald count if neither player is Super.
		if (player1_em > player2_em) return -1;
		else if (player1_em < player2_em) return 1;

		// One has a shield, the other doesn't.
		// (the likelihood of a shielded player being worse off than one without one is low.)
		if (player1->powers[pw_shield] && !player2->powers[pw_shield]) return -1;
		else if (player2->powers[pw_shield] && !player1->powers[pw_shield]) return 1;

		// Ring weapons count
		if (player1_rw > player2_rw) return -1;
		else if (player1_rw < player2_rw) return 1;

		// Ring ammo if they have the same number of weapons
		if (player1_am > player2_am) return -1;
		else if (player1_am < player2_am) return 1;
	}

	// Identical for our purposes
	return 0;
}
#endif

// Handles random monitor weights via console.
static mobjtype_t P_DoRandomBoxChances(void)
{
	mobjtype_t spawnchance[256];
	INT32 numchoices = 0, i = 0;

	if (!(netgame || multiplayer))
	{
		switch (P_RandomKey(10))
		{
			case 0:
				return MT_RING_ICON;
			case 1:
				return MT_SNEAKERS_ICON;
			case 2:
				return MT_INVULN_ICON;
			case 3:
				return MT_WHIRLWIND_ICON;
			case 4:
				return MT_ELEMENTAL_ICON;
			case 5:
				return MT_ATTRACT_ICON;
			case 6:
				return MT_FORCE_ICON;
			case 7:
				return MT_ARMAGEDDON_ICON;
			case 8:
				return MT_1UP_ICON;
			case 9:
				return MT_EGGMAN_ICON;
		}
		return MT_NULL;
	}

#define QUESTIONBOXCHANCES(type, cvar) \
for (i = cvar.value; i; --i) spawnchance[numchoices++] = type
	QUESTIONBOXCHANCES(MT_RING_ICON,       cv_superring);
	QUESTIONBOXCHANCES(MT_SNEAKERS_ICON,   cv_supersneakers);
	QUESTIONBOXCHANCES(MT_INVULN_ICON,     cv_invincibility);
	QUESTIONBOXCHANCES(MT_WHIRLWIND_ICON,  cv_jumpshield);
	QUESTIONBOXCHANCES(MT_ELEMENTAL_ICON,  cv_watershield);
	QUESTIONBOXCHANCES(MT_ATTRACT_ICON,    cv_ringshield);
	QUESTIONBOXCHANCES(MT_FORCE_ICON,      cv_forceshield);
	QUESTIONBOXCHANCES(MT_ARMAGEDDON_ICON, cv_bombshield);
	QUESTIONBOXCHANCES(MT_1UP_ICON,        cv_1up);
	QUESTIONBOXCHANCES(MT_EGGMAN_ICON,     cv_eggmanbox);
	QUESTIONBOXCHANCES(MT_MIXUP_ICON,      cv_teleporters);
	QUESTIONBOXCHANCES(MT_RECYCLER_ICON,   cv_recycler);
#undef QUESTIONBOXCHANCES

	if (numchoices == 0) return MT_NULL;
	return spawnchance[P_RandomKey(numchoices)];
}

//
// ACTION ROUTINES
//

// Function: A_Look
//
// Description: Look for a player and set your target to them.
//
// var1:
//		lower 16 bits = look all around
//		upper 16 bits = distance limit
// var2 = If 1, only change to seestate. If 2, only play seesound. If 0, do both.
//
void A_Look(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Look", actor))
		return;
#endif

	if (!P_LookForPlayers(actor, locvar1 & 65535, false , FixedMul((locvar1 >> 16)*FRACUNIT, actor->scale)))
		return;

	// go into chase state
	if (!locvar2)
	{
		P_SetMobjState(actor, actor->info->seestate);
		A_PlaySeeSound(actor);
	}
	else if (locvar2 == 1) // Only go into seestate
		P_SetMobjState(actor, actor->info->seestate);
	else if (locvar2 == 2) // Only play seesound
		A_PlaySeeSound(actor);
}

// Function: A_Chase
//
// Description: Chase after your target.
//
// var1:
//		1 = don't check meleestate
//		2 = don't check missilestate
//		3 = don't check meleestate and missilestate
// var2 = unused
//
void A_Chase(mobj_t *actor)
{
	INT32 delta;
	INT32 locvar1 = var1;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Chase", actor))
		return;
#endif

	I_Assert(actor != NULL);
	I_Assert(!P_MobjWasRemoved(actor));

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjStateNF(actor, actor->info->spawnstate);
		return;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if (!(locvar1 & 1) && actor->info->meleestate && P_CheckMeleeRange(actor))
	{
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if (!(locvar1 & 2) && actor->info->missilestate)
	{
		if (actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if (multiplayer && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false, 0))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
		P_NewChaseDir(actor);
}

// Function: A_FaceStabChase
//
// Description: A_Chase for CastleBot FaceStabber.
//
// var1 = unused
// var2 = unused
//
void A_FaceStabChase(mobj_t *actor)
{
	INT32 delta;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FaceStabChase", actor))
		return;
#endif

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjStateNF(actor, actor->info->spawnstate);
		return;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if (actor->info->meleestate && P_FaceStabCheckMeleeRange(actor))
	{
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if (actor->info->missilestate)
	{
		if (actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if (multiplayer && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false, 0))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
		P_NewChaseDir(actor);
}

// Function: A_JetJawRoam
//
// Description: Roaming routine for JetJaw
//
// var1 = unused
// var2 = unused
//
void A_JetJawRoam(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_JetJawRoam", actor))
		return;
#endif
	if (actor->reactiontime)
	{
		actor->reactiontime--;
		P_InstaThrust(actor, actor->angle, FixedMul(actor->info->speed*FRACUNIT/4, actor->scale));
	}
	else
	{
		actor->reactiontime = actor->info->reactiontime;
		actor->angle += ANGLE_180;
	}

	if (P_LookForPlayers(actor, false, false, actor->radius * 16))
		P_SetMobjState(actor, actor->info->seestate);
}

// Function: A_JetJawChomp
//
// Description: Chase and chomp at the target, as long as it is in view
//
// var1 = unused
// var2 = unused
//
void A_JetJawChomp(mobj_t *actor)
{
	INT32 delta;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_JetJawChomp", actor))
		return;
#endif

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	// Stop chomping if target's dead or you can't see it
	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE)
		|| actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
	{
		P_SetMobjStateNF(actor, actor->info->spawnstate);
		return;
	}

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
		P_NewChaseDir(actor);
}

// Function: A_PointyThink
//
// Description: Thinker function for Pointy
//
// var1 = unused
// var2 = unused
//
void A_PointyThink(mobj_t *actor)
{
	INT32 i;
	player_t *player = NULL;
	mobj_t *ball;
	TVector v;
	TVector *res;
	angle_t fa;
	fixed_t radius = FixedMul(actor->info->radius*actor->info->reactiontime, actor->scale);
	boolean firsttime = true;
	INT32 sign;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_PointyThink", actor))
		return;
#endif
	actor->momx = actor->momy = actor->momz = 0;

	// Find nearest player
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (!players[i].mo)
			continue;

		if (!players[i].mo->health)
			continue;

		if (!P_CheckSight(actor, players[i].mo))
			continue;

		if (firsttime)
		{
			firsttime = false;
			player = &players[i];
		}
		else
		{
			if (P_AproxDistance(players[i].mo->x - actor->x, players[i].mo->y - actor->y) <
				P_AproxDistance(player->mo->x - actor->x, player->mo->y - actor->y))
				player = &players[i];
		}
	}

	if (!player)
		return;

	// Okay, we found the closest player. Let's move based on his movement.
	P_SetTarget(&actor->target, player->mo);
	A_FaceTarget(actor);

	if (P_AproxDistance(player->mo->x - actor->x, player->mo->y - actor->y) < P_AproxDistance(player->mo->x + player->mo->momx - actor->x, player->mo->y + player->mo->momy - actor->y))
		sign = -1; // Player is moving away
	else
		sign = 1; // Player is moving closer

	if (player->mo->momx || player->mo->momy)
	{
		P_InstaThrust(actor, R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y), FixedMul(actor->info->speed*sign, actor->scale));

		// Rotate our spike balls
		actor->lastlook += actor->info->damage;
		actor->lastlook %= FINEANGLES/4;
	}

	if (!actor->tracer) // For some reason we do not have spike balls...
		return;

	// Position spike balls relative to the value of 'lastlook'.
	ball = actor->tracer;

	i = 0;
	while (ball)
	{
		fa = actor->lastlook+i;
		v[0] = FixedMul(FINECOSINE(fa),radius);
		v[1] = 0;
		v[2] = FixedMul(FINESINE(fa),radius);
		v[3] = FRACUNIT;

		res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(actor->lastlook+i)));
		M_Memcpy(&v, res, sizeof (v));
		res = VectorMatrixMultiply(v, *RotateZMatrix(actor->angle+ANGLE_180));
		M_Memcpy(&v, res, sizeof (v));

		P_UnsetThingPosition(ball);
		ball->x = actor->x + v[0];
		ball->y = actor->y + v[1];
		ball->z = actor->z + (actor->height>>1) + v[2];
		P_SetThingPosition(ball);

		ball = ball->tracer;
		i += ANGLE_90 >> ANGLETOFINESHIFT;
	}
}

// Function: A_CheckBuddy
//
// Description: Checks if target/tracer exists/has health. If not, the object removes itself.
//
// var1:
//		0 = target
//		1 = tracer
// var2 = unused
//
void A_CheckBuddy(mobj_t *actor)
{
	INT32 locvar1 = var1;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckBuddy", actor))
		return;
#endif
	if (locvar1 && (!actor->tracer || actor->tracer->health <= 0))
		P_RemoveMobj(actor);
	else if (!locvar1 && (!actor->target || actor->target->health <= 0))
		P_RemoveMobj(actor);
}

// Function: A_HoodThink
//
// Description: Thinker for Robo-Hood
//
// var1 = unused
// var2 = unused
//
void A_HoodThink(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_HoodThink", actor))
		return;
#endif
	// Currently in the air...
	if (!(actor->eflags & MFE_VERTICALFLIP) && actor->z > actor->floorz)
	{
		if (actor->momz > 0)
			P_SetMobjStateNF(actor, actor->info->xdeathstate); // Rising
		else
			P_SetMobjStateNF(actor, actor->info->raisestate); // Falling

		return;
	}
	else if ((actor->eflags & MFE_VERTICALFLIP) && actor->z + actor->height < actor->ceilingz)
	{
		if (actor->momz < 0)
			P_SetMobjStateNF(actor, actor->info->xdeathstate); // Rising
		else
			P_SetMobjStateNF(actor, actor->info->raisestate); // Falling

		return;
	}

	if (actor->state == &states[actor->info->xdeathstate]
		|| actor->state == &states[actor->info->raisestate])
		P_SetMobjStateNF(actor, actor->info->seestate);

	if (!actor->target)
	{
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	A_FaceTarget(actor); // Aiming... aiming...

	if (--actor->reactiontime > 0)
		return;

	// Shoot, if not too close (cheap shots are lame)
	if ((P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) > FixedMul(192*FRACUNIT, actor->scale))
	|| (actor->spawnpoint && (actor->spawnpoint->options & MTF_AMBUSH))) // If you can't jump, might as well shoot regardless of distance!
		P_SetMobjState(actor, actor->info->missilestate);
	else if (!(actor->spawnpoint && (actor->spawnpoint->options & MTF_AMBUSH)))// But we WILL jump!
		P_SetMobjState(actor, actor->info->painstate);

	actor->reactiontime = actor->info->reactiontime;
}

// Function: A_ArrowCheck
//
// Description: Checks arrow direction and adjusts sprite accordingly
//
// var1 = unused
// var2 = unused
//
void A_ArrowCheck(mobj_t *actor)
{
	fixed_t x,y,z;
	angle_t angle;
	fixed_t dist;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ArrowCheck", actor))
		return;
#endif

	// Movement vector
	x = actor->momx;
	y = actor->momy;
	z = actor->momz;

	// Calculate the angle of movement.
	/*
	       Z
	     / |
	   /   |
	 /     |
	0------dist(X,Y)
	*/

	dist = P_AproxDistance(x, y);

	angle = R_PointToAngle2(0, 0, dist, z);

	if (angle > ANG20 && angle <= ANGLE_180)
		P_SetMobjStateNF(actor, actor->info->raisestate);
	else if (angle < ANG340 && angle > ANGLE_180)
		P_SetMobjStateNF(actor, actor->info->xdeathstate);
	else
		P_SetMobjStateNF(actor, actor->info->spawnstate);
}

// Function: A_SnailerThink
//
// Description: Thinker function for Snailer
//
// var1 = unused
// var2 = unused
//
void A_SnailerThink(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SnailerThink", actor))
		return;
#endif

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (!P_LookForPlayers(actor, true, false, 0))
			return;
	}

	// We now have a target. Oh bliss, rapture, and contentment!

	if (actor->target->z + actor->target->height > actor->z - FixedMul(32*FRACUNIT, actor->scale)
		&& actor->target->z < actor->z + actor->height + FixedMul(32*FRACUNIT, actor->scale)
		&& !(leveltime % (TICRATE*2)))
	{
		angle_t an;
		fixed_t z;

		// Actor shouldn't face target, so we'll do things a bit differently here

		an = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) - actor->angle;

		z = actor->z + actor->height/2;

		if (an > ANGLE_45 && an < ANGLE_315) // fire as close as you can to the target, even if too sharp an angle from your front
		{
			fixed_t dist;
			fixed_t dx, dy;

			dist = P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y);

			if (an > ANGLE_45 && an <= ANGLE_90) // fire at 45 degrees to the left
			{
				dx = actor->x + P_ReturnThrustX(actor, actor->angle + ANGLE_45, dist);
				dy = actor->y + P_ReturnThrustY(actor, actor->angle + ANGLE_45, dist);
			}
			else if (an >= ANGLE_270 && an < ANGLE_315) // fire at 45 degrees to the right
			{
				dx = actor->x + P_ReturnThrustX(actor, actor->angle - ANGLE_45, dist);
				dy = actor->y + P_ReturnThrustY(actor, actor->angle - ANGLE_45, dist);
			}
			else // fire straight ahead
			{
				dx = actor->x + P_ReturnThrustX(actor, actor->angle, dist);
				dy = actor->y + P_ReturnThrustY(actor, actor->angle, dist);
			}

			P_SpawnPointMissile(actor, dx, dy, actor->target->z, MT_ROCKET, actor->x, actor->y, z);
		}
		else
			P_SpawnXYZMissile(actor, actor->target, MT_ROCKET, actor->x, actor->y, z);
	}

	if ((!(actor->eflags & MFE_VERTICALFLIP) && actor->target->z > actor->z)
	|| (actor->eflags & MFE_VERTICALFLIP && (actor->target->z + actor->target->height) > (actor->z + actor->height)))
		actor->momz += FixedMul(actor->info->speed, actor->scale);
	else if ((!(actor->eflags & MFE_VERTICALFLIP) && actor->target->z < actor->z)
	|| (actor->eflags & MFE_VERTICALFLIP && (actor->target->z + actor->target->height) < (actor->z + actor->height)))
		actor->momz -= FixedMul(actor->info->speed, actor->scale);

	actor->momz /= 2;
}

// Function: A_SharpChase
//
// Description: Thinker/Chase routine for Sharps
//
// var1 = unused
// var2 = unused
//
void A_SharpChase(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SharpChase", actor))
		return;
#endif

	if (!actor->health)
	{
		P_SetMobjState(actor, actor->info->deathstate);
		return;
	}

	if (actor->reactiontime)
	{
		INT32 delta;

		actor->reactiontime--;

		// turn towards movement direction if not there yet
		if (actor->movedir < NUMDIRS)
		{
			actor->angle &= (7<<29);
			delta = actor->angle - (actor->movedir << 29);

			if (delta > 0)
				actor->angle -= ANGLE_45;
			else if (delta < 0)
				actor->angle += ANGLE_45;
		}

		if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
		{
			// look for a new target
			if (P_LookForPlayers(actor, true, false, 0))
				return; // got a new target

			P_SetMobjState(actor, actor->info->spawnstate);
			return;
		}

		// chase towards player
		if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
			P_NewChaseDir(actor);
	}
	else
	{
		actor->threshold = actor->info->painchance;
		P_SetMobjState(actor, actor->info->missilestate);
		S_StartSound(actor, actor->info->attacksound);
	}
}

// Function: A_SharpSpin
//
// Description: Spin chase routine for Sharps
//
// var1 = unused
// var2 = unused
//
void A_SharpSpin(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SharpSpin", actor))
		return;
#endif

	if (!actor->health)
	{
		P_SetMobjState(actor, actor->info->deathstate);
		return;
	}

	if (actor->threshold && actor->target)
	{
		actor->angle += ANGLE_22h;
		P_Thrust(actor, R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y), FixedMul(actor->info->speed*FRACUNIT, actor->scale));
		actor->threshold--;
	}
	else
	{
		actor->reactiontime = actor->info->reactiontime;
		P_SetMobjState(actor, actor->info->spawnstate);

		var1 = 1;
		A_Look(actor);
		if (actor->target)
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
	}
}

// Function: A_VultureVtol
//
// Description: Vulture rising up to match target's height
//
// var1 = unused
// var2 = unused
//
void A_VultureVtol(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_VultureVtol", actor))
		return;
#endif

	if (!actor->target)
		return;

	actor->flags |= MF_NOGRAVITY;
	actor->flags |= MF_FLOAT;

	A_FaceTarget(actor);

	S_StopSound(actor);

	if (actor->z < actor->target->z+(actor->target->height/4) && actor->z + actor->height < actor->ceilingz)
		actor->momz = FixedMul(2*FRACUNIT, actor->scale);
	else if (actor->z > (actor->target->z+(actor->target->height/4)*3) && actor->z > actor->floorz)
		actor->momz = FixedMul(-2*FRACUNIT, actor->scale);
	else
	{
		// Attack!
		actor->momz = 0;
		P_SetMobjState(actor, actor->info->missilestate);
		S_StartSound(actor, actor->info->activesound);
	}
}

// Function: A_VultureCheck
//
// Description: If the vulture is stopped, look for a new target
//
// var1 = unused
// var2 = unused
//
void A_VultureCheck(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_VultureCheck", actor))
		return;
#endif

	if (actor->momx || actor->momy)
		return;

	actor->flags &= ~MF_NOGRAVITY; // Fall down

	if (actor->z <= actor->floorz)
	{
		actor->angle -= ANGLE_180; // turn around
		P_SetMobjState(actor, actor->info->spawnstate);
	}
}

// Function: A_SkimChase
//
// Description: Thinker/Chase routine for Skims
//
// var1 = unused
// var2 = unused
//
void A_SkimChase(mobj_t *actor)
{
	INT32 delta;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SkimChase", actor))
		return;
#endif
	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		P_LookForPlayers(actor, true, false, 0);

		// the spawnstate for skims already calls this function so just return either way
		// without changing state
		return;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if (actor->info->meleestate && P_SkimCheckMeleeRange(actor))
	{
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if (actor->info->missilestate)
	{
		if (actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if (multiplayer && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false, 0))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
		P_NewChaseDir(actor);
}

// Function: A_FaceTarget
//
// Description: Immediately turn to face towards your target.
//
// var1 = unused
// var2 = unused
//
void A_FaceTarget(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FaceTarget", actor))
		return;
#endif
	if (!actor->target)
		return;

	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
}

// Function: A_FaceTracer
//
// Description: Immediately turn to face towards your tracer.
//
// var1 = unused
// var2 = unused
//
void A_FaceTracer(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FaceTracer", actor))
		return;
#endif
	if (!actor->tracer)
		return;

	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->tracer->x, actor->tracer->y);
}

// Function: A_LobShot
//
// Description: Lob an object at your target.
//
// var1 = object # to lob
// var2:
//		var2 >> 16 = height offset
//		var2 & 65535 = airtime
//
void A_LobShot(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2 >> 16;
	mobj_t *shot, *hitspot;
	angle_t an;
	fixed_t z;
	fixed_t dist;
	fixed_t vertical, horizontal;
	fixed_t airtime = var2 & 65535;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_LobShot", actor))
		return;
#endif
	if (!actor->target)
		return;

	A_FaceTarget(actor);

	if (actor->eflags & MFE_VERTICALFLIP)
	{
		z = actor->z + actor->height - FixedMul(locvar2*FRACUNIT, actor->scale);
		if (actor->type == MT_BLACKEGGMAN)
			z -= FixedMul(mobjinfo[locvar1].height, actor->scale/2);
		else
			z -= FixedMul(mobjinfo[locvar1].height, actor->scale);
	}
	else
		z = actor->z + FixedMul(locvar2*FRACUNIT, actor->scale);

	shot = P_SpawnMobj(actor->x, actor->y, z, locvar1);

	if (actor->type == MT_BLACKEGGMAN)
	{
		shot->destscale = actor->scale/2;
		P_SetScale(shot, actor->scale/2);
	}
	else
	{
		shot->destscale = actor->scale;
		P_SetScale(shot, actor->scale);
	}

	// Keep track of where it's going to land
	hitspot = P_SpawnMobj(actor->target->x&(64*FRACUNIT-1), actor->target->y&(64*FRACUNIT-1), actor->target->subsector->sector->floorheight, MT_NULL);
	hitspot->tics = airtime;
	P_SetTarget(&shot->tracer, hitspot);

	P_SetTarget(&shot->target, actor); // where it came from

	shot->angle = an = actor->angle;
	an >>= ANGLETOFINESHIFT;

	dist = P_AproxDistance(actor->target->x - shot->x, actor->target->y - shot->y);

	horizontal = dist / airtime;
	vertical = FixedMul((gravity*airtime)/2, shot->scale);

	shot->momx = FixedMul(horizontal, FINECOSINE(an));
	shot->momy = FixedMul(horizontal, FINESINE(an));
	shot->momz = vertical;

/* Try to adjust when destination is not the same height
	if (actor->z != actor->target->z)
	{
		fixed_t launchhyp;
		fixed_t diff;
		fixed_t orig;

		diff = actor->z - actor->target->z;
		{
			launchhyp = P_AproxDistance(horizontal, vertical);

			orig = FixedMul(FixedDiv(vertical, horizontal), diff);

			CONS_Debug(DBG_GAMELOGIC, "orig: %d\n", (orig)>>FRACBITS);

			horizontal = dist / airtime;
			vertical = (gravity*airtime)/2;
		}
		dist -= orig;
		shot->momx = FixedMul(horizontal, FINECOSINE(an));
		shot->momy = FixedMul(horizontal, FINESINE(an));
		shot->momz = vertical;
*/

	if (shot->info->seesound)
		S_StartSound(shot, shot->info->seesound);

	if (!(actor->flags & MF_BOSS))
	{
		if (ultimatemode)
			actor->reactiontime = actor->info->reactiontime*TICRATE;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	}
}

// Function: A_FireShot
//
// Description: Shoot an object at your target.
//
// var1 = object # to shoot
// var2 = height offset
//
void A_FireShot(mobj_t *actor)
{
	fixed_t z;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FireShot", actor))
		return;
#endif
	if (!actor->target)
		return;

	A_FaceTarget(actor);

	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - FixedMul(48*FRACUNIT + locvar2*FRACUNIT, actor->scale);
	else
		z = actor->z + FixedMul(48*FRACUNIT + locvar2*FRACUNIT, actor->scale);

	P_SpawnXYZMissile(actor, actor->target, locvar1, actor->x, actor->y, z);

	if (!(actor->flags & MF_BOSS))
	{
		if (ultimatemode)
			actor->reactiontime = actor->info->reactiontime*TICRATE;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	}
}

// Function: A_SuperFireShot
//
// Description: Shoot an object at your target that will even stall Super Sonic.
//
// var1 = object # to shoot
// var2 = height offset
//
void A_SuperFireShot(mobj_t *actor)
{
	fixed_t z;
	mobj_t *mo;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SuperFireShot", actor))
		return;
#endif
	if (!actor->target)
		return;

	A_FaceTarget(actor);

	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - FixedMul(48*FRACUNIT + locvar2*FRACUNIT, actor->scale);
	else
		z = actor->z + FixedMul(48*FRACUNIT + locvar2*FRACUNIT, actor->scale);

	mo = P_SpawnXYZMissile(actor, actor->target, locvar1, actor->x, actor->y, z);

	if (mo)
		mo->flags2 |= MF2_SUPERFIRE;

	if (!(actor->flags & MF_BOSS))
	{
		if (ultimatemode)
			actor->reactiontime = actor->info->reactiontime*TICRATE;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	}
}

// Function: A_BossFireShot
//
// Description: Shoot an object at your target ala Bosses:
//
// var1 = object # to shoot
// var2:
//		0 - Boss 1 Left side
//		1 - Boss 1 Right side
//		2 - Boss 3 Left side upper
//		3 - Boss 3 Left side lower
//		4 - Boss 3 Right side upper
//		5 - Boss 3 Right side lower
//
void A_BossFireShot(mobj_t *actor)
{
	fixed_t x, y, z;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BossFireShot", actor))
		return;
#endif
	if (!actor->target)
		return;

	A_FaceTarget(actor);

	switch (locvar2)
	{
		case 0:
			x = actor->x + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(48*FRACUNIT, actor->scale);
			else
				z = actor->z + FixedMul(48*FRACUNIT, actor->scale);
			break;
		case 1:
			x = actor->x + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(48*FRACUNIT, actor->scale);
			else
				z = actor->z + FixedMul(48*FRACUNIT, actor->scale);
			break;
		case 2:
			x = actor->x + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedMul(56*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedMul(56*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(42*FRACUNIT, actor->scale);
			else
				z = actor->z + FixedMul(42*FRACUNIT, actor->scale);
			break;
		case 3:
			x = actor->x + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedMul(58*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedMul(58*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(30*FRACUNIT, actor->scale);
			else
				z = actor->z + FixedMul(30*FRACUNIT, actor->scale);
			break;
		case 4:
			x = actor->x + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedMul(56*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedMul(56*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(42*FRACUNIT, actor->scale);
			else
				z = actor->z + FixedMul(42*FRACUNIT, actor->scale);
			break;
		case 5:
			x = actor->x + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedMul(58*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedMul(58*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(30*FRACUNIT, actor->scale);
			else
				z = actor->z + FixedMul(30*FRACUNIT, actor->scale);
			break;
		default:
			x = actor->x;
			y = actor->y;
			z = actor->z + actor->height/2;
			break;
	}

	P_SpawnXYZMissile(actor, actor->target, locvar1, x, y, z);
}

// Function: A_Boss7FireMissiles
//
// Description: Shoot 4 missiles of a specific object type at your target ala Black Eggman
//
// var1 = object # to shoot
// var2 = firing sound
//
void A_Boss7FireMissiles(mobj_t *actor)
{
	mobj_t dummymo;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss7FireMissiles", actor))
		return;
#endif

	if (!actor->target)
	{
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	A_FaceTarget(actor);

	S_StartSound(NULL, locvar2);

	// set dummymo's coordinates
	dummymo.x = actor->target->x;
	dummymo.y = actor->target->y;
	dummymo.z = actor->target->z + FixedMul(16*FRACUNIT, actor->scale); // raised height

	P_SpawnXYZMissile(actor, &dummymo, locvar1,
		actor->x + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->y + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->z + FixedDiv(actor->height, 3*FRACUNIT/2));

	P_SpawnXYZMissile(actor, &dummymo, locvar1,
		actor->x + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->y + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->z + FixedDiv(actor->height, 3*FRACUNIT/2));

	P_SpawnXYZMissile(actor, &dummymo, locvar1,
		actor->x + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->y + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->z + actor->height/2);

	P_SpawnXYZMissile(actor, &dummymo, locvar1,
		actor->x + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->y + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedDiv(actor->radius, 3*FRACUNIT/2)+FixedMul(4*FRACUNIT, actor->scale)),
		actor->z + actor->height/2);
}

// Function: A_Boss1Laser
//
// Description: Shoot an object at your target ala Bosses:
//
// var1 = object # to shoot
// var2:
//		0 - Boss 1 Left side
//		1 - Boss 1 Right side
//
void A_Boss1Laser(mobj_t *actor)
{
	fixed_t x, y, z, floorz, speed;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	INT32 i;
	angle_t angle;
	mobj_t *point;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss1Laser", actor))
		return;
#endif
	if (!actor->target)
		return;

	switch (locvar2)
	{
		case 0:
			x = actor->x + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(56*FRACUNIT, actor->scale) - mobjinfo[locvar1].height;
			else
				z = actor->z + FixedMul(56*FRACUNIT, actor->scale);
			break;
		case 1:
			x = actor->x + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			y = actor->y + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedMul(43*FRACUNIT, actor->scale));
			if (actor->eflags & MFE_VERTICALFLIP)
				z = actor->z + actor->height - FixedMul(56*FRACUNIT, actor->scale) - mobjinfo[locvar1].height;
			else
				z = actor->z + FixedMul(56*FRACUNIT, actor->scale);
			break;
		default:
			x = actor->x;
			y = actor->y;
			z = actor->z + actor->height/2;
			break;
	}

	if (!(actor->flags2 & MF2_FIRING))
	{
		actor->angle = R_PointToAngle2(x, y, actor->target->x, actor->target->y);
		if (mobjinfo[locvar1].seesound)
			S_StartSound(actor, mobjinfo[locvar1].seesound);
		if (!(actor->spawnpoint && actor->spawnpoint->options & MTF_AMBUSH))
		{
			point = P_SpawnMobj(x + P_ReturnThrustX(actor, actor->angle, actor->radius), y + P_ReturnThrustY(actor, actor->angle, actor->radius), actor->z - actor->height / 2, MT_EGGMOBILE_TARGET);
			point->angle = actor->angle;
			point->fuse = actor->tics+1;
			P_SetTarget(&point->target, actor->target);
			P_SetTarget(&actor->target, point);
		}
	}
	/* -- the following was relevant when the MT_EGGMOBILE_TARGET was allowed to move left and right from its path
	else if (actor->target && !(actor->spawnpoint && actor->spawnpoint->options & MTF_AMBUSH))
		actor->angle = R_PointToAngle2(x, y, actor->target->x, actor->target->y);*/

	if (actor->spawnpoint && actor->spawnpoint->options & MTF_AMBUSH)
		angle = FixedAngle(FixedDiv(actor->tics*160*FRACUNIT, actor->state->tics*FRACUNIT) + 10*FRACUNIT);
	else
		angle = R_PointToAngle2(z + (mobjinfo[locvar1].height>>1), 0, actor->target->z, R_PointToDist2(x, y, actor->target->x, actor->target->y));
	point = P_SpawnMobj(x, y, z, locvar1);
	P_SetTarget(&point->target, actor);
	point->angle = actor->angle;
	speed = point->radius*2;
	point->momz = FixedMul(FINECOSINE(angle>>ANGLETOFINESHIFT), speed);
	point->momx = FixedMul(FINESINE(angle>>ANGLETOFINESHIFT), FixedMul(FINECOSINE(point->angle>>ANGLETOFINESHIFT), speed));
	point->momy = FixedMul(FINESINE(angle>>ANGLETOFINESHIFT), FixedMul(FINESINE(point->angle>>ANGLETOFINESHIFT), speed));

	for (i = 0; i < 256; i++)
	{
		mobj_t *mo = P_SpawnMobj(point->x, point->y, point->z, point->type);
		mo->angle = point->angle;
		P_UnsetThingPosition(mo);
		mo->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY|MF_SCENERY;
		P_SetThingPosition(mo);

		x = point->x, y = point->y, z = point->z;
		if (P_RailThinker(point))
			break;
	}

	floorz = P_FloorzAtPos(x, y, z, mobjinfo[MT_EGGMOBILE_FIRE].height);
	if (z - floorz < mobjinfo[MT_EGGMOBILE_FIRE].height>>1)
	{
		point = P_SpawnMobj(x, y, floorz+1, MT_EGGMOBILE_FIRE);
		point->target = actor;
		point->destscale = 3*FRACUNIT;
		point->scalespeed = FRACUNIT>>2;
		point->fuse = TICRATE;
	}

	if (actor->tics > 1)
		actor->flags2 |= MF2_FIRING;
	else
		actor->flags2 &= ~MF2_FIRING;
}

// Function: A_FocusTarget
//
// Description: Home in on your target.
//
// var1:
//		0 - accelerative focus with friction
//		1 - steady focus with fixed movement speed
//      anything else - don't move
// var2:
//		0 - don't trace target, just move forwards
//      & 1 - change horizontal angle
//      & 2 - change vertical angle
//
void A_FocusTarget(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FocusTarget", actor))
		return;
#endif

	if (actor->target)
	{
		fixed_t speed = FixedMul(actor->info->speed, actor->scale);
		fixed_t dist = (locvar2 ? R_PointToDist2(actor->x, actor->y, actor->target->x, actor->target->y) : speed+1);
		angle_t hangle = ((locvar2 & 1) ? R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) : actor->angle);
		angle_t vangle = ((locvar2 & 2) ? R_PointToAngle2(actor->z , 0, actor->target->z + (actor->target->height>>1), dist) : ANGLE_90);
		switch(locvar1)
		{
		case 0:
			{
				actor->momx -= actor->momx>>4, actor->momy -= actor->momy>>4, actor->momz -= actor->momz>>4;
				actor->momz += FixedMul(FINECOSINE(vangle>>ANGLETOFINESHIFT), speed);
				actor->momx += FixedMul(FINESINE(vangle>>ANGLETOFINESHIFT), FixedMul(FINECOSINE(hangle>>ANGLETOFINESHIFT), speed));
				actor->momy += FixedMul(FINESINE(vangle>>ANGLETOFINESHIFT), FixedMul(FINESINE(hangle>>ANGLETOFINESHIFT), speed));
			}
			break;
		case 1:
			if (dist > speed)
			{
				actor->momz = FixedMul(FINECOSINE(vangle>>ANGLETOFINESHIFT), speed);
				actor->momx = FixedMul(FINESINE(vangle>>ANGLETOFINESHIFT), FixedMul(FINECOSINE(hangle>>ANGLETOFINESHIFT), speed));
				actor->momy = FixedMul(FINESINE(vangle>>ANGLETOFINESHIFT), FixedMul(FINESINE(hangle>>ANGLETOFINESHIFT), speed));
			}
			else
			{
				actor->momx = 0, actor->momy = 0, actor->momz = 0;
				actor->z = actor->target->z + (actor->target->height>>1);
				P_TryMove(actor, actor->target->x, actor->target->y, true);
			}
			break;
		default:
			break;
		}
	}
}

// Function: A_Boss4Reverse
//
// Description: Reverse arms direction.
//
// var1 = sfx to play
// var2 = unused
//
void A_Boss4Reverse(mobj_t *actor)
{
	sfxenum_t locvar1 = (sfxenum_t)var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss4Reverse", actor))
		return;
#endif
	S_StartSound(NULL, locvar1);
	actor->reactiontime = 0;
	if (actor->movedir == 1)
		actor->movedir = 2;
	else
		actor->movedir = 1;
}

// Function: A_Boss4SpeedUp
//
// Description: Speed up arms
//
// var1 = sfx to play
// var2 = unused
//
void A_Boss4SpeedUp(mobj_t *actor)
{
	sfxenum_t locvar1 = (sfxenum_t)var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss4SpeedUp", actor))
		return;
#endif
	S_StartSound(NULL, locvar1);
	actor->reactiontime = 2;
}

// Function: A_Boss4Raise
//
// Description: Raise helmet
//
// var1 = sfx to play
// var2 = unused
//
void A_Boss4Raise(mobj_t *actor)
{
	sfxenum_t locvar1 = (sfxenum_t)var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss4Raise", actor))
		return;
#endif
	S_StartSound(NULL, locvar1);
	actor->reactiontime = 1;
}

// Function: A_SkullAttack
//
// Description: Fly at the player like a missile.
//
// var1:
//		0 - Fly at the player
//		1 - Fly away from the player
//		2 - Strafe in relation to the player
// var2:
//		0 - Fly horizontally and vertically
//		1 - Fly horizontal-only (momz = 0)
//
#define SKULLSPEED (20*FRACUNIT)

void A_SkullAttack(mobj_t *actor)
{
	mobj_t *dest;
	angle_t an;
	INT32 dist;
	INT32 speed;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SkullAttack", actor))
		return;
#endif
	if (!actor->target)
		return;

	speed = FixedMul(SKULLSPEED, actor->scale);

	dest = actor->target;
	actor->flags2 |= MF2_SKULLFLY;
	if (actor->info->activesound)
		S_StartSound(actor, actor->info->activesound);
	A_FaceTarget(actor);

	if (locvar1 == 1)
		actor->angle += ANGLE_180;
	else if (locvar1 == 2)
		actor->angle += (P_RandomChance(FRACUNIT/2)) ? ANGLE_90 : -ANGLE_90;

	an = actor->angle >> ANGLETOFINESHIFT;

	actor->momx = FixedMul(speed, FINECOSINE(an));
	actor->momy = FixedMul(speed, FINESINE(an));
	dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
	dist = dist / speed;

	if (dist < 1)
		dist = 1;

	actor->momz = (dest->z + (dest->height>>1) - actor->z) / dist;

	if (locvar1 == 1)
		actor->momz = -actor->momz;
	if (locvar2 == 1)
		actor->momz = 0;
}

// Function: A_BossZoom
//
// Description: Like A_SkullAttack, but used by Boss 1.
//
// var1 = unused
// var2 = unused
//
void A_BossZoom(mobj_t *actor)
{
	mobj_t *dest;
	angle_t an;
	INT32 dist;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BossZoom", actor))
		return;
#endif
	if (!actor->target)
		return;

	dest = actor->target;
	actor->flags2 |= MF2_SKULLFLY;
	if (actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
	A_FaceTarget(actor);
	an = actor->angle >> ANGLETOFINESHIFT;
	actor->momx = FixedMul(FixedMul(actor->info->speed*5*FRACUNIT, actor->scale), FINECOSINE(an));
	actor->momy = FixedMul(FixedMul(actor->info->speed*5*FRACUNIT, actor->scale), FINESINE(an));
	dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
	dist = dist / FixedMul(actor->info->speed*5*FRACUNIT, actor->scale);

	if (dist < 1)
		dist = 1;
	actor->momz = (dest->z + (dest->height>>1) - actor->z) / dist;
}

// Function: A_BossScream
//
// Description: Spawns explosions and plays appropriate sounds around the defeated boss.
//
// var1:
//		0 - Use movecount to spawn explosions evenly
//		1 - Use P_Random to spawn explosions at complete random
// var2 = Object to spawn. Default is MT_BOSSEXPLODE.
//
void A_BossScream(mobj_t *actor)
{
	mobj_t *mo;
	fixed_t x, y, z;
	angle_t fa;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobjtype_t explodetype;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BossScream", actor))
		return;
#endif
	switch (locvar1)
	{
	default:
	case 0:
		actor->movecount += 4*16;
		actor->movecount %= 360;
		fa = (FixedAngle(actor->movecount*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
		break;
	case 1:
		fa = (FixedAngle(P_RandomKey(360)*FRACUNIT)>>ANGLETOFINESHIFT) & FINEMASK;
		break;
	}
	x = actor->x + FixedMul(FINECOSINE(fa),actor->radius);
	y = actor->y + FixedMul(FINESINE(fa),actor->radius);

	// Determine what mobj to spawn. If undefined or invalid, use MT_BOSSEXPLODE as default.
	if (locvar2 <= 0 || locvar2 >= NUMMOBJTYPES)
		explodetype = MT_BOSSEXPLODE;
	else
		explodetype = (mobjtype_t)locvar2;

	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - mobjinfo[explodetype].height - FixedMul((P_RandomByte()<<(FRACBITS-2)) - 8*FRACUNIT, actor->scale);
	else
		z = actor->z + FixedMul((P_RandomByte()<<(FRACBITS-2)) - 8*FRACUNIT, actor->scale);

	mo = P_SpawnMobj(x, y, z, explodetype);
	if (actor->eflags & MFE_VERTICALFLIP)
		mo->flags2 |= MF2_OBJECTFLIP;
	mo->destscale = actor->scale;
	P_SetScale(mo, mo->destscale);
	if (actor->info->deathsound)
		S_StartSound(mo, actor->info->deathsound);
}

// Function: A_Scream
//
// Description: Starts the death sound of the object.
//
// var1 = unused
// var2 = unused
//
void A_Scream(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Scream", actor))
		return;
#endif
	if (actor->tracer && (actor->tracer->type == MT_SHELL || actor->tracer->type == MT_FIREBALL))
		S_StartScreamSound(actor, sfx_mario2);
	else if (actor->info->deathsound)
		S_StartScreamSound(actor, actor->info->deathsound);
}

// Function: A_Pain
//
// Description: Starts the pain sound of the object.
//
// var1 = unused
// var2 = unused
//
void A_Pain(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Pain", actor))
		return;
#endif
	if (actor->info->painsound)
		S_StartSound(actor, actor->info->painsound);

	actor->flags2 &= ~MF2_FIRING;
	actor->flags2 &= ~MF2_SUPERFIRE;
}

// Function: A_Fall
//
// Description: Changes a dying object's flags to reflect its having fallen to the ground.
//
// var1 = unused
// var2 = unused
//
void A_Fall(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Fall", actor))
		return;
#endif
	// actor is on ground, it can be walked over
	actor->flags &= ~MF_SOLID;

	// fall through the floor
	actor->flags |= MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY;

	// So change this if corpse objects
	// are meant to be obstacles.
}

#define LIVESBOXDISPLAYPLAYER // Use displayplayer instead of closest player

// Function: A_1upThinker
//
// Description: Used by the 1up box to show the player's face.
//
// var1 = unused
// var2 = unused
//
void A_1upThinker(mobj_t *actor)
{
	INT32 i;
	fixed_t dist = INT32_MAX;
	fixed_t temp;
	INT32 closestplayer = -1;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_1upThinker", actor))
		return;
#endif
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].bot || players[i].spectator)
			continue;

		if (!players[i].mo)
			continue;

		if ((netgame || multiplayer) && players[i].playerstate != PST_LIVE)
			continue;

		temp = P_AproxDistance(players[i].mo->x-actor->x, players[i].mo->y-actor->y);

		if (temp < dist)
		{
			closestplayer = i;
			dist = temp;
		}
	}

	if (closestplayer == -1 || skins[players[closestplayer].skin].sprites[SPR2_LIFE].numframes == 0)
	{ // Closest player not found (no players in game?? may be empty dedicated server!), or does not have correct sprite.
		if (actor->tracer) {
			P_RemoveMobj(actor->tracer);
			actor->tracer = NULL;
		}
		return;
	}

	// We're using the overlay, so use the overlay 1up box (no text)
	actor->sprite = SPR_TV1P;

	if (!actor->tracer)
	{
		P_SetTarget(&actor->tracer, P_SpawnMobj(actor->x, actor->y, actor->z, MT_OVERLAY));
		P_SetTarget(&actor->tracer->target, actor);
		actor->tracer->skin = &skins[players[closestplayer].skin]; // required here to prevent spr2 default showing stand for a single frame
		P_SetMobjState(actor->tracer, actor->info->seestate);

		// The overlay is going to be one tic early turning off and on
		// because it's going to get its thinker run the frame we spawned it.
		// So make it take one tic longer if it just spawned.
		++actor->tracer->tics;
	}

	actor->tracer->color = players[closestplayer].mo->color;
	actor->tracer->skin = &skins[players[closestplayer].skin];
}

// Function: A_MonitorPop
//
// Description: Used by monitors when they explode.
//
// var1 = unused
// var2 = unused
//
void A_MonitorPop(mobj_t *actor)
{
	mobjtype_t item = 0;
	mobj_t *newmobj;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MonitorPop", actor))
		return;
#endif

	// Spawn the "pop" explosion.
	if (actor->info->deathsound)
		S_StartSound(actor, actor->info->deathsound);
	P_SpawnMobjFromMobj(actor, 0, 0, actor->height/4, MT_EXPLODE);

	// We're dead now. De-solidify.
	actor->health = 0;
	P_UnsetThingPosition(actor);
	actor->flags &= ~MF_SOLID;
	actor->flags |= MF_NOCLIP;
	P_SetThingPosition(actor);

	if (actor->info->damage == MT_UNKNOWN)
	{
		// MT_UNKNOWN is random. Because it's unknown to us... get it?
		item = P_DoRandomBoxChances();

		if (item == MT_NULL)
		{
			CONS_Alert(CONS_WARNING, M_GetText("All monitors turned off.\n"));
			return;
		}
	}
	else
		item = actor->info->damage;

	if (item == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup item not defined in 'damage' field for A_MonitorPop\n");
		return;
	}

	newmobj = P_SpawnMobjFromMobj(actor, 0, 0, 13*FRACUNIT, item);
	P_SetTarget(&newmobj->target, actor->target); // Transfer target

	if (item == MT_1UP_ICON)
	{
		if (actor->tracer) // Remove the old lives icon.
			P_RemoveMobj(actor->tracer);

		if (!newmobj->target
		 || !newmobj->target->player
		 || !newmobj->target->skin
		 || ((skin_t *)newmobj->target->skin)->sprites[SPR2_LIFE].numframes == 0)
			{} // No lives icon for this player, use the default.
		else
		{ // Spawn the lives icon.
			mobj_t *livesico = P_SpawnMobjFromMobj(newmobj, 0, 0, 0, MT_OVERLAY);
			P_SetTarget(&livesico->target, newmobj);
			P_SetTarget(&newmobj->tracer, livesico);

			livesico->color = newmobj->target->player->mo->color;
			livesico->skin = &skins[newmobj->target->player->skin];
			P_SetMobjState(livesico, newmobj->info->seestate);

			// We're using the overlay, so use the overlay 1up sprite (no text)
			newmobj->sprite = SPR_TV1P;
		}
	}
}

// Function: A_GoldMonitorPop
//
// Description: Used by repeating monitors when they turn off. They don't really pop, but, you know...
//
// var1 = unused
// var2 = unused
//
void A_GoldMonitorPop(mobj_t *actor)
{
	mobjtype_t item = 0;
	mobj_t *newmobj;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GoldMonitorPop", actor))
		return;
#endif

	// Don't spawn the "pop" explosion, because the monitor isn't broken.
	if (actor->info->deathsound)
		S_StartSound(actor, actor->info->deathsound);
	//P_SpawnMobjFromMobj(actor, 0, 0, actor.height/4, MT_EXPLODE);

	// Remove our flags for a bit.
	// Players can now stand on top of us.
	P_UnsetThingPosition(actor);
	actor->flags  &= ~(MF_MONITOR|MF_SHOOTABLE);
	actor->flags2 |= MF2_STANDONME;
	P_SetThingPosition(actor);

	// Don't count this box in statistics. Sorry.
	if (actor->target && actor->target->player)
		--actor->target->player->numboxes;
	actor->fuse = 0; // Don't let the monitor code screw us up.

	if (actor->info->damage == MT_UNKNOWN)
	{
		// MT_UNKNOWN is random. Because it's unknown to us... get it?
		item = P_DoRandomBoxChances();

		if (item == MT_NULL)
		{
			CONS_Alert(CONS_WARNING, M_GetText("All monitors turned off.\n"));
			return;
		}
	}
	else
		item = actor->info->damage;

	if (item == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup item not defined in 'damage' field for A_GoldMonitorPop\n");
		return;
	}

	// Note: the icon spawns 1 fracunit higher
	newmobj = P_SpawnMobjFromMobj(actor, 0, 0, 14*FRACUNIT, item);
	P_SetTarget(&newmobj->target, actor->target); // Transfer target

	if (item == MT_1UP_ICON)
	{
		if (actor->tracer) // Remove the old lives icon.
			P_RemoveMobj(actor->tracer);

		if (!newmobj->target
		 || !newmobj->target->player
		 || !newmobj->target->skin
		 || ((skin_t *)newmobj->target->skin)->sprites[SPR2_LIFE].numframes == 0)
			{} // No lives icon for this player, use the default.
		else
		{ // Spawn the lives icon.
			mobj_t *livesico = P_SpawnMobjFromMobj(newmobj, 0, 0, 0, MT_OVERLAY);
			P_SetTarget(&livesico->target, newmobj);
			P_SetTarget(&newmobj->tracer, livesico);

			livesico->color = newmobj->target->player->mo->color;
			livesico->skin = &skins[newmobj->target->player->skin];
			P_SetMobjState(livesico, newmobj->info->seestate);

			// We're using the overlay, so use the overlay 1up sprite (no text)
			newmobj->sprite = SPR_TV1P;
		}
	}
}

// Function: A_GoldMonitorRestore
//
// Description: A repeating monitor is coming back to life. Reset monitor flags, etc.
//
// var1 = unused
// var2 = unused
//
void A_GoldMonitorRestore(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GoldMonitorRestore", actor))
		return;
#endif

	actor->flags |= MF_MONITOR|MF_SHOOTABLE;
	actor->flags2 &= ~MF2_STANDONME;
	actor->health = 1; // Just in case.
}

// Function: A_GoldMonitorSparkle
//
// Description: Spawns the little sparkly effect around big monitors. Looks pretty, doesn't it?
//
// var1 = unused
// var2 = unused
//
void A_GoldMonitorSparkle(mobj_t *actor)
{
	fixed_t i, ngangle, xofs, yofs;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GoldMonitorSparkle", actor))
		return;
#endif

	ngangle = FixedAngle(((leveltime * 21) % 360) << FRACBITS);
	xofs = FINESINE((ngangle>>ANGLETOFINESHIFT) & FINEMASK)   * (actor->radius>>FRACBITS);
	yofs = FINECOSINE((ngangle>>ANGLETOFINESHIFT) & FINEMASK) * (actor->radius>>FRACBITS);

	for (i = FRACUNIT*2; i <= FRACUNIT*3; i += FRACUNIT/2)
		P_SetObjectMomZ(P_SpawnMobjFromMobj(actor, xofs, yofs, 0, MT_BOXSPARKLE), i, false);
}

// Function: A_Explode
//
// Description: Explodes an object, doing damage to any objects nearby. The target is used as the cause of the explosion. Damage value is used as explosion range.
//
// var1 = unused
// var2 = unused
//
void A_Explode(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Explode", actor))
		return;
#endif
	P_RadiusAttack(actor, actor->target, actor->info->damage);
}

// Function: A_BossDeath
//
// Description: Possibly trigger special effects when boss dies.
//
// var1 = unused
// var2 = unused
//
void A_BossDeath(mobj_t *mo)
{
	thinker_t *th;
	mobj_t *mo2;
	line_t junk;
	INT32 i;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BossDeath", mo))
		return;
#endif

	P_LinedefExecute(LE_BOSSDEAD, mo, NULL);
	mo->health = 0;

	// Boss is dead (but not necessarily fleeing...)
	// Lua may use this to ignore bosses after they start fleeing
	mo->flags2 |= MF2_BOSSDEAD;

	// make sure there is a player alive for victory
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && ((players[i].mo && players[i].mo->health)
			|| ((netgame || multiplayer) && (players[i].lives || players[i].continues))))
			break;

	if (i == MAXPLAYERS)
		return; // no one left alive, so do not end game

	// scan the remaining thinkers to see
	// if all bosses are dead
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2 != mo && (mo2->flags & MF_BOSS) && mo2->health > 0)
			goto bossjustdie; // other boss not dead - just go straight to dying!
	}

	// victory!
	P_LinedefExecute(LE_ALLBOSSESDEAD, mo, NULL);
	if (mo->flags2 & MF2_BOSSNOTRAP)
	{
		for (i = 0; i < MAXPLAYERS; i++)
			P_DoPlayerExit(&players[i]);
	}
	else
	{
		// Bring the egg trap up to the surface
		junk.tag = 680;
		EV_DoElevator(&junk, elevateHighest, false);
		junk.tag = 681;
		EV_DoElevator(&junk, elevateUp, false);
		junk.tag = 682;
		EV_DoElevator(&junk, elevateHighest, false);
	}

bossjustdie:
#ifdef HAVE_BLUA
	if (LUAh_BossDeath(mo))
		return;
	else if (P_MobjWasRemoved(mo))
		return;
#endif
	if (mo->type == MT_BLACKEGGMAN || mo->type == MT_CYBRAKDEMON)
	{
		mo->flags |= MF_NOCLIP;
		mo->flags &= ~MF_SPECIAL;

		S_StartSound(NULL, sfx_befall);
	}
	else if (mo->type == MT_KOOPA)
	{
		junk.tag = 650;
		EV_DoCeiling(&junk, raiseToHighest);
		return;
	}
	else // eggmobiles
	{
		// Stop exploding and prepare to run.
		P_SetMobjState(mo, mo->info->xdeathstate);
		if (P_MobjWasRemoved(mo))
			return;

		P_SetTarget(&mo->target, NULL);

		// Flee! Flee! Find a point to escape to! If none, just shoot upward!
		// scan the thinkers to find the runaway point
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type == MT_BOSSFLYPOINT)
			{
				// If this one's closer then the last one, go for it.
				if (!mo->target ||
					P_AproxDistance(P_AproxDistance(mo->x - mo2->x, mo->y - mo2->y), mo->z - mo2->z) <
					P_AproxDistance(P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y), mo->z - mo->target->z))
						P_SetTarget(&mo->target, mo2);
				// Otherwise... Don't!
			}
		}

		mo->flags |= MF_NOGRAVITY|MF_NOCLIP;
		mo->flags |= MF_NOCLIPHEIGHT;

		if (mo->target)
		{
			mo->angle = R_PointToAngle2(mo->x, mo->y, mo->target->x, mo->target->y);
			mo->flags2 |= MF2_BOSSFLEE;
			mo->momz = FixedMul(FixedDiv(mo->target->z - mo->z, P_AproxDistance(mo->x-mo->target->x,mo->y-mo->target->y)), FixedMul(2*FRACUNIT, mo->scale));
		}
		else
			mo->momz = FixedMul(2*FRACUNIT, mo->scale);
	}

	if (mo->type == MT_EGGMOBILE2)
	{
		mo2 = P_SpawnMobj(mo->x + P_ReturnThrustX(mo, mo->angle - ANGLE_90, FixedMul(32*FRACUNIT, mo->scale)),
			mo->y + P_ReturnThrustY(mo, mo->angle - ANGLE_90, FixedMul(32*FRACUNIT, mo->scale)),
			mo->z + mo->height/2 + ((mo->eflags & MFE_VERTICALFLIP)? FixedMul(8*FRACUNIT, mo->scale)-mobjinfo[MT_BOSSTANK1].height : -FixedMul(8*FRACUNIT, mo->scale)), MT_BOSSTANK1); // Right tank
		mo2->angle = mo->angle;
		mo2->destscale = mo->scale;
		P_SetScale(mo2, mo2->destscale);
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			mo2->eflags |= MFE_VERTICALFLIP;
			mo2->flags2 |= MF2_OBJECTFLIP;
		}
		P_InstaThrust(mo2, mo2->angle - ANGLE_90, FixedMul(4*FRACUNIT, mo2->scale));
		P_SetObjectMomZ(mo2, 4*FRACUNIT, false);

		mo2 = P_SpawnMobj(mo->x + P_ReturnThrustX(mo, mo->angle + ANGLE_90, FixedMul(32*FRACUNIT, mo->scale)),
			mo->y + P_ReturnThrustY(mo, mo->angle + ANGLE_90, FixedMul(32*FRACUNIT, mo->scale)),
			mo->z + mo->height/2 + ((mo->eflags & MFE_VERTICALFLIP)? FixedMul(8*FRACUNIT, mo->scale)-mobjinfo[MT_BOSSTANK2].height : -FixedMul(8*FRACUNIT, mo->scale)), MT_BOSSTANK2); // Left tank
		mo2->angle = mo->angle;
		mo2->destscale = mo->scale;
		P_SetScale(mo2, mo2->destscale);
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			mo2->eflags |= MFE_VERTICALFLIP;
			mo2->flags2 |= MF2_OBJECTFLIP;
		}
		P_InstaThrust(mo2, mo2->angle + ANGLE_90, FixedMul(4*FRACUNIT, mo2->scale));
		P_SetObjectMomZ(mo2, 4*FRACUNIT, false);

		mo2 = P_SpawnMobj(mo->x, mo->y,
			mo->z + ((mo->eflags & MFE_VERTICALFLIP)? mobjinfo[MT_BOSSSPIGOT].height-FixedMul(32*FRACUNIT,mo->scale): mo->height + FixedMul(32*FRACUNIT, mo->scale)), MT_BOSSSPIGOT);
		mo2->angle = mo->angle;
		mo2->destscale = mo->scale;
		P_SetScale(mo2, mo2->destscale);
		if (mo->eflags & MFE_VERTICALFLIP)
		{
			mo2->eflags |= MFE_VERTICALFLIP;
			mo2->flags2 |= MF2_OBJECTFLIP;
		}
		P_SetObjectMomZ(mo2, 4*FRACUNIT, false);
		return;
	}
}

// Function: A_CustomPower
//
// Description: Provides a custom powerup. Target (must be a player) is awarded the powerup. Reactiontime of the object is used as an index to the powers array.
//
// var1 = Power index #
// var2 = Power duration in tics
//
void A_CustomPower(mobj_t *actor)
{
	player_t *player;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	boolean spawnshield = false;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CustomPower", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	if (locvar1 >= NUMPOWERS)
	{
		CONS_Debug(DBG_GAMELOGIC, "Power #%d out of range!\n", locvar1);
		return;
	}

	player = actor->target->player;

	if (locvar1 == pw_shield && player->powers[pw_shield] != locvar2)
		spawnshield = true;

	player->powers[locvar1] = (UINT16)locvar2;
	if (actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);

	if (spawnshield) //workaround for a bug
		P_SpawnShieldOrb(player);
}

// Function: A_GiveWeapon
//
// Description: Gives the player the specified weapon panels.
//
// var1 = Weapon index #
// var2 = unused
//
void A_GiveWeapon(mobj_t *actor)
{
	player_t *player;
	INT32 locvar1 = var1;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GiveWeapon", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	if (locvar1 >= 1<<(NUM_WEAPONS-1))
	{
		CONS_Debug(DBG_GAMELOGIC, "Weapon #%d out of range!\n", locvar1);
		return;
	}

	player = actor->target->player;

	player->ringweapons |= locvar1;
	if (actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_RingBox
//
// Description: Awards the player 10 rings.
//
// var1 = unused
// var2 = unused
//
void A_RingBox(mobj_t *actor)
{
	player_t *player;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RingBox", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;

	P_GivePlayerRings(player, actor->info->reactiontime);
	if (actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_Invincibility
//
// Description: Awards the player invincibility.
//
// var1 = unused
// var2 = unused
//
void A_Invincibility(mobj_t *actor)
{
	player_t *player;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Invincibility", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;
	player->powers[pw_invulnerability] = invulntics + 1;

	if (P_IsLocalPlayer(player) && !player->powers[pw_super])
	{
		S_StopMusic();
		if (mariomode)
			G_GhostAddColor(GHC_INVINCIBLE);
		strlcpy(S_sfx[sfx_None].caption, "Invincibility", 14);
		S_StartCaption(sfx_None, -1, player->powers[pw_invulnerability]);
		S_ChangeMusicInternal((mariomode) ? "_minv" : "_inv", false);
	}
}

// Function: A_SuperSneakers
//
// Description: Awards the player super sneakers.
//
// var1 = unused
// var2 = unused
//
void A_SuperSneakers(mobj_t *actor)
{
	player_t *player;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SuperSneakers", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;

	actor->target->player->powers[pw_sneakers] = sneakertics + 1;

	if (P_IsLocalPlayer(player) && !player->powers[pw_super])
	{
		if (S_SpeedMusic(0.0f) && (mapheaderinfo[gamemap-1]->levelflags & LF_SPEEDMUSIC))
			S_SpeedMusic(1.4f);
		else
		{
			S_StopMusic();
			S_ChangeMusicInternal("_shoes", false);
		}
		strlcpy(S_sfx[sfx_None].caption, "Speed shoes", 12);
		S_StartCaption(sfx_None, -1, player->powers[pw_sneakers]);
	}
}

// Function: A_AwardScore
//
// Description: Adds a set amount of points to the player's score.
//
// var1 = unused
// var2 = unused
//
void A_AwardScore(mobj_t *actor)
{
	player_t *player;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_AwardScore", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;

	P_AddPlayerScore(player, actor->info->reactiontime);
	if (actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_ExtraLife
//
// Description: Awards the player an extra life.
//
// var1 = unused
// var2 = unused
//
void A_ExtraLife(mobj_t *actor)
{
	player_t *player;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ExtraLife", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;

	if (actor->type == MT_1UP_ICON && actor->tracer)
	{
		// We're using the overlay, so use the overlay 1up sprite (no text)
		actor->sprite = SPR_TV1P;
	}

	if (ultimatemode) //I don't THINK so!
	{
		S_StartSound(player->mo, sfx_lose);
		return;
	}

	// In shooter gametypes, give the player 100 rings instead of an extra life.
	if (gametype != GT_COOP && gametype != GT_COMPETITION)
	{
		P_GivePlayerRings(player, 100);
		P_PlayLivesJingle(player);
	}
	else
		P_GiveCoopLives(player, 1, true);
}

// Function: A_GiveShield
//
// Description: Awards the player a specified shield.
//
// var1 = Shield type (make with SH_ constants)
// var2 = unused
//
void A_GiveShield(mobj_t *actor)
{
	player_t *player;
	UINT16 locvar1 = var1;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GiveShield", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;

	P_SwitchShield(player, locvar1);
	S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_GravityBox
//
// Description: Awards the player gravity boots.
//
// var1 = unused
// var2 = unused
//
void A_GravityBox(mobj_t *actor)
{
	player_t *player;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GravityBox", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	player = actor->target->player;

	S_StartSound(player, actor->info->activesound);

	player->powers[pw_gravityboots] = (UINT16)(actor->info->reactiontime + 1);
}

// Function: A_ScoreRise
//
// Description: Makes the little score logos rise. Speed value sets speed.
//
// var1 = unused
// var2 = unused
//
void A_ScoreRise(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ScoreRise", actor))
		return;
#endif
	// make logo rise!
	P_SetObjectMomZ(actor, actor->info->speed, false);
}

// Function: A_ParticleSpawn
//
// Description: Hyper-specialised function for spawning a particle for MT_PARTICLEGEN.
//
// var1 = unused
// var2 = unused
//
void A_ParticleSpawn(mobj_t *actor)
{
	INT32 i = 0;
	mobj_t *spawn;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ParticleSpawn", actor))
		return;
#endif
	if (!actor->health)
		return;

	if (!actor->lastlook)
		return;

	if (!actor->threshold)
		return;

	for (i = 0; i < actor->lastlook; i++)
	{
		spawn = P_SpawnMobj(
			actor->x + FixedMul(FixedMul(actor->friction, actor->scale), FINECOSINE(actor->angle>>ANGLETOFINESHIFT)),
			actor->y + FixedMul(FixedMul(actor->friction, actor->scale), FINESINE(actor->angle>>ANGLETOFINESHIFT)),
			actor->z,
			(mobjtype_t)actor->threshold);
		P_SetScale(spawn, actor->scale);
		spawn->momz = FixedMul(actor->movefactor, spawn->scale);
		spawn->destscale = spawn->scale/100;
		spawn->scalespeed = spawn->scale/actor->health;
		spawn->tics = (tic_t)actor->health;
		spawn->flags2 |= (actor->flags2 & MF2_OBJECTFLIP);
		spawn->angle += P_RandomKey(36)*ANG10; // irrelevant for default objects but might make sense for some custom ones

		actor->angle += actor->movedir;
	}

	actor->angle += (angle_t)actor->movecount;
	actor->tics = (tic_t)actor->reactiontime;
}

// Function: A_BunnyHop
//
// Description: Makes object hop like a bunny.
//
// var1 = jump strength
// var2 = horizontal movement
//
void A_BunnyHop(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BunnyHop", actor))
		return;
#endif
	if (((actor->eflags & MFE_VERTICALFLIP) && actor->z + actor->height >= actor->ceilingz)
		|| (!(actor->eflags & MFE_VERTICALFLIP) && actor->z <= actor->floorz))
	{
		P_SetObjectMomZ(actor, locvar1*FRACUNIT, false);
		P_InstaThrust(actor, actor->angle, FixedMul(locvar2*FRACUNIT, actor->scale)); // Launch the hopping action! PHOOM!!
	}
}

// Function: A_BubbleSpawn
//
// Description: Spawns a randomly sized bubble from the object's location. Only works underwater.
//
// var1 = Distance to look for players.  If no player is in this distance, bubbles aren't spawned. (Ambush overrides)
// var2 = unused
//
void A_BubbleSpawn(mobj_t *actor)
{
	INT32 i, locvar1 = var1;
	UINT8 prandom;
	mobj_t *bubble = NULL;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BubbleSpawn", actor))
		return;
#endif
	if (!(actor->eflags & MFE_UNDERWATER))
	{
		// Don't draw or spawn bubbles above water
		actor->flags2 |= MF2_DONTDRAW;
		return;
	}
	actor->flags2 &= ~MF2_DONTDRAW;

	if (!(actor->flags2 & MF2_AMBUSH))
	{
		// Quick! Look through players!
		// Don't spawn bubbles unless a player is relatively close by (var2).
		for (i = 0; i < MAXPLAYERS; ++i)
			if (playeringame[i] && players[i].mo
			 && P_AproxDistance(actor->x - players[i].mo->x, actor->y - players[i].mo->y) < (locvar1<<FRACBITS))
				break; // Stop looking.
		if (i == MAXPLAYERS)
			return; // don't make bubble!
	}

	prandom = P_RandomByte();

	if (leveltime % (3*TICRATE) < 8)
		bubble = P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_EXTRALARGEBUBBLE);
	else if (prandom > 128)
		bubble = P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_SMALLBUBBLE);
	else if (prandom < 128 && prandom > 96)
		bubble = P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_MEDIUMBUBBLE);

	if (bubble)
	{
		bubble->destscale = actor->scale;
		P_SetScale(bubble, actor->scale);
	}
}

// Function: A_FanBubbleSpawn
//
// Description: Spawns bubbles from fans, if they're underwater.
//
// var1 = Distance to look for players.  If no player is in this distance, bubbles aren't spawned. (Ambush overrides)
// var2 = unused
//
void A_FanBubbleSpawn(mobj_t *actor)
{
	INT32 i, locvar1 = var1;
	UINT8 prandom;
	mobj_t *bubble = NULL;
	fixed_t hz = actor->z + (4*actor->height)/5;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FanBubbleSpawn", actor))
		return;
#endif
	if (!(actor->eflags & MFE_UNDERWATER))
		return;

	if (!(actor->flags2 & MF2_AMBUSH))
	{
	// Quick! Look through players!
	// Don't spawn bubbles unless a player is relatively close by (var2).
		for (i = 0; i < MAXPLAYERS; ++i)
			if (playeringame[i] && players[i].mo
			 && P_AproxDistance(actor->x - players[i].mo->x, actor->y - players[i].mo->y) < (locvar1<<FRACBITS))
				break; // Stop looking.
		if (i == MAXPLAYERS)
			return; // don't make bubble!
	}

	prandom = P_RandomByte();

	if ((prandom & 0x7) == 0x7)
		bubble = P_SpawnMobj(actor->x, actor->y, hz, MT_SMALLBUBBLE);
	else if ((prandom & 0xF0) == 0xF0)
		bubble = P_SpawnMobj(actor->x, actor->y, hz, MT_MEDIUMBUBBLE);

	if (bubble)
	{
		bubble->destscale = actor->scale;
		P_SetScale(bubble, actor->scale);
	}
}

// Function: A_BubbleRise
//
// Description: Raises a bubble
//
// var1:
//		0 = Bend around the water abit, looking more realistic
//		1 = Rise straight up
// var2 = rising speed
//
void A_BubbleRise(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BubbleRise", actor))
		return;
#endif
	if (actor->type == MT_EXTRALARGEBUBBLE)
		P_SetObjectMomZ(actor, FixedDiv(6*FRACUNIT,5*FRACUNIT), false); // make bubbles rise!
	else
	{
		P_SetObjectMomZ(actor, locvar2, true); // make bubbles rise!

		// Move around slightly to make it look like it's bending around the water
		if (!locvar1)
		{
			UINT8 prandom = P_RandomByte();
			if (!(prandom & 0x7)) // *****000
			{
				P_InstaThrust(actor, prandom & 0x70 ? actor->angle + ANGLE_90 : actor->angle,
					FixedMul(prandom & 0xF0 ? FRACUNIT/2 : -FRACUNIT/2, actor->scale));
			}
			else if (!(prandom & 0x38)) // **000***
			{
				P_InstaThrust(actor, prandom & 0x70 ? actor->angle - ANGLE_90 : actor->angle - ANGLE_180,
					FixedMul(prandom & 0xF0 ? FRACUNIT/2 : -FRACUNIT/2, actor->scale));
			}
		}
	}
}

// Function: A_BubbleCheck
//
// Description: Checks if a bubble should be drawn or not. Bubbles are not drawn above water.
//
// var1 = unused
// var2 = unused
//
void A_BubbleCheck(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BubbleCheck", actor))
		return;
#endif
	if (actor->eflags & MFE_UNDERWATER)
		actor->flags2 &= ~MF2_DONTDRAW; // underwater so draw
	else
		actor->flags2 |= MF2_DONTDRAW; // above water so don't draw
}

// Function: A_AttractChase
//
// Description: Makes a ring chase after a player with a ring shield and also causes spilled rings to flicker.
//
// var1 = unused
// var2 = unused
//
void A_AttractChase(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_AttractChase", actor))
		return;
#endif
	if (actor->flags2 & MF2_NIGHTSPULL || !actor->health)
		return;

	// spilled rings flicker before disappearing
	if (leveltime & 1 && actor->type == (mobjtype_t)actor->info->reactiontime && actor->fuse && actor->fuse < 2*TICRATE)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	// Turn flingrings back into regular rings if attracted.
	if (actor->tracer && actor->tracer->player
		&& !(actor->tracer->player->powers[pw_shield] & SH_PROTECTELECTRIC) && actor->info->reactiontime && actor->type != (mobjtype_t)actor->info->reactiontime)
	{
		mobj_t *newring;
		newring = P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->reactiontime);
		newring->momx = actor->momx;
		newring->momy = actor->momy;
		newring->momz = actor->momz;
		P_RemoveMobj(actor);
		return;
	}

	P_LookForShield(actor); // Go find 'em, boy!

	if (!actor->tracer
		|| !actor->tracer->player
		|| !actor->tracer->health
		|| !P_CheckSight(actor, actor->tracer)) // You have to be able to SEE it...sorta
	{
		// Lost attracted rings don't through walls anymore.
		actor->flags &= ~MF_NOCLIP;
		P_SetTarget(&actor->tracer, NULL);
		return;
	}

	// If a FlingRing gets attracted by a shield, change it into a normal ring.
	if (actor->type == (mobjtype_t)actor->info->reactiontime)
	{
		P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->painchance);
		P_RemoveMobj(actor);
		return;
	}

	// Keep stuff from going down inside floors and junk
	actor->flags &= ~MF_NOCLIPHEIGHT;

	// Let attracted rings move through walls and such.
	actor->flags |= MF_NOCLIP;

	P_Attract(actor, actor->tracer, false);
}

// Function: A_DropMine
//
// Description: Drops a mine. Raisestate specifies the object # to use for the mine.
//
// var1 = height offset
// var2:
//		lower 16 bits = proximity check distance (0 disables)
//		upper 16 bits = 0 to check proximity with target, 1 for tracer
//
void A_DropMine(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	fixed_t z;
	mobj_t *mine;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_DropMine", actor))
		return;
#endif

	if (locvar2 & 65535)
	{
		fixed_t dist;
		mobj_t *target;

		if (locvar2 >> 16)
			target = actor->tracer;
		else
			target = actor->target;

		if (!target)
			return;

		dist = P_AproxDistance(actor->x-target->x, actor->y-target->y)>>FRACBITS;

		if (dist > FixedMul((locvar2 & 65535), actor->scale))
			return;
	}

	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - mobjinfo[actor->info->raisestate].height - FixedMul((locvar1*FRACUNIT) - 12*FRACUNIT, actor->scale);
	else
		z = actor->z + FixedMul((locvar1*FRACUNIT) - 12*FRACUNIT, actor->scale);

	// Use raisestate instead of MT_MINE
	mine = P_SpawnMobj(actor->x, actor->y, z, (mobjtype_t)actor->info->raisestate);
	if (actor->eflags & MFE_VERTICALFLIP)
		mine->eflags |= MFE_VERTICALFLIP;
	mine->momz = actor->momz + actor->pmomz;

	S_StartSound(actor, actor->info->attacksound);
}

// Function: A_FishJump
//
// Description: Makes the stupid harmless fish in Greenflower Zone jump.
//
// var1 = Jump strength (in FRACBITS), if specified. Otherwise, uses the angle value.
// var2 = unused
//
void A_FishJump(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FishJump", actor))
		return;
#endif

	if (locvar2)
	{
		fixed_t rad = actor->radius>>FRACBITS;
		P_SpawnMobjFromMobj(actor, P_RandomRange(rad, -rad)<<FRACBITS, P_RandomRange(rad, -rad)<<FRACBITS, 0, (mobjtype_t)locvar2);
	}

	if ((actor->z <= actor->floorz) || (actor->z <= actor->watertop - FixedMul((64 << FRACBITS), actor->scale)))
	{
		fixed_t jumpval;

		if (locvar1)
			jumpval = var1;
		else
			jumpval = FixedMul(AngleFixed(actor->angle)/4, actor->scale);

		if (!jumpval) jumpval = FixedMul(44*(FRACUNIT/4), actor->scale);
		actor->momz = jumpval;
		P_SetMobjStateNF(actor, actor->info->seestate);
	}

	if (actor->momz < 0
		&& (actor->state < &states[actor->info->meleestate] || actor->state > &states[actor->info->xdeathstate]))
		P_SetMobjStateNF(actor, actor->info->meleestate);
}

// Function:A_ThrownRing
//
// Description: Thinker for thrown rings/sparkle trail
//
// var1 = unused
// var2 = unused
//
void A_ThrownRing(mobj_t *actor)
{
	INT32 c = 0;
	INT32 stop;
	player_t *player;
	fixed_t dist;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ThrownRing", actor))
		return;
#endif

	if (leveltime % (TICRATE/7) == 0)
	{
		mobj_t *ring = NULL;

		if (actor->flags2 & MF2_EXPLOSION)
		{
			if (actor->momx != 0 || actor->momy != 0)
				ring = P_SpawnMobj(actor->x, actor->y, actor->z, MT_SMOKE);
			// Else spawn nothing because it's totally stationary and constantly smoking would be weird -SH
		}
		else if (actor->flags2 & MF2_AUTOMATIC)
			ring = P_SpawnGhostMobj(actor);
		else if (!(actor->flags2 & MF2_RAILRING))
			ring = P_SpawnMobj(actor->x, actor->y, actor->z, MT_SPARK);

		if (ring)
		{
			/*
			P_SetTarget(&ring->target, actor);
			ring->color = actor->color; //copy color
			*/
			ring->destscale = actor->scale;
			P_SetScale(ring, actor->scale);
		}
	}

	// A_GrenadeRing beeping lives once moooooore -SH
	if (actor->type == MT_THROWNGRENADE && actor->fuse % TICRATE == 0)
		S_StartSound(actor, actor->info->attacksound);

	// decrement bounce ring time
	if (actor->flags2 & MF2_BOUNCERING)
	{
		if (actor->fuse)
			actor->fuse--;
		else {
			P_RemoveMobj(actor);
			return;
		}
	}

	// spilled rings (and thrown bounce) flicker before disappearing
	if (leveltime & 1 && actor->fuse > 0 && actor->fuse < 2*TICRATE
		&& actor->type != MT_THROWNGRENADE)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if (actor->tracer && actor->tracer->health <= 0)
		P_SetTarget(&actor->tracer, NULL);

	// Updated homing ring special capability
	// If you have a ring shield, all rings thrown
	// at you become homing (except rail)!
	if (actor->tracer)
	{
		// A non-homing ring getting attracted by a
		// magnetic player. If he gets too far away, make
		// sure to stop the attraction!
		if ((!actor->tracer->health) || (actor->tracer->player && (actor->tracer->player->powers[pw_shield] & SH_PROTECTELECTRIC)
		    && P_AproxDistance(P_AproxDistance(actor->tracer->x-actor->x,
		    actor->tracer->y-actor->y), actor->tracer->z-actor->z) > FixedMul(RING_DIST/4, actor->tracer->scale)))
		{
			P_SetTarget(&actor->tracer, NULL);
		}

		if (actor->tracer && (actor->tracer->health)
			&& (actor->tracer->player->powers[pw_shield] & SH_PROTECTELECTRIC))// Already found someone to follow.
		{
			const INT32 temp = actor->threshold;
			actor->threshold = 32000;
			P_HomingAttack(actor, actor->tracer);
			actor->threshold = temp;
			return;
		}
	}

	// first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_RandomByte();

	actor->lastlook %= MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for (; ; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
	{
		// done looking
		if (actor->lastlook == stop)
			return;

		if (!playeringame[actor->lastlook])
			continue;

		if (c++ == 2)
			return;

		player = &players[actor->lastlook];

		if (!player->mo)
			continue;

		if (player->mo->health <= 0)
			continue; // dead

		if ((netgame || multiplayer) && player->spectator)
			continue; // spectator

		if (actor->target && actor->target->player)
		{
			if (player->mo == actor->target)
				continue;

			// Don't home in on teammates.
			if (gametype == GT_CTF
				&& actor->target->player->ctfteam == player->ctfteam)
				continue;
		}

		dist = P_AproxDistance(P_AproxDistance(player->mo->x-actor->x,
			player->mo->y-actor->y), player->mo->z-actor->z);

		// check distance
		if (actor->flags2 & MF2_RAILRING)
		{
			if (dist > FixedMul(RING_DIST/2, player->mo->scale))
				continue;
		}
		else if (dist > FixedMul(RING_DIST, player->mo->scale))
			continue;

		// do this after distance check because it's more computationally expensive
		if (!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if ((player->powers[pw_shield] & SH_PROTECTELECTRIC)
			&& dist < FixedMul(RING_DIST/4, player->mo->scale))
			P_SetTarget(&actor->tracer, player->mo);
		return;
	}

	return;
}

// Function: A_SetSolidSteam
//
// Description: Makes steam solid so it collides with the player to boost them.
//
// var1 = unused
// var2 = unused
//
void A_SetSolidSteam(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetSolidSteam", actor))
		return;
#endif
	actor->flags &= ~MF_NOCLIP;
	actor->flags |= MF_SOLID;
	if (!(actor->flags2 & MF2_AMBUSH))
	{
		if (P_RandomChance(FRACUNIT/8))
		{
			if (actor->info->deathsound)
				S_StartSound(actor, actor->info->deathsound); // Hiss!
		}
		else
		{
			if (actor->info->painsound)
				S_StartSound(actor, actor->info->painsound);
		}
	}

	P_SetObjectMomZ (actor, 1, true);
}

// Function: A_UnsetSolidSteam
//
// Description: Makes an object non-solid and also noclip. Used by the steam.
//
// var1 = unused
// var2 = unused
//
void A_UnsetSolidSteam(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_UnsetSolidSteam", actor))
		return;
#endif
	actor->flags &= ~MF_SOLID;
	actor->flags |= MF_NOCLIP;
}

// Function: A_SignPlayer
//
// Description: Changes the state of a level end sign to reflect the player that hit it.
//
// var1 = unused
// var2 = unused
//
void A_SignPlayer(mobj_t *actor)
{
	mobj_t *ov;
	skin_t *skin;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SignPlayer", actor))
		return;
#endif
	if (!actor->target)
		return;

	if (!actor->target->player)
		return;

	skin = &skins[actor->target->player->skin];

	if ((actor->target->player->skincolor == skin->prefcolor) && (skin->prefoppositecolor)) // Set it as the skin's preferred oppositecolor?
	{
		actor->color = skin->prefoppositecolor;
		/*
		If you're here from the comment above Color_Opposite,
		the following line is the one which is dependent on the
		array being symmetrical. It gets the opposite of the
		opposite of your desired colour just so it can get the
		brightness frame for the End Sign. It's not a great
		design choice, but it's constant time array access and
		the idea that the colours should be OPPOSITES is kind
		of in the name. If you have a better idea, feel free
		to let me know. ~toast 2016/07/20
		*/
		actor->frame += (15 - Color_Opposite[(Color_Opposite[(skin->prefoppositecolor - 1)*2] - 1)*2 + 1]);
	}
	else if (actor->target->player->skincolor) // Set the sign to be an appropriate background color for this player's skincolor.
	{
		actor->color = Color_Opposite[(actor->target->player->skincolor - 1)*2];
		actor->frame += (15 - Color_Opposite[(actor->target->player->skincolor - 1)*2 + 1]);
	}

	if (skin->sprites[SPR2_SIGN].numframes)
	{
		// spawn an overlay of the player's face.
		ov = P_SpawnMobj(actor->x, actor->y, actor->z, MT_OVERLAY);
		P_SetTarget(&ov->target, actor);
		ov->color = actor->target->player->skincolor;
		ov->skin = skin;
		P_SetMobjState(ov, actor->info->seestate); // S_PLAY_SIGN
	}
}

// Function: A_OverlayThink
//
// Description: Moves the overlay to the position of its target.
//
// var1 = unused
// var2 = invert, z offset
//
void A_OverlayThink(mobj_t *actor)
{
	fixed_t destx, desty;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_OverlayThink", actor))
		return;
#endif
	if (!actor->target)
		return;

	if (!splitscreen && rendermode != render_soft)
	{
		angle_t viewingangle;

		if (players[displayplayer].awayviewtics)
			viewingangle = R_PointToAngle2(actor->target->x, actor->target->y, players[displayplayer].awayviewmobj->x, players[displayplayer].awayviewmobj->y);
		else if (!camera.chase && players[displayplayer].mo)
			viewingangle = R_PointToAngle2(actor->target->x, actor->target->y, players[displayplayer].mo->x, players[displayplayer].mo->y);
		else
			viewingangle = R_PointToAngle2(actor->target->x, actor->target->y, camera.x, camera.y);

		destx = actor->target->x + P_ReturnThrustX(actor->target, viewingangle, FixedMul(FRACUNIT, actor->scale));
		desty = actor->target->y + P_ReturnThrustY(actor->target, viewingangle, FixedMul(FRACUNIT, actor->scale));
	}
	else
	{
		destx = actor->target->x;
		desty = actor->target->y;
	}
	P_UnsetThingPosition(actor);
	actor->x = destx;
	actor->y = desty;
	P_SetThingPosition(actor);
	if (actor->eflags & MFE_VERTICALFLIP)
		actor->z = actor->target->z + actor->target->height - mobjinfo[actor->type].height  - ((var2>>16) ? -1 : 1)*(var2&0xFFFF)*FRACUNIT;
	else
		actor->z = actor->target->z + ((var2>>16) ? -1 : 1)*(var2&0xFFFF)*FRACUNIT;
	actor->angle = actor->target->angle;
	actor->eflags = actor->target->eflags;

	actor->momx = actor->target->momx;
	actor->momy = actor->target->momy;
	actor->momz = actor->target->momz; // assume target has correct momz! Do not use P_SetObjectMomZ!
}

// Function: A_JetChase
//
// Description: A_Chase for Jettysyns
//
// var1 = unused
// var2 = unused
//
void A_JetChase(mobj_t *actor)
{
	fixed_t thefloor;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_JetChase", actor))
		return;
#endif

	if (actor->flags2 & MF2_AMBUSH)
		return;

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - FixedMul(256*FRACUNIT, actor->scale))
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->reactiontime)
		actor->reactiontime--;

	if (P_RandomChance(FRACUNIT/32))
	{
		actor->momx = actor->momx / 2;
		actor->momy = actor->momy / 2;
		actor->momz = actor->momz / 2;
	}

	// Bounce if too close to floor or ceiling -
	// ideal for Jetty-Syns above you on 3d floors
	if (actor->momz && ((actor->z - FixedMul((32<<FRACBITS), actor->scale)) < thefloor) && !((thefloor + FixedMul(32*FRACUNIT, actor->scale) + actor->height) > actor->ceilingz))
		actor->momz = -actor->momz/2;

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if ((multiplayer || netgame) && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target)))
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

	// If the player is over 3072 fracunits away, then look for another player
	if (P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y),
		actor->target->z - actor->z) > FixedMul(3072*FRACUNIT, actor->scale) && P_LookForPlayers(actor, true, false, FixedMul(3072*FRACUNIT, actor->scale)))
	{
		return; // got a new target
	}

	// chase towards player
	if (ultimatemode)
		P_Thrust(actor, actor->angle, FixedMul(actor->info->speed/2, actor->scale));
	else
		P_Thrust(actor, actor->angle, FixedMul(actor->info->speed/4, actor->scale));

	// must adjust height
	if (ultimatemode)
	{
		if (actor->z < (actor->target->z + actor->target->height + FixedMul((64<<FRACBITS), actor->scale)))
			actor->momz += FixedMul(FRACUNIT/2, actor->scale);
		else
			actor->momz -= FixedMul(FRACUNIT/2, actor->scale);
	}
	else
	{
		if (actor->z < (actor->target->z + actor->target->height + FixedMul((32<<FRACBITS), actor->scale)))
			actor->momz += FixedMul(FRACUNIT/2, actor->scale);
		else
			actor->momz -= FixedMul(FRACUNIT/2, actor->scale);
	}
}

// Function: A_JetbThink
//
// Description: Thinker for Jetty-Syn bombers
//
// var1 = unused
// var2 = unused
//
void A_JetbThink(mobj_t *actor)
{
	sector_t *nextsector;
	fixed_t thefloor;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_JetbThink", actor))
		return;
#endif

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - FixedMul(256*FRACUNIT, actor->scale))
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->target)
	{
		A_JetChase(actor);
		// check for melee attack
		if ((actor->z > (actor->floorz + FixedMul((32<<FRACBITS), actor->scale)))
			&& P_JetbCheckMeleeRange(actor) && !actor->reactiontime
			&& (actor->target->z >= actor->floorz))
		{
			mobj_t *bomb;
			if (actor->info->attacksound)
				S_StartAttackSound(actor, actor->info->attacksound);

			// use raisestate instead of MT_MINE
			bomb = P_SpawnMobj(actor->x, actor->y, actor->z - FixedMul((32<<FRACBITS), actor->scale), (mobjtype_t)actor->info->raisestate);

			P_SetTarget(&bomb->target, actor);
			bomb->destscale = actor->scale;
			P_SetScale(bomb, actor->scale);
			actor->reactiontime = TICRATE; // one second
			S_StartSound(actor, actor->info->attacksound);
		}
	}
	else if (((actor->z - FixedMul((32<<FRACBITS), actor->scale)) < thefloor) && !((thefloor + FixedMul((32<<FRACBITS), actor->scale) + actor->height) > actor->ceilingz))
			actor->z = thefloor+FixedMul((32<<FRACBITS), actor->scale);

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if (nextsector->ceilingheight < actor->z + actor->height)
		actor->momz -= FixedMul(5*FRACUNIT, actor->scale);
	else if (nextsector->floorheight > actor->z)
		actor->momz += FixedMul(5*FRACUNIT, actor->scale);
}

// Function: A_JetgShoot
//
// Description: Firing function for Jetty-Syn gunners.
//
// var1 = unused
// var2 = unused
//
void A_JetgShoot(mobj_t *actor)
{
	fixed_t dist;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_JetgShoot", actor))
		return;
#endif

	if (!actor->target)
		return;

	if (actor->reactiontime)
		return;

	dist = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);

	if (dist > FixedMul(actor->info->painchance*FRACUNIT, actor->scale))
		return;

	if (dist < FixedMul(64*FRACUNIT, actor->scale))
		return;

	A_FaceTarget(actor);
	P_SpawnMissile(actor, actor->target, (mobjtype_t)actor->info->raisestate);

	if (ultimatemode)
		actor->reactiontime = actor->info->reactiontime*TICRATE;
	else
		actor->reactiontime = actor->info->reactiontime*TICRATE*2;

	if (actor->info->attacksound)
		S_StartSound(actor, actor->info->attacksound);
}

// Function: A_ShootBullet
//
// Description: Shoots a bullet. Raisestate defines object # to use as projectile.
//
// var1 = unused
// var2 = unused
//
void A_ShootBullet(mobj_t *actor)
{
	fixed_t dist;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ShootBullet", actor))
		return;
#endif

	if (!actor->target)
		return;

	dist = P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y), actor->target->z - actor->z);

	if (dist > FixedMul(actor->info->painchance*FRACUNIT, actor->scale))
		return;

	A_FaceTarget(actor);
	P_SpawnMissile(actor, actor->target, (mobjtype_t)actor->info->raisestate);

	if (actor->info->attacksound)
		S_StartSound(actor, actor->info->attacksound);
}

// Function: A_MinusDigging
//
// Description: Minus digging in the ground.
//
// var1 = unused
// var2 = unused
//
void A_MinusDigging(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MinusDigging", actor))
		return;
#endif
	actor->flags &= ~MF_SPECIAL;
	actor->flags &= ~MF_SHOOTABLE;

	if (!actor->target)
	{
		A_Look(actor);
		return;
	}

	if (actor->reactiontime)
	{
		actor->reactiontime--;
		return;
	}

	// Dirt trail
	P_SpawnGhostMobj(actor);

	actor->flags |= MF_NOCLIPTHING;
	var1 = 3;
	A_Chase(actor);
	actor->flags &= ~MF_NOCLIPTHING;

	// Play digging sound
	if (!(leveltime & 15))
		S_StartSound(actor, actor->info->activesound);

	// If we're close enough to our target, pop out of the ground
	if (P_AproxDistance(actor->target->x-actor->x, actor->target->y-actor->y) < actor->radius
		&& abs(actor->target->z - actor->z) < 2*actor->height)
		P_SetMobjState(actor, actor->info->missilestate);

	// Snap to ground
	if (actor->eflags & MFE_VERTICALFLIP)
		actor->z = actor->ceilingz - actor->height;
	else
		actor->z = actor->floorz;
}

// Function: A_MinusPopup
//
// Description: Minus popping out of the ground.
//
// var1 = unused
// var2 = unused
//
void A_MinusPopup(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MinusPopup", actor))
		return;
#endif
	P_SetObjectMomZ(actor, 10*FRACUNIT, false);

	actor->flags |= MF_SPECIAL;
	actor->flags |= MF_SHOOTABLE;

	// Sound for busting out of the ground.
	S_StartSound(actor, actor->info->attacksound);
}

// Function: A_MinusCheck
//
// Description: If the minus hits the floor, dig back into the ground.
//
// var1 = unused
// var2 = unused
//
void A_MinusCheck(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MinusCheck", actor))
		return;
#endif
	if ((!(actor->eflags & MFE_VERTICALFLIP) && actor->z <= actor->floorz)
	|| ((actor->eflags & MFE_VERTICALFLIP) && actor->z + actor->height >= actor->ceilingz))
	{
		actor->flags &= ~MF_SPECIAL;
		actor->flags &= ~MF_SHOOTABLE;
		actor->reactiontime = TICRATE;
		P_SetMobjState(actor, actor->info->seestate);
		return;
	}

	// 'Falling' animation
	if (P_MobjFlip(actor)*actor->momz < 0 && actor->state < &states[actor->info->meleestate])
		P_SetMobjState(actor, actor->info->meleestate);
}

// Function: A_ChickenCheck
//
// Description: Resets the chicken once it hits the floor again.
//
// var1 = unused
// var2 = unused
//
void A_ChickenCheck(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ChickenCheck", actor))
		return;
#endif
	if ((!(actor->eflags & MFE_VERTICALFLIP) && actor->z <= actor->floorz)
	|| (actor->eflags & MFE_VERTICALFLIP && actor->z + actor->height >= actor->ceilingz))
	{
		if (!(actor->momx || actor->momy || actor->momz)
			&& actor->state > &states[actor->info->seestate])
		{
			A_Chase(actor);
			P_SetMobjState(actor, actor->info->seestate);
		}

		actor->momx >>= 2;
		actor->momy >>= 2;
	}
}

// Function: A_JetgThink
//
// Description: Thinker for Jetty-Syn Gunners
//
// var1 = unused
// var2 = unused
//
void A_JetgThink(mobj_t *actor)
{
	sector_t *nextsector;

	fixed_t thefloor;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_JetgThink", actor))
		return;
#endif

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - FixedMul(256*FRACUNIT, actor->scale))
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->target)
	{
		if (P_RandomChance(FRACUNIT/8) && !actor->reactiontime)
			P_SetMobjState(actor, actor->info->missilestate);
		else
			A_JetChase (actor);
	}
	else if (actor->z - FixedMul((32<<FRACBITS), actor->scale) < thefloor && !(thefloor + FixedMul((32<<FRACBITS), actor->scale)
		+ actor->height > actor->ceilingz))
	{
		actor->z = thefloor + FixedMul((32<<FRACBITS), actor->scale);
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if (nextsector->ceilingheight < actor->z + actor->height)
		actor->momz -= FixedMul(5*FRACUNIT, actor->scale);
	else if (nextsector->floorheight > actor->z)
		actor->momz += FixedMul(5*FRACUNIT, actor->scale);
}

// Function: A_MouseThink
//
// Description: Thinker for scurrying mice.
//
// var1 = unused
// var2 = unused
//
void A_MouseThink(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MouseThink", actor))
		return;
#endif

	if (actor->reactiontime)
		actor->reactiontime--;

	if (((!(actor->eflags & MFE_VERTICALFLIP) && actor->z == actor->floorz)
		|| (actor->eflags & MFE_VERTICALFLIP && actor->z + actor->height == actor->ceilingz))
		&& !actor->reactiontime)
	{
		if (twodlevel || actor->flags2 & MF2_TWOD)
		{
			if (P_RandomChance(FRACUNIT/2))
				actor->angle += ANGLE_180;
		}
		else if (P_RandomChance(FRACUNIT/2))
			actor->angle += ANGLE_90;
		else
			actor->angle -= ANGLE_90;

		P_InstaThrust(actor, actor->angle, FixedMul(actor->info->speed, actor->scale));
		actor->reactiontime = TICRATE/5;
	}
}

// Function: A_DetonChase
//
// Description: Chases a Deton after a player.
//
// var1 = unused
// var2 = unused
//
void A_DetonChase(mobj_t *actor)
{
	angle_t exact;
	fixed_t xydist, dist;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_DetonChase", actor))
		return;
#endif

	// modify tracer threshold
	if (!actor->tracer || actor->tracer->health <= 0)
		actor->threshold = 0;
	else
		actor->threshold = 1;

	if (!actor->tracer || !(actor->tracer->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, true, 0))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	if (multiplayer && !actor->threshold && P_LookForPlayers(actor, true, true, 0))
		return; // got a new target

	// Face movement direction if not doing so
	exact = R_PointToAngle2(actor->x, actor->y, actor->tracer->x, actor->tracer->y);
	actor->angle = exact;
	/*if (exact != actor->angle)
	{
		if (exact - actor->angle > ANGLE_180)
		{
			actor->angle -= actor->info->raisestate;
			if (exact - actor->angle < ANGLE_180)
				actor->angle = exact;
		}
		else
		{
			actor->angle += actor->info->raisestate;
			if (exact - actor->angle > ANGLE_180)
				actor->angle = exact;
		}
	}*/
	// movedir is up/down angle: how much it has to go up as it goes over to the player
	xydist = P_AproxDistance(actor->tracer->x - actor->x, actor->tracer->y - actor->y);
	exact = R_PointToAngle2(0, 0, xydist, actor->tracer->z - actor->z);
	actor->movedir = exact;
	/*if (exact != actor->movedir)
	{
		if (exact - actor->movedir > ANGLE_180)
		{
			actor->movedir -= actor->info->raisestate;
			if (exact - actor->movedir < ANGLE_180)
				actor->movedir = exact;
		}
		else
		{
			actor->movedir += actor->info->raisestate;
			if (exact - actor->movedir > ANGLE_180)
				actor->movedir = exact;
		}
	}*/

	// check for melee attack
	if (actor->tracer)
	{
		if (P_AproxDistance(actor->tracer->x-actor->x, actor->tracer->y-actor->y) < actor->radius+actor->tracer->radius)
		{
			if (!((actor->tracer->z > actor->z + actor->height) || (actor->z > actor->tracer->z + actor->tracer->height)))
			{
				P_ExplodeMissile(actor);
				return;
			}
		}
	}

	// chase towards player
	if ((dist = P_AproxDistance(xydist, actor->tracer->z-actor->z))
		> FixedMul((actor->info->painchance << FRACBITS), actor->scale))
	{
		P_SetTarget(&actor->tracer, NULL); // Too far away
		return;
	}

	if (actor->reactiontime == 0)
	{
		actor->reactiontime = actor->info->reactiontime;
		return;
	}

	if (actor->reactiontime > 1)
	{
		actor->reactiontime--;
		return;
	}

	if (actor->reactiontime > 0)
	{
		actor->reactiontime = -42;

		if (actor->info->seesound)
			S_StartScreamSound(actor, actor->info->seesound);
	}

	if (actor->reactiontime == -42)
	{
		fixed_t xyspeed;

		actor->reactiontime = -42;

		exact = actor->movedir>>ANGLETOFINESHIFT;
		xyspeed = FixedMul(FixedMul(actor->tracer->player->normalspeed,3*FRACUNIT/4), FINECOSINE(exact));
		actor->momz = FixedMul(FixedMul(actor->tracer->player->normalspeed,3*FRACUNIT/4), FINESINE(exact));

		exact = actor->angle>>ANGLETOFINESHIFT;
		actor->momx = FixedMul(xyspeed, FINECOSINE(exact));
		actor->momy = FixedMul(xyspeed, FINESINE(exact));

		// Variable re-use
		xyspeed = (P_AproxDistance(actor->tracer->x - actor->x, P_AproxDistance(actor->tracer->y - actor->y, actor->tracer->z - actor->z))>>(FRACBITS+6));

		if (xyspeed < 1)
			xyspeed = 1;

		if (leveltime % xyspeed == 0)
			S_StartSound(actor, sfx_deton);
	}
}

// Function: A_CapeChase
//
// Description: Set an object's location to its target or tracer.
//
// var1:
//		0 = Use target
//		1 = Use tracer
//		upper 16 bits = Z offset
// var2:
//		upper 16 bits = forward/backward offset
//		lower 16 bits = sideways offset
//
void A_CapeChase(mobj_t *actor)
{
	mobj_t *chaser;
	fixed_t foffsetx, foffsety, boffsetx, boffsety;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	angle_t angle;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CapeChase", actor))
		return;
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_CapeChase called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	if (locvar1 & 65535)
		chaser = actor->tracer;
	else
		chaser = actor->target;

	if (!chaser || (chaser->health <= 0))
	{
		if (chaser)
			CONS_Debug(DBG_GAMELOGIC, "Hmm, the guy I'm chasing (object type %d) has no health.. so I'll die too!\n", chaser->type);

		P_RemoveMobj(actor);
		return;
	}

	angle = (chaser->player ? chaser->player->drawangle : chaser->angle);

	foffsetx = P_ReturnThrustX(chaser, angle, FixedMul((locvar2 >> 16)*FRACUNIT, actor->scale));
	foffsety = P_ReturnThrustY(chaser, angle, FixedMul((locvar2 >> 16)*FRACUNIT, actor->scale));

	boffsetx = P_ReturnThrustX(chaser, angle-ANGLE_90, FixedMul((locvar2 & 65535)*FRACUNIT, actor->scale));
	boffsety = P_ReturnThrustY(chaser, angle-ANGLE_90, FixedMul((locvar2 & 65535)*FRACUNIT, actor->scale));

	P_UnsetThingPosition(actor);
	actor->x = chaser->x + foffsetx + boffsetx;
	actor->y = chaser->y + foffsety + boffsety;
	if (chaser->eflags & MFE_VERTICALFLIP)
	{
		actor->eflags |= MFE_VERTICALFLIP;
		actor->flags2 |= MF2_OBJECTFLIP;
		actor->z = chaser->z + chaser->height - actor->height - FixedMul((locvar1 >> 16)*FRACUNIT, actor->scale);
	}
	else
	{
		actor->eflags &= ~MFE_VERTICALFLIP;
		actor->flags2 &= ~MF2_OBJECTFLIP;
		actor->z = chaser->z + FixedMul((locvar1 >> 16)*FRACUNIT, actor->scale);
	}
	actor->angle = angle;
	P_SetThingPosition(actor);
}

// Function: A_RotateSpikeBall
//
// Description: Rotates a spike ball around its target/tracer.
//
// var1:
//		0 = Use target
//		1 = Use tracer
// var2 = unused
//
void A_RotateSpikeBall(mobj_t *actor)
{
	INT32 locvar1 = var1;
	const fixed_t radius = FixedMul(12*actor->info->speed, actor->scale);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RotateSpikeBall", actor))
		return;
#endif

	if (actor->type == MT_SPECIALSPIKEBALL) // don't remove this, these spikeballs share the same states as the rotating spikeballs
		return;

	if (!((!locvar1 && (actor->target)) || (locvar1 && (actor->tracer))))// This should NEVER happen.
	{
		CONS_Debug(DBG_GAMELOGIC, "A_RotateSpikeBall: Spikeball has no target\n");
		P_RemoveMobj(actor);
		return;
	}

	if (!actor->info->speed)
	{
		CONS_Debug(DBG_GAMELOGIC, "A_RotateSpikeBall: Object has no speed.\n");
		return;
	}

	actor->angle += FixedAngle(actor->info->speed);
	P_UnsetThingPosition(actor);
	{
		const angle_t fa = actor->angle>>ANGLETOFINESHIFT;
		if (!locvar1)
		{
			actor->x = actor->target->x + FixedMul(FINECOSINE(fa),radius);
			actor->y = actor->target->y + FixedMul(FINESINE(fa),radius);
			actor->z = actor->target->z + actor->target->height/2;
		}
		else
		{
			actor->x = actor->tracer->x + FixedMul(FINECOSINE(fa),radius);
			actor->y = actor->tracer->y + FixedMul(FINESINE(fa),radius);
			actor->z = actor->tracer->z + actor->tracer->height/2;
		}
		P_SetThingPosition(actor);
	}
}

// Function: A_UnidusBall
//
// Description: Rotates a spike ball around its target.
//
// var1:
//		0 = Don't throw
//		1 = Throw
//		2 = Throw when target leaves MF2_SKULLFLY.
// var2 = unused
//
void A_UnidusBall(mobj_t *actor)
{
	INT32 locvar1 = var1;
	boolean canthrow = false;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_UnidusBall", actor))
		return;
#endif

	actor->angle += ANGLE_11hh;

	if (actor->movecount)
	{
		if (P_AproxDistance(actor->momx, actor->momy) < FixedMul(actor->info->damage/2, actor->scale))
			P_ExplodeMissile(actor);
		return;
	}

	if (!actor->target || !actor->target->health)
	{
		CONS_Debug(DBG_GAMELOGIC, "A_UnidusBall: Removing unthrown spikeball from nonexistant Unidus\n");
		P_RemoveMobj(actor);
		return;
	}

	P_UnsetThingPosition(actor);
	{
		const angle_t angle = actor->movedir + FixedAngle(actor->info->speed*(leveltime%360));
		const UINT16 fa = angle>>ANGLETOFINESHIFT;

		actor->x = actor->target->x + FixedMul(FINECOSINE(fa),actor->threshold);
		actor->y = actor->target->y + FixedMul(  FINESINE(fa),actor->threshold);
		actor->z = actor->target->z + actor->target->height/2 - actor->height/2;

		if (locvar1 == 1 && actor->target->target)
		{
			const angle_t tang = R_PointToAngle2(actor->target->x, actor->target->y, actor->target->target->x, actor->target->target->y);
			const angle_t mina = tang-ANGLE_11hh;
			canthrow = (angle-mina < FixedAngle(actor->info->speed*3));
		}
	}
	P_SetThingPosition(actor);

	if (locvar1 == 1 && canthrow)
	{
		if (P_AproxDistance(actor->target->target->x - actor->target->x, actor->target->target->y - actor->target->y) > FixedMul(MISSILERANGE>>1, actor->scale)
		|| !P_CheckSight(actor, actor->target->target))
			return;

		actor->movecount = actor->info->damage>>FRACBITS;
		actor->flags &= ~(MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING);
		P_InstaThrust(actor, R_PointToAngle2(actor->x, actor->y, actor->target->target->x, actor->target->target->y), FixedMul(actor->info->damage, actor->scale));
	}
	else if (locvar1 == 2)
	{
		boolean skull = (actor->target->flags2 & MF2_SKULLFLY) == MF2_SKULLFLY;
		if (actor->target->state == &states[actor->target->info->painstate])
		{
			P_KillMobj(actor, NULL, NULL, 0);
			return;
		}
		switch(actor->extravalue2)
		{
		case 0: // at least one frame where not dashing
			if (!skull) ++actor->extravalue2;
			else break;
		case 1: // at least one frame where ARE dashing
			if (skull) ++actor->extravalue2;
			else break;
		case 2: // not dashing again?
			if (skull) break;
			// launch.
		{
			mobj_t *target = actor->target;
			if (actor->target->target)
				target = actor->target->target;
			actor->movecount = actor->info->damage>>FRACBITS;
			actor->flags &= ~(MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOCLIPTHING);
			P_InstaThrust(actor, R_PointToAngle2(actor->x, actor->y, target->x, target->y), FixedMul(actor->info->damage, actor->scale));
		}
		default: // from our compiler appeasement program (CAP).
			break;
		}
	}
}

// Function: A_RockSpawn
//
// Spawns rocks at a specified interval
//
// var1 = unused
// var2 = unused
void A_RockSpawn(mobj_t *actor)
{
	mobj_t *mo;
	mobjtype_t type;
	INT32 i = P_FindSpecialLineFromTag(12, (INT16)actor->threshold, -1);
	line_t *line;
	fixed_t dist;
	fixed_t randomoomph;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RockSpawn", actor))
		return;
#endif

	if (i == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "A_RockSpawn: Unable to find parameter line 12 (tag %d)!\n", actor->threshold);
		return;
	}

	line = &lines[i];

	if (!(sides[line->sidenum[0]].textureoffset >> FRACBITS))
	{
		CONS_Debug(DBG_GAMELOGIC, "A_RockSpawn: No X-offset detected! (tag %d)!\n", actor->threshold);
		return;
	}

	dist = P_AproxDistance(line->dx, line->dy)/16;

	if (dist < 1)
		dist = 1;

	type = MT_ROCKCRUMBLE1 + (sides[line->sidenum[0]].rowoffset >> FRACBITS);

	if (line->flags & ML_NOCLIMB)
		randomoomph = P_RandomByte() * (FRACUNIT/32);
	else
		randomoomph = 0;

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_FALLINGROCK);
	P_SetMobjState(mo, mobjinfo[type].spawnstate);
	mo->angle = R_PointToAngle2(line->v2->x, line->v2->y, line->v1->x, line->v1->y);

	P_InstaThrust(mo, mo->angle, dist + randomoomph);
	mo->momz = dist + randomoomph;

	var1 = sides[line->sidenum[0]].textureoffset >> FRACBITS;
	A_SetTics(actor);
}

//
// Function: A_SlingAppear
//
// Appears a sling.
//
// var1 = unused
// var2 = unused
//
void A_SlingAppear(mobj_t *actor)
{
	boolean firsttime = true;
	UINT8 mlength = 4;
	mobj_t *spawnee;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SlingAppear", actor))
		return;
#endif

	P_UnsetThingPosition(actor);
	actor->flags &= ~(MF_NOBLOCKMAP|MF_NOCLIP|MF_NOGRAVITY|MF_NOCLIPHEIGHT);
	P_SetThingPosition(actor);
	actor->lastlook = 128;
	actor->movecount = actor->lastlook;
	actor->health = actor->angle>>ANGLETOFINESHIFT;
	actor->threshold = 0;
	actor->movefactor = actor->threshold;
	actor->friction = 128;

	while (mlength > 0)
	{
		spawnee = P_SpawnMobj(actor->x, actor->y, actor->z, MT_SMALLMACECHAIN);

		P_SetTarget(&spawnee->target, actor);

		spawnee->threshold = 0;
		spawnee->reactiontime = mlength;

		if (firsttime)
		{
			// This is the outermost link in the chain
			spawnee->flags2 |= MF2_AMBUSH;
			firsttime = false;
		}

		mlength--;
	}
}

// Function: A_SetFuse
//
// Description: Sets the actor's fuse timer if not set already. May also change state when fuse reaches the last tic, otherwise by default the actor will die or disappear. (Replaces A_SnowBall)
//
// var1 = fuse timer duration (in tics).
// var2:
//		lower 16 bits = if > 0, state to change to when fuse = 1
//		upper 16 bits: 0 = (default) don't set fuse unless 0, 1 = force change, 2 = force no change
//
void A_SetFuse(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetFuse", actor))
		return;
#endif

	if ((!actor->fuse || (locvar2 >> 16)) && (locvar2 >> 16) != 2) // set the actor's fuse value
		actor->fuse = locvar1;

	if (actor->fuse == 1 && (locvar2 & 65535)) // change state on the very last tic (fuse is handled before actions in P_MobjThinker)
	{
		actor->fuse = 0; // don't die/disappear the next tic!
		P_SetMobjState(actor, locvar2 & 65535);
	}
}

// Function: A_CrawlaCommanderThink
//
// Description: Thinker for Crawla Commander.
//
// var1 = shoot bullets?
// var2 = "pogo mode" speed
//
void A_CrawlaCommanderThink(mobj_t *actor)
{
	fixed_t dist;
	sector_t *nextsector;
	fixed_t thefloor;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CrawlaCommanderThink", actor))
		return;
#endif

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - FixedMul(256*FRACUNIT, actor->scale))
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->fuse & 1)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if (actor->reactiontime > 0)
		actor->reactiontime--;

	if (actor->fuse < 2)
	{
		actor->fuse = 0;
		actor->flags2 &= ~MF2_FRET;
	}

	// Hover mode
	if (actor->health > 1 || actor->fuse)
	{
		if (actor->z < thefloor + FixedMul(16*FRACUNIT, actor->scale))
			actor->momz += FixedMul(FRACUNIT, actor->scale);
		else if (actor->z < thefloor + FixedMul(32*FRACUNIT, actor->scale))
			actor->momz += FixedMul(FRACUNIT/2, actor->scale);
		else
			actor->momz += FixedMul(16, actor->scale);
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		if (actor->state != &states[actor->info->spawnstate])
			P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	dist = P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y);

	if (actor->target->player && actor->health > 1)
	{
		if (dist < FixedMul(128*FRACUNIT, actor->scale)
			&& ((actor->target->player->pflags & PF_JUMPED) || (actor->target->player->pflags & PF_SPINNING)))
		{
			// Auugh! He's trying to kill you! Strafe! STRAAAAFFEEE!!
			if (actor->target->momx || actor->target->momy)
				P_InstaThrust(actor, actor->angle - ANGLE_180, FixedMul(20*FRACUNIT, actor->scale));
			return;
		}
	}

	if (locvar1)
	{
		if (actor->health < 2 && P_RandomChance(FRACUNIT/128))
			P_SpawnMissile(actor, actor->target, locvar1);
	}

	// Face the player
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if (actor->threshold && dist > FixedMul(256*FRACUNIT, actor->scale))
		actor->momx = actor->momy = 0;

	if (actor->reactiontime && actor->reactiontime <= 2*TICRATE && dist > actor->target->radius - FixedMul(FRACUNIT, actor->scale))
	{
		actor->threshold = 0;

		// Roam around, somewhat in the player's direction.
		actor->angle += (P_RandomByte()<<10);
		actor->angle -= (P_RandomByte()<<10);

		if (actor->health > 1)
			P_InstaThrust(actor, actor->angle, FixedMul(10*FRACUNIT, actor->scale));
	}
	else if (!actor->reactiontime)
	{
		if (actor->health > 1) // Hover Mode
		{
			if (dist < FixedMul(512*FRACUNIT, actor->scale))
			{
				actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
				P_InstaThrust(actor, actor->angle, FixedMul(60*FRACUNIT, actor->scale));
				actor->threshold = 1;
			}
		}
		actor->reactiontime = 2*TICRATE + P_RandomByte()/2;
	}

	if (actor->health == 1)
		P_Thrust(actor, actor->angle, 1);

	// Pogo Mode
	if (!actor->fuse && actor->health == 1 && actor->z <= actor->floorz)
	{
		if (dist < FixedMul(256*FRACUNIT, actor->scale))
		{
			actor->momz = FixedMul(locvar2, actor->scale);
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
			P_InstaThrust(actor, actor->angle, FixedMul(locvar2/8, actor->scale));
			// pogo on player
		}
		else
		{
			UINT8 prandom = P_RandomByte();
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_RandomChance(FRACUNIT/2) ? -prandom : +prandom);
			P_InstaThrust(actor, actor->angle, FixedDiv(FixedMul(locvar2, actor->scale), 3*FRACUNIT/2));
			actor->momz = FixedMul(locvar2, actor->scale); // Bounce up in air
		}
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if (nextsector->floorheight > actor->z && nextsector->floorheight - actor->z < FixedMul(128*FRACUNIT, actor->scale))
		actor->momz += (nextsector->floorheight - actor->z) / 4;
}

// Function: A_RingExplode
//
// Description: An explosion ring exploding
//
// var1 = unused
// var2 = unused
//
void A_RingExplode(mobj_t *actor)
{
	mobj_t *mo2;
	thinker_t *th;
	angle_t d;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RingExplode", actor))
		return;
#endif

	for (d = 0; d < 16; d++)
		P_SpawnParaloop(actor->x, actor->y, actor->z + actor->height, FixedMul(actor->info->painchance, actor->scale), 16, MT_NIGHTSPARKLE, S_NULL, d*(ANGLE_22h), true);

	S_StartSound(actor, sfx_prloop);

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2 == actor) // Don't explode yourself! Endless loop!
			continue;

		if (P_AproxDistance(P_AproxDistance(mo2->x - actor->x, mo2->y - actor->y), mo2->z - actor->z) > FixedMul(actor->info->painchance, actor->scale))
			continue;

		if (mo2->flags & MF_SHOOTABLE)
		{
			actor->flags2 |= MF2_DEBRIS;
			P_DamageMobj(mo2, actor, actor->target, 1, 0);
			continue;
		}
	}
	return;
}

// Function: A_OldRingExplode
//
// Description: An explosion ring exploding, 1.09.4 style
//
// var1 = object # to explode as debris
// var2 = unused
//
void A_OldRingExplode(mobj_t *actor) {
	UINT8 i;
	mobj_t *mo;
	const fixed_t ns = FixedMul(20 * FRACUNIT, actor->scale);
	INT32 locvar1 = var1;
	//INT32 locvar2 = var2;
	boolean changecolor = (actor->target && actor->target->player);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_OldRingExplode", actor))
		return;
#endif

	for (i = 0; i < 32; i++)
	{
		const angle_t fa = (i*FINEANGLES/16) & FINEMASK;

		mo = P_SpawnMobj(actor->x, actor->y, actor->z, locvar1);
		P_SetTarget(&mo->target, actor->target); // Transfer target so player gets the points

		mo->momx = FixedMul(FINECOSINE(fa),ns);
		mo->momy = FixedMul(FINESINE(fa),ns);

		if (i > 15)
		{
			if (i & 1)
				mo->momz = ns;
			else
				mo->momz = -ns;
		}

		mo->flags2 |= MF2_DEBRIS;
		mo->fuse = TICRATE/5;

		if (changecolor)
		{
			if (gametype != GT_CTF)
				mo->color = actor->target->color; //copy color
			else if (actor->target->player->ctfteam == 2)
				mo->color = skincolor_bluering;
		}
	}

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, locvar1);

	P_SetTarget(&mo->target, actor->target);
	mo->momz = ns;
	mo->flags2 |= MF2_DEBRIS;
	mo->fuse = TICRATE/5;

	if (changecolor)
	{
		if (gametype != GT_CTF)
			mo->color = actor->target->color; //copy color
		else if (actor->target->player->ctfteam == 2)
			mo->color = skincolor_bluering;
	}

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, locvar1);

	P_SetTarget(&mo->target, actor->target);
	mo->momz = -ns;
	mo->flags2 |= MF2_DEBRIS;
	mo->fuse = TICRATE/5;

	if (changecolor)
	{
		if (gametype != GT_CTF)
			mo->color = actor->target->color; //copy color
		else if (actor->target->player->ctfteam == 2)
			mo->color = skincolor_bluering;
	}
}

// Function: A_MixUp
//
// Description: Mix up all of the player positions.
//
// var1 = unused
// var2 = unused
//
void A_MixUp(mobj_t *actor)
{
	boolean teleported[MAXPLAYERS];
	INT32 i, numplayers = 0, prandom = 0;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MixUp", actor))
		return;
#else
	(void)actor;
#endif

	if (!multiplayer)
		return;

	// No mix-up monitors in hide and seek or time only race.
	// The random factor is okay for other game modes, but in these, it is cripplingly unfair.
	if (gametype == GT_HIDEANDSEEK || gametype == GT_RACE)
	{
		S_StartSound(actor, sfx_lose);
		return;
	}

	numplayers = 0;
	memset(teleported, 0, sizeof (teleported));

	// Count the number of players in the game
	// and grab their xyz coords
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].mo && players[i].mo->health > 0 && players[i].playerstate == PST_LIVE
			&& !players[i].exiting && !players[i].powers[pw_super] && players[i].powers[pw_carry] != CR_NIGHTSMODE)
		{
			if ((netgame || multiplayer) && players[i].spectator) // Ignore spectators
				continue;

			numplayers++;
		}

	if (numplayers <= 1) // Not enough players to mix up.
	{
		S_StartSound(actor, sfx_lose);
		return;
	}
	else if (numplayers == 2) // Special case -- simple swap
	{
		fixed_t x, y, z;
		angle_t angle;
		INT32 one = -1, two = 0; // default value 0 to make the compiler shut up

		// Zoom tube stuff
		mobj_t *tempthing = NULL; //tracer
		UINT16 carry1,carry2;     //carry
		INT32 transspeed;         //player speed

		// Starpost stuff
		INT16 starpostx, starposty, starpostz;
		INT32 starpostnum;
		tic_t starposttime;
		angle_t starpostangle;

		INT32 mflags2;

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && players[i].mo->health > 0 && players[i].playerstate == PST_LIVE
				&& !players[i].exiting && !players[i].powers[pw_super])
			{
				if ((netgame || multiplayer) && players[i].spectator) // Ignore spectators
					continue;

				if (one == -1)
					one = i;
				else
				{
					two = i;
					break;
				}
			}

		//get this done first!
		tempthing = players[one].mo->tracer;
		P_SetTarget(&players[one].mo->tracer, players[two].mo->tracer);
		P_SetTarget(&players[two].mo->tracer, tempthing);

		//zoom tubes use player->speed to determine direction and speed
		transspeed = players[one].speed;
		players[one].speed = players[two].speed;
		players[two].speed = transspeed;

		//set flags variables now but DON'T set them.
		carry1 = (players[one].powers[pw_carry] == CR_PLAYER ? CR_NONE : players[one].powers[pw_carry]);
		carry2 = (players[two].powers[pw_carry] == CR_PLAYER ? CR_NONE : players[two].powers[pw_carry]);

		x = players[one].mo->x;
		y = players[one].mo->y;
		z = players[one].mo->z;
		angle = players[one].mo->angle;

		starpostx = players[one].starpostx;
		starposty = players[one].starposty;
		starpostz = players[one].starpostz;
		starpostangle = players[one].starpostangle;
		starpostnum = players[one].starpostnum;
		starposttime = players[one].starposttime;

		mflags2 = players[one].mo->flags2;

		P_MixUp(players[one].mo, players[two].mo->x, players[two].mo->y, players[two].mo->z, players[two].mo->angle,
				players[two].starpostx, players[two].starposty, players[two].starpostz,
				players[two].starpostnum, players[two].starposttime, players[two].starpostangle,
				players[two].mo->flags2);

		P_MixUp(players[two].mo, x, y, z, angle, starpostx, starposty, starpostz,
				starpostnum, starposttime, starpostangle,
				mflags2);

		//carry set after mixup.  Stupid P_ResetPlayer() takes away some of the stuff we look for...
		//but not all of it!  So we need to make sure they aren't set wrong or anything.
		players[one].powers[pw_carry] = carry2;
		players[two].powers[pw_carry] = carry1;

		teleported[one] = true;
		teleported[two] = true;
	}
	else
	{
		fixed_t position[MAXPLAYERS][3];
		angle_t anglepos[MAXPLAYERS];
		INT32 pindex[MAXPLAYERS], counter = 0, teleportfrom = 0;

		// Zoom tube stuff
		mobj_t *transtracer[MAXPLAYERS];  //tracer
		//pflags_t transflag[MAXPLAYERS]; //cyan pink white pink cyan
		UINT16 transcarry[MAXPLAYERS];    //player carry
		INT32 transspeed[MAXPLAYERS];     //player speed

		// Star post stuff
		INT16 spposition[MAXPLAYERS][3];
		INT32 starpostnum[MAXPLAYERS];
		tic_t starposttime[MAXPLAYERS];
		angle_t starpostangle[MAXPLAYERS];

		INT32 flags2[MAXPLAYERS];

		for (i = 0; i < MAXPLAYERS; i++)
		{
			position[i][0] = position[i][1] = position[i][2] = anglepos[i] = pindex[i] = -1;
			teleported[i] = false;
		}

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && players[i].mo->health > 0 && !players[i].exiting && !players[i].powers[pw_super] && players[i].powers[pw_carry] != CR_NIGHTSMODE)
			{
				if ((netgame || multiplayer) && players[i].spectator)// Ignore spectators
					continue;

				position[counter][0] = players[i].mo->x;
				position[counter][1] = players[i].mo->y;
				position[counter][2] = players[i].mo->z;
				pindex[counter] = i;
				anglepos[counter] = players[i].mo->angle;
				players[i].mo->momx = players[i].mo->momy = players[i].mo->momz =
					players[i].rmomx = players[i].rmomy = 1;
				players[i].cmomx = players[i].cmomy = 0;

				transcarry[counter] = (players[i].powers[pw_carry] == CR_PLAYER ? CR_NONE : players[i].powers[pw_carry]);
				transspeed[counter] = players[i].speed;
				transtracer[counter] = players[i].mo->tracer;

				spposition[counter][0] = players[i].starpostx;
				spposition[counter][1] = players[i].starposty;
				spposition[counter][2] = players[i].starpostz;
				starpostnum[counter] = players[i].starpostnum;
				starposttime[counter] = players[i].starposttime;
				starpostangle[counter] = players[i].starpostangle;

				flags2[counter] = players[i].mo->flags2;

				counter++;
			}
		}

		counter = 0;

		// Mix them up!
		for (;;)
		{
			if (counter > 255) // fail-safe to avoid endless loop
				break;
			prandom = P_RandomByte();
			prandom %= numplayers; // I love modular arithmetic, don't you?
			if (prandom) // Make sure it's not a useless mix
				break;
			counter++;
		}

		counter = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && players[i].mo->health > 0 && !players[i].exiting && !players[i].powers[pw_super] && players[i].powers[pw_carry] != CR_NIGHTSMODE)
			{
				if ((netgame || multiplayer) && players[i].spectator)// Ignore spectators
					continue;

				teleportfrom = (counter + prandom) % numplayers;

				//speed and tracer come before...
				players[i].speed = transspeed[teleportfrom];
				P_SetTarget(&players[i].mo->tracer, transtracer[teleportfrom]);

				P_MixUp(players[i].mo, position[teleportfrom][0], position[teleportfrom][1], position[teleportfrom][2], anglepos[teleportfrom],
					spposition[teleportfrom][0], spposition[teleportfrom][1], spposition[teleportfrom][2],
					starpostnum[teleportfrom], starposttime[teleportfrom], starpostangle[teleportfrom],
					flags2[teleportfrom]);

				//...carry after.  same reasoning.
				players[i].powers[pw_carry] = transcarry[teleportfrom];

				teleported[i] = true;
				counter++;
			}
		}
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (teleported[i])
		{
			if (playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && players[i].mo->health > 0 && !players[i].exiting && !players[i].powers[pw_super] && players[i].powers[pw_carry] != CR_NIGHTSMODE)
			{
				if ((netgame || multiplayer) && players[i].spectator)// Ignore spectators
					continue;

				P_SetThingPosition(players[i].mo);

#ifdef ESLOPE
				players[i].mo->floorz = P_GetFloorZ(players[i].mo, players[i].mo->subsector->sector, players[i].mo->x, players[i].mo->y, NULL);
				players[i].mo->ceilingz = P_GetCeilingZ(players[i].mo, players[i].mo->subsector->sector, players[i].mo->x, players[i].mo->y, NULL);
#else
				players[i].mo->floorz = players[i].mo->subsector->sector->floorheight;
				players[i].mo->ceilingz = players[i].mo->subsector->sector->ceilingheight;
#endif

				P_CheckPosition(players[i].mo, players[i].mo->x, players[i].mo->y);
			}
		}
	}

	// Play the 'bowrwoosh!' sound
	S_StartSound(NULL, sfx_mixup);
}

// Function: A_RecyclePowers
//
// Description: Take all player's powers, and swap 'em.
//
// var1 = unused
// var2 = unused
//
void A_RecyclePowers(mobj_t *actor)
{
	INT32 i, j, k, numplayers = 0;

#ifdef WEIGHTEDRECYCLER
	UINT8 beneficiary = 255;
#endif
	UINT8 playerslist[MAXPLAYERS];
	UINT8 postscramble[MAXPLAYERS];

	UINT16 powers[MAXPLAYERS][NUMPOWERS];
	INT32 weapons[MAXPLAYERS];
	INT32 weaponheld[MAXPLAYERS];

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RecyclePowers", actor))
		return;
#endif

#if !defined(WEIGHTEDRECYCLER) && !defined(HAVE_BLUA)
	// actor is used in all scenarios but this one, funny enough
	(void)actor;
#endif

	if (!multiplayer)
	{
		S_StartSound(actor, sfx_lose);
		return;
	}

	numplayers = 0;

	// Count the number of players in the game
	for (i = 0, j = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && players[i].mo && players[i].mo->health > 0 && players[i].playerstate == PST_LIVE
			&& !players[i].exiting && !((netgame || multiplayer) && players[i].spectator))
		{
#ifndef WEIGHTEDRECYCLER
			if (players[i].powers[pw_super])
				continue; // Ignore super players
#endif

			numplayers++;
			postscramble[j] = playerslist[j] = (UINT8)i;

#ifdef WEIGHTEDRECYCLER
			// The guy who started the recycle gets the best result
			if (actor && actor->target && actor->target->player && &players[i] == actor->target->player)
				beneficiary = (UINT8)i;
#endif

			// Save powers
			for (k = 0; k < NUMPOWERS; k++)
				powers[i][k] = players[i].powers[k];
			//1.1: ring weapons too
			weapons[i] = players[i].ringweapons;
			weaponheld[i] = players[i].currentweapon;

			j++;
		}
	}

	if (numplayers <= 1)
	{
		S_StartSound(actor, sfx_lose);
		return; //nobody to touch!
	}

	//shuffle the post scramble list, whee!
	// hardcoded 0-1 to 1-0 for two players
	if (numplayers == 2)
	{
		postscramble[0] = playerslist[1];
		postscramble[1] = playerslist[0];
	}
	else
	for (j = 0; j < numplayers; j++)
	{
		UINT8 tempint;

		i = j + ((P_RandomByte() + leveltime) % (numplayers - j));
		tempint = postscramble[j];
		postscramble[j] = postscramble[i];
		postscramble[i] = tempint;
	}

#ifdef WEIGHTEDRECYCLER
	//the joys of qsort...
	if (beneficiary != 255) {
		qsort(playerslist, numplayers, sizeof(UINT8), P_RecycleCompare);

		// now, make sure the benificiary is in the best slot
		// swap out whatever poor sap was going to get the best items
		for (i = 0; i < numplayers; i++)
		{
			if (postscramble[i] == beneficiary)
			{
				postscramble[i] = postscramble[0];
				postscramble[0] = beneficiary;
				break;
			}
		}
	}
#endif

	// now assign!
	for (i = 0; i < numplayers; i++)
	{
		UINT8 send_pl = playerslist[i];
		UINT8 recv_pl = postscramble[i];

		// debugF
		CONS_Debug(DBG_GAMELOGIC, "sending player %hu's items to %hu\n", (UINT16)send_pl, (UINT16)recv_pl);

		for (j = 0; j < NUMPOWERS; j++)
		{
			if (j == pw_flashing || j == pw_underwater || j == pw_spacetime || j == pw_carry
			    || j == pw_tailsfly || j == pw_extralife || j == pw_nocontrol || j == pw_super)
				continue;
			players[recv_pl].powers[j] = powers[send_pl][j];
		}

		//1.1: weapon rings too
		players[recv_pl].ringweapons = weapons[send_pl];
		players[recv_pl].currentweapon = weaponheld[send_pl];

		P_SpawnShieldOrb(&players[recv_pl]);
		if (P_IsLocalPlayer(&players[recv_pl]))
			P_RestoreMusic(&players[recv_pl]);
		P_FlashPal(&players[recv_pl], PAL_RECYCLE, 10);
	}

	S_StartSound(NULL, sfx_gravch); //heh, the sound effect I used is already in
}

// Function: A_Boss1Chase
//
// Description: Like A_Chase, but for Boss 1.
//
// var1 = unused
// var2 = unused
//
void A_Boss1Chase(mobj_t *actor)
{
	INT32 delta;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss1Chase", actor))
		return;
#endif

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjStateNF(actor, actor->info->spawnstate);
		return;
	}

	if (actor->reactiontime)
		actor->reactiontime--;

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	if (actor->movecount)
		goto nomissile;

	if (!P_CheckMissileRange(actor))
		goto nomissile;

	if (actor->reactiontime <= 0)
	{
		if (actor->health > actor->info->damage)
		{
			if (P_RandomChance(FRACUNIT/2))
				P_SetMobjState(actor, actor->info->missilestate);
			else
				P_SetMobjState(actor, actor->info->meleestate);
		}
		else
		{
			P_LinedefExecute(LE_PINCHPHASE, actor, NULL);
			P_SetMobjState(actor, actor->info->raisestate);
		}

		actor->flags2 |= MF2_JUSTATTACKED;
		actor->reactiontime = actor->info->reactiontime;
		return;
	}

	// ?
nomissile:
	// possibly choose another target
	if (multiplayer && P_RandomChance(FRACUNIT/128))
	{
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target
	}

	if (actor->flags & MF_FLOAT && !(actor->flags2 & MF2_SKULLFLY))
	{ // Float up/down to your target's position. Stay above them, but not out of jump range.
		fixed_t target_min = actor->target->floorz+FixedMul(64*FRACUNIT, actor->scale);
		if (target_min < actor->target->z - actor->height)
			target_min = actor->target->z - actor->height;
		if (target_min < actor->floorz+FixedMul(33*FRACUNIT, actor->scale))
			target_min = actor->floorz+FixedMul(33*FRACUNIT, actor->scale);
		if (actor->z > target_min+FixedMul(16*FRACUNIT, actor->scale))
			actor->momz = FixedMul((-actor->info->speed<<(FRACBITS-1)), actor->scale);
		else if (actor->z < target_min)
			actor->momz = FixedMul(actor->info->speed<<(FRACBITS-1), actor->scale);
		else
			actor->momz = FixedMul(actor->momz,7*FRACUNIT/8);
	}

	// chase towards player
	if (P_AproxDistance(actor->target->x-actor->x, actor->target->y-actor->y) > actor->radius+actor->target->radius)
	{
		if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
			P_NewChaseDir(actor);
	}
	// too close, don't want to chase.
	else if (--actor->movecount < 0)
	{
		// A mini-A_FaceTarget based on P_NewChaseDir.
		// Yes, it really is this simple when you get down to it.
		fixed_t deltax, deltay;

		deltax = actor->target->x - actor->x;
		deltay = actor->target->y - actor->y;

		actor->movedir = diags[((deltay < 0)<<1) + (deltax > 0)];
		actor->movecount = P_RandomByte() & 15;
	}
}

// Function: A_Boss2Chase
//
// Description: Really doesn't 'chase', but rather goes in a circle.
//
// var1 = unused
// var2 = unused
//
void A_Boss2Chase(mobj_t *actor)
{
	fixed_t radius;
	boolean reverse = false;
	INT32 speedvar;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss2Chase", actor))
		return;
#endif

	if (actor->health <= 0)
		return;

	// Startup randomness
	if (actor->reactiontime <= -666)
		actor->reactiontime = 2*TICRATE + P_RandomByte();

	// When reactiontime hits zero, he will go the other way
	if (--actor->reactiontime <= 0)
	{
		reverse = true;
		actor->reactiontime = 2*TICRATE + P_RandomByte();
	}

	P_SetTarget(&actor->target, P_GetClosestAxis(actor));

	if (!actor->target) // This should NEVER happen.
	{
		CONS_Debug(DBG_GAMELOGIC, "Boss2 has no target!\n");
		A_BossDeath(actor);
		return;
	}

	radius = actor->target->radius;

	if (reverse)
	{
		actor->watertop = -actor->watertop;
		actor->extravalue1 = 18;
		if (actor->flags2 & MF2_AMBUSH)
			actor->extravalue1 -= (actor->info->spawnhealth - actor->health)*2;
		actor->extravalue2 = actor->extravalue1;
	}

	// Turnaround
	if (actor->extravalue1 > 0)
	{
		--actor->extravalue1;

		// Set base angle
		{
			const angle_t fa = (actor->target->angle + FixedAngle(actor->watertop))>>ANGLETOFINESHIFT;
			const fixed_t fc = FixedMul(FINECOSINE(fa),radius);
			const fixed_t fs = FixedMul(FINESINE(fa),radius);
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x + fc, actor->target->y + fs);
		}

		// Now turn you around!
		// Note that the start position is the final position, we move it back around
		// to intermediary positions...
		actor->angle -= FixedAngle(FixedMul(FixedDiv(180<<FRACBITS, actor->extravalue2<<FRACBITS), actor->extravalue1<<FRACBITS));
	}
	else
	{
		// Only speed up if you have the 'Deaf' flag.
		if (actor->flags2 & MF2_AMBUSH)
			speedvar = actor->health;
		else
			speedvar = actor->info->spawnhealth;

		actor->target->angle += // Don't use FixedAngleC!
			FixedAngle(FixedDiv(FixedMul(actor->watertop, (actor->info->spawnhealth*(FRACUNIT/4)*3)), speedvar*FRACUNIT));

		P_UnsetThingPosition(actor);
		{
			const angle_t fa = actor->target->angle>>ANGLETOFINESHIFT;
			const fixed_t fc = FixedMul(FINECOSINE(fa),radius);
			const fixed_t fs = FixedMul(FINESINE(fa),radius);
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x + fc, actor->target->y + fs);
			actor->x = actor->target->x + fc;
			actor->y = actor->target->y + fs;
		}
		P_SetThingPosition(actor);

		// Spray goo once every second
		if (leveltime % (speedvar*15/10)-1 == 0)
		{
			const fixed_t ns = FixedMul(3 * FRACUNIT, actor->scale);
			mobj_t *goop;
			fixed_t fz = actor->z+actor->height+FixedMul(24*FRACUNIT, actor->scale);
			angle_t fa;
			// actor->movedir is used to determine the last
			// direction goo was sprayed in. There are 8 possible
			// directions to spray. (45-degree increments)

			actor->movedir++;
			actor->movedir %= NUMDIRS;
			fa = (actor->movedir*FINEANGLES/8) & FINEMASK;

			goop = P_SpawnMobj(actor->x, actor->y, fz, actor->info->painchance);
			goop->momx = FixedMul(FINECOSINE(fa),ns);
			goop->momy = FixedMul(FINESINE(fa),ns);
			goop->momz = FixedMul(4*FRACUNIT, actor->scale);
			goop->fuse = 10*TICRATE;

			if (actor->info->attacksound)
				S_StartAttackSound(actor, actor->info->attacksound);

			if (P_RandomChance(FRACUNIT/2))
			{
				goop->momx *= 2;
				goop->momy *= 2;
			}
			else if (P_RandomChance(129*FRACUNIT/256))
			{
				goop->momx *= 3;
				goop->momy *= 3;
			}

			actor->flags2 |= MF2_JUSTATTACKED;
		}
	}
}

// Function: A_Boss2Pogo
//
// Description: Pogo part of Boss 2 AI.
//
// var1 = unused
// var2 = unused
//
void A_Boss2Pogo(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss2Pogo", actor))
		return;
#endif
	if (actor->z <= actor->floorz + FixedMul(8*FRACUNIT, actor->scale) && actor->momz <= 0)
	{
		if (actor->state != &states[actor->info->raisestate])
			P_SetMobjState(actor, actor->info->raisestate);
		// Pogo Mode
	}
	else if (actor->momz < 0 && actor->reactiontime)
	{
		const fixed_t ns = FixedMul(3 * FRACUNIT, actor->scale);
		mobj_t *goop;
		fixed_t fz = actor->z+actor->height+FixedMul(24*FRACUNIT, actor->scale);
		angle_t fa;
		INT32 i;
		// spray in all 8 directions!
		for (i = 0; i < 8; i++)
		{
			actor->movedir++;
			actor->movedir %= NUMDIRS;
			fa = (actor->movedir*FINEANGLES/8) & FINEMASK;

			goop = P_SpawnMobj(actor->x, actor->y, fz, actor->info->painchance);
			goop->momx = FixedMul(FINECOSINE(fa),ns);
			goop->momy = FixedMul(FINESINE(fa),ns);
			goop->momz = FixedMul(4*FRACUNIT, actor->scale);

			goop->fuse = 10*TICRATE;
		}
		actor->reactiontime = 0; // we already shot goop, so don't do it again!
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);
		actor->flags2 |= MF2_JUSTATTACKED;
	}
}

// Function: A_Boss2TakeDamage
//
// Description: Special function for Boss 2 so you can't just sit and destroy him.
//
// var1 = Invincibility duration
// var2 = unused
//
void A_Boss2TakeDamage(mobj_t *actor)
{
	INT32 locvar1 = var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss2TakeDamage", actor))
		return;
#endif
	A_Pain(actor);
	actor->reactiontime = 1; // turn around
	if (locvar1 == 0) // old A_Invincibilerize behavior
		actor->movecount = TICRATE;
	else
		actor->movecount = locvar1; // become flashing invulnerable for this long.
}

// Function: A_Boss7Chase
//
// Description: Like A_Chase, but for Black Eggman
//
// var1 = unused
// var2 = unused
//
void A_Boss7Chase(mobj_t *actor)
{
	INT32 delta;
	INT32 i;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss7Chase", actor))
		return;
#endif

	if (actor->z != actor->floorz)
		return;

	// Self-adjust if stuck on the edge
	if (actor->tracer)
	{
		if (P_AproxDistance(actor->x - actor->tracer->x, actor->y - actor->tracer->y) > 128*FRACUNIT - actor->radius)
			P_InstaThrust(actor, R_PointToAngle2(actor->x, actor->y, actor->tracer->x, actor->tracer->y), FRACUNIT);
	}

	if (actor->flags2 & MF2_FRET)
	{
		P_SetMobjState(actor, S_BLACKEGG_DESTROYPLAT1);
		S_StartSound(0, sfx_s3k53);
		actor->flags2 &= ~MF2_FRET;
		return;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	// Is a player on top of us?
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (!players[i].mo)
			continue;

		if (players[i].mo->health <= 0)
			continue;

		if (P_AproxDistance(players[i].mo->x - actor->x, players[i].mo->y - actor->y) > actor->radius)
			continue;

		if (players[i].mo->z > actor->z + actor->height - 2*FRACUNIT
			&& players[i].mo->z < actor->z + actor->height + 32*FRACUNIT)
		{
			// Punch him!
			P_SetMobjState(actor, actor->info->meleestate);
			S_StartSound(0, sfx_begrnd); // warning sound
			return;
		}
	}

	if (actor->health <= actor->info->damage
		&& actor->target
		&& actor->target->player
		&& (actor->target->player->powers[pw_carry] == CR_GENERIC))
	{
		A_FaceTarget(actor);
		P_SetMobjState(actor, S_BLACKEGG_SHOOT1);
		actor->movecount = TICRATE + P_RandomByte()/2;
		return;
	}

	if (actor->reactiontime)
		actor->reactiontime--;

	if (actor->reactiontime <= 0 && actor->z == actor->floorz)
	{
		// Here, we'll call P_RandomByte() and decide what kind of attack to do
		switch(actor->threshold)
		{
			case 0: // Lob cannon balls
				if (actor->z < 1056*FRACUNIT)
				{
					A_FaceTarget(actor);
					P_SetMobjState(actor, actor->info->xdeathstate);
					actor->movecount = 7*TICRATE + P_RandomByte();
					break;
				}
				actor->threshold++;
				/* FALLTHRU */
			case 1: // Chaingun Goop
				A_FaceTarget(actor);
				P_SetMobjState(actor, S_BLACKEGG_SHOOT1);

				if (actor->health > actor->info->damage)
					actor->movecount = TICRATE + P_RandomByte()/3;
				else
					actor->movecount = TICRATE + P_RandomByte()/2;
				break;
			case 2: // Homing Missile
				A_FaceTarget(actor);
				P_SetMobjState(actor, actor->info->missilestate);
				S_StartSound(0, sfx_beflap);
				break;
		}

		actor->threshold++;
		actor->threshold %= 3;
		return;
	}

	// possibly choose another target
	if (multiplayer && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_BossTargetPlayer(actor, false))
		return; // got a new target

	if (leveltime & 1)
	{
		// chase towards player
		if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
			P_NewChaseDir(actor);
	}
}

// Function: A_GoopSplat
//
// Description: Black Eggman goop hits a target and sticks around for awhile.
//
// var1 = unused
// var2 = unused
//
void A_GoopSplat(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GoopSplat", actor))
		return;
#endif
	P_UnsetThingPosition(actor);
	if (sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}
	actor->flags = MF_SPECIAL; // Not a typo
	P_SetThingPosition(actor);
}

// Function: A_Boss2PogoSFX
//
// Description: Pogoing for Boss 2
//
// var1 = pogo jump strength
// var2 = idle pogo speed
//
void A_Boss2PogoSFX(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss2PogoSFX", actor))
		return;
#endif

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		return;
	}

	// Boing!
	if (P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) < FixedMul(256*FRACUNIT, actor->scale))
	{
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
		P_InstaThrust(actor, actor->angle, FixedMul(actor->info->speed, actor->scale));
		// pogo on player
	}
	else
	{
		UINT8 prandom = P_RandomByte();
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_RandomChance(FRACUNIT/2) ? -prandom : +prandom);
		P_InstaThrust(actor, actor->angle, FixedMul(FixedMul(actor->info->speed,(locvar2)), actor->scale));
	}
	if (actor->info->activesound) S_StartSound(actor, actor->info->activesound);
	actor->momz = FixedMul(locvar1, actor->scale); // Bounce up in air
	actor->reactiontime = 1;
}

// Function: A_Boss2PogoTarget
//
// Description: Pogoing for Boss 2, tries to actually land on the player directly.
//
// var1 = pogo jump strength
// var2 = idle pogo speed
//
void A_Boss2PogoTarget(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss2PogoTarget", actor))
		return;
#endif

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE) || (actor->target->player && actor->target->player->powers[pw_flashing])
	|| P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) >= FixedMul(512*FRACUNIT, actor->scale))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 512*FRACUNIT))
			; // got a new target
		else if (P_LookForPlayers(actor, true, false, 0))
			; // got a new target
		else
			return;
	}

	// Target hit, retreat!
	if (actor->target->player->powers[pw_flashing] > TICRATE || actor->flags2 & MF2_FRET)
	{
		UINT8 prandom = P_RandomByte();
		actor->z++; // unstick from the floor
		actor->momz = FixedMul(locvar1, actor->scale); // Bounce up in air
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_RandomChance(FRACUNIT/2) ? -prandom : +prandom); // Pick a direction, and randomize it.
		P_InstaThrust(actor, actor->angle+ANGLE_180, FixedMul(FixedMul(actor->info->speed,(locvar2)), actor->scale)); // Move at wandering speed
	}
	// Try to land on top of the player.
	else if (P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) < FixedMul(512*FRACUNIT, actor->scale))
	{
		fixed_t airtime, gravityadd, zoffs;

		// check gravity in the sector (for later math)
		P_CheckGravity(actor, true);
		gravityadd = actor->momz;

		actor->z++; // unstick from the floor
		actor->momz = FixedMul(locvar1 + (locvar1>>2), actor->scale); // Bounce up in air

		/*badmath = 0;
		airtime = 0;
		do {
			badmath += momz;
			momz += gravityadd;
			airtime++;
		} while(badmath > 0);
		airtime = 2*airtime<<FRACBITS;
		*/

		// Remember, kids!
		// Reduced down Calculus lets you avoid bad 'logic math' loops!
		//airtime = FixedDiv(-actor->momz<<1, gravityadd)<<1; // going from 0 to 0 is much simpler
		zoffs = (P_GetPlayerHeight(actor->target->player)>>1) + (actor->target->floorz - actor->floorz); // offset by the difference in floor height plus half the player height,
		airtime = FixedDiv((-actor->momz - FixedSqrt(FixedMul(actor->momz,actor->momz)+zoffs)), gravityadd)<<1; // to try and land on their head rather than on their feet

		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
		P_InstaThrust(actor, actor->angle, FixedDiv(P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y), airtime));
	}
	// Wander semi-randomly towards the player to get closer.
	else
	{
		UINT8 prandom = P_RandomByte();
		actor->z++; // unstick from the floor
		actor->momz = FixedMul(locvar1, actor->scale); // Bounce up in air
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_RandomChance(FRACUNIT/2) ? -prandom : +prandom); // Pick a direction, and randomize it.
		P_InstaThrust(actor, actor->angle, FixedMul(FixedMul(actor->info->speed,(locvar2)), actor->scale)); // Move at wandering speed
	}
	// Boing!
	if (actor->info->activesound) S_StartSound(actor, actor->info->activesound);

	if (actor->info->missilestate) // spawn the pogo stick collision box
	{
		mobj_t *pogo = P_SpawnMobj(actor->x, actor->y, actor->z - mobjinfo[actor->info->missilestate].height, (mobjtype_t)actor->info->missilestate);
		pogo->target = actor;
	}

	actor->reactiontime = 1;
}

// Function: A_EggmanBox
//
// Description: Harms the player
//
// var1 = unused
// var2 = unused
//
void A_EggmanBox(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_EggmanBox", actor))
		return;
#endif
	if (!actor->target || !actor->target->player)
	{
		CONS_Debug(DBG_GAMELOGIC, "Powerup has no target.\n");
		return;
	}

	P_DamageMobj(actor->target, actor, actor, 1, 0); // Ow!
}

// Function: A_TurretFire
//
// Description: Initiates turret fire.
//
// var1 = object # to repeatedly fire
// var2 = distance threshold
//
void A_TurretFire(mobj_t *actor)
{
	INT32 count = 0;
	fixed_t dist;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_TurretFire", actor))
		return;
#endif

	if (locvar2)
		dist = FixedMul(locvar2*FRACUNIT, actor->scale);
	else
		dist = FixedMul(2048*FRACUNIT, actor->scale);

	if (!locvar1)
		locvar1 = MT_TURRETLASER;

	while (P_SupermanLook4Players(actor) && count < MAXPLAYERS)
	{
		if (P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) < dist)
		{
			actor->flags2 |= MF2_FIRING;
			actor->extravalue1 = locvar1;
			break;
		}

		count++;
	}
}

// Function: A_SuperTurretFire
//
// Description: Initiates turret fire that even stops Super Sonic.
//
// var1 = object # to repeatedly fire
// var2 = distance threshold
//
void A_SuperTurretFire(mobj_t *actor)
{
	INT32 count = 0;
	fixed_t dist;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SuperTurretFire", actor))
		return;
#endif

	if (locvar2)
		dist = FixedMul(locvar2*FRACUNIT, actor->scale);
	else
		dist = FixedMul(2048*FRACUNIT, actor->scale);

	if (!locvar1)
		locvar1 = MT_TURRETLASER;

	while (P_SupermanLook4Players(actor) && count < MAXPLAYERS)
	{
		if (P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) < dist)
		{
			actor->flags2 |= MF2_FIRING;
			actor->flags2 |= MF2_SUPERFIRE;
			actor->extravalue1 = locvar1;
			break;
		}

		count++;
	}
}

// Function: A_TurretStop
//
// Description: Stops the turret fire.
//
// var1 = Don't play activesound?
// var2 = unused
//
void A_TurretStop(mobj_t *actor)
{
	INT32 locvar1 = var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_TurretStop", actor))
		return;
#endif

	actor->flags2 &= ~MF2_FIRING;
	actor->flags2 &= ~MF2_SUPERFIRE;

	if (actor->target && actor->info->activesound && !locvar1)
		S_StartSound(actor, actor->info->activesound);
}

// Function: A_SparkFollow
//
// Description: Used by the hyper sparks to rotate around their target.
//
// var1 = unused
// var2 = unused
//
void A_SparkFollow(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SparkFollow", actor))
		return;
#endif

	if ((!actor->target || (actor->target->health <= 0))
		|| (actor->target->player && !actor->target->player->powers[pw_super]))
	{
		P_RemoveMobj(actor);
		return;
	}

	actor->angle += FixedAngle(actor->info->damage*FRACUNIT);
	P_UnsetThingPosition(actor);
	{
		const angle_t fa = actor->angle>>ANGLETOFINESHIFT;
		actor->x = actor->target->x + FixedMul(FINECOSINE(fa),FixedMul(actor->info->speed, actor->scale));
		actor->y = actor->target->y + FixedMul(FINESINE(fa),FixedMul(actor->info->speed, actor->scale));
		if (actor->target->eflags & MFE_VERTICALFLIP)
			actor->z = actor->target->z + actor->target->height - FixedDiv(actor->target->height,3*FRACUNIT);
		else
			actor->z = actor->target->z + FixedDiv(actor->target->height,3*FRACUNIT) - actor->height;
	}
	P_SetThingPosition(actor);
}

// Function: A_BuzzFly
//
// Description: Makes an object slowly fly after a player, in the manner of a Buzz.
//
// var1 = sfx to play
// var2 = length of sfx, set to threshold if played
//
void A_BuzzFly(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BuzzFly", actor))
		return;
#endif
	if (actor->flags2 & MF2_AMBUSH)
		return;

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		actor->momz = actor->momy = actor->momx = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// turn towards movement direction if not there yet
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if (actor->target->health <= 0 || (!actor->threshold && !P_CheckSight(actor, actor->target)))
	{
		if ((multiplayer || netgame) && P_LookForPlayers(actor, true, false, FixedMul(3072*FRACUNIT, actor->scale)))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate); // Go back to looking around
		return;
	}

	// If the player is over 3072 fracunits away, then look for another player
	if (P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y),
		actor->target->z - actor->z) > FixedMul(3072*FRACUNIT, actor->scale))
	{
		if (multiplayer || netgame)
			P_LookForPlayers(actor, true, false, FixedMul(3072*FRACUNIT, actor->scale)); // maybe get a new target

		return;
	}

	// chase towards player
	{
		INT32 dist, realspeed;
		const fixed_t mf = 5*(FRACUNIT/4);

		if (ultimatemode)
			realspeed = FixedMul(FixedMul(actor->info->speed,mf), actor->scale);
		else
			realspeed = FixedMul(actor->info->speed, actor->scale);

		dist = P_AproxDistance(P_AproxDistance(actor->target->x - actor->x,
			actor->target->y - actor->y), actor->target->z - actor->z);

		if (dist < 1)
			dist = 1;

		actor->momx = FixedMul(FixedDiv(actor->target->x - actor->x, dist), realspeed);
		actor->momy = FixedMul(FixedDiv(actor->target->y - actor->y, dist), realspeed);
		actor->momz = FixedMul(FixedDiv(actor->target->z - actor->z, dist), realspeed);

		if (actor->z+actor->momz >= actor->waterbottom && actor->watertop > actor->floorz
			&& actor->z+actor->momz > actor->watertop - FixedMul(256*FRACUNIT, actor->scale)
			&& actor->z+actor->momz <= actor->watertop)
		{
			actor->momz = 0;
			actor->z = actor->watertop;
		}
	}

	if (locvar1 != sfx_None && !actor->threshold)
	{
		S_StartSound(actor, locvar1);
		actor->threshold = locvar2;
	}
}

// Function: A_GuardChase
//
// Description: Modified A_Chase for Egg Guard
//
// var1 = unused
// var2 = unused
//
void A_GuardChase(mobj_t *actor)
{
	INT32 delta;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GuardChase", actor))
		return;
#endif

	if (actor->reactiontime)
		actor->reactiontime--;

	if ((!actor->tracer || !actor->tracer->health) && actor->threshold != 42)
	{
		P_SetTarget(&actor->tracer, NULL);
		actor->threshold = 42;
		P_SetMobjState(actor, actor->info->painstate);
		actor->flags |= MF_SPECIAL|MF_SHOOTABLE;
		return;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjStateNF(actor, actor->info->spawnstate);
		return;
	}

	// possibly choose another target
	if (multiplayer && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false, 0))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor, (actor->flags2 & MF2_AMBUSH) ? actor->info->speed * 2 : actor->info->speed))
	{
		P_NewChaseDir(actor);
		actor->movecount += 5; // Increase tics before change in direction allowed.
	}

	// Now that we've moved, its time for our shield to move!
	// Otherwise it'll never act as a proper overlay.
	if (actor->tracer && actor->tracer->state
	&& actor->tracer->state->action.acp1)
	{
		var1 = actor->tracer->state->var1, var2 = actor->tracer->state->var2;
		actor->tracer->state->action.acp1(actor->tracer);
	}
}

// Function: A_EggShield
//
// Description: Modified A_Chase for Egg Guard's shield
//
// var1 = unused
// var2 = unused
//
void A_EggShield(mobj_t *actor)
{
	INT32 i;
	player_t *player;
	fixed_t blockdist;
	fixed_t newx, newy;
	fixed_t movex, movey;
	angle_t angle;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_EggShield", actor))
		return;
#endif

	if (!actor->target || !actor->target->health)
	{
		P_RemoveMobj(actor);
		return;
	}

	newx = actor->target->x + P_ReturnThrustX(actor, actor->target->angle, FixedMul(FRACUNIT, actor->scale));
	newy = actor->target->y + P_ReturnThrustY(actor, actor->target->angle, FixedMul(FRACUNIT, actor->scale));

	movex = newx - actor->x;
	movey = newy - actor->y;

	actor->angle = actor->target->angle;
	if (actor->target->eflags & MFE_VERTICALFLIP)
	{
		actor->eflags |= MFE_VERTICALFLIP;
		actor->z = actor->target->z + actor->target->height - actor->height;
	}
	else
		actor->z = actor->target->z;

	actor->destscale = actor->target->destscale;
	P_SetScale(actor, actor->target->scale);

	actor->floorz = actor->target->floorz;
	actor->ceilingz = actor->target->ceilingz;

	if (!movex && !movey)
		return;

	P_UnsetThingPosition(actor);
	actor->x = newx;
	actor->y = newy;
	P_SetThingPosition(actor);

	// Search for players to push
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		player = &players[i];

		if (!player->mo)
			continue;

		if (player->mo->z > actor->z + actor->height)
			continue;

		if (player->mo->z + player->mo->height < actor->z)
			continue;

		blockdist = actor->radius + player->mo->radius;

		if (abs(actor->x - player->mo->x) >= blockdist || abs(actor->y - player->mo->y) >= blockdist)
			continue; // didn't hit it

		angle = R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y) - actor->angle;

		if (angle > ANGLE_90 && angle < ANGLE_270)
			continue;

		// Blocked by the shield
		player->mo->momx += movex;
		player->mo->momy += movey;
		return;
	}
}


// Function: A_SetReactionTime
//
// Description: Sets the object's reaction time.
//
// var1 = 1 (use value in var2); 0 (use info table value)
// var2 = if var1 = 1, then value to set
//
void A_SetReactionTime(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetReactionTime", actor))
		return;
#endif
	if (var1)
		actor->reactiontime = var2;
	else
		actor->reactiontime = actor->info->reactiontime;
}

// Function: A_Boss1Spikeballs
//
// Description: Boss 1 spikeball spawning loop.
//
// var1 = ball number
// var2 = total balls
//
void A_Boss1Spikeballs(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *ball;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss1Spikeballs", actor))
		return;
#endif

	ball = P_SpawnMobj(actor->x, actor->y, actor->z, MT_EGGMOBILE_BALL);
	P_SetTarget(&ball->target, actor);
	ball->movedir = FixedAngle(FixedMul(FixedDiv(locvar1<<FRACBITS, locvar2<<FRACBITS), 360<<FRACBITS));
	ball->threshold = ball->radius + actor->radius + ball->info->painchance;

	S_StartSound(ball, ball->info->seesound);
	var1 = ball->state->var1, var2 = ball->state->var2;
	ball->state->action.acp1(ball);
}

// Function: A_Boss3TakeDamage
//
// Description: Called when Boss 3 takes damage.
//
// var1 = movecount value
// var2 = unused
//
void A_Boss3TakeDamage(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss3TakeDamage", actor))
		return;
#endif
	actor->movecount = var1;

	if (actor->target && actor->target->spawnpoint)
		actor->threshold = actor->target->spawnpoint->extrainfo;
}

// Function: A_Boss3Path
//
// Description: Does pathfinding along Boss 3's nodes.
//
// var1 = unused
// var2 = unused
//
void A_Boss3Path(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Boss3Path", actor))
		return;
#endif

	if (actor->tracer && actor->tracer->health && actor->tracer->movecount)
		actor->movecount |= 1;
	else if (actor->movecount & 1)
		actor->movecount = 0;

	if (actor->movecount & 2) // We've reached a firing point?
	{
		// Wait here and pretend to be angry or something.
		actor->momx = 0;
		actor->momy = 0;
		actor->momz = 0;
		P_SetTarget(&actor->target, actor->tracer->target);
		var1 = 0, var2 = 0;
		A_FaceTarget(actor);
		if (actor->tracer->state == &states[actor->tracer->info->missilestate])
			P_SetMobjState(actor, actor->info->missilestate);
		return;
	}
	else if (actor->threshold >= 0) // Traveling mode
	{
		thinker_t *th;
		mobj_t *mo2;
		fixed_t dist, dist2;
		fixed_t speed;

		P_SetTarget(&actor->target, NULL);

		// scan the thinkers
		// to find a point that matches
		// the number
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;
			if (mo2->type == MT_BOSS3WAYPOINT && mo2->spawnpoint && mo2->spawnpoint->angle == actor->threshold)
			{
				P_SetTarget(&actor->target, mo2);
				break;
			}
		}

		if (!actor->target) // Should NEVER happen
		{
			CONS_Debug(DBG_GAMELOGIC, "Error: Boss 3 Dummy was unable to find specified waypoint: %d\n", actor->threshold);
			return;
		}

		dist = P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y), actor->target->z - actor->z);

		if (dist < 1)
			dist = 1;

		if (actor->tracer && ((actor->tracer->movedir)
		|| (actor->tracer->health <= actor->tracer->info->damage)))
			speed = actor->info->speed * 2;
		else
			speed = actor->info->speed;

		actor->momx = FixedMul(FixedDiv(actor->target->x - actor->x, dist), speed);
		actor->momy = FixedMul(FixedDiv(actor->target->y - actor->y, dist), speed);
		actor->momz = FixedMul(FixedDiv(actor->target->z - actor->z, dist), speed);

		if (actor->momx != 0 || actor->momy != 0)
			actor->angle = R_PointToAngle2(0, 0, actor->momx, actor->momy);

		dist2 = P_AproxDistance(P_AproxDistance(actor->target->x - (actor->x + actor->momx), actor->target->y - (actor->y + actor->momy)), actor->target->z - (actor->z + actor->momz));

		if (dist2 < 1)
			dist2 = 1;

		if ((dist >> FRACBITS) <= (dist2 >> FRACBITS))
		{
			// If further away, set XYZ of mobj to waypoint location
			P_UnsetThingPosition(actor);
			actor->x = actor->target->x;
			actor->y = actor->target->y;
			actor->z = actor->target->z;
			actor->momx = actor->momy = actor->momz = 0;
			P_SetThingPosition(actor);

			if (actor->threshold == 0)
			{
				P_RemoveMobj(actor); // Cycle completed. Dummy removed.
				return;
			}

			// Set to next waypoint in sequence
			if (actor->target->spawnpoint)
			{
				// From the center point, choose one of the five paths
				if (actor->target->spawnpoint->angle == 0)
				{
					P_RemoveMobj(actor); // Cycle completed. Dummy removed.
					return;
				}
				else
					actor->threshold = actor->target->spawnpoint->extrainfo;

				// If the deaf flag is set, go into firing mode
				if (actor->target->spawnpoint->options & MTF_AMBUSH)
					actor->movecount |= 2;
			}
			else // This should never happen, as well
				CONS_Debug(DBG_GAMELOGIC, "Error: Boss 3 Dummy waypoint has no spawnpoint associated with it.\n");
		}
	}
}

// Function: A_LinedefExecute
//
// Description: Object's location is used to set the calling sector. The tag used is var1. Optionally, if var2 is set, the actor's angle (multiplied by var2) is added to the tag number as well.
//
// var1 = tag
// var2 = add angle to tag (optional)
//
void A_LinedefExecute(mobj_t *actor)
{
	INT32 tagnum;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_LinedefExecute", actor))
		return;
#endif

	tagnum = locvar1;
	// state numbers option is no more, custom states cannot be guaranteed to stay the same number anymore, now that they can be defined by names instead

	if (locvar2)
		tagnum += locvar2*(AngleFixed(actor->angle)>>FRACBITS);

	CONS_Debug(DBG_GAMELOGIC, "A_LinedefExecute: Running mobjtype %d's sector with tag %d\n", actor->type, tagnum);

	// tag 32768 displayed in map editors is actually tag -32768, tag 32769 is -32767, 65535 is -1 etc.
	P_LinedefExecute((INT16)tagnum, actor, actor->subsector->sector);
}

// Function: A_PlaySeeSound
//
// Description: Plays the object's seesound.
//
// var1 = unused
// var2 = unused
//
void A_PlaySeeSound(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_PlaySeeSound", actor))
		return;
#endif
	if (actor->info->seesound)
		S_StartScreamSound(actor, actor->info->seesound);
}

// Function: A_PlayAttackSound
//
// Description: Plays the object's attacksound.
//
// var1 = unused
// var2 = unused
//
void A_PlayAttackSound(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_PlayAttackSound", actor))
		return;
#endif
	if (actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
}

// Function: A_PlayActiveSound
//
// Description: Plays the object's activesound.
//
// var1 = unused
// var2 = unused
//
void A_PlayActiveSound(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_PlayActiveSound", actor))
		return;
#endif
	if (actor->info->activesound)
		S_StartSound(actor, actor->info->activesound);
}

// Function: A_SmokeTrailer
//
// Description: Adds smoke trails to an object.
//
// var1 = object # to spawn as smoke
// var2 = unused
//
void A_SmokeTrailer(mobj_t *actor)
{
	mobj_t *th;
	INT32 locvar1 = var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SmokeTrailer", actor))
		return;
#endif

	if (leveltime % 4)
		return;

	// add the smoke behind the rocket
	if (actor->eflags & MFE_VERTICALFLIP)
	{
		th = P_SpawnMobj(actor->x-actor->momx, actor->y-actor->momy, actor->z + actor->height - FixedMul(mobjinfo[locvar1].height, actor->scale), locvar1);
		th->flags2 |= MF2_OBJECTFLIP;
	}
	else
		th = P_SpawnMobj(actor->x-actor->momx, actor->y-actor->momy, actor->z, locvar1);
	P_SetObjectMomZ(th, FRACUNIT, false);
	th->destscale = actor->scale;
	P_SetScale(th, actor->scale);
	th->tics -= P_RandomByte() & 3;
	if (th->tics < 1)
		th->tics = 1;
}

// Function: A_SpawnObjectAbsolute
//
// Description: Spawns an object at an absolute position
//
// var1:
//		var1 >> 16 = x
//		var1 & 65535 = y
// var2:
//		var2 >> 16 = z
//		var2 & 65535 = type
//
void A_SpawnObjectAbsolute(mobj_t *actor)
{
	INT16 x, y, z; // Want to be sure we can use negative values
	mobjtype_t type;
	mobj_t *mo;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SpawnObjectAbsolute", actor))
		return;
#endif

	x = (INT16)(locvar1>>16);
	y = (INT16)(locvar1&65535);
	z = (INT16)(locvar2>>16);
	type = (mobjtype_t)(locvar2&65535);

	mo = P_SpawnMobj(x<<FRACBITS, y<<FRACBITS, z<<FRACBITS, type);

	// Spawn objects with an angle matching the spawner's, rather than spawning Eastwards - Monster Iestyn
	mo->angle = actor->angle;

	if (actor->eflags & MFE_VERTICALFLIP)
		mo->flags2 |= MF2_OBJECTFLIP;
}

// Function: A_SpawnObjectRelative
//
// Description: Spawns an object relative to the location of the actor
//
// var1:
//		var1 >> 16 = x
//		var1 & 65535 = y
// var2:
//		var2 >> 16 = z
//		var2 & 65535 = type
//
void A_SpawnObjectRelative(mobj_t *actor)
{
	INT16 x, y, z; // Want to be sure we can use negative values
	mobjtype_t type;
	mobj_t *mo;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SpawnObjectRelative", actor))
		return;
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_SpawnObjectRelative called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	x = (INT16)(locvar1>>16);
	y = (INT16)(locvar1&65535);
	z = (INT16)(locvar2>>16);
	type = (mobjtype_t)(locvar2&65535);

	// Spawn objects correctly in reverse gravity.
	// NOTE: Doing actor->z + actor->height is the bottom of the object while the object has reverse gravity. - Flame
	mo = P_SpawnMobj(actor->x + FixedMul(x<<FRACBITS, actor->scale),
		actor->y + FixedMul(y<<FRACBITS, actor->scale),
		(actor->eflags & MFE_VERTICALFLIP) ? ((actor->z + actor->height - mobjinfo[type].height) - FixedMul(z<<FRACBITS, actor->scale)) : (actor->z + FixedMul(z<<FRACBITS, actor->scale)), type);

	// Spawn objects with an angle matching the spawner's, rather than spawning Eastwards - Monster Iestyn
	mo->angle = actor->angle;

	if (actor->eflags & MFE_VERTICALFLIP)
		mo->flags2 |= MF2_OBJECTFLIP;
}

// Function: A_ChangeAngleRelative
//
// Description: Changes the angle to a random relative value between the min and max. Set min and max to the same value to eliminate randomness
//
// var1 = min
// var2 = max
//
void A_ChangeAngleRelative(mobj_t *actor)
{
	// Oh god, the old code /sucked/. Changed this and the absolute version to get a random range using amin and amax instead of
	//  getting a random angle from the _entire_ spectrum and then clipping. While we're at it, do the angle conversion to the result
	//  rather than the ranges, so <0 and >360 work as possible values. -Red
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	//angle_t angle = (P_RandomByte()+1)<<24;
	const fixed_t amin = locvar1*FRACUNIT;
	const fixed_t amax = locvar2*FRACUNIT;
	//const angle_t amin = FixedAngle(locvar1*FRACUNIT);
	//const angle_t amax = FixedAngle(locvar2*FRACUNIT);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ChangeAngleRelative", actor))
		return;
#endif

#ifdef PARANOIA
	if (amin > amax)
		I_Error("A_ChangeAngleRelative: var1 is greater then var2");
#endif
/*
	if (angle < amin)
		angle = amin;
	if (angle > amax)
		angle = amax;*/

	actor->angle += FixedAngle(P_RandomRange(amin, amax));
}

// Function: A_ChangeAngleAbsolute
//
// Description: Changes the angle to a random absolute value between the min and max. Set min and max to the same value to eliminate randomness
//
// var1 = min
// var2 = max
//
void A_ChangeAngleAbsolute(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	//angle_t angle = (P_RandomByte()+1)<<24;
	const fixed_t amin = locvar1*FRACUNIT;
	const fixed_t amax = locvar2*FRACUNIT;
	//const angle_t amin = FixedAngle(locvar1*FRACUNIT);
	//const angle_t amax = FixedAngle(locvar2*FRACUNIT);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ChangeAngleAbsolute", actor))
		return;
#endif

#ifdef PARANOIA
	if (amin > amax)
		I_Error("A_ChangeAngleAbsolute: var1 is greater then var2");
#endif
/*
	if (angle < amin)
		angle = amin;
	if (angle > amax)
		angle = amax;*/

	actor->angle = FixedAngle(P_RandomRange(amin, amax));
}

// Function: A_PlaySound
//
// Description: Plays a sound
//
// var1 = sound # to play
// var2:
//		0 = Play sound without an origin
//		1 = Play sound using calling object as origin
//
void A_PlaySound(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_PlaySound", actor))
		return;
#endif

	S_StartSound(locvar2 ? actor : NULL, locvar1);
}

// Function: A_FindTarget
//
// Description: Finds the nearest/furthest mobj of the specified type and sets actor->target to it.
//
// var1 = mobj type
// var2 = if (0) nearest; else furthest;
//
void A_FindTarget(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *targetedmobj = NULL;
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist1 = 0, dist2 = 0;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FindTarget", actor))
		return;
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_FindTarget called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == (mobjtype_t)locvar1)
		{
			if (mo2->player && (mo2->player->spectator || mo2->player->pflags & PF_INVIS))
				continue; // Ignore spectators
			if ((mo2->player || mo2->flags & MF_ENEMY) && mo2->health <= 0)
				continue; // Ignore dead things
			if (targetedmobj == NULL)
			{
				targetedmobj = mo2;
				dist2 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);
			}
			else
			{
				dist1 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);

				if ((!locvar2 && dist1 < dist2) || (locvar2 && dist1 > dist2))
				{
					targetedmobj = mo2;
					dist2 = dist1;
				}
			}
		}
	}

	if (!targetedmobj)
	{
		CONS_Debug(DBG_GAMELOGIC, "A_FindTarget: Unable to find the specified object to target.\n");
		return; // Oops, nothing found..
	}

	CONS_Debug(DBG_GAMELOGIC, "A_FindTarget: Found a target.\n");

	P_SetTarget(&actor->target, targetedmobj);
}

// Function: A_FindTracer
//
// Description: Finds the nearest/furthest mobj of the specified type and sets actor->tracer to it.
//
// var1 = mobj type
// var2 = if (0) nearest; else furthest;
//
void A_FindTracer(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *targetedmobj = NULL;
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist1 = 0, dist2 = 0;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FindTracer", actor))
		return;
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_FindTracer called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == (mobjtype_t)locvar1)
		{
			if (mo2->player && (mo2->player->spectator || mo2->player->pflags & PF_INVIS))
				continue; // Ignore spectators
			if ((mo2->player || mo2->flags & MF_ENEMY) && mo2->health <= 0)
				continue; // Ignore dead things
			if (targetedmobj == NULL)
			{
				targetedmobj = mo2;
				dist2 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);
			}
			else
			{
				dist1 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);

				if ((!locvar2 && dist1 < dist2) || (locvar2 && dist1 > dist2))
				{
					targetedmobj = mo2;
					dist2 = dist1;
				}
			}
		}
	}

	if (!targetedmobj)
	{
		CONS_Debug(DBG_GAMELOGIC, "A_FindTracer: Unable to find the specified object to target.\n");
		return; // Oops, nothing found..
	}

	CONS_Debug(DBG_GAMELOGIC, "A_FindTracer: Found a target.\n");

	P_SetTarget(&actor->tracer, targetedmobj);
}

// Function: A_SetTics
//
// Description: Sets the animation tics of an object
//
// var1 = tics to set to
// var2 = if this is set, and no var1 is supplied, the mobj's threshold value will be used.
//
void A_SetTics(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetTics", actor))
		return;
#endif

	if (locvar1)
		actor->tics = locvar1;
	else if (locvar2)
		actor->tics = actor->threshold;
}

// Function: A_SetRandomTics
//
// Description: Sets the animation tics of an object to a random value
//
// var1 = lower bound
// var2 = upper bound
//
void A_SetRandomTics(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetRandomTics", actor))
		return;
#endif

	actor->tics = P_RandomRange(locvar1, locvar2);
}

// Function: A_ChangeColorRelative
//
// Description: Changes the color of an object
//
// var1 = if (var1 > 0), find target and add its color value to yours
// var2 = if (var1 = 0), color value to add
//
void A_ChangeColorRelative(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ChangeColorRelative", actor))
		return;
#endif

	if (locvar1)
	{
		// Have you ever seen anything so hideous?
		if (actor->target)
			actor->color = (UINT8)(actor->color + actor->target->color);
	}
	else
		actor->color = (UINT8)(actor->color + locvar2);
}

// Function: A_ChangeColorAbsolute
//
// Description: Changes the color of an object by an absolute value. Note: 0 is default colormap.
//
// var1 = if (var1 > 0), set your color to your target's color
// var2 = if (var1 = 0), color value to set to
//
void A_ChangeColorAbsolute(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ChangeColorAbsolute", actor))
		return;
#endif

	if (locvar1)
	{
		if (actor->target)
			actor->color = actor->target->color;
	}
	else
		actor->color = (UINT8)locvar2;
}

// Function: A_MoveRelative
//
// Description: Moves an object (wrapper for P_Thrust)
//
// var1 = angle
// var2 = force
//
void A_MoveRelative(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MoveRelative", actor))
		return;
#endif

	P_Thrust(actor, actor->angle+FixedAngle(locvar1*FRACUNIT), FixedMul(locvar2*FRACUNIT, actor->scale));
}

// Function: A_MoveAbsolute
//
// Description: Moves an object (wrapper for P_InstaThrust)
//
// var1 = angle
// var2 = force
//
void A_MoveAbsolute(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MoveAbsolute", actor))
		return;
#endif

	P_InstaThrust(actor, FixedAngle(locvar1*FRACUNIT), FixedMul(locvar2*FRACUNIT, actor->scale));
}

// Function: A_Thrust
//
// Description: Pushes the object horizontally at its current angle.
//
// var1 = amount of force
// var2 = If 1, xy momentum is lost. If 0, xy momentum is kept
//
void A_Thrust(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Thrust", actor))
		return;
#endif

	if (!locvar1)
		CONS_Debug(DBG_GAMELOGIC, "A_Thrust: Var1 not specified!\n");

	if (locvar2)
		P_InstaThrust(actor, actor->angle, FixedMul(locvar1*FRACUNIT, actor->scale));
	else
		P_Thrust(actor, actor->angle, FixedMul(locvar1*FRACUNIT, actor->scale));
}

// Function: A_ZThrust
//
// Description: Pushes the object up or down.
//
// var1 = amount of force
// var2:
//		lower 16 bits = If 1, xy momentum is lost. If 0, xy momentum is kept
//		upper 16 bits = If 1, z momentum is lost. If 0, z momentum is kept
//
void A_ZThrust(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ZThrust", actor))
		return;
#endif

	if (!locvar1)
		CONS_Debug(DBG_GAMELOGIC, "A_ZThrust: Var1 not specified!\n");

	if (locvar2 & 65535)
		actor->momx = actor->momy = 0;

	if (actor->eflags & MFE_VERTICALFLIP)
		actor->z--;
	else
		actor->z++;

	P_SetObjectMomZ(actor, locvar1*FRACUNIT, !(locvar2 >> 16));
}

// Function: A_SetTargetsTarget
//
// Description: Sets your target to the object who your target is targeting. Yikes! If it happens to be NULL, you're just out of luck.
//
// var1: (Your target)
//		0 = target
//		1 = tracer
// var2: (Your target's target)
//		0 = target/tracer's target
//		1 = target/tracer's tracer
//
void A_SetTargetsTarget(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *oldtarg = NULL, *newtarg = NULL;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetTargetsTarget", actor))
		return;
#endif

	// actor's target
	if (locvar1) // or tracer
		oldtarg = actor->tracer;
	else
		oldtarg = actor->target;

	if (P_MobjWasRemoved(oldtarg))
		return;

	// actor's target's target!
	if (locvar2) // or tracer
		newtarg = oldtarg->tracer;
	else
		newtarg = oldtarg->target;

	if (P_MobjWasRemoved(newtarg))
		return;

	// set actor's new target
	if (locvar1) // or tracer
		P_SetTarget(&actor->tracer, newtarg);
	else
		P_SetTarget(&actor->target, newtarg);
}

// Function: A_SetObjectFlags
//
// Description: Sets the flags of an object
//
// var1 = flag value to set
// var2:
//		if var2 == 2, add the flag to the current flags
//		else if var2 == 1, remove the flag from the current flags
//		else if var2 == 0, set the flags to the exact value
//
void A_SetObjectFlags(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	boolean unlinkthings = false;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetObjectFlags", actor))
		return;
#endif

	if (locvar2 == 2)
		locvar1 = actor->flags | locvar1;
	else if (locvar2 == 1)
		locvar1 = actor->flags & ~locvar1;

	if ((UINT32)(locvar1 & (MF_NOBLOCKMAP|MF_NOSECTOR)) != (actor->flags & (MF_NOBLOCKMAP|MF_NOSECTOR))) // Blockmap/sector status has changed, so reset the links
		unlinkthings = true;

	if (unlinkthings) {
		P_UnsetThingPosition(actor);
		if (sector_list)
		{
			P_DelSeclist(sector_list);
			sector_list = NULL;
		}
	}

	actor->flags = locvar1;

	if (unlinkthings)
		P_SetThingPosition(actor);
}

// Function: A_SetObjectFlags2
//
// Description: Sets the flags2 of an object
//
// var1 = flag value to set
// var2:
//		if var2 == 2, add the flag to the current flags
//		else if var2 == 1, remove the flag from the current flags
//		else if var2 == 0, set the flags to the exact value
//
void A_SetObjectFlags2(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetObjectFlags2", actor))
		return;
#endif

	if (locvar2 == 2)
		actor->flags2 |= locvar1;
	else if (locvar2 == 1)
		actor->flags2 &= ~locvar1;
	else
		actor->flags2 = locvar1;
}

// Function: A_BossJetFume
//
// Description: Spawns jet fumes/other attachment miscellany for the boss. To only be used when he is spawned.
//
// var1:
//		0 - Triple jet fume pattern
//		1 - Boss 3's propeller
//		2 - Metal Sonic jet fume
//		3 - Boss 4 jet flame
// var2 = unused
//
void A_BossJetFume(mobj_t *actor)
{
	mobj_t *filler;
	INT32 locvar1 = var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BossJetFume", actor))
		return;
#endif

	if (locvar1 == 0) // Boss1 jet fumes
	{
		fixed_t jetx, jety, jetz;

		jetx = actor->x + P_ReturnThrustX(actor, actor->angle, -FixedMul(64*FRACUNIT, actor->scale));
		jety = actor->y + P_ReturnThrustY(actor, actor->angle, -FixedMul(64*FRACUNIT, actor->scale));
		if (actor->eflags & MFE_VERTICALFLIP)
			jetz = actor->z + actor->height - FixedMul(38*FRACUNIT + mobjinfo[MT_JETFUME1].height, actor->scale);
		else
			jetz = actor->z + FixedMul(38*FRACUNIT, actor->scale);

		filler = P_SpawnMobj(jetx, jety, jetz, MT_JETFUME1);
		P_SetTarget(&filler->target, actor);
		filler->destscale = actor->scale;
		P_SetScale(filler, filler->destscale);
		if (actor->eflags & MFE_VERTICALFLIP)
			filler->flags2 |= MF2_OBJECTFLIP;
		filler->fuse = 56;

		if (actor->eflags & MFE_VERTICALFLIP)
			jetz = actor->z + actor->height - FixedMul(12*FRACUNIT + mobjinfo[MT_JETFUME1].height, actor->scale);
		else
			jetz = actor->z + FixedMul(12*FRACUNIT, actor->scale);

		filler = P_SpawnMobj(jetx + P_ReturnThrustX(actor, actor->angle-ANGLE_90, FixedMul(24*FRACUNIT, actor->scale)),
				jety + P_ReturnThrustY(actor, actor->angle-ANGLE_90, FixedMul(24*FRACUNIT, actor->scale)),
				jetz, MT_JETFUME1);
		P_SetTarget(&filler->target, actor);
		filler->destscale = actor->scale;
		P_SetScale(filler, filler->destscale);
		if (actor->eflags & MFE_VERTICALFLIP)
			filler->flags2 |= MF2_OBJECTFLIP;
		filler->fuse = 57;

		filler = P_SpawnMobj(jetx + P_ReturnThrustX(actor, actor->angle+ANGLE_90, FixedMul(24*FRACUNIT, actor->scale)),
				jety + P_ReturnThrustY(actor, actor->angle+ANGLE_90, FixedMul(24*FRACUNIT, actor->scale)),
				jetz, MT_JETFUME1);
		P_SetTarget(&filler->target, actor);
		filler->destscale = actor->scale;
		P_SetScale(filler, filler->destscale);
		if (actor->eflags & MFE_VERTICALFLIP)
			filler->flags2 |= MF2_OBJECTFLIP;
		filler->fuse = 58;

		P_SetTarget(&actor->tracer, filler);
	}
	else if (locvar1 == 1) // Boss 3 propeller
	{
		fixed_t jetx, jety, jetz;

		jetx = actor->x + P_ReturnThrustX(actor, actor->angle, -FixedMul(60*FRACUNIT, actor->scale));
		jety = actor->y + P_ReturnThrustY(actor, actor->angle, -FixedMul(60*FRACUNIT, actor->scale));
		if (actor->eflags & MFE_VERTICALFLIP)
			jetz = actor->z + actor->height - FixedMul(17*FRACUNIT + mobjinfo[MT_PROPELLER].height, actor->scale);
		else
			jetz = actor->z + FixedMul(17*FRACUNIT, actor->scale);

		filler = P_SpawnMobj(jetx, jety, jetz, MT_PROPELLER);
		P_SetTarget(&filler->target, actor);
		filler->destscale = actor->scale;
		P_SetScale(filler, filler->destscale);
		if (actor->eflags & MFE_VERTICALFLIP)
			filler->flags2 |= MF2_OBJECTFLIP;
		filler->angle = actor->angle - ANGLE_180;

		P_SetTarget(&actor->tracer, filler);
	}
	else if (locvar1 == 2) // Metal Sonic jet fumes
	{
		filler = P_SpawnMobj(actor->x, actor->y, actor->z, MT_JETFUME1);
		P_SetTarget(&filler->target, actor);
		filler->fuse = 59;
		P_SetTarget(&actor->tracer, filler);
		filler->destscale = actor->scale/2;
		P_SetScale(filler, filler->destscale);
		if (actor->eflags & MFE_VERTICALFLIP)
			filler->flags2 |= MF2_OBJECTFLIP;
	}
	else if (locvar1 == 3) // Boss 4 jet flame
	{
		fixed_t jetz;
		if (actor->eflags & MFE_VERTICALFLIP)
			jetz = actor->z + actor->height + FixedMul(50*FRACUNIT - mobjinfo[MT_JETFLAME].height, actor->scale);
		else
			jetz = actor->z - FixedMul(50*FRACUNIT, actor->scale);
		filler = P_SpawnMobj(actor->x, actor->y, jetz, MT_JETFLAME);
		P_SetTarget(&filler->target, actor);
		// Boss 4 already uses its tracer for other things
		filler->destscale = actor->scale;
		P_SetScale(filler, filler->destscale);
		if (actor->eflags & MFE_VERTICALFLIP)
			filler->flags2 |= MF2_OBJECTFLIP;
	}
}

// Function: A_RandomState
//
// Description: Chooses one of the two state numbers supplied randomly.
//
// var1 = state number 1
// var2 = state number 2
//
void A_RandomState(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RandomState", actor))
		return;
#endif

	P_SetMobjState(actor, P_RandomChance(FRACUNIT/2) ? locvar1 : locvar2);
}

// Function: A_RandomStateRange
//
// Description: Chooses a random state within the range supplied.
//
// var1 = Minimum state number to choose.
// var2 = Maximum state number to use.
//
void A_RandomStateRange(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RandomStateRange", actor))
		return;
#endif

	P_SetMobjState(actor, P_RandomRange(locvar1, locvar2));
}

// Function: A_DualAction
//
// Description: Calls two actions. Be careful, if you reference the same state this action is called from, you can create an infinite loop.
//
// var1 = state # to use 1st action from
// var2 = state # to use 2nd action from
//
void A_DualAction(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_DualAction", actor))
		return;
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_DualAction called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	var1 = states[locvar1].var1;
	var2 = states[locvar1].var2;
#ifdef HAVE_BLUA
	astate = &states[locvar1];
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_DualAction: Calling First Action (state %d)...\n", locvar1);
	states[locvar1].action.acp1(actor);

	var1 = states[locvar2].var1;
	var2 = states[locvar2].var2;
#ifdef HAVE_BLUA
	astate = &states[locvar2];
#endif

	CONS_Debug(DBG_GAMELOGIC, "A_DualAction: Calling Second Action (state %d)...\n", locvar2);
	states[locvar2].action.acp1(actor);
}

// Function: A_RemoteAction
//
// Description: var1 is the remote object. var2 is the state reference for calling the action called on var1. var1 becomes the actor's target, the action (var2) is called on var1. actor's target is restored
//
// var1 = remote object (-2 uses tracer, -1 uses target)
// var2 = state reference for calling an action
//
void A_RemoteAction(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *originaltarget = actor->target; // Hold on to the target for later.
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RemoteAction", actor))
		return;
#endif

	// If >=0, find the closest target.
	if (locvar1 >= 0)
	{
		///* DO A_FINDTARGET STUFF *///
		mobj_t *targetedmobj = NULL;
		thinker_t *th;
		mobj_t *mo2;
		fixed_t dist1 = 0, dist2 = 0;

		// scan the thinkers
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type == (mobjtype_t)locvar1)
			{
				if (targetedmobj == NULL)
				{
					targetedmobj = mo2;
					dist2 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);
				}
				else
				{
					dist1 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);

					if ((locvar2 && dist1 < dist2) || (!locvar2 && dist1 > dist2))
					{
						targetedmobj = mo2;
						dist2 = dist1;
					}
				}
			}
		}

		if (!targetedmobj)
		{
			CONS_Debug(DBG_GAMELOGIC, "A_RemoteAction: Unable to find the specified object to target.\n");
			return; // Oops, nothing found..
		}

		CONS_Debug(DBG_GAMELOGIC, "A_RemoteAction: Found a target.\n");

		P_SetTarget(&actor->target, targetedmobj);

		///* END A_FINDTARGET STUFF *///
	}

	// If -2, use the tracer as the target
	else if (locvar1 == -2)
		P_SetTarget(&actor->target, actor->tracer);
	// if -1 or anything else, just use the target.

	if (actor->target)
	{
		// Steal the var1 and var2 from "locvar2"
		var1 = states[locvar2].var1;
		var2 = states[locvar2].var2;
#ifdef HAVE_BLUA
		astate = &states[locvar2];
#endif

		CONS_Debug(DBG_GAMELOGIC, "A_RemoteAction: Calling action on %p\n"
				"var1 is %d\nvar2 is %d\n", actor->target, var1, var2);
		states[locvar2].action.acp1(actor->target);
	}

	P_SetTarget(&actor->target, originaltarget); // Restore the original target.
}

// Function: A_ToggleFlameJet
//
// Description: Turns a flame jet on and off.
//
// var1 = unused
// var2 = unused
//
void A_ToggleFlameJet(mobj_t* actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ToggleFlameJet", actor))
		return;
#endif
	// threshold - off delay
	// movecount - on timer

	if (actor->flags2 & MF2_FIRING)
	{
		actor->flags2 &= ~MF2_FIRING;

		if (actor->threshold)
			actor->tics = actor->threshold;
	}
	else
	{
		actor->flags2 |= MF2_FIRING;

		if (actor->movecount)
			actor->tics = actor->movecount;
	}
}

// Function: A_OrbitNights
//
// Description: Used by Chaos Emeralds to orbit around Nights (aka Super Sonic.)
//
// var1 = Angle adjustment (aka orbit speed)
// var2 = Lower four bits: height offset, Upper 4 bits = set if object is Nightopian Helper
//
void A_OrbitNights(mobj_t* actor)
{
	INT32 ofs = (var2 & 0xFFFF);
	boolean ishelper = (var2 & 0xFFFF0000);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_OrbitNights", actor))
		return;
#endif

	if (!actor->target || !actor->target->player ||
	    !(actor->target->player->powers[pw_carry] == CR_NIGHTSMODE) || !actor->target->player->nightstime
	    // Also remove this object if they no longer have a NiGHTS helper
		|| (ishelper && !actor->target->player->powers[pw_nights_helper]))
	{
		P_RemoveMobj(actor);
		return;
	}
	else
	{
		actor->extravalue1 += var1;
		P_UnsetThingPosition(actor);
		{
			const angle_t fa  = (angle_t)actor->extravalue1 >> ANGLETOFINESHIFT;
			const angle_t ofa = ((angle_t)actor->extravalue1 + (ofs*ANG1)) >> ANGLETOFINESHIFT;

			const fixed_t fc = FixedMul(FINECOSINE(fa),FixedMul(32*FRACUNIT, actor->scale));
			const fixed_t fh = FixedMul(FINECOSINE(ofa),FixedMul(20*FRACUNIT, actor->scale));
			const fixed_t fs = FixedMul(FINESINE(fa),FixedMul(32*FRACUNIT, actor->scale));

			actor->x = actor->target->x + fc;
			actor->y = actor->target->y + fs;
			actor->z = actor->target->z + fh + FixedMul(16*FRACUNIT, actor->scale);

			// Semi-lazy hack
			actor->angle = (angle_t)actor->extravalue1 + ANGLE_90;
		}
		P_SetThingPosition(actor);

		if (ishelper) // Flash a helper that's about to be removed.
		{
			if ((actor->target->player->powers[pw_nights_helper] < TICRATE)
			&& (actor->target->player->powers[pw_nights_helper] & 1))
				actor->flags2 |= MF2_DONTDRAW;
			else
				actor->flags2 &= ~MF2_DONTDRAW;
		}
	}
}

// Function: A_GhostMe
//
// Description: Spawns a "ghost" mobj of this actor, ala spindash trails and the minus's digging "trails"
//
// var1 = unused
// var2 = unused
//
void A_GhostMe(mobj_t *actor)
{
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_GhostMe", actor))
		return;
#endif
	P_SpawnGhostMobj(actor);
}

// Function: A_SetObjectState
//
// Description: Changes the state of an object's target/tracer.
//
// var1 = state number
// var2:
//		0 = target
//		1 = tracer
//
void A_SetObjectState(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *target;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetObjectState", actor))
		return;
#endif

	if ((!locvar2 && !actor->target) || (locvar2 && !actor->tracer))
	{
		if (cv_debug)
			CONS_Printf("A_SetObjectState: No target to change state!\n");
		return;
	}

	if (!locvar2) // target
		target = actor->target;
	else // tracer
		target = actor->tracer;

	if (target->health > 0)
	{
		if (!target->player)
			P_SetMobjState(target, locvar1);
		else
			P_SetPlayerMobjState(target, locvar1);
	}
}

// Function: A_SetObjectTypeState
//
// Description: Changes the state of all active objects of a certain type in a certain range of the actor.
//
// var1 = state number
// var2:
//		lower 16 bits = type
//		upper 16 bits = range (if == 0, across whole map)
//
void A_SetObjectTypeState(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	const UINT16 loc2lw = (UINT16)(locvar2 & 65535);
	const UINT16 loc2up = (UINT16)(locvar2 >> 16);

	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist = 0;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetObjectTypeState", actor))
		return;
#endif

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == (mobjtype_t)loc2lw)
		{
			dist = P_AproxDistance(mo2->x - actor->x, mo2->y - actor->y);

			if (mo2->health > 0)
			{
				if (loc2up == 0)
					P_SetMobjState(mo2, locvar1);
				else
				{
					if (dist <= FixedMul(loc2up*FRACUNIT, actor->scale))
						P_SetMobjState(mo2, locvar1);
				}
			}
		}
	}
}

// Function: A_KnockBack
//
// Description: Knocks back the object's target at its current speed.
//
// var1:
//		0 = target
//		1 = tracer
// var2 = unused
//
void A_KnockBack(mobj_t *actor)
{
	INT32 locvar1 = var1;
	mobj_t *target;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_KnockBack", actor))
		return;
#endif

	if (!locvar1)
		target = actor->target;
	else
		target = actor->tracer;

	if (!target)
	{
		if(cv_debug)
			CONS_Printf("A_KnockBack: No target!\n");
		return;
	}

	target->momx *= -1;
	target->momy *= -1;
}

// Function: A_PushAway
//
// Description: Pushes an object's target away from the calling object.
//
// var1 = amount of force
// var2:
//		lower 16 bits = If 1, xy momentum is lost. If 0, xy momentum is kept
//		upper 16 bits = 0 - target, 1 - tracer
//
void A_PushAway(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *target; // target
	angle_t an; // actor to target angle
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_PushAway", actor))
		return;
#endif

	if ((!(locvar2 >> 16) && !actor->target) || ((locvar2 >> 16) && !actor->tracer))
		return;

	if (!locvar1)
		CONS_Printf("A_Thrust: Var1 not specified!\n");

	if (!(locvar2 >> 16)) // target
		target = actor->target;
	else // tracer
		target = actor->tracer;

	an = R_PointToAngle2(actor->x, actor->y, target->x, target->y);

	if (locvar2 & 65535)
		P_InstaThrust(target, an, FixedMul(locvar1*FRACUNIT, actor->scale));
	else
		P_Thrust(target, an, FixedMul(locvar1*FRACUNIT, actor->scale));
}

// Function: A_RingDrain
//
// Description: Drain targeted player's rings.
//
// var1 = ammount of drained rings
// var2 = unused
//
void A_RingDrain(mobj_t *actor)
{
	INT32 locvar1 = var1;
	player_t *player;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RingDrain", actor))
		return;
#endif

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("A_RingDrain: No player targeted!\n");
		return;
	}

	player = actor->target->player;
	P_GivePlayerRings(player, -min(locvar1, player->rings));
}

// Function: A_SplitShot
//
// Description: Shoots 2 missiles that hit next to the player.
//
// var1 = target x-y-offset
// var2:
//		lower 16 bits = missile type
//		upper 16 bits = height offset
//
void A_SplitShot(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	const UINT16 loc2lw = (UINT16)(locvar2 & 65535);
	const UINT16 loc2up = (UINT16)(locvar2 >> 16);
	const fixed_t offs = (fixed_t)(locvar1*FRACUNIT);
	const fixed_t hoffs = (fixed_t)(loc2up*FRACUNIT);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SplitShot", actor))
		return;
#endif

	A_FaceTarget(actor);
	{
		const angle_t an = (actor->angle + ANGLE_90) >> ANGLETOFINESHIFT;
		const fixed_t fasin = FINESINE(an);
		const fixed_t facos = FINECOSINE(an);
		fixed_t xs = FixedMul(facos,FixedMul(offs, actor->scale));
		fixed_t ys = FixedMul(fasin,FixedMul(offs, actor->scale));
		fixed_t z;

		if (actor->eflags & MFE_VERTICALFLIP)
			z = actor->z + actor->height - FixedMul(hoffs, actor->scale);
		else
			z = actor->z + FixedMul(hoffs, actor->scale);

		P_SpawnPointMissile(actor, actor->target->x+xs, actor->target->y+ys, actor->target->z, loc2lw, actor->x, actor->y, z);
		P_SpawnPointMissile(actor, actor->target->x-xs, actor->target->y-ys, actor->target->z, loc2lw, actor->x, actor->y, z);
	}
}

// Function: A_MissileSplit
//
// Description: If the object is a missile it will create a new missile with an alternate flight path owned by the one who shot the former missile.
//
// var1 = splitting missile type
// var2 = splitting angle
//
void A_MissileSplit(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MissileSplit", actor))
		return;
#endif
	if (actor->eflags & MFE_VERTICALFLIP)
		P_SpawnAlteredDirectionMissile(actor, locvar1, actor->x, actor->y, actor->z+actor->height, locvar2);
	else
		P_SpawnAlteredDirectionMissile(actor, locvar1, actor->x, actor->y, actor->z, locvar2);
}

// Function: A_MultiShot
//
// Description: Shoots objects horizontally that spread evenly in all directions.
//
// var1:
//		lower 16 bits = number of missiles
//		upper 16 bits = missile type #
// var2 = height offset
//
void A_MultiShot(mobj_t *actor)
{
	fixed_t z, xr, yr;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	const UINT16 loc1lw = (UINT16)(locvar1 & 65535);
	const UINT16 loc1up = (UINT16)(locvar1 >> 16);
	INT32 count = 0;
	fixed_t ad;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_MultiShot", actor))
		return;
#endif

	if (actor->target)
		A_FaceTarget(actor);

	if(loc1lw > 90)
		ad = FixedMul(90*FRACUNIT, actor->scale);
	else
		ad = FixedMul(loc1lw*FRACUNIT, actor->scale);

	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - FixedMul(48*FRACUNIT + locvar2*FRACUNIT, actor->scale);
	else
		z = actor->z + FixedMul(48*FRACUNIT + locvar2*FRACUNIT, actor->scale);
	xr = FixedMul((P_SignedRandom()/3)<<FRACBITS, actor->scale); // please note p_mobj.c's P_Rand() abuse
	yr = FixedMul((P_SignedRandom()/3)<<FRACBITS, actor->scale); // of rand(), RAND_MAX and signness mess

	while(count <= loc1lw && loc1lw >= 1)
	{
		const angle_t fa = FixedAngleC(count*FRACUNIT*360, ad)>>ANGLETOFINESHIFT;
		const fixed_t rc = FINECOSINE(fa);
		const fixed_t rs = FINESINE(fa);
		const fixed_t xrc = FixedMul(xr, rc);
		const fixed_t yrs = FixedMul(yr, rs);
		const fixed_t xrs = FixedMul(xr, rs);
		const fixed_t yrc = FixedMul(yr, rc);

		P_SpawnPointMissile(actor, xrc-yrs+actor->x, xrs+yrc+actor->y, z, loc1up, actor->x, actor->y, z);
		count++;
	}

	if (!(actor->flags & MF_BOSS))
	{
		if (ultimatemode)
			actor->reactiontime = actor->info->reactiontime*TICRATE;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	}
}

// Function: A_InstaLoop
//
// Description: Makes the object move along a 2d (view angle, z) polygon.
//
// var1:
//		lower 16 bits = current step
//		upper 16 bits = maximum step #
// var2 = force
//
void A_InstaLoop(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	fixed_t force = max(locvar2, 1)*FRACUNIT; // defaults to 1 if var2 < 1
	const UINT16 loc1lw = (UINT16)(locvar1 & 65535);
	const UINT16 loc1up = (UINT16)(locvar1 >> 16);
	const angle_t fa = FixedAngleC(loc1lw*FRACUNIT*360, loc1up*FRACUNIT)>>ANGLETOFINESHIFT;
	const fixed_t ac = FINECOSINE(fa);
	const fixed_t as = FINESINE(fa);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_InstaLoop", actor))
		return;
#endif

	P_InstaThrust(actor, actor->angle, FixedMul(ac, FixedMul(force, actor->scale)));
	P_SetObjectMomZ(actor, FixedMul(as, force), false);
}

// Function: A_Custom3DRotate
//
// Description: Rotates the actor around its target in 3 dimensions.
//
// var1:
//		lower 16 bits = radius in fracunits
//		upper 16 bits = vertical offset
// var2:
//		lower 16 bits = vertical rotation speed in 1/10 fracunits per tic
//		upper 16 bits = horizontal rotation speed in 1/10 fracunits per tic
//
void A_Custom3DRotate(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

	const UINT16 loc1lw = (UINT16)(locvar1 & 65535);
	const UINT16 loc1up = (UINT16)(locvar1 >> 16);
	const UINT16 loc2lw = (UINT16)(locvar2 & 65535);
	const UINT16 loc2up = (UINT16)(locvar2 >> 16);

	const fixed_t radius = FixedMul(loc1lw*FRACUNIT, actor->scale);
	const fixed_t hOff = FixedMul(loc1up*FRACUNIT, actor->scale);
	const fixed_t hspeed = FixedMul(loc2up*FRACUNIT/10, actor->scale);
	const fixed_t vspeed = FixedMul(loc2lw*FRACUNIT/10, actor->scale);
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Custom3DRotate", actor))
		return;
#endif

	if (actor->target->health == 0)
	{
		P_RemoveMobj(actor);
		return;
	}

	if (!actor->target) // This should NEVER happen.
	{
		if (cv_debug)
			CONS_Printf("Error: Object has no target\n");
		P_RemoveMobj(actor);
		return;
	}
	if (hspeed==0 && vspeed==0)
	{
		CONS_Printf("Error: A_Custom3DRotate: Object has no speed.\n");
		return;
	}

	actor->angle += FixedAngle(hspeed);
	actor->movedir += FixedAngle(vspeed);
	P_UnsetThingPosition(actor);
	{
		const angle_t fa = actor->angle>>ANGLETOFINESHIFT;

		if (vspeed == 0 && hspeed != 0)
		{
			actor->x = actor->target->x + FixedMul(FINECOSINE(fa),radius);
			actor->y = actor->target->y + FixedMul(FINESINE(fa),radius);
			actor->z = actor->target->z + actor->target->height/2 - actor->height/2 + hOff;
		}
		else
		{
			const angle_t md = actor->movedir>>ANGLETOFINESHIFT;
			actor->x = actor->target->x + FixedMul(FixedMul(FINESINE(md),FINECOSINE(fa)),radius);
			actor->y = actor->target->y + FixedMul(FixedMul(FINESINE(md),FINESINE(fa)),radius);
			actor->z = actor->target->z + FixedMul(FINECOSINE(md),radius) + actor->target->height/2 - actor->height/2 + hOff;
		}
	}
	P_SetThingPosition(actor);
}

// Function: A_SearchForPlayers
//
// Description: Checks if the actor has targeted a vulnerable player. If not a new player will be searched all around. If no players are available the object can call a specific state. (Useful for not moving enemies)
//
// var1:
//		if var1 == 0, if necessary call state with same state number as var2
//		else, do not call a specific state if no players are available
// var2 = state number
//
void A_SearchForPlayers(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SearchForPlayers", actor))
		return;
#endif

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		if(locvar1==0)
		{
			P_SetMobjStateNF(actor, locvar2);
			return;
		}
	}
}

// Function: A_CheckRandom
//
// Description: Calls a state by chance.
//
// var1:
//		lower 16 bits = denominator
//		upper 16 bits = numerator (defaults to 1 if zero)
// var2 = state number
//
void A_CheckRandom(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	fixed_t chance = FRACUNIT;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckRandom", actor))
		return;
#endif
	if ((locvar1 & 0xFFFF) == 0)
		return;

	// The PRNG doesn't suck anymore, OK?
	if (locvar1 >> 16)
		chance *= (locvar1 >> 16);
	chance /= (locvar1 & 0xFFFF);

	if (P_RandomChance(chance))
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckTargetRings
//
// Description: Calls a state depending on the ammount of rings currently owned by targeted players.
//
// var1 = if player rings >= var1 call state
// var2 = state number
//
void A_CheckTargetRings(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckTargetRings", actor))
		return;
#endif

	if (!(actor->target) || !(actor->target->player))
	   return;

	if (actor->target->player->rings >= locvar1)
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckRings
//
// Description: Calls a state depending on the ammount of rings currently owned by all players.
//
// var1 = if player rings >= var1 call state
// var2 = state number
//
void A_CheckRings(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	INT32 i, cntr = 0;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckRings", actor))
		return;
#endif

	for (i = 0; i < MAXPLAYERS; i++)
		cntr += players[i].rings;

	if (cntr >= locvar1)
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckTotalRings
//
// Description: Calls a state depending on the maximum ammount of rings owned by all players during this try.
//
// var1 = if total player rings >= var1 call state
// var2 = state number
//
void A_CheckTotalRings(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

	INT32 i, cntr = 0;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckTotalRings", actor))
		return;
#endif

	for (i = 0; i < MAXPLAYERS; i++)
		cntr += players[i].totalring;

	if (cntr >= locvar1)
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckHealth
//
// Description: Calls a state depending on the object's current health.
//
// var1 = if health <= var1 call state
// var2 = state number
//
void A_CheckHealth(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckHealth", actor))
		return;
#endif

	if (actor->health <= locvar1)
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckRange
//
// Description: Calls a state if the object's target is in range.
//
// var1:
//		lower 16 bits = range
//		upper 16 bits = 0 - target, 1 - tracer
// var2 = state number
//
void A_CheckRange(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	fixed_t dist;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckRange", actor))
		return;
#endif

	if ((!(locvar1 >> 16) && !actor->target) || ((locvar1 >> 16) && !actor->tracer))
		return;

	if (!(locvar1 >> 16)) //target
		dist = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);
	else //tracer
		dist = P_AproxDistance(actor->tracer->x - actor->x, actor->tracer->y - actor->y);

	if (dist <= FixedMul((locvar1 & 65535)*FRACUNIT, actor->scale))
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckHeight
//
// Description: Calls a state if the object and it's target have a height offset <= var1 compared to each other.
//
// var1:
//		lower 16 bits = height offset
//		upper 16 bits = 0 - target, 1 - tracer
// var2 = state number
//
void A_CheckHeight(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	fixed_t height;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckHeight", actor))
		return;
#endif

	if ((!(locvar1 >> 16) && !actor->target) || ((locvar1 >> 16) && !actor->tracer))
		return;

	if (!(locvar1 >> 16)) // target
		height = abs(actor->target->z - actor->z);
	else // tracer
		height = abs(actor->tracer->z - actor->z);

	if (height <= FixedMul((locvar1 & 65535)*FRACUNIT, actor->scale))
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckTrueRange
//
// Description: Calls a state if the object's target is in true range. (Checks height, too.)
//
// var1:
//		lower 16 bits = range
//		upper 16 bits = 0 - target, 1 - tracer
// var2 = state number
//
void A_CheckTrueRange(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	fixed_t height; // vertical range
	fixed_t dist; // horizontal range
	fixed_t l; // true range
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckTrueRange", actor))
		return;
#endif

	if ((!(locvar1 >> 16) && !actor->target) || ((locvar1 >> 16) && !actor->tracer))
		return;

	if (!(locvar1 >> 16)) // target
	{
		height = actor->target->z - actor->z;
		dist = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);

	}
	else // tracer
	{
		height = actor->tracer->z - actor->z;
		dist = P_AproxDistance(actor->tracer->x - actor->x, actor->tracer->y - actor->y);
	}

	l = P_AproxDistance(dist, height);

	if (l <= FixedMul((locvar1 & 65535)*FRACUNIT, actor->scale))
		P_SetMobjState(actor, locvar2);

}

// Function: A_CheckThingCount
//
// Description: Calls a state depending on the number of active things in range.
//
// var1:
//		lower 16 bits = number of things
//		upper 16 bits = thing type
// var2:
//		lower 16 bits = state to call
//		upper 16 bits = range (if == 0, check whole map)
//
void A_CheckThingCount(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

	const UINT16 loc1lw = (UINT16)(locvar1 & 65535);
	const UINT16 loc1up = (UINT16)(locvar1 >> 16);
	const UINT16 loc2lw = (UINT16)(locvar2 & 65535);
	const UINT16 loc2up = (UINT16)(locvar2 >> 16);

	INT32 count = 0;
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist = 0;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckThingCount", actor))
		return;
#endif

	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == (mobjtype_t)loc1up)
		{
			dist = P_AproxDistance(mo2->x - actor->x, mo2->y - actor->y);

			if (loc2up == 0)
				count++;
			else
			{
				if (dist <= FixedMul(loc2up*FRACUNIT, actor->scale))
					count++;
			}
		}
	}

	if(loc1lw <= count)
		P_SetMobjState(actor, loc2lw);
}

// Function: A_CheckAmbush
//
// Description: Calls a state if the actor is behind its targeted player.
//
// var1:
//		0 = target
//		1 = tracer
// var2 = state number
//
void A_CheckAmbush(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	angle_t at; // angle target is currently facing
	angle_t atp; // actor to target angle
	angle_t an; // angle between at and atp

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckAmbush", actor))
		return;
#endif

	if ((!locvar1 && !actor->target) || (locvar1 && !actor->tracer))
		return;

	if (!locvar1) // target
	{
		at = actor->target->angle;
		atp = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
	}
	else // tracer
	{
		at = actor->tracer->angle;
		atp = R_PointToAngle2(actor->x, actor->y, actor->tracer->x, actor->tracer->y);
	}

	an = atp - at;

	if (an > ANGLE_180) // flip angle if bigger than 180
		an = InvAngle(an);

	if (an < ANGLE_90+ANGLE_22h) // within an angle of 112.5 from each other?
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckCustomValue
//
// Description: Calls a state depending on the object's custom value.
//
// var1 = if custom value >= var1, call state
// var2 = state number
//
void A_CheckCustomValue(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckCustomValue", actor))
		return;
#endif

	if (actor->cusval >= locvar1)
		P_SetMobjState(actor, locvar2);
}

// Function: A_CheckCusValMemo
//
// Description: Calls a state depending on the object's custom memory value.
//
// var1 = if memory value >= var1, call state
// var2 = state number
//
void A_CheckCusValMemo(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CheckCusValMemo", actor))
		return;
#endif

	if (actor->cvmem >= locvar1)
		P_SetMobjState(actor, locvar2);
}

// Function: A_SetCustomValue
//
// Description: Changes the custom value of an object.
//
// var1 = manipulating value
// var2:
//      if var2 == 5, multiply the custom value by var1
//      else if var2 == 4, divide the custom value by var1
//      else if var2 == 3, apply modulo var1 to the custom value
//      else if var2 == 2, add var1 to the custom value
//      else if var2 == 1, substract var1 from the custom value
//      else if var2 == 0, replace the custom value with var1
//
void A_SetCustomValue(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetCustomValue", actor))
		return;
#endif

	if (cv_debug)
		CONS_Printf("Init custom value is %d\n", actor->cusval);

	if (locvar1 == 0 && locvar2 == 4)
		return; // DON'T DIVIDE BY ZERO

	// no need for a "temp" value here, just modify the cusval directly
	if (locvar2 == 5) // multiply
		actor->cusval *= locvar1;
	else if (locvar2 == 4) // divide
		actor->cusval /= locvar1;
	else if (locvar2 == 3) // modulo
		actor->cusval %= locvar1;
	else if (locvar2 == 2) // add
		actor->cusval += locvar1;
	else if (locvar2 == 1) // subtract
		actor->cusval -= locvar1;
	else // replace
		actor->cusval = locvar1;

	if(cv_debug)
		CONS_Printf("New custom value is %d\n", actor->cusval);
}

// Function: A_UseCusValMemo
//
// Description: Memorizes or recalls a current custom value.
//
// var1:
//      if var1 == 1, manipulate memory value
//      else, recall memory value replacing the custom value
// var2:
//      if var2 == 5, mem = mem*cv  ||  cv = cv*mem
//      else if var2 == 4,  mem = mem/cv  ||  cv = cv/mem
//      else if var2 == 3, mem = mem%cv  ||  cv = cv%mem
//      else if var2 == 2, mem += cv  ||  cv += mem
//      else if var2 == 1,  mem -= cv  ||  cv -= mem
//      else mem = cv  ||  cv = mem
//
void A_UseCusValMemo(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

	INT32 temp = actor->cusval; // value being manipulated
	INT32 tempM = actor->cvmem; // value used to manipulate temp with
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_UseCusValMemo", actor))
		return;
#endif

	if (locvar1 == 1) // cvmem being changed using cusval
	{
		temp = actor->cvmem;
		tempM = actor->cusval;
	}
	else // cusval being changed with cvmem
	{
		temp = actor->cusval;
		tempM = actor->cvmem;
	}

	if (tempM == 0 && locvar2 == 4)
		return; // DON'T DIVIDE BY ZERO

	// now get new value for cusval/cvmem using the other
	if (locvar2 == 5) // multiply
		temp *= tempM;
	else if (locvar2 == 4) // divide
		temp /= tempM;
	else if (locvar2 == 3) // modulo
		temp %= tempM;
	else if (locvar2 == 2) // add
		temp += tempM;
	else if (locvar2 == 1) // subtract
		temp -= tempM;
	else // replace
		temp = tempM;

	// finally, give cusval/cvmem the new value!
	if (locvar1 == 1)
		actor->cvmem = temp;
	else
		actor->cusval = temp;
}

// Function: A_RelayCustomValue
//
// Description: Manipulates the custom value of the object's target/tracer.
//
// var1:
//		lower 16 bits:
//					if var1 == 0, use own custom value
//					else, use var1 value
//		upper 16 bits = 0 - target, 1 - tracer
// var2:
//      if var2 == 5, multiply the target's custom value by var1
//      else if var2 == 4, divide the target's custom value by var1
//      else if var2 == 3, apply modulo var1 to the target's custom value
//      else if var2 == 2, add var1 to the target's custom value
//      else if var2 == 1, substract var1 from the target's custom value
//      else if var2 == 0, replace the target's custom value with var1
//
void A_RelayCustomValue(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

	INT32 temp; // reference value - var1 lower 16 bits changes this
	INT32 tempT; // target's value - changed to tracer if var1 upper 16 bits set, then modified to become final value
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RelayCustomValue", actor))
		return;
#endif

	if ((!(locvar1 >> 16) && !actor->target) || ((locvar1 >> 16) && !actor->tracer))
		return;

	// reference custom value
	if ((locvar1 & 65535) == 0)
		temp = actor->cusval; // your own custom value
	else
		temp = (locvar1 & 65535); // var1 value

	if (!(locvar1 >> 16)) // target's custom value
		tempT = actor->target->cusval;
	else // tracer's custom value
		tempT = actor->tracer->cusval;

	if (temp == 0 && locvar2 == 4)
		return; // DON'T DIVIDE BY ZERO

	// now get new cusval using target's and the reference
	if (locvar2 == 5) // multiply
		tempT *= temp;
	else if (locvar2 == 4) // divide
		tempT /= temp;
	else if (locvar2 == 3) // modulo
		tempT %= temp;
	else if (locvar2 == 2) // add
		tempT += temp;
	else if (locvar2 == 1) // subtract
		tempT -= temp;
	else // replace
		tempT = temp;

	// finally, give target/tracer the new cusval!
	if (!(locvar1 >> 16)) // target
		actor->target->cusval = tempT;
	else // tracer
		actor->tracer->cusval = tempT;
}

// Function: A_CusValAction
//
// Description: Calls an action from a reference state applying custom value parameters.
//
// var1 = state # to use action from
// var2:
//      if var2 == 5, only replace new action's var2 with memory value
//      else if var2 == 4, only replace new action's var1 with memory value
//      else if var2 == 3, replace new action's var2 with custom value and var1 with memory value
//      else if var2 == 2, replace new action's var1 with custom value and var2 with memory value
//      else if var2 == 1, only replace new action's var2 with custom value
//      else if var2 == 0, only replace new action's var1 with custom value
//
void A_CusValAction(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_CusValAction", actor))
		return;
#endif

	if (locvar2 == 5)
	{
		var1 = states[locvar1].var1;
		var2 = (INT32)actor->cvmem;
	}
	else if (locvar2 == 4)
	{
		var1 = (INT32)actor->cvmem;
		var2 = states[locvar1].var2;
	}
	else if (locvar2 == 3)
	{
		var1 = (INT32)actor->cvmem;
		var2 = (INT32)actor->cusval;
	}
	else if (locvar2 == 2)
	{
		var1 = (INT32)actor->cusval;
		var2 = (INT32)actor->cvmem;
	}
	else if (locvar2 == 1)
	{
		var1 = states[locvar1].var1;
		var2 = (INT32)actor->cusval;
	}
	else
	{
		var1 = (INT32)actor->cusval;
		var2 = states[locvar1].var2;
	}

#ifdef HAVE_BLUA
	astate = &states[locvar1];
#endif
	states[locvar1].action.acp1(actor);
}

// Function: A_ForceStop
//
// Description: Actor immediately stops its current movement.
//
// var1:
//      if var1 == 0, stop x-y-z-movement
//      else, stop x-y-movement only
// var2 = unused
//
void A_ForceStop(mobj_t *actor)
{
	INT32 locvar1 = var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ForceStop", actor))
		return;
#endif

	actor->momx = actor->momy = 0;
	if (locvar1 == 0)
		actor->momz = 0;
}

// Function: A_ForceWin
//
// Description: Makes all players win the level.
//
// var1 = unused
// var2 = unused
//
void A_ForceWin(mobj_t *actor)
{
	INT32 i;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_ForceWin", actor))
		return;
#else
	(void)actor;
#endif

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && ((players[i].mo && players[i].mo->health)
		    || ((netgame || multiplayer) && (players[i].lives || players[i].continues))))
			break;
	}

	if (i == MAXPLAYERS)
		return;

	for (i = 0; i < MAXPLAYERS; i++)
		P_DoPlayerExit(&players[i]);
}

// Function: A_SpikeRetract
//
// Description: Toggles actor solid flag.
//
// var1:
//        if var1 == 0, actor no collide
//        else, actor solid
// var2 = unused
//
void A_SpikeRetract(mobj_t *actor)
{
	INT32 locvar1 = var1;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SpikeRetract", actor))
		return;
#endif

	if (actor->flags & MF_NOBLOCKMAP)
		return;

	if (locvar1 == 0)
	{
		actor->flags &= ~MF_SOLID;
		actor->flags |= MF_NOCLIPTHING;
	}
	else
	{
		actor->flags |= MF_SOLID;
		actor->flags &= ~MF_NOCLIPTHING;
	}
	if (actor->flags & MF_SOLID)
		P_CheckPosition(actor, actor->x, actor->y);
}

// Function: A_InfoState
//
// Description: Set mobj state to one predefined in mobjinfo.
//
// var1:
//        if var1 == 0, set actor to spawnstate
//        else if var1 == 1, set actor to seestate
//        else if var1 == 2, set actor to meleestate
//        else if var1 == 3, set actor to missilestate
//        else if var1 == 4, set actor to deathstate
//        else if var1 == 5, set actor to xdeathstate
//        else if var1 == 6, set actor to raisestate
// var2 = unused
//
void A_InfoState(mobj_t *actor)
{
	INT32 locvar1 = var1;
	switch (locvar1)
	{
	case 0:
		if (actor->state != &states[actor->info->spawnstate])
			P_SetMobjState(actor, actor->info->spawnstate);
		break;
	case 1:
		if (actor->state != &states[actor->info->seestate])
			P_SetMobjState(actor, actor->info->seestate);
		break;
	case 2:
		if (actor->state != &states[actor->info->meleestate])
			P_SetMobjState(actor, actor->info->meleestate);
		break;
	case 3:
		if (actor->state != &states[actor->info->missilestate])
			P_SetMobjState(actor, actor->info->missilestate);
		break;
	case 4:
		if (actor->state != &states[actor->info->deathstate])
			P_SetMobjState(actor, actor->info->deathstate);
		break;
	case 5:
		if (actor->state != &states[actor->info->xdeathstate])
			P_SetMobjState(actor, actor->info->xdeathstate);
		break;
	case 6:
		if (actor->state != &states[actor->info->raisestate])
			P_SetMobjState(actor, actor->info->raisestate);
		break;
	default:
		break;
	}
}

// Function: A_Repeat
//
// Description: Returns to state var2 until animation has been used var1 times, then continues to nextstate.
//
// var1 = repeat count
// var2 = state to return to if extravalue2 > 0
//
void A_Repeat(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_Repeat", actor))
		return;
#endif

	if ((!(actor->extravalue2)) || actor->extravalue2 > locvar1)
		actor->extravalue2 = locvar1;

	if (--actor->extravalue2 > 0)
		P_SetMobjState(actor, locvar2);
}

// Function: A_SetScale
//
// Description: Changes the scale of the actor or its target/tracer
//
// var1 = new scale (1*FRACUNIT = 100%)
// var2:
//        upper 16 bits: 0 = actor, 1 = target, 2 = tracer
//        lower 16 bits: 0 = instant change, 1 = smooth change
//
void A_SetScale(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *target;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SetScale", actor))
		return;
#endif

	if (locvar1 <= 0)
	{
		if(cv_debug)
			CONS_Printf("A_SetScale: Valid scale not specified!\n");
		return;
	}

	if ((locvar2>>16) == 1)
		target = actor->target;
	else if ((locvar2>>16) == 2)
		target = actor->tracer;
	else // default to yourself!
		target = actor;

	if (!target)
	{
		if(cv_debug)
			CONS_Printf("A_SetScale: No target!\n");
		return;
	}

	target->destscale = locvar1; // destination scale
	if (!(locvar2 & 65535))
		P_SetScale(target, locvar1); // this instantly changes current scale to var1 if used, if not destscale will alter scale to var1 anyway
}

// Function: A_RemoteDamage
//
// Description: Damages, kills or even removes either the actor or its target/tracer. Actor acts as the inflictor/source unless harming itself
//
// var1 = Mobj affected: 0 - actor, 1 - target, 2 - tracer
// var2 = Action: 0 - Damage, 1 - Kill, 2 - Remove
//
void A_RemoteDamage(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *target; // we MUST have a target
	mobj_t *source = NULL; // on the other hand we don't necessarily need a source
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_RemoteDamage", actor))
		return;
#endif
	if (locvar1 == 1)
		target = actor->target;
	else if (locvar1 == 2)
		target = actor->tracer;
	else // default to yourself!
		target = actor;

	if (locvar1 == 1 || locvar1 == 2)
		source = actor;

	if (!target)
	{
		if(cv_debug)
			CONS_Printf("A_RemoteDamage: No target!\n");
		return;
	}

	if (locvar2 == 1) // Kill mobj!
	{
		if (target->player) // players die using P_DamageMobj instead for some reason
			P_DamageMobj(target, source, source, 1, DMG_INSTAKILL);
		else
			P_KillMobj(target, source, source, 0);
	}
	else if (locvar2 == 2) // Remove mobj!
	{
		if (target->player) //don't remove players!
			return;

		P_RemoveMobj(target);
	}
	else // default: Damage mobj!
		P_DamageMobj(target, source, source, 1, 0);
}

// Function: A_HomingChase
//
// Description: Actor chases directly towards its destination object
//
// var1 = speed multiple
// var2 = destination: 0 = target, 1 = tracer
//
void A_HomingChase(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *dest;
	fixed_t dist;
	fixed_t speedmul;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_HomingChase", actor))
		return;
#endif

	if (locvar2 == 1)
		dest = actor->tracer;
	else //default
		dest = actor->target;

	if (!dest || !dest->health)
		return;

	actor->angle = R_PointToAngle2(actor->x, actor->y, dest->x, dest->y);

	dist = P_AproxDistance(P_AproxDistance(dest->x - actor->x, dest->y - actor->y), dest->z - actor->z);

	if (dist < 1)
		dist = 1;

	speedmul = FixedMul(locvar1, actor->scale);

	actor->momx = FixedMul(FixedDiv(dest->x - actor->x, dist), speedmul);
	actor->momy = FixedMul(FixedDiv(dest->y - actor->y, dist), speedmul);
	actor->momz = FixedMul(FixedDiv(dest->z - actor->z, dist), speedmul);
}

// Function: A_TrapShot
//
// Description: Fires a missile in a particular direction and angle rather than AT something, Trapgoyle-style!
//
// var1:
//        lower 16 bits = object # to fire
//        upper 16 bits = front offset
// var2:
//        lower 15 bits = vertical angle variable
//        16th bit:
//			- 0: use vertical angle variable as vertical angle in degrees
//			- 1: mimic P_SpawnXYZMissile
//				use z of actor minus z of missile as vertical distance to cover during momz calculation
//				use vertical angle variable as horizontal distance to cover during momz calculation
//        upper 16 bits = height offset
//
void A_TrapShot(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	boolean oldstyle = (locvar2 & 32768) ? true : false;
	mobjtype_t type = (mobjtype_t)(locvar1 & 65535);
	mobj_t *missile;
	INT16 frontoff = (INT16)(locvar1 >> 16);
	INT16 vertoff = (INT16)(locvar2 >> 16);
	fixed_t x, y, z;
	fixed_t speed;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_TrapShot", actor))
		return;
#endif

	x = actor->x + P_ReturnThrustX(actor, actor->angle, FixedMul(frontoff*FRACUNIT, actor->scale));
	y = actor->y + P_ReturnThrustY(actor, actor->angle, FixedMul(frontoff*FRACUNIT, actor->scale));

	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - FixedMul(vertoff*FRACUNIT, actor->scale) - FixedMul(mobjinfo[type].height, actor->scale);
	else
		z = actor->z + FixedMul(vertoff*FRACUNIT, actor->scale);

	CONS_Debug(DBG_GAMELOGIC, "A_TrapShot: missile no. = %d, front offset = %d, vertical angle = %d, z offset = %d\n",
		type, frontoff, (INT16)(locvar2 & 65535), vertoff);

	missile = P_SpawnMobj(x, y, z, type);

	if (actor->eflags & MFE_VERTICALFLIP)
		missile->flags2 |= MF2_OBJECTFLIP;

	missile->destscale = actor->scale;
	P_SetScale(missile, actor->scale);

	if (missile->info->seesound)
		S_StartSound(missile, missile->info->seesound);

	P_SetTarget(&missile->target, actor);
	missile->angle = actor->angle;

	speed = FixedMul(missile->info->speed, missile->scale);

	if (oldstyle)
	{
		missile->momx = FixedMul(FINECOSINE(missile->angle>>ANGLETOFINESHIFT), speed);
		missile->momy = FixedMul(FINESINE(missile->angle>>ANGLETOFINESHIFT), speed);
		// The below line basically mimics P_SpawnXYZMissile's momz calculation.
		missile->momz = (actor->z + ((actor->eflags & MFE_VERTICALFLIP) ? actor->height : 0) - z) / ((fixed_t)(locvar2 & 32767)*FRACUNIT / speed);
		P_CheckMissileSpawn(missile);
	}
	else
	{
		angle_t vertang = FixedAngle(((INT16)(locvar2 & 32767))*FRACUNIT);
		if (actor->eflags & MFE_VERTICALFLIP)
				vertang = InvAngle(vertang); // flip firing angle
		missile->momx = FixedMul(FINECOSINE(vertang>>ANGLETOFINESHIFT), FixedMul(FINECOSINE(missile->angle>>ANGLETOFINESHIFT), speed));
		missile->momy = FixedMul(FINECOSINE(vertang>>ANGLETOFINESHIFT), FixedMul(FINESINE(missile->angle>>ANGLETOFINESHIFT), speed));
		missile->momz = FixedMul(FINESINE(vertang>>ANGLETOFINESHIFT), speed);
	}
}

// Function: A_VileTarget
//
// Description: Spawns an object directly on the target, and sets this object as the actor's tracer.
//              Originally used by Archviles to summon a pillar of hellfire, hence the name.
//
// var1 = mobj to spawn
// var2 = If 0, target only the actor's target. Else, target every player, period.
//
void A_VileTarget(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *fog;
	mobjtype_t fogtype;
	INT32 i;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_VileTarget", actor))
		return;
#endif

	if (!actor->target)
		return;

	A_FaceTarget(actor);

	// Determine object to spawn
	if (locvar1 <= 0 || locvar1 >= NUMMOBJTYPES)
		fogtype = MT_CYBRAKDEMON_TARGET_RETICULE;
	else
		fogtype = (mobjtype_t)locvar1;

	if (!locvar2)
	{
		fog = P_SpawnMobj(actor->target->x,
							actor->target->y,
							actor->target->z + ((actor->target->eflags & MFE_VERTICALFLIP) ? actor->target->height - mobjinfo[fogtype].height : 0),
							fogtype);
		if (actor->target->eflags & MFE_VERTICALFLIP)
		{
			fog->eflags |= MFE_VERTICALFLIP;
			fog->flags2 |= MF2_OBJECTFLIP;
		}
		fog->destscale = actor->target->scale;
		P_SetScale(fog, fog->destscale);

		P_SetTarget(&actor->tracer, fog);
		P_SetTarget(&fog->target, actor);
		P_SetTarget(&fog->tracer, actor->target);
		A_VileFire(fog);
	}
	else
	{
		// Our "Archvile" here is actually Oprah. "YOU GET A TARGET! YOU GET A TARGET! YOU ALL GET A TARGET!"
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo)
				continue;

			if (!players[i].mo->health)
				continue;

			fog = P_SpawnMobj(players[i].mo->x,
							players[i].mo->y,
							players[i].mo->z + ((players[i].mo->eflags & MFE_VERTICALFLIP) ? players[i].mo->height - mobjinfo[fogtype].height : 0),
							fogtype);
			if (players[i].mo->eflags & MFE_VERTICALFLIP)
			{
				fog->eflags |= MFE_VERTICALFLIP;
				fog->flags2 |= MF2_OBJECTFLIP;
			}
			fog->destscale = players[i].mo->scale;
			P_SetScale(fog, fog->destscale);

			if (players[i].mo == actor->target) // We only care to track the fog targeting who we REALLY hate right now
				P_SetTarget(&actor->tracer, fog);
			P_SetTarget(&fog->target, actor);
			P_SetTarget(&fog->tracer, players[i].mo);
			A_VileFire(fog);
		}
	}
}

// Function: A_VileAttack
//
// Description: Instantly hurts the actor's target, if it's in the actor's line of sight.
//              Originally used by Archviles to cause explosions where their hellfire pillars were, hence the name.
//
// var1 = sound to play
// var2:
//		Lower 16 bits = optional explosion object
//		Upper 16 bits = If 0, attack only the actor's target. Else, attack all the players. All of them.
//
void A_VileAttack(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	sfxenum_t soundtoplay;
	mobjtype_t explosionType = MT_NULL;
	mobj_t *fire;
	INT32 i;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_VileAttack", actor))
		return;
#endif

	if (!actor->target)
		return;

	A_FaceTarget(actor);

	if (locvar1 <= 0 || locvar1 >= NUMSFX)
		soundtoplay = sfx_brakrx;
	else
		soundtoplay = (sfxenum_t)locvar1;

	if ((locvar2 & 0xFFFF) > 0 && (locvar2 & 0xFFFF) <= NUMMOBJTYPES)
	{
		explosionType = (mobjtype_t)(locvar2 & 0xFFFF);
	}

	if (!(locvar2 & 0xFFFF0000)) {
		if (!P_CheckSight(actor, actor->target))
			return;

		S_StartSound(actor, soundtoplay);
		P_DamageMobj(actor->target, actor, actor, 1, 0);
		//actor->target->momz = 1000*FRACUNIT/actor->target->info->mass; // How id did it
		actor->target->momz += FixedMul(10*FRACUNIT, actor->scale)*P_MobjFlip(actor->target); // How we're doing it
		if (explosionType != MT_NULL)
		{
			P_SpawnMobj(actor->target->x, actor->target->y, actor->target->z, explosionType);
		}

		// Extra attack. This was for additional damage in Doom. Doesn't really belong in SRB2, but the heck with it, it's here anyway.
		fire = actor->tracer;

		if (!fire)
			return;

		// move the fire between the vile and the player
		//fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
		//fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);
		P_TeleportMove(fire,
						actor->target->x - P_ReturnThrustX(fire, actor->angle, FixedMul(24*FRACUNIT, fire->scale)),
						actor->target->y - P_ReturnThrustY(fire, actor->angle, FixedMul(24*FRACUNIT, fire->scale)),
						fire->z);
		P_RadiusAttack(fire, actor, 70*FRACUNIT);
	}
	else
	{
		// Oprahvile strikes again, but this time, she brings HOT PAIN
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo)
				continue;

			if (!players[i].mo->health)
				continue;

			if (!P_CheckSight(actor, players[i].mo))
				continue;

			S_StartSound(actor, soundtoplay);
			P_DamageMobj(players[i].mo, actor, actor, 1, 0);
			//actor->target->momz = 1000*FRACUNIT/actor->target->info->mass; // How id did it
			players[i].mo->momz += FixedMul(10*FRACUNIT, actor->scale)*P_MobjFlip(players[i].mo); // How we're doing it
			if (explosionType != MT_NULL)
			{
				P_SpawnMobj(players[i].mo->x, players[i].mo->y, players[i].mo->z, explosionType);
			}

			// Extra attack. This was for additional damage in Doom. Doesn't really belong in SRB2, but the heck with it, it's here anyway.
			// However, it ONLY applies to the actor's target. Nobody else matters!
			if (actor->target != players[i].mo)
				continue;

			fire = actor->tracer;

			if (!fire)
				continue;

			// move the fire between the vile and the player
			//fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
			//fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);
			P_TeleportMove(fire,
							actor->target->x - P_ReturnThrustX(fire, actor->angle, FixedMul(24*FRACUNIT, fire->scale)),
							actor->target->y - P_ReturnThrustY(fire, actor->angle, FixedMul(24*FRACUNIT, fire->scale)),
							fire->z);
			P_RadiusAttack(fire, actor, 70*FRACUNIT);
		}
	}

}

// Function: A_VileFire
//
// Description: Kind of like A_CapeChase; keeps this object in front of its tracer, unless its target can't see it.
//				Originally used by Archviles to keep their hellfire pillars on top of the player, hence the name (although it was just "A_Fire" there; added "Vile" to make it more specific).
//				Added some functionality to optionally draw a line directly to the enemy doing the targetting. Y'know, to hammer things in a bit.
//
// var1 = sound to play
// var2:
//		Lower 16 bits = mobj to spawn (0 doesn't spawn a line at all)
//		Upper 16 bits = # to spawn (default is 8)
//
void A_VileFire(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	mobj_t *dest;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_VileFire", actor))
		return;
#endif

	dest = actor->tracer;
	if (!dest)
		return;

	// don't move it if the vile lost sight
	if (!P_CheckSight(actor->target, dest))
		return;

	// keep to same scale and gravity as tracer ALWAYS
	actor->destscale = dest->scale;
	P_SetScale(actor, actor->destscale);
	if (dest->eflags & MFE_VERTICALFLIP)
	{
		actor->eflags |= MFE_VERTICALFLIP;
		actor->flags2 |= MF2_OBJECTFLIP;
	}
	else
	{
		actor->eflags &= ~MFE_VERTICALFLIP;
		actor->flags2 &= ~MF2_OBJECTFLIP;
	}

	P_UnsetThingPosition(actor);
	actor->x = dest->x + P_ReturnThrustX(actor, dest->angle, FixedMul(24*FRACUNIT, actor->scale));
	actor->y = dest->y + P_ReturnThrustY(actor, dest->angle, FixedMul(24*FRACUNIT, actor->scale));
	actor->z = dest->z + ((actor->eflags & MFE_VERTICALFLIP) ? dest->height-actor->height : 0);
	P_SetThingPosition(actor);

	// Play sound, if one's specified
	if (locvar1 > 0 && locvar1 < NUMSFX)
		S_StartSound(actor, (sfxenum_t)locvar1);

	// Now draw the line to the actor's target
	if (locvar2 & 0xFFFF)
	{
		mobjtype_t lineMobj;
		UINT16 numLineMobjs;
		fixed_t distX;
		fixed_t distY;
		fixed_t distZ;
		UINT16 i;

		lineMobj = (mobjtype_t)(locvar2 & 0xFFFF);
		numLineMobjs = (UINT16)(locvar2 >> 16);
		if (numLineMobjs == 0) {
			numLineMobjs = 8;
		}

		// Get distance for each step
		distX = (actor->target->x - actor->x) / numLineMobjs;
		distY = (actor->target->y - actor->y) / numLineMobjs;
		distZ = ((actor->target->z + FixedMul(actor->target->height/2, actor->target->scale)) - (actor->z + FixedMul(actor->height/2, actor->scale))) / numLineMobjs;

		for (i = 1; i <= numLineMobjs; i++)
		{
			P_SpawnMobj(actor->x + (distX * i), actor->y + (distY * i), actor->z + (distZ * i) + FixedMul(actor->height/2, actor->scale), lineMobj);
		}
	}
}

// Function: A_BrakChase
//
// Description: Chase after your target, but speed and attack are tied to health.
//
// Every time this is called, generate a random number from a 1/4 to 3/4 of mobj's spawn health.
// If health is above that value, use missilestate to attack.
// If health is at or below that value, use meleestate to attack (default to missile state if not available).
//
// Likewise, state will linearly speed up as health goes down.
// Upper bound will be the frame's normal length.
// Lower bound defaults to 1 tic (technically 0, but we round up), unless a lower bound is specified in var1.
//
// var1 = lower-bound of frame length, in tics
// var2 = optional sound to play
//
void A_BrakChase(mobj_t *actor)
{
	INT32 delta;
	INT32 lowerbound;
	INT32 newtics;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BrakChase", actor))
		return;
#endif

	// Set new tics NOW, in case the state changes while we're doing this and we try applying this to the painstate or something silly
	if (actor->tics > 1 && locvar1 < actor->tics) // Not much point, otherwise
	{
		if (locvar1 < 0)
			lowerbound = 0;
		else
			lowerbound = locvar1;

		newtics = (((actor->tics - lowerbound) * actor->health) / actor->info->spawnhealth) + lowerbound;
		if (newtics < 1)
			newtics = 1;

		actor->tics = newtics;
	}

	if (actor->reactiontime)
	{
		actor->reactiontime--;
		if (actor->reactiontime == 0 && actor->type == MT_CYBRAKDEMON)
			S_StartSound(0, sfx_bewar1 + P_RandomKey(4));
	}

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANGLE_45;
		else if (delta < 0)
			actor->angle += ANGLE_45;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false, 0))
			return; // got a new target

		P_SetMobjStateNF(actor, actor->info->spawnstate);
		return;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// Check if we can attack
	if (P_CheckMissileRange(actor) && !actor->movecount)
	{
		// Check if we should use "melee" attack first. (Yes, this still runs outside of melee range. Quiet, you.)
		if (actor->info->meleestate
			&& actor->health <= P_RandomRange(actor->info->spawnhealth/4, (actor->info->spawnhealth * 3)/4)) // Guaranteed true if <= 1/4 health, guaranteed false if > 3/4 health
		{
			if (actor->info->attacksound)
				S_StartAttackSound(actor, actor->info->attacksound);

			P_SetMobjState(actor, actor->info->meleestate);
			actor->flags2 |= MF2_JUSTATTACKED;
			return;
		}
		// Else, check for missile attack.
		else if (actor->info->missilestate)
		{
			P_SetMobjState(actor, actor->info->missilestate);
			actor->flags2 |= MF2_JUSTATTACKED;
			return;
		}
	}

	// possibly choose another target
	if (multiplayer && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false, 0))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor, actor->info->speed))
		P_NewChaseDir(actor);

	// Optionally play a sound effect
	if (locvar2 > 0 && locvar2 < NUMSFX)
		S_StartSound(actor, (sfxenum_t)locvar2);

	// make active sound
	if (actor->type != MT_CYBRAKDEMON && actor->info->activesound && P_RandomChance(3*FRACUNIT/256))
	{
		S_StartSound(actor, actor->info->activesound);
	}
}

// Function: A_BrakFireShot
//
// Description: Shoot an object at your target, offset to match where Brak's gun is.
// Also, sets Brak's reaction time; behaves normally otherwise.
//
// var1 = object # to shoot
// var2 = unused
//
void A_BrakFireShot(mobj_t *actor)
{
	fixed_t x, y, z;
	INT32 locvar1 = var1;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BrakFireShot", actor))
		return;
#endif
	if (!actor->target)
		return;

	A_FaceTarget(actor);

	x = actor->x
		+ P_ReturnThrustX(actor, actor->angle, FixedMul(64*FRACUNIT, actor->scale))
		+ P_ReturnThrustX(actor, actor->angle+ANGLE_270, FixedMul(32*FRACUNIT, actor->scale));
	y = actor->y
		+ P_ReturnThrustY(actor, actor->angle, FixedMul(64*FRACUNIT, actor->scale))
		+ P_ReturnThrustY(actor, actor->angle+ANGLE_270, FixedMul(32*FRACUNIT, actor->scale));
	if (actor->eflags & MFE_VERTICALFLIP)
		z = actor->z + actor->height - FixedMul(144*FRACUNIT, actor->scale);
	else
		z = actor->z + FixedMul(144*FRACUNIT, actor->scale);

	P_SpawnXYZMissile(actor, actor->target, locvar1, x, y, z);

	if (!(actor->flags & MF_BOSS))
	{
		if (ultimatemode)
			actor->reactiontime = actor->info->reactiontime*TICRATE;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	}
}

// Function: A_BrakLobShot
//
// Description: Lobs an object at the floor about a third of the way toward your target.
//				Implication is it'll bounce the rest of the way.
//				(You can also just aim straight at the target, but whatever)
//				Formula grabbed from http://en.wikipedia.org/wiki/Trajectory_of_a_projectile#Angle_required_to_hit_coordinate_.28x.2Cy.29
//
// var1 = object # to lob
// var2:
//		Lower 16 bits: height offset to shoot from, from the actor's bottom (none that "airtime" malarky)
//		Upper 16 bits: if 0, aim 1/3 of the way. Else, aim directly at target.
//

void A_BrakLobShot(mobj_t *actor)
{
	fixed_t v; // Velocity to shoot object
	fixed_t a1, a2, aToUse; // Velocity squared
	fixed_t g; // Gravity
	fixed_t x; // Horizontal difference
	INT32 x_int; // x! But in integer form!
	fixed_t y; // Vertical difference (yes that's normally z in SRB2 shut up)
	INT32 y_int; // y! But in integer form!
	INT32 intHypotenuse; // x^2 + y^2. Frequently overflows fixed point, hence why we need integers proper.
	fixed_t fixedHypotenuse; // However, we can work around that and still get a fixed-point number.
	angle_t theta; // Angle of attack
	mobjtype_t typeOfShot;
	mobj_t *shot; // Object to shoot
	fixed_t newTargetX; // If not aiming directly
	fixed_t newTargetY; // If not aiming directly
	INT32 locvar1 = var1;
	INT32 locvar2 = var2 & 0x0000FFFF;
	INT32 aimDirect = var2 & 0xFFFF0000;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_BrakLobShot", actor))
		return;
#endif

	if (!actor->target)
		return; // Don't even bother if we've got nothing to aim at.

	// Look up actor's current gravity situation
	if (actor->subsector->sector->gravity)
		g = FixedMul(gravity,(FixedDiv(*actor->subsector->sector->gravity>>FRACBITS, 1000)));
	else
		g = gravity;

	// Look up distance between actor and its target
	x = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);
	if (!aimDirect)
	{
		// Distance should actually be a third of the way over
		x = FixedDiv(x, 3<<FRACBITS);
		newTargetX = actor->x + P_ReturnThrustX(actor, actor->angle, x);
		newTargetY = actor->y + P_ReturnThrustY(actor, actor->angle, x);
		x = P_AproxDistance(newTargetX - actor->x, newTargetY - actor->y);
		// Look up height difference between actor and the ground 1/3 of the way to its target
		y = P_FloorzAtPos(newTargetX, newTargetY, actor->target->z, actor->target->height) - (actor->z + FixedMul(locvar2*FRACUNIT, actor->scale));
	}
	else
	{
		// Look up height difference between actor and its target
		y = actor->target->z - (actor->z + FixedMul(locvar2*FRACUNIT, actor->scale));
	}

	// Get x^2 + y^2. Have to do it in a roundabout manner, because this overflows fixed_t way too easily otherwise.
	x_int = x>>FRACBITS;
	y_int = y>>FRACBITS;
	intHypotenuse = (x_int*x_int) + (y_int*y_int);
	fixedHypotenuse = FixedSqrt(intHypotenuse) *256;

	// a = g(y+/-sqrt(x^2+y^2)). a1 can be +, a2 can be -.
	a1 = FixedMul(g,y+fixedHypotenuse);
	a2 = FixedMul(g,y-fixedHypotenuse);

	// Determine which one isn't actually an imaginary number (or the smaller of the two, if both are real), and use that for v.
	if (a1 < 0 || a2 < 0)
	{
		if (a1 < 0 && a2 < 0)
		{
			//Somehow, v^2 is negative in both cases. v is therefore imaginary and something is horribly wrong. Abort!
			return;
		}
		// Just find which one's NOT negative, and use that
		aToUse = max(a1,a2);
	}
	else
	{
		// Both are positive; use whichever's smaller so it can decay faster
		aToUse = min(a1,a2);
	}
	v = FixedSqrt(aToUse);
	// Okay, so we know the velocity. Let's actually find theta.
	// We can cut the "+/- sqrt" part out entirely, since v was calculated specifically for it to equal zero. So:
	//theta = tantoangle[FixedDiv(aToUse,FixedMul(g,x)) >> DBITS];
	theta = tantoangle[SlopeDiv(aToUse,FixedMul(g,x))];

	// Okay, complicated math done. Let's fire our object already, sheesh.
	A_FaceTarget(actor);
	if (locvar1 <= 0 || locvar1 >= NUMMOBJTYPES)
		typeOfShot = MT_CANNONBALL;
	else typeOfShot = (mobjtype_t)locvar1;
	shot = P_SpawnMobj(actor->x, actor->y, actor->z + FixedMul(locvar2*FRACUNIT, actor->scale), typeOfShot);
	if (shot->info->seesound)
		S_StartSound(shot, shot->info->seesound);
	P_SetTarget(&shot->target, actor); // where it came from

	shot->angle = actor->angle;

	// Horizontal axes first. First parameter is initial horizontal impulse, second is to correct its angle.
	shot->momx = FixedMul(FixedMul(v, FINECOSINE(theta >> ANGLETOFINESHIFT)), FINECOSINE(shot->angle >> ANGLETOFINESHIFT));
	shot->momy = FixedMul(FixedMul(v, FINECOSINE(theta >> ANGLETOFINESHIFT)), FINESINE(shot->angle >> ANGLETOFINESHIFT));
	// Then the vertical axis. No angle-correction needed here.
	shot->momz = FixedMul(v, FINESINE(theta >> ANGLETOFINESHIFT));
	// I hope that's all that's needed, ugh
}

// Function: A_NapalmScatter
//
// Description: Scatters a specific number of projectiles around in a circle.
//				Intended for use with objects that are affected by gravity; would be kind of silly otherwise.
//
// var1:
//		Lower 16 bits: object # to lob (TODO: come up with a default)
//		Upper 16 bits: Number to lob (default 8)
// var2:
//		Lower 16 bits: distance to toss them (No default - 0 does just that - but negatives will revert to 128)
//		Upper 16 bits: airtime in tics (default 16)
//
void A_NapalmScatter(mobj_t *actor)
{
	mobjtype_t typeOfShot = var1 & 0x0000FFFF; // Type
	INT32 numToShoot = (var1 & 0xFFFF0000) >> 16; // How many
	fixed_t distance = (var2 & 0x0000FFFF) << FRACBITS; // How far
	fixed_t airtime = var2 & 0xFFFF0000; // How long until impact (assuming no obstacles)
	fixed_t vx; // Horizontal momentum
	fixed_t vy; // Vertical momentum
	fixed_t g; // Gravity
	INT32 i; // for-loop cursor
	mobj_t *mo; // each and every spawned napalm burst

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_NapalmScatter", actor))
		return;
#endif

	// Some quick sanity-checking
	if (typeOfShot >= NUMMOBJTYPES) // I'd add a <0 check, too, but 0x0000FFFF isn't negative in this case
		typeOfShot = MT_NULL;
	if (numToShoot <= 0) // Presumably you forgot to set var1 up; else, why are you calling this to shoot nothing?
		numToShoot = 8;
	else if (numToShoot > 8192) // If you seriously need this many objects spawned, stop and ask yourself "Why am I doing this?"
		numToShoot = 8192;
	if (distance < 0) // Presumably you thought this was an unsigned integer, you naive fool
		distance = 32767<<FRACBITS;
	if (airtime <= 0) // Same deal as distance I guess
		airtime = 16<<FRACBITS;

	// Look up actor's current gravity situation
	if (actor->subsector->sector->gravity)
		g = FixedMul(gravity,(FixedDiv(*actor->subsector->sector->gravity>>FRACBITS, 1000)));
	else
		g = gravity;

	// vy = (g*(airtime-1))/2
	vy = FixedMul(g,(airtime-(1<<FRACBITS)))>>1;
	// vx = distance/airtime
	vx = FixedDiv(distance, airtime);

	for (i = 0; i<numToShoot; i++)
	{
		const angle_t fa = (i*FINEANGLES/numToShoot) & FINEMASK;

		mo = P_SpawnMobj(actor->x, actor->y, actor->z, typeOfShot);
		P_SetTarget(&mo->target, actor->target); // Transfer target so Brak doesn't hit himself like an idiot

		mo->angle = fa << ANGLETOFINESHIFT;
		mo->momx = FixedMul(FINECOSINE(fa),vx);
		mo->momy = FixedMul(FINESINE(fa),vx);
		mo->momz = vy;
	}
}

// Function: A_SpawnFreshCopy
//
// Description: Spawns a copy of the mobj. x, y, z, angle, scale, target and tracer carry over; everything else starts anew.
//				Mostly writing this because I want to do multiple actions to pass these along in a single frame instead of several.
//
// var1 = unused
// var2 = unused
//
void A_SpawnFreshCopy(mobj_t *actor)
{
	mobj_t *newObject;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_SpawnFreshCopy", actor))
		return;
#endif

	newObject = P_SpawnMobj(actor->x, actor->y, actor->z, actor->type);
	newObject->angle = actor->angle;
	P_SetScale(newObject, actor->scale);
	newObject->destscale = actor->destscale;
	P_SetTarget(&newObject->target, actor->target);
	P_SetTarget(&newObject->tracer, actor->tracer);

	if (newObject->info->seesound)
		S_StartSound(newObject, newObject->info->seesound);
}

// Internal Flicky spawning function.
mobj_t *P_InternalFlickySpawn(mobj_t *actor, mobjtype_t flickytype, fixed_t momz, boolean lookforplayers)
{
	mobj_t *flicky;

	if (!flickytype)
	{
		if (!mapheaderinfo[gamemap-1] || !mapheaderinfo[gamemap-1]->numFlickies) // No mapheader, no shoes, no service.
			return NULL;
		else
		{
			INT32 prandom = P_RandomKey(mapheaderinfo[gamemap-1]->numFlickies);
			flickytype = mapheaderinfo[gamemap-1]->flickies[prandom];
		}
	}

	flicky = P_SpawnMobjFromMobj(actor, 0, 0, 0, flickytype);
	flicky->angle = actor->angle;

	if (flickytype == MT_SEED)
		flicky->z += P_MobjFlip(actor)*(actor->height - flicky->height)/2;

	if (actor->eflags & MFE_UNDERWATER)
		momz = FixedDiv(momz, FixedSqrt(3*FRACUNIT));

	P_SetObjectMomZ(flicky, momz, false);
	flicky->movedir = (P_RandomChance(FRACUNIT/2) ?  -1 : 1);
	flicky->fuse = P_RandomRange(595, 700);	// originally 300, 350
	flicky->threshold = 0;

	if (lookforplayers)
		P_LookForPlayers(flicky, true, false, 0);

	return flicky;
}

// Function: A_FlickySpawn
//
// Description: Flicky spawning function.
//
// var1:
//		lower 16 bits: if 0, spawns random flicky based on level header. Else, spawns the designated thing type.
//		upper 16 bits: if 0, no sound is played. Else, A_Scream is called.
// var2 = upwards thrust for spawned flicky. If zero, default value is provided.
//
void A_FlickySpawn(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickySpawn", actor))
		return;
#endif

	if (locvar1 >> 16) {
		A_Scream(actor); // A shortcut for the truly lazy.
		locvar1 &= 65535;
	}

	P_InternalFlickySpawn(actor, locvar1, ((locvar2) ? locvar2 : 8*FRACUNIT), true);
}

// Internal Flicky bubbling function.
void P_InternalFlickyBubble(mobj_t *actor)
{
	if (actor->eflags & MFE_UNDERWATER)
	{
		mobj_t *overlay;

		if (!((actor->z + 3*actor->height/2) < actor->watertop) || !mobjinfo[actor->type].raisestate || actor->tracer)
			return;

		overlay = P_SpawnMobj(actor->x, actor->y, actor->z, MT_OVERLAY);
		P_SetMobjStateNF(overlay, mobjinfo[actor->type].raisestate);
		P_SetTarget(&actor->tracer, overlay);
		P_SetTarget(&overlay->target, actor);
		return;
	}

	if (!actor->tracer || P_MobjWasRemoved(actor->tracer))
		return;

	P_RemoveMobj(actor->tracer);
	P_SetTarget(&actor->tracer, NULL);
}

// Function: A_FlickyAim
//
// Description: Flicky aiming function.
//
// var1 = how far around the target (in angle constants) the flicky should look
// var2 = distance from target to aim for
//
void A_FlickyAim(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	boolean flickyhitwall = false;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyAim", actor))
		return;
#endif

	if (actor->momx == actor->momy && actor->momy == 0)
		flickyhitwall = true;

	P_InternalFlickyBubble(actor);
	P_InstaThrust(actor, 0, 0);

	if (!actor->target)
	{
		P_LookForPlayers(actor, true, false, 0);
		actor->angle = P_RandomKey(36)*ANG10;
		return;
	}

	if (actor->fuse > 2*TICRATE)
	{
		angle_t posvar;
		fixed_t chasevar, chasex, chasey;

		if (flickyhitwall)
			actor->movedir *= -1;

		posvar = ((R_PointToAngle2(actor->target->x, actor->target->y, actor->x, actor->y) + actor->movedir*locvar1) >> ANGLETOFINESHIFT) & FINEMASK;
		chasevar = FixedSqrt(max(FRACUNIT, P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y) - locvar2)) + locvar2;

		chasex = actor->target->x + FixedMul(FINECOSINE(posvar), chasevar);
		chasey = actor->target->y + FixedMul(FINESINE(posvar), chasevar);

		if (P_AproxDistance(chasex - actor->x, chasey - actor->y))
			actor->angle = R_PointToAngle2(actor->x, actor->y, chasex, chasey);
	}
	else if (flickyhitwall)
	{
		actor->angle += ANGLE_180;
		actor->threshold = 0;
	}
}

//Internal Flicky flying function. Also usuable as an underwater swim thrust.
void P_InternalFlickyFly(mobj_t *actor, fixed_t flyspeed, fixed_t targetdist, fixed_t chasez)
{
	angle_t vertangle;

	flyspeed = FixedMul(flyspeed, actor->scale);
	actor->flags |= MF_NOGRAVITY;

	var1 = ANG30;
	var2 = 32*FRACUNIT;
	A_FlickyAim(actor);

	chasez *= 8;
	if (!actor->target || !(actor->fuse > 2*TICRATE))
		chasez += ((actor->eflags & MFE_VERTICALFLIP) ? actor->ceilingz - 24*FRACUNIT : actor->floorz + 24*FRACUNIT);
	else
	{
		fixed_t add = actor->target->z + (actor->target->height - actor->height)/2;
		if (add > (actor->ceilingz - 24*actor->scale - actor->height))
			add = actor->ceilingz - 24*actor->scale - actor->height;
		else if (add < (actor->floorz + 24*actor->scale))
			add = actor->floorz + 24*actor->scale;
		chasez += add;
	}

	if (!targetdist)
		targetdist = 16*FRACUNIT; //Default!

	if (actor->target && abs(chasez - actor->z) > targetdist)
		targetdist = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);

	vertangle = (R_PointToAngle2(0, actor->z, targetdist, chasez) >> ANGLETOFINESHIFT) & FINEMASK;
	P_InstaThrust(actor, actor->angle, FixedMul(FINECOSINE(vertangle), flyspeed));
	actor->momz = FixedMul(FINESINE(vertangle), flyspeed);
}

// Function: A_FlickyFly
//
// Description: Flicky flying function.
//
// var1 = how fast to fly
// var2 = how far ahead the target should be considered
//
void A_FlickyFly(mobj_t *actor)
{
	// We're not setting up locvars here - it passes var1 and var2 through to P_InternalFlickyFly instead.
	//INT32 locvar1 = var1;
	//INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyFly", actor))
		return;
#endif
	P_InternalFlickyFly(actor, var1, var2,
	FINECOSINE((((actor->fuse % 36) * ANG10) >> ANGLETOFINESHIFT) & FINEMASK)
	);
}

// Function: A_FlickySoar
//
// Description: Flicky soaring function - specific to puffin.
//
// var1 = how fast to fly
// var2 = how far ahead the target should be considered
//
void A_FlickySoar(mobj_t *actor)
{
	// We're not setting up locvars here - it passes var1 and var2 through to P_InternalFlickyFly instead.
	//INT32 locvar1 = var1;
	//INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickySoar", actor))
		return;
#endif
	P_InternalFlickyFly(actor, var1, var2,
	2*(FRACUNIT/2 - abs(FINECOSINE((((actor->fuse % 144) * 5*ANG1/2) >> ANGLETOFINESHIFT) & FINEMASK)))
	);

	if (P_MobjFlip(actor)*actor->momz > 0 && actor->frame == 1 && actor->sprite == SPR_FL10)
		actor->frame = 3;
}

//Function: A_FlickyCoast
//
// Description: Flicky swim-coasting function.
//
// var1 = speed to change state upon reaching
// var2 = state to change to upon slowing down
// the spawnstate of the mobj = state to change to when above water
//
void A_FlickyCoast(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyCoast", actor))
		return;
#endif
	if (actor->eflags & MFE_UNDERWATER)
	{
		actor->momx = (11*actor->momx)/12;
		actor->momy = (11*actor->momy)/12;
		actor->momz = (11*actor->momz)/12;

		if (P_AproxDistance(P_AproxDistance(actor->momx, actor->momy), actor->momz) < locvar1)
			P_SetMobjState(actor, locvar2);

		return;
	}

	actor->flags &= ~MF_NOGRAVITY;
	P_SetMobjState(actor, mobjinfo[actor->type].spawnstate);
}

// Internal Flicky hopping function.
void P_InternalFlickyHop(mobj_t *actor, fixed_t momz, fixed_t momh, angle_t angle)
{
	if (((!(actor->eflags & MFE_VERTICALFLIP) && actor->z <= actor->floorz)
	|| ((actor->eflags & MFE_VERTICALFLIP) && actor->z + actor->height >= actor->ceilingz)))
	{
		if (momz)
		{
			if (actor->eflags & MFE_UNDERWATER)
				momz = FixedDiv(momz, FixedSqrt(3*FRACUNIT));
			P_SetObjectMomZ(actor, momz, false);
		}
		P_InstaThrust(actor, angle, FixedMul(momh, actor->scale));
	}
}

// Function: A_FlickyHop
//
// Description: Flicky hopping function.
//
// var1 = vertical thrust
// var2 = horizontal thrust
//
void A_FlickyHop(mobj_t *actor)
{
	// We're not setting up locvars here - it passes var1 and var2 through to P_InternalFlickyHop instead.
	//INT32 locvar1 = var1;
	//INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyHop", actor))
		return;
#endif
	P_InternalFlickyHop(actor, var1, var2, actor->angle);
}

// Function: A_FlickyFlounder
//
// Description: Flicky floundering function.
//
// var1 = intended vertical thrust
// var2 = intended horizontal thrust
//
void A_FlickyFlounder(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
	angle_t hopangle;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyFlounder", actor))
		return;
#endif
	locvar1 *= (P_RandomKey(2) + 1);
	locvar2 *= (P_RandomKey(2) + 1);
	hopangle = (actor->angle + (P_RandomKey(9) - 4)*ANG2);
	P_InternalFlickyHop(actor, locvar1, locvar2, hopangle);
}

// Function: A_FlickyCheck
//
// Description: Flicky airtime check function.
//
// var1 = state to change to upon touching the floor
// var2 = state to change to upon falling
// the meleestate of the mobj = state to change to when underwater
//
void A_FlickyCheck(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyCheck", actor))
		return;
#endif
	if (locvar2 && P_MobjFlip(actor)*actor->momz < 1)
		P_SetMobjState(actor, locvar2);
	else if (locvar1 && ((!(actor->eflags & MFE_VERTICALFLIP) && actor->z <= actor->floorz)
	|| ((actor->eflags & MFE_VERTICALFLIP) && actor->z + actor->height >= actor->ceilingz)))
		P_SetMobjState(actor, locvar1);
	else if (mobjinfo[actor->type].meleestate && (actor->eflags & MFE_UNDERWATER))
		P_SetMobjState(actor, mobjinfo[actor->type].meleestate);
	P_InternalFlickyBubble(actor);
}

// Function: A_FlickyHeightCheck
//
// Description: Flicky height check function.
//
// var1 = state to change to when falling below height relative to target
// var2 = height relative to target to change state at
//
void A_FlickyHeightCheck(mobj_t *actor)
{
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyHeightCheck", actor))
		return;
#endif
	if (locvar1 && actor->target && P_MobjFlip(actor)*actor->momz < 1
	&& ((P_MobjFlip(actor)*((actor->z + actor->height/2) - (actor->target->z + actor->target->height/2)) < locvar2)
	|| (actor->z - actor->height < actor->floorz) || (actor->z + 2*actor->height > actor->ceilingz)))
		P_SetMobjState(actor, locvar1);
	P_InternalFlickyBubble(actor);
}

// Function: A_FlickyFlutter
//
// Description: Flicky fluttering function - specific to chicken.
//
// var1 = state to change to upon touching the floor
// var2 = state to change to upon falling
// the meleestate of the mobj = state to change to when underwater
//
void A_FlickyFlutter(mobj_t *actor)
{
	// We're not setting up locvars here - it passes var1 and var2 through to A_FlickyCheck instead.
	//INT32 locvar1 = var1;
	//INT32 locvar2 = var2;
#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlickyFlutter", actor))
		return;
#endif
	A_FlickyCheck(actor);

	var1 = ANG30;
	var2 = 32*FRACUNIT;
	A_FlickyAim(actor);

	P_InstaThrust(actor, actor->angle, 2*actor->scale);
	if (P_MobjFlip(actor)*actor->momz < -FRACUNIT/2)
		actor->momz = -P_MobjFlip(actor)*actor->scale/2;
}

#undef FLICKYHITWALL

// Function: A_FlameParticle
//
// Description: Creates the mobj's painchance at a random position around the object's radius.
//
// var1 = momz of particle.
// var2 = chance of particle spawn
//
void A_FlameParticle(mobj_t *actor)
{
	mobjtype_t type = (mobjtype_t)(mobjinfo[actor->type].painchance);
	fixed_t rad, hei;
	mobj_t *particle;
	INT32 locvar1 = var1;
	INT32 locvar2 = var2;

#ifdef HAVE_BLUA
	if (LUA_CallAction("A_FlameParticle", actor))
		return;
#endif

	if (!P_RandomChance(locvar2))
		return;

	if (!type)
		return;

	rad = 2*actor->radius>>FRACBITS;
	hei = actor->height>>FRACBITS;
	particle = P_SpawnMobjFromMobj(actor,
		P_RandomRange(rad, -rad)<<FRACBITS,
		P_RandomRange(rad, -rad)<<FRACBITS,
		P_RandomRange(hei/2, hei)<<FRACBITS,
		type);
	P_SetObjectMomZ(particle, locvar1<<FRACBITS, false);
}
