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
/// \file  p_setup.h
/// \brief Setup a game, startup stuff

#ifndef __P_SETUP__
#define __P_SETUP__

#include "doomdata.h"
#include "doomstat.h"
#include "r_defs.h"

// map md5, sent to players via PT_SERVERINFO
extern unsigned char mapmd5[16];

// Player spawn spots for deathmatch.
#define MAX_DM_STARTS 64
extern mapthing_t *deathmatchstarts[MAX_DM_STARTS];
extern INT32 numdmstarts, numcoopstarts, numredctfstarts, numbluectfstarts;

extern boolean levelloading;
extern UINT8 levelfadecol;

extern lumpnum_t lastloadedmaplumpnum; // for comparative savegame

/* for levelflat type */
enum
{
	LEVELFLAT_NONE,/* HOM time my friend */
	LEVELFLAT_FLAT,
	LEVELFLAT_PATCH,
	LEVELFLAT_PNG,
	LEVELFLAT_TEXTURE,
};

//
// MAP used flats lookup table
//
typedef struct
{
	char name[9]; // resource name from wad

	UINT8  type;
	union
	{
		struct
		{
			lumpnum_t     lumpnum; // lump number of the flat
			// for flat animation
			lumpnum_t baselumpnum;
		}
		flat;
		struct
		{
			INT32             num;
			INT32         lastnum; // texture number of the flat
			// for flat animation
			INT32         basenum;
		}
		texture;
	}
	u;

	UINT16 width, height;

	// for flat animation
	INT32 animseq; // start pos. in the anim sequence
	INT32 numpics;
	INT32 speed;

	// for textures
	UINT8 *picture;
#ifdef HWRENDER
	void *mipmap;
#endif
} levelflat_t;

extern size_t numlevelflats;
extern levelflat_t *levelflats;
INT32 P_AddLevelFlat(const char *flatname, levelflat_t *levelflat);
INT32 P_AddLevelFlatRuntime(const char *flatname);
INT32 P_CheckLevelFlat(const char *flatname);

extern size_t nummapthings;
extern mapthing_t *mapthings;

void P_SetupLevelSky(INT32 skynum, boolean global);
#ifdef SCANTHINGS
void P_ScanThings(INT16 mapnum, INT16 wadnum, INT16 lumpnum);
#endif
void P_RespawnThings(void);
boolean P_LoadLevel(boolean fromnetsave);
#ifdef HWRENDER
void HWR_SetupLevel(void);
#endif
boolean P_AddWadFile(const char *wadfilename);
boolean P_RunSOC(const char *socfilename);
void P_LoadSoundsRange(UINT16 wadnum, UINT16 first, UINT16 num);
void P_LoadMusicsRange(UINT16 wadnum, UINT16 first, UINT16 num);
void P_WriteThings(lumpnum_t lump);
size_t P_PrecacheLevelFlats(void);
void P_AllocMapHeader(INT16 i);

void P_SetDemoFlickies(INT16 i);
void P_DeleteFlickies(INT16 i);

// Needed for NiGHTS
void P_ReloadRings(void);
void P_SwitchSpheresBonusMode(boolean bonustime);
void P_DeleteGrades(INT16 i);
void P_AddGradesForMare(INT16 i, UINT8 mare, char *gtext);
UINT8 P_GetGrade(UINT32 pscore, INT16 map, UINT8 mare);
UINT8 P_HasGrades(INT16 map, UINT8 mare);
UINT32 P_GetScoreForGrade(INT16 map, UINT8 mare, UINT8 grade);

#endif
