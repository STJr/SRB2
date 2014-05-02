// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2014 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_mobj.c
/// \brief Moving object handling. Spawn functions

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "p_local.h"
#include "p_setup.h"
#include "r_main.h"
#include "r_things.h"
#include "r_sky.h"
#include "r_splats.h"
#include "s_sound.h"
#include "z_zone.h"
#include "m_random.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "info.h"
#include "i_video.h"
#include "lua_hook.h"
#include "b_bot.h"

// protos.
static CV_PossibleValue_t viewheight_cons_t[] = {{16, "MIN"}, {56, "MAX"}, {0, NULL}};
consvar_t cv_viewheight = {"viewheight", VIEWHEIGHTS, 0, viewheight_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_splats = {"splats", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

actioncache_t actioncachehead;

static mobj_t *overlaycap = NULL;

void P_InitCachedActions(void)
{
	actioncachehead.prev = actioncachehead.next = &actioncachehead;
}

void P_RunCachedActions(void)
{
	actioncache_t *ac;
	actioncache_t *next;

	for (ac = actioncachehead.next; ac != &actioncachehead; ac = next)
	{
		var1 = states[ac->statenum].var1;
		var2 = states[ac->statenum].var2;
#ifdef HAVE_BLUA
		astate = &states[ac->statenum];
#endif
		if (ac->mobj && !P_MobjWasRemoved(ac->mobj)) // just in case...
			states[ac->statenum].action.acp1(ac->mobj);
		next = ac->next;
		Z_Free(ac);
	}
}

void P_AddCachedAction(mobj_t *mobj, INT32 statenum)
{
	actioncache_t *newaction = Z_Calloc(sizeof(actioncache_t), PU_LEVEL, NULL);
	newaction->mobj = mobj;
	newaction->statenum = statenum;
	actioncachehead.prev->next = newaction;
	newaction->next = &actioncachehead;
	newaction->prev = actioncachehead.prev;
	actioncachehead.prev = newaction;
}

//
// P_CycleMobjState
//
static void P_CycleMobjState(mobj_t *mobj)
{
	// cycle through states,
	// calling action functions at transitions
	if (mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if (!mobj->tics && mobj->state)
			if (!P_SetMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

//
// P_CycleMobjState for players.
//
static void P_CyclePlayerMobjState(mobj_t *mobj)
{
	// cycle through states,
	// calling action functions at transitions
	if (mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if (!mobj->tics && mobj->state)
			if (!P_SetPlayerMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

//
// P_SetPlayerMobjState
// Returns true if the mobj is still present.
//
// Separate from P_SetMobjState because of the pw_flashing check and Super states
//
boolean P_SetPlayerMobjState(mobj_t *mobj, statenum_t state)
{
	state_t *st;
	player_t *player = mobj->player;

	// remember states seen, to detect cycles:
	static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
	statenum_t *seenstate = seenstate_tab; // pointer to table
	static INT32 recursion; // detects recursion
	statenum_t i; // initial state
	statenum_t tempstate[NUMSTATES]; // for use with recursion

#ifdef PARANOIA
	if (player == NULL)
		I_Error("P_SetPlayerMobjState used for non-player mobj. Use P_SetMobjState instead!\n(Mobj type: %d, State: %d)", mobj->type, state);
#endif

	// Catch state changes for Super Sonic
	if (player->powers[pw_super] && (player->charflags & SF_SUPERANIMS))
	{
		switch (state)
		{
		case S_PLAY_STND:
		case S_PLAY_TAP1:
		case S_PLAY_TAP2:
		case S_PLAY_GASP:
			P_SetPlayerMobjState(mobj, S_PLAY_SUPERSTAND);
			return true;
		case S_PLAY_FALL1:
		case S_PLAY_SPRING:
		case S_PLAY_RUN1:
		case S_PLAY_RUN2:
		case S_PLAY_RUN3:
		case S_PLAY_RUN4:
			P_SetPlayerMobjState(mobj, S_PLAY_SUPERWALK1);
			return true;
		case S_PLAY_FALL2:
		case S_PLAY_RUN5:
		case S_PLAY_RUN6:
		case S_PLAY_RUN7:
		case S_PLAY_RUN8:
			P_SetPlayerMobjState(mobj, S_PLAY_SUPERWALK2);
			return true;
		case S_PLAY_SPD1:
		case S_PLAY_SPD2:
			P_SetPlayerMobjState(mobj, S_PLAY_SUPERFLY1);
			return true;
		case S_PLAY_SPD3:
		case S_PLAY_SPD4:
			P_SetPlayerMobjState(mobj, S_PLAY_SUPERFLY2);
			return true;
		case S_PLAY_TEETER1:
		case S_PLAY_TEETER2:
			P_SetPlayerMobjState(mobj, S_PLAY_SUPERTEETER);
			return true;
		case S_PLAY_ATK1:
		case S_PLAY_ATK2:
		case S_PLAY_ATK3:
		case S_PLAY_ATK4:
			if (!(player->charflags & SF_SUPERSPIN))
				return true;
			break;
		default:
			break;
		}
	}
	// You were in pain state after taking a hit, and you're moving out of pain state now?
	else if (mobj->state == &states[mobj->info->painstate] && player->powers[pw_flashing] == flashingtics && state != mobj->info->painstate)
	{
		// Start flashing, since you've landed.
		player->powers[pw_flashing] = flashingtics-1;
		P_DoPityCheck(player);
	}

	// Set animation state
	// The pflags version of this was just as convoluted.
	if ((state >= S_PLAY_STND && state <= S_PLAY_TAP2) || (state >= S_PLAY_TEETER1 && state <= S_PLAY_TEETER2) || state == S_PLAY_CARRY
	|| state == S_PLAY_SUPERSTAND || state == S_PLAY_SUPERTEETER)
		player->panim = PA_IDLE;
	else if ((state >= S_PLAY_RUN1 && state <= S_PLAY_RUN8)
	|| (state >= S_PLAY_SUPERWALK1 && state <= S_PLAY_SUPERWALK2))
		player->panim = PA_WALK;
	else if ((state >= S_PLAY_SPD1 && state <= S_PLAY_SPD4)
	|| (state >= S_PLAY_SUPERFLY1 && state <= S_PLAY_SUPERFLY2))
		player->panim = PA_RUN;
	else if (state >= S_PLAY_ATK1 && state <= S_PLAY_ATK4)
		player->panim = PA_ROLL;
	else if (state >= S_PLAY_FALL1 && state <= S_PLAY_FALL2)
		player->panim = PA_FALL;
	else if (state >= S_PLAY_ABL1 && state <= S_PLAY_ABL2)
		player->panim = PA_ABILITY;
	else
		player->panim = PA_ETC;

	if (recursion++) // if recursion detected,
		memset(seenstate = tempstate, 0, sizeof tempstate); // clear state table

	i = state;

	do
	{
		if (state == S_NULL)
		{ // Bad SOC!
			CONS_Alert(CONS_ERROR, "Cannot remove player mobj by setting its state to S_NULL.\n");
			//P_RemoveMobj(mobj);
			return false;
		}

		st = &states[state];
		mobj->state = st;
		mobj->tics = st->tics;

		// Adjust the player's animation speed to match their velocity.
		if (!(disableSpeedAdjust || player->charflags & SF_NOSPEEDADJUST))
		{
			fixed_t speed = FixedDiv(player->speed, mobj->scale);
			if (player->panim == PA_ROLL)
			{
				if (speed > 16<<FRACBITS)
					mobj->tics = 1;
				else
					mobj->tics = 2;
			}
			else if (player->panim == PA_FALL)
			{
				speed = FixedDiv(abs(mobj->momz), mobj->scale);
				if (speed < 10<<FRACBITS)
					mobj->tics = 4;
				else if (speed < 20<<FRACBITS)
					mobj->tics = 3;
				else if (speed < 30<<FRACBITS)
					mobj->tics = 2;
				else
					mobj->tics = 1;
			}
			else if (P_IsObjectOnGround(mobj) || player->powers[pw_super]) // Only if on the ground or superflying.
			{
				if (player->panim == PA_WALK)
				{
					if (speed > 12<<FRACBITS)
						mobj->tics = 2;
					else if (speed > 6<<FRACBITS)
						mobj->tics = 3;
					else
						mobj->tics = 4;
				}
				else if (player->panim == PA_RUN)
				{
					if (speed > 52<<FRACBITS)
						mobj->tics = 1;
					else
						mobj->tics = 2;
				}
			}
		}

		mobj->sprite = st->sprite;
		mobj->frame = st->frame;

		// Modified handling.
		// Call action functions when the state is set

		if (st->action.acp1)
		{
			var1 = st->var1;
			var2 = st->var2;
#ifdef HAVE_BLUA
			astate = st;
#endif
			st->action.acp1(mobj);

			// woah. a player was removed by an action.
			// this sounds like a VERY BAD THING, but there's nothing we can do now...
			if (P_MobjWasRemoved(mobj))
				return false;
		}

		seenstate[state] = 1 + st->nextstate;

		state = st->nextstate;
	} while (!mobj->tics && !seenstate[state]);

	if (!mobj->tics)
		CONS_Alert(CONS_WARNING, M_GetText("State cycle detected, exiting.\n"));

	if (!--recursion)
		for (;(state = seenstate[i]) > S_NULL; i = state - 1)
			seenstate[i] = S_NULL; // erase memory of states

	return true;
}


boolean P_SetMobjState(mobj_t *mobj, statenum_t state)
{
	state_t *st;

	// remember states seen, to detect cycles:
	static statenum_t seenstate_tab[NUMSTATES]; // fast transition table
	statenum_t *seenstate = seenstate_tab; // pointer to table
	static INT32 recursion; // detects recursion
	statenum_t i = state; // initial state
	statenum_t tempstate[NUMSTATES]; // for use with recursion

#ifdef PARANOIA
	if (mobj->player != NULL)
		I_Error("P_SetMobjState used for player mobj. Use P_SetPlayerMobjState instead!\n(State called: %d)", state);
#endif

	if (recursion++) // if recursion detected,
		memset(seenstate = tempstate, 0, sizeof tempstate); // clear state table

	do
	{
		if (state == S_NULL)
		{
			P_RemoveMobj(mobj);
			return false;
		}

		st = &states[state];
		mobj->state = st;
		mobj->tics = st->tics;
		mobj->sprite = st->sprite;
		mobj->frame = st->frame;

		// Modified handling.
		// Call action functions when the state is set

		if (st->action.acp1)
		{
			var1 = st->var1;
			var2 = st->var2;
#ifdef HAVE_BLUA
			astate = st;
#endif
			st->action.acp1(mobj);
			if (P_MobjWasRemoved(mobj))
				return false;
		}

		seenstate[state] = 1 + st->nextstate;

		state = st->nextstate;
	} while (!mobj->tics && !seenstate[state]);

	if (!mobj->tics)
		CONS_Alert(CONS_WARNING, M_GetText("State cycle detected, exiting.\n"));

	if (!--recursion)
		for (;(state = seenstate[i]) > S_NULL; i = state - 1)
			seenstate[i] = S_NULL; // erase memory of states

	return true;
}

//----------------------------------------------------------------------------
//
// FUNC P_SetMobjStateNF
//
// Same as P_SetMobjState, but does not call the state function.
//
//----------------------------------------------------------------------------

boolean P_SetMobjStateNF(mobj_t *mobj, statenum_t state)
{
	state_t *st;

	if (state == S_NULL)
	{ // Remove mobj
		P_RemoveMobj(mobj);
		return false;
	}
	st = &states[state];
	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame;
	return true;
}

static boolean P_SetPrecipMobjState(precipmobj_t *mobj, statenum_t state)
{
	state_t *st;

	if (state == S_NULL)
	{ // Remove mobj
		P_RemovePrecipMobj(mobj);
		return false;
	}
	st = &states[state];
	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame;
	return true;
}

//
// P_MobjFlip
//
// Special utility to return +1 or -1 depending on mobj's gravity
//
SINT8 P_MobjFlip(mobj_t *mobj)
{
	if (mobj && mobj->eflags & MFE_VERTICALFLIP)
		return -1;
	return 1;
}

//
// P_WeaponOrPanel
//
// Returns true if weapon ring/panel; otherwise returns false
//
boolean P_WeaponOrPanel(mobjtype_t type)
{
	if (type == MT_BOUNCERING
	|| type == MT_AUTOMATICRING
	|| type == MT_INFINITYRING
	|| type == MT_RAILRING
	|| type == MT_EXPLOSIONRING
	|| type == MT_SCATTERRING
	|| type == MT_GRENADERING
	|| type == MT_BOUNCEPICKUP
	|| type == MT_RAILPICKUP
	|| type == MT_AUTOPICKUP
	|| type == MT_EXPLODEPICKUP
	|| type == MT_SCATTERPICKUP
	|| type == MT_GRENADEPICKUP)
		return true;

	return false;
}

//
// P_EmeraldManager
//
// Power Stone emerald management
//
void P_EmeraldManager(void)
{
	thinker_t *think;
	mobj_t *mo;
	INT32 i,j;
	INT32 numtospawn;
	INT32 emeraldsspawned = 0;

	boolean hasemerald[MAXHUNTEMERALDS];
	INT32 numwithemerald = 0;

	// record empty spawn points
	mobj_t *spawnpoints[MAXHUNTEMERALDS];
	INT32 numspawnpoints = 0;

	for (i = 0; i < MAXHUNTEMERALDS; i++)
	{
		hasemerald[i] = false;
		spawnpoints[i] = NULL;
	}

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;

		if (mo->type == MT_EMERALDSPAWN)
		{
			if (mo->threshold || mo->target) // Either has the emerald spawned or is spawning
			{
				numwithemerald++;
				emeraldsspawned |= mobjinfo[mo->reactiontime].speed;
			}
			else if (numspawnpoints < MAXHUNTEMERALDS)
				spawnpoints[numspawnpoints++] = mo; // empty spawn points
		}
		else if (mo->type == MT_FLINGEMERALD)
		{
			numwithemerald++;
			emeraldsspawned |= mo->threshold;
		}
	}

	if (numspawnpoints == 0)
		return;

	// But wait! We need to check all the players too, to see if anyone has some of the emeralds.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		if (!players[i].mo)
			continue;

		if (players[i].powers[pw_emeralds] & EMERALD1)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD1;
		}
		if (players[i].powers[pw_emeralds] & EMERALD2)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD2;
		}
		if (players[i].powers[pw_emeralds] & EMERALD3)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD3;
		}
		if (players[i].powers[pw_emeralds] & EMERALD4)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD4;
		}
		if (players[i].powers[pw_emeralds] & EMERALD5)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD5;
		}
		if (players[i].powers[pw_emeralds] & EMERALD6)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD6;
		}
		if (players[i].powers[pw_emeralds] & EMERALD7)
		{
			numwithemerald++;
			emeraldsspawned |= EMERALD7;
		}
	}

	// All emeralds spawned, no worries
	if (numwithemerald >= 7)
		return;

	// Set up spawning for the emeralds
	numtospawn = 7 - numwithemerald;

#ifdef PARANOIA
	if (numtospawn <= 0) // ???
		I_Error("P_EmeraldManager: numtospawn is %d!\n", numtospawn);
#endif

	for (i = 0, j = 0; i < numtospawn; i++)
	{
		INT32 tries = 0;
		while (true)
		{
			tries++;

			if (tries > 50)
				break;

			j = P_RandomKey(numspawnpoints);

			if (hasemerald[j])
				continue;

			hasemerald[j] = true;

			if (!(emeraldsspawned & EMERALD1))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD1;
				emeraldsspawned |= EMERALD1;
			}
			else if (!(emeraldsspawned & EMERALD2))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD2;
				emeraldsspawned |= EMERALD2;
			}
			else if (!(emeraldsspawned & EMERALD3))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD3;
				emeraldsspawned |= EMERALD3;
			}
			else if (!(emeraldsspawned & EMERALD4))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD4;
				emeraldsspawned |= EMERALD4;
			}
			else if (!(emeraldsspawned & EMERALD5))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD5;
				emeraldsspawned |= EMERALD5;
			}
			else if (!(emeraldsspawned & EMERALD6))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD6;
				emeraldsspawned |= EMERALD6;
			}
			else if (!(emeraldsspawned & EMERALD7))
			{
				spawnpoints[j]->reactiontime = MT_EMERALD7;
				emeraldsspawned |= EMERALD7;
			}
			else
				break;

			if (leveltime < TICRATE) // Start of map
				spawnpoints[j]->threshold = 60*TICRATE + P_Random() * (TICRATE/5);
			else
				spawnpoints[j]->threshold = P_Random() * (TICRATE/5);

			break;
		}
	}
}

//
// P_ExplodeMissile
//
void P_ExplodeMissile(mobj_t *mo)
{
	mobj_t *explodemo;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	mo->momx = mo->momy = mo->momz = 0;

	if (mo->flags & MF_NOCLIPTHING)
		return;

	if (mo->type == MT_DETON)
	{
		P_RadiusAttack(mo, mo, 96*FRACUNIT);

		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx += (P_Random() % 32) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy += (P_Random() % 32) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_pop);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx += (P_Random() % 64) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy -= (P_Random() % 64) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_dmpain);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx -= (P_Random() % 128) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy += (P_Random() % 128) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_pop);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx -= (P_Random() % 96) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy -= (P_Random() % 96) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_cybdth);
	}

	mo->flags &= ~MF_MISSILE;

	mo->flags |= MF_NOGRAVITY; // Dead missiles don't need to sink anymore.
	mo->flags |= MF_NOCLIPTHING; // Dummy flag to indicate that this was already called.

	if (mo->info->deathsound && !(mo->flags2 & MF2_DEBRIS))
		S_StartSound(mo, mo->info->deathsound);

	P_SetMobjState(mo, mo->info->deathstate);
}

// P_InsideANonSolidFFloor
//
// Returns TRUE if mobj is inside a non-solid 3d floor.
boolean P_InsideANonSolidFFloor(mobj_t *mobj, ffloor_t *rover)
{
	if (!(rover->flags & FF_EXISTS))
		return false;

	if ((((rover->flags & FF_BLOCKPLAYER) && mobj->player)
		|| ((rover->flags & FF_BLOCKOTHERS) && !mobj->player)))
		return false;

	if (mobj->z > *rover->topheight)
		return false;

	if (mobj->z + mobj->height < *rover->bottomheight)
		return false;

	return true;
}

//
// P_CheckGravity
//
// Checks the current gravity state
// of the object. If affect is true,
// a gravity force will be applied.
//
void P_CheckGravity(mobj_t *mo, boolean affect)
{
	fixed_t gravityadd = 0;
	boolean no3dfloorgrav = true; // Custom gravity
	boolean goopgravity = false;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (mo->type != MT_SPINFIRE)
		mo->eflags &= ~MFE_VERTICALFLIP;

	if (mo->subsector->sector->ffloors) // Check for 3D floor gravity too.
	{
		ffloor_t *rover;

		for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (P_InsideANonSolidFFloor(mo, rover))
			{
				if ((rover->flags & (FF_SWIMMABLE|FF_GOOWATER)) == (FF_SWIMMABLE|FF_GOOWATER))
					goopgravity = true;
				if (rover->master->frontsector->gravity)
				{
					gravityadd = -FixedMul(gravity,
						(FixedDiv(*rover->master->frontsector->gravity>>FRACBITS, 1000)));

					if (rover->master->frontsector->verticalflip && gravityadd > 0)
						mo->eflags |= MFE_VERTICALFLIP;

					no3dfloorgrav = false;
					break;
				}
			}
		}
	}

	if (no3dfloorgrav)
	{
		if (mo->subsector->sector->gravity)
			gravityadd = -FixedMul(gravity,
				(FixedDiv(*mo->subsector->sector->gravity>>FRACBITS, 1000)));
		else
			gravityadd = -gravity;

		if (mo->subsector->sector->verticalflip && gravityadd > 0)
			mo->eflags |= MFE_VERTICALFLIP;
	}

	// Less gravity underwater.
	if (mo->eflags & MFE_UNDERWATER && !goopgravity)
		gravityadd = gravityadd/3;

	if (!mo->momz) // mobj at stop, no floor, so feel the push of gravity!
		gravityadd <<= 1;

	if (mo->player)
	{
		if (mo->player->charability == CA_FLY && (mo->player->powers[pw_tailsfly]
		|| (mo->state >= &states[S_PLAY_SPC1] && mo->state <= &states[S_PLAY_SPC4])))
			gravityadd = gravityadd/3; // less gravity while flying
		if (mo->player->pflags & PF_GLIDING)
			gravityadd = gravityadd/3; // less gravity while gliding
		if (mo->player->climbing)
			gravityadd = 0;
		if (mo->player->pflags & PF_NIGHTSMODE)
			gravityadd = 0;

		{
			UINT8 bits = 0;
			if (mo->flags2 & MF2_OBJECTFLIP)
				bits ^= 1;
			if (mo->player->powers[pw_gravityboots])
				bits ^= 1;
			if (bits & 1)
			{
				gravityadd = -gravityadd;
				if (mo->eflags & MFE_VERTICALFLIP)
					mo->eflags &= ~MFE_VERTICALFLIP;
				else
					mo->eflags |= MFE_VERTICALFLIP;
			}
		}
	}
	else
	{
		// Objects with permanent reverse gravity (MF2_OBJECTFLIP)
		if (mo->flags2 & MF2_OBJECTFLIP)
		{
			mo->eflags |= MFE_VERTICALFLIP;
			if (gravityadd < 0) // Don't sink, only rise up
				gravityadd *= -1;
			if (mo->z + mo->height >= mo->ceilingz)
				gravityadd = 0;
		}
		else //Otherwise, sort through the other exceptions.
		{
			switch (mo->type)
			{
				case MT_FLINGRING:
				case MT_FLINGCOIN:
				case MT_FLINGEMERALD:
				case MT_BOUNCERING:
				case MT_RAILRING:
				case MT_INFINITYRING:
				case MT_AUTOMATICRING:
				case MT_EXPLOSIONRING:
				case MT_SCATTERRING:
				case MT_GRENADERING:
				case MT_BOUNCEPICKUP:
				case MT_RAILPICKUP:
				case MT_AUTOPICKUP:
				case MT_EXPLODEPICKUP:
				case MT_SCATTERPICKUP:
				case MT_GRENADEPICKUP:
				case MT_REDFLAG:
				case MT_BLUEFLAG:
					if (mo->target)
					{
						// Flung items copy the gravity of their tosser.
						if ((mo->target->eflags & MFE_VERTICALFLIP) && !(mo->eflags & MFE_VERTICALFLIP))
						{
							gravityadd = -gravityadd;
							mo->eflags |= MFE_VERTICALFLIP;
						}
					}
					break;
				case MT_WATERDROP:
					gravityadd >>= 1;
				default:
					break;
			}
		}
	}

	// Goop has slower, reversed gravity
	if (goopgravity)
		gravityadd = -gravityadd/5;

	if (affect)
		mo->momz += FixedMul(gravityadd, mo->scale);

	if (mo->type == MT_SKIM && mo->z + mo->momz <= mo->watertop && mo->z >= mo->watertop)
	{
		mo->momz = 0;
		mo->flags |= MF_NOGRAVITY;
	}
}

#define STOPSPEED (FRACUNIT)
#define FRICTION (ORIG_FRICTION) // 0.90625

//
// P_SceneryXYFriction
//
static void P_SceneryXYFriction(mobj_t *mo, fixed_t oldx, fixed_t oldy)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (abs(mo->momx) < FixedMul(STOPSPEED/32, mo->scale)
		&& abs(mo->momy) < FixedMul(STOPSPEED/32, mo->scale))
	{
		mo->momx = 0;
		mo->momy = 0;
	}
	else
	{
		if ((oldx == mo->x) && (oldy == mo->y)) // didn't go anywhere
		{
			mo->momx = FixedMul(mo->momx,ORIG_FRICTION);
			mo->momy = FixedMul(mo->momy,ORIG_FRICTION);
		}
		else
		{
			mo->momx = FixedMul(mo->momx,mo->friction);
			mo->momy = FixedMul(mo->momy,mo->friction);
		}

		if (mo->type == MT_CANNONBALLDECOR)
		{
			// Stolen from P_SpawnFriction
			mo->friction = FRACUNIT - 0x100;
			mo->movefactor = ((0x10092 - mo->friction)*(0x70))/0x158;
		}
		else
			mo->friction = ORIG_FRICTION;
	}
}

//
// P_XYFriction
//
// adds friction on the xy plane
//
static void P_XYFriction(mobj_t *mo, fixed_t oldx, fixed_t oldy)
{
	player_t *player;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	player = mo->player;
	if (player) // valid only if player avatar
	{
		if (abs(player->rmomx) < FixedMul(STOPSPEED, mo->scale)
			&& abs(player->rmomy) < FixedMul(STOPSPEED, mo->scale)
			&& (!(player->cmd.forwardmove && !(twodlevel || (player->mo->flags2 & MF2_TWOD))) && !player->cmd.sidemove && !(player->pflags & PF_SPINNING)))
		{
			// if in a walking frame, stop moving
			if (player && player->panim == PA_WALK)
				P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			mo->momx = player->cmomx;
			mo->momy = player->cmomy;
		}
		else
		{
			if ((oldx == mo->x) && (oldy == mo->y)) // didn't go anywhere
			{
				mo->momx = FixedMul(mo->momx, ORIG_FRICTION);
				mo->momy = FixedMul(mo->momy, ORIG_FRICTION);
			}
			else
			{
				mo->momx = FixedMul(mo->momx, mo->friction);
				mo->momy = FixedMul(mo->momy, mo->friction);
			}

			if (mo->momx || mo->momy)
			{
				INT32 direction = P_GetPlayerControlDirection(player);

				if (direction == 2)
				{
					mo->momx >>= 1;
					mo->momy >>= 1;
				}
			}

			mo->friction = ORIG_FRICTION;
		}
	}
	else
		P_SceneryXYFriction(mo, oldx, oldy);
}

static void P_PushableCheckBustables(mobj_t *mo)
{
	msecnode_t *node;
	fixed_t oldx;
	fixed_t oldy;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (netgame && mo->player && mo->player->spectator)
		return;

	oldx = mo->x;
	oldy = mo->y;

	P_UnsetThingPosition(mo);
	mo->x += mo->momx;
	mo->y += mo->momy;
	P_SetThingPosition(mo);

	for (node = mo->touching_sectorlist; node; node = node->m_snext)
	{
		if (!node->m_sector)
			break;

		if (node->m_sector->ffloors)
		{
			ffloor_t *rover;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS)) continue;

				if (!(rover->flags & FF_BUSTUP)) continue;

				// Needs ML_EFFECT4 flag for pushables to break it
				if (!(rover->master->flags & ML_EFFECT4)) continue;

				if (!rover->master->frontsector->crumblestate)
				{
					// Height checks
					if (rover->flags & FF_SHATTERBOTTOM)
					{
						if (mo->z+mo->momz + mo->height < *rover->bottomheight)
							continue;

						if (mo->z+mo->height > *rover->bottomheight)
							continue;
					}
					else if (rover->flags & FF_SPINBUST)
					{
						if (mo->z+mo->momz > *rover->topheight)
							continue;

						if (mo->z+mo->height < *rover->bottomheight)
							continue;
					}
					else if (rover->flags & FF_SHATTER)
					{
						if (mo->z+mo->momz > *rover->topheight)
							continue;

						if (mo->z+mo->momz + mo->height < *rover->bottomheight)
							continue;
					}
					else
					{
						if (mo->z >= *rover->topheight)
							continue;

						if (mo->z+mo->height < *rover->bottomheight)
							continue;
					}

					EV_CrumbleChain(node->m_sector, rover);

					// Run a linedef executor??
					if (rover->master->flags & ML_EFFECT5)
						P_LinedefExecute((INT16)(P_AproxDistance(rover->master->dx, rover->master->dy)>>FRACBITS), mo, node->m_sector);

					goto bustupdone;
				}
			}
		}
	}
bustupdone:
	P_UnsetThingPosition(mo);
	mo->x = oldx;
	mo->y = oldy;
	P_SetThingPosition(mo);
}

//
// P_CheckSkyHit
//
static boolean P_CheckSkyHit(mobj_t *mo)
{
	if (ceilingline && ceilingline->backsector
		&& ceilingline->backsector->ceilingpic == skyflatnum
		&& ceilingline->frontsector
		&& ceilingline->frontsector->ceilingpic == skyflatnum
		&& (mo->z >= ceilingline->frontsector->ceilingheight
		|| mo->z >= ceilingline->backsector->ceilingheight))
			return true;
	return false;
}

//
// P_XYMovement
//
void P_XYMovement(mobj_t *mo)
{
	player_t *player;
	fixed_t xmove, ymove;
	fixed_t oldx, oldy; // reducing bobbing/momentum on ice when up against walls
	boolean moved;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	moved = true;

	// if it's stopped
	if (!mo->momx && !mo->momy)
	{
		if (mo->flags2 & MF2_SKULLFLY)
		{
			// the skull slammed into something
			mo->flags2 &= ~MF2_SKULLFLY;
			mo->momx = mo->momy = mo->momz = 0;

			// set in 'search new direction' state?
			if (mo->type != MT_EGGMOBILE)
				P_SetMobjState(mo, mo->info->spawnstate);

			return;
		}
	}

	player = mo->player; //valid only if player avatar

	xmove = mo->momx;
	ymove = mo->momy;

	oldx = mo->x;
	oldy = mo->y;

	// Pushables can break some blocks
	if (CheckForBustableBlocks && (mo->flags & MF_PUSHABLE))
		P_PushableCheckBustables(mo);

	if (!P_TryMove(mo, mo->x + xmove, mo->y + ymove, true) && !tmsprung)
	{
		// blocked move

		if (mo->player) {
			moved = false;
			if (mo->player->bot)
				B_MoveBlocked(mo->player);
		}

		if (mo->flags & MF_BOUNCE)
		{
			P_BounceMove(mo);
			xmove = ymove = 0;
			S_StartSound(mo, mo->info->activesound);

			// Bounce ring algorithm
			if (mo->type == MT_THROWNBOUNCE)
			{
				mo->threshold++;

				// Gain lower amounts of time on each bounce.
				if (mo->threshold < 5)
					mo->fuse += ((5 - mo->threshold) * TICRATE);

				// Check for hit against sky here
				if (P_CheckSkyHit(mo))
				{
					// Hack to prevent missiles exploding
					// against the sky.
					// Does not handle sky floors.
					// Check frontsector as well.

					P_RemoveMobj(mo);
					return;
				}
			}
		}
		else if (mo->flags & MF_STICKY)
		{
			S_StartSound(mo, mo->info->activesound);
			mo->momx = mo->momy = mo->momz = 0; //Full stop!
			mo->flags |= MF_NOGRAVITY; //Stay there!
			mo->flags &= ~MF_STICKY; //Don't check again!

			// Check for hit against sky here
			if (P_CheckSkyHit(mo))
			{
				// Hack to prevent missiles exploding
				// against the sky.
				// Does not handle sky floors.
				// Check frontsector as well.

				P_RemoveMobj(mo);
				return;
			}
		}
		else if ((mo->player) || (mo->flags & MF_SLIDEME)
			|| (mo->flags & MF_PUSHABLE))
		{ // try to slide along it
			P_SlideMove(mo);
			xmove = ymove = 0;
		}
		else if (mo->flags & MF_MISSILE)
		{
			// explode a missile
			if (P_CheckSkyHit(mo))
			{
				// Hack to prevent missiles exploding
				// against the sky.
				// Does not handle sky floors.
				// Check frontsector as well.

				P_RemoveMobj(mo);
				return;
			}

			// draw damage on wall
			//SPLAT TEST ----------------------------------------------------------
#ifdef WALLSPLATS
			if (blockingline && mo->type != MT_REDRING && mo->type != MT_FIREBALL
				&& !(mo->flags2 & MF2_AUTOMATIC) && !(mo->flags2 & MF2_RAILRING)
				&& !(mo->flags2 & MF2_BOUNCERING) && !(mo->flags2 & MF2_EXPLOSION)
				&& !(mo->flags2 & MF2_SCATTER))
				// set by last P_TryMove() that failed
			{
				divline_t divl;
				divline_t misl;
				fixed_t frac;

				P_MakeDivline(blockingline, &divl);
				misl.x = mo->x;
				misl.y = mo->y;
				misl.dx = mo->momx;
				misl.dy = mo->momy;
				frac = P_InterceptVector(&divl, &misl);
				R_AddWallSplat(blockingline, P_PointOnLineSide(mo->x,mo->y,blockingline),
					"A_DMG3", mo->z, frac, SPLATDRAWMODE_SHADE);
			}
#endif
			// --------------------------------------------------------- SPLAT TEST

			P_ExplodeMissile(mo);
			return;
		}
		else
			mo->momx = mo->momy = 0;
	}
	else if (mo->player)
		moved = true;

	if (P_MobjWasRemoved(mo)) // MF_SPECIAL touched a player! O_o;;
		return;

	// Check the gravity status.
	P_CheckGravity(mo, false);

	if (mo->player && !moved && (mo->player->pflags & PF_NIGHTSMODE) && mo->target)
	{
		angle_t fa;

		P_UnsetThingPosition(mo);
		mo->player->angle_pos = mo->player->old_angle_pos;
		mo->player->speed = FixedMul(mo->player->speed, 4*FRACUNIT/5);
		if (player->flyangle >= 0 && player->flyangle < 90)
			player->flyangle = 135;
		else if (player->flyangle >= 90 && player->flyangle < 180)
			player->flyangle = 45;
		else if (player->flyangle >= 180 && player->flyangle < 270)
			player->flyangle = 315;
		else
			player->flyangle = 225;
		player->flyangle %= 360;

		if (player->pflags & PF_TRANSFERTOCLOSEST)
		{
			mo->x -= mo->momx;
			mo->y -= mo->momy;
		}
		else
		{
			fa = player->old_angle_pos>>ANGLETOFINESHIFT;

			mo->x = mo->target->x + FixedMul(FINECOSINE(fa),mo->target->radius);
			mo->y = mo->target->y + FixedMul(FINESINE(fa),mo->target->radius);
		}

		mo->momx = mo->momy = 0;
		P_SetThingPosition(mo);
	}

	if ((mo->flags & MF_MISSILE) || (mo->flags2 & MF2_SKULLFLY) || mo->type == MT_SHELL || mo->type == MT_VULTURE)
		return; // no friction for missiles ever

	if (mo->player && mo->player->homing) // no friction for homing
		return;

	if (((!(mo->eflags & MFE_VERTICALFLIP) && mo->z > mo->floorz) || (mo->eflags & MFE_VERTICALFLIP && mo->z+mo->height < mo->ceilingz))
		&& !(mo->player && (mo->player->pflags & PF_SLIDING)))
		return; // no friction when airborne

	// spinning friction
	if (player)
	{
		if ((player->pflags & PF_SPINNING) && (player->rmomx || player->rmomy) && !(player->pflags & PF_STARTDASH))
		{
			const fixed_t ns = FixedDiv(549*FRICTION,500*FRACUNIT);
			mo->momx = FixedMul(mo->momx, ns);
			mo->momy = FixedMul(mo->momy, ns);
			return;
		}
	}

	P_XYFriction(mo, oldx, oldy);
}

static void P_RingXYMovement(mobj_t *mo)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (!P_SceneryTryMove(mo, mo->x + mo->momx, mo->y + mo->momy))
		P_SlideMove(mo);
}

static void P_SceneryXYMovement(mobj_t *mo)
{
	fixed_t oldx, oldy; // reducing bobbing/momentum on ice when up against walls

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	oldx = mo->x;
	oldy = mo->y;

	if (!P_SceneryTryMove(mo, mo->x + mo->momx, mo->y + mo->momy))
		P_SlideMove(mo);

	if (mo->z > mo->floorz)
		return; // no friction when airborne

	P_SceneryXYFriction(mo, oldx, oldy);
}

static void P_RingZMovement(mobj_t *mo)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		INT32 thingtop = mo->z + mo->height;

		for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if ((!(rover->flags & FF_BLOCKOTHERS || rover->flags & FF_QUICKSAND) || (rover->flags & FF_SWIMMABLE)))
				continue;

			if (rover->flags & FF_QUICKSAND)
			{
				if (mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					if (mo->floorz < mo->z)
						mo->floorz = mo->z;
				}
				continue;
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if (*rover->topheight > mo->floorz && abs(delta1) < abs(delta2)
				&& (!(rover->flags & FF_REVERSEPLATFORM)))
			{
				mo->floorz = *rover->topheight;
			}
			if (*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
			}
		}
	}

	// adjust height
	if (mo->pmomz && mo->z != mo->floorz)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	// clip movement
	if (mo->z <= mo->floorz && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		mo->z = mo->floorz;

		mo->momz = 0;
	}
	else if (mo->z + mo->height > mo->ceilingz && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		mo->momz = 0;

		mo->z = mo->ceilingz - mo->height;
	}
}

boolean P_CheckDeathPitCollide(mobj_t *mo)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (((mo->z <= mo->subsector->sector->floorheight
		&& !(mo->eflags & MFE_VERTICALFLIP) && (mo->subsector->sector->flags & SF_FLIPSPECIAL_FLOOR))
	|| (mo->z + mo->height >= mo->subsector->sector->ceilingheight
		&& (mo->eflags & MFE_VERTICALFLIP) && (mo->subsector->sector->flags & SF_FLIPSPECIAL_CEILING)))
	&& (GETSECSPECIAL(mo->subsector->sector->special, 1) == 6
	|| GETSECSPECIAL(mo->subsector->sector->special, 1) == 7))
		return true;

	return false;
}

boolean P_CheckSolidLava(mobj_t *mo, ffloor_t *rover)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if ((rover->flags & FF_SWIMMABLE) && GETSECSPECIAL(rover->master->frontsector->special, 1) == 3
		&& !(rover->master->flags & ML_BLOCKMONSTERS)
		&& ((rover->master->flags & ML_EFFECT3) || mo->z-mo->momz > *rover->topheight - FixedMul(16*FRACUNIT, mo->scale)))
			return true;

	return false;
}

//
// P_ZMovement
// Returns false if the mobj was killed/exploded/removed, true otherwise.
//
static boolean P_ZMovement(mobj_t *mo)
{
	fixed_t dist, delta;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2, thingtop = mo->z + mo->height;

		for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
#if 0 // I question the utility of having four seperate z movement functions.
			if (!(rover->flags & FF_EXISTS)
				|| (!((((rover->flags & FF_BLOCKPLAYER) && mo->player)
				|| ((rover->flags & FF_BLOCKOTHERS) && !mo->player)) || rover->flags & FF_QUICKSAND)
				|| (rover->flags & FF_SWIMMABLE)))
			{
				continue;
			}
#else
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (mo->player && P_CheckSolidLava(mo, rover)) // only the player should be affected
				;
			else if (!((((rover->flags & FF_BLOCKPLAYER) && mo->player)
				|| ((rover->flags & FF_BLOCKOTHERS) && !mo->player))
				|| rover->flags & FF_QUICKSAND))
				continue;
#endif
			if (rover->flags & FF_QUICKSAND)
			{
				if (mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					if (mo->floorz < mo->z)
						mo->floorz = mo->z;
				}
				continue;
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if (*rover->topheight > mo->floorz && abs(delta1) < abs(delta2)
				&& (!(rover->flags & FF_REVERSEPLATFORM)))
			{
				mo->floorz = *rover->topheight;
			}
			if (*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
			}
		}
	}

	// adjust height
	if (mo->pmomz && mo->z != mo->floorz)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	switch (mo->type)
	{
		case MT_THROWNBOUNCE:
			if ((mo->flags & MF_BOUNCE) && (mo->z <= mo->floorz || mo->z+mo->height >= mo->ceilingz))
			{
				mo->momz = -mo->momz;
				mo->z += mo->momz;
				S_StartSound(mo, mo->info->activesound);
				mo->threshold++;

				// Be sure to change the XY one too if you change this.
				// Gain lower amounts of time on each bounce.
				if (mo->threshold < 5)
					mo->fuse += ((5 - mo->threshold) * TICRATE);
			}
			break;

		case MT_SKIM:
			// skims don't bounce
			if (mo->z > mo->watertop && mo->z - mo->momz <= mo->watertop)
			{
				mo->z = mo->watertop;
				mo->momz = 0;
				mo->flags |= MF_NOGRAVITY;
			}
			break;
		case MT_GOOP:
			if (P_CheckDeathPitCollide(mo))
			{
				P_RemoveMobj(mo);
				return false;
			}
			if (mo->z <= mo->floorz && mo->momz)
			{
				P_SetMobjState(mo, mo->info->meleestate);
				mo->momx = mo->momy = mo->momz = 0;
				mo->z = mo->floorz;
				if (mo->info->painsound)
					S_StartSound(mo, mo->info->painsound);
			}
			break;
		case MT_FALLINGROCK:
		case MT_BIGTUMBLEWEED:
		case MT_LITTLETUMBLEWEED:
		case MT_SHELL:
			// Remove stuff from death pits.
			if (P_CheckDeathPitCollide(mo))
			{
				P_RemoveMobj(mo);
				return false;
			}
			break;
		case MT_REDFLAG:
		case MT_BLUEFLAG:
			// Remove from death pits.  DON'T FUCKING DESPAWN IT DAMMIT
			if (P_CheckDeathPitCollide(mo))
			{
				mo->fuse = 1;
				return false;
			}
			break;

		case MT_RING: // Ignore still rings
		case MT_COIN:
#ifdef BLUE_SPHERES
		case MT_BLUEBALL:
#endif
		case MT_REDTEAMRING:
		case MT_BLUETEAMRING:
		case MT_FLINGRING:
		case MT_FLINGCOIN:
			// Remove flinged stuff from death pits.
			if (P_CheckDeathPitCollide(mo))
			{
				P_RemoveMobj(mo);
				return false;
			}
			if (!(mo->momx || mo->momy || mo->momz))
				return true;
			break;
		case MT_BOUNCERING:
		case MT_INFINITYRING:
		case MT_AUTOMATICRING:
		case MT_RAILRING:
		case MT_EXPLOSIONRING:
		case MT_SCATTERRING:
		case MT_GRENADERING:
		case MT_BOUNCEPICKUP:
		case MT_RAILPICKUP:
		case MT_AUTOPICKUP:
		case MT_EXPLODEPICKUP:
		case MT_SCATTERPICKUP:
		case MT_GRENADEPICKUP:
			// Remove flinged stuff from death pits.
			if (P_CheckDeathPitCollide(mo) && (mo->flags2 & MF2_DONTRESPAWN))
			{
				P_RemoveMobj(mo);
				return false;
			}
			if (!(mo->momx || mo->momy || mo->momz))
				return true;
			break;
		case MT_FLINGEMERALD:
		case MT_NIGHTSWING:
			if (!(mo->momx || mo->momy || mo->momz))
				return true;
			break;
		case MT_FLAMEJET:
		case MT_VERTICALFLAMEJET:
			if (!(mo->flags & MF_BOUNCE))
				return true;
			break;
		case MT_SPIKE:
			// Dead spike particles disappear upon ground contact
			if ((mo->z <= mo->floorz || mo->z + mo->height >= mo->ceilingz) && mo->health <= 0)
			{
				P_RemoveMobj(mo);
				return false;
			}
			break;
		default:
			break;
	}

	if (P_CheckDeathPitCollide(mo))
	{
		if (mo->flags & MF_PUSHABLE)
		{
			// Remove other pushable items from death pits.
			P_RemoveMobj(mo);
			return false;
		}
		else if (mo->flags & MF_ENEMY || mo->flags & MF_BOSS)
		{
			// Kill enemies and bosses that fall into death pits.
			if (mo->health)
			{
				P_KillMobj(mo, NULL, NULL);
				return false;
			}
		}
	}

	if (P_MobjFlip(mo)*mo->momz < 0
	&& (mo->flags2 & MF2_CLASSICPUSH))
		mo->momx = mo->momy = 0;

	if (mo->flags & MF_FLOAT && mo->target && mo->health
	&& !(mo->type == MT_EGGMOBILE) && mo->target->health > 0)
	{
		// float down towards target if too close
		if (!(mo->flags2 & MF2_SKULLFLY) && !(mo->flags2 & MF2_INFLOAT))
		{
			dist = P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y);

			delta = (mo->target->z + (mo->height>>1)) - mo->z;

			if (delta < 0 && dist < -(delta*3))
				mo->z -= FixedMul(FLOATSPEED, mo->scale);
			else if (delta > 0 && dist < (delta*3))
				mo->z += FixedMul(FLOATSPEED, mo->scale);

			if (mo->type == MT_JETJAW && mo->z + mo->height > mo->watertop)
				mo->z = mo->watertop - mo->height;
		}

	}

	// clip movement
	if (mo->z <= mo->floorz && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		// hit the floor
		if (mo->type == MT_FIREBALL) // special case for the fireball
			mo->momz = FixedMul(5*FRACUNIT, mo->scale);
		else if (mo->flags & MF_MISSILE)
		{
			mo->z = mo->floorz;
			if (!(mo->flags & MF_NOCLIP))
			{
				// This is a really ugly hard-coded hack to prevent grenades
				// from exploding the instant they hit the ground, and then
				// another to prevent them from turning into hockey pucks.
				// I'm sorry in advance. -SH
				// PS: Oh, and Brak's napalm bombs too, now.
				if (mo->flags & MF_GRENADEBOUNCE)
				{
					// Going down? (Or up in reverse gravity?)
					if (mo->momz*P_MobjFlip(mo) < 0)
					{
						// If going slower than a fracunit, just stop.
						if (abs(mo->momz) < FixedMul(FRACUNIT, mo->scale))
						{
							mo->momx = mo->momy = mo->momz = 0;

							// Napalm hack
							if (mo->type == MT_CYBRAKDEMON_NAPALM_BOMB_LARGE && mo->fuse)
								mo->fuse = 1;
						}
						// Otherwise bounce up at half speed.
						else
							mo->momz = -FixedMul(mo->momz, FRACUNIT/2);
						S_StartSound(mo, mo->info->activesound);
					}
				}
				// Hack over. Back to your regularly scheduled detonation. -SH
				else
				{
					// Don't explode on the sky!
					if (mo->subsector->sector->floorpic == skyflatnum
					&& mo->subsector->sector->floorheight == mo->floorz)
						P_RemoveMobj(mo);
					else
						P_ExplodeMissile(mo);
					return false;
				}
			}
		}

		if (mo->flags2 & MF2_SKULLFLY) // the skull slammed into something
			mo->momz = -mo->momz;

		if (mo->momz < 0) // falling
		{
			// set it once and not continuously
			if (tmfloorthing)
			{
				// Bouncing boxes
				if (tmfloorthing->z > tmfloorthing->floorz)
				{
					if ((tmfloorthing->flags & MF_MONITOR) || (tmfloorthing->flags & MF_PUSHABLE))
						mo->momz = FixedMul(4*FRACUNIT, mo->scale);
				}
			}
			if (!tmfloorthing || tmfloorthing->flags & MF_PUSHABLE
				|| tmfloorthing->flags2 & MF2_STANDONME || tmfloorthing->type == MT_PLAYER)
			{
				if (!tmfloorthing || mo->momz)
					mo->eflags |= MFE_JUSTHITFLOOR;
			}

			// Flingrings bounce
			if (mo->type == MT_FLINGRING
				|| mo->type == MT_FLINGCOIN
				|| P_WeaponOrPanel(mo->type)
				|| mo->type == MT_FLINGEMERALD
				|| mo->type == MT_BIGTUMBLEWEED
				|| mo->type == MT_LITTLETUMBLEWEED
				|| mo->type == MT_CANNONBALLDECOR
				|| mo->type == MT_FALLINGROCK
				|| ((mo->eflags & MFE_VERTICALFLIP) && mo->type == MT_REDFLAG)
				|| ((mo->eflags & MFE_VERTICALFLIP) && mo->type == MT_BLUEFLAG))
			{
				if (maptol & TOL_NIGHTS)
					mo->momz = -FixedDiv(mo->momz, 10*FRACUNIT);
				else
					mo->momz = -FixedMul(mo->momz, FixedDiv(17*FRACUNIT,20*FRACUNIT));

				if (mo->type == MT_BIGTUMBLEWEED || mo->type == MT_LITTLETUMBLEWEED)
				{
					if (abs(mo->momx) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mo->momy) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mo->momz) < FixedMul(STOPSPEED*3, mo->scale))
					{
						if (!(mo->flags & MF_AMBUSH))
						{
							mo->momx = mo->momy = mo->momz = 0;
							P_SetMobjState(mo, mo->info->spawnstate);
						}
						else
						{
							// If deafed, give the tumbleweed another random kick if it runs out of steam.
							mo->momz += FixedMul(6*FRACUNIT, mo->scale);

							if (P_Random() & 1)
								mo->momx += FixedMul(6*FRACUNIT, mo->scale);
							else
								mo->momx -= FixedMul(6*FRACUNIT, mo->scale);

							if (P_Random() & 1)
								mo->momy += FixedMul(6*FRACUNIT, mo->scale);
							else
								mo->momy -= FixedMul(6*FRACUNIT, mo->scale);
						}
					}

					// Stolen from P_SpawnFriction
					mo->friction = FRACUNIT - 0x100;
					mo->movefactor = ((0x10092 - mo->friction)*(0x70))/0x158;
				}
				else if (mo->type == MT_FALLINGROCK)
				{
					if (mo->momz > FixedMul(2*FRACUNIT, mo->scale))
						S_StartSound(mo, mo->info->activesound + P_RandomKey(mo->info->mass));

					mo->momz /= 2; // Rocks not so bouncy

					if (abs(mo->momx) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mo->momy) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mo->momz) < FixedMul(STOPSPEED*3, mo->scale))
					{
						P_RemoveMobj(mo);
						return false;
					}
				}
				else if (mo->type == MT_CANNONBALLDECOR)
				{
					mo->momz /= 2;
					if (abs(mo->momz) < FixedMul(STOPSPEED*3, mo->scale))
						mo->momz = 0;
				}
			}
			else if (tmfloorthing && (tmfloorthing->flags & MF_PUSHABLE
			|| tmfloorthing->flags2 & MF2_STANDONME || tmfloorthing->type == MT_PLAYER))
				mo->momz = tmfloorthing->momz;
			else if (!tmfloorthing)
				mo->momz = 0;
		}

		mo->z = mo->floorz;

		if (mo->type == MT_STEAM)
			return true;
	}
	else if (!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MFE_JUSTHITFLOOR;

		P_CheckGravity(mo, true);
	}

	if (mo->z + mo->height > mo->ceilingz && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->momz > 0) // hit the ceiling
		{
			// Flingrings bounce
			if ((mo->eflags & MFE_VERTICALFLIP) && (mo->type == MT_FLINGRING
				|| mo->type == MT_FLINGCOIN
				|| P_WeaponOrPanel(mo->type)
				|| mo->type == MT_FLINGEMERALD))
			{
				if (maptol & TOL_NIGHTS)
					mo->momz = -FixedDiv(mo->momz, 10*FRACUNIT);
				else
					mo->momz = -FixedMul(mo->momz, FixedDiv(17*FRACUNIT,20*FRACUNIT));
			}
			else
				mo->momz = 0;
		}

		mo->z = mo->ceilingz - mo->height;

		if (mo->flags2 & MF2_SKULLFLY) // the skull slammed into something
			mo->momz = -mo->momz;


		if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
		{
			// Hack 2: Electric Boogaloo -SH
			if (mo->type == MT_THROWNGRENADE || mo->type == MT_CYBRAKDEMON_NAPALM_BOMB_LARGE)
			{
				if (mo->momz >= 0)
				{
					mo->momz = -mo->momz;
					S_StartSound(mo, mo->info->activesound);
				}
			}
			else
			{
				// Don't explode on the sky!
				if (mo->subsector->sector->ceilingpic == skyflatnum
				&& mo->subsector->sector->ceilingheight == mo->ceilingz)
					P_RemoveMobj(mo);
				else
					P_ExplodeMissile(mo);
				return false;
			}
		}
	}
	return true;
}

static void P_PlayerZMovement(mobj_t *mo)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (!mo->player)
		return;

	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		INT32 thingtop = mo->z + mo->height;

		for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (P_CheckSolidLava(mo, rover) || P_CanRunOnWater(mo->player, rover))
				;
			else if (!(rover->flags & FF_BLOCKPLAYER || rover->flags & FF_QUICKSAND))
				continue;

			if (rover->flags & FF_QUICKSAND)
			{
				if (mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					if (mo->floorz < mo->z)
						mo->floorz = mo->z;
				}
				continue; // This is so you can jump/spring up through quicksand from below.
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if (*rover->topheight > mo->floorz && abs(delta1) < abs(delta2)
				&& (!(rover->flags & FF_REVERSEPLATFORM)))
			{
				mo->floorz = *rover->topheight;
			}
			if (*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
			}
		}
	}

	// check for smooth step up
	if ((mo->eflags & MFE_VERTICALFLIP && mo->z + mo->height > mo->ceilingz)
		|| (!(mo->eflags & MFE_VERTICALFLIP) && mo->z < mo->floorz))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->player->viewheight -= (mo->z+mo->height) - mo->ceilingz;
		else
			mo->player->viewheight -= mo->floorz - mo->z;

		mo->player->deltaviewheight =
			(FixedMul(cv_viewheight.value<<FRACBITS, mo->scale) - mo->player->viewheight)>>3;
	}

	// adjust height
/*	if (mo->pmomz && mo->z > mo->floorz && !(mo->player->pflags & PF_JUMPED))
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}*/

	mo->z += mo->momz;

	// Have player fall through floor?
	if (mo->player->playerstate == PST_DEAD
	|| mo->player->playerstate == PST_REBORN)
		return;

	// clip movement
	if (P_IsObjectOnGround(mo))
	{
		if (mo->player && (mo->player->pflags & PF_NIGHTSMODE))
		{
			if (mo->player->flyangle < 90 || mo->player->flyangle >= 270)
				mo->player->flyangle += 90;
			else
				mo->player->flyangle -= 90;
			mo->z = mo->floorz;
			mo->player->speed = FixedMul(mo->player->speed, 4*FRACUNIT/5);
			goto nightsdone;
		}
		// Get up if you fell.
		if (mo->state == &states[mo->info->painstate] || mo->state == &states[S_PLAY_SUPERHIT])
			P_SetPlayerMobjState(mo, S_PLAY_STND);

		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->ceilingz - mo->height;
		else
			mo->z = mo->floorz;

		if (P_MobjFlip(mo)*mo->momz < 0) // falling
		{
			// Squat down. Decrease viewheight for a moment after hitting the ground (hard),
			if (mo->player)
			{
				if (P_MobjFlip(mo)*mo->momz < -FixedMul(8*FRACUNIT, mo->scale))
					mo->player->deltaviewheight = (P_MobjFlip(mo)*mo->momz)>>3; // make sure momz is negative
			}

			// set it once and not continuously
			if (tmfloorthing)
			{
				if ((tmfloorthing->flags & MF_MONITOR) || (tmfloorthing->flags & MF_PUSHABLE)
					|| (tmfloorthing->flags2 & MF2_STANDONME))
				{
					if (mo->player)
					{
						if (!(mo->player->pflags & PF_JUMPED))
							tmfloorthing = 0;
					}
				}
			}

			if (P_IsObjectOnGround(mo) && (!tmfloorthing || tmfloorthing->flags & MF_PUSHABLE
				|| tmfloorthing->flags2 & MF2_STANDONME || tmfloorthing->type == MT_PLAYER)) // Spin Attack
			{
				if ((tmfloorthing && mo->momz) || !tmfloorthing)
					mo->eflags |= MFE_JUSTHITFLOOR; // Spin Attack

				if (mo->eflags & MFE_JUSTHITFLOOR)
				{
#ifdef POLYOBJECTS
					// Check if we're on a polyobject
					// that triggers a linedef executor.
					msecnode_t *node;

					for (node = mo->touching_sectorlist; node; node = node->m_snext)
					{
						sector_t *sec = node->m_sector;
						subsector_t *newsubsec;
						size_t i;

						for (i = 0; i < numsubsectors; i++)
						{
							newsubsec = &subsectors[i];

							if (newsubsec->sector != sec)
								continue;

							if (newsubsec->polyList)
							{
								polyobj_t *po = newsubsec->polyList;
								sector_t *polysec;

								while(po)
								{
									if (!(po->flags & POF_LDEXEC)
										|| !(po->flags & POF_SOLID))
									{
										po = (polyobj_t *)(po->link.next);
										continue;
									}

									if (!P_MobjInsidePolyobj(po, mo))
									{
										po = (polyobj_t *)(po->link.next);
										continue;
									}

									// We're inside it! Yess...
									polysec = po->lines[0]->backsector;

									if (mo->z == polysec->ceilingheight)
									{
										// We're landing on a PO, so check for
										// a linedef executor.
										// Trigger tags are 32000 + the PO's ID number.
										P_LinedefExecute((INT16)(32000 + po->id), mo, NULL);
									}

									po = (polyobj_t *)(po->link.next);
								}
							}
						}
					}
#endif

					// Cut momentum in half when you hit the ground and
					// aren't pressing any controls.
					if (!(mo->player->cmd.forwardmove || mo->player->cmd.sidemove) && !mo->player->cmomx && !mo->player->cmomy && !(mo->player->pflags & PF_SPINNING))
					{
						mo->momx = mo->momx/2;
						mo->momy = mo->momy/2;
					}
				}

				if (mo->health)
				{
					if (mo->player->pflags & PF_GLIDING) // ground gliding
					{
						mo->player->skidtime = TICRATE;
						mo->tics = -1;
					}
					else if (mo->player->pflags & PF_JUMPED || (mo->player->pflags & (PF_SPINNING|PF_USEDOWN)) != (PF_SPINNING|PF_USEDOWN))
					{
						if (mo->player->cmomx || mo->player->cmomy)
						{
							if (mo->player->speed >= FixedMul(mo->player->runspeed, mo->scale) && mo->player->panim != PA_RUN)
								P_SetPlayerMobjState(mo, S_PLAY_SPD1);
							else if ((mo->player->rmomx || mo->player->rmomy) && mo->player->panim != PA_WALK)
								P_SetPlayerMobjState(mo, S_PLAY_RUN1);
							else if (!mo->player->rmomx && !mo->player->rmomy && mo->player->panim != PA_IDLE)
								P_SetPlayerMobjState(mo, S_PLAY_STND);
						}
						else
						{
							if (mo->player->speed >= FixedMul(mo->player->runspeed, mo->scale) && mo->player->panim != PA_RUN)
								P_SetPlayerMobjState(mo, S_PLAY_SPD1);
							else if ((mo->momx || mo->momy) && mo->player->panim != PA_WALK)
								P_SetPlayerMobjState(mo, S_PLAY_RUN1);
							else if (!mo->momx && !mo->momy && mo->player->panim != PA_IDLE)
								P_SetPlayerMobjState(mo, S_PLAY_STND);
						}
					}

					if (mo->player->pflags & PF_JUMPED)
						mo->player->pflags &= ~PF_SPINNING;
					else if (!(mo->player->pflags & PF_USEDOWN))
						mo->player->pflags &= ~PF_SPINNING;

					if (!(mo->player->pflags & PF_GLIDING))
						mo->player->pflags &= ~PF_JUMPED;
					mo->player->pflags &= ~PF_THOKKED;
					//mo->player->pflags &= ~PF_GLIDING;
					mo->player->jumping = 0;
					mo->player->secondjump = 0;
					mo->player->glidetime = 0;
					mo->player->climbing = 0;
				}
			}
			if (mo->player && !(mo->player->pflags & PF_SPINNING))
				mo->player->pflags &= ~PF_STARTDASH;

			if (!tmfloorthing || tmfloorthing->flags & MF_PUSHABLE
				|| tmfloorthing->flags2 & MF2_STANDONME || tmfloorthing->type == MT_PLAYER)
				mo->momz = 0;
		}
	}
	else if (!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		boolean wasflip;
		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MFE_JUSTHITFLOOR;

		wasflip = (mo->eflags & MFE_VERTICALFLIP) != 0;
		P_CheckGravity(mo, true);

		if ((wasflip && !(mo->eflags & MFE_VERTICALFLIP))
		|| (!wasflip && (mo->eflags & MFE_VERTICALFLIP)))
		{
			G_GhostAddFlip();
			// Flip aiming to match!
			if (mo->player->pflags & PF_FLIPCAM)
			{
				mo->player->aiming = InvAngle(mo->player->aiming);
				if (mo->player-players == displayplayer)
				{
					localaiming = mo->player->aiming;
					if (camera.chase) {
						camera.aiming = InvAngle(camera.aiming);
						camera.z = mo->z - camera.z + mo->z;
						if (mo->eflags & MFE_VERTICALFLIP)
							camera.z += FixedMul(20*FRACUNIT, mo->scale);
					}
				}
				else if (mo->player-players == secondarydisplayplayer)
				{
					localaiming2 = mo->player->aiming;
					if (camera2.chase) {
						camera2.aiming = InvAngle(camera2.aiming);
						camera2.z = mo->z - camera2.z + mo->z;
						if (mo->eflags & MFE_VERTICALFLIP)
							camera2.z += FixedMul(20*FRACUNIT, mo->scale);
					}
				}
			}
		}
	}

nightsdone:

	if ((mo->eflags & MFE_VERTICALFLIP && mo->z < mo->floorz) || (!(mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height > mo->ceilingz))
	{
		if (mo->player && (mo->player->pflags & PF_NIGHTSMODE))
		{
			if (mo->player->flyangle < 90 || mo->player->flyangle >= 270)
				mo->player->flyangle -= 90;
			else
				mo->player->flyangle += 90;
			mo->player->flyangle %= 360;
			mo->z = mo->ceilingz - mo->height;
			mo->player->speed = FixedMul(mo->player->speed, 4*FRACUNIT/5);
		}

		// Check for "Mario" blocks to hit and bounce them
		if (P_MobjFlip(mo)*mo->momz > 0)
		{
			msecnode_t *node;

			if (CheckForMarioBlocks && mo->player && !(netgame && mo->player->spectator)) // Only let the player punch
			{
				// Search the touching sectors, from side-to-side...
				for (node = mo->touching_sectorlist; node; node = node->m_snext)
				{
					ffloor_t *rover;
					if (!node->m_sector->ffloors)
						continue;

					for (rover = node->m_sector->ffloors; rover; rover = rover->next)
					{
						if (!(rover->flags & FF_EXISTS))
							continue;

						// Come on, it's time to go...
						if (rover->flags & FF_MARIO
						&& !(mo->eflags & MFE_VERTICALFLIP) // if you were flipped, your head isn't actually hitting your ceilingz is it?
						&& *rover->bottomheight == mo->ceilingz) // The player's head hit the bottom!
							// DO THE MARIO!
							EV_MarioBlock(rover->master->frontsector, node->m_sector, *rover->topheight, mo);
					}
				} // Ugly ugly billions of braces! Argh!
			}

			// hit the ceiling
			if (mariomode)
				S_StartSound(mo, sfx_mario1);

			if (!(mo->player && mo->player->climbing))
				mo->momz = 0;
		}

		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->floorz;
		else
			mo->z = mo->ceilingz - mo->height;
	}
}

static boolean P_SceneryZMovement(mobj_t *mo)
{
	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t delta1, delta2;
		INT32 thingtop = mo->z + mo->height;

		for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if ((!(rover->flags & FF_BLOCKOTHERS || rover->flags & FF_QUICKSAND) || (rover->flags & FF_SWIMMABLE)))
				continue;

			if (rover->flags & FF_QUICKSAND)
			{
				if (mo->z < *rover->topheight && *rover->bottomheight < thingtop)
				{
					mo->floorz = mo->z;
					continue;
				}
			}

			delta1 = mo->z - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			delta2 = thingtop - (*rover->bottomheight + ((*rover->topheight - *rover->bottomheight)/2));
			if (*rover->topheight > mo->floorz && abs(delta1) < abs(delta2)
				&& (!(rover->flags & FF_REVERSEPLATFORM)))
			{
				mo->floorz = *rover->topheight;
			}
			if (*rover->bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
				&& (/*mo->z + mo->height <= *rover->bottomheight ||*/ !(rover->flags & FF_PLATFORM)))
			{
				mo->ceilingz = *rover->bottomheight;
			}
		}
	}

	// adjust height
	if (mo->pmomz && mo->z != mo->floorz)
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
	}
	mo->z += mo->momz;

	switch (mo->type)
	{
		case MT_SMALLBUBBLE:
			if (mo->z <= mo->floorz || mo->z+mo->height >= mo->ceilingz) // Hit the floor, so POP!
			{
				// don't sounds stop when you kill the mobj..?
				// yes, they do, making this entirely redundant
				P_RemoveMobj(mo);
				return false;
			}
			break;
		case MT_MEDIUMBUBBLE:
			if (P_CheckDeathPitCollide(mo)) // Don't split if you fell in a pit
			{
				P_RemoveMobj(mo);
				return false;
			}
			if ((!(mo->eflags & MFE_VERTICALFLIP) && mo->z <= mo->floorz)
			|| (mo->eflags & MFE_VERTICALFLIP && mo->z+mo->height >= mo->ceilingz)) // Hit the floor, so split!
			{
				// split
				mobj_t *explodemo;
				UINT8 prandom, i;

				for (i = 0; i < 4; ++i) // split into four
				{
					prandom = P_Random();
					explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_SMALLBUBBLE);
					explodemo->momx += ((prandom & 0x0F) << (FRACBITS-2)) * (i & 2 ? -1 : 1);
					explodemo->momy += ((prandom & 0xF0) << (FRACBITS-6)) * (i & 1 ? -1 : 1);
					explodemo->destscale = mo->scale;
					P_SetScale(explodemo, mo->scale);
				}

				if (mo->threshold != 42) // Don't make pop sound if threshold is 42.
					S_StartSound(explodemo, sfx_bubbl1 + P_RandomKey(5));
				//note that we assign the bubble sound to one of the new bubbles.
				// in other words, IT ACTUALLY GETS USED YAAAAAAAY

				P_RemoveMobj(mo);
				return false;
			}
			else if (mo->z <= mo->floorz || mo->z+mo->height >= mo->ceilingz) // Hit the ceiling instead? Just disappear anyway
			{
				P_RemoveMobj(mo);
				return false;
			}
			break;
		case MT_SEED: // now scenery
			if (P_CheckDeathPitCollide(mo)) // No flowers for death pits
			{
				P_RemoveMobj(mo);
				return false;
			}
			// Soniccd seed turns into a flower!
			if ((!(mo->eflags & MFE_VERTICALFLIP) && mo->z <= mo->floorz)
			|| (mo->eflags & MFE_VERTICALFLIP && mo->z+mo->height >= mo->ceilingz))
			{
				// DO NOT use random numbers here.
				// SonicCD mode is console togglable and
				// affects demos.
				UINT8 rltime = (leveltime & 4);

				if (!rltime)
					P_SpawnMobj(mo->x, mo->y, mo->floorz, MT_GFZFLOWER3);
				else if (rltime == 2)
					P_SpawnMobj(mo->x, mo->y, mo->floorz, MT_GFZFLOWER2);
				else
					P_SpawnMobj(mo->x, mo->y, mo->floorz, MT_GFZFLOWER1);

				P_RemoveMobj(mo);
				return false;
			}

		default:
			break;
	}

	// Fix for any silly pushables like the egg statues that are also scenery for some reason -- Monster Iestyn
	if (P_CheckDeathPitCollide(mo))
	{
		if (mo->flags & MF_PUSHABLE)
		{
			P_RemoveMobj(mo);
			return false;
		}
	}

	// clip movement
	if (mo->z <= mo->floorz && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		mo->z = mo->floorz;

		if (mo->momz < 0) // falling
		{
			if (!tmfloorthing || tmfloorthing->flags & MF_PUSHABLE
				|| tmfloorthing->flags2 & MF2_STANDONME || tmfloorthing->type == MT_PLAYER)
			{
				if (!tmfloorthing || mo->momz)
					mo->eflags |= MFE_JUSTHITFLOOR; // Spin Attack
			}

			if (!tmfloorthing)
				mo->momz = 0;
		}
	}
	else if (!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MFE_JUSTHITFLOOR;

		P_CheckGravity(mo, true);
	}

	if (mo->z + mo->height > mo->ceilingz && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->momz > 0)
		{
			// hit the ceiling
			mo->momz = 0;
		}

		mo->z = mo->ceilingz - mo->height;
	}

	return true;
}

// P_CanRunOnWater
//
// Returns true if player can waterrun on the 3D floor
//
boolean P_CanRunOnWater(player_t *player, ffloor_t *rover)
{
	if (!(player->pflags & PF_NIGHTSMODE) && !player->homing
		&& (((player->charability == CA_SWIM) || player->powers[pw_super]) && player->mo->ceilingz-*rover->topheight >= player->mo->height)
		&& (rover->flags & FF_SWIMMABLE) && !(player->pflags & PF_SPINNING) && player->speed > FixedMul(player->runspeed, player->mo->scale)
		&& !(player->pflags & PF_SLIDING)
		&& abs(player->mo->z - *rover->topheight) < FixedMul(30*FRACUNIT, player->mo->scale))
		return true;

	return false;
}

//
// P_MobjCheckWater
//
// Check for water, set stuff in mobj_t struct for movement code later.
// This is called either by P_MobjThinker() or P_PlayerThink()
void P_MobjCheckWater(mobj_t *mobj)
{
	boolean waterwasnotset = (mobj->watertop == INT32_MAX);
	boolean wasinwater = (mobj->eflags & MFE_UNDERWATER) == MFE_UNDERWATER;
	boolean wasingoo = (mobj->eflags & MFE_GOOWATER) == MFE_GOOWATER;
	fixed_t thingtop = mobj->z + mobj->height; // especially for players, infotable height does not neccessarily match actual height
	sector_t *sector = mobj->subsector->sector;
	ffloor_t *rover;
	player_t *p = mobj->player; // Will just be null if not a player.

	// Default if no water exists.
	mobj->watertop = mobj->waterbottom = mobj->subsector->sector->floorheight - 1000*FRACUNIT;

	// Reset water state.
	mobj->eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER|MFE_GOOWATER);

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE)
		 || (((rover->flags & FF_BLOCKPLAYER) && mobj->player)
		 || ((rover->flags & FF_BLOCKOTHERS) && !mobj->player)))
			continue;

		if (mobj->eflags & MFE_VERTICALFLIP)
		{
			if (*rover->topheight < (thingtop - FixedMul(mobj->info->height/2, mobj->scale))
			 || *rover->bottomheight > thingtop)
				continue;
		}
		else
		{
			if (*rover->topheight < mobj->z
			 || *rover->bottomheight > (mobj->z + FixedMul(mobj->info->height/2, mobj->scale)))
				continue;
		}

		// Set the watertop and waterbottom
		mobj->watertop = *rover->topheight;
		mobj->waterbottom = *rover->bottomheight;

		// Just touching the water?
		if (((mobj->eflags & MFE_VERTICALFLIP) && thingtop - FixedMul(mobj->info->height, mobj->scale) < *rover->bottomheight)
		 || (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + FixedMul(mobj->info->height, mobj->scale) > *rover->topheight))
		{
			mobj->eflags |= MFE_TOUCHWATER;
			if (rover->flags & FF_GOOWATER && !(mobj->flags & MF_NOGRAVITY))
				mobj->eflags |= MFE_GOOWATER;
		}
		// Actually in the water?
		if (((mobj->eflags & MFE_VERTICALFLIP) && thingtop - FixedMul(mobj->info->height/2, mobj->scale) > *rover->bottomheight)
		 || (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + FixedMul(mobj->info->height/2, mobj->scale) < *rover->topheight))
		{
			mobj->eflags |= MFE_UNDERWATER;
			if (rover->flags & FF_GOOWATER && !(mobj->flags & MF_NOGRAVITY))
				mobj->eflags |= MFE_GOOWATER;
		}
	}

	// Specific things for underwater players
	if (p && (mobj->eflags & MFE_UNDERWATER) == MFE_UNDERWATER)
	{
		if (!((p->powers[pw_super]) || (p->powers[pw_invulnerability])))
		{
			if ((p->powers[pw_shield] & SH_NOSTACK) == SH_ATTRACT)
			{ // Water removes attract shield.
				p->powers[pw_shield] = p->powers[pw_shield] & SH_STACK;
				P_FlashPal(p, PAL_WHITE, 1);
			}
		}

		// Drown timer setting
		if ((p->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL // Has elemental
		 || (p->exiting) // Or exiting
		 || (maptol & TOL_NIGHTS) // Or in NiGHTS mode
		 || (mariomode)) // Or in Mario mode...
		{
			// Can't drown.
			p->powers[pw_underwater] = 0;
		}
		else if (p->powers[pw_underwater] <= 0) // No underwater timer set
		{
			// Then we'll set it!
			p->powers[pw_underwater] = underwatertics + 1;
		}
	}

	// The rest of this code only executes on a water state change.
	if (waterwasnotset || !!(mobj->eflags & MFE_UNDERWATER) == wasinwater)
		return;

	// Spectators and dead players also don't count.
	if (p && (p->spectator || p->playerstate != PST_LIVE))
		return;

	if ((p) // Players
	 || (mobj->flags & MF_PUSHABLE) // Pushables
	 || ((mobj->info->flags & MF_PUSHABLE) && mobj->fuse) // Previously pushable, might be moving still
	)
	{
		// Check to make sure you didn't just cross into a sector to jump out of
		// that has shallower water than the block you were originally in.
		if (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->watertop-mobj->floorz <= FixedMul(mobj->info->height, mobj->scale)>>1)
			return;

		if ((mobj->eflags & MFE_VERTICALFLIP) && mobj->ceilingz-mobj->waterbottom <= FixedMul(mobj->info->height, mobj->scale)>>1)
			return;

		if ((mobj->eflags & MFE_GOOWATER || wasingoo)) { // Decide what happens to your momentum when you enter/leave goopy water.
			if (wasinwater && P_MobjFlip(mobj)*mobj->momz > 0)
				mobj->momz = FixedMul(mobj->momz, FixedDiv(9*FRACUNIT, 8*FRACUNIT)); // Give the mobj a little out-of-goo boost.
			else if (P_MobjFlip(mobj)*mobj->momz < 0)
				mobj->momz = FixedMul(mobj->momz, FixedDiv(2*FRACUNIT, 5*FRACUNIT)); // KILL its momentum.
		}
		else if (wasinwater && P_MobjFlip(mobj)*mobj->momz > 0)
			mobj->momz = FixedMul(mobj->momz, FixedDiv(780*FRACUNIT, 457*FRACUNIT)); // Give the mobj a little out-of-water boost.

		if (P_MobjFlip(mobj)*mobj->momz < 0)
		{
			if ((mobj->eflags & MFE_VERTICALFLIP && thingtop-(FixedMul(mobj->info->height, mobj->scale)>>1)-mobj->momz <= mobj->waterbottom)
				|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z+(FixedMul(mobj->info->height, mobj->scale)>>1)-mobj->momz >= mobj->watertop))
			{
				// Spawn a splash
				mobj_t *splish;
				if (mobj->eflags & MFE_VERTICALFLIP)
				{
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->waterbottom-FixedMul(mobjinfo[MT_SPLISH].height, mobj->scale), MT_SPLISH);
					splish->flags2 |= MF2_OBJECTFLIP;
					splish->eflags |= MFE_VERTICALFLIP;
				}
				else
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->watertop, MT_SPLISH);
				splish->destscale = mobj->scale;
				P_SetScale(splish, mobj->scale);
			}

			// skipping stone!
			if (p && (p->charability2 == CA2_SPINDASH) && p->speed/2 > abs(mobj->momz)
				&& ((p->pflags & (PF_SPINNING|PF_JUMPED)) == PF_SPINNING)
				&& ((!(mobj->eflags & MFE_VERTICALFLIP) && thingtop - mobj->momz > mobj->watertop)
				|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->z - mobj->momz < mobj->waterbottom)))
			{
				mobj->momz = -mobj->momz/2;

				if (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->momz > FixedMul(6*FRACUNIT, mobj->scale))
					mobj->momz = FixedMul(6*FRACUNIT, mobj->scale);
				else if (mobj->eflags & MFE_VERTICALFLIP && mobj->momz < FixedMul(-6*FRACUNIT, mobj->scale))
					mobj->momz = FixedMul(-6*FRACUNIT, mobj->scale);
			}

		}
		else if (P_MobjFlip(mobj)*mobj->momz > 0)
		{
			if (((mobj->eflags & MFE_VERTICALFLIP && thingtop-(FixedMul(mobj->info->height, mobj->scale)>>1)-mobj->momz > mobj->waterbottom)
				|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z+(FixedMul(mobj->info->height, mobj->scale)>>1)-mobj->momz < mobj->watertop))
				&& !(mobj->eflags & MFE_UNDERWATER)) // underwater check to prevent splashes on opposite side
			{
				// Spawn a splash
				mobj_t *splish;
				if (mobj->eflags & MFE_VERTICALFLIP)
				{
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->waterbottom-FixedMul(mobjinfo[MT_SPLISH].height, mobj->scale), MT_SPLISH);
					splish->flags2 |= MF2_OBJECTFLIP;
					splish->eflags |= MFE_VERTICALFLIP;
				}
				else
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->watertop, MT_SPLISH);
				splish->destscale = mobj->scale;
				P_SetScale(splish, mobj->scale);
			}
		}

		// Time to spawn the bubbles!
		{
			INT32 i;
			INT32 bubblecount;
			UINT8 prandom[4];
			mobj_t *bubble;
			mobjtype_t bubbletype;

			if (mobj->eflags & MFE_GOOWATER || wasingoo)
				S_StartSound(mobj, sfx_ghit);
			else
				S_StartSound(mobj, sfx_splish); // And make a sound!

			bubblecount = FixedDiv(abs(mobj->momz), mobj->scale)>>(FRACBITS-1);
			// Max bubble count
			if (bubblecount > 128)
				bubblecount = 128;

			// Create tons of bubbles
			for (i = 0; i < bubblecount; i++)
			{
				// P_Random()s are called individually to allow consistency
				// across various compilers, since the order of function calls
				// in C is not part of the ANSI specification.
				prandom[0] = P_Random();
				prandom[1] = P_Random();
				prandom[2] = P_Random();
				prandom[3] = P_Random();

				bubbletype = MT_SMALLBUBBLE;
				if (!(prandom[0] & 0x3)) // medium bubble chance up to 64 from 32
					bubbletype = MT_MEDIUMBUBBLE;

				bubble = P_SpawnMobj(
					mobj->x + FixedMul((prandom[1]<<(FRACBITS-3)) * (prandom[0]&0x80 ? 1 : -1), mobj->scale),
					mobj->y + FixedMul((prandom[2]<<(FRACBITS-3)) * (prandom[0]&0x40 ? 1 : -1), mobj->scale),
					mobj->z + FixedMul((prandom[3]<<(FRACBITS-2)), mobj->scale), bubbletype);

				if (bubble)
				{
					if (P_MobjFlip(mobj)*mobj->momz < 0)
						bubble->momz = mobj->momz >> 4;
					else
						bubble->momz = 0;

					bubble->destscale = mobj->scale;
					P_SetScale(bubble, mobj->scale);
				}
			}
		}
	}
}

static void P_SceneryCheckWater(mobj_t *mobj)
{
	sector_t *sector;

	// Default if no water exists.
	mobj->watertop = mobj->waterbottom = mobj->subsector->sector->floorheight - 1000*FRACUNIT;

	// see if we are in water, and set some flags for later
	sector = mobj->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		mobj->eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER);

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKOTHERS)
				continue;
			if (*rover->topheight <= mobj->z
				|| *rover->bottomheight > (mobj->z + FixedMul(mobj->info->height >> 1, mobj->scale)))
				continue;

			if (mobj->z + FixedMul(mobj->info->height, mobj->scale) > *rover->topheight)
				mobj->eflags |= MFE_TOUCHWATER;
			else
				mobj->eflags &= ~MFE_TOUCHWATER;

			// Set the watertop and waterbottom
			mobj->watertop = *rover->topheight;
			mobj->waterbottom = *rover->bottomheight;

			if (mobj->z + FixedMul(mobj->info->height >> 1, mobj->scale) < *rover->topheight)
				mobj->eflags |= MFE_UNDERWATER;
			else
				mobj->eflags &= ~MFE_UNDERWATER;
		}
	}
	else
		mobj->eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER);
}

static boolean P_CameraCheckHeat(camera_t *thiscam)
{
	sector_t *sector;

	// see if we are in water
	sector = thiscam->subsector->sector;

	if (P_FindSpecialLineFromTag(13, sector->tag, -1) != -1)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKOTHERS)
				continue;
			if (*rover->topheight <= thiscam->z
				|| *rover->bottomheight > (thiscam->z + (thiscam->height >> 1)))
				continue;

			if (thiscam->z + (thiscam->height >> 1) < *rover->topheight)
			{
				if (P_FindSpecialLineFromTag(13, rover->master->frontsector->tag, -1) != -1)
					return true;
			}
		}
	}

	return false;
}

static boolean P_CameraCheckWater(camera_t *thiscam)
{
	sector_t *sector;

	// see if we are in water
	sector = thiscam->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKOTHERS)
				continue;
			if (*rover->topheight <= thiscam->z
				|| *rover->bottomheight > (thiscam->z + (thiscam->height >> 1)))
				continue;

			if (thiscam->z + (thiscam->height >> 1) < *rover->topheight)
				return true;
		}
	}

	return false;
}

void P_DestroyRobots(void)
{
	// Search through all the thinkers for enemies.
	mobj_t *mo;
	thinker_t *think;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;
		if (mo->health <= 0 || !(mo->flags & MF_ENEMY || mo->flags & MF_BOSS))
			continue; // not a valid enemy

		if (mo->type == MT_PLAYER) // Don't chase after other players!
			continue;

		// Found a target enemy
		P_KillMobj(mo, players[consoleplayer].mo, players[consoleplayer].mo);
	}
}

// P_CameraThinker
//
// Process the mobj-ish required functions of the camera
boolean P_CameraThinker(player_t *player, camera_t *thiscam, boolean resetcalled)
{
	boolean itsatwodlevel = false;
	postimg_t postimg = postimg_none;
	if (twodlevel
		|| (thiscam == &camera && players[displayplayer].mo && (players[displayplayer].mo->flags2 & MF2_TWOD))
		|| (thiscam == &camera2 && players[secondarydisplayplayer].mo && (players[secondarydisplayplayer].mo->flags2 & MF2_TWOD)))
		itsatwodlevel = true;

	// Are we in water?
	if (player->pflags & PF_FLIPCAM && player->mo->eflags & MFE_VERTICALFLIP)
		postimg = postimg_flip;
	else if (P_CameraCheckWater(thiscam))
		postimg = postimg_water;
	else if (P_CameraCheckHeat(thiscam))
		postimg = postimg_heat;

	if (postimg != postimg_none)
	{
		if (splitscreen && player == &players[secondarydisplayplayer])
			postimgtype2 = postimg;
		else
			postimgtype = postimg;
	}

	if (thiscam->momx || thiscam->momy)
	{
		if (!P_TryCameraMove(thiscam->x + thiscam->momx, thiscam->y + thiscam->momy, thiscam))
		{ // Never fails for 2D mode.
			mobj_t dummy;
			dummy.thinker.function.acp1 = (actionf_p1)P_MobjThinker;
			dummy.subsector = thiscam->subsector;
			dummy.x = thiscam->x;
			dummy.y = thiscam->y;
			dummy.z = thiscam->z;
			dummy.height = thiscam->height;
			if (!resetcalled && !(player->pflags & PF_NOCLIP) && !P_CheckSight(&dummy, player->mo)) // TODO: "P_CheckCameraSight" instead.
				P_ResetCamera(player, thiscam);
			else
				P_SlideCameraMove(thiscam);
			if (resetcalled) // Okay this means the camera is fully reset.
				return true;
		}
	}

	if (!itsatwodlevel)
		P_CheckCameraPosition(thiscam->x, thiscam->y, thiscam);

	thiscam->subsector = R_PointInSubsector(thiscam->x, thiscam->y);
	thiscam->floorz = tmfloorz;
	thiscam->ceilingz = tmceilingz;

	if (thiscam->momz)
	{
		// adjust height
		thiscam->z += thiscam->momz;

		if (!itsatwodlevel && !(player->pflags & PF_NOCLIP))
		{
			// clip movement
			if (thiscam->z <= thiscam->floorz) // hit the floor
			{
				fixed_t cam_height = cv_cam_height.value;
				thiscam->z = thiscam->floorz;

				if (player == &players[secondarydisplayplayer])
					cam_height = cv_cam2_height.value;
				if (thiscam->z > player->mo->z + player->mo->height + FixedMul(cam_height*FRACUNIT + 16*FRACUNIT, player->mo->scale))
				{
					if (!resetcalled)
						P_ResetCamera(player, thiscam);
					return true;
				}
			}

			if (thiscam->z + thiscam->height > thiscam->ceilingz)
			{
				if (thiscam->momz > 0)
				{
					// hit the ceiling
					thiscam->momz = 0;
				}

				thiscam->z = thiscam->ceilingz - thiscam->height;

				if (thiscam->z + thiscam->height < player->mo->z - player->mo->height)
				{
					if (!resetcalled)
						P_ResetCamera(player, thiscam);
					return true;
				}
			}
		}
	}

	if (itsatwodlevel
	|| (thiscam->ceilingz - thiscam->z < thiscam->height
		&& thiscam->ceilingz >= thiscam->z))
	{
		thiscam->ceilingz = thiscam->z + thiscam->height;
		thiscam->floorz = thiscam->z;
	}
	return false;
}

//
// P_PlayerMobjThinker
//
static void P_PlayerMobjThinker(mobj_t *mobj)
{
	msecnode_t *node;
	boolean wasflip;

	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));

	wasflip = (mobj->eflags & MFE_VERTICALFLIP) != 0;
	P_MobjCheckWater(mobj);

	// momentum movement
	mobj->eflags &= ~MFE_JUSTSTEPPEDDOWN;

	// Zoom tube
	if (mobj->tracer && mobj->tracer->type == MT_TUBEWAYPOINT)
	{
		P_UnsetThingPosition(mobj);
		mobj->x += mobj->momx;
		mobj->y += mobj->momy;
		mobj->z += mobj->momz;
		P_SetThingPosition(mobj);
		P_CheckPosition(mobj, mobj->x, mobj->y);
		goto animonly;
	}
	else if (mobj->player && (mobj->player->pflags & PF_MACESPIN) && mobj->tracer)
	{
		P_CheckPosition(mobj, mobj->x, mobj->y);
		goto animonly;
	}

	// Needed for gravity boots
	P_CheckGravity(mobj, false);

	if (mobj->momx || mobj->momy)
	{
		P_XYMovement(mobj);

		if (P_MobjWasRemoved(mobj))
			return;
	}
	else
		P_TryMove(mobj, mobj->x, mobj->y, true);

	if ((wasflip && !(mobj->eflags & MFE_VERTICALFLIP))
	|| (!wasflip && (mobj->eflags & MFE_VERTICALFLIP)))
	{ // Flip aiming to match!
		G_GhostAddFlip();
		if (mobj->player->pflags & PF_FLIPCAM)
		{
			mobj->player->aiming = InvAngle(mobj->player->aiming);
			if (mobj->player-players == displayplayer)
			{
				localaiming = mobj->player->aiming;
				if (camera.chase) {
					camera.aiming = InvAngle(camera.aiming);
					camera.z = mobj->z - camera.z + mobj->z;
					if (mobj->eflags & MFE_VERTICALFLIP)
						camera.z += FixedMul(20*FRACUNIT, mobj->scale);
				}
			}
			else if (mobj->player-players == secondarydisplayplayer)
			{
				localaiming2 = mobj->player->aiming;
				if (camera2.chase) {
					camera2.aiming = InvAngle(camera2.aiming);
					camera2.z = mobj->z - camera2.z + mobj->z;
					if (mobj->eflags & MFE_VERTICALFLIP)
						camera2.z += FixedMul(20*FRACUNIT, mobj->scale);
				}
			}
		}
	}

	if (!(netgame && mobj->player && mobj->player->spectator))
	{
		// Crumbling platforms
		for (node = mobj->touching_sectorlist; node; node = node->m_snext)
		{
			ffloor_t *rover;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_CRUMBLE))
					continue;

				if ((*rover->topheight == mobj->z && !(mobj->eflags & MFE_VERTICALFLIP))
				|| (*rover->bottomheight == mobj->z + mobj->height && mobj->eflags & MFE_VERTICALFLIP)) // You nut.
					EV_StartCrumble(rover->master->frontsector, rover, (rover->flags & FF_FLOATBOB), mobj->player, rover->alpha, !(rover->flags & FF_NORETURN));
			}
		}
	}

	// Check for floating water platforms and bounce them
	if (CheckForFloatBob && P_MobjFlip(mobj)*mobj->momz < 0)
	{
		boolean thereiswater = false;

		for (node = mobj->touching_sectorlist; node; node = node->m_snext)
		{
			if (node->m_sector->ffloors)
			{
				ffloor_t *rover;
				// Get water boundaries first
				for (rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS))
						continue;

					if (rover->flags & FF_SWIMMABLE) // Is there water?
					{
						thereiswater = true;
						break;
					}
				}
			}
		}
		if (thereiswater)
		{
			for (node = mobj->touching_sectorlist; node; node = node->m_snext)
			{
				if (node->m_sector->ffloors)
				{
					ffloor_t *rover;
					for (rover = node->m_sector->ffloors; rover; rover = rover->next)
					{
						if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_FLOATBOB))
							continue;

						if ((!(mobj->eflags & MFE_VERTICALFLIP) && abs(*rover->topheight-mobj->z) <= abs(mobj->momz)) // The player is landing on the cheese!
						|| (mobj->eflags & MFE_VERTICALFLIP && abs(*rover->bottomheight-(mobj->z+mobj->height)) <= abs(mobj->momz)))
						{
							// Initiate a 'bouncy' elevator function
							// which slowly diminishes.
							EV_BounceSector(rover->master->frontsector, -mobj->momz, rover->master);
						}
					}
				}
			}
		} // Ugly ugly billions of braces! Argh!
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if (!(mobj->eflags & MFE_ONGROUND) || mobj->momz
		|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height != mobj->ceilingz)
		|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z != mobj->floorz)
		|| ((mobj->eflags & (MFE_UNDERWATER|MFE_GOOWATER)) == (MFE_UNDERWATER|MFE_GOOWATER)))
	{
		P_PlayerZMovement(mobj);
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!

		if (P_MobjWasRemoved(mobj))
			return;
	}
	else
	{
		mobj->player->jumping = 0;
		mobj->player->pflags &= ~PF_JUMPED;
		mobj->eflags &= ~MFE_JUSTHITFLOOR;
	}

animonly:
	// cycle through states,
	// calling action functions at transitions
	if (mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if (!mobj->tics)
			if (!P_SetPlayerMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
	}
}

static void CalculatePrecipFloor(precipmobj_t *mobj)
{
	// recalculate floorz each time
	const sector_t *mobjsecsubsec;
	if (mobj && mobj->subsector && mobj->subsector->sector)
		mobjsecsubsec = mobj->subsector->sector;
	else
		return;
	mobj->floorz = mobjsecsubsec->floorheight;
	if (mobjsecsubsec->ffloors)
	{
		ffloor_t *rover;

		for (rover = mobjsecsubsec->ffloors; rover; rover = rover->next)
		{
			// If it exists, it'll get rained on.
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_BLOCKOTHERS) && !(rover->flags & FF_SWIMMABLE))
				continue;

			if (*rover->topheight > mobj->floorz)
				mobj->floorz = *rover->topheight;
		}
	}
}

void P_RecalcPrecipInSector(sector_t *sector)
{
	mprecipsecnode_t *psecnode;

	if (!sector)
		return;

	sector->moved = true; // Recalc lighting and things too, maybe

	for (psecnode = sector->touching_preciplist; psecnode; psecnode = psecnode->m_snext)
		CalculatePrecipFloor(psecnode->m_thing);
}

//
// P_NullPrecipThinker
//
// For "Blank" precipitation
//
void P_NullPrecipThinker(precipmobj_t *mobj)
{
	(void)mobj;
}

void P_SnowThinker(precipmobj_t *mobj)
{
	// adjust height
	if ((mobj->z += mobj->momz) <= mobj->floorz)
		mobj->z = mobj->ceilingz;
}

void P_RainThinker(precipmobj_t *mobj)
{
	if (mobj->state != &states[S_RAIN1])
	{
		// cycle through states,
		// calling action functions at transitions
		if (mobj->tics > 0 && --mobj->tics == 0)
		{
			// you can cycle through multiple states in a tic
			if (!P_SetPrecipMobjState(mobj, mobj->state->nextstate))
				return; // freed itself
		}

		if (mobj->state == &states[S_RAINRETURN])
		{
			mobj->z = mobj->ceilingz;
			P_SetPrecipMobjState(mobj, S_RAIN1);
		}
		return;
	}

	// adjust height
	mobj->z += mobj->momz;

	if (mobj->z <= mobj->floorz)
	{
		// no splashes on sky or bottomless pits
		if (mobj->precipflags & PCF_PIT)
			mobj->z = mobj->ceilingz;
		else
		{
			mobj->z = mobj->floorz;
			P_SetPrecipMobjState(mobj, S_SPLASH1);
		}
	}
}

static void P_RingThinker(mobj_t *mobj)
{
	if (mobj->momx || mobj->momy)
	{
		P_RingXYMovement(mobj);

		if (P_MobjWasRemoved(mobj))
			return;
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if (mobj->momz)
	{
		P_RingZMovement(mobj);
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!

		if (P_MobjWasRemoved(mobj))
			return;
	}

	P_CycleMobjState(mobj);
}

//
// P_BossTargetPlayer
// If closest is true, find the closest player.
// Returns true if a player is targeted.
//
boolean P_BossTargetPlayer(mobj_t *actor, boolean closest)
{
	INT32 stop = -1, c = 0;
	player_t *player;
	fixed_t dist, lastdist = 0;

	// first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_Random();
	actor->lastlook &= PLAYERSMASK;

	for( ; ; actor->lastlook = (actor->lastlook+1) & PLAYERSMASK)
	{
		// save the first look so we stop next time.
		if (stop < 0)
			stop = actor->lastlook;
		// reached the beginning again, done looking.
		else if (actor->lastlook == stop)
			return (closest && lastdist > 0);

		if (!playeringame[actor->lastlook])
			continue;

		if (!closest && c++ == 2)
			return false;

		player = &players[actor->lastlook];

		if (player->health <= 0)
			continue; // dead

		if (player->pflags & PF_INVIS || player->bot || player->spectator)
			continue; // ignore notarget

		if (!player->mo || P_MobjWasRemoved(player->mo))
			continue;

		if (!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if (closest)
		{
			dist = P_AproxDistance(actor->x - player->mo->x, actor->y - player->mo->y);
			if (!lastdist || dist < lastdist)
			{
				lastdist = dist+1;
				P_SetTarget(&actor->target, player->mo);
			}
			continue;
		}

		P_SetTarget(&actor->target, player->mo);
		return true;
	}
}

// Finds the player no matter what they're hiding behind (even lead!)
boolean P_SupermanLook4Players(mobj_t *actor)
{
	INT32 c, stop = 0;
	player_t *playersinthegame[MAXPLAYERS];

	for (c = 0; c < MAXPLAYERS; c++)
	{
		if (playeringame[c])
		{
			if (players[c].health <= 0)
				continue; // dead

			if (players[c].pflags & PF_INVIS)
				continue; // ignore notarget

			if (!players[c].mo || players[c].bot)
				continue;

			playersinthegame[stop] = &players[c];
			stop++;
		}
	}

	if (!stop)
		return false;

	P_SetTarget(&actor->target, playersinthegame[P_RandomKey(stop)]->mo);
	return true;
}

// AI for a generic boss.
static void P_GenericBossThinker(mobj_t *mobj)
{
	if (mobj->state->nextstate == mobj->info->spawnstate && mobj->tics == 1)
		mobj->flags2 &= ~MF2_FRET;

	if (!mobj->target || !(mobj->target->flags & MF_SHOOTABLE))
	{
		if (mobj->health <= 0)
		{
			// look for a new target
			if (P_BossTargetPlayer(mobj, false) && mobj->info->mass) // Bid farewell!
				S_StartSound(mobj, mobj->info->mass);
			return;
		}

		// look for a new target
		if (P_BossTargetPlayer(mobj, false) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	// Don't call A_ functions here, let the SOC do the AI!

	if (mobj->state == &states[mobj->info->meleestate]
		|| (mobj->state == &states[mobj->info->missilestate]
		&& mobj->health > mobj->info->damage))
	{
		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
	}
}

// AI for the first boss.
static void P_Boss1Thinker(mobj_t *mobj)
{
	if (mobj->flags2 & MF2_FRET && (statenum_t)(mobj->state-states) == mobj->info->spawnstate) {
		mobj->flags2 &= ~(MF2_FRET|MF2_SKULLFLY);
		mobj->momx = mobj->momy = mobj->momz = 0;
	}

	if (!mobj->tracer)
	{
		var1 = 0;
		A_BossJetFume(mobj);
	}

	if (!mobj->target || !(mobj->target->flags & MF_SHOOTABLE))
	{
		if (mobj->target && mobj->target->health
		&& mobj->target->type == MT_EGGMOBILE_TARGET) // Oh, we're just firing our laser.
			return; // It's okay, then.

		if (mobj->health <= 0)
		{
			if (P_BossTargetPlayer(mobj, false) && mobj->info->mass) // Bid farewell!
				S_StartSound(mobj, mobj->info->mass);
			return;
		}

		// look for a new target
		if (P_BossTargetPlayer(mobj, false) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	if (mobj->state != &states[mobj->info->spawnstate] && mobj->health > 0 && mobj->flags & MF_FLOAT && !(mobj->flags2 & MF2_SKULLFLY))
		mobj->momz = FixedMul(mobj->momz,7*FRACUNIT/8);

	if (mobj->state == &states[mobj->info->meleestate]
		|| (mobj->state == &states[mobj->info->missilestate]
		&& mobj->health > mobj->info->damage))
	{
		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
	}
}

// AI for the second boss.
// No, it does NOT convert "Boss" to a "Thinker". =P
static void P_Boss2Thinker(mobj_t *mobj)
{
	if (mobj->movecount)
		mobj->movecount--;

	if (!mobj->movecount)
		mobj->flags2 &= ~MF2_FRET;

	if (!mobj->tracer
#ifdef CHAOSISNOTDEADYET
		&& gametype != GT_CHAOS
#endif
		)
	{
		var1 = 0;
		A_BossJetFume(mobj);
	}

	if (mobj->health <= mobj->info->damage && (!mobj->target || !(mobj->target->flags & MF_SHOOTABLE)))
	{
		if (mobj->health <= 0)
		{
			// look for a new target
			if (P_BossTargetPlayer(mobj, false) && mobj->info->mass) // Bid farewell!
				S_StartSound(mobj, mobj->info->mass);
			return;
		}

		// look for a new target
		if (P_BossTargetPlayer(mobj, false) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	if (mobj->state == &states[mobj->info->spawnstate] && mobj->health > mobj->info->damage
#ifdef CHAOSISNOTDEADYET
	&& gametype != GT_CHAOS
#endif
	)
		A_Boss2Chase(mobj);
	else if (mobj->health > 0 && mobj->state != &states[mobj->info->painstate] && mobj->state != &states[mobjinfo[mobj->info->missilestate].raisestate])
	{
		mobj->flags &= ~MF_NOGRAVITY;
		A_Boss2Pogo(mobj);
		P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
	}
}

// AI for the third boss.
//
// Notes for reminders:
// movedir = move 2x fast?
// movecount = fire missiles?
// reactiontime = shock the water?
// threshold = next waypoint #
// extravalue1 = previous time shock sound was used
//
static void P_Boss3Thinker(mobj_t *mobj)
{
	if (mobj->state == &states[mobj->info->spawnstate])
		mobj->flags2 &= ~MF2_FRET;

	if (mobj->flags2 & MF2_FRET)
		mobj->movedir = 1;

	if (!mobj->tracer)
	{
		var1 = 1;
		A_BossJetFume(mobj);
	}

	if (mobj->health <= 0)
	{
		mobj->movecount = 0;
		mobj->reactiontime = 0;

		if (mobj->state < &states[mobj->info->xdeathstate])
			return;

		if (mobj->threshold == -1)
		{
			mobj->momz = mobj->info->speed;
			return;
		}
	}

	if (mobj->reactiontime) // Shock mode
	{
		UINT32 i;

		if (mobj->state != &states[mobj->info->spawnstate])
			P_SetMobjState(mobj, mobj->info->spawnstate);

		mobj->reactiontime--;
		if (!mobj->reactiontime)
		{
			ffloor_t *rover;

			// Shock the water
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;

				if (!players[i].mo)
					continue;

				if (players[i].mo->health <= 0)
					continue;

				if (players[i].mo->eflags & MFE_UNDERWATER)
					P_DamageMobj(players[i].mo, mobj, mobj, 1);
			}

			// Make the water flash
			for (i = 0; i < numsectors; i++)
			{
				if (!sectors[i].ffloors)
					continue;

				for (rover = sectors[i].ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_EXISTS))
						continue;

					if (!(rover->flags & FF_SWIMMABLE))
						continue;

					P_SpawnLightningFlash(rover->master->frontsector);
					break;
				}
			}

			if ((UINT32)mobj->extravalue1 + TICRATE*2 < leveltime)
			{
				mobj->extravalue1 = (INT32)leveltime;
				S_StartSound(0, sfx_buzz1);
			}

			// If in the center, check to make sure
			// none of the players are in the water
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;

				if (!players[i].mo || players[i].bot)
					continue;

				if (players[i].mo->health <= 0)
					continue;

				if (players[i].mo->eflags & MFE_UNDERWATER)
				{ // Stay put
					mobj->reactiontime = 2*TICRATE;
					return;
				}
			}
		}

		if (!mobj->reactiontime && mobj->health <= mobj->info->damage)
		{ // Spawn pinch dummies from the center when we're leaving it.
			mobj_t *dummy;
			SINT8 way = mobj->threshold - 1; // 0 through 4.
			SINT8 way2;

			way = (way + P_RandomRange(1,3)) % 5; // dummy 1 at one of the first three options after eggmobile
			dummy = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->info->mass);
			dummy->angle = mobj->angle;
			dummy->threshold = way + 1;
			dummy->tracer = mobj;

			do
				way2 = (way + P_RandomRange(1,3)) % 5; // dummy 2 has to be careful,
			while (way2 == mobj->threshold - 1); // to make sure it doesn't try to go the Eggman Way if dummy 1 rolled high.
			dummy = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->info->mass);
			dummy->angle = mobj->angle;
			dummy->threshold = way2 + 1;
			dummy->tracer = mobj;

			CONS_Debug(DBG_GAMELOGIC, "Eggman path %d - Dummy selected paths %d and %d\n", mobj->threshold, way + 1, dummy->threshold);
			P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
		}
	}
	else if (mobj->movecount) // Firing mode
	{
		UINT32 i;

		// look for a new target
		P_BossTargetPlayer(mobj, false);

		if (!mobj->target || !mobj->target->player)
			return;

		// Are there any players underwater? If so, shock them!
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo || players[i].bot)
				continue;

			if (players[i].mo->health <= 0)
				continue;

			if (players[i].mo->eflags & MFE_UNDERWATER)
			{
				mobj->movecount = 0;
				P_SetMobjState(mobj, mobj->info->spawnstate);
				return;
			}
		}

		// Always face your target.
		A_FaceTarget(mobj);

		// Check if the attack animation is running. If not, play it.
		if (mobj->state < &states[mobj->info->missilestate] || mobj->state > &states[mobj->info->raisestate])
		{
			if (mobj->health <= mobj->info->damage) // pinch phase
				mobj->movecount--; // limited number of shots before diving again
			if (mobj->movecount)
				P_SetMobjState(mobj, mobj->info->missilestate);
		}
	}
	else if (mobj->threshold >= 0) // Traveling mode
	{
		thinker_t *th;
		mobj_t *mo2;
		fixed_t dist, dist2;
		fixed_t speed;

		P_SetTarget(&mobj->target, NULL);

		if (mobj->state != &states[mobj->info->spawnstate] && mobj->health > 0
			&& !(mobj->flags2 & MF2_FRET))
			P_SetMobjState(mobj, mobj->info->spawnstate);

		// scan the thinkers
		// to find a point that matches
		// the number
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;
			if (mo2->type == MT_BOSS3WAYPOINT && mo2->spawnpoint && mo2->spawnpoint->angle == mobj->threshold)
			{
				P_SetTarget(&mobj->target, mo2);
				break;
			}
		}

		if (!mobj->target) // Should NEVER happen
		{
			CONS_Debug(DBG_GAMELOGIC, "Error: Boss 3 was unable to find specified waypoint: %d\n", mobj->threshold);
			return;
		}

		dist = P_AproxDistance(P_AproxDistance(mobj->target->x - mobj->x, mobj->target->y - mobj->y), mobj->target->z - mobj->z);

		if (dist < 1)
			dist = 1;

		if ((mobj->movedir) || (mobj->health <= mobj->info->damage))
			speed = mobj->info->speed * 2;
		else
			speed = mobj->info->speed;

		mobj->momx = FixedMul(FixedDiv(mobj->target->x - mobj->x, dist), speed);
		mobj->momy = FixedMul(FixedDiv(mobj->target->y - mobj->y, dist), speed);
		mobj->momz = FixedMul(FixedDiv(mobj->target->z - mobj->z, dist), speed);

		if (mobj->momx != 0 || mobj->momy != 0)
			mobj->angle = R_PointToAngle2(0, 0, mobj->momx, mobj->momy);

		dist2 = P_AproxDistance(P_AproxDistance(mobj->target->x - (mobj->x + mobj->momx), mobj->target->y - (mobj->y + mobj->momy)), mobj->target->z - (mobj->z + mobj->momz));

		if (dist2 < 1)
			dist2 = 1;

		if ((dist >> FRACBITS) <= (dist2 >> FRACBITS))
		{
			// If further away, set XYZ of mobj to waypoint location
			P_UnsetThingPosition(mobj);
			mobj->x = mobj->target->x;
			mobj->y = mobj->target->y;
			mobj->z = mobj->target->z;
			mobj->momx = mobj->momy = mobj->momz = 0;
			P_SetThingPosition(mobj);

			if (mobj->threshold == 0)
			{
				mobj->reactiontime = 1; // Bzzt! Shock the water!
				mobj->movedir = 0;

				if (mobj->health <= 0)
				{
					mobj->flags |= MF_NOGRAVITY|MF_NOCLIP;
					mobj->flags |= MF_NOCLIPHEIGHT;
					mobj->threshold = -1;
					return;
				}
			}

			// Set to next waypoint in sequence
			if (mobj->target->spawnpoint)
			{
				// From the center point, choose one of the five paths
				if (mobj->target->spawnpoint->angle == 0)
					mobj->threshold = P_RandomRange(1,5);
				else
					mobj->threshold = mobj->target->spawnpoint->extrainfo;

				// If the deaf flag is set, go into firing mode
				if (mobj->target->spawnpoint->options & MTF_AMBUSH)
					mobj->movecount = mobj->health+1;
			}
			else // This should never happen, as well
				CONS_Debug(DBG_GAMELOGIC, "Error: Boss 3 waypoint has no spawnpoint associated with it.\n");
		}
	}
}

// Move Boss4's sectors by delta.
static boolean P_Boss4MoveCage(fixed_t delta)
{
	const UINT16 tag = 65534;
	INT32 snum;
	sector_t *sector;
	for (snum = sectors[tag%numsectors].firsttag; snum != -1; snum = sector->nexttag)
	{
		sector = &sectors[snum];
		if (sector->tag != tag)
			continue;
		sector->floorheight += delta;
		sector->ceilingheight += delta;
		P_CheckSector(sector, true);
	}
	return sectors[tag%numsectors].firsttag != -1;
}

// Move Boss4's arms to angle
static void P_Boss4MoveSpikeballs(mobj_t *mobj, angle_t angle, fixed_t fz)
{
	INT32 s;
	mobj_t *base = mobj, *seg;
	fixed_t dist, bz = (mobj->spawnpoint->z+16)<<FRACBITS;
	while ((base = base->tracer))
	{
		for (seg = base, dist = 172*FRACUNIT, s = 9; seg; seg = seg->hnext, dist += 124*FRACUNIT, --s)
			P_TeleportMove(seg, mobj->x + P_ReturnThrustX(mobj, angle, dist), mobj->y + P_ReturnThrustY(mobj, angle, dist), bz + FixedMul(fz, FixedDiv(s<<FRACBITS, 9<<FRACBITS)));
		angle += ANGLE_MAX/3;
	}
}

// Pull them closer.
static void P_Boss4PinchSpikeballs(mobj_t *mobj, angle_t angle, fixed_t fz)
{
	INT32 s;
	mobj_t *base = mobj, *seg;
	fixed_t dist, bz = (mobj->spawnpoint->z+16)<<FRACBITS;
	while ((base = base->tracer))
	{
		for (seg = base, dist = 112*FRACUNIT, s = 9; seg; seg = seg->hnext, dist += 132*FRACUNIT, --s)
		{
			seg->z = bz + FixedMul(fz, FixedDiv(s<<FRACBITS, 9<<FRACBITS));
			P_TryMove(seg, mobj->x + P_ReturnThrustX(mobj, angle, dist), mobj->y + P_ReturnThrustY(mobj, angle, dist), true);
		}
		angle += ANGLE_MAX/3;
	}
}

// Destroy cage FOFs.
static void P_Boss4DestroyCage(void)
{
	const UINT16 tag = 65534;
	INT32 snum, next;
	size_t a;
	sector_t *sector, *rsec;
	ffloor_t *rover;

	// This will be the final iteration of sector tag.
	// We'll destroy the tag list as we go.
	next = sectors[tag%numsectors].firsttag;
	sectors[tag%numsectors].firsttag = -1;

	for (snum = next; snum != -1; snum = next)
	{
		sector = &sectors[snum];

		next = sector->nexttag;
		sector->nexttag = -1;
		if (sector->tag != tag)
			continue;
		sector->tag = 0;

		// Destroy the FOFs.
		for (a = 0; a < sector->numattached; a++)
		{
			rsec = &sectors[sector->attached[a]];
			for (rover = rsec->ffloors; rover; rover = rover->next)
				if (rover->flags & FF_EXISTS && rover->secnum == (size_t)snum)
				{
					if (rover->flags & FF_RENDERALL) // checking for FF_RENDERANY.
						EV_CrumbleChain(rsec, rover); // This FOF is visible to some extent? Crumble it.
					else // Completely invisible FOF
					{
						// no longer exists (can't collide with again)
						rover->flags &= ~FF_EXISTS;
						sector->moved = true;
						rsec->moved = true;
					}
				}
		}
	}
}

// Destroy Boss4's arms
static void P_Boss4PopSpikeballs(mobj_t *mobj)
{
	mobj_t *base = mobj->tracer, *seg, *next;
	P_SetTarget(&mobj->tracer, NULL);
	while(base)
	{
		next = base->tracer;
		P_SetTarget(&base->tracer, NULL);
		for (seg = base; seg; seg = seg->hnext)
			if (seg->health)
				P_KillMobj(seg, NULL, NULL);
		base = next;
	}
}

//
// AI for the fourth boss.
//
static void P_Boss4Thinker(mobj_t *mobj)
{
	if ((statenum_t)(mobj->state-states) == mobj->info->spawnstate)
	{
		if (mobj->health > mobj->info->damage || mobj->movedir == 4)
			mobj->flags2 &= ~MF2_FRET;
		mobj->reactiontime = 0; // Drop the cage immediately.
	}

	// Oh no, we dead? D:
	if (!mobj->health)
	{
		if (mobj->tracer) // need to clean up!
		{
			P_Boss4DestroyCage(); // Just in case pinch phase was skipped.
			P_Boss4PopSpikeballs(mobj);
		}
		return;
	}

	// movedir == battle stage:
	//   0: initialization
	//   1: phase 1 forward
	//   2: phase 1 reverse
	//   3: pinch rise
	//   4: pinch phase
	switch(mobj->movedir)
	{
	// WELCOME to your DOOM!
	case 0:
	{
		// For this stage only:
		// movecount == cage height
		// threshold == cage momz
		if (mobj->movecount == 0) // Initialize stage!
		{
			fixed_t z;
			INT32 i, arm;
			mobj_t *seg, *base = mobj;
			// First frame init, spawn all the things.
			mobj->spawnpoint->z = mobj->z>>FRACBITS;
			z = mobj->z + mobj->height/2 - mobjinfo[MT_EGGMOBILE4_MACE].height/2;
			for (arm = 0; arm <3 ; arm++)
			{
				seg = P_SpawnMobj(mobj->x, mobj->y, z, MT_EGGMOBILE4_MACE);
				P_SetTarget(&base->tracer, seg);
				base = seg;
				P_SetTarget(&seg->target, mobj);
				for (i = 0; i < 9; i++)
				{
					seg->hnext = P_SpawnMobj(mobj->x, mobj->y, z, MT_EGGMOBILE4_MACE);
					seg->hnext->hprev = seg;
					seg = seg->hnext;
				}
			}
			// Move the cage up to the sky.
			mobj->movecount = 800*FRACUNIT;
			if (!P_Boss4MoveCage(mobj->movecount))
			{
				mobj->movecount = 0;
				mobj->threshold = 3*TICRATE;
				mobj->extravalue1 = 1;
				mobj->movedir++; // We don't have a cage, just continue.
			}
			else
				P_Boss4MoveSpikeballs(mobj, 0, mobj->movecount);
		}
		else // Cage slams down over Eggman's head!
		{
			fixed_t oldz = mobj->movecount;
			mobj->threshold -= 5*FRACUNIT;
			mobj->movecount += mobj->threshold;
			if (mobj->movecount < 0)
				mobj->movecount = 0;
			P_Boss4MoveCage(mobj->movecount - oldz);
			P_Boss4MoveSpikeballs(mobj, 0, mobj->movecount);
			if (mobj->movecount == 0)
			{
				mobj->threshold = 3*TICRATE;
				mobj->extravalue1 = 1;
				P_LinedefExecute(LE_BOSS4DROP, mobj, NULL);
				mobj->movedir++; // Initialization complete, next phase!
			}
		}
		return;
	}

	// Normal operation
	case 1:
	case 2:
		break;

	// Pinch phase init!
	case 3:
	{
		fixed_t z;
		if (mobj->z < (mobj->spawnpoint->z+512)<<FRACBITS)
			mobj->momz = 8*FRACUNIT;
		else
		{
			mobj->momz = 0;
			mobj->movedir++;
		}
		mobj->movecount += 400<<(FRACBITS>>1);
		mobj->movecount %= 360*FRACUNIT;
		z = mobj->z - (mobj->spawnpoint->z<<FRACBITS) - mobjinfo[MT_EGGMOBILE4_MACE].height - mobj->height/2;
		if (z < 0) // We haven't risen high enough to pull the spikeballs along yet
			P_Boss4MoveSpikeballs(mobj, FixedAngle(mobj->movecount), 0); // So don't pull the spikeballs along yet.
		else
			P_Boss4PinchSpikeballs(mobj, FixedAngle(mobj->movecount), z);
		return;
	}
	// Pinch phase!
	case 4:
	{
		if (mobj->z < (mobj->spawnpoint->z+512+128*(mobj->info->damage-mobj->health))<<FRACBITS)
			mobj->momz = 8*FRACUNIT;
		else
			mobj->momz = 0;
		mobj->movecount += (800+800*(mobj->info->damage-mobj->health))<<(FRACBITS>>1);
		mobj->movecount %= 360*FRACUNIT;
		P_Boss4PinchSpikeballs(mobj, FixedAngle(mobj->movecount), mobj->z - (mobj->spawnpoint->z<<FRACBITS) - mobjinfo[MT_EGGMOBILE4_MACE].height - mobj->height/2);

		if (!mobj->target || !mobj->target->health)
			P_SupermanLook4Players(mobj);
		A_FaceTarget(mobj);
		return;
	}

	default: // ?????
		return;
	}

	// Haahahahahaaa, and let the FUN.. BEGIN!
	// movedir   == arms direction
	// movecount == arms angle
	// threshold == countdown to next attack
	// reactiontime == cage raise, speed burst
	// movefactor == cage z
	// friction == turns until helm lift

	// Raise the cage!
	if (mobj->reactiontime == 1)
	{
		fixed_t oldz = mobj->movefactor;
		mobj->movefactor += 8*FRACUNIT;
		if (mobj->movefactor > 128*FRACUNIT)
			mobj->movefactor = 128*FRACUNIT;
		P_Boss4MoveCage(mobj->movefactor - oldz);
	}
	// Drop the cage!
	else if (mobj->movefactor)
	{
		fixed_t oldz = mobj->movefactor;
		mobj->movefactor -= 4*FRACUNIT;
		if (mobj->movefactor < 0)
			mobj->movefactor = 0;
		P_Boss4MoveCage(mobj->movefactor - oldz);
		if (!mobj->movefactor)
		{
			if (mobj->health <= mobj->info->damage)
			{ // Proceed to pinch phase!
				P_Boss4DestroyCage();
				mobj->movedir = 3;
				P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
				return;
			}
			P_LinedefExecute(LE_BOSS4DROP, mobj, NULL);
		}
	}

	{
		fixed_t movespeed = 170<<(FRACBITS>>1);
		if (mobj->reactiontime == 2)
			movespeed *= 3;
		if (mobj->movedir == 2)
			mobj->movecount -= movespeed;
		else
			mobj->movecount += movespeed;
	}
	mobj->movecount %= 360*FRACUNIT;
	P_Boss4MoveSpikeballs(mobj, FixedAngle(mobj->movecount), mobj->movefactor);

	// Check for attacks, always tick the timer even while animating!!
	if (!(mobj->flags2 & MF2_FRET) // but pause for pain so we don't interrupt pinch phase, eep!
	&& mobj->threshold-- == 0)
	{
		// 5 -> 2.5 second timer
		mobj->threshold = 5*TICRATE-(TICRATE/2)*(mobj->info->spawnhealth-mobj->health);
		if (mobj->threshold < 1)
			mobj->threshold = 1;

		if (mobj->extravalue1-- == 0)
		{
			P_SetMobjState(mobj, mobj->info->raisestate);
			mobj->extravalue1 = 3;
		}
		else
		{
			if (mobj->reactiontime == 1) // Cage is raised?
				mobj->reactiontime = 0; // Drop it!
			switch(P_RandomKey(10))
			{
				// Telegraph Right (Speed Up!!)
				case 1:
				case 3:
				case 4:
				case 5:
				case 6:
					P_SetMobjState(mobj, mobj->info->missilestate);
					break;
				// Telegraph Left (Reverse Direction)
				default:
					P_SetMobjState(mobj, mobj->info->meleestate);
					break;
			}
		}
	}

	// Leave if animating.
	if ((statenum_t)(mobj->state-states) != mobj->info->spawnstate)
		return;

	// Map allows us to get killed despite cage being down?
	if (mobj->health <= mobj->info->damage)
	{ // Proceed to pinch phase!
		P_Boss4DestroyCage();
		// spawn jet's flame now you're flying upwards
		// tracer is already used, so if this ever gets reached again we've got problems
		var1 = 3;
		A_BossJetFume(mobj);
		mobj->movedir = 3;
		P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
		return;
	}

	mobj->reactiontime = 0; // Drop the cage if it hasn't been dropped already.
	if (!mobj->target || !mobj->target->health)
		P_SupermanLook4Players(mobj);
	A_FaceTarget(mobj);
}

//
// AI for Black Eggman
// Note: You CANNOT have more than ONE Black Eggman
// in a level! Just don't try it!
//
static void P_Boss7Thinker(mobj_t *mobj)
{
	if (!mobj->target || !(mobj->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_BossTargetPlayer(mobj, false))
			return; // got a new target

		P_SetMobjStateNF(mobj, mobj->info->spawnstate);
		return;
	}

	if (mobj->health >= mobj->info->spawnhealth && (leveltime & 14) == 0)
	{
		mobj_t *smoke = P_SpawnMobj(mobj->x, mobj->y, mobj->z + mobj->height, MT_SMOKE);
		smoke->destscale = mobj->destscale;
		P_SetScale(smoke, smoke->destscale);
		smoke->momz = FixedMul(FRACUNIT, smoke->scale);
	}

	if (mobj->state == &states[S_BLACKEGG_STND] && mobj->tics == mobj->state->tics)
	{
		mobj->reactiontime += P_Random();

		if (mobj->health <= mobj->info->damage)
			mobj->reactiontime /= 4;
	}
	else if (mobj->state == &states[S_BLACKEGG_DIE4] && mobj->tics == mobj->state->tics)
		A_BossDeath(mobj);
	else if (mobj->state >= &states[S_BLACKEGG_WALK1]
		&& mobj->state <= &states[S_BLACKEGG_WALK6])
		A_Boss7Chase(mobj);
	else if (mobj->state == &states[S_BLACKEGG_PAIN1] && mobj->tics == mobj->state->tics)
	{
		if (mobj->health > 0)
			mobj->health--;

		S_StartSound(0, (mobj->health) ? sfx_behurt : sfx_bedie2);

		mobj->reactiontime /= 3;

		if (mobj->health <= 0)
		{
			INT32 i;

			P_KillMobj(mobj, NULL, NULL);

			// It was a team effort
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;

				P_AddPlayerScore(&players[i], 1000);
			}
		}
	}
	else if (mobj->state == &states[S_BLACKEGG_PAIN35] && mobj->tics == 1)
	{
		if (mobj->health == mobj->info->damage)
		{
			// Begin platform destruction
			mobj->flags2 |= MF2_FRET;
			P_SetMobjState(mobj, mobj->info->raisestate);
			P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
		}
	}
	else if (mobj->state == &states[S_BLACKEGG_HITFACE4] && mobj->tics == mobj->state->tics)
	{
		// This is where Black Eggman hits his face.
		// If a player is on top of him, the player gets hurt.
		// But, if the player has managed to escape,
		// Black Eggman gets hurt!
		INT32 i;
		mobj->state->nextstate = mobj->info->painstate; // Reset

		S_StartSound(0, sfx_bedeen);

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo)
				continue;

			if (players[i].mo->health <= 0)
				continue;

			if (P_AproxDistance(players[i].mo->x - mobj->x, players[i].mo->y - mobj->y) > (mobj->radius + players[i].mo->radius))
				continue;

			if (players[i].mo->z > mobj->z + mobj->height - FRACUNIT
				&& players[i].mo->z < mobj->z + mobj->height + 128*FRACUNIT) // You can't be in the vicinity, either...
			{
				// Punch him!
				P_DamageMobj(players[i].mo, mobj, mobj, 1);
				mobj->state->nextstate = mobj->info->spawnstate;

				// Laugh
				S_StartSound(0, sfx_bewar1 + P_RandomKey(4));
			}
		}
	}
	else if (mobj->state == &states[S_BLACKEGG_GOOP])
	{
		// Lob cannon balls
		if (mobj->movecount-- <= 0 || !mobj->target)
		{
			P_SetMobjState(mobj, mobj->info->spawnstate);
			return;
		}

		if ((leveltime & 15) == 0)
		{
			var1 = MT_CANNONBALL;

			var2 = 2*TICRATE + (80<<16);

			A_LobShot(mobj);
			S_StartSound(0, sfx_begoop);
		}
	}
	else if (mobj->state == &states[S_BLACKEGG_SHOOT2])
	{
		// Chaingun goop
		mobj_t *missile;

		if (mobj->movecount-- <= 0 || !mobj->target)
		{
			P_SetMobjState(mobj, mobj->info->spawnstate);
			return;
		}

		A_FaceTarget(mobj);

		missile = P_SpawnXYZMissile(mobj, mobj->target, MT_BLACKEGGMAN_GOOPFIRE,
			mobj->x + P_ReturnThrustX(mobj, mobj->angle-ANGLE_90, FixedDiv(mobj->radius, 3*FRACUNIT/2)+(4*FRACUNIT)),
			mobj->y + P_ReturnThrustY(mobj, mobj->angle-ANGLE_90, FixedDiv(mobj->radius, 3*FRACUNIT/2)+(4*FRACUNIT)),
			mobj->z + FixedDiv(mobj->height, 3*FRACUNIT/2));

		S_StopSound(missile);

		if (leveltime & 1)
			S_StartSound(0, sfx_beshot);
	}
	else if (mobj->state == &states[S_BLACKEGG_JUMP1] && mobj->tics == 1)
	{
		mobj_t *hitspot = NULL, *mo2;
		angle_t an;
		fixed_t dist, closestdist;
		fixed_t vertical, horizontal;
		fixed_t airtime = 5*TICRATE;
		INT32 waypointNum = 0;
		thinker_t *th;
		INT32 i;
		boolean foundgoop = false;
		INT32 closestNum;

		// Looks for players in goop. If you find one, try to jump on him.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo)
				continue;

			if (players[i].mo->health <= 0)
				continue;

			if (players[i].powers[pw_ingoop])
			{
				closestNum = -1;
				closestdist = 16384*FRACUNIT; // Just in case...

				// Find waypoint he is closest to
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;
					if (mo2->type == MT_BOSS3WAYPOINT && mo2->spawnpoint)
					{
						dist = P_AproxDistance(players[i].mo->x - mo2->x, players[i].mo->y - mo2->y);

						if (closestNum == -1 || dist < closestdist)
						{
							closestNum = (mo2->spawnpoint->options & 7);
							closestdist = dist;
							foundgoop = true;
						}
					}
				}
				waypointNum = closestNum;
				break;
			}
		}

		if (!foundgoop)
		{
			if (mobj->z > 1056*FRACUNIT)
				waypointNum = 0;
			else
				waypointNum = 1 + P_RandomKey(4);
		}

		// Don't jump to the center when health is low.
		// Force the player to beat you with missiles.
		if (mobj->health <= mobj->info->damage && waypointNum == 0)
			waypointNum = 1 + P_RandomKey(4);

		if (mobj->tracer && mobj->tracer->type == MT_BOSS3WAYPOINT
			&& mobj->tracer->spawnpoint && (mobj->tracer->spawnpoint->options & 7) == waypointNum)
		{
			if (P_Random() & 1)
				waypointNum++;
			else
				waypointNum--;

			waypointNum %= 5;

			if (waypointNum < 0)
				waypointNum = 0;
		}

		if (waypointNum == 0 && mobj->health <= mobj->info->damage)
			waypointNum = 1 + (P_Random() & 1);

		// scan the thinkers to find
		// the waypoint to use
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;
			if (mo2->type == MT_BOSS3WAYPOINT && mo2->spawnpoint && (mo2->spawnpoint->options & 7) == waypointNum)
			{
				hitspot = mo2;
				break;
			}
		}

		if (hitspot == NULL)
		{
			CONS_Debug(DBG_GAMELOGIC, "BlackEggman unable to find waypoint #%d!\n", waypointNum);
			P_SetMobjState(mobj, mobj->info->spawnstate);
			return;
		}

		P_SetTarget(&mobj->tracer, hitspot);

		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, hitspot->x, hitspot->y);

		an = mobj->angle;
		an >>= ANGLETOFINESHIFT;

		dist = P_AproxDistance(hitspot->x - mobj->x, hitspot->y - mobj->y);

		horizontal = dist / airtime;
		vertical = (gravity*airtime)/2;

		mobj->momx = FixedMul(horizontal, FINECOSINE(an));
		mobj->momy = FixedMul(horizontal, FINESINE(an));
		mobj->momz = vertical;

//		mobj->momz = 10*FRACUNIT;
	}
	else if (mobj->state == &states[S_BLACKEGG_JUMP2] && mobj->z <= mobj->floorz)
	{
		// BANG! onto the ground
		INT32 i,j;
		fixed_t ns;
		fixed_t x,y,z;
		mobj_t *mo2;

		S_StartSound(0, sfx_befall);

		z = mobj->floorz;
		for (j = 0; j < 2; j++)
		{
			for (i = 0; i < 32; i++)
			{
				const angle_t fa = (i*FINEANGLES/16) & FINEMASK;
				ns = 64 * FRACUNIT;
				x = mobj->x + FixedMul(FINESINE(fa),ns);
				y = mobj->y + FixedMul(FINECOSINE(fa),ns);

				mo2 = P_SpawnMobj(x, y, z, MT_EXPLODE);
				ns = 16 * FRACUNIT;
				mo2->momx = FixedMul(FINESINE(fa),ns);
				mo2->momy = FixedMul(FINECOSINE(fa),ns);
			}
			z -= 32*FRACUNIT;
		}

		// Hurt player??
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo)
				continue;

			if (players[i].mo->health <= 0)
				continue;

			if (P_AproxDistance(players[i].mo->x - mobj->x, players[i].mo->y - mobj->y) > mobj->radius*4)
				continue;

			if (players[i].mo->z > mobj->z + 128*FRACUNIT)
				continue;

			if (players[i].mo->z < mobj->z - 64*FRACUNIT)
				continue;

			P_DamageMobj(players[i].mo, mobj, mobj, 1);

			// Laugh
			S_StartSound(0, sfx_bewar1 + P_RandomKey(4));
		}

		P_SetMobjState(mobj, mobj->info->spawnstate);
	}
	else if (mobj->state == &states[mobj->info->deathstate] && mobj->tics == mobj->state->tics)
		S_StartSound(0, sfx_bedie1 + (P_Random() & 1));

}

// Metal Sonic battle boss
// You CAN put multiple Metal Sonics in a single map
// because I am a totally competent programmer who can do shit right.
static void P_Boss9Thinker(mobj_t *mobj)
{
	if ((statenum_t)(mobj->state-states) == mobj->info->spawnstate)
		mobj->flags2 &= ~MF2_FRET;

	if (!mobj->tracer)
	{
		thinker_t *th;
		mobj_t *mo2;
		mobj_t *last=NULL;

		// Initialize the boss, spawn jet fumes, etc.
		mobj->threshold = 0;
		mobj->reactiontime = 0;
		mobj->watertop = mobj->floorz + 32*FRACUNIT;
		var1 = 2;
		A_BossJetFume(mobj);

		// Run through the thinkers ONCE and find all of the MT_BOSS9GATHERPOINT in the map.
		// Build a hoop linked list of 'em!
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;
			if (mo2->type == MT_BOSS9GATHERPOINT)
			{
				if (last)
					last->hnext = mo2;
				else
					mobj->hnext = mo2;
				mo2->hprev = last;
				last = mo2;
			}
		}
	}

	if (mobj->health <= 0)
		return;

	if ((!mobj->target || !(mobj->target->flags & MF_SHOOTABLE)))
	{
		P_BossTargetPlayer(mobj, false);
		if (mobj->target && (!P_IsObjectOnGround(mobj->target) || mobj->target->player->pflags & PF_SPINNING))
			P_SetTarget(&mobj->target, NULL); // Wait for them to hit the ground first
		if (!mobj->target) // Still no target, aww.
		{
			// Reset the boss.
			P_SetMobjState(mobj, mobj->info->spawnstate);
			mobj->fuse = 0;
			mobj->momx = FixedDiv(mobj->momx, FRACUNIT + (FRACUNIT>>2));
			mobj->momy = FixedDiv(mobj->momy, FRACUNIT + (FRACUNIT>>2));
			mobj->momz = FixedDiv(mobj->momz, FRACUNIT + (FRACUNIT>>2));
			return;
		}
		else if (!mobj->fuse)
			mobj->fuse = 10*TICRATE;
	}

	// AI goes here.
	{
		boolean danger = true;
		angle_t angle;
		if (mobj->threshold)
			mobj->momz = (mobj->watertop-mobj->z)/16; // Float to your desired position FASTER
		else
			mobj->momz = (mobj->watertop-mobj->z)/40; // Float to your desired position

		if (mobj->movecount == 2) {
			mobj_t *spawner;
			fixed_t dist = 0;
			angle = 0x06000000*leveltime;

			// Alter your energy bubble's size/position
			if (mobj->health > 3) {
				mobj->tracer->destscale = FRACUNIT + (4*TICRATE - mobj->fuse)*(FRACUNIT/2)/TICRATE + FixedMul(FINECOSINE(angle>>ANGLETOFINESHIFT),FRACUNIT/2);
				P_SetScale(mobj->tracer, mobj->tracer->destscale);
				P_TeleportMove(mobj->tracer, mobj->x, mobj->y, mobj->z + mobj->height/2 - mobj->tracer->height/2);
				mobj->tracer->momx = mobj->momx;
				mobj->tracer->momy = mobj->momy;
				mobj->tracer->momz = mobj->momz;
			}

			// Face your target
			P_BossTargetPlayer(mobj, true);
			angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y); // absolute angle
			angle = (angle-mobj->angle); // relative angle
			if (angle < ANGLE_180)
				mobj->angle += angle/8;
			else
				mobj->angle -= InvAngle(angle)/8;

			// Spawn energy particles
			for (spawner = mobj->hnext; spawner; spawner = spawner->hnext) {
				dist = P_AproxDistance(spawner->x - mobj->x, spawner->y - mobj->y);
				if (P_RandomRange(1,(dist>>FRACBITS)/16) == 1)
					break;
			}
			if (spawner) {
				mobj_t *missile = P_SpawnMissile(spawner, mobj, MT_MSGATHER);
				missile->momz = FixedDiv(missile->momz, 7*FRACUNIT/4);
				if (dist == 0)
					missile->fuse = 0;
				else
					missile->fuse = (dist/P_AproxDistance(missile->momx, missile->momy));
				if (missile->fuse > mobj->fuse)
					P_RemoveMobj(missile);
			}
		}

		// Pre-threshold reactiontime stuff for attack phases
		if (mobj->reactiontime && mobj->movecount == 3) {
			if (mobj->movedir == 0 || mobj->movedir == 2) { // Pausing between bounces in the pinball phase
				if (mobj->target->player->powers[pw_tailsfly]) // Trying to escape, eh?
					mobj->watertop = mobj->target->z + mobj->target->momz*6; // Readjust your aim. >:3
				else
					mobj->watertop = mobj->target->floorz + 16*FRACUNIT;
				if (!(mobj->threshold%4))
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x + mobj->target->momx*4, mobj->target->y + mobj->target->momy*4);
			}
			// Pausing between energy ball shots
			mobj->reactiontime--;
			return;
		}

		// threshold is used for attacks/maneuvers.
		if (mobj->threshold) {
			fixed_t speed = 20*FRACUNIT + FixedMul(40*FRACUNIT, FixedDiv((mobj->info->spawnhealth - mobj->health)<<FRACBITS, mobj->info->spawnhealth<<FRACBITS));
			int tries = 0;

			// Firin' mah lazors
			if (mobj->movecount == 3 && mobj->movedir == 1) {
				if (!(mobj->threshold&1)) {
					mobj_t *missile;
					if (mobj->info->seesound)
						S_StartSound(mobj, mobj->info->seesound);
					P_SetMobjState(mobj, mobj->info->missilestate);
					if (mobj->extravalue1 == 3)
						mobj->reactiontime = TICRATE/16;
					else
						mobj->reactiontime = TICRATE/8;

					A_FaceTarget(mobj);
					missile = P_SpawnMissile(mobj, mobj->target, mobj->info->speed);
					if (mobj->extravalue1 == 2 || mobj->extravalue1 == 3) {
						missile->destscale = FRACUNIT>>1;
						P_SetScale(missile, missile->destscale);
					}
					missile->fuse = 3*TICRATE;
					missile->z -= missile->height/2;

					if (mobj->extravalue1 == 2) {
						int i;
						mobj_t *spread;
						missile->flags |= MF_MISSILE;
						for (i = 0; i < 5; i++) {
							if (i == 2)
								continue;
							spread = P_SpawnMobj(missile->x, missile->y, missile->z, missile->type);
							spread->angle = missile->angle+(ANGLE_11hh/2)*(i-2);
							P_InstaThrust(spread,spread->angle,spread->info->speed);
							spread->momz = missile->momz;
							spread->destscale = FRACUNIT>>1;
							P_SetScale(spread, spread->destscale);
							spread->fuse = 3*TICRATE;
						}
						missile->flags &= ~MF_MISSILE;
					}
				} else {
					P_SetMobjState(mobj, mobj->state->nextstate);
					if (mobj->extravalue1 == 3)
						mobj->reactiontime = TICRATE/8;
					else
						mobj->reactiontime = TICRATE/4;
				}
				mobj->threshold--;
				return;
			}

			P_SpawnGhostMobj(mobj);

			// Pinball attack!
			if (mobj->movecount == 3 && (mobj->movedir == 0 || mobj->movedir == 2)) {
				if ((statenum_t)(mobj->state-states) != mobj->info->seestate)
					P_SetMobjState(mobj, mobj->info->seestate);
				if (mobj->movedir == 0) // mobj health == 1
					P_InstaThrust(mobj, mobj->angle, 38*FRACUNIT);
				else if (mobj->health == 3)
					P_InstaThrust(mobj, mobj->angle, 22*FRACUNIT);
				else // mobj health == 2
					P_InstaThrust(mobj, mobj->angle, 30*FRACUNIT);
				if (!P_TryMove(mobj, mobj->x+mobj->momx, mobj->y+mobj->momy, true)) { // Hit a wall? Find a direction to bounce
					mobj->threshold--;
					if (mobj->threshold) {
						P_SetMobjState(mobj, mobj->state->nextstate);
						if (mobj->info->mass)
							S_StartSound(mobj, mobj->info->mass);
						if (!(mobj->threshold%4)) { // We've decided to lock onto the player this bounce.
							mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x + mobj->target->momx*4, mobj->target->y + mobj->target->momy*4);
							mobj->reactiontime = TICRATE; // targetting time
						} else { // No homing, just use P_BounceMove
							P_BounceMove(mobj);
							mobj->angle = R_PointToAngle2(0,0,mobj->momx,mobj->momy);
							mobj->reactiontime = TICRATE/4; // just a pause before you bounce away
						}
						mobj->momx = mobj->momy = 0;
					}
				}
				return;
			}

			// Vector form dodge!
			mobj->angle += mobj->movedir;
			P_InstaThrust(mobj, mobj->angle, -speed);
			while (!P_TryMove(mobj, mobj->x+mobj->momx, mobj->y+mobj->momy, true) && tries++ < 16) {
				mobj->angle += mobj->movedir;
				P_InstaThrust(mobj, mobj->angle, -speed);
			}
			mobj->momx = mobj->momy = 0;
			mobj->threshold--;
			if (!mobj->threshold)
			{ // Go into stun after dodge.
				// from 3*TICRATE down to 1.25*TICRATE
				//mobj->reactiontime = 5*TICRATE/4 + (FixedMul((7*TICRATE/4)<<FRACBITS, FixedDiv((mobj->health-1)<<FRACBITS, (mobj->info->spawnhealth-1)<<FRACBITS))>>FRACBITS);
				// from 3*TICRATE down to 2*TICRATE
				mobj->reactiontime = 2*TICRATE + (FixedMul((1*TICRATE)<<FRACBITS, FixedDiv((mobj->health-1)<<FRACBITS, (mobj->info->spawnhealth-1)<<FRACBITS))>>FRACBITS);
				mobj->flags |= MF_SPECIAL|MF_SHOOTABLE;
				P_SetMobjState(mobj, mobj->state->nextstate);
			}
			return;
		}

		angle = 0x06000000*leveltime;
		mobj->momz += FixedMul(FINECOSINE(angle>>ANGLETOFINESHIFT),2*FRACUNIT); // Use that "angle" to bob gently in the air
		// This is below threshold because we don't want to bob while zipping around

		// Ohh you're in for it now..
		if (mobj->flags2 & MF2_FRET && mobj->health <= mobj->info->damage)
			mobj->fuse = 0;

		// reactiontime is used for delays.
		if (mobj->reactiontime)
		{
			// Stunned after vector form
			if (mobj->movedir > ANGLE_180)
				mobj->angle -= FixedAngle(FixedMul(AngleFixed(InvAngle(mobj->movedir)),FixedDiv(mobj->reactiontime<<FRACBITS,24<<FRACBITS)));
			else
				mobj->angle += FixedAngle(FixedMul(AngleFixed(mobj->movedir),FixedDiv(mobj->reactiontime<<FRACBITS,24<<FRACBITS)));
			mobj->reactiontime--;
			if (!mobj->reactiontime)
				// Out of stun.
				P_SetMobjState(mobj, mobj->state->nextstate);
			return;
		}

		// Not stunned? Can hit.
		// Here because stun won't always get the chance to complete due to pinch phase activating, being hit, etc.
		mobj->flags &= ~(MF_SPECIAL|MF_SHOOTABLE);

		if (mobj->health <= mobj->info->damage && mobj->fuse && !(mobj->fuse%TICRATE))
		{
			var1 = 1;
			var2 = 0;
			A_BossScream(mobj);
		}

		// Don't move if we're still in pain!
		if (mobj->flags2 & MF2_FRET)
			return;

		if (mobj->state == &states[mobj->info->raisestate])
		{ // Charging energy
			if (mobj->momx != 0 || mobj->momy != 0) { // Apply the air breaks
				if (abs(mobj->momx)+abs(mobj->momy) < FRACUNIT)
					mobj->momx = mobj->momy = 0;
				else
					P_Thrust(mobj, R_PointToAngle2(0, 0, mobj->momx, mobj->momy), -6*FRACUNIT/8);
			}
			return;
		}

		if (mobj->fuse == 0)
		{
			// It's time to attack! What are we gonna do?!
			switch(mobj->movecount)
			{
			case 0:
			default:
				// Fly up and prepare for an attack!
				// We have to charge up first, so let's go up into the air
				P_SetMobjState(mobj, mobj->info->raisestate);
				if (mobj->floorz >= mobj->target->floorz)
					mobj->watertop = mobj->floorz + 256*FRACUNIT;
				else
					mobj->watertop = mobj->target->floorz + 256*FRACUNIT;
				break;

			case 1: {
				// Okay, we're up? Good, time to gather energy...
				if (mobj->health > mobj->info->damage)
				{ // No more bubble if we're broken (pinch phase)
					mobj_t *shield = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_MSSHIELD_FRONT);
					P_SetTarget(&mobj->tracer, shield);
					P_SetTarget(&shield->target, mobj);
				}
				else
					P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
				mobj->fuse = 4*TICRATE;
				mobj->flags |= MF_PAIN;
				if (mobj->info->attacksound)
					S_StartSound(mobj, mobj->info->attacksound);
				A_FaceTarget(mobj);
				break;
			}

			case 2:
				// We're all charged and ready now! Unleash the fury!!
				if (mobj->health > mobj->info->damage)
				{
					mobj_t *removemobj = mobj->tracer;
					P_SetTarget(&mobj->tracer, mobj->hnext);
					P_RemoveMobj(removemobj);
				}
				if (mobj->health <= mobj->info->damage) {
					// Attack 1: Pinball dash!
					if (mobj->health == 1)
						mobj->movedir = 0;
					else
						mobj->movedir = 2;
					if (mobj->info->seesound)
						S_StartSound(mobj, mobj->info->seesound);
					P_SetMobjState(mobj, mobj->info->seestate);
					if (mobj->movedir == 2)
						mobj->threshold = 16; // bounce 16 times
					else
						mobj->threshold = 32; // bounce 32 times
					mobj->watertop = mobj->target->floorz + 16*FRACUNIT;
					P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);
				} else {
					// Attack 2: Energy shot!
					mobj->movedir = 1;

					if (mobj->health >= 8)
						mobj->extravalue1 = 0;
					else if (mobj->health >= 5)
						mobj->extravalue1 = 2;
					else if (mobj->health >= 4)
						mobj->extravalue1 = 1;
					else
						mobj->extravalue1 = 3;

					switch(mobj->extravalue1) {
					case 0: // shoot once
					case 2: // spread-shot
					default:
						mobj->threshold = 2;
						break;
					case 1: // shoot 3 times
						mobj->threshold = 3*2;
						break;
					case 3: // shoot like a goddamn machinegun
						mobj->threshold = 8*2;
						break;
					}
				}
				break;

			case 3:
				// Return to idle.
				mobj->watertop = mobj->target->floorz + 32*FRACUNIT;
				P_SetMobjState(mobj, mobj->info->spawnstate);
				mobj->flags &= ~MF_PAIN;
				mobj->fuse = 10*TICRATE;
				break;
			}
			mobj->movecount++;
			mobj->movecount %= 4;
			return;
		}

		// Idle AI
		if (mobj->state == &states[mobj->info->spawnstate])
		{
			fixed_t dist;

			// Target the closest player
			P_BossTargetPlayer(mobj, true);

			// Face your target
			angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y); // absolute angle
			angle = (angle-mobj->angle); // relative angle
			if (angle < ANGLE_180)
				mobj->angle += angle/8;
			else
				mobj->angle -= InvAngle(angle)/8;
			//A_FaceTarget(mobj);

			// Check if we're being attacked
			if (!(mobj->target->player->pflags & (PF_JUMPED|PF_SPINNING)
			|| mobj->target->player->powers[pw_tailsfly]
			|| mobj->target->player->powers[pw_invulnerability]
			|| mobj->target->player->powers[pw_super]))
				danger = false;
			if (mobj->target->x+mobj->target->radius+abs(mobj->target->momx*2) < mobj->x-mobj->radius)
				danger = false;
			if (mobj->target->x-mobj->target->radius-abs(mobj->target->momx*2) > mobj->x+mobj->radius)
				danger = false;
			if (mobj->target->y+mobj->target->radius+abs(mobj->target->momy*2) < mobj->y-mobj->radius)
				danger = false;
			if (mobj->target->y-mobj->target->radius-abs(mobj->target->momy*2) > mobj->y+mobj->radius)
				danger = false;
			if (mobj->target->z+mobj->target->height+mobj->target->momz*2 < mobj->z)
				danger = false;
			if (mobj->target->z+mobj->target->momz*2 > mobj->z+mobj->height)
				danger = false;
			if (danger) {
				// An incoming attack is detected! What should we do?!
				// Go into vector form!
				mobj->movedir = ANGLE_11hh - FixedAngle(FixedMul(AngleFixed(ANGLE_11hh), FixedDiv((mobj->info->spawnhealth - mobj->health)<<FRACBITS, (mobj->info->spawnhealth-1)<<FRACBITS)));
				if (P_Random()&1)
					mobj->movedir = InvAngle(mobj->movedir);
				mobj->threshold = 6 + (FixedMul(24<<FRACBITS, FixedDiv((mobj->info->spawnhealth - mobj->health)<<FRACBITS, (mobj->info->spawnhealth-1)<<FRACBITS))>>FRACBITS);
				if (mobj->info->activesound)
					S_StartSound(mobj, mobj->info->activesound);
				if (mobj->info->painchance)
					P_SetMobjState(mobj, mobj->info->painchance);
				return;
			}

			// Move normally: Approach the player using normal thrust and simulated friction.
			dist = P_AproxDistance(mobj->x-mobj->target->x, mobj->y-mobj->target->y);
			P_Thrust(mobj, R_PointToAngle2(0, 0, mobj->momx, mobj->momy), -3*FRACUNIT/8);
			if (dist < 64*FRACUNIT)
				P_Thrust(mobj, mobj->angle, -4*FRACUNIT);
			else if (dist > 180*FRACUNIT)
				P_Thrust(mobj, mobj->angle, FRACUNIT);
			mobj->momz += P_AproxDistance(mobj->momx, mobj->momy)/12; // Move up higher the faster you're going.
		}
	}
}

//
// P_GetClosestAxis
//
// Finds the CLOSEST axis to the source mobj
mobj_t *P_GetClosestAxis(mobj_t *source)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	fixed_t dist1, dist2 = 0;

	// scan the thinkers to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_AXIS)
		{
			if (closestaxis == NULL)
			{
				closestaxis = mo2;
				dist2 = R_PointToDist2(source->x, source->y, mo2->x, mo2->y)-mo2->radius;
			}
			else
			{
				dist1 = R_PointToDist2(source->x, source->y, mo2->x, mo2->y)-mo2->radius;

				if (dist1 < dist2)
				{
					closestaxis = mo2;
					dist2 = dist1;
				}
			}
		}
	}

	if (closestaxis == NULL)
		CONS_Debug(DBG_NIGHTS, "ERROR: No axis points found!\n");

	return closestaxis;
}

static void P_GimmeAxisXYPos(mobj_t *closestaxis, degenmobj_t *mobj)
{
	const angle_t fa = R_PointToAngle2(closestaxis->x, closestaxis->y, mobj->x, mobj->y)>>ANGLETOFINESHIFT;

	mobj->x = closestaxis->x + FixedMul(FINECOSINE(fa),closestaxis->radius);
	mobj->y = closestaxis->y + FixedMul(FINESINE(fa),closestaxis->radius);
}

static void P_MoveHoop(mobj_t *mobj)
{
	const fixed_t fuse = (mobj->fuse*mobj->extravalue2);
	const angle_t fa = mobj->movedir*(FINEANGLES/mobj->extravalue1);
	TVector v;
	TVector *res;
	fixed_t finalx, finaly, finalz;
	fixed_t mthingx, mthingy, mthingz;

	//I_Assert(mobj->target != NULL);
	if (!mobj->target) /// \todo DEBUG ME! Target was P_RemoveMobj'd at some point, and therefore no longer valid!
		return;

	mthingx = mobj->target->x;
	mthingy = mobj->target->y;
	mthingz = mobj->target->z+mobj->target->height/2;

	// Make the sprite travel towards the center of the hoop
	v[0] = FixedMul(FINECOSINE(fa),fuse);
	v[1] = 0;
	v[2] = FixedMul(FINESINE(fa),fuse);
	v[3] = FRACUNIT;

	res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(mobj->target->movedir*FRACUNIT)));
	M_Memcpy(&v, res, sizeof (v));
	res = VectorMatrixMultiply(v, *RotateZMatrix(FixedAngle(mobj->target->movecount*FRACUNIT)));
	M_Memcpy(&v, res, sizeof (v));

	finalx = mthingx + v[0];
	finaly = mthingy + v[1];
	finalz = mthingz + v[2];

	P_UnsetThingPosition(mobj);
	mobj->x = finalx;
	mobj->y = finaly;
	P_SetThingPosition(mobj);
	mobj->z = finalz - mobj->height/2;
}

void P_SpawnHoopOfSomething(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, INT32 number, mobjtype_t type, angle_t rotangle)
{
	mobj_t *mobj;
	INT32 i;
	TVector v;
	TVector *res;
	fixed_t finalx, finaly, finalz;
	mobj_t hoopcenter;
	mobj_t *axis;
	degenmobj_t xypos;
	angle_t degrees, fa, closestangle;

	hoopcenter.x = x;
	hoopcenter.y = y;
	hoopcenter.z = z;

	axis = P_GetClosestAxis(&hoopcenter);

	if (!axis)
	{
		CONS_Debug(DBG_NIGHTS, "You forgot to put axis points in the map!\n");
		return;
	}

	xypos.x = x;
	xypos.y = y;

	P_GimmeAxisXYPos(axis, &xypos);

	x = xypos.x;
	y = xypos.y;

	hoopcenter.z = z - mobjinfo[type].height/2;

	hoopcenter.x = x;
	hoopcenter.y = y;

	closestangle = R_PointToAngle2(x, y, axis->x, axis->y);

	degrees = FINEANGLES/number;

	radius >>= FRACBITS;

	// Create the hoop!
	for (i = 0; i < number; i++)
	{
		fa = (i*degrees);
		v[0] = FixedMul(FINECOSINE(fa),radius);
		v[1] = 0;
		v[2] = FixedMul(FINESINE(fa),radius);
		v[3] = FRACUNIT;

		res = VectorMatrixMultiply(v, *RotateXMatrix(rotangle));
		M_Memcpy(&v, res, sizeof (v));
		res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
		M_Memcpy(&v, res, sizeof (v));

		finalx = x + v[0];
		finaly = y + v[1];
		finalz = z + v[2];

		mobj = P_SpawnMobj(finalx, finaly, finalz, type);
		mobj->z -= mobj->height/2;
	}
}

void P_SpawnParaloop(fixed_t x, fixed_t y, fixed_t z, fixed_t radius, INT32 number, mobjtype_t type, statenum_t nstate, angle_t rotangle, boolean spawncenter)
{
	mobj_t *mobj;
	INT32 i;
	TVector v;
	TVector *res;
	fixed_t finalx, finaly, finalz, dist;
	angle_t degrees, fa, closestangle;
	fixed_t mobjx, mobjy, mobjz;

	degrees = FINEANGLES/number;

	radius = FixedDiv(radius,5*(FRACUNIT/4));

	closestangle = 0;

	// Create the hoop!
	for (i = 0; i < number; i++)
	{
		fa = (i*degrees);
		v[0] = FixedMul(FINECOSINE(fa),radius);
		v[1] = 0;
		v[2] = FixedMul(FINESINE(fa),radius);
		v[3] = FRACUNIT;

		res = VectorMatrixMultiply(v, *RotateXMatrix(rotangle));
		M_Memcpy(&v, res, sizeof (v));
		res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
		M_Memcpy(&v, res, sizeof (v));

		finalx = x + v[0];
		finaly = y + v[1];
		finalz = z + v[2];

		mobj = P_SpawnMobj(finalx, finaly, finalz, type);

		mobj->z -= mobj->height>>1;

		// change angle
		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, x, y);

		// change slope
		dist = P_AproxDistance(P_AproxDistance(x - mobj->x, y - mobj->y), z - mobj->z);

		if (dist < 1)
			dist = 1;

		mobjx = mobj->x;
		mobjy = mobj->y;
		mobjz = mobj->z;

		// set to special state
		if (nstate != S_NULL)
			P_SetMobjState(mobj, nstate);

		mobj->momx = FixedMul(FixedDiv(x - mobjx, dist), 5*FRACUNIT);
		mobj->momy = FixedMul(FixedDiv(y - mobjy, dist), 5*FRACUNIT);
		mobj->momz = FixedMul(FixedDiv(z - mobjz, dist), 5*FRACUNIT);
		mobj->fuse = (radius>>(FRACBITS+2)) + 1;

		if (spawncenter)
		{
			mobj->x = x;
			mobj->y = y;
			mobj->z = z;
		}

		if (mobj->fuse <= 1)
			mobj->fuse = 2;

		mobj->flags |= MF_NOCLIPTHING;
		mobj->flags &= ~MF_SPECIAL;

		if (mobj->fuse > 7)
			mobj->tics = mobj->fuse - 7;
		else
			mobj->tics = 1;
	}
}

//
// P_SetScale
//
// Sets the sprite scaling
//
void P_SetScale(mobj_t *mobj, fixed_t newscale)
{
	player_t *player;
	fixed_t oldscale;

	if (!mobj)
		return;

	oldscale = mobj->scale; //keep for adjusting stuff below

	mobj->scale = newscale;

	mobj->radius = FixedMul(mobj->info->radius, newscale);
	mobj->height = FixedMul(mobj->info->height, newscale);

	player = mobj->player;

	if (player)
	{
		G_GhostAddScale(newscale);
		player->viewheight = FixedMul(FixedDiv(player->viewheight, oldscale), newscale); // Nonono don't calculate viewheight elsewhere, this is the best place for it!
	}
}

// Returns true if no boss with health is in the level.
// Used for Chaos mode

#ifdef CHAOSISNOTDEADYET
static boolean P_BossDoesntExist(void)
{
	thinker_t *th;
	mobj_t *mo2;

	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->flags & MF_BOSS && mo2->health)
			return false;
	}

	// No boss found!
	return true;
}
#endif

void P_Attract(mobj_t *source, mobj_t *dest, boolean nightsgrab) // Home in on your target
{
	fixed_t dist, ndist, speedmul;
	fixed_t tx = dest->x;
	fixed_t ty = dest->y;
	fixed_t tz = dest->z + (dest->height/2); // Aim for center

	if (!dest || dest->health <= 0 || !dest->player || !source->tracer)
		return;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, tx, ty);

	// change slope
	dist = P_AproxDistance(P_AproxDistance(tx - source->x, ty - source->y), tz - source->z);

	if (dist < 1)
		dist = 1;

	if (nightsgrab)
		speedmul = P_AproxDistance(dest->momx, dest->momy) + FixedMul(8*FRACUNIT, source->scale);
	else
		speedmul = P_AproxDistance(dest->momx, dest->momy) + FixedMul(source->info->speed, source->scale);

	source->momx = FixedMul(FixedDiv(tx - source->x, dist), speedmul);
	source->momy = FixedMul(FixedDiv(ty - source->y, dist), speedmul);
	source->momz = FixedMul(FixedDiv(tz - source->z, dist), speedmul);

	// Instead of just unsetting NOCLIP like an idiot, let's check the distance to our target.
	ndist = P_AproxDistance(P_AproxDistance(tx - (source->x+source->momx),
	                                        ty - (source->y+source->momy)),
	                                        tz - (source->z+source->momz));

	if (ndist > dist) // gone past our target
	{
		// place us on top of them then.
		source->momx = source->momy = source->momz = 0;
		P_UnsetThingPosition(source);
		source->x = tx;
		source->y = ty;
		source->z = tz;
		P_SetThingPosition(source);
	}
}

static void P_NightsItemChase(mobj_t *thing)
{
	if (!thing->tracer)
	{
		P_SetTarget(&thing->tracer, NULL);
		thing->flags2 &= ~MF2_NIGHTSPULL;
		return;
	}

	if (!thing->tracer->player)
		return;

	P_Attract(thing, thing->tracer, true);
}

static boolean P_ShieldLook(mobj_t *thing, shieldtype_t shield)
{
	fixed_t destx, desty;

	if (!thing->target || thing->target->health <= 0 || !thing->target->player
		|| (thing->target->player->powers[pw_shield] & SH_NOSTACK) == SH_NONE || thing->target->player->powers[pw_super]
		|| thing->target->player->powers[pw_invulnerability] > 1)
	{
		P_RemoveMobj(thing);
		return false;
	}

	// TODO: Make an MT_SHIELDORB which changes color/states to always match the appropriate shield,
	// instead of having completely seperate mobjtypes.
	if (shield != SH_FORCE)
	{ // Regular shields check for themselves only
		if ((shieldtype_t)(thing->target->player->powers[pw_shield] & SH_NOSTACK) != shield)
		{
			P_RemoveMobj(thing);
			return false;
		}
	}
	else if (!(thing->target->player->powers[pw_shield] & SH_FORCE))
	{ // Force shields check for any force shield
		P_RemoveMobj(thing);
		return false;
	}

	if (!splitscreen && rendermode != render_soft)
	{
		angle_t viewingangle;

		if (players[displayplayer].awayviewtics)
			viewingangle = R_PointToAngle2(thing->target->x, thing->target->y, players[displayplayer].awayviewmobj->x, players[displayplayer].awayviewmobj->y);
		else if (!camera.chase && players[displayplayer].mo)
			viewingangle = R_PointToAngle2(thing->target->x, thing->target->y, players[displayplayer].mo->x, players[displayplayer].mo->y);
		else
			viewingangle = R_PointToAngle2(thing->target->x, thing->target->y, camera.x, camera.y);

		destx = thing->target->x + P_ReturnThrustX(thing->target, viewingangle, FixedMul(FRACUNIT, thing->scale));
		desty = thing->target->y + P_ReturnThrustY(thing->target, viewingangle, FixedMul(FRACUNIT, thing->scale));
	}
	else
	{
		destx = thing->target->x;
		desty = thing->target->y;
	}

	if (shield == SH_FORCE && thing->movecount != (thing->target->player->powers[pw_shield] & 0xFF))
	{
		thing->movecount = (thing->target->player->powers[pw_shield] & 0xFF);
		if (thing->movecount < 1)
		{
			if (thing->info->painstate)
				P_SetMobjState(thing,thing->info->painstate);
			else
				thing->flags2 |= MF2_SHADOW;
		}
		else
		{
			if (thing->info->painstate)
				P_SetMobjState(thing,thing->info->spawnstate);
			else
				thing->flags2 &= ~MF2_SHADOW;
		}
	}

	thing->flags |= MF_NOCLIPHEIGHT;
	thing->eflags = (thing->eflags & ~MFE_VERTICALFLIP)|(thing->target->eflags & MFE_VERTICALFLIP);

	P_SetScale(thing, thing->target->scale);
	P_UnsetThingPosition(thing);
	thing->x = destx;
	thing->y = desty;
	if (thing->eflags & MFE_VERTICALFLIP)
		thing->z = thing->target->z + thing->target->height - thing->height + FixedDiv(P_GetPlayerHeight(thing->target->player) - thing->target->height, 3*FRACUNIT) - FixedMul(2*FRACUNIT, thing->target->scale);
	else
		thing->z = thing->target->z - FixedDiv(P_GetPlayerHeight(thing->target->player) - thing->target->height, 3*FRACUNIT) + FixedMul(2*FRACUNIT, thing->target->scale);
	P_SetThingPosition(thing);
	P_CheckPosition(thing, thing->x, thing->y);

	if (P_MobjWasRemoved(thing))
		return false;

//	if (thing->z < thing->floorz)
//		thing->z = thing->floorz;

	return true;
}

mobj_t *shields[MAXPLAYERS*2];
INT32 numshields = 0;

void P_RunShields(void)
{
	INT32 i;
	mobj_t *mo, *next;
	fixed_t destx,desty,zoffs;

	// run shields
	for (i = 0; i < numshields; i++)
	{
		P_ShieldLook(shields[i], shields[i]->info->speed);
		P_SetTarget(&shields[i], NULL);
	}
	numshields = 0;

	// run overlays
	next = NULL;
	for (mo = overlaycap; mo; mo = next)
	{
		I_Assert(!P_MobjWasRemoved(mo));

		// grab next in chain, then unset the chain target
		next = mo->hnext;
		P_SetTarget(&mo->hnext, NULL);

		if (!mo->target)
			continue;
		if (!splitscreen /*&& rendermode != render_soft*/)
		{
			angle_t viewingangle;

			if (players[displayplayer].awayviewtics)
				viewingangle = R_PointToAngle2(mo->target->x, mo->target->y, players[displayplayer].awayviewmobj->x, players[displayplayer].awayviewmobj->y);
			else if (!camera.chase && players[displayplayer].mo)
				viewingangle = R_PointToAngle2(mo->target->x, mo->target->y, players[displayplayer].mo->x, players[displayplayer].mo->y);
			else
				viewingangle = R_PointToAngle2(mo->target->x, mo->target->y, camera.x, camera.y);

			if (mo->state->var1)
				viewingangle += ANGLE_180;
			destx = mo->target->x + P_ReturnThrustX(mo->target, viewingangle, FixedMul(FRACUNIT/4, mo->scale));
			desty = mo->target->y + P_ReturnThrustY(mo->target, viewingangle, FixedMul(FRACUNIT/4, mo->scale));
		}
		else
		{
			destx = mo->target->x;
			desty = mo->target->y;
		}

		mo->eflags = (mo->eflags & ~MFE_VERTICALFLIP) | (mo->target->eflags & MFE_VERTICALFLIP);
		mo->scale = mo->destscale = mo->target->scale;
		zoffs = FixedMul(((signed)mo->state->var2)*FRACUNIT, mo->scale);
		mo->angle = mo->target->angle;

		P_UnsetThingPosition(mo);
		mo->x = destx;
		mo->y = desty;
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = (mo->target->z + mo->target->height - mo->height) - zoffs;
		else
			mo->z = mo->target->z + zoffs;
		if (mo->state->var1)
			P_SetUnderlayPosition(mo);
		else
			P_SetThingPosition(mo);
		P_CheckPosition(mo, mo->x, mo->y);
	}
	P_SetTarget(&overlaycap, NULL);
}

static boolean P_AddShield(mobj_t *thing)
{
	shieldtype_t shield = thing->info->speed;

	if (!thing->target || thing->target->health <= 0 || !thing->target->player
		|| (thing->target->player->powers[pw_shield] & SH_NOSTACK) == SH_NONE || thing->target->player->powers[pw_super]
		|| thing->target->player->powers[pw_invulnerability] > 1)
	{
		P_RemoveMobj(thing);
		return false;
	}

	if (shield != SH_FORCE)
	{ // Regular shields check for themselves only
		if ((shieldtype_t)(thing->target->player->powers[pw_shield] & SH_NOSTACK) != shield)
		{
			P_RemoveMobj(thing);
			return false;
		}
	}
	else if (!(thing->target->player->powers[pw_shield] & SH_FORCE))
	{ // Force shields check for any force shield
		P_RemoveMobj(thing);
		return false;
	}

	// Queue has been hit... why?!?
	if (numshields >= MAXPLAYERS*2)
		return P_ShieldLook(thing, thing->info->speed);

	P_SetTarget(&shields[numshields++], thing);
	return true;
}

// Called only when MT_OVERLAY thinks.
static void P_AddOverlay(mobj_t *thing)
{
	I_Assert(thing != NULL);

	if (overlaycap == NULL)
		P_SetTarget(&overlaycap, thing);
	else {
		mobj_t *mo;
		for (mo = overlaycap; mo && mo->hnext; mo = mo->hnext)
			;

		I_Assert(mo != NULL);
		I_Assert(mo->hnext == NULL);

		P_SetTarget(&mo->hnext, thing);
	}
	P_SetTarget(&thing->hnext, NULL);
}

// Called only when MT_OVERLAY (or anything else in the overlaycap list) is removed.
// Keeps the hnext list from corrupting.
static void P_RemoveOverlay(mobj_t *thing)
{
	mobj_t *mo;
	for (mo = overlaycap; mo; mo = mo->hnext)
		if (mo->hnext == thing)
		{
			P_SetTarget(&mo->hnext, thing->hnext);
			P_SetTarget(&thing->hnext, NULL);
			return;
		}
}

void A_BossDeath(mobj_t *mo);
// AI for the Koopa boss.
static void P_KoopaThinker(mobj_t *koopa)
{
	P_MobjCheckWater(koopa);

	if (koopa->watertop > koopa->z + koopa->height + FixedMul(128*FRACUNIT, koopa->scale) && koopa->health > 0)
	{
		A_BossDeath(koopa);
		P_RemoveMobj(koopa);
		return;
	}

	// Koopa moves ONLY on the X axis!
	if (koopa->threshold > 0)
	{
		koopa->threshold--;
		koopa->momx = FixedMul(FRACUNIT, koopa->scale);

		if (!koopa->threshold)
			koopa->threshold = -TICRATE*2;
	}
	else if (koopa->threshold < 0)
	{
		koopa->threshold++;
		koopa->momx = FixedMul(-FRACUNIT, koopa->scale);

		if (!koopa->threshold)
			koopa->threshold = TICRATE*2;
	}
	else
		koopa->threshold = TICRATE*2;

	P_XYMovement(koopa);

	if (P_Random() < 8 && koopa->z <= koopa->floorz)
		koopa->momz = FixedMul(5*FRACUNIT, koopa->scale);

	if (koopa->z > koopa->floorz)
		koopa->momz += FixedMul(FRACUNIT/4, koopa->scale);

	if (P_Random() < 4)
	{
		mobj_t *flame;
		flame = P_SpawnMobj(koopa->x - koopa->radius + FixedMul(5*FRACUNIT, koopa->scale), koopa->y, koopa->z + (P_Random()<<(FRACBITS-2)), MT_KOOPAFLAME);
		flame->momx = -FixedMul(flame->info->speed, flame->scale);
		S_StartSound(flame, sfx_koopfr);
	}
	else if (P_Random() > 250)
	{
		mobj_t *hammer;
		hammer = P_SpawnMobj(koopa->x - koopa->radius, koopa->y, koopa->z + koopa->height, MT_HAMMER);
		hammer->momx = FixedMul(-5*FRACUNIT, hammer->scale);
		hammer->momz = FixedMul(7*FRACUNIT, hammer->scale);
	}
}

//
// P_MobjThinker
//
void P_MobjThinker(mobj_t *mobj)
{
	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));

	if (mobj->flags & MF_NOTHINK)
		return;

	// Remove dead target/tracer.
	if (mobj->target && P_MobjWasRemoved(mobj->target))
		P_SetTarget(&mobj->target, NULL);
	if (mobj->tracer && P_MobjWasRemoved(mobj->tracer))
		P_SetTarget(&mobj->tracer, NULL);

	mobj->flags2 &= ~MF2_PUSHED;

	// 970 allows ANY mobj to trigger a linedef exec
	if (mobj->subsector && GETSECSPECIAL(mobj->subsector->sector->special, 2) == 8)
	{
		sector_t *sec2;

		sec2 = P_ThingOnSpecial3DFloor(mobj);
		if (sec2 && GETSECSPECIAL(sec2->special, 2) == 1)
			P_LinedefExecute(sec2->tag, mobj, sec2);
	}

	// Slowly scale up/down to reach your destscale.
	if (mobj->scale != mobj->destscale)
	{
		fixed_t oldheight = mobj->height;
		UINT8 correctionType = 0; // Don't correct Z position, just gain height

		if (mobj->z > mobj->floorz && mobj->z + mobj->height < mobj->ceilingz
		&& mobj->type != MT_EGGMOBILE_FIRE)
			correctionType = 1; // Correct Z position by centering
		else if (mobj->eflags & MFE_VERTICALFLIP)
			correctionType = 2; // Correct Z position by moving down

		if (abs(mobj->scale - mobj->destscale) < mobj->scalespeed)
			P_SetScale(mobj, mobj->destscale);
		else if (mobj->scale < mobj->destscale)
			P_SetScale(mobj, mobj->scale + mobj->scalespeed);
		else if (mobj->scale > mobj->destscale)
			P_SetScale(mobj, mobj->scale - mobj->scalespeed);

		if (correctionType == 1)
			mobj->z -= (mobj->height - oldheight)/2;
		else if (correctionType == 2)
			mobj->z -= mobj->height - oldheight;

		if (mobj->scale == mobj->destscale)
			/// \todo Lua hook for "reached destscale"?
			switch(mobj->type)
			{
			case MT_EGGMOBILE_FIRE:
				mobj->destscale = FRACUNIT;
				mobj->scalespeed = FRACUNIT>>4;
				break;
			default:
				break;
			}
	}

	if (mobj->type == MT_GHOST && mobj->fuse > 0 // Not guaranteed to be MF_SCENERY or not MF_SCENERY!
	&& (signed)(mobj->frame >> FF_TRANSSHIFT) < (NUMTRANSMAPS-1) - mobj->fuse / 2)
		// fade out when nearing the end of fuse...
		mobj->frame = (mobj->frame & ~FF_TRANSMASK) | (((NUMTRANSMAPS-1) - mobj->fuse / 2) << FF_TRANSSHIFT);

	// Special thinker for scenery objects
	if (mobj->flags & MF_SCENERY)
	{
#ifdef HAVE_BLUA
		if (LUAh_MobjThinker(mobj))
			return;
		if (P_MobjWasRemoved(mobj))
			return;
#endif
		switch (mobj->type)
		{
			case MT_HOOP:
				if (mobj->fuse > 1)
					P_MoveHoop(mobj);
				else if (mobj->fuse == 1)
					mobj->movecount = 1;

				if (mobj->movecount)
				{
					mobj->fuse++;

					if (mobj->fuse > 32)
					{
						// Don't kill the hoop center. For the sake of respawning.
						//if (mobj->target)
						//	P_RemoveMobj(mobj->target);

						P_RemoveMobj(mobj);
					}
				}
				else
					mobj->fuse--;
				return;
			case MT_NIGHTSPARKLE:
				if (mobj->tics != -1)
				{
					mobj->tics--;

					// you can cycle through multiple states in a tic
					if (!mobj->tics)
						if (!P_SetMobjState(mobj, mobj->state->nextstate))
							return; // freed itself
				}

				P_UnsetThingPosition(mobj);
				mobj->x += mobj->momx;
				mobj->y += mobj->momy;
				mobj->z += mobj->momz;
				P_SetThingPosition(mobj);
				return;
			case MT_NIGHTSLOOPHELPER:
				if (--mobj->tics <= 0)
					P_RemoveMobj(mobj);

				// Don't touch my fuse!
				return;
			case MT_OVERLAY:
				if (!mobj->target)
				{
					P_RemoveMobj(mobj);
					return;
				}
				else
					P_AddOverlay(mobj);
				break;
			case MT_BLACKORB:
			case MT_WHITEORB:
			case MT_GREENORB:
			case MT_YELLOWORB:
			case MT_BLUEORB:
			case MT_PITYORB:
				if (!P_AddShield(mobj))
					return;
				break;
			case MT_WATERDROP:
				P_SceneryCheckWater(mobj);
				if ((mobj->z <= mobj->floorz || mobj->z <= mobj->watertop)
					&& mobj->health > 0)
				{
					mobj->health = 0;
					P_SetMobjState(mobj, mobj->info->deathstate);
					S_StartSound(mobj, mobj->info->deathsound+P_RandomKey(mobj->info->mass));
					return;
				}
				break;
			case MT_BUBBLES:
				P_SceneryCheckWater(mobj);
				break;
			case MT_SMALLBUBBLE:
			case MT_MEDIUMBUBBLE:
			case MT_EXTRALARGEBUBBLE:	// start bubble dissipate
				P_SceneryCheckWater(mobj);
				if (P_MobjWasRemoved(mobj)) // bubble was removed by not being in water
					return;
				if (!(mobj->eflags & MFE_UNDERWATER)
					|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height >= mobj->ceilingz)
					|| (mobj->eflags & MFE_VERTICALFLIP && mobj->z <= mobj->floorz)
					|| (P_CheckDeathPitCollide(mobj))
					|| --mobj->fuse <= 0) // Bubbles eventually dissipate if they can't reach the surface.
				{
					// no playing sound: no point; the object is being removed
					P_RemoveMobj(mobj);
					return;
				}
				break;
			case MT_DROWNNUMBERS:
				if (!mobj->target)
				{
					P_RemoveMobj(mobj);
					return;
				}
				if (!mobj->target->player || !(mobj->target->player->powers[pw_underwater] || mobj->target->player->powers[pw_spacetime]))
				{
					P_RemoveMobj(mobj);
					return;
				}
				mobj->x = mobj->target->x;
				mobj->y = mobj->target->y;

				mobj->destscale = mobj->target->destscale;
				P_SetScale(mobj, mobj->target->scale);

				if (mobj->target->eflags & MFE_VERTICALFLIP)
				{
					mobj->z = mobj->target->z - FixedMul(16*FRACUNIT, mobj->target->scale) - mobj->height;
					if (mobj->target->player->pflags & PF_FLIPCAM)
						mobj->eflags |= MFE_VERTICALFLIP;
				}
				else
					mobj->z = mobj->target->z + (mobj->target->height) + FixedMul(8*FRACUNIT, mobj->target->scale); // Adjust height for height changes

				if (mobj->threshold <= 35)
					mobj->flags2 |= MF2_DONTDRAW;
				else
					mobj->flags2 &= ~MF2_DONTDRAW;
				if (mobj->threshold <= 30)
					mobj->threshold = 40;
				mobj->threshold--;
				break;
			case MT_FLAMEJET:
				if ((mobj->flags2 & MF2_FIRING) && (leveltime & 3) == 0)
				{
					mobj_t *flame;
					fixed_t strength;

					// Wave the flames back and forth. Reactiontime determines which direction it's going.
					if (mobj->fuse <= -16)
						mobj->reactiontime = 1;
					else if (mobj->fuse >= 16)
						mobj->reactiontime = 0;

					if (mobj->reactiontime)
						mobj->fuse += 2;
					else
						mobj->fuse -= 2;

					flame = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_FLAMEJETFLAME);

					flame->angle = mobj->angle;

					if (mobj->flags & MF_AMBUSH) // Wave up and down instead of side-to-side
						flame->momz = mobj->fuse << (FRACBITS-2);
					else
						flame->angle += FixedAngle(mobj->fuse*FRACUNIT);

					strength = 20*FRACUNIT;
					strength -= ((20*FRACUNIT)/16)*mobj->movedir;

					P_InstaThrust(flame, flame->angle, strength);
					S_StartSound(flame, sfx_fire);
				}
				break;
			case MT_VERTICALFLAMEJET:
				if ((mobj->flags2 & MF2_FIRING) && (leveltime & 3) == 0)
				{
					mobj_t *flame;
					fixed_t strength;

					// Wave the flames back and forth. Reactiontime determines which direction it's going.
					if (mobj->fuse <= -16)
						mobj->reactiontime = 1;
					else if (mobj->fuse >= 16)
						mobj->reactiontime = 0;

					if (mobj->reactiontime)
						mobj->fuse++;
					else
						mobj->fuse--;

					flame = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_FLAMEJETFLAME);

					strength = 20*FRACUNIT;
					strength -= ((20*FRACUNIT)/16)*mobj->movedir;

					// If deaf'd, the object spawns on the ceiling.
					if (mobj->flags & MF_AMBUSH)
					{
						mobj->z = mobj->ceilingz-mobj->height;
						flame->momz = -strength;
					}
					else
						flame->momz = strength;
					P_InstaThrust(flame, mobj->angle, FixedDiv(mobj->fuse*FRACUNIT,3*FRACUNIT));
					S_StartSound(flame, sfx_fire);
				}
				break;
			case MT_SEED:
				mobj->momz = mobj->info->speed;
				break;
			default:
				if (mobj->fuse)
				{ // Scenery object fuse! Very basic!
					mobj->fuse--;
					if (!mobj->fuse)
					{
#ifdef HAVE_BLUA
						if (!LUAh_MobjFuse(mobj))
#endif
						P_RemoveMobj(mobj);
						return;
					}
				}
				break;
		}

		P_SceneryThinker(mobj);
		return;
	}

#ifdef HAVE_BLUA
	// Check for a Lua thinker first
	if (!mobj->player && LUAh_MobjThinker(mobj))
		return;
	else if (mobj->player && !mobj->player->spectator)
		LUAh_MobjThinker(mobj);
	if (P_MobjWasRemoved(mobj))
		return;
#endif
	// if it's pushable, or if it would be pushable other than temporary disablement, use the
	// separate thinker
	if (mobj->flags & MF_PUSHABLE || (mobj->info->flags & MF_PUSHABLE && mobj->fuse))
	{
		P_MobjCheckWater(mobj);
		P_PushableThinker(mobj);

		// Extinguish fire objects in water. (Yes, it's extraordinarily rare to have a pushable flame object, but Brak uses such a case.)
		if (mobj->flags & MF_FIRE && mobj->type != MT_PUMA && mobj->type != MT_FIREBALL
			&& (mobj->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
		{
			P_KillMobj(mobj, NULL, NULL);
			return;
		}
	}
	else if (mobj->flags & MF_BOSS)
	{
#ifdef HAVE_BLUA
		if (LUAh_BossThinker(mobj))
		{
			if (P_MobjWasRemoved(mobj))
				return;
		}
		else if (P_MobjWasRemoved(mobj))
			return;
		else
#endif
		switch (mobj->type)
		{
			case MT_EGGMOBILE:
				if (mobj->health < mobj->info->damage+1 && leveltime & 1 && mobj->health > 0)
					P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_SMOKE);
				if (mobj->flags2 & MF2_SKULLFLY)
#if 1
					P_SpawnGhostMobj(mobj);
#else
				{
					mobj_t *spawnmobj;
					spawnmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->info->painchance);
					P_SetTarget(&spawnmobj->target, mobj);
					spawnmobj->color = SKINCOLOR_GREY;
				}
#endif
				P_Boss1Thinker(mobj);
				break;
			case MT_EGGMOBILE2:
				P_Boss2Thinker(mobj);
				break;
			case MT_EGGMOBILE3:
				P_Boss3Thinker(mobj);
				break;
			case MT_EGGMOBILE4:
				P_Boss4Thinker(mobj);
				break;
			case MT_BLACKEGGMAN:
				P_Boss7Thinker(mobj);
				break;
			case MT_METALSONIC_BATTLE:
				P_Boss9Thinker(mobj);
				break;
			default: // Generic SOC-made boss
				if (mobj->flags2 & MF2_SKULLFLY)
					P_SpawnGhostMobj(mobj);
				P_GenericBossThinker(mobj);
				break;
		}
		if (mobj->flags2 & MF2_BOSSFLEE)
			P_InstaThrust(mobj, mobj->angle, FixedMul(12*FRACUNIT, mobj->scale));
	}
	else if (mobj->health <= 0) // Dead things think differently than the living.
		switch (mobj->type)
		{
#ifdef BLUE_SPHERES
		case MT_BLUEBALL:
			if ((mobj->tics>>2)+1 > 0 && (mobj->tics>>2)+1 <= tr_trans60) // tr_trans50 through tr_trans90, shifting once every second frame
				mobj->frame = (NUMTRANSMAPS-((mobj->tics>>2)+1))<<FF_TRANSSHIFT;
			else // tr_trans60 otherwise
				mobj->frame = tr_trans60<<FF_TRANSSHIFT;
			break;
#endif
		case MT_EGGCAPSULE:
			if (mobj->z <= mobj->floorz)
			{
				P_RemoveMobj(mobj);
				return;
			}
			break;
		case MT_EGGTRAP: // Egg Capsule animal release
			if (mobj->fuse > 0 && mobj->fuse < 2*TICRATE-(TICRATE/7)
				&& (mobj->fuse & 3))
			{
				INT32 i,j;
				fixed_t x,y,z;
				fixed_t ns;
				mobj_t *mo2;

				i = P_Random();
				z = mobj->subsector->sector->floorheight + ((P_Random()&63)*FRACUNIT);
				for (j = 0; j < 2; j++)
				{
					const angle_t fa = (P_Random()*FINEANGLES/16) & FINEMASK;
					ns = 64 * FRACUNIT;
					x = mobj->x + FixedMul(FINESINE(fa),ns);
					y = mobj->y + FixedMul(FINECOSINE(fa),ns);

					mo2 = P_SpawnMobj(x, y, z, MT_EXPLODE);
					ns = 4 * FRACUNIT;
					mo2->momx = FixedMul(FINESINE(fa),ns);
					mo2->momy = FixedMul(FINECOSINE(fa),ns);

					i = P_Random();

					if (i % 5 == 0)
						P_SpawnMobj(x, y, z, MT_CHICKEN);
					else if (i % 4 == 0)
						P_SpawnMobj(x, y, z, MT_COW);
					else if (i % 3 == 0)
					{
						P_SpawnMobj(x, y, z, MT_BIRD);
						S_StartSound(mo2, mobj->info->deathsound);
					}
					else if ((i & 1) == 0)
						P_SpawnMobj(x, y, z, MT_BUNNY);
					else
						P_SpawnMobj(x, y, z, MT_MOUSE);
				}

				mobj->fuse--;
			}
			break;
		case MT_PLAYER:
			/// \todo Have the player's dead body completely finish its animation even if they've already respawned.
			if (!(mobj->flags2 & MF2_DONTDRAW))
			{
				if (mobj->player && mobj->player->deadtimer > 3*TICRATE)
				{ // Go away.
					/// \todo Actually go ahead and remove mobj completely, and fix any bugs and crashes doing this creates. Chasecam should stop moving, and F12 should never return to it.
					mobj->momz = 0;
					mobj->flags2 |= MF2_DONTDRAW;
				}
				else // Apply gravity to fall downwards.
					P_SetObjectMomZ(mobj, -2*FRACUNIT/3, true);
			}
			break;
		default:
			break;
		}
	else switch (mobj->type)
	{
		case MT_ROCKCRUMBLE1:
		case MT_ROCKCRUMBLE2:
		case MT_ROCKCRUMBLE3:
		case MT_ROCKCRUMBLE4:
		case MT_ROCKCRUMBLE5:
		case MT_ROCKCRUMBLE6:
		case MT_ROCKCRUMBLE7:
		case MT_ROCKCRUMBLE8:
		case MT_ROCKCRUMBLE9:
		case MT_ROCKCRUMBLE10:
		case MT_ROCKCRUMBLE11:
		case MT_ROCKCRUMBLE12:
		case MT_ROCKCRUMBLE13:
		case MT_ROCKCRUMBLE14:
		case MT_ROCKCRUMBLE15:
		case MT_ROCKCRUMBLE16:
			if (mobj->z <= P_FloorzAtPos(mobj->x, mobj->y, mobj->z, mobj->height)
				&& mobj->state != &states[mobj->info->deathstate])
			{
				P_SetMobjState(mobj, mobj->info->deathstate);
				return;
			}
			break;
		case MT_EMERALDSPAWN:
			if (mobj->threshold)
			{
				mobj->threshold--;

				if (!mobj->threshold && !mobj->target && mobj->reactiontime)
				{
					mobj_t *emerald = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->reactiontime);
					emerald->threshold = 42;
					P_SetTarget(&mobj->target, emerald);
					P_SetTarget(&emerald->target, mobj);
				}
			}
			break;
		case MT_AQUABUZZ:
		case MT_BIGAIRMINE:
			{
				if (mobj->tracer && mobj->tracer->player && mobj->tracer->health > 0
					&& P_AproxDistance(P_AproxDistance(mobj->tracer->x - mobj->x, mobj->tracer->y - mobj->y), mobj->tracer->z - mobj->z) <= mobj->radius * 16)
				{
					// Home in on the target.
					P_HomingAttack(mobj, mobj->tracer);

					if (mobj->z < mobj->floorz)
						mobj->z = mobj->floorz;

					if (leveltime % mobj->info->painchance == 0)
						S_StartSound(mobj, mobj->info->activesound);
				}
				else
				{
					// Try to find a player
					P_LookForPlayers(mobj, true, true, mobj->radius * 16);
					mobj->momx >>= 1;
					mobj->momy >>= 1;
					mobj->momz >>= 1;
				}
			}
			break;
		case MT_BIGMINE:
			{
				if (mobj->tracer && mobj->tracer->player && mobj->tracer->health > 0
					&& P_AproxDistance(P_AproxDistance(mobj->tracer->x - mobj->x, mobj->tracer->y - mobj->y), mobj->tracer->z - mobj->z) <= mobj->radius * 16)
				{
					P_MobjCheckWater(mobj);

					// Home in on the target.
					P_HomingAttack(mobj, mobj->tracer);

					// Don't let it go out of water
					if (mobj->z + mobj->height > mobj->watertop)
						mobj->z = mobj->watertop - mobj->height;

					if (mobj->z < mobj->floorz)
						mobj->z = mobj->floorz;

					if (leveltime % mobj->info->painchance == 0)
						S_StartSound(mobj, mobj->info->activesound);
				}
				else
				{
					// Try to find a player
					P_LookForPlayers(mobj, true, true, mobj->radius * 16);
					mobj->momx >>= 1;
					mobj->momy >>= 1;
					mobj->momz >>= 1;
				}
			}
			break;
		case MT_SPINMACEPOINT:
			if (leveltime & 1)
			{
				if (mobj->lastlook > mobj->movecount)
					mobj->lastlook--;
/*
				if (mobj->threshold > mobj->movefactor)
					mobj->threshold -= FRACUNIT;
				else if (mobj->threshold < mobj->movefactor)
					mobj->threshold += FRACUNIT;*/
			}
			break;
		case MT_EGGCAPSULE:
			if (!mobj->reactiontime)
			{
				// Target nearest player on your mare.
				// (You can make it float up/down by adding MF_FLOAT,
				//  but beware level design pitfalls.)
				fixed_t shortest = 1024*FRACUNIT;
				INT32 i;
				P_SetTarget(&mobj->target, NULL);
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].mo
					&& players[i].mare == mobj->threshold && players[i].health > 1)
					{
						fixed_t dist = P_AproxDistance(players[i].mo->x - mobj->x, players[i].mo->y - mobj->y);
						if (dist < shortest)
						{
							P_SetTarget(&mobj->target, players[i].mo);
							shortest = dist;
						}
					}
			}
			break;
		case MT_EGGMOBILE2_POGO:
			if (!mobj->target
			|| !mobj->target->health
			|| mobj->target->state == &states[mobj->target->info->spawnstate]
			|| mobj->target->state == &states[mobj->target->info->raisestate])
			{
				P_RemoveMobj(mobj);
				return;
			}
			P_TeleportMove(mobj, mobj->target->x, mobj->target->y, mobj->target->z - mobj->height);
			break;
		case MT_HAMMER:
			if (mobj->z <= mobj->floorz)
			{
				P_RemoveMobj(mobj);
				return;
			}
			break;
		case MT_KOOPA:
			P_KoopaThinker(mobj);
			break;
		case MT_REDRING:
			if (((mobj->z < mobj->floorz) || (mobj->z + mobj->height > mobj->ceilingz))
				&& mobj->flags & MF_MISSILE)
			{
				P_ExplodeMissile(mobj);
				return;
			}
			break;
		case MT_BOSSFLYPOINT:
			return;
		case MT_NIGHTSCORE:
			mobj->color = (UINT8)(leveltime % SKINCOLOR_WHITE);
			break;
		case MT_JETFUME1:
			{
				fixed_t jetx, jety;

				if (!mobj->target // if you have no target
				|| (!(mobj->target->flags & MF_BOSS) && mobj->target->health <= 0)) // or your target isn't a boss and it's popped now
				{ // then remove yourself as well!
					P_RemoveMobj(mobj);
					return;
				}

#ifdef CHAOSISNOTDEADYET
				if (gametype == GT_CHAOS && mobj->target->health <= 0)
				{
					P_RemoveMobj(mobj);
					return;
				}
#endif

				jetx = mobj->target->x + P_ReturnThrustX(mobj->target, mobj->target->angle, FixedMul(-64*FRACUNIT, mobj->target->scale));
				jety = mobj->target->y + P_ReturnThrustY(mobj->target, mobj->target->angle, FixedMul(-64*FRACUNIT, mobj->target->scale));

				if (mobj->fuse == 56) // First one
				{
					P_UnsetThingPosition(mobj);
					mobj->x = jetx;
					mobj->y = jety;
					if (mobj->target->eflags & MFE_VERTICALFLIP)
						mobj->z = mobj->target->z + mobj->target->height - mobj->height - FixedMul(38*FRACUNIT, mobj->target->scale);
					else
						mobj->z = mobj->target->z + FixedMul(38*FRACUNIT, mobj->target->scale);
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				else if (mobj->fuse == 57)
				{
					P_UnsetThingPosition(mobj);
					mobj->x = jetx + P_ReturnThrustX(mobj->target, mobj->target->angle-ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
					mobj->y = jety + P_ReturnThrustY(mobj->target, mobj->target->angle-ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
					if (mobj->target->eflags & MFE_VERTICALFLIP)
						mobj->z = mobj->target->z + mobj->target->height - mobj->height - FixedMul(12*FRACUNIT, mobj->target->scale);
					else
						mobj->z = mobj->target->z + FixedMul(12*FRACUNIT, mobj->target->scale);
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				else if (mobj->fuse == 58)
				{
					P_UnsetThingPosition(mobj);
					mobj->x = jetx + P_ReturnThrustX(mobj->target, mobj->target->angle+ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
					mobj->y = jety + P_ReturnThrustY(mobj->target, mobj->target->angle+ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
					if (mobj->target->eflags & MFE_VERTICALFLIP)
						mobj->z = mobj->target->z + mobj->target->height - mobj->height - FixedMul(12*FRACUNIT, mobj->target->scale);
					else
						mobj->z = mobj->target->z + FixedMul(12*FRACUNIT, mobj->target->scale);
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				else if (mobj->fuse == 59)
				{
					jetx = mobj->target->x + P_ReturnThrustX(mobj->target, mobj->target->angle, -mobj->target->radius);
					jety = mobj->target->y + P_ReturnThrustY(mobj->target, mobj->target->angle, -mobj->target->radius);
					P_UnsetThingPosition(mobj);
					mobj->x = jetx;
					mobj->y = jety;
					if (mobj->target->eflags & MFE_VERTICALFLIP)
						mobj->z = mobj->target->z + mobj->target->height/2 + mobj->height/2;
					else
						mobj->z = mobj->target->z + mobj->target->height/2 - mobj->height/2;
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_SetThingPosition(mobj);
				}
				mobj->fuse++;
			}
			break;
		case MT_PROPELLER:
			{
				fixed_t jetx, jety;

				if (!mobj->target // if you have no target
				|| (!(mobj->target->flags & MF_BOSS) && mobj->target->health <= 0)) // or your target isn't a boss and it's popped now
				{ // then remove yourself as well!
					P_RemoveMobj(mobj);
					return;
				}

#ifdef CHAOSISNOTDEADYET
				if (gametype == GT_CHAOS && mobj->target->health <= 0)
				{
					P_RemoveMobj(mobj);
					return;
				}
#endif

				jetx = mobj->target->x + P_ReturnThrustX(mobj->target, mobj->target->angle, FixedMul(-60*FRACUNIT, mobj->target->scale));
				jety = mobj->target->y + P_ReturnThrustY(mobj->target, mobj->target->angle, FixedMul(-60*FRACUNIT, mobj->target->scale));

				P_UnsetThingPosition(mobj);
				mobj->x = jetx;
				mobj->y = jety;
				mobj->z = mobj->target->z + FixedMul(17*FRACUNIT, mobj->target->scale);
				mobj->angle = mobj->target->angle - ANGLE_180;
				mobj->floorz = mobj->z;
				mobj->ceilingz = mobj->z+mobj->height;
				P_SetThingPosition(mobj);
			}
			break;
		case MT_JETFLAME:
			{
				if (!mobj->target // if you have no target
				|| (!(mobj->target->flags & MF_BOSS) && mobj->target->health <= 0)) // or your target isn't a boss and it's popped now
				{ // then remove yourself as well!
					P_RemoveMobj(mobj);
					return;
				}

#ifdef CHAOSISNOTDEADYET
				if (gametype == GT_CHAOS && mobj->target->health <= 0)
				{
					P_RemoveMobj(mobj);
					return;
				}
#endif
				P_UnsetThingPosition(mobj);
				mobj->x = mobj->target->x;
				mobj->y = mobj->target->y;
				mobj->z = mobj->target->z - FixedMul(50*FRACUNIT, mobj->target->scale);
				mobj->floorz = mobj->z;
				mobj->ceilingz = mobj->z+mobj->height;
				P_SetThingPosition(mobj);
			}
			break;
		case MT_NIGHTSDRONE:
			if (mobj->state >= &states[S_NIGHTSDRONE_SPARKLING1] && mobj->state <= &states[S_NIGHTSDRONE_SPARKLING16])
			{
				mobj->flags2 &= ~MF2_DONTDRAW;
				mobj->z = mobj->floorz + mobj->height + (mobj->spawnpoint->options >> ZSHIFT) * FRACUNIT;
				mobj->angle = 0;

				if (!mobj->target)
				{
					mobj_t *goalpost = P_SpawnMobj(mobj->x, mobj->y, mobj->z + FRACUNIT, MT_NIGHTSGOAL);
					CONS_Debug(DBG_NIGHTSBASIC, "Adding goal post\n");
					goalpost->angle = mobj->angle;
					P_SetTarget(&mobj->target, goalpost);
				}

				if (G_IsSpecialStage(gamemap))
				{ // Never show the NiGHTS drone in special stages. Check ANYONE for bonustime.
					INT32 i;
					boolean bonustime = false;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i] && players[i].bonustime)
						{
							bonustime = true;
							break;
						}
					if (!bonustime)
					{
						mobj->flags &= ~MF_NOGRAVITY;
						P_SetMobjState(mobj, S_NIGHTSDRONE1);
					}
				}
				else if (mobj->tracer && mobj->tracer->player)
				{
					if (!(mobj->tracer->player->pflags & PF_NIGHTSMODE))
					{
						mobj->flags &= ~MF_NOGRAVITY;
						mobj->flags2 &= ~MF2_DONTDRAW;
						P_SetMobjState(mobj, S_NIGHTSDRONE1);
					}
					else if (!mobj->tracer->player->bonustime)
					{
						mobj->flags &= ~MF_NOGRAVITY;
						P_SetMobjState(mobj, S_NIGHTSDRONE1);
					}
				}
			}
			else
			{
				if (G_IsSpecialStage(gamemap))
				{ // Never show the NiGHTS drone in special stages. Check ANYONE for bonustime.
					INT32 i;

					boolean bonustime = false;
					for (i = 0; i < MAXPLAYERS; i++)
						if (playeringame[i] && players[i].bonustime)
						{
							bonustime = true;
							break;
						}

					if (bonustime)
					{
						P_SetMobjState(mobj, S_NIGHTSDRONE_SPARKLING1);
						mobj->flags |= MF_NOGRAVITY;
					}
					else
					{
						if (mobj->target)
						{
							CONS_Debug(DBG_NIGHTSBASIC, "Removing goal post\n");
							P_RemoveMobj(mobj->target);
							P_SetTarget(&mobj->target, NULL);
						}
						mobj->flags2 |= MF2_DONTDRAW;
					}
				}
				else if (mobj->tracer && mobj->tracer->player)
				{
					if (mobj->target)
					{
						CONS_Debug(DBG_NIGHTSBASIC, "Removing goal post\n");
						P_RemoveMobj(mobj->target);
						P_SetTarget(&mobj->target, NULL);
					}

					if (mobj->tracer->player->pflags & PF_NIGHTSMODE)
					{
						if (mobj->tracer->player->bonustime)
						{
							P_SetMobjState(mobj, S_NIGHTSDRONE_SPARKLING1);
							mobj->flags |= MF_NOGRAVITY;
						}
						else
							mobj->flags2 |= MF2_DONTDRAW;
					}
					else // Not NiGHTS
						mobj->flags2 &= ~MF2_DONTDRAW;
				}
				mobj->angle += ANG10;
				if (mobj->z <= mobj->floorz)
					mobj->momz = 5*FRACUNIT;
			}
			break;
		case MT_PLAYER:
			if (mobj->player)
				P_PlayerMobjThinker(mobj);
			return;
		case MT_SKIM:
			// check mobj against possible water content, before movement code
			P_MobjCheckWater(mobj);

			// Keep Skim at water surface
			if (mobj->z <= mobj->watertop)
			{
				mobj->flags |= MF_NOGRAVITY;
				if (mobj->z < mobj->watertop)
				{
					if (mobj->watertop - mobj->z <= FixedMul(mobj->info->speed*FRACUNIT, mobj->scale))
						mobj->z = mobj->watertop;
					else
						mobj->momz = FixedMul(mobj->info->speed*FRACUNIT, mobj->scale);
				}
			}
			else
			{
				mobj->flags &= ~MF_NOGRAVITY;
				if (mobj->z > mobj->watertop && mobj->z - mobj->watertop < FixedMul(MAXSTEPMOVE, mobj->scale))
					mobj->z = mobj->watertop;
			}
			break;
		case MT_RING:
		case MT_COIN:
#ifdef BLUE_SPHERES
		case MT_BLUEBALL:
#endif
		case MT_REDTEAMRING:
		case MT_BLUETEAMRING:
			// No need to check water. Who cares?
			P_RingThinker(mobj);
			if (mobj->flags2 & MF2_NIGHTSPULL)
				P_NightsItemChase(mobj);
			else
				A_AttractChase(mobj);
			return;
		// Flung items
		case MT_FLINGRING:
		case MT_FLINGCOIN:
			if (mobj->flags2 & MF2_NIGHTSPULL)
				P_NightsItemChase(mobj);
			else
				A_AttractChase(mobj);
			break;
		case MT_NIGHTSWING:
			if (mobj->flags2 & MF2_NIGHTSPULL)
				P_NightsItemChase(mobj);
			break;
		case MT_SHELL:
			if (mobj->threshold > TICRATE)
				mobj->threshold--;

			if (mobj->state != &states[S_SHELL])
			{
				mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->x+mobj->momx, mobj->y+mobj->momy);
				P_InstaThrust(mobj, mobj->angle, FixedMul(mobj->info->speed, mobj->scale));
			}
			break;
		case MT_TURRET:
			P_MobjCheckWater(mobj);
			P_CheckPosition(mobj, mobj->x, mobj->y);
			if (P_MobjWasRemoved(mobj))
				return;
			mobj->floorz = tmfloorz;
			mobj->ceilingz = tmceilingz;

			if ((mobj->eflags & MFE_UNDERWATER) && mobj->health > 0)
			{
				P_SetMobjState(mobj, mobj->info->deathstate);
				mobj->health = 0;
				mobj->flags2 &= ~MF2_FIRING;
			}
			else if (mobj->health > 0 && mobj->z + mobj->height > mobj->ceilingz) // Crushed
			{
				INT32 i,j;
				fixed_t ns;
				fixed_t x,y,z;
				mobj_t *mo2;

				z = mobj->subsector->sector->floorheight + FixedMul(64*FRACUNIT, mobj->scale);
				for (j = 0; j < 2; j++)
				{
					for (i = 0; i < 32; i++)
					{
						const angle_t fa = (i*FINEANGLES/16) & FINEMASK;
						ns = FixedMul(64 * FRACUNIT, mobj->scale);
						x = mobj->x + FixedMul(FINESINE(fa),ns);
						y = mobj->y + FixedMul(FINECOSINE(fa),ns);

						mo2 = P_SpawnMobj(x, y, z, MT_EXPLODE);
						ns = FixedMul(16 * FRACUNIT, mobj->scale);
						mo2->momx = FixedMul(FINESINE(fa),ns);
						mo2->momy = FixedMul(FINECOSINE(fa),ns);
					}
					z -= FixedMul(32*FRACUNIT, mobj->scale);
				}
				P_SetMobjState(mobj, mobj->info->deathstate);
				mobj->health = 0;
				mobj->flags2 &= ~MF2_FIRING;
			}
			break;
		case MT_BLUEFLAG:
		case MT_REDFLAG:
			{
				sector_t *sec2;
				sec2 = P_ThingOnSpecial3DFloor(mobj);
				if ((sec2 && GETSECSPECIAL(sec2->special, 4) == 2) || (GETSECSPECIAL(mobj->subsector->sector->special, 4) == 2))
					mobj->fuse = 1; // Return to base.
				break;
			}
		case MT_CANNONBALL:
#ifdef FLOORSPLATS
			R_AddFloorSplat(mobj->tracer->subsector, mobj->tracer, "TARGET", mobj->tracer->x,
				mobj->tracer->y, mobj->tracer->floorz, SPLATDRAWMODE_SHADE);
#endif
			break;
		case MT_SPINFIRE:
			if (mobj->eflags & MFE_VERTICALFLIP)
				mobj->z = mobj->ceilingz - mobj->height;
			else
				mobj->z = mobj->floorz+1;
			// THERE IS NO BREAK HERE ON PURPOSE
		default:
			// check mobj against possible water content, before movement code
			P_MobjCheckWater(mobj);

			// Extinguish fire objects in water
			if (mobj->flags & MF_FIRE && mobj->type != MT_PUMA && mobj->type != MT_FIREBALL
				&& (mobj->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
			{
				P_KillMobj(mobj, NULL, NULL);
				return;
			}
			break;
	}
	if (P_MobjWasRemoved(mobj))
		return;

	if (mobj->flags2 & MF2_FIRING && mobj->target && mobj->health > 0)
	{
		if (mobj->state->action.acp1 == A_Boss1Laser)
		{
			var1 = mobj->state->var1;
			var2 = mobj->state->var2;
			mobj->state->action.acp1(mobj);
		}
		else if (leveltime & 1) // Fire mode
		{
			mobj_t *missile;

			if (mobj->target->player && mobj->target->player->nightstime)
			{
				fixed_t oldval = mobjinfo[mobj->extravalue1].speed;

				mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x+mobj->target->momx, mobj->target->y+mobj->target->momy);
				mobjinfo[mobj->extravalue1].speed = FixedMul(60*FRACUNIT, mobj->scale);
				missile = P_SpawnMissile(mobj, mobj->target, mobj->extravalue1);
				mobjinfo[mobj->extravalue1].speed = oldval;
			}
			else
			{
				mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
				missile = P_SpawnMissile(mobj, mobj->target, mobj->extravalue1);
			}

			if (missile)
			{
				if (mobj->flags2 & MF2_SUPERFIRE)
					missile->flags2 |= MF2_SUPERFIRE;

				if (mobj->info->attacksound)
					S_StartSound(missile, mobj->info->attacksound);
			}
		}
		else
			mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
	}

	if (mobj->flags & MF_AMBIENT)
	{
		if (!(leveltime % mobj->health) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);
		return;
	}

	// Check fuse
	if (mobj->fuse)
	{
		mobj->fuse--;
		if (!mobj->fuse)
		{
			subsector_t *ss;
			fixed_t x, y, z;
			mobj_t *flagmo, *newmobj;

#ifdef HAVE_BLUA
			if (!LUAh_MobjFuse(mobj) && !P_MobjWasRemoved(mobj))
#endif
			switch (mobj->type)
			{
				// gargoyle and snowman handled in P_PushableThinker, not here
				case MT_THROWNGRENADE:
				case MT_CYBRAKDEMON_NAPALM_BOMB_LARGE:
					P_SetMobjState(mobj, mobj->info->deathstate);
					break;
				case MT_BLUEFLAG:
				case MT_REDFLAG:
					if (mobj->spawnpoint)
					{
						x = mobj->spawnpoint->x << FRACBITS;
						y = mobj->spawnpoint->y << FRACBITS;
						ss = R_PointInSubsector(x, y);
						if (mobj->spawnpoint->options & MTF_OBJECTFLIP)
						{
							z = ss->sector->ceilingheight - mobjinfo[mobj->type].height;
							if (mobj->spawnpoint->z)
								z -= mobj->spawnpoint->z << FRACBITS;
						}
						else
						{
							z = ss->sector->floorheight;
							if (mobj->spawnpoint->z)
								z += mobj->spawnpoint->z << FRACBITS;
						}
						flagmo = P_SpawnMobj(x, y, z, mobj->type);
						flagmo->spawnpoint = mobj->spawnpoint;
						if (mobj->spawnpoint->options & MTF_OBJECTFLIP)
							flagmo->spawnpoint->options |= MTF_OBJECTFLIP;

						if (mobj->type == MT_REDFLAG)
						{
							if (!(mobj->flags2 & MF2_JUSTATTACKED))
								CONS_Printf(M_GetText("The red flag has returned to base.\n"));

							if (players[consoleplayer].ctfteam == 1)
								S_StartSound(NULL, sfx_hoop1);

							redflag = flagmo;
						}
						else // MT_BLUEFLAG
						{
							if (!(mobj->flags2 & MF2_JUSTATTACKED))
								CONS_Printf(M_GetText("The blue flag has returned to base.\n"));

							if (players[consoleplayer].ctfteam == 2)
								S_StartSound(NULL, sfx_hoop1);

							blueflag = flagmo;
						}
					}
					P_RemoveMobj(mobj);
					return;
				case MT_YELLOWTV: // Ring shield box
				case MT_BLUETV: // Force shield box
				case MT_GREENTV: // Water shield box
				case MT_BLACKTV: // Bomb shield box
				case MT_WHITETV: // Jump shield box
				case MT_SNEAKERTV: // Super Sneaker box
				case MT_SUPERRINGBOX: // 10-Ring box
				case MT_REDRINGBOX: // Red Team 10-Ring box
				case MT_BLUERINGBOX: // Blue Team 10-Ring box
				case MT_INV: // Invincibility box
				case MT_MIXUPBOX: // Teleporter Mixup box
				case MT_RECYCLETV: // Recycler box
				case MT_SCORETVSMALL:
				case MT_SCORETVLARGE:
				case MT_PRUP: // 1up!
				case MT_EGGMANBOX: // Eggman box
				case MT_GRAVITYBOX: // Gravity box
				case MT_QUESTIONBOX:
					if ((mobj->flags & MF_AMBUSH || mobj->flags2 & MF2_STRONGBOX) && mobj->type != MT_QUESTIONBOX)
					{
						mobjtype_t spawnchance[64];
						INT32 numchoices = 0, i = 0;

// This define should make it a lot easier to organize and change monitor weights
#define SETMONITORCHANCES(type, strongboxamt, weakboxamt) \
for (i = ((mobj->flags2 & MF2_STRONGBOX) ? strongboxamt : weakboxamt); i; --i) spawnchance[numchoices++] = type

						//                Type            SRM WRM
						SETMONITORCHANCES(MT_SNEAKERTV,     0, 10); // Super Sneakers
						SETMONITORCHANCES(MT_INV,           2,  0); // Invincibility
						SETMONITORCHANCES(MT_WHITETV,       3,  8); // Whirlwind Shield
						SETMONITORCHANCES(MT_GREENTV,       3,  8); // Elemental Shield
						SETMONITORCHANCES(MT_YELLOWTV,      2,  0); // Attraction Shield
						SETMONITORCHANCES(MT_BLUETV,        3,  3); // Force Shield
						SETMONITORCHANCES(MT_BLACKTV,       2,  0); // Armageddon Shield
						SETMONITORCHANCES(MT_MIXUPBOX,      0,  1); // Teleporters
						SETMONITORCHANCES(MT_RECYCLETV,     0,  1); // Recycler
						SETMONITORCHANCES(MT_PRUP,          1,  1); // 1-Up
						// ======================================
						//                Total            16  32

#undef SETMONITORCHANCES

						i = P_RandomKey(numchoices); // Gotta love those random numbers!
						newmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, spawnchance[i]);

						// If the monitor respawns randomly, transfer the flag.
						if (mobj->flags & MF_AMBUSH)
							newmobj->flags |= MF_AMBUSH;

						// Transfer flags2 (strongbox, objectflip)
						newmobj->flags2 = mobj->flags2;
					}
					else
					{
						newmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->type);

						// Transfer flags2 (strongbox, objectflip)
						newmobj->flags2 = mobj->flags2;
					}
					P_RemoveMobj(mobj); // make sure they disappear
					return;
				case MT_CHAOSSPAWNER: // Chaos Mode spawner thingy
				{
					// 8 enemies: Blue Crawla, Red Crawla, Crawla Commander,
					//            Jett-Synn Bomber, Jett-Synn Gunner, Skim,
					//            Egg Mobile, Egg Slimer.
					// Max. 3 chances per enemy.

#ifdef CHAOSISNOTDEADYET

					mobjtype_t spawnchance[8*3], enemy;
					mobj_t *spawnedmo;
					INT32 i = 0, numchoices = 0, stop;
					fixed_t sfloorz, space, airspace, spawnz[8*3];

					sfloorz = mobj->floorz;
					space = mobj->ceilingz - sfloorz;

					// This makes the assumption there is no gravity-defying water.
					// A fair assumption to make, if you ask me.
					airspace = min(space, mobj->ceilingz - mobj->watertop);

					mobj->fuse = cv_chaos_spawnrate.value*TICRATE;
					prandom = P_Random(); // Gotta love those random numbers!

					if (cv_chaos_bluecrawla.value && space >= mobjinfo[MT_BLUECRAWLA].height)
					{
						stop = i + cv_chaos_bluecrawla.value;
						for (; i < stop; i++)
						{
							spawnchance[i] = MT_BLUECRAWLA;
							spawnz[i] = sfloorz;
							numchoices++;
						}
					}
					if (cv_chaos_redcrawla.value && space >= mobjinfo[MT_REDCRAWLA].height)
					{
						stop = i + cv_chaos_redcrawla.value;
						for (; i < stop; i++)
						{
							spawnchance[i] = MT_REDCRAWLA;
							spawnz[i] = sfloorz;
							numchoices++;
						}
					}
					if (cv_chaos_crawlacommander.value
						&& space >= mobjinfo[MT_CRAWLACOMMANDER].height + 33*FRACUNIT)
					{
						stop = i + cv_chaos_crawlacommander.value;
						for (; i < stop; i++)
						{
							spawnchance[i] = MT_CRAWLACOMMANDER;
							spawnz[i] = sfloorz + 33*FRACUNIT;
							numchoices++;
						}
					}
					if (cv_chaos_jettysynbomber.value
						&& airspace >= mobjinfo[MT_JETTBOMBER].height + 33*FRACUNIT)
					{
						stop = i + cv_chaos_jettysynbomber.value;
						for (; i < stop; i++)
						{
							spawnchance[i] = MT_JETTBOMBER;
							spawnz[i] = max(sfloorz, mobj->watertop) + 33*FRACUNIT;
							numchoices++;
						}
					}
					if (cv_chaos_jettysyngunner.value
						&& airspace >= mobjinfo[MT_JETTGUNNER].height + 33*FRACUNIT)
					{
						stop = i + cv_chaos_jettysyngunner.value;
						for (; i < stop; i++)
						{
							spawnchance[i] = MT_JETTGUNNER;
							spawnz[i] = max(sfloorz, mobj->watertop) + 33*FRACUNIT;
							numchoices++;
						}
					}
					if (cv_chaos_skim.value
						&& mobj->watertop < mobj->ceilingz - mobjinfo[MT_SKIM].height
						&& mobj->watertop - sfloorz > mobjinfo[MT_SKIM].height/2)
					{
						stop = i + cv_chaos_skim.value;
						for (; i < stop; i++)
						{
							spawnchance[i] = MT_SKIM;
							spawnz[i] = mobj->watertop;
							numchoices++;
						}
					}
					if (P_BossDoesntExist())
					{
						if (cv_chaos_eggmobile1.value
							&& space >= mobjinfo[MT_EGGMOBILE].height + 33*FRACUNIT)
						{
							stop = i + cv_chaos_eggmobile1.value;
							for (; i < stop; i++)
							{
								spawnchance[i] = MT_EGGMOBILE;
								spawnz[i] = sfloorz + 33*FRACUNIT;
								numchoices++;
							}
						}
						if (cv_chaos_eggmobile2.value
							&& space >= mobjinfo[MT_EGGMOBILE2].height + 33*FRACUNIT)
						{
							stop = i + cv_chaos_eggmobile2.value;
							for (; i < stop; i++)
							{
								spawnchance[i] = MT_EGGMOBILE2;
								spawnz[i] = sfloorz + 33*FRACUNIT;
								numchoices++;
							}
						}
					}

					if (numchoices)
					{
						fixed_t fogz;

						i = prandom % numchoices;
						enemy = spawnchance[i];

						fogz = spawnz[i] - 32*FRACUNIT;
						if (fogz < sfloorz)
							fogz = sfloorz;

						spawnedmo = P_SpawnMobj(mobj->x, mobj->y, spawnz[i], enemy);
						P_SpawnMobj(mobj->x, mobj->y, fogz, MT_TFOG);

						P_SupermanLook4Players(spawnedmo);
						if (spawnedmo->target && spawnedmo->type != MT_SKIM)
							P_SetMobjState(spawnedmo, spawnedmo->info->seestate);

						if (spawnedmo->flags & MF_BOSS)
						{
							spawnedmo->flags2 |= MF2_CHAOSBOSS;
							spawnedmo->momx = spawnedmo->momy = 0;
						}
					}
#endif
					break;
				}
				case MT_METALSONIC_BATTLE:
					break; // don't remove
				case MT_SPIKE:
					P_SetMobjState(mobj, mobj->state->nextstate);
					mobj->fuse = mobj->info->speed;
					if (mobj->spawnpoint)
						mobj->fuse += mobj->spawnpoint->angle;
					break;
				case MT_NIGHTSCORE:
					P_RemoveMobj(mobj);
					return;
				default:
					P_SetMobjState(mobj, mobj->info->xdeathstate); // will remove the mobj if S_NULL.
					break;
			}
			if (P_MobjWasRemoved(mobj))
				return;
		}
	}

	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));

	if (mobj->momx || mobj->momy || (mobj->flags2 & MF2_SKULLFLY))
	{
		P_XYMovement(mobj);
		if (P_MobjWasRemoved(mobj))
			return;
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if (!(mobj->eflags & MFE_ONGROUND) || mobj->momz
		|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height != mobj->ceilingz)
		|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z != mobj->floorz)
		|| ((mobj->eflags & (MFE_UNDERWATER|MFE_GOOWATER)) == (MFE_UNDERWATER|MFE_GOOWATER)))
	{
		if (!P_ZMovement(mobj))
			return; // mobj was removed
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
		if (P_MobjWasRemoved(mobj))
			return;
	}
	else
	{
		mobj->pmomz = 0; // to prevent that weird rocketing gargoyle bug
		mobj->eflags &= ~MFE_JUSTHITFLOOR;
	}

	if (mobj->flags & (MF_ENEMY|MF_BOSS) && mobj->health
		&& P_CheckDeathPitCollide(mobj)) // extra pit check in case these didn't have momz
	{
		P_KillMobj(mobj, NULL, NULL);
		return;
	}

	// Crush enemies!
	if (mobj->ceilingz - mobj->floorz < mobj->height)
	{
		if ((
		(mobj->flags & (MF_ENEMY|MF_BOSS)
			&& mobj->flags & MF_SHOOTABLE)
		|| mobj->type == MT_EGGSHIELD)
		&& !(mobj->flags & MF_NOCLIPHEIGHT)
		&& mobj->health > 0)
		{
			P_KillMobj(mobj, NULL, NULL);
			return;
		}
	}

	// Can end up here if a player dies.
	if (mobj->player)
		P_CyclePlayerMobjState(mobj);
	else
		P_CycleMobjState(mobj);

	if (P_MobjWasRemoved(mobj))
		return;

	switch (mobj->type)
	{
		case MT_BOUNCEPICKUP:
		case MT_RAILPICKUP:
		case MT_AUTOPICKUP:
		case MT_EXPLODEPICKUP:
		case MT_SCATTERPICKUP:
		case MT_GRENADEPICKUP:
			if (mobj->health == 0) // Fading tile
			{
				INT32 value = mobj->info->damage/10;
				value = mobj->fuse/value;
				value = 10-value;
				value--;

				if (value <= 0)
					value = 1;

				mobj->frame &= ~FF_TRANSMASK;
				mobj->frame |= value << FF_TRANSSHIFT;
			}
			break;
		default:
			break;
	}
}

// Quick, optimized function for the Rail Rings
// Returns true if move failed or mobj was removed by movement (death pit, missile hits wall, etc.)
boolean P_RailThinker(mobj_t *mobj)
{
	fixed_t x, y, z;

	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));

	x = mobj->x, y = mobj->y, z = mobj->z;

	if (mobj->momx || mobj->momy)
	{
		P_XYMovement(mobj);
		if (P_MobjWasRemoved(mobj))
			return true;
	}

	if (mobj->momz)
	{
		if (!P_ZMovement(mobj))
			return true; // mobj was removed
		//P_CheckPosition(mobj, mobj->x, mobj->y);
	}

	return P_MobjWasRemoved(mobj) || (x == mobj->x && y == mobj->y && z == mobj->z);
}

// Unquick, unoptimized function for pushables
void P_PushableThinker(mobj_t *mobj)
{
	sector_t *sec;

	I_Assert(mobj != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));

	sec = mobj->subsector->sector;

	if (GETSECSPECIAL(sec->special, 2) == 1 && mobj->z == sec->floorheight)
		P_LinedefExecute(sec->tag, mobj, sec);
//	else if (GETSECSPECIAL(sec->special, 2) == 8)
	{
		sector_t *sec2;

		sec2 = P_ThingOnSpecial3DFloor(mobj);
		if (sec2 && GETSECSPECIAL(sec2->special, 2) == 1)
			P_LinedefExecute(sec2->tag, mobj, sec2);
	}

	// it has to be pushable RIGHT NOW for this part to happen
	if (mobj->flags & MF_PUSHABLE && !(mobj->momx || mobj->momy))
		P_TryMove(mobj, mobj->x, mobj->y, true);

	if (mobj->fuse == 1) // it would explode in the MobjThinker code
	{
		mobj_t *spawnmo;
		fixed_t x, y, z;
		subsector_t *ss;

		// Left here just in case we'd
		// want to make pushable bombs
		// or something in the future.
		switch (mobj->type)
		{
			case MT_SNOWMAN:
			case MT_GARGOYLE:
				x = mobj->spawnpoint->x << FRACBITS;
				y = mobj->spawnpoint->y << FRACBITS;

				ss = R_PointInSubsector(x, y);

				if (mobj->spawnpoint->z != 0)
					z = mobj->spawnpoint->z << FRACBITS;
				else
					z = ss->sector->floorheight;

				spawnmo = P_SpawnMobj(x, y, z, mobj->type);
				spawnmo->spawnpoint = mobj->spawnpoint;
				P_UnsetThingPosition(spawnmo);
				spawnmo->flags = mobj->flags;
				P_SetThingPosition(spawnmo);
				spawnmo->flags2 = mobj->flags2;
				spawnmo->flags |= MF_PUSHABLE;
				P_RemoveMobj(mobj);
				break;
			default:
				break;
		}
	}
}

// Quick, optimized function for scenery
void P_SceneryThinker(mobj_t *mobj)
{
	if (mobj->flags & MF_BOXICON)
	{
		if (!(mobj->eflags & MFE_VERTICALFLIP))
		{
			if (mobj->z < mobj->floorz + FixedMul(mobj->info->damage, mobj->scale))
				mobj->momz = FixedMul(mobj->info->speed, mobj->scale);
			else
				mobj->momz = 0;
		}
		else
		{
			if (mobj->z + FixedMul(mobj->info->height, mobj->scale) > mobj->ceilingz - FixedMul(mobj->info->damage, mobj->scale))
				mobj->momz = -FixedMul(mobj->info->speed, mobj->scale);
			else
				mobj->momz = 0;
		}
	}

	// momentum movement
	if (mobj->momx || mobj->momy)
	{
		P_SceneryXYMovement(mobj);

		if (P_MobjWasRemoved(mobj))
			return;
	}

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if (!(mobj->eflags & MFE_ONGROUND) || mobj->momz
		|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height != mobj->ceilingz)
		|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z != mobj->floorz)
		|| ((mobj->eflags & (MFE_UNDERWATER|MFE_GOOWATER)) == (MFE_UNDERWATER|MFE_GOOWATER)))
	{
		if (!P_SceneryZMovement(mobj))
			return; // mobj was removed
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
		if (P_MobjWasRemoved(mobj))
			return;
		mobj->floorz = tmfloorz;
		mobj->ceilingz = tmceilingz;
	}
	else
	{
		mobj->pmomz = 0; // to prevent that weird rocketing gargoyle bug
		mobj->eflags &= ~MFE_JUSTHITFLOOR;
	}

	P_CycleMobjState(mobj);
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnMobj
//
mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	const mobjinfo_t *info = &mobjinfo[type];
	state_t *st;
	mobj_t *mobj = Z_Calloc(sizeof (*mobj), PU_LEVEL, NULL);

	// this is officially a mobj, declared as soon as possible.
	mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
	mobj->type = type;
	mobj->info = info;

	mobj->x = x;
	mobj->y = y;

	mobj->radius = info->radius;
	mobj->height = info->height;
	mobj->flags = info->flags;

	mobj->health = info->spawnhealth;

	mobj->reactiontime = info->reactiontime;

	mobj->lastlook = -1; // stuff moved in P_enemy.P_LookForPlayer

	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	st = &states[info->spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame; // FF_FRAMEMASK for frame, and other bits..
	mobj->friction = ORIG_FRICTION;

	mobj->movefactor = ORIG_FRICTION_FACTOR;

	// All mobjs are created at 100% scale.
	mobj->scale = FRACUNIT;
	mobj->destscale = mobj->scale;
	mobj->scalespeed = FRACUNIT/12;

	// TODO: Make this a special map header
	if ((maptol & TOL_ERZ3) && !(mobj->type == MT_BLACKEGGMAN))
		mobj->destscale = FRACUNIT/2;

	// set subsector and/or block links
	P_SetThingPosition(mobj);
	I_Assert(mobj->subsector != NULL);

	// Make sure scale matches destscale immediately when spawned
	P_SetScale(mobj, mobj->destscale);

	mobj->floorz = mobj->subsector->sector->floorheight;
	mobj->ceilingz = mobj->subsector->sector->ceilingheight;

	// Tells MobjCheckWater that the water height was not set.
	mobj->watertop = INT32_MAX;

	if (z == ONFLOORZ)
	{
		mobj->z = mobj->floorz;

		if (mobj->type == MT_UNIDUS)
			mobj->z += FixedMul(mobj->info->mass, mobj->scale);

		// defaults onground
		if (mobj->z == mobj->floorz)
			mobj->eflags |= MFE_ONGROUND;
	}
	else if (z == ONCEILINGZ)
	{
		mobj->z = mobj->ceilingz - mobj->height;

		if (mobj->type == MT_UNIDUS)
			mobj->z -= FixedMul(mobj->info->mass, mobj->scale);
	}
	else
		mobj->z = z;

#ifdef HAVE_BLUA
	if (!LUAh_MobjSpawn(mobj))
#endif
	switch (mobj->type)
	{
		case MT_CYBRAKDEMON_NAPALM_BOMB_LARGE:
			mobj->fuse = mobj->info->mass;
		case MT_BLACKEGGMAN:
			{
				mobj_t *spawn = P_SpawnMobj(mobj->x, mobj->z, mobj->z+mobj->height-16*FRACUNIT, MT_BLACKEGGMAN_HELPER);
				spawn->destscale = mobj->scale;
				P_SetScale(spawn, mobj->scale);
				P_SetTarget(&spawn->target, mobj);
			}
			break;
		case MT_BLACKEGGMAN_HELPER:
			// Collision helper can be stood on but not pushed
			mobj->flags2 |= MF2_STANDONME;
			break;
		case MT_SPIKE:
			mobj->flags2 |= MF2_STANDONME;
			break;
		case MT_DETON:
			mobj->movedir = 0;
			break;
		case MT_EGGGUARD:
			{
				mobj_t *spawn = P_SpawnMobj(x, y, z, MT_EGGSHIELD);
				spawn->destscale = mobj->scale;
				P_SetScale(spawn, mobj->scale);
				P_SetTarget(&mobj->tracer, spawn);
				P_SetTarget(&spawn->target, mobj);
			}
			break;
		case MT_UNIDUS:
		{
			INT32 i;
			mobj_t *ball;
			// Spawn "damage" number of "painchance" spikeball mobjs
			// threshold is the distance they should keep from the MT_UNIDUS (touching radius + ball painchance)
			for (i = 0; i < mobj->info->damage; i++)
			{
				ball = P_SpawnMobj(x, y, z, mobj->info->painchance);
				ball->destscale = mobj->scale;
				P_SetScale(ball, mobj->scale);
				P_SetTarget(&ball->target, mobj);
				ball->movedir = FixedAngle(FixedMul(FixedDiv(i<<FRACBITS, mobj->info->damage<<FRACBITS), 360<<FRACBITS));
				ball->threshold = ball->radius + mobj->radius + FixedMul(ball->info->painchance, ball->scale);

				var1 = ball->state->var1, var2 = ball->state->var2;
				ball->state->action.acp1(ball);
			}
			break;
		}
		case MT_POINTY:
		{
			INT32 q;
			mobj_t *ball, *lastball = mobj;

			for (q = 0; q < mobj->info->painchance; q++)
			{
				ball = P_SpawnMobj(x, y, z, mobj->info->mass);
				ball->destscale = mobj->scale;
				P_SetScale(ball, mobj->scale);
				P_SetTarget(&lastball->tracer, ball);
				P_SetTarget(&ball->target, mobj);
				lastball = ball;
			}
			break;
		}
		case MT_EGGMOBILE2:
			// Special condition for the 2nd boss.
			mobj->watertop = mobj->info->speed;
			break;
		case MT_BIRD:
		case MT_BUNNY:
		case MT_MOUSE:
		case MT_CHICKEN:
		case MT_COW:
		case MT_REDBIRD:
			mobj->fuse = P_RandomRange(300, 350);
			break;
		case MT_REDRING: // Make MT_REDRING red by default
			mobj->color = skincolor_redring;
			break;
		case MT_SMALLBUBBLE: // Bubbles eventually dissipate, in case they get caught somewhere.
		case MT_MEDIUMBUBBLE:
		case MT_EXTRALARGEBUBBLE:
			mobj->fuse += 30 * TICRATE;
			break;
		case MT_EGGCAPSULE:
			mobj->extravalue1 = -1; // timer for how long a player has been at the capsule
		case MT_REDTEAMRING:
			mobj->color = skincolor_redteam;
			break;
		case MT_BLUETEAMRING:
			mobj->color = skincolor_blueteam;
			break;
		case MT_RING:
		case MT_COIN:
#ifdef BLUE_SPHERES
		case MT_BLUEBALL:
#endif
			nummaprings++;
		default:
			break;
	}

	if (!(mobj->flags & MF_NOTHINK))
		P_AddThinker(&mobj->thinker);

	// Call action functions when the state is set
	if (st->action.acp1 && (mobj->flags & MF_RUNSPAWNFUNC))
	{
		if (levelloading)
		{
			// Cache actions in a linked list
			// with function pointer, and
			// var1 & var2, which will be executed
			// when the level finishes loading.
			P_AddCachedAction(mobj, mobj->info->spawnstate);
		}
		else
		{
			var1 = st->var1;
			var2 = st->var2;
#ifdef HAVE_BLUA
			astate = st;
#endif
			st->action.acp1(mobj);
			// DANGER! This is the ONLY way for P_SpawnMobj to return NULL!
			// Avoid using MF_RUNSPAWNFUNC on mobjs whose spawn state expects target or tracer to already be set!
			if (P_MobjWasRemoved(mobj))
				return NULL;
		}
	}

	if (CheckForReverseGravity && !(mobj->flags & MF_NOBLOCKMAP))
		P_CheckGravity(mobj, false);

	return mobj;
}

static precipmobj_t *P_SpawnPrecipMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	state_t *st;
	precipmobj_t *mobj = Z_Calloc(sizeof (*mobj), PU_LEVEL, NULL);

	mobj->x = x;
	mobj->y = y;
	mobj->flags = mobjinfo[type].flags;

	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	st = &states[mobjinfo[type].spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame; // FF_FRAMEMASK for frame, and other bits..

	// set subsector and/or block links
	P_SetPrecipitationThingPosition(mobj);

	mobj->floorz = mobj->subsector->sector->floorheight;
	mobj->ceilingz = mobj->subsector->sector->ceilingheight;

	mobj->z = z;
	mobj->momz = mobjinfo[type].speed;

	mobj->thinker.function.acp1 = (actionf_p1)P_NullPrecipThinker;
	P_AddThinker(&mobj->thinker);

	CalculatePrecipFloor(mobj);

	if (mobj->floorz != mobj->subsector->sector->floorheight)
		mobj->precipflags |= PCF_FOF;
	else if (GETSECSPECIAL(mobj->subsector->sector->special, 1) == 7
	 || GETSECSPECIAL(mobj->subsector->sector->special, 1) == 6
	 || mobj->subsector->sector->floorpic == skyflatnum)
		mobj->precipflags |= PCF_PIT;

	return mobj;
}

static inline precipmobj_t *P_SpawnRainMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	precipmobj_t *mo = P_SpawnPrecipMobj(x,y,z,type);
	mo->thinker.function.acp1 = (actionf_p1)P_RainThinker;
	return mo;
}

static inline precipmobj_t *P_SpawnSnowMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	precipmobj_t *mo = P_SpawnPrecipMobj(x,y,z,type);
	mo->thinker.function.acp1 = (actionf_p1)P_SnowThinker;
	return mo;
}

//
// P_RemoveMobj
//
mapthing_t *itemrespawnque[ITEMQUESIZE];
tic_t itemrespawntime[ITEMQUESIZE];
size_t iquehead, iquetail;

#ifdef PARANOIA
#define SCRAMBLE_REMOVED // Force debug build to crash when Removed mobj is accessed
#endif
void P_RemoveMobj(mobj_t *mobj)
{
	I_Assert(mobj != NULL);
#ifdef HAVE_BLUA
	if (P_MobjWasRemoved(mobj))
		return; // something already removing this mobj.

	mobj->thinker.function.acp1 = (actionf_p1)P_RemoveThinkerDelayed; // shh. no recursing.
	LUAh_MobjRemoved(mobj);
	mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker; // needed for P_UnsetThingPosition, etc. to work.
#else
	I_Assert(!P_MobjWasRemoved(mobj));
#endif

	// Rings only, please!
	if (mobj->spawnpoint &&
		(mobj->type == MT_RING
		|| mobj->type == MT_COIN
#ifdef BLUE_SPHERES
		|| mobj->type == MT_BLUEBALL
#endif
		|| mobj->type == MT_REDTEAMRING
		|| mobj->type == MT_BLUETEAMRING
		|| P_WeaponOrPanel(mobj->type))
		&& !(mobj->flags2 & MF2_DONTRESPAWN))
	{
		itemrespawnque[iquehead] = mobj->spawnpoint;
		itemrespawntime[iquehead] = leveltime;
		iquehead = (iquehead+1)&(ITEMQUESIZE-1);
		// lose one off the end?
		if (iquehead == iquetail)
			iquetail = (iquetail+1)&(ITEMQUESIZE-1);
	}

	if (mobj->type == MT_OVERLAY)
		P_RemoveOverlay(mobj);

	mobj->health = 0; // Just because

	// unlink from sector and block lists
	P_UnsetThingPosition(mobj);
	if (sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}
	mobj->flags |= MF_NOSECTOR|MF_NOBLOCKMAP;
	mobj->subsector = NULL;
	mobj->state = NULL;
	mobj->player = NULL;

	// stop any playing sound
	S_StopSound(mobj);

	// killough 11/98:
	//
	// Remove any references to other mobjs.
	P_SetTarget(&mobj->target, P_SetTarget(&mobj->tracer, NULL));

	// free block
	// DBG: set everything in mobj_t to 0xFF instead of leaving it. debug memory error.
	if (mobj->flags & MF_NOTHINK && !mobj->thinker.next)
	{ // Uh-oh, the mobj doesn't think, P_RemoveThinker would never go through!
		if (!mobj->thinker.references)
		{
#ifdef SCRAMBLE_REMOVED
			// Invalidate mobj_t data to cause crashes if accessed!
			memset(mobj, 0xff, sizeof(mobj_t));
#endif
			Z_Free(mobj); // No refrences? Can be removed immediately! :D
		}
		else
		{ // Add thinker just to delay removing it until refrences are gone.
			mobj->flags &= ~MF_NOTHINK;
			P_AddThinker((thinker_t *)mobj);
#ifdef SCRAMBLE_REMOVED
			// Invalidate mobj_t data to cause crashes if accessed!
			memset((UINT8 *)mobj + sizeof(thinker_t), 0xff, sizeof(mobj_t) - sizeof(thinker_t));
#endif
			P_RemoveThinker((thinker_t *)mobj);
		}
	}
	else
	{
#ifdef SCRAMBLE_REMOVED
		// Invalidate mobj_t data to cause crashes if accessed!
		memset((UINT8 *)mobj + sizeof(thinker_t), 0xff, sizeof(mobj_t) - sizeof(thinker_t));
#endif
		P_RemoveThinker((thinker_t *)mobj);
	}
}

// This does not need to be added to Lua.
// To test it in Lua, check mobj.valid
boolean P_MobjWasRemoved(mobj_t *mobj)
{
	if (mobj && mobj->thinker.function.acp1 == (actionf_p1)P_MobjThinker)
		return false;
	return true;
}

void P_RemovePrecipMobj(precipmobj_t *mobj)
{
	// unlink from sector and block lists
	P_UnsetPrecipThingPosition(mobj);

	if (precipsector_list)
	{
		P_DelPrecipSeclist(precipsector_list);
		precipsector_list = NULL;
	}

	// free block
	P_RemoveThinker((thinker_t *)mobj);
}

// Clearing out stuff for savegames
void P_RemoveSavegameMobj(mobj_t *mobj)
{
	// unlink from sector and block lists
	P_UnsetThingPosition(mobj);

	// Remove touching_sectorlist from mobj.
	if (sector_list)
	{
		P_DelSeclist(sector_list);
		sector_list = NULL;
	}

	// stop any playing sound
	S_StopSound(mobj);

	// free block
	P_RemoveThinker((thinker_t *)mobj);
}

static CV_PossibleValue_t respawnitemtime_cons_t[] = {{1, "MIN"}, {300, "MAX"}, {0, NULL}};
consvar_t cv_itemrespawntime = {"respawnitemtime", "30", CV_NETVAR|CV_CHEAT, respawnitemtime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_itemrespawn = {"respawnitem", "On", CV_NETVAR, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
static CV_PossibleValue_t flagtime_cons_t[] = {{0, "MIN"}, {300, "MAX"}, {0, NULL}};
consvar_t cv_flagtime = {"flagtime", "30", CV_NETVAR|CV_CHEAT, flagtime_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_suddendeath = {"suddendeath", "Off", CV_NETVAR|CV_CHEAT, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

void P_SpawnPrecipitation(void)
{
	INT32 i, j, mrand;
	fixed_t basex, basey, x, y, height;
	subsector_t *precipsector = NULL;
	precipmobj_t *rainmo = NULL;

	if (dedicated || !cv_precipdensity.value || curWeather == PRECIP_NONE)
		return;

	// Use the blockmap to narrow down our placing patterns
	for (i = 0; i < bmapwidth*bmapheight; ++i)
	{
		basex = bmaporgx + (i % bmapwidth) * MAPBLOCKSIZE;
		basey = bmaporgy + (i / bmapwidth) * MAPBLOCKSIZE;

		for (j = 0; j < cv_precipdensity.value; ++j)
		{
			x = basex + ((M_RandomKey(MAPBLOCKUNITS<<3)<<FRACBITS)>>3);
			y = basey + ((M_RandomKey(MAPBLOCKUNITS<<3)<<FRACBITS)>>3);

			precipsector = R_IsPointInSubsector(x, y);

			// No sector? Stop wasting time,
			// move on to the next entry in the blockmap
			if (!precipsector)
				break;

			// Exists, but is too small for reasonable precipitation.
			if (!(precipsector->sector->floorheight <= precipsector->sector->ceilingheight - (32<<FRACBITS)))
				continue;

			// Don't set height yet...
			height = precipsector->sector->ceilingheight;

			if (curWeather == PRECIP_SNOW)
			{
				// Not in a sector with visible sky -- exception for NiGHTS.
				if (!(maptol & TOL_NIGHTS) && precipsector->sector->ceilingpic != skyflatnum)
					continue;

				rainmo = P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE);
				mrand = M_Random();
				if (mrand < 64)
					P_SetPrecipMobjState(rainmo, S_SNOW3);
				else if (mrand < 144)
					P_SetPrecipMobjState(rainmo, S_SNOW2);
			}
			else // everything else.
			{
				// Not in a sector with visible sky.
				if (precipsector->sector->ceilingpic != skyflatnum)
					continue;

				rainmo = P_SpawnRainMobj(x, y, height, MT_RAIN);
			}

			// Randomly assign a height, now that floorz is set.
			rainmo->z = M_RandomRange(rainmo->floorz>>FRACBITS, rainmo->ceilingz>>FRACBITS)<<FRACBITS;
		}
	}

	if (curWeather == PRECIP_BLANK)
	{
		curWeather = PRECIP_RAIN;
		P_SwitchWeather(PRECIP_BLANK);
	}
	else if (curWeather == PRECIP_STORM_NORAIN)
	{
		curWeather = PRECIP_RAIN;
		P_SwitchWeather(PRECIP_STORM_NORAIN);
	}
}

//
// P_PrecipitationEffects
//
void P_PrecipitationEffects(void)
{
	INT16 thunderchance = INT16_MAX;
	INT32 volume;
	size_t i;

	boolean sounds_rain = true;
	boolean sounds_thunder = true;
	boolean effects_lightning = true;
	boolean lightningStrike = false;

	// No thunder except every other tic.
	if (leveltime & 1);
	// Before, consistency failures were possible if a level started
	// with global rain and switched players to anything else ...
	// If the global weather has lightning strikes,
	// EVERYONE gets them at the SAME time!
	else if (globalweather == PRECIP_STORM
	 || globalweather == PRECIP_STORM_NORAIN)
		thunderchance = (P_RandomKey(8192));
	// But on the other hand, if the global weather is ANYTHING ELSE,
	// don't sync lightning strikes.
	// It doesn't matter whatever curWeather is, we'll only use
	// the variable if we care about it.
	else
		thunderchance = (M_RandomKey(8192));

	if (thunderchance < 70)
		lightningStrike = true;

	switch (curWeather)
	{
		case PRECIP_RAIN: // no lightning or thunder whatsoever
			sounds_thunder = false;
		case PRECIP_STORM_NOSTRIKES: // no lightning strikes specifically
			effects_lightning = false;
			break;
		case PRECIP_STORM_NORAIN: // no rain, lightning and thunder allowed
			sounds_rain = false;
		case PRECIP_STORM: // everything.
			break;
		default:
			// Other weathers need not apply.
			return;
	}

	// Currently thunderstorming with lightning, and we're sounding the thunder...
	// and where there's thunder, there's gotta be lightning!
	if (effects_lightning && lightningStrike)
	{
		sector_t *ss = sectors;

		for (i = 0; i < numsectors; i++, ss++)
			if (ss->ceilingpic == skyflatnum) // Only for the sky.
				P_SpawnLightningFlash(ss); // Spawn a quick flash thinker
	}

	// Local effects from here on out!
	// If we're not in game fully yet, we don't worry about them.
	if (!playeringame[displayplayer] || !players[displayplayer].mo)
		return;

	if (nosound || sound_disabled)
		return; // Sound off? D'aw, no fun.

	if (players[displayplayer].mo->subsector->sector->ceilingpic == skyflatnum)
		volume = 255; // Sky above? We get it full blast.
	else
	{
		fixed_t x, y, yl, yh, xl, xh;
		fixed_t closedist, newdist;

		// Essentially check in a 1024 unit radius of the player for an outdoor area.
		yl = players[displayplayer].mo->y - 1024*FRACUNIT;
		yh = players[displayplayer].mo->y + 1024*FRACUNIT;
		xl = players[displayplayer].mo->x - 1024*FRACUNIT;
		xh = players[displayplayer].mo->x + 1024*FRACUNIT;
		closedist = 2048*FRACUNIT;
		for (y = yl; y <= yh; y += FRACUNIT*64)
			for (x = xl; x <= xh; x += FRACUNIT*64)
			{
				if (R_PointInSubsector(x, y)->sector->ceilingpic == skyflatnum) // Found the outdoors!
				{
					newdist = S_CalculateSoundDistance(players[displayplayer].mo->x, players[displayplayer].mo->y, 0, x, y, 0);
					if (newdist < closedist)
						closedist = newdist;
				}
			}

		volume = 255 - (closedist>>(FRACBITS+2));
	}

	if (volume < 0)
		volume = 0;
	else if (volume > 255)
		volume = 255;

	if (sounds_rain && (!leveltime || leveltime % 80 == 1))
		S_StartSoundAtVolume(players[displayplayer].mo, sfx_rainin, volume);

	if (!sounds_thunder)
		return;

	if (effects_lightning && lightningStrike && volume)
	{
		// Large, close thunder sounds to go with our lightning.
		S_StartSoundAtVolume(players[displayplayer].mo, sfx_litng1 + M_RandomKey(4), volume);
	}
	else if (thunderchance < 20)
	{
		// You can always faintly hear the thunder...
		if (volume < 80)
			volume = 80;

		S_StartSoundAtVolume(players[displayplayer].mo, sfx_athun1 + M_RandomKey(2), volume);
	}
}

//
// P_RespawnSpecials
//
void P_RespawnSpecials(void)
{
	fixed_t x, y, z;
	mobj_t *mo = NULL;
	mapthing_t *mthing = NULL;

	// only respawn items when cv_itemrespawn is on
	if (!(netgame || multiplayer) // Never respawn in single player
	|| gametype == GT_COOP        // Never respawn in co-op gametype
	|| !cv_itemrespawn.value)     // cvar is turned off
		return;

	// Don't respawn in special stages!
	if (G_IsSpecialStage(gamemap))
		return;

	// nothing left to respawn?
	if (iquehead == iquetail)
		return;

	// the first item in the queue is the first to respawn
	// wait at least 30 seconds
	if (leveltime - itemrespawntime[iquetail] < (tic_t)cv_itemrespawntime.value*TICRATE)
		return;

	mthing = itemrespawnque[iquetail];

#ifdef PARANOIA
	if (!mthing)
		I_Error("itemrespawnque[iquetail] is NULL!");
#endif

	if (mthing)
	{
		mobjtype_t i;
		x = mthing->x << FRACBITS;
		y = mthing->y << FRACBITS;

		// find which type to spawn
		for (i = 0; i < NUMMOBJTYPES; i++)
			if (mthing->type == mobjinfo[i].doomednum)
				break;

		z = (mthing->options >> ZSHIFT) * FRACUNIT;

		//CTF rings should continue to respawn as normal rings outside of CTF.
		if (gametype != GT_CTF)
		{
			if (i == MT_REDTEAMRING || i == MT_BLUETEAMRING)
				i = MT_RING;
		}

		mo = P_SpawnMobj(x, y, z, i);
		mo->spawnpoint = mthing;
		mo->angle = ANGLE_45 * (mthing->angle/45);

		if (mthing->options & MTF_OBJECTFLIP)
		{
			mo->z = mo->ceilingz - (mthing->options >> ZSHIFT) * FRACUNIT;
			if (mthing->options & MTF_AMBUSH
			&& (i == MT_RING || i == MT_REDTEAMRING || i == MT_BLUETEAMRING || i == MT_COIN || P_WeaponOrPanel(i)))
				mo->z -= 24*FRACUNIT;
			mo->z -= mobjinfo[i].height; // Don't forget the height!

			mo->eflags |= MFE_VERTICALFLIP;
			mo->flags2 |= MF2_OBJECTFLIP;
		}
		else
		{
			mo->z = mo->floorz + (mthing->options >> ZSHIFT) * FRACUNIT;
			if (mthing->options & MTF_AMBUSH
			&& (i == MT_RING || i == MT_REDTEAMRING || i == MT_BLUETEAMRING || i == MT_COIN || P_WeaponOrPanel(i)))
				mo->z += 24*FRACUNIT;
		}
	}
	// pull it from the que
	iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged between levels.
//
void P_SpawnPlayer(INT32 playernum)
{
	player_t *p = &players[playernum];
	mobj_t *mobj;

	if (p->playerstate == PST_REBORN)
		G_PlayerReborn(playernum);

	// spawn as spectator determination
	if (!G_GametypeHasSpectators())
		p->spectator = false;
	else if (netgame && p->jointime < 1)
		p->spectator = true;
	else if (multiplayer && !netgame)
	{
		// If you're in a team game and you don't have a team assigned yet...
		if (G_GametypeHasTeams() && p->ctfteam == 0)
		{
			changeteam_union NetPacket;
			UINT16 usvalue;
			NetPacket.value.l = NetPacket.value.b = 0;

			// Spawn as a spectator,
			// yes even in splitscreen mode
			p->spectator = true;
			if (playernum&1) p->skincolor = skincolor_redteam;
			else             p->skincolor = skincolor_blueteam;

			// but immediately send a team change packet.
			NetPacket.packet.playernum = playernum;
			NetPacket.packet.verification = true;
			NetPacket.packet.newteam = !(playernum&1) + 1;

			usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}
		else // Otherwise, never spectator.
			p->spectator = false;
	}

	if (G_GametypeHasTeams())
	{
		// Fix stupid non spectator spectators.
		if (!p->spectator && !p->ctfteam)
			p->spectator = true;

		// Fix team colors.
		// This code isn't being done right somewhere else. Oh well.
		if (p->ctfteam == 1)
			p->skincolor = skincolor_redteam;
		else if (p->ctfteam == 2)
			p->skincolor = skincolor_blueteam;
	}

	mobj = P_SpawnMobj(0, 0, 0, MT_PLAYER);
	(mobj->player = p)->mo = mobj;

	mobj->angle = 0;

	// set color translations for player sprites
	mobj->color = p->skincolor;

	// set 'spritedef' override in mobj for player skins.. (see ProjectSprite)
	// (usefulness: when body mobj is detached from player (who respawns),
	// the dead body mobj retains the skin through the 'spritedef' override).
	mobj->skin = &skins[p->skin];

	mobj->health = p->health;
	p->playerstate = PST_LIVE;

	p->bonustime = false;
	p->realtime = leveltime;

	//awayview stuff
	p->awayviewmobj = NULL;
	p->awayviewtics = 0;

	// set the scale to the mobj's destscale so settings get correctly set.  if we don't, they sometimes don't.
	P_SetScale(mobj, mobj->destscale);
	P_FlashPal(p, 0, 0); // Resets

	// Spawn with a pity shield if necessary.
	P_DoPityCheck(p);
}

void P_AfterPlayerSpawn(INT32 playernum)
{
	player_t *p = &players[playernum];
	mobj_t *mobj = p->mo;

	if (playernum == consoleplayer)
		localangle = mobj->angle;
	else if (playernum == secondarydisplayplayer)
		localangle2 = mobj->angle;

	p->viewheight = cv_viewheight.value<<FRACBITS;

	if (p->mo->eflags & MFE_VERTICALFLIP)
		p->viewz = p->mo->z + p->mo->height - p->viewheight;
	else
		p->viewz = p->mo->z + p->viewheight;

	P_SetPlayerMobjState(p->mo, S_PLAY_STND);
	p->pflags &= ~PF_SPINNING;

	if (playernum == consoleplayer)
	{
		// wake up the status bar
		ST_Start();
		// wake up the heads up text
		HU_Start();
	}

	SV_SpawnPlayer(playernum, mobj->x, mobj->y, mobj->angle);

	if (camera.chase)
	{
		if (displayplayer == playernum)
			P_ResetCamera(p, &camera);
	}
	if (camera2.chase && splitscreen)
	{
		if (secondarydisplayplayer == playernum)
			P_ResetCamera(p, &camera2);
	}

	if (CheckForReverseGravity)
		P_CheckGravity(mobj, false);
}

// spawn it at a playerspawn mapthing
void P_MovePlayerToSpawn(INT32 playernum, mapthing_t *mthing)
{
	fixed_t x = 0, y = 0;
	angle_t angle = 0;

	fixed_t z;
	sector_t *sector;


	player_t *p = &players[playernum];
	mobj_t *mobj = p->mo;
	I_Assert(mobj != NULL);

	if (mthing)
	{
		x = mthing->x << FRACBITS;
		y = mthing->y << FRACBITS;
		angle = FixedAngle(mthing->angle*FRACUNIT);
	}
	//spawn at the origin as a desperation move if there is no mapthing

	// set Z height
	sector = R_PointInSubsector(x, y)->sector;
	if (mthing)
	{
		// Flagging a player's ambush will make them start on the ceiling
		// Objectflip inverts
		if (((mthing->options & MTF_AMBUSH) == MTF_AMBUSH) ^ ((mthing->options & MTF_OBJECTFLIP) == MTF_OBJECTFLIP))
		{
			z = sector->ceilingheight - mobjinfo[MT_PLAYER].height;
			if (mthing->options >> ZSHIFT)
				z -= ((mthing->options >> ZSHIFT) << FRACBITS);
		}
		else
		{
			z = sector->floorheight;
			if (mthing->options >> ZSHIFT)
				z += ((mthing->options >> ZSHIFT) << FRACBITS);
		}

		if (mthing->options & MTF_OBJECTFLIP)
			mobj->flags2 |= MF2_OBJECTFLIP; // flip the player!
	}
	else
		z = sector->floorheight;

	if (z < sector->floorheight)
		z = sector->floorheight;
	else if (z > sector->ceilingheight - mobjinfo[MT_PLAYER].height)
		z = sector->ceilingheight - mobjinfo[MT_PLAYER].height;

	mobj->floorz = sector->floorheight;
	mobj->ceilingz = sector->ceilingheight;

	P_UnsetThingPosition(mobj);
	mobj->x = x;
	mobj->y = y;
	P_SetThingPosition(mobj);

	mobj->z = z;
	if (mobj->z == sector->floorheight)
		mobj->eflags |= MFE_ONGROUND;

	mobj->angle = angle;

	P_AfterPlayerSpawn(playernum);
}

void P_MovePlayerToStarpost(INT32 playernum)
{
	fixed_t z;
	sector_t *sector;

	player_t *p = &players[playernum];
	mobj_t *mobj = p->mo;
	I_Assert(mobj != NULL);

	P_UnsetThingPosition(mobj);
	mobj->x = p->starpostx << FRACBITS;
	mobj->y = p->starposty << FRACBITS;
	P_SetThingPosition(mobj);
	sector = R_PointInSubsector(mobj->x, mobj->y)->sector;

	z = p->starpostz << FRACBITS;
	if (z < sector->floorheight)
		z = sector->floorheight;
	else if (z > sector->ceilingheight - mobjinfo[MT_PLAYER].height)
		z = sector->ceilingheight - mobjinfo[MT_PLAYER].height;

	mobj->floorz = sector->floorheight;
	mobj->ceilingz = sector->ceilingheight;

	mobj->z = z;
	if (mobj->z == mobj->floorz)
		mobj->eflags |= MFE_ONGROUND;

	mobj->angle = p->starpostangle;

	P_AfterPlayerSpawn(playernum);

	if (!(netgame || multiplayer))
		leveltime = p->starposttime;
}

#define MAXHUNTEMERALDS 64
mapthing_t *huntemeralds[MAXHUNTEMERALDS];
INT32 numhuntemeralds;

//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing(mapthing_t *mthing)
{
	mobjtype_t i;
	mobj_t *mobj;
	fixed_t x, y, z;
	subsector_t *ss;

	if (!mthing->type)
		return; // Ignore type-0 things as NOPs

	// Always spawn in objectplace.
	// Skip all returning code.
	if (objectplacing)
	{
		// find which type to spawn
		for (i = 0; i < NUMMOBJTYPES; i++)
			if (mthing->type == mobjinfo[i].doomednum)
				break;

		if (i == NUMMOBJTYPES)
		{
			if (mthing->type == 3328) // 3D Mode start Thing
				return;
			CONS_Alert(CONS_WARNING, M_GetText("Unknown thing type %d placed at (%d, %d)\n"), mthing->type, mthing->x, mthing->y);
			i = MT_UNKNOWN;
		}
		goto noreturns;
	}

	// count deathmatch start positions
	if (mthing->type == 33)
	{
		if (numdmstarts < MAX_DM_STARTS)
		{
			deathmatchstarts[numdmstarts] = mthing;
			mthing->type = 0;
			numdmstarts++;
		}
		return;
	}

	else if (mthing->type == 34) // Red CTF Starts
	{
		if (numredctfstarts < MAXPLAYERS)
		{
			redctfstarts[numredctfstarts] = mthing;
			mthing->type = 0;
			numredctfstarts++;
		}
		return;
	}

	else if (mthing->type == 35) // Blue CTF Starts
	{
		if (numbluectfstarts < MAXPLAYERS)
		{
			bluectfstarts[numbluectfstarts] = mthing;
			mthing->type = 0;
			numbluectfstarts++;
		}
		return;
	}

	else if (mthing->type == 300 // Ring
		|| mthing->type == 308 || mthing->type == 309 // Team Rings
		|| mthing->type == 1706 // Nights Wing
		|| (mthing->type >= 600 && mthing->type <= 609) // Placement patterns
		|| mthing->type == 1705 || mthing->type == 1713 // NiGHTS Hoops
		|| mthing->type == 1800) // Mario Coin
	{
		// Don't spawn hoops, wings, or rings yet!
		return;
	}

	// check for players specially
	if (mthing->type > 0 && mthing->type <= 32)
	{
		// save spots for respawning in network games
		if (!metalrecording)
			playerstarts[mthing->type-1] = mthing;
		return;
	}

	if (metalrecording && mthing->type == mobjinfo[MT_METALSONIC_RACE].doomednum)
	{ // If recording, you ARE Metal Sonic. Do not spawn it, do not save normal spawnpoints.
		playerstarts[0] = mthing;
		return;
	}

	// find which type to spawn
	for (i = 0; i < NUMMOBJTYPES; i++)
		if (mthing->type == mobjinfo[i].doomednum)
			break;

	if (i == NUMMOBJTYPES)
	{
		if (mthing->type == 3328) // 3D Mode start Thing
			return;
		CONS_Alert(CONS_WARNING, M_GetText("Unknown thing type %d placed at (%d, %d)\n"), mthing->type, mthing->x, mthing->y);
		i = MT_UNKNOWN;
	}

	if (metalrecording) // Metal Sonic can't use these things.
		if (mobjinfo[i].flags & (MF_ENEMY|MF_BOSS) || i == MT_EMMY || i == MT_STARPOST)
			return;

	if (i >= MT_EMERALD1 && i <= MT_EMERALD7) // Pickupable Emeralds
	{
		if (gametype != GT_COOP) // Don't place emeralds in non-coop modes
			return;

		if (metalrecording)
			return; // Metal Sonic isn't for collecting emeralds.

		if (emeralds & mobjinfo[i].speed) // You already have this emerald!
			return;
	}

	if (!G_RingSlingerGametype() || !cv_specialrings.value)
		if (P_WeaponOrPanel(i))
			return; // Don't place weapons/panels in non-ringslinger modes

	if (i == MT_EMERHUNT)
	{
		// Emerald Hunt is Coop only.
		if (gametype != GT_COOP)
			return;

		ss = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS);
		mthing->z = (INT16)((ss->sector->floorheight>>FRACBITS) + (mthing->options >> ZSHIFT));

		if (numhuntemeralds < MAXHUNTEMERALDS)
			huntemeralds[numhuntemeralds++] = mthing;
		return;
	}

	if (i == MT_EMERALDSPAWN)
	{
		if (!cv_powerstones.value)
			return;

		if (!(gametype == GT_MATCH || gametype == GT_CTF))
			return;

		runemeraldmanager = true;
	}

	if (!G_PlatformGametype()) // No enemies in match or CTF modes
		if ((mobjinfo[i].flags & MF_ENEMY) || (mobjinfo[i].flags & MF_BOSS))
			return;

	// Set powerup boxes to user settings for competition.
	if (gametype == GT_COMPETITION)
	{
		if ((mobjinfo[i].flags & MF_MONITOR) && cv_competitionboxes.value) // not Normal
		{
			if (cv_competitionboxes.value == 1) // Random
				i = MT_QUESTIONBOX;
			else if (cv_competitionboxes.value == 2) // Teleports
				i = MT_MIXUPBOX;
			else if (cv_competitionboxes.value == 3) // None
				return; // Don't spawn!
		}
	}

	// Set powerup boxes to user settings for other netplay modes
	else if (gametype != GT_COOP)
	{
		if ((mobjinfo[i].flags & MF_MONITOR) && cv_matchboxes.value) // not Normal
		{
			if (cv_matchboxes.value == 1) // Random
				i = MT_QUESTIONBOX;
			else if (cv_matchboxes.value == 3) // Don't spawn
				return;
			else // cv_matchboxes.value == 2, Non-Random
			{
				if (i == MT_QUESTIONBOX)
					return; // don't spawn in Non-Random

				mthing->options &= ~(MTF_AMBUSH|MTF_OBJECTSPECIAL); // no random respawning!
			}
		}
	}

	if (gametype != GT_CTF) // CTF specific things
	{
		if (i == MT_BLUETEAMRING || i == MT_REDTEAMRING)
			i = MT_RING;
		else if (i == MT_BLUERINGBOX || i == MT_REDRINGBOX)
			i = MT_SUPERRINGBOX;
		else if (i == MT_BLUEFLAG || i == MT_REDFLAG)
			return; // No flags in non-CTF modes!
	}
	else
	{
		if ((i == MT_BLUEFLAG && blueflag) || (i == MT_REDFLAG && redflag))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Only one flag per team allowed in CTF!"));
			return;
		}
	}

	if (!G_PlatformGametype() && (i == MT_SIGN || i == MT_STARPOST))
		return; // Don't spawn exit signs or starposts in wrong game modes

	if (modeattacking) // Record Attack special stuff
	{
		// Don't spawn starposts that wouldn't be usable
		if (i == MT_STARPOST)
			return;

		// Emerald Tokens -->> Score Tokens
		else if (i == MT_EMMY)
			return; /// \todo

		// 1UPs -->> Score TVs
		else if (i == MT_PRUP) // 1UP
		{
			// Either or, doesn't matter which.
			if (mthing->options & (MTF_AMBUSH|MTF_OBJECTSPECIAL))
				i = MT_SCORETVLARGE; // 10,000
			else
				i = MT_SCORETVSMALL; // 1,000
		}
	}

	if (ultimatemode)
	{
		if (i == MT_PITYTV || i == MT_GREENTV || i == MT_YELLOWTV || i == MT_BLUETV || i == MT_BLACKTV || i == MT_WHITETV)
			return; // No shields in Ultimate mode

		if (i == MT_SUPERRINGBOX && !G_IsSpecialStage(gamemap))
			return; // No rings in Ultimate mode (except special stages)
	}

	if (i == MT_EMMY && (gametype != GT_COOP || ultimatemode || tokenbits == 30 || tokenlist & (1 << tokenbits++)))
		return; // you already got this token, or there are too many, or the gametype's not right

	// Objectplace landing point
	noreturns:

	// spawn it
	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;
	ss = R_PointInSubsector(x, y);

	if (i == MT_NIGHTSBUMPER)
		z = ss->sector->floorheight + ((mthing->options >> ZSHIFT) << FRACBITS);
	else if (i == MT_AXIS || i == MT_AXISTRANSFER || i == MT_AXISTRANSFERLINE)
		z = ONFLOORZ;
	else if (i == MT_SPECIALSPIKEBALL || P_WeaponOrPanel(i) || i == MT_EMERALDSPAWN || i == MT_EMMY)
	{
		if (!(mthing->options & MTF_OBJECTFLIP))
		{
			z = ss->sector->floorheight;

			if (mthing->options & MTF_AMBUSH) // Special flag for rings
				z += 24*FRACUNIT;
			if (mthing->options >> ZSHIFT)
				z += (mthing->options >> ZSHIFT)*FRACUNIT;
		}
		else
		{
			z = ss->sector->ceilingheight;

			if (mthing->options & MTF_AMBUSH) // Special flag for rings
				z -= 24*FRACUNIT;
			if (mthing->options >> ZSHIFT)
				z -= (mthing->options >> ZSHIFT)*FRACUNIT;

			z -= mobjinfo[i].height; //Don't forget the height!
		}

		if (z == ONFLOORZ)
			mthing->z = 0;
		else
			mthing->z = (INT16)(z>>FRACBITS);
	}
	else
	{
		fixed_t offset = 0;
		boolean flip = (((mobjinfo[i].flags & MF_SPAWNCEILING) == MF_SPAWNCEILING) ^ ((mthing->options & MTF_OBJECTFLIP) == MTF_OBJECTFLIP));

		// base positions
		if (flip)
			z = ss->sector->ceilingheight - mobjinfo[i].height;
		else
			z = ss->sector->floorheight;

		// offsetting
		if (mthing->options >> ZSHIFT)
			offset = ((mthing->options >> ZSHIFT) << FRACBITS);
		else if (i == MT_CRAWLACOMMANDER || i == MT_DETON || i == MT_JETTBOMBER || i == MT_JETTGUNNER || i == MT_EGGMOBILE2)
			offset = 33*FRACUNIT;
		else if (i == MT_EGGMOBILE)
			offset = 128*FRACUNIT;
		else if (i == MT_GOLDBUZZ || i == MT_REDBUZZ)
			offset = 288*FRACUNIT;

		// applying offsets! (if any)
		if (flip)
		{
			if (offset)
				z -= offset;
			else
				z = ONCEILINGZ;
		}
		else
		{
			if (offset)
				z += offset;
			else
				z = ONFLOORZ;
		}

		if (z == ONFLOORZ)
			mthing->z = 0;
		else
			mthing->z = (INT16)(z>>FRACBITS);
	}

	mobj = P_SpawnMobj(x, y, z, i);
	mobj->spawnpoint = mthing;

	switch(mobj->type)
	{
	case MT_SKYBOX:
		mobj->angle = 0;
		if (mthing->options & MTF_OBJECTSPECIAL)
			skyboxmo[1] = mobj;
		else
			skyboxmo[0] = mobj;
		break;
	case MT_FAN:
		if (mthing->options & MTF_OBJECTSPECIAL)
		{
			P_UnsetThingPosition(mobj);
			if (sector_list)
			{
				P_DelSeclist(sector_list);
				sector_list = NULL;
			}
			mobj->flags |= MF_NOSECTOR; // this flag basically turns it invisible
			P_SetThingPosition(mobj);
		}
		if (mthing->angle)
			mobj->health = mthing->angle;
		else
			mobj->health = FixedMul((ss->sector->ceilingheight-ss->sector->floorheight), 3*(FRACUNIT/4))>>FRACBITS;
		break;
	case MT_WATERDRIP:
		if (mthing->angle)
			mobj->tics = 3*TICRATE + mthing->angle;
		else
			mobj->tics = 3*TICRATE;
		break;
	case MT_FLAMEJET:
	case MT_VERTICALFLAMEJET:
		mobj->threshold = (mthing->angle >> 10) & 7;
		mobj->movecount = (mthing->angle >> 13);

		mobj->threshold *= (TICRATE/2);
		mobj->movecount *= (TICRATE/2);

		mobj->movedir = mthing->extrainfo;
		break;
	case MT_MACEPOINT:
	case MT_SWINGMACEPOINT:
	case MT_HANGMACEPOINT:
	case MT_SPINMACEPOINT:
	{
		fixed_t mlength, mspeed, mxspeed, mzspeed, mstartangle, mmaxspeed;
		mobjtype_t chainlink = MT_SMALLMACECHAIN;
		mobjtype_t macetype = MT_SMALLMACE;
		boolean firsttime;
		mobj_t *spawnee;
		size_t line;
		const size_t mthingi = (size_t)(mthing - mapthings);

		// Why does P_FindSpecialLineFromTag not work here?!?
		for (line = 0; line < numlines; line++)
		{
			if (lines[line].special == 9 && lines[line].tag == mthing->angle)
				break;
		}

		if (line == numlines)
		{
			CONS_Debug(DBG_GAMELOGIC, "Mace chain (mapthing #%s) needs tagged to a #9 parameter line (trying to find tag %d).\n", sizeu1(mthingi), mthing->angle);
			return;
		}
/*
No deaf - small mace
Deaf - big mace

ML_NOCLIMB : Direction not controllable
*/
		mlength = abs(lines[line].dx >> FRACBITS);
		mspeed = abs(lines[line].dy >> FRACBITS);
		mxspeed = sides[lines[line].sidenum[0]].textureoffset >> FRACBITS;
		mzspeed = sides[lines[line].sidenum[0]].rowoffset >> FRACBITS;
		mstartangle = lines[line].frontsector->floorheight >> FRACBITS;
		mmaxspeed = lines[line].frontsector->ceilingheight >> FRACBITS;

		mstartangle %= 360;
		mxspeed %= 360;
		mzspeed %= 360;

		CONS_Debug(DBG_GAMELOGIC, "Mace Chain (mapthing #%s):\n"
				"Length is %d\n"
				"Speed is %d\n"
				"Xspeed is %d\n"
				"Zspeed is %d\n"
				"startangle is %d\n"
				"maxspeed is %d\n",
				sizeu1(mthingi), mlength, mspeed, mxspeed, mzspeed, mstartangle, mmaxspeed);

		mobj->lastlook = mspeed << 4;
		mobj->movecount = mobj->lastlook;
		mobj->health = (FixedAngle(mzspeed*FRACUNIT)>>ANGLETOFINESHIFT) + (FixedAngle(mstartangle*FRACUNIT)>>ANGLETOFINESHIFT);
		mobj->threshold = (FixedAngle(mxspeed*FRACUNIT)>>ANGLETOFINESHIFT) + (FixedAngle(mstartangle*FRACUNIT)>>ANGLETOFINESHIFT);
		mobj->movefactor = mobj->threshold;
		mobj->friction = mmaxspeed;

		if (lines[line].flags & ML_NOCLIMB)
			mobj->flags |= MF_SLIDEME;

		mobj->reactiontime = 0;

		if (mthing->options & MTF_AMBUSH)
		{
			chainlink = MT_BIGMACECHAIN;
			macetype = MT_BIGMACE;
		}

		if (mthing->options & MTF_OBJECTSPECIAL)
			mobj->flags2 |= MF2_BOSSNOTRAP; // shut up maces.

		if (mobj->type == MT_HANGMACEPOINT || mobj->type == MT_SPINMACEPOINT)
			firsttime = true;
		else
		{
			firsttime = false;

			spawnee = P_SpawnMobj(mobj->x, mobj->y, mobj->z, macetype);
			P_SetTarget(&spawnee->target, mobj);

			if (mobj->type == MT_SWINGMACEPOINT)
				spawnee->movecount = FixedAngle(mstartangle*FRACUNIT)>>ANGLETOFINESHIFT;
			else
				spawnee->movecount = 0;

			spawnee->threshold = FixedAngle(mstartangle*FRACUNIT)>>ANGLETOFINESHIFT;
			spawnee->reactiontime = mlength+1;
		}

		while (mlength > 0)
		{
			spawnee = P_SpawnMobj(mobj->x, mobj->y, mobj->z, chainlink);

			P_SetTarget(&spawnee->target, mobj);

			if (mobj->type == MT_HANGMACEPOINT || mobj->type == MT_SWINGMACEPOINT)
				spawnee->movecount = FixedAngle(mstartangle*FRACUNIT)>>ANGLETOFINESHIFT;
			else
				spawnee->movecount = 0;

			spawnee->threshold = FixedAngle(mstartangle*FRACUNIT)>>ANGLETOFINESHIFT;
			spawnee->reactiontime = mlength;

			if (firsttime)
			{
				// This is the outermost link in the chain
				spawnee->flags |= MF_AMBUSH;
				firsttime = false;
			}

			mlength--;
		}
		break;
	}
	case MT_ROCKSPAWNER:
		mobj->threshold = mthing->angle;
		mobj->movecount = mthing->extrainfo;
		break;
	case MT_POPUPTURRET:
		if (mthing->angle)
			mobj->threshold = mthing->angle;
		else
			mobj->threshold = (TICRATE*2)-1;
		break;
	case MT_NIGHTSBUMPER:
		// Lower 4 bits specify the angle of
		// the bumper in 30 degree increments.
		mobj->threshold = (mthing->options & 15) % 12; // It loops over, etc
		P_SetMobjState(mobj, mobj->info->spawnstate+mobj->threshold);

		// you can shut up now, OBJECTFLIP.  And all of the other options, for that matter.
		mthing->options &= ~0xF;
		break;
	case MT_EGGCAPSULE:
		if (mthing->angle <= 0)
			mthing->angle = 20; // prevent 0 health

		mobj->health = mthing->angle;
		mobj->threshold = min(mthing->extrainfo, 7);
		break;
	case MT_TUBEWAYPOINT:
		mobj->health = mthing->angle & 255;
		mobj->threshold = mthing->angle >> 8;
		break;
	case MT_NIGHTSDRONE:
		if (mthing->angle > 0)
			mobj->health = mthing->angle;
		break;
	case MT_CHAOSSPAWNER:
#ifndef CHAOSISNOTDEADYET
		return;
#else
		if (gametype != GT_CHAOS)
			return;
		mobj->fuse = P_Random()*2;
		break;
#endif
	case MT_TRAPGOYLE:
	case MT_TRAPGOYLEUP:
	case MT_TRAPGOYLEDOWN:
	case MT_TRAPGOYLELONG:
		if (mthing->angle >= 360)
			mobj->tics += 7*(mthing->angle / 360) + 1; // starting delay
	default:
		break;
	}

	if (mobj->flags & MF_BOSS)
	{
		if (mthing->options & MTF_OBJECTSPECIAL) // No egg trap for this boss
			mobj->flags2 |= MF2_BOSSNOTRAP;

		z = R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> (ZSHIFT)) << FRACBITS);

		mthing->z = (INT16)(z>>FRACBITS);
	}

	if (i == MT_AXIS || i == MT_AXISTRANSFER || i == MT_AXISTRANSFERLINE) // Axis Points
	{
		// Mare it belongs to
		mobj->threshold = min(mthing->extrainfo, 7);

		// # in the mare
		mobj->health = mthing->options;

		mobj->flags2 |= MF2_AXIS;

		if (i == MT_AXIS)
		{
			// Inverted if uppermost bit is set
			if (mthing->angle & 16384)
				mobj->flags |= MF_AMBUSH;

			if (mthing->angle > 0)
				mobj->radius = (mthing->angle & 16383)*FRACUNIT;
		}
	}
	else if (i == MT_EMMY)
	{
		if (mthing->options & MTF_OBJECTSPECIAL) // Mario Block version
			mobj->flags &= ~(MF_NOGRAVITY|MF_NOCLIPHEIGHT);
		else
		{
			fixed_t zheight = mobj->z;
			mobj_t *tokenobj;

			if (mthing->options & MTF_OBJECTFLIP)
				zheight += mobj->height-FixedMul(mobjinfo[MT_TOKEN].height, mobj->scale); // align with emmy properly!

			tokenobj = P_SpawnMobj(x, y, zheight, MT_TOKEN);
			P_SetTarget(&mobj->tracer, tokenobj);
			tokenobj->destscale = mobj->scale;
			P_SetScale(tokenobj, mobj->scale);
			if (mthing->options & MTF_OBJECTFLIP) // flip token to match emmy
			{
				tokenobj->eflags |= MFE_VERTICALFLIP;
				tokenobj->flags2 |= MF2_OBJECTFLIP;
			}
		}

		// We advanced tokenbits earlier due to the return check.
		// Subtract 1 here for the correct value.
		mobj->health = 1 << (tokenbits - 1);
	}
/*	else if (i == MT_EGGMOBILE && mthing->options & MTF_AMBUSH)
	{
		mobj_t *spikemobj;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		P_SetTarget(&spikemobj->target, mobj);
		spikemobj->angle = 0;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		P_SetTarget(&spikemobj->target, mobj);
		spikemobj->angle = ANGLE_90;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		P_SetTarget(&spikemobj->target, mobj);
		spikemobj->angle = ANGLE_180;
		spikemobj = P_SpawnMobj(x, y, z, MT_SPIKEBALL);
		P_SetTarget(&spikemobj->target, mobj);
		spikemobj->angle = ANGLE_270;
	}*/
	else if (i == MT_CYBRAKDEMON && mthing->options & MTF_AMBUSH)
	{
		mobj_t *elecmobj;
		elecmobj = P_SpawnMobj(x, y, z, MT_CYBRAKDEMON_ELECTRIC_BARRIER);
		P_SetTarget(&elecmobj->target, mobj);
		elecmobj->angle = FixedAngle(mthing->angle*FRACUNIT);;
		elecmobj->destscale = mobj->scale*2;
		P_SetScale(elecmobj, elecmobj->destscale);
	}
	else if (i == MT_STARPOST)
	{
		thinker_t *th;
		mobj_t *mo2;
		boolean foundanother = false;
		mobj->health = (mthing->angle / 360) + 1;

		// See if other starposts exist in this level that have the same value.
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2 == mobj)
				continue;

			if (mo2->type == MT_STARPOST && mo2->health == mobj->health)
			{
				foundanother = true;
				break;
			}
		}

		if (!foundanother)
			numstarposts++;
	}
	else if (i == MT_SPIKE)
	{
		// Pop up spikes!
		if (mthing->options & MTF_OBJECTSPECIAL)
		{
			mobj->flags &= ~MF_SCENERY;
			mobj->fuse = mthing->angle + mobj->info->speed;
		}
		// Use per-thing collision for spikes if the deaf flag is checked.
		if (mthing->options & MTF_AMBUSH && !metalrecording)
		{
			P_UnsetThingPosition(mobj);
			mobj->flags &= ~(MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT);
			mobj->flags |= MF_SOLID;
			P_SetThingPosition(mobj);
		}
	}

	//count 10 ring boxes into the number of rings equation too.
	if (i == MT_SUPERRINGBOX)
		nummaprings += 10;

	if (i == MT_BIGTUMBLEWEED || i == MT_LITTLETUMBLEWEED)
	{
		if (mthing->options & MTF_AMBUSH)
		{
			mobj->momz += FixedMul(16*FRACUNIT, mobj->scale);

			if (P_Random() & 1)
				mobj->momx += FixedMul(16*FRACUNIT, mobj->scale);
			else
				mobj->momx -= FixedMul(16*FRACUNIT, mobj->scale);

			if (P_Random() & 1)
				mobj->momy += FixedMul(16*FRACUNIT, mobj->scale);
			else
				mobj->momy -= FixedMul(16*FRACUNIT,mobj->scale);
		}
	}

	// CTF flag pointers
	if (i == MT_REDFLAG)
	{
		redflag = mobj;
		rflagpoint = mobj->spawnpoint;
	}
	if (i == MT_BLUEFLAG)
	{
		blueflag = mobj;
		bflagpoint = mobj->spawnpoint;
	}

	// special push/pull stuff
	if (i == MT_PUSH || i == MT_PULL)
	{
		mobj->health = 0; // Default behaviour: pushing uses XY, fading uses XYZ

		if (mthing->options & MTF_AMBUSH)
			mobj->health |= 1; // If ambush is set, push using XYZ
		if (mthing->options & MTF_OBJECTSPECIAL)
			mobj->health |= 2; // If object special is set, fade using XY

		if (G_IsSpecialStage(gamemap))
		{
			if (i == MT_PUSH)
				P_SetMobjState(mobj, S_GRAVWELLGREEN);
			if (i == MT_PULL)
				P_SetMobjState(mobj, S_GRAVWELLRED);
		}
	}

	mobj->angle = FixedAngle(mthing->angle*FRACUNIT);

	if ((mthing->options & MTF_AMBUSH)
	&& (mthing->options & MTF_OBJECTSPECIAL)
	&& (mobj->flags & MF_PUSHABLE))
		mobj->flags2 |= MF2_CLASSICPUSH;
	else
	{
		if (mthing->options & MTF_AMBUSH)
		{
			if (i == MT_YELLOWDIAG || i == MT_REDDIAG)
				mobj->angle += ANGLE_22h;

			if (mobj->flags & MF_NIGHTSITEM)
			{
				// Spawn already displayed
				mobj->flags |= MF_SPECIAL;
				mobj->flags &= ~MF_NIGHTSITEM;
				P_SetMobjState(mobj, mobj->info->seestate);
			}

			if (mobj->flags & MF_PUSHABLE)
			{
				mobj->flags &= ~MF_PUSHABLE;
				mobj->flags2 |= MF2_STANDONME;
			}

			if (mobj->flags & MF_MONITOR)
			{
				// flag for strong/weak random boxes
				if (mthing->type == mobjinfo[MT_SUPERRINGBOX].doomednum || mthing->type == mobjinfo[MT_PRUP].doomednum ||
					mthing->type == mobjinfo[MT_SNEAKERTV].doomednum || mthing->type == mobjinfo[MT_INV].doomednum ||
					mthing->type == mobjinfo[MT_WHITETV].doomednum || mthing->type == mobjinfo[MT_GREENTV].doomednum ||
					mthing->type == mobjinfo[MT_YELLOWTV].doomednum || mthing->type == mobjinfo[MT_BLUETV].doomednum ||
					mthing->type == mobjinfo[MT_BLACKTV].doomednum || mthing->type == mobjinfo[MT_PITYTV].doomednum ||
					mthing->type == mobjinfo[MT_RECYCLETV].doomednum || mthing->type == mobjinfo[MT_MIXUPBOX].doomednum)
						mobj->flags |= MF_AMBUSH;
			}

			else if (mthing->type != mobjinfo[MT_AXIS].doomednum &&
				mthing->type != mobjinfo[MT_AXISTRANSFER].doomednum &&
				mthing->type != mobjinfo[MT_AXISTRANSFERLINE].doomednum &&
				mthing->type != mobjinfo[MT_NIGHTSBUMPER].doomednum &&
				mthing->type != mobjinfo[MT_STARPOST].doomednum)
				mobj->flags |= MF_AMBUSH;
		}

		if (mthing->options & MTF_OBJECTSPECIAL)
		{
			// flag for strong/weak random boxes
			if (mthing->type == mobjinfo[MT_SUPERRINGBOX].doomednum || mthing->type == mobjinfo[MT_PRUP].doomednum ||
				mthing->type == mobjinfo[MT_SNEAKERTV].doomednum || mthing->type == mobjinfo[MT_INV].doomednum ||
				mthing->type == mobjinfo[MT_WHITETV].doomednum || mthing->type == mobjinfo[MT_GREENTV].doomednum ||
				mthing->type == mobjinfo[MT_YELLOWTV].doomednum || mthing->type == mobjinfo[MT_BLUETV].doomednum ||
				mthing->type == mobjinfo[MT_BLACKTV].doomednum || mthing->type == mobjinfo[MT_PITYTV].doomednum ||
				mthing->type == mobjinfo[MT_RECYCLETV].doomednum || mthing->type == mobjinfo[MT_MIXUPBOX].doomednum)
					mobj->flags2 |= MF2_STRONGBOX;

			// Requires you to be in bonus time to activate
			if (mobj->flags & MF_NIGHTSITEM)
				mobj->flags2 |= MF2_STRONGBOX;

			// Pushables bounce and slide coolly with object special flag set
			if (mobj->flags & MF_PUSHABLE)
			{
				mobj->flags2 |= MF2_SLIDEPUSH;
				mobj->flags |= MF_BOUNCE;
			}
		}
	}

	// Generic reverse gravity for individual objects flag.
	if (mthing->options & MTF_OBJECTFLIP)
	{
		mobj->eflags |= MFE_VERTICALFLIP;
		mobj->flags2 |= MF2_OBJECTFLIP;
	}

	mthing->mobj = mobj;
}

void P_SpawnHoopsAndRings(mapthing_t *mthing)
{
	mobj_t *mobj = NULL;
	INT32 r, i;
	fixed_t x, y, z, finalx, finaly, finalz, mthingx, mthingy, mthingz;
	TVector v, *res;
	angle_t closestangle, fa;

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;

	// NiGHTS hoop!
	if (mthing->type == 1705)
	{
		mobj_t *nextmobj = NULL;
		mobj_t *hoopcenter;
		INT16 spewangle;

		mthingx = mthing->x << FRACBITS;
		mthingy = mthing->y << FRACBITS;

		mthingz = mthing->options << FRACBITS;

		hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_HOOPCENTER);

		hoopcenter->spawnpoint = mthing;

		// Screw these damn hoops, I need this thinker.
		//hoopcenter->flags |= MF_NOTHINK;

		mthingz += R_PointInSubsector(mthingx, mthingy)->sector->floorheight;

		hoopcenter->z = mthingz - hoopcenter->height/2;

		P_UnsetThingPosition(hoopcenter);
		hoopcenter->x = mthingx;
		hoopcenter->y = mthingy;
		P_SetThingPosition(hoopcenter);

		// Scale 0-255 to 0-359 =(
		closestangle = FixedAngle(FixedMul((mthing->angle>>8)*FRACUNIT,
			360*(FRACUNIT/256)));

		hoopcenter->movedir = FixedInt(FixedMul((mthing->angle&255)*FRACUNIT,
			360*(FRACUNIT/256)));
		hoopcenter->movecount = FixedInt(AngleFixed(closestangle));

		// For the hoop when it flies away
		hoopcenter->extravalue1 = 32;
		hoopcenter->extravalue2 = 8 * FRACUNIT;

		spewangle = (INT16)hoopcenter->movedir;

		// Create the hoop!
		for (i = 0; i < 32; i++)
		{
			fa = i*(FINEANGLES/32);
			v[0] = FixedMul(FINECOSINE(fa),96*FRACUNIT);
			v[1] = 0;
			v[2] = FixedMul(FINESINE(fa),96*FRACUNIT);
			v[3] = FRACUNIT;

			res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(spewangle*FRACUNIT)));
			M_Memcpy(&v, res, sizeof (v));
			res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
			M_Memcpy(&v, res, sizeof (v));

			finalx = mthingx + v[0];
			finaly = mthingy + v[1];
			finalz = mthingz + v[2];

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOP);

			if (maptol & TOL_XMAS)
				P_SetMobjState(mobj, mobj->info->seestate + (i & 1));

			mobj->z -= mobj->height/2;
			P_SetTarget(&mobj->target, hoopcenter); // Link the sprite to the center.
			mobj->fuse = 0;

			// Link all the sprites in the hoop together
			if (nextmobj)
			{
				mobj->hprev = nextmobj;
				mobj->hprev->hnext = mobj;
			}
			else
				mobj->hprev = mobj->hnext = NULL;

			nextmobj = mobj;
		}

		// Create the collision detectors!
		for (i = 0; i < 16; i++)
		{
			fa = i*FINEANGLES/16;
			v[0] = FixedMul(FINECOSINE(fa),32*FRACUNIT);
			v[1] = 0;
			v[2] = FixedMul(FINESINE(fa),32*FRACUNIT);
			v[3] = FRACUNIT;
			res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(spewangle*FRACUNIT)));
			M_Memcpy(&v, res, sizeof (v));
			res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
			M_Memcpy(&v, res, sizeof (v));

			finalx = mthingx + v[0];
			finaly = mthingy + v[1];
			finalz = mthingz + v[2];

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOPCOLLIDE);
			mobj->z -= mobj->height/2;

			// Link all the collision sprites together.
			mobj->hnext = NULL;
			mobj->hprev = nextmobj;
			mobj->hprev->hnext = mobj;

			nextmobj = mobj;
		}
		// Create the collision detectors!
		for (i = 0; i < 16; i++)
		{
			fa = i*FINEANGLES/16;
			v[0] = FixedMul(FINECOSINE(fa),64*FRACUNIT);
			v[1] = 0;
			v[2] = FixedMul(FINESINE(fa),64*FRACUNIT);
			v[3] = FRACUNIT;
			res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(spewangle*FRACUNIT)));
			M_Memcpy(&v, res, sizeof (v));
			res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
			M_Memcpy(&v, res, sizeof (v));

			finalx = mthingx + v[0];
			finaly = mthingy + v[1];
			finalz = mthingz + v[2];

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOPCOLLIDE);
			mobj->z -= mobj->height/2;

			// Link all the collision sprites together.
			mobj->hnext = NULL;
			mobj->hprev = nextmobj;
			mobj->hprev->hnext = mobj;

			nextmobj = mobj;
		}
		return;
	}
	// CUSTOMIZABLE NiGHTS hoop!
	else if (mthing->type == 1713)
	{
		mobj_t *nextmobj = NULL;
		mobj_t *hoopcenter;
		INT16 spewangle;
		INT32 hoopsize;
		INT32 hoopplacement;

		mthingx = mthing->x << FRACBITS;
		mthingy = mthing->y << FRACBITS;

		// Save our flags!
		mthingz = (mthing->options>>ZSHIFT) << FRACBITS;

		hoopcenter = P_SpawnMobj(mthingx, mthingy, mthingz, MT_HOOPCENTER);
		hoopcenter->spawnpoint = mthing;

		mthingz += R_PointInSubsector(mthingx, mthingy)->sector->floorheight;
		hoopcenter->z = mthingz - hoopcenter->height/2;

		P_UnsetThingPosition(hoopcenter);
		hoopcenter->x = mthingx;
		hoopcenter->y = mthingy;
		P_SetThingPosition(hoopcenter);

		// Scale 0-255 to 0-359 =(
		closestangle = FixedAngle(FixedMul((mthing->angle>>8)*FRACUNIT,
			360*(FRACUNIT/256)));

		hoopcenter->movedir = FixedInt(FixedMul((mthing->angle&255)*FRACUNIT,
			360*(FRACUNIT/256)));
		hoopcenter->movecount = FixedInt(AngleFixed(closestangle));

		spewangle = (INT16)hoopcenter->movedir;

		// Super happy fun time
		// For each flag add 4 fracunits to the size
		// Default (0 flags) is 8 fracunits
		hoopsize = 8 + (4 * (mthing->options & 0xF));
		hoopplacement = hoopsize * (4*FRACUNIT);

		// For the hoop when it flies away
		hoopcenter->extravalue1 = hoopsize;
		hoopcenter->extravalue2 = FixedDiv(hoopplacement, 12*FRACUNIT);

		// Create the hoop!
		for (i = 0; i < hoopsize; i++)
		{
			fa = i*(FINEANGLES/hoopsize);
			v[0] = FixedMul(FINECOSINE(fa), hoopplacement);
			v[1] = 0;
			v[2] = FixedMul(FINESINE(fa), hoopplacement);
			v[3] = FRACUNIT;

			res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(spewangle*FRACUNIT)));
			M_Memcpy(&v, res, sizeof (v));
			res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
			M_Memcpy(&v, res, sizeof (v));

			finalx = mthingx + v[0];
			finaly = mthingy + v[1];
			finalz = mthingz + v[2];

			mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOP);

			if (maptol & TOL_XMAS)
				P_SetMobjState(mobj, mobj->info->seestate + (i & 1));

			mobj->z -= mobj->height/2;
			P_SetTarget(&mobj->target, hoopcenter); // Link the sprite to the center.
			mobj->fuse = 0;

			// Link all the sprites in the hoop together
			if (nextmobj)
			{
				mobj->hprev = nextmobj;
				mobj->hprev->hnext = mobj;
			}
			else
				mobj->hprev = mobj->hnext = NULL;

			nextmobj = mobj;
		}

		// Create the collision detectors!
		// Create them until the size is less than 8
		// But always create at least ONE set of collision detectors
		do
		{
			if (hoopsize >= 32)
				hoopsize -= 16;
			else
				hoopsize /= 2;

			hoopplacement = hoopsize * (4*FRACUNIT);

			for (i = 0; i < hoopsize; i++)
			{
				fa = i*FINEANGLES/hoopsize;
				v[0] = FixedMul(FINECOSINE(fa), hoopplacement);
				v[1] = 0;
				v[2] = FixedMul(FINESINE(fa), hoopplacement);
				v[3] = FRACUNIT;
				res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(spewangle*FRACUNIT)));
				M_Memcpy(&v, res, sizeof (v));
				res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
				M_Memcpy(&v, res, sizeof (v));

				finalx = mthingx + v[0];
				finaly = mthingy + v[1];
				finalz = mthingz + v[2];

				mobj = P_SpawnMobj(finalx, finaly, finalz, MT_HOOPCOLLIDE);
				mobj->z -= mobj->height/2;

				// Link all the collision sprites together.
				mobj->hnext = NULL;
				mobj->hprev = nextmobj;
				mobj->hprev->hnext = mobj;

				nextmobj = mobj;
			}
		} while (hoopsize >= 8);

		return;
	}
	// Wing logo item.
	else if (mthing->type == mobjinfo[MT_NIGHTSWING].doomednum)
	{
		if (mthing->options >> ZSHIFT)
			mthing->z = (INT16)((R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> ZSHIFT) << FRACBITS))>>FRACBITS);
		else
			mthing->z = (INT16)(R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight>>FRACBITS);

		mobj = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS,mthing->z << FRACBITS, MT_NIGHTSWING);
		mobj->spawnpoint = mthing;

		if (G_IsSpecialStage(gamemap) && useNightsSS)
			P_SetMobjState(mobj, mobj->info->meleestate);
		else if (maptol & TOL_XMAS)
			P_SetMobjState(mobj, mobj->info->seestate);

		mobj->angle = FixedAngle(mthing->angle*FRACUNIT);
		mobj->flags |= MF_AMBUSH;
		mthing->mobj = mobj;
	}
	// All manners of rings and coins
	else if (mthing->type == mobjinfo[MT_RING].doomednum || mthing->type == mobjinfo[MT_COIN].doomednum ||
	         mthing->type == mobjinfo[MT_REDTEAMRING].doomednum || mthing->type == mobjinfo[MT_BLUETEAMRING].doomednum)
	{
		mobjtype_t ringthing = MT_RING;

		// No rings in Ultimate!
		if (ultimatemode && !(G_IsSpecialStage(gamemap) || (maptol & TOL_NIGHTS)))
			return;

		// Which ringthing to use
		switch (mthing->type)
		{
			case 1800:
				ringthing = MT_COIN;
				break;
			case 308: // No team rings in non-CTF
				ringthing = (gametype == GT_CTF) ? MT_REDTEAMRING : MT_RING;
				break;
			case 309: // No team rings in non-CTF
				ringthing = (gametype == GT_CTF) ? MT_BLUETEAMRING : MT_RING;
				break;
			default:
#ifdef BLUE_SPHERES
				// Spawn rings as blue spheres in special stages, ala S3+K.
				if (G_IsSpecialStage(gamemap) && useNightsSS)
					ringthing = MT_BLUEBALL;
#endif
				break;
		}

		// Set proper height
		if (mthing->options >> ZSHIFT)
		{
			if (!(mthing->options & MTF_OBJECTFLIP))
				mthing->z = (INT16)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> ZSHIFT) << FRACBITS))>>FRACBITS);
			else
				mthing->z = (INT16)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->ceilingheight - mobjinfo[ringthing].height - ((mthing->options >> ZSHIFT) << FRACBITS))>>FRACBITS);
		}
		else
		{
			if (!(mthing->options & MTF_OBJECTFLIP))
				mthing->z = (INT16)(R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight>>FRACBITS);
			else
				mthing->z = (INT16)((R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->ceilingheight - mobjinfo[ringthing].height)>>FRACBITS);
		}

		if (mthing->options & MTF_AMBUSH) // Special flag for rings
		{
			if (!(mthing->options & MTF_OBJECTFLIP))
				mthing->z += 24;
			else
				mthing->z -= 24;
		}

		mobj = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS,mthing->z << FRACBITS, ringthing);
		mobj->spawnpoint = mthing;

		if (mthing->options & MTF_OBJECTFLIP)
		{
			mobj->eflags |= MFE_VERTICALFLIP;
			mobj->flags2 |= MF2_OBJECTFLIP;
		}

		mobj->angle = FixedAngle(mthing->angle*FRACUNIT);
		mobj->flags |= MF_AMBUSH;
		mthing->mobj = mobj;
	}
	// ***
	// Special placement patterns
	// ***

	// Vertical Rings - Stack of 5 (handles both red and yellow)
	else if (mthing->type == 600 || mthing->type == 601)
	{
		INT32 dist = 64*FRACUNIT;
		mobjtype_t ringthing = MT_RING;
		if (mthing->type == 601)
			dist = 128*FRACUNIT;

		// No rings in Ultimate!
		if (ultimatemode && !(G_IsSpecialStage(gamemap) || (maptol & TOL_NIGHTS)))
			return;

#ifdef BLUE_SPHERES
		// Spawn rings as blue spheres in special stages, ala S3+K.
		if (G_IsSpecialStage(gamemap) && useNightsSS)
			ringthing = MT_BLUEBALL;
#endif

		for (r = 1; r <= 5; r++)
		{
			if (mthing->options >> ZSHIFT)
			{
				if (!(mthing->options & MTF_OBJECTFLIP))
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> ZSHIFT) << FRACBITS)) + dist*r;
				else
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->ceilingheight - mobjinfo[ringthing].height - ((mthing->options >> ZSHIFT) << FRACBITS)) - dist*r;
			}
			else
			{
				if (!(mthing->options & MTF_OBJECTFLIP))
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + dist*r;
				else
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->ceilingheight - mobjinfo[ringthing].height - dist*r;
			}

			mobj = P_SpawnMobj(x, y, z, ringthing);

			if (mthing->options & MTF_OBJECTFLIP)
			{
				mobj->eflags |= MFE_VERTICALFLIP;
				mobj->flags2 |= MF2_OBJECTFLIP;
			}

			mobj->angle = FixedAngle(mthing->angle*FRACUNIT);
			if (mthing->options & MTF_AMBUSH)
				mobj->flags |= MF_AMBUSH;
		}
	}
	// Diagonal rings (handles both types)
	else if (mthing->type == 602 || mthing->type == 603) // Diagonal rings (5)
	{
		angle_t angle = ANGLE_45 * (mthing->angle/45);
		mobjtype_t ringthing = MT_RING;
		INT32 iterations = 5;
		if (mthing->type == 603)
			iterations = 10;

		// No rings in Ultimate!
		if (ultimatemode && !(G_IsSpecialStage(gamemap) || (maptol & TOL_NIGHTS)))
			return;

#ifdef BLUE_SPHERES
		// Spawn rings as blue spheres in special stages, ala S3+K.
		if (G_IsSpecialStage(gamemap) && useNightsSS)
			ringthing = MT_BLUEBALL;
#endif

		angle >>= ANGLETOFINESHIFT;

		for (r = 1; r <= iterations; r++)
		{
			x += FixedMul(64*FRACUNIT, FINECOSINE(angle));
			y += FixedMul(64*FRACUNIT, FINESINE(angle));

			if (mthing->options >> ZSHIFT)
			{
				if (!(mthing->options & MTF_OBJECTFLIP))
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + ((mthing->options >> ZSHIFT) << FRACBITS)) + 64*FRACUNIT*r;
				else
					z = (R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->ceilingheight - mobjinfo[ringthing].height - ((mthing->options >> ZSHIFT) << FRACBITS)) - 64*FRACUNIT*r;
			}
			else
			{
				if (!(mthing->options & MTF_OBJECTFLIP))
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->floorheight + 64*FRACUNIT*r;
				else
					z = R_PointInSubsector(mthing->x << FRACBITS, mthing->y << FRACBITS)->sector->ceilingheight - mobjinfo[ringthing].height - 64*FRACUNIT*r;
			}

			mobj = P_SpawnMobj(x, y, z, ringthing);

			if (mthing->options & MTF_OBJECTFLIP)
			{
				mobj->eflags |= MFE_VERTICALFLIP;
				mobj->flags2 |= MF2_OBJECTFLIP;
			}

			mobj->angle = FixedAngle(mthing->angle*FRACUNIT);
			if (mthing->options & MTF_AMBUSH)
				mobj->flags |= MF_AMBUSH;
		}
	}
	// Rings of items (all six of them)
	else if (mthing->type >= 604 && mthing->type <= 609)
	{
		INT32 numitems = 8;
		INT32 size = 96*FRACUNIT;
		mobjtype_t itemToSpawn = MT_NIGHTSWING;

		if (mthing->type & 1)
		{
			numitems = 16;
			size = 192*FRACUNIT;
		}

		mthingx = mthing->x << FRACBITS;
		mthingy = mthing->y << FRACBITS;

		if (mthing->options >> ZSHIFT)
			mthingz = (R_PointInSubsector(x, y)->sector->floorheight + ((mthing->options >> ZSHIFT) << FRACBITS));
		else
			mthingz = R_PointInSubsector(mthingx, mthingy)->sector->floorheight;

		closestangle = FixedAngle(mthing->angle*FRACUNIT);

		// Create the hoop!
		for (i = 0; i < numitems; i++)
		{
			switch (mthing->type)
			{
				case 604:
				case 605:
					itemToSpawn = MT_RING;
					break;
				case 608:
				case 609:
					itemToSpawn = (i & 1) ? MT_NIGHTSWING : MT_RING;
					break;
				case 606:
				case 607:
					itemToSpawn = MT_NIGHTSWING;
					break;
				default:
					break;
			}

			// No rings in Ultimate!
			if (itemToSpawn == MT_RING)
			{
				if (ultimatemode && !(G_IsSpecialStage(gamemap) || (maptol & TOL_NIGHTS)))
					continue;

#ifdef BLUE_SPHERES
				// Spawn rings as blue spheres in special stages, ala S3+K.
				if (G_IsSpecialStage(gamemap) && useNightsSS)
					itemToSpawn = MT_BLUEBALL;
#endif
			}

			fa = i*FINEANGLES/numitems;
			v[0] = FixedMul(FINECOSINE(fa),size);
			v[1] = 0;
			v[2] = FixedMul(FINESINE(fa),size);
			v[3] = FRACUNIT;

			res = VectorMatrixMultiply(v, *RotateZMatrix(closestangle));
			M_Memcpy(&v, res, sizeof (v));

			finalx = mthingx + v[0];
			finaly = mthingy + v[1];
			finalz = mthingz + v[2];

			mobj = P_SpawnMobj(finalx, finaly, finalz, itemToSpawn);
			mobj->z -= mobj->height/2;

			if (itemToSpawn == MT_NIGHTSWING)
			{
				if (G_IsSpecialStage(gamemap) && useNightsSS)
					P_SetMobjState(mobj, mobj->info->meleestate);
				else if ((maptol & TOL_XMAS))
					P_SetMobjState(mobj, mobj->info->seestate);
			}
		}
		return;
	}
}

//
// P_CheckMissileSpawn
// Moves the missile forward a bit and possibly explodes it right there.
//
boolean P_CheckMissileSpawn(mobj_t *th)
{
	// move a little forward so an angle can be computed if it immediately explodes
	if (!(th->flags & MF_GRENADEBOUNCE)) // hack: bad! should be a flag.
	{
		th->x += th->momx>>1;
		th->y += th->momy>>1;
		th->z += th->momz>>1;
	}

	if (!P_TryMove(th, th->x, th->y, true))
	{
		P_ExplodeMissile(th);
		return false;
	}
	return true;
}

//
// P_SpawnXYZMissile
//
// Spawns missile at specific coords
//
mobj_t *P_SpawnXYZMissile(mobj_t *source, mobj_t *dest, mobjtype_t type,
	fixed_t x, fixed_t y, fixed_t z)
{
	mobj_t *th;
	angle_t an;
	INT32 dist;
	fixed_t speed;

	I_Assert(source != NULL);
	I_Assert(dest != NULL);

	if (source->eflags & MFE_VERTICALFLIP)
		z -= FixedMul(mobjinfo[type].height, source->scale);

	th = P_SpawnMobj(x, y, z, type);

	if (source->eflags & MFE_VERTICALFLIP)
		th->flags2 |= MF2_OBJECTFLIP;

	th->destscale = source->scale;
	P_SetScale(th, source->scale);

	speed = FixedMul(th->info->speed, th->scale);

	if (speed == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "P_SpawnXYZMissile - projectile has 0 speed! (mobj type %d)\n", type);
		speed = mobjinfo[MT_ROCKET].speed;
	}

	if (th->info->seesound)
		S_StartSound(th, th->info->seesound);

	P_SetTarget(&th->target, source); // where it came from
	an = R_PointToAngle2(x, y, dest->x, dest->y);

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = FixedMul(speed, FINECOSINE(an));
	th->momy = FixedMul(speed, FINESINE(an));

	dist = P_AproxDistance(dest->x - x, dest->y - y);
	dist = dist / speed;

	if (dist < 1)
		dist = 1;

	th->momz = (dest->z - z) / dist;

	if (th->flags & MF_MISSILE)
		dist = P_CheckMissileSpawn(th);
	else
		dist = 1;

	return dist ? th : NULL;
}

//
// P_SpawnAlteredDirectionMissile
//
// Spawns a missile with same source as spawning missile yet an altered direction
//
mobj_t *P_SpawnAlteredDirectionMissile(mobj_t *source, mobjtype_t type, fixed_t x, fixed_t y, fixed_t z, INT32 shiftingAngle)
{
	mobj_t *th;
	angle_t an;
	fixed_t dist, speed;

	I_Assert(source != NULL);

	if (!(source->target) || !(source->flags & MF_MISSILE))
		return NULL;

	if (source->eflags & MFE_VERTICALFLIP)
		z -= FixedMul(mobjinfo[type].height, source->scale);

	th = P_SpawnMobj(x, y, z, type);

	if (source->eflags & MFE_VERTICALFLIP)
		th->flags2 |= MF2_OBJECTFLIP;

	th->destscale = source->scale;
	P_SetScale(th, source->scale);

	speed = FixedMul(th->info->speed, th->scale);

	if (speed == 0) // Backwards compatibility with 1.09.2
	{
		CONS_Printf("P_SpawnAlteredDirectionMissile - projectile has 0 speed! (mobj type %d)\nPlease update this SOC.", type);
		speed = mobjinfo[MT_ROCKET].speed;
	}

	if (th->info->seesound)
		S_StartSound(th, th->info->seesound);

	P_SetTarget(&th->target, source->target); // where it came from
	an = R_PointToAngle2(x, y, x+source->momx*16, y+source->momy*16) + (ANG1*shiftingAngle);

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = FixedMul(speed, FINECOSINE(an));
	th->momy = FixedMul(speed, FINESINE(an));

	dist = P_AproxDistance(source->momx*800, source->momy*800);
	dist = dist / speed;

	if (dist < 1)
		dist = 1;

	th->momz = (source->momz*800) / dist;

	if (th->flags & MF_MISSILE)
	{
		dist = P_CheckMissileSpawn(th);
		th->x -= th->momx>>1;
		th->y -= th->momy>>1;
		th->z -= th->momz>>1;
	}
	else
		dist = 1;

	return dist ? th : NULL;
}

//
// P_SpawnPointMissile
//
// Spawns a missile at specific coords
//
mobj_t *P_SpawnPointMissile(mobj_t *source, fixed_t xa, fixed_t ya, fixed_t za, mobjtype_t type,
	fixed_t x, fixed_t y, fixed_t z)
{
	mobj_t *th;
	angle_t an;
	fixed_t dist, speed;

	I_Assert(source != NULL);

	if (source->eflags & MFE_VERTICALFLIP)
		z -= FixedMul(mobjinfo[type].height, source->scale);

	th = P_SpawnMobj(x, y, z, type);

	if (source->eflags & MFE_VERTICALFLIP)
		th->flags2 |= MF2_OBJECTFLIP;

	th->destscale = source->scale;
	P_SetScale(th, source->scale);

	speed = FixedMul(th->info->speed, th->scale);

	if (speed == 0) // Backwards compatibility with 1.09.2
	{
		CONS_Printf("P_SpawnPointMissile - projectile has 0 speed! (mobj type %d)\nPlease update this SOC.", type);
		speed = mobjinfo[MT_ROCKET].speed;
	}

	if (th->info->seesound)
		S_StartSound(th, th->info->seesound);

	P_SetTarget(&th->target, source); // where it came from
	an = R_PointToAngle2(x, y, xa, ya);

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = FixedMul(speed, FINECOSINE(an));
	th->momy = FixedMul(speed, FINESINE(an));

	dist = P_AproxDistance(xa - x, ya - y);
	dist = dist / speed;

	if (dist < 1)
		dist = 1;

	th->momz = (za - z) / dist;

	if (th->flags & MF_MISSILE)
		dist = P_CheckMissileSpawn(th);
	else
		dist = 1;

	return dist ? th : NULL;
}

//
// P_SpawnMissile
//
mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type)
{
	mobj_t *th;
	angle_t an;
	INT32 dist;
	fixed_t z;
	const fixed_t gsf = (fixed_t)6;
	fixed_t speed;

	I_Assert(source != NULL);
	I_Assert(dest != NULL);
	if (source->type == MT_JETTGUNNER)
	{
		if (source->eflags & MFE_VERTICALFLIP)
			z = source->z + source->height - FixedMul(4*FRACUNIT, source->scale);
		else
			z = source->z + FixedMul(4*FRACUNIT, source->scale);
	}
	else
		z = source->z + source->height/2;

	if (source->eflags & MFE_VERTICALFLIP)
		z -= FixedMul(mobjinfo[type].height, source->scale);

	th = P_SpawnMobj(source->x, source->y, z, type);

	if (source->eflags & MFE_VERTICALFLIP)
		th->flags2 |= MF2_OBJECTFLIP;

	th->destscale = source->scale;
	P_SetScale(th, source->scale);

	if (source->type == MT_METALSONIC_BATTLE && source->health < 4)
		speed = FixedMul(FixedMul(th->info->speed, 3*FRACUNIT/2), th->scale);
	else
		speed = FixedMul(th->info->speed, th->scale);

	if (speed == 0)
	{
		CONS_Debug(DBG_GAMELOGIC, "P_SpawnMissile - projectile has 0 speed! (mobj type %d)\n", type);
		speed = FixedMul(mobjinfo[MT_TURRETLASER].speed, th->scale);
	}

	if (th->info->seesound)
		S_StartSound(source, th->info->seesound);

	P_SetTarget(&th->target, source); // where it came from

	if (type == MT_TURRETLASER || type == MT_ENERGYBALL) // More accurate!
		an = R_PointToAngle2(source->x, source->y,
			dest->x + (dest->momx*gsf),
			dest->y + (dest->momy*gsf));
	else
		an = R_PointToAngle2(source->x, source->y, dest->x, dest->y);

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = FixedMul(speed, FINECOSINE(an));
	th->momy = FixedMul(speed, FINESINE(an));

	if (type == MT_TURRETLASER || type == MT_ENERGYBALL) // More accurate!
		dist = P_AproxDistance(dest->x+(dest->momx*gsf) - source->x, dest->y+(dest->momy*gsf) - source->y);
	else
		dist = P_AproxDistance(dest->x - source->x, dest->y - source->y);

	dist = dist / speed;

	if (dist < 1)
		dist = 1;

	if (type == MT_TURRETLASER || type == MT_ENERGYBALL) // More accurate!
		th->momz = (dest->z + (dest->momz*gsf) - z) / dist;
	else
		th->momz = (dest->z - z) / dist;

	if (th->flags & MF_MISSILE)
		dist = P_CheckMissileSpawn(th);
	else
		dist = 1;
	return dist ? th : NULL;
}

//
// P_ColorTeamMissile
// Colors a player's ring based on their team
//
void P_ColorTeamMissile(mobj_t *missile, player_t *source)
{
	if (G_GametypeHasTeams())
	{
		if (source->ctfteam == 2)
			missile->color = skincolor_bluering;
		else if (source->ctfteam == 1)
			missile->color = skincolor_redring;
	}
	/*
	else
		missile->color = player->mo->color; //copy color
	*/
}

//
// P_SPMAngle
// Fires missile at angle from what is presumably a player
//
mobj_t *P_SPMAngle(mobj_t *source, mobjtype_t type, angle_t angle, UINT8 allowaim, UINT32 flags2)
{
	mobj_t *th;
	angle_t an;
	fixed_t x, y, z, slope = 0;

	// angle at which you fire, is player angle
	an = angle;

	if (allowaim) // aiming allowed?
		slope = AIMINGTOSLOPE(source->player->aiming);

	x = source->x;
	y = source->y;

	if (source->eflags & MFE_VERTICALFLIP)
		z = source->z + 2*source->height/3 - FixedMul(mobjinfo[type].height, source->scale);
	else
		z = source->z + source->height/3;

	th = P_SpawnMobj(x, y, z, type);

	if (source->eflags & MFE_VERTICALFLIP)
		th->flags2 |= MF2_OBJECTFLIP;

	th->destscale = source->scale;
	P_SetScale(th, source->scale);

	th->flags2 |= flags2;

	// The rail ring has no unique thrown object, so we must do this.
	if (th->info->seesound && !(th->flags2 & MF2_RAILRING))
		S_StartSound(source, th->info->seesound);

	P_SetTarget(&th->target, source);

	th->angle = an;
	th->momx = FixedMul(th->info->speed, FINECOSINE(an>>ANGLETOFINESHIFT));
	th->momy = FixedMul(th->info->speed, FINESINE(an>>ANGLETOFINESHIFT));

	if (allowaim)
	{
		th->momx = FixedMul(th->momx,FINECOSINE(source->player->aiming>>ANGLETOFINESHIFT));
		th->momy = FixedMul(th->momy,FINECOSINE(source->player->aiming>>ANGLETOFINESHIFT));
	}

	th->momz = FixedMul(th->info->speed, slope);

	//scaling done here so it doesn't clutter up the code above
	th->momx = FixedMul(th->momx, th->scale);
	th->momy = FixedMul(th->momy, th->scale);
	th->momz = FixedMul(th->momz, th->scale);

	slope = P_CheckMissileSpawn(th);

	return slope ? th : NULL;
}

//
// P_FlashPal
// Flashes a player's palette.  ARMAGEDDON BLASTS!
//
void P_FlashPal(player_t *pl, UINT16 type, UINT16 duration)
{
	if (!pl)
		return;
	pl->flashcount = duration;
	pl->flashpal = type;
}
