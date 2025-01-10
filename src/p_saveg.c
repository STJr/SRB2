// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  p_saveg.c
/// \brief Archiving: SaveGame I/O

#include "doomdef.h"
#include "byteptr.h"
#include "d_main.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_random.h"
#include "m_misc.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "r_data.h"
#include "r_fps.h"
#include "r_textures.h"
#include "r_things.h"
#include "r_skins.h"
#include "r_state.h"
#include "w_wad.h"
#include "y_inter.h"
#include "z_zone.h"
#include "r_main.h"
#include "r_sky.h"
#include "p_polyobj.h"
#include "lua_script.h"
#include "p_slopes.h"
#include "hu_stuff.h"

savedata_t savedata;

#define ALLOC_SIZE(p, z) \
	if ((p)->pos + (z) > (p)->size) \
	{ \
		while ((p)->pos + (z) > (p)->size) \
			(p)->size <<= 1; \
		(p)->buf = realloc((p)->buf, (p)->size); \
	}

void P_WriteUINT8(save_t *p, UINT8 v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteSINT8(save_t *p, SINT8 v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteUINT16(save_t *p, UINT16 v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteINT16(save_t *p, INT16 v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteUINT32(save_t *p, UINT32 v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteINT32(save_t *p, INT32 v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteChar(save_t *p, char v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteFixed(save_t *p, fixed_t v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteAngle(save_t *p, angle_t v)
{
	ALLOC_SIZE(p, sizeof(v));
	memcpy(&p->buf[p->pos], &v, sizeof(v));
	p->pos += sizeof(v);
}

void P_WriteStringN(save_t *p, char const *s, size_t n)
{
	size_t i;

	for (i = 0; i < n && s[i] != '\0'; i++)
		P_WriteChar(p, s[i]);

	if (i < n)
		P_WriteChar(p, '\0');
}

void P_WriteStringL(save_t *p, char const *s, size_t n)
{
	size_t i;

	for (i = 0; i < n - 1 && s[i] != '\0'; i++)
		P_WriteChar(p, s[i]);

	P_WriteChar(p, '\0');
}

void P_WriteString(save_t *p, char const *s)
{
	size_t i;

	for (i = 0; s[i] != '\0'; i++)
		P_WriteChar(p, s[i]);

	P_WriteChar(p, '\0');
}

void P_WriteMem(save_t *p, void const *s, size_t n)
{
	ALLOC_SIZE(p, n);
	memcpy(&p->buf[p->pos], s, n);
	p->pos += n;
}

void P_SkipStringN(save_t *p, size_t n)
{
	size_t i;
	for (i = 0; p->pos < p->size && i < n && P_ReadChar(p) != '\0'; i++);
}

void P_SkipStringL(save_t *p, size_t n)
{
	P_SkipStringN(p, n);
}

void P_SkipString(save_t *p)
{
	P_SkipStringN(p, SIZE_MAX);
}

UINT8 P_ReadUINT8(save_t *p)
{
	UINT8 v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

SINT8 P_ReadSINT8(save_t *p)
{
	SINT8 v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

UINT16 P_ReadUINT16(save_t *p)
{
	UINT16 v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

INT16 P_ReadINT16(save_t *p)
{
	INT16 v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

UINT32 P_ReadUINT32(save_t *p)
{
	UINT32 v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

INT32 P_ReadINT32(save_t *p)
{
	INT32 v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

char P_ReadChar(save_t *p)
{
	char v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

fixed_t P_ReadFixed(save_t *p)
{
	fixed_t v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

angle_t P_ReadAngle(save_t *p)
{
	angle_t v;
	if (p->pos + sizeof(v) > p->size)
		return 0;
	memcpy(&v, &p->buf[p->pos], sizeof(v));
	p->pos += sizeof(v);
	return v;
}

void P_ReadStringN(save_t *p, char *s, size_t n)
{
	size_t i;
	for (i = 0; p->pos < p->size && i < n && (s[i] = P_ReadChar(p)) != '\0'; i++);
	s[i] = '\0';
}

void P_ReadStringL(save_t *p, char *s, size_t n)
{
	P_ReadStringN(p, s, n - 1);
}

void P_ReadString(save_t *p, char *s)
{
	P_ReadStringN(p, s, SIZE_MAX);
}

void P_ReadMem(save_t *p, void *s, size_t n)
{
	if (p->pos + n > p->size)
		return;
	memcpy(s, &p->buf[p->pos], n);
	p->pos += n;
}

// Block UINT32s to attempt to ensure that the correct data is
// being sent and received
#define ARCHIVEBLOCK_MISC       0x7FEEDEED
#define ARCHIVEBLOCK_PLAYERS    0x7F448008
#define ARCHIVEBLOCK_WORLD      0x7F8C08C0
#define ARCHIVEBLOCK_POBJS      0x7F928546
#define ARCHIVEBLOCK_THINKERS   0x7F37037C
#define ARCHIVEBLOCK_SPECIALS   0x7F228378
#define ARCHIVEBLOCK_EMBLEMS    0x7F4A5445
#define ARCHIVEBLOCK_SECPORTALS 0x7FBE34C9

// Note: This cannot be bigger
// than an UINT16
typedef enum
{
	CAPSULE    = 0x04,
	AWAYVIEW   = 0x08,
	FIRSTAXIS  = 0x10,
	SECONDAXIS = 0x20,
	FOLLOW     = 0x40,
	DRONE      = 0x80,
} player_saveflags;

static inline void P_ArchivePlayer(save_t *save_p)
{
	const player_t *player = &players[consoleplayer];
	SINT8 pllives = player->lives;
	if (pllives < startinglivesbalance[numgameovers]) // Bump up to 3 lives if the player
		pllives = startinglivesbalance[numgameovers]; // has less than that.

#ifdef NEWSKINSAVES
	// Write a specific value into the old skininfo location.
	// If we read something other than this, it's an older save file that used skin numbers.
	P_WriteUINT16(save_p, NEWSKINSAVES);
#endif

	// Write skin names, so that loading skins in different orders
	// doesn't change who the save file is for!
	P_WriteStringN(save_p, skins[player->skin]->name, SKINNAMESIZE);

	if (botskin != 0)
	{
		P_WriteStringN(save_p, skins[botskin-1]->name, SKINNAMESIZE);
	}
	else
	{
		P_WriteStringN(save_p, "\0", SKINNAMESIZE);
	}

	P_WriteUINT8(save_p, numgameovers);
	P_WriteSINT8(save_p, pllives);
	P_WriteUINT32(save_p, player->score);
	P_WriteINT32(save_p, player->continues);
}

static inline void P_UnArchivePlayer(save_t *save_p)
{
#ifdef NEWSKINSAVES
	INT16 backwardsCompat = P_ReadUINT16(save_p);

	if (backwardsCompat != NEWSKINSAVES)
	{
		// This is an older save file, which used direct skin numbers.
		savedata.skin = backwardsCompat & ((1<<5) - 1);
		savedata.botskin = backwardsCompat >> 5;
	}
	else
#endif
	{
		char ourSkinName[SKINNAMESIZE+1];
		char botSkinName[SKINNAMESIZE+1];

		P_ReadStringN(save_p, ourSkinName, SKINNAMESIZE);
		savedata.skin = R_SkinAvailable(ourSkinName);

		P_ReadStringN(save_p, botSkinName, SKINNAMESIZE);
		savedata.botskin = R_SkinAvailable(botSkinName) + 1;
	}

	savedata.numgameovers = P_ReadUINT8(save_p);
	savedata.lives = P_ReadSINT8(save_p);
	savedata.score = P_ReadUINT32(save_p);
	savedata.continues = P_ReadINT32(save_p);
}

static void P_NetArchivePlayers(save_t *save_p)
{
	INT32 i, j;
	UINT16 flags;
//	size_t q;

	P_WriteUINT32(save_p, ARCHIVEBLOCK_PLAYERS);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		P_WriteSINT8(save_p, (SINT8)adminplayers[i]);

		if (!playeringame[i])
			continue;

		flags = 0;

		// no longer send ticcmds

		P_WriteStringN(save_p, player_names[i], MAXPLAYERNAME);
		P_WriteINT16(save_p, players[i].angleturn);
		P_WriteINT16(save_p, players[i].oldrelangleturn);
		P_WriteAngle(save_p, players[i].aiming);
		P_WriteAngle(save_p, players[i].drawangle);
		P_WriteAngle(save_p, players[i].viewrollangle);
		P_WriteAngle(save_p, players[i].awayviewaiming);
		P_WriteINT32(save_p, players[i].awayviewtics);
		P_WriteINT16(save_p, players[i].rings);
		P_WriteINT16(save_p, players[i].spheres);

		P_WriteSINT8(save_p, players[i].pity);
		P_WriteINT32(save_p, players[i].currentweapon);
		P_WriteINT32(save_p, players[i].ringweapons);

		P_WriteUINT16(save_p, players[i].ammoremoval);
		P_WriteUINT32(save_p, players[i].ammoremovaltimer);
		P_WriteINT32(save_p, players[i].ammoremovaltimer);

		for (j = 0; j < NUMPOWERS; j++)
			P_WriteUINT16(save_p, players[i].powers[j]);

		P_WriteUINT8(save_p, players[i].playerstate);
		P_WriteUINT32(save_p, players[i].pflags);
		P_WriteUINT8(save_p, players[i].panim);
		P_WriteUINT8(save_p, players[i].stronganim);
		P_WriteUINT8(save_p, players[i].spectator);
		P_WriteUINT8(save_p, players[i].muted);

		P_WriteUINT16(save_p, players[i].flashpal);
		P_WriteUINT16(save_p, players[i].flashcount);

		P_WriteUINT16(save_p, players[i].skincolor);
		P_WriteINT32(save_p, players[i].skin);
		P_WriteUINT32(save_p, players[i].availabilities);
		P_WriteUINT32(save_p, players[i].score);
		P_WriteUINT32(save_p, players[i].recordscore);
		P_WriteFixed(save_p, players[i].dashspeed);
		P_WriteSINT8(save_p, players[i].lives);
		P_WriteSINT8(save_p, players[i].continues);
		P_WriteSINT8(save_p, players[i].xtralife);
		P_WriteUINT8(save_p, players[i].gotcontinue);
		P_WriteFixed(save_p, players[i].speed);
		P_WriteUINT8(save_p, players[i].secondjump);
		P_WriteUINT8(save_p, players[i].fly1);
		P_WriteUINT8(save_p, players[i].scoreadd);
		P_WriteUINT32(save_p, players[i].glidetime);
		P_WriteUINT8(save_p, players[i].climbing);
		P_WriteINT32(save_p, players[i].deadtimer);
		P_WriteUINT32(save_p, players[i].exiting);
		P_WriteUINT8(save_p, players[i].homing);
		P_WriteUINT32(save_p, players[i].dashmode);
		P_WriteUINT32(save_p, players[i].skidtime);

		//////////
		// Bots //
		//////////
		P_WriteUINT8(save_p, players[i].bot);
		P_WriteUINT8(save_p, players[i].botmem.lastForward);
		P_WriteUINT8(save_p, players[i].botmem.lastBlocked);
		P_WriteUINT8(save_p, players[i].botmem.catchup_tics);
		P_WriteUINT8(save_p, players[i].botmem.thinkstate);
		P_WriteUINT8(save_p, players[i].removing);

		P_WriteUINT8(save_p, players[i].blocked);
		P_WriteUINT16(save_p, players[i].lastbuttons);

		////////////////////////////
		// Conveyor Belt Movement //
		////////////////////////////
		P_WriteFixed(save_p, players[i].cmomx); // Conveyor momx
		P_WriteFixed(save_p, players[i].cmomy); // Conveyor momy
		P_WriteFixed(save_p, players[i].rmomx); // "Real" momx (momx - cmomx)
		P_WriteFixed(save_p, players[i].rmomy); // "Real" momy (momy - cmomy)

		/////////////////////
		// Race Mode Stuff //
		/////////////////////
		P_WriteINT16(save_p, players[i].numboxes);
		P_WriteINT16(save_p, players[i].totalring);
		P_WriteUINT32(save_p, players[i].realtime);
		P_WriteUINT8(save_p, players[i].laps);

		////////////////////
		// CTF Mode Stuff //
		////////////////////
		P_WriteINT32(save_p, players[i].ctfteam);
		P_WriteUINT16(save_p, players[i].gotflag);

		P_WriteINT32(save_p, players[i].weapondelay);
		P_WriteINT32(save_p, players[i].tossdelay);

		P_WriteUINT32(save_p, players[i].starposttime);
		P_WriteINT16(save_p, players[i].starpostx);
		P_WriteINT16(save_p, players[i].starposty);
		P_WriteINT16(save_p, players[i].starpostz);
		P_WriteINT32(save_p, players[i].starpostnum);
		P_WriteAngle(save_p, players[i].starpostangle);
		P_WriteFixed(save_p, players[i].starpostscale);

		P_WriteAngle(save_p, players[i].angle_pos);
		P_WriteAngle(save_p, players[i].old_angle_pos);

		P_WriteINT32(save_p, players[i].flyangle);
		P_WriteUINT32(save_p, players[i].drilltimer);
		P_WriteINT32(save_p, players[i].linkcount);
		P_WriteUINT32(save_p, players[i].linktimer);
		P_WriteINT32(save_p, players[i].anotherflyangle);
		P_WriteUINT32(save_p, players[i].nightstime);
		P_WriteUINT32(save_p, players[i].bumpertime);
		P_WriteINT32(save_p, players[i].drillmeter);
		P_WriteUINT8(save_p, players[i].drilldelay);
		P_WriteUINT8(save_p, players[i].bonustime);
		P_WriteFixed(save_p, players[i].oldscale);
		P_WriteUINT8(save_p, players[i].mare);
		P_WriteUINT8(save_p, players[i].marelap);
		P_WriteUINT8(save_p, players[i].marebonuslap);
		P_WriteUINT32(save_p, players[i].marebegunat);
		P_WriteUINT32(save_p, players[i].lastmaretime);
		P_WriteUINT32(save_p, players[i].startedtime);
		P_WriteUINT32(save_p, players[i].finishedtime);
		P_WriteUINT32(save_p, players[i].lapbegunat);
		P_WriteUINT32(save_p, players[i].lapstartedtime);
		P_WriteINT16(save_p, players[i].finishedspheres);
		P_WriteINT16(save_p, players[i].finishedrings);
		P_WriteUINT32(save_p, players[i].marescore);
		P_WriteUINT32(save_p, players[i].lastmarescore);
		P_WriteUINT32(save_p, players[i].totalmarescore);
		P_WriteUINT8(save_p, players[i].lastmare);
		P_WriteUINT8(save_p, players[i].lastmarelap);
		P_WriteUINT8(save_p, players[i].lastmarebonuslap);
		P_WriteUINT8(save_p, players[i].totalmarelap);
		P_WriteUINT8(save_p, players[i].totalmarebonuslap);
		P_WriteINT32(save_p, players[i].maxlink);
		P_WriteUINT8(save_p, players[i].texttimer);
		P_WriteUINT8(save_p, players[i].textvar);

		if (players[i].capsule)
			flags |= CAPSULE;

		if (players[i].awayviewmobj)
			flags |= AWAYVIEW;

		if (players[i].axis1)
			flags |= FIRSTAXIS;

		if (players[i].axis2)
			flags |= SECONDAXIS;

		if (players[i].followmobj)
			flags |= FOLLOW;

		if (players[i].drone)
			flags |= DRONE;

		P_WriteINT16(save_p, players[i].lastsidehit);
		P_WriteINT16(save_p, players[i].lastlinehit);

		P_WriteUINT32(save_p, players[i].losstime);

		P_WriteUINT8(save_p, players[i].timeshit);

		P_WriteINT32(save_p, players[i].onconveyor);

		P_WriteUINT32(save_p, players[i].jointime);
		P_WriteUINT32(save_p, players[i].quittime);

		P_WriteUINT16(save_p, flags);

		if (flags & CAPSULE)
			P_WriteUINT32(save_p, players[i].capsule->mobjnum);

		if (flags & FIRSTAXIS)
			P_WriteUINT32(save_p, players[i].axis1->mobjnum);

		if (flags & SECONDAXIS)
			P_WriteUINT32(save_p, players[i].axis2->mobjnum);

		if (flags & AWAYVIEW)
			P_WriteUINT32(save_p, players[i].awayviewmobj->mobjnum);

		if (flags & FOLLOW)
			P_WriteUINT32(save_p, players[i].followmobj->mobjnum);

		if (flags & DRONE)
			P_WriteUINT32(save_p, players[i].drone->mobjnum);

		P_WriteFixed(save_p, players[i].camerascale);
		P_WriteFixed(save_p, players[i].shieldscale);

		P_WriteUINT8(save_p, players[i].charability);
		P_WriteUINT8(save_p, players[i].charability2);
		P_WriteUINT32(save_p, players[i].charflags);
		P_WriteUINT32(save_p, (UINT32)players[i].thokitem);
		P_WriteUINT32(save_p, (UINT32)players[i].spinitem);
		P_WriteUINT32(save_p, (UINT32)players[i].revitem);
		P_WriteUINT32(save_p, (UINT32)players[i].followitem);
		P_WriteFixed(save_p, players[i].actionspd);
		P_WriteFixed(save_p, players[i].mindash);
		P_WriteFixed(save_p, players[i].maxdash);
		P_WriteFixed(save_p, players[i].normalspeed);
		P_WriteFixed(save_p, players[i].runspeed);
		P_WriteUINT8(save_p, players[i].thrustfactor);
		P_WriteUINT8(save_p, players[i].accelstart);
		P_WriteUINT8(save_p, players[i].acceleration);
		P_WriteFixed(save_p, players[i].jumpfactor);
		P_WriteFixed(save_p, players[i].height);
		P_WriteFixed(save_p, players[i].spinheight);
	}
}

static void P_NetUnArchivePlayers(save_t *save_p)
{
	INT32 i, j;
	UINT16 flags;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_PLAYERS)
		I_Error("Bad $$$.sav at archive block Players");

	for (i = 0; i < MAXPLAYERS; i++)
	{
		adminplayers[i] = (INT32)P_ReadSINT8(save_p);

		// Do NOT memset player struct to 0
		// other areas may initialize data elsewhere
		//memset(&players[i], 0, sizeof (player_t));
		if (!playeringame[i])
			continue;

		// NOTE: sending tics should (hopefully) no longer be necessary

		P_ReadStringN(save_p, player_names[i], MAXPLAYERNAME);
		players[i].angleturn = P_ReadINT16(save_p);
		players[i].oldrelangleturn = P_ReadINT16(save_p);
		players[i].aiming = P_ReadAngle(save_p);
		players[i].drawangle = P_ReadAngle(save_p);
		players[i].viewrollangle = P_ReadAngle(save_p);
		players[i].awayviewaiming = P_ReadAngle(save_p);
		players[i].awayviewtics = P_ReadINT32(save_p);
		players[i].rings = P_ReadINT16(save_p);
		players[i].spheres = P_ReadINT16(save_p);

		players[i].pity = P_ReadSINT8(save_p);
		players[i].currentweapon = P_ReadINT32(save_p);
		players[i].ringweapons = P_ReadINT32(save_p);

		players[i].ammoremoval = P_ReadUINT16(save_p);
		players[i].ammoremovaltimer = P_ReadUINT32(save_p);
		players[i].ammoremovalweapon = P_ReadINT32(save_p);

		for (j = 0; j < NUMPOWERS; j++)
			players[i].powers[j] = P_ReadUINT16(save_p);

		players[i].playerstate = P_ReadUINT8(save_p);
		players[i].pflags = P_ReadUINT32(save_p);
		players[i].panim = P_ReadUINT8(save_p);
		players[i].stronganim = P_ReadUINT8(save_p);
		players[i].spectator = P_ReadUINT8(save_p);
		players[i].muted = P_ReadUINT8(save_p);

		players[i].flashpal = P_ReadUINT16(save_p);
		players[i].flashcount = P_ReadUINT16(save_p);

		players[i].skincolor = P_ReadUINT16(save_p);
		players[i].skin = P_ReadINT32(save_p);
		players[i].availabilities = P_ReadUINT32(save_p);
		players[i].score = P_ReadUINT32(save_p);
		players[i].recordscore = P_ReadUINT32(save_p);
		players[i].dashspeed = P_ReadFixed(save_p); // dashing speed
		players[i].lives = P_ReadSINT8(save_p);
		players[i].continues = P_ReadSINT8(save_p); // continues that player has acquired
		players[i].xtralife = P_ReadSINT8(save_p); // Ring Extra Life counter
		players[i].gotcontinue = P_ReadUINT8(save_p); // got continue from stage
		players[i].speed = P_ReadFixed(save_p); // Player's speed (distance formula of MOMX and MOMY values)
		players[i].secondjump = P_ReadUINT8(save_p);
		players[i].fly1 = P_ReadUINT8(save_p); // Tails flying
		players[i].scoreadd = P_ReadUINT8(save_p); // Used for multiple enemy attack bonus
		players[i].glidetime = P_ReadUINT32(save_p); // Glide counter for thrust
		players[i].climbing = P_ReadUINT8(save_p); // Climbing on the wall
		players[i].deadtimer = P_ReadINT32(save_p); // End game if game over lasts too long
		players[i].exiting = P_ReadUINT32(save_p); // Exitlevel timer
		players[i].homing = P_ReadUINT8(save_p); // Are you homing?
		players[i].dashmode = P_ReadUINT32(save_p); // counter for dashmode ability
		players[i].skidtime = P_ReadUINT32(save_p); // Skid timer

		//////////
		// Bots //
		//////////
		players[i].bot = P_ReadUINT8(save_p);

		players[i].botmem.lastForward = P_ReadUINT8(save_p);
		players[i].botmem.lastBlocked = P_ReadUINT8(save_p);
		players[i].botmem.catchup_tics = P_ReadUINT8(save_p);
		players[i].botmem.thinkstate = P_ReadUINT8(save_p);
		players[i].removing = P_ReadUINT8(save_p);

		players[i].blocked = P_ReadUINT8(save_p);
		players[i].lastbuttons = P_ReadUINT16(save_p);

		////////////////////////////
		// Conveyor Belt Movement //
		////////////////////////////
		players[i].cmomx = P_ReadFixed(save_p); // Conveyor momx
		players[i].cmomy = P_ReadFixed(save_p); // Conveyor momy
		players[i].rmomx = P_ReadFixed(save_p); // "Real" momx (momx - cmomx)
		players[i].rmomy = P_ReadFixed(save_p); // "Real" momy (momy - cmomy)

		/////////////////////
		// Race Mode Stuff //
		/////////////////////
		players[i].numboxes = P_ReadINT16(save_p); // Number of item boxes obtained for Race Mode
		players[i].totalring = P_ReadINT16(save_p); // Total number of rings obtained for Race Mode
		players[i].realtime = P_ReadUINT32(save_p); // integer replacement for leveltime
		players[i].laps = P_ReadUINT8(save_p); // Number of laps (optional)

		////////////////////
		// CTF Mode Stuff //
		////////////////////
		players[i].ctfteam = P_ReadINT32(save_p); // 1 == Red, 2 == Blue
		players[i].gotflag = P_ReadUINT16(save_p); // 1 == Red, 2 == Blue Do you have the flag?

		players[i].weapondelay = P_ReadINT32(save_p);
		players[i].tossdelay = P_ReadINT32(save_p);

		players[i].starposttime = P_ReadUINT32(save_p);
		players[i].starpostx = P_ReadINT16(save_p);
		players[i].starposty = P_ReadINT16(save_p);
		players[i].starpostz = P_ReadINT16(save_p);
		players[i].starpostnum = P_ReadINT32(save_p);
		players[i].starpostangle = P_ReadAngle(save_p);
		players[i].starpostscale = P_ReadFixed(save_p);

		players[i].angle_pos = P_ReadAngle(save_p);
		players[i].old_angle_pos = P_ReadAngle(save_p);

		players[i].flyangle = P_ReadINT32(save_p);
		players[i].drilltimer = P_ReadUINT32(save_p);
		players[i].linkcount = P_ReadINT32(save_p);
		players[i].linktimer = P_ReadUINT32(save_p);
		players[i].anotherflyangle = P_ReadINT32(save_p);
		players[i].nightstime = P_ReadUINT32(save_p);
		players[i].bumpertime = P_ReadUINT32(save_p);
		players[i].drillmeter = P_ReadINT32(save_p);
		players[i].drilldelay = P_ReadUINT8(save_p);
		players[i].bonustime = (boolean)P_ReadUINT8(save_p);
		players[i].oldscale = P_ReadFixed(save_p);
		players[i].mare = P_ReadUINT8(save_p);
		players[i].marelap = P_ReadUINT8(save_p);
		players[i].marebonuslap = P_ReadUINT8(save_p);
		players[i].marebegunat = P_ReadUINT32(save_p);
		players[i].lastmaretime = P_ReadUINT32(save_p);
		players[i].startedtime = P_ReadUINT32(save_p);
		players[i].finishedtime = P_ReadUINT32(save_p);
		players[i].lapbegunat = P_ReadUINT32(save_p);
		players[i].lapstartedtime = P_ReadUINT32(save_p);
		players[i].finishedspheres = P_ReadINT16(save_p);
		players[i].finishedrings = P_ReadINT16(save_p);
		players[i].marescore = P_ReadUINT32(save_p);
		players[i].lastmarescore = P_ReadUINT32(save_p);
		players[i].totalmarescore = P_ReadUINT32(save_p);
		players[i].lastmare = P_ReadUINT8(save_p);
		players[i].lastmarelap = P_ReadUINT8(save_p);
		players[i].lastmarebonuslap = P_ReadUINT8(save_p);
		players[i].totalmarelap = P_ReadUINT8(save_p);
		players[i].totalmarebonuslap = P_ReadUINT8(save_p);
		players[i].maxlink = P_ReadINT32(save_p);
		players[i].texttimer = P_ReadUINT8(save_p);
		players[i].textvar = P_ReadUINT8(save_p);

		players[i].lastsidehit = P_ReadINT16(save_p);
		players[i].lastlinehit = P_ReadINT16(save_p);

		players[i].losstime = P_ReadUINT32(save_p);

		players[i].timeshit = P_ReadUINT8(save_p);

		players[i].onconveyor = P_ReadINT32(save_p);

		players[i].jointime = P_ReadUINT32(save_p);
		players[i].quittime = P_ReadUINT32(save_p);

		flags = P_ReadUINT16(save_p);

		if (flags & CAPSULE)
			players[i].capsule = (mobj_t *)(size_t)P_ReadUINT32(save_p);

		if (flags & FIRSTAXIS)
			players[i].axis1 = (mobj_t *)(size_t)P_ReadUINT32(save_p);

		if (flags & SECONDAXIS)
			players[i].axis2 = (mobj_t *)(size_t)P_ReadUINT32(save_p);

		if (flags & AWAYVIEW)
			players[i].awayviewmobj = (mobj_t *)(size_t)P_ReadUINT32(save_p);

		if (flags & FOLLOW)
			players[i].followmobj = (mobj_t *)(size_t)P_ReadUINT32(save_p);

		if (flags & DRONE)
			players[i].drone = (mobj_t *)(size_t)P_ReadUINT32(save_p);

		players[i].camerascale = P_ReadFixed(save_p);
		players[i].shieldscale = P_ReadFixed(save_p);

		//SetPlayerSkinByNum(i, players[i].skin);
		players[i].charability = P_ReadUINT8(save_p);
		players[i].charability2 = P_ReadUINT8(save_p);
		players[i].charflags = P_ReadUINT32(save_p);
		players[i].thokitem = P_ReadUINT32(save_p);
		players[i].spinitem = P_ReadUINT32(save_p);
		players[i].revitem = P_ReadUINT32(save_p);
		players[i].followitem = P_ReadUINT32(save_p);
		players[i].actionspd = P_ReadFixed(save_p);
		players[i].mindash = P_ReadFixed(save_p);
		players[i].maxdash = P_ReadFixed(save_p);
		players[i].normalspeed = P_ReadFixed(save_p);
		players[i].runspeed = P_ReadFixed(save_p);
		players[i].thrustfactor = P_ReadUINT8(save_p);
		players[i].accelstart = P_ReadUINT8(save_p);
		players[i].acceleration = P_ReadUINT8(save_p);
		players[i].jumpfactor = P_ReadFixed(save_p);
		players[i].height = P_ReadFixed(save_p);
		players[i].spinheight = P_ReadFixed(save_p);

		players[i].viewheight = 41*players[i].height/48; // scale cannot be factored in at this point
	}
}

///
/// Colormaps
///

static extracolormap_t *net_colormaps = NULL;
static UINT32 num_net_colormaps = 0;
static UINT32 num_ffloors = 0; // for loading

// Copypasta from r_data.c AddColormapToList
// But also check for equality and return the matching index
static UINT32 CheckAddNetColormapToList(extracolormap_t *extra_colormap)
{
	extracolormap_t *exc, *exc_prev = NULL;
	UINT32 i = 0;

	if (!net_colormaps)
	{
		net_colormaps = R_CopyColormap(extra_colormap, false);
		net_colormaps->next = 0;
		net_colormaps->prev = 0;
		num_net_colormaps = i+1;
		return i;
	}

	for (exc = net_colormaps; exc; exc_prev = exc, exc = exc->next)
	{
		if (R_CheckEqualColormaps(exc, extra_colormap, true, true, true))
			return i;
		i++;
	}

	exc_prev->next = R_CopyColormap(extra_colormap, false);
	extra_colormap->prev = exc_prev;
	extra_colormap->next = 0;

	num_net_colormaps = i+1;
	return i;
}

static extracolormap_t *GetNetColormapFromList(UINT32 index)
{
	// For loading, we have to be tricky:
	// We load the sectors BEFORE knowing the colormap values
	// So if an index doesn't exist, fill our list with dummy colormaps
	// until we get the index we want
	// Then when we load the color data, we set up the dummy colormaps

	extracolormap_t *exc, *last_exc = NULL;
	UINT32 i = 0;

	if (!net_colormaps) // initialize our list
		net_colormaps = R_CreateDefaultColormap(false);

	for (exc = net_colormaps; exc; last_exc = exc, exc = exc->next)
	{
		if (i++ == index)
			return exc;
	}


	// LET'S HOPE that index is a sane value, because we create up to [index]
	// entries in net_colormaps. At this point, we don't know
	// what the total colormap count is
	if (index >= numsectors*3 + num_ffloors)
		// if every sector had a unique colormap change AND a fade color thinker which has two colormap entries
		// AND every ffloor had a fade FOF thinker with one colormap entry
		I_Error("Colormap %d from server is too high for sectors %d", index, (UINT32)numsectors);

	// our index doesn't exist, so just make the entry
	for (; i <= index; i++)
	{
		exc = R_CreateDefaultColormap(false);
		if (last_exc)
			last_exc->next = exc;
		exc->prev = last_exc;
		exc->next = NULL;
		last_exc = exc;
	}
	return exc;
}

static void ClearNetColormaps(void)
{
	// We're actually Z_Freeing each entry here,
	// so don't call this in P_NetUnArchiveColormaps (where entries will be used in-game)
	extracolormap_t *exc, *exc_next;

	for (exc = net_colormaps; exc; exc = exc_next)
	{
		exc_next = exc->next;
		Z_Free(exc);
	}
	num_net_colormaps = 0;
	num_ffloors = 0;
	net_colormaps = NULL;
}

static void P_NetArchiveColormaps(save_t *save_p)
{
	// We save and then we clean up our colormap mess
	extracolormap_t *exc, *exc_next;
	UINT32 i = 0;
	P_WriteUINT32(save_p, num_net_colormaps); // save for safety

	for (exc = net_colormaps; i < num_net_colormaps; i++, exc = exc_next)
	{
		// We must save num_net_colormaps worth of data
		// So fill non-existent entries with default.
		if (!exc)
			exc = R_CreateDefaultColormap(false);

		P_WriteUINT8(save_p, exc->fadestart);
		P_WriteUINT8(save_p, exc->fadeend);
		P_WriteUINT8(save_p, exc->flags);

		P_WriteINT32(save_p, exc->rgba);
		P_WriteINT32(save_p, exc->fadergba);

#ifdef EXTRACOLORMAPLUMPS
		P_WriteStringN(save_p, exc->lumpname, 9);
#endif

		exc_next = exc->next;
		Z_Free(exc); // don't need anymore
	}

	num_net_colormaps = 0;
	num_ffloors = 0;
	net_colormaps = NULL;
}

static void P_NetUnArchiveColormaps(save_t *save_p)
{
	// When we reach this point, we already populated our list with
	// dummy colormaps. Now that we are loading the color data,
	// set up the dummies.
	extracolormap_t *exc, *existing_exc, *exc_next = NULL;
	UINT32 i = 0;

	num_net_colormaps = P_ReadUINT32(save_p);

	for (exc = net_colormaps; i < num_net_colormaps; i++, exc = exc_next)
	{
		UINT8 fadestart, fadeend, flags;
		INT32 rgba, fadergba;
#ifdef EXTRACOLORMAPLUMPS
		char lumpname[9];
#endif

		fadestart = P_ReadUINT8(save_p);
		fadeend = P_ReadUINT8(save_p);
		flags = P_ReadUINT8(save_p);

		rgba = P_ReadINT32(save_p);
		fadergba = P_ReadINT32(save_p);

#ifdef EXTRACOLORMAPLUMPS
		P_ReadStringN(save_p, lumpname, 9);

		if (lumpname[0])
		{
			if (!exc)
				// no point making a new entry since nothing points to it,
				// but we needed to read the data so now continue
				continue;

			exc_next = exc->next; // this gets overwritten during our operations here, so get it now
			existing_exc = R_ColormapForName(lumpname);
			*exc = *existing_exc;
			R_AddColormapToList(exc); // see HACK note below on why we're adding duplicates
			continue;
		}
#endif

		if (!exc)
			// no point making a new entry since nothing points to it,
			// but we needed to read the data so now continue
			continue;

		exc_next = exc->next; // this gets overwritten during our operations here, so get it now

		exc->fadestart = fadestart;
		exc->fadeend = fadeend;
		exc->flags = flags;

		exc->rgba = rgba;
		exc->fadergba = fadergba;

#ifdef EXTRACOLORMAPLUMPS
		exc->lump = LUMPERROR;
		exc->lumpname[0] = 0;
#endif

		existing_exc = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags);

		if (existing_exc)
			exc->colormap = existing_exc->colormap;
		else
			// CONS_Debug(DBG_RENDER, "Creating Colormap: rgba(%d,%d,%d,%d) fadergba(%d,%d,%d,%d)\n",
			// 	R_GetRgbaR(rgba), R_GetRgbaG(rgba), R_GetRgbaB(rgba), R_GetRgbaA(rgba),
			//	R_GetRgbaR(fadergba), R_GetRgbaG(fadergba), R_GetRgbaB(fadergba), R_GetRgbaA(fadergba));
			exc->colormap = R_CreateLightTable(exc);

		// HACK: If this dummy is a duplicate, we're going to add it
		// to the extra_colormaps list anyway. I think this is faster
		// than going through every loaded sector and correcting their
		// colormap address to the pre-existing one, PER net_colormap entry
		R_AddColormapToList(exc);

		if (i < num_net_colormaps-1 && !exc_next)
			exc_next = R_CreateDefaultColormap(false);
	}

	// if we still have a valid net_colormap after iterating up to num_net_colormaps,
	// some sector had a colormap index higher than num_net_colormaps. We done goofed or $$$ was corrupted.
	// In any case, add them to the colormap list too so that at least the sectors' colormap
	// addresses are valid and accounted properly
	if (exc_next)
	{
		existing_exc = R_GetDefaultColormap();
		for (exc = exc_next; exc; exc = exc->next)
		{
			exc->colormap = existing_exc->colormap; // all our dummies are default values
			R_AddColormapToList(exc);
		}
	}

	// Don't need these anymore
	num_net_colormaps = 0;
	num_ffloors = 0;
	net_colormaps = NULL;
}

static void P_NetArchiveWaypoints(save_t *save_p)
{
	INT32 i, j;

	for (i = 0; i < NUMWAYPOINTSEQUENCES; i++)
	{
		P_WriteUINT16(save_p, numwaypoints[i]);
		for (j = 0; j < numwaypoints[i]; j++)
			P_WriteUINT32(save_p, waypoints[i][j] ? waypoints[i][j]->mobjnum : 0);
	}
}

static void P_NetUnArchiveWaypoints(save_t *save_p)
{
	INT32 i, j;
	UINT32 mobjnum;

	for (i = 0; i < NUMWAYPOINTSEQUENCES; i++)
	{
		numwaypoints[i] = P_ReadUINT16(save_p);
		for (j = 0; j < numwaypoints[i]; j++)
		{
			mobjnum = P_ReadUINT32(save_p);
			waypoints[i][j] = (mobjnum == 0) ? NULL : P_FindNewPosition(mobjnum);
		}
	}
}

///
/// World Archiving
///

#define SD_FLOORHT  0x01
#define SD_CEILHT   0x02
#define SD_FLOORPIC 0x04
#define SD_CEILPIC  0x08
#define SD_LIGHT    0x10
#define SD_SPECIAL  0x20
#define SD_DIFF2    0x40
#define SD_FFLOORS  0x80

// diff2 flags
#define SD_FXOFFS    0x01
#define SD_FYOFFS    0x02
#define SD_CXOFFS    0x04
#define SD_CYOFFS    0x08
#define SD_FLOORANG  0x10
#define SD_CEILANG   0x20
#define SD_TAG       0x40
#define SD_DIFF3     0x80

// diff3 flags
#define SD_TAGLIST      0x01
#define SD_COLORMAP     0x02
#define SD_CRUMBLESTATE 0x04
#define SD_FLOORLIGHT   0x08
#define SD_CEILLIGHT    0x10
#define SD_FLAG         0x20
#define SD_SPECIALFLAG  0x40
#define SD_DIFF4        0x80

// diff4 flags
#define SD_DAMAGETYPE 0x01
#define SD_TRIGGERTAG 0x02
#define SD_TRIGGERER 0x04
#define SD_FXSCALE   0x08
#define SD_FYSCALE   0x10
#define SD_CXSCALE   0x20
#define SD_CYSCALE   0x40
#define SD_DIFF5     0x80

// diff5 flags
#define SD_GRAVITY     0x01
#define SD_FLOORPORTAL 0x02
#define SD_CEILPORTAL  0x04

// diff1 flags
#define LD_FLAG          0x01
#define LD_SPECIAL       0x02
#define LD_CLLCOUNT      0x04
#define LD_ARGS          0x08
#define LD_STRINGARGS    0x10
#define LD_SIDE1         0x20
#define LD_SIDE2         0x40
#define LD_DIFF2         0x80

// diff2 flags
#define LD_EXECUTORDELAY 0x01
#define LD_TRANSFPORTAL  0x02

// sidedef flags
enum
{
	LD_SDTEXOFFX     = 1,
	LD_SDTEXOFFY     = 1<<1,
	LD_SDTOPTEX      = 1<<2,
	LD_SDBOTTEX      = 1<<3,
	LD_SDMIDTEX      = 1<<4,
	LD_SDTOPOFFX     = 1<<5,
	LD_SDTOPOFFY     = 1<<6,
	LD_SDMIDOFFX     = 1<<7,
	LD_SDMIDOFFY     = 1<<8,
	LD_SDBOTOFFX     = 1<<9,
	LD_SDBOTOFFY     = 1<<10,
	LD_SDTOPSCALEX   = 1<<11,
	LD_SDTOPSCALEY   = 1<<12,
	LD_SDMIDSCALEX   = 1<<13,
	LD_SDMIDSCALEY   = 1<<14,
	LD_SDBOTSCALEX   = 1<<15,
	LD_SDBOTSCALEY   = 1<<16,
	LD_SDLIGHT       = 1<<17,
	LD_SDTOPLIGHT    = 1<<18,
	LD_SDMIDLIGHT    = 1<<19,
	LD_SDBOTLIGHT    = 1<<20,
	LD_SDREPEATCNT   = 1<<21,
	LD_SDFLAGS       = 1<<22,
	LD_SDLIGHTABS    = 1<<23,
	LD_SDTOPLIGHTABS = 1<<24,
	LD_SDMIDLIGHTABS = 1<<25,
	LD_SDBOTLIGHTABS = 1<<26
};

static boolean P_AreArgsEqual(const line_t *li, const line_t *spawnli)
{
	UINT8 i;
	for (i = 0; i < NUMLINEARGS; i++)
		if (li->args[i] != spawnli->args[i])
			return false;

	return true;
}

static boolean P_AreStringArgsEqual(const line_t *li, const line_t *spawnli)
{
	UINT8 i;
	for (i = 0; i < NUMLINESTRINGARGS; i++)
	{
		if (!li->stringargs[i])
			return !spawnli->stringargs[i];

		if (strcmp(li->stringargs[i], spawnli->stringargs[i]))
			return false;
	}

	return true;
}

#define FD_FLAGS 0x01
#define FD_ALPHA 0x02

// Check if any of the sector's FOFs differ from how they spawned
static boolean CheckFFloorDiff(const sector_t *ss)
{
	ffloor_t *rover;

	for (rover = ss->ffloors; rover; rover = rover->next)
	{
		if (rover->fofflags != rover->spawnflags
		|| rover->alpha != rover->spawnalpha)
			{
				return true; // we found an FOF that changed!
				// don't bother checking for more, we do that later
			}
	}
	return false;
}

// Special case: save the stats of all modified ffloors along with their ffloor "number"s
// we don't bother with ffloors that haven't changed, that would just add to savegame even more than is really needed
static void ArchiveFFloors(save_t *save_p, const sector_t *ss)
{
	size_t j = 0; // ss->ffloors is saved as ffloor #0, ss->ffloors->next is #1, etc
	ffloor_t *rover;
	UINT8 fflr_diff;
	for (rover = ss->ffloors; rover; rover = rover->next)
	{
		fflr_diff = 0; // reset diff flags
		if (rover->fofflags != rover->spawnflags)
			fflr_diff |= FD_FLAGS;
		if (rover->alpha != rover->spawnalpha)
			fflr_diff |= FD_ALPHA;

		if (fflr_diff)
		{
			P_WriteUINT16(save_p, j); // save ffloor "number"
			P_WriteUINT8(save_p, fflr_diff);
			if (fflr_diff & FD_FLAGS)
				P_WriteUINT32(save_p, rover->fofflags);
			if (fflr_diff & FD_ALPHA)
				P_WriteINT16(save_p, rover->alpha);
		}
		j++;
	}
	P_WriteUINT16(save_p, 0xffff);
}

static void UnArchiveFFloors(save_t *save_p, const sector_t *ss)
{
	UINT16 j = 0; // number of current ffloor in loop
	UINT16 fflr_i; // saved ffloor "number" of next modified ffloor
	UINT16 fflr_diff; // saved ffloor diff
	ffloor_t *rover;

	rover = ss->ffloors;
	if (!rover) // it is assumed sectors[i].ffloors actually exists, but just in case...
		I_Error("Sector does not have any ffloors!");

	fflr_i = P_ReadUINT16(save_p); // get first modified ffloor's number ready
	for (;;) // for some reason the usual for (rover = x; ...) thing doesn't work here?
	{
		if (fflr_i == 0xffff) // end of modified ffloors list, let's stop already
			break;
		// should NEVER need to be checked
		//if (rover == NULL)
			//break;
		if (j != fflr_i) // this ffloor was not modified
		{
			j++;
			rover = rover->next;
			continue;
		}

		fflr_diff = P_ReadUINT8(save_p);

		if (fflr_diff & FD_FLAGS)
			rover->fofflags = P_ReadUINT32(save_p);
		if (fflr_diff & FD_ALPHA)
			rover->alpha = P_ReadINT16(save_p);

		fflr_i = P_ReadUINT16(save_p); // get next ffloor "number" ready

		j++;
		rover = rover->next;
	}
}

static void ArchiveSectors(save_t *save_p)
{
	size_t i, j;
	const sector_t *ss = sectors;
	const sector_t *spawnss = spawnsectors;
	UINT8 diff, diff2, diff3, diff4, diff5;

	for (i = 0; i < numsectors; i++, ss++, spawnss++)
	{
		diff = diff2 = diff3 = diff4 = diff5 = 0;
		if (ss->floorheight != spawnss->floorheight)
			diff |= SD_FLOORHT;
		if (ss->ceilingheight != spawnss->ceilingheight)
			diff |= SD_CEILHT;
		//
		// flats
		//
		if (ss->floorpic != spawnss->floorpic)
			diff |= SD_FLOORPIC;
		if (ss->ceilingpic != spawnss->ceilingpic)
			diff |= SD_CEILPIC;

		if (ss->lightlevel != spawnss->lightlevel)
			diff |= SD_LIGHT;
		if (ss->special != spawnss->special)
			diff |= SD_SPECIAL;

		if (ss->floorxoffset != spawnss->floorxoffset)
			diff2 |= SD_FXOFFS;
		if (ss->flooryoffset != spawnss->flooryoffset)
			diff2 |= SD_FYOFFS;
		if (ss->ceilingxoffset != spawnss->ceilingxoffset)
			diff2 |= SD_CXOFFS;
		if (ss->ceilingyoffset != spawnss->ceilingyoffset)
			diff2 |= SD_CYOFFS;
		if (ss->floorxscale != spawnss->floorxscale)
			diff2 |= SD_FXSCALE;
		if (ss->flooryscale != spawnss->flooryscale)
			diff2 |= SD_FYSCALE;
		if (ss->ceilingxscale != spawnss->ceilingxscale)
			diff2 |= SD_CXSCALE;
		if (ss->ceilingyscale != spawnss->ceilingyscale)
			diff2 |= SD_CYSCALE;
		if (ss->floorangle != spawnss->floorangle)
			diff2 |= SD_FLOORANG;
		if (ss->ceilingangle != spawnss->ceilingangle)
			diff2 |= SD_CEILANG;

		if (!Tag_Compare(&ss->tags, &spawnss->tags))
			diff2 |= SD_TAG;

		if (ss->extra_colormap != spawnss->extra_colormap)
			diff3 |= SD_COLORMAP;
		if (ss->crumblestate)
			diff3 |= SD_CRUMBLESTATE;

		if (ss->floorlightlevel != spawnss->floorlightlevel || ss->floorlightabsolute != spawnss->floorlightabsolute)
			diff3 |= SD_FLOORLIGHT;
		if (ss->ceilinglightlevel != spawnss->ceilinglightlevel || ss->ceilinglightabsolute != spawnss->ceilinglightabsolute)
			diff3 |= SD_CEILLIGHT;
		if (ss->flags != spawnss->flags)
			diff3 |= SD_FLAG;
		if (ss->specialflags != spawnss->specialflags)
			diff3 |= SD_SPECIALFLAG;
		if (ss->damagetype != spawnss->damagetype)
			diff4 |= SD_DAMAGETYPE;
		if (ss->triggertag != spawnss->triggertag)
			diff4 |= SD_TRIGGERTAG;
		if (ss->triggerer != spawnss->triggerer)
			diff4 |= SD_TRIGGERER;
		if (ss->gravity != spawnss->gravity)
			diff5 |= SD_GRAVITY;
		if (ss->portal_floor != spawnss->portal_floor)
			diff5 |= SD_FLOORPORTAL;
		if (ss->portal_ceiling != spawnss->portal_ceiling)
			diff5 |= SD_CEILPORTAL;

		if (ss->ffloors && CheckFFloorDiff(ss))
			diff |= SD_FFLOORS;

		if (diff5)
			diff4 |= SD_DIFF5;

		if (diff4)
			diff3 |= SD_DIFF4;

		if (diff3)
			diff2 |= SD_DIFF3;

		if (diff2)
			diff |= SD_DIFF2;

		if (diff)
		{
			P_WriteUINT32(save_p, i);
			P_WriteUINT8(save_p, diff);
			if (diff & SD_DIFF2)
				P_WriteUINT8(save_p, diff2);
			if (diff2 & SD_DIFF3)
				P_WriteUINT8(save_p, diff3);
			if (diff3 & SD_DIFF4)
				P_WriteUINT8(save_p, diff4);
			if (diff4 & SD_DIFF5)
				P_WriteUINT8(save_p, diff5);
			if (diff & SD_FLOORHT)
				P_WriteFixed(save_p, ss->floorheight);
			if (diff & SD_CEILHT)
				P_WriteFixed(save_p, ss->ceilingheight);
			if (diff & SD_FLOORPIC)
				P_WriteMem(save_p, levelflats[ss->floorpic].name, 8);
			if (diff & SD_CEILPIC)
				P_WriteMem(save_p, levelflats[ss->ceilingpic].name, 8);
			if (diff & SD_LIGHT)
				P_WriteINT16(save_p, ss->lightlevel);
			if (diff & SD_SPECIAL)
				P_WriteINT16(save_p, ss->special);
			if (diff2 & SD_FXOFFS)
				P_WriteFixed(save_p, ss->floorxoffset);
			if (diff2 & SD_FYOFFS)
				P_WriteFixed(save_p, ss->flooryoffset);
			if (diff2 & SD_CXOFFS)
				P_WriteFixed(save_p, ss->ceilingxoffset);
			if (diff2 & SD_CYOFFS)
				P_WriteFixed(save_p, ss->ceilingyoffset);
			if (diff2 & SD_FLOORANG)
				P_WriteAngle(save_p, ss->floorangle);
			if (diff2 & SD_CEILANG)
				P_WriteAngle(save_p, ss->ceilingangle);
			if (diff2 & SD_TAG)
			{
				P_WriteUINT32(save_p, ss->tags.count);
				for (j = 0; j < ss->tags.count; j++)
					P_WriteINT16(save_p, ss->tags.tags[j]);
			}

			if (diff3 & SD_COLORMAP)
				P_WriteUINT32(save_p, CheckAddNetColormapToList(ss->extra_colormap));
					// returns existing index if already added, or appends to net_colormaps and returns new index
			if (diff3 & SD_CRUMBLESTATE)
				P_WriteINT32(save_p, ss->crumblestate);
			if (diff3 & SD_FLOORLIGHT)
			{
				P_WriteINT16(save_p, ss->floorlightlevel);
				P_WriteUINT8(save_p, ss->floorlightabsolute);
			}
			if (diff3 & SD_CEILLIGHT)
			{
				P_WriteINT16(save_p, ss->ceilinglightlevel);
				P_WriteUINT8(save_p, ss->ceilinglightabsolute);
			}
			if (diff3 & SD_FLAG)
				P_WriteUINT32(save_p, ss->flags);
			if (diff3 & SD_SPECIALFLAG)
				P_WriteUINT32(save_p, ss->specialflags);
			if (diff4 & SD_DAMAGETYPE)
				P_WriteUINT8(save_p, ss->damagetype);
			if (diff4 & SD_TRIGGERTAG)
				P_WriteINT16(save_p, ss->triggertag);
			if (diff4 & SD_TRIGGERER)
				P_WriteUINT8(save_p, ss->triggerer);
			if (diff4 & SD_FXSCALE)
				P_WriteFixed(save_p, ss->floorxscale);
			if (diff4 & SD_FYSCALE)
				P_WriteFixed(save_p, ss->flooryscale);
			if (diff4 & SD_CXSCALE)
				P_WriteFixed(save_p, ss->ceilingxscale);
			if (diff4 & SD_CYSCALE)
				P_WriteFixed(save_p, ss->ceilingyscale);
			if (diff5 & SD_GRAVITY)
				P_WriteFixed(save_p, ss->gravity);
			if (diff5 & SD_FLOORPORTAL)
				P_WriteUINT32(save_p, ss->portal_floor);
			if (diff5 & SD_CEILPORTAL)
				P_WriteUINT32(save_p, ss->portal_ceiling);
			if (diff & SD_FFLOORS)
				ArchiveFFloors(save_p, ss);
		}
	}

	P_WriteUINT32(save_p, 0xffffffff);
}

static void UnArchiveSectors(save_t *save_p)
{
	UINT32 i;
	UINT16 j;
	UINT8 diff, diff2, diff3, diff4, diff5;
	for (;;)
	{
		i = P_ReadUINT32(save_p);

		if (i == 0xffffffff)
			break;

		if (i > numsectors)
			I_Error("Invalid sector number %u from server (expected end at %s)", i, sizeu1(numsectors));

		diff = P_ReadUINT8(save_p);
		if (diff & SD_DIFF2)
			diff2 = P_ReadUINT8(save_p);
		else
			diff2 = 0;
		if (diff2 & SD_DIFF3)
			diff3 = P_ReadUINT8(save_p);
		else
			diff3 = 0;
		if (diff3 & SD_DIFF4)
			diff4 = P_ReadUINT8(save_p);
		else
			diff4 = 0;
		if (diff4 & SD_DIFF5)
			diff5 = P_ReadUINT8(save_p);
		else
			diff5 = 0;

		if (diff & SD_FLOORHT)
			sectors[i].floorheight = P_ReadFixed(save_p);
		if (diff & SD_CEILHT)
			sectors[i].ceilingheight = P_ReadFixed(save_p);
		if (diff & SD_FLOORPIC)
		{
			sectors[i].floorpic = P_AddLevelFlatRuntime((char *)&save_p->buf[save_p->pos]);
			save_p->pos += 8;
		}
		if (diff & SD_CEILPIC)
		{
			sectors[i].ceilingpic = P_AddLevelFlatRuntime((char *)&save_p->buf[save_p->pos]);
			save_p->pos += 8;
		}
		if (diff & SD_LIGHT)
			sectors[i].lightlevel = P_ReadINT16(save_p);
		if (diff & SD_SPECIAL)
			sectors[i].special = P_ReadINT16(save_p);

		if (diff2 & SD_FXOFFS)
			sectors[i].floorxoffset = P_ReadFixed(save_p);
		if (diff2 & SD_FYOFFS)
			sectors[i].flooryoffset = P_ReadFixed(save_p);
		if (diff2 & SD_CXOFFS)
			sectors[i].ceilingxoffset = P_ReadFixed(save_p);
		if (diff2 & SD_CYOFFS)
			sectors[i].ceilingyoffset = P_ReadFixed(save_p);
		if (diff2 & SD_FLOORANG)
			sectors[i].floorangle  = P_ReadAngle(save_p);
		if (diff2 & SD_CEILANG)
			sectors[i].ceilingangle = P_ReadAngle(save_p);
		if (diff2 & SD_TAG)
		{
			size_t ncount = P_ReadUINT32(save_p);

			// Remove entries from global lists.
			for (j = 0; j < sectors[i].tags.count; j++)
				Taggroup_Remove(tags_sectors, sectors[i].tags.tags[j], i);

			// Reallocate if size differs.
			if (ncount != sectors[i].tags.count)
			{
				sectors[i].tags.count = ncount;
				sectors[i].tags.tags = Z_Realloc(sectors[i].tags.tags, ncount*sizeof(mtag_t), PU_LEVEL, NULL);
			}

			for (j = 0; j < ncount; j++)
				sectors[i].tags.tags[j] = P_ReadINT16(save_p);

			// Add new entries.
			for (j = 0; j < sectors[i].tags.count; j++)
				Taggroup_Remove(tags_sectors, sectors[i].tags.tags[j], i);
		}


		if (diff3 & SD_COLORMAP)
			sectors[i].extra_colormap = GetNetColormapFromList(P_ReadUINT32(save_p));
		if (diff3 & SD_CRUMBLESTATE)
			sectors[i].crumblestate = P_ReadINT32(save_p);
		if (diff3 & SD_FLOORLIGHT)
		{
			sectors[i].floorlightlevel = P_ReadINT16(save_p);
			sectors[i].floorlightabsolute = P_ReadUINT8(save_p);
		}
		if (diff3 & SD_CEILLIGHT)
		{
			sectors[i].ceilinglightlevel = P_ReadINT16(save_p);
			sectors[i].ceilinglightabsolute = P_ReadUINT8(save_p);
		}
		if (diff3 & SD_FLAG)
		{
			sectors[i].flags = P_ReadUINT32(save_p);
			CheckForReverseGravity |= (sectors[i].flags & MSF_GRAVITYFLIP);
		}
		if (diff3 & SD_SPECIALFLAG)
			sectors[i].specialflags = P_ReadUINT32(save_p);
		if (diff4 & SD_DAMAGETYPE)
			sectors[i].damagetype = P_ReadUINT8(save_p);
		if (diff4 & SD_TRIGGERTAG)
			sectors[i].triggertag = P_ReadINT16(save_p);
		if (diff4 & SD_TRIGGERER)
			sectors[i].triggerer = P_ReadUINT8(save_p);
		if (diff4 & SD_FXSCALE)
			sectors[i].floorxscale = P_ReadFixed(save_p);
		if (diff4 & SD_FYSCALE)
			sectors[i].flooryscale = P_ReadFixed(save_p);
		if (diff4 & SD_CXSCALE)
			sectors[i].ceilingxscale = P_ReadFixed(save_p);
		if (diff4 & SD_CYSCALE)
			sectors[i].ceilingyscale = P_ReadFixed(save_p);
		if (diff5 & SD_GRAVITY)
			sectors[i].gravity = P_ReadFixed(save_p);
		if (diff5 & SD_FLOORPORTAL)
			sectors[i].portal_floor = P_ReadUINT32(save_p);
		if (diff5 & SD_CEILPORTAL)
			sectors[i].portal_ceiling = P_ReadUINT32(save_p);

		if (diff & SD_FFLOORS)
			UnArchiveFFloors(save_p, &sectors[i]);
	}
}

static UINT32 GetSideDiff(const side_t *si, const side_t *spawnsi)
{
	UINT32 diff = 0;
	if (si->textureoffset != spawnsi->textureoffset)
		diff |= LD_SDTEXOFFX;
	if (si->rowoffset != spawnsi->rowoffset)
		diff |= LD_SDTEXOFFY;
	//SoM: 4/1/2000: Some textures are colormaps. Don't worry about invalid textures.
	if (si->toptexture != spawnsi->toptexture)
		diff |= LD_SDTOPTEX;
	if (si->bottomtexture != spawnsi->bottomtexture)
		diff |= LD_SDBOTTEX;
	if (si->midtexture != spawnsi->midtexture)
		diff |= LD_SDMIDTEX;
	if (si->offsetx_top != spawnsi->offsetx_top)
		diff |= LD_SDTOPOFFX;
	if (si->offsetx_mid != spawnsi->offsetx_mid)
		diff |= LD_SDMIDOFFX;
	if (si->offsetx_bottom != spawnsi->offsetx_bottom)
		diff |= LD_SDBOTOFFX;
	if (si->offsety_top != spawnsi->offsety_top)
		diff |= LD_SDTOPOFFY;
	if (si->offsety_mid != spawnsi->offsety_mid)
		diff |= LD_SDMIDOFFY;
	if (si->offsety_bottom != spawnsi->offsety_bottom)
		diff |= LD_SDBOTOFFY;
	if (si->scalex_top != spawnsi->scalex_top)
		diff |= LD_SDTOPSCALEX;
	if (si->scalex_mid != spawnsi->scalex_mid)
		diff |= LD_SDMIDSCALEX;
	if (si->scalex_bottom != spawnsi->scalex_bottom)
		diff |= LD_SDBOTSCALEX;
	if (si->scaley_top != spawnsi->scaley_top)
		diff |= LD_SDTOPSCALEY;
	if (si->scaley_mid != spawnsi->scaley_mid)
		diff |= LD_SDMIDSCALEY;
	if (si->scaley_bottom != spawnsi->scaley_bottom)
		diff |= LD_SDBOTSCALEY;
	if (si->repeatcnt != spawnsi->repeatcnt)
		diff |= LD_SDREPEATCNT;
	if (si->light != spawnsi->light)
		diff |= LD_SDLIGHT;
	if (si->light_top != spawnsi->light_top)
		diff |= LD_SDTOPLIGHT;
	if (si->light_mid != spawnsi->light_mid)
		diff |= LD_SDMIDLIGHT;
	if (si->light_bottom != spawnsi->light_bottom)
		diff |= LD_SDBOTLIGHT;
	if (si->lightabsolute != spawnsi->lightabsolute)
		diff |= LD_SDLIGHTABS;
	if (si->lightabsolute_top != spawnsi->lightabsolute_top)
		diff |= LD_SDTOPLIGHTABS;
	if (si->lightabsolute_mid != spawnsi->lightabsolute_mid)
		diff |= LD_SDMIDLIGHTABS;
	if (si->lightabsolute_bottom != spawnsi->lightabsolute_bottom)
		diff |= LD_SDBOTLIGHTABS;
	return diff;
}

static void ArchiveSide(save_t *save_p, const side_t *si, UINT32 diff)
{
	P_WriteUINT32(save_p, diff);

	if (diff & LD_SDTEXOFFX)
		P_WriteFixed(save_p, si->textureoffset);
	if (diff & LD_SDTEXOFFY)
		P_WriteFixed(save_p, si->rowoffset);
	if (diff & LD_SDTOPTEX)
		P_WriteINT32(save_p, si->toptexture);
	if (diff & LD_SDBOTTEX)
		P_WriteINT32(save_p, si->bottomtexture);
	if (diff & LD_SDMIDTEX)
		P_WriteINT32(save_p, si->midtexture);
	if (diff & LD_SDTOPOFFX)
		P_WriteFixed(save_p, si->offsetx_top);
	if (diff & LD_SDMIDOFFX)
		P_WriteFixed(save_p, si->offsetx_mid);
	if (diff & LD_SDBOTOFFX)
		P_WriteFixed(save_p, si->offsetx_bottom);
	if (diff & LD_SDTOPOFFY)
		P_WriteFixed(save_p, si->offsety_top);
	if (diff & LD_SDMIDOFFY)
		P_WriteFixed(save_p, si->offsety_mid);
	if (diff & LD_SDBOTOFFY)
		P_WriteFixed(save_p, si->offsety_bottom);
	if (diff & LD_SDTOPSCALEX)
		P_WriteFixed(save_p, si->scalex_top);
	if (diff & LD_SDMIDSCALEX)
		P_WriteFixed(save_p, si->scalex_mid);
	if (diff & LD_SDBOTSCALEX)
		P_WriteFixed(save_p, si->scalex_bottom);
	if (diff & LD_SDTOPSCALEY)
		P_WriteFixed(save_p, si->scaley_top);
	if (diff & LD_SDMIDSCALEY)
		P_WriteFixed(save_p, si->scaley_mid);
	if (diff & LD_SDBOTSCALEY)
		P_WriteFixed(save_p, si->scaley_bottom);
	if (diff & LD_SDREPEATCNT)
		P_WriteINT16(save_p, si->repeatcnt);
	if (diff & LD_SDLIGHT)
		P_WriteINT16(save_p, si->light);
	if (diff & LD_SDTOPLIGHT)
		P_WriteINT16(save_p, si->light_top);
	if (diff & LD_SDMIDLIGHT)
		P_WriteINT16(save_p, si->light_mid);
	if (diff & LD_SDBOTLIGHT)
		P_WriteINT16(save_p, si->light_bottom);
	if (diff & LD_SDLIGHTABS)
		P_WriteUINT8(save_p, si->lightabsolute);
	if (diff & LD_SDTOPLIGHTABS)
		P_WriteUINT8(save_p, si->lightabsolute_top);
	if (diff & LD_SDMIDLIGHTABS)
		P_WriteUINT8(save_p, si->lightabsolute_mid);
	if (diff & LD_SDBOTLIGHTABS)
		P_WriteUINT8(save_p, si->lightabsolute_bottom);
}

static void ArchiveLines(save_t *save_p)
{
	size_t i;
	const line_t *li = lines;
	const line_t *spawnli = spawnlines;
	UINT8 diff, diff2;
	UINT32 side1diff;
	UINT32 side2diff;

	for (i = 0; i < numlines; i++, spawnli++, li++)
	{
		diff = diff2 = 0;
		side1diff = side2diff = 0;

		if (li->special != spawnli->special)
			diff |= LD_SPECIAL;

		if (spawnli->special == 321 || spawnli->special == 322) // only reason li->callcount would be non-zero is if either of these are involved
			diff |= LD_CLLCOUNT;

		if (!P_AreArgsEqual(li, spawnli))
			diff |= LD_ARGS;

		if (!P_AreStringArgsEqual(li, spawnli))
			diff |= LD_STRINGARGS;

		if (li->executordelay != spawnli->executordelay)
			diff2 |= LD_EXECUTORDELAY;

		if (li->secportal != spawnli->secportal)
			diff2 |= LD_TRANSFPORTAL;

		if (li->sidenum[0] != NO_SIDEDEF)
		{
			side1diff = GetSideDiff(&sides[li->sidenum[0]], &spawnsides[li->sidenum[0]]);
			if (side1diff)
				diff |= LD_SIDE1;
		}
		if (li->sidenum[1] != NO_SIDEDEF)
		{
			side2diff = GetSideDiff(&sides[li->sidenum[1]], &spawnsides[li->sidenum[1]]);
			if (side2diff)
				diff |= LD_SIDE2;
		}

		if (diff2)
			diff |= LD_DIFF2;

		if (diff)
		{
			P_WriteUINT32(save_p, i);
			P_WriteUINT8(save_p, diff);
			if (diff & LD_DIFF2)
				P_WriteUINT8(save_p, diff2);
			if (diff & LD_FLAG)
				P_WriteINT16(save_p, li->flags);
			if (diff & LD_SPECIAL)
				P_WriteINT16(save_p, li->special);
			if (diff & LD_CLLCOUNT)
				P_WriteINT16(save_p, li->callcount);
			if (diff & LD_ARGS)
			{
				UINT8 j;
				for (j = 0; j < NUMLINEARGS; j++)
					P_WriteINT32(save_p, li->args[j]);
			}
			if (diff & LD_STRINGARGS)
			{
				UINT8 j;
				for (j = 0; j < NUMLINESTRINGARGS; j++)
				{
					size_t len, k;

					if (!li->stringargs[j])
					{
						P_WriteINT32(save_p, 0);
						continue;
					}

					len = strlen(li->stringargs[j]);
					P_WriteINT32(save_p, len);
					for (k = 0; k < len; k++)
						P_WriteChar(save_p, li->stringargs[j][k]);
				}
			}
			if (diff & LD_SIDE1)
				ArchiveSide(save_p, &sides[li->sidenum[0]], side1diff);
			if (diff & LD_SIDE2)
				ArchiveSide(save_p, &sides[li->sidenum[1]], side2diff);
			if (diff2 & LD_EXECUTORDELAY)
				P_WriteINT32(save_p, li->executordelay);
			if (diff2 & LD_TRANSFPORTAL)
				P_WriteUINT32(save_p, li->secportal);
		}
	}
	P_WriteUINT32(save_p, 0xffffffff);
}

static void UnArchiveSide(save_t *save_p, side_t *si)
{
	UINT32 diff = P_ReadUINT32(save_p);

	if (diff & LD_SDTEXOFFX)
		si->textureoffset = P_ReadFixed(save_p);
	if (diff & LD_SDTEXOFFY)
		si->rowoffset = P_ReadFixed(save_p);
	if (diff & LD_SDTOPTEX)
		si->toptexture = P_ReadINT32(save_p);
	if (diff & LD_SDBOTTEX)
		si->bottomtexture = P_ReadINT32(save_p);
	if (diff & LD_SDMIDTEX)
		si->midtexture = P_ReadINT32(save_p);
	if (diff & LD_SDTOPOFFX)
		si->offsetx_top = P_ReadFixed(save_p);
	if (diff & LD_SDMIDOFFX)
		si->offsetx_mid = P_ReadFixed(save_p);
	if (diff & LD_SDBOTOFFX)
		si->offsetx_bottom = P_ReadFixed(save_p);
	if (diff & LD_SDTOPOFFY)
		si->offsety_top = P_ReadFixed(save_p);
	if (diff & LD_SDMIDOFFY)
		si->offsety_mid = P_ReadFixed(save_p);
	if (diff & LD_SDBOTOFFY)
		si->offsety_bottom = P_ReadFixed(save_p);
	if (diff & LD_SDTOPSCALEX)
		si->scalex_top = P_ReadFixed(save_p);
	if (diff & LD_SDMIDSCALEX)
		si->scalex_mid = P_ReadFixed(save_p);
	if (diff & LD_SDBOTSCALEX)
		si->scalex_bottom = P_ReadFixed(save_p);
	if (diff & LD_SDTOPSCALEY)
		si->scaley_top = P_ReadFixed(save_p);
	if (diff & LD_SDMIDSCALEY)
		si->scaley_mid = P_ReadFixed(save_p);
	if (diff & LD_SDBOTSCALEY)
		si->scaley_bottom = P_ReadFixed(save_p);
	if (diff & LD_SDREPEATCNT)
		si->repeatcnt = P_ReadINT16(save_p);
	if (diff & LD_SDLIGHT)
		si->light = P_ReadINT16(save_p);
	if (diff & LD_SDTOPLIGHT)
		si->light_top = P_ReadINT16(save_p);
	if (diff & LD_SDMIDLIGHT)
		si->light_mid = P_ReadINT16(save_p);
	if (diff & LD_SDBOTLIGHT)
		si->light_bottom = P_ReadINT16(save_p);
	if (diff & LD_SDLIGHTABS)
		si->lightabsolute = P_ReadUINT8(save_p);
	if (diff & LD_SDTOPLIGHTABS)
		si->lightabsolute_top = P_ReadUINT8(save_p);
	if (diff & LD_SDMIDLIGHTABS)
		si->lightabsolute_mid = P_ReadUINT8(save_p);
	if (diff & LD_SDBOTLIGHTABS)
		si->lightabsolute_bottom = P_ReadUINT8(save_p);
}

static void UnArchiveLines(save_t *save_p)
{
	UINT32 i;
	line_t *li;
	UINT8 diff, diff2;

	for (;;)
	{
		i = P_ReadUINT32(save_p);

		if (i == 0xffffffff)
			break;
		if (i > numlines)
			I_Error("Invalid line number %u from server", i);

		diff = P_ReadUINT8(save_p);
		if (diff & LD_DIFF2)
			diff2 = P_ReadUINT8(save_p);
		else
			diff2 = 0;
		li = &lines[i];

		if (diff & LD_FLAG)
			li->flags = P_ReadINT16(save_p);
		if (diff & LD_SPECIAL)
			li->special = P_ReadINT16(save_p);
		if (diff & LD_CLLCOUNT)
			li->callcount = P_ReadINT16(save_p);
		if (diff & LD_ARGS)
		{
			UINT8 j;
			for (j = 0; j < NUMLINEARGS; j++)
				li->args[j] = P_ReadINT32(save_p);
		}
		if (diff & LD_STRINGARGS)
		{
			UINT8 j;
			for (j = 0; j < NUMLINESTRINGARGS; j++)
			{
				size_t len = P_ReadINT32(save_p);
				size_t k;

				if (!len)
				{
					Z_Free(li->stringargs[j]);
					li->stringargs[j] = NULL;
					continue;
				}

				li->stringargs[j] = Z_Realloc(li->stringargs[j], len + 1, PU_LEVEL, NULL);
				for (k = 0; k < len; k++)
					li->stringargs[j][k] = P_ReadChar(save_p);
				li->stringargs[j][len] = '\0';
			}
		}
		if (diff & LD_SIDE1)
			UnArchiveSide(save_p, &sides[li->sidenum[0]]);
		if (diff & LD_SIDE2)
			UnArchiveSide(save_p, &sides[li->sidenum[1]]);
		if (diff2 & LD_EXECUTORDELAY)
			li->executordelay = P_ReadINT32(save_p);
		if (diff2 & LD_TRANSFPORTAL)
			li->secportal = P_ReadUINT32(save_p);
	}
}

static void P_NetArchiveWorld(save_t *save_p)
{
	// initialize colormap vars because paranoia
	ClearNetColormaps();

	P_WriteUINT32(save_p, ARCHIVEBLOCK_WORLD);

	ArchiveSectors(save_p);
	ArchiveLines(save_p);
	R_ClearTextureNumCache(false);
}

static void P_NetUnArchiveWorld(save_t *save_p)
{
	UINT16 i;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_WORLD)
		I_Error("Bad $$$.sav at archive block World");

	// initialize colormap vars because paranoia
	ClearNetColormaps();

	// count the level's ffloors so that colormap loading can have an upper limit
	for (i = 0; i < numsectors; i++)
	{
		ffloor_t *rover;
		for (rover = sectors[i].ffloors; rover; rover = rover->next)
			num_ffloors++;
	}

	UnArchiveSectors(save_p);
	UnArchiveLines(save_p);
}

//
// Thinkers
//

typedef enum
{
	MD_SPAWNPOINT  = 1,
	MD_POS         = 1<<1,
	MD_TYPE        = 1<<2,
	MD_MOM         = 1<<3,
	MD_RADIUS      = 1<<4,
	MD_HEIGHT      = 1<<5,
	MD_FLAGS       = 1<<6,
	MD_HEALTH      = 1<<7,
	MD_RTIME       = 1<<8,
	MD_STATE       = 1<<9,
	MD_TICS        = 1<<10,
	MD_SPRITE      = 1<<11,
	MD_FRAME       = 1<<12,
	MD_EFLAGS      = 1<<13,
	MD_PLAYER      = 1<<14,
	MD_MOVEDIR     = 1<<15,
	MD_MOVECOUNT   = 1<<16,
	MD_THRESHOLD   = 1<<17,
	MD_LASTLOOK    = 1<<18,
	MD_TARGET      = 1<<19,
	MD_TRACER      = 1<<20,
	MD_FRICTION    = 1<<21,
	MD_MOVEFACTOR  = 1<<22,
	MD_FLAGS2      = 1<<23,
	MD_FUSE        = 1<<24,
	MD_WATERTOP    = 1<<25,
	MD_WATERBOTTOM = 1<<26,
	MD_SCALE       = 1<<27,
	MD_DSCALE      = 1<<28,
	MD_BLUEFLAG    = 1<<29,
	MD_REDFLAG     = 1<<30,
	MD_MORE        = 1<<31
} mobj_diff_t;

typedef enum
{
	MD2_CUSVAL              = 1,
	MD2_CVMEM               = 1<<1,
	MD2_SKIN                = 1<<2,
	MD2_COLOR               = 1<<3,
	MD2_SCALESPEED          = 1<<4,
	MD2_EXTVAL1             = 1<<5,
	MD2_EXTVAL2             = 1<<6,
	MD2_HNEXT               = 1<<7,
	MD2_HPREV               = 1<<8,
	MD2_FLOORROVER          = 1<<9,
	MD2_CEILINGROVER        = 1<<10,
	MD2_SLOPE               = 1<<11,
	MD2_COLORIZED           = 1<<12,
	MD2_MIRRORED            = 1<<13,
	MD2_SPRITEROLL          = 1<<14,
	MD2_SHADOWSCALE         = 1<<15,
	MD2_RENDERFLAGS         = 1<<16,
	MD2_BLENDMODE           = 1<<17,
	MD2_SPRITEXSCALE        = 1<<18,
	MD2_SPRITEYSCALE        = 1<<19,
	MD2_SPRITEXOFFSET       = 1<<20,
	MD2_SPRITEYOFFSET       = 1<<21,
	MD2_FLOORSPRITESLOPE    = 1<<22,
	MD2_DISPOFFSET          = 1<<23,
	MD2_DRAWONLYFORPLAYER   = 1<<24,
	MD2_DONTDRAWFORVIEWMOBJ = 1<<25,
	MD2_TRANSLATION         = 1<<26,
	MD2_ALPHA               = 1<<27
} mobj_diff2_t;

typedef enum
{
	tc_mobj,
	tc_ceiling,
	tc_floor,
	tc_flash,
	tc_strobe,
	tc_glow,
	tc_fireflicker,
	tc_thwomp,
	tc_camerascanner,
	tc_elevator,
	tc_continuousfalling,
	tc_bouncecheese,
	tc_startcrumble,
	tc_marioblock,
	tc_marioblockchecker,
	tc_floatsector,
	tc_crushceiling,
	tc_scroll,
	tc_friction,
	tc_pusher,
	tc_laserflash,
	tc_lightfade,
	tc_executor,
	tc_raisesector,
	tc_noenemies,
	tc_eachtime,
	tc_disappear,
	tc_fade,
	tc_fadecolormap,
	tc_planedisplace,
	tc_dynslopeline,
	tc_dynslopevert,
	tc_polyrotate, // haleyjd 03/26/06: polyobjects
	tc_polymove,
	tc_polywaypoint,
	tc_polyslidedoor,
	tc_polyswingdoor,
	tc_polyflag,
	tc_polydisplace,
	tc_polyrotdisplace,
	tc_polyfade,
	tc_end
} specials_e;

static inline UINT32 SaveMobjnum(const mobj_t *mobj)
{
	if (mobj) return mobj->mobjnum;
	return 0;
}

static UINT32 SaveSector(const sector_t *sector)
{
	if (sector) return (UINT32)(sector - sectors);
	return 0xFFFFFFFF;
}

static UINT32 SaveLine(const line_t *line)
{
	if (line) return (UINT32)(line - lines);
	return 0xFFFFFFFF;
}

static inline UINT32 SavePlayer(const player_t *player)
{
	if (player) return (UINT32)(player - players);
	return 0xFFFFFFFF;
}

static UINT32 SaveSlope(const pslope_t *slope)
{
	if (slope) return (UINT32)(slope->id);
	return 0xFFFFFFFF;
}

static void SaveMobjThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const mobj_t *mobj = (const mobj_t *)th;
	UINT32 diff;
	UINT32 diff2;

	// Ignore stationary hoops - these will be respawned from mapthings.
	if (mobj->type == MT_HOOP)
		return;

	// These are NEVER saved.
	if (mobj->type == MT_HOOPCOLLIDE)
		return;

	// This hoop has already been collected.
	if (mobj->type == MT_HOOPCENTER && mobj->threshold == 4242)
		return;

	if (mobj->spawnpoint && mobj->info->doomednum != -1)
	{
		// spawnpoint is not modified but we must save it since it is an identifier
		diff = MD_SPAWNPOINT;

		if ((mobj->x != mobj->spawnpoint->x << FRACBITS) ||
			(mobj->y != mobj->spawnpoint->y << FRACBITS) ||
			(mobj->angle != FixedAngle(mobj->spawnpoint->angle*FRACUNIT)) ||
			(mobj->pitch != FixedAngle(mobj->spawnpoint->pitch*FRACUNIT)) ||
			(mobj->roll != FixedAngle(mobj->spawnpoint->roll*FRACUNIT)) )
			diff |= MD_POS;

		if (mobj->info->doomednum != mobj->spawnpoint->type)
			diff |= MD_TYPE;
	}
	else
		diff = MD_POS | MD_TYPE; // not a map spawned thing so make it from scratch

	diff2 = 0;

	// not the default but the most probable
	if (mobj->momx != 0 || mobj->momy != 0 || mobj->momz != 0 || mobj->pmomz !=0)
		diff |= MD_MOM;
	if (mobj->radius != mobj->info->radius)
		diff |= MD_RADIUS;
	if (mobj->height != mobj->info->height)
		diff |= MD_HEIGHT;
	if (mobj->flags != mobj->info->flags)
		diff |= MD_FLAGS;
	if (mobj->flags2)
		diff |= MD_FLAGS2;
	if (mobj->health != mobj->info->spawnhealth)
		diff |= MD_HEALTH;
	if (mobj->reactiontime != mobj->info->reactiontime)
		diff |= MD_RTIME;
	if ((statenum_t)(mobj->state-states) != mobj->info->spawnstate)
		diff |= MD_STATE;
	if (mobj->tics != mobj->state->tics)
		diff |= MD_TICS;
	if (mobj->sprite != mobj->state->sprite)
		diff |= MD_SPRITE;
	if (mobj->sprite == SPR_PLAY && mobj->sprite2 != P_GetStateSprite2(mobj->state))
		diff |= MD_SPRITE;
	if (mobj->frame != mobj->state->frame)
		diff |= MD_FRAME;
	if (mobj->anim_duration != (UINT16)mobj->state->var2)
		diff |= MD_FRAME;
	if (mobj->eflags)
		diff |= MD_EFLAGS;
	if (mobj->player)
		diff |= MD_PLAYER;

	if (mobj->movedir)
		diff |= MD_MOVEDIR;
	if (mobj->movecount)
		diff |= MD_MOVECOUNT;
	if (mobj->threshold)
		diff |= MD_THRESHOLD;
	if (mobj->lastlook != -1)
		diff |= MD_LASTLOOK;
	if (mobj->target)
		diff |= MD_TARGET;
	if (mobj->tracer)
		diff |= MD_TRACER;
	if (mobj->friction != ORIG_FRICTION)
		diff |= MD_FRICTION;
	if (mobj->movefactor != FRACUNIT)
		diff |= MD_MOVEFACTOR;
	if (mobj->fuse)
		diff |= MD_FUSE;
	if (mobj->watertop)
		diff |= MD_WATERTOP;
	if (mobj->waterbottom)
		diff |= MD_WATERBOTTOM;
	if (mobj->scale != FRACUNIT)
		diff |= MD_SCALE;
	if (mobj->destscale != mobj->scale)
		diff |= MD_DSCALE;
	if (mobj->scalespeed != FRACUNIT/12)
		diff2 |= MD2_SCALESPEED;

	if (mobj == redflag)
		diff |= MD_REDFLAG;
	if (mobj == blueflag)
		diff |= MD_BLUEFLAG;

	if (mobj->cusval)
		diff2 |= MD2_CUSVAL;
	if (mobj->cvmem)
		diff2 |= MD2_CVMEM;
	if (mobj->color)
		diff2 |= MD2_COLOR;
	if (mobj->translation)
		diff2 |= MD2_TRANSLATION;
	if (mobj->skin)
		diff2 |= MD2_SKIN;
	if (mobj->extravalue1)
		diff2 |= MD2_EXTVAL1;
	if (mobj->extravalue2)
		diff2 |= MD2_EXTVAL2;
	if (mobj->hnext)
		diff2 |= MD2_HNEXT;
	if (mobj->hprev)
		diff2 |= MD2_HPREV;
	if (mobj->floorrover)
		diff2 |= MD2_FLOORROVER;
	if (mobj->ceilingrover)
		diff2 |= MD2_CEILINGROVER;
	if (mobj->standingslope)
		diff2 |= MD2_SLOPE;
	if (mobj->colorized)
		diff2 |= MD2_COLORIZED;
	if (mobj->mirrored)
		diff2 |= MD2_MIRRORED;
	if (mobj->spriteroll)
		diff2 |= MD2_SPRITEROLL;
	if (mobj->shadowscale)
		diff2 |= MD2_SHADOWSCALE;
	if (mobj->renderflags)
		diff2 |= MD2_RENDERFLAGS;
	if (mobj->blendmode != AST_TRANSLUCENT)
		diff2 |= MD2_BLENDMODE;
	if (mobj->spritexscale != FRACUNIT)
		diff2 |= MD2_SPRITEXSCALE;
	if (mobj->spriteyscale != FRACUNIT)
		diff2 |= MD2_SPRITEYSCALE;
	if (mobj->spritexoffset)
		diff2 |= MD2_SPRITEXOFFSET;
	if (mobj->spriteyoffset)
		diff2 |= MD2_SPRITEYOFFSET;
	if (mobj->floorspriteslope)
	{
		pslope_t *slope = mobj->floorspriteslope;
		if (slope->zangle || slope->zdelta || slope->xydirection
		|| slope->o.x || slope->o.y || slope->o.z
		|| slope->d.x || slope->d.y
		|| slope->normal.x || slope->normal.y
		|| (slope->normal.z != FRACUNIT))
			diff2 |= MD2_FLOORSPRITESLOPE;
	}
	if (mobj->drawonlyforplayer)
		diff2 |= MD2_DRAWONLYFORPLAYER;
	if (mobj->dontdrawforviewmobj)
		diff2 |= MD2_DONTDRAWFORVIEWMOBJ;
	if (mobj->dispoffset != mobj->info->dispoffset)
		diff2 |= MD2_DISPOFFSET;
	if (mobj->alpha != FRACUNIT)
		diff2 |= MD2_ALPHA;

	if (diff2 != 0)
		diff |= MD_MORE;

	// Scrap all of that. If we're a hoop center, this is ALL we're saving.
	if (mobj->type == MT_HOOPCENTER)
		diff = MD_SPAWNPOINT;

	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, diff);
	if (diff & MD_MORE)
		P_WriteUINT32(save_p, diff2);

	// save pointer, at load time we will search this pointer to reinitilize pointers
	P_WriteUINT32(save_p, (size_t)mobj);

	P_WriteFixed(save_p, mobj->z); // Force this so 3dfloor problems don't arise.
	P_WriteFixed(save_p, mobj->floorz);
	P_WriteFixed(save_p, mobj->ceilingz);

	if (diff2 & MD2_FLOORROVER)
	{
		P_WriteUINT32(save_p, SaveSector(mobj->floorrover->target));
		P_WriteUINT16(save_p, P_GetFFloorID(mobj->floorrover));
	}

	if (diff2 & MD2_CEILINGROVER)
	{
		P_WriteUINT32(save_p, SaveSector(mobj->ceilingrover->target));
		P_WriteUINT16(save_p, P_GetFFloorID(mobj->ceilingrover));
	}

	if (diff & MD_SPAWNPOINT)
	{
		size_t z;

		for (z = 0; z < nummapthings; z++)
			if (&mapthings[z] == mobj->spawnpoint)
				P_WriteUINT16(save_p, z);
		if (mobj->type == MT_HOOPCENTER)
			return;
	}

	if (diff & MD_TYPE)
		P_WriteUINT32(save_p, mobj->type);
	if (diff & MD_POS)
	{
		P_WriteFixed(save_p, mobj->x);
		P_WriteFixed(save_p, mobj->y);
		P_WriteAngle(save_p, mobj->angle);
		P_WriteAngle(save_p, mobj->pitch);
		P_WriteAngle(save_p, mobj->roll);
	}
	if (diff & MD_MOM)
	{
		P_WriteFixed(save_p, mobj->momx);
		P_WriteFixed(save_p, mobj->momy);
		P_WriteFixed(save_p, mobj->momz);
		P_WriteFixed(save_p, mobj->pmomz);
	}
	if (diff & MD_RADIUS)
		P_WriteFixed(save_p, mobj->radius);
	if (diff & MD_HEIGHT)
		P_WriteFixed(save_p, mobj->height);
	if (diff & MD_FLAGS)
		P_WriteUINT32(save_p, mobj->flags);
	if (diff & MD_FLAGS2)
		P_WriteUINT32(save_p, mobj->flags2);
	if (diff & MD_HEALTH)
		P_WriteINT32(save_p, mobj->health);
	if (diff & MD_RTIME)
		P_WriteINT32(save_p, mobj->reactiontime);
	if (diff & MD_STATE)
		P_WriteUINT16(save_p, mobj->state-states);
	if (diff & MD_TICS)
		P_WriteINT32(save_p, mobj->tics);
	if (diff & MD_SPRITE) {
		P_WriteUINT16(save_p, mobj->sprite);
		if (mobj->sprite == SPR_PLAY)
			P_WriteUINT16(save_p, mobj->sprite2);
	}
	if (diff & MD_FRAME)
	{
		P_WriteUINT32(save_p, mobj->frame);
		P_WriteUINT16(save_p, mobj->anim_duration);
	}
	if (diff & MD_EFLAGS)
		P_WriteUINT16(save_p, mobj->eflags);
	if (diff & MD_PLAYER)
		P_WriteUINT8(save_p, mobj->player-players);
	if (diff & MD_MOVEDIR)
		P_WriteAngle(save_p, mobj->movedir);
	if (diff & MD_MOVECOUNT)
		P_WriteINT32(save_p, mobj->movecount);
	if (diff & MD_THRESHOLD)
		P_WriteINT32(save_p, mobj->threshold);
	if (diff & MD_LASTLOOK)
		P_WriteINT32(save_p, mobj->lastlook);
	if (diff & MD_TARGET)
		P_WriteUINT32(save_p, mobj->target->mobjnum);
	if (diff & MD_TRACER)
		P_WriteUINT32(save_p, mobj->tracer->mobjnum);
	if (diff & MD_FRICTION)
		P_WriteFixed(save_p, mobj->friction);
	if (diff & MD_MOVEFACTOR)
		P_WriteFixed(save_p, mobj->movefactor);
	if (diff & MD_FUSE)
		P_WriteINT32(save_p, mobj->fuse);
	if (diff & MD_WATERTOP)
		P_WriteFixed(save_p, mobj->watertop);
	if (diff & MD_WATERBOTTOM)
		P_WriteFixed(save_p, mobj->waterbottom);
	if (diff & MD_SCALE)
		P_WriteFixed(save_p, mobj->scale);
	if (diff & MD_DSCALE)
		P_WriteFixed(save_p, mobj->destscale);
	if (diff2 & MD2_SCALESPEED)
		P_WriteFixed(save_p, mobj->scalespeed);
	if (diff2 & MD2_CUSVAL)
		P_WriteINT32(save_p, mobj->cusval);
	if (diff2 & MD2_CVMEM)
		P_WriteINT32(save_p, mobj->cvmem);
	if (diff2 & MD2_SKIN)
		P_WriteUINT8(save_p, (UINT8)(((skin_t *)mobj->skin)->skinnum));
	if (diff2 & MD2_COLOR)
		P_WriteUINT16(save_p, mobj->color);
	if (diff2 & MD2_EXTVAL1)
		P_WriteINT32(save_p, mobj->extravalue1);
	if (diff2 & MD2_EXTVAL2)
		P_WriteINT32(save_p, mobj->extravalue2);
	if (diff2 & MD2_HNEXT)
		P_WriteUINT32(save_p, mobj->hnext->mobjnum);
	if (diff2 & MD2_HPREV)
		P_WriteUINT32(save_p, mobj->hprev->mobjnum);
	if (diff2 & MD2_SLOPE)
		P_WriteUINT16(save_p, mobj->standingslope->id);
	if (diff2 & MD2_COLORIZED)
		P_WriteUINT8(save_p, mobj->colorized);
	if (diff2 & MD2_MIRRORED)
		P_WriteUINT8(save_p, mobj->mirrored);
	if (diff2 & MD2_SPRITEROLL)
		P_WriteAngle(save_p, mobj->spriteroll);
	if (diff2 & MD2_SHADOWSCALE)
		P_WriteFixed(save_p, mobj->shadowscale);
	if (diff2 & MD2_RENDERFLAGS)
		P_WriteUINT32(save_p, mobj->renderflags);
	if (diff2 & MD2_BLENDMODE)
		P_WriteINT32(save_p, mobj->blendmode);
	if (diff2 & MD2_SPRITEXSCALE)
		P_WriteFixed(save_p, mobj->spritexscale);
	if (diff2 & MD2_SPRITEYSCALE)
		P_WriteFixed(save_p, mobj->spriteyscale);
	if (diff2 & MD2_SPRITEXOFFSET)
		P_WriteFixed(save_p, mobj->spritexoffset);
	if (diff2 & MD2_SPRITEYOFFSET)
		P_WriteFixed(save_p, mobj->spriteyoffset);
	if (diff2 & MD2_FLOORSPRITESLOPE)
	{
		pslope_t *slope = mobj->floorspriteslope;

		P_WriteFixed(save_p, slope->zdelta);
		P_WriteAngle(save_p, slope->zangle);
		P_WriteAngle(save_p, slope->xydirection);

		P_WriteFixed(save_p, slope->o.x);
		P_WriteFixed(save_p, slope->o.y);
		P_WriteFixed(save_p, slope->o.z);

		P_WriteFixed(save_p, slope->d.x);
		P_WriteFixed(save_p, slope->d.y);

		P_WriteFixed(save_p, slope->normal.x);
		P_WriteFixed(save_p, slope->normal.y);
		P_WriteFixed(save_p, slope->normal.z);
	}
	if (diff2 & MD2_DRAWONLYFORPLAYER)
		P_WriteUINT8(save_p, mobj->drawonlyforplayer-players);
	if (diff2 & MD2_DONTDRAWFORVIEWMOBJ)
		P_WriteUINT32(save_p, mobj->dontdrawforviewmobj->mobjnum);
	if (diff2 & MD2_DISPOFFSET)
		P_WriteINT32(save_p, mobj->dispoffset);
	if (diff2 & MD2_TRANSLATION)
		P_WriteUINT16(save_p, mobj->translation);
	if (diff2 & MD2_ALPHA)
		P_WriteFixed(save_p, mobj->alpha);

	P_WriteUINT32(save_p, mobj->mobjnum);
}

static void SaveNoEnemiesThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const noenemies_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
}

static void SaveBounceCheeseThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const bouncecheese_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteFixed(save_p, ht->speed);
	P_WriteFixed(save_p, ht->distance);
	P_WriteFixed(save_p, ht->floorwasheight);
	P_WriteFixed(save_p, ht->ceilingwasheight);
	P_WriteChar(save_p, ht->low);
}

static void SaveContinuousFallThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const continuousfall_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteFixed(save_p, ht->speed);
	P_WriteINT32(save_p, ht->direction);
	P_WriteFixed(save_p, ht->floorstartheight);
	P_WriteFixed(save_p, ht->ceilingstartheight);
	P_WriteFixed(save_p, ht->destheight);
}

static void SaveMarioBlockThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const mariothink_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteFixed(save_p, ht->speed);
	P_WriteINT32(save_p, ht->direction);
	P_WriteFixed(save_p, ht->floorstartheight);
	P_WriteFixed(save_p, ht->ceilingstartheight);
	P_WriteINT16(save_p, ht->tag);
}

static void SaveMarioCheckThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const mariocheck_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteUINT32(save_p, SaveSector(ht->sector));
}

static void SaveThwompThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const thwomp_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteFixed(save_p, ht->crushspeed);
	P_WriteFixed(save_p, ht->retractspeed);
	P_WriteINT32(save_p, ht->direction);
	P_WriteFixed(save_p, ht->floorstartheight);
	P_WriteFixed(save_p, ht->ceilingstartheight);
	P_WriteINT32(save_p, ht->delay);
	P_WriteINT16(save_p, ht->tag);
	P_WriteUINT16(save_p, ht->sound);
}

static void SaveFloatThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const floatthink_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT16(save_p, ht->tag);
}

static void SaveEachTimeThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const eachtime_t *ht  = (const void *)th;
	size_t i;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	for (i = 0; i < MAXPLAYERS; i++)
	{
		P_WriteChar(save_p, ht->playersInArea[i]);
	}
	P_WriteChar(save_p, ht->triggerOnExit);
}

static void SaveRaiseThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const raise_t *ht  = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT16(save_p, ht->tag);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteFixed(save_p, ht->ceilingbottom);
	P_WriteFixed(save_p, ht->ceilingtop);
	P_WriteFixed(save_p, ht->basespeed);
	P_WriteFixed(save_p, ht->extraspeed);
	P_WriteUINT8(save_p, ht->shaketimer);
	P_WriteUINT8(save_p, ht->flags);
}

static void SaveCeilingThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const ceiling_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT8(save_p, ht->type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteFixed(save_p, ht->bottomheight);
	P_WriteFixed(save_p, ht->topheight);
	P_WriteFixed(save_p, ht->speed);
	P_WriteFixed(save_p, ht->delay);
	P_WriteFixed(save_p, ht->delaytimer);
	P_WriteUINT8(save_p, ht->crush);
	P_WriteINT32(save_p, ht->texture);
	P_WriteINT32(save_p, ht->direction);
	P_WriteINT16(save_p, ht->tag);
	P_WriteFixed(save_p, ht->origspeed);
	P_WriteFixed(save_p, ht->sourceline);
}

static void SaveFloormoveThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const floormove_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT8(save_p, ht->type);
	P_WriteUINT8(save_p, ht->crush);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT32(save_p, ht->direction);
	P_WriteINT32(save_p, ht->texture);
	P_WriteFixed(save_p, ht->floordestheight);
	P_WriteFixed(save_p, ht->speed);
	P_WriteFixed(save_p, ht->origspeed);
	P_WriteFixed(save_p, ht->delay);
	P_WriteFixed(save_p, ht->delaytimer);
	P_WriteINT16(save_p, ht->tag);
	P_WriteFixed(save_p, ht->sourceline);
}

static void SaveLightflashThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const lightflash_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT32(save_p, ht->maxlight);
	P_WriteINT32(save_p, ht->minlight);
}

static void SaveStrobeThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const strobe_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT32(save_p, ht->count);
	P_WriteINT16(save_p, ht->minlight);
	P_WriteINT16(save_p, ht->maxlight);
	P_WriteINT32(save_p, ht->darktime);
	P_WriteINT32(save_p, ht->brighttime);
}

static void SaveGlowThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const glow_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT16(save_p, ht->minlight);
	P_WriteINT16(save_p, ht->maxlight);
	P_WriteINT16(save_p, ht->direction);
	P_WriteINT16(save_p, ht->speed);
}

static inline void SaveFireflickerThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const fireflicker_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT32(save_p, ht->count);
	P_WriteINT32(save_p, ht->resetcount);
	P_WriteINT16(save_p, ht->maxlight);
	P_WriteINT16(save_p, ht->minlight);
}

static void SaveElevatorThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const elevator_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT8(save_p, ht->type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteUINT32(save_p, SaveSector(ht->actionsector));
	P_WriteINT32(save_p, ht->direction);
	P_WriteFixed(save_p, ht->floordestheight);
	P_WriteFixed(save_p, ht->ceilingdestheight);
	P_WriteFixed(save_p, ht->speed);
	P_WriteFixed(save_p, ht->origspeed);
	P_WriteFixed(save_p, ht->low);
	P_WriteFixed(save_p, ht->high);
	P_WriteFixed(save_p, ht->distance);
	P_WriteFixed(save_p, ht->delay);
	P_WriteFixed(save_p, ht->delaytimer);
	P_WriteFixed(save_p, ht->floorwasheight);
	P_WriteFixed(save_p, ht->ceilingwasheight);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
}

static void SaveCrumbleThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const crumble_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteUINT32(save_p, SaveSector(ht->actionsector));
	P_WriteUINT32(save_p, SavePlayer(ht->player)); // was dummy
	P_WriteINT32(save_p, ht->direction);
	P_WriteINT32(save_p, ht->origalpha);
	P_WriteINT32(save_p, ht->timer);
	P_WriteFixed(save_p, ht->speed);
	P_WriteFixed(save_p, ht->floorwasheight);
	P_WriteFixed(save_p, ht->ceilingwasheight);
	P_WriteUINT8(save_p, ht->flags);
}

static inline void SaveScrollThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const scroll_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteFixed(save_p, ht->dx);
	P_WriteFixed(save_p, ht->dy);
	P_WriteINT32(save_p, ht->affectee);
	P_WriteINT32(save_p, ht->control);
	P_WriteFixed(save_p, ht->last_height);
	P_WriteFixed(save_p, ht->vdx);
	P_WriteFixed(save_p, ht->vdy);
	P_WriteINT32(save_p, ht->accel);
	P_WriteINT32(save_p, ht->exclusive);
	P_WriteUINT8(save_p, ht->type);
}

static inline void SaveFrictionThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const friction_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->friction);
	P_WriteINT32(save_p, ht->movefactor);
	P_WriteINT32(save_p, ht->affectee);
	P_WriteINT32(save_p, ht->referrer);
	P_WriteUINT8(save_p, ht->roverfriction);
}

static inline void SavePusherThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const pusher_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT8(save_p, ht->type);
	P_WriteFixed(save_p, ht->x_mag);
	P_WriteFixed(save_p, ht->y_mag);
	P_WriteFixed(save_p, ht->z_mag);
	P_WriteINT32(save_p, ht->affectee);
	P_WriteUINT8(save_p, ht->roverpusher);
	P_WriteINT32(save_p, ht->referrer);
	P_WriteINT32(save_p, ht->exclusive);
	P_WriteINT32(save_p, ht->slider);
}

static void SaveLaserThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const laserthink_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT16(save_p, ht->tag);
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteUINT8(save_p, ht->nobosses);
}

static void SaveLightlevelThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const lightlevel_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT16(save_p, ht->sourcelevel);
	P_WriteINT16(save_p, ht->destlevel);
	P_WriteFixed(save_p, ht->fixedcurlevel);
	P_WriteFixed(save_p, ht->fixedpertic);
	P_WriteINT32(save_p, ht->timer);
}

static void SaveExecutorThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const executor_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveLine(ht->line));
	P_WriteUINT32(save_p, SaveMobjnum(ht->caller));
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteINT32(save_p, ht->timer);
}

static void SaveDisappearThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const disappear_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, ht->appeartime);
	P_WriteUINT32(save_p, ht->disappeartime);
	P_WriteUINT32(save_p, ht->offset);
	P_WriteUINT32(save_p, ht->timer);
	P_WriteINT32(save_p, ht->affectee);
	P_WriteINT32(save_p, ht->sourceline);
	P_WriteINT32(save_p, ht->exists);
}

static void SaveFadeThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const fade_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, CheckAddNetColormapToList(ht->dest_exc));
	P_WriteUINT32(save_p, ht->sectornum);
	P_WriteUINT32(save_p, ht->ffloornum);
	P_WriteINT32(save_p, ht->alpha);
	P_WriteINT16(save_p, ht->sourcevalue);
	P_WriteINT16(save_p, ht->destvalue);
	P_WriteINT16(save_p, ht->destlightlevel);
	P_WriteINT16(save_p, ht->speed);
	P_WriteUINT8(save_p, (UINT8)ht->ticbased);
	P_WriteINT32(save_p, ht->timer);
	P_WriteUINT8(save_p, ht->doexists);
	P_WriteUINT8(save_p, ht->dotranslucent);
	P_WriteUINT8(save_p, ht->dolighting);
	P_WriteUINT8(save_p, ht->docolormap);
	P_WriteUINT8(save_p, ht->docollision);
	P_WriteUINT8(save_p, ht->doghostfade);
	P_WriteUINT8(save_p, ht->exactalpha);
}

static void SaveFadeColormapThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const fadecolormap_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSector(ht->sector));
	P_WriteUINT32(save_p, CheckAddNetColormapToList(ht->source_exc));
	P_WriteUINT32(save_p, CheckAddNetColormapToList(ht->dest_exc));
	P_WriteUINT8(save_p, (UINT8)ht->ticbased);
	P_WriteINT32(save_p, ht->duration);
	P_WriteINT32(save_p, ht->timer);
}

static void SavePlaneDisplaceThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const planedisplace_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->affectee);
	P_WriteINT32(save_p, ht->control);
	P_WriteFixed(save_p, ht->last_height);
	P_WriteFixed(save_p, ht->speed);
	P_WriteUINT8(save_p, ht->type);
}

static inline void SaveDynamicLineSlopeThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const dynlineplanethink_t* ht = (const void*)th;

	P_WriteUINT8(save_p, type);
	P_WriteUINT8(save_p, ht->type);
	P_WriteUINT32(save_p, SaveSlope(ht->slope));
	P_WriteUINT32(save_p, SaveLine(ht->sourceline));
	P_WriteFixed(save_p, ht->extent);
}

static inline void SaveDynamicVertexSlopeThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	size_t i;
	const dynvertexplanethink_t* ht = (const void*)th;

	P_WriteUINT8(save_p, type);
	P_WriteUINT32(save_p, SaveSlope(ht->slope));
	for (i = 0; i < 3; i++)
		P_WriteUINT32(save_p, SaveSector(ht->secs[i]));
	P_WriteMem(save_p, ht->vex, sizeof(ht->vex));
	P_WriteMem(save_p, ht->origsecheights, sizeof(ht->origsecheights));
	P_WriteMem(save_p, ht->origvecheights, sizeof(ht->origvecheights));
	P_WriteUINT8(save_p, ht->relative);
}

static inline void SavePolyrotatetThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polyrotate_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteINT32(save_p, ht->speed);
	P_WriteINT32(save_p, ht->distance);
	P_WriteUINT8(save_p, ht->turnobjs);
}

static void SavePolymoveThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polymove_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteINT32(save_p, ht->speed);
	P_WriteFixed(save_p, ht->momx);
	P_WriteFixed(save_p, ht->momy);
	P_WriteINT32(save_p, ht->distance);
	P_WriteAngle(save_p, ht->angle);
}

static void SavePolywaypointThinker(save_t *save_p, const thinker_t *th, UINT8 type)
{
	const polywaypoint_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteINT32(save_p, ht->speed);
	P_WriteINT32(save_p, ht->sequence);
	P_WriteINT32(save_p, ht->pointnum);
	P_WriteINT32(save_p, ht->direction);
	P_WriteUINT8(save_p, ht->returnbehavior);
	P_WriteUINT8(save_p, ht->continuous);
	P_WriteUINT8(save_p, ht->stophere);
}

static void SavePolyslidedoorThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polyslidedoor_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteINT32(save_p, ht->delay);
	P_WriteINT32(save_p, ht->delayCount);
	P_WriteINT32(save_p, ht->initSpeed);
	P_WriteINT32(save_p, ht->speed);
	P_WriteINT32(save_p, ht->initDistance);
	P_WriteINT32(save_p, ht->distance);
	P_WriteUINT32(save_p, ht->initAngle);
	P_WriteUINT32(save_p, ht->angle);
	P_WriteUINT32(save_p, ht->revAngle);
	P_WriteFixed(save_p, ht->momx);
	P_WriteFixed(save_p, ht->momy);
	P_WriteUINT8(save_p, ht->closing);
}

static void SavePolyswingdoorThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polyswingdoor_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteINT32(save_p, ht->delay);
	P_WriteINT32(save_p, ht->delayCount);
	P_WriteINT32(save_p, ht->initSpeed);
	P_WriteINT32(save_p, ht->speed);
	P_WriteINT32(save_p, ht->initDistance);
	P_WriteINT32(save_p, ht->distance);
	P_WriteUINT8(save_p, ht->closing);
}

static void SavePolydisplaceThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polydisplace_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteUINT32(save_p, SaveSector(ht->controlSector));
	P_WriteFixed(save_p, ht->dx);
	P_WriteFixed(save_p, ht->dy);
	P_WriteFixed(save_p, ht->oldHeights);
}

static void SavePolyrotdisplaceThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polyrotdisplace_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteUINT32(save_p, SaveSector(ht->controlSector));
	P_WriteFixed(save_p, ht->rotscale);
	P_WriteUINT8(save_p, ht->turnobjs);
	P_WriteFixed(save_p, ht->oldHeights);
}

static void SavePolyfadeThinker(save_t *save_p, const thinker_t *th, const UINT8 type)
{
	const polyfade_t *ht = (const void *)th;
	P_WriteUINT8(save_p, type);
	P_WriteINT32(save_p, ht->polyObjNum);
	P_WriteINT32(save_p, ht->sourcevalue);
	P_WriteINT32(save_p, ht->destvalue);
	P_WriteUINT8(save_p, (UINT8)ht->docollision);
	P_WriteUINT8(save_p, (UINT8)ht->doghostfade);
	P_WriteUINT8(save_p, (UINT8)ht->ticbased);
	P_WriteINT32(save_p, ht->duration);
	P_WriteINT32(save_p, ht->timer);
}

static void P_NetArchiveThinkers(save_t *save_p)
{
	const thinker_t *th;
	UINT32 i;

	P_WriteUINT32(save_p, ARCHIVEBLOCK_THINKERS);

	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		UINT32 numsaved = 0;
		// save off the current thinkers
		for (th = thlist[i].next; th != &thlist[i]; th = th->next)
		{
			if (!(th->removing || th->function.acp1 == (actionf_p1)P_NullPrecipThinker))
				numsaved++;

			if (th->function.acp1 == (actionf_p1)P_MobjThinker)
			{
				SaveMobjThinker(save_p, th, tc_mobj);
				continue;
			}
	#ifdef PARANOIA
			else if (th->function.acp1 == (actionf_p1)P_NullPrecipThinker);
	#endif
			else if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
			{
				SaveCeilingThinker(save_p, th, tc_ceiling);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_CrushCeiling)
			{
				SaveCeilingThinker(save_p, th, tc_crushceiling);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MoveFloor)
			{
				SaveFloormoveThinker(save_p, th, tc_floor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_LightningFlash)
			{
				SaveLightflashThinker(save_p, th, tc_flash);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
			{
				SaveStrobeThinker(save_p, th, tc_strobe);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Glow)
			{
				SaveGlowThinker(save_p, th, tc_glow);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_FireFlicker)
			{
				SaveFireflickerThinker(save_p, th, tc_fireflicker);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MoveElevator)
			{
				SaveElevatorThinker(save_p, th, tc_elevator);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_ContinuousFalling)
			{
				SaveContinuousFallThinker(save_p, th, tc_continuousfalling);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_ThwompSector)
			{
				SaveThwompThinker(save_p, th, tc_thwomp);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_NoEnemiesSector)
			{
				SaveNoEnemiesThinker(save_p, th, tc_noenemies);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_EachTimeThinker)
			{
				SaveEachTimeThinker(save_p, th, tc_eachtime);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_RaiseSector)
			{
				SaveRaiseThinker(save_p, th, tc_raisesector);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_CameraScanner)
			{
				SaveElevatorThinker(save_p, th, tc_camerascanner);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Scroll)
			{
				SaveScrollThinker(save_p, th, tc_scroll);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Friction)
			{
				SaveFrictionThinker(save_p, th, tc_friction);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Pusher)
			{
				SavePusherThinker(save_p, th, tc_pusher);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_BounceCheese)
			{
				SaveBounceCheeseThinker(save_p, th, tc_bouncecheese);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_StartCrumble)
			{
				SaveCrumbleThinker(save_p, th, tc_startcrumble);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MarioBlock)
			{
				SaveMarioBlockThinker(save_p, th, tc_marioblock);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_MarioBlockChecker)
			{
				SaveMarioCheckThinker(save_p, th, tc_marioblockchecker);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_FloatSector)
			{
				SaveFloatThinker(save_p, th, tc_floatsector);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_LaserFlash)
			{
				SaveLaserThinker(save_p, th, tc_laserflash);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_LightFade)
			{
				SaveLightlevelThinker(save_p, th, tc_lightfade);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_ExecutorDelay)
			{
				SaveExecutorThinker(save_p, th, tc_executor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Disappear)
			{
				SaveDisappearThinker(save_p, th, tc_disappear);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_Fade)
			{
				SaveFadeThinker(save_p, th, tc_fade);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_FadeColormap)
			{
				SaveFadeColormapThinker(save_p, th, tc_fadecolormap);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PlaneDisplace)
			{
				SavePlaneDisplaceThinker(save_p, th, tc_planedisplace);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjRotate)
			{
				SavePolyrotatetThinker(save_p, th, tc_polyrotate);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjMove)
			{
				SavePolymoveThinker(save_p, th, tc_polymove);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjWaypoint)
			{
				SavePolywaypointThinker(save_p, th, tc_polywaypoint);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyDoorSlide)
			{
				SavePolyslidedoorThinker(save_p, th, tc_polyslidedoor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyDoorSwing)
			{
				SavePolyswingdoorThinker(save_p, th, tc_polyswingdoor);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjFlag)
			{
				SavePolymoveThinker(save_p, th, tc_polyflag);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjDisplace)
			{
				SavePolydisplaceThinker(save_p, th, tc_polydisplace);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjRotDisplace)
			{
				SavePolyrotdisplaceThinker(save_p, th, tc_polyrotdisplace);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_PolyObjFade)
			{
				SavePolyfadeThinker(save_p, th, tc_polyfade);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_DynamicSlopeLine)
			{
				SaveDynamicLineSlopeThinker(save_p, th, tc_dynslopeline);
				continue;
			}
			else if (th->function.acp1 == (actionf_p1)T_DynamicSlopeVert)
			{
				SaveDynamicVertexSlopeThinker(save_p, th, tc_dynslopevert);
				continue;
			}
#ifdef PARANOIA
			else
				I_Assert(th->removing); // wait garbage collection
#endif
		}

		CONS_Debug(DBG_NETPLAY, "%u thinkers saved in list %d\n", numsaved, i);

		P_WriteUINT8(save_p, tc_end);
	}
}

// Now save the pointers, tracer and target, but at load time we must
// relink to this; the savegame contains the old position in the pointer
// field copyed in the info field temporarily, but finally we just search
// for the old position and relink to it.
mobj_t *P_FindNewPosition(UINT32 oldposition)
{
	thinker_t *th;
	mobj_t *mobj;

	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mobj = (mobj_t *)th;
		if (mobj->mobjnum != oldposition)
			continue;

		return mobj;
	}
	CONS_Debug(DBG_GAMELOGIC, "mobj not found\n");
	return NULL;
}

static inline mobj_t *LoadMobj(UINT32 mobjnum)
{
	if (mobjnum == 0) return NULL;
	return (mobj_t *)(size_t)mobjnum;
}

static sector_t *LoadSector(UINT32 sector)
{
	if (sector >= numsectors) return NULL;
	return &sectors[sector];
}

static line_t *LoadLine(UINT32 line)
{
	if (line >= numlines) return NULL;
	return &lines[line];
}

static inline player_t *LoadPlayer(UINT32 player)
{
	if (player >= MAXPLAYERS) return NULL;
	return &players[player];
}

static inline pslope_t *LoadSlope(UINT32 slopeid)
{
	pslope_t *p = slopelist;
	if (slopeid > slopecount) return NULL;
	do
	{
		if (p->id == slopeid)
			return p;
	} while ((p = p->next));
	return NULL;
}

static thinker_t* LoadMobjThinker(save_t *save_p, actionf_p1 thinker)
{
	thinker_t *next;
	mobj_t *mobj;
	UINT32 diff;
	UINT32 diff2;
	INT32 i;
	fixed_t z, floorz, ceilingz;
	ffloor_t *floorrover = NULL, *ceilingrover = NULL;

	diff = P_ReadUINT32(save_p);
	if (diff & MD_MORE)
		diff2 = P_ReadUINT32(save_p);
	else
		diff2 = 0;

	next = (void *)(size_t)P_ReadUINT32(save_p);

	z = P_ReadFixed(save_p); // Force this so 3dfloor problems don't arise.
	floorz = P_ReadFixed(save_p);
	ceilingz = P_ReadFixed(save_p);

	if (diff2 & MD2_FLOORROVER)
	{
		sector_t *sec = LoadSector(P_ReadUINT32(save_p));
		UINT16 id = P_ReadUINT16(save_p);
		floorrover = P_GetFFloorByID(sec, id);
	}

	if (diff2 & MD2_CEILINGROVER)
	{
		sector_t *sec = LoadSector(P_ReadUINT32(save_p));
		UINT16 id = P_ReadUINT16(save_p);
		ceilingrover = P_GetFFloorByID(sec, id);
	}

	if (diff & MD_SPAWNPOINT)
	{
		UINT16 spawnpointnum = P_ReadUINT16(save_p);

		if (mapthings[spawnpointnum].type == 1713) // NiGHTS Hoop special case
		{
			P_SpawnHoop(&mapthings[spawnpointnum]);
			return NULL;
		}

		mobj = Z_Calloc(sizeof (*mobj), PU_LEVEL, NULL);

		mobj->spawnpoint = &mapthings[spawnpointnum];
		mapthings[spawnpointnum].mobj = mobj;
	}
	else
		mobj = Z_Calloc(sizeof (*mobj), PU_LEVEL, NULL);

	// declare this as a valid mobj as soon as possible.
	mobj->thinker.function.acp1 = thinker;

	mobj->z = z;
	mobj->floorz = floorz;
	mobj->ceilingz = ceilingz;
	mobj->floorrover = floorrover;
	mobj->ceilingrover = ceilingrover;

	if (diff & MD_TYPE)
		mobj->type = P_ReadUINT32(save_p);
	else
	{
		for (i = 0; i < NUMMOBJTYPES; i++)
			if (mobj->spawnpoint && mobj->spawnpoint->type == mobjinfo[i].doomednum)
				break;
		if (i == NUMMOBJTYPES)
		{
			if (mobj->spawnpoint)
				CONS_Alert(CONS_ERROR, "Found mobj with unknown map thing type %d\n", mobj->spawnpoint->type);
			else
				CONS_Alert(CONS_ERROR, "Found mobj with unknown map thing type NULL\n");
			I_Error("Savegame corrupted");
		}
		mobj->type = i;
	}
	mobj->info = &mobjinfo[mobj->type];
	if (diff & MD_POS)
	{
		mobj->x = P_ReadFixed(save_p);
		mobj->y = P_ReadFixed(save_p);
		mobj->angle = P_ReadAngle(save_p);
		mobj->pitch = P_ReadAngle(save_p);
		mobj->roll = P_ReadAngle(save_p);
	}
	else
	{
		mobj->x = mobj->spawnpoint->x << FRACBITS;
		mobj->y = mobj->spawnpoint->y << FRACBITS;
		mobj->angle = FixedAngle(mobj->spawnpoint->angle*FRACUNIT);
		mobj->pitch = FixedAngle(mobj->spawnpoint->pitch*FRACUNIT);
		mobj->roll = FixedAngle(mobj->spawnpoint->roll*FRACUNIT);
	}
	if (diff & MD_MOM)
	{
		mobj->momx = P_ReadFixed(save_p);
		mobj->momy = P_ReadFixed(save_p);
		mobj->momz = P_ReadFixed(save_p);
		mobj->pmomz = P_ReadFixed(save_p);
	} // otherwise they're zero, and the memset took care of it

	if (diff & MD_RADIUS)
		mobj->radius = P_ReadFixed(save_p);
	else
		mobj->radius = mobj->info->radius;
	if (diff & MD_HEIGHT)
		mobj->height = P_ReadFixed(save_p);
	else
		mobj->height = mobj->info->height;
	if (diff & MD_FLAGS)
		mobj->flags = P_ReadUINT32(save_p);
	else
		mobj->flags = mobj->info->flags;
	if (diff & MD_FLAGS2)
		mobj->flags2 = P_ReadUINT32(save_p);
	if (diff & MD_HEALTH)
		mobj->health = P_ReadINT32(save_p);
	else
		mobj->health = mobj->info->spawnhealth;
	if (diff & MD_RTIME)
		mobj->reactiontime = P_ReadINT32(save_p);
	else
		mobj->reactiontime = mobj->info->reactiontime;

	if (diff & MD_STATE)
		mobj->state = &states[P_ReadUINT16(save_p)];
	else
		mobj->state = &states[mobj->info->spawnstate];
	if (diff & MD_TICS)
		mobj->tics = P_ReadINT32(save_p);
	else
		mobj->tics = mobj->state->tics;
	if (diff & MD_SPRITE) {
		mobj->sprite = P_ReadUINT16(save_p);
		if (mobj->sprite == SPR_PLAY)
			mobj->sprite2 = P_ReadUINT16(save_p);
	}
	else {
		mobj->sprite = mobj->state->sprite;
		if (mobj->sprite == SPR_PLAY)
			mobj->sprite2 = P_GetStateSprite2(mobj->state);
	}
	if (diff & MD_FRAME)
	{
		mobj->frame = P_ReadUINT32(save_p);
		mobj->anim_duration = P_ReadUINT16(save_p);
	}
	else
	{
		mobj->frame = mobj->state->frame;
		mobj->anim_duration = (UINT16)mobj->state->var2;
	}
	if (diff & MD_EFLAGS)
		mobj->eflags = P_ReadUINT16(save_p);
	if (diff & MD_PLAYER)
	{
		i = P_ReadUINT8(save_p);
		mobj->player = &players[i];
		mobj->player->mo = mobj;
	}
	if (diff & MD_MOVEDIR)
		mobj->movedir = P_ReadAngle(save_p);
	if (diff & MD_MOVECOUNT)
		mobj->movecount = P_ReadINT32(save_p);
	if (diff & MD_THRESHOLD)
		mobj->threshold = P_ReadINT32(save_p);
	if (diff & MD_LASTLOOK)
		mobj->lastlook = P_ReadINT32(save_p);
	else
		mobj->lastlook = -1;
	if (diff & MD_TARGET)
		mobj->target = (mobj_t *)(size_t)P_ReadUINT32(save_p);
	if (diff & MD_TRACER)
		mobj->tracer = (mobj_t *)(size_t)P_ReadUINT32(save_p);
	if (diff & MD_FRICTION)
		mobj->friction = P_ReadFixed(save_p);
	else
		mobj->friction = ORIG_FRICTION;
	if (diff & MD_MOVEFACTOR)
		mobj->movefactor = P_ReadFixed(save_p);
	else
		mobj->movefactor = FRACUNIT;
	if (diff & MD_FUSE)
		mobj->fuse = P_ReadINT32(save_p);
	if (diff & MD_WATERTOP)
		mobj->watertop = P_ReadFixed(save_p);
	if (diff & MD_WATERBOTTOM)
		mobj->waterbottom = P_ReadFixed(save_p);
	if (diff & MD_SCALE)
		mobj->scale = P_ReadFixed(save_p);
	else
		mobj->scale = FRACUNIT;
	if (diff & MD_DSCALE)
		mobj->destscale = P_ReadFixed(save_p);
	else
		mobj->destscale = mobj->scale;
	if (diff2 & MD2_SCALESPEED)
		mobj->scalespeed = P_ReadFixed(save_p);
	else
		mobj->scalespeed = FRACUNIT/12;
	if (diff2 & MD2_CUSVAL)
		mobj->cusval = P_ReadINT32(save_p);
	if (diff2 & MD2_CVMEM)
		mobj->cvmem = P_ReadINT32(save_p);
	if (diff2 & MD2_SKIN)
		mobj->skin = skins[P_ReadUINT8(save_p)];
	if (diff2 & MD2_COLOR)
		mobj->color = P_ReadUINT16(save_p);
	if (diff2 & MD2_EXTVAL1)
		mobj->extravalue1 = P_ReadINT32(save_p);
	if (diff2 & MD2_EXTVAL2)
		mobj->extravalue2 = P_ReadINT32(save_p);
	if (diff2 & MD2_HNEXT)
		mobj->hnext = (mobj_t *)(size_t)P_ReadUINT32(save_p);
	if (diff2 & MD2_HPREV)
		mobj->hprev = (mobj_t *)(size_t)P_ReadUINT32(save_p);
	if (diff2 & MD2_SLOPE)
		mobj->standingslope = P_SlopeById(P_ReadUINT16(save_p));
	if (diff2 & MD2_COLORIZED)
		mobj->colorized = P_ReadUINT8(save_p);
	if (diff2 & MD2_MIRRORED)
		mobj->mirrored = P_ReadUINT8(save_p);
	if (diff2 & MD2_SPRITEROLL)
		mobj->spriteroll = P_ReadAngle(save_p);
	if (diff2 & MD2_SHADOWSCALE)
		mobj->shadowscale = P_ReadFixed(save_p);
	if (diff2 & MD2_RENDERFLAGS)
		mobj->renderflags = P_ReadUINT32(save_p);
	if (diff2 & MD2_BLENDMODE)
		mobj->blendmode = P_ReadINT32(save_p);
	else
		mobj->blendmode = AST_TRANSLUCENT;
	if (diff2 & MD2_SPRITEXSCALE)
		mobj->spritexscale = P_ReadFixed(save_p);
	else
		mobj->spritexscale = FRACUNIT;
	if (diff2 & MD2_SPRITEYSCALE)
		mobj->spriteyscale = P_ReadFixed(save_p);
	else
		mobj->spriteyscale = FRACUNIT;
	if (diff2 & MD2_SPRITEXOFFSET)
		mobj->spritexoffset = P_ReadFixed(save_p);
	if (diff2 & MD2_SPRITEYOFFSET)
		mobj->spriteyoffset = P_ReadFixed(save_p);
	if (diff2 & MD2_FLOORSPRITESLOPE)
	{
		pslope_t *slope = (pslope_t *)P_CreateFloorSpriteSlope(mobj);

		slope->zdelta = P_ReadFixed(save_p);
		slope->zangle = P_ReadAngle(save_p);
		slope->xydirection = P_ReadAngle(save_p);

		slope->o.x = P_ReadFixed(save_p);
		slope->o.y = P_ReadFixed(save_p);
		slope->o.z = P_ReadFixed(save_p);

		slope->d.x = P_ReadFixed(save_p);
		slope->d.y = P_ReadFixed(save_p);

		slope->normal.x = P_ReadFixed(save_p);
		slope->normal.y = P_ReadFixed(save_p);
		slope->normal.z = P_ReadFixed(save_p);

		slope->moved = true;
	}
	if (diff2 & MD2_DRAWONLYFORPLAYER)
		mobj->drawonlyforplayer = &players[P_ReadUINT8(save_p)];
	if (diff2 & MD2_DONTDRAWFORVIEWMOBJ)
		mobj->dontdrawforviewmobj = (mobj_t *)(size_t)P_ReadUINT32(save_p);
	if (diff2 & MD2_DISPOFFSET)
		mobj->dispoffset = P_ReadINT32(save_p);
	else
		mobj->dispoffset = mobj->info->dispoffset;
	if (diff2 & MD2_TRANSLATION)
		mobj->translation = P_ReadUINT16(save_p);
	if (diff2 & MD2_ALPHA)
		mobj->alpha = P_ReadFixed(save_p);
	else
		mobj->alpha = FRACUNIT;

	if (diff & MD_REDFLAG)
	{
		redflag = mobj;
		rflagpoint = mobj->spawnpoint;
	}
	if (diff & MD_BLUEFLAG)
	{
		blueflag = mobj;
		bflagpoint = mobj->spawnpoint;
	}

	// set sprev, snext, bprev, bnext, subsector
	P_SetThingPosition(mobj);

	mobj->mobjnum = P_ReadUINT32(save_p);

	if (mobj->player)
	{
		if (mobj->eflags & MFE_VERTICALFLIP)
			mobj->player->viewz = mobj->z + mobj->height - mobj->player->viewheight;
		else
			mobj->player->viewz = mobj->player->mo->z + mobj->player->viewheight;
	}

	if (mobj->type == MT_SKYBOX && mobj->spawnpoint)
	{
		mtag_t tag = Tag_FGet(&mobj->spawnpoint->tags);
		if (tag >= 0 && tag <= 15)
		{
			if (mobj->spawnpoint->args[0])
				skyboxcenterpnts[tag] = mobj;
			else
				skyboxviewpnts[tag] = mobj;
		}
	}

	mobj->info = (mobjinfo_t *)next; // temporarily, set when leave this function

	R_AddMobjInterpolator(mobj);

	return &mobj->thinker;
}

static thinker_t* LoadNoEnemiesThinker(save_t *save_p, actionf_p1 thinker)
{
	noenemies_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	return &ht->thinker;
}

static thinker_t* LoadBounceCheeseThinker(save_t *save_p, actionf_p1 thinker)
{
	bouncecheese_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->speed = P_ReadFixed(save_p);
	ht->distance = P_ReadFixed(save_p);
	ht->floorwasheight = P_ReadFixed(save_p);
	ht->ceilingwasheight = P_ReadFixed(save_p);
	ht->low = P_ReadChar(save_p);

	if (ht->sector)
		ht->sector->ceilingdata = ht;

	return &ht->thinker;
}

static thinker_t* LoadContinuousFallThinker(save_t *save_p, actionf_p1 thinker)
{
	continuousfall_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->speed = P_ReadFixed(save_p);
	ht->direction = P_ReadINT32(save_p);
	ht->floorstartheight = P_ReadFixed(save_p);
	ht->ceilingstartheight = P_ReadFixed(save_p);
	ht->destheight = P_ReadFixed(save_p);

	if (ht->sector)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadMarioBlockThinker(save_t *save_p, actionf_p1 thinker)
{
	mariothink_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->speed = P_ReadFixed(save_p);
	ht->direction = P_ReadINT32(save_p);
	ht->floorstartheight = P_ReadFixed(save_p);
	ht->ceilingstartheight = P_ReadFixed(save_p);
	ht->tag = P_ReadINT16(save_p);

	if (ht->sector)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadMarioCheckThinker(save_t *save_p, actionf_p1 thinker)
{
	mariocheck_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	return &ht->thinker;
}

static thinker_t* LoadThwompThinker(save_t *save_p, actionf_p1 thinker)
{
	thwomp_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->crushspeed = P_ReadFixed(save_p);
	ht->retractspeed = P_ReadFixed(save_p);
	ht->direction = P_ReadINT32(save_p);
	ht->floorstartheight = P_ReadFixed(save_p);
	ht->ceilingstartheight = P_ReadFixed(save_p);
	ht->delay = P_ReadINT32(save_p);
	ht->tag = P_ReadINT16(save_p);
	ht->sound = P_ReadUINT16(save_p);

	if (ht->sector)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadFloatThinker(save_t *save_p, actionf_p1 thinker)
{
	floatthink_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->tag = P_ReadINT16(save_p);
	return &ht->thinker;
}

static thinker_t* LoadEachTimeThinker(save_t *save_p, actionf_p1 thinker)
{
	size_t i;
	eachtime_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	for (i = 0; i < MAXPLAYERS; i++)
	{
		ht->playersInArea[i] = P_ReadChar(save_p);
	}
	ht->triggerOnExit = P_ReadChar(save_p);
	return &ht->thinker;
}

static thinker_t* LoadRaiseThinker(save_t *save_p, actionf_p1 thinker)
{
	raise_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->tag = P_ReadINT16(save_p);
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->ceilingbottom = P_ReadFixed(save_p);
	ht->ceilingtop = P_ReadFixed(save_p);
	ht->basespeed = P_ReadFixed(save_p);
	ht->extraspeed = P_ReadFixed(save_p);
	ht->shaketimer = P_ReadUINT8(save_p);
	ht->flags = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static thinker_t* LoadCeilingThinker(save_t *save_p, actionf_p1 thinker)
{
	ceiling_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = P_ReadUINT8(save_p);
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->bottomheight = P_ReadFixed(save_p);
	ht->topheight = P_ReadFixed(save_p);
	ht->speed = P_ReadFixed(save_p);
	ht->delay = P_ReadFixed(save_p);
	ht->delaytimer = P_ReadFixed(save_p);
	ht->crush = P_ReadUINT8(save_p);
	ht->texture = P_ReadINT32(save_p);
	ht->direction = P_ReadINT32(save_p);
	ht->tag = P_ReadINT16(save_p);
	ht->origspeed = P_ReadFixed(save_p);
	ht->sourceline = P_ReadFixed(save_p);
	if (ht->sector)
		ht->sector->ceilingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadFloormoveThinker(save_t *save_p, actionf_p1 thinker)
{
	floormove_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = P_ReadUINT8(save_p);
	ht->crush = P_ReadUINT8(save_p);
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->direction = P_ReadINT32(save_p);
	ht->texture = P_ReadINT32(save_p);
	ht->floordestheight = P_ReadFixed(save_p);
	ht->speed = P_ReadFixed(save_p);
	ht->origspeed = P_ReadFixed(save_p);
	ht->delay = P_ReadFixed(save_p);
	ht->delaytimer = P_ReadFixed(save_p);
	ht->tag = P_ReadINT16(save_p);
	ht->sourceline = P_ReadFixed(save_p);
	if (ht->sector)
		ht->sector->floordata = ht;
	return &ht->thinker;
}

static thinker_t* LoadLightflashThinker(save_t *save_p, actionf_p1 thinker)
{
	lightflash_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->maxlight = P_ReadINT32(save_p);
	ht->minlight = P_ReadINT32(save_p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadStrobeThinker(save_t *save_p, actionf_p1 thinker)
{
	strobe_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->count = P_ReadINT32(save_p);
	ht->minlight = P_ReadINT16(save_p);
	ht->maxlight = P_ReadINT16(save_p);
	ht->darktime = P_ReadINT32(save_p);
	ht->brighttime = P_ReadINT32(save_p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadGlowThinker(save_t *save_p, actionf_p1 thinker)
{
	glow_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->minlight = P_ReadINT16(save_p);
	ht->maxlight = P_ReadINT16(save_p);
	ht->direction = P_ReadINT16(save_p);
	ht->speed = P_ReadINT16(save_p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadFireflickerThinker(save_t *save_p, actionf_p1 thinker)
{
	fireflicker_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->count = P_ReadINT32(save_p);
	ht->resetcount = P_ReadINT32(save_p);
	ht->maxlight = P_ReadINT16(save_p);
	ht->minlight = P_ReadINT16(save_p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static thinker_t* LoadElevatorThinker(save_t *save_p, actionf_p1 thinker, boolean setplanedata)
{
	elevator_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = P_ReadUINT8(save_p);
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->actionsector = LoadSector(P_ReadUINT32(save_p));
	ht->direction = P_ReadINT32(save_p);
	ht->floordestheight = P_ReadFixed(save_p);
	ht->ceilingdestheight = P_ReadFixed(save_p);
	ht->speed = P_ReadFixed(save_p);
	ht->origspeed = P_ReadFixed(save_p);
	ht->low = P_ReadFixed(save_p);
	ht->high = P_ReadFixed(save_p);
	ht->distance = P_ReadFixed(save_p);
	ht->delay = P_ReadFixed(save_p);
	ht->delaytimer = P_ReadFixed(save_p);
	ht->floorwasheight = P_ReadFixed(save_p);
	ht->ceilingwasheight = P_ReadFixed(save_p);
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));

	if (ht->sector && setplanedata)
	{
		ht->sector->ceilingdata = ht;
		ht->sector->floordata = ht;
	}

	return &ht->thinker;
}

static thinker_t* LoadCrumbleThinker(save_t *save_p, actionf_p1 thinker)
{
	crumble_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->actionsector = LoadSector(P_ReadUINT32(save_p));
	ht->player = LoadPlayer(P_ReadUINT32(save_p));
	ht->direction = P_ReadINT32(save_p);
	ht->origalpha = P_ReadINT32(save_p);
	ht->timer = P_ReadINT32(save_p);
	ht->speed = P_ReadFixed(save_p);
	ht->floorwasheight = P_ReadFixed(save_p);
	ht->ceilingwasheight = P_ReadFixed(save_p);
	ht->flags = P_ReadUINT8(save_p);

	if (ht->sector)
		ht->sector->floordata = ht;

	return &ht->thinker;
}

static thinker_t* LoadScrollThinker(save_t *save_p, actionf_p1 thinker)
{
	scroll_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->dx = P_ReadFixed(save_p);
	ht->dy = P_ReadFixed(save_p);
	ht->affectee = P_ReadINT32(save_p);
	ht->control = P_ReadINT32(save_p);
	ht->last_height = P_ReadFixed(save_p);
	ht->vdx = P_ReadFixed(save_p);
	ht->vdy = P_ReadFixed(save_p);
	ht->accel = P_ReadINT32(save_p);
	ht->exclusive = P_ReadINT32(save_p);
	ht->type = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadFrictionThinker(save_t *save_p, actionf_p1 thinker)
{
	friction_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->friction = P_ReadINT32(save_p);
	ht->movefactor = P_ReadINT32(save_p);
	ht->affectee = P_ReadINT32(save_p);
	ht->referrer = P_ReadINT32(save_p);
	ht->roverfriction = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static thinker_t* LoadPusherThinker(save_t *save_p, actionf_p1 thinker)
{
	pusher_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->type = P_ReadUINT8(save_p);
	ht->x_mag = P_ReadFixed(save_p);
	ht->y_mag = P_ReadFixed(save_p);
	ht->z_mag = P_ReadFixed(save_p);
	ht->affectee = P_ReadINT32(save_p);
	ht->roverpusher = P_ReadUINT8(save_p);
	ht->referrer = P_ReadINT32(save_p);
	ht->exclusive = P_ReadINT32(save_p);
	ht->slider = P_ReadINT32(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadLaserThinker(save_t *save_p, actionf_p1 thinker)
{
	laserthink_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->tag = P_ReadINT16(save_p);
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->nobosses = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadLightlevelThinker(save_t *save_p, actionf_p1 thinker)
{
	lightlevel_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->sourcelevel = P_ReadINT16(save_p);
	ht->destlevel = P_ReadINT16(save_p);
	ht->fixedcurlevel = P_ReadFixed(save_p);
	ht->fixedpertic = P_ReadFixed(save_p);
	ht->timer = P_ReadINT32(save_p);
	if (ht->sector)
		ht->sector->lightingdata = ht;
	return &ht->thinker;
}

static inline thinker_t* LoadExecutorThinker(save_t *save_p, actionf_p1 thinker)
{
	executor_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->line = LoadLine(P_ReadUINT32(save_p));
	ht->caller = LoadMobj(P_ReadUINT32(save_p));
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->timer = P_ReadINT32(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadDisappearThinker(save_t *save_p, actionf_p1 thinker)
{
	disappear_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->appeartime = P_ReadUINT32(save_p);
	ht->disappeartime = P_ReadUINT32(save_p);
	ht->offset = P_ReadUINT32(save_p);
	ht->timer = P_ReadUINT32(save_p);
	ht->affectee = P_ReadINT32(save_p);
	ht->sourceline = P_ReadINT32(save_p);
	ht->exists = P_ReadINT32(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadFadeThinker(save_t *save_p, actionf_p1 thinker)
{
	sector_t *ss;
	fade_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->dest_exc = GetNetColormapFromList(P_ReadUINT32(save_p));
	ht->sectornum = P_ReadUINT32(save_p);
	ht->ffloornum = P_ReadUINT32(save_p);
	ht->alpha = P_ReadINT32(save_p);
	ht->sourcevalue = P_ReadINT16(save_p);
	ht->destvalue = P_ReadINT16(save_p);
	ht->destlightlevel = P_ReadINT16(save_p);
	ht->speed = P_ReadINT16(save_p);
	ht->ticbased = (boolean)P_ReadUINT8(save_p);
	ht->timer = P_ReadINT32(save_p);
	ht->doexists = P_ReadUINT8(save_p);
	ht->dotranslucent = P_ReadUINT8(save_p);
	ht->dolighting = P_ReadUINT8(save_p);
	ht->docolormap = P_ReadUINT8(save_p);
	ht->docollision = P_ReadUINT8(save_p);
	ht->doghostfade = P_ReadUINT8(save_p);
	ht->exactalpha = P_ReadUINT8(save_p);

	ss = LoadSector(ht->sectornum);
	if (ss)
	{
		size_t j = 0; // ss->ffloors is saved as ffloor #0, ss->ffloors->next is #1, etc
		ffloor_t *rover;
		for (rover = ss->ffloors; rover; rover = rover->next)
		{
			if (j == ht->ffloornum)
			{
				ht->rover = rover;
				rover->fadingdata = ht;
				break;
			}
			j++;
		}
	}
	return &ht->thinker;
}

static inline thinker_t* LoadFadeColormapThinker(save_t *save_p, actionf_p1 thinker)
{
	fadecolormap_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->sector = LoadSector(P_ReadUINT32(save_p));
	ht->source_exc = GetNetColormapFromList(P_ReadUINT32(save_p));
	ht->dest_exc = GetNetColormapFromList(P_ReadUINT32(save_p));
	ht->ticbased = (boolean)P_ReadUINT8(save_p);
	ht->duration = P_ReadINT32(save_p);
	ht->timer = P_ReadINT32(save_p);
	if (ht->sector)
		ht->sector->fadecolormapdata = ht;
	return &ht->thinker;
}

static inline thinker_t* LoadPlaneDisplaceThinker(save_t *save_p, actionf_p1 thinker)
{
	planedisplace_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;

	ht->affectee = P_ReadINT32(save_p);
	ht->control = P_ReadINT32(save_p);
	ht->last_height = P_ReadFixed(save_p);
	ht->speed = P_ReadFixed(save_p);
	ht->type = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadDynamicLineSlopeThinker(save_t *save_p, actionf_p1 thinker)
{
	dynlineplanethink_t* ht = Z_Malloc(sizeof(*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;

	ht->type = P_ReadUINT8(save_p);
	ht->slope = LoadSlope(P_ReadUINT32(save_p));
	ht->sourceline = LoadLine(P_ReadUINT32(save_p));
	ht->extent = P_ReadFixed(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadDynamicVertexSlopeThinker(save_t *save_p, actionf_p1 thinker)
{
	size_t i;
	dynvertexplanethink_t* ht = Z_Malloc(sizeof(*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;

	ht->slope = LoadSlope(P_ReadUINT32(save_p));
	for (i = 0; i < 3; i++)
		ht->secs[i] = LoadSector(P_ReadUINT32(save_p));
	P_ReadMem(save_p, ht->vex, sizeof(ht->vex));
	P_ReadMem(save_p, ht->origsecheights, sizeof(ht->origsecheights));
	P_ReadMem(save_p, ht->origvecheights, sizeof(ht->origvecheights));
	ht->relative = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyrotatetThinker(save_t *save_p, actionf_p1 thinker)
{
	polyrotate_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->speed = P_ReadINT32(save_p);
	ht->distance = P_ReadINT32(save_p);
	ht->turnobjs = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static thinker_t* LoadPolymoveThinker(save_t *save_p, actionf_p1 thinker)
{
	polymove_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->speed = P_ReadINT32(save_p);
	ht->momx = P_ReadFixed(save_p);
	ht->momy = P_ReadFixed(save_p);
	ht->distance = P_ReadINT32(save_p);
	ht->angle = P_ReadAngle(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolywaypointThinker(save_t *save_p, actionf_p1 thinker)
{
	polywaypoint_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->speed = P_ReadINT32(save_p);
	ht->sequence = P_ReadINT32(save_p);
	ht->pointnum = P_ReadINT32(save_p);
	ht->direction = P_ReadINT32(save_p);
	ht->returnbehavior = P_ReadUINT8(save_p);
	ht->continuous = P_ReadUINT8(save_p);
	ht->stophere = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyslidedoorThinker(save_t *save_p, actionf_p1 thinker)
{
	polyslidedoor_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->delay = P_ReadINT32(save_p);
	ht->delayCount = P_ReadINT32(save_p);
	ht->initSpeed = P_ReadINT32(save_p);
	ht->speed = P_ReadINT32(save_p);
	ht->initDistance = P_ReadINT32(save_p);
	ht->distance = P_ReadINT32(save_p);
	ht->initAngle = P_ReadUINT32(save_p);
	ht->angle = P_ReadUINT32(save_p);
	ht->revAngle = P_ReadUINT32(save_p);
	ht->momx = P_ReadFixed(save_p);
	ht->momy = P_ReadFixed(save_p);
	ht->closing = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyswingdoorThinker(save_t *save_p, actionf_p1 thinker)
{
	polyswingdoor_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->delay = P_ReadINT32(save_p);
	ht->delayCount = P_ReadINT32(save_p);
	ht->initSpeed = P_ReadINT32(save_p);
	ht->speed = P_ReadINT32(save_p);
	ht->initDistance = P_ReadINT32(save_p);
	ht->distance = P_ReadINT32(save_p);
	ht->closing = P_ReadUINT8(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolydisplaceThinker(save_t *save_p, actionf_p1 thinker)
{
	polydisplace_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->controlSector = LoadSector(P_ReadUINT32(save_p));
	ht->dx = P_ReadFixed(save_p);
	ht->dy = P_ReadFixed(save_p);
	ht->oldHeights = P_ReadFixed(save_p);
	return &ht->thinker;
}

static inline thinker_t* LoadPolyrotdisplaceThinker(save_t *save_p, actionf_p1 thinker)
{
	polyrotdisplace_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->controlSector = LoadSector(P_ReadUINT32(save_p));
	ht->rotscale = P_ReadFixed(save_p);
	ht->turnobjs = P_ReadUINT8(save_p);
	ht->oldHeights = P_ReadFixed(save_p);
	return &ht->thinker;
}

static thinker_t* LoadPolyfadeThinker(save_t *save_p, actionf_p1 thinker)
{
	polyfade_t *ht = Z_Malloc(sizeof (*ht), PU_LEVSPEC, NULL);
	ht->thinker.function.acp1 = thinker;
	ht->polyObjNum = P_ReadINT32(save_p);
	ht->sourcevalue = P_ReadINT32(save_p);
	ht->destvalue = P_ReadINT32(save_p);
	ht->docollision = (boolean)P_ReadUINT8(save_p);
	ht->doghostfade = (boolean)P_ReadUINT8(save_p);
	ht->ticbased = (boolean)P_ReadUINT8(save_p);
	ht->duration = P_ReadINT32(save_p);
	ht->timer = P_ReadINT32(save_p);
	return &ht->thinker;
}

static void P_NetUnArchiveThinkers(save_t *save_p)
{
	thinker_t *currentthinker;
	thinker_t *next;
	UINT8 tclass;
	UINT8 restoreNum = false;
	UINT32 i;
	UINT32 numloaded = 0;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_THINKERS)
		I_Error("Bad $$$.sav at archive block Thinkers");

	// remove all the current thinkers
	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		currentthinker = thlist[i].next;
		for (currentthinker = thlist[i].next; currentthinker != &thlist[i]; currentthinker = next)
		{
			next = currentthinker->next;

			if (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker || currentthinker->function.acp1 == (actionf_p1)P_NullPrecipThinker)
				P_RemoveSavegameMobj((mobj_t *)currentthinker); // item isn't saved, don't remove it
			else
			{
				(next->prev = currentthinker->prev)->next = next;
				R_DestroyLevelInterpolators(currentthinker);
				Z_Free(currentthinker);
			}
		}
	}

	// we don't want the removed mobjs to come back
	iquetail = iquehead = 0;
	P_InitThinkers();

	// clear sector thinker pointers so they don't point to non-existant thinkers for all of eternity
	for (i = 0; i < numsectors; i++)
	{
		sectors[i].floordata = sectors[i].ceilingdata = sectors[i].lightingdata = sectors[i].fadecolormapdata = NULL;
	}

	// read in saved thinkers
	for (i = 0; i < NUM_THINKERLISTS; i++)
	{
		for (;;)
		{
			thinker_t* th = NULL;
			tclass = P_ReadUINT8(save_p);

			if (tclass == tc_end)
				break; // leave the saved thinker reading loop
			numloaded++;

			switch (tclass)
			{
				case tc_mobj:
					th = LoadMobjThinker(save_p, (actionf_p1)P_MobjThinker);
					break;

				case tc_ceiling:
					th = LoadCeilingThinker(save_p, (actionf_p1)T_MoveCeiling);
					break;

				case tc_crushceiling:
					th = LoadCeilingThinker(save_p, (actionf_p1)T_CrushCeiling);
					break;

				case tc_floor:
					th = LoadFloormoveThinker(save_p, (actionf_p1)T_MoveFloor);
					break;

				case tc_flash:
					th = LoadLightflashThinker(save_p, (actionf_p1)T_LightningFlash);
					break;

				case tc_strobe:
					th = LoadStrobeThinker(save_p, (actionf_p1)T_StrobeFlash);
					break;

				case tc_glow:
					th = LoadGlowThinker(save_p, (actionf_p1)T_Glow);
					break;

				case tc_fireflicker:
					th = LoadFireflickerThinker(save_p, (actionf_p1)T_FireFlicker);
					break;

				case tc_elevator:
					th = LoadElevatorThinker(save_p, (actionf_p1)T_MoveElevator, true);
					break;

				case tc_continuousfalling:
					th = LoadContinuousFallThinker(save_p, (actionf_p1)T_ContinuousFalling);
					break;

				case tc_thwomp:
					th = LoadThwompThinker(save_p, (actionf_p1)T_ThwompSector);
					break;

				case tc_noenemies:
					th = LoadNoEnemiesThinker(save_p, (actionf_p1)T_NoEnemiesSector);
					break;

				case tc_eachtime:
					th = LoadEachTimeThinker(save_p, (actionf_p1)T_EachTimeThinker);
					break;

				case tc_raisesector:
					th = LoadRaiseThinker(save_p, (actionf_p1)T_RaiseSector);
					break;

				case tc_camerascanner:
					th = LoadElevatorThinker(save_p, (actionf_p1)T_CameraScanner, false);
					break;

				case tc_bouncecheese:
					th = LoadBounceCheeseThinker(save_p, (actionf_p1)T_BounceCheese);
					break;

				case tc_startcrumble:
					th = LoadCrumbleThinker(save_p, (actionf_p1)T_StartCrumble);
					break;

				case tc_marioblock:
					th = LoadMarioBlockThinker(save_p, (actionf_p1)T_MarioBlock);
					break;

				case tc_marioblockchecker:
					th = LoadMarioCheckThinker(save_p, (actionf_p1)T_MarioBlockChecker);
					break;

				case tc_floatsector:
					th = LoadFloatThinker(save_p, (actionf_p1)T_FloatSector);
					break;

				case tc_laserflash:
					th = LoadLaserThinker(save_p, (actionf_p1)T_LaserFlash);
					break;

				case tc_lightfade:
					th = LoadLightlevelThinker(save_p, (actionf_p1)T_LightFade);
					break;

				case tc_executor:
					th = LoadExecutorThinker(save_p, (actionf_p1)T_ExecutorDelay);
					restoreNum = true;
					break;

				case tc_disappear:
					th = LoadDisappearThinker(save_p, (actionf_p1)T_Disappear);
					break;

				case tc_fade:
					th = LoadFadeThinker(save_p, (actionf_p1)T_Fade);
					break;

				case tc_fadecolormap:
					th = LoadFadeColormapThinker(save_p, (actionf_p1)T_FadeColormap);
					break;

				case tc_planedisplace:
					th = LoadPlaneDisplaceThinker(save_p, (actionf_p1)T_PlaneDisplace);
					break;
				case tc_polyrotate:
					th = LoadPolyrotatetThinker(save_p, (actionf_p1)T_PolyObjRotate);
					break;

				case tc_polymove:
					th = LoadPolymoveThinker(save_p, (actionf_p1)T_PolyObjMove);
					break;

				case tc_polywaypoint:
					th = LoadPolywaypointThinker(save_p, (actionf_p1)T_PolyObjWaypoint);
					break;

				case tc_polyslidedoor:
					th = LoadPolyslidedoorThinker(save_p, (actionf_p1)T_PolyDoorSlide);
					break;

				case tc_polyswingdoor:
					th = LoadPolyswingdoorThinker(save_p, (actionf_p1)T_PolyDoorSwing);
					break;

				case tc_polyflag:
					th = LoadPolymoveThinker(save_p, (actionf_p1)T_PolyObjFlag);
					break;

				case tc_polydisplace:
					th = LoadPolydisplaceThinker(save_p, (actionf_p1)T_PolyObjDisplace);
					break;

				case tc_polyrotdisplace:
					th = LoadPolyrotdisplaceThinker(save_p, (actionf_p1)T_PolyObjRotDisplace);
					break;

				case tc_polyfade:
					th = LoadPolyfadeThinker(save_p, (actionf_p1)T_PolyObjFade);
					break;

				case tc_dynslopeline:
					th = LoadDynamicLineSlopeThinker(save_p, (actionf_p1)T_DynamicSlopeLine);
					break;

				case tc_dynslopevert:
					th = LoadDynamicVertexSlopeThinker(save_p, (actionf_p1)T_DynamicSlopeVert);
					break;

				case tc_scroll:
					th = LoadScrollThinker(save_p, (actionf_p1)T_Scroll);
					break;

				case tc_friction:
					th = LoadFrictionThinker(save_p, (actionf_p1)T_Friction);
					break;

				case tc_pusher:
					th = LoadPusherThinker(save_p, (actionf_p1)T_Pusher);
					break;

				default:
					I_Error("P_UnarchiveSpecials: Unknown tclass %d in savegame", tclass);
			}
			if (th)
				P_AddThinker(i, th);
		}

		CONS_Debug(DBG_NETPLAY, "%u thinkers loaded in list %d\n", numloaded, i);
	}

	// Set each skyboxmo to the first skybox (or NULL)
	skyboxmo[0] = skyboxviewpnts[0];
	skyboxmo[1] = skyboxcenterpnts[0];

	if (restoreNum)
	{
		executor_t *delay = NULL;
		UINT32 mobjnum;
		for (currentthinker = thlist[THINK_MAIN].next; currentthinker != &thlist[THINK_MAIN]; currentthinker = currentthinker->next)
		{
			if (currentthinker->function.acp1 != (actionf_p1)T_ExecutorDelay)
				continue;
			delay = (void *)currentthinker;
			if (!(mobjnum = (UINT32)(size_t)delay->caller))
				continue;
			delay->caller = P_FindNewPosition(mobjnum);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// haleyjd 03/26/06: PolyObject saving code
//
#define PD_FLAGS  0x01
#define PD_TRANS   0x02

static inline void P_ArchivePolyObj(save_t *save_p, polyobj_t *po)
{
	UINT8 diff = 0;
	P_WriteINT32(save_p, po->id);
	P_WriteAngle(save_p, po->angle);

	P_WriteFixed(save_p, po->spawnSpot.x);
	P_WriteFixed(save_p, po->spawnSpot.y);

	if (po->flags != po->spawnflags)
		diff |= PD_FLAGS;
	if (po->translucency != po->spawntrans)
		diff |= PD_TRANS;

	P_WriteUINT8(save_p, diff);

	if (diff & PD_FLAGS)
		P_WriteINT32(save_p, po->flags);
	if (diff & PD_TRANS)
		P_WriteINT32(save_p, po->translucency);
}

static inline void P_UnArchivePolyObj(save_t *save_p, polyobj_t *po)
{
	INT32 id;
	UINT32 angle;
	fixed_t x, y;
	UINT8 diff;

	// nullify all polyobject thinker pointers;
	// the thinkers themselves will fight over who gets the field
	// when they first start to run.
	po->thinker = NULL;

	id = P_ReadINT32(save_p);

	angle = P_ReadAngle(save_p);

	x = P_ReadFixed(save_p);
	y = P_ReadFixed(save_p);

	diff = P_ReadUINT8(save_p);

	if (diff & PD_FLAGS)
		po->flags = P_ReadINT32(save_p);
	if (diff & PD_TRANS)
		po->translucency = P_ReadINT32(save_p);

	// if the object is bad or isn't in the id hash, we can do nothing more
	// with it, so return now
	if (po->isBad || po != Polyobj_GetForNum(id))
		return;

	// rotate and translate polyobject
	Polyobj_MoveOnLoad(po, angle, x, y);
}

static inline void P_ArchivePolyObjects(save_t *save_p)
{
	INT32 i;

	P_WriteUINT32(save_p, ARCHIVEBLOCK_POBJS);

	// save number of polyobjects
	P_WriteINT32(save_p, numPolyObjects);

	for (i = 0; i < numPolyObjects; ++i)
		P_ArchivePolyObj(save_p, &PolyObjects[i]);
}

static inline void P_UnArchivePolyObjects(save_t *save_p)
{
	INT32 i, numSavedPolys;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_POBJS)
		I_Error("Bad $$$.sav at archive block Pobjs");

	numSavedPolys = P_ReadINT32(save_p);

	if (numSavedPolys != numPolyObjects)
		I_Error("P_UnArchivePolyObjects: polyobj count inconsistency\n");

	for (i = 0; i < numSavedPolys; ++i)
		P_UnArchivePolyObj(save_p, &PolyObjects[i]);
}

static inline void P_FinishMobjs(void)
{
	thinker_t *currentthinker;
	mobj_t *mobj;

	// put info field there real value
	for (currentthinker = thlist[THINK_MOBJ].next; currentthinker != &thlist[THINK_MOBJ];
		currentthinker = currentthinker->next)
	{
		if (currentthinker->removing)
			continue;

		mobj = (mobj_t *)currentthinker;
		mobj->info = &mobjinfo[mobj->type];
	}
}

static void P_RelinkPointers(void)
{
	thinker_t *currentthinker;
	mobj_t *mobj;
	UINT32 temp;

	// use info field (value = oldposition) to relink mobjs
	for (currentthinker = thlist[THINK_MOBJ].next; currentthinker != &thlist[THINK_MOBJ];
		currentthinker = currentthinker->next)
	{
		if (currentthinker->removing)
			continue;

		mobj = (mobj_t *)currentthinker;

		if (mobj->type == MT_HOOP || mobj->type == MT_HOOPCOLLIDE || mobj->type == MT_HOOPCENTER)
			continue;

		if (mobj->dontdrawforviewmobj)
		{
			temp = (UINT32)(size_t)mobj->dontdrawforviewmobj;
			mobj->dontdrawforviewmobj = NULL;
			if (!P_SetTarget(&mobj->dontdrawforviewmobj, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "dontdrawforviewmobj not found on %d\n", mobj->type);
		}
		if (mobj->tracer)
		{
			temp = (UINT32)(size_t)mobj->tracer;
			mobj->tracer = NULL;
			if (!P_SetTarget(&mobj->tracer, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "tracer not found on %d\n", mobj->type);
		}
		if (mobj->target)
		{
			temp = (UINT32)(size_t)mobj->target;
			mobj->target = NULL;
			if (!P_SetTarget(&mobj->target, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "target not found on %d\n", mobj->type);
		}
		if (mobj->hnext)
		{
			temp = (UINT32)(size_t)mobj->hnext;
			mobj->hnext = NULL;
			if (!(mobj->hnext = P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "hnext not found on %d\n", mobj->type);
		}
		if (mobj->hprev)
		{
			temp = (UINT32)(size_t)mobj->hprev;
			mobj->hprev = NULL;
			if (!(mobj->hprev = P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "hprev not found on %d\n", mobj->type);
		}
		if (mobj->player && mobj->player->capsule)
		{
			temp = (UINT32)(size_t)mobj->player->capsule;
			mobj->player->capsule = NULL;
			if (!P_SetTarget(&mobj->player->capsule, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "capsule not found on %d\n", mobj->type);
		}
		if (mobj->player && mobj->player->axis1)
		{
			temp = (UINT32)(size_t)mobj->player->axis1;
			mobj->player->axis1 = NULL;
			if (!P_SetTarget(&mobj->player->axis1, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "axis1 not found on %d\n", mobj->type);
		}
		if (mobj->player && mobj->player->axis2)
		{
			temp = (UINT32)(size_t)mobj->player->axis2;
			mobj->player->axis2 = NULL;
			if (!P_SetTarget(&mobj->player->axis2, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "axis2 not found on %d\n", mobj->type);
		}
		if (mobj->player && mobj->player->awayviewmobj)
		{
			temp = (UINT32)(size_t)mobj->player->awayviewmobj;
			mobj->player->awayviewmobj = NULL;
			if (!P_SetTarget(&mobj->player->awayviewmobj, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "awayviewmobj not found on %d\n", mobj->type);
		}
		if (mobj->player && mobj->player->followmobj)
		{
			temp = (UINT32)(size_t)mobj->player->followmobj;
			mobj->player->followmobj = NULL;
			if (!P_SetTarget(&mobj->player->followmobj, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "followmobj not found on %d\n", mobj->type);
		}
		if (mobj->player && mobj->player->drone)
		{
			temp = (UINT32)(size_t)mobj->player->drone;
			mobj->player->drone = NULL;
			if (!P_SetTarget(&mobj->player->drone, P_FindNewPosition(temp)))
				CONS_Debug(DBG_GAMELOGIC, "drone not found on %d\n", mobj->type);
		}
	}
}

static inline void P_NetArchiveSpecials(save_t *save_p)
{
	size_t i, z;

	P_WriteUINT32(save_p, ARCHIVEBLOCK_SPECIALS);

	// itemrespawn queue for deathmatch
	i = iquetail;
	while (iquehead != i)
	{
		for (z = 0; z < nummapthings; z++)
		{
			if (&mapthings[z] == itemrespawnque[i])
			{
				P_WriteUINT32(save_p, z);
				break;
			}
		}
		P_WriteUINT32(save_p, itemrespawntime[i]);
		i = (i + 1) & (ITEMQUESIZE-1);
	}

	// end delimiter
	P_WriteUINT32(save_p, 0xffffffff);

	// Sky number
	P_WriteINT32(save_p, globallevelskynum);

	// Current global weather type
	P_WriteUINT8(save_p, globalweather);

	if (metalplayback) // Is metal sonic running?
	{
		UINT8 *p = &save_p->buf[save_p->pos+1];
		P_WriteUINT8(save_p, 0x01);
		G_SaveMetal(&p);
	}
	else
		P_WriteUINT8(save_p, 0x00);
}

static void P_NetUnArchiveSpecials(save_t *save_p)
{
	size_t i;
	INT32 j;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_SPECIALS)
		I_Error("Bad $$$.sav at archive block Specials");

	// BP: added save itemrespawn queue for deathmatch
	iquetail = iquehead = 0;
	while ((i = P_ReadUINT32(save_p)) != 0xffffffff)
	{
		itemrespawnque[iquehead] = &mapthings[i];
		itemrespawntime[iquehead++] = P_ReadINT32(save_p);
	}

	j = P_ReadINT32(save_p);
	if (j != globallevelskynum)
		P_SetupLevelSky(j, true);

	globalweather = P_ReadUINT8(save_p);

	if (globalweather)
	{
		if (curWeather == globalweather)
			curWeather = PRECIP_NONE;

		P_SwitchWeather(globalweather);
	}
	else // PRECIP_NONE
	{
		if (curWeather != PRECIP_NONE)
			P_SwitchWeather(globalweather);
	}

	if (P_ReadUINT8(save_p) == 0x01) // metal sonic
	{
		UINT8 *p = &save_p->buf[save_p->pos];
		G_LoadMetal(&p);
	}
}

// =======================================================================
//          Misc
// =======================================================================
static inline void P_ArchiveMisc(save_t *save_p, INT16 mapnum)
{
	//lastmapsaved = mapnum;
	lastmaploaded = mapnum;

	if (gamecomplete)
		mapnum |= 8192;

	P_WriteINT16(save_p, mapnum);
	P_WriteUINT16(save_p, emeralds+357);
	P_WriteStringN(save_p, timeattackfolder, sizeof(timeattackfolder));
}

static inline void P_UnArchiveSPGame(save_t *save_p, INT16 mapoverride)
{
	char testname[sizeof(timeattackfolder)];

	gamemap = P_ReadINT16(save_p);

	if (mapoverride != 0)
	{
		gamemap = mapoverride;
		gamecomplete = 1;
	}
	else
		gamecomplete = 0;

	// gamemap changed; we assume that its map header is always valid,
	// so make it so
	if(!mapheaderinfo[gamemap-1])
		P_AllocMapHeader(gamemap-1);

	//lastmapsaved = gamemap;
	lastmaploaded = gamemap;

	tokenlist = 0;
	token = 0;

	savedata.emeralds = P_ReadUINT16(save_p)-357;

	P_ReadStringN(save_p, testname, sizeof(testname));

	if (strcmp(testname, timeattackfolder))
	{
		if (modifiedgame)
			I_Error("Save game not for this modification.");
		else
			I_Error("This save file is for a particular mod, it cannot be used with the regular game.");
	}

	memset(playeringame, 0, sizeof(*playeringame));
	playeringame[consoleplayer] = true;
}

static void P_NetArchiveMisc(save_t *save_p, boolean resending)
{
	INT32 i;

	P_WriteUINT32(save_p, ARCHIVEBLOCK_MISC);

	if (resending)
		P_WriteUINT32(save_p, gametic);
	P_WriteINT16(save_p, gamemap);

	if (gamestate != GS_LEVEL)
		P_WriteINT16(save_p, GS_WAITINGPLAYERS); // nice hack to put people back into waitingplayers
	else
		P_WriteINT16(save_p, gamestate);
	P_WriteINT16(save_p, gametype);

	{
		UINT32 pig = 0;
		for (i = 0; i < MAXPLAYERS; i++)
			pig |= (playeringame[i] != 0)<<i;
		P_WriteUINT32(save_p, pig);
	}

	P_WriteUINT32(save_p, P_GetRandSeed());

	P_WriteUINT32(save_p, tokenlist);

	P_WriteUINT32(save_p, leveltime);
	P_WriteUINT32(save_p, ssspheres);
	P_WriteINT16(save_p, lastmap);
	P_WriteUINT16(save_p, bossdisabled);

	P_WriteUINT16(save_p, emeralds);
	{
		UINT8 globools = 0;
		if (stagefailed)
			globools |= 1;
		if (stoppedclock)
			globools |= (1<<1);
		P_WriteUINT8(save_p, globools);
	}

	P_WriteUINT32(save_p, token);
	P_WriteINT32(save_p, sstimer);
	P_WriteUINT32(save_p, bluescore);
	P_WriteUINT32(save_p, redscore);

	P_WriteUINT16(save_p, skincolor_redteam);
	P_WriteUINT16(save_p, skincolor_blueteam);
	P_WriteUINT16(save_p, skincolor_redring);
	P_WriteUINT16(save_p, skincolor_bluering);

	P_WriteINT32(save_p, modulothing);

	P_WriteINT16(save_p, autobalance);
	P_WriteINT16(save_p, teamscramble);

	for (i = 0; i < MAXPLAYERS; i++)
		P_WriteINT16(save_p, scrambleplayers[i]);

	for (i = 0; i < MAXPLAYERS; i++)
		P_WriteINT16(save_p, scrambleteams[i]);

	P_WriteINT16(save_p, scrambletotal);
	P_WriteINT16(save_p, scramblecount);

	P_WriteUINT32(save_p, countdown);
	P_WriteUINT32(save_p, countdown2);

	P_WriteFixed(save_p, gravity);

	P_WriteUINT32(save_p, countdowntimer);
	P_WriteUINT8(save_p, countdowntimeup);

	P_WriteUINT32(save_p, hidetime);

	// Is it paused?
	if (paused)
		P_WriteUINT8(save_p, 0x2f);
	else
		P_WriteUINT8(save_p, 0x2e);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		P_WriteUINT8(save_p, spam_tokens[i]);
		P_WriteUINT32(save_p, spam_tics[i]);
	}
}

static inline boolean P_NetUnArchiveMisc(save_t *save_p, boolean reloading)
{
	INT32 i;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_MISC)
		I_Error("Bad $$$.sav at archive block Misc");

	if (reloading)
		gametic = P_ReadUINT32(save_p);

	gamemap = P_ReadINT16(save_p);

	// gamemap changed; we assume that its map header is always valid,
	// so make it so
	if(!mapheaderinfo[gamemap-1])
		P_AllocMapHeader(gamemap-1);

	// tell the sound code to reset the music since we're skipping what
	// normally sets this flag
	mapmusflags |= MUSIC_RELOADRESET;

	G_SetGamestate(P_ReadINT16(save_p));

	gametype = P_ReadINT16(save_p);

	{
		UINT32 pig = P_ReadUINT32(save_p);
		for (i = 0; i < MAXPLAYERS; i++)
		{
			playeringame[i] = (pig & (1<<i)) != 0;
			// playerstate is set in unarchiveplayers
		}
	}

	P_SetRandSeed(P_ReadUINT32(save_p));

	tokenlist = P_ReadUINT32(save_p);

	if (!P_LoadLevel(true, reloading))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Can't load the level!\n"));
		return false;
	}

	// get the time
	leveltime = P_ReadUINT32(save_p);
	ssspheres = P_ReadUINT32(save_p);
	lastmap = P_ReadINT16(save_p);
	bossdisabled = P_ReadUINT16(save_p);

	emeralds = P_ReadUINT16(save_p);
	{
		UINT8 globools = P_ReadUINT8(save_p);
		stagefailed = !!(globools & 1);
		stoppedclock = !!(globools & (1<<1));
	}

	token = P_ReadUINT32(save_p);
	sstimer = P_ReadINT32(save_p);
	bluescore = P_ReadUINT32(save_p);
	redscore = P_ReadUINT32(save_p);

	skincolor_redteam = P_ReadUINT16(save_p);
	skincolor_blueteam = P_ReadUINT16(save_p);
	skincolor_redring = P_ReadUINT16(save_p);
	skincolor_bluering = P_ReadUINT16(save_p);

	modulothing = P_ReadINT32(save_p);

	autobalance = P_ReadINT16(save_p);
	teamscramble = P_ReadINT16(save_p);

	for (i = 0; i < MAXPLAYERS; i++)
		scrambleplayers[i] = P_ReadINT16(save_p);

	for (i = 0; i < MAXPLAYERS; i++)
		scrambleteams[i] = P_ReadINT16(save_p);

	scrambletotal = P_ReadINT16(save_p);
	scramblecount = P_ReadINT16(save_p);

	countdown = P_ReadUINT32(save_p);
	countdown2 = P_ReadUINT32(save_p);

	gravity = P_ReadFixed(save_p);

	countdowntimer = (tic_t)P_ReadUINT32(save_p);
	countdowntimeup = (boolean)P_ReadUINT8(save_p);

	hidetime = P_ReadUINT32(save_p);

	// Is it paused?
	if (P_ReadUINT8(save_p) == 0x2f)
		paused = true;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		spam_tokens[i] = P_ReadUINT8(save_p);
		spam_tics[i] = P_ReadUINT32(save_p);
	}

	return true;
}

static inline void P_NetArchiveEmblems(save_t *save_p)
{
	gamedata_t *data = serverGamedata;
	INT32 i, j;
	UINT8 btemp;
	INT32 curmare;

	P_WriteUINT32(save_p, ARCHIVEBLOCK_EMBLEMS);

	// These should be synchronized before savegame loading by the wad files being the same anyway,
	// but just in case, for now, we'll leave them here for testing. It would be very bad if they mismatch.
	P_WriteUINT8(save_p, (UINT8)savemoddata);
	P_WriteINT32(save_p, numemblems);
	P_WriteINT32(save_p, numextraemblems);

	// The rest of this is lifted straight from G_SaveGameData in g_game.c
	// TODO: Optimize this to only send information about emblems, unlocks, etc. which actually exist
	//       There is no need to go all the way up to MAXEMBLEMS when wads are guaranteed to be the same.

	P_WriteUINT32(save_p, data->totalplaytime);

	// TODO put another cipher on these things? meh, I don't care...
	for (i = 0; i < NUMMAPS; i++)
		P_WriteUINT8(save_p, (data->mapvisited[i] & MV_MAX));

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			btemp |= (data->collected[j+i] << j);
		P_WriteUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXEXTRAEMBLEMS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXEXTRAEMBLEMS; ++j)
			btemp |= (data->extraCollected[j+i] << j);
		P_WriteUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			btemp |= (data->unlocked[j+i] << j);
		P_WriteUINT8(save_p, btemp);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		btemp = 0;
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			btemp |= (data->achieved[j+i] << j);
		P_WriteUINT8(save_p, btemp);
		i += j;
	}

	P_WriteUINT32(save_p, data->timesBeaten);
	P_WriteUINT32(save_p, data->timesBeatenWithEmeralds);
	P_WriteUINT32(save_p, data->timesBeatenUltimate);

	// Main records
	for (i = 0; i < NUMMAPS; i++)
	{
		if (data->mainrecords[i])
		{
			P_WriteUINT32(save_p, data->mainrecords[i]->score);
			P_WriteUINT32(save_p, data->mainrecords[i]->time);
			P_WriteUINT16(save_p, data->mainrecords[i]->rings);
		}
		else
		{
			P_WriteUINT32(save_p, 0);
			P_WriteUINT32(save_p, 0);
			P_WriteUINT16(save_p, 0);
		}
	}

	// NiGHTS records
	for (i = 0; i < NUMMAPS; i++)
	{
		if (!data->nightsrecords[i] || !data->nightsrecords[i]->nummares)
		{
			P_WriteUINT8(save_p, 0);
			continue;
		}

		P_WriteUINT8(save_p, data->nightsrecords[i]->nummares);

		for (curmare = 0; curmare < (data->nightsrecords[i]->nummares + 1); ++curmare)
		{
			P_WriteUINT32(save_p, data->nightsrecords[i]->score[curmare]);
			P_WriteUINT8(save_p, data->nightsrecords[i]->grade[curmare]);
			P_WriteUINT32(save_p, data->nightsrecords[i]->time[curmare]);
		}
	}

	// Mid-map stuff
	P_WriteUINT32(save_p, unlocktriggers);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!ntemprecords[i].nummares)
		{
			P_WriteUINT8(save_p, 0);
			continue;
		}

		P_WriteUINT8(save_p, ntemprecords[i].nummares);

		for (curmare = 0; curmare < (ntemprecords[i].nummares + 1); ++curmare)
		{
			P_WriteUINT32(save_p, ntemprecords[i].score[curmare]);
			P_WriteUINT8(save_p, ntemprecords[i].grade[curmare]);
			P_WriteUINT32(save_p, ntemprecords[i].time[curmare]);
		}
	}
}

static inline void P_NetUnArchiveEmblems(save_t *save_p)
{
	gamedata_t *data = serverGamedata;
	INT32 i, j;
	UINT8 rtemp;
	UINT32 recscore;
	tic_t rectime;
	UINT16 recrings;
	UINT8 recmares;
	INT32 curmare;

	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_EMBLEMS)
		I_Error("Bad $$$.sav at archive block Emblems");

	savemoddata = (boolean)P_ReadUINT8(save_p); // this one is actually necessary because savemoddata stays false otherwise for some reason.

	if (numemblems != P_ReadINT32(save_p))
		I_Error("Bad $$$.sav dearchiving Emblems (numemblems mismatch)");
	if (numextraemblems != P_ReadINT32(save_p))
		I_Error("Bad $$$.sav dearchiving Emblems (numextraemblems mismatch)");

	// This shouldn't happen, but if something really fucked up happens and you transfer
	// the SERVER player's gamedata over your own CLIENT gamedata,
	// then this prevents it from being saved over yours.
	data->loaded = false;

	M_ClearSecrets(data);
	G_ClearRecords(data);

	// The rest of this is lifted straight from G_LoadGameData in g_game.c
	// TODO: Optimize this to only read information about emblems, unlocks, etc. which actually exist
	//       There is no need to go all the way up to MAXEMBLEMS when wads are guaranteed to be the same.

	data->totalplaytime = P_ReadUINT32(save_p);

	// TODO put another cipher on these things? meh, I don't care...
	for (i = 0; i < NUMMAPS; i++)
		if ((data->mapvisited[i] = P_ReadUINT8(save_p)) > MV_MAX)
			I_Error("Bad $$$.sav dearchiving Emblems (invalid visit flags)");

	// To save space, use one bit per collected/achieved/unlocked flag
	for (i = 0; i < MAXEMBLEMS;)
	{
		rtemp = P_ReadUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXEMBLEMS; ++j)
			data->collected[j+i] = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXEXTRAEMBLEMS;)
	{
		rtemp = P_ReadUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXEXTRAEMBLEMS; ++j)
			data->extraCollected[j+i] = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXUNLOCKABLES;)
	{
		rtemp = P_ReadUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXUNLOCKABLES; ++j)
			data->unlocked[j+i] = ((rtemp >> j) & 1);
		i += j;
	}
	for (i = 0; i < MAXCONDITIONSETS;)
	{
		rtemp = P_ReadUINT8(save_p);
		for (j = 0; j < 8 && j+i < MAXCONDITIONSETS; ++j)
			data->achieved[j+i] = ((rtemp >> j) & 1);
		i += j;
	}

	data->timesBeaten = P_ReadUINT32(save_p);
	data->timesBeatenWithEmeralds = P_ReadUINT32(save_p);
	data->timesBeatenUltimate = P_ReadUINT32(save_p);

	// Main records
	for (i = 0; i < NUMMAPS; ++i)
	{
		recscore = P_ReadUINT32(save_p);
		rectime  = (tic_t)P_ReadUINT32(save_p);
		recrings = P_ReadUINT16(save_p);

		if (recrings > 10000 || recscore > MAXSCORE)
			I_Error("Bad $$$.sav dearchiving Emblems (invalid score)");

		if (recscore || rectime || recrings)
		{
			G_AllocMainRecordData((INT16)i, data);
			data->mainrecords[i]->score = recscore;
			data->mainrecords[i]->time = rectime;
			data->mainrecords[i]->rings = recrings;
		}
	}

	// Nights records
	for (i = 0; i < NUMMAPS; ++i)
	{
		if ((recmares = P_ReadUINT8(save_p)) == 0)
			continue;

		G_AllocNightsRecordData((INT16)i, data);

		for (curmare = 0; curmare < (recmares+1); ++curmare)
		{
			data->nightsrecords[i]->score[curmare] = P_ReadUINT32(save_p);
			data->nightsrecords[i]->grade[curmare] = P_ReadUINT8(save_p);
			data->nightsrecords[i]->time[curmare] = (tic_t)P_ReadUINT32(save_p);

			if (data->nightsrecords[i]->grade[curmare] > GRADE_S)
			{
				I_Error("Bad $$$.sav dearchiving Emblems (invalid grade)");
			}
		}

		data->nightsrecords[i]->nummares = recmares;
	}

	// Mid-map stuff
	unlocktriggers = P_ReadUINT32(save_p);

	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if ((recmares = P_ReadUINT8(save_p)) == 0)
			continue;

		for (curmare = 0; curmare < (recmares+1); ++curmare)
		{
			ntemprecords[i].score[curmare] = P_ReadUINT32(save_p);
			ntemprecords[i].grade[curmare] = P_ReadUINT8(save_p);
			ntemprecords[i].time[curmare] = (tic_t)P_ReadUINT32(save_p);

			if (ntemprecords[i].grade[curmare] > GRADE_S)
			{
				I_Error("Bad $$$.sav dearchiving Emblems (invalid temp grade)");
			}
		}

		ntemprecords[i].nummares = recmares;
	}
}

static void P_NetArchiveSectorPortals(save_t *save_p)
{
	P_WriteUINT32(save_p, ARCHIVEBLOCK_SECPORTALS);

	P_WriteUINT32(save_p, secportalcount);

	for (size_t i = 0; i < secportalcount; i++)
	{
		UINT8 type = secportals[i].type;

		P_WriteUINT8(save_p, type);
		P_WriteUINT8(save_p, secportals[i].ceiling ? 1 : 0);
		P_WriteUINT32(save_p, SaveSector(secportals[i].target));

		switch (type)
		{
		case SECPORTAL_LINE:
			P_WriteUINT32(save_p, SaveLine(secportals[i].line.start));
			P_WriteUINT32(save_p, SaveLine(secportals[i].line.dest));
			break;
		case SECPORTAL_PLANE:
		case SECPORTAL_HORIZON:
		case SECPORTAL_FLOOR:
		case SECPORTAL_CEILING:
			P_WriteUINT32(save_p, SaveSector(secportals[i].sector));
			break;
		case SECPORTAL_OBJECT:
			if (!P_MobjWasRemoved(secportals[i].mobj))
				P_WriteUINT32(save_p, SaveMobjnum(secportals[i].mobj));
			else
				P_WriteUINT32(save_p, 0);
			break;
		default:
			break;
		}
	}
}

static void P_NetUnArchiveSectorPortals(save_t *save_p)
{
	if (P_ReadUINT32(save_p) != ARCHIVEBLOCK_SECPORTALS)
		I_Error("Bad $$$.sav at archive block Secportals");

	Z_Free(secportals);
	P_InitSectorPortals();

	UINT32 count = P_ReadUINT32(save_p);

	for (UINT32 i = 0; i < count; i++)
	{
		UINT32 id = P_NewSectorPortal();

		sectorportal_t *secportal = &secportals[id];

		secportal->type = P_ReadUINT8(save_p);
		secportal->ceiling = (P_ReadUINT8(save_p) != 0) ? true : false;
		secportal->target = LoadSector(P_ReadUINT32(save_p));

		switch (secportal->type)
		{
		case SECPORTAL_LINE:
			secportal->line.start = LoadLine(P_ReadUINT32(save_p));
			secportal->line.dest = LoadLine(P_ReadUINT32(save_p));
			break;
		case SECPORTAL_PLANE:
		case SECPORTAL_HORIZON:
		case SECPORTAL_FLOOR:
		case SECPORTAL_CEILING:
			secportal->sector = LoadSector(P_ReadUINT32(save_p));
			break;
		case SECPORTAL_OBJECT:
			id = P_ReadUINT32(save_p);
			secportal->mobj = (id == 0) ? NULL : P_FindNewPosition(id);
			break;
		default:
			break;
		}
	}
}

static inline void P_ArchiveLuabanksAndConsistency(save_t *save_p)
{
	UINT8 i, banksinuse = NUM_LUABANKS;

	while (banksinuse && !luabanks[banksinuse-1])
		banksinuse--; // get the last used bank

	if (banksinuse)
	{
		P_WriteUINT8(save_p, 0xb7); // luabanks marker
		P_WriteUINT8(save_p, banksinuse);
		for (i = 0; i < banksinuse; i++)
			P_WriteINT32(save_p, luabanks[i]);
	}

	P_WriteUINT8(save_p, 0x1d); // consistency marker
}

static inline boolean P_UnArchiveLuabanksAndConsistency(save_t *save_p)
{
	switch (P_ReadUINT8(save_p))
	{
		case 0xb7: // luabanks marker
			{
				UINT8 i, banksinuse = P_ReadUINT8(save_p);
				if (banksinuse > NUM_LUABANKS)
				{
					CONS_Alert(CONS_ERROR, M_GetText("Corrupt Luabanks! (Too many banks in use)\n"));
					return false;
				}
				for (i = 0; i < banksinuse; i++)
					luabanks[i] = P_ReadINT32(save_p);
				if (P_ReadUINT8(save_p) != 0x1d) // consistency marker
				{
					CONS_Alert(CONS_ERROR, M_GetText("Corrupt Luabanks! (Failed consistency check)\n"));
					return false;
				}
			}
		case 0x1d: // consistency marker
			break;
		default: // anything else is nonsense
			CONS_Alert(CONS_ERROR, M_GetText("Failed consistency check (???)\n"));
			return false;
	}

	return true;
}

void P_SaveGame(save_t *save_p, INT16 mapnum)
{
	P_ArchiveMisc(save_p, mapnum);
	P_ArchivePlayer(save_p);
	P_ArchiveLuabanksAndConsistency(save_p);
}

void P_SaveNetGame(save_t *save_p, boolean resending)
{
	thinker_t *th;
	mobj_t *mobj;
	INT32 i = 1; // don't start from 0, it'd be confused with a blank pointer otherwise

	CV_SaveNetVars(save_p);
	P_NetArchiveMisc(save_p, resending);
	P_NetArchiveEmblems(save_p);

	// Assign the mobjnumber for pointer tracking
	for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
	{
		if (th->removing)
			continue;

		mobj = (mobj_t *)th;
		if (mobj->type == MT_HOOP || mobj->type == MT_HOOPCOLLIDE || mobj->type == MT_HOOPCENTER)
			continue;
		mobj->mobjnum = i++;
	}

	P_NetArchivePlayers(save_p);
	if (gamestate == GS_LEVEL)
	{
		P_NetArchiveWorld(save_p);
		P_ArchivePolyObjects(save_p);
		P_NetArchiveThinkers(save_p);
		P_NetArchiveSpecials(save_p);
		P_NetArchiveColormaps(save_p);
		P_NetArchiveWaypoints(save_p);
		P_NetArchiveSectorPortals(save_p);
	}
	LUA_Archive(save_p);

	P_ArchiveLuabanksAndConsistency(save_p);
}

boolean P_LoadGame(save_t *save_p, INT16 mapoverride)
{
	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	G_SetGamestate(GS_NULL); // should be changed in P_UnArchiveMisc

	P_UnArchiveSPGame(save_p, mapoverride);
	P_UnArchivePlayer(save_p);

	if (!P_UnArchiveLuabanksAndConsistency(save_p))
		return false;

	// Only do this after confirming savegame is ok
	G_DeferedInitNew(false, G_BuildMapName(gamemap), savedata.skin, false, true);
	COM_BufAddText("dummyconsvar 1\n"); // G_DeferedInitNew doesn't do this

	return true;
}

boolean P_LoadNetGame(save_t *save_p, boolean reloading)
{
	CV_LoadNetVars(save_p);
	if (!P_NetUnArchiveMisc(save_p, reloading))
		return false;
	P_NetUnArchiveEmblems(save_p);
	P_NetUnArchivePlayers(save_p);
	if (gamestate == GS_LEVEL)
	{
		P_NetUnArchiveWorld(save_p);
		P_UnArchivePolyObjects(save_p);
		P_NetUnArchiveThinkers(save_p);
		P_NetUnArchiveSpecials(save_p);
		P_NetUnArchiveColormaps(save_p);
		P_NetUnArchiveWaypoints(save_p);
		P_NetUnArchiveSectorPortals(save_p);
		P_RelinkPointers();
		P_FinishMobjs();
	}
	LUA_UnArchive(save_p);

	// This is stupid and hacky, but maybe it'll work!
	P_SetRandSeed(P_GetInitSeed());

	// The precipitation would normally be spawned in P_SetupLevel, which is called by
	// P_NetUnArchiveMisc above. However, that would place it up before P_NetUnArchiveThinkers,
	// so the thinkers would be deleted later. Therefore, P_SetupLevel will *not* spawn
	// precipitation when loading a netgame save. Instead, precip has to be spawned here.
	// This is done in P_NetUnArchiveSpecials now.

	return P_UnArchiveLuabanksAndConsistency(save_p);
}
