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
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    Dynamic BSP sub-trees for dynaseg sorting.
//
//-----------------------------------------------------------------------------

#ifndef R_DYNABSP_H__
#define R_DYNABSP_H__

#include "r_dynseg.h"

typedef struct rpolynode_s
{
	dynaseg_t          *partition;   // partition dynaseg
	struct rpolynode_s *children[2]; // child node lists (0=right, 1=left)
	dseglink_t         *owned;       // owned segs created by partition splits
	dseglink_t         *altered;     // polyobject-owned segs altered by partitions.
} rpolynode_t;

typedef struct rpolybsp_s
{
	boolean      dirty; // needs to be rebuilt if true
	rpolynode_t *root;  // root of tree
} rpolybsp_t;

rpolybsp_t *R_BuildDynaBSP(const subsector_t *subsec);
void R_FreeDynaBSP(rpolybsp_t *bsp);

//
// R_PointOnDynaSegSide
//
// Returns 0 for front/right, 1 for back/left.
//
static inline int R_PointOnDynaSegSide(const dynaseg_t *ds, float x, float y)
{
	return ((ds->pdx * (y - ds->psy)) >= (ds->pdy * (x - ds->psx)));
}

void R_ComputeIntersection(const dynaseg_t *part, const dynaseg_t *seg, double *outx, double *outy);

#endif

// EOF
