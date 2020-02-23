// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_splats.h
/// \brief Flat sprites & splats effects

#ifndef __R_SPLATS_H__
#define __R_SPLATS_H__

#include "r_defs.h"

//#define WALLSPLATS      // comment this out to compile without splat effects
/*#ifdef USEASM
#define FLOORSPLATS
#endif*/

#define MAXLEVELSPLATS      1024

// splat flags
#define SPLATDRAWMODE_MASK 0x03 // mask to get drawmode from flags
#define SPLATDRAWMODE_OPAQUE 0x00
#define SPLATDRAWMODE_SHADE 0x01
#define SPLATDRAWMODE_TRANS 0x02

// ==========================================================================
// DEFINITIONS
// ==========================================================================

// WALL SPLATS are patches drawn on top of wall segs
typedef struct wallsplat_s
{
	lumpnum_t patch; // lump id.
	vertex_t v1, v2; // vertices along the linedef
	fixed_t top;
	fixed_t offset; // offset in columns<<FRACBITS from start of linedef to start of splat
	INT32 flags;
	fixed_t *yoffset;
	line_t *line; // the parent line of the splat seg
	struct wallsplat_s *next;
} wallsplat_t;

// FLOOR SPLATS are pic_t (raw horizontally stored) drawn on top of the floor or ceiling
typedef struct floorsplat_s
{
	lumpnum_t pic; // a pic_t lump id
	INT32 flags;
	INT32 size; // 64, 128, 256, etc.
	vertex_t verts[4]; // (x,y) as viewn from above on map
	fixed_t z; // z (height) is constant for all the floorsplats
	subsector_t *subsector; // the parent subsector
	mobj_t *mobj; // Mobj it is tied to
	struct floorsplat_s *next;
	struct floorsplat_s *nextvis;
} floorsplat_t;

// p_setup.c
fixed_t P_SegLength(seg_t *seg);

// call at P_SetupLevel()
void R_ClearLevelSplats(void);

#ifdef WALLSPLATS
void R_AddWallSplat(line_t *wallline, INT16 sectorside, const char *patchname, fixed_t top,
	fixed_t wallfrac, INT32 flags);
#endif
#ifdef FLOORSPLATS
void R_AddFloorSplat(subsector_t *subsec, mobj_t *mobj, const char *picname, fixed_t x, fixed_t y, fixed_t z,
	INT32 flags);
#endif

void R_ClearVisibleFloorSplats(void);
void R_AddVisibleFloorSplats(subsector_t *subsec);
void R_DrawVisibleFloorSplats(void);

#endif /*__R_SPLATS_H__*/
