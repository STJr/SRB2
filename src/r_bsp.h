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
/// \file  r_bsp.h
/// \brief Refresh module, BSP traversal and handling

#ifndef __R_BSP__
#define __R_BSP__

#ifdef __GNUG__
#pragma interface
#endif

extern INT32 checkcoord[12][4];

struct bspcontext_s;
struct rendercontext_s;
struct viewcontext_s;

// BSP?
void R_ClearClipSegs(struct bspcontext_s *context, INT32 start, INT32 end);
void R_ClearDrawSegs(struct bspcontext_s *context);
void R_RenderBSPNode(struct rendercontext_s *context, INT32 bspnum);

void R_SortPolyObjects(subsector_t *sub);

extern size_t numpolys;        // number of polyobjects in current subsector
extern size_t num_po_ptrs;     // number of polyobject pointers allocated
extern polyobj_t **po_ptrs; // temp ptr array to sort polyobject pointers

sector_t *R_FakeFlat(struct viewcontext_s *viewcontext, sector_t *sec, sector_t *tempsec,
	INT32 *floorlightlevel, INT32 *ceilinglightlevel, boolean back);
boolean R_IsEmptyLine(seg_t *line, sector_t *front, sector_t *back);

INT32 R_GetPlaneLight(sector_t *sector, fixed_t planeheight, boolean underside);
void R_Prep3DFloors(sector_t *sector);
#endif
