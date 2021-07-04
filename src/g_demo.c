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
/// \file  g_demo.c
/// \brief Demo recording and playback

#include "doomdef.h"
#include "console.h"
#include "d_main.h"
#include "d_player.h"
#include "d_clisrv.h"
#include "p_setup.h"
#include "i_system.h"
#include "m_random.h"
#include "p_local.h"
#include "r_draw.h"
#include "r_main.h"
#include "g_game.h"
#include "g_demo.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_argv.h"
#include "hu_stuff.h"
#include "z_zone.h"
#include "i_video.h"
#include "byteptr.h"
#include "i_joy.h"
#include "r_local.h"
#include "r_skins.h"
#include "y_inter.h"
#include "v_video.h"
#include "lua_hook.h"
#include "md5.h" // demo checksums

boolean timingdemo; // if true, exit with report on completion
boolean nodrawers; // for comparative timing purposes
boolean noblit; // for comparative timing purposes
tic_t demostarttime; // for comparative timing purposes

static char demoname[64];
boolean demorecording;
boolean demoplayback;
boolean titledemo; // Title Screen demo can be cancelled by any key
static UINT8 *demobuffer = NULL;
static UINT8 *demo_p, *demotime_p;
static UINT8 *demoend;
static UINT8 demoflags;
static UINT16 demoversion;
boolean singledemo; // quit after playing a demo from cmdline
boolean demo_start; // don't start playing demo right away
boolean demosynced = true; // console warning message

boolean metalrecording; // recording as metal sonic
mobj_t *metalplayback;
static UINT8 *metalbuffer = NULL;
static UINT8 *metal_p;
static UINT16 metalversion;

// extra data stuff (events registered this frame while recording)
static struct {
	UINT8 flags; // EZT flags

	// EZT_COLOR
	UINT16 color, lastcolor;

	// EZT_SCALE
	fixed_t scale, lastscale;

	// EZT_HIT
	UINT16 hits;
	mobj_t **hitlist;
} ghostext;

// Your naming conventions are stupid and useless.
// There is no conflict here.
typedef struct demoghost {
	UINT8 checksum[16];
	UINT8 *buffer, *p, fadein;
	UINT16 color;
	UINT16 version;
	mobj_t oldmo, *mo;
	struct demoghost *next;
} demoghost;
demoghost *ghosts = NULL;

//
// DEMO RECORDING
//

#define DEMOVERSION 0x000d
#define DEMOHEADER  "\xF0" "SRB2Replay" "\x0F"

#define DF_GHOST        0x01 // This demo contains ghost data too!
#define DF_RECORDATTACK 0x02 // This demo is from record attack and contains its final completion time, score, and rings!
#define DF_NIGHTSATTACK 0x04 // This demo is from NiGHTS attack and contains its time left, score, and mares!
#define DF_ATTACKMASK   0x06 // This demo is from ??? attack and contains ???
#define DF_ATTACKSHIFT  1

// For demos
#define ZT_FWD     0x01
#define ZT_SIDE    0x02
#define ZT_ANGLE   0x04
#define ZT_BUTTONS 0x08
#define ZT_AIMING  0x10
#define DEMOMARKER 0x80 // demoend
#define METALDEATH 0x44
#define METALSNICE 0x69

static ticcmd_t oldcmd;

// For Metal Sonic and time attack ghosts
#define GZT_XYZ    0x01
#define GZT_MOMXY  0x02
#define GZT_MOMZ   0x04
#define GZT_ANGLE  0x08
#define GZT_FRAME  0x10 // Animation frame
#define GZT_SPR2   0x20 // Player animations
#define GZT_EXTRA  0x40
#define GZT_FOLLOW 0x80 // Followmobj

// GZT_EXTRA flags
#define EZT_THOK   0x01 // Spawned a thok object
#define EZT_SPIN   0x02 // Because one type of thok object apparently wasn't enough
#define EZT_REV    0x03 // And two types wasn't enough either yet
#define EZT_THOKMASK 0x03
#define EZT_COLOR  0x04 // Changed color (Super transformation, Mario fireflowers/invulnerability, etc.)
#define EZT_FLIP   0x08 // Reversed gravity
#define EZT_SCALE  0x10 // Changed size
#define EZT_HIT    0x20 // Damaged a mobj
#define EZT_SPRITE 0x40 // Changed sprite set completely out of PLAY (NiGHTS, SOCs, whatever)
#define EZT_HEIGHT 0x80 // Changed height

// GZT_FOLLOW flags
#define FZT_SPAWNED 0x01 // just been spawned
#define FZT_SKIN 0x02 // has skin
#define FZT_LINKDRAW 0x04 // has linkdraw (combine with spawned only)
#define FZT_COLORIZED 0x08 // colorized (ditto)
#define FZT_SCALE 0x10 // different scale to object
// spare FZT slots 0x20 to 0x80

static mobj_t oldmetal, oldghost;

void G_SaveMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	WRITEUINT32(*buffer, metal_p - metalbuffer);
}

void G_LoadMetal(UINT8 **buffer)
{
	I_Assert(buffer != NULL && *buffer != NULL);

	G_DoPlayMetal();
	metal_p = metalbuffer + READUINT32(*buffer);
}


void G_ReadDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	UINT8 ziptic;

	if (!demo_p || !demo_start)
		return;
	ziptic = READUINT8(demo_p);

	if (ziptic & ZT_FWD)
		oldcmd.forwardmove = READSINT8(demo_p);
	if (ziptic & ZT_SIDE)
		oldcmd.sidemove = READSINT8(demo_p);
	if (ziptic & ZT_ANGLE)
		oldcmd.angleturn = READINT16(demo_p);
	if (ziptic & ZT_BUTTONS)
		oldcmd.buttons = (oldcmd.buttons & (BT_CAMLEFT|BT_CAMRIGHT)) | (READUINT16(demo_p) & ~(BT_CAMLEFT|BT_CAMRIGHT));
	if (ziptic & ZT_AIMING)
		oldcmd.aiming = READINT16(demo_p);

	G_CopyTiccmd(cmd, &oldcmd, 1);
	players[playernum].angleturn = cmd->angleturn;

	if (!(demoflags & DF_GHOST) && *demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_WriteDemoTiccmd(ticcmd_t *cmd, INT32 playernum)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	(void)playernum;

	if (!demo_p)
		return;
	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	if (cmd->forwardmove != oldcmd.forwardmove)
	{
		WRITEUINT8(demo_p,cmd->forwardmove);
		oldcmd.forwardmove = cmd->forwardmove;
		ziptic |= ZT_FWD;
	}

	if (cmd->sidemove != oldcmd.sidemove)
	{
		WRITEUINT8(demo_p,cmd->sidemove);
		oldcmd.sidemove = cmd->sidemove;
		ziptic |= ZT_SIDE;
	}

	if (cmd->angleturn != oldcmd.angleturn)
	{
		WRITEINT16(demo_p,cmd->angleturn);
		oldcmd.angleturn = cmd->angleturn;
		ziptic |= ZT_ANGLE;
	}

	if (cmd->buttons != oldcmd.buttons)
	{
		WRITEUINT16(demo_p,cmd->buttons);
		oldcmd.buttons = cmd->buttons;
		ziptic |= ZT_BUTTONS;
	}

	if (cmd->aiming != oldcmd.aiming)
	{
		WRITEINT16(demo_p,cmd->aiming);
		oldcmd.aiming = cmd->aiming;
		ziptic |= ZT_AIMING;
	}

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (!(demoflags & DF_GHOST) && ziptic_p > demoend - 9)
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

void G_GhostAddThok(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_THOK;
}

void G_GhostAddSpin(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_SPIN;
}

void G_GhostAddRev(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags = (ghostext.flags & ~EZT_THOKMASK) | EZT_REV;
}

void G_GhostAddFlip(void)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	ghostext.flags |= EZT_FLIP;
}

void G_GhostAddColor(ghostcolor_t color)
{
	if (!demorecording || !(demoflags & DF_GHOST))
		return;
	if (ghostext.lastcolor == (UINT16)color)
	{
		ghostext.flags &= ~EZT_COLOR;
		return;
	}
	ghostext.flags |= EZT_COLOR;
	ghostext.color = (UINT16)color;
}

void G_GhostAddScale(fixed_t scale)
{
	if (!metalrecording && (!demorecording || !(demoflags & DF_GHOST)))
		return;
	if (ghostext.lastscale == scale)
	{
		ghostext.flags &= ~EZT_SCALE;
		return;
	}
	ghostext.flags |= EZT_SCALE;
	ghostext.scale = scale;
}

void G_GhostAddHit(mobj_t *victim)
{
	if (!demorecording || !(demoflags & DF_GHOST))
		return;
	ghostext.flags |= EZT_HIT;
	ghostext.hits++;
	ghostext.hitlist = Z_Realloc(ghostext.hitlist, ghostext.hits * sizeof(mobj_t *), PU_LEVEL, NULL);
	ghostext.hitlist[ghostext.hits-1] = victim;
}

void G_WriteGhostTic(mobj_t *ghost)
{
	char ziptic = 0;
	UINT8 *ziptic_p;
	UINT32 i;
	fixed_t height;

	if (!demo_p)
		return;
	if (!(demoflags & DF_GHOST))
		return; // No ghost data to write.

	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	#define MAXMOM (0xFFFF<<8)

	// GZT_XYZ is only useful if you've moved 256 FRACUNITS or more in a single tic.
	if (abs(ghost->x-oldghost.x) > MAXMOM
	|| abs(ghost->y-oldghost.y) > MAXMOM
	|| abs(ghost->z-oldghost.z) > MAXMOM)
	{
		oldghost.x = ghost->x;
		oldghost.y = ghost->y;
		oldghost.z = ghost->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldghost.x);
		WRITEFIXED(demo_p,oldghost.y);
		WRITEFIXED(demo_p,oldghost.z);
	}
	else
	{
		// For moving normally:
		// Store one full byte of movement, plus one byte of fractional movement.
		INT16 momx = (INT16)((ghost->x-oldghost.x)>>8);
		INT16 momy = (INT16)((ghost->y-oldghost.y)>>8);
		if (momx != oldghost.momx
		|| momy != oldghost.momy)
		{
			oldghost.momx = momx;
			oldghost.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEINT16(demo_p,momx);
			WRITEINT16(demo_p,momy);
		}
		momx = (INT16)((ghost->z-oldghost.z)>>8);
		if (momx != oldghost.momz)
		{
			oldghost.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEINT16(demo_p,momx);
		}

		// This SHOULD set oldghost.x/y/z to match ghost->x/y/z
		// but it keeps the fractional loss of one byte,
		// so it will hopefully be made up for in future tics.
		oldghost.x += oldghost.momx<<8;
		oldghost.y += oldghost.momy<<8;
		oldghost.z += oldghost.momz<<8;
	}

	#undef MAXMOM

	// Only store the 8 most relevant bits of angle
	// because exact values aren't too easy to discern to begin with when only 8 angles have different sprites
	// and it does not affect this mode of movement at all anyway.
	if (ghost->player && ghost->player->drawangle>>24 != oldghost.angle)
	{
		oldghost.angle = ghost->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldghost.angle);
	}

	// Store the sprite frame.
	if ((ghost->frame & FF_FRAMEMASK) != oldghost.frame)
	{
		oldghost.frame = (ghost->frame & FF_FRAMEMASK);
		ziptic |= GZT_FRAME;
		WRITEUINT8(demo_p,oldghost.frame);
	}

	if (ghost->sprite == SPR_PLAY
	&& ghost->sprite2 != oldghost.sprite2)
	{
		oldghost.sprite2 = ghost->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demo_p,oldghost.sprite2);
	}

	// Check for sprite set changes
	if (ghost->sprite != oldghost.sprite)
	{
		oldghost.sprite = ghost->sprite;
		ghostext.flags |= EZT_SPRITE;
	}

	if ((height = FixedDiv(ghost->height, ghost->scale)) != oldghost.height)
	{
		oldghost.height = height;
		ghostext.flags |= EZT_HEIGHT;
	}

	if (ghostext.flags)
	{
		ziptic |= GZT_EXTRA;

		if (ghostext.color == ghostext.lastcolor)
			ghostext.flags &= ~EZT_COLOR;
		if (ghostext.scale == ghostext.lastscale)
			ghostext.flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext.flags);
		if (ghostext.flags & EZT_COLOR)
		{
			WRITEUINT16(demo_p,ghostext.color);
			ghostext.lastcolor = ghostext.color;
		}
		if (ghostext.flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext.scale);
			ghostext.lastscale = ghostext.scale;
		}
		if (ghostext.flags & EZT_HIT)
		{
			WRITEUINT16(demo_p,ghostext.hits);
			for (i = 0; i < ghostext.hits; i++)
			{
				mobj_t *mo = ghostext.hitlist[i];
				//WRITEUINT32(demo_p,UINT32_MAX); // reserved for some method of determining exactly which mobj this is. (mobjnum doesn't work here.)
				WRITEUINT32(demo_p,mo->type);
				WRITEUINT16(demo_p,(UINT16)mo->health);
				WRITEFIXED(demo_p,mo->x);
				WRITEFIXED(demo_p,mo->y);
				WRITEFIXED(demo_p,mo->z);
				WRITEANGLE(demo_p,mo->angle);
			}
			Z_Free(ghostext.hitlist);
			ghostext.hits = 0;
			ghostext.hitlist = NULL;
		}
		if (ghostext.flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldghost.sprite);
		if (ghostext.flags & EZT_HEIGHT)
		{
			height >>= FRACBITS;
			WRITEINT16(demo_p, height);
		}
		ghostext.flags = 0;
	}

	if (ghost->player && ghost->player->followmobj && !(ghost->player->followmobj->sprite == SPR_NULL || (ghost->player->followmobj->flags2 & MF2_DONTDRAW))) // bloats tails runs but what can ya do
	{
		INT16 temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (ghost->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldghost.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,ghost->player->followmobj->info->height>>FRACBITS);
			if (ghost->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (ghost->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)(ghost->player->followmobj->skin))-skins));
			oldghost.flags2 |= MF2_AMBUSH;
		}

		if (ghost->player->followmobj->scale != ghost->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,ghost->player->followmobj->scale);
		}

		temp = (INT16)((ghost->player->followmobj->x-ghost->x)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((ghost->player->followmobj->y-ghost->y)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((ghost->player->followmobj->z-ghost->z)>>8);
		WRITEINT16(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demo_p,ghost->player->followmobj->sprite2);
		WRITEUINT16(demo_p,ghost->player->followmobj->sprite);
		WRITEUINT8(demo_p,(ghost->player->followmobj->frame & FF_FRAMEMASK));
		WRITEUINT16(demo_p,ghost->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldghost.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (demo_p >= demoend - (13 + 9 + 9))
	{
		G_CheckDemoStatus(); // no more space
		return;
	}
}

// Uses ghost data to do consistency checks on your position.
// This fixes desynchronising demos when fighting eggman.
void G_ConsGhostTic(void)
{
	UINT8 ziptic;
	UINT16 px,py,pz,gx,gy,gz;
	mobj_t *testmo;

	if (!demo_p || !demo_start)
		return;
	if (!(demoflags & DF_GHOST))
		return; // No ghost data to use.

	testmo = players[0].mo;

	// Grab ghost data.
	ziptic = READUINT8(demo_p);
	if (ziptic & GZT_XYZ)
	{
		oldghost.x = READFIXED(demo_p);
		oldghost.y = READFIXED(demo_p);
		oldghost.z = READFIXED(demo_p);
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldghost.momx = READINT16(demo_p)<<8;
			oldghost.momy = READINT16(demo_p)<<8;
		}
		if (ziptic & GZT_MOMZ)
			oldghost.momz = READINT16(demo_p)<<8;
		oldghost.x += oldghost.momx;
		oldghost.y += oldghost.momy;
		oldghost.z += oldghost.momz;
	}
	if (ziptic & GZT_ANGLE)
		demo_p++;
	if (ziptic & GZT_FRAME)
		demo_p++;
	if (ziptic & GZT_SPR2)
		demo_p++;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		UINT8 xziptic = READUINT8(demo_p);
		if (xziptic & EZT_COLOR)
			demo_p += (demoversion==0x000c) ? 1 : sizeof(UINT16);
		if (xziptic & EZT_SCALE)
			demo_p += sizeof(fixed_t);
		if (xziptic & EZT_HIT)
		{ // Resync mob damage.
			UINT16 i, count = READUINT16(demo_p);
			thinker_t *th;
			mobj_t *mobj;

			UINT32 type;
			UINT16 health;
			fixed_t x;
			fixed_t y;
			fixed_t z;

			for (i = 0; i < count; i++)
			{
				//demo_p += 4; // reserved.
				type = READUINT32(demo_p);
				health = READUINT16(demo_p);
				x = READFIXED(demo_p);
				y = READFIXED(demo_p);
				z = READFIXED(demo_p);
				demo_p += sizeof(angle_t); // angle, unnecessary for cons.

				mobj = NULL;
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;
					mobj = (mobj_t *)th;
					if (mobj->type == (mobjtype_t)type && mobj->x == x && mobj->y == y && mobj->z == z)
						break;
				}
				if (th != &thlist[THINK_MOBJ] && mobj->health != health) // Wasn't damaged?! This is desync! Fix it!
				{
					if (demosynced)
						CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced!\n"));
					demosynced = false;
					P_DamageMobj(mobj, players[0].mo, players[0].mo, 1, 0);
				}
			}
		}
		if (xziptic & EZT_SPRITE)
			demo_p += sizeof(UINT16);
		if (xziptic & EZT_HEIGHT)
			demo_p += sizeof(INT16);
	}

	if (ziptic & GZT_FOLLOW)
	{ // Even more...
		UINT8 followtic = READUINT8(demo_p);
		if (followtic & FZT_SPAWNED)
		{
			demo_p += sizeof(INT16);
			if (followtic & FZT_SKIN)
				demo_p++;
		}
		if (followtic & FZT_SCALE)
			demo_p += sizeof(fixed_t);
		demo_p += sizeof(INT16);
		demo_p += sizeof(INT16);
		demo_p += sizeof(INT16);
		if (followtic & FZT_SKIN)
			demo_p++;
		demo_p += sizeof(UINT16);
		demo_p++;
		demo_p += (demoversion==0x000c) ? 1 : sizeof(UINT16);
	}

	// Re-synchronise
	px = testmo->x>>FRACBITS;
	py = testmo->y>>FRACBITS;
	pz = testmo->z>>FRACBITS;
	gx = oldghost.x>>FRACBITS;
	gy = oldghost.y>>FRACBITS;
	gz = oldghost.z>>FRACBITS;

	if (px != gx || py != gy || pz != gz)
	{
		if (demosynced)
			CONS_Alert(CONS_WARNING, M_GetText("Demo playback has desynced!\n"));
		demosynced = false;

		P_UnsetThingPosition(testmo);
		testmo->x = oldghost.x;
		testmo->y = oldghost.y;
		P_SetThingPosition(testmo);
		testmo->z = oldghost.z;
	}

	if (*demo_p == DEMOMARKER)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}
}

void G_GhostTicker(void)
{
	demoghost *g,*p;
	for(g = ghosts, p = NULL; g; g = g->next)
	{
		// Skip normal demo data.
		UINT8 ziptic = READUINT8(g->p);
		UINT8 xziptic = 0;
		if (ziptic & ZT_FWD)
			g->p++;
		if (ziptic & ZT_SIDE)
			g->p++;
		if (ziptic & ZT_ANGLE)
			g->p += 2;
		if (ziptic & ZT_BUTTONS)
			g->p += 2;
		if (ziptic & ZT_AIMING)
			g->p += 2;

		// Grab ghost data.
		ziptic = READUINT8(g->p);
		if (ziptic & GZT_XYZ)
		{
			g->oldmo.x = READFIXED(g->p);
			g->oldmo.y = READFIXED(g->p);
			g->oldmo.z = READFIXED(g->p);
		}
		else
		{
			if (ziptic & GZT_MOMXY)
			{
				g->oldmo.momx = READINT16(g->p)<<8;
				g->oldmo.momy = READINT16(g->p)<<8;
			}
			if (ziptic & GZT_MOMZ)
				g->oldmo.momz = READINT16(g->p)<<8;
			g->oldmo.x += g->oldmo.momx;
			g->oldmo.y += g->oldmo.momy;
			g->oldmo.z += g->oldmo.momz;
		}
		if (ziptic & GZT_ANGLE)
			g->mo->angle = READUINT8(g->p)<<24;
		if (ziptic & GZT_FRAME)
			g->oldmo.frame = READUINT8(g->p);
		if (ziptic & GZT_SPR2)
			g->oldmo.sprite2 = READUINT8(g->p);

		// Update ghost
		P_UnsetThingPosition(g->mo);
		g->mo->x = g->oldmo.x;
		g->mo->y = g->oldmo.y;
		g->mo->z = g->oldmo.z;
		P_SetThingPosition(g->mo);
		g->mo->frame = g->oldmo.frame | tr_trans30<<FF_TRANSSHIFT;
		if (g->fadein)
		{
			g->mo->frame += (((--g->fadein)/6)<<FF_TRANSSHIFT); // this calc never exceeds 9 unless g->fadein is bad, and it's only set once, so...
			g->mo->flags2 &= ~MF2_DONTDRAW;
		}
		g->mo->sprite2 = g->oldmo.sprite2;

		if (ziptic & GZT_EXTRA)
		{ // But wait, there's more!
			xziptic = READUINT8(g->p);
			if (xziptic & EZT_COLOR)
			{
				g->color = (g->version==0x000c) ? READUINT8(g->p) : READUINT16(g->p);
				switch(g->color)
				{
				default:
				case GHC_RETURNSKIN:
					g->mo->skin = g->oldmo.skin;
					/* FALLTHRU */
				case GHC_NORMAL: // Go back to skin color
					g->mo->color = g->oldmo.color;
					break;
				// Handled below
				case GHC_SUPER:
				case GHC_INVINCIBLE:
					break;
				case GHC_FIREFLOWER: // Fireflower
					g->mo->color = SKINCOLOR_WHITE;
					break;
				case GHC_NIGHTSSKIN: // not actually a colour
					g->mo->skin = &skins[DEFAULTNIGHTSSKIN];
					break;
				}
			}
			if (xziptic & EZT_FLIP)
				g->mo->eflags ^= MFE_VERTICALFLIP;
			if (xziptic & EZT_SCALE)
			{
				g->mo->destscale = READFIXED(g->p);
				if (g->mo->destscale != g->mo->scale)
					P_SetScale(g->mo, g->mo->destscale);
			}
			if (xziptic & EZT_THOKMASK)
			{ // Let's only spawn ONE of these per frame, thanks.
				mobj_t *mobj;
				UINT32 type = MT_NULL;
				if (g->mo->skin)
				{
					skin_t *skin = (skin_t *)g->mo->skin;
					switch (xziptic & EZT_THOKMASK)
					{
					case EZT_THOK:
						type = skin->thokitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].painchance : (UINT32)skin->thokitem;
						break;
					case EZT_SPIN:
						type = skin->spinitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].damage : (UINT32)skin->spinitem;
						break;
					case EZT_REV:
						type = skin->revitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].raisestate : (UINT32)skin->revitem;
						break;
					}
				}
				if (type != MT_NULL)
				{
					if (type == MT_GHOST)
					{
						mobj = P_SpawnGhostMobj(g->mo); // does a large portion of the work for us
						mobj->frame = (mobj->frame & ~FF_FRAMEMASK)|tr_trans60<<FF_TRANSSHIFT; // P_SpawnGhostMobj sets trans50, we want trans60
					}
					else
					{
						mobj = P_SpawnMobjFromMobj(g->mo, 0, 0, -FixedDiv(FixedMul(g->mo->info->height, g->mo->scale) - g->mo->height,3*FRACUNIT), MT_THOK);
						mobj->sprite = states[mobjinfo[type].spawnstate].sprite;
						mobj->frame = (states[mobjinfo[type].spawnstate].frame & FF_FRAMEMASK) | tr_trans60<<FF_TRANSSHIFT;
						mobj->color = g->mo->color;
						mobj->skin = g->mo->skin;
						P_SetScale(mobj, (mobj->destscale = g->mo->scale));

						if (type == MT_THOK) // spintrail-specific modification for MT_THOK
						{
							mobj->frame = FF_TRANS80;
							mobj->fuse = mobj->tics;
						}
						mobj->tics = -1; // nope.
					}
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
					P_UnsetThingPosition(mobj);
					mobj->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
					P_SetThingPosition(mobj);
					if (!mobj->fuse)
						mobj->fuse = 8;
					P_SetTarget(&mobj->target, g->mo);
				}
			}
			if (xziptic & EZT_HIT)
			{ // Spawn hit poofs for killing things!
				UINT16 i, count = READUINT16(g->p), health;
				UINT32 type;
				fixed_t x,y,z;
				angle_t angle;
				mobj_t *poof;
				for (i = 0; i < count; i++)
				{
					//g->p += 4; // reserved
					type = READUINT32(g->p);
					health = READUINT16(g->p);
					x = READFIXED(g->p);
					y = READFIXED(g->p);
					z = READFIXED(g->p);
					angle = READANGLE(g->p);
					if (!(mobjinfo[type].flags & MF_SHOOTABLE)
					|| !(mobjinfo[type].flags & (MF_ENEMY|MF_MONITOR))
					|| health != 0 || i >= 4) // only spawn for the first 4 hits per frame, to prevent ghosts from splode-spamming too bad.
						continue;
					poof = P_SpawnMobj(x, y, z, MT_GHOST);
					poof->angle = angle;
					poof->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
					poof->health = 0;
					P_SetMobjStateNF(poof, S_XPLD1);
				}
			}
			if (xziptic & EZT_SPRITE)
				g->mo->sprite = READUINT16(g->p);
			if (xziptic & EZT_HEIGHT)
			{
				fixed_t temp = READINT16(g->p)<<FRACBITS;
				g->mo->height = FixedMul(temp, g->mo->scale);
			}
		}

		// Tick ghost colors (Super and Mario Invincibility flashing)
		switch(g->color)
		{
		case GHC_SUPER: // Super (P_DoSuperStuff)
			if (g->mo->skin)
			{
				skin_t *skin = (skin_t *)g->mo->skin;
				g->mo->color = skin->supercolor;
			}
			else
				g->mo->color = SKINCOLOR_SUPERGOLD1;
			g->mo->color += abs( ( (signed)( (unsigned)leveltime >> 1 ) % 9) - 4);
			break;
		case GHC_INVINCIBLE: // Mario invincibility (P_CheckInvincibilityTimer)
			g->mo->color = (UINT16)(SKINCOLOR_RUBY + (leveltime % (FIRSTSUPERCOLOR - SKINCOLOR_RUBY))); // Passes through all saturated colours
			break;
		default:
			break;
		}

#define follow g->mo->tracer
		if (ziptic & GZT_FOLLOW)
		{ // Even more...
			UINT8 followtic = READUINT8(g->p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(g->mo, 0, 0, 0, MT_GHOST));
				P_SetTarget(&follow->tracer, g->mo);
				follow->tics = -1;
				temp = READINT16(g->p)<<FRACBITS;
				follow->height = FixedMul(follow->scale, temp);

				if (followtic & FZT_LINKDRAW)
					follow->flags2 |= MF2_LINKDRAW;

				if (followtic & FZT_COLORIZED)
					follow->colorized = true;

				if (followtic & FZT_SKIN)
					follow->skin = &skins[READUINT8(g->p)];
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(g->p);
				else
					follow->destscale = g->mo->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale);

				P_UnsetThingPosition(follow);
				temp = READINT16(g->p)<<8;
				follow->x = g->mo->x + temp;
				temp = READINT16(g->p)<<8;
				follow->y = g->mo->y + temp;
				temp = READINT16(g->p)<<8;
				follow->z = g->mo->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = READUINT8(g->p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(g->p);
				follow->frame = (READUINT8(g->p)) | (g->mo->frame & FF_TRANSMASK);
				follow->angle = g->mo->angle;
				follow->color = (g->version==0x000c) ? READUINT8(g->p) : READUINT16(g->p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}
		// Demo ends after ghost data.
		if (*g->p == DEMOMARKER)
		{
			g->mo->momx = g->mo->momy = g->mo->momz = 0;
#if 1 // freeze frame (maybe more useful for time attackers)
			g->mo->colorized = true;
			if (follow)
				follow->colorized = true;
#else // dissapearing act
			g->mo->fuse = TICRATE;
			if (follow)
				follow->fuse = TICRATE;
#endif
			if (p)
				p->next = g->next;
			else
				ghosts = g->next;
			Z_Free(g);
			continue;
		}
		p = g;
#undef follow
	}
}

void G_ReadMetalTic(mobj_t *metal)
{
	UINT8 ziptic;
	UINT8 xziptic = 0;

	if (!metal_p)
		return;

	if (!metal->health)
	{
		G_StopMetalDemo();
		return;
	}

	switch (*metal_p)
	{
		case METALSNICE:
			break;
		case METALDEATH:
			if (metal->tracer)
				P_RemoveMobj(metal->tracer);
			P_KillMobj(metal, NULL, NULL, 0);
			/* FALLTHRU */
		case DEMOMARKER:
		default:
			// end of demo data stream
			G_StopMetalDemo();
			return;
	}
	metal_p++;

	ziptic = READUINT8(metal_p);

	// Read changes from the tic
	if (ziptic & GZT_XYZ)
	{
		// make sure the values are read in the right order
		oldmetal.x = READFIXED(metal_p);
		oldmetal.y = READFIXED(metal_p);
		oldmetal.z = READFIXED(metal_p);
		P_TeleportMove(metal, oldmetal.x, oldmetal.y, oldmetal.z);
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
	}
	else
	{
		if (ziptic & GZT_MOMXY)
		{
			oldmetal.momx = READINT16(metal_p)<<8;
			oldmetal.momy = READINT16(metal_p)<<8;
		}
		if (ziptic & GZT_MOMZ)
			oldmetal.momz = READINT16(metal_p)<<8;
		oldmetal.x += oldmetal.momx;
		oldmetal.y += oldmetal.momy;
		oldmetal.z += oldmetal.momz;
	}
	if (ziptic & GZT_ANGLE)
		metal->angle = READUINT8(metal_p)<<24;
	if (ziptic & GZT_FRAME)
		oldmetal.frame = READUINT32(metal_p);
	if (ziptic & GZT_SPR2)
		oldmetal.sprite2 = READUINT8(metal_p);

	// Set movement, position, and angle
	// oldmetal contains where you're supposed to be.
	metal->momx = oldmetal.momx;
	metal->momy = oldmetal.momy;
	metal->momz = oldmetal.momz;
	P_UnsetThingPosition(metal);
	metal->x = oldmetal.x;
	metal->y = oldmetal.y;
	metal->z = oldmetal.z;
	P_SetThingPosition(metal);
	metal->frame = oldmetal.frame;
	metal->sprite2 = oldmetal.sprite2;

	if (ziptic & GZT_EXTRA)
	{ // But wait, there's more!
		xziptic = READUINT8(metal_p);
		if (xziptic & EZT_FLIP)
		{
			metal->eflags ^= MFE_VERTICALFLIP;
			metal->flags2 ^= MF2_OBJECTFLIP;
		}
		if (xziptic & EZT_SCALE)
		{
			metal->destscale = READFIXED(metal_p);
			if (metal->destscale != metal->scale)
				P_SetScale(metal, metal->destscale);
		}
		if (xziptic & EZT_THOKMASK)
		{ // Let's only spawn ONE of these per frame, thanks.
			mobj_t *mobj;
			UINT32 type = MT_NULL;
			if (metal->skin)
			{
				skin_t *skin = (skin_t *)metal->skin;
				switch (xziptic & EZT_THOKMASK)
				{
				case EZT_THOK:
					type = skin->thokitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].painchance : (UINT32)skin->thokitem;
					break;
				case EZT_SPIN:
					type = skin->spinitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].damage : (UINT32)skin->spinitem;
					break;
				case EZT_REV:
					type = skin->revitem < 0 ? (UINT32)mobjinfo[MT_PLAYER].raisestate : (UINT32)skin->revitem;
					break;
				}
			}
			if (type != MT_NULL)
			{
				if (type == MT_GHOST)
				{
					mobj = P_SpawnGhostMobj(metal); // does a large portion of the work for us
				}
				else
				{
					mobj = P_SpawnMobjFromMobj(metal, 0, 0, -FixedDiv(FixedMul(metal->info->height, metal->scale) - metal->height,3*FRACUNIT), MT_THOK);
					mobj->sprite = states[mobjinfo[type].spawnstate].sprite;
					mobj->frame = states[mobjinfo[type].spawnstate].frame;
					mobj->angle = metal->angle;
					mobj->color = metal->color;
					mobj->skin = metal->skin;
					P_SetScale(mobj, (mobj->destscale = metal->scale));

					if (type == MT_THOK) // spintrail-specific modification for MT_THOK
					{
						mobj->frame = FF_TRANS70;
						mobj->fuse = mobj->tics;
					}
					mobj->tics = -1; // nope.
				}
				mobj->floorz = mobj->z;
				mobj->ceilingz = mobj->z+mobj->height;
				P_UnsetThingPosition(mobj);
				mobj->flags = MF_NOBLOCKMAP|MF_NOCLIP|MF_NOCLIPHEIGHT|MF_NOGRAVITY; // make an ATTEMPT to curb crazy SOCs fucking stuff up...
				P_SetThingPosition(mobj);
				if (!mobj->fuse)
					mobj->fuse = 8;
				P_SetTarget(&mobj->target, metal);
			}
		}
		if (xziptic & EZT_SPRITE)
			metal->sprite = READUINT16(metal_p);
		if (xziptic & EZT_HEIGHT)
		{
			fixed_t temp = READINT16(metal_p)<<FRACBITS;
			metal->height = FixedMul(temp, metal->scale);
		}
	}

#define follow metal->tracer
		if (ziptic & GZT_FOLLOW)
		{ // Even more...
			UINT8 followtic = READUINT8(metal_p);
			fixed_t temp;
			if (followtic & FZT_SPAWNED)
			{
				if (follow)
					P_RemoveMobj(follow);
				P_SetTarget(&follow, P_SpawnMobjFromMobj(metal, 0, 0, 0, MT_GHOST));
				P_SetTarget(&follow->tracer, metal);
				follow->tics = -1;
				temp = READINT16(metal_p)<<FRACBITS;
				follow->height = FixedMul(follow->scale, temp);

				if (followtic & FZT_LINKDRAW)
					follow->flags2 |= MF2_LINKDRAW;

				if (followtic & FZT_COLORIZED)
					follow->colorized = true;

				if (followtic & FZT_SKIN)
					follow->skin = &skins[READUINT8(metal_p)];
			}
			if (follow)
			{
				if (followtic & FZT_SCALE)
					follow->destscale = READFIXED(metal_p);
				else
					follow->destscale = metal->destscale;
				if (follow->destscale != follow->scale)
					P_SetScale(follow, follow->destscale);

				P_UnsetThingPosition(follow);
				temp = READINT16(metal_p)<<8;
				follow->x = metal->x + temp;
				temp = READINT16(metal_p)<<8;
				follow->y = metal->y + temp;
				temp = READINT16(metal_p)<<8;
				follow->z = metal->z + temp;
				P_SetThingPosition(follow);
				if (followtic & FZT_SKIN)
					follow->sprite2 = READUINT8(metal_p);
				else
					follow->sprite2 = 0;
				follow->sprite = READUINT16(metal_p);
				follow->frame = READUINT32(metal_p); // NOT & FF_FRAMEMASK here, so 32 bits
				follow->angle = metal->angle;
				follow->color = (metalversion==0x000c) ? READUINT8(metal_p) : READUINT16(metal_p);

				if (!(followtic & FZT_SPAWNED))
				{
					if (xziptic & EZT_FLIP)
					{
						follow->flags2 ^= MF2_OBJECTFLIP;
						follow->eflags ^= MFE_VERTICALFLIP;
					}
				}
			}
		}
		else if (follow)
		{
			P_RemoveMobj(follow);
			P_SetTarget(&follow, NULL);
		}
#undef follow
}

void G_WriteMetalTic(mobj_t *metal)
{
	UINT8 ziptic = 0;
	UINT8 *ziptic_p;
	fixed_t height;

	if (!demo_p) // demo_p will be NULL until the race start linedef executor is activated!
		return;

	WRITEUINT8(demo_p, METALSNICE);
	ziptic_p = demo_p++; // the ziptic, written at the end of this function

	#define MAXMOM (0xFFFF<<8)

	// GZT_XYZ is only useful if you've moved 256 FRACUNITS or more in a single tic.
	if (abs(metal->x-oldmetal.x) > MAXMOM
	|| abs(metal->y-oldmetal.y) > MAXMOM
	|| abs(metal->z-oldmetal.z) > MAXMOM)
	{
		oldmetal.x = metal->x;
		oldmetal.y = metal->y;
		oldmetal.z = metal->z;
		ziptic |= GZT_XYZ;
		WRITEFIXED(demo_p,oldmetal.x);
		WRITEFIXED(demo_p,oldmetal.y);
		WRITEFIXED(demo_p,oldmetal.z);
	}
	else
	{
		// For moving normally:
		// Store one full byte of movement, plus one byte of fractional movement.
		INT16 momx = (INT16)((metal->x-oldmetal.x)>>8);
		INT16 momy = (INT16)((metal->y-oldmetal.y)>>8);
		if (momx != oldmetal.momx
		|| momy != oldmetal.momy)
		{
			oldmetal.momx = momx;
			oldmetal.momy = momy;
			ziptic |= GZT_MOMXY;
			WRITEINT16(demo_p,momx);
			WRITEINT16(demo_p,momy);
		}
		momx = (INT16)((metal->z-oldmetal.z)>>8);
		if (momx != oldmetal.momz)
		{
			oldmetal.momz = momx;
			ziptic |= GZT_MOMZ;
			WRITEINT16(demo_p,momx);
		}

		// This SHOULD set oldmetal.x/y/z to match metal->x/y/z
		// but it keeps the fractional loss of one byte,
		// so it will hopefully be made up for in future tics.
		oldmetal.x += oldmetal.momx<<8;
		oldmetal.y += oldmetal.momy<<8;
		oldmetal.z += oldmetal.momz<<8;
	}

	#undef MAXMOM

	// Only store the 8 most relevant bits of angle
	// because exact values aren't too easy to discern to begin with when only 8 angles have different sprites
	// and it does not affect movement at all anyway.
	if (metal->player && metal->player->drawangle>>24 != oldmetal.angle)
	{
		oldmetal.angle = metal->player->drawangle>>24;
		ziptic |= GZT_ANGLE;
		WRITEUINT8(demo_p,oldmetal.angle);
	}

	// Store the sprite frame.
	if ((metal->frame & FF_FRAMEMASK) != oldmetal.frame)
	{
		oldmetal.frame = metal->frame; // NOT & FF_FRAMEMASK here, so 32 bits
		ziptic |= GZT_FRAME;
		WRITEUINT32(demo_p,oldmetal.frame);
	}

	if (metal->sprite == SPR_PLAY
	&& metal->sprite2 != oldmetal.sprite2)
	{
		oldmetal.sprite2 = metal->sprite2;
		ziptic |= GZT_SPR2;
		WRITEUINT8(demo_p,oldmetal.sprite2);
	}

	// Check for sprite set changes
	if (metal->sprite != oldmetal.sprite)
	{
		oldmetal.sprite = metal->sprite;
		ghostext.flags |= EZT_SPRITE;
	}

	if ((height = FixedDiv(metal->height, metal->scale)) != oldmetal.height)
	{
		oldmetal.height = height;
		ghostext.flags |= EZT_HEIGHT;
	}

	if (ghostext.flags & ~(EZT_COLOR|EZT_HIT)) // these two aren't handled by metal ever
	{
		ziptic |= GZT_EXTRA;

		if (ghostext.scale == ghostext.lastscale)
			ghostext.flags &= ~EZT_SCALE;

		WRITEUINT8(demo_p,ghostext.flags);
		if (ghostext.flags & EZT_SCALE)
		{
			WRITEFIXED(demo_p,ghostext.scale);
			ghostext.lastscale = ghostext.scale;
		}
		if (ghostext.flags & EZT_SPRITE)
			WRITEUINT16(demo_p,oldmetal.sprite);
		if (ghostext.flags & EZT_HEIGHT)
		{
			height >>= FRACBITS;
			WRITEINT16(demo_p, height);
		}
		ghostext.flags = 0;
	}

	if (metal->player && metal->player->followmobj && !(metal->player->followmobj->sprite == SPR_NULL || (metal->player->followmobj->flags2 & MF2_DONTDRAW)))
	{
		INT16 temp;
		UINT8 *followtic_p = demo_p++;
		UINT8 followtic = 0;

		ziptic |= GZT_FOLLOW;

		if (metal->player->followmobj->skin)
			followtic |= FZT_SKIN;

		if (!(oldmetal.flags2 & MF2_AMBUSH))
		{
			followtic |= FZT_SPAWNED;
			WRITEINT16(demo_p,metal->player->followmobj->info->height>>FRACBITS);
			if (metal->player->followmobj->flags2 & MF2_LINKDRAW)
				followtic |= FZT_LINKDRAW;
			if (metal->player->followmobj->colorized)
				followtic |= FZT_COLORIZED;
			if (followtic & FZT_SKIN)
				WRITEUINT8(demo_p,(UINT8)(((skin_t *)(metal->player->followmobj->skin))-skins));
			oldmetal.flags2 |= MF2_AMBUSH;
		}

		if (metal->player->followmobj->scale != metal->scale)
		{
			followtic |= FZT_SCALE;
			WRITEFIXED(demo_p,metal->player->followmobj->scale);
		}

		temp = (INT16)((metal->player->followmobj->x-metal->x)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((metal->player->followmobj->y-metal->y)>>8);
		WRITEINT16(demo_p,temp);
		temp = (INT16)((metal->player->followmobj->z-metal->z)>>8);
		WRITEINT16(demo_p,temp);
		if (followtic & FZT_SKIN)
			WRITEUINT8(demo_p,metal->player->followmobj->sprite2);
		WRITEUINT16(demo_p,metal->player->followmobj->sprite);
		WRITEUINT32(demo_p,metal->player->followmobj->frame); // NOT & FF_FRAMEMASK here, so 32 bits
		WRITEUINT16(demo_p,metal->player->followmobj->color);

		*followtic_p = followtic;
	}
	else
		oldmetal.flags2 &= ~MF2_AMBUSH;

	*ziptic_p = ziptic;

	// attention here for the ticcmd size!
	// latest demos with mouse aiming byte in ticcmd
	if (demo_p >= demoend - 32)
	{
		G_StopMetalRecording(false); // no more space
		return;
	}
}

//
// G_RecordDemo
//
void G_RecordDemo(const char *name)
{
	INT32 maxsize;

	strcpy(demoname, name);
	strcat(demoname, ".lmp");
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
//	if (demobuffer)
//		free(demobuffer);
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;

	demorecording = true;
}

void G_RecordMetal(void)
{
	INT32 maxsize;
	maxsize = 1024*1024;
	if (M_CheckParm("-maxdemo") && M_IsNextParm())
		maxsize = atoi(M_GetNextParm()) * 1024;
	demo_p = NULL;
	demobuffer = malloc(maxsize);
	demoend = demobuffer + maxsize;
	metalrecording = true;
}

void G_BeginRecording(void)
{
	UINT8 i;
	char name[MAXCOLORNAME+1];
	player_t *player = &players[consoleplayer];

	if (demo_p)
		return;
	memset(name,0,sizeof(name));

	demo_p = demobuffer;
	demoflags = DF_GHOST|(modeattacking<<DF_ATTACKSHIFT);

	// Setup header.
	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);

	// demo checksum
	demo_p += 16;

	// game data
	M_Memcpy(demo_p, "PLAY", 4); demo_p += 4;
	WRITEINT16(demo_p,gamemap);
	M_Memcpy(demo_p, mapmd5, 16); demo_p += 16;

	WRITEUINT8(demo_p,demoflags);
	switch ((demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
	case ATTACKING_NONE: // 0
		break;
	case ATTACKING_RECORD: // 1
		demotime_p = demo_p;
		WRITEUINT32(demo_p,UINT32_MAX); // time
		WRITEUINT32(demo_p,0); // score
		WRITEUINT16(demo_p,0); // rings
		break;
	case ATTACKING_NIGHTS: // 2
		demotime_p = demo_p;
		WRITEUINT32(demo_p,UINT32_MAX); // time
		WRITEUINT32(demo_p,0); // score
		break;
	default: // 3
		break;
	}

	WRITEUINT32(demo_p,P_GetInitSeed());

	// Name
	for (i = 0; i < 16 && cv_playername.string[i]; i++)
		name[i] = cv_playername.string[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;

	// Skin
	for (i = 0; i < 16 && cv_skin.string[i]; i++)
		name[i] = cv_skin.string[i];
	for (; i < 16; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,16);
	demo_p += 16;

	// Color
	for (i = 0; i < MAXCOLORNAME && cv_playercolor.string[i]; i++)
		name[i] = cv_playercolor.string[i];
	for (; i < MAXCOLORNAME; i++)
		name[i] = '\0';
	M_Memcpy(demo_p,name,MAXCOLORNAME);
	demo_p += MAXCOLORNAME;

	// Stats
	WRITEUINT8(demo_p,player->charability);
	WRITEUINT8(demo_p,player->charability2);
	WRITEUINT8(demo_p,player->actionspd>>FRACBITS);
	WRITEUINT8(demo_p,player->mindash>>FRACBITS);
	WRITEUINT8(demo_p,player->maxdash>>FRACBITS);
	WRITEUINT8(demo_p,player->normalspeed>>FRACBITS);
	WRITEUINT8(demo_p,player->runspeed>>FRACBITS);
	WRITEUINT8(demo_p,player->thrustfactor);
	WRITEUINT8(demo_p,player->accelstart);
	WRITEUINT8(demo_p,player->acceleration);
	WRITEUINT8(demo_p,player->height>>FRACBITS);
	WRITEUINT8(demo_p,player->spinheight>>FRACBITS);
	WRITEUINT8(demo_p,player->camerascale>>FRACBITS);
	WRITEUINT8(demo_p,player->shieldscale>>FRACBITS);

	// Trying to convert it back to % causes demo desync due to precision loss.
	// Don't do it.
	WRITEFIXED(demo_p, player->jumpfactor);

	// And mobjtype_t is best with UINT32 too...
	WRITEUINT32(demo_p, player->followitem);

	// Save pflag data - see SendWeaponPref()
	{
		UINT8 buf = 0;
		pflags_t pflags = 0;
		if (cv_flipcam.value)
		{
			buf |= 0x01;
			pflags |= PF_FLIPCAM;
		}
		if (cv_analog[0].value)
		{
			buf |= 0x02;
			pflags |= PF_ANALOGMODE;
		}
		if (cv_directionchar[0].value)
		{
			buf |= 0x04;
			pflags |= PF_DIRECTIONCHAR;
		}
		if (cv_autobrake.value)
		{
			buf |= 0x08;
			pflags |= PF_AUTOBRAKE;
		}
		if (cv_usejoystick.value)
			buf |= 0x10;
		CV_SetValue(&cv_showinputjoy, !!(cv_usejoystick.value));

		WRITEUINT8(demo_p,buf);
		player->pflags = pflags;
	}

	// Save netvar data
	CV_SaveDemoVars(&demo_p);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));
	memset(&ghostext,0,sizeof(ghostext));
	ghostext.lastcolor = ghostext.color = GHC_NORMAL;
	ghostext.lastscale = ghostext.scale = FRACUNIT;

	if (player->mo)
	{
		oldghost.x = player->mo->x;
		oldghost.y = player->mo->y;
		oldghost.z = player->mo->z;
		oldghost.angle = player->mo->angle>>24;

		// preticker started us gravity flipped
		if (player->mo->eflags & MFE_VERTICALFLIP)
			ghostext.flags |= EZT_FLIP;
	}
}

void G_BeginMetal(void)
{
	mobj_t *mo = players[consoleplayer].mo;

#if 0
	if (demo_p)
		return;
#endif

	demo_p = demobuffer;

	// Write header.
	M_Memcpy(demo_p, DEMOHEADER, 12); demo_p += 12;
	WRITEUINT8(demo_p,VERSION);
	WRITEUINT8(demo_p,SUBVERSION);
	WRITEUINT16(demo_p,DEMOVERSION);

	// demo checksum
	demo_p += 16;

	M_Memcpy(demo_p, "METL", 4); demo_p += 4;

	memset(&ghostext,0,sizeof(ghostext));
	ghostext.lastscale = ghostext.scale = FRACUNIT;

	// Set up our memory.
	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	oldmetal.angle = mo->angle>>24;
}

void G_SetDemoTime(UINT32 ptime, UINT32 pscore, UINT16 prings)
{
	if (!demorecording || !demotime_p)
		return;
	if (demoflags & DF_RECORDATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, pscore);
		WRITEUINT16(demotime_p, prings);
		demotime_p = NULL;
	}
	else if (demoflags & DF_NIGHTSATTACK)
	{
		WRITEUINT32(demotime_p, ptime);
		WRITEUINT32(demotime_p, pscore);
		demotime_p = NULL;
	}
}

// Returns bitfield:
// 1 == new demo has lower time
// 2 == new demo has higher score
// 4 == new demo has higher rings
UINT8 G_CmpDemoTime(char *oldname, char *newname)
{
	UINT8 *buffer,*p;
	UINT8 flags;
	UINT32 oldtime, newtime, oldscore, newscore;
	UINT16 oldrings, newrings, oldversion;
	size_t bufsize ATTRUNUSED;
	UINT8 c;
	UINT16 s ATTRUNUSED;
	UINT8 aflags = 0;

	// load the new file
	FIL_DefaultExtension(newname, ".lmp");
	bufsize = FIL_ReadFile(newname, &buffer);
	I_Assert(bufsize != 0);
	p = buffer;

	// read demo header
	I_Assert(!memcmp(p, DEMOHEADER, 12));
	p += 12; // DEMOHEADER
	c = READUINT8(p); // VERSION
	I_Assert(c == VERSION);
	c = READUINT8(p); // SUBVERSION
	I_Assert(c == SUBVERSION);
	s = READUINT16(p);
	I_Assert(s >= 0x000c);
	p += 16; // demo checksum
	I_Assert(!memcmp(p, "PLAY", 4));
	p += 4; // PLAY
	p += 2; // gamemap
	p += 16; // map md5
	flags = READUINT8(p); // demoflags

	aflags = flags & (DF_RECORDATTACK|DF_NIGHTSATTACK);
	I_Assert(aflags);
	if (flags & DF_RECORDATTACK)
	{
		newtime = READUINT32(p);
		newscore = READUINT32(p);
		newrings = READUINT16(p);
	}
	else if (flags & DF_NIGHTSATTACK)
	{
		newtime = READUINT32(p);
		newscore = READUINT32(p);
		newrings = 0;
	}
	else // appease compiler
		return 0;

	Z_Free(buffer);

	// load old file
	FIL_DefaultExtension(oldname, ".lmp");
	if (!FIL_ReadFile(oldname, &buffer))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), oldname);
		return UINT8_MAX;
	}
	p = buffer;

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 12; // DEMOHEADER
	p++; // VERSION
	p++; // SUBVERSION
	oldversion = READUINT16(p);
	switch(oldversion) // demoversion
	{
	case DEMOVERSION: // latest always supported
	case 0x000c: // all that changed between then and now was longer color name
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	p += 16; // demo checksum
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' invalid format. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	} p += 4; // "PLAY"
	if (oldversion <= 0x0008)
		p++; // gamemap
	else
		p += 2; // gamemap
	p += 16; // mapmd5
	flags = READUINT8(p);
	if (!(flags & aflags))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("File '%s' not from same game mode. It will be overwritten.\n"), oldname);
		Z_Free(buffer);
		return UINT8_MAX;
	}
	if (flags & DF_RECORDATTACK)
	{
		oldtime = READUINT32(p);
		oldscore = READUINT32(p);
		oldrings = READUINT16(p);
	}
	else if (flags & DF_NIGHTSATTACK)
	{
		oldtime = READUINT32(p);
		oldscore = READUINT32(p);
		oldrings = 0;
	}
	else // appease compiler
		return UINT8_MAX;

	Z_Free(buffer);

	c = 0;
	if (newtime < oldtime
	|| (newtime == oldtime && (newscore > oldscore || newrings > oldrings)))
		c |= 1; // Better time
	if (newscore > oldscore
	|| (newscore == oldscore && newtime < oldtime))
		c |= 1<<1; // Better score
	if (newrings > oldrings
	|| (newrings == oldrings && newtime < oldtime))
		c |= 1<<2; // Better rings
	return c;
}

//
// G_PlayDemo
//
void G_DeferedPlayDemo(const char *name)
{
	COM_BufAddText("playdemo \"");
	COM_BufAddText(name);
	COM_BufAddText("\"\n");
}

//
// Start a demo from a .LMP file or from a wad resource
//
void G_DoPlayDemo(char *defdemoname)
{
	UINT8 i;
	lumpnum_t l;
	char skin[17],color[MAXCOLORNAME+1],*n,*pdemoname;
	UINT8 version,subversion,charability,charability2,thrustfactor,accelstart,acceleration,cnamelen;
	pflags_t pflags;
	UINT32 randseed, followitem;
	fixed_t camerascale,shieldscale,actionspd,mindash,maxdash,normalspeed,runspeed,jumpfactor,height,spinheight;
	char msg[1024];
#ifdef OLD22DEMOCOMPAT
	boolean use_old_demo_vars = false;
#endif

	skin[16] = '\0';
	color[MAXCOLORNAME] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(defdemoname))
	{
		//FIL_DefaultExtension(defdemoname, ".lmp");
		if (!FIL_ReadFile(defdemoname, &demobuffer))
		{
			snprintf(msg, 1024, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			CONS_Alert(CONS_ERROR, "%s", msg);
			gameaction = ga_nothing;
			M_StartMessage(msg, NULL, MM_NOTHING);
			return;
		}
		demo_p = demobuffer;
	}
	// load demo resource from WAD
	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		snprintf(msg, 1024, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		gameaction = ga_nothing;
		M_StartMessage(msg, NULL, MM_NOTHING);
		return;
	}
	else // it's an internal demo
		demobuffer = demo_p = W_CacheLumpNum(l, PU_STATIC);

	// read demo header
	gameaction = ga_nothing;
	demoplayback = true;
	if (memcmp(demo_p, DEMOHEADER, 12))
	{
		snprintf(msg, 1024, M_GetText("%s is not a SRB2 replay file.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 12; // DEMOHEADER

	version = READUINT8(demo_p);
	subversion = READUINT8(demo_p);
	demoversion = READUINT16(demo_p);
	switch(demoversion)
	{
	case DEMOVERSION: // latest always supported
		cnamelen = MAXCOLORNAME;
		break;
#ifdef OLD22DEMOCOMPAT
	// all that changed between then and now was longer color name
	case 0x000c:
		cnamelen = 16;
		use_old_demo_vars = true;
		break;
#endif
	// too old, cannot support.
	default:
		snprintf(msg, 1024, M_GetText("%s is an incompatible replay format and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 16; // demo checksum
	if (memcmp(demo_p, "PLAY", 4))
	{
		snprintf(msg, 1024, M_GetText("%s is the wrong type of recording and cannot be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}
	demo_p += 4; // "PLAY"
	gamemap = READINT16(demo_p);
	demo_p += 16; // mapmd5

	demoflags = READUINT8(demo_p);
	modeattacking = (demoflags & DF_ATTACKMASK)>>DF_ATTACKSHIFT;
	CON_ToggleOff();

	hu_demoscore = 0;
	hu_demotime = UINT32_MAX;
	hu_demorings = 0;

	switch (modeattacking)
	{
	case ATTACKING_NONE: // 0
		break;
	case ATTACKING_RECORD: // 1
		hu_demotime  = READUINT32(demo_p);
		hu_demoscore = READUINT32(demo_p);
		hu_demorings = READUINT16(demo_p);
		break;
	case ATTACKING_NIGHTS: // 2
		hu_demotime  = READUINT32(demo_p);
		hu_demoscore = READUINT32(demo_p);
		break;
	default: // 3
		modeattacking = ATTACKING_NONE;
		break;
	}

	// Random seed
	randseed = READUINT32(demo_p);

	// Player name
	M_Memcpy(player_names[0],demo_p,16);
	demo_p += 16;

	// Skin
	M_Memcpy(skin,demo_p,16);
	demo_p += 16;

	// Color
	M_Memcpy(color,demo_p,cnamelen);
	demo_p += cnamelen;

	charability = READUINT8(demo_p);
	charability2 = READUINT8(demo_p);
	actionspd = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	mindash = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	maxdash = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	normalspeed = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	runspeed = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	thrustfactor = READUINT8(demo_p);
	accelstart = READUINT8(demo_p);
	acceleration = READUINT8(demo_p);
	height = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	spinheight = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	camerascale = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	shieldscale = (fixed_t)READUINT8(demo_p)<<FRACBITS;
	jumpfactor = READFIXED(demo_p);
	followitem = READUINT32(demo_p);

	// pflag data
	{
		UINT8 buf = READUINT8(demo_p);
		pflags = 0;
		if (buf & 0x01)
			pflags |= PF_FLIPCAM;
		if (buf & 0x02)
			pflags |= PF_ANALOGMODE;
		if (buf & 0x04)
			pflags |= PF_DIRECTIONCHAR;
		if (buf & 0x08)
			pflags |= PF_AUTOBRAKE;
		CV_SetValue(&cv_showinputjoy, !!(buf & 0x10));
	}

	// net var data
#ifdef OLD22DEMOCOMPAT
	if (use_old_demo_vars)
		CV_LoadOldDemoVars(&demo_p);
	else
#endif
		CV_LoadDemoVars(&demo_p);

	// Sigh ... it's an empty demo.
	if (*demo_p == DEMOMARKER)
	{
		snprintf(msg, 1024, M_GetText("%s contains no data to be played.\n"), pdemoname);
		CONS_Alert(CONS_ERROR, "%s", msg);
		M_StartMessage(msg, NULL, MM_NOTHING);
		Z_Free(pdemoname);
		Z_Free(demobuffer);
		demoplayback = false;
		titledemo = false;
		return;
	}

	Z_Free(pdemoname);

	memset(&oldcmd,0,sizeof(oldcmd));
	memset(&oldghost,0,sizeof(oldghost));

	if (VERSION != version || SUBVERSION != subversion)
		CONS_Alert(CONS_WARNING, M_GetText("Demo version does not match game version. Desyncs may occur.\n"));

	// didn't start recording right away.
	demo_start = false;

	// Set skin
	SetPlayerSkin(0, skin);

#ifdef HAVE_BLUA
	LUAh_MapChange(gamemap);
#endif
	displayplayer = consoleplayer = 0;
	memset(playeringame,0,sizeof(playeringame));
	playeringame[0] = true;
	P_SetRandSeed(randseed);
	G_InitNew(false, G_BuildMapName(gamemap), true, true, false);

	// Set color
	players[0].skincolor = skins[players[0].skin].prefcolor;
	for (i = 0; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name,color))
		{
			players[0].skincolor = i;
			break;
		}
	CV_StealthSetValue(&cv_playercolor, players[0].skincolor);
	if (players[0].mo)
	{
		players[0].mo->color = players[0].skincolor;
		oldghost.x = players[0].mo->x;
		oldghost.y = players[0].mo->y;
		oldghost.z = players[0].mo->z;
	}

	// Set saved attribute values
	// No cheat checking here, because even if they ARE wrong...
	// it would only break the replay if we clipped them.
	players[0].camerascale = camerascale;
	players[0].shieldscale = shieldscale;
	players[0].charability = charability;
	players[0].charability2 = charability2;
	players[0].actionspd = actionspd;
	players[0].mindash = mindash;
	players[0].maxdash = maxdash;
	players[0].normalspeed = normalspeed;
	players[0].runspeed = runspeed;
	players[0].thrustfactor = thrustfactor;
	players[0].accelstart = accelstart;
	players[0].acceleration = acceleration;
	players[0].height = height;
	players[0].spinheight = spinheight;
	players[0].jumpfactor = jumpfactor;
	players[0].followitem = followitem;
	players[0].pflags = pflags;

	demo_start = true;
}

void G_AddGhost(char *defdemoname)
{
	INT32 i;
	lumpnum_t l;
	char name[17],skin[17],color[MAXCOLORNAME+1],*n,*pdemoname,md5[16];
	UINT8 cnamelen;
	demoghost *gh;
	UINT8 flags;
	UINT8 *buffer,*p;
	mapthing_t *mthing;
	UINT16 count, ghostversion;

	name[16] = '\0';
	skin[16] = '\0';
	color[16] = '\0';

	n = defdemoname+strlen(defdemoname);
	while (*n != '/' && *n != '\\' && n != defdemoname)
		n--;
	if (n != defdemoname)
		n++;
	pdemoname = ZZ_Alloc(strlen(n)+1);
	strcpy(pdemoname,n);

	// Internal if no extension, external if one exists
	if (FIL_CheckExtension(defdemoname))
	{
		//FIL_DefaultExtension(defdemoname, ".lmp");
		if (!FIL_ReadFileTag(defdemoname, &buffer, PU_LEVEL))
		{
			CONS_Alert(CONS_ERROR, M_GetText("Failed to read file '%s'.\n"), defdemoname);
			Z_Free(pdemoname);
			return;
		}
		p = buffer;
	}
	// load demo resource from WAD
	else if ((l = W_CheckNumForName(defdemoname)) == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to read lump '%s'.\n"), defdemoname);
		Z_Free(pdemoname);
		return;
	}
	else // it's an internal demo
		buffer = p = W_CacheLumpNum(l, PU_LEVEL);

	// read demo header
	if (memcmp(p, DEMOHEADER, 12))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Not a SRB2 replay.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 12; // DEMOHEADER
	p++; // VERSION
	p++; // SUBVERSION
	ghostversion = READUINT16(p);
	switch(ghostversion)
	{
	case DEMOVERSION: // latest always supported
		cnamelen = MAXCOLORNAME;
		break;
	// all that changed between then and now was longer color name
	case 0x000c:
		cnamelen = 16;
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo version incompatible.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}
	M_Memcpy(md5, p, 16); p += 16; // demo checksum
	for (gh = ghosts; gh; gh = gh->next)
		if (!memcmp(md5, gh->checksum, 16)) // another ghost in the game already has this checksum?
		{ // Don't add another one, then!
			CONS_Debug(DBG_SETUP, "Rejecting duplicate ghost %s (MD5 was matched)\n", pdemoname);
			Z_Free(pdemoname);
			Z_Free(buffer);
			return;
		}
	if (memcmp(p, "PLAY", 4))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: Demo format unacceptable.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	} p += 4; // "PLAY"
	if (ghostversion <= 0x0008)
		p++; // gamemap
	else
		p += 2; // gamemap
	p += 16; // mapmd5 (possibly check for consistency?)
	flags = READUINT8(p);
	if (!(flags & DF_GHOST))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Ghost %s: No ghost data in this demo.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}
	switch ((flags & DF_ATTACKMASK)>>DF_ATTACKSHIFT)
	{
	case ATTACKING_NONE: // 0
		break;
	case ATTACKING_RECORD: // 1
		p += 10; // demo time, score, and rings
		break;
	case ATTACKING_NIGHTS: // 2
		p += 8; // demo time left, score
		break;
	default: // 3
		break;
	}

	p += 4; // random seed

	// Player name (TODO: Display this somehow if it doesn't match cv_playername!)
	M_Memcpy(name, p,16);
	p += 16;

	// Skin
	M_Memcpy(skin, p,16);
	p += 16;

	// Color
	M_Memcpy(color, p,cnamelen);
	p += cnamelen;

	// Ghosts do not have a player structure to put this in.
	p++; // charability
	p++; // charability2
	p++; // actionspd
	p++; // mindash
	p++; // maxdash
	p++; // normalspeed
	p++; // runspeed
	p++; // thrustfactor
	p++; // accelstart
	p++; // acceleration
	p++; // height
	p++; // spinheight
	p++; // camerascale
	p++; // shieldscale
	p += 4; // jumpfactor
	p += 4; // followitem

	p++; // pflag data

	// net var data
	count = READUINT16(p);
	while (count--)
	{
		p += 2;
		SKIPSTRING(p);
		p++;
	}

	if (*p == DEMOMARKER)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Failed to add ghost %s: Replay is empty.\n"), pdemoname);
		Z_Free(pdemoname);
		Z_Free(buffer);
		return;
	}

	gh = Z_Calloc(sizeof(demoghost), PU_LEVEL, NULL);
	gh->next = ghosts;
	gh->buffer = buffer;
	M_Memcpy(gh->checksum, md5, 16);
	gh->p = p;

	ghosts = gh;

	gh->version = ghostversion;
	mthing = playerstarts[0];
	I_Assert(mthing);
	{ // A bit more complex than P_SpawnPlayer because ghosts aren't solid and won't just push themselves out of the ceiling.
		fixed_t z,f,c;
		fixed_t offset = mthing->z << FRACBITS;
		gh->mo = P_SpawnMobj(mthing->x << FRACBITS, mthing->y << FRACBITS, 0, MT_GHOST);
		gh->mo->angle = FixedAngle(mthing->angle << FRACBITS);
		f = gh->mo->floorz;
		c = gh->mo->ceilingz - mobjinfo[MT_PLAYER].height;
		if (!!(mthing->options & MTF_AMBUSH) ^ !!(mthing->options & MTF_OBJECTFLIP))
		{
			z = c - offset;
			if (z < f)
				z = f;
		}
		else
		{
			z = f + offset;
			if (z > c)
				z = c;
		}
		gh->mo->z = z;
	}

	gh->oldmo.x = gh->mo->x;
	gh->oldmo.y = gh->mo->y;
	gh->oldmo.z = gh->mo->z;

	// Set skin
	gh->mo->skin = &skins[0];
	for (i = 0; i < numskins; i++)
		if (!stricmp(skins[i].name,skin))
		{
			gh->mo->skin = &skins[i];
			break;
		}
	gh->oldmo.skin = gh->mo->skin;

	// Set color
	gh->mo->color = ((skin_t*)gh->mo->skin)->prefcolor;
	for (i = 0; i < numskincolors; i++)
		if (!stricmp(skincolors[i].name,color))
		{
			gh->mo->color = (UINT16)i;
			break;
		}
	gh->oldmo.color = gh->mo->color;

	gh->mo->state = states+S_PLAY_STND;
	gh->mo->sprite = gh->mo->state->sprite;
	gh->mo->sprite2 = (gh->mo->state->frame & FF_FRAMEMASK);
	//gh->mo->frame = tr_trans30<<FF_TRANSSHIFT;
	gh->mo->flags2 |= MF2_DONTDRAW;
	gh->fadein = (9-3)*6; // fade from invisible to trans30 over as close to 35 tics as possible
	gh->mo->tics = -1;

	CONS_Printf(M_GetText("Added ghost %s from %s\n"), name, pdemoname);
	Z_Free(pdemoname);
}

// Clean up all ghosts
void G_FreeGhosts(void)
{
	while (ghosts)
	{
		demoghost *next = ghosts->next;
		Z_Free(ghosts);
		ghosts = next;
	}
	ghosts = NULL;
}

//
// G_TimeDemo
// NOTE: name is a full filename for external demos
//
static INT32 restorecv_vidwait;

void G_TimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw");
	noblit = M_CheckParm("-noblit");
	restorecv_vidwait = cv_vidwait.value;
	if (cv_vidwait.value)
		CV_Set(&cv_vidwait, "0");
	timingdemo = true;
	singletics = true;
	framecount = 0;
	demostarttime = I_GetTime();
	G_DeferedPlayDemo(name);
}

void G_DoPlayMetal(void)
{
	lumpnum_t l;
	mobj_t *mo = NULL;
	thinker_t *th;

	// it's an internal demo
	if ((l = W_CheckNumForName(va("%sMS",G_BuildMapName(gamemap)))) == LUMPERROR)
	{
		CONS_Alert(CONS_WARNING, M_GetText("No bot recording for this map.\n"));
		return;
	}
	else
		metalbuffer = metal_p = W_CacheLumpNum(l, PU_STATIC);

	// find metal sonic
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)th;
		if (mo->type != MT_METALSONIC_RACE)
			continue;

		break;
	}
	if (th == &thlist[THINK_MOBJ])
	{
		CONS_Alert(CONS_ERROR, M_GetText("Failed to find bot entity.\n"));
		Z_Free(metalbuffer);
		return;
	}

	// read demo header
    metal_p += 12; // DEMOHEADER
	metal_p++; // VERSION
	metal_p++; // SUBVERSION
	metalversion = READUINT16(metal_p);
	switch(metalversion)
	{
	case DEMOVERSION: // latest always supported
	case 0x000c: // all that changed between then and now was longer color name
		break;
	// too old, cannot support.
	default:
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, format version incompatible.\n"));
		Z_Free(metalbuffer);
		return;
	}
	metal_p += 16; // demo checksum
	if (memcmp(metal_p, "METL", 4))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Failed to load bot recording for this map, wasn't recorded in Metal format.\n"));
		Z_Free(metalbuffer);
		return;
	} metal_p += 4; // "METL"

	// read initial tic
	memset(&oldmetal,0,sizeof(oldmetal));
	oldmetal.x = mo->x;
	oldmetal.y = mo->y;
	oldmetal.z = mo->z;
	metalplayback = mo;
}

void G_DoneLevelLoad(void)
{
	CONS_Printf(M_GetText("Loaded level in %f sec\n"), (double)(I_GetTime() - demostarttime) / TICRATE);
	framecount = 0;
	demostarttime = I_GetTime();
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

// Writes the demo's checksum, or just random garbage if you can't do that for some reason.
static void WriteDemoChecksum(void)
{
	UINT8 *p = demobuffer+16; // checksum position
#ifdef NOMD5
	UINT8 i;
	for (i = 0; i < 16; i++, p++)
		*p = P_RandomByte(); // This MD5 was chosen by fair dice roll and most likely < 50% correct.
#else
	md5_buffer((char *)p+16, demo_p - (p+16), p); // make a checksum of everything after the checksum in the file.
#endif
}

// Stops recording a demo.
static void G_StopDemoRecording(void)
{
	boolean saved = false;
	if (demo_p)
	{
		WRITEUINT8(demo_p, DEMOMARKER); // add the demo end marker
		WriteDemoChecksum();
		saved = FIL_WriteFile(va(pandf, srb2home, demoname), demobuffer, demo_p - demobuffer); // finally output the file.
	}
	free(demobuffer);
	demorecording = false;

	if (modeattacking != ATTACKING_RECORD)
	{
		if (saved)
			CONS_Printf(M_GetText("Demo %s recorded\n"), demoname);
		else
			CONS_Alert(CONS_WARNING, M_GetText("Demo %s not saved\n"), demoname);
	}
}

// Stops metal sonic's demo. Separate from other functions because metal + replays can coexist
void G_StopMetalDemo(void)
{

	// Metal Sonic finishing doesn't end the game, dammit.
	Z_Free(metalbuffer);
	metalbuffer = NULL;
	metalplayback = NULL;
	metal_p = NULL;
}

// Stops metal sonic recording.
ATTRNORETURN void FUNCNORETURN G_StopMetalRecording(boolean kill)
{
	boolean saved = false;
	if (demo_p)
	{
		WRITEUINT8(demo_p, (kill) ? METALDEATH : DEMOMARKER); // add the demo end (or metal death) marker
		WriteDemoChecksum();
		saved = FIL_WriteFile(va("%sMS.LMP", G_BuildMapName(gamemap)), demobuffer, demo_p - demobuffer); // finally output the file.
	}
	free(demobuffer);
	metalrecording = false;
	if (saved)
		I_Error("Saved to %sMS.LMP", G_BuildMapName(gamemap));
	I_Error("Failed to save demo!");
}

// Stops timing a demo.
static void G_StopTimingDemo(void)
{
	INT32 demotime;
	double f1, f2;
	demotime = I_GetTime() - demostarttime;
	if (!demotime)
		return;
	G_StopDemo();
	timingdemo = false;
	f1 = (double)demotime;
	f2 = (double)framecount*TICRATE;

	CONS_Printf(M_GetText("timed %u gametics in %d realtics - %u frames\n%f seconds, %f avg fps\n"),
		leveltime,demotime,(UINT32)framecount,f1/TICRATE,f2/f1);

	// CSV-readable timedemo results, for external parsing
	if (timedemo_csv)
	{
		FILE *f;
		const char *csvpath = va("%s"PATHSEP"%s", srb2home, "timedemo.csv");
		const char *header = "id,demoname,seconds,avgfps,leveltime,demotime,framecount,ticrate,rendermode,vidmode,vidwidth,vidheight,procbits\n";
		const char *rowformat = "\"%s\",\"%s\",%f,%f,%u,%d,%u,%u,%u,%u,%u,%u,%u\n";
		boolean headerrow = !FIL_FileExists(csvpath);
		UINT8 procbits = 0;

		// Bitness
		if (sizeof(void*) == 4)
			procbits = 32;
		else if (sizeof(void*) == 8)
			procbits = 64;

		f = fopen(csvpath, "a+");

		if (f)
		{
			if (headerrow)
				fputs(header, f);
			fprintf(f, rowformat,
				timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
			fclose(f);
			CONS_Printf("Timedemo results saved to '%s'\n", csvpath);
		}
		else
		{
			// Just print the CSV output to console
			CON_LogMessage(header);
			CONS_Printf(rowformat,
				timedemo_csv_id,timedemo_name,f1/TICRATE,f2/f1,leveltime,demotime,(UINT32)framecount,TICRATE,rendermode,vid.modenum,vid.width,vid.height,procbits);
		}
	}

	if (restorecv_vidwait != cv_vidwait.value)
		CV_SetValue(&cv_vidwait, restorecv_vidwait);
	D_AdvanceDemo();
}

// reset engine variable set for the demos
// called from stopdemo command, map command, and g_checkdemoStatus.
void G_StopDemo(void)
{
	Z_Free(demobuffer);
	demobuffer = NULL;
	demoplayback = false;
	titledemo = false;
	timingdemo = false;
	singletics = false;

	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission(); // cleanup

	G_SetGamestate(GS_NULL);
	wipegamestate = GS_NULL;
	SV_StopServer();
	SV_ResetServer();
}

boolean G_CheckDemoStatus(void)
{
	G_FreeGhosts();

	// DO NOT end metal sonic demos here

	if (timingdemo)
	{
		G_StopTimingDemo();
		return true;
	}

	if (demoplayback)
	{
		if (singledemo)
			I_Quit();
		G_StopDemo();

		if (modeattacking)
			M_EndModeAttackRun();
		else
			D_AdvanceDemo();

		return true;
	}

	if (demorecording)
	{
		G_StopDemoRecording();
		return true;
	}

	return false;
}
