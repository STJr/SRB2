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
/// \file  p_tick.c
/// \brief Archiving: SaveGame I/O, Thinker, Ticker

#include "doomstat.h"
#include "g_game.h"
#include "p_local.h"
#include "z_zone.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "p_polyobj.h"
#include "m_random.h"
#include "lua_script.h"
#include "lua_hook.h"

// Object place
#include "m_cheat.h"

tic_t leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Calloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

// The entries will behave like both the head and tail of the lists.
thinker_t thlist[NUM_THINKERLISTS];

void Command_Numthinkers_f(void)
{
	INT32 num;
	INT32 count = 0;
	actionf_p1 action;
	thinker_t *think;
	thinklistnum_t start = 0;
	thinklistnum_t end = NUM_THINKERLISTS - 1;
	thinklistnum_t i;

	if (gamestate != GS_LEVEL)
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("numthinkers <#>: Count number of thinkers\n"));
		CONS_Printf(
			"\t1: P_MobjThinker\n"
			/*"\t2: P_RainThinker\n"
			"\t3: P_SnowThinker\n"*/
			"\t2: P_NullPrecipThinker\n"
			"\t3: T_Friction\n"
			"\t4: T_Pusher\n"
			"\t5: P_RemoveThinkerDelayed\n");
		return;
	}

	num = atoi(COM_Argv(1));

	switch (num)
	{
		case 1:
			start = end = THINK_MOBJ;
			action = (actionf_p1)P_MobjThinker;
			CONS_Printf(M_GetText("Number of %s: "), "P_MobjThinker");
			break;
		/*case 2:
			action = (actionf_p1)P_RainThinker;
			CONS_Printf(M_GetText("Number of %s: "), "P_RainThinker");
			break;
		case 3:
			action = (actionf_p1)P_SnowThinker;
			CONS_Printf(M_GetText("Number of %s: "), "P_SnowThinker");
			break;*/
		case 2:
			start = end = THINK_PRECIP;
			action = (actionf_p1)P_NullPrecipThinker;
			CONS_Printf(M_GetText("Number of %s: "), "P_NullPrecipThinker");
			break;
		case 3:
			start = end = THINK_MAIN;
			action = (actionf_p1)T_Friction;
			CONS_Printf(M_GetText("Number of %s: "), "T_Friction");
			break;
		case 4:
			start = end = THINK_MAIN;
			action = (actionf_p1)T_Pusher;
			CONS_Printf(M_GetText("Number of %s: "), "T_Pusher");
			break;
		case 5:
			action = (actionf_p1)P_RemoveThinkerDelayed;
			CONS_Printf(M_GetText("Number of %s: "), "P_RemoveThinkerDelayed");
			break;
		default:
			CONS_Printf(M_GetText("That is not a valid number.\n"));
			return;
	}

	for (i = start; i <= end; i++)
	{
		for (think = thlist[i].next; think != &thlist[i]; think = think->next)
		{
			if (think->function.acp1 != action)
				continue;

			count++;
		}
	}

	CONS_Printf("%d\n", count);
}

void Command_CountMobjs_f(void)
{
	thinker_t *th;
	mobjtype_t i;
	INT32 count;

	if (gamestate != GS_LEVEL)
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (COM_Argc() >= 2)
	{
		size_t j;
		for (j = 1; j < COM_Argc(); j++)
		{
			i = atoi(COM_Argv(j));
			if (i >= NUMMOBJTYPES)
			{
				CONS_Printf(M_GetText("Object number %d out of range (max %d).\n"), i, NUMMOBJTYPES-1);
				continue;
			}

			count = 0;

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				if (((mobj_t *)th)->type == i)
					count++;
			}

			CONS_Printf(M_GetText("There are %d objects of type %d currently in the level.\n"), count, i);
		}
		return;
	}

	CONS_Printf(M_GetText("Count of active objects in level:\n"));

	for (i = 0; i < NUMMOBJTYPES; i++)
	{
		count = 0;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			if (((mobj_t *)th)->type == i)
				count++;
		}

		if (count > 0) // Don't bother displaying if there are none of this type!
			CONS_Printf(" * %d: %d\n", i, count);
	}
}

//
// P_InitThinkers
//
void P_InitThinkers(void)
{
	UINT8 i;
	for (i = 0; i < NUM_THINKERLISTS; i++)
		thlist[i].prev = thlist[i].next = &thlist[i];
}

// Adds a new thinker at the end of the list.
void P_AddThinker(const thinklistnum_t n, thinker_t *thinker)
{
#ifdef PARANOIA
	I_Assert(n < NUM_THINKERLISTS);
#endif

	thlist[n].prev->next = thinker;
	thinker->next = &thlist[n];
	thinker->prev = thlist[n].prev;
	thlist[n].prev = thinker;

	thinker->references = 0;    // killough 11/98: init reference counter to 0
}

//
// killough 11/98:
//
// Make currentthinker external, so that P_RemoveThinkerDelayed
// can adjust currentthinker when thinkers self-remove.

static thinker_t *currentthinker;

//
// P_RemoveThinkerDelayed()
//
// Called automatically as part of the thinker loop in P_RunThinkers(),
// on nodes which are pending deletion.
//
// If this thinker has no more pointers referencing it indirectly,
// remove it, and set currentthinker to one node preceeding it, so
// that the next step in P_RunThinkers() will get its successor.
//
void P_RemoveThinkerDelayed(thinker_t *thinker)
{
	thinker_t *next;
#ifdef PARANOIA
#define BEENAROUNDBIT (0x40000000) // has to be sufficiently high that it's unlikely to happen in regular gameplay. If you change this, pay attention to the bit pattern of INT32_MIN.
	if (thinker->references & ~BEENAROUNDBIT)
	{
		if (thinker->references & BEENAROUNDBIT) // Usually gets cleared up in one frame; what's going on here, then?
			CONS_Printf("Number of potentially faulty references: %d\n", (thinker->references & ~BEENAROUNDBIT));
		thinker->references |= BEENAROUNDBIT;
		return;
	}
#undef BEENAROUNDBIT
#else
	if (thinker->references)
		return;
#endif

	/* Remove from main thinker list */
	next = thinker->next;
	/* Note that currentthinker is guaranteed to point to us,
	* and since we're freeing our memory, we had better change that. So
	* point it to thinker->prev, so the iterator will correctly move on to
	* thinker->prev->next = thinker->next */
	(next->prev = currentthinker = thinker->prev)->next = next;

	Z_Free(thinker);
}

//
// P_RemoveThinker
//
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// killough 4/25/98:
//
// Instead of marking the function with -1 value cast to a function pointer,
// set the function to P_RemoveThinkerDelayed(), so that later, it will be
// removed automatically as part of the thinker process.
//
void P_RemoveThinker(thinker_t *thinker)
{
	LUA_InvalidateUserdata(thinker);
	thinker->function.acp1 = (actionf_p1)P_RemoveThinkerDelayed;
}

/*
 * P_SetTarget
 *
 * This function is used to keep track of pointer references to mobj thinkers.
 * In Doom, objects such as lost souls could sometimes be removed despite
 * their still being referenced. In Boom, 'target' mobj fields were tested
 * during each gametic, and any objects pointed to by them would be prevented
 * from being removed. But this was incomplete, and was slow (every mobj was
 * checked during every gametic). Now, we keep a count of the number of
 * references, and delay removal until the count is 0.
 */

mobj_t *P_SetTarget(mobj_t **mop, mobj_t *targ)
{
	if (*mop)              // If there was a target already, decrease its refcount
		(*mop)->thinker.references--;
if ((*mop = targ) != NULL) // Set new target and if non-NULL, increase its counter
		targ->thinker.references++;
	return targ;
}

//
// P_RunThinkers
//
// killough 4/25/98:
//
// Fix deallocator to stop using "next" pointer after node has been freed
// (a Doom bug).
//
// Process each thinker. For thinkers which are marked deleted, we must
// load the "next" pointer prior to freeing the node. In Doom, the "next"
// pointer was loaded AFTER the thinker was freed, which could have caused
// crashes.
//
// But if we are not deleting the thinker, we should reload the "next"
// pointer after calling the function, in case additional thinkers are
// added at the end of the list.
//
// killough 11/98:
//
// Rewritten to delete nodes implicitly, by making currentthinker
// external and using P_RemoveThinkerDelayed() implicitly.
//
static inline void P_RunThinkers(void)
{
	size_t i;
	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (currentthinker = thlist[i].next; currentthinker != &thlist[i]; currentthinker = currentthinker->next)
		{
#ifdef PARANOIA
			I_Assert(currentthinker->function.acp1 != NULL);
#endif
			currentthinker->function.acp1(currentthinker);
		}
	}

}

//
// P_DoAutobalanceTeams()
//
// Determine if the teams are unbalanced, and if so, move a player to the other team.
//
static void P_DoAutobalanceTeams(void)
{
	changeteam_union NetPacket;
	UINT16 usvalue;
	INT32 i=0;
	INT32 red=0, blue=0;
	INT32 redarray[MAXPLAYERS], bluearray[MAXPLAYERS];
	INT32 redflagcarrier = 0, blueflagcarrier = 0;
	INT32 totalred = 0, totalblue = 0;

	NetPacket.value.l = NetPacket.value.b = 0;
	memset(redarray, 0, sizeof(redarray));
	memset(bluearray, 0, sizeof(bluearray));

	// Only do it if we have enough room in the net buffer to send it.
	// Otherwise, come back next time and try again.
	if (sizeof(usvalue) > GetFreeXCmdSize())
		return;

	//We have to store the players in an array with the rest of their team.
	//We can then pick a random player to be forced to change teams.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && players[i].ctfteam)
		{
			if (players[i].ctfteam == 1)
			{
				if (!players[i].gotflag)
				{
					redarray[red] = i; //store the player's node.
					red++;
				}
				else
					redflagcarrier++;
			}
			else
			{
				if (!players[i].gotflag)
				{
					bluearray[blue] = i; //store the player's node.
					blue++;
				}
				else
					blueflagcarrier++;
			}
		}
	}

	totalred = red + redflagcarrier;
	totalblue = blue + blueflagcarrier;

	if ((abs(totalred - totalblue) > max(1, (totalred + totalblue) / 8)))
	{
		if (totalred > totalblue)
		{
			i = M_RandomKey(red);
			NetPacket.packet.newteam = 2;
			NetPacket.packet.playernum = redarray[i];
			NetPacket.packet.verification = true;
			NetPacket.packet.autobalance = true;

			usvalue  = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}
		else //if (totalblue > totalred)
		{
			i = M_RandomKey(blue);
			NetPacket.packet.newteam = 1;
			NetPacket.packet.playernum = bluearray[i];
			NetPacket.packet.verification = true;
			NetPacket.packet.autobalance = true;

			usvalue  = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}
	}
}

//
// P_DoTeamscrambling()
//
// If a team scramble has been started, scramble one person from the
// pre-made scramble array. Said array is created in TeamScramble_OnChange()
//
void P_DoTeamscrambling(void)
{
	changeteam_union NetPacket;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	// Only do it if we have enough room in the net buffer to send it.
	// Otherwise, come back next time and try again.
	if (sizeof(usvalue) > GetFreeXCmdSize())
		return;

	if (scramblecount < scrambletotal)
	{
		if (players[scrambleplayers[scramblecount]].ctfteam != scrambleteams[scramblecount])
		{
			NetPacket.packet.newteam = scrambleteams[scramblecount];
			NetPacket.packet.playernum = scrambleplayers[scramblecount];
			NetPacket.packet.verification = true;
			NetPacket.packet.scrambled = true;

			usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
			SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
		}

		scramblecount++; //Increment, and get to the next player when we come back here next time.
	}
	else
		CV_SetValue(&cv_teamscramble, 0);
}

static inline void P_DoSpecialStageStuff(void)
{
	boolean stillalive = false;
	INT32 i;

	// Can't drown in a special stage
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i] || players[i].spectator)
			continue;

		players[i].powers[pw_underwater] = players[i].powers[pw_spacetime] = 0;
	}

	//if (sstimer < 15*TICRATE+6 && sstimer > 7 && (mapheaderinfo[gamemap-1]->levelflags & LF_SPEEDMUSIC))
		//S_SpeedMusic(1.4f);

	if (sstimer && !objectplacing)
	{
		UINT16 countspheres = 0;
		// Count up the rings of all the players and see if
		// they've collected the required amount.
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				tic_t oldnightstime = players[i].nightstime;
				countspheres += players[i].spheres;

				if (!oldnightstime)
					continue;

				// If in water, deplete timer 6x as fast.
				if (players[i].mo->eflags & (MFE_TOUCHWATER|MFE_UNDERWATER) && !(players[i].powers[pw_shield] & SH_PROTECTWATER))
					players[i].nightstime -= 5;
				if (--players[i].nightstime > 6)
				{
					if (P_IsLocalPlayer(&players[i]) && oldnightstime > 10*TICRATE && players[i].nightstime <= 10*TICRATE)
						S_ChangeMusicInternal("_drown", false);
					stillalive = true;
				}
				else if (!players[i].exiting)
				{
					players[i].exiting = (14*TICRATE)/5 + 1;
					players[i].pflags &= ~(PF_GLIDING|PF_BOUNCING);
					players[i].nightstime = 0;
					if (P_IsLocalPlayer(&players[i]))
						S_StartSound(NULL, sfx_s3k66);
				}
			}

		if (stillalive)
		{
			if (countspheres >= ssspheres)
			{
				// Halt all the players
				for (i = 0; i < MAXPLAYERS; i++)
					if (playeringame[i] && !players[i].exiting)
					{
						players[i].mo->momx = players[i].mo->momy = 0;
						players[i].exiting = (14*TICRATE)/5 + 1;
					}
				sstimer = 0;
				P_GiveEmerald(true);
				P_RestoreMusic(&players[consoleplayer]);
			}
		}
		else
			sstimer = 0;
	}
}

static inline void P_DoTagStuff(void)
{
	INT32 i;

	// tell the netgame who the initial IT person is.
	if (leveltime == TICRATE)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (players[i].pflags & PF_TAGIT)
			{
				CONS_Printf(M_GetText("%s is now IT!\n"), player_names[i]); // Tell everyone who is it!
				break;
			}
		}
	}

	//increment survivor scores
	if (leveltime % TICRATE == 0 && leveltime > (hidetime * TICRATE))
	{
		INT32 participants = 0;

		for (i=0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator)
				participants++;
		}

		for (i=0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && !players[i].spectator && players[i].playerstate == PST_LIVE
			&& !(players[i].pflags & (PF_TAGIT|PF_GAMETYPEOVER)))
				//points given is the number of participating players divided by two.
				P_AddPlayerScore(&players[i], participants/2);
		}
	}
}

static inline void P_DoCTFStuff(void)
{
	// Automatic team balance for CTF and team match
	if (leveltime % (TICRATE * 5) == 0) //only check once per five seconds for the sake of CPU conservation.
	{
		// Do not attempt to autobalance and scramble teams at the same time.
		// Only the server should execute this. No verified admins, please.
		if ((cv_autobalance.value && !cv_teamscramble.value) && cv_allowteamchange.value && server)
			P_DoAutobalanceTeams();
	}

	// Team scramble code for team match and CTF.
	if ((leveltime % (TICRATE/7)) == 0)
	{
		// If we run out of time in the level, the beauty is that
		// the Y_Ticker() team scramble code will pick it up.
		if (cv_teamscramble.value && server)
			P_DoTeamscrambling();
	}
}

//
// P_Ticker
//
void P_Ticker(boolean run)
{
	INT32 i;

	// Increment jointime and quittime even if paused
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
		{
			players[i].jointime++;

			if (players[i].quittime)
			{
				players[i].quittime++;

				if (players[i].quittime == 30 * TICRATE && G_TagGametype())
					P_CheckSurvivors();

				if (server && players[i].quittime >= (tic_t)FixedMul(cv_rejointimeout.value, 60 * TICRATE)
				&& !(players[i].quittime % TICRATE))
					SendKick(i, KICK_MSG_PLAYER_QUIT);
			}
		}

	if (objectplacing)
	{
		if (OP_FreezeObjectplace())
		{
			P_MapStart();
			OP_ObjectplaceMovement(&players[0]);
			P_MoveChaseCamera(&players[0], &camera, false);
			P_MapEnd();
			S_SetStackAdjustmentStart();
			return;
		}
	}

	// Check for pause or menu up in single player
	if (paused || P_AutoPause())
	{
		S_SetStackAdjustmentStart();
		return;
	}

	if (!S_MusicPaused())
		S_AdjustMusicStackTics();

	postimgtype = postimgtype2 = postimg_none;

	P_MapStart();

	if (run)
	{
		if (demorecording)
			G_WriteDemoTiccmd(&players[consoleplayer].cmd, 0);
		if (demoplayback)
			G_ReadDemoTiccmd(&players[consoleplayer].cmd, 0);

		LUAh_PreThinkFrame();

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
				P_PlayerThink(&players[i]);
	}

	// Keep track of how long they've been playing!
	if (!demoplayback) // Don't increment if a demo is playing.
		totalplaytime++;

	if (!(maptol & TOL_NIGHTS) && G_IsSpecialStage(gamemap))
		P_DoSpecialStageStuff();

	if (runemeraldmanager)
		P_EmeraldManager(); // Power stone mode

	if (run)
	{
		P_RunThinkers();

		// Run any "after all the other thinkers" stuff
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
				P_PlayerAfterThink(&players[i]);

		LUAh_ThinkFrame();
	}

	// Run shield positioning
	P_RunShields();
	P_RunOverlays();

	P_UpdateSpecials();
	P_RespawnSpecials();

	// Lightning, rain sounds, etc.
	P_PrecipitationEffects();

	if (run)
		leveltime++;
	timeinmap++;

	if (G_TagGametype())
		P_DoTagStuff();

	if (G_GametypeHasTeams())
		P_DoCTFStuff();

	if (run)
	{
		if (countdowntimer && G_PlatformGametype() && (gametype == GT_COOP || leveltime >= 4*TICRATE) && !stoppedclock && --countdowntimer <= 0)
		{
			countdowntimer = 0;
			countdowntimeup = true;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;

				if (!players[i].mo)
					continue;

				if (multiplayer || netgame)
					players[i].exiting = 0;
				P_DamageMobj(players[i].mo, NULL, NULL, 1, DMG_INSTAKILL);
			}
		}

		if (countdown > 1)
			countdown--;

		if (countdown2)
			countdown2--;

		if (quake.time)
		{
			fixed_t ir = quake.intensity>>1;
			/// \todo Calculate distance from epicenter if set and modulate the intensity accordingly based on radius.
			quake.x = M_RandomRange(-ir,ir);
			quake.y = M_RandomRange(-ir,ir);
			quake.z = M_RandomRange(-ir,ir);
			--quake.time;
		}
		else
			quake.x = quake.y = quake.z = 0;

		if (metalplayback)
			G_ReadMetalTic(metalplayback);
		if (metalrecording)
			G_WriteMetalTic(players[consoleplayer].mo);
		if (demorecording)
			G_WriteGhostTic(players[consoleplayer].mo);
		if (demoplayback) // Use Ghost data for consistency checks.
			G_ConsGhostTic();
		if (modeattacking)
			G_GhostTicker();

		LUAh_PostThinkFrame();
	}

	P_MapEnd();

//	Z_CheckMemCleanup();
}

// Abbreviated ticker for pre-loading, calls thinkers and assorted things
void P_PreTicker(INT32 frames)
{
	INT32 i,framecnt;
	ticcmd_t temptic;

	postimgtype = postimgtype2 = postimg_none;

	for (framecnt = 0; framecnt < frames; ++framecnt)
	{
		P_MapStart();

		LUAh_PreThinkFrame();

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
			{
				// stupid fucking cmd hack
				// if it isn't for this, players can move in preticker time
				// (and disrupt demo recording and other things !!)
				memcpy(&temptic, &players[i].cmd, sizeof(ticcmd_t));
				memset(&players[i].cmd, 0, sizeof(ticcmd_t));
				// correct angle on spawn...
				players[i].cmd.angleturn = temptic.angleturn;

				P_PlayerThink(&players[i]);

				memcpy(&players[i].cmd, &temptic, sizeof(ticcmd_t));
			}

		P_RunThinkers();

		// Run any "after all the other thinkers" stuff
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && !P_MobjWasRemoved(players[i].mo))
				P_PlayerAfterThink(&players[i]);

		LUAh_ThinkFrame();

		// Run shield positioning
		P_RunShields();
		P_RunOverlays();

		P_UpdateSpecials();
		P_RespawnSpecials();

		LUAh_PostThinkFrame();

		P_MapEnd();
	}
}
