// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2021 by Sonic Team Junior.
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
#include "r_textures.h"
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
#include "v_video.h" // V_AUTOFADEOUT|V_ALLOWLOWERCASE
#include "m_misc.h"
#include "m_cond.h" //unlock triggers
#include "lua_hook.h" // LUAh_LinedefExecute
#include "f_finale.h" // control text prompt
#include "r_skins.h" // skins

#ifdef HW3SOUND
#include "hardware/hw3sound.h"
#endif

// Not sure if this is necessary, but it was in w_wad.c, so I'm putting it here too -Shadow Hog
#include <errno.h>

mobj_t *skyboxmo[2]; // current skybox mobjs: 0 = viewpoint, 1 = centerpoint
mobj_t *skyboxviewpnts[16]; // array of MT_SKYBOX viewpoint mobjs
mobj_t *skyboxcenterpnts[16]; // array of MT_SKYBOX centerpoint mobjs

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
  * Used for loading an ANIMDEFS lump from a wad.
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

static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);
static void Add_Pusher(pushertype_e type, fixed_t x_mag, fixed_t y_mag, mobj_t *source, INT32 affectee, INT32 referrer, INT32 exclusive, INT32 slider); //SoM: 3/9/2000
static void Add_MasterDisappearer(tic_t appeartime, tic_t disappeartime, tic_t offset, INT32 line, INT32 sourceline);
static void P_ResetFakeFloorFader(ffloor_t *rover, fade_t *data, boolean finalize);
#define P_RemoveFakeFloorFader(l) P_ResetFakeFloorFader(l, NULL, false);
static boolean P_FadeFakeFloor(ffloor_t *rover, INT16 sourcevalue, INT16 destvalue, INT16 speed, boolean ticbased, INT32 *timer,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha);
static void P_AddFakeFloorFader(ffloor_t *rover, size_t sectornum, size_t ffloornum,
	INT16 destvalue, INT16 speed, boolean ticbased, boolean relative,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha);
static void P_ResetColormapFader(sector_t *sector);
static void Add_ColormapFader(sector_t *sector, extracolormap_t *source_exc, extracolormap_t *dest_exc,
	boolean ticbased, INT32 duration);
static void P_AddBlockThinker(sector_t *sec, line_t *sourceline);
static void P_AddFloatThinker(sector_t *sec, UINT16 tag, line_t *sourceline);
//static void P_AddBridgeThinker(line_t *sourceline, sector_t *sec);
static void P_AddFakeFloorsByLine(size_t line, ffloortype_e ffloorflags, thinkerlist_t *secthinkers);
static void P_ProcessLineSpecial(line_t *line, mobj_t *mo, sector_t *callsec);
static void Add_Friction(INT32 friction, INT32 movefactor, INT32 affectee, INT32 referrer);
static void P_AddPlaneDisplaceThinker(INT32 type, fixed_t speed, INT32 control, INT32 affectee, UINT8 reverse);


//SoM: 3/7/2000: New sturcture without limits.
static anim_t *lastanim;
static anim_t *anims = NULL; /// \todo free leak
static size_t maxanims;

// Animating line specials

// Init animated textures
// - now called at level loading P_SetupLevel()

static animdef_t *animdefs = NULL;

// Increase the size of animdefs to make room for a new animation definition
static void GrowAnimDefs(void)
{
	maxanims++;
	animdefs = (animdef_t *)Z_Realloc(animdefs, sizeof(animdef_t)*(maxanims + 1), PU_STATIC, NULL);
}

// A prototype; here instead of p_spec.h, so they're "private"
void P_ParseANIMDEFSLump(INT32 wadNum, UINT16 lumpnum);
void P_ParseAnimationDefintion(SINT8 istexture);

/** Sets up texture and flat animations.
  *
  * Converts an ::animdef_t array loaded from a lump into
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
	size_t i;

	I_Assert(animdefs == NULL);

	maxanims = 0;

	for (w = numwadfiles-1; w >= 0; w--)
	{
		UINT16 animdefsLumpNum;

		// Find ANIMDEFS lump in the WAD
		animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", w, 0);

		while (animdefsLumpNum != INT16_MAX)
		{
			P_ParseANIMDEFSLump(w, animdefsLumpNum);
			animdefsLumpNum = W_CheckNumForNamePwad("ANIMDEFS", (UINT16)w, animdefsLumpNum + 1);
		}
	}

	// Define the last one
	animdefs[maxanims].istexture = -1;
	strncpy(animdefs[maxanims].endname, "", 9);
	strncpy(animdefs[maxanims].startname, "", 9);
	animdefs[maxanims].speed = 0;

	if (anims)
		free(anims);

	anims = (anim_t *)malloc(sizeof (*anims)*(maxanims + 1));
	if (!anims)
		I_Error("Not enough free memory for ANIMDEFS data");

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

			lastanim->picnum = R_GetFlatNumForName(animdefs[i].endname);
			lastanim->basepic = R_GetFlatNumForName(animdefs[i].startname);
		}

		lastanim->istexture = animdefs[i].istexture;
		lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

		if (lastanim->numpics < 2)
		{
			free(anims);
			I_Error("P_InitPicAnims: bad cycle from %s to %s",
				animdefs[i].startname, animdefs[i].endname);
		}

		lastanim->speed = LONG(animdefs[i].speed);
		lastanim++;
	}
	lastanim->istexture = -1;
	R_ClearTextureNumCache(false);

	// Clear animdefs now that we're done with it.
	// We'll only be using anims from now on.
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
		if (animdefs[i].istexture == istexture // Check if it's the same type!
		&& stricmp(animdefsToken, animdefs[i].startname) == 0)
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
		GrowAnimDefs();
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

#ifdef WALLFLATS
	// hehe... uhh.....
	if (!istexture)
	{
		GrowAnimDefs();
		M_Memcpy(&animdefs[maxanims-1], &animdefs[i], sizeof(animdef_t));
		animdefs[maxanims-1].istexture = 1;
	}
#endif
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
		if ((anims[animnum].istexture) && (foundflats->type == LEVELFLAT_TEXTURE)
			&& ((UINT16)foundflats->u.texture.num >= startflatnum && (UINT16)foundflats->u.texture.num <= endflatnum))
		{
			foundflats->u.texture.basenum = startflatnum;
			foundflats->animseq = foundflats->u.texture.num - startflatnum;
			foundflats->numpics = endflatnum - startflatnum + 1;
			foundflats->speed = anims[animnum].speed;

			CONS_Debug(DBG_SETUP, "animflat: #%03d name:%.8s animseq:%d numpics:%d speed:%d\n",
					atoi(sizeu1(i)), foundflats->name, foundflats->animseq,
					foundflats->numpics,foundflats->speed);
		}
		else if ((!anims[animnum].istexture) && (foundflats->type == LEVELFLAT_FLAT)
			&& (foundflats->u.flat.lumpnum >= startflatnum && foundflats->u.flat.lumpnum <= endflatnum))
		{
			foundflats->u.flat.baselumpnum = startflatnum;
			foundflats->animseq = foundflats->u.flat.lumpnum - startflatnum;
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
		P_FindAnimatedFlat(i);
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

// Parses arguments for parameterized polyobject door types
static boolean PolyDoor(line_t *line)
{
	polydoordata_t pdd;

	pdd.polyObjNum = Tag_FGet(&line->tags); // polyobject id

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

// Parses arguments for parameterized polyobject move specials
static boolean PolyMove(line_t *line)
{
	polymovedata_t pmd;

	pmd.polyObjNum = Tag_FGet(&line->tags);
	pmd.speed      = sides[line->sidenum[0]].textureoffset / 8;
	pmd.angle      = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y);
	pmd.distance   = sides[line->sidenum[0]].rowoffset;

	pmd.overRide = (line->special == 483); // Polyobj_OR_Move

	return EV_DoPolyObjMove(&pmd);
}

// Makes a polyobject invisible and intangible
// If NOCLIMB is ticked, the polyobject will still be tangible, just not visible.
static void PolyInvisible(line_t *line)
{
	INT32 polyObjNum = Tag_FGet(&line->tags);
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

// Makes a polyobject visible and tangible
// If NOCLIMB is ticked, the polyobject will not be tangible, just visible.
static void PolyVisible(line_t *line)
{
	INT32 polyObjNum = Tag_FGet(&line->tags);
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
	po->flags |= (po->spawnflags & POF_RENDERALL);
}


// Sets the translucency of a polyobject
// Frontsector floor / 100 = translevel
static void PolyTranslucency(line_t *line)
{
	INT32 polyObjNum = Tag_FGet(&line->tags);
	polyobj_t *po;
	INT32 value;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "EV_DoPolyObjWaypoint: bad polyobj %d\n", polyObjNum);
		return;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return;

	// If Front X Offset is specified, use that. Else, use floorheight.
	value = (sides[line->sidenum[0]].textureoffset ? sides[line->sidenum[0]].textureoffset : line->frontsector->floorheight) >> FRACBITS;

	// If DONTPEGBOTTOM, specify raw translucency value. Else, take it out of 1000.
	if (!(line->flags & ML_DONTPEGBOTTOM))
		value /= 100;

	if (line->flags & ML_EFFECT3) // relative calc
		po->translucency += value;
	else
		po->translucency = value;

	po->translucency = max(min(po->translucency, NUMTRANSMAPS), 0);
}

// Makes a polyobject translucency fade and applies tangibility
static boolean PolyFade(line_t *line)
{
	INT32 polyObjNum = Tag_FGet(&line->tags);
	polyobj_t *po;
	polyfadedata_t pfd;
	INT32 value;

	if (!(po = Polyobj_GetForNum(polyObjNum)))
	{
		CONS_Debug(DBG_POLYOBJ, "PolyFade: bad polyobj %d\n", polyObjNum);
		return 0;
	}

	// don't allow line actions to affect bad polyobjects
	if (po->isBad)
		return 0;

	// Prevent continuous execs from interfering on an existing fade
	if (!(line->flags & ML_EFFECT5)
		&& po->thinker
		&& po->thinker->function.acp1 == (actionf_p1)T_PolyObjFade)
	{
		CONS_Debug(DBG_POLYOBJ, "Line type 492 Executor: Fade PolyObject thinker already exists\n");
		return 0;
	}

	pfd.polyObjNum = polyObjNum;

	// If Front X Offset is specified, use that. Else, use floorheight.
	value = (sides[line->sidenum[0]].textureoffset ? sides[line->sidenum[0]].textureoffset : line->frontsector->floorheight) >> FRACBITS;

	// If DONTPEGBOTTOM, specify raw translucency value. Else, take it out of 1000.
	if (!(line->flags & ML_DONTPEGBOTTOM))
		value /= 100;

	if (line->flags & ML_EFFECT3) // relative calc
		pfd.destvalue = po->translucency + value;
	else
		pfd.destvalue = value;

	pfd.destvalue = max(min(pfd.destvalue, NUMTRANSMAPS), 0);

	// already equal, nothing to do
	if (po->translucency == pfd.destvalue)
		return 1;

	pfd.docollision = !(line->flags & ML_BOUNCY);         // do not handle collision flags
	pfd.doghostfade = (line->flags & ML_EFFECT1);         // do ghost fade (no collision flags during fade)
	pfd.ticbased = (line->flags & ML_EFFECT4);            // Speed = Tic Duration

	// allow Back Y Offset to be consistent with other fade specials
	pfd.speed = (line->sidenum[1] != 0xFFFF && !sides[line->sidenum[0]].rowoffset) ?
		abs(sides[line->sidenum[1]].rowoffset>>FRACBITS)
		: abs(sides[line->sidenum[0]].rowoffset>>FRACBITS);


	return EV_DoPolyObjFade(&pfd);
}

// Parses arguments for parameterized polyobject waypoint movement
static boolean PolyWaypoint(line_t *line)
{
	polywaypointdata_t pwd;

	pwd.polyObjNum = Tag_FGet(&line->tags);
	pwd.speed      = sides[line->sidenum[0]].textureoffset / 8;
	pwd.sequence   = sides[line->sidenum[0]].rowoffset >> FRACBITS; // Sequence #

	// Behavior after reaching the last waypoint?
	if (line->flags & ML_EFFECT3)
		pwd.returnbehavior = PWR_WRAP; // Wrap back to first waypoint
	else if (line->flags & ML_EFFECT2)
		pwd.returnbehavior = PWR_COMEBACK; // Go through sequence in reverse
	else
		pwd.returnbehavior = PWR_STOP; // Stop

	// Flags
	pwd.flags = 0;
	if (line->flags & ML_EFFECT1)
		pwd.flags |= PWF_REVERSE;
	if (line->flags & ML_EFFECT4)
		pwd.flags |= PWF_LOOP;

	return EV_DoPolyObjWaypoint(&pwd);
}

// Parses arguments for parameterized polyobject rotate specials
static boolean PolyRotate(line_t *line)
{
	polyrotdata_t prd;

	prd.polyObjNum = Tag_FGet(&line->tags);
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

// Parses arguments for polyobject flag waving special
static boolean PolyFlag(line_t *line)
{
	polyflagdata_t pfd;

	pfd.polyObjNum = Tag_FGet(&line->tags);
	pfd.speed = P_AproxDistance(line->dx, line->dy) >> FRACBITS;
	pfd.angle = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y) >> ANGLETOFINESHIFT;
	pfd.momx = sides[line->sidenum[0]].textureoffset >> FRACBITS;

	return EV_DoPolyObjFlag(&pfd);
}

// Parses arguments for parameterized polyobject move-by-sector-heights specials
static boolean PolyDisplace(line_t *line)
{
	polydisplacedata_t pdd;

	pdd.polyObjNum = Tag_FGet(&line->tags);

	pdd.controlSector = line->frontsector;
	pdd.dx = line->dx>>8;
	pdd.dy = line->dy>>8;

	return EV_DoPolyObjDisplace(&pdd);
}


// Parses arguments for parameterized polyobject rotate-by-sector-heights specials
static boolean PolyRotDisplace(line_t *line)
{
	polyrotdisplacedata_t pdd;
	fixed_t anginter, distinter;

	pdd.polyObjNum = Tag_FGet(&line->tags);
	pdd.controlSector = line->frontsector;

	// Rotate 'anginter' interval for each 'distinter' interval from the control sector.
	// Use default values if not provided as fallback.
	anginter	= sides[line->sidenum[0]].rowoffset ? sides[line->sidenum[0]].rowoffset : 90*FRACUNIT;
	distinter	= sides[line->sidenum[0]].textureoffset ? sides[line->sidenum[0]].textureoffset : 128*FRACUNIT;
	pdd.rotscale = FixedDiv(anginter, distinter);

	// Same behavior as other rotators when carrying things.
	if (line->flags & ML_NOCLIMB)
		pdd.turnobjs = 0;
	else if (line->flags & ML_EFFECT4)
		pdd.turnobjs = 2;
	else
		pdd.turnobjs = 1;

	return EV_DoPolyObjRotDisplace(&pdd);
}


//
// P_RunNightserizeExecutors
//
void P_RunNightserizeExecutors(mobj_t *actor)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 323 || lines[i].special == 324)
			P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}

//
// P_RunDeNightserizeExecutors
//
void P_RunDeNightserizeExecutors(mobj_t *actor)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 325 || lines[i].special == 326)
			P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}

//
// P_RunNightsLapExecutors
//
void P_RunNightsLapExecutors(mobj_t *actor)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == 327 || lines[i].special == 328)
			P_RunTriggerLinedef(&lines[i], actor, NULL);
	}
}

//
// P_RunNightsCapsuleTouchExecutors
//
void P_RunNightsCapsuleTouchExecutors(mobj_t *actor, boolean entering, boolean enoughspheres)
{
	size_t i;

	for (i = 0; i < numlines; i++)
	{
		if ((lines[i].special == 329 || lines[i].special == 330)
			&& ((entering && (lines[i].flags & ML_TFERLINE))
				|| (!entering && !(lines[i].flags & ML_TFERLINE)))
			&& ((lines[i].flags & ML_DONTPEGTOP)
				|| (enoughspheres && !(lines[i].flags & ML_BOUNCY))
				|| (!enoughspheres && (lines[i].flags & ML_BOUNCY))))
			P_RunTriggerLinedef(&lines[i], actor, NULL);
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
	INT32 delay;

	if (udmf)
		delay = line->executordelay;
	else
	{
		if (!line->backsector)
			I_Error("P_AddExecutorDelay: Line has no backsector!\n");

		delay = (line->backsector->ceilingheight >> FRACBITS) + (line->backsector->floorheight >> FRACBITS);
	}

	e = Z_Calloc(sizeof (*e), PU_LEVSPEC, NULL);

	e->thinker.function.acp1 = (actionf_p1)T_ExecutorDelay;
	e->line = line;
	e->sector = sector;
	e->timer = delay;
	P_SetTarget(&e->caller, mobj); // Use P_SetTarget to make sure the mobj doesn't get freed while we're delaying.
	P_AddThinker(THINK_MAIN, &e->thinker);
}

/** Used by P_RunTriggerLinedef to check a NiGHTS trigger linedef's conditions
  *
  * \param triggerline Trigger linedef to check conditions for; should NEVER be NULL.
  * \param actor Object initiating the action; should not be NULL.
  * \sa P_RunTriggerLinedef
  */
static boolean P_CheckNightsTriggerLine(line_t *triggerline, mobj_t *actor)
{
	INT16 specialtype = triggerline->special;
	size_t i;

	UINT8 inputmare = max(0, min(255, sides[triggerline->sidenum[0]].textureoffset>>FRACBITS));
	UINT8 inputlap = max(0, min(255, sides[triggerline->sidenum[0]].rowoffset>>FRACBITS));

	boolean ltemare = triggerline->flags & ML_NOCLIMB;
	boolean gtemare = triggerline->flags & ML_BLOCKMONSTERS;
	boolean ltelap = triggerline->flags & ML_EFFECT1;
	boolean gtelap = triggerline->flags & ML_EFFECT2;

	boolean lapfrombonustime = triggerline->flags & ML_EFFECT3;
	boolean perglobalinverse = triggerline->flags & ML_DONTPEGBOTTOM;
	boolean perglobal = !(triggerline->flags & ML_EFFECT4) && !perglobalinverse;

	boolean donomares = triggerline->flags & ML_BOUNCY; // nightserize: run at end of level (no mares)
	boolean fromnonights = triggerline->flags & ML_TFERLINE; // nightserize: from non-nights // denightserize: all players no nights
	boolean fromnights = triggerline->flags & ML_DONTPEGTOP; // nightserize: from nights // denightserize: >0 players are nights

	UINT8 currentmare = UINT8_MAX;
	UINT8 currentlap = UINT8_MAX;

	// Do early returns for Nightserize
	if (specialtype >= 323 && specialtype <= 324)
	{
		// run only when no mares are found
		if (donomares && P_FindLowestMare() != UINT8_MAX)
			return false;

		// run only if there is a mare present
		if (!donomares && P_FindLowestMare() == UINT8_MAX)
			return false;

		// run only if player is nightserizing from non-nights
		if (fromnonights)
		{
			if (!actor->player)
				return false;
			else if (actor->player->powers[pw_carry] == CR_NIGHTSMODE)
				return false;
		}
		// run only if player is nightserizing from nights
		else if (fromnights)
		{
			if (!actor->player)
				return false;
			else if (actor->player->powers[pw_carry] != CR_NIGHTSMODE)
				return false;
		}
	}

	// Get current mare and lap (and check early return for DeNightserize)
	if (perglobal || perglobalinverse
		|| (specialtype >= 325 && specialtype <= 326 && (fromnonights || fromnights)))
	{
		UINT8 playersarenights = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			UINT8 lap;
			if (!playeringame[i] || players[i].spectator)
				continue;

			// denightserize: run only if all players are not nights
			if (specialtype >= 325 && specialtype <= 326 && fromnonights
				&& players[i].powers[pw_carry] == CR_NIGHTSMODE)
				return false;

			// count number of nights players for denightserize return
			if (specialtype >= 325 && specialtype <= 326 && fromnights
				&& players[i].powers[pw_carry] == CR_NIGHTSMODE)
				playersarenights++;

			lap = lapfrombonustime ? players[i].marebonuslap : players[i].marelap;

			// get highest mare/lap of players
			if (perglobal)
			{
				if (players[i].mare > currentmare || currentmare == UINT8_MAX)
				{
					currentmare = players[i].mare;
					currentlap = UINT8_MAX;
				}
				if (players[i].mare == currentmare
					&& (lap > currentlap || currentlap == UINT8_MAX))
					currentlap = lap;
			}
			// get lowest mare/lap of players
			else if (perglobalinverse)
			{
				if (players[i].mare < currentmare || currentmare == UINT8_MAX)
				{
					currentmare = players[i].mare;
					currentlap = UINT8_MAX;
				}
				if (players[i].mare == currentmare
					&& (lap < currentlap || currentlap == UINT8_MAX))
					currentlap = lap;
			}
		}

		// denightserize: run only if >0 players are nights
		if (specialtype >= 325 && specialtype <= 326 && fromnights
			&& playersarenights < 1)
			return false;
	}
	// get current mare/lap from triggering player
	else if (!perglobal && !perglobalinverse)
	{
		if (!actor->player)
			return false;
		currentmare = actor->player->mare;
		currentlap = lapfrombonustime ? actor->player->marebonuslap : actor->player->marelap;
	}

	if (lapfrombonustime && !currentlap)
		return false; // special case: player->marebonuslap is 0 until passing through on bonus time. Don't trigger lines looking for inputlap 0.

	// Compare current mare/lap to input mare/lap based on rules
	if (!(specialtype >= 323 && specialtype <= 324 && donomares) // don't return false if donomares and we got this far
		&& ((ltemare && currentmare > inputmare)
		|| (gtemare && currentmare < inputmare)
		|| (!ltemare && !gtemare && currentmare != inputmare)
		|| (ltelap && currentlap > inputlap)
		|| (gtelap && currentlap < inputlap)
		|| (!ltelap && !gtelap && currentlap != inputlap))
		)
		return false;

	return true;
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

				if (!players[i].mo || ((maptol & TOL_NIGHTS) ? players[i].spheres : players[i].rings) <= 0)
					continue;

				rings += (maptol & TOL_NIGHTS) ? players[i].spheres : players[i].rings;
			}
		}
		else
		{
			if (!(actor && actor->player))
				return false; // no player to count rings from here, sorry

			rings = (maptol & TOL_NIGHTS) ? actor->player->spheres : actor->player->rings;
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
			if ((mo->flags & MF_PUSHABLE) || ((mo->info->flags & MF_PUSHABLE) && mo->fuse))
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
		case 323: // nightserize - each time
		case 324: // nightserize - once
		case 325: // denightserize - each time
		case 326: // denightserize - once
		case 327: // nights lap - each time
		case 328: // nights lap - once
		case 329: // nights egg capsule touch - each time
		case 330: // nights egg capsule touch - once
			if (!P_CheckNightsTriggerLine(triggerline, actor))
				return false;
			break;
		case 331: // continuous
		case 332: // each time
		case 333: // once
			if (!(actor && actor->player && ((stricmp(triggerline->text, skins[actor->player->skin].name) == 0) ^ ((triggerline->flags & ML_NOCLIMB) == ML_NOCLIMB))))
				return false;
			break;
		case 334: // object dye - continuous
		case 335: // object dye - each time
		case 336: // object dye - once
			{
				INT32 triggercolor = (INT32)sides[triggerline->sidenum[0]].toptexture;
				UINT16 color = (actor->player ? actor->player->powers[pw_dye] : actor->color);
				boolean invert = (triggerline->flags & ML_NOCLIMB ? true : false);

				if (invert ^ (triggercolor != color))
					return false;
			}
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
				if (ctlsector->lines[i]->executordelay)
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
				if (ctlsector->lines[i]->executordelay)
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
	 || specialtype == 313  // No More Enemies - Once
	 || specialtype == 315  // No of pushables - Once
	 || specialtype == 318  // Unlockable trigger - Once
	 || specialtype == 320  // Unlockable - Once
	 || specialtype == 321 || specialtype == 322 // Trigger on X calls - Continuous + Each Time
	 || specialtype == 324 // Nightserize - Once
	 || specialtype == 326 // DeNightserize - Once
	 || specialtype == 328 // Nights lap - Once
	 || specialtype == 330 // Nights Bonus Time - Once
	 || specialtype == 333 // Skin - Once
	 || specialtype == 336 // Dye - Once
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
		if (Tag_FGet(&lines[masterline].tags) != tag)
			continue;

		// "No More Enemies" and "Level Load" take care of themselves.
		if (lines[masterline].special == 313
		 || lines[masterline].special == 399
		 // Each-time executors handle themselves, too
		 || lines[masterline].special == 301 // Each time
		 || lines[masterline].special == 306 // Character ability - Each time
		 || lines[masterline].special == 310 // CTF Red team - Each time
		 || lines[masterline].special == 312 // CTF Blue team - Each time
		 || lines[masterline].special == 322 // Trigger on X calls - Each Time
		 || lines[masterline].special == 332 // Skin - Each time
		 || lines[masterline].special == 335)// Dye - Each time
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

		for (think = thlist[THINK_PRECIP].next; think != &thlist[THINK_PRECIP]; think = think->next)
		{
			if (think->function.acp1 != (actionf_p1)P_NullPrecipThinker)
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

		for (think = thlist[THINK_PRECIP].next; think != &thlist[THINK_PRECIP]; think = think->next)
		{
			if (think->function.acp1 != (actionf_p1)P_NullPrecipThinker)
				continue; // not a precipmobj thinker
			precipmobj = (precipmobj_t *)think;

			if (swap == PRECIP_RAIN) // Snow To Rain
			{
				precipmobj->flags = mobjinfo[MT_RAIN].flags;
				st = &states[mobjinfo[MT_RAIN].spawnstate];
				precipmobj->state = st;
				precipmobj->tics = st->tics;
				precipmobj->sprite = st->sprite;
				precipmobj->frame = st->frame;
				precipmobj->momz = mobjinfo[MT_RAIN].speed;

				precipmobj->precipflags &= ~PCF_INVISIBLE;

				precipmobj->precipflags |= PCF_RAIN;
				//think->function.acp1 = (actionf_p1)P_RainThinker;
			}
			else if (swap == PRECIP_SNOW) // Rain To Snow
			{
				INT32 z;

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

				precipmobj->precipflags &= ~(PCF_INVISIBLE|PCF_RAIN);

				//think->function.acp1 = (actionf_p1)P_SnowThinker;
			}
			else if (swap == PRECIP_BLANK || swap == PRECIP_STORM_NORAIN) // Remove precip, but keep it around for reuse.
			{
				//think->function.acp1 = (actionf_p1)P_NullPrecipThinker;

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
	mtag_t tag = Tag_FGet(&line->tags);
	TAG_ITER_DECLARECOUNTER(0);

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
				TAG_ITER_SECTORS(0, tag, secnum)
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
			TAG_ITER_SECTORS(0, tag, secnum)
				Tag_SectorFSet(secnum,(INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS));
			break;
		}

		case 410: // Change front sector's tag
			Tag_SectorFSet((UINT32)(line->frontsector - sectors), (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS));
			break;

		case 411: // Stop floor/ceiling movement in tagged sector(s)
			TAG_ITER_SECTORS(0, tag, secnum)
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
					if ((secnum = Tag_Iterate_Sectors(tag, 0)) < 0)
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
			if ((line->flags & ML_NOCLIMB) || (mo && mo->player && P_IsLocalPlayer(mo->player)) || titlemapinaction)
			{
				boolean musicsame = (!sides[line->sidenum[0]].text[0] || !strnicmp(sides[line->sidenum[0]].text, S_MusicName(), 7));
				UINT16 tracknum = (UINT16)max(sides[line->sidenum[0]].bottomtexture, 0);
				INT32 position = (INT32)max(sides[line->sidenum[0]].midtexture, 0);
				UINT32 prefadems = (UINT32)max(sides[line->sidenum[0]].textureoffset >> FRACBITS, 0);
				UINT32 postfadems = (UINT32)max(sides[line->sidenum[0]].rowoffset >> FRACBITS, 0);
				UINT8 fadetarget = (UINT8)max((line->sidenum[1] != 0xffff) ? sides[line->sidenum[1]].textureoffset >> FRACBITS : 0, 0);
				INT16 fadesource = (INT16)max((line->sidenum[1] != 0xffff) ? sides[line->sidenum[1]].rowoffset >> FRACBITS : -1, -1);

				// Seek offset from current song position
				if (line->flags & ML_EFFECT1)
				{
					// adjust for loop point if subtracting
					if (position < 0 && S_GetMusicLength() &&
						S_GetMusicPosition() > S_GetMusicLoopPoint() &&
						S_GetMusicPosition() + position < S_GetMusicLoopPoint())
						position = max(S_GetMusicLength() - (S_GetMusicLoopPoint() - (S_GetMusicPosition() + position)), 0);
					else
						position = max(S_GetMusicPosition() + position, 0);
				}

				// Fade current music to target volume (if music won't be changed)
				if ((line->flags & ML_EFFECT2) && fadetarget && musicsame)
				{
					// 0 fadesource means fade from current volume.
					// meaning that we can't specify volume 0 as the source volume -- this starts at 1.
					if (!fadesource)
						fadesource = -1;

					if (!postfadems)
						S_SetInternalMusicVolume(fadetarget);
					else
						S_FadeMusicFromVolume(fadetarget, fadesource, postfadems);

					if (position)
						S_SetMusicPosition(position);
				}
				// Change the music and apply position/fade operations
				else
				{
					strncpy(mapmusname, sides[line->sidenum[0]].text, 7);
					mapmusname[6] = 0;

					mapmusflags = tracknum & MUSIC_TRACKMASK;
					if (!(line->flags & ML_BLOCKMONSTERS))
						mapmusflags |= MUSIC_RELOADRESET;
					if (line->flags & ML_BOUNCY)
						mapmusflags |= MUSIC_FORCERESET;

					mapmusposition = position;

					S_ChangeMusicEx(mapmusname, mapmusflags, !(line->flags & ML_EFFECT4), position,
						!(line->flags & ML_EFFECT2) ? prefadems : 0,
						!(line->flags & ML_EFFECT2) ? postfadems : 0);

					if ((line->flags & ML_EFFECT2) && fadetarget)
					{
						if (!postfadems)
							S_SetInternalMusicVolume(fadetarget);
						else
							S_FadeMusicFromVolume(fadetarget, fadesource, postfadems);
					}
				}

				// Except, you can use the ML_BLOCKMONSTERS flag to change this behavior.
				// if (mapmusflags & MUSIC_RELOADRESET) then it will reset the music in G_PlayerReborn.
			}
			break;

		case 414: // Play SFX
			{
				INT32 sfxnum;

				sfxnum = sides[line->sidenum[0]].toptexture;

				if (sfxnum == sfx_None)
					return; // Do nothing!
				if (sfxnum < sfx_None || sfxnum >= NUMSFX)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 414 Executor: sfx number %d is invalid!\n", sfxnum);
					return;
				}

				if (tag != 0) // Do special stuff only if a non-zero linedef tag is set
				{
					// Play sounds from tagged sectors' origins.
					if (line->flags & ML_EFFECT5) // Repeat Midtexture
					{
						// Additionally play the sound from tagged sectors' soundorgs
						sector_t *sec;

						TAG_ITER_SECTORS(0, tag, secnum)
						{
							sec = &sectors[secnum];
							S_StartSound(&sec->soundorg, sfxnum);
						}
					}

					// Play the sound without origin for anyone, as long as they're inside tagged areas.
					else
					{
						UINT8 i = 0;
						mobj_t* camobj = players[displayplayer].mo;
						ffloor_t *rover;
						boolean foundit = false;

						for (i = 0; i < 2; camobj = players[secondarydisplayplayer].mo, i++)
						{
							if (!camobj)
								continue;

							if (foundit || Tag_Find(&camobj->subsector->sector->tags, tag))
							{
								foundit = true;
								break;
							}

							// Only trigger if mobj is touching the tag
							for(rover = camobj->subsector->sector->ffloors; rover; rover = rover->next)
							{
								if (!Tag_Find(&rover->master->frontsector->tags, tag))
									continue;

								if (camobj->z > P_GetSpecialTopZ(camobj, sectors + rover->secnum, camobj->subsector->sector))
									continue;

								if (camobj->z + camobj->height < P_GetSpecialBottomZ(camobj, sectors + rover->secnum, camobj->subsector->sector))
									continue;

								foundit = true;
								break;
							}
						}

						if (foundit)
							S_StartSound(NULL, sfxnum);
					}
				}
				else
				{
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
			TAG_ITER_SECTORS(0, tag, secnum)
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
			TAG_ITER_SECTORS(0, tag, secnum)
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
			TAG_ITER_SECTORS(0, tag, secnum)
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
			TAG_ITER_SECTORS(0, tag, secnum)
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
			P_FadeLight(tag,
				(line->flags & ML_DONTPEGBOTTOM) ? max(sides[line->sidenum[0]].textureoffset>>FRACBITS, 0) : line->frontsector->lightlevel,
				// failsafe: if user specifies Back Y Offset and NOT Front Y Offset, use the Back Offset
				// to be consistent with other light and fade specials
				(line->flags & ML_DONTPEGBOTTOM) ?
					((line->sidenum[1] != 0xFFFF && !(sides[line->sidenum[0]].rowoffset>>FRACBITS)) ?
						max(min(sides[line->sidenum[1]].rowoffset>>FRACBITS, 255), 0)
						: max(min(sides[line->sidenum[0]].rowoffset>>FRACBITS, 255), 0))
					: abs(P_AproxDistance(line->dx, line->dy))>>FRACBITS,
				(line->flags & ML_EFFECT4),
				(line->flags & ML_EFFECT5));
			break;

		case 421: // Stop lighting effect in tagged sectors
			TAG_ITER_SECTORS(0, tag, secnum)
				if (sectors[secnum].lightingdata)
				{
					P_RemoveThinker(&((elevator_t *)sectors[secnum].lightingdata)->thinker);
					sectors[secnum].lightingdata = NULL;
				}
			break;

		case 422: // Cut away to another view
			{
				mobj_t *altview;

				if ((!mo || !mo->player) && !titlemapinaction) // only players have views, and title screens
					return;

				if ((secnum = Tag_Iterate_Sectors(tag, 0)) < 0)
					return;

				altview = P_GetObjectTypeInSectorNum(MT_ALTVIEWMAN, secnum);
				if (!altview)
					return;

				// If titlemap, set the camera ref for title's thinker
				// This is not revoked until overwritten; awayviewtics is ignored
				if (titlemapinaction)
					titlemapcameraref = altview;
				else
				{
					P_SetTarget(&mo->player->awayviewmobj, altview);
					mo->player->awayviewtics = P_AproxDistance(line->dx, line->dy)>>FRACBITS;
				}


				if (line->flags & ML_NOCLIMB) // lets you specify a vertical angle
				{
					INT32 aim;

					aim = sides[line->sidenum[0]].textureoffset>>FRACBITS;
					aim = (aim + 360) % 360;
					aim *= (ANGLE_90>>8);
					aim /= 90;
					aim <<= 8;
					if (titlemapinaction)
						titlemapcameraref->cusval = (angle_t)aim;
					else
						mo->player->awayviewaiming = (angle_t)aim;
				}
				else
				{
					// straight ahead
					if (!titlemapinaction)
						mo->player->awayviewaiming = 0;
					// don't do cusval cause that's annoying
				}
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
			if (mo && mo->player)
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
			if (mo && mo->player)
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

				for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)T_Scroll)
						continue;

					scroller = (scroll_t *)th;
					if (!Tag_Find(&sectors[scroller->affectee].tags, tag))
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
				boolean foundrover = false; // for debug, "Can't find a FOF" message

				TAG_ITER_SECTORS(0, sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 436 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							EV_CrumbleChain(sec, rover);
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 436 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}
				}
			}
			break;

		case 437: // Disable Player Controls
			if (mo && mo->player)
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

					if (!Tag_Find(&lines[linenum].tags, tag))
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
						S_StartSound(NULL, sfx_s3k68);
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

			TAG_ITER_SECTORS(0, tag, secnum)
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
			if (line->stringargs[0])
				LUAh_LinedefExecute(line, mo, callsec);
			else
				CONS_Alert(CONS_WARNING, "Linedef %s is missing the hook name of the Lua function to call! (This should be given in arg0str)\n", sizeu1(line-lines));
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
				boolean foundrover = false; // for debug, "Can't find a FOF" message
				ffloortype_e oldflags; // store FOF's old flags

				TAG_ITER_SECTORS(0, sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 445 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							oldflags = rover->flags;

							// Abracadabra!
							if (line->flags & ML_NOCLIMB)
								rover->flags |= FF_EXISTS;
							else
								rover->flags &= ~FF_EXISTS;

							// if flags changed, reset sector's light list
							if (rover->flags != oldflags)
							{
								sec->moved = true;
								P_RecalcPrecipInSector(sec);
							}
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 445 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}
				}
			}
			break;

		case 446: // Make block fall remotely (acts like FF_CRUMBLE)
			{
				INT16 sectag = (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
				INT16 foftag = (INT16)(sides[line->sidenum[0]].rowoffset>>FRACBITS);
				sector_t *sec; // Sector that the FOF is visible in
				ffloor_t *rover; // FOF that we are going to make fall down
				boolean foundrover = false; // for debug, "Can't find a FOF" message
				player_t *player = NULL; // player that caused FOF to fall
				boolean respawn = true; // should the fallen FOF respawn?

				if (mo) // NULL check
					player = mo->player;

				if (line->flags & ML_NOCLIMB) // don't respawn!
					respawn = false;

				TAG_ITER_SECTORS(0, sectag, secnum)
				{
					sec = sectors + secnum;

					if (!sec->ffloors)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 446 Executor: Target sector #%d has no FOFs.\n", secnum);
						return;
					}

					for (rover = sec->ffloors; rover; rover = rover->next)
					{
						if (Tag_Find(&rover->master->frontsector->tags, foftag))
						{
							foundrover = true;

							if (line->flags & ML_BLOCKMONSTERS) // FOF flags determine respawn ability instead?
								respawn = !(rover->flags & FF_NORETURN) ^ !!(line->flags & ML_NOCLIMB); // no climb inverts

							EV_StartCrumble(rover->master->frontsector, rover, (rover->flags & FF_FLOATBOB), player, rover->alpha, respawn);
						}
					}

					if (!foundrover)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 446 Executor: Can't find a FOF control sector with tag %d\n", foftag);
						return;
					}
				}
			}
			break;

		case 447: // Change colormap of tagged sectors!
			// Basically this special applies a colormap to the tagged sectors, just like 606 (the colormap linedef)
			// Except it is activated by linedef executor, not level load
			// This could even override existing colormaps I believe
			// -- Monster Iestyn 14/06/18
		{
			extracolormap_t *source;
			if (!udmf)
				source = sides[line->sidenum[0]].colormap_data;
			else
			{
				if (!line->args[1])
					source = line->frontsector->extra_colormap;
				else
				{
					INT32 sourcesec = Tag_Iterate_Sectors(line->args[1], 0);
					if (sourcesec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 447 Executor: Can't find sector with source colormap (tag %d)!\n", line->args[1]);
						return;
					}
					source = sectors[sourcesec].extra_colormap;
				}
			}
			TAG_ITER_SECTORS(0, line->args[0], secnum)
			{
				if (sectors[secnum].colormap_protected)
					continue;

				P_ResetColormapFader(&sectors[secnum]);

				if (line->args[2] & TMCF_RELATIVE)
				{
					extracolormap_t *target = (!udmf && (line->flags & ML_TFERLINE) && line->sidenum[1] != 0xFFFF) ?
						sides[line->sidenum[1]].colormap_data : sectors[secnum].extra_colormap; // use back colormap instead of target sector

						extracolormap_t *exc = R_AddColormaps(
							target,
							source,
							line->args[2] & TMCF_SUBLIGHTR,
							line->args[2] & TMCF_SUBLIGHTG,
							line->args[2] & TMCF_SUBLIGHTB,
							line->args[2] & TMCF_SUBLIGHTA,
							line->args[2] & TMCF_SUBFADER,
							line->args[2] & TMCF_SUBFADEG,
							line->args[2] & TMCF_SUBFADEB,
							line->args[2] & TMCF_SUBFADEA,
							line->args[2] & TMCF_SUBFADESTART,
							line->args[2] & TMCF_SUBFADEEND,
							line->args[2] & TMCF_IGNOREFLAGS,
							false);

					if (!(sectors[secnum].extra_colormap = R_GetColormapFromList(exc)))
					{
						exc->colormap = R_CreateLightTable(exc);
						R_AddColormapToList(exc);
						sectors[secnum].extra_colormap = exc;
					}
					else
						Z_Free(exc);
				}
				else
					sectors[secnum].extra_colormap = source;
			}
			break;
		}
		case 448: // Change skybox viewpoint/centerpoint
			if ((mo && mo->player && P_IsLocalPlayer(mo->player)) || (line->flags & ML_NOCLIMB))
			{
				INT32 viewid = sides[line->sidenum[0]].textureoffset>>FRACBITS;
				INT32 centerid = sides[line->sidenum[0]].rowoffset>>FRACBITS;

				if ((line->flags & (ML_EFFECT4|ML_BLOCKMONSTERS)) == ML_EFFECT4) // Solid Midtexture is on but Block Enemies is off?
				{
					CONS_Alert(CONS_WARNING,
					M_GetText("Skybox switch linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"),
					tag);
				}
				else
				{
					// set viewpoint mobj
					if (!(line->flags & ML_EFFECT4)) // Solid Midtexture turns off viewpoint setting
					{
						if (viewid >= 0 && viewid < 16)
							skyboxmo[0] = skyboxviewpnts[viewid];
						else
							skyboxmo[0] = NULL;
					}

					// set centerpoint mobj
					if (line->flags & ML_BLOCKMONSTERS) // Block Enemies turns ON centerpoint setting
					{
						if (centerid >= 0 && centerid < 16)
							skyboxmo[1] = skyboxcenterpnts[centerid];
						else
							skyboxmo[1] = NULL;
					}
				}

				CONS_Debug(DBG_GAMELOGIC, "Line type 448 Executor: viewid = %d, centerid = %d, viewpoint? = %s, centerpoint? = %s\n",
						viewid,
						centerid,
						((line->flags & ML_EFFECT4) ? "no" : "yes"),
						((line->flags & ML_BLOCKMONSTERS) ? "yes" : "no"));
			}
			break;

		case 449: // Enable bosses with parameter
		{
			INT32 bossid = sides[line->sidenum[0]].textureoffset>>FRACBITS;
			if (bossid & ~15) // if any bits other than first 16 are set
			{
				CONS_Alert(CONS_WARNING,
					M_GetText("Boss enable linedef (tag %d) has an invalid texture x offset.\nConsider changing it or removing it entirely.\n"),
					tag);
				break;
			}
			if (line->flags & ML_NOCLIMB)
			{
				bossdisabled |= (1<<bossid);
				CONS_Debug(DBG_GAMELOGIC, "Line type 449 Executor: bossid disabled = %d", bossid);
			}
			else
			{
				bossdisabled &= ~(1<<bossid);
				CONS_Debug(DBG_GAMELOGIC, "Line type 449 Executor: bossid enabled = %d", bossid);
			}
			break;
		}

		case 450: // Execute Linedef Executor - for recursion
			P_LinedefExecute(tag, mo, NULL);
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

		case 452: // Set FOF alpha
		{
			INT16 destvalue = line->sidenum[1] != 0xffff ?
				(INT16)(sides[line->sidenum[1]].textureoffset>>FRACBITS) : (INT16)(P_AproxDistance(line->dx, line->dy)>>FRACBITS);
			INT16 sectag = (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
			INT16 foftag = (INT16)(sides[line->sidenum[0]].rowoffset>>FRACBITS);
			sector_t *sec; // Sector that the FOF is visible in
			ffloor_t *rover; // FOF that we are going to operate
			boolean foundrover = false; // for debug, "Can't find a FOF" message

			TAG_ITER_SECTORS(0, sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 452 Executor: Target sector #%d has no FOFs.\n", secnum);
					return;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						// If fading an invisible FOF whose render flags we did not yet set,
						// initialize its alpha to 1
						// for relative alpha calc
						if (!(line->flags & ML_NOCLIMB) &&      // do translucent
							(rover->spawnflags & FF_NOSHADE) && // do not include light blocks, which don't set FF_NOSHADE
							!(rover->spawnflags & FF_RENDERSIDES) &&
							!(rover->spawnflags & FF_RENDERPLANES) &&
							!(rover->flags & FF_RENDERALL))
							rover->alpha = 1;

						P_RemoveFakeFloorFader(rover);
						P_FadeFakeFloor(rover,
							rover->alpha,
							max(1, min(256, (line->flags & ML_EFFECT3) ? rover->alpha + destvalue : destvalue)),
							0,                                  // set alpha immediately
							false, NULL,                        // tic-based logic
							false,                              // do not handle FF_EXISTS
							!(line->flags & ML_NOCLIMB),        // handle FF_TRANSLUCENT
							false,                              // do not handle lighting
							false,                              // do not handle colormap
							false,                              // do not handle collision
							false,                              // do not do ghost fade (no collision during fade)
							true);                               // use exact alpha values (for opengl)
					}
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 452 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return;
				}
			}
			break;
		}

		case 453: // Fade FOF
		{
			INT16 destvalue = line->sidenum[1] != 0xffff ?
				(INT16)(sides[line->sidenum[1]].textureoffset>>FRACBITS) : (INT16)(line->dx>>FRACBITS);
			INT16 speed = line->sidenum[1] != 0xffff ?
				(INT16)(abs(sides[line->sidenum[1]].rowoffset>>FRACBITS)) : (INT16)(abs(line->dy)>>FRACBITS);
			INT16 sectag = (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
			INT16 foftag = (INT16)(sides[line->sidenum[0]].rowoffset>>FRACBITS);
			sector_t *sec; // Sector that the FOF is visible in
			ffloor_t *rover; // FOF that we are going to operate
			boolean foundrover = false; // for debug, "Can't find a FOF" message
			size_t j = 0; // sec->ffloors is saved as ffloor #0, ss->ffloors->next is #1, etc

			TAG_ITER_SECTORS(0, sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 453 Executor: Target sector #%d has no FOFs.\n", secnum);
					return;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						// Prevent continuous execs from interfering on an existing fade
						if (!(line->flags & ML_EFFECT5)
							&& rover->fadingdata)
							//&& ((fade_t*)rover->fadingdata)->timer > (ticbased ? 2 : speed*2))
						{
							CONS_Debug(DBG_GAMELOGIC, "Line type 453 Executor: Fade FOF thinker already exists, timer: %d\n", ((fade_t*)rover->fadingdata)->timer);
							continue;
						}

						if (speed > 0)
							P_AddFakeFloorFader(rover, secnum, j,
								destvalue,
								speed,
								(line->flags & ML_EFFECT4),         // tic-based logic
								(line->flags & ML_EFFECT3),         // Relative destvalue
								!(line->flags & ML_BLOCKMONSTERS),  // do not handle FF_EXISTS
								!(line->flags & ML_NOCLIMB),        // do not handle FF_TRANSLUCENT
								!(line->flags & ML_EFFECT2),        // do not handle lighting
								!(line->flags & ML_EFFECT2),        // do not handle colormap (ran out of flags)
								!(line->flags & ML_BOUNCY),         // do not handle collision
								(line->flags & ML_EFFECT1),         // do ghost fade (no collision during fade)
								(line->flags & ML_TFERLINE));       // use exact alpha values (for opengl)
						else
						{
							// If fading an invisible FOF whose render flags we did not yet set,
							// initialize its alpha to 1
							// for relative alpha calc
							if (!(line->flags & ML_NOCLIMB) &&      // do translucent
								(rover->spawnflags & FF_NOSHADE) && // do not include light blocks, which don't set FF_NOSHADE
								!(rover->spawnflags & FF_RENDERSIDES) &&
								!(rover->spawnflags & FF_RENDERPLANES) &&
								!(rover->flags & FF_RENDERALL))
								rover->alpha = 1;

							P_RemoveFakeFloorFader(rover);
							P_FadeFakeFloor(rover,
								rover->alpha,
								max(1, min(256, (line->flags & ML_EFFECT3) ? rover->alpha + destvalue : destvalue)),
								0,                                  // set alpha immediately
								false, NULL,                        // tic-based logic
								!(line->flags & ML_BLOCKMONSTERS),  // do not handle FF_EXISTS
								!(line->flags & ML_NOCLIMB),        // do not handle FF_TRANSLUCENT
								!(line->flags & ML_EFFECT2),        // do not handle lighting
								!(line->flags & ML_EFFECT2),        // do not handle colormap (ran out of flags)
								!(line->flags & ML_BOUNCY),         // do not handle collision
								(line->flags & ML_EFFECT1),         // do ghost fade (no collision during fade)
								(line->flags & ML_TFERLINE));       // use exact alpha values (for opengl)
						}
					}
					j++;
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 453 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return;
				}
			}
			break;
		}

		case 454: // Stop fading FOF
		{
			INT16 sectag = (INT16)(sides[line->sidenum[0]].textureoffset>>FRACBITS);
			INT16 foftag = (INT16)(sides[line->sidenum[0]].rowoffset>>FRACBITS);
			sector_t *sec; // Sector that the FOF is visible in
			ffloor_t *rover; // FOF that we are going to operate
			boolean foundrover = false; // for debug, "Can't find a FOF" message

			TAG_ITER_SECTORS(0, sectag, secnum)
			{
				sec = sectors + secnum;

				if (!sec->ffloors)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 454 Executor: Target sector #%d has no FOFs.\n", secnum);
					return;
				}

				for (rover = sec->ffloors; rover; rover = rover->next)
				{
					if (Tag_Find(&rover->master->frontsector->tags, foftag))
					{
						foundrover = true;

						P_ResetFakeFloorFader(rover, NULL,
							!(line->flags & ML_BLOCKMONSTERS)); // do not finalize collision flags
					}
				}

				if (!foundrover)
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 454 Executor: Can't find a FOF control sector with tag %d\n", foftag);
					return;
				}
			}
			break;
		}

		case 455: // Fade colormap
		{
			extracolormap_t *dest;
			if (!udmf)
				dest = sides[line->sidenum[0]].colormap_data;
			else
			{
				if (!line->args[1])
					dest = line->frontsector->extra_colormap;
				else
				{
					INT32 destsec = Tag_Iterate_Sectors(line->args[1], 0);
					if (destsec == -1)
					{
						CONS_Debug(DBG_GAMELOGIC, "Line type 455 Executor: Can't find sector with destination colormap (tag %d)!\n", line->args[1]);
						return;
					}
					dest = sectors[destsec].extra_colormap;
				}
			}

			TAG_ITER_SECTORS(0, line->args[0], secnum)
			{
				extracolormap_t *source_exc, *dest_exc, *exc;

				if (sectors[secnum].colormap_protected)
					continue;

				// Don't interrupt ongoing fade
				if (!(line->args[3] & TMCF_OVERRIDE)
					&& sectors[secnum].fadecolormapdata)
					//&& ((fadecolormap_t*)sectors[secnum].fadecolormapdata)->timer > (ticbased ? 2 : speed*2))
				{
					CONS_Debug(DBG_GAMELOGIC, "Line type 455 Executor: Fade color thinker already exists, timer: %d\n", ((fadecolormap_t*)sectors[secnum].fadecolormapdata)->timer);
					continue;
				}

				if (!udmf && (line->flags & ML_TFERLINE)) // use back colormap instead of target sector
					sectors[secnum].extra_colormap = (line->sidenum[1] != 0xFFFF) ?
					sides[line->sidenum[1]].colormap_data : NULL;

				exc = sectors[secnum].extra_colormap;

				if (!(line->args[3] & TMCF_FROMBLACK) // Override fade from default rgba
					&& !R_CheckDefaultColormap(dest, true, false, false)
					&& R_CheckDefaultColormap(exc, true, false, false))
				{
					exc = R_CopyColormap(exc, false);
					exc->rgba = R_GetRgbaRGB(dest->rgba) + R_PutRgbaA(R_GetRgbaA(exc->rgba));
					//exc->fadergba = R_GetRgbaRGB(dest->rgba) + R_PutRgbaA(R_GetRgbaA(exc->fadergba));

					if (!(source_exc = R_GetColormapFromList(exc)))
					{
						exc->colormap = R_CreateLightTable(exc);
						R_AddColormapToList(exc);
						source_exc = exc;
					}
					else
						Z_Free(exc);

					sectors[secnum].extra_colormap = source_exc;
				}
				else
					source_exc = exc ? exc : R_GetDefaultColormap();

				if (line->args[3] & TMCF_RELATIVE)
				{
					exc = R_AddColormaps(
						source_exc,
						dest,
						line->args[3] & TMCF_SUBLIGHTR,
						line->args[3] & TMCF_SUBLIGHTG,
						line->args[3] & TMCF_SUBLIGHTB,
						line->args[3] & TMCF_SUBLIGHTA,
						line->args[3] & TMCF_SUBFADER,
						line->args[3] & TMCF_SUBFADEG,
						line->args[3] & TMCF_SUBFADEB,
						line->args[3] & TMCF_SUBFADEA,
						line->args[3] & TMCF_SUBFADESTART,
						line->args[3] & TMCF_SUBFADEEND,
						line->args[3] & TMCF_IGNOREFLAGS,
						false);
				}
				else
					exc = R_CopyColormap(dest, false);

				if (!(dest_exc = R_GetColormapFromList(exc)))
				{
					exc->colormap = R_CreateLightTable(exc);
					R_AddColormapToList(exc);
					dest_exc = exc;
				}
				else
					Z_Free(exc);

				Add_ColormapFader(&sectors[secnum], source_exc, dest_exc, true, // tic-based timing
					line->args[2]);
			}
			break;
		}
		case 456: // Stop fade colormap
			TAG_ITER_SECTORS(0, line->args[0], secnum)
				P_ResetColormapFader(&sectors[secnum]);
			break;

		case 457: // Track mobj angle to point
			if (mo)
			{
				INT32 failureangle = FixedAngle((min(max(abs(sides[line->sidenum[0]].textureoffset>>FRACBITS), 0), 360))*FRACUNIT);
				INT32 failuredelay = abs(sides[line->sidenum[0]].rowoffset>>FRACBITS);
				INT32 failureexectag = line->sidenum[1] != 0xffff ?
					(INT32)(sides[line->sidenum[1]].textureoffset>>FRACBITS) : 0;
				boolean persist = (line->flags & ML_EFFECT2);
				mobj_t *anchormo;

				if ((secnum = Tag_Iterate_Sectors(tag, 0)) < 0)
					return;

				anchormo = P_GetObjectTypeInSectorNum(MT_ANGLEMAN, secnum);
				if (!anchormo)
					return;

				mo->eflags |= MFE_TRACERANGLE;
				P_SetTarget(&mo->tracer, anchormo);
				mo->lastlook = persist; // don't disable behavior after first failure
				mo->extravalue1 = failureangle; // angle to exceed for failure state
				mo->extravalue2 = failureexectag; // exec tag for failure state (angle is not within range)
				mo->cusval = mo->cvmem = failuredelay; // cusval = tics to allow failure before line trigger; cvmem = decrement timer
			}
			break;

		case 458: // Stop tracking mobj angle to point
			if (mo && (mo->eflags & MFE_TRACERANGLE))
			{
				mo->eflags &= ~MFE_TRACERANGLE;
				P_SetTarget(&mo->tracer, NULL);
				mo->lastlook = mo->cvmem = mo->cusval = mo->extravalue1 = mo->extravalue2 = 0;
			}
			break;

		case 459: // Control Text Prompt
			// console player only unless NOCLIMB is set
			if (mo && mo->player && P_IsLocalPlayer(mo->player) && (!bot || bot != mo))
			{
				INT32 promptnum = max(0, (sides[line->sidenum[0]].textureoffset>>FRACBITS)-1);
				INT32 pagenum = max(0, (sides[line->sidenum[0]].rowoffset>>FRACBITS)-1);
				INT32 postexectag = abs((line->sidenum[1] != 0xFFFF) ? sides[line->sidenum[1]].textureoffset>>FRACBITS : tag);

				boolean closetextprompt = (line->flags & ML_BLOCKMONSTERS);
				//boolean allplayers = (line->flags & ML_NOCLIMB);
				boolean runpostexec = (line->flags & ML_EFFECT1);
				boolean blockcontrols = !(line->flags & ML_EFFECT2);
				boolean freezerealtime = !(line->flags & ML_EFFECT3);
				//boolean freezethinkers = (line->flags & ML_EFFECT4);
				boolean callbynamedtag = (line->flags & ML_TFERLINE);

				if (closetextprompt)
					F_EndTextPrompt(false, false);
				else
				{
					if (callbynamedtag && sides[line->sidenum[0]].text && sides[line->sidenum[0]].text[0])
						F_GetPromptPageByNamedTag(sides[line->sidenum[0]].text, &promptnum, &pagenum);
					F_StartTextPrompt(promptnum, pagenum, mo, runpostexec ? postexectag : 0, blockcontrols, freezerealtime);
				}
			}
			break;

		case 460: // Award rings
			{
				INT16 rings = (sides[line->sidenum[0]].textureoffset>>FRACBITS);
				INT32 delay = (sides[line->sidenum[0]].rowoffset>>FRACBITS);
				if (mo && mo->player)
				{
					if (delay <= 0 || !(leveltime % delay))
						P_GivePlayerRings(mo->player, rings);
				}
			}
			break;

		case 461: // Spawns an object on the map based on texture offsets
			{
				const mobjtype_t type = (mobjtype_t)(sides[line->sidenum[0]].toptexture);
				mobj_t *mobj;

				fixed_t x, y, z;
				x = sides[line->sidenum[0]].textureoffset;
				y = sides[line->sidenum[0]].rowoffset;
				z = line->frontsector->floorheight;

				if (line->flags & ML_NOCLIMB) // If noclimb is set, spawn randomly within a range
				{
					if (line->sidenum[1] != 0xffff) // Make sure the linedef has a back side
					{
						x = P_RandomRange(sides[line->sidenum[0]].textureoffset>>FRACBITS, sides[line->sidenum[1]].textureoffset>>FRACBITS)<<FRACBITS;
						y = P_RandomRange(sides[line->sidenum[0]].rowoffset>>FRACBITS, sides[line->sidenum[1]].rowoffset>>FRACBITS)<<FRACBITS;
						z = P_RandomRange(line->frontsector->floorheight>>FRACBITS, line->frontsector->ceilingheight>>FRACBITS)<<FRACBITS;
					}
					else
					{
						CONS_Alert(CONS_WARNING,"Linedef Type %d - Spawn Object: Linedef is set for random range but has no back side.\n", line->special);
						break;
					}
				}

				mobj = P_SpawnMobj(x, y, z, type);
				if (mobj)
				{
					if (line->flags & ML_EFFECT1)
						mobj->angle = R_PointToAngle2(line->v1->x, line->v1->y, line->v2->x, line->v2->y);
					CONS_Debug(DBG_GAMELOGIC, "Linedef Type %d - Spawn Object: %d spawned at (%d, %d, %d)\n", line->special, mobj->type, mobj->x>>FRACBITS, mobj->y>>FRACBITS, mobj->z>>FRACBITS); //TODO: Convert mobj->type to a string somehow.
				}
				else
					CONS_Alert(CONS_ERROR,"Linedef Type %d - Spawn Object: Object did not spawn!\n", line->special);
			}
			break;

		case 462: // Stop clock (and end level in record attack)
			if (G_PlatformGametype())
			{
				stoppedclock = true;
				CONS_Debug(DBG_GAMELOGIC, "Clock stopped!\n");
				if (modeattacking)
				{
					UINT8 i;
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;
						P_DoPlayerExit(&players[i]);
					}
				}
			}
			break;

		case 463: // Dye object
			{
				INT32 color = sides[line->sidenum[0]].toptexture;

				if (mo)
				{
					if (color < 0 || color >= numskincolors)
						return;

					var1 = 0;
					var2 = color;
					A_Dye(mo);
				}
			}
			break;

		case 464: // Trigger Egg Capsule
			{
				thinker_t *th;
				mobj_t *mo2;

				// Find the center of the Eggtrap and release all the pretty animals!
				// The chimps are my friends.. heeheeheheehehee..... - LouisJM
				for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
				{
					if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
						continue;

					mo2 = (mobj_t *)th;

					if (mo2->type != MT_EGGTRAP)
						continue;

					if (!mo2->spawnpoint)
						continue;

					if (mo2->spawnpoint->angle != tag)
						continue;

					P_KillMobj(mo2, NULL, mo, 0);
				}

				if (!(line->flags & ML_NOCLIMB))
				{
					INT32 i;

					// Mark all players with the time to exit thingy!
					for (i = 0; i < MAXPLAYERS; i++)
					{
						if (!playeringame[i])
							continue;
						P_DoPlayerExit(&players[i]);
					}
				}
			}
			break;

		case 465: // Set linedef executor delay
			{
				INT32 linenum;
				TAG_ITER_DECLARECOUNTER(1);

				if (!udmf)
					break;

				TAG_ITER_LINES(1, line->args[0], linenum)
				{
					if (line->args[2])
						lines[linenum].executordelay += line->args[1];
					else
						lines[linenum].executordelay = line->args[1];
				}
			}
			break;

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
		case 492:
			PolyFade(line);
			break;

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

		if (!numfound
			&& !(player->mo->target && player->mo->target->type == MT_SIGN)
			&& !((gametyperules & GTR_FRIENDLY) && (netgame || multiplayer) && cv_exitmove.value))
				P_SetTarget(&player->mo->target, thing);

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetTarget(&thing->target, player->mo);
		P_SetObjectMomZ(thing, 12*FRACUNIT, false);
		P_SetMobjState(thing, S_SIGNSPIN1);
		if (thing->info->seesound)
			S_StartSound(thing, thing->info->seesound);

		++numfound;
	}

	if (numfound)
		return;

	// didn't find any signposts in the exit sector.
	// spin all signposts in the level then.
	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		thing = (mobj_t *)think;
		if (thing->type != MT_SIGN)
			continue;

		if (!numfound
			&& !(player->mo->target && player->mo->target->type == MT_SIGN)
			&& !((gametyperules & GTR_FRIENDLY) && (netgame || multiplayer) && cv_exitmove.value))
				P_SetTarget(&player->mo->target, thing);

		if (thing->state != &states[thing->info->spawnstate])
			continue;

		P_SetTarget(&thing->target, player->mo);
		P_SetObjectMomZ(thing, 12*FRACUNIT, false);
		P_SetMobjState(thing, S_SIGNSPIN1);
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
	INT32 specialnum = (flag == MT_REDFLAG) ? 3 : 4;

	for (think = thlist[THINK_MOBJ].next; think != &thlist[THINK_MOBJ]; think = think->next)
	{
		if (think->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
			continue;

		mo = (mobj_t *)think;

		if (mo->type != flag)
			continue;

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

				if (!(mo->z <= P_GetSpecialTopZ(mo, sectors + rover->secnum, mo->subsector->sector)
					&& mo->z >= P_GetSpecialBottomZ(mo, sectors + rover->secnum, mo->subsector->sector)))
					continue;

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
			boolean floorallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z == topheight));
			boolean ceilingallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z + player->mo->height == bottomheight));
			// Thing must be on top of the floor to be affected...
			if (!(floorallowed || ceilingallowed))
				continue;
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
				boolean floorallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z == topheight));
				boolean ceilingallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z + player->mo->height == bottomheight));
				// Thing must be on top of the floor to be affected...
				if (!(floorallowed || ceilingallowed))
					continue;
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
			boolean floorallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(mo->eflags & MFE_VERTICALFLIP)) && (mo->z == top));
			boolean ceilingallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (mo->eflags & MFE_VERTICALFLIP)) && (mo->z + mo->height == bottom));
			// Thing must be on top of the floor to be affected...
			if (!(floorallowed || ceilingallowed))
				continue;
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
	boolean floorallowed = ((sec->flags & SF_FLIPSPECIAL_FLOOR) && ((sec->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(mo->eflags & MFE_VERTICALFLIP)) && (mo->z == P_GetSpecialBottomZ(mo, sec, sec)));
	boolean ceilingallowed = ((sec->flags & SF_FLIPSPECIAL_CEILING) && ((sec->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (mo->eflags & MFE_VERTICALFLIP)) && (mo->z + mo->height == P_GetSpecialTopZ(mo, sec, sec)));
	// Thing must be on top of the floor to be affected...
	return (floorallowed || ceilingallowed);
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
	mtag_t sectag = Tag_FGet(&sector->tags);

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
	if (player->playerstate != PST_LIVE)
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
				P_DamageMobj(player->mo, NULL, NULL, 1, 0);
			break;
		case 2: // Damage (Water)
			if ((roversector || P_MobjReadyToTrigger(player->mo, sector)) && (player->powers[pw_underwater] || player->powers[pw_carry] == CR_NIGHTSMODE))
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_WATER);
			break;
		case 3: // Damage (Fire)
			if (roversector || P_MobjReadyToTrigger(player->mo, sector))
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_FIRE);
			break;
		case 4: // Damage (Electrical)
			if (roversector || P_MobjReadyToTrigger(player->mo, sector))
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_ELECTRIC);
			break;
		case 5: // Spikes
			if (roversector || P_MobjReadyToTrigger(player->mo, sector))
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_SPIKE);
			break;
		case 6: // Death Pit (Camera Mod)
		case 7: // Death Pit (No Camera Mod)
			if (roversector || P_MobjReadyToTrigger(player->mo, sector))
			{
				if (player->quittime)
					G_MovePlayerToSpawnOrStarpost(player - players);
				else
					P_DamageMobj(player->mo, NULL, NULL, 1, DMG_DEATHPIT);
			}
			break;
		case 8: // Instant Kill
			if (player->quittime)
				G_MovePlayerToSpawnOrStarpost(player - players);
			else
				P_DamageMobj(player->mo, NULL, NULL, 1, DMG_INSTAKILL);
			break;
		case 9: // Ring Drainer (Floor Touch)
		case 10: // Ring Drainer (No Floor Touch)
			if (leveltime % (TICRATE/2) == 0 && player->rings > 0)
			{
				player->rings--;
				S_StartSound(player->mo, sfx_antiri);
			}
			break;
		case 11: // Special Stage Damage
			if (player->exiting || player->bot) // Don't do anything for bots or players who have just finished
				break;

			if (!(player->powers[pw_shield] || player->spheres > 0)) // Don't do anything if no shield or spheres anyway
				break;

			P_SpecialStageDamage(player, NULL, NULL);
			break;
		case 12: // Space Countdown
			if (!(player->powers[pw_shield] & SH_PROTECTWATER) && !player->powers[pw_spacetime])
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
			{
				if (!playeringame[i])
					continue;
				if (!players[i].mo)
					continue;
				if (players[i].spectator)
					continue;
				if (players[i].bot)
					continue;
				if (G_CoopGametype() && players[i].lives <= 0)
					continue;
				if (roversector)
				{
					if (sector->flags & SF_TRIGGERSPECIAL_TOUCH)
					{
						msecnode_t *node;
						for (node = players[i].mo->touching_sectorlist; node; node = node->m_sectorlist_next)
						{
							if (P_ThingIsOnThe3DFloor(players[i].mo, sector, node->m_sector))
								break;
						}
						if (!node)
							goto DoneSection2;
					}
					else if (players[i].mo->subsector && !P_ThingIsOnThe3DFloor(players[i].mo, sector, players[i].mo->subsector->sector)) // this function handles basically everything for us lmao
						goto DoneSection2;
				}
				else
				{
					if (players[i].mo->subsector->sector == sector)
						;
					else if (sector->flags & SF_TRIGGERSPECIAL_TOUCH)
					{
						msecnode_t *node;
						for (node = players[i].mo->touching_sectorlist; node; node = node->m_sectorlist_next)
						{
							if (node->m_sector == sector)
								break;
						}
						if (!node)
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
				P_LinedefExecute(sectag, player->mo, sector);
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
			for (th = thlist[THINK_MOBJ].next; th != &thlist[THINK_MOBJ]; th = th->next)
			{
				if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					continue;
				mo2 = (mobj_t *)th;
				if (mo2->type != MT_EGGTRAP)
					continue;
				P_KillMobj(mo2, NULL, player->mo, 0);
			}

			// clear the special so you can't push the button twice.
			sector->special = 0;

			// Initialize my junk
			junk.tags.tags = NULL;
			junk.tags.count = 0;

			// Move the button down
			Tag_FSet(&junk.tags, LE_CAPSULE0);
			EV_DoElevator(&junk, elevateDown, false);

			// Open the top FOF
			Tag_FSet(&junk.tags, LE_CAPSULE1);
			EV_DoFloor(&junk, raiseFloorToNearestFast);
			// Open the bottom FOF
			Tag_FSet(&junk.tags, LE_CAPSULE2);
			EV_DoCeiling(&junk, lowerToLowestFast);

			// Mark all players with the time to exit thingy!
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				P_DoPlayerExit(&players[i]);
			}
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
		case 1: // Unused
		case 2: // Wind/Current
		case 3: // Unused
		case 4: // Conveyor Belt
			break;

		case 5: // Speed pad
			if (player->powers[pw_flashing] != 0 && player->powers[pw_flashing] < TICRATE/2)
				break;

			i = Tag_FindLineSpecial(4, sectag);

			if (i != -1)
			{
				angle_t lineangle;
				fixed_t linespeed;
				fixed_t sfxnum;

				lineangle = R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y);
				linespeed = sides[lines[i].sidenum[0]].textureoffset;

				if (linespeed == 0)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Speed pad (tag %d) at zero speed.\n", sectag);
					break;
				}

				player->mo->angle = player->drawangle = lineangle;

				if (!demoplayback || P_ControlStyle(player) == CS_LMAOGALOG)
					P_SetPlayerAngle(player, player->mo->angle);

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

				if (lines[i].flags & ML_EFFECT5) // Roll!
				{
					if (!(player->pflags & PF_SPINNING))
						player->pflags |= PF_SPINNING;

					P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
				}

				player->powers[pw_flashing] = TICRATE/3;

				sfxnum = sides[lines[i].sidenum[0]].toptexture;

				if (!sfxnum)
					sfxnum = sfx_spdpad;

				S_StartSound(player->mo, sfxnum);
			}
			break;

		case 6: // Unused
		case 7: // Unused
		case 8: // Unused
		case 9: // Unused
		case 10: // Unused
		case 11: // Unused
		case 12: // Unused
		case 13: // Unused
		case 14: // Unused
		case 15: // Unused
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

			P_TouchStarPost(post, player, false);
			break;
		}

		case 2: // Special stage GOAL sector / Exit Sector / CTF Flag Return
			if (player->bot || !(gametyperules & GTR_ALLOWEXIT))
				break;
			if (!(maptol & TOL_NIGHTS) && G_IsSpecialStage(gamemap) && player->nightstime > 6)
			{
				player->nightstime = 6; // Just let P_Ticker take care of the rest.
				return;
			}

			// Exit (for FOF exits; others are handled in P_PlayerThink in p_user.c)
			{
				INT32 lineindex;

				P_DoPlayerFinish(player);

				P_SetupSignExit(player);
				// important: use sector->tag on next line instead of player->mo->subsector->tag
				// this part is different from in P_PlayerThink, this is what was causing
				// FOF custom exits not to work.
				lineindex = Tag_FindLineSpecial(2, sectag);

				if (G_CoopGametype() && lineindex != -1) // Custom exit!
				{
					// Special goodies with the block monsters flag depending on emeralds collected
					if ((lines[lineindex].flags & ML_BLOCKMONSTERS) && ALL7EMERALDS(emeralds))
						nextmapoverride = (INT16)(lines[lineindex].frontsector->ceilingheight>>FRACBITS);
					else
						nextmapoverride = (INT16)(lines[lineindex].frontsector->floorheight>>FRACBITS);

					if (lines[lineindex].flags & ML_NOCLIMB)
						skipstats = 1;
				}
			}
			break;

		case 3: // Red Team's Base
			if ((gametyperules & GTR_TEAMFLAGS) && P_IsObjectOnGround(player->mo))
			{
				if (player->ctfteam == 1 && (player->gotflag & GF_BLUEFLAG))
				{
					mobj_t *mo;

					// Make sure the red team still has their own
					// flag at their base so they can score.
					if (!P_IsFlagAtBase(MT_REDFLAG))
						break;

					HU_SetCEchoFlags(V_AUTOFADEOUT|V_ALLOWLOWERCASE);
					HU_SetCEchoDuration(5);
					HU_DoCEcho(va(M_GetText("\205%s\200\\CAPTURED THE \204BLUE FLAG\200.\\\\\\\\"), player_names[player-players]));

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
			if ((gametyperules & GTR_TEAMFLAGS) && P_IsObjectOnGround(player->mo))
			{
				if (player->ctfteam == 2 && (player->gotflag & GF_REDFLAG))
				{
					mobj_t *mo;

					// Make sure the blue team still has their own
					// flag at their base so they can score.
					if (!P_IsFlagAtBase(MT_BLUEFLAG))
						break;

					HU_SetCEchoFlags(V_AUTOFADEOUT|V_ALLOWLOWERCASE);
					HU_SetCEchoDuration(5);
					HU_DoCEcho(va(M_GetText("\204%s\200\\CAPTURED THE \205RED FLAG\200.\\\\\\\\"), player_names[player-players]));

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
				P_SetPlayerMobjState(player->mo, S_PLAY_FALL);
			break;

		case 6: // Super Sonic transformer
			if (player->mo->health > 0 && !player->bot && (player->charflags & SF_SUPER) && !player->powers[pw_super] && ALL7EMERALDS(emeralds))
				P_DoSuperTransformation(player, true);
			break;

		case 7: // Make player spin
			if (!(player->pflags & PF_SPINNING))
			{
				player->pflags |= PF_SPINNING;
				P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
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
				mobj_t *waypoint = NULL;
				angle_t an;

				if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && player->powers[pw_carry] == CR_ZOOMTUBE)
					break;

				if (player->powers[pw_ignorelatch] & (1<<15))
					break;

				// Find line #3 tagged to this sector
				lineindex = Tag_FindLineSpecial(3, sectag);

				if (lineindex == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Sector special %d missing line special #3.\n", sector->special);
					break;
				}

				// Grab speed and sequence values
				speed = abs(sides[lines[lineindex].sidenum[0]].textureoffset)/8;
				sequence = abs(sides[lines[lineindex].sidenum[0]].rowoffset)>>FRACBITS;

				if (speed == 0)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Waypoint sequence %d at zero speed.\n", sequence);
					break;
				}

				waypoint = P_GetFirstWaypoint(sequence);

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
				player->powers[pw_carry] = CR_ZOOMTUBE;
				player->speed = speed;
				player->pflags |= PF_SPINNING;
				player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_BOUNCING|PF_SLIDING|PF_CANCARRY);
				player->climbing = 0;

				if (player->mo->state-states != S_PLAY_ROLL)
				{
					P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
					S_StartSound(player->mo, sfx_spin);
				}
			}
			break;

		case 9: // Zoom Tube End
			{
				INT32 sequence;
				fixed_t speed;
				INT32 lineindex;
				mobj_t *waypoint = NULL;
				angle_t an;

				if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && player->powers[pw_carry] == CR_ZOOMTUBE)
					break;

				if (player->powers[pw_ignorelatch] & (1<<15))
					break;

				// Find line #3 tagged to this sector
				lineindex = Tag_FindLineSpecial(3, sectag);

				if (lineindex == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Sector special %d missing line special #3.\n", sector->special);
					break;
				}

				// Grab speed and sequence values
				speed = -abs(sides[lines[lineindex].sidenum[0]].textureoffset)/8; // Negative means reverse
				sequence = abs(sides[lines[lineindex].sidenum[0]].rowoffset)>>FRACBITS;

				if (speed == 0)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Waypoint sequence %d at zero speed.\n", sequence);
					break;
				}

				waypoint = P_GetLastWaypoint(sequence);

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
				player->powers[pw_carry] = CR_ZOOMTUBE;
				player->speed = speed;
				player->pflags |= PF_SPINNING;
				player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_BOUNCING|PF_SLIDING|PF_CANCARRY);
				player->climbing = 0;

				if (player->mo->state-states != S_PLAY_ROLL)
				{
					P_SetPlayerMobjState(player->mo, S_PLAY_ROLL);
					S_StartSound(player->mo, sfx_spin);
				}
			}
			break;

		case 10: // Finish Line
			if (((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE) && !player->exiting)
			{
				if (player->starpostnum == numstarposts) // Must have touched all the starposts
				{
					player->laps++;

					if (player->powers[pw_carry] == CR_NIGHTSMODE)
						player->drillmeter += 48*20;

					if (player->laps >= (UINT8)cv_numlaps.value)
						CONS_Printf(M_GetText("%s has finished the race.\n"), player_names[player-players]);
					else if (player->laps == (UINT8)cv_numlaps.value-1)
						CONS_Printf(M_GetText("%s started the \205final lap\200!\n"), player_names[player-players]);
					else
						CONS_Printf(M_GetText("%s started lap %u\n"), player_names[player-players], (UINT32)player->laps+1);

					// Reset starposts (checkpoints) info
					player->starpostscale = player->starpostangle = player->starposttime = player->starpostnum = 0;
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
				mobj_t *waypointmid = NULL;
				mobj_t *waypointhigh = NULL;
				mobj_t *waypointlow = NULL;
				mobj_t *closest = NULL;
				vector3_t p, line[2], resulthigh, resultlow;

				if (player->mo->tracer && player->mo->tracer->type == MT_TUBEWAYPOINT && player->powers[pw_carry] == CR_ROPEHANG)
					break;

				if (player->powers[pw_ignorelatch] & (1<<15))
					break;

				if (player->mo->momz > 0)
					break;

				if (player->cmd.buttons & BT_SPIN)
					break;

				if (!(player->pflags & PF_SLIDING) && player->mo->state == &states[player->mo->info->painstate])
					break;

				if (player->exiting)
					break;

				//initialize resulthigh and resultlow with 0
				memset(&resultlow, 0x00, sizeof(resultlow));
				memset(&resulthigh, 0x00, sizeof(resulthigh));

				// Find line #11 tagged to this sector
				lineindex = Tag_FindLineSpecial(11, sectag);

				if (lineindex == -1)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Sector special %d missing line special #11.\n", sector->special);
					break;
				}

				// Grab speed and sequence values
				speed = abs(sides[lines[lineindex].sidenum[0]].textureoffset)/8;
				sequence = abs(sides[lines[lineindex].sidenum[0]].rowoffset)>>FRACBITS;

				if (speed == 0)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: Waypoint sequence %d at zero speed.\n", sequence);
					break;
				}

				// Find the closest waypoint
				// Find the preceding waypoint
				// Find the proceeding waypoint
				// Determine the closest spot on the line between the three waypoints
				// Put player at that location.

				waypointmid = P_GetClosestWaypoint(sequence, player->mo);

				if (!waypointmid)
				{
					CONS_Debug(DBG_GAMELOGIC, "ERROR: WAYPOINT(S) IN SEQUENCE %d NOT FOUND.\n", sequence);
					break;
				}

				waypointlow = P_GetPreviousWaypoint(waypointmid, true);
				waypointhigh = P_GetNextWaypoint(waypointmid, true);

				CONS_Debug(DBG_GAMELOGIC, "WaypointMid: %d; WaypointLow: %d; WaypointHigh: %d\n",
								waypointmid->health, waypointlow ? waypointlow->health : -1, waypointhigh ? waypointhigh->health : -1);

				// Now we have three waypoints... the closest one we're near, and the one that comes before, and after.
				// Next, we need to find the closest point on the line between each set, and determine which one we're
				// closest to.

				p.x = player->mo->x;
				p.y = player->mo->y;
				p.z = player->mo->z;

				// Waypointmid and Waypointlow:
				if (waypointlow)
				{
					line[0].x = waypointmid->x;
					line[0].y = waypointmid->y;
					line[0].z = waypointmid->z;
					line[1].x = waypointlow->x;
					line[1].y = waypointlow->y;
					line[1].z = waypointlow->z;

					P_ClosestPointOnLine3D(&p, line, &resultlow);
				}

				// Waypointmid and Waypointhigh:
				if (waypointhigh)
				{
					line[0].x = waypointmid->x;
					line[0].y = waypointmid->y;
					line[0].z = waypointmid->z;
					line[1].x = waypointhigh->x;
					line[1].y = waypointhigh->y;
					line[1].z = waypointhigh->z;

					P_ClosestPointOnLine3D(&p, line, &resulthigh);
				}

				// 3D support now available. Disregard the previous notice here. -Red

				P_UnsetThingPosition(player->mo);
				P_ResetPlayer(player);
				player->mo->momx = player->mo->momy = player->mo->momz = 0;

				if (lines[lineindex].flags & ML_EFFECT1) // Don't wrap
				{
					mobj_t *highest = P_GetLastWaypoint(sequence);
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
				else if ((lines[lineindex].flags & ML_EFFECT1) && waypointmid->health == numwaypoints[sequence] - 1)
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
				player->powers[pw_carry] = CR_ROPEHANG;

				// Option for static ropes.
				if (lines[lineindex].flags & ML_NOCLIMB)
					player->speed = 0;
				else
					player->speed = speed;

				S_StartSound(player->mo, sfx_s3k4a);

				player->pflags &= ~(PF_JUMPED|PF_NOJUMPDAMAGE|PF_GLIDING|PF_BOUNCING|PF_SLIDING|PF_CANCARRY);
				player->climbing = 0;
				P_SetThingPosition(player->mo);
				P_SetPlayerMobjState(player->mo, S_PLAY_RIDE);
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
			boolean floorallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(mo->eflags & MFE_VERTICALFLIP)) && (mo->z == topheight));
			boolean ceilingallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (mo->eflags & MFE_VERTICALFLIP)) && (mo->z + mo->height == bottomheight));
			// Thing must be on top of the floor to be affected...
			if (!(floorallowed || ceilingallowed))
				continue;
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
			boolean floorallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_FLOOR) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z == topheight));
			boolean ceilingallowed = ((rover->master->frontsector->flags & SF_FLIPSPECIAL_CEILING) && ((rover->master->frontsector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z + player->mo->height == bottomheight));
			// Thing must be on top of the floor to be affected...
			if (!(floorallowed || ceilingallowed))
				continue;
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
				;
			else if (po->flags & POF_SOLID)
			{
				boolean floorallowed = ((polysec->flags & SF_FLIPSPECIAL_FLOOR) && ((polysec->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z == polysec->ceilingheight));
				boolean ceilingallowed = ((polysec->flags & SF_FLIPSPECIAL_CEILING) && ((polysec->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z + player->mo->height == polysec->floorheight));
				// Thing must be on top of the floor to be affected...
				if (!(floorallowed || ceilingallowed))
				{
					po = (polyobj_t *)(po->link.next);
					continue;
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
			if (!(maptol & TOL_NIGHTS) && G_IsSpecialStage(gamemap))
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

	{
		boolean floorallowed = ((sector->flags & SF_FLIPSPECIAL_FLOOR) && ((sector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || !(player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z == f_affectpoint));
		boolean ceilingallowed = ((sector->flags & SF_FLIPSPECIAL_CEILING) && ((sector->flags & SF_TRIGGERSPECIAL_HEADBUMP) || (player->mo->eflags & MFE_VERTICALFLIP)) && (player->mo->z + player->mo->height == c_affectpoint));
		// Thing must be on top of the floor to be affected...
		if (!(floorallowed || ceilingallowed))
			return;
	}

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
			// update the levelflat texture number
			if ((foundflats->type == LEVELFLAT_TEXTURE) && (foundflats->u.texture.basenum != -1))
				foundflats->u.texture.num = foundflats->u.texture.basenum + ((leveltime/foundflats->speed + foundflats->animseq) % foundflats->numpics);
			// update the levelflat lump number
			else if ((foundflats->type == LEVELFLAT_FLAT) && (foundflats->u.flat.baselumpnum != LUMPERROR))
				foundflats->u.flat.lumpnum = foundflats->u.flat.baselumpnum + ((leveltime/foundflats->speed + foundflats->animseq) % foundflats->numpics);
		}
	}
}

//
// Floor over floors (FOFs), 3Dfloors, 3Dblocks, fake floors (ffloors), rovers, or whatever you want to call them
//

/** Gets the ID number for a 3Dfloor in its target sector.
  *
  * \param fflr The 3Dfloor we want an ID for.
  * \return ID of 3Dfloor in target sector. Note that the first FOF's ID is 0. UINT16_MAX is given if invalid.
  * \sa P_GetFFloorByID
  */
UINT16 P_GetFFloorID(ffloor_t *fflr)
{
	ffloor_t *rover;
	sector_t *sec;
	UINT16 i = 0;

	if (!fflr)
		return UINT16_MAX;

	sec = fflr->target;

	if (!sec->ffloors)
		return UINT16_MAX;
	for (rover = sec->ffloors; rover; rover = rover->next, i++)
		if (rover == fflr)
			return i;
	return UINT16_MAX;
}

/** Gets a 3Dfloor by control sector.
  *
  * \param sec  Target sector.
  * \param sec2 Control sector.
  * \return Pointer to found 3Dfloor, or NULL.
  * \sa P_GetFFloorByID
  */
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

/** Gets a 3Dfloor by ID number.
  *
  * \param sec Target sector.
  * \param id  ID of 3Dfloor in target sector. Note that the first FOF's ID is 0.
  * \return Pointer to found 3Dfloor, or NULL.
  * \sa P_GetFFloorBySec, P_GetFFloorID
  */
ffloor_t *P_GetFFloorByID(sector_t *sec, UINT16 id)
{
	ffloor_t *rover;
	UINT16 i = 0;

	if (!sec->ffloors)
		return NULL;
	for (rover = sec->ffloors; rover; rover = rover->next)
		if (i++ == id)
			return rover;
	return NULL;
}

/** Adds a newly formed 3Dfloor structure to a sector's ffloors list.
  *
  * \param sec    Target sector.
  * \param fflr   Newly formed 3Dfloor structure.
  * \sa P_AddFakeFloor
  */
static inline void P_AddFFloorToList(sector_t *sec, ffloor_t *fflr)
{
	ffloor_t *rover;

	if (!sec->ffloors)
	{
		sec->ffloors = fflr;
		fflr->next = 0;
		fflr->prev = 0;
		return;
	}

	for (rover = sec->ffloors; rover->next; rover = rover->next);

	rover->next = fflr;
	fflr->prev = rover;
	fflr->next = 0;
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
	ffloor_t *fflr;
	thinker_t *th;
	friction_t *f;
	pusher_t *p;
	size_t sec2num;
	size_t i;

	if (sec == sec2)
		return NULL; //Don't need a fake floor on a control sector.
	if ((fflr = (P_GetFFloorBySec(sec, sec2))))
		return fflr; // If this ffloor already exists, return it

	if (sec2->ceilingheight < sec2->floorheight)
	{
		fixed_t tempceiling = sec2->ceilingheight;
		//flip the sector around and print an error instead of crashing 12.1.08 -Inuyasha
		CONS_Alert(CONS_ERROR, M_GetText("FOF (line %s) has a top height below its bottom.\n"), sizeu1(master - lines));
		sec2->ceilingheight = sec2->floorheight;
		sec2->floorheight = tempceiling;
	}

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
	fflr = Z_Calloc(sizeof (*fflr), PU_LEVEL, NULL);
	fflr->secnum = sec2 - sectors;
	fflr->target = sec;
	fflr->bottomheight = &sec2->floorheight;
	fflr->bottompic = &sec2->floorpic;
	fflr->bottomxoffs = &sec2->floor_xoffs;
	fflr->bottomyoffs = &sec2->floor_yoffs;
	fflr->bottomangle = &sec2->floorpic_angle;

	// Add the ceiling
	fflr->topheight = &sec2->ceilingheight;
	fflr->toppic = &sec2->ceilingpic;
	fflr->toplightlevel = &sec2->lightlevel;
	fflr->topxoffs = &sec2->ceiling_xoffs;
	fflr->topyoffs = &sec2->ceiling_yoffs;
	fflr->topangle = &sec2->ceilingpic_angle;

	// Add slopes
	fflr->t_slope = &sec2->c_slope;
	fflr->b_slope = &sec2->f_slope;
	// mark the target sector as having slopes, if the FOF has any of its own
	// (this fixes FOF slopes glitching initially at level load in software mode)
	if (sec2->hasslope)
		sec->hasslope = true;

	if ((flags & FF_SOLID) && (master->flags & ML_EFFECT1)) // Block player only
		flags &= ~FF_BLOCKOTHERS;

	if ((flags & FF_SOLID) && (master->flags & ML_EFFECT2)) // Block all BUT player
		flags &= ~FF_BLOCKPLAYER;

	fflr->spawnflags = fflr->flags = flags;
	fflr->master = master;
	fflr->norender = INFTICS;
	fflr->fadingdata = NULL;

	// Scan the thinkers to check for special conditions applying to this FOF.
	// If we have thinkers sorted by sector, just check the relevant ones;
	// otherwise, check them all. Apologies for the ugly loop...

	sec2num = sec2 - sectors;

	// Just initialise both of these to placate the compiler.
	i = 0;
	th = thlist[THINK_MAIN].next;

	for(;;)
	{
		if(secthinkers)
		{
			if(i < secthinkers[sec2num].count)
				th = secthinkers[sec2num].thinkers[i];
			else break;
		}
		else if (th == &thlist[THINK_MAIN])
			break;

		// Should this FOF have friction?
		if(th->function.acp1 == (actionf_p1)T_Friction)
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
			fflr->alpha = sides[master->sidenum[0]].toptexture; // for future reference, "#0" is 1, and "#255" is 256. Be warned
		else
			fflr->alpha = 0x80;
	}
	else
		fflr->alpha = 0xff;

	fflr->spawnalpha = fflr->alpha; // save for netgames

	if (flags & FF_QUICKSAND)
		CheckForQuicksand = true;

	if ((flags & FF_BUSTUP) || (flags & FF_SHATTER) || (flags & FF_SPINBUST))
		CheckForBustableBlocks = true;

	if ((flags & FF_MARIO))
	{
		if (!(flags & FF_SHATTERBOTTOM)) // Don't change the textures of a brick block, just a question block
			P_AddBlockThinker(sec2, master);
		CheckForMarioBlocks = true;
	}

	if ((flags & FF_CRUMBLE))
		sec2->crumblestate = CRUMBLE_WAIT;

	if ((flags & FF_FLOATBOB))
	{
		P_AddFloatThinker(sec2, Tag_FGet(&master->tags), master);
		CheckForFloatBob = true;
	}

	P_AddFFloorToList(sec, fflr);

	return fflr;
}

//
// SPECIAL SPAWNING
//

/** Adds a float thinker.
  * Float thinkers cause solid 3Dfloors to float on water.
  *
  * \param sec          Control sector.
  * \param actionsector Target sector.
  * \sa P_SpawnSpecials, T_FloatSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddFloatThinker(sector_t *sec, UINT16 tag, line_t *sourceline)
{
	floatthink_t *floater;

	// create and initialize new thinker
	floater = Z_Calloc(sizeof (*floater), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &floater->thinker);

	floater->thinker.function.acp1 = (actionf_p1)T_FloatSector;

	floater->sector = sec;
	floater->tag = (INT16)tag;
	floater->sourceline = sourceline;
}

/**
  * Adds a plane displacement thinker.
  * Whenever the "control" sector moves,
  * the "affectee" sector's floor or ceiling plane moves too!
  *
  * \param speed            Rate of movement relative to control sector
  * \param control          Control sector.
  * \param affectee         Target sector.
  * \param reverse          Reverse direction?
  * \sa P_SpawnSpecials, T_PlaneDisplace
  * \author Monster Iestyn
  */
static void P_AddPlaneDisplaceThinker(INT32 type, fixed_t speed, INT32 control, INT32 affectee, UINT8 reverse)
{
	planedisplace_t *displace;

	// create and initialize new displacement thinker
	displace = Z_Calloc(sizeof (*displace), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &displace->thinker);

	displace->thinker.function.acp1 = (actionf_p1)T_PlaneDisplace;
	displace->affectee = affectee;
	displace->control = control;
	displace->last_height = sectors[control].floorheight;
	displace->speed = speed;
	displace->type = type;
	displace->reverse = reverse;
}

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
	mariocheck_t *block;

	// create and initialize new elevator thinker
	block = Z_Calloc(sizeof (*block), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &block->thinker);

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
  * \param sec          Control sector.
  * \sa P_SpawnSpecials, T_RaiseSector
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddRaiseThinker(sector_t *sec, INT16 tag, fixed_t speed, fixed_t ceilingtop, fixed_t ceilingbottom, boolean lower, boolean spindash)
{
	raise_t *raise;

	raise = Z_Calloc(sizeof (*raise), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &raise->thinker);

	raise->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	raise->tag = tag;
	raise->sector = sec;

	raise->ceilingtop = ceilingtop;
	raise->ceilingbottom = ceilingbottom;

	raise->basespeed = speed;

	if (lower)
		raise->flags |= RF_REVERSE;
	if (spindash)
		raise->flags |= RF_SPINDASH;
}

static void P_AddAirbob(sector_t *sec, INT16 tag, fixed_t dist, boolean raise, boolean spindash, boolean dynamic)
{
	raise_t *airbob;

	airbob = Z_Calloc(sizeof (*airbob), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &airbob->thinker);

	airbob->thinker.function.acp1 = (actionf_p1)T_RaiseSector;

	airbob->tag = tag;
	airbob->sector = sec;

	airbob->ceilingtop = sec->ceilingheight;
	airbob->ceilingbottom = sec->ceilingheight - dist;

	airbob->basespeed = FRACUNIT;

	if (!raise)
		airbob->flags |= RF_REVERSE;
	if (spindash)
		airbob->flags |= RF_SPINDASH;
	if (dynamic)
		airbob->flags |= RF_DYNAMIC;
}

/** Adds a thwomp thinker.
  * Even thwomps need to think!
  *
  * \param sec          Control sector.
  * \sa P_SpawnSpecials, T_ThwompSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddThwompThinker(sector_t *sec, line_t *sourceline, fixed_t crushspeed, fixed_t retractspeed, UINT16 sound)
{
	thwomp_t *thwomp;

	// You *probably* already have a thwomp in this sector. If you've combined it with something
	// else that uses the floordata/ceilingdata, you must be weird.
	if (sec->floordata || sec->ceilingdata)
		return;

	// create and initialize new elevator thinker
	thwomp = Z_Calloc(sizeof (*thwomp), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &thwomp->thinker);

	thwomp->thinker.function.acp1 = (actionf_p1)T_ThwompSector;

	// set up the fields according to the type of elevator action
	thwomp->sourceline = sourceline;
	thwomp->sector = sec;
	thwomp->crushspeed = crushspeed;
	thwomp->retractspeed = retractspeed;
	thwomp->direction = 0;
	thwomp->floorstartheight = sec->floorheight;
	thwomp->ceilingstartheight = sec->ceilingheight;
	thwomp->delay = 1;
	thwomp->tag = Tag_FGet(&sourceline->tags);
	thwomp->sound = sound;

	sec->floordata = thwomp;
	sec->ceilingdata = thwomp;
	// Start with 'resting' texture
	sides[sourceline->sidenum[0]].midtexture = sides[sourceline->sidenum[0]].bottomtexture;
}

/** Adds a thinker which checks if any MF_ENEMY objects with health are in the defined area.
  * If not, a linedef executor is run once.
  *
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_NoEnemiesSector
  * \author SSNTails <http://www.ssntails.org>
  */
static inline void P_AddNoEnemiesThinker(line_t *sourceline)
{
	noenemies_t *nobaddies;

	// create and initialize new thinker
	nobaddies = Z_Calloc(sizeof (*nobaddies), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &nobaddies->thinker);

	nobaddies->thinker.function.acp1 = (actionf_p1)T_NoEnemiesSector;

	nobaddies->sourceline = sourceline;
}

/** Adds a thinker for Each-Time linedef executors. A linedef executor is run
  * only when a player enters the area and doesn't run again until they re-enter.
  *
  * \param sourceline   Control linedef.
  * \sa P_SpawnSpecials, T_EachTimeThinker
  * \author SSNTails <http://www.ssntails.org>
  */
static void P_AddEachTimeThinker(line_t *sourceline)
{
	eachtime_t *eachtime;

	// create and initialize new thinker
	eachtime = Z_Calloc(sizeof (*eachtime), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &eachtime->thinker);

	eachtime->thinker.function.acp1 = (actionf_p1)T_EachTimeThinker;

	eachtime->sourceline = sourceline;
	eachtime->triggerOnExit = !!(sourceline->flags & ML_BOUNCY);
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

	CONS_Alert(CONS_WARNING, M_GetText("Detected a camera scanner effect (linedef type 5). This effect is deprecated and will be removed in the future!\n"));

	// create and initialize new elevator thinker
	elevator = Z_Calloc(sizeof (*elevator), PU_LEVSPEC, NULL);
	P_AddThinker(THINK_MAIN, &elevator->thinker);

	elevator->thinker.function.acp1 = (actionf_p1)T_CameraScanner;
	elevator->type = elevateBounce;

	// set up the fields according to the type of elevator action
	elevator->sector = sourcesec;
	elevator->actionsector = actionsector;
	elevator->distance = FixedInt(AngleFixed(angle));
}

/** Flashes a laser block.
  *
  * \param flash Thinker structure for this laser.
  * \sa P_AddLaserThinker
  * \author SSNTails <http://www.ssntails.org>
  */
void T_LaserFlash(laserthink_t *flash)
{
	msecnode_t *node;
	mobj_t *thing;
	INT32 s;
	ffloor_t *fflr;
	sector_t *sector;
	sector_t *sourcesec = flash->sourceline->frontsector;
	fixed_t top, bottom;
	TAG_ITER_DECLARECOUNTER(0);

	TAG_ITER_SECTORS(0, flash->tag, s)
	{
		sector = &sectors[s];
		for (fflr = sector->ffloors; fflr; fflr = fflr->next)
		{
			if (fflr->master != flash->sourceline)
				continue;

			if (!(fflr->flags & FF_EXISTS))
				break;

			if (leveltime & 2)
				//fflr->flags |= FF_RENDERALL;
				fflr->alpha = 0xB0;
			else
				//fflr->flags &= ~FF_RENDERALL;
				fflr->alpha = 0x90;

			top    = P_GetFFloorTopZAt   (fflr, sector->soundorg.x, sector->soundorg.y);
			bottom = P_GetFFloorBottomZAt(fflr, sector->soundorg.x, sector->soundorg.y);
			sector->soundorg.z = (top + bottom)/2;
			S_StartSound(&sector->soundorg, sfx_laser);

			// Seek out objects to DESTROY! MUAHAHHAHAHAA!!!*cough*
			for (node = sector->touching_thinglist; node && node->m_thing; node = node->m_thinglist_next)
			{
				thing = node->m_thing;

				if (flash->nobosses && thing->flags & MF_BOSS)
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
					P_DamageMobj(thing, NULL, NULL, 1, 0);
				else if (thing->type == MT_EGGSHIELD)
					P_KillMobj(thing, NULL, NULL, 0);
			}

			break;
		}
	}
}

static inline void P_AddLaserThinker(INT16 tag, line_t *line, boolean nobosses)
{
	laserthink_t *flash = Z_Calloc(sizeof (*flash), PU_LEVSPEC, NULL);

	P_AddThinker(THINK_MAIN, &flash->thinker);

	flash->thinker.function.acp1 = (actionf_p1)T_LaserFlash;
	flash->tag = tag;
	flash->sourceline = line;
	flash->nobosses = nobosses;
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

/** Before things are loaded, initialises certain stuff in case they're needed
  * by P_SpawnSlopes or P_LoadThings. This was split off from
  * P_SpawnSpecials, in case you couldn't tell.
  *
  * \sa P_SpawnSpecials
  * \author Monster Iestyn
  */
void P_InitSpecials(void)
{
	// Set the default gravity. Custom gravity overrides this setting.
	gravity = mapheaderinfo[gamemap-1]->gravity;

	// Defaults in case levels don't have them set.
	sstimer = mapheaderinfo[gamemap-1]->sstimer*TICRATE + 6;
	ssspheres = mapheaderinfo[gamemap-1]->ssspheres;

	CheckForBustableBlocks = CheckForBouncySector = CheckForQuicksand = CheckForMarioBlocks = CheckForFloatBob = CheckForReverseGravity = false;

	// Set curWeather
	switch (mapheaderinfo[gamemap-1]->weather)
	{
		case PRECIP_SNOW: // snow
		case PRECIP_RAIN: // rain
		case PRECIP_STORM: // storm
		case PRECIP_STORM_NORAIN: // storm w/o rain
		case PRECIP_STORM_NOSTRIKES: // storm w/o lightning
			curWeather = mapheaderinfo[gamemap-1]->weather;
			break;
		default: // blank/none
			curWeather = PRECIP_NONE;
			break;
	}

	// Set globalweather
	globalweather = mapheaderinfo[gamemap-1]->weather;
}

static void P_ApplyFlatAlignment(line_t *master, sector_t *sector, angle_t flatangle, fixed_t xoffs, fixed_t yoffs)
{
	if (!(master->flags & ML_NETONLY)) // Modify floor flat alignment unless ML_NETONLY flag is set
	{
		sector->floorpic_angle = flatangle;
		sector->floor_xoffs += xoffs;
		sector->floor_yoffs += yoffs;
	}

	if (!(master->flags & ML_NONET)) // Modify ceiling flat alignment unless ML_NONET flag is set
	{
		sector->ceilingpic_angle = flatangle;
		sector->ceiling_xoffs += xoffs;
		sector->ceiling_yoffs += yoffs;
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
void P_SpawnSpecials(boolean fromnetsave)
{
	sector_t *sector;
	size_t i;
	INT32 j;
	thinkerlist_t *secthinkers;
	thinker_t *th;
	// This used to be used, and *should* be used in the future,
	// but currently isn't.
	(void)fromnetsave;

	// yep, we do this here - "bossdisabled" is considered an apparatus of specials.
	bossdisabled = 0;
	stoppedclock = false;

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
				//Terrible hack to replace an even worse hack:
				//Spike damage automatically sets SF_TRIGGERSPECIAL_TOUCH.
				//Yes, this also affects other specials on the same sector. Sorry.
				sector->flags |= SF_TRIGGERSPECIAL_TOUCH;
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
				ssspheres = sector->ceilingheight>>FRACBITS; // Ring count for special stage
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
				if ((gametyperules & (GTR_RACE|GTR_LIVES)) == GTR_RACE)
					circuitmap = true;
				break;
		}
	}

	P_SpawnScrollers(); // Add generalized scrollers
	P_SpawnFriction();  // Friction model using linedefs
	P_SpawnPushers();   // Pusher model using linedefs

	// Look for thinkers that affect FOFs, and sort them by sector

	secthinkers = Z_Calloc(numsectors * sizeof(thinkerlist_t), PU_STATIC, NULL);

	// Firstly, find out how many there are in each sector
	for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
	{
		if (th->function.acp1 == (actionf_p1)T_Friction)
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
	for (th = thlist[THINK_MAIN].next; th != &thlist[THINK_MAIN]; th = th->next)
	{
		size_t secnum = (size_t)-1;

		if (th->function.acp1 == (actionf_p1)T_Friction)
			secnum = ((friction_t *)th)->affectee;
		else if (th->function.acp1 == (actionf_p1)T_Pusher)
			secnum = ((pusher_t *)th)->affectee;

		if (secnum != (size_t)-1)
			secthinkers[secnum].thinkers[secthinkers[secnum].count++] = th;
	}


	// Init line EFFECTs
	for (i = 0; i < numlines; i++)
	{
		mtag_t tag = Tag_FGet(&lines[i].tags);

		if (lines[i].special != 7) // This is a hack. I can at least hope nobody wants to prevent flat alignment in netgames...
		{
			// set line specials to 0 here too, same reason as above
			if (netgame || multiplayer)
			{
				if (lines[i].flags & ML_NONET)
				{
					lines[i].special = 0;
					continue;
				}
			}
			else if (lines[i].flags & ML_NETONLY)
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
			TAG_ITER_DECLARECOUNTER(0);

			case 1: // Definable gravity per sector
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(0, tag, s)
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
				TAG_ITER_SECTORS(0, tag, s)
					P_AddCameraScanner(&sectors[sec], &sectors[s], R_PointToAngle2(lines[i].v2->x, lines[i].v2->y, lines[i].v1->x, lines[i].v1->y));
				break;

			case 7: // Flat alignment - redone by toast
				if ((lines[i].flags & (ML_NETONLY|ML_NONET)) != (ML_NETONLY|ML_NONET)) // If you can do something...
				{
					angle_t flatangle = InvAngle(R_PointToAngle2(lines[i].v1->x, lines[i].v1->y, lines[i].v2->x, lines[i].v2->y));
					fixed_t xoffs;
					fixed_t yoffs;

					if (lines[i].flags & ML_EFFECT6) // Set offset through x and y texture offsets if ML_EFFECT6 flag is set
					{
						xoffs = sides[lines[i].sidenum[0]].textureoffset;
						yoffs = sides[lines[i].sidenum[0]].rowoffset;
					}
					else // Otherwise, set calculated offsets such that line's v1 is the apparent origin
					{
						xoffs = -lines[i].v1->x;
						yoffs = lines[i].v1->y;
					}

					//If no tag is given, apply to front sector
					if (tag == 0)
						P_ApplyFlatAlignment(lines + i, lines[i].frontsector, flatangle, xoffs, yoffs);
					else
					{
						TAG_ITER_SECTORS(0, tag, s)
							P_ApplyFlatAlignment(lines + i, sectors + s, flatangle, xoffs, yoffs);
					}
				}
				else // Otherwise, print a helpful warning. Can I do no less?
					CONS_Alert(CONS_WARNING,
					M_GetText("Flat alignment linedef (tag %d) doesn't have anything to do.\nConsider changing the linedef's flag configuration or removing it entirely.\n"),
					tag);
				break;

			case 8: // Sector Parameters
				TAG_ITER_SECTORS(0, tag, s)
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
					if (lines[i].flags & ML_EFFECT2)
						sectors[s].flags |= SF_TRIGGERSPECIAL_HEADBUMP;

					if (lines[i].flags & ML_EFFECT1)
						sectors[s].flags |= SF_INVERTPRECIP;

					if (lines[i].frontsector && GETSECSPECIAL(lines[i].frontsector->special, 4) == 12)
						sectors[s].camsec = sides[*lines[i].sidenum].sector-sectors;
				}
				break;

			case 9: // Chain Parameters
				break;

			case 10: // Vertical culling plane for sprites and FOFs
				TAG_ITER_SECTORS(0, tag, s)
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
				TAG_ITER_SECTORS(0, tag, s)
					sectors[s].heightsec = (INT32)sec;
				break;

			case 64: // Appearing/Disappearing FOF option
				if (lines[i].flags & ML_BLOCKMONSTERS) { // Find FOFs by control sector tag
					TAG_ITER_SECTORS(0, tag, s)
						for (j = 0; (unsigned)j < sectors[s].linecount; j++)
							if (sectors[s].lines[j]->special >= 100 && sectors[s].lines[j]->special < 300)
								Add_MasterDisappearer(abs(lines[i].dx>>FRACBITS), abs(lines[i].dy>>FRACBITS), abs(sides[lines[i].sidenum[0]].sector->floorheight>>FRACBITS), (INT32)(sectors[s].lines[j]-lines), (INT32)i);
				} else // Find FOFs by effect sector tag
				{
					TAG_ITER_LINES(0, tag, s)
					{
						if ((size_t)s == i)
							continue;
						if (Tag_Find(&sides[lines[s].sidenum[0]].sector->tags, Tag_FGet(&sides[lines[i].sidenum[0]].sector->tags)))
							Add_MasterDisappearer(abs(lines[i].dx>>FRACBITS), abs(lines[i].dy>>FRACBITS), abs(sides[lines[i].sidenum[0]].sector->floorheight>>FRACBITS), s, (INT32)i);
					}
				}
				break;

			case 66: // Displace floor by front sector
				TAG_ITER_SECTORS(0, tag, s)
					P_AddPlaneDisplaceThinker(pd_floor, P_AproxDistance(lines[i].dx, lines[i].dy)>>8, sides[lines[i].sidenum[0]].sector-sectors, s, !!(lines[i].flags & ML_NOCLIMB));
				break;
			case 67: // Displace ceiling by front sector
				TAG_ITER_SECTORS(0, tag, s)
					P_AddPlaneDisplaceThinker(pd_ceiling, P_AproxDistance(lines[i].dx, lines[i].dy)>>8, sides[lines[i].sidenum[0]].sector-sectors, s, !!(lines[i].flags & ML_NOCLIMB));
				break;
			case 68: // Displace both floor AND ceiling by front sector
				TAG_ITER_SECTORS(0, tag, s)
					P_AddPlaneDisplaceThinker(pd_both, P_AproxDistance(lines[i].dx, lines[i].dy)>>8, sides[lines[i].sidenum[0]].sector-sectors, s, !!(lines[i].flags & ML_NOCLIMB));
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
					ffloorflags |= FF_CUTLEVEL|FF_BOTHPLANES|FF_ALLSIDES;
					ffloorflags &= ~(FF_EXTRA|FF_CUTEXTRA);
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
					ffloorflags |= FF_CUTLEVEL|FF_BOTHPLANES|FF_ALLSIDES;
					ffloorflags &= ~(FF_EXTRA|FF_CUTEXTRA);
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
					ffloorflags |= FF_CUTLEVEL|FF_BOTHPLANES|FF_ALLSIDES;
					ffloorflags &= ~(FF_EXTRA|FF_CUTEXTRA);
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
					ffloorflags |= FF_CUTLEVEL|FF_BOTHPLANES|FF_ALLSIDES;
					ffloorflags &= ~(FF_EXTRA|FF_CUTEXTRA);
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
					ffloorflags |= FF_CUTLEVEL|FF_BOTHPLANES|FF_ALLSIDES;
					ffloorflags &= ~(FF_EXTRA|FF_CUTEXTRA);
				}

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 146: // Intangible floor/ceiling with solid sides (fences/hoops maybe?)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERSIDES|FF_ALLSIDES|FF_INTANGIBLEFLATS, secthinkers);
				break;

			case 150: // Air bobbing platform
			case 151: // Adjustable air bobbing platform
			{
				fixed_t dist = (lines[i].special == 150) ? 16*FRACUNIT : P_AproxDistance(lines[i].dx, lines[i].dy);
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				P_AddAirbob(lines[i].frontsector, tag, dist, false, !!(lines[i].flags & ML_NOCLIMB), false);
				break;
			}
			case 152: // Adjustable air bobbing platform in reverse
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				P_AddAirbob(lines[i].frontsector, tag, P_AproxDistance(lines[i].dx, lines[i].dy), true, !!(lines[i].flags & ML_NOCLIMB), false);
				break;
			case 153: // Dynamic Sinking Platform
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				P_AddAirbob(lines[i].frontsector, tag, P_AproxDistance(lines[i].dx, lines[i].dy), false, !!(lines[i].flags & ML_NOCLIMB), true);
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
				P_AddAirbob(lines[i].frontsector, tag, 16*FRACUNIT, false, !!(lines[i].flags & ML_NOCLIMB), false);
				break;

			case 177: // Air bobbing platform that will crumble and bob on
				// the water when it falls and hits, then never return
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_FLOATBOB|FF_CRUMBLE|FF_NORETURN, secthinkers);
				P_AddAirbob(lines[i].frontsector, tag, 16*FRACUNIT, false, !!(lines[i].flags & ML_NOCLIMB), false);
				break;

			case 178: // Crumbling platform that will float when it hits water
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CRUMBLE|FF_FLOATBOB, secthinkers);
				break;

			case 179: // Crumbling platform that will float when it hits water, but not return
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE|FF_FLOATBOB|FF_NORETURN, secthinkers);
				break;

			case 180: // Air bobbing platform that will crumble
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_CRUMBLE, secthinkers);
				P_AddAirbob(lines[i].frontsector, tag, 16*FRACUNIT, false, !!(lines[i].flags & ML_NOCLIMB), false);
				break;

			case 190: // Rising Platform FOF (solid, opaque, shadows)
			case 191: // Rising Platform FOF (solid, opaque, no shadows)
			case 192: // Rising Platform TL block: FOF (solid, translucent)
			case 193: // Rising Platform FOF (solid, invisible)
			case 194: // Rising Platform 'Platform' - You can jump up through it
			case 195: // Rising Platform Translucent "platform"
			{
				fixed_t speed = FixedDiv(P_AproxDistance(lines[i].dx, lines[i].dy), 4*FRACUNIT);
				fixed_t ceilingtop = P_FindHighestCeilingSurrounding(lines[i].frontsector);
				fixed_t ceilingbottom = P_FindLowestCeilingSurrounding(lines[i].frontsector);

				ffloorflags = FF_EXISTS|FF_SOLID;
				if (lines[i].special != 193)
					ffloorflags |= FF_RENDERALL;
				if (lines[i].special <= 191)
					ffloorflags |= FF_CUTLEVEL;
				if (lines[i].special == 192 || lines[i].special == 195)
					ffloorflags |= FF_TRANSLUCENT|FF_EXTRA|FF_CUTEXTRA;
				if (lines[i].special >= 194)
					ffloorflags |= FF_PLATFORM|FF_BOTHPLANES|FF_ALLSIDES;
				if (lines[i].special != 190 && (lines[i].special <= 193 || lines[i].flags & ML_NOCLIMB))
					ffloorflags |= FF_NOSHADE;
				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);

				P_AddRaiseThinker(lines[i].frontsector, tag, speed, ceilingtop, ceilingbottom, !!(lines[i].flags & ML_BLOCKMONSTERS), !!(lines[i].flags & ML_NOCLIMB));
				break;
			}

			case 200: // Double light effect
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_CUTSPRITES|FF_DOUBLESHADOW, secthinkers);
				break;

			case 201: // Light effect
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_CUTSPRITES, secthinkers);
				break;

			case 202: // Fog
				ffloorflags = FF_EXISTS|FF_RENDERALL|FF_FOG|FF_INVERTPLANES|FF_INVERTSIDES|FF_CUTEXTRA|FF_EXTRA|FF_DOUBLESHADOW|FF_CUTSPRITES;
				sec = sides[*lines[i].sidenum].sector - sectors;
				// SoM: Because it's fog, check for an extra colormap and set the fog flag...
				if (sectors[sec].extra_colormap)
					sectors[sec].extra_colormap->flags = CMF_FOG;
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
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL|FF_MARIO;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_SHATTERBOTTOM;
				if (lines[i].flags & ML_EFFECT1)
					ffloorflags &= ~(FF_SOLID|FF_RENDERALL|FF_CUTLEVEL);

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 251: // A THWOMP!
			{
				fixed_t crushspeed = (lines[i].flags & ML_EFFECT5) ? lines[i].dy >> 3 : 10*FRACUNIT;
				fixed_t retractspeed = (lines[i].flags & ML_EFFECT5) ? lines[i].dx >> 3 : 2*FRACUNIT;
				UINT16 sound = (lines[i].flags & ML_EFFECT4) ? sides[lines[i].sidenum[0]].textureoffset >> FRACBITS : sfx_thwomp;
				P_AddThwompThinker(lines[i].frontsector, &lines[i], crushspeed, retractspeed, sound);
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_CUTLEVEL, secthinkers);
				break;
			}

			case 252: // Shatter block (breaks when touched)
				ffloorflags = FF_EXISTS|FF_BLOCKOTHERS|FF_RENDERALL|FF_BUSTUP|FF_SHATTER;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_BLOCKPLAYER|FF_SHATTERBOTTOM;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 253: // Translucent shatter block (see 76)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_BLOCKOTHERS|FF_RENDERALL|FF_BUSTUP|FF_SHATTER|FF_TRANSLUCENT, secthinkers);
				break;

			case 254: // Bustable block
				ffloorflags = FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP;
				if (lines[i].flags & ML_NOCLIMB)
					ffloorflags |= FF_STRONGBUST;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 255: // Spin bust block (breaks when jumped or spun downwards onto)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SPINBUST, secthinkers);
				break;

			case 256: // Translucent spin bust block (see 78)
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_SOLID|FF_RENDERALL|FF_BUSTUP|FF_SPINBUST|FF_TRANSLUCENT, secthinkers);
				break;

			case 257: // Quicksand
				ffloorflags = FF_EXISTS|FF_QUICKSAND|FF_RENDERALL|FF_ALLSIDES|FF_CUTSPRITES;
				if (lines[i].flags & ML_EFFECT5)
					ffloorflags |= FF_RIPPLE;

				P_AddFakeFloorsByLine(i, ffloorflags, secthinkers);
				break;

			case 258: // Laser block
				P_AddLaserThinker(tag, lines + i, !!(lines[i].flags & ML_EFFECT1));
				P_AddFakeFloorsByLine(i, FF_EXISTS|FF_RENDERALL|FF_NOSHADE|FF_EXTRA|FF_CUTEXTRA|FF_TRANSLUCENT, secthinkers);
				break;

			case 259: // Custom FOF
				if (lines[i].sidenum[1] != 0xffff)
				{
					ffloortype_e fofflags = sides[lines[i].sidenum[1]].toptexture;
					P_AddFakeFloorsByLine(i, fofflags, secthinkers);
				}
				else
					I_Error("Custom FOF (tag %d) found without a linedef back side!", tag);
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
				if (!(gametyperules & GTR_RACE))
					lines[i].special = 0;
				break;

			// Linedef executor triggers for CTF teams.
			case 309:
			case 311:
				if (!(gametyperules & GTR_TEAMFLAGS))
					lines[i].special = 0;
				break;

			// Each time executors
			case 306:
			case 301:
			case 310:
			case 312:
			case 332:
			case 335:
				P_AddEachTimeThinker(&lines[i]);
				break;

			// No More Enemies Linedef Exec
			case 313:
				P_AddNoEnemiesThinker(&lines[i]);
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
					P_AddEachTimeThinker(&lines[i]);
				break;

			// NiGHTS trigger executors
			case 323:
			case 324:
			case 325:
			case 326:
			case 327:
			case 328:
			case 329:
			case 330:
				break;

			// Skin trigger executors
			case 331:
			case 333:
				break;

			// Object dye executors
			case 334:
			case 336:
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

			case 449: // Enable bosses with parameter
			{
				INT32 bossid = sides[*lines[i].sidenum].textureoffset>>FRACBITS;
				if (bossid & ~15) // if any bits other than first 16 are set
				{
					CONS_Alert(CONS_WARNING,
						M_GetText("Boss enable linedef (tag %d) has an invalid texture x offset.\nConsider changing it or removing it entirely.\n"),
						tag);
					break;
				}
				if (!(lines[i].flags & ML_NOCLIMB))
				{
					bossdisabled |= (1<<bossid); // gotta disable in the first place to enable
					CONS_Debug(DBG_GAMELOGIC, "Line type 449 spawn effect: bossid disabled = %d", bossid);
				}
				break;
			}

			// 500 is used for a scroller
			// 501 is used for a scroller
			// 502 is used for a scroller
			// 503 is used for a scroller
			// 504 is used for a scroller
			// 505 is used for a scroller
			// 506 is used for a scroller
			// 507 is used for a scroller
			// 508 is used for a scroller
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
				TAG_ITER_SECTORS(0, tag, s)
					sectors[s].floorlightsec = (INT32)sec;
				break;

			case 601: // ceiling lighting independently
				sec = sides[*lines[i].sidenum].sector-sectors;
				TAG_ITER_SECTORS(0, tag, s)
					sectors[s].ceilinglightsec = (INT32)sec;
				break;

			case 602: // Adjustable pulsating light
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(0, tag, s)
					P_SpawnAdjustableGlowingLight(&sectors[sec], &sectors[s],
						P_AproxDistance(lines[i].dx, lines[i].dy)>>FRACBITS);
				break;

			case 603: // Adjustable flickering light
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(0, tag, s)
					P_SpawnAdjustableFireFlicker(&sectors[sec], &sectors[s],
						P_AproxDistance(lines[i].dx, lines[i].dy)>>FRACBITS);
				break;

			case 604: // Adjustable Blinking Light (unsynchronized)
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(0, tag, s)
					P_SpawnAdjustableStrobeFlash(&sectors[sec], &sectors[s],
						abs(lines[i].dx)>>FRACBITS, abs(lines[i].dy)>>FRACBITS, false);
				break;

			case 605: // Adjustable Blinking Light (synchronized)
				sec = sides[*lines[i].sidenum].sector - sectors;
				TAG_ITER_SECTORS(0, tag, s)
					P_SpawnAdjustableStrobeFlash(&sectors[sec], &sectors[s],
						abs(lines[i].dx)>>FRACBITS, abs(lines[i].dy)>>FRACBITS, true);
				break;

			case 606: // HACK! Copy colormaps. Just plain colormaps.
				TAG_ITER_SECTORS(0, lines[i].args[0], s)
				{
					extracolormap_t *exc;

					if (sectors[s].colormap_protected)
						continue;

					if (!udmf)
						exc = sides[lines[i].sidenum[0]].colormap_data;
					else
					{
						if (!lines[i].args[1])
							exc = lines[i].frontsector->extra_colormap;
						else
						{
							INT32 sourcesec = Tag_Iterate_Sectors(lines[i].args[1], 0);
							if (sourcesec == -1)
							{
								CONS_Debug(DBG_GAMELOGIC, "Line type 606: Can't find sector with source colormap (tag %d)!\n", lines[i].args[1]);
								return;
							}
							exc = sectors[sourcesec].extra_colormap;
						}
					}
					sectors[s].extra_colormap = sectors[s].spawn_extra_colormap = exc;
				}
				break;

			default:
				break;
		}
	}

	// Allocate each list
	for (i = 0; i < numsectors; i++)
		if(secthinkers[i].thinkers)
			Z_Free(secthinkers[i].thinkers);

	Z_Free(secthinkers);

	// haleyjd 02/20/06: spawn polyobjects
	Polyobj_InitLevel();

	for (i = 0; i < numlines; i++)
	{
		switch (lines[i].special)
		{
			case 30: // Polyobj_Flag
				PolyFlag(&lines[i]);
				break;

			case 31: // Polyobj_Displace
				PolyDisplace(&lines[i]);
				break;

			case 32: // Polyobj_RotDisplace
				PolyRotDisplace(&lines[i]);
				break;
		}
	}

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
	TAG_ITER_DECLARECOUNTER(0);
	INT32 s;
	mtag_t tag = Tag_FGet(&lines[line].tags);
	size_t sec = sides[*lines[line].sidenum].sector-sectors;

	line_t* li = lines + line;
	TAG_ITER_SECTORS(0, tag, s)
		P_AddFakeFloor(&sectors[s], &sectors[sec], li, ffloorflags, secthinkers);
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
		thing->eflags |= MFE_PUSHED;
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
		TAG_ITER_DECLARECOUNTER(0);

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

				TAG_ITER_SECTORS(0, Tag_FGet(&line->tags), sect)
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

						if (thing->eflags & MFE_PUSHED) // Already pushed this tic by an exclusive pusher.
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

					if (thing->eflags & MFE_PUSHED)
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
				TAG_ITER_SECTORS(0, Tag_FGet(&line->tags), sect)
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

						if (thing->eflags & MFE_PUSHED)
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

					if (thing->eflags & MFE_PUSHED)
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
	P_AddThinker(THINK_MAIN, &s->thinker);
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
	mtag_t tag;

	for (i = 0; i < numlines; i++, l++)
	{
		fixed_t dx = l->dx >> SCROLL_SHIFT; // direction and speed of scrolling
		fixed_t dy = l->dy >> SCROLL_SHIFT;
		INT32 control = -1, accel = 0; // no control sector or acceleration
		INT32 special = l->special;

		tag = Tag_FGet(&l->tags);

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
			TAG_ITER_DECLARECOUNTER(0);

			case 513: // scroll effect ceiling
			case 533: // scroll and carry objects on ceiling
				TAG_ITER_SECTORS(0, tag, s)
					Add_Scroller(sc_ceiling, -dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				if (special != 533)
					break;
				/* FALLTHRU */

			case 523:	// carry objects on ceiling
				dx = FixedMul(dx, CARRYFACTOR);
				dy = FixedMul(dy, CARRYFACTOR);
				TAG_ITER_SECTORS(0, tag, s)
					Add_Scroller(sc_carry_ceiling, dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				break;

			case 510: // scroll effect floor
			case 530: // scroll and carry objects on floor
				TAG_ITER_SECTORS(0, tag, s)
					Add_Scroller(sc_floor, -dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				if (special != 530)
					break;
				/* FALLTHRU */

			case 520:	// carry objects on floor
				dx = FixedMul(dx, CARRYFACTOR);
				dy = FixedMul(dy, CARRYFACTOR);
				TAG_ITER_SECTORS(0, tag, s)
					Add_Scroller(sc_carry, dx, dy, control, s, accel, l->flags & ML_NOCLIMB);
				break;

			// scroll wall according to linedef
			// (same direction and speed as scrolling floors)
			case 502:
			{
				TAG_ITER_LINES(0, tag, s)
					if (s != (INT32)i)
					{
						if (l->flags & ML_EFFECT2) // use texture offsets instead
						{
							dx = sides[l->sidenum[0]].textureoffset;
							dy = sides[l->sidenum[0]].rowoffset;
						}
						if (l->flags & ML_EFFECT3)
						{
							if (lines[s].sidenum[1] != 0xffff)
								Add_Scroller(sc_side, dx, dy, control, lines[s].sidenum[1], accel, 0);
						}
						else
							Add_Scroller(sc_side, dx, dy, control, lines[s].sidenum[0], accel, 0);
					}
				break;
			}

			case 505:
				s = lines[i].sidenum[0];
				Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, s, accel, 0);
				break;

			case 506:
				s = lines[i].sidenum[1];

				if (s != 0xffff)
					Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, lines[i].sidenum[0], accel, 0);
				else
					CONS_Debug(DBG_GAMELOGIC, "Line special 506 (line #%s) missing back side!\n", sizeu1(i));
				break;

			case 507:
				s = lines[i].sidenum[0];

				if (lines[i].sidenum[1] != 0xffff)
					Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, lines[i].sidenum[1], accel, 0);
				else
					CONS_Debug(DBG_GAMELOGIC, "Line special 507 (line #%s) missing back side!\n", sizeu1(i));
				break;

			case 508:
				s = lines[i].sidenum[1];

				if (s != 0xffff)
					Add_Scroller(sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1, s, accel, 0);
				else
					CONS_Debug(DBG_GAMELOGIC, "Line special 508 (line #%s) missing back side!\n", sizeu1(i));
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

	P_AddThinker(THINK_MAIN, &d->thinker);
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
		mtag_t afftag = Tag_FGet(&lines[d->affectee].tags);
		TAG_ITER_DECLARECOUNTER(0);

		TAG_ITER_SECTORS(0, afftag, s)
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
						sectors[s].soundorg.z = P_GetFFloorTopZAt(rover, sectors[s].soundorg.x, sectors[s].soundorg.y);
						S_StartSound(&sectors[s].soundorg, sfx_appear);
					}
				}
			}
			sectors[s].moved = true;
			P_RecalcPrecipInSector(&sectors[s]);
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

/** Removes fadingdata from FOF control sector
 *
 * \param line	line to search for target faders
 * \param data	pointer to set new fadingdata to. Can be NULL to erase.
 */
static void P_ResetFakeFloorFader(ffloor_t *rover, fade_t *data, boolean finalize)
{
	fade_t *fadingdata = (fade_t *)rover->fadingdata;
	// find any existing thinkers and remove them, then replace with new data
	if (fadingdata != data)
	{
		if (fadingdata)
		{
			if (finalize)
				P_FadeFakeFloor(rover,
					fadingdata->sourcevalue,
					fadingdata->alpha >= fadingdata->destvalue ?
						fadingdata->alpha - 1 : // trigger fade-out finish
						fadingdata->alpha + 1, // trigger fade-in finish
					0,
					fadingdata->ticbased,
					&fadingdata->timer,
					fadingdata->doexists,
					fadingdata->dotranslucent,
					fadingdata->dolighting,
					fadingdata->docolormap,
					fadingdata->docollision,
					fadingdata->doghostfade,
					fadingdata->exactalpha);
			rover->alpha = fadingdata->alpha;

			if (fadingdata->dolighting)
				P_RemoveLighting(&sectors[rover->secnum]);

			if (fadingdata->docolormap)
				P_ResetColormapFader(&sectors[rover->secnum]);

			P_RemoveThinker(&fadingdata->thinker);
		}

		rover->fadingdata = data;
	}
}

static boolean P_FadeFakeFloor(ffloor_t *rover, INT16 sourcevalue, INT16 destvalue, INT16 speed, boolean ticbased, INT32 *timer,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha)
{
	boolean stillfading = false;
	INT32 alpha;
	fade_t *fadingdata = (fade_t *)rover->fadingdata;
	(void)docolormap; // *shrug* maybe we can use this in the future. For now, let's be consistent with our other function params

	if (rover->master->special == 258) // Laser block
		return false;

	// If fading an invisible FOF whose render flags we did not yet set,
	// initialize its alpha to 1
	if (dotranslucent &&
		(rover->spawnflags & FF_NOSHADE) && // do not include light blocks, which don't set FF_NOSHADE
		!(rover->flags & FF_FOG) && // do not include fog
		!(rover->spawnflags & FF_RENDERSIDES) &&
		!(rover->spawnflags & FF_RENDERPLANES) &&
		!(rover->flags & FF_RENDERALL))
		rover->alpha = 1;

	if (fadingdata)
		alpha = fadingdata->alpha;
	else
		alpha = rover->alpha;

	// routines specific to fade in and fade out
	if (!ticbased && alpha == destvalue)
		return stillfading;
	else if (alpha > destvalue) // fade out
	{
		// finish fading out
		if (speed < 1 || (!ticbased && alpha - speed <= destvalue + speed) ||
			(ticbased && (--(*timer) <= 0 || alpha <= destvalue)))
		{
			alpha = destvalue;

			if (docollision)
			{
				if (rover->spawnflags & FF_SOLID)
					rover->flags &= ~FF_SOLID;
				if (rover->spawnflags & FF_SWIMMABLE)
					rover->flags &= ~FF_SWIMMABLE;
				if (rover->spawnflags & FF_QUICKSAND)
					rover->flags &= ~FF_QUICKSAND;
				if (rover->spawnflags & FF_BUSTUP)
					rover->flags &= ~FF_BUSTUP;
				if (rover->spawnflags & FF_MARIO)
					rover->flags &= ~FF_MARIO;
			}
		}
		else // continue fading out
		{
			if (!ticbased)
				alpha -= speed;
			else
			{
				INT16 delta = abs(destvalue - sourcevalue);
				fixed_t factor = min(FixedDiv(speed - (*timer), speed), 1*FRACUNIT);
				alpha = max(min(alpha, sourcevalue - (INT16)FixedMul(delta, factor)), destvalue);
			}
			stillfading = true;
		}
	}
	else // fade in
	{
		// finish fading in
		if (speed < 1 || (!ticbased && alpha + speed >= destvalue - speed) ||
			(ticbased && (--(*timer) <= 0 || alpha >= destvalue)))
		{
			alpha = destvalue;

			if (docollision)
			{
				if (rover->spawnflags & FF_SOLID)
					rover->flags |= FF_SOLID;
				if (rover->spawnflags & FF_SWIMMABLE)
					rover->flags |= FF_SWIMMABLE;
				if (rover->spawnflags & FF_QUICKSAND)
					rover->flags |= FF_QUICKSAND;
				if (rover->spawnflags & FF_BUSTUP)
					rover->flags |= FF_BUSTUP;
				if (rover->spawnflags & FF_MARIO)
					rover->flags |= FF_MARIO;
			}
		}
		else // continue fading in
		{
			if (!ticbased)
				alpha += speed;
			else
			{
				INT16 delta = abs(destvalue - sourcevalue);
				fixed_t factor = min(FixedDiv(speed - (*timer), speed), 1*FRACUNIT);
				alpha = min(max(alpha, sourcevalue + (INT16)FixedMul(delta, factor)), destvalue);
			}
			stillfading = true;
		}
	}

	// routines common to both fade in and fade out
	if (!stillfading)
	{
		if (doexists && !(rover->spawnflags & FF_BUSTUP))
		{
			if (alpha <= 1)
				rover->flags &= ~FF_EXISTS;
			else
				rover->flags |= FF_EXISTS;

			// Re-render lighting at end of fade
			if (dolighting && !(rover->spawnflags & FF_NOSHADE) && !(rover->flags & FF_EXISTS))
				rover->target->moved = true;
		}

		if (dotranslucent && !(rover->flags & FF_FOG))
		{
			if (alpha >= 256)
			{
				if (!(rover->flags & FF_CUTSOLIDS) &&
					(rover->spawnflags & FF_CUTSOLIDS))
				{
					rover->flags |= FF_CUTSOLIDS;
					rover->target->moved = true;
				}

				rover->flags &= ~FF_TRANSLUCENT;
			}
			else
			{
				rover->flags |= FF_TRANSLUCENT;

				if ((rover->flags & FF_CUTSOLIDS) &&
					(rover->spawnflags & FF_CUTSOLIDS))
				{
					rover->flags &= ~FF_CUTSOLIDS;
					rover->target->moved = true;
				}
			}

			if ((rover->spawnflags & FF_NOSHADE) && // do not include light blocks, which don't set FF_NOSHADE
				!(rover->spawnflags & FF_RENDERSIDES) &&
				!(rover->spawnflags & FF_RENDERPLANES))
			{
				if (rover->alpha > 1)
					rover->flags |= FF_RENDERALL;
				else
					rover->flags &= ~FF_RENDERALL;
			}
		}
	}
	else
	{
		if (doexists && !(rover->spawnflags & FF_BUSTUP))
		{
			// Re-render lighting if we haven't yet set FF_EXISTS (beginning of fade)
			if (dolighting && !(rover->spawnflags & FF_NOSHADE) && !(rover->flags & FF_EXISTS))
				rover->target->moved = true;

			rover->flags |= FF_EXISTS;
		}

		if (dotranslucent && !(rover->flags & FF_FOG))
		{
			rover->flags |= FF_TRANSLUCENT;

			if ((rover->flags & FF_CUTSOLIDS) &&
				(rover->spawnflags & FF_CUTSOLIDS))
			{
				rover->flags &= ~FF_CUTSOLIDS;
				rover->target->moved = true;
			}

			if ((rover->spawnflags & FF_NOSHADE) && // do not include light blocks, which don't set FF_NOSHADE
				!(rover->spawnflags & FF_RENDERSIDES) &&
				!(rover->spawnflags & FF_RENDERPLANES))
				rover->flags |= FF_RENDERALL;
		}

		if (docollision)
		{
			if (doghostfade) // remove collision flags during fade
			{
				if (rover->spawnflags & FF_SOLID)
					rover->flags &= ~FF_SOLID;
				if (rover->spawnflags & FF_SWIMMABLE)
					rover->flags &= ~FF_SWIMMABLE;
				if (rover->spawnflags & FF_QUICKSAND)
					rover->flags &= ~FF_QUICKSAND;
				if (rover->spawnflags & FF_BUSTUP)
					rover->flags &= ~FF_BUSTUP;
				if (rover->spawnflags & FF_MARIO)
					rover->flags &= ~FF_MARIO;
			}
			else // keep collision during fade
			{
				if (rover->spawnflags & FF_SOLID)
					rover->flags |= FF_SOLID;
				if (rover->spawnflags & FF_SWIMMABLE)
					rover->flags |= FF_SWIMMABLE;
				if (rover->spawnflags & FF_QUICKSAND)
					rover->flags |= FF_QUICKSAND;
				if (rover->spawnflags & FF_BUSTUP)
					rover->flags |= FF_BUSTUP;
				if (rover->spawnflags & FF_MARIO)
					rover->flags |= FF_MARIO;
			}
		}
	}

	if (!(rover->flags & FF_FOG)) // don't set FOG alpha
	{
		if (!stillfading || exactalpha)
			rover->alpha = alpha;
		else // clamp fadingdata->alpha to software's alpha levels
		{
			if (alpha < 12)
				rover->alpha = destvalue < 12 ? destvalue : 1; // Don't even draw it
			else if (alpha < 38)
				rover->alpha = destvalue >= 12 && destvalue < 38 ? destvalue : 25;
			else if (alpha < 64)
				rover->alpha = destvalue >=38 && destvalue < 64 ? destvalue : 51;
			else if (alpha < 89)
				rover->alpha = destvalue >= 64 && destvalue < 89 ? destvalue : 76;
			else if (alpha < 115)
				rover->alpha = destvalue >= 89 && destvalue < 115 ? destvalue : 102;
			else if (alpha < 140)
				rover->alpha = destvalue >= 115 && destvalue < 140 ? destvalue : 128;
			else if (alpha < 166)
				rover->alpha = destvalue >= 140 && destvalue < 166 ? destvalue : 154;
			else if (alpha < 192)
				rover->alpha = destvalue >= 166 && destvalue < 192 ? destvalue : 179;
			else if (alpha < 217)
				rover->alpha = destvalue >= 192 && destvalue < 217 ? destvalue : 204;
			else if (alpha < 243)
				rover->alpha = destvalue >= 217 && destvalue < 243 ? destvalue : 230;
			else // Opaque
				rover->alpha = destvalue >= 243 ? destvalue : 256;
		}
	}

	if (fadingdata)
		fadingdata->alpha = alpha;

	return stillfading;
}

/** Adds fake floor fader thinker.
  *
  * \param destvalue    transparency value to fade to
  * \param speed        speed to fade by
  * \param ticbased     tic-based logic, speed = duration
  * \param relative     Destvalue is relative to rover->alpha
  * \param doexists	    handle FF_EXISTS
  * \param dotranslucent handle FF_TRANSLUCENT
  * \param dolighting  fade FOF light
  * \param docollision handle interactive flags
  * \param doghostfade  no interactive flags during fading
  * \param exactalpha   use exact alpha values (opengl)
  */
static void P_AddFakeFloorFader(ffloor_t *rover, size_t sectornum, size_t ffloornum,
	INT16 destvalue, INT16 speed, boolean ticbased, boolean relative,
	boolean doexists, boolean dotranslucent, boolean dolighting, boolean docolormap,
	boolean docollision, boolean doghostfade, boolean exactalpha)
{
	fade_t *d;

	// If fading an invisible FOF whose render flags we did not yet set,
	// initialize its alpha to 1
	if (dotranslucent &&
		(rover->spawnflags & FF_NOSHADE) && // do not include light blocks, which don't set FF_NOSHADE
		!(rover->spawnflags & FF_RENDERSIDES) &&
		!(rover->spawnflags & FF_RENDERPLANES) &&
		!(rover->flags & FF_RENDERALL))
		rover->alpha = 1;

	// already equal, nothing to do
	if (rover->alpha == max(1, min(256, relative ? rover->alpha + destvalue : destvalue)))
		return;

	d = Z_Malloc(sizeof *d, PU_LEVSPEC, NULL);

	d->thinker.function.acp1 = (actionf_p1)T_Fade;
	d->rover = rover;
	d->sectornum = (UINT32)sectornum;
	d->ffloornum = (UINT32)ffloornum;

	d->alpha = d->sourcevalue = rover->alpha;
	d->destvalue = max(1, min(256, relative ? rover->alpha + destvalue : destvalue)); // rover->alpha is 1-256

	if (ticbased)
	{
		d->ticbased = true;
		d->timer = d->speed = abs(speed); // use d->speed as total duration
	}
	else
	{
		d->ticbased = false;
		d->speed = max(1, speed); // minimum speed 1/tic // if speed < 1, alpha is set immediately in thinker
		d->timer = -1;
	}

	d->doexists = doexists;
	d->dotranslucent = dotranslucent;
	d->dolighting = dolighting;
	d->docolormap = docolormap;
	d->docollision = docollision;
	d->doghostfade = doghostfade;
	d->exactalpha = exactalpha;

	// find any existing thinkers and remove them, then replace with new data
	P_ResetFakeFloorFader(rover, d, false);

	// Set a separate thinker for shadow fading
	if (dolighting && !(rover->flags & FF_NOSHADE))
	{
		UINT16 lightdelta = abs(sectors[rover->secnum].spawn_lightlevel - rover->target->lightlevel);
		fixed_t alphapercent = min(FixedDiv(d->destvalue, rover->spawnalpha), 1*FRACUNIT); // don't make darker than spawn_lightlevel
		fixed_t adjustedlightdelta = FixedMul(lightdelta, alphapercent);

		if (rover->target->lightlevel >= sectors[rover->secnum].spawn_lightlevel) // fading out, get lighter
			d->destlightlevel = rover->target->lightlevel - adjustedlightdelta;
		else // fading in, get darker
			d->destlightlevel = rover->target->lightlevel + adjustedlightdelta;

		P_FadeLightBySector(&sectors[rover->secnum],
			d->destlightlevel,
			ticbased ? d->timer :
				FixedFloor(FixedDiv(abs(d->destvalue - d->alpha), d->speed))/FRACUNIT,
			true);
	}
	else
		d->destlightlevel = -1;

	// Set a separate thinker for colormap fading
	if (docolormap && !(rover->flags & FF_NOSHADE) && sectors[rover->secnum].spawn_extra_colormap && !sectors[rover->secnum].colormap_protected)
	{
		extracolormap_t *dest_exc,
			*source_exc = sectors[rover->secnum].extra_colormap ? sectors[rover->secnum].extra_colormap : R_GetDefaultColormap();

		INT32 colordelta = R_GetRgbaA(sectors[rover->secnum].spawn_extra_colormap->rgba); // alpha is from 0
		fixed_t alphapercent = min(FixedDiv(d->destvalue, rover->spawnalpha), 1*FRACUNIT); // don't make darker than spawn_lightlevel
		fixed_t adjustedcolordelta = FixedMul(colordelta, alphapercent);
		INT32 coloralpha;

		coloralpha = adjustedcolordelta;

		dest_exc = R_CopyColormap(sectors[rover->secnum].spawn_extra_colormap, false);
		dest_exc->rgba = R_GetRgbaRGB(dest_exc->rgba) + R_PutRgbaA(coloralpha);

		if (!(d->dest_exc = R_GetColormapFromList(dest_exc)))
		{
			dest_exc->colormap = R_CreateLightTable(dest_exc);
			R_AddColormapToList(dest_exc);
			d->dest_exc = dest_exc;
		}
		else
			Z_Free(dest_exc);

		// If fading from 0, set source_exc rgb same to dest_exc
		if (!R_CheckDefaultColormap(d->dest_exc, true, false, false)
			&& R_CheckDefaultColormap(source_exc, true, false, false))
		{
			extracolormap_t *exc = R_CopyColormap(source_exc, false);
			exc->rgba = R_GetRgbaRGB(d->dest_exc->rgba) + R_PutRgbaA(R_GetRgbaA(source_exc->rgba));
			exc->fadergba = R_GetRgbaRGB(d->dest_exc->rgba) + R_PutRgbaA(R_GetRgbaA(source_exc->fadergba));

			if (!(source_exc = R_GetColormapFromList(exc)))
			{
				exc->colormap = R_CreateLightTable(exc);
				R_AddColormapToList(exc);
				source_exc = exc;
			}
			else
				Z_Free(exc);
		}

		Add_ColormapFader(&sectors[rover->secnum], source_exc, d->dest_exc, true,
			ticbased ? d->timer :
				FixedFloor(FixedDiv(abs(d->destvalue - d->alpha), d->speed))/FRACUNIT);
	}

	P_AddThinker(THINK_MAIN, &d->thinker);
}

/** Makes a FOF fade
  *
  * \param d Fade thinker.
  * \sa P_AddFakeFloorFader
  */
void T_Fade(fade_t *d)
{
	if (d->rover && !P_FadeFakeFloor(d->rover, d->sourcevalue, d->destvalue, d->speed, d->ticbased, &d->timer,
		d->doexists, d->dotranslucent, d->dolighting, d->docolormap, d->docollision, d->doghostfade, d->exactalpha))
	{
		// Finalize lighting, copypasta from P_AddFakeFloorFader
		if (d->dolighting && !(d->rover->flags & FF_NOSHADE) && d->destlightlevel > -1)
			sectors[d->rover->secnum].lightlevel = d->destlightlevel;

		// Finalize colormap
		if (d->docolormap && !(d->rover->flags & FF_NOSHADE) && sectors[d->rover->secnum].spawn_extra_colormap)
			sectors[d->rover->secnum].extra_colormap = d->dest_exc;

		P_RemoveFakeFloorFader(d->rover);
	}
}

static void P_ResetColormapFader(sector_t *sector)
{
	if (sector->fadecolormapdata)
	{
		// The thinker is the first member in all the action structs,
		// so just let the thinker get freed, and that will free the whole
		// structure.
		P_RemoveThinker(&((elevator_t *)sector->fadecolormapdata)->thinker);
		sector->fadecolormapdata = NULL;
	}
}

static void Add_ColormapFader(sector_t *sector, extracolormap_t *source_exc, extracolormap_t *dest_exc,
	boolean ticbased, INT32 duration)
{
	fadecolormap_t *d;

	P_ResetColormapFader(sector);

	// nothing to do, set immediately
	if (!duration || R_CheckEqualColormaps(source_exc, dest_exc, true, true, true))
	{
		sector->extra_colormap = dest_exc;
		return;
	}

	d = Z_Malloc(sizeof *d, PU_LEVSPEC, NULL);
	d->thinker.function.acp1 = (actionf_p1)T_FadeColormap;
	d->sector = sector;
	d->source_exc = source_exc;
	d->dest_exc = dest_exc;

	if (ticbased)
	{
		d->ticbased = true;
		d->duration = d->timer = duration;
	}
	else
	{
		d->ticbased = false;
		d->timer = 256;
		d->duration = duration; // use as speed
	}

	sector->fadecolormapdata = d;
	P_AddThinker(THINK_MAIN, &d->thinker);
}

void T_FadeColormap(fadecolormap_t *d)
{
	if ((d->ticbased && --d->timer <= 0)
		|| (!d->ticbased && (d->timer -= d->duration) <= 0)) // d->duration used as speed decrement
	{
		d->sector->extra_colormap = d->dest_exc;
		P_ResetColormapFader(d->sector);
	}
	else
	{
		extracolormap_t *exc;
		INT32 duration = d->ticbased ? d->duration : 256;
		fixed_t factor = min(FixedDiv(duration - d->timer, duration), 1*FRACUNIT);
		INT16 cr, cg, cb, ca, fadestart, fadeend, flags;
		INT32 rgba, fadergba;

		// NULL failsafes (or intentionally set to signify default)
		if (!d->sector->extra_colormap)
			d->sector->extra_colormap = R_GetDefaultColormap();

		if (!d->source_exc)
			d->source_exc = R_GetDefaultColormap();

		if (!d->dest_exc)
			d->dest_exc = R_GetDefaultColormap();

		// For each var (rgba + fadergba + params = 11 vars), we apply
		// percentage fading: currentval = sourceval + (delta * percent of duration elapsed)
		// delta is negative when fading out (destval is lower)
		// max/min are used to ensure progressive calcs don't go backwards and to cap values to dest.

#define APPLYFADE(dest, src, cur) (\
(dest-src < 0) ? \
	max(\
		min(cur,\
			src + (INT16)FixedMul(dest-src, factor)),\
		dest)\
: (dest-src > 0) ? \
	min(\
		max(cur,\
			src + (INT16)FixedMul(dest-src, factor)),\
		dest)\
: \
	dest\
)

		cr = APPLYFADE(R_GetRgbaR(d->dest_exc->rgba), R_GetRgbaR(d->source_exc->rgba), R_GetRgbaR(d->sector->extra_colormap->rgba));
		cg = APPLYFADE(R_GetRgbaG(d->dest_exc->rgba), R_GetRgbaG(d->source_exc->rgba), R_GetRgbaG(d->sector->extra_colormap->rgba));
		cb = APPLYFADE(R_GetRgbaB(d->dest_exc->rgba), R_GetRgbaB(d->source_exc->rgba), R_GetRgbaB(d->sector->extra_colormap->rgba));
		ca = APPLYFADE(R_GetRgbaA(d->dest_exc->rgba), R_GetRgbaA(d->source_exc->rgba), R_GetRgbaA(d->sector->extra_colormap->rgba));

		rgba = R_PutRgbaRGBA(cr, cg, cb, ca);

		cr = APPLYFADE(R_GetRgbaR(d->dest_exc->fadergba), R_GetRgbaR(d->source_exc->fadergba), R_GetRgbaR(d->sector->extra_colormap->fadergba));
		cg = APPLYFADE(R_GetRgbaG(d->dest_exc->fadergba), R_GetRgbaG(d->source_exc->fadergba), R_GetRgbaG(d->sector->extra_colormap->fadergba));
		cb = APPLYFADE(R_GetRgbaB(d->dest_exc->fadergba), R_GetRgbaB(d->source_exc->fadergba), R_GetRgbaB(d->sector->extra_colormap->fadergba));
		ca = APPLYFADE(R_GetRgbaA(d->dest_exc->fadergba), R_GetRgbaA(d->source_exc->fadergba), R_GetRgbaA(d->sector->extra_colormap->fadergba));

		fadergba = R_PutRgbaRGBA(cr, cg, cb, ca);

		fadestart = APPLYFADE(d->dest_exc->fadestart, d->source_exc->fadestart, d->sector->extra_colormap->fadestart);
		fadeend = APPLYFADE(d->dest_exc->fadeend, d->source_exc->fadeend, d->sector->extra_colormap->fadeend);
		flags = abs(factor) > FRACUNIT/2 ? d->dest_exc->flags : d->source_exc->flags; // set new flags halfway through fade

#undef APPLYFADE

		//////////////////
		// setup new colormap
		//////////////////

		if (!(d->sector->extra_colormap = R_GetColormapFromListByValues(rgba, fadergba, fadestart, fadeend, flags)))
		{
			exc = R_CreateDefaultColormap(false);
			exc->fadestart = fadestart;
			exc->fadeend = fadeend;
			exc->flags = flags;
			exc->rgba = rgba;
			exc->fadergba = fadergba;
			exc->colormap = R_CreateLightTable(exc);
			R_AddColormapToList(exc);
			d->sector->extra_colormap = exc;
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

	P_AddThinker(THINK_MAIN, &f->thinker);
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

	// Get FOF control sector
	if (f->roverfriction)
		referrer = sectors + f->referrer;

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
					if (thing->player)
						thing->movefactor = f->movefactor;
				}
			}
			else if (P_GetSpecialBottomZ(thing, sec, sec) == thing->floorz && (thing->friction == ORIG_FRICTION // normal friction?
				|| f->friction < thing->friction))
			{
				thing->friction = f->friction;
				if (thing->player)
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
	mtag_t tag;
	register INT32 s;
	fixed_t strength; // frontside texture offset controls magnitude
	fixed_t friction; // friction value to be applied during movement
	INT32 movefactor; // applied to each player move to simulate inertia
	TAG_ITER_DECLARECOUNTER(0);

	for (i = 0; i < numlines; i++, l++)
		if (l->special == 540)
		{
			tag = Tag_FGet(&l->tags);
			strength = sides[l->sidenum[0]].textureoffset>>FRACBITS;
			if (strength > 0) // sludge
				strength = strength*2; // otherwise, the maximum sludginess value is +967...

			// The following might seem odd. At the time of movement,
			// the move distance is multiplied by 'friction/0x10000', so a
			// higher friction value actually means 'less friction'.
			friction = ORIG_FRICTION - (0x1EB8*strength)/0x80; // ORIG_FRICTION is 0xE800

			if (friction > FRACUNIT)
				friction = FRACUNIT;
			if (friction < 0)
				friction = 0;

			movefactor = FixedDiv(ORIG_FRICTION, friction);
			if (movefactor < FRACUNIT)
				movefactor = 8*movefactor - 7*FRACUNIT;
			else
				movefactor = FRACUNIT;

			TAG_ITER_SECTORS(0, tag, s)
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
	P_AddThinker(THINK_MAIN, &p->thinker);
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
	if (thing->eflags & MFE_PUSHED)
		return false;

	if (thing->player && thing->player->powers[pw_carry] == CR_ROPEHANG)
		return false;

	// Allow this to affect pushable objects at some point?
	if (thing->player && (!(thing->flags & (MF_NOGRAVITY | MF_NOCLIP)) || thing->player->powers[pw_carry] == CR_NIGHTSMODE))
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
			if (thing->player->powers[pw_carry] != CR_NIGHTSMODE)
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
		thing->eflags |= MFE_PUSHED;

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

		if (GETSECSPECIAL(referrer->special, 3) != 2)
			return;
	}
	else if (GETSECSPECIAL(sec->special, 3) != 2)
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

		if (!((thing->flags & MF_PUSHABLE) || ((thing->info->flags & MF_PUSHABLE) && thing->fuse))
			&& !(thing->type == MT_PLAYER
			|| thing->type == MT_SMALLBUBBLE
			|| thing->type == MT_MEDIUMBUBBLE
			|| thing->type == MT_EXTRALARGEBUBBLE
			|| thing->type == MT_LITTLETUMBLEWEED
			|| thing->type == MT_BIGTUMBLEWEED))
			continue;

		if (thing->eflags & MFE_PUSHED)
			continue;

		if (thing->player && thing->player->powers[pw_carry] == CR_ROPEHANG)
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
				pflags_t jumped = (thing->player->pflags & (PF_JUMPED|PF_NOJUMPDAMAGE));
				P_ResetPlayer (thing->player);

				if (jumped)
					thing->player->pflags |= jumped;

				thing->player->pflags |= PF_SLIDING;
				thing->angle = R_PointToAngle2 (0, 0, xspeed<<(FRACBITS-PUSH_FACTOR), yspeed<<(FRACBITS-PUSH_FACTOR));

				if (!demoplayback || P_ControlStyle(thing->player) == CS_LMAOGALOG)
				{
					angle_t angle = thing->player->angleturn << 16;
					if (thing->angle - angle > ANGLE_180)
						P_SetPlayerAngle(thing->player, angle - (angle - thing->angle) / 8);
					else
						P_SetPlayerAngle(thing->player, angle + (thing->angle - angle) / 8);
					//P_SetPlayerAngle(thing->player, thing->angle);
				}
			}

			if (p->exclusive)
				thing->eflags |= MFE_PUSHED;
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
	mtag_t tag;
	register INT32 s;
	mobj_t *thing;
	TAG_ITER_DECLARECOUNTER(0);

	for (i = 0; i < numlines; i++, l++)
	{
		tag = Tag_FGet(&l->tags);
		switch (l->special)
		{
			case 541: // wind
				TAG_ITER_SECTORS(0, tag, s)
					Add_Pusher(p_wind, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 544: // current
				TAG_ITER_SECTORS(0, tag, s)
					Add_Pusher(p_current, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 547: // push/pull
				TAG_ITER_SECTORS(0, tag, s)
				{
					thing = P_GetPushThing(s);
					if (thing) // No MT_P* means no effect
						Add_Pusher(p_push, l->dx, l->dy, thing, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				}
				break;
			case 545: // current up
				TAG_ITER_SECTORS(0, tag, s)
					Add_Pusher(p_upcurrent, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 546: // current down
				TAG_ITER_SECTORS(0, tag, s)
					Add_Pusher(p_downcurrent, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 542: // wind up
				TAG_ITER_SECTORS(0, tag, s)
					Add_Pusher(p_upwind, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
			case 543: // wind down
				TAG_ITER_SECTORS(0, tag, s)
					Add_Pusher(p_downwind, l->dx, l->dy, NULL, s, -1, l->flags & ML_NOCLIMB, l->flags & ML_EFFECT4);
				break;
		}
	}
}
