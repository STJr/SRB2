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
/// \file  p_ceilng.c
/// \brief Ceiling aninmation (lowering, crushing, raising)

#include "doomdef.h"
#include "p_local.h"
#include "r_fps.h"
#include "r_main.h"
#include "s_sound.h"
#include "z_zone.h"
#include "d_netcmd.h"

// ==========================================================================
//                              CEILINGS
// ==========================================================================

// the list of ceilings moving currently, including crushers
INT32 ceilmovesound = sfx_None;

/** Moves a moving ceiling.
  *
  * \param ceiling Thinker for the ceiling to be moved.
  * \sa EV_DoCeiling
  * \todo Split up into multiple functions.
  */
void T_MoveCeiling(ceiling_t *ceiling)
{
	result_e res;

	if (ceiling->delaytimer)
	{
		ceiling->delaytimer--;
		return;
	}

	res = T_MovePlane(ceiling->sector, ceiling->speed, (ceiling->direction == 1) ? ceiling->topheight : ceiling->bottomheight, false, true, ceiling->direction);

	if (ceiling->type == bounceCeiling)
	{
		const fixed_t origspeed = FixedDiv(ceiling->origspeed, (ELEVATORSPEED/2));
		const fixed_t fs = abs(ceiling->sector->ceilingheight - lines[ceiling->sourceline].frontsector->ceilingheight);
		const fixed_t bs = abs(ceiling->sector->ceilingheight - lines[ceiling->sourceline].backsector->ceilingheight);
		if (fs < bs)
			ceiling->speed = FixedDiv(fs, 25*FRACUNIT) + FRACUNIT/4;
		else
			ceiling->speed = FixedDiv(bs, 25*FRACUNIT) + FRACUNIT/4;
		ceiling->speed = FixedMul(ceiling->speed, origspeed);
	}

	if (res == pastdest)
	{
		switch (ceiling->type)
		{
			case instantMoveCeilingByFrontSector:
				if (ceiling->texture > -1) // flat changing
					ceiling->sector->ceilingpic = ceiling->texture;
				ceiling->sector->ceilingdata = NULL;
				ceiling->sector->ceilspeed = 0;
				P_RemoveThinker(&ceiling->thinker);
				return;
			case moveCeilingByFrontSector:
				if (ceiling->tag) // chained linedef executing
					P_LinedefExecute(ceiling->tag, NULL, NULL);
				if (ceiling->texture > -1) // flat changing
					ceiling->sector->ceilingpic = ceiling->texture;
				/* FALLTHRU */
			case raiseToHighest:
			case moveCeilingByDistance:
				ceiling->sector->ceilingdata = NULL;
				ceiling->sector->ceilspeed = 0;
				P_RemoveThinker(&ceiling->thinker);
				return;
			case bounceCeiling:
			case bounceCeilingCrush:
			{
				fixed_t dest = (ceiling->direction == 1) ? ceiling->topheight : ceiling->bottomheight;

				if (dest == lines[ceiling->sourceline].frontsector->ceilingheight)
				{
					dest = lines[ceiling->sourceline].backsector->ceilingheight;
					ceiling->origspeed = lines[ceiling->sourceline].args[3] << (FRACBITS - 2); // return trip, use args[3]
				}
				else
				{
					dest = lines[ceiling->sourceline].frontsector->ceilingheight;
					ceiling->origspeed = lines[ceiling->sourceline].args[2] << (FRACBITS - 2); // going frontways, use args[2]
				}

				if (ceiling->type == bounceCeilingCrush)
					ceiling->speed = ceiling->origspeed;

				if (dest < ceiling->sector->ceilingheight) // must move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = dest;
				}
				else // must move up
				{
					ceiling->direction = 1;
					ceiling->topheight = dest;
				}

				ceiling->delaytimer = ceiling->delay;
				break;
			}
			default:
				break;
		}
	}
	ceiling->sector->ceilspeed = ceiling->speed*ceiling->direction;
}

/** Moves a ceiling crusher.
  *
  * \param ceiling Thinker for the crusher to be moved.
  * \sa EV_DoCrush
  */
void T_CrushCeiling(ceiling_t *ceiling)
{
	result_e res;

	switch (ceiling->direction)
	{
		case 0: // IN STASIS
			break;
		case 1: // UP
			if (ceiling->type == crushBothOnce)
			{
				// Move the floor
				T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight-(ceiling->topheight-ceiling->bottomheight), false, false, -ceiling->direction);
			}

			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->topheight, false, true, ceiling->direction);

			if (res == pastdest)
			{
				ceiling->direction = -1;
				ceiling->speed = lines[ceiling->sourceline].args[2] << (FRACBITS - 2);

				if (ceiling->type == crushCeilOnce
					|| ceiling->type == crushBothOnce)
				{
					// Remove
					switch(ceiling->type)
					{
						case crushCeilOnce:
							ceiling->sector->ceilspeed = 0;
							ceiling->sector->ceilingdata = NULL;
							break;
						case crushBothOnce:
							ceiling->sector->floorspeed = 0;
							ceiling->sector->ceilspeed = 0;
							ceiling->sector->ceilingdata = NULL;
							break;
						default:
							break;
					}
					P_RemoveThinker(&ceiling->thinker);
					return;
				}
			}
			break;

		case -1: // DOWN
			if (ceiling->type == crushBothOnce)
			{
				// Move the floor
				T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight, ceiling->crush, false, -ceiling->direction);
			}

			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight, ceiling->crush, true, ceiling->direction);

			if (res == pastdest)
			{
				mobj_t *mp = (void *)&ceiling->sector->soundorg;
				ceiling->sector->soundorg.z = ceiling->sector->floorheight;
				S_StartSound(mp,sfx_pstop);

				ceiling->direction = 1;
				ceiling->speed = lines[ceiling->sourceline].args[3] << (FRACBITS - 2);
			}
			break;
	}

	if (ceiling->type == crushBothOnce)
		ceiling->sector->floorspeed = ceiling->speed*(-ceiling->direction);

	ceiling->sector->ceilspeed = ceiling->speed*ceiling->direction;
}

/** Starts a ceiling mover.
  *
  * \param tag Tag.
  * \param line The source line.
  * \param type The type of ceiling movement.
  * \return 1 if at least one ceiling mover was started, 0 otherwise.
  * \sa EV_DoCrush, EV_DoFloor, EV_DoElevator, T_MoveCeiling
  */
INT32 EV_DoCeiling(mtag_t tag, line_t *line, ceiling_e type)
{
	INT32 rtn = 0, firstone = 1;
	INT32 secnum = -1;
	sector_t *sec;
	ceiling_t *ceiling;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sec = &sectors[secnum];

		if (sec->ceilingdata)
			continue;

		// new door thinker
		rtn = 1;
		ceiling = Z_Calloc(sizeof (*ceiling), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &ceiling->thinker);
		sec->ceilingdata = ceiling;
		ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
		ceiling->sector = sec;
		ceiling->crush = false;
		ceiling->sourceline = (INT32)(line-lines);

		switch (type)
		{
			case raiseToHighest:
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				ceiling->speed = CEILSPEED;
				break;

			case lowerToLowestFast:
				ceiling->bottomheight = P_FindLowestCeilingSurrounding(sec);
				ceiling->direction = -1;
				ceiling->speed = 4*FRACUNIT;
				break;

			case instantRaise:
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				ceiling->speed = INT32_MAX/2;
				break;

			//  Linedef executor excellence
			case moveCeilingByFrontSector:
				ceiling->speed = line->args[2] << (FRACBITS - 3);
				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}

				// chained linedef executing ability
				// only set it on ONE of the moving sectors (the smallest numbered)
				// only set it if there isn't also a floor mover
				if (line->args[3] && line->args[1] == 1)
					ceiling->tag = firstone ? (INT16)line->args[3] : 0;

				// flat changing ability
				ceiling->texture = line->args[4] ? line->frontsector->ceilingpic : -1;
				break;

			// More linedef executor junk
			case instantMoveCeilingByFrontSector:
				ceiling->speed = INT32_MAX/2;

				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}

				// If flag is set, change ceiling texture after moving
				ceiling->texture = line->args[2] ? line->frontsector->ceilingpic : -1;
				break;

			case moveCeilingByDistance:
				if (line->args[4])
					ceiling->speed = INT32_MAX/2; // as above, "instant" is one tic
				else
					ceiling->speed = line->args[3] << (FRACBITS - 3);
				if (line->args[2] > 0)
				{
					ceiling->direction = 1; // up
					ceiling->topheight = sec->ceilingheight + (line->args[2] << FRACBITS);
				}
				else {
					ceiling->direction = -1; // down
					ceiling->bottomheight = sec->ceilingheight + (line->args[2] << FRACBITS);
				}
				break;

			case bounceCeiling:
			case bounceCeilingCrush:
				ceiling->speed = line->args[2] << (FRACBITS - 2); // same speed as elevateContinuous
				ceiling->origspeed = ceiling->speed;
				if (line->frontsector->ceilingheight >= sec->ceilingheight) // Move up
				{
					ceiling->direction = 1;
					ceiling->topheight = line->frontsector->ceilingheight;
				}
				else // Move down
				{
					ceiling->direction = -1;
					ceiling->bottomheight = line->frontsector->ceilingheight;
				}

				// Any delay?
				ceiling->delay = line->args[5];
				ceiling->delaytimer = line->args[4]; // Initial delay
				break;

			default:
				break;

		}

		ceiling->type = type;
		firstone = 0;

		// interpolation
		R_CreateInterpolator_SectorPlane(&ceiling->thinker, sec, true);
	}
	return rtn;
}

/** Starts a ceiling crusher.
  *
  * \param tag Tag.
  * \param line The source line.
  * \param type The type of ceiling, either ::crushAndRaise or
  *             ::fastCrushAndRaise.
  * \return 1 if at least one crusher was started, 0 otherwise.
  * \sa EV_DoCeiling, EV_DoFloor, EV_DoElevator, T_CrushCeiling
  */
INT32 EV_DoCrush(mtag_t tag, line_t *line, ceiling_e type)
{
	INT32 rtn = 0;
	INT32 secnum = -1;
	sector_t *sec;
	ceiling_t *ceiling;

	TAG_ITER_SECTORS(tag, secnum)
	{
		sec = &sectors[secnum];

		if (sec->ceilingdata)
			continue;

		// new door thinker
		rtn = 1;
		ceiling = Z_Calloc(sizeof (*ceiling), PU_LEVSPEC, NULL);
		P_AddThinker(THINK_MAIN, &ceiling->thinker);
		sec->ceilingdata = ceiling;
		ceiling->thinker.function.acp1 = (actionf_p1)T_CrushCeiling;
		ceiling->sector = sec;
		ceiling->crush = true;
		ceiling->sourceline = (INT32)(line-lines);
		ceiling->speed = ceiling->origspeed = line->args[2] << (FRACBITS - 2);

		switch(type)
		{
			case raiseAndCrush: // Up and then down
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				// Retain stupid behavior for backwards compatibility
				if (!udmf && !(line->flags & ML_MIDSOLID))
					ceiling->speed /= 2;
				else
					ceiling->speed = line->args[3] << (FRACBITS - 2);
				ceiling->bottomheight = sec->floorheight + FRACUNIT;
				break;
			case crushBothOnce:
				ceiling->topheight = sec->ceilingheight;
				ceiling->bottomheight = sec->floorheight + (sec->ceilingheight-sec->floorheight)/2;
				ceiling->direction = -1;
				break;
			case crushCeilOnce:
			default: // Down and then up.
				ceiling->topheight = sec->ceilingheight;
				ceiling->direction = -1;
				ceiling->bottomheight = sec->floorheight + FRACUNIT;
				break;
		}

		ceiling->type = type;

		// interpolation
		R_CreateInterpolator_SectorPlane(&ceiling->thinker, sec, false);
		R_CreateInterpolator_SectorPlane(&ceiling->thinker, sec, true);
	}
	return rtn;
}
