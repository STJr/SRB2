// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 James Haley et al.
//
// Portions derived from BSP 5.2, "Node builder for DOOM levels"
//   (c) 1996 Raphael Quinet
//   (c) 1998 Colin Reed, Lee Killough
//   (c) 2001 Simon Howard
//   (c) 2006 Colin Phipps
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

#include "z_zone.h"

#include "p_setup.h"
#include "r_dynabsp.h"

//=============================================================================
//
// rpolynode Maintenance
//

static rpolynode_t *polyNodeFreeList;

//
// R_GetFreePolyNode
//
// Gets a node from the free list or allocates a new one.
//
static rpolynode_t *R_GetFreePolyNode(void)
{
	rpolynode_t *ret = NULL;

	if(polyNodeFreeList)
	{
		ret = polyNodeFreeList;
		polyNodeFreeList = polyNodeFreeList->children[0];
		memset(ret, 0, sizeof(*ret));
	}
	else
		ret = calloc(sizeof(rpolynode_t), 1);

	return ret;
}

//
// R_FreePolyNode
//
// Puts a dynamic node onto the free list.
//
static void R_FreePolyNode(rpolynode_t *rpn)
{
	rpn->children[0] = polyNodeFreeList;
	polyNodeFreeList = rpn;
}

//=============================================================================
//
// Dynaseg Setup
//

static void InsertIntoNodeList(dsegnode_t **tail, dynaseg_t *seg)
{
	dsegnode_t *node = Z_Calloc(sizeof(dsegnode_t), PU_STATIC, NULL);
	node->dynaseg = seg;

	if (*tail)
	{
		node->prev = (*tail);
		(*tail)->next = node;
	}

	(*tail) = node;
}

static void RemoveFromNodeList(dsegnode_t *node, dsegnode_t **tail, dsegnode_t **head)
{
	if (node->next)
		node->next->prev = node->prev;
	else
		(*tail) = node->prev;

	if (node->prev)
		node->prev->next = node->next;
	else
		(*head) = node->next;

	Z_Free(node);
}

static void FindNodeListHead(dsegnode_t **list)
{
	if (!*list)
		return;
	while ((*list)->prev)
		(*list) = (*list)->prev;
}

static void FindNodeListTail(dsegnode_t **list)
{
	if (!*list)
		return;
	while ((*list)->next)
		(*list) = (*list)->next;
}

//
// R_setupDSForBSP
//
// Dynasegs need a small amount of preparation in order to achieve maximum
// efficiency during the node building process.
//
static void R_setupDSForBSP(dynaseg_t *ds)
{
	// Fast access to double-precision coordinates
	ds->psx = FixedToFloat(ds->seg.v1->x);
	ds->psy = FixedToFloat(ds->seg.v1->y);
	ds->pex = FixedToFloat(ds->seg.v2->x);
	ds->pey = FixedToFloat(ds->seg.v2->y);

	// Fast access to delta x, delta y
	ds->pdx = ds->pex - ds->psx;
	ds->pdy = ds->pey - ds->psy;

	// General line equation coefficient 'c'
	ds->ptmp = ds->pdx * ds->psy - ds->psx * ds->pdy;

	// Length - we DEFINITELY don't want to do this any more than necessary
	ds->len = sqrt(ds->pdx * ds->pdx + ds->pdy * ds->pdy);
}

//=============================================================================
//
// Partition Selection
//

// I am not TOO worried about accuracy during partition selection, but...
#define CHECK_EPSILON 0.0001

static inline boolean nearzero(double d)
{
	return (d < CHECK_EPSILON && d > -CHECK_EPSILON);
}

// split weighting factor
// I have no idea why "17" is good, but let's stick with it.
#define FACTOR 17

//
// R_selectPartition
//
// This routine decides the best dynaseg to use as a nodeline by attempting to
// minimize the number of splits it would cause in remaining dynasegs.
//
// Credit to Raphael Quinet and DEU.
//
// Rewritten by Lee Killough for significant performance increases.
//  (haleyjd - using gotos, naturally ;)
//
static dsegnode_t *R_selectPartition(dsegnode_t *segs)
{
	dsegnode_t *rover;
	dsegnode_t *best = NULL;
	int bestcost = INT_MAX;
	int cnt = 0;

	// count the number of segs on the input list
	for(rover = segs; rover; rover = rover->next)
		++cnt;

	// Try each seg as a partition line
	for(rover = segs; rover; rover = rover->next)
	{
		dynaseg_t  *part = rover->dynaseg;
		dsegnode_t *crover;
		int cost = 0, tot = 0, diff = cnt;

		// haleyjd: add one seg worth of cost to non-orthogonal lines
		if(part->seg.linedef->slopetype > ST_VERTICAL)
			cost += FACTOR;

		// Check partition against all segs
		for(crover = segs; crover; crover = crover->next)
		{
			dynaseg_t *check = crover->dynaseg;

			// classify both end points
			double a = part->pdy * check->psx - part->pdx * check->psy + part->ptmp;
			double b = part->pdy * check->pex - part->pdx * check->pey + part->ptmp;

			if(!(a*b >= 0)) // oppositely signed?
			{
				if(!nearzero(a) && !nearzero(b)) // not within epsilon of 0?
				{
					// line is split
					double l = check->len;
					double d = (l * a) / (a - b); // distance from intersection

					if(d >= 2.0)
					{
						cost += FACTOR;

						// killough: This is the heart of my pruning idea - it
						// catches bad segs early on.
						if(cost > bestcost)
							goto prune;

						++tot;
					}
					else if(l - d < 2.0 ? check->pdx * part->pdx + check->pdy * part->pdy < 0 : b < 0)
						goto leftside;
				}
				else
					goto leftside;
			}
			else if(a <= 0 && (!nearzero(a) ||
				(nearzero(b) && check->pdx * part->pdx + check->pdy * part->pdy < 0)))
			{
leftside:
				diff -= 2;
			}
		} // end for

		// Take absolute value; diff is being used to obtain the min/max values
		// by way of min(a, b) = (a + b - abs(a - b)) / 2
		if((diff -= tot) < 0)
			diff = -diff;

		// Make sure at least one seg is on each side of the partition
		if(tot + cnt > diff && (cost += diff) < bestcost)
		{
			// we have a new better choice
			bestcost = cost;
			best = rover;     // remember the best seg
		}
prune: ; // early exit and skip past the tests above
	} // end for

	// haleyjd: failsafe. Maybe there's just one left in the list. I'm not
	// taking any chances that the above algorithm might freak out when that
	// becomes the case. I KNOW the list is not empty.
	if(!best)
		best = segs;

	return best; // All finished, return best seg
}

//=============================================================================
//
// Tree Building
//

//
// Calculate the point of intersection of two lines
//
void R_ComputeIntersection(const dynaseg_t *part, const dynaseg_t *seg, double *outx, double *outy)
{
	double a2, b2, l2, w, d;
	double dx, dy, dx2, dy2;

	dx  = part->pex - part->psx;
	dy  = part->pey - part->psy;
	dx2 = seg->pex  - seg->psx;
	dy2 = seg->pey  - seg->psy;

	l2 = sqrt(dx2*dx2 + dy2*dy2);

	if(l2 == 0.0)
	{
		// feh.
		*outx = seg->psx;
		*outy = seg->psy;
		return;
	}

	a2 = dx2 / l2;
	b2 = dy2 / l2;
	d  = dy * a2 - dx * b2;

	if(d != 0.0)
	{
		w = (dx * (seg->psy - part->psy) + dy * (part->psx - seg->psx)) / d;
		(*outx) = seg->psx + (a2 * w);
		(*outy) = seg->psy + (b2 * w);
	}
	else
	{
		(*outx) = seg->psx;
		(*outy) = seg->psy;
	}
}

// return-type enumeration for R_classifyDynaSeg
enum
{
	END_ON      = 0x01,
	END_LEFT    = 0x02,
	END_RIGHT   = 0x04,

	START_ON    = 0x10,
	START_LEFT  = 0x20,
	START_RIGHT = 0x40,

	// cases where seg must be split
	SPLIT_SL_ER = (START_LEFT  | END_RIGHT),
	SPLIT_SR_EL = (START_RIGHT | END_LEFT ),

	// cases where seg is not split
	CLASSIFY_LEFT  = (START_LEFT  | END_LEFT ),
	CLASSIFY_RIGHT = (START_RIGHT | END_RIGHT),
	CLASSIFY_ON    = (START_ON    | END_ON   )
};

//
// R_classifyDynaSeg
//
// The seg is either left, right, or on the partition line.
//
static int R_classifyDynaSeg(const dynaseg_t *part, const dynaseg_t *seg, double pdx, double pdy)
{
	double dx2, dy2, dx3, dy3, a, b;

	// check line against partition
	dx2 = part->psx - seg->psx;
	dy2 = part->psy - seg->psy;
	dx3 = part->psx - seg->pex;
	dy3 = part->psy - seg->pey;

	a = pdy * dx2 - pdx * dy2;
	b = pdy * dx3 - pdx * dy3;

	if(!(a*b >= 0) && !nearzero(a) && !nearzero(b))
	{
		double x = 0.0, y = 0.0;

		// line is split
		R_ComputeIntersection(part, seg, &x, &y);

		// find distance from line start to split point
		dx2 = seg->psx - x;
		dy2 = seg->psy - y;

		if(nearzero(dx2) && nearzero(dy2))
			a = 0.0;
		else
		{
			double l = dx2*dx2 + dy2*dy2;
			if(l < 4.0)
				a = 0.0;
		}

		dx3 = seg->pex - x;
		dy3 = seg->pey - y;

		if(nearzero(dx3) && nearzero(dy3))
			b = 0.0;
		else
		{
			double l = dx3*dx3 + dy3*dy3;
			if(l < 4.0)
				b = 0.0;
		}
	}

	int val = 0;

	if(nearzero(a))   // start is on the partition
		val |= START_ON;
	else if(a < 0.0)  // start is on left
		val |= START_LEFT;
	else              // start is on right
		val |= START_RIGHT;

	if(nearzero(b))   // end is on partition
		val |= END_ON;
	else if(b < 0.0)  // end is on left
		val |= END_LEFT;
	else              // end is on right
		val |= END_RIGHT;

	return val;
}


//
// R_divideSegs
//
// Split the input list of segs into left and right lists using one of the segs
// selected as a partition line for the current node.
//
static void R_divideSegs(rpolynode_t *rpn, dsegnode_t **ts, dsegnode_t **rs, dsegnode_t **ls)
{
	dynaseg_t *best, *add_to_rs = NULL, *add_to_ls = NULL;
	dsegnode_t **tail = ts, **head = ts, *partition;

	double pdx, pdy;
	dsegnode_t *cur;

	// select best seg to use as partition line
	partition = R_selectPartition(*ts);
	best = rpn->partition = partition->dynaseg;

	head = tail = ts;
	FindNodeListTail(tail);
	RemoveFromNodeList(partition, head, tail);

#ifdef RANGECHECK
	// Should not happen.
	if(!rpn->partition)
		I_Error("R_divideSegs: could not select partition!\n");
#endif

	// use the partition line to split any other lines in the list that intersect
	// it into left and right halves
	pdx = best->psx - best->pex;
	pdy = best->psy - best->pey;

	// iterate from beginning until the original list is empty
	while ((cur = *head))
	{
		dynaseg_t *seg = cur->dynaseg;
		add_to_ls = add_to_rs = NULL;

		int val = R_classifyDynaSeg(best, seg, pdx, pdy);

		if(val == SPLIT_SR_EL || val == SPLIT_SL_ER)
		{
			double x, y;

			// seg is split by the partition
			R_ComputeIntersection(best, seg, &x, &y);

			// create a new vertex at the intersection point
			dynavertex_t *nv = R_GetFreeDynaVertex();
			nv->x = FloatToFixed(x);
			nv->y = FloatToFixed(y);

			// create a new dynaseg from nv to v2
			dynaseg_t *nds = R_CreateDynaSeg(seg, nv, seg->seg.dyv2);
			R_setupDSForBSP(nds);
			nds->seg.polysector  = seg->seg.polysector;
			nds->seg.frontsector = seg->seg.frontsector;
			nds->seg.backsector  = seg->seg.backsector;
			nds->seg.length      = FloatToFixed(nds->len);

			// modify original seg to run from v1 to nv
			boolean notmarked = !seg->originalv2;
			if(notmarked)   // only if not already marked!
				R_SetDynaVertexRef(&seg->originalv2, seg->seg.dyv2);
			R_SetDynaVertexRef(&seg->seg.dyv2, nv);
			R_setupDSForBSP(seg);
			seg->seg.length = FloatToFixed(seg->len);

			// add the new seg to the current node's ownership list,
			// so it can get freed later
			M_DLListInsert(&nds->ownerlink.link, (mdllistitem_t **)(&rpn->owned));
			nds->ownerlink.link.next = NULL;
			nds->ownerlink.dynaseg = nds;

			if(notmarked)
			{
				M_DLListInsert(&seg->alterlink.link, (mdllistitem_t **)(&rpn->altered));
				seg->alterlink.link.next = NULL;
				seg->alterlink.dynaseg = seg;
			}

			// classify left or right
			if(val == SPLIT_SR_EL)
			{
				add_to_ls = nds;
				add_to_rs = seg;
			}
			else
			{
				add_to_ls = seg;
				add_to_rs = nds;
			}
		}
		else
		{
			// Not split; which side?
			if(val & CLASSIFY_LEFT)
				add_to_ls = seg; // at least one vertex is left, other is left or on
			if(val & CLASSIFY_RIGHT)
				add_to_rs = seg; // at least one vertex is right, other is right or on
			if(val == CLASSIFY_ON)
			{
				// We know the segs are parallel or nearly so; take their dot
				// product to determine their relative orientation
				if((seg->psx - seg->pex) * pdx + (seg->psy - seg->pey) * pdy < 0.0)
					add_to_ls = seg;
				else
					add_to_rs = seg;
			}
		}

		// add to right side?
		if(add_to_rs)
		{
			//CONS_Printf("Added %p into right side\n", cur);
			if (add_to_rs == seg)
				RemoveFromNodeList(cur, head, tail);
			InsertIntoNodeList(rs, add_to_rs);
		}

		// add to left side?
		if(add_to_ls)
		{
			//CONS_Printf("Added %p into left side\n", cur);
			if (add_to_ls == seg)
				RemoveFromNodeList(cur, head, tail);
			InsertIntoNodeList(ls, add_to_ls);
		}
	}

	FindNodeListHead(rs);
	FindNodeListHead(ls);
}

//
// R_createNode
//
// Primary recursive node building routine. Partition the segs using one of the
// segs as the partition line, then recurse into the back and front spaces until
// there are no segs left to classify.
//
// A tree of rpolynode instances is returned. NULL is returned in the terminal
// case where there are no segs left to classify.
//
static rpolynode_t *R_createNode(dsegnode_t **ts)
{
	dsegnode_t *rights = NULL;
	dsegnode_t *lefts  = NULL;

	if(!*ts)
		return NULL; // terminal case: empty list

	rpolynode_t *rpn = R_GetFreePolyNode();

	// divide the segs into two lists
	R_divideSegs(rpn, ts, &rights, &lefts);

	// recurse into right space
	rpn->children[0] = R_createNode(&rights);

	// recurse into left space
	rpn->children[1] = R_createNode(&lefts);

	return rpn;
}

//=============================================================================
//
// Transform subsector fragments
//

//
// R_collapseFragmentsToDSList
//
// Take a subsector and turn its list of rpolyobj fragments into a flat list of
// dynasegs linked by their bsplinks. The dynasegs' BSP-related fields will also
// be initialized. The result is suitable for input to R_BuildDynaBSP.
//
static boolean R_collapseFragmentsToDSList(const subsector_t *subsec, dsegnode_t **list)
{
	rpolyobj_t *fragment = subsec->renderPolyList;

	// Nothing to do? (We shouldn't really be called in that case, but, hey...)
	if(!fragment)
		return false;

	while(fragment)
	{
		dynaseg_t *ds = fragment->dynaSegs;

		while(ds)
		{
			R_setupDSForBSP(ds);

			InsertIntoNodeList(list, ds);

			// NB: fragment links are not disturbed by this process.
			ds = ds->subnext;
		}

		fragment = (rpolyobj_t *)(fragment->link.next);
	}

	FindNodeListHead(list);

	return (*list != NULL);
}

//=============================================================================
//
// Freeing
//
// Almost as much work as building it was :P
//

//
// R_returnOwnedList
//
// Return all dynasegs on a node's "owned" list. These are dynasegs that were
// created during the splitting process and are not referenced by rpolyobj_t
// instances.
//
static void R_returnOwnedList(rpolynode_t *node)
{
	dseglink_t *dsl = node->owned;

	while(dsl)
	{
		dseglink_t *next = (dseglink_t *)(dsl->link.next);
		dynaseg_t  *ds   = dsl->dynaseg;

		R_FreeDynaSeg(ds);

		dsl = next;
	}

	// Now also fix altered polyobject dynasegs
	dsl = node->altered;
	while(dsl)
	{
		dseglink_t *next = (dseglink_t *)(dsl->link.next);
		dynaseg_t *ds = dsl->dynaseg;
		R_SetDynaVertexRef(&ds->seg.dyv2, ds->originalv2);
		R_FreeDynaVertex(&ds->originalv2);
		P_CalcDynaSegLength(ds);
		M_DLListRemove(&dsl->link);
		dsl = next;
	}
}

//
// R_freeTreeRecursive
//
// Recursively free the BSP tree nodes.
//
static void R_freeTreeRecursive(rpolynode_t *root)
{
	if(!root)
		return;

	// free right and left sides
	R_freeTreeRecursive(root->children[0]);
	R_freeTreeRecursive(root->children[1]);

	// free resources stored in this node
	R_returnOwnedList(root);

	// return the bsp node
	R_FreePolyNode(root);
}

//=============================================================================
//
// External Interface
//

//
// R_BuildDynaBSP
//
// Call to build a dynamic BSP sub-tree for sorting of dynasegs.
//
rpolybsp_t *R_BuildDynaBSP(const subsector_t *subsec)
{
	rpolybsp_t *bsp  = NULL;
	dsegnode_t *segs = NULL;

	if(R_collapseFragmentsToDSList(subsec, &segs))
	{
		bsp = Z_Calloc(sizeof(rpolybsp_t), PU_LEVEL, NULL);
		bsp->dirty = false;
		bsp->root = R_createNode(&segs);
	}

	while (segs)
	{
		dsegnode_t *next = segs->next;
		Z_Free(segs);
		segs = next;
	}

	return bsp;
}

//
// R_FreeDynaBSP
//
// Return all resources owned by a dynamic BSP tree.
//
void R_FreeDynaBSP(rpolybsp_t *bsp)
{
	R_freeTreeRecursive(bsp->root);
	Z_Free(bsp);
}

// EOF
