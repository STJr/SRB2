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
/// \file  m_cheat.c
/// \brief Cheat sequence checking

#include "doomdef.h"
#include "g_input.h"
#include "g_game.h"
#include "s_sound.h"

#include "r_local.h"
#include "p_local.h"
#include "p_setup.h"
#include "d_net.h"

#include "m_cheat.h"
#include "m_menu.h"
#include "m_random.h"
#include "m_misc.h"

#include "hu_stuff.h"
#include "m_cond.h" // secrets unlocked?

#include "v_video.h"
#include "z_zone.h"
#include "p_slopes.h"

#include "lua_script.h"
#include "lua_hook.h"

//
// CHEAT SEQUENCE PACKAGE
//

#define SCRAMBLE(a) \
((((a)&1)<<7) + (((a)&2)<<5) + ((a)&4) + (((a)&8)<<1) \
 + (((a)&16)>>1) + ((a)&32) + (((a)&64)>>5) + (((a)&128)>>7))

typedef struct
{
	UINT8 *p;
	UINT8 (*func)(void); // called when cheat confirmed.
	UINT8 sequence[];
} cheatseq_t;

// ==========================================================================
//                             CHEAT Structures
// ==========================================================================

// Cheat responders
static UINT8 cheatf_ultimate(void)
{
	if (menuactive && (currentMenu != &MainDef && currentMenu != &SP_LoadDef))
		return 0; // Only on the main menu, or the save select!

	BwehHehHe();
	ultimate_selectable = (!ultimate_selectable);

	// If on the save select, move to what is now Ultimate Mode!
	if (currentMenu == &SP_LoadDef)
		M_ForceSaveSlotSelected(NOSAVESLOT);
	return 1;
}

static UINT8 cheatf_warp(void)
{
	if (modifiedgame)
		return 0;

	if (menuactive && currentMenu != &MainDef)
		return 0; // Only on the main menu!

	S_StartSound(0, sfx_itemup);

	// Temporarily unlock stuff.
	G_SetGameModified(false);
	unlockables[31].unlocked = true; // credits
	unlockables[30].unlocked = true; // sound test
	unlockables[28].unlocked = true; // level select

	// Refresh secrets menu existing.
	M_ClearMenus(true);
	M_StartControlPanel();
	return 1;
}

#ifdef DEVELOP
static UINT8 cheatf_devmode(void)
{
	UINT8 i;

	if (modifiedgame)
		return 0;

	if (menuactive && currentMenu != &MainDef)
		return 0; // Only on the main menu!

	S_StartSound(0, sfx_itemup);

	// Just unlock all the things and turn on -debug and console devmode.
	G_SetGameModified(false);
	for (i = 0; i < MAXUNLOCKABLES; i++)
		unlockables[i].unlocked = true;
	devparm = true;
	cv_debug |= 0x8000;

	// Refresh secrets menu existing.
	M_ClearMenus(true);
	M_StartControlPanel();
	return 1;
}
#endif

static cheatseq_t cheat_ultimate = {
	0, cheatf_ultimate,
	{ SCRAMBLE('u'), SCRAMBLE('l'), SCRAMBLE('t'), SCRAMBLE('i'), SCRAMBLE('m'), SCRAMBLE('a'), SCRAMBLE('t'), SCRAMBLE('e'), 0xff }
};

static cheatseq_t cheat_ultimate_joy = {
	0, cheatf_ultimate,
	{ SCRAMBLE(KEY_UPARROW), SCRAMBLE(KEY_UPARROW), SCRAMBLE(KEY_DOWNARROW), SCRAMBLE(KEY_DOWNARROW),
	  SCRAMBLE(KEY_LEFTARROW), SCRAMBLE(KEY_RIGHTARROW), SCRAMBLE(KEY_LEFTARROW), SCRAMBLE(KEY_RIGHTARROW),
	  SCRAMBLE(KEY_ENTER), 0xff }
};

static cheatseq_t cheat_warp = {
	0, cheatf_warp,
	{ SCRAMBLE('c'), SCRAMBLE('a'), SCRAMBLE('s'), SCRAMBLE('h'), SCRAMBLE('r'), SCRAMBLE('i'), SCRAMBLE('d'), SCRAMBLE('a'), 0xff }
};

static cheatseq_t cheat_warp_joy = {
	0, cheatf_warp,
	{ SCRAMBLE(KEY_RIGHTARROW), SCRAMBLE(KEY_RIGHTARROW), SCRAMBLE(KEY_DOWNARROW),
	  SCRAMBLE(KEY_LEFTARROW), SCRAMBLE(KEY_LEFTARROW), SCRAMBLE(KEY_DOWNARROW),
	  SCRAMBLE(KEY_RIGHTARROW), SCRAMBLE(KEY_DOWNARROW),
	  SCRAMBLE(KEY_ENTER), 0xff }
};

#ifdef DEVELOP
static cheatseq_t cheat_devmode = {
	0, cheatf_devmode,
	{ SCRAMBLE('d'), SCRAMBLE('e'), SCRAMBLE('v'), SCRAMBLE('m'), SCRAMBLE('o'), SCRAMBLE('d'), SCRAMBLE('e'), 0xff }
};
#endif

// ==========================================================================
//                        CHEAT SEQUENCE PACKAGE
// ==========================================================================

static UINT8 cheat_xlate_table[256];

void cht_Init(void)
{
	size_t i = 0;
	INT16 pi = 0;
	for (; i < 256; i++, pi++)
	{
		const INT32 cc = SCRAMBLE(pi);
		cheat_xlate_table[i] = (UINT8)cc;
	}
}

//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
static UINT8 cht_CheckCheat(cheatseq_t *cht, char key)
{
	UINT8 rc = 0;

	if (!cht->p)
		cht->p = cht->sequence; // initialize if first time

	if (*cht->p == 0)
		*(cht->p++) = key;
	else if (cheat_xlate_table[(UINT8)key] == *cht->p)
		cht->p++;
	else
		cht->p = cht->sequence;

	if (*cht->p == 1)
		cht->p++;
	else if (*cht->p == 0xff) // end of sequence character
	{
		cht->p = cht->sequence;
		rc = cht->func();
	}

	return rc;
}

boolean cht_Responder(event_t *ev)
{
	UINT8 ret = 0, ch = 0;
	if (ev->type != ev_keydown)
		return false;

	if (ev->data1 > 0xFF)
	{
		// map some fake (joy) inputs into keys
		// map joy inputs into keys
		switch (ev->data1)
		{
			case KEY_JOY1:
			case KEY_JOY1 + 2:
				ch = KEY_ENTER;
				break;
			case KEY_HAT1:
				ch = KEY_UPARROW;
				break;
			case KEY_HAT1 + 1:
				ch = KEY_DOWNARROW;
				break;
			case KEY_HAT1 + 2:
				ch = KEY_LEFTARROW;
				break;
			case KEY_HAT1 + 3:
				ch = KEY_RIGHTARROW;
				break;
			default:
				// no mapping
				return false;
		}
	}
	else
		ch = (UINT8)ev->data1;

	ret += cht_CheckCheat(&cheat_ultimate, (char)ch);
	ret += cht_CheckCheat(&cheat_ultimate_joy, (char)ch);
	ret += cht_CheckCheat(&cheat_warp, (char)ch);
	ret += cht_CheckCheat(&cheat_warp_joy, (char)ch);
#ifdef DEVELOP
	ret += cht_CheckCheat(&cheat_devmode, (char)ch);
#endif
	return (ret != 0);
}

// Console cheat commands rely on these a lot...
#define REQUIRE_PANDORA if (!M_SecretUnlocked(SECRET_PANDORA) && !cv_debug)\
{ CONS_Printf(M_GetText("You haven't earned this yet.\n")); return; }

#define REQUIRE_DEVMODE if (!cv_debug)\
{ CONS_Printf(M_GetText("DEVMODE must be enabled.\n")); return; }

#define REQUIRE_OBJECTPLACE if (!objectplacing)\
{ CONS_Printf(M_GetText("OBJECTPLACE must be enabled.\n")); return; }

#define REQUIRE_INLEVEL if (gamestate != GS_LEVEL || demoplayback)\
{ CONS_Printf(M_GetText("You must be in a level to use this.\n")); return; }

#define REQUIRE_SINGLEPLAYER if (netgame || multiplayer)\
{ CONS_Printf(M_GetText("This only works in single player.\n")); return; }

#define REQUIRE_NOULTIMATE if (ultimatemode)\
{ CONS_Printf(M_GetText("You're too good to be cheating!\n")); return; }

// command that can be typed at the console!
void Command_CheatNoClip_f(void)
{
	player_t *plyr;

	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;

	plyr = &players[consoleplayer];
	plyr->pflags ^= PF_NOCLIP;
	CONS_Printf(M_GetText("No Clipping %s\n"), plyr->pflags & PF_NOCLIP ? M_GetText("On") : M_GetText("Off"));

	G_SetGameModified(multiplayer);
}

void Command_CheatGod_f(void)
{
	player_t *plyr;

	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;

	plyr = &players[consoleplayer];
	plyr->pflags ^= PF_GODMODE;
	CONS_Printf(M_GetText("Cheese Mode %s\n"), plyr->pflags & PF_GODMODE ? M_GetText("On") : M_GetText("Off"));

	G_SetGameModified(multiplayer);
}

void Command_CheatNoTarget_f(void)
{
	player_t *plyr;

	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;

	plyr = &players[consoleplayer];
	plyr->pflags ^= PF_INVIS;
	CONS_Printf(M_GetText("SEP Field %s\n"), plyr->pflags & PF_INVIS ? M_GetText("On") : M_GetText("Off"));

	G_SetGameModified(multiplayer);
}

void Command_Scale_f(void)
{
	const double scaled = atof(COM_Argv(1));
	fixed_t scale = FLOAT_TO_FIXED(scaled);

	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (scale < FRACUNIT/100 || scale > 100*FRACUNIT) //COM_Argv(1) will return a null string if they did not give a paramater, so...
	{
		CONS_Printf(M_GetText("scale <value> (0.01-100.0): set player scale size\n"));
		return;
	}

	if (!players[consoleplayer].mo)
		return;

	players[consoleplayer].mo->destscale = scale;

	CONS_Printf(M_GetText("Scale set to %s\n"), COM_Argv(1));
}

void Command_Gravflip_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (players[consoleplayer].mo)
		players[consoleplayer].mo->flags2 ^= MF2_OBJECTFLIP;
}

void Command_Hurtme_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("hurtme <damage>: Damage yourself by a specific amount\n"));
		return;
	}

	P_DamageMobj(players[consoleplayer].mo, NULL, NULL, atoi(COM_Argv(1)), 0);
}

// Moves the NiGHTS player to another axis within the current mare
void Command_JumpToAxis_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("jumptoaxis <axisnum>: Jump to axis within current mare.\n"));
		return;
	}

	P_TransferToAxis(&players[consoleplayer], atoi(COM_Argv(1)));
}

void Command_Charability_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("charability <1/2> <value>: change character abilities\n"));
		return;
	}

	if (atoi(COM_Argv(1)) == 1)
		players[consoleplayer].charability = (UINT8)atoi(COM_Argv(2));
	else if (atoi(COM_Argv(1)) == 2)
		players[consoleplayer].charability2 = (UINT8)atoi(COM_Argv(2));
	else
		CONS_Printf(M_GetText("charability <1/2> <value>: change character abilities\n"));
}

void Command_Charspeed_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() < 3)
	{
		CONS_Printf(M_GetText("charspeed <normalspeed/runspeed/thrustfactor/accelstart/acceleration/actionspd> <value>: set character speed\n"));
		return;
	}

	if (!strcasecmp(COM_Argv(1), "normalspeed"))
		players[consoleplayer].normalspeed = atoi(COM_Argv(2))<<FRACBITS;
	else if (!strcasecmp(COM_Argv(1), "runspeed"))
		players[consoleplayer].runspeed = atoi(COM_Argv(2))<<FRACBITS;
	else if (!strcasecmp(COM_Argv(1), "thrustfactor"))
		players[consoleplayer].thrustfactor = atoi(COM_Argv(2));
	else if (!strcasecmp(COM_Argv(1), "accelstart"))
		players[consoleplayer].accelstart = atoi(COM_Argv(2));
	else if (!strcasecmp(COM_Argv(1), "acceleration"))
		players[consoleplayer].acceleration = atoi(COM_Argv(2));
	else if (!strcasecmp(COM_Argv(1), "actionspd"))
		players[consoleplayer].actionspd = atoi(COM_Argv(2))<<FRACBITS;
	else
		CONS_Printf(M_GetText("charspeed <normalspeed/runspeed/thrustfactor/accelstart/acceleration/actionspd> <value>: set character speed\n"));
}

void Command_RTeleport_f(void)
{
	fixed_t intx, inty, intz;
	size_t i;
	player_t *p = &players[consoleplayer];
	subsector_t *ss;

	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() < 3 || COM_Argc() > 7)
	{
		CONS_Printf(M_GetText("rteleport -x <value> -y <value> -z <value>: relative teleport to a location\n"));
		return;
	}

	if (!p->mo)
		return;

	i = COM_CheckParm("-x");
	if (i)
		intx = atoi(COM_Argv(i + 1));
	else
		intx = 0;

	i = COM_CheckParm("-y");
	if (i)
		inty = atoi(COM_Argv(i + 1));
	else
		inty = 0;

	ss = R_PointInSubsectorOrNull(p->mo->x + intx*FRACUNIT, p->mo->y + inty*FRACUNIT);
	if (!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Not a valid location.\n"));
		return;
	}
	i = COM_CheckParm("-z");
	if (i)
	{
		intz = atoi(COM_Argv(i + 1));
		intz <<= FRACBITS;
		intz += p->mo->z;
		if (intz < ss->sector->floorheight)
			intz = ss->sector->floorheight;
		if (intz > ss->sector->ceilingheight - p->mo->height)
			intz = ss->sector->ceilingheight - p->mo->height;
	}
	else
		intz = p->mo->z;

	CONS_Printf(M_GetText("Teleporting by %d, %d, %d...\n"), intx, inty, FixedInt((intz-p->mo->z)));

	P_MapStart();
	if (!P_TeleportMove(p->mo, p->mo->x+intx*FRACUNIT, p->mo->y+inty*FRACUNIT, intz))
		CONS_Alert(CONS_WARNING, M_GetText("Unable to teleport to that spot!\n"));
	else
		S_StartSound(p->mo, sfx_mixup);
	P_MapEnd();
}

void Command_Teleport_f(void)
{
	fixed_t intx, inty, intz;
	size_t i;
	player_t *p = &players[consoleplayer];
	subsector_t *ss;

	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() < 3 || COM_Argc() > 11)
	{
		CONS_Printf(M_GetText("teleport -x <value> -y <value> -z <value> -ang <value> -aim <value>: teleport to a location\nteleport -sp <sequence> <placement>: teleport to specified checkpoint\n"));
		return;
	}

	if (!p->mo)
		return;

	i = COM_CheckParm("-sp");
	if (i)
	{
		INT32 starpostnum = atoi(COM_Argv(i + 1)); // starpost number
		INT32 starpostpath = atoi(COM_Argv(i + 2)); // quick, dirty way to distinguish between paths

		if (starpostnum < 0 || starpostpath < 0)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Negative starpost indexing is not valid.\n"));
			return;
		}

		if (!starpostnum) // spawnpoints...
		{
			mapthing_t *mt;
			fixed_t offset;

			if (starpostpath >= numcoopstarts)
			{
				CONS_Alert(CONS_NOTICE, M_GetText("Player %d spawnpoint not found (%d max).\n"), starpostpath+1, numcoopstarts-1);
				return;
			}

			mt = playerstarts[starpostpath]; // Given above check, should never be NULL.
			intx = mt->x<<FRACBITS;
			inty = mt->y<<FRACBITS;
			offset = mt->z<<FRACBITS;

			ss = R_PointInSubsectorOrNull(intx, inty);
			if (!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
			{
				CONS_Alert(CONS_NOTICE, M_GetText("Spawnpoint not in a valid location.\n"));
				return;
			}

			// Flagging a player's ambush will make them start on the ceiling
			// Objectflip inverts
			if (!!(mt->options & MTF_AMBUSH) ^ !!(mt->options & MTF_OBJECTFLIP))
				intz = ss->sector->ceilingheight - p->mo->height - offset;
			else
				intz = ss->sector->floorheight + offset;

			if (mt->options & MTF_OBJECTFLIP) // flip the player!
			{
				p->mo->eflags |= MFE_VERTICALFLIP;
				p->mo->flags2 |= MF2_OBJECTFLIP;
			}
			else
			{
				p->mo->eflags &= ~MFE_VERTICALFLIP;
				p->mo->flags2 &= ~MF2_OBJECTFLIP;
			}

			p->mo->angle = p->drawangle = FixedAngle(mt->angle<<FRACBITS);
			P_SetPlayerAngle(p, p->mo->angle);
		}
		else // scan the thinkers to find starposts...
		{
			mobj_t *mo2 = NULL;
			thinker_t *th;

			INT32 starpostmax = 0;
			intz = starpostpath; // variable reuse - counting down for selection purposes

			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;

				mo2 = (mobj_t *)th;

				if (mo2->type != MT_STARPOST)
					continue;

				if (mo2->health != starpostnum)
				{
					if (mo2->health > starpostmax)
						starpostmax = mo2->health;
					continue;
				}

				if (intz--)
					continue;

				break;
			}

			if (th == &thlist[THINK_MOBJ])
			{
				if (intz == starpostpath)
					CONS_Alert(CONS_NOTICE, M_GetText("No starpost of position %d found (%d max).\n"), starpostnum, starpostmax);
				else
					CONS_Alert(CONS_NOTICE, M_GetText("Starpost of position %d, %d not found (%d, %d max).\n"), starpostnum, starpostpath, starpostmax, (starpostpath-intz)-1);
				return;
			}

			ss = R_PointInSubsectorOrNull(mo2->x, mo2->y);
			if (!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
			{
				CONS_Alert(CONS_NOTICE, M_GetText("Starpost not in a valid location.\n"));
				return;
			}

			intx = mo2->x;
			inty = mo2->y;
			intz = mo2->z;

			if (mo2->flags2 & MF2_OBJECTFLIP) // flip the player!
			{
				p->mo->eflags |= MFE_VERTICALFLIP;
				p->mo->flags2 |= MF2_OBJECTFLIP;
			}
			else
			{
				p->mo->eflags &= ~MFE_VERTICALFLIP;
				p->mo->flags2 &= ~MF2_OBJECTFLIP;
			}

			p->mo->angle = p->drawangle = mo2->angle;
			P_SetPlayerAngle(p, p->mo->angle);
		}

		CONS_Printf(M_GetText("Teleporting to checkpoint %d, %d...\n"), starpostnum, starpostpath);
	}
	else
	{
		i = COM_CheckParm("-nop"); // undocumented stupid addition to allow pivoting on the spot with -ang and -aim
		if (i)
		{
			intx = p->mo->x;
			inty = p->mo->y;
		}
		else
		{
			i = COM_CheckParm("-x");
			if (i)
				intx = atoi(COM_Argv(i + 1))<<FRACBITS;
			else
			{
				CONS_Alert(CONS_NOTICE, M_GetText("%s value not specified.\n"), "X");
				return;
			}

			i = COM_CheckParm("-y");
			if (i)
				inty = atoi(COM_Argv(i + 1))<<FRACBITS;
			else
			{
				CONS_Alert(CONS_NOTICE, M_GetText("%s value not specified.\n"), "Y");
				return;
			}
		}

		ss = R_PointInSubsectorOrNull(intx, inty);
		if (!ss || ss->sector->ceilingheight - ss->sector->floorheight < p->mo->height)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Not a valid location.\n"));
			return;
		}
		i = COM_CheckParm("-z");
		if (i)
		{
			intz = atoi(COM_Argv(i + 1))<<FRACBITS;
			if (intz < ss->sector->floorheight)
				intz = ss->sector->floorheight;
			if (intz > ss->sector->ceilingheight - p->mo->height)
				intz = ss->sector->ceilingheight - p->mo->height;
		}
		else
			intz = ((p->mo->eflags & MFE_VERTICALFLIP) ? ss->sector->ceilingheight : ss->sector->floorheight);

		i = COM_CheckParm("-ang");
		if (i)
		{
			p->drawangle = p->mo->angle = FixedAngle(atoi(COM_Argv(i + 1))<<FRACBITS);
			P_SetPlayerAngle(p, p->mo->angle);
		}

		i = COM_CheckParm("-aim");
		if (i)
		{
			angle_t aim = FixedAngle(atoi(COM_Argv(i + 1))<<FRACBITS);
			if (aim >= ANGLE_90 && aim <= ANGLE_270)
			{
				CONS_Alert(CONS_NOTICE, M_GetText("Not a valid aiming angle (between +/-90).\n"));
				return;
			}
			localaiming = p->aiming = aim;
		}

		CONS_Printf(M_GetText("Teleporting to %d, %d, %d...\n"), FixedInt(intx), FixedInt(inty), FixedInt(intz));
	}

	P_MapStart();
	if (!P_TeleportMove(p->mo, intx, inty, intz))
		CONS_Alert(CONS_WARNING, M_GetText("Unable to teleport to that spot!\n"));
	else
		S_StartSound(p->mo, sfx_mixup);
	P_MapEnd();
}

void Command_Skynum_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("skynum <sky#>: change the sky\n"));
		CONS_Printf(M_GetText("Current sky is %d\n"), levelskynum);
		return;
	}

	CONS_Printf(M_GetText("Previewing sky %s...\n"), COM_Argv(1));

	P_SetupLevelSky(atoi(COM_Argv(1)), false);
}

void Command_Weather_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("weather <weather#>: change the weather\n"));
		CONS_Printf(M_GetText("Current weather is %d\n"), curWeather);
		return;
	}

	CONS_Printf(M_GetText("Previewing weather %s...\n"), COM_Argv(1));

	P_SwitchWeather(atoi(COM_Argv(1)));
}

void Command_Toggletwod_f(void)
{
	player_t *p = &players[consoleplayer];

	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	if (p->mo)
		p->mo->flags2 ^= MF2_TWOD;
}

#ifdef _DEBUG
// You never thought you needed this, did you? >=D
// Yes, this has the specific purpose of completely screwing you up
// to see if the consistency restoration code can fix you.
// Don't enable this for normal builds...
void Command_CauseCfail_f(void)
{
	if (consoleplayer == serverplayer)
	{
		CONS_Printf(M_GetText("Only remote players can use this command.\n"));
		return;
	}

	P_UnsetThingPosition(players[consoleplayer].mo);
	P_RandomFixed();
	P_RandomByte();
	P_RandomFixed();
	players[consoleplayer].mo->x = 0;
	players[consoleplayer].mo->y = 123311; //cfail cansuled kthxbye
	players[consoleplayer].mo->z = 123311;
	players[consoleplayer].score = 1337;
	players[consoleplayer].rings = 1337;
	players[consoleplayer].mo->destscale = 25;
	P_SetThingPosition(players[consoleplayer].mo);

	// CTF consistency test
	if (gametyperules & GTR_TEAMFLAGS)
	{
		if (blueflag) {
			P_RemoveMobj(blueflag);
			blueflag = NULL;
		}
		if (redflag)
		{
			redflag->x = 423423;
			redflag->y = 666;
			redflag->z = 123311;
		}
	}
}
#endif

#ifdef LUA_ALLOW_BYTECODE
void Command_Dumplua_f(void)
{
	if (modifiedgame)
	{
		CONS_Printf(M_GetText("This command has been disabled in modified games, to prevent scripted attacks.\n"));
		return;
	}

	if (COM_Argc() < 2)
	{
		CONS_Printf(M_GetText("dumplua <filename>: Compile a plain text lua script (not a wad!) into bytecode.\n"));
		CONS_Alert(CONS_WARNING, M_GetText("This command makes irreversible file changes, please use with caution!\n"));
		return;
	}

	LUA_DumpFile(COM_Argv(1));
}
#endif

void Command_Savecheckpoint_f(void)
{
	REQUIRE_DEVMODE;
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;

	players[consoleplayer].starposttime = players[consoleplayer].realtime;
	players[consoleplayer].starpostx = players[consoleplayer].mo->x>>FRACBITS;
	players[consoleplayer].starposty = players[consoleplayer].mo->y>>FRACBITS;
	players[consoleplayer].starpostz = players[consoleplayer].mo->floorz>>FRACBITS;
	players[consoleplayer].starpostangle = players[consoleplayer].mo->angle;
	players[consoleplayer].starpostscale = players[consoleplayer].mo->destscale;
	if (players[consoleplayer].mo->flags2 & MF2_OBJECTFLIP)
	{
		players[consoleplayer].starpostscale *= -1;
		players[consoleplayer].starpostz += players[consoleplayer].mo->height;
	}

	CONS_Printf(M_GetText("Temporary checkpoint created at %d, %d, %d\n"), players[consoleplayer].starpostx, players[consoleplayer].starposty, players[consoleplayer].starpostz);
}

// Like M_GetAllEmeralds() but for console devmode junkies.
void Command_Getallemeralds_f(void)
{
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;
	REQUIRE_PANDORA;

	emeralds = ((EMERALD7)*2)-1;

	CONS_Printf(M_GetText("You now have all 7 emeralds.\n"));
}

void Command_Resetemeralds_f(void)
{
	REQUIRE_SINGLEPLAYER;
	REQUIRE_PANDORA;

	emeralds = 0;

	CONS_Printf(M_GetText("Emeralds reset to zero.\n"));
}

void Command_Devmode_f(void)
{
#ifndef _DEBUG
	REQUIRE_SINGLEPLAYER;
#endif
	REQUIRE_NOULTIMATE;

	if (COM_Argc() > 1)
	{
		const char *arg = COM_Argv(1);

		if (arg[0] && arg[0] == '0' &&
			arg[1] && arg[1] == 'x') // Use hexadecimal!
			cv_debug = axtoi(arg+2);
		else
			cv_debug = atoi(arg);
	}
	else
	{
		CONS_Printf(M_GetText("devmode <flags>: enable debugging tools and info, prepend with 0x to use hexadecimal\n"));
		return;
	}

	G_SetGameModified(multiplayer);
}

void Command_Setrings_f(void)
{
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;
	REQUIRE_PANDORA;

	if (COM_Argc() > 1)
	{
		if (!(maptol & TOL_NIGHTS))
		{
			// P_GivePlayerRings does value clamping
			players[consoleplayer].rings = 0;
			P_GivePlayerRings(&players[consoleplayer], atoi(COM_Argv(1)));
			players[consoleplayer].totalring -= atoi(COM_Argv(1)); //undo totalring addition done in P_GivePlayerRings
		}
		else
		{
			players[consoleplayer].spheres = 0;
			P_GivePlayerSpheres(&players[consoleplayer], atoi(COM_Argv(1)));
			// no totalsphere addition to revert
		}

		G_SetGameModified(multiplayer);
	}
}

void Command_Setlives_f(void)
{
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;
	REQUIRE_PANDORA;

	if (COM_Argc() > 1)
	{
		SINT8 lives = atoi(COM_Argv(1));
		if (lives == -1)
			players[consoleplayer].lives = INFLIVES; // infinity!
		else
		{
			// P_GivePlayerLives does value clamping
			players[consoleplayer].lives = 0;
			P_GivePlayerLives(&players[consoleplayer], atoi(COM_Argv(1)));
		}

		G_SetGameModified(multiplayer);
	}
}

void Command_Setcontinues_f(void)
{
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;
	REQUIRE_PANDORA;

	if (!continuesInSession)
	{
		CONS_Printf(M_GetText("This session does not use continues.\n"));
		return;
	}

	if (COM_Argc() > 1)
	{
		INT32 numcontinues = atoi(COM_Argv(1));
		if (numcontinues > 99)
			numcontinues = 99;
		else if (numcontinues < 0)
			numcontinues = 0;

		players[consoleplayer].continues = numcontinues;

		G_SetGameModified(multiplayer);
	}
}

//
// OBJECTPLACE (and related variables)
//
static CV_PossibleValue_t op_mapthing_t[] = {{0, "MIN"}, {4095, "MAX"}, {0, NULL}};
static CV_PossibleValue_t op_speed_t[] = {{1, "MIN"}, {128, "MAX"}, {0, NULL}};
static CV_PossibleValue_t op_flags_t[] = {{0, "MIN"}, {15, "MAX"}, {0, NULL}};
static CV_PossibleValue_t op_hoopflags_t[] = {{0, "MIN"}, {15, "MAX"}, {0, NULL}};

consvar_t cv_mapthingnum = {"op_mapthingnum", "0", CV_NOTINNET, op_mapthing_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_speed = {"op_speed", "16", CV_NOTINNET, op_speed_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_opflags = {"op_flags", "0", CV_NOTINNET, op_flags_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_ophoopflags = {"op_hoopflags", "4", CV_NOTINNET, op_hoopflags_t, NULL, 0, NULL, NULL, 0, 0, NULL};

boolean objectplacing = false;
mobjtype_t op_currentthing = 0; // For the object placement mode
UINT16 op_currentdoomednum = 0; // For display, etc
UINT32 op_displayflags = 0; // for display in ST_stuff

static pflags_t op_oldpflags = 0;
static mobjflag_t op_oldflags1 = 0;
static mobjflag2_t op_oldflags2 = 0;
static UINT32 op_oldeflags = 0;
static fixed_t op_oldmomx = 0, op_oldmomy = 0, op_oldmomz = 0, op_oldheight = 0;
static statenum_t op_oldstate = 0;
static UINT16 op_oldcolor = 0;

//
// Static calculation / common output help
//
static void OP_CycleThings(INT32 amt)
{
	INT32 add = (amt > 0 ? 1 : -1);

	while (amt)
	{
		do
		{
			op_currentthing += add;
			if (op_currentthing <= 0)
				op_currentthing = NUMMOBJTYPES-1;
			if (op_currentthing >= NUMMOBJTYPES)
				op_currentthing = 0;
		} while
		(mobjinfo[op_currentthing].doomednum == -1
			|| op_currentthing == MT_NIGHTSDRONE
			|| mobjinfo[op_currentthing].flags & (MF_AMBIENT|MF_NOSECTOR)
			|| (states[mobjinfo[op_currentthing].spawnstate].sprite == SPR_NULL
			 && states[mobjinfo[op_currentthing].seestate].sprite == SPR_NULL)
		);
		amt -= add;
	}

	// HACK, minus has SPR_NULL sprite
	if (states[mobjinfo[op_currentthing].spawnstate].sprite == SPR_NULL)
	{
		states[S_OBJPLACE_DUMMY].sprite = states[mobjinfo[op_currentthing].seestate].sprite;
		states[S_OBJPLACE_DUMMY].frame = states[mobjinfo[op_currentthing].seestate].frame;
	}
	else
	{
		states[S_OBJPLACE_DUMMY].sprite = states[mobjinfo[op_currentthing].spawnstate].sprite;
		states[S_OBJPLACE_DUMMY].frame = states[mobjinfo[op_currentthing].spawnstate].frame;
	}
	if (players[0].mo->eflags & MFE_VERTICALFLIP) // correct z when flipped
		players[0].mo->z += players[0].mo->height - FixedMul(mobjinfo[op_currentthing].height, players[0].mo->scale);
	players[0].mo->height = FixedMul(mobjinfo[op_currentthing].height, players[0].mo->scale);
	P_SetPlayerMobjState(players[0].mo, S_OBJPLACE_DUMMY);

	op_currentdoomednum = mobjinfo[op_currentthing].doomednum;
}

static boolean OP_HeightOkay(player_t *player, UINT8 ceiling)
{
	sector_t *sec = player->mo->subsector->sector;

	if (ceiling)
	{
		// Truncate position to match where mapthing would be when spawned
		// (this applies to every further P_GetSlopeZAt call as well)
		fixed_t cheight = P_GetSectorCeilingZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);

		if (((cheight - player->mo->z - player->mo->height)>>FRACBITS) >= (1 << (16-ZSHIFT)))
		{
			CONS_Printf(M_GetText("Sorry, you're too %s to place this object (max: %d %s).\n"), M_GetText("low"),
				(1 << (16-ZSHIFT)), M_GetText("below top ceiling"));
			return false;
		}
	}
	else
	{
		fixed_t fheight = P_GetSectorFloorZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);
		if (((player->mo->z - fheight)>>FRACBITS) >= (1 << (16-ZSHIFT)))
		{
			CONS_Printf(M_GetText("Sorry, you're too %s to place this object (max: %d %s).\n"), M_GetText("high"),
				(1 << (16-ZSHIFT)), M_GetText("above bottom floor"));
			return false;
		}
	}
	return true;
}

static mapthing_t *OP_CreateNewMapThing(player_t *player, UINT16 type, boolean ceiling)
{
	mapthing_t *mt = mapthings;
	sector_t *sec = player->mo->subsector->sector;

	LUA_InvalidateMapthings();

	mapthings = Z_Realloc(mapthings, ++nummapthings * sizeof (*mapthings), PU_LEVEL, NULL);

	// as Z_Realloc can relocate mapthings, quickly go through thinker list and correct
	// the spawnpoints of any objects that have them to the new location
	if (mt != mapthings)
	{
		thinker_t *th;
		mobj_t *mo;

		for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
		{
			if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
				continue;

			mo = (mobj_t *)th;
			// get offset from mt, which points to old mapthings, then add new location
			if (!mo->spawnpoint)
				continue;
			mo->spawnpoint = (mo->spawnpoint - mt) + mapthings;
		}
	}

	mt = (mapthings+nummapthings-1);

	mt->type = type;
	mt->x = (INT16)(player->mo->x>>FRACBITS);
	mt->y = (INT16)(player->mo->y>>FRACBITS);
	if (ceiling)
	{
		fixed_t cheight = P_GetSectorCeilingZAt(sec, mt->x << FRACBITS, mt->y << FRACBITS);
		mt->z = (UINT16)((cheight - player->mo->z - player->mo->height)>>FRACBITS);
	}
	else
	{
		fixed_t fheight = P_GetSectorFloorZAt(sec, mt->x << FRACBITS, mt->y << FRACBITS);
		mt->z = (UINT16)((player->mo->z - fheight)>>FRACBITS);
	}
	mt->angle = (INT16)(FixedInt(AngleFixed(player->mo->angle)));

	mt->options = (mt->z << ZSHIFT) | (UINT16)cv_opflags.value;
	mt->scale = player->mo->scale;
	mt->tag = 0;
	memset(mt->args, 0, NUMMAPTHINGARGS*sizeof(*mt->args));
	memset(mt->stringargs, 0x00, NUMMAPTHINGSTRINGARGS*sizeof(*mt->stringargs));
	mt->pitch = mt->roll = 0;
	return mt;
}

//
// Helper functions
//
boolean OP_FreezeObjectplace(void)
{
	if (!objectplacing)
		return false;

	if ((maptol & TOL_NIGHTS) && (players[consoleplayer].powers[pw_carry] == CR_NIGHTSMODE))
		return false;

	return true;
}

void OP_ResetObjectplace(void)
{
	objectplacing = false;
	op_currentthing = 0;
}

//
// Main meat of objectplace: handling functions
//
void OP_NightsObjectplace(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	mapthing_t *mt;

	player->nightstime = 3*TICRATE;
	player->drillmeter = TICRATE;

	if (player->pflags & PF_ATTACKDOWN)
	{
		// Are ANY objectplace buttons pressed?  If no, remove flag.
		if (!(cmd->buttons & (BT_ATTACK|BT_TOSSFLAG|BT_SPIN|BT_WEAPONNEXT|BT_WEAPONPREV)))
			player->pflags &= ~PF_ATTACKDOWN;

		// Do nothing.
		return;
	}

	// This places a hoop!
	if (cmd->buttons & BT_ATTACK)
	{
		UINT16 angle = (UINT16)(player->anotherflyangle % 360);
		INT16 temp = (INT16)FixedInt(AngleFixed(player->mo->angle)); // Traditional 2D Angle

		player->pflags |= PF_ATTACKDOWN;

		mt = OP_CreateNewMapThing(player, 1713, false);

		// Tilt
		mt->angle = (INT16)FixedInt(FixedDiv(angle*FRACUNIT, 360*(FRACUNIT/256)));

		if (player->anotherflyangle < 90 || player->anotherflyangle > 270)
			temp -= 90;
		else
			temp += 90;
		temp %= 360;

		mt->options = (mt->options & ~(UINT16)cv_opflags.value) | (UINT16)cv_ophoopflags.value;
		mt->angle = (INT16)(mt->angle+(INT16)((FixedInt(FixedDiv(temp*FRACUNIT, 360*(FRACUNIT/256))))<<8));

		P_SpawnHoop(mt);
	}

	// This places a bumper!
	if (cmd->buttons & BT_TOSSFLAG)
	{
		UINT16 vertangle = (UINT16)(player->anotherflyangle % 360);
		UINT16 newflags;

		player->pflags |= PF_ATTACKDOWN;
		if (!OP_HeightOkay(player, false))
			return;

		mt = OP_CreateNewMapThing(player, (UINT16)mobjinfo[MT_NIGHTSBUMPER].doomednum, false);
		mt->z = min(mt->z - (mobjinfo[MT_NIGHTSBUMPER].height/4), 0);
			// height offset: from P_TouchSpecialThing case MT_NIGHTSBUMPER

		// clockwise
		if (vertangle >= 75 && vertangle < 105) // up
			newflags = 3;
		else if (vertangle >= 105 && vertangle < 135) // 60 upward tilt
			newflags = 2;
		else if (vertangle >= 135 && vertangle < 165) // 30 upward tilt
			newflags = 1;
		//else if (vertangle >= 165 && vertangle < 195) // forward, see else case
		//	newflags = 0;
		else if (vertangle >= 195 && vertangle < 225) // 30 downward tilt
			newflags = 11;
		else if (vertangle >= 225 && vertangle < 255) // 60 downward tilt
			newflags = 10;
		else if (vertangle >= 255 && vertangle < 285) // down
			newflags = 9;
		else if (vertangle >= 285 && vertangle < 315) // 60 downward tilt backwards
			newflags = 8;
		else if (vertangle >= 315 && vertangle < 345) // 30 downward tilt backwards
			newflags = 7;
		else if (vertangle >= 345 || vertangle < 15) // backwards
			newflags = 6;
		else if (vertangle >= 15 && vertangle < 45) // 30 upward tilt backwards
			newflags = 5;
		else if (vertangle >= 45 && vertangle < 75) // 60 upward tilt backwards
			newflags = 4;
		else // forward
			newflags = 0;

		mt->options = (mt->z << ZSHIFT) | newflags;

		// if NiGHTS is facing backwards, orient the Thing angle forwards so that the sprite angle
		// displays correctly. Backwards movement via the Thing flags is unaffected.
		if (vertangle < 90 || vertangle > 270)
			mt->angle = (mt->angle + 180) % 360;

		P_SpawnMapThing(mt);
	}

	// This places a sphere!
	if (cmd->buttons & BT_WEAPONNEXT)
	{
		player->pflags |= PF_ATTACKDOWN;
		if (!OP_HeightOkay(player, false))
			return;

		mt = OP_CreateNewMapThing(player, (UINT16)mobjinfo[MT_BLUESPHERE].doomednum, false);
		P_SpawnMapThing(mt);
	}

	// This places a ring!
	if (cmd->buttons & BT_WEAPONPREV)
	{
		player->pflags |= PF_ATTACKDOWN;
		if (!OP_HeightOkay(player, false))
			return;

		mt = OP_CreateNewMapThing(player, (UINT16)mobjinfo[MT_RING].doomednum, false);
		P_SpawnMapThing(mt);
	}

	// This places a custom object as defined in the console cv_mapthingnum.
	if (cmd->buttons & BT_SPIN)
	{
		UINT16 angle;

		player->pflags |= PF_ATTACKDOWN;
		if (!cv_mapthingnum.value)
		{
			CONS_Alert(CONS_WARNING, "Set op_mapthingnum first!\n");
			return;
		}
		if (!OP_HeightOkay(player, false))
			return;

		if (player->mo->target->flags2 & MF2_AMBUSH)
			angle = (UINT16)player->anotherflyangle;
		else
		{
			angle = (UINT16)((360-player->anotherflyangle) % 360);
			if (angle > 90 && angle < 270)
			{
				angle += 180;
				angle %= 360;
			}
		}

		mt = OP_CreateNewMapThing(player, (UINT16)cv_mapthingnum.value, false);
		mt->angle = angle;

		if (mt->type >= 600 && mt->type <= 609) // Placement patterns
			P_SpawnItemPattern(mt, false);
		else if (mt->type == 1705 || mt->type == 1713) // NiGHTS Hoops
			P_SpawnHoop(mt);
		else
			P_SpawnMapThing(mt);
	}
}

//
// OP_ObjectplaceMovement
//
// Control code for Objectplace mode
//
void OP_ObjectplaceMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;

	player->drawangle = player->mo->angle = (cmd->angleturn<<16 /* not FRACBITS */);

	ticruned++;
	if (!(cmd->angleturn & TICCMD_RECEIVED))
		ticmiss++;

	if (cmd->buttons & BT_JUMP)
		player->mo->z += player->mo->scale*cv_speed.value;
	else if (cmd->buttons & BT_SPIN)
		player->mo->z -= player->mo->scale*cv_speed.value;

	if (cmd->forwardmove != 0)
	{
		P_Thrust(player->mo, player->mo->angle, (cmd->forwardmove*player->mo->scale/MAXPLMOVE)*cv_speed.value);
		P_TeleportMove(player->mo, player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z);
		player->mo->momx = player->mo->momy = 0;
	}
	if (cmd->sidemove != 0)
	{
		P_Thrust(player->mo, player->mo->angle-ANGLE_90, (cmd->sidemove*player->mo->scale/MAXPLMOVE)*cv_speed.value);
		P_TeleportMove(player->mo, player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z);
		player->mo->momx = player->mo->momy = 0;
	}

	if (player->mo->z > player->mo->ceilingz - player->mo->height)
		player->mo->z = player->mo->ceilingz - player->mo->height;
	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	if (cv_opflags.value & MTF_OBJECTFLIP)
		player->mo->eflags |= MFE_VERTICALFLIP;
	else
		player->mo->eflags &= ~MFE_VERTICALFLIP;

	// make sure viewz follows player if in 1st person mode
	player->deltaviewheight = 0;
	player->viewheight = FixedMul(41*player->height/48, player->mo->scale);
	if (player->mo->eflags & MFE_VERTICALFLIP)
		player->viewz = player->mo->z + player->mo->height - player->viewheight;
	else
		player->viewz = player->mo->z + player->viewheight;

	// Display flag information
	// Moved up so it always updates.
	{
		sector_t *sec = player->mo->subsector->sector;

		if (!!(mobjinfo[op_currentthing].flags & MF_SPAWNCEILING) ^ !!(cv_opflags.value & MTF_OBJECTFLIP))
		{
			fixed_t cheight = P_GetSectorCeilingZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);
			op_displayflags = (UINT16)((cheight - player->mo->z - mobjinfo[op_currentthing].height)>>FRACBITS);
		}
		else
		{
			fixed_t fheight = P_GetSectorFloorZAt(sec, player->mo->x & 0xFFFF0000, player->mo->y & 0xFFFF0000);
			op_displayflags = (UINT16)((player->mo->z - fheight)>>FRACBITS);
		}
		op_displayflags <<= ZSHIFT;
		op_displayflags |= (UINT16)cv_opflags.value;
	}

	if (player->pflags & PF_ATTACKDOWN)
	{
		// Are ANY objectplace buttons pressed?  If no, remove flag.
		if (!(cmd->buttons & (BT_ATTACK|BT_TOSSFLAG|BT_WEAPONNEXT|BT_WEAPONPREV)))
			player->pflags &= ~PF_ATTACKDOWN;

		// Do nothing.
		return;
	}

	if (cmd->buttons & BT_WEAPONPREV)
	{
		OP_CycleThings(-1);
		player->pflags |= PF_ATTACKDOWN;
	}

	if (cmd->buttons & BT_WEAPONNEXT)
	{
		OP_CycleThings(1);
		player->pflags |= PF_ATTACKDOWN;
	}

	// Place an object and add it to the maplist
	if (cmd->buttons & BT_ATTACK)
	{
		mapthing_t *mt;
		mobjtype_t spawnmid = op_currentthing;
		mobjtype_t spawnthing = op_currentdoomednum;
		boolean ceiling;

		player->pflags |= PF_ATTACKDOWN;

		if (cv_mapthingnum.value > 0 && cv_mapthingnum.value < 4096)
		{
			// find which type to spawn
			for (spawnmid = 0; spawnmid < NUMMOBJTYPES; ++spawnmid)
				if (cv_mapthingnum.value == mobjinfo[spawnmid].doomednum)
					break;

			if (spawnmid == NUMMOBJTYPES)
			{
				CONS_Alert(CONS_ERROR, M_GetText("Can't place an object with mapthingnum %d.\n"), cv_mapthingnum.value);
				return;
			}
			spawnthing = mobjinfo[spawnmid].doomednum;
		}

		ceiling = !!(mobjinfo[spawnmid].flags & MF_SPAWNCEILING) ^ !!(cv_opflags.value & MTF_OBJECTFLIP);
		if (!OP_HeightOkay(player, ceiling))
			return;

		mt = OP_CreateNewMapThing(player, (UINT16)spawnthing, ceiling);
		if (mt->type >= 600 && mt->type <= 609) // Placement patterns
			P_SpawnItemPattern(mt, false);
		else if (mt->type == 1705 || mt->type == 1713) // NiGHTS Hoops
			P_SpawnHoop(mt);
		else
			P_SpawnMapThing(mt);

		CONS_Printf(M_GetText("Placed object type %d at %d, %d, %d, %d\n"), mt->type, mt->x, mt->y, mt->z, mt->angle);
	}
}

//
// Objectplace related commands.
//
void Command_Writethings_f(void)
{
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_OBJECTPLACE;

	P_WriteThings(W_GetNumForName(G_BuildMapName(gamemap)) + ML_THINGS);
}

void Command_ObjectPlace_f(void)
{
	REQUIRE_INLEVEL;
	REQUIRE_SINGLEPLAYER;
	REQUIRE_NOULTIMATE;

	G_SetGameModified(multiplayer);

	// Entering objectplace?
	if (!objectplacing || COM_Argc() > 1)
	{
		if (!objectplacing)
		{
			objectplacing = true;

			if (players[0].powers[pw_carry] == CR_NIGHTSMODE)
				return;

			if (!COM_CheckParm("-silent"))
			{
				HU_SetCEchoFlags(V_RETURN8|V_MONOSPACE|V_AUTOFADEOUT);
				HU_SetCEchoDuration(10);
				HU_DoCEcho(va(M_GetText(
					"\\\\\\\\\\\\\\\\\\\\\\\\\x82"
					"   Objectplace Controls:   \x80\\\\"
					"Weapon Next/Prev: Cycle mapthings\\"
					"            Jump: Float up       \\"
					"            Spin: Float down     \\"
					"       Fire Ring: Place object   \\")));
			}

			// Save all the player's data.
			op_oldflags1 = players[0].mo->flags;
			op_oldflags2 = players[0].mo->flags2;
			op_oldeflags = players[0].mo->eflags;
			op_oldpflags = players[0].pflags;
			op_oldmomx = players[0].mo->momx;
			op_oldmomy = players[0].mo->momy;
			op_oldmomz = players[0].mo->momz;
			op_oldheight = players[0].mo->height;
			op_oldstate = S_PLAY_STND;
			op_oldcolor = players[0].mo->color; // save color too in case of super/fireflower

			// Remove ALL flags and motion.
			P_UnsetThingPosition(players[0].mo);
			players[0].pflags = 0;
			players[0].mo->flags2 = 0;
			players[0].mo->eflags = 0;
			players[0].mo->flags = (MF_NOCLIP|MF_NOGRAVITY|MF_NOBLOCKMAP);
			players[0].mo->momx = players[0].mo->momy = players[0].mo->momz = 0;
			P_SetThingPosition(players[0].mo);

			// Take away color so things display properly
			players[0].mo->color = 0;

			// Like the classics, recover from death by entering objectplace
			if (players[0].mo->health <= 0)
			{
				players[0].mo->health = 1;
				players[0].deadtimer = 0;
				op_oldflags1 = mobjinfo[MT_PLAYER].flags;
				++players[0].lives;
				players[0].playerstate = PST_LIVE;
				P_RestoreMusic(&players[0]);
			}
			else
				op_oldstate = (statenum_t)(players[0].mo->state-states);
		}

		if (COM_Argc() > 1)
		{
			UINT16 mapthingnum = atoi(COM_Argv(1));
			mobjtype_t type = P_GetMobjtype(mapthingnum);
			if (type == MT_UNKNOWN)
				CONS_Printf(M_GetText("No mobj type delegated to thing type %d.\n"), mapthingnum);
			else
				op_currentthing = type;
		}

		// If no thing set, then cycle a little
		if (!op_currentthing)
		{
			op_currentthing = 1;
			OP_CycleThings(1);
		}
		else
			OP_CycleThings(0); // sets all necessary height values without cycling op_currentthing

		P_SetPlayerMobjState(players[0].mo, S_OBJPLACE_DUMMY);
	}
	// Or are we leaving it instead?
	else
	{
		objectplacing = false;

		// Don't touch the NiGHTS Objectplace stuff.
		// ... or if the mo mysteriously vanished.
		if (!players[0].mo || (players[0].powers[pw_carry] == CR_NIGHTSMODE))
			return;

		// If still in dummy state, get out of it.
		if (players[0].mo->state == &states[S_OBJPLACE_DUMMY])
			P_SetPlayerMobjState(players[0].mo, op_oldstate);

		// Reset everything back to how it was before we entered objectplace.
		P_UnsetThingPosition(players[0].mo);
		players[0].mo->flags = op_oldflags1;
		players[0].mo->flags2 = op_oldflags2;
		players[0].mo->eflags = op_oldeflags;
		players[0].pflags = op_oldpflags;
		players[0].mo->momx = op_oldmomx;
		players[0].mo->momy = op_oldmomy;
		players[0].mo->momz = op_oldmomz;
		players[0].mo->height = op_oldheight;
		P_SetThingPosition(players[0].mo);

		// Return their color to normal.
		players[0].mo->color = op_oldcolor;

		// This is necessary for recovery of dying players.
		if (players[0].powers[pw_flashing] >= flashingtics)
			players[0].powers[pw_flashing] = flashingtics - 1;
	}
}
