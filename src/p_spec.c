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
/// \file  p_spec.c
/// \brief Implements special effects:
///        Texture animation, height or lighting changes
///        according to adjacent sectors, respective
///        utility functions, etc.
///        Line Tag handling. Line and Sector triggers.

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "p_setup.h" // levelflats for flat animation
#include "r_data.h"
#include "m_random.h"
#include "p_mobj.h"
#include "i_system.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "r_main.h" //Two extra includes.
#include "r_sky.h"
#include "p_polyobj.h"
#include "p_slopes.h"
#include "hu_stuff.h"
#include "m_misc.h"
#include "m_cond.h" //unlock triggers
#include "lua_hook.h" // LUAh_LinedefExecute

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

// Not sure if this is necessary, but it was in w_wad.c, so I'm putting it here too -Shadow Hog
#ifdef _WIN32_WCE
#define AVOID_ERRNO
#else
#include <errno.h>
#endif

mobj_t *skyboxmo[2];

// Amount (dx, dy) vector linedef is shifted right to get scroll amount
#define SCROLL_SHIFT 5

/** Animated texture descriptor
  * This keeps track of an animated texture or an animated flat.
  * \sa P_UpdateSpecials, P_InitPicAnims, animdef_t
  */
typedef struct
{
	SINT8 istexture; ///< ::true for a texture, ::false for a flat
	INT32 picnum;    ///< The end flat number
	INT32 basepic;   ///< The start flat number
	INT32 numpics;   ///< Number of frames in the animation
	tic_t speed;     ///< Number of tics for which each frame is shown
} anim_t;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

/** Animated texture definition.
  * Used for ::harddefs and for loading an ANIMATED lump from a wad.
  *
  * Animations are defined by the first and last frame (i.e., flat or texture).
  * The animation sequence uses all flats between the start and end entry, in
  * the order found in the wad.
  *
  * \sa anim_t
  */
typedef struct
{
	SINT8 istexture; ///< True for a texture, false for a flat.
	char endname[9]; ///< Name of the last frame, null-terminated.
	char startname[9]; ///< Name of the first frame, null-terminated.
	INT32 speed ; ///< Number of tics for which each frame is shown.
} ATTRPACK animdef_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

typedef struct
{
	UINT32 count;
	thinker_t **thinkers;
} thinkerlist_t;

static void P_SearchForDisableLinedefs(void);
static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, mobj_t *source, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider); //SoM: 3/9/2000
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline);
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline);
static void P_AddFloatThinker(sector_t *sec, INT32 tag, line_t *sourceline);
//static void P_AddBridgeThinker(line_t *sourceline, sector_t *sec);
static void P_AddFakeFloorsByLine(size_t line, ffloortype_e ffloorflags, thinkerlist_t *secthinkers);
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec);
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer);
static void P_AddSpikeThinker(sector_t *sec, INT32 referrer);


//SoM: 3/7/2000: New sturcture without limits.
static anim_t *lastanim;
static anim_t *anims = NULL; /// \todo free leak
static size_t maxanims;

//
// P_InitPicAnims
//
/** Hardcoded animation sequences.
  * Used if no ANIMATED lump is found in a loaded wad.
  */
static animdef_t harddefs[] =
{
	// flat animations.
	{false,     "LITEY3",       "LITEY1",       4},
	{false,     "FWATER16",     "FWATER1",      4},
	{false,     "BWATER16",     "BWATER01",     4},
	{false,     "LWATER16",     "LWATER1",      4},
	{false,     "WATER7",       "WATER0",       4},
	{false,     "LAVA4",        "LAVA1",        8},
	{false,     "DLAVA4",       "DLAVA1",       8},
	{false,     "RLAVA8",       "RLAVA1",       8},
	{false,     "LITER3",       "LITER1",       8},
	{false,     "SURF08",       "SURF01",       4},

	{false,     "CHEMG16",      "CHEMG01",      4}, // THZ Chemical gunk
	{false,     "GOOP16",       "GOOP01",       4}, // Green chemical gunk
	{false,     "OIL16",        "OIL01",        4}, // Oil
	{false,     "THZBOXF4",     "THZBOXF1",     2}, // Moved up with the flats
	{false,     "ALTBOXF4",     "ALTBOXF1",     2},

	{false,     "LITEB3",       "LITEB1",       4},
	{false,     "LITEN3",       "LITEN1",       4},
	{false,     "ACZRFL1H",     "ACZRFL1A",     4},
	{false,     "ACZRFL2H",     "ACZRFL2A",     4},
	{false,     "EGRIDF3",      "EGRIDF1",      4},
	{false,     "ERZFAN4",      "ERZFAN1",      1},
	{false,     "ERZFANR4",     "ERZFANR1",     1},
	{false,     "DISCO4",       "DISCO1",      15},

	// animated textures
	{true,      "GFALL4",       "GFALL1",       2}, // Short waterfall
	{true,      "CFALL4",       "CFALL1",       2}, // Long waterfall
	{true,      "TFALL4",       "TFALL1",       2}, // THZ Chemical fall
	{true,      "AFALL4",       "AFALL1",       2}, // Green Chemical fall
	{true,      "QFALL4",       "QFALL1",       2}, // Quicksand fall
	{true,      "Q2FALL4",      "Q2FALL1",      2},
	{true,      "Q3FALL4",      "Q3FALL1",      2},
	{true,      "Q4FALL4",      "Q4FALL1",      2},
	{true,      "Q5FALL4",      "Q5FALL1",      2},
	{true,      "Q6FALL4",      "Q6FALL1",      2},
	{true,      "Q7FALL4",      "Q7FALL1",      2},
	{true,      "LFALL4",       "LFALL1",       2},
	{true,      "MFALL4",       "MFALL1",       2},
	{true,      "OFALL4",       "OFALL1",       2},
	{true,      "DLAVA4",       "DLAVA1",       8},
	{true,      "ERZLASA2",     "ERZLASA1",     1},
	{true,      "ERZLASB4",     "ERZLASB1",     1},
	{true,      "ERZLASC4",     "ERZLASC1",     1},
	{true,      "THZBOX04",     "THZBOX01",     2},
	{true,      "ALTBOX04",     "ALTBOX01",     2},
	{true,      "SFALL4",       "SFALL1",       4}, // Lava fall
	{true,      "RVZFALL8",     "RVZFALL1",     4},
	{true,      "BFALL4",       "BFALL1",       2}, // HPZ waterfall
	{true,      "GREYW3",       "GREYW1",       4},
	{true,      "BLUEW3",       "BLUEW1",       4},
	{true,      "COMP6",        "COMP4",        4},
	{true,      "RED3",         "RED1",         4},
	{true,      "YEL3",         "YEL1",         4},
	{true,      "ACWRFL1D",     "ACWRFL1A",     1},
	{true,      "ACWRFL2D",     "ACWRFL2A",     1},
	{true,      "ACWRFL3D",     "ACWRFL3A",     1},
	{true,      "ACWRFL4D",     "ACWRFL4A",     1},
	{true,      "ACWRP1D",      "ACWRP1A",      1},
	{true,      "ACWRP2D",      "ACWRP2A",      1},
	{true,      "ACZRP1D",      "ACZRP1A",      1},
	{true,      "ACZRP2D",      "ACZRP2A",      1},
	{true,      "OILFALL4",     "OILFALL1",     2},
	{true,      "SOLFALL4",     "SOLFALL1",     2},
	{true,      "DOWN1C",       "DOWN1A",       4},
	{true,      "DOWN2C",       "DOWN2A",       4},
	{true,      "DOWN3D",       "DOWN3A",       4},
	{true,      "DOWN4C",       "DOWN4A",       4},
	{true,      "DOWN5C",       "DOWN5A",       4},
	{true,      "UP1C",         "UP1A",         4},
	{true,      "UP2C",         "UP2A",         4},
	{true,      "UP3D",         "UP3A",         4},
	{true,      "UP4C",         "UP4A",         4},
	{true,      "UP5C",         "UP5A",         4},
	{true,      "EGRID3",       "EGRID1",       4},
	{true,      "ERFANW4",      "ERFANW1",      1},
	{true,      "ERFANX4",      "ERFANX1",      1},
	{true,      "DISCOD4",      "DISCOD1",     15},
	{true,      "DANCE4",       "DANCE1",       8},
	{true,      "SKY135",       "SKY132",       2},
	{true,      "APPLMS4",      "APPLMS1",      2},
	{true,      "APBOXW3",      "APBOXW1",      2},
	{true,      "ERZLAZC4",     "ERZLAZC1",     4},

	// End of line
	{   -1,             "",            "",    0},
};

// Animating line specials

// Init animated textures
// - now called at level loading P_SetupLevel()

static animdef_t *animdefs = NULL;

// A prototype; here instead of p_spec.h, so they're "private"
void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum);
void P_ParseAnimationDefintion(SINT8 istexture);

/** Sets up texture and flat animations.
  *
  * Converts an ::animdef_t array loaded from ::harddefs or a lump into
  * ::anim_t format.
  *
  * Issues an error if any animation cycles are invalid.
  *
  * \sa P_FindAnimatedFlat, P_SetupLevelFlatAnims
  * \author Steven McGranahan (original), Shadow Hog (had to rewrite it to handle multiple WADs), JTE (had to rewrite it to handle multiple WADs _correctly_)
  */
void P_InitPicAnims(void)
{
	// Init animation
	INT32 w; // WAD
	UINT8 *animatedLump;
	UINT8 *currentPos;
	size_t i;

	I_Assert(animdefs == NULL);

	if (W_CheckNumForName("ANIMATED") != LUMPERROR || W_CheckNumForName("ANIMDEFS") != LUMPERROR)
	{
		for (w = numwadfiles-1, maxanims = 0; w >= 0; w--)
		{
			UINT16 animatedLumpNum;
			UINT16 animdefsLumpNum;

			// Find ANIMATED lump in the WAD
			animatedLumpNum = W_CheckNumForNamePwad("ANIMATED", w, 0);
			if (animatedLumpNum != INT16_MAX)
			{
				animatedLump = (UINT8 *)W_CacheLumpNumPwad(w, animatedLumpNum, PU_STATIC);

				// Get the number of animations in the file
				i = maxanims;
				for (currentPos = animatedLump; *currentPos != UINT8_MAX; maxanims++, currentPos+=23);

				// Resize animdefs (or if it hasn't been created, create it)
				animdefs = (animdef_t *)Z_Realloc(animdefs, sizeof(animdef_t)*(maxanims + 1), PU_STATIC, NULL);
				// Sanity check it
				if (!animdefs)
					I_Error("Not enough free memory for ANIMATED data");

				// Populate the new array
				for (currentPos = animatedLump; *currentPos != UINT8_MAX; i++, currentPos+=23)
				{
					M_Memcpy(&(animdefs[i].istexture), currentPos, 1); // istexture, 1 byte
					M_Memcpy(animdefs[i].endname, (currentPos + 1), 9); // endname, 9 bytes
					M_Memcpy(animdefs[i].startname, (currentPos + 10), 9); // startname, 9 bytes
					M_Memcpy(&(animdefs[i].speed), (currentPos + 19), 4); // speed, 4 bytes
				}

				Z_Free(animatedLump);
			}

			// Now find ANIMDEFS
			animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", w, 0);
			if (animdefsLumpNum != INT16_MAX)
				P_ParseANIMDEFSLump(w, animdefsLumpNum);
		}
		// Define the last one
		animdefs[maxanims].istexture = -1;
		strncpy(animdefs[maxanims].endname, "", 9);
		strncpy(animdefs[maxanims].startname, "", 9);
		animdefs[maxanims].speed = 0;
	}
	else
	{
		animdefs = harddefs;
		for (maxanims = 0; animdefs[maxanims].istexture != -1; maxanims++);
	}

	if (anims)
		free(anims);

	anims = (anim_t *)malloc(sizeof (*anims)*(maxanims + 1));
	if (!anims)
		I_Error("Not enough free memory for ANIMATED data");

	lastanim = anims;
	for (i = 0; animdefs[i].istexture != -1; i++)
	{
		if (animdefs[i].istexture)
		{
			if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
				continue;

			lastanim->picnum = R_TextureNumForName(animdefs[i].endname);
			lastanim->basepic = R_TextureNumForName(animdefs[i].startname);
		}
		else
		{
			if ((W_CheckNumForName(animdefs[i].startname)) == LUMPERROR)
				continue;

			lastanim->picnum = R_FlatNumForName(animdefs[i].endname);
			lastanim->basepic = R_FlatNumForName(animdefs[i].startname);
		}

		lastanim->istexture = animdefs[i].istexture;
		lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

		if (lastanim->numpics < 2)
		{
			free(anims);
			I_Error("P_InitPicAnims: bad cycle from %s to %s",
				animdefs[i].startname, animdefs[i].endname);
		}

		if (animdefs == harddefs)
			lastanim->speed = animdefs[i].speed;
		else
			lastanim->speed = LONG(animdefs[i].speed);
		lastanim++;
	}
	lastanim->istexture = -1;
	R_ClearTextureNumCache(false);

	// Clear animdefs now that we're done with it.
	// We'll only be using anims from now on.
	if (animdefs != harddefs)
		Z_Free(animdefs);
	animdefs = NULL;
}

void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum)
{
	char *animdefsLump;
	size_t animdefsLumpLength;
	char *animdefsText;
	char *animdefsToken;
	char *p;

	// Since lumps AREN'T \0-terminated like I'd assumed they should be, I'll
	// need to make a space of memory where I can ensure that it will terminate
	// correctly. Start by loading the relevant data from the WAD.
	animdefsLump = (char *)W_CacheLumpNumPwad(wadNum,lumpnum,PU_STATIC);
	// If that didn't exist, we have nothing to do here.
	if (animdefsLump == NULL) return;
	// If we're still here, then it DOES exist; figure out how long it is, and allot memory accordingly.
	animdefsLumpLength = W_LumpLengthPwad(wadNum,lumpnum);
	animdefsText = (char *)Z_Malloc((animdefsLumpLength+1)*sizeof(char),PU_STATIC,NULL);
	// Now move the contents of the lump into this new location.
	memmove(animdefsText,animdefsLump,animdefsLumpLength);
	// Make damn well sure the last character in our new memory location is \0.
	animdefsText[animdefsLumpLength] = '\0';
	// Finally, free up the memory from the first data load, because we really
	// don't need it.
	Z_Free(animdefsLump);

	// Now, let's start parsing this thing
	p = animdefsText;
	animdefsToken = M_GetToken(p);
	while (animdefsToken != NULL)
	{
		if (stricmp(animdefsToken, "TEXTURE") == 0)
		{
			Z_Free(animdefsToken);
			P_ParseAnimationDefintion(1);
		}
		else if (stricmp(animdefsToken, "FLAT") == 0)
		{
			Z_Free(animdefsToken);
			P_ParseAnimationDefintion(0);
		}
		else if (stricmp(animdefsToken, "OSCILLATE") == 0)
		{
			// This probably came off the tail of an earlier definition. It's technically legal syntax, but we don't support it.
			I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"OSCILLATE\" (the animation plays in reverse when it reaches the end) are not supported by SRB2");
		}
		else
		{
			I_Error("Error parsing ANIMDEFS lump: Expected \"TEXTURE\" or \"FLAT\", got \"%s\"",animdefsToken);
		}
		// parse next line
		while (*p != '\0' && *p != '\n') ++p;
		if (*p == '\n') ++p;
		animdefsToken = M_GetToken(p);
	}
	Z_Free(animdefsToken);
	Z_Free((void *)animdefsText);
}

void P_ParseAnimationDefintion(SINT8 istexture)
{
	char *animdefsToken;
	size_t animdefsTokenLength;
	char *endPos;
	INT32 animSpeed;
	size_t i;

	// Startname
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where start texture/flat name should be");
	}
	if (stricmp(animdefsToken, "OPTIONAL") == 0)
	{
		// This is meaningful to ZDoom - it tells the program NOT to bomb out
		// if the textures can't be found - but it's useless in SRB2, so we'll
		// just smile, nod, and carry on
		Z_Free(animdefsToken);
		animdefsToken = M_GetToken(NULL);

		if (animdefsToken == NULL)
		{
			I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where start texture/flat name should be");
		}
		else if (stricmp(animdefsToken, "RANGE") == 0)
		{
			// Oh. Um. Apparently "OPTIONAL" is a texture name. Naughty.
			// I should probably handle this more gracefully, but right now
			// I can't be bothered; especially since ZDoom doesn't handle this
			// condition at all.
			I_Error("Error parsing ANIMDEFS lump: \"OPTIONAL\" is a keyword; you cannot use it as the startname of an animation");
		}
	}
	animdefsTokenLength = strlen(animdefsToken);
	if (animdefsTokenLength>8)
	{
		I_Error("Error parsing ANIMDEFS lump: lump name \"%s\" exceeds 8 characters", animdefsToken);
	}

	// Search for existing animdef
	for (i = 0; i < maxanims; i++)
		if (stricmp(animdefsToken, animdefs[i].startname) == 0)
		{
			//CONS_Alert(CONS_NOTICE, "Duplicate animation: %s\n", animdefsToken);

			// If we weren't parsing in reverse order, we would `break` here and parse the new data into the existing slot we found.
			// Instead, we're just going to skip parsing the rest of this line entirely.
			Z_Free(animdefsToken);
			return;
		}

	// Not found
	if (i == maxanims)
	{
		// Increase the size to make room for the new animation definition
		maxanims++;
		animdefs = (animdef_t *)Z_Realloc(animdefs, sizeof(animdef_t)*(maxanims + 1), PU_STATIC, NULL);
		strncpy(animdefs[i].startname, animdefsToken, 9);
	}

	// animdefs[i].startname is now set to animdefsToken either way.
	Z_Free(animdefsToken);

	// set texture type
	animdefs[i].istexture = istexture;

	// "RANGE"
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"RANGE\" after \"%s\"'s startname should be", animdefs[i].startname);
	}
	if (stricmp(animdefsToken, "ALLOWDECALS") == 0)
	{
		// Another ZDoom keyword, ho-hum. Skip it, move on to the next token.
		Z_Free(animdefsToken);
		animdefsToken = M_GetToken(NULL);
	}
	if (stricmp(animdefsToken, "PIC") == 0)
	{
		// This is technically legitimate ANIMDEFS syntax, but SRB2 doesn't support it.
		I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"PIC\" (specific frames instead of a consecutive range) are not supported by SRB2");
	}
	if (stricmp(animdefsToken, "RANGE") != 0)
	{
		I_Error("Error parsing ANIMDEFS lump: Expected \"RANGE\" after \"%s\"'s startname, got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	Z_Free(animdefsToken);

	// Endname
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s end texture/flat name should be", animdefs[i].startname);
	}
	animdefsTokenLength = strlen(animdefsToken);
	if (animdefsTokenLength>8)
	{
		I_Error("Error parsing ANIMDEFS lump: lump name \"%s\" exceeds 8 characters", animdefsToken);
	}
	strncpy(animdefs[i].endname, animdefsToken, 9);
	Z_Free(animdefsToken);

	// "TICS"
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s \"TICS\" should be", animdefs[i].startname);
	}
	if (stricmp(animdefsToken, "RAND") == 0)
	{
		// This is technically legitimate ANIMDEFS syntax, but SRB2 doesn't support it.
		I_Error("Error parsing ANIMDEFS lump: Animation definitions utilizing \"RAND\" (random duration per frame) are not supported by SRB2");
	}
	if (stricmp(animdefsToken, "TICS") != 0)
	{
		I_Error("Error parsing ANIMDEFS lump: Expected \"TICS\" in animation definition for \"%s\", got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	Z_Free(animdefsToken);

	// Speed
	animdefsToken = M_GetToken(NULL);
	if (animdefsToken == NULL)
	{
		I_Error("Error parsing ANIMDEFS lump: Unexpected end of file where \"%s\"'s animation speed should be", animdefs[i].startname);
	}
	endPos = NULL;
#ifndef AVOID_ERRNO
	errno = 0;
#endif
	animSpeed = strtol(animdefsToken,&endPos,10);
	if (endPos == animdefsToken // Empty string
		|| *endPos != '\0' // Not end of string
#ifndef AVOID_ERRNO
		|| errno == ERANGE // Number out-of-range
#endif
		|| animSpeed < 0) // Number is not positive
	{
		I_Error("Error parsing ANIMDEFS lump: Expected a positive integer for \"%s\"'s animation speed, got \"%s\"", animdefs[i].startname, animdefsToken);
	}
	animdefs[i].speed = animSpeed;
	Z_Free(animdefsToken);
}


/** Checks for flats in levelflats that are part of a flat animation sequence
  * and sets them up for animation.
  *
  * \param animnum Index into ::anims to find flats for.
  * \sa P_SetupLevelFlatAnims
  */
static inline void P_FindAnimatedFlat(INT32 animnum)
{
	size_t i;
	lumpnum_t startflatnum, endflatnum;
	levelflat_t *foundflats;

	foundflats = levelflats;
	startflatnum = anims[animnum].basepic;
	endflatnum = anims[animnum].picnum;

	// note: high word of lumpnum is the wad number
	if ((startflatnum>>16) != (endflatnum>>16))
		I_Error("AnimatedFlat start %s not in same wad as end %s\n",
			animdefs[animnum].startname, animdefs[animnum].endname);

	//
	// now search through the levelflats if this anim flat sequence is used
	//
	for (i = 0; i < numlevelflats; i++, foundflats++)
	{
		// is that levelflat from the flat anim sequence ?
		if (foundflats->lumpnum >= startflatnum && foundflats->lumpnum <= endflatnum)
		{
			foundflats->baselumpnum = startflatnum;
			foundflats->animseq = foundflats->lumpnum - startflatnum;
			foundflats->numpics = endflatnum - startflatnum + 1;
			foundflats->speed = anims[animnum].speed;

			CONS_Debug(DBG_SETUP, "animflat: #%03d name:%.8s animseq:%d numpics:%d speed:%d\n",
					atoi(sizeu1(i)), foundflats->name, foundflats->animseq,
					foundflats->numpics,foundflats->speed);
		}
	}
}

/** Sets up all flats used in a level.
  *
  * \sa P_InitPicAnims, P_FindAnimatedFlat
  */
void P_SetupLevelFlatAnims(void)
{
	INT32 i;

	// the original game flat anim sequences
	for (i = 0; anims[i].istexture != -1; i++)
	{
		if (!anims[i].istexture)
			P_FindAnimatedFlat(i);
	}
}

//
// UTILITIES
//

#if 0
/** Gets a side from a sector line.
  *
  * \param currentSector Sector the line is in.
  * \param line          Index of the line within the sector.
  * \param side          0 for front, 1 for back.
  * \return Pointer to the side_t of the side you want.
  * \sa getSector, twoSided, getNextSector
  */
static inline side_t *getSide(INT32 currentSector, INT32 line, INT32 side)
{
	return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}

/** Gets a sector from a sector line.
  *
  * \param currentSector Sector the line is in.
  * \param line          Index of the line within the sector.
  * \param side          0 for front, 1 for back.
  * \return Pointer to the ::sector_t of the sector on that side.
  * \sa getSide, twoSided, getNextSector
  */
static inline sector_t *getSector(INT32 currentSector, INT32 line, INT32 side)
{
	return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

/** Determines whether a sector line is two-sided.
  * Uses the Boom method, checking if the line's back side is set to -1, rather
  * than looking for ::ML_TWOSIDED.
  *
  * \param sector The sector.
  * \param line   Line index within the sector.
  * \return 1 if the sector is two-sided, 0 otherwise.
  * \sa getSide, getSector, getNextSector
  */
static inline boolean twoSided(INT32 sector, INT32 line)
{
	return (sectors[sector].lines[line])->sidenum[1] != 0xffff;
}
#endif

/** Finds sector next to current.
  *
  * \param line Pointer to the line to cross.
  * \param sec  Pointer to the current sector.
  * \return Pointer to a ::sector_t of the adjacent sector, or NULL if the line
  *         is one-sided.
  * \sa getSide, getSector, twoSided
  * \author Steven McGranahan
  */
static sector_t *getNextSector(line_t *line, sector_t *sec)
{
	if (line->frontsector == sec)
	{
		if (line->backsector != sec)
			return line->backsector;
		else
			return NULL;
	}
	return line->frontsector;
}

/** Finds lowest floor in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Lowest floor height in an adjacent sector.
  * \sa P_FindHighestFloorSurrounding, P_FindNextLowestFloor,
  *     P_FindLowestCeilingSurrounding
  */
fixed_t P_FindLowestFloorSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t floorh;

	floorh = sec->floorheight;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check,sec);

		if (!other)
			continue;

		if (other->floorheight < floorh)
			floorh = other->floorheight;
	}
	return floorh;
}

/** Finds highest floor in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Highest floor height in an adjacent sector.
  * \sa P_FindLowestFloorSurrounding, P_FindNextHighestFloor,
  *     P_FindHighestCeilingSurrounding
  */
fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t floorh = -500*FRACUNIT;
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->floorheight > floorh || !foundsector)
			floorh = other->floorheight;

		if (!foundsector)
			foundsector = 1;
	}
	return floorh;
}

/** Finds next highest floor in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next highest floor height in an adjacent sector, or currentheight
  *         if there are none higher.
  * \sa P_FindHighestFloorSurrounding, P_FindNextLowestFloor,
  *     P_FindNextHighestCeiling
  * \author Lee Killough
  */
fixed_t P_FindNextHighestFloor(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if (other && other->floorheight > currentheight)
		{
			height = other->floorheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if (other &&
					other->floorheight < height &&
					other->floorheight > currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

////////////////////////////////////////////////////
// SoM: Start new Boom functions
////////////////////////////////////////////////////

/** Finds next lowest floor in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next lowest floor height in an adjacent sector, or currentheight
  *         if there are none lower.
  * \sa P_FindLowestFloorSurrounding, P_FindNextHighestFloor,
  *     P_FindNextLowestCeiling
  * \author Lee Killough
  */
fixed_t P_FindNextLowestFloor(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if (other && other->floorheight < currentheight)
		{
			height = other->floorheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i], sec);
				if (other &&	other->floorheight > height
					&& other->floorheight < currentheight)
					height = other->floorheight;
			}
			return height;
		}
	}
	return currentheight;
}

#if 0
/** Finds next lowest ceiling in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next lowest ceiling height in an adjacent sector, or currentheight
  *         if there are none lower.
  * \sa P_FindLowestCeilingSurrounding, P_FindNextHighestCeiling,
  *     P_FindNextLowestFloor
  * \author Lee Killough
  */
static fixed_t P_FindNextLowestCeiling(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i],sec);
		if (other &&	other->ceilingheight < currentheight)
		{
			height = other->ceilingheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if (other &&	other->ceilingheight > height
					&& other->ceilingheight < currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}

/** Finds next highest ceiling in adjacent sectors.
  *
  * \param sec           Sector to start in.
  * \param currentheight Height to start at.
  * \return Next highest ceiling height in an adjacent sector, or currentheight
  *         if there are none higher.
  * \sa P_FindHighestCeilingSurrounding, P_FindNextLowestCeiling,
  *     P_FindNextHighestFloor
  * \author Lee Killough
  */
static fixed_t P_FindNextHighestCeiling(sector_t *sec, fixed_t currentheight)
{
	sector_t *other;
	size_t i;
	fixed_t height;

	for (i = 0; i < sec->linecount; i++)
	{
		other = getNextSector(sec->lines[i], sec);
		if (other && other->ceilingheight > currentheight)
		{
			height = other->ceilingheight;
			while (++i < sec->linecount)
			{
				other = getNextSector(sec->lines[i],sec);
				if (other && other->ceilingheight < height
					&& other->ceilingheight > currentheight)
					height = other->ceilingheight;
			}
			return height;
		}
	}
	return currentheight;
}
#endif

////////////////////////////
// End New Boom functions
////////////////////////////

/** Finds lowest ceiling in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Lowest ceiling height in an adjacent sector.
  * \sa P_FindHighestCeilingSurrounding, P_FindNextLowestCeiling,
  *     P_FindLowestFloorSurrounding
  */
fixed_t P_FindLowestCeilingSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t height = 32000*FRACUNIT; //SoM: 3/7/2000: Remove ovf
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight < height || !foundsector)
			height = other->ceilingheight;

		if (!foundsector)
			foundsector = 1;
	}
	return height;
}

/** Finds Highest ceiling in adjacent sectors.
  *
  * \param sec Sector to start in.
  * \return Highest ceiling height in an adjacent sector.
  * \sa P_FindLowestCeilingSurrounding, P_FindNextHighestCeiling,
  *     P_FindHighestFloorSurrounding
  */
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec)
{
	size_t i;
	line_t *check;
	sector_t *other;
	fixed_t height = 0;
	INT32 foundsector = 0;

	for (i = 0; i < sec->linecount; i++)
	{
		check = sec->lines[i];
		other = getNextSector(check, sec);

		if (!other)
			continue;

		if (other->ceilingheight > height || !foundsector)
			height = other->ceilingheight;

		if (!foundsector)
			foundsector = 1;
	}
	return height;
}

#if 0
//SoM: 3/7/2000: UTILS.....
//
// P_FindShortestTextureAround()
//
// Passed a sector number, returns the shortest lower texture on a
// linedef bounding the sector.
//
//
static fixed_t P_FindShortestTextureAround(INT32 secnum)
{
	fixed_t minsize = 32000<<FRACBITS;
	side_t *side;
	size_t i;
	sector_t *sec= &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if (side->bottomtexture > 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
			side = getSide(secnum,i,1);
			if (side->bottomtexture > 0)
				if (textureheight[side->bottomtexture] < minsize)
					minsize = textureheight[side->bottomtexture];
		}
	}
	return minsize;
}

//SoM: 3/7/2000: Stuff.... (can you tell I'm getting tired? It's 12 : 30!)
//
// P_FindShortestUpperAround()
//
// Passed a sector number, returns the shortest upper texture on a
// linedef bounding the sector.
//
//
static fixed_t P_FindShortestUpperAround(INT32 secnum)
{
	fixed_t minsize = 32000<<FRACBITS;
	side_t *side;
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			side = getSide(secnum,i,0);
			if (side->toptexture > 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
			side = getSide(secnum,i,1);
			if (side->toptexture > 0)
				if (textureheight[side->toptexture] < minsize)
					minsize = textureheight[side->toptexture];
		}
	}
	return minsize;
}

//SoM: 3/7/2000
//
// P_FindModelFloorSector()
//
// Passed a floor height and a sector number, return a pointer to a
// a sector with that floor height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
//
static sector_t *P_FindModelFloorSector(fixed_t floordestheight, INT32 secnum)
{
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			if (getSide(secnum,i,0)->sector-sectors == secnum)
				sec = getSector(secnum,i,1);
			else
				sec = getSector(secnum,i,0);

			if (sec->floorheight == floordestheight)
				return sec;
		}
	}
	return NULL;
}

//
// P_FindModelCeilingSector()
//
// Passed a ceiling height and a sector number, return a pointer to a
// a sector with that ceiling height across the lowest numbered two sided
// line surrounding the sector.
//
// Note: If no sector at that height bounds the sector passed, return NULL
//
static sector_t *P_FindModelCeilingSector(fixed_t ceildestheight, INT32 secnum)
{
	size_t i;
	sector_t *sec = &sectors[secnum];

	for (i = 0; i < sec->linecount; i++)
	{
		if (twoSided(secnum, i))
		{
			if (getSide(secnum, i, 0)->sector - sectors == secnum)
				sec = getSector(secnum, i, 1);
			else
				sec = getSector(secnum, i, 0);

			if (sec->ceilingheight == ceildestheight)
				return sec;
		}
	}
	return NULL;
}
#endif

/** Searches the tag lists for the next sector tagged to a line.
  *
  * \param line  Tagged line used as a reference.
  * \param start -1 to start at the beginning, or the result of a previous call
  *              to keep searching.
  * \return Number of the next tagged sector found.
  * \sa P_FindSectorFromTag, P_FindLineFromLineTag
  */
INT32 P_FindSectorFromLineTag(line_t *line, INT32 start)
{
	if (line->tag == -1)
	{
		start++;

		if (start >= (INT32)numsectors)
			return -1;

		return start;
	}
	else
	{
		start = start >= 0 ? sectors[start].nexttag :
			sectors[(unsigned)line->tag % numsectors].firsttag;
		while (start >= 0 && sectors[start].tag != line->tag)
			start = sectors[start].nexttag;
		return start;
	}
}

/** Searches the tag lists for the next sector with a given tag.
  *
  * \param tag   Tag number to look for.
  * \param start -1 to start anew, or the result of a previous call to keep
  *              searching.
  * \return Number of the next tagged sector found.
  * \sa P_FindSectorFromLineTag
  */
INT32 P_FindSectorFromTag(INT16 tag, INT32 start)
{
	if (tag == -1)
	{
		start++;

		if (start >= (INT32)numsectors)
			return -1;

		return start;
	}
	else
	{
		start = start >= 0 ? sectors[start].nexttag :
			sectors[(unsigned)tag % numsectors].firsttag;
		while (start >= 0 && sectors[start].tag != tag)
			start = sectors[start].nexttag;
		return start;
	}
}

/** Searches the tag lists for the next line tagged to a line.
  *
  * \param line  Tagged line used as a reference.
  * \param start -1 to start anew, or the result of a previous call to keep
  *              searching.
  * \return Number of the next tagged line found.
  * \sa P_FindSectorFromLineTag
  */
static INT32 P_FindLineFromLineTag(const line_t *line, INT32 start)
{
	if (line->tag == -1)
	{
		start++;

		if (start >= (INT32)numlines)
			return -1;

		return start;
	}
	else
	{
		start = start >= 0 ? lines[start].nexttag :
			lines[(unsigned)line->tag % numlines].firsttag;
		while (start >= 0 && lines[start].tag != line->tag)
			start = lines[start].nexttag;
		return start;
	}
}
#if 0
/** Searches the tag lists for the next line with a given tag and special.
  *
  * \param tag     Tag number.
  * \param start   -1 to start anew, or the result of a previous call to keep
  *                searching.
  * \return Number of next suitable line found.
  * \sa P_FindLineFromLineTag
  * \author Graue <graue@oceanbase.org>
  */
static INT32 P_FindLineFromTag(INT32 tag, INT32 start)
{
	if (tag == -1)
	{
		start++;

		if (start >= numlines)
			return -1;

		return start;
	}
	else
	{
		start = start >= 0 ? lines[start].nexttag :
			lines[(unsigned)tag % numlines].firsttag;
		while (start >= 0 && lines[start].tag != tag)
			start = lines[start].nexttag;
		return start;
	}
}
#endif
//
// P_FindSpecialLineFromTag
//
INT32 P_FindSpecialLineFromTag(INT16 special, INT16 tag, INT32 start)
{
	if (tag == -1)
	{
		start++;

		// This redundant check stops the compiler from complaining about function expansion
		// elsewhere for some reason and everything is awful
		if (start >= (INT32)numlines)
			return -1;

		while (start < (INT32)numlines && lines[start].special != special)
			start++;

		if (start >= (INT32)numlines)
			return -1;

		return start;
	}
	else
	{
		start = start >= 0 ? lines[start].nexttag :
			lines[(unsigned)tag % numlines].firsttag;
		while (start >= 0 && (lines[start].tag != tag || lines[start].special != special))
			start = lines[start].nexttag;
		return start;
	}
}

// haleyjd: temporary define
#ifdef POLYOBJECTS

//
// PolyDoor
//
// Parses arguments for parameterized polyobject door types
//
static boolean PolyDoor(line_t *line)
{
	polydoordata_t pdd;

	pdd.polyObjNum = line->tag; // polyobject id

	switch(line->special)
	{
		case 480: // Polyobj_DoorSlide
			pdd.doorType = POLY_DOOR_SLIDE;
			pdd.speed    = sides[line->sidenum[0]].textureoffset / 8;
			pdd.angle    = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y); // angle of motion
			pdd.distance = sides[line->sidenum[0]].rowoffset;

			if (line->sidenum[1] != 0xffff)
				pdd.delay = sides[line->sidenum[1]].textureoffset >> FRACBITS; // delay in tics
			else
				pdd.delay = 0;
			break;
		case 481: // Polyobj_DoorSwing
			pdd.doorType = POLY_DOOR_SWING;
			pdd.speed    = sides[line->sidenum[0]].textureoffset >> FRACBITS; // angular speed
			pdd.distance = sides[line->sidenum[0]].rowoffset >> FRACBITS; // angular distance

			if (line->sidenum[1] != 0xffff)
				pdd.delay = sides[line->sidenum[1]].textureoffset >> FRACBITS; // delay in tics
			else
				pdd.delay = 0;
			break;
		default:
			return 0; // ???
	}

	return EV_DoPolyDoor(&pdd);
}

//
// PolyMove
//
// Parses arguments for parameterized polyobject move specials
//
static boolean PolyMove(line_t *line)
{
	polymovedata_t pmd;

	pmd.polyObjNum = line->tag;
	pmd.speed      = sides[line->sidenum[0]].textureoffset / 8;
	pmd.angle      = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y);
	pmd.distance   = sides[line->sidenum[0]].rowoffset;

	pmd.overRide = (line->special == 483); // Polyobj_OR_Move

	return EV_DoPolyObjMove(&pmd);
}

//
// PolyInvisible
//
// Makes a polyobject invisible and intangible
// If NOCLIMB is ticked, the polyobject will still be tangible, just not visible.
//
static void PolyInvisible(line_t *line)
{
	INT32 polyObjNum = line->tag;
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyInvisible: bad polyobj %d\n", polyObjNum);
		return;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return;

	if (!(line->flags & ML_NOCLIMB))
		po->flags &= ~POF_SOLID;

	po->flags |= POF_NOSPECIALS;
	po->flags &= ~POF_RENDERALL;
}

//
// PolyVisible
//
// Makes a polyobject visible and tangible
// If NOCLIMB is ticked, the polyobject will not be tangible, just visible.
//
static void PolyVisible(line_t *line)
{
	INT32 polyObjNum = line->tag;
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyVisible: bad polyobj %d\n", polyObjNum);
		return;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return;

	if (!(line->flags & ML_NOCLIMB))
		po->flags |= POF_SOLID;

	po->flags &= ~POF_NOSPECIALS;
	po->flags |= POF_RENDERALL;
}

//
// PolyTranslucency
//
// Sets the translucency of a polyobject
// Frontsector floor / 100 = translevel
//
static void PolyTranslucency(line_t *line)
{
	INT32 polyObjNum = line->tag;
	polyobj_t *po;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: bad polyobj %d\n", polyObjNum);
		return;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return;

	po->translucency = (line->frontsector->floorheight >> FRACBITS) / 100;
}

//
// PolyWaypoint
//
// Parses arguments for parameterized polyobject waypoint movement
//
static boolean PolyWaypoint(line_t *line)
{
	polywaypointdata_t pwd;

	pwd.polyObjNum = line->tag;
	pwd.speed      = sides[line->sidenum[0]].textureoffset / 8;
	pwd.sequence   = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Sequence #
	pwd.reverse    = (line->flags & ML_EFFECT1) == ML_EFFECT1; // Reverse?
	pwd.comeback   = (line->flags & ML_EFFECT2) == ML_EFFECT2; // Return when reaching end?
	pwd.wrap       = (line->flags & ML_EFFECT3) == ML_EFFECT3; // Wrap around waypoints
	pwd.continuous = (line->flags & ML_EFFECT4) == ML_EFFECT4; // Continuously move - used with COMEBACK or WRAP

	return EV_DoPolyObjWaypoint(&pwd);
}

//
// PolyRotate
//
// Parses arguments for parameterized polyobject rotate specials
//
static boolean PolyRotate(line_t *line)
{
	polyrotdata_t prd;

	prd.polyObjNum = line->tag;
	prd.speed      = sides[line->sidenum[0]].textureoffset >> FRACBITS; // angular speed
	prd.distance   = sides[line->sidenum[0]].rowoffset >> FRACBITS; // angular distance

	// Polyobj_(OR_)RotateRight have dir == -1
	prd.direction = (line->special == 484 || line->special == 485) ? -1 : 1;

	// Polyobj_OR types have override set to true
	prd.overRide  = (line->special == 485 || line->special == 487);

	if (line->flags & ML_NOCLIMB)
		prd.turnobjs = 0;
	else if (line->flags & ML_EFFECT4)
		prd.turnobjs = 2;
	else
		prd.turnobjs = 1;

	return EV_DoPolyObjRotate(&prd);
}

//
// PolyDisplace
//
// Parses arguments for parameterized polyobject move-by-sector-heights specials
//
static boolean PolyDisplace(line_t *line)
{
	polydisplacedata_t pdd;

	pdd.polyObjNum = line->tag;

	pdd.controlSector = line->frontsector;
	pdd.dx = line->dx>>8;
	pdd.dy = line->dy>>8;

	return EV_DoPolyObjDisplace(&pdd);
}

#endif // ifdef POLYOBJECTS

/** Changes a sector's tag.
  * Used by the linedef executor tag changer and by crumblers.
  *
  * \param sector Sector whose tag will be changed.
  * \param newtag New tag number for this sector.
  * \sa P_InitTagLists, P_FindSectorFromTag
  * \author Graue <graue@oceanbase.org>
  */
void P_ChangeSectorTag(UINT32 sector, INT16 newtag)
{
	INT16 oldtag;
	INT32 i;

	I_Assert(sector < numsectors);

	if ((oldtag = sectors[sector].tag) == newtag)
		return;

	// first you have to remove it from the old tag's taglist
	i = sectors[(unsigned)oldtag % numsectors].firsttag;

	if (i == -1) // shouldn't happen
		I_Error("Corrupt tag list for sector %u\n", sector);
	else if ((UINT32)i == sector)
		sectors[(unsigned)oldtag % numsectors].firsttag = sectors[sector].nexttag;
	else
	{
		while (sectors[i].nexttag != -1 && (UINT32)sectors[i].nexttag < sector )
			i = sectors[i].nexttag;

		sectors[i].nexttag = sectors[sector].nexttag;
	}

	sectors[sector].tag = newtag;

	// now add it to the new tag's taglist
	if ((UINT32)sectors[(unsigned)newtag % numsectors].firsttag > sector)
	{
		sectors[sector].nexttag = sectors[(unsigned)newtag % numsectors].firsttag;
		sectors[(unsigned)newtag % numsectors].firsttag = sector;
	}
	else
	{
		i = sectors[(unsigned)newtag % numsectors].firsttag;

		if (i == -1)
		{
			sectors[(unsigned)newtag % numsectors].firsttag = sector;
			sectors[sector].nexttag = -1;
		}
		else
		{
			while (sectors[i].nexttag != -1 && (UINT32)sectors[i].nexttag < sector )
				i = sectors[i].nexttag;

			sectors[sector].nexttag = sectors[i].nexttag;
			sectors[i].nexttag = sector;
		}
	}
}

/** Hashes the sector tags across the sectors and linedefs.
  *
  * \sa P_FindSectorFromTag, P_ChangeSectorTag
  * \author Lee Killough
  */
static inline void P_InitTagLists(void)
{
	register size_t i;

	for (i = numsectors - 1; i != (size_t)-1; i--)
	{
		size_t j = (unsigned)sectors[i].tag % numsectors;
		sectors[i].nexttag = sectors[j].firsttag;
		sectors[j].firsttag = (INT32)i;
		sectors[i].spawn_nexttag = sectors[i].nexttag;
		sectors[j].spawn_firsttag = sectors[j].firsttag;
	}

	for (i = numlines - 1; i != (size_t)-1; i--)
	{
		size_t j = (unsigned)lines[i].tag % numlines;
		lines[i].nexttag = lines[j].firsttag;
		lines[j].firsttag = (INT32)i;
	}
}

/** Finds minimum light from an adjacent sector.
  *
  * \param sector Sector to start in.
  * \param max    Maximum value to return.
  * \return Minimum light value from an adjacent sector, or max if the minimum
  *         light value is greater than max.
  */
INT32 P_FindMinSurroundingLight(sector_t *sector, INT32 max)
{
	size_t i;
	INT32 min = max;
	line_t *line;
	sector_t *check;

	for (i = 0; i < sector->linecount; i++)
	{
		line = sector->lines[i];
		check = getNextSector(line,sector);

		if (!check)
			continue;

		if (check->lightlevel < min)
			min = check->lightlevel;
	}
	return min;
}

void T_ExecutorDelay(executor_t *e)
{
	if (--e->timer <= 0)
	{
		if (e->caller && P_MobjWasRemoved(e->caller)) // If the mobj died while we were delaying
			P_SetTarget(&e->caller, NULL); // Call with no mobj!
		P_ProcessLineSpecial(e->line, e->caller, e->sector);
		P_SetTarget(&e->caller, NULL); // Let the mobj know it can be removed now.
		P_RemoveThinker(&e->thinker);
	}
}

static void P_AddExecutorDelay(line_t *line, mobj_t *mobj, sector_t *sector)
{
	executor_t *e;

	if (!line->backsector)
		I_Error("P_AddExecutorDelay: Line has no backsector!\n");

	e = Z_Calloc(sizeof (*e), PU_LEVSPEC, NULL);

	e->thinker.function.acp1 = (actionf_p1)T_ExecutorDelay;
	e->line = line;
	e->sector = sector;
	e->timer = (line->backsector->ceilingheight>>FRACBITS)+(line->backsector->floorheight>>FRACBITS);
	P_SetTarget(&e->caller, mobj); // Use P_SetTarget to make sure the mobj doesn't get freed while we're delaying.
	P_AddThinker(&e->thinker);
}

/** Used by P_LinedefExecute to check a trigger linedef's conditions
  * The linedef executor specials in the trigger linedef's sector are run if all conditions are met.
  * Return false cancels P_LinedefExecute, this happens if a condition is not met.
  *
  * \param triggerline Trigger linedef to check conditions for; should NEVER be NULL.
  * \param actor Object initiating the action; should not be NULL.
  * \param caller Sector in which the action was started. May be NULL.
  * \sa P_ProcessLineSpecial, P_LinedefExecute
  */
boolean P_RunTriggerLinedef(line_t *triggerline, mobj_t *actor, sector_t *caller)
{
	sector_t *ctlsector;
	fixed_t dist = P_AproxDistance(triggerline->dx, triggerline->dy)>>FRACBITS;
	size_t i, linecnt, sectori;
	INT16 specialtype = triggerline->special;

	/////////////////////////////////////////////////
	// Distance-checking/sector trigger conditions //
	/////////////////////////////////////////////////

	// Linetypes 303 and 304 require a specific
	// number, or minimum or maximum, of rings.
	if (specialtype == 303 || specialtype == 304)
	{
		fixed_t rings = 0;

		// With the passuse flag, count all player's
		// rings.
		if (triggerline->flags & ML_EFFECT4)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i] || players[i].spectator)
					continue;

				if (!players[i].mo || players[i].mo->health < 1)
					continue;

				rings += players[i].mo->health-1;
			}
		}
		else
		{
			if (!(actor && actor->player))
				return false; // no player to count rings from here, sorry

			rings = actor->health-1;
		}

		if (triggerline->flags & ML_NOCLIMB)
		{
			if (rings > dist)
				return false;
		}
		else if (triggerline->flags & ML_BLOCKMONSTERS)
		{
			if (rings < dist)
				return false;
		}
		else
		{
			if (rings != dist)
				return false;
		}
	}
	else if (specialtype >= 314 && specialtype <= 315)
	{
		msecnode_t *node;
		mobj_t *mo;
		INT32 numpush = 0;
		INT32 numneeded = dist;

		if (!caller)
			return false; // we need a calling sector to find pushables in, silly!

		// Count the pushables in this sector
		node = caller->touching_thinglist; // things touching this sector
		while (node)
		{
			mo = node->m_thing;
			if (mo->flags & MF_PUSHABLE)
				numpush++;
			node = node->m_thinglist_next;
		}

		if (triggerline->flags & ML_NOCLIMB) // Need at least or more
		{
			if (numpush < numneeded)
				return false;
		}
		else if (triggerline->flags & ML_EFFECT4) // Need less than
		{
			if (numpush >= numneeded)
				return false;
		}
		else // Need exact
		{
			if (numpush != numneeded)
				return false;
		}
	}
	else if (caller)
	{
		if (GETSECSPECIAL(caller->special, 2) == 6)
		{
			if (!(ALL7EMERALDS(emeralds)))
				return false;
		}
		else if (GETSECSPECIAL(caller->special, 2) == 7)
		{
			UINT8 mare;

			if (!(maptol & TOL_NIGHTS))
				return false;

			mare = P_FindLowestMare();

			if (triggerline->flags & ML_NOCLIMB)
			{
				if (!(mare <= dist))
					return false;
			}
			else if (triggerline->flags & ML_BLOCKMONSTERS)
			{
				if (!(mare >= dist))
					return false;
			}
			else
			{
				if (!(mare == dist))
					return false;
			}
		}
		// If we were not triggered by a sector type especially for the purpose,
		// a Linedef Executor linedef trigger is not handling sector triggers properly, return.

		else if ((!GETSECSPECIAL(caller->special, 2) || GETSECSPECIAL(caller->special, 2) > 7) && (specialtype > 322))
		{
			CONS_Alert(CONS_WARNING,
				M_GetText("Linedef executor trigger isn't handling sector triggers properly!\nspecialtype = %d, if you are not a dev, report this warning instance\nalong with the wad that caused it!\n"),
				specialtype);
			return false;
		}
	}

	//////////////////////////////////////
	// Miscellaneous trigger conditions //
	//////////////////////////////////////

	switch (specialtype)
	{
		case 305: // continuous
		case 306: // each time
		case 307: // once
			if (!(actor && actor->player && actor->player->charability == dist/10))
				return false;
			break;
		case 309: // continuous
		case 310: // each time
			// Only red team members can activate this.
			if (!(actor && actor->player && actor->player->ctfteam == 1))
				return false;
			break;
		case 311: // continuous
		case 312: // each time
			// Only blue team members can activate this.
			if (!(actor && actor->player && actor->player->ctfteam == 2))
				return false;
			break;
		case 317: // continuous
		case 318: // once
			{ // Unlockable triggers required
				INT32 trigid = (INT32)(sides[triggerline->sidenum[0]].textureoffset>>FRACBITS);

				if ((modifiedgame && !savemoddata) || (netgame || multiplayer))
					return false;
				else if (trigid < 0 || trigid > 31) // limited by 32 bit variable
				{
					CONS_Debug(DBG_GAMELOGIC, "Unlockable trigger (sidedef %hu): bad trigger ID %d\n", triggerline->sidenum[0], trigid);
					return false;
				}
				else if (!(unlocktriggers & (1 << trigid)))
					return false;
			}
			break;
		case 319: // continuous
		case 320: // once
			{ // An unlockable itself must be unlocked!
				INT32 unlockid = (INT32)(sides[triggerline->sidenum[0]].textureoffset>>FRACBITS);

				if ((modifiedgame && !savemoddata) || (netgame || multiplayer))
					return false;
				else if (unlockid < 0 || unlockid >= MAXUNLOCKABLES) // limited by unlockable count
				{
					CONS_Debug(DBG_GAMELOGIC, "Unlockable check (sidedef %hu): bad unlockable ID %d\n", triggerline->sidenum[0], unlockid);
					return false;
				}
				else if (!(unlockables[unlockid-1].unlocked))
					return false;
			}
			break;
		case 321: // continuous
		case 322: // each time
			// decrement calls left before triggering
			if (triggerline->callcount > 0)
			{
				if (--triggerline->callcount > 0)
					return false;
			}
			break;
		default:
			break;
	}

	/////////////////////////////////
	// Processing linedef specials //
	/////////////////////////////////

	ctlsector = triggerline->frontsector;
	sectori = (size_t)(ctlsector - sectors);
	linecnt = ctlsector->linecount;

	if (triggerline->flags & ML_EFFECT5) // disregard order for efficiency
	{
		for (i = 0; i < linecnt; i++)
			if (ctlsector->lines[i]->special >= 400
				&& ctlsector->lines[i]->special < 500)
			{
				if (ctlsector->lines[i]->flags & ML_DONTPEGTOP)
					P_AddExecutorDelay(ctlsector->lines[i], actor, caller);
				else
					P_ProcessLineSpecial(ctlsector->lines[i], actor, caller);
			}
	}
	else // walk around the sector in a defined order
	{
		boolean backwards = false;
		size_t j, masterlineindex = (size_t)-1;

		for (i = 0; i < linecnt; i++)
			if (ctlsector->lines[i] == triggerline)
			{
				masterlineindex = i;
				break;
			}

#ifdef PARANOIA
		if (masterlineindex == (size_t)-1)
		{
			const size_t li = (size_t)(ctlsector->lines[i] - lines);
			I_Error("Line %s isn't linked into its front sector", sizeu1(li));
		}
#endif

		// i == masterlineindex
		for (;;)
		{
			if (backwards) // v2 to v1
			{
				for (j = 0; j < linecnt; j++)
				{
					if (i == j)
						continue;
					if (ctlsector->lines[i]->v1 == ctlsector->lines[j]->v2)
					{
						i = j;
						break;
					}
					if (ctlsector->lines[i]->v1 == ctlsector->lines[j]->v1)
					{
						i = j;
						backwards = false;
						break;
					}
				}
				if (j == linecnt)
				{
					const size_t vertexei = (size_t)(ctlsector->lines[i]->v1 - vertexes);
					CONS_Debug(DBG_GAMELOGIC, "Warning: Sector %s is not closed at vertex %s (%d, %d)\n",
						sizeu1(sectori), sizeu2(vertexei), ctlsector->lines[i]->v1->x, ctlsector->lines[i]->v1->y);
					return false; // abort
				}
			}
			else // v1 to v2
			{
				for (j = 0; j < linecnt; j++)
				{
					if (i == j)
						continue;
					if (ctlsector->lines[i]->v2 == ctlsector->lines[j]->v1)
					{
						i = j;
						break;
					}
					if (ctlsector->lines[i]->v2 == ctlsector->lines[j]->v2)
					{
						i = j;
						backwards = true;
						break;
					}
				}
				if (j == linecnt)
				{
					const size_t vertexei = (size_t)(ctlsector->lines[i]->v1 - vertexes);
					CONS_Debug(DBG_GAMELOGIC, "Warning: Sector %s is not closed at vertex %s (%d, %d)\n",
						sizeu1(sectori), sizeu2(vertexei), ctlsector->lines[i]->v2->x, ctlsector->lines[i]->v2->y);
					return false; // abort
				}
			}

			if (i == masterlineindex)
				break;

			if (ctlsector->lines[i]->special >= 400
				&& ctlsector->lines[i]->special < 500)
			{
				if (ctlsector->lines[i]->flags & ML_DONTPEGTOP)
					P_AddExecutorDelay(ctlsector->lines[i], actor, caller);
				else
					P_ProcessLineSpecial(ctlsector->lines[i], actor, caller);
			}
		}
	}

	// "Trigger on X calls" linedefs reset if noclimb is set
	if ((specialtype == 321 || specialtype == 322) && triggerline->flags & ML_NOCLIMB)
		triggerline->callcount = sides[triggerline->sidenum[0]].textureoffset>>FRACBITS;
	else
	// These special types work only once
	if (specialtype == 302  // Once
	 || specialtype == 304  // Ring count - Once
	 || specialtype == 307  // Character ability - Once
	 || specialtype == 308  // Race only - Once
	 || specialtype == 315  // No of pushables - Once
	 || specialtype == 318  // Unlockable trigger - Once
	 || specialtype == 320  // Unlockable - Once
	 || specialtype == 321 || specialtype == 322 // Trigger on X calls - Continuous + Each Time
	 || specialtype == 399) // Level Load
		triggerline->special = 0; // Clear it out

	return true;
}

/** Runs a linedef executor.
  * Can be called by:
  *   - a player moving into a special sector or FOF.
  *   - a pushable object moving into a special sector or FOF.
  *   - a ceiling or floor movement from a previous linedef executor finishing.
  *   - any object in a state with the A_LinedefExecute() action.
  *
  * \param tag Tag of the linedef executor to run.
  * \param actor Object initiating the action; should not be NULL.
  * \param caller Sector in which the action was started. May be NULL.
  * \sa P_ProcessLineSpecial, P_RunTriggerLinedef
  * \author Graue <graue@oceanbase.org>
  */
void P_LinedefExecute(INT16 tag, mobj_t *actor, sector_t *caller)
{
	size_t masterline;

	CONS_Debug(DBG_GAMELOGIC, "P_LinedefExecute: Executing trigger linedefs of tag %d\n", tag);

	I_Assert(!actor || !P_MobjWasRemoved(actor)); // If actor is there, it must be valid.

	for (masterline = 0; masterline < numlines; masterline++)
	{
		if (lines[masterline].tag != tag)
			continue;

		// "No More Enemies" and "Level Load" take care of themselves.
		if (lines[masterline].special == 313
		 || lines[masterline].special == 399
		 // Each-time executors handle themselves, too
		 || lines[masterline].special == 301 // Each time
		 || lines[masterline].special == 306 // Character ability - Each time
		 || lines[masterline].special == 310 // CTF Red team - Each time
		 || lines[masterline].special == 312 // CTF Blue team - Each time
		 || lines[masterline].special == 322) // Trigger on X calls - Each Time
			continue;

		if (lines[masterline].special < 300
			|| lines[masterline].special > 399)
			continue;

		if (!P_RunTriggerLinedef(&lines[masterline], actor, caller))
			return; // cancel P_LinedefExecute if function returns false
	}
}

//
// P_SwitchWeather
//
// Switches the weather!
//
void P_SwitchWeather(INT32 weathernum)
{
	boolean purge = false;
	INT32 swap = 0;

	switch (weathernum)
	{
		case PRECIP_NONE: // None
			if (curWeather == PRECIP_NONE)
				return; // Nothing to do.
			purge = true;
			break;
		case PRECIP_STORM: // Storm
		case PRECIP_STORM_NOSTRIKES: // Storm w/ no lightning
		case PRECIP_RAIN: // Rain
			if (curWeather == PRECIP_SNOW || curWeather == PRECIP_BLANK || curWeather == PRECIP_STORM_NORAIN)
				swap = PRECIP_RAIN;
			break;
		case PRECIP_SNOW: // Snow
			if (curWeather == PRECIP_SNOW)
				return; // Nothing to do.
			if (curWeather == PRECIP_RAIN || curWeather == PRECIP_STORM || curWeather == PRECIP_STORM_NOSTRIKES || curWeather == PRECIP_BLANK || curWeather == PRECIP_STORM_NORAIN)
				swap = PRECIP_SNOW; // Need to delete the other precips.
			break;
		case PRECIP_STORM_NORAIN: // Storm w/o rain
			if (curWeather == PRECIP_SNOW
				|| curWeather == PRECIP_STORM
				|| curWeather == PRECIP_STORM_NOSTRIKES
				|| curWeather == PRECIP_RAIN
				|| curWeather == PRECIP_BLANK)
				swap = PRECIP_STORM_NORAIN;
			else if (curWeather == PRECIP_STORM_NORAIN)
				return;
			break;
		case PRECIP_BLANK:
			if (curWeather == PRECIP_SNOW
				|| curWeather == PRECIP_STORM
				|| curWeather == PRECIP_STORM_NOSTRIKES
				|| curWeather == PRECIP_RAIN)
				swap = PRECIP_BLANK;
			else if (curWeather == PRECIP_STORM_NORAIN)
				swap = PRECIP_BLANK;
			else if (curWeather == PRECIP_BLANK)
				return;
			break;
		default:
			CONS_Debug(DBG_GAMELOGIC, "P_SwitchWeather: Unknown weather type %d.\n", weathernum);
			break;
	}

	if (purge)
	{
		thinker_t *think;
		precipmobj_t *precipmobj;

		for (think = thinkercap.next; think != &thinkercap; think = think->next)
		{
			if ((think->function.acp1 != (actionf_p1)P_SnowThinker)
				&& (think->function.acp1 != (actionf_p1)P_RainThinker))
				continue; // not a precipmobj thinker

			precipmobj = (precipmobj_t *)think;

			P_RemovePrecipMobj(precipmobj);
		}
	}
	else if (swap && !((swap == PRECIP_BLANK && curWeather == PRECIP_STORM_NORAIN) || (swap == PRECIP_STORM_NORAIN && curWeather == PRECIP_BLANK))) // Rather than respawn all that crap, reuse it!
	{
		thinker_t *think;
		precipmobj_t *precipmobj;
		state_t *st;

		for (think = thinkercap.next; think != &thinkercap; think = think->next)
		{
			if (swap == PRECIP_RAIN) // Snow To Rain
			{
				if (!(think->function.acp1 == (actionf_p1)P_SnowThinker
					|| think->function.acp1 == (actionf_p1)P_NullPrecipThinker))
					continue; // not a precipmobj thinker

				precipmobj = (precipmobj_t *)think;

				precipmobj->flags = mobjinfo[MT_RAIN].flags;
				st = &states[mobjinfo[MT_RAIN].spawnstate];
				precipmobj->state = st;
				precipmobj->tics = st->tics;
				precipmobj->sprite = st->sprite;
				precipmobj->frame = st->frame;
				precipmobj->momz = mobjinfo[MT_RAIN].speed;

				precipmobj->precipflags &= ~PCF_INVISIBLE;

				think->function.acp1 = (actionf_p1)P_RainThinker;
			}
			else if (swap == PRECIP_SNOW) // Rain To Snow
			{
				INT32 z;

				if (!(think->function.acp1 == (actionf_p1)P_RainThinker
					|| think->function.acp1 == (actionf_p1)P_NullPrecipThinker))
					continue; // not a precipmobj thinker

				precipmobj = (precipmobj_t *)think;

				precipmobj->flags = mobjinfo[MT_SNOWFLAKE].flags;
				z = M_RandomByte();

				if (z < 64)
					z = 2;
				else if (z < 144)
					z = 1;
				else
					z = 0;

				st = &states[mobjinfo[MT_SNOWFLAKE].spawnstate+z];
				precipmobj->state = st;
				precipmobj->tics = st->tics;
				precipmobj->sprite = st->sprite;
				precipmobj->frame = st->frame;
				precipmobj->momz = mobjinfo[MT_SNOWFLAKE].speed;

				precipmobj->precipflags &= ~PCF_INVISIBLE;

				think->function.acp1 = (actionf_p1)P_SnowThinker;
			}
			else if (swap == PRECIP_BLANK || swap == PRECIP_STORM_NORAIN) // Remove precip, but keep it around for reuse.
			{
				if (!(think->function.acp1 == (actionf_p1)P_RainThinker
					|| think->function.acp1 == (actionf_p1)P_SnowThinker))
					continue;

				precipmobj = (precipmobj_t *)think;

				think->function.acp1 = (actionf_p1)P_NullPrecipThinker;

				precipmobj->precipflags |= PCF_INVISIBLE;
			}
		}
	}

	switch (weathernum)
	{
		case PRECIP_SNOW: // snow
			curWeather = PRECIP_SNOW;

			if (!swap)
				P_SpawnPrecipitation();

			break;
		case PRECIP_RAIN: // rain
		{
			boolean dontspawn = false;

			if (curWeather == PRECIP_RAIN || curWeather == PRECIP_STORM || curWeather == PRECIP_STORM_NOSTRIKES)
				dontspawn = true;

			curWeather = PRECIP_RAIN;

			if (!dontspawn && !swap)
				P_SpawnPrecipitation();

			break;
		}
		case PRECIP_STORM: // storm
		{
			boolean dontspawn = false;

			if (curWeather == PRECIP_RAIN || curWeather == PRECIP_STORM || curWeather == PRECIP_STORM_NOSTRIKES)
				dontspawn = true;

			curWeather = PRECIP_STORM;

			if (!dontspawn && !swap)
				P_SpawnPrecipitation();

			break;
		}
		case PRECIP_STORM_NOSTRIKES: // storm w/o lightning
		{
			boolean dontspawn = false;

			if (curWeather == PRECIP_RAIN || curWeather == PRECIP_STORM || curWeather == PRECIP_STORM_NOSTRIKES)
				dontspawn = true;

			curWeather = PRECIP_STORM_NOSTRIKES;

			if (!dontspawn && !swap)
				P_SpawnPrecipitation();

			break;
		}
		case PRECIP_STORM_NORAIN: // storm w/o rain
			curWeather = PRECIP_STORM_NORAIN;

			if (!swap)
				P_SpawnPrecipitation();

			break;
		case PRECIP_BLANK:
			curWeather = PRECIP_BLANK;

			if (!swap)
				P_SpawnPrecipitation();

			break;
		default:
			curWeather = PRECIP_NONE;
			break;
	}
}

/** Gets an object.
  *
  * \param type Object type to look for.
  * \param s Sector number to look in.
  * \return Pointer to the first ::type found in the sector.
  * \sa P_GetPushThing
  */
static mobj_t *P_GetObjectTypeInSectorNum(mobjtype_t type, size_t s)
{
	sector_t *sec = sectors + s;
	mobj_t *thing = sec->thinglist;

	while (thing)
	{
		if (thing->type == type)
			return thing;
		thing = thing->snext;
	}
	return NULL;
}

/** Processes the line special triggered by an object.
  *
  * \param line Line with the special command on it.
  * \param mo   mobj that triggered the line. Must be valid and non-NULL.
  * \param callsec sector in which action was initiated; this can be NULL.
  *        Because of the A_LinedefExecute() action, even if non-NULL,
  *        this sector might not have the same tag as the linedef executor,
  *        and it might not have the linedef executor sector type.
  * \todo Handle mo being NULL gracefully. T_MoveFloor() and T_MoveCeiling()
  *       don't have an object to pass.
  * \todo Split up into multiple functions.
  * \sa P_LinedefExecute
  * \author Graue <graue@oceanbase.org>
  */
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec)
{
	INT32 secnum = -1;
	mobj_t *bot = NULL;

	I_Assert(!mo || !P_MobjWasRemoved(mo)); // If mo is there, mo must be valid!

	if (mo && mo->player && botingame)
		bot = players[secondarydisplayplayer].mo;

	// note: only commands with linedef types >= 400 && < 500 can be used
	switch (line->special)
	{
		case 400: // Set tagged sector's floor height/pic
			EV_DoFloor(line, instantMoveFloorByFrontSector);
			break;

		case 401: // Set tagged sector's ceiling height/pic
			EV_DoCeiling(line, instantMoveCeilingByFrontSector);
			break;

		case 402: // Set tagged sector's light level
			{
				INT16 newlightlevel;
				INT32 newfloorlightsec, newceilinglightsec;

				newlightlevel = line->frontsector->lightlevel;
				newfloorlightsec = line->frontsector->floorlightsec;
				newceilinglightsec = line->frontsector->ceilinglightsec;

				// act on all sectors with the same tag as the triggering linedef
				while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
				{
					if (sectors[secnum].lightingdata)
					{
						// Stop the lighting madness going on in this sector!
						P_RemoveThinker(&((elevator_t *)sectors[secnum].lightingdata)->thinker);
						sectors[secnum].lightingdata = NULL;

						// No, it's not an elevator_t, but any struct with a thinker_t named
						// 'thinker' at the beginning will do here. (We don't know what it
						// actually is: could be lightlevel_t, fireflicker_t, glow_t, etc.)
					}

					sectors[secnum].lightlevel = newlightlevel;
					sectors[secnum].floorlightsec = newfloorlightsec;
					sectors[secnum].ceilinglightsec = newceilinglightsec;
				}
			}
			break;

		case 403: // Move floor, linelen = speed, frontsector floor = dest height
			EV_DoFloor(line, moveFloorByFrontSector);
			break;

		case 404: // Move ceiling, linelen = speed, frontsector ceiling = dest height
			EV_DoCeiling(line, moveCeilingByFrontSector);
			break;

		case 405: // Move floor by front side texture offsets, offset x = speed, offset y = amount to raise/lower
			EV_DoFloor(line, moveFloorByFrontTexture);
			break;

		case 407: // Move ceiling by front side texture offsets, offset x = speed, offset y = amount to raise/lower
			EV_DoCeiling(line, moveCeilingByFrontTexture);
			break;

/*		case 405: // Lower floor by line, dx = speed, dy = amount to lower
			EV_DoFloor(line, lowerFloorByLine);
			break;

		case 406: // Raise floor by line, dx = speed, dy = amount to raise
			EV_DoFloor(line, raiseFloorByLine);
			break;

		case 407: // Lower ceiling by line, dx = speed, dy = amount to lower
			EV_DoCeiling(line, lowerCeilingByLine);
			break;

		case 408: // Raise ceiling by line, dx = speed, dy = amount to raise
			EV_DoCeiling(line, raiseCeilingByLine);
			break;*/

		case 409: // Change tagged sectors' tag
		// (formerly "Change calling sectors' tag", but behavior was changed)
		{
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
				P_ChangeSectorTag(secnum,(INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS));
			break;
		}

		case 410: // Change front sector's tag
			P_ChangeSectorTag((UINT32)(line->frontsector - sectors), (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS));
			break;

		case 411: // Stop floor/ceiling movement in tagged sector(s)
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if (sectors[secnum].floordata)
				{
					if (sectors[secnum].floordata == sectors[secnum].ceilingdata) // elevator
					{
						P_RemoveThinker(&((elevator_t *)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = sectors[secnum].ceilingdata = NULL;
						sectors[secnum].floorspeed = sectors[secnum].ceilspeed = 0;
					}
					else // floormove
					{
						P_RemoveThinker(&((floormove_t *)sectors[secnum].floordata)->thinker);
						sectors[secnum].floordata = NULL;
						sectors[secnum].floorspeed = 0;
					}
				}

				if (sectors[secnum].ceilingdata) // ceiling
				{
					P_RemoveThinker(&((ceiling_t *)sectors[secnum].ceilingdata)->thinker);
					sectors[secnum].ceilingdata = NULL;
					sectors[secnum].ceilspeed = 0;
				}
			}
			break;

		case 412: // Teleport the player or thing
			{
				mobj_t *dest;

				if (!mo) // nothing to teleport
					return;

				if (line->flags & ML_EFFECT3) // Relative silent teleport
				{
					fixed_t x, y, z;

					x = sides[line->sidenum[0]].textureoffset;
					y = sides[line->sidenum[0]].rowoffset;
					z = line->frontsector->ceilingheight;

					P_UnsetThingPosition(mo);
					mo->x += x;
					mo->y += y;
					mo->z += z;
					P_SetThingPosition(mo);

					if (mo->player)
					{
						if (bot) // This might put poor Tails in a wall if he's too far behind! D: But okay, whatever! >:3
							P_TeleportMove(bot, bot->x + x, bot->y + y, bot->z + z);
						if (splitscreen && mo->player == &players[secondarydisplayplayer] && camera2.chase)
						{
							camera2.x += x;
							camera2.y += y;
							camera2.z += z;
							camera2.subsector = R_PointInSubsector(camera2.x, camera2.y);
						}
						else if (camera.chase && mo->player == &players[displayplayer])
						{
							camera.x += x;
							camera.y += y;
							camera.z += z;
							camera.subsector = R_PointInSubsector(camera.x, camera.y);
						}
					}
				}
				else
				{
					if ((secnum = P_FindSectorFromLineTag(line, -1)) < 0)
						return;

					dest = P_GetObjectTypeInSectorNum(MT_TELEPORTMAN, secnum);
					if (!dest)
						return;

					if (bot)
						P_Teleport(bot, dest->x, dest->y, dest->z, (line->flags & ML_NOCLIMB) ?  mo->angle : dest->angle, (line->flags & ML_BLOCKMONSTERS) == 0, (line->flags & ML_EFFECT4) == ML_EFFECT4);
					if (line->flags & ML_BLOCKMONSTERS)
						P_Teleport(mo, dest->x, dest->y, dest->z, (line->flags & ML_NOCLIMB) ?  mo->angle : dest->angle, false, (line->flags & ML_EFFECT4) == ML_EFFECT4);
					else
					{
						P_Teleport(mo, dest->x, dest->y, dest->z, (line->flags & ML_NOCLIMB) ?  mo->angle : dest->angle, true, (line->flags & ML_EFFECT4) == ML_EFFECT4);
						// Play the 'bowrwoosh!' sound
						S_StartSound(dest, sfx_mixup);
					}
				}
			}
			break;

		case 413: // Change music
			// console player only unless NOCLIMB is set
			if ((line->flags & ML_NOCLIMB) || (mo && mo->player && P_IsLocalPlayer(mo->player)))
			{
				UINT16 tracknum = (UINT16)sides[line->sidenum[0]].bottomtexture;

				strncpy(mapmusname, sides[line->sidenum[0]].text, 7);
				mapmusname[6] = 0;

				mapmusflags = tracknum & MUSIC_TRACKMASK;
				if (!(line->flags & ML_BLOCKMONSTERS))
					mapmusflags |= MUSIC_RELOADRESET;

				S_ChangeMusic(mapmusname, mapmusflags, !(line->flags & ML_EFFECT4));

				// Except, you can use the ML_BLOCKMONSTERS flag to change this behavior.
				// if (mapmusflags & MUSIC_RELOADRESET) then it will reset the music in G_PlayerReborn.
			}
			break;

		case 414: // Play SFX
			{
				INT32 sfxnum;

				sfxnum = sides[line->sidenum[0]].toptexture; //P_AproxDistance(line->dx, line->dy)>>FRACBITS;

				if (sfxnum == sfx_None)
					return; // Do nothing!
				if (sfxnum < sfx_None || sfxnum >= NUMSFX)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 414 Executor: sfx number %d is invalid!\n", sfxnum);
					return;
				}
				if (line->tag != 0) // Do special stuff only if a non-zero linedef tag is set
				{
					if (line->flags & ML_EFFECT5) // Repeat Midtexture
					{
						// Additionally play the sound from tagged sectors' soundorgs
						sector_t *sec;

						while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
						{
							sec = &sectors[secnum];
							S_StartSound(&sec->soundorg, sfxnum);
						}
					}
					else if (mo) // A mobj must have triggered the executor
					{
						// Only trigger if mobj is touching the tag
						ffloor_t *rover;
						boolean foundit = false;

						for(rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
						{
							if (rover->master->frontsector->tag != line->tag)
								continue;

							if (mo->z > P_GetSpecialTopZ(mo, sectors + rover->secnum, mo->subsector->sector))
								continue;

							if (mo->z + mo->height < P_GetSpecialBottomZ(mo, sectors + rover->secnum, mo->subsector->sector))
								continue;

							foundit = true;
						}

						if (mo->subsector->sector->tag == line->tag)
							foundit = true;

						if (!foundit)
							return;
					}
				}

				if (line->flags & ML_NOCLIMB)
				{
					// play the sound from nowhere, but only if display player triggered it
					if (mo && mo->player && (mo->player == &players[displayplayer] || mo->player == &players[secondarydisplayplayer]))
						S_StartSound(NULL, sfxnum);
				}
				else if (line->flags & ML_EFFECT4)
				{
					// play the sound from nowhere
					S_StartSound(NULL, sfxnum);
				}
				else if (line->flags & ML_BLOCKMONSTERS)
				{
					// play the sound from calling sector's soundorg
					if (callsec)
						S_StartSound(&callsec->soundorg, sfxnum);
					else if (mo)
						S_StartSound(&mo->subsector->sector->soundorg, sfxnum);
				}
				else if (mo)
				{
					// play the sound from mobj that triggered it
					S_StartSound(mo, sfxnum);
				}
			}
			break;

		case 415: // Run a script
			if (cv_runscripts.value)
			{
				INT32 scrnum;
				lumpnum_t lumpnum;
				char newname[9];

				strcpy(newname, G_BuildMapName(gamemap));
				newname[0] = 'S';
				newname[1] = 'C';
				newname[2] = 'R';

				scrnum = sides[line->sidenum[0]].textureoffset>>FRACBITS;
				if (scrnum < 0 || scrnum > 999)
				{
					scrnum = 0;
					newname[5] = newname[6] = newname[7] = '0';
				}
				else
				{
					newname[5] = (char)('0' + (char)((scrnum/100)));
					newname[6] = (char)('0' + (char)((scrnum%100)/10));
					newname[7] = (char)('0' + (char)(scrnum%10));
				}
				newname[8] = '\0';

				lumpnum = W_CheckNumForName(newname);

				if (lumpnum == LUMPERROR || W_LumpLength(lumpnum) == 0)
				{
					CONS_Debug(DBG_SETUP, "SOC Error: script lump %s not found/not valid.\n", newname);
				}
				else
					COM_BufInsertText(W_CacheLumpNum(lumpnum, PU_CACHE));
			}
			break;

		case 416: // Spawn adjustable fire flicker
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if (line->flags & ML_NOCLIMB && line->backsector)
				{
					// Use front sector for min light level, back sector for max.
					// This is tricky because P_SpawnAdjustableFireFlicker expects
					// the maxsector (second argument) to also be the target
					// sector, so we have to do some light level twiddling.
					fireflicker_t *flick;
					INT16 reallightlevel = sectors[secnum].lightlevel;
					sectors[secnum].lightlevel = line->backsector->lightlevel;

					flick = P_SpawnAdjustableFireFlicker(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);

					// Make sure the starting light level is in range.
					if (reallightlevel < flick->minlight)
						reallightlevel = (INT16)flick->minlight;
					else if (reallightlevel > flick->maxlight)
						reallightlevel = (INT16)flick->maxlight;

					sectors[secnum].lightlevel = reallightlevel;
				}
				else
				{
					// Use front sector for min, target sector for max,
					// the same way linetype 61 does it.
					P_SpawnAdjustableFireFlicker(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);
				}
			}
			break;

		case 417: // Spawn adjustable glowing light
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if (line->flags & ML_NOCLIMB && line->backsector)
				{
					// Use front sector for min light level, back sector for max.
					// This is tricky because P_SpawnAdjustableGlowingLight expects
					// the maxsector (second argument) to also be the target
					// sector, so we have to do some light level twiddling.
					glow_t *glow;
					INT16 reallightlevel = sectors[secnum].lightlevel;
					sectors[secnum].lightlevel = line->backsector->lightlevel;

					glow = P_SpawnAdjustableGlowingLight(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);

					// Make sure the starting light level is in range.
					if (reallightlevel < glow->minlight)
						reallightlevel = (INT16)glow->minlight;
					else if (reallightlevel > glow->maxlight)
						reallightlevel = (INT16)glow->maxlight;

					sectors[secnum].lightlevel = reallightlevel;
				}
				else
				{
					// Use front sector for min, target sector for max,
					// the same way linetype 602 does it.
					P_SpawnAdjustableGlowingLight(line->frontsector, &sectors[secnum],
						P_AproxDistance(line->dx, line->dy)>>FRACBITS);
				}
			}
			break;

		case 418: // Spawn adjustable strobe flash (unsynchronized)
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if (line->flags & ML_NOCLIMB && line->backsector)
				{
					// Use front sector for min light level, back sector for max.
					// This is tricky because P_SpawnAdjustableGlowingLight expects
					// the maxsector (second argument) to also be the target
					// sector, so we have to do some light level twiddling.
					strobe_t *flash;
					INT16 reallightlevel = sectors[secnum].lightlevel;
					sectors[secnum].lightlevel = line->backsector->lightlevel;

					flash = P_SpawnAdjustableStrobeFlash(line->frontsector, &sectors[secnum],
						abs(line->dx)>>FRACBITS, abs(line->dy)>>FRACBITS, false);

					// Make sure the starting light level is in range.
					if (reallightlevel < flash->minlight)
						reallightlevel = (INT16)flash->minlight;
					else if (reallightlevel > flash->maxlight)
						reallightlevel = (INT16)flash->maxlight;

					sectors[secnum].lightlevel = reallightlevel;
				}
				else
				{
					// Use front sector for min, target sector for max,
					// the same way linetype 602 does it.
					P_SpawnAdjustableStrobeFlash(line->frontsector, &sectors[secnum],
						abs(line->dx)>>FRACBITS, abs(line->dy)>>FRACBITS, false);
				}
			}
			break;

		case 419: // Spawn adjustable strobe flash (synchronized)
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				if (line->flags & ML_NOCLIMB && line->backsector)
				{
					// Use front sector for min light level, back sector for max.
					// This is tricky because P_SpawnAdjustableGlowingLight expects
					// the maxsector (second argument) to also be the target
					// sector, so we have to do some light level twiddling.
					strobe_t *flash;
					INT16 reallightlevel = sectors[secnum].lightlevel;
					sectors[secnum].lightlevel = line->backsector->lightlevel;

					flash = P_SpawnAdjustableStrobeFlash(line->frontsector, &sectors[secnum],
						abs(line->dx)>>FRACBITS, abs(line->dy)>>FRACBITS, true);

					// Make sure the starting light level is in range.
					if (reallightlevel < flash->minlight)
						reallightlevel = (INT16)flash->minlight;
					else if (reallightlevel > flash->maxlight)
						reallightlevel = (INT16)flash->maxlight;

					sectors[secnum].lightlevel = reallightlevel;
				}
				else
				{
					// Use front sector for min, target sector for max,
					// the same way linetype 602 does it.
					P_SpawnAdjustableStrobeFlash(line->frontsector, &sectors[secnum],
						abs(line->dx)>>FRACBITS, abs(line->dy)>>FRACBITS, true);
				}
			}
			break;

		case 420: // Fade light levels in tagged sectors to new value
			P_FadeLight(line->tag, line->frontsector->lightlevel, P_AproxDistance(line->dx, line->dy)>>FRACBITS);
			break;

		case 421: // Stop lighting effect in tagged sectors
			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
				if (sectors[secnum].lightingdata)
				{
					P_RemoveThinker(&((elevator_t *)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}
			break;

		case 422: // Cut away to another view
			{
				mobj_t *altview;

				if (!mo || !mo->player) // only players have views
					return;

				if ((secnum = P_FindSectorFromLineTag(line, -1)) < 0)
					return;

				altview = P_GetObjectTypeInSectorNum(MT_ALTVIEWMAN, secnum);
				if (!altview)
					return;

				P_SetTarget(&mo->player->awayviewmobj, altview);
				mo->player->awayviewtics = P_AproxDistance(line->dx, line->dy)>>FRACBITS;

				if (line->flags & ML_NOCLIMB) // lets you specify a vertical angle
				{
					INT32 aim;

					aim = sides[line->sidenum[0]].textureoffset>>FRACBITS;
					while (aim < 0)
						aim += 360;
					while (aim >= 360)
						aim -= 360;
					aim *= (ANGLE_90>>8);
					aim /= 90;
					aim <<= 8;
					mo->player->awayviewaiming = (angle_t)aim;
				}
				else
					mo->player->awayviewaiming = 0; // straight ahead
			}
			break;

		case 423: // Change Sky
			if ((mo && mo->player && P_IsLocalPlayer(mo->player)) || (line->flags & ML_NOCLIMB))
				P_SetupLevelSky(sides[line->sidenum[0]].textureoffset>>FRACBITS, (line->flags & ML_NOCLIMB));
			break;

		case 424: // Change Weather
			if (line->flags & ML_NOCLIMB)
			{
				globalweather = (UINT8)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
				P_SwitchWeather(globalweather);
			}
			else if (mo && mo->player && P_IsLocalPlayer(mo->player))
				P_SwitchWeather(sides[line->sidenum[0]].textureoffset>>FRACBITS);
			break;

		case 425: // Calls P_SetMobjState on calling mobj
			if (mo && !mo->player)
				P_SetMobjState(mo, sides[line->sidenum[0]].toptexture); //P_AproxDistance(line->dx, line->dy)>>FRACBITS);
			break;

		case 426: // Moves the mobj to its sector's soundorg and on the floor, and stops it
			if (!mo)
				return;

			if (line->flags & ML_NOCLIMB)
			{
				P_UnsetThingPosition(mo);
				mo->x = mo->subsector->sector->soundorg.x;
				mo->y = mo->subsector->sector->soundorg.y;
				mo->z = mo->floorz;
				P_SetThingPosition(mo);
			}

			mo->momx = mo->momy = mo->momz = 1;
			mo->pmomz = 0;

			if (mo->player)
			{
				mo->player->rmomx = mo->player->rmomy = 1;
				mo->player->cmomx = mo->player->cmomy = 0;
				P_ResetPlayer(mo->player);
				P_SetPlayerMobjState(mo, S_PLAY_STND);

				// Reset bot too.
				if (bot) {
					if (line->flags & ML_NOCLIMB)
						P_TeleportMove(bot, mo->x, mo->y, mo->z);
					bot->momx = bot->momy = bot->momz = 1;
					bot->pmomz = 0;
					bot->player->rmomx = bot->player->rmomy = 1;
					bot->player->cmomx = bot->player->cmomy = 0;
					P_ResetPlayer(bot->player);
					P_SetPlayerMobjState(bot, S_PLAY_STND);
				}
			}
			break;

		case 427: // Awards points if the mobj is a player
			if (mo && mo->player)
				P_AddPlayerScore(mo->player, sides[line->sidenum[0]].textureoffset>>FRACBITS);
			break;

		case 428: // Start floating platform movement
			EV_DoElevator(line, elevateContinuous, true);
			break;

		case 429: // Crush Ceiling Down Once
			EV_DoCrush(line, crushCeilOnce);
			break;

		case 430: // Crush Floor Up Once
			EV_DoFloor(line, crushFloorOnce);
			break;

		case 431: // Crush Floor & Ceiling to middle Once
			EV_DoCrush(line, crushBothOnce);
			break;

		case 432: // Enable 2D Mode (Disable if noclimb)
			if (mo->player)
			{
				if (line->flags & ML_NOCLIMB)
					mo->flags2 &= ~MF2_TWOD;
				else
					mo->flags2 |= MF2_TWOD;

				// Copy effect to bot if necessary
				// (Teleport them to you so they don't break it.)
				if (bot && (bot->flags2 & MF2_TWOD) != (mo->flags2 & MF2_TWOD)) {
					bot->flags2 = (bot->flags2 & ~MF2_TWOD) | (mo->flags2 & MF2_TWOD);
					P_TeleportMove(bot, mo->x, mo->y, mo->z);
				}
			}
			break;

		case 433: // Flip gravity (Flop gravity if noclimb) Works on pushables, too!
			if (line->flags & ML_NOCLIMB)
				mo->flags2 &= ~MF2_OBJECTFLIP;
			else
				mo->flags2 |= MF2_OBJECTFLIP;
			if (bot)
				bot->flags2 = (bot->flags2 & ~MF2_OBJECTFLIP) | (mo->flags2 & MF2_OBJECTFLIP);
			break;

		case 434: // Custom Power
			if (mo->player)
			{
				mobj_t *dummy = P_SpawnMobj(mo->x, mo->y, mo->z, MT_NULL);

				var1 = sides[line->sidenum[0]].toptexture; //(line->dx>>FRACBITS)-1;

				if (line->sidenum[1] != 0xffff && line->flags & ML_BLOCKMONSTERS) // read power from back sidedef
					var2 = sides[line->sidenum[1]].toptexture;
				else if (line->flags & ML_NOCLIMB) // 'Infinite'
					var2 = UINT16_MAX;
				else
					var2 = sides[line->sidenum[0]].textureoffset>>FRACBITS;

				P_SetTarget(&dummy->target, mo);
				A_CustomPower(dummy);

				if (bot) {
					P_SetTarget(&dummy->target, bot);
					A_CustomPower(dummy);
				}
				P_RemoveMobj(dummy);
			}
			break;

		case 435: // Change scroller direction
			{
				scroll_t *scroller;
				thinker_t *th;

				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)T_Scroll)
						continue;

					scroller = (scroll_t *)th;

					if (sectors[scroller->affectee].tag != line->tag)
						continue;

					scroller->dx = FixedMul(line->dx>>SCROLL_SHIFT, CARRYFACTOR);
					scroller->dy = FixedMul(line->dy>>SCROLL_SHIFT, CARRYFACTOR);
				}
			}
			break;

		case 436: // Shatter block remotely
			{
				INT16 sectag = (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
				INT16 foftag = (INT16)(sides[line->sidenum[0]].rowoffset>>FRACBITS);
				sector_t *sec; // Sector that the FOF is visible in
				ffloor_t *rover; // FOF that we are going to crumble

				for (secnum = -1; (secnum = P_FindSectorFromTag(sectag, secnum)) >= 0 ;)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 436 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (rover->master->frontsector->tag == foftag)
							break;
					}

					if (!rover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 436 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}

					EV_CrumbleChain(sec, rover);
				}
			}
			break;

		case 437: // Disable Player Controls
			if (mo->player)
			{
				UINT16 fractime = (UINT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
				if (fractime < 1)
					fractime = 1; //instantly wears off upon leaving
				if (line->flags & ML_NOCLIMB)
					fractime |= 1<<15; //more crazy &ing, as if music stuff wasn't enough
				mo->player->powers[pw_nocontrol] = fractime;
				if (bot)
					bot->player->powers[pw_nocontrol] = fractime;
			}
			break;

		case 438: // Set player scale
			if (mo)
			{
				mo->destscale = FixedDiv(P_AproxDistance(line->dx, line->dy), 100<<FRACBITS);
				if (mo->destscale < FRACUNIT/100)
					mo->destscale = FRACUNIT/100;
				if (mo->player && bot)
					bot->destscale = mo->destscale;
			}
			break;

		case 439: // Set texture
			{
				size_t linenum;
				side_t *set = &sides[line->sidenum[0]], *this;
				boolean always = !(line->flags & ML_NOCLIMB); // If noclimb: Only change mid texture if mid texture already exists on tagged lines, etc.
				for (linenum = 0; linenum < numlines; linenum++)
				{
					if (lines[linenum].special == 439)
						continue; // Don't override other set texture lines!

					if (lines[linenum].tag != line->tag)
						continue; // Find tagged lines

					// Front side
					this = &sides[lines[linenum].sidenum[0]];
					if (always || this->toptexture) this->toptexture = set->toptexture;
					if (always || this->midtexture) this->midtexture = set->midtexture;
					if (always || this->bottomtexture) this->bottomtexture = set->bottomtexture;

					if (lines[linenum].sidenum[1] == 0xffff)
						continue; // One-sided stops here.

					// Back side
					this = &sides[lines[linenum].sidenum[1]];
					if (always || this->toptexture) this->toptexture = set->toptexture;
					if (always || this->midtexture) this->midtexture = set->midtexture;
					if (always || this->bottomtexture) this->bottomtexture = set->bottomtexture;
				}
			}
			break;

		case 440: // Play race countdown and start Metal Sonic
			if (!metalrecording && !metalplayback)
				G_DoPlayMetal();
			break;

		case 441: // Trigger unlockable
			if ((!modifiedgame || savemoddata) && !(netgame || multiplayer))
			{
				INT32 trigid = (INT32)(sides[line->sidenum[0]].textureoffset>>FRACBITS);

				if (trigid < 0 || trigid > 31) // limited by 32 bit variable
					CONS_Debug(DBG_GAMELOGIC, "Unlockable trigger (sidedef %hu): bad trigger ID %d\n", line->sidenum[0], trigid);
				else
				{
					unlocktriggers |= 1 << trigid;

					// Unlocked something?
					if (M_UpdateUnlockablesAndExtraEmblems())
					{
						S_StartSound(NULL, sfx_ncitem);
						G_SaveGameData(); // only save if unlocked something
					}
				}
			}

			// Execute one time only
			line->special = 0;
			break;

		case 442: // Calls P_SetMobjState on mobjs of a given type in the tagged sectors
		{
			const mobjtype_t type = (mobjtype_t)sides[line->sidenum[0]].toptexture;
			statenum_t state = NUMSTATES;
			sector_t *sec;
			mobj_t *thing;

			if (line->sidenum[1] != 0xffff)
				state = (statenum_t)sides[line->sidenum[1]].toptexture;

			while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
			{
				boolean tryagain;
				sec = sectors + secnum;
				do {
					tryagain = false;
					for (thing = sec->thinglist; thing; thing = thing->snext)
						if (thing->type == type)
						{
							if (state != NUMSTATES)
							{
								if (!P_SetMobjState(thing, state)) // set state to specific state
								{ // mobj was removed
									tryagain = true; // snext is corrupt, we'll have to start over.
									break;
								}
							}
							else if (!P_SetMobjState(thing, thing->state->nextstate)) // set state to nextstate
							{ // mobj was removed
								tryagain = true; // snext is corrupt, we'll have to start over.
								break;
							}
						}
				} while (tryagain);
			}
			break;
		}

		case 443: // Calls a named Lua function
#ifdef HAVE_BLUA
			if (line->text)
				LUAh_LinedefExecute(line, mo, callsec);
			else
				CONS_Alert(CONS_WARNING, "Linedef %s is missing the hook name of the Lua function to call! (This should be given in the front texture fields)\n", sizeu1(line-lines));
#else
			CONS_Alert(CONS_ERROR, "The map is trying to run a Lua script, but this exe was not compiled with Lua support!\n");
#endif
			break;

		case 444: // Earthquake camera
		{
			quake.intensity = sides[line->sidenum[0]].textureoffset;
			quake.radius = sides[line->sidenum[0]].rowoffset;
			quake.time = P_AproxDistance(line->dx, line->dy)>>FRACBITS;

			quake.epicenter = NULL; /// \todo

			// reasonable defaults.
			if (!quake.intensity)
				quake.intensity = 8<<FRACBITS;
			if (!quake.radius)
				quake.radius = 512<<FRACBITS;
			break;
		}

		case 445: // Force block disappear remotely (reappear if noclimb)
			{
				INT16 sectag = (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
				INT16 foftag = (INT16)(sides[line->sidenum[0]].rowoffset>>FRACBITS);
				sector_t *sec; // Sector that the FOF is visible (or not visible) in
				ffloor_t *rover; // FOF to vanish/un-vanish
				ffloortype_e oldflags; // store FOF's old flags

				for (secnum = -1; (secnum = P_FindSectorFromTag(sectag, secnum)) >= 0 ;)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 445 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (rover->master->frontsector->tag == foftag)
							break;
					}

					if (!rover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 445 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}

					oldflags = rover->flags;

					// Abracadabra!
					if (line->flags & ML_NOCLIMB)
						rover->flags |= FF_EXISTS;
					else
						rover->flags &= ~FF_EXISTS;

					// if flags changed, reset sector's light list
					if (rover->flags != oldflags)
						sec->moved = true;
				}
			}
			break;

		case 450: // Execute Linedef Executor - for recursion
			P_LinedefExecute(line->tag, mo, NULL);
			break;

		case 451: // Execute Random Linedef Executor
		{
			INT32 rvalue1 = sides[line->sidenum[0]].textureoffset>>FRACBITS;
			INT32 rvalue2 = sides[line->sidenum[0]].rowoffset>>FRACBITS;
			INT32 result;

			if (rvalue1 <= rvalue2)
				result = P_RandomRange(rvalue1, rvalue2);
			else
				result = P_RandomRange(rvalue2, rvalue1);

			P_LinedefExecute((INT16)result, mo, NULL);
			break;
		}

#ifdef POLYOBJECTS
		case 480: // Polyobj_DoorSlide
		case 481: // Polyobj_DoorSwing
			PolyDoor(line);
			break;
		case 482: // Polyobj_Move
		case 483: // Polyobj_OR_Move
			PolyMove(line);
			break;
		case 484: // Polyobj_RotateRight
		case 485: // Polyobj_OR_RotateRight
		case 486: // Polyobj_RotateLeft
		case 487: // Polyobj_OR_RotateLeft
			PolyRotate(line);
			break;
		case 488: // Polyobj_Waypoint
			PolyWaypoint(line);
			break;
		case 489:
			PolyInvisible(line);
			break;
		case 490:
			PolyVisible(line);
			break;
		case 491:
			PolyTranslucency(line);
			break;
#endif

		default:
			break;
	}
}

//
// P_SetupSignExit
//
// Finds the exit sign in the current sector and
// sets its target to the player who passed the map.
//
void P_SetupSignExit(player_t *player)
{
	mobj_t *thing;
	msecnode_t *node = player->mo->subsector->sector->touching_thinglist; // things touching this sector
	thinker_t *think;
	INT32 numfound = 0;

	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (thing->type != MT_SIGN)
			continue;

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetTarget(&thing->target, player->mo);
		P_SetMobjState(thing, S_SIGN1);
		if (thing->info->seesound)
			S_StartSound(thing, thing->info->seesound);

		++numfound;
	}

	if (numfound)
		return;

	// didn't find any signposts in the exit sector.
	// spin all signposts in the level then.
	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		thing = (mobj_t *)think;
		if (thing->type != MT_SIGN)
			continue;

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetTarget(&thing->target, player->mo);
		P_SetMobjState(thing, S_SIGN1);
		if (thing->info->seesound)
			S_StartSound(thing, thing->info->seesound);

		++numfound;
	}
}

//
// P_IsFlagAtBase
//
// Checks to see if a flag is at its base.
//
boolean P_IsFlagAtBase(mobjtype_t flag)
{
	thinker_t *think;
	mobj_t *mo;
	INT32 specialnum = 0;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;

		if (mo->type != flag)
			continue;

		if (mo->type == MT_REDFLAG)
			specialnum = 3;
		else if (mo->type == MT_BLUEFLAG)
			specialnum = 4;

		if (GETSECSPECIAL(mo->subsector->sector->special, 4) == specialnum)
			return true;
		else if (mo->subsector->sector->ffloors) // Check the 3D floors
		{
			ffloor_t *rover;

			for (rover = mo->subsector->sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->flags & FF_EXISTS))
					continue;

				if (GETSECSPECIAL(rover->master->frontsector->special, 4) != specialnum)
					continue;

				if (mo->z <= P_GetSpecialTopZ(mo, sectors + rover->secnum, mo->subsector->sector)
					&& mo->z >= P_GetSpecialBottomZ(mo, sectors + rover->secnum, mo->subsector->sector))
					return true;
			}
		}
	}
	return false;
}

//
// P_PlayerTouchingSectorSpecial
//
// Replaces the old player->specialsector.
// This allows a player to touch more than
// one sector at a time, if necessary.
//
// Returns a pointer to the first sector of
// the particular type that it finds.
// Returns NULL if it doesn't find it.
//
sector_t *P_PlayerTouchingSectorSpecial(player_t *player, INT32 section, INT32 number)
{
	msecnode_t *node;
	ffloor_t *rover;

	if (!player->mo)
		return NULL;

	// Check default case first
	if (GETSECSPECIAL(player->mo->subsector->sector->special, section) == number)
		return player->mo->subsector->sector;

	// Hmm.. maybe there's a FOF that has it...
	for (rover = player->mo->subsector->sector->ffloors; rover; rover = rover->next)
	{
		fixed_t topheight, bottomheight;

		if (GETSECSPECIAL(rover->master->frontsector->special, section) != number)
			continue;

		if (!(rover->flags & FF_EXISTS))
			continue;

		topheight = P_GetSpecialTopZ(player->mo, sectors + rover->secnum, player->mo->subsector->sector);
		bottomheight = P_GetSpecialBottomZ(player->mo, sectors + rover->secnum, player->mo->subsector->sector);

		// Check the 3D floor's type...
		if (rover->flags & FF_BLOCKPLAYER)
		{
			// Thing must be on top of the floor to be affected...
			if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING))
			{
				if ((player->mo->eflags & MFE_VERTICALFLIP) || player->mo->z != topheight)
					continue;
			}
			else if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR))
			{
				if (!(player->mo->eflags & MFE_VERTICALFLIP)
					|| player->mo->z + player->mo->height != bottomheight)
					continue;
			}
			else if (rover->master->frontsector->flags & SF_FLIPSPECIAL_BOTH)
			{
				if (!((player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height == bottomheight)
					|| (!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z == topheight)))
					continue;
			}
		}
		else
		{
			// Water and DEATH FOG!!! heh
			if (player->mo->z > topheight || (player->mo->z + player->mo->height) < bottomheight)
				continue;
		}

		// This FOF has the special we're looking for!
		return rover->master->frontsector;
	}

	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		if (GETSECSPECIAL(node->m_sector->special, section) == number)
		{
			// This sector has the special we're looking for, but
			// are we allowed to touch it?
			if (node->m_sector == player->mo->subsector->sector
				|| (node->m_sector->flags & SF_TRIGGERSPECIAL_TOUCH))
				return node->m_sector;
		}

		// Hmm.. maybe there's a FOF that has it...
		for (rover = node->m_sector->ffloors; rover; rover = rover->next)
		{
			fixed_t topheight, bottomheight;

			if (GETSECSPECIAL(rover->master->frontsector->special, section) != number)
				continue;

			if (!(rover->flags & FF_EXISTS))
				continue;

			topheight = P_GetSpecialTopZ(player->mo, sectors + rover->secnum, player->mo->subsector->sector);
			bottomheight = P_GetSpecialBottomZ(player->mo, sectors + rover->secnum, player->mo->subsector->sector);

			// Check the 3D floor's type...
			if (rover->flags & FF_BLOCKPLAYER)
			{
				// Thing must be on top of the floor to be affected...
				if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR)
					&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING))
				{
					if ((player->mo->eflags & MFE_VERTICALFLIP) || player->mo->z != topheight)
						continue;
				}
				else if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING)
					&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR))
				{
					if (!(player->mo->eflags & MFE_VERTICALFLIP)
						|| player->mo->z + player->mo->height != bottomheight)
						continue;
				}
				else if (rover->master->frontsector->flags & SF_FLIPSPECIAL_BOTH)
				{
					if (!((player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height == bottomheight)
						|| (!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z == topheight)))
						continue;
				}
			}
			else
			{
				// Water and DEATH FOG!!! heh
				if (player->mo->z > topheight || (player->mo->z + player->mo->height) < bottomheight)
					continue;
			}

			// This FOF has the special we're looking for, but are we allowed to touch it?
			if (node->m_sector == player->mo->subsector->sector
				|| (rover->master->frontsector->flags & SF_TRIGGERSPECIAL_TOUCH))
				return rover->master->frontsector;
		}
	}

	return NULL;
}

//
// P_ThingIsOnThe3DFloor
//
// This checks whether the mobj is on/in the FOF we want it to be at
// Needed for the "All players" trigger sector specials only
//
static boolean P_ThingIsOnThe3DFloor(mobj_t *mo, sector_t *sector, sector_t *targetsec)
{
	ffloor_t *rover;
	fixed_t top, bottom;

	if (!mo->player) // should NEVER happen
		return false;

	if (!targetsec->ffloors) // also should NEVER happen
		return false;

	for (rover = targetsec->ffloors; rover; rover = rover->next)
	{
		if (rover->master->frontsector != sector)
			continue;

		// we're assuming the FOF existed when the first player touched it
		//if (!(rover->flags & FF_EXISTS))
		//	return false;

		top = P_GetSpecialTopZ(mo, sector, targetsec);
		bottom = P_GetSpecialBottomZ(mo, sector, targetsec);

		// Check the 3D floor's type...
		if (rover->flags & FF_BLOCKPLAYER)
		{
			// Thing must be on top of the floor to be affected...
			if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING))
			{
				if ((mo->eflags & MFE_VERTICALFLIP) || mo->z != top)
					return false;
			}
			else if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR))
			{
				if (!(mo->eflags & MFE_VERTICALFLIP)
					|| mo->z + mo->height != bottom)
					return false;
			}
			else if (rover->master->frontsector->flags & SF_FLIPSPECIAL_BOTH)
			{
				if (!((mo->eflags & MFE_VERTICALFLIP && mo->z + mo->height == bottom)
					|| (!(mo->eflags & MFE_VERTICALFLIP) && mo->z == top)))
					return false;
			}
		}
		else
		{
			// Water and intangible FOFs
			if (mo->z > top || (mo->z + mo->height) < bottom)
				return false;
		}

		return true;
	}

	return false;
}

//
// P_MobjReadyToTrigger
//
// Is player standing on the sector's "ground"?
//
static boolean P_MobjReadyToTrigger(mobj_t *mo, sector_t *sec)
{
	if (mo->eflags & MFE_VERTICALFLIP)
		return (mo->z+mo->height == P_GetSpecialTopZ(mo, sec, sec) && sec->flags & SF_FLIPSPECIAL_CEILING);
	else
		return (mo->z == P_GetSpecialBottomZ(mo, sec, sec) && sec->flags & SF_FLIPSPECIAL_FLOOR);
}

/** Applies a sector special to a player.
  *
  * \param player       Player in the sector.
  * \param sector       Sector with the special.
  * \param roversector  If !NULL, sector is actually an FOF; otherwise, sector
  *                     is being physically contacted by the player.
  * \todo Split up into multiple functions.
  * \sa P_PlayerInSpecialSector, P_PlayerOnSpecial3DFloor
  */
void P_ProcessSpecialSector(player_t *player, sector_t *sector, sector_t *roversector)
{
	INT32 i = 0;
	INT32 section1, section2, section3, section4;
	INT32 special;

	section1 = GETSECSPECIAL(sector->special, 1);
	section2 = GETSECSPECIAL(sector->special, 2);
	section3 = GETSECSPECIAL(sector->special, 3);
	section4 = GETSECSPECIAL(sector->special, 4);

	// Ignore spectators
	if (player->spectator)
		return;

	// Ignore dead players.
	// If this strange phenomenon could be potentially used in levels,
	// TODO: modify this to accommodate for it.
	if (player->playerstate == PST_DEAD)
		return;

	// Conveyor stuff
	if (section3 == 2 || section3 == 4)
		player->onconveyor = section3;

	special = section1;

	// Process Section 1
	switch (special)
	{
		case 1: // Damage (Generic)
			if (roversector || P_MobjReadyToTrigger(player->mo, sector))
				P_DamageMobj(player->mo, NULL, NULL, 1);
			break;
		case 2: // Damage (Water)
			if ((roversector || P_MobjReadyToTrigger(player->mo, sector)) && (player->powers[pw_underwater] || player->pflags & PF_NIGHTSMODE) && (player->powers[pw_shield] & SH_NOSTACK) != SH_ELEMENTAL)
				P_DamageMobj(player->mo, NULL, NULL, 1);
			break;
		case 3: // Damage (Fire)
			if ((roversector || P_MobjReadyToTrigger(player->mo, sector)) && (player->powers[pw_shield] & SH_NOSTACK) != SH_ELEMENTAL)
				P_DamageMobj(player->mo, NULL, NULL, 1);
			break;
		case 4: // Damage (Electrical)
			if ((roversector || P_MobjReadyToTrigger(player->mo, sector)) && (player->powers[pw_shield] & SH_NOSTACK) != SH_ATTRACT)
				P_DamageMobj(player->mo, NULL, NULL, 1);
			break;
		case 5: // Spikes
			// Don't do anything. In Soviet Russia, spikes find you.
			break;
		case 6: // Death Pit (Camera Mod)
		case 7: // Death Pit (No Camera Mod)
			if (roversector || P_MobjReadyToTrigger(player->mo, sector))
				P_DamageMobj(player->mo, NULL, NULL, 10000);
			break;
		case 8: // Instant Kill
			P_DamageMobj(player->mo, NULL, NULL, 10000);
			break;
		case 9: // Ring Drainer (Floor Touch)
		case 10: // Ring Drainer (No Floor Touch)
			if (leveltime % (TICRATE/2) == 0 && player->mo->health > 1)
			{
				player->mo->health--;
				player->health--;
				S_StartSound(player->mo, sfx_itemup);
			}
			break;
		case 11: // Special Stage Damage - Kind of like a mini-P_DamageMobj()
			if (player->powers[pw_invulnerability] || player->powers[pw_flashing] || player->powers[pw_super] || player->exiting || player->bot)
				break;

			if (!(player->powers[pw_shield] || player->mo->health > 1)) // Don't do anything if no shield or rings anyway
				break;

			if (player->powers[pw_shield])
			{
				P_RemoveShield(player);
				S_StartSound(player->mo, sfx_shldls); // Ba-Dum! Shield loss.
			}
			else if (player->mo->health > 1)
			{
				P_PlayRinglossSound(player->mo);
				if (player->mo->health > 10)
					player->mo->health -= 10;
				else
					player->mo->health = 1;
				player->health = player->mo->health;
			}

			P_DoPlayerPain(player, NULL, NULL); // this does basically everything that was here before

			if (gametype == GT_CTF && player->gotflag & (GF_REDFLAG|GF_BLUEFLAG))
				P_PlayerFlagBurst(player, false);
			break;
		case 12: // Space Countdown
			if ((player->powers[pw_shield] & SH_NOSTACK) != SH_ELEMENTAL && !player->powers[pw_spacetime])
				player->powers[pw_spacetime] = spacetimetics + 1;
			break;
		case 13: // Ramp Sector (Increase step-up/down)
		case 14: // Non-Ramp Sector (Don't step-down)
		case 15: // Bouncy Sector (FOF Control Only)
			break;
	}

	special = section2;

	// Process Section 2
	switch (special)
	{
		case 1: // Trigger Linedef Exec (Pushable Objects)
			break;
		case 2: // Linedef executor requires all players present+doesn't require touching floor
		case 3: // Linedef executor requires all players present
			/// \todo check continues for proper splitscreen support?
			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && !players[i].bot && players[i].mo && (gametype != GT_COOP || players[i].lives > 0))
				{
					if (roversector)
					{
						if (players[i].mo->subsector->sector == roversector)
							;
						else if (sector->flags & SF_TRIGGERSPECIAL_TOUCH)
						{
							boolean insector = false;
							msecnode_t *node;
							for (node = players[i].mo->touching_sectorlist; node; node = node->m_sectorlist_next)
							{
								if (node->m_sector == roversector)
								{
									insector = true;
									break;
								}
							}
							if (!insector)
								goto DoneSection2;
						}
						else
							goto DoneSection2;

						if (!P_ThingIsOnThe3DFloor(players[i].mo, sector, roversector))
							goto DoneSection2;
					}
					else
					{
						if (players[i].mo->subsector->sector == sector)
							;
						else if (sector->flags & SF_TRIGGERSPECIAL_TOUCH)
						{
							boolean insector = false;
							msecnode_t *node;
							for (node = players[i].mo->touching_sectorlist; node; node = node->m_sectorlist_next)
							{
								if (node->m_sector == sector)
								{
									insector = true;
									break;
								}
							}
							if (!insector)
								goto DoneSection2;
						}
						else
							goto DoneSection2;

						if (special == 3 && !P_MobjReadyToTrigger(players[i].mo, sector))
							goto DoneSection2;
					}
				}
			/* FALLTHRU */
		case 4: // Linedef executor that doesn't require touching floor
		case 5: // Linedef executor
		case 6: // Linedef executor (7 Emeralds)
		case 7: // Linedef executor (NiGHTS Mare)
			if (!player->bot)
				P_LinedefExecute(sector->tag, player->mo, sector);
			break;
		case 8: // Tells pushable things to check FOFs
			break;
		case 9: // Egg trap capsule
		{
			thinker_t *th;
			mobj_t *mo2;
			line_t junk;

			if (player->bot || sector->ceilingdata || sector->floordata)
				return;

			// Find the center of the Eggtrap and release all the pretty animals!
			// The chimps are my friends.. heeheeheheehehee..... - LouisJM
			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

				mo2 = (mobj_t *)th;
				if (mo2->type == MT_EGGTRAP)
					P_KillMobj(mo2, NULL, player->mo);
			}

			// clear the special so you can't push the button twice.
			sector->special = 0;

			// Move the button down
			junk.tag = 680;
			EV_DoElevator(&junk, elevateDown, false);

			// Open the top FOF
			junk.tag = 681;
			EV_DoFloor(&junk, raiseFloorToNearestFast);
			// Open the bottom FOF
			junk.tag = 682;
			EV_DoCeiling(&junk, lowerToLowestFast);

			// Mark all players with the time to exit thingy!
			for (i = 0; i < MAXPLAYERS; i++)
				P_DoPlayerExit(&players[i]);
			break;
		}
		case 10: // Special Stage Time/Rings
		case 11: // Custom Gravity
			break;
		case 12: // Lua sector special
			break;
	}
DoneSection2:

	special = section3;

	// Process Section 3
	switch (special)
	{
		case 1: // Ice/Sludge
		case 2: // Wind/Current
		case 3: // Ice/Sludge and Wind/Current
		case 4: // Conveyor Belt
			break;

		case 5: // Speed pad w/o spin
		case 6: // Speed pad w/ spin
			if (player->powers[pw_flashing] != 0 && player->powers[pw_flashing] < TICRATE/2)
				break;

			i = P_FindSpecialLineFromTag(4, sector->tag, -1);

			if (i != -1)
			{
				angle_t lineangle;
				fixed_t linespeed;

				lineangle = R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y);
				linespeed = P_AproxDistance(lines[i].v2->x-lines[i].v1->x, lines[i].v2->y-lines[i].v1->y);

				player->mo->angle = lineangle;

				if (!demoplayback || P_AnalogMove(player))
				{
					if (player == &players[consoleplayer])
						localangle = player->mo->angle;
					else if (player == &players[secondarydisplayplayer])
						localangle2 = player->mo->angle;
				}

				if (!(lines[i].flags & ML_EFFECT4))
				{
					P_UnsetThingPosition(player->mo);
					if (roversector) // make FOF speed pads work
					{
						player->mo->x = roversector->soundorg.x;
						player->mo->y = roversector->soundorg.y;
					}
					else
					{
						player->mo->x = sector->soundorg.x;
						player->mo->y = sector->soundorg.y;
					}
					P_SetThingPosition(player->mo);
				}

				P_InstaThrust(player->mo, player->mo->angle, linespeed);

				if (GETSECSPECIAL(sector->special, 3) == 6 && (player->charability2 == CA2_SPINDASH))
				{
					if (!(player->pflags & PF_SPINNING))
						player->pflags |= PF_SPINNING;

					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
				}

				player->powers[pw_flashing] = TICRATE/3;
				S_StartSound(player->mo, sfx_spdpad);
			}
			break;

		case 7: // Bustable block sprite parameter
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			break;
	}

	special = section4;

	// Process Section 4
	switch (special)
	{
		case 1: // Starpost Activator
		{
			mobj_t *post = P_GetObjectTypeInSectorNum(MT_STARPOST, sector - sectors);

			if (!post)
				break;

			P_TouchSpecialThing(post, player->mo, false);
			break;
		}

		case 2: // Special stage GOAL sector / Exit Sector / CTF Flag Return
			if (player->bot)
				break;
			if (!useNightsSS && G_IsSpecialStage(gamemap) && sstimer > 6)
				sstimer = 6; // Just let P_Ticker take care of the rest.

			// Exit (for FOF exits; others are handled in P_PlayerThink in p_user.c)
			{
				INT32 lineindex;

				P_DoPlayerExit(player);

				P_SetupSignExit(player);
				// important: use sector->tag on next line instead of player->mo->subsector->tag
				// this part is different from in P_PlayerThink, this is what was causing
				// FOF custom exits not to work.
				lineindex = P_FindSpecialLineFromTag(2, sector->tag, -1);

				if (gametype == GT_COOP && lineindex != -1) // Custom exit!
				{
					// Special goodies with the block monsters flag depending on emeralds collected
					if ((lines[lineindex].flags & ML_BLOCKMONSTERS) && ALL7EMERALDS(emeralds))
						nextmapoverride = (INT16)(lines[lineindex].frontsector->ceilingheight>>FRACBITS);
					else
						nextmapoverride = (INT16)(lines[lineindex].frontsector->floorheight>>FRACBITS);

					if (lines[lineindex].flags & ML_NOCLIMB)
						skipstats = true;
				}
			}
			break;

		case 3: // Red Team's Base
			if (gametype == GT_CTF && P_IsObjectOnGround(player->mo))
			{
				if (player->ctfteam == 1 && (player->gotflag & GF_BLUEFLAG))
				{
					mobj_t *mo;

					// Make sure the red team still has their own
					// flag at their base so they can score.
					if (!P_IsFlagAtBase(MT_REDFLAG))
						break;

					HU_SetCEchoFlags(0);
					HU_SetCEchoDuration(5);
					HU_DoCEcho(va(M_GetText("%s\\captured the blue flag.\\\\\\\\"), player_names[player-players]));

					if (splitscreen || players[consoleplayer].ctfteam == 1)
						S_StartSound(NULL, sfx_flgcap);
					else if (players[consoleplayer].ctfteam == 2)
						S_StartSound(NULL, sfx_lose);

					mo = P_SpawnMobj(player->mo->x,player->mo->y,player->mo->z,MT_BLUEFLAG);
					player->gotflag &= ~GF_BLUEFLAG;
					mo->flags &= ~MF_SPECIAL;
					mo->fuse = TICRATE;
					mo->spawnpoint = bflagpoint;
					mo->flags2 |= MF2_JUSTATTACKED;
					redscore += 1;
					P_AddPlayerScore(player, 250);
				}
			}
			break;

		case 4: // Blue Team's Base
			if (gametype == GT_CTF && P_IsObjectOnGround(player->mo))
			{
				if (player->ctfteam == 2 && (player->gotflag & GF_REDFLAG))
				{
					mobj_t *mo;

					// Make sure the blue team still has their own
					// flag at their base so they can score.
					if (!P_IsFlagAtBase(MT_BLUEFLAG))
						break;

					HU_SetCEchoFlags(0);
					HU_SetCEchoDuration(5);
					HU_DoCEcho(va(M_GetText("%s\\captured the red flag.\\\\\\\\"), player_names[player-players]));

					if (splitscreen || players[consoleplayer].ctfteam == 2)
						S_StartSound(NULL, sfx_flgcap);
					else if (players[consoleplayer].ctfteam == 1)
						S_StartSound(NULL, sfx_lose);

					mo = P_SpawnMobj(player->mo->x,player->mo->y,player->mo->z,MT_REDFLAG);
					player->gotflag &= ~GF_REDFLAG;
					mo->flags &= ~MF_SPECIAL;
					mo->fuse = TICRATE;
					mo->spawnpoint = rflagpoint;
					mo->flags2 |= MF2_JUSTATTACKED;
					bluescore += 1;
					P_AddPlayerScore(player, 250);
				}
			}
			break;

		case 5: // Fan sector
			player->mo->momz += mobjinfo[MT_FAN].mass/4;

			if (player->mo->momz > mobjinfo[MT_FAN].mass)
				player->mo->momz = mobjinfo[MT_FAN].mass;

			P_ResetPlayer(player);
			if (player->panim != PA_FALL)
				P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
			break;

		case 6: // Super Sonic transformer
			if (player->mo->health > 0 && !player->bot && (player->charflags & SF_SUPER) && !player->powers[pw_super] && ALL7EMERALDS(emeralds))
				P_DoSuperTransformation(player, true);
			break;

		case 7: // Make player spin
			if (!(player->pflags & PF_SPINNING) && P_IsObjectOnGround(player->mo) && (player->charability2 == CA2_SPINDASH))
			{
				player->pflags |= PF_SPINNING;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
				S_StartAttackSound(player->mo, sfx_spin);

				if (abs(player->rmomx) < FixedMul(5*FRACUNIT, player->mo->scale)
				&& abs(player->rmomy) < FixedMul(5*FRACUNIT, player->mo->scale))
					P_InstaThrust(player->mo, player->mo->angle, FixedMul(10*FRACUNIT, player->mo->scale));
			}
			break;

		case 8: // Zoom Tube Start
			{
				INT32 sequence;
				fixed_t speed;
				INT32 lineindex;
				thinker_t *th;
				mobj_t *waypoint = NULL;
				mobj_t *mo2;
				angle_t an;

				if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT)
					break;

				// Find line #3 tagged to this sector
				lineindex = P_FindSpecialLineFromTag(3, sector->tag, -1);

				if (lineindex == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Sector special %d missing line special #3.\n", sector->special);
					break;
				}

				// Grab speed and sequence values
				speed = abs(lines[lineindex].dx)/8;
				sequence = abs(lines[lineindex].dy)>>FRACBITS;

				// scan the thinkers
				// to find the first waypoint
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type == MT_TUBEWAYPOINT && mo2->threshold == sequence
						&& mo2->health == 0)
					{
						waypoint = mo2;
						break;
					}
				}

				if (!waypoint)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: FIRST WAYPOINT IN SEQUENCE %d NOT FOUND.\n", sequence);
					break;
				}
				else
				{
					CONS_Debug(DBG_GAMELOGIC, "Waypoint %d found in sequence %d - speed = %d\n", waypoint->health, sequence, speed);
				}

				an = R_PointToAngle2(player->mo->x, player->mo->y, waypoint->x, waypoint->y) - player->mo->angle;

				if (an > ANGLE_90 && an < ANGLE_270 && !(lines[lineindex].flags & ML_EFFECT4))
					break; // behind back

				P_SetTarget(&player->mo->tracer, waypoint);
				player->speed = speed;
				player->pflags |= PF_SPINNING;
				player->pflags &= ~PF_JUMPED;
				player->pflags &= ~PF_GLIDING;
				player->climbing = 0;

				if (!(player->mo->state >= &states[S_PLAY_ATK1] && player->mo->state <= &states[S_PLAY_ATK4]))
				{
					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
					S_StartSound(player->mo, sfx_spin);
				}
			}
			break;

		case 9: // Zoom Tube End
			{
				INT32 sequence;
				fixed_t speed;
				INT32 lineindex;
				thinker_t *th;
				mobj_t *waypoint = NULL;
				mobj_t *mo2;
				angle_t an;

				if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT)
					break;

				// Find line #3 tagged to this sector
				lineindex = P_FindSpecialLineFromTag(3, sector->tag, -1);

				if (lineindex == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Sector special %d missing line special #3.\n", sector->special);
					break;
				}

				// Grab speed and sequence values
				speed = -(abs(lines[lineindex].dx)/8); // Negative means reverse
				sequence = abs(lines[lineindex].dy)>>FRACBITS;

				// scan the thinkers
				// to find the last waypoint
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type == MT_TUBEWAYPOINT && mo2->threshold == sequence)
					{
						if (!waypoint)
							waypoint = mo2;
						else if (mo2->health > waypoint->health)
							waypoint = mo2;
					}
				}

				if (!waypoint)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: LAST WAYPOINT IN SEQUENCE %d NOT FOUND.\n", sequence);
					break;
				}
				else
				{
					CONS_Debug(DBG_GAMELOGIC, "Waypoint %d found in sequence %d - speed = %d\n", waypoint->health, sequence, speed);
				}

				an = R_PointToAngle2(player->mo->x, player->mo->y, waypoint->x, waypoint->y) - player->mo->angle;

				if (an > ANGLE_90 && an < ANGLE_270 && !(lines[lineindex].flags & ML_EFFECT4))
					break; // behind back

				P_SetTarget(&player->mo->tracer, waypoint);
				player->speed = speed;
				player->pflags |= PF_SPINNING;
				player->pflags &= ~PF_JUMPED;

				if (!(player->mo->state >= &states[S_PLAY_ATK1] && player->mo->state <= &states[S_PLAY_ATK4]))
				{
					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
					S_StartSound(player->mo, sfx_spin);
				}
			}
			break;

		case 10: // Finish Line
			if (gametype == GT_RACE && !player->exiting)
			{
				if (player->starpostnum == numstarposts) // Must have touched all the starposts
				{
					player->laps++;

					if (player->pflags & PF_NIGHTSMODE)
						player->drillmeter += 48*20;

					if (player->laps >= (UINT8)cv_numlaps.value)
						CONS_Printf(M_GetText("%s has finished the race.\n"), player_names[player-players]);
					else
						CONS_Printf(M_GetText("%s started lap %u\n"), player_names[player-players], (UINT32)player->laps+1);

					// Reset starposts (checkpoints) info
					player->starpostangle = player->starposttime = player->starpostnum = 0;
					player->starpostx = player->starposty = player->starpostz = 0;
					P_ResetStarposts();

					// Play the starpost sound for 'consistency'
					S_StartSound(player->mo, sfx_strpst);
				}
				else if (player->starpostnum)
				{
					// blatant reuse of a variable that's normally unused in circuit
					if (!player->tossdelay)
						S_StartSound(player->mo, sfx_lose);
					player->tossdelay = 3;
				}

				if (player->laps >= (unsigned)cv_numlaps.value)
				{
					if (P_IsLocalPlayer(player))
					{
						HU_SetCEchoFlags(0);
						HU_SetCEchoDuration(5);
						HU_DoCEcho("FINISHED!");
					}

					P_DoPlayerExit(player);
				}
			}
			break;

		case 11: // Rope hang
			{
				INT32 sequence;
				fixed_t speed;
				INT32 lineindex;
				thinker_t *th;
				mobj_t *waypointmid = NULL;
				mobj_t *waypointhigh = NULL;
				mobj_t *waypointlow = NULL;
				mobj_t *mo2;
				mobj_t *closest = NULL;
				line_t junk;
				vertex_t v1, v2, resulthigh, resultlow;
				mobj_t *highest = NULL;

				if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT)
					break;

				if (player->mo->momz > 0)
					break;

				if (player->cmd.buttons & BT_USE)
					break;

				if (!(player->pflags & PF_SLIDING) && player->mo->state == &states[player->mo->info->painstate])
					break;

				if (player->exiting)
					break;

				//initialize resulthigh and resultlow with 0
				memset(&resultlow, 0x00, sizeof(resultlow));
				memset(&resulthigh, 0x00, sizeof(resulthigh));

				// Find line #11 tagged to this sector
				lineindex = P_FindSpecialLineFromTag(11, sector->tag, -1);

				if (lineindex == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Sector special %d missing line special #11.\n", sector->special);
					break;
				}

				// Grab speed and sequence values
				speed = abs(lines[lineindex].dx)/8;
				sequence = abs(lines[lineindex].dy)>>FRACBITS;

				// Find the closest waypoint
				// Find the preceding waypoint
				// Find the proceeding waypoint
				// Determine the closest spot on the line between the three waypoints
				// Put player at that location.

				// scan the thinkers
				// to find the first waypoint
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type != MT_TUBEWAYPOINT)
						continue;

					if (mo2->threshold != sequence)
						continue;

					if (!highest)
						highest = mo2;
					else if (mo2->health > highest->health) // Find the highest waypoint # in case we wrap
						highest = mo2;

					if (closest && P_AproxDistance(P_AproxDistance(player->mo->x-mo2->x, player->mo->y-mo2->y),
						player->mo->z-mo2->z) > P_AproxDistance(P_AproxDistance(player->mo->x-closest->x,
						player->mo->y-closest->y), player->mo->z-closest->z))
						continue;

					// Found a target
					closest = mo2;
				}

				waypointmid = closest;

				closest = NULL;

				if (waypointmid == NULL)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: WAYPOINT(S) IN SEQUENCE %d NOT FOUND.\n", sequence);
					break;
				}

				// Find waypoint before this one (waypointlow)
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type != MT_TUBEWAYPOINT)
						continue;

					if (mo2->threshold != sequence)
						continue;

					if (waypointmid->health == 0)
					{
						if (mo2->health != highest->health)
							continue;
					}
					else if (mo2->health != waypointmid->health - 1)
						continue;

					// Found a target
					waypointlow = mo2;
					break;
				}

				// Find waypoint after this one (waypointhigh)
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type != MT_TUBEWAYPOINT)
						continue;

					if (mo2->threshold != sequence)
						continue;

					if (waypointmid->health == highest->health)
					{
						if (mo2->health != 0)
							continue;
					}
					else if (mo2->health != waypointmid->health + 1)
						continue;

					// Found a target
					waypointhigh = mo2;
					break;
				}

				CONS_Debug(DBG_GAMELOGIC, "WaypointMid: %d; WaypointLow: %d; WaypointHigh: %d\n",
								waypointmid->health, waypointlow ? waypointlow->health : -1, waypointhigh ? waypointhigh->health : -1);

				// Now we have three waypoints... the closest one we're near, and the one that comes before, and after.
				// Next, we need to find the closest point on the line between each set, and determine which one we're
				// closest to.

				// Waypointmid and Waypointlow:
				if (waypointlow)
				{
					v1.x = waypointmid->x;
					v1.y = waypointmid->y;
					v1.z = waypointmid->z;
					v2.x = waypointlow->x;
					v2.y = waypointlow->y;
					v2.z = waypointlow->z;
					junk.v1 = &v1;
					junk.v2 = &v2;
					junk.dx = v2.x - v1.x;
					junk.dy = v2.y - v1.y;

					P_ClosestPointOnLine3D(player->mo->x, player->mo->y, player->mo->z, &junk, &resultlow);
				}

				// Waypointmid and Waypointhigh:
				if (waypointhigh)
				{
					v1.x = waypointmid->x;
					v1.y = waypointmid->y;
					v1.z = waypointmid->z;
					v2.x = waypointhigh->x;
					v2.y = waypointhigh->y;
					v2.z = waypointhigh->z;
					junk.v1 = &v1;
					junk.v2 = &v2;
					junk.dx = v2.x - v1.x;
					junk.dy = v2.y - v1.y;

					P_ClosestPointOnLine3D(player->mo->x, player->mo->y, player->mo->z, &junk, &resulthigh);
				}

				// 3D support now available. Disregard the previous notice here. -Red

				P_UnsetThingPosition(player->mo);
				P_ResetPlayer(player);
				player->mo->momx = player->mo->momy = player->mo->momz = 0;

				if (lines[lineindex].flags & ML_EFFECT1) // Don't wrap
				{
					highest->flags |= MF_SLIDEME;
				}

				// Changing the conditions on these ifs to fix issues with snapping to the wrong spot -Red
				if ((lines[lineindex].flags & ML_EFFECT1) && waypointmid->health == 0)
				{
					closest = waypointhigh;
					player->mo->x = resulthigh.x;
					player->mo->y = resulthigh.y;
					player->mo->z = resulthigh.z - P_GetPlayerHeight(player);
				}
				else if ((lines[lineindex].flags & ML_EFFECT1) && waypointmid->health == highest->health)
				{
					closest = waypointmid;
					player->mo->x = resultlow.x;
					player->mo->y = resultlow.y;
					player->mo->z = resultlow.z - P_GetPlayerHeight(player);
				}
				else
				{
					if (P_AproxDistance(P_AproxDistance(player->mo->x-resultlow.x, player->mo->y-resultlow.y),
							player->mo->z-resultlow.z) < P_AproxDistance(P_AproxDistance(player->mo->x-resulthigh.x,
								player->mo->y-resulthigh.y), player->mo->z-resulthigh.z))
					{
						// Line between Mid and Low is closer
						closest = waypointmid;
						player->mo->x = resultlow.x;
						player->mo->y = resultlow.y;
						player->mo->z = resultlow.z - P_GetPlayerHeight(player);
					}
					else
					{
						// Line between Mid and High is closer
						closest = waypointhigh;
						player->mo->x = resulthigh.x;
						player->mo->y = resulthigh.y;
						player->mo->z = resulthigh.z - P_GetPlayerHeight(player);
					}
				}

				P_SetTarget(&player->mo->tracer, closest);

				// Option for static ropes.
				if (lines[lineindex].flags & ML_NOCLIMB)
					player->speed = 0;
				else
					player->speed = speed;

				player->pflags |= PF_ROPEHANG;

				S_StartSound(player->mo, sfx_s3k4a);

				player->pflags &= ~PF_JUMPED;
				player->pflags &= ~PF_GLIDING;
				player->pflags &= ~PF_SLIDING;
				player->climbing = 0;
				P_SetThingPosition(player->mo);
				P_SetPlayerMobjState(player->mo, S_PLAY_CARRY);
			}
			break;
		case 12: // Camera noclip
		case 13: // Unused
		case 14: // Unused
		case 15: // Unused
			break;
	}
}

/** Checks if an object is standing on or is inside a special 3D floor.
  * If so, the sector is returned.
  *
  * \param mo Object to check.
  * \return Pointer to the sector with a special type, or NULL if no special 3D
  *         floors are being contacted.
  * \sa P_PlayerOnSpecial3DFloor
  */
sector_t *P_ThingOnSpecial3DFloor(mobj_t *mo)
{
	sector_t *sector;
	ffloor_t *rover;
	fixed_t topheight, bottomheight;

	sector = mo->subsector->sector;
	if (!sector->ffloors)
		return NULL;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->special)
			continue;

		if (!(rover->flags & FF_EXISTS))
			continue;

		topheight = P_GetSpecialTopZ(mo, sectors + rover->secnum, sector);
		bottomheight = P_GetSpecialBottomZ(mo, sectors + rover->secnum, sector);

		// Check the 3D floor's type...
		if (((rover->flags & FF_BLOCKPLAYER) && mo->player)
			|| ((rover->flags & FF_BLOCKOTHERS) && !mo->player))
		{
			// Thing must be on top of the floor to be affected...
			if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING))
			{
				if ((mo->eflags & MFE_VERTICALFLIP) || mo->z != topheight)
					continue;
			}
			else if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR))
			{
				if (!(mo->eflags & MFE_VERTICALFLIP)
					|| mo->z + mo->height != bottomheight)
					continue;
			}
			else if (rover->master->frontsector->flags & SF_FLIPSPECIAL_BOTH)
			{
				if (!((mo->eflags & MFE_VERTICALFLIP && mo->z + mo->height == bottomheight)
					|| (!(mo->eflags & MFE_VERTICALFLIP) && mo->z == topheight)))
					continue;
			}
		}
		else
		{
			// Water and intangible FOFs
			if (mo->z > topheight || (mo->z + mo->height) < bottomheight)
				continue;
		}

		return rover->master->frontsector;
	}

	return NULL;
}

#define TELEPORTED (player->mo->subsector->sector != originalsector)

/** Checks if a player is standing on or is inside a 3D floor (e.g. water) and
  * applies any specials.
  *
  * \param player Player to check.
  * \sa P_ThingOnSpecial3DFloor, P_PlayerInSpecialSector
  */
static void P_PlayerOnSpecial3DFloor(player_t *player, sector_t *sector)
{
	sector_t *originalsector = player->mo->subsector->sector;
	ffloor_t *rover;
	fixed_t topheight, bottomheight;

	for (rover = sector->ffloors; rover; rover = rover->next)
	{
		if (!rover->master->frontsector->special)
			continue;

		if (!(rover->flags & FF_EXISTS))
			continue;

		topheight = P_GetSpecialTopZ(player->mo, sectors + rover->secnum, sector);
		bottomheight = P_GetSpecialBottomZ(player->mo, sectors + rover->secnum, sector);

		// Check the 3D floor's type...
		if (rover->flags & FF_BLOCKPLAYER)
		{
			// Thing must be on top of the floor to be affected...
			if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING))
			{
				if ((player->mo->eflags & MFE_VERTICALFLIP) || player->mo->z != topheight)
					continue;
			}
			else if ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING)
				&& !(rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR))
			{
				if (!(player->mo->eflags & MFE_VERTICALFLIP)
					|| player->mo->z + player->mo->height != bottomheight)
					continue;
			}
			else if (rover->master->frontsector->flags & SF_FLIPSPECIAL_BOTH)
			{
				if (!((player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height == bottomheight)
					|| (!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z == topheight)))
					continue;
			}
		}
		else
		{
			// Water and DEATH FOG!!! heh
			if (player->mo->z > topheight || (player->mo->z + player->mo->height) < bottomheight)
				continue;
		}

		// This FOF has the special we're looking for, but are we allowed to touch it?
		if (sector == player->mo->subsector->sector
			|| (rover->master->frontsector->flags & SF_TRIGGERSPECIAL_TOUCH))
		{
			P_ProcessSpecialSector(player, rover->master->frontsector, sector);
			if TELEPORTED return;
		}
	}

	// Allow sector specials to be applied to polyobjects!
	if (player->mo->subsector->polyList)
	{
		polyobj_t *po = player->mo->subsector->polyList;
		sector_t *polysec;
		boolean touching = false;
		boolean inside = false;

		while (po)
		{
			if (po->flags & POF_NOSPECIALS)
			{
				po = (polyobj_t *)(po->link.next);
				continue;
			}

			polysec = po->lines[0]->backsector;

			if ((polysec->flags & SF_TRIGGERSPECIAL_TOUCH))
				touching = P_MobjTouchingPolyobj(po, player->mo);
			else
				touching = false;

			inside = P_MobjInsidePolyobj(po, player->mo);

			if (!(inside || touching))
			{
				po = (polyobj_t *)(po->link.next);
				continue;
			}

			// We're inside it! Yess...
			if (!polysec->special)
			{
				po = (polyobj_t *)(po->link.next);
				continue;
			}

			if (!(po->flags & POF_TESTHEIGHT)) // Don't do height checking
			{
			}
			else if (po->flags & POF_SOLID)
			{
				// Thing must be on top of the floor to be affected...
				if ((polysec->flags & SF_FLIPSPECIAL_FLOOR)
					&& !(polysec->flags & SF_FLIPSPECIAL_CEILING))
				{
					if ((player->mo->eflags & MFE_VERTICALFLIP) || player->mo->z != polysec->ceilingheight)
					{
						po = (polyobj_t *)(po->link.next);
						continue;
					}
				}
				else if ((polysec->flags & SF_FLIPSPECIAL_CEILING)
					&& !(polysec->flags & SF_FLIPSPECIAL_FLOOR))
				{
					if (!(player->mo->eflags & MFE_VERTICALFLIP)
						|| player->mo->z + player->mo->height != polysec->floorheight)
					{
						po = (polyobj_t *)(po->link.next);
						continue;
					}
				}
				else if (polysec->flags & SF_FLIPSPECIAL_BOTH)
				{
					if (!((player->mo->eflags & MFE_VERTICALFLIP && player->mo->z + player->mo->height == polysec->floorheight)
						|| (!(player->mo->eflags & MFE_VERTICALFLIP) && player->mo->z == polysec->ceilingheight)))
					{
						po = (polyobj_t *)(po->link.next);
						continue;
					}
				}
			}
			else
			{
				// Water and DEATH FOG!!! heh
				if (player->mo->z > polysec->ceilingheight || (player->mo->z + player->mo->height) < polysec->floorheight)
				{
					po = (polyobj_t *)(po->link.next);
					continue;
				}
			}

			P_ProcessSpecialSector(player, polysec, sector);
			if TELEPORTED return;

			po = (polyobj_t *)(po->link.next);
		}
	}
}

#define VDOORSPEED (FRACUNIT*2)

//
// P_RunSpecialSectorCheck
//
// Helper function to P_PlayerInSpecialSector
//
static void P_RunSpecialSectorCheck(player_t *player, sector_t *sector)
{
	boolean nofloorneeded = false;
	fixed_t f_affectpoint, c_affectpoint;

	if (!sector->special) // nothing special, exit
		return;

	if (GETSECSPECIAL(sector->special, 2) == 9) // Egg trap capsule -- should only be for 3dFloors!
		return;

	// The list of specials that activate without floor touch
	// Check Section 1
	switch(GETSECSPECIAL(sector->special, 1))
	{
		case 2: // Damage (water)
		case 8: // Instant kill
		case 10: // Ring drainer that doesn't require floor touch
		case 12: // Space countdown
			nofloorneeded = true;
			break;
	}

	// Check Section 2
	switch(GETSECSPECIAL(sector->special, 2))
	{
		case 2: // Linedef executor (All players needed)
		case 4: // Linedef executor
		case 6: // Linedef executor (7 Emeralds)
		case 7: // Linedef executor (NiGHTS Mare)
			nofloorneeded = true;
			break;
	}

	// Check Section 3
/*	switch(GETSECSPECIAL(sector->special, 3))
	{

	}*/

	// Check Section 4
	switch(GETSECSPECIAL(sector->special, 4))
	{
		case 2: // Level Exit / GOAL Sector / Flag Return
			if (!useNightsSS && G_IsSpecialStage(gamemap))
			{
				// Special stage GOAL sector
				// requires touching floor.
				break;
			}
			/* FALLTHRU */

		case 1: // Starpost activator
		case 5: // Fan sector
		case 6: // Super Sonic Transform
		case 8: // Zoom Tube Start
		case 9: // Zoom Tube End
		case 10: // Finish line
			nofloorneeded = true;
			break;
	}

	if (nofloorneeded)
	{
		P_ProcessSpecialSector(player, sector, NULL);
		return;
	}

	f_affectpoint = P_GetSpecialBottomZ(player->mo, sector, sector);
	c_affectpoint = P_GetSpecialTopZ(player->mo, sector, sector);

	// Only go further if on the ground
	if ((sector->flags & SF_FLIPSPECIAL_FLOOR) && !(sector->flags & SF_FLIPSPECIAL_CEILING) && player->mo->z != f_affectpoint)
		return;

	if ((sector->flags & SF_FLIPSPECIAL_CEILING) && !(sector->flags & SF_FLIPSPECIAL_FLOOR) && player->mo->z + player->mo->height != c_affectpoint)
		return;

	if ((sector->flags & SF_FLIPSPECIAL_BOTH)
		&& player->mo->z != f_affectpoint
		&& player->mo->z + player->mo->height != c_affectpoint)
		return;

	P_ProcessSpecialSector(player, sector, NULL);
}

/** Checks if the player is in a special sector or FOF and apply any specials.
  *
  * \param player Player to check.
  * \sa P_PlayerOnSpecial3DFloor, P_ProcessSpecialSector
  */
void P_PlayerInSpecialSector(player_t *player)
{
	sector_t *originalsector;
	sector_t *loopsector;
	msecnode_t *node;

	if (!player->mo)
		return;

	originalsector = player->mo->subsector->sector;

	P_PlayerOnSpecial3DFloor(player, originalsector); // Handle FOFs first.
	if TELEPORTED return;

	P_RunSpecialSectorCheck(player, originalsector);
	if TELEPORTED return;

	// Iterate through touching_sectorlist for SF_TRIGGERSPECIAL_TOUCH
	for (node = player->mo->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		loopsector = node->m_sector;

		if (loopsector == originalsector) // Don't duplicate
			continue;

		// Check 3D floors...
		P_PlayerOnSpecial3DFloor(player, loopsector);
		if TELEPORTED return;

		if (!(loopsector->flags & SF_TRIGGERSPECIAL_TOUCH))
			continue;

		P_RunSpecialSectorCheck(player, loopsector);
		if TELEPORTED return;
	}
}

#undef TELEPORTED

/** Animate planes, scroll walls, etc. and keeps track of level timelimit and exits if time is up.
  *
  * \sa P_CheckTimeLimit, P_CheckPointLimit
  */
void P_UpdateSpecials(void)
{
	anim_t *anim;
	INT32 i;
	INT32 pic;
	size_t j;

	levelflat_t *foundflats; // for flat animation

	// LEVEL TIMER
	P_CheckTimeLimit();

	// POINT LIMIT
	P_CheckPointLimit();

#ifdef ESLOPE
	// Dynamic slopeness
	P_RunDynamicSlopes();
#endif

	// ANIMATE TEXTURES
	for (anim = anims; anim < lastanim; anim++)
	{
		for (i = 0; i < anim->numpics; i++)
		{
			pic = anim->basepic + ((leveltime/anim->speed + i) % anim->numpics);
			if (anim->istexture)
				texturetranslation[anim->basepic+i] = pic;
		}
	}

	// ANIMATE FLATS
	/// \todo do not check the non-animate flat.. link the animated ones?
	/// \note its faster than the original anywaysince it animates only
	///    flats used in the level, and there's usually very few of them
	foundflats = levelflats;
	for (j = 0; j < numlevelflats; j++, foundflats++)
	{
		if (foundflats->speed) // it is an animated flat
		{
			// update the levelflat lump number
			foundflats->lumpnum = foundflats->baselumpnum +
				((leveltime/foundflats->speed + foundflats->animseq) % foundflats->numpics);
		}
	}
}

static inline ffloor_t *P_GetFFloorBySec(sector_t *sec, sector_t *sec2)
{
	ffloor_t *rover;

	if (!sec->ffloors)
		return NULL;
	for (rover = sec->ffloors; rover; rover = rover->next)
		if (rover->secnum == (size_t)(sec2 - sectors))
			return rover;
	return NULL;
}

/** Adds a newly formed 3Dfloor structure to a sector's ffloors list.
  *
  * \param sec    Target sector.
  * \param ffloor Newly formed 3Dfloor structure.
  * \sa P_AddFakeFloor
  */
static inline void P_AddFFloorToList(sector_t *sec, ffloor_t *ffloor)
{
	ffloor_t *rover;

	if (!sec->ffloors)
	{
		sec->ffloors = ffloor;
		ffloor->next = 0;
		ffloor->prev = 0;
		return;
	}

	for (rover = sec->ffloors; rover->next; rover = rover->next);

	rover->next = ffloor;
	ffloor->prev = rover;
	ffloor->next = 0;
}

/** Adds a 3Dfloor.
  *
  * \param sec         Target sector.
  * \param sec2        Control sector.
  * \param master      Control linedef.
  * \param flags       Options affecting this 3Dfloor.
  * \param secthinkers List of relevant thinkers sorted by sector. May be NULL.
  * \return Pointer to the new 3Dfloor.
  * \sa P_AddFFloor, P_AddFakeFloorsByLine, P_SpawnSpecials
  */
static ffloor_t *P_AddFakeFloor(sector_t *sec, sector_t *sec2, line_t *master, ffloortype_e flags, thinkerlist_t *secthinkers)
{
	ffloor_t *ffloor;
	thinker_t *th;
	friction_t *f;
	pusher_t *p;
	levelspecthink_t *lst;
	size_t sec2num;
	size_t i;

	if (sec == sec2)
		return NULL; //Don't need a fake floor on a control sector.
	if ((ffloor = (P_GetFFloorBySec(sec, sec2))))
		return ffloor; // If this ffloor already exists, return it

	if (sec2->ceilingheight < sec2->floorheight)
	{
		fixed_t tempceiling = sec2->ceilingheight;
		//flip the sector around and print an error instead of crashing 12.1.08 -Inuyasha
		CONS_Alert(CONS_ERROR, M_GetText("A FOF tagged %d has a top height below its bottom.\n"), master->tag);
		sec2->ceilingheight = sec2->floorheight;
		sec2->floorheight = tempceiling;
	}

	sec2->tagline = master;

	if (sec2->numattached == 0)
	{
		sec2->attached = Z_Malloc(sizeof (*sec2->attached) * sec2->maxattached, PU_STATIC, NULL);
		sec2->attachedsolid = Z_Malloc(sizeof (*sec2->attachedsolid) * sec2->maxattached, PU_STATIC, NULL);
		sec2->attached[0] = sec - sectors;
		sec2->numattached = 1;
		sec2->attachedsolid[0] = (flags & FF_SOLID);
	}
	else
	{
		for (i = 0; i < sec2->numattached; i++)
			if (sec2->attached[i] == (size_t)(sec - sectors))
				return NULL;

		if (sec2->numattached >= sec2->maxattached)
		{
			sec2->maxattached *= 2;
			sec2->attached = Z_Realloc(sec2->attached, sizeof (*sec2->attached) * sec2->maxattached, PU_STATIC, NULL);
			sec2->attachedsolid = Z_Realloc(sec2->attachedsolid, sizeof (*sec2->attachedsolid) * sec2->maxattached, PU_STATIC, NULL);
		}
		sec2->attached[sec2->numattached] = sec - sectors;
		sec2->attachedsolid[sec2->numattached] = (flags & FF_SOLID);
		sec2->numattached++;
	}

	// Add the floor
	ffloor = Z_Calloc(sizeof (*ffloor), PU_LEVEL, NULL);
	ffloor->secnum = sec2 - sectors;
	ffloor->target = sec;
	ffloor->bottomheight = &sec2->floorheight;
	ffloor->bottompic = &sec2->floorpic;
	ffloor->bottomxoffs = &sec2->floor_xoffs;
	ffloor->bottomyoffs = &sec2->floor_yoffs;
	ffloor->bottomangle = &sec2->floorpic_angle;

	// Add the ceiling
	ffloor->topheight = &sec2->ceilingheight;
	ffloor->toppic = &sec2->ceilingpic;
	ffloor->toplightlevel = &sec2->lightlevel;
	ffloor->topxoffs = &sec2->ceiling_xoffs;
	ffloor->topyoffs = &sec2->ceiling_yoffs;
	ffloor->topangle = &sec2->ceilingpic_angle;

#ifdef ESLOPE
	// Add slopes
	ffloor->t_slope = &sec2->c_slope;
	ffloor->b_slope = &sec2->f_slope;
#endif

	if ((flags & FF_SOLID) && (master->flags & ML_EFFECT1)) // Block player only
		flags &= ~FF_BLOCKOTHERS;

	if ((flags & FF_SOLID) && (master->flags & ML_EFFECT2)) // Block all BUT player
		flags &= ~FF_BLOCKPLAYER;

	ffloor->spawnflags = ffloor->flags = flags;
	ffloor->master = master;
	ffloor->norender = INFTICS;


	// Scan the thinkers to check for special conditions applying to this FOF.
	// If we have thinkers sorted by sector, just check the relevant ones;
	// otherwise, check them all. Apologies for the ugly loop...

	sec2num = sec2 - sectors;

	// Just initialise both of these to placate the compiler.
	i = 0;
	th = thinkercap.next;

	for(;;)
	{
		if(secthinkers)
		{
			if(i < secthinkers[sec2num].count)
				th = secthinkers[sec2num].thinkers[i];
			else break;
		}
		else if (th == &thinkercap)
			break;

		// Should this FOF have spikeness?
		if (th->function.acp1 == (actionf_p1)T_SpikeSector)
		{
			lst = (levelspecthink_t *)th;

			if (lst->sector == sec2)
				P_AddSpikeThinker(sec, (INT32)sec2num);
		}
		// Should this FOF have friction?
		else if(th->function.acp1 == (actionf_p1)T_Friction)
		{
			f = (friction_t *)th;

			if (f->affectee == (INT32)sec2num)
				Add_Friction(f->friction, f->movefactor, (INT32)(sec-sectors), f->affectee);
		}
		// Should this FOF have wind/current/pusher?
		else if(th->function.acp1 == (actionf_p1)T_Pusher)
		{
			p = (pusher_t *)th;

			if (p->affectee == (INT32)sec2num)
				Add_Pusher(p->type, p->x_mag<<FRACBITS, p->y_mag<<FRACBITS, p->source, (INT32)(sec-sectors), p->affectee, p->exclusive, p->slider);
		}

		if(secthinkers) i++;
		else th = th->next;
	}


	if (flags & FF_TRANSLUCENT)
	{
		if (sides[master->sidenum[0]].toptexture > 0)
			ffloor->alpha = sides[master->sidenum[0]].toptexture; // for future reference, "#0" is 1, and "#255" is 256. Be warned
		else
			ffloor->alpha = 0x80;
	}
	else
		ffloor->alpha = 0xff;

	ffloor->spawnalpha = ffloor->alpha; // save for netgames

	if (flags & FF_QUICKSAND)
		CheckForQuicksand = true;

	if ((flags & FF_BUSTUP) || (flags & FF_SHATTER) || (flags & FF_SPINBUST))
		CheckForBustableBlocks = true;

	if ((flags & FF_MARIO))
	{
		P_AddBlockThinker(sec2, master);
		CheckForMarioBlocks = true;
	}

	if ((flags & FF_CRUMBLE))
		sec2->crumblestate = 1;

	if ((flags & FF_FLOATBOB))
	{
		P_AddFloatThinker(sec2, sec->tag, master);
		CheckForFloatBob = true;
	}

	P_AddFFloorToList(sec, ffloor);

	return ffloor;
}

//
// SPECIAL SPAWNING
//

/** Adds a spike thinker.
  * Sector type Section1:5 will result in this effect.
  *
  * \param sec Sector in which to add the thinker.
  * \param referrer If != sec, then we're dealing with a FOF
  * \sa P_SpawnSpecials, T_SpikeSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddSpikeThinker(sector_t *sec, INT32 referrer)
{
	levelspecthink_t *spikes;

	// create and initialize new thinker
	spikes = Z_Calloc(sizeof (*spikes), PU_LEVSPEC, NULL);
	P_AddThinker(&spikes->thinker);

	spikes->thinker.function.acp1 = (actionf_p1)T_SpikeSector;

	spikes->sector = sec;
	spikes->vars[0] = referrer;
}

/** Adds a float thinker.
  * Float thinkers cause solid 3Dfloors to float on water.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \sa P_SpawnSpecials, T_FloatSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddFloatThinker(sector_t *sec, INT32 tag, line_t *sourceline)
{
	levelspecthink_t *floater;

	// create and initialize new thinker
	floater = Z_Calloc(sizeof (*floater), PU_LEVSPEC, NULL);
	P_AddThinker(&floater->thinker);

	floater->thinker.function.acp1 = (actionf_p1)T_FloatSector;

	floater->sector = sec;
	floater->vars[0] = tag;
	floater->sourceline = sourceline;
}

/** Adds a bridge thinker.
  * Bridge thinkers cause a group of FOFs to behave like
  * a bridge made up of pieces, that bows under weight.
  *
  * \param sec          Control sector.
  * \sa P_SpawnSpecials, T_BridgeThinker
  * \author SSNTails <http://www.ssntails.org>
  */
/*
static inline void P_AddBridgeThinker(line_t *sourceline, sector_t *sec)
{
	levelspecthink_t *bridge;

	// create an initialize new thinker
	bridge = Z_Calloc(sizeof (*bridge), PU_LEVSPEC, NULL);
	P_AddThinker(&bridge->thinker);

	bridge->thinker.function.acp1 = (actionf_p1)T_BridgeThinker;

	bridge->sector = sec;
	bridge->vars[0] = sourceline->frontsector->floorheight;
	bridge->vars[1] = sourceline->frontsector->ceilingheight;
	bridge->vars[2] = P_AproxDistance(sourceline->dx, sourceline->dy); // Speed
	bridge->vars[2] = FixedDiv(bridge->vars[2], 16*FRACUNIT);
	bridge->vars[3] = bridge->vars[2];

	// Start tag and end tag are TARGET SECTORS, not CONTROL SECTORS
	// Control sector tags should be End_Tag + (End_Tag - Start_Tag)
	bridge->vars[4] = sourceline->tag; // Start tag
	bridge->vars[5] = (sides[sourceline->sidenum[0]].textureoffset>>FRACBITS); // End tag
}
*/

/** Adds a Mario block thinker, which changes the block's texture between blank
  * and ? depending on whether it has contents.
  * Needed in case objects respawn inside.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_MarioBlockChecker
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline)
{
	levelspecthink_t *block;

	// create and initialize new elevator thinker
	block = Z_Calloc(sizeof (*block), PU_LEVSPEC, NULL);
	P_AddThinker(&block->thinker);

	block->thinker.function.acp1 = (actionf_p1)T_MarioBlockChecker;
	block->sourceline = sourceline;

	block->sector = sec;
}

/** Adds a raise thinker.
  * A raise thinker checks to see if the
  * player is standing on its 3D Floor,
  * and if so, raises the platform towards
  * it's destination. Otherwise, it lowers
  * to the lowest nearby height if not
  * there already.
  *
  * Replaces the old "AirBob".
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_RaiseSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddRaiseThinker(sector_t *sec, line_t *sourceline)
{
	levelspecthink_t *raise;

	raise = Z_Calloc(sizeof (*raise), PU_LEVSPEC, NULL);
	P_AddThinker(&raise->thinker);

	raise->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	if (sourceline->flags & ML_BLOCKMONSTERS)
		raise->vars[0] = 1;
	else
		raise->vars[0] = 0;

	// set up the fields
	raise->sector = sec;

	// Require a spindash to activate
	if (sourceline->flags & ML_NOCLIMB)
		raise->vars[1] = 1;
	else
		raise->vars[1] = 0;

	raise->vars[2] = P_AproxDistance(sourceline->dx, sourceline->dy);
	raise->vars[2] = FixedDiv(raise->vars[2], 4*FRACUNIT);
	raise->vars[3] = raise->vars[2];

	raise->vars[5] = P_FindHighestCeilingSurrounding(sec);
	raise->vars[4] = raise->vars[5]
		- (sec->ceilingheight - sec->floorheight);

	raise->vars[7] = P_FindLowestCeilingSurrounding(sec);
	raise->vars[6] = raise->vars[7]
		- (sec->ceilingheight - sec->floorheight);

	raise->sourceline = sourceline;
}

// Function to maintain backwards compatibility
static void P_AddOldAirbob(sector_t *sec, line_t *sourceline, boolean noadjust)
{
	levelspecthink_t *airbob;

	airbob = Z_Calloc(sizeof (*airbob), PU_LEVSPEC, NULL);
	P_AddThinker(&airbob->thinker);

	airbob->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	// set up the fields
	airbob->sector = sec;

	// Require a spindash to activate
	if (sourceline->flags & ML_NOCLIMB)
		airbob->vars[1] = 1;
	else
		airbob->vars[1] = 0;

	airbob->vars[2] = FRACUNIT;

	if (noadjust)
		airbob->vars[7] = airbob->sector->ceilingheight-16*FRACUNIT;
	else
		airbob->vars[7] = airbob->sector->ceilingheight - P_AproxDistance(sourceline->dx, sourceline->dy);
	airbob->vars[6] = airbob->vars[7]
		- (sec->ceilingheight - sec->floorheight);

	airbob->vars[3] = airbob->vars[2];

	if (sourceline->flags & ML_BLOCKMONSTERS)
		airbob->vars[0] = 1;
	else
		airbob->vars[0] = 0;

	airbob->vars[5] = sec->ceilingheight;
	airbob->vars[4] = airbob->vars[5]
			- (sec->ceilingheight - sec->floorheight);

	airbob->sourceline = sourceline;
}

/** Adds a thwomp thinker.
  * Even thwomps need to think!
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_ThwompSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddThwompThinker(sector_t *sec, sector_t *actionsector, line_t *sourceline)
{
#define speed vars[1]
#define direction vars[2]
#define distance vars[3]
#define floorwasheight vars[4]
#define ceilingwasheight vars[5]
	levelspecthink_t *thwomp;

	// You *probably* already have a thwomp in this sector. If you've combined it with something
	// else that uses the floordata/ceilingdata, you must be weird.
	if (sec->floordata || sec->ceilingdata)
		return;

	// create and initialize new elevator thinker
	thwomp = Z_Calloc(sizeof (*thwomp), PU_LEVSPEC, NULL);
	P_AddThinker(&thwomp->thinker);

	thwomp->thinker.function.acp1 = (actionf_p1)T_ThwompSector;

	// set up the fields according to the type of elevator action
	thwomp->sector = sec;
	thwomp->vars[0] = actionsector->tag;
	thwomp->floorwasheight = thwomp->sector->floorheight;
	thwomp->ceilingwasheight = thwomp->sector->ceilingheight;
	thwomp->direction = 0;
	thwomp->distance = 1;
	thwomp->sourceline = sourceline;
	thwomp->sector->floordata = thwomp;
	thwomp->sector->ceilingdata = thwomp;
	return;
#undef speed
#undef direction
#undef distance
#undef floorwasheight
#undef ceilingwasheight
}

/** Adds a thinker which checks if any MF_ENEMY objects with health are in the defined area.
  * If not, a linedef executor is run once.
  *
  * \param sec          Control sector.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_NoEnemiesSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddNoEnemiesThinker(sector_t *sec, line_t *sourceline)
{
	levelspecthink_t *nobaddies;

	// create and initialize new thinker
	nobaddies = Z_Calloc(sizeof (*nobaddies), PU_LEVSPEC, NULL);
	P_AddThinker(&nobaddies->thinker);

	nobaddies->thinker.function.acp1 = (actionf_p1)T_NoEnemiesSector;

	nobaddies->sector = sec;
	nobaddies->sourceline = sourceline;
}

/** Adds a thinker for Each-Time linedef executors. A linedef executor is run
  * only when a player enters the area and doesn't run again until they re-enter.
  *
  * \param sec          Control sector that contains the lines of executors we will want to run.
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_EachTimeThinker
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddEachTimeThinker(sector_t *sec, line_t *sourceline)
{
	levelspecthink_t *eachtime;

	// create and initialize new thinker
	eachtime = Z_Calloc(sizeof (*eachtime), PU_LEVSPEC, NULL);
	P_AddThinker(&eachtime->thinker);

	eachtime->thinker.function.acp1 = (actionf_p1)T_EachTimeThinker;

	eachtime->sector = sec;
	eachtime->sourceline = sourceline;
}

/** Adds a camera scanner.
  *
  * \param sourcesec    Control sector.
  * \param actionsector Target sector.
  * \param angle        Angle of the source line.
  * \sa P_SpawnSpecials, T_CameraScanner
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddCameraScanner(sector_t *sourcesec, sector_t *actionsector, angle_t angle)
{
	elevator_t *elevator; // Why not? LOL

	// create and initialize new elevator thinker
	elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
	P_AddThinker(&elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_CameraScanner;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sourcesec;
	elevator->actionsector = actionsector;
	elevator->distance = FixedInt(AngleFixed(angle));
}

static const ffloortype_e laserflags = FF_EXISTS|FF_RENDERALL|FF_NOSHADE|FF_EXTRA|FF_CUTEXTRA;

/** Flashes a laser block.
  *
  * \param flash Thinker structure for this laser.
  * \sa EV_AddLaserThinker
  * \author SSNTails <http://www.ssntails.org>
  */
void T_LaserFlash(laserthink_t *flash)
{
	msecnode_t *node;
	mobj_t *thing;
	sector_t *sourcesec;
	ffloor_t *ffloor = flash->ffloor;
	sector_t *sector = flash->sector;
	fixed_t top, bottom;

	if (!ffloor || !(ffloor->flags & FF_EXISTS))
		return;

	if (leveltime & 1)
		ffloor->flags |= FF_RENDERALL;
	else
		ffloor->flags &= ~FF_RENDERALL;

	sourcesec = ffloor->master->frontsector; // Less to type!

#ifdef ESLOPE
	top = (*ffloor->t_slope) ? P_GetZAt(*ffloor->t_slope, sector->soundorg.x, sector->soundorg.y)
			: *ffloor->topheight;
	bottom = (*ffloor->b_slope) ? P_GetZAt(*ffloor->b_slope, sector->soundorg.x, sector->soundorg.y)
			: *ffloor->bottomheight;
	sector->soundorg.z = (top + bottom)/2;
#else
	sector->soundorg.z = (*ffloor->topheight + *ffloor->bottomheight)/2;
#endif
	S_StartSound(&sector->soundorg, sfx_laser);

	// Seek out objects to DESTROY! MUAHAHHAHAHAA!!!*cough*
	for (node = sector->touching_thinglist; node && node->m_thing; node = node->m_thinglist_next)
	{
		thing = node->m_thing;

		if ((ffloor->master->flags & ML_EFFECT1)
			&& thing->flags & MF_BOSS)
			continue; // Don't hurt bosses

		// Don't endlessly kill egg guard shields (or anything else for that matter)
		if (thing->health <= 0)
			continue;

		top = P_GetSpecialTopZ(thing, sourcesec, sector);
		bottom = P_GetSpecialBottomZ(thing, sourcesec, sector);

		if (thing->z >= top
		|| thing->z + thing->height <= bottom)
			continue;

		if (thing->flags & MF_SHOOTABLE)
			P_DamageMobj(thing, NULL, NULL, 1);
		else if (thing->type == MT_EGGSHIELD)
			P_KillMobj(thing, NULL, NULL);
	}
}

/** Adds a laser thinker to a 3Dfloor.
  *
  * \param ffloor      3Dfloor to turn into a laser block.
  * \param sector      Target sector.
  * \param secthkiners Lists of thinkers sorted by sector. May be NULL.
  * \sa T_LaserFlash
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void EV_AddLaserThinker(sector_t *sec, sector_t *sec2, line_t *line, thinkerlist_t *secthinkers)
{
	laserthink_t *flash;
	ffloor_t *ffloor = P_AddFakeFloor(sec, sec2, line, laserflags, secthinkers);

	if (!ffloor)
		return;

	flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(&flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LaserFlash;
	flash->ffloor = ffloor;
	flash->sector = sec; // For finding mobjs
	flash->sec = sec2;
	flash->sourceline = line;
}

//
// P_RunLevelLoadExecutors
//
// After loading/spawning all other specials
// and items, execute these.
//
static void P_RunLevelLoadExecutors(void)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 399)
			P_RunTriggerLinedef(&lines[i], NULL, NULL);
	}
}

/** After the map has loaded, scans for specials that spawn 3Dfloors and
  * thinkers.
  *
  * \todo Split up into multiple functions.
  * \todo Get rid of all the magic numbers.
  * \todo Potentially use 'fromnetsave' to stop any new thinkers from being created
  *       as they'll just be erased by UnArchiveThinkers.
  * \sa P_SpawnPrecipitation, P_SpawnFriction, P_SpawnPushers, P_SpawnScrollers
  */
void P_SpawnSpecials(INT32 fromnetsave)
{
	sector_t *sector;
	size_t i;
	INT32 j;
	thinkerlist_t *secthinkers;
	thinker_t *th;

	// This used to be used, and *should* be used in the future,
	// but currently isn't.
	(void)fromnetsave;

	// Set the default gravity. Custom gravity overrides this setting.
	gravity = FRACUNIT/2;

	// Defaults in case levels don't have them set.
	sstimer = 90*TICRATE + 6;
	totalrings = 1;

	CheckForBustableBlocks = CheckForBouncySector = CheckForQuicksand = CheckForMarioBlocks = CheckForFloatBob = CheckForReverseGravity = false;

	// Init special SECTORs.
	sector = sectors;
	for (i = 0; i < numsectors; i++, sector++)
	{
		if (!sector->special)
			continue;

		// Process Section 1
		switch(GETSECSPECIAL(sector->special, 1))
		{
			case 5: // Spikes
				P_AddSpikeThinker(sector, (INT32)(sector-sectors));
				break;

			case 15: // Bouncy sector
				CheckForBouncySector = true;
				break;
		}

		// Process Section 2
		switch(GETSECSPECIAL(sector->special, 2))
		{
			case 10: // Time for special stage
				sstimer = (sector->floorheight>>FRACBITS) * TICRATE + 6; // Time to finish
				totalrings = sector->ceilingheight>>FRACBITS; // Ring count for special stage
				break;

			case 11: // Custom global gravity!
				gravity = sector->floorheight/1000;
				break;
		}

		// Process Section 3
/*		switch(GETSECSPECIAL(player->specialsector, 3))
		{

		}*/

		// Process Section 4
		switch(GETSECSPECIAL(sector->special, 4))
		{
			case 10: // Circuit finish line
				if (gametype == GT_RACE)
					circuitmap = true;
				break;
		}
	}

	if (mapheaderinfo[gamemap-1]->weather == 2) // snow
		curWeather = PRECIP_SNOW;
	else if (mapheaderinfo[gamemap-1]->weather == 3) // rain
		curWeather = PRECIP_RAIN;
	else if (mapheaderinfo[gamemap-1]->weather == 1) // storm
		curWeather = PRECIP_STORM;
	else if (mapheaderinfo[gamemap-1]->weather == 5) // storm w/o rain
		curWeather = PRECIP_STORM_NORAIN;
	else if (mapheaderinfo[gamemap-1]->weather == 6) // storm w/o lightning
		curWeather = PRECIP_STORM_NOSTRIKES;
	else
		curWeather = PRECIP_NONE;

	P_InitTagLists();   // Create xref tables for tags
	P_SearchForDisableLinedefs(); // Disable linedefs are now allowed to disable *any* line

	P_SpawnScrollers(); // Add generalized scrollers
	P_SpawnFriction();  // Friction model using linedefs
	P_SpawnPushers();   // Pusher model using linedefs

	// Look for thinkers that affect FOFs, and sort them by sector

	secthinkers = Z_Calloc(numsectors * sizeof(thinkerlist_t), PU_STATIC, NULL);

	// Firstly, find out how many there are in each sector
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)T_SpikeSector)
			secthinkers[((levelspecthink_t *)th)->sector - sectors].count++;
		else if (th->function.acp1 == (actionf_p1)T_Friction)
			secthinkers[((friction_t *)th)->affectee].count++;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secthinkers[((pusher_t *)th)->affectee].count++;
	}

	// Allocate each list, and then zero the count so we can use it to track
	// the end of the list as we add the thinkers
	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].count > 0)
		{
			secthinkers[i].thinkers = Z_Malloc(secthinkers[i].count * sizeof(thinker_t *), PU_STATIC, NULL);
			secthinkers[i].count = 0;
		}

	// Finally, populate the lists.
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		size_t secnum = (size_t)-1;

		if (th->function.acp1 == (actionf_p1)T_SpikeSector)
			secnum = ((levelspecthink_t *)th)->sector - sectors;
		else if (th->function.acp1 == (actionf_p1)T_Friction)
			secnum = ((friction_t *)th)->affectee;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secnum = ((pusher_t *)th)->affectee;

		if (secnum != (size_t)-1)
			secthinkers[secnum].thinkers[secthinkers[secnum].count++] = th;
	}


	// Init line EFFECTs
	for (i = 0; i < numlines; i++)
	{
		// set line specials to 0 here too, same reason as above
		if (netgame || multiplayer)
		{
			// future: nonet flag?
		}
		else if ((lines[i].flags & ML_NETONLY) == ML_NETONLY)
		{
			lines[i].special = 0;
			continue;
		}
		else
		{
			if (players[consoleplayer].charability == CA_THOK && (lines[i].flags & ML_NOSONIC))
			{
				lines[i].special = 0;
				continue;
			}
			if (players[consoleplayer].charability == CA_FLY && (lines[i].flags & ML_NOTAILS))
			{
				lines[i].special = 0;
				continue;
			}
			if (players[consoleplayer].charability == CA_GLIDEANDCLIMB && (lines[i].flags & ML_NOKNUX))
			{
				lines[i].special = 0;
				continue;
			}
		}

		switch (lines[i].special)
		{
			INT32 s;
			size_t sec;
			ffloortype_e ffloorflags;

			case 1: // Definable gravity per sector
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
				{
					sectors[s].gravity = &sectors[sec].floorheight; // This allows it to change in realtime!

					if (lines[i].flags & ML_NOCLIMB)
						sectors[s].verticalflip = true;
					else
						sectors[s].verticalflip = false;

					CheckForReverseGravity = sectors[s].verticalflip;
				}
				break;

			case 2: // Custom exit
				break;

			case 3: // Zoom Tube Parameters
				break;

			case 4: // Speed pad (combines with sector special Section3:5 or Section3:6)
				break;

			case 5: // Change camera info
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					P_AddCameraScanner(&sectors[sec], &sectors[s], R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y));
				break;

#ifdef PARANOIA
			case 6: // Disable tags if level not cleared
				I_Error("Failed to catch a disable linedef");
				break;
#endif

			case 7: // Flat alignment
				if (lines[i].flags & ML_EFFECT4) // Align angle
				{
					if (!(lines[i].flags & ML_EFFECT5)) // Align floor unless ALLTRIGGER flag is set
					{
						for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
							sectors[s].spawn_flrpic_angle = sectors[s].floorpic_angle = R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y);
					}

					if (!(lines[i].flags & ML_BOUNCY)) // Align ceiling unless BOUNCY flag is set
					{
						for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
							sectors[s].spawn_ceilpic_angle = sectors[s].ceilingpic_angle = R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y);
					}
				}
				else // Do offsets
				{
					if (!(lines[i].flags & ML_BLOCKMONSTERS)) // Align floor unless BLOCKMONSTERS flag is set
					{
						for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
						{
							sectors[s].floor_xoffs += lines[i].dx;
							sectors[s].floor_yoffs += lines[i].dy;
							// saved for netgames
							sectors[s].spawn_flr_xoffs = sectors[s].floor_xoffs;
							sectors[s].spawn_flr_yoffs = sectors[s].floor_yoffs;
						}
					}

					if (!(lines[i].flags & ML_NOCLIMB)) // Align ceiling unless NOCLIMB flag is set
					{
						for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
						{
							sectors[s].ceiling_xoffs += lines[i].dx;
							sectors[s].ceiling_yoffs += lines[i].dy;
							// saved for netgames
							sectors[s].spawn_ceil_xoffs = sectors[s].ceiling_xoffs;
							sectors[s].spawn_ceil_yoffs = sectors[s].ceiling_yoffs;
						}
					}
				}
				break;

			case 8: // Sector Parameters
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
				{
					if (lines[i].flags & ML_NOCLIMB)
					{
						sectors[s].flags &= ~SF_FLIPSPECIAL_FLOOR;
						sectors[s].flags |= SF_FLIPSPECIAL_CEILING;
					}
					else if (lines[i].flags & ML_EFFECT4)
						sectors[s].flags |= SF_FLIPSPECIAL_BOTH;

					if (lines[i].flags & ML_EFFECT3)
						sectors[s].flags |= SF_TRIGGERSPECIAL_TOUCH;

					if (lines[i].frontsector && GETSECSPECIAL(lines[i].frontsector->special, 4) == 12)
						sectors[s].camsec = sides[*lines[i].sidenum].sector-sectors;
				}
				break;

			case 9: // Chain Parameters
				break;

			case 10: // Vertical culling plane for sprites and FOFs
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					sectors[s].cullheight = &lines[i]; // This allows it to change in realtime!
				break;

			case 50: // Insta-Lower Sector
				EV_DoFloor(&lines[i], instantLower);
				break;

			case 51: // Instant raise for ceilings
				EV_DoCeiling(&lines[i], instantRaise);
				break;

			case 52: // Continuously Falling sector
				EV_DoContinuousFall(lines[i].frontsector, lines[i].backsector, P_AproxDistance(lines[i].dx, lines[i].dy), (lines[i].flags & ML_NOCLIMB));
				break;

			case 53: // New super cool and awesome moving floor and ceiling type
			case 54: // New super cool and awesome moving floor type
				if (lines[i].backsector)
					EV_DoFloor(&lines[i], bounceFloor);
				if (lines[i].special == 54)
					break;
				/* FALLTHRU */

			case 55: // New super cool and awesome moving ceiling type
				if (lines[i].backsector)
					EV_DoCeiling(&lines[i], bounceCeiling);
				break;

			case 56: // New super cool and awesome moving floor and ceiling crush type
			case 57: // New super cool and awesome moving floor crush type
				if (lines[i].backsector)
					EV_DoFloor(&lines[i], bounceFloorCrush);

				if (lines[i].special == 57)
					break; //only move the floor
				/* FALLTHRU */

			case 58: // New super cool and awesome moving ceiling crush type
				if (lines[i].backsector)
					EV_DoCeiling(&lines[i], bounceCeilingCrush);
				break;

			case 59: // Activate floating platform
				EV_DoElevator(&lines[i], elevateContinuous, false);
				break;

			case 60: // Floating platform with adjustable speed
				EV_DoElevator(&lines[i], elevateContinuous, true);
				break;

			case 61: // Crusher!
				EV_DoCrush(&lines[i], crushAndRaise);
				break;

			case 62: // Crusher (up and then down)!
				EV_DoCrush(&lines[i], fastCrushAndRaise);
				break;

			case 63: // support for drawn heights coming from different sector
				sec = sides[*lines[i].sidenum].sector-sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					sectors[s].heightsec = (INT32)sec;
				break;

			case 64: // Appearing/Disappearing FOF option
				if (lines[i].flags & ML_BLOCKMONSTERS) { // Find FOFs by control sector tag
					for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
						for (j = 0; (unsigned)j < sectors[s].linecount; j++)
							if (sectors[s].lines[j]->special >= 100 && sectors[s].lines[j]->special < 300)
								Add_MasterDisappearer(abs(lines[i].dx>>FRACBITS), abs(lines[i].dy>>FRACBITS), abs(sides[lines[i].sidenum[0]].sector->floorheight>>FRACBITS), (INT32)(sectors[s].lines[j]-lines), (INT32)i);
				} else // Find FOFs by effect sector tag
					for (s = -1; (s = P_FindLineFromLineTag(lines + i, s)) >= 0 ;)
					{
						if ((size_t)s == i)
							continue;
						if (sides[lines[s].sidenum[0]].sector->tag == sides[lines[i].sidenum[0]].sector->tag)
							Add_MasterDisappearer(abs(lines[i].dx>>FRACBITS), abs(lines[i].dy>>FRACBITS), abs(sides[lines[i].sidenum[0]].sector->floorheight>>FRACBITS), s, (INT32)i);
					}
				break;

			case 65: // Bridge Thinker
				/*
				// Disable this until it's working right!
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					P_AddBridgeThinker(&lines[i], &sectors[s]);*/
				break;

			case 100: // FOF (solid, opaque, shadows)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				break;

			case 101: // FOF (solid, opaque, no shadows)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_CUTLEVEL, secthinkers);
				break;

			case 102: // TL block: FOF (solid, translucent)
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA;

				// Draw the 'insides' of the block too
				if (lines[i].flags & ML_NOCLIMB)
				{
					ffloorflags |= FF_CUTLEVEL;
					ffloorflags |= FF_BOTHPLANES;
					ffloorflags |= FF_ALLSIDES;
					ffloorflags &= ~FF_EXTRA;
					ffloorflags &= ~FF_CUTEXTRA;
				}

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 103: // Solid FOF with no floor/ceiling (quite possibly useless)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERSIDES|FF_NOSHADE|FF_CUTLEVEL, secthinkers);
				break;

			case 104: // 3D Floor type that doesn't draw sides
				// If line has no-climb set, give it shadows, otherwise don't
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERPLANES|FF_CUTLEVEL;
				if (!(lines[i].flags & ML_NOCLIMB))
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 105: // FOF (solid, invisible)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_NOSHADE, secthinkers);
				break;

			case 120: // Opaque water
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_SWIMMABLE|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_DOUBLESHADOW;
				if (lines[i].flags & ML_EFFECT4)
					ffloorflags |= FF_COLORMAPONLY;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 121: // TL water
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_DOUBLESHADOW;
				if (lines[i].flags & ML_EFFECT4)
					ffloorflags |= FF_COLORMAPONLY;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 122: // Opaque water, no sides
				ffloorflags = FF_EXISTS|FF_RENDERPLANES|FF_SWIMMABLE|FF_BOTHPLANES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_DOUBLESHADOW;
				if (lines[i].flags & ML_EFFECT4)
					ffloorflags |= FF_COLORMAPONLY;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 123: // TL water, no sides
				ffloorflags = FF_EXISTS|FF_RENDERPLANES|FF_TRANSLUCENT|FF_SWIMMABLE|FF_BOTHPLANES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_DOUBLESHADOW;
				if (lines[i].flags & ML_EFFECT4)
					ffloorflags |= FF_COLORMAPONLY;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 124: // goo water
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_TRANSLUCENT|FF_SWIMMABLE|FF_GOOWATER|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_DOUBLESHADOW;
				if (lines[i].flags & ML_EFFECT4)
					ffloorflags |= FF_COLORMAPONLY;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 125: // goo water, no sides
				ffloorflags = FF_EXISTS|FF_RENDERPLANES|FF_TRANSLUCENT|FF_SWIMMABLE|FF_GOOWATER|FF_BOTHPLANES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_DOUBLESHADOW;
				if (lines[i].flags & ML_EFFECT4)
					ffloorflags |= FF_COLORMAPONLY;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 140: // 'Platform' - You can jump up through it
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_PLATFORM|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 141: // Translucent "platform"
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_PLATFORM|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				// Draw the 'insides' of the block too
				if (lines[i].flags & ML_EFFECT2)
				{
					ffloorflags |= FF_CUTLEVEL;
					ffloorflags |= FF_BOTHPLANES;
					ffloorflags |= FF_ALLSIDES;
					ffloorflags &= ~FF_EXTRA;
					ffloorflags &= ~FF_CUTEXTRA;
				}

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 142: // Translucent "platform" with no sides
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERPLANES|FF_TRANSLUCENT|FF_PLATFORM|FF_EXTRA|FF_CUTEXTRA;
				if (lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				// Draw the 'insides' of the block too
				if (lines[i].flags & ML_EFFECT2)
				{
					ffloorflags |= FF_CUTLEVEL;
					ffloorflags |= FF_BOTHPLANES;
					ffloorflags |= FF_ALLSIDES;
					ffloorflags &= ~FF_EXTRA;
					ffloorflags &= ~FF_CUTEXTRA;
				}

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 143: // 'Reverse platform' - You fall through it
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_REVERSEPLATFORM|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 144: // Translucent "reverse platform"
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_REVERSEPLATFORM|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				// Draw the 'insides' of the block too
				if (lines[i].flags & ML_EFFECT2)
				{
					ffloorflags |= FF_CUTLEVEL;
					ffloorflags |= FF_BOTHPLANES;
					ffloorflags |= FF_ALLSIDES;
					ffloorflags &= ~FF_EXTRA;
					ffloorflags &= ~FF_CUTEXTRA;
				}

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 145: // Translucent "reverse platform" with no sides
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERPLANES|FF_TRANSLUCENT|FF_REVERSEPLATFORM|FF_EXTRA|FF_CUTEXTRA;
				if (lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				// Draw the 'insides' of the block too
				if (lines[i].flags & ML_EFFECT2)
				{
					ffloorflags |= FF_CUTLEVEL;
					ffloorflags |= FF_BOTHPLANES;
					ffloorflags |= FF_ALLSIDES;
					ffloorflags &= ~FF_EXTRA;
					ffloorflags &= ~FF_CUTEXTRA;
				}

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 146: // Intangible floor/ceiling with solid sides (fences/hoops maybe?)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERSIDES|FF_ALLSIDES|FF_INTANGABLEFLATS, secthinkers);
				break;

			case 150: // Air bobbing platform
			case 151: // Adjustable air bobbing platform
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				lines[i].flags |= ML_BLOCKMONSTERS;
				P_AddOldAirbob(lines[i].frontsector, lines + i, (lines[i].special != 151));
				break;
			case 152: // Adjustable air bobbing platform in reverse
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				P_AddOldAirbob(lines[i].frontsector, lines + i, true);
				break;

			case 160: // Float/bob platform
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_FLOATBOB, secthinkers);
				break;

			case 170: // Crumbling platform
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE, secthinkers);
				break;

			case 171: // Crumbling platform that will not return
				P_AddFakeFloorsByLine(i,
					FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE|FF_NORETURN, secthinkers);
				break;

			case 172: // "Platform" that crumbles and returns
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_PLATFORM|FF_CRUMBLE|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 173: // "Platform" that crumbles and doesn't return
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_PLATFORM|FF_CRUMBLE|FF_NORETURN|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 174: // Translucent "platform" that crumbles and returns
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_CRUMBLE|FF_TRANSLUCENT|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 175: // Translucent "platform" that crumbles and doesn't return
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_PLATFORM|FF_CRUMBLE|FF_NORETURN|FF_TRANSLUCENT|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB) // shade it unless no-climb
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 176: // Air bobbing platform that will crumble and bob on the water when it falls and hits
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_FLOATBOB|FF_CRUMBLE, secthinkers);
				lines[i].flags |= ML_BLOCKMONSTERS;
				P_AddOldAirbob(lines[i].frontsector, lines + i, true);
				break;

			case 177: // Air bobbing platform that will crumble and bob on
				// the water when it falls and hits, then never return
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_FLOATBOB|FF_CRUMBLE|FF_NORETURN, secthinkers);
				lines[i].flags |= ML_BLOCKMONSTERS;
				P_AddOldAirbob(lines[i].frontsector, lines + i, true);
				break;

			case 178: // Crumbling platform that will float when it hits water
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CRUMBLE|FF_FLOATBOB, secthinkers);
				break;

			case 179: // Crumbling platform that will float when it hits water, but not return
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE|FF_FLOATBOB|FF_NORETURN, secthinkers);
				break;

			case 180: // Air bobbing platform that will crumble
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE, secthinkers);
				lines[i].flags |= ML_BLOCKMONSTERS;
				P_AddOldAirbob(lines[i].frontsector, lines + i, true);
				break;

			case 190: // Rising Platform FOF (solid, opaque, shadows)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, &lines[i]);
				break;

			case 191: // Rising Platform FOF (solid, opaque, no shadows)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_CUTLEVEL, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, &lines[i]);
				break;

			case 192: // Rising Platform TL block: FOF (solid, translucent)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_NOSHADE|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, &lines[i]);
				break;

			case 193: // Rising Platform FOF (solid, invisible)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_NOSHADE, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, &lines[i]);
				break;

			case 194: // Rising Platform 'Platform' - You can jump up through it
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_PLATFORM|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, &lines[i]);
				break;

			case 195: // Rising Platform Translucent "platform"
				// If line has no-climb set, don't give it shadows, otherwise do
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_PLATFORM|FF_TRANSLUCENT|FF_BOTHPLANES|FF_ALLSIDES|FF_EXTRA|FF_CUTEXTRA;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				P_AddRaiseThinker(lines[i].frontsector, &lines[i]);
				break;

			case 200: // Double light effect
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_CUTSPRITES|FF_DOUBLESHADOW, secthinkers);
				break;

			case 201: // Light effect
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_CUTSPRITES, secthinkers);
				break;

			case 202: // Fog
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_FOG|FF_BOTHPLANES|FF_INVERTPLANES|FF_ALLSIDES|FF_INVERTSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
				sec = sides[*lines[i].sidenum].sector - sectors;
				// SoM: Because it's fog, check for an extra colormap and set
				// the fog flag...
				if (sectors[sec].extra_colormap)
					sectors[sec].extra_colormap->fog = 1;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 220: // Like opaque water, but not swimmable. (Good for snow effect on FOFs)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_BOTHPLANES|FF_ALLSIDES|FF_CUTEXTRA|FF_EXTRA|FF_CUTSPRITES, secthinkers);
				break;

			case 221: // FOF (intangible, translucent)
				// If line has no-climb set, give it shadows, otherwise don't
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA|FF_CUTSPRITES;
				if (!(lines[i].flags & ML_NOCLIMB))
					ffloorflags |= FF_NOSHADE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 222: // FOF with no floor/ceiling (good for GFZGRASS effect on FOFs)
				// If line has no-climb set, give it shadows, otherwise don't
				ffloorflags = FF_EXISTS|FF_RENDERSIDES|FF_ALLSIDES;
				if (!(lines[i].flags & ML_NOCLIMB))
					ffloorflags |= FF_NOSHADE|FF_CUTSPRITES;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 223: // FOF (intangible, invisible) - for combining specials in a sector
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_NOSHADE, secthinkers);
				break;

			case 250: // Mario Block
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_MARIO, secthinkers);
				break;

			case 251: // A THWOMP!
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
				{
					P_AddThwompThinker(&sectors[sec], &sectors[s], &lines[i]);
					P_AddFakeFloor(&sectors[s], &sectors[sec], lines + i,
						FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				}
				break;

			case 252: // Shatter block (breaks when touched)
				if (lines[i].flags & ML_NOCLIMB)
					P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SHATTER|FF_SHATTERBOTTOM, secthinkers);
				else
					P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_BUSTUP|FF_SHATTER, secthinkers);
				break;

			case 253: // Translucent shatter block (see 76)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_BUSTUP|FF_SHATTER|FF_TRANSLUCENT, secthinkers);
				break;

			case 254: // Bustable block
				if (lines[i].flags & ML_NOCLIMB)
					P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_ONLYKNUX, secthinkers);
				else
					P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP, secthinkers);
				break;

			case 255: // Spin bust block (breaks when jumped or spun downwards onto)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SPINBUST, secthinkers);
				break;

			case 256: // Translucent spin bust block (see 78)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SPINBUST|FF_TRANSLUCENT, secthinkers);
				break;

			case 257: // Quicksand
				if (lines[i].flags & ML_EFFECT5)
					P_AddFakeFloorsByLine(i, FF_EXISTS|FF_QUICKSAND|FF_RENDERALL|FF_ALLSIDES|FF_CUTSPRITES|FF_RIPPLE, secthinkers);
				else
					P_AddFakeFloorsByLine(i, FF_EXISTS|FF_QUICKSAND|FF_RENDERALL|FF_ALLSIDES|FF_CUTSPRITES, secthinkers);
				break;

			case 258: // Laser block
				sec = sides[*lines[i].sidenum].sector - sectors;

				// No longer totally disrupts netgames
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					EV_AddLaserThinker(&sectors[s], &sectors[sec], lines + i, secthinkers);
				break;

			case 259: // Make-Your-Own FOF!
				if (lines[i].sidenum[1] != 0xffff)
				{
					UINT8 *data = W_CacheLumpNum(lastloadedmaplumpnum + ML_SIDEDEFS,PU_STATIC);
					UINT16 b;

					for (b = 0; b < (INT16)numsides; b++)
					{
						register mapsidedef_t *msd = (mapsidedef_t *)data + b;

						if (b == lines[i].sidenum[1])
						{
							if ((msd->toptexture[0] >= '0' && msd->toptexture[0] <= '9')
								|| (msd->toptexture[0] >= 'A' && msd->toptexture[0] <= 'F'))
							{
								ffloortype_e FOF_Flags = axtoi(msd->toptexture);

								P_AddFakeFloorsByLine(i, FOF_Flags, secthinkers);
								break;
							}
							else
								I_Error("Make-Your-Own-FOF (tag %d) needs a value in the linedef's second side upper texture field.", lines[i].tag);
						}
					}
					Z_Free(data);
				}
				else
					I_Error("Make-Your-Own FOF (tag %d) found without a 2nd linedef side!", lines[i].tag);
				break;

			case 300: // Linedef executor (combines with sector special 974/975) and commands
			case 302:
			case 303:
			case 304:

			// Charability linedef executors
			case 305:
			case 307:
				break;

			case 308: // Race-only linedef executor. Triggers once.
				if (gametype != GT_RACE && gametype != GT_COMPETITION)
					lines[i].special = 0;
				break;

			// Linedef executor triggers for CTF teams.
			case 309:
			case 311:
				if (gametype != GT_CTF)
					lines[i].special = 0;
				break;

			// Each time executors
			case 306:
			case 301:
			case 310:
			case 312:
				sec = sides[*lines[i].sidenum].sector - sectors;
				P_AddEachTimeThinker(&sectors[sec], &lines[i]);
				break;

			// No More Enemies Linedef Exec
			case 313:
				sec = sides[*lines[i].sidenum].sector - sectors;
				P_AddNoEnemiesThinker(&sectors[sec], &lines[i]);
				break;

			// Pushable linedef executors (count # of pushables)
			case 314:
			case 315:
				break;

			// Unlock trigger executors
			case 317:
			case 318:
				break;
			case 319:
			case 320:
				break;

			// Trigger on X calls
			case 321:
			case 322:
				if (lines[i].flags & ML_NOCLIMB && sides[lines[i].sidenum[0]].rowoffset > 0) // optional "starting" count
					lines[i].callcount = sides[lines[i].sidenum[0]].rowoffset>>FRACBITS;
				else
					lines[i].callcount = sides[lines[i].sidenum[0]].textureoffset>>FRACBITS;
				if (lines[i].special == 322) // Each time
				{
					sec = sides[*lines[i].sidenum].sector - sectors;
					P_AddEachTimeThinker(&sectors[sec], &lines[i]);
				}
				break;

			case 399: // Linedef execute on map load
				// This is handled in P_RunLevelLoadExecutors.
				break;

			case 400:
			case 401:
			case 402:
			case 403:
			case 404:
			case 405:
			case 406:
			case 407:
			case 408:
			case 409:
			case 410:
			case 411:
			case 412:
			case 413:
			case 414:
			case 415:
			case 416:
			case 417:
			case 418:
			case 419:
			case 420:
			case 421:
			case 422:
			case 423:
			case 424:
			case 425:
			case 426:
			case 427:
			case 428:
			case 429:
			case 430:
			case 431:
				break;

			// 500 is used for a scroller
			// 501 is used for a scroller
			// 502 is used for a scroller
			// 503 is used for a scroller
			// 504 is used for a scroller
			// 505 is used for a scroller
			// 510 is used for a scroller
			// 511 is used for a scroller
			// 512 is used for a scroller
			// 513 is used for a scroller
			// 514 is used for a scroller
			// 515 is used for a scroller
			// 520 is used for a scroller
			// 521 is used for a scroller
			// 522 is used for a scroller
			// 523 is used for a scroller
			// 524 is used for a scroller
			// 525 is used for a scroller
			// 530 is used for a scroller
			// 531 is used for a scroller
			// 532 is used for a scroller
			// 533 is used for a scroller
			// 534 is used for a scroller
			// 535 is used for a scroller
			// 540 is used for friction
			// 541 is used for wind
			// 542 is used for upwards wind
			// 543 is used for downwards wind
			// 544 is used for current
			// 545 is used for upwards current
			// 546 is used for downwards current
			// 547 is used for push/pull

			case 600: // floor lighting independently (e.g. lava)
				sec = sides[*lines[i].sidenum].sector-sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					sectors[s].floorlightsec = (INT32)sec;
				break;

			case 601: // ceiling lighting independently
				sec = sides[*lines[i].sidenum].sector-sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					sectors[s].ceilinglightsec = (INT32)sec;
				break;

			case 602: // Adjustable pulsating light
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					P_SpawnAdjustableGlowingLight(&sectors[sec], &sectors[s],
						P_AproxDistance(lines[i].dx, lines[i].dy)>>FRACBITS);
				break;

			case 603: // Adjustable flickering light
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					P_SpawnAdjustableFireFlicker(&sectors[sec], &sectors[s],
						P_AproxDistance(lines[i].dx, lines[i].dy)>>FRACBITS);
				break;

			case 604: // Adjustable Blinking Light (unsynchronized)
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					P_SpawnAdjustableStrobeFlash(&sectors[sec], &sectors[s],
						abs(lines[i].dx)>>FRACBITS, abs(lines[i].dy)>>FRACBITS, false);
				break;

			case 605: // Adjustable Blinking Light (synchronized)
				sec = sides[*lines[i].sidenum].sector - sectors;
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					P_SpawnAdjustableStrobeFlash(&sectors[sec], &sectors[s],
						abs(lines[i].dx)>>FRACBITS, abs(lines[i].dy)>>FRACBITS, true);
				break;

			case 606: // HACK! Copy colormaps. Just plain colormaps.
				for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0 ;)
					sectors[s].midmap = lines[i].frontsector->midmap;
				break;

#ifdef ESLOPE // Slope copy specials. Handled here for sanity.
			case 720:
			case 721:
			case 722:
				P_CopySectorSlope(&lines[i]);
				break;
#endif

			default:
				break;
		}
	}

	// Allocate each list
	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].thinkers)
			Z_Free(secthinkers[i].thinkers);

	Z_Free(secthinkers);

#ifdef POLYOBJECTS
	// haleyjd 02/20/06: spawn polyobjects
	Polyobj_InitLevel();

	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			case 30: // Polyobj_Flag
				EV_DoPolyObjFlag(&lines[i]);
				break;

			case 31: // Polyobj_Displace
				PolyDisplace(&lines[i]);
				break;
		}
	}
#endif

	P_RunLevelLoadExecutors();
}

/** Adds 3Dfloors as appropriate based on a common control linedef.
  *
  * \param line        Control linedef to use.
  * \param ffloorflags 3Dfloor flags to use.
  * \param secthkiners Lists of thinkers sorted by sector. May be NULL.
  * \sa P_SpawnSpecials, P_AddFakeFloor
  * \author Graue <graue@oceanbase.org>
  */
static void P_AddFakeFloorsByLine(size_t line, ffloortype_e ffloorflags, thinkerlist_t *secthinkers)
{
	INT32 s;
	size_t sec = sides[*lines[line].sidenum].sector-sectors;

	for (s = -1; (s = P_FindSectorFromLineTag(lines+line, s)) >= 0 ;)
		P_AddFakeFloor(&sectors[s], &sectors[sec], lines+line, ffloorflags, secthinkers);
}

/*
 SoM: 3/8/2000: General scrolling functions.
 T_Scroll,
 Add_Scroller,
 Add_WallScroller,
 P_SpawnScrollers
*/

// helper function for T_Scroll
static void P_DoScrollMove(mobj_t *thing, fixed_t dx, fixed_t dy, INT32 exclusive)
{
	fixed_t fuckaj = 0; // Nov 05 14:12:08 <+MonsterIestyn> I've heard of explicitly defined variables but this is ridiculous
	if (thing->player)
	{
		if (!(dx | dy))
		{
			thing->player->cmomx = 0;
			thing->player->cmomy = 0;
		}
		else
		{
			thing->player->cmomx += dx;
			thing->player->cmomy += dy;
			thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
			thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
		}
	}

	if (thing->player && (thing->player->pflags & PF_SPINNING) && (thing->player->rmomx || thing->player->rmomy) && !(thing->player->pflags & PF_STARTDASH))
		fuckaj = FixedDiv(549*ORIG_FRICTION,500*FRACUNIT);
	else if (thing->friction != ORIG_FRICTION)
		fuckaj = thing->friction;

	if (fuckaj) {
		// refactor thrust for new friction
		dx = FixedDiv(dx, CARRYFACTOR);
		dy = FixedDiv(dy, CARRYFACTOR);

		dx = FixedMul(dx, FRACUNIT-fuckaj);
		dy = FixedMul(dy, FRACUNIT-fuckaj);
	}

	thing->momx += dx;
	thing->momy += dy;

	if (exclusive)
		thing->flags2 |= MF2_PUSHED;
}

/** Processes an active scroller.
  * This function, with the help of r_plane.c and r_bsp.c, supports generalized
  * scrolling floors and walls, with optional mobj-carrying properties, e.g.
  * conveyor belts, rivers, etc. A linedef with a special type affects all
  * tagged sectors the same way, by creating scrolling and/or object-carrying
  * properties. Multiple linedefs may be used on the same sector and are
  * cumulative, although the special case of scrolling a floor and carrying
  * things on it requires only one linedef.
  *
  * The linedef's direction determines the scrolling direction, and the
  * linedef's length determines the scrolling speed. This was designed so an
  * edge around a sector can be used to control the direction of the sector's
  * scrolling, which is usually what is desired.
  *
  * \param s Thinker for the scroller to process.
  * \todo Split up into multiple functions.
  * \todo Use attached lists to make ::sc_carry_ceiling case faster and
  *       cleaner.
  * \sa Add_Scroller, Add_WallScroller, P_SpawnScrollers
  * \author Steven McGranahan
  * \author Graue <graue@oceanbase.org>
  */
void T_Scroll(scroll_t *s)
{
	fixed_t dx = s->dx, dy = s->dy;
	boolean is3dblock = false;

	if (s->control != -1)
	{ // compute scroll amounts based on a sector's height changes
		fixed_t height = sectors[s->control].floorheight +
			sectors[s->control].ceilingheight;
		fixed_t delta = height - s->last_height;
		s->last_height = height;
		dx = FixedMul(dx, delta);
		dy = FixedMul(dy, delta);
	}

	if (s->accel)
	{
		s->vdx = dx += s->vdx;
		s->vdy = dy += s->vdy;
	}

//	if (!(dx | dy)) // no-op if both (x,y) offsets 0
//		return;

	switch (s->type)
	{
		side_t *side;
		sector_t *sec;
		fixed_t height;
		msecnode_t *node;
		mobj_t *thing;
		line_t *line;
		size_t i;
		INT32 sect;
		ffloor_t *rover;

		case sc_side: // scroll wall texture
			side = sides + s->affectee;
			side->textureoffset += dx;
			side->rowoffset += dy;
			break;

		case sc_floor: // scroll floor texture
			sec = sectors + s->affectee;
			sec->floor_xoffs += dx;
			sec->floor_yoffs += dy;
			break;

		case sc_ceiling: // scroll ceiling texture
			sec = sectors + s->affectee;
			sec->ceiling_xoffs += dx;
			sec->ceiling_yoffs += dy;
			break;

		case sc_carry:
			sec = sectors + s->affectee;
			height = sec->floorheight;

			// sec is the control sector, find the real sector(s) to use
			for (i = 0; i < sec->linecount; i++)
			{
				line = sec->lines[i];

				if (line->special < 100 || line->special >= 300)
					is3dblock = false;
				else
					is3dblock = true;

				if (!is3dblock)
					continue;

				for (sect = -1; (sect = P_FindSectorFromTag(line->tag, sect)) >= 0 ;)
				{
					sector_t *psec;
					psec = sectors + sect;

					// Find the FOF corresponding to the control linedef
					for (rover = psec->ffloors; rover; rover = rover->next)
					{
						if (rover->master == sec->lines[i])
							break;
					}

					if (!rover) // This should be impossible, but don't complain if it is the case somehow
						continue;

					if (!(rover->flags & FF_EXISTS)) // If the FOF does not "exist", we pretend that nobody's there
						continue;

					for (node = psec->touching_thinglist; node; node = node->m_thinglist_next)
					{
						thing = node->m_thing;

						if (thing->flags2 & MF2_PUSHED) // Already pushed this tic by an exclusive pusher.
							continue;

						height = P_GetSpecialBottomZ(thing, sec, psec);

						if (!(thing->flags & MF_NOCLIP)) // Thing must be clipped
						if (!(thing->flags & MF_NOGRAVITY || thing->z+thing->height != height)) // Thing must a) be non-floating and have z+height == height
						{
							// Move objects only if on floor
							// non-floating, and clipped.
							P_DoScrollMove(thing, dx, dy, s->exclusive);
						}
					} // end of for loop through touching_thinglist
				} // end of loop through sectors
			}

			if (!is3dblock)
			{
				for (node = sec->touching_thinglist; node; node = node->m_thinglist_next)
				{
					thing = node->m_thing;

					if (thing->flags2 & MF2_PUSHED)
						continue;

					height = P_GetSpecialBottomZ(thing, sec, sec);

					if (!(thing->flags & MF_NOCLIP) &&
						(!(thing->flags & MF_NOGRAVITY || thing->z > height)))
					{
						// Move objects only if on floor or underwater,
						// non-floating, and clipped.
						P_DoScrollMove(thing, dx, dy, s->exclusive);
					}
				}
			}
			break;

		case sc_carry_ceiling: // carry on ceiling (FOF scrolling)
			sec = sectors + s->affectee;
			height = sec->ceilingheight;

			// sec is the control sector, find the real sector(s) to use
			for (i = 0; i < sec->linecount; i++)
			{
				line = sec->lines[i];
				if (line->special < 100 || line->special >= 300)
					is3dblock = false;
				else
					is3dblock = true;

				if (!is3dblock)
					continue;

				for (sect = -1; (sect = P_FindSectorFromTag(line->tag, sect)) >= 0 ;)
				{
					sector_t *psec;
					psec = sectors + sect;

					// Find the FOF corresponding to the control linedef
					for (rover = psec->ffloors; rover; rover = rover->next)
					{
						if (rover->master == sec->lines[i])
							break;
					}

					if (!rover) // This should be impossible, but don't complain if it is the case somehow
						continue;

					if (!(rover->flags & FF_EXISTS)) // If the FOF does not "exist", we pretend that nobody's there
						continue;

					for (node = psec->touching_thinglist; node; node = node->m_thinglist_next)
					{
						thing = node->m_thing;

						if (thing->flags2 & MF2_PUSHED)
							continue;

						height = P_GetSpecialTopZ(thing, sec, psec);

						if (!(thing->flags & MF_NOCLIP)) // Thing must be clipped
						if (!(thing->flags & MF_NOGRAVITY || thing->z != height))// Thing must a) be non-floating and have z == height
						{
							// Move objects only if on floor or underwater,
							// non-floating, and clipped.
							P_DoScrollMove(thing, dx, dy, s->exclusive);
						}
					} // end of for loop through touching_thinglist
				} // end of loop through sectors
			}

			if (!is3dblock)
			{
				for (node = sec->touching_thinglist; node; node = node->m_thinglist_next)
				{
					thing = node->m_thing;

					if (thing->flags2 & MF2_PUSHED)
						continue;

					height = P_GetSpecialTopZ(thing, sec, sec);

					if (!(thing->flags & MF_NOCLIP) &&
						(!(thing->flags & MF_NOGRAVITY || thing->z+thing->height < height)))
					{
						// Move objects only if on floor or underwater,
						// non-floating, and clipped.
						P_DoScrollMove(thing, dx, dy, s->exclusive);
					}
				}
			}
			break; // end of sc_carry_ceiling
	} // end of switch
}

/** Adds a generalized scroller to the thinker list.
  *
  * \param type     The enumerated type of scrolling.
  * \param dx       x speed of scrolling or its acceleration.
  * \param dy       y speed of scrolling or its acceleration.
  * \param control  Sector whose heights control this scroller's effect
  *                 remotely, or -1 if there is no control sector.
  * \param affectee Index of the affected object, sector or sidedef.
  * \param accel    Nonzero for an accelerative effect.
  * \sa Add_WallScroller, P_SpawnScrollers, T_Scroll
  */
static void Add_Scroller(INT32 type, fixed_t dx, fixed_t dy, INT32 control, INT32 affectee, INT32 accel, INT32 exclusive)
{
	scroll_t *s = Z_Calloc(sizeof *s, PU_LEVSPEC, NULL);
	s->thinker.function.acp1 = (actionf_p1)T_Scroll;
	s->type = type;
	s->dx = dx;
	s->dy = dy;
	s->accel = accel;
	s->exclusive = exclusive;
	s->vdx = s->vdy = 0;
	if ((s->control = control) != -1)
		s->last_height = sectors[control].floorheight + sectors[control].ceilingheight;
	s->affectee = affectee;
	P_AddThinker(&s->thinker);
}

/** Adds a wall scroller.
  * Scroll amount is rotated with respect to wall's linedef first, so that
  * scrolling towards the wall in a perpendicular direction is translated into
  * vertical motion, while scrolling along the wall in a parallel direction is
  * translated into horizontal motion.
  *
  * \param dx      x speed of scrolling or its acceleration.
  * \param dy      y speed of scrolling or its acceleration.
  * \param l       Line whose front side will scroll.
  * \param control Sector whose heights control this scroller's effect
  *                remotely, or -1 if there is no control sector.
  * \param accel   Nonzero for an accelerative effect.
  * \sa Add_Scroller, P_SpawnScrollers
  */
static void Add_WallScroller(fixed_t dx, fixed_t dy, const line_t *l, INT32 control, INT32 accel)
{
	fixed_t x = abs(l->dx), y = abs(l->dy), d;
	if (y > x)
		d = x, x = y, y = d;
	d = FixedDiv(x, FINESINE((tantoangle[FixedDiv(y, x) >> DBITS] + ANGLE_90) >> ANGLETOFINESHIFT));
	x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
	y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
	Add_Scroller(sc_side, x, y, control, *l->sidenum, accel, 0);
}

/** Initializes the scrollers.
  *
  * \todo Get rid of all the magic numbers.
  * \sa P_SpawnSpecials, Add_Scroller, Add_WallScroller
  */
static void P_SpawnScrollers(void)
{
	size_t i;
	line_t *l = lines;

	for (i = 0; i < numlines; i++, l++)
	{
		fixed_t dx = l->dx >> SCROLL_SHIFT; // direction and speed of scrolling
		fixed_t dy = l->dy >> SCROLL_SHIFT;
		INT32 control = -1, accel = 0; // no control sector or acceleration
		INT32 special = l->special;

		// These types are same as the ones they get set to except that the
		// first side's sector's heights cause scrolling when they change, and
		// this linedef controls the direction and speed of the scrolling. The
		// most complicated linedef since donuts, but powerful :)

		if (special == 515 || special == 512 || special == 522 || special == 532 || special == 504) // displacement scrollers
		{
			special -= 2;
			control = (INT32)(sides[*l->sidenum].sector - sectors);
		}
		else if (special == 514 || special == 511 || special == 521 || special == 531 || special == 503) // accelerative scrollers
		{
			special--;
			accel = 1;
			control = (INT32)(sides[*l->sidenum].sector - sectors);
		}
		else if (special == 535 || special == 525) // displacement scrollers
		{
			special -= 2;
			control = (INT32)(sides[*l->sidenum].sector - sectors);
		}
		else if (special == 534 || special == 524) // accelerative scrollers
		{
			accel = 1;
			special--;
			control = (INT32)(sides[*l->sidenum].sector - sectors);
		}

		switch (special)
		{
			register INT32 s;

			case 513: // scroll effect ceiling
			case 533: // scroll and carry objects on ceiling
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Scroller(sc_ceiling, -dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				if (special != 533)
					break;
				/* FALLTHRU */

			case 523:	// carry objects on ceiling
				dx = FixedMul(dx, CARRYFACTOR);
				dy = FixedMul(dy, CARRYFACTOR);
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Scroller(sc_carry_ceiling, dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				break;

			case 510: // scroll effect floor
			case 530: // scroll and carry objects on floor
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Scroller(sc_floor, -dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				if (special != 530)
					break;
				/* FALLTHRU */

			case 520:	// carry objects on floor
				dx = FixedMul(dx, CARRYFACTOR);
				dy = FixedMul(dy, CARRYFACTOR);
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Scroller(sc_carry, dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				break;

			// scroll wall according to linedef
			// (same direction and speed as scrolling floors)
			case 502:
				for (s = -1; (s = P_FindLineFromLineTag(l, s)) >= 0 ;)
					if (s != (INT32)i)
						Add_WallScroller(dx, dy, lines+s, control, accel);
				break;

			case 505:
				s = lines[i].sidenum[0];
				Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, s, accel, 0);
				break;

			case 506:
				s = lines[i].sidenum[1];

				if (s != 0xffff)
					Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, lines[i].sidenum[0], accel, 0);
				else
					CONS_Debug(DBG_GAMELOGIC, "Line special 506 (line #%s) missing 2nd side!\n", sizeu1(i));
				break;

			case 500: // scroll first side
				Add_Scroller(sc_side, FRACUNIT, 0, -1, lines[i].sidenum[0], accel, 0);
				break;

			case 501: // jff 1/30/98 2-way scroll
				Add_Scroller(sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel, 0);
				break;
		}
	}
}

/** Adds master appear/disappear thinker.
  *
  * \param appeartime		tics to be existent
  * \param disappeartime	tics to be nonexistent
  * \param sector			pointer to control sector
  */
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline)
{
	disappear_t *d = Z_Malloc(sizeof *d, PU_LEVSPEC, NULL);

	d->thinker.function.acp1 = (actionf_p1)T_Disappear;
	d->appeartime = appeartime;
	d->disappeartime = disappeartime;
	d->offset = offset;
	d->affectee = line;
	d->sourceline = sourceline;
	d->exists = true;
	d->timer = 1;

	P_AddThinker(&d->thinker);
}

/** Makes a FOF appear/disappear
  *
  * \param d Disappear thinker.
  * \sa Add_MasterDisappearer
  */
void T_Disappear(disappear_t *d)
{
	if (d->offset && !d->exists)
	{
		d->offset--;
		return;
	}

	if (--d->timer <= 0)
	{
		ffloor_t *rover;
		register INT32 s;

		for (s = -1; (s = P_FindSectorFromLineTag(&lines[d->affectee], s)) >= 0 ;)
		{
			for (rover = sectors[s].ffloors; rover; rover = rover->next)
			{
				if (rover->master != &lines[d->affectee])
					continue;

				if (d->exists)
					rover->flags &= ~FF_EXISTS;
				else
				{
					rover->flags |= FF_EXISTS;

					if (!(lines[d->sourceline].flags & ML_NOCLIMB))
					{
#ifdef ESLOPE
						if (*rover->t_slope)
							sectors[s].soundorg.z = P_GetZAt(*rover->t_slope, sectors[s].soundorg.x, sectors[s].soundorg.y);
						else
#endif
						sectors[s].soundorg.z = *rover->topheight;
						S_StartSound(&sectors[s].soundorg, sfx_appear);
					}
				}
			}
			sectors[s].moved = true;
		}

		if (d->exists)
		{
			d->timer = d->disappeartime;
			d->exists = false;
		}
		else
		{
			d->timer = d->appeartime;
			d->exists = true;
		}
	}
}

/*
 SoM: 3/8/2000: Friction functions start.
 Add_Friction,
 T_Friction,
 P_SpawnFriction
*/

/** Adds friction thinker.
  *
  * \param friction      Friction value, 0xe800 is normal.
  * \param movefactor    Inertia factor.
  * \param affectee      Target sector.
  * \param roverfriction FOF or not
  * \sa T_Friction, P_SpawnFriction
  */
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer)
{
	friction_t *f = Z_Calloc(sizeof *f, PU_LEVSPEC, NULL);

	f->thinker.function.acp1 = (actionf_p1)T_Friction;
	f->friction = friction;
	f->movefactor = movefactor;
	f->affectee = affectee;

	if (referrer != -1)
	{
		f->roverfriction = true;
		f->referrer = referrer;
	}
	else
		f->roverfriction = false;

	P_AddThinker(&f->thinker);
}

/** Applies friction to all things in a sector.
  *
  * \param f Friction thinker.
  * \sa Add_Friction
  */
void T_Friction(friction_t *f)
{
	sector_t *sec, *referrer = NULL;
	mobj_t *thing;
	msecnode_t *node;

	sec = sectors + f->affectee;

	// Make sure the sector type hasn't changed
	if (f->roverfriction)
	{
		referrer = sectors + f->referrer;

		if (!(GETSECSPECIAL(referrer->special, 3) == 1
			|| GETSECSPECIAL(referrer->special, 3) == 3))
			return;
	}
	else
	{
		if (!(GETSECSPECIAL(sec->special, 3) == 1
			|| GETSECSPECIAL(sec->special, 3) == 3))
			return;
	}

	// Assign the friction value to players on the floor, non-floating,
	// and clipped. Normally the object's friction value is kept at
	// ORIG_FRICTION and this thinker changes it for icy or muddy floors.

	// When the object is straddling sectors with the same
	// floorheight that have different frictions, use the lowest
	// friction value (muddy has precedence over icy).

	node = sec->touching_thinglist; // things touching this sector
	while (node)
	{
		thing = node->m_thing;
		// apparently, all I had to do was comment out part of the next line and
		// friction works for all mobj's
		// (or at least MF_PUSHABLEs, which is all I care about anyway)
		if (!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) && thing->z == thing->floorz)
		{
			if (f->roverfriction)
			{
				if (thing->floorz != P_GetSpecialTopZ(thing, referrer, sec))
				{
					node = node->m_thinglist_next;
					continue;
				}

				if ((thing->friction == ORIG_FRICTION) // normal friction?
					|| (f->friction < thing->friction))
				{
					thing->friction = f->friction;
					thing->movefactor = f->movefactor;
				}
			}
			else if (P_GetSpecialBottomZ(thing, sec, sec) == thing->floorz && (thing->friction == ORIG_FRICTION // normal friction?
				|| f->friction < thing->friction))
			{
				thing->friction = f->friction;
				thing->movefactor = f->movefactor;
			}
		}
		node = node->m_thinglist_next;
	}
}

/** Spawns all friction effects.
  *
  * \sa P_SpawnSpecials, Add_Friction
  */
static void P_SpawnFriction(void)
{
	size_t i;
	line_t *l = lines;
	register INT32 s;
	fixed_t length; // line length controls magnitude
	fixed_t friction; // friction value to be applied during movement
	INT32 movefactor; // applied to each player move to simulate inertia

	for (i = 0; i < numlines; i++, l++)
		if (l->special == 540)
		{
			length = P_AproxDistance(l->dx, l->dy)>>FRACBITS;
			friction = (0x1EB8*length)/0x80 + 0xD000;

			if (friction > FRACUNIT)
				friction = FRACUNIT;
			if (friction < 0)
				friction = 0;

			// The following check might seem odd. At the time of movement,
			// the move distance is multiplied by 'friction/0x10000', so a
			// higher friction value actually means 'less friction'.

			if (friction > ORIG_FRICTION) // ice
				movefactor = ((0x10092 - friction)*(0x70))/0x158;
			else
				movefactor = ((friction - 0xDB34)*(0xA))/0x80;

			// killough 8/28/98: prevent odd situations
			if (movefactor < 32)
				movefactor = 32;

			for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
				Add_Friction(friction, movefactor, s, -1);
		}
}

/*
 SoM: 3/8/2000: Push/Pull/Wind/Current functions.
 Add_Pusher,
 PIT_PushThing,
 T_Pusher,
 P_GetPushThing,
 P_SpawnPushers
*/

#define PUSH_FACTOR 7

/** Adds a pusher.
  *
  * \param type     Type of push/pull effect.
  * \param x_mag    X magnitude.
  * \param y_mag    Y magnitude.
  * \param source   For a point pusher/puller, the source object.
  * \param affectee Target sector.
  * \param referrer What sector set it
  * \sa T_Pusher, P_GetPushThing, P_SpawnPushers
  */
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, mobj_t *source, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider)
{
	pusher_t *p = Z_Calloc(sizeof *p, PU_LEVSPEC, NULL);

	p->thinker.function.acp1 = (actionf_p1)T_Pusher;
	p->source = source;
	p->type = type;
	p->x_mag = x_mag>>FRACBITS;
	p->y_mag = y_mag>>FRACBITS;
	p->exclusive = exclusive;
	p->slider = slider;

	if (referrer != -1)
	{
		p->roverpusher = true;
		p->referrer = referrer;
	}
	else
		p->roverpusher = false;

	// "The right triangle of the square of the length of the hypotenuse is equal to the sum of the squares of the lengths of the other two sides."
	// "Bah! Stupid brains! Don't you know anything besides the Pythagorean Theorem?" - Earthworm Jim
	if (type == p_downcurrent || type == p_upcurrent || type == p_upwind || type == p_downwind)
		p->magnitude = P_AproxDistance(p->x_mag,p->y_mag)<<(FRACBITS-PUSH_FACTOR);
	else
		p->magnitude = P_AproxDistance(p->x_mag,p->y_mag);
	if (source) // point source exist?
	{
		// where force goes to zero
		if (type == p_push)
			p->radius = AngleFixed(source->angle);
		else
			p->radius = (p->magnitude)<<(FRACBITS+1);

		p->x = p->source->x;
		p->y = p->source->y;
		p->z = p->source->z;
	}
	p->affectee = affectee;
	P_AddThinker(&p->thinker);
}


// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.
static pusher_t *tmpusher; // pusher structure for blockmap searches

/** Applies a point pusher/puller to a thing.
  *
  * \param thing Thing to be pushed.
  * \return True if the thing was pushed.
  * \todo Make a more robust P_BlockThingsIterator() so the hidden parameter
  *       ::tmpusher won't need to be used.
  * \sa T_Pusher
  */
static inline boolean PIT_PushThing(mobj_t *thing)
{
	if (thing->flags2 & MF2_PUSHED)
		return false;

	if (thing->player && thing->player->pflags & PF_ROPEHANG)
		return false;

	// Allow this to affect pushable objects at some point?
	if (thing->player && (!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) || thing->player->pflags & PF_NIGHTSMODE))
	{
		INT32 dist;
		INT32 speed;
		INT32 sx, sy, sz;

		sx = tmpusher->x;
		sy = tmpusher->y;
		sz = tmpusher->z;

		// don't fade wrt Z if health & 2 (mapthing has multi flag)
		if (tmpusher->source->health & 2)
			dist = P_AproxDistance(thing->x - sx,thing->y - sy);
		else
		{
			// Make sure the Z is in range
			if (thing->z < sz - tmpusher->radius || thing->z > sz + tmpusher->radius)
				return false;

			dist = P_AproxDistance(P_AproxDistance(thing->x - sx, thing->y - sy),
				thing->z - sz);
		}

		speed = (tmpusher->magnitude - ((dist>>FRACBITS)>>1))<<(FRACBITS - PUSH_FACTOR - 1);

		// If speed <= 0, you're outside the effective radius. You also have
		// to be able to see the push/pull source point.

		// Written with bits and pieces of P_HomingAttack
		if ((speed > 0) && (P_CheckSight(thing, tmpusher->source)))
		{
			if (!(thing->player->pflags & PF_NIGHTSMODE))
			{
				// only push wrt Z if health & 1 (mapthing has ambush flag)
				if (tmpusher->source->health & 1)
				{
					fixed_t tmpmomx, tmpmomy, tmpmomz;

					tmpmomx = FixedMul(FixedDiv(sx - thing->x, dist), speed);
					tmpmomy = FixedMul(FixedDiv(sy - thing->y, dist), speed);
					tmpmomz = FixedMul(FixedDiv(sz - thing->z, dist), speed);
					if (tmpusher->source->type == MT_PUSH) // away!
					{
						tmpmomx *= -1;
						tmpmomy *= -1;
						tmpmomz *= -1;
					}

					thing->momx += tmpmomx;
					thing->momy += tmpmomy;
					thing->momz += tmpmomz;

					if (thing->player)
					{
						thing->player->cmomx += tmpmomx;
						thing->player->cmomy += tmpmomy;
						thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
						thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
					}
				}
				else
				{
					angle_t pushangle;

					pushangle = R_PointToAngle2(thing->x, thing->y, sx, sy);
					if (tmpusher->source->type == MT_PUSH)
						pushangle += ANGLE_180; // away
					pushangle >>= ANGLETOFINESHIFT;
					thing->momx += FixedMul(speed, FINECOSINE(pushangle));
					thing->momy += FixedMul(speed, FINESINE(pushangle));

					if (thing->player)
					{
						thing->player->cmomx += FixedMul(speed, FINECOSINE(pushangle));
						thing->player->cmomy += FixedMul(speed, FINESINE(pushangle));
						thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
						thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
					}
				}
			}
			else
			{
				//NiGHTS-specific handling.
				//By default, pushes and pulls only affect the Z-axis.
				//By having the ambush flag, it affects the X-axis.
				//By having the object special flag, it affects the Y-axis.
				fixed_t tmpmomx, tmpmomy, tmpmomz;

				if (tmpusher->source->health & 1)
					tmpmomx = FixedMul(FixedDiv(sx - thing->x, dist), speed);
				else
					tmpmomx = 0;

				if (tmpusher->source->health & 2)
					tmpmomy = FixedMul(FixedDiv(sy - thing->y, dist), speed);
				else
					tmpmomy = 0;

				tmpmomz = FixedMul(FixedDiv(sz - thing->z, dist), speed);

				if (tmpusher->source->type == MT_PUSH) // away!
				{
					tmpmomx *= -1;
					tmpmomy *= -1;
					tmpmomz *= -1;
				}

				thing->momx += tmpmomx;
				thing->momy += tmpmomy;
				thing->momz += tmpmomz;

				if (thing->player)
				{
					thing->player->cmomx += tmpmomx;
					thing->player->cmomy += tmpmomy;
					thing->player->cmomx = FixedMul(thing->player->cmomx, 0xe800);
					thing->player->cmomy = FixedMul(thing->player->cmomy, 0xe800);
				}
			}
		}
	}

	if (tmpusher->exclusive)
		thing->flags2 |= MF2_PUSHED;

	return true;
}

/** Applies a pusher to all affected objects.
  *
  * \param p Thinker for the pusher effect.
  * \todo Split up into multiple functions.
  * \sa Add_Pusher, PIT_PushThing
  */
void T_Pusher(pusher_t *p)
{
	sector_t *sec, *referrer = NULL;
	mobj_t *thing;
	msecnode_t *node;
	INT32 xspeed = 0,yspeed = 0;
	INT32 xl, xh, yl, yh, bx, by;
	INT32 radius;
	//INT32 ht = 0;
	boolean inFOF;
	boolean touching;
	boolean moved;

	xspeed = yspeed = 0;

	sec = sectors + p->affectee;

	// Be sure the special sector type is still turned on. If so, proceed.
	// Else, bail out; the sector type has been changed on us.

	if (p->roverpusher)
	{
		referrer = &sectors[p->referrer];

		if (!(GETSECSPECIAL(referrer->special, 3) == 2
			|| GETSECSPECIAL(referrer->special, 3) == 3))
			return;
	}
	else if (!(GETSECSPECIAL(sec->special, 3) == 2
			|| GETSECSPECIAL(sec->special, 3) == 3))
		return;

	// For constant pushers (wind/current) there are 3 situations:
	//
	// 1) Affected Thing is above the floor.
	//
	//    Apply the full force if wind, no force if current.
	//
	// 2) Affected Thing is on the ground.
	//
	//    Apply half force if wind, full force if current.
	//
	// 3) Affected Thing is below the ground (underwater effect).
	//
	//    Apply no force if wind, full force if current.
	//
	// Apply the effect to clipped players only for now.
	//
	// In Phase II, you can apply these effects to Things other than players.

	if (p->type == p_push)
	{

		// Seek out all pushable things within the force radius of this
		// point pusher. Crosses sectors, so use blockmap.

		tmpusher = p; // MT_PUSH/MT_PULL point source
		radius = p->radius; // where force goes to zero
		tmbbox[BOXTOP]    = p->y + radius;
		tmbbox[BOXBOTTOM] = p->y - radius;
		tmbbox[BOXRIGHT]  = p->x + radius;
		tmbbox[BOXLEFT]   = p->x - radius;

		xl = (unsigned)(tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
		xh = (unsigned)(tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
		yl = (unsigned)(tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
		yh = (unsigned)(tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
		for (bx = xl; bx <= xh; bx++)
			for (by = yl; by <= yh; by++)
				P_BlockThingsIterator(bx,by, PIT_PushThing);
		return;
	}

	// constant pushers p_wind and p_current
	node = sec->touching_thinglist; // things touching this sector
	for (; node; node = node->m_thinglist_next)
	{
		thing = node->m_thing;
		if (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)
			&& !(thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE))
			continue;

		if (!(thing->flags & MF_PUSHABLE) && !(thing->type == MT_PLAYER
			|| thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE
			|| thing->type == MT_LITTLETUMBLEWEED
			|| thing->type == MT_BIGTUMBLEWEED))
			continue;

		if (thing->flags2 & MF2_PUSHED)
			continue;

		if (thing->player && thing->player->pflags & PF_ROPEHANG)
			continue;

		if (thing->player && (thing->state == &states[thing->info->painstate]) && (thing->player->powers[pw_flashing] > (flashingtics/4)*3 && thing->player->powers[pw_flashing] <= flashingtics))
			continue;

		inFOF = touching = moved = false;

		// Find the area that the 'thing' is in
		if (p->roverpusher)
		{
			fixed_t top, bottom;

			top = P_GetSpecialTopZ(thing, referrer, sec);
			bottom = P_GetSpecialBottomZ(thing, referrer, sec);

			if (thing->eflags & MFE_VERTICALFLIP)
			{
				if (bottom > thing->z + thing->height
					|| top < (thing->z + (thing->height >> 1)))
					continue;

				if (thing->z < bottom)
					touching = true;

				if (thing->z + (thing->height >> 1) > bottom)
					inFOF = true;

			}
			else
			{
				if (top < thing->z || bottom > (thing->z + (thing->height >> 1)))
					continue;
				if (thing->z + thing->height > top)
					touching = true;

				if (thing->z + (thing->height >> 1) < top)
					inFOF = true;
			}
		}
		else // Treat the entire sector as one big FOF
		{
			if (thing->z == P_GetSpecialBottomZ(thing, sec, sec))
				touching = true;
			else if (p->type != p_current)
				inFOF = true;
		}

		if (!touching && !inFOF) // Object is out of range of effect
			continue;

		if (p->type == p_wind)
		{
			if (touching) // on ground
			{
				xspeed = (p->x_mag)>>1; // half force
				yspeed = (p->y_mag)>>1;
				moved = true;
			}
			else if (inFOF)
			{
				xspeed = (p->x_mag); // full force
				yspeed = (p->y_mag);
				moved = true;
			}
		}
		else if (p->type == p_upwind)
		{
			if (touching) // on ground
			{
				thing->momz += (p->magnitude)>>1;
				moved = true;
			}
			else if (inFOF)
			{
				thing->momz += p->magnitude;
				moved = true;
			}
		}
		else if (p->type == p_downwind)
		{
			if (touching) // on ground
			{
				thing->momz -= (p->magnitude)>>1;
				moved = true;
			}
			else if (inFOF)
			{
				thing->momz -= p->magnitude;
				moved = true;
			}
		}
		else // p_current
		{
			if (!touching && !inFOF) // Not in water at all
				xspeed = yspeed = 0; // no force
			else // underwater / touching water
			{
				if (p->type == p_upcurrent)
					thing->momz += p->magnitude;
				else if (p->type == p_downcurrent)
					thing->momz -= p->magnitude;
				else
				{
					xspeed = p->x_mag; // full force
					yspeed = p->y_mag;
				}
				moved = true;
			}
		}

		if (p->type != p_downcurrent && p->type != p_upcurrent
			&& p->type != p_upwind && p->type != p_downwind)
		{
			thing->momx += xspeed<<(FRACBITS-PUSH_FACTOR);
			thing->momy += yspeed<<(FRACBITS-PUSH_FACTOR);
			if (thing->player)
			{
				thing->player->cmomx += xspeed<<(FRACBITS-PUSH_FACTOR);
				thing->player->cmomy += yspeed<<(FRACBITS-PUSH_FACTOR);
				thing->player->cmomx = FixedMul(thing->player->cmomx, ORIG_FRICTION);
				thing->player->cmomy = FixedMul(thing->player->cmomy, ORIG_FRICTION);
			}

			// Tumbleweeds bounce a bit...
			if (thing->type == MT_LITTLETUMBLEWEED || thing->type == MT_BIGTUMBLEWEED)
				thing->momz += P_AproxDistance(xspeed<<(FRACBITS-PUSH_FACTOR), yspeed<<(FRACBITS-PUSH_FACTOR)) >> 2;
		}

		if (moved)
		{
			if (p->slider && thing->player)
			{
				boolean jumped = (thing->player->pflags & PF_JUMPED);
				P_ResetPlayer (thing->player);

				if (jumped)
					thing->player->pflags |= PF_JUMPED;

				thing->player->pflags |= PF_SLIDING;
				P_SetPlayerMobjState (thing, thing->info->painstate); // Whee!
				thing->angle = R_PointToAngle2 (0, 0, xspeed<<(FRACBITS-PUSH_FACTOR), yspeed<<(FRACBITS-PUSH_FACTOR));

				if (!demoplayback || P_AnalogMove(thing->player))
				{
					if (thing->player == &players[consoleplayer])
					{
						if (thing->angle - localangle > ANGLE_180)
							localangle -= (localangle - thing->angle) / 8;
						else
							localangle += (thing->angle - localangle) / 8;
					}
					else if (thing->player == &players[secondarydisplayplayer])
					{
						if (thing->angle - localangle2 > ANGLE_180)
							localangle2 -= (localangle2 - thing->angle) / 8;
						else
							localangle2 += (thing->angle - localangle2) / 8;
					}
					/*if (thing->player == &players[consoleplayer])
						localangle = thing->angle;
					else if (thing->player == &players[secondarydisplayplayer])
						localangle2 = thing->angle;*/
				}
			}

			if (p->exclusive)
				thing->flags2 |= MF2_PUSHED;
		}
	}
}


/** Gets a push/pull object.
  *
  * \param s Sector number to look in.
  * \return Pointer to the first ::MT_PUSH or ::MT_PULL object found in the
  *         sector.
  * \sa P_GetTeleportDestThing, P_GetStarpostThing, P_GetAltViewThing
  */
mobj_t *P_GetPushThing(UINT32 s)
{
	mobj_t *thing;
	sector_t *sec;

	sec = sectors + s;
	thing = sec->thinglist;
	while (thing)
	{
		switch (thing->type)
		{
			case MT_PUSH:
			case MT_PULL:
				return thing;
			default:
				break;
		}
		thing = thing->snext;
	}
	return NULL;
}

/** Spawns pushers.
  *
  * \todo Remove magic numbers.
  * \sa P_SpawnSpecials, Add_Pusher
  */
static void P_SpawnPushers(void)
{
	size_t i;
	line_t *l = lines;
	register INT32 s;
	mobj_t *thing;

	for (i = 0; i < numlines; i++, l++)
		switch (l->special)
		{
			case 541: // wind
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Pusher(p_wind, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 544: // current
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Pusher(p_current, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 547: // push/pull
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
				{
					thing = P_GetPushThing(s);
					if (thing) // No MT_P* means no effect
						Add_Pusher(p_push, l->dx, l->dy, thing, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				}
				break;
			case 545: // current up
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Pusher(p_upcurrent, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 546: // current down
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Pusher(p_downcurrent, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 542: // wind up
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Pusher(p_upwind, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 543: // wind down
				for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0 ;)
					Add_Pusher(p_downwind, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
		}
}

static void P_SearchForDisableLinedefs(void)
{
	size_t i;
	INT32 j;

	// Look for disable linedefs
	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 6)
		{
			// Remove special
			// Do *not* remove tag. That would mess with the tag lists
			// that P_InitTagLists literally just created!
			lines[i].special = 0;

			// Ability flags can disable disable linedefs now, lol
			if (netgame || multiplayer)
			{
				// future: nonet flag?
			}
			else if ((lines[i].flags & ML_NETONLY) == ML_NETONLY)
				continue; // Net-only never triggers in single player
			else if (players[consoleplayer].charability == CA_THOK && (lines[i].flags & ML_NOSONIC))
				continue;
			else if (players[consoleplayer].charability == CA_FLY && (lines[i].flags & ML_NOTAILS))
				continue;
			else if (players[consoleplayer].charability == CA_GLIDEANDCLIMB && (lines[i].flags & ML_NOKNUX))
				continue;

			// Disable any linedef specials with our tag.
			for (j = -1; (j = P_FindLineFromLineTag(&lines[i], j)) >= 0;)
				lines[j].special = 0;
		}
	}
}
