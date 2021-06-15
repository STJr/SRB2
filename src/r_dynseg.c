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

#include "z_zone.h"
#include "i_system.h"
#include "m_bbox.h"
#include "p_maputl.h"
#include "r_main.h"
#include "r_dynseg.h"
#include "r_dynabsp.h"
#include "r_state.h"

//
// dynaseg free list
//
// Let's do as little allocation as possible.
//
static dynaseg_t *dynaSegFreeList;

//
// dynaseg vertex free list
//
static dynavertex_t *dynaVertexFreeList;

//
// rpolyobj_t freelist
//
static rpolyobj_t *freePolyFragments;

//
// Used for dynasegs, not base segs
//
void P_CalcDynaSegLength(dynaseg_t *dynaseg)
{
	seg_t *lseg = &dynaseg->seg;
	lseg->length = P_SegLength(lseg);
#ifdef HWRENDER
	lseg->flength = FixedToFloat(lseg->length);
#endif
}

//
// R_AddDynaSubsec
//
// Keeps track of pointers to subsectors which hold dynasegs in a
// reallocating array, for purposes of later detaching the dynasegs.
// Each polyobject contains its own subsector array.
//
static void R_AddDynaSubsec(subsector_t *ss, polyobj_t *po)
{
	int i;

	// If the subsector has a BSP tree, it will need to be rebuilt.
	if(ss->bsp)
		ss->bsp->dirty = true;

	// make sure subsector is not already tracked
	for(i = 0; i < po->numDSS; ++i)
	{
		if(po->dynaSubsecs[i] == ss)
			return;
	}

	if(po->numDSS >= po->numDSSAlloc)
	{
		po->numDSSAlloc = po->numDSSAlloc ? po->numDSSAlloc * 2 : 8;
		po->dynaSubsecs = Z_Realloc(po->dynaSubsecs, po->numDSSAlloc * sizeof(subsector_t *), PU_LEVEL, NULL);
	}

	po->dynaSubsecs[po->numDSS++] = ss;
}

//
// R_GetFreeDynaVertex
//
// Gets a vertex from the free list or allocates a new one.
//
dynavertex_t *R_GetFreeDynaVertex(void)
{
	dynavertex_t *ret = NULL;

	if(dynaVertexFreeList)
	{
		ret = dynaVertexFreeList;
		dynaVertexFreeList = dynaVertexFreeList->dynanext;
		memset(ret, 0, sizeof(dynavertex_t));
	}
	else
		ret = calloc(sizeof(dynavertex_t), 1);

	return ret;
}

//
// R_FreeDynaVertex
//
// Puts a dynamic vertex onto the free list, if its refcount becomes zero.
//
void R_FreeDynaVertex(dynavertex_t **vtx)
{
	dynavertex_t *v;

	if(!*vtx)
		return;

	v = *vtx;

	if(v->refcount > 0)
	{
		v->refcount--;
		if(v->refcount == 0)
		{
			v->refcount = -1;
			v->dynanext = dynaVertexFreeList;
			dynaVertexFreeList = v;
		}
	}

	*vtx = NULL;
}

//
// R_SetDynaVertexRef
//
// Safely set a reference to a dynamic vertex, maintaining the reference count.
// Do not assign dynavertex pointers without using this routine! Note that if
// *target already points to a vertex, that vertex WILL be freed if its ref
// count reaches zero.
//
void R_SetDynaVertexRef(dynavertex_t **target, dynavertex_t *vtx)
{
	if(*target)
		R_FreeDynaVertex(target);

	if((*target = vtx))
		(*target)->refcount++;
}

//
// R_GetFreeDynaSeg
//
// Gets a dynaseg from the free list or allocates a new one.
//
static dynaseg_t *R_GetFreeDynaSeg(void)
{
	dynaseg_t *ret = NULL;

	if(dynaSegFreeList)
	{
		ret = dynaSegFreeList;
		dynaSegFreeList = dynaSegFreeList->freenext;
		memset(ret, 0, sizeof(dynaseg_t));
	}
	else
		ret = calloc(sizeof(dynaseg_t), 1);

	return ret;
}

//
// R_FreeDynaSeg
//
// Puts a dynaseg onto the free list.
//
void R_FreeDynaSeg(dynaseg_t *dseg)
{
	R_FreeDynaVertex(&dseg->seg.dyv1);
	R_FreeDynaVertex(&dseg->seg.dyv2);
	R_FreeDynaVertex(&dseg->originalv2);

	M_DLListRemove(&dseg->alterlink.link);  // remove it from alterable list
	dseg->freenext = dynaSegFreeList;
	dynaSegFreeList = dseg;
}

//
// R_GetFreeRPolyObj
//
// Gets an rpolyobj_t from the free list or creates a new one.
//
static rpolyobj_t *R_GetFreeRPolyObj(void)
{
	rpolyobj_t *ret = NULL;

	if(freePolyFragments)
	{
		ret = freePolyFragments;
		freePolyFragments = freePolyFragments->freenext;
		memset(ret, 0, sizeof(rpolyobj_t));
	}
	else
		ret = calloc(sizeof(rpolyobj_t), 1);

	return ret;
}

//
// R_FreeRPolyObj
//
// Puts an rpolyobj_t on the freelist.
//
static void R_FreeRPolyObj(rpolyobj_t *rpo)
{
	rpo->freenext = freePolyFragments;
	freePolyFragments = rpo;
}

//
// R_FindFragment
//
// Looks in the given subsector for a polyobject fragment corresponding
// to the given polyobject. If one is not found, then a new one is created
// and returned.
//
static rpolyobj_t *R_FindFragment(subsector_t *ss, polyobj_t *po)
{
	rpolyobj_t *link = ss->renderPolyList;
	rpolyobj_t *rpo;

	while(link)
	{
		if(link->polyobj == po)
			return link;

		link = (rpolyobj_t *)(link->link.next);
	}

	// there is not one, so create a new one and link it in
	rpo = R_GetFreeRPolyObj();

	rpo->polyobj = po;

	M_DLListInsert(&rpo->link, (mdllistitem_t **)(void *)(&ss->renderPolyList));

	return rpo;
}

//
// Calculates dynaseg offset using the originating seg's dynavertices.
//
static void R_calcDynaSegOffset(dynaseg_t *dynaseg, int side)
{
	fixed_t dx = (side ? dynaseg->linev2->x : dynaseg->linev1->x) - dynaseg->seg.v1->x;
	fixed_t dy = (side ? dynaseg->linev2->y : dynaseg->linev1->y) - dynaseg->seg.v1->y;
	dynaseg->seg.offset = FixedHypot(dx>>1, dy>>1)<<1;
}

//
// R_CreateDynaSeg
//
// Gets a new dynaseg and initializes it with all needed information.
//
dynaseg_t *R_CreateDynaSeg(const dynaseg_t *proto, dynavertex_t *v1, dynavertex_t *v2)
{
	dynaseg_t *ret = R_GetFreeDynaSeg();

	// properties inherited from prototype seg
	ret->polyobj     = proto->polyobj;
	ret->seg.linedef = proto->seg.linedef;
	ret->seg.sidedef = proto->seg.sidedef;
	ret->seg.side    = proto->seg.side;

	ret->linev1 = proto->linev1;
	ret->linev2 = proto->linev2;

	// vertices
	R_SetDynaVertexRef(&ret->seg.dyv1, v1);
	R_SetDynaVertexRef(&ret->seg.dyv2, v2);

	// calculate texture offset
	R_calcDynaSegOffset(ret, ret->seg.side);

	return ret;
}

//
// R_IntersectPoint
//
// Finds the point where a node line crosses a seg.
//
static boolean R_IntersectPoint(const seg_t *lseg, const node_t *node, dynavertex_t *nv)
{
	// get the fnode for the node
	fnode_t *bsp = &fnodes[node - nodes];

	double a1 = FixedToFloat(lseg->v2->y) - FixedToFloat(lseg->v1->y);
	double b1 = FixedToFloat(lseg->v1->x) - FixedToFloat(lseg->v2->x);
	double c1 = FixedToFloat(lseg->v2->x) * FixedToFloat(lseg->v1->y) - FixedToFloat(lseg->v1->x) * FixedToFloat(lseg->v2->y);

	// haleyjd 05/13/09: massive optimization
	double a2 = -bsp->a;
	double b2 = -bsp->b;
	double c2 = -bsp->c;

	double d = a1 * b2 - a2 * b1;
	float fx, fy;

	// lines are parallel?? shouldn't be.
	// FIXME: could this occur due to roundoff error in R_PointOnSide?
	//        Guess we'll find out the hard way ;)
	//        If so I'll need my own R_PointOnSide routine with some
	//        epsilon values.
	if(d == 0.0)
		return false;

	fx = (float)((b1 * c2 - b2 * c1) / d);
	fy = (float)((a2 * c1 - a1 * c2) / d);

	// set fixed-point coordinates
	nv->x = FloatToFixed(fx);
	nv->y = FloatToFixed(fy);

	return true;
}

//
// R_PartitionDistance
//
// This routine uses the general line equation, whose coefficients are now
// precalculated in the BSP nodes, to determine the distance of the point
// from the partition line. If the distance is too small, we may decide to
// change our idea of sidedness.
//
static inline double R_PartitionDistance(double x, double y, const fnode_t *node)
{
	return fabs((node->a * x + node->b * y + node->c) / node->len);
}

#define DS_EPSILON 0.3125

//
// Checks if seg is on top of a partition line
//
static boolean R_segIsOnPartition(const seg_t *seg, const subsector_t *frontss)
{
	const line_t *line;
	float midpx, midpy;
	int sign;

	if(seg->backsector)
		return true;

	line = seg->linedef;
	sign = line->frontsector == seg->frontsector ? 1 : -1;

	midpx = (float)((FixedToFloat(seg->v1->x) + FixedToFloat(seg->v2->x)) / 2 - line->nx * DS_EPSILON * sign);
	midpy = (float)((FixedToFloat(seg->v1->y) + FixedToFloat(seg->v2->y)) / 2 - line->ny * DS_EPSILON * sign);

	return (R_PointInSubsector(FloatToFixed(midpx), FloatToFixed(midpy)) != frontss);
}

//
// Checks the subsector for any wall segs which should cut or totally remove dseg.
// Necessary to avoid polyobject bleeding. Returns true if entire dynaseg is gone.
//
static boolean R_cutByWallSegs(dynaseg_t *dseg, dynaseg_t *backdseg, const subsector_t *ss)
{
	INT32 i;

	// The dynaseg must be in front of all wall segs. Otherwise, it's considered
	// hidden behind walls.
	seg_t *lseg = &dseg->seg;
	dseg->psx = FixedToFloat(lseg->v1->x);
	dseg->psy = FixedToFloat(lseg->v1->y);
	dseg->pex = FixedToFloat(lseg->v2->x);
	dseg->pey = FixedToFloat(lseg->v2->y);

	// Fast access to delta x, delta y
	dseg->pdx = dseg->pex - dseg->psx;
	dseg->pdy = dseg->pey - dseg->psy;

	for(i = 0; i < ss->numlines; ++i)
	{
		const seg_t *wall = &segs[ss->firstline + i];
		const vertex_t *v1 = wall->v1;
		const vertex_t *v2 = wall->v2;
		const divline_t walldl = { v1->x, v1->y, v2->x - v1->x, v2->y - v1->y };

		int side_v1, side_v2;
		dynaseg_t part;   // this shall be the wall

		double vx, vy;
		dynavertex_t *nv;

		if(R_segIsOnPartition(wall, ss))
			continue;   // only check 1-sided lines

		side_v1 = P_PointOnDivlineSidePrecise(lseg->v1->x, lseg->v1->y, &walldl);
		side_v2 = P_PointOnDivlineSidePrecise(lseg->v2->x, lseg->v2->y, &walldl);

		if(side_v1 == 0 && side_v2 == 0)
			continue;   // this one is fine.
		if(side_v1 == 1 && side_v2 == 1)
			return true;  // totally occluded by one

		// We have a real intersection: cut it now.
		part.psx = FixedToFloat(wall->v1->x);
		part.psy = FixedToFloat(wall->v1->y);
		part.pex = FixedToFloat(wall->v2->x);
		part.pey = FixedToFloat(wall->v2->y);

		R_ComputeIntersection(&part, dseg, &vx, &vy);

		nv = R_GetFreeDynaVertex();
		nv->x = FloatToFixed(vx);
		nv->y = FloatToFixed(vy);

		if(side_v1 == 0)
		{
			R_SetDynaVertexRef(&lseg->dyv2, nv);
			if(backdseg)
			{
				R_SetDynaVertexRef(&backdseg->seg.dyv1, nv);
				R_calcDynaSegOffset(backdseg, 1);
			}
		}
		else
		{
			R_SetDynaVertexRef(&lseg->dyv1, nv);
			R_calcDynaSegOffset(dseg, 0); // also need to update this
			if(backdseg)
				R_SetDynaVertexRef(&backdseg->seg.dyv2, nv);
		}
		// Keep looking for other intersectors
	}
	return false;   // all are in front. So return.
}

//
// R_SplitLine
//
// Given a single dynaseg representing the full length of a linedef, generates a
// set of dynasegs by recursively splitting the line through the BSP tree.
// Also does the same for a back dynaseg for 2-sided lines.
//
static void R_SplitLine(dynaseg_t *dseg, dynaseg_t *backdseg, int bspnum)
{
	int num;
	rpolyobj_t *fragment;

	while(!(bspnum & NF_SUBSECTOR))
	{
		const node_t  *bsp   = &nodes[bspnum];
		const fnode_t *fnode = &fnodes[bspnum];
		const seg_t   *lseg  = &dseg->seg;

		// test vertices against node line
		int side_v1 = R_PointOnSide(lseg->v1->x, lseg->v1->y, bsp);
		int side_v2 = R_PointOnSide(lseg->v2->x, lseg->v2->y, bsp);

		// ioanch 20160226: fix the polyobject visual clipping bug
		M_AddToBox(bsp->bbox[side_v1], lseg->v1->x, lseg->v1->y);
		M_AddToBox(bsp->bbox[side_v2], lseg->v2->x, lseg->v2->y);

		// get distance of vertices from partition line
		double dist_v1 = R_PartitionDistance(FixedToFloat(lseg->v1->x), FixedToFloat(lseg->v1->y), fnode);
		double dist_v2 = R_PartitionDistance(FixedToFloat(lseg->v2->x), FixedToFloat(lseg->v2->y), fnode);

		// If the distances are less than epsilon, consider the points as being
		// on the same side as the polyobj origin. Why? People like to build
		// polyobject doors flush with their door tracks. This breaks using the
		// usual assumptions.
		if(dist_v1 <= DS_EPSILON)
		{
			if(dist_v2 <= DS_EPSILON)
			{
				// both vertices are within epsilon distance; classify the seg
				// with respect to the polyobject center point
				side_v1 = side_v2 = R_PointOnSide(dseg->polyobj->centerPt.x, dseg->polyobj->centerPt.y, bsp);
			}
			else
				side_v1 = side_v2; // v1 is very close; classify as v2 side
		}
		else if(dist_v2 <= DS_EPSILON)
		{
			side_v2 = side_v1; // v2 is very close; classify as v1 side
		}

		if(side_v1 != side_v2)
		{
			// the partition line crosses this seg, so we must split it.
			dynavertex_t *nv = R_GetFreeDynaVertex();
			dynaseg_t *nds;

			if(R_IntersectPoint(lseg, bsp, nv))
			{
				dynaseg_t *backnds;

				// ioanch 20160722: fix the polyobject visual clipping bug (more needed)
				M_AddToBox(bsp->bbox[0], nv->x, nv->y);
				M_AddToBox(bsp->bbox[1], nv->x, nv->y);

				// create new dynaseg from nv to seg->v2
				nds = R_CreateDynaSeg(dseg, nv, lseg->dyv2);

				// alter current seg to run from seg->v1 to nv
				R_SetDynaVertexRef(&lseg->dyv2, nv);

				if(backdseg)
				{
					backnds = R_CreateDynaSeg(backdseg, backdseg->seg.dyv1, nv);
					R_SetDynaVertexRef(&backdseg->seg.dyv1, nv);
					R_calcDynaSegOffset(backdseg, 1);
				}
				else
					backnds = NULL;

				// recurse to split v2 side
				R_SplitLine(nds, backnds, bsp->children[side_v2]);
			}
			else
			{
				// Classification failed (this really should not happen, but, math
				// on computers is not ideal...). Return the dynavertex and do
				// nothing here; the seg will be classified on v1 side for lack of
				// anything better to do with it.
				R_FreeDynaVertex(&nv);
			}
		}

		// continue on v1 side
		bspnum = bsp->children[side_v1];
	}

	// reached a subsector: attach dynaseg
	num = bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR;

#ifdef RANGECHECK
	if(num >= numsubsectors)
		I_Error("R_SplitLine: ss %d with numss = %d\n", num, numsubsectors);
#endif

	// First, cut it off by any wall segs
	if(R_cutByWallSegs(dseg, backdseg, &subsectors[num]))
	{
		// If it's occluded by everything, cancel it.
		R_FreeDynaSeg(dseg);
		if(backdseg)
			R_FreeDynaSeg(backdseg);
		return;
	}

	// see if this subsector already has an rpolyobj_t for this polyobject
	// if it does not, then one will be created.
	fragment = R_FindFragment(&subsectors[num], dseg->polyobj);

	// link this seg in at the end of the list in the rpolyobj_t
	if(fragment->dynaSegs)
	{
		dynaseg_t *fdseg = fragment->dynaSegs;

		while(fdseg->subnext)
			fdseg = fdseg->subnext;

		fdseg->subnext = dseg;
	}
	else
		fragment->dynaSegs = dseg;
	dseg->subnext = backdseg;

	// 05/13/09: calculate seg length for SoM
	P_CalcDynaSegLength(dseg);
	if(backdseg)
		backdseg->seg.length = dseg->seg.length;

	// 07/15/09: rendering consistency - set frontsector/backsector here
	dseg->seg.polysector = subsectors[num].sector;
	dseg->seg.frontsector = dseg->seg.linedef->frontsector;

	// 10/30/09: only set backsector if line is 2S
	if(dseg->seg.linedef->backsector)
		dseg->seg.backsector = dseg->seg.linedef->backsector;
	else
		dseg->seg.backsector = NULL;

	if(backdseg)
	{
		backdseg->seg.polysector = subsectors[num].sector;
		backdseg->seg.frontsector = dseg->seg.linedef->frontsector;
		backdseg->seg.backsector = dseg->seg.linedef->backsector;
	}

	// add the subsector if it hasn't been added already
	R_AddDynaSubsec(&subsectors[num], dseg->polyobj);
}

//
// R_AttachPolyObject
//
// Generates dynamic segs for a single polyobject.
//
void R_AttachPolyObject(polyobj_t *poly)
{
	size_t i;

	// iterate on the polyobject lines array
	for(i = 0; i < poly->numLines; ++i)
	{
		line_t *line = poly->lines[i];
		side_t *side = &sides[line->sidenum[0]];
		dynaseg_t *backdseg;

		// create initial dseg representing the entire linedef
		dynaseg_t *idseg = R_GetFreeDynaSeg();

		dynavertex_t *v1 = R_GetFreeDynaVertex();
		dynavertex_t *v2 = R_GetFreeDynaVertex();

		memcpy(v1, line->v1, sizeof(vertex_t));
		memcpy(v2, line->v2, sizeof(vertex_t));

		idseg->polyobj     = poly;
		idseg->seg.linedef = line;
		idseg->seg.sidedef = side;

		R_SetDynaVertexRef(&idseg->seg.dyv1, v1);
		R_SetDynaVertexRef(&idseg->seg.dyv2, v2);
		idseg->linev1 = line->v1;
		idseg->linev2 = line->v2;

		// create backside dynaseg now
		if (!(poly->flags & POF_ONESIDE))
		{
			backdseg = R_GetFreeDynaSeg();
			backdseg->polyobj = poly;
			backdseg->seg.side = 1;
			backdseg->seg.linedef = line;
			backdseg->seg.sidedef = side;
			R_SetDynaVertexRef(&backdseg->seg.dyv1, v2);
			R_SetDynaVertexRef(&backdseg->seg.dyv2, v1);
			backdseg->linev1 = line->v1;
			backdseg->linev2 = line->v2;
		}
		else
			backdseg = NULL;

		// Split seg into BSP tree to generate more dynasegs;
		// The dynasegs are stored in the subsectors in which they finally end up.
		R_SplitLine(idseg, backdseg, numnodes - 1);
	}

	poly->attached = true;
}

//
// R_DetachPolyObject
//
// Removes a polyobject from all subsectors to which it is attached, reclaiming
// all dynasegs, vertices, and rpolyobj_t fragment objects associated with the
// given polyobject.
//
void R_DetachPolyObject(polyobj_t *poly)
{
	int i;

	// no dynaseg-containing subsecs?
	if(!poly->dynaSubsecs || !poly->numDSS)
	{
		poly->attached = false;
		return;
	}

	// iterate over stored subsector pointers
	for(i = 0; i < poly->numDSS; ++i)
	{
		subsector_t *ss = poly->dynaSubsecs[i];
		rpolyobj_t *link = ss->renderPolyList;
		rpolyobj_t *next;

		// mark BSPs dirty
		if(ss->bsp)
			ss->bsp->dirty = true;

		// iterate on subsector rpolyobj_t lists
		while(link)
		{
			rpolyobj_t *rpo = link;
			next = (rpolyobj_t *)(rpo->link.next);

			if(rpo->polyobj == poly)
			{
				// iterate on segs in rpolyobj_t
				while(rpo->dynaSegs)
				{
					dynaseg_t *ds     = rpo->dynaSegs;
					dynaseg_t *nextds = ds->subnext;

					// free dynamic vertices
					// put this dynaseg on the free list
					R_FreeDynaSeg(ds);

					rpo->dynaSegs = nextds;
				}

				// unlink this rpolyobj_t
				M_DLListRemove(&link->link);

				// put it on the freelist
				R_FreeRPolyObj(rpo);
			}

			link = next;
		}

		// no longer tracking this subsector
		poly->dynaSubsecs[i] = NULL;
	}

	// no longer tracking any subsectors
	poly->numDSS = 0;
	poly->attached = false;
}

//
// R_ClearDynaSegs
//
// Call at the end of a level to clear all dynasegs.
//
// If this were not done, all dynasegs, their vertices, and polyobj fragments
// would be lost.
//
void R_ClearDynaSegs(void)
{
	size_t i;

	for(i = 0; i < (unsigned)numPolyObjects; i++)
		R_DetachPolyObject(&PolyObjects[i]);

	for(i = 0; i < numsubsectors; i++)
	{
		if(subsectors[i].bsp)
			R_FreeDynaBSP(subsectors[i].bsp);
	}
}

// EOF
