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
/// \file  p_floor.c
/// \brief Floor animation, elevators

#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"
#include "g_game.h"
#include "r_main.h"

// ==========================================================================
//                              FLOORS
// ==========================================================================

//
// Mini-P_IsObjectOnGroundIn for T_MovePlane hack
//
static inline boolean P_MobjReadyToMove(mobj_t *mo, sector_t *sec, boolean sectorisffloor, boolean sectorisquicksand)
{
	if (sectorisquicksand)
		return (mo->z > sec->floorheight && mo->z < sec->ceilingheight);
	else if (!!(mo->flags & MF_SPAWNCEILING) ^ !!(mo->eflags & MFE_VERTICALFLIP))
		return ((sectorisffloor) ? (mo->z+mo->height != sec->floorheight) : (mo->z+mo->height != sec->ceilingheight));
	else
		return ((sectorisffloor) ? (mo->z != sec->ceilingheight) : (mo->z != sec->floorheight));
}

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e T_MovePlane(sector_t *sector, fixed_t speed, fixed_t dest, boolean crush,
	INT32 floorOrCeiling, INT32 direction)
{
	boolean flag;
	fixed_t lastpos;
	fixed_t destheight; // used to keep floors/ceilings from moving through each other
	// Stuff used for mobj hacks.
	INT32 secnum = -1;
	mobj_t *mo = NULL;
	sector_t *sec = NULL;
	ffloor_t *rover = NULL;
	boolean sectorisffloor = false;
	boolean sectorisquicksand = false;

	sector->moved = true;

	switch (floorOrCeiling)
	{
		case 0:
			// moving a floor
			switch (direction)
			{
				case -1:
					// Moving a floor down
					if (sector->floorheight - speed < dest)
					{
						lastpos = sector->floorheight;
						sector->floorheight = dest;
						flag = P_CheckSector(sector, crush);
						if (flag && sector->numattached)
						{
							sector->floorheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						lastpos = sector->floorheight;
						sector->floorheight -= speed;
						flag = P_CheckSector(sector, crush);
						if (flag && sector->numattached)
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
						lastpos = sector->floorheight;
						sector->floorheight = destheight;
						flag = P_CheckSector(sector, crush);
						if (flag)
						{
							sector->floorheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						// crushing is possible
						lastpos = sector->floorheight;
						sector->floorheight += speed;
						flag = P_CheckSector(sector, crush);
						if (flag)
						{
							sector->floorheight = lastpos;
							P_CheckSector(sector, crush);
							return crushed;
						}
					}
					break;
			}
			break;

		case 1:
			// moving a ceiling
			switch (direction)
			{
				case -1:
					// moving a ceiling down
					// keep ceiling from moving through floors
					destheight = (dest > sector->floorheight) ? dest : sector->floorheight;
					if (sector->ceilingheight - speed < destheight)
					{
						lastpos = sector->ceilingheight;
						sector->ceilingheight = destheight;
						flag = P_CheckSector(sector, crush);

						if (flag)
						{
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						// crushing is possible
						lastpos = sector->ceilingheight;
						sector->ceilingheight -= speed;
						flag = P_CheckSector(sector, crush);

						if (flag)
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
						lastpos = sector->ceilingheight;
						sector->ceilingheight = dest;
						flag = P_CheckSector(sector, crush);
						if (flag && sector->numattached)
						{
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
						}
						return pastdest;
					}
					else
					{
						lastpos = sector->ceilingheight;
						sector->ceilingheight += speed;
						flag = P_CheckSector(sector, crush);
						if (flag && sector->numattached)
						{
							sector->ceilingheight = lastpos;
							P_CheckSector(sector, crush);
							return crushed;
						}
					}
					break;
			}
			break;
	}

	// Hack for buggy mobjs to move by gravity with moving planes.
	if (sector->tagline)
		sectorisffloor = true;

	// Optimization condition. If the sector is not an FOF, declare sec as the main sector outside of the loop.
	if (!sectorisffloor)
		sec = sector;

	// Optimization condition. Only run the logic if there is any Things in the sector.
	if (sectorisffloor || sec->thinglist)
	{
		// If this is an FOF being checked, check all the affected sectors for moving mobjs.
		while ((sectorisffloor ? (secnum = P_FindSectorFromLineTag(sector->tagline, secnum)) : (secnum = 1)) >= 0)
		{
			if (sectorisffloor)
			{
				// Get actual sector from the list of sectors.
				sec = &sectors[secnum];

				// Can't use P_InQuicksand because it will return the incorrect result
				// because of checking for heights.
				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (rover->target == sec && (rover->flags & FF_QUICKSAND))
					{
						sectorisquicksand = true;
						break;
					}
				}
			}

			for (mo = sec->thinglist; mo; mo = mo->snext)
			{
				// The object should be ready to move as defined by this function.
				if (!P_MobjReadyToMove(mo, sec, sectorisffloor, sectorisquicksand))
					continue;

				// The object should not be moving at all.
				if (mo->momx || mo->momy || mo->momz)
					continue;

				// These objects will be affected by this condition.
				switch (mo->type)
				{
					case MT_GOOP: // Egg Slimer's goop objects
					case MT_SPINFIRE: // Elemental Shield flame balls
					case MT_SPIKE: // Floor Spike
						// Is the object hang from the ceiling?
						// In that case, swap the planes used.
						// verticalflip inverts
						if (!!(mo->flags & MF_SPAWNCEILING) ^ !!(mo->eflags & MFE_VERTICALFLIP))
						{
							if (sectorisffloor && !sectorisquicksand)
								mo->z = mo->ceilingz - mo->height;
							else
								mo->z = mo->ceilingz = mo->subsector->sector->ceilingheight - mo->height;
						}
						else
						{
							if (sectorisffloor && !sectorisquicksand)
								mo->z = mo->floorz;
							else
								mo->z = mo->floorz = mo->subsector->sector->floorheight;
						}
						break;
					// Kill warnings...
					default:
						break;
				}
			}

			// Break from loop if there is no FOFs to check.
			if (!sectorisffloor)
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
	                  movefloor->crush, 0, movefloor->direction);

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
			1,                          // move floor
			elevator->direction
		);

		res2 = T_MovePlane
		(
			elevator->sector,
			elevator->speed,
			elevator->floordestheight,
			elevator->distance,
			0,                        // move ceiling
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
			0,                          // move ceiling
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
				1,                        // move floor
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
void T_ContinuousFalling(levelspecthink_t *faller)
{
#define speed vars[0]
#define direction vars[1]
#define floorwasheight vars[2]
#define ceilingwasheight vars[3]
#define floordestheight vars[4]
#define ceilingdestheight vars[5]

	if (faller->direction == -1)
	{
		faller->sector->ceilingheight -= faller->speed;
		faller->sector->floorheight -= faller->speed;
	}
	else
	{
		faller->sector->ceilingheight += faller->speed;
		faller->sector->floorheight += faller->speed;
	}

	P_CheckSector(faller->sector, false);

	if (faller->direction == -1) // Down
	{
		if (faller->sector->ceilingheight <= faller->ceilingdestheight)            // if destination height acheived
		{
			faller->sector->ceilingheight = faller->ceilingwasheight;
			faller->sector->floorheight = faller->floorwasheight;
		}
	}
	else // Up
	{
		if (faller->sector->floorheight >= faller->floordestheight)            // if destination height acheived
		{
			faller->sector->ceilingheight = faller->ceilingwasheight;
			faller->sector->floorheight = faller->floorwasheight;
		}
	}

	faller->sector->floorspeed = faller->speed*faller->direction;
	faller->sector->ceilspeed = 42;
	faller->sector->moved = true;
#undef speed
#undef direction
#undef floorwasheight
#undef ceilingwasheight
#undef floordestheight
#undef ceilingdestheight
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

void T_BounceCheese(levelspecthink_t *bouncer)
{
#define speed vars[0]
#define distance vars[1]
#define low vars[2]
#define ceilingwasheight vars[3]
#define floorwasheight vars[4]
	fixed_t halfheight;
	fixed_t waterheight;
	fixed_t floorheight;
	sector_t *actionsector;
	INT32 i;

	if (bouncer->sector->crumblestate == 4 || bouncer->sector->crumblestate == 1
		|| bouncer->sector->crumblestate == 2) // Oops! Crumbler says to remove yourself!
	{
		bouncer->sector->crumblestate = 1;
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

		halfheight = abs(bouncer->sector->ceilingheight - bouncer->sector->floorheight) >> 1;

		waterheight = P_SectorCheckWater(actionsector, bouncer->sector); // sorts itself out if there's no suitable water in the sector

		floorheight = P_FloorzAtPos(actionsector->soundorg.x, actionsector->soundorg.y, bouncer->sector->floorheight, halfheight << 1);

		// Water level is up to the ceiling.
		if (waterheight > bouncer->sector->ceilingheight - halfheight && bouncer->sector->ceilingheight >= actionsector->ceilingheight) // Tails 01-08-2004
		{
			bouncer->sector->ceilingheight = actionsector->ceilingheight;
			bouncer->sector->floorheight = bouncer->sector->ceilingheight - (halfheight*2);
			P_RecalcPrecipInSector(actionsector);
			bouncer->sector->ceilingdata = NULL;
			bouncer->sector->floordata = NULL;
			bouncer->sector->floorspeed = 0;
			bouncer->sector->ceilspeed = 0;
			bouncer->sector->moved = true;
			P_RemoveThinker(&bouncer->thinker); // remove bouncer from actives
			return;
		}
		// Water level is too shallow.
		else if (waterheight < bouncer->sector->floorheight + halfheight && bouncer->sector->floorheight <= floorheight)
		{
			bouncer->sector->ceilingheight = floorheight + (halfheight << 1);
			bouncer->sector->floorheight = floorheight;
			P_RecalcPrecipInSector(actionsector);
			bouncer->sector->ceilingdata = NULL;
			bouncer->sector->floordata = NULL;
			bouncer->sector->floorspeed = 0;
			bouncer->sector->ceilspeed = 0;
			bouncer->sector->moved = true;
			P_RemoveThinker(&bouncer->thinker); // remove bouncer from actives
			return;
		}
		else
		{
			bouncer->ceilingwasheight = waterheight + halfheight;
			bouncer->floorwasheight = waterheight - halfheight;
		}

		T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->ceilingheight -
			70*FRACUNIT, 0, 1, -1); // move floor
		T_MovePlane(bouncer->sector, bouncer->speed/2, bouncer->sector->floorheight - 70*FRACUNIT,
			0, 0, -1); // move ceiling

		bouncer->sector->floorspeed = -bouncer->speed/2;
		bouncer->sector->ceilspeed = 42;

		if (bouncer->sector->ceilingheight < bouncer->ceilingwasheight && bouncer->low == 0) // Down
		{
			if (abs(bouncer->speed) < 6*FRACUNIT)
				bouncer->speed -= bouncer->speed/3;
			else
				bouncer->speed -= bouncer->speed/2;

			bouncer->low = 1;
			if (abs(bouncer->speed) > 6*FRACUNIT)
			{
				mobj_t *mp = (void *)&actionsector->soundorg;
				actionsector->soundorg.z = bouncer->sector->floorheight;
				S_StartSound(mp, sfx_splash);
			}
		}
		else if (bouncer->sector->ceilingheight > bouncer->ceilingwasheight && bouncer->low) // Up
		{
			if (abs(bouncer->speed) < 6*FRACUNIT)
				bouncer->speed -= bouncer->speed/3;
			else
				bouncer->speed -= bouncer->speed/2;

			bouncer->low = 0;
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
#undef speed
#undef distance
#undef low
#undef ceilingwasheight
#undef floorwasheight
}

//////////////////////////////////////////////////
// T_StartCrumble ////////////////////////////////
//////////////////////////////////////////////////
// Crumbling platform Tails 03-11-2002
//
// DEFINITION OF THE 'CRUMBLESTATE'S:
//
// 0 - No crumble thinker
// 1 - Don't float on water because this is supposed to wait for a crumble
// 2 - Crumble thinker activated, but hasn't fallen yet
// 3 - Crumble thinker is falling
// 4 - Crumble thinker is about to restore to original position
//
void T_StartCrumble(elevator_t *elevator)
{
	ffloor_t *rover;
	sector_t *sector;
	INT32 i;

	// Once done, the no-return thinker just sits there,
	// constantly 'returning'... kind of an oxymoron, isn't it?
	if (((elevator->floordestheight == 1 && elevator->direction == -1)
		|| (elevator->floordestheight == 0 && elevator->direction == 1))
		&& elevator->type == elevateContinuous) // No return crumbler
	{
		elevator->sector->ceilspeed = 0;
		elevator->sector->floorspeed = 0;
		return;
	}

	if (elevator->distance != 0)
	{
		if (elevator->distance > 0) // Count down the timer
		{
			elevator->distance--;
			if (elevator->distance <= 0)
				elevator->distance = -15*TICRATE; // Timer until platform returns to original position.
			else
			{
				// Timer isn't up yet, so just keep waiting.
				elevator->sector->ceilspeed = 0;
				elevator->sector->floorspeed = 0;
				return;
			}
		}
		else if (++elevator->distance == 0) // Reposition back to original spot
		{
			for (i = -1; (i = P_FindSectorFromTag(elevator->sourceline->tag, i)) >= 0 ;)
			{
				sector = &sectors[i];

				for (rover = sector->ffloors; rover; rover = rover->next)
				{
					if (rover->flags & FF_CRUMBLE && rover->flags & FF_FLOATBOB
						&& rover->master == elevator->sourceline)
					{
						rover->alpha = elevator->origspeed;

						if (rover->alpha == 0xff)
							rover->flags &= ~FF_TRANSLUCENT;
					}
				}
			}

			// Up!
			if (elevator->floordestheight == 1)
				elevator->direction = -1;
			else
				elevator->direction = 1;

			elevator->sector->ceilspeed = 0;
			elevator->sector->floorspeed = 0;
			return;
		}

		// Flash to indicate that the platform is about to return.
		if (elevator->distance > -224 && (leveltime % ((abs(elevator->distance)/8) + 1) == 0))
		{
			for (i = -1; (i = P_FindSectorFromTag(elevator->sourceline->tag, i)) >= 0 ;)
			{
				sector = &sectors[i];

				for (rover = sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_NORETURN) && rover->flags & FF_CRUMBLE && rover->flags & FF_FLOATBOB
						&& rover->master == elevator->sourceline)
					{
						if (rover->alpha == elevator->origspeed)
						{
							rover->flags |= FF_TRANSLUCENT;
							rover->alpha = 0x00;
						}
						else
						{
							if (elevator->origspeed == 0xff)
								rover->flags &= ~FF_TRANSLUCENT;

							rover->alpha = elevator->origspeed;
						}
					}
				}
			}
		}

		// We're about to go back to the original position,
		// so set this to let other thinkers know what is
		// about to happen.
		if (elevator->distance < 0 && elevator->distance > -3)
			elevator->sector->crumblestate = 4; // makes T_BounceCheese remove itself
	}

	if ((elevator->floordestheight == 0 && elevator->direction == -1)
		|| (elevator->floordestheight == 1 && elevator->direction == 1)) // Down
	{
		elevator->sector->crumblestate = 3; // Allow floating now.

		// Only fall like this if it isn't meant to float on water
		if (elevator->high != 42)
		{
			elevator->speed += gravity; // Gain more and more speed

			if ((elevator->floordestheight == 0 && !(elevator->sector->ceilingheight < -16384*FRACUNIT))
				|| (elevator->floordestheight == 1 && !(elevator->sector->ceilingheight > 16384*FRACUNIT)))
			{
				fixed_t dest;

				if (elevator->floordestheight == 1)
					dest = elevator->sector->ceilingheight + (elevator->speed*2);
				else
					dest = elevator->sector->ceilingheight - (elevator->speed*2);

				T_MovePlane             //jff 4/7/98 reverse order of ceiling/floor
				(
				  elevator->sector,
				  elevator->speed,
				  dest,
				  0,
				  1, // move floor
				  elevator->direction
				);

				if (elevator->floordestheight == 1)
					dest = elevator->sector->floorheight + (elevator->speed*2);
				else
					dest = elevator->sector->floorheight - (elevator->speed*2);

				  T_MovePlane
				  (
					elevator->sector,
					elevator->speed,
					dest,
					0,
					0,                        // move ceiling
					elevator->direction
				);

				elevator->sector->ceilspeed = 42;
				elevator->sector->floorspeed = elevator->speed*elevator->direction;
			}
		}
	}
	else // Up (restore to original position)
	{
		elevator->sector->crumblestate = 1;
		elevator->sector->ceilingheight = elevator->ceilingwasheight;
		elevator->sector->floorheight = elevator->floorwasheight;
		elevator->sector->floordata = NULL;
		elevator->sector->ceilingdata = NULL;
		elevator->sector->ceilspeed = 0;
		elevator->sector->floorspeed = 0;
		elevator->sector->moved = true;
		P_RemoveThinker(&elevator->thinker);
	}

	for (i = -1; (i = P_FindSectorFromTag(elevator->sourceline->tag, i)) >= 0 ;)
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
void T_MarioBlock(levelspecthink_t *block)
{
	INT32 i;

#define speed vars[1]
#define direction vars[2]
#define floorwasheight vars[3]
#define ceilingwasheight vars[4]
#define distance vars[5]
#define low vars[6]

	T_MovePlane
	(
	  block->sector,
	  block->speed,
	  block->sector->ceilingheight + 70*FRACUNIT * block->direction,
	  0,
	  1, // move floor
	  block->direction
	);

	T_MovePlane
	(
	  block->sector,
	  block->speed,
	  block->sector->floorheight + 70*FRACUNIT * block->direction,
	  0,
	  0, // move ceiling
	  block->direction
	);

	if (block->sector->ceilingheight >= block->ceilingwasheight + 32*FRACUNIT) // Go back down now..
		block->direction = -block->direction;
	else if (block->sector->ceilingheight <= block->ceilingwasheight)
	{
		block->sector->ceilingheight = block->ceilingwasheight;
		block->sector->floorheight = block->floorwasheight;
		P_RemoveThinker(&block->thinker);
		block->sector->floordata = NULL;
		block->sector->ceilingdata = NULL;
		block->sector->floorspeed = 0;
		block->sector->ceilspeed = 0;
	}

	for (i = -1; (i = P_FindSectorFromTag((INT16)block->vars[0], i)) >= 0 ;)
		P_RecalcPrecipInSector(&sectors[i]);

#undef speed
#undef direction
#undef floorwasheight
#undef ceilingwasheight
#undef distance
#undef low
}

void T_SpikeSector(levelspecthink_t *spikes)
{
	mobj_t *thing;
	msecnode_t *node;
	boolean dothepain;
	sector_t *affectsec;

	node = spikes->sector->touching_thinglist; // things touching this sector

	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (!thing->player)
			continue;

		dothepain = false;
		affectsec = &sectors[spikes->vars[0]];

		if (affectsec == spikes->sector) // Applied to an actual sector
		{
			fixed_t affectfloor = P_GetSpecialBottomZ(thing, affectsec, affectsec);
			fixed_t affectceil = P_GetSpecialTopZ(thing, affectsec, affectsec);

			if (affectsec->flags & SF_FLIPSPECIAL_FLOOR)
			{
				if (!(thing->eflags & MFE_VERTICALFLIP) && thing->momz > 0)
					continue;

				if (thing->z == affectfloor)
					dothepain = true;
			}

			if (affectsec->flags & SF_FLIPSPECIAL_CEILING)
			{
				if ((thing->eflags & MFE_VERTICALFLIP) && thing->momz < 0)
					continue;

				if (thing->z + thing->height == affectceil)
					dothepain = true;
			}
		}
		else
		{
			fixed_t affectfloor = P_GetSpecialBottomZ(thing, affectsec, spikes->sector);
			fixed_t affectceil = P_GetSpecialTopZ(thing, affectsec, spikes->sector);
			if (affectsec->flags & SF_FLIPSPECIAL_FLOOR)
			{
				if (!(thing->eflags & MFE_VERTICALFLIP) && thing->momz > 0)
					continue;

				if (thing->z == affectceil)
					dothepain = true;
			}

			if (affectsec->flags & SF_FLIPSPECIAL_CEILING)
			{
				if ((thing->eflags & MFE_VERTICALFLIP) && thing->momz < 0)
					continue;

				if (thing->z + thing->height == affectfloor)
					dothepain = true;
			}
		}

		if (dothepain)
		{
			mobj_t *killer = P_SpawnMobj(thing->x, thing->y, thing->z, MT_NULL);
			killer->threshold = 43; // Special flag that it was spikes which hurt you.
			P_DamageMobj(thing, killer, killer, 1);
			break;
		}
	}
}

void T_FloatSector(levelspecthink_t *floater)
{
	fixed_t cheeseheight;
	sector_t *actionsector;
	INT32 secnum;

	cheeseheight = (floater->sector->ceilingheight + floater->sector->floorheight)>>1;

	// Just find the first sector with the tag.
	// Doesn't work with multiple sectors that have different floor/ceiling heights.
	secnum = P_FindSectorFromTag((INT16)floater->vars[0], -1);

	if (secnum > 0)
		actionsector = &sectors[secnum];
	else
		actionsector = NULL;

	if (actionsector)
	{
		//boolean floatanyway = false; // Ignore the crumblestate setting.
		fixed_t waterheight = P_SectorCheckWater(actionsector, floater->sector); // find the highest suitable water block around

		if (waterheight == cheeseheight) // same height, no floating needed
			;
		else if (floater->sector->floorheight == actionsector->floorheight && waterheight < cheeseheight) // too low
			;
		else if (floater->sector->ceilingheight == actionsector->ceilingheight && waterheight > cheeseheight) // too high
			;
		// we have something to float in! Or we're for some reason above the ground, let's fall anyway
		else if (floater->sector->crumblestate == 0 || floater->sector->crumblestate >= 3/* || floatanyway*/)
			EV_BounceSector(floater->sector, FRACUNIT, floater->sourceline);

		P_RecalcPrecipInSector(actionsector);
	}
}

//
// T_BridgeThinker
//
// Kind of like T_RaiseSector,
// but spreads out across
// multiple FOFs at varying
// intensity.
//
void T_BridgeThinker(levelspecthink_t *bridge)
{
	msecnode_t *node;
	mobj_t *thing;
	sector_t *sector;
	sector_t *controlsec = NULL;
	INT32 i, k;

	INT16 j;
	boolean playeronme = false;
	fixed_t ceilingdestination = 0, floordestination = 0;
	result_e res = 0;

#define ORIGFLOORHEIGHT (bridge->vars[0])
#define ORIGCEILINGHEIGHT (bridge->vars[1])
#define BASESPEED (bridge->vars[2])
#define CURSPEED (bridge->vars[3])
#define STARTTAG ((INT16)bridge->vars[4])
#define ENDTAG ((INT16)bridge->vars[5])
#define DIRECTION (bridge->vars[8])
#define SAGAMT (8*FRACUNIT)
	fixed_t lowceilheight = ORIGCEILINGHEIGHT - SAGAMT;
	fixed_t lowfloorheight = ORIGFLOORHEIGHT - SAGAMT;
#define LOWCEILINGHEIGHT (lowceilheight)
#define LOWFLOORHEIGHT (lowfloorheight)
#define STARTCONTROLTAG (ENDTAG + 1)
#define ENDCONTROLTAG (ENDTAG + (ENDTAG - STARTTAG) + 1)

	// Is someone standing on it?
	for (j = STARTTAG; j <= ENDTAG; j++)
	{
		for (i = -1; (i = P_FindSectorFromTag(j, i)) >= 0 ;)
		{
			sector = &sectors[i];

			// Nab the control sector that this sector belongs to.
			k = P_FindSectorFromTag((INT16)(j + (ENDTAG-STARTTAG) + 1), -1);

			if (k == -1)
				break;

			controlsec = &sectors[k];

			// Is a player standing on me?
			for (node = sector->touching_thinglist; node; node = node->m_thinglist_next)
			{
				thing = node->m_thing;

				if (!thing->player)
					continue;

				if (!(thing->z == controlsec->ceilingheight))
					continue;

				playeronme = true;
				goto wegotit; // Just take the first one?
			}
		}
	}
wegotit:
	if (playeronme)
	{
		// Lower controlsec like a regular T_RaiseSector
		// Set the heights of all the other control sectors to
		// be a gradient of this height toward the edges
	}
	else
	{
		// Raise controlsec like a regular T_RaiseSector
		// Set the heights of all the other control sectors to
		// be a gradient of this height toward the edges.
	}

	if (playeronme && controlsec)
	{
		INT32 dist;

		bridge->sector = controlsec;
		CURSPEED = BASESPEED;

		{
			// Translate tags to - 0 + range
			/*so you have a number in [min, max].
			let range = max - min, subtract min
			from your number to get [0, range].
			subtract range/2 to get [-range/2, range/2].
			take absolute value and get [0, range/2] where
			lower number = closer to midpoint. divide by
			range/2 to get [0, 1]. subtract that number
			from 1 to get [0, 1] with higher number = closer
			to midpoint. multiply this by max sag amount*/

			INT32 midpoint = STARTCONTROLTAG + ((ENDCONTROLTAG-STARTCONTROLTAG) + 1)/2;
//			INT32 tagstart = STARTTAG - midpoint;
//			INT32 tagend = ENDTAG - midpoint;

//			CONS_Debug(DBG_GAMELOGIC, "tagstart is %d, tagend is %d\n", tagstart, tagend);

			// Sag is adjusted by how close you are to the center
			dist = ((ENDCONTROLTAG - STARTCONTROLTAG))/2 - abs(bridge->sector->tag - midpoint);

//			CONS_Debug(DBG_GAMELOGIC, "Dist is %d\n", dist);
			LOWCEILINGHEIGHT -= (SAGAMT) * dist;
			LOWFLOORHEIGHT -= (SAGAMT) * dist;
		}

		// go down
		if (bridge->sector->ceilingheight <= LOWCEILINGHEIGHT)
		{
			bridge->sector->floorheight = LOWCEILINGHEIGHT - (bridge->sector->ceilingheight - bridge->sector->floorheight);
			bridge->sector->ceilingheight = LOWCEILINGHEIGHT;
			bridge->sector->ceilspeed = 0;
			bridge->sector->floorspeed = 0;
			goto dorest;
		}

		DIRECTION = -1;
		ceilingdestination = LOWCEILINGHEIGHT;
		floordestination = LOWFLOORHEIGHT;

		if ((bridge->sector->ceilingheight - LOWCEILINGHEIGHT)
			< (ORIGCEILINGHEIGHT - bridge->sector->ceilingheight))
		{
			fixed_t origspeed = CURSPEED;

			// Slow down as you get closer to the bottom
			CURSPEED = FixedMul(CURSPEED,FixedDiv(bridge->sector->ceilingheight - LOWCEILINGHEIGHT, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

			if (CURSPEED <= origspeed/16)
				CURSPEED = origspeed/16;
			else if (CURSPEED > origspeed)
				CURSPEED = origspeed;
		}
		else
		{
			fixed_t origspeed = CURSPEED;
			// Slow down as you get closer to the top
			CURSPEED = FixedMul(CURSPEED,FixedDiv(ORIGCEILINGHEIGHT - bridge->sector->ceilingheight, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

			if (CURSPEED <= origspeed/16)
				CURSPEED = origspeed/16;
			else if (CURSPEED > origspeed)
				CURSPEED = origspeed;
		}

//		CONS_Debug(DBG_GAMELOGIC, "Curspeed is %d\n", CURSPEED>>FRACBITS);

		res = T_MovePlane
		(
			bridge->sector,         // sector
			CURSPEED,          // speed
			ceilingdestination, // dest
			0,                        // crush
			1,                        // floor or ceiling (1 for ceiling)
			DIRECTION       // direction
		);

		if (res == ok || res == pastdest)
			T_MovePlane
			(
				bridge->sector,           // sector
				CURSPEED,            // speed
				floordestination, // dest
				0,                          // crush
				0,                          // floor or ceiling (0 for floor)
				DIRECTION         // direction
			);

		bridge->sector->ceilspeed = 42;
		bridge->sector->floorspeed = CURSPEED*DIRECTION;

	dorest:
		// Adjust joined sector heights
		{
			sector_t *sourcesec = bridge->sector;

			INT32 divisor = sourcesec->tag - ENDTAG + 1;
			fixed_t heightdiff = ORIGCEILINGHEIGHT - sourcesec->ceilingheight;
			fixed_t interval;
			INT32 plusplusme = 0;

			if (divisor > 0)
			{
				interval = heightdiff/divisor;

//				CONS_Debug(DBG_GAMELOGIC, "interval is %d\n", interval>>FRACBITS);

				// TODO: Use T_MovePlane

				for (j = (INT16)(ENDTAG+1); j <= sourcesec->tag; j++, plusplusme++)
				{
					for (i = -1; (i = P_FindSectorFromTag(j, i)) >= 0 ;)
					{
						if (sectors[i].ceilingheight >= sourcesec->ceilingheight)
						{
							sectors[i].ceilingheight = ORIGCEILINGHEIGHT - (interval*plusplusme);
							sectors[i].floorheight = ORIGFLOORHEIGHT - (interval*plusplusme);
						}
						else // Do the regular rise
						{
							bridge->sector = &sectors[i];

							CURSPEED = BASESPEED/2;

							// rise back up
							if (bridge->sector->ceilingheight >= ORIGCEILINGHEIGHT)
							{
								bridge->sector->floorheight = ORIGCEILINGHEIGHT - (bridge->sector->ceilingheight - bridge->sector->floorheight);
								bridge->sector->ceilingheight = ORIGCEILINGHEIGHT;
								bridge->sector->ceilspeed = 0;
								bridge->sector->floorspeed = 0;
								continue;
							}

							DIRECTION = 1;
							ceilingdestination = ORIGCEILINGHEIGHT;
							floordestination = ORIGFLOORHEIGHT;

//							CONS_Debug(DBG_GAMELOGIC, "ceildest: %d, floordest: %d\n", ceilingdestination>>FRACBITS, floordestination>>FRACBITS);

							if ((bridge->sector->ceilingheight - LOWCEILINGHEIGHT)
								< (ORIGCEILINGHEIGHT - bridge->sector->ceilingheight))
							{
								fixed_t origspeed = CURSPEED;

								// Slow down as you get closer to the bottom
								CURSPEED = FixedMul(CURSPEED,FixedDiv(bridge->sector->ceilingheight - LOWCEILINGHEIGHT, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

								if (CURSPEED <= origspeed/16)
									CURSPEED = origspeed/16;
								else if (CURSPEED > origspeed)
									CURSPEED = origspeed;
							}
							else
							{
								fixed_t origspeed = CURSPEED;
								// Slow down as you get closer to the top
								CURSPEED = FixedMul(CURSPEED,FixedDiv(ORIGCEILINGHEIGHT - bridge->sector->ceilingheight, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

								if (CURSPEED <= origspeed/16)
									CURSPEED = origspeed/16;
								else if (CURSPEED > origspeed)
									CURSPEED = origspeed;
							}

							res = T_MovePlane
							(
								bridge->sector,         // sector
								CURSPEED,          // speed
								ceilingdestination, // dest
								0,                        // crush
								1,                        // floor or ceiling (1 for ceiling)
								DIRECTION       // direction
							);

							if (res == ok || res == pastdest)
								T_MovePlane
								(
									bridge->sector,           // sector
									CURSPEED,            // speed
									floordestination, // dest
									0,                          // crush
									0,                          // floor or ceiling (0 for floor)
									DIRECTION         // direction
								);

							bridge->sector->ceilspeed = 42;
							bridge->sector->floorspeed = CURSPEED*DIRECTION;
						}
					}
				}
			}

			// Now the other side
			divisor = ENDTAG + (ENDTAG-STARTTAG) + 1;
			divisor -= sourcesec->tag;

			if (divisor > 0)
			{
				interval = heightdiff/divisor;
				plusplusme = 0;

//				CONS_Debug(DBG_GAMELOGIC, "interval2 is %d\n", interval>>FRACBITS);

				for (j = (INT16)(sourcesec->tag+1); j <= ENDTAG + (ENDTAG-STARTTAG) + 1; j++, plusplusme++)
				{
					for (i = -1; (i = P_FindSectorFromTag(j, i)) >= 0 ;)
					{
						if (sectors[i].ceilingheight >= sourcesec->ceilingheight)
						{
							sectors[i].ceilingheight = sourcesec->ceilingheight + (interval*plusplusme);
							sectors[i].floorheight = sourcesec->floorheight + (interval*plusplusme);
						}
						else // Do the regular rise
						{
							bridge->sector = &sectors[i];

							CURSPEED = BASESPEED/2;

							// rise back up
							if (bridge->sector->ceilingheight >= ORIGCEILINGHEIGHT)
							{
								bridge->sector->floorheight = ORIGCEILINGHEIGHT - (bridge->sector->ceilingheight - bridge->sector->floorheight);
								bridge->sector->ceilingheight = ORIGCEILINGHEIGHT;
								bridge->sector->ceilspeed = 0;
								bridge->sector->floorspeed = 0;
								continue;
							}

							DIRECTION = 1;
							ceilingdestination = ORIGCEILINGHEIGHT;
							floordestination = ORIGFLOORHEIGHT;

//							CONS_Debug(DBG_GAMELOGIC, "ceildest: %d, floordest: %d\n", ceilingdestination>>FRACBITS, floordestination>>FRACBITS);

							if ((bridge->sector->ceilingheight - LOWCEILINGHEIGHT)
								< (ORIGCEILINGHEIGHT - bridge->sector->ceilingheight))
							{
								fixed_t origspeed = CURSPEED;

								// Slow down as you get closer to the bottom
								CURSPEED = FixedMul(CURSPEED,FixedDiv(bridge->sector->ceilingheight - LOWCEILINGHEIGHT, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

								if (CURSPEED <= origspeed/16)
									CURSPEED = origspeed/16;
								else if (CURSPEED > origspeed)
									CURSPEED = origspeed;
							}
							else
							{
								fixed_t origspeed = CURSPEED;
								// Slow down as you get closer to the top
								CURSPEED = FixedMul(CURSPEED,FixedDiv(ORIGCEILINGHEIGHT - bridge->sector->ceilingheight, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

								if (CURSPEED <= origspeed/16)
									CURSPEED = origspeed/16;
								else if (CURSPEED > origspeed)
									CURSPEED = origspeed;
							}

							res = T_MovePlane
							(
								bridge->sector,         // sector
								CURSPEED,          // speed
								ceilingdestination, // dest
								0,                        // crush
								1,                        // floor or ceiling (1 for ceiling)
								DIRECTION       // direction
							);

							if (res == ok || res == pastdest)
								T_MovePlane
								(
									bridge->sector,           // sector
									CURSPEED,            // speed
									floordestination, // dest
									0,                          // crush
									0,                          // floor or ceiling (0 for floor)
									DIRECTION         // direction
								);

							bridge->sector->ceilspeed = 42;
							bridge->sector->floorspeed = CURSPEED*DIRECTION;
						}
					}
				}
			}
		}

	//	for (i = -1; (i = P_FindSectorFromTag(bridge->sourceline->tag, i)) >= 0 ;)
	//		P_RecalcPrecipInSector(&sectors[i]);
	}
	else
	{
		// Iterate control sectors
		for (j = (INT16)(ENDTAG+1); j <= (ENDTAG+(ENDTAG-STARTTAG)+1); j++)
		{
			for (i = -1; (i = P_FindSectorFromTag(j, i)) >= 0 ;)
			{
				bridge->sector = &sectors[i];

				CURSPEED = BASESPEED/2;

				// rise back up
				if (bridge->sector->ceilingheight >= ORIGCEILINGHEIGHT)
				{
					bridge->sector->floorheight = ORIGCEILINGHEIGHT - (bridge->sector->ceilingheight - bridge->sector->floorheight);
					bridge->sector->ceilingheight = ORIGCEILINGHEIGHT;
					bridge->sector->ceilspeed = 0;
					bridge->sector->floorspeed = 0;
					continue;
				}

				DIRECTION = 1;
				ceilingdestination = ORIGCEILINGHEIGHT;
				floordestination = ORIGFLOORHEIGHT;

//				CONS_Debug(DBG_GAMELOGIC, "ceildest: %d, floordest: %d\n", ceilingdestination>>FRACBITS, floordestination>>FRACBITS);

				if ((bridge->sector->ceilingheight - LOWCEILINGHEIGHT)
					< (ORIGCEILINGHEIGHT - bridge->sector->ceilingheight))
				{
					fixed_t origspeed = CURSPEED;

					// Slow down as you get closer to the bottom
					CURSPEED = FixedMul(CURSPEED,FixedDiv(bridge->sector->ceilingheight - LOWCEILINGHEIGHT, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

					if (CURSPEED <= origspeed/16)
						CURSPEED = origspeed/16;
					else if (CURSPEED > origspeed)
						CURSPEED = origspeed;
				}
				else
				{
					fixed_t origspeed = CURSPEED;
					// Slow down as you get closer to the top
					CURSPEED = FixedMul(CURSPEED,FixedDiv(ORIGCEILINGHEIGHT - bridge->sector->ceilingheight, (ORIGCEILINGHEIGHT - LOWCEILINGHEIGHT)>>5));

					if (CURSPEED <= origspeed/16)
						CURSPEED = origspeed/16;
					else if (CURSPEED > origspeed)
						CURSPEED = origspeed;
				}

				res = T_MovePlane
				(
					bridge->sector,         // sector
					CURSPEED,          // speed
					ceilingdestination, // dest
					0,                        // crush
					1,                        // floor or ceiling (1 for ceiling)
					DIRECTION       // direction
				);

				if (res == ok || res == pastdest)
					T_MovePlane
					(
						bridge->sector,           // sector
						CURSPEED,            // speed
						floordestination, // dest
						0,                          // crush
						0,                          // floor or ceiling (0 for floor)
						DIRECTION         // direction
					);

				bridge->sector->ceilspeed = 42;
				bridge->sector->floorspeed = CURSPEED*DIRECTION;
			}
		}
		// Update precip
	}

#undef SAGAMT
#undef LOWFLOORHEIGHT
#undef LOWCEILINGHEIGHT
#undef ORIGFLOORHEIGHT
#undef ORIGCEILINGHEIGHT
#undef BASESPEED
#undef CURSPEED
#undef STARTTAG
#undef ENDTAG
#undef DIRECTION
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
		case MT_THOK:
		case MT_GHOST:
		case MT_OVERLAY:
		case MT_EMERALDSPAWN:
		case MT_GREENORB:
		case MT_YELLOWORB:
		case MT_BLUEORB:
		case MT_BLACKORB:
		case MT_WHITEORB:
		case MT_PITYORB:
		case MT_IVSP:
		case MT_SUPERSPARK:
		case MT_RAIN:
		case MT_SNOWFLAKE:
		case MT_SPLISH:
		case MT_SMOKE:
		case MT_SMALLBUBBLE:
		case MT_MEDIUMBUBBLE:
		case MT_TFOG:
		case MT_SEED:
		case MT_PARTICLE:
		case MT_SCORE:
		case MT_DROWNNUMBERS:
		case MT_GOTEMERALD:
		case MT_TAG:
		case MT_GOTFLAG:
		case MT_GOTFLAG2:
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
		if (node->m_thing->flags & MF_MONITOR
		&& node->m_thing->threshold == 68)
			continue;
		// Okay, we found something valid.
		if (!thing // take either the first thing
		|| node->m_thing->x < thing->x // or the thing with the lowest x value (left to right item order)
		|| node->m_thing->y < thing->y) // or the thing with the lowest y value (top to bottom item order)
			thing = node->m_thing;
	}
	return thing;
}

void T_MarioBlockChecker(levelspecthink_t *block)
{
	line_t *masterline = block->sourceline;
	if (SearchMarioNode(block->sector->touching_thinglist))
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].bottomtexture;
	else
		sides[masterline->sidenum[0]].midtexture = sides[masterline->sidenum[0]].toptexture;
}

// This is the Thwomp's 'brain'. It looks around for players nearby, and if
// it finds any, **SMASH**!!! Muahahhaa....
void T_ThwompSector(levelspecthink_t *thwomp)
{
#define speed vars[1]
#define direction vars[2]
#define distance vars[3]
#define floorwasheight vars[4]
#define ceilingwasheight vars[5]
	fixed_t thwompx, thwompy;
	sector_t *actionsector;
	INT32 secnum;

	// If you just crashed down, wait a second before coming back up.
	if (--thwomp->distance > 0)
	{
		sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].bottomtexture;
		return;
	}

	// Just find the first sector with the tag.
	// Doesn't work with multiple sectors that have different floor/ceiling heights.
	secnum = P_FindSectorFromTag((INT16)thwomp->vars[0], -1);

	if (secnum > 0)
		actionsector = &sectors[secnum];
	else
		return; // Bad bad bad!

	thwompx = actionsector->soundorg.x;
	thwompy = actionsector->soundorg.y;

	if (thwomp->direction > 0) // Moving back up..
	{
		result_e res = 0;

		// Set the texture from the lower one (normal)
		sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].bottomtexture;
		/// \note this should only have to be done once, but is already done repeatedly, above

		if (thwomp->sourceline->flags & ML_EFFECT5)
			thwomp->speed = thwomp->sourceline->dx/8;
		else
			thwomp->speed = 2*FRACUNIT;

		res = T_MovePlane
		(
			thwomp->sector,         // sector
			thwomp->speed,          // speed
			thwomp->floorwasheight, // dest
			0,                      // crush
			0,                      // floor or ceiling (0 for floor)
			thwomp->direction       // direction
		);

		if (res == ok || res == pastdest)
			T_MovePlane
			(
				thwomp->sector,           // sector
				thwomp->speed,            // speed
				thwomp->ceilingwasheight, // dest
				0,                        // crush
				1,                        // floor or ceiling (1 for ceiling)
				thwomp->direction         // direction
			);

		if (res == pastdest)
			thwomp->direction = 0; // stop moving

		thwomp->sector->ceilspeed = 42;
		thwomp->sector->floorspeed = thwomp->speed*thwomp->direction;
	}
	else if (thwomp->direction < 0) // Crashing down!
	{
		result_e res = 0;

		// Set the texture from the upper one (angry)
		sides[thwomp->sourceline->sidenum[0]].midtexture = sides[thwomp->sourceline->sidenum[0]].toptexture;

		if (thwomp->sourceline->flags & ML_EFFECT5)
			thwomp->speed = thwomp->sourceline->dy/8;
		else
			thwomp->speed = 10*FRACUNIT;

		res = T_MovePlane
		(
			thwomp->sector,   // sector
			thwomp->speed,    // speed
			P_FloorzAtPos(thwompx, thwompy, thwomp->sector->floorheight,
				thwomp->sector->ceilingheight - thwomp->sector->floorheight), // dest
			0,                  // crush
			0,                  // floor or ceiling (0 for floor)
			thwomp->direction // direction
		);

		if (res == ok || res == pastdest)
			T_MovePlane
			(
				thwomp->sector,   // sector
				thwomp->speed,    // speed
				P_FloorzAtPos(thwompx, thwompy, thwomp->sector->floorheight,
					thwomp->sector->ceilingheight
					- (thwomp->sector->floorheight + thwomp->speed))
					+ (thwomp->sector->ceilingheight
					- (thwomp->sector->floorheight + thwomp->speed/2)), // dest
				0,                  // crush
				1,                  // floor or ceiling (1 for ceiling)
				thwomp->direction // direction
			);

		if (res == pastdest)
		{
			mobj_t *mp = (void *)&actionsector->soundorg;

			if (thwomp->sourceline->flags & ML_EFFECT4)
				S_StartSound(mp, sides[thwomp->sourceline->sidenum[0]].textureoffset>>FRACBITS);
			else
				S_StartSound(mp, sfx_thwomp);

			thwomp->direction = 1; // start heading back up
			thwomp->distance = TICRATE; // but only after a small delay
		}

		thwomp->sector->ceilspeed = 42;
		thwomp->sector->floorspeed = thwomp->speed*thwomp->direction;
	}
	else // Not going anywhere, so look for players.
	{
		thinker_t *th;
		mobj_t *mo;

		// scan the thinkers to find players!
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo = (mobj_t *)th;
			if (mo->type == MT_PLAYER && mo->health && mo->z <= thwomp->sector->ceilingheight
				&& P_AproxDistance(thwompx - mo->x, thwompy - mo->y) <= 96*FRACUNIT)
			{
				thwomp->direction = -1;
				break;
			}
		}

		thwomp->sector->ceilspeed = 0;
		thwomp->sector->floorspeed = 0;
	}

	P_RecalcPrecipInSector(actionsector);
#undef speed
#undef direction
#undef distance
#undef floorwasheight
#undef ceilingwasheight
}

//
// T_NoEnemiesThinker
//
// Runs a linedef exec when no more MF_ENEMY/MF_BOSS objects with health are in the area
// \sa P_AddNoEnemiesThinker
//
void T_NoEnemiesSector(levelspecthink_t *nobaddies)
{
	size_t i;
	fixed_t upperbound, lowerbound;
	INT32 s;
	sector_t *checksector;
	msecnode_t *node;
	mobj_t *thing;
	boolean exists = false;

	for (i = 0; i < nobaddies->sector->linecount; i++)
	{
		if (nobaddies->sector->lines[i]->special == 223)
		{

			upperbound = nobaddies->sector->ceilingheight;
			lowerbound = nobaddies->sector->floorheight;

			for (s = -1; (s = P_FindSectorFromLineTag(nobaddies->sector->lines[i], s)) >= 0 ;)
			{
				checksector = &sectors[s];

				node = checksector->touching_thinglist; // things touching this sector
				while (node)
				{
					thing = node->m_thing;

					if ((thing->flags & (MF_ENEMY|MF_BOSS)) && thing->health > 0
						&& thing->z < upperbound && thing->z+thing->height > lowerbound)
					{
						exists = true;
						goto foundenemy;
					}

					node = node->m_thinglist_next;
				}
			}
		}
	}
foundenemy:
	if (exists)
		return;

	s = P_AproxDistance(nobaddies->sourceline->dx, nobaddies->sourceline->dy)>>FRACBITS;

	CONS_Debug(DBG_GAMELOGIC, "Running no-more-enemies exec with tag of %d\n", s);

	// Otherwise, run the linedef exec and terminate this thinker
	P_LinedefExecute((INT16)s, NULL, NULL);
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

//
// P_HavePlayersEnteredArea
//
// Helper function for T_EachTimeThinker
//
static INT32 P_HavePlayersEnteredArea(boolean *curPlayers, boolean *oldPlayers, boolean inAndOut)
{
	INT32 i;

	// Easy check... nothing has changed
	if (!memcmp(curPlayers, oldPlayers, sizeof(boolean)*MAXPLAYERS))
		return -1;

	// Otherwise, we have to check if any new players have entered
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (inAndOut && !curPlayers[i] && oldPlayers[i])
			return i;

		if (curPlayers[i] && !oldPlayers[i])
			return i;
	}

	return -1;
}

//
// T_EachTimeThinker
//
// Runs a linedef exec whenever a player enters an area.
// Keeps track of players currently in the area and notices any changes.
//
// \sa P_AddEachTimeThinker
//
void T_EachTimeThinker(levelspecthink_t *eachtime)
{
	size_t i, j;
	sector_t *sec = NULL;
	sector_t *targetsec = NULL;
	//sector_t *usesec = NULL;
	INT32 secnum = -1;
	INT32 affectPlayer = 0;
	boolean oldPlayersInArea[MAXPLAYERS];
	boolean playersInArea[MAXPLAYERS];
	boolean oldPlayersOnArea[MAXPLAYERS];
	boolean playersOnArea[MAXPLAYERS];
	boolean *oldPlayersArea;
	boolean *playersArea;
	boolean FOFsector = false;
	boolean inAndOut = false;
	boolean floortouch = false;
	fixed_t bottomheight, topheight;
	msecnode_t *node;
	ffloor_t *rover;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (i & 1)
		{
			oldPlayersInArea[i] = eachtime->vars[i/2] & 65535;
			oldPlayersOnArea[i] = eachtime->var2s[i/2] & 65535;
			eachtime->vars[i/2] = 0;
			eachtime->var2s[i/2] = 0;
		}
		else
		{
			oldPlayersInArea[i] = eachtime->vars[i/2] >> 16;
			oldPlayersOnArea[i] = eachtime->var2s[i/2] >> 16;
		}

		playersInArea[i] = false;
		playersOnArea[i] = false;
	}

	while ((secnum = P_FindSectorFromLineTag(eachtime->sourceline, secnum)) >= 0)
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

			while ((targetsecnum = P_FindSectorFromLineTag(sec->lines[i], targetsecnum)) >= 0)
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
					if (!playeringame[j])
						continue;

					if (!players[j].mo)
						continue;

					if (players[j].mo->health <= 0)
						continue;

					if ((netgame || multiplayer) && players[j].spectator)
						continue;

					if (players[j].mo->subsector->sector == targetsec)
						;
					else if (sec->flags & SF_TRIGGERSPECIAL_TOUCH)
					{
						boolean insector = false;
						for (node = players[j].mo->touching_sectorlist; node; node = node->m_sectorlist_next)
						{
							if (node->m_sector == targetsec)
							{
								insector = true;
								break;
							}
						}
						if (!insector)
							continue;
					}
					else
						continue;

					topheight = P_GetSpecialTopZ(players[j].mo, sec, targetsec);
					bottomheight = P_GetSpecialBottomZ(players[j].mo, sec, targetsec);

					if (players[j].mo->z > topheight)
						continue;

					if (players[j].mo->z + players[j].mo->height < bottomheight)
						continue;

					if (floortouch == true && P_IsObjectOnGroundIn(players[j].mo, targetsec))
					{
						if (j & 1)
							eachtime->var2s[j/2] |= 1;
						else
							eachtime->var2s[j/2] |= 1 << 16;

						playersOnArea[j] = true;
					}
					else
					{
						if (j & 1)
							eachtime->vars[j/2] |= 1;
						else
							eachtime->vars[j/2] |= 1 << 16;

						playersInArea[j] = true;
					}
				}
			}
		}

		if (!FOFsector)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				if (!players[i].mo)
					continue;

				if (players[i].mo->health <= 0)
					continue;

				if ((netgame || multiplayer) && players[i].spectator)
					continue;

				if (players[i].mo->subsector->sector == sec)
					;
				else if (sec->flags & SF_TRIGGERSPECIAL_TOUCH)
				{
					boolean insector = false;
					for (node = players[i].mo->touching_sectorlist; node; node = node->m_sectorlist_next)
					{
						if (node->m_sector == sec)
						{
							insector = true;
							break;
						}
					}
					if (!insector)
						continue;
				}
				else
					continue;

				if (!(players[i].mo->subsector->sector == sec
					|| P_PlayerTouchingSectorSpecial(&players[i], 2, (GETSECSPECIAL(sec->special, 2))) == sec))
					continue;

				if (floortouch == true && P_IsObjectOnRealGround(players[i].mo, sec))
				{
					if (i & 1)
						eachtime->var2s[i/2] |= 1;
					else
						eachtime->var2s[i/2] |= 1 << 16;

					playersOnArea[i] = true;
				}
				else
				{
					if (i & 1)
						eachtime->vars[i/2] |= 1;
					else
						eachtime->vars[i/2] |= 1 << 16;

					playersInArea[i] = true;
				}
			}
		}
	}

	if ((eachtime->sourceline->flags & ML_BOUNCY) == ML_BOUNCY)
		inAndOut = true;

	// Check if a new player entered.
	// If not, check if a player hit the floor.
	// If either condition is true, execute.
	if (floortouch == true)
	{
		playersArea = playersOnArea;
		oldPlayersArea = oldPlayersOnArea;
	}
	else
	{
		playersArea = playersInArea;
		oldPlayersArea = oldPlayersInArea;
	}

	while ((affectPlayer = P_HavePlayersEnteredArea(playersArea, oldPlayersArea, inAndOut)) != -1)
	{
		if (GETSECSPECIAL(sec->special, 2) == 2 || GETSECSPECIAL(sec->special, 2) == 3)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				if (!players[i].mo)
					continue;

				if (players[i].mo->health <= 0)
					continue;

				if ((netgame || multiplayer) && players[i].spectator)
					continue;

				if (!playersArea[i])
					return;
			}
		}

		CONS_Debug(DBG_GAMELOGIC, "Trying to activate each time executor with tag %d\n", eachtime->sourceline->tag);

		// 03/08/14 -Monster Iestyn
		// No more stupid hacks involving changing eachtime->sourceline's tag or special or whatever!
		// This should now run ONLY the stuff for eachtime->sourceline itself, instead of all trigger linedefs sharing the same tag.
		// Makes much more sense doing it this way, honestly.
		P_RunTriggerLinedef(eachtime->sourceline, players[affectPlayer].mo, sec);

		if (!eachtime->sourceline->special) // this happens only for "Trigger on X calls" linedefs
			P_RemoveThinker(&eachtime->thinker);

		oldPlayersArea[affectPlayer]=playersArea[affectPlayer];
	}
}

//
// T_RaiseSector
//
// Rises up to its topmost position when a
// player steps on it. Lowers otherwise.
//
void T_RaiseSector(levelspecthink_t *raise)
{
	msecnode_t *node;
	mobj_t *thing;
	sector_t *sector;
	INT32 i;
	boolean playeronme = false;
	fixed_t ceilingdestination, floordestination;
	result_e res = 0;

	if (raise->sector->crumblestate >= 3 || raise->sector->ceilingdata)
		return;

	for (i = -1; (i = P_FindSectorFromTag(raise->sourceline->tag, i)) >= 0 ;)
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
			if (raise->vars[1] && !(thing->player->pflags & PF_STARTDASH))
				continue;

			if (!(thing->z == P_GetSpecialTopZ(thing, raise->sector, sector)))
				continue;

			playeronme = true;
			break;
		}
	}

	if (playeronme)
	{
		raise->vars[3] = raise->vars[2];

		if (raise->vars[0] == 1)
		{
			if (raise->sector->ceilingheight <= raise->vars[7])
			{
				raise->sector->floorheight = raise->vars[7] - (raise->sector->ceilingheight - raise->sector->floorheight);
				raise->sector->ceilingheight = raise->vars[7];
				raise->sector->ceilspeed = 0;
				raise->sector->floorspeed = 0;
				return;
			}

			raise->vars[8] = -1;
			ceilingdestination = raise->vars[7];
			floordestination = raise->vars[6];
		}
		else // elevateUp
		{
			if (raise->sector->ceilingheight >= raise->vars[5])
			{
				raise->sector->floorheight = raise->vars[5] - (raise->sector->ceilingheight - raise->sector->floorheight);
				raise->sector->ceilingheight = raise->vars[5];
				raise->sector->ceilspeed = 0;
				raise->sector->floorspeed = 0;
				return;
			}

			raise->vars[8] = 1;
			ceilingdestination = raise->vars[5];
			floordestination = raise->vars[4];
		}
	}
	else
	{
		raise->vars[3] = raise->vars[2]/2;

		if (raise->vars[0] == 1)
		{
			if (raise->sector->ceilingheight >= raise->vars[5])
			{
				raise->sector->floorheight = raise->vars[5] - (raise->sector->ceilingheight - raise->sector->floorheight);
				raise->sector->ceilingheight = raise->vars[5];
				raise->sector->ceilspeed = 0;
				raise->sector->floorspeed = 0;
				return;
			}
			raise->vars[8] = 1;
			ceilingdestination = raise->vars[5];
			floordestination = raise->vars[4];
		}
		else // elevateUp
		{
			if (raise->sector->ceilingheight <= raise->vars[7])
			{
				raise->sector->floorheight = raise->vars[7] - (raise->sector->ceilingheight - raise->sector->floorheight);
				raise->sector->ceilingheight = raise->vars[7];
				raise->sector->ceilspeed = 0;
				raise->sector->floorspeed = 0;
				return;
			}
			raise->vars[8] = -1;
			ceilingdestination = raise->vars[7];
			floordestination = raise->vars[6];
		}
	}

	if ((raise->sector->ceilingheight - raise->vars[7])
		< (raise->vars[5] - raise->sector->ceilingheight))
	{
		fixed_t origspeed = raise->vars[3];

		// Slow down as you get closer to the bottom
		raise->vars[3] = FixedMul(raise->vars[3],FixedDiv(raise->sector->ceilingheight - raise->vars[7], (raise->vars[5] - raise->vars[7])>>5));

		if (raise->vars[3] <= origspeed/16)
			raise->vars[3] = origspeed/16;
		else if (raise->vars[3] > origspeed)
			raise->vars[3] = origspeed;
	}
	else
	{
		fixed_t origspeed = raise->vars[3];
		// Slow down as you get closer to the top
		raise->vars[3] = FixedMul(raise->vars[3],FixedDiv(raise->vars[5] - raise->sector->ceilingheight, (raise->vars[5] - raise->vars[7])>>5));

		if (raise->vars[3] <= origspeed/16)
			raise->vars[3] = origspeed/16;
		else if (raise->vars[3] > origspeed)
			raise->vars[3] = origspeed;
	}

	res = T_MovePlane
	(
		raise->sector,         // sector
		raise->vars[3],          // speed
		ceilingdestination, // dest
		0,                        // crush
		1,                        // floor or ceiling (1 for ceiling)
		raise->vars[8]       // direction
	);

	if (res == ok || res == pastdest)
		T_MovePlane
		(
			raise->sector,           // sector
			raise->vars[3],            // speed
			floordestination, // dest
			0,                          // crush
			0,                          // floor or ceiling (0 for floor)
			raise->vars[8]         // direction
		);

	raise->sector->ceilspeed = 42;
	raise->sector->floorspeed = raise->vars[3]*raise->vars[8];

	for (i = -1; (i = P_FindSectorFromTag(raise->sourceline->tag, i)) >= 0 ;)
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

//
// EV_DoFloor
//
// Set up and start a floor thinker.
//
// Called by P_ProcessLineSpecial (linedef executors), P_ProcessSpecialSector
// (egg capsule button), P_PlayerInSpecialSector (buttons),
// and P_SpawnSpecials (continuous floor movers and instant lower).
//
INT32 EV_DoFloor(line_t *line, floor_e floortype)
{
	INT32 rtn = 0, firstone = 1;
	INT32 secnum = -1;
	sector_t *sec;
	floormove_t *dofloor;

	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		if (sec->floordata) // if there's already a thinker on this floor,
			continue; // then don't add another one

		// new floor thinker
		rtn = 1;
		dofloor = Z_Calloc(sizeof (*dofloor), PU_LEVSPEC, NULL);
		P_AddThinker(&dofloor->thinker);

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

	return rtn;
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
INT32 EV_DoElevator(line_t *line, elevator_e elevtype, boolean customspeed)
{
	INT32 secnum = -1;
	INT32 rtn = 0;
	sector_t *sec;
	elevator_t *elevator;

	// act on all sectors with the same tag as the triggering linedef
	while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
	{
		sec = &sectors[secnum];

		// If either floor or ceiling is already activated, skip it
		if (sec->floordata || sec->ceilingdata)
			continue;

		// create and initialize new elevator thinker
		rtn = 1;
		elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
		P_AddThinker(&elevator->thinker);
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
	return rtn;
}

void EV_CrumbleChain(sector_t *sec, ffloor_t *rover)
{
	size_t i;
	size_t leftmostvertex = 0, rightmostvertex = 0;
	size_t topmostvertex = 0, bottommostvertex = 0;
	fixed_t leftx, rightx;
	fixed_t topy, bottomy;
	fixed_t topz;
	fixed_t a, b, c;
	mobjtype_t type = MT_ROCKCRUMBLE1;

	// If the control sector has a special
	// of Section3:7-15, use the custom debris.
	if (GETSECSPECIAL(rover->master->frontsector->special, 3) >= 8)
		type = MT_ROCKCRUMBLE1+(GETSECSPECIAL(rover->master->frontsector->special, 3)-7);

	// soundorg z height never gets set normally, so MEH.
	sec->soundorg.z = sec->floorheight;
	S_StartSound(&sec->soundorg, sfx_crumbl);

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

	leftx = sec->lines[leftmostvertex]->v1->x+(16<<FRACBITS);
	rightx = sec->lines[rightmostvertex]->v1->x;
	topy = sec->lines[topmostvertex]->v1->y-(16<<FRACBITS);
	bottomy = sec->lines[bottommostvertex]->v1->y;
	topz = *rover->topheight-(16<<FRACBITS);

	for (a = leftx; a < rightx; a += (32<<FRACBITS))
	{
		for (b = topy; b > bottomy; b -= (32<<FRACBITS))
		{
			if (R_PointInSubsector(a, b)->sector == sec)
			{
				mobj_t *spawned = NULL;
				for (c = topz; c > *rover->bottomheight; c -= (32<<FRACBITS))
				{
					spawned = P_SpawnMobj(a, b, c, type);
					spawned->fuse = 3*TICRATE;
				}
			}
		}
	}

	// no longer exists (can't collide with again)
	rover->flags &= ~FF_EXISTS;
	rover->master->frontsector->moved = true;
	sec->moved = true;
}

// Used for bobbing platforms on the water
INT32 EV_BounceSector(sector_t *sec, fixed_t momz, line_t *sourceline)
{
#define speed vars[0]
#define distance vars[1]
#define low vars[2]
	levelspecthink_t *bouncer;

	// create and initialize new thinker
	if (sec->ceilingdata) // One at a time, ma'am.
		return 0;

	bouncer = Z_Calloc(sizeof (*bouncer), PU_LEVSPEC, NULL);
	P_AddThinker(&bouncer->thinker);
	sec->ceilingdata = bouncer;
	bouncer->thinker.function.acp1 = (actionf_p1)T_BounceCheese;

	// set up the fields according to the type of elevator action
	bouncer->sector = sec;
	bouncer->speed = momz/2;
	bouncer->sourceline = sourceline;
	bouncer->distance = FRACUNIT;
	bouncer->low = 1;

	return 1;
#undef speed
#undef distance
#undef low
}

// For T_ContinuousFalling special
INT32 EV_DoContinuousFall(sector_t *sec, sector_t *backsector, fixed_t spd, boolean backwards)
{
#define speed vars[0]
#define direction vars[1]
#define floorwasheight vars[2]
#define ceilingwasheight vars[3]
#define floordestheight vars[4]
#define ceilingdestheight vars[5]
	levelspecthink_t *faller;

	// workaround for when there is no back sector
	if (backsector == NULL)
		backsector = sec;

	// create and initialize new thinker
	faller = Z_Calloc(sizeof (*faller), PU_LEVSPEC, NULL);
	P_AddThinker(&faller->thinker);
	faller->thinker.function.acp1 = (actionf_p1)T_ContinuousFalling;

	// set up the fields
	faller->sector = sec;
	faller->speed = spd;

	faller->floorwasheight = sec->floorheight;
	faller->ceilingwasheight = sec->ceilingheight;

	if (backwards)
	{
		faller->ceilingdestheight = backsector->ceilingheight;
		faller->floordestheight = faller->ceilingdestheight;
		faller->direction = 1; // Up!
	}
	else
	{
		faller->floordestheight = backsector->floorheight;
		faller->ceilingdestheight = faller->floordestheight;
		faller->direction = -1;
	}

	return 1;
#undef speed
#undef direction
#undef floorwasheight
#undef ceilingwasheight
#undef floordestheight
#undef ceilingdestheight
}

// Some other 3dfloor special things Tails 03-11-2002 (Search p_mobj.c for description)
INT32 EV_StartCrumble(sector_t *sec, ffloor_t *rover, boolean floating,
	player_t *player, fixed_t origalpha, boolean crumblereturn)
{
	elevator_t *elevator;
	sector_t *foundsec;
	INT32 i;

	// If floor is already activated, skip it
	if (sec->floordata)
		return 0;

	if (sec->crumblestate > 1)
		return 0;

	// create and initialize new elevator thinker
	elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
	P_AddThinker(&elevator->thinker);
	elevator->thinker.function.acp1 = (actionf_p1)T_StartCrumble;

	// Does this crumbler return?
	if (crumblereturn)
		elevator->type = elevateBounce;
	else
		elevator->type = elevateContinuous;

	// set up the fields according to the type of elevator action
	elevator->sector = sec;
	elevator->speed = 0;

	if (player && player->mo && (player->mo->eflags & MFE_VERTICALFLIP))
	{
		elevator->direction = 1; // Up
		elevator->floordestheight = 1;
	}
	else
	{
		elevator->direction = -1; // Down
		elevator->floordestheight = 0;
	}

	elevator->floorwasheight = elevator->sector->floorheight;
	elevator->ceilingwasheight = elevator->sector->ceilingheight;
	elevator->distance = TICRATE; // Used for delay time
	elevator->low = 0;
	elevator->player = player;
	elevator->origspeed = origalpha;

	elevator->sourceline = rover->master;

	sec->floordata = elevator;

	if (floating)
		elevator->high = 42;
	else
		elevator->high = 0;

	elevator->sector->crumblestate = 2;

	for (i = -1; (i = P_FindSectorFromTag(elevator->sourceline->tag, i)) >= 0 ;)
	{
		foundsec = &sectors[i];

		P_SpawnMobj(foundsec->soundorg.x, foundsec->soundorg.y, elevator->direction == 1 ? elevator->sector->floorheight : elevator->sector->ceilingheight, MT_CRUMBLEOBJ);
	}

	return 1;
}

INT32 EV_MarioBlock(sector_t *sec, sector_t *roversector, fixed_t topheight, mobj_t *puncher)
{
	levelspecthink_t *block;
	mobj_t *thing;
	fixed_t oldx = 0, oldy = 0, oldz = 0;

	I_Assert(puncher != NULL);
	I_Assert(puncher->player != NULL);

	if (sec->floordata || sec->ceilingdata)
		return 0;

	// Find an item to pop out!
	thing = SearchMarioNode(sec->touching_thinglist);

	// Found something!
	if (thing)
	{
		const boolean itsamonitor = (thing->flags & MF_MONITOR) == MF_MONITOR;
		// create and initialize new elevator thinker

		block = Z_Calloc(sizeof (*block), PU_LEVSPEC, NULL);
		P_AddThinker(&block->thinker);
		sec->floordata = block;
		sec->ceilingdata = block;
		block->thinker.function.acp1 = (actionf_p1)T_MarioBlock;

		// Set up the fields
		block->sector = sec;
		block->vars[0] = roversector->tag; // actionsector
		block->vars[1] = 4*FRACUNIT; // speed
		block->vars[2] = 1; // Up // direction
		block->vars[3] = block->sector->floorheight; // floorwasheight
		block->vars[4] = block->sector->ceilingheight; // ceilingwasheight
		block->vars[5] = FRACUNIT; // distance
		block->vars[6] = 1; // low

		if (itsamonitor)
		{
			oldx = thing->x;
			oldy = thing->y;
			oldz = thing->z;
		}

		P_UnsetThingPosition(thing);
		thing->x = roversector->soundorg.x;
		thing->y = roversector->soundorg.y;
		thing->z = topheight;
		thing->momz = FixedMul(6*FRACUNIT, thing->scale);
		P_SetThingPosition(thing);
		if (thing->flags & MF_SHOOTABLE)
			P_DamageMobj(thing, puncher, puncher, 1);
		else if (thing->type == MT_RING || thing->type == MT_COIN)
		{
			thing->momz = FixedMul(3*FRACUNIT, thing->scale);
			P_TouchSpecialThing(thing, puncher, false);
			// "Thunk!" sound
			S_StartSound(puncher, sfx_mario1); // Puncher is "close enough"
		}
		else
		{
			if (thing->type == MT_EMMY && thing->spawnpoint && (thing->spawnpoint->options & MTF_OBJECTSPECIAL))
			{
				mobj_t *tokenobj = P_SpawnMobj(roversector->soundorg.x, roversector->soundorg.y, topheight, MT_TOKEN);
				P_SetTarget(&thing->tracer, tokenobj);
				P_SetTarget(&tokenobj->target, thing);
				P_SetMobjState(tokenobj, mobjinfo[MT_TOKEN].seestate);
			}

			// "Powerup rise" sound
			S_StartSound(puncher, sfx_mario9); // Puncher is "close enough"
		}

		if (itsamonitor)
		{
			P_UnsetThingPosition(tmthing);
			tmthing->x = oldx;
			tmthing->y = oldy;
			tmthing->z = oldz;
			tmthing->momx = 1;
			tmthing->momy = 1;
			P_SetThingPosition(tmthing);
		}
	}
	else
		S_StartSound(puncher, sfx_mario1); // "Thunk!" sound - puncher is "close enough".

	return 1;
}
