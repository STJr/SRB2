// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2013 James Haley et al.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
// Additional terms and conditions compatible with the GPLv3 apply. See the
// file COPYING-EE for details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//      Dynamic segs for PolyObject re-implementation.
//
//-----------------------------------------------------------------------------

#ifndef R_DYNSEG_H__
#define R_DYNSEG_H__

#include "r_defs.h"
#include "m_dllist.h"
#include "p_polyobj.h"

typedef struct dseglink_s
{
	mdllistitem_t link;
	struct dynaseg_s *dynaseg;
} dseglink_t;

typedef struct dynavertex_s // : vertex_t
{
	fixed_t x, y;
	boolean floorzset, ceilingzset;
	fixed_t floorz, ceilingz;
	struct dynavertex_s *dynanext;
	int refcount;
} dynavertex_t;

//
// dynaseg
//
typedef struct dynaseg_s
{
	seg_t seg; // a dynaseg is a seg, after all ;)

	dynavertex_t *originalv2;  // reference to original v2 before a split
	vertex_t *linev1, *linev2;   // dynavertices belonging to the endpoint segs

	struct dynaseg_s *subnext;         // next dynaseg in fragment
	struct dynaseg_s *freenext;        // next dynaseg on freelist
	polyobj_t *polyobj;  // polyobject

	dseglink_t ownerlink; // link for owning node chain
	dseglink_t alterlink; // link for non-dynaBSP segs changed by dynaBSP

	float prevlen, prevofs; // for interpolation (keep them out of seg_t)

	// properties needed for efficiency in the BSP builder
	double psx, psy, pex, pey; // end points
	double pdx, pdy;           // delta x, delta y
	double ptmp;               // general line coefficient 'c'
	double len;                // length
} dynaseg_t;

// Replaced dseglist_t with a different linked list implementation.
typedef struct dsegnode_s
{
	dynaseg_t *dynaseg;
	struct dsegnode_s *prev, *next;
} dsegnode_t;

//
// rpolyobj_t
//
// Subsectors now hold pointers to rpolyobj_t's instead of to polyobj_t's.
// An rpolyobj_t is a set of dynasegs belonging to a single polyobject.
// It is necessary to keep dynasegs belonging to different polyobjects
// separate from each other so that the renderer can continue to efficiently
// support multiple polyobjects per subsector (we do not want to do a z-sort
// on every single dynaseg, as that is significant unnecessary overhead).
//
typedef struct rpolyobj_s
{
	mdllistitem_t link; // for subsector links; must be first

	dynaseg_t         *dynaSegs; // list of dynasegs
	polyobj_t         *polyobj;  // polyobject of which this rpolyobj_t is a fragment
	struct rpolyobj_s *freenext; // next on freelist
} rpolyobj_t;

void P_CalcDynaSegLength(dynaseg_t *lseg);

dynavertex_t  *R_GetFreeDynaVertex(void);
void       R_FreeDynaVertex(dynavertex_t **vtx);
void       R_SetDynaVertexRef(dynavertex_t **target, dynavertex_t *vtx);
dynaseg_t *R_CreateDynaSeg(const dynaseg_t *proto, dynavertex_t *v1, dynavertex_t *v2);
void       R_FreeDynaSeg(dynaseg_t *dseg);

void R_AttachPolyObject(polyobj_t *poly);
void R_DetachPolyObject(polyobj_t *poly);
void R_ClearDynaSegs(void);

#endif

// EOF
