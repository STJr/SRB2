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
#include "r_skins.h"
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
#include "p_slopes.h"
#include "f_finale.h"
#include "m_cond.h"

static CV_PossibleValue_t CV_BobSpeed[] = {{0, "MIN"}, {4*FRACUNIT, "MAX"}, {0, NULL}};
consvar_t cv_movebob = {"movebob", "1.0", CV_FLOAT|CV_SAVE, CV_BobSpeed, NULL, 0, NULL, NULL, 0, 0, NULL};

#ifdef WALLSPLATS
consvar_t cv_splats = {"splats", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
#endif

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
		astate = &states[ac->statenum];
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
// P_SetupStateAnimation
//
FUNCINLINE static ATTRINLINE void P_SetupStateAnimation(mobj_t *mobj, state_t *st)
{
	INT32 animlength = (mobj->sprite == SPR_PLAY && mobj->skin)
		? (INT32)(((skin_t *)mobj->skin)->sprites[mobj->sprite2].numframes) - 1
		: st->var1;

	if (!(st->frame & FF_ANIMATE))
		return;

	if (animlength <= 0 || st->var2 == 0)
	{
		mobj->frame &= ~FF_ANIMATE;
		return; // Crash/stupidity prevention
	}

	mobj->anim_duration = (UINT16)st->var2;

	if (st->frame & FF_GLOBALANIM)
	{
		// Attempt to account for the pre-ticker for objects spawned on load
		if (!leveltime) return;

		mobj->anim_duration -= (leveltime + 2) % st->var2;            // Duration synced to timer
		mobj->frame += ((leveltime + 2) / st->var2) % (animlength + 1); // Frame synced to timer (duration taken into account)
	}
	else if (st->frame & FF_RANDOMANIM)
	{
		mobj->frame += P_RandomKey(animlength + 1);     // Random starting frame
		mobj->anim_duration -= P_RandomKey(st->var2); // Random duration for first frame
	}
}

//
// P_CycleStateAnimation
//
FUNCINLINE static ATTRINLINE void P_CycleStateAnimation(mobj_t *mobj)
{
	// var2 determines delay between animation frames
	if (!(mobj->frame & FF_ANIMATE) || --mobj->anim_duration != 0)
		return;

	mobj->anim_duration = (UINT16)mobj->state->var2;

	if (mobj->sprite != SPR_PLAY)
	{
		// compare the current sprite frame to the one we started from
		// if more than var1 away from it, swap back to the original
		// else just advance by one
		if (((++mobj->frame) & FF_FRAMEMASK) - (mobj->state->frame & FF_FRAMEMASK) > (UINT32)mobj->state->var1)
			mobj->frame = (mobj->state->frame & FF_FRAMEMASK) | (mobj->frame & ~FF_FRAMEMASK);

		return;
	}

	// sprite2 version of above
	if (mobj->skin && (((++mobj->frame) & FF_FRAMEMASK) >= (UINT32)(((skin_t *)mobj->skin)->sprites[mobj->sprite2].numframes)))
		mobj->frame &= ~FF_FRAMEMASK;
}

//
// P_CycleMobjState
//
static void P_CycleMobjState(mobj_t *mobj)
{
	// state animations
	P_CycleStateAnimation(mobj);

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
	// state animations
	P_CycleStateAnimation(mobj);

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

	// Catch falling for nojumpspin
	if ((state == S_PLAY_JUMP) && (player->charflags & SF_NOJUMPSPIN) && (P_MobjFlip(mobj)*mobj->momz < 0))
		return P_SetPlayerMobjState(mobj, S_PLAY_FALL);

	// Catch swimming versus flying
	if (state == S_PLAY_FLY && player->mo->eflags & MFE_UNDERWATER)
		return P_SetPlayerMobjState(player->mo, S_PLAY_SWIM);
	else if (state == S_PLAY_SWIM && !(player->mo->eflags & MFE_UNDERWATER))
		return P_SetPlayerMobjState(player->mo, S_PLAY_FLY);

	// Catch SF_NOSUPERSPIN jumps for Supers
	if (player->powers[pw_super] && (player->charflags & SF_NOSUPERSPIN))
	{
		if (state == S_PLAY_JUMP)
		{
			if (player->mo->state-states == S_PLAY_WALK)
				return P_SetPlayerMobjState(mobj, S_PLAY_FLOAT);
			return true;
		}
		else if (player->mo->state-states == S_PLAY_FLOAT && state == S_PLAY_STND)
			return true;
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
	switch(state)
	{
	case S_PLAY_STND:
	case S_PLAY_WAIT:
	case S_PLAY_NIGHTS_STAND:
		player->panim = PA_IDLE;
		break;
	case S_PLAY_EDGE:
		player->panim = PA_EDGE;
		break;
	case S_PLAY_WALK:
	case S_PLAY_SKID:
	case S_PLAY_FLOAT:
		player->panim = PA_WALK;
		break;
	case S_PLAY_RUN:
	case S_PLAY_FLOAT_RUN:
		player->panim = PA_RUN;
		break;
	case S_PLAY_DASH:
		player->panim = PA_DASH;
		break;
	case S_PLAY_PAIN:
	case S_PLAY_STUN:
		player->panim = PA_PAIN;
		break;
	case S_PLAY_ROLL:
	//case S_PLAY_SPINDASH: -- everyone can ROLL thanks to zoom tubes...
	case S_PLAY_NIGHTS_ATTACK:
		player->panim = PA_ROLL;
		break;
	case S_PLAY_JUMP:
		player->panim = PA_JUMP;
		break;
	case S_PLAY_SPRING:
		player->panim = PA_SPRING;
		break;
	case S_PLAY_FALL:
	case S_PLAY_NIGHTS_FLOAT:
		player->panim = PA_FALL;
		break;
	case S_PLAY_FLY:
	case S_PLAY_FLY_TIRED:
	case S_PLAY_SWIM:
	case S_PLAY_GLIDE:
	case S_PLAY_BOUNCE:
	case S_PLAY_BOUNCE_LANDING:
	case S_PLAY_TWINSPIN:
		player->panim = PA_ABILITY;
		break;
	case S_PLAY_SPINDASH: // ...but the act of SPINDASHING is charability2 specific.
	case S_PLAY_FIRE:
	case S_PLAY_FIRE_FINISH:
	case S_PLAY_MELEE:
	case S_PLAY_MELEE_FINISH:
	case S_PLAY_MELEE_LANDING:
		player->panim = PA_ABILITY2;
		break;
	case S_PLAY_RIDE:
		player->panim = PA_RIDE;
		break;
	default:
		player->panim = PA_ETC;
		break;
	}

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
		if (state == S_PLAY_STND && player->powers[pw_super] && skins[player->skin].sprites[SPR2_WAIT|FF_SPR2SUPER].numframes == 0) // if no super wait, don't wait at all
			mobj->tics = -1;
		else if (player->panim == PA_EDGE && (player->charflags & SF_FASTEDGE))
			mobj->tics = 2;
		else if (!(disableSpeedAdjust || player->charflags & SF_NOSPEEDADJUST))
		{
			fixed_t speed;// = FixedDiv(player->speed, FixedMul(mobj->scale, player->mo->movefactor));
			if (player->panim == PA_FALL)
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
			else if (player->panim == PA_ABILITY2 && player->charability2 == CA2_SPINDASH)
			{
				fixed_t step = (player->maxdash - player->mindash)/4;
				speed = (player->dashspeed - player->mindash);
				if (speed > 3*step)
					mobj->tics = 1;
				else if (speed > step)
					mobj->tics = 2;
				else
					mobj->tics = 3;
			}
			else
			{
				speed = FixedDiv(player->speed, FixedMul(mobj->scale, player->mo->movefactor));
				if (player->panim == PA_ROLL || player->panim == PA_JUMP)
				{
					if (speed > 16<<FRACBITS)
						mobj->tics = 1;
					else
						mobj->tics = 2;
				}
				else if (P_IsObjectOnGround(mobj) || ((player->charability == CA_FLOAT || player->charability == CA_SLOWFALL) && player->secondjump == 1) || player->powers[pw_super]) // Only if on the ground or superflying.
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
					else if ((player->panim == PA_RUN) || (player->panim == PA_DASH))
					{
						if (speed > 52<<FRACBITS)
							mobj->tics = 1;
						else
							mobj->tics = 2;
					}
				}
			}
		}

		// Player animations
		if (st->sprite == SPR_PLAY)
		{
			skin_t *skin = ((skin_t *)mobj->skin);
			UINT16 frame = (mobj->frame & FF_FRAMEMASK)+1;
			UINT8 numframes, spr2;

			if (skin)
			{
				spr2 = P_GetSkinSprite2(skin, (((player->powers[pw_super]) ? FF_SPR2SUPER : 0)|st->frame) & FF_FRAMEMASK, mobj->player);
				numframes = skin->sprites[spr2].numframes;
			}
			else
			{
				spr2 = 0;
				frame = 0;
				numframes = 0;
			}

			if (mobj->sprite != SPR_PLAY)
			{
				mobj->sprite = SPR_PLAY;
				frame = 0;
			}
			else if (mobj->sprite2 != spr2)
			{
				if ((st->frame & FF_SPR2MIDSTART) && numframes && P_RandomChance(FRACUNIT/2))
					frame = numframes/2;
				else
					frame = 0;
			}

			if (frame >= numframes)
			{
				if (st->frame & FF_SPR2ENDSTATE) // no frame advancement
				{
					if (st->var1 == mobj->state-states)
						frame--;
					else
					{
						if (mobj->frame & FF_FRAMEMASK)
							mobj->frame--;
						return P_SetPlayerMobjState(mobj, st->var1);
					}
				}
				else
					frame = 0;
			}

			mobj->sprite2 = spr2;
			mobj->frame = frame|(st->frame&~FF_FRAMEMASK);
			if (mobj->color >= FIRSTSUPERCOLOR && mobj->color < numskincolors) // Super colours? Super bright!
				mobj->frame |= FF_FULLBRIGHT;
		}
		// Regular sprites
		else
		{
			mobj->sprite = st->sprite;
			mobj->frame = st->frame;
		}

		P_SetupStateAnimation(mobj, st);

		// Modified handling.
		// Call action functions when the state is set

		if (st->action.acp1)
		{
			var1 = st->var1;
			var2 = st->var2;
			astate = st;
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

		// Player animations
		if (st->sprite == SPR_PLAY)
		{
			skin_t *skin = ((skin_t *)mobj->skin);
			UINT16 frame = (mobj->frame & FF_FRAMEMASK)+1;
			UINT8 numframes, spr2;

			if (skin)
			{
				spr2 = P_GetSkinSprite2(skin, st->frame & FF_FRAMEMASK, mobj->player);
				numframes = skin->sprites[spr2].numframes;
			}
			else
			{
				spr2 = 0;
				frame = 0;
				numframes = 0;
			}

			if (mobj->sprite != SPR_PLAY)
			{
				mobj->sprite = SPR_PLAY;
				frame = 0;
			}
			else if (mobj->sprite2 != spr2)
			{
				if ((st->frame & FF_SPR2MIDSTART) && numframes && P_RandomChance(FRACUNIT/2))
					frame = numframes/2;
				else
					frame = 0;
			}

			if (frame >= numframes)
			{
				if (st->frame & FF_SPR2ENDSTATE) // no frame advancement
				{
					if (st->var1 == mobj->state-states)
						frame--;
					else
					{
						if (mobj->frame & FF_FRAMEMASK)
							mobj->frame--;
						return P_SetMobjState(mobj, st->var1);
					}
				}
				else
					frame = 0;
			}

			mobj->sprite2 = spr2;
			mobj->frame = frame|(st->frame&~FF_FRAMEMASK);
		}
		// Regular sprites
		else
		{
			mobj->sprite = st->sprite;
			mobj->frame = st->frame;
		}

		P_SetupStateAnimation(mobj, st);

		// Modified handling.
		// Call action functions when the state is set

		if (st->action.acp1)
		{
			var1 = st->var1;
			var2 = st->var2;
			astate = st;
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
	P_SetupStateAnimation(mobj, st);

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
	P_SetupStateAnimation((mobj_t*)mobj, st);

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

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

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

			spawnpoints[j]->threshold = emeraldspawndelay + P_RandomByte() * (TICRATE/5);
			break;
		}
	}

	emeraldspawndelay = 0;
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
		P_RadiusAttack(mo, mo, 96*FRACUNIT, 0);

		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx += (P_RandomByte() % 32) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy += (P_RandomByte() % 32) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_pop);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx += (P_RandomByte() % 64) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy -= (P_RandomByte() % 64) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_dmpain);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx -= (P_RandomByte() % 128) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy += (P_RandomByte() % 128) * FixedMul(FRACUNIT/8, explodemo->scale);
		S_StartSound(explodemo, sfx_pop);
		explodemo = P_SpawnMobj(mo->x, mo->y, mo->z, MT_EXPLODE);
		P_SetScale(explodemo, mo->scale);
		explodemo->destscale = mo->destscale;
		explodemo->momx -= (P_RandomByte() % 96) * FixedMul(FRACUNIT/8, explodemo->scale);
		explodemo->momy -= (P_RandomByte() % 96) * FixedMul(FRACUNIT/8, explodemo->scale);
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
	fixed_t topheight, bottomheight;
	if (!(rover->flags & FF_EXISTS))
		return false;

	if ((((rover->flags & FF_BLOCKPLAYER) && mobj->player)
		|| ((rover->flags & FF_BLOCKOTHERS) && !mobj->player)))
		return false;

	topheight    = P_GetFFloorTopZAt   (rover, mobj->x, mobj->y);
	bottomheight = P_GetFFloorBottomZAt(rover, mobj->x, mobj->y);

	if (mobj->z > topheight)
		return false;

	if (mobj->z + mobj->height < bottomheight)
		return false;

	return true;
}

// P_GetFloorZ (and its ceiling counterpart)
// Gets the floor height (or ceiling height) of the mobj's contact point in sector, assuming object's center if moved to [x, y]
// If line is supplied, it's a divider line on the sector. Set it to NULL if you're not checking for collision with a line
// Supply boundsec ONLY when checking for specials! It should be the "in-level" sector, and sector the control sector (if separate).
// If set, then this function will iterate through boundsec's linedefs to find the highest contact point on the slope. Non-special-checking
// usage will handle that later.
static fixed_t HighestOnLine(fixed_t radius, fixed_t x, fixed_t y, line_t *line, pslope_t *slope, boolean actuallylowest)
{
	// Alright, so we're sitting on a line that contains our slope sector, and need to figure out the highest point we're touching...
	// The solution is simple! Get the line's vertices, and pull each one in along its line until it touches the object's bounding box
	// (assuming it isn't already inside), then test each point's slope Z and return the higher of the two.
	vertex_t v1, v2;
	v1.x = line->v1->x;
	v1.y = line->v1->y;
	v2.x = line->v2->x;
	v2.y = line->v2->y;

	/*CONS_Printf("BEFORE: v1 = %f %f %f\n",
				FIXED_TO_FLOAT(v1.x),
				FIXED_TO_FLOAT(v1.y),
				FIXED_TO_FLOAT(P_GetSlopeZAt(slope, v1.x, v1.y))
				);
	CONS_Printf("        v2 = %f %f %f\n",
				FIXED_TO_FLOAT(v2.x),
				FIXED_TO_FLOAT(v2.y),
				FIXED_TO_FLOAT(P_GetSlopeZAt(slope, v2.x, v2.y))
				);*/

	if (abs(v1.x-x) > radius) {
		// v1's x is out of range, so rein it in
		fixed_t diff = abs(v1.x-x) - radius;

		if (v1.x < x) { // Moving right
			v1.x += diff;
			v1.y += FixedMul(diff, FixedDiv(line->dy, line->dx));
		} else { // Moving left
			v1.x -= diff;
			v1.y -= FixedMul(diff, FixedDiv(line->dy, line->dx));
		}
	}

	if (abs(v1.y-y) > radius) {
		// v1's y is out of range, so rein it in
		fixed_t diff = abs(v1.y-y) - radius;

		if (v1.y < y) { // Moving up
			v1.y += diff;
			v1.x += FixedMul(diff, FixedDiv(line->dx, line->dy));
		} else { // Moving down
			v1.y -= diff;
			v1.x -= FixedMul(diff, FixedDiv(line->dx, line->dy));
		}
	}

	if (abs(v2.x-x) > radius) {
		// v1's x is out of range, so rein it in
		fixed_t diff = abs(v2.x-x) - radius;

		if (v2.x < x) { // Moving right
			v2.x += diff;
			v2.y += FixedMul(diff, FixedDiv(line->dy, line->dx));
		} else { // Moving left
			v2.x -= diff;
			v2.y -= FixedMul(diff, FixedDiv(line->dy, line->dx));
		}
	}

	if (abs(v2.y-y) > radius) {
		// v2's y is out of range, so rein it in
		fixed_t diff = abs(v2.y-y) - radius;

		if (v2.y < y) { // Moving up
			v2.y += diff;
			v2.x += FixedMul(diff, FixedDiv(line->dx, line->dy));
		} else { // Moving down
			v2.y -= diff;
			v2.x -= FixedMul(diff, FixedDiv(line->dx, line->dy));
		}
	}

	/*CONS_Printf("AFTER:  v1 = %f %f %f\n",
				FIXED_TO_FLOAT(v1.x),
				FIXED_TO_FLOAT(v1.y),
				FIXED_TO_FLOAT(P_GetSlopeZAt(slope, v1.x, v1.y))
				);
	CONS_Printf("        v2 = %f %f %f\n",
				FIXED_TO_FLOAT(v2.x),
				FIXED_TO_FLOAT(v2.y),
				FIXED_TO_FLOAT(P_GetSlopeZAt(slope, v2.x, v2.y))
				);*/

	// Return the higher of the two points
	if (actuallylowest)
		return min(
			P_GetSlopeZAt(slope, v1.x, v1.y),
			P_GetSlopeZAt(slope, v2.x, v2.y)
		);
	else
		return max(
			P_GetSlopeZAt(slope, v1.x, v1.y),
			P_GetSlopeZAt(slope, v2.x, v2.y)
		);
}

fixed_t P_MobjFloorZ(mobj_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect)
{
	I_Assert(mobj != NULL);
	I_Assert(sector != NULL);

	if (sector->f_slope) {
		fixed_t testx, testy;
		pslope_t *slope = sector->f_slope;

		// Get the corner of the object that should be the highest on the slope
		if (slope->d.x < 0)
			testx = mobj->radius;
		else
			testx = -mobj->radius;

		if (slope->d.y < 0)
			testy = mobj->radius;
		else
			testy = -mobj->radius;

		if ((slope->zdelta > 0) ^ !!(lowest)) {
			testx = -testx;
			testy = -testy;
		}

		testx += x;
		testy += y;

		// If the highest point is in the sector, then we have it easy! Just get the Z at that point
		if (R_PointInSubsector(testx, testy)->sector == (boundsec ? boundsec : sector))
			return P_GetSlopeZAt(slope, testx, testy);

		// If boundsec is set, we're looking for specials. In that case, iterate over every line in this sector to find the TRUE highest/lowest point
		if (perfect) {
			size_t i;
			line_t *ld;
			fixed_t bbox[4];
			fixed_t finalheight;

			if (lowest)
				finalheight = INT32_MAX;
			else
				finalheight = INT32_MIN;

			bbox[BOXLEFT] = x-mobj->radius;
			bbox[BOXRIGHT] = x+mobj->radius;
			bbox[BOXTOP] = y+mobj->radius;
			bbox[BOXBOTTOM] = y-mobj->radius;
			for (i = 0; i < boundsec->linecount; i++) {
				ld = boundsec->lines[i];

				if (bbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || bbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
				|| bbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || bbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
					continue;

				if (P_BoxOnLineSide(bbox, ld) != -1)
					continue;

				if (lowest)
					finalheight = min(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, true));
				else
					finalheight = max(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, false));
			}

			return finalheight;
		}

		// If we're just testing for base sector location (no collision line), just go for the center's spot...
		// It'll get fixed when we test for collision anyway, and the final result can't be lower than this
		if (line == NULL)
			return P_GetSlopeZAt(slope, x, y);

		return HighestOnLine(mobj->radius, x, y, line, slope, lowest);
	} else // Well, that makes it easy. Just get the floor height
		return sector->floorheight;
}

fixed_t P_MobjCeilingZ(mobj_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect)
{
	I_Assert(mobj != NULL);
	I_Assert(sector != NULL);

	if (sector->c_slope) {
		fixed_t testx, testy;
		pslope_t *slope = sector->c_slope;

		// Get the corner of the object that should be the highest on the slope
		if (slope->d.x < 0)
			testx = mobj->radius;
		else
			testx = -mobj->radius;

		if (slope->d.y < 0)
			testy = mobj->radius;
		else
			testy = -mobj->radius;

		if ((slope->zdelta > 0) ^ !!(lowest)) {
			testx = -testx;
			testy = -testy;
		}

		testx += x;
		testy += y;

		// If the highest point is in the sector, then we have it easy! Just get the Z at that point
		if (R_PointInSubsector(testx, testy)->sector == (boundsec ? boundsec : sector))
			return P_GetSlopeZAt(slope, testx, testy);

		// If boundsec is set, we're looking for specials. In that case, iterate over every line in this sector to find the TRUE highest/lowest point
		if (perfect) {
			size_t i;
			line_t *ld;
			fixed_t bbox[4];
			fixed_t finalheight;

			if (lowest)
				finalheight = INT32_MAX;
			else
				finalheight = INT32_MIN;

			bbox[BOXLEFT] = x-mobj->radius;
			bbox[BOXRIGHT] = x+mobj->radius;
			bbox[BOXTOP] = y+mobj->radius;
			bbox[BOXBOTTOM] = y-mobj->radius;
			for (i = 0; i < boundsec->linecount; i++) {
				ld = boundsec->lines[i];

				if (bbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || bbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
				|| bbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || bbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
					continue;

				if (P_BoxOnLineSide(bbox, ld) != -1)
					continue;

				if (lowest)
					finalheight = min(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, true));
				else
					finalheight = max(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, false));
			}

			return finalheight;
		}

		// If we're just testing for base sector location (no collision line), just go for the center's spot...
		// It'll get fixed when we test for collision anyway, and the final result can't be lower than this
		if (line == NULL)
			return P_GetSlopeZAt(slope, x, y);

		return HighestOnLine(mobj->radius, x, y, line, slope, lowest);
	} else // Well, that makes it easy. Just get the ceiling height
		return sector->ceilingheight;
}

// Now do the same as all above, but for cameras because apparently cameras are special?
fixed_t P_CameraFloorZ(camera_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect)
{
	I_Assert(mobj != NULL);
	I_Assert(sector != NULL);

	if (sector->f_slope) {
		fixed_t testx, testy;
		pslope_t *slope = sector->f_slope;

		// Get the corner of the object that should be the highest on the slope
		if (slope->d.x < 0)
			testx = mobj->radius;
		else
			testx = -mobj->radius;

		if (slope->d.y < 0)
			testy = mobj->radius;
		else
			testy = -mobj->radius;

		if ((slope->zdelta > 0) ^ !!(lowest)) {
			testx = -testx;
			testy = -testy;
		}

		testx += x;
		testy += y;

		// If the highest point is in the sector, then we have it easy! Just get the Z at that point
		if (R_PointInSubsector(testx, testy)->sector == (boundsec ? boundsec : sector))
			return P_GetSlopeZAt(slope, testx, testy);

		// If boundsec is set, we're looking for specials. In that case, iterate over every line in this sector to find the TRUE highest/lowest point
		if (perfect) {
			size_t i;
			line_t *ld;
			fixed_t bbox[4];
			fixed_t finalheight;

			if (lowest)
				finalheight = INT32_MAX;
			else
				finalheight = INT32_MIN;

			bbox[BOXLEFT] = x-mobj->radius;
			bbox[BOXRIGHT] = x+mobj->radius;
			bbox[BOXTOP] = y+mobj->radius;
			bbox[BOXBOTTOM] = y-mobj->radius;
			for (i = 0; i < boundsec->linecount; i++) {
				ld = boundsec->lines[i];

				if (bbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || bbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
				|| bbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || bbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
					continue;

				if (P_BoxOnLineSide(bbox, ld) != -1)
					continue;

				if (lowest)
					finalheight = min(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, true));
				else
					finalheight = max(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, false));
			}

			return finalheight;
		}

		// If we're just testing for base sector location (no collision line), just go for the center's spot...
		// It'll get fixed when we test for collision anyway, and the final result can't be lower than this
		if (line == NULL)
			return P_GetSlopeZAt(slope, x, y);

		return HighestOnLine(mobj->radius, x, y, line, slope, lowest);
	} else // Well, that makes it easy. Just get the floor height
		return sector->floorheight;
}

fixed_t P_CameraCeilingZ(camera_t *mobj, sector_t *sector, sector_t *boundsec, fixed_t x, fixed_t y, line_t *line, boolean lowest, boolean perfect)
{
	I_Assert(mobj != NULL);
	I_Assert(sector != NULL);

	if (sector->c_slope) {
		fixed_t testx, testy;
		pslope_t *slope = sector->c_slope;

		// Get the corner of the object that should be the highest on the slope
		if (slope->d.x < 0)
			testx = mobj->radius;
		else
			testx = -mobj->radius;

		if (slope->d.y < 0)
			testy = mobj->radius;
		else
			testy = -mobj->radius;

		if ((slope->zdelta > 0) ^ !!(lowest)) {
			testx = -testx;
			testy = -testy;
		}

		testx += x;
		testy += y;

		// If the highest point is in the sector, then we have it easy! Just get the Z at that point
		if (R_PointInSubsector(testx, testy)->sector == (boundsec ? boundsec : sector))
			return P_GetSlopeZAt(slope, testx, testy);

		// If boundsec is set, we're looking for specials. In that case, iterate over every line in this sector to find the TRUE highest/lowest point
		if (perfect) {
			size_t i;
			line_t *ld;
			fixed_t bbox[4];
			fixed_t finalheight;

			if (lowest)
				finalheight = INT32_MAX;
			else
				finalheight = INT32_MIN;

			bbox[BOXLEFT] = x-mobj->radius;
			bbox[BOXRIGHT] = x+mobj->radius;
			bbox[BOXTOP] = y+mobj->radius;
			bbox[BOXBOTTOM] = y-mobj->radius;
			for (i = 0; i < boundsec->linecount; i++) {
				ld = boundsec->lines[i];

				if (bbox[BOXRIGHT] <= ld->bbox[BOXLEFT] || bbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
				|| bbox[BOXTOP] <= ld->bbox[BOXBOTTOM] || bbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
					continue;

				if (P_BoxOnLineSide(bbox, ld) != -1)
					continue;

				if (lowest)
					finalheight = min(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, true));
				else
					finalheight = max(finalheight, HighestOnLine(mobj->radius, x, y, ld, slope, false));
			}

			return finalheight;
		}

		// If we're just testing for base sector location (no collision line), just go for the center's spot...
		// It'll get fixed when we test for collision anyway, and the final result can't be lower than this
		if (line == NULL)
			return P_GetSlopeZAt(slope, x, y);

		return HighestOnLine(mobj->radius, x, y, line, slope, lowest);
	} else // Well, that makes it easy. Just get the ceiling height
		return sector->ceilingheight;
}
static void P_PlayerFlip(mobj_t *mo)
{
	if (!mo->player)
		return;

	G_GhostAddFlip();
	// Flip aiming to match!

	if (mo->player->powers[pw_carry] == CR_NIGHTSMODE) // NiGHTS doesn't use flipcam
		;
	else if (mo->player->pflags & PF_FLIPCAM)
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

//
// P_GetMobjGravity
//
// Returns the current gravity
// value of the object.
//
fixed_t P_GetMobjGravity(mobj_t *mo)
{
	fixed_t gravityadd = 0;
	boolean no3dfloorgrav = true; // Custom gravity
	boolean goopgravity = false;
	boolean wasflip;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	wasflip = (mo->eflags & MFE_VERTICALFLIP) != 0;

	if (mo->type != MT_SPINFIRE)
		mo->eflags &= ~MFE_VERTICALFLIP;

	if (mo->subsector->sector->ffloors) // Check for 3D floor gravity too.
	{
		ffloor_t *rover;

		for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !P_InsideANonSolidFFloor(mo, rover)) // P_InsideANonSolidFFloor checks for FF_EXISTS itself, but let's not always call this function
				continue;

			if ((rover->flags & (FF_SWIMMABLE|FF_GOOWATER)) == (FF_SWIMMABLE|FF_GOOWATER))
				goopgravity = true;

			if (!(rover->master->frontsector->gravity))
				continue;

			gravityadd = -FixedMul(gravity,
				(FixedDiv(*rover->master->frontsector->gravity>>FRACBITS, 1000)));

			if (rover->master->frontsector->verticalflip && gravityadd > 0)
				mo->eflags |= MFE_VERTICALFLIP;

			no3dfloorgrav = false;
			break;
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

	if (mo->player)
	{
		if ((mo->player->pflags & PF_GLIDING)
		|| (mo->player->charability == CA_FLY && mo->player->panim == PA_ABILITY))
			gravityadd = gravityadd/3; // less gravity while flying/gliding
		if (mo->player->climbing || (mo->player->powers[pw_carry] == CR_NIGHTSMODE))
			gravityadd = 0;

		if (!(mo->flags2 & MF2_OBJECTFLIP) != !(mo->player->powers[pw_gravityboots])) // negated to turn numeric into bool - would be double negated, but not needed if both would be
		{
			gravityadd = -gravityadd;
			mo->eflags ^= MFE_VERTICALFLIP;
		}
		if (wasflip == !(mo->eflags & MFE_VERTICALFLIP)) // note!! == ! is not equivalent to != here - turns numeric into bool this way
			P_PlayerFlip(mo);
	}
	else
	{
		// Objects with permanent reverse gravity (MF2_OBJECTFLIP)
		if (mo->flags2 & MF2_OBJECTFLIP)
		{
			mo->eflags |= MFE_VERTICALFLIP;
			if (mo->z + mo->height >= mo->ceilingz)
				gravityadd = 0;
			else if (gravityadd < 0) // Don't sink, only rise up
				gravityadd *= -1;
		}
		else //Otherwise, sort through the other exceptions.
		{
			switch (mo->type)
			{
				case MT_FLINGRING:
				case MT_FLINGCOIN:
				case MT_FLINGBLUESPHERE:
				case MT_FLINGNIGHTSCHIP:
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
				case MT_CYBRAKDEMON:
					gravityadd >>= 1;
				default:
					break;
			}
		}
	}

	// Goop has slower, reversed gravity
	if (goopgravity)
		gravityadd = -((gravityadd/5) + (gravityadd/8));

	gravityadd = FixedMul(gravityadd, mo->scale);

	return gravityadd;
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
	fixed_t gravityadd = P_GetMobjGravity(mo);

	if (!mo->momz) // mobj at stop, no floor, so feel the push of gravity!
		gravityadd <<= 1;

	if (affect)
		mo->momz += gravityadd;

	if (mo->type == MT_SKIM && mo->z + mo->momz <= mo->watertop && mo->z >= mo->watertop)
	{
		mo->momz = 0;
		mo->flags |= MF_NOGRAVITY;
	}
}

#define STOPSPEED (FRACUNIT)

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
		// spinning friction
		if (player->pflags & PF_SPINNING && (player->rmomx || player->rmomy) && !(player->pflags & PF_STARTDASH))
		{
			if (twodlevel || player->mo->flags2 & MF2_TWOD) // Otherwise handled in P_3DMovement
			{
				const fixed_t ns = FixedDiv(549*ORIG_FRICTION,500*FRACUNIT);
				mo->momx = FixedMul(mo->momx, ns);
				mo->momy = FixedMul(mo->momy, ns);
			}
		}
		else if (abs(player->rmomx) < FixedMul(STOPSPEED, mo->scale)
		    && abs(player->rmomy) < FixedMul(STOPSPEED, mo->scale)
		    && (!(player->cmd.forwardmove && !(twodlevel || mo->flags2 & MF2_TWOD)) && !player->cmd.sidemove && !(player->pflags & PF_SPINNING))
			&& !(player->mo->standingslope && (!(player->mo->standingslope->flags & SL_NOPHYSICS)) && (abs(player->mo->standingslope->zdelta) >= FRACUNIT/2)))
		{
			// if in a walking frame, stop moving
			if (player->panim == PA_WALK)
				P_SetPlayerMobjState(mo, S_PLAY_STND);
			mo->momx = player->cmomx;
			mo->momy = player->cmomy;
		}
		else if (!(mo->eflags & MFE_SPRUNG))
		{
			if (oldx == mo->x && oldy == mo->y) // didn't go anywhere
			{
				mo->momx = FixedMul(mo->momx, ORIG_FRICTION);
				mo->momy = FixedMul(mo->momy, ORIG_FRICTION);
			}
			else
			{
				mo->momx = FixedMul(mo->momx, mo->friction);
				mo->momy = FixedMul(mo->momy, mo->friction);
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

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;
		fixed_t topheight, bottomheight;

		if (!node->m_sector)
			break;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_BUSTUP))
				continue;

			// Needs ML_EFFECT4 flag for pushables to break it
			if (!(rover->master->flags & ML_EFFECT4))
				continue;

			if (rover->master->frontsector->crumblestate != CRUMBLE_NONE)
				continue;

			topheight = P_GetFOFTopZ(mo, node->m_sector, rover, mo->x, mo->y, NULL);
			bottomheight = P_GetFOFBottomZ(mo, node->m_sector, rover, mo->x, mo->y, NULL);

			// Height checks
			if (rover->flags & FF_SHATTERBOTTOM)
			{
				if (mo->z + mo->momz + mo->height < bottomheight)
					continue;

				if (mo->z + mo->height > bottomheight)
					continue;
			}
			else if (rover->flags & FF_SPINBUST)
			{
				if (mo->z + mo->momz > topheight)
					continue;

				if (mo->z + mo->height < bottomheight)
					continue;
			}
			else if (rover->flags & FF_SHATTER)
			{
				if (mo->z + mo->momz > topheight)
					continue;

				if (mo->z + mo->momz + mo->height < bottomheight)
					continue;
			}
			else
			{
				if (mo->z >= topheight)
					continue;

				if (mo->z + mo->height < bottomheight)
					continue;
			}

			EV_CrumbleChain(NULL, rover); // node->m_sector

			// Run a linedef executor??
			if (rover->master->flags & ML_EFFECT5)
				P_LinedefExecute((INT16)(P_AproxDistance(rover->master->dx, rover->master->dy)>>FRACBITS), mo, node->m_sector);

			goto bustupdone;
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
	pslope_t *oldslope = NULL;
	vector3_t slopemom = {0,0,0};
	fixed_t predictedz = 0;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

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

	if (mo->flags & MF_NOCLIPHEIGHT)
		mo->standingslope = NULL;

	// adjust various things based on slope
	if (mo->standingslope && abs(mo->standingslope->zdelta) > FRACUNIT>>8) {
		if (!P_IsObjectOnGround(mo)) { // We fell off at some point? Do the twisty thing!
			P_SlopeLaunch(mo);
			xmove = mo->momx;
			ymove = mo->momy;
		} else { // Still on the ground.
			slopemom.x = xmove;
			slopemom.y = ymove;
			slopemom.z = 0;
			P_QuantizeMomentumToSlope(&slopemom, mo->standingslope);

			xmove = slopemom.x;
			ymove = slopemom.y;

			predictedz = mo->z + slopemom.z; // We'll use this later...

			oldslope = mo->standingslope;
		}
	} else if (P_IsObjectOnGround(mo) && !mo->momz)
		predictedz = mo->z;

	// Pushables can break some blocks
	if (CheckForBustableBlocks && ((mo->flags & MF_PUSHABLE) || ((mo->info->flags & MF_PUSHABLE) && mo->fuse)))
		P_PushableCheckBustables(mo);

	if (!P_TryMove(mo, mo->x + xmove, mo->y + ymove, true)
		&& !(P_MobjWasRemoved(mo) || mo->eflags & MFE_SPRUNG))
	{
		// blocked move
		moved = false;

		if (player) {
			if (player->bot)
				B_MoveBlocked(player);
		}

		if (LUAh_MobjMoveBlocked(mo))
		{
			if (P_MobjWasRemoved(mo))
				return;
		}
		else if (P_MobjWasRemoved(mo))
			return;
		else if (mo->flags & MF_BOUNCE)
		{
			P_BounceMove(mo);
			xmove = ymove = 0;
			S_StartSound(mo, mo->info->activesound);

			// Bounce ring algorithm
			if (mo->type == MT_THROWNBOUNCE)
			{
				mo->threshold++;

				// Gain lower amounts of time on each bounce.
				if (mo->fuse && mo->threshold < 5)
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
		else if (player || mo->flags & (MF_SLIDEME|MF_PUSHABLE))
		{ // try to slide along it
			// Wall transfer part 1.
			pslope_t *transferslope = NULL;
			fixed_t transfermomz = 0;
			if (oldslope && (P_MobjFlip(mo)*(predictedz - mo->z) > 0)) // Only for moving up (relative to gravity), otherwise there's a failed launch when going down slopes and hitting walls
			{
				transferslope = ((mo->standingslope) ? mo->standingslope : oldslope);
				if (((transferslope->zangle < ANGLE_180) ? transferslope->zangle : InvAngle(transferslope->zangle)) >= ANGLE_45) // Prevent some weird stuff going on on shallow slopes.
					transfermomz = P_GetWallTransferMomZ(mo, transferslope);
			}

			P_SlideMove(mo);
			if (player)
				player->powers[pw_pushing] = 3;
			xmove = ymove = 0;

			// Wall transfer part 2.
			if (transfermomz && transferslope) // Are we "transferring onto the wall" (really just a disguised vertical launch)?
			{
				angle_t relation; // Scale transfer momentum based on how head-on it is to the slope.
				if (mo->momx || mo->momy) // "Guess" the angle of the wall you hit using new momentum
					relation = transferslope->xydirection - R_PointToAngle2(0, 0, mo->momx, mo->momy);
				else // Give it for free, I guess.
					relation = ANGLE_90;
				transfermomz = FixedMul(transfermomz,
					abs(FINESINE((relation >> ANGLETOFINESHIFT) & FINEMASK)));
				if (P_MobjFlip(mo)*(transfermomz - mo->momz) > 2*FRACUNIT) // Do the actual launch!
				{
					mo->momz = transfermomz;
					mo->standingslope = NULL;
					if (player)
					{
						player->powers[pw_justlaunched] = 2;
						if (player->pflags & PF_SPINNING)
							player->pflags |= PF_THOKKED;
					}
				}
			}
		}
		else if (mo->type == MT_SPINFIRE)
		{
			P_RemoveMobj(mo);
			return;
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
			&& !(mo->flags2 & (MF2_AUTOMATIC|MF2_RAILRING|MF2_BOUNCERING|MF2_EXPLOSION|MF2_SCATTER)))
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
	else
		moved = true;

	if (P_MobjWasRemoved(mo)) // MF_SPECIAL touched a player! O_o;;
		return;

	if (moved && oldslope && !(mo->flags & MF_NOCLIPHEIGHT)) { // Check to see if we ran off

		if (oldslope != mo->standingslope) { // First, compare different slopes
			angle_t oldangle, newangle;
			angle_t moveangle = R_PointToAngle2(0, 0, mo->momx, mo->momy);

			oldangle = FixedMul((signed)oldslope->zangle, FINECOSINE((moveangle - oldslope->xydirection) >> ANGLETOFINESHIFT));

			if (mo->standingslope)
				newangle = FixedMul((signed)mo->standingslope->zangle, FINECOSINE((moveangle - mo->standingslope->xydirection) >> ANGLETOFINESHIFT));
			else
				newangle = 0;

			// Now compare the Zs of the different quantizations
			if (oldangle-newangle > ANG30 && oldangle-newangle < ANGLE_180) { // Allow for a bit of sticking - this value can be adjusted later
				mo->standingslope = oldslope;
				P_SlopeLaunch(mo);

				//CONS_Printf("launched off of slope - ");
			}

			/*CONS_Printf("old angle %f - new angle %f = %f\n",
						FIXED_TO_FLOAT(AngleFixed(oldangle)),
						FIXED_TO_FLOAT(AngleFixed(newangle)),
						FIXED_TO_FLOAT(AngleFixed(oldangle-newangle))
						);*/
		} else if (predictedz-mo->z > abs(slopemom.z/2)) { // Now check if we were supposed to stick to this slope
			//CONS_Printf("%d-%d > %d\n", (predictedz), (mo->z), (slopemom.z/2));
			P_SlopeLaunch(mo);
		}
	} else if (moved && mo->standingslope && predictedz) {
		angle_t moveangle = R_PointToAngle2(0, 0, mo->momx, mo->momy);
		angle_t newangle = FixedMul((signed)mo->standingslope->zangle, FINECOSINE((moveangle - mo->standingslope->xydirection) >> ANGLETOFINESHIFT));

			/*CONS_Printf("flat to angle %f - predicted z of %f\n",
						FIXED_TO_FLOAT(AngleFixed(ANGLE_MAX-newangle)),
						FIXED_TO_FLOAT(predictedz)
						);*/
		if (ANGLE_MAX-newangle > ANG30 && newangle > ANGLE_180) {
			mo->momz = P_MobjFlip(mo)*FRACUNIT/2;
			mo->z = predictedz + P_MobjFlip(mo);
			mo->standingslope = NULL;
			//CONS_Printf("Launched off of flat surface running into downward slope\n");
		}
	}

	// Check the gravity status.
	P_CheckGravity(mo, false);

	if (player && !moved && player->powers[pw_carry] == CR_NIGHTSMODE && mo->target)
	{
		angle_t fa;

		P_UnsetThingPosition(mo);
		player->angle_pos = player->old_angle_pos;
		player->speed = FixedMul(player->speed, 4*FRACUNIT/5);
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

	if (mo->flags & MF_NOCLIPHEIGHT)
		return; // no frictions for objects that can pass through floors

	if (mo->flags & MF_MISSILE || mo->flags2 & MF2_SKULLFLY || mo->type == MT_SHELL || mo->type == MT_VULTURE || mo->type == MT_PENGUINATOR)
		return; // no friction for missiles ever

	if (player && player->homing) // no friction for homing
		return;

	if (player && player->powers[pw_carry] == CR_NIGHTSMODE)
		return; // no friction for NiGHTS players

	if ((mo->type == MT_BIGTUMBLEWEED || mo->type == MT_LITTLETUMBLEWEED)
			&& (mo->standingslope && abs(mo->standingslope->zdelta) > FRACUNIT>>8)) // Special exception for tumbleweeds on slopes
		return;

	if (((!(mo->eflags & MFE_VERTICALFLIP) && mo->z > mo->floorz) || (mo->eflags & MFE_VERTICALFLIP && mo->z+mo->height < mo->ceilingz))
		&& !(player && player->pflags & PF_SLIDING))
		return; // no friction when airborne

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

	if ((!(mo->eflags & MFE_VERTICALFLIP) && mo->z > mo->floorz) || (mo->eflags & MFE_VERTICALFLIP && mo->z+mo->height < mo->ceilingz))
		return; // no friction when airborne

	if (mo->flags & MF_NOCLIPHEIGHT)
		return; // no frictions for objects that can pass through floors

	P_SceneryXYFriction(mo, oldx, oldy);
}

//
// P_AdjustMobjFloorZ_FFloors
//
// Utility function for P_ZMovement and related
// Adjusts mo->floorz/mo->ceiling accordingly for FFloors
//
// "motype" determines what behaviour to use exactly
// This is to keep things consistent in case these various object types NEED to be different
//
// motype options:
// 0 - normal
// 1 - forces false check for water (rings)
// 2 - forces false check for water + different quicksand behaviour (scenery)
//
void P_AdjustMobjFloorZ_FFloors(mobj_t *mo, sector_t *sector, UINT8 motype)
{
	ffloor_t *rover;
	fixed_t delta1, delta2, thingtop;
	fixed_t topheight, bottomheight;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	thingtop = mo->z + mo->height;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!(rover->flags & FF_EXISTS))
			continue;

		topheight = P_GetFOFTopZ(mo, sector, rover, mo->x, mo->y, NULL);
		bottomheight = P_GetFOFBottomZ(mo, sector, rover, mo->x, mo->y, NULL);

		if (mo->player && (P_CheckSolidLava(rover) || P_CanRunOnWater(mo->player, rover))) // only the player should stand on lava or run on water
			;
		else if (motype != 0 && rover->flags & FF_SWIMMABLE) // "scenery" only
			continue;
		else if (rover->flags & FF_QUICKSAND) // quicksand
			;
		else if (!( // if it's not either of the following...
				(rover->flags & (FF_BLOCKPLAYER|FF_MARIO) && mo->player) // ...solid to players? (mario blocks are always solid from beneath to players)
			    || (rover->flags & FF_BLOCKOTHERS && !mo->player) // ...solid to others?
				)) // ...don't take it into account.
			continue;
		if (rover->flags & FF_QUICKSAND)
		{
			switch (motype)
			{
				case 2: // scenery does things differently for some reason
					if (mo->z < topheight && bottomheight < thingtop)
					{
						mo->floorz = mo->z;
						continue;
					}
					break;
				default:
					if (mo->z < topheight && bottomheight < thingtop)
					{
						if (mo->floorz < mo->z)
							mo->floorz = mo->z;
					}
					continue; // This is so you can jump/spring up through quicksand from below.
			}
		}

		delta1 = mo->z - (bottomheight + ((topheight - bottomheight)/2));
		delta2 = thingtop - (bottomheight + ((topheight - bottomheight)/2));

		if (topheight > mo->floorz && abs(delta1) < abs(delta2)
			&& (rover->flags & FF_SOLID) // Non-FF_SOLID Mario blocks are only solid from bottom
			&& !(rover->flags & FF_REVERSEPLATFORM)
			&& ((P_MobjFlip(mo)*mo->momz >= 0) || (!(rover->flags & FF_PLATFORM)))) // In reverse gravity, only clip for FOFs that are intangible from their bottom (the "top" you're falling through) if you're coming from above ("below" in your frame of reference)
		{
			mo->floorz = topheight;
		}
		if (bottomheight < mo->ceilingz && abs(delta1) >= abs(delta2)
			&& !(rover->flags & FF_PLATFORM)
			&& ((P_MobjFlip(mo)*mo->momz >= 0) || ((rover->flags & FF_SOLID) && !(rover->flags & FF_REVERSEPLATFORM)))) // In normal gravity, only clip for FOFs that are intangible from the top if you're coming from below
		{
			mo->ceilingz = bottomheight;
		}
	}
}

//
// P_AdjustMobjFloorZ_PolyObjs
//
// Utility function for P_ZMovement and related
// Adjusts mo->floorz/mo->ceiling accordingly for PolyObjs
//
static void P_AdjustMobjFloorZ_PolyObjs(mobj_t *mo, subsector_t *subsec)
{
	polyobj_t *po = subsec->polyList;
	sector_t *polysec;
	fixed_t delta1, delta2, thingtop;
	fixed_t polytop, polybottom;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	thingtop = mo->z + mo->height;

	while(po)
	{
		if (!P_MobjInsidePolyobj(po, mo) || !(po->flags & POF_SOLID))
		{
			po = (polyobj_t *)(po->link.next);
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

		delta1 = mo->z - (polybottom + ((polytop - polybottom)/2));
		delta2 = thingtop - (polybottom + ((polytop - polybottom)/2));

		if (polytop > mo->floorz && abs(delta1) < abs(delta2))
			mo->floorz = polytop;

		if (polybottom < mo->ceilingz && abs(delta1) >= abs(delta2))
			mo->ceilingz = polybottom;

		po = (polyobj_t *)(po->link.next);
	}
}

static void P_RingZMovement(mobj_t *mo)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
		P_AdjustMobjFloorZ_FFloors(mo, mo->subsector->sector, 1);
	if (mo->subsector->polyList)
		P_AdjustMobjFloorZ_PolyObjs(mo, mo->subsector);

	// adjust height
	if (mo->eflags & MFE_APPLYPMOMZ && !P_IsObjectOnGround(mo))
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
		mo->eflags &= ~MFE_APPLYPMOMZ;
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
		mo->z = mo->ceilingz - mo->height;

		mo->momz = 0;
	}
}

boolean P_CheckDeathPitCollide(mobj_t *mo)
{
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (mo->player && mo->player->pflags & PF_GODMODE)
		return false;

	if (((mo->z <= mo->subsector->sector->floorheight
		&& ((mo->subsector->sector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(mo->eflags & MFE_VERTICALFLIP)) && (mo->subsector->sector->flags & SF_FLIPSPECIAL_FLOOR))
	|| (mo->z + mo->height >= mo->subsector->sector->ceilingheight
		&& ((mo->subsector->sector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (mo->eflags & MFE_VERTICALFLIP)) && (mo->subsector->sector->flags & SF_FLIPSPECIAL_CEILING)))
	&& (GETSECSPECIAL(mo->subsector->sector->special, 1) == 6
	|| GETSECSPECIAL(mo->subsector->sector->special, 1) == 7))
		return true;

	return false;
}

boolean P_CheckSolidLava(ffloor_t *rover)
{
	if (rover->flags & FF_SWIMMABLE && GETSECSPECIAL(rover->master->frontsector->special, 1) == 3
		&& !(rover->master->flags & ML_BLOCKMONSTERS))
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
	boolean onground;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
		P_AdjustMobjFloorZ_FFloors(mo, mo->subsector->sector, 0);
	if (mo->subsector->polyList)
		P_AdjustMobjFloorZ_PolyObjs(mo, mo->subsector);

	// adjust height
	if (mo->eflags & MFE_APPLYPMOMZ && !P_IsObjectOnGround(mo))
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
		mo->eflags &= ~MFE_APPLYPMOMZ;
	}
	mo->z += mo->momz;
	onground = P_IsObjectOnGround(mo);

	if (mo->standingslope)
	{
		if (mo->flags & MF_NOCLIPHEIGHT)
			mo->standingslope = NULL;
		else if (!onground)
			P_SlopeLaunch(mo);
	}

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
				if (mo->fuse && mo->threshold < 5)
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
		case MT_SPINFIRE:
			if (P_CheckDeathPitCollide(mo))
			{
				P_RemoveMobj(mo);
				return false;
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
		case MT_BLUESPHERE:
		case MT_BOMBSPHERE:
		case MT_NIGHTSCHIP:
		case MT_NIGHTSSTAR:
		case MT_REDTEAMRING:
		case MT_BLUETEAMRING:
		case MT_FLINGRING:
		case MT_FLINGCOIN:
		case MT_FLINGBLUESPHERE:
		case MT_FLINGNIGHTSCHIP:
		case MT_FLINGEMERALD:
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
		case MT_FLAMEJET:
		case MT_VERTICALFLAMEJET:
			if (!(mo->flags & MF_BOUNCE))
				return true;
			break;
		case MT_SPIKE:
		case MT_WALLSPIKE:
			// Dead spike particles disappear upon ground contact
			if (!mo->health && (mo->z <= mo->floorz || mo->z + mo->height >= mo->ceilingz))
			{
				P_RemoveMobj(mo);
				return false;
			}
			break;
		default:
			break;
	}

	if (!mo->player && P_CheckDeathPitCollide(mo))
	{
		switch (mo->type)
		{
			case MT_GHOST:
			case MT_METALSONIC_RACE:
			case MT_EXPLODE:
			case MT_BOSSEXPLODE:
			case MT_SONIC3KBOSSEXPLODE:
				break;
			default:
				if (mo->flags & MF_ENEMY || mo->flags & MF_BOSS || mo->type == MT_MINECART)
				{
					// Kill enemies, bosses and minecarts that fall into death pits.
					if (mo->health)
					{
						P_KillMobj(mo, NULL, NULL, 0);
					}
					return false;
				}
				else
				{
					P_RemoveMobj(mo);
					return false;
				}
				break;
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
	if (((mo->z <= mo->floorz && !(mo->eflags & MFE_VERTICALFLIP))
		|| (mo->z + mo->height >= mo->ceilingz && mo->eflags & MFE_VERTICALFLIP))
	&& !(mo->flags & MF_NOCLIPHEIGHT))
	{
		vector3_t mom;
		mom.x = mo->momx;
		mom.y = mo->momy;
		mom.z = mo->momz;

		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->ceilingz - mo->height;
		else
			mo->z = mo->floorz;

		if (!(mo->flags & MF_MISSILE) && mo->standingslope) // You're still on the ground; why are we here?
		{
			mo->momz = 0;
			return true;
		}

		P_CheckPosition(mo, mo->x, mo->y); // Sets mo->standingslope correctly
		if (((mo->eflags & MFE_VERTICALFLIP) ? tmceilingslope : tmfloorslope) && (mo->type != MT_STEAM))
		{
			mo->standingslope = (mo->eflags & MFE_VERTICALFLIP) ? tmceilingslope : tmfloorslope;
			P_ReverseQuantizeMomentumToSlope(&mom, mo->standingslope);
		}

		// hit the floor
		if (mo->type == MT_FIREBALL) // special case for the fireball
			mom.z = P_MobjFlip(mo)*FixedMul(5*FRACUNIT, mo->scale);
		else if (mo->type == MT_SPINFIRE) // elemental shield fire is another exception here
			;
		else if (mo->flags & MF_MISSILE)
		{
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
					if (P_MobjFlip(mo)*mom.z < 0)
					{
						// If going slower than a fracunit, just stop.
						if (abs(mom.z) < FixedMul(FRACUNIT, mo->scale))
						{
							mom.x = mom.y = mom.z = 0;

							// Napalm hack
							if (mo->type == MT_CYBRAKDEMON_NAPALM_BOMB_LARGE && mo->fuse)
								mo->fuse = 1;
						}
						// Otherwise bounce up at half speed.
						else
							mom.z = -mom.z/2;
						S_StartSound(mo, mo->info->activesound);
					}
				}
				// Hack over. Back to your regularly scheduled detonation. -SH
				else
				{
					// Don't explode on the sky!
					if (!(mo->eflags & MFE_VERTICALFLIP)
					&& mo->subsector->sector->floorpic == skyflatnum
					&& mo->subsector->sector->floorheight == mo->floorz)
						P_RemoveMobj(mo);
					else if (mo->eflags & MFE_VERTICALFLIP
					&& mo->subsector->sector->ceilingpic == skyflatnum
					&& mo->subsector->sector->ceilingheight == mo->ceilingz)
						P_RemoveMobj(mo);
					else
						P_ExplodeMissile(mo);
					return false;
				}
			}
		}

		if (P_MobjFlip(mo)*mom.z < 0) // falling
		{
			mo->eflags |= MFE_JUSTHITFLOOR;

			if (mo->flags2 & MF2_SKULLFLY) // the skull slammed into something
				mom.z = -mom.z;
			else
			// Flingrings bounce
			if (mo->type == MT_FLINGRING
				|| mo->type == MT_FLINGCOIN
				|| mo->type == MT_FLINGBLUESPHERE
				|| mo->type == MT_FLINGNIGHTSCHIP
				|| P_WeaponOrPanel(mo->type)
				|| mo->type == MT_FLINGEMERALD
				|| mo->type == MT_BIGTUMBLEWEED
				|| mo->type == MT_LITTLETUMBLEWEED
				|| mo->type == MT_CANNONBALLDECOR
				|| mo->type == MT_FALLINGROCK)
			{
				if (maptol & TOL_NIGHTS)
					mom.z = -FixedDiv(mom.z, 10*FRACUNIT);
				else
					mom.z = -FixedMul(mom.z, FixedDiv(17*FRACUNIT,20*FRACUNIT));

				if (mo->type == MT_BIGTUMBLEWEED || mo->type == MT_LITTLETUMBLEWEED)
				{
					if (abs(mom.x) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mom.y) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mom.z) < FixedMul(STOPSPEED*3, mo->scale))
					{
						if (mo->flags2 & MF2_AMBUSH)
						{
							// If deafed, give the tumbleweed another random kick if it runs out of steam.
							mom.z += P_MobjFlip(mo)*FixedMul(6*FRACUNIT, mo->scale);

							if (P_RandomChance(FRACUNIT/2))
								mom.x += FixedMul(6*FRACUNIT, mo->scale);
							else
								mom.x -= FixedMul(6*FRACUNIT, mo->scale);

							if (P_RandomChance(FRACUNIT/2))
								mom.y += FixedMul(6*FRACUNIT, mo->scale);
							else
								mom.y -= FixedMul(6*FRACUNIT, mo->scale);
						}
						else if (mo->standingslope && abs(mo->standingslope->zdelta) > FRACUNIT>>8)
						{
							// Pop the object up a bit to encourage bounciness
							//mom.z = P_MobjFlip(mo)*mo->scale;
						}
						else
						{
							mom.x = mom.y = mom.z = 0;
							P_SetMobjState(mo, mo->info->spawnstate);
						}
					}

					// Stolen from P_SpawnFriction
					mo->friction = FRACUNIT - 0x100;
				}
				else if (mo->type == MT_FALLINGROCK)
				{
					if (P_MobjFlip(mo)*mom.z > FixedMul(2*FRACUNIT, mo->scale))
						S_StartSound(mo, mo->info->activesound + P_RandomKey(mo->info->reactiontime));

					mom.z /= 2; // Rocks not so bouncy

					if (abs(mom.x) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mom.y) < FixedMul(STOPSPEED, mo->scale)
						&& abs(mom.z) < FixedMul(STOPSPEED*3, mo->scale))
					{
						P_RemoveMobj(mo);
						return false;
					}
				}
				else if (mo->type == MT_CANNONBALLDECOR)
				{
					mom.z /= 2;
					if (abs(mom.z) < FixedMul(STOPSPEED*3, mo->scale))
						mom.z = 0;
				}
			}
			else
				mom.z = (tmfloorthing ? tmfloorthing->momz : 0);

		}
		else if (tmfloorthing)
			mom.z = tmfloorthing->momz;

		if (mo->standingslope) { // MT_STEAM will never have a standingslope, see above.
			P_QuantizeMomentumToSlope(&mom, mo->standingslope);
		}

		mo->momx = mom.x;
		mo->momy = mom.y;
		mo->momz = mom.z;

		if (mo->type == MT_STEAM)
			return true;
	}
	else if (!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MFE_JUSTHITFLOOR;

		P_CheckGravity(mo, true);
	}

	if (((mo->z + mo->height > mo->ceilingz && !(mo->eflags & MFE_VERTICALFLIP))
		|| (mo->z < mo->floorz && mo->eflags & MFE_VERTICALFLIP))
	&& !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->floorz;
		else
			mo->z = mo->ceilingz - mo->height;

		if (mo->type == MT_SPINFIRE)
			;
		else if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
		{
			// Hack 2: Electric Boogaloo -SH
			if (mo->flags & MF_GRENADEBOUNCE)
			{
				if (P_MobjFlip(mo)*mo->momz >= 0)
				{
					mo->momz = -mo->momz;
					S_StartSound(mo, mo->info->activesound);
				}
			}
			else
			{
				// Don't explode on the sky!
				if (!(mo->eflags & MFE_VERTICALFLIP)
				&& mo->subsector->sector->ceilingpic == skyflatnum
				&& mo->subsector->sector->ceilingheight == mo->ceilingz)
					P_RemoveMobj(mo);
				else if (mo->eflags & MFE_VERTICALFLIP
				&& mo->subsector->sector->floorpic == skyflatnum
				&& mo->subsector->sector->floorheight == mo->floorz)
					P_RemoveMobj(mo);
				else
					P_ExplodeMissile(mo);
				return false;
			}
		}

		if (P_MobjFlip(mo)*mo->momz > 0) // hit the ceiling
		{
			if (mo->flags2 & MF2_SKULLFLY) // the skull slammed into something
				mo->momz = -mo->momz;
			else
			// Flags bounce
			if (mo->type == MT_REDFLAG || mo->type == MT_BLUEFLAG)
			{
				if (maptol & TOL_NIGHTS)
					mo->momz = -FixedDiv(mo->momz, 10*FRACUNIT);
				else
					mo->momz = -FixedMul(mo->momz, FixedDiv(17*FRACUNIT,20*FRACUNIT));
			}
			else
				mo->momz = 0;
		}
	}

	return true;
}

// Check for "Mario" blocks to hit and bounce them
static void P_CheckMarioBlocks(mobj_t *mo)
{
	msecnode_t *node;

	if (netgame && mo->player->spectator)
		return;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_MARIO))
				continue;

			if (mo->eflags & MFE_VERTICALFLIP)
				continue; // if you were flipped, your head isn't actually hitting your ceilingz is it?

			if (*rover->bottomheight != mo->ceilingz)
				continue;

			if (rover->flags & FF_SHATTERBOTTOM) // Brick block!
				EV_CrumbleChain(node->m_sector, rover);
			else // Question block!
				EV_MarioBlock(rover, node->m_sector, mo);
		}
	}
}

// Check if we're on a polyobject that triggers a linedef executor.
static boolean P_PlayerPolyObjectZMovement(mobj_t *mo)
{
	msecnode_t *node;
	boolean stopmovecut = false;

	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		sector_t *sec = node->m_sector;
		subsector_t *newsubsec;
		size_t i;

		for (i = 0; i < numsubsectors; i++)
		{
			polyobj_t *po;
			sector_t *polysec;
			newsubsec = &subsectors[i];

			if (newsubsec->sector != sec)
				continue;

			for (po = newsubsec->polyList; po; po = (polyobj_t *)(po->link.next))
			{
				if (!(po->flags & POF_SOLID))
					continue;

				if (!P_MobjInsidePolyobj(po, mo))
					continue;

				polysec = po->lines[0]->backsector;

				// Moving polyobjects should act like conveyors if the player lands on one. (I.E. none of the momentum cut thing below) -Red
				if ((mo->z == polysec->ceilingheight || mo->z + mo->height == polysec->floorheight) && po->thinker)
					stopmovecut = true;

				if (!(po->flags & POF_LDEXEC))
					continue;

				if (mo->z != polysec->ceilingheight)
					continue;

				// We're landing on a PO, so check for a linedef executor.
				// Trigger tags are 32000 + the PO's ID number.
				P_LinedefExecute((INT16)(32000 + po->id), mo, NULL);
			}
		}
	}

	return stopmovecut;
}

static void P_PlayerZMovement(mobj_t *mo)
{
	boolean onground;

	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	if (!mo->player)
		return;

	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
		P_AdjustMobjFloorZ_FFloors(mo, mo->subsector->sector, 0);
	if (mo->subsector->polyList)
		P_AdjustMobjFloorZ_PolyObjs(mo, mo->subsector);

	// check for smooth step up
	if ((mo->eflags & MFE_VERTICALFLIP && mo->z + mo->height > mo->ceilingz)
		|| (!(mo->eflags & MFE_VERTICALFLIP) && mo->z < mo->floorz))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->player->viewheight -= (mo->z+mo->height) - mo->ceilingz;
		else
			mo->player->viewheight -= mo->floorz - mo->z;

		mo->player->deltaviewheight =
			(FixedMul(41*mo->player->height/48, mo->scale) - mo->player->viewheight)>>3;
	}

	// adjust height
	if (mo->eflags & MFE_APPLYPMOMZ && !P_IsObjectOnGround(mo))
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
		mo->eflags &= ~MFE_APPLYPMOMZ;
	}

	mo->z += mo->momz;
	onground = P_IsObjectOnGround(mo);

	// Have player fall through floor?
	if (mo->player->playerstate == PST_DEAD
	|| mo->player->playerstate == PST_REBORN)
		return;

	if (mo->standingslope)
	{
		if (mo->flags & MF_NOCLIPHEIGHT)
			mo->standingslope = NULL;
		else if (!onground)
			P_SlopeLaunch(mo);
	}

	// clip movement
	if (onground && !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->ceilingz - mo->height;
		else
			mo->z = mo->floorz;

		if (mo->player->powers[pw_carry] == CR_NIGHTSMODE)
		{
			// bounce off floor if you were flying towards it
			if ((mo->eflags & MFE_VERTICALFLIP && mo->player->flyangle > 0 && mo->player->flyangle < 180)
			|| (!(mo->eflags & MFE_VERTICALFLIP) && mo->player->flyangle > 180 && mo->player->flyangle <= 359))
			{
				if (mo->player->flyangle < 90 || mo->player->flyangle >= 270)
					mo->player->flyangle += P_MobjFlip(mo)*90;
				else
					mo->player->flyangle -= P_MobjFlip(mo)*90;
				mo->player->speed = FixedMul(mo->player->speed, 4*FRACUNIT/5);
			}
			goto nightsdone;
		}
		// Get up if you fell.
		if (mo->player->panim == PA_PAIN)
			P_SetPlayerMobjState(mo, S_PLAY_WALK);

		if (!mo->standingslope && (mo->eflags & MFE_VERTICALFLIP ? tmceilingslope : tmfloorslope)) {
			// Handle landing on slope during Z movement
			P_HandleSlopeLanding(mo, (mo->eflags & MFE_VERTICALFLIP ? tmceilingslope : tmfloorslope));
		}

		if (P_MobjFlip(mo)*mo->momz < 0) // falling
		{
			boolean clipmomz = !(P_CheckDeathPitCollide(mo));

			mo->pmomz = 0; // We're on a new floor, don't keep doing platform movement.

			// Squat down. Decrease viewheight for a moment after hitting the ground (hard),
			if (P_MobjFlip(mo)*mo->momz < -FixedMul(8*FRACUNIT, mo->scale))
				mo->player->deltaviewheight = (P_MobjFlip(mo)*mo->momz)>>3; // make sure momz is negative

			mo->eflags |= MFE_JUSTHITFLOOR; // Spin Attack

			clipmomz = P_PlayerHitFloor(mo->player, true);

			if (!P_PlayerPolyObjectZMovement(mo))
			{
				// Cut momentum in half when you hit the ground and
				// aren't pressing any controls.
				if (!(mo->player->cmd.forwardmove || mo->player->cmd.sidemove) && !mo->player->cmomx && !mo->player->cmomy && !(mo->player->pflags & PF_SPINNING))
				{
					mo->momx >>= 1;
					mo->momy >>= 1;
				}
			}

			if (!(mo->player->pflags & PF_SPINNING) && mo->player->powers[pw_carry] != CR_NIGHTSMODE)
				mo->player->pflags &= ~PF_STARTDASH;

			if (clipmomz)
				mo->momz = (tmfloorthing ? tmfloorthing->momz : 0);
		}
		else if (tmfloorthing)
			mo->momz = tmfloorthing->momz;
	}
	else if (!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		if (P_IsObjectInGoop(mo) && !(mo->flags & MF_NOCLIPHEIGHT))
		{
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
		}
		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MFE_JUSTHITFLOOR;
		P_CheckGravity(mo, true);
	}

nightsdone:

	if (((mo->eflags & MFE_VERTICALFLIP && mo->z < mo->floorz) || (!(mo->eflags & MFE_VERTICALFLIP) && mo->z + mo->height > mo->ceilingz))
		&& !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->floorz;
		else
			mo->z = mo->ceilingz - mo->height;

		if (mo->player->powers[pw_carry] == CR_NIGHTSMODE)
		{
			// bounce off ceiling if you were flying towards it
			if ((mo->eflags & MFE_VERTICALFLIP && mo->player->flyangle > 180 && mo->player->flyangle <= 359)
			|| (!(mo->eflags & MFE_VERTICALFLIP) && mo->player->flyangle > 0 && mo->player->flyangle < 180))
				{
				if (mo->player->flyangle < 90 || mo->player->flyangle >= 270)
					mo->player->flyangle -= P_MobjFlip(mo)*90;
				else
					mo->player->flyangle += P_MobjFlip(mo)*90;
				mo->player->flyangle %= 360;
				mo->player->speed = FixedMul(mo->player->speed, 4*FRACUNIT/5);
			}
		}

		if (P_MobjFlip(mo)*mo->momz > 0)
		{
			if (CheckForMarioBlocks)
				P_CheckMarioBlocks(mo);

			// hit the ceiling
			if (mariomode)
				S_StartSound(mo, sfx_mario1);

			if (!mo->player->climbing)
				mo->momz = 0;
		}
	}
}

static boolean P_SceneryZMovement(mobj_t *mo)
{
	// Intercept the stupid 'fall through 3dfloors' bug
	if (mo->subsector->sector->ffloors)
		P_AdjustMobjFloorZ_FFloors(mo, mo->subsector->sector, 2);
	if (mo->subsector->polyList)
		P_AdjustMobjFloorZ_PolyObjs(mo, mo->subsector);

	// adjust height
	if (mo->eflags & MFE_APPLYPMOMZ && !P_IsObjectOnGround(mo))
	{
		mo->momz += mo->pmomz;
		mo->pmomz = 0;
		mo->eflags &= ~MFE_APPLYPMOMZ;
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
				mobj_t *explodemo = NULL;
				UINT8 prandom, i;

				for (i = 0; i < 4; ++i) // split into four
				{
					prandom = P_RandomByte();
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
				mobjtype_t flowertype = ((P_RandomChance(FRACUNIT/2)) ? MT_GFZFLOWER1 : MT_GFZFLOWER3);
				mobj_t *flower = P_SpawnMobjFromMobj(mo, 0, 0, 0, flowertype);
				if (flower)
				{
					P_SetScale(flower, mo->scale/16);
					flower->destscale = mo->scale;
					flower->scalespeed = mo->scale/8;
				}

				P_RemoveMobj(mo);
				return false;
			}
		default:
			break;
	}

	if (P_CheckDeathPitCollide(mo))
	{
		P_RemoveMobj(mo);
		return false;
	}

	// clip movement
	if (((mo->z <= mo->floorz && !(mo->eflags & MFE_VERTICALFLIP))
		|| (mo->z + mo->height >= mo->ceilingz && mo->eflags & MFE_VERTICALFLIP))
	&& !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->ceilingz - mo->height;
		else
			mo->z = mo->floorz;

		if (P_MobjFlip(mo)*mo->momz < 0) // falling
		{
			mo->eflags |= MFE_JUSTHITFLOOR; // Spin Attack

			if (tmfloorthing)
				mo->momz = tmfloorthing->momz;
			else if (!tmfloorthing)
				mo->momz = 0;
		}
	}
	else if (!(mo->flags & MF_NOGRAVITY)) // Gravity here!
	{
		/// \todo may not be needed (done in P_MobjThinker normally)
		mo->eflags &= ~MFE_JUSTHITFLOOR;

		P_CheckGravity(mo, true);
	}

	if (((mo->z + mo->height > mo->ceilingz && !(mo->eflags & MFE_VERTICALFLIP))
		|| (mo->z < mo->floorz && mo->eflags & MFE_VERTICALFLIP))
	&& !(mo->flags & MF_NOCLIPHEIGHT))
	{
		if (mo->eflags & MFE_VERTICALFLIP)
			mo->z = mo->floorz;
		else
			mo->z = mo->ceilingz - mo->height;

		if (P_MobjFlip(mo)*mo->momz > 0) // hit the ceiling
			mo->momz = 0;
	}

	return true;
}

// P_CanRunOnWater
//
// Returns true if player can waterrun on the 3D floor
//
boolean P_CanRunOnWater(player_t *player, ffloor_t *rover)
{
	fixed_t topheight = P_GetFFloorTopZAt(rover, player->mo->x, player->mo->y);

	if (!player->powers[pw_carry] && !player->homing
		&& ((player->powers[pw_super] || player->charflags & SF_RUNONWATER || player->dashmode >= DASHMODE_THRESHOLD) && player->mo->ceilingz-topheight >= player->mo->height)
		&& (rover->flags & FF_SWIMMABLE) && !(player->pflags & PF_SPINNING) && player->speed > FixedMul(player->runspeed, player->mo->scale)
		&& !(player->pflags & PF_SLIDING)
		&& abs(player->mo->z - topheight) < FixedMul(30*FRACUNIT, player->mo->scale))
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
	fixed_t thingtop = mobj->z + mobj->height;
	sector_t *sector = mobj->subsector->sector;
	ffloor_t *rover;
	player_t *p = mobj->player; // Will just be null if not a player.
	fixed_t height = (p ? P_GetPlayerHeight(p) : mobj->height); // for players, calculation height does not necessarily match actual height for gameplay reasons (spin, etc)
	boolean wasgroundpounding = (p && ((p->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL || (p->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP) && (p->pflags & PF_SHIELDABILITY));

	// Default if no water exists.
	mobj->watertop = mobj->waterbottom = mobj->z - 1000*FRACUNIT;

	// Reset water state.
	mobj->eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER|MFE_GOOWATER|MFE_TOUCHLAVA);

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		fixed_t topheight, bottomheight;
		if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE)
		 || (((rover->flags & FF_BLOCKPLAYER) && mobj->player)
		 || ((rover->flags & FF_BLOCKOTHERS) && !mobj->player)))
			continue;

		topheight    = P_GetFFloorTopZAt   (rover, mobj->x, mobj->y);
		bottomheight = P_GetFFloorBottomZAt(rover, mobj->x, mobj->y);

		if (mobj->eflags & MFE_VERTICALFLIP)
		{
			if (topheight < (thingtop - (height>>1))
			 || bottomheight > thingtop)
				continue;
		}
		else
		{
			if (topheight < mobj->z
			 || bottomheight > (mobj->z + (height>>1)))
				continue;
		}

		// Set the watertop and waterbottom
		mobj->watertop = topheight;
		mobj->waterbottom = bottomheight;

		// Just touching the water?
		if (((mobj->eflags & MFE_VERTICALFLIP) && thingtop - height < bottomheight)
		 || (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + height > topheight))
			mobj->eflags |= MFE_TOUCHWATER;

		// Actually in the water?
		if (((mobj->eflags & MFE_VERTICALFLIP) && thingtop - (height>>1) > bottomheight)
		 || (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z + (height>>1) < topheight))
			mobj->eflags |= MFE_UNDERWATER;

		if (mobj->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER))
		{
			if (GETSECSPECIAL(rover->master->frontsector->special, 1) == 3)
				mobj->eflags |= MFE_TOUCHLAVA;

			if (rover->flags & FF_GOOWATER && !(mobj->flags & MF_NOGRAVITY))
				mobj->eflags |= MFE_GOOWATER;
		}
	}

	// Spectators and dead players don't get to do any of the things after this.
	if (p && (p->spectator || p->playerstate != PST_LIVE))
		return;

	// Specific things for underwater players
	if (p && (mobj->eflags & MFE_UNDERWATER) == MFE_UNDERWATER)
	{
		if (!((p->powers[pw_super]) || (p->powers[pw_invulnerability])))
		{
			boolean electric = !!(p->powers[pw_shield] & SH_PROTECTELECTRIC);
			if (electric || ((p->powers[pw_shield] & SH_PROTECTFIRE) && !(p->powers[pw_shield] & SH_PROTECTWATER) && !(mobj->eflags & MFE_TOUCHLAVA)))
			{ // Water removes electric and non-water fire shields...
				P_FlashPal(p,
				electric
				? PAL_WHITE
				: PAL_NUKE,
				1);
				p->powers[pw_shield] = p->powers[pw_shield] & SH_STACK;
			}
		}

		// Drown timer setting
		if ((p->powers[pw_shield] & SH_PROTECTWATER) // Has water protection
		 || (p->exiting) || (p->pflags & PF_FINISHED) // Or finished/exiting
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

		if ((wasgroundpounding = ((mobj->eflags & MFE_GOOWATER) && wasgroundpounding)))
		{
			p->pflags &= ~PF_SHIELDABILITY;
			mobj->momz >>= 1;
		}
	}

	// The rest of this code only executes on a water state change.
	if (waterwasnotset || !!(mobj->eflags & MFE_UNDERWATER) == wasinwater)
		return;

	if ((p) // Players
	 || (mobj->flags & MF_PUSHABLE) // Pushables
	 || ((mobj->info->flags & MF_PUSHABLE) && mobj->fuse) // Previously pushable, might be moving still
	)
	{
		// Check to make sure you didn't just cross into a sector to jump out of
		// that has shallower water than the block you were originally in.
		if ((!(mobj->eflags & MFE_VERTICALFLIP) && mobj->watertop-mobj->floorz <= height>>1)
			|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->ceilingz-mobj->waterbottom <= height>>1))
			return;

		if (mobj->eflags & MFE_GOOWATER || wasingoo) { // Decide what happens to your momentum when you enter/leave goopy water.
			if (P_MobjFlip(mobj)*mobj->momz > 0)
			{
				mobj->momz -= (mobj->momz/8); // cut momentum a little bit to prevent multiple bobs
				//CONS_Printf("leaving\n");
			}
			else
			{
				if (!wasgroundpounding)
					mobj->momz >>= 1; // kill momentum significantly, to make the goo feel thick.
				//CONS_Printf("entering\n");
			}
		}
		else if (wasinwater && P_MobjFlip(mobj)*mobj->momz > 0)
			mobj->momz = FixedMul(mobj->momz, FixedDiv(780*FRACUNIT, 457*FRACUNIT)); // Give the mobj a little out-of-water boost.

		if (P_MobjFlip(mobj)*mobj->momz < 0)
		{
			if ((mobj->eflags & MFE_VERTICALFLIP && thingtop-(height>>1)-mobj->momz <= mobj->waterbottom)
				|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z+(height>>1)-mobj->momz >= mobj->watertop))
			{
				// Spawn a splash
				mobj_t *splish;
				mobjtype_t splishtype = (mobj->eflags & MFE_TOUCHLAVA) ? MT_LAVASPLISH : MT_SPLISH;
				if (mobj->eflags & MFE_VERTICALFLIP)
				{
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->waterbottom-FixedMul(mobjinfo[splishtype].height, mobj->scale), splishtype);
					splish->flags2 |= MF2_OBJECTFLIP;
					splish->eflags |= MFE_VERTICALFLIP;
				}
				else
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->watertop, splishtype);
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
			if (((mobj->eflags & MFE_VERTICALFLIP && thingtop-(height>>1)-mobj->momz > mobj->waterbottom)
				|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z+(height>>1)-mobj->momz < mobj->watertop))
				&& !(mobj->eflags & MFE_UNDERWATER)) // underwater check to prevent splashes on opposite side
			{
				// Spawn a splash
				mobj_t *splish;
				mobjtype_t splishtype = (mobj->eflags & MFE_TOUCHLAVA) ? MT_LAVASPLISH : MT_SPLISH;
				if (mobj->eflags & MFE_VERTICALFLIP)
				{
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->waterbottom-FixedMul(mobjinfo[splishtype].height, mobj->scale), splishtype);
					splish->flags2 |= MF2_OBJECTFLIP;
					splish->eflags |= MFE_VERTICALFLIP;
				}
				else
					splish = P_SpawnMobj(mobj->x, mobj->y, mobj->watertop, splishtype);
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
			else if (mobj->eflags & MFE_TOUCHLAVA)
				S_StartSound(mobj, sfx_splash);
			else
				S_StartSound(mobj, sfx_splish); // And make a sound!

			bubblecount = FixedDiv(abs(mobj->momz), mobj->scale)>>(FRACBITS-1);
			// Max bubble count
			if (bubblecount > 128)
				bubblecount = 128;

			// Create tons of bubbles
			for (i = 0; i < bubblecount; i++)
			{
				// P_RandomByte()s are called individually to allow consistency
				// across various compilers, since the order of function calls
				// in C is not part of the ANSI specification.
				prandom[0] = P_RandomByte();
				prandom[1] = P_RandomByte();
				prandom[2] = P_RandomByte();
				prandom[3] = P_RandomByte();

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
	mobj->watertop = mobj->waterbottom = mobj->z - 1000*FRACUNIT;

	// see if we are in water, and set some flags for later
	sector = mobj->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight, bottomheight;

		mobj->eflags &= ~(MFE_UNDERWATER|MFE_TOUCHWATER);

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKOTHERS)
				continue;

			topheight    = P_GetFFloorTopZAt   (rover, mobj->x, mobj->y);
			bottomheight = P_GetFFloorBottomZAt(rover, mobj->x, mobj->y);

			if (topheight <= mobj->z
				|| bottomheight > (mobj->z + (mobj->height>>1)))
				continue;

			if (mobj->z + mobj->height > topheight)
				mobj->eflags |= MFE_TOUCHWATER;
			else
				mobj->eflags &= ~MFE_TOUCHWATER;

			// Set the watertop and waterbottom
			mobj->watertop = topheight;
			mobj->waterbottom = bottomheight;

			if (mobj->z + (mobj->height>>1) < topheight)
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
	fixed_t halfheight = thiscam->z + (thiscam->height >> 1);

	// see if we are in water
	sector = thiscam->subsector->sector;

	if (P_FindSpecialLineFromTag(13, sector->tag, -1) != -1)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (halfheight >= P_GetFFloorTopZAt(rover, thiscam->x, thiscam->y))
				continue;
			if (halfheight <= P_GetFFloorBottomZAt(rover, thiscam->x, thiscam->y))
				continue;

			if (P_FindSpecialLineFromTag(13, rover->master->frontsector->tag, -1) != -1)
				return true;
		}
	}

	return false;
}

static boolean P_CameraCheckWater(camera_t *thiscam)
{
	sector_t *sector;
	fixed_t halfheight = thiscam->z + (thiscam->height >> 1);

	// see if we are in water
	sector = thiscam->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS) || !(rover->flags & FF_SWIMMABLE) || rover->flags & FF_BLOCKOTHERS)
				continue;

			if (halfheight >= P_GetFFloorTopZAt(rover, thiscam->x, thiscam->y))
				continue;
			if (halfheight <= P_GetFFloorBottomZAt(rover, thiscam->x, thiscam->y))
				continue;

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

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)think;
		if (mo->health <= 0 || !(mo->flags & (MF_ENEMY|MF_BOSS)))
			continue; // not a valid enemy

		if (mo->type == MT_PLAYER) // Don't chase after other players!
			continue;

		// Found a target enemy
		P_KillMobj(mo, players[consoleplayer].mo, players[consoleplayer].mo, 0);
	}
}

// the below is chasecam only, if you're curious. check out P_CalcPostImg in p_user.c for first person
void P_CalcChasePostImg(player_t *player, camera_t *thiscam)
{
	postimg_t postimg = postimg_none;

	if (player->pflags & PF_FLIPCAM && !(player->powers[pw_carry] == CR_NIGHTSMODE) && player->mo->eflags & MFE_VERTICALFLIP)
		postimg = postimg_flip;
	else if (player->awayviewtics && player->awayviewmobj && !P_MobjWasRemoved(player->awayviewmobj)) // Camera must obviously exist
	{
		camera_t dummycam;
		dummycam.subsector = player->awayviewmobj->subsector;
		dummycam.x = player->awayviewmobj->x;
		dummycam.y = player->awayviewmobj->y;
		dummycam.z = player->awayviewmobj->z;
		//dummycam.height = 40*FRACUNIT; // alt view height is 20*FRACUNIT
		dummycam.height = 0;			 // Why? Remote viewpoint cameras have no height.
		// Are we in water?
		if (P_CameraCheckWater(&dummycam))
			postimg = postimg_water;
		else if (P_CameraCheckHeat(&dummycam))
			postimg = postimg_heat;
	}
	else
	{
		// Are we in water?
		if (P_CameraCheckWater(thiscam))
			postimg = postimg_water;
		else if (P_CameraCheckHeat(thiscam))
			postimg = postimg_heat;
	}

	if (postimg == postimg_none)
		return;

	if (splitscreen && player == &players[secondarydisplayplayer])
		postimgtype2 = postimg;
	else
		postimgtype = postimg;
}

// P_CameraThinker
//
// Process the mobj-ish required functions of the camera
boolean P_CameraThinker(player_t *player, camera_t *thiscam, boolean resetcalled)
{
	boolean itsatwodlevel = false;
	if (twodlevel
		|| (thiscam == &camera && players[displayplayer].mo && (players[displayplayer].mo->flags2 & MF2_TWOD))
		|| (thiscam == &camera2 && players[secondarydisplayplayer].mo && (players[secondarydisplayplayer].mo->flags2 & MF2_TWOD)))
		itsatwodlevel = true;

	P_CalcChasePostImg(player, thiscam);

	if (thiscam->momx || thiscam->momy)
	{
		if (!P_TryCameraMove(thiscam->x + thiscam->momx, thiscam->y + thiscam->momy, thiscam)) // Thanks for the greatly improved camera, Lach -- Sev
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
			{
				fixed_t camspeed = P_AproxDistance(thiscam->momx, thiscam->momy);

				P_SlideCameraMove(thiscam);

				if (!resetcalled && P_AproxDistance(thiscam->momx, thiscam->momy) == camspeed)
				{
					P_ResetCamera(player, thiscam);
					resetcalled = true;
				}
			}
			if (resetcalled) // Okay this means the camera is fully reset.
				return true;
		}
	}

	if (!itsatwodlevel)
		P_CheckCameraPosition(thiscam->x, thiscam->y, thiscam);

	thiscam->subsector = R_PointInSubsector(thiscam->x, thiscam->y);
	thiscam->floorz = tmfloorz;
	thiscam->ceilingz = tmceilingz;

	if (thiscam->momz || player->mo->pmomz)
	{
		// adjust height
		thiscam->z += thiscam->momz + player->mo->pmomz;

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

static void P_CheckCrumblingPlatforms(mobj_t *mobj)
{
	msecnode_t *node;

	if (netgame && mobj->player->spectator)
		return;

	for (node = mobj->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_CRUMBLE))
				continue;

			if (mobj->eflags & MFE_VERTICALFLIP)
			{
				if (P_GetSpecialBottomZ(mobj, sectors + rover->secnum, node->m_sector) != mobj->z + mobj->height)
					continue;
			}
			else
			{
				if (P_GetSpecialTopZ(mobj, sectors + rover->secnum, node->m_sector) != mobj->z)
					continue;
			}

			EV_StartCrumble(rover->master->frontsector, rover, (rover->flags & FF_FLOATBOB), mobj->player, rover->alpha, !(rover->flags & FF_NORETURN));
		}
	}
}

static boolean P_MobjTouchesSectorWithWater(mobj_t *mobj)
{
	msecnode_t *node;

	for (node = mobj->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_SWIMMABLE))
				continue;

			return true;
		}
	}

	return false;
}

// Check for floating water platforms and bounce them
static void P_CheckFloatbobPlatforms(mobj_t *mobj)
{
	msecnode_t *node;

	// Can't land on anything if you're not moving downwards
	if (P_MobjFlip(mobj)*mobj->momz >= 0)
		return;

	if (!P_MobjTouchesSectorWithWater(mobj))
		return;

	for (node = mobj->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		ffloor_t *rover;

		if (!node->m_sector->ffloors)
			continue;

		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_FLOATBOB))
				continue;


			if (mobj->eflags & MFE_VERTICALFLIP)
			{
				if (abs(*rover->bottomheight - (mobj->z + mobj->height)) > abs(mobj->momz))
					continue;
			}
			else
			{
				if (abs(*rover->topheight - mobj->z) > abs(mobj->momz))
					continue;
			}

			// Initiate a 'bouncy' elevator function which slowly diminishes.
			EV_BounceSector(rover->master->frontsector, -mobj->momz, rover->master);
		}
	}
}

static void P_PlayerMobjThinker(mobj_t *mobj)
{
	I_Assert(mobj != NULL);
	I_Assert(mobj->player != NULL);
	I_Assert(!P_MobjWasRemoved(mobj));

	P_MobjCheckWater(mobj);

	P_ButteredSlope(mobj);

	// momentum movement
	mobj->eflags &= ~MFE_JUSTSTEPPEDDOWN;

	if (mobj->state-states == S_PLAY_BOUNCE_LANDING)
		goto animonly; // no need for checkposition - doesn't move at ALL

	// Zoom tube
	if (mobj->tracer)
	{
		if (mobj->player->powers[pw_carry] == CR_ZOOMTUBE || mobj->player->powers[pw_carry] == CR_ROPEHANG)
		{
			P_UnsetThingPosition(mobj);
			mobj->x += mobj->momx;
			mobj->y += mobj->momy;
			mobj->z += mobj->momz;
			P_SetThingPosition(mobj);
			P_CheckPosition(mobj, mobj->x, mobj->y);
			mobj->floorz = tmfloorz;
			mobj->ceilingz = tmceilingz;
			goto animonly;
		}
		else if (mobj->player->powers[pw_carry] == CR_MACESPIN)
		{
			P_CheckPosition(mobj, mobj->x, mobj->y);
			mobj->floorz = tmfloorz;
			mobj->ceilingz = tmceilingz;
			goto animonly;
		}
	}

	// Needed for gravity boots
	P_CheckGravity(mobj, false);

	mobj->player->powers[pw_justlaunched] = 0;
	if (mobj->momx || mobj->momy)
	{
		P_XYMovement(mobj);

		if (P_MobjWasRemoved(mobj))
			return;
	}
	else
		P_TryMove(mobj, mobj->x, mobj->y, true);

	P_CheckCrumblingPlatforms(mobj);

	if (CheckForFloatBob)
		P_CheckFloatbobPlatforms(mobj);

	// always do the gravity bit now, that's simpler
	// BUT CheckPosition only if wasn't done before.
	if (!(mobj->eflags & MFE_ONGROUND) || mobj->momz
		|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height != mobj->ceilingz)
		|| (!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z != mobj->floorz)
		|| P_IsObjectInGoop(mobj))
	{
		P_PlayerZMovement(mobj);
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!

		if (P_MobjWasRemoved(mobj))
			return;
	}
	else
	{
#if 0 // i don't know why this is here, it's causing a few undesired state glitches, and disabling it doesn't appear to negatively affect the game, but i don't want it gone permanently just in case some obscure bug crops up
		if (!(mobj->player->powers[pw_carry] == CR_NIGHTSMODE)) // used for drilling
			mobj->player->pflags &= ~PF_STARTJUMP;
		mobj->player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE);
		if (mobj->player->secondjump || mobj->player->powers[pw_tailsfly])
		{
			mobj->player->secondjump = 0;
			mobj->player->powers[pw_tailsfly] = 0;
			P_SetPlayerMobjState(mobj, S_PLAY_WALK);
		}
#endif
		mobj->eflags &= ~MFE_JUSTHITFLOOR;
	}

animonly:
	P_CyclePlayerMobjState(mobj);
}

static void CalculatePrecipFloor(precipmobj_t *mobj)
{
	// recalculate floorz each time
	const sector_t *mobjsecsubsec;
	if (mobj && mobj->subsector && mobj->subsector->sector)
		mobjsecsubsec = mobj->subsector->sector;
	else
		return;
	mobj->floorz = P_GetSectorFloorZAt(mobjsecsubsec, mobj->x, mobj->y);
	if (mobjsecsubsec->ffloors)
	{
		ffloor_t *rover;
		fixed_t topheight;

		for (rover = mobjsecsubsec->ffloors; rover; rover = rover->next)
		{
			// If it exists, it'll get rained on.
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (!(rover->flags & FF_BLOCKOTHERS) && !(rover->flags & FF_SWIMMABLE))
				continue;

			topheight = P_GetFFloorTopZAt(rover, mobj->x, mobj->y);
			if (topheight > mobj->floorz)
				mobj->floorz = topheight;
		}
	}
}

void P_RecalcPrecipInSector(sector_t *sector)
{
	mprecipsecnode_t *psecnode;

	if (!sector)
		return;

	sector->moved = true; // Recalc lighting and things too, maybe

	for (psecnode = sector->touching_preciplist; psecnode; psecnode = psecnode->m_thinglist_next)
		CalculatePrecipFloor(psecnode->m_thing);
}

//
// P_NullPrecipThinker
//
// For "Blank" precipitation
//
void P_NullPrecipThinker(precipmobj_t *mobj)
{
	//(void)mobj;
	mobj->precipflags &= ~PCF_THUNK;
}

void P_SnowThinker(precipmobj_t *mobj)
{
	P_CycleStateAnimation((mobj_t *)mobj);

	// adjust height
	if ((mobj->z += mobj->momz) <= mobj->floorz)
		mobj->z = mobj->ceilingz;
}

void P_RainThinker(precipmobj_t *mobj)
{
	P_CycleStateAnimation((mobj_t *)mobj);

	if (mobj->state != &states[S_RAIN1])
	{
		// cycle through states,
		// calling action functions at transitions
		if (mobj->tics <= 0)
			return;

		if (--mobj->tics)
			return;

		if (!P_SetPrecipMobjState(mobj, mobj->state->nextstate))
			return;

		if (mobj->state != &states[S_RAINRETURN])
			return;

		mobj->z = mobj->ceilingz;
		P_SetPrecipMobjState(mobj, S_RAIN1);

		return;
	}

	// adjust height
	if ((mobj->z += mobj->momz) > mobj->floorz)
		return;

	// no splashes on sky or bottomless pits
	if (mobj->precipflags & PCF_PIT)
	{
		mobj->z = mobj->ceilingz;
		return;
	}

	mobj->z = mobj->floorz;
	P_SetPrecipMobjState(mobj, S_SPLASH1);
}

static void P_KillRingsInLava(mobj_t *mo)
{
	msecnode_t *node;
	I_Assert(mo != NULL);
	I_Assert(!P_MobjWasRemoved(mo));

	// go through all sectors being touched by the ring
	for (node = mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (!node->m_sector)
			break;

		if (node->m_sector->ffloors)
		{
			ffloor_t *rover;
			fixed_t topheight, bottomheight;

			for (rover = node->m_sector->ffloors; rover; rover = rover->next) // go through all fofs in the sector
			{
				if (!(rover->flags & FF_EXISTS)) continue; // fof must be real

				if (!(rover->flags & FF_SWIMMABLE // fof must be water
					&& GETSECSPECIAL(rover->master->frontsector->special, 1) == 3)) // fof must be lava water
					continue;

				// find heights of FOF
				topheight = P_GetFOFTopZ(mo, node->m_sector, rover, mo->x, mo->y, NULL);
				bottomheight = P_GetFOFBottomZ(mo, node->m_sector, rover, mo->x, mo->y, NULL);

				if (mo->z <= topheight && mo->z + mo->height >= bottomheight) // if ring touches it, KILL IT
				{
					P_KillMobj(mo, NULL, NULL, DMG_FIRE);
					return;
				}
			}
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
		actor->lastlook = P_RandomByte();
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

		if (player->pflags & PF_INVIS || player->bot || player->spectator)
			continue; // ignore notarget

		if (!player->mo || P_MobjWasRemoved(player->mo))
			continue;

		if (player->mo->health <= 0)
			continue; //dead

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
			if (players[c].pflags & PF_INVIS)
				continue; // ignore notarget

			if (!players[c].mo || players[c].bot)
				continue;

			if (players[c].mo->health <= 0)
				continue; // dead

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
			return;

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
	if (mobj->flags2 & MF2_FRET && mobj->state->nextstate == mobj->info->spawnstate && mobj->tics == 1) {
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
			return;

		// look for a new target
		if (P_BossTargetPlayer(mobj, false) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	if (mobj->flags2 & MF2_SKULLFLY)
	{
		fixed_t dist = (mobj->eflags & MFE_VERTICALFLIP)
			? ((mobj->ceilingz-(2*mobj->height)) - (mobj->z+mobj->height))
			: (mobj->z - (mobj->floorz+(2*mobj->height)));
		if (dist > 0 && P_MobjFlip(mobj)*mobj->momz > 0)
			mobj->momz = FixedMul(mobj->momz, FRACUNIT - (dist>>12));
	}
	else if (mobj->state != &states[mobj->info->spawnstate] && mobj->health > 0 && mobj->flags & MF_FLOAT)
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

	if (!mobj->tracer)
	{
		var1 = 0;
		A_BossJetFume(mobj);
	}

	if (mobj->health <= mobj->info->damage && (!mobj->target || !(mobj->target->flags & MF_SHOOTABLE)))
	{
		if (mobj->health <= 0)
			return;

		// look for a new target
		if (P_BossTargetPlayer(mobj, false) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);

		return;
	}

	if (mobj->state == &states[mobj->info->spawnstate] && mobj->health > mobj->info->damage)
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

	if (mobj->health <= 0)
		return;
	/*
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
		else
		{
			mobj->flags |= MF_NOGRAVITY|MF_NOCLIP;
			mobj->flags |= MF_NOCLIPHEIGHT;
			mobj->threshold = -1;
			return;
		}
	}*/

	if (mobj->reactiontime) // At the bottom of the water
	{
		UINT32 i;
		SINT8 curpath = mobj->threshold;

		// Choose one of the paths you're not already on
		mobj->threshold = P_RandomKey(8-1);
		if (mobj->threshold >= curpath)
			mobj->threshold++;

		if (mobj->state != &states[mobj->info->spawnstate])
			P_SetMobjState(mobj, mobj->info->spawnstate);

		mobj->reactiontime--;

		if (!mobj->reactiontime && mobj->health <= mobj->info->damage)
		{ // Spawn pinch dummies from the center when we're leaving it.
			thinker_t *th;
			mobj_t *mo2;
			mobj_t *dummy;
			SINT8 way0 = mobj->threshold; // 0 through 4.
			SINT8 way1, way2;

			i = 0; // reset i to 0 so we can check how many clones we've removed

			// scan the thinkers to make sure all the old pinch dummies are gone before making new ones
			// this can happen if the boss was hurt earlier than expected
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo2 = (mobj_t *)th;
				if (mo2->type != (mobjtype_t)mobj->info->mass)
					continue;
				if (mo2->tracer != mobj)
					continue;

				P_RemoveMobj(mo2);
				if (++i == 2) // we've already removed 2 of these, let's stop now
					break;
			}

			way1 = P_RandomKey(8-2);
			if (way1 >= curpath)
				way1++;
			if (way1 >= way0)
			{
				way1++;
				if (way1 == curpath)
					way1++;
			}

			dummy = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->info->mass);
			dummy->angle = mobj->angle;
			dummy->threshold = way1;
			P_SetTarget(&dummy->tracer, mobj);
			dummy->movefactor = mobj->movefactor;
			dummy->cusval = mobj->cusval;

			way2 = P_RandomKey(8-3);
			if (way2 >= curpath)
				way2++;
			if (way2 >= way0)
			{
				way2++;
				if (way2 == curpath)
					way2++;
			}
			if (way2 >= way1)
			{
				way2++;
				if (way2 == curpath || way2 == way0)
					way2++;
			}

			dummy = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->info->mass);
			dummy->angle = mobj->angle;
			dummy->threshold = way2;
			P_SetTarget(&dummy->tracer, mobj);
			dummy->movefactor = mobj->movefactor;
			dummy->cusval = mobj->cusval;

			CONS_Debug(DBG_GAMELOGIC, "Eggman path %d - Dummy selected paths %d and %d\n", way0, way1, way2);
			P_LinedefExecute(LE_PINCHPHASE+(mobj->cusval*LE_PARAMWIDTH), mobj, NULL);
		}
	}
	else if (mobj->movecount) // Firing mode
	{
		// Check if the attack animation is running. If not, play it.
		if (mobj->state < &states[mobj->info->missilestate] || mobj->state > &states[mobj->info->raisestate])
		{
			// look for a new target
			P_BossTargetPlayer(mobj, true);

			if (!mobj->target || !mobj->target->player)
				return;

			if (mobj->health <= mobj->info->damage) // pinch phase
				mobj->movecount--; // limited number of shots before diving again
			if (mobj->movecount)
				P_SetMobjState(mobj, mobj->info->missilestate+1);
		}
		else if (mobj->target && mobj->target->player)
		{
			angle_t diff = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y) - mobj->angle;
			if (diff > ANGLE_180)
				diff = InvAngle(InvAngle(diff)/4);
			else
				diff /= 4;
			mobj->angle += diff;
		}
	}
	else if (mobj->threshold >= 0) // Traveling mode
	{
		fixed_t dist = 0;
		fixed_t speed;

		P_SetTarget(&mobj->target, NULL);

		if (mobj->state != &states[mobj->info->spawnstate] && mobj->health > 0
			&& !(mobj->flags2 & MF2_FRET))
			P_SetMobjState(mobj, mobj->info->spawnstate);

		if (!(mobj->flags2 & MF2_STRONGBOX))
		{
			thinker_t *th;
			mobj_t *mo2;

			P_SetTarget(&mobj->tracer, NULL);

			// scan the thinkers
			// to find a point that matches
			// the number
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo2 = (mobj_t *)th;
				if (mo2->type != MT_BOSS3WAYPOINT)
					continue;
				if (!mo2->spawnpoint)
					continue;
				if (mo2->spawnpoint->angle != mobj->threshold)
					continue;
				if (mo2->spawnpoint->extrainfo != mobj->cusval)
					continue;

				P_SetTarget(&mobj->tracer, mo2);
				break;
			}
		}

		if (!mobj->tracer) // Should NEVER happen
		{
			CONS_Debug(DBG_GAMELOGIC, "Error: Boss 3 was unable to find specified waypoint: %d, %d\n", mobj->threshold, mobj->cusval);
			return;
		}

		if ((mobj->movedir) || (mobj->health <= mobj->info->damage))
			speed = mobj->info->speed * 2;
		else
			speed = mobj->info->speed;

		if (mobj->tracer->x == mobj->x && mobj->tracer->y == mobj->y)
		{
			// apply ambush for old routing, otherwise whack a mole only
			dist = P_AproxDistance(P_AproxDistance(mobj->tracer->x - mobj->x, mobj->tracer->y - mobj->y), mobj->tracer->z + mobj->movefactor - mobj->z);

			if (dist < 1)
				dist = 1;

			mobj->momx = FixedMul(FixedDiv(mobj->tracer->x - mobj->x, dist), speed);
			mobj->momy = FixedMul(FixedDiv(mobj->tracer->y - mobj->y, dist), speed);
			mobj->momz = FixedMul(FixedDiv(mobj->tracer->z + mobj->movefactor - mobj->z, dist), speed);

			if (mobj->momx != 0 || mobj->momy != 0)
				mobj->angle = R_PointToAngle2(0, 0, mobj->momx, mobj->momy);
		}

		if (dist <= speed)
		{
			// If distance to point is less than travel in that frame, set XYZ of mobj to waypoint location
			P_UnsetThingPosition(mobj);
			mobj->x = mobj->tracer->x;
			mobj->y = mobj->tracer->y;
			mobj->z = mobj->tracer->z + mobj->movefactor;
			mobj->momx = mobj->momy = mobj->momz = 0;
			P_SetThingPosition(mobj);

			if (!mobj->movefactor) // to firing mode
			{
				UINT8 i, numtospawn = 24;
				angle_t ang = 0, interval = FixedAngle((360 << FRACBITS) / numtospawn);
				mobj_t *shock = NULL, *sfirst = NULL, *sprev = NULL;

				mobj->movecount = mobj->health+1;
				mobj->movefactor = -512*FRACUNIT;

				// shock the water!
				for (i = 0; i < numtospawn; i++)
				{
					shock = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_SHOCKWAVE);
					P_SetTarget(&shock->target, mobj);
					shock->fuse = shock->info->painchance;

					if (i % 2 == 0)
					P_SetMobjState(shock, shock->state->nextstate);

					if (!sprev)
						sfirst = shock;
					else
					{
						if (i == numtospawn - 1)
							P_SetTarget(&shock->hnext, sfirst);
						P_SetTarget(&sprev->hnext, shock);
					}

					P_Thrust(shock, ang, shock->info->speed);
					ang += interval;
					sprev = shock;
				}
				S_StartSound(mobj, shock->info->seesound);

				// look for a new target
				P_BossTargetPlayer(mobj, true);

				if (mobj->target && mobj->target->player)
					P_SetMobjState(mobj, mobj->info->missilestate);
			}
			else if (mobj->flags2 & (MF2_STRONGBOX|MF2_CLASSICPUSH)) // just hit the bottom of your tube
			{
				mobj->flags2 &= ~(MF2_STRONGBOX|MF2_CLASSICPUSH);
				mobj->reactiontime = 1; // spawn pinch dummies
				mobj->movedir = 0;
			}
			else // just shifted to another tube
			{
				mobj->flags2 |= MF2_STRONGBOX;
				if (mobj->health > 0)
					mobj->movefactor = 0;
			}
		}
	}
}

// Move Boss4's sectors by delta.
static boolean P_Boss4MoveCage(mobj_t *mobj, fixed_t delta)
{
	const UINT16 tag = 65534 + (mobj->spawnpoint ? mobj->spawnpoint->extrainfo*LE_PARAMWIDTH : 0);
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
	fixed_t dist, bz = mobj->watertop+(8<<FRACBITS);
	while ((base = base->tracer))
	{
		for (seg = base, dist = 172*FRACUNIT, s = 9; seg; seg = seg->hnext, dist += 124*FRACUNIT, --s)
			P_TeleportMove(seg, mobj->x + P_ReturnThrustX(mobj, angle, dist), mobj->y + P_ReturnThrustY(mobj, angle, dist), bz + FixedMul(fz, FixedDiv(s<<FRACBITS, 9<<FRACBITS)));
		angle += ANGLE_MAX/3;
	}
}

#define CEZ3TILT

// Pull them closer.
static void P_Boss4PinchSpikeballs(mobj_t *mobj, angle_t angle, fixed_t dz)
{
	INT32 s;
	mobj_t *base = mobj, *seg;
	fixed_t workx, worky, dx, dy, bz = mobj->watertop+(8<<FRACBITS);
	fixed_t rad = (9*132)<<FRACBITS;
#ifdef CEZ3TILT
	fixed_t originx, originy;
	if (mobj->spawnpoint)
	{
		originx = mobj->spawnpoint->x << FRACBITS;
		originy = mobj->spawnpoint->y << FRACBITS;
	}
	else
	{
		originx = mobj->x;
		originy = mobj->y;
	}
#else
	if (mobj->spawnpoint)
	{
		rad -= R_PointToDist2(mobj->x, mobj->y,
			(mobj->spawnpoint->x<<FRACBITS), (mobj->spawnpoint->y<<FRACBITS));
	}
#endif

	dz /= 9;

	while ((base = base->tracer)) // there are 10 per spoke, remember that
	{
#ifdef CEZ3TILT
		dx = (originx + P_ReturnThrustX(mobj, angle, rad) - mobj->x)/9;
		dy = (originy + P_ReturnThrustY(mobj, angle, rad) - mobj->y)/9;
#else
		dx = P_ReturnThrustX(mobj, angle, rad)/9;
		dy = P_ReturnThrustY(mobj, angle, rad)/9;
#endif
		workx = mobj->x + P_ReturnThrustX(mobj, angle, (112)<<FRACBITS);
		worky = mobj->y + P_ReturnThrustY(mobj, angle, (112)<<FRACBITS);
		for (seg = base, s = 9; seg; seg = seg->hnext, --s)
		{
			seg->z = bz + (dz*(9-s));
			P_TryMove(seg, workx + (dx*s), worky + (dy*s), true);
		}
		angle += ANGLE_MAX/3;
	}
}

// Destroy cage FOFs.
static void P_Boss4DestroyCage(mobj_t *mobj)
{
	const UINT16 tag = 65534 + (mobj->spawnpoint ? mobj->spawnpoint->extrainfo*LE_PARAMWIDTH : 0);
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
				P_KillMobj(seg, NULL, NULL, 0);
		base = next;
	}
}

//
// AI for the fourth boss.
//
static void P_Boss4Thinker(mobj_t *mobj)
{
	fixed_t movespeed = 0;

	if ((statenum_t)(mobj->state-states) == mobj->info->spawnstate)
	{
		if (mobj->flags2 & MF2_FRET && (mobj->health > mobj->info->damage))
			mobj->flags2 &= ~MF2_FRET;
		mobj->reactiontime = 0; // Drop the cage immediately.
	}

	// Oh no, we dead? D:
	if (!mobj->health)
	{
		if (mobj->tracer) // need to clean up!
		{
			P_Boss4DestroyCage(mobj); // Just in case pinch phase was skipped.
			P_Boss4PopSpikeballs(mobj);
		}
		return;
	}

	if (mobj->movedir) // only not during init
	{
		INT32 oldmovecount = mobj->movecount;
		if (mobj->movedir == 3) // pinch start
			movespeed = -(210<<(FRACBITS>>1));
		else if (mobj->movedir > 3) // pinch
		{
			movespeed = 420<<(FRACBITS>>1);
			movespeed += (420*(mobj->info->damage-mobj->health)<<(FRACBITS>>1));
			if (mobj->movedir == 4)
				movespeed = -movespeed;
		}
		else // normal
		{
			movespeed = 170<<(FRACBITS>>1);
			movespeed += ((50*(mobj->info->spawnhealth-mobj->health))<<(FRACBITS>>1));
			if (mobj->movedir == 2)
				movespeed = -movespeed;
			if (mobj->movefactor)
				movespeed /= 2;
			else if (mobj->threshold)
			{
				// 1 -> 1.5 second timer
				INT32 maxtimer = TICRATE+(TICRATE*(mobj->info->spawnhealth-mobj->health)/10);
				if (maxtimer < 1)
					maxtimer = 1;
				maxtimer = ((mobj->threshold*movespeed)/(2*maxtimer));
				movespeed -= maxtimer;
			}
		}

		mobj->movecount += movespeed + 360*FRACUNIT;
		mobj->movecount %= 360*FRACUNIT;

		if (((oldmovecount>>FRACBITS)%120 >= 60) && !((mobj->movecount>>FRACBITS)%120 >= 60))
			S_StartSound(NULL, sfx_mswing);
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
			mobj->watertop = mobj->z;
			z = mobj->z + mobj->height/2 - mobjinfo[MT_EGGMOBILE4_MACE].height/2;
			for (arm = 0; arm <3 ; arm++)
			{
				seg = P_SpawnMobj(mobj->x, mobj->y, z, MT_EGGMOBILE4_MACE);
				P_SetTarget(&base->tracer, seg);
				base = seg;
				P_SetTarget(&seg->target, mobj);
				for (i = 0; i < 9; i++)
				{
					P_SetTarget(&seg->hnext, P_SpawnMobj(mobj->x, mobj->y, z, MT_EGGMOBILE4_MACE));
					P_SetTarget(&seg->hnext->hprev, seg);
					seg = seg->hnext;
				}
			}
			// Move the cage up to the sky.
			mobj->movecount = 800*FRACUNIT;
			if (!P_Boss4MoveCage(mobj, mobj->movecount))
			{
				mobj->movecount = 0;
				//mobj->threshold = 3*TICRATE;
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
			if (mobj->movecount <= 0)
			{
				mobj->flags2 &= ~MF2_INVERTAIMABLE;
				mobj->movecount = 0;
				mobj->movedir++; // Initialization complete, next phase!
			}
			P_Boss4MoveCage(mobj, mobj->movecount - oldz);
			P_Boss4MoveSpikeballs(mobj, 0, mobj->movecount);
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
		if (mobj->z < mobj->watertop+(400<<FRACBITS))
			mobj->momz = 8*FRACUNIT;
		else
		{
			mobj->momz = mobj->movefactor = 0;
			mobj->threshold = 1110<<FRACBITS;
			S_StartSound(NULL, sfx_s3k60);
			mobj->movedir++;
		}

		z = mobj->z - mobj->watertop - mobjinfo[MT_EGGMOBILE4_MACE].height - mobj->height/2;
		if (z < (8<<FRACBITS)) // We haven't risen high enough to pull the spikeballs along yet
			P_Boss4MoveSpikeballs(mobj, FixedAngle(mobj->movecount), 0); // So don't pull the spikeballs along yet.
		else
			P_Boss4PinchSpikeballs(mobj, FixedAngle(mobj->movecount), z);
		return;
	}
	// Pinch phase!
	case 4:
	case 5:
	{
		mobj->angle -= FixedAngle(movespeed/8);

		if (mobj->movefactor != mobj->threshold)
		{
			if (mobj->threshold - mobj->movefactor < FRACUNIT)
			{
				mobj->movefactor = mobj->threshold;
				mobj->flags2 &= ~MF2_FRET;
			}
			else
				mobj->movefactor += (mobj->threshold - mobj->movefactor)/8;
		}

		if (mobj->spawnpoint)
			P_TryMove(mobj,
				(mobj->spawnpoint->x<<FRACBITS) - P_ReturnThrustX(mobj, mobj->angle, mobj->movefactor),
				(mobj->spawnpoint->y<<FRACBITS) - P_ReturnThrustY(mobj, mobj->angle, mobj->movefactor),
				true);

		P_Boss4PinchSpikeballs(mobj, FixedAngle(mobj->movecount), mobj->z - mobj->watertop - mobjinfo[MT_EGGMOBILE4_MACE].height - mobj->height/2);

		if (!mobj->target || !mobj->target->health)
			P_SupermanLook4Players(mobj);
		//A_FaceTarget(mobj);
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
		if (mobj->movefactor != 128*FRACUNIT)
		{
			if (mobj->movefactor < 128*FRACUNIT)
			{
				mobj->movefactor += 8*FRACUNIT;
				if (!oldz)
				{
					// 5 -> 2.5 second timer
					mobj->threshold = 5*TICRATE-(TICRATE*(mobj->info->spawnhealth-mobj->health)/2);
					if (mobj->threshold < 1)
						mobj->threshold = 1;
				}
			}
			else
				mobj->movefactor = 128*FRACUNIT;
			P_Boss4MoveCage(mobj, mobj->movefactor - oldz);
		}
	}
	// Drop the cage!
	else if (mobj->movefactor)
	{
		fixed_t oldz = mobj->movefactor;
		mobj->movefactor -= 4*FRACUNIT;
		if (mobj->movefactor < 0)
			mobj->movefactor = 0;
		P_Boss4MoveCage(mobj, mobj->movefactor - oldz);
		if (!mobj->movefactor)
		{
			if (mobj->health <= mobj->info->damage)
			{ // Proceed to pinch phase!
				P_Boss4DestroyCage(mobj);
				mobj->movedir = 3;
				P_LinedefExecute(LE_PINCHPHASE + (mobj->spawnpoint ? mobj->spawnpoint->extrainfo*LE_PARAMWIDTH : 0), mobj, NULL);
				P_Boss4MoveSpikeballs(mobj, FixedAngle(mobj->movecount), 0);
				var1 = 3;
				A_BossJetFume(mobj);
				return;
			}
			P_LinedefExecute(LE_BOSS4DROP - (mobj->info->spawnhealth-mobj->health) + (mobj->spawnpoint ? mobj->spawnpoint->extrainfo*LE_PARAMWIDTH : 0), mobj, NULL);
			// 1 -> 1.5 second timer
			mobj->threshold = TICRATE+(TICRATE*(mobj->info->spawnhealth-mobj->health)/10);
			if (mobj->threshold < 1)
				mobj->threshold = 1;
		}
	}

	P_Boss4MoveSpikeballs(mobj, FixedAngle(mobj->movecount), mobj->movefactor);

	// Check for attacks, always tick the timer even while animating!!
	if (mobj->threshold)
	{
		if (!(mobj->flags2 & MF2_FRET) && !(--mobj->threshold)) // but pause for pain so we don't interrupt pinch phase, eep!
		{
			if (mobj->reactiontime == 1) // Cage is raised?
			{
				P_SetMobjState(mobj, mobj->info->spawnstate);
				mobj->reactiontime = 0; // Drop it!
			}
		}
	}

	// Leave if animating.
	if ((statenum_t)(mobj->state-states) != mobj->info->spawnstate)
		return;

	// Map allows us to get killed despite cage being down?
	if (mobj->health <= mobj->info->damage)
	{ // Proceed to pinch phase!
		P_Boss4DestroyCage(mobj);
		mobj->movedir = 3;
		P_LinedefExecute(LE_PINCHPHASE + (mobj->spawnpoint ? mobj->spawnpoint->extrainfo*LE_PARAMWIDTH : 0), mobj, NULL);
		var1 = 3;
		A_BossJetFume(mobj);
		return;
	}

	mobj->reactiontime = 0; // Drop the cage if it hasn't been dropped already.
	if (!mobj->target || !mobj->target->health)
		P_SupermanLook4Players(mobj);
	A_FaceTarget(mobj);
}

//
// AI for the fifth boss.
//
static void P_Boss5Thinker(mobj_t *mobj)
{
	if (!mobj->health)
	{
		if (mobj->fuse)
		{
			if (mobj->flags2 & MF2_SLIDEPUSH)
			{
				INT32 trans = 10-((10*mobj->fuse)/70);
				if (trans > 9)
					trans = 9;
				if (trans < 0)
					trans = 0;
				mobj->frame = (mobj->frame & ~FF_TRANSMASK)|(trans<<FF_TRANSSHIFT);
				if (!(mobj->fuse & 1))
				{
					mobj->colorized = !mobj->colorized;
					mobj->frame ^= FF_FULLBRIGHT;
				}
			}
			return;
		}
		if (mobj->state == &states[mobj->info->xdeathstate])
			mobj->momz -= (2*FRACUNIT)/3;
		else if (mobj->tracer && P_AproxDistance(mobj->tracer->x - mobj->x, mobj->tracer->y - mobj->y) < 2*mobj->radius)
			mobj->flags &= ~MF_NOCLIP;
	}
	else
	{
		if (mobj->flags2 & MF2_FRET && (leveltime & 1)
		&& mobj->state != &states[S_FANG_PAIN1] && mobj->state != &states[S_FANG_PAIN2])
			mobj->flags2 |= MF2_DONTDRAW;
		else
			mobj->flags2 &= ~MF2_DONTDRAW;
	}

	if (mobj->state == &states[S_FANG_BOUNCE3]
	||  mobj->state == &states[S_FANG_BOUNCE4]
	||  mobj->state == &states[S_FANG_PINCHBOUNCE3]
	||  mobj->state == &states[S_FANG_PINCHBOUNCE4])
	{
		if (P_MobjFlip(mobj)*mobj->momz > 0
		&& abs(mobj->momx) < FRACUNIT/2 && abs(mobj->momy) < FRACUNIT/2
		&& !P_IsObjectOnGround(mobj))
		{
			mobj_t *prevtarget = mobj->target;
			P_SetTarget(&mobj->target, NULL);
			var1 = var2 = 0;
			A_DoNPCPain(mobj);
			P_SetTarget(&mobj->target, prevtarget);
			P_SetMobjState(mobj, S_FANG_WALLHIT);
			mobj->extravalue2++;
		}
	}
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
		mobj->reactiontime += P_RandomByte();

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

			P_KillMobj(mobj, NULL, NULL, 0);

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
				P_DamageMobj(players[i].mo, mobj, mobj, 1, 0);
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
		UINT8 extrainfo = (mobj->spawnpoint ? mobj->spawnpoint->extrainfo : 0);

		// Looks for players in goop. If you find one, try to jump on him.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (!players[i].mo)
				continue;

			if (players[i].mo->health <= 0)
				continue;

			if (players[i].powers[pw_carry] == CR_BRAKGOOP)
			{
				closestNum = -1;
				closestdist = INT32_MAX; // Just in case...

				// Find waypoint he is closest to
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					mo2 = (mobj_t *)th;
					if (mo2->type != MT_BOSS3WAYPOINT)
						continue;
					if (!mo2->spawnpoint)
						continue;
					if (mo2->spawnpoint->extrainfo != extrainfo)
						continue;
					if (mobj->health <= mobj->info->damage && !(mo2->spawnpoint->options & 7))
						continue; // don't jump to center

					dist = P_AproxDistance(players[i].mo->x - mo2->x, players[i].mo->y - mo2->y);

					if (!(closestNum == -1 || dist < closestdist))
						continue;

					closestNum = (mo2->spawnpoint->options & 7);
					closestdist = dist;
					foundgoop = true;
				}
				waypointNum = closestNum;
				break;
			}
		}

		if (!foundgoop)
		{
			// Don't jump to the center when health is low.
			// Force the player to beat you with missiles.
			if (mobj->z <= 1056*FRACUNIT || mobj->health <= mobj->info->damage)
				waypointNum = 1 + P_RandomKey(4);
			else
				waypointNum = 0;
		}

		if (mobj->tracer && mobj->tracer->type == MT_BOSS3WAYPOINT
			&& mobj->tracer->spawnpoint && (mobj->tracer->spawnpoint->options & 7) == waypointNum)
		{
			if (P_RandomChance(FRACUNIT/2))
				waypointNum++;
			else
				waypointNum--;

			if (mobj->health <= mobj->info->damage)
				waypointNum = ((waypointNum + 3) % 4) + 1; // plus four to avoid modulo being negative, minus one to avoid waypoint #0
			else
				waypointNum = ((waypointNum + 5) % 5);
		}

		// scan the thinkers to find
		// the waypoint to use
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo2 = (mobj_t *)th;
			if (mo2->type != MT_BOSS3WAYPOINT)
				continue;
			if (!mo2->spawnpoint)
				continue;
			if ((mo2->spawnpoint->options & 7) != waypointNum)
				continue;
			if (mo2->spawnpoint->extrainfo != extrainfo)
				continue;

			hitspot = mo2;
			break;
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

			P_DamageMobj(players[i].mo, mobj, mobj, 1, 0);

			// Laugh
			S_StartSound(0, sfx_bewar1 + P_RandomKey(4));
		}

		P_SetMobjState(mobj, mobj->info->spawnstate);
	}
	else if (mobj->state == &states[mobj->info->deathstate] && mobj->tics == mobj->state->tics)
		S_StartSound(0, sfx_bedie1 + (P_RandomFixed() & 1));

}

#define vectorise mobj->movedir = ANGLE_11hh - FixedAngle(FixedMul(AngleFixed(ANGLE_11hh), FixedDiv((mobj->info->spawnhealth - mobj->health)<<FRACBITS, (mobj->info->spawnhealth-1)<<FRACBITS)));\
				if (P_RandomChance(FRACUNIT/2))\
					mobj->movedir = InvAngle(mobj->movedir);\
				mobj->threshold = 6 + (FixedMul(24<<FRACBITS, FixedDiv((mobj->info->spawnhealth - mobj->health)<<FRACBITS, (mobj->info->spawnhealth-1)<<FRACBITS))>>FRACBITS);\
				if (mobj->info->activesound)\
					S_StartSound(mobj, mobj->info->activesound);\
				if (mobj->info->painchance)\
					P_SetMobjState(mobj, mobj->info->painchance);\
				mobj->flags2 &= ~MF2_INVERTAIMABLE;\

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
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo2 = (mobj_t *)th;
			if (mo2->type == MT_BOSS9GATHERPOINT)
			{
				if (last)
					P_SetTarget(&last->hnext, mo2);
				else
					P_SetTarget(&mobj->hnext, mo2);
				P_SetTarget(&mo2->hprev, last);
				last = mo2;
			}
		}
	}

	if (mobj->health <= 0)
		return;

	if ((statenum_t)(mobj->state-states) == mobj->info->meleestate)
	{
		P_InstaThrust(mobj, mobj->angle, -4*FRACUNIT);
		P_TryMove(mobj, mobj->x+mobj->momx, mobj->y+mobj->momy, true);
		mobj->momz -= gravity;
		if (mobj->z < mobj->watertop || mobj->z < (mobj->floorz + 16*FRACUNIT))
		{
			mobj->watertop = mobj->floorz + 32*FRACUNIT;
			P_SetMobjState(mobj, mobj->info->spawnstate);
		}
		return;
	}

	if ((!mobj->target || !(mobj->target->flags & MF_SHOOTABLE)))
	{
		if (mobj->hprev)
		{
			P_RemoveMobj(mobj->hprev);
			P_SetTarget(&mobj->hprev, NULL);
		}
		P_BossTargetPlayer(mobj, false);
		if (mobj->target && (!P_IsObjectOnGround(mobj->target) || mobj->target->player->pflags & PF_SPINNING))
			P_SetTarget(&mobj->target, NULL); // Wait for them to hit the ground first
		if (!mobj->target) // Still no target, aww.
		{
			// Reset the boss.
			if (mobj->hprev)
			{
				P_RemoveMobj(mobj->hprev);
				P_SetTarget(&mobj->hprev, NULL);
			}
			P_SetMobjState(mobj, mobj->info->spawnstate);
			mobj->fuse = 0;
			mobj->momx = FixedDiv(mobj->momx, FRACUNIT + (FRACUNIT>>2));
			mobj->momy = FixedDiv(mobj->momy, FRACUNIT + (FRACUNIT>>2));
			mobj->momz = FixedDiv(mobj->momz, FRACUNIT + (FRACUNIT>>2));
			mobj->watertop = mobj->floorz + 32*FRACUNIT;
			mobj->momz = (mobj->watertop - mobj->z)>>3;
			mobj->threshold = 0;
			mobj->movecount = 0;
			mobj->flags = mobj->info->flags;
			return;
		}
		else if (!mobj->fuse)
			mobj->fuse = 8*TICRATE;
	}

	// AI goes here.
	{
		angle_t angle;
		if (mobj->threshold || mobj->movecount)
			mobj->momz = (mobj->watertop-mobj->z)/16; // Float to your desired position FASTER
		else
			mobj->momz = (mobj->watertop-mobj->z)/40; // Float to your desired position

		if (mobj->movecount == 2)
		{
			mobj_t *spawner;
			fixed_t dist = 0;
			angle = 0x06000000*leveltime; // wtf?

			// Face your target
			P_BossTargetPlayer(mobj, true);
			angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y); // absolute angle
			angle = (angle-mobj->angle); // relative angle
			if (angle < ANGLE_180)
				mobj->angle += angle/8;
			else
				mobj->angle -= InvAngle(angle)/8;

			// Alter your energy bubble's size/position
			if (mobj->health > mobj->info->damage)
			{
				if (mobj->hprev)
				{
					mobj->hprev->destscale = FRACUNIT + (2*TICRATE - mobj->fuse)*(FRACUNIT/2)/TICRATE + FixedMul(FINECOSINE(angle>>ANGLETOFINESHIFT),FRACUNIT/2);
					P_SetScale(mobj->hprev, mobj->hprev->destscale);

					P_TeleportMove(mobj->hprev, mobj->x, mobj->y, mobj->z + mobj->height/2 - mobj->hprev->height/2);
					mobj->hprev->momx = mobj->momx;
					mobj->hprev->momy = mobj->momy;
					mobj->hprev->momz = mobj->momz;
				}

				// Firin' mah lazors - INDICATOR
				if (mobj->fuse > TICRATE/2)
				{
					tic_t shoottime, worktime, calctime;
					shoottime = (TICRATE/((mobj->extravalue1 == 3) ? 8 : 4));
					shoottime += (shoottime>>1);
					worktime = shoottime*(mobj->threshold/2);
					calctime = mobj->fuse-(TICRATE/2);

					if (calctime <= worktime && (calctime % shoottime == 0))
					{
						mobj_t *missile;

						missile = P_SpawnMissile(mobj, mobj->target, MT_MSGATHER);
						S_StopSound(missile);
						if (mobj->extravalue1 >= 2)
							P_SetScale(missile, FRACUNIT>>1);
						missile->destscale = missile->scale>>1;
						missile->fuse = TICRATE/2;
						missile->scalespeed = abs(missile->destscale - missile->scale)/missile->fuse;
						missile->z -= missile->height/2;
						missile->momx *= -1;
						missile->momy *= -1;
						missile->momz *= -1;

						if (mobj->extravalue1 == 2)
						{
							UINT8 i;
							mobj_t *spread;
							for (i = 0; i < 5; i++)
							{
								if (i == 2)
									continue;
								spread = P_SpawnMobj(missile->x, missile->y, missile->z, missile->type);
								spread->angle = missile->angle+(ANGLE_11hh/2)*(i-2);
								P_InstaThrust(spread,spread->angle,-spread->info->speed);
								spread->momz = missile->momz;
								P_SetScale(spread, missile->scale);
								spread->destscale = missile->destscale;
								spread->scalespeed = missile->scalespeed;
								spread->fuse = missile->fuse;
								P_UnsetThingPosition(spread);
								spread->x -= spread->fuse*spread->momx;
								spread->y -= spread->fuse*spread->momy;
								spread->z -= spread->fuse*spread->momz;
								P_SetThingPosition(spread);
							}
							P_InstaThrust(missile,missile->angle,-missile->info->speed);
						}
						else if (mobj->extravalue1 >= 3)
						{
							UINT8 i;
							mobj_t *spread;
							mobj->target->z -= (4*missile->height);
							for (i = 0; i < 5; i++)
							{
								if (i != 2)
								{
									spread = P_SpawnMissile(mobj, mobj->target, missile->type);
									P_SetScale(spread, missile->scale);
									spread->destscale = missile->destscale;
									spread->fuse = missile->fuse;
									spread->z -= spread->height/2;
									spread->momx *= -1;
									spread->momy *= -1;
									spread->momz *= -1;
									P_UnsetThingPosition(spread);
									spread->x -= spread->fuse*spread->momx;
									spread->y -= spread->fuse*spread->momy;
									spread->z -= spread->fuse*spread->momz;
									P_SetThingPosition(spread);
								}
								mobj->target->z += missile->height*2;
							}
							mobj->target->z -= (6*missile->height);
						}

						P_UnsetThingPosition(missile);
						missile->x -= missile->fuse*missile->momx;
						missile->y -= missile->fuse*missile->momy;
						missile->z -= missile->fuse*missile->momz;
						P_SetThingPosition(missile);

						S_StartSound(mobj, sfx_s3kb3);
					}
				}
			}

			// up...
			mobj->z += mobj->height/2;

			// Spawn energy particles
			for (spawner = mobj->hnext; spawner; spawner = spawner->hnext)
			{
				dist = P_AproxDistance(spawner->x - mobj->x, spawner->y - mobj->y);
				if (P_RandomRange(1,(dist>>FRACBITS)/16) == 1)
					break;
			}
			if (spawner)
			{
				mobj_t *missile = P_SpawnMissile(spawner, mobj, MT_MSGATHER);

				if (dist == 0)
					missile->fuse = 0;
				else
					missile->fuse = (dist/P_AproxDistance(missile->momx, missile->momy));

				if (missile->fuse > mobj->fuse)
					P_RemoveMobj(missile);

				if (mobj->health > mobj->info->damage)
				{
					P_SetScale(missile, FRACUNIT/3);
					missile->color = SKINCOLOR_MAGENTA; // sonic OVA/4 purple power
				}
				else
				{
					P_SetScale(missile, FRACUNIT/5);
					missile->color = SKINCOLOR_SUNSET; // sonic cd electric power
				}
				missile->destscale = missile->scale*2;
				missile->scalespeed = abs(missile->scale - missile->destscale)/missile->fuse;
				missile->colorized = true;
			}

			// ...then down. easier than changing the missile's momz after-the-fact
			mobj->z -= mobj->height/2;
		}

		// Pre-threshold reactiontime stuff for attack phases
		if (mobj->reactiontime && mobj->movecount == 3)
		{
			mobj->reactiontime--;

			if (mobj->movedir == 0 || mobj->movedir == 2) { // Pausing between bounces in the pinball phase
				if (mobj->target->player->powers[pw_tailsfly]) // Trying to escape, eh?
					mobj->watertop = mobj->target->z + mobj->target->momz*6; // Readjust your aim. >:3
				else if (mobj->target->floorz > mobj->floorz)
					mobj->watertop = mobj->target->floorz + 16*FRACUNIT;
				else
					mobj->watertop = mobj->floorz + 16*FRACUNIT;

				if (!(mobj->threshold%4)) {
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x + mobj->target->momx*4, mobj->target->y + mobj->target->momy*4);
					if (!mobj->reactiontime)
						S_StartSound(mobj, sfx_zoom); // zoom!
				}
			}
			// else -- Pausing between energy ball shots

			return;
		}

		// threshold is used for attacks/maneuvers.
		if (mobj->threshold && mobj->movecount != 2) {
			fixed_t speed = 20*FRACUNIT + FixedMul(40*FRACUNIT, FixedDiv((mobj->info->spawnhealth - mobj->health)<<FRACBITS, mobj->info->spawnhealth<<FRACBITS));
			UINT8 tries = 0;

			// Firin' mah lazors
			if (mobj->movecount == 3 && mobj->movedir == 1)
			{
				if (!(mobj->threshold & 1))
				{
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
					if (mobj->extravalue1 >= 2)
					{
						missile->destscale = FRACUNIT>>1;
						P_SetScale(missile, missile->destscale);
					}
					missile->fuse = 3*TICRATE;
					missile->z -= missile->height/2;

					if (mobj->extravalue1 == 2)
					{
						UINT8 i;
						mobj_t *spread;
						for (i = 0; i < 5; i++)
						{
							if (i == 2)
								continue;
							spread = P_SpawnMobj(missile->x, missile->y, missile->z, missile->type);
							spread->angle = missile->angle+(ANGLE_11hh/2)*(i-2);
							P_InstaThrust(spread,spread->angle,spread->info->speed);
							spread->momz = missile->momz;
							spread->destscale = FRACUNIT>>1;
							P_SetScale(spread, spread->destscale);
							spread->fuse = missile->fuse;
						}
						P_InstaThrust(missile,missile->angle,missile->info->speed);
					}
					else if (mobj->extravalue1 >= 3)
					{
						UINT8 i;
						mobj_t *spread;
						mobj->target->z -= (2*missile->height);
						for (i = 0; i < 5; i++)
						{
							if (i != 2)
							{
								spread = P_SpawnMissile(mobj, mobj->target, missile->type);
								spread->destscale = FRACUNIT>>1;
								P_SetScale(spread, spread->destscale);
								spread->fuse = missile->fuse;
								spread->z -= spread->height/2;
							}
							mobj->target->z += missile->height;
						}
						mobj->target->z -= (3*missile->height);
					}
				}
				else
				{
					P_SetMobjState(mobj, mobj->state->nextstate);
					if (mobj->extravalue1 == 3)
						mobj->reactiontime = TICRATE/8;
					else
						mobj->reactiontime = TICRATE/4;
				}
				mobj->threshold--;
				return;
			}

			// Pinball attack!
			if (mobj->movecount == 3 && (mobj->movedir == 0 || mobj->movedir == 2))
			{
				if ((statenum_t)(mobj->state-states) != mobj->info->seestate)
					P_SetMobjState(mobj, mobj->info->seestate);
				if (mobj->movedir == 0) // mobj health == 1
					P_InstaThrust(mobj, mobj->angle, 38*FRACUNIT);
				else if (mobj->health == 3)
					P_InstaThrust(mobj, mobj->angle, 22*FRACUNIT);
				else // mobj health == 2
					P_InstaThrust(mobj, mobj->angle, 30*FRACUNIT);
				if (!P_TryMove(mobj, mobj->x+mobj->momx, mobj->y+mobj->momy, true))
				{ // Hit a wall? Find a direction to bounce
					mobj->threshold--;
					if (!mobj->threshold) { // failed bounce!
						S_StartSound(mobj, sfx_mspogo);
						P_BounceMove(mobj);
						mobj->angle = R_PointToAngle2(mobj->momx, mobj->momy,0,0);
						mobj->momz = 4*FRACUNIT;
						mobj->flags &= ~MF_PAIN;
						mobj->fuse = 8*TICRATE;
						mobj->movecount = 0;
						P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_CYBRAKDEMON_VILE_EXPLOSION);
						P_SetMobjState(mobj, mobj->info->meleestate);
					}
					else if (!(mobj->threshold%4))
					{ // We've decided to lock onto the player this bounce.
						P_SetMobjState(mobj, mobj->state->nextstate);
						S_StartSound(mobj, sfx_s3k5a);
						mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x + mobj->target->momx*4, mobj->target->y + mobj->target->momy*4);
						mobj->reactiontime = TICRATE - 5*(mobj->info->damage - mobj->health); // targetting time
					}
					else
					{ // No homing, just use P_BounceMove
						S_StartSound(mobj, sfx_s3kaa); // make the bounces distinct...
						P_BounceMove(mobj);
						mobj->angle = R_PointToAngle2(0,0,mobj->momx,mobj->momy);
						mobj->reactiontime = 1; // TICRATE/4; // just a pause before you bounce away
					}
					mobj->momx = mobj->momy = 0;
				}
				return;
			}

			P_SpawnGhostMobj(mobj)->colorized = false;

			// Vector form dodge!
			mobj->angle += mobj->movedir;
			P_InstaThrust(mobj, mobj->angle, -speed);
			while (!P_TryMove(mobj, mobj->x+mobj->momx, mobj->y+mobj->momy, true) && tries++ < 16)
			{
				S_StartSound(mobj, sfx_mspogo);
				P_BounceMove(mobj);
				mobj->angle = R_PointToAngle2(mobj->momx, mobj->momy,0,0);
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

		if (mobj->movecount == 1 || mobj->movecount == 2)
		{ // Charging energy
			if (mobj->momx != 0 || mobj->momy != 0) { // Apply the air breaks
				if (abs(mobj->momx)+abs(mobj->momy) < FRACUNIT)
					mobj->momx = mobj->momy = 0;
				else
					P_Thrust(mobj, R_PointToAngle2(0, 0, mobj->momx, mobj->momy), -6*FRACUNIT/8);
			}
			if (mobj->state == states+mobj->info->raisestate)
				return;
		}

		if (mobj->fuse == 0)
		{
			mobj->flags2 &= ~MF2_INVERTAIMABLE;
			// It's time to attack! What are we gonna do?!
			switch(mobj->movecount)
			{
			case 0:
			default:
				// Fly up and prepare for an attack!
				// We have to charge up first, so let's go up into the air
				S_StartSound(mobj, sfx_beflap);
				P_SetMobjState(mobj, mobj->info->raisestate);
				if (mobj->floorz >= mobj->target->floorz)
					mobj->watertop = mobj->floorz + 256*FRACUNIT;
				else
					mobj->watertop = mobj->target->floorz + 256*FRACUNIT;
				break;

			case 1:
				// Okay, we're up? Good, time to gather energy...
				if (mobj->health > mobj->info->damage)
				{ // No more bubble if we're broken (pinch phase)
					mobj_t *shield = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_MSSHIELD_FRONT);
					P_SetTarget(&mobj->hprev, shield);
					P_SetTarget(&shield->target, mobj);

					// Attack 2: Energy shot!
					switch (mobj->health)
					{
						case 8: // shoot once
						default:
							mobj->extravalue1 = 0;
							mobj->threshold = 2;
							break;
						case 7: // spread shot (vertical)
							mobj->extravalue1 = 4;
							mobj->threshold = 2;
							break;
						case 6: // three shots
							mobj->extravalue1 = 1;
							mobj->threshold = 3*2;
							break;
						case 5: // spread shot (horizontal)
							mobj->extravalue1 = 2;
							mobj->threshold = 2;
							break;
						case 4: // machine gun
							mobj->extravalue1 = 3;
							mobj->threshold = 5*2;
							break;
					}
				}
				else
				{
					/*mobj_t *shield = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_MSSHIELD_FRONT);
					P_SetTarget(&mobj->tracer, shield);
					P_SetTarget(&shield->target, mobj);
					shield->height -= 20*FRACUNIT; // different offset...
					P_SetMobjState(shield, S_MSSHIELD_F2);*/
					P_SetMobjState(mobj, S_METALSONIC_BOUNCE);
					//P_LinedefExecute(LE_PINCHPHASE, mobj, NULL); -- why does this happen twice? see case 2...
				}
				mobj->fuse = 3*TICRATE;
				mobj->flags |= MF_PAIN;
				if (mobj->info->attacksound)
					S_StartSound(mobj, mobj->info->attacksound);
				A_FaceTarget(mobj);

				break;

			case 2:
			{
				// We're all charged and ready now! Unleash the fury!!
				S_StopSound(mobj);
				if (mobj->hprev)
				{
					P_RemoveMobj(mobj->hprev);
					P_SetTarget(&mobj->hprev, NULL);
				}
				if (mobj->health <= mobj->info->damage)
				{
					// Attack 1: Pinball dash!
					if (mobj->health == 1)
						mobj->movedir = 0;
					else
						mobj->movedir = 2;
					if (mobj->info->seesound)
						S_StartSound(mobj, mobj->info->seesound);
					P_SetMobjState(mobj, mobj->info->seestate);
					if (mobj->movedir == 2)
						mobj->threshold = 12; // bounce 12 times
					else
						mobj->threshold = 24; // bounce 24 times
					if (mobj->floorz >= mobj->target->floorz)
						mobj->watertop = mobj->floorz + 16*FRACUNIT;
					else
						mobj->watertop = mobj->target->floorz + 16*FRACUNIT;
					P_LinedefExecute(LE_PINCHPHASE, mobj, NULL);

#if 0
					whoosh = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_GHOST); // done here so the offset is correct
					whoosh->frame = FF_FULLBRIGHT;
					whoosh->sprite = SPR_ARMA;
					whoosh->destscale = whoosh->scale<<1;
					whoosh->scalespeed = FixedMul(whoosh->scalespeed, whoosh->scale);
					whoosh->height = 38*whoosh->scale;
					whoosh->fuse = 10;
					whoosh->color = SKINCOLOR_SUNSET;
					whoosh->colorized = true;
					whoosh->flags |= MF_NOCLIPHEIGHT;
#endif

					P_SetMobjState(mobj->tracer, S_JETFUMEFLASH);
					P_SetScale(mobj->tracer, mobj->scale << 1);
				}
				else
				{
					// Attack 2: Energy shot!
					mobj->movedir = 1;
					// looking for the number of things to fire? that's done in case 1 now
				}
				break;
			}
			case 3:
				// Return to idle.
				if (mobj->floorz >= mobj->target->floorz)
					mobj->watertop = mobj->floorz + 32*FRACUNIT;
				else
					mobj->watertop = mobj->target->floorz + 32*FRACUNIT;
				P_SetMobjState(mobj, mobj->info->spawnstate);
				mobj->flags &= ~MF_PAIN;
				mobj->fuse = 8*TICRATE;
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

			if (mobj->flags2 & MF2_CLASSICPUSH)
				mobj->flags2 &= ~MF2_CLASSICPUSH; // a missile caught us in PIT_CheckThing!
			else
			{
				// Check if we're being attacked
				if (!mobj->target || !mobj->target->player || !P_PlayerCanDamage(mobj->target->player, mobj))
					goto nodanger;
				if (mobj->target->x+mobj->target->radius+abs(mobj->target->momx*2) < mobj->x-mobj->radius)
					goto nodanger;
				if (mobj->target->x-mobj->target->radius-abs(mobj->target->momx*2) > mobj->x+mobj->radius)
					goto nodanger;
				if (mobj->target->y+mobj->target->radius+abs(mobj->target->momy*2) < mobj->y-mobj->radius)
					goto nodanger;
				if (mobj->target->y-mobj->target->radius-abs(mobj->target->momy*2) > mobj->y+mobj->radius)
					goto nodanger;
				if (mobj->target->z+mobj->target->height+mobj->target->momz*2 < mobj->z)
					goto nodanger;
				if (mobj->target->z+mobj->target->momz*2 > mobj->z+mobj->height)
					goto nodanger;
			}

			// An incoming attack is detected! What should we do?!
			// Go into vector form!
			vectorise;
			return;
nodanger:

			mobj->flags2 |= MF2_INVERTAIMABLE;

			// Move normally: Approach the player using normal thrust and simulated friction.
			dist = P_AproxDistance(mobj->x-mobj->target->x, mobj->y-mobj->target->y);
			P_Thrust(mobj, R_PointToAngle2(0, 0, mobj->momx, mobj->momy), -3*FRACUNIT/8);
			if (dist < 64*FRACUNIT && !(mobj->target->player && mobj->target->player->homing))
				P_Thrust(mobj, mobj->angle, -4*FRACUNIT);
			else if (dist > 180*FRACUNIT)
				P_Thrust(mobj, mobj->angle, FRACUNIT);
			else
				P_Thrust(mobj, mobj->angle + ANGLE_90, FINECOSINE((((angle_t)(leveltime*ANG1))>>ANGLETOFINESHIFT) & FINEMASK)>>1);
			mobj->momz += P_AproxDistance(mobj->momx, mobj->momy)/12; // Move up higher the faster you're going.
		}
	}
}
#undef vectorise

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
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
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
	fixed_t x, y, z;

	//I_Assert(mobj->target != NULL);
	if (!mobj->target) /// \todo DEBUG ME! Target was P_RemoveMobj'd at some point, and therefore no longer valid!
		return;

	x = mobj->target->x;
	y = mobj->target->y;
	z = mobj->target->z+mobj->target->height/2;

	// Make the sprite travel towards the center of the hoop
	v[0] = FixedMul(FINECOSINE(fa),fuse);
	v[1] = 0;
	v[2] = FixedMul(FINESINE(fa),fuse);
	v[3] = FRACUNIT;

	res = VectorMatrixMultiply(v, *RotateXMatrix(FixedAngle(mobj->target->movedir*FRACUNIT)));
	M_Memcpy(&v, res, sizeof (v));
	res = VectorMatrixMultiply(v, *RotateZMatrix(FixedAngle(mobj->target->movecount*FRACUNIT)));
	M_Memcpy(&v, res, sizeof (v));

	finalx = x + v[0];
	finaly = y + v[1];
	finalz = z + v[2];

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

	mobj->radius = FixedMul(FixedDiv(mobj->radius, oldscale), newscale);
	mobj->height = FixedMul(FixedDiv(mobj->height, oldscale), newscale);

	player = mobj->player;

	if (player)
	{
		G_GhostAddScale(newscale);
		player->viewheight = FixedMul(FixedDiv(player->viewheight, oldscale), newscale); // Nonono don't calculate viewheight elsewhere, this is the best place for it!
	}
}

void P_Attract(mobj_t *source, mobj_t *dest, boolean nightsgrab) // Home in on your target
{
	fixed_t dist, ndist, speedmul;
	angle_t vangle;
	fixed_t tx = dest->x;
	fixed_t ty = dest->y;
	fixed_t tz = dest->z + (dest->height/2); // Aim for center
	fixed_t xydist = P_AproxDistance(tx - source->x, ty - source->y);

	if (!dest || dest->health <= 0 || !dest->player || !source->tracer)
		return;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, tx, ty);

	// change slope
	dist = P_AproxDistance(xydist, tz - source->z);

	if (dist < 1)
		dist = 1;

	if (nightsgrab && source->movefactor)
	{
		source->movefactor += FRACUNIT/2;

		if (dist < source->movefactor)
		{
			source->momx = source->momy = source->momz = 0;
			P_TeleportMove(source, tx, ty, tz);
		}
		else
		{
			vangle = R_PointToAngle2(source->z, 0, tz, xydist);

			source->momx = FixedMul(FINESINE(vangle >> ANGLETOFINESHIFT), FixedMul(FINECOSINE(source->angle >> ANGLETOFINESHIFT), source->movefactor));
			source->momy = FixedMul(FINESINE(vangle >> ANGLETOFINESHIFT), FixedMul(FINESINE(source->angle >> ANGLETOFINESHIFT), source->movefactor));
			source->momz = FixedMul(FINECOSINE(vangle >> ANGLETOFINESHIFT), source->movefactor);
		}
	}
	else
	{
		if (nightsgrab)
			speedmul = P_AproxDistance(dest->momx, dest->momy) + FixedMul(8*FRACUNIT, source->scale);
		else
			speedmul = P_AproxDistance(dest->momx, dest->momy) + FixedMul(source->info->speed, source->scale);

		source->momx = FixedMul(FixedDiv(tx - source->x, dist), speedmul);
		source->momy = FixedMul(FixedDiv(ty - source->y, dist), speedmul);
		source->momz = FixedMul(FixedDiv(tz - source->z, dist), speedmul);
	}

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
		thing->movefactor = 0;
		return;
	}

	if (!thing->tracer->player)
		return;

	P_Attract(thing, thing->tracer, true);
}

//
// P_MaceRotate
// Spins a hnext-chain of objects around its centerpoint, side to side or periodically.
//
void P_MaceRotate(mobj_t *center, INT32 baserot, INT32 baseprevrot)
{
	TVector unit_lengthways, unit_sideways, pos_lengthways, pos_sideways;
	TVector *res;
	fixed_t radius, dist, zstore;
	angle_t fa;
	boolean dosound = false;
	mobj_t *mobj = center->hnext, *hnext = NULL;

	INT32 lastthreshold = -1; // needs to never be equal at start of loop
	fixed_t lastfriction = INT32_MIN; // ditto; almost certainly never, but...

	INT32 rot;
	INT32 prevrot;

	dist = pos_sideways[0] = pos_sideways[1] = pos_sideways[2] = pos_sideways[3] = unit_sideways[3] =\
	 pos_lengthways[0] = pos_lengthways[1] = pos_lengthways[2] = pos_lengthways[3] = 0;

	while (mobj)
	{
		if (P_MobjWasRemoved(mobj) || !mobj->health)
		{
			mobj = mobj->hnext;
			continue;
		}

		mobj->momx = mobj->momy = mobj->momz = 0;

		if (mobj->threshold != lastthreshold
		|| mobj->friction != lastfriction)
		{
			rot = (baserot + mobj->threshold) & FINEMASK;
			prevrot = (baseprevrot + mobj->threshold) & FINEMASK;

			pos_lengthways[0] = pos_lengthways[1] = pos_lengthways[2] = pos_lengthways[3] = 0;

			dist = ((mobj->info->speed) ? mobj->info->speed : mobjinfo[MT_SMALLMACECHAIN].speed);
			dist = ((center->scale == FRACUNIT) ? dist : FixedMul(dist, center->scale));

			fa = (FixedAngle(center->movefactor*FRACUNIT) >> ANGLETOFINESHIFT);
			radius = FixedMul(dist, FINECOSINE(fa));
			unit_lengthways[1] = -FixedMul(dist, FINESINE(fa));
			unit_lengthways[3] = FRACUNIT;

			// Swinging Chain.
			if (center->flags2 & MF2_STRONGBOX)
			{
				fixed_t swingmag = FixedMul(FINECOSINE(rot), center->lastlook << FRACBITS);
				fixed_t prevswingmag = FINECOSINE(prevrot);

				if ((prevswingmag > 0) != (swingmag > 0)) // just passed its lowest point
					dosound = true;

				fa = ((FixedAngle(swingmag) >> ANGLETOFINESHIFT) + mobj->friction) & FINEMASK;

				unit_lengthways[0] = FixedMul(FINESINE(fa), -radius);
				unit_lengthways[2] = FixedMul(FINECOSINE(fa), -radius);
			}
			// Rotating Chain.
			else
			{
				angle_t prevfa = (prevrot + mobj->friction) & FINEMASK;
				fa = (rot + mobj->friction) & FINEMASK;

				if (!(prevfa > (FINEMASK/2)) && (fa > (FINEMASK/2))) // completed a full swing
					dosound = true;

				unit_lengthways[0] = FixedMul(FINECOSINE(fa), radius);
				unit_lengthways[2] = FixedMul(FINESINE(fa), radius);
			}

			// Calculate the angle matrixes for the link.
			res = VectorMatrixMultiply(unit_lengthways, *RotateXMatrix(center->threshold << ANGLETOFINESHIFT));
			M_Memcpy(&unit_lengthways, res, sizeof(unit_lengthways));
			res = VectorMatrixMultiply(unit_lengthways, *RotateZMatrix(center->angle));
			M_Memcpy(&unit_lengthways, res, sizeof(unit_lengthways));

			lastthreshold = mobj->threshold;
			lastfriction = mobj->friction;
		}

		if (dosound && (mobj->flags2 & MF2_BOSSNOTRAP))
		{
			S_StartSound(mobj, mobj->info->activesound);
			dosound = false;
		}

		if (pos_sideways[3] != mobj->movefactor)
		{
			if (!unit_sideways[3])
			{
				unit_sideways[1] = dist;
				unit_sideways[0] = unit_sideways[2] = 0;
				unit_sideways[3] = FRACUNIT;

				res = VectorMatrixMultiply(unit_sideways, *RotateXMatrix(center->threshold << ANGLETOFINESHIFT));
				M_Memcpy(&unit_sideways, res, sizeof(unit_sideways));
				res = VectorMatrixMultiply(unit_sideways, *RotateZMatrix(center->angle));
				M_Memcpy(&unit_sideways, res, sizeof(unit_sideways));
			}

			if (pos_sideways[3] > mobj->movefactor)
			{
				do
				{
					pos_sideways[0] -= unit_sideways[0];
					pos_sideways[1] -= unit_sideways[1];
					pos_sideways[2] -= unit_sideways[2];
				}
				while ((--pos_sideways[3]) != mobj->movefactor);
			}
			else
			{
				do
				{
					pos_sideways[0] += unit_sideways[0];
					pos_sideways[1] += unit_sideways[1];
					pos_sideways[2] += unit_sideways[2];
				}
				while ((++pos_sideways[3]) != mobj->movefactor);
			}
		}

		hnext = mobj->hnext; // just in case the mobj is removed

		if (pos_lengthways[3] > mobj->movecount)
		{
			do
			{
				pos_lengthways[0] -= unit_lengthways[0];
				pos_lengthways[1] -= unit_lengthways[1];
				pos_lengthways[2] -= unit_lengthways[2];
			}
			while ((--pos_lengthways[3]) != mobj->movecount);
		}
		else if (pos_lengthways[3] < mobj->movecount)
		{
			do
			{
				pos_lengthways[0] += unit_lengthways[0];
				pos_lengthways[1] += unit_lengthways[1];
				pos_lengthways[2] += unit_lengthways[2];
			}
			while ((++pos_lengthways[3]) != mobj->movecount);
		}

		P_UnsetThingPosition(mobj);

		mobj->x = center->x;
		mobj->y = center->y;
		mobj->z = center->z;

		// Add on the appropriate distances to the center's co-ordinates.
		if (pos_lengthways[3])
		{
			mobj->x += pos_lengthways[0];
			mobj->y += pos_lengthways[1];
			zstore = pos_lengthways[2] + pos_sideways[2];
		}
		else
			zstore = pos_sideways[2];

		mobj->x += pos_sideways[0];
		mobj->y += pos_sideways[1];

		// Cut the height to align the link with the axis.
		if (mobj->type == MT_SMALLMACECHAIN || mobj->type == MT_BIGMACECHAIN || mobj->type == MT_SMALLGRABCHAIN || mobj->type == MT_BIGGRABCHAIN)
			zstore -= P_MobjFlip(mobj)*mobj->height/4;
		else
			zstore -= P_MobjFlip(mobj)*mobj->height/2;

		mobj->z += zstore;

#if 0 // toaster's testing flashie!
		if (!(mobj->movecount & 1) && !(leveltime & TICRATE)) // I had a brainfart and the flashing isn't exactly what I expected it to be, but it's actually much more useful.
			mobj->flags2 ^= MF2_DONTDRAW;
#endif

		P_SetThingPosition(mobj);

#if 0 // toaster's height-clipping dealie!
		if (!pos_lengthways[3] || P_MobjWasRemoved(mobj) || (mobj->flags & MF_NOCLIPHEIGHT))
			goto cont;

		if ((fa = ((center->threshold & (FINEMASK/2)) << ANGLETOFINESHIFT)) > ANGLE_45 && fa < ANGLE_135) // only move towards center when the motion is towards/away from the ground, rather than alongside it
			goto cont;

		if (mobj->subsector->sector->ffloors)
			P_AdjustMobjFloorZ_FFloors(mobj, mobj->subsector->sector, 2);

		if (mobj->floorz > mobj->z)
			zstore = (mobj->floorz - zstore);
		else if (mobj->ceilingz < mobj->z)
			zstore = (mobj->ceilingz - mobj->height - zstore);
		else
			goto cont;

		zstore = FixedDiv(zstore, dist); // Still needs work... scaling factor is wrong!

		P_UnsetThingPosition(mobj);

		mobj->x -= FixedMul(unit_lengthways[0], zstore);
		mobj->y -= FixedMul(unit_lengthways[1], zstore);

		P_SetThingPosition(mobj);

cont:
#endif
		mobj = hnext;
	}
}

static boolean P_ShieldLook(mobj_t *thing, shieldtype_t shield)
{
	if (!thing->target || thing->target->health <= 0 || !thing->target->player
		|| (thing->target->player->powers[pw_shield] & SH_NOSTACK) == SH_NONE || thing->target->player->powers[pw_super]
		|| thing->target->player->powers[pw_invulnerability] > 1)
	{
		P_RemoveMobj(thing);
		return false;
	}

	// TODO: Make an MT_SHIELDORB which changes color/states to always match the appropriate shield,
	// instead of having completely seperate mobjtypes.
	if (!(shield & SH_FORCE))
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

	if (shield & SH_FORCE && thing->movecount != (thing->target->player->powers[pw_shield] & SH_FORCEHP))
	{
		thing->movecount = (thing->target->player->powers[pw_shield] & SH_FORCEHP);
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

	P_SetScale(thing, FixedMul(thing->target->scale, thing->target->player->shieldscale));
	thing->destscale = thing->scale;
	P_UnsetThingPosition(thing);
	thing->x = thing->target->x;
	thing->y = thing->target->y;
	if (thing->eflags & MFE_VERTICALFLIP)
		thing->z = thing->target->z + (thing->target->height - thing->height + FixedDiv(P_GetPlayerHeight(thing->target->player) - thing->target->height, 3*FRACUNIT)) - FixedMul(2*FRACUNIT, thing->target->scale);
	else
		thing->z = thing->target->z - (FixedDiv(P_GetPlayerHeight(thing->target->player) - thing->target->height, 3*FRACUNIT)) + FixedMul(2*FRACUNIT, thing->target->scale);
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

	// run shields
	for (i = 0; i < numshields; i++)
	{
		P_ShieldLook(shields[i], shields[i]->threshold);
		P_SetTarget(&shields[i], NULL);
	}
	numshields = 0;
}

static boolean P_AddShield(mobj_t *thing)
{
	shieldtype_t shield = thing->threshold;

	if (!thing->target || thing->target->health <= 0 || !thing->target->player
		|| (thing->target->player->powers[pw_shield] & SH_NOSTACK) == SH_NONE || thing->target->player->powers[pw_super]
		|| thing->target->player->powers[pw_invulnerability] > 1)
	{
		P_RemoveMobj(thing);
		return false;
	}

	if (!(shield & SH_FORCE))
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

void P_RunOverlays(void)
{
	// run overlays
	mobj_t *mo, *next = NULL;
	fixed_t destx,desty,zoffs;

	for (mo = overlaycap; mo; mo = next)
	{
		I_Assert(!P_MobjWasRemoved(mo));

		// grab next in chain, then unset the chain target
		next = mo->hnext;
		P_SetTarget(&mo->hnext, NULL);

		if (!mo->target)
			continue;

		if (P_MobjWasRemoved(mo->target))
		{
			P_RemoveMobj(mo);
			continue;
		}

		if (!splitscreen /*&& rendermode != render_soft*/)
		{
			angle_t viewingangle;

			if (players[displayplayer].awayviewtics && players[displayplayer].awayviewmobj != NULL && !P_MobjWasRemoved(players[displayplayer].awayviewmobj))
				viewingangle = R_PointToAngle2(mo->target->x, mo->target->y, players[displayplayer].awayviewmobj->x, players[displayplayer].awayviewmobj->y);
			else if (!camera.chase && players[displayplayer].mo)
				viewingangle = R_PointToAngle2(mo->target->x, mo->target->y, players[displayplayer].mo->x, players[displayplayer].mo->y);
			else
				viewingangle = R_PointToAngle2(mo->target->x, mo->target->y, camera.x, camera.y);

			if (!(mo->state->frame & FF_ANIMATE) && mo->state->var1)
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
		mo->angle = mo->target->angle + mo->movedir;

		if (!(mo->state->frame & FF_ANIMATE))
			zoffs = FixedMul(((signed)mo->state->var2)*FRACUNIT, mo->scale);
		// if you're using FF_ANIMATE on an overlay,
		// then you're on your own.
		else
			zoffs = 0;

		P_UnsetThingPosition(mo);
		mo->x = destx;
		mo->y = desty;
		mo->radius = mo->target->radius;
		mo->height = mo->target->height;
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
	{
		if (mo->hnext != thing)
			continue;

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

	if (P_RandomChance(FRACUNIT/32) && koopa->z <= koopa->floorz)
		koopa->momz = FixedMul(5*FRACUNIT, koopa->scale);

	if (koopa->z > koopa->floorz)
		koopa->momz += FixedMul(FRACUNIT/4, koopa->scale);

	if (P_RandomChance(FRACUNIT/64))
	{
		mobj_t *flame;
		flame = P_SpawnMobj(koopa->x - koopa->radius + FixedMul(5*FRACUNIT, koopa->scale), koopa->y, koopa->z + (P_RandomByte()<<(FRACBITS-2)), MT_KOOPAFLAME);
		flame->momx = -FixedMul(flame->info->speed, flame->scale);
		S_StartSound(flame, sfx_koopfr);
	}
	else if (P_RandomChance(5*FRACUNIT/256))
	{
		mobj_t *hammer;
		hammer = P_SpawnMobj(koopa->x - koopa->radius, koopa->y, koopa->z + koopa->height, MT_HAMMER);
		hammer->momx = FixedMul(-5*FRACUNIT, hammer->scale);
		hammer->momz = FixedMul(7*FRACUNIT, hammer->scale);
	}
}

// Spawns and chains the minecart sides.
static void P_SpawnMinecartSegments(mobj_t *mobj, boolean mode)
{
	fixed_t x = mobj->x;
	fixed_t y = mobj->y;
	fixed_t z = mobj->z;
	mobj_t *prevseg = mobj;
	mobj_t *seg;
	UINT8 i;

	for (i = 0; i < 4; i++)
	{
		seg = P_SpawnMobj(x, y, z, MT_MINECARTSEG);
		P_SetMobjState(seg, (statenum_t)(S_MINECARTSEG_FRONT + i));
		if (i >= 2)
			seg->extravalue1 = (i == 2) ? -18 : 18; // make -20/20 when papersprite projection fixed
		else
		{
			seg->extravalue2 = (i == 0) ? 24 : -24;
			seg->cusval = -90;
		}
		if (!mode)
			seg->frame &= ~FF_ANIMATE;
		P_SetTarget(&prevseg->tracer, seg);
		prevseg = seg;
	}
}

// Updates the chained segments.
static void P_UpdateMinecartSegments(mobj_t *mobj)
{
	mobj_t *seg = mobj->tracer;
	fixed_t x = mobj->x;
	fixed_t y = mobj->y;
	fixed_t z = mobj->z;
	angle_t ang = mobj->angle;
	angle_t fa = (ang >> ANGLETOFINESHIFT) & FINEMASK;
	fixed_t c = FINECOSINE(fa);
	fixed_t s = FINESINE(fa);
	INT32 dx, dy;
	INT32 sang;

	while (seg)
	{
		dx = seg->extravalue1;
		dy = seg->extravalue2;
		sang = seg->cusval;
		P_TeleportMove(seg, x + s*dx + c*dy, y - c*dx + s*dy, z);
		seg->angle = ang + FixedAngle(FRACUNIT*sang);
		seg->flags2 = (seg->flags2 & ~MF2_DONTDRAW) | (mobj->flags2 & MF2_DONTDRAW);
		seg = seg->tracer;
	}
}

void P_HandleMinecartSegments(mobj_t *mobj)
{
	if (!mobj->tracer)
		P_SpawnMinecartSegments(mobj, (mobj->type == MT_MINECART));
	P_UpdateMinecartSegments(mobj);
}

static void P_PyreFlyBurn(mobj_t *mobj, fixed_t hoffs, INT16 vrange, mobjtype_t mobjtype, fixed_t momz)
{
	angle_t fa = (FixedAngle(P_RandomKey(360)*FRACUNIT) >> ANGLETOFINESHIFT) & FINEMASK;
	fixed_t xoffs = FixedMul(FINECOSINE(fa), mobj->radius + hoffs);
	fixed_t yoffs = FixedMul(FINESINE(fa), mobj->radius + hoffs);
	fixed_t zoffs = P_RandomRange(-vrange, vrange)*FRACUNIT;
	mobj_t *particle = P_SpawnMobjFromMobj(mobj, xoffs, yoffs, zoffs, mobjtype);
	particle->momz = momz;
}

static void P_MobjScaleThink(mobj_t *mobj)
{
	fixed_t oldheight = mobj->height;
	UINT8 correctionType = 0; // Don't correct Z position, just gain height

	if (mobj->flags & MF_NOCLIPHEIGHT || (mobj->z > mobj->floorz && mobj->z + mobj->height < mobj->ceilingz))
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
		switch (mobj->type)
		{
		default:
			break;
		}
}

static void P_MaceSceneryThink(mobj_t *mobj)
{
	angle_t oldmovedir = mobj->movedir;

	// Always update movedir to prevent desyncing (in the traditional sense, not the netplay sense).
	mobj->movedir = (mobj->movedir + mobj->lastlook) & FINEMASK;

	// If too far away and not deliberately spitting in the face of optimisation, don't think!
	if (!(mobj->flags2 & MF2_BOSSNOTRAP))
	{
		UINT8 i;
		// Quick! Look through players! Don't move unless a player is relatively close by.
		// The below is selected based on CEZ2's first room. I promise you it is a coincidence that it looks like the weed number.
		for (i = 0; i < MAXPLAYERS; ++i)
			if (playeringame[i] && players[i].mo
				&& P_AproxDistance(P_AproxDistance(mobj->x - players[i].mo->x, mobj->y - players[i].mo->y), mobj->z - players[i].mo->z) < (4200 << FRACBITS))
				break; // Stop looking.
		if (i == MAXPLAYERS)
		{
			if (!(mobj->flags2 & MF2_BEYONDTHEGRAVE))
			{
				mobj_t *ref = mobj;

				// stop/hide all your babies
				while ((ref = ref->hnext))
				{
					ref->eflags = (((ref->flags & MF_NOTHINK) ? 0 : 1)
						| ((ref->flags & MF_NOCLIPTHING) ? 0 : 2)
						| ((ref->flags2 & MF2_DONTDRAW) ? 0 : 4)); // oh my god this is nasty.
					ref->flags |= MF_NOTHINK|MF_NOCLIPTHING;
					ref->flags2 |= MF2_DONTDRAW;
					ref->momx = ref->momy = ref->momz = 0;
				}

				mobj->flags2 |= MF2_BEYONDTHEGRAVE;
			}
			return; // don't make bubble!
		}
		else if (mobj->flags2 & MF2_BEYONDTHEGRAVE)
		{
			mobj_t *ref = mobj;

			// start/show all your babies
			while ((ref = ref->hnext))
			{
				if (ref->eflags & 1)
					ref->flags &= ~MF_NOTHINK;
				if (ref->eflags & 2)
					ref->flags &= ~MF_NOCLIPTHING;
				if (ref->eflags & 4)
					ref->flags2 &= ~MF2_DONTDRAW;
				ref->eflags = 0; // le sign
			}

			mobj->flags2 &= ~MF2_BEYONDTHEGRAVE;
		}
	}

	// Okay, time to MOVE
	P_MaceRotate(mobj, mobj->movedir, oldmovedir);
}

static boolean P_DrownNumbersSceneryThink(mobj_t *mobj)
{
	if (!mobj->target)
	{
		P_RemoveMobj(mobj);
		return false;
	}
	if (!mobj->target->player || !(mobj->target->player->powers[pw_underwater] || mobj->target->player->powers[pw_spacetime]))
	{
		P_RemoveMobj(mobj);
		return false;
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
	return true;
}

static void P_FlameJetSceneryThink(mobj_t *mobj)
{
	mobj_t *flame;
	fixed_t strength;

	if (!(mobj->flags2 & MF2_FIRING))
		return;

	if ((leveltime & 3) != 0)
		return;

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
	P_SetMobjState(flame, S_FLAMEJETFLAME4);

	flame->angle = mobj->angle;

	if (mobj->flags2 & MF2_AMBUSH) // Wave up and down instead of side-to-side
		flame->momz = mobj->fuse << (FRACBITS - 2);
	else
		flame->angle += FixedAngle(mobj->fuse<<FRACBITS);

	strength = 20*FRACUNIT;
	strength -= ((20*FRACUNIT)/16)*mobj->movedir;

	P_InstaThrust(flame, flame->angle, strength);
	S_StartSound(flame, sfx_fire);
}

static void P_VerticalFlameJetSceneryThink(mobj_t *mobj)
{
	mobj_t *flame;
	fixed_t strength;

	if (!(mobj->flags2 & MF2_FIRING))
		return;

	if ((leveltime & 3) != 0)
		return;

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
	if (mobj->flags2 & MF2_AMBUSH)
	{
		mobj->z = mobj->ceilingz - mobj->height;
		flame->momz = -strength;
	}
	else
	{
		flame->momz = strength;
		P_SetMobjState(flame, S_FLAMEJETFLAME7);
	}
	P_InstaThrust(flame, mobj->angle, FixedDiv(mobj->fuse*FRACUNIT, 3*FRACUNIT));
	S_StartSound(flame, sfx_fire);
}

static boolean P_ParticleGenSceneryThink(mobj_t *mobj)
{
	if (!mobj->lastlook)
		return false;

	if (!mobj->threshold)
		return false;

	if (--mobj->fuse <= 0)
	{
		INT32 i = 0;
		mobj_t *spawn;
		fixed_t bottomheight, topheight;
		INT32 type = mobj->threshold, line = mobj->cvmem;

		mobj->fuse = (tic_t)mobj->reactiontime;

		bottomheight = lines[line].frontsector->floorheight;
		topheight = lines[line].frontsector->ceilingheight - mobjinfo[(mobjtype_t)type].height;

		if (mobj->waterbottom != bottomheight || mobj->watertop != topheight)
		{
			if (mobj->movefactor && (topheight > bottomheight))
				mobj->health = (tic_t)(FixedDiv((topheight - bottomheight), abs(mobj->movefactor)) >> FRACBITS);
			else
				mobj->health = 0;

			mobj->z = ((mobj->flags2 & MF2_OBJECTFLIP) ? topheight : bottomheight);
		}

		if (!mobj->health)
			return false;

		for (i = 0; i < mobj->lastlook; i++)
		{
			spawn = P_SpawnMobj(
				mobj->x + FixedMul(FixedMul(mobj->friction, mobj->scale), FINECOSINE(mobj->angle >> ANGLETOFINESHIFT)),
				mobj->y + FixedMul(FixedMul(mobj->friction, mobj->scale), FINESINE(mobj->angle >> ANGLETOFINESHIFT)),
				mobj->z,
				(mobjtype_t)mobj->threshold);
			P_SetScale(spawn, mobj->scale);
			spawn->momz = FixedMul(mobj->movefactor, spawn->scale);
			spawn->destscale = spawn->scale/100;
			spawn->scalespeed = spawn->scale/mobj->health;
			spawn->tics = (tic_t)mobj->health;
			spawn->flags2 |= (mobj->flags2 & MF2_OBJECTFLIP);
			spawn->angle += P_RandomKey(36)*ANG10; // irrelevant for default objects but might make sense for some custom ones

			mobj->angle += mobj->movedir;
		}

		mobj->angle += (angle_t)mobj->movecount;
	}

	return true;
}

static void P_RosySceneryThink(mobj_t *mobj)
{
	UINT8 i;
	fixed_t pdist = 1700*mobj->scale, work, actualwork;
	player_t *player = NULL;
	statenum_t stat = (mobj->state - states);
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;
		if (!players[i].mo)
			continue;
		if (players[i].bot)
			continue;
		if (!players[i].mo->health)
			continue;
		actualwork = work = FixedHypot(mobj->x - players[i].mo->x, mobj->y - players[i].mo->y);
		if (player)
		{
			if (players[i].skin == 0 || players[i].skin == 5)
				work = (2*work)/3;
			if (work >= pdist)
				continue;
		}
		pdist = actualwork;
		player = &players[i];
	}

	if (stat == S_ROSY_JUMP || stat == S_ROSY_PAIN)
	{
		if (P_IsObjectOnGround(mobj))
		{
			mobj->momx = mobj->momy = 0;
			if (player && mobj->cvmem < (-2*TICRATE))
				stat = S_ROSY_UNHAPPY;
			else
				stat = S_ROSY_WALK;
			P_SetMobjState(mobj, stat);
		}
		else if (P_MobjFlip(mobj)*mobj->momz < 0)
			mobj->frame = mobj->state->frame + mobj->state->var1;
	}

	if (!player)
	{
		if ((stat < S_ROSY_IDLE1 || stat > S_ROSY_IDLE4) && stat != S_ROSY_JUMP)
		{
			mobj->momx = mobj->momy = 0;
			P_SetMobjState(mobj, S_ROSY_IDLE1);
		}
	}
	else
	{
		boolean dojump = false, targonground, love, makeheart = false;
		if (mobj->target != player->mo)
			P_SetTarget(&mobj->target, player->mo);
		// Tatsuru: Don't try to hug them if they're above or below you!
		targonground = (P_IsObjectOnGround(mobj->target) && (player->panim == PA_IDLE || player->panim == PA_WALK || player->panim == PA_RUN) && player->mo->z == mobj->z);
		love = (player->skin == 0 || player->skin == 5);

		switch (stat)
		{
		case S_ROSY_IDLE1:
		case S_ROSY_IDLE2:
		case S_ROSY_IDLE3:
		case S_ROSY_IDLE4:
			dojump = true;
			break;
		case S_ROSY_JUMP:
		case S_ROSY_PAIN:
			// handled above
			break;
		case S_ROSY_WALK:
		{
			fixed_t x = mobj->x, y = mobj->y, z = mobj->z;
			angle_t angletoplayer = R_PointToAngle2(x, y, mobj->target->x, mobj->target->y);
			boolean allowed = P_TryMove(mobj, mobj->target->x, mobj->target->y, false);

			P_UnsetThingPosition(mobj);
			mobj->x = x;
			mobj->y = y;
			mobj->z = z;
			P_SetThingPosition(mobj);

			if (allowed)
			{
				fixed_t mom, max;
				P_Thrust(mobj, angletoplayer, (3*FRACUNIT) >> 1);
				mom = FixedHypot(mobj->momx, mobj->momy);
				max = pdist;
				if ((--mobj->extravalue1) <= 0)
				{
					if (++mobj->frame > mobj->state->frame + mobj->state->var1)
						mobj->frame = mobj->state->frame;
					if (mom > 12*mobj->scale)
						mobj->extravalue1 = 2;
					else if (mom > 6*mobj->scale)
						mobj->extravalue1 = 3;
					else
						mobj->extravalue1 = 4;
				}
				if (max < (mobj->radius + mobj->target->radius))
				{
					mobj->momx = mobj->target->player->cmomx;
					mobj->momy = mobj->target->player->cmomy;
					if ((mobj->cvmem > TICRATE && !player->exiting) || !targonground)
						P_SetMobjState(mobj, (stat = S_ROSY_STND));
					else
					{
						mobj->target->momx = mobj->momx;
						mobj->target->momy = mobj->momy;
						P_SetMobjState(mobj, (stat = S_ROSY_HUG));
						S_StartSound(mobj, sfx_cdpcm6);
						mobj->angle = angletoplayer;
					}
				}
				else
				{
					max /= 3;
					if (max > 30*mobj->scale)
						max = 30*mobj->scale;
					if (mom > max && max > mobj->scale)
					{
						max = FixedDiv(max, mom);
						mobj->momx = FixedMul(mobj->momx, max);
						mobj->momy = FixedMul(mobj->momy, max);
					}
					if (abs(mobj->momx) > mobj->scale || abs(mobj->momy) > mobj->scale)
						mobj->angle = R_PointToAngle2(0, 0, mobj->momx, mobj->momy);
				}
			}
			else
				dojump = true;
		}
		break;
		case S_ROSY_HUG:
			if (targonground)
			{
				player->pflags |= PF_STASIS;
				if (mobj->cvmem < 5*TICRATE)
					mobj->cvmem++;
				if (love && !(leveltime & 7))
					makeheart = true;
			}
			else
			{
				if (mobj->cvmem < (love ? 5*TICRATE : 0))
				{
					P_SetMobjState(mobj, (stat = S_ROSY_PAIN));
					S_StartSound(mobj, sfx_cdpcm7);
				}
				else
					P_SetMobjState(mobj, (stat = S_ROSY_JUMP));
				var1 = var2 = 0;
				A_DoNPCPain(mobj);
				mobj->cvmem -= TICRATE;
			}
			break;
		case S_ROSY_STND:
			if ((pdist > (mobj->radius + mobj->target->radius + 3*(mobj->scale + mobj->target->scale))))
				P_SetMobjState(mobj, (stat = S_ROSY_WALK));
			else if (!targonground)
				;
			else
			{
				if (love && !(leveltime & 15))
					makeheart = true;
				if (player->exiting || --mobj->cvmem < TICRATE)
				{
					P_SetMobjState(mobj, (stat = S_ROSY_HUG));
					S_StartSound(mobj, sfx_cdpcm6);
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
					mobj->target->momx = mobj->momx;
					mobj->target->momy = mobj->momy;
				}
			}
			break;
		case S_ROSY_UNHAPPY:
		default:
			break;
		}

		if (stat == S_ROSY_HUG)
		{
			if (player->panim != PA_IDLE)
				P_SetPlayerMobjState(mobj->target, S_PLAY_STND);
			player->pflags |= PF_STASIS;
		}

		if (dojump)
		{
			P_SetMobjState(mobj, S_ROSY_JUMP);
			mobj->z += P_MobjFlip(mobj);
			mobj->momx = mobj->momy = 0;
			P_SetObjectMomZ(mobj, 6 << FRACBITS, false);
			S_StartSound(mobj, sfx_cdfm02);
		}

		if (makeheart)
		{
			mobj_t *cdlhrt = P_SpawnMobjFromMobj(mobj, 0, 0, mobj->height, MT_CDLHRT);
			cdlhrt->destscale = (5*mobj->scale) >> 4;
			P_SetScale(cdlhrt, cdlhrt->destscale);
			cdlhrt->fuse = (5*TICRATE) >> 1;
			cdlhrt->momz = mobj->scale;
			P_SetTarget(&cdlhrt->target, mobj);
			cdlhrt->extravalue1 = mobj->x;
			cdlhrt->extravalue2 = mobj->y;
		}
	}
}

static void P_MobjSceneryThink(mobj_t *mobj)
{
	if (LUAh_MobjThinker(mobj))
		return;
	if (P_MobjWasRemoved(mobj))
		return;

	if ((mobj->flags2 & MF2_SHIELD) && !P_AddShield(mobj))
		return;

	switch (mobj->type)
	{
	case MT_BOSSJUNK:
		mobj->flags2 ^= MF2_DONTDRAW;
		break;
	case MT_MACEPOINT:
	case MT_CHAINMACEPOINT:
	case MT_SPRINGBALLPOINT:
	case MT_CHAINPOINT:
	case MT_FIREBARPOINT:
	case MT_CUSTOMMACEPOINT:
	case MT_HIDDEN_SLING:
		P_MaceSceneryThink(mobj);
		break;
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
	case MT_PITY_ORB:
	case MT_WHIRLWIND_ORB:
	case MT_ARMAGEDDON_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		break;
	case MT_ATTRACT_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		if (/*(mobj->target) -- the following is implicit by P_AddShield
		&& (mobj->target->player)
		&& */ (mobj->target->player->homing) && (mobj->target->player->pflags & PF_SHIELDABILITY))
		{
			P_SetMobjState(mobj, mobj->info->painstate);
			mobj->tics++;
		}
		break;
	case MT_ELEMENTAL_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		if (mobj->tracer
			/* && mobj->target -- the following is implicit by P_AddShield
			&& mobj->target->player
			&& (mobj->target->player->powers[pw_shield] & SH_NOSTACK) == SH_ELEMENTAL */
			&& mobj->target->player->pflags & PF_SHIELDABILITY
			&& ((statenum_t)(mobj->tracer->state - states) < mobj->info->raisestate
				|| (mobj->tracer->state->nextstate < mobj->info->raisestate && mobj->tracer->tics == 1)))
		{
			P_SetMobjState(mobj, mobj->info->painstate);
			mobj->tics++;
			P_SetMobjState(mobj->tracer, mobj->info->raisestate);
			mobj->tracer->tics++;
		}
		break;
	case MT_FORCE_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		if (/*
		&& mobj->target -- the following is implicit by P_AddShield
		&& mobj->target->player
		&& (mobj->target->player->powers[pw_shield] & SH_FORCE)
		&& */ (mobj->target->player->pflags & PF_SHIELDABILITY))
		{
			mobj_t *whoosh = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_GHOST); // done here so the offset is correct
			P_SetMobjState(whoosh, mobj->info->raisestate);
			whoosh->destscale = whoosh->scale << 1;
			whoosh->scalespeed = FixedMul(whoosh->scalespeed, whoosh->scale);
			whoosh->height = 38*whoosh->scale;
			whoosh->fuse = 10;
			whoosh->flags |= MF_NOCLIPHEIGHT;
			whoosh->momz = mobj->target->momz; // Stay reasonably centered for a few frames
			mobj->target->player->pflags &= ~PF_SHIELDABILITY; // prevent eternal whoosh
		}
		/* FALLTHRU */
	case MT_FLAMEAURA_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		if ((statenum_t)(mobj->state - states) < mobj->info->painstate)
			mobj->angle = mobj->target->angle; // implicitly okay because of P_AddShield
		if (mobj->tracer
			/* && mobj->target -- the following is implicit by P_AddShield
			&& mobj->target->player
			&& (mobj->target->player->powers[pw_shield] & SH_NOSTACK) == SH_FLAMEAURA */
			&& mobj->target->player->pflags & PF_SHIELDABILITY
			&& ((statenum_t)(mobj->tracer->state - states) < mobj->info->raisestate
				|| (mobj->tracer->state->nextstate < mobj->info->raisestate && mobj->tracer->tics == 1)))
		{
			P_SetMobjState(mobj, mobj->info->painstate);
			mobj->tics++;
			P_SetMobjState(mobj->tracer, mobj->info->raisestate);
			mobj->tracer->tics++;
		}
		break;
	case MT_BUBBLEWRAP_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		if (mobj->tracer
			/* && mobj->target -- the following is implicit by P_AddShield
			&& mobj->target->player
			&& (mobj->target->player->powers[pw_shield] & SH_NOSTACK) == SH_BUBBLEWRAP */
			)
		{
			if (mobj->target->player->pflags & PF_SHIELDABILITY
				&& ((statenum_t)(mobj->state - states) < mobj->info->painstate
					|| (mobj->state->nextstate < mobj->info->painstate && mobj->tics == 1)))
			{
				P_SetMobjState(mobj, mobj->info->painstate);
				mobj->tics++;
				P_SetMobjState(mobj->tracer, mobj->info->raisestate);
				mobj->tracer->tics++;
			}
			else if (mobj->target->eflags & MFE_JUSTHITFLOOR
				&& (statenum_t)(mobj->state - states) == mobj->info->painstate)
			{
				P_SetMobjState(mobj, mobj->info->painstate + 1);
				mobj->tics++;
				P_SetMobjState(mobj->tracer, mobj->info->raisestate + 1);
				mobj->tracer->tics++;
			}
		}
		break;
	case MT_THUNDERCOIN_ORB:
		if (!(mobj->flags2 & MF2_SHIELD))
			return;
		if (mobj->tracer
			/* && mobj->target -- the following is implicit by P_AddShield
			&& mobj->target->player
			&& (mobj->target->player->powers[pw_shield] & SH_NOSTACK) == SH_THUNDERCOIN */
			&& (mobj->target->player->pflags & PF_SHIELDABILITY))
		{
			P_SetMobjState(mobj, mobj->info->painstate);
			mobj->tics++;
			P_SetMobjState(mobj->tracer, mobj->info->raisestate);
			mobj->tracer->tics++;
			mobj->target->player->pflags &= ~PF_SHIELDABILITY; // prevent eternal spark
		}
		break;
	case MT_WATERDROP:
		P_SceneryCheckWater(mobj);
		if ((mobj->z <= mobj->floorz || mobj->z <= mobj->watertop)
			&& mobj->health > 0)
		{
			mobj->health = 0;
			P_SetMobjState(mobj, mobj->info->deathstate);
			S_StartSound(mobj, mobj->info->deathsound + P_RandomKey(mobj->info->mass));
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
	case MT_LOCKON:
		if (!mobj->target)
		{
			P_RemoveMobj(mobj);
			return;
		}

		mobj->flags2 &= ~MF2_DONTDRAW;

		mobj->x = mobj->target->x;
		mobj->y = mobj->target->y;

		mobj->eflags |= (mobj->target->eflags & MFE_VERTICALFLIP);

		mobj->destscale = mobj->target->destscale;
		P_SetScale(mobj, mobj->target->scale);

		if (!(mobj->eflags & MFE_VERTICALFLIP))
			mobj->z = mobj->target->z + mobj->target->height + FixedMul((16 + abs((signed)(leveltime % TICRATE) - TICRATE/2))*FRACUNIT, mobj->target->scale);
		else
			mobj->z = mobj->target->z - FixedMul((16 + abs((signed)(leveltime % TICRATE) - TICRATE/2))*FRACUNIT, mobj->target->scale) - mobj->height;
		break;
	case MT_LOCKONINF:
		if (!(mobj->flags2 & MF2_STRONGBOX))
		{
			mobj->threshold = mobj->z;
			mobj->flags2 |= MF2_STRONGBOX;
		}
		if (!(mobj->eflags & MFE_VERTICALFLIP))
			mobj->z = mobj->threshold + FixedMul((16 + abs((signed)(leveltime % TICRATE) - TICRATE/2))*FRACUNIT, mobj->scale);
		else
			mobj->z = mobj->threshold - FixedMul((16 + abs((signed)(leveltime % TICRATE) - TICRATE/2))*FRACUNIT, mobj->scale);
		break;
	case MT_DROWNNUMBERS:
		if (!P_DrownNumbersSceneryThink(mobj))
			return;
		break;
	case MT_FLAMEJET:
		P_FlameJetSceneryThink(mobj);
		break;
	case MT_VERTICALFLAMEJET:
		P_VerticalFlameJetSceneryThink(mobj);
		break;
	case MT_FLICKY_01_CENTER:
	case MT_FLICKY_02_CENTER:
	case MT_FLICKY_03_CENTER:
	case MT_FLICKY_04_CENTER:
	case MT_FLICKY_05_CENTER:
	case MT_FLICKY_06_CENTER:
	case MT_FLICKY_07_CENTER:
	case MT_FLICKY_08_CENTER:
	case MT_FLICKY_09_CENTER:
	case MT_FLICKY_10_CENTER:
	case MT_FLICKY_11_CENTER:
	case MT_FLICKY_12_CENTER:
	case MT_FLICKY_13_CENTER:
	case MT_FLICKY_14_CENTER:
	case MT_FLICKY_15_CENTER:
	case MT_FLICKY_16_CENTER:
	case MT_SECRETFLICKY_01_CENTER:
	case MT_SECRETFLICKY_02_CENTER:
		if (mobj->tracer && (mobj->flags & MF_NOCLIPTHING)
			&& (mobj->flags & MF_GRENADEBOUNCE))
			// for now: only do this bounce routine if flicky is in-place. \todo allow in all movements
		{
			if (!(mobj->tracer->flags2 & MF2_OBJECTFLIP) && mobj->tracer->z <= mobj->tracer->floorz)
				mobj->tracer->momz = 7*FRACUNIT;
			else if ((mobj->tracer->flags2 & MF2_OBJECTFLIP) && mobj->tracer->z >= mobj->tracer->ceilingz - mobj->tracer->height)
				mobj->tracer->momz = -7*FRACUNIT;
		}
		break;
	case MT_SEED:
		if (P_MobjFlip(mobj)*mobj->momz < mobj->info->speed)
			mobj->momz = P_MobjFlip(mobj)*mobj->info->speed;
		break;
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
	case MT_WOODDEBRIS:
	case MT_BRICKDEBRIS:
	case MT_BROKENROBOT:
		if (mobj->z <= P_FloorzAtPos(mobj->x, mobj->y, mobj->z, mobj->height)
			&& mobj->state != &states[mobj->info->deathstate])
		{
			P_SetMobjState(mobj, mobj->info->deathstate);
			return;
		}
		break;
	case MT_PARTICLEGEN:
		if (!P_ParticleGenSceneryThink(mobj))
			return;
		break;
	case MT_FSGNA:
		if (mobj->movedir)
			mobj->angle += mobj->movedir;
		break;
	case MT_ROSY:
		P_RosySceneryThink(mobj);
		break;
	case MT_CDLHRT:
	{
		if (mobj->cvmem < 24)
			mobj->cvmem++;
		mobj->movedir += ANG10;
		P_UnsetThingPosition(mobj);
		mobj->x = mobj->extravalue1 + P_ReturnThrustX(mobj, mobj->movedir, mobj->cvmem*mobj->scale);
		mobj->y = mobj->extravalue2 + P_ReturnThrustY(mobj, mobj->movedir, mobj->cvmem*mobj->scale);
		P_SetThingPosition(mobj);

		if (!mobj->fuse)
		{
			if (!LUAh_MobjFuse(mobj))
				P_RemoveMobj(mobj);
			return;
		}
		if (mobj->fuse < 0)
			return;
		if (mobj->fuse < 6)
			mobj->frame = (mobj->frame & ~FF_TRANSMASK) | ((10 - (mobj->fuse*2)) << (FF_TRANSSHIFT));
		mobj->fuse--;
	}
	break;
	case MT_FINISHFLAG:
	{
		if (!mobj->target || mobj->target->player->playerstate == PST_DEAD || !cv_exitmove.value)
		{
			P_RemoveMobj(mobj);
			return;
		}

		if (!camera.chase)
			mobj->flags2 |= MF2_DONTDRAW;
		else
			mobj->flags2 &= ~MF2_DONTDRAW;

		P_UnsetThingPosition(mobj);
		{
			fixed_t radius = FixedMul(10*mobj->info->speed, mobj->target->scale);
			angle_t fa;

			mobj->angle += FixedAngle(mobj->info->speed);

			fa = mobj->angle >> ANGLETOFINESHIFT;

			mobj->x = mobj->target->x + FixedMul(FINECOSINE(fa),radius);
			mobj->y = mobj->target->y + FixedMul(FINESINE(fa),radius);
			mobj->z = mobj->target->z + mobj->target->height/2;
		}
		P_SetThingPosition(mobj);

		P_SetScale(mobj, mobj->target->scale);
	}
	break;
	case MT_VWREF:
	case MT_VWREB:
	{
		INT32 strength;
		++mobj->movedir;
		mobj->frame &= ~FF_TRANSMASK;
		strength = min(mobj->fuse, (INT32)mobj->movedir)*3;
		if (strength < 10)
			mobj->frame |= ((10 - strength) << (FF_TRANSSHIFT));
	}
	/* FALLTHRU */
	default:
		if (mobj->fuse)
		{ // Scenery object fuse! Very basic!
			mobj->fuse--;
			if (!mobj->fuse)
			{
				if (!LUAh_MobjFuse(mobj))
					P_RemoveMobj(mobj);
				return;
			}
		}
		break;
	}

	P_SceneryThinker(mobj);
}

static boolean P_MobjPushableThink(mobj_t *mobj)
{
	P_MobjCheckWater(mobj);
	P_PushableThinker(mobj);

	// Extinguish fire objects in water. (Yes, it's extraordinarily rare to have a pushable flame object, but Brak uses such a case.)
	if (mobj->flags & MF_FIRE && mobj->type != MT_PUMA && mobj->type != MT_FIREBALL
		&& (mobj->eflags & (MFE_UNDERWATER | MFE_TOUCHWATER)))
	{
		P_KillMobj(mobj, NULL, NULL, 0);
		return false;
	}

	return true;
}

static boolean P_MobjBossThink(mobj_t *mobj)
{
	if (LUAh_BossThinker(mobj))
	{
		if (P_MobjWasRemoved(mobj))
			return false;
	}
	else if (P_MobjWasRemoved(mobj))
		return false;
	else
		switch (mobj->type)
		{
		case MT_EGGMOBILE:
			if (mobj->health < mobj->info->damage + 1 && leveltime & 2)
			{
				fixed_t rad = mobj->radius >> FRACBITS;
				fixed_t hei = mobj->height >> FRACBITS;
				mobj_t *particle = P_SpawnMobjFromMobj(mobj,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(hei / 2, hei) << FRACBITS,
					MT_SMOKE);
				P_SetObjectMomZ(particle, 2 << FRACBITS, false);
				particle->momz += mobj->momz;
			}
			if (mobj->flags2 & MF2_SKULLFLY)
#if 1
				P_SpawnGhostMobj(mobj);
#else // all the way back from final demo... MT_THOK isn't even the same size anymore!
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
			if (mobj->health < mobj->info->damage + 1 && leveltime & 2)
			{
				fixed_t rad = mobj->radius >> FRACBITS;
				fixed_t hei = mobj->height >> FRACBITS;
				mobj_t *particle = P_SpawnMobjFromMobj(mobj,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(hei/2, hei) << FRACBITS,
					MT_SMOKE);
				P_SetObjectMomZ(particle, 2 << FRACBITS, false);
				particle->momz += mobj->momz;
			}
			P_Boss2Thinker(mobj);
			break;
		case MT_EGGMOBILE3:
			if (mobj->health < mobj->info->damage + 1 && leveltime & 2)
			{
				fixed_t rad = mobj->radius >> FRACBITS;
				fixed_t hei = mobj->height >> FRACBITS;
				mobj_t *particle = P_SpawnMobjFromMobj(mobj,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(hei/2, hei) << FRACBITS,
					MT_SMOKE);
				P_SetObjectMomZ(particle, 2 << FRACBITS, false);
				particle->momz += mobj->momz;
			}
			P_Boss3Thinker(mobj);
			break;
		case MT_EGGMOBILE4:
			if (mobj->health < mobj->info->damage + 1 && leveltime & 2)
			{
				fixed_t rad = mobj->radius >> FRACBITS;
				fixed_t hei = mobj->height >> FRACBITS;
				mobj_t* particle = P_SpawnMobjFromMobj(mobj,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(rad, -rad) << FRACBITS,
					P_RandomRange(hei/2, hei) << FRACBITS,
					MT_SMOKE);
				P_SetObjectMomZ(particle, 2 << FRACBITS, false);
				particle->momz += mobj->momz;
			}
			P_Boss4Thinker(mobj);
			break;
		case MT_FANG:
			P_Boss5Thinker(mobj);
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
	{
		if (mobj->extravalue1)
		{
			if (!(--mobj->extravalue1))
			{
				if (mobj->target)
				{
					mobj->momz = FixedMul(FixedDiv(mobj->target->z - mobj->z, P_AproxDistance(mobj->x - mobj->target->x, mobj->y - mobj->target->y)), mobj->scale << 1);
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
				}
				else
					mobj->momz = 8*mobj->scale;
			}
			else
				mobj->angle += mobj->movedir;
		}
		else if (mobj->target)
			P_InstaThrust(mobj, mobj->angle, FixedMul(12*FRACUNIT, mobj->scale));
	}
	if (mobj->type == MT_CYBRAKDEMON && !mobj->health)
	{
		if (!(mobj->tics & 1))
		{
			var1 = 2;
			var2 = 0;
			A_BossScream(mobj);
		}
		if (P_CheckDeathPitCollide(mobj))
		{
			P_RemoveMobj(mobj);
			return false;
		}
		if (mobj->momz && mobj->z + mobj->momz <= mobj->floorz)
		{
			S_StartSound(mobj, sfx_befall);
			if (mobj->state != states + S_CYBRAKDEMON_DIE8)
				P_SetMobjState(mobj, S_CYBRAKDEMON_DIE8);
		}
	}
	return true;
}

static boolean P_MobjDeadThink(mobj_t *mobj)
{
	switch (mobj->type)
	{
	case MT_BLUESPHERE:
		if ((mobj->tics >> 2) + 1 > 0 && (mobj->tics >> 2) + 1 <= tr_trans60) // tr_trans50 through tr_trans90, shifting once every second frame
			mobj->frame = (NUMTRANSMAPS - ((mobj->tics >> 2) + 1)) << FF_TRANSSHIFT;
		else // tr_trans60 otherwise
			mobj->frame = tr_trans60 << FF_TRANSSHIFT;
		break;
	case MT_EGGCAPSULE:
		if (mobj->z <= mobj->floorz)
		{
			P_RemoveMobj(mobj);
			return false;
		}
		break;
	case MT_FAKEMOBILE:
		if (mobj->scale == mobj->destscale)
		{
			if (!mobj->fuse)
			{
				S_StartSound(mobj, sfx_s3k77);
				mobj->flags2 |= MF2_DONTDRAW;
				mobj->fuse = TICRATE;
			}
			return false;
		}
		if (!mobj->reactiontime)
		{
			if (P_RandomChance(FRACUNIT/2))
				mobj->movefactor = FRACUNIT;
			else
				mobj->movefactor = -FRACUNIT;
			if (P_RandomChance(FRACUNIT/2))
				mobj->movedir = ANG20;
			else
				mobj->movedir = -ANG20;
			mobj->reactiontime = 5;
		}
		mobj->momz += mobj->movefactor;
		mobj->angle += mobj->movedir;
		P_InstaThrust(mobj, mobj->angle, -mobj->info->speed);
		mobj->reactiontime--;
		break;
	case MT_EGGSHIELD:
		mobj->flags2 ^= MF2_DONTDRAW;
		break;
	case MT_EGGTRAP: // Egg Capsule animal release
		if (mobj->fuse > 0)// && mobj->fuse < TICRATE-(TICRATE/7))
		{
			INT32 i;
			fixed_t x, y, z;
			fixed_t ns;
			mobj_t* mo2;
			mobj_t* flicky;

			z = mobj->subsector->sector->floorheight + FRACUNIT + (P_RandomKey(64) << FRACBITS);
			for (i = 0; i < 3; i++)
			{
				const angle_t fa = P_RandomKey(FINEANGLES) & FINEMASK;
				ns = 64*FRACUNIT;
				x = mobj->x + FixedMul(FINESINE(fa), ns);
				y = mobj->y + FixedMul(FINECOSINE(fa), ns);

				mo2 = P_SpawnMobj(x, y, z, MT_EXPLODE);
				P_SetMobjStateNF(mo2, S_XPLD_EGGTRAP); // so the flickies don't lose their target if they spawn
				ns = 4*FRACUNIT;
				mo2->momx = FixedMul(FINESINE(fa), ns);
				mo2->momy = FixedMul(FINECOSINE(fa), ns);
				mo2->angle = fa << ANGLETOFINESHIFT;

				if (!i && !(mobj->fuse & 2))
					S_StartSound(mo2, mobj->info->deathsound);

				flicky = P_InternalFlickySpawn(mo2, 0, 8*FRACUNIT, false, -1);
				if (!flicky)
					break;

				P_SetTarget(&flicky->target, mo2);
				flicky->momx = mo2->momx;
				flicky->momy = mo2->momy;
			}

			mobj->fuse--;
		}
		break;
	case MT_PLAYER:
		/// \todo Have the player's dead body completely finish its animation even if they've already respawned.
		if (!mobj->fuse)
		{ // Go away.
		  /// \todo Actually go ahead and remove mobj completely, and fix any bugs and crashes doing this creates. Chasecam should stop moving, and F12 should never return to it.
			mobj->momz = 0;
			if (mobj->player)
				mobj->flags2 |= MF2_DONTDRAW;
			else // safe to remove, nobody's going to complain!
			{
				P_RemoveMobj(mobj);
				return false;
			}
		}
		else // Apply gravity to fall downwards.
		{
			if (mobj->player && !(mobj->fuse % 8) && (mobj->player->charflags & SF_MACHINE))
			{
				fixed_t r = mobj->radius >> FRACBITS;
				mobj_t *explosion = P_SpawnMobj(
					mobj->x + (P_RandomRange(r, -r) << FRACBITS),
					mobj->y + (P_RandomRange(r, -r) << FRACBITS),
					mobj->z + (P_RandomKey(mobj->height >> FRACBITS) << FRACBITS),
					MT_SONIC3KBOSSEXPLODE);
				S_StartSound(explosion, sfx_s3kb4);
			}
			if (mobj->movedir == DMG_DROWNED)
				P_SetObjectMomZ(mobj, -FRACUNIT/2, true); // slower fall from drowning
			else
				P_SetObjectMomZ(mobj, -2*FRACUNIT/3, true);
		}
		break;
	case MT_METALSONIC_RACE:
	{
		if (!(mobj->fuse % 8))
		{
			fixed_t r = mobj->radius >> FRACBITS;
			mobj_t *explosion = P_SpawnMobj(
				mobj->x + (P_RandomRange(r, -r) << FRACBITS),
				mobj->y + (P_RandomRange(r, -r) << FRACBITS),
				mobj->z + (P_RandomKey(mobj->height >> FRACBITS) << FRACBITS),
				MT_SONIC3KBOSSEXPLODE);
			S_StartSound(explosion, sfx_s3kb4);
		}
		P_SetObjectMomZ(mobj, -2*FRACUNIT/3, true);
	}
	break;
	default:
		break;
	}
	return true;
}

// Angle-to-tracer to trigger a linedef exec
// See Linedef Exec 457 (Track mobj angle to point)
static void P_TracerAngleThink(mobj_t *mobj)
{
	angle_t looking;
	angle_t ang;

	if (!mobj->tracer)
		return;

	if (!mobj->extravalue2)
		return;

	// mobj->lastlook - Don't disable behavior after first failure
	// mobj->extravalue1 - Angle tolerance
	// mobj->extravalue2 - Exec tag upon failure
	// mobj->cvval - Allowable failure delay
	// mobj->cvmem - Failure timer

	if (mobj->player)
		looking = ( mobj->player->cmd.angleturn << 16 );/* fixes CS_LMAOGALOG */
	else
		looking = mobj->angle;

	ang = looking - R_PointToAngle2(mobj->x, mobj->y, mobj->tracer->x, mobj->tracer->y);

	// \todo account for distance between mobj and tracer
	// Because closer mobjs can be facing beyond the angle tolerance
	// yet tracer is still in the camera view

	// failure state: mobj is not facing tracer
	// Reasaonable defaults: ANGLE_67h, ANGLE_292h
	if (ang >= (angle_t)mobj->extravalue1 && ang <= ANGLE_MAX - (angle_t)mobj->extravalue1)
	{
		if (mobj->cvmem)
			mobj->cvmem--;
		else
		{
			INT32 exectag = mobj->extravalue2; // remember this before we erase the values

			if (mobj->lastlook)
				mobj->cvmem = mobj->cusval; // reset timer for next failure
			else
			{
				// disable after first failure
				mobj->eflags &= ~MFE_TRACERANGLE;
				mobj->lastlook = mobj->extravalue1 = mobj->extravalue2 = mobj->cvmem = mobj->cusval = 0;
			}

			P_LinedefExecute(exectag, mobj, NULL);
		}
	}
	else
		mobj->cvmem = mobj->cusval; // reset failure timer
}

static void P_ArrowThink(mobj_t *mobj)
{
	if (mobj->flags & MF_MISSILE)
	{
		// Calculate the angle of movement.
		/*
			   momz
			 / |
		   /   |
		 /     |
		0------dist(momx,momy)
		*/

		fixed_t dist = P_AproxDistance(mobj->momx, mobj->momy);
		angle_t angle = R_PointToAngle2(0, 0, dist, mobj->momz);

		if (angle > ANG20 && angle <= ANGLE_180)
			mobj->frame = 2;
		else if (angle < ANG340 && angle > ANGLE_180)
			mobj->frame = 0;
		else
			mobj->frame = 1;

		if (!(mobj->extravalue1) && (mobj->momz < 0))
		{
			mobj->extravalue1 = 1;
			S_StartSound(mobj, mobj->info->activesound);
		}
		if (leveltime & 1)
		{
			mobj_t *dust = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_PARTICLE);
			dust->tics = 18;
			dust->scalespeed = 4096;
			dust->destscale = FRACUNIT/32;
		}
	}
	else
		mobj->flags2 ^= MF2_DONTDRAW;
}

static void P_BumbleboreThink(mobj_t *mobj)
{
	statenum_t st = mobj->state - states;
	if (st == S_BUMBLEBORE_FLY1 || st == S_BUMBLEBORE_FLY2)
	{
		if (!mobj->target)
			P_SetMobjState(mobj, mobj->info->spawnstate);
		else if (P_MobjFlip(mobj)*((mobj->z + (mobj->height >> 1)) - (mobj->target->z + (mobj->target->height >> 1))) > 0
			&& R_PointToDist2(mobj->x, mobj->y, mobj->target->x, mobj->target->y) <= 32*FRACUNIT)
		{
			mobj->momx >>= 1;
			mobj->momy >>= 1;
			if (++mobj->movefactor == 4)
			{
				S_StartSound(mobj, mobj->info->seesound);
				mobj->momx = mobj->momy = mobj->momz = 0;
				mobj->flags = (mobj->flags|MF_PAIN) & ~MF_NOGRAVITY;
				P_SetMobjState(mobj, mobj->info->meleestate);
			}
		}
		else
			mobj->movefactor = 0;
	}
	else if (st == S_BUMBLEBORE_RAISE || st == S_BUMBLEBORE_FALL2) // no _FALL1 because it's an 0-tic
	{
		if (P_IsObjectOnGround(mobj))
		{
			S_StopSound(mobj);
			S_StartSound(mobj, mobj->info->attacksound);
			mobj->flags = (mobj->flags | MF_NOGRAVITY) & ~MF_PAIN;
			mobj->momx = mobj->momy = mobj->momz = 0;
			P_SetMobjState(mobj, mobj->info->painstate);
		}
		else
		{
			mobj->angle += ANGLE_22h;
			mobj->frame = mobj->state->frame + ((mobj->tics & 2) >> 1);
		}
	}
	else if (st == S_BUMBLEBORE_STUCK2 && mobj->tics < TICRATE)
		mobj->frame = mobj->state->frame + ((mobj->tics & 2) >> 1);
}

static boolean P_HangsterThink(mobj_t *mobj)
{
	statenum_t st = mobj->state - states;
	//ghost image trail when flying down
	if (st == S_HANGSTER_SWOOP1 || st == S_HANGSTER_SWOOP2)
	{
		P_SpawnGhostMobj(mobj);
		//curve when in line with target, otherwise curve to avoid crashing into floor
		if ((mobj->z - mobj->floorz <= 80*FRACUNIT) || (mobj->target && (mobj->z - mobj->target->z <= 80*FRACUNIT)))
			P_SetMobjState(mobj, (st = S_HANGSTER_ARC1));
	}

	//swoop arc movement stuff
	if (st == S_HANGSTER_ARC1)
	{
		A_FaceTarget(mobj);
		P_Thrust(mobj, mobj->angle, 1*FRACUNIT);
	}
	else if (st == S_HANGSTER_ARC2)
		P_Thrust(mobj, mobj->angle, 2*FRACUNIT);
	else if (st == S_HANGSTER_ARC3)
		P_Thrust(mobj, mobj->angle, 4*FRACUNIT);
	//if movement has stopped while flying (like hitting a wall), fly up immediately
	else if (st == S_HANGSTER_FLY1 && !mobj->momx && !mobj->momy)
	{
		mobj->extravalue1 = 0;
		P_SetMobjState(mobj, S_HANGSTER_ARCUP1);
	}
	//after swooping back up, check for ceiling
	else if ((st == S_HANGSTER_RETURN1 || st == S_HANGSTER_RETURN2) && mobj->momz == 0 && mobj->ceilingz == (mobj->z + mobj->height))
		P_SetMobjState(mobj, (st = S_HANGSTER_RETURN3));

	//should you roost on a ceiling with F_SKY1 as its flat, disappear forever
	if (st == S_HANGSTER_RETURN3 && mobj->momz == 0 && mobj->ceilingz == (mobj->z + mobj->height)
		&& mobj->subsector->sector->ceilingpic == skyflatnum
		&& mobj->subsector->sector->ceilingheight == mobj->ceilingz)
	{
		P_RemoveMobj(mobj);
		return false;
	}

	return true;
}

static boolean P_JetFume1Think(mobj_t *mobj)
{
	fixed_t jetx, jety;

	if (!mobj->target // if you have no target
		|| (!(mobj->target->flags & MF_BOSS) && mobj->target->health <= 0)) // or your target isn't a boss and it's popped now
	{ // then remove yourself as well!
		P_RemoveMobj(mobj);
		return false;
	}

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
		mobj->ceilingz = mobj->z + mobj->height;
		P_SetThingPosition(mobj);
	}
	else if (mobj->fuse == 57)
	{
		P_UnsetThingPosition(mobj);
		mobj->x = jetx + P_ReturnThrustX(mobj->target, mobj->target->angle - ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
		mobj->y = jety + P_ReturnThrustY(mobj->target, mobj->target->angle - ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
		if (mobj->target->eflags & MFE_VERTICALFLIP)
			mobj->z = mobj->target->z + mobj->target->height - mobj->height - FixedMul(12*FRACUNIT, mobj->target->scale);
		else
			mobj->z = mobj->target->z + FixedMul(12*FRACUNIT, mobj->target->scale);
		mobj->floorz = mobj->z;
		mobj->ceilingz = mobj->z + mobj->height;
		P_SetThingPosition(mobj);
	}
	else if (mobj->fuse == 58)
	{
		P_UnsetThingPosition(mobj);
		mobj->x = jetx + P_ReturnThrustX(mobj->target, mobj->target->angle + ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
		mobj->y = jety + P_ReturnThrustY(mobj->target, mobj->target->angle + ANGLE_90, FixedMul(24*FRACUNIT, mobj->target->scale));
		if (mobj->target->eflags & MFE_VERTICALFLIP)
			mobj->z = mobj->target->z + mobj->target->height - mobj->height - FixedMul(12*FRACUNIT, mobj->target->scale);
		else
			mobj->z = mobj->target->z + FixedMul(12*FRACUNIT, mobj->target->scale);
		mobj->floorz = mobj->z;
		mobj->ceilingz = mobj->z + mobj->height;
		P_SetThingPosition(mobj);
	}
	else if (mobj->fuse == 59)
	{
		boolean dashmod = ((mobj->target->flags & MF_PAIN) && (mobj->target->health <= mobj->target->info->damage));
		jetx = mobj->target->x + P_ReturnThrustX(mobj->target, mobj->target->angle, -mobj->target->radius);
		jety = mobj->target->y + P_ReturnThrustY(mobj->target, mobj->target->angle, -mobj->target->radius);
		P_UnsetThingPosition(mobj);
		mobj->x = jetx;
		mobj->y = jety;
		mobj->destscale = mobj->target->scale;
		if (!(dashmod && mobj->target->state == states + S_METALSONIC_BOUNCE))
		{
			mobj->destscale = (mobj->destscale + FixedDiv(R_PointToDist2(0, 0, mobj->target->momx, mobj->target->momy), 36*mobj->target->scale))/3;
		}
		if (mobj->target->eflags & MFE_VERTICALFLIP)
			mobj->z = mobj->target->z + mobj->target->height/2 + mobj->height/2;
		else
			mobj->z = mobj->target->z + mobj->target->height/2 - mobj->height/2;
		mobj->floorz = mobj->z;
		mobj->ceilingz = mobj->z + mobj->height;
		P_SetThingPosition(mobj);
		if (dashmod)
		{
			mobj->color = SKINCOLOR_SUNSET;
			if (mobj->target->movecount == 3 && !mobj->target->reactiontime && (mobj->target->movedir == 0 || mobj->target->movedir == 2))
				P_SpawnGhostMobj(mobj);
		}
		else
			mobj->color = SKINCOLOR_ICY;
	}
	mobj->fuse++;
	return true;
}

static boolean P_EggRobo1Think(mobj_t *mobj)
{
#define SPECTATORRADIUS (96*mobj->scale)
	if (!(mobj->flags2 & MF2_STRONGBOX))
	{
		mobj->cusval = mobj->x; // eat my SOCs, p_mobj.h warning, we have lua now
		mobj->cvmem = mobj->y; // ditto
		mobj->movedir = mobj->angle;
		mobj->threshold = P_MobjFlip(mobj)*10*mobj->scale;
		if (mobj->threshold < 0)
			mobj->threshold += (mobj->ceilingz - mobj->height);
		else
			mobj->threshold += mobj->floorz;
		var1 = 4;
		A_BossJetFume(mobj);
		mobj->flags2 |= MF2_STRONGBOX;
	}

	if (mobj->state == &states[mobj->info->deathstate]) // todo: make map actually set health to 0 for these
	{
		if (mobj->movecount)
		{
			if (!(--mobj->movecount))
				S_StartSound(mobj, mobj->info->deathsound);
		}
		else
		{
			mobj->momz += P_MobjFlip(mobj)*mobj->scale;
			if (mobj->momz > 0)
			{
				if (mobj->z + mobj->momz > mobj->ceilingz + (1000 << FRACBITS))
				{
					P_RemoveMobj(mobj);
					return false;
				}
			}
			else if (mobj->z + mobj->height + mobj->momz < mobj->floorz - (1000 << FRACBITS))
			{
				P_RemoveMobj(mobj);
				return false;
			}
		}
	}
	else
	{
		fixed_t basex = mobj->cusval, basey = mobj->cvmem;

		if (mobj->spawnpoint && mobj->spawnpoint->options & (MTF_AMBUSH|MTF_OBJECTSPECIAL))
		{
			angle_t sideang = mobj->movedir + ((mobj->spawnpoint->options & MTF_AMBUSH) ? ANGLE_90 : -ANGLE_90);
			fixed_t oscillate = FixedMul(FINESINE(((leveltime * ANG1) >> (ANGLETOFINESHIFT + 2)) & FINEMASK), 250*mobj->scale);
			basex += P_ReturnThrustX(mobj, sideang, oscillate);
			basey += P_ReturnThrustY(mobj, sideang, oscillate);
		}

		mobj->z = mobj->threshold + FixedMul(FINESINE(((leveltime + mobj->movecount)*ANG2 >> (ANGLETOFINESHIFT - 2)) & FINEMASK), 8*mobj->scale);
		if (mobj->state != &states[mobj->info->meleestate])
		{
			boolean didmove = false;

			if (mobj->state == &states[mobj->info->spawnstate])
			{
				UINT8 i;
				fixed_t dist = INT32_MAX;

				for (i = 0; i < MAXPLAYERS; i++)
				{
					fixed_t compdist;
					if (!playeringame[i])
						continue;
					if (players[i].spectator)
						continue;
					if (!players[i].mo)
						continue;
					if (!players[i].mo->health)
						continue;
					if (P_PlayerInPain(&players[i]))
						continue;
					if (players[i].mo->z > mobj->z + mobj->height + 8*mobj->scale)
						continue;
					if (players[i].mo->z + players[i].mo->height < mobj->z - 8*mobj->scale)
						continue;
					compdist = P_AproxDistance(
						players[i].mo->x + players[i].mo->momx - basex,
						players[i].mo->y + players[i].mo->momy - basey);
					if (compdist >= dist)
						continue;
					dist = compdist;
					P_SetTarget(&mobj->target, players[i].mo);
				}

				if (dist < (SPECTATORRADIUS << 1))
				{
					didmove = true;
					mobj->frame = 3 + ((leveltime & 2) >> 1);
					mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);

					if (P_AproxDistance(
						mobj->x - basex,
						mobj->y - basey)
						< mobj->scale)
						S_StartSound(mobj, mobj->info->seesound);

					P_TeleportMove(mobj,
						(15*(mobj->x >> 4)) + (basex >> 4) + P_ReturnThrustX(mobj, mobj->angle, SPECTATORRADIUS >> 4),
						(15*(mobj->y >> 4)) + (basey >> 4) + P_ReturnThrustY(mobj, mobj->angle, SPECTATORRADIUS >> 4),
						mobj->z);
				}
				else
				{
					angle_t diff = (mobj->movedir - mobj->angle);
					if (diff > ANGLE_180)
						diff = InvAngle(InvAngle(diff)/8);
					else
						diff /= 8;
					mobj->angle += diff;

					dist = FINECOSINE(((leveltime + mobj->movecount)*ANG2 >> (ANGLETOFINESHIFT - 2)) & FINEMASK);

					if (abs(dist) < FRACUNIT/2)
						mobj->frame = 0;
					else
						mobj->frame = (dist > 0) ? 1 : 2;
				}
			}

			if (!didmove)
			{
				if (P_AproxDistance(mobj->x - basex, mobj->y - basey) < mobj->scale)
					P_TeleportMove(mobj, basex, basey, mobj->z);
				else
					P_TeleportMove(mobj,
					(15*(mobj->x >> 4)) + (basex >> 4),
						(15*(mobj->y >> 4)) + (basey >> 4),
						mobj->z);
			}
		}
	}
	return true;
#undef SPECTATORRADIUS
}

static void P_NiGHTSDroneThink(mobj_t *mobj)
{
	{
		// variable setup
		mobj_t *goalpost = NULL;
		mobj_t *sparkle = NULL;
		mobj_t *droneman = NULL;

		boolean flip = mobj->flags2 & MF2_OBJECTFLIP;
		boolean topaligned = (mobj->flags & MF_SLIDEME) && !(mobj->flags & MF_GRENADEBOUNCE);
		boolean middlealigned = (mobj->flags & MF_GRENADEBOUNCE) && !(mobj->flags & MF_SLIDEME);
		boolean bottomoffsetted = !(mobj->flags & MF_SLIDEME) && !(mobj->flags & MF_GRENADEBOUNCE);
		boolean flipchanged = false;

		fixed_t dronemanoffset, goaloffset, sparkleoffset, droneboxmandiff, dronemangoaldiff;

		if (mobj->target && mobj->target->type == MT_NIGHTSDRONE_GOAL)
		{
			goalpost = mobj->target;
			if (goalpost->target && goalpost->target->type == MT_NIGHTSDRONE_SPARKLING)
				sparkle = goalpost->target;
			if (goalpost->tracer && goalpost->tracer->type == MT_NIGHTSDRONE_MAN)
				droneman = goalpost->tracer;
		}

		if (!goalpost || !sparkle || !droneman)
			return;

		// did NIGHTSDRONE position, scale, flip, or flags change? all elements need to be synced
		droneboxmandiff = max(mobj->height - droneman->height, 0);
		dronemangoaldiff = max(droneman->height - goalpost->height, 0);

		if (!(goalpost->flags2 & MF2_OBJECTFLIP) && (mobj->flags2 & MF2_OBJECTFLIP))
		{
			goalpost->eflags |= MFE_VERTICALFLIP;
			goalpost->flags2 |= MF2_OBJECTFLIP;
			sparkle->eflags |= MFE_VERTICALFLIP;
			sparkle->flags2 |= MF2_OBJECTFLIP;
			droneman->eflags |= MFE_VERTICALFLIP;
			droneman->flags2 |= MF2_OBJECTFLIP;
			flipchanged = true;
		}
		else if ((goalpost->flags2 & MF2_OBJECTFLIP) && !(mobj->flags2 & MF2_OBJECTFLIP))
		{
			goalpost->eflags &= ~MFE_VERTICALFLIP;
			goalpost->flags2 &= ~MF2_OBJECTFLIP;
			sparkle->eflags &= ~MFE_VERTICALFLIP;
			sparkle->flags2 &= ~MF2_OBJECTFLIP;
			droneman->eflags &= ~MFE_VERTICALFLIP;
			droneman->flags2 &= ~MF2_OBJECTFLIP;
			flipchanged = true;
		}

		if (goalpost->destscale != mobj->destscale
			|| goalpost->movefactor != mobj->z
			|| goalpost->friction != mobj->height
			|| flipchanged
			|| goalpost->threshold != (INT32)(mobj->flags & (MF_SLIDEME|MF_GRENADEBOUNCE)))
		{
			goalpost->destscale = sparkle->destscale = droneman->destscale = mobj->destscale;

			// straight copy-pasta from P_SpawnMapThing, case MT_NIGHTSDRONE
			if (!flip)
			{
				if (topaligned) // Align droneman to top of hitbox
				{
					dronemanoffset = droneboxmandiff;
					goaloffset = dronemangoaldiff/2 + dronemanoffset;
				}
				else if (middlealigned) // Align droneman to center of hitbox
				{
					dronemanoffset = droneboxmandiff/2;
					goaloffset = dronemangoaldiff/2 + dronemanoffset;
				}
				else if (bottomoffsetted)
				{
					dronemanoffset = 24*FRACUNIT;
					goaloffset = dronemangoaldiff + dronemanoffset;
				}
				else
				{
					dronemanoffset = 0;
					goaloffset = dronemangoaldiff/2 + dronemanoffset;
				}

				sparkleoffset = goaloffset - FixedMul(15*FRACUNIT, mobj->scale);
			}
			else
			{
				if (topaligned) // Align droneman to top of hitbox
				{
					dronemanoffset = 0;
					goaloffset = dronemangoaldiff/2 + dronemanoffset;
				}
				else if (middlealigned) // Align droneman to center of hitbox
				{
					dronemanoffset = droneboxmandiff/2;
					goaloffset = dronemangoaldiff/2 + dronemanoffset;
				}
				else if (bottomoffsetted)
				{
					dronemanoffset = droneboxmandiff - FixedMul(24*FRACUNIT, mobj->scale);
					goaloffset = dronemangoaldiff + dronemanoffset;
				}
				else
				{
					dronemanoffset = droneboxmandiff;
					goaloffset = dronemangoaldiff/2 + dronemanoffset;
				}

				sparkleoffset = goaloffset + FixedMul(15*FRACUNIT, mobj->scale);
			}

			P_TeleportMove(goalpost, mobj->x, mobj->y, mobj->z + goaloffset);
			P_TeleportMove(sparkle, mobj->x, mobj->y, mobj->z + sparkleoffset);
			if (goalpost->movefactor != mobj->z || goalpost->friction != mobj->height)
			{
				P_TeleportMove(droneman, mobj->x, mobj->y, mobj->z + dronemanoffset);
				goalpost->movefactor = mobj->z;
				goalpost->friction = mobj->height;
			}
			goalpost->threshold = mobj->flags & (MF_SLIDEME|MF_GRENADEBOUNCE);
		}
		else
		{
			if (goalpost->x != mobj->x || goalpost->y != mobj->y)
			{
				P_TeleportMove(goalpost, mobj->x, mobj->y, goalpost->z);
				P_TeleportMove(sparkle, mobj->x, mobj->y, sparkle->z);
			}

			if (droneman->x != mobj->x || droneman->y != mobj->y)
				P_TeleportMove(droneman, mobj->x, mobj->y,
					droneman->z >= mobj->floorz && droneman->z <= mobj->ceilingz ? droneman->z : mobj->z);
		}

		// now toggle states!
		// GOAL mode?
		if (sparkle->state >= &states[S_NIGHTSDRONE_SPARKLING1] && sparkle->state <= &states[S_NIGHTSDRONE_SPARKLING16])
		{
			INT32 i;
			boolean bonustime = false;

			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && players[i].bonustime && players[i].powers[pw_carry] == CR_NIGHTSMODE)
				{
					bonustime = true;
					break;
				}

			if (!bonustime)
			{
				CONS_Debug(DBG_NIGHTSBASIC, "Removing goal post\n");
				if (goalpost && goalpost->state != &states[S_INVISIBLE])
					P_SetMobjState(goalpost, S_INVISIBLE);
				if (sparkle && sparkle->state != &states[S_INVISIBLE])
					P_SetMobjState(sparkle, S_INVISIBLE);
			}
		}
		// Invisible/bouncing mode.
		else
		{
			INT32 i;
			boolean bonustime = false;
			fixed_t zcomp;

			// Bouncy bouncy!
			if (!flip)
			{
				if (topaligned)
					zcomp = droneboxmandiff + mobj->z;
				else if (middlealigned)
					zcomp = (droneboxmandiff/2) + mobj->z;
				else if (bottomoffsetted)
					zcomp = mobj->z + FixedMul(24*FRACUNIT, mobj->scale);
				else
					zcomp = mobj->z;
			}
			else
			{
				if (topaligned)
					zcomp = mobj->z;
				else if (middlealigned)
					zcomp = (droneboxmandiff/2) + mobj->z;
				else if (bottomoffsetted)
					zcomp = mobj->z + droneboxmandiff - FixedMul(24*FRACUNIT, mobj->scale);
				else
					zcomp = mobj->z + droneboxmandiff;
			}

			droneman->angle += ANG10;
			if (!flip && droneman->z <= zcomp)
				droneman->momz = FixedMul(5*FRACUNIT, droneman->scale);
			else if (flip && droneman->z >= zcomp)
				droneman->momz = FixedMul(-5*FRACUNIT, droneman->scale);

			// state switching logic
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && players[i].bonustime && players[i].powers[pw_carry] == CR_NIGHTSMODE)
				{
					bonustime = true;
					break;
				}

			if (bonustime)
			{
				CONS_Debug(DBG_NIGHTSBASIC, "Adding goal post\n");
				if (!(droneman->flags2 & MF2_DONTDRAW))
					droneman->flags2 |= MF2_DONTDRAW;
				if (goalpost->state == &states[S_INVISIBLE])
					P_SetMobjState(goalpost, mobjinfo[goalpost->type].meleestate);
				if (sparkle->state == &states[S_INVISIBLE])
					P_SetMobjState(sparkle, mobjinfo[sparkle->type].meleestate);
			}
			else if (!G_IsSpecialStage(gamemap))
			{
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && players[i].powers[pw_carry] != CR_NIGHTSMODE)
					{
						bonustime = true; // variable reuse
						break;
					}

				if (bonustime)
				{
					// show droneman if at least one player is non-nights
					if (goalpost->state != &states[S_INVISIBLE])
						P_SetMobjState(goalpost, S_INVISIBLE);
					if (sparkle->state != &states[S_INVISIBLE])
						P_SetMobjState(sparkle, S_INVISIBLE);
					if (droneman->state != &states[mobjinfo[droneman->type].meleestate])
						P_SetMobjState(droneman, mobjinfo[droneman->type].meleestate);
					if (droneman->flags2 & MF2_DONTDRAW)
						droneman->flags2 &= ~MF2_DONTDRAW;
				}
				else
				{
					// else, hide it
					if (!(droneman->flags2 & MF2_DONTDRAW))
						droneman->flags2 |= MF2_DONTDRAW;
				}
			}
		}
	}
}

static boolean P_TurretThink(mobj_t *mobj)
{
	P_MobjCheckWater(mobj);
	P_CheckPosition(mobj, mobj->x, mobj->y);
	if (P_MobjWasRemoved(mobj))
		return false;
	mobj->floorz = tmfloorz;
	mobj->ceilingz = tmceilingz;
	mobj->floorrover = tmfloorrover;
	mobj->ceilingrover = tmceilingrover;

	if ((mobj->eflags & MFE_UNDERWATER) && mobj->health > 0)
	{
		P_SetMobjState(mobj, mobj->info->deathstate);
		mobj->health = 0;
		mobj->flags2 &= ~MF2_FIRING;
	}
	else if (mobj->health > 0 && mobj->z + mobj->height > mobj->ceilingz) // Crushed
	{
		INT32 i, j;
		fixed_t ns;
		fixed_t x, y, z;
		mobj_t* mo2;

		z = mobj->subsector->sector->floorheight + FixedMul(64*FRACUNIT, mobj->scale);
		for (j = 0; j < 2; j++)
		{
			for (i = 0; i < 32; i++)
			{
				const angle_t fa = (i*FINEANGLES/16) & FINEMASK;
				ns = FixedMul(64*FRACUNIT, mobj->scale);
				x = mobj->x + FixedMul(FINESINE(fa), ns);
				y = mobj->y + FixedMul(FINECOSINE(fa), ns);

				mo2 = P_SpawnMobj(x, y, z, MT_EXPLODE);
				ns = FixedMul(16*FRACUNIT, mobj->scale);
				mo2->momx = FixedMul(FINESINE(fa), ns);
				mo2->momy = FixedMul(FINECOSINE(fa), ns);
			}
			z -= FixedMul(32*FRACUNIT, mobj->scale);
		}
		P_SetMobjState(mobj, mobj->info->deathstate);
		mobj->health = 0;
		mobj->flags2 &= ~MF2_FIRING;
	}
	return true;
}

static void P_SaloonDoorThink(mobj_t *mobj)
{
	fixed_t x = mobj->tracer->x;
	fixed_t y = mobj->tracer->y;
	fixed_t z = mobj->tracer->z;
	angle_t oang = FixedAngle(mobj->extravalue1);
	angle_t fa = (oang >> ANGLETOFINESHIFT) & FINEMASK;
	fixed_t c0 = -96*FINECOSINE(fa);
	fixed_t s0 = -96*FINESINE(fa);
	angle_t fma;
	fixed_t c, s;
	angle_t angdiff;

	// Adjust angular speed
	fixed_t da = AngleFixed(mobj->angle - oang);
	if (da > 180*FRACUNIT)
		da -= 360*FRACUNIT;
	mobj->extravalue2 = 8*(mobj->extravalue2 - da/32)/9;

	// Update angle
	mobj->angle += FixedAngle(mobj->extravalue2);

	angdiff = mobj->angle - FixedAngle(mobj->extravalue1);
	if (angdiff > (ANGLE_90 - ANG2) && angdiff < ANGLE_180)
	{
		mobj->angle = FixedAngle(mobj->extravalue1) + (ANGLE_90 - ANG2);
		mobj->extravalue2 /= 2;
	}
	else if (angdiff < (ANGLE_270 + ANG2) && angdiff >= ANGLE_180)
	{
		mobj->angle = FixedAngle(mobj->extravalue1) + (ANGLE_270 + ANG2);
		mobj->extravalue2 /= 2;
	}

	// Update position
	fma = (mobj->angle >> ANGLETOFINESHIFT) & FINEMASK;
	c = 48*FINECOSINE(fma);
	s = 48*FINESINE(fma);
	P_TeleportMove(mobj, x + c0 + c, y + s0 + s, z);
}

static void P_PyreFlyThink(mobj_t *mobj)
{
	fixed_t hdist;

	mobj->extravalue1 = (mobj->extravalue1 + 3) % 360;
	mobj->z += FINESINE(((mobj->extravalue1 * ANG1) >> ANGLETOFINESHIFT) & FINEMASK);

	if (!(mobj->flags2 & MF2_BOSSNOTRAP))
		P_LookForPlayers(mobj, true, false, 1500*FRACUNIT);

	if (!mobj->target)
		return;

	if (mobj->extravalue2 == 1)
		P_PyreFlyBurn(mobj, 0, 20, MT_SMOKE, 4*FRACUNIT);
	else if (mobj->extravalue2 == 2)
	{
		INT32 fireradius = min(100 - mobj->fuse, 52);
		P_PyreFlyBurn(mobj, P_RandomRange(0, fireradius) << FRACBITS, 20, MT_FLAMEPARTICLE, 4*FRACUNIT);
		P_PyreFlyBurn(mobj, fireradius*FRACUNIT, 40, MT_PYREFLY_FIRE, 0);
	}

	hdist = R_PointToDist2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);

	if (hdist > 1500*FRACUNIT)
	{
		mobj->flags2 &= ~MF2_BOSSNOTRAP;
		P_SetTarget(&mobj->target, NULL);
		return;
	}

	if (!(mobj->flags2 & MF2_BOSSNOTRAP) && hdist <= 450*FRACUNIT)
		mobj->flags2 |= MF2_BOSSNOTRAP;

	if (!(mobj->flags2 & MF2_BOSSNOTRAP))
		return;

	if (hdist < 1000*FRACUNIT)
	{
		//Aim for player z position. If too close to floor/ceiling, aim just above/below them.
		fixed_t destz = min(max(mobj->target->z, mobj->target->floorz + 70*FRACUNIT), mobj->target->ceilingz - 80*FRACUNIT - mobj->height);
		fixed_t dist = P_AproxDistance(hdist, destz - mobj->z);
		P_InstaThrust(mobj, R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y), 2*FRACUNIT);
		mobj->momz = FixedMul(FixedDiv(destz - mobj->z, dist), 2*FRACUNIT);
	}
	else
	{
		mobj->momx = 0;
		mobj->momy = 0;
		mobj->momz = 0;
	}
}

static void P_PterabyteThink(mobj_t *mobj)
{
	if (mobj->extravalue1 & 4) // Cooldown after grabbing
	{
		if (mobj->movefactor)
			mobj->movefactor--;
		else
		{
			P_SetTarget(&mobj->target, NULL);
			mobj->extravalue1 &= 3;
		}
	}

	if ((mobj->extravalue1 & 3) == 0) // Hovering
	{
		fixed_t vdist, hdist, time;
		fixed_t hspeed = 3*mobj->info->speed;
		angle_t fa;

		var1 = 1;
		var2 = 0;
		A_CapeChase(mobj);

		if (mobj->target)
			return; // Still carrying a player or in cooldown

		P_LookForPlayers(mobj, true, false, 256*FRACUNIT);

		if (!mobj->target)
			return;

		if (mobj->target->player->powers[pw_flashing])
		{
			P_SetTarget(&mobj->target, NULL);
			return;
		}

		vdist = mobj->z - mobj->target->z - mobj->target->height;
		if (P_MobjFlip(mobj)*vdist <= 0)
		{
			P_SetTarget(&mobj->target, NULL);
			return;
		}

		hdist = R_PointToDist2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
		if (hdist > 450*FRACUNIT)
		{
			P_SetTarget(&mobj->target, NULL);
			return;
		}

		P_SetMobjState(mobj, S_PTERABYTE_SWOOPDOWN);
		mobj->extravalue1++;
		S_StartSound(mobj, mobj->info->attacksound);
		time = FixedDiv(hdist, hspeed);
		mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
		fa = (mobj->angle >> ANGLETOFINESHIFT) & FINEMASK;
		mobj->momx = FixedMul(FINECOSINE(fa), hspeed);
		mobj->momy = FixedMul(FINESINE(fa), hspeed);
		mobj->momz = -2*FixedDiv(vdist, time);
		mobj->extravalue2 = -FixedDiv(mobj->momz, time); //Z accel
		mobj->movecount = time >> FRACBITS;
		mobj->reactiontime = mobj->movecount;
	}
	else if ((mobj->extravalue1 & 3) == 1) // Swooping
	{
		mobj->reactiontime--;
		mobj->momz += mobj->extravalue2;
		if (mobj->reactiontime)
			return;

		if (mobj->state - states == S_PTERABYTE_SWOOPDOWN)
		{
			P_SetMobjState(mobj, S_PTERABYTE_SWOOPUP);
			mobj->reactiontime = mobj->movecount;
		}
		else if (mobj->state - states == S_PTERABYTE_SWOOPUP)
		{
			P_SetMobjState(mobj, S_PTERABYTE_FLY1);
			mobj->extravalue1++;
			if (mobj->target && mobj->target->tracer != mobj)
				P_SetTarget(&mobj->target, NULL); // Failed to grab the target
			mobj->momx = mobj->momy = mobj->momz = 0;
		}
	}
	else // Returning
	{
		var1 = 2*mobj->info->speed;
		var2 = 1;
		A_HomingChase(mobj);
		if (P_AproxDistance(mobj->x - mobj->tracer->x, mobj->y - mobj->tracer->y) <= mobj->info->speed)
		{
			mobj->extravalue1 -= 2;
			mobj->momx = mobj->momy = mobj->momz = 0;
		}
	}
}

static void P_DragonbomberThink(mobj_t *mobj)
{
#define DRAGONTURNSPEED ANG2
	mobj->movecount = (mobj->movecount + 9) % 360;
	P_SetObjectMomZ(mobj, 4*FINESINE(((mobj->movecount*ANG1) >> ANGLETOFINESHIFT) & FINEMASK), false);
	if (mobj->threshold > 0) // are we dropping mines?
	{
		mobj->threshold--;
		if (mobj->threshold == 0) // if the timer hits 0, look for a mine to drop!
		{
			mobj_t *segment = mobj;
			while (segment->tracer != NULL && !P_MobjWasRemoved(segment->tracer) && segment->tracer->state == &states[segment->tracer->info->spawnstate])
				segment = segment->tracer;
			if (segment != mobj) // found an unactivated segment?
			{
				mobj_t *mine = P_SpawnMobjFromMobj(segment, 0, 0, 0, segment->info->painchance);
				mine->angle = segment->angle;
				P_InstaThrust(mine, mobj->angle, P_AproxDistance(mobj->momx, mobj->momy) >> 1);
				P_SetObjectMomZ(mine, -2*FRACUNIT, true);
				S_StartSound(mine, mine->info->seesound);
				P_SetMobjState(segment, segment->info->raisestate);
				mobj->threshold = mobj->info->painchance;
			}
		}
	}
	if (mobj->target) // Are we chasing a player?
	{
		fixed_t dist = P_AproxDistance(mobj->x - mobj->target->x, mobj->y - mobj->target->y);
		if (dist > 2000*mobj->scale) // Not anymore!
			P_SetTarget(&mobj->target, NULL);
		else
		{
			fixed_t vspeed = FixedMul(mobj->info->speed >> 3, mobj->scale);
			fixed_t z = mobj->target->z + (mobj->height >> 1) + (mobj->flags & MFE_VERTICALFLIP ? -128*mobj->scale : 128*mobj->scale + mobj->target->height);
			angle_t diff = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y) - mobj->angle;
			if (diff > ANGLE_180)
				mobj->angle -= DRAGONTURNSPEED;
			else
				mobj->angle += DRAGONTURNSPEED;
			if (!mobj->threshold && dist < 512*mobj->scale) // Close enough to drop bombs
			{
				mobj->threshold = mobj->info->painchance;
			}
			mobj->momz += max(min(z - mobj->z, vspeed), -vspeed);
		}
	}
	else // Can we find a player to chase?
	{
		if (mobj->tracer == NULL || mobj->tracer->state != &states[mobj->tracer->info->spawnstate]
			|| !P_LookForPlayers(mobj, true, false, 2000*mobj->scale)) // if not, circle around the spawnpoint
		{
			if (!mobj->spawnpoint) // unless we don't have one, in which case uhhh just circle around wherever we currently are I guess??
				mobj->angle += DRAGONTURNSPEED;
			else
			{
				fixed_t vspeed = FixedMul(mobj->info->speed >> 3, mobj->scale);
				fixed_t x = mobj->spawnpoint->x << FRACBITS;
				fixed_t y = mobj->spawnpoint->y << FRACBITS;
				fixed_t z = mobj->spawnpoint->z << FRACBITS;
				angle_t diff = R_PointToAngle2(mobj->x, mobj->y, x, y) - mobj->angle;
				if (diff > ANGLE_180)
					mobj->angle -= DRAGONTURNSPEED;
				else
					mobj->angle += DRAGONTURNSPEED;
				mobj->momz += max(min(z - mobj->z, vspeed), -vspeed);
			}
		}
	}
	P_InstaThrust(mobj, mobj->angle, FixedMul(mobj->info->speed, mobj->scale));
#undef DRAGONTURNSPEED
}

static boolean P_MobjRegularThink(mobj_t *mobj)
{
	if ((mobj->flags & MF_ENEMY) && (mobj->state->nextstate == mobj->info->spawnstate && mobj->tics == 1))
		mobj->flags2 &= ~MF2_FRET;

	if (mobj->eflags & MFE_TRACERANGLE)
		P_TracerAngleThink(mobj);

	switch (mobj->type)
	{
	case MT_WALLSPIKEBASE:
		if (!mobj->target) {
			P_RemoveMobj(mobj);
			return false;
		}
		mobj->frame = (mobj->frame & ~FF_FRAMEMASK)|(mobj->target->frame & FF_FRAMEMASK);
#if 0
		if (mobj->angle != mobj->target->angle + ANGLE_90) // reposition if not the correct angle
		{
			mobj_t* target = mobj->target; // shortcut
			const fixed_t baseradius = target->radius - (target->scale/2); //FixedMul(FRACUNIT/2, target->scale);
			P_UnsetThingPosition(mobj);
			mobj->x = target->x - P_ReturnThrustX(target, target->angle, baseradius);
			mobj->y = target->y - P_ReturnThrustY(target, target->angle, baseradius);
			P_SetThingPosition(mobj);
			mobj->angle = target->angle + ANGLE_90;
		}
#endif
		break;
	case MT_FALLINGROCK:
		// Despawn rocks here in case zmovement code can't do so (blame slopes)
		if (!mobj->momx && !mobj->momy && !mobj->momz
			&& ((mobj->eflags & MFE_VERTICALFLIP) ?
				mobj->z + mobj->height >= mobj->ceilingz
				: mobj->z <= mobj->floorz))
		{
			P_RemoveMobj(mobj);
			return false;
		}
		P_MobjCheckWater(mobj);
		break;
	case MT_ARROW:
		P_ArrowThink(mobj);
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
	case MT_BUGGLE:
		mobj->eflags |= MFE_UNDERWATER; //P_MobjCheckWater(mobj); // solely for MFE_UNDERWATER for A_FlickySpawn
		{
			if (mobj->tracer && mobj->tracer->player && mobj->tracer->health > 0
				&& P_AproxDistance(P_AproxDistance(mobj->tracer->x - mobj->x, mobj->tracer->y - mobj->y), mobj->tracer->z - mobj->z) <= mobj->radius*16)
			{
				var1 = mobj->info->speed;
				var2 = 1;

				// Home in on the target.
				A_HomingChase(mobj);

				if (mobj->z < mobj->floorz)
					mobj->z = mobj->floorz;

				if (leveltime % mobj->info->painchance == 0)
					S_StartSound(mobj, mobj->info->activesound);

				if ((statenum_t)(mobj->state - states) != mobj->info->seestate)
					P_SetMobjState(mobj, mobj->info->seestate);
			}
			else
			{
				// Try to find a player
				P_LookForPlayers(mobj, true, true, mobj->radius*16);
				mobj->momx >>= 1;
				mobj->momy >>= 1;
				mobj->momz >>= 1;
				if ((statenum_t)(mobj->state - states) != mobj->info->spawnstate)
					P_SetMobjState(mobj, mobj->info->spawnstate);
			}
		}
		break;
	case MT_BUMBLEBORE:
		P_BumbleboreThink(mobj);
		break;
	case MT_BIGMINE:
		mobj->extravalue1 += 3;
		mobj->extravalue1 %= 360;
		P_UnsetThingPosition(mobj);
		mobj->z += FINESINE(mobj->extravalue1*(FINEMASK + 1)/360);
		P_SetThingPosition(mobj);
		break;
	case MT_FLAME:
		if (mobj->flags2 & MF2_BOSSNOTRAP)
		{
			if (!mobj->target || P_MobjWasRemoved(mobj->target))
			{
				if (mobj->tracer && !P_MobjWasRemoved(mobj->tracer))
					P_RemoveMobj(mobj->tracer);
				P_RemoveMobj(mobj);
				return false;
			}
			mobj->z = mobj->target->z + mobj->target->momz;
			if (!(mobj->eflags & MFE_VERTICALFLIP))
				mobj->z += mobj->target->height;
		}
		if (mobj->tracer && !P_MobjWasRemoved(mobj->tracer))
		{
			mobj->tracer->z = mobj->z + P_MobjFlip(mobj)*20*mobj->scale;
			if (mobj->eflags & MFE_VERTICALFLIP)
				mobj->tracer->z += mobj->height;
		}
		break;
	case MT_WAVINGFLAG1:
	case MT_WAVINGFLAG2:
	{
		fixed_t base = (leveltime << (FRACBITS + 1));
		mobj_t *seg = mobj->tracer, *prev = mobj;
		mobj->movedir = mobj->angle
			+ ((((FINESINE((FixedAngle(base << 1) >> ANGLETOFINESHIFT) & FINEMASK)
				+ FINESINE((FixedAngle(base << 4) >> ANGLETOFINESHIFT) & FINEMASK)) >> 1)
				+ FINESINE((FixedAngle(base*9) >> ANGLETOFINESHIFT) & FINEMASK)
				+ FINECOSINE(((FixedAngle(base*9)) >> ANGLETOFINESHIFT) & FINEMASK)) << 12); //*2^12
		while (seg)
		{
			seg->movedir = seg->angle;
			seg->angle = prev->movedir;
			P_UnsetThingPosition(seg);
			seg->x = prev->x + P_ReturnThrustX(prev, prev->angle, prev->radius);
			seg->y = prev->y + P_ReturnThrustY(prev, prev->angle, prev->radius);
			seg->z = prev->z + prev->height - (seg->scale >> 1);
			P_SetThingPosition(seg);
			prev = seg;
			seg = seg->tracer;
		}
	}
	break;
	case MT_SPINCUSHION:
		if (mobj->target && mobj->state - states >= S_SPINCUSHION_AIM1 && mobj->state - states <= S_SPINCUSHION_AIM5)
			mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x, mobj->target->y);
		break;
	case MT_CRUSHCLAW:
		if (mobj->state - states == S_CRUSHCLAW_STAY && mobj->target)
		{
			mobj_t *chain = mobj->target->target;
			SINT8 sign = ((mobj->tics & 1) ? mobj->tics : -(SINT8)(mobj->tics));
			while (chain)
			{
				chain->z = chain->movefactor + sign*mobj->scale;
				sign = -sign;
				chain = chain->target;
			}
		}
		break;
	case MT_SMASHINGSPIKEBALL:
		mobj->momx = mobj->momy = 0;
		if (mobj->state - states == S_SMASHSPIKE_FALL && P_IsObjectOnGround(mobj))
		{
			P_SetMobjState(mobj, S_SMASHSPIKE_STOMP1);
			S_StartSound(mobj, sfx_spsmsh);
		}
		else if (mobj->state - states == S_SMASHSPIKE_RISE2 && P_MobjFlip(mobj)*(mobj->z - mobj->movecount) >= 0)
		{
			mobj->momz = 0;
			P_SetMobjState(mobj, S_SMASHSPIKE_FLOAT);
		}
		break;
	case MT_HANGSTER:
		if (!P_HangsterThink(mobj))
			return false;
		break;
	case MT_LHRT:
		mobj->momx = FixedMul(mobj->momx, mobj->extravalue2);
		mobj->momy = FixedMul(mobj->momy, mobj->extravalue2);
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
					&& players[i].mare == mobj->threshold && players[i].spheres > 0)
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
			return false;
		}
		P_TeleportMove(mobj, mobj->target->x, mobj->target->y, mobj->target->z - mobj->height);
		break;
	case MT_HAMMER:
		if (mobj->z <= mobj->floorz)
		{
			P_RemoveMobj(mobj);
			return false;
		}
		break;
	case MT_KOOPA:
		P_KoopaThinker(mobj);
		break;
	case MT_FIREBALL:
		if (P_AproxDistance(mobj->momx, mobj->momy) <= 16*FRACUNIT) // Once fireballs lose enough speed, kill them
		{
			P_KillMobj(mobj, NULL, NULL, 0);
			return false;
		}
		break;
	case MT_REDRING:
		if (((mobj->z < mobj->floorz) || (mobj->z + mobj->height > mobj->ceilingz))
			&& mobj->flags & MF_MISSILE)
		{
			P_ExplodeMissile(mobj);
			return false;
		}
		break;
	case MT_BOSSFLYPOINT:
		return false;
	case MT_NIGHTSCORE:
		mobj->color = (UINT16)(leveltime % SKINCOLOR_WHITE);
		break;
	case MT_JETFUME1:
		if (!P_JetFume1Think(mobj))
			return false;
		break;
	case MT_JETFLAME:
	{
		if (!mobj->target // if you have no target
			|| (!(mobj->target->flags & MF_BOSS) && mobj->target->health <= 0)) // or your target isn't a boss and it's popped now
		{ // then remove yourself as well!
			P_RemoveMobj(mobj);
			return false;
		}

		P_UnsetThingPosition(mobj);
		mobj->x = mobj->target->x;
		mobj->y = mobj->target->y;
		mobj->z = mobj->target->z - 50*mobj->target->scale;
		mobj->floorz = mobj->z;
		mobj->ceilingz = mobj->z + mobj->height;
		P_SetThingPosition(mobj);
	}
	break;
	case MT_EGGROBO1:
		if (!P_EggRobo1Think(mobj))
			return false;
		break;
	case MT_EGGROBO1JET:
	{
		if (!mobj->target || P_MobjWasRemoved(mobj->target) // if you have no target
			|| (mobj->target->health <= 0)) // or your target isn't a boss and it's popped now
		{ // then remove yourself as well!
			P_RemoveMobj(mobj);
			return false;
		}

		mobj->flags2 ^= MF2_DONTDRAW;

		P_UnsetThingPosition(mobj);
		mobj->x = mobj->target->x + P_ReturnThrustX(mobj, mobj->target->angle + ANGLE_90, mobj->movefactor*mobj->target->scale) - P_ReturnThrustX(mobj, mobj->target->angle, 19*mobj->target->scale);
		mobj->y = mobj->target->y + P_ReturnThrustY(mobj, mobj->target->angle + ANGLE_90, mobj->movefactor*mobj->target->scale) - P_ReturnThrustY(mobj, mobj->target->angle, 19*mobj->target->scale);
		mobj->z = mobj->target->z;
		if (mobj->target->eflags & MFE_VERTICALFLIP)
			mobj->z += (mobj->target->height - mobj->height);
		mobj->floorz = mobj->z;
		mobj->ceilingz = mobj->z + mobj->height;
		P_SetThingPosition(mobj);
	}
	break;
	case MT_NIGHTSDRONE:
		P_NiGHTSDroneThink(mobj);
		break;
	case MT_PLAYER:
		if (mobj->player)
			P_PlayerMobjThinker(mobj);
		return false;
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
	case MT_REDTEAMRING:
	case MT_BLUETEAMRING:
		P_KillRingsInLava(mobj);
		if (P_MobjWasRemoved(mobj))
			return false;
		/* FALLTHRU */
	case MT_COIN:
	case MT_BLUESPHERE:
	case MT_BOMBSPHERE:
	case MT_NIGHTSCHIP:
	case MT_NIGHTSSTAR:
		// No need to check water. Who cares?
		P_RingThinker(mobj);
		if (mobj->flags2 & MF2_NIGHTSPULL)
			P_NightsItemChase(mobj);
		else
			A_AttractChase(mobj);
		return false;
		// Flung items
	case MT_FLINGRING:
		P_KillRingsInLava(mobj);
		if (P_MobjWasRemoved(mobj))
			return false;
		/* FALLTHRU */
	case MT_FLINGCOIN:
	case MT_FLINGBLUESPHERE:
	case MT_FLINGNIGHTSCHIP:
		if (mobj->flags2 & MF2_NIGHTSPULL)
			P_NightsItemChase(mobj);
		else
			A_AttractChase(mobj);
		break;
	case MT_EMBLEM:
		if (mobj->flags2 & MF2_NIGHTSPULL)
			P_NightsItemChase(mobj);
		break;
	case MT_SHELL:
		if (mobj->threshold && mobj->threshold != TICRATE)
			mobj->threshold--;

		if (mobj->threshold >= TICRATE)
		{
			mobj->angle += ((mobj->movedir == 1) ? ANGLE_22h : ANGLE_337h);
			P_InstaThrust(mobj, R_PointToAngle2(0, 0, mobj->momx, mobj->momy), (mobj->info->speed*mobj->scale));
		}
		break;
	case MT_TURRET:
		if (!P_TurretThink(mobj))
			return false;
		break;
	case MT_BLUEFLAG:
	case MT_REDFLAG:
	{
		sector_t* sec2;
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
	case MT_SPINDUST: // Spindash dust
		mobj->momx = FixedMul(mobj->momx, (3*FRACUNIT)/4); // originally 50000
		mobj->momy = FixedMul(mobj->momy, (3*FRACUNIT)/4); // same
		//mobj->momz = mobj->momz+P_MobjFlip(mobj)/3; // no meaningful change in value to be frank
		if (mobj->state >= &states[S_SPINDUST_BUBBLE1] && mobj->state <= &states[S_SPINDUST_BUBBLE4]) // bubble dust!
		{
			P_MobjCheckWater(mobj);
			if (mobj->watertop != mobj->subsector->sector->floorheight - 1000*FRACUNIT
				&& mobj->z + mobj->height >= mobj->watertop - 5*FRACUNIT)
				mobj->flags2 |= MF2_DONTDRAW;
		}
		break;
	case MT_TRAINDUSTSPAWNER:
		if (leveltime % 5 == 0) {
			mobj_t* traindust = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_PARTICLE);
			traindust->flags = MF_SCENERY;
			P_SetMobjState(traindust, S_TRAINDUST);
			traindust->frame = P_RandomRange(0, 8)|FF_TRANS90;
			traindust->angle = mobj->angle;
			traindust->tics = TICRATE*4;
			traindust->destscale = FRACUNIT*64;
			traindust->scalespeed = FRACUNIT/24;
			P_SetScale(traindust, FRACUNIT*6);
		}
		break;
	case MT_TRAINSTEAMSPAWNER:
		if (leveltime % 5 == 0) {
			mobj_t *steam = P_SpawnMobj(mobj->x + FRACUNIT*P_SignedRandom()/2, mobj->y + FRACUNIT*P_SignedRandom()/2, mobj->z, MT_PARTICLE);
			P_SetMobjState(steam, S_TRAINSTEAM);
			steam->frame = P_RandomRange(0, 1)|FF_TRANS90;
			steam->tics = TICRATE*8;
			steam->destscale = FRACUNIT*64;
			steam->scalespeed = FRACUNIT/8;
			P_SetScale(steam, FRACUNIT*16);
			steam->momx = P_SignedRandom()*32;
			steam->momy = -64*FRACUNIT;
			steam->momz = 2*FRACUNIT;
		}
		break;
	case MT_CANARIVORE_GAS:
	{
		fixed_t momz;

		if (mobj->flags2 & MF2_AMBUSH)
		{
			mobj->momx = FixedMul(mobj->momx, 50*FRACUNIT/51);
			mobj->momy = FixedMul(mobj->momy, 50*FRACUNIT/51);
			break;
		}

		if (mobj->eflags & MFE_VERTICALFLIP)
		{
			if ((mobj->z + mobj->height + mobj->momz) <= mobj->ceilingz)
				break;
		}
		else
		{
			if ((mobj->z + mobj->momz) >= mobj->floorz)
				break;
		}

		momz = abs(mobj->momz);
		if (R_PointToDist2(0, 0, mobj->momx, mobj->momy) < momz)
			P_InstaThrust(mobj, R_PointToAngle2(0, 0, mobj->momx, mobj->momy), momz);
		mobj->flags2 |= MF2_AMBUSH;
		break;
	}
	case MT_SALOONDOOR:
		P_SaloonDoorThink(mobj);
		break;
	case MT_MINECARTSPAWNER:
		P_HandleMinecartSegments(mobj);
		if (!mobj->fuse || mobj->fuse > TICRATE)
			break;
		if (mobj->fuse == 2)
		{
			mobj->fuse = 0;
			break;
		}
		mobj->flags2 ^= MF2_DONTDRAW;
		break;
	case MT_LAVAFALLROCK:
		if (P_IsObjectOnGround(mobj))
			P_RemoveMobj(mobj);
		break;
	case MT_PYREFLY:
		P_PyreFlyThink(mobj);
		break;
	case MT_PTERABYTE:
		P_PterabyteThink(mobj);
		break;
	case MT_DRAGONBOMBER:
		P_DragonbomberThink(mobj);
		break;
	case MT_MINUS:
		if (P_IsObjectOnGround(mobj))
			mobj->rollangle = 0;
		else
			mobj->rollangle = R_PointToAngle2(0, 0, mobj->momz, (mobj->scale << 1) - min(abs(mobj->momz), mobj->scale << 1));
		break;
	case MT_SPINFIRE:
		if (mobj->flags & MF_NOGRAVITY)
		{
			if (mobj->eflags & MFE_VERTICALFLIP)
				mobj->z = mobj->ceilingz - mobj->height;
			else
				mobj->z = mobj->floorz;
		}
		else if ((!(mobj->eflags & MFE_VERTICALFLIP) && mobj->z <= mobj->floorz)
			|| ((mobj->eflags & MFE_VERTICALFLIP) && mobj->z + mobj->height >= mobj->ceilingz))
		{
			mobj->flags |= MF_NOGRAVITY;
			mobj->momx = 8; // this is a hack which is used to ensure it still behaves as a missile and can damage others
			mobj->momy = mobj->momz = 0;
			mobj->z = ((mobj->eflags & MFE_VERTICALFLIP) ? mobj->ceilingz - mobj->height : mobj->floorz);
		}
		/* FALLTHRU */
	default:
		// check mobj against possible water content, before movement code
		P_MobjCheckWater(mobj);

		// Extinguish fire objects in water
		if (mobj->flags & MF_FIRE && mobj->type != MT_PUMA && mobj->type != MT_FIREBALL
			&& (mobj->eflags & (MFE_UNDERWATER|MFE_TOUCHWATER)))
		{
			P_KillMobj(mobj, NULL, NULL, 0);
			return false;
		}
		break;
	}
	return true;
}

static void P_FiringThink(mobj_t *mobj)
{
	if (!mobj->target)
		return;

	if (mobj->health <= 0)
		return;

	if (mobj->state->action.acp1 == (actionf_p1)A_Boss1Laser)
	{
		if (mobj->state->tics > 1)
		{
			var1 = mobj->state->var1;
			var2 = mobj->state->var2 & 65535;
			mobj->state->action.acp1(mobj);
		}
	}
	else if (leveltime & 1) // Fire mode
	{
		mobj_t *missile;

		if (mobj->target->player && mobj->target->player->powers[pw_carry] == CR_NIGHTSMODE)
		{
			fixed_t oldval = mobjinfo[mobj->extravalue1].speed;

			mobj->angle = R_PointToAngle2(mobj->x, mobj->y, mobj->target->x + mobj->target->momx, mobj->target->y + mobj->target->momy);
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

static void P_MonitorFuseThink(mobj_t *mobj)
{
	mobj_t *newmobj;

	// Special case for ALL monitors.
	// If a box's speed is nonzero, it's allowed to respawn as a WRM/SRM.
	if (mobj->info->speed != 0 && (mobj->flags2 & (MF2_AMBUSH|MF2_STRONGBOX)))
	{
		mobjtype_t spawnchance[64];
		INT32 numchoices = 0, i = 0;

		// This define should make it a lot easier to organize and change monitor weights
#define SETMONITORCHANCES(type, strongboxamt, weakboxamt) \
for (i = ((mobj->flags2 & MF2_STRONGBOX) ? strongboxamt : weakboxamt); i; --i) spawnchance[numchoices++] = type

					//                Type             SRM WRM
		SETMONITORCHANCES(MT_SNEAKERS_BOX, 0, 10); // Super Sneakers
		SETMONITORCHANCES(MT_INVULN_BOX, 2, 0); // Invincibility
		SETMONITORCHANCES(MT_WHIRLWIND_BOX, 3, 8); // Whirlwind Shield
		SETMONITORCHANCES(MT_ELEMENTAL_BOX, 3, 8); // Elemental Shield
		SETMONITORCHANCES(MT_ATTRACT_BOX, 2, 0); // Attraction Shield
		SETMONITORCHANCES(MT_FORCE_BOX, 3, 3); // Force Shield
		SETMONITORCHANCES(MT_ARMAGEDDON_BOX, 2, 0); // Armageddon Shield
		SETMONITORCHANCES(MT_MIXUP_BOX, 0, 1); // Teleporters
		SETMONITORCHANCES(MT_RECYCLER_BOX, 0, 1); // Recycler
		SETMONITORCHANCES(MT_1UP_BOX, 1, 1); // 1-Up
		// =======================================
		//                Total             16  32

#undef SETMONITORCHANCES

		i = P_RandomKey(numchoices); // Gotta love those random numbers!
		newmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, spawnchance[i]);
	}
	else
		newmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobj->type);

	// Transfer flags2 (ambush, strongbox, objectflip)
	newmobj->flags2 = mobj->flags2;
	P_RemoveMobj(mobj); // make sure they disappear
}

static void P_FlagFuseThink(mobj_t *mobj)
{
	subsector_t *ss;
	fixed_t x, y, z;
	mobj_t* flagmo;

	if (!mobj->spawnpoint)
		return;

	x = mobj->spawnpoint->x << FRACBITS;
	y = mobj->spawnpoint->y << FRACBITS;
	z = mobj->spawnpoint->z << FRACBITS;
	ss = R_PointInSubsector(x, y);
	if (mobj->spawnpoint->options & MTF_OBJECTFLIP)
		z = ss->sector->ceilingheight - mobjinfo[mobj->type].height - z;
	else
		z = ss->sector->floorheight + z;
	flagmo = P_SpawnMobj(x, y, z, mobj->type);
	flagmo->spawnpoint = mobj->spawnpoint;
	if (mobj->spawnpoint->options & MTF_OBJECTFLIP)
	{
		flagmo->eflags |= MFE_VERTICALFLIP;
		flagmo->flags2 |= MF2_OBJECTFLIP;
	}

	if (mobj->type == MT_REDFLAG)
	{
		if (!(mobj->flags2 & MF2_JUSTATTACKED))
			CONS_Printf(M_GetText("The %c%s%c has returned to base.\n"), 0x85, M_GetText("Red flag"), 0x80);

		// Assumedly in splitscreen players will be on opposing teams
		if (players[consoleplayer].ctfteam == 1 || splitscreen)
			S_StartSound(NULL, sfx_hoop1);
		else if (players[consoleplayer].ctfteam == 2)
			S_StartSound(NULL, sfx_hoop3);

		redflag = flagmo;
	}
	else // MT_BLUEFLAG
	{
		if (!(mobj->flags2 & MF2_JUSTATTACKED))
			CONS_Printf(M_GetText("The %c%s%c has returned to base.\n"), 0x84, M_GetText("Blue flag"), 0x80);

		// Assumedly in splitscreen players will be on opposing teams
		if (players[consoleplayer].ctfteam == 2 || splitscreen)
			S_StartSound(NULL, sfx_hoop1);
		else if (players[consoleplayer].ctfteam == 1)
			S_StartSound(NULL, sfx_hoop3);

		blueflag = flagmo;
	}
}

static boolean P_FuseThink(mobj_t *mobj)
{
	if (mobj->type == MT_SNAPPER_HEAD || mobj->type == MT_SNAPPER_LEG || mobj->type == MT_MINECARTSEG)
		mobj->flags2 ^= MF2_DONTDRAW;

	mobj->fuse--;

	if (mobj->fuse)
		return true;

	if (LUAh_MobjFuse(mobj) || P_MobjWasRemoved(mobj))
		;
	else if (mobj->info->flags & MF_MONITOR)
	{
		P_MonitorFuseThink(mobj);
		return false;
	}
	else switch (mobj->type)
	{
		// gargoyle and snowman handled in P_PushableThinker, not here
	case MT_THROWNGRENADE:
	case MT_CYBRAKDEMON_NAPALM_BOMB_LARGE:
		P_SetMobjState(mobj, mobj->info->deathstate);
		break;
	case MT_LHRT:
		P_KillMobj(mobj, NULL, NULL, 0);
		break;
	case MT_BLUEFLAG:
	case MT_REDFLAG:
		P_FlagFuseThink(mobj);
		P_RemoveMobj(mobj);
		return false;
	case MT_FANG:
		if (mobj->flags2 & MF2_SLIDEPUSH)
		{
			var1 = 0;
			var2 = 0;
			A_BossDeath(mobj);
			return false;
		}
		P_SetMobjState(mobj, mobj->state->nextstate);
		if (P_MobjWasRemoved(mobj))
			return false;
		break;
	case MT_METALSONIC_BATTLE:
		break; // don't remove
	case MT_SPIKE:
		P_SetMobjState(mobj, mobj->state->nextstate);
		mobj->fuse = mobj->info->speed;
		if (mobj->spawnpoint)
			mobj->fuse += mobj->spawnpoint->angle;
		break;
	case MT_WALLSPIKE:
		P_SetMobjState(mobj, mobj->state->nextstate);
		mobj->fuse = mobj->info->speed;
		if (mobj->spawnpoint)
			mobj->fuse += (mobj->spawnpoint->angle / 360);
		break;
	case MT_NIGHTSCORE:
		P_RemoveMobj(mobj);
		return false;
	case MT_LAVAFALL:
		if (mobj->state - states == S_LAVAFALL_DORMANT)
		{
			mobj->fuse = 30;
			P_SetMobjState(mobj, S_LAVAFALL_TELL);
			S_StartSound(mobj, mobj->info->seesound);
		}
		else if (mobj->state - states == S_LAVAFALL_TELL)
		{
			mobj->fuse = 40;
			P_SetMobjState(mobj, S_LAVAFALL_SHOOT);
			S_StopSound(mobj);
			S_StartSound(mobj, mobj->info->attacksound);
		}
		else
		{
			mobj->fuse = 30;
			P_SetMobjState(mobj, S_LAVAFALL_DORMANT);
			S_StopSound(mobj);
		}
		return false;
	case MT_PYREFLY:
		if (mobj->health <= 0)
			break;

		mobj->extravalue2 = (mobj->extravalue2 + 1) % 3;
		if (mobj->extravalue2 == 0)
		{
			P_SetMobjState(mobj, mobj->info->spawnstate);
			mobj->fuse = 100;
			S_StopSound(mobj);
			S_StartSound(mobj, sfx_s3k8c);
		}
		else if (mobj->extravalue2 == 1)
		{
			mobj->fuse = 50;
			S_StartSound(mobj, sfx_s3ka3);
		}
		else
		{
			P_SetMobjState(mobj, mobj->info->meleestate);
			mobj->fuse = 100;
			S_StopSound(mobj);
			S_StartSound(mobj, sfx_s3kc2l);
		}
		return false;
	case MT_PLAYER:
		break; // don't remove
	default:
		P_SetMobjState(mobj, mobj->info->xdeathstate); // will remove the mobj if S_NULL.
		break;
		// Looking for monitors? They moved to a special condition above.
	}

	return !P_MobjWasRemoved(mobj);
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

	if ((mobj->flags & MF_BOSS) && mobj->spawnpoint && (bossdisabled & (1<<mobj->spawnpoint->extrainfo)))
		return;

	// Remove dead target/tracer.
	if (mobj->target && P_MobjWasRemoved(mobj->target))
		P_SetTarget(&mobj->target, NULL);
	if (mobj->tracer && P_MobjWasRemoved(mobj->tracer))
		P_SetTarget(&mobj->tracer, NULL);
	if (mobj->hnext && P_MobjWasRemoved(mobj->hnext))
		P_SetTarget(&mobj->hnext, NULL);
	if (mobj->hprev && P_MobjWasRemoved(mobj->hprev))
		P_SetTarget(&mobj->hprev, NULL);

	mobj->eflags &= ~(MFE_PUSHED|MFE_SPRUNG);

	tmfloorthing = tmhitthing = NULL;

	// Sector special (2,8) allows ANY mobj to trigger a linedef exec
	if (mobj->subsector && GETSECSPECIAL(mobj->subsector->sector->special, 2) == 8)
	{
		sector_t *sec2;

		sec2 = P_ThingOnSpecial3DFloor(mobj);
		if (sec2 && GETSECSPECIAL(sec2->special, 2) == 1)
			P_LinedefExecute(sec2->tag, mobj, sec2);
	}

	if (mobj->scale != mobj->destscale)
		P_MobjScaleThink(mobj); // Slowly scale up/down to reach your destscale.

	if ((mobj->type == MT_GHOST || mobj->type == MT_THOK) && mobj->fuse > 0) // Not guaranteed to be MF_SCENERY or not MF_SCENERY!
	{
		if (mobj->flags2 & MF2_BOSSNOTRAP) // "fast" flag
		{
			if ((signed)((mobj->frame & FF_TRANSMASK) >> FF_TRANSSHIFT) < (NUMTRANSMAPS-1) - (2*mobj->fuse)/3)
				// fade out when nearing the end of fuse...
				mobj->frame = (mobj->frame & ~FF_TRANSMASK) | (((NUMTRANSMAPS-1) - (2*mobj->fuse)/3) << FF_TRANSSHIFT);
		}
		else
		{
			if ((signed)((mobj->frame & FF_TRANSMASK) >> FF_TRANSSHIFT) < (NUMTRANSMAPS-1) - mobj->fuse / 2)
				// fade out when nearing the end of fuse...
				mobj->frame = (mobj->frame & ~FF_TRANSMASK) | (((NUMTRANSMAPS-1) - mobj->fuse / 2) << FF_TRANSSHIFT);
		}
	}

	// Special thinker for scenery objects
	if (mobj->flags & MF_SCENERY)
	{
		P_MobjSceneryThink(mobj);
		return;
	}

	// Check for a Lua thinker first
	if (!mobj->player)
	{
		if (LUAh_MobjThinker(mobj) || P_MobjWasRemoved(mobj))
			return;
	}
	else if (!mobj->player->spectator)
	{
		// You cannot short-circuit the player thinker like you can other thinkers.
		LUAh_MobjThinker(mobj);
		if (P_MobjWasRemoved(mobj))
			return;
	}

	// if it's pushable, or if it would be pushable other than temporary disablement, use the
	// separate thinker
	if (mobj->flags & MF_PUSHABLE || (mobj->info->flags & MF_PUSHABLE && mobj->fuse))
	{
		if (!P_MobjPushableThink(mobj))
			return;
	}
	else if (mobj->flags & MF_BOSS)
	{
		if (!P_MobjBossThink(mobj))
			return;
	}
	else if (mobj->health <= 0) // Dead things think differently than the living.
	{
		if (!P_MobjDeadThink(mobj))
			return;
	}
	else
	{
		if (!P_MobjRegularThink(mobj))
			return;
	}
	if (P_MobjWasRemoved(mobj))
		return;

	if (mobj->flags2 & MF2_FIRING)
		P_FiringThink(mobj);

	if (mobj->flags & MF_AMBIENT)
	{
		if (!(leveltime % mobj->health) && mobj->info->seesound)
			S_StartSound(mobj, mobj->info->seesound);
		return;
	}

	// Check fuse
	if (mobj->fuse && !P_FuseThink(mobj))
		return;

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
		|| P_IsObjectInGoop(mobj))
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

	// Sliding physics for slidey mobjs!
	if (mobj->type == MT_FLINGRING
		|| mobj->type == MT_FLINGCOIN
		|| mobj->type == MT_FLINGBLUESPHERE
		|| mobj->type == MT_FLINGNIGHTSCHIP
		|| P_WeaponOrPanel(mobj->type)
		|| mobj->type == MT_FLINGEMERALD
		|| mobj->type == MT_BIGTUMBLEWEED
		|| mobj->type == MT_LITTLETUMBLEWEED
		|| mobj->type == MT_CANNONBALLDECOR
		|| mobj->type == MT_FALLINGROCK) {
		P_TryMove(mobj, mobj->x, mobj->y, true); // Sets mo->standingslope correctly
		//if (mobj->standingslope) CONS_Printf("slope physics on mobj\n");
		P_ButteredSlope(mobj);
	}

	if (mobj->flags & (MF_ENEMY|MF_BOSS) && mobj->health
		&& P_CheckDeathPitCollide(mobj)) // extra pit check in case these didn't have momz
	{
		P_KillMobj(mobj, NULL, NULL, DMG_DEATHPIT);
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
			P_KillMobj(mobj, NULL, NULL, DMG_CRUSHED);
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

	if (mobj->type == MT_MINECART && mobj->health)
	{
		// If player is ded, remove this minecart
		if (!mobj->target || P_MobjWasRemoved(mobj->target) || !mobj->target->health || !mobj->target->player || mobj->target->player->powers[pw_carry] != CR_MINECART)
		{
			P_KillMobj(mobj, NULL, NULL, 0);
			return;
		}
	}

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
		|| P_IsObjectInGoop(mobj))
	{
		if (!P_SceneryZMovement(mobj))
			return; // mobj was removed
		P_CheckPosition(mobj, mobj->x, mobj->y); // Need this to pick up objects!
		if (P_MobjWasRemoved(mobj))
			return;
		mobj->floorz = tmfloorz;
		mobj->ceilingz = tmceilingz;
		mobj->floorrover = tmfloorrover;
		mobj->ceilingrover = tmceilingrover;
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

static fixed_t P_DefaultMobjShadowScale (mobj_t *thing)
{
	switch (thing->type)
	{
		case MT_PLAYER:
		case MT_ROLLOUTROCK:

		case MT_EGGMOBILE4_MACE:
		case MT_SMALLMACE:
		case MT_BIGMACE:

		case MT_SMALLGRABCHAIN:
		case MT_BIGGRABCHAIN:

		case MT_YELLOWSPRINGBALL:
		case MT_REDSPRINGBALL:

			return FRACUNIT;

		case MT_RING:
		case MT_FLINGRING:

		case MT_BLUESPHERE:
		case MT_FLINGBLUESPHERE:
		case MT_BOMBSPHERE:

		case MT_REDTEAMRING:
		case MT_BLUETEAMRING:
		case MT_REDFLAG:
		case MT_BLUEFLAG:

		case MT_EMBLEM:

		case MT_TOKEN:
		case MT_EMERALD1:
		case MT_EMERALD2:
		case MT_EMERALD3:
		case MT_EMERALD4:
		case MT_EMERALD5:
		case MT_EMERALD6:
		case MT_EMERALD7:
		case MT_EMERHUNT:
		case MT_FLINGEMERALD:

			return 2*FRACUNIT/3;

		default:

			if (thing->flags & (MF_ENEMY|MF_BOSS))
				return FRACUNIT;
			else
				return 0;
	}
}

//
// P_SpawnMobj
//
mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	const mobjinfo_t *info = &mobjinfo[type];
	SINT8 sc = -1;
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

	mobj->health = (info->spawnhealth ? info->spawnhealth : 1);

	mobj->reactiontime = info->reactiontime;

	mobj->lastlook = -1; // stuff moved in P_enemy.P_LookForPlayer

	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	st = &states[info->spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame; // FF_FRAMEMASK for frame, and other bits..
	P_SetupStateAnimation(mobj, st);

	mobj->friction = ORIG_FRICTION;

	mobj->movefactor = FRACUNIT;

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

	mobj->floorz   = P_GetSectorFloorZAt  (mobj->subsector->sector, x, y);
	mobj->ceilingz = P_GetSectorCeilingZAt(mobj->subsector->sector, x, y);

	mobj->floorrover = NULL;
	mobj->ceilingrover = NULL;

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

		// defaults onground
		if (mobj->z + mobj->height == mobj->ceilingz)
			mobj->eflags |= MFE_ONGROUND;
	}
	else
		mobj->z = z;

	// Set shadowscale here, before spawn hook so that Lua can change it
	mobj->shadowscale = P_DefaultMobjShadowScale(mobj);

	// DANGER! This can cause P_SpawnMobj to return NULL!
	// Avoid using P_RemoveMobj on the newly created mobj in "MobjSpawn" Lua hooks!
	if (LUAh_MobjSpawn(mobj))
	{
		if (P_MobjWasRemoved(mobj))
			return NULL;
	}
	else if (P_MobjWasRemoved(mobj))
		return NULL;
	else
	switch (mobj->type)
	{
		case MT_ALTVIEWMAN:
			if (titlemapinaction) mobj->flags &= ~MF_NOTHINK;
			break;
		case MT_LOCKONINF:
			P_SetScale(mobj, (mobj->destscale = 3*mobj->scale));
			break;
		case MT_CYBRAKDEMON_NAPALM_BOMB_LARGE:
			mobj->fuse = mobj->info->painchance;
			break;
		case MT_BLACKEGGMAN:
			{
				mobj_t *spawn = P_SpawnMobj(mobj->x, mobj->z, mobj->z+mobj->height-16*FRACUNIT, MT_BLACKEGGMAN_HELPER);
				spawn->destscale = mobj->scale;
				P_SetScale(spawn, mobj->scale);
				P_SetTarget(&spawn->target, mobj);
			}
			break;
		case MT_FAKEMOBILE:
		case MT_EGGSHIELD:
			mobj->flags2 |= MF2_INVERTAIMABLE;
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
			}
			break;
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
			}
			break;
		case MT_CRUSHSTACEAN:
			{
				mobj_t *bigmeatyclaw = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_CRUSHCLAW);
				bigmeatyclaw->angle = mobj->angle + ((mobj->flags2 & MF2_AMBUSH) ? ANGLE_90 : ANGLE_270);;
				P_SetTarget(&mobj->tracer, bigmeatyclaw);
				P_SetTarget(&bigmeatyclaw->tracer, mobj);
				mobj->reactiontime >>= 1;
			}
			break;
		case MT_BANPYURA:
			{
				mobj_t *bigmeatyclaw = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_BANPSPRING);
				bigmeatyclaw->angle = mobj->angle + ((mobj->flags2 & MF2_AMBUSH) ? ANGLE_90 : ANGLE_270);;
				P_SetTarget(&mobj->tracer, bigmeatyclaw);
				P_SetTarget(&bigmeatyclaw->tracer, mobj);
				mobj->reactiontime >>= 1;
			}
			break;
		case MT_BIGMINE:
			mobj->extravalue1 = FixedHypot(mobj->x, mobj->y)>>FRACBITS;
			break;
		case MT_WAVINGFLAG1:
		case MT_WAVINGFLAG2:
			{
				mobj_t *prev = mobj, *cur;
				UINT8 i;
				for (i = 0; i <= 16; i++) // probably should be < but staying authentic to the Lua version
				{
					cur = P_SpawnMobjFromMobj(mobj, 0, 0, 0, ((mobj->type == MT_WAVINGFLAG1) ? MT_WAVINGFLAGSEG1 : MT_WAVINGFLAGSEG2));;
					P_SetTarget(&prev->tracer, cur);
					cur->extravalue1 = i;
					prev = cur;
				}
			}
			break;
		case MT_EGGMOBILE2:
			// Special condition for the 2nd boss.
			mobj->watertop = mobj->info->speed;
			break;
		case MT_EGGMOBILE3:
			mobj->movefactor = -512*FRACUNIT;
			mobj->flags2 |= MF2_CLASSICPUSH;
			break;
		case MT_EGGMOBILE4:
			mobj->flags2 |= MF2_INVERTAIMABLE;
			break;
		case MT_FLICKY_08:
			mobj->color = (P_RandomChance(FRACUNIT/2) ? SKINCOLOR_RED : SKINCOLOR_AQUA);
			break;
		case MT_BALLOON:
			mobj->color = SKINCOLOR_RED;
			break;
		case MT_EGGROBO1:
			mobj->movecount = P_RandomKey(13);
			mobj->color = SKINCOLOR_RUBY + P_RandomKey(numskincolors - SKINCOLOR_RUBY);
			break;
		case MT_HIVEELEMENTAL:
			mobj->extravalue1 = 5;
			break;
		case MT_SMASHINGSPIKEBALL:
			mobj->movecount = mobj->z;
			break;
		case MT_SPINBOBERT:
			{
				mobj_t *fire;
				fire = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_SPINBOBERT_FIRE1);
				P_SetTarget(&fire->target, mobj);
				P_SetTarget(&mobj->hnext, fire);
				fire = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_SPINBOBERT_FIRE2);
				P_SetTarget(&fire->target, mobj);
				P_SetTarget(&mobj->hprev, fire);
			}
			break;
		case MT_REDRING: // Make MT_REDRING red by default
			mobj->color = skincolor_redring;
			break;
		case MT_SMALLBUBBLE: // Bubbles eventually dissipate, in case they get caught somewhere.
		case MT_MEDIUMBUBBLE:
		case MT_EXTRALARGEBUBBLE:
			mobj->fuse += 30 * TICRATE;
			break;
		case MT_NIGHTSDRONE:
			nummaprings = -1; // no perfect bonus, rings are free
			break;
		case MT_EGGCAPSULE:
			mobj->reactiontime = 0;
			mobj->extravalue1 = mobj->cvmem =\
			 mobj->cusval = mobj->movecount =\
			 mobj->lastlook = mobj->extravalue2 = -1;
			break;
		case MT_REDTEAMRING:
			mobj->color = skincolor_redteam;
			break;
		case MT_BLUETEAMRING:
			mobj->color = skincolor_blueteam;
			break;
		case MT_RING:
		case MT_COIN:
		case MT_NIGHTSSTAR:
			if (nummaprings >= 0)
				nummaprings++;
			break;
		case MT_METALSONIC_RACE:
			mobj->skin = &skins[5];
			/* FALLTHRU */
		case MT_METALSONIC_BATTLE:
			mobj->color = skins[5].prefcolor;
			sc = 5;
			break;
		case MT_FANG:
			sc = 4;
			break;
		case MT_ROSY:
			sc = 3;
			break;
		case MT_CORK:
			mobj->flags2 |= MF2_SUPERFIRE;
			break;
		case MT_FBOMB:
			mobj->flags2 |= MF2_EXPLOSION;
			break;
		case MT_OILLAMP:
			{
				mobj_t* overlay = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_OVERLAY);
				P_SetTarget(&overlay->target, mobj);
				P_SetMobjState(overlay, S_OILLAMPFLARE);
				break;
			}
		case MT_TNTBARREL:
			mobj->momx = 1; //stack hack
			mobj->flags2 |= MF2_INVERTAIMABLE;
			break;
		case MT_MINECARTEND:
			P_SetTarget(&mobj->tracer, P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_MINECARTENDSOLID));
			mobj->tracer->angle = mobj->angle + ANGLE_90;
			break;
		case MT_TORCHFLOWER:
			{
				mobj_t *fire = P_SpawnMobjFromMobj(mobj, 0, 0, 46*FRACUNIT, MT_FLAME);
				P_SetTarget(&mobj->target, fire);
				break;
			}
		case MT_PYREFLY:
			mobj->extravalue1 = (FixedHypot(mobj->x, mobj->y)/FRACUNIT) % 360;
			mobj->extravalue2 = 0;
			mobj->fuse = 100;
			break;
		case MT_SIGN:
			P_SetTarget(&mobj->tracer, P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_OVERLAY));
			P_SetTarget(&mobj->tracer->target, mobj);
			P_SetMobjState(mobj->tracer, S_SIGNBOARD);
			mobj->tracer->movedir = ANGLE_90;
		default:
			break;
	}

	if (sc != -1 && !(mobj->flags2 & MF2_SLIDEPUSH))
	{
		UINT8 i;
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i] || players[i].spectator)
				continue;

			if (players[i].skin == sc)
			{
				mobj->color = SKINCOLOR_SILVER;
				mobj->colorized = true;
				mobj->flags2 |= MF2_SLIDEPUSH;
				break;
			}
		}
	}

	if (!(mobj->flags & MF_NOTHINK))
		P_AddThinker(THINK_MOBJ, &mobj->thinker);

	if (mobj->skin) // correct inadequecies above.
	{
		mobj->sprite2 = P_GetSkinSprite2(mobj->skin, (mobj->frame & FF_FRAMEMASK), NULL);
		mobj->frame &= ~FF_FRAMEMASK;
	}

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
			astate = st;
			st->action.acp1(mobj);
			// DANGER! This can cause P_SpawnMobj to return NULL!
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
	fixed_t starting_floorz;

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
	P_SetupStateAnimation((mobj_t*)mobj, st);

	// set subsector and/or block links
	P_SetPrecipitationThingPosition(mobj);

	mobj->floorz   = starting_floorz = P_GetSectorFloorZAt  (mobj->subsector->sector, x, y);
	mobj->ceilingz                   = P_GetSectorCeilingZAt(mobj->subsector->sector, x, y);

	mobj->floorrover = NULL;
	mobj->ceilingrover = NULL;

	mobj->z = z;
	mobj->momz = mobjinfo[type].speed;

	mobj->thinker.function.acp1 = (actionf_p1)P_NullPrecipThinker;
	P_AddThinker(THINK_PRECIP, &mobj->thinker);

	CalculatePrecipFloor(mobj);

	if (mobj->floorz != starting_floorz)
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
	mo->precipflags |= PCF_RAIN;
	//mo->thinker.function.acp1 = (actionf_p1)P_RainThinker;
	return mo;
}

static inline precipmobj_t *P_SpawnSnowMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
	precipmobj_t *mo = P_SpawnPrecipMobj(x,y,z,type);
	//mo->thinker.function.acp1 = (actionf_p1)P_SnowThinker;
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
	if (P_MobjWasRemoved(mobj))
		return; // something already removing this mobj.

	mobj->thinker.function.acp1 = (actionf_p1)P_RemoveThinkerDelayed; // shh. no recursing.
	LUAh_MobjRemoved(mobj);
	mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker; // needed for P_UnsetThingPosition, etc. to work.

	// Rings only, please!
	if (mobj->spawnpoint &&
		(mobj->type == MT_RING
		|| mobj->type == MT_COIN
		|| mobj->type == MT_NIGHTSSTAR
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

	if (mobj->player && mobj->player->followmobj)
	{
		P_RemoveMobj(mobj->player->followmobj);
		P_SetTarget(&mobj->player->followmobj, NULL);
	}

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

	if (mobj->hnext && !P_MobjWasRemoved(mobj->hnext))
		P_SetTarget(&mobj->hnext->hprev, mobj->hprev);
	if (mobj->hprev && !P_MobjWasRemoved(mobj->hprev))
		P_SetTarget(&mobj->hprev->hnext, mobj->hnext);

	P_SetTarget(&mobj->hnext, P_SetTarget(&mobj->hprev, NULL));

	// DBG: set everything in mobj_t to 0xFF instead of leaving it. debug memory error.
#ifdef SCRAMBLE_REMOVED
	// Invalidate mobj_t data to cause crashes if accessed!
	memset((UINT8 *)mobj + sizeof(thinker_t), 0xff, sizeof(mobj_t) - sizeof(thinker_t));
#endif

	// free block
	if (!mobj->thinker.next)
	{ // Uh-oh, the mobj doesn't think, P_RemoveThinker would never go through!
		INT32 prevreferences;
		if (!mobj->thinker.references)
		{
			Z_Free(mobj); // No refrrences? Can be removed immediately! :D
			return;
		}

		prevreferences = mobj->thinker.references;
		P_AddThinker(THINK_MOBJ, (thinker_t *)mobj);
		mobj->thinker.references = prevreferences;
	}

	P_RemoveThinker((thinker_t *)mobj);
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

void P_SpawnPrecipitation(void)
{
	INT32 i, mrand;
	fixed_t basex, basey, x, y, height;
	subsector_t *precipsector = NULL;
	precipmobj_t *rainmo = NULL;

	if (dedicated || !(cv_drawdist_precip.value) || curWeather == PRECIP_NONE)
		return;

	// Use the blockmap to narrow down our placing patterns
	for (i = 0; i < bmapwidth*bmapheight; ++i)
	{
		basex = bmaporgx + (i % bmapwidth) * MAPBLOCKSIZE;
		basey = bmaporgy + (i / bmapwidth) * MAPBLOCKSIZE;

		x = basex + ((M_RandomKey(MAPBLOCKUNITS<<3)<<FRACBITS)>>3);
		y = basey + ((M_RandomKey(MAPBLOCKUNITS<<3)<<FRACBITS)>>3);

		precipsector = R_PointInSubsectorOrNull(x, y);

		// No sector? Stop wasting time,
		// move on to the next entry in the blockmap
		if (!precipsector)
			continue;

		// Exists, but is too small for reasonable precipitation.
		if (!(precipsector->sector->floorheight <= precipsector->sector->ceilingheight - (32<<FRACBITS)))
			continue;

		// Don't set height yet...
		height = precipsector->sector->ceilingheight;

		if (curWeather == PRECIP_SNOW)
		{
			// Not in a sector with visible sky -- exception for NiGHTS.
			if ((!(maptol & TOL_NIGHTS) && (precipsector->sector->ceilingpic != skyflatnum)) == !(precipsector->sector->flags & SF_INVERTPRECIP))
				continue;

			rainmo = P_SpawnSnowMobj(x, y, height, MT_SNOWFLAKE);
			mrand = M_RandomByte();
			if (mrand < 64)
				P_SetPrecipMobjState(rainmo, S_SNOW3);
			else if (mrand < 144)
				P_SetPrecipMobjState(rainmo, S_SNOW2);
		}
		else // everything else.
		{
			// Not in a sector with visible sky.
			if ((precipsector->sector->ceilingpic != skyflatnum) == !(precipsector->sector->flags & SF_INVERTPRECIP))
				continue;

			rainmo = P_SpawnRainMobj(x, y, height, MT_RAIN);
		}

		// Randomly assign a height, now that floorz is set.
		rainmo->z = M_RandomRange(rainmo->floorz>>FRACBITS, rainmo->ceilingz>>FRACBITS)<<FRACBITS;
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
			/* FALLTHRU */
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

	if (sound_disabled)
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

/** Returns corresponding mobj type from mapthing number.
 * \param mthingtype Mapthing number in question.
 * \return Mobj type; MT_UNKNOWN if nothing found.
 */
static mobjtype_t P_GetMobjtype(UINT16 mthingtype)
{
	mobjtype_t i;
	for (i = 0; i < NUMMOBJTYPES; i++)
		if (mthingtype == mobjinfo[i].doomednum)
			return i;
	return MT_UNKNOWN;
}

//
// P_RespawnSpecials
//
void P_RespawnSpecials(void)
{
	mapthing_t *mthing = NULL;

	// only respawn items when cv_itemrespawn is on
	if (!(netgame || multiplayer) // Never respawn in single player
	|| (maptol & TOL_NIGHTS)      // Never respawn in NiGHTs
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
		P_SpawnMapThing(mthing);

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
		G_PlayerReborn(playernum, false);

	// spawn as spectator determination
	if (!G_GametypeHasSpectators())
	{
		p->spectator = p->outofcoop =
		(((multiplayer || netgame) && G_CoopGametype()) // only question status in coop
		&& ((leveltime > 0
		&& ((G_IsSpecialStage(gamemap)) // late join special stage
		|| (cv_coopstarposts.value == 2 && (p->jointime < 1 || p->outofcoop)))) // late join or die in new coop
		|| (!P_GetLives(p) && p->lives <= 0))); // game over and can't redistribute lives
	}
	else
	{
		p->outofcoop = false;
		if (netgame && p->jointime < 1)
		{
			// Averted by GTR_NOSPECTATORSPAWN.
			p->spectator = (gametyperules & GTR_NOSPECTATORSPAWN) ? false : true;
		}
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

	if ((netgame || multiplayer) && ((gametyperules & GTR_SPAWNINVUL) || leveltime) && !p->spectator && !(maptol & TOL_NIGHTS))
		p->powers[pw_flashing] = flashingtics-1; // Babysitting deterrent

	mobj = P_SpawnMobj(0, 0, 0, MT_PLAYER);
	(mobj->player = p)->mo = mobj;

	mobj->angle = 0;

	// set color translations for player sprites
	mobj->color = p->skincolor;

	// set 'spritedef' override in mobj for player skins.. (see ProjectSprite)
	// (usefulness: when body mobj is detached from player (who respawns),
	// the dead body mobj retains the skin through the 'spritedef' override).
	mobj->skin = &skins[p->skin];
	P_SetupStateAnimation(mobj, mobj->state);

	mobj->health = 1;
	p->playerstate = PST_LIVE;

	p->bonustime = false;
	p->realtime = leveltime;
	p->followitem = skins[p->skin].followitem;

	// Make sure player's stats are reset if they were in dashmode!
	if (p->dashmode)
	{
		p->dashmode = 0;
		p->normalspeed = skins[p->skin].normalspeed;
		p->jumpfactor = skins[p->skin].jumpfactor;
	}

	//awayview stuff
	p->awayviewmobj = NULL;
	p->awayviewtics = 0;

	// set the scale to the mobj's destscale so settings get correctly set.  if we don't, they sometimes don't.
	P_SetScale(mobj, mobj->destscale);
	P_FlashPal(p, 0, 0); // Resets

	// Set bounds accurately.
	mobj->radius = FixedMul(skins[p->skin].radius, mobj->scale);
	mobj->height = P_GetPlayerHeight(p);

	if (!leveltime && !p->spectator && ((maptol & TOL_NIGHTS) == TOL_NIGHTS) != (G_IsSpecialStage(gamemap))) // non-special NiGHTS stage or special non-NiGHTS stage
	{
		if (maptol & TOL_NIGHTS)
		{
			if (p == players) // this is totally the wrong place to do this aaargh.
			{
				mobj_t *idya = P_SpawnMobjFromMobj(mobj, 0, 0, mobj->height, MT_GOTEMERALD);
				idya->health = 0; // for identification
				P_SetTarget(&idya->target, mobj);
				P_SetMobjState(idya, mobjinfo[MT_GOTEMERALD].missilestate);
				P_SetTarget(&mobj->tracer, idya);
			}
		}
		else if (sstimer)
			p->nightstime = sstimer;
	}

	// Spawn with a pity shield if necessary.
	P_DoPityCheck(p);
}

void P_AfterPlayerSpawn(INT32 playernum)
{
	player_t *p = &players[playernum];
	mobj_t *mobj = p->mo;

	P_SetPlayerAngle(p, mobj->angle);

	p->viewheight = 41*p->height/48;

	if (p->mo->eflags & MFE_VERTICALFLIP)
		p->viewz = p->mo->z + p->mo->height - p->viewheight;
	else
		p->viewz = p->mo->z + p->viewheight;

	if (playernum == consoleplayer)
	{
		// wake up the status bar
		ST_Start();
		// wake up the heads up text
		HU_Start();
	}

	p->drawangle = mobj->angle;

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

	if (p->pflags & PF_FINISHED)
		P_GiveFinishFlags(p);
}

// spawn it at a playerspawn mapthing
void P_MovePlayerToSpawn(INT32 playernum, mapthing_t *mthing)
{
	fixed_t x = 0, y = 0;
	angle_t angle = 0;

	fixed_t z;
	sector_t *sector;
	fixed_t floor, ceiling, ceilingspawn;

	player_t *p = &players[playernum];
	mobj_t *mobj = p->mo;
	I_Assert(mobj != NULL);

	if (mthing)
	{
		x = mthing->x << FRACBITS;
		y = mthing->y << FRACBITS;
		angle = FixedAngle(mthing->angle<<FRACBITS);
	}
	//spawn at the origin as a desperation move if there is no mapthing

	// set Z height
	sector = R_PointInSubsector(x, y)->sector;

	floor   = P_GetSectorFloorZAt  (sector, x, y);
	ceiling = P_GetSectorCeilingZAt(sector, x, y);
	ceilingspawn = ceiling - mobjinfo[MT_PLAYER].height;

	if (mthing)
	{
		fixed_t offset = mthing->z << FRACBITS;

		// Flagging a player's ambush will make them start on the ceiling
		// Objectflip inverts
		if (!!(mthing->options & MTF_AMBUSH) ^ !!(mthing->options & MTF_OBJECTFLIP))
			z = ceilingspawn - offset;
		else
			z = floor + offset;

		if (mthing->options & MTF_OBJECTFLIP) // flip the player!
		{
			mobj->eflags |= MFE_VERTICALFLIP;
			mobj->flags2 |= MF2_OBJECTFLIP;
		}
		if (mthing->options & MTF_AMBUSH)
			P_SetPlayerMobjState(mobj, S_PLAY_FALL);
		else if (metalrecording)
			P_SetPlayerMobjState(mobj, S_PLAY_WAIT);
	}
	else
		z = floor;

	if (z < floor)
		z = floor;
	else if (z > ceilingspawn)
		z = ceilingspawn;

	mobj->floorz = floor;
	mobj->ceilingz = ceiling;

	P_UnsetThingPosition(mobj);
	mobj->x = x;
	mobj->y = y;
	P_SetThingPosition(mobj);

	mobj->z = z;
	if (mobj->flags2 & MF2_OBJECTFLIP)
	{
		if (mobj->z + mobj->height == mobj->ceilingz)
			mobj->eflags |= MFE_ONGROUND;
	}
	else if (mobj->z == mobj->floorz)
		mobj->eflags |= MFE_ONGROUND;

	mobj->angle = angle;

	P_AfterPlayerSpawn(playernum);
}

void P_MovePlayerToStarpost(INT32 playernum)
{
	fixed_t z;
	sector_t *sector;
	fixed_t floor, ceiling;

	player_t *p = &players[playernum];
	mobj_t *mobj = p->mo;
	I_Assert(mobj != NULL);

	P_UnsetThingPosition(mobj);
	mobj->x = p->starpostx << FRACBITS;
	mobj->y = p->starposty << FRACBITS;
	P_SetThingPosition(mobj);
	sector = R_PointInSubsector(mobj->x, mobj->y)->sector;

	floor   = P_GetSectorFloorZAt  (sector, mobj->x, mobj->y);
	ceiling = P_GetSectorCeilingZAt(sector, mobj->x, mobj->y);

	z = p->starpostz << FRACBITS;

	P_SetScale(mobj, (mobj->destscale = abs(p->starpostscale)));

	if (p->starpostscale < 0)
	{
		mobj->flags2 |= MF2_OBJECTFLIP;
		if (z >= ceiling)
		{
			mobj->eflags |= MFE_ONGROUND;
			z = ceiling;
		}
		z -= mobj->height;
	}
	else if (z <= floor)
	{
		mobj->eflags |= MFE_ONGROUND;
		z = floor;
	}

	mobj->floorz = floor;
	mobj->ceilingz = ceiling;

	mobj->z = z;

	mobj->angle = p->starpostangle;

	P_AfterPlayerSpawn(playernum);

	if (!(netgame || multiplayer))
		leveltime = p->starposttime;
}

#define MAXHUNTEMERALDS 64
mapthing_t *huntemeralds[MAXHUNTEMERALDS];
INT32 numhuntemeralds;

fixed_t P_GetMobjSpawnHeight(const mobjtype_t mobjtype, const fixed_t x, const fixed_t y, const fixed_t offset, const boolean flip)
{
	const subsector_t *ss = R_PointInSubsector(x, y);

	// Axis objects snap to the floor.
	if (mobjtype == MT_AXIS || mobjtype == MT_AXISTRANSFER || mobjtype == MT_AXISTRANSFERLINE)
		return ONFLOORZ;

	// Establish height.
	if (flip)
		return P_GetSectorCeilingZAt(ss->sector, x, y) - offset - mobjinfo[mobjtype].height;
	else
		return P_GetSectorFloorZAt(ss->sector, x, y) + offset;
}

fixed_t P_GetMapThingSpawnHeight(const mobjtype_t mobjtype, const mapthing_t* mthing, const fixed_t x, const fixed_t y)
{
	fixed_t offset = mthing->z << FRACBITS;
	boolean flip = (!!(mobjinfo[mobjtype].flags & MF_SPAWNCEILING) ^ !!(mthing->options & MTF_OBJECTFLIP));

	switch (mobjtype)
	{
	// Bumpers never spawn flipped.
	case MT_NIGHTSBUMPER:
		flip = false;
		break;

	// Objects with a non-zero default height.
	case MT_CRAWLACOMMANDER:
	case MT_DETON:
	case MT_JETTBOMBER:
	case MT_JETTGUNNER:
	case MT_EGGMOBILE2:
		if (!offset)
			offset = 33*FRACUNIT;
		break;
	case MT_EGGMOBILE:
		if (!offset)
			offset = 128*FRACUNIT;
		break;
	case MT_GOLDBUZZ:
	case MT_REDBUZZ:
		if (!offset)
			offset = 288*FRACUNIT;
		break;

	// Horizontal springs, may float additional units with MTF_AMBUSH.
	case MT_YELLOWHORIZ:
	case MT_REDHORIZ:
	case MT_BLUEHORIZ:
		offset += mthing->options & MTF_AMBUSH ? 16*FRACUNIT : 0;
		break;

	// Ring-like items, may float additional units with MTF_AMBUSH.
	case MT_SPIKEBALL:
	case MT_EMERHUNT:
	case MT_EMERALDSPAWN:
	case MT_TOKEN:
	case MT_EMBLEM:
	case MT_RING:
	case MT_REDTEAMRING:
	case MT_BLUETEAMRING:
	case MT_COIN:
	case MT_BLUESPHERE:
	case MT_BOMBSPHERE:
	case MT_NIGHTSCHIP:
	case MT_NIGHTSSTAR:
		offset += mthing->options & MTF_AMBUSH ? 24*FRACUNIT : 0;
		break;

	// Remaining objects.
	default:
		if (P_WeaponOrPanel(mobjtype))
			offset += mthing->options & MTF_AMBUSH ? 24*FRACUNIT : 0;
	}

	if (!offset) // Snap to the surfaces when there's no offset set.
	{
		if (flip)
			return ONCEILINGZ;
		else
			return ONFLOORZ;
	}

	return P_GetMobjSpawnHeight(mobjtype, x, y, offset, flip);
}

static boolean P_SpawnNonMobjMapThing(mapthing_t *mthing)
{
#if MAXPLAYERS > 32
	You should think about modifying the deathmatch starts to take full advantage of this!
#endif
	if (mthing->type <= MAXPLAYERS) // Player starts
	{
		// save spots for respawning in network games
		if (!metalrecording)
			playerstarts[mthing->type - 1] = mthing;
		return true;
	}
	else if (mthing->type == 33) // Match starts
	{
		if (numdmstarts < MAX_DM_STARTS)
		{
			deathmatchstarts[numdmstarts] = mthing;
			mthing->type = 0;
			numdmstarts++;
		}
		return true;
	}
	else if (mthing->type == 34) // Red CTF starts
	{
		if (numredctfstarts < MAXPLAYERS)
		{
			redctfstarts[numredctfstarts] = mthing;
			mthing->type = 0;
			numredctfstarts++;
		}
		return true;
	}
	else if (mthing->type == 35) // Blue CTF starts
	{
		if (numbluectfstarts < MAXPLAYERS)
		{
			bluectfstarts[numbluectfstarts] = mthing;
			mthing->type = 0;
			numbluectfstarts++;
		}
		return true;
	}
	else if (metalrecording && mthing->type == mobjinfo[MT_METALSONIC_RACE].doomednum)
	{ // If recording, you ARE Metal Sonic. Do not spawn it, do not save normal spawnpoints.
		playerstarts[0] = mthing;
		return true;
	}
	else if (mthing->type == 750 // Slope vertex point (formerly chaos spawn)
		     || (mthing->type >= 600 && mthing->type <= 609) // Special placement patterns
		     || mthing->type == 1705 || mthing->type == 1713) // Hoops
		return true; // These are handled elsewhere.
	else if (mthing->type == mobjinfo[MT_EMERHUNT].doomednum)
	{
		// Emerald Hunt is Coop only. Don't spawn the emerald yet, but save the spawnpoint for later.
		if ((gametyperules & GTR_EMERALDHUNT) && numhuntemeralds < MAXHUNTEMERALDS)
			huntemeralds[numhuntemeralds++] = mthing;
		return true;
	}

	return false;
}

static boolean P_AllowMobjSpawn(mapthing_t* mthing, mobjtype_t i)
{
	switch (i)
	{
	case MT_EMERALD1:
	case MT_EMERALD2:
	case MT_EMERALD3:
	case MT_EMERALD4:
	case MT_EMERALD5:
	case MT_EMERALD6:
	case MT_EMERALD7:
		if (!G_CoopGametype()) // Don't place emeralds in non-coop modes
			return false;

		if (metalrecording)
			return false; // Metal Sonic isn't for collecting emeralds.

		if (emeralds & mobjinfo[i].speed) // You already have this emerald!
			return false;

		break;
	case MT_EMERALDSPAWN:
		if (!cv_powerstones.value)
			return false;

		if (!(gametyperules & GTR_POWERSTONES))
			return false;

		runemeraldmanager = true;
		break;
	case MT_ROSY:
		if (!(G_CoopGametype() || (mthing->options & MTF_EXTRA)))
			return false; // she doesn't hang out here

		if (!mariomode && !(netgame || multiplayer) && players[consoleplayer].skin == 3)
			return false; // no doubles

		break;
	case MT_TOKEN:
		if (!(gametyperules & GTR_EMERALDTOKENS))
			return false; // Gametype's not right

		if (tokenbits == 30)
			return false; // Too many tokens

		if (tokenlist & (1 << tokenbits++))
			return false; // You already got this token

		break;
	case MT_EMBLEM:
		if (netgame || multiplayer)
			return false; // Single player

		if (modifiedgame && !savemoddata)
			return false; // No cheating!!

		break;
	default:
		break;
	}

	if (metalrecording) // Metal Sonic can't use these things.
	{
		if ((mobjinfo[i].flags & (MF_ENEMY|MF_BOSS)) || i == MT_TOKEN || i == MT_STARPOST
			|| i == MT_RING || i == MT_BLUETEAMRING || i == MT_REDTEAMRING || i == MT_COIN
			|| i == MT_BLUESPHERE || i == MT_BOMBSPHERE || i == MT_NIGHTSCHIP || i == MT_NIGHTSSTAR)
			return false;
	}

	if (((mobjinfo[i].flags & MF_ENEMY) || (mobjinfo[i].flags & MF_BOSS)) && !(gametyperules & GTR_SPAWNENEMIES))
		return false; // No enemies in ringslinger modes

	if (!(gametyperules & GTR_ALLOWEXIT) && (i == MT_SIGN))
		return false; // Don't spawn exit signs in wrong game modes

	if (!G_PlatformGametype() && (i == MT_STARPOST))
		return false; // Don't spawn starposts in wrong game modes

	if (!G_RingSlingerGametype() || !cv_specialrings.value)
		if (P_WeaponOrPanel(i))
			return false; // Don't place weapons/panels in non-ringslinger modes

	if (!(gametyperules & GTR_TEAMFLAGS)) // CTF specific things
	{
		if (i == MT_BLUEFLAG || i == MT_REDFLAG)
			return false; // No flags in non-CTF modes!
	}
	else
	{
		if ((i == MT_BLUEFLAG && blueflag) || (i == MT_REDFLAG && redflag))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Only one flag per team allowed in CTF!\n"));
			return false;
		}
	}

	if (modeattacking) // Record Attack special stuff
	{
		// Don't spawn starposts that wouldn't be usable
		if (i == MT_STARPOST)
			return false;
	}

	if (ultimatemode)
	{
		if (i == MT_RING || i == MT_REDTEAMRING || i == MT_BLUETEAMRING
			|| i == MT_COIN || i == MT_NIGHTSSTAR || i == MT_NIGHTSCHIP
			|| i == MT_PITY_BOX || i == MT_ELEMENTAL_BOX || i == MT_ATTRACT_BOX
			|| i == MT_FORCE_BOX || i == MT_ARMAGEDDON_BOX || i == MT_WHIRLWIND_BOX
			|| i == MT_FLAMEAURA_BOX || i == MT_BUBBLEWRAP_BOX || i == MT_THUNDERCOIN_BOX
			|| i == MT_RING_BOX || i == MT_STARPOST)
			return false; // No rings or shields in Ultimate mode

		// Don't include the gold repeating boxes here please.
		// They're likely facets of the level's design and therefore required to progress.
	}

	return true;
}

#define nightsreplace ((maptol & TOL_NIGHTS) && !G_IsSpecialStage(gamemap))

static mobjtype_t P_GetMobjtypeSubstitute(mapthing_t *mthing, mobjtype_t i)
{
	// Altering monitor spawns via cvars
	// If MF_GRENADEBOUNCE is set in the monitor's info,
	// skip this step. (Used for gold monitors)
	// Yeah, this is a dirty hack.
	if ((mobjinfo[i].flags & (MF_MONITOR|MF_GRENADEBOUNCE)) == MF_MONITOR)
	{
		if (gametyperules & GTR_RACE)
		{
			// Set powerup boxes to user settings for competition.
			switch (cv_competitionboxes.value)
			{
			case 1: // Mystery
				return MT_MYSTERY_BOX;
			case 2: // Teleport
				return MT_MIXUP_BOX;
			case 3: // None
				return MT_NULL; // Don't spawn!
			default:
				return i;
			}
		}
		// Set powerup boxes to user settings for other netplay modes
		else if (!G_CoopGametype())
		{
			switch (cv_matchboxes.value)
			{
			case 1: // Mystery
				return MT_MYSTERY_BOX;
			case 2: // Unchanging
				if (i == MT_MYSTERY_BOX)
					return MT_NULL; // don't spawn
				mthing->options &= ~(MTF_AMBUSH|MTF_OBJECTSPECIAL); // no random respawning!
				return i;
			case 3: // Don't spawn
				return MT_NULL;
			default:
				return i;
			}
		}
	}

	if (nightsreplace)
	{
		if (i == MT_RING || i == MT_REDTEAMRING || i == MT_BLUETEAMRING || i == MT_COIN)
			return MT_NIGHTSSTAR;

		if (i == MT_BLUESPHERE)
			return MT_NIGHTSCHIP;
	}

	if (!(gametyperules & GTR_TEAMS))
	{
		if (i == MT_BLUETEAMRING || i == MT_REDTEAMRING)
			return MT_RING;

		if (i == MT_RING_BLUEBOX || i == MT_RING_REDBOX)
			return MT_RING_BOX;
	}

	if (modeattacking && i == MT_1UP_BOX) // 1UPs -->> Score TVs
	{
		// Either or, doesn't matter which.
		if (mthing->options & (MTF_AMBUSH | MTF_OBJECTSPECIAL))
			return MT_SCORE10K_BOX; // 10,000
		else
			return MT_SCORE1K_BOX; // 1,000
	}

	if (mariomode && i == MT_ROSY)
		return MT_TOAD; // don't remove on penalty of death

	return i;
}

static boolean P_SetupEmblem(mapthing_t *mthing, mobj_t *mobj)
{
	INT32 j;
	emblem_t* emblem = M_GetLevelEmblems(gamemap);
	skincolornum_t emcolor;

	while (emblem)
	{
		if ((emblem->type == ET_GLOBAL || emblem->type == ET_SKIN) && emblem->tag == mthing->angle)
			break;

		emblem = M_GetLevelEmblems(-1);
	}

	if (!emblem)
	{
		CONS_Debug(DBG_GAMELOGIC, "No map emblem for map %d with tag %d found!\n", gamemap, mthing->angle);
		return false;
	}

	j = emblem - emblemlocations;

	I_Assert(emblemlocations[j].sprite >= 'A' && emblemlocations[j].sprite <= 'Z');
	P_SetMobjState(mobj, mobj->info->spawnstate + (emblemlocations[j].sprite - 'A'));

	mobj->health = j + 1;
	emcolor = M_GetEmblemColor(&emblemlocations[j]); // workaround for compiler complaint about bad function casting
	mobj->color = (UINT16)emcolor;

	if (emblemlocations[j].collected
		|| (emblemlocations[j].type == ET_SKIN && emblemlocations[j].var != players[0].skin))
	{
		P_UnsetThingPosition(mobj);
		mobj->flags |= MF_NOCLIP;
		mobj->flags &= ~MF_SPECIAL;
		mobj->flags |= MF_NOBLOCKMAP;
		mobj->frame |= (tr_trans50 << FF_TRANSSHIFT);
		P_SetThingPosition(mobj);
	}
	else
	{
		mobj->frame &= ~FF_TRANSMASK;

		if (emblemlocations[j].type == ET_GLOBAL)
		{
			mobj->reactiontime = emblemlocations[j].var;
			if (emblemlocations[j].var & GE_NIGHTSITEM)
			{
				mobj->flags |= MF_NIGHTSITEM;
				mobj->flags &= ~MF_SPECIAL;
				mobj->flags2 |= MF2_DONTDRAW;
			}
		}
	}
	return true;
}

static boolean P_SetupMace(mapthing_t *mthing, mobj_t *mobj, boolean *doangle)
{
	fixed_t mlength, mmaxlength, mlengthset, mspeed, mphase, myaw, mpitch, mminlength, mnumspokes, mpinch, mroll, mnumnospokes, mwidth, mwidthset, mmin, msound, radiusfactor, widthfactor;
	angle_t mspokeangle;
	mobjtype_t chainlink, macetype, firsttype, linktype;
	boolean mdosound, mdocenter, mchainlike = false;
	mobj_t *spawnee = NULL, *hprev = mobj;
	mobjflag_t mflagsapply;
	mobjflag2_t mflags2apply;
	mobjeflag_t meflagsapply;
	INT32 line;
	const size_t mthingi = (size_t)(mthing - mapthings);

	// Find the corresponding linedef special, using angle as tag
	// P_FindSpecialLineFromTag works here now =D
	line = P_FindSpecialLineFromTag(9, mthing->angle, -1);

	if (line == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "Mace chain (mapthing #%s) needs to be tagged to a #9 parameter line (trying to find tag %d).\n", sizeu1(mthingi), mthing->angle);
		return false;
	}
	/*
	mapthing -
	MTF_AMBUSH :
		MT_SPRINGBALLPOINT - upgrade from yellow to red spring
		anything else - bigger mace/chain theory
	MTF_OBJECTSPECIAL - force silent
	MTF_GRAVFLIP - flips objects, doesn't affect chain arrangements
	Parameter value : number of "spokes"

	linedef -
	ML_NOCLIMB :
		MT_CHAINPOINT/MT_CHAINMACEPOINT with ML_EFFECT1 applied - Direction not controllable
		anything else - no functionality
	ML_EFFECT1 : Swings instead of spins
	ML_EFFECT2 : Linktype is replaced with macetype for all spokes not ending in chains (inverted for MT_FIREBARPOINT)
	ML_EFFECT3 : Spawn a bonus linktype at the hinge point
	ML_EFFECT4 : Don't clip inside the ground
	ML_EFFECT5 : Don't stop thinking when too far away
	*/
	mlength = abs(lines[line].dx >> FRACBITS);
	mspeed = abs(lines[line].dy >> (FRACBITS - 4));
	mphase = (sides[lines[line].sidenum[0]].textureoffset >> FRACBITS) % 360;
	if ((mminlength = -sides[lines[line].sidenum[0]].rowoffset >> FRACBITS) < 0)
		mminlength = 0;
	else if (mminlength > mlength - 1)
		mminlength = mlength - 1;
	mpitch = (lines[line].frontsector->floorheight >> FRACBITS) % 360;
	myaw = (lines[line].frontsector->ceilingheight >> FRACBITS) % 360;

	mnumspokes = mthing->extrainfo + 1;
	mspokeangle = FixedAngle((360*FRACUNIT)/mnumspokes) >> ANGLETOFINESHIFT;

	if (lines[line].backsector)
	{
		mpinch = (lines[line].backsector->floorheight >> FRACBITS) % 360;
		mroll = (lines[line].backsector->ceilingheight >> FRACBITS) % 360;
		mnumnospokes = (sides[lines[line].sidenum[1]].textureoffset >> FRACBITS);
		if ((mwidth = sides[lines[line].sidenum[1]].rowoffset >> FRACBITS) < 0)
			mwidth = 0;
	}
	else
		mpinch = mroll = mnumnospokes = mwidth = 0;

	CONS_Debug(DBG_GAMELOGIC, "Mace/Chain (mapthing #%s):\n"
		"Length is %d (minus %d)\n"
		"Speed is %d\n"
		"Phase is %d\n"
		"Yaw is %d\n"
		"Pitch is %d\n"
		"No. of spokes is %d (%d antispokes)\n"
		"Pinch is %d\n"
		"Roll is %d\n"
		"Width is %d\n",
		sizeu1(mthingi), mlength, mminlength, mspeed, mphase, myaw, mpitch, mnumspokes, mnumnospokes, mpinch, mroll, mwidth);

	if (mnumnospokes > 0 && (mnumnospokes < mnumspokes))
		mnumnospokes = mnumspokes/mnumnospokes;
	else
		mnumnospokes = ((mobj->type == MT_CHAINMACEPOINT) ? (mnumspokes) : 0);

	mobj->lastlook = mspeed;
	mobj->movecount = mobj->lastlook;
	mobj->angle = FixedAngle(myaw << FRACBITS);
	*doangle = false;
	mobj->threshold = (FixedAngle(mpitch << FRACBITS) >> ANGLETOFINESHIFT);
	mobj->movefactor = mpinch;
	mobj->movedir = 0;

	// Mobjtype selection
	switch (mobj->type)
	{
	case MT_SPRINGBALLPOINT:
		macetype = ((mthing->options & MTF_AMBUSH)
			? MT_REDSPRINGBALL
			: MT_YELLOWSPRINGBALL);
		chainlink = MT_SMALLMACECHAIN;
		break;
	case MT_FIREBARPOINT:
		macetype = ((mthing->options & MTF_AMBUSH)
			? MT_BIGFIREBAR
			: MT_SMALLFIREBAR);
		chainlink = MT_NULL;
		break;
	case MT_CUSTOMMACEPOINT:
		macetype = (mobjtype_t)sides[lines[line].sidenum[0]].toptexture;
		if (lines[line].backsector)
			chainlink = (mobjtype_t)sides[lines[line].sidenum[1]].toptexture;
		else
			chainlink = MT_NULL;
		break;
	case MT_CHAINPOINT:
		if (mthing->options & MTF_AMBUSH)
		{
			macetype = MT_BIGGRABCHAIN;
			chainlink = MT_BIGMACECHAIN;
		}
		else
		{
			macetype = MT_SMALLGRABCHAIN;
			chainlink = MT_SMALLMACECHAIN;
		}
		mchainlike = true;
		break;
	default:
		if (mthing->options & MTF_AMBUSH)
		{
			macetype = MT_BIGMACE;
			chainlink = MT_BIGMACECHAIN;
		}
		else
		{
			macetype = MT_SMALLMACE;
			chainlink = MT_SMALLMACECHAIN;
		}
		break;
	}

	if (!macetype && !chainlink)
		return true;

	if (mobj->type == MT_CHAINPOINT)
	{
		if (!mlength)
			return true;
	}
	else
		mlength++;

	firsttype = macetype;

	// Adjustable direction
	if (lines[line].flags & ML_NOCLIMB)
		mobj->flags |= MF_SLIDEME;

	// Swinging
	if (lines[line].flags & ML_EFFECT1)
	{
		mobj->flags2 |= MF2_STRONGBOX;
		mmin = ((mnumnospokes > 1) ? 1 : 0);
	}
	else
		mmin = mnumspokes;

	// If over distance away, don't move UNLESS this flag is applied
	if (lines[line].flags & ML_EFFECT5)
		mobj->flags2 |= MF2_BOSSNOTRAP;

	// Make the links the same type as the end - repeated below
	if ((mobj->type != MT_CHAINPOINT) && (((lines[line].flags & ML_EFFECT2) == ML_EFFECT2) != (mobj->type == MT_FIREBARPOINT))) // exclusive or
	{
		linktype = macetype;
		radiusfactor = 2; // Double the radius.
	}
	else
		radiusfactor = (((linktype = chainlink) == MT_NULL) ? 2 : 1);

	if (!mchainlike)
		mchainlike = (firsttype == chainlink);
	widthfactor = (mchainlike ? 1 : 2);

	mflagsapply = ((lines[line].flags & ML_EFFECT4) ? 0 : (MF_NOCLIP | MF_NOCLIPHEIGHT));
	mflags2apply = ((mthing->options & MTF_OBJECTFLIP) ? MF2_OBJECTFLIP : 0);
	meflagsapply = ((mthing->options & MTF_OBJECTFLIP) ? MFE_VERTICALFLIP : 0);

	msound = (mchainlike ? 0 : (mwidth & 1));

	// Quick and easy preparatory variable setting
	mphase = (FixedAngle(mphase << FRACBITS) >> ANGLETOFINESHIFT);
	mroll = (FixedAngle(mroll << FRACBITS) >> ANGLETOFINESHIFT);

#define makemace(mobjtype, dist, moreflags2) {\
	spawnee = P_SpawnMobj(mobj->x, mobj->y, mobj->z, mobjtype);\
	P_SetTarget(&spawnee->tracer, mobj);\
	spawnee->threshold = mphase;\
	spawnee->friction = mroll;\
	spawnee->movefactor = mwidthset;\
	spawnee->movecount = dist;\
	spawnee->angle = myaw;\
	spawnee->flags |= (MF_NOGRAVITY|mflagsapply);\
	spawnee->flags2 |= (mflags2apply|moreflags2);\
	spawnee->eflags |= meflagsapply;\
	P_SetTarget(&hprev->hnext, spawnee);\
	P_SetTarget(&spawnee->hprev, hprev);\
	hprev = spawnee;\
}

	mdosound = (mspeed && !(mthing->options & MTF_OBJECTSPECIAL));
	mdocenter = (macetype && (lines[line].flags & ML_EFFECT3));

	// The actual spawning of spokes
	while (mnumspokes-- > 0)
	{
		// Offsets
		if (lines[line].flags & ML_EFFECT1) // Swinging
			mroll = (mroll - mspokeangle) & FINEMASK;
		else // Spinning
			mphase = (mphase - mspokeangle) & FINEMASK;

		if (mnumnospokes && !(mnumspokes % mnumnospokes)) // Skipping a "missing" spoke
		{
			if (mobj->type != MT_CHAINMACEPOINT)
				continue;

			linktype = chainlink;
			firsttype = ((mthing->options & MTF_AMBUSH) ? MT_BIGGRABCHAIN : MT_SMALLGRABCHAIN);
			mmaxlength = 1 + (mlength - 1) * radiusfactor;
			radiusfactor = widthfactor = 1;
		}
		else
		{
			if (mobj->type == MT_CHAINMACEPOINT)
			{
				// Make the links the same type as the end - repeated above
				if (lines[line].flags & ML_EFFECT2)
				{
					linktype = macetype;
					radiusfactor = 2;
				}
				else
					radiusfactor = (((linktype = chainlink) == MT_NULL) ? 2 : 1);

				firsttype = macetype;
				widthfactor = 2;
			}

			mmaxlength = mlength;
		}

		mwidthset = mwidth;
		mlengthset = mminlength;

		if (mdocenter) // Innermost link
			makemace(linktype, 0, 0);

		// Out from the center...
		if (linktype)
		{
			while ((++mlengthset) < mmaxlength)
				makemace(linktype, radiusfactor*mlengthset, 0);
		}
		else
			mlengthset = mmaxlength;

		// Outermost mace/link
		if (firsttype)
			makemace(firsttype, radiusfactor*mlengthset, MF2_AMBUSH);

		if (!mwidth)
		{
			if (mdosound && mnumspokes <= mmin) // Can it make a sound?
				spawnee->flags2 |= MF2_BOSSNOTRAP;
		}
		else
		{
			// Across the bar!
			if (!firsttype)
				mwidthset = -mwidth;
			else if (mwidth > 0)
			{
				while ((mwidthset -= widthfactor) > -mwidth)
				{
					makemace(firsttype, radiusfactor*mlengthset, MF2_AMBUSH);
					if (mdosound && (mwidthset == msound) && mnumspokes <= mmin) // Can it make a sound?
						spawnee->flags2 |= MF2_BOSSNOTRAP;
				}
			}
			else
			{
				while ((mwidthset += widthfactor) < -mwidth)
				{
					makemace(firsttype, radiusfactor*mlengthset, MF2_AMBUSH);
					if (mdosound && (mwidthset == msound) && mnumspokes <= mmin) // Can it make a sound?
						spawnee->flags2 |= MF2_BOSSNOTRAP;
				}
			}
			mwidth = -mwidth;

			// Outermost mace/link again!
			if (firsttype)
				makemace(firsttype, radiusfactor*(mlengthset--), MF2_AMBUSH);

			// ...and then back into the center!
			if (linktype)
				while (mlengthset > mminlength)
					makemace(linktype, radiusfactor*(mlengthset--), 0);

			if (mdocenter) // Innermost link
				makemace(linktype, 0, 0);
		}
	}
#undef makemace
	return true;
}

static boolean P_SetupParticleGen(mapthing_t *mthing, mobj_t *mobj)
{
	fixed_t radius, speed;
	INT32 type, numdivisions, anglespeed, ticcount;
	angle_t angledivision;
	INT32 line;
	const size_t mthingi = (size_t)(mthing - mapthings);

	// Find the corresponding linedef special, using angle as tag
	line = P_FindSpecialLineFromTag(15, mthing->angle, -1);

	if (line == -1)
	{
		CONS_Debug(DBG_GAMELOGIC, "Particle generator (mapthing #%s) needs to be tagged to a #15 parameter line (trying to find tag %d).\n", sizeu1(mthingi), mthing->angle);
		return false;
	}

	if (sides[lines[line].sidenum[0]].toptexture)
		type = sides[lines[line].sidenum[0]].toptexture; // Set as object type in p_setup.c...
	else
		type = (INT32)MT_PARTICLE;

	if (!lines[line].backsector
		|| (ticcount = (sides[lines[line].sidenum[1]].textureoffset >> FRACBITS)) < 1)
		ticcount = 3;

	numdivisions = mthing->z;

	if (numdivisions)
	{
		radius = R_PointToDist2(lines[line].v1->x, lines[line].v1->y, lines[line].v2->x, lines[line].v2->y);
		anglespeed = (sides[lines[line].sidenum[0]].rowoffset >> FRACBITS) % 360;
		angledivision = 360/numdivisions;
	}
	else
	{
		numdivisions = 1; // Simple trick to make A_ParticleSpawn simpler.
		radius = 0;
		anglespeed = 0;
		angledivision = 0;
	}

	speed = abs(sides[lines[line].sidenum[0]].textureoffset);
	if (mthing->options & MTF_OBJECTFLIP)
		speed *= -1;

	CONS_Debug(DBG_GAMELOGIC, "Particle Generator (mapthing #%s):\n"
		"Radius is %d\n"
		"Speed is %d\n"
		"Anglespeed is %d\n"
		"Numdivisions is %d\n"
		"Angledivision is %d\n"
		"Type is %d\n"
		"Tic seperation is %d\n",
		sizeu1(mthingi), radius, speed, anglespeed, numdivisions, angledivision, type, ticcount);

	mobj->angle = 0;
	mobj->movefactor = speed;
	mobj->lastlook = numdivisions;
	mobj->movedir = angledivision*ANG1;
	mobj->movecount = anglespeed*ANG1;
	mobj->friction = radius;
	mobj->threshold = type;
	mobj->reactiontime = ticcount;
	mobj->cvmem = line;
	mobj->watertop = mobj->waterbottom = 0;
	return true;
}

static boolean P_SetupNiGHTSDrone(mapthing_t* mthing, mobj_t* mobj)
{
	boolean flip = mthing->options & MTF_OBJECTFLIP;
	boolean topaligned = (mthing->options & MTF_OBJECTSPECIAL) && !(mthing->options & MTF_EXTRA);
	boolean middlealigned = (mthing->options & MTF_EXTRA) && !(mthing->options & MTF_OBJECTSPECIAL);
	boolean bottomoffsetted = !(mthing->options & MTF_OBJECTSPECIAL) && !(mthing->options & MTF_EXTRA);

	INT16 timelimit = mthing->angle & 0xFFF;
	fixed_t hitboxradius = ((mthing->angle & 0xF000) >> 12)*32*FRACUNIT;
	fixed_t hitboxheight = mthing->extrainfo*32*FRACUNIT;
	fixed_t oldheight = mobj->height;
	fixed_t dronemanoffset, goaloffset, sparkleoffset, droneboxmandiff, dronemangoaldiff;

	if (timelimit > 0)
		mobj->health = timelimit;

	if (hitboxradius > 0)
		mobj->radius = hitboxradius;

	if (hitboxheight > 0)
		mobj->height = hitboxheight;
	else
		mobj->height = mobjinfo[MT_NIGHTSDRONE].height;

	droneboxmandiff = max(mobj->height - mobjinfo[MT_NIGHTSDRONE_MAN].height, 0);
	dronemangoaldiff = max(mobjinfo[MT_NIGHTSDRONE_MAN].height - mobjinfo[MT_NIGHTSDRONE_GOAL].height, 0);

	if (flip && mobj->height != oldheight)
		P_TeleportMove(mobj, mobj->x, mobj->y, mobj->z - (mobj->height - oldheight));

	if (!flip)
	{
		if (topaligned) // Align droneman to top of hitbox
		{
			dronemanoffset = droneboxmandiff;
			goaloffset = dronemangoaldiff/2 + dronemanoffset;
		}
		else if (middlealigned) // Align droneman to center of hitbox
		{
			dronemanoffset = droneboxmandiff/2;
			goaloffset = dronemangoaldiff/2 + dronemanoffset;
		}
		else if (bottomoffsetted)
		{
			dronemanoffset = 24*FRACUNIT;
			goaloffset = dronemangoaldiff + dronemanoffset;
		}
		else
		{
			dronemanoffset = 0;
			goaloffset = dronemangoaldiff/2 + dronemanoffset;
		}

		sparkleoffset = goaloffset - FixedMul(15*FRACUNIT, mobj->scale);
	}
	else
	{
		mobj->eflags |= MFE_VERTICALFLIP;
		mobj->flags2 |= MF2_OBJECTFLIP;

		if (topaligned) // Align droneman to top of hitbox
		{
			dronemanoffset = 0;
			goaloffset = dronemangoaldiff/2 + dronemanoffset;
		}
		else if (middlealigned) // Align droneman to center of hitbox
		{
			dronemanoffset = droneboxmandiff/2;
			goaloffset = dronemangoaldiff/2 + dronemanoffset;
		}
		else if (bottomoffsetted)
		{
			dronemanoffset = droneboxmandiff - FixedMul(24*FRACUNIT, mobj->scale);
			goaloffset = dronemangoaldiff + dronemanoffset;
		}
		else
		{
			dronemanoffset = droneboxmandiff;
			goaloffset = dronemangoaldiff/2 + dronemanoffset;
		}

		sparkleoffset = goaloffset + FixedMul(15*FRACUNIT, mobj->scale);
	}

	// spawn visual elements
	{
		mobj_t* goalpost = P_SpawnMobjFromMobj(mobj, 0, 0, goaloffset, MT_NIGHTSDRONE_GOAL);
		mobj_t* sparkle = P_SpawnMobjFromMobj(mobj, 0, 0, sparkleoffset, MT_NIGHTSDRONE_SPARKLING);
		mobj_t* droneman = P_SpawnMobjFromMobj(mobj, 0, 0, dronemanoffset, MT_NIGHTSDRONE_MAN);

		P_SetTarget(&mobj->target, goalpost);
		P_SetTarget(&goalpost->target, sparkle);
		P_SetTarget(&goalpost->tracer, droneman);

		// correct Z position
		if (flip)
		{
			P_TeleportMove(goalpost, goalpost->x, goalpost->y, mobj->z + goaloffset);
			P_TeleportMove(sparkle, sparkle->x, sparkle->y, mobj->z + sparkleoffset);
			P_TeleportMove(droneman, droneman->x, droneman->y, mobj->z + dronemanoffset);
		}

		// Remember position preference for later
		mobj->flags &= ~(MF_SLIDEME|MF_GRENADEBOUNCE);
		if (topaligned)
			mobj->flags |= MF_SLIDEME;
		else if (middlealigned)
			mobj->flags |= MF_GRENADEBOUNCE;
		else if (!bottomoffsetted)
			mobj->flags |= MF_SLIDEME|MF_GRENADEBOUNCE;

		// Remember old Z position and flags for correction detection
		goalpost->movefactor = mobj->z;
		goalpost->friction = mobj->height;
		goalpost->threshold = mobj->flags & (MF_SLIDEME|MF_GRENADEBOUNCE);
	}
	return true;
}

static boolean P_SetupBooster(mapthing_t* mthing, mobj_t* mobj, boolean strong)
{
	angle_t angle = FixedAngle(mthing->angle << FRACBITS);
	fixed_t x1 = FINECOSINE((angle >> ANGLETOFINESHIFT) & FINEMASK);
	fixed_t y1 = FINESINE((angle >> ANGLETOFINESHIFT) & FINEMASK);
	fixed_t x2 = FINECOSINE(((angle + ANGLE_90) >> ANGLETOFINESHIFT) & FINEMASK);
	fixed_t y2 = FINESINE(((angle + ANGLE_90) >> ANGLETOFINESHIFT) & FINEMASK);
	statenum_t facestate = strong ? S_REDBOOSTERSEG_FACE : S_YELLOWBOOSTERSEG_FACE;
	statenum_t leftstate = strong ? S_REDBOOSTERSEG_LEFT : S_YELLOWBOOSTERSEG_LEFT;
	statenum_t rightstate = strong ? S_REDBOOSTERSEG_RIGHT : S_YELLOWBOOSTERSEG_RIGHT;
	statenum_t rollerstate = strong ? S_REDBOOSTERROLLER : S_YELLOWBOOSTERROLLER;

	mobj_t *seg = P_SpawnMobjFromMobj(mobj, 26*x1, 26*y1, 0, MT_BOOSTERSEG);
	seg->angle = angle - ANGLE_90;
	P_SetMobjState(seg, facestate);
	seg = P_SpawnMobjFromMobj(mobj, -26*x1, -26*y1, 0, MT_BOOSTERSEG);
	seg->angle = angle + ANGLE_90;
	P_SetMobjState(seg, facestate);
	seg = P_SpawnMobjFromMobj(mobj, 21*x2, 21*y2, 0, MT_BOOSTERSEG);
	seg->angle = angle;
	P_SetMobjState(seg, leftstate);
	seg = P_SpawnMobjFromMobj(mobj, -21*x2, -21*y2, 0, MT_BOOSTERSEG);
	seg->angle = angle;
	P_SetMobjState(seg, rightstate);

	seg = P_SpawnMobjFromMobj(mobj, 13*(x1 + x2), 13*(y1 + y2), 0, MT_BOOSTERROLLER);
	seg->angle = angle;
	P_SetMobjState(seg, rollerstate);
	seg = P_SpawnMobjFromMobj(mobj, 13*(x1 - x2), 13*(y1 - y2), 0, MT_BOOSTERROLLER);
	seg->angle = angle;
	P_SetMobjState(seg, rollerstate);
	seg = P_SpawnMobjFromMobj(mobj, -13*(x1 + x2), -13*(y1 + y2), 0, MT_BOOSTERROLLER);
	seg->angle = angle;
	P_SetMobjState(seg, rollerstate);
	seg = P_SpawnMobjFromMobj(mobj, -13*(x1 - x2), -13*(y1 - y2), 0, MT_BOOSTERROLLER);
	seg->angle = angle;
	P_SetMobjState(seg, rollerstate);

	return true;
}

static boolean P_SetupSpawnedMapThing(mapthing_t *mthing, mobj_t *mobj, boolean *doangle)
{
	boolean override = LUAh_MapThingSpawn(mobj, mthing);

	if (P_MobjWasRemoved(mobj))
		return false;

	if (override)
		return true;

	switch (mobj->type)
	{
	case MT_EMBLEM:
	{
		if (!P_SetupEmblem(mthing, mobj))
			return false;
		break;
	}
	case MT_SKYBOX:
		if (mthing->options & MTF_OBJECTSPECIAL)
			skyboxcenterpnts[mthing->extrainfo] = mobj;
		else
			skyboxviewpnts[mthing->extrainfo] = mobj;
		break;
	case MT_EGGSTATUE:
		if (mthing->options & MTF_EXTRA)
		{
			mobj->color = SKINCOLOR_GOLD;
			mobj->colorized = true;
		}
		break;
	case MT_EGGMOBILE3:
		mobj->cusval = mthing->extrainfo;
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
			mobj->health = FixedMul(mobj->subsector->sector->ceilingheight - mobj->subsector->sector->floorheight, 3*(FRACUNIT/4)) >> FRACBITS;
		break;
	case MT_METALSONIC_RACE:
	case MT_METALSONIC_BATTLE:
	case MT_FANG:
	case MT_ROSY:
		if (mthing->options & MTF_EXTRA)
		{
			mobj->color = SKINCOLOR_SILVER;
			mobj->colorized = true;
			mobj->flags2 |= MF2_SLIDEPUSH;
		}
		break;
	case MT_BALLOON:
		if (mthing->angle > 0)
			mobj->color = ((mthing->angle - 1) % (numskincolors - 1)) + 1;
		break;
#define makesoftwarecorona(mo, h) \
			corona = P_SpawnMobjFromMobj(mo, 0, 0, h<<FRACBITS, MT_PARTICLE);\
			corona->sprite = SPR_FLAM;\
			corona->frame = (FF_FULLBRIGHT|FF_TRANS90|12);\
			corona->tics = -1
	case MT_FLAME:
		if (mthing->options & MTF_EXTRA)
		{
			mobj_t *corona;
			makesoftwarecorona(mobj, 20);
			P_SetScale(corona, (corona->destscale = mobj->scale*3));
			P_SetTarget(&mobj->tracer, corona);
		}
		break;
	case MT_FLAMEHOLDER:
		if (!(mthing->options & MTF_OBJECTSPECIAL)) // Spawn the fire
		{
			mobj_t *flame = P_SpawnMobjFromMobj(mobj, 0, 0, mobj->height, MT_FLAME);
			P_SetTarget(&flame->target, mobj);
			flame->flags2 |= MF2_BOSSNOTRAP;
			if (mthing->options & MTF_EXTRA)
			{
				mobj_t *corona;
				makesoftwarecorona(flame, 20);
				P_SetScale(corona, (corona->destscale = flame->scale*3));
				P_SetTarget(&flame->tracer, corona);
			}
		}
		break;
	case MT_CANDLE:
	case MT_CANDLEPRICKET:
		if (mthing->options & MTF_EXTRA)
		{
			mobj_t *corona;
			makesoftwarecorona(mobj, ((mobj->type == MT_CANDLE) ? 42 : 176));
		}
		break;
#undef makesoftwarecorona
	case MT_JACKO1:
	case MT_JACKO2:
	case MT_JACKO3:
		if (!(mthing->options & MTF_EXTRA)) // take the torch out of the crafting recipe
		{
			mobj_t *overlay = P_SpawnMobjFromMobj(mobj, 0, 0, 0, MT_OVERLAY);
			P_SetTarget(&overlay->target, mobj);
			P_SetMobjState(overlay, mobj->info->raisestate);
		}
		break;
	case MT_WATERDRIP:
		mobj->tics = 3*TICRATE + mthing->angle;
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
	case MT_CHAINMACEPOINT:
	case MT_SPRINGBALLPOINT:
	case MT_CHAINPOINT:
	case MT_FIREBARPOINT:
	case MT_CUSTOMMACEPOINT:
		if (!P_SetupMace(mthing, mobj, doangle))
			return false;
		break;
	case MT_PARTICLEGEN:
		if (!P_SetupParticleGen(mthing, mobj))
			return false;
		break;
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
		P_SetMobjState(mobj, mobj->info->spawnstate + mobj->threshold);
		break;
	case MT_EGGCAPSULE:
		if (mthing->angle <= 0)
			mthing->angle = 20; // prevent 0 health

		mobj->health = mthing->angle;
		mobj->threshold = min(mthing->extrainfo, 7);
		break;
	case MT_TUBEWAYPOINT:
	{
		UINT8 sequence = mthing->angle >> 8;
		UINT8 id = mthing->angle & 255;
		mobj->health = id;
		mobj->threshold = sequence;
		P_AddWaypoint(sequence, id, mobj);
		break;
	}
	case MT_IDEYAANCHOR:
		mobj->health = mthing->extrainfo;
		break;
	case MT_NIGHTSDRONE:
		if (!P_SetupNiGHTSDrone(mthing, mobj))
			return false;
		break;
	case MT_HIVEELEMENTAL:
		if (mthing->extrainfo)
			mobj->extravalue1 = mthing->extrainfo;
		break;
	case MT_GLAREGOYLE:
	case MT_GLAREGOYLEUP:
	case MT_GLAREGOYLEDOWN:
	case MT_GLAREGOYLELONG:
		if (mthing->angle >= 360)
			mobj->tics += 7*(mthing->angle/360) + 1; // starting delay
		break;
	case MT_DSZSTALAGMITE:
	case MT_DSZ2STALAGMITE:
	case MT_KELP:
		if (mthing->options & MTF_OBJECTSPECIAL) { // make mobj twice as big as normal
			P_SetScale(mobj, 2*mobj->scale); // not 2*FRACUNIT in case of something like the old ERZ3 mode
			mobj->destscale = mobj->scale;
		}
		break;
	case MT_THZTREE:
	{ // Spawn the branches
		angle_t mobjangle = FixedAngle((mthing->angle % 113) << FRACBITS);
		P_SpawnMobjFromMobj(mobj, FRACUNIT, 0, 0, MT_THZTREEBRANCH)->angle = mobjangle + ANGLE_22h;
		P_SpawnMobjFromMobj(mobj, 0, FRACUNIT, 0, MT_THZTREEBRANCH)->angle = mobjangle + ANGLE_157h;
		P_SpawnMobjFromMobj(mobj, -FRACUNIT, 0, 0, MT_THZTREEBRANCH)->angle = mobjangle + ANGLE_270;
	}
	break;
	case MT_CEZPOLE1:
	case MT_CEZPOLE2:
	{ // Spawn the banner
		angle_t mobjangle = FixedAngle(mthing->angle << FRACBITS);
		P_SpawnMobjFromMobj(mobj,
			P_ReturnThrustX(mobj, mobjangle, 4 << FRACBITS),
			P_ReturnThrustY(mobj, mobjangle, 4 << FRACBITS),
			0, ((mobj->type == MT_CEZPOLE1) ? MT_CEZBANNER1 : MT_CEZBANNER2))->angle = mobjangle + ANGLE_90;
	}
	break;
	case MT_HHZTREE_TOP:
	{ // Spawn the branches
		angle_t mobjangle = FixedAngle(mthing->angle << FRACBITS) & (ANGLE_90 - 1);
		mobj_t* leaf;
#define doleaf(x, y) \
			leaf = P_SpawnMobjFromMobj(mobj, x, y, 0, MT_HHZTREE_PART);\
			leaf->angle = mobjangle;\
			P_SetMobjState(leaf, leaf->info->seestate);\
			mobjangle += ANGLE_90
		doleaf(FRACUNIT, 0);
		doleaf(0, FRACUNIT);
		doleaf(-FRACUNIT, 0);
		doleaf(0, -FRACUNIT);
#undef doleaf
	}
	break;
	case MT_SMASHINGSPIKEBALL:
		if (mthing->angle > 0)
			mobj->tics += mthing->angle;
		break;
	case MT_LAVAFALL:
		mobj->fuse = 30 + mthing->angle;
		if (mthing->options & MTF_AMBUSH)
		{
			P_SetScale(mobj, 2*mobj->scale);
			mobj->destscale = mobj->scale;
		}
		break;
	case MT_PYREFLY:
		//start on fire if Ambush flag is set, otherwise behave normally
		if (mthing->options & MTF_AMBUSH)
		{
			P_SetMobjState(mobj, mobj->info->meleestate);
			mobj->extravalue2 = 2;
			S_StartSound(mobj, sfx_s3kc2l);
		}
		break;
	case MT_BIGFERN:
	{
		angle_t angle = FixedAngle(mthing->angle << FRACBITS);
		UINT8 j;
		for (j = 0; j < 8; j++)
		{
			angle_t fa = (angle >> ANGLETOFINESHIFT) & FINEMASK;
			fixed_t xoffs = FINECOSINE(fa);
			fixed_t yoffs = FINESINE(fa);
			mobj_t* leaf = P_SpawnMobjFromMobj(mobj, xoffs, yoffs, 0, MT_BIGFERNLEAF);
			leaf->angle = angle;
			angle += ANGLE_45;
		}
		break;
	}
	case MT_REDBOOSTER:
	case MT_YELLOWBOOSTER:
		if (!P_SetupBooster(mthing, mobj, mobj->type == MT_REDBOOSTER))
			return false;
		break;
	case MT_AXIS:
		// Inverted if uppermost bit is set
		if (mthing->angle & 16384)
			mobj->flags2 |= MF2_AMBUSH;

		if (mthing->angle > 0)
			mobj->radius = (mthing->angle & 16383) << FRACBITS;
		// FALLTHRU
	case MT_AXISTRANSFER:
	case MT_AXISTRANSFERLINE:
		// Mare it belongs to
		mobj->threshold = min(mthing->extrainfo, 7);

		// # in the mare
		mobj->health = mthing->options;

		mobj->flags2 |= MF2_AXIS;
		break;
	case MT_TOKEN:
		// We advanced tokenbits earlier due to the return check.
		// Subtract 1 here for the correct value.
		mobj->health = 1 << (tokenbits - 1);
		break;
	case MT_CYBRAKDEMON:
		if (mthing->options & MTF_AMBUSH)
		{
			mobj_t* elecmobj;
			elecmobj = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_CYBRAKDEMON_ELECTRIC_BARRIER);
			P_SetTarget(&elecmobj->target, mobj);
			elecmobj->angle = FixedAngle(mthing->angle << FRACBITS);
			elecmobj->destscale = mobj->scale*2;
			P_SetScale(elecmobj, elecmobj->destscale);
		}
		break;
	case MT_STARPOST:
	{
		thinker_t* th;
		mobj_t* mo2;
		boolean foundanother = false;

		if (mthing->extrainfo)
			// Allow thing Parameter to define star post num too!
			// For starposts above param 15 (the 16th), add 360 to the angle like before and start parameter from 1 (NOT 0)!
			// So the 16th starpost is angle=0 param=15, the 17th would be angle=360 param=1.
			// This seems more intuitive for mappers to use until UDMF is ready, since most SP maps won't have over 16 consecutive star posts.
			mobj->health = mthing->extrainfo + (mthing->angle/360)*15 + 1;
		else
			// Old behavior if Parameter is 0; add 360 to the angle for each consecutive star post.
			mobj->health = (mthing->angle/360) + 1;

		// See if other starposts exist in this level that have the same value.
		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo2 = (mobj_t*)th;

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
		break;
	}
	case MT_SPIKE:
		// Pop up spikes!
		if (mthing->options & MTF_OBJECTSPECIAL)
		{
			mobj->flags &= ~MF_SCENERY;
			mobj->fuse = (16 - mthing->extrainfo)*(mthing->angle + mobj->info->speed)/16;
			if (mthing->options & MTF_EXTRA)
				P_SetMobjState(mobj, mobj->info->meleestate);
		}
		// Use per-thing collision for spikes if the deaf flag isn't checked.
		if (!(mthing->options & MTF_AMBUSH) && !metalrecording)
		{
			P_UnsetThingPosition(mobj);
			mobj->flags &= ~(MF_NOBLOCKMAP|MF_NOGRAVITY|MF_NOCLIPHEIGHT);
			mobj->flags |= MF_SOLID;
			P_SetThingPosition(mobj);
		}
		break;
	case MT_WALLSPIKE:
		// Pop up spikes!
		if (mthing->options & MTF_OBJECTSPECIAL)
		{
			mobj->flags &= ~MF_SCENERY;
			mobj->fuse = (16 - mthing->extrainfo)*((mthing->angle/360) + mobj->info->speed)/16;
			if (mthing->options & MTF_EXTRA)
				P_SetMobjState(mobj, mobj->info->meleestate);
		}
		// Use per-thing collision for spikes if the deaf flag isn't checked.
		if (!(mthing->options & MTF_AMBUSH) && !metalrecording)
		{
			P_UnsetThingPosition(mobj);
			mobj->flags &= ~(MF_NOBLOCKMAP | MF_NOCLIPHEIGHT);
			mobj->flags |= MF_SOLID;
			P_SetThingPosition(mobj);
		}

		// spawn base
		{
			const angle_t mobjangle = FixedAngle(mthing->angle << FRACBITS); // the mobj's own angle hasn't been set quite yet so...
			const fixed_t baseradius = mobj->radius - mobj->scale;
			mobj_t* base = P_SpawnMobj(
				mobj->x - P_ReturnThrustX(mobj, mobjangle, baseradius),
				mobj->y - P_ReturnThrustY(mobj, mobjangle, baseradius),
				mobj->z, MT_WALLSPIKEBASE);
			base->angle = mobjangle + ANGLE_90;
			base->destscale = mobj->destscale;
			P_SetScale(base, mobj->scale);
			P_SetTarget(&base->target, mobj);
			P_SetTarget(&mobj->tracer, base);
		}
		break;
	case MT_RING_BOX:
		//count 10 ring boxes into the number of rings equation too.
		if (nummaprings >= 0)
			nummaprings += 10;
		break;
	case MT_BIGTUMBLEWEED:
	case MT_LITTLETUMBLEWEED:
		if (mthing->options & MTF_AMBUSH)
		{
			fixed_t offset = FixedMul(16*FRACUNIT, mobj->scale);
			mobj->momx += P_RandomChance(FRACUNIT/2) ? offset : -offset;
			mobj->momy += P_RandomChance(FRACUNIT/2) ? offset : -offset;
			mobj->momz += offset;
		}
		break;
	case MT_REDFLAG:
		redflag = mobj;
		rflagpoint = mobj->spawnpoint;
		break;
	case MT_BLUEFLAG:
		blueflag = mobj;
		bflagpoint = mobj->spawnpoint;
		break;
	case MT_PUSH:
	case MT_PULL:
		mobj->health = 0; // Default behaviour: pushing uses XY, fading uses XYZ

		if (mthing->options & MTF_AMBUSH)
			mobj->health |= 1; // If ambush is set, push using XYZ
		if (mthing->options & MTF_OBJECTSPECIAL)
			mobj->health |= 2; // If object special is set, fade using XY

		if (G_IsSpecialStage(gamemap))
			P_SetMobjState(mobj, (mobj->type == MT_PUSH) ? S_GRAVWELLGREEN : S_GRAVWELLRED);
		break;
	case MT_NIGHTSSTAR:
		if (maptol & TOL_XMAS)
			P_SetMobjState(mobj, mobj->info->seestate);
		break;
	default:
		break;
	}

	if (mobj->flags & MF_BOSS)
	{
		if (mthing->options & MTF_OBJECTSPECIAL) // No egg trap for this boss
			mobj->flags2 |= MF2_BOSSNOTRAP;
	}

	return true;
}

static void P_SetAmbush(mobj_t *mobj)
{
	if (mobj->type == MT_YELLOWDIAG || mobj->type == MT_REDDIAG || mobj->type == MT_BLUEDIAG)
		mobj->angle += ANGLE_22h;

	if (mobj->flags & MF_NIGHTSITEM)
	{
		// Spawn already displayed
		mobj->flags |= MF_SPECIAL;
		mobj->flags &= ~MF_NIGHTSITEM;
	}

	if (mobj->flags & MF_PUSHABLE)
		mobj->flags &= ~MF_PUSHABLE;

	if ((mobj->flags & MF_MONITOR) && mobj->info->speed != 0)
	{
		// flag for strong/weak random boxes
		// any monitor with nonzero speed is allowed to respawn like this
		mobj->flags2 |= MF2_AMBUSH;
	}

	else if (mobj->type != MT_AXIS &&
		mobj->type != MT_AXISTRANSFER &&
		mobj->type != MT_AXISTRANSFERLINE &&
		mobj->type != MT_NIGHTSBUMPER &&
		mobj->type != MT_STARPOST)
		mobj->flags2 |= MF2_AMBUSH;
}

static void P_SetObjectSpecial(mobj_t *mobj)
{
	if (mobj->type == MT_YELLOWDIAG || mobj->type == MT_REDDIAG || mobj->type == MT_BLUEDIAG)
		mobj->flags |= MF_NOGRAVITY;

	if ((mobj->flags & MF_MONITOR) && mobj->info->speed != 0)
	{
		// flag for strong/weak random boxes
		// any monitor with nonzero speed is allowed to respawn like this
		mobj->flags2 |= MF2_STRONGBOX;
	}

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

static mobj_t *P_SpawnMobjFromMapThing(mapthing_t *mthing, fixed_t x, fixed_t y, fixed_t z, mobjtype_t i)
{
	mobj_t *mobj = NULL;
	boolean doangle = true;

	mobj = P_SpawnMobj(x, y, z, i);
	mobj->spawnpoint = mthing;

	if (!P_SetupSpawnedMapThing(mthing, mobj, &doangle))
		return mobj;

	if (doangle)
		mobj->angle = FixedAngle(mthing->angle << FRACBITS);

	mthing->mobj = mobj;

	// ignore MTF_ flags and return early
	if (i == MT_NIGHTSBUMPER)
		return mobj;

	if ((mthing->options & MTF_AMBUSH)
		&& (mthing->options & MTF_OBJECTSPECIAL)
		&& (mobj->flags & MF_PUSHABLE))
		mobj->flags2 |= MF2_CLASSICPUSH;
	else
	{
		if (mthing->options & MTF_AMBUSH)
			P_SetAmbush(mobj);

		if (mthing->options & MTF_OBJECTSPECIAL)
			P_SetObjectSpecial(mobj);
	}

	// Generic reverse gravity for individual objects flag.
	if (mthing->options & MTF_OBJECTFLIP)
	{
		mobj->eflags |= MFE_VERTICALFLIP;
		mobj->flags2 |= MF2_OBJECTFLIP;
	}

	// Extra functionality
	if (mthing->options & MTF_EXTRA)
	{
		if (mobj->flags & MF_MONITOR && (mthing->angle & 16384))
		{
			// Store line exec tag to run upon popping
			mobj->lastlook = (mthing->angle & 16383);
		}
	}

	// Final set of not being able to draw nightsitems.
	if (mobj->flags & MF_NIGHTSITEM)
		mobj->flags2 |= MF2_DONTDRAW;

	return mobj;
}

//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
mobj_t *P_SpawnMapThing(mapthing_t *mthing)
{
	mobjtype_t i;
	mobj_t *mobj = NULL;
	fixed_t x, y, z;

	if (!mthing->type)
		return mobj; // Ignore type-0 things as NOPs

	if (mthing->type == 3328) // 3D Mode start Thing
		return mobj;

	if (!objectplacing && P_SpawnNonMobjMapThing(mthing))
		return mobj;

	i = P_GetMobjtype(mthing->type);
	if (i == MT_UNKNOWN)
		CONS_Alert(CONS_WARNING, M_GetText("Unknown thing type %d placed at (%d, %d)\n"), mthing->type, mthing->x, mthing->y);

	// Skip all returning/substitution code in objectplace.
	if (!objectplacing)
	{
		if (!P_AllowMobjSpawn(mthing, i))
			return mobj;

		i = P_GetMobjtypeSubstitute(mthing, i);
		if (i == MT_NULL) // Don't spawn mobj
			return mobj;
	}

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;
	z = P_GetMapThingSpawnHeight(i, mthing, x, y);
	return P_SpawnMobjFromMapThing(mthing, x, y, z, i);
}

static void P_SpawnHoopInternal(mapthing_t *mthing, INT32 hoopsize, fixed_t sizefactor)
{
	mobj_t *mobj = NULL;
	mobj_t *nextmobj = NULL;
	mobj_t *hoopcenter;
	TMatrix *pitchmatrix, *yawmatrix;
	fixed_t radius = hoopsize*sizefactor;
	INT32 i;
	angle_t fa;
	TVector v, *res;
	fixed_t x = mthing->x << FRACBITS;
	fixed_t y = mthing->y << FRACBITS;
	fixed_t z = P_GetMobjSpawnHeight(MT_HOOP, x, y, mthing->z << FRACBITS, false);

	hoopcenter = P_SpawnMobj(x, y, z, MT_HOOPCENTER);
	hoopcenter->spawnpoint = mthing;
	hoopcenter->z -= hoopcenter->height/2;

	P_UnsetThingPosition(hoopcenter);
	hoopcenter->x = x;
	hoopcenter->y = y;
	P_SetThingPosition(hoopcenter);

	// Scale 0-255 to 0-359 =(
	hoopcenter->movedir = ((mthing->angle & 255)*360)/256; // Pitch
	pitchmatrix = RotateXMatrix(FixedAngle(hoopcenter->movedir << FRACBITS));
	hoopcenter->movecount = (((UINT16)mthing->angle >> 8)*360)/256; // Yaw
	yawmatrix = RotateZMatrix(FixedAngle(hoopcenter->movecount << FRACBITS));

	// For the hoop when it flies away
	hoopcenter->extravalue1 = hoopsize;
	hoopcenter->extravalue2 = radius/12;

	// Create the hoop!
	for (i = 0; i < hoopsize; i++)
	{
		fa = i*(FINEANGLES/hoopsize);
		v[0] = FixedMul(FINECOSINE(fa), radius);
		v[1] = 0;
		v[2] = FixedMul(FINESINE(fa), radius);
		v[3] = FRACUNIT;

		res = VectorMatrixMultiply(v, *pitchmatrix);
		M_Memcpy(&v, res, sizeof(v));
		res = VectorMatrixMultiply(v, *yawmatrix);
		M_Memcpy(&v, res, sizeof(v));

		mobj = P_SpawnMobj(x + v[0], y + v[1], z + v[2], MT_HOOP);
		mobj->z -= mobj->height/2;

		if (maptol & TOL_XMAS)
			P_SetMobjState(mobj, mobj->info->seestate + (i & 1));

		P_SetTarget(&mobj->target, hoopcenter); // Link the sprite to the center.
		mobj->fuse = 0;

		// Link all the sprites in the hoop together
		if (nextmobj)
		{
			P_SetTarget(&mobj->hprev, nextmobj);
			P_SetTarget(&mobj->hprev->hnext, mobj);
		}
		else
			P_SetTarget(&mobj->hprev, P_SetTarget(&mobj->hnext, NULL));

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

		radius = hoopsize*sizefactor;

		for (i = 0; i < hoopsize; i++)
		{
			fa = i*(FINEANGLES/hoopsize);
			v[0] = FixedMul(FINECOSINE(fa), radius);
			v[1] = 0;
			v[2] = FixedMul(FINESINE(fa), radius);
			v[3] = FRACUNIT;

			res = VectorMatrixMultiply(v, *pitchmatrix);
			M_Memcpy(&v, res, sizeof(v));
			res = VectorMatrixMultiply(v, *yawmatrix);
			M_Memcpy(&v, res, sizeof(v));

			mobj = P_SpawnMobj(x + v[0], y + v[1], z + v[2], MT_HOOPCOLLIDE);
			mobj->z -= mobj->height/2;

			// Link all the collision sprites together.
			P_SetTarget(&mobj->hnext, NULL);
			P_SetTarget(&mobj->hprev, nextmobj);
			P_SetTarget(&mobj->hprev->hnext, mobj);

			nextmobj = mobj;
		}
	} while (hoopsize >= 8);
}

void P_SpawnHoop(mapthing_t *mthing)
{
	if (metalrecording)
		return;

	if (mthing->type == 1705) // Generic hoop
		P_SpawnHoopInternal(mthing, 24, 4*FRACUNIT);
	else // Customizable hoop
		// For each flag add 16 fracunits to the size
		// Default (0 flags) is 32 fracunits
		P_SpawnHoopInternal(mthing, 8 + (4*(mthing->options & 0xF)), 4*FRACUNIT);
}

void P_SetBonusTime(mobj_t *mobj)
{
	if (!mobj)
		return;

	if (mobj->type != MT_BLUESPHERE && mobj->type != MT_NIGHTSCHIP)
		return;

	P_SetMobjState(mobj, mobj->info->raisestate);
}

static void P_SpawnItemRow(mapthing_t *mthing, mobjtype_t* itemtypes, UINT8 numitemtypes, INT32 numitems, fixed_t horizontalspacing, fixed_t verticalspacing, INT16 fixedangle, boolean bonustime)
{
	mapthing_t dummything;
	mobj_t *mobj = NULL;
	fixed_t x = mthing->x << FRACBITS;
	fixed_t y = mthing->y << FRACBITS;
	fixed_t z = mthing->z << FRACBITS;
	INT32 r;
	angle_t angle = FixedAngle(fixedangle << FRACBITS);
	angle_t fineangle = (angle >> ANGLETOFINESHIFT) & FINEMASK;

	for (r = 0; r < numitemtypes; r++)
	{
		dummything = *mthing;
		dummything.type = mobjinfo[itemtypes[r]].doomednum;
		// Skip all returning/substitution code in objectplace.
		if (!objectplacing)
		{
			if (!P_AllowMobjSpawn(&dummything, itemtypes[r]))
			{
				itemtypes[r] = MT_NULL;
				continue;
			}

			itemtypes[r] = P_GetMobjtypeSubstitute(&dummything, itemtypes[r]);
		}
	}
	z = P_GetMobjSpawnHeight(itemtypes[0], x, y, z, mthing->options & MTF_OBJECTFLIP);

	for (r = 0; r < numitems; r++)
	{
		mobjtype_t itemtype = itemtypes[r % numitemtypes];
		if (itemtype == MT_NULL)
			continue;
		dummything.type = mobjinfo[itemtype].doomednum;

		x += FixedMul(horizontalspacing, FINECOSINE(fineangle));
		y += FixedMul(horizontalspacing, FINESINE(fineangle));
		z += (mthing->options & MTF_OBJECTFLIP) ? -verticalspacing : verticalspacing;

		mobj = P_SpawnMobjFromMapThing(&dummything, x, y, z, itemtype);

		if (!mobj)
			continue;

		mobj->spawnpoint = NULL;
		if (bonustime)
			P_SetBonusTime(mobj);
	}
}

static void P_SpawnSingularItemRow(mapthing_t* mthing, mobjtype_t itemtype, INT32 numitems, fixed_t horizontalspacing, fixed_t verticalspacing, INT16 fixedangle, boolean bonustime)
{
	mobjtype_t itemtypes[1] = { itemtype };
	return P_SpawnItemRow(mthing, itemtypes, 1, numitems, horizontalspacing, verticalspacing, fixedangle, bonustime);
}

static void P_SpawnItemCircle(mapthing_t *mthing, mobjtype_t *itemtypes, UINT8 numitemtypes, INT32 numitems, fixed_t size, boolean bonustime)
{
	mapthing_t dummything;
	mobj_t* mobj = NULL;
	fixed_t x = mthing->x << FRACBITS;
	fixed_t y = mthing->y << FRACBITS;
	fixed_t z = mthing->z << FRACBITS;
	angle_t angle = FixedAngle(mthing->angle << FRACBITS);
	angle_t fa;
	INT32 i;
	TVector v, *res;

	for (i = 0; i < numitemtypes; i++)
	{
		dummything = *mthing;
		dummything.type = mobjinfo[itemtypes[i]].doomednum;
		// Skip all returning/substitution code in objectplace.
		if (!objectplacing)
		{
			if (!P_AllowMobjSpawn(&dummything, itemtypes[i]))
			{
				itemtypes[i] = MT_NULL;
				continue;
			}

			itemtypes[i] = P_GetMobjtypeSubstitute(&dummything, itemtypes[i]);
		}
	}
	z = P_GetMobjSpawnHeight(itemtypes[0], x, y, z, false);

	for (i = 0; i < numitems; i++)
	{
		mobjtype_t itemtype = itemtypes[i % numitemtypes];
		if (itemtype == MT_NULL)
			continue;
		dummything.type = mobjinfo[itemtype].doomednum;

		fa = i*FINEANGLES/numitems;
		v[0] = FixedMul(FINECOSINE(fa), size);
		v[1] = 0;
		v[2] = FixedMul(FINESINE(fa), size);
		v[3] = FRACUNIT;

		res = VectorMatrixMultiply(v, *RotateZMatrix(angle));
		M_Memcpy(&v, res, sizeof(v));

		mobj = P_SpawnMobjFromMapThing(&dummything, x + v[0], y + v[1], z + v[2], itemtype);

		if (!mobj)
			continue;

		mobj->z -= mobj->height/2;
		mobj->spawnpoint = NULL;
		if (bonustime)
			P_SetBonusTime(mobj);
	}
}

void P_SpawnItemPattern(mapthing_t *mthing, boolean bonustime)
{
	switch (mthing->type)
	{
	// Special placement patterns
	case 600: // 5 vertical rings (yellow spring)
		P_SpawnSingularItemRow(mthing, MT_RING, 5, 0, 64*FRACUNIT, 0, bonustime);
		return;
	case 601: // 5 vertical rings (red spring)
		P_SpawnSingularItemRow(mthing, MT_RING, 5, 0, 128*FRACUNIT, 0, bonustime);
		return;
	case 602: // 5 diagonal rings (yellow spring)
		P_SpawnSingularItemRow(mthing, MT_RING, 5, 64*FRACUNIT, 64*FRACUNIT, mthing->angle, bonustime);
		return;
	case 603: // 10 diagonal rings (red spring)
		P_SpawnSingularItemRow(mthing, MT_RING, 10, 64*FRACUNIT, 64*FRACUNIT, mthing->angle, bonustime);
		return;
	case 604: // Circle of rings (8 items)
	case 605: // Circle of rings (16 items)
	case 606: // Circle of blue spheres (8 items)
	case 607: // Circle of blue spheres (16 items)
	{
		INT32 numitems = (mthing->type & 1) ? 16 : 8;
		fixed_t size = (mthing->type & 1) ? 192*FRACUNIT : 96*FRACUNIT;
		mobjtype_t itemtypes[1] = { (mthing->type < 606) ? MT_RING : MT_BLUESPHERE };
		P_SpawnItemCircle(mthing, itemtypes, 1, numitems, size, bonustime);
		return;
	}
	case 608: // Circle of rings and blue spheres (8 items)
	case 609: // Circle of rings and blue spheres (16 items)
	{
		INT32 numitems = (mthing->type & 1) ? 16 : 8;
		fixed_t size = (mthing->type & 1) ? 192*FRACUNIT : 96*FRACUNIT;
		mobjtype_t itemtypes[2] = { MT_RING, MT_BLUESPHERE };
		P_SpawnItemCircle(mthing, itemtypes, 2, numitems, size, bonustime);
		return;
	}
	default:
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
	an = R_PointToAngle2(0, 0, source->momx, source->momy) + (ANG1*shiftingAngle);

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
	fixed_t x, y, z, slope = 0, speed;

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

	speed = th->info->speed;
	if (source->player && source->player->charability == CA_FLY)
		speed = FixedMul(speed, 3*FRACUNIT/2);

	th->angle = an;
	th->momx = FixedMul(speed, FINECOSINE(an>>ANGLETOFINESHIFT));
	th->momy = FixedMul(speed, FINESINE(an>>ANGLETOFINESHIFT));

	if (allowaim)
	{
		th->momx = FixedMul(th->momx,FINECOSINE(source->player->aiming>>ANGLETOFINESHIFT));
		th->momy = FixedMul(th->momy,FINECOSINE(source->player->aiming>>ANGLETOFINESHIFT));
	}

	th->momz = FixedMul(speed, slope);

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

//
// P_SpawnMobjFromMobj
// Spawns an object with offsets relative to the position of another object.
// Scale, gravity flip, etc. is taken into account automatically.
//
mobj_t *P_SpawnMobjFromMobj(mobj_t *mobj, fixed_t xofs, fixed_t yofs, fixed_t zofs, mobjtype_t type)
{
	mobj_t *newmobj;

	xofs = FixedMul(xofs, mobj->scale);
	yofs = FixedMul(yofs, mobj->scale);
	zofs = FixedMul(zofs, mobj->scale);

	newmobj = P_SpawnMobj(mobj->x + xofs, mobj->y + yofs, mobj->z + zofs, type);
	if (!newmobj)
		return NULL;

	if (mobj->eflags & MFE_VERTICALFLIP)
	{
		fixed_t elementheight = FixedMul(newmobj->info->height, mobj->scale);

		newmobj->eflags |= MFE_VERTICALFLIP;
		newmobj->flags2 |= MF2_OBJECTFLIP;
		newmobj->z = mobj->z + mobj->height - zofs - elementheight;
	}

	newmobj->destscale = mobj->destscale;
	P_SetScale(newmobj, mobj->scale);
	return newmobj;
}
