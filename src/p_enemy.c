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
/// \file  p_enemy.c
/// \brief Enemy thinking, AI

#include "dehacked.h"
#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "p_action.h"
#include "r_main.h"
#include "m_random.h"

player_t *stplyr;

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
			if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_SWIMMABLE))
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

		if (player->bot == BOT_2PAI || player->bot == BOT_2PHUMAN)
			continue; // ignore followbots

		if (player->quittime)
			continue; // Ignore uncontrolled bodies

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
boolean P_LookForShield(mobj_t *actor)
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
			&& (R_PointToDist2(0, 0, R_PointToDist2(0, 0, actor->x-player->mo->x, actor->y-player->mo->y), actor->z-player->mo->z) < FixedMul(RING_DIST, player->mo->scale)))
		{
			P_SetTarget(&actor->tracer, player->mo);

			if (actor->hnext)
				P_SetTarget(&actor->hnext->hprev, actor->hprev);
			if (actor->hprev)
				P_SetTarget(&actor->hprev->hnext, actor->hnext);

			return true;
		}
	}

	//return false;
}
