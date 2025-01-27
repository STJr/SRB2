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
/// \file  r_things.c
/// \brief Refresh of things, i.e. objects represented by sprites

#include "doomdef.h"
#include "console.h"
#include "g_game.h"
#include "r_local.h"
#include "st_stuff.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_menu.h" // character select
#include "m_misc.h"
#include "info.h" // spr2names
#include "i_video.h" // rendermode
#include "i_system.h"
#include "r_fps.h"
#include "r_things.h"
#include "r_translation.h"
#include "r_patch.h"
#include "r_patchrotation.h"
#include "r_picformats.h"
#include "r_plane.h"
#include "r_portal.h"
#include "r_splats.h"
#include "p_tick.h"
#include "p_local.h"
#include "p_slopes.h"
#include "netcode/d_netfil.h" // blargh. for nameonly().
#include "m_cheat.h" // objectplace
#ifdef HWRENDER
#include "hardware/hw_md2.h"
#include "hardware/hw_glob.h"
#include "hardware/hw_light.h"
#include "hardware/hw_drv.h"
#endif

#define MINZ (FRACUNIT*4)
#define BASEYCENTER (BASEVIDHEIGHT/2)

typedef struct
{
	INT32 x1, x2;
	INT32 column;
	INT32 topclip, bottomclip;
} maskdraw_t;

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
static lighttable_t **spritelights;

// constant arrays used for psprite clipping and initializing clipping
INT16 negonearray[MAXVIDWIDTH];
INT16 screenheightarray[MAXVIDWIDTH];

spriteinfo_t spriteinfo[NUMSPRITES];

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up and range check thing_t sprites patches
spritedef_t *sprites;
size_t numsprites;

static spriteframe_t sprtemp[MAXFRAMENUM];
static size_t maxframe;
static const char *spritename;

//
// Clipping against drawsegs optimization, from prboom-plus
//
// TODO: This should be done with proper subsector pass through
// sprites which would ideally remove the need to do it at all.
// Unfortunately, SRB2's drawing loop has lots of annoying
// changes from Doom for portals, which make it hard to implement.

typedef struct drawseg_xrange_item_s
{
	INT16 x1, x2;
	drawseg_t *user;
} drawseg_xrange_item_t;

typedef struct drawsegs_xrange_s
{
	drawseg_xrange_item_t *items;
	INT32 count;
} drawsegs_xrange_t;

#define DS_RANGES_COUNT 3
static drawsegs_xrange_t drawsegs_xranges[DS_RANGES_COUNT];

static drawseg_xrange_item_t *drawsegs_xrange;
static size_t drawsegs_xrange_size = 0;
static INT32 drawsegs_xrange_count = 0;

// ==========================================================================
//
// Sprite loading routines: support sprites in pwad, dehacked sprite renaming,
// replacing not all frames of an existing sprite, add sprites at run-time,
// add wads at run-time.
//
// ==========================================================================

spritenum_t R_GetSpriteNumByName(const char *name)
{
	for (spritenum_t i = 0; i < NUMSPRITES; i++)
		if (!strcmp(name, sprnames[i]))
			return i;
	return NUMSPRITES;
}

//
//
//
static void R_InstallSpriteLump(UINT16 wad,            // graphics patch
                                UINT16 lump,
                                size_t lumpid,      // identifier
                                UINT8 frame,
                                UINT8 rotation,
                                UINT8 flipped)
{
	char cn = R_Frame2Char(frame), cr = R_Rotation2Char(rotation); // for debugging

	char framedescription[256];
	if (cn != '\xFF')
		sprintf(framedescription, "%s frame %d (%c)", spritename, frame, cn);
	else
		sprintf(framedescription, "%s frame %d", spritename, frame);

	INT32 r;
	lumpnum_t lumppat = (wad << 16) + lump;

	if (maxframe ==(size_t)-1 || frame > maxframe)
		maxframe = frame;

#ifdef ROTSPRITE
	for (r = 0; r < 16; r++)
	{
		sprtemp[frame].rotated[r] = NULL;
	}
#endif

	if (rotation == 0)
	{
		// the lump should be used for all rotations
		if (sprtemp[frame].rotate == SRF_SINGLE)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has multiple rot = 0 lump\n", framedescription);
		else if (sprtemp[frame].rotate != SRF_NONE) // Let's bundle 1-8/16 and L/R rotations into one debug message.
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has rotations and a rot = 0 lump\n", framedescription);

		sprtemp[frame].rotate = SRF_SINGLE;
		for (r = 0; r < 16; r++)
		{
			sprtemp[frame].lumppat[r] = lumppat;
			sprtemp[frame].lumpid[r] = lumpid;
		}
		sprtemp[frame].flip = flipped ? 0xFFFF : 0; // 1111111111111111 in binary
		return;
	}

	if (rotation == ROT_L || rotation == ROT_R)
	{
		UINT8 rightfactor = ((rotation == ROT_R) ? 4 : 0);

		// the lump should be used for half of all rotations
		if (sprtemp[frame].rotate == SRF_NONE)
			sprtemp[frame].rotate = SRF_SINGLE;
		else if (sprtemp[frame].rotate == SRF_SINGLE)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has L/R rotations and a rot = 0 lump\n", framedescription);
		else if (sprtemp[frame].rotate == SRF_3D)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has both L/R and 1-8 rotations\n", framedescription);
		else if (sprtemp[frame].rotate == SRF_3DGE)
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has both L/R and 1-G rotations\n", framedescription);
		else if ((sprtemp[frame].rotate & SRF_LEFT) && (rotation == ROT_L))
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has multiple L rotations\n", framedescription);
		else if ((sprtemp[frame].rotate & SRF_RIGHT) && (rotation == ROT_R))
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has multiple R rotations\n", framedescription);

		sprtemp[frame].rotate |= ((rotation == ROT_R) ? SRF_RIGHT : SRF_LEFT);
		if ((sprtemp[frame].rotate & SRF_2D) == SRF_2D)
			sprtemp[frame].rotate &= ~SRF_3DMASK; // SRF_3D|SRF_2D being enabled at the same time doesn't HURT in the current sprite angle implementation, but it DOES mean more to check in some of the helper functions. Let's not allow this scenario to happen.

		// load into every relevant angle, including the front one
		for (r = 0; r < 4; r++)
		{
			sprtemp[frame].lumppat[r + rightfactor] = lumppat;
			sprtemp[frame].lumpid[r + rightfactor] = lumpid;
			sprtemp[frame].lumppat[r + rightfactor + 8] = lumppat;
			sprtemp[frame].lumpid[r + rightfactor + 8] = lumpid;

		}

		if (flipped)
			sprtemp[frame].flip |= (0x0F0F<<rightfactor); // 0000111100001111 or 1111000011110000 in binary, depending on rotation being ROT_L or ROT_R
		else
			sprtemp[frame].flip &= ~(0x0F0F<<rightfactor); // ditto

		return;
	}

	if (sprtemp[frame].rotate == SRF_NONE)
		sprtemp[frame].rotate = SRF_SINGLE;
	else if (sprtemp[frame].rotate == SRF_SINGLE)
		CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has 1-8/G rotations and a rot = 0 lump\n", framedescription);
	else if (sprtemp[frame].rotate & SRF_2D)
		CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s has both L/R and 1-8/G rotations\n", framedescription);

	// make 0 based
	rotation--;

	{
		// SRF_3D|SRF_3DGE being enabled at the same time doesn't HURT in the current sprite angle implementation, but it DOES mean more to check in some of the helper functions. Let's not allow this scenario to happen.
		UINT8 threedrot = (rotation > 7) ? SRF_3DGE : (sprtemp[frame].rotate & SRF_3DMASK);
		if (!threedrot)
			threedrot = SRF_3D;

		if (rotation == 0 || rotation == 4) // Front or back...
			sprtemp[frame].rotate = threedrot; // Prevent L and R changeover
		else if ((rotation & 7) > 3) // Right side
			sprtemp[frame].rotate = (threedrot | (sprtemp[frame].rotate & SRF_LEFT)); // Continue allowing L frame changeover
		else // if ((rotation & 7) <= 3) // Left side
			sprtemp[frame].rotate = (threedrot | (sprtemp[frame].rotate & SRF_RIGHT)); // Continue allowing R frame changeover
	}

	if (sprtemp[frame].lumppat[rotation] != LUMPERROR)
	{
		if (cn != '\xFF')
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s: %d_%c (%c%c) has two lumps mapped to it\n", spritename, frame, cr, cn, cr);
		else
			CONS_Debug(DBG_SETUP, "R_InitSprites: Sprite %s: %d_%c has two lumps mapped to it\n", spritename, frame, cr);
	}

	// lumppat & lumpid are the same for original Doom, but different
	// when using sprites in pwad : the lumppat points the new graphics
	sprtemp[frame].lumppat[rotation] = lumppat;
	sprtemp[frame].lumpid[rotation] = lumpid;
	if (flipped)
		sprtemp[frame].flip |= (1<<rotation);
	else
		sprtemp[frame].flip &= ~(1<<rotation);
}

static boolean GetFramesAndRotationsFromShortLumpName(
	const char *name,
	INT32 *ret_frame,
	UINT8 *ret_rotation,
	INT32 *ret_frame2,
	UINT8 *ret_rotation2
)
{
	size_t namelen = strlen(name);

	if (namelen != 6 && namelen != 8)
		return false;

	*ret_frame = R_Char2Frame(name[4]);
	*ret_rotation = R_Char2Rotation(name[5]);
	if (*ret_frame >= 64 || *ret_rotation == 255)
		return false;

	if (namelen == 8)
	{
		*ret_frame2 = R_Char2Frame(name[6]);
		*ret_rotation2 = R_Char2Rotation(name[7]);
		if (*ret_frame2 >= 64 || *ret_rotation2 == 255)
			return false;

		// TRNSLATE is a valid but extremely unlikely sprite name:
		// * The sprite name is "TRNS"
		// * The frame is L, rotation A; mirrored to frame T, rotation E
		// In the very unfortunate event that TRNSLATE is found between sprite lumps,
		// this name check prevents it from being added as a sprite, when it actually isn't.
		if (memcmp(name, "TRNSLATE", 8) == 0)
			return false;
	}
	else
	{
		*ret_frame2 = -1;
		*ret_rotation2 = 255;
	}

	return true;
}

static boolean GetSingleFrameAndRotation(
	const char *name,
	size_t len,
	INT32 *ret_frame,
	UINT8 *ret_rotation
)
{
	const char *underscore = strchr(name, '_');

	// Found but past the part of the name we are parsing
	if ((size_t)(underscore - name) >= len)
		underscore = NULL;

	size_t framelen = underscore ? (size_t)(underscore - name) : len;
	if (framelen < 1 || framelen > 4)
		return false;

	char framepart[4 + 1]; // Max 9999
	strlcpy(framepart, name, framelen + 1);

	for (size_t i = 0; i < framelen; i++)
		if (!isdigit(framepart[i]))
			return false;

	*ret_frame = atoi(framepart);
	*ret_rotation = underscore ? R_Char2Rotation(*(underscore + 1)) : 0;
	if (*ret_frame >= MAXFRAMENUM || *ret_rotation == 255)
		return false;

	return true;
}

static boolean GetFramesAndRotationsFromLongLumpName(
	const char *name,
	INT32 *ret_frame,
	UINT8 *ret_rotation,
	INT32 *ret_frame2,
	UINT8 *ret_rotation2
)
{
	const char *plus = strchr(name, '+');

	if (plus)
	{
		size_t len1 = plus - name;

		if (!GetSingleFrameAndRotation(name, len1, ret_frame, ret_rotation))
			return false;
		if (!GetSingleFrameAndRotation(plus + 1, strlen(name) - len1 - 1, ret_frame2, ret_rotation2))
			return false;
	}
	else
	{
		if (!GetSingleFrameAndRotation(name, strlen(name), ret_frame, ret_rotation))
			return false;

		*ret_frame2 = -1;
		*ret_rotation2 = 255;
	}

	return true;
}

static UINT8 GetOppositeRotation(UINT8 rotation, UINT8 flags)
{
	if (flags & ~SRF_3DMASK)
		I_Error("GetOppositeRotation: rotation type not supported");

	UINT8 numrotations = (flags == SRF_3D) ? 8 : 16;
	return (rotation == 1) ? 1 : numrotations + 2 - rotation;
}

static void MirrorMissingRotations(void)
{
	for (UINT32 framenum = 0; framenum < maxframe; framenum++)
	{
		spriteframe_t *frame = &sprtemp[framenum];

		if (frame->rotate == SRF_NONE || !(frame->rotate & (SRF_3DMASK | SRF_2D)))
			continue;

		UINT8 numrotations = frame->rotate == SRF_3D ? 8 : 16;

		for (UINT8 rotation = 1; rotation <= numrotations; rotation++)
		{
			if (frame->lumppat[rotation - 1] != LUMPERROR)
				continue;

			UINT8 baserotation = GetOppositeRotation(rotation, frame->rotate);
			UINT32 lumpnum = frame->lumppat[baserotation - 1];
			R_InstallSpriteLump(WADFILENUM(lumpnum), LUMPNUM(lumpnum), frame->lumpid[baserotation - 1], framenum, rotation, 1);
		}
	}
}

// Some checks to help development
static void CheckFrame(const char *sprname)
{
	for (UINT32 frame = 0; frame < maxframe; frame++)
	{
		spriteframe_t *spriteframe = &sprtemp[frame];

		char framedescription[256];
		if (frame < 64)
			sprintf(framedescription, "%s frame %d (%c)", sprname, frame, R_Frame2Char(frame));
		else
			sprintf(framedescription, "%s frame %d", sprname, frame);

		switch (spriteframe->rotate)
		{
		case SRF_NONE:
			// no rotations were found for that frame at all
			I_Error("R_AddSingleSpriteDef: No patches found for %s", framedescription);
			break;

		case SRF_SINGLE:
			// only the first rotation is needed
			break;

		case SRF_2D: // both Left and Right rotations
			// we test to see whether the left and right slots are present
			if ((spriteframe->lumppat[2] == LUMPERROR) || (spriteframe->lumppat[6] == LUMPERROR))
				I_Error("R_AddSingleSpriteDef: Sprite %s is missing rotations (L-R mode)",
				framedescription);
			break;

		default:
			{
				// must have all 8/16 frames
				UINT8 rotation = ((spriteframe->rotate & SRF_3DGE) ? 16 : 8);
				while (rotation--)
				{
					// we test the patch lump, or the id lump whatever
					// if it was not loaded the two are LUMPERROR
					if (spriteframe->lumppat[rotation] == LUMPERROR)
						I_Error("R_AddSingleSpriteDef: Sprite %s is missing rotations (1-%c mode)",
								framedescription, ((spriteframe->rotate & SRF_3DGE) ? 'G' : '8'));
				}
			}
			break;
		}
	}
}

// Install a single sprite, given its identifying name (4 chars)
//
// (originally part of R_AddSpriteDefs)
//
// Pass: name of sprite
//       spritedef_t
//       wadnum         : wad number, indexes wadfiles[], where patches
//                        for frames are found
//       startlump      : first lump to search for sprite frames
//       endlump        : AFTER the last lump to search
//       longname       : whether to use long sprite names or 4-char names
//
// Returns true if the sprite was succesfully added
//
boolean R_AddSingleSpriteDef(const char *sprname, spritedef_t *spritedef, UINT16 wadnum, UINT16 startlump, UINT16 endlump, boolean longname)
{
	UINT16 l;
	lumpinfo_t *lumpinfo;
	UINT16 numadded = 0;

	memset(sprtemp,0xFF, sizeof (sprtemp));
	maxframe = (size_t)-1;

	spritename = sprname;

	// are we 'patching' a sprite already loaded ?
	// if so, it might patch only certain frames, not all
	if (spritedef->numframes) // (then spriteframes is not null)
	{
		// copy the already defined sprite frames
		M_Memcpy(sprtemp, spritedef->spriteframes,
		 spritedef->numframes * sizeof (spriteframe_t));
		maxframe = spritedef->numframes - 1;
	}

	// scan the lumps,
	//  filling in the frames for whatever is found
	lumpinfo = wadfiles[wadnum]->lumpinfo;
	if (endlump > wadfiles[wadnum]->numlumps)
		endlump = wadfiles[wadnum]->numlumps;

	for (l = startlump; l < endlump; l++)
	{
		if (longname && W_IsLumpFolder(wadnum, l))
			I_Error("R_AddSingleSpriteDef: all frame lumps for a sprite should be contained inside a single folder\n");

		// For long sprites, the startlump-endlump range only includes
		// relevant lumps, so no check needed in that case
		if (longname || (strlen(sprname) == 4 && !memcmp(lumpinfo[l].name, sprname, 4)))
		{
			INT16 width, height;
			INT16 topoffset, leftoffset;
			INT32 frame, frame2;
			UINT8 rotation, rotation2;

			boolean good = longname ?
				GetFramesAndRotationsFromLongLumpName(lumpinfo[l].longname, &frame, &rotation, &frame2, &rotation2) :
				GetFramesAndRotationsFromShortLumpName(lumpinfo[l].name, &frame, &rotation, &frame2, &rotation2);

			if (!good) // Give an actual NAME error -_-...
			{
				CONS_Alert(CONS_WARNING, M_GetText("Bad sprite name: %s\n"), W_CheckNameForNumPwad(wadnum,l));
				continue;
			}

			// skip NULL sprites from very old dmadds pwads
			if (W_LumpLengthPwad(wadnum,l)<=8)
				continue;

			// Get the patch's dimensions only
			if (!W_ReadPatchHeaderPwad(wadnum, l, &width, &height, &topoffset, &leftoffset))
				continue;

			// store sprite info in lookup tables
			//FIXME : numspritelumps do not duplicate sprite replacements
			spritecachedinfo[numspritelumps].width = width<<FRACBITS;
			spritecachedinfo[numspritelumps].offset = leftoffset<<FRACBITS;
			spritecachedinfo[numspritelumps].topoffset = topoffset<<FRACBITS;
			spritecachedinfo[numspritelumps].height = height<<FRACBITS;

			// BP: we cannot use special tric in hardware mode because feet in ground caused by z-buffer
			// Monster Iestyn (21 Sep 2023): the above comment no longer makes sense in context!!! So I give an explanation here!
			// FEETADJUST was originally an OpenGL-exclusive hack from Doom Legacy to avoid the player's feet being clipped as
			// a result of rendering partially under the ground, but sometime before SRB2 2.1's release this was changed to apply
			// to the software renderer as well.
			// TODO: kill FEETADJUST altogether somehow and somehow fix OpenGL not to clip sprites that are partially underground (if possible)?
			spritecachedinfo[numspritelumps].topoffset += FEETADJUST;

			//----------------------------------------------------

			R_InstallSpriteLump(wadnum, l, numspritelumps, frame, rotation, 0);
			if (frame2 != -1)
				R_InstallSpriteLump(wadnum, l, numspritelumps, frame2, rotation2, 1);

			if (++numspritelumps >= max_spritelumps)
			{
				max_spritelumps *= 2;
				Z_Realloc(spritecachedinfo, max_spritelumps*sizeof(*spritecachedinfo), PU_STATIC, &spritecachedinfo);
			}

			++numadded;
		}
	}

	//
	// if no frames found for this sprite
	//
	if (maxframe == (size_t)-1)
	{
		// the first time (which is for the original wad),
		// all sprites should have their initial frames
		// and then, patch wads can replace it
		// we will skip non-replaced sprite frames, only if
		// they have already have been initially defined (original wad)

		//check only after all initial pwads added
		//if (spritedef->numframes == 0)
		//    I_Error("R_AddSpriteDefs: no initial frames found for sprite %s\n",
		//             namelist[i]);

		// sprite already has frames, and is not replaced by this wad
		return false;
	}
	else if (!numadded)
	{
		// Nothing related to this spritedef has been changed
		// so there is no point going back through these checks again.
		return false;
	}

	maxframe++;

	if (longname)
		MirrorMissingRotations();

	CheckFrame(sprname);

	// allocate space for the frames present and copy sprtemp to it
	if (spritedef->numframes &&             // has been allocated
		spritedef->numframes < maxframe)   // more frames are defined ?
	{
		Z_Free(spritedef->spriteframes);
		spritedef->spriteframes = NULL;
	}

	// allocate this sprite's frames
	if (!spritedef->spriteframes)
		spritedef->spriteframes =
		 Z_Malloc(maxframe * sizeof (*spritedef->spriteframes), PU_STATIC, NULL);

	spritedef->numframes = maxframe;
	M_Memcpy(spritedef->spriteframes, sprtemp, maxframe*sizeof (spriteframe_t));

	return true;
}

static void AddShortSpriteDefs(UINT16 wadnum, size_t *ptr_spritesadded, size_t *ptr_framesadded)
{
	size_t i;
	UINT16 start, end;

	// Find the sprites section in this resource file.
	switch (wadfiles[wadnum]->type)
	{
	case RET_WAD:
		start = W_CheckNumForMarkerStartPwad("S_START", wadnum, 0);
		if (start == INT16_MAX)
			start = W_CheckNumForMarkerStartPwad("SS_START", wadnum, 0); //deutex compatib.

		end = W_CheckNumForNamePwad("S_END",wadnum,start);
		if (end == INT16_MAX)
			end = W_CheckNumForNamePwad("SS_END",wadnum,start);     //deutex compatib.
		break;
	case RET_PK3:
	case RET_FOLDER:
		start = W_CheckNumForFolderStartPK3("Sprites/", wadnum, 0);
		end = W_CheckNumForFolderEndPK3("Sprites/", wadnum, start);
		break;
	default:
		return;
	}

	if (start == INT16_MAX)
	{
		// ignore skin wads (we don't want skin sprites interfering with vanilla sprites)
		if (W_CheckNumForNamePwad("S_SKIN", wadnum, 0) != UINT16_MAX)
			return;

		start = 0; //let say S_START is lump 0
	}

	if (end == INT16_MAX || start >= end)
	{
		CONS_Debug(DBG_SETUP, "no sprites in pwad %d\n", wadnum);
		return;
	}

	//
	// scan through lumps, for each sprite, find all the sprite frames
	//
	for (i = 0; i < numsprites; i++)
	{
		if (R_AddSingleSpriteDef(sprnames[i], &sprites[i], wadnum, start, end, false))
		{
			// if a new sprite was added (not just replaced)
			(*ptr_spritesadded)++;
#ifndef ZDEBUG
			CONS_Debug(DBG_SETUP, "sprite %s set in pwad %d\n", sprnames[i], wadnum);
#endif
		}
	}

	*ptr_framesadded += end - start;
}

static void AddLongSpriteDefs(UINT16 wadnum, size_t *ptr_spritesadded, size_t *ptr_framesadded)
{
	if (!W_FileHasFolders(wadfiles[wadnum]))
		return;

	UINT16 start = W_CheckNumForFolderStartPK3("LongSprites/", wadnum, 0);
	UINT16 end = W_CheckNumForFolderEndPK3("LongSprites/", wadnum, start);

	if (start == INT16_MAX || end == INT16_MAX || start >= end)
		return;

	size_t lumpnum = start;

	while (lumpnum < end)
	{
		if (W_IsLumpFolder(wadnum, lumpnum))
		{
			lumpnum++;
			continue;
		}

		UINT16 folderstart, folderend;
		char *folderpath = W_GetLumpFolderPathPK3(wadnum, lumpnum);
		folderstart = lumpnum;
		folderend = W_CheckNumForFolderEndPK3(folderpath, wadnum, lumpnum);
		Z_Free(folderpath);

		spritenum_t sprnum;
		char *sprname = W_GetLumpFolderNamePK3(wadnum, lumpnum);
		strupr(sprname);
		sprnum = R_GetSpriteNumByName(sprname);

		if (sprnum != NUMSPRITES && R_AddSingleSpriteDef(sprname, &sprites[sprnum], wadnum, folderstart, folderend, true))
		{
			// A new sprite was added (not just replaced)
			(*ptr_spritesadded)++;
#ifndef ZDEBUG
			CONS_Debug(DBG_SETUP, "long sprite %s set in pwad %d\n", sprname, wadnum);
#endif
		}

		Z_Free(sprname);

		lumpnum = folderend;
	}

	*ptr_framesadded += end - start;
}

//
// Search for sprites replacements in a wad whose names are in namelist
//
void R_AddSpriteDefs(UINT16 wadnum)
{
	char wadname[MAX_WADPATH];
	size_t spritesadded = 0;
	size_t framesadded = 0;

	AddShortSpriteDefs(wadnum, &spritesadded, &framesadded);
	AddLongSpriteDefs(wadnum, &spritesadded, &framesadded);

	if (spritesadded || framesadded)
	{
		nameonly(strcpy(wadname, wadfiles[wadnum]->filename));
		CONS_Printf(M_GetText("%s added %s frames in %s sprites\n"), wadname, sizeu1(framesadded), sizeu2(spritesadded));
	}
}

//
// GAME FUNCTIONS
//
UINT32 visspritecount, numvisiblesprites;

static UINT32 clippedvissprites;
static vissprite_t *visspritechunks[MAXVISSPRITES >> VISSPRITECHUNKBITS] = {NULL};

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(void)
{
	size_t i;
#ifdef ROTSPRITE
	INT32 angle;
	float fa;
#endif

	for (i = 0; i < MAXVIDWIDTH; i++)
		negonearray[i] = -1;

#ifdef ROTSPRITE
	for (angle = 1; angle < ROTANGLES; angle++)
	{
		fa = ANG2RAD(FixedAngle((ROTANGDIFF * angle)<<FRACBITS));
		rollcosang[angle] = FLOAT_TO_FIXED(cos(-fa));
		rollsinang[angle] = FLOAT_TO_FIXED(sin(-fa));
	}
#endif

	//
	// count the number of sprite names, and allocate sprites table
	//
	numsprites = 0;
	for (i = 0; i < NUMSPRITES + 1; i++)
		if (sprnames[i][0] != '\0') numsprites++;

	if (!numsprites)
		I_Error("R_AddSpriteDefs: no sprites in namelist\n");

	sprites = Z_Calloc(numsprites * sizeof (*sprites), PU_STATIC, NULL);

	// find sprites in each -file added pwad
	for (i = 0; i < numwadfiles; i++)
		R_AddSpriteDefs((UINT16)i);

	//
	// now check for skins
	//

	// it can be is do before loading config for skin cvar possible value
	R_InitSkins();
	for (i = 0; i < numwadfiles; i++)
	{
		R_AddSkins((UINT16)i, true);
		R_PatchSkins((UINT16)i, true);
		R_LoadSpriteInfoLumps(i, wadfiles[i]->numlumps);
	}
	ST_ReloadSkinFaceGraphics();

#ifdef HWRENDER
	if (rendermode == render_opengl)
		HWR_LoadModels();
#endif
}

//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites(void)
{
	visspritecount = numvisiblesprites = clippedvissprites = 0;
}

//
// R_NewVisSprite
//
static vissprite_t overflowsprite;

static vissprite_t *R_GetVisSprite(UINT32 num)
{
		UINT32 chunk = num >> VISSPRITECHUNKBITS;

		// Allocate chunk if necessary
		if (!visspritechunks[chunk])
			Z_Malloc(sizeof(vissprite_t) * VISSPRITESPERCHUNK, PU_LEVEL, &visspritechunks[chunk]);

		return visspritechunks[chunk] + (num & VISSPRITEINDEXMASK);
}

static vissprite_t *R_NewVisSprite(void)
{
	if (visspritecount == MAXVISSPRITES)
		return &overflowsprite;

	return R_GetVisSprite(visspritecount++);
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
INT16 *mfloorclip;
INT16 *mceilingclip;

fixed_t spryscale = 0, sprtopscreen = 0, sprbotscreen = 0;
fixed_t windowtop = 0, windowbottom = 0;

void R_DrawMaskedColumn(column_t *column, unsigned lengthcol)
{
	fixed_t basetexturemid = dc_texturemid;

	(void)lengthcol;

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];

		dc_postlength = post->length;

		INT32 topscreen = sprtopscreen + spryscale*post->topdelta;
		INT32 bottomscreen = topscreen + spryscale*dc_postlength;

		dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
		dc_yh = (bottomscreen-1)>>FRACBITS;

		if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
		{
			if (windowtop > topscreen)
				dc_yl = (windowtop + FRACUNIT - 1)>>FRACBITS;
			if (windowbottom < bottomscreen)
				dc_yh = (windowbottom - 1)>>FRACBITS;
		}

		if (dc_yh >= mfloorclip[dc_x])
			dc_yh = mfloorclip[dc_x]-1;
		if (dc_yl <= mceilingclip[dc_x])
			dc_yl = mceilingclip[dc_x]+1;
		if (dc_yl < 0)
			dc_yl = 0;
		if (dc_yh >= vid.height) // dc_yl must be < vid.height, so reduces number of checks in tight loop
			dc_yh = vid.height - 1;

		if (dc_yl <= dc_yh && dc_yh > 0)
		{
			dc_source = column->pixels + post->data_offset;
			dc_texturemid = basetexturemid - (post->topdelta<<FRACBITS);

			colfunc();
		}
	}

	dc_texturemid = basetexturemid;
}

static UINT8 *flippedcol = NULL;
static size_t flippedcolsize = 0;

void R_DrawFlippedPost(UINT8 *source, unsigned length, void (*drawcolfunc)(void))
{
	if (!length)
		return;

	if (!flippedcolsize || length > flippedcolsize)
	{
		flippedcolsize = length;
		flippedcol = Z_Realloc(flippedcol, length, PU_STATIC, NULL);
	}

	dc_source = flippedcol;

	for (UINT8 *s = (UINT8 *)source, *d = flippedcol+length-1; d >= flippedcol; s++)
		*d-- = *s;

	drawcolfunc();
}

void R_DrawFlippedMaskedColumn(column_t *column, unsigned lengthcol)
{
	INT32 topscreen;
	INT32 bottomscreen;
	fixed_t basetexturemid = dc_texturemid;
	INT32 topdelta;

	for (unsigned i = 0; i < column->num_posts; i++)
	{
		post_t *post = &column->posts[i];
		if (!post->length)
			continue;

		dc_postlength = post->length;

		topdelta = lengthcol-dc_postlength-post->topdelta;
		topscreen = sprtopscreen + spryscale*topdelta;
		bottomscreen = sprbotscreen == INT32_MAX ? topscreen + spryscale*dc_postlength
		                                      : sprbotscreen + spryscale*dc_postlength;

		dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
		dc_yh = (bottomscreen-1)>>FRACBITS;

		if (windowtop != INT32_MAX && windowbottom != INT32_MAX)
		{
			if (windowtop > topscreen)
				dc_yl = (windowtop + FRACUNIT - 1)>>FRACBITS;
			if (windowbottom < bottomscreen)
				dc_yh = (windowbottom - 1)>>FRACBITS;
		}

		if (dc_yh >= mfloorclip[dc_x])
			dc_yh = mfloorclip[dc_x]-1;
		if (dc_yl <= mceilingclip[dc_x])
			dc_yl = mceilingclip[dc_x]+1;
		if (dc_yl < 0)
			dc_yl = 0;
		if (dc_yh >= vid.height) // dc_yl must be < vid.height, so reduces number of checks in tight loop
			dc_yh = vid.height - 1;

		if (dc_yl <= dc_yh && dc_yh > 0)
		{
			dc_texturemid = basetexturemid - (topdelta<<FRACBITS);

			R_DrawFlippedPost(column->pixels + post->data_offset, post->length, colfunc);
		}
	}

	dc_texturemid = basetexturemid;
}

UINT8 *R_GetTranslationForThing(mobj_t *mobj, skincolornum_t color, UINT16 translation)
{
	INT32 skinnum = TC_DEFAULT;

	boolean is_player = mobj->skin && mobj->sprite == SPR_PLAY;
	if (is_player) // This thing is a player!
		skinnum = ((skin_t*)mobj->skin)->skinnum;

	if (color != SKINCOLOR_NONE)
	{
		// New colormap stuff for skins Tails 06-07-2002
		if ((mobj->state-states == S_METALSONIC_DASH || mobj->state-states == S_METALSONIC_BOUNCE) && (((leveltime/2) & 1))) // Metal boss doing attack
			skinnum = TC_DASHMODE;
		else if (mobj->colorized)
			skinnum = TC_RAINBOW;
		else if (mobj->player && mobj->player->dashmode >= DASHMODE_THRESHOLD
			&& (mobj->player->charflags & SF_DASHMODE)
			&& ((leveltime/2) & 1))
		{
			if (mobj->player->charflags & SF_MACHINE)
				skinnum = TC_DASHMODE;
			else
				skinnum = TC_RAINBOW;
		}
	}

	if (R_ThingIsFlashing(mobj)) // Bosses "flash"
	{
		if (mobj->type == MT_CYBRAKDEMON || mobj->colorized)
			return R_GetTranslationColormap(TC_ALLWHITE, 0, GTC_CACHE);
		else if (mobj->type == MT_METALSONIC_BATTLE)
			return R_GetTranslationColormap(TC_METALSONIC, 0, GTC_CACHE);
		else
			return R_GetTranslationColormap(TC_BOSS, color, GTC_CACHE);
	}
	else if (translation != 0)
	{
		UINT8 *tr = R_GetTranslationRemap(translation, color, skinnum);
		if (tr != NULL)
			return tr;
	}
	else if (color != SKINCOLOR_NONE)
		return R_GetTranslationColormap(skinnum, color, GTC_CACHE);
	else if (mobj->sprite == SPR_PLAY) // Looks like a player, but doesn't have a color? Get rid of green sonic syndrome.
		return R_GetTranslationColormap(TC_DEFAULT, SKINCOLOR_BLUE, GTC_CACHE);

	return NULL;
}

// Based off of R_GetLinedefTransTable
transnum_t R_GetThingTransTable(fixed_t alpha, transnum_t transmap)
{
	return (20*(FRACUNIT - ((alpha * (10 - transmap))/10) - 1) + FRACUNIT) >> (FRACBITS+1);
}

//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
static void R_DrawVisSprite(vissprite_t *vis)
{
	column_t *column;
	void (*localcolfunc)(column_t *, unsigned);
	INT32 pwidth;
	fixed_t frac;
	patch_t *patch = vis->patch;
	fixed_t this_scale = vis->thingscale;
	INT32 x1, x2;
	INT64 overflow_test;
	unsigned lengthcol;

	if (!patch)
		return;

	// Check for overflow
	overflow_test = (INT64)centeryfrac - (((INT64)vis->texturemid*vis->scale)>>FRACBITS);
	if (overflow_test < 0) overflow_test = -overflow_test;
	if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL) return; // fixed point mult would overflow

	if (vis->scalestep) // handles right edge too
	{
		overflow_test = (INT64)centeryfrac - (((INT64)vis->texturemid*(vis->scale + (vis->scalestep*(vis->x2 - vis->x1))))>>FRACBITS);
		if (overflow_test < 0) overflow_test = -overflow_test;
		if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL) return; // ditto
	}

	// TODO This check should not be necessary. But Papersprites near to the camera will sometimes create invalid values
	// for the vissprite's startfrac. This happens because they are not depth culled like other sprites.
	// Someone who is more familiar with papersprites pls check and try to fix <3
	if (vis->startfrac < 0 || vis->startfrac > (patch->width << FRACBITS))
	{
		// never draw vissprites with startfrac out of patch range
		return;
	}

	colfunc = colfuncs[BASEDRAWFUNC]; // hack: this isn't resetting properly somewhere.
	dc_colormap = vis->colormap;
	dc_translation = R_GetTranslationForThing(vis->mobj, vis->color, vis->translation);

	if (R_ThingIsFlashing(vis->mobj)) // Bosses "flash"
		colfunc = colfuncs[COLDRAWFUNC_TRANS]; // translate certain pixels to white
	else if (dc_translation && vis->transmap) // Color mapping
	{
		colfunc = colfuncs[COLDRAWFUNC_TRANSTRANS];
		dc_transmap = vis->transmap;
	}
	else if (vis->transmap)
	{
		colfunc = colfuncs[COLDRAWFUNC_FUZZY];
		dc_transmap = vis->transmap;    //Fab : 29-04-98: translucency table
	}
	else if (dc_translation) // translate green skin to another color
		colfunc = colfuncs[COLDRAWFUNC_TRANS];

	// Hack: Use a special column function for drop shadows that bypasses
	// invalid memory access crashes caused by R_ProjectDropShadow putting wrong values
	// in dc_texturemid and dc_iscale when the shadow is sloped.
	if (vis->cut & SC_SHADOW)
		colfunc = R_DrawDropShadowColumn_8;

	if (vis->extra_colormap && !(vis->renderflags & RF_NOCOLORMAPS))
	{
		if (!dc_colormap)
			dc_colormap = vis->extra_colormap->colormap;
		else
			dc_colormap = &vis->extra_colormap->colormap[dc_colormap - colormaps];
	}
	if (!dc_colormap)
		dc_colormap = colormaps;

	dc_texturemid = vis->texturemid;
	dc_texheight = 0;

	frac = vis->startfrac;
	windowtop = windowbottom = sprbotscreen = INT32_MAX;

	if (vis->cut & SC_SHADOW && vis->mobj->skin && ((skin_t *)vis->mobj->skin)->flags & SF_HIRES)
		this_scale = FixedMul(this_scale, ((skin_t *)vis->mobj->skin)->highresscale);
	if (this_scale <= 0)
		this_scale = 1;
	if (this_scale != FRACUNIT)
	{
		if (!(vis->cut & SC_ISSCALED))
		{
			vis->scale = FixedMul(vis->scale, this_scale);
			vis->scalestep = FixedMul(vis->scalestep, this_scale);
			vis->xiscale = FixedDiv(vis->xiscale, this_scale);
			vis->cut |= SC_ISSCALED;
		}
		dc_texturemid = FixedDiv(dc_texturemid, this_scale);
	}

	spryscale = vis->scale;

	if (!(vis->scalestep))
	{
		sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
		sprtopscreen += vis->shear.tan * vis->shear.offset;
		dc_iscale = FixedDiv(FRACUNIT, vis->scale);
	}

	x1 = vis->x1;
	x2 = vis->x2;

	if (vis->x1 < 0)
	{
		spryscale += vis->scalestep*(-vis->x1);
		vis->x1 = 0;
	}

	if (vis->x2 >= vid.width)
		vis->x2 = vid.width-1;

	localcolfunc = (vis->cut & SC_VFLIP) ? R_DrawFlippedMaskedColumn : R_DrawMaskedColumn;
	lengthcol = patch->height;

	// Split drawing loops for paper and non-paper to reduce conditional checks per sprite
	if (vis->scalestep)
	{
		fixed_t horzscale = FixedMul(vis->spritexscale, this_scale);
		fixed_t scalestep = FixedMul(vis->scalestep, vis->spriteyscale);

		pwidth = patch->width;

		// Papersprite drawing loop
		for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, spryscale += scalestep)
		{
			angle_t angle = ((vis->centerangle + xtoviewangle[dc_x]) >> ANGLETOFINESHIFT) & 0xFFF;
			INT32 texturecolumn = (vis->paperoffset - FixedMul(FINETANGENT(angle), vis->paperdistance)) / horzscale;

			if (texturecolumn < 0 || texturecolumn >= pwidth)
				continue;

			if (vis->xiscale < 0) // Flipped sprite
				texturecolumn = pwidth - 1 - texturecolumn;

			sprtopscreen = (centeryfrac - FixedMul(dc_texturemid, spryscale));
			dc_iscale = (0xffffffffu / (unsigned)spryscale);

			column = &patch->columns[texturecolumn];

			localcolfunc (column, lengthcol);
		}
	}
	else if (vis->cut & SC_SHEAR)
	{
#ifdef RANGECHECK
		pwidth = patch->width;
#endif

		// Vertically sheared sprite
		for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale, dc_texturemid -= vis->shear.tan)
		{
			column = &patch->columns[frac>>FRACBITS];
			sprtopscreen = (centeryfrac - FixedMul(dc_texturemid, spryscale));
			localcolfunc (column, lengthcol);
		}
	}
	else
	{
#ifdef RANGECHECK
		pwidth = patch->width;
#endif

		// Non-paper drawing loop
		for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale, sprtopscreen += vis->shear.tan)
		{
			column = &patch->columns[frac>>FRACBITS];
			localcolfunc (column, lengthcol);
		}
	}

	colfunc = colfuncs[BASEDRAWFUNC];

	vis->x1 = x1;
	vis->x2 = x2;
}

// Special precipitation drawer Tails 08-18-2002
static void R_DrawPrecipitationVisSprite(vissprite_t *vis)
{
	patch_t *patch = vis->patch;
	if (!patch)
		return;

	// Check for overflow
	INT64 overflow_test = (INT64)centeryfrac - (((INT64)vis->texturemid*vis->scale)>>FRACBITS);
	if (overflow_test < 0) overflow_test = -overflow_test;
	if ((UINT64)overflow_test&0xFFFFFFFF80000000ULL) return; // fixed point mult would overflow

	if (vis->transmap)
	{
		colfunc = colfuncs[COLDRAWFUNC_FUZZY];
		dc_transmap = vis->transmap;    //Fab : 29-04-98: translucency table
	}

	dc_colormap = colormaps;

	dc_iscale = FixedDiv(FRACUNIT, vis->scale);
	dc_texturemid = vis->texturemid;
	dc_texheight = 0;

	spryscale = vis->scale;
	sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);
	windowtop = windowbottom = sprbotscreen = INT32_MAX;

	if (vis->x1 < 0)
		vis->x1 = 0;
	if (vis->x2 >= vid.width)
		vis->x2 = vid.width-1;

	fixed_t frac = vis->startfrac;

	for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale)
		R_DrawMaskedColumn(&patch->columns[frac>>FRACBITS], patch->height);

	colfunc = colfuncs[BASEDRAWFUNC];
}

//
// R_SplitSprite
// runs through a sector's lightlist and Knuckles
static void R_SplitSprite(vissprite_t *sprite)
{
	INT32 i, lightnum, lindex;
	INT16 cutfrac;
	sector_t *sector;
	vissprite_t *newsprite;

	sector = sprite->sector;

	for (i = 1; i < sector->numlights; i++)
	{
		fixed_t testheight;

		if (!(sector->lightlist[i].caster->fofflags & FOF_CUTSPRITES))
			continue;

		testheight = P_GetLightZAt(&sector->lightlist[i], sprite->gx, sprite->gy);

		if (testheight >= sprite->gzt)
			continue;
		if (testheight <= sprite->gz)
			return;

		cutfrac = (INT16)((centeryfrac - FixedMul(testheight - viewz, sprite->linkscale))>>FRACBITS);
		if (cutfrac < 0)
			continue;
		if (cutfrac > viewheight)
			return;

		// Found a split! Make a new sprite, copy the old sprite to it, and
		// adjust the heights.
		newsprite = M_Memcpy(R_NewVisSprite(), sprite, sizeof (vissprite_t));

		newsprite->cut |= (sprite->cut & SC_FLAGMASK);

		sprite->cut |= SC_BOTTOM;
		sprite->gz = testheight;

		newsprite->gzt = sprite->gz;

		sprite->sz = cutfrac;
		newsprite->szt = (INT16)(sprite->sz - 1);

		if (testheight < sprite->pzt && testheight > sprite->pz)
			sprite->pz = newsprite->pzt = testheight;
		else
		{
			newsprite->pz = newsprite->gz;
			newsprite->pzt = newsprite->gzt;
		}

		newsprite->szt -= 8;

		newsprite->cut |= SC_TOP;
		if (!(sector->lightlist[i].caster->fofflags & FOF_NOSHADE))
		{
			lightnum = (*sector->lightlist[i].lightlevel >> LIGHTSEGSHIFT);

			if (lightnum < 0)
				spritelights = scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
				spritelights = scalelight[LIGHTLEVELS-1];
			else
				spritelights = scalelight[lightnum];

			newsprite->extra_colormap = *sector->lightlist[i].extra_colormap;

			if (!(newsprite->cut & SC_FULLBRIGHT)
				|| (newsprite->extra_colormap && (newsprite->extra_colormap->flags & CMF_FADEFULLBRIGHTSPRITES)))
			{
				lindex = FixedMul(sprite->xscale, LIGHTRESOLUTIONFIX)>>(LIGHTSCALESHIFT);

				if (lindex >= MAXLIGHTSCALE)
					lindex = MAXLIGHTSCALE-1;

				if (newsprite->cut & SC_SEMIBRIGHT)
					lindex = (MAXLIGHTSCALE/2) + (lindex >>1);

				newsprite->colormap = spritelights[lindex];
			}
		}
		sprite = newsprite;
	}
}

//
// R_GetShadowZ(thing, shadowslope)
// Get the first visible floor below the object for shadows
// shadowslope is filled with the floor's slope, if provided
//
fixed_t R_GetShadowZ(mobj_t *thing, pslope_t **shadowslope)
{
	fixed_t halfHeight;
	boolean isflipped = thing->eflags & MFE_VERTICALFLIP;
	fixed_t floorz;
	fixed_t ceilingz;
	fixed_t z, groundz = isflipped ? INT32_MAX : INT32_MIN;
	pslope_t *slope, *groundslope = NULL;
	msecnode_t *node;
	sector_t *sector;
	ffloor_t *rover;

	// for frame interpolation
	interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	halfHeight = interp.z + (interp.height >> 1);
	floorz = P_GetFloorZ(thing, interp.subsector->sector, interp.x, interp.y, NULL);
	ceilingz = P_GetCeilingZ(thing, interp.subsector->sector, interp.x, interp.y, NULL);

#define CHECKZ (isflipped ? z > halfHeight && z < groundz : z < halfHeight && z > groundz)

	for (node = thing->touching_sectorlist; node; node = node->m_sectorlist_next)
	{
		sector = node->m_sector;

		slope = sector->heightsec != -1 ? NULL : (isflipped ? sector->c_slope : sector->f_slope);

		if (sector->heightsec != -1)
			z = isflipped ? sectors[sector->heightsec].ceilingheight : sectors[sector->heightsec].floorheight;
		else
			z = isflipped ? P_GetSectorCeilingZAt(sector, interp.x, interp.y) : P_GetSectorFloorZAt(sector, interp.x, interp.y);

		if CHECKZ
		{
			groundz = z;
			groundslope = slope;
		}

		if (sector->ffloors)
			for (rover = sector->ffloors; rover; rover = rover->next)
			{
				if (!(rover->fofflags & FOF_EXISTS) || !(rover->fofflags & FOF_RENDERPLANES) || (rover->alpha < 90 && !(rover->fofflags & FOF_SWIMMABLE)))
					continue;

				z = isflipped ? P_GetFFloorBottomZAt(rover, interp.x, interp.y) : P_GetFFloorTopZAt(rover, interp.x, interp.y);
				if CHECKZ
				{
					groundz = z;
					groundslope = isflipped ? *rover->b_slope : *rover->t_slope;
				}
			}
	}

	// Check polyobjects and see if groundz needs to be altered
	{
		// This isn't very precise, but the precise method was far too slow.
		// (Polies are just naturally pretty flickery anyway :P)
		polyobj_t *po = interp.subsector->polyList;

		while (po)
		{
			if (!(po->flags & POF_RENDERPLANES) || !P_MobjInsidePolyobj(po, thing))
			{
				po = (polyobj_t *)(po->link.next);
				continue;
			}

			// We're inside it! Yess...
			z = isflipped ? po->lines[0]->backsector->floorheight : po->lines[0]->backsector->ceilingheight;
			if CHECKZ
			{
				groundz = z;
				groundslope = NULL;
			}

			po = (polyobj_t *)(po->link.next);
		}
	}

	if (isflipped ? (ceilingz < groundz - (!groundslope ? 0 : FixedMul(abs(groundslope->zdelta), interp.radius*3/2)))
		: (floorz > groundz + (!groundslope ? 0 : FixedMul(abs(groundslope->zdelta), interp.radius*3/2))))
	{
		groundz = isflipped ? ceilingz : floorz;
		groundslope = NULL;
	}

	if (shadowslope != NULL)
		*shadowslope = groundslope;

	return groundz;
#undef CHECKZ
}

static void R_SkewShadowSprite(
			mobj_t *thing, pslope_t *groundslope,
			fixed_t groundz, INT32 spriteheight, fixed_t scalemul,
			fixed_t *shadowyscale, fixed_t *shadowskew)
{

	// haha let's try some dumb stuff
	fixed_t xslope, zslope;
	angle_t sloperelang;

	// for frame interpolation
	interpmobjstate_t interp = {0};

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	sloperelang = (R_PointToAngle(interp.x, interp.y) - groundslope->xydirection) >> ANGLETOFINESHIFT;

	xslope = FixedMul(FINESINE(sloperelang), groundslope->zdelta);
	zslope = FixedMul(FINECOSINE(sloperelang), groundslope->zdelta);

	//CONS_Printf("Shadow is sloped by %d %d\n", xslope, zslope);

	if (viewz < groundz)
		*shadowyscale += FixedMul(FixedMul(interp.radius*2 / spriteheight, scalemul), zslope);
	else
		*shadowyscale -= FixedMul(FixedMul(interp.radius*2 / spriteheight, scalemul), zslope);

	*shadowyscale = abs((*shadowyscale));
	*shadowskew = xslope;
}

static void R_ProjectDropShadow(mobj_t *thing, vissprite_t *vis, fixed_t scale, fixed_t tx, fixed_t tz)
{
	vissprite_t *shadow;
	patch_t *patch;
	fixed_t xscale, yscale, shadowxscale, shadowyscale, shadowskew, x1, x2;
	INT32 heightsec, phs;
	fixed_t scalemul;
	UINT8 trans;
	fixed_t floordiff;
	fixed_t groundz;
	pslope_t *groundslope;
	boolean isflipped = thing->eflags & MFE_VERTICALFLIP;
	interpmobjstate_t interp = {0};

	groundz = R_GetShadowZ(thing, &groundslope);

	if (abs(groundz-viewz)/tz > 4) return; // Prevent stretchy shadows and possible crashes

	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1) // only clip things which are in special sectors
	{
		if (viewz < sectors[phs].floorheight ?
		groundz >= sectors[heightsec].floorheight :
		groundz < sectors[heightsec].floorheight)
			return;
		if (viewz > sectors[phs].ceilingheight ?
		groundz < sectors[heightsec].ceilingheight && viewz >= sectors[heightsec].ceilingheight :
		groundz >= sectors[heightsec].ceilingheight)
			return;
	}

	floordiff = abs((isflipped ? interp.height : 0) + interp.z - groundz);

	trans = floordiff / (100*FRACUNIT) + 3;
	trans = R_GetThingTransTable(thing->alpha, trans);
	if (trans >= 9) return;

	scalemul = FixedMul(FRACUNIT - floordiff/640, scale);

	patch = W_CachePatchName("DSHADOW", PU_SPRITE);
	xscale = FixedDiv(projection, tz);
	yscale = FixedDiv(projectiony, tz);
	shadowxscale = FixedMul(interp.radius*2, scalemul);
	shadowyscale = FixedMul(FixedMul(interp.radius*2, scalemul), FixedDiv(abs(groundz - viewz), tz));
	shadowyscale = min(shadowyscale, shadowxscale) / patch->height;
	shadowxscale /= patch->width;
	shadowskew = 0;

	if (groundslope)
		R_SkewShadowSprite(thing, groundslope, groundz, patch->height, scalemul, &shadowyscale, &shadowskew);

	tx -= patch->width * shadowxscale/2;
	x1 = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;
	if (x1 >= viewwidth) return;

	tx += patch->width * shadowxscale;
	x2 = ((centerxfrac + FixedMul(tx,xscale))>>FRACBITS); x2--;
	if (x2 < 0 || x2 <= x1) return;

	if (shadowyscale < FRACUNIT/patch->height) return; // fix some crashes?

	shadow = R_NewVisSprite();
	shadow->patch = patch;
	shadow->heightsec = vis->heightsec;

	shadow->thingheight = FRACUNIT;
	shadow->pz = groundz + (isflipped ? -shadow->thingheight : 0);
	shadow->pzt = shadow->pz + shadow->thingheight;

	shadow->mobjflags = 0;
	shadow->sortscale = vis->sortscale;
	shadow->dispoffset = vis->dispoffset - 5;
	shadow->gx = interp.x;
	shadow->gy = interp.y;
	shadow->gzt = (isflipped ? shadow->pzt : shadow->pz) + patch->height * shadowyscale / 2;
	shadow->gz = shadow->gzt - patch->height * shadowyscale;
	shadow->texturemid = FixedMul(interp.scale, FixedDiv(shadow->gzt - viewz, shadowyscale));
	if (thing->skin && ((skin_t *)thing->skin)->flags & SF_HIRES)
		shadow->texturemid = FixedMul(shadow->texturemid, ((skin_t *)thing->skin)->highresscale);
	shadow->scalestep = 0;
	shadow->shear.tan = shadowskew; // repurposed variable

	shadow->mobj = thing; // Easy access! Tails 06-07-2002
	shadow->color = thing->color;
	shadow->translation = 0;

	shadow->x1 = x1 < portalclipstart ? portalclipstart : x1;
	shadow->x2 = x2 >= portalclipend ? portalclipend-1 : x2;

	shadow->xscale = FixedMul(xscale, shadowxscale); //SoM: 4/17/2000
	shadow->scale = FixedMul(yscale, shadowyscale);
	shadow->thingscale = interp.scale;
	shadow->sector = vis->sector;
	shadow->szt = (INT16)((centeryfrac - FixedMul(shadow->gzt - viewz, yscale))>>FRACBITS);
	shadow->sz = (INT16)((centeryfrac - FixedMul(shadow->gz - viewz, yscale))>>FRACBITS);
	shadow->cut = SC_ISSCALED|SC_SHADOW; //check this

	shadow->startfrac = 0;
	//shadow->xiscale = 0x7ffffff0 / (shadow->xscale/2);
	shadow->xiscale = (patch->width<<FRACBITS)/(x2-x1+1); // fuck it

	if (shadow->x1 > x1)
		shadow->startfrac += shadow->xiscale*(shadow->x1-x1);

	// reusing x1 variable
	x1 += (x2-x1)/2;
	shadow->shear.offset = shadow->x1-x1;

	if (thing->renderflags & RF_NOCOLORMAPS)
		shadow->extra_colormap = NULL;
	else
		shadow->extra_colormap = P_GetColormapFromSectorAt(thing->subsector->sector, interp.x, interp.y, shadow->gzt);

	shadow->transmap = R_GetTranslucencyTable(trans + 1);
	shadow->colormap = scalelight[0][0]; // full dark!

	objectsdrawn++;
}

static void R_ProjectBoundingBox(mobj_t *thing, vissprite_t *vis)
{
	fixed_t gx, gy;
	fixed_t tx, tz;

	vissprite_t *box;

	if (!R_ThingBoundingBoxVisible(thing))
	{
		return;
	}

	// uncapped/interpolation
	boolean interpolate = cv_renderhitboxinterpolation.value;
	interpmobjstate_t interp = {0};

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused && interpolate)
	{
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(thing, FRACUNIT, &interp);
	}

	// 1--3
	// |  |
	// 0--2

	// start in the (0) corner
	gx = interp.x - interp.radius - viewx;
	gy = interp.y - interp.radius - viewy;

	tz = FixedMul(gx, viewcos) + FixedMul(gy, viewsin);

	// thing is behind view plane?
	// if parent vis is visible, ignore this
	if (!vis && (tz < FixedMul(MINZ, interp.scale)))
	{
		return;
	}

	tx = FixedMul(gx, viewsin) - FixedMul(gy, viewcos);

	// too far off the side?
	if (!vis && abs(tx) > FixedMul(tz, fovtan)<<2)
	{
		return;
	}

	box = R_NewVisSprite();
	box->mobj = thing;
	box->mobjflags = thing->flags;
	box->thingheight = interp.height;
	box->cut = SC_BBOX;

	box->gx = tx;
	box->gy = tz;

	box->scale  = 2 * FixedMul(interp.radius, viewsin);
	box->xscale = 2 * FixedMul(interp.radius, viewcos);

	box->pz = interp.z;
	box->pzt = box->pz + box->thingheight;

	box->gzt = box->pzt;
	box->gz = box->pz;
	box->texturemid = box->gzt - viewz;

	if (vis)
	{
		box->x1 = vis->x1;
		box->x2 = vis->x2;
		box->szt = vis->szt;
		box->sz = vis->sz;

		box->sortscale = vis->sortscale; // link sorting to sprite
		box->dispoffset = vis->dispoffset + 5;

		box->cut |= SC_LINKDRAW;
	}
	else
	{
		fixed_t xscale = FixedDiv(projection, tz);
		fixed_t yscale = FixedDiv(projectiony, tz);
		fixed_t top = (centeryfrac - FixedMul(box->texturemid, yscale));

		box->x1 = (centerxfrac + FixedMul(box->gx, xscale)) / FRACUNIT;
		box->x2 = box->x1;

		box->szt = top / FRACUNIT;
		box->sz = (top + FixedMul(box->thingheight, yscale)) / FRACUNIT;

		box->sortscale = yscale;
		box->dispoffset = 0;
	}
}

//
// R_ProjectSprite
// Generates a vissprite for a thing
// if it might be visible.
//
static void R_ProjectSprite(mobj_t *thing)
{
	mobj_t *oldthing = thing;
	fixed_t tr_x, tr_y;
	fixed_t tx, tz;
	fixed_t xscale, yscale; //added : 02-02-98 : aaargll..if I were a math-guy!!!
	fixed_t radius, height; // For drop shadows
	fixed_t sortscale, sortsplat = 0;
	fixed_t linkscale = 0;
	fixed_t sort_x = 0, sort_y = 0, sort_z;

	INT32 x1, x2;

	spritedef_t *sprdef;
	spriteframe_t *sprframe;
#ifdef ROTSPRITE
	spriteinfo_t *sprinfo;
#endif
	size_t lump;

	size_t frame, rot;
	UINT16 flip;
	boolean vflip = (!(thing->eflags & MFE_VERTICALFLIP) != !R_ThingVerticallyFlipped(thing));
	boolean mirrored = thing->mirrored;
	boolean hflip = (!R_ThingHorizontallyFlipped(thing) != !mirrored);

	INT32 lindex;
	INT32 trans;

	vissprite_t *vis;
	patch_t *patch;

	spritecut_e cut = SC_NONE;

	angle_t ang = 0; // compiler complaints
	fixed_t iscale;
	fixed_t scalestep;
	fixed_t offset, offset2;

	fixed_t sheartan = 0;
	fixed_t shadowscale = FRACUNIT;
	fixed_t basetx, basetz; // drop shadows

	boolean shadowdraw, shadoweffects, shadowskew;
	boolean splat = R_ThingIsFloorSprite(thing);
	boolean papersprite = (R_ThingIsPaperSprite(thing) && !splat);
	fixed_t paperoffset = 0, paperdistance = 0;
	angle_t centerangle = 0;

	INT32 dispoffset = thing->dispoffset;

	//SoM: 3/17/2000
	fixed_t gz = 0, gzt = 0;
	INT32 heightsec, phs;
	INT32 light = 0;
	fixed_t this_scale;
	fixed_t spritexscale, spriteyscale;

	// rotsprite
	fixed_t spr_width, spr_height;
	fixed_t spr_offset, spr_topoffset;

#ifdef ROTSPRITE
	patch_t *rotsprite = NULL;
	INT32 rollangle = 0;
	angle_t spriterotangle = 0;
#endif

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	if (!r_renderthings)
		return;

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolateMobjState(oldthing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolateMobjState(oldthing, FRACUNIT, &interp);
	}

	this_scale = interp.scale;

	if (this_scale < 1)
		return;

	radius = interp.radius; // For drop shadows
	height = interp.height; // Ditto

	// transform the origin point
	if (thing->type == MT_OVERLAY) // Handle overlays
		R_ThingOffsetOverlay(thing, &interp.x, &interp.y);
	tr_x = interp.x - viewx;
	tr_y = interp.y - viewy;

	basetz = tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin); // near/far distance

	// thing is behind view plane?
	if (!papersprite && (tz < FixedMul(MINZ, this_scale))) // papersprite clipping is handled later
		return;

	basetx = tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos); // sideways distance

	// too far off the side?
	if (!papersprite && abs(tx) > FixedMul(tz, fovtan)<<2) // papersprite clipping is handled later
		return;

	// aspect ratio stuff
	xscale = FixedDiv(projection, tz);
	sortscale = FixedDiv(projectiony, tz);

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((size_t)(thing->sprite) >= numsprites)
		I_Error("R_ProjectSprite: invalid sprite number %d", thing->sprite);
#endif

	frame = thing->frame & FF_FRAMEMASK;

	//Fab : 02-08-98: 'skin' override spritedef currently used for skin
	if (thing->skin && thing->sprite == SPR_PLAY)
	{
		sprdef = P_GetSkinSpritedef(thing->skin, thing->sprite2);
#ifdef ROTSPRITE
		sprinfo = P_GetSkinSpriteInfo(thing->skin, thing->sprite2);
#endif

		if (frame >= sprdef->numframes)
		{
			CONS_Alert(CONS_ERROR, M_GetText("R_ProjectSprite: invalid skins[\"%s\"].sprites[SPR2_%s] %sframe %s\n"), ((skin_t *)thing->skin)->name, spr2names[thing->sprite2 & SPR2F_MASK], (thing->sprite2 & SPR2F_SUPER) ? "super ": "", sizeu5(frame));
			thing->sprite = states[S_UNKNOWN].sprite;
			thing->frame = states[S_UNKNOWN].frame;
			sprdef = &sprites[thing->sprite];
#ifdef ROTSPRITE
			sprinfo = &spriteinfo[thing->sprite];
#endif
			frame = thing->frame&FF_FRAMEMASK;
		}
	}
	else
	{
		sprdef = &sprites[thing->sprite];
#ifdef ROTSPRITE
		sprinfo = &spriteinfo[thing->sprite];
#endif

		if (frame >= sprdef->numframes)
		{
			CONS_Alert(CONS_ERROR, M_GetText("R_ProjectSprite: invalid sprite frame %s/%s for %s\n"),
				sizeu1(frame), sizeu2(sprdef->numframes), sprnames[thing->sprite]);
			if (thing->sprite == thing->state->sprite && thing->frame == thing->state->frame)
			{
				thing->state->sprite = states[S_UNKNOWN].sprite;
				thing->state->frame = states[S_UNKNOWN].frame;
			}
			thing->sprite = states[S_UNKNOWN].sprite;
			thing->frame = states[S_UNKNOWN].frame;
			sprdef = &sprites[thing->sprite];
			sprinfo = &spriteinfo[thing->sprite];
			frame = thing->frame&FF_FRAMEMASK;
		}
	}

	sprframe = &sprdef->spriteframes[frame];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("R_ProjectSprite: sprframes NULL for sprite %d\n", thing->sprite);
#endif

	if (splat)
	{
		ang = R_PointToAngle2(0, viewz, 0, interp.z);
	}
	else if (sprframe->rotate != SRF_SINGLE || papersprite)
	{
		ang = R_PointToAngle (interp.x, interp.y) - interp.angle;
		if (mirrored)
			ang = InvAngle(ang);
	}

	if (sprframe->rotate == SRF_SINGLE)
	{
		// use single rotation for all views
		rot = 0;                        //Fab: for vis->patch below
		lump = sprframe->lumpid[0];     //Fab: see note above
		flip = sprframe->flip; 			// Will only be 0 or 0xFFFF
	}
	else
	{
		// choose a different rotation based on player view
		//ang = R_PointToAngle (interp.x, interp.y) - interpangle;

		if ((sprframe->rotate & SRF_RIGHT) && (ang < ANGLE_180)) // See from right
			rot = 6; // F7 slot
		else if ((sprframe->rotate & SRF_LEFT) && (ang >= ANGLE_180)) // See from left
			rot = 2; // F3 slot
		else if (sprframe->rotate & SRF_3DGE) // 16-angle mode
		{
			rot = (ang+ANGLE_180+ANGLE_11hh)>>28;
			rot = ((rot & 1)<<3)|(rot>>1);
		}
		else // Normal behaviour
			rot = (ang+ANGLE_202h)>>29;

		//Fab: lumpid is the index for spritewidth,spriteoffset... tables
		lump = sprframe->lumpid[rot];
		flip = sprframe->flip & (1<<rot);
	}

	I_Assert(lump < max_spritelumps);

	spr_width = spritecachedinfo[lump].width;
	spr_height = spritecachedinfo[lump].height;
	spr_offset = spritecachedinfo[lump].offset;
	spr_topoffset = spritecachedinfo[lump].topoffset;

	//Fab: lumppat is the lump number of the patch to use, this is different
	//     than lumpid for sprites-in-pwad : the graphics are patched
	patch = W_CachePatchNum(sprframe->lumppat[rot], PU_SPRITE);

#ifdef ROTSPRITE
	spriterotangle = R_SpriteRotationAngle(&interp);

	if (spriterotangle != 0
	&& !(splat && !(thing->renderflags & RF_NOSPLATROLLANGLE)))
	{
		if (papersprite && ang >= ANGLE_180)
		{
			// Makes Software act much more sane like OpenGL
			rollangle = R_GetRollAngle(InvAngle(spriterotangle));
		}
		else
		{
			rollangle = R_GetRollAngle(spriterotangle);
		}

		rotsprite = Patch_GetRotatedSprite(sprframe, (thing->frame & FF_FRAMEMASK), rot, flip, sprinfo, rollangle);

		if (rotsprite != NULL)
		{
			patch = rotsprite;
			cut |= SC_ISROTATED;

			spr_width = rotsprite->width << FRACBITS;
			spr_height = rotsprite->height << FRACBITS;
			spr_offset = rotsprite->leftoffset << FRACBITS;
			spr_topoffset = rotsprite->topoffset << FRACBITS;
			spr_topoffset += FEETADJUST;

			// flip -> rotate, not rotate -> flip
			flip = 0;
		}
	}
#endif

	flip = !flip != !hflip;

	// calculate edges of the shape
	spritexscale = interp.spritexscale;
	spriteyscale = interp.spriteyscale;

	if (thing->skin && ((skin_t *)thing->skin)->flags & SF_HIRES)
	{
		fixed_t highresscale = ((skin_t *)thing->skin)->highresscale;
		spritexscale = FixedMul(spritexscale, highresscale);
		spriteyscale = FixedMul(spriteyscale, highresscale);
	}

	if (spritexscale < 1 || spriteyscale < 1)
		return;

	if (thing->renderflags & RF_ABSOLUTEOFFSETS)
	{
		spr_offset = interp.spritexoffset;
		spr_topoffset = interp.spriteyoffset;
	}
	else
	{
		SINT8 flipoffset = 1;

		if ((thing->renderflags & RF_FLIPOFFSETS) && flip)
			flipoffset = -1;

		spr_offset += interp.spritexoffset * flipoffset;
		spr_topoffset += interp.spriteyoffset * flipoffset;
	}

	if (flip)
		offset = spr_offset - spr_width;
	else
		offset = -spr_offset;

	offset = FixedMul(offset, FixedMul(spritexscale, this_scale));
	offset2 = FixedMul(spr_width, FixedMul(spritexscale, this_scale));

	if (papersprite)
	{
		fixed_t xscale2, yscale2, cosmul, sinmul, tx2, tz2;
		INT32 range;

		if (ang >= ANGLE_180)
		{
			offset *= -1;
			offset2 *= -1;
		}

		cosmul = FINECOSINE(interp.angle >> ANGLETOFINESHIFT);
		sinmul = FINESINE(interp.angle >> ANGLETOFINESHIFT);

		tr_x += FixedMul(offset, cosmul);
		tr_y += FixedMul(offset, sinmul);
		tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);

		tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos);

		// Get paperoffset (offset) and paperoffset (distance)
		paperoffset = -FixedMul(tr_x, cosmul) - FixedMul(tr_y, sinmul);
		paperdistance = -FixedMul(tr_x, sinmul) + FixedMul(tr_y, cosmul);
		if (paperdistance < 0)
		{
			paperoffset = -paperoffset;
			paperdistance = -paperdistance;
		}
		centerangle = viewangle - interp.angle;

		tr_x += FixedMul(offset2, cosmul);
		tr_y += FixedMul(offset2, sinmul);
		tz2 = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);

		tx2 = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos);

		if (max(tz, tz2) < FixedMul(MINZ, this_scale)) // non-papersprite clipping is handled earlier
			return;

		// Needs partially clipped
		if (tz < FixedMul(MINZ, this_scale))
		{
			fixed_t div = FixedDiv(tz2-tz, FixedMul(MINZ, this_scale)-tz);
			tx += FixedDiv(tx2-tx, div);
			tz = FixedMul(MINZ, this_scale);
		}
		else if (tz2 < FixedMul(MINZ, this_scale))
		{
			fixed_t div = FixedDiv(tz-tz2, FixedMul(MINZ, this_scale)-tz2);
			tx2 += FixedDiv(tx-tx2, div);
			tz2 = FixedMul(MINZ, this_scale);
		}

		if (tx2 < -(FixedMul(tz2, fovtan)<<2) || tx > FixedMul(tz, fovtan)<<2) // too far off the side?
			return;

		yscale = FixedDiv(projectiony, tz);
		xscale = FixedDiv(projection, tz);

		x1 = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;

		// off the right side?
		if (x1 > viewwidth)
			return;

		yscale2 = FixedDiv(projectiony, tz2);
		xscale2 = FixedDiv(projection, tz2);

		x2 = (centerxfrac + FixedMul(tx2,xscale2))>>FRACBITS;

		// off the left side
		if (x2 < 0)
			return;

		if ((range = x2 - x1) <= 0)
			return;

		range++; // fencepost problem

		// Compatibility with MSVC - SSNTails
		scalestep = ((yscale2 - yscale) / range);
		if (!scalestep)
			scalestep = 1;

		xscale = FixedDiv(range<<FRACBITS, abs(offset2));

		// The following two are alternate sorting methods which might be more applicable in some circumstances. TODO - maybe enable via MF2?
		// sortscale = max(yscale, yscale2);
		// sortscale = min(yscale, yscale2);
	}
	else
	{
		scalestep = 0;
		yscale = sortscale;
		tx += offset;
		x1 = (centerxfrac + FixedMul(tx,xscale))>>FRACBITS;

		// off the right side?
		if (x1 > viewwidth)
			return;

		tx += offset2;
		x2 = ((centerxfrac + FixedMul(tx,xscale))>>FRACBITS); x2--;

		// off the left side
		if (x2 < 0)
			return;
	}

	// Adjust the sort scale if needed
	if (splat)
	{
		sort_z = (patch->height - patch->topoffset) * FRACUNIT;
		ang = (viewangle >> ANGLETOFINESHIFT);
		sort_x = FixedMul(FixedMul(FixedMul(spritexscale, this_scale), sort_z), FINECOSINE(ang));
		sort_y = FixedMul(FixedMul(FixedMul(spriteyscale, this_scale), sort_z), FINESINE(ang));

		tr_x = (interp.x + sort_x) - viewx;
		tr_y = (interp.y + sort_y) - viewy;
		sort_z = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);
		sortscale = FixedDiv(projectiony, sort_z);
	}

	if ((thing->flags2 & MF2_LINKDRAW) && thing->tracer) // toast 16/09/16 (SYMMETRY)
	{
		interpmobjstate_t tracer_interp = {0};

		thing = thing->tracer;

		if (! R_ThingVisible(thing))
			return;

		if (R_UsingFrameInterpolation() && !paused)
		{
			R_InterpolateMobjState(thing, rendertimefrac, &tracer_interp);
		}
		else
		{
			R_InterpolateMobjState(thing, FRACUNIT, &tracer_interp);
		}
		radius = tracer_interp.radius; // For drop shadows
		height = tracer_interp.height; // Ditto

		if (thing->type == MT_OVERLAY) // Handle overlays
			R_ThingOffsetOverlay(thing, &tracer_interp.x, &tracer_interp.y);
		tr_x = (tracer_interp.x + sort_x) - viewx;
		tr_y = (tracer_interp.y + sort_y) - viewy;
		tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);
		linkscale = FixedDiv(projectiony, tz);

		if (tz < FixedMul(MINZ, this_scale))
			return;

		if (sortscale < linkscale)
			dispoffset *= -1; // if it's physically behind, make sure it's ordered behind (if dispoffset > 0)

		//sortscale = linkscale; // now make sure it's linked
		// No need to do that, linkdraw already excludes it from regular sorting.

		cut |= SC_LINKDRAW;
	}
	else
	{
		linkscale = sortscale;
	}

	// Calculate the splat's sortscale
	if (splat)
	{
		tr_x = (interp.x - sort_x) - viewx;
		tr_y = (interp.y - sort_y) - viewy;
		sort_z = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin);
		sortsplat = FixedDiv(projectiony, sort_z);
		centerangle = interp.angle;
	}

	// PORTAL SPRITE CLIPPING
	if (portalrender && portalclipline)
	{
		if (x2 < portalclipstart || x1 >= portalclipend)
			return;

		if (P_PointOnLineSide(interp.x, interp.y, portalclipline) != 0)
			return;
	}

	INT32 blendmode;
	if (oldthing->frame & FF_BLENDMASK)
		blendmode = ((oldthing->frame & FF_BLENDMASK) >> FF_BLENDSHIFT) + 1;
	else
		blendmode = oldthing->blendmode;

	// Determine the translucency value.
	if (oldthing->flags2 & MF2_SHADOW || thing->flags2 & MF2_SHADOW) // actually only the player should use this (temporary invisibility)
		trans = tr_trans80; // because now the translucency is set through FF_TRANSMASK
	else if (oldthing->frame & FF_TRANSMASK)
	{
		trans = (oldthing->frame & FF_TRANSMASK) >> FF_TRANSSHIFT;
		if (!R_BlendLevelVisible(blendmode, trans))
			return;
	}
	else
		trans = 0;

	if ((oldthing->flags2 & MF2_LINKDRAW) && oldthing->tracer)
		trans = R_GetThingTransTable(oldthing->tracer->alpha, trans);
	else
		trans = R_GetThingTransTable(oldthing->alpha, trans);

	// Check if this sprite needs to be rendered like a shadow
	shadowdraw = (!!(thing->renderflags & RF_SHADOWDRAW) && !(papersprite || splat));
	shadoweffects = (thing->renderflags & RF_SHADOWEFFECTS);
	shadowskew = (shadowdraw && thing->standingslope);

	if (shadowdraw || shadoweffects)
	{
		fixed_t groundz = R_GetShadowZ(thing, NULL);
		boolean isflipped = (thing->eflags & MFE_VERTICALFLIP);

		if (shadoweffects)
		{
			mobj_t *caster = thing->target;
			interpmobjstate_t casterinterp = { 0 }; // MSVC compatibility - SSNTails

			if (R_UsingFrameInterpolation() && !paused)
			{
				R_InterpolateMobjState(caster, rendertimefrac, &casterinterp);
			}
			else
			{
				R_InterpolateMobjState(caster, FRACUNIT, &casterinterp);
			}

			if (caster && !P_MobjWasRemoved(caster))
			{
				fixed_t floordiff;

				if (abs(groundz-viewz)/tz > 4)
					return; // Prevent stretchy shadows and possible crashes

				floordiff = abs((isflipped ? casterinterp.height : 0) + casterinterp.z - groundz);
				trans += ((floordiff / (100*FRACUNIT)) + 3);
				shadowscale = FixedMul(FRACUNIT - floordiff/640, casterinterp.scale);
			}
			else
				trans += 3;

			if (trans >= NUMTRANSMAPS)
				return;

			trans--;
		}

		if (shadowdraw)
		{
			spritexscale = FixedMul(radius * 2, FixedMul(shadowscale, spritexscale));
			spriteyscale = FixedMul(radius * 2, FixedMul(shadowscale, spriteyscale));
			spriteyscale = FixedMul(spriteyscale, FixedDiv(abs(groundz - viewz), tz));
			spriteyscale = min(spriteyscale, spritexscale) / patch->height;
			spritexscale /= patch->width;
		}
		else
		{
			spritexscale = FixedMul(shadowscale, spritexscale);
			spriteyscale = FixedMul(shadowscale, spriteyscale);
		}

		if (shadowskew)
		{
			R_SkewShadowSprite(thing, thing->standingslope, groundz, patch->height, shadowscale, &spriteyscale, &sheartan);

			gzt = (isflipped ? (interp.z + height) : interp.z) + patch->height * spriteyscale / 2;
			gz = gzt - patch->height * spriteyscale;

			cut |= SC_SHEAR;
		}
	}

	if (!shadowskew)
	{
		//SoM: 3/17/2000: Disregard sprites that are out of view..
		if (vflip)
		{
			// When vertical flipped, draw sprites from the top down, at least as far as offsets are concerned.
			// sprite height - sprite topoffset is the proper inverse of the vertical offset, of course.
			// remember gz and gzt should be seperated by sprite height, not thing height - thing height can be shorter than the sprite itself sometimes!
			gz = interp.z + interp.height - FixedMul(spr_topoffset, FixedMul(spriteyscale, this_scale));
			gzt = gz + FixedMul(spr_height, FixedMul(spriteyscale, this_scale));
		}
		else
		{
			gzt = interp.z + FixedMul(spr_topoffset, FixedMul(spriteyscale, this_scale));
			gz = gzt - FixedMul(spr_height, FixedMul(spriteyscale, this_scale));
		}
	}

	if (thing->subsector->sector->cullheight)
	{
		if (R_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, viewz, gz, gzt))
			return;
	}

	if (thing->subsector->sector->numlights)
	{
		light = P_GetSectorLightNumAt(thing->subsector->sector, interp.x, interp.y, splat ? gz : gzt);

		INT32 lightnum = (*thing->subsector->sector->lightlist[light].lightlevel >> LIGHTSEGSHIFT);
		if (lightnum < 0)
			spritelights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			spritelights = scalelight[LIGHTLEVELS-1];
		else
			spritelights = scalelight[lightnum];
	}

	heightsec = thing->subsector->sector->heightsec;
	if (viewplayer->mo && viewplayer->mo->subsector)
		phs = viewplayer->mo->subsector->sector->heightsec;
	else
		phs = -1;

	if (heightsec != -1 && phs != -1) // only clip things which are in special sectors
	{
		fixed_t top = gzt;
		fixed_t bottom = interp.z;

		if (splat)
			top = bottom;

		if (viewz < sectors[phs].floorheight ?
		bottom >= sectors[heightsec].floorheight :
		top < sectors[heightsec].floorheight)
			return;
		if (viewz > sectors[phs].ceilingheight ?
		top < sectors[heightsec].ceilingheight && viewz >= sectors[heightsec].ceilingheight :
		bottom >= sectors[heightsec].ceilingheight)
			return;
	}

	// store information in a vissprite
	vis = R_NewVisSprite();
	vis->renderflags = thing->renderflags;
	vis->rotateflags = sprframe->rotate;
	vis->heightsec = heightsec; //SoM: 3/17/2000
	vis->mobjflags = thing->flags;
	vis->sortscale = sortscale;
	vis->sortsplat = sortsplat;
	vis->linkscale = linkscale;
	vis->dispoffset = dispoffset; // Monster Iestyn: 23/11/15
	vis->gx = interp.x;
	vis->gy = interp.y;
	vis->gz = gz;
	vis->gzt = gzt;
	vis->thingheight = height;
	vis->pz = interp.z;
	vis->pzt = vis->pz + vis->thingheight;
	vis->texturemid = FixedDiv(gzt - viewz, spriteyscale);
	vis->scalestep = scalestep;
	vis->paperoffset = paperoffset;
	vis->paperdistance = paperdistance;
	vis->centerangle = centerangle;
	vis->shear.tan = sheartan;
	vis->shear.offset = 0;
	vis->viewpoint.x = viewx;
	vis->viewpoint.y = viewy;
	vis->viewpoint.z = viewz;
	vis->viewpoint.angle = viewangle;

	vis->mobj = thing; // Easy access! Tails 06-07-2002
	if ((oldthing->flags2 & MF2_LINKDRAW) && oldthing->tracer && oldthing->color == SKINCOLOR_NONE)
		vis->color = oldthing->tracer->color;
	else
		vis->color = oldthing->color;

	if ((oldthing->flags2 & MF2_LINKDRAW) && oldthing->tracer && oldthing->translation == 0)
		vis->translation = oldthing->tracer->translation;
	else
		vis->translation = oldthing->translation;

	vis->x1 = x1 < portalclipstart ? portalclipstart : x1;
	vis->x2 = x2 >= portalclipend ? portalclipend-1 : x2;

	vis->sector = thing->subsector->sector;

	// Using linkscale here improves cut detection for LINKDRAW.
	vis->szt = (INT16)((centeryfrac - FixedMul(vis->gzt - viewz, linkscale))>>FRACBITS);
	vis->sz = (INT16)((centeryfrac - FixedMul(vis->gz - viewz, linkscale))>>FRACBITS);
	vis->cut = cut;

	if (thing->subsector->sector->numlights)
		vis->extra_colormap = *thing->subsector->sector->lightlist[light].extra_colormap;
	else
		vis->extra_colormap = thing->subsector->sector->extra_colormap;

	vis->xscale = FixedMul(spritexscale, xscale); //SoM: 4/17/2000
	vis->scale = FixedMul(spriteyscale, yscale); //<<detailshift;
	vis->thingscale = interp.scale;

	vis->spritexscale = spritexscale;
	vis->spriteyscale = spriteyscale;
	vis->spritexoffset = spr_offset;
	vis->spriteyoffset = spr_topoffset;

	if (shadowdraw || shadoweffects)
	{
		iscale = (patch->width<<FRACBITS)/(x2-x1+1); // fuck it
		x1 += (x2-x1)/2; // reusing x1 variable
		vis->shear.offset = vis->x1-x1;
	}
	else
		iscale = FixedDiv(FRACUNIT, vis->xscale);

	vis->shadowscale = shadowscale;

	if (flip)
	{
		vis->startfrac = spr_width-1;
		vis->xiscale = -iscale;
	}
	else
	{
		vis->startfrac = 0;
		vis->xiscale = iscale;
	}

	if (vis->x1 > x1)
	{
		vis->startfrac += FixedDiv(vis->xiscale, this_scale) * (vis->x1 - x1);
		vis->scale += FixedMul(scalestep, spriteyscale) * (vis->x1 - x1);
	}

	if ((blendmode != AST_COPY) && cv_translucency.value)
		vis->transmap = R_GetBlendTable(blendmode, trans);
	else
		vis->transmap = NULL;

	if (R_ThingIsFullBright(oldthing) || oldthing->flags2 & MF2_SHADOW || thing->flags2 & MF2_SHADOW)
		vis->cut |= SC_FULLBRIGHT;
	else if (R_ThingIsSemiBright(oldthing))
		vis->cut |= SC_SEMIBRIGHT;
	else if (R_ThingIsFullDark(oldthing))
		vis->cut |= SC_FULLDARK;

	//
	// determine the colormap (lightlevel & special effects)
	//
	if (vis->cut & SC_FULLBRIGHT
		&& (!vis->extra_colormap || !(vis->extra_colormap->flags & CMF_FADEFULLBRIGHTSPRITES)))
	{
		// full bright: goggles
		vis->colormap = colormaps;
	}
	else if (vis->cut & SC_FULLDARK)
		vis->colormap = scalelight[0][0];
	else
	{
		// diminished light
		lindex = FixedMul(xscale, LIGHTRESOLUTIONFIX)>>(LIGHTSCALESHIFT);

		if (lindex >= MAXLIGHTSCALE)
			lindex = MAXLIGHTSCALE-1;

		if (vis->cut & SC_SEMIBRIGHT)
			lindex = (MAXLIGHTSCALE/2) + (lindex >> 1);

		vis->colormap = spritelights[lindex];
	}

	if (vflip)
		vis->cut |= SC_VFLIP;
	if (splat)
		vis->cut |= SC_SPLAT; // I like ya cut g

	vis->patch = patch;

	if (thing->subsector->sector->numlights && !(shadowdraw || splat))
		R_SplitSprite(vis);

	if (oldthing->shadowscale && cv_shadow.value)
		R_ProjectDropShadow(oldthing, vis, oldthing->shadowscale, basetx, basetz);

	R_ProjectBoundingBox(oldthing, vis);

	// Debug
	++objectsdrawn;
}

static void R_ProjectPrecipitationSprite(precipmobj_t *thing)
{
	fixed_t tr_x, tr_y;
	fixed_t tx, tz;
	fixed_t xscale, yscale; //added : 02-02-98 : aaargll..if I were a math-guy!!!

	INT32 x1, x2;

	spritedef_t *sprdef;
	spriteframe_t *sprframe;
	size_t lump;

	vissprite_t *vis;

	fixed_t iscale;

	//SoM: 3/17/2000
	fixed_t gz, gzt;

	// uncapped/interpolation
	interpmobjstate_t interp = {0};

	// do interpolation
	if (R_UsingFrameInterpolation() && !paused)
	{
		R_InterpolatePrecipMobjState(thing, rendertimefrac, &interp);
	}
	else
	{
		R_InterpolatePrecipMobjState(thing, FRACUNIT, &interp);
	}

	// transform the origin point
	tr_x = interp.x - viewx;
	tr_y = interp.y - viewy;

	tz = FixedMul(tr_x, viewcos) + FixedMul(tr_y, viewsin); // near/far distance

	// thing is behind view plane?
	if (tz < MINZ)
		return;

	tx = FixedMul(tr_x, viewsin) - FixedMul(tr_y, viewcos); // sideways distance

	// too far off the side?
	if (abs(tx) > FixedMul(tz, fovtan)<<2)
		return;

	// aspect ratio stuff :
	xscale = FixedDiv(projection, tz);
	yscale = FixedDiv(projectiony, tz);

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= numsprites)
		I_Error("R_ProjectPrecipitationSprite: invalid sprite number %d ",
			thing->sprite);
#endif

	sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
	if ((UINT8)(thing->frame&FF_FRAMEMASK) >= sprdef->numframes)
		I_Error("R_ProjectPrecipitationSprite: invalid sprite frame %d : %d for %s",
			thing->sprite, thing->frame, sprnames[thing->sprite]);
#endif

	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

#ifdef PARANOIA
	if (!sprframe)
		I_Error("R_ProjectPrecipitationSprite: sprframes NULL for sprite %d\n", thing->sprite);
#endif

	// use single rotation for all views
	lump = sprframe->lumpid[0];     //Fab: see note above

	// calculate edges of the shape
	tx -= spritecachedinfo[lump].offset;
	x1 = (centerxfrac + FixedMul (tx,xscale)) >>FRACBITS;

	// off the right side?
	if (x1 > viewwidth)
		return;

	tx += spritecachedinfo[lump].width;
	x2 = ((centerxfrac + FixedMul (tx,xscale)) >>FRACBITS) - 1;

	// off the left side
	if (x2 < 0)
		return;

	// PORTAL SPRITE CLIPPING
	if (portalrender && portalclipline)
	{
		if (x2 < portalclipstart || x1 >= portalclipend)
			return;

		if (P_PointOnLineSide(interp.x, interp.y, portalclipline) != 0)
			return;
	}


	//SoM: 3/17/2000: Disregard sprites that are out of view..
	gzt = interp.z + spritecachedinfo[lump].topoffset;
	gz = gzt - spritecachedinfo[lump].height;

	if (thing->subsector->sector->cullheight)
	{
		if (R_DoCulling(thing->subsector->sector->cullheight, viewsector->cullheight, viewz, gz, gzt))
			goto weatherthink;
	}

	// store information in a vissprite
	vis = R_NewVisSprite();
	vis->scale = vis->sortscale = yscale; //<<detailshift;
	vis->dispoffset = 0; // Monster Iestyn: 23/11/15
	vis->gx = interp.x;
	vis->gy = interp.y;
	vis->gz = gz;
	vis->gzt = gzt;
	vis->thingheight = 4*FRACUNIT;
	vis->pz = interp.z;
	vis->pzt = vis->pz + vis->thingheight;
	vis->texturemid = vis->gzt - viewz;
	vis->scalestep = 0;
	vis->paperdistance = 0;
	vis->shear.tan = 0;
	vis->shear.offset = 0;

	vis->x1 = x1 < portalclipstart ? portalclipstart : x1;
	vis->x2 = x2 >= portalclipend ? portalclipend-1 : x2;

	vis->xscale = xscale; //SoM: 4/17/2000
	vis->sector = thing->subsector->sector;
	vis->szt = (INT16)((centeryfrac - FixedMul(vis->gzt - viewz, yscale))>>FRACBITS);
	vis->sz = (INT16)((centeryfrac - FixedMul(vis->gz - viewz, yscale))>>FRACBITS);

	iscale = FixedDiv(FRACUNIT, xscale);

	vis->startfrac = 0;
	vis->xiscale = iscale;

	if (vis->x1 > x1)
		vis->startfrac += vis->xiscale*(vis->x1-x1);

	//Fab: lumppat is the lump number of the patch to use, this is different
	//     than lumpid for sprites-in-pwad : the graphics are patched
	vis->patch = W_CachePatchNum(sprframe->lumppat[0], PU_SPRITE);

	// specific translucency
	if (thing->frame & FF_TRANSMASK)
		vis->transmap = R_GetTranslucencyTable((thing->frame & FF_TRANSMASK) >> FF_TRANSSHIFT);
	else
		vis->transmap = NULL;

	vis->mobj = (mobj_t *)thing;
	vis->mobjflags = 0;
	vis->cut = SC_PRECIP;
	vis->extra_colormap = thing->subsector->sector->extra_colormap;
	vis->heightsec = thing->subsector->sector->heightsec;
	vis->color = SKINCOLOR_NONE;
	vis->translation = 0;

	// Fullbright
	vis->colormap = colormaps;

weatherthink:
	// okay... this is a hack, but weather isn't networked, so it should be ok
	if (!(thing->precipflags & PCF_THUNK))
	{
		if (thing->precipflags & PCF_RAIN)
			P_RainThinker(thing);
		else
			P_SnowThinker(thing);
		thing->precipflags |= PCF_THUNK;
	}
}

// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites(sector_t *sec, INT32 lightlevel)
{
	mobj_t *thing;
	precipmobj_t *precipthing; // Tails 08-25-2002
	INT32 lightnum;
	fixed_t limit_dist, hoop_limit_dist;

	if (rendermode != render_soft)
		return;

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if (sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	if (!sec->numlights)
	{
		if (sec->heightsec == -1) lightlevel = sec->lightlevel;

		lightnum = (lightlevel >> LIGHTSEGSHIFT);

		if (lightnum < 0)
			spritelights = scalelight[0];
		else if (lightnum >= LIGHTLEVELS)
			spritelights = scalelight[LIGHTLEVELS-1];
		else
			spritelights = scalelight[lightnum];
	}

	// Handle all things in sector.
	// If a limit exists, handle things a tiny bit different.
	limit_dist = (fixed_t)(cv_drawdist.value) << FRACBITS;
	hoop_limit_dist = (fixed_t)(cv_drawdist_nights.value) << FRACBITS;
	for (thing = sec->thinglist; thing; thing = thing->snext)
	{
		if (R_ThingWithinDist(thing, limit_dist, hoop_limit_dist))
		{
			const INT32 oldobjectsdrawn = objectsdrawn;

			if (R_ThingVisible(thing))
			{
				R_ProjectSprite(thing);
			}

			// I'm so smart :^)
			if (objectsdrawn == oldobjectsdrawn)
			{
				/*
				Object is invisible OR is off screen but
				render its bbox even if the latter because
				radius could be bigger than sprite.
				*/
				R_ProjectBoundingBox(thing, NULL);
			}
		}
	}

	// no, no infinite draw distance for precipitation. this option at zero is supposed to turn it off
	if ((limit_dist = (fixed_t)cv_drawdist_precip.value << FRACBITS))
	{
		for (precipthing = sec->preciplist; precipthing; precipthing = precipthing->snext)
		{
			if (R_PrecipThingVisible(precipthing, limit_dist))
				R_ProjectPrecipitationSprite(precipthing);
		}
	}
}

static boolean R_SortVisSpriteFunc(vissprite_t *ds, fixed_t bestscale, INT32 bestdispoffset)
{
	if (ds->sortscale < bestscale)
	{
		return true;
	}
	// order visprites of same scale by dispoffset, smallest first
	else if (ds->sortscale == bestscale && ds->dispoffset < bestdispoffset)
	{
		return true;
	}

	return false;
}

//
// R_SortVisSprites
//
static void R_SortVisSprites(vissprite_t* vsprsortedhead, UINT32 start, UINT32 end)
{
	UINT32       i, linkedvissprites = 0;
	vissprite_t *ds, *dsprev, *dsnext, *dsfirst;
	vissprite_t *best = NULL;
	vissprite_t  unsorted;
	fixed_t      bestscale;
	INT32        bestdispoffset;

	unsorted.next = unsorted.prev = &unsorted;

	dsfirst = R_GetVisSprite(start);

	// The first's prev and last's next will be set to
	// nonsense, but are fixed in a moment
	for (i = start, dsnext = dsfirst, ds = NULL; i < end; i++)
	{
		dsprev = ds;
		ds = dsnext;
		if (i < end - 1) dsnext = R_GetVisSprite(i + 1);

		ds->next = dsnext;
		ds->prev = dsprev;
		ds->linkdraw = NULL;
	}

	// Fix first and last. ds still points to the last one after the loop
	dsfirst->prev = &unsorted;
	unsorted.next = dsfirst;
	if (ds)
	{
		ds->next = &unsorted;
		ds->linkdraw = NULL;
	}
	unsorted.prev = ds;

	// bundle linkdraw
	for (ds = unsorted.prev; ds != &unsorted; ds = ds->prev)
	{
		// Remove this sprite if it was determined to not be visible
		if (ds->cut & SC_NOTVISIBLE)
		{
			ds->next->prev = ds->prev;
			ds->prev->next = ds->next;
			continue;
		}

		if (!(ds->cut & SC_LINKDRAW))
			continue;

		if (ds->cut & SC_SHADOW)
			continue;

		// reuse dsfirst...
		for (dsfirst = unsorted.prev; dsfirst != &unsorted; dsfirst = dsfirst->prev)
		{
			// don't connect if it's also a link
			if (dsfirst->cut & SC_LINKDRAW)
				continue;

			// don't connect to your shadow!
			if (dsfirst->cut & SC_SHADOW)
				continue;

			// don't connect to your bounding box!
			if (dsfirst->cut & SC_BBOX)
				continue;

			// don't connect if it's not the tracer
			if (dsfirst->mobj != ds->mobj)
				continue;

			// don't connect if the tracer's top is cut off, but lower than the link's top
			if ((dsfirst->cut & SC_TOP) && dsfirst->szt > ds->szt)
				continue;

			// don't connect if the tracer's bottom is cut off, but higher than the link's bottom
			if ((dsfirst->cut & SC_BOTTOM) && dsfirst->sz < ds->sz)
				continue;

			// If the object isn't visible, then the bounding box isn't either
			if (ds->cut & SC_BBOX && dsfirst->cut & SC_NOTVISIBLE)
				ds->cut |= SC_NOTVISIBLE;

			break;
		}

		// remove from chain
		ds->next->prev = ds->prev;
		ds->prev->next = ds->next;

		if (ds->cut & SC_NOTVISIBLE)
			continue;

		linkedvissprites++;

		if (dsfirst != &unsorted)
		{
			if (!(ds->cut & SC_FULLBRIGHT))
				ds->colormap = dsfirst->colormap;
			ds->extra_colormap = dsfirst->extra_colormap;

			// reusing dsnext...
			dsnext = dsfirst->linkdraw;

			if (dsnext == NULL || R_SortVisSpriteFunc(ds, dsnext->sortscale, dsnext->dispoffset) == true)
			{
				ds->next = dsnext;
				dsfirst->linkdraw = ds;
			}
			else
			{
				for (; dsnext->next != NULL; dsnext = dsnext->next)
				{
					if (R_SortVisSpriteFunc(ds, dsnext->next->sortscale, dsnext->next->dispoffset) == true)
					{
						break;
					}
				}

				ds->next = dsnext->next;
				dsnext->next = ds;
			}
		}
	}

	// pull the vissprites out by scale
	vsprsortedhead->next = vsprsortedhead->prev = vsprsortedhead;
	for (i = start; i < end-linkedvissprites; i++)
	{
		bestscale = bestdispoffset = INT32_MAX;
		for (ds = unsorted.next; ds != &unsorted; ds = ds->next)
		{
#ifdef PARANOIA
			if (ds->cut & SC_LINKDRAW)
				I_Error("R_SortVisSprites: no link or discardal made for linkdraw!");
#endif

			if (R_SortVisSpriteFunc(ds, bestscale, bestdispoffset) == true)
			{
				bestscale = ds->sortscale;
				bestdispoffset = ds->dispoffset;
				best = ds;
			}
		}
		if (best)
		{
			best->next->prev = best->prev;
			best->prev->next = best->next;
			best->next = vsprsortedhead;
			best->prev = vsprsortedhead->prev;
			vsprsortedhead->prev->next = best;
			vsprsortedhead->prev = best;
		}
	}
}

//
// R_CreateDrawNodes
// Creates and sorts a list of drawnodes for the scene being rendered.
static drawnode_t *R_CreateDrawNode(drawnode_t *link);

static drawnode_t nodebankhead;

static void R_CreateDrawNodes(maskcount_t* mask, drawnode_t* head, boolean tempskip)
{
	drawnode_t *entry;
	drawseg_t *ds;
	INT32 i, p, best, x1, x2;
	fixed_t bestdelta, delta;
	vissprite_t *rover;
	static vissprite_t vsprsortedhead;
	drawnode_t *r2;
	visplane_t *plane;
	INT32 sintersect;
	fixed_t scale = 0;

	// Add the 3D floors, thicksides, and masked textures...
	for (ds = drawsegs + mask->drawsegs[1]; ds-- > drawsegs + mask->drawsegs[0];)
	{
		if (ds->numthicksides && r_renderwalls)
		{
			for (i = 0; i < ds->numthicksides; i++)
			{
				entry = R_CreateDrawNode(head);
				entry->thickseg = ds;
				entry->ffloor = ds->thicksides[i];
			}
		}
		// Check for a polyobject plane, but only if this is a front line
		if (ds->curline->polyseg && ds->curline->polyseg->visplane && !ds->curline->side) {
			plane = ds->curline->polyseg->visplane;
			R_PlaneBounds(plane);

			if (plane->low < 0 || plane->high > vid.height || plane->high > plane->low)
				;
			else {
				// Put it in!
				entry = R_CreateDrawNode(head);
				if (r_renderwalls)
					entry->seg = ds;
				if (r_renderfloors)
					entry->plane = plane;
			}
			ds->curline->polyseg->visplane = NULL;
		}
		if (ds->maskedtexturecol && r_renderwalls)
		{
			entry = R_CreateDrawNode(head);
			entry->seg = ds;
		}
		if (ds->numffloorplanes && r_renderfloors)
		{
			for (i = 0; i < ds->numffloorplanes; i++)
			{
				best = -1;
				bestdelta = 0;
				for (p = 0; p < ds->numffloorplanes; p++)
				{
					if (!ds->ffloorplanes[p])
						continue;
					plane = ds->ffloorplanes[p];
					R_PlaneBounds(plane);

					if (plane->low < 0 || plane->high > vid.height || plane->high > plane->low || plane->polyobj)
					{
						ds->ffloorplanes[p] = NULL;
						continue;
					}

					delta = abs(plane->height - viewz);
					if (delta > bestdelta)
					{
						best = p;
						bestdelta = delta;
					}
				}
				if (best != -1)
				{
					entry = R_CreateDrawNode(head);
					entry->plane = ds->ffloorplanes[best];
					entry->seg = ds;
					ds->ffloorplanes[best] = NULL;
				}
				else
					break;
			}
		}
	}

	if (tempskip)
		return;

	// find all the remaining polyobject planes and add them on the end of the list
	// probably this is a terrible idea if we wanted them to be sorted properly
	// but it works getting them in for now
	for (i = 0; i < numPolyObjects; i++)
	{
		if (!PolyObjects[i].visplane)
			continue;
		plane = PolyObjects[i].visplane;
		R_PlaneBounds(plane);

		if (plane->low < 0 || plane->high > vid.height || plane->high > plane->low)
		{
			PolyObjects[i].visplane = NULL;
			continue;
		}
		entry = R_CreateDrawNode(head);
		entry->plane = plane;
		// note: no seg is set, for what should be obvious reasons
		PolyObjects[i].visplane = NULL;
	}

	// No vissprites in this mask?
	if (mask->vissprites[1] - mask->vissprites[0] == 0)
		return;

	R_SortVisSprites(&vsprsortedhead, mask->vissprites[0], mask->vissprites[1]);

	for (rover = vsprsortedhead.prev; rover != &vsprsortedhead; rover = rover->prev)
	{
		if (rover->szt > vid.height || rover->sz < 0)
			continue;

		sintersect = (rover->x1 + rover->x2) / 2;

		for (r2 = head->next; r2 != head; r2 = r2->next)
		{
			if (r2->plane)
			{
				fixed_t planeobjectz, planecameraz;
				if (r2->plane->minx > rover->x2 || r2->plane->maxx < rover->x1)
					continue;
				if (rover->szt > r2->plane->low || rover->sz < r2->plane->high)
					continue;

				// Effective height may be different for each comparison in the case of slopes
				planeobjectz = P_GetZAt(r2->plane->slope, rover->gx, rover->gy, r2->plane->height);
				planecameraz = P_GetZAt(r2->plane->slope,     viewx,     viewy, r2->plane->height);

				if (rover->mobjflags & MF_NOCLIPHEIGHT)
				{
					//Objects with NOCLIPHEIGHT can appear halfway in.
					if (planecameraz < viewz && rover->pz+(rover->thingheight/2) >= planeobjectz)
						continue;
					if (planecameraz > viewz && rover->pzt-(rover->thingheight/2) <= planeobjectz)
						continue;
				}
				else
				{
					if (planecameraz < viewz && rover->pz >= planeobjectz)
						continue;
					if (planecameraz > viewz && rover->pzt <= planeobjectz)
						continue;
				}

				// SoM: NOTE: Because a visplane's shape and scale is not directly
				// bound to any single linedef, a simple poll of it's frontscale is
				// not adequate. We must check the entire frontscale array for any
				// part that is in front of the sprite.

				x1 = rover->x1;
				x2 = rover->x2;
				if (x1 < r2->plane->minx) x1 = r2->plane->minx;
				if (x2 > r2->plane->maxx) x2 = r2->plane->maxx;

				if (r2->seg) // if no seg set, assume the whole thing is in front or something stupid
				{
					for (i = x1; i <= x2; i++)
					{
						if (r2->seg->frontscale[i] > rover->sortscale)
							break;
					}
					if (i > x2)
						continue;
				}

				entry = R_CreateDrawNode(NULL);
				(entry->prev = r2->prev)->next = entry;
				(entry->next = r2)->prev = entry;
				entry->sprite = rover;
				break;
			}
			else if (r2->thickseg)
			{
				fixed_t topplaneobjectz, topplanecameraz, botplaneobjectz, botplanecameraz;
				if (rover->x1 > r2->thickseg->x2 || rover->x2 < r2->thickseg->x1)
					continue;

				scale = r2->thickseg->scale1 > r2->thickseg->scale2 ? r2->thickseg->scale1 : r2->thickseg->scale2;
				if (scale <= rover->sortscale)
					continue;
				scale = r2->thickseg->scale1 + (r2->thickseg->scalestep * (sintersect - r2->thickseg->x1));
				if (scale <= rover->sortscale)
					continue;

				topplaneobjectz = P_GetFFloorTopZAt   (r2->ffloor, rover->gx, rover->gy);
				topplanecameraz = P_GetFFloorTopZAt   (r2->ffloor,     viewx,     viewy);
				botplaneobjectz = P_GetFFloorBottomZAt(r2->ffloor, rover->gx, rover->gy);
				botplanecameraz = P_GetFFloorBottomZAt(r2->ffloor,     viewx,     viewy);

				if ((topplanecameraz > viewz && botplanecameraz < viewz) ||
				    (topplanecameraz < viewz && rover->gzt < topplaneobjectz) ||
				    (botplanecameraz > viewz && rover->gz > botplaneobjectz))
				{
					entry = R_CreateDrawNode(NULL);
					(entry->prev = r2->prev)->next = entry;
					(entry->next = r2)->prev = entry;
					entry->sprite = rover;
					break;
				}
			}
			else if (r2->seg)
			{
				if (rover->x1 > r2->seg->x2 || rover->x2 < r2->seg->x1)
					continue;

				scale = r2->seg->scale1 > r2->seg->scale2 ? r2->seg->scale1 : r2->seg->scale2;
				if (scale <= rover->sortscale)
					continue;
				scale = r2->seg->scale1 + (r2->seg->scalestep * (sintersect - r2->seg->x1));

				if (rover->sortscale < scale)
				{
					entry = R_CreateDrawNode(NULL);
					(entry->prev = r2->prev)->next = entry;
					(entry->next = r2)->prev = entry;
					entry->sprite = rover;
					break;
				}
			}
			else if (r2->sprite)
			{
				boolean infront = (r2->sprite->sortscale > rover->sortscale
								|| (r2->sprite->sortscale == rover->sortscale && r2->sprite->dispoffset > rover->dispoffset));

				if (rover->cut & SC_SPLAT || r2->sprite->cut & SC_SPLAT)
				{
					fixed_t scale1 = (rover->cut & SC_SPLAT ? rover->sortsplat : rover->sortscale);
					fixed_t scale2 = (r2->sprite->cut & SC_SPLAT ? r2->sprite->sortsplat : r2->sprite->sortscale);
					boolean behind = (scale2 > scale1 || (scale2 == scale1 && r2->sprite->dispoffset > rover->dispoffset));

					if (!behind)
					{
						fixed_t z1 = 0, z2 = 0;

						if (rover->mobj->z - viewz > 0)
						{
							z1 = rover->pz;
							z2 = r2->sprite->pz;
						}
						else
						{
							z1 = r2->sprite->pz;
							z2 = rover->pz;
						}

						z1 -= viewz;
						z2 -= viewz;

						infront = (z1 >= z2);
					}
				}
				else
				{
					if (r2->sprite->x1 > rover->x2 || r2->sprite->x2 < rover->x1)
						continue;
					if (r2->sprite->szt > rover->sz || r2->sprite->sz < rover->szt)
						continue;
				}

				if (infront)
				{
					entry = R_CreateDrawNode(NULL);
					(entry->prev = r2->prev)->next = entry;
					(entry->next = r2)->prev = entry;
					entry->sprite = rover;
					break;
				}
			}
		}
		if (r2 == head)
		{
			entry = R_CreateDrawNode(head);
			entry->sprite = rover;
		}
	}
}

static drawnode_t *R_CreateDrawNode(drawnode_t *link)
{
	drawnode_t *node = nodebankhead.next;

	if (node == &nodebankhead)
	{
		node = malloc(sizeof (*node));
		if (!node)
			I_Error("No more free memory to CreateDrawNode");
	}
	else
		(nodebankhead.next = node->next)->prev = &nodebankhead;

	if (link)
	{
		node->next = link;
		node->prev = link->prev;
		link->prev->next = node;
		link->prev = node;
	}

	node->plane = NULL;
	node->seg = NULL;
	node->thickseg = NULL;
	node->ffloor = NULL;
	node->sprite = NULL;

	ps_numdrawnodes.value.i++;
	return node;
}

static void R_DoneWithNode(drawnode_t *node)
{
	(node->next->prev = node->prev)->next = node->next;
	(node->next = nodebankhead.next)->prev = node;
	(node->prev = &nodebankhead)->next = node;
}

static void R_ClearDrawNodes(drawnode_t* head)
{
	drawnode_t *rover;
	drawnode_t *next;

	for (rover = head->next; rover != head;)
	{
		next = rover->next;
		R_DoneWithNode(rover);
		rover = next;
	}

	head->next = head->prev = head;
}

void R_InitDrawNodes(void)
{
	nodebankhead.next = nodebankhead.prev = &nodebankhead;
}

//
// R_DrawSprite
//
static void R_DrawSprite(vissprite_t *spr)
{
	mfloorclip = spr->clipbot;
	mceilingclip = spr->cliptop;

	if (spr->cut & SC_BBOX)
		R_DrawThingBoundingBox(spr);
	else if (spr->cut & SC_SPLAT)
		R_DrawFloorSplat(spr);
	else
		R_DrawVisSprite(spr);
}

// Special drawer for precipitation sprites Tails 08-18-2002
static void R_DrawPrecipitationSprite(vissprite_t *spr)
{
	mfloorclip = spr->clipbot;
	mceilingclip = spr->cliptop;
	R_DrawPrecipitationVisSprite(spr);
}

//SoM: 3/17/2000: Clip sprites in water.
static void R_HeightSecClip(vissprite_t *spr, INT32 x1, INT32 x2)
{
	fixed_t mh, h;
	INT32 x, phs;

	if (spr->heightsec == -1)
		return;

	if (spr->cut & (SC_SPLAT | SC_SHADOW) || spr->renderflags & RF_SHADOWDRAW)
		return;

	phs = viewplayer->mo->subsector->sector->heightsec;

	if ((mh = sectors[spr->heightsec].floorheight) > spr->gz &&
		(h = centeryfrac - FixedMul(mh -= viewz, spr->sortscale)) >= 0 &&
		(h >>= FRACBITS) < viewheight)
	{
		if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
		{                          // clip bottom
			for (x = x1; x <= x2; x++)
				if (spr->clipbot[x] == -2 || h < spr->clipbot[x])
					spr->clipbot[x] = (INT16)h;
		}
		else						// clip top
		{
			for (x = x1; x <= x2; x++)
				if (spr->cliptop[x] == -2 || h > spr->cliptop[x])
					spr->cliptop[x] = (INT16)h;
		}
	}

	if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt &&
		(h = centeryfrac - FixedMul(mh-viewz, spr->sortscale)) >= 0 &&
		(h >>= FRACBITS) < viewheight)
	{
		if (phs != -1 && viewz >= sectors[phs].ceilingheight)
		{                         // clip bottom
			for (x = x1; x <= x2; x++)
				if (spr->clipbot[x] == -2 || h < spr->clipbot[x])
					spr->clipbot[x] = (INT16)h;
		}
		else                       // clip top
		{
			for (x = x1; x <= x2; x++)
				if (spr->cliptop[x] == -2 || h > spr->cliptop[x])
					spr->cliptop[x] = (INT16)h;
		}
	}
}

static boolean R_CheckSpriteVisible(vissprite_t *spr, INT32 x1, INT32 x2)
{
	INT16 sz = spr->sz;
	INT16 szt = spr->szt;

	fixed_t texturemid = 0, yscale = 0, scalestep = spr->scalestep; // "= 0" pleases the compiler
	INT32 height = 0;

	if (scalestep)
	{
		height = spr->patch->height;
		yscale = spr->scale;
		scalestep = FixedMul(scalestep, spr->spriteyscale);

		if (spr->thingscale != FRACUNIT)
			texturemid = FixedDiv(spr->texturemid, max(spr->thingscale, 1));
		else
			texturemid = spr->texturemid;
	}

	for (INT32 x = x1; x <= x2; x++)
	{
		if (scalestep)
		{
			fixed_t top = centeryfrac - FixedMul(texturemid, yscale);
			fixed_t bottom = top + (height * yscale);
			szt = (INT16)(top >> FRACBITS);
			sz = (INT16)(bottom >> FRACBITS);
			yscale += scalestep;
		}

		if (spr->cliptop[x] < spr->clipbot[x] && sz > spr->cliptop[x] && szt < spr->clipbot[x])
			return true;
	}

	return false;
}

// R_ClipVisSprite
// Clips vissprites without drawing, so that portals can work. -Red
static void R_ClipVisSprite(vissprite_t *spr, INT32 x1, INT32 x2, portal_t* portal)
{
	drawseg_t *ds;
	INT32		x;
	INT32		r1;
	INT32		r2;
	fixed_t		scale;
	fixed_t		lowscale;
	INT32		silhouette;

	for (x = x1; x <= x2; x++)
		spr->clipbot[x] = spr->cliptop[x] = -2;

	// Scan drawsegs from end to start for obscuring segs.
	// The first drawseg that has a greater scale
	//  is the clip seg.
	//SoM: 4/8/2000:
	// Pointer check was originally nonportable
	// and buggy, by going past LEFT end of array:

	// e6y: optimization
	if (drawsegs_xrange_size)
	{
		const drawseg_xrange_item_t *last = &drawsegs_xrange[drawsegs_xrange_count - 1];
		drawseg_xrange_item_t *curr = &drawsegs_xrange[-1];

		while (++curr <= last)
		{
			// determine if the drawseg obscures the sprite
			if (curr->x1 > x2 || curr->x2 < x1)
			{
				// does not cover sprite
				continue;
			}

			ds = curr->user;

			if (ds->portalpass > 0 && ds->portalpass <= portalrender)
				continue; // is a portal

			if (ds->scale1 > ds->scale2)
			{
				lowscale = ds->scale2;
				scale = ds->scale1;
			}
			else
			{
				lowscale = ds->scale1;
				scale = ds->scale2;
			}

			if (scale < spr->sortscale ||
				(lowscale < spr->sortscale &&
				 !R_PointOnSegSide (spr->gx, spr->gy, ds->curline)))
			{
				// seg is behind sprite
				continue;
			}

			r1 = ds->x1 < x1 ? x1 : ds->x1;
			r2 = ds->x2 > x2 ? x2 : ds->x2;

			// clip this piece of the sprite
			silhouette = ds->silhouette;

			if (spr->gz >= ds->bsilheight)
				silhouette &= ~SIL_BOTTOM;

			if (spr->gzt <= ds->tsilheight)
				silhouette &= ~SIL_TOP;

			if (silhouette == SIL_BOTTOM)
			{
				// bottom sil
				for (x = r1; x <= r2; x++)
					if (spr->clipbot[x] == -2)
						spr->clipbot[x] = ds->sprbottomclip[x];
			}
			else if (silhouette == SIL_TOP)
			{
				// top sil
				for (x = r1; x <= r2; x++)
					if (spr->cliptop[x] == -2)
						spr->cliptop[x] = ds->sprtopclip[x];
			}
			else if (silhouette == (SIL_TOP|SIL_BOTTOM))
			{
				// both
				for (x = r1; x <= r2; x++)
				{
					if (spr->clipbot[x] == -2)
						spr->clipbot[x] = ds->sprbottomclip[x];
					if (spr->cliptop[x] == -2)
						spr->cliptop[x] = ds->sprtopclip[x];
				}
			}
		}
	}

	R_HeightSecClip(spr, x1, x2);

	if (spr->cut & SC_TOP && spr->cut & SC_BOTTOM)
	{
		for (x = x1; x <= x2; x++)
		{
			if (spr->cliptop[x] == -2 || spr->szt > spr->cliptop[x])
				spr->cliptop[x] = spr->szt;

			if (spr->clipbot[x] == -2 || spr->sz < spr->clipbot[x])
				spr->clipbot[x] = spr->sz;
		}
	}
	else if (spr->cut & SC_TOP)
	{
		for (x = x1; x <= x2; x++)
		{
			if (spr->cliptop[x] == -2 || spr->szt > spr->cliptop[x])
				spr->cliptop[x] = spr->szt;
		}
	}
	else if (spr->cut & SC_BOTTOM)
	{
		for (x = x1; x <= x2; x++)
		{
			if (spr->clipbot[x] == -2 || spr->sz < spr->clipbot[x])
				spr->clipbot[x] = spr->sz;
		}
	}

	// all clipping has been performed, so store the values - what, did you think we were drawing them NOW?

	// check for unclipped columns
	for (x = x1; x <= x2; x++)
	{
		if (spr->clipbot[x] == -2)
			spr->clipbot[x] = (INT16)viewheight;

		if (spr->cliptop[x] == -2)
			spr->cliptop[x] = -1;
	}

	if (portal)
	{
		INT32 start_index = max(portal->start, x1);
		INT32 end_index = min(portal->start + portal->end - portal->start, x2);
		for (x = x1; x < start_index; x++)
		{
			spr->clipbot[x] = -1;
			spr->cliptop[x] = -1;
		}
		for (x = start_index; x <= end_index; x++)
		{
			if (spr->clipbot[x] > portal->floorclip[x - portal->start])
				spr->clipbot[x] = portal->floorclip[x - portal->start];
			if (spr->cliptop[x] < portal->ceilingclip[x - portal->start])
				spr->cliptop[x] = portal->ceilingclip[x - portal->start];
		}
		for (x = end_index + 1; x <= x2; x++)
		{
			spr->clipbot[x] = -1;
			spr->cliptop[x] = -1;
		}
	}

	// Check if it'll be visible
	// Not done for floorsprites.
	if (cv_spriteclip.value && (spr->cut & SC_SPLAT) == 0)
	{
		if (!R_CheckSpriteVisible(spr, x1, x2))
			spr->cut |= SC_NOTVISIBLE;
	}
}

void R_ClipSprites(drawseg_t* dsstart, portal_t* portal)
{
	const size_t maxdrawsegs = ds_p - drawsegs;
	const INT32 cx = viewwidth / 2;
	drawseg_t* ds;
	INT32 i;

	// e6y
	// Reducing of cache misses in the following R_DrawSprite()
	// Makes sense for scenes with huge amount of drawsegs.
	// ~12% of speed improvement on epic.wad map05
	for (i = 0; i < DS_RANGES_COUNT; i++)
	{
		drawsegs_xranges[i].count = 0;
	}

	if (visspritecount - clippedvissprites <= 0)
	{
		return;
	}

	if (drawsegs_xrange_size < maxdrawsegs)
	{
		drawsegs_xrange_size = 2 * maxdrawsegs;

		for (i = 0; i < DS_RANGES_COUNT; i++)
		{
			drawsegs_xranges[i].items = Z_Realloc(
				drawsegs_xranges[i].items,
				drawsegs_xrange_size * sizeof(drawsegs_xranges[i].items[0]),
				PU_STATIC, NULL
			);
		}
	}

	for (ds = ds_p; ds-- > dsstart;)
	{
		if (ds->silhouette || ds->maskedtexturecol)
		{
			drawsegs_xranges[0].items[drawsegs_xranges[0].count].x1 = ds->x1;
			drawsegs_xranges[0].items[drawsegs_xranges[0].count].x2 = ds->x2;
			drawsegs_xranges[0].items[drawsegs_xranges[0].count].user = ds;

			// e6y: ~13% of speed improvement on sunder.wad map10
			if (ds->x1 < cx)
			{
				drawsegs_xranges[1].items[drawsegs_xranges[1].count] =
					drawsegs_xranges[0].items[drawsegs_xranges[0].count];
				drawsegs_xranges[1].count++;
			}

			if (ds->x2 >= cx)
			{
				drawsegs_xranges[2].items[drawsegs_xranges[2].count] =
					drawsegs_xranges[0].items[drawsegs_xranges[0].count];
				drawsegs_xranges[2].count++;
			}

			drawsegs_xranges[0].count++;
		}
	}

	for (; clippedvissprites < visspritecount; clippedvissprites++)
	{
		vissprite_t *spr = R_GetVisSprite(clippedvissprites);

		if (cv_spriteclip.value
		&& (spr->szt > vid.height || spr->sz < 0)
		&& !((spr->cut & SC_SPLAT) || spr->scalestep))
		{
			spr->cut |= SC_NOTVISIBLE;
			continue;
		}

		if (spr->cut & SC_BBOX)
		{
			numvisiblesprites++;
			continue;
		}

		INT32 x1 = (spr->cut & SC_SPLAT) ? 0 : spr->x1;
		INT32 x2 = (spr->cut & SC_SPLAT) ? viewwidth : spr->x2;

		if (x2 < cx)
		{
			drawsegs_xrange = drawsegs_xranges[1].items;
			drawsegs_xrange_count = drawsegs_xranges[1].count;
		}
		else if (x1 >= cx)
		{
			drawsegs_xrange = drawsegs_xranges[2].items;
			drawsegs_xrange_count = drawsegs_xranges[2].count;
		}
		else
		{
			drawsegs_xrange = drawsegs_xranges[0].items;
			drawsegs_xrange_count = drawsegs_xranges[0].count;
		}

		R_ClipVisSprite(spr, x1, x2, portal);

		if ((spr->cut & SC_NOTVISIBLE) == 0)
			numvisiblesprites++;
	}
}

/* Check if thing may be drawn from our current view. */
boolean R_ThingVisible (mobj_t *thing)
{
	return (!(
		(thing->sprite == SPR_NULL) || // Don't draw null-sprites
		(thing->flags2 & MF2_DONTDRAW) || // Don't draw MF2_LINKDRAW objects
		(thing->drawonlyforplayer && thing->drawonlyforplayer != viewplayer) || // Don't draw other players' personal objects
		(!R_BlendLevelVisible(thing->blendmode, R_GetThingTransTable(thing->alpha, 0))) ||
		(!P_MobjWasRemoved(r_viewmobj) && (
		  (r_viewmobj == thing) || // Don't draw first-person players or awayviewmobj objects
		  (r_viewmobj->player && r_viewmobj->player->followmobj == thing) || // Don't draw first-person players' followmobj
		  (r_viewmobj == thing->dontdrawforviewmobj) // Don't draw objects that are hidden for the current view
		))
	));
}

boolean R_ThingWithinDist (mobj_t *thing,
		fixed_t      limit_dist,
		fixed_t hoop_limit_dist)
{
	const fixed_t approx_dist = P_AproxDistance(viewx-thing->x, viewy-thing->y);

	if (thing->sprite == SPR_HOOP)
	{
		if (hoop_limit_dist && approx_dist > hoop_limit_dist)
			return false;
	}
	else
	{
		if (limit_dist && approx_dist > limit_dist)
			return false;
	}

	return true;
}

/* Check if precipitation may be drawn from our current view. */
boolean R_PrecipThingVisible (precipmobj_t *precipthing,
		fixed_t limit_dist)
{
	fixed_t approx_dist;

	if (( precipthing->precipflags & PCF_INVISIBLE ))
		return false;

	approx_dist = P_AproxDistance(viewx-precipthing->x, viewy-precipthing->y);

	return ( approx_dist <= limit_dist );
}

boolean R_ThingHorizontallyFlipped(mobj_t *thing)
{
	return (thing->frame & FF_HORIZONTALFLIP || thing->renderflags & RF_HORIZONTALFLIP);
}

boolean R_ThingVerticallyFlipped(mobj_t *thing)
{
	return (thing->frame & FF_VERTICALFLIP || thing->renderflags & RF_VERTICALFLIP);
}

boolean R_ThingIsPaperSprite(mobj_t *thing)
{
	return (thing->frame & FF_PAPERSPRITE || thing->renderflags & RF_PAPERSPRITE);
}

boolean R_ThingIsFloorSprite(mobj_t *thing)
{
	return (thing->flags2 & MF2_SPLAT || thing->frame & FF_FLOORSPRITE || thing->renderflags & RF_FLOORSPRITE);
}

boolean R_ThingIsFullBright(mobj_t *thing)
{
	return ((thing->frame & FF_BRIGHTMASK) == FF_FULLBRIGHT || (thing->renderflags & RF_BRIGHTMASK) == RF_FULLBRIGHT);
}

boolean R_ThingIsSemiBright(mobj_t *thing)
{
	return ((thing->frame & FF_BRIGHTMASK) == FF_SEMIBRIGHT || (thing->renderflags & RF_BRIGHTMASK) == RF_SEMIBRIGHT);
}

boolean R_ThingIsFullDark(mobj_t *thing)
{
	return ((thing->frame & FF_BRIGHTMASK) == FF_FULLDARK || (thing->renderflags & RF_BRIGHTMASK) == RF_FULLDARK);
}

boolean R_ThingIsFlashing(mobj_t *thing)
{
	if (thing == NULL)
		return false;

	return (thing->flags & (MF_ENEMY|MF_BOSS)) && (thing->flags2 & MF2_FRET) && !(thing->flags & MF_GRENADEBOUNCE) && (leveltime & 1);
}

// Offsets MT_OVERLAY towards the camera at render-time - Works in splitscreen!
// The &x and &y arguments should be pre-interpolated, and will be modified
void R_ThingOffsetOverlay(mobj_t *thing, fixed_t *x, fixed_t *y)
{
	mobj_t *mobj = thing;
	INT16 offset = 0; // Offset towards or away from the camera, and how much
	fixed_t offsetscale = thing->scale; // Occasionally needs to be interpolated
	angle_t viewingangle;
	UINT8 looplimit = 255; // Prevent infinite loops - A chain of 255 connected overlays is enough for any sane use case

#ifdef PARANOIA
	if (P_MobjWasRemoved(mobj) || !x || !y)
		I_Error("R_ThingOffsetOverlay: thing, x, or y is invalid");
#endif

	do // Get the overlay's offset
	{
		// Does the overlay use FF_ANIMATE? If not, if var1 is non-zero, it's an underlay instead of an overlay
		if (!(mobj->state->frame & FF_ANIMATE) && mobj->state->var1)
			offset += 1; // Underlay below the target, away from the camera
		else
			offset -= 1; // Overlay on top of the target, towards the camera

		looplimit -= 1;
		mobj = mobj->target;
	} while (!P_MobjWasRemoved(mobj) && mobj->type == MT_OVERLAY && looplimit > 0); // Handle overlays following other overlays

	// Does the offset scale need to be interpolated?
	if (thing->scale != thing->old_scale && R_UsingFrameInterpolation() && !paused)
	{
		interpmobjstate_t interp = {0};
		R_InterpolateMobjState(thing, rendertimefrac, &interp);
		offsetscale = interp.scale;
	}


	// Get the angle from the camera to the X and Y coordinates
	viewingangle = R_PointToAngle(*x, *y);

	// Finally, offset the X and Y coordinates towards or away from the camera
	*x += P_ReturnThrustX(thing, viewingangle, FixedMul(offset * (FRACUNIT/4), offsetscale));
	*y += P_ReturnThrustY(thing, viewingangle, FixedMul(offset * (FRACUNIT/4), offsetscale));
}

//
// R_DrawMasked
//
static void R_DrawMaskedList (drawnode_t* head)
{
	drawnode_t *r2;
	drawnode_t *next;

	for (r2 = head->next; r2 != head; r2 = r2->next)
	{
		if (r2->plane)
		{
			next = r2->prev;
			R_DrawSinglePlane(r2->plane);
			R_DoneWithNode(r2);
			r2 = next;
		}
		else if (r2->seg && r2->seg->maskedtexturecol != NULL)
		{
			next = r2->prev;
			R_RenderMaskedSegRange(r2->seg, r2->seg->x1, r2->seg->x2);
			r2->seg->maskedtexturecol = NULL;
			R_DoneWithNode(r2);
			r2 = next;
		}
		else if (r2->thickseg)
		{
			next = r2->prev;
			R_RenderThickSideRange(r2->thickseg, r2->thickseg->x1, r2->thickseg->x2, r2->ffloor);
			R_DoneWithNode(r2);
			r2 = next;
		}
		else if (r2->sprite)
		{
			next = r2->prev;

			// Tails 08-18-2002
			if (r2->sprite->cut & SC_PRECIP)
				R_DrawPrecipitationSprite(r2->sprite);
			else if (!r2->sprite->linkdraw)
				R_DrawSprite(r2->sprite);
			else // unbundle linkdraw
			{
				vissprite_t *ds = r2->sprite->linkdraw;

				for (;
					(ds != NULL && r2->sprite->dispoffset > ds->dispoffset);
					ds = ds->next)
				{
					R_DrawSprite(ds);
				}

				R_DrawSprite(r2->sprite);

				for (;
					ds != NULL;
					ds = ds->next)
				{
					R_DrawSprite(ds);
				}
			}

			R_DoneWithNode(r2);
			r2 = next;
		}
	}
}

void R_DrawMasked(maskcount_t* masks, INT32 nummasks)
{
	drawnode_t *heads;	/**< Drawnode lists; as many as number of views/portals. */
	INT32 i;

	heads = calloc(nummasks, sizeof(drawnode_t));

	for (i = 0; i < nummasks; i++)
	{
		heads[i].next = heads[i].prev = &heads[i];

		viewx = masks[i].viewx;
		viewy = masks[i].viewy;
		viewz = masks[i].viewz;
		viewsector = masks[i].viewsector;

		R_CreateDrawNodes(&masks[i], &heads[i], false);
	}

	//for (i = 0; i < nummasks; i++)
	//	CONS_Printf("Mask no.%d:\ndrawsegs: %d\n vissprites: %d\n\n", i, masks[i].drawsegs[1] - masks[i].drawsegs[0], masks[i].vissprites[1] - masks[i].vissprites[0]);

	for (; nummasks > 0; nummasks--)
	{
		viewx = masks[nummasks - 1].viewx;
		viewy = masks[nummasks - 1].viewy;
		viewz = masks[nummasks - 1].viewz;
		viewsector = masks[nummasks - 1].viewsector;

		R_DrawMaskedList(&heads[nummasks - 1]);
		R_ClearDrawNodes(&heads[nummasks - 1]);
	}

	free(heads);
}
