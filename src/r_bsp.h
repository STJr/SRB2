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

extern seg_t *curline;
extern side_t *sidedef;
extern line_t *linedef;
extern sector_t *frontsector;
extern sector_t *backsector;
extern boolean portalline; // is curline a portal seg?

// drawsegs are allocated on the fly... see r_segs.c

extern INT32 checkcoord[12][4];

extern drawseg_t *curdrawsegs;
extern drawseg_t *drawsegs;
extern drawseg_t *ds_p;
extern INT32 doorclosed;

// BSP?
void R_ClearClipSegs(void);
void R_PortalClearClipSegs(INT32 start, INT32 end);
void R_ClearDrawSegs(void);
void R_RenderBSPNode(INT32 bspnum);

void R_SortPolyObjects(subsector_t *sub);

extern size_t numpolys;        // number of polyobjects in current subsector
extern size_t num_po_ptrs;     // number of polyobject pointers allocated
extern polyobj_t **po_ptrs; // temp ptr array to sort polyobject pointers

sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec, INT32 *floorlightlevel,
	INT32 *ceilinglightlevel, boolean back);
boolean R_IsEmptyLine(seg_t *line, sector_t *front, sector_t *back);

INT32 R_GetPlaneLight(sector_t *sector, fixed_t planeheight, boolean underside);
void R_Prep3DFloors(sector_t *sector);
#endif
