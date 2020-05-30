// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_floor.c
/// \brief Floor animation, elevators

#include "doomdef.h"
#include "doomstat.h"
#include "m_random.h"
#include "p_local.h"
#include "p_slopes.h"
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"
#include "g_game.h"
#include "r_main.h"

// ==========================================================================
//                              FLOORS
// ==========================================================================

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, boolean crush,
	boolean ceiling, INT32 direction)
{
	fixed_t lastpos;
	fixed_t destheight; // used to keep floors/ceilings from moving through each other
	sector->moved = true;

	if (ceiling)
	{
		lastpos = sector->ceilingheight;
		// moving a ceiling
		switch (direction)
		{
			case -1:
				// moving a ceiling down
				// keep ceiling from moving through floors
				destheight = (dest > sector->floorheight) ? dest : sector->floorheight;
				if (sector->ceilingheight - speed < destheight)
				{
					sector->ceilingheight = destheight;
					if (P_CheckSector(sector, crush))
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{
					// crushing is possible
					sector->ceilingheight -= speed;
					if (P_CheckSector(sector, crush))
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;

			case 1:
				// moving a ceiling up
				if (sector->ceilingheight + speed > dest)
				{
					sector->ceilingheight = dest;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{
					sector->ceilingheight += speed;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->ceilingheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;
		}
	}
	else
	{
		lastpos = sector->floorheight;
		// moving a floor
		switch (direction)
		{
			case -1:
				// Moving a floor down
				if (sector->floorheight - speed < dest)
				{
					sector->floorheight = dest;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{
					sector->floorheight -= speed;
					if (P_CheckSector(sector, crush) && sector->numattached)
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;

			case 1:
				// Moving a floor up
				// keep floor from moving through ceilings
				destheight = (dest < sector->ceilingheight) ? dest : sector->ceilingheight;
				if (sector->floorheight + speed > destheight)
				{
					sector->floorheight = destheight;
					if (P_CheckSector(sector, crush))
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
					}
					return pastdest;
				}
				else
				{
					// crushing is possible
					sector->floorheight += speed;
					if (P_CheckSector(sector, crush))
					{
						sector->floorheight = lastpos;
						P_CheckSector(sector, crush);
						return crushed;
					}
				}
				break;
		}
	}

	return ok;
}

//
// MOVE A FLOOR TO ITS DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t *movefloor)
{
	result_e res = 0;
	boolean dontupdate = false;

	if (movefloor->delaytimer)
	{
		movefloor->delaytimer--;
		return;
	}

	res = T_MovePlane(movefloor->sector,
	                  movefloor->speed,
	                  movefloor->floordestheight,
	                  movefloor->crush, false, movefloor->direction);

	if (movefloor->type == bounceFloor)
	{
		const fixed_t origspeed = FixedDiv(movefloor->origspeed,(ELEVATORSPEED/2));
		const fixed_t fs = abs(movefloor->sector->floorheight - lines[movefloor->texture].frontsector->floorheight);
		const fixed_t bs = abs(movefloor->sector->floorheight - lines[movefloor->texture].backsector->floorheight);
		if (fs < bs)
			movefloor->speed = FixedDiv(fs,25*FRACUNIT) + FRACUNIT/4;
		else
			movefloor->speed = FixedDiv(bs,25*FRACUNIT) + FRACUNIT/4;

		movefloor->speed = FixedMul(movefloor->speed,origspeed);
	}

	if (res == pastdest)
	{
		if (movefloor->direction == 1)
		{
			switch (movefloor->type)
			{
				case moveFloorByFrontSector:
					if (movefloor->texture < -1) // chained linedef executing
						P_LinedefExecute((INT16)(movefloor->texture + INT16_MAX + 2), NULL, NULL);
					/* FALLTHRU */
				case instantMoveFloorByFrontSector:
					if (movefloor->texture > -1) // flat changing
						movefloor->sector->floorpic = movefloor->texture;
					break;
				case bounceFloor: // Graue 03-12-2004
					if (movefloor->floordestheight == lines[movefloor->texture].frontsector->floorheight)
						movefloor->floordestheight = lines[movefloor->texture].backsector->floorheight;
					else
						movefloor->floordestheight = lines[movefloor->texture].frontsector->floorheight;
					movefloor->direction = (movefloor->floordestheight < movefloor->sector->floorheight) ? -1 : 1;
					movefloor->sector->floorspeed = movefloor->speed * movefloor->direction;
					movefloor->delaytimer = movefloor->delay;
					P_RecalcPrecipInSector(movefloor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				case bounceFloorCrush: // Graue 03-27-2004
					if (movefloor->floordestheight == lines[movefloor->texture].frontsector->floorheight)
					{
						movefloor->floordestheight = lines[movefloor->texture].backsector->floorheight;
						movefloor->speed = movefloor->origspeed = FixedDiv(abs(lines[movefloor->texture].dy),4*FRACUNIT); // return trip, use dy
					}
					else
					{
						movefloor->floordestheight = lines[movefloor->texture].frontsector->floorheight;
						movefloor->speed = movefloor->origspeed = FixedDiv(abs(lines[movefloor->texture].dx),4*FRACUNIT); // forward again, use dx
					}
					movefloor->direction = (movefloor->floordestheight < movefloor->sector->floorheight) ? -1 : 1;
					movefloor->sector->floorspeed = movefloor->speed * movefloor->direction;
					movefloor->delaytimer = movefloor->delay;
					P_RecalcPrecipInSector(movefloor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				case crushFloorOnce:
					movefloor->floordestheight = lines[movefloor->texture].frontsector->floorheight;
					movefloor->direction = -1;
					movefloor->sector->soundorg.z = movefloor->sector->floorheight;
					S_StartSound(&movefloor->sector->soundorg,sfx_pstop);
					P_RecalcPrecipInSector(movefloor->sector);
					return;
				default:
					break;
			}
		}
		else if (movefloor->direction == -1)
		{
			switch (movefloor->type)
			{
				case moveFloorByFrontSector:
					if (movefloor->texture < -1) // chained linedef executing
						P_LinedefExecute((INT16)(movefloor->texture + INT16_MAX + 2), NULL, NULL);
					/* FALLTHRU */
				case instantMoveFloorByFrontSector:
					if (movefloor->texture > -1) // flat changing
						movefloor->sector->floorpic = movefloor->texture;
					break;
				case bounceFloor: // Graue 03-12-2004
					if (movefloor->floordestheight == lines[movefloor->texture].frontsector->floorheight)
						movefloor->floordestheight = lines[movefloor->texture].backsector->floorheight;
					else
						movefloor->floordestheight = lines[movefloor->texture].frontsector->floorheight;
					movefloor->direction = (movefloor->floordestheight < movefloor->sector->floorheight) ? -1 : 1;
					movefloor->sector->floorspeed = movefloor->speed * movefloor->direction;
					movefloor->delaytimer = movefloor->delay;
					P_RecalcPrecipInSector(movefloor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				case bounceFloorCrush: // Graue 03-27-2004
					if (movefloor->floordestheight == lines[movefloor->texture].frontsector->floorheight)
					{
						movefloor->floordestheight = lines[movefloor->texture].backsector->floorheight;
						movefloor->speed = movefloor->origspeed = FixedDiv(abs(lines[movefloor->texture].dy),4*FRACUNIT); // return trip, use dy
					}
					else
					{
						movefloor->floordestheight = lines[movefloor->texture].frontsector->floorheight;
						movefloor->speed = movefloor->origspeed = FixedDiv(abs(lines[movefloor->texture].dx),4*FRACUNIT); // forward again, use dx
					}
					movefloor->direction = (movefloor->floordestheight < movefloor->sector->floorheight) ? -1 : 1;
					movefloor->sector->floorspeed = movefloor->speed * movefloor->direction;
					movefloor->delaytimer = movefloor->delay;
					P_RecalcPrecipInSector(movefloor->sector);
					return; // not break, why did this work? Graue 04-03-2004
				case crushFloorOnce:
					movefloor->sector->floordata = NULL; // Clear up the thinker so others can use it
					P_RemoveThinker(&movefloor->thinker);
					movefloor->sector->floorspeed = 0;
					P_RecalcPrecipInSector(movefloor->sector);
					return;
				default:
					break;
			}
		}

		movefloor->sector->floordata = NULL; // Clear up the thinker so others can use it
		movefloor->sector->floorspeed = 0;
		P_RemoveThinker(&movefloor->thinker);
		dontupdate = true;
	}
	if (!dontupdate)
		movefloor->sector->floorspeed = movefloor->speed*movefloor->direction;
	else
		movefloor->sector->floorspeed = 0;

	P_RecalcPrecipInSector(movefloor->sector);
}

//
// T_MoveElevator
//
// Move an elevator to it's destination (up or down)
// Called once per tick for each moving floor.
//
// Passed an elevator_t structure that contains all pertinent info about the
// move. See p_spec.h for fields.
// No return.
//
// The function moves the planes differently based on direction, so if it's
// traveling really fast, the floor and ceiling won't hit each other and
// stop the lift.
void T_MoveElevator(elevator_t *elevator)
{
	result_e res1 = 0, res2 = 0, res = 0;
	boolean dontupdate = false;
	fixed_t oldfloor, oldceiling;

	if (elevator->delaytimer)
	{
		elevator->delaytimer--;
		return;
	}

	if (elevator->direction < 0) // moving down
	{
		if (elevator->type == elevateContinuous)
		{
			const fixed_t origspeed = FixedDiv(elevator->origspeed,(ELEVATORSPEED/2));
			const fixed_t wh = abs(elevator->sector->floorheight - elevator->floorwasheight);
			const fixed_t dh = abs(elevator->sector->floorheight - elevator->floordestheight);

			// Slow down when reaching destination Tails 12-06-2000
			if (wh < dh)
				elevator->speed = FixedDiv(wh,25*FRACUNIT) + FRACUNIT/4;
			else
				elevator->speed = FixedDiv(dh,25*FRACUNIT) + FRACUNIT/4;

			if (elevator->origspeed)
			{
				elevator->speed = FixedMul(elevator->speed,origspeed);
				if (elevator->speed > elevator->origspeed)
					elevator->speed = (elevator->origspeed);
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
			else
			{
				if (elevator->speed > 3*FRACUNIT)
					elevator->speed = 3*FRACUNIT;
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
		}

		oldfloor = elevator->sector->floorheight;
		oldceiling = elevator->sector->ceilingheight;

		res1 = T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
			elevator->sector,
			elevator->speed,
			elevator->ceilingdestheight,
			elevator->distance,
			true,                          // move ceiling
			elevator->direction
		);

		res2 = T_MovePlane
		(
			elevator->sector,
			elevator->speed,
			elevator->floordestheight,
			elevator->distance,
			false,                        // move floor
			elevator->direction
		);

		if (elevator->distance && (res1 == crushed || res2 == crushed))
		{
			res = crushed;
			elevator->sector->floorheight = oldfloor;
			elevator->sector->ceilingheight = oldceiling;
		}
		else
			res = res1;
	}
	else // moving up
	{
		if (elevator->type == elevateContinuous)
		{
			const fixed_t origspeed = FixedDiv(elevator->origspeed,(ELEVATORSPEED/2));
			const fixed_t wc = abs(elevator->sector->ceilingheight - elevator->ceilingwasheight);
			const fixed_t dc = abs(elevator->sector->ceilingheight - elevator->ceilingdestheight);
			// Slow down when reaching destination Tails 12-06-2000
			if (wc < dc)
				elevator->speed = FixedDiv(wc,25*FRACUNIT) + FRACUNIT/4;
			else
				elevator->speed = FixedDiv(dc,25*FRACUNIT) + FRACUNIT/4;

			if (elevator->origspeed)
			{
				elevator->speed = FixedMul(elevator->speed,origspeed);
				if (elevator->speed > elevator->origspeed)
					elevator->speed = (elevator->origspeed);
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
			else
			{
				if (elevator->speed > 3*FRACUNIT)
					elevator->speed = 3*FRACUNIT;
				if (elevator->speed < 1)
					elevator->speed = 1;
			}
		}

		oldfloor = elevator->sector->floorheight;
		oldceiling = elevator->sector->ceilingheight;

		res1 = T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
		(
			elevator->sector,
			elevator->speed,
			elevator->floordestheight,
			elevator->distance,
			false,                          // move floor
			elevator->direction
		);

		if (res1 != crushed)
		{
			res2 = T_MovePlane
			(
				elevator->sector,
				elevator->speed,
				elevator->ceilingdestheight,
				elevator->distance,
				true,                        // move ceiling
				elevator->direction
			);
		}

		if (elevator->distance && (res1 == crushed || res2 == crushed))
		{
			res = crushed;
			elevator->sector->floorheight = oldfloor;
			elevator->sector->ceilingheight = oldceiling;
		}
		else
			res = res1;
	}
/*
	// make floor move sound
	if (!(leveltime&7))
		S_StartSound(&elevator->sector->soundorg, sfx_stnmov);
*/
	if (res == pastdest || res == crushed)            // if destination height acheived
	{
		if (elevator->type == elevateContinuous)
		{
			if (elevator->direction > 0)
			{
				elevator->high = 1;
				elevator->low = 0;
				elevator->direction = -1;

				if (elevator->origspeed)
					elevator->speed = elevator->origspeed;
				else
					elevator->speed = 3*FRACUNIT;

				elevator->floorwasheight = elevator->floordestheight;
				elevator->ceilingwasheight = elevator->ceilingdestheight;

				if (elevator->low)
				{
					elevator->floordestheight =
						P_FindNextHighestFloor(elevator->sector, elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				else
				{
					elevator->floordestheight =
						P_FindNextLowestFloor(elevator->sector,elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
//				T_MoveElevator(elevator);
			}
			else
			{
				elevator->high = 0;
				elevator->low = 1;
				elevator->direction = 1;

				if (elevator->origspeed)
					elevator->speed = elevator->origspeed;
				else
					elevator->speed = 3*FRACUNIT;

				elevator->floorwasheight = elevator->floordestheight;
				elevator->ceilingwasheight = elevator->ceilingdestheight;

				if (elevator->low)
				{
					elevator->floordestheight =
						P_FindNextHighestFloor(elevator->sector, elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
				else
				{
					elevator->floordestheight =
						P_FindNextLowestFloor(elevator->sector,elevator->sector->floorheight);
					elevator->ceilingdestheight =
						elevator->floordestheight + elevator->sector->ceilingheight - elevator->sector->floorheight;
				}
//				T_MoveElevator(elevator);
			}
			elevator->delaytimer = elevator->delay;
		}
		else
		{
			elevator->sector->floordata = NULL;     //jff 2/22/98
			elevator->sector->ceilingdata = NULL;   //jff 2/22/98
			elevator->sector->ceilspeed = 0;
			elevator->sector->floorspeed = 0;
			P_RemoveThinker(&elevator->thinker);    // remove elevator from actives
			dontupdate = true;
		}
		// make floor stop sound
		// S_StartSound(&elevator->sector->soundorg, sfx_pstop);
	}
	if (!dontupdate)
	{
		elevator->sector->floorspeed = elevator->speed*elevator->direction;
		elevator->sector->ceilspeed = 42;
	}
	else
	{
		elevator->sector->floorspeed = 0;
		elevator->sector->ceilspeed = 0;
		elevator->sector->floordata = NULL;
		elevator->sector->ceilingdata = NULL;
	}
}

//
// T_ContinuousFalling
//
// A sector that continuously falls until its ceiling
// is below that of its actionsector's floor, then
// it instantly returns to its original position and
// falls again.
//
// Useful for things like intermittent falling lava.
//
void T_ContinuousFalling(continuousfall_t *faller)
{
	faller->sector->ceilingheight += faller->speed*faller->direction;
	faller->sector->floorheight += faller->speed*faller->direction;

	P_CheckSector(faller->sector, false);

	if ((faller->direction == -1 && faller->sector->ceilingheight <= faller->destheight)
		|| (faller->direction == 1 && faller->sector->floorheight >= faller->destheight))
	{
		faller->sector->ceilingheight = faller->ceilingstartheight;
		faller->sector->floorheight = faller->floorstartheight;
	}

	P_CheckSector(faller->sector, false); // you might think this is irrelevant. you would be wrong

	faller->sector->floorspeed = faller->speed*faller->direction;
	faller->sector->ceilspeed = 42;
	faller->sector->moved = true;
}

//
// P_SectorCheckWater
//
// Like P_MobjCheckWater, but takes a sector instead of a mobj.
static fixed_t P_SectorCheckWater(sector_t *analyzesector,
	sector_t *elevatorsec)
{
	fixed_t watertop;

	// Default if no water exists.
	watertop = analyzesector->floorheight - 512*FRACUNIT;

	// see if we are in water, and set some flags for later
	if (analyzesector->ffloors)
	{
		ffloor_t *rover;

		for (rover = analyzesector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_SOLID)
				continue;

			// If the sector is below the water, don't bother.
			if ((elevatorsec->ceilingheight + elevatorsec->floorheight)>>1 < *rover->bottomheight)
				continue;

			// Do the same as above if the water is too shallow.
			if (*rover->topheight < analyzesector->floorheight + abs((elevatorsec->ceilingheight - elevatorsec->floorheight)>>1))
				continue;

			if (*rover->topheight > watertop) // highest water block is the one to go for
				watertop = *rover->topheight;
		}
	}

	return watertop;
}

//////////////////////////////////////////////////
// T_BounceCheese ////////////////////////////////
//////////////////////////////////////////////////
// Bounces a floating cheese

void T_BounceCheese(bouncecheese_t *bouncer)
{
	fixed_t sectorheight;
	fixed_t halfheight;
	fixed_t waterheight;
	fixed_t floorheight;
	sector_t *actionsector;
	INT32 i;
	boolean remove;

	if (bouncer->sector->crumblestate == CRUMBLE_RESTORE || bouncer->sector->crumblestate == CRUMBLE_WAIT
		|| bouncer->sector->crumblestate == CRUMBLE_ACTIVATED) // Oops! Crumbler says to remove yourself!
	{
		bouncer->sector->crumblestate = CRUMBLE_WAIT;
		bouncer->sector->ceilingdata = NULL;
		bouncer->sector->ceilspeed = 0;
		bouncer->sector->floordata = NULL;
		bouncer->sector->floorspeed = 0;
		P_RemoveThinker(&bouncer->thinker); // remove bouncer from actives
		return;
	}

	// You can use multiple target sectors, but at your own risk!!!
	for (i = -1; (i = P_FindSectorFromTag(bouncer->sourceline->tag, i)) >= 0 ;)
	{
		actionsector = &sectors[i];
		actionsector->moved = true;

		sectorheight = abs(bouncer->sector->ceilingheight - bouncer->sector->floorheight);
		halfheight = sectorheight/2;

		waterheight = P_SectorCheckWater(actionsector, bouncer->sector); // sorts itself out if there's no suitable water in the sector

		floorheight = P_FloorzAtPos(actionsector->soundorg.x, actionsector->soundorg.y, bouncer->sector->floorheight, sectorheight);

		remove = false;

		// Water level is up to the ceiling.
		if (waterheight > bouncer->sector->ceilingheight - halfheight && bouncer->sector->ceilingheight >= actionsector->ceilingheight) // Tails 01-08-2004
		{
			bouncer->sector->ceilingheight = actionsector->ceilingheight;
			bouncer->sector->floorheight = actionsector->ceilingheight - sectorheight;
			remove = true;
		}
		// Water level is too shallow.
		else if (waterheight < bouncer->sector->floorheight + halfheight && bouncer->sector->floorheight <= floorheight)
		{
			bouncer->sector->ceilingheight = floorheight + sectorheight;
			bouncer->sector->floorheight = floorheight;
			remove = true;
		}
		else
		{
			bouncer->ceilingwasheight = waterheight + halfheight;
			bouncer->floorwasheight = waterheight - halfheight;
		}

		if (remove)
		{
			T_MovePlane(bouncer->sector, 0, bouncer->sector->ceilingheight, false, true, -1); // update things on ceiling
			T_MovePlane(bouncer->sector, 0, bouncer->sector->floorheight, false, false, -1); // update things on floor
			P_RecalcPrecipInSector(actionsector);
			bouncer->sector->ceilingdata = NULL;
			bouncer->sector->floordata = NULL;
			bouncer->sector->floorspeed = 0;
			bouncer->sector->ceilspeed = 0;
			bouncer->sector->moved = true;
			P_RemoveThinker(&bouncer->thinker); // remove bouncer from actives
			return;
		}

		if (bouncer->speed >= 0) // move floor first to fix height desync and any bizarre bugs following that
		{
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->floorheight - 70*FRACUNIT,
				false, false, -1); // move floor
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->ceilingheight -
				70*FRACUNIT, false, true, -1); // move ceiling
		}
		else
		{
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->ceilingheight -
				70*FRACUNIT, false, true, -1); // move ceiling
			T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->floorheight - 70*FRACUNIT,
				false, false, -1); // move floor
		}

		bouncer->sector->floorspeed = -bouncer->speed/2;
		bouncer->sector->ceilspeed = 42;

		if ((bouncer->sector->ceilingheight < bouncer->ceilingwasheight && !bouncer->low) // Down
			|| (bouncer->sector->ceilingheight > bouncer->ceilingwasheight && bouncer->low)) // Up
		{
			if (abs(bouncer->speed) < 6*FRACUNIT)
				bouncer->speed -= bouncer->speed/3;
			else
				bouncer->speed -= bouncer->speed/2;

			bouncer->low = !bouncer->low;
			if (abs(bouncer->speed) > 6*FRACUNIT)
			{
				mobj_t *mp = (void *)&actionsector->soundorg;
				actionsector->soundorg.z = bouncer->sector->floorheight;
				S_StartSound(mp, sfx_splash);
			}
		}

		if (bouncer->sector->ceilingheight < bouncer->ceilingwasheight) // Down
		{
			bouncer->speed -= bouncer->distance;
		}
		else if (bouncer->sector->ceilingheight > bouncer->ceilingwasheight) // Up
		{
			bouncer->speed += gravity;
		}

		if (abs(bouncer->speed) < 2*FRACUNIT
		&& abs(bouncer->sector->ceilingheight-bouncer->ceilingwasheight) < FRACUNIT/4)
		{
			bouncer->sector->floorheight = bouncer->floorwasheight;
			bouncer->sector->ceilingheight = bouncer->ceilingwasheight;
			T_MovePlane(bouncer->sector, 0, bouncer->sector->ceilingheight, false, true, -1); // update things on ceiling
			T_MovePlane(bouncer->sector, 0, bouncer->sector->floorheight, false, false, -1); // update things on floor
			bouncer->sector->ceilingdata = NULL;
			bouncer->sector->floordata = NULL;
			bouncer->sector->floorspeed = 0;
			bouncer->sector->ceilspeed = 0;
			bouncer->sector->moved = true;
			P_RemoveThinker(&bouncer->thinker);    // remove bouncer from actives
		}

		if (bouncer->distance > 0)
			bouncer->distance--;

		if (actionsector)
			P_RecalcPrecipInSector(actionsector);
	}
}

//////////////////////////////////////////////////
// T_StartCrumble ////////////////////////////////
//////////////////////////////////////////////////
// Crumbling platform Tails 03-11-2002
void T_StartCrumble(crumble_t *crumble)
{
	ffloor_t *rover;
	sector_t *sector;
	INT32 i;

	// Once done, the no-return thinker just sits there,
	// constantly 'returning'... kind of an oxymoron, isn't it?
	if ((((crumble->flags & CF_REVERSE) && crumble->direction == -1)
		|| (!(crumble->flags & CF_REVERSE) && crumble->direction == 1))
		&& !(crumble->flags & CF_RETURN))
	{
		crumble->sector->ceilspeed = 0;
		crumble->sector->floorspeed = 0;
		return;
	}

	if (crumble->timer != 0)
	{
		if (crumble->timer > 0) // Count down the timer
		{
			if (--crumble->timer <= 0)
				crumble->timer = -15*TICRATE; // Timer until platform returns to original position.
			else
			{
				// Timer isn't up yet, so just keep waiting.
				crumble->sector->ceilspeed = 0;
				crumble->sector->floorspeed = 0;
				return;
			}
		}
		else if (++crumble->timer == 0) // Reposition back to original spot
		{
			for (i = -1; (i = P_FindSectorFromTag(crumble->sourceline->tag, i)) >= 0 ;)
			{
				sector = &sectors[i];

				for (rover = sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_CRUMBLE))
						continue;

					if (!(rover->flags & FF_FLOATBOB))
						continue;

					if (rover->master != crumble->sourceline)
						continue;

					rover->alpha = crumble->origalpha;

					if (rover->alpha == 0xff)
						rover->flags &= ~FF_TRANSLUCENT;
				}
			}

			// Up!
			if (crumble->flags & CF_REVERSE)
				crumble->direction = -1;
			else
				crumble->direction = 1;

			crumble->sector->ceilspeed = 0;
			crumble->sector->floorspeed = 0;
			return;
		}

		// Flash to indicate that the platform is about to return.
		if (crumble->timer > -224 && (leveltime % ((abs(crumble->timer)/8) + 1) == 0))
		{
			for (i = -1; (i = P_FindSectorFromTag(crumble->sourceline->tag, i)) >= 0 ;)
			{
				sector = &sectors[i];

				for (rover = sector->ffloors; rover; rover = rover->next)
				{
					if (rover->flags & FF_NORETURN)
						continue;

					if (!(rover->flags & FF_CRUMBLE))
						continue;

					if (!(rover->flags & FF_FLOATBOB))
						continue;

					if (rover->master != crumble->sourceline)
						continue;

					if (rover->alpha == crumble->origalpha)
					{
						rover->flags |= FF_TRANSLUCENT;
						rover->alpha = 0x00;
					}
					else
					{
						rover->alpha = crumble->origalpha;

						if (rover->alpha == 0xff)
							rover->flags &= ~FF_TRANSLUCENT;
					}
				}
			}
		}

		// We're about to go back to the original position,
		// so set this to let other thinkers know what is
		// about to happen.
		if (crumble->timer < 0 && crumble->timer > -3)
			crumble->sector->crumblestate = CRUMBLE_RESTORE; // makes T_BounceCheese remove itself
	}

	if ((!(crumble->flags & CF_REVERSE) && crumble->direction == -1)
		|| ((crumble->flags & CF_REVERSE) && crumble->direction == 1)) // Down
	{
		crumble->sector->crumblestate = CRUMBLE_FALL; // Allow floating now.

		// Only fall like this if it isn't meant to float on water
		if (!(crumble->flags & CF_FLOATBOB))
		{
			crumble->speed += gravity; // Gain more and more speed

			if ((!(crumble->flags & CF_REVERSE) && crumble->sector->ceilingheight >= -16384*FRACUNIT)
				|| ((crumble->flags & CF_REVERSE) && crumble->sector->ceilingheight <= 16384*FRACUNIT))
			{
				T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
				(
				  crumble->sector,
				  crumble->speed,
				  crumble->sector->ceilingheight + crumble->direction*crumble->speed*2,
				  false,
				  true, // move ceiling
				  crumble->direction
				);

				T_MovePlane
				(
				  crumble->sector,
				  crumble->speed,
				  crumble->sector->floorheight + crumble->direction*crumble->speed*2,
				  false,
				  false, // move floor
				  crumble->direction
				);

				crumble->sector->ceilspeed = 42;
				crumble->sector->floorspeed = crumble->speed*crumble->direction;
			}
		}
	}
	else // Up (restore to original position)
	{
		crumble->sector->crumblestate = CRUMBLE_WAIT;
		crumble->sector->ceilingheight = crumble->ceilingwasheight;
		crumble->sector->floorheight = crumble->floorwasheight;
		crumble->sector->floordata = NULL;
		crumble->sector->ceilingdata = NULL;
		crumble->sector->ceilspeed = 0;
		crumble->sector->floorspeed = 0;
		crumble->sector->moved = true;
		P_RemoveThinker(&crumble->thinker);
	}

	for (i = -1; (i = P_FindSectorFromTag(crumble->sourceline->tag, i)) >= 0 ;)
	{
		sector = &sectors[i];
		sector->moved = true;
		P_RecalcPrecipInSector(sector);
	}
}

//////////////////////////////////////////////////
// T_MarioBlock //////////////////////////////////
//////////////////////////////////////////////////
// Mario hits a block!
//
void T_MarioBlock(mariothink_t *block)
{
	INT32 i;

	T_MovePlane
	(
	  block->sector,
	  block->speed,
	  block->sector->ceilingheight + 70*FRACUNIT * block->direction,
	  false,
	  true, // move ceiling
	  block->direction
	);

	T_MovePlane
	(
	  block->sector,
	  block->speed,
	  block->sector->floorheight + 70*FRACUNIT * block->direction,
	  false,
	  false, // move floor
	  block->direction
	);

	if (block->sector->ceilingheight >= block->ceilingstartheight + 32*FRACUNIT) // Go back down now..
		block->direction *= -1;
	else if (block->sector->ceilingheight <= block->ceilingstartheight)
	{
		block->sector->ceilingheight = block->ceilingstartheight;
		block->sector->floorheight = block->floorstartheight;
		P_RemoveThinker(&block->thinker);
		block->sector->floordata = NULL;
		block->sector->ceilingdata = NULL;
		block->sector->floorspeed = 0;
		block->sector->ceilspeed = 0;
		block->direction = 0;
	}

	for (i = -1; (i = P_FindSectorFromTag(block->tag, i)) >= 0 ;)
		P_RecalcPrecipInSector(&sectors[i]);
}

void T_FloatSector(floatthink_t *floater)
{
	fixed_t cheeseheight;
	fixed_t waterheight;
	sector_t *actionsector;
	INT32 secnum;

	// Just find the first sector with the tag.
	// Doesn't work with multiple sectors that have different floor/ceiling heights.
	secnum = P_FindSectorFromTag(floater->tag, -1);
	if (secnum <= 0)
		return;
	actionsector = &sectors[secnum];

	cheeseheight = (floater->sector->ceilingheight + floater->sector->floorheight)>>1;

	//boolean floatanyway = false; // Ignore the crumblestate setting.
	waterheight = P_SectorCheckWater(actionsector, floater->sector); // find the highest suitable water block around

	if (waterheight == cheeseheight) // same height, no floating needed
		return;

	if (floater->sector->floorheight == actionsector->floorheight && waterheight < cheeseheight) // too low
		return;

	if (floater->sector->ceilingheight == actionsector->ceilingheight && waterheight > cheeseheight) // too high
		return;

	// we have something to float in! Or we're for some reason above the ground, let's fall anyway
	if (floater->sector->crumblestate == CRUMBLE_NONE || floater->sector->crumblestate >= CRUMBLE_FALL/* || floatanyway*/)
		EV_BounceSector(floater->sector, FRACUNIT, floater->sourceline);
}

static mobj_t *SearchMarioNode(msecnode_t *node)
{
	mobj_t *thing = NULL;
	for (; node; node = node->m_thinglist_next)
	{
		// Things which should NEVER be ejected from a MarioBlock, by type.
		switch (node->m_thing->type)
		{
		case MT_NULL:
		case MT_UNKNOWN:
		case MT_TAILSOVERLAY:
		case MT_THOK:
		case MT_GHOST:
		case MT_OVERLAY:
		case MT_EMERALDSPAWN:
		case MT_ELEMENTAL_ORB:
		case MT_ATTRACT_ORB:
		case MT_FORCE_ORB:
		case MT_ARMAGEDDON_ORB:
		case MT_WHIRLWIND_ORB:
		case MT_PITY_ORB:
		case MT_FLAMEAURA_ORB:
		case MT_BUBBLEWRAP_ORB:
		case MT_THUNDERCOIN_ORB:
		case MT_IVSP:
		case MT_SUPERSPARK:
		case MT_RAIN:
		case MT_SNOWFLAKE:
		case MT_SPLISH:
		case MT_LAVASPLISH:
		case MT_SMOKE:
		case MT_SMALLBUBBLE:
		case MT_MEDIUMBUBBLE:
		case MT_TFOG:
		case MT_SEED:
		case MT_PARTICLE:
		case MT_SCORE:
		case MT_DROWNNUMBERS:
		case MT_GOTEMERALD:
		case MT_LOCKON:
		case MT_TAG:
		case MT_GOTFLAG:
		case MT_HOOP:
		case MT_HOOPCOLLIDE:
		case MT_NIGHTSCORE:
#ifdef SEENAMES
		case MT_NAMECHECK: // DEFINITELY not this, because it is client-side.
#endif
			continue;
		default:
			break;
		}
		// Ignore popped monitors, too.
		if (node->m_thing->health == 0 // this only really applies for monitors
		|| (!(node->m_thing->flags & MF_MONITOR) && (mobjinfo[node->m_thing->type].flags & MF_MONITOR))) // gold monitor support
			continue;
		// Okay, we found something valid.
		if (!thing // take either the first thing
		|| node->m_thing->x < thing->x // or the thing with the lowest x value (left to right item order)
		|| node->m_thing->y < thing->y) // or the thing with the lowest y value (top to bottom item order)
			thing = node->m_thing;
	}
	return thing;
}

void T_MarioBlockChecker(mariocheck_t *block)
{
	line_t *masterline = block->sourceline;
	if (SearchMarioNode(block->sector->touching_thinglist))
	{
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].bottomtexture; // Update textures
		if (masterline->backsector)
			block->sector->ceilingpic = block->sector->floorpic = masterline->backsector->ceilingpic; // Update flats to be backside's ceiling
	}
	else
	{
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].toptexture;
		if (masterline->backsector)
			block->sector->ceilingpic = block->sector->floorpic = masterline->backsector->floorpic; // Update flats to be backside's floor
	}
}

static boolean P_IsPlayerValid(size_t playernum)
{
	if (!playeringame[playernum])
		return false;

	if (!players[playernum].mo)
		return false;

	if (players[playernum].mo->health <= 0)
		return false;

	if (players[playernum].spectator)
		return false;

	return true;
}

// This is the Thwomp's 'brain'. It looks around for players nearby, and if
// it finds any, **SMASH**!!! Muahahhaa....
void T_ThwompSector(thwomp_t *thwomp)
{
	fixed_t thwompx, thwompy;
	sector_t *actionsector;
	ffloor_t *rover = NULL;
	INT32 secnum;
	fixed_t speed;

	// If you just crashed down, wait a second before coming back up.
	if (--thwomp->delay > 0)
		return;

	// Just find the first sector with the tag.
	// Doesn't work with multiple sectors that have different floor/ceiling heights.
	secnum = P_FindSectorFromTag(thwomp->tag, -1);

	if (secnum <= 0)
		return; // Bad bad bad!

	actionsector = &sectors[secnum];

	// Look for thwomp FOF
	for (rover = actionsector->ffloors; rover; rover = rover->next)
	{
		if (rover->master == thwomp->sourceline)
			break;
	}

	if (!rover)
		return; // Didn't find any FOFs, so bail out

	thwompx = actionsector->soundorg.x;
	thwompy = actionsector->soundorg.y;

	if (thwomp->direction == 0) // Not going anywhere, so look for players.
	{
		if (rover->flags & FF_EXISTS)
		{
			UINT8 i;
			// scan the players to find victims!
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!P_IsPlayerValid(i))
					continue;

				if (players[i].mo->z > thwomp->sector->ceilingheight)
					continue;

				if (P_AproxDistance(thwompx - players[i].mo->x, thwompy - players[i].mo->y) > 96*FRACUNIT)
					continue;

				thwomp->direction = -1;
				break;
			}
		}

		thwomp->sector->ceilspeed = 0;
		thwomp->sector->floorspeed = 0;
	}
	else
	{
		result_e res = 0;

		if (thwomp->direction > 0) //Moving back up..
		{
			// Set the texture from the lower one (normal)
			sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].bottomtexture;

			speed = thwomp->retractspeed;

			res = T_MovePlane
			(
				thwomp->sector,           // sector
				speed,                    // speed
				thwomp->floorstartheight, // dest
				false,                    // crush
				false,                    // ceiling?
				thwomp->direction         // direction
			);

			if (res == ok || res == pastdest)
				T_MovePlane
				(
					thwomp->sector,             // sector
					speed,                      // speed
					thwomp->ceilingstartheight, // dest
					false,                      // crush
					true,                       // ceiling?
					thwomp->direction           // direction
				);

			if (res == pastdest)
				thwomp->direction = 0; // stop moving
			}
		else // Crashing down!
		{
			// Set the texture from the upper one (angry)
			sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].toptexture;

			speed = thwomp->crushspeed;

			res = T_MovePlane
			(
				thwomp->sector,   // sector
				speed,            // speed
				P_FloorzAtPos(thwompx, thwompy, thwomp->sector->floorheight,
					thwomp->sector->ceilingheight - thwomp->sector->floorheight), // dest
				false,              // crush
				false,              // ceiling?
				thwomp->direction // direction
			);

			if (res == ok || res == pastdest)
				T_MovePlane
				(
					thwomp->sector,   // sector
					speed,            // speed
					P_FloorzAtPos(thwompx, thwompy, thwomp->sector->floorheight,
						thwomp->sector->ceilingheight
						- (thwomp->sector->floorheight + speed))
						+ (thwomp->sector->ceilingheight
						- (thwomp->sector->floorheight + speed/2)), // dest
					false,             // crush
					true,              // ceiling?
					thwomp->direction // direction
				);

			if (res == pastdest)
			{
				if (rover->flags & FF_EXISTS)
					S_StartSound((void *)&actionsector->soundorg, thwomp->sound);

				thwomp->direction = 1; // start heading back up
				thwomp->delay = TICRATE; // but only after a small delay
			}
		}

		thwomp->sector->ceilspeed = 42;
		thwomp->sector->floorspeed = speed*thwomp->direction;
	}

	P_RecalcPrecipInSector(actionsector);
}

static boolean T_SectorHasEnemies(sector_t *sec)
{
	msecnode_t *node = sec->touching_thinglist; // things touching this sector
	mobj_t *mo;
	while (node)
	{
		mo = node->m_thing;

		if ((mo->flags & (MF_ENEMY|MF_BOSS))
			&& mo->health > 0
			&& mo->z < sec->ceilingheight
			&& mo->z + mo->height > sec->floorheight)
			return true;

		node = node->m_thinglist_next;
	}

	return false;
}

//
// T_NoEnemiesThinker
//
// Runs a linedef exec when no more MF_ENEMY/MF_BOSS objects with health are in the area
// \sa P_AddNoEnemiesThinker
//
void T_NoEnemiesSector(noenemies_t *nobaddies)
{
	size_t i;
	sector_t *sec = NULL;
	INT32 secnum = -1;
	boolean FOFsector = false;

	while ((secnum = P_FindSectorFromTag(nobaddies->sourceline->tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		FOFsector = false;

		// Check the lines of this sector, to see if it is a FOF control sector.
		for (i = 0; i < sec->linecount; i++)
		{
			INT32 targetsecnum = -1;

			if (sec->lines[i]->special < 100 || sec->lines[i]->special >= 300)
				continue;

			FOFsector = true;

			while ((targetsecnum = P_FindSectorFromTag(sec->lines[i]->tag, targetsecnum)) >= 0)
			{
				if (T_SectorHasEnemies(&sectors[targetsecnum]))
					return;
			}
		}

		if (!FOFsector && T_SectorHasEnemies(sec))
			return;
	}

	CONS_Debug(DBG_GAMELOGIC, "Running no-more-enemies exec with tag of %d\n", nobaddies->sourceline->tag);

	// No enemies found, run the linedef exec and terminate this thinker
	P_RunTriggerLinedef(nobaddies->sourceline, NULL, NULL);
	P_RemoveThinker(&nobaddies->thinker);
}

//
// P_IsObjectOnRealGround
//
// Helper function for T_EachTimeThinker
// Like P_IsObjectOnGroundIn, except ONLY THE REAL GROUND IS CONSIDERED, NOT FOFS
// I'll consider whether to make this a more globally accessible function or whatever in future
// -- Monster Iestyn
//
static boolean P_IsObjectOnRealGround(mobj_t *mo, sector_t *sec)
{
	// Is the object in reverse gravity?
	if (mo->eflags & MFE_VERTICALFLIP)
	{
		// Detect if the player is on the ceiling.
		if (mo->z+mo->height >= P_GetSpecialTopZ(mo, sec, sec))
			return true;
	}
	// Nope!
	else
	{
		// Detect if the player is on the floor.
		if (mo->z <= P_GetSpecialBottomZ(mo, sec, sec))
			return true;
	}
	return false;
}

static boolean P_IsMobjTouchingSector(mobj_t *mo, sector_t *sec)
{
	msecnode_t *node;

	if (mo->subsector->sector == sec)
		return true;

	if (!(sec->flags & SF_TRIGGERSPECIAL_TOUCH))
		return false;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (node->m_sector == sec)
			return true;
	}

	return false;
}

//
// T_EachTimeThinker
//
// Runs a linedef exec whenever a player enters an area.
// Keeps track of players currently in the area and notices any changes.
//
// \sa P_AddEachTimeThinker
//
void T_EachTimeThinker(eachtime_t *eachtime)
{
	size_t i, j;
	sector_t *sec = NULL;
	sector_t *targetsec = NULL;
	INT32 secnum = -1;
	boolean oldPlayersInArea[MAXPLAYERS];
	boolean oldPlayersOnArea[MAXPLAYERS];
	boolean *oldPlayersArea;
	boolean *playersArea;
	boolean FOFsector = false;
	boolean floortouch = false;
	fixed_t bottomheight, topheight;
	ffloor_t *rover;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		oldPlayersInArea[i] = eachtime->playersInArea[i];
		oldPlayersOnArea[i] = eachtime->playersOnArea[i];
		eachtime->playersInArea[i] = false;
		eachtime->playersOnArea[i] = false;
	}

	while ((secnum = P_FindSectorFromTag(eachtime->sourceline->tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		FOFsector = false;

		if (GETSECSPECIAL(sec->special, 2) == 3 || GETSECSPECIAL(sec->special, 2) == 5)
			floortouch = true;
		else if (GETSECSPECIAL(sec->special, 2) >= 1 && GETSECSPECIAL(sec->special, 2) <= 8)
			floortouch = false;
		else
			continue;

		// Check the lines of this sector, to see if it is a FOF control sector.
		for (i = 0; i < sec->linecount; i++)
		{
			INT32 targetsecnum = -1;

			if (sec->lines[i]->special < 100 || sec->lines[i]->special >= 300)
				continue;

			FOFsector = true;

			while ((targetsecnum = P_FindSectorFromTag(sec->lines[i]->tag, targetsecnum)) >= 0)
			{
				targetsec = &sectors[targetsecnum];

				// Find the FOF corresponding to the control linedef
				for (rover = targetsec->ffloors; rover; rover = rover->next)
				{
					if (rover->master == sec->lines[i])
						break;
				}

				if (!rover) // This should be impossible, but don't complain if it is the case somehow
					continue;

				if (!(rover->flags & FF_EXISTS)) // If the FOF does not "exist", we pretend that nobody's there
					continue;

				for (j = 0; j < MAXPLAYERS; j++)
				{
					if (!P_IsPlayerValid(j))
						continue;

					if (!P_IsMobjTouchingSector(players[j].mo, targetsec))
						continue;

					topheight = P_GetSpecialTopZ(players[j].mo, sec, targetsec);
					bottomheight = P_GetSpecialBottomZ(players[j].mo, sec, targetsec);

					if (players[j].mo->z > topheight)
						continue;

					if (players[j].mo->z + players[j].mo->height < bottomheight)
						continue;

					if (floortouch && P_IsObjectOnGroundIn(players[j].mo, targetsec))
						eachtime->playersOnArea[j] = true;
					else
						eachtime->playersInArea[j] = true;
				}
			}
		}

		if (!FOFsector)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!P_IsPlayerValid(i))
					continue;

				if (!P_IsMobjTouchingSector(players[i].mo, sec))
					continue;

				if (!(players[i].mo->subsector->sector == sec
					|| P_PlayerTouchingSectorSpecial(&players[i], 2, (GETSECSPECIAL(sec->special, 2))) == sec))
					continue;

				if (floortouch && P_IsObjectOnRealGround(players[i].mo, sec))
					eachtime->playersOnArea[i] = true;
				else
					eachtime->playersInArea[i] = true;
			}
		}
	}

	// Check if a new player entered.
	// If not, check if a player hit the floor.
	// If either condition is true, execute.
	if (floortouch)
	{
		playersArea = eachtime->playersOnArea;
		oldPlayersArea = oldPlayersOnArea;
	}
	else
	{
		playersArea = eachtime->playersInArea;
		oldPlayersArea = oldPlayersInArea;
	}

	// Easy check... nothing has changed
	if (!memcmp(playersArea, oldPlayersArea, sizeof(boolean)*MAXPLAYERS))
		return;

	// If sector has an "all players" trigger type, all players need to be in area
	if (GETSECSPECIAL(sec->special, 2) == 2 || GETSECSPECIAL(sec->special, 2) == 3)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (P_IsPlayerValid(i) && playersArea[i])
				continue;
		}
	}

	// Trigger for every player who has entered (and exited, if triggerOnExit)
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playersArea[i] == oldPlayersArea[i])
			continue;

		// If player has just left, check if still valid
		if (!playersArea[i] && (!eachtime->triggerOnExit || !P_IsPlayerValid(i)))
			continue;

		CONS_Debug(DBG_GAMELOGIC, "Trying to activate each time executor with tag %d\n", eachtime->sourceline->tag);

		// 03/08/14 -Monster Iestyn
		// No more stupid hacks involving changing eachtime->sourceline's tag or special or whatever!
		// This should now run ONLY the stuff for eachtime->sourceline itself, instead of all trigger linedefs sharing the same tag.
		// Makes much more sense doing it this way, honestly.
		P_RunTriggerLinedef(eachtime->sourceline, players[i].mo, sec);

		if (!eachtime->sourceline->special) // this happens only for "Trigger on X calls" linedefs
			P_RemoveThinker(&eachtime->thinker);
	}
}

//
// T_RaiseSector
//
// Rises up to its topmost position when a
// player steps on it. Lowers otherwise.
//
void T_RaiseSector(raise_t *raise)
{
	msecnode_t *node;
	mobj_t *thing;
	sector_t *sector;
	INT32 i;
	boolean playeronme = false, active = false;
	boolean moveUp;
	fixed_t ceilingdestination, floordestination;
	fixed_t speed, origspeed;
	fixed_t distToNearestEndpoint;
	INT32 direction;
	result_e res = 0;

	if (raise->sector->crumblestate >= CRUMBLE_FALL || raise->sector->ceilingdata)
		return;

	for (i = -1; (i = P_FindSectorFromTag(raise->tag, i)) >= 0 ;)
	{
		sector = &sectors[i];

		// Is a player standing on me?
		for (node = sector->touching_thinglist; node; node = node->m_thinglist_next)
		{
			thing = node->m_thing;

			if (!thing->player)
				continue;

			// Ignore spectators.
			if (thing->player && thing->player->spectator)
				continue;

			// Option to require spindashing.
			if (raise->flags & RF_SPINDASH && !(thing->player->pflags & PF_STARTDASH))
				continue;

			if (!(thing->z == P_GetSpecialTopZ(thing, raise->sector, sector)))
				continue;

			playeronme = true;
			break;
		}
	}

	if (raise->flags & RF_DYNAMIC) // Dynamically Sinking Platform^tm
	{
#define shaketime 10
		if (raise->shaketimer > shaketime) // State: moving
		{
			if (playeronme) // If player is standing on the platform, accelerate
			{
				raise->extraspeed += (FRACUNIT >> 5);
			}
			else // otherwise, decelerate until inflection
			{
				raise->extraspeed -= FRACUNIT >> 3;
				if (raise->extraspeed <= 0) // inflection!
				{
					raise->extraspeed = 0;
					raise->shaketimer = 0; // allow the shake to occur again (fucks over players attempting to jump-cheese)
				}
			}
			active = raise->extraspeed > 0;
		}
		else // State: shaking
		{
			if (playeronme || raise->shaketimer)
			{
				active = true;
				if (++raise->shaketimer > shaketime)
				{
					if (playeronme)
						raise->extraspeed = FRACUNIT >> 5;
					else
						raise->extraspeed = FRACUNIT << 1;
				}
				else
				{
					raise->extraspeed = ((shaketime/2) - raise->shaketimer) << FRACBITS;
					if (raise->extraspeed < -raise->basespeed/2)
						raise->extraspeed = -raise->basespeed/2;
				}
			}
		}
#undef shaketime
	}
	else // Air bobbing platform (not a Dynamically Sinking Platform^tm)
		active = playeronme;

	moveUp = active ^ (raise->flags & RF_REVERSE);
	ceilingdestination = moveUp ? raise->ceilingtop : raise->ceilingbottom;
	floordestination = ceilingdestination - (raise->sector->ceilingheight - raise->sector->floorheight);

	if ((moveUp && raise->sector->ceilingheight >= ceilingdestination)
		|| (!moveUp && raise->sector->ceilingheight <= ceilingdestination))
	{
		raise->sector->floorheight = floordestination;
		raise->sector->ceilingheight = ceilingdestination;
		raise->sector->ceilspeed = 0;
		raise->sector->floorspeed = 0;
		return;
	}
	direction = moveUp ? 1 : -1;

	origspeed = raise->basespeed;
	if (!active)
		origspeed /= 2;

	// Speed up as you get closer to the middle, then slow down again
	distToNearestEndpoint = min(raise->sector->ceilingheight - raise->ceilingbottom, raise->ceilingtop - raise->sector->ceilingheight);
	speed = FixedMul(origspeed, FixedDiv(distToNearestEndpoint, (raise->ceilingtop - raise->ceilingbottom) >> 5));

	if (speed <= origspeed/16)
		speed = origspeed/16;
	else if (speed > origspeed)
		speed = origspeed;

	speed += raise->extraspeed;

	res = T_MovePlane
	(
		raise->sector,      // sector
		speed,              // speed
		ceilingdestination, // dest
		false,              // crush
		true,               // ceiling?
		direction           // direction
	);

	if (res == ok || res == pastdest)
		T_MovePlane
		(
			raise->sector,    // sector
			speed,            // speed
			floordestination, // dest
			false,            // crush
			false,            // ceiling?
			direction         // direction
		);

	raise->sector->ceilspeed = 42;
	raise->sector->floorspeed = speed*direction;

	for (i = -1; (i = P_FindSectorFromTag(raise->tag, i)) >= 0 ;)
		P_RecalcPrecipInSector(&sectors[i]);
}

void T_CameraScanner(elevator_t *elevator)
{
	// leveltime is compared to make multiple scanners in one map function correctly.
	static tic_t lastleveltime = 32000; // any number other than 0 should do here
	static boolean camerascanned, camerascanned2;

	if (leveltime != lastleveltime) // Back on the first camera scanner
	{
		camerascanned = camerascanned2 = false;
		lastleveltime = leveltime;
	}

	if (players[displayplayer].mo)
	{
		if (players[displayplayer].mo->subsector->sector == elevator->actionsector)
		{
			if (t_cam_dist == -42)
				t_cam_dist = cv_cam_dist.value;
			if (t_cam_height == -42)
				t_cam_height = cv_cam_height.value;
			if (t_cam_rotate == -42)
				t_cam_rotate = cv_cam_rotate.value;
			CV_SetValue(&cv_cam_height, FixedInt(elevator->sector->floorheight));
			CV_SetValue(&cv_cam_dist, FixedInt(elevator->sector->ceilingheight));
			CV_SetValue(&cv_cam_rotate, elevator->distance);
			camerascanned = true;
		}
		else if (!camerascanned)
		{
			if (t_cam_height != -42 && cv_cam_height.value != t_cam_height)
				CV_Set(&cv_cam_height, va("%f", (double)FIXED_TO_FLOAT(t_cam_height)));
			if (t_cam_dist != -42 && cv_cam_dist.value != t_cam_dist)
				CV_Set(&cv_cam_dist, va("%f", (double)FIXED_TO_FLOAT(t_cam_dist)));
			if (t_cam_rotate != -42 && cv_cam_rotate.value != t_cam_rotate)
				CV_Set(&cv_cam_rotate, va("%f", (double)t_cam_rotate));

			t_cam_dist = t_cam_height = t_cam_rotate = -42;
		}
	}

	if (splitscreen && players[secondarydisplayplayer].mo)
	{
		if (players[secondarydisplayplayer].mo->subsector->sector == elevator->actionsector)
		{
			if (t_cam2_rotate == -42)
				t_cam2_dist = cv_cam2_dist.value;
			if (t_cam2_rotate == -42)
				t_cam2_height = cv_cam2_height.value;
			if (t_cam2_rotate == -42)
				t_cam2_rotate = cv_cam2_rotate.value;
			CV_SetValue(&cv_cam2_height, FixedInt(elevator->sector->floorheight));
			CV_SetValue(&cv_cam2_dist, FixedInt(elevator->sector->ceilingheight));
			CV_SetValue(&cv_cam2_rotate, elevator->distance);
			camerascanned2 = true;
		}
		else if (!camerascanned2)
		{
			if (t_cam2_height != -42 && cv_cam2_height.value != t_cam2_height)
				CV_Set(&cv_cam2_height, va("%f", (double)FIXED_TO_FLOAT(t_cam2_height)));
			if (t_cam2_dist != -42 && cv_cam2_dist.value != t_cam2_dist)
				CV_Set(&cv_cam2_dist, va("%f", (double)FIXED_TO_FLOAT(t_cam2_dist)));
			if (t_cam2_rotate != -42 && cv_cam2_rotate.value != t_cam2_rotate)
				CV_Set(&cv_cam2_rotate, va("%f", (double)t_cam2_rotate));

			t_cam2_dist = t_cam2_height = t_cam2_rotate = -42;
		}
	}
}

void T_PlaneDisplace(planedisplace_t *pd)
{
	sector_t *control, *target;
	INT32 direction;
	fixed_t diff;

	control = &sectors[pd->control];
	target = &sectors[pd->affectee];

	if (control->floorheight == pd->last_height)
		return; // no change, no movement

	direction = (control->floorheight > pd->last_height) ? 1 : -1;
	diff = FixedMul(control->floorheight-pd->last_height, pd->speed);

	if (pd->reverse) // reverse direction?
	{
		direction *= -1;
		diff *= -1;
	}

	if (pd->type == pd_floor || pd->type == pd_both)
		T_MovePlane(target, INT32_MAX/2, target->floorheight+diff, false, false, direction); // move floor
	if (pd->type == pd_ceiling || pd->type == pd_both)
		T_MovePlane(target, INT32_MAX/2, target->ceilingheight+diff, false, true, direction); // move ceiling

	pd->last_height = control->floorheight;
}

//
// EV_DoFloor
//
// Set up and start a floor thinker.
//
// Called by P_ProcessLineSpecial (linedef executors), P_ProcessSpecialSector
// (egg capsule button), P_PlayerInSpecialSector (buttons),
// and P_SpawnSpecials (continuous floor movers and instant lower).
//
void EV_DoFloor(line_t *line, floor_e floortype)
{
	INT32 firstone = 1;
	INT32 secnum = -1;
	sector_t *sec;
	floormove_t *dofloor;

	while ((secnum = P_FindSectorFromTag(line->tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		if (sec->floordata) // if there's already a thinker on this floor,
			continue; // then don't add another one

		// new floor thinker
		dofloor = Z_Calloc(sizeof (*dofloor), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &dofloor->thinker);

		// make sure another floor thinker won't get started over this one
		sec->floordata = dofloor;

		// set up some generic aspects of the floormove_t
		dofloor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
		dofloor->type = floortype;
		dofloor->crush = false; // default: types that crush will change this
		dofloor->sector = sec;

		switch (floortype)
		{
			// Lowers a floor to the lowest surrounding floor.
			case lowerFloorToLowest:
				dofloor->direction = -1; // down
				dofloor->speed = FLOORSPEED*2; // 2 fracunits per tic
				dofloor->floordestheight = P_FindLowestFloorSurrounding(sec);
				break;

			// Used for part of the Egg Capsule, when an FOF with type 666 is
			// contacted by the player.
			case raiseFloorToNearestFast:
				dofloor->direction = -1; // down
				dofloor->speed = FLOORSPEED*4; // 4 fracunits per tic
				dofloor->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				break;

			// Used for sectors tagged to 50 linedefs (effectively
			// changing the base height for placing things in that sector).
			case instantLower:
				dofloor->direction = -1; // down
				dofloor->speed = INT32_MAX/2; // "instant" means "takes one tic"
				dofloor->floordestheight = P_FindLowestFloorSurrounding(sec);
				break;

			// Linedef executor command, linetype 101.
			// Front sector floor = destination height.
			case instantMoveFloorByFrontSector:
				dofloor->speed = INT32_MAX/2; // as above, "instant" is one tic
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1; // up
				else
					dofloor->direction = -1; // down

				// New for 1.09: now you can use the no climb flag to
				// DISABLE the flat changing. This makes it work
				// totally opposite the way linetype 106 does. Yet
				// another reason I'll be glad to break backwards
				// compatibility for the final.
				if (line->flags & ML_NOCLIMB)
					dofloor->texture = -1; // don't mess with the floorpic
				else
					dofloor->texture = line->frontsector->floorpic;
				break;

			// Linedef executor command, linetype 106.
			// Line length = speed, front sector floor = destination height.
			case moveFloorByFrontSector:
				dofloor->speed = P_AproxDistance(line->dx, line->dy);
				dofloor->speed = FixedDiv(dofloor->speed,8*FRACUNIT);
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1; // up
				else
					dofloor->direction = -1; // down

				// chained linedef executing ability
				if (line->flags & ML_BLOCKMONSTERS)
				{
					// Only set it on one of the moving sectors (the
					// smallest numbered) and only if the front side
					// x offset is positive, indicating a valid tag.
					if (firstone && sides[line->sidenum[0]].textureoffset > 0)
						dofloor->texture = (sides[line->sidenum[0]].textureoffset>>FRACBITS) - 32769;
					else
						dofloor->texture = -1;
				}

				// flat changing ability
				else if (line->flags & ML_NOCLIMB)
					dofloor->texture = line->frontsector->floorpic;
				else
					dofloor->texture = -1; // nothing special to do after movement completes

				break;

			case moveFloorByFrontTexture:
				if (line->flags & ML_NOCLIMB)
					dofloor->speed = INT32_MAX/2; // as above, "instant" is one tic
				else
					dofloor->speed = FixedDiv(sides[line->sidenum[0]].textureoffset,8*FRACUNIT); // texture x offset
				dofloor->floordestheight = sec->floorheight + sides[line->sidenum[0]].rowoffset; // texture y offset
				if (dofloor->floordestheight > sec->floorheight)
					dofloor->direction = 1; // up
				else
					dofloor->direction = -1; // down
				break;

/*
			// Linedef executor command, linetype 108.
			// dx = speed, dy = amount to lower.
			case lowerFloorByLine:
				dofloor->direction = -1; // down
				dofloor->speed = FixedDiv(abs(line->dx),8*FRACUNIT);
				dofloor->floordestheight = sec->floorheight - abs(line->dy);
				if (dofloor->floordestheight > sec->floorheight) // wrapped around
					I_Error("Can't lower sector %d\n", secnum);
				break;

			// Linedef executor command, linetype 109.
			// dx = speed, dy = amount to raise.
			case raiseFloorByLine:
				dofloor->direction = 1; // up
				dofloor->speed = FixedDiv(abs(line->dx),8*FRACUNIT);
				dofloor->floordestheight = sec->floorheight + abs(line->dy);
				if (dofloor->floordestheight < sec->floorheight) // wrapped around
					I_Error("Can't raise sector %d\n", secnum);
				break;
*/

			// Linetypes 2/3.
			// Move floor up and down indefinitely like the old elevators.
			case bounceFloor:
				dofloor->speed = P_AproxDistance(line->dx, line->dy); // same speed as elevateContinuous
				dofloor->speed = FixedDiv(dofloor->speed,4*FRACUNIT);
				dofloor->origspeed = dofloor->speed; // it gets slowed down at the top and bottom
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1; // up
				else
					dofloor->direction = -1; // down

				// Any delay?
				dofloor->delay = sides[line->sidenum[0]].textureoffset >> FRACBITS;
				dofloor->delaytimer = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Initial delay

				dofloor->texture = (fixed_t)(line - lines); // hack: store source line number
				break;

			// Linetypes 6/7.
			// Like 2/3, but no slowdown at the top and bottom of movement,
			// and the speed is line->dx the first way, line->dy for the
			// return trip. Good for crushers.
			case bounceFloorCrush:
				dofloor->speed = FixedDiv(abs(line->dx),4*FRACUNIT);
				dofloor->origspeed = dofloor->speed;
				dofloor->floordestheight = line->frontsector->floorheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1; // up
				else
					dofloor->direction = -1; // down

				// Any delay?
				dofloor->delay = sides[line->sidenum[0]].textureoffset >> FRACBITS;
				dofloor->delaytimer = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Initial delay

				dofloor->texture = (fixed_t)(line - lines); // hack: store source line number
				break;

			case crushFloorOnce:
				dofloor->speed = FixedDiv(abs(line->dx),4*FRACUNIT);
				dofloor->origspeed = dofloor->speed;
				dofloor->floordestheight = line->frontsector->ceilingheight;

				if (dofloor->floordestheight >= sec->floorheight)
					dofloor->direction = 1; // up
				else
					dofloor->direction = -1; // down

				// Any delay?
				dofloor->delay = sides[line->sidenum[0]].textureoffset >> FRACBITS;
				dofloor->delaytimer = sides[line->sidenum[0]].rowoffset >> FRACBITS;

				dofloor->texture = (fixed_t)(line - lines); // hack: store source line number
				break;

			default:
				break;
		}

		firstone = 0;
	}
}

// SoM: Boom elevator support.
//
// EV_DoElevator
//
// Handle elevator linedef types
//
// Passed the linedef that triggered the elevator and the elevator action
//
// jff 2/22/98 new type to move floor and ceiling in parallel
//
void EV_DoElevator(line_t *line, elevator_e elevtype, boolean customspeed)
{
	INT32 secnum = -1;
	sector_t *sec;
	elevator_t *elevator;

	// act on all sectors with the same tag as the triggering linedef
	while ((secnum = P_FindSectorFromTag(line->tag,secnum)) >= 0)
	{
		sec = &sectors[secnum];

		// If either floor or ceiling is already activated, skip it
		if (sec->floordata || sec->ceilingdata)
			continue;

		// create and initialize new elevator thinker
		elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &elevator->thinker);
		sec->floordata = elevator;
		sec->ceilingdata = elevator;
		elevator->thinker.function.acp1 = (actionf_p1)T_MoveElevator;
		elevator->type = elevtype;
		elevator->sourceline = line;
		elevator->distance = 1; // Always crush unless otherwise

		// set up the fields according to the type of elevator action
		switch (elevtype)
		{
			// elevator down to next floor
			case elevateDown:
				elevator->direction = -1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED/2; // half speed
				elevator->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			// elevator up to next floor
			case elevateUp:
				elevator->direction = 1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED/4; // quarter speed
				elevator->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			// elevator up to highest floor
			case elevateHighest:
				elevator->direction = 1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED/4; // quarter speed
				elevator->floordestheight = P_FindHighestFloorSurrounding(sec);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			// elevator to floor height of activating switch's front sector
			case elevateCurrent:
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED;
				elevator->floordestheight = line->frontsector->floorheight;
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				elevator->direction = elevator->floordestheight > sec->floorheight?  1 : -1;
				break;

			case elevateContinuous:
				if (customspeed)
				{
					elevator->origspeed = P_AproxDistance(line->dx, line->dy);
					elevator->origspeed = FixedDiv(elevator->origspeed,4*FRACUNIT);
					elevator->speed = elevator->origspeed;
				}
				else
				{
					elevator->speed = ELEVATORSPEED/2;
					elevator->origspeed = elevator->speed;
				}

				elevator->sector = sec;
				elevator->low = !(line->flags & ML_NOCLIMB); // go down first unless noclimb is on
				if (elevator->low)
				{
					elevator->direction = 1;
					elevator->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
					elevator->ceilingdestheight = elevator->floordestheight
						+ sec->ceilingheight - sec->floorheight;
				}
				else
				{
					elevator->direction = -1;
					elevator->floordestheight = P_FindNextLowestFloor(sec,sec->floorheight);
					elevator->ceilingdestheight = elevator->floordestheight
						+ sec->ceilingheight - sec->floorheight;
				}
				elevator->floorwasheight = elevator->sector->floorheight;
				elevator->ceilingwasheight = elevator->sector->ceilingheight;

				elevator->delay = sides[line->sidenum[0]].textureoffset >> FRACBITS;
				elevator->delaytimer = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Initial delay
				break;

			case bridgeFall:
				elevator->direction = -1;
				elevator->sector = sec;
				elevator->speed = ELEVATORSPEED*4; // quadruple speed
				elevator->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
				elevator->ceilingdestheight = elevator->floordestheight
					+ sec->ceilingheight - sec->floorheight;
				break;

			default:
				break;
		}
	}
}

void EV_CrumbleChain(sector_t *sec, ffloor_t *rover)
{
	size_t i, leftmostvertex, rightmostvertex, topmostvertex, bottommostvertex;
	fixed_t leftx, rightx, topy, bottomy, topz, bottomz, widthfactor, heightfactor, a, b, c, spacing;
	mobjtype_t type;
	tic_t lifetime;
	INT16 flags;

	sector_t *controlsec = rover->master->frontsector;

	if (sec == NULL)
	{
		if (controlsec->numattached)
		{
			for (i = 0; i < controlsec->numattached; i++)
			{
				sec = &sectors[controlsec->attached[i]];
				if (!sec->ffloors)
					continue;

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (rover->master->frontsector == controlsec)
						EV_CrumbleChain(sec, rover);
				}
			}
		}
		return;
	}

	leftmostvertex = rightmostvertex = topmostvertex = bottommostvertex = 0;
	widthfactor = heightfactor = FRACUNIT;
	spacing = (32<<FRACBITS);
	type = MT_ROCKCRUMBLE1;
	lifetime = 3*TICRATE;
	flags = 0;

	if (controlsec->tag != 0)
	{
		INT32 tagline = P_FindSpecialLineFromTag(14, controlsec->tag, -1);
		if (tagline != -1)
		{
			if (sides[lines[tagline].sidenum[0]].toptexture)
				type = (mobjtype_t)sides[lines[tagline].sidenum[0]].toptexture; // Set as object type in p_setup.c...
			if (sides[lines[tagline].sidenum[0]].textureoffset)
				spacing = sides[lines[tagline].sidenum[0]].textureoffset;
			if (sides[lines[tagline].sidenum[0]].rowoffset)
			{
				if (sides[lines[tagline].sidenum[0]].rowoffset>>FRACBITS != -1)
					lifetime = (sides[lines[tagline].sidenum[0]].rowoffset>>FRACBITS);
				else
					lifetime = 0;
			}
			flags = lines[tagline].flags;
		}
	}

#undef controlsec

	// soundorg z height never gets set normally, so MEH.
	sec->soundorg.z = sec->floorheight;
	S_StartSound(&sec->soundorg, mobjinfo[type].activesound);

	// Find the outermost vertexes in the subsector
	for (i = 0; i < sec->linecount; i++)
	{
		// Find the leftmost vertex in the subsector.
		if ((sec->lines[i]->v1->x < sec->lines[leftmostvertex]->v1->x))
			leftmostvertex = i;
		// Find the rightmost vertex in the subsector.
		if ((sec->lines[i]->v1->x > sec->lines[rightmostvertex]->v1->x))
			rightmostvertex = i;
		// Find the topmost vertex in the subsector.
		if ((sec->lines[i]->v1->y > sec->lines[topmostvertex]->v1->y))
			topmostvertex = i;
		// Find the bottommost vertex in the subsector.
		if ((sec->lines[i]->v1->y < sec->lines[bottommostvertex]->v1->y))
			bottommostvertex = i;
	}

	leftx = sec->lines[leftmostvertex]->v1->x+(spacing>>1);
	rightx = sec->lines[rightmostvertex]->v1->x;
	topy = sec->lines[topmostvertex]->v1->y-(spacing>>1);
	bottomy = sec->lines[bottommostvertex]->v1->y;

	topz = *rover->topheight-(spacing>>1);
	bottomz = *rover->bottomheight;

	if (flags & ML_EFFECT1)
	{
		widthfactor = (rightx + topy - leftx - bottomy)>>3;
		heightfactor = (topz - *rover->bottomheight)>>2;
	}

	for (a = leftx; a < rightx; a += spacing)
	{
		for (b = topy; b > bottomy; b -= spacing)
		{
			if (R_PointInSubsector(a, b)->sector == sec)
			{
				mobj_t *spawned = NULL;
				if (*rover->t_slope)
					topz = P_GetSlopeZAt(*rover->t_slope, a, b) - (spacing>>1);
				if (*rover->b_slope)
					bottomz = P_GetSlopeZAt(*rover->b_slope, a, b);

				for (c = topz; c > bottomz; c -= spacing)
				{
					spawned = P_SpawnMobj(a, b, c, type);
					spawned->angle += P_RandomKey(36)*ANG10; // irrelevant for default objects but might make sense for some custom ones

					if (flags & ML_EFFECT1)
					{
						P_InstaThrust(spawned, R_PointToAngle2(sec->soundorg.x, sec->soundorg.y, a, b), FixedDiv(P_AproxDistance(a - sec->soundorg.x, b - sec->soundorg.y), widthfactor));
						P_SetObjectMomZ(spawned, FixedDiv((c - bottomz), heightfactor), false);
					}

					spawned->fuse = lifetime;
				}
			}
		}
	}

	// no longer exists (can't collide with again)
	rover->flags &= ~FF_EXISTS;
	rover->master->frontsector->moved = true;
	P_RecalcPrecipInSector(sec);
}

// Used for bobbing platforms on the water
void EV_BounceSector(sector_t *sec, fixed_t momz, line_t *sourceline)
{
	bouncecheese_t *bouncer;

	// create and initialize new thinker
	if (sec->ceilingdata) // One at a time, ma'am.
		return;

	bouncer = Z_Calloc(sizeof (*bouncer), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &bouncer->thinker);
	sec->ceilingdata = bouncer;
	bouncer->thinker.function.acp1 = (actionf_p1)T_BounceCheese;

	// set up the fields according to the type of elevator action
	bouncer->sourceline = sourceline;
	bouncer->sector = sec;
	bouncer->speed = momz/2;
	bouncer->distance = FRACUNIT;
	bouncer->low = true;
}

// For T_ContinuousFalling special
void EV_DoContinuousFall(sector_t *sec, sector_t *backsector, fixed_t spd, boolean backwards)
{
	continuousfall_t *faller;

	// workaround for when there is no back sector
	if (!backsector)
		backsector = sec;

	// create and initialize new thinker
	faller = Z_Calloc(sizeof (*faller), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &faller->thinker);
	faller->thinker.function.acp1 = (actionf_p1)T_ContinuousFalling;

	// set up the fields
	faller->sector = sec;
	faller->speed = spd;

	faller->floorstartheight = sec->floorheight;
	faller->ceilingstartheight = sec->ceilingheight;

	faller->destheight = backwards ? backsector->ceilingheight : backsector->floorheight;
	faller->direction = backwards ? 1 : -1;
}

// Some other 3dfloor special things Tails 03-11-2002 (Search p_mobj.c for description)
INT32 EV_StartCrumble(sector_t *sec, ffloor_t *rover, boolean floating,
	player_t *player, fixed_t origalpha, boolean crumblereturn)
{
	crumble_t *crumble;
	sector_t *foundsec;
	INT32 i;

	// If floor is already activated, skip it
	if (sec->floordata)
		return 0;

	if (sec->crumblestate >= CRUMBLE_ACTIVATED)
		return 0;

	// create and initialize new crumble thinker
	crumble = Z_Calloc(sizeof (*crumble), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &crumble->thinker);
	crumble->thinker.function.acp1 = (actionf_p1)T_StartCrumble;

	// set up the fields
	crumble->sector = sec;
	crumble->speed = 0;

	if (player && player->mo && (player->mo->eflags & MFE_VERTICALFLIP))
	{
		crumble->direction = 1; // Up
		crumble->flags |= CF_REVERSE;
	}
	else
		crumble->direction = -1; // Down

	crumble->floorwasheight = crumble->sector->floorheight;
	crumble->ceilingwasheight = crumble->sector->ceilingheight;
	crumble->timer = TICRATE;
	crumble->player = player;
	crumble->origalpha = origalpha;

	crumble->sourceline = rover->master;

	sec->floordata = crumble;

	if (crumblereturn)
		crumble->flags |= CF_RETURN;
	if (floating)
		crumble->flags |= CF_FLOATBOB;

	crumble->sector->crumblestate = CRUMBLE_ACTIVATED;

	for (i = -1; (i = P_FindSectorFromTag(crumble->sourceline->tag, i)) >= 0 ;)
	{
		foundsec = &sectors[i];

		P_SpawnMobj(foundsec->soundorg.x, foundsec->soundorg.y, crumble->direction == 1 ? crumble->sector->floorheight : crumble->sector->ceilingheight, MT_CRUMBLEOBJ);
	}

	return 1;
}

void EV_MarioBlock(ffloor_t *rover, sector_t *sector, mobj_t *puncher)
{
	sector_t *roversec = rover->master->frontsector;
	fixed_t topheight = *rover->topheight;
	mariothink_t *block;
	mobj_t *thing;
	fixed_t oldx = 0, oldy = 0, oldz = 0;

	I_Assert(puncher != NULL);
	I_Assert(puncher->player != NULL);

	if (roversec->floordata || roversec->ceilingdata)
		return;

	if (!(rover->flags & FF_SOLID))
		rover->flags |= (FF_SOLID|FF_RENDERALL|FF_CUTLEVEL);

	// Find an item to pop out!
	thing = SearchMarioNode(roversec->touching_thinglist);

	if (!thing)
		S_StartSound(puncher, sfx_mario1); // "Thunk!" sound - puncher is "close enough".
	else // Found something!
	{
		const boolean itsamonitor = (thing->flags & MF_MONITOR) == MF_MONITOR;
		// create and initialize new elevator thinker

		block = Z_Calloc(sizeof (*block), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &block->thinker);
		roversec->floordata = block;
		roversec->ceilingdata = block;
		block->thinker.function.acp1 = (actionf_p1)T_MarioBlock;

		// Set up the fields
		block->sector = roversec;
		block->speed = 4*FRACUNIT;
		block->direction = 1;
		block->floorstartheight = block->sector->floorheight;
		block->ceilingstartheight = block->sector->ceilingheight;
		block->tag = (INT16)sector->tag;

		if (itsamonitor)
		{
			oldx = thing->x;
			oldy = thing->y;
			oldz = thing->z;
		}

		P_UnsetThingPosition(thing);
		thing->x = sector->soundorg.x;
		thing->y = sector->soundorg.y;
		thing->z = topheight;
		thing->momz = FixedMul(6*FRACUNIT, thing->scale);
		P_SetThingPosition(thing);
		if (thing->flags & MF_SHOOTABLE)
			P_DamageMobj(thing, puncher, puncher, 1, 0);
		else if (thing->type == MT_RING || thing->type == MT_COIN || thing->type == MT_TOKEN)
		{
			thing->momz = FixedMul(3*FRACUNIT, thing->scale);
			P_TouchSpecialThing(thing, puncher, false);
			// "Thunk!" sound
			S_StartSound(puncher, sfx_mario1); // Puncher is "close enough"
		}
		else
		{
			// "Powerup rise" sound
			S_StartSound(puncher, sfx_mario9); // Puncher is "close enough"
		}

		if (itsamonitor && thing)
		{
			P_UnsetThingPosition(thing);
			thing->x = oldx;
			thing->y = oldy;
			thing->z = oldz;
			thing->momx = 1;
			thing->momy = 1;
			P_SetThingPosition(thing);
		}
	}
}
